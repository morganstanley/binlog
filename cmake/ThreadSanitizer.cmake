include(CheckCXXCompilerFlag)
include(CMakePushCheckState)

option(BINLOG_USE_TSAN "Build with Thread Sanitizer (TSAN) enabled" OFF)

if (BINLOG_USE_TSAN)

  check_cxx_compiler_flag("-fsanitize=thread" HAS_TSAN)

  CMAKE_PUSH_CHECK_STATE(RESET)
    # Make check_cxx_compiler_flag pass required flags to linker as well:
    set(CMAKE_REQUIRED_FLAGS "-fsanitize=thread -static-libtsan")
    check_cxx_compiler_flag("-fsanitize=thread -static-libtsan" HAS_TSAN_NEEDS_LIB)
  CMAKE_POP_CHECK_STATE()

  if (HAS_TSAN)
    add_compile_options("-fsanitize=thread")
    add_link_options("-fsanitize=thread")
  elseif (HAS_TSAN_NEEDS_LIB)
    add_compile_options("-fsanitize=thread" "-static-libtsan")
    add_link_options("-fsanitize=thread" "-static-libtsan")
  else ()
    message(FATAL_ERROR "Thread Sanitizer requested by BINLOG_USE_TSAN, but appears to be not supported on this platform")
  endif ()

  set(MEMORYCHECK_TYPE ThreadSanitizer)

  message(STATUS "Use Thread Sanitizer")

endif ()
