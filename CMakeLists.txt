cmake_minimum_required (VERSION 2.6)
project (Binlog)
LIST(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(CTest)
include(AddressSanitizer)

# Set default type to Release with debug symbols
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Debug|Release|RelWithDebInfo|MinSizeRel" FORCE)
endif(NOT CMAKE_BUILD_TYPE)

if (MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Werror -Wconversion -Wsign-conversion)

  # TODO for gcc, add -Wlogical-op -Wold-style-cast -Wsuggest-override
  # TODO for clang, add -Wno-missing-braces
endif()

# UnitTest
add_executable(UnitTest test/unit/UnitTest.cpp)

add_test(NAME UnitTest
         COMMAND UnitTest)
target_compile_features(UnitTest PRIVATE cxx_std_14)
