//[stdopt
#include <binlog/adapt_stdoptional.hpp> // must be included to log std::optional, requires C++17
//]

#include <binlog/binlog.hpp>

#include <optional>
#include <iostream>

int main()
{
  // Std optional - loggable if adapt_stdoptional.hpp is included

  //[stdopt

  std::optional<int> opt(123);
  std::optional<int> emptyOpt;
  BINLOG_INFO("Optionals: {} {}", opt, emptyOpt);
  // Outputs: Optionals: 123 {null}
  //]

  binlog::consume(std::cout);
  return 0;
}
