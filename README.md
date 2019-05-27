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

## Clean

    $ rm -rf Release/ Debug/ Sanitized/

## Test

Make sure that the targets are up-to-date, `ctest` doesn't check that by default.

    Release/$ ctest --output-on-failure
