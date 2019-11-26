#ifndef BINLOG_CONST_CHAR_PTR_IS_STRING_HPP
#define BINLOG_CONST_CHAR_PTR_IS_STRING_HPP

#include <mserialize/cx_string.hpp>
#include <mserialize/serialize.hpp>
#include <mserialize/string_view.hpp>
#include <mserialize/tag.hpp>

#include <cassert>
#include <cstdint>
#include <cstring>

/*
 * By definition, `const char*` is a pointer to a constant character.
 * Unfortunately however, in practice, it is used to point to
 * a null terminated string, and assumed to behave as such.
 * It further complicates the issue that character arrays
 * also decay to pointers to characters.
 *
 * By default, as dictated by the type, mserialize would only
 * serialize the character directly pointed by the `const char*`,
 * if it is not null.
 *
 * To avoid surprising, unconventional behaviour,
 * the specializations below make `const char*` to be
 * treated as a null terminated string, instead of an
 * optional character.
 *
 * To be consistent with how other pointers checked
 * for null (and with some printf implementations
 * which allow NULL as a %s argument), if the
 * `const char*` argument is null, "{null}" will be logged.
 */

namespace mserialize {

template <>
struct CustomSerializer<const char*>
{
  template <typename OutputStream>
  static void serialize(const char* str, OutputStream& ostream)
  {
    mserialize::serialize(view(str), ostream);
  }

  static std::size_t serialized_size(const char* str)
  {
    return mserialize::serialized_size(view(str));
  }

private:
  static string_view view(const char* str)
  {
    return string_view(str != nullptr ? str : "{null}");
  }
};

template <>
struct CustomTag<const char*> : detail::Tag<string_view>::type {};

} // namespace mserialize

#endif // BINLOG_CONST_CHAR_PTR_IS_STRING_HPP
