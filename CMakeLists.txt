cmake_minimum_required(VERSION 2.6)
project(Binlog)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(CTest)
include(AddressSanitizer)
include(Coverage)
include(MarkdownToHtml)
include(OptionalCompileOption)

# Set default type to Release with debug symbols
if (NOT CMAKE_BUILD_TYPE AND NOT BINLOG_GEN_COVERAGE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Debug|Release|RelWithDebInfo|MinSizeRel" FORCE)
endif ()

# Setup compile options
if (MSVC)
  add_compile_options(/W4)
else ()
  add_compile_options(-Wall -Wextra -Werror -pedantic)
  add_optional_compile_options(
    -Wconversion -Wsign-conversion -Wold-style-cast -Wsuggest-override
  )
endif ()

# Clang Tidy
# BOOST_TEST triggers C vararg warnings
# BOOST_AUTO_TEST_CASE triggers exception in global var ctor warning (cert-err58-cpp)
# [i,o]stream read/write usage requires reinterpret_cast
# std::runtime_error is not nothrow copy constructible (cert-err60-cpp)
# Aliases are not useful (hicpp)
# llvm-header-guard derives the macro name from the absolute path sometimes
if (BINLOG_USE_CLANG_TIDY)
  set(CMAKE_CXX_CLANG_TIDY clang-tidy
    -checks=*,-fuchsia-*,-cppcoreguidelines-pro-type-vararg,-cppcoreguidelines-pro-type-reinterpret-cast,-cppcoreguidelines-pro-bounds-array-to-pointer-decay,-hicpp-*,-cert-err58-cpp,-cert-err60-cpp,-readability-named-parameter,-modernize-use-auto,-llvm-header-guard,-google-runtime-references
    -warnings-as-errors=*
    -header-filter=.*
  )
endif ()

# Boost
find_package(Boost 1.59.0 REQUIRED COMPONENTS unit_test_framework)

# Mserialize

add_library(mserialize INTERFACE)
target_include_directories(mserialize INTERFACE include)

# UnitTest
add_executable(UnitTest test/unit/UnitTest.cpp
  test/unit/mserialize/roundtrip.cpp
)
  target_compile_definitions(UnitTest PRIVATE
    BOOST_TEST_DYN_LINK
    BOOST_TEST_NO_MAIN
  )
  target_include_directories(UnitTest SYSTEM PRIVATE
    ${Boost_INCLUDE_DIRS}
  )
  target_link_libraries(UnitTest Boost::unit_test_framework)
  target_link_libraries(UnitTest mserialize)

add_test(NAME UnitTest
         COMMAND UnitTest --log_level=test_suite --color_output=true)
target_compile_features(UnitTest PRIVATE cxx_std_14)

# Documentation
markdown_to_html_group(Documentation UserGuide)
