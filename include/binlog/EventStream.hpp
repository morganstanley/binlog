#ifndef BINLOG_EVENT_STREAM_HPP
#define BINLOG_EVENT_STREAM_HPP

#include <binlog/Entries.hpp>
#include <binlog/EntryStream.hpp>
#include <binlog/Range.hpp>

#include <istream>
#include <map>

namespace binlog {

/** Convert a binlog stream to events. */
class EventStream
{
public:
  /**
   * Get the next event from `input`.
   *
   * The returned pointer (and the objects reachable from it)
   * is valid until the next call to `nextEvent` and
   * as long as `*this` and the entry given by `input` is valid.
   *
   * If an entry given by `input` is invalid,
   * it is droppend, *this remains unchanged
   * and an exception is thrown.
   *
   * @param input contains binlog entries
   * @returns pointer to the next event
   *          or nullptr on `input` returns an empty entry
   * @throws std::runtime_error on error.
   */
  const Event* nextEvent(EntryStream& input);

  /**
   * @return the most recent writer properties consumed from
   *         the stream or a default constructed
   *         object if no such entry was found.
   */
  const WriterProp& writerProp() const { return _writerProp; }

  /**
   * @return the most recent clock sync consumed
   *         from the stream, or a default constructed
   *         object if no such clock sync was found.
   */
  const ClockSync& clockSync() const { return _clockSync; }

private:
  void readEventSource(Range range);

  void readWriterProp(Range range);

  void readClockSync(Range range);

  void readEvent(std::uint64_t eventSourceId, Range range);

  std::map<std::uint64_t, EventSource> _eventSources; // TODO(benedek) perf: use SegmentedMap
  WriterProp _writerProp;
  ClockSync _clockSync;
  Event _event;
};

} // namespace binlog

#endif // BINLOG_EVENT_STREAM_HPP
