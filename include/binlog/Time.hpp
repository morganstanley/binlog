#ifndef BINLOG_TIME_HPP
#define BINLOG_TIME_HPP

#include <binlog/Entries.hpp> // ClockSync

#include <chrono>
#include <cstdint>
#include <ctime>

#include <time.h> // NOLINT localtime_r, gmtime_r, clock_gettime

namespace binlog {

struct BrokenDownTime : std::tm
{
  int tm_nsec; /**< fraction of seconds, expressed in nanoseconds */
};

/**
 * Given a clock ticking at `frequency`,
 * calculate the number of complete nanoseconds elapsed
 * while `ticks` number of clock ticks.
 *
 * For sub-nanosecond precision clocks, the result
 * is truncated, e.g:
 *
 *    frequency = 3 GHz (3 ticks / 1 ns)
 *    ticks = 4
 *    theoretical result = 1 + 1/3 nanoseconds
 *    result = 1 nanoseconds
 *
 * @pre int64_t(frequency) > 0
 */
std::chrono::nanoseconds ticksToNanoseconds(std::uint64_t frequency, std::int64_t ticks);

/**
 * Given a clock sync, calculate the number of
 * nanoseconds elapsed since the Unix Epoch
 * at the time point given by `clockValue`.
 *
 * Adjustments to the system clock (e.g: DST changes)
 * happening between `clockSync.clockValue` and `clockValue`,
 * which are not reflected by the log clock
 * (e.g: if the log clock is the CPU TSC)
 * are not reflected by the returned value.
 *
 * @pre clockSync.clockFrequency != 0
 */
std::chrono::nanoseconds clockToNsSinceEpoch(const ClockSync& clockSync, std::uint64_t clockValue);

/**
 * Converts the time point given by `sinceEpoch` to BrokenDownTime,
 * expressed in UTC.
 *
 * @param sinceEpoch nanoseconds since UNIX epoch, not counting leap seconds
 * @param dst result is written to this variable
 */
void nsSinceEpochToBrokenDownTimeUTC(std::chrono::nanoseconds sinceEpoch, BrokenDownTime& dst);

/**
 * Create a ClockSync corresponding to std::chrono::system_clock.
 *
 * Time zone is set according to std::localtime.
 *
 * This function is inline to make the log producer use-case header-only.
 */
inline ClockSync systemClockSync()
{
  using Clock = std::chrono::system_clock;
  static_assert(Clock::period::num == 1, "Clock measures integer fractions of a second");

  const auto now = Clock::now();
  const auto since_epoch = now.time_since_epoch();
  const std::time_t now_tt = Clock::to_time_t(now);

  std::tm now_tm{};
  #ifdef _WIN32
    localtime_s(&now_tm, &now_tt);
  #else // assume POSIX
    localtime_r(&now_tt, &now_tm);
  #endif

  // TODO(benedek) platform: access TZ related tm fields, where available
  int offset = 0;
  char offset_str[6] = {0};
  if (strftime(offset_str, sizeof(offset_str), "%z", &now_tm) == 5)
  {
    // offset_str is +HHMM or -HHMM
    offset = (
      ((offset_str[1]-'0') * 10 + (offset_str[2]-'0')) * 3600 +
      ((offset_str[3]-'0') * 10 + (offset_str[4]-'0')) * 60
    ) * ((offset_str[0] == '-') ? -1 : 1);
  }

  char tzName[128] = {0};
  if (strftime(tzName, sizeof(tzName), "%Z", &now_tm) == 0)
  {
    tzName[0] = 0;
  }

  // on linux, clock_gettime is used directly
  #ifdef __linux__
    const uint64_t frequency = 1'000'000'000; // nanoseconds
  #else
    const uint64_t frequency = std::uint64_t(Clock::period::den);
  #endif

  return ClockSync{
    std::uint64_t(since_epoch.count()),
    frequency,
    std::uint64_t(std::chrono::duration_cast<std::chrono::nanoseconds>(since_epoch).count()),
    offset,
    tzName
  };
}

/** Get the number of nanoseconds since the UNIX epoch in UTC (no leaps seconds) */
inline std::uint64_t clockNow()
{
  // Depending on how libstdc++ is built (_GLIBCXX_USE_CLOCK_GETTIME_SYSCALL),
  // system_clock::now() can result in a clock_gettime syscall,
  // which is really slow compared to the vsyscall equivalent.
  // Call clock_gettime unconditionally on linux:
  #ifdef __linux__
    struct timespec ts{};
    clock_gettime(CLOCK_REALTIME, &ts);
    const std::chrono::nanoseconds nanos{
      std::chrono::seconds{ts.tv_sec} + std::chrono::nanoseconds{ts.tv_nsec}
    };
    return std::uint64_t(nanos.count());
  #else
    return std::uint64_t(std::chrono::system_clock::now().time_since_epoch().count());
  #endif
}

} // namespace binlog

#endif // BINLOG_TIME_HPP
