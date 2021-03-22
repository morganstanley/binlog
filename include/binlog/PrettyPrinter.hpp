#ifndef BINLOG_PRETTY_PRINTER_HPP
#define BINLOG_PRETTY_PRINTER_HPP

#include <binlog/Entries.hpp>
#include <binlog/Range.hpp>
#include <binlog/Time.hpp>
#include <binlog/detail/OstreamBuffer.hpp>

#include <mserialize/Visitor.hpp>

#include <cstdint>
#include <iosfwd>
#include <string>

namespace binlog {

/**
 * Convert Events to string, according to the specified format.
 *
 * Event format placeholders:
 *
 *    %I Source id
 *    %S Severity
 *    %C Category
 *    %M Function
 *    %F File, full path
 *    %G File, file name only
 *    %L Line
 *    %P Format string
 *    %T Argument tags
 *    %n Writer (thread) name
 *    %t Writer (thread) id
 *    %d Timestamp, in producer timezone
 *    %u Timestamp, in UTC
 *    %r Timestamp, raw clock value
 *    %m Message (format string with arguments substituted)
 *    %% Literal %
 *
 * Time format placeholders (used by %d and %u):
 *
 *    %Y, %y, %m, %d, %H, %M, %S, %z, %Z as for strftime
 *    %N Nanoseconds (0-999999999)
 */
class PrettyPrinter
{
public:
  PrettyPrinter(std::string eventFormat, std::string timeFormat);

  /**
   * Print `event` using `writerProp` and `clockSync`
   * to `ostr`, according to the format specified in the consturctor.
   *
   * If clockSync.clockFrequency is not positive after casting to int64_t,
   * broken down timestamps (%d and %u) are shown
   * as: "no_clock_sync?", as there's not enough context to
   * render them. The raw clock value remains accessible via %r.
   *
   * @pre event.source must be valid
   */
  void printEvent(
    std::ostream& ostr,
    const Event& event,
    const WriterProp& writerProp = {},
    const ClockSync& clockSync = {}
  );

  /**
   * If the type indicated by `sb` is known, deserialize it from `input`,
   * and print it to `out`, then return true.
   *
   * If the type is not known, `input` remains unchanged and the method returns false.
   *
   * The output might be affected by the last call to printEvent.
   */
  bool printStruct(detail::OstreamBuffer& out, mserialize::Visitor::StructBegin sb, Range& input) const;

private:
  void printEventField(
    detail::OstreamBuffer& out,
    char spec,
    const Event& event,
    const WriterProp& writerProp
  ) const;

  void printEventMessage(detail::OstreamBuffer& out, const Event& event) const;

  void printProducerLocalTime(detail::OstreamBuffer& out, std::uint64_t clockValue) const;
  void printUTCTime(detail::OstreamBuffer& out, std::uint64_t clockValue) const;

  void printTime(detail::OstreamBuffer& out, BrokenDownTime& bdt, int tzoffset, const char* tzname) const;
  void printTimeField(detail::OstreamBuffer& out, char spec, BrokenDownTime& bdt, int tzoffset, const char* tzname) const;

  std::string _eventFormat;
  std::string _timeFormat;
  bool _useLocaltime; // true if timestamps in messages should be rendered in producer-localtime
  const ClockSync* _clockSync;
};

} // namespace binlog

#endif // BINLOG_PRETTY_PRINTER_HPP
