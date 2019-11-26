#include <binlog/const_char_ptr_is_string.hpp>

#include "test_utils.hpp"

#include <binlog/Session.hpp>
#include <binlog/SessionWriter.hpp>
#include <binlog/advanced_log_macros.hpp>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(ConstCharPtrIsString)

BOOST_AUTO_TEST_CASE(const_char_ptr_is_string)
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 128);

  const char* str = "C string";
  BINLOG_INFO_W(writer, "{}", str);

  BOOST_TEST(getEvents(session, "%m") == std::vector<std::string>{"C string"}, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(char_array_is_string)
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 128);

  char array[] = {'a', 'b', 'c'};
  BINLOG_INFO_W(writer, "{}", array);

  BOOST_TEST(getEvents(session, "%m") == std::vector<std::string>{"abc"}, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(nullptr_is_allowed)
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 128);

  const char* str = nullptr;
  BINLOG_INFO_W(writer, "{}", str);

  BOOST_TEST(getEvents(session, "%m") == std::vector<std::string>{"{null}"}, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(char_ptr_is_not_string)
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 128);

  // char* str = "C string"; // A decent compiler should warn about this
  char c = 'c';
  char* ptr = &c;
  BINLOG_INFO_W(writer, "{}", ptr);

  BOOST_TEST(getEvents(session, "%m") == std::vector<std::string>{"c"}, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_SUITE_END()
