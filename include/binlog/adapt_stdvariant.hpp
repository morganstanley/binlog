#ifndef BINLOG_ADAPT_STDVARIANT_HPP
#define BINLOG_ADAPT_STDVARIANT_HPP

// Make std::variant loggable by including this file

#include <mserialize/cx_string.hpp>
#include <mserialize/serialize.hpp>
#include <mserialize/tag.hpp>

#include <cstdint>
#include <variant>

namespace mserialize {

template <typename... T>
struct CustomSerializer<std::variant<T...>>
{
  static_assert(sizeof...(T) <= 255, "Number of variant alternatives <= 255");

  template <typename OutputStream>
  static void serialize(const std::variant<T...>& v, OutputStream& ostream)
  {
    // clang-tidy complains about exceptions escaping
    // if valueless_by_exception() is checked instead of try-catch
    try
    {
      std::visit([&ostream, &v](auto&& t)
      {
        mserialize::serialize(std::uint8_t(v.index()), ostream);
        mserialize::serialize(t, ostream);
      }, v);
    }
    catch (const std::bad_variant_access&)
    {
      mserialize::serialize(std::uint8_t(sizeof...(T)), ostream);
    }
  }

  static std::size_t serialized_size(const std::variant<T...>& v)
  {
    // clang-tidy complains about exceptions escaping
    // if valueless_by_exception() is checked instead of try-catch
    try
    {
      return sizeof(std::uint8_t) +
             std::visit([](auto&& t) { return mserialize::serialized_size(t); }, v);
    }
    catch (const std::bad_variant_access&)
    {
      return sizeof(std::uint8_t);
    }
  }
};

template <typename... T>
struct CustomTag<std::variant<T...>>
{
  // Make this type not-constructible, if some Tag<T> is not constructible
  std::conditional_t<
    detail::conjunction<detail::has_tag<T>...>::value,
    std::true_type,
    mserialize::detail::BuiltinTag<void>
  > tag_guard;

  static constexpr auto tag_string()
  {
    return cx_strcat(
      make_cx_string("<"),
      mserialize::tag<T>()...,
      make_cx_string("0>") // 0 for valueless_by_exception
    );
  }
};

template <>
struct CustomSerializer<std::monostate>
{
  template <typename OutputStream>
  static void serialize(const std::monostate, OutputStream&) {}

  static std::size_t serialized_size(const std::monostate)
  {
    return 0;
  }
};

template <>
struct CustomTag<std::monostate>
{
  static constexpr auto tag_string()
  {
    return make_cx_string("0");
  }
};

} // namespace mserialize

#endif // BINLOG_ADAPT_STDVARIANT_HPP
