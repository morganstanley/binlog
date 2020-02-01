#include "test_utils.hpp"

#include <binlog/Entries.hpp> // Event
#include <binlog/EventStream.hpp>
#include <binlog/PrettyPrinter.hpp>

#include <boost/test/unit_test.hpp>

#include <ctime>
#include <istream>
#include <sstream>

std::vector<std::string> streamToEvents(std::istream& input, const char* eventFormat)
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

  input.clear();
  input.seekg(0);

  return result;
}

std::vector<std::string> getEvents(binlog::Session& session, const char* eventFormat)
{
  std::stringstream stream;
  const binlog::Session::ConsumeResult cr = session.consume(stream);
  BOOST_TEST(stream.tellp() == cr.bytesConsumed);
  return streamToEvents(stream, eventFormat);
}

std::string timePointToString(std::chrono::system_clock::time_point tp)
{
  char buffer[128] = {0};
  const std::time_t tt = std::chrono::system_clock::to_time_t(tp);
  const std::tm* tm = std::localtime(&tt);
  strftime(buffer, 128, "%Y.%m.%d %H:%M:%S", tm);
  return buffer;
}

std::size_t countTags(std::istream& input, std::uint64_t tagToCount)
{
  std::uint32_t size = 0;
  std::uint64_t tag = 0;

  std::size_t result = 0;

  while (
    input.read(reinterpret_cast<char*>(&size), sizeof(size))
         .read(reinterpret_cast<char*>(&tag), sizeof(tag))
    && size >= sizeof(tag)
  )
  {
    if (tag == tagToCount) { ++result; }
    input.ignore(std::streamsize(size - sizeof(tag)));
  }

  input.clear();
  input.seekg(0);

  return result;
}
