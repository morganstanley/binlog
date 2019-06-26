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

// Forward declarations

template <typename Arithmetic>
struct ArithmeticSerializer;

template <typename Sequence>
struct SequenceSerializer;

// Builtin serializer - one specialization for each category

template <typename T, typename = void>
struct BuiltinSerializer : InvalidSerializer {};

template <typename T>
struct BuiltinSerializer<T, enable_spec_if<std::is_arithmetic<T>>>
  : ArithmeticSerializer<T> {};

template <typename T>
struct BuiltinSerializer<T, enable_spec_if<
    is_serializable_iterator<sequence_iterator_t<T>>
>> : SequenceSerializer<T> {};

// Serializer - entry point

template <typename T>
struct  Serializer
{
  using type = BuiltinSerializer<T>;
};

// Arithmetic serializer

template <typename Arithmetic>
struct ArithmeticSerializer
{
  template <typename OutputStream>
  static void serialize(const Arithmetic t, OutputStream& ostream)
  {
    ostream.write(reinterpret_cast<const char*>(&t), sizeof(Arithmetic));
  }
};

// Sequence serializer

template <typename Sequence>
struct SequenceSerializer
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

} // namespace detail
} // namespace mserialize

#endif // MSERIALIZE_DETAIL_SERIALIZER_HPP
