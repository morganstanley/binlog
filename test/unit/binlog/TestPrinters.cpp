#include <printers.hpp>

#include <binlog/Session.hpp>
#include <binlog/SessionWriter.hpp>
#include <binlog/advanced_log_macros.hpp>

#include <boost/test/unit_test.hpp>

#include <sstream>
#include <string>
#include <vector>

namespace {

std::vector<std::string> streamToLines(std::istream& input)
{
  std::vector<std::string> result;
  std::string line;
  while (std::getline(input, line))
  {
    result.push_back(std::move(line));
  }
  return result;
}

void logClock(binlog::SessionWriter& writer, std::uint64_t clock)
{
  BINLOG_CREATE_SOURCE_AND_EVENT(writer, binlog::Severity::info, main, clock, "{}", clock);
}

} // namespace

BOOST_AUTO_TEST_SUITE(Printers)

BOOST_AUTO_TEST_CASE(print_events)
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 512);

  BINLOG_INFO_W(writer, "Hello {}", std::string("World"));
  BINLOG_WARN_W(writer, "foobar {}", 123);

  std::stringstream binstream;
  session.consume(binstream);

  std::stringstream txtstream;
  printEvents(binstream, txtstream, "%S %m\n", "");

  const std::vector<std::string> expected{
    "INFO Hello World",
    "WARN foobar 123",
  };
  BOOST_TEST(streamToLines(txtstream) == expected, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(print_sorted_events)
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 512);

  logClock(writer, 100);
  logClock(writer, 200);
  logClock(writer, 300);
  logClock(writer, 250);
  logClock(writer, 150);
  logClock(writer, 260);

  logClock(writer, 30'000'000'000 + 100);
  logClock(writer, 30'000'000'000 + 300);
  logClock(writer, 30'000'000'000 + 200);
  logClock(writer, 30'000'000'000 + 150);
  logClock(writer, 30'000'000'000 + 250);
  logClock(writer, 30'000'000'000 + 260);

  logClock(writer, 50'000'000'000 + 300);
  logClock(writer, 40'000'000'000 + 100);
  logClock(writer, 80'000'000'000 + 250);
  logClock(writer, 70'000'000'000 + 150);
  logClock(writer, 60'000'000'000 + 200);
  logClock(writer, 90'000'000'000 + 260);

  std::stringstream binstream;
  session.consume(binstream);

  std::stringstream txtstream;
  printSortedEvents(binstream, txtstream, "%m\n", "");

  const std::vector<std::string> expected{
    "100", "150", "200", "250", "260", "300",
    "30000000100", "30000000150", "30000000200",
    "30000000250", "30000000260", "30000000300",
    "40000000100", "50000000300", "60000000200",
    "70000000150", "80000000250", "90000000260",
  };
  BOOST_TEST(streamToLines(txtstream) == expected, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_SUITE_END()
