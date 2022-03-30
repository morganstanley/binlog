#include <binlog/detail/Cueue.hpp>

#include <doctest/doctest.h>

#include <array>
#include <cstring> // memcmp
#include <random>
#include <thread>
#include <vector>

namespace {

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

void write_messages(binlog::detail::CueueWriter& w, int msg_count, unsigned max_msg_size)
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

    w.write(buffer.data() + msg, std::streamsize(msg_size));
    w.endWrite();
  }
}

void read_messages(binlog::detail::CueueReader& r, int msg_count, unsigned max_msg_size)
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
      FAIL("Unexpected message content, msg index=", i);
    }

    return msg_size;
  };

  for (int i = 0; i < msg_count;)
  {
    // read some messages
    auto rr = r.beginRead();
    while (rr.size == 0)
    {
      std::this_thread::yield();
      rr = r.beginRead();
    }

    // check messages in the returned buffer.
    // do conditional tests to reduce test log size
    while (rr.size != 0)
    {
      const auto msg_size = check_message(i, rr.buffer, rr.size);
      ++i;
      rr.buffer += msg_size;
      rr.size -= msg_size;
    }

    r.endRead();
  }

  CHECK_MESSAGE(true, "Successfully consumed ", msg_count, " messages");
}

} // namespace

TEST_CASE("full_capacity")
{
  std::size_t capacity = 4096;
  binlog::detail::Cueue q(capacity);
  capacity = q.capacity();

  auto w = q.writer();
  auto r = q.reader();

  // full capacity can be used
  CHECK(w.beginWrite() == capacity);
  const std::vector<char> wbuf(capacity, '0');
  w.write(wbuf.data(), std::streamsize(capacity));
  w.endWrite();

  // no space left to write
  CHECK(w.beginWrite() == 0);

  // reader can observe comitted data
  auto rr1 = r.beginRead();
  CHECK(rr1.size == capacity);
  CHECK(rr1.buffer != nullptr);
  r.endRead();

  // full capacity can be used, even if
  // only one byte was left at the end
  CHECK(w.beginWrite() == capacity);
  w.write(wbuf.data(), std::streamsize(capacity-1));
  w.endWrite();

  auto rr2 = r.beginRead();
  CHECK(rr2.size == capacity-1);
  CHECK(rr2.buffer != nullptr);
  r.endRead();

  CHECK(w.beginWrite() == capacity);
  CHECK(w.beginWrite(capacity) == true);
  // ... but no more
  CHECK(w.beginWrite(capacity+1) == false);
}

TEST_CASE("transmit_one")
{
  std::size_t capacity = 4096;
  binlog::detail::Cueue q(capacity);

  auto w = q.writer();
  auto r = q.reader();

  // full capacity can be written
  CHECK(w.beginWrite(capacity));

  // uncommitted changes are not observable
  CHECK(r.beginRead().size == 0);

  CHECK(w.beginWrite(capacity));

  const char data = 'X';
  w.write(&data, 1);
  w.endWrite();

  // commited changes are observable ...
  auto rr = r.beginRead();
  REQUIRE(rr.size == 1);
  CHECK(rr.buffer[0] == 'X');

  // ... as long as not disposed
  rr = r.beginRead();
  REQUIRE(rr.size == 1);
  CHECK(rr.buffer[0] == 'X');

  r.endRead();
  CHECK(r.beginRead().size == 0);
}

TEST_CASE("transmit_more")
{
  for (const unsigned capacity : {4096U, 4 * 4096U, 1U << 20})
  {
    for (const unsigned max_msg_size : {32U, 64U, 128U})
    {
      binlog::detail::Cueue q(capacity);
      auto w = q.writer();
      auto r = q.reader();

      const int msg_count = 1'000'000;

      std::thread reader(read_messages, std::ref(r), msg_count, max_msg_size);

      write_messages(w, msg_count, max_msg_size);

      reader.join();
    }
  }
}
