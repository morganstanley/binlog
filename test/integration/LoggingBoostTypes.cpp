#include <binlog/binlog.hpp>

#include <boost/container/deque.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/container/list.hpp>
#include <boost/container/map.hpp>
#include <boost/container/set.hpp>
#include <boost/container/slist.hpp>
#include <boost/container/string.hpp>
#include <boost/container/vector.hpp>

#include <boost/smart_ptr/scoped_ptr.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/weak_ptr.hpp>

#include <boost/optional/optional.hpp>

#include <iostream>

//[optspec
namespace mserialize { namespace detail {
  template <typename T> struct is_optional<boost::optional<T>> : std::true_type {};
}}
//]

// The boost container constructors throw, and clang tidy does not like this
// NOLINTBEGIN(bugprone-exception-escape)

int main()
{
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

  // Boost string - no specific adoption is required

  boost::container::string str = "foo";
  BINLOG_INFO("Boost string: {}", str);
  // Outputs: Boost string: foo

  // Boost pointers - no specific adoption is required

  boost::scoped_ptr<int> bscoped(new int(1));
  boost::shared_ptr<int> bshared(new int(2));
  BINLOG_INFO("Boost pointers: {} {}", bscoped, bshared);
  // Outputs: Boost pointers: 1 2

  bshared.reset();
  BINLOG_INFO("Empty Boost pointers: {} {}", boost::scoped_ptr<bool>{}, bshared);
  // Outputs: Empty Boost pointers: {null} {null}

  static_assert(
    !mserialize::detail::is_serializable<boost::weak_ptr<int>>::value,
    "boost::weak_ptr is not loggable"
  );

  // Boost optional - loggable with the is_optional specialization above

  boost::optional<int> bopt(123);
  boost::optional<int> bemptyOpt;
  BINLOG_INFO("Boost optionals: {} {}", bopt, bemptyOpt);
  // Outputs: Boost optionals: 123 {null}

  binlog::consume(std::cout);
  return 0;
}

// NOLINTEND(bugprone-exception-escape)
