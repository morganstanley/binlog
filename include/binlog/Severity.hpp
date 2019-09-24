#ifndef BINLOG_SEVERITY_HPP
#define BINLOG_SEVERITY_HPP

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
};

} // namespace binlog

#endif // BINLOG_SEVERITY_HPP
