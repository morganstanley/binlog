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

// Sequence value type via iterator

template <typename Sequence>
using sequence_value_t = typename std::iterator_traits<
  sequence_iterator_t<Sequence>
>::value_type;

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

// Sequence data

template <typename Sequence>
auto sequence_data(Sequence& s, int = 0) -> decltype(s.data())
{
  return s.data();
}

template <typename T, size_t N>
T* sequence_data(T (&array)[N], int = 0) noexcept
{
  return std::begin(array);
}

template <typename T>
void sequence_data(T&, char);

template <typename Sequence>
using sequence_data_ptr_t = decltype(sequence_data(std::declval<Sequence&>(), 0));

template <typename Sequence>
using sequence_data_t = std::remove_pointer_t<sequence_data_ptr_t<Sequence>>;

// Sequence has contiguous data

template <typename Sequence>
using sequence_has_contiguous_data = std::is_pointer<
  sequence_data_ptr_t<Sequence>
>;

// Is sequence batch serializable

template <typename Sequence>
using is_sequence_batch_serializable = conjunction<
  sequence_has_contiguous_data<Sequence>,
  std::is_arithmetic<sequence_data_t<Sequence>>
>;

// Is sequence batch deserializable

template <typename Sequence>
using is_sequence_batch_deserializable = conjunction<
  sequence_has_contiguous_data<Sequence>,
  std::is_arithmetic<sequence_data_t<Sequence>>,
  negation<std::is_const<sequence_data_t<Sequence>>>
>;

// Is proxy sequence

template <typename Sequence>
struct is_proxy_sequence : std::false_type {};

template <>
struct is_proxy_sequence<std::vector<bool>> : std::true_type {};

// Has resize

template <typename, typename = void>
struct has_resize : std::false_type {};

template <typename Sequence>
struct has_resize<Sequence, void_t<decltype(std::declval<Sequence>().resize(0u))>> : std::true_type {};

// Has insert - matches insert(vt) (set/map), but not insert(it, vt) (sequences)

template <typename Sequence, typename = void>
struct has_insert : std::false_type {};

template <typename Sequence>
struct has_insert<Sequence, void_t<decltype(
  std::declval<Sequence>().insert(std::declval<typename Sequence::value_type>())
)>> : std::true_type {};

} // namespace detail
} // namespace mserialize

#endif // MSERIALIZE_DETAIL_SEQUENCE_TRAITS_HPP
