#ifndef BINLOG_SEVERITY_HPP
#define BINLOG_SEVERITY_HPP

#include <mserialize/cx_string.hpp>

#include <cstdint>

namespace binlog {

enum class Severity : std::uint16_t
{
  trace    = 1 << 5,
  debug    = 1 << 6,
  info     = 1 << 7,
  warning  = 1 << 8,
  error    = 1 << 9,
  critical = 1 << 10,
  no_logs  = 1 << 15, // For filtering, not to create events
};

inline mserialize::cx_string<4> severityToString(Severity severity)
{
  switch (severity)
  {
    case Severity::trace:    return mserialize::cx_string<4>{"TRAC"};
    case Severity::debug:    return mserialize::cx_string<4>{"DEBG"};
    case Severity::info:     return mserialize::cx_string<4>{"INFO"};
    case Severity::warning:  return mserialize::cx_string<4>{"WARN"};
    case Severity::error:    return mserialize::cx_string<4>{"ERRO"};
    case Severity::critical: return mserialize::cx_string<4>{"CRIT"};
    case Severity::no_logs:  return mserialize::cx_string<4>{"NOLG"};
    default:                 return mserialize::cx_string<4>{"UNKW"};
  }
}

} // namespace binlog

#endif // BINLOG_SEVERITY_HPP
