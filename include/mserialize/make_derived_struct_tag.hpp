#ifndef MSERIALIZE_MAKE_DERIVED_STRUCT_TAG_HPP
#define MSERIALIZE_MAKE_DERIVED_STRUCT_TAG_HPP

#include <mserialize/cx_string.hpp>
#include <mserialize/detail/foreach.hpp>
#include <mserialize/detail/preprocessor.hpp>
#include <mserialize/tag.hpp>

/**
 * MSERIALIZE_MAKE_DERIVED_STRUCT_TAG(StructName, (Bases...), members...)
 *
 * Define a CustomTag specialization for the given
 * struct or class, that has tagged base classes,
 * allowing serialized objects of it to be visited
 * using mserialize::visit without repeating the fields of its bases.
 *
 * The first argument of the macro must be the name
 * of the type for the tag is to be made.
 * The second argument is a parenthesised list of tagged base-classes
 * that `StructName` derives from.
 * Following the second argument come the members,
 * which are either accessible fields or getters.
 *
 * Example:
 *
 *     struct Person {
 *       int age;
 *       std::string name() const;
 *     };
 *     MSERIALIZE_MAKE_STRUCT_TAG(Person, age, name)
 *
 *     struct Employee : Person {
 *       std::string eid;
 *     };
 *     MSERIALIZE_MAKE_DERIVED_STRUCT_TAG(Employee, (Person), eid)
 *
 * The macro has to be called in global scope
 * (outside of any namespace).
 *
 * The list of base-classes must not be empty.
 * The member list can be empty.
 * The member list does not have to enumerate every member
 * of the given type, but it must be sync with
 * the specialization of CustomSerializer,
 * if visitation of objects serialized that way
 * is desired.
 *
 * The maximum number of members is limited by
 * mserialize/detail/foreach.hpp, currently 100.
 *
 * Limitation: it is not possible to generate
 * a tag for a recursive type with this macro.
 * CustomTag for recursive types need to be manually
 * specialized.
 *
 * If some of the members are private, the following friend
 * declaration can be added to the type declaration:
 *
 *     template <typename, typename>
 *     friend struct mserialize::CustomTag;
 */
#define MSERIALIZE_MAKE_DERIVED_STRUCT_TAG(derived, ...)       \
  namespace mserialize {                                       \
  template <>                                                  \
  struct CustomTag<derived>                                    \
  {                                                            \
    using T = derived;                                         \
    static constexpr auto tag_string()                         \
    {                                                          \
      return cx_strcat(                                        \
        make_cx_string(                                        \
          "{" MSERIALIZE_STRINGIZE(derived)                    \
        ),                                                     \
        MSERIALIZE_FOREACH(                                    \
          MSERIALIZE_BASE_TAG_EXPR, _, x,                      \
          MSERIALIZE_UNTUPLE(MSERIALIZE_FIRST(__VA_ARGS__))    \
        )                                                      \
        MSERIALIZE_FOREACH(                                    \
          MSERIALIZE_STRUCT_MEMBER_TAG, _, __VA_ARGS__         \
        )                                                      \
        make_cx_string("}")                                    \
      );                                                       \
    }                                                          \
  };                                                           \
  } /* namespace mserialize */                                 \
  /**/

#define MSERIALIZE_BASE_TAG_EXPR(_, t) make_cx_string("`'"), mserialize::tag<t>(),

#endif // MSERIALIZE_MAKE_DERIVED_STRUCT_TAG_HPP
