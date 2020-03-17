#ifndef BINLOG_ADAPT_STDOPTIONAL_HPP
#define BINLOG_ADAPT_STDOPTIONAL_HPP

// Make std::optional loggable by including this file

#include <mserialize/detail/type_traits.hpp>

#include <optional>
#include <type_traits>

namespace mserialize {
namespace detail {

template <typename T>
struct is_optional<std::optional<T>> : std::true_type {};

} // namespace detail
} // namespace mserialize

#endif // BINLOG_ADAPT_STDOPTIONAL_HPP
