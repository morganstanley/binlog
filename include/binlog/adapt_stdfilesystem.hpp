#ifndef BINLOG_ADAPT_STDFILESYSTEM_HPP
#define BINLOG_ADAPT_STDFILESYSTEM_HPP

// Make std::filesystem components loggable by including this file

#include <binlog/adapt_enum.hpp>
#include <binlog/adapt_struct.hpp>

#include <mserialize/serialize.hpp>

#include <filesystem>
#include <type_traits> // is_same

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

// BAD: p.string() allocates memory. It is called twice per logging
// on platforms where path::value_type is not char, e.g: Windows.
template <>
struct CustomSerializer<std::filesystem::path>
{
  template <typename OutputStream>
  static void serialize(const std::filesystem::path& p, OutputStream& ostream)
  {
    if constexpr (std::is_same_v<char, std::filesystem::path::value_type>)
    {
      mserialize::serialize(p.native(), ostream);
    }
    else
    {
      mserialize::serialize(p.string(), ostream);
    }
  }

  static std::size_t serialized_size(const std::filesystem::path& p)
  {
    if constexpr (std::is_same_v<char, std::filesystem::path::value_type>)
    {
      return mserialize::serialized_size(p.native());
    }

    return mserialize::serialized_size(p.string());
  }
};

template <>
struct CustomTag<std::filesystem::path>
{
  static constexpr auto tag_string()
  {
    return make_cx_string("{std::filesystem::path`str'[c}");
  }
};

} // namespace mserialize

BINLOG_ADAPT_STRUCT(std::filesystem::directory_entry, path)

#endif // BINLOG_ADAPT_STDFILESYSTEM_HPP
