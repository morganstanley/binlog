#ifndef MSERIALIZE_DETAIL_TAG_HPP
#define MSERIALIZE_DETAIL_TAG_HPP

#include <mserialize/cx_string.hpp>
#include <mserialize/detail/sequence_traits.hpp>
#include <mserialize/detail/type_traits.hpp>

#include <type_traits>

namespace mserialize {

// Custom tag - can be specialized for any type, takes precedence over BuiltinTag

template <typename T, typename = void>
struct CustomTag
{
  CustomTag() = delete; // used by has_tag
};

namespace detail {

// Builtin tag - must be specialized for each tagged type

template <typename T, typename = void>
struct BuiltinTag
{
  BuiltinTag() = delete; // used by has_tag

  static constexpr cx_string<1> tag_string()
  {
    static_assert(always_false<T>::value, "T has no associated tag");
    return make_cx_string("?"); // reduce the number of errors on static assert fail
  }
};

// Tag - entry point

template <typename T>
struct Tag
{
  using U = remove_cvref_t<T>;
  using type = std::conditional_t<
    std::is_constructible<CustomTag<U>>::value,
    CustomTag<U>,
    BuiltinTag<U>
  >;
};

template <typename T>
using has_tag = std::is_constructible<typename Tag<T>::type>;

// Arithmetic tag

template <typename Arithmetic>
struct BuiltinTag<Arithmetic, enable_spec_if<std::is_arithmetic<Arithmetic>>>
{
  static constexpr cx_string<1> tag_string()
  {
    using S = cx_string<1>;

    constexpr bool sign = std::is_signed<Arithmetic>::value;
    constexpr bool integral = std::is_integral<Arithmetic>::value;
    constexpr std::size_t size = sizeof(Arithmetic);

    return
      (std::is_same<Arithmetic, bool>::value) ? S("y")
    : (std::is_same<Arithmetic, char>::value) ? S("c")
    : (integral &&  sign && size == sizeof(std::int8_t))  ? S("b")
    : (integral &&  sign && size == sizeof(std::int16_t)) ? S("s")
    : (integral &&  sign && size == sizeof(std::int32_t)) ? S("i")
    : (integral &&  sign && size == sizeof(std::int64_t)) ? S("l")
    : (integral && !sign && size == sizeof(std::uint8_t))  ? S("B")
    : (integral && !sign && size == sizeof(std::uint16_t)) ? S("S")
    : (integral && !sign && size == sizeof(std::uint32_t)) ? S("I")
    : (integral && !sign && size == sizeof(std::uint64_t)) ? S("L")
    : (std::is_same<Arithmetic, float>::value) ? S("f")
    : (std::is_same<Arithmetic, double>::value) ? S("d")
    : (std::is_same<Arithmetic, long double>::value) ? S("D")
    : throw "This Arithmetic type is not taggable";
  }
};

// Sequence tag

template <typename Sequence>
struct BuiltinTag<Sequence, enable_spec_if<
  has_tag<sequence_value_t<Sequence>>
>>
{
  static constexpr auto tag_string()
  {
    return cx_strcat(
      make_cx_string("["),
      Tag<sequence_value_t<Sequence>>::type::tag_string()
    );
  }
};

// Tuple tag

template <typename... E, template <class...> class Tuple>
struct BuiltinTag<Tuple<E...>, enable_spec_if<is_tuple<Tuple<E...>>>>
{
  static constexpr auto tag_string()
  {
    return cx_strcat(
      make_cx_string("("),
      Tag<E>::type::tag_string()...,
      make_cx_string(")")
    );
  }
};

// Optional tag

template <typename Optional>
struct BuiltinTag<Optional, enable_spec_if<is_optional<Optional>>>
{
  static constexpr auto tag_string()
  {
    using value_type = decltype(*std::declval<Optional>());
    return cx_strcat(
      make_cx_string("<0"),
      Tag<value_type>::type::tag_string(),
      make_cx_string(">")
    );
  }
};

// Enum tag

template <typename Enum>
struct BuiltinTag<Enum, enable_spec_if<std::is_enum<Enum>>>
  :Tag<std::underlying_type_t<Enum>>::type
{};

} // namespace detail
} // namespace mserialize

#endif // MSERIALIZE_DETAIL_TAG_HPP
