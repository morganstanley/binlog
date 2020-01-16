#ifndef BINLOG_BIN_PRINTERS_HPP
#define BINLOG_BIN_PRINTERS_HPP

#include <iosfwd>
#include <string>

/**
 * Print the events in `input` to output, according to
 * `format` and `dateFormat`.
 *
 * @see PrettyPrinter on `format` and `dateFormat`.
 * @throws std::runtime_error if invalid binlog entry found in `input`.
 */
void printEvents(std::istream& input, std::ostream& output, const std::string& format, const std::string& dateFormat);

/**
 * Print the events in `input` to output, according to
 * `format` and `dateFormat`, approximately sorted by event clock.
 *
 * To allow online usage, and remain efficient for large logfiles,
 * the sorting is approximated by:
 *
 *  - buffer events of a 60 seconds window
 *  - sort and print the events of the first 30 seconds
 *  - loop until done, sort and print the remaining buffer
 *
 * @see PrettyPrinter on `format` and `dateFormat`.
 * @throws std::runtime_error if invalid binlog entry found in `input`.
 */
void printSortedEvents(std::istream& input, std::ostream& output, const std::string& format, const std::string& dateFormat);

#endif // BINLOG_BIN_PRINTERS_HPP
