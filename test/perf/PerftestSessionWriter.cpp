#include <binlog/binlog.hpp>

#include <benchmark/benchmark.h>

#include <array>
#include <ios> // streamsize

#ifdef _WIN32
  #include <intrin.h>
  #pragma intrinsic(__rdtsc)
#else
  #include <x86intrin.h>
#endif

namespace {

void doNotOptimizeBuffer(const char* buf, std::size_t size)
{
  for (std::size_t i = 0; i < size; ++i)
  {
    auto buf_data = buf[i];
    benchmark::DoNotOptimize(buf_data);
  }
}

struct NullOstream
{
  NullOstream& write(const char*, std::streamsize) { return *this; }
};

void BM_addEvent(benchmark::State& state)
{
  binlog::Session session;
  binlog::SessionWriter writer(session);

  for (int i = 0; state.KeepRunning(); ++i)
  {
    BINLOG_INFO_W(writer, "Single int: {}", i);

    // flush the queue, otherwise queue allocation will be timed
    if (i == 2048)
    {
      state.PauseTiming();
      i = 0;
      NullOstream out;
      session.consume(out);
      state.ResumeTiming();
    }
  }
}
BENCHMARK(BM_addEvent); // NOLINT

void BM_addEventPoisonCache(benchmark::State& state)
{
  binlog::Session session;
  binlog::SessionWriter writer(session);

  std::array<char, 64 * 1024> workingSet{};

  for (int i = 0; state.KeepRunning(); ++i)
  {
    BINLOG_INFO_W(writer, "Single int: {}", i);

    state.PauseTiming();

    // Simulate the application using the data cache
    workingSet.fill(i&1 ? 'x' : 'y'); // NOLINT
    doNotOptimizeBuffer(workingSet.data(), workingSet.size());

    // flush the queue, otherwise queue allocation will be timed
    if (i == 2048)
    {
      i = 0;
      NullOstream out;
      session.consume(out);
    }

    state.ResumeTiming();
  }
}
BENCHMARK(BM_addEventPoisonCache); // NOLINT

void BM_addEventNoClock(benchmark::State& state)
{
  binlog::Session session;
  binlog::SessionWriter writer(session);

  for (int i = 0; state.KeepRunning(); ++i)
  {
    BINLOG_CREATE_SOURCE_AND_EVENT(writer, binlog::Severity::info, main, /*clock=*/0, "Single int: {}", i);

    if (i == 2048)
    {
      state.PauseTiming();
      i = 0;
      NullOstream out;
      session.consume(out);
      state.ResumeTiming();
    }
  }
}
BENCHMARK(BM_addEventNoClock); // NOLINT

void BM_addEventTscClock(benchmark::State& state)
{
  binlog::Session session;
  binlog::SessionWriter writer(session);

  for (int i = 0; state.KeepRunning(); ++i)
  {
    BINLOG_CREATE_SOURCE_AND_EVENT(writer, binlog::Severity::info, main, __rdtsc(), "Single int: {}", i);

    if (i == 2048)
    {
      state.PauseTiming();
      i = 0;
      NullOstream out;
      session.consume(out);
      state.ResumeTiming();
    }
  }
}
BENCHMARK(BM_addEventTscClock); // NOLINT

// Benchmark logging of different arguments

void BM_addEvent_ThreeFloatArguments(benchmark::State& state)
{
  binlog::Session session;
  binlog::SessionWriter writer(session);

  const float x = 1.2345f;
  const float y = 6.7890f;
  const float z = 8.1234f;

  for (int i = 0; state.KeepRunning(); ++i)
  {
    BINLOG_INFO_W(writer, "Three floats: x={} y={} z={}", x, y, z);

    // flush the queue, otherwise queue allocation will be timed
    if (i == 2048)
    {
      state.PauseTiming();
      i = 0;
      NullOstream out;
      session.consume(out);
      state.ResumeTiming();
    }
  }
}
BENCHMARK(BM_addEvent_ThreeFloatArguments); // NOLINT

void BM_addEvent_OneStringArgument(benchmark::State& state)
{
  binlog::Session session;
  binlog::SessionWriter writer(session);

  const std::string s = "foobar";

  for (int i = 0; state.KeepRunning(); ++i)
  {
    BINLOG_INFO_W(writer, "One string: {}", s);

    // flush the queue, otherwise queue allocation will be timed
    if (i == 2048)
    {
      state.PauseTiming();
      i = 0;
      NullOstream out;
      session.consume(out);
      state.ResumeTiming();
    }
  }
}
BENCHMARK(BM_addEvent_OneStringArgument); // NOLINT

} // namespace

BENCHMARK_MAIN();
