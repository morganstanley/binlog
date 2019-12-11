#ifndef MSERIALIZE_DETAIL_DESERIALIZER_HPP
#define MSERIALIZE_DETAIL_DESERIALIZER_HPP

#include <mserialize/detail/sequence_traits.hpp>
#include <mserialize/detail/type_traits.hpp>

#include <cassert>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string> // to_string
#include <type_traits>
#include <utility> // move

namespace mserialize {

template <typename T, typename InputStream>
void deserialize(T& out, InputStream& istream);

// Custom deserializer - can be specialized for any type, takes precedence over BuiltinDeserializer

template <typename T, typename = void>
struct CustomDeserializer
{
  CustomDeserializer() = delete; // used by is_deserializable
};

namespace detail {

// Builtin deserializer - must be specialized for each supported type

template <typename T, typename = void>
struct BuiltinDeserializer
{
  BuiltinDeserializer() = delete; // used by is_deserializable

  template <typename InputStream>
  static void deserialize(const T& /* t */, InputStream& /* istream */)
  {
    static_assert(always_false<T>::value, "T is not deserializable");
  }
};

// Deserializer - entry point

template <typename T>
struct Deserializer
{
  using type = std::conditional_t<
    std::is_constructible<CustomDeserializer<T>>::value,
    CustomDeserializer<T>,
    BuiltinDeserializer<T>
  >;
};

// Trivial deserializer

template <typename T>
struct TrivialDeserializer
{
  static_assert(std::is_trivially_copyable<T>::value, "");

  template <typename InputStream>
  static void deserialize(T& t, InputStream& istream)
  {
    istream.read(reinterpret_cast<char*>(&t), sizeof(T));
  }
};

// Arithmetic deserializer

template <typename Arithmetic>
struct BuiltinDeserializer<Arithmetic, enable_spec_if<std::is_arithmetic<Arithmetic>>>
  :TrivialDeserializer<Arithmetic>
{};

// Enum deserializer

template <typename Enum>
struct BuiltinDeserializer<Enum, enable_spec_if<std::is_enum<Enum>>>
  :TrivialDeserializer<Enum>
{};

// Sequence deserializer

template <typename Sequence>
struct BuiltinDeserializer<Sequence, enable_spec_if<
  is_deserializable_iterator<sequence_iterator_t<Sequence>>
>>
{
  template <typename InputStream>
  static void deserialize(Sequence& s, InputStream& istream)
  {
    std::uint32_t size;
    mserialize::deserialize(size, istream);

    deserialize_elems(has_insert<Sequence>{}, s, size, istream);
  }

private:
  // e.g: [multi]{map,set}, unordered_{map,set}
  template <typename InputStream>
  static void deserialize_elems(
    std::true_type  /* has insert */,
    Sequence& s, std::uint32_t size, InputStream& istream
  )
  {
    // map value_type is pair<const K, V>, we need pair<K, V>
    using T = deep_remove_const_t<typename Sequence::value_type>;

    while (size--)
    {
      T elem;
      mserialize::deserialize(elem, istream);
      s.insert(std::move(elem));
    }
  }

  template <typename InputStream>
  static void deserialize_elems(
    std::false_type  /* no insert */,
    Sequence& s, std::uint32_t size, InputStream& istream
  )
  {
    resize(has_resize<Sequence>{}, s, size);

    deserialize_elems_noinsert(
      is_sequence_batch_deserializable<Sequence>{},
      is_proxy_sequence<Sequence>{},
      s, size, istream
    );
  }

  static void resize(std::true_type /* has resize */, Sequence& s, std::uint32_t new_size)
  {
    return s.resize(new_size);
  }

  static void resize(std::false_type /* no resize */, Sequence& s, std::uint32_t new_size)
  {
    const auto size = sequence_size(s);
    if (size != new_size)
    {
      throw std::runtime_error(
        "Serialized sequence size = " + std::to_string(new_size)
        + " != " + std::to_string(size) + " = target size, "
        "and target cannot be .resize()-ed"
      );
    }
  }

  // e.g: vector, array, C array - value type is trivial
  template <typename InputStream>
  static void deserialize_elems_noinsert(
    std::true_type  /* batch deserialization */,
    std::false_type /* not a proxy sequence */,
    Sequence& s, std::uint32_t size, InputStream& istream
  )
  {
    char* data = reinterpret_cast<char*>(sequence_data(s));
    const size_t serialized_size = sizeof(sequence_data_t<Sequence>) * size;
    istream.read(data, std::streamsize(serialized_size));
  }

  // e.g: any deque, list, forward_list -
  // and vector, array, C array of non trivial value type
  template <typename InputStream>
  static void deserialize_elems_noinsert(
    std::false_type /* no batch deserialization */,
    std::false_type /* not a proxy sequence */,
    Sequence& s, std::uint32_t /*size*/, InputStream& istream
  )
  {
    for (auto&& elem : s)
    {
      mserialize::deserialize(elem, istream);
    }
  }

  // e.g: vector<bool>
  template <typename InputStream, typename Ignored>
  static void deserialize_elems_noinsert(
    Ignored        /* dont care about batch deserialization */,
    std::true_type /* proxy sequence */,
    Sequence& s, std::uint32_t /*size*/, InputStream& istream
  )
  {
    // For some sequences (e.g: std::vector<bool>)
    // S::reference != S::value_type &
    // as a proxy object is used.

    using value_type = typename Sequence::value_type;

    for (auto&& proxy : s)
    {
      value_type elem;
      mserialize::deserialize(elem, istream);
      proxy = std::move(elem);
    }
  }
};

// Tuple deserializer

template <typename... E, template <class...> class Tuple>
struct BuiltinDeserializer<Tuple<E...>, enable_spec_if<is_tuple<Tuple<E...>>>>
{
  template <typename InputStream>
  static void deserialize(Tuple<E...>& t, InputStream& istream)
  {
    deserialize_elems(t, istream, std::index_sequence_for<E...>{});
  }

private:
  template <typename InputStream, std::size_t... I>
  static void deserialize_elems(Tuple<E...>& t, InputStream& istream, std::index_sequence<I...>)
  {
    using swallow = int[];
    using std::get;
    (void)swallow{1, (mserialize::deserialize(get<I>(t), istream), int{})...};
  }
};

// Optional deserializer

template <typename Optional>
struct BuiltinDeserializer<Optional, enable_spec_if<conjunction<
  is_optional<Optional>,
  negation<std::is_pointer<Optional>>
>>>
{
  template <typename InputStream>
  static void deserialize(Optional& opt, InputStream& istream)
  {
    std::uint8_t discriminator;
    mserialize::deserialize(discriminator, istream);
    if (discriminator)
    {
      assert(discriminator == 1 && "Discriminator of non-empty optional must be 1");
      make_nonempty(opt);
      mserialize::deserialize(*opt, istream);
    }
    else
    {
      opt = {};
    }
  }

private:
  template <typename Opt>
  static void_t<typename Opt::value_type> make_nonempty(Opt& opt)
  {
    using T = typename Opt::value_type;
    opt = T{};
  }

  template <typename SmartPtr>
  static void_t<typename SmartPtr::element_type> make_nonempty(SmartPtr& ptr)
  {
    using T = typename SmartPtr::element_type;
    ptr.reset(new T{}); // NOLINT(cppcoreguidelines-owning-memory)
  }
};

} // namespace detail
} // namespace mserialize

#endif // MSERIALIZE_DETAIL_DESERIALIZER_HPP
