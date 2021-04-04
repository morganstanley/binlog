#include <binlog/Session.hpp>

#include <binlog/Entries.hpp>

#include <doctest/doctest.h>

#include <ios> // streamsize

namespace {

struct NullOstream
{
  NullOstream& write(const char*, std::streamsize) { return *this; }
};

} // namespace

TEST_CASE("channel_lifecycle")
{
  binlog::Session session;

  NullOstream out;

  binlog::Session::ConsumeResult cr = session.consume(out);
  CHECK(cr.channelsPolled == 0);
  CHECK(cr.channelsRemoved == 0);

  std::shared_ptr<binlog::Session::Channel> ch1 = session.createChannel(128);

  cr = session.consume(out);
  CHECK(cr.channelsPolled == 1);
  CHECK(cr.channelsRemoved == 0);

  std::shared_ptr<binlog::Session::Channel> ch2 = session.createChannel(128);

  cr = session.consume(out);
  CHECK(cr.channelsPolled == 2);
  CHECK(cr.channelsRemoved == 0);

  ch1.reset();
  cr = session.consume(out);
  CHECK(cr.channelsPolled == 2);
  CHECK(cr.channelsRemoved == 1);

  ch2.reset();
  cr = session.consume(out);
  CHECK(cr.channelsPolled == 1);
  CHECK(cr.channelsRemoved == 1);

  cr = session.consume(out);
  CHECK(cr.channelsPolled == 0);
  CHECK(cr.channelsRemoved == 0);
}

TEST_CASE("set_channel_name")
{
  binlog::Session session;
  std::shared_ptr<binlog::Session::Channel> ch = session.createChannel(128);

  session.setChannelWriterName(*ch, "Sio");
  CHECK(ch->writerProp.name == "Sio");
}

TEST_CASE("min_severity")
{
  binlog::Session session;
  CHECK(session.minSeverity() == binlog::Severity::trace);

  session.setMinSeverity(binlog::Severity::info);
  CHECK(session.minSeverity() == binlog::Severity::info);
}

TEST_CASE("sources_consumed_once")
{
  binlog::Session session;
  binlog::EventSource eventSource;
  session.addEventSource(eventSource);

  NullOstream out;
  binlog::Session::ConsumeResult cr = session.consume(out);
  CHECK(cr.bytesConsumed != 0);

  cr = session.consume(out);
  CHECK(cr.bytesConsumed == 0);
}

// addEventSource and consume are further tested in TestSessionWriter.cpp
