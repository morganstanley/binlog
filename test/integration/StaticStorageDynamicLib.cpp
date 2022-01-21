#include <binlog/binlog.hpp>

extern "C" void dynamic_lib_function()
{
  BINLOG_INFO("Log from dynamic lib function");
}
