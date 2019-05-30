cmake_minimum_required(VERSION 2.6)
project(Binlog)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(CTest)
include(AddressSanitizer)
include(Coverage)
include(MarkdownToHtml)

# Set default type to Release with debug symbols
if (NOT CMAKE_BUILD_TYPE AND NOT BINLOG_GEN_COVERAGE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Debug|Release|RelWithDebInfo|MinSizeRel" FORCE)
endif ()

# Setup compile options
if (MSVC)
  add_compile_options(/W4)
else ()
  add_compile_options(-Wall -Wextra -Werror -Wconversion -Wsign-conversion)

  # TODO for gcc, add -Wlogical-op -Wold-style-cast -Wsuggest-override
  # TODO for clang, add -Wno-missing-braces
endif ()

# Clang Tidy
# BOOST_TEST triggers C vararg warnings
# BOOST_AUTO_TEST_CASE triggers exception in global var ctor warning (cert-err58-cpp)
if (BINLOG_USE_CLANG_TIDY)
  set(CMAKE_CXX_CLANG_TIDY clang-tidy
    -checks=*,-fuchsia-*,-cppcoreguidelines-pro-type-vararg,-hicpp-vararg,-cert-err58-cpp
    -warnings-as-errors=*
    -header-filter=.*
  )
endif ()

# Boost
find_package(Boost 1.59.0 REQUIRED COMPONENTS unit_test_framework)

# UnitTest
add_executable(UnitTest test/unit/UnitTest.cpp)
  target_compile_definitions(UnitTest PRIVATE
    BOOST_TEST_DYN_LINK
    BOOST_TEST_NO_MAIN
  )
  target_include_directories(UnitTest SYSTEM PRIVATE
    ${Boost_INCLUDE_DIRS}
  )
  target_link_libraries(UnitTest Boost::unit_test_framework)

add_test(NAME UnitTest
         COMMAND UnitTest --log_level=test_suite --color_output=true)
target_compile_features(UnitTest PRIVATE cxx_std_14)

# Documentation
markdown_to_html_group(Documentation UserGuide)
