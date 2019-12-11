#ifndef TEST_UNIT_MSERIALIZE_TEST_TYPE_LISTS_HPP
#define TEST_UNIT_MSERIALIZE_TEST_TYPE_LISTS_HPP

#include "custom_array.hpp"

#include <boost/mpl/list.hpp>

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

using arithmetic_types = boost::mpl::list<
  bool,
  std::int8_t, std::int16_t, std::int32_t, std::int64_t,
  std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t,
  float, double, long double
>;

using float_types = boost::mpl::list<
  float, double, long double
>;

template <typename T>
using sequence_types = boost::mpl::list<
  T[10], std::array<T, 10>,
  std::deque<T>, std::forward_list<T>,
  std::list<T>, std::vector<T>,
  test::CustomArray<T, 10>
>;

template <typename T>
using var_size_sequence_types = boost::mpl::list<
  std::deque<T>, std::forward_list<T>,
  std::list<T>, std::vector<T>
>;

template <typename T>
using smart_pointers = boost::mpl::list<
  std::unique_ptr<T>,
  std::shared_ptr<T>
>;

template <typename T>
using sets = boost::mpl::list<
  std::set<T>,
  std::multiset<T>,
  std::unordered_set<T>
>;

template <typename K, typename V>
using maps = boost::mpl::list<
  std::map<K, V>,
  std::multimap<K, V>,
  std::unordered_map<K, V>
>;

#endif // TEST_UNIT_MSERIALIZE_TEST_TYPE_LISTS_HPP
