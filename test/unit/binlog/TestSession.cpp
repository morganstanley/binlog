#include <binlog/Session.hpp>

#include <binlog/Entries.hpp>

#include <boost/test/unit_test.hpp>

#include <ios> // streamsize

namespace {

struct NullOstream
{
  NullOstream& write(const char*, std::streamsize) { return *this; }
};

} // namespace

BOOST_AUTO_TEST_SUITE(Session)

BOOST_AUTO_TEST_CASE(channel_lifecycle)
{
  binlog::Session session;

  NullOstream out;

  binlog::Session::ConsumeResult cr = session.consume(out);
  BOOST_TEST(cr.channelsPolled == 0);
  BOOST_TEST(cr.channelsRemoved == 0);

  binlog::Session::Channel& ch1 = session.createChannel(128);

  cr = session.consume(out);
  BOOST_TEST(cr.channelsPolled == 1);
  BOOST_TEST(cr.channelsRemoved == 0);

  binlog::Session::Channel& ch2 = session.createChannel(128);

  cr = session.consume(out);
  BOOST_TEST(cr.channelsPolled == 2);
  BOOST_TEST(cr.channelsRemoved == 0);

  ch1.closed = true;
  cr = session.consume(out);
  BOOST_TEST(cr.channelsPolled == 2);
  BOOST_TEST(cr.channelsRemoved == 1);

  ch2.closed = true;
  cr = session.consume(out);
  BOOST_TEST(cr.channelsPolled == 1);
  BOOST_TEST(cr.channelsRemoved == 1);

  cr = session.consume(out);
  BOOST_TEST(cr.channelsPolled == 0);
  BOOST_TEST(cr.channelsRemoved == 0);
}

BOOST_AUTO_TEST_CASE(set_channel_name)
{
  binlog::Session session;
  binlog::Session::Channel& ch = session.createChannel(128);

  session.setChannelWriterName(ch, "Sio");
  BOOST_TEST(ch.writerProp.name == "Sio");
}

BOOST_AUTO_TEST_CASE(min_severity)
{
  binlog::Session session;
  BOOST_TEST((session.minSeverity() == binlog::Severity::trace));

  session.setMinSeverity(binlog::Severity::info);
  BOOST_TEST((session.minSeverity() == binlog::Severity::info));
}

BOOST_AUTO_TEST_CASE(sources_consumed_once)
{
  binlog::Session session;
  binlog::EventSource eventSource;
  session.addEventSource(eventSource);

  NullOstream out;
  binlog::Session::ConsumeResult cr = session.consume(out);
  BOOST_TEST(cr.bytesConsumed != 0);

  cr = session.consume(out);
  BOOST_TEST(cr.bytesConsumed == 0);
}

// addEventSource and consume are further tested in TestSessionWriter.cpp

BOOST_AUTO_TEST_SUITE_END()
