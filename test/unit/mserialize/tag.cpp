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

} // namespace

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
    return cx_string<8>("{Custom}");
  }
};

} // namespace mserialize

static_assert(mserialize::tag<Custom>() == "{Custom}", "");
