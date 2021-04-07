//[ec
#include <binlog/adapt_stderrorcode.hpp> // must be included to log std::error_code
//]

#include <binlog/binlog.hpp>

#include <iostream>
#include <string>
#include <system_error>

class error_category : public std::error_category
{
public:
  const char* name() const noexcept override { return "error_category"; }
  std::string message(int) const override { return "Success"; }
};

int main()
{
  //[ec

  std::error_code ec;
  //]

  // Use a custom error category to make sure the message is platform-agnostic
  error_category category;
  ec.assign(0, category);

  //[ec
  BINLOG_INFO("ec: {}", ec);
  // Outputs: ec: Success
  //]

  binlog::consume(std::cout);
  return 0;
}
