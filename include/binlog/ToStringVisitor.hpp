#ifndef BINLOG_TO_STRING_VISITOR_HPP
#define BINLOG_TO_STRING_VISITOR_HPP

#include <binlog/detail/OstreamBuffer.hpp>

#include <mserialize/Visitor.hpp>

#include <cstdint>

namespace binlog {

/**
 * Convert serialized values to string.
 *
 * Writes the result to the given stream.
 * Models the mserialize::Visitor concept
 *
 * Usage:
 *
 *    std::ostringstream str;
 *    binlog::detail::OstreamBuffer buf(str);
 *    ToStringVisitor visitor(buf);
 *    mserialize::visit(tag, visitor, istream);
 */
class ToStringVisitor
{
public:
  explicit ToStringVisitor(detail::OstreamBuffer& out);

  // catch all for arithmetic types
  template <typename T>
  void visit(T v)
  {
    comma();
    _out << v;
  }

  void visit(std::int8_t);
  void visit(std::uint8_t);

  void visit(mserialize::Visitor::SequenceBegin);
  void visit(mserialize::Visitor::SequenceEnd);

  void visit(mserialize::Visitor::String);

  void visit(mserialize::Visitor::TupleBegin);
  void visit(mserialize::Visitor::TupleEnd);

  void visit(mserialize::Visitor::VariantBegin) {}
  void visit(mserialize::Visitor::VariantEnd) {}
  void visit(mserialize::Visitor::Null);

  void visit(mserialize::Visitor::Enum);

  void visit(mserialize::Visitor::StructBegin);
  void visit(mserialize::Visitor::StructEnd);

  void visit(mserialize::Visitor::FieldBegin);
  void visit(mserialize::Visitor::FieldEnd);

private:
  void comma();

  void enterSeq();

  void leaveSeq();

  // In sequences (sequence, tuple, structure fields)
  // add a comma between each element.
  enum class State { Normal, SeqBegin, Seq };

  State _state;
  int _seqDepth;
  bool _emptyStruct;
  detail::OstreamBuffer& _out;
};

} // namespace binlog

#endif // BINLOG_TO_STRING_VISITOR_HPP
