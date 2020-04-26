# Changelog

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
