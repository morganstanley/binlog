#include <binlog/detail/OstreamBuffer.hpp>

#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstring>

namespace binlog {
namespace detail {

OstreamBuffer::OstreamBuffer(std::ostream& out)
  :_out(out),
   _buf{},
   _p(_buf.data())
{}

OstreamBuffer::~OstreamBuffer()
{
  flush();
}

void OstreamBuffer::put(char c)
{
  reserve(1);
  *_p++ = c;
}

void OstreamBuffer::write(const char* buf, std::size_t size)
{
  while (size != 0)
  {
    const std::size_t wsize = (std::min)(size, _buf.size());
    reserve(wsize);
    memcpy(_p, buf, wsize);
    _p += wsize;
    size -= wsize;
    buf += wsize;
  }
}

OstreamBuffer& OstreamBuffer::operator<<(bool b)
{
  if (b) { return *this << "true"; }
           return *this << "false";
}

OstreamBuffer& OstreamBuffer::operator<<(double v)
{
  reserve(64);
  _p += snprintf(_p, 64, "%.16g", v);
  return *this;
}

OstreamBuffer& OstreamBuffer::operator<<(long double v)
{
  reserve(128);
  _p += snprintf(_p, 128, "%.16Lg", v);
  return *this;
}

void OstreamBuffer::writeSigned(std::int64_t v)
{
  // TODO(benedek) perf: use a faster than printf int-to-string solution
  reserve(32);
  _p += snprintf(_p, 32, "%" PRId64, v);
}

void OstreamBuffer::writeUnsigned(std::uint64_t v)
{
  // TODO(benedek) perf: use a faster than printf uint-to-string solution
  reserve(32);
  _p += snprintf(_p, 32, "%" PRIu64, v);
}

void OstreamBuffer::reserve(std::size_t n)
{
  assert(n <= _buf.size());

  if (std::size_t(_p - _buf.data()) + n > _buf.size())
  {
    flush();
  }
}

void OstreamBuffer::flush()
{
  _out.write(_buf.data(), _p - _buf.data());
  _p = _buf.data();
}

} // namespace detail
} // namespace binlog
