#include <binlog/detail/OstreamBuffer.hpp>

#include <doctest/doctest.h>

#include <cstdint>
#include <sstream>

namespace {

struct TestcaseBase
{
  std::ostringstream str;
  binlog::detail::OstreamBuffer buf{str};

  std::string result()
  {
    buf.flush();
    return str.str();
  }

  template <typename T>
  std::string toString(T t)
  {
    buf << t;
    std::string s = result();
    str.str({}); // clear ostream
    return s;
  }
};

} // namespace

TEST_CASE_FIXTURE(TestcaseBase, "empty")
{
  CHECK(result() == "");
  buf.write(nullptr, 0);
  CHECK(result() == "");
}

TEST_CASE_FIXTURE(TestcaseBase, "put")
{
  buf.put('a');
  buf.put('b');
  buf.put('c');
  CHECK(result() == "abc");
}

TEST_CASE_FIXTURE(TestcaseBase, "write")
{
  buf.write("defgh", 5);
  CHECK(result() == "defgh");
}

TEST_CASE_FIXTURE(TestcaseBase, "shift_op")
{
  CHECK(toString(true) == "true");
  CHECK(toString(false) == "false");

  CHECK(toString('x') == "x");

  CHECK(toString<std::int8_t>(23) == "23");
  CHECK(toString<std::int16_t>(123) == "123");
  CHECK(toString<std::int32_t>(-123) == "-123");
  CHECK(toString<std::int64_t>(1357246) == "1357246");

  CHECK(toString<std::uint8_t>(23) == "23");
  CHECK(toString<std::uint16_t>(123) == "123");
  CHECK(toString<std::uint32_t>(456) == "456");
  CHECK(toString<std::uint64_t>(1357246) == "1357246");

  CHECK(toString<float>(0.0f) == "0");
  CHECK(toString<float>(1.0f) == "1");
  CHECK(toString<float>(120.5625f) == "120.5625");

  CHECK(toString<double>(0.0) == "0");
  CHECK(toString<double>(1.0) == "1");
  CHECK(toString<double>(120.5625) == "120.5625");

  CHECK(toString<long double>(0.0) == "0");
  CHECK(toString<long double>(1.0) == "1");
  CHECK(toString<long double>(-120.5625) == "-120.5625");
  CHECK(toString<long double>(1234234.0234242) == "1234234.0234242");

  CHECK(toString("foobar") == "foobar");
}
