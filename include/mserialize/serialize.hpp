#ifndef MSERIALIZE_SERIALIZE_HPP
#define MSERIALIZE_SERIALIZE_HPP

#include <mserialize/detail/Serializer.hpp>

#include <cstddef> // size_t

namespace mserialize {

/**
 * Serialize `in` to `ostream`.
 *
 * @requires `T` to be a serializable type, including:
 *  - Arithmetic types (std::is_arithmetic)
 *  - Enums (std::is_enum)
 *  - Containers (begin/end, serializable value_type)
 *  - Tuples (std::pair, std::tuple, is_tuple)
 *  - Pointers (T*, std::shared_ptr, std::unique_ptr)
 *
 * @requires `OutputStream` to model the following concept:
 *
 * template <typename OutStr>
 * concept OutputStream = requires(OutStr ostream, const char* buf, std::streamsize size)
 * {
 *   // Append `size` bytes from `buffer` to the stream
 *   { ostream.write(buf, size) } -> OutStr&;
 * }
 *
 * @throws exception `ostream` throws while writing.
 */
template <typename T, typename OutputStream>
void serialize(const T& in, OutputStream& ostream)
{
  detail::Serializer<T>::type::serialize(in, ostream);
}

/**
 * Get the serialized size of `in`.
 *
 * @requires `T` to be a serializable type
 * @returns The number of bytes `in` would occupy when serialized.
 */
template <typename T>
std::size_t serialized_size(const T& in)
{
  return detail::Serializer<T>::type::serialized_size(in);
}

} // namespace mserialize

#endif // MSERIALIZE_SERIALIZE_HPP
