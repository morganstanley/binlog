#include <binlog/binlog.hpp>

#include <binlog/TextOutputStream.hpp>

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <type_traits>

enum class BufferType
{
  Metadata = 0,
  Data = 1,
};

BINLOG_ADAPT_ENUM(BufferType, Data, Metadata)

struct RecoveredBuffer
{
  BufferType type;
  std::uintptr_t session;
  std::vector<char> buffer;
};

namespace {

void printLogs()
{
  static binlog::TextOutputStream textlog(std::cerr, "%S [%d] %m\n");
  binlog::consume(textlog);
}

#define STDERR_INFO(...)  BINLOG_INFO(__VA_ARGS__);  printLogs()
#define STDERR_ERROR(...) BINLOG_ERROR(__VA_ARGS__); printLogs()

void showHelp()
{
  std::cout <<
    "brecovery -- extract unconsumed binlog data from a memory dump\n"
    "\n"
    "Synopsis:\n"
    "  brecovery corefile [outputfile]\n"
    "\n"
    "Arguments:\n"
    "  corefile        Path to a corefile (memory dump)\n"
    "  outputfile      Path to write recovered data. If '-' or unspecified, read from stdin\n"
    "\n"
    "Notes:\n"
    "  Recovery is done by looking for 'magic numbers' in the corefile,\n"
    "  and extracting structured data following those. Metadata and unconsumed data\n"
    "  are extracted from each session, and written to the output.\n"
    "  The output is a valid binlog logfile (if it was not corrupted previously),\n"
    "  and can be read using bread:\n"
    "\n"
    "    $ brecovery app.core recovered.blog\n"
    "    $ bread recovered.blog\n"
    "\n"
    "Report bugs to:\n"
    "  https://github.com/Morgan-Stanley/binlog/issues\n";
    ;
}

std::ostream& openFile(const std::string& path, std::ofstream& file)
{
  if (path == "-")
  {
    return std::cout;
  }

  file.open(path, std::ios_base::out | std::ios_base::binary);
  return file;
}

std::array<unsigned char, 8> toArray(std::uint64_t n)
{
  std::array<unsigned char, 8> result{};
  memcpy(result.data(), &n, result.size());
  return result;
}

bool checkEntryBuffer(const std::vector<char>& buffer)
{
  binlog::Range range(buffer.data(), buffer.size());
  try
  {
    while (range)
    {
      const std::uint32_t size = range.read<std::uint32_t>();
      range.view(size);
    }
  }
  catch (const std::runtime_error&)
  {
    STDERR_ERROR("  Buffer contains invalid entry");
    return false;
  }

  return true;
}

std::size_t remainingSize(std::istream& input)
{
  const auto pos = input.tellg();
  input.seekg(0, std::ios_base::end);
  const auto end = input.tellg();
  const std::size_t size = std::size_t(end - pos);
  input.seekg(pos);
  return size;
}

bool readMetadata(std::istream& input, std::vector<RecoveredBuffer>& output)
{
  // get session ptr as a number
  void* sessionptr = nullptr;
  input.read(reinterpret_cast<char*>(&sessionptr), sizeof(void*));
  const std::uintptr_t session = reinterpret_cast<std::uintptr_t>(sessionptr);

  // get the size of the metadata in the buffer
  std::size_t size = 0;
  input.read(reinterpret_cast<char*>(&size), sizeof(size));

  // get the buffer
  if (size > remainingSize(input))
  {
    STDERR_ERROR("  Input doesn't have {} bytes", size);
    return false;
  }

  std::vector<char> metadata;
  try
  {
    metadata.resize(size);
  }
  catch (const std::bad_alloc&)
  {
    STDERR_ERROR("  Failed to allocate {} bytes for metadata", size);
    return false;
  }
  input.read(metadata.data(), std::streamsize(size));

  if (! input)
  {
    STDERR_ERROR("  Failed to read {} bytes of metadata from input", size);
    return false;
  }

  if (! checkEntryBuffer(metadata)) { return false; }

  STDERR_INFO("  Recovered {} bytes of metadata from session={}", metadata.size(), session);

  output.push_back(RecoveredBuffer{BufferType::Metadata, session, std::move(metadata)});
  return true;
}

bool checkQueueInvariants(binlog::detail::Queue& queue)
{
  if (queue.writeIndex > queue.capacity)
  {
    BINLOG_ERROR("  Queue invariant violated: writer={} > capacity={}", queue.writeIndex.load(), queue.capacity);
    return false;
  }

  if (queue.dataEnd > queue.capacity)
  {
    BINLOG_ERROR("  Queue invariant violated: dataEnd={} > capacity={}", queue.dataEnd, queue.capacity);
    return false;
  }

  if (queue.readIndex > queue.capacity)
  {
    BINLOG_ERROR("  Queue invariant violated: reader={} > capacity={}", queue.readIndex.load(), queue.capacity);
    return false;
  }

  return true;
}

bool readData(std::istream& input, std::vector<RecoveredBuffer>& output)
{
  // get session ptr as a number
  void* sessionptr = nullptr;
  input.read(reinterpret_cast<char*>(&sessionptr), sizeof(void*));
  const std::uintptr_t session = reinterpret_cast<std::uintptr_t>(sessionptr);

  // get queue with writer and reader positions
  // Depending on the standard/compiler, the assert below is invalid
  // because of the std::atomic inside Queue - but we do not care.
  // static_assert(std::is_trivially_copyable<binlog::detail::Queue>::value, "Queue must be trivial");
  binlog::detail::Queue queue(nullptr, 0);
  input.read(reinterpret_cast<char*>(&queue), sizeof(queue));

  if (! checkQueueInvariants(queue)) { return false; }

  STDERR_INFO("  Queue state is valid: capacity={} windex={} rindex={} dataend={}",
    queue.capacity, queue.writeIndex.load(), queue.readIndex.load(), queue.dataEnd);

  // get queue buffer
  if (queue.capacity > remainingSize(input))
  {
    STDERR_ERROR("  Input doesn't have {} bytes", queue.capacity);
    return false;
  }

  std::vector<char> queueBuffer;
  try
  {
    queueBuffer.resize(queue.capacity);
  }
  catch (const std::bad_alloc&)
  {
    STDERR_ERROR("  Failed to allocate {} bytes for queue data", queue.capacity);
    return false;
  }
  input.read(queueBuffer.data(), std::streamsize(queueBuffer.size()));

  if (! input) { return false; }

  // create a reader, get the unread data from the buffer
  queue.buffer = queueBuffer.data();
  binlog::detail::QueueReader reader(queue);
  const binlog::detail::QueueReader::ReadResult dataview = reader.beginRead();

  std::vector<char> data;
  data.reserve(dataview.size());
  data.insert(data.end(), dataview.buffer1, dataview.buffer1 + dataview.size1);
  data.insert(data.end(), dataview.buffer2, dataview.buffer2 + dataview.size2);

  if (! checkEntryBuffer(data)) { return false; }

  STDERR_INFO("  Recovered {} bytes of data from session={}", data.size(), session);

  output.push_back(RecoveredBuffer{BufferType::Data, session, std::move(data)});
  return true;
}

} // namespace

