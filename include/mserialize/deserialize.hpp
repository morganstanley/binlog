#ifndef MSERIALIZE_DESERIALIZE_HPP
#define MSERIALIZE_DESERIALIZE_HPP

#include <mserialize/detail/Deserializer.hpp>

namespace mserialize {

/**
 * Deserialize a `T` object from `istream` and put it into `out`.
 *
 * @requires `T` to be a deserializable type, including:
 *  - Arithmetic types (std::is_arithmetic)
 *  - Enums (std::is_enum)
 *  - Containers (begin/end, deserializable value_type)
 *  - Tuples (std::pair, std::tuple, is_tuple)
 *  - Smart pointers (std::shared_ptr, std::unique_ptr)
 *
 * @requires `InputStream` to model the following concept:
 *
 * template <typename InpStr>
 * concept InputStream = requires(InpStr istream, char* buf, std::streamsize size)
 * {
 *   // Consume `size` bytes from the stream and copy them to the `buffer`.
 *   // Throw std::exception on failure (i.e: not enough bytes available)
 *   { istream.read(buf, size) } -> InpStr&;
 * };
 *
 * Please note, InputStream is *required* to throw on read failure.
 * Therefore, if std::istream is used, the exception mask must be set accordingly:
 * istream.exceptions(std::ios_base::failbit);
 *
 * @pre `istream` must contain a serialized object, compatible with `T`
 *   at the current read position.
 *
 * @throws std::exception if the deserialization fails.
 *   On exception, `out` is left in an unspecified, but destructible state.
 */
template <typename T, typename InputStream>
void deserialize(T& out, InputStream& istream)
{
  detail::Deserializer<T>::type::deserialize(out, istream);
}

} // namespace mserialize

#endif // MSERIALIZE_DESERIALIZE_HPP
