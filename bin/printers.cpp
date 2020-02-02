#include "printers.hpp"

#include <binlog/Entries.hpp> // Event
#include <binlog/EntryStream.hpp>
#include <binlog/EventStream.hpp>
#include <binlog/PrettyPrinter.hpp>

#include <algorithm>
#include <cstdint>
#include <istream>
#include <ostream>
#include <sstream>
#include <utility>
#include <vector>

void printEvents(std::istream& input, std::ostream& output, const std::string& format, const std::string& dateFormat)
{
  binlog::IstreamEntryStream entryStream(input);
  binlog::EventStream eventStream;
  binlog::PrettyPrinter pp(format, dateFormat);

  while (const binlog::Event* event = eventStream.nextEvent(entryStream))
  {
    pp.printEvent(output, *event, eventStream.writerProp(), eventStream.clockSync());
  }
}

void printSortedEvents(std::istream& input, std::ostream& output, const std::string& format, const std::string& dateFormat)
{
  binlog::IstreamEntryStream entryStream(input);
  binlog::EventStream eventStream;
  binlog::PrettyPrinter pp(format, dateFormat);

  using Pair = std::pair<std::uint64_t /* clock */, std::string /* pretty printed event */>;
  std::vector<Pair> buffer;
  std::ostringstream stream;

  // buffer every event in input
  while (const binlog::Event* event = eventStream.nextEvent(entryStream))
  {
    stream.str({}); // reset stream
    pp.printEvent(stream, *event, eventStream.writerProp(), eventStream.clockSync());
    buffer.emplace_back(event->clockValue, stream.str());
  }

  // sort and print the the buffer
  const auto cmpClock = [](const Pair& p1, const Pair& p2) { return p1.first < p2.first; };
  std::stable_sort(buffer.begin(), buffer.end(), cmpClock);
  for (const Pair& p : buffer)
  {
    output << p.second;
  }
}
