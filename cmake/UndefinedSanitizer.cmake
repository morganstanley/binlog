include(CheckCXXCompilerFlag)
include(CMakePushCheckState)

option(BINLOG_USE_UBSAN "Build with Undefined Behavior Sanitizer (UBSAN) enabled" OFF)

if (BINLOG_USE_UBSAN)

  check_cxx_compiler_flag("-fsanitize=undefined" HAS_UBSAN)

  CMAKE_PUSH_CHECK_STATE(RESET)
    # Make check_cxx_compiler_flag pass required flags to linker as well:
    set(CMAKE_REQUIRED_FLAGS "-fsanitize=undefined -static-libubsan")
    check_cxx_compiler_flag("-fsanitize=undefined -static-libubsan" HAS_UBSAN_NEEDS_LIB)
  CMAKE_POP_CHECK_STATE()

  if (HAS_UBSAN_NEEDS_LIB)
    add_compile_options("-fsanitize=undefined" "-static-libubsan")
    add_link_options("-fsanitize=undefined" "-static-libubsan")
  elseif (HAS_UBSAN)
    add_compile_options("-fsanitize=undefined")
    add_link_options("-fsanitize=undefined")
  else ()
    message(FATAL_ERROR "Undefined Behavior Sanitizer requested by BINLOG_USE_UBSAN, but appears to be not supported on this platform")
  endif ()

  message(STATUS "Use Undefined Behavior Sanitizer")

endif ()
