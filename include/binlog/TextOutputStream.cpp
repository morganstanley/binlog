#include <binlog/TextOutputStream.hpp>

#include <binlog/Entries.hpp> // Event
#include <binlog/EntryStream.hpp> // RangeEntryStream
#include <binlog/Range.hpp>

#include <utility> // move

namespace binlog {

TextOutputStream::TextOutputStream(std::ostream& out, std::string eventFormat, std::string dateFormat)
  :_out(out),
   _printer(std::move(eventFormat), std::move(dateFormat))
{}

TextOutputStream& TextOutputStream::write(const char* data, std::streamsize size)
{
  const Range range{data, data + size};
  RangeEntryStream entryStream(range);

  while (const Event* event = _eventStream.nextEvent(entryStream))
  {
    _printer.printEvent(_out, *event, _eventStream.writerProp(), _eventStream.clockSync());
  }

  return *this;
}

} // namespace binlog
