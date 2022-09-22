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
include(UndefinedSanitizer)

#---------------------------
# Depencencies
#---------------------------

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(Boost 1.64.0)
find_package(benchmark COMPONENTS benchmark)

#---------------------------
# CMake workarounds
#---------------------------

set(BINLOG_IS_TOPLEVEL_PROJECT OFF)
if(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
  set(BINLOG_IS_TOPLEVEL_PROJECT ON)
endif()

function(optional_include_boost target)
  if(Boost_FOUND)
    if(CMAKE_VERSION VERSION_GREATER 3.5)
      target_link_libraries(${target} Boost::boost) # headers
    else()
      target_include_directories(${target} SYSTEM PRIVATE ${Boost_INCLUDE_DIRS})
    endif()
    target_compile_definitions(${target} PRIVATE BINLOG_HAS_BOOST)
  endif()
endfunction()

# do not use -rdynamic, make binaries smaller
if(POLICY CMP0065)
  cmake_policy(SET CMP0065 NEW)
endif()

#---------------------------
# Build type
#---------------------------

if(BINLOG_IS_TOPLEVEL_PROJECT)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Defaulting to build type: 'RelWithDebInfo'")
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Debug|Release|RelWithDebInfo|MinSizeRel" FORCE)
endif()

endif()

#---------------------------
# C++ standard
#---------------------------

if(BINLOG_IS_TOPLEVEL_PROJECT)

set(CMAKE_CXX_STANDARD 14 CACHE STRING "C++ standard (11/14/17/20/...)") # -std=c++14
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)         # no gnu++14

endif()

#---------------------------
# Compiler options
#---------------------------

if(BINLOG_IS_TOPLEVEL_PROJECT)

if (MSVC)
  add_compile_options(/W4)
  if (MSVC_VERSION GREATER_EQUAL 1914)
    add_compile_options(/Zc:__cplusplus)
  endif()
else ()
  add_compile_options(-Wall -Wextra -Werror -pedantic)
  add_optional_compile_options(-Wconversion -Wsign-conversion -Wold-style-cast -Wsuggest-override -Wshadow)
endif ()

endif()

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

# Interprocedural Optimization (LTO)
if(POLICY CMP0069)
  cmake_policy(SET CMP0069 NEW)
  include(CheckIPOSupported) # fails if policy 69 is not set
  check_ipo_supported(RESULT BINLOG_HAS_IPO)
  if(BINLOG_HAS_IPO)
    message(STATUS "Use interprocedural optimization")
  endif()
endif()

#---------------------------
# clang-tidy
#---------------------------

option(BINLOG_USE_CLANG_TIDY "Run clang-tidy on sources" OFF)

if(BINLOG_USE_CLANG_TIDY)
  if(CMAKE_VERSION VERSION_GREATER 3.6)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy")

    if (CLANG_TIDY_EXE)
      message(STATUS "Use clang-tidy: ${CLANG_TIDY_EXE}")
      set(CMAKE_CXX_CLANG_TIDY  "${CLANG_TIDY_EXE}" -warnings-as-errors=*)

      message(STATUS "Disable interprocedural optimization because of clang-tidy")
      set(BINLOG_HAS_IPO OFF)
    else()
      message(SEND_ERROR "clang-tidy executable not found")
    endif()
  else()
    message(SEND_ERROR "clang-tidy was requested, but cmake is too old, 3.6 or greater is required")
  endif()
endif()

#---------------------------
# binlog libraries
#---------------------------

add_library(headers INTERFACE)
  if(BINLOG_IS_TOPLEVEL_PROJECT)
    target_include_directories(headers INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
    )
  else()
    target_include_directories(headers SYSTEM INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
    )
  endif()
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
  include/binlog/detail/OstreamBuffer.cpp
)
  target_link_libraries(binlog PUBLIC headers)
  set_property(TARGET binlog PROPERTY INTERPROCEDURAL_OPTIMIZATION ${BINLOG_HAS_IPO})

# make add_subdirectory usage consistent with find_package
if(NOT BINLOG_IS_TOPLEVEL_PROJECT)
  add_library(binlog::headers ALIAS headers)
  add_library(binlog::binlog  ALIAS binlog)
endif()

#---------------------------
# bread
#---------------------------

add_executable(bread
  bin/bread.cpp
  bin/printers.cpp
  $<$<CXX_COMPILER_ID:MSVC>:bin/getopt.cpp bin/binaryio.cpp>
)
  target_link_libraries(bread PRIVATE binlog)
  set_property(TARGET bread  PROPERTY INTERPROCEDURAL_OPTIMIZATION ${BINLOG_HAS_IPO})

#---------------------------
# brecovery
#---------------------------

add_executable(brecovery
  bin/brecovery.cpp
  $<$<CXX_COMPILER_ID:MSVC>:bin/binaryio.cpp>
)
  target_link_libraries(brecovery PRIVATE binlog)

