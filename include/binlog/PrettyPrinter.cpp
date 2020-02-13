#include <binlog/PrettyPrinter.hpp>

#include <binlog/ToStringVisitor.hpp>

#include <mserialize/detail/tag_util.hpp>
#include <mserialize/string_view.hpp>
#include <mserialize/visit.hpp>

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib> // abs
#include <iomanip> // setw
#include <ostream>

namespace {

// Given path=foo/bar/baz.cpp, write baz.cpp to out,
// or the full path if no path separator (/ or \) found.
void printFilename(binlog::detail::OstreamBuffer& out, const std::string& path)
{
  std::size_t i = path.size();
  while (i != 0)
  {
    if (path[i-1] == '/' || path[i-1] == '\\') { break; }
    --i;
  }
  out.write(path.data() + i, path.size() - i);
}

void printTwoDigits(binlog::detail::OstreamBuffer& out, int i)
{
  assert(0 <= i && i < 100);
  const int b = i % 10;
  const int a = (i - b) / 10;
  const char digits[2]{char('0' + a), char('0' + b)};
  out.write(digits, 2);
}

void printNineDigits(binlog::detail::OstreamBuffer& out, int i)
{
  char buf[16];
  snprintf(buf, sizeof(buf), "%.9d", i);
  out.write(buf, 9);
}

// print `seconds` as TZ offset in the ISO 8601 format (e.g: +0340 or -0430)
// @pre out.fill() == '0'
void printTimeZoneOffset(binlog::detail::OstreamBuffer& out, int seconds)
{
  const char sign = (seconds >= 0) ? '+' : '-';
  const int psecs = std::abs(seconds);
  const int hours = psecs / 3600;
  const int mins  = (psecs / 60) - 60 * hours;
  out.put(sign);
  printTwoDigits(out, hours < 100 ? hours : 0);
  printTwoDigits(out, mins < 100 ? mins : 0);
}

} // namespace

namespace binlog {

PrettyPrinter::PrettyPrinter(std::string eventFormat, std::string timeFormat)
  :_eventFormat(std::move(eventFormat)),
   _timeFormat(std::move(timeFormat))
{}

void PrettyPrinter::printEvent(
  std::ostream& ostr,
  const Event& event,
  const WriterProp& writerProp,
  const ClockSync& clockSync
) const
{
  detail::OstreamBuffer out(ostr);

  for (std::size_t i = 0; i < _eventFormat.size(); ++i)
  {
    const char c = _eventFormat[i];
    if (c == '%' && ++i != _eventFormat.size())
    {
      const char spec = _eventFormat[i];
      printEventField(out, spec, event, writerProp, clockSync);
    }
    else
    {
      out.put(c);
    }
  }
}

void PrettyPrinter::printEventField(
  detail::OstreamBuffer& out,
  char spec,
  const Event& event,
  const WriterProp& writerProp,
  const ClockSync& clockSync
) const
{
  switch (spec)
  {
  case 'I':
    out << event.source->id;
    break;
  case 'S':
    out << severityToString(event.source->severity);
    break;
  case 'C':
    out << event.source->category;
    break;
  case 'M':
    out << event.source->function;
    break;
  case 'F':
    out << event.source->file;
    break;
  case 'G':
    printFilename(out, event.source->file);
    break;
  case 'L':
    out << event.source->line;
    break;
  case 'P':
    out << event.source->formatString;
    break;
  case 'T':
    out << event.source->argumentTags;
    break;
  case 'n':
    out << writerProp.name;
    break;
  case 't':
    out << writerProp.id;
    break;
  case 'd':
    printProducerLocalTime(out, clockSync, event.clockValue);
    break;
  case 'u':
    printUTCTime(out, clockSync, event.clockValue);
    break;
  case 'r':
    out << event.clockValue;
    break;
  case 'm':
    printEventMessage(out, event);
    break;
  case '%':
    out.put('%');
    break;
  default:
    out << '%' << spec;
    break;
  }
}

void PrettyPrinter::printEventMessage(detail::OstreamBuffer& out, const Event& event) const
{
  mserialize::string_view tags = event.source->argumentTags;
  Range args = event.arguments;
  ToStringVisitor visitor(out);

  const std::string& fmt = event.source->formatString;
  for (std::size_t i = 0; i < fmt.size(); ++i)
  {
    const char c = fmt[i];
    if (c == '{' && fmt[i+1] == '}')
    {
      const mserialize::string_view tag = mserialize::detail::tag_pop(tags);
      mserialize::visit(tag, visitor, args);
      ++i; // skip }
    }
    else
    {
      out.put(c);
    }
  }
}

void PrettyPrinter::printProducerLocalTime(detail::OstreamBuffer& out, const ClockSync& clockSync, std::uint64_t clockValue) const
{
  // TODO(benedek) perf: cache bdt, update instead of complete recompute

  if (clockSync.clockFrequency != 0)
  {
    BrokenDownTime bdt{};
    const std::chrono::nanoseconds sinceEpoch = clockToNsSinceEpoch(clockSync, clockValue);
    const std::chrono::nanoseconds sinceEpochTz = sinceEpoch + std::chrono::seconds{clockSync.tzOffset};
    nsSinceEpochToBrokenDownTimeUTC(sinceEpochTz, bdt);
    printTime(out, bdt, clockSync.tzOffset, clockSync.tzName.data());
  }
  else
  {
    out << "no_clock_sync?";
  }
}

void PrettyPrinter::printUTCTime(detail::OstreamBuffer& out, const ClockSync& clockSync, std::uint64_t clockValue) const
{
  // TODO(benedek) perf: cache bdt, update instead of complete recompute

  if (clockSync.clockFrequency != 0)
  {
    BrokenDownTime bdt{};
    const std::chrono::nanoseconds sinceEpoch = clockToNsSinceEpoch(clockSync, clockValue);
    nsSinceEpochToBrokenDownTimeUTC(sinceEpoch, bdt);
    printTime(out, bdt, 0, "UTC");
  }
  else
  {
    out << "no_clock_sync?";
  }
}

void PrettyPrinter::printTime(detail::OstreamBuffer& out, BrokenDownTime& bdt, int tzoffset, const char* tzname) const
{
  for (std::size_t i = 0; i < _timeFormat.size(); ++i)
  {
    const char c = _timeFormat[i];
    if (c == '%' && ++i != _timeFormat.size())
    {
      const char spec = _timeFormat[i];
      printTimeField(out, spec, bdt, tzoffset, tzname);
    }
    else
    {
      out.put(c);
    }
  }
}

void PrettyPrinter::printTimeField(detail::OstreamBuffer& out, char spec, BrokenDownTime& bdt, int tzoffset, const char* tzname) const
{
  switch (spec)
  {
  case 'Y':
    out << bdt.tm_year + 1900;
    break;
  case 'y':
    printTwoDigits(out, bdt.tm_year % 100);
    break;
  case 'm':
    printTwoDigits(out, bdt.tm_mon + 1);
    break;
  case 'd':
    printTwoDigits(out, bdt.tm_mday);
    break;
  case 'H':
    printTwoDigits(out, bdt.tm_hour);
    break;
  case 'M':
    printTwoDigits(out, bdt.tm_min);
    break;
  case 'S':
    printTwoDigits(out, bdt.tm_sec);
    break;
  case 'z':
    printTimeZoneOffset(out, tzoffset);
    break;
  case 'Z':
    out << tzname;
    break;
  case 'N':
    printNineDigits(out, bdt.tm_nsec);
    break;
  default:
    out << '%' << spec;
    break;
  }
}

} // namespace binlog
