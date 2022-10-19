// This is an example reader program that converts a binlog file to json.
// The binlog input is read from stdin, and for each entry, a json object
// is written to stdout.
//
// Usage example:
//     ./LoggingAdaptedStructs | ./binlog2json
//
// To test correctness:
//     for i in Logging*; do echo $i; ./$i|./binlog2json|python -m json.tool --json-line > /dev/null ; done

#include <iomanip>
#include <iostream>
#include <map>

#include <binlog/Entries.hpp>
#include <binlog/EntryStream.hpp>
#include <binlog/Range.hpp>

#include <mserialize/deserialize.hpp>
#include <mserialize/visit.hpp>

#include <mserialize/detail/tag_util.hpp>

// An mserialize::Visitor that visits a serialized value
// and writes it to the given ostream as JSON.
// Mostly a copy of binlog/ToStringVisitor.
//
// Example input (before serialization):
//
//   struct Foo {
//      std::vector<int> bar = {1,2,3};
//      std::tuple<char, std::string> baz = {'c', "qux"};
//      enum { Left, Right } dir = Right;
//   }
//
// Example output (after visitation):
//
//   {"@type": "Foo", "bar": [1,2,3], "baz": ["c", "qux"], "dir": "Right"}
//
// To keep this example simple, this solution
// produces invalid JSON if a visited string contains an invalid Unicode codepoint.
class ToJsonVisitor
{
public:
  explicit ToJsonVisitor(std::ostream& out)
    :_out(out)
  {
    _out << std::boolalpha;
  }

  // catch all for arithmetic types
  template <typename T>
  void visit(T v)
  {
    comma();
    _out << v;
  }

  void visit(char);

  void visit(std::int8_t);
  void visit(std::uint8_t);

  bool visit(mserialize::Visitor::SequenceBegin, binlog::Range&);
  void visit(mserialize::Visitor::SequenceEnd);

  bool visit(mserialize::Visitor::TupleBegin, const binlog::Range&);
  void visit(mserialize::Visitor::TupleEnd);

  bool visit(mserialize::Visitor::VariantBegin, const binlog::Range&) { return false; }
  void visit(mserialize::Visitor::VariantEnd) {}
  void visit(mserialize::Visitor::Null);

  void visit(mserialize::Visitor::Enum);

  bool visit(mserialize::Visitor::StructBegin, binlog::Range&);
  void visit(mserialize::Visitor::StructEnd);

  void visit(mserialize::Visitor::FieldBegin);
  void visit(mserialize::Visitor::FieldEnd);

  void visit(mserialize::Visitor::RepeatBegin);
  void visit(mserialize::Visitor::RepeatEnd);

private:
  void comma();

  void enterSeq();

  void leaveSeq();

  // In sequences (sequence, tuple, structure fields)
  // add a comma between each element.
  enum class State { Normal, SeqBegin, Seq };

  State _state = State::Normal;
  int _seqDepth = 0;
  std::ostream& _out;
};

void ToJsonVisitor::visit(char c)
{
  comma();
  _out.put('"').put(c).put('"');
}

// avoid displaying int8_t and uint8_t as a character
void ToJsonVisitor::visit(std::int8_t v)
{
  comma();
  _out << int(v);
}

void ToJsonVisitor::visit(std::uint8_t v)
{
  comma();
  _out << unsigned(v);
}

bool ToJsonVisitor::visit(mserialize::Visitor::SequenceBegin sb, binlog::Range& input)
{
  comma();

  if (sb.tag.size() == 1 && sb.tag[0] == 'c') // skip char-by-char visitation of strings
  {
    // Note: if needed, add Unicode verification here.
    const std::string str(input.view(sb.size), sb.size);
    _out << std::quoted(str);
    return true;
  }

  _out.put('[');
  enterSeq();

  return false;
}

void ToJsonVisitor::visit(mserialize::Visitor::SequenceEnd)
{
  _out.put(']');
  leaveSeq();
}

bool ToJsonVisitor::visit(mserialize::Visitor::TupleBegin, const binlog::Range&)
{
  comma();
  _out.put('[');
  enterSeq();
  return false;
}

void ToJsonVisitor::visit(mserialize::Visitor::TupleEnd)
{
  _out.put(']');
  leaveSeq();
}

void ToJsonVisitor::visit(mserialize::Visitor::Null)
{
  comma();
  _out << "\"null\"";
}

void ToJsonVisitor::visit(mserialize::Visitor::Enum e)
{
  comma();
  if (e.enumerator.empty())
  {
    _out << "\"0x" << e.value << '"';
  }
  else
  {
    _out << std::quoted(e.enumerator.to_string());
  }
}

bool ToJsonVisitor::visit(mserialize::Visitor::StructBegin sb, binlog::Range&)
{
  comma();

  _out << "{\"@type\":" << std::quoted(mserialize::detail::remove_prefix_before(sb.name, '<').to_string());
  enterSeq();
  _state = State::Seq;

  return false;
}

void ToJsonVisitor::visit(mserialize::Visitor::StructEnd)
{
  _out.put('}');
  leaveSeq();
}

