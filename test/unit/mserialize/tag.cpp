#include "test_enums.hpp"

#include <mserialize/make_enum_tag.hpp>
#include <mserialize/tag.hpp>

#include <cstdint>
#include <memory>
#include <tuple>
#include <vector>

namespace {

template <size_t N>
constexpr bool operator==(const mserialize::cx_string<N>& a, const char (&b)[N+1])
{
  for (size_t i = 0; i <= N; ++i)
  {
    if (a[i] != b[i]) { return false; }
  }
  return true;
}

} // namespace

static_assert(mserialize::tag<bool>() == "y", "");
static_assert(mserialize::tag<char>() == "c", "");

static_assert(mserialize::tag<std::int8_t>()  == "b", "");
static_assert(mserialize::tag<std::int16_t>() == "s", "");
static_assert(mserialize::tag<std::int32_t>() == "i", "");
static_assert(mserialize::tag<std::int64_t>() == "l", "");

static_assert(mserialize::tag<std::uint8_t>()  == "B", "");
static_assert(mserialize::tag<std::uint16_t>() == "S", "");
static_assert(mserialize::tag<std::uint32_t>() == "I", "");
static_assert(mserialize::tag<std::uint64_t>() == "L", "");

static_assert(mserialize::tag<float>() == "f", "");
static_assert(mserialize::tag<double>() == "d", "");
static_assert(mserialize::tag<long double>() == "D", "");

static_assert(mserialize::tag<std::vector<int>>() == "[i", "");
static_assert(mserialize::tag<std::vector<float>>() == "[f", "");
static_assert(mserialize::tag<std::vector<bool>>() == "[y", "");

static_assert(mserialize::tag<std::pair<int, double>>() == "(id)", "");
static_assert(mserialize::tag<std::tuple<>>() == "()", "");
static_assert(mserialize::tag<std::tuple<float, double, char>>() == "(fdc)", "");

static_assert(mserialize::tag<std::unique_ptr<int>>() == "<0i>", "");

static_assert(mserialize::tag<std::unique_ptr<
  std::tuple<
    std::vector<int>,
    std::pair<bool, char>,
    std::shared_ptr<long double>
  >
>>() == "<0([i(yc)<0D>)>", "");

// const/ref qualifier does not change the result

static_assert(mserialize::tag<const std::int32_t>() == "i", "");
static_assert(mserialize::tag<std::int32_t&>() == "i", "");
static_assert(mserialize::tag<const std::int32_t&>() == "i", "");
static_assert(mserialize::tag<std::tuple<const float, double&, const char&>>() == "(fdc)", "");

// test CustomTag extension point

struct Custom {};

namespace mserialize {

template <>
struct CustomTag<Custom>
{
  static constexpr auto tag_string()
  {
    return make_cx_string("{Custom}");
  }
};

} // namespace mserialize

static_assert(mserialize::tag<Custom>() == "{Custom}", "");

// test MSERIALIZE_MAKE_ENUM_TAG

MSERIALIZE_MAKE_ENUM_TAG(test::CEnum, Alpha, Bravo, Charlie)
static_assert(mserialize::tag<test::CEnum>() == "/I`test::CEnum'0`Alpha'1`Bravo'2`Charlie'\\", "");

MSERIALIZE_MAKE_ENUM_TAG(test::EnumClass, Delta, Echo, Foxtrot)
static_assert(mserialize::tag<test::EnumClass>() == "/i`test::EnumClass'0`Delta'1`Echo'2`Foxtrot'\\", "");

MSERIALIZE_MAKE_ENUM_TAG(test::LargeEnumClass, Golf, Hotel, India, Juliet, Kilo)
static_assert(mserialize::tag<test::LargeEnumClass>() == "/l`test::LargeEnumClass'-8000000000000000`Golf'-400`Hotel'0`India'800`Juliet'7FFFFFFFFFFFFFFF`Kilo'\\", "");

MSERIALIZE_MAKE_ENUM_TAG(test::UnsignedLargeEnumClass, Lima, Mike, November, Oscar)
static_assert(mserialize::tag<test::UnsignedLargeEnumClass>() == "/L`test::UnsignedLargeEnumClass'0`Lima'400`Mike'4000`November'FFFFFFFFFFFFFFFF`Oscar'\\", "");
