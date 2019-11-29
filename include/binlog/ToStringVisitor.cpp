#include <binlog/ToStringVisitor.hpp>

#include <mserialize/detail/tag_util.hpp> // remove_prefix_before

#include <ios> // boolalpha

namespace binlog {

ToStringVisitor::ToStringVisitor(std::ostream& out)
  :_state(State::Normal),
   _seqDepth(0),
   _emptyStruct(false),
   _out(out)
{
  _out << std::boolalpha;
}

// avoid displaying int8_t and uint8_t as a character
void ToStringVisitor::visit(std::int8_t v)
{
  comma();
  _out << int(v);
}

void ToStringVisitor::visit(std::uint8_t v)
{
  comma();
  _out << unsigned(v);
}

void ToStringVisitor::visit(mserialize::Visitor::SequenceBegin sb)
{
  comma();

  // TODO(benedek) perf: special-case strings at the Visitor interface level
  if (sb.tag == "c")
  {
    // special case for strings, render them as foo instead of [ f, o, o ]
    _state = State::CharSeq;
    ++_seqDepth;
  }
  else
  {
    _out.put('[');
    enterSeq();
  }
}

void ToStringVisitor::visit(mserialize::Visitor::SequenceEnd)
{
  if (_state != State::CharSeq) { _out.put(']'); }
  leaveSeq();
}

void ToStringVisitor::visit(mserialize::Visitor::TupleBegin)
{
  comma();
  _out.put('(');
  enterSeq();
}

void ToStringVisitor::visit(mserialize::Visitor::TupleEnd)
{
  _out.put(')');
  leaveSeq();
}

void ToStringVisitor::visit(mserialize::Visitor::Null)
{
  comma();
  _out << "{null}";
}

void ToStringVisitor::visit(mserialize::Visitor::Enum e)
{
  comma();
  if (e.enumerator.empty())
  {
    _out << "0x" << e.value;
  }
  else
  {
    _out << e.enumerator;
  }
}

void ToStringVisitor::visit(mserialize::Visitor::StructBegin sb)
{
  comma();
  _out << mserialize::detail::remove_prefix_before(sb.name, '<');
  if (sb.tag.empty())
  {
    _emptyStruct = true;
  }
  else
  {
    _out << "{ ";
    enterSeq();
  }
}

void ToStringVisitor::visit(mserialize::Visitor::StructEnd)
{
  if (_emptyStruct)
  {
    _emptyStruct = false;
  }
  else
  {
    _out.write(" }", 2);
    leaveSeq();
  }
}

void ToStringVisitor::visit(mserialize::Visitor::FieldBegin fb)
{
  comma();
  if (! fb.name.empty())
  {
    _out << fb.name << ": ";
  }
  _state = State::Normal;
}

void ToStringVisitor::visit(mserialize::Visitor::FieldEnd)
{
  _state = State::Seq;
}

void ToStringVisitor::comma()
{
  if (_state == State::SeqBegin)
  {
    _state = State::Seq;
  }
  else if (_state == State::Seq)
  {
    _out.write(", ", 2);
  }
}

void ToStringVisitor::enterSeq()
{
  _state = State::SeqBegin;
  ++_seqDepth;
}

void ToStringVisitor::leaveSeq()
{
  --_seqDepth;
  _state = (_seqDepth == 0) ? State::Normal : State::Seq;
}


} // namespace binlog
