#ifndef BINLOG_DETAIL_QUEUE_READER_HPP
#define BINLOG_DETAIL_QUEUE_READER_HPP

#include <binlog/detail/Queue.hpp>

#include <atomic>
#include <cstring> // memcpy

namespace binlog {
namespace detail {

/** @see Queue */
class QueueReader
{
public:
  struct ReadResult
  {
    std::size_t size() const { return size1 + size2; }

    const char* buffer1 = nullptr;
    std::size_t size1 = 0;
    const char* buffer2 = nullptr;
    std::size_t size2 = 0;
  };

  explicit QueueReader(Queue& q)
    :_queue(&q)
  {}

  /** @returns the maximum number of bytes the queue can store */
  std::size_t capacity() const { return _queue->capacity; }

  /**
   * Access the currently readable parts of the queue.
   *
   * As it is possible, that the readable parts
   * wrap around the end of the buffer,
   * this method returns two buffers.
   * If the first buffer is empty, the queue was empty,
   * if the second part is empty, there was no wrap-around.
   *
   * @returns A two-buffer view of the readable data
   */
  ReadResult beginRead()
  {
    const std::size_t w = _queue->writeIndex.load(std::memory_order_acquire);
    const std::size_t r = _queue->readIndex.load(std::memory_order_relaxed);

    _readEnd = w;

    if (r <= w)  // [...R######W...]
    {
      return ReadResult{buffer() + r, w - r, nullptr, 0};
    }

    if (r < _queue->dataEnd)  // [###W...R###E..]
    {
      return ReadResult{
        buffer() + r, _queue->dataEnd - r,
        buffer(), w
      };
    }

    // [###W......RE..]
    return ReadResult{buffer(), w, nullptr, 0};
  }

  /** Make the consumed parts of the internal buffer available to write. */
  void endRead()
  {
    _queue->readIndex.store(_readEnd, std::memory_order_release);
  }

private:
  char* buffer() { return _queue->buffer; }

  Queue* _queue;
  std::size_t _readEnd = 0;
};

} // namespace detail
} // namespace binlog

#endif // BINLOG_DETAIL_QUEUE_READER_HPP
