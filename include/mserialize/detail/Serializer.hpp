#ifndef MSERIALIZE_DETAIL_SERIALIZER_HPP
#define MSERIALIZE_DETAIL_SERIALIZER_HPP

#include <mserialize/detail/sequence_traits.hpp>
#include <mserialize/detail/type_traits.hpp>

#include <cassert>
#include <cstdint>
#include <ios> // streamsize (macOS)
#include <type_traits>

namespace mserialize {

template <typename T, typename OutputStream>
void serialize(const T& in, OutputStream& ostream);

template <typename T>
std::size_t serialized_size(const T& in);

// Custom serializer - can be specialized for any type, takes precedence over BuiltinSerializer

template <typename T, typename = void>
struct CustomSerializer
{
  CustomSerializer() = delete; // used by is_serializable
};

namespace detail {

// Builtin serializer - must be specialized for each supported type

template <typename T, typename = void>
struct BuiltinSerializer
{
  BuiltinSerializer() = delete; // used by is_serializable

  template <typename OutputStream>
  static void serialize(const T& /* t */, OutputStream& /* ostream */)
  {
    static_assert(always_false<T>::value, "T is not serializable");
  }

  static std::size_t serialized_size(const T& /* t */)
  {
    static_assert(always_false<T>::value, "T is not serializable");
    return 0; // reduce the number of errors on static assert fail
  }
};

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

// Trivial serializer

template <typename T>
struct TrivialSerializer
{
  static_assert(std::is_trivially_copyable<T>::value, "");

  template <typename OutputStream>
  static void serialize(const T t, OutputStream& ostream)
  {
    ostream.write(reinterpret_cast<const char*>(&t), sizeof(T));
  }

  static std::size_t serialized_size(const T)
  {
    return sizeof(T);
  }
};

// Arithmetic serializer

template <typename Arithmetic>
struct BuiltinSerializer<Arithmetic, enable_spec_if<std::is_arithmetic<Arithmetic>>>
  :TrivialSerializer<Arithmetic>
{};

// Enum serializer

template <typename Enum>
struct BuiltinSerializer<Enum, enable_spec_if<std::is_enum<Enum>>>
  :TrivialSerializer<Enum>
{};

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

  static std::size_t serialized_size(const Sequence& s)
  {
    // Note: we are using is_arithmetic to fast-path some types.
    // The correct trait would be has_fixed_size, which could
    // include tuples of fixed size objects and fixed size
    // sequences (std::array, C array) of fixed size objects.
    // However, that would need a lot of code, so we just
    // let the optimizer do the job.
    return sizeof(std::uint32_t)
         + sizeof_elems(std::is_arithmetic<sequence_data_t<const Sequence>>{}, s);
  }

private:
  using value_type = sequence_value_t<Sequence>;

  template <typename OutputStream>
  static void serialize_elems(
    std::false_type /* no batch copy */,
    const Sequence& s, std::uint32_t /* size */, OutputStream& ostream
  )
  {
    for (auto&& elem : s)
    {
      mserialize::serialize<value_type>(elem, ostream);
    }
  }

  template <typename OutputStream>
  static void serialize_elems(
    std::true_type /* batch copy */,
    const Sequence& s, std::uint32_t size, OutputStream& ostream
  )
  {
    // Avoid passing nullptr `data` to write, that ends up calling memcpy
    // (e.g: if OutputStream is QueueWriter) as it is undefined behavior
    // C11 7.24.1 String function conventions p2
    if (size)
    {
      const char* data = reinterpret_cast<const char*>(sequence_data(s));
      const size_t serialized_size = sizeof(sequence_data_t<const Sequence>) * size;
      ostream.write(data, std::streamsize(serialized_size));
    }
  }

  static std::size_t sizeof_elems(
    std::false_type /* value_type is not arithmetic */,
    const Sequence& s
  )
  {
    std::size_t result = 0;
    for (auto&& elem : s)
    {
      result += mserialize::serialized_size<value_type>(elem);
    }
    return result;
  }

  static std::size_t sizeof_elems(
    std::true_type /* value_type is arithmetic */,
    const Sequence& s
  )
  {
    return std::size_t(sequence_size(s)) * sizeof(sequence_data_t<const Sequence>);
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

  static std::size_t serialized_size(const Tuple<E...>& t)
  {
    return serialized_size_impl(t, std::index_sequence_for<E...>{});
  }

private:
  template <typename OutputStream, std::size_t... I>
  static void serialize_elems(const Tuple<E...>& t, OutputStream& ostream, std::index_sequence<I...>)
  {
    using swallow = int[];
    using std::get;
    (void)swallow{1, (mserialize::serialize(get<I>(t), ostream), int{})...};
  }

  template <std::size_t... I>
  static std::size_t serialized_size_impl(const Tuple<E...>& t, std::index_sequence<I...>)
  {
    using std::get;
    const std::size_t elem_sizes[] = {0, mserialize::serialized_size(get<I>(t))...};

    std::size_t result = 0;
    for (auto s : elem_sizes) { result += s; }
    return result;
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

  static std::size_t serialized_size(const Optional& opt)
  {
    return (opt)
      ? sizeof(std::uint8_t) + mserialize::serialized_size(*opt)
      : sizeof(std::uint8_t);
  }
};

} // namespace detail
} // namespace mserialize

#endif // MSERIALIZE_DETAIL_SERIALIZER_HPP
