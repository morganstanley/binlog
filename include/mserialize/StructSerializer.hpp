#ifndef MSERIALIZE_STRUCT_SERIALIZER_HPP
#define MSERIALIZE_STRUCT_SERIALIZER_HPP

#include <mserialize/serialize.hpp>

#include <type_traits>

namespace mserialize {

/**
 * Resolve a (possibly overloaded) member pointer
 * to a serializable member.
 *
 * This function is declared (but not defined)
 * for a member pointer (&T::m), if:
 *
 *  - `m` is non-static, non-reference, non-bitfield data member, or
 *  - `m` is a const qualified, nullary member function.
 *    (also see Limitation below)
 *
 * Usage: decltype(mserialize::serializable_member(&T::m))
 *
 * This can be used together with StructSerializer, when
 * the actual type of the member is not known, e.g:
 * the StructSerializer is instantiated by a generic macro.
 *
 * Limitation: if `m` is a member function, it cannot be overloaded
 * with a member function template, which has its arguments defaulted,
 * e.g, the following will not compile:
 *
 * struct T {
 *   int m() const;
 *   template <typename U> int m(U = 0) const;
 * };
 *
 * using M = decltype(mserialize::serializable_member(&T::m)); // fails to compile
 */

template <typename T, typename Field>
auto serializable_member(Field T::*field) -> decltype(field);

template <typename T, typename Ret>
auto serializable_member(Ret (T::*getter)() const) -> decltype(getter);

#if __cplusplus >= 201703L
template <typename T, typename Ret>
auto serializable_member(Ret (T::*getter)() const noexcept) -> decltype(getter);
#endif

/**
 * Serialize the given members of a custom type `T`.
 *
 * `Members...` is a pack of std::integral_constant<M, m>,
 * where each `M` is a member pointer type of a single `T` type,
 * and `m` is a member pointer of type `M` and:
 *
 *  - `m` points to a non-static, serializable data member, or
 *  - `m` points to a const qualified, nullary member function, which returns
 *    a serializable object.
 *
 * This helper class can be used to define a CustomSerializer for user defined structures:
 *
 *     namespace mserialize {
 *     template <>
 *     struct CustomSerializer<T, void>
 *       : StructSerializer<
 *           T,
 *           std::integral_constant<decltype(&T::field), &T::field>,
 *           std::integral_constant<decltype(&T::getter), &T::getter>,
 *       >
 *     {};
 *     } // namespace mserialize
 *
 * If `getter` is (possibly) overloaded, `serializable_member` can be used to resolve it
 * in a generic way (as it does the right thing for data members as well):
 *
 *     std::integral_constant<decltype(serializable_member(&T::overloaded_getter)), &T::overloaded_getter>
 *
 * If a private or protected member of `T` needs to be serialized,
 * the following friend declaration can be added to the declaration of `T`:
 *
 *     template <typename, typename>
 *     friend struct mserialize::CustomSerializer;
 *
 * Note: C++ does not allow taking the address of reference members or bitfields,
 * therefore those cannot be serialized directly: a getter must be used instead.
 */
template <typename T, typename... Members>
struct StructSerializer
{
  template <typename OutputStream>
  static void serialize(const T& t, OutputStream& ostream)
  {
    using swallow = int[];
    (void)swallow{1, (serialize_member(t, Members::value, ostream), int{})...};
  }

  static std::size_t serialized_size(const T& t)
  {
    const std::size_t field_sizes[] = {0, serialized_size_member(t, Members::value)...};

    std::size_t result = 0;
    for (auto s : field_sizes) { result += s; }
    return result;
  }

private:
  // U might be the base of T, if T got adopted through a concept

  template <typename U, typename Field, typename OutputStream>
  static void serialize_member(const T& t, Field U::*field, OutputStream& ostream)
  {
    mserialize::serialize(t.*field, ostream);
  }

  template <typename U, typename Field, typename OutputStream>
  static void serialize_member(const T& t, Field (U::*getter)() const, OutputStream& ostream)
  {
    mserialize::serialize((t.*getter)(), ostream);
  }

#if __cplusplus >= 201703L
  template <typename U, typename Field, typename OutputStream>
  static void serialize_member(const T& t, Field (U::*getter)() const noexcept, OutputStream& ostream)
  {
    mserialize::serialize((t.*getter)(), ostream);
  }
#endif

  template <typename U, typename Field>
  static std::size_t serialized_size_member(const T& t, Field U::*field)
  {
    return mserialize::serialized_size(t.*field);
  }

  template <typename U, typename Field>
  static std::size_t serialized_size_member(const T& t, Field (U::*getter)() const)
  {
    return mserialize::serialized_size((t.*getter)());
  }

#if __cplusplus >= 201703L
  template <typename U, typename Field>
  static std::size_t serialized_size_member(const T& t, Field (U::*getter)() const noexcept)
  {
    return mserialize::serialized_size((t.*getter)());
  }
#endif
};

} // namespace mserialize

#endif // MSERIALIZE_STRUCT_SERIALIZER_HPP
