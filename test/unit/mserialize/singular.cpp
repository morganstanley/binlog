#include <mserialize/singular.hpp>

#include <doctest/doctest.h>

namespace {

bool non_recursive_singular(mserialize::string_view tag)
{
  return mserialize::singular(tag, tag);
}

} // namespace

TEST_CASE("singular")
{
  CHECK(non_recursive_singular("()") == true);
  CHECK(non_recursive_singular("(())") == true);
  CHECK(non_recursive_singular("((()))") == true);

  CHECK(non_recursive_singular("{Empty}") == true);
  CHECK(non_recursive_singular("{Empty`x'()}") == true);
  CHECK(non_recursive_singular("{Empty`x'(())}") == true);
  CHECK(non_recursive_singular("{Empty`x'{Nil}}") == true);
  CHECK(non_recursive_singular("{Empty`x'{Nil}`y'{Nul}}") == true);

}

TEST_CASE("not_singular")
{
  CHECK(non_recursive_singular("y") == false);
  CHECK(non_recursive_singular("c") == false);

  CHECK(non_recursive_singular("b") == false);
  CHECK(non_recursive_singular("s") == false);
  CHECK(non_recursive_singular("i") == false);
  CHECK(non_recursive_singular("l") == false);

  CHECK(non_recursive_singular("B") == false);
  CHECK(non_recursive_singular("S") == false);
  CHECK(non_recursive_singular("I") == false);
  CHECK(non_recursive_singular("L") == false);

  CHECK(non_recursive_singular("f") == false);
  CHECK(non_recursive_singular("d") == false);
  CHECK(non_recursive_singular("D") == false);

  CHECK(non_recursive_singular("[i") == false);
  CHECK(non_recursive_singular("[f") == false);
  CHECK(non_recursive_singular("[y") == false);

  CHECK(non_recursive_singular("(i)") == false);

  CHECK(non_recursive_singular("<>") == false);
  CHECK(non_recursive_singular("<i>") == false);
  CHECK(non_recursive_singular("<()>") == false);

  CHECK(non_recursive_singular("/i`E'\\") == false);

  CHECK(non_recursive_singular("{List`n'<0List>}") == false);
}

TEST_CASE("recursive")
{
  CHECK(mserialize::singular("{Empty`x'{Empty}}", "{Empty}") == false); // infinite recursion
  CHECK(mserialize::singular("{R`r'[{R}}", "{R}") == false);
}

TEST_CASE("corrupt_but_singular")
{
  CHECK(mserialize::singular("[{1t0", "{1t") == true);
  CHECK(mserialize::singular("(", "(") == true);
}
