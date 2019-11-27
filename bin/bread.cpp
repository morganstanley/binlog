#include <binlog/Entries.hpp> // Event
#include <binlog/EventStream.hpp>
#include <binlog/PrettyPrinter.hpp>

#include <fstream>
#include <iostream>
#include <string>

namespace {

void printEvents(std::istream& input, std::ostream& output, const std::string& format)
{
  binlog::EventStream eventStream(input);
  binlog::PrettyPrinter pp(format);

  while (const binlog::Event* event = eventStream.nextEvent())
  {
    pp.printEvent(output, *event, eventStream.writerProp(), eventStream.clockSync());
  }
}

} // namespace

int main(int argc, const char* argv[])
{
  const std::string logfile = (argc > 1) ? argv[1] : "-";
  const std::string format = (argc > 2) ? argv[2] + std::string("\n") : "%S %C [%d] %n %m (%G:%L)\n";

  try
  {
    if (logfile == "-")
    {
      printEvents(std::cin, std::cout, format);
    }
    else
    {
      std::ifstream input(logfile, std::ios_base::in | std::ios_base::binary);
      if (! input)
      {
        std::cerr << "[bread] Failed to open: " << logfile << "\n";
        return 2;
      }

      printEvents(input, std::cout, format);
    }
  }
  catch (const std::exception& ex)
  {
    std::cerr << "[bread] Exception: " << ex.what() << "\n";
    return 3;
  }

  return 0;
}
