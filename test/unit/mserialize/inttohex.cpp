#include <mserialize/visit.hpp>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <ios>
#include <limits>
#include <sstream>
#include <string>

namespace {

std::string hexvalue(int i)
{
  std::ostringstream ostream;
  ostream << std::hex << std::uppercase << i;
  return ostream.str();
}

} // namespace

BOOST_AUTO_TEST_SUITE(MserializeInttohex)

BOOST_AUTO_TEST_CASE(empty)
{
  mserialize::detail::IntegerToHex hex;
  BOOST_TEST(hex.value() == "");
  BOOST_TEST(hex.delimited_value('x', 'y') == "xy");
}

BOOST_AUTO_TEST_CASE(convert_positive_int)
{
  for (int i = 0; i < 512; ++i)
  {
    mserialize::detail::IntegerToHex hex;
    hex.visit(i);
    BOOST_TEST(hex.value() == hexvalue(i));
    BOOST_TEST(hex.delimited_value('!', '?') == '!' + hexvalue(i) + '?');
  }
}

BOOST_AUTO_TEST_CASE(convert_min)
{
  mserialize::detail::IntegerToHex hex;
  hex.visit(std::numeric_limits<std::int64_t>::min());
  BOOST_TEST(hex.value() == "-8000000000000000");
  BOOST_TEST(hex.delimited_value('!', '?') == "!-8000000000000000?");
}

BOOST_AUTO_TEST_CASE(convert_max)
{
  mserialize::detail::IntegerToHex hex;
  hex.visit(std::numeric_limits<std::uint64_t>::max());
  BOOST_TEST(hex.value() == "FFFFFFFFFFFFFFFF");
  BOOST_TEST(hex.delimited_value('!', '?') == "!FFFFFFFFFFFFFFFF?");
}

BOOST_AUTO_TEST_CASE(multi_visit)
{
  mserialize::detail::IntegerToHex hex;

  for (int i = 0; i <= 256; ++i)
  {
    hex.visit(i);
  }

  // only the last visited integer value is kept:
  BOOST_TEST(hex.value() == "100");
  BOOST_TEST(hex.delimited_value('x', 'y') == "x100y");
}

BOOST_AUTO_TEST_SUITE_END()
