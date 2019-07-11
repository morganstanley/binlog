#ifndef MSERIALIZE_DETAIL_TAG_HPP
#define MSERIALIZE_DETAIL_TAG_HPP

#include <mserialize/cx_string.hpp>
#include <mserialize/detail/sequence_traits.hpp>
#include <mserialize/detail/type_traits.hpp>

#include <type_traits>

namespace mserialize {

// Custom tag - can be specialized for any type, takes precedence over BuiltinTag

template <typename T>
struct CustomTag;

namespace detail {

// Builtin tag - must be specialized for each tagged type

template <typename T, typename = void>
struct BuiltinTag
{
  BuiltinTag() = delete; // used by has_tag

  static constexpr cx_string<1> tag_string()
  {
    static_assert(always_false<T>::value, "T has no associated tag");
    return cx_string<1>("?"); // reduce the number of errors on static assert fail
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
  static constexpr bool sign = std::is_signed<Arithmetic>::value;
  static constexpr bool integral = std::is_integral<Arithmetic>::value;
  static constexpr std::size_t size = sizeof(Arithmetic);

  static constexpr cx_string<1> tag_string()
  {
    using S = cx_string<1>;
    return
      (std::is_same<Arithmetic, bool>::value) ? S("y")
    : (std::is_same<Arithmetic, char>::value) ? S("c")
    : (integral &&  sign && size == 1) ? S("b")
    : (integral &&  sign && size == 2) ? S("s")
    : (integral &&  sign && size == 4) ? S("i")
    : (integral &&  sign && size == 8) ? S("l")
    : (integral && !sign && size == 1) ? S("B")
    : (integral && !sign && size == 2) ? S("S")
    : (integral && !sign && size == 4) ? S("I")
    : (integral && !sign && size == 8) ? S("L")
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
      cx_string<1>("["),
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
      cx_string<1>("("),
      Tag<E>::type::tag_string()...,
      cx_string<1>(")")
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
      cx_string<2>("<0"),
      Tag<value_type>::type::tag_string(),
      cx_string<1>(">")
    );
  }
};

} // namespace detail
} // namespace mserialize

#endif // MSERIALIZE_DETAIL_TAG_HPP
