INCLUDE(CheckCXXCompilerFlag)
INCLUDE(CMakePushCheckState)

OPTION(BINLOG_USE_ASAN "Build with Address Sanitizer (ASAN) enabled" OFF)

IF (BINLOG_USE_ASAN)

  check_cxx_compiler_flag("-fsanitize=address" HAS_ASAN)

  CMAKE_PUSH_CHECK_STATE(RESET)
    # Make check_cxx_compiler_flag pass required flags to linker as well:
    SET(CMAKE_REQUIRED_FLAGS "-fsanitize=address -static-libasan")
    check_cxx_compiler_flag("-fsanitize=address -static-libasan" HAS_ASAN_NEEDS_LIB)
  CMAKE_POP_CHECK_STATE()

  IF (HAS_ASAN)
    add_compile_options("-fsanitize=address")
    add_link_options("-fsanitize=address")
  ELSEIF (HAS_ASAN_NEEDS_LIB)
    add_compile_options("-fsanitize=address" "-static-libasan")
    add_link_options("-fsanitize=address" "-static-libasan")
  ELSE ()
    MESSAGE(FATAL_ERROR "Address Sanitizer requested by BINLOG_USE_ASAN, but appears to be not supported on this platform")
  ENDIF ()

  SET(MEMORYCHECK_TYPE AddressSanitizer)

  MESSAGE(STATUS "Use Address Sanitizer")

ENDIF ()
