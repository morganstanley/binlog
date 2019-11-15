#include <binlog/create_source_and_event.hpp>

#include "test_utils.hpp"

#include <binlog/Session.hpp>
#include <binlog/SessionWriter.hpp>
#include <binlog/Severity.hpp>

#include <boost/test/unit_test.hpp>

#include <chrono>
#include <string>
#include <thread>
#include <vector>

namespace {

void writeEvent(binlog::Session& session)
{
  binlog::SessionWriter writer(session, 128);

  BINLOG_CREATE_SOURCE_AND_EVENT(writer, binlog::Severity::info, category, "Hello Concurrent World", 0);
}

} // namespace

BOOST_AUTO_TEST_SUITE(CreateSourceAndEvent)

BOOST_AUTO_TEST_CASE(no_arg)
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 128);

  BINLOG_CREATE_SOURCE_AND_EVENT(writer, binlog::Severity::info, category, "Hello", 0);

  BOOST_TEST(getEvents(session, "%m") == std::vector<std::string>{"Hello"}, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(one_arg)
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 128);

  BINLOG_CREATE_SOURCE_AND_EVENT(
    writer, binlog::Severity::info, category, "Hello {}", 0,
    std::string("World")
  );

  BOOST_TEST(getEvents(session, "%m") == std::vector<std::string>{"Hello World"}, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(several_args)
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 128);

  BINLOG_CREATE_SOURCE_AND_EVENT(
    writer, binlog::Severity::info, category, "Hello {} a={} b={} c={}", 0,
    std::string("World"), 1, true, std::vector<int>{2,3,4}
  );

  BOOST_TEST(getEvents(session, "%m") == std::vector<std::string>{"Hello World a=1 b=true c=[2, 3, 4]"}, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(severity_and_category)
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 128);

  BINLOG_CREATE_SOURCE_AND_EVENT(writer, binlog::Severity::info, my_category, "Hello", 0);

  BOOST_TEST(getEvents(session, "%S %C %m") == std::vector<std::string>{"INFO my_category Hello"}, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(location)
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 128);

  const std::uint64_t line = __LINE__ + 1;
  BINLOG_CREATE_SOURCE_AND_EVENT(writer, binlog::Severity::info, my_category, "Hello", 0);

  std::ostringstream msg;
  msg << __func__ << " " << __FILE__ << " " << line << " Hello";

  BOOST_TEST(getEvents(session, "%M %F %L %m") == std::vector<std::string>{msg.str()}, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(actor)
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 128);
  writer.setName("w1");

  BINLOG_CREATE_SOURCE_AND_EVENT(writer, binlog::Severity::info, category, "Hello", 0);

  BOOST_TEST(getEvents(session, "%n %m") == std::vector<std::string>{"w1 Hello"}, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(time)
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 128);

  const auto now = std::chrono::system_clock::now();
  BINLOG_CREATE_SOURCE_AND_EVENT(
    writer, binlog::Severity::info, category, "Hello",
    std::uint64_t(now.time_since_epoch().count())
  );

  BOOST_TEST(getEvents(session, "%d %m") == std::vector<std::string>{timePointToString(now) + " Hello"}, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(loop)
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 4096);

  for (int i = 0; i < 10; ++i)
  {
    BINLOG_CREATE_SOURCE_AND_EVENT(writer, binlog::Severity::info, category, "{}", 0, i);
  }

  std::stringstream stream;
  session.consume(stream);

  // Make sure only one event source was added
  BOOST_TEST(countTags(stream, binlog::EventSource::Tag) == 1);

  // Make sure events are correct
  std::vector<std::string> expectedEvents;
  expectedEvents.reserve(10);
  for (int i = 0; i < 10; ++i)
  {
    expectedEvents.push_back(std::to_string(i));
  }
  BOOST_TEST(streamToEvents(stream, "%m") == expectedEvents, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(two_threads)
{
  binlog::Session session;

  std::thread threadA(writeEvent, std::ref(session));
  std::thread threadB(writeEvent, std::ref(session));

  threadA.join();
  threadB.join();

  std::stringstream stream;
  session.consume(stream);

  // Exact number of event sources is not specified:
  // It is possible that one thread adds the source first,
  // but it is also legal that both adds the same source,
  // assigning it two different ids, both valid.
  // However, data race is not allowed, TSAN must not be triggered.
  const std::size_t eventSourceCount = countTags(stream, binlog::EventSource::Tag);
  const bool oneOrTwoEventSources = eventSourceCount == 1 || eventSourceCount == 2;
  BOOST_TEST(oneOrTwoEventSources);

  const std::vector<std::string> expectedEvents{
    "Hello Concurrent World",
    "Hello Concurrent World",
  };
  BOOST_TEST(streamToEvents(stream, "%m") == expectedEvents, boost::test_tools::per_element());
}

static_assert(binlog::detail::count_placeholders("") == 0, "");
static_assert(binlog::detail::count_placeholders("foo") == 0, "");
static_assert(binlog::detail::count_placeholders("foo {") == 0, "");
static_assert(binlog::detail::count_placeholders("foo { bar") == 0, "");
static_assert(binlog::detail::count_placeholders("foo { bar }") == 0, "");
static_assert(binlog::detail::count_placeholders("{}") == 1, "");
static_assert(binlog::detail::count_placeholders("{} foo") == 1, "");
static_assert(binlog::detail::count_placeholders("foo {}") == 1, "");
static_assert(binlog::detail::count_placeholders("foo {} bar") == 1, "");
static_assert(binlog::detail::count_placeholders("{{}") == 1, "");
static_assert(binlog::detail::count_placeholders("{}}") == 1, "");
static_assert(binlog::detail::count_placeholders("foo {} bar {}") == 2, "");
static_assert(binlog::detail::count_placeholders("{} foo {} bar {}") == 3, "");
static_assert(binlog::detail::count_placeholders("{}{}{}") == 3, "");
static_assert(binlog::detail::count_placeholders("{{}{}{}}") == 3, "");
static_assert(binlog::detail::count_placeholders("{}{}{}{}{}{}{}{}{}{}") == 10, "");

BOOST_AUTO_TEST_SUITE_END()
