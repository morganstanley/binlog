#include <binlog/EventStream.hpp>

#include <mserialize/deserialize.hpp>

namespace binlog {

const Event* EventStream::nextEvent(EntryStream& input)
{
  while (true)
  {
    Range range = input.nextEntryPayload();
    if (range.empty()) { return nullptr; }

    const std::uint64_t tag = range.read<std::uint64_t>();
    const bool special = (tag & (std::uint64_t(1) << 63)) != 0;

    if (special)
    {
      switch (tag)
      {
        case EventSource::Tag:
          readEventSource(range);
          break;
        case WriterProp::Tag:
          readWriterProp(range);
          break;
        case ClockSync::Tag:
          readClockSync(range);
          break;
        // default: ignore unkown special entries
        // to be forward compatible.
      }
    }
    else
    {
      readEvent(tag, range);
      return &_event;
    }
  }
}

void EventStream::readEventSource(Range range)
{
  EventSource eventSource;
  mserialize::deserialize(eventSource, range);
  _eventSources.emplace(eventSource.id, std::move(eventSource));
}

void EventStream::readWriterProp(Range range)
{
  // Make sure _writerProp is updated only if deserialize does not throw
  WriterProp wp;
  mserialize::deserialize(wp, range);
  _writerProp = std::move(wp);
}

void EventStream::readClockSync(Range range)
{
  // Make sure _clockSync is updated only if deserialize does not throw
  ClockSync clockSync;
  mserialize::deserialize(clockSync, range);
  _clockSync = std::move(clockSync);
}

void EventStream::readEvent(std::uint64_t eventSourceId, Range range)
{
  auto&& it = _eventSources.find(eventSourceId);
  if (it == _eventSources.end())
  {
    throw std::runtime_error("Event has invalid source id: " + std::to_string(eventSourceId));
  }

  _event.source = it;
  _event.clockValue = range.read<std::uint64_t>();
  _event.arguments = range;
}

} // namespace binlog
