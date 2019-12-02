#include <binlog/binlog.hpp>

#include <boost/optional/optional.hpp>
#include <boost/smart_ptr/scoped_ptr.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/weak_ptr.hpp>

#include <iostream>

#include <memory>

//[optspec
namespace mserialize { namespace detail {
  template <typename T> struct is_optional<boost::optional<T>> : std::true_type {};
}}
//]

int main()
{
  // Standard pointers

  //[stdptr
  int* ptr = nullptr;
  std::unique_ptr<int> uptr(std::make_unique<int>(1));
  std::shared_ptr<int> sptr(std::make_shared<int>(2));
  BINLOG_INFO("Pointers: {} {} {}", ptr, uptr, sptr);
  // Outputs: Pointers: {null} 1 2
  //]

  int value = 3;
  ptr = &value;
  uptr.reset();
  sptr.reset();
  BINLOG_INFO("Pointers: {} {} {}", ptr, uptr, sptr);
  // Outputs: Pointers: 3 {null} {null}

  // weak_ptr is not loggable by design, must be .lock()-ed first
  static_assert(
    !mserialize::detail::is_serializable<std::weak_ptr<int>>::value,
    "std::weak_ptr is not loggable"
  );

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

  boost::optional<int> opt(123);
  boost::optional<int> emptyOpt;
  BINLOG_INFO("Boost optionals: {} {}", opt, emptyOpt);
  // Outputs: Boost optionals: 123 {null}

  binlog::consume(std::cout);
  return 0;
}
