#ifndef BINLOG_DETAIL_CUEUE_HPP
#define BINLOG_DETAIL_CUEUE_HPP

#include <atomic>
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ios>
#include <memory>
#include <system_error>
#include <utility>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace binlog {
namespace detail {

/** @returns Smallest power of 2 not smaller than `n` */
inline std::size_t nextPowerTwo(std::size_t n)
{
  --n;
  std::size_t result = 2;
  while ((n >>= 1) != 0) { result <<= 1; }
  return result;
}

class File
{
public:
  explicit File(int fd) :_fd(fd) {}

  ~File()
  {
    if (_fd >= 0)
    {
      close(_fd);
    }
  }

  File(File&& other) noexcept
    :_fd(other._fd)
  {
    other._fd = -1;
  }

  File& operator=(File&& other) noexcept
  {
    std::swap(_fd, other._fd);
    return *this;
  }

  explicit operator bool() const {
    return _fd >= 0;
  }

  int operator*() const { return _fd; }

  File(const File&) = delete;
  void operator=(const File&) = delete;

private:
  int _fd;
};

struct UnmapMemory
{
  std::size_t size = 0;

  void operator()(void* map) const
  {
    if (map != MAP_FAILED) // NOLINT
    {
      munmap(map, size);
    }
  }
};

using MemoryMap = std::unique_ptr<void, UnmapMemory>;

struct CueueControlBlock {
  alignas(64) std::uint64_t magic = 0; // for recovery
  void* discriminator = nullptr;       // for recovery
  std::size_t capacity = 0;

  // Queue is empty if R == W
  // Queue is full if W == R+capacity
  // Invariant: W >= R
  // Invariant: R + capacity >= W

  // members written by Writer
  std::atomic<std::size_t> writePosition = {0};

  // members written by Reader
  alignas(64) std::atomic<std::size_t> readPosition = {0};
};

class CueueWriter
{
  CueueControlBlock* _qcb = nullptr;

  char* _buffer = nullptr;
  char* _writeBegin = nullptr;
  char* _writePos = nullptr;
  char* _writeEnd = nullptr;
  size_t _mask = 0;

public:
  /**
   * Construct an empty writer.
   *
   * A non-empty writer can be assigned to it,
   * or it can be destructed.
   */
  CueueWriter() = default;

  /**
   * Construct a writer that writes `buffer` using `qcb`.
   */
  CueueWriter(CueueControlBlock& qcb, char* buffer)
    :_qcb(& qcb),
     _buffer(buffer),
     _mask(qcb.capacity - 1)
  {}

  /** @returns the maximum number of bytes the queue can store */
  std::size_t capacity() const { return _qcb->capacity; }

  /** @returns the number of bytes currently available for write */
  std::size_t writeCapacity() const
  {
    return std::size_t(_writeEnd - _writePos);
  }

  /**
   * Maximize writeCapacity().
   *
   * Resets the internal write buffer,
   * uncommitted writes (those without a subsequent endWrite())
   * will be lost.
   *
   * @post writeCapacity() == sizeof max contiguous writable arena
   * @returns writeCapacity()
   */
  std::size_t beginWrite()
  {
    const std::size_t w = _qcb->writePosition.load(std::memory_order_relaxed);
    const std::size_t r = _qcb->readPosition.load(std::memory_order_acquire);

    assert(r <= w);
    assert(r + _qcb->capacity >= w);

    const std::size_t wc = _qcb->capacity - w + r;
    const std::size_t wi = w & _mask;
    _writeBegin = _buffer + wi;
    _writePos = _writeBegin;
    _writeEnd = _writeBegin + wc;

    return wc;
  }

  /**
   * Attempt to make writeCapacity() >= `size`.
   *
   * Possibly calls beginWrite(), which resets the internal write buffer,
   * uncommitted writes (those without a subsequent endWrite())
   * will be lost.
   *
   * @returns `size` <= writeCapacity()
   */
  bool beginWrite(std::size_t size)
  {
    const std::size_t wc = writeCapacity();
    if (size <= wc)
    {
      return true;
    }

    return size <= beginWrite();
  }

