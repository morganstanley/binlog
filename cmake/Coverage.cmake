include(CheckCXXCompilerFlag)
include(CMakePushCheckState)

option(BINLOG_GEN_COVERAGE "Generate code coverage report" OFF)

if (BINLOG_GEN_COVERAGE)

  check_cxx_compiler_flag("-fprofile-instr-generate -fcoverage-mapping" HAS_CLANG_COV)

  CMAKE_PUSH_CHECK_STATE(RESET)
    # Make check_cxx_compiler_flag pass required flags to linker as well:
    set(CMAKE_REQUIRED_FLAGS "--coverage")
    check_cxx_compiler_flag("--coverage" HAS_GCC_COV)
  CMAKE_POP_CHECK_STATE()

  if (HAS_CLANG_COV)
    add_compile_options("-fprofile-instr-generate" "-fcoverage-mapping")
    add_link_options("-fprofile-instr-generate" "-fcoverage-mapping")

    add_custom_target(coverage_init
      COMMENT "Initialize coverage counters"
      COMMAND "rm" "-f" "default.profraw" "default.profdata")

    add_custom_target(coverage
      COMMENT "Generate coverage report"
      COMMAND "llvm-profdata" "merge" "-sparse" "default.profraw"
                              "-o" "default.profdata"
      COMMAND "llvm-cov" "show" "-format" "html" "-instr-profile" "default.profdata"
                         "UnitTest"
                         ">" "coverage_report.html"
      COMMAND "echo" "Coverage report generated: coverage_report.html"
    )
  elseif (HAS_GCC_COV)
    add_compile_options("--coverage")
    add_link_options("--coverage")

    add_custom_target(coverage_init
      COMMENT "Initialize coverage counters"
      COMMAND "lcov" "--capture" "--initial" "--directory" "CMakeFiles"
                     "-o" "coverage_baseline.info")

    add_custom_target(coverage
      COMMENT "Generate coverage report"
      COMMAND "lcov" "--capture" "--directory" "CMakeFiles"
                     "-o" "coverage_test.info"
      COMMAND "lcov" "--add-tracefile" "coverage_baseline.info"
                     "--add-tracefile" "coverage_test.info"
                     "-o" "coverage_final.info"
      COMMAND "genhtml" "coverage_final.info" "--output-directory" "coverage_report"
      COMMAND "echo" "Coverage report generated into dir: coverage_report"
    )
  else ()
    message(FATAL_ERROR "Coverage report requrested by BINLOG_GEN_COVERAGE, but appears to be not supported on this platform")
  endif ()

  add_compile_options("-g")

  message(STATUS "Enable code coverage data generation")

endif ()
