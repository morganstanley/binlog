// The purpose of this suite is to test snippets which are included in the documentation automatically.
// This suite is not to test the features themselves, which are tested in other suites.

// Different snippets that are extracted automatically into the doc
// include the same headers, ignore them:
// NOLINTBEGIN(readability-duplicate-include)

#include <doctest/doctest.h>

#include <sstream>

//[serialize
#include <mserialize/serialize.hpp>
//]

//[deserialize
#include <mserialize/deserialize.hpp>
//]

//[adapt_custom_type
#include <mserialize/make_struct_deserializable.hpp>
#include <mserialize/make_struct_serializable.hpp>

// Given a custom type:
struct Alpha { int a = 0; std::string b; };

// Serialization and deserialization can be enabled by macros:
MSERIALIZE_MAKE_STRUCT_SERIALIZABLE(Alpha, a, b)
MSERIALIZE_MAKE_STRUCT_DESERIALIZABLE(Alpha, a, b)
//]

//[adapt_custom_templates
#include <mserialize/make_template_deserializable.hpp>
#include <mserialize/make_template_serializable.hpp>

template <typename A, typename B>
struct Pair { A a; B b; };

MSERIALIZE_MAKE_TEMPLATE_SERIALIZABLE((typename A, typename B), (Pair<A,B>), a, b)
MSERIALIZE_MAKE_TEMPLATE_DESERIALIZABLE((typename A, typename B), (Pair<A,B>), a, b)
//]

//[adapt_custom_type_with_getters_and_setters
// Given a custom type with getters and setters:
class Beta
{
  std::string c;
  float d;

public:
  const std::string& getC() const;
  void setC(std::string);

  float getD() const;
  void setD(float);
};

// Serialization and deserialization can be enabled the same way:
MSERIALIZE_MAKE_STRUCT_SERIALIZABLE(Beta, getC, getD)
MSERIALIZE_MAKE_STRUCT_DESERIALIZABLE(Beta, setC, setD)
//]

//[adapt_custom_type_with_private_members
class Gamma
{
  std::string e;  // private data member
  int f() const;  // private getter
  void f(int);    // private setter

  template <typename, typename>
  friend struct mserialize::CustomSerializer;

  template <typename, typename>
  friend struct mserialize::CustomDeserializer;
};
//]

//[adapt_custom_derived_type
#include <mserialize/make_derived_struct_deserializable.hpp>
#include <mserialize/make_derived_struct_serializable.hpp>

struct Zeta : Beta { int e = 0; };

MSERIALIZE_MAKE_DERIVED_STRUCT_SERIALIZABLE(Zeta, (Beta), e)
MSERIALIZE_MAKE_DERIVED_STRUCT_DESERIALIZABLE(Zeta, (Beta), e)
//]

//[visit_out
#include <mserialize/serialize.hpp>
#include <mserialize/tag.hpp>
//]

//[visit_in
#include <mserialize/deserialize.hpp>
#include <mserialize/visit.hpp>
//]

//[adapt_enum_for_visit
#include <mserialize/make_enum_tag.hpp>

enum Delta { a, b, c };
MSERIALIZE_MAKE_ENUM_TAG(Delta, a, b, c)
//]

//[adapt_custom_type_for_visit
#include <mserialize/make_struct_tag.hpp>

struct Epsilon { int a; std::string b; };
MSERIALIZE_MAKE_STRUCT_TAG(Epsilon, a, b)
//]

class Phi
{
  int i = 0;

  //[custom_tag_friend
  template <typename, typename>
  friend struct mserialize::CustomTag;
  //]
};

MSERIALIZE_MAKE_STRUCT_TAG(Phi, i)

MSERIALIZE_MAKE_STRUCT_TAG(Beta)
//[custom_tag_derived
#include <mserialize/make_derived_struct_tag.hpp>

MSERIALIZE_MAKE_DERIVED_STRUCT_TAG(Zeta, (Beta), e)
//]

//[adapt_custom_template_for_visit
#include <mserialize/make_template_tag.hpp>

template <typename A, typename B, typename C>
struct Triplet { A a; B b; C c; };

MSERIALIZE_MAKE_TEMPLATE_TAG((typename A, typename B, typename C), (Triplet<A,B,C>), a, b, c)
//]

//[recursive_tag
#include <mserialize/tag.hpp>

struct Node { int value; Node* next; };

namespace mserialize {

template <>
struct CustomTag<Node>
{
  static constexpr auto tag_string()
  {
    return make_cx_string("{Node`value'i`next'<0{Node}>}");
  }
};

} // namespace mserialize
//]

#include <cassert>
#include <cstdio> // remove
#include <fstream>
#include <string>

struct Visitor
{
  template <typename T> void visit(T) {}
  template <typename T> bool visit(T, std::ifstream&) { return false; }
};

TEST_CASE("roundtrip")
{
  using T = std::string;

  const std::string path = "mserialize_test_documentation_roundtrip.data";

  {
    //[serialize

    const T my_value;
    std::ofstream ostream(path);
    mserialize::serialize(my_value, ostream);
    //]
  }

  {
    //[deserialize

    T my_value;
    std::ifstream istream(path);
    istream.exceptions(std::ios_base::failbit);
    mserialize::deserialize(my_value, istream);
    //]
  }

  (void)std::remove(path.data());
  CHECK(true);
}

TEST_CASE("adapt_custom_type")
{
  //[adapt_custom_type

  // At this point, objects of `Alpha` can be used
  // together with mserialize::serialize and deserialize,
  // the same way as by-default supported objects.
  const Alpha in{30, "foo"};
  std::stringstream stream;
  mserialize::serialize(in, stream);

  Alpha out;
  stream.exceptions(std::ios_base::failbit);
  mserialize::deserialize(out, stream);

  assert(in.a == out.a && in.b == out.b);
  //]
  CHECK(true);
}

TEST_CASE("visit")
{
  using T = std::string;

  const std::string path = "mserialize_test_documentation_visit.data";

  {
    //[visit_out

    // serialize a T object
    const T t;
    const auto tag = mserialize::tag<T>();
    std::ofstream ostream(path);
    mserialize::serialize(tag, ostream);
    mserialize::serialize(t, ostream);
    //]
  }

  {
    //[visit_in

    // visit the object
    std::ifstream istream(path);
    istream.exceptions(std::ios_base::failbit);
    std::string tag;
    mserialize::deserialize(tag, istream);
    Visitor visitor;
    mserialize::visit(tag, visitor, istream);
    //]
  }

  (void)std::remove(path.data());
  CHECK(true);
}

// NOLINTEND(readability-duplicate-include)
