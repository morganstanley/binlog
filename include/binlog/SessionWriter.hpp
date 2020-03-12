#ifndef BINLOG_SESSION_WRITER_HPP
#define BINLOG_SESSION_WRITER_HPP

#include <binlog/Session.hpp>
#include <binlog/detail/QueueWriter.hpp>

#include <mserialize/serialize.hpp>

#include <algorithm> // max
#include <cstddef>
#include <memory> // shared_ptr
#include <utility> // move

namespace binlog {

/**
 * Add events to a Session Channel.
 *
 * It wraps the single-producer Channel of a Session
 * and exports an interface suitable to add log
 * events to the channel, without explicit
 * channel lifetime and concurrency management.
 */
class SessionWriter
{
public:
  /**
   * Construct a SessionWriter attached to `session`.
   *
   * Creates a session channel internally.
   *
   * @param queueCapacity capacity in bytes of the channels queue
   * @param id see setId
   * @param name see setName
   */
  explicit SessionWriter(Session& session, std::size_t queueCapacity = 1 << 20, std::uint64_t id = {}, std::string name = {});

  /** Marks the underlying channel closed. */
  ~SessionWriter() = default;

  SessionWriter(const SessionWriter&) = delete;
  SessionWriter& operator=(const SessionWriter&) = delete;

  // Moved-from objects can be assigned to or destructed
  SessionWriter(SessionWriter&& rhs) noexcept = default;
  SessionWriter& operator=(SessionWriter&& rhs) noexcept = default;

  /** @return a reference to the session it is attached to */
  Session& session() { return *_session; }

  /**
   * Set the writer id of the underlying channel.
   *
   * The writer id is shown when produced events are
   * pretty printed with "%t", see PrettyPrinter.
   *
   * This call takes effect on events produced by
   * this writer only, including events already produced but
   * not yet consumed, and yet to be consumed events.
   * Does not affect already consumed events.
   *
   * Can be called concurrently with other
   * writer and session methods (most notably: consume)
   */
  void setId(std::uint64_t id);

  /**
   * Set the writer name of the underlying channel.
   *
   * The writer name is shown when produced events are
   * pretty printed with "%n", see PrettyPrinter.
   *
   * This call takes effect on events produced by
   * this writer only, including events already produced but
   * not yet consumed, and yet to be consumed events.
   * Does not affect already consumed events.
   *
   * Can be called concurrently with other
   * writer and session methods (most notably: consume)
   */
  void setName(std::string name);

  /**
   * Add a log event to the queue of the underlying channel.
   *
   * First, it computes the serialized size of the event,
   * then allocates memory in the queue, then
   * serializes the event, and finally commits the write.
   *
   * As size is computed separately from serialization,
   * some getters of serializable types (e.g: which return a string)
   * will be called twice. It is important to always
   * return the same value during a single log call,
   * otherwise undefined behaviour might be invoked.
   *
   * If the queue is full (it has not enough space for the event),
   * a new channel is created, suitable to hold this event,
   * and the old one is closed.
   *
   * @pre `eventSourceId` must be the id of an event source added to `session()`,
   *      see Session::addEventSource.
   * @param eventSourceId The id of the source which produces the event
   * @param clock Value of the clock (specified by ClockSync),
   *        when the event was created (now).
   * @param args Log arguments, which substitute {} placeholders of
   *        the event source format string when pretty printed.
   *        mserialize tag of `Args` must match `argumentTags` of the event source.
   *
   * @returns true on success, false if there's not enough space in the queue
   *          and failed to create a new channel.
   */
  template <typename... Args>
  bool addEvent(std::uint64_t eventSourceId, std::uint64_t clock, Args&&... args) noexcept;

private:
  bool replaceChannel(std::size_t minQueueCapacity) noexcept;

  Session* _session;
  std::shared_ptr<Session::Channel> _channel;
  detail::QueueWriter _qw;
};

inline SessionWriter::SessionWriter(Session& session, std::size_t queueCapacity, std::uint64_t id, std::string name)
  :_session(& session),
   _channel(session.createChannel(queueCapacity)),
   _qw(_channel->queue())
{
  if (id != 0) { setId(id); }
  if (! name.empty()) { setName(std::move(name)); }
}

inline void SessionWriter::setId(std::uint64_t id)
{
  _session->setChannelWriterId(*_channel, id);
}

inline void SessionWriter::setName(std::string name)
{
  _session->setChannelWriterName(*_channel, std::move(name));
}

template <typename... Args>
bool SessionWriter::addEvent(std::uint64_t eventSourceId, std::uint64_t clock, Args&&... args) noexcept
{
  // compute size (excludes size field)
  std::size_t size = 0;
  const std::size_t sizes[] = {sizeof(eventSourceId), sizeof(clock), mserialize::serialized_size(args)...};
  for (auto s : sizes) { size += s; }

  // allocate space (totalSize includes size field)
  const std::size_t totalSize = size + sizeof(std::uint32_t);
  if (! _qw.beginWrite(totalSize))
  {
    // not enough space in queue, create a new channel
    replaceChannel(totalSize);
    if (! _qw.beginWrite(totalSize)) { return false; }
  }

  // serialize fields
  using swallow = int[];
  (void)swallow{
    (mserialize::serialize(std::uint32_t(size), _qw), int{}),
    (mserialize::serialize(eventSourceId, _qw), int{}),
    (mserialize::serialize(clock, _qw), int{}),
    (mserialize::serialize(args, _qw), int{})...
  };

  _qw.endWrite();
  return true;
}

inline bool SessionWriter::replaceChannel(std::size_t minQueueCapacity) noexcept
{
  const std::size_t newCapacity = (std::max)(_qw.capacity(), 2 * minQueueCapacity);

  try
  {
    WriterProp wp{_channel->writerProp.id, _channel->writerProp.name, 0}; // avoid racing on the last field
    _channel = _session->createChannel(newCapacity, std::move(wp));
    _qw = detail::QueueWriter(_channel->queue());
  }
  catch (...)
  {
    // allocation and mutex lock in createChannel can throw,
    // but addEvent is more efficient if noexcept:
    // indicate failure by return value instead.
    return false;
  }

  return true;
}

} // namespace binlog

#endif // BINLOG_SESSION_WRITER_HPP
