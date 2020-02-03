cmake_minimum_required(VERSION 3.1)
project(binlog VERSION 0.1.0 LANGUAGES CXX)

#---------------------------
# CMake modules
#---------------------------

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

include(AddressSanitizer)
include(CMakePackageConfigHelpers)
include(CTest)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(Coverage)
include(MarkdownToHtml)
include(OptionalCompileOption)
include(ThreadSanitizer)

#---------------------------
# Depencencies
#---------------------------

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(BINLOG_FORCE_TESTS)
  find_package(Boost 1.64.0 REQUIRED COMPONENTS unit_test_framework filesystem system)
else()
  find_package(Boost 1.64.0 COMPONENTS unit_test_framework filesystem system)
endif()

find_package(benchmark COMPONENTS benchmark)

#---------------------------
# CMake workarounds
#---------------------------

function(include_boost target)
  if(CMAKE_VERSION VERSION_GREATER 3.5)
    target_link_libraries(${target} Boost::boost) # headers
  else()
    target_include_directories(${target} SYSTEM PRIVATE ${Boost_INCLUDE_DIRS})
  endif()
endfunction()

function(link_boost target library)
  if(CMAKE_VERSION VERSION_GREATER 3.5)
    target_link_libraries(${target} "Boost::${library}")
  else()
    string(TOUPPER ${library} LIBRARY)
    target_link_libraries(${target} "${Boost_${LIBRARY}_LIBRARY}")
  endif()
endfunction()

#---------------------------
# Build type
#---------------------------

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Defaulting to build type: 'RelWithDebInfo'")
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Debug|Release|RelWithDebInfo|MinSizeRel" FORCE)
endif()

#---------------------------
# C++ standard
#---------------------------

set(CMAKE_CXX_STANDARD 14)            # -std=c++14
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)         # no gnu++14

#---------------------------
# Compiler options
#---------------------------

if (MSVC)
  add_compile_options(/W4)
else ()
  add_compile_options(-Wall -Wextra -Werror -pedantic)
  add_optional_compile_options(-Wconversion -Wsign-conversion -Wold-style-cast -Wsuggest-override)
endif ()

#---------------------------
# Platform specific configuration
#---------------------------

# clock_gettime
check_function_exists("clock_gettime" HAS_CLOCK_GETTIME)
if(NOT HAS_CLOCK_GETTIME)
  # Before glibc 2.17, clock_gettime is in librt
  check_library_exists("rt" "clock_gettime" "" CLOCK_GETTIME_IN_LIBRT)
  set(HAS_CLOCK_GETTIME 1)
endif()

#---------------------------
# clang-tidy
#---------------------------

if(CMAKE_VERSION VERSION_GREATER 3.6)
  option(BINLOG_USE_CLANG_TIDY "Run clang-tidy on sources" OFF)
  find_program(CLANG_TIDY_EXE NAMES "clang-tidy")

  if(BINLOG_USE_CLANG_TIDY AND CLANG_TIDY_EXE)
    message(STATUS "Use clang-tidy: ${CLANG_TIDY_EXE}")
    set(CMAKE_CXX_CLANG_TIDY  "${CLANG_TIDY_EXE}" -warnings-as-errors=*)
  endif()
endif()

#---------------------------
# binlog libraries
#---------------------------

add_library(headers INTERFACE)
  target_include_directories(headers INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  )
  if(CLOCK_GETTIME_IN_LIBRT)
    target_link_libraries(headers INTERFACE -lrt)
  endif()

add_library(binlog STATIC
  include/binlog/EventStream.cpp
  include/binlog/Time.cpp
  include/binlog/ToStringVisitor.cpp
  include/binlog/PrettyPrinter.cpp
  include/binlog/EntryStream.cpp
  include/binlog/TextOutputStream.cpp
)
  target_link_libraries(binlog PUBLIC headers)

#---------------------------
# bread
#---------------------------

add_executable(bread
  bin/bread.cpp
  bin/printers.cpp
  $<$<CXX_COMPILER_ID:MSVC>:bin/getopt.cpp bin/binaryio.cpp>
)
  target_link_libraries(bread PRIVATE binlog)

#---------------------------
# Documentation
#---------------------------

markdown_to_html_group(Documentation UserGuide Mserialize)

#---------------------------
# Examples
#---------------------------

function(add_example name)
  add_executable(${name} example/${name}.cpp)
  target_link_libraries(${name} headers)
endfunction()

add_example(HelloWorld)
add_example(DetailedHelloWorld)
add_example(ConsumeLoop)
add_example(LogRotation)
add_example(TextOutput)
  target_link_libraries(TextOutput binlog)
add_example(MultiOutput)
  target_link_libraries(MultiOutput binlog)

#---------------------------
# Unit Test
#---------------------------

