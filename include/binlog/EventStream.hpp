#ifndef BINLOG_EVENT_STREAM_HPP
#define BINLOG_EVENT_STREAM_HPP

#include <binlog/Entries.hpp>
#include <binlog/Range.hpp>

#include <istream>
#include <map>

namespace binlog {

/** Convert a binlog stream to events. */
class EventStream
{
public:
  explicit EventStream(std::istream& input);

  /**
   * Get the next event from the stream.
   *
   * The returned pointer (and the objects reachable from it)
   * is valid until the next call to `nextEvent` and
   * as long as `*this` is valid.
   *
   * If the next entry cannot be consumed from
   * the input stream because it is incomplete,
   * before signalling error via exception,
   * the read position will be reset, allowing
   * the writer of the stream, if any, to complete
   * the entry.
   *
   * If the next entry can be consumed, but contains
   * errors, the entry is irrecoverable, therefore
   * it is skipped, before an exception is thrown,
   * to allow consuming subsequent entries.
   *
   * @returns pointer to the next event
   *          or nullptr on EOF.
   * @throws std::runtime_error on error.
   */
  const Event* nextEvent();

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
  Range nextSizePrefixedRange();

  void readEventSource(Range range);

  void readWriterProp(Range range);

  void readClockSync(Range range);

  void readEvent(std::uint64_t eventSourceId, Range range);

  std::istream& _input;
  std::vector<char> _buffer; // TODO(benedek) perf: use input buffer directly
  std::map<std::uint64_t, EventSource> _eventSources; // TODO(benedek) perf: use SegmentedMap
  WriterProp _writerProp;
  ClockSync _clockSync;
  Event _event;
};

} // namespace binlog

#endif // BINLOG_EVENT_STREAM_HPP
