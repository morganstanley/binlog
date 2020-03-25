#ifndef BINLOG_ADAPT_STDFILESYSTEM_HPP
#define BINLOG_ADAPT_STDFILESYSTEM_HPP

// Make std::filesystem components loggable by including this file

#include <binlog/adapt_enum.hpp>
#include <binlog/adapt_struct.hpp>

#include <mserialize/serialize.hpp>
#include <mserialize/tag.hpp>

#include <filesystem>
#include <string>

// TODO Move SingleMemberSerializer

BINLOG_ADAPT_ENUM(std::filesystem::file_type,
  none, not_found, regular, directory, symlink, block, character, fifo, socket, unknown
)

BINLOG_ADAPT_ENUM(std::filesystem::perms,
  none, owner_read, owner_write, owner_exec, owner_all, group_read,
  group_write, group_exec, group_all, others_read, others_write, others_exec,
  others_all, all, set_uid, set_gid, sticky_bit, mask
)

BINLOG_ADAPT_STRUCT(std::filesystem::space_info, capacity, free, available)

BINLOG_ADAPT_STRUCT(std::filesystem::file_status, type, permissions)

namespace mserialize {

template <typename T, typename Member, Member member>
struct SingleMemberSerializer
{
  template <typename OutputStream>
  static void serialize(const T& t, OutputStream& ostream)
  {
    mserialize::serialize((t.*member)(), ostream);
  }

  static std::size_t serialized_size(const T& t)
  {
    return mserialize::serialized_size((t.*member)());
  }
};

//
// path
//

template <>
struct CustomSerializer<std::filesystem::path>
  :SingleMemberSerializer<
    std::filesystem::path,
    decltype(&std::filesystem::path::native),
    &std::filesystem::path::native
  >
{};

template <>
struct CustomTag<std::filesystem::path> : detail::Tag<std::string>::type {};

//
// directory_entry
//

template <>
struct CustomSerializer<std::filesystem::directory_entry>
  :SingleMemberSerializer<
    std::filesystem::directory_entry,
    decltype(&std::filesystem::directory_entry::path),
    &std::filesystem::directory_entry::path
  >
{};

template <>
struct CustomTag<std::filesystem::directory_entry> : detail::Tag<std::string>::type {};

} // namespace mserialize

#endif // BINLOG_ADAPT_STDFILESYSTEM_HPP
