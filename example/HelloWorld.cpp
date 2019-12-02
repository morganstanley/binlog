//[hello
#include <binlog/binlog.hpp>
//]

#include <fstream>
#include <iostream>

//[hello

int main()
{
  BINLOG_INFO("Hello {}!", "World");

  std::ofstream logfile("hello.blog", std::ofstream::out|std::ofstream::binary);
  binlog::consume(logfile);
//]

  if (! logfile)
  {
    std::cerr << "Failed to write hello.blog\n";
    return 1;
  }

  std::cout << "Binary log written to hello.blog\n";
  return 0;

//[hello
}
//]
