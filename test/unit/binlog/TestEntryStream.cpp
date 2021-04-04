#include <binlog/EntryStream.hpp>

#include "test_utils.hpp"

#include <doctest/doctest.h>

#include <cstdint>
#include <cstring> // strncmp
#include <sstream>

TEST_CASE("istream_empty")
{
  std::istringstream stream;
  binlog::IstreamEntryStream entryStream(stream);

  const binlog::Range empty = entryStream.nextEntryPayload();
  CHECK(empty.size() == 0);
}

TEST_CASE("istream_two_entries")
{
  constexpr std::uint32_t size1 = 8;
  const char payload1[size1] = "abcdefg";

  constexpr std::uint32_t size2 = 4;
  const char payload2[size2] = "hij";

  std::stringstream stream;

  stream.write(reinterpret_cast<const char*>(&size1), sizeof(size1));
  stream.write(payload1, sizeof(payload1));

  stream.write(reinterpret_cast<const char*>(&size2), sizeof(size2));
  stream.write(payload2, sizeof(payload2));

  binlog::IstreamEntryStream entryStream(stream);

  binlog::Range range1 = entryStream.nextEntryPayload();
  CHECK(strncmp(range1.view(size1), payload1, size1) == 0);

  binlog::Range range2 = entryStream.nextEntryPayload();
  CHECK(strncmp(range2.view(size2), payload2, size2) == 0);

  const binlog::Range empty = entryStream.nextEntryPayload();
  CHECK(empty.size() == 0);
}

TEST_CASE("istream_zero_size")
{
  std::stringstream stream;
  const std::uint32_t size = 0;
  stream.write(reinterpret_cast<const char*>(&size), sizeof(size));

  binlog::IstreamEntryStream entryStream(stream);

  const binlog::Range empty = entryStream.nextEntryPayload();
  CHECK(empty.size() == 0);
  CHECK(stream.tellg() == sizeof(size));
}

TEST_CASE("istream_incomplete_size")
{
  std::stringstream stream;
  stream.write("abcd", 4);
  stream.seekg(2);

  binlog::IstreamEntryStream entryStream(stream);

  CHECK_THROWS_AS(entryStream.nextEntryPayload(), std::runtime_error);
  CHECK(stream.tellg() == 2);
}

TEST_CASE("istream_incomplete_payload")
{
  std::stringstream stream;
  stream.write("abcd", 4);
  stream.seekg(4);

  constexpr std::uint32_t size = 8;
  const char payload[size-1] = "abcdef";

  stream.write(reinterpret_cast<const char*>(&size), sizeof(size));
  stream.write(payload, std::streamsize(size-1));

  binlog::IstreamEntryStream entryStream(stream);

  CHECK_THROWS_AS(entryStream.nextEntryPayload(), std::runtime_error);
  CHECK(stream.tellg() == 4);
}

TEST_CASE("range_empty")
{
  binlog::RangeEntryStream entryStream(binlog::Range{});

  const binlog::Range empty = entryStream.nextEntryPayload();
  CHECK(empty.size() == 0);
}

TEST_CASE("range_two_entries")
{
  constexpr std::uint32_t size1 = 8;
  const char payload1[size1] = "abcdefg";

  constexpr std::uint32_t size2 = 4;
  const char payload2[size2] = "hij";

  TestStream stream;

  stream.write(reinterpret_cast<const char*>(&size1), sizeof(size1));
  stream.write(payload1, sizeof(payload1));

  stream.write(reinterpret_cast<const char*>(&size2), sizeof(size2));
  stream.write(payload2, sizeof(payload2));

  const binlog::Range range{stream.buffer.data(), stream.buffer.size()};
  binlog::RangeEntryStream entryStream(range);

  binlog::Range range1 = entryStream.nextEntryPayload();
  CHECK(strncmp(range1.view(size1), payload1, size1) == 0);

  binlog::Range range2 = entryStream.nextEntryPayload();
  CHECK(strncmp(range2.view(size2), payload2, size2) == 0);

  const binlog::Range empty = entryStream.nextEntryPayload();
  CHECK(empty.size() == 0);
}

TEST_CASE("range_zero_size")
{
  const std::uint32_t size = 0;
  const binlog::Range range{reinterpret_cast<const char*>(&size), sizeof(size)};
  binlog::RangeEntryStream entryStream(range);

  const binlog::Range empty = entryStream.nextEntryPayload();
  CHECK(empty.size() == 0);
}

TEST_CASE("range_incomplete_size")
{
  const binlog::Range range{"ab", 2};
  binlog::RangeEntryStream entryStream(range);

  CHECK_THROWS_AS(entryStream.nextEntryPayload(), std::runtime_error);
}

TEST_CASE("range_incomplete_payload")
{
  constexpr std::uint32_t size = 8;
  const char payload[size-1] = "abcdef";

  TestStream stream;
  stream.write(reinterpret_cast<const char*>(&size), sizeof(size));
  stream.write(payload, std::streamsize(size-1));

  const binlog::Range range{stream.buffer.data(), stream.buffer.size()};
  binlog::RangeEntryStream entryStream(range);

  CHECK_THROWS_AS(entryStream.nextEntryPayload(), std::runtime_error);
}
