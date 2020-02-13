#include <binlog/ToStringVisitor.hpp>

#include <mserialize/detail/tag_util.hpp> // remove_prefix_before


namespace binlog {

ToStringVisitor::ToStringVisitor(detail::OstreamBuffer& out)
  :_state(State::Normal),
   _seqDepth(0),
   _emptyStruct(false),
   _out(out)
{}

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

void ToStringVisitor::visit(mserialize::Visitor::SequenceBegin)
{
  comma();

  _out.put('[');
  enterSeq();
}

void ToStringVisitor::visit(mserialize::Visitor::SequenceEnd)
{
  _out.put(']');
  leaveSeq();
}

void ToStringVisitor::visit(mserialize::Visitor::String str)
{
  comma();
  _out << str.data;
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
    _out << " }";
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
    _out << ", ";
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
