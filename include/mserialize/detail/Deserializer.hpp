#ifndef MSERIALIZE_DETAIL_DESERIALIZER_HPP
#define MSERIALIZE_DETAIL_DESERIALIZER_HPP

#include <mserialize/detail/type_traits.hpp>

#include <type_traits>

namespace mserialize {
namespace detail {

// Default case: Invalid deserializer

template <typename T, typename = void>
struct BuiltinDeserializer
{
  template <typename InputStream>
  static void serialize(const T& /* t */, InputStream& /* istream */)
  {
    static_assert(always_false<T>::value, "T is not deserializable");
  }
};

// Arithmetic deserializer

template <typename Arithmetic>
struct BuiltinDeserializer<Arithmetic, enable_spec_if<std::is_arithmetic<Arithmetic>>>
{
  template <typename InputStream>
  static void deserialize(Arithmetic& t, InputStream& istream)
  {
    istream.read(reinterpret_cast<char*>(&t), sizeof(Arithmetic));
  }
};

template <typename T>
using Deserializer = BuiltinDeserializer<T>;

} // namespace detail
} // namespace mserialize

#endif // MSERIALIZE_DETAIL_DESERIALIZER_HPP
