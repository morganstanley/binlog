#include <mserialize/detail/tag_util.hpp>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(MserializeTagUtil)

BOOST_AUTO_TEST_CASE(resolve_recursive_tag)
{
  using mserialize::detail::resolve_recursive_tag;
  BOOST_TEST(resolve_recursive_tag("", "") == "");
  BOOST_TEST(resolve_recursive_tag("{A}", "{A") == "");
  BOOST_TEST(resolve_recursive_tag("{A`f'i}", "{A") == "`f'i");
  BOOST_TEST(resolve_recursive_tag("(ii{A`f'i}II)", "{A") == "`f'i");
  BOOST_TEST(resolve_recursive_tag("{A`n'<0{A}>}", "{A") == "`n'<0{A}>");
  BOOST_TEST(resolve_recursive_tag("{AA`f'{A}}", "{A") == "");
}

BOOST_AUTO_TEST_SUITE_END()
