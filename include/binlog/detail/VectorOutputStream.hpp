#ifndef BINLOG_DETAIL_VECTOR_OUTPUT_STREAM_HPP
#define BINLOG_DETAIL_VECTOR_OUTPUT_STREAM_HPP

#include <cstdint>
#include <cstring>
#include <ios> // streamsize
#include <vector>

namespace binlog {
namespace detail {

/** Adapts std::vector to mserialize::OutputStream */
struct VectorOutputStream
{
  std::vector<char> vector;

  VectorOutputStream& write(const char* buffer, std::streamsize size)
  {
    vector.insert(vector.end(), buffer, buffer + size);
    return *this;
  }

  void clear() { vector.clear(); }
  const char* data() const { return vector.data(); }
  std::streamsize ssize() const { return std::streamsize(vector.size()); }
};

/**
 * Like a VectorOutputStream, but with additional
 * data prefixed to the data, that allows the recovery
 * of the data from a memory dump.
 *
 * Internal layout:
 *
 *     [u64 magic|ptr id|u64 size][...size bytes of data...]
 *
 * `magic` is used to identify the object in the memory dump.
 * `id` can be used to correlate the object to others.
 * `size` the number of valid bytes following it.
 *
 * @models mserialize::OutputStream
 */
class RecoverableVectorOutputStream
{
  static constexpr std::size_t HeaderSize =
    sizeof(std::uint64_t) + sizeof(void*) + sizeof(std::uint64_t);

  static constexpr std::size_t SizeOffset =
    sizeof(std::uint64_t) + sizeof(void*);

public:
  RecoverableVectorOutputStream(std::uint64_t magic, void* id)
  {
    _vector.resize(HeaderSize);
    char* buffer = _vector.data();

    memcpy(buffer, &magic, sizeof(magic));
    buffer += sizeof(magic);

    memcpy(buffer, &id, sizeof(id));
    buffer += sizeof(id);

    new (buffer) std::uint64_t{0}; // size
  }

  ~RecoverableVectorOutputStream()
  {
    // do not recover invalid data from destroyed objects
    clearMagic();
  }

  RecoverableVectorOutputStream(const RecoverableVectorOutputStream&) = delete;
  void operator=(const RecoverableVectorOutputStream&) = delete;

  RecoverableVectorOutputStream(RecoverableVectorOutputStream&&) = default;
  RecoverableVectorOutputStream& operator=(RecoverableVectorOutputStream&&) = default;

  RecoverableVectorOutputStream& write(const char* buffer, std::streamsize size)
  {
    std::uint64_t magic = 0;
    if (_vector.capacity() < _vector.size() + std::size_t(size))
    {
      // vector will reallocate, clear the magic of the old buffer
      // to avoid recovering invalid data
      magic = clearMagic();
    }

    _vector.insert(_vector.end(), buffer, buffer + size);
    if (magic != 0) { setMagic(magic); }
    updateSize();
    return *this;
  }

  const char* data() const
  {
    return _vector.data() + HeaderSize;
  }

  std::size_t size() const
  {
    return _vector.size() - HeaderSize;
  }

  std::streamsize ssize() const
  {
    return std::streamsize(size());
  }

private:
  void updateSize()
  {
    const std::uint64_t sz = size();
    memcpy(_vector.data() + SizeOffset, &sz, sizeof(sz));
  }

  void setMagic(std::uint64_t magic)
  {
    memcpy(_vector.data(), &magic, sizeof(magic));
  }

  std::uint64_t clearMagic()
  {
    std::uint64_t magic = 0;
    memcpy(&magic, _vector.data(), sizeof(magic));
    setMagic(0);
    return magic;
  }

  std::vector<char> _vector;
};

} // namespace detail
} // namespace binlog

#endif // BINLOG_DETAIL_VECTOR_OUTPUT_STREAM_HPP
