#include <binlog/detail/Cueue.hpp>

#include <benchmark/benchmark.h>

#include <atomic>
#include <cstring> // memcpy
#include <thread>
#include <vector>

namespace {

void doNotOptimizeBuffer(const char* buf, std::size_t size)
{
  for (std::size_t i = 0; i < size; ++i)
  {
    benchmark::DoNotOptimize(buf[i]);
  }
}

std::atomic<bool> g_done;

void readQueue(binlog::detail::Cueue& q, benchmark::State& state)
{
  std::size_t failedReadCount = 0;

  auto r = q.reader();

  while (true)
  {
    const auto rr = r.beginRead();
    if (rr.size != 0)
    {
      doNotOptimizeBuffer(rr.buffer, rr.size);
      r.endRead();
    }
    else if (g_done.load(std::memory_order_acquire))
    {
      break;
    }
    else
    {
      ++failedReadCount;
      std::this_thread::yield();
    }
  }

  state.counters["FailedRead"] = double(failedReadCount);
}

void BM_cueueWrite(benchmark::State& state)
{
  const std::size_t bufSize = std::size_t(state.range(0));
  const std::vector<char> buf(bufSize);

  std::vector<char> buffer(1 << 20);
  binlog::detail::Cueue q(1 << 20);

  g_done = false;
  std::thread reader(readQueue, std::ref(q), std::ref(state));

  auto w = q.writer();
  std::size_t totalWriteSize = 0;
  std::size_t failedWriteCount = 0;

  while (state.KeepRunning())
  {
    if (w.beginWrite(buf.size()))
    {
      w.write(buf.data(), std::streamsize(buf.size()));
      w.endWrite();
      totalWriteSize += buf.size();
    }
    else
    {
      ++failedWriteCount;
    }
  }

  g_done = true;
  reader.join();

  state.SetBytesProcessed(std::int64_t(totalWriteSize));
  state.counters["FailedWrite"] = double(failedWriteCount);
}

BENCHMARK(BM_cueueWrite)->Range(8, 1024); // NOLINT

} // namespace

BENCHMARK_MAIN();
