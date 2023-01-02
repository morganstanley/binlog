#ifndef MSERIALIZE_MAKE_TEMPLATE_TAG_HPP
#define MSERIALIZE_MAKE_TEMPLATE_TAG_HPP

#include <mserialize/cx_string.hpp>
#include <mserialize/detail/foreach.hpp>
#include <mserialize/detail/preprocessor.hpp>
#include <mserialize/make_struct_tag.hpp>

/**
 * MSERIALIZE_MAKE_TEMPLATE_TAG(TemplateArgs, TypenameWithTemplateArgs, members...)
 *
 * Define a CustomTag specialization for the given
 * struct or class template, allowing serialized objects
 * of it to be visited using mserialize::visit.
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
 *
 * Example:
 *
 *     template <typename A, typename B>
 *     struct Pair {
 *       A a;
 *       B b;
 *     };
 *     MSERIALIZE_MAKE_TEMPLATE_TAG((typename A, typename B), (Pair<A,B>), a, b)
 *
 * The macro has to be called in global scope
 * (outside of any namespace).
 *
 * The member list can be empty.
 * The member list does not have to enumerate every member
 * of the given type, but it must be sync with
 * the specialization of CustomSerializer,
 * if visitation of objects serialized that way
 * is desired.
 *
 * The maximum number of members and template arguments
 * is limited by mserialize/detail/foreach.hpp, currently 100.
 *
 * Limitation: it is not possible to generate
 * a tag for a recursive types with this macro.
 * CustomTag for recursive types need to be manually
 * specialized.
 *
 * If some of the members are private, the following friend
 * declaration can be added to the type declaration:
 *
 *     template <typename, typename>
 *     friend struct mserialize::CustomTag;
 */
#define MSERIALIZE_MAKE_TEMPLATE_TAG(TemplateArgs, ...)                                    \
  MSERIALIZE_MAKE_TEMPLATE_TAG_I(TemplateArgs, MSERIALIZE_FIRST(__VA_ARGS__), __VA_ARGS__) \
  /**/

// The macros below deserve a bit of an explanation.
//
// Given the following example call:
//
// MSERIALIZE_MAKE_TEMPLATE_TAG_I((typename A, typename B, typename C), (Tuple<A,B,C>), (Tuple<A,B,C>), a, b, c)
//
// Then the below macros expand to:
//
// TemplateArgs = (typename A, typename B, typename C)
// Specname = (Tuple<A,B,C>)
// __VA_ARGS__ = (Tuple<A,B,C>), a, b, c
//               ^-- this is here to avoid empty pack if there are no members,
//                   as until C++20 the pack cannot be empty
//
// MSERIALIZE_FIRST(__VA_ARGS__) = (Tuple<A,B,C>)
// MSERIALIZE_UNTUPLE((Tuple<A,B,C>)) = Tuple<A,B,C>
// MSERIALIZE_STRINGIZE_LIST(Tuple<A,B,C>) ~= "Tuple<A,B,C>"
//
// Specname is useful for concepts, where it is distinct from the typename.
//
// tag_guard: to make has_tag report correctly if T<A> has a tag or not,
// (that depends on the template parameter), we first check if every member
// has a tag (see conjunction), then either create a constructible member
// (true_type, if every member has a tag), or a not-constructible member,
// (BuiltinTag), making this spec also non-constructible - has_tag
// checks constructibility.
#define MSERIALIZE_MAKE_TEMPLATE_TAG_I(TemplateArgs, Specname, ...)           \
  namespace mserialize {                                          \
  template <MSERIALIZE_UNTUPLE(TemplateArgs)>                     \
  struct CustomTag<MSERIALIZE_UNTUPLE(Specname), void>            \
  {                                                               \
    using T = MSERIALIZE_UNTUPLE(Specname);                       \
    std::conditional_t<                                           \
      mserialize::detail::conjunction<                            \
        MSERIALIZE_FOREACH(MSERIALIZE_HAS_TAG, _, __VA_ARGS__)    \
      std::true_type>::value,                                     \
      std::true_type,                                             \
      mserialize::detail::BuiltinTag<void>                        \
    > tag_guard;                                                  \
    static constexpr auto tag_string()                            \
    {                                                             \
      return cx_strcat(                                           \
        make_cx_string(                                           \
          "{" MSERIALIZE_STRINGIZE_LIST(MSERIALIZE_UNTUPLE(MSERIALIZE_FIRST(__VA_ARGS__))) \
        ),                                                        \
        MSERIALIZE_FOREACH(                                       \
          MSERIALIZE_STRUCT_MEMBER_TAG, _, __VA_ARGS__            \
        )                                                         \
        make_cx_string("}")                                       \
      );                                                          \
    }                                                             \
  };                                                              \
  } /* namespace mserialize */                                    \
  /**/

/** MSERIALIZE_STRINGIZE_LIST(a,b,c) -> "a" "," "b" "," "c" */
#define MSERIALIZE_STRINGIZE_LIST(...)                            \
  MSERIALIZE_STRINGIZE(MSERIALIZE_FIRST(__VA_ARGS__))             \
  MSERIALIZE_FOREACH(MSERIALIZE_STRINGIZE_LIST_I, _, __VA_ARGS__) \
  /**/

#define MSERIALIZE_STRINGIZE_LIST_I(_, a) "," #a

#define MSERIALIZE_HAS_TAG(_, m)                                                      \
  mserialize::detail::has_tag<decltype(mserialize::serializable_member_type(&T::m))>, \
  /**/

#endif // MSERIALIZE_MAKE_TEMPLATE_TAG_HPP
