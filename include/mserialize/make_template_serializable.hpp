#ifndef MSERIALIZE_MAKE_TEMPLATE_SERIALIZABLE_HPP
#define MSERIALIZE_MAKE_TEMPLATE_SERIALIZABLE_HPP

#include <mserialize/StructSerializer.hpp>
#include <mserialize/detail/Serializer.hpp>
#include <mserialize/detail/foreach.hpp>
#include <mserialize/detail/preprocessor.hpp>

#include <type_traits> // integral_constant

/**
 * MSERIALIZE_MAKE_TEMPLATE_SERIALIZABLE(TemplateArgs, TypenameWithTempalteArgs, members...)
 *
 * Define a CustomSerializer specialization for the given
 * struct or class template, allowing its instantiations
 * to be serialized using mserialize::serialize.
 *
 * The first argument of the macro must be the arguments
 * of the template, with the necessary typename prefix,
 * where needed, as they appear after the template keyword
 * in the definition, wrapped by parentheses.
 * (The parentheses are required to avoid the preprocessor
 * splitting the arguments at the commas)
 *
 * The second argument is the template name with
 * the template arguments, as it should appear in a specialization,
 * wrapped by parentheses.
 *
 * Following the second argument come the members,
 * which are either accessible fields or getters.
 * For more on the allowed members, see: mserialize::serializable_member.
 *
 * Example:
 *
 *     template <typename A, typename B>
 *     struct Pair {
 *       A a;
 *       B b;
 *     };
 *     MSERIALIZE_MAKE_TEMPLATE_SERIALIZABLE((typename A, typename B), (Pair<A,B>), a, b)
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
#define MSERIALIZE_MAKE_TEMPLATE_SERIALIZABLE(TemplateArgs, ...)        \
  namespace mserialize {                                                \
  template <MSERIALIZE_UNTUPLE(TemplateArgs)>                           \
  struct CustomSerializer<MSERIALIZE_UNTUPLE(MSERIALIZE_FIRST(__VA_ARGS__)), void> \
    :StructSerializer<MSERIALIZE_UNTUPLE(MSERIALIZE_FIRST(__VA_ARGS__)) \
      MSERIALIZE_FOREACH(                                               \
        MSERIALIZE_SERIALIZABLE_TEMPLATE_MEMBER,                        \
        MSERIALIZE_FIRST(__VA_ARGS__),                                  \
        __VA_ARGS__                                                     \
      )                                                                 \
     >                                                                  \
  {};                                                                   \
  } /* namespace mserialize */                                          \
  /**/

#define MSERIALIZE_SERIALIZABLE_TEMPLATE_MEMBER(T,m)                                                          \
  ,std::integral_constant<decltype(serializable_member(&MSERIALIZE_UNTUPLE(T)::m)),&MSERIALIZE_UNTUPLE(T)::m> \
  /**/

#endif // MSERIALIZE_MAKE_TEMPLATE_SERIALIZABLE_HPP
