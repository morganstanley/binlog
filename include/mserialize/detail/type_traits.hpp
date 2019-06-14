#ifndef MSERIALIZE_DETAIL_TYPE_TRAITS_HPP
#define MSERIALIZE_DETAIL_TYPE_TRAITS_HPP

#include <type_traits>

namespace mserialize {
namespace detail {

// void_t

// C++17: template <typename...> using void_t = void;
// A different implementation is here to protect against CWG 1558
template <typename... Ts> struct make_void { using type = void; };
template <typename... Ts> using void_t = typename make_void<Ts...>::type;

// enable_spec_if

template <typename Pred>
using enable_spec_if = void_t<std::enable_if_t<Pred::value, void>>;

// always_false

// Using this in static_assert does not violate
// [temp.res]/8 "If no valid specialization can be generated ... the template is ill-formed",
// as it is theoretically possible to specialize this helper, e.g:
// template <> struct always_false<X> : std::true_type {};
template <typename>
struct always_false : std::false_type {};

} // namespace detail
} // namespace mserialize

#endif // MSERIALIZE_DETAIL_TYPE_TRAITS_HPP
