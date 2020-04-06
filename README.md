# Binlog

A high performance C++ log library to produce structured binary logs.

    BINLOG_INFO("Log anything! {}, {} or even {}", 1.2f, std::vector{3,4,5}, AdaptedStruct{1, "Foo"});

## Motivation

Consider the following log excerpt, taken from an application, that
computes a float value for each request using multiple threads:

    INFO [2019/12/10 12:54:47.062805012] thread1 Process request #1, result: 6.765831 (bin/app/processor.cpp:137)
    INFO [2019/12/10 12:54:47.062805881] thread1 Process request #2, result: 7.835833 (bin/app/processor.cpp:137)
    INFO [2019/12/10 12:54:47.062806406] thread2 Process request #3, result: 2.800832 (bin/app/processor.cpp:137)
    INFO [2019/12/10 12:54:47.062806903] thread3 Process request #5, result: 5.765831 (bin/app/processor.cpp:137)
    INFO [2019/12/10 12:54:47.062807397] thread2 Process request #4, result: 1.784832 (bin/app/processor.cpp:137)
    INFO [2019/12/10 12:54:47.062807877] thread2 Process request #7, result: 2.777844 (bin/app/processor.cpp:137)
    INFO [2019/12/10 12:54:47.062808437] thread3 Process request #6, result: 3.783869 (bin/app/processor.cpp:137)

There's a lot of **redundancy** (the severity, the format string, the file path are printed several times),
**wasted space** (the timestamp is represented using 29 bytes, where 8 bytes of information would be plenty),
and **loss of precision** (the originally computed floating point _result_ is lost, only its text representation
remains). Furthermore, the log was **expensive to produce** (consider the string conversion of each timestamp
and float value) and **not trivial to parse** using automated tools. Conventional log solutions also have to
make a trade-off: either implement synchronous logging (possibly via slow locking) that ensures the
log events are sorted by creation time, or asynchronous logging (without global locks) that usually
produces **unsorted output** and typically prone to data loss if the application crashes.

Binlog solves these issues by using _structured binary logs_.
The static parts (severity, format string, file and line, etc) are saved only once to the logfile.
While logging, instead of converting the log arguments to text, they are simply copied as-is,
and the static parts are referenced by a single identifier.
This way the logfile becomes **less redundant** (static parts are only written once),
smaller (e.g: timestamps are stored on fewer bytes without loss of precision),
and possibly **more precise** (e.g: the exact float is saved, without having the representation
interfering with the value).
Because of to the smaller representation, _binary logfiles_ are much **faster to produce**,
saving precious cycles on the hot path of the application.
Thanks to the _structured_ nature, binary logfiles can be consumed by other programs,
without reverting to fragile text processing methods.

_Binary logfiles_ are not human readable, but they can be converted to text using the `bread` program.
The format of the text log is configurable, independent of the logfile.
`bread` can also efficiently sort the logs by time, taking advantage of its structured nature.

_Asynchronous logging_ saves cycles on the hot path, but allows log events to remain in the
internal buffers if the application crashes. Binlog solves this by providing a recovery tool,
`brecovery`, that recovers stuck data from coredumps in a platform agnostic way.

For further information, please refer to the [Documentation][].

## Features

 - Log (almost) anything:
   - Fundamentals (char, bool, integers, floats)
   - Containers (vector, deque, list, map, set and more)
   - Strings (const char pointer, string, string_view)
   - Pointers (raw, unique_ptr, shared_ptr and optionals)
   - Pairs and Tuples
   - Enums
   - Custom structures (when adapted)
 - High performance: log in a matter of tens of nanoseconds
 - Asynchronous logging via lockfree queues
 - Format strings with `{}` placeholders
 - Vertical separation of logs via custom categories
 - Horizontal separation of logs via runtime configurable severities
 - Sort logs by time while reading
 - Recover buffered log events from coredumps

## Performance

The performance of creating log events is benchmarked using the [Google Benchmark][] library.
Benchmark code is located at `test/perf`.
The benchmark measures the time it takes to create a timestamped log event with various
log arguments (e.g: one integer, one string, three floats) in a single-producer,
single-consumer queue (the logging is asynchronous), running in a tight loop.
Different clocks are used to timestamp log events. `no clock` means constant 0,
`TSC clock` uses the time stamp counter, while `sys clock` means
`std::chrono::system_clock::now()` or `clock_gettime`.
There's a bechmark `poison cache`, trying to guess the cache effect, that invalidates
the data cache on each iteration by writing a large byte buffer.
The timing includes the writing of the buffer.

The results reported below are a sample of several runs,
only to show approximate performance, and not for direct comparison.
Machine configuration: Intel Xeon E5-2698 2.20 GHz, RHEL 2.6.32.

| Benchmark                   | Std. Dev. | Median     |
|:----------------------------|----------:|-----------:|
| One integer (no clock)      |      0 ns |       8 ns |
| One integer (TSC clock)     |      0 ns |       9 ns |
| One integer (sys clock)     |      0 ns |      34 ns |
| One integer (poison cache)  |      8 ns |     740 ns |
| One string                  |      0 ns |      38 ns |
| Three floats                |      0 ns |      30 ns |

## Install

See `INSTALL.md`.
In general, Binlog requires a C++14 conforming platform.
Binlog is built and tested on Linux, macOS and Windows,
with various recent versions of GCC, Clang and MSVC.

[Documentation]: http://binlog.org/UserGuide.html
[Google Benchmark]: https://github.com/google/benchmark
