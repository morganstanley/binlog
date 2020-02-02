#ifndef TEST_UNIT_BINLOG_TEST_UTILS_HPP
#define TEST_UNIT_BINLOG_TEST_UTILS_HPP

#include <binlog/EntryStream.hpp>
#include <binlog/Session.hpp>

#include <chrono>
#include <cstdint>
#include <ios>
#include <string>
#include <vector>

/** EntryStream that also models mserialize::OutputStream */
struct TestStream : binlog::EntryStream
{
  std::vector<char> buffer;
  std::size_t readPos = 0;

  TestStream& write(const char* data, std::streamsize size);

  binlog::Range nextEntryPayload() override;
};

/** Pretty print the events of a binlog stream `input` according to `eventFormat` */
std::vector<std::string> streamToEvents(TestStream& input, const char* eventFormat);

/** Consume `session`, pretty print the consumed events according to `eventFormat` */
std::vector<std::string> getEvents(binlog::Session& session, const char* eventFormat);

/** Convert `tp` to local time string representation */
std::string timePointToString(std::chrono::system_clock::time_point tp);

/** Count the binlog entries in `input` with tag = `tagToCount` */
std::size_t countTags(TestStream& input, std::uint64_t tagToCount);

#endif // TEST_UNIT_BINLOG_TEST_UTILS_HPP
