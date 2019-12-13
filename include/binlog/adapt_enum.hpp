#ifndef BINLOG_ADAPT_ENUM_HPP
#define BINLOG_ADAPT_ENUM_HPP

#include <mserialize/make_enum_tag.hpp>

/**
 * BINLOG_ADAPT_ENUM(Enum, enumerators...)
 *
 * Make Enum loggable in a way that in the text
 * output the enumerators will be shown instead
 * of the underlying values.
 *
 * By default, enums are loggable, but in the text
 * output, only their underlying value is shown.
 * Adapting an enum doesn't make logging any slower:
 * still the underlying value will be logged,
 * but during reading, it will be converted to
 * the matching enumerator string.
 *
 * Example:
 *
 *     enum class Flag { A, B, C};
 *     BINLOG_ADAPT_ENUM(Flag, A, B, C)
 *
 * The macro has to be called in global scope
 * (outside of any namespace).
 * Works with regular and scoped enums (enum class).
 * The list of enumerators can be empty.
 * The maximum number of enumerators is limited to 100.
 * If an enumerator is omitted from the macro call,
 * and it is logged, the hex representation of
 * the underlying value will be shown.
 *
 * @see MSERIALIZE_MAKE_ENUM_TAG
 */
#define BINLOG_ADAPT_ENUM MSERIALIZE_MAKE_ENUM_TAG

#endif // BINLOG_ADAPT_ENUM_HPP
