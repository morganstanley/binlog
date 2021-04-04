#include <binlog/binlog.hpp>

#include <iostream>
#include <memory>

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

  // Addresses (raw pointer value)

  std::uintptr_t address = 0xf777123;

  int* any_pointer = nullptr;
  void* void_pointer = nullptr;
  std::memcpy(&any_pointer, &address, sizeof(any_pointer));
  std::memcpy(&void_pointer, &address, sizeof(void_pointer));

  //[address
  BINLOG_INFO("Raw pointer value: {} {}", binlog::address(any_pointer), void_pointer);
  // Outputs: Raw pointer value: 0xF777123 0xF777123
  //]

  const int* const_any_pointer = nullptr;
  const void* const_void_pointer = nullptr;
  std::memcpy(&const_any_pointer, &address, sizeof(const_any_pointer));
  std::memcpy(&const_void_pointer, &address, sizeof(const_void_pointer));

  BINLOG_INFO("Const pointer value: {} {}", binlog::address(const_any_pointer), const_void_pointer);
  // Outputs: Const pointer value: 0xF777123 0xF777123

  binlog::consume(std::cout);
  return 0;
}
