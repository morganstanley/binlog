#include <binlog/Session.hpp>

#include <binlog/SessionWriter.hpp>

#include "test_utils.hpp"

#include <boost/test/unit_test.hpp>

#include <chrono>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

BOOST_AUTO_TEST_SUITE(SessionWriter)

BOOST_AUTO_TEST_CASE(add_event)
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 128);

  binlog::EventSource eventSource{
    0, binlog::Severity::info, "cat", "fun", "file", 123, "a={} b={}", "i[c"
  };
  eventSource.id = session.addEventSource(eventSource);

  BOOST_TEST(writer.addEvent(eventSource.id, 0, 456, std::string("foo")));

  BOOST_TEST(getEvents(session, "%m") == std::vector<std::string>{"a=456 b=foo"}, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(add_event_with_time)
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 128);

  binlog::EventSource eventSource{
    0, binlog::Severity::info, "cat", "fun", "file", 123, "a={} b={}", "i[c"
  };
  eventSource.id = session.addEventSource(eventSource);

  const auto now = std::chrono::system_clock::now();
  const auto clock = std::uint64_t(now.time_since_epoch().count());
  BOOST_TEST(writer.addEvent(eventSource.id, clock, 456, std::string("foo")));

  BOOST_TEST(getEvents(session, "%d %m") == std::vector<std::string>{timePointToString(now) + " a=456 b=foo"}, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(add_event_with_writer_id_name)
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 128);

  binlog::EventSource eventSource{
    0, binlog::Severity::info, "cat", "fun", "file", 123, "a={} b={}", "i[c"
  };
  eventSource.id = session.addEventSource(eventSource);

  writer.setId(111);
  writer.setName("John");
  BOOST_TEST(writer.addEvent(eventSource.id, 0, 456, std::string("foo")));

  BOOST_TEST(getEvents(session, "%t %n %m") == std::vector<std::string>{"111 John a=456 b=foo"}, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(add_event_with_writer_id_name_ctor)
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 128, 111, "John");

  binlog::EventSource eventSource{
    0, binlog::Severity::info, "cat", "fun", "file", 123, "a={} b={}", "i[c"
  };
  eventSource.id = session.addEventSource(eventSource);

  BOOST_TEST(writer.addEvent(eventSource.id, 0, 456, std::string("foo")));

  BOOST_TEST(getEvents(session, "%t %n %m") == std::vector<std::string>{"111 John a=456 b=foo"}, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(add_event_then_close)
{
  // Make sure event reaches the consumer, even after the producer is destructed

  binlog::Session session;

  {
    binlog::SessionWriter writer(session, 128);

    binlog::EventSource eventSource{
      0, binlog::Severity::info, "cat", "fun", "file", 123, "a={} b={}", "i[c"
    };
    eventSource.id = session.addEventSource(eventSource);

    BOOST_TEST(writer.addEvent(eventSource.id, 0, 456, std::string("foo")));
  }

  BOOST_TEST(getEvents(session, "%m") == std::vector<std::string>{"a=456 b=foo"}, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(consume_metadata_twice)
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 128);

  binlog::EventSource eventSource{
    0, binlog::Severity::info, "cat", "fun", "file", 123, "a={} b={}", "i[c"
  };
  eventSource.id = session.addEventSource(eventSource);

  BOOST_TEST(writer.addEvent(eventSource.id, 0, 123, std::string("foo")));
  getEvents(session, ""); // consume metadata and data

  const auto now = std::chrono::system_clock::now();
  const auto clock = std::uint64_t(now.time_since_epoch().count());
  BOOST_TEST(writer.addEvent(eventSource.id, clock, 456, std::string("bar")));

  TestStream stream;
  session.reconsumeMetadata(stream); // add clock sync and event source
  session.consume(stream); // consume the second event

  BOOST_TEST(streamToEvents(stream, "%d %m") == std::vector<std::string>{timePointToString(now) + " a=456 b=bar"}, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(sources_first)
{
  binlog::Session session;
  binlog::SessionWriter writer1(session, 128);
  binlog::SessionWriter writer2(session, 128);

  // Add EventSource 1 from writer 1, add Event using EventSource 2
  {
    binlog::EventSource eventSource{
      0, binlog::Severity::info, "cat", "fun", "file", 123, "a={} b={}", "i[c"
    };
    eventSource.id = session.addEventSource(eventSource);

    BOOST_TEST(writer1.addEvent(eventSource.id, 0, 456, std::string("foo")));
  }

  // Add EventSource 2 from writer 2, add Event using EventSource 1
  {
    binlog::EventSource eventSource{
      0, binlog::Severity::info, "cat", "fun", "file", 123, "c={} d={}", "y[i"
    };
    eventSource.id = session.addEventSource(eventSource);

    BOOST_TEST(writer2.addEvent(eventSource.id, 0, true, std::vector<int>{1,2,3}));
  }

  // Expect correct output: event sources are consumed first.
  const std::vector<std::string> expectedEvents{
    "a=456 b=foo",
    "c=true d=[1, 2, 3]",
  };
  BOOST_TEST(getEvents(session, "%m") == expectedEvents, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(add_events_from_threads)
{
  binlog::Session session;

  binlog::EventSource eventSource{
    0, binlog::Severity::info, "cat", "fun", "file", 123, "a={}", "i"
  };
  eventSource.id = session.addEventSource(eventSource);

  auto writeEvents = [&eventSource, &session](const char* name)
  {
    binlog::SessionWriter writer(session, 4096);
    writer.setName(name);

    for (int i = 0; i < 1000; ++i)
    {
      while (! writer.addEvent(eventSource.id, 0, i)) { std::this_thread::yield(); }
    }
  };

  std::thread threadA(writeEvents, "A");
  std::thread threadB(writeEvents, "B");

  TestStream out;
  std::atomic<bool> write_done{false};
  std::thread consumer([&session, &out, &write_done]()
  {
    while (! write_done.load())
    {
      session.consume(out);
    }

    // consume events written after last consume but before the loop condition was tested
    session.consume(out);
  });

  threadA.join();
  threadB.join();
  write_done.store(true);

  consumer.join();

  std::vector<std::string> events = streamToEvents(out, "%n %m");

  // order of events is not specified across threads: sort them by thread name
  std::stable_sort(
    events.begin(), events.end(),
    [](const std::string& a, const std::string& b) { return a[0] < b[0]; }
  );

  // generate expected events: not easy on memory, but keeps the report small
  std::vector<std::string> expectedEvents;
  expectedEvents.reserve(2000);
  for (char writerName : std::string{"AB"})
  {
    for (int i = 0; i < 1000; ++i)
    {
      std::ostringstream s;
      s << writerName << " a=" << i;
      expectedEvents.push_back(s.str());
    }
  }

  BOOST_TEST(events == expectedEvents, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(queue_is_full)
{
  binlog::Session session;
  binlog::SessionWriter writer(session, 128);

  writer.setId(7);
  writer.setName("Seven");

  binlog::EventSource eventSource{
    0, binlog::Severity::info, "cat", "fun", "file", 123, "a={}", "[i"
  };
  eventSource.id = session.addEventSource(eventSource);

  // add more data that would otherwise fit in the queue
  for (int i = 0; i < 256; ++i)
  {
    // even if the queue is full, writer allocates a new queue, returns true
    BOOST_TEST(writer.addEvent(eventSource.id, 0, std::vector<int>{i,i+1,i+2}));
  }

  std::vector<std::string> expectedEvents;
  expectedEvents.reserve(256);
  for (int i = 0; i < 256; ++i)
  {
    std::ostringstream s;
    s << "7 Seven a=[" << i << ", " << i+1 << ", " << i+2 << "]";
    expectedEvents.push_back(s.str());
  }

  TestStream stream;
  const binlog::Session::ConsumeResult cr = session.consume(stream);

  // make sure old channels are closed
  BOOST_TEST(cr.channelsPolled > 1);
  BOOST_TEST(cr.channelsRemoved + 1 == cr.channelsPolled);

  // make sure the events are correct, and the writer properties are preserved
  BOOST_TEST(streamToEvents(stream, "%t %n %m") == expectedEvents, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(move_ctor)
{
  binlog::Session session;
  binlog::SessionWriter writerToBeMoved(session, 128);
  binlog::SessionWriter writer(std::move(writerToBeMoved));

  // no channel is closed
  std::ostringstream out;
  const binlog::Session::ConsumeResult cr = session.consume(out);
  BOOST_TEST(cr.channelsPolled == 1);
  BOOST_TEST(cr.channelsRemoved == 0);

  // channel is still operational
  binlog::EventSource eventSource{
    0, binlog::Severity::info, "cat", "fun", "file", 123, "a={} b={}", "i[c"
  };
  eventSource.id = session.addEventSource(eventSource);

  BOOST_TEST(writer.addEvent(eventSource.id, 0, 456, std::string("foo")));

  BOOST_TEST(getEvents(session, "%m") == std::vector<std::string>{"a=456 b=foo"}, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(move_assign)
{
  binlog::Session session;
  binlog::SessionWriter writer1(session, 128);
  binlog::SessionWriter writer2(session, 128);

  writer1.setName("W1");
  writer2.setName("W2");

  binlog::EventSource eventSource{
    0, binlog::Severity::info, "cat", "fun", "file", 123, "a={} b={}", "i[c"
  };
  eventSource.id = session.addEventSource(eventSource);

  BOOST_TEST(writer1.addEvent(eventSource.id, 0, 123, std::string("foo")));
  BOOST_TEST(writer2.addEvent(eventSource.id, 0, 456, std::string("bar")));

  writer2 = std::move(writer1);
  BOOST_TEST(writer2.addEvent(eventSource.id, 0, 789, std::string("baz")));

  const std::vector<std::string> expectedEvents{
    "W1 a=123 b=foo",
    "W1 a=789 b=baz",
    "W2 a=456 b=bar",
  };
  BOOST_TEST(getEvents(session, "%n %m") == expectedEvents, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_SUITE_END()
