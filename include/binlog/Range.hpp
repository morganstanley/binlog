#ifndef BINLOG_RANGE_HPP
#define BINLOG_RANGE_HPP

#include <cstddef> // size_t, nullptr
#include <cstring> // memcpy
#include <ios> // streamsize
#include <stdexcept> // runtime_error
#include <string>
#include <type_traits> // is_trivially_copyable

namespace binlog {

/**
 * A view to a contiguous buffer of bytes.
 *
 * Does not own the underlying data.
 * Cheap to copy.
 * Provides convenience read() members to copy
 * data from the viewed buffer.
 * Models the mserialize::InputStream concept.
 */
class Range
{
public:
  /** @post empty() */
  Range() = default;

  /**
   * Create a new range representing the bytes
   * at [begin, end).
   */
  Range(const char* begin, const char* end)
    :_begin(begin),
     _end(end)
  {}

  /**
   * Create a new range representing the bytes
   * at [begin, begin+size).
   */
  Range(const char* begin, std::size_t size)
    :_begin(begin),
     _end(begin + size)
  {}

  /** @returns size() == 0 */
  bool empty() const { return _begin == _end; }

  /** @returns the number of bytes viewed */
  std::size_t size() const { return std::size_t(_end - _begin); }

  /** @returns true if not empty() */
  explicit operator bool() const { return _begin != _end; }

  /**
   * From the viewed bytes, construct a T object by memcopy.
   *
   * Drops the copied bytes from the view.
   * Provides strong exception guarantee.
   *
   * @requires T to be trivially copyable
   * @throws std::runtime_error if size() < sizeof(T)
   * @post size() == old_size - sizeof(T)
   * @returns the constructed T
   */
  template <typename T>
  T read()
  {
    static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");

    throw_if_overflow(sizeof(T));

    T dst;
    memcpy(&dst, _begin, sizeof(T));
    _begin += sizeof(T);

    return dst;
  }

  /**
   * Copy `size` bytes from the viewed bytes to `dst`.
   *
   * Drops the copied bytes from the view.
   * Provides strong exception guarantee.
   *
   * @pre size >= 0
   * @throws std::runtime_error if size() < size
   * @post size() == old_size - size
   * @returns *this
   */
  Range& read(char* dst, std::streamsize size)
  {
    throw_if_overflow(std::size_t(size));

    memcpy(dst, _begin, std::size_t(size));
    _begin += size;

    return *this;
  }

  /**
   * Drop `size` bytes from the view.
   *
   * Provides strong exception guarantee.
   *
   * @pre size >= 0
   * @throws std::runtime_error if size() < size
   * @post size() == old_size - size
   * @returns pointer to the dropped bytes
   */
  const char* view(std::size_t size)
  {
    throw_if_overflow(size);

    const char* result = _begin;
    _begin += size;
    return result;
  }

private:
  void throw_if_overflow(std::size_t s) const
  {
    if (_begin + s > _end)
    {
      throw std::runtime_error(
        "Range overflow " + std::to_string(s) + " > "
                          + std::to_string(size())
      );
    }
  }

  const char* _begin = nullptr;
  const char* _end = nullptr;
};

} // namespace binlog

#endif // BINLOG_RANGE_HPP
