#include <binlog/Session.hpp>
#include <binlog/SessionWriter.hpp>
#include <binlog/advanced_log_macros.hpp>

#include <fstream>
#include <iostream>

int main()
{
  binlog::Session session;
  binlog::SessionWriter writer(session);

  BINLOG_INFO_W(writer, "Hello {}!", "World");

  std::ofstream logfile("hello.blog", std::ofstream::out|std::ofstream::binary);
  session.consume(logfile);

  if (! logfile)
  {
    std::cerr << "Failed to write hello.blog\n";
    return 1;
  }

  std::cout << "Binary log written to hello.blog\n";
  return 0;
}
