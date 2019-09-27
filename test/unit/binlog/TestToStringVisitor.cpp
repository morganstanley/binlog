#include <binlog/ToStringVisitor.hpp>

#include <mserialize/Visitor.hpp>

#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <sstream>

namespace {

using not_bool_arithmetic_types = boost::mpl::list<
  std::int8_t, std::int16_t, std::int32_t, std::int64_t,
  std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t,
  float, double, long double
>;

struct TestcaseBase
{
  using V = mserialize::Visitor;

  std::ostringstream str;
  binlog::ToStringVisitor visitor{str};

  std::string result() const { return str.str(); }
};

} // namespace

BOOST_AUTO_TEST_SUITE(ToStringVisitor)

BOOST_FIXTURE_TEST_CASE(empty, TestcaseBase)
{
  BOOST_TEST(result() == "");
}

BOOST_FIXTURE_TEST_CASE(bool_true, TestcaseBase)
{
  visitor.visit(true);
  BOOST_TEST(result() == "true");
}

BOOST_FIXTURE_TEST_CASE(bool_false, TestcaseBase)
{
  visitor.visit(false);
  BOOST_TEST(result() == "false");
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(arithmetic, T, not_bool_arithmetic_types, TestcaseBase)
{
  const T t{123};
  visitor.visit(t);
  BOOST_TEST(result() == "123");
}

BOOST_FIXTURE_TEST_CASE(empty_sequence_of_int, TestcaseBase)
{
  visitor.visit(V::SequenceBegin{0, "i"});
  visitor.visit(V::SequenceEnd{});
  BOOST_TEST(result() == "[]");
}

BOOST_FIXTURE_TEST_CASE(sequence_of_int, TestcaseBase)
{
  visitor.visit(V::SequenceBegin{3, "i"});
  visitor.visit(int(1));
  visitor.visit(int(2));
  visitor.visit(int(3));
  visitor.visit(V::SequenceEnd{});

  BOOST_TEST(result() == "[1, 2, 3]");
}

BOOST_FIXTURE_TEST_CASE(sequence_of_char, TestcaseBase)
{
  visitor.visit(V::SequenceBegin{3, "c"});
  visitor.visit('a');
  visitor.visit('b');
  visitor.visit('c');
  visitor.visit(V::SequenceEnd{});

  BOOST_TEST(result() == "abc");
}

BOOST_FIXTURE_TEST_CASE(seq_of_seq_of_int, TestcaseBase)
{
  visitor.visit(V::SequenceBegin{3, "[i"});
  visitor.visit(V::SequenceBegin{2, "i"});
  visitor.visit(int(1));
  visitor.visit(int(2));
  visitor.visit(V::SequenceEnd{});
  visitor.visit(V::SequenceBegin{2, "i"});
  visitor.visit(int(3));
  visitor.visit(int(4));
  visitor.visit(V::SequenceEnd{});
  visitor.visit(V::SequenceBegin{2, "i"});
  visitor.visit(int(5));
  visitor.visit(int(6));
  visitor.visit(V::SequenceEnd{});
  visitor.visit(V::SequenceEnd{});

  BOOST_TEST(result() == "[[1, 2], [3, 4], [5, 6]]");
}

BOOST_FIXTURE_TEST_CASE(seq_of_seq_of_char, TestcaseBase)
{
  visitor.visit(V::SequenceBegin{3, "[c"});
  visitor.visit(V::SequenceBegin{2, "c"});
  visitor.visit('a');
  visitor.visit('b');
  visitor.visit(V::SequenceEnd{});
  visitor.visit(V::SequenceBegin{2, "c"});
  visitor.visit('c');
  visitor.visit('d');
  visitor.visit(V::SequenceEnd{});
  visitor.visit(V::SequenceBegin{2, "c"});
  visitor.visit('e');
  visitor.visit('f');
  visitor.visit(V::SequenceEnd{});
  visitor.visit(V::SequenceEnd{});

  BOOST_TEST(result() == "[ab, cd, ef]");
}

BOOST_FIXTURE_TEST_CASE(empty_tuple, TestcaseBase)
{
  visitor.visit(V::TupleBegin{""});
  visitor.visit(V::TupleEnd{});
  BOOST_TEST(result() == "()");
}

BOOST_FIXTURE_TEST_CASE(tuple_of_int_bool_char, TestcaseBase)
{
  visitor.visit(V::TupleBegin{"iyc"});
  visitor.visit(int(1));
  visitor.visit(true);
  visitor.visit('a');
  visitor.visit(V::TupleEnd{});

  BOOST_TEST(result() == "(1, true, a)");
}

BOOST_FIXTURE_TEST_CASE(seq_of_variant, TestcaseBase)
{
  visitor.visit(V::SequenceBegin{3, "<0i>"});
  visitor.visit(V::VariantBegin{1, "i"});
  visitor.visit(V::Null{});
  visitor.visit(V::VariantEnd{});
  visitor.visit(V::VariantBegin{0, "0"});
  visitor.visit(int(1));
  visitor.visit(V::VariantEnd{});
  visitor.visit(V::VariantBegin{1, "i"});
  visitor.visit(int(2));
  visitor.visit(V::VariantEnd{});
  visitor.visit(V::SequenceEnd{});

  BOOST_TEST(result() == "[{null}, 1, 2]");
}

BOOST_FIXTURE_TEST_CASE(seq_of_enum, TestcaseBase)
{
  visitor.visit(V::SequenceBegin{3, "/i`E'0`a'1`b'\\"});
  visitor.visit(V::Enum{"E", "b", 'i', "1"});
  visitor.visit(V::Enum{"E", "a", 'i', "2"});
  visitor.visit(V::Enum{"E", "", 'i', "3"});
  visitor.visit(V::SequenceEnd{});

  BOOST_TEST(result() == "[b, a, 3]");
}

BOOST_FIXTURE_TEST_CASE(empty_struct, TestcaseBase)
{
  visitor.visit(V::StructBegin{"Empty", ""});
  visitor.visit(V::StructEnd{});
  BOOST_TEST(result() == "Empty");
}

BOOST_FIXTURE_TEST_CASE(simple_struct, TestcaseBase)
{
  visitor.visit(V::StructBegin{"Alpha", "`a'i`b'y"});
  visitor.visit(V::FieldBegin{"a", "i"});
  visitor.visit(int(1));
  visitor.visit(V::FieldEnd{});
  visitor.visit(V::FieldBegin{"b", "y"});
  visitor.visit(false);
  visitor.visit(V::FieldEnd{});
  visitor.visit(V::StructEnd{});

  BOOST_TEST(result() == "Alpha{ a: 1, b: false }");
}

BOOST_FIXTURE_TEST_CASE(pair_of_structs, TestcaseBase)
{
  visitor.visit(V::TupleBegin{"{Alpha`a'i}{Empty}{Beta`b'y}"});
  visitor.visit(V::StructBegin{"Alpha", "`a'i"});
  visitor.visit(V::FieldBegin{"a", "i"});
  visitor.visit(int(1));
  visitor.visit(V::FieldEnd{});
  visitor.visit(V::StructEnd{});
  visitor.visit(V::StructBegin{"Empty", ""});
  visitor.visit(V::StructEnd{});
  visitor.visit(V::StructBegin{"Beta", "`b'y"});
  visitor.visit(V::FieldBegin{"b", "y"});
  visitor.visit(true);
  visitor.visit(V::FieldEnd{});
  visitor.visit(V::StructEnd{});
  visitor.visit(V::TupleEnd{});

  BOOST_TEST(result() == "(Alpha{ a: 1 }, Empty, Beta{ b: true })");
}

BOOST_FIXTURE_TEST_CASE(struct_of_structs, TestcaseBase)
{
  visitor.visit(V::StructBegin{"Alpha", "`ccc'{Beta`d'i}`e'{Empty}"});
  visitor.visit(V::FieldBegin{"ccc", "{Beta`d'i}"});
  visitor.visit(V::StructBegin{"Beta", "`d'i"});
  visitor.visit(V::FieldBegin{"d", "i"});
  visitor.visit(int(1));
  visitor.visit(V::FieldEnd{});
  visitor.visit(V::StructEnd{});
  visitor.visit(V::FieldEnd{});
  visitor.visit(V::FieldBegin{"e", "{Empty}"});
  visitor.visit(V::StructBegin{"Empty", ""});
  visitor.visit(V::StructEnd{});
  visitor.visit(V::FieldEnd{});
  visitor.visit(V::StructEnd{});

  BOOST_TEST(result() == "Alpha{ ccc: Beta{ d: 1 }, e: Empty }");
}

BOOST_FIXTURE_TEST_CASE(empty_field_name, TestcaseBase)
{
  visitor.visit(V::StructBegin{"BoundedInt", "`'i"});
  visitor.visit(V::FieldBegin{"", "i"});
  visitor.visit(int(1024));
  visitor.visit(V::FieldEnd{});
  visitor.visit(V::StructEnd{});

  BOOST_TEST(result() == "BoundedInt{ 1024 }");
}

BOOST_AUTO_TEST_SUITE_END()
