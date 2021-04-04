#include <binlog/binlog.hpp>

#include <mserialize/string_view.hpp>

#include <iostream>

#include <string>

int main()
{
  //[stdstr
  std::string str = "String";
  BINLOG_INFO("Hello {}!", str);
  // Outputs: Hello String!
  //]

  const char* ccs = "bar";
  mserialize::string_view sv = "baz";
  BINLOG_INFO("Strings: {}, {}", ccs, sv);
  // Outputs: Strings: bar, baz

  // const char arr[] = "string"; is not tested,
  // because of the 0 terminator at the end.
  // This is not worked around, as there's no way
  // to distinguish this from:
  // const char arr[] = {3,2,1,0};

  const char arr[3] = {'f','o','o'};
  BINLOG_INFO("Char array: {}", arr);
  // Outputs: Char array: foo

  binlog::consume(std::cout);
  return 0;
}
