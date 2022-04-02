#ifndef BINLOG_DETAIL_READ_SOURCES_MACOS_HPP
#define BINLOG_DETAIL_READ_SOURCES_MACOS_HPP

#include <binlog/Entries.hpp>

#include <mserialize/string_view.hpp>

#include <mach-o/dyld.h>
#include <mach-o/getsect.h>

#include <cstring>
#include <vector>

namespace binlog {
namespace detail {

inline void add_event_sources_of_mhdr(const mach_header* mhdr, std::vector<EventSource>& result)
{
  // regardless the platform, _dyld_get_image_header returns a mach_header*
#ifdef __LP64__
  const mach_header_64* mhdrp = reinterpret_cast<const mach_header_64*>(mhdr);
#else
  const mach_header mhdrp = mhdr;
#endif

  unsigned long size = 0;
  const uint8_t* data = getsectiondata(mhdrp, "__DATA_CONST", ".binlog.esrc", &size);
  const uint8_t* end = data + size;

  const char* ptrs[8];
  for (const uint8_t* p = data; p + sizeof(ptrs) <= end; p += sizeof(ptrs))
  {
    memcpy(ptrs, p, sizeof(ptrs));
    result.push_back(EventSource{
        std::uintptr_t(p) / 64,
        severityFromString(ptrs[0]),
        ptrs[1],
        ptrs[2],
        ptrs[3],
        std::strtoul(ptrs[4], nullptr, 10),
        ptrs[5],
        ptrs[6],
    });
  }
}

inline std::vector<EventSource> event_sources_of_running_program(mserialize::string_view pathsuffix = {})
{
  std::vector<EventSource> result;

  for (std::uint32_t i = 0; true; ++i)
  {
    const char* path = _dyld_get_image_name(i);
    if (! path) break; // i is invalid, reached end of loaded images
    if (! mserialize::string_view{path}.ends_with(pathsuffix)) { continue; }
    const mach_header* mhdr = _dyld_get_image_header(i);
    add_event_sources_of_mhdr(mhdr, result);
  }

  return result;
}

} // namespace detail
} // namespace binlog

#endif // BINLOG_DETAIL_READ_SOURCES_MACOS_HPP
