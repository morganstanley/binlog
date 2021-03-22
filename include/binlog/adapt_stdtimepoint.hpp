#ifndef BINLOG_ADAPT_STDTIMEPOINT_HPP
#define BINLOG_ADAPT_STDTIMEPOINT_HPP

#include <mserialize/cx_string.hpp>
#include <mserialize/serialize.hpp>
#include <mserialize/tag.hpp>

#include <chrono>
#include <cstdint>

namespace mserialize {

template <typename Duration>
struct CustomSerializer<std::chrono::time_point<std::chrono::system_clock, Duration>>
{
  template <typename OutputStream>
  static void serialize(const std::chrono::time_point<std::chrono::system_clock, Duration> tp, OutputStream& ostream)
  {
    const std::int64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count();
    mserialize::serialize(ns, ostream);
  }

  static std::size_t serialized_size(const std::chrono::system_clock::time_point)
  {
    return sizeof(std::int64_t);
  }
};

template <typename Duration>
struct CustomTag<std::chrono::time_point<std::chrono::system_clock, Duration>>
{
  static constexpr auto tag_string()
  {
    return make_cx_string("{std::chrono::system_clock::time_point`ns'l}");
  }
};

} // namespace mserialize

#endif // BINLOG_ADAPT_STDTIMEPOINT_HPP
