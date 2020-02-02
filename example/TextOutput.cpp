#include <binlog/binlog.hpp>
#include <binlog/TextOutputStream.hpp> // requires binlog library to be linked

#include <iostream>

int main()
{
  BINLOG_INFO("Hello Text Output!");

  binlog::TextOutputStream output(std::cout);
  binlog::consume(output);

  return 0;
}
