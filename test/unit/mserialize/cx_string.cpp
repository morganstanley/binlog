#include <mserialize/cx_string.hpp>

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

constexpr auto empty()     { return mserialize::cx_string<0>(""); }
constexpr auto foo()       { return mserialize::cx_string<3>("foo"); }
constexpr auto bar()       { return mserialize::cx_string<3>("bar"); }
constexpr auto foobar()    { return mserialize::cx_string<6>("foobar"); }
constexpr auto foobarbaz() { return mserialize::cx_string<9>("foobarbaz"); }
constexpr auto with_null() { return mserialize::cx_string<9>("with\0null"); }

static_assert(mserialize::cx_strcat() == "", "");
static_assert(mserialize::cx_strcat(empty()) == "", "");
static_assert(mserialize::cx_strcat(empty(), empty(), empty()) == "", "");
static_assert(mserialize::cx_strcat(foo()) == "foo", "");
static_assert(mserialize::cx_strcat(foo(), bar()) == "foobar", "");
static_assert(mserialize::cx_strcat(
  foo(), with_null(), bar())
  == "foowith\0nullbar", "" // NOLINT(misc-string-literal-with-embedded-nul)
);
static_assert(mserialize::cx_strcat(
  foo(), bar(), foobar(), foobarbaz(), bar())
  == "foobarfoobarfoobarbazbar", ""
);

} // namespace
