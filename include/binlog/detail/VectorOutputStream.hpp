#ifndef BINLOG_DETAIL_VECTOR_OUTPUT_STREAM_HPP
#define BINLOG_DETAIL_VECTOR_OUTPUT_STREAM_HPP

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

} // namespace detail
} // namespace binlog

#endif // BINLOG_DETAIL_VECTOR_OUTPUT_STREAM_HPP
