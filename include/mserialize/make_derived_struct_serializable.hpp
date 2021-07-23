#ifndef MSERIALIZE_MAKE_DERIVED_STRUCT_SERIALIZABLE_HPP
#define MSERIALIZE_MAKE_DERIVED_STRUCT_SERIALIZABLE_HPP

#include <mserialize/StructSerializer.hpp>
#include <mserialize/detail/foreach.hpp>
#include <mserialize/detail/preprocessor.hpp>
#include <mserialize/make_struct_serializable.hpp>
#include <mserialize/serialize.hpp>

#include <cstddef>

/**
 * MSERIALIZE_MAKE_DERIVED_STRUCT_SERIALIZABLE(StructName, (Bases...), members...)
 *
 * Define a CustomSerializer specialization for the given
 * struct or class, that has serializable base classes,
 * allowing it to be serialized using mserialize::serialize,
 * without repeating the fields of its bases.
 *
 * The first argument of the macro must be the name
 * of the type to be made serializable.
 * The second argument is a parenthesised list of serializable base-classes
 * that `StructName` derives from.
 * Following the second argument come the members,
 * which are either accessible fields or getters.
 * For more on the allowed members, see: mserialize::serializable_member.
 *
 * Example:
 *
 *     struct Person {
 *       int age;
 *       std::string name() const;
 *     };
 *     MSERIALIZE_MAKE_STRUCT_SERIALIZABLE(Person, age, name)
 *
 *     struct Employee : Person {
 *       std::string eid;
 *     };
 *     MSERIALIZE_MAKE_DERIVED_STRUCT_SERIALIZABLE(Employee, (Person), eid)
 *
 * The macro has to be called in global scope
 * (outside of any namespace).
 *
 * The list of base-classes must not be empty.
 * The member list can be empty.
 * The member list does not have to enumerate every member
 * of the given type: if a member is omitted, it will
 * be simply ignored during serialization.
 *
 * The maximum number of members is limited by
 * mserialize/detail/foreach.hpp, currently 100.
 *
 * If a private member has to be serialized, the following friend
 * declaration can be added to the type declaration:
 *
 *     template <typename, typename>
 *     friend struct mserialize::CustomSerializer;
 */
#define MSERIALIZE_MAKE_DERIVED_STRUCT_SERIALIZABLE(derived, ...)     \
  namespace mserialize {                                              \
  template <>                                                         \
  struct CustomSerializer<derived, void>                              \
  {                                                                   \
    using DerivedSerializer = StructSerializer<derived                \
      MSERIALIZE_FOREACH(                                             \
        MSERIALIZE_SERIALIZABLE_MEMBER,                               \
        derived,                                                      \
        __VA_ARGS__                                                   \
      )                                                               \
     >;                                                               \
    static std::size_t serialized_size(const derived& t)              \
    {                                                                 \
      return                                                          \
        MSERIALIZE_FOREACH(                                           \
          MSERIALIZE_BASE_SERIALIZED_SIZE_EXPR, _, x,                 \
          MSERIALIZE_UNTUPLE(MSERIALIZE_FIRST(__VA_ARGS__))           \
        )                                                             \
        DerivedSerializer::serialized_size(t);                        \
    }                                                                 \
    template <typename OutputStream>                                  \
    static void serialize(const derived& t, OutputStream& ostream)    \
    {                                                                 \
      MSERIALIZE_FOREACH(                                             \
        MSERIALIZE_BASE_SERIALIZE_EXPR, _, x,                         \
        MSERIALIZE_UNTUPLE(MSERIALIZE_FIRST(__VA_ARGS__))             \
      )                                                               \
      DerivedSerializer::serialize(t, ostream);                       \
    }                                                                 \
  };                                                                  \
  } /* namespace mserialize */                                        \
  /**/

#define MSERIALIZE_BASE_SERIALIZED_SIZE_EXPR(_, b) mserialize::serialized_size<b>(t) +
#define MSERIALIZE_BASE_SERIALIZE_EXPR(_, b) mserialize::serialize<b>(t, ostream);

#endif // MSERIALIZE_MAKE_DERIVED_STRUCT_SERIALIZABLE_HPP
