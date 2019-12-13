#ifndef BINLOG_ARRAY_VIEW_HPP
#define BINLOG_ARRAY_VIEW_HPP

#include <cstddef>

namespace binlog {

template <typename T>
class ArrayView
{
public:
  ArrayView(const T* begin, const T* end)
    :_begin(begin),
     _end(end)
  {}

  const T* begin() const { return _begin; }
  const T* end()   const { return _end;   }

private:
  const T* _begin;
  const T* _end;
};

/**
 * Create a loggable container view from a pointer+size pair.
 *
 * Useful if a C style array must be logged, but only
 * a pointer to the first element and its size is available.
 *
 * Example:
 *
 *    void f(int* array, std::size_t size)
 *    {
 *      BINLOG_INFO("Array: {}", binlog::array_view(array, size));
 *    }
 *
 * @pre [array, array+size) must be valid
 */
template <typename T>
ArrayView<T> array_view(const T* array, std::size_t size)
{
  return ArrayView<T>(array, array+size);
}

} // namespace binlog

#endif // BINLOG_ARRAY_VIEW_HPP
