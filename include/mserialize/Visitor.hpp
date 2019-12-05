#ifndef MSERIALIZE_VISITOR_HPP
#define MSERIALIZE_VISITOR_HPP

#include <mserialize/string_view.hpp>

#include <cstdint>

namespace mserialize {

/**
 * Interface/concept of the `visitor` argument of mserialize::visit.
 *
 * The Visitor object given to mserialize::visit
 * must derive from this class OR must have methods
 * matching the virtual methods of this class.
 *
 * During visitation, the `visit` methods
 * will be called, according to the given
 * type tag and input stream.
 */
struct Visitor
{
  virtual ~Visitor() = default;

  // Sequence

  struct SequenceBegin
  {
    std::size_t size{}; /**< Size of the sequence */
    string_view tag;    /**< Type tag of the sequence elements */
  };
  struct SequenceEnd {};

  // String - a special Sequence of char, for efficiency

  struct String
  {
    string_view data;
  };

  // Tuple

  struct TupleBegin
  {
    string_view tag; /**< Concatenated type tag of the tuple elements */
  };
  struct TupleEnd {};

  // Variant

  struct VariantBegin
  {
    std::size_t discriminator{}; /**< Index of the selected option */
    string_view tag;             /**< Type tag of the selected option */
  };
  struct VariantEnd {};

  /** Called for type tag `0`, e.g: when a nullptr is deserialized */
  struct Null {};

  // Structure

  struct StructBegin
  {
    string_view name; /**< Name of the structure */
    string_view tag;  /**< Concatenated field names and tags */
  };
  struct StructEnd {};

  struct FieldBegin
  {
    string_view name; /**< Name of the structure field */
    string_view tag;  /**< Type tag of the structure field */
  };
  struct FieldEnd {};

  // Enum

  struct Enum
  {
    string_view name;       /**< Name of the enum */
    string_view enumerator; /**< Name of the selected enumerator, empty if unknown */
    char tag{};             /**< Integer tag of the underlying type */
    string_view value;      /**< Hexadecimal value of the enumerator */
  };

  // visitor methods - to be implemented by derived type

  virtual void visit(bool          ) = 0;
  virtual void visit(char          ) = 0;
  virtual void visit(std::int8_t   ) = 0;
  virtual void visit(std::int16_t  ) = 0;
  virtual void visit(std::int32_t  ) = 0;
  virtual void visit(std::int64_t  ) = 0;
  virtual void visit(std::uint8_t  ) = 0;
  virtual void visit(std::uint16_t ) = 0;
  virtual void visit(std::uint32_t ) = 0;
  virtual void visit(std::uint64_t ) = 0;

  virtual void visit(float         ) = 0;
  virtual void visit(double        ) = 0;
  virtual void visit(long double   ) = 0;

  virtual void visit(SequenceBegin ) = 0;
  virtual void visit(SequenceEnd   ) = 0;

  virtual void visit(TupleBegin    ) = 0;
  virtual void visit(TupleEnd      ) = 0;

  virtual void visit(VariantBegin  ) = 0;
  virtual void visit(VariantEnd    ) = 0;
  virtual void visit(Null          ) = 0;

  virtual void visit(StructBegin   ) = 0;
  virtual void visit(StructEnd     ) = 0;

  virtual void visit(FieldBegin    ) = 0;
  virtual void visit(FieldEnd      ) = 0;

  virtual void visit(Enum          ) = 0;
};

} // namespace mserialize

#endif // MSERIALIZE_VISITOR_HPP
