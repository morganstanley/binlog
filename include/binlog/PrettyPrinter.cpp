#include <binlog/PrettyPrinter.hpp>

#include <binlog/ToStringVisitor.hpp>

#include <mserialize/detail/Visit.hpp> // IntegerToHex
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

// false iff %u appears earlier than %d in `format`, % escapes %
bool useLocaltime(const std::string& format)
{
  bool placeholder = false;
  for (char c : format)
  {
    if (placeholder)
    {
      if (c == 'd') { return true; }
      if (c == 'u') { return false; }
      placeholder = false;
    }
    else
    {
      placeholder = (c == '%');
    }
  }
  return true;
}

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
  (void)snprintf(buf, sizeof(buf), "%.9d", i);
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
   _timeFormat(std::move(timeFormat)),
   _useLocaltime(useLocaltime(_eventFormat)),
   _clockSync(nullptr)
{}

void PrettyPrinter::printEvent(
  std::ostream& ostr,
  const Event& event,
  const WriterProp& writerProp,
  const ClockSync& clockSync
)
{
  detail::OstreamBuffer out(ostr);
  _clockSync = &clockSync;

  for (std::size_t i = 0; i < _eventFormat.size(); ++i)
  {
    const char c = _eventFormat[i];
    if (c == '%' && ++i != _eventFormat.size())
    {
      const char spec = _eventFormat[i];
      printEventField(out, spec, event, writerProp);
    }
    else
    {
      out.put(c);
    }
  }
}

bool PrettyPrinter::printStruct(detail::OstreamBuffer& out, mserialize::Visitor::StructBegin sb, Range& input) const
{
  if (sb.name == "binlog::address" && sb.tag == "`value'L")
  {
    const std::uint64_t value = input.read<std::uint64_t>();
    mserialize::detail::IntegerToHex tohex;
    tohex.visit(value);
    out << "0x" << tohex.value();
    return true;
  }

  if (sb.name == "std::chrono::system_clock::time_point" && sb.tag == "`ns'l")
  {
    if (_clockSync == nullptr) { return false; }

    BrokenDownTime bdt{};
    const auto sinceEpoch = std::chrono::nanoseconds{input.read<std::int64_t>()};

    if (_useLocaltime)
    {
      const std::chrono::nanoseconds sinceEpochTz = sinceEpoch + std::chrono::seconds{_clockSync->tzOffset};
      nsSinceEpochToBrokenDownTimeUTC(sinceEpochTz, bdt);
      printTime(out, bdt, _clockSync->tzOffset, _clockSync->tzName.data());
    }
    else
    {
      nsSinceEpochToBrokenDownTimeUTC(sinceEpoch, bdt);
      printTime(out, bdt, 0, "UTC");
    }
    return true;
  }

  if (sb.name.starts_with("std::chrono::duration<Rep,"))
  {
    const char* suffix =
      sb.name.ends_with("std::nano>") ? "ns" :
      sb.name.ends_with("std::micro>") ? "us" :
      sb.name.ends_with("std::milli>") ? "ms" :
      sb.name.ends_with("std::ratio<1>>") ? "s" :
      sb.name.ends_with("std::ratio<60>>") ? "m" :
      sb.name.ends_with("std::ratio<3600>>") ? "h" :
      nullptr;
    if (suffix != nullptr)
    {
      if (sb.tag == "`count'l")
      {
        const std::int64_t count = input.read<std::int64_t>();
        out << count << suffix;
        return true;
      }
      if (sb.tag == "`count'i") // on MSVC, for minutes and hours
      {
        const std::int32_t count = input.read<std::int32_t>();
        out << count << suffix;
        return true;
      }
    }
  }

  if ((sb.name == "std::filesystem::path" && sb.tag == "`str'[c")
  || (sb.name == "std::filesystem::directory_entry" && sb.tag == "`path'{std::filesystem::path`str'[c}")
  || (sb.name == "std::error_code" && sb.tag == "`message'[c")
  ) {
    const std::uint32_t size = input.read<std::uint32_t>();
    out.write(input.view(size), size);
    return true;
  }

  return false;
}

void PrettyPrinter::printEventField(
  detail::OstreamBuffer& out,
  char spec,
  const Event& event,
  const WriterProp& writerProp
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
    printProducerLocalTime(out, event.clockValue);
    break;
  case 'u':
    printUTCTime(out, event.clockValue);
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
  ToStringVisitor visitor(out, this);

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

void PrettyPrinter::printProducerLocalTime(detail::OstreamBuffer& out, std::uint64_t clockValue) const
{
  // TODO(benedek) perf: cache bdt, update instead of complete recompute

  if (std::int64_t(_clockSync->clockFrequency) > 0)
  {
    BrokenDownTime bdt{};
    const std::chrono::nanoseconds sinceEpoch = clockToNsSinceEpoch(*_clockSync, clockValue);
    const std::chrono::nanoseconds sinceEpochTz = sinceEpoch + std::chrono::seconds{_clockSync->tzOffset};
    nsSinceEpochToBrokenDownTimeUTC(sinceEpochTz, bdt);
    printTime(out, bdt, _clockSync->tzOffset, _clockSync->tzName.data());
  }
  else
  {
    out << "no_clock_sync?";
  }
}

void PrettyPrinter::printUTCTime(detail::OstreamBuffer& out, std::uint64_t clockValue) const
{
  // TODO(benedek) perf: cache bdt, update instead of complete recompute

  if (std::int64_t(_clockSync->clockFrequency) > 0)
  {
    BrokenDownTime bdt{};
    const std::chrono::nanoseconds sinceEpoch = clockToNsSinceEpoch(*_clockSync, clockValue);
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