  /**
   * Copy the range [src,src+size) to the internal write buffer.
   *
   * @pre writeCapacity() >= `size`
   * @post writeCapacity() -= `size`
   * @returns A pointer to the bitwise copy of the range,
   * which pointer remains valid until the next `beginWrite()` or `endWrite()`.
   */
  void* write(const void* src, std::streamsize size)
  {
    assert(_writePos + size <= _writeEnd);

    void* result = memcpy(_writePos, src, std::size_t(size)); // NOLINT
    _writePos += size;
    return result;
  }

  /** Make the written parts of the internal buffer available to read. */
  void endWrite()
  {
    const std::size_t w = _qcb->writePosition.load(std::memory_order_relaxed);
    const std::size_t writeSize = std::size_t(_writePos - _writeBegin);

    _writeBegin += writeSize;
    _qcb->writePosition.store(w + writeSize, std::memory_order_release);
  }
};

class CueueReader
{
  CueueControlBlock* _qcb = nullptr;

  const char* _buffer = nullptr;
  const char* _readBegin = nullptr;
  std::size_t _readSize = 0;
  std::size_t _mask = 0;

public:
  struct ReadResult
  {
    const char* buffer = nullptr;
    std::size_t size = 0;
  };

  /**
   * Construct an empty reader.
   *
   * A non-empty reader can be assigned to it,
   * or it can be destructed.
   */
  CueueReader() = default;

  /**
   * Construct a reader that reads `buffer` using `qcb`.
   */
  CueueReader(CueueControlBlock& qcb, const char* buffer)
    :_qcb(& qcb),
     _buffer(buffer),
     _mask(qcb.capacity - 1)
  {}

  /**
   * Maximize readCapacity, the number of bytes available for read
   *
   * @returns A view of the readable data
   */
  ReadResult beginRead()
  {
    const std::size_t w = _qcb->writePosition.load(std::memory_order_acquire);
    const std::size_t r = _qcb->readPosition.load(std::memory_order_relaxed);

    assert(r <= w);
    assert(r + _qcb->capacity >= w);

    const std::size_t ri = r & _mask;
    _readBegin = _buffer + ri;
    _readSize = w - r;

    return ReadResult{_readBegin, _readSize};
  }

  /** Make the consumed parts of the internal buffer available to write. */
  void endRead()
  {
    const std::size_t r = _qcb->readPosition.load(std::memory_order_relaxed);
    _qcb->readPosition.store(r + _readSize, std::memory_order_release);
  }
};

/**
 * A truly circular queue of bytes.
 *
 * A circular queue can be concurrently written
 * and read by a single writer and a single reader.
 *
 * The byte queue observed by the writer and the reader
 * is circular, after the last byte comes the first.
 * This is achieved by mapping an in-memory file into virtual memory
 * twice, putting the maps next to each other.
 *
 * Limitations:
 *  - Linux/macOS only
 *  - capacity must be power of two, multiple of page size
 *  - Linux requires tmpfs (e.g: /dev/shm)
 */
class Cueue
{
public:
  /** @param capacity must be a power of two, multiple of page size */
  explicit Cueue(std::size_t capacity)
  {
    const std::size_t cbsize = controlblocksize();
    capacity = nextPowerTwo(std::max(capacity, cbsize));

    // Create a large enough file in memory to host the control block and the buffer
    const File f(memoryfile(cbsize + capacity));

    // Create a map: [C][BBBBBB][BBBBBB]
    // C: Control block
    // B: Buffer, mapped twice
    _map = doublemap(*f, cbsize, capacity);

    // Create control block
    new (_map.get()) CueueControlBlock{0, nullptr, capacity, {0}, {0}};
  }

