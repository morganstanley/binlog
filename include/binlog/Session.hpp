#ifndef BINLOG_SESSION_HPP
#define BINLOG_SESSION_HPP

#include <binlog/Entries.hpp>
#include <binlog/Severity.hpp>
#include <binlog/Time.hpp>
#include <binlog/detail/Queue.hpp>
#include <binlog/detail/QueueReader.hpp>
#include <binlog/detail/VectorOutputStream.hpp>

#include <algorithm> // remove_if
#include <atomic>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <utility> // move
#include <vector>

namespace binlog {

/**
 * A concurrently writable and readable log stream.
 *
 * Session manages metadata (event sources, clock sync),
 * and data (log events). Members of this class
 * are thread-safe.
 *
 * Writers can add event sources and events.
 * Event sources are added directly, safe concurrent
 * access is ensured by a mutex.
 * Events can be added parallel, via Channels.
 * Channels wrap a single producer, lockfree queue.
 * The channel interface is raw, log events should be
 * added using SessionWriter.
 *
 * Readers can read metadata and data, via consume.
 * Concurrent reads are serialized by a mutex.
 *
 * Session responsibilities:
 *  - Assign unique ids to event sources
 *  - Add clock syncs to the stream when needed
 *  - Own data channels (lifetime management)
 *  - Ensure proper ordering of metadata and data,
 *    as observed by readers
 */
class Session
{
public:
  struct Channel
  {
    explicit Channel(Session& session, std::size_t queueCapacity, WriterProp writerProp_ = {});
    ~Channel();

    Channel(const Channel&) = delete;
    void operator=(const Channel&) = delete;

    Channel(Channel&&) = delete;
    void operator=(Channel&&) = delete;

    detail::Queue& queue();

    WriterProp writerProp;      /**< Describes the writer of this channel (optional) */ // NOLINT

  private:
    std::unique_ptr<char[]> _queue; /**< Magic, Queue, and the underlying buffer of `queue` */
  };

  /** Describe the result of a consume call */
  struct ConsumeResult
  {
    std::size_t bytesConsumed = 0;      /**< Number of bytes written to the output stream by this call */
    std::size_t totalBytesConsumed = 0; /**< Total number of bytes written to the output stream in the lifetime of this session */
    std::size_t channelsPolled = 0;     /**< Number of channels polled to get log data from */
    std::size_t channelsRemoved = 0;    /**< Number of channels removed because they are empty and closed */
  };

  Session();

  /**
   * Create a channel with a queue of `queueCapacity` bytes.
   *
   * Session retains partial ownership of the created channel.
   * The channel is disposed when this ownership becomes exclusive
   * (i.e: there are no more outstanding shared pointers)
   * and the channel is empty - by the next `consume` call.
   *
   * @return a shared pointer to the created channel
   */
  std::shared_ptr<Channel> createChannel(std::size_t queueCapacity, WriterProp writerProp = {});

  /**
   * Thread-safe way to set the writer id of `channel` to `id`.
   *
   * @pre `channel` must be owned by *this
   * @post channel.writerProp.id == id
   */
  void setChannelWriterId(Channel& channel, std::uint64_t id);

  /**
   * Thread-safe way to set the writer name of `channel` to `name`.
   *
   * @pre `channel` must be owned by *this
   * @post channel.writerProp.name == name
   */
  void setChannelWriterName(Channel& channel, std::string name);

  /**
   * Add `eventSource` to the set of metadata managed by this session.
   *
   * The returned id can be used by event producers to
   * reference `eventSource` later in the stream.
   *
   * Events created after the addition of an EventSource
   * (addEventSource happens before addEvent)
   * are guaranteed to be consumed after the event source
   * by `Session::consume`.
   *
   * @returns the id assigned to the added event source
   */
  std::uint64_t addEventSource(EventSource eventSource);

  /** @returns Severity below writers should not add events */
  Severity minSeverity() const;

  /**
   * Set minimum severity new added events.
   *
   * This is advisory only: writers are encouraged
   * not to add new events with severity below the given limit,
   * but not required to.
   */
  void setMinSeverity(Severity severity);

  /**
   * Add `clockSync` to the set of managed metadata.
   *
   * Affects Events consumed after this call.
   * Overwrites previously set, and the default ClockSync.
   */
  void setClockSync(const ClockSync& clockSync);

