#include <binlog/detail/SegmentedMap.hpp>

#include <doctest/doctest.h>

using IntMap = binlog::detail::SegmentedMap<int>;

TEST_CASE("basics")
{
  IntMap m;
  CHECK(m.empty());
  CHECK(m.size() == 0);
  CHECK(m.find(0) == m.end());
  CHECK(m.find(123) == m.end());
  CHECK(m.find(IntMap::key_type(-1)) == m.end());
}

TEST_CASE("emplace")
{
  IntMap m;

  CHECK(m.find(123) == m.end());

  m.emplace(123, 1000);
  auto&& f1 = m.find(123);
  CHECK(f1 != m.end());
  CHECK(*f1 == 1000);
  CHECK(m.find(0) == m.end());
  CHECK(m.find(122) == m.end());
  CHECK(m.find(124) == m.end());
  CHECK(! m.empty());
  CHECK(m.size() == 1);
}

TEST_CASE("emplace_more")
{
  IntMap m;

  for (int j = 0; j < 5000; j += 1000)
  {
    for (int i = 0; i < 100; ++i)
    {
      const int key = j + i;
      m.emplace(IntMap::key_type(key), j * i);
    }
  }

  CHECK(m.size() == 500);

  for (int j = 0; j < 5000; j += 1000)
  {
    for (int i = 0; i < 100; ++i)
    {
      const int key = j + i;
      CHECK(*m.find(IntMap::key_type(key)) == j * i);
    }
  }
}
