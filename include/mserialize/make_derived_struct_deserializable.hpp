#ifndef MSERIALIZE_MAKE_DERIVED_STRUCT_DESERIALIZABLE_HPP
#define MSERIALIZE_MAKE_DERIVED_STRUCT_DESERIALIZABLE_HPP

#include <mserialize/StructDeserializer.hpp>
#include <mserialize/deserialize.hpp>
#include <mserialize/detail/foreach.hpp>
#include <mserialize/detail/preprocessor.hpp>
#include <mserialize/make_struct_deserializable.hpp>

/**
 * MSERIALIZE_MAKE_DERIVED_STRUCT_DESERIALIZABLE(StructName, (Bases...), members...)
 *
 * Define a CustomDeserializer specialization for the given
 * struct or class, that has deserializable base classes,
 * allowing it to be deserialized using mserialize::deserialize,
 * without repeating the fields of its bases.
 *
 * The first argument of the macro must be the name
 * of the type to be made deserializable.
 * The second argument is a parenthesised list of deserializable base-classes
 * that `StructName` derives from.
 * Following the second argument come the members,
 * which are either accessible fields or setters.
 * For more on the allowed members, see: mserialize::deserializable_member.
 *
 * Example:
 *
 *     struct Person {
 *       int age;
 *       void name(std::string);
 *     };
 *     MSERIALIZE_MAKE_STRUCT_DESERIALIZABLE(Person, age, name)
 *
 *     struct Employee : Person {
 *       std::string eid;
 *     };
 *     MSERIALIZE_MAKE_DERIVED_STRUCT_DESERIALIZABLE(Employee, (Person), eid)
 *
 * The macro has to be called in global scope
 * (outside of any namespace).
 *
 * The list of base-classes must not be empty.
 * The member list can be empty.
 * The member list does not have to enumerate every member
 * of the given type: if a member is omitted, it will
 * be simply ignored during deserialization.
 *
 * The maximum number of members is limited by
 * mserialize/detail/foreach.hpp, currently 100.
 *
 * If a private member has to be deserialized, the following friend
 * declaration can be added to the type declaration:
 *
 *     template <typename, typename>
 *     friend struct mserialize::CustomDeserializer;
 */
#define MSERIALIZE_MAKE_DERIVED_STRUCT_DESERIALIZABLE(derived, ...) \
  namespace mserialize {                                            \
  template <>                                                       \
  struct CustomDeserializer<derived, void>                          \
  {                                                                 \
    using DerivedDeserializer = StructDeserializer<derived          \
      MSERIALIZE_FOREACH(                                           \
        MSERIALIZE_DESERIALIZABLE_MEMBER,                           \
        derived,                                                    \
        __VA_ARGS__                                                 \
      )                                                             \
    >;                                                              \
    template <typename InputStream>                                 \
    static void deserialize(derived& t, InputStream& istream)       \
    {                                                               \
      MSERIALIZE_FOREACH(                                           \
        MSERIALIZE_BASE_DESERIALIZE_EXPR, _, x,                     \
        MSERIALIZE_UNTUPLE(MSERIALIZE_FIRST(__VA_ARGS__))           \
      )                                                             \
      DerivedDeserializer::deserialize(t, istream);                 \
    }                                                               \
  };                                                                \
  } /* namespace mserialize */                                      \
  /**/

#define MSERIALIZE_BASE_DESERIALIZE_EXPR(_, b) mserialize::deserialize<b>(t, istream);

#endif // MSERIALIZE_MAKE_DERIVED_STRUCT_DESERIALIZABLE_HPP
