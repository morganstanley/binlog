#ifndef BINLOG_DETAIL_READ_SOURCES_HPP
#define BINLOG_DETAIL_READ_SOURCES_HPP

#ifdef __linux__
  #include <binlog/detail/read_sources_linux.hpp>
#elif defined(__APPLE__)
  #include <binlog/detail/read_sources_macos.hpp>
#else
  #error "Unsupported platform"
#endif

#endif // BINLOG_DETAIL_READ_SOURCES_HPP
