#include <binlog/binlog.hpp>

#include <iostream>

#include <string>
#include <tuple>
#include <utility> // pair

int main()
{
  std::tuple<> empty;
  BINLOG_INFO("Empty tuple: {}", empty);
  // Outputs: Empty tuple: ()

  std::pair<int, char> p{1, 'a'};
  std::tuple<std::string, bool, int> t{"foo", true, 2};
  BINLOG_INFO("Pair: {}, Tuple: {}", p, t);
  // Outputs: Pair: (1, a), Tuple: (foo, true, 2)

  binlog::consume(std::cout);
  return 0;
}
