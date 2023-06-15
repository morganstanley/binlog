// This example shows how to timestamp Binlog events with TSC

#include <binlog/binlog.hpp>

#include <cstdint>
#include <fstream>
#include <iostream>

// Make __rdtsc available. Works on x86 GCC, Clang, MSVC.
// Other platforms might need different treatment.
#ifdef _MSC_VER
  #include <intrin.h>
  #pragma intrinsic(__rdtsc)
#elif defined(__i386__) or defined(__x86_64__)
  #include <x86intrin.h>
#else
  // other platform - provide a dummy implementation
  std::uint64_t __rdtsc() { return 0; }
#endif

// Define a log macro, similar to BINLOG_INFO_W,
// that timestamps the event with TSC (instead of system_clock).
//
// For efficiency, BINLOG_CREATE_SOURCE_AND_EVENT is used instead of
// BINLOG_CREATE_SOURCE_AND_EVENT_IF - therefore the Session configured
// minSeverity is not honored, the Event is always emitted, saving a branch.
#define TSC_INFO_W(writer, ...) BINLOG_CREATE_SOURCE_AND_EVENT(writer, binlog::Severity::info, main, __rdtsc(), __VA_ARGS__)

std::uint64_t tscFrequency()
{
  // Getting the tsc frequency is beyond the scope of this example.
  // There are several, platform dependant ways, e.g:
  //  - https://github.com/torvalds/linux/blob/master/arch/x86/kernel/tsc.c
  //  - https://github.com/google/benchmark/blob/master/src/sysinfo.cc
  return 3'000'000'000; // For example only. Do NOT use in production.
}

int main()
{
  std::ofstream logfile("tscclock.blog", std::ofstream::out|std::ofstream::binary);

  binlog::Session session;

  // Create a ClockSync, that connects the TSC value to the wall clock time.
  const auto tscValue = __rdtsc();
  const binlog::ClockSync systemClockSync = binlog::systemClockSync();
  const binlog::ClockSync tscSync{
    tscValue,
    tscFrequency(),
    systemClockSync.nsSinceEpoch,
    systemClockSync.tzOffset,
    systemClockSync.tzName
  };

  // Add the created ClockSync to the metadata of the session.
  // Events consumed after this are assumed to be timestamped
  // with the clock defined by the ClockSync (this case, TSC)
  session.setClockSync(tscSync);

  // Add one event, consume the writer queue...

  binlog::SessionWriter writer(session);
  TSC_INFO_W(writer, "a={}, b={}", 123, 456);

  session.consume(logfile);

  if (! logfile)
  {
    std::cerr << "Failed to write tscclock.blog\n";
    return 1;
  }

  std::cout << "Binary log written to tscclock.blog\n";
  return 0;
}