void ToJsonVisitor::visit(mserialize::Visitor::FieldBegin fb)
{
  comma();
  _out << std::quoted(fb.name.to_string()) << ':';
  _state = State::Normal;
}

void ToJsonVisitor::visit(mserialize::Visitor::FieldEnd)
{
  _state = State::Seq;
}

void ToJsonVisitor::comma()
{
  if (_state == State::SeqBegin)
  {
    _state = State::Seq;
  }
  else if (_state == State::Seq)
  {
    _out.put(',');
  }
}

void ToJsonVisitor::visit(mserialize::Visitor::RepeatBegin rb)
{
  _out << R"({"@type":"RepeatedValue","size":)" << rb.size
       << ",\"value\":";
}

void ToJsonVisitor::visit(mserialize::Visitor::RepeatEnd)
{
  _out.put('}');
}

void ToJsonVisitor::enterSeq()
{
  _state = State::SeqBegin;
  ++_seqDepth;
}

void ToJsonVisitor::leaveSeq()
{
  --_seqDepth;
  _state = (_seqDepth == 0) ? State::Normal : State::Seq;
}

// Helpers that convert a binlog Entry to JSON

void writeJson(const binlog::EventSource& es, std::ostream& out) {
  out << R"({"@type":"EventSource")"
      << ",\"id\":" << es.id
      << ",\"severity\":" << std::quoted(binlog::severityToString(es.severity).data())
      << ",\"category\":" << std::quoted(es.category)
      << ",\"function\":" << std::quoted(es.function)
      << ",\"file\":" << std::quoted(es.file)
      << ",\"line\":" << es.line
      << ",\"formatString\":" << std::quoted(es.formatString)
      << ",\"argumentTags\":" << std::quoted(es.argumentTags)
      << "}\n";
}

void writeJson(const binlog::WriterProp& wp, std::ostream& out) {
  out << R"({"@type":"WriterProp")"
      << ",\"id\":" << wp.id
      << ",\"name\":" << std::quoted(wp.name)
      << ",\"batchSize\":" << wp.batchSize
      << "}\n";
}

void writeJson(const binlog::ClockSync& cs, std::ostream& out) {
  out << R"({"@type":"ClockSync")"
      << ",\"clockValue\":" << cs.clockValue
      << ",\"clockFrequency\":" << cs.clockFrequency
      << ",\"nsSinceEpoch\":" << cs.nsSinceEpoch
      << ",\"tzOffset\":" << cs.tzOffset
      << ",\"tzName\":" << std::quoted(cs.tzName)
      << "}\n";
}

void writeJson(binlog::Range args, mserialize::string_view tags, std::ostream& out) {
  ToJsonVisitor visitor(out);
  visitor.visit(mserialize::Visitor::SequenceBegin{}, args);
  while (! tags.empty()) {
    const mserialize::string_view tag = mserialize::detail::tag_pop(tags);
    mserialize::visit(tag, visitor, args);
  }
  visitor.visit(mserialize::Visitor::SequenceEnd{});
}

void binlog2json(binlog::IstreamEntryStream& input, std::ostream& out) {
  std::map<std::uint64_t, binlog::EventSource> sources;

  while (true)
  {
    binlog::Range range = input.nextEntryPayload();
    if (range.empty()) { break; }

    const std::uint64_t tag = range.read<std::uint64_t>();
    const bool special = (tag & (std::uint64_t(1) << 63)) != 0;

    if (special)
    {
      switch (tag)
      {
        case binlog::EventSource::Tag:
        {
          binlog::EventSource es;
          mserialize::deserialize(es, range);
          writeJson(es, out);
          sources.emplace(es.id, std::move(es));
          break;
        }
        case binlog::WriterProp::Tag:
        {
          binlog::WriterProp wp;
          mserialize::deserialize(wp, range);
          writeJson(wp, out);
          break;
        }
        case binlog::ClockSync::Tag:
        {
          binlog::ClockSync cs;
          mserialize::deserialize(cs, range);
          writeJson(cs, out);
          break;
        }
        // default: ignore unkown special entries
        // to be forward compatible.
      }
    }
    else
    {
      auto it = sources.find(tag);
      if (it == sources.end())
      {
        std::cerr << "Event has invalid source id: " << tag << "\n";
        continue;
      }
      auto&& es = it->second;

      const auto clockValue = range.read<std::uint64_t>();
      out << R"({"@type":"Event")"
        << ",\"clockValue\":" << clockValue
        << ",\"eventSourceId\":" << es.id
        << ",\"arguments\":";
      writeJson(range, es.argumentTags, out);
      out << "}\n";
    }
  }
}

// Mostly copied from binlog/EventStream.cpp
int main()
{
  std::ostream::sync_with_stdio(false);
  binlog::IstreamEntryStream input(std::cin);

  try
  {
    binlog2json(input, std::cout);
  }
  catch (const std::exception& ex)
  {
    std::cerr << "[binlog2json] Exception: " << ex.what() << "\n";
    return 1;
  }
}

