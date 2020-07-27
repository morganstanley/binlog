#ifndef MSERIALIZE_SINGULAR_HPP
#define MSERIALIZE_SINGULAR_HPP

#include <mserialize/string_view.hpp>

#include <mserialize/detail/Singular.hpp>

namespace mserialize {

/**
 * A tag of type T is singular, if objects of type T
 * have only a single valid value.
 *
 * Objects of singular types are always serialized
 * using 0 bytes.
 *
 * `full_tag` is needed to tell apart empty structs
 * and references of recursive structs.
 * Recursive structs are always considered non-singular.
 * While the infinitely recursive "{E`e'{E}}" could be considered
 * to be singular, it cannot be serialized or visited,
 * therefore it doesn't matter.
 *
 * @pre `tag` must be a valid type tag, e.g:
 *   a tag returned by mserialize::tag<T>().
 * @throws std::runtime_error if tag is too deeply nested,
 *   to prevent stack overflow.
 * @return true if `tag` is singular
 */
inline bool singular(string_view full_tag, string_view tag, int max_recursion = 2048)
{
  return detail::singular_impl(full_tag, tag, max_recursion);
}

} // namespace mserialize

#endif // MSERIALIZE_SINGULAR_HPP
