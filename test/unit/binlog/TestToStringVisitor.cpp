#include <binlog/ToStringVisitor.hpp>

#include <binlog/detail/OstreamBuffer.hpp>

#include <mserialize/Visitor.hpp>

#include <doctest/doctest.h>

#include <cstdint>
#include <sstream>

namespace {

#define NOT_BOOL_ARITHMETIC_TYPES                            \
  std::int8_t, std::int16_t, std::int32_t, std::int64_t,     \
  std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t, \
  float, double, long double                                 \
  /**/

struct TestcaseBase
{
  using V = mserialize::Visitor;

  binlog::Range input;
  std::ostringstream str;
  binlog::detail::OstreamBuffer buf{str};
  binlog::ToStringVisitor visitor{buf};

  std::string result()
  {
    buf.flush();
    return str.str();
  }
};

} // namespace

TEST_CASE_FIXTURE(TestcaseBase, "empty")
{
  CHECK(result() == "");
}

TEST_CASE_FIXTURE(TestcaseBase, "bool_true")
{
  visitor.visit(true);
  CHECK(result() == "true");
}

TEST_CASE_FIXTURE(TestcaseBase, "bool_false")
{
  visitor.visit(false);
  CHECK(result() == "false");
}

TEST_CASE_TEMPLATE("arithmetic", T, NOT_BOOL_ARITHMETIC_TYPES)
{
  std::ostringstream str;
  binlog::detail::OstreamBuffer buf{str};
  binlog::ToStringVisitor visitor{buf};

  const T t{123};
  visitor.visit(t);

  buf.flush();
  CHECK(str.str() == "123");
}

TEST_CASE_FIXTURE(TestcaseBase, "empty_sequence_of_int")
{
  visitor.visit(V::SequenceBegin{0, "i"}, input);
  visitor.visit(V::SequenceEnd{});
  CHECK(result() == "[]");
}

TEST_CASE_FIXTURE(TestcaseBase, "sequence_of_int")
{
  visitor.visit(V::SequenceBegin{3, "i"}, input);
  visitor.visit(int(1));
  visitor.visit(int(2));
  visitor.visit(int(3));
  visitor.visit(V::SequenceEnd{});

  CHECK(result() == "[1, 2, 3]");
}

TEST_CASE_FIXTURE(TestcaseBase, "sequence_of_char")
{
  input = binlog::Range("abc", 3);
  CHECK(visitor.visit(V::SequenceBegin{3, "c"}, input));
  CHECK(result() == "abc");
}

TEST_CASE_FIXTURE(TestcaseBase, "seq_of_seq_of_int")
{
  visitor.visit(V::SequenceBegin{3, "[i"}, input);
  visitor.visit(V::SequenceBegin{2, "i"}, input);
  visitor.visit(int(1));
  visitor.visit(int(2));
  visitor.visit(V::SequenceEnd{});
  visitor.visit(V::SequenceBegin{2, "i"}, input);
  visitor.visit(int(3));
  visitor.visit(int(4));
  visitor.visit(V::SequenceEnd{});
  visitor.visit(V::SequenceBegin{2, "i"}, input);
  visitor.visit(int(5));
  visitor.visit(int(6));
  visitor.visit(V::SequenceEnd{});
  visitor.visit(V::SequenceEnd{});

  CHECK(result() == "[[1, 2], [3, 4], [5, 6]]");
}

TEST_CASE_FIXTURE(TestcaseBase, "seq_of_seq_of_char")
{
  input = binlog::Range("abcdef", 6);
  visitor.visit(V::SequenceBegin{3, "[c"}, input);
  CHECK(visitor.visit(V::SequenceBegin{2, "c"}, input));
  CHECK(visitor.visit(V::SequenceBegin{2, "c"}, input));
  CHECK(visitor.visit(V::SequenceBegin{2, "c"}, input));
  visitor.visit(V::SequenceEnd{});

  CHECK(result() == "[ab, cd, ef]");
}

TEST_CASE_FIXTURE(TestcaseBase, "empty_tuple")
{
  visitor.visit(V::TupleBegin{""}, input);
  visitor.visit(V::TupleEnd{});
  CHECK(result() == "()");
}

TEST_CASE_FIXTURE(TestcaseBase, "tuple_of_int_bool_char")
{
  visitor.visit(V::TupleBegin{"iyc"}, input);
  visitor.visit(int(1));
  visitor.visit(true);
  visitor.visit('a');
  visitor.visit(V::TupleEnd{});

  CHECK(result() == "(1, true, a)");
}

TEST_CASE_FIXTURE(TestcaseBase, "seq_of_variant")
{
  visitor.visit(V::SequenceBegin{3, "<0i>"}, input);
  visitor.visit(V::VariantBegin{1, "i"}, input);
  visitor.visit(V::Null{});
  visitor.visit(V::VariantEnd{});
  visitor.visit(V::VariantBegin{0, "0"}, input);
  visitor.visit(int(1));
  visitor.visit(V::VariantEnd{});
  visitor.visit(V::VariantBegin{1, "i"}, input);
  visitor.visit(int(2));
  visitor.visit(V::VariantEnd{});
  visitor.visit(V::SequenceEnd{});

  CHECK(result() == "[{null}, 1, 2]");
}

