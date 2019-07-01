#ifndef MSERIALIZE_DETAIL_SERIALIZER_HPP
#define MSERIALIZE_DETAIL_SERIALIZER_HPP

#include <mserialize/detail/sequence_traits.hpp>
#include <mserialize/detail/type_traits.hpp>

#include <cassert>
#include <cstdint>
#include <type_traits>

namespace mserialize {

template <typename T, typename OutputStream>
void serialize(const T& in, OutputStream& ostream);

namespace detail {

// Invalid serializer

struct InvalidSerializer
{
  template <typename T, typename OutputStream>
  static void serialize(const T& /* t */, OutputStream& /* ostream */)
  {
    static_assert(always_false<T>::value, "T is not serializable");
  }
};

} // namespace detail

// Custom serializer - can be specialized for any type, takes precedence over BuiltinSerializer

template <typename T, typename = void>
struct CustomSerializer;

namespace detail {

// Builtin serializer - must be specialized for each supported type

template <typename T, typename = void>
struct BuiltinSerializer : InvalidSerializer {};

// Serializer - entry point

template <typename T>
struct Serializer
{
  using type = std::conditional_t<
    std::is_constructible<CustomSerializer<T>>::value,
    CustomSerializer<T>,
    BuiltinSerializer<T>
  >;
};

// Arithmetic serializer

template <typename Arithmetic>
struct BuiltinSerializer<Arithmetic, enable_spec_if<std::is_arithmetic<Arithmetic>>>
{
  template <typename OutputStream>
  static void serialize(const Arithmetic t, OutputStream& ostream)
  {
    ostream.write(reinterpret_cast<const char*>(&t), sizeof(Arithmetic));
  }
};

// Sequence serializer

template <typename Sequence>
struct BuiltinSerializer<Sequence, enable_spec_if<
  is_serializable_iterator<sequence_iterator_t<Sequence>>
>>
{
  template <typename OutputStream>
  static void serialize(const Sequence& s, OutputStream& ostream)
  {
    const auto size = sequence_size(s);
    const auto size32 = std::uint32_t(size);
    assert(size32 == size && "sequence size must fit on 32 bits");

    mserialize::serialize(size32, ostream);
    serialize_elems(is_sequence_batch_serializable<const Sequence>{}, s, size32, ostream);
  }

private:
  template <typename OutputStream>
  static void serialize_elems(
    std::false_type /* no batch copy */,
    const Sequence& s, std::uint32_t /* size */, OutputStream& ostream
  )
  {
    for (auto&& elem : s)
    {
      mserialize::serialize(elem, ostream);
    }
  }

  template <typename OutputStream>
  static void serialize_elems(
    std::true_type /* batch copy */,
    const Sequence& s, std::uint32_t size, OutputStream& ostream)
  {
    const char* data = reinterpret_cast<const char*>(sequence_data(s));
    const size_t serialized_size = sizeof(sequence_data_t<const Sequence>) * size;
    ostream.write(data, std::streamsize(serialized_size));
  }
};

// Tuple serializer

template <typename... E, template <class...> class Tuple>
struct BuiltinSerializer<Tuple<E...>, enable_spec_if<is_tuple<Tuple<E...>>>>
{
  template <typename OutputStream>
  static void serialize(const Tuple<E...>& t, OutputStream& ostream)
  {
    serialize_elems(t, ostream, std::index_sequence_for<E...>{});
  }

private:
  template <typename OutputStream, std::size_t... I>
  static void serialize_elems(const Tuple<E...>& t, OutputStream& ostream, std::index_sequence<I...>)
  {
    using swallow = int[];
    using std::get;
    (void)swallow{1, (mserialize::serialize(get<I>(t), ostream), int{})...};
  }
};

// Optional serializer

template <typename Optional>
struct BuiltinSerializer<Optional, enable_spec_if<is_optional<Optional>>>
{
  template <typename OutputStream>
  static void serialize(const Optional& opt, OutputStream& ostream)
  {
    if (opt)
    {
      mserialize::serialize(std::uint8_t{1}, ostream);
      mserialize::serialize(*opt, ostream);
    }
    else
    {
      mserialize::serialize(std::uint8_t{0}, ostream);
    }
  }
};

} // namespace detail
} // namespace mserialize

#endif // MSERIALIZE_DETAIL_SERIALIZER_HPP
