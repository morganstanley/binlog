#include <binlog/detail/Queue.hpp>
#include <binlog/detail/QueueReader.hpp>
#include <binlog/detail/QueueWriter.hpp>

#include <boost/test/unit_test.hpp>

#include <boost/test/data/test_case.hpp>

#include <array>
#include <cstring> // memcmp
#include <random>
#include <thread>
#include <vector>

namespace bdata = boost::unit_test::data;

namespace {

void writeq(binlog::detail::QueueWriter& w, std::size_t size)
{
  BOOST_TEST_REQUIRE(w.beginWrite(size));

  std::unique_ptr<char[]> buf(new char[size]());
  w.writeBuffer(buf.get(), size);
  w.endWrite();
}

void readq(binlog::detail::QueueReader& r, std::size_t size)
{
  const auto rr = r.beginRead();
  BOOST_TEST(rr.size1 + rr.size2 >= size);
  r.endRead();
}

std::array<std::uint8_t, 1255> message_buffer()
{
  std::array<std::uint8_t, 1255> buffer{};
  std::uint8_t c = 0;
  for (auto& b : buffer)
  {
    b = c++;
  }
  return buffer;
}

void write_messages(binlog::detail::QueueWriter& w, int msg_count, unsigned max_msg_size)
{
  const std::array<std::uint8_t, 1255> buffer = message_buffer();

  std::minstd_rand prng(1); // NOLINT

  for (int i = 0; i < msg_count; ++i)
  {
    const auto state = prng();
    const auto msg_size = state % max_msg_size;
    std::uint8_t msg = std::uint8_t(state & 255);

    while (! w.beginWrite(msg_size))
    {
      std::this_thread::yield();
    }

    w.writeBuffer(buffer.data() + msg, msg_size);
    w.endWrite();
  }
}

void read_messages(binlog::detail::QueueReader& r, int msg_count, unsigned max_msg_size)
{
  const std::array<std::uint8_t, 1255> expected_buffer = message_buffer();

  std::minstd_rand prng(1); // NOLINT

  auto check_message = [&](int i, const char* buffer, std::size_t size)
  {
    const auto state = prng();
    const auto msg_size = state % max_msg_size;
    std::uint8_t msg = std::uint8_t(state & 255);

    if (size < msg_size || memcmp(expected_buffer.data() + msg, buffer, msg_size) != 0)
    {
      BOOST_ERROR("Unexpected message content, msg index=" << i);
    }

    return msg_size;
  };

  for (int i = 0; i < msg_count;)
  {
    // read some messages
    auto rr = r.beginRead();
    while (rr.size1 == 0)
    {
      std::this_thread::yield();
      rr = r.beginRead();
    }

    // check messages in the returned buffer.
    // do conditional tests to reduce test log size
    while (rr.size1 != 0)
    {
      const auto msg_size = check_message(i, rr.buffer1, rr.size1);
      ++i;
      rr.buffer1 += msg_size;
      rr.size1 -= msg_size;
    }

    while (rr.size2 != 0)
    {
      const auto msg_size = check_message(i, rr.buffer2, rr.size2);
      ++i;
      rr.buffer2 += msg_size;
      rr.size2 -= msg_size;
    }

    r.endRead();
  }

  BOOST_TEST_MESSAGE("Successfully consumed " << msg_count << " messages");
  BOOST_TEST(true);
}

} // namespace

BOOST_AUTO_TEST_SUITE(Queue)

BOOST_AUTO_TEST_CASE(capacity)
{
  char buffer[1024];
  binlog::detail::Queue q(buffer, 1024);
  BOOST_TEST(q.capacity == 1024);

  binlog::detail::QueueWriter w(q);
  BOOST_TEST(w.capacity() == 1024);

  binlog::detail::QueueReader r(q);
  BOOST_TEST(r.capacity() == 1024);
}

