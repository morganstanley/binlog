cmake_minimum_required(VERSION 2.6)
project(Binlog)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(CTest)
include(AddressSanitizer)
include(ThreadSanitizer)
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
if (BINLOG_USE_CLANG_TIDY)
  set(CMAKE_CXX_CLANG_TIDY clang-tidy
    -warnings-as-errors=*
  )
endif ()

# Threads (e.g: -pthread)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Boost
find_package(Boost 1.64.0 REQUIRED COMPONENTS unit_test_framework system)

# Mserialize
add_library(mserialize INTERFACE)
target_include_directories(mserialize INTERFACE include)

# Binlog
add_library(binlog OBJECT
  include/binlog/EventStream.cpp
  include/binlog/Time.cpp
  include/binlog/ToStringVisitor.cpp
  include/binlog/PrettyPrinter.cpp
)
  target_compile_features(binlog PRIVATE cxx_std_14)
  target_include_directories(binlog PUBLIC include)

# bread
add_executable(bread
  bin/bread.cpp
)
  target_compile_features(bread PRIVATE cxx_std_14)
  target_link_libraries(bread binlog)

# UnitTest
add_executable(UnitTest test/unit/UnitTest.cpp
  test/unit/mserialize/roundtrip.cpp
  test/unit/mserialize/cx_string.cpp
  test/unit/mserialize/tag.cpp
  test/unit/mserialize/visit.cpp
  test/unit/mserialize/documentation.cpp

  $<TARGET_OBJECTS:binlog>
  test/unit/binlog/TestEventStream.cpp
  test/unit/binlog/TestTime.cpp
  test/unit/binlog/TestToStringVisitor.cpp
  test/unit/binlog/TestPrettyPrinter.cpp
  test/unit/binlog/TestQueue.cpp
  test/unit/binlog/TestSession.cpp
  test/unit/binlog/TestSessionWriter.cpp
  test/unit/binlog/TestCreateSourceAndEvent.cpp
  test/unit/binlog/TestCreateSourceAndEventIf.cpp
  test/unit/binlog/TestAdvancedLogMacros.cpp
  test/unit/binlog/TestBasicLogMacros.cpp
  test/unit/binlog/TestArrayView.cpp
  test/unit/binlog/TestConstCharPtrIsString.cpp

  test/unit/binlog/test_utils.cpp
)
  target_compile_features(UnitTest PRIVATE cxx_std_14)
  target_compile_definitions(UnitTest PRIVATE
    BOOST_TEST_DYN_LINK
    BOOST_TEST_NO_MAIN
  )
  target_include_directories(UnitTest SYSTEM PRIVATE
    ${Boost_INCLUDE_DIRS}
  )
  target_link_libraries(UnitTest Boost::unit_test_framework)
  target_link_libraries(UnitTest mserialize)
  target_link_libraries(UnitTest Threads::Threads) # used by: TestQueue, TestSessionWriter, TestCreateSourceAndEvent

add_test(NAME UnitTest
         COMMAND UnitTest --log_level=test_suite --color_output=true)

# IntegrationTest

add_executable(IntegrationTest test/integration/IntegrationTest.cpp)
  target_compile_features(IntegrationTest PRIVATE cxx_std_14)
  target_compile_definitions(IntegrationTest PRIVATE
    BOOST_TEST_DYN_LINK
    BOOST_TEST_NO_MAIN
  )
  target_include_directories(IntegrationTest SYSTEM PRIVATE
    ${Boost_INCLUDE_DIRS}
  )
  target_link_libraries(IntegrationTest Boost::unit_test_framework)
  target_link_libraries(IntegrationTest Boost::system)
  target_link_libraries(IntegrationTest mserialize)

add_test(NAME IntegrationTest
  COMMAND IntegrationTest --log_level=test_suite --color_output=true -- $<TARGET_FILE:bread> ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR})

function(add_inttest name)
  add_executable(${name} test/integration/${name}.cpp)
    target_link_libraries(${name} binlog)
endfunction()

add_inttest(Logging)
add_inttest(LoggingFundamentals)
add_inttest(LoggingContainers)
  target_include_directories(LoggingContainers SYSTEM PRIVATE ${Boost_INCLUDE_DIRS})
add_inttest(LoggingStrings)
  target_include_directories(LoggingStrings SYSTEM PRIVATE ${Boost_INCLUDE_DIRS})
add_inttest(LoggingPointers)
  target_include_directories(LoggingPointers SYSTEM PRIVATE ${Boost_INCLUDE_DIRS})

# Tools
add_executable(GenerateForeachMacro tools/generate_foreach_macro.cpp)

# Documentation
markdown_to_html_group(Documentation UserGuide Mserialize)
