#ifndef MSERIALIZE_VISIT_HPP
#define MSERIALIZE_VISIT_HPP

#include <mserialize/detail/Visit.hpp>

#include <mserialize/cx_string.hpp>
#include <mserialize/string_view.hpp>

namespace mserialize {

/**
 * Visit the serialized objects in `istream`.
 *
 * @requires `visitor` to model the Visitor concept, see Vistor.hpp
 * @requires `istream` must model the mserialize::InputStream concept.
 * @pre `tag` must be a valid type tag, e.g:
 *   a tag returned by mserialize::tag<T>().
 * @pre `istream` must contain a serialized object,
 *   whose type tag is `tag`.
 * @throws std::exception if reading of `istream` fails.
 * @throws std::runtime_error if tag is too deeply nested,
 *   to prevent stack overflow.
 * @throws std::runtime_error if tag is syntactically invalid
 */
template <typename Visitor, typename InputStream>
void visit(string_view tag, Visitor& visitor, InputStream& istream)
{
  detail::visit_impl(tag, tag, visitor, istream, 2048);
}

} // namespace mserialize

#endif // MSERIALIZE_VISIT_HPP
