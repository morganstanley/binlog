#ifndef TEST_UNIT_BINLOG_TEST_UTILS_HPP
#define TEST_UNIT_BINLOG_TEST_UTILS_HPP

#include <binlog/Session.hpp>

#include <chrono>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

/** Pretty print the events of a binlog stream `input` according to `eventFormat` */
std::vector<std::string> streamToEvents(std::istream& input, const char* eventFormat);

/** Consume `session`, pretty print the consumed events according to `eventFormat` */
std::vector<std::string> getEvents(binlog::Session& session, const char* eventFormat);

/** Convert `tp` to local time string representation */
std::string timePointToString(std::chrono::system_clock::time_point tp);

/** Count the binlog entries in `input` with tag = `tagToCount` */
std::size_t countTags(std::istream& input, std::uint64_t tagToCount);

#endif // TEST_UNIT_BINLOG_TEST_UTILS_HPP
