#ifndef BINLOG_TEXT_OUTPUT_STREAM_HPP
#define BINLOG_TEXT_OUTPUT_STREAM_HPP

#include <binlog/EventStream.hpp>
#include <binlog/PrettyPrinter.hpp>

#include <ostream>
#include <string>

namespace binlog {

/**
 * Convert a binlog stream to text.
 *
 * Models mserialize::OutputStream.
 * Suitable to convert data consumed from Session directly.
 */
class TextOutputStream
{
public:
  /**
   * Will write events converted to text to `out`,
   * according to the specified formats.
   *
   * @see PrettyPrinter.hpp on the available placeholders.
   *
   * `out` must remain valid as long as *this is valid.
   */
  explicit TextOutputStream(
    std::ostream& out,
    std::string eventFormat = "%S %C [%d] %n %m (%G:%L)\n",
    std::string dateFormat = "%m/%d %H:%M:%S.%N"
  );

  /**
   * Write the binlog entries in [data, data+size) as text
   * to the output as specified in the constructor.
   *
   * The entries in the buffer must be complete,
   * no partial entry is allowed.
   *
   * @throw std::runtime_error on invalid input
   */
  TextOutputStream& write(const char* data, std::streamsize size);

private:
  std::ostream& _out;
  binlog::EventStream _eventStream;
  binlog::PrettyPrinter _printer;
};

} // namespace binlog

#endif // BINLOG_TEXT_OUTPUT_STREAM_HPP
