#include <binlog/EventFilter.hpp>

#include <binlog/Session.hpp>
#include <binlog/SessionWriter.hpp>
#include <binlog/advanced_log_macros.hpp>

#include "test_utils.hpp"

#include <doctest/doctest.h>

#include <sstream>

namespace {

struct FilterAdapter
{
  binlog::EventFilter& filter;
  TestStream stream;

  FilterAdapter& write(const char* buffer, std::streamsize size)
  {
    const std::size_t oldSize = stream.buffer.size();
    const std::size_t writeSize = filter.writeAllowed(buffer, std::size_t(size), stream);
    CHECK(oldSize + writeSize == stream.buffer.size());
    return *this;
  }
};

std::vector<std::string> filterEvents(binlog::Session& session, binlog::EventFilter& filter)
{
  FilterAdapter adapter{filter, {}};
  session.reconsumeMetadata(adapter);
  session.consume(adapter);
  return streamToEvents(adapter.stream, "%S %m");
}

} // namespace

TEST_CASE("allow_none")
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 512);

  binlog::EventFilter filter([](auto){ return false; });

  BINLOG_INFO_W(writer, "Hello");
  CHECK(filterEvents(session, filter) == std::vector<std::string>{});

  BINLOG_WARN_W(writer, "Hello");
  CHECK(filterEvents(session, filter) == std::vector<std::string>{});
}

TEST_CASE("allow_all")
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 512);

  binlog::EventFilter filter([](auto){ return true; });

  BINLOG_INFO_W(writer, "Hello");
  CHECK(filterEvents(session, filter) == std::vector<std::string>{"INFO Hello"});

  BINLOG_WARN_W(writer, "Hello");
  CHECK(filterEvents(session, filter) == std::vector<std::string>{"WARN Hello"});
}

TEST_CASE("allow_some")
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 512);

  binlog::EventFilter filter([](const binlog::EventSource& source){ return source.severity >= binlog::Severity::info; });

  BINLOG_DEBUG_W(writer, "Hello");
  BINLOG_INFO_W(writer, "Hello");
  BINLOG_WARN_W(writer, "Hello");
  const std::vector<std::string> expectedEvents1{
    "INFO Hello", "WARN Hello",
  };
  CHECK(filterEvents(session, filter) == expectedEvents1);

  for (int i = 0; i < 8; ++i)
  {
    BINLOG_DEBUG_W(writer, "i={}", i);
    BINLOG_INFO_W(writer, "i={}", i);
  }
  const std::vector<std::string> expectedEvents2{
    "INFO i=0", "INFO i=1", "INFO i=2", "INFO i=3",
    "INFO i=4", "INFO i=5", "INFO i=6", "INFO i=7",
  };
  CHECK(filterEvents(session, filter) == expectedEvents2);
}
