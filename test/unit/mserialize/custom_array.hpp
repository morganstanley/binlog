#ifndef TEST_UNIT_MSERIALIZE_CUSTOM_ARRAY_HPP
#define TEST_UNIT_MSERIALIZE_CUSTOM_ARRAY_HPP

#include <algorithm> // copy
#include <initializer_list>
#include <iterator>
#include <ostream>
#include <stdexcept>

namespace test {

template <typename T, size_t N>
struct CustomArray
{
  T array[N];

  CustomArray() = default;

  CustomArray(std::initializer_list<T> il)
  {
    if (il.size() != N) { throw std::runtime_error("init list size does not match array size"); }
    std::copy(il.begin(), il.end(), std::begin(array));
  }

  bool operator==(const CustomArray& rhs) const
  {
    return std::equal(std::begin(array), std::end(array), std::begin(rhs.array));
  }
};

template <typename T, size_t N>
T* begin(CustomArray<T,N>& a) { return std::begin(a.array); }

template <typename T, size_t N>
T* end(CustomArray<T,N>& a) { return std::end(a.array); }

template <typename T, size_t N>
const T* begin(const CustomArray<T,N>& a) { return std::begin(a.array); }

template <typename T, size_t N>
const T* end(const CustomArray<T,N>& a) { return std::end(a.array); }

template <typename T, size_t N>
std::ostream& operator<<(std::ostream& out, const CustomArray<T,N>& a)
{
  out << '[';
  for (const T& t : a) { out << t << ','; }
  return out << ']';
}

} // namespace test

#endif // TEST_UNIT_MSERIALIZE_CUSTOM_ARRAY_HPP
