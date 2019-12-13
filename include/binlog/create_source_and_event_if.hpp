#ifndef BINLOG_CREATE_SOURCE_AND_EVENT_IF_HPP
#define BINLOG_CREATE_SOURCE_AND_EVENT_IF_HPP

#include <binlog/Session.hpp>
#include <binlog/SessionWriter.hpp>
#include <binlog/create_source_and_event.hpp>

/**
 * Call BINLOG_CREATE_SOURCE_AND_EVENT with the given
 * arguments if `severity` >= the minimum severity
 * configured for `writer`.
 *
 * If `severity` is below the configured minimum severity,
 * no event will be created, and the event arguments
 * will not be evaluated.
 *
 * @see BINLOG_CREATE_SOURCE_AND_EVENT
 */
#define BINLOG_CREATE_SOURCE_AND_EVENT_IF(writer, severity, category, clock, ...)     \
  do {                                                               \
    if (severity >= writer.session().minSeverity())                  \
    {                                                                \
      BINLOG_CREATE_SOURCE_AND_EVENT(writer, severity, category, clock, __VA_ARGS__); \
    }                                                                \
  } while (false)                                                    \
  /**/

#endif // BINLOG_CREATE_SOURCE_AND_EVENT_IF_HPP
