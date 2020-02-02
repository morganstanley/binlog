#include <binlog/TextOutputStream.hpp>

#include <binlog/Session.hpp>
#include <binlog/SessionWriter.hpp>
#include <binlog/advanced_log_macros.hpp>

#include <boost/test/unit_test.hpp>

#include <array>
#include <sstream>

BOOST_AUTO_TEST_SUITE(TextOutputStream)

BOOST_AUTO_TEST_CASE(empty)
{
  std::ostringstream out;
  binlog::TextOutputStream text(out);

  text.write(nullptr, 0);

  BOOST_TEST(out.str() == "");
}

BOOST_AUTO_TEST_CASE(events)
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 512);

  std::ostringstream out;
  binlog::TextOutputStream text(out, "%S %m\n");
  session.consume(text);

  BINLOG_INFO_W(writer, "Hello {}!", std::string{"Text"});
  session.consume(text);

  BINLOG_INFO_W(writer, "Numbers: {}", std::array<int, 6>{1, 1, 2, 3, 5, 7});
  BINLOG_INFO_W(writer, "More numbers: {}", std::array<int, 5>{12, 19, 31, 50, 81});
  session.consume(text);

  BOOST_TEST(out.str() ==
    "INFO Hello Text!\n" "INFO Numbers: [1, 1, 2, 3, 5, 7]\n" "INFO More numbers: [12, 19, 31, 50, 81]\n"
  );
}

BOOST_AUTO_TEST_CASE(corruption_allows_progress)
{
  std::ostringstream out;
  binlog::TextOutputStream text(out, "%S %m\n");

  binlog::Session session;
  binlog::SessionWriter writer(session, 512);
  session.consume(text);

  // corruption is signalled
  BOOST_CHECK_THROW(text.write("foobar", 6), std::runtime_error);

  // but allows progress
  BINLOG_INFO_W(writer, "Hello {}!", std::string{"Text"});
  session.consume(text);

  BOOST_TEST(out.str() == "INFO Hello Text!\n");
}

BOOST_AUTO_TEST_SUITE_END()
