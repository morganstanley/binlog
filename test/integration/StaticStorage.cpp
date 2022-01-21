#include <binlog/binlog.hpp>

#include <dlfcn.h> // dlopen

#include <iostream>

// Test that the static storage implementation
// does not cause section type conflicts. See:
// https://stackoverflow.com/questions/35091862/

template <typename>
void template_function()
{
  BINLOG_INFO("Log from template function");
}

inline void inline_function()
{
  BINLOG_INFO("Log from inline function");
}

void shared_lib_function();

void call_dynamic_lib_function(const char* path)
{
  void* dso = dlopen(path, RTLD_LAZY);
  if (dso == nullptr)
  {
    BINLOG_ERROR("Failed to load dso: {}", dlerror()); // NOLINT
    return;
  }

  //[addEventSources
  binlog::default_session().addEventSources(path);
  //]

  void* dynamic_lib_function_p = dlsym(dso, "dynamic_lib_function");
  if (dynamic_lib_function_p == nullptr)
  {
    BINLOG_ERROR("Failed to get function pointer from dso: {}", dlerror()); // NOLINT
    dlclose(dso);
    return;
  }

  void (*dynamic_lib_function)() = nullptr;
  std::memcpy(&dynamic_lib_function, &dynamic_lib_function_p, sizeof(dynamic_lib_function));

  dynamic_lib_function();

  dlclose(dso);
}

int main()
{
  BINLOG_INFO("Log from main");
  // Outputs: Log from main

  template_function<int>();
  // Outputs: Log from template function

  inline_function();
  // Outputs: Log from inline function

  shared_lib_function();
  // Outputs: Log from shared lib function

  // dlopen requires additional care
  call_dynamic_lib_function("libStaticStorageDynamicLib.so");
  // Outputs: Log from dynamic lib function

  binlog::consume(std::cout);
  return 0;
}
