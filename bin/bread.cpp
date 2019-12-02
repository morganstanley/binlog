#include <binlog/Entries.hpp> // Event
#include <binlog/EventStream.hpp>
#include <binlog/PrettyPrinter.hpp>

#include <fstream>
#include <iostream>
#include <string>

#include <getopt.h>

namespace {

std::istream& openFile(const std::string& path, std::ifstream& file)
{
  if (path == "-")
  {
    return std::cin;
  }

  file.open(path, std::ios_base::in | std::ios_base::binary);
  return file;
}

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

int main(int argc, /*const*/ char* argv[])
{
  std::string inputPath = "-";
  std::string format = "%S %C [%d] %n %m (%G:%L)\n";

  int opt;
  while ((opt = getopt(argc, argv, "f:")) != -1)
  {
    switch (opt)
    {
    case 'f':
      format = optarg;
      format += "\n";
      break;
    default:
      std::cerr << "[bread] Unknown command line argument\n";
      // TODO(benedek) show help
      return 1;
    }
  }

  if (optind < argc)
  {
    inputPath = argv[optind];
  }

  std::ifstream inputFile;
  std::istream& input = openFile(inputPath, inputFile);
  if (! input)
  {
    std::cerr << "[bread] Failed to open '" << inputPath << "' for reading\n";
    return 2;
  }

  try
  {
    printEvents(input, std::cout, format);
  }
  catch (const std::exception& ex)
  {
    std::cerr << "[bread] Exception: " << ex.what() << "\n";
    return 3;
  }

  return 0;
}
