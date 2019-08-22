#ifndef MSERIALIZE_DETAIL_INTEGER_TO_HEX_HPP
#define MSERIALIZE_DETAIL_INTEGER_TO_HEX_HPP

#include <mserialize/cx_string.hpp>

#include <cstdint>
#include <iterator> // end

namespace mserialize {
namespace detail {

template <typename Integer>
constexpr std::size_t hex_string_size(Integer v)
{
  std::size_t size = 1;
  if (v >= 0)
  {
    for (; v >= 16; v /= 16) { ++size; }
  }
  else
  {
    for (; std::int64_t(v) < 0; v /= 16) { ++size; }
  }
  return size;
}

template <typename Integer>
constexpr char* write_integer_as_hex(Integer v, char* end)
{
  const char digits[] = "0123456789ABCDEF";

  if (v == 0)
  {
    *--end = '0';
  }
  else if (v > 0)
  {
    while (v != 0)
    {
      *--end = digits[v % 16];
      v /= 16;
    }
  }
  else
  {
    while (v != 0)
    {
      *--end = digits[-(v % 16)];
      v /= 16;
    }
    *--end = '-';
  }

  return end;
}

template <typename Integer, Integer V>
constexpr cx_string<hex_string_size(V)> integer_to_hex()
{
  char buffer[hex_string_size(V)] = {0};
  write_integer_as_hex(V, std::end(buffer));
  return cx_string<sizeof(buffer)>(buffer);
}

} // namespace detail
} // namespace mserialize

#endif // MSERIALIZE_DETAIL_INTEGER_TO_HEX_HPP