  /**
   * Move metadata and data from the session to `out`.
   *
   * If needed (i.e: first time to consume), a
   * ClockSync is consumed, which describes std::chrono::system_clock.
   *
   * Then, metadata (EventSources) are consumed.
   * The consume logic makes sure sources are always consumed
   * sooner than events referencing them.
   *
   * After that, each channel is polled for log data,
   * and consumed together with an WriterProp entry, if data is found.
   * Closed and empty channels are removed.
   * Because data is consumed in batches, it is possible
   * that concurrently added events consumed from different channels
   * appear out of order. Events consumed from a single channel
   * are always in order.
   *
   * It is guaranteed that `out.write` always receives a sequence
   * of complete entries - no partial entries are written.
   *
   * @requires OutputStream must model the mserialize::OutputStream concept
   * @param out where the binary data will be written to.
   *
   * @returns description of the job done, see ConsumeResult.
   */
  template <typename OutputStream>
  ConsumeResult consume(OutputStream& out);

  /**
   * Move already consumed metadata again to `out`.
   *
   * Already consumed EventSources and the ClockSync are consumed.
   * Not-yet consumed EventSources will not be consumed.
   *
   * Useful if `out` changes runtime, e.g: because of log rotation.
   * Re-adding metadata makes the new logfile self contained.
   *
   * @requires OutputStream must model the mserialize::OutputStream concept
   * @param out where the binary data will be written to.
   *
   * @returns description of the job done, see ConsumeResult.
   */
  template <typename OutputStream>
  ConsumeResult reconsumeMetadata(OutputStream& out);

private:
  template <typename Entry, typename OutputStream>
  std::size_t consumeSpecialEntry(const Entry& entry, OutputStream& out);

  std::mutex _mutex;

  std::vector<std::shared_ptr<Channel>> _channels;
  detail::RecoverableVectorOutputStream _clockSync = {0xFE214F726E35BDBC, this};
  detail::RecoverableVectorOutputStream _sources = {0xFE214F726E35BDBC, this};
  std::streamsize _sourcesConsumePos = 0;
  std::uint64_t _nextSourceId = 1;

  std::size_t _totalConsumedBytes = 0;

  std::atomic<Severity> _minSeverity = {Severity::trace};

  bool _consumeClockSync = true;

