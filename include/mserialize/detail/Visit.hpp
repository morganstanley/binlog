#ifndef MSERIALIZE_DETAIL_VISIT_HPP
#define MSERIALIZE_DETAIL_VISIT_HPP

#include <mserialize/Visitor.hpp>
#include <mserialize/deserialize.hpp>
#include <mserialize/detail/integer_to_hex.hpp>
#include <mserialize/detail/tag_util.hpp>

#include <type_traits>

namespace mserialize {
namespace detail {

/** Convert a single visited integer to a hex string */
class IntegerToHex // NOLINT(cppcoreguidelines-pro-type-member-init)
{
  char _buffer[20];
  char* _p = &_buffer[19];

public:
  template <typename Integer>
  std::enable_if_t<std::is_integral<Integer>::value>
  visit(Integer v)
  {
    _p = write_integer_as_hex(v, _p) - 1;
  }

  template <typename T>
  std::enable_if_t<!std::is_integral<T>::value>
  visit(T) {}

  string_view value() const
  {
    return string_view(_p+1, std::size_t(_buffer + 18 - _p));
  }

  string_view delimited_value(char prefix, char postfix)
  {
    _buffer[19] = postfix;
    *_p = prefix;
    return string_view(_p, std::size_t(_buffer + 20 - _p));
  }
};

// forward declaration
template <typename Visitor, typename InputStream>
void visit_impl(string_view full_tag, string_view tag, Visitor& visitor, InputStream& istream);

/** @pre tag in "ycbsilBSILfdD" */
template <typename Visitor, typename InputStream>
void visit_arithmetic(char tag, Visitor& visitor, InputStream& istream)
{
  switch(tag)
  {
  case 'y': bool y; mserialize::deserialize(y, istream); visitor.visit(y); break;
  case 'c': char c; mserialize::deserialize(c, istream); visitor.visit(c); break;

  case 'b': std::int8_t  b; mserialize::deserialize(b, istream); visitor.visit(b); break;
  case 's': std::int16_t s; mserialize::deserialize(s, istream); visitor.visit(s); break;
  case 'i': std::int32_t i; mserialize::deserialize(i, istream); visitor.visit(i); break;
  case 'l': std::int64_t l; mserialize::deserialize(l, istream); visitor.visit(l); break;

  case 'B': std::uint8_t  B; mserialize::deserialize(B, istream); visitor.visit(B); break;
  case 'S': std::uint16_t S; mserialize::deserialize(S, istream); visitor.visit(S); break;
  case 'I': std::uint32_t I; mserialize::deserialize(I, istream); visitor.visit(I); break;
  case 'L': std::uint64_t L; mserialize::deserialize(L, istream); visitor.visit(L); break;

  case 'f': float f;       mserialize::deserialize(f, istream); visitor.visit(f); break;
  case 'd': double d;      mserialize::deserialize(d, istream); visitor.visit(d); break;
  case 'D': long double D; mserialize::deserialize(D, istream); visitor.visit(D); break;
  }
}

template <typename Visitor, typename InputStream>
void visit_sequence_impl(const string_view full_tag, const string_view elem_tag, std::uint32_t size, Visitor& visitor, InputStream& istream, char)
{
  visitor.visit(mserialize::Visitor::SequenceBegin{size, elem_tag});

  while (size--)
  {
    visit_impl(full_tag, elem_tag, visitor, istream);
  }

  visitor.visit(mserialize::Visitor::SequenceEnd{});
}

template <typename Visitor, typename InputStream>
void_t<decltype(&InputStream::view)>
visit_sequence_impl(const string_view full_tag, const string_view elem_tag, std::uint32_t size, Visitor& visitor, InputStream& istream, int)
{
  if (elem_tag.size() == 1 && elem_tag[0] == 'c')
  {
    string_view data(istream.view(size), size);
    visitor.visit(mserialize::Visitor::String{data});
  }
  else
  {
    visit_sequence_impl(full_tag, elem_tag, size, visitor, istream, '\0');
  }
}

/** @pre tag.front() == '[' , followed by a single tag */
template <typename Visitor, typename InputStream>
void visit_sequence(const string_view full_tag, string_view tag, Visitor& visitor, InputStream& istream)
{
  tag.remove_prefix(1); // drop [

  std::uint32_t size;
  mserialize::deserialize(size, istream);
  const string_view elem_tag = tag_pop(tag);

  visit_sequence_impl(full_tag, elem_tag, size, visitor, istream, 0);
}

/** @pre tag.front() == '(' and tag.back() == ')' , enclosing any number of concatenated tags */
template <typename Visitor, typename InputStream>
void visit_tuple(const string_view full_tag, string_view tag, Visitor& visitor, InputStream& istream)
{
  tag.remove_prefix(1); // drop (
  tag.remove_suffix(1); // drop )

  visitor.visit(mserialize::Visitor::TupleBegin{tag});

  for (string_view elem_tag = tag_pop(tag); ! elem_tag.empty(); elem_tag = tag_pop(tag))
  {
    visit_impl(full_tag, elem_tag, visitor, istream);
  }

  visitor.visit(mserialize::Visitor::TupleEnd{});
}

/** @pre tag.front() == '<' and tag.back() == '>' , enclosing 1-255 concatenated tags */
template <typename Visitor, typename InputStream>
void visit_variant(const string_view full_tag, string_view tag, Visitor& visitor, InputStream& istream)
{
  tag.remove_prefix(1); // drop <
  tag.remove_suffix(1); // drop >

  std::uint8_t discriminator;
  mserialize::deserialize(discriminator, istream);

  std::uint8_t preceding_tag_count = discriminator;
  while (preceding_tag_count--) { tag_pop(tag); }

  const string_view option_tag = tag_pop(tag);

  visitor.visit(mserialize::Visitor::VariantBegin{discriminator, option_tag});

  visit_impl(full_tag, option_tag, visitor, istream);

  visitor.visit(mserialize::Visitor::VariantEnd{});
}

/**
 * tag describes the name of the structure, and the name and type tag of its fields, e.g:
 * {Person`name'[c`age'i}
 * Empty structures are allowed: {Empty}
 * In case of recursive structures, only the first occurrence must describe the
 * fields, subsequent occurrences can look look like empty structs, e.g:
 * {Node`value'i`next'<0{Node}>}
 *
 * @pre tag.front() == '{' and tag.back() == '}'
 * @pre the described structure must not be infinitely recursive
 */
template <typename Visitor, typename InputStream>
void visit_struct(const string_view full_tag, string_view tag, Visitor& visitor, InputStream& istream)
{
  tag.remove_suffix(1); // drop }

  string_view intro = remove_prefix_before(tag, '`');

  if (tag.empty())
  {
    // perhaps a recursive struct?
    const std::size_t intro_pos = full_tag.find(intro);
    tag = string_view(full_tag.data() + intro_pos, full_tag.size() - intro_pos);
    tag = tag_pop(tag);
    tag.remove_prefix(intro.size());
    tag.remove_suffix(1);
  }

  intro.remove_prefix(1); // drop {

  visitor.visit(mserialize::Visitor::StructBegin{intro, tag});

  while (! tag.empty())
  {
    const string_view field_name = tag_pop_label(tag);
    const string_view field_tag = tag_pop(tag);

    visitor.visit(mserialize::Visitor::FieldBegin{field_name, field_tag});

    visit_impl(full_tag, field_tag, visitor, istream);

    visitor.visit(mserialize::Visitor::FieldEnd{});
  }

  visitor.visit(mserialize::Visitor::StructEnd{});
}

/**
 * The enum tag describes the enum name, the underlying type,
 * and the possible enumerator names and values, in hex, e.g:
 * /i`Flag'0`No'1A`Yes'\
 *
 * @pre tag.front() == / and tag.back() == \
 * @pre the underlying type must be an integer tag
 */
template <typename Visitor, typename InputStream>
void visit_enum(string_view tag, Visitor& visitor, InputStream& istream)
{
  tag.remove_prefix(1); // drop slash
  tag.remove_suffix(1); // drop backslash

  if (tag.empty()) { return; }

  // convert the discriminator to hex
  const char underlying_type_tag = tag[0];
  IntegerToHex hex;
  visit_arithmetic(underlying_type_tag, hex, istream);

  tag.remove_prefix(2); // drop underlying_type_tag and `
  const string_view name = remove_prefix_before(tag, '\'');

  string_view enumerator;

  const string_view dvalue = hex.delimited_value('\'', '`');
  const std::size_t value_pos = tag.find(dvalue);

  if (value_pos != string_view::npos)
  {
    tag.remove_prefix(value_pos + dvalue.size() - 1);
    enumerator = tag_pop_label(tag);
  }

  visitor.visit(mserialize::Visitor::Enum{name, enumerator, underlying_type_tag, hex.value()});
}

/**
 * Visit `tag` by (recursively) decomposing it into primitive elements.
 *
 * `full_tag` is needed because of recursive structures, where the
 * original definition of the structure has to be referenced.
 *
 * @pre tag is a substring of full_tag
 */
template <typename Visitor, typename InputStream>
void visit_impl(const string_view full_tag, string_view tag, Visitor& visitor, InputStream& istream)
{
  if (tag.empty()) { return; }

  switch (tag.front())
  {
  case '[':
    visit_sequence(full_tag, tag, visitor, istream);
    break;
  case '(':
    visit_tuple(full_tag, tag, visitor, istream);
    break;
  case '<':
    visit_variant(full_tag, tag, visitor, istream);
    break;
  case '{':
    visit_struct(full_tag, tag, visitor, istream);
    break;
  case '/':
    visit_enum(tag, visitor, istream);
    break;
  case '0':
    visitor.visit(mserialize::Visitor::Null{});
    break;
  default:
    visit_arithmetic(tag.front(), visitor, istream);
  }
}

} // namespace detail
} // namespace mserialize

#endif // MSERIALIZE_DETAIL_VISIT_HPP
