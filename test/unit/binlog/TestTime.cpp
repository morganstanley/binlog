#include <binlog/Time.hpp>

#include <boost/test/unit_test.hpp>

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

BOOST_AUTO_TEST_SUITE(Time)

BOOST_AUTO_TEST_CASE(system_clock_measures_utc)
{
  // As of C++20, std::chrono::system_clock measures UTC time (without leap seconds).
  // In the code, this is assumed even before C++20.
  // This assumption is tested below.
  // If this test fails on a conforming platform,
  // add a configuration time macro, which tracks the difference.

  const std::chrono::system_clock::time_point epoch_tp{};
  const std::time_t epoch_tt = std::chrono::system_clock::to_time_t(epoch_tp);
  const std::tm* t = std::gmtime(&epoch_tt);

  BOOST_TEST_REQUIRE(t != nullptr);
  BOOST_TEST(t->tm_year == 70);
  BOOST_TEST(t->tm_mon == 0);
  BOOST_TEST(t->tm_mday == 1);
  BOOST_TEST(t->tm_hour == 0);
  BOOST_TEST(t->tm_min == 0);
  BOOST_TEST(t->tm_sec == 0);
}

BOOST_AUTO_TEST_CASE(ticks_to_ns)
{
  BOOST_TEST(binlog::ticksToNanoseconds(1, 0) == std::chrono::nanoseconds{0});
  BOOST_TEST(binlog::ticksToNanoseconds(1, 1) == std::chrono::seconds{1});
  BOOST_TEST(binlog::ticksToNanoseconds(1, 100) == std::chrono::seconds{100});

  BOOST_TEST(binlog::ticksToNanoseconds(100, 0) == std::chrono::nanoseconds{0});
  BOOST_TEST(binlog::ticksToNanoseconds(100, 1) == std::chrono::milliseconds{10});

  BOOST_TEST(binlog::ticksToNanoseconds(1'000'000'000, 0) == std::chrono::nanoseconds{0});
  BOOST_TEST(binlog::ticksToNanoseconds(1'000'000'000, 1) == std::chrono::nanoseconds{1});
  BOOST_TEST(binlog::ticksToNanoseconds(1'000'000'000, 234) == std::chrono::nanoseconds{234});

  // for sub-nano precision clocks, it truncates
  BOOST_TEST(binlog::ticksToNanoseconds(3'000'000'000, 0) == std::chrono::nanoseconds{0});
  BOOST_TEST(binlog::ticksToNanoseconds(3'000'000'000, 1) == std::chrono::nanoseconds{0});
  BOOST_TEST(binlog::ticksToNanoseconds(3'000'000'000, 2) == std::chrono::nanoseconds{0});
  BOOST_TEST(binlog::ticksToNanoseconds(3'000'000'000, 3) == std::chrono::nanoseconds{1});
  BOOST_TEST(binlog::ticksToNanoseconds(3'000'000'000, 31) == std::chrono::nanoseconds{10});
}

BOOST_AUTO_TEST_CASE(clock_to_ns)
{
  const binlog::ClockSync clockSync{
    123,                    // clock
    3,                      // ticks per sec
    1569902400'000000000,   // sync at 2019.10.01 04:00:00
    456,                    // random tz offset (tested code shouldn't care)
    ""                      // no tz name
  };

  // control the present
  BOOST_TEST(binlog::clockToNsSinceEpoch(clockSync, 123) == std::chrono::seconds{1569902400});

  // control the past
  BOOST_TEST(binlog::clockToNsSinceEpoch(clockSync, 120) == std::chrono::seconds{1569902400-1});
  BOOST_TEST(binlog::clockToNsSinceEpoch(clockSync, 0) == std::chrono::seconds{1569902400-41});

  // control the future
  BOOST_TEST(binlog::clockToNsSinceEpoch(clockSync, 124) == std::chrono::nanoseconds{1569902400333333333});
  BOOST_TEST(binlog::clockToNsSinceEpoch(clockSync, 126) == std::chrono::seconds{1569902400+1});
  BOOST_TEST(binlog::clockToNsSinceEpoch(clockSync, 3508909323) == std::chrono::seconds{2739538800});
}

BOOST_AUTO_TEST_CASE(ns_to_gmt)
{
  binlog::BrokenDownTime bdt{};

  binlog::nsSinceEpochToBrokenDownTimeUTC(std::chrono::nanoseconds{0}, bdt);
  BOOST_TEST(str(bdt) == "1970-01-01 00:00:00.000000000");

  binlog::nsSinceEpochToBrokenDownTimeUTC(std::chrono::nanoseconds{123}, bdt);
  BOOST_TEST(str(bdt) == "1970-01-01 00:00:00.000000123");

  binlog::nsSinceEpochToBrokenDownTimeUTC(std::chrono::nanoseconds{435601550'123456789}, bdt);
  BOOST_TEST(str(bdt) == "1983-10-21 16:25:50.123456789");

  binlog::nsSinceEpochToBrokenDownTimeUTC(std::chrono::nanoseconds{1542364201'987654321}, bdt);
  BOOST_TEST(str(bdt) == "2018-11-16 10:30:01.987654321");

  // Who controls the past controls the future.
  // Windows does not: "Midnight, January 1, 1970, is the lower bound of the date range"
  // https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/gmtime-s-gmtime32-s-gmtime64-s?view=vs-2019
  #ifndef _WIN32
    binlog::nsSinceEpochToBrokenDownTimeUTC(std::chrono::seconds{ -69781770 }, bdt);
    BOOST_TEST(str(bdt) == "1967-10-16 08:10:30.000000000");
  #endif
}

BOOST_AUTO_TEST_SUITE_END()
