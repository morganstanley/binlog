#include <binlog/binlog.hpp>

#include <iostream>

enum Enum { Alpha = 123, Beta = 124 };

enum AdaptedEnum { Gamma, Delta };
BINLOG_ADAPT_ENUM(AdaptedEnum, Gamma, Delta)

enum class ScopedEnum { Epsilon, Phi };
BINLOG_ADAPT_ENUM(ScopedEnum, Epsilon, Phi)

enum PartiallyAdaptedEnum { Rho = 20, Sigma = 30, Tau = 40 };
BINLOG_ADAPT_ENUM(PartiallyAdaptedEnum, Rho, Sigma)

int main()
{
  BINLOG_INFO("Enum: {}", Alpha);
  // Outputs: Enum: 123

  BINLOG_INFO("Adapted enum: {}", Delta);
  // Outputs: Adapted enum: Delta

  BINLOG_INFO("Scoped enum: {}", ScopedEnum::Epsilon);
  // Outputs: Scoped enum: Epsilon

  BINLOG_INFO("Partially adapted enum: {} {} {}", Rho, Sigma, Tau);
  // Outputs: Partially adapted enum: Rho Sigma 0x28

  binlog::consume(std::cout);
  return 0;
}
