cmake_minimum_required(VERSION 3.1)
project(install_test VERSION 0.1.0 LANGUAGES CXX)

find_package(binlog 0.1.0 REQUIRED)

set(CMAKE_CXX_STANDARD 14)            # -std=c++14
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)         # no gnu++14

add_executable(hello_headers ../headers.cpp)
  target_link_libraries(hello_headers binlog::headers)

add_executable(hello_lib ../lib.cpp)
  target_link_libraries(hello_lib binlog::binlog)
