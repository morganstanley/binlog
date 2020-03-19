#ifndef BINLOG_CHAR_PTR_IS_STRING_HPP
#define BINLOG_CHAR_PTR_IS_STRING_HPP

#include <mserialize/cx_string.hpp>
#include <mserialize/serialize.hpp>
#include <mserialize/string_view.hpp>
#include <mserialize/tag.hpp>

#include <cassert>
#include <cstdint>
#include <cstring>

/*
 * By definition, `char*` is a pointer to a character.
 * Unfortunately however, in practice, sometimes it is used to point to
 * a null terminated string, and assumed to behave as such.
 * It further complicates the issue that character arrays
 * also decay to pointers to characters.
 *
 * By default, as dictated by the type, mserialize would only
 * serialize the character directly pointed by the `char*`,
 * if it is not null.
 *
 * This is a safe choice, but might be also surprising,
 * e.g: if strerror(errno) is being logged, as it returns
 * a char* (not a const char*).
 *
 * With the specializations below, Binlog will treat `char*`
 * as a null terminated string, instead of an optional character.
 * This can be convenient, but also dangerous,
 * e.g: if `char x[3] = {1,2,3}` is being logged.
 * By including this file, the user accepts the risk,
 * and trades safety for convenience.
 *
 * To be consistent with how other pointers checked
 * for null (and with some printf implementations
 * which allow NULL as a %s argument), if the
 * `char*` argument is null, "{null}" will be logged.
 */

namespace mserialize {

template <>
struct CustomSerializer<char*> : CustomSerializer<const char*> {};

template <>
struct CustomTag<char*> : detail::Tag<string_view>::type {};

} // namespace mserialize

#endif // BINLOG_CHAR_PTR_IS_STRING_HPP
