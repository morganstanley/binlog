include(CheckCXXCompilerFlag)
include(CMakePushCheckState)

option(BINLOG_USE_ASAN "Build with Address Sanitizer (ASAN) enabled" OFF)

if (BINLOG_USE_ASAN)

  check_cxx_compiler_flag("-fsanitize=address" HAS_ASAN)

  CMAKE_PUSH_CHECK_STATE(RESET)
    # Make check_cxx_compiler_flag pass required flags to linker as well:
    set(CMAKE_REQUIRED_FLAGS "-fsanitize=address -static-libasan")
    check_cxx_compiler_flag("-fsanitize=address -static-libasan" HAS_ASAN_NEEDS_LIB)
  CMAKE_POP_CHECK_STATE()

  if (HAS_ASAN)
    add_compile_options("-fsanitize=address")
    add_link_options("-fsanitize=address")
  elseif (HAS_ASAN_NEEDS_LIB)
    add_compile_options("-fsanitize=address" "-static-libasan")
    add_link_options("-fsanitize=address" "-static-libasan")
  else ()
    message(FATAL_ERROR "Address Sanitizer requested by BINLOG_USE_ASAN, but appears to be not supported on this platform")
  endif ()

  set(MEMORYCHECK_TYPE AddressSanitizer)

  message(STATUS "Use Address Sanitizer")

endif ()
