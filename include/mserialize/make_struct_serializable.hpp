#ifndef MSERIALIZE_MAKE_STRUCT_SERIALIZABLE_HPP
#define MSERIALIZE_MAKE_STRUCT_SERIALIZABLE_HPP

#include <mserialize/StructSerializer.hpp>
#include <mserialize/detail/Serializer.hpp>
#include <mserialize/detail/foreach.hpp>
#include <mserialize/detail/preprocessor.hpp>

#include <type_traits> // integral_constant

/**
 * MSERIALIZE_MAKE_STRUCT_SERIALIZABLE(StructName, members...)
 *
 * Define a CustomSerializer specialization for the given
 * struct or class, allowing it to be serialized
 * using mserialize::serialize.
 *
 * The first argument of the macro must be the name
 * of the type to be made serializable.
 * Following the first argument come the members,
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
 * The macro has to be called in global scope
 * (outside of any namespace).
 *
 * The member list can be empty.
 * The member list does not have to enumerate every member
 * of the given type: if a member is omitted, it will
 * be simply ignored during serialization.
 *
 * The maximum number of enumerators is limited by
 * mserialize/detail/foreach.hpp, currently 100.
 *
 * If a private member has to be serialized, the following friend
 * declaration can be added to the type declaration:
 *
 *     template <typename, typename>
 *     friend struct mserialize::CustomSerializer;
 */
#define MSERIALIZE_MAKE_STRUCT_SERIALIZABLE(...)                      \
  namespace mserialize {                                              \
  template <>                                                         \
  struct CustomSerializer<MSERIALIZE_FIRST(__VA_ARGS__), void>        \
    :StructSerializer<MSERIALIZE_FIRST(__VA_ARGS__)                   \
      MSERIALIZE_FOREACH(                                             \
        MSERIALIZE_SERIALIZABLE_MEMBER,                               \
        MSERIALIZE_FIRST(__VA_ARGS__),                                \
        __VA_ARGS__                                                   \
      )                                                               \
     >                                                                \
  {};                                                                 \
  } /* namespace mserialize */                                        \
  /**/

#define MSERIALIZE_SERIALIZABLE_MEMBER(T,m)                           \
  ,std::integral_constant<decltype(serializable_member(&T::m)),&T::m> \
  /**/

#endif // MSERIALIZE_MAKE_STRUCT_SERIALIZABLE_HPP
