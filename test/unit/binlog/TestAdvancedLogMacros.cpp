#include <binlog/advanced_log_macros.hpp>

#include "test_utils.hpp"

#include <binlog/Session.hpp>
#include <binlog/SessionWriter.hpp>

#include <doctest/doctest.h>

#include <map>
#include <memory> // unique_ptr
#include <string>
#include <vector>

TEST_CASE("wc_no_arg")
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 512);

  BINLOG_TRACE_WC(   writer, my_cat, "Hello");
  BINLOG_DEBUG_WC(   writer, my_cat, "Hello");
  BINLOG_INFO_WC(    writer, my_cat, "Hello");
  BINLOG_WARN_WC(    writer, my_cat, "Hello");
  BINLOG_ERROR_WC(   writer, my_cat, "Hello");
  BINLOG_CRITICAL_WC(writer, my_cat, "Hello");

  const std::vector<std::string> expectedEvents{
    "TRAC my_cat Hello",
    "DEBG my_cat Hello",
    "INFO my_cat Hello",
    "WARN my_cat Hello",
    "ERRO my_cat Hello",
    "CRIT my_cat Hello",
  };
  CHECK(getEvents(session, "%S %C %m") == expectedEvents);
}

TEST_CASE("wc_more_args")
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 512);

  const std::unique_ptr<int> pi(new int(123));
  const std::unique_ptr<int> pn;
  const std::map<int, int> m{{1,2}, {3,4}};

  BINLOG_TRACE_WC(   writer, my_cat, "Hello {}", std::string("World"));
  BINLOG_DEBUG_WC(   writer, my_cat, "Hello {}", std::vector<int>{1,2,3});
  BINLOG_INFO_WC(    writer, my_cat, "Hello {} {}", pi, m);
  BINLOG_WARN_WC(    writer, my_cat, "Hello {} {}", pn, pi);
  BINLOG_ERROR_WC(   writer, my_cat, "Hello {}", m);
  BINLOG_CRITICAL_WC(writer, my_cat, "Hello {} {} {} {}", pi, pn, m, 456);

  const std::vector<std::string> expectedEvents{
    "TRAC my_cat Hello World",
    "DEBG my_cat Hello [1, 2, 3]",
    "INFO my_cat Hello 123 [(1, 2), (3, 4)]",
    "WARN my_cat Hello {null} 123",
    "ERRO my_cat Hello [(1, 2), (3, 4)]",
    "CRIT my_cat Hello 123 {null} [(1, 2), (3, 4)] 456",
  };
  CHECK(getEvents(session, "%S %C %m") == expectedEvents);
}

TEST_CASE("wc_writer_name")
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 512);

  writer.setName("W");

  BINLOG_TRACE_WC(   writer, my_cat, "Hello");
  BINLOG_DEBUG_WC(   writer, my_cat, "Hello");
  BINLOG_INFO_WC(    writer, my_cat, "Hello");
  BINLOG_WARN_WC(    writer, my_cat, "Hello");
  BINLOG_ERROR_WC(   writer, my_cat, "Hello");
  BINLOG_CRITICAL_WC(writer, my_cat, "Hello");

  const std::vector<std::string> expectedEvents{
    "TRAC my_cat W Hello",
    "DEBG my_cat W Hello",
    "INFO my_cat W Hello",
    "WARN my_cat W Hello",
    "ERRO my_cat W Hello",
    "CRIT my_cat W Hello",
  };
  CHECK(getEvents(session, "%S %C %n %m") == expectedEvents);
}

TEST_CASE("w_mixed")
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 512);

  writer.setName("W2");

  BINLOG_TRACE_W(   writer, "Hello");
  BINLOG_DEBUG_W(   writer, "Hello");
  BINLOG_INFO_W(    writer, "Hello {}", std::string("World"));
  BINLOG_WARN_W(    writer, "Hello");
  BINLOG_ERROR_W(   writer, "Hello");
  BINLOG_CRITICAL_W(writer, "Hello {}", 123);

  const std::vector<std::string> expectedEvents{
    "TRAC main W2 Hello",
    "DEBG main W2 Hello",
    "INFO main W2 Hello World",
    "WARN main W2 Hello",
    "ERRO main W2 Hello",
    "CRIT main W2 Hello 123",
  };
  CHECK(getEvents(session, "%S %C %n %m") == expectedEvents);
}
