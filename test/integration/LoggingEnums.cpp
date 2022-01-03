#include <binlog/binlog.hpp>

#include <iostream>

//[basic
enum Enum { Alpha = 123, Beta = 124 };
//]

//[adapted
enum AdaptedEnum { Gamma, Delta };
BINLOG_ADAPT_ENUM(AdaptedEnum, Gamma, Delta)
//]

//[scoped
enum class ScopedEnum { Epsilon, Phi };
BINLOG_ADAPT_ENUM(ScopedEnum, Epsilon, Phi)
//]

enum PartiallyAdaptedEnum { Rho = 20, Sigma = 30, Tau = 40 };
BINLOG_ADAPT_ENUM(PartiallyAdaptedEnum, Rho, Sigma)

//[shadowed
struct Nest {
  enum Nested
  {
    Bird
  } Nested; // Note: field name is same as enum name
};

// BINLOG_ADAPT_ENUM(Nest::Nested, Bird) // Doesn't work, type is shadowed by field.
typedef enum Nest::Nested NestedT; // workaround: use elaborated type specifier // NOLINT
BINLOG_ADAPT_ENUM(NestedT, Bird)
//]

int main()
{
  //[basic
  BINLOG_INFO("Enum: {}", Alpha);
  // Outputs: Enum: 123
  //]

  //[adapted

  BINLOG_INFO("Adapted enum: {}", Delta);
  // Outputs: Adapted enum: Delta
  //]

  //[scoped

  BINLOG_INFO("Scoped enum: {}", ScopedEnum::Epsilon);
  // Outputs: Scoped enum: Epsilon

  //]

  BINLOG_INFO("Partially adapted enum: {} {} {}", Rho, Sigma, Tau);
  // Outputs: Partially adapted enum: Rho Sigma 0x28

  BINLOG_INFO("Shadowed enum type: {}", Nest::Nested::Bird);
  // Outputs: Shadowed enum type: Bird

  binlog::consume(std::cout);
  return 0;
}
