#ifndef MSERIALIZE_DETAIL_SINGULAR_HPP
#define MSERIALIZE_DETAIL_SINGULAR_HPP

#include <mserialize/detail/tag_util.hpp>

namespace mserialize {
namespace detail {

inline bool singular_impl(string_view, string_view, int);

inline bool singular_tuple(string_view full_tag, string_view tag, int max_recursion)
{
  tag.remove_prefix(1); // drop (
  tag.remove_suffix(1); // drop )
  for (string_view elem_tag = tag_pop(tag); ! elem_tag.empty(); elem_tag = tag_pop(tag))
  {
    if (! singular_impl(full_tag, elem_tag, max_recursion)) { return false; }
  }
  return true;
}

inline bool singular_struct(string_view full_tag, string_view tag, int max_recursion)
{
  tag.remove_suffix(1); // drop }

  string_view intro = remove_prefix_before(tag, '`');

  if (tag.empty())
  {
    // perhaps a recursive struct?
    tag = resolve_recursive_tag(full_tag, intro);
    // drop the name of the first field, if any
    tag_pop_label(tag);
    // if tag is not empty at this point, this is a recursive struct: non-singular
    return tag.empty();
  }

  while (! tag.empty())
  {
    tag_pop_label(tag); // field_name
    const string_view field_tag = tag_pop(tag);

    if (! singular_impl(full_tag, field_tag, max_recursion)) { return false; }
  }

  return true;
}

inline bool singular_impl(string_view full_tag, string_view tag, int max_recursion)
{
  if (max_recursion == 0) { throw std::runtime_error("Recursion limit exceeded while visiting tag: " + full_tag.to_string()); }

  if (tag.empty()) { return true; }

  switch (tag.front())
  {
  case '(':
    return singular_tuple(full_tag, tag, max_recursion - 1);
  case '{':
    return singular_struct(full_tag, tag, max_recursion - 1);
  }

  return false;
}

} // namespace detail
} // namespace mserialize

#endif // MSERIALIZE_DETAIL_SINGULAR_HPP
