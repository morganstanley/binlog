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

void printEvents(std::istream& input, std::ostream& output, const std::string& format, const std::string& dateFormat)
{
  binlog::EventStream eventStream(input);
  binlog::PrettyPrinter pp(format, dateFormat);

  while (const binlog::Event* event = eventStream.nextEvent())
  {
    pp.printEvent(output, *event, eventStream.writerProp(), eventStream.clockSync());
  }
}

void showHelp()
{
  std::cout <<
    "bread -- convert binary logfiles to human readable text\n"
    "\n"
    "Synopsis:\n"
    "  bread filename [-f format] [-d date-format]\n"
    "\n"
    "Examples:\n"
    "  bread logfile.blog"                                 "\n"
    "  bread logfile.blog -f '%S %m (%G:%L)'"              "\n"
    "  zcat logfile.blog.gz | bread - -f '%S %m (%G:%L)'"  "\n"
    "\n"
    "Arguments:\n"
    "  filename       Path to a logfile. If '-' or unspecified, read from stdin\n"
    "  format         Arbitrary string with optional placeholders, see 'Event Format'\n"
    "  date-format    Arbitrary string with optional placeholders, see 'Date Format'\n"
    "\n"
    "Allowed options:\n"
    "  -h             Show this help\n"
    "  -f             Set a custom format string to write events, see 'Event Format'\n"
    "  -d             Set a custom format string to write timestamps, see 'Date Format'\n"
    "\n"
    "Event Format\n"
    "  Log events are transformed to text by substituting placeholders"
    " of the format string by event fields. Available placeholders:\n"
    "\n"
    "  %I \t Source id\n"
    "  %S \t Severity\n"
    "  %C \t Category\n"
    "  %M \t Function\n"
    "  %F \t File, full path\n"
    "  %G \t File, file name only\n"
    "  %L \t Line\n"
    "  %P \t Format string\n"
    "  %T \t Argument tags\n"
    "  %n \t Writer (thread) name\n"
    "  %t \t Writer (thread) id\n"
    "  %d \t Timestamp, in producer timezone\n"
    "  %u \t Timestamp, in UTC\n"
    "  %r \t Timestamp, raw clock value\n"
    "  %m \t Message (format string with arguments substituted)\n"
    "  %% \t Literal %\n"
    "\n"
    "  Default format string: \"%S %C [%d] %n %m (%G:%L)\"\n"
    "\n"
    "Date Format\n"
    "  Timestamps are transformed to text by substituting placeholders"
    " of the date format string by date components. Available placeholders:\n"
    "\n"
    "  %Y \t Year, four digits\n"
    "  %y \t Year, two digits\n"
    "  %m \t Month (01-12)\n"
    "  %d \t Day (01-31)\n"
    "  %H \t Hour (00-23)\n"
    "  %M \t Minute (00-59)\n"
    "  %S \t Second (00-59)\n"
    "  %N \t Nanosecond (0-999999999)\n"
    "  %z \t Offset from UTC in ISO 8601 format (e.g: +0430)\n"
    "  %Z \t Time zone name abbreviation\n"
    "\n"
    "  Default date format string: \"%m/%d %H:%M:%S.%N\"\n"
    "\n"
    "Report bugs to:\n"
    "  https://github.com/Morgan-Stanley/binlog/issues\n";
}

} // namespace

int main(int argc, /*const*/ char* argv[])
{
  std::string inputPath = "-";
  std::string format = "%S %C [%d] %n %m (%G:%L)\n";
  std::string dateFormat = "%m/%d %H:%M:%S.%N";

  int opt;
  while ((opt = getopt(argc, argv, "f:d:h")) != -1)
  {
    switch (opt)
    {
    case 'f':
      format = optarg;
      format += "\n";
      break;
    case 'd':
      dateFormat = optarg;
      break;
    case 'h':
      showHelp();
      return 0;
    default:
      // getopt prints a useful error message by default (opterr is set)
      showHelp();
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

  std::ostream::sync_with_stdio(false);

  try
  {
    printEvents(input, std::cout, format, dateFormat);
  }
  catch (const std::exception& ex)
  {
    std::cerr << "[bread] Exception: " << ex.what() << "\n";
    return 3;
  }

  return 0;
}
