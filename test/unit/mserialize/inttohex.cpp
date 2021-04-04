#include <mserialize/visit.hpp>

#include <doctest/doctest.h>

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

TEST_CASE("empty")
{
  mserialize::detail::IntegerToHex hex;
  CHECK(hex.value() == "");
  CHECK(hex.delimited_value('x', 'y') == "xy");
}

TEST_CASE("convert_positive_int")
{
  for (int i = 0; i < 512; ++i)
  {
    mserialize::detail::IntegerToHex hex;
    hex.visit(i);
    CHECK(hex.value() == hexvalue(i));
    CHECK(hex.delimited_value('!', '?') == '!' + hexvalue(i) + '?');
  }
}

TEST_CASE("convert_min")
{
  mserialize::detail::IntegerToHex hex;
  hex.visit(std::numeric_limits<std::int64_t>::min());
  CHECK(hex.value() == "-8000000000000000");
  CHECK(hex.delimited_value('!', '?') == "!-8000000000000000?");
}

TEST_CASE("convert_max")
{
  mserialize::detail::IntegerToHex hex;
  hex.visit(std::numeric_limits<std::uint64_t>::max());
  CHECK(hex.value() == "FFFFFFFFFFFFFFFF");
  CHECK(hex.delimited_value('!', '?') == "!FFFFFFFFFFFFFFFF?");
}

TEST_CASE("multi_visit")
{
  mserialize::detail::IntegerToHex hex;

  for (int i = 0; i <= 256; ++i)
  {
    hex.visit(i);
  }

  // only the last visited integer value is kept:
  CHECK(hex.value() == "100");
  CHECK(hex.delimited_value('x', 'y') == "x100y");
}
