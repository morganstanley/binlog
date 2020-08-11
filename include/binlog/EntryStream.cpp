#include <binlog/EntryStream.hpp>

#include <cstdint>
#include <istream>
#include <stdexcept>

namespace binlog {

IstreamEntryStream::IstreamEntryStream(std::istream& input)
  :_input(input)
{}

Range IstreamEntryStream::nextEntryPayload()
{
  std::uint32_t size;
  _input.read(reinterpret_cast<char*>(&size), sizeof(size));
  if (_input.gcount() == 0)
  {
    return {}; // eof
  }

  if (! _input) // found some bytes, but not enough
  {
    rewind(_input.gcount());
    throw std::runtime_error("Failed to read entry size from istream, only got "
      + std::to_string(_input.gcount()) + " bytes, expected " + std::to_string(sizeof(size)));
  }

  // TODO(benedek) protect agains bad alloc by limiting size?
  _buffer.resize(size);
  _input.read(_buffer.data(), std::streamsize(size));
  if (! _input) // payload is truncated
  {
    rewind(std::streamsize(sizeof(size)) + _input.gcount());
    throw std::runtime_error("Failed to read entry payload from istream, only got "
      + std::to_string(_input.gcount()) + " bytes, expected " + std::to_string(size));
  }

  return Range{_buffer.data(), _buffer.size()};
}

void IstreamEntryStream::rewind(std::streamsize size)
{
  _input.clear();
  _input.seekg(-1 * size, std::ios_base::cur);
}

RangeEntryStream::RangeEntryStream(Range input)
  :_input(input)
{}

Range RangeEntryStream::nextEntryPayload()
{
  if (_input.empty()) { return {}; } // eof
  const std::uint32_t size = _input.read<std::uint32_t>();
  return Range{_input.view(size), size};
}

} // namespace binlog
