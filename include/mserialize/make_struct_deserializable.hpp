#ifndef MSERIALIZE_MAKE_STRUCT_DESERIALIZABLE_HPP
#define MSERIALIZE_MAKE_STRUCT_DESERIALIZABLE_HPP

#include <mserialize/StructDeserializer.hpp>
#include <mserialize/detail/Deserializer.hpp>
#include <mserialize/detail/foreach.hpp>
#include <mserialize/detail/preprocessor.hpp>

#include <type_traits> // integral_constant

/**
 * MSERIALIZE_MAKE_STRUCT_DESERIALIZABLE(StructName, members...)
 *
 * Define a CustomDesrializer specialization for the given
 * struct or class, allowing it to be deserialized
 * using mserialize::deserialize.
 *
 * The first argument of the macro must be the name
 * of the type to be made deserializable.
 * Following the first argument come the members,
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
 * The macro has to be called in global scope
 * (outside of any namespace).
 *
 * The member list can be empty.
 * The member list does not have to enumerate every member
 * of the given type: if a member is omitted, it will
 * be simply ignored during deserialization.
 *
 * The maximum number of enumerators is limited by
 * mserialize/detail/foreach.hpp, currently 100.
 *
 * If a private member has to be deserialized, the following friend
 * declaration can be added to the type declaration:
 *
 *     template <typename, typename>
 *     friend struct mserialize::CustomDeserializer;
 */
#define MSERIALIZE_MAKE_STRUCT_DESERIALIZABLE(...)                      \
  namespace mserialize {                                                \
  template <>                                                           \
  struct CustomDeserializer<MSERIALIZE_FIRST(__VA_ARGS__), void>        \
    :StructDeserializer<MSERIALIZE_FIRST(__VA_ARGS__)                   \
      MSERIALIZE_FOREACH(                                               \
        MSERIALIZE_DESERIALIZABLE_MEMBER,                               \
        MSERIALIZE_FIRST(__VA_ARGS__),                                  \
        __VA_ARGS__                                                     \
      )                                                                 \
     >                                                                  \
  {};                                                                   \
  } /* namespace mserialize */                                          \
  /**/

#define MSERIALIZE_DESERIALIZABLE_MEMBER(T,m)                           \
  ,std::integral_constant<decltype(deserializable_member(&T::m)),&T::m> \
  /**/

#endif // MSERIALIZE_MAKE_STRUCT_DESERIALIZABLE_HPP
