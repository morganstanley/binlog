#include <binlog/Time.hpp>

namespace binlog {

std::chrono::nanoseconds ticksToNanoseconds(std::uint64_t frequency, std::int64_t ticks)
{
  // Compute `ticks * std::nano::den / frequency`.
  // To avoid `ticks * std::nano::den` overflowing, assume that:
  //   ticks = q*f + r    and     r < f
  // then multiply and divide the additives one by one.
  // We assume that the result fits on 64 bits, as that many nanos cover a long timespan.
  const std::int64_t sf = std::int64_t(frequency);
  const std::int64_t q = ticks / sf;
  const std::int64_t r = ticks % sf;
  return std::chrono::nanoseconds{q * std::nano::den + r * std::nano::den / sf};
}

std::chrono::nanoseconds clockToNsSinceEpoch(const ClockSync& clockSync, std::uint64_t clockValue)
{
  using nanos = std::chrono::nanoseconds;

  const std::int64_t diffValue = std::int64_t(clockValue - clockSync.clockValue);
  const nanos diff = ticksToNanoseconds(clockSync.clockFrequency, diffValue);
  const nanos sinceEpoch = nanos{clockSync.nsSinceEpoch} + diff;

  return sinceEpoch;
}

void nsSinceEpochToBrokenDownTimeUTC(std::chrono::nanoseconds sinceEpoch, BrokenDownTime& dst)
{
  using clock = std::chrono::system_clock;

  // assumption: system_clock measures Unix Time
  // (i.e., time since 1970.01.01 00:00:00 UTC, not counting leap seconds).
  // Valid since C++20, tested by unit tests.
  const clock::time_point tp{std::chrono::duration_cast<clock::duration>(sinceEpoch)};
  const std::time_t tt = clock::to_time_t(tp);

  #ifdef _WIN32
    gmtime_s(&dst, &tt);
  #else // assume POSIX
    gmtime_r(&tt, &dst);
  #endif

  // set the sub-second part
  const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(sinceEpoch);
  const std::chrono::nanoseconds remainder{sinceEpoch - seconds};

  dst.tm_nsec = int(remainder.count());
}

} // namespace binlog
