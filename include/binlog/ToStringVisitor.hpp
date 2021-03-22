#ifndef BINLOG_TO_STRING_VISITOR_HPP
#define BINLOG_TO_STRING_VISITOR_HPP

#include <binlog/Range.hpp>
#include <binlog/detail/OstreamBuffer.hpp>

#include <mserialize/Visitor.hpp>

#include <cstdint>

namespace binlog {

class PrettyPrinter;

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
  explicit ToStringVisitor(detail::OstreamBuffer& out, const PrettyPrinter* pp = nullptr);

  // catch all for arithmetic types
  template <typename T>
  void visit(T v)
  {
    comma();
    _out << v;
  }

  void visit(std::int8_t);
  void visit(std::uint8_t);

  bool visit(mserialize::Visitor::SequenceBegin, Range&);
  void visit(mserialize::Visitor::SequenceEnd);

  bool visit(mserialize::Visitor::TupleBegin, const Range&);
  void visit(mserialize::Visitor::TupleEnd);

  bool visit(mserialize::Visitor::VariantBegin, const Range&) { return false; }
  void visit(mserialize::Visitor::VariantEnd) {}
  void visit(mserialize::Visitor::Null);

  void visit(mserialize::Visitor::Enum);

  bool visit(mserialize::Visitor::StructBegin, Range&);
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

  State _state;
  int _seqDepth;
  bool _emptyStruct;
  detail::OstreamBuffer& _out;
  const PrettyPrinter* _pp;
};

} // namespace binlog

#endif // BINLOG_TO_STRING_VISITOR_HPP
