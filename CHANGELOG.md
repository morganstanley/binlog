# Changelog

## Upcoming Release

### Added

 - Support logging of std::optional
 - `binlog/char_ptr_is_string.hpp`: treat `char*` as string if included

### Fixed

 - Use -Wshadow on supporting compilers, fix warnings

## 2020-02-29

First stable release.

### Added

 - Mserialize serialization library
 - Binlog log library (high performance, log anything, asynchronous logging)
 - bread: binlog to text
 - brecovery: coredump to binlog
 - Extensive tests and documentation
