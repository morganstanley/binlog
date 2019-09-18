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

## Build the documentation

The documentation is generated from markdown files, with code snippets
automatically included from source files. To build the documentation,
`python` and the `markdown` python package are required.

    Release/$ make Documentation

## Further Build Options

 - `-DBINLOG_USE_CLANG_TIDY`: runs clang-tidy on built sources.
 - `-DBOOST_ROOT`: specifies the path to an alternate boost installation (tests depend on boost)
 - `-DBINLOG_SOURCE_BROWSER_URL`: if specified, code snippets in documentation will use this link prefix

## Clean

    $ rm -rf Release/ Debug/ Sanitized/ Coverage/

## Test

Make sure that the targets are up-to-date, `ctest` doesn't check that by default.

    Release/$ ctest --output-on-failure
