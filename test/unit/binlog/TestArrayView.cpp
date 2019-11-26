#include <binlog/ArrayView.hpp>

#include "test_utils.hpp"

#include <binlog/Session.hpp>
#include <binlog/SessionWriter.hpp>
#include <binlog/advanced_log_macros.hpp>

#include <boost/test/unit_test.hpp>

#include <string>

BOOST_AUTO_TEST_SUITE(ArrayView)

BOOST_AUTO_TEST_CASE(empty)
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 128);

  const int* array = nullptr;
  BINLOG_INFO_W(writer, "Empty array: {}", binlog::array_view(array, 0));

  BOOST_TEST(getEvents(session, "%m") == std::vector<std::string>{"Empty array: []"}, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(some_integers)
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 128);

  const int array[] = {1, 2, 3};
  BINLOG_INFO_W(writer, "Ints: {}", binlog::array_view(array, 3));

  BOOST_TEST(getEvents(session, "%m") == std::vector<std::string>{"Ints: [1, 2, 3]"}, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(some_strings)
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 128);

  const std::string array[] = {"foo", "bar", "baz", "qux"};
  BINLOG_INFO_W(writer, "Strings: {}", binlog::array_view(array, 4));

  BOOST_TEST(getEvents(session, "%m") == std::vector<std::string>{"Strings: [foo, bar, baz, qux]"}, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_SUITE_END()
