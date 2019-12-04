#include <binlog/binlog.hpp>

#include <benchmark/benchmark.h>

#include <array>
#include <ios> // streamsize

namespace {

void doNotOptimizeBuffer(const char* buf, std::size_t size)
{
  for (std::size_t i = 0; i < size; ++i)
  {
    benchmark::DoNotOptimize(buf[i]);
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
    // Simulate the application using the data cache
    workingSet.fill(i&1 ? 'x' : 'y');
    doNotOptimizeBuffer(workingSet.data(), workingSet.size());

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
BENCHMARK(BM_addEventPoisonCache); // NOLINT

void BM_addEventNoClock(benchmark::State& state)
{
  binlog::Session session;
  binlog::SessionWriter writer(session);

  for (int i = 0; state.KeepRunning(); ++i)
  {
    BINLOG_CREATE_SOURCE_AND_EVENT_IF(writer, binlog::Severity::info, main, /*clock=*/0, "Single int: {}", i);

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

} // namespace

BENCHMARK_MAIN();
