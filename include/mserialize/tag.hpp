#ifndef MSERIALIZE_TAG_HPP
#define MSERIALIZE_TAG_HPP

#include <mserialize/detail/Tag.hpp>

namespace mserialize {

/**
 * Get the type tag of `T`.
 *
 * @requires `T` to have a type tag, which includes:
 *  - Arithmetic types (std::is_arithmetic)
 *  - Sequence containers (begin/end, tagged value_type)
 *  - Tuples (std::pair, std::tuple, is_tuple)
 *  - Pointers (T*, std::shared_ptr, std::unique_ptr)
 *
 * Type tags can be used to identify the type
 * of serialized objects. Type A and type B
 * are compatible, if tag(A) == tag(B).
 * Compatible types are de/serializable into eachother,
 * i.e: a serialized A can be deserialized into an
 * object of B.
 *
 * @returns a mserialize::cx_string<N> which describes `T`.
 */
template <typename T>
constexpr auto tag()
{
  return detail::Tag<T>::type::tag_string();
}

} // namespace mserialize

#endif // MSERIALIZE_TAG_HPP