  detail::VectorOutputStream _specialEntryBuffer;
};

inline Session::Channel::Channel(Session& session, std::size_t queueCapacity, WriterProp writerProp_)
  :writerProp(std::move(writerProp_)),
   _queue(new char[sizeof(std::uint64_t) + sizeof(Session*) + sizeof(detail::Queue) + queueCapacity])
{
  // To be able to recover unconsumed queue data from memory dumps,
  // put a magic number, a pointer to the owning session, the queue and the queue buffer
  // next to each other.
  char* buffer = _queue.get();

  // The magic number is used to indentify the queue in the memory dump
  new (buffer) std::uint64_t(0xFE213F716D34BCBC);
  buffer += sizeof(std::uint64_t);

  // Session* is used to separate the queues of different sessions of the program
  new (buffer) Session*(&session);
  buffer += sizeof(Session*);

  // Queue is used normally to store log events, and also during recovery,
  // to determine the unconsumed parts of the buffer.
  static_assert(alignof(detail::Queue) <= alignof(std::uint64_t), "");
  static_assert(alignof(detail::Queue) <= alignof(Session*), "");
  char* queueBuffer = buffer + sizeof(detail::Queue);
  new (buffer) detail::Queue(queueBuffer, queueCapacity);
}

inline Session::Channel::~Channel()
{
  // clear magic number - do not recover invalid data
  std::uint64_t magic = 0;
  memcpy(_queue.get(), &magic, sizeof(magic));

  // destroy queue
  queue().~Queue();
}

inline detail::Queue& Session::Channel::queue()
{
  return *reinterpret_cast<detail::Queue*>(_queue.get() + sizeof(std::uint64_t) + sizeof(Session*));
}

inline Session::Session()
{
  const ClockSync clockSync = systemClockSync();
  serializeSizePrefixedTagged(clockSync, _clockSync);
}

inline std::shared_ptr<Session::Channel> Session::createChannel(std::size_t queueCapacity, WriterProp writerProp)
{
  std::lock_guard<std::mutex> lock(_mutex);

  _channels.push_back(std::make_shared<Channel>(*this, queueCapacity, std::move(writerProp)));
  return _channels.back();
}

inline void Session::setChannelWriterId(Channel& channel, std::uint64_t id)
{
  std::lock_guard<std::mutex> lock(_mutex);

  channel.writerProp.id = id;
}

inline void Session::setChannelWriterName(Channel& channel, std::string name)
{
  std::lock_guard<std::mutex> lock(_mutex);

  channel.writerProp.name = std::move(name);
}

inline std::uint64_t Session::addEventSource(EventSource eventSource)
{
  std::lock_guard<std::mutex> lock(_mutex);

  eventSource.id = _nextSourceId;
  serializeSizePrefixedTagged(eventSource, _sources);
  return _nextSourceId++;
}

inline Severity Session::minSeverity() const
{
  return _minSeverity.load(std::memory_order_acquire);
}

inline void Session::setMinSeverity(Severity severity)
{
  _minSeverity.store(severity, std::memory_order_release);
}

inline void Session::setClockSync(const ClockSync& clockSync)
{
  std::lock_guard<std::mutex> lock(_mutex);

  serializeSizePrefixedTagged(clockSync, _clockSync);
  _consumeClockSync = true;
}

template <typename OutputStream>
Session::ConsumeResult Session::consume(OutputStream& out)
{
  // This lock:
  //  - Ensures only a single consumer is running at a time
  //  - Ensures safe read of _channels
  //  - Ensures safe read of Channel::writerProp (written by setChannelWriterName)
  //  - Ensures safe read of _sources
  //  - Ensures no new EventSource can be added to _sources while consuming
  //
  // Without this lock, the following becomes possible:
  //  - Consumer starts, consumes _sources
  //  - Producer1 adds a new elem (ES123) to sources
  //  - Producer2 finds that ES123 is already added
  //  - Producer2 adds a new event using ES123
  //  - Consumer continues, consumes the event added by Producer2
  // This would result in a corrupt stream, as the event source
  // must precede every event referencing it. This is solved by the lock
  // that blocks P1 *and* P2 while adding ES123.
  std::lock_guard<std::mutex> lock(_mutex);

  ConsumeResult result;

  // add a clock sync if not yet added
  if (_consumeClockSync)
  {
    out.write(_clockSync.data(), _clockSync.ssize());
    result.bytesConsumed += std::size_t(_clockSync.ssize());
    _consumeClockSync = false;
  }

  // consume event sources before events
  const std::streamsize sourceWriteSize = _sources.ssize() - _sourcesConsumePos;
  out.write(_sources.data() + _sourcesConsumePos, sourceWriteSize);
  _sourcesConsumePos += sourceWriteSize;
  result.bytesConsumed += std::size_t(sourceWriteSize);

  // consume some events
  for (std::shared_ptr<Channel>& channelptr : _channels)
  {
    // Important to check if channel is closed before beginRead,
    // otherwise the following race becomes possible:
    //  - Consumer finds queue is empty
    //  - Producer adds data
    //  - Producer closes the queue
    //  - Consumer finds queue is closed, removes it -> data loss
    const bool isClosed = (channelptr.use_count() == 1);

    Channel& ch = *channelptr;

    detail::QueueReader reader(ch.queue());
    const detail::QueueReader::ReadResult data = reader.beginRead();
    if (data.size())
    {
      // consume writerProp entry
      ch.writerProp.batchSize = data.size();
      result.bytesConsumed += consumeSpecialEntry(ch.writerProp, out);

      // consume queue data
      out.write(data.buffer1, std::streamsize(data.size1));
      if (data.size2)
      {
        // data wraps around the end of the queue, consume the second half as well
        out.write(data.buffer2, std::streamsize(data.size2));
      }

      reader.endRead();
      result.bytesConsumed += data.size();
    }

    if (isClosed)
    {
      // queue is empty and closed, remove it
      channelptr.reset();
      result.channelsRemoved++;
    }

    result.channelsPolled++;
  }

  // remove empty and closed channels
  _channels.erase(
    std::remove_if(
      _channels.begin(), _channels.end(),
      [](const std::shared_ptr<Channel>& channelptr) { return !channelptr; }
    ),
    _channels.end()
  );

  _totalConsumedBytes += result.bytesConsumed;
  result.totalBytesConsumed = _totalConsumedBytes;

  return result;
}

template <typename OutputStream>
Session::ConsumeResult Session::reconsumeMetadata(OutputStream& out)
{
  std::lock_guard<std::mutex> lock(_mutex);

  ConsumeResult result;

  // add clock sync
  out.write(_clockSync.data(), _clockSync.ssize());
  result.bytesConsumed += std::size_t(_clockSync.ssize());

  // add consumed sources
  out.write(_sources.data(), _sourcesConsumePos);
  result.bytesConsumed += std::size_t(_sourcesConsumePos);

  _totalConsumedBytes += result.bytesConsumed;
  result.totalBytesConsumed = _totalConsumedBytes;
  return result;
}

template <typename Entry, typename OutputStream>
std::size_t Session::consumeSpecialEntry(const Entry& entry, OutputStream& out)
{
  // Write entry to `_specialEntryBuffer` first, only then to `out` in one go.
  // This makes OutputStream logic simpler (if it parses the stream),
  // as it does not have to deal with partial entries.
  // (serializeSizePrefixedTagged serializes Entry field by field)
  // This is also more efficient if OutputStream does unbuffered I/O.
  _specialEntryBuffer.clear();
  const std::size_t size = serializeSizePrefixedTagged(entry, _specialEntryBuffer);
  out.write(_specialEntryBuffer.data(), _specialEntryBuffer.ssize());
  return size;
}

} // namespace binlog

#endif // BINLOG_SESSION_HPP
