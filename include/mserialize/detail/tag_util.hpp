#ifndef MSERIALIZE_DETAIL_TAG_UTIL_HPP
#define MSERIALIZE_DETAIL_TAG_UTIL_HPP

#include <mserialize/string_view.hpp>

namespace mserialize {
namespace detail {

/**
 * @returns the number of characters in `s` between
 * the balanced separators `open` and `close`, inclusive.
 *
 * If the separators are not balanced, returns `s.size()`.
 *
 * @pre s[0] == open
 *
 * Example: size_between_balanced("((foo)(bar))qux", '(', ')') == 12
 */
inline std::size_t size_between_balanced(string_view s, char open, char close)
{
  std::size_t open_count = 1;
  std::size_t i = 1;

  for (; i < s.size(); ++i)
  {
    if (s[i] == open)
    {
      ++open_count;
    }
    else if (s[i] == close)
    {
      --open_count;
      if (open_count == 0) { ++i; break; }
    }
  }

  return i;
}

/** @returns `s.find(c)` if `c` in `s`, else `s.size()` */
inline std::size_t find_pos(string_view s, char c)
{
  std::size_t i = 0;
  for (; i < s.size(); ++i)
  {
    if (s[i] == c) { break; }
  }
  return i;
}

/** Remove and return the part of `s` before `c` */
inline string_view remove_prefix_before(string_view& s, char c)
{
  const std::size_t cpos = find_pos(s, c);
  const string_view prefix(s.data(), cpos);
  s.remove_prefix(cpos);
  return prefix;
}

/** @returns the size of the first tag in `tags` */
inline std::size_t tag_first_size(string_view tags)
{
  if (tags.empty()) { return 0; }

  switch (tags.front())
  {
  case '[':
    tags.remove_prefix(1);
    return 1 + tag_first_size(tags);
  case '(':
    return size_between_balanced(tags, '(', ')');
  case '<':
    return size_between_balanced(tags, '<', '>');
  case '{':
    return size_between_balanced(tags, '{', '}');
  case '/':
    return size_between_balanced(tags, '/', '\\');
  }

  return 1; // assume arithmetic
}

/** Remove and return the first tag in the concatenated `tags` */
inline string_view tag_pop(string_view& tags)
{
  const size_t first_size = tag_first_size(tags);
  string_view first(tags.begin(), first_size);
  tags.remove_prefix(first_size);
  return first;
}

/**
 * Remove and return a label from `tags`.
 *
 * A label is a string, enclosed by
 * ` and '.
 *
 * @return the label string at the beginning of `tags`,
 * without the enclosing markers.
 *
 * @pre tags[0] == '`'
 */
inline string_view tag_pop_label(string_view& tags)
{
  tags.remove_prefix(1); // drop `
  const string_view label(tags.begin(), find_pos(tags, '\''));
  tags.remove_prefix(label.size() + 1);
  return label;
}

/**
 * Remove and return an arithmetic tag from `tags`.
 *
 * @pre tags.empty() || tags[0] is an arithmetic tag
 *
 * @return tags[0] or empty if tags is empty
 */
inline string_view tag_pop_arithmetic(string_view& tags)
{
  if (tags.empty()) { return {}; }
  const string_view arithmetic_tag(tags.data(), 1);
  tags.remove_prefix(1);
  return arithmetic_tag;
}

} // namespace detail
} // namespace mserialize

#endif // MSERIALIZE_DETAIL_TAG_UTIL_HPP
