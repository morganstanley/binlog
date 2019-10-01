#ifndef BINLOG_TIME_HPP
#define BINLOG_TIME_HPP

#include <binlog/Entries.hpp> // ClockSync

#include <chrono>
#include <cstdint>
#include <ctime>

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
 * @pre frequency != 0
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

} // namespace binlog

#endif // BINLOG_TIME_HPP
