#include "printers.hpp"

#include <binlog/Entries.hpp> // Event
#include <binlog/EventStream.hpp>
#include <binlog/PrettyPrinter.hpp>

#include <istream>
#include <ostream>

void printEvents(std::istream& input, std::ostream& output, const std::string& format, const std::string& dateFormat)
{
  binlog::EventStream eventStream(input);
  binlog::PrettyPrinter pp(format, dateFormat);

  while (const binlog::Event* event = eventStream.nextEvent())
  {
    pp.printEvent(output, *event, eventStream.writerProp(), eventStream.clockSync());
  }
}
