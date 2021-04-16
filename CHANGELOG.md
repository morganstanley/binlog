# Changelog

## 2021-04-16

Major release with new fatures.
In CI, C++17 is preferred now (but C++14 is still being tested on Linux),
and the online documentation is updated automatically on merge.
Boost is no longer required to build the tests, doctest is used instead.

### Changed

 - Change default bread date format to %Y-%m-%d %H:%M:%S.%N
 - Make this_thread::get_id() the name of default_thread_local_writer

### Added

 - Make MSERIALIZE_MAKE_STRUCT_TAG support C-style array members
 - Allow logging addresses, display them in hex
 - Make time_point loggable with pretty printing
 - Support logging of std::filesystem components
 - Support logging of std::variant
 - Support logging of std::error_code

### Fixed

 - Allow noexcept getters in C++17
 - Repeated singular objects (e.g: empty tuple) do not spam the output
 - Avoid not-helpful -Wtype-limits on GCC 10.2
 - Fix bread bugs uncovered by fuzzy testing

## 2020-04-26

Bugfix release with minor new features.

### Added

 - Support logging of std::optional
 - `binlog/char_ptr_is_string.hpp`: treat `char*` as string if included
 - Example of using TSC as log clock

### Fixed

 - Fix overflow in ticksToNanoseconds
 - Use -Wshadow on supporting compilers, fix warnings
 - Do not call memcpy with null, even if size is 0
 - Fix interaction with non-conforming libc++ bool vector

## 2020-02-29

First stable release.

### Added

 - Mserialize serialization library
 - Binlog log library (high performance, log anything, asynchronous logging)
 - bread: binlog to text
 - brecovery: coredump to binlog
 - Extensive tests and documentation
