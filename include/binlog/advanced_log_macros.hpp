#ifndef BINLOG_ADVANCED_LOG_MACROS_HPP
#define BINLOG_ADVANCED_LOG_MACROS_HPP

#include <binlog/Severity.hpp>
#include <binlog/Time.hpp>
#include <binlog/create_source_and_event_if.hpp>

#include <chrono>

/**
 * BINLOG_<SEVERITY>_WC(writer, category, format, args...)
 *
 * Add a new event to `writer` if <SEVERITY> is greater-or-equal
 * the minimum severity configured for `writer`,
 * using the specified `category`.
 *
 * <SEVERITY> is one of: TRACE,DEBUG,INFO,WARN,ERROR,CRITICAL.
 *
 * The number of arguments must match the number of {} placeholders in `format`.
 * The event is timestamped using std::chrono::system_clock.
 * If <SEVERITY> is below the minimum severity configured
 * for `writer`, no event is created, the arguments are not evaluated.
 *
 * @param writer binlog::SessionWriter
 * @param category arbitrary valid symbol name
 * @param format string literal with {} placeholders
 * @param args... any number of serializable, tagged, log arguments. Can be empty
 */

#define BINLOG_TRACE_WC(writer, category, ...)                                  \
  BINLOG_CREATE_SOURCE_AND_EVENT_IF(                                            \
    writer, binlog::Severity::trace, category,                                  \
    binlog::clockNow(),                                                         \
    __VA_ARGS__                                                                 \
  )                                                                             \
  /**/

#define BINLOG_DEBUG_WC(writer, category, ...)                                  \
  BINLOG_CREATE_SOURCE_AND_EVENT_IF(                                            \
    writer, binlog::Severity::debug, category,                                  \
    binlog::clockNow(),                                                         \
    __VA_ARGS__                                                                 \
  )                                                                             \
  /**/

#define BINLOG_INFO_WC(writer, category, ...)                                   \
  BINLOG_CREATE_SOURCE_AND_EVENT_IF(                                            \
    writer, binlog::Severity::info, category,                                   \
    binlog::clockNow(),                                                         \
    __VA_ARGS__                                                                 \
  )                                                                             \
  /**/

#define BINLOG_WARN_WC(writer, category, ...)                                   \
  BINLOG_CREATE_SOURCE_AND_EVENT_IF(                                            \
    writer, binlog::Severity::warning, category,                                \
    binlog::clockNow(),                                                         \
    __VA_ARGS__                                                                 \
  )                                                                             \
  /**/

#define BINLOG_ERROR_WC(writer, category, ...)                                  \
  BINLOG_CREATE_SOURCE_AND_EVENT_IF(                                            \
    writer, binlog::Severity::error, category,                                  \
    binlog::clockNow(),                                                         \
    __VA_ARGS__                                                                 \
  )                                                                             \
  /**/

#define BINLOG_CRITICAL_WC(writer, category, ...)                               \
  BINLOG_CREATE_SOURCE_AND_EVENT_IF(                                            \
    writer, binlog::Severity::critical, category,                               \
    binlog::clockNow(),                                                         \
    __VA_ARGS__                                                                 \
  )                                                                             \
  /**/

/**
 * BINLOG_<SEVERITY>_W(writer, format, args...)
 *
 * Add a new event to `writer` if <SEVERITY> is greater-or-equal
 * the minimum severity configured for `writer`.
 *
 * <SEVERITY> is one of: TRACE,DEBUG,INFO,WARN,ERROR,CRITICAL.
 *
 * The number of arguments must match the number of {} placeholders in `format`.
 * The event is timestamped using std::chrono::system_clock.
 * If <SEVERITY> is below the minimum severity configured
 * for `writer`, no event is created, the arguments are not evaluated.
 * The event category will be "main".
 *
 * @param writer binlog::SessionWriter
 * @param format string literal with {} placeholders
 * @param args... any number of serializable, tagged, log arguments. Can be empty
 */

#define BINLOG_TRACE_W(   writer, ...) BINLOG_TRACE_WC(   writer, main, __VA_ARGS__)
#define BINLOG_DEBUG_W(   writer, ...) BINLOG_DEBUG_WC(   writer, main, __VA_ARGS__)
#define BINLOG_INFO_W(    writer, ...) BINLOG_INFO_WC(    writer, main, __VA_ARGS__)
#define BINLOG_WARN_W(    writer, ...) BINLOG_WARN_WC(    writer, main, __VA_ARGS__)
#define BINLOG_ERROR_W(   writer, ...) BINLOG_ERROR_WC(   writer, main, __VA_ARGS__)
#define BINLOG_CRITICAL_W(writer, ...) BINLOG_CRITICAL_WC(writer, main, __VA_ARGS__)

#endif // BINLOG_ADVANCED_LOG_MACROS_HPP