  /** Same as above, but also sets magic and discriminator in the control block for recovery */
  Cueue(std::size_t capacity, std::uint64_t magic, void* discriminator)
    :Cueue(capacity)
  {
    controlblock().magic = magic;
    controlblock().discriminator = discriminator;
  }

  std::size_t capacity() const { return controlblock().capacity; }

  CueueControlBlock& controlblock()
  {
    return *reinterpret_cast<CueueControlBlock*>(_map.get());
  }

  const CueueControlBlock& controlblock() const
  {
    return *reinterpret_cast<const CueueControlBlock*>(_map.get());
  }

  CueueWriter writer() { return CueueWriter(controlblock(), buffer()); }
  CueueReader reader() { return CueueReader(controlblock(), buffer()); }

  void clearMagic()
  {
    controlblock().magic = 0;
  }

private:
  /** Size of the control block before the buffeer */
  static std::size_t controlblocksize()
  {
    long pagesize = sysconf(_SC_PAGESIZE); // NOLINT(google-runtime-int)
    if (pagesize < 0) { throw_errno("sysconf"); }
    return std::size_t(pagesize);
  }

  static void throw_errno(const char* msg)
  {
    throw std::system_error(errno, std::generic_category(), msg);
  }

  /** Create a file descriptor that points to a `size` big chunk of memory */
  static File memoryfile(std::size_t size)
  {
#ifdef __linux__
    // Do not use memfd_create to retain compatibility with older systems (e.g: rhel7)
    char path[] = "/dev/shm/binlog_XXXXXX";
    File f(mkstemp(path));
    if (!f) { throw_errno("mkstemp"); }
    unlink(path);
#elif defined(__APPLE__)
    // create a temp file to reserve a system-wide unique name
    char path[] = "/tmp/binlog_XXXXXX";
    File nf(mkstemp(path));
    if (!nf) { throw_errno("mkstemp"); }

    File f(shm_open(path, O_RDWR|O_CREAT|O_EXCL));
    unlink(path);
    if (!f) { throw_errno("shm_open"); }
#endif

    if (ftruncate(*f, off_t(size)) != 0) { throw_errno("ftruncate"); }
    return f;
  }

  /** Map a `size` chunk of `fd` at `offset` twice, next to each other in virtual memory */
  static MemoryMap doublemap(int fd, std::size_t offset, std::size_t size)
  {
    // Create a map, offset + twice the size, to get a suitable virtual address which will work with MAP_FIXED
    const int rw = PROT_READ | PROT_WRITE;
    MemoryMap map(
      mmap(nullptr, offset + size * 2, rw, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0),
      UnmapMemory{offset + size * 2}
    );
    if (map.get() == MAP_FAILED) { throw_errno("mmap 1"); } // NOLINT

#ifdef __linux__
    const int platform_flags = MAP_POPULATE;
#else
    const int platform_flags = 0;
#endif

    // Map f twice, put maps next to each other with MAP_FIXED
    // MAP_SHARED is required to have the changes propagated between maps
    char* first_addr = static_cast<char*>(map.get()) + offset;
    const void* first_map = mmap(first_addr, size, rw, MAP_SHARED | MAP_FIXED | platform_flags, fd, off_t(offset));
    if (first_map != first_addr) { throw_errno("mmap 2"); }

    void* second_addr = first_addr + size;
    const void* second_map = mmap(second_addr, size, rw, MAP_SHARED | MAP_FIXED, fd, off_t(offset));
    if (second_map != second_addr) { throw_errno("mmap 3"); }

    // man mmap:
    // If the memory region specified by addr and len overlaps
    // pages of any existing mapping(s), then the overlapped part
    // of the existing mapping(s) will be discarded.
    // -> No need to munmap `first_map` and `second_map`, ~map will do both
    return map;
  }

  char* buffer()
  {
    return static_cast<char*>(_map.get()) + controlblocksize();
  }

  MemoryMap _map;
};

} // namespace detail
} // namespace binlog

#endif // BINLOG_DETAIL_CUEUE_HPP
