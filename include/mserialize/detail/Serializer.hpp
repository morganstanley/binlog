#ifndef MSERIALIZE_DETAIL_SERIALIZER_HPP
#define MSERIALIZE_DETAIL_SERIALIZER_HPP

#include <mserialize/detail/type_traits.hpp>

#include <type_traits>

namespace mserialize {
namespace detail {

// Default case: Invalid serializer

template <typename T, typename = void>
struct BuiltinSerializer
{
  template <typename OutputStream>
  static void serialize(const T& /* t */, OutputStream& /* ostream */)
  {
    static_assert(always_false<T>::value, "T is not serializable");
  }
};

// Arithmetic serializer

template <typename Arithmetic>
struct BuiltinSerializer<Arithmetic, enable_spec_if<std::is_arithmetic<Arithmetic>>>
{
  template <typename OutputStream>
  static void serialize(const Arithmetic t, OutputStream& ostream)
  {
    ostream.write(reinterpret_cast<const char*>(&t), sizeof(Arithmetic));
  }
};

template <typename T>
using Serializer = BuiltinSerializer<T>;

} // namespace detail
} // namespace mserialize

#endif // MSERIALIZE_DETAIL_SERIALIZER_HPP
