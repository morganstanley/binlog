#ifndef MSERIALIZE_SERIALIZE_HPP
#define MSERIALIZE_SERIALIZE_HPP

#include <mserialize/detail/Serializer.hpp>

namespace mserialize {

/**
 * Serialize `in` to `ostream`.
 *
 * @requires `T` to be a serializable type, including:
 *  - Arithmetic types (std::is_arithmetic)
 *  - Sequence containers (begin/end, serializable value_type)
 *  - Tuples (std::pair, std::tuple, is_tuple)
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

} // namespace mserialize

#endif // MSERIALIZE_SERIALIZE_HPP
