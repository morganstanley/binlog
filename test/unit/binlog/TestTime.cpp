#include <binlog/Time.hpp>

#include <doctest/doctest.h>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

namespace {

std::string str(const binlog::BrokenDownTime& a)
{
  std::ostringstream s;
  s << std::setfill('0')
    << a.tm_year + 1900 << '-'
    << std::setw(2) << a.tm_mon+1 << '-'
    << std::setw(2) << a.tm_mday  << ' '
    << std::setw(2) << a.tm_hour  << ':'
    << std::setw(2) << a.tm_min   << ':'
    << std::setw(2) << a.tm_sec   << '.'
    << std::setw(9) << a.tm_nsec;
  return s.str();
}

} // namespace

namespace std { // NOLINT

ostream& operator<<(ostream& out, chrono::nanoseconds ns)
{
  return out << ns.count() << "ns";
}

} // namespace std

TEST_CASE("system_clock_measures_utc")
{
  // As of C++20, std::chrono::system_clock measures UTC time (without leap seconds).
  // In the code, this is assumed even before C++20.
  // This assumption is tested below.
  // If this test fails on a conforming platform,
  // add a configuration time macro, which tracks the difference.

  const std::chrono::system_clock::time_point epoch_tp{};
  const std::time_t epoch_tt = std::chrono::system_clock::to_time_t(epoch_tp);
  const std::tm* t = std::gmtime(&epoch_tt); // NOLINT(concurrency-mt-unsafe)

  REQUIRE(t != nullptr);
  CHECK(t->tm_year == 70);
  CHECK(t->tm_mon == 0);
  CHECK(t->tm_mday == 1);
  CHECK(t->tm_hour == 0);
  CHECK(t->tm_min == 0);
  CHECK(t->tm_sec == 0);
}

TEST_CASE("ticks_to_ns")
{
  CHECK(binlog::ticksToNanoseconds(1, 0) == std::chrono::nanoseconds{0});
  CHECK(binlog::ticksToNanoseconds(1, 1) == std::chrono::seconds{1});
  CHECK(binlog::ticksToNanoseconds(1, 100) == std::chrono::seconds{100});

  CHECK(binlog::ticksToNanoseconds(100, 0) == std::chrono::nanoseconds{0});
  CHECK(binlog::ticksToNanoseconds(100, 1) == std::chrono::milliseconds{10});

  CHECK(binlog::ticksToNanoseconds(1'000'000'000, 0) == std::chrono::nanoseconds{0});
  CHECK(binlog::ticksToNanoseconds(1'000'000'000, 1) == std::chrono::nanoseconds{1});
  CHECK(binlog::ticksToNanoseconds(1'000'000'000, 234) == std::chrono::nanoseconds{234});

  // for sub-nano precision clocks, it truncates
  CHECK(binlog::ticksToNanoseconds(3'000'000'000, 0) == std::chrono::nanoseconds{0});
  CHECK(binlog::ticksToNanoseconds(3'000'000'000, 1) == std::chrono::nanoseconds{0});
  CHECK(binlog::ticksToNanoseconds(3'000'000'000, 2) == std::chrono::nanoseconds{0});
  CHECK(binlog::ticksToNanoseconds(3'000'000'000, 3) == std::chrono::nanoseconds{1});
  CHECK(binlog::ticksToNanoseconds(3'000'000'000, 31) == std::chrono::nanoseconds{10});

  // make sure it does not overflow
  CHECK(binlog::ticksToNanoseconds(1'000'000'000, 31'534'085'395) == std::chrono::nanoseconds{31534085395});
  CHECK(binlog::ticksToNanoseconds(3'000'000'000, 30'000'000'000) == std::chrono::seconds{10});

  // int64_t(double(x)) != x
  CHECK(binlog::ticksToNanoseconds(1'000'000'000, 9007199254740993) == std::chrono::nanoseconds{9007199254740993});
  CHECK(binlog::ticksToNanoseconds(1'000'000'000, 1575293913602967233) == std::chrono::nanoseconds{1575293913602967233});
}

TEST_CASE("clock_to_ns")
{
  const binlog::ClockSync clockSync{
    123,                    // clock
    3,                      // ticks per sec
    1569902400'000000000,   // sync at 2019.10.01 04:00:00
    456,                    // random tz offset (tested code shouldn't care)
    ""                      // no tz name
  };

  // control the present
  CHECK(binlog::clockToNsSinceEpoch(clockSync, 123) == std::chrono::seconds{1569902400});

  // control the past
  CHECK(binlog::clockToNsSinceEpoch(clockSync, 120) == std::chrono::seconds{1569902400-1});
  CHECK(binlog::clockToNsSinceEpoch(clockSync, 0) == std::chrono::seconds{1569902400-41});

  // control the future
  CHECK(binlog::clockToNsSinceEpoch(clockSync, 124) == std::chrono::nanoseconds{1569902400333333333});
  CHECK(binlog::clockToNsSinceEpoch(clockSync, 126) == std::chrono::seconds{1569902400+1});
  CHECK(binlog::clockToNsSinceEpoch(clockSync, 3508909323) == std::chrono::seconds{2739538800});
}

TEST_CASE("ns_to_gmt")
{
  binlog::BrokenDownTime bdt{};

  binlog::nsSinceEpochToBrokenDownTimeUTC(std::chrono::nanoseconds{0}, bdt);
  CHECK(str(bdt) == "1970-01-01 00:00:00.000000000");

  binlog::nsSinceEpochToBrokenDownTimeUTC(std::chrono::nanoseconds{123}, bdt);
  CHECK(str(bdt) == "1970-01-01 00:00:00.000000123");

  binlog::nsSinceEpochToBrokenDownTimeUTC(std::chrono::nanoseconds{435601550'123456789}, bdt);
  CHECK(str(bdt) == "1983-10-21 16:25:50.123456789");

  binlog::nsSinceEpochToBrokenDownTimeUTC(std::chrono::nanoseconds{1542364201'987654321}, bdt);
  CHECK(str(bdt) == "2018-11-16 10:30:01.987654321");

  // Who controls the past controls the future.
  // Windows does not: "Midnight, January 1, 1970, is the lower bound of the date range"
  // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/gmtime-s-gmtime32-s-gmtime64-s?view=vs-2019
  #ifndef _WIN32
    binlog::nsSinceEpochToBrokenDownTimeUTC(std::chrono::seconds{ -69781770 }, bdt);
    CHECK(str(bdt) == "1967-10-16 08:10:30.000000000");
  #endif
}
