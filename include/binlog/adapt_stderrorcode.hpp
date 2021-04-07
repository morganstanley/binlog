#ifndef BINLOG_ADAPT_STDERRORCODE_HPP
#define BINLOG_ADAPT_STDERRORCODE_HPP

// Make std::error_code loggable by including this file

#include <binlog/adapt_struct.hpp>

BINLOG_ADAPT_STRUCT(std::error_code, message)

#endif // BINLOG_ADAPT_STDERRORCODE_HPP
