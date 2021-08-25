#ifndef BINLOG_ADAPT_STDDURATION_HPP
#define BINLOG_ADAPT_STDDURATION_HPP

#include <binlog/adapt_struct.hpp>

#include <chrono>

BINLOG_ADAPT_TEMPLATE((typename Rep), (std::chrono::duration<Rep, std::nano>), count)
BINLOG_ADAPT_TEMPLATE((typename Rep), (std::chrono::duration<Rep, std::micro>), count)
BINLOG_ADAPT_TEMPLATE((typename Rep), (std::chrono::duration<Rep, std::milli>), count)
BINLOG_ADAPT_TEMPLATE((typename Rep), (std::chrono::duration<Rep, std::ratio<1>>), count)
BINLOG_ADAPT_TEMPLATE((typename Rep), (std::chrono::duration<Rep, std::ratio<60>>), count)
BINLOG_ADAPT_TEMPLATE((typename Rep), (std::chrono::duration<Rep, std::ratio<3600>>), count)

#endif // BINLOG_ADAPT_STDDURATION_HPP
