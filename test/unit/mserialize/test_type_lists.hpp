#ifndef TEST_UNIT_MSERIALIZE_TEST_TYPE_LISTS_HPP
#define TEST_UNIT_MSERIALIZE_TEST_TYPE_LISTS_HPP

#include "custom_array.hpp"

#include <cstdint>

#include <array>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define ARITHMETIC_TYPES                                     \
  bool,                                                      \
  std::int8_t, std::int16_t, std::int32_t, std::int64_t,     \
  std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t, \
  float, double, long double                                 \
  /**/

#define FLOAT_TYPES float, double, long double

#define SEQUENCE_TYPES(T)              \
  T[10], std::array<T, 10>,            \
  std::deque<T>, std::forward_list<T>, \
  std::list<T>, std::vector<T>,        \
  test::CustomArray<T, 10>             \
  /**/

#define SEQUENCE_TYPES_TO_STRING(T)        \
 TYPE_TO_STRING(T[10]); /* NOLINT(bugprone-macro-parentheses) */ \
 TYPE_TO_STRING(std::array<T,10>);         \
 TYPE_TO_STRING(std::deque<T>);            \
 TYPE_TO_STRING(std::forward_list<T>);     \
 TYPE_TO_STRING(std::list<T>);             \
 TYPE_TO_STRING(std::vector<T>);           \
 TYPE_TO_STRING(test::CustomArray<T, 10>)  \
 /**/

#define VAR_SIZE_SEQUENCE_TYPES(T)     \
  std::deque<T>, std::forward_list<T>, \
  std::list<T>, std::vector<T>         \
  /**/

#define SMART_POINTERS(T) std::unique_ptr<T>, std::shared_ptr<T>

#define SMART_POINTERS_TO_STRING(T)   \
  TYPE_TO_STRING(std::unique_ptr<T>); \
  TYPE_TO_STRING(std::shared_ptr<T>)  \
  /**/

#define SETS(T) std::set<T>, std::multiset<T>, std::unordered_set<T>

#define SETS_TO_STRING(T)               \
  TYPE_TO_STRING(std::set<T>);          \
  TYPE_TO_STRING(std::multiset<T>);     \
  TYPE_TO_STRING(std::unordered_set<T>) \
  /**/

#define MAPS(K, V) std::map<K, V>, std::multimap<K, V>, std::unordered_map<K, V>

#define MAPS_TO_STRING(K, V)               \
  TYPE_TO_STRING(std::map<K, V>);          \
  TYPE_TO_STRING(std::multimap<K, V>);     \
  TYPE_TO_STRING(std::unordered_map<K, V>) \
  /**/

#endif // TEST_UNIT_MSERIALIZE_TEST_TYPE_LISTS_HPP
