#include <binlog/binlog.hpp>

#include <fstream>
#include <iostream>
#include <string>

void processInput(const std::string& input, binlog::SessionWriter& writer)
{
  BINLOG_INFO_W(writer, "Input received: {}", input);
  /* do processing ... */
  BINLOG_INFO_W(writer, "Input processed");
}

int main()
{
  std::ofstream logfile("consumeloop.blog", std::ofstream::out|std::ofstream::binary);

  binlog::Session session;
  const std::size_t queueCapacityBytes = 1 << 20;
  binlog::SessionWriter writer(session, queueCapacityBytes);

  std::string input;
  while (std::getline(std::cin, input))
  {
    processInput(input, writer); // logs using `writer`
    session.consume(logfile);
  }

  if (! logfile)
  {
    std::cerr << "Failed to write consumeloop.blog\n";
    return 1;
  }

  std::cout << "Binary log written to consumeloop.blog\n";
  return 0;
}
