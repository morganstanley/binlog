#ifndef MSERIALIZE_DETAIL_SERIALIZER_HPP
#define MSERIALIZE_DETAIL_SERIALIZER_HPP

#include <mserialize/detail/type_traits.hpp>

#include <type_traits>

namespace mserialize {
namespace detail {

// Invalid serializer

struct InvalidSerializer
{
  template <typename T, typename OutputStream>
  static void serialize(const T& /* t */, OutputStream& /* ostream */)
  {
    static_assert(always_false<T>::value, "T is not serializable");
  }
};

// Forward declarations

template <typename Arithmetic>
struct ArithmeticSerializer;

// Builtin serializer - one specialization for each category

template <typename T, typename = void>
struct BuiltinSerializer : InvalidSerializer {};

template <typename T>
struct BuiltinSerializer<T, enable_spec_if<std::is_arithmetic<T>>>
  : ArithmeticSerializer<T> {};

// Serializer - entry point

template <typename T>
struct  Serializer
{
  using type = BuiltinSerializer<T>;
};

// Arithmetic serializer

template <typename Arithmetic>
struct ArithmeticSerializer
{
  template <typename OutputStream>
  static void serialize(const Arithmetic t, OutputStream& ostream)
  {
    ostream.write(reinterpret_cast<const char*>(&t), sizeof(Arithmetic));
  }
};

} // namespace detail
} // namespace mserialize

#endif // MSERIALIZE_DETAIL_SERIALIZER_HPP