TEST_CASE_FIXTURE(TestcaseBase, "seq_of_enum")
{
  visitor.visit(V::SequenceBegin{3, "/i`E'0`a'1`b'\\"}, input);
  visitor.visit(V::Enum{"E", "b", 'i', "1"});
  visitor.visit(V::Enum{"E", "a", 'i', "2"});
  visitor.visit(V::Enum{"E", "", 'i', "3"});
  visitor.visit(V::SequenceEnd{});

  CHECK(result() == "[b, a, 0x3]");
}

TEST_CASE_FIXTURE(TestcaseBase, "empty_struct")
{
  visitor.visit(V::StructBegin{"Empty", ""}, input);
  visitor.visit(V::StructEnd{});
  CHECK(result() == "Empty");
}

TEST_CASE_FIXTURE(TestcaseBase, "empty_struct_template")
{
  visitor.visit(V::StructBegin{"Empty<A,B,C>", ""}, input);
  visitor.visit(V::StructEnd{});
  CHECK(result() == "Empty");
}

TEST_CASE_FIXTURE(TestcaseBase, "simple_struct")
{
  visitor.visit(V::StructBegin{"Alpha", "`a'i`b'y"}, input);
  visitor.visit(V::FieldBegin{"a", "i"});
  visitor.visit(int(1));
  visitor.visit(V::FieldEnd{});
  visitor.visit(V::FieldBegin{"b", "y"});
  visitor.visit(false);
  visitor.visit(V::FieldEnd{});
  visitor.visit(V::StructEnd{});

  CHECK(result() == "Alpha{ a: 1, b: false }");
}

TEST_CASE_FIXTURE(TestcaseBase, "pair_of_structs")
{
  visitor.visit(V::TupleBegin{"{Alpha`a'i}{Empty}{Beta`b'y}"}, input);
  visitor.visit(V::StructBegin{"Alpha", "`a'i"}, input);
  visitor.visit(V::FieldBegin{"a", "i"});
  visitor.visit(int(1));
  visitor.visit(V::FieldEnd{});
  visitor.visit(V::StructEnd{});
  visitor.visit(V::StructBegin{"Empty", ""}, input);
  visitor.visit(V::StructEnd{});
  visitor.visit(V::StructBegin{"Beta", "`b'y"}, input);
  visitor.visit(V::FieldBegin{"b", "y"});
  visitor.visit(true);
  visitor.visit(V::FieldEnd{});
  visitor.visit(V::StructEnd{});
  visitor.visit(V::TupleEnd{});

  CHECK(result() == "(Alpha{ a: 1 }, Empty, Beta{ b: true })");
}

TEST_CASE_FIXTURE(TestcaseBase, "struct_of_structs")
{
  visitor.visit(V::StructBegin{"Alpha", "`ccc'{Beta`d'i}`e'{Empty}"}, input);
  visitor.visit(V::FieldBegin{"ccc", "{Beta`d'i}"});
  visitor.visit(V::StructBegin{"Beta", "`d'i"}, input);
  visitor.visit(V::FieldBegin{"d", "i"});
  visitor.visit(int(1));
  visitor.visit(V::FieldEnd{});
  visitor.visit(V::StructEnd{});
  visitor.visit(V::FieldEnd{});
  visitor.visit(V::FieldBegin{"e", "{Empty}"});
  visitor.visit(V::StructBegin{"Empty", ""}, input);
  visitor.visit(V::StructEnd{});
  visitor.visit(V::FieldEnd{});
  visitor.visit(V::StructEnd{});

  CHECK(result() == "Alpha{ ccc: Beta{ d: 1 }, e: Empty }");
}

TEST_CASE_FIXTURE(TestcaseBase, "empty_field_name")
{
  visitor.visit(V::StructBegin{"BoundedInt", "`'i"}, input);
  visitor.visit(V::FieldBegin{"", "i"});
  visitor.visit(int(1024));
  visitor.visit(V::FieldEnd{});
  visitor.visit(V::StructEnd{});

  CHECK(result() == "BoundedInt{ 1024 }");
}

TEST_CASE_FIXTURE(TestcaseBase, "repeat_once")
{
  visitor.visit(V::RepeatBegin{1, "i"});
  visitor.visit(int(1));
  visitor.visit(V::RepeatEnd{1, "i"});

  CHECK(result() == "1");
}

TEST_CASE_FIXTURE(TestcaseBase, "repeat_more")
{
  visitor.visit(V::RepeatBegin{9, "i"});
  visitor.visit(int(1));
  visitor.visit(V::RepeatEnd{9, "i"});

  CHECK(result() == "1 ... <repeats 9 times>");
}
