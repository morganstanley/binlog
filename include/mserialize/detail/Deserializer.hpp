#ifndef MSERIALIZE_DETAIL_DESERIALIZER_HPP
#define MSERIALIZE_DETAIL_DESERIALIZER_HPP

#include <mserialize/detail/type_traits.hpp>

#include <type_traits>

namespace mserialize {
namespace detail {

// Invalid deserializer

struct InvalidDeserializer
{
  template <typename T, typename InputStream>
  static void serialize(const T& /* t */, InputStream& /* istream */)
  {
    static_assert(always_false<T>::value, "T is not deserializable");
  }
};

// Forward declarations

template <typename Arithmetic>
struct ArithmeticDeserializer;

// Builtin deserializer - one specialization for each category

template <typename T, typename = void>
struct BuiltinDeserializer : InvalidDeserializer {};

template <typename T>
struct BuiltinDeserializer<T, enable_spec_if<std::is_arithmetic<T>>>
  : ArithmeticDeserializer<T> {};

// Deserializer - entry point

template <typename T>
struct Deserializer
{
  using type = BuiltinDeserializer<T>;
};

// Arithmetic deserializer

template <typename Arithmetic>
struct ArithmeticDeserializer
{
  template <typename InputStream>
  static void deserialize(Arithmetic& t, InputStream& istream)
  {
    istream.read(reinterpret_cast<char*>(&t), sizeof(Arithmetic));
  }
};

} // namespace detail
} // namespace mserialize

#endif // MSERIALIZE_DETAIL_DESERIALIZER_HPP
