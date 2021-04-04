#include <binlog/basic_log_macros.hpp>

#include "test_utils.hpp"

#include <doctest/doctest.h>

#include <map>
#include <memory> // unique_ptr
#include <ostream>
#include <string>
#include <thread>
#include <vector>

namespace {

std::vector<std::string> getEventsFromDefaultSession(const char* eventFormat)
{
  TestStream stream;
  const binlog::Session::ConsumeResult cr = binlog::consume(stream);
  CHECK(stream.buffer.size() == cr.bytesConsumed);
  return streamToEvents(stream, eventFormat);
}

} // namespace

TEST_CASE("no_arg")
{
  BINLOG_TRACE("Hello");
  BINLOG_DEBUG("Hello");
  BINLOG_INFO("Hello");
  BINLOG_WARN("Hello");
  BINLOG_ERROR("Hello");
  BINLOG_CRITICAL("Hello");

  const std::vector<std::string> expectedEvents{
    "TRAC main Hello",
    "DEBG main Hello",
    "INFO main Hello",
    "WARN main Hello",
    "ERRO main Hello",
    "CRIT main Hello",
  };
  CHECK(getEventsFromDefaultSession("%S %C %m") == expectedEvents);
}

TEST_CASE("more_args")
{
  const std::unique_ptr<int> pi(new int(123));
  const std::unique_ptr<int> pn;
  const std::map<int, int> m{{1,2}, {3,4}};

  BINLOG_TRACE("Hello {}", std::string("World"));
  BINLOG_DEBUG("Hello {}", std::vector<int>{1,2,3});
  BINLOG_INFO("Hello {} {}", pi, m);
  BINLOG_WARN("Hello {} {}", pn, pi);
  BINLOG_ERROR("Hello {}", m);
  BINLOG_CRITICAL("Hello {} {} {} {}", pi, pn, m, 456);

  const std::vector<std::string> expectedEvents{
    "TRAC main Hello World",
    "DEBG main Hello [1, 2, 3]",
    "INFO main Hello 123 [(1, 2), (3, 4)]",
    "WARN main Hello {null} 123",
    "ERRO main Hello [(1, 2), (3, 4)]",
    "CRIT main Hello 123 {null} [(1, 2), (3, 4)] 456",
  };
  CHECK(getEventsFromDefaultSession("%S %C %m") == expectedEvents);
}

TEST_CASE("writer_name")
{
  binlog::default_thread_local_writer().setName("W");

  BINLOG_TRACE("Hello");
  BINLOG_DEBUG("Hello");
  BINLOG_INFO("Hello");
  BINLOG_WARN("Hello");
  BINLOG_ERROR("Hello");
  BINLOG_CRITICAL("Hello");

  const std::vector<std::string> expectedEvents{
    "TRAC main W Hello",
    "DEBG main W Hello",
    "INFO main W Hello",
    "WARN main W Hello",
    "ERRO main W Hello",
    "CRIT main W Hello",
  };
  CHECK(getEventsFromDefaultSession("%S %C %n %m") == expectedEvents);
}

TEST_CASE("default_writer_name")
{
  std::thread t([]() // start a new thread to test the defaults
  {
    std::ostringstream namestream;
    namestream << std::this_thread::get_id();
    const std::string name = namestream.str();

    BINLOG_TRACE("Hello");
    BINLOG_DEBUG("Hello");
    BINLOG_INFO("Hello");
    BINLOG_WARN("Hello");
    BINLOG_ERROR("Hello");
    BINLOG_CRITICAL("Hello");

    const std::vector<std::string> expectedEvents{
      "TRAC main " + name + " Hello",
      "DEBG main " + name + " Hello",
      "INFO main " + name + " Hello",
      "WARN main " + name + " Hello",
      "ERRO main " + name + " Hello",
      "CRIT main " + name + " Hello",
    };
    CHECK(getEventsFromDefaultSession("%S %C %n %m") == expectedEvents);
  });
  t.join();
}

TEST_CASE("c_mixed")
{
  binlog::default_thread_local_writer().setName("W2");

  BINLOG_TRACE_C(   some_cat, "Hello");
  BINLOG_DEBUG_C(   some_cat, "Hello");
  BINLOG_INFO_C(    some_cat, "Hello {}", std::string("World"));
  BINLOG_WARN_C(    some_cat, "Hello");
  BINLOG_ERROR_C(   some_other_cat, "Hello");
  BINLOG_CRITICAL_C(some_cat, "Hello {}", 123);

  const std::vector<std::string> expectedEvents{
    "TRAC some_cat W2 Hello",
    "DEBG some_cat W2 Hello",
    "INFO some_cat W2 Hello World",
    "WARN some_cat W2 Hello",
    "ERRO some_other_cat W2 Hello",
    "CRIT some_cat W2 Hello 123",
  };
  CHECK(getEventsFromDefaultSession("%S %C %n %m") == expectedEvents);
}
