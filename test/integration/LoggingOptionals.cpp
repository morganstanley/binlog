//[stdopt
#include <binlog/adapt_stdoptional.hpp> // must be included to log std::optional, requires C++17
//]

#include <binlog/binlog.hpp>

#include <boost/optional/optional.hpp>

#include <optional>
#include <iostream>

//[optspec
namespace mserialize { namespace detail {
  template <typename T> struct is_optional<boost::optional<T>> : std::true_type {};
}}
//]

int main()
{
  // Std optional - loggable if adapt_stdoptional.hpp is included

  //[stdopt

  std::optional<int> opt(123);
  std::optional<int> emptyOpt;
  BINLOG_INFO("Optionals: {} {}", opt, emptyOpt);
  // Outputs: Optionals: 123 {null}
  //]

  // Boost optional - loggable with the is_optional specialization above

  boost::optional<int> bopt(123);
  boost::optional<int> bemptyOpt;
  BINLOG_INFO("Boost optionals: {} {}", bopt, bemptyOpt);
  // Outputs: Boost optionals: 123 {null}

  binlog::consume(std::cout);
  return 0;
}
