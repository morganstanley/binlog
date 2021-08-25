//[duration
#include <binlog/adapt_stdduration.hpp> // must be included to log std::chrono::duration helper types
//]

#include <binlog/binlog.hpp>

#include <chrono>
#include <iostream>

int main()
{

  //[duration

  BINLOG_INFO("{} {} {}", std::chrono::nanoseconds{1}, std::chrono::microseconds{2}, std::chrono::milliseconds{3});
  // Outputs: 1ns 2us 3ms
  BINLOG_INFO("{} {} {}", std::chrono::seconds{4}, std::chrono::minutes{5}, std::chrono::hours{6});
  // Outputs: 4s 5m 6h
  //]


  binlog::consume(std::cout);
  return 0;
}
