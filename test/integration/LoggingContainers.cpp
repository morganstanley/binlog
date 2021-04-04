#include <binlog/binlog.hpp>

#include <iostream>

#include <array>
#include <forward_list>
#include <list>
#include <map>
#include <set>
#include <vector>

int main()
{
  // Standard containers

  //[sequence
  std::vector<int>       vec{1,2,3};
  std::array<int, 3>     arr{4,5,6};
  std::forward_list<int> lst{7,8,9};
  BINLOG_INFO("Sequence containers: {} {} {}", vec, arr, lst);
  // Outputs: Sequence containers: [1, 2, 3] [4, 5, 6] [7, 8, 9]
  //]

  std::deque<int> deq{1,2,3};
  std::list<int> list{4,5,6};
  std::vector<bool> vb{true, false, true};
  BINLOG_INFO("More sequence containers: {} {} {}", deq, list, vb);
  // Outputs: More sequence containers: [1, 2, 3] [4, 5, 6] [true, false, true]

  BINLOG_INFO("Empty containers: {} {}", std::vector<int>{}, std::list<bool>{});
  // Outputs: Empty containers: [] []

  //[associative
  std::set<int> set{4,8,15,16,23,42};
  std::map<char, std::string> map{{'a', "alpha"}, {'b', "beta"}};
  BINLOG_INFO("Associative containers: {} {}", set, map);
  // Outputs: Associative containers: [4, 8, 15, 16, 23, 42] [(a, alpha), (b, beta)]
  //]

  std::multiset<int> ms{1,2,2,3,1};
  std::multimap<int, char> mm{{1, 'a'}, {1, 'b'}, {2, 'c'}};
  BINLOG_INFO("Multimap and set: {} {}", ms, mm);
  // Outputs: Multimap and set: [1, 1, 2, 2, 3] [(1, a), (1, b), (2, c)]

  // unordered_set and map cannot be tested this way,
  // since the order of the elements is not specified.

  //[carray
  int array[] = {1, 2, 3};
  BINLOG_INFO("Array: {}", binlog::array_view(array, 3));
  // Outputs: Array: [1, 2, 3]
  //]

  binlog::consume(std::cout);
  return 0;
}
