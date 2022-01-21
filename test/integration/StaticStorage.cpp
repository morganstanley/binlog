#include <binlog/binlog.hpp>

#include <iostream>

// Test that the static storage implementation
// does not cause section type conflicts. See:
// https://stackoverflow.com/questions/35091862/

template <typename>
void template_function()
{
  BINLOG_INFO("Log from template function");
}

inline void inline_function()
{
  BINLOG_INFO("Log from inline function");
}

void shared_lib_function();

int main()
{
  BINLOG_INFO("Log from main");
  // Outputs: Log from main

  template_function<int>();
  // Outputs: Log from template function

  inline_function();
  // Outputs: Log from inline function

  shared_lib_function();
  // Outputs: Log from shared lib function

  binlog::consume(std::cout);
  return 0;
}
