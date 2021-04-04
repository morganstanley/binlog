#ifndef BINLOG_TEST_DOCTEST_DOCTEST_H
#define BINLOG_TEST_DOCTEST_DOCTEST_H

// detail/doctest.h must not be included without the extensions below
// to avoid ODR violation: https://github.com/onqtam/doctest/issues/170

#include <doctest/detail/doctest.h>

#include <sstream>
#include <tuple>
#include <vector>

namespace doctest {

template <typename T>
struct StringMaker<std::tuple<T>>
{
  static String convert(const std::tuple<T>& in)
  {
    std::ostringstream oss;
    oss << "(" << std::get<0>(in) << ")";
    return oss.str().c_str();
  }
};

template <typename T>
struct StringMaker<std::vector<T>>
{
  static String convert(const std::vector<T>& in)
  {
    std::ostringstream oss;

    oss << "[";
    for (auto it = in.begin(); it != in.end();)
    {
      oss << StringMaker<T>::convert(*it++);
      if (it != in.end()) { oss << ", "; }
    }
    oss << "]";

    return oss.str().c_str();
  }
};

} // namespace doctest

#endif // BINLOG_TEST_DOCTEST_DOCTEST_H
