#ifndef BINLOG_DETAIL_READ_SOURCES_LINUX_HPP
#define BINLOG_DETAIL_READ_SOURCES_LINUX_HPP

#include <binlog/Entries.hpp>
#include <binlog/detail/MemoryMappedFile.hpp>

#include <mserialize/string_view.hpp>

#include <elf.h>
#include <link.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace binlog {
namespace detail {

inline void add_event_sources_of_file(const std::string& path, std::uintptr_t loadaddr, std::vector<EventSource>& result)
{
  const MemoryMappedFile f(path.data());
  ElfW(Ehdr) ehdr;
  f.read(0, sizeof(ehdr), &ehdr);

  if (ehdr.e_type == ET_EXEC)
  {
    // the load address is not added to the address of the sections of the main executable
    loadaddr = 0;
  }

  std::vector<ElfW(Shdr)> shdrs(ehdr.e_shnum);
  f.read(ehdr.e_shoff, shdrs.size() * sizeof(ElfW(Shdr)), shdrs.data());

  const ElfW(Off) string_table_offset = shdrs.at(ehdr.e_shstrndx).sh_offset;

  auto foffset = [&shdrs](std::uint64_t v) // TODO(benedek) perf: cache last hit
  {
    for (const ElfW(Shdr)& shdr : shdrs)
    {
      if (shdr.sh_addr <= v && shdr.sh_addr + shdr.sh_size > v)
      {
        return v - shdr.sh_addr + shdr.sh_offset;
      }
    }
    throw std::runtime_error("address out of bounds");
  };

  const mserialize::string_view event_source_section_name(".binlog.esrc");

  for (const ElfW(Shdr)& shdr : shdrs)
  {
    const auto section_name = f.string(string_table_offset + shdr.sh_name);
    if (section_name == event_source_section_name)
    {
      std::uint64_t ptrs[8];
      for (ElfW(Xword) offset = 0; offset < shdr.sh_size; offset += sizeof(ptrs))
      {
        f.read(shdr.sh_offset + offset, sizeof(ptrs), ptrs);

        result.push_back(EventSource{
          (loadaddr + shdr.sh_addr + offset) / 64,
          severityFromString(f.string(foffset(ptrs[0]))),
          f.string(foffset(ptrs[1])).to_string(),
          f.string(foffset(ptrs[2])).to_string(),
          f.string(foffset(ptrs[3])).to_string(),
          std::strtoul(f.string(foffset(ptrs[4])).data(), nullptr, 10),
          f.string(foffset(ptrs[5])).to_string(),
          f.string(foffset(ptrs[6])).to_string()
        });
      }
    }
  }
}

/** Represents a line in /proc/<pid>/maps. */
struct MapRecord
{
  explicit MapRecord(const std::string& line);

  std::uintptr_t begin = 0;
  std::uintptr_t end = 0;
  std::uintptr_t offset = 0;
  std::string path;
};

inline MapRecord::MapRecord(const std::string& line)
{
  std::istringstream str(line);
  str >> std::hex >> begin;
  str.ignore(1, '-');
  str >> std::hex >> end;
  str.ignore(1, ' ');
  str.ignore(16, ' '); // perms
  str >> std::hex >> offset;
  str.ignore(1, ' ');
  str.ignore(16, ' '); // device
  str.ignore(16, ' '); // inode
  str >> path;

  if (str.fail() || str.bad()) { begin = end = 0; }
}

inline std::vector<EventSource> event_sources_of_running_program(mserialize::string_view pathsuffix = {})
{
  std::vector<EventSource> result;

  std::set<std::string> visited;

  std::ifstream maps("/proc/self/maps");
  std::string line;
  while (std::getline(maps, line))
  {
    const MapRecord m(line);
    if (m.begin == m.end || m.path.empty() || m.offset != 0) { continue; }
    if (! mserialize::string_view{m.path}.ends_with(pathsuffix)) { continue; }
    if (! visited.insert(m.path).second) { continue; }

    try
    {
      add_event_sources_of_file(m.path, m.begin, result);
    }
    catch (const std::runtime_error&)
    {
      // unable to parse pseudo modules: [heap], [stack], [vdso], [vsyscall]
      // there's no way to propagate this error up, noop for now.
    }
  }

  return result;
}

} // namespace detail
} // namespace binlog

#endif // BINLOG_DETAIL_READ_SOURCES_LINUX_HPP
