#ifndef BINLOG_DETAIL_QUEUE_HPP
#define BINLOG_DETAIL_QUEUE_HPP

#include <atomic>
#include <cstddef>

namespace binlog {
namespace detail {

/**
 * A single producer, single consumer, concurrent queue of bytes.
 *
 * After the Queue is constructed, it can be written by
 * a QueueWriter, and read by a QueueReader, possibly
 * concurrently (i.e: by different threads).
 *
 * A differentiating feature of this Queue,
 * (e.g: compared to boost::lockfree::spsc_queue)
 * is that it allows efficient, consistent
 * write and read of batches of bytes:
 *
 *    char buffer[1024];
 *    Queue q(buffer, sizeof(buffer)); // queue of 1024 bytes
 *
 *    QueueWriter w(q);
 *    if (w.beginWrite(32))
 *    {
 *      // 32 contiguous bytes are available for writing
 *      w.writeBuffer(buf1, 16);
 *      w.writeBuffer(buf2, 16);
 *      w.endWrite(); // until this point, the writes above are not observable by the reader
 *    }
 *    // else: queue doesn't have 32 contiguous free bytes
 *
 *    QueueReader r(q);
 *    const QueueReader::ReadResult rr = r.beginRead();
 *    if (rr.size1)
 *    {
 *      consume(rr.buffer1, rr.size1);
 *      consume(rr.buffer2, rr.size2);
 *      r.endRead(); // make consumed bytes available for writing
 *    }
 *    // else: queue is empty
 *
 * Queue does not manage the lifetime of the underlying buffer,
 * to allow fine grained buffer placement.
 *
 * Internally, the Queue maintains 3 shared indices:
 *  - W: next index to write
 *  - R: next index to read
 *  - E: end of readable data
 *
 * Invariants are:
 *  - Queue is empty if R == W
 *  - Queue is full if (W+1 == R) or (W == capacity() and R == 0)
 *
 * The bytes in the queue are either empty (.) or written (#).
 *
 * Initially, the queue is empty:
 *
 *     W
 *    [...............]
 *     R
 *
 * A write is completed by moving W:
 *
 *                 W
 *    [############...]
 *     R
 *
 * At this point, the written data is available to the reader.
 * The written area is freed up after reading:
 *
 *                 W
 *    [...............]
 *                 R
 *
 * The queue is now empty again.
 * If the writer begins a new write, which size
 * exceeds the free space 'to the right', but
 * would fit 'on the left', W is moved to the beginning
 * of the queue, leaving E at the end, to mark
 * the end of valid data:
 *
 *     W           E
 *    [...............]
 *                 R
 *
 * The writer continues writing the beginning of the queue.
 * The reader will notice that E is reached, so it will
 * also wrap around, starting again from the beginning.
 */
class Queue
{
public:
  /**
   * Construct a queue using the provided `buffer`.
   *
   * @pre [buffer,buffer+capacity) must be valid
   */
  explicit Queue(char* buffer, std::size_t capacity)
    :_writeIndex(0),
     _dataEnd(0),
     _capacity(capacity),
     _buffer(buffer),
     _readIndex(0)
  {}

  /** @returns the maximum number of bytes the queue can store */
  std::size_t capacity() const { return _capacity; }

private:
  friend class QueueWriter;
  friend class QueueReader;

  char*       buffer()       { return _buffer; }
  const char* buffer() const { return _buffer; }

  // members written by Writer
  std::atomic<std::size_t> _writeIndex; /**< Next index to write */
  std::size_t _dataEnd;                 /**< No valid data after this index */
  std::size_t _capacity;                /**< Buffer size */
  char* _buffer;                        /**< Unmanaged underlying buffer */

  // members written by Reader
  std::atomic<std::size_t> _readIndex;  /**< Next index to read */
};

} // namespace detail
} // namespace binlog

#endif // BINLOG_DETAIL_QUEUE_HPP
