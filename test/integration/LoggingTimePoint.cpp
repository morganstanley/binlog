//[timepoint
#include <binlog/adapt_stdtimepoint.hpp> // must be included to log std::chrono::system_clock::time_point
//]

#include <binlog/binlog.hpp>

#include <chrono>
#include <cstdlib>
#include <iostream>

int main()
{
  // make testing localtime easier
  #ifdef _WIN32
    _putenv_s("TZ", "UTC");
  #else
    setenv("TZ", "UTC", 1);
  #endif

  //[timepoint

  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
  //]

  // use microseconds instead of nanoseconds because of macOS
  now = std::chrono::system_clock::time_point(std::chrono::microseconds{1615917902123456});

  //[timepoint
  BINLOG_INFO("Now: {}", now);
  // Outputs: Now: 2021-03-16 18:05:02.123456000
  //]

  BINLOG_INFO("Now (seconds): {}", std::chrono::time_point_cast<std::chrono::seconds>(now));
  // Outputs: Now (seconds): 2021-03-16 18:05:02.000000000

  const std::chrono::system_clock::time_point zero;
  BINLOG_INFO("Zero: {}", zero);
  // Outputs: Zero: 1970-01-01 00:00:00.000000000

  binlog::consume(std::cout);
  return 0;
}
