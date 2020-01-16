#include "printers.hpp"

#include <binlog/Entries.hpp> // Event
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
  binlog::EventStream eventStream(input);
  binlog::PrettyPrinter pp(format, dateFormat);

  while (const binlog::Event* event = eventStream.nextEvent())
  {
    pp.printEvent(output, *event, eventStream.writerProp(), eventStream.clockSync());
  }
}

void printSortedEvents(std::istream& input, std::ostream& output, const std::string& format, const std::string& dateFormat)
{
  const std::uint64_t maxClockDiff = 30'000'000'000; // 30s assuming 1/ns clock frequency

  binlog::EventStream eventStream(input);
  binlog::PrettyPrinter pp(format, dateFormat);

  using Pair = std::pair<std::uint64_t /* clock */, std::string /* pretty printed event */>;
  std::vector<Pair> buffer;
  std::ostringstream stream;

  std::uint64_t minClock = std::uint64_t(-1);

  const auto cmpClock = [](const Pair& p1, const Pair& p2) { return p1.first < p2.first; };

  while (const binlog::Event* event = eventStream.nextEvent())
  {
    // maintain minClock
    const std::uint64_t now = event->clockValue;
    minClock = (std::min)(minClock, now);

    // add new event to the buffer
    stream.str({}); // reset stream
    pp.printEvent(stream, *event, eventStream.writerProp(), eventStream.clockSync());
    buffer.emplace_back(now, stream.str());

    // Assume that there's no pair of events that
    // a.clockValue + maxClockDiff < b.clockValue
    // AND `a` comes after `b` in the log.
    // Given that, we can sort and print some events.
    // To avoid sorting too few events at a time,
    // we wait until maxClockDiff*2 elapses,
    // and then sort a maxClockDiff window.
    if (minClock + maxClockDiff * 2 < now)
    {
      // separate events we'd like to print, only sort them
      const auto isOld = [&](const Pair& p) { return p.first + maxClockDiff < now; };
      const auto end = std::stable_partition(buffer.begin(), buffer.end(), isOld);

      std::stable_sort(buffer.begin(), end, cmpClock);

      // print sorted events
      for (auto it = buffer.begin(); it != end; ++it)
      {
        output << it->second;
      }

      // remove printed events
      buffer.erase(buffer.begin(), end);

      // re-compute minClock
      minClock = std::min_element(buffer.begin(), buffer.end(), cmpClock)->first;
    }
  }

  // sort and print the remaining events
  std::stable_sort(buffer.begin(), buffer.end(), cmpClock);
  for (const Pair& p : buffer)
  {
    output << p.second;
  }
}
