#ifndef BINLOG_DETAIL_QUEUE_WRITER_HPP
#define BINLOG_DETAIL_QUEUE_WRITER_HPP

#include <binlog/detail/Queue.hpp>

#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstring> // memcpy
#include <ios> // streamsize

namespace binlog {
namespace detail {

/**
 * @see Queue
 *
 * Models the mserialize::OutputStream concept
 */
class QueueWriter
{
public:
  explicit QueueWriter(Queue& q)
    :_queue(&q),
     _writePos(buffer()),
     _writeEnd(buffer())
  {}

  /** @returns the maximum number of bytes the queue can store */
  std::size_t capacity() const { return _queue->capacity; }

  /** @returns the number of bytes currently available for write */
  std::size_t writeCapacity() const
  {
    return std::size_t(_writeEnd - _writePos);
  }

  /** @returns the number of committed bytes not yet consumed by the reader */
  std::size_t unreadWriteSize() const
  {
    const std::size_t w = _queue->writeIndex.load(std::memory_order_relaxed);
    const std::size_t r = _queue->readIndex.load(std::memory_order_acquire);

    return (r <= w)
      ? std::size_t(w - r)
      : std::size_t(_queue->dataEnd - r + w);
  }

  /**
   * Attempt to make writeCapacity() >= `size`.
   *
   * Possibly resets the internal write buffer,
   * in that case, uncommitted writes
   * (those without a subsequent endWrite())
   * will be lost.
   *
   * @returns `size` <= writeCapacity()
   */
  bool beginWrite(std::size_t size)
  {
    return (size <= writeCapacity())
      ? true
      : size <= maximizeWriteCapacity();
  }

  /**
   * Copy the range [src,src+size) to the internal write buffer.
   *
   * @pre writeCapacity() >= `size`
   * @post writeCapacity() -= `size`
   * @returns A pointer to the bitwise copy of the range,
   * which pointer remains valid until the next `beginWrite()` or `endWrite()`.
   */
  void* writeBuffer(const void* src, std::size_t size)
  {
    assert(_writePos + size <= _writeEnd);

    void* result = memcpy(_writePos, src, size);
    _writePos += size;
    return result;
  }

  /** Same as writeBuffer(src, size) */
  void* write(const void* src, std::streamsize size)
  {
    return writeBuffer(src, std::size_t(size));
  }

  /** Make the written parts of the internal buffer available to read. */
  void endWrite()
  {
    const std::size_t newW = std::size_t(_writePos - buffer());
    _queue->writeIndex.store(newW, std::memory_order_release);
  }

private:
  /**
   * Maximize writeCapacity() by selecting the largest contiguous writable arena.
   *
   * Resets the internal write buffer,
   * uncommitted writes (those without a subsequent endWrite())
   * will be lost.
   *
   * @post writeCapacity() == sizeof max contiguous writable arena
   * @returns writeCapacity()
   */
  std::size_t maximizeWriteCapacity()
  {
    const std::size_t w = _queue->writeIndex.load(std::memory_order_relaxed);
    const std::size_t r = _queue->readIndex.load(std::memory_order_acquire);

    if (w < r) // [####W.....R###E..]
    {
      _writePos = buffer() + w;
      _writeEnd = buffer() + r - 1;
    }
    else // [...R###W......]
    {
      const std::int64_t rightSize = std::int64_t(_queue->capacity - w);
      const std::int64_t leftSize = std::int64_t(r) - 1;

      if (rightSize >= leftSize)
      {
        _writePos = buffer() + w;
        _writeEnd = buffer() + w + rightSize;
      }
      else
      {
        _queue->dataEnd = w;
        _writePos = buffer();
        _writeEnd = buffer() + leftSize;
      }
    }

    return writeCapacity();
  }

  char* buffer() { return _queue->buffer; }

  Queue* _queue;

  char* _writePos;
  char* _writeEnd;
};

} // namespace detail
} // namespace binlog

#endif // BINLOG_DETAIL_QUEUE_WRITER_HPP
