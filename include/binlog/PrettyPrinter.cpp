#include <binlog/PrettyPrinter.hpp>

#include <binlog/ToStringVisitor.hpp>

#include <mserialize/detail/tag_util.hpp>
#include <mserialize/string_view.hpp>
#include <mserialize/visit.hpp>

#include <cstddef>
#include <cstdlib> // abs
#include <iomanip> // setw
#include <ostream>

namespace {

// Given path=foo/bar/baz.cpp, write baz.cpp to out,
// or the full path if no slash found.
void printFilename(std::ostream& out, const std::string& path)
{
  const std::size_t i = path.rfind('/') + 1;
  out.write(path.data() + i, std::streamsize(path.size() - i));
}

// print `seconds` as TZ offset in the ISO 8601 format (e.g: +0340 or -0430)
// @pre out.fill() == '0'
void printTimeZoneOffset(std::ostream& out, int seconds)
{
  const char sign = (seconds >= 0) ? '+' : '-';
  const std::int64_t psecs = std::abs(seconds);
  const std::int64_t hours = psecs / 3600;
  const std::int64_t mins  = (psecs / 60) - 60 * hours;
  out << sign
      << std::setw(2) << hours
      << std::setw(2) << mins;
}

} // namespace

namespace binlog {

PrettyPrinter::PrettyPrinter(std::string eventFormat, std::string timeFormat)
  :_eventFormat(std::move(eventFormat)),
   _timeFormat(std::move(timeFormat))
{}

void PrettyPrinter::printEvent(
  std::ostream& out,
  const Event& event,
  const Actor& actor,
  const ClockSync& clockSync
) const
{
  // TODO(benedek) perf: write local buffer instead of ostream directly

  for (std::size_t i = 0; i < _eventFormat.size(); ++i)
  {
    const char c = _eventFormat[i];
    if (c == '%' && ++i != _eventFormat.size())
    {
      const char spec = _eventFormat[i];
      printEventField(out, spec, event, actor, clockSync);
    }
    else
    {
      out.put(c);
    }
  }
}

void PrettyPrinter::printEventField(
  std::ostream& out,
  char spec,
  const Event& event,
  const Actor& actor,
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
    out << actor.name;
    break;
  case 't':
    out << actor.id;
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

void PrettyPrinter::printEventMessage(std::ostream& out, const Event& event) const
{
  // TODO(benedek) perf: write local buffer instead of ostream directly

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

void PrettyPrinter::printProducerLocalTime(std::ostream& out, const ClockSync& clockSync, std::uint64_t clockValue) const
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

void PrettyPrinter::printUTCTime(std::ostream& out, const ClockSync& clockSync, std::uint64_t clockValue) const
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

void PrettyPrinter::printTime(std::ostream& out, BrokenDownTime& bdt, int tzoffset, const char* tzname) const
{
  const char oldFill = out.fill('0');

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

  out.fill(oldFill);
}

void PrettyPrinter::printTimeField(std::ostream& out, char spec, BrokenDownTime& bdt, int tzoffset, const char* tzname) const
{
  // TODO(benedek) perf: there's a faster way to print two digits (y,m,d,H,M,S)

  switch (spec)
  {
  case 'Y':
    out << bdt.tm_year + 1900;
    break;
  case 'y':
    out << std::setw(2) << bdt.tm_year % 100;
    break;
  case 'm':
    out << std::setw(2) << bdt.tm_mon + 1;
    break;
  case 'd':
    out << std::setw(2) << bdt.tm_mday;
    break;
  case 'H':
    out << std::setw(2) << bdt.tm_hour;
    break;
  case 'M':
    out << std::setw(2) << bdt.tm_min;
    break;
  case 'S':
    out << std::setw(2) << bdt.tm_sec;
    break;
  case 'z':
    printTimeZoneOffset(out, tzoffset);
    break;
  case 'Z':
    out << tzname;
    break;
  case 'N':
    out << std::setw(9) << bdt.tm_nsec;
    break;
  default:
    out << '%' << spec;
    break;
  }
}

} // namespace binlog
