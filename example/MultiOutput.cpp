#include <binlog/binlog.hpp>

//[ostream
#include <binlog/EventFilter.hpp>
#include <binlog/TextOutputStream.hpp> // requires binlog library to be linked
//]

#include <fstream>
#include <iostream>
#include <stdexcept>

//[ostream

// Write complete binlog output to `binary`,
// and also write error and above events to `text` - as text.
class MultiOutputStream
{
public:
  MultiOutputStream(std::ostream& binary, std::ostream& text)
    :_binary(binary),
     _text(text),
     _filter([](const binlog::EventSource& source) {
        return source.severity >= binlog::Severity::error;
     })
  {}

  MultiOutputStream& write(const char* buffer, std::streamsize size)
  {
    _binary.write(buffer, size);

    try
    {
      _filter.writeAllowed(buffer, std::size_t(size), _text);
    }
    catch (const std::runtime_error& ex)
    {
      std::cerr << "Failed to convert buffer to text: " << ex.what() << "\n";
    }

    return *this;
  }

private:
  std::ostream& _binary;
  binlog::TextOutputStream _text;
  binlog::EventFilter _filter;
};
//]

int main()
{
  const std::string path = "multioutput.blog";
  BINLOG_INFO("Events below Error will be written only to the logfile: {}", path);

  BINLOG_ERROR("Error and Critical events will be also shown on the standard error");

  //[usage
  std::ofstream logfile(path, std::ofstream::out|std::ofstream::binary);
  MultiOutputStream output(logfile, std::cerr);
  binlog::consume(output);
  //]

  return 0;
}
