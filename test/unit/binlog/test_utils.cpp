#include "test_utils.hpp"

#include <binlog/Entries.hpp> // Event
#include <binlog/EventStream.hpp>
#include <binlog/PrettyPrinter.hpp>

#include <doctest/doctest.h>

#include <ctime>
#include <sstream>

TestStream& TestStream::write(const char* data, std::streamsize size)
{
  buffer.insert(buffer.end(), data, data + size);
  return *this;
}

binlog::Range TestStream::nextEntryPayload()
{
  binlog::Range input(buffer.data() + readPos, buffer.data() + buffer.size());
  if (input.empty()) { return {}; }

  const std::uint32_t size = input.read<std::uint32_t>();
  binlog::Range result(input.view(size), size);
  readPos += sizeof(size) + size;
  return result;
}

std::vector<std::string> streamToEvents(TestStream& input, const char* eventFormat)
{
  std::vector<std::string> result;

  binlog::EventStream eventStream;
  binlog::PrettyPrinter pp(eventFormat, "%Y.%m.%d %H:%M:%S");

  while (const binlog::Event* event = eventStream.nextEvent(input))
  {
    std::ostringstream str;
    pp.printEvent(str, *event, eventStream.writerProp(), eventStream.clockSync());
    result.push_back(str.str());
  }

  return result;
}

std::vector<std::string> getEvents(binlog::Session& session, const char* eventFormat)
{
  TestStream stream;
  const binlog::Session::ConsumeResult cr = session.consume(stream);
  CHECK(stream.buffer.size() == cr.bytesConsumed);
  return streamToEvents(stream, eventFormat);
}

std::string timePointToString(std::chrono::system_clock::time_point tp)
{
  char buffer[128] = {0};
  const std::time_t tt = std::chrono::system_clock::to_time_t(tp);
  const std::tm* tm = std::localtime(&tt); // NOLINT(concurrency-mt-unsafe)
  (void)strftime(buffer, 128, "%Y.%m.%d %H:%M:%S", tm);
  return buffer;
}

std::size_t countTags(TestStream& input, std::uint64_t tagToCount)
{
  std::size_t result = 0;
  const std::size_t oldReadPos = input.readPos;

  while (binlog::Range payload = input.nextEntryPayload())
  {
    const std::uint64_t tag = payload.read<std::uint64_t>();
    if (tag == tagToCount) { ++result; }
  }

  input.readPos = oldReadPos;

  return result;
}
