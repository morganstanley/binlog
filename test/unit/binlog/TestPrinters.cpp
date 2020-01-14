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

BOOST_AUTO_TEST_SUITE_END()
