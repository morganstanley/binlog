#ifndef MSERIALIZE_STRUCT_DESERIALIZER_HPP
#define MSERIALIZE_STRUCT_DESERIALIZER_HPP

#include <mserialize/deserialize.hpp>
#include <mserialize/detail/type_traits.hpp>

#include <type_traits>
#include <utility>

namespace mserialize {

/**
 * Resolve a (possibly overloaded) member pointer
 * to a deserializable member.
 *
 * This function is declared (but not defined)
 * for a member pointer (&T::m), if:
 *
 *  - `m` is non-static, non-reference, non-bitfield data member, or
 *  - `m` is a non-const, unary member function.
 *    (also see Limitation below)
 *
 * Usage: decltype(mserialize::deserializable_member(&T::m))
 *
 * This can be used together with StructDeserializer, when
 * the actual type of the member is not known, e.g:
 * the StructDeserializer is instantiated by a generic macro.
 *
 * Limitation: if `m` is a member function, it cannot be overloaded
 * with a member function template, which has its arguments defaulted,
 * e.g, the following will not compile:
 *
 * struct T {
 *   void m(int&);
 *   template <typename U> void m(U = 0);
 * };
 *
 * using M = decltype(mserialize::serializable_member(&T::m)); // fails to compile
 */

template <typename T, typename Field>
auto deserializable_member(Field T::*field) -> decltype(field);

template <typename T, typename Arg, typename Ret>
auto deserializable_member(Ret (T::*setter)(Arg)) -> decltype(setter);

/**
 * Deserialize the given members of a custom type `T`.
 *
 * `Members...` is a pack of std::integral_constant<M, m>,
 * where each `M` is a member pointer type of a single `T` type,
 * and `m` is a member pointer of type `M` and:
 *
 *  - `m` points to a non-static, deserializable data member, or
 *  - `m` points to a non-const, unary member function, which takes
 *    a default constructible, deserializable object.
 *
 * This helper class can be used to define a CustomDeserializer for user defined structures:
 *
 *     namespace mserialize {
 *     template <>
 *     struct CustomDeserializer<T, void>
 *       : StructDeserializer<
 *           T,
 *           std::integral_constant<decltype(&T::field), &T::field>,
 *           std::integral_constant<decltype(&T::setter), &T::setter>,
 *       >
 *     {};
 *     } // namespace mserialize
 *
 * If `setter` is (possibly) overloaded, `deserializable_member` can be used to resolve it
 * in a generic way (as it does the right thing for data members as well):
 *
 *     std::integral_constant<decltype(deserializable_member(&T::overloaded_setter)), &T::overloaded_setter>
 *
 * If a private or protected member of `T` needs to be deserialized,
 * the following friend declaration can be added to the declaration of `T`:
 *
 *     template <typename, typename>
 *     friend struct mserialize::CustomDeserializer;
 *
 * Note: C++ does not allow taking the address of reference members or bitfields,
 * therefore those cannot be deserialized directly: a setter must be used instead.
 */
template <typename T, typename... Members>
struct StructDeserializer
{
  template <typename InputStream>
  static void deserialize(T& t, InputStream& istream)
  {
    using swallow = int[];
    (void)swallow{1, (deserialize_member(t, Members::value, istream), int{})...};
  }

private:
  template <typename Field, typename InputStream>
  static void deserialize_member(T& t, Field T::*field, InputStream& istream)
  {
    mserialize::deserialize(t.*field, istream);
  }

  template <typename Ret, typename Arg, typename InputStream>
  static void deserialize_member(T& t, Ret (T::*setter)(Arg), InputStream& istream)
  {
    detail::remove_cvref_t<Arg> arg;
    mserialize::deserialize(arg, istream);
    (t.*setter)(std::move(arg));
  }
};

} // namespace mserialize

#endif // MSERIALIZE_STRUCT_DESERIALIZER_HPP
