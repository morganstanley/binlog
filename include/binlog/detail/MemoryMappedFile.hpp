#ifndef BINLOG_DETAIL_MEMORY_MAPPED_FILE_HPP
#define BINLOG_DETAIL_MEMORY_MAPPED_FILE_HPP

#include <mserialize/string_view.hpp>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>

namespace binlog {
namespace detail {

/** A read-only view of a fixed size file on the filesystem */
class MemoryMappedFile
{
public:
  /**
   * Create a view of the file at `path`.
   *
   * @throws on error: file not found, file has unknown size, file cannot be mapped
   */
  explicit MemoryMappedFile(const char* path);
  ~MemoryMappedFile();

  MemoryMappedFile(const MemoryMappedFile&) = delete;
  MemoryMappedFile(MemoryMappedFile&&) = delete;
  void operator=(const MemoryMappedFile&) = delete;
  void operator=(MemoryMappedFile&&) = delete;

  /**
   * Read `size` bytes into `dst` from `offset`.
   *
   * @throws if out of bounds.
   */
  void read(std::size_t offset, std::size_t size, void* dst) const;

  /**
   * @return a pointer to a null-terminated C string at `offset`.
   * @throws if there's no such string at `offset`.
   */
  mserialize::string_view string(std::size_t offset) const;

private:
  char* _map;
  std::size_t _size;
};

inline MemoryMappedFile::MemoryMappedFile(const char* path)
{
  const int fd = open(path, O_RDONLY|O_CLOEXEC);
  if (fd < 0)
  {
    throw std::system_error(errno, std::generic_category(), path);
  }
  const off_t size = lseek(fd, 0, SEEK_END);
  if (size < 0)
  {
    const int e = errno;
    close(fd);
    throw std::system_error(e, std::generic_category(), path);
  }

  _size = std::size_t(size);
  void* map = mmap(nullptr, _size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (map == MAP_FAILED) // NOLINT
  {
    const int e = errno;
    close(fd);
    throw std::system_error(e, std::generic_category(), path);
  }
  _map = static_cast<char*>(map);

  close(fd);
}

inline MemoryMappedFile::~MemoryMappedFile()
{
  munmap(_map, _size);
}

inline void MemoryMappedFile::read(std::size_t offset, std::size_t size, void* dst) const
{
  if (offset > _size || size > _size - offset)
  {
    throw std::runtime_error("MemoryMappedFile::read out of bounds");
  }
  memcpy(dst, _map + offset, size);
}

inline mserialize::string_view MemoryMappedFile::string(std::size_t offset) const
{
  if (offset > _size)
  {
    throw std::runtime_error("MemoryMappedFile::string out of bounds");
  }
  const void* end = memchr(_map + offset, 0, _size - offset);
  if (end == nullptr)
  {
    throw std::runtime_error("MemoryMappedFile::string no C-string at offset");
  }
  return mserialize::string_view(
    _map + offset,
    std::size_t(reinterpret_cast<const char*>(end) - _map) - offset
  );
}

} // namespace detail
} // namespace binlog

#endif // BINLOG_DETAIL_MEMORY_MAPPED_FILE_HPP
