#ifndef BINLOG_CREATE_SOURCE_AND_EVENT_HPP
#define BINLOG_CREATE_SOURCE_AND_EVENT_HPP

#include <binlog/Session.hpp>
#include <binlog/SessionWriter.hpp>
#include <binlog/create_source.hpp>

#include <mserialize/cx_string.hpp>
#include <mserialize/detail/preprocessor.hpp>
#include <mserialize/tag.hpp>

#include <atomic>
#include <cstdint>
#include <type_traits> // integral_constant
#include <utility> // forward

/**
 * BINLOG_CREATE_SOURCE_AND_EVENT(writer, severity, category, clock, format, args...)
 *
 * Add a static event source that describes the callsite (function, file, line),
 * and other static properties (severity, category, format string, argument tags)
 * to the binary compile time.
 * Each time it is called, add an event that references
 * the event source with `clock` and `args...` to `writer`.
 * @see SessionWriter::addEvent.
 *
 * @param writer binlog::SessionWriter
 * @param severity binlog::Severity
 * @param category arbitrary valid symbol name
 * @param clock std::uint64_t clock value, see ClockSync
 * @param format string literal with {} placeholders
 * @param args... any number of serializable, tagged, log arguments. Can be empty
 *
 * The number of arguments must match the number of {} placeholders in `format`.
 */
#define BINLOG_CREATE_SOURCE_AND_EVENT(writer, severity, category, clock, /* format, */ ...) \
  do {                                                                                       \
    static_assert(                                                                           \
      binlog::detail::count_placeholders(MSERIALIZE_FIRST(__VA_ARGS__))+1 ==                 \
      decltype(binlog::detail::count_arguments(__VA_ARGS__))::value,                         \
      "Number of {} placeholders in format string must match number of arugments"            \
    );                                                                                       \
    BINLOG_CREATE_SOURCE(                                                                    \
      _binlog_sid, binlog::severityToConstCharPtr(severity), category,                       \
      MSERIALIZE_FIRST(__VA_ARGS__),                                                         \
      decltype(binlog::detail::concatenated_tags(__VA_ARGS__))::tag.data());                 \
    binlog::detail::addEventIgnoreFirst(writer, _binlog_sid, clock, __VA_ARGS__);            \
  } while (false)                                                                            \
  /**/

namespace binlog {
namespace detail {

// Can't use the return value of cx_strcat directly, because sv.data(),
// a pointer to a subobject of a temporary, cannot appear in a constexpr.
// Instead, save it in a static field of this type.
template <typename... T>
struct ConcatenatedTags
{
  static constexpr auto tag = mserialize::cx_strcat(mserialize::tag<T>()...);
};

/**
 * No temporary of non-literal type allowed in constant expression.
 * Hack it around by avoiding taking reference to any argument,
 * this function should only appear in non-evaluated context.
 * The first argument is dropped because __VA_ARGS__ cannot be empty,
 * therefore it is always combined with something unrelated.
 */
template <typename... T>
ConcatenatedTags<T...> concatenated_tags(const char*, T&&...);

/** @return the number of "{}" substrings in `str` */
constexpr std::size_t count_placeholders(const char* str)
{
  std::size_t result = 0;
  for (std::size_t i = 0; str[i] != 0; ++i)
  {
    if (str[i] == '{' && str[i+1] == '}')
    {
      ++result;
      ++i;
    }
  }

  return result;
}

template <typename... T>
constexpr std::integral_constant<std::size_t, sizeof...(T)>
count_arguments(T&&...) { return {}; } // Implementation should be omitted but cannot be on MSVC

// The first argument is dropped because __VA_ARGS__ cannot be empty,
// therefore it is always combined with something unrelated.
template <typename Writer, typename Unused, typename... T>
void addEventIgnoreFirst(Writer& writer, std::uint64_t eventSourceId, std::uint64_t clock, Unused&&, T&&... t)
{
  writer.addEvent(eventSourceId, clock, std::forward<T>(t)...);
}

} // namespace detail
} // namespace binlog

#endif // BINLOG_CREATE_SOURCE_AND_EVENT_HPP
