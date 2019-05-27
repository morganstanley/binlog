# Binlog

## Release Build

Building requires `cmake`. The build files are generated in a separate directory.

    $ mkdir Release
    $ cd Release
    Release/$ cmake ..
    Release/$ make VERBOSE=1

## Debug Build

    $ mkdir Debug
    $ cd Debug
    Debug/$ cmake .. -DCMAKE_BUILD_TYPE=Debug
    Debug/$ make VERBOSE=1

## Sanitized Build

    $ mkdir Sanitized
    $ cd Sanitized
    Sanitized/$ cmake .. -DBINLOG_USE_ASAN=On
    Sanitized/$ make VERBOSE=1

## Debug Build with Code Coverage

When building with `GCC`, requires: `gcov`, `lcov`, `genhtml`.
When building with `Clang`, requires: `llvm-profdata`, `llvm-tools`.

    $ mkdir Coverage
    $ cd Coverage
    Coverage/$ cmake .. -DCMAKE_BUILD_TYPE=Debug -DBINLOG_GEN_COVERAGE=On
    Coverage/$ make
    Coverage/$ make coverage_init
    Coverage/$ make test
    Coverage/$ make coverage

## Further Build Options

 - `BINLOG_USE_CLANG_TIDY`: runs clang-tidy on built sources.

## Clean

    $ rm -rf Release/ Debug/ Sanitized/ Coverage/

## Test

Make sure that the targets are up-to-date, `ctest` doesn't check that by default.

    Release/$ ctest --output-on-failure
