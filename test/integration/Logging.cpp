#include <binlog/binlog.hpp>

#include <iostream>

int main()
{
  BINLOG_TRACE("Hello");
  // Outputs: TRAC Hello
  BINLOG_DEBUG("Hello");
  // Outputs: DEBG Hello
  BINLOG_INFO("Hello");
  // Outputs: INFO Hello
  BINLOG_WARN("Hello");
  // Outputs: WARN Hello
  BINLOG_ERROR("Hello");
  // Outputs: ERRO Hello
  BINLOG_CRITICAL("Hello");
  // Outputs: CRIT Hello

  //[log
  BINLOG_INFO("Result: {}", 42);
  // Outputs: INFO Result: 42
  //]

  binlog::consume(std::cout);
  return 0;
}
