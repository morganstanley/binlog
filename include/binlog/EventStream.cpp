#include <binlog/EventStream.hpp>

#include <mserialize/deserialize.hpp>

namespace {

std::size_t readIstream(std::istream& input, void* dst, std::size_t size)
{
  input.read(static_cast<char*>(dst), std::streamsize(size));
  return std::size_t(input.gcount());
}

void rewindIstream(std::istream& input, std::size_t n)
{
  input.clear();
  input.seekg(-1 * std::streamsize(n), std::ios_base::cur);
}

} // namespace

namespace binlog {

EventStream::EventStream(std::istream& input)
  :_input(input)
{}

const Event* EventStream::nextEvent()
{
  while (true)
  {
    Range range = nextSizePrefixedRange();
    if (range.empty()) { return nullptr; }

    const std::uint64_t tag = range.read<std::uint64_t>();
    switch (tag)
    {
      case EventSource::Tag:
        readEventSource(range);
        break;
      case Actor::Tag:
        readActor(range);
        break;
      default:
        readEvent(tag, range);
        return &_event;
    }
  }
}

Range EventStream::nextSizePrefixedRange()
{
  std::uint32_t size = 0;
  const std::size_t readSize1 = readIstream(_input, &size, sizeof(size));
  if (readSize1 == 0)
  {
    return {}; // eof
  }

  if (readSize1 != sizeof(size))
  {
    rewindIstream(_input, readSize1);
    throw std::runtime_error("Failed to read range size from file, only got "
      + std::to_string(readSize1) + " bytes, expected " + std::to_string(sizeof(size)));
  }

  // TODO(benedek) protect agains bad alloc by limiting size?

  _buffer.resize(size);
  const std::size_t readSize2 = readIstream(_input, _buffer.data(), size);

  if (readSize2 != size)
  {
    rewindIstream(_input, readSize1 + readSize2);
    throw std::runtime_error("Failed to read range from file, only got "
      + std::to_string(readSize2) + " bytes, expected " + std::to_string(size));
  }

  return {_buffer.data(), _buffer.data() + size};
}

void EventStream::readEventSource(Range range)
{
  EventSource eventSource;
  mserialize::deserialize(eventSource, range);
  _eventSources[eventSource.id] = std::move(eventSource);
}

void EventStream::readActor(Range range)
{
  // Make sure _actor is updated only if deserialize does not throw
  Actor actor;
  mserialize::deserialize(actor, range);
  _actor = std::move(actor);
}

void EventStream::readEvent(std::uint64_t eventSourceId, Range range)
{
  auto it = _eventSources.find(eventSourceId);
  if (it == _eventSources.end())
  {
    throw std::runtime_error("Event has invalid source id: " + std::to_string(eventSourceId));
  }

  _event.source = &it->second;
  _event.arguments = range;
}

} // namespace binlog