if(Boost_FOUND)

  message(STATUS "Build unit tests")

  add_executable(UnitTest
    test/unit/UnitTest.cpp

    test/unit/mserialize/roundtrip.cpp
    test/unit/mserialize/cx_string.cpp
    test/unit/mserialize/tag.cpp
    test/unit/mserialize/visit.cpp
    test/unit/mserialize/documentation.cpp

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
    test/unit/binlog/TestEntryStream.cpp
    test/unit/binlog/TestTextOutputStream.cpp
    test/unit/binlog/TestEventFilter.cpp

    bin/printers.cpp
    test/unit/binlog/TestPrinters.cpp

    test/unit/binlog/test_utils.cpp
  )
    target_compile_definitions(UnitTest PRIVATE
      BOOST_TEST_DYN_LINK
      BOOST_TEST_NO_MAIN
    )
    include_boost(UnitTest)
    link_boost(UnitTest unit_test_framework)
    target_link_libraries(UnitTest binlog)
    target_link_libraries(UnitTest Threads::Threads) # used by: TestQueue, TestSessionWriter, TestCreateSourceAndEvent
    target_include_directories(UnitTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/bin)

  add_test(NAME UnitTest COMMAND UnitTest --log_level=test_suite --color_output=true)

else()
  message(STATUS "Boost not found, will not build unit tests")
endif()

#---------------------------
# Integration Test
#---------------------------

if(Boost_FOUND)

  message(STATUS "Build integration tests")

  add_executable(IntegrationTest test/integration/IntegrationTest.cpp)
    target_compile_definitions(IntegrationTest PRIVATE
      BOOST_TEST_DYN_LINK
      BOOST_TEST_NO_MAIN
      BOOST_ALL_NO_LIB
    )
    include_boost(IntegrationTest)
    link_boost(IntegrationTest unit_test_framework)
    link_boost(IntegrationTest filesystem)
    link_boost(IntegrationTest system)
    target_link_libraries(IntegrationTest headers)
    target_link_libraries(IntegrationTest Threads::Threads) # Boost.Process -> ASIO -> pthread_join as of 1.70 f2cf918b

  add_test(NAME IntegrationTest
    COMMAND IntegrationTest --log_level=test_suite --color_output=true -- "$<TARGET_FILE:bread>" "$<TARGET_FILE_DIR:IntegrationTest>" "${PROJECT_SOURCE_DIR}")

  function(add_inttest name)
    add_executable(${name} test/integration/${name}.cpp $<$<CXX_COMPILER_ID:MSVC>:bin/binaryio.cpp>)
    target_link_libraries(${name} headers)
  endfunction()

  add_inttest(Logging)
  add_inttest(LoggingFundamentals)
  add_inttest(LoggingContainers)
    include_boost(LoggingContainers)
  add_inttest(LoggingStrings)
    include_boost(LoggingStrings)
  add_inttest(LoggingPointers)
    include_boost(LoggingPointers)
  add_inttest(LoggingTuples)
  add_inttest(LoggingEnums)
  add_inttest(LoggingAdaptedStructs)
  add_inttest(NamedWriters)
  add_inttest(SeverityControl)
  add_inttest(Categories)

else()
  message(STATUS "Boost not found, will not build integration tests")
endif()

#---------------------------
# Performance Test
#---------------------------

if (benchmark_FOUND)

  message(STATUS "Build performance tests")

  function(add_benchmark name)
    add_executable(${name} test/perf/${name}.cpp)
    target_link_libraries(${name} headers)
    target_link_libraries(${name} benchmark::benchmark)
  endfunction()

  add_benchmark(PerftestQueue)
  add_benchmark(PerftestSessionWriter)

else ()
  message(STATUS "Google Benchmark library not found, will not build performance tests")
endif ()

#---------------------------
# Tools
#---------------------------

add_executable(LargeLogfile test/perf/LargeLogfile.cpp)
  target_link_libraries(LargeLogfile headers)

add_executable(GenerateForeachMacro tools/generate_foreach_macro.cpp)

#---------------------------
# Install
#---------------------------

write_basic_package_version_file(
  binlogConfigVersion.cmake
  VERSION ${PACKAGE_VERSION}
  COMPATIBILITY ExactVersion
)

configure_package_config_file(
  "${PROJECT_SOURCE_DIR}/cmake/binlogConfig.cmake.in"
  "${PROJECT_BINARY_DIR}/binlogConfig.cmake"
  INSTALL_DESTINATION lib/cmake/binlog
)

install(
  FILES "${PROJECT_BINARY_DIR}/binlogConfigVersion.cmake"
        "${PROJECT_BINARY_DIR}/binlogConfig.cmake"
  DESTINATION lib/cmake/binlog
)

install(
  TARGETS bread headers binlog
  EXPORT binlogTargets
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
)

install(
  EXPORT binlogTargets
  FILE binlogTargets.cmake
  NAMESPACE binlog::
  DESTINATION lib/cmake/binlog
)

install(
  DIRECTORY ${PROJECT_SOURCE_DIR}/include/
  DESTINATION include
  FILES_MATCHING PATTERN "*.hpp"
)
