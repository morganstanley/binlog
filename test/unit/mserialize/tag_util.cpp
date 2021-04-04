#include <mserialize/detail/tag_util.hpp>

#include <doctest/doctest.h>

#include <vector>

TEST_CASE("tag_first_size_large_input")
{
  std::vector<char> buffer(1'000'001, '[');
  buffer.back() = 'i';

  const mserialize::string_view tag(buffer.data(), buffer.size());
  CHECK(buffer.size() == mserialize::detail::tag_first_size(tag));
}

TEST_CASE("resolve_recursive_tag")
{
  using mserialize::detail::resolve_recursive_tag;
  CHECK(resolve_recursive_tag("", "") == "");
  CHECK(resolve_recursive_tag("{", "") == "");
  CHECK(resolve_recursive_tag("{A}", "{A") == "");
  CHECK(resolve_recursive_tag("{A`f'i}", "{A") == "`f'i");
  CHECK(resolve_recursive_tag("(ii{A`f'i}II)", "{A") == "`f'i");
  CHECK(resolve_recursive_tag("{A`n'<0{A}>}", "{A") == "`n'<0{A}>");
  CHECK(resolve_recursive_tag("{AA`f'{A}}", "{A") == "");
}
