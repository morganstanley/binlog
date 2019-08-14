#ifndef TEST_UNIT_MSERIALIZE_TEST_ENUMS_HPP
#define TEST_UNIT_MSERIALIZE_TEST_ENUMS_HPP

#include <cstdint>

namespace test {

enum CEnum
{
  Alpha,
  Bravo,
  Charlie
};

enum class EnumClass
{
  Delta,
  Echo,
  Foxtrot
};

enum class LargeEnumClass : std::int64_t
{
  Golf = INT64_MIN,
  Hotel = 0,
  India = INT64_MAX
};

} // namespace test

#endif // TEST_UNIT_MSERIALIZE_TEST_ENUMS_HPP
