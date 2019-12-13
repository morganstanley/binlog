#ifndef BINLOG_CREATE_SOURCE_AND_EVENT_HPP
#define BINLOG_CREATE_SOURCE_AND_EVENT_HPP

#include <binlog/Session.hpp>
#include <binlog/SessionWriter.hpp>

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
 * When called for the first time, create an EventSource that describes
 * the call site (function, file, line), and other static properties
 * (severity, category, format string, argument tags) - and add it to the
 * session of `writer`.
 * Furthermore, each time it is called, add an event that references
 * the added event source with `clock` and `args...` to `writer`.
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
 *
 * TODO(benedek) perf: do not instantiate a full EventSource
 */
#define BINLOG_CREATE_SOURCE_AND_EVENT(writer, severity, category, clock, /* format, */ ...) \
  do {                                                                                       \
    static_assert(                                                                           \
      binlog::detail::count_placeholders(MSERIALIZE_FIRST(__VA_ARGS__))+1 ==                 \
      decltype(binlog::detail::count_arguments(__VA_ARGS__))::value,                         \
      "Number of {} placeholders in format string must match number of arugments"            \
    );                                                                                       \
    static std::atomic<std::uint64_t> _binlog_sid{0};                                        \
    std::uint64_t _binlog_sid_v = _binlog_sid.load(std::memory_order_relaxed);               \
    if (_binlog_sid_v == 0)                                                                  \
    {                                                                                        \
      _binlog_sid_v = writer.session().addEventSource(binlog::EventSource{                   \
        0, severity, #category, __func__, __FILE__, __LINE__, MSERIALIZE_FIRST(__VA_ARGS__), \
        binlog::detail::concatenated_tags(__VA_ARGS__).data()                                \
      });                                                                                    \
      _binlog_sid.store(_binlog_sid_v);                                                      \
    }                                                                                        \
    binlog::detail::addEventIgnoreFirst(writer, _binlog_sid_v, clock, __VA_ARGS__);          \
  } while (false)                                                                            \
  /**/

namespace binlog {
namespace detail {

// The first argument is dropped because __VA_ARGS__ cannot be empty,
// therefore it is always combined with something unrelated.
template <typename Unused, typename... T>
constexpr auto concatenated_tags(Unused&&, T&&...)
{
  return mserialize::cx_strcat(mserialize::tag<T>()...);
}

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
