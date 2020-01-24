#ifndef BINLOG_EVENT_FILTER_HPP
#define BINLOG_EVENT_FILTER_HPP

#include <binlog/Entries.hpp> // EventSource
#include <binlog/Range.hpp>

#include <mserialize/deserialize.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <ios> // streamsize
#include <set>
#include <stdexcept> // runtime_error
#include <utility> // move

namespace binlog {

/**
 * From a stream of entries, pass through events produced by
 * event sources selected by a user specified predicate.
 */
class EventFilter
{
public:
  using Predicate = std::function<bool(const EventSource&)>;

  /** @param isAllowed should return true for allowed EventSources */
  explicit EventFilter(Predicate isAllowed);

  /**
   * From the sequence of entries in [buffer, buffer+bufferSize),
   * write special entries and events produced allowed EventSources
   * to `out`.
   *
   * Each entry in the input is inspected:
   *  - special entries are written to `out` unconditionally
   *  - EventSources are categorized by the Predicate given in the constructor
   *    as either allowed or disallowed sources.
   *  - Events are written to `out` only if produced by allowed EventSources.
   *
   * The predicate is invoked for each EventSource, but not for Events.
   * Only EventSources are deserialized, other entries are categorized by their tags.
   *
   * @requires OutputStream must model the mserialize::OutputStream concept
   * @throws std::runtime_error if `buffer` contains an invalid entry
   */
  template <typename OutputStream>
  std::size_t writeAllowed(const char* buffer, std::size_t bufferSize, OutputStream& out);

private:
  Predicate _isAllowed;
  std::set<std::uint64_t> _allowedSourceIds; // TODO(benedek) perf: use a more efficient set
};

inline EventFilter::EventFilter(Predicate isAllowed)
  :_isAllowed(std::move(isAllowed))
{}

template <typename OutputStream>
std::size_t EventFilter::writeAllowed(const char* buffer, std::size_t bufferSize, OutputStream& out)
{
  std::size_t totalWriteSize = 0;

  Range entries(buffer, bufferSize);

  while (! entries.empty())
  {
    Range entry = entries;
    const std::uint32_t size = entries.read<std::uint32_t>();
    Range payload(entries.view(size), size);
    const std::uint64_t tag = payload.read<std::uint64_t>();
    const bool special = (tag & (std::uint64_t(1) << 63)) != 0;

    if (special)
    {
      // event sources are inspected to populate _allowedSourceIds
      if (tag == EventSource::Tag)
      {
        EventSource eventSource;
        mserialize::deserialize(eventSource, payload);
        if (_isAllowed(eventSource))
        {
          _allowedSourceIds.insert(eventSource.id);
        }
        // else: event source is not allowed, events referencing it
        // will not be written.
      }
    }
    else if (_allowedSourceIds.count(tag) == 0)
    {
      // event is produced by a disallowed source, ignore it
      continue;
    }

    // either special entry or event produced by an allowed source, write it
    // TODO(benedek) perf: batch contiguous entries into one out.write
    const std::size_t sizePrefixedSize = size + sizeof(size);
    out.write(entry.view(sizePrefixedSize), std::streamsize(sizePrefixedSize));
    totalWriteSize += sizePrefixedSize;
  }

  return totalWriteSize;
}

} // namespace binlog

#endif // BINLOG_EVENT_FILTER_HPP
