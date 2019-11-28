#include <binlog/binlog.hpp>

#include <boost/container/deque.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/container/list.hpp>
#include <boost/container/map.hpp>
#include <boost/container/set.hpp>
#include <boost/container/slist.hpp>
#include <boost/container/vector.hpp>

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

  std::vector<int>       vec{1,2,3};
  std::array<int, 3>     arr{4,5,6};
  std::forward_list<int> lst{7,8,9};
  BINLOG_INFO("Sequence containers: {} {} {}", vec, arr, lst);
  // Outputs: Sequence containers: [1, 2, 3] [4, 5, 6] [7, 8, 9]

  std::deque<int> deq{1,2,3};
  std::list<int> list{4,5,6};
  std::vector<bool> vb{true, false, true};
  BINLOG_INFO("More sequence containers: {} {} {}", deq, list, vb);
  // Outputs: More sequence containers: [1, 2, 3] [4, 5, 6] [true, false, true]

  std::set<int> set{4,8,15,16,23,42};
  std::map<char, std::string> map{{'a', "alpha"}, {'b', "beta"}};
  BINLOG_INFO("Associative containers: {} {}", set, map);
  // Outputs: Associative containers: [4, 8, 15, 16, 23, 42] [(a, alpha), (b, beta)]

  std::multiset<int> ms{1,2,2,3,1};
  std::multimap<int, char> mm{{1, 'a'}, {1, 'b'}, {2, 'c'}};
  BINLOG_INFO("Multimap and set: {} {}", ms, mm);
  // Outputs: Multimap and set: [1, 1, 2, 2, 3] [(1, a), (1, b), (2, c)]

  // unordered_set and map cannot be tested this way,
  // since the order of the elements is not specified.

  int array[] = {1, 2, 3};
  BINLOG_INFO("Array: {}", binlog::array_view(array, 3));
  // Outputs: Array: [1, 2, 3]

  // Boost containers - no specific adoption is required

  boost::container::deque<int> bdq{1,2,3};
  boost::container::list<int> bl{7,8,9};
  boost::container::slist<int> bsl{10,20,30};
  boost::container::vector<int> bv{};
  BINLOG_INFO("Boost sequence containers: {} {} {} {}", bdq, bl, bsl, bv);
  // Outputs: Boost sequence containers: [1, 2, 3] [7, 8, 9] [10, 20, 30] []

  boost::container::map<int,int> bm{{1,1},{2,4},{3,9}};
  boost::container::set<int> bs{4,5,6};
  boost::container::flat_map<int,int> bfm{{1,1},{2,4},{3,9}};
  boost::container::flat_set<int> bfs{2,3,5,8};
  BINLOG_INFO("Boost associative containers: {} {} {} {}", bm, bs, bfm, bfs);
  // Outputs: Boost associative containers: [(1, 1), (2, 4), (3, 9)] [4, 5, 6] [(1, 1), (2, 4), (3, 9)] [2, 3, 5, 8]

  binlog::consume(std::cout);
  return 0;
}
