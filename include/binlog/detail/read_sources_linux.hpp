#ifndef BINLOG_DETAIL_READ_SOURCES_LINUX_HPP
#define BINLOG_DETAIL_READ_SOURCES_LINUX_HPP

#include <binlog/Entries.hpp>
#include <binlog/detail/MemoryMappedFile.hpp>

#include <mserialize/string_view.hpp>

#include <elf.h>
#include <link.h>

#include <vector>

namespace binlog {
namespace detail {

struct EventSourcesOfRunningProgram {
  mserialize::string_view pathsuffix;
  std::vector<EventSource> result;
};

inline void collect_sources_of_mapped_object(const MemoryMappedFile& f, dl_phdr_info* info, std::vector<EventSource>& result)
{
  // Read ELF header
  ElfW(Ehdr) ehdr;
  f.read(0, sizeof(ehdr), &ehdr);

  // Read ELF section headers
  std::vector<ElfW(Shdr)> shdrs(ehdr.e_shnum);
  f.read(ehdr.e_shoff, shdrs.size() * sizeof(ElfW(Shdr)), shdrs.data());

  // Find our section
  const ElfW(Off) string_table_offset = shdrs.at(ehdr.e_shstrndx).sh_offset;
  const mserialize::string_view event_source_section_name(".binlog.esrc");

  for (const ElfW(Shdr)& shdr : shdrs)
  {
    const auto section_name = f.string(string_table_offset + shdr.sh_name);
    if (section_name == event_source_section_name)
    {
      // find this section mapped into one of the segments
      for (int i = 0; i < info->dlpi_phnum; ++i)
      {
        const ElfW(Phdr)& phdr = info->dlpi_phdr[i];
        if (phdr.p_offset <= shdr.sh_offset && shdr.sh_offset + shdr.sh_size <= phdr.p_offset + phdr.p_filesz)
        {
          // this segment contains our section
          const auto offset = shdr.sh_offset - phdr.p_offset;

          const char* data = reinterpret_cast<const char*>(info->dlpi_addr + phdr.p_vaddr + offset); // NOLINT
          const char* end = data + shdr.sh_size;
          const char* ptrs[8];
          for (const char* p = data; p + sizeof(ptrs) <= end; p += sizeof(ptrs))
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

          break;
        }
      }
    }
  }
}

inline int collect_sources_of_loaded_object(dl_phdr_info* info, size_t, void* vsources)
{
  // man dl_iterate_phdr: For the main program, the dlpi_name field will be an empty string.
  const char* path = (info->dlpi_name[0] != '\0') ? info->dlpi_name : "/proc/self/exe";

  EventSourcesOfRunningProgram& sources = *reinterpret_cast<EventSourcesOfRunningProgram*>(vsources);
  if (mserialize::string_view(path).ends_with(sources.pathsuffix))
  {
    try
    {
      const MemoryMappedFile f(path);
      collect_sources_of_mapped_object(f, info, sources.result);
    }
    catch (const std::runtime_error&)
    {
      // unable to open file: it might have been deleted, or pseudo file (vdso).
      // there's no way to propagate this error up, noop for now.
    }
  }

  return 0;
}

inline std::vector<EventSource> event_sources_of_running_program(mserialize::string_view pathsuffix = {})
{
  EventSourcesOfRunningProgram sources{pathsuffix, {}};
  dl_iterate_phdr(collect_sources_of_loaded_object, &sources);
  return sources.result;
}

} // namespace detail
} // namespace binlog

#endif // BINLOG_DETAIL_READ_SOURCES_LINUX_HPP
