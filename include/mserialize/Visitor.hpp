#ifndef MSERIALIZE_VISITOR_HPP
#define MSERIALIZE_VISITOR_HPP

#include <mserialize/string_view.hpp>

#include <cstdint>

namespace mserialize {

/*
 * Concept of the `visitor` argument of mserialize::visit.
 *
 * The Visitor object given to mserialize::visit
 * must model this concept.
 *
 * During visitation, the `visit` methods
 * are called, according to the given
 * type tag and input stream.
 *
 * Some `visit` methods take an InputStream argument:
 * The visitor can decide to consume the complete
 * object indicated by the type tag from the given InputStream,
 * and `return true` to signal that visitation of
 * this object must be skipped.
//[concept
template <typename V, typename InputStream>
concept Visitor = requires(V visitor)
{
  visitor.visit(bool          );
  visitor.visit(char          );
  visitor.visit(std::int8_t   );
  visitor.visit(std::int16_t  );
  visitor.visit(std::int32_t  );
  visitor.visit(std::int64_t  );
  visitor.visit(std::uint8_t  );
  visitor.visit(std::uint16_t );
  visitor.visit(std::uint32_t );
  visitor.visit(std::uint64_t );

  visitor.visit(float         );
  visitor.visit(double        );
  visitor.visit(long double   );

  visitor.visit(mserialize::Visitor::SequenceBegin, InputStream&) -> bool;
  visitor.visit(mserialize::Visitor::SequenceEnd   );

  visitor.visit(mserialize::Visitor::String        );

  visitor.visit(mserialize::Visitor::TupleBegin, InputStream&) -> bool;
  visitor.visit(mserialize::Visitor::TupleEnd      );

  visitor.visit(mserialize::Visitor::VariantBegin, InputStream&) -> bool;
  visitor.visit(mserialize::Visitor::VariantEnd    );
  visitor.visit(mserialize::Visitor::Null          );

  visitor.visit(mserialize::Visitor::StructBegin, InputStream&) -> bool;
  visitor.visit(mserialize::Visitor::StructEnd     );

  visitor.visit(mserialize::Visitor::FieldBegin    );
  visitor.visit(mserialize::Visitor::FieldEnd      );

  visitor.visit(mserialize::Visitor::Enum          );

  visitor.visit(mserialize::Visitor::RepeatBegin   );
  visitor.visit(mserialize::Visitor::RepeatEnd     );
};
//]
*/

/**
 * Collection of the visit tags `mserialize::visit` use.
 */
struct Visitor
{
  // Sequence

  struct SequenceBegin
  {
    std::size_t size{}; /**< Size of the sequence */
    string_view tag;    /**< Type tag of the sequence elements */
  };
  struct SequenceEnd {};

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

  // Repeat - same value repeats `size` times

  struct RepeatBegin
  {
    std::uint32_t size{}; /**< Number of time the next visted value repeats */
    string_view tag;      /**< Type tag of the repeating value */
  };

  struct RepeatEnd
  {
    std::uint32_t size{}; /**< Number of time the previous visted value repeats */
    string_view tag;      /**< Type tag of the repeating value */
  };
};

} // namespace mserialize

#endif // MSERIALIZE_VISITOR_HPP