BOOST_AUTO_TEST_CASE(full_capacity)
{
  char buffer[1024];
  binlog::detail::Queue q(buffer, 1024);
  binlog::detail::QueueWriter w(q);

  BOOST_TEST(w.writeCapacity() == 0);
  BOOST_TEST(w.unreadWriteSize() == 0);

  BOOST_TEST(w.beginWrite(1024));
  BOOST_TEST(w.writeCapacity() == 1024);
  BOOST_TEST(w.unreadWriteSize() == 0);

  char buf[512] = {0};
  w.writeBuffer(buf, 512);
  w.endWrite();

  BOOST_TEST(w.writeCapacity() == 512);
  BOOST_TEST(w.unreadWriteSize() == 512);

  BOOST_TEST(w.beginWrite(512));
  w.writeBuffer(buf, 512);
  w.endWrite();

  BOOST_TEST(w.writeCapacity() == 0);
  BOOST_TEST(w.unreadWriteSize() == 1024);
}

BOOST_AUTO_TEST_CASE(full_capacity_almost)
{
  // This test doesn't check a hard requirement,
  // but shows a particular deficiency of the
  // current implementation:
  // The queue can only hold capacity-1 number
  // of bytes, unless read index == 0.
  // I have a dream, that one day,
  // we will have a better queue.
  // Until that happes, this implementation remains, as:
  //  - it is running in production for a long time
  //  - this limitation does not affect logging

  char buffer[1024];
  binlog::detail::Queue q(buffer, 1024);
  binlog::detail::QueueWriter w(q);
  binlog::detail::QueueReader r(q);

  writeq(w, 100);
  readq(r, 100);

  BOOST_TEST(w.unreadWriteSize() == 0);

  writeq(w, 1024 - 100);
  writeq(w, 100                  - 1);
  // Ideally, shouldn't be here: ^^^

  BOOST_TEST(w.writeCapacity() == 0);
  BOOST_TEST(w.unreadWriteSize() == 1024 - 1);
}

BOOST_AUTO_TEST_CASE(unread_write_size)
{
  char buffer[1000];
  binlog::detail::Queue q(buffer, 1000);
  binlog::detail::QueueWriter w(q);
  binlog::detail::QueueReader r(q);

  BOOST_TEST(w.unreadWriteSize() == 0);

  writeq(w, 50);
  BOOST_TEST(w.unreadWriteSize() == 50);

  readq(r, 50);
  BOOST_TEST(w.unreadWriteSize() == 0);

  writeq(w, 900);
  BOOST_TEST(w.unreadWriteSize() == 900);

  readq(r, 900);
  BOOST_TEST(w.unreadWriteSize() == 0);

  writeq(w, 100);
  BOOST_TEST(w.unreadWriteSize() == 100);

  readq(r, 100);
  BOOST_TEST(w.unreadWriteSize() == 0);
}

BOOST_AUTO_TEST_CASE(transmit_one)
{
  char buffer[1000];
  binlog::detail::Queue q(buffer, 1000);
  binlog::detail::QueueWriter w(q);
  binlog::detail::QueueReader r(q);

  // write one byte
  BOOST_TEST(w.beginWrite(1));

  const char data = 'X';
  w.writeBuffer(&data, 1);

  // uncommitted changes are not observable
  BOOST_TEST(r.beginRead().size() == 0);

  w.endWrite();

  // commited changes are observable ...
  auto rr = r.beginRead();
  BOOST_TEST_REQUIRE(rr.size() == 1);
  BOOST_TEST(rr.buffer1[0] == 'X');

  // ... as long as not disposed
  rr = r.beginRead();
  BOOST_TEST_REQUIRE(rr.size() == 1);
  BOOST_TEST(rr.buffer1[0] == 'X');
  r.endRead();

  BOOST_TEST(r.beginRead().size() == 0);
}

BOOST_DATA_TEST_CASE(transmit_more,
  bdata::make({1000U, 1024U, 1U << 20}) * bdata::make({32u, 64u, 128u}),
  queue_size,                             max_msg_size)
{
  std::vector<char> buffer(queue_size);
  binlog::detail::Queue q(buffer.data(), queue_size);
  binlog::detail::QueueReader r(q);
  binlog::detail::QueueWriter w(q);

  const int msg_count = 1'000'000;

  std::thread reader(read_messages, std::ref(r), msg_count, max_msg_size);

  write_messages(w, msg_count, max_msg_size);

  reader.join();
}

BOOST_AUTO_TEST_SUITE_END()
