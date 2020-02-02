#ifndef BINLOG_ENTRY_STREAM_HPP
#define BINLOG_ENTRY_STREAM_HPP

#include <binlog/Range.hpp>

#include <iosfwd>
#include <vector>

namespace binlog {

/**
 * Interface of extracting entries from an underlying device.
 *
 * @see Entries.hpp
 */
class EntryStream
{
public:
  virtual ~EntryStream() = default;

  /**
   * Binlog entries start with 32 byte size,
   * followed by `size` number of bytes, the payload.
   *
   * @return the payload part of the next entry
   *         stored in the underlying device,
   *         or an empty range, if there are no more bytes.
   *
   * @throw std::runtime_error if the underlying device is not empty
   *         but a complete entry cannot be read.
   */
  virtual Range nextEntryPayload() = 0;
};

/** Entry stream with a std::istream as the underlying device */
class IstreamEntryStream : public EntryStream
{
public:
  /**
   * Stores a reference to `input`: it must remain valid
   * as long as *this is valid
   */
  explicit IstreamEntryStream(std::istream& input);

  /**
   * @see EntryStream::nextEntryPayload
   *
   * On error, input.tellg() remains unchanged.
   */
  Range nextEntryPayload() override;

private:
  void rewind(std::streamsize size);

  std::vector<char> _buffer; // TODO(benedek) perf: avoid double buffering
  std::istream& _input;
};

/** Entry stream with a Range (sequence of bytes) as the underlying device */
class RangeEntryStream : public EntryStream
{
public:
  /**
   * The buffer referenced by `input` must remain valid
   * as long as *this is valid.
   */
  explicit RangeEntryStream(Range input);

  /**
   * @see EntryStream::nextEntryPayload
   *
   * On error, *this remains in an unspecified but valid state.
   */
  Range nextEntryPayload() override;

private:
  Range _input;
};

} // namespace binlog

#endif // BINLOG_ENTRY_STREAM_HPP
