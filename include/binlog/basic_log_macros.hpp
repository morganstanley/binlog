#ifndef BINLOG_BASIC_LOG_MACROS_HPP
#define BINLOG_BASIC_LOG_MACROS_HPP

#include <binlog/advanced_log_macros.hpp>
#include <binlog/default_session.hpp> // default_thread_local_writer

/**
 * BINLOG_<SEVERITY>(format, args...)
 *
 * Create a new event, in a thread-local lockfree queue,
 * consumable via binlog::consume().
 *
 * <SEVERITY> is one of: TRACE,DEBUG,INFO,WARN,ERROR,CRITICAL.
 *
 * The number of arguments must match the number of {} placeholders in `format`.
 * The event is timestamped using std::chrono::system_clock.
 * The event category will be "main".
 *
 * Severity levels can be enabled/disabled by:
 *
 *     binlog::Severity threshold = binlog::Severity::info; // choose the desired level
 *     binlog::default_thread_local_writer().setMinSeverity(threshold);
 *
 * Disabled severity levels do not produce any events,
 * and do not evaluate their log arguments.
 *
 * Global state is accessed internally (the default session and default thread local writer),
 * avoid using these macros in a global destructor context.
 * For greater control, see advanced_log_macros.hpp.
 *
 * @param format string literal with {} placeholders
 * @param args... any number of serializable, tagged, log arguments. Can be empty
 */

#define BINLOG_TRACE(...)    BINLOG_TRACE_WC(   binlog::default_thread_local_writer(), main, __VA_ARGS__)
#define BINLOG_DEBUG(...)    BINLOG_DEBUG_WC(   binlog::default_thread_local_writer(), main, __VA_ARGS__)
#define BINLOG_INFO(...)     BINLOG_INFO_WC(    binlog::default_thread_local_writer(), main, __VA_ARGS__)
#define BINLOG_WARN(...)     BINLOG_WARN_WC(    binlog::default_thread_local_writer(), main, __VA_ARGS__)
#define BINLOG_ERROR(...)    BINLOG_ERROR_WC(   binlog::default_thread_local_writer(), main, __VA_ARGS__)
#define BINLOG_CRITICAL(...) BINLOG_CRITICAL_WC(binlog::default_thread_local_writer(), main, __VA_ARGS__)

/**
 * BINLOG_<SEVERITY>_C(category, format, args...)
 *
 * Create a new event, in a thread-local lockfree queue,
 * using the specified `category`, consumable via binlog::consume().
 *
 * <SEVERITY> is one of: TRACE,DEBUG,INFO,WARN,ERROR,CRITICAL.
 *
 * The number of arguments must match the number of {} placeholders in `format`.
 * The event is timestamped using std::chrono::system_clock.
 *
 * Severity levels can be enabled/disabled the same way
 * as for BINLOG_<SEVERITY>. Disabled severity levels
 * do not produce any events, and do not evaluate their log arguments.
 *
 * Global state is accessed internally (the default session and default thread local writer),
 * avoid using these macros in a global destructor context.
 * For greater control, see advanced_log_macros.hpp.
 *
 * @param category arbitrary valid symbol name
 * @param format string literal with {} placeholders
 * @param args... any number of serializable, tagged, log arguments. Can be empty
 */

#define BINLOG_TRACE_C(   category, ...) BINLOG_TRACE_WC(   binlog::default_thread_local_writer(), category, __VA_ARGS__)
#define BINLOG_DEBUG_C(   category, ...) BINLOG_DEBUG_WC(   binlog::default_thread_local_writer(), category, __VA_ARGS__)
#define BINLOG_INFO_C(    category, ...) BINLOG_INFO_WC(    binlog::default_thread_local_writer(), category, __VA_ARGS__)
#define BINLOG_WARN_C(    category, ...) BINLOG_WARN_WC(    binlog::default_thread_local_writer(), category, __VA_ARGS__)
#define BINLOG_ERROR_C(   category, ...) BINLOG_ERROR_WC(   binlog::default_thread_local_writer(), category, __VA_ARGS__)
#define BINLOG_CRITICAL_C(category, ...) BINLOG_CRITICAL_WC(binlog::default_thread_local_writer(), category, __VA_ARGS__)

#endif // BINLOG_BASIC_LOG_MACROS_HPP
