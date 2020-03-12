# Install

Binlog comes with complete CMake support.
Binlog consist of three main components:

 - A header only part for producing logs
 - A compiled executable to read the logs
 - A compiled library to read the logs programmatically

In addition, as an **optional** dependency, **tests** depend on Boost 1.64.0 or higher,
and performance tests on [Google Benchmark][].

## Build and Use

Building requires `cmake`, version 3.1 or higher. The build files are generated in a separate directory.

    $ git clone <URL> binlog
    $ mkdir binlog/Release
    $ cd binlog/Release
    binlog/Release/$ cmake ..
    binlog/Release/$ make VERBOSE=1
    binlog/Release/$ make install DESTDIR=/path/to/binlog/install

The installed package can be depended on using CMake:

    find_package(binlog 0.1.0 REQUIRED)

    add_executable(YourApp your_app.cpp)
        target_compile_features(YourApp PRIVATE cxx_std_14)
        target_link_libraries(YourApp binlog::headers)

Then configure your application with `CMAKE_PREFIX_PATH`:

    your/project$ CMAKE_PREFIX_PATH=/path/to/binlog/install cmake

## Build the documentation

The documentation is generated from markdown files, with code snippets
automatically included from source files. To build the documentation,
`python` and the `markdown` python package are required.

    Release/$ make Documentation

The most recent documentation is available at [binlog.org][]

[binlog.org]: http://binlog.org/

## Further Build Options

To be used when invoking `cmake`.

 - `-DBINLOG_USE_CLANG_TIDY`: runs clang-tidy on built sources. Requires cmake 3.6 or greater.
 - `-DBINLOG_SOURCE_BROWSER_URL`: if specified, links to code in documentation will use this prefix
 - `-DBINLOG_USE_ASAN`: use [address sanitizer][] (with gcc or clang) - also enables the Leak Sanitizer
 - `-DBINLOG_USE_TSAN`: use [thread sanitizer][] (with gcc or clang)
 - `-DBINLOG_USE_UBSAN`: use [undefined behavior sanitizer][] (with gcc or clang)
 - `-DBINLOG_GEN_COVERAGE`: generate coverage data
 - `-DBINLOG_FORCE_TESTS`: fail during configuration if tests will not be built
 - `-DBOOST_ROOT`: specifies the path to an alternate boost installation (tests depend on boost)

[address sanitizer]: https://github.com/google/sanitizers/wiki/AddressSanitizer
[thread sanitizer]: https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual
[undefined behavior sanitizer]: https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html

## Different Build Flavours

Useful mostly for Binlog developers or debugging.

### Debug Build

    $ mkdir Debug
    $ cd Debug
    Debug/$ cmake .. -DCMAKE_BUILD_TYPE=Debug
    Debug/$ make VERBOSE=1

### Address Sanitized Build

    $ mkdir AddressSanitized
    $ cd AddressSanitized
    Sanitized/$ cmake .. -DBINLOG_USE_ASAN=On
    Sanitized/$ make VERBOSE=1

### Thread Sanitized Build

    $ mkdir ThreadSanitized
    $ cd ThreadSanitized
    Sanitized/$ cmake .. -DBINLOG_USE_TSAN=On
    Sanitized/$ make VERBOSE=1

### Debug Build with Code Coverage

When building with `GCC`, requires: `gcov`, `lcov`, `genhtml`.
When building with `clang`, requires: `llvm-profdata`, `llvm-tools`.

    $ mkdir Coverage
    $ cd Coverage
    Coverage/$ cmake .. -DCMAKE_BUILD_TYPE=Debug -DBINLOG_GEN_COVERAGE=On
    Coverage/$ make
    Coverage/$ make coverage_init
    Coverage/$ make test
    Coverage/$ make coverage

### Clean

    $ rm -rf Release/ Debug/ AddressSanitized/ ThreadSanitized/ Coverage/

## Test

Make sure that the targets are up-to-date, `ctest` doesn't check that by default.
Tests require Boost 1.64.0 or higher.

    Release/$ ctest -VV

## Performance Test

Performance tests require the [Google Benchmark][] library.
If installed to a non-standard location, `cmake` can still use it:

    $ ls /path/to/benchmark/install
    benchmarkConfig.cmake  benchmarkConfigVersion.cmake  benchmarkTargets-relwithdebinfo.cmake  benchmarkTargets.cmake
    Release/$ CMAKE_PREFIX_PATH=/path/to/benchmark/install cmake ..

[Google Benchmark]: https://github.com/google/benchmark
