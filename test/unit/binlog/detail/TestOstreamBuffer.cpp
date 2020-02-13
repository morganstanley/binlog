#include <binlog/detail/OstreamBuffer.hpp>

#include <boost/test/unit_test.hpp>

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

BOOST_AUTO_TEST_SUITE(OstreamBuffer)

BOOST_FIXTURE_TEST_CASE(empty, TestcaseBase)
{
  BOOST_TEST(result() == "");
  buf.write(nullptr, 0);
  BOOST_TEST(result() == "");
}

BOOST_FIXTURE_TEST_CASE(put, TestcaseBase)
{
  buf.put('a');
  buf.put('b');
  buf.put('c');
  BOOST_TEST(result() == "abc");
}

BOOST_FIXTURE_TEST_CASE(write, TestcaseBase)
{
  buf.write("defgh", 5);
  BOOST_TEST(result() == "defgh");
}

BOOST_FIXTURE_TEST_CASE(shift_op, TestcaseBase)
{
  BOOST_TEST(toString(true) == "true");
  BOOST_TEST(toString(false) == "false");

  BOOST_TEST(toString('x') == "x");

  BOOST_TEST(toString<std::int8_t>(23) == "23");
  BOOST_TEST(toString<std::int16_t>(123) == "123");
  BOOST_TEST(toString<std::int32_t>(-123) == "-123");
  BOOST_TEST(toString<std::int64_t>(1357246) == "1357246");

  BOOST_TEST(toString<std::uint8_t>(23) == "23");
  BOOST_TEST(toString<std::uint16_t>(123) == "123");
  BOOST_TEST(toString<std::uint32_t>(456) == "456");
  BOOST_TEST(toString<std::uint64_t>(1357246) == "1357246");

  BOOST_TEST(toString<float>(0.0f) == "0");
  BOOST_TEST(toString<float>(1.0f) == "1");
  BOOST_TEST(toString<float>(1.2f) == "1.2");
  BOOST_TEST(toString<float>(123.456f) == "123.456");

  BOOST_TEST(toString<double>(0.0) == "0");
  BOOST_TEST(toString<double>(1.0) == "1");
  BOOST_TEST(toString<double>(-1.2) == "-1.2");
  BOOST_TEST(toString<double>(123.456) == "123.456");

  BOOST_TEST(toString<long double>(0.0) == "0");
  BOOST_TEST(toString<long double>(1.0) == "1");
  BOOST_TEST(toString<long double>(1.2) == "1.2");
  BOOST_TEST(toString<long double>(-123.456) == "-123.456");

  BOOST_TEST(toString("foobar") == "foobar");
}

BOOST_AUTO_TEST_SUITE_END()
