#include <binlog/detail/Queue.hpp>
#include <binlog/detail/QueueReader.hpp>
#include <binlog/detail/QueueWriter.hpp>

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
    auto buf_data = buf[i];
    benchmark::DoNotOptimize(buf_data);
  }
}

std::atomic<bool> g_done;

void readQueue(binlog::detail::Queue& q, benchmark::State& state)
{
  std::size_t failedReadCount = 0;

  binlog::detail::QueueReader r(q);

  while (true)
  {
    const auto rr = r.beginRead();
    if (rr.size1 != 0)
    {
      doNotOptimizeBuffer(rr.buffer1, rr.size1);
      doNotOptimizeBuffer(rr.buffer2, rr.size2);
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

void BM_queueWrite(benchmark::State& state)
{
  const std::size_t bufSize = std::size_t(state.range(0));
  const std::vector<char> buf(bufSize);

  std::vector<char> buffer(1 << 20);
  binlog::detail::Queue q(buffer.data(), buffer.size());

  g_done = false;
  std::thread reader(readQueue, std::ref(q), std::ref(state));

  binlog::detail::QueueWriter w(q);
  std::size_t totalWriteSize = 0;
  std::size_t failedWriteCount = 0;

  while (state.KeepRunning())
  {
    if (w.beginWrite(buf.size()))
    {
      w.writeBuffer(buf.data(), buf.size());
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

BENCHMARK(BM_queueWrite)->Range(8, 1024); // NOLINT

} // namespace

BENCHMARK_MAIN();
