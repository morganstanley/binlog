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
    #if defined(__GNUC__) and not defined(__clang__)
      // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=96742
      #pragma GCC diagnostic push
      #pragma GCC diagnostic ignored "-Wtype-limits"
    #endif
    for (; std::int64_t(v) < 0; v /= 16) { ++size; }
    #if defined(__GNUC__) and not defined(__clang__)
      #pragma GCC diagnostic pop
    #endif
  }
  return size;
}

#ifdef _MSC_VER
  // "unary minus operator applied to unsigned type, result still unsigned"
  // The referenced line will be never reached if Integer is unsigned.
  #pragma warning(push)
  #pragma warning(disable : 4146)
#endif

// static_cast below: writing v /= 16 triggers GCC ubsan error:
// error: conversion from 'int' to 'unsigned char' may change value [-Werror=conversion]

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
      v = static_cast<Integer>(v / 16);
    }
  }
  else
  {
    while (v != 0)
    {
      *--end = digits[-(v % 16)];
      v = static_cast<Integer>(v / 16);
    }
    *--end = '-';
  }

  return end;
}

#ifdef _MSC_VER
  #pragma warning(pop)
#endif

constexpr char* write_integer_as_hex(bool v, char* end)
{
  *--end = v ? '1' : '0';
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
