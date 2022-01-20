#ifndef BINLOG_CREATE_SOURCE_HPP
#define BINLOG_CREATE_SOURCE_HPP

#include <cstdint>
#include <cstdlib>

/**
 * Create a static EventSource in the binary compile time.
 *
 * Defines a variable named `id`, that will have a unique value (even for DSOs) runtime.
 * The static_casts are needed to avoid section type conflicts with clang.
 */
#define BINLOG_CREATE_SOURCE(id, severity, category, format, argumenttags) \
  __attribute__((section(".binlog.esrc"), used))                           \
  static constexpr const char* _binlog_esrc[] = {                          \
    severity,                                                              \
    #category,                                                             \
    static_cast<const char*>(__func__),                                    \
    __FILE__,                                                              \
    MSERIALIZE_STRINGIZE(__LINE__),                                        \
    format,                                                                \
    static_cast<const char*>(argumenttags),                                \
    nullptr                                                                \
  };                                                                       \
  const std::uint64_t id = std::uint64_t(&_binlog_esrc) >> 6;              \
  /**/

#endif // BINLOG_CREATE_SOURCE_HPP
