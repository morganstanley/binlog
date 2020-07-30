#include <mserialize/singular.hpp>

#include <boost/test/unit_test.hpp>

namespace {

bool non_recursive_singular(mserialize::string_view tag)
{
  return mserialize::singular(tag, tag);
}

} // namespace

BOOST_AUTO_TEST_SUITE(MserializeSingular)

BOOST_AUTO_TEST_CASE(singular)
{
  BOOST_TEST(non_recursive_singular("()") == true);
  BOOST_TEST(non_recursive_singular("(())") == true);
  BOOST_TEST(non_recursive_singular("((()))") == true);

  BOOST_TEST(non_recursive_singular("{Empty}") == true);
  BOOST_TEST(non_recursive_singular("{Empty`x'()}") == true);
  BOOST_TEST(non_recursive_singular("{Empty`x'(())}") == true);
  BOOST_TEST(non_recursive_singular("{Empty`x'{Nil}}") == true);
  BOOST_TEST(non_recursive_singular("{Empty`x'{Nil}`y'{Nul}}") == true);

}

BOOST_AUTO_TEST_CASE(not_singular)
{
  BOOST_TEST(non_recursive_singular("y") == false);
  BOOST_TEST(non_recursive_singular("c") == false);

  BOOST_TEST(non_recursive_singular("b") == false);
  BOOST_TEST(non_recursive_singular("s") == false);
  BOOST_TEST(non_recursive_singular("i") == false);
  BOOST_TEST(non_recursive_singular("l") == false);

  BOOST_TEST(non_recursive_singular("B") == false);
  BOOST_TEST(non_recursive_singular("S") == false);
  BOOST_TEST(non_recursive_singular("I") == false);
  BOOST_TEST(non_recursive_singular("L") == false);

  BOOST_TEST(non_recursive_singular("f") == false);
  BOOST_TEST(non_recursive_singular("d") == false);
  BOOST_TEST(non_recursive_singular("D") == false);

  BOOST_TEST(non_recursive_singular("[i") == false);
  BOOST_TEST(non_recursive_singular("[f") == false);
  BOOST_TEST(non_recursive_singular("[y") == false);

  BOOST_TEST(non_recursive_singular("(i)") == false);

  BOOST_TEST(non_recursive_singular("<>") == false);
  BOOST_TEST(non_recursive_singular("<i>") == false);
  BOOST_TEST(non_recursive_singular("<()>") == false);

  BOOST_TEST(non_recursive_singular("/i`E'\\") == false);

  BOOST_TEST(non_recursive_singular("{List`n'<0List>}") == false);
}

BOOST_AUTO_TEST_CASE(recursive)
{
  BOOST_TEST(mserialize::singular("{Empty`x'{Empty}}", "{Empty}") == false); // infinite recursion
  BOOST_TEST(mserialize::singular("{R,`r'[{R}}", "{R}") == false);
}

BOOST_AUTO_TEST_CASE(corrupt_but_singular)
{
  BOOST_TEST(mserialize::singular("[{1t0", "{1t") == true);
  BOOST_TEST(mserialize::singular("(", "(") == true);
}

BOOST_AUTO_TEST_SUITE_END()
