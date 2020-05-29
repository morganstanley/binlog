#ifndef MSERIALIZE_MAKE_STRUCT_TAG_HPP
#define MSERIALIZE_MAKE_STRUCT_TAG_HPP

#include <mserialize/cx_string.hpp>
#include <mserialize/detail/foreach.hpp>
#include <mserialize/detail/preprocessor.hpp>
#include <mserialize/tag.hpp>

/**
 * MSERIALIZE_MAKE_STRUCT_TAG(StructName, members...)
 *
 * Define a CustomTag specialization for the given
 * struct or class, allowing serialized objects
 * of it to be visited using mserialize::visit.
 *
 * The first argument of the macro must be the name
 * of the type for the tag is to be made.
 * Following the first argument come the members,
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
#define MSERIALIZE_MAKE_STRUCT_TAG(...)                           \
  namespace mserialize {                                          \
  template <>                                                     \
  struct CustomTag<MSERIALIZE_FIRST(__VA_ARGS__), void>           \
  {                                                               \
    using T = MSERIALIZE_FIRST(__VA_ARGS__);                      \
    static constexpr auto tag_string()                            \
    {                                                             \
      return cx_strcat(                                           \
        make_cx_string(                                           \
          "{" MSERIALIZE_STRINGIZE(MSERIALIZE_FIRST(__VA_ARGS__)) \
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

#define MSERIALIZE_STRUCT_MEMBER_TAG(_, m)                                  \
  make_cx_string("`" MSERIALIZE_STRINGIZE(m) "'"),                          \
  mserialize::tag<decltype(mserialize::serializable_member_type(&T::m))>(), \
  /**/

namespace mserialize {

/**
 * Resolve a (possibly overloaded) member pointer
 * to a type the serializable member gets serialized into.
 *
 * Usage: decltype(mserialize::serializable_member_type(&T::m))
 *
 * The requirements and limitations of mserialize::serializable_member
 * apply, see them there.
 */

template <typename T, typename Field>
auto serializable_member_type(Field T::*field) -> Field&;

template <typename T, typename Ret>
auto serializable_member_type(Ret (T::*getter)() const) -> Ret;

} // namespace mserialize

#endif // MSERIALIZE_MAKE_STRUCT_TAG_HPP
