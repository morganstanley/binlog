#ifndef BINLOG_ENTRIES_HPP
#define BINLOG_ENTRIES_HPP

#include <binlog/Range.hpp>
#include <binlog/Severity.hpp>

#include <mserialize/make_struct_deserializable.hpp>
#include <mserialize/make_struct_serializable.hpp>
#include <mserialize/serialize.hpp>

#include <cstdint>
#include <string>

/*
 * The structures below represent the entries
 * of a binlog stream. The entries are serialized as:
 *
 * u32 size | u64 tag | tag specific data
 *
 * The size is a 32 bit unsigned, little endian integer,
 * equal to the size of the remaining payload (tag+data).
 * The tag selects the entry type: tags with their
 * most significant bit set are reserved for special entries,
 * given below by SpecEntry::Tag static members.
 * Other tags indicate Events, where the tag
 * is the identifier of the matching Event Source.
 *
 * To ensure compatibility of entries across
 * versions, only add new fields to the end of the structure.
 */

namespace binlog {

/** Represents a piece of code which emits Events */
struct EventSource
{
  static constexpr std::uint64_t Tag = std::uint64_t(-1);

  std::uint64_t id = {};
  Severity severity = Severity::info;
  std::string category;
  std::string function;
  std::string file;
  std::uint64_t line = {};
  std::string formatString;
  std::string argumentTags; /**< mserialize::tag of the arguments */
};

/**
 * Represents a writer (thread, fiber, coroutine, task)
 * that triggers EventSources to produce events.
 *
 * `id` and `name` are free-form, can be anything
 * which helps the user to identify the writer.
 *
 * `batchSize` is a hint, equal to the size
 * of the events following this entry.
 * If specified, must be a valid offset to a later entry,
 * useful for fast forward seeking in the stream.
 * Can be zero if unknown.
 *
 * Events following directly are assumed to be produced
 * by the writer represented by this entry.
 */
struct WriterProp
{
  static constexpr std::uint64_t Tag = std::uint64_t(-2);

  std::uint64_t id = {};
  std::string name;
  std::uint64_t batchSize = {};
};

/**
 * Represents an equation between log clock and UTC time.
 *
 * The clock Events use for time stamping is not defined.
 * This allows them to use any clock suitable for the
 * application, e.g: std::chrono::system_clock,
 * clock_gettime, Time Stamp Counter, a simulated clock, etc.
 *
 * For a string representation, the value of this unspecified
 * clock must be first converted to a system_clock value.
 *
 * To be able to do the conversion, a connection has to be
 * made between the two clocks. This connection is
 * established by this entry:
 *
 * `clockValue` is the value of the unspecified log clock
 * at the time point `nsSinceEpoch`, which is given
 * as the nanoseconds since the UNIX epoch in UTC (not counting leap seconds).
 * `clockFrequency` is the number of log clock ticks per second.
 *
 * The time zone of the producer is also given:
 * `tzOffset` is the difference between UTC time and localtime at that time.
 * `tzName` is the (possibly abbreviated) name of the time zone.
 */
struct ClockSync
{
  static constexpr std::uint64_t Tag = std::uint64_t(-3);

  std::uint64_t clockValue = {};     /**< Clock when time is `nsSinceEpoch` */
  std::uint64_t clockFrequency = {}; /**< Number of clock ticks in a second */

  std::uint64_t nsSinceEpoch = {};   /**< Nanoseconds since UNIX epoch in UTC, no leap seconds */
  std::int32_t tzOffset = {};        /**< Time zone offset from UTC in seconds */
  std::string tzName;                /**< Time zone name */
};

/**
 * Represents a log event (one line in a logfile).
 *
 * The event arguments can be visited by:
 * mserialize::visit(e.source->argumentTags, visitor, e.arguments);
 *
 * `clockValue` marks the time when the event was created.
 * It can be interpreted together with a ClockSync.
 * `clockValue` is zero if the event is not timestamped.
 */
struct Event
{
  const EventSource* source = nullptr;
  std::uint64_t clockValue = {};
  Range arguments;
};

/**
 * Serialize `entry` to `out`, prefixed with size and tag.
 *
 * @see documentation top of this file
 *
 * @requires Entry must be serializable
 * @requires Entry::Tag must be uint64_t
 * @requires OutputStream to model the mserialize::OutputStream concept
 *
 * @returns total number of bytes written to `out`
 */
template <typename Entry, typename OutputStream>
std::size_t serializeSizePrefixedTagged(const Entry& entry, OutputStream& out)
{
  const std::uint64_t tag = Entry::Tag;
  const std::uint32_t size = std::uint32_t(mserialize::serialized_size(entry) + sizeof(tag));
  mserialize::serialize(size, out);
  mserialize::serialize(tag, out);
  mserialize::serialize(entry, out);

  return size + sizeof(size);
}

} // namespace binlog

MSERIALIZE_MAKE_STRUCT_SERIALIZABLE(  binlog::EventSource, id, severity, category, function, file, line, formatString, argumentTags)
MSERIALIZE_MAKE_STRUCT_DESERIALIZABLE(binlog::EventSource, id, severity, category, function, file, line, formatString, argumentTags)

MSERIALIZE_MAKE_STRUCT_SERIALIZABLE(  binlog::WriterProp, id, name, batchSize)
MSERIALIZE_MAKE_STRUCT_DESERIALIZABLE(binlog::WriterProp, id, name, batchSize)

MSERIALIZE_MAKE_STRUCT_SERIALIZABLE(  binlog::ClockSync, clockValue, clockFrequency, nsSinceEpoch, tzOffset, tzName)
MSERIALIZE_MAKE_STRUCT_DESERIALIZABLE(binlog::ClockSync, clockValue, clockFrequency, nsSinceEpoch, tzOffset, tzName)

#endif // BINLOG_ENTRIES_HPP
