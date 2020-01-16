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
 * `format` and `dateFormat`, sorted by event clock.
 *
 * First buffer every event in `input`, then sort and print them.
 *
 * @see PrettyPrinter on `format` and `dateFormat`.
 * @throws std::runtime_error if invalid binlog entry found in `input`.
 */
void printSortedEvents(std::istream& input, std::ostream& output, const std::string& format, const std::string& dateFormat);

#endif // BINLOG_BIN_PRINTERS_HPP
