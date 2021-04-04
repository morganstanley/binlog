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
  CHECK(toString<float>(1.2f) == "1.2");
  CHECK(toString<float>(123.456f) == "123.456");

  CHECK(toString<double>(0.0) == "0");
  CHECK(toString<double>(1.0) == "1");
  CHECK(toString<double>(-1.2) == "-1.2");
  CHECK(toString<double>(123.456) == "123.456");

  CHECK(toString<long double>(0.0) == "0");
  CHECK(toString<long double>(1.0) == "1");
  CHECK(toString<long double>(1.2) == "1.2");
  CHECK(toString<long double>(-123.456) == "-123.456");

  CHECK(toString("foobar") == "foobar");
}
