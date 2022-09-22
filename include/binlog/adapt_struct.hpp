#ifndef BINLOG_ADAPT_STRUCT_HPP
#define BINLOG_ADAPT_STRUCT_HPP

#include <mserialize/make_derived_struct_serializable.hpp>
#include <mserialize/make_derived_struct_tag.hpp>
#include <mserialize/make_struct_serializable.hpp>
#include <mserialize/make_struct_tag.hpp>
#include <mserialize/make_template_serializable.hpp>
#include <mserialize/make_template_tag.hpp>

/**
 * BINLOG_ADAPT_STRUCT(Struct, members...)
 *
 * Make `Struct` loggable.
 *
 * Example:
 *
 *     struct Person {
 *       int age;
 *       std::string name() const;
 *     };
 *     BINLOG_ADAPT_STRUCT(Person, age, name)
 *
 * The macro has to be called in global scope (outside of any namespace).
 * `Struct` is a fully qualified typename. `Struct` must not be recursive.
 * members... is a list of data members or getters of Struct, i.e:
 * each member is either:
 *
 *  - a non-static, non-reference, non-bitfield data member, or
 *  - a const qualified, nullary member function.
 *    (also see Limitation below)
 *
 * The member list can be empty.
 * The member list does not have to enumerate every member
 * of `Struct`: if a member is omitted, it will not be logged.
 * The maximum number of members is limited to 100.
 * If the member list references a private or protected
 * member, the following (or equivalent) friend declaration must be added:
 *
 *    BINLOG_ADAPT_STRUCT_FRIEND;
 *
 * Limitation: member functions specified in the member list cannot be overloaded
 * with a member function template, which has its function arguments defaulted.
 *
 * @see MSERIALIZE_MAKE_STRUCT_TAG
 * @see MSERIALIZE_MAKE_STRUCT_SERIALIZABLE
 */
#define BINLOG_ADAPT_STRUCT(...)                   \
  MSERIALIZE_MAKE_STRUCT_TAG(__VA_ARGS__)          \
  MSERIALIZE_MAKE_STRUCT_SERIALIZABLE(__VA_ARGS__) \
  /**/

/**
 * BINLOG_ADAPT_DERIVED(Derived, (Bases...), members...)
 *
 * Make `Derived` loggable, without repeating the
 * members of bases.
 *
 * Example:
 *
 *     struct Derived : Base { int x; }
 *     BINLOG_ADAPT_DERIVED(Derived, (Base), x)
 *
 * Example for multiple inheritance:
 *
 *     struct Derived : Base1, Base2 { int x; }
 *     BINLOG_ADAPT_DERIVED(Derived, (Base1, Base2), x)
 *
 * `Derived` must be publicly derived from Bases....
 * Bases... must be loggable types.
 *
 * @see BINLOG_ADAPT_STRUCT on members... and restrictions.
 */
#define BINLOG_ADAPT_DERIVED(...)                                             \
  MSERIALIZE_EXPAND(MSERIALIZE_MAKE_DERIVED_STRUCT_TAG(__VA_ARGS__))          \
  MSERIALIZE_EXPAND(MSERIALIZE_MAKE_DERIVED_STRUCT_SERIALIZABLE(__VA_ARGS__)) \

/**
 * BINLOG_ADAPT_TEMPLATE(TemplateArgs, TypenameWithTemplateArgs, members...)
 *
 * Make every instance of the given template loggable.
 *
 * Example:
 *
 *     template <typename A, typename B>
 *     struct Pair {
 *       A a;
 *       B b;
 *     };
 *     BINLOG_ADAPT_TEMPLATE((typename A, typename B), (Pair<A,B>), a, b)
 *
 * The macro has to be called in global scope (outside of any namespace).
 *
 * The first argument of the macro is the arguments
 * of the template, with the necessary typename prefix,
 * where needed, as they appear after the template keyword
 * in the definition, wrapped by parentheses.
 *
 * The second argument is the template name with
 * the template arguments, as it should appear in a specialization,
 * wrapped by parentheses.
 *
 * Following the second argument come the members: data members or getters,
 * as specified by BINLOG_ADAPT_STRUCT.
 *
 * If the member list references a private or protected
 * member, the following (or equivalent) friend declaration must be added:
 *
 *    BINLOG_ADAPT_STRUCT_FRIEND;
 *
 * @see BINLOG_ADAPT_STRUCT
 * @see MSERIALIZE_MAKE_TEMPLATE_TAG
 * @see MSERIALIZE_MAKE_TEMPLATE_SERIALIZABLE
 */
#define BINLOG_ADAPT_TEMPLATE(...)                                      \
  MSERIALIZE_EXPAND(MSERIALIZE_MAKE_TEMPLATE_TAG(__VA_ARGS__))          \
  MSERIALIZE_EXPAND(MSERIALIZE_MAKE_TEMPLATE_SERIALIZABLE(__VA_ARGS__)) \
  /**/

#if __cplusplus >= 202002L // Otherwise __VA_OPT__ breaks
/**
 * BINLOG_ADAPT_CONCEPT(Concept, members...)
 *
 * Make types that model `Concept` loggable, in terms of the specified members.
 *
 * Example:
 *
 *     template<typename T>
 *     concept Stringable = requires(T a)
 *     {
 *       { a.str() } -> std::convertible_to<std::string>;
 *     };
 *
 *     BINLOG_ADAPT_CONCEPT(Stringable, str)
 *
 * The macro has to be called in global scope (outside of any namespace).
 * `Concept` is a Named Requirement - see C++20 concepts.
 *
 * members... is a list of data members or getters `Concept` requires.
 * members... can be empty.
 * See BINLOG_ADAPT_STRUCT for limitations.
 *
 * Calling this macro requires C++20 or greater.
 *
 * If type Foo models concept Bar, and Bar is adapted using this macro,
 * when a Foo object is logged (that is otherwise not adapted),
 * on the output, the type name will be "Bar", not "Foo".
 *
 * @see BINLOG_ADAPT_STRUCT
 */
#define BINLOG_ADAPT_CONCEPT(Concept, ...)                                                                         \
  MSERIALIZE_EXPAND(MSERIALIZE_MAKE_TEMPLATE_TAG((Concept Concept), (Concept) __VA_OPT__(,) __VA_ARGS__))          \
  MSERIALIZE_EXPAND(MSERIALIZE_MAKE_TEMPLATE_SERIALIZABLE((Concept Concept), (Concept) __VA_OPT__(,) __VA_ARGS__)) \
  /**/
#endif // >= C++20

/**
 * Allow member list of BINLOG_ADAPT_STRUCT or BINLOG_ADAPT_TEMPLATE
 * to reference private and protected members.
 *
 * Example:
 *
 *     class Private {
 *       int a = 0;
 *       BINLOG_ADAPT_STRUCT_FRIEND;
 *     };
 *     BINLOG_ADAPT_STRUCT(Private, a)
 */
#define BINLOG_ADAPT_STRUCT_FRIEND                                         \
  template <typename, typename> friend struct mserialize::CustomTag;       \
  template <typename, typename> friend struct mserialize::CustomSerializer \
  /**/

#endif // BINLOG_ADAPT_STRUCT_HPP