int main(int argc, const char* argv[])
{
  if (argc < 2)
  {
    showHelp();
    return 1;
  }

  std::ifstream input(argv[1], std::ios_base::in|std::ios_base::binary);
  if (! input)
  {
    STDERR_ERROR("Failed to open {} for reading", argv[1]);
    showHelp();
    return 2;
  }

  const std::string outputPath = (argc > 2) ? argv[2] : "-";
  std::ofstream outputFile;
  std::ostream& output = openFile(outputPath, outputFile);
  if (! output)
  {
    STDERR_ERROR("Failed to open {} for writing", argv[2]);
    showHelp();
    return 3;
  }

  std::vector<RecoveredBuffer> buffers;

  // do not initialize arrays directly to remain endianness agnostic
  const std::array<unsigned char, 8> metadataMagic = toArray(0xFE214F726E35BDBC);
  const std::array<unsigned char, 8> dataMagic = toArray(0xFE213F716D34BCBC);

  // magic numbers start with the same byte, makes searching easier
  assert(metadataMagic[0] == dataMagic[0]);
  const unsigned char firstMagicByte = metadataMagic[0];

  STDERR_INFO("Read input from {}", argv[1]);

  while (input.ignore(std::numeric_limits<std::streamsize>::max(), firstMagicByte).good())
  {
    std::array<unsigned char, 8> magic{firstMagicByte, 0, 0, 0, 0, 0, 0, 0};
    input.read(reinterpret_cast<char*>(magic.data()+1), 7);

    const auto afterMagicPos = std::streamoff{input.tellg()};

    if (magic == metadataMagic)
    {
      STDERR_INFO("Magic number found, read metadata at tellg={}", afterMagicPos);
      if (! readMetadata(input, buffers))
      {
        STDERR_ERROR("  Failed to read metadata, continue searching at tellg={}", afterMagicPos);
        input.clear();
        input.seekg(afterMagicPos);
      }
    }
    else if (magic == dataMagic)
    {
      STDERR_INFO("Magic number found, read data at tellg={}", afterMagicPos);
      if (! readData(input, buffers))
      {
        STDERR_ERROR("  Failed to read data, continue searching at tellg={}", afterMagicPos);
        input.clear();
        input.seekg(afterMagicPos);
      }
    }
    else
    {
      // not a magic number, seek back
      input.seekg(-7, std::ios_base::cur);
    }
  }

  if (input.eof())
  {
    STDERR_INFO("Done reading input");
  }
  else
  {
    STDERR_ERROR("Failure while reading input, continue anyway");
  }

  STDERR_INFO("Write output");

  // make sure metadata precedes data, and everything is grouped by sessions
  const auto cmp = [](auto&& a, auto&& b)
  {
    return a.session == b.session ? a.type < b.type : a.session < b.session;
  };
  std::sort(buffers.begin(), buffers.end(), cmp);

  std::size_t offset = 0;
  for (const RecoveredBuffer& buffer : buffers)
  {
    STDERR_INFO("Write {} bytes of recovered {} to output at offset {}", buffer.buffer.size(), buffer.type, offset);
    output.write(buffer.buffer.data(), std::streamsize(buffer.buffer.size()));
    offset += buffer.buffer.size();

    if (! output)
    {
      STDERR_ERROR("Failure while writing output");
      return 4;

    }
  }

  STDERR_INFO("Done writing output");

  return 0;
}
