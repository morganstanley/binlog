#ifndef MSERIALIZE_DETAIL_SEQUENCE_TRAITS_HPP
#define MSERIALIZE_DETAIL_SEQUENCE_TRAITS_HPP

#include <mserialize/detail/type_traits.hpp>

#include <iterator>
#include <vector>

namespace mserialize {
namespace detail {

// Sequence iterator

template <typename Sequence>
auto sequence_iterator(Sequence& s, char) -> decltype(begin(s));

template <typename Sequence>
auto sequence_iterator(Sequence& s, int) -> decltype(std::begin(s));

template <typename Sequence>
using sequence_iterator_t = decltype(sequence_iterator(std::declval<Sequence&>(), 0));

// Is forward iterator

template <typename Iterator>
using is_forward_iterator = std::is_convertible<
  typename std::iterator_traits<Iterator>::iterator_category,
  std::forward_iterator_tag
>;

// Is serializable iterator

template <typename Iterator>
using is_serializable_iterator = conjunction<
  is_forward_iterator<Iterator>,
  is_serializable<typename std::iterator_traits<Iterator>::value_type>
>;

// Is desierializable iterator

template <typename Iterator>
using is_deserializable_iterator = conjunction<
  is_forward_iterator<Iterator>,
  is_deserializable<typename std::iterator_traits<Iterator>::value_type>
>;

// Sequence size

template <typename Sequence, typename = void>
struct SequenceSize
{
  static auto value(const Sequence& s)
  {
    using std::begin;
    using std::end;
    return std::distance(begin(s), end(s));
  }
};

template <typename Sequence>
struct SequenceSize<Sequence, void_t<decltype(&Sequence::size)>>
{
  static auto value(const Sequence& s) { return s.size(); }
};

template <typename T, size_t N>
struct SequenceSize<const T(&)[N], void>
{
  static size_t value(const T(&)[N]) { return N; }
};

// Complexity:
//  - Constant for C arrays and containers with .size()
//  - Otherwise the same a std::distance(begin_of_s, end_of_s)
template <typename Sequence>
auto sequence_size(const Sequence& s)
{
  return SequenceSize<Sequence>::value(s);
}

// Is proxy sequence

template <typename Sequence>
struct is_proxy_sequence : std::false_type {};

template <>
struct is_proxy_sequence<std::vector<bool>> : std::true_type {};

} // namespace detail
} // namespace mserialize

#endif // MSERIALIZE_DETAIL_SEQUENCE_TRAITS_HPP
