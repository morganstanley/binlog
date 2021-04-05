//[fs
#include <binlog/adapt_stdfilesystem.hpp> // must be included to log std::filesystem types, requires C++17
//]

#include <binlog/binlog.hpp>

#include <filesystem>
#include <iostream>
#include <system_error>

int main()
{
  // path

  //[fs

  const std::filesystem::path p("/path/to/file");
  BINLOG_INFO("std::filesystem::path: {}", p);
  // Outputs: std::filesystem::path: /path/to/file
  //]

  // directory_entry
  std::error_code ec;
  const std::filesystem::directory_entry de(".", ec);
  BINLOG_INFO("std::filesystem::directory_entry: {}", de);
  // Outputs: std::filesystem::directory_entry: .

  // directory_iterator, recursive_directory_iterator are input iterators - not supported

  // file_type
  const std::filesystem::file_type ft = std::filesystem::file_type::regular;
  BINLOG_INFO("std::filesystem::file_type: {}", ft);
  // Outputs: std::filesystem::file_type: regular

  // perms
  const std::filesystem::perms ps = std::filesystem::perms::owner_exec;
  BINLOG_INFO("std::filesystem::perms: {}", ps);
  // Outputs: std::filesystem::perms: owner_exec

  // space_info
  const std::filesystem::space_info si{3,1,1};
  BINLOG_INFO("{}", si);
  // Outputs: std::filesystem::space_info{ capacity: 3, free: 1, available: 1 }

  // file_status
  const std::filesystem::file_status fs{
    std::filesystem::file_type::directory,
    std::filesystem::perms::owner_read
  };
  BINLOG_INFO("{}", fs);
  // Outputs: std::filesystem::file_status{ type: directory, permissions: owner_read }

  binlog::consume(std::cout);
}