#---------------------------
# Documentation
#---------------------------

markdown_to_html_group(Documentation UserGuide Internals Mserialize)

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
add_example(TscClock)

#---------------------------
# Unit Test
#---------------------------

  message(STATUS "Build unit tests")

  add_executable(UnitTest
    test/unit/UnitTest.cpp

    test/unit/mserialize/roundtrip.cpp
    test/unit/mserialize/cx_string.cpp
    test/unit/mserialize/tag.cpp
    test/unit/mserialize/visit.cpp
    test/unit/mserialize/documentation.cpp
    test/unit/mserialize/inttohex.cpp
    test/unit/mserialize/singular.cpp
    test/unit/mserialize/tag_util.cpp

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
    test/unit/binlog/detail/TestOstreamBuffer.cpp
    test/unit/binlog/detail/TestSegmentedMap.cpp

    bin/printers.cpp
    test/unit/binlog/TestPrinters.cpp

    test/unit/binlog/test_utils.cpp
  )
    target_compile_definitions(UnitTest PRIVATE
      DOCTEST_CONFIG_SUPER_FAST_ASSERTS
      DOCTEST_CONFIG_NO_MULTI_LANE_ATOMICS
    )
    optional_include_boost(UnitTest) # used by: roundtrip.cpp
    target_link_libraries(UnitTest binlog)
    target_link_libraries(UnitTest Threads::Threads) # used by: TestQueue, TestSessionWriter, TestCreateSourceAndEvent
    target_include_directories(UnitTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/bin)
    target_include_directories(UnitTest SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/test) # for doctest/doctest.h

  add_test(NAME UnitTest COMMAND UnitTest -s --force-colors)
  set_property(TEST UnitTest PROPERTY ENVIRONMENT ASAN_OPTIONS=detect_leaks=1)

#---------------------------
# Integration Test
#---------------------------

  message(STATUS "Build integration tests")

  add_executable(IntegrationTest test/integration/IntegrationTest.cpp)
    target_compile_definitions(IntegrationTest PRIVATE
      DOCTEST_CONFIG_SUPER_FAST_ASSERTS
      DOCTEST_CONFIG_NO_MULTI_LANE_ATOMICS
    )
    target_link_libraries(IntegrationTest headers)
    target_include_directories(IntegrationTest SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/test) # for doctest/doctest.h
    set_target_properties(IntegrationTest PROPERTIES CXX_CLANG_TIDY "")

  add_test(NAME IntegrationTest
    COMMAND IntegrationTest -s --force-colors -- "$<TARGET_FILE:bread>" "$<TARGET_FILE_DIR:IntegrationTest>" "${PROJECT_SOURCE_DIR}")
  set_property(TEST IntegrationTest PROPERTY ENVIRONMENT ASAN_OPTIONS=detect_leaks=1:allocator_may_return_null=1)

  function(add_inttest name)
    add_executable(${name} test/integration/${name}.cpp $<$<CXX_COMPILER_ID:MSVC>:bin/binaryio.cpp>)
    target_link_libraries(${name} headers)
  endfunction()

  add_inttest(Logging)
  add_inttest(LoggingFundamentals)
  add_inttest(LoggingContainers)
  add_inttest(LoggingStrings)
  add_inttest(LoggingCStrings)
  add_inttest(LoggingPointers)
  add_inttest(LoggingTuples)
  add_inttest(LoggingTimePoint)
  add_inttest(LoggingDuration)
  add_inttest(LoggingEnums)
  add_inttest(LoggingAdaptedStructs)
  add_inttest(LoggingErrorCode)
  add_inttest(NamedWriters)
  add_inttest(SeverityControl)
  add_inttest(Categories)
  add_inttest(Shell)

  if(BINLOG_USE_ASAN OR BINLOG_USE_TSAN)
    # Crash recovery test creates a coredump of Shell.
    # Make sure it is not compiled with any sanitizer,
    # to avoid dumping a multi TB shadow memory.
    target_compile_options(Shell PRIVATE "-fno-sanitize=all")
    target_link_options(Shell PRIVATE "-fno-sanitize=all")
  endif()

  if(${CMAKE_CXX_STANDARD} GREATER_EQUAL 17)
    add_inttest(LoggingOptionals)
    add_inttest(LoggingFilesystem)
    add_inttest(LoggingVariants)
  endif()

  if(${CMAKE_CXX_STANDARD} GREATER_EQUAL 20)
    add_inttest(LoggingAdaptedConcepts)
  endif()

  if(Boost_FOUND)
    target_compile_definitions(IntegrationTest PRIVATE BINLOG_HAS_BOOST)

    add_inttest(LoggingBoostTypes)
      optional_include_boost(LoggingBoostTypes)
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
  TARGETS bread brecovery headers binlog
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
