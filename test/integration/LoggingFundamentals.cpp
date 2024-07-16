#include <binlog/binlog.hpp>

#include <cstdint>
#include <iostream>

template <typename T>
void logSigned()
{
  T a = -20;
  const T b = 30;
  const T& c = b;
  BINLOG_INFO("{} {} {} {}", a, b, c, T(-40));
}

template <typename T>
void logUnsigned()
{
  T a = 20;
  const T b = 30;
  const T& c = b;
  BINLOG_INFO("{} {} {} {}", a, b, c, T(40));
}

template <typename T>
void logDouble()
{
  T f = 1234234.0234242;
  BINLOG_INFO("{}", f);
}

int main()
{
  logSigned<signed short>(); // NOLINT
  logSigned<signed int>();
  logSigned<signed long>(); // NOLINT
  logSigned<signed long long>(); // NOLINT
  // Outputs: -20 30 30 -40
  // Outputs: -20 30 30 -40
  // Outputs: -20 30 30 -40
  // Outputs: -20 30 30 -40

  logSigned<std::int8_t>();
  logSigned<std::int16_t>();
  logSigned<std::int32_t>();
  logSigned<std::int64_t>();
  // Outputs: -20 30 30 -40
  // Outputs: -20 30 30 -40
  // Outputs: -20 30 30 -40
  // Outputs: -20 30 30 -40

  logUnsigned<unsigned short>(); // NOLINT
  logUnsigned<unsigned int>();
  logUnsigned<unsigned long>(); // NOLINT
  logUnsigned<unsigned long long>(); // NOLINT
  // Outputs: 20 30 30 40
  // Outputs: 20 30 30 40
  // Outputs: 20 30 30 40
  // Outputs: 20 30 30 40

  logUnsigned<std::uint8_t>();
  logUnsigned<std::uint16_t>();
  logUnsigned<std::uint32_t>();
  logUnsigned<std::uint64_t>();
  // Outputs: 20 30 30 40
  // Outputs: 20 30 30 40
  // Outputs: 20 30 30 40
  // Outputs: 20 30 30 40

  logSigned<float>();
  logSigned<double>();
  logSigned<long double>();
  // Outputs: -20 30 30 -40
  // Outputs: -20 30 30 -40
  // Outputs: -20 30 30 -40

  logDouble<double>();
  logDouble<long double>();
  // Outputs: 1234234.0234242
  // Outputs: 1234234.0234242

  char c = 'A';
  const char cc = 'B';
  const char& ccr = 'C';
  BINLOG_INFO("{} {} {} {}", c, cc, ccr, 'D');
  // Outputs: A B C D

  bool b = true;
  const bool cb = false;
  bool& br = b;
  BINLOG_INFO("{} {} {} {}", b, cb, br, false);
  // Outputs: true false true false

  binlog::consume(std::cout);
  return 0;
}
