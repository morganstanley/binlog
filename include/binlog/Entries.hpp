#ifndef BINLOG_ENTRIES_HPP
#define BINLOG_ENTRIES_HPP

#include <binlog/Range.hpp>
#include <binlog/Severity.hpp>

#include <mserialize/make_struct_deserializable.hpp>
#include <mserialize/make_struct_serializable.hpp>

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
 * Represents an actor (thread, fiber, coroutine, task)
 * which triggers EventSources to produce events.
 *
 * `id` and `name` are free-form, can be anything
 * which helps the user to identify the actor.
 *
 * `batchSize` is a hint, equal to the size
 * of the events following this entry.
 * If specified, must be a valid offset to a later entry,
 * useful for fast forward seeking in the stream.
 * Can be zero if unknown.
 */
struct Actor
{
  static constexpr std::uint64_t Tag = std::uint64_t(-2);

  std::uint64_t id = {};
  std::string name;
  std::uint64_t batchSize = {};
};

/**
 * Represents a log event (one line in a logfile).
 *
 * The event arguments can be visited by:
 * mserialize::visit(e.source->argumentTags, visitor, e.arguments);
 */
struct Event
{
  EventSource* source = nullptr;
  Range arguments;
};

} // namespace binlog

MSERIALIZE_MAKE_STRUCT_SERIALIZABLE(  binlog::EventSource, id, severity, category, function, file, line, formatString, argumentTags)
MSERIALIZE_MAKE_STRUCT_DESERIALIZABLE(binlog::EventSource, id, severity, category, function, file, line, formatString, argumentTags)

MSERIALIZE_MAKE_STRUCT_SERIALIZABLE(  binlog::Actor, id, name, batchSize)
MSERIALIZE_MAKE_STRUCT_DESERIALIZABLE(binlog::Actor, id, name, batchSize)

#endif // BINLOG_ENTRIES_HPP
