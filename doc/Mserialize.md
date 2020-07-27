Title: Mserialize

A fast, platform dependent serialization library.

[TOC]

# Overview

Mserialize supports the serialization and deserialization of values,
represented by objects of supported types, and the visitation of serialized values.
The primary goal of the library is to make serialization as fast as possible,
while allowing correct deserialization and visitation.
To achieve this goal, Mserialize adds very little overhead, if at all, to the serialized values.
Most of the time, the [serialized format](#serialized-format) matches the original value, byte by byte.
The format is not intended to be universal, it does not serve the purpose of
exchanging messages between different platforms (e.g: between platforms of different endianness).

# Serialization and Deserialization

Objects of supported types can be easily serialized:

    [catchfile test/unit/mserialize/documentation.cpp serialize]

The supported types (`T`) for serialization include:

 - Fundamental types (`bool`, `char`, integer and floating point types)
 - Enums
 - Strings (`const char*`, string, string_view)
 - Containers (vector, deque, set, map, etc., anything with begin/end and supported `value_type`)
 - Pairs and Tuples
 - Pointers and Smart Pointers (`T*`, unique_ptr, shared_ptr, weak_ptr, with a supported `elem_type`)
 - Optional like types
 - Variant like types

Additional types can be adapted to the library, as [shown below](#adapting-custom-types).
The target `ostream` can be anything that models the [OutputStream](#outputstream) concept.

Deserialization of serialized objects is equally easy:

    [catchfile test/unit/mserialize/documentation.cpp deserialize]

Almost any object, that can be serialized, can be also _deserialized into_,
except a few, that do not own the underlying resource (e.g: `T*`, string_view, weak_ptr)
 - see [Design Rationale](#design-rationale) for details.

The source `istream` can be anything that models the [InputStream](#inputstream) concept.
Therefore, standard streams must be configured to throw exceptions on failure.

The type of the value in the input stream is inferred from the destination (first) argument of `deserialize`.
It is only allowed to deserialize a value into an object of a _compatible type_.
Compatibility is defined in terms of [type tags](#visiting-serialized-values).
Two types are _compatible_ if their type tags match.

If the deserialization fails (e.g: if the content of the stream cannot be interpreted
as a serialized value of the type inferred from the destination), `deserialize`
throws `std::runtime_error`.

## Adapting custom types

By default, serialization and deserialization of generic user defined types are not supported
(unless they meet the requirements of some supported concept, e.g: a container).
Such types can be still adapted though, by specializing extension points for these types.
The easiest way is to use macros:

    [catchfile test/unit/mserialize/documentation.cpp adapt_custom_type]

The macros have to be invoked at the global scope, outside of any namespace.
The first argument is the name of the type, the rest are the members
(The member list can be empty).
The member list does not have to enumerate every member of the given type: if a
member is omitted, it will be simply ignored during
serialization/deserialization (e.g: a mutex member is typically not to be
serialized). However, to make roundtrip work, the member lists given to each
macro must match exactly.

For serialization, a member can be either a non-static, non-reference,
non-bitfield data member, or a getter, which is a const qualified, nullary
member function, which returns a serializable object.

For deserialization, a member can be either a non-const, non-static,
non-reference, non-bitfield data member, or a setter, which takes
a single, deserializable argument.

    [catchfile test/unit/mserialize/documentation.cpp adapt_custom_type_with_getters_and_setters]

If some of the data members, getters or setters are private, but serialization
or deserialization is still preferred via those members, the following friend
declarations can be added to the type:

    [catchfile test/unit/mserialize/documentation.cpp adapt_custom_type_with_private_members]

Class templates can be made serializable and deserializable on the same conditions,
except that a different macro must be called:

    [catchfile test/unit/mserialize/documentation.cpp adapt_custom_templates]

The first argument of the macro must be the arguments of the template, with the
necessary typename prefix, where needed, as they appear after the template
keyword in the definition, wrapped by parentheses. (The parentheses are
required to avoid the preprocessor splitting the arguments at the commas)

The second argument is the template name with the template arguments, as it
should appear in a specialization, wrapped by parentheses.
The rest of the arguments are members, same as above.

# Visiting serialized values

As an alternative to deserialization, serialized objects
can be visited. Visitation is useful if the precise type
of the serialized object is not known, the type is not available,
or not deserializable.

While the precise type of the serialized object is not needed,
a _type tag_ still must be available for visitation to work.
A type tag is a string, that describes a serializable type to the extent that it can be visited.

The following example shows how serialization and visitation can work together:

    [catchfile test/unit/mserialize/documentation.cpp visit_out]

&nbsp;

    [catchfile test/unit/mserialize/documentation.cpp visit_in]

`Visitor` can be any type that models the [Visitor](#visitor) concept.
`visit` throws `std::exception` if the visitation fails
(e.g: the provided tag does not match the serialized object in the stream).
In the example, the tag is serialized alongside the object.
In general, the tag is not required to be in the stream,
it can be sent to the visiting party by any other means.
The tag given to visit must be a valid type tag:
do not use tags coming from a potentially malicious source.

## Adapting enums for visitation

By default, enums have no tag associated.
A tag, suitable for visitation can be defined in the following way:

    [catchfile test/unit/mserialize/documentation.cpp adapt_enum_for_visit]

This works with both enums and enum classes, regardless the underlying type of the enum.
The macro has to be called in global scope (outside of any namespace).
If an enumerator is omitted from the macro call, the tag will be incomplete,
and during visitation, if the missing enumerator is visited, only its
underlying value will be available, the enumerator name will be empty.

## Adapting user defined types for visitation

By default, in general, user defined types have no tag associated.
(In general, since any type modeling a specific supported concept, e.g:
user defined containers, does have a tag associated by default).
A tag, suitable for visitation can be defined in the following way:

    [catchfile test/unit/mserialize/documentation.cpp adapt_custom_type_for_visit]

The macro has to be called in global scope (outside of any namespace).
The members can be data members or getters, just like for [serialization](#adapting-custom-types).
For private members, the following friend declaration can be added:

    [catchfile test/unit/mserialize/documentation.cpp custom_tag_friend]

The member list must be in sync with the `MSERIALIZE_MAKE_STRUCT_SERIALIZABLE` call,
if visitation of objects serialized that way is desired.
`MSERIALIZE_MAKE_STRUCT_TAG` cannot be used with recursive types.
See [Adapting user defined recursive types for visitation](#adapting-user-defined-recursive-types-for-visitation) for a solution.

A tag can be assigned to class templates on the same conditions,
except that a different macro must be called:

    [catchfile test/unit/mserialize/documentation.cpp adapt_custom_template_for_visit]

The first argument of the macro must be the arguments of the template, with the
necessary typename prefix, where needed, as they appear after the template
keyword in the definition, wrapped by parentheses. (The parentheses are
required to avoid the preprocessor splitting the arguments at the commas)

The second argument is the template name with the template arguments, as it
should appear in a specialization, wrapped by parentheses.
The rest of the arguments are members, same as above.

## Adapting user defined recursive types for visitation

From the tag generation point of view, a structure is recursive
if one of its fields has a type tag that includes the type tag of the parent type.
Currently, `MSERIALIZE_MAKE_STRUCT_TAG` is unable to deal with such recursive structures.
As a workaround, such type tags can be manually assigned:

    [catchfile test/unit/mserialize/documentation.cpp recursive_tag]

A breakdown of the string literal:

 - `{`: Start structure tag
 - `Node`: Name of the structure
 - <code>`value'</code>: Name of the first field
 - `i`: Type tag of the first field. Also see the [type tag reference](#type-tags)
 - <code>`next'</code>: Name of the second field
 - `<`: Begin variant tag (pointers are modeled as either nothing or something)
 - `0`: The pointer is either null
 - `{Node}`: or points to a Node objects. This is the important part: the structure
   definition here is not expanded again, as that would result in infinite recursion.
   The visitor will recognize that Node is not an empty type, but something defined earlier.
 - `>` End variant tag
 - `}` End structure tag

# Limitations

 - To keep the implementation and interface simple, values cannot be deserialized
   into object that do not own the underlying resource, e.g: `T*` or string_view.
   See [Design Rationale](#design-rationale) for considered alternatives.

 - In the [Serialized format](#serialized-format) the size of a serialized sequence
   is represented by a 32 bit unsigned integer. Therefore, sequences longer than
   2^32 cannot be serialized. As a workaround, such sequences can be split
   into a sequence of smaller sequences.

 - In the [Serialized format](#serialized-format) the discriminator of a variant
   is represented by an 8 bit unsigned integer. Therefore, variants with more
   than 256 alternatives cannot be serialized. As a workaround, such variants
   can be split into a variant of smaller variants.

 - Macros taking arbitrary number of arguments (e.g: member lists, enumerators)
   need to iterate over the given arguments. The iteration is done by loop
   unrolling, which is currently capped at 100. This limit can be increased
   by regenerating foreach.hpp, but MSVC does not support macros with
   more than 127 arguments.

# Design Rationale

Library design tends to be arguable. Some decisions need to be explained.

**How to signal errors when deserializing?**

 1. Leave the stream in bad state. This is common practice in standard library
    components, but does not give enough context about the nature of the error.

 2. Set an `error_code`. This requires the `ec` to be propagated through every
    deserialization layer (which might or might not be good), and also requires
    several extra checks (to stop if the `ec` is set). As a deserialization
    error is considered exceptional, the nominal case should not be penalized
    with extra checks, which can be avoided with exceptions.

 3. Throw an exception. Can provide enough context, fast if there are no
    errors, requires extra care. This is the chosen solution. The type of the
    exception should be `std::runtime_error`, but on platforms using the
    pre-C++11 ABI, `std::ios_base::failure` (thrown by streams) is not derived
    from `std::runtime_error`, therefore `std::exception` must be used.

**How to deserialize non-owning types?** Let's consider `T*`:

 1. Simply allocate a `T` object on the heap, assign its address to the target
    pointer, and expect that the user will properly delete it later. This
    solution is simple, but hard to get right, especially with complicated
    structures.

 2. Provide an overload, which takes a memory manager.
    This solution is memory safe and allows a wider range of types to be
    deserialized, by requires the introduction of yet another concept, with
    further complexity.

 3. Do not allow direct deserialization of such types.
    This sharp solution is simple to implement, but it restricts some common
    types (e.g: string_view), and prevents the user from using the same type on
    both ends. Because of its simplicity, this is the chosen solution.

**How should the type tag of user defined types look like?**

 1. Type tags of user defined types should be shallow, e.g: `{Person}`, and the
    complete definition of the type has to be supplied via yet another side
    channel. This approach diverges from the original meaning of type tag, (as
    the shallow tag on its own doesn't allow visitation), and puts additional
    load on the user. On the other hand, it is easy to implement, even for
    recursive structures.

 2. Type tags should always describe the complete type. e.g:
    ``{Person`age'i`name'[c}``, _allow_ automatic generation of tags for recursive
    structures. This is a pure approach, fits nicely to the original concept
    of type tags. However, it is difficult to implement (in a efficient
    constexpr fashion) if recursive (including mutually recursive) types need
    to be supported.

 3. Type tags should always describe the complete type. e.g:
    ``{Person`age'i`name'[c}``, _disallow_ automatic generation of tags for
    recursive structures. A pure approach with some restriction. It remains
    easy to use, while allowing clients to use more difficult ways if
    visitation of recursive structures is needed. This is the chosen solution.

**Split making types serializable and generation of tags or not?**

 1. Combining them leads to slightly smaller source code, but ties the
    requirements and usage together.

 2. Separate serialization and tag generation logic aligns with the _only pay
    for what you use_ principle. It allows the two to have different
    requirements (e.g: whether recursive types are allowed), and deserializer
    programs to inspect tags without pulling in the serializer logic. On the
    other hand, the separate specialization logic needs slightly more code.
    This is the chosen solution.

# Concepts

## OutputStream

    template <typename OutStr>
    concept OutputStream = requires(OutStr ostream, const char* buf, std::streamsize size)
    {
      // Append `size` bytes from `buffer` to the stream
      { ostream.write(buf, size) } -> OutStr&;
    };

## InputStream

    template <typename InpStr>
    concept InputStream = requires(InpStr istream, char* buf, std::streamsize size)
    {
      // Consume `size` bytes from the stream and copy them to the `buffer`.
      // Throw std::exception on failure (i.e: not enough bytes available)
      { istream.read(buf, size) } -> InpStr&;
    };

## ViewStream

    template <typename VStr>
    concept ViewStream = requires(VStr vstream, std::size_t size)
    {
      // Consume `size` bytes from the stream and return a pointer
      // to the start of the consumed bytes. The returned pointer
      // is valid until the next operation.
      // Throw std::exception on failure (i.e: not enough bytes available)
      { vstream.view(size) } -> const char*;
    };

## Visitor

    template <typename V>
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

      visitor.visit(mserialize::Visitor::SequenceBegin );
      visitor.visit(mserialize::Visitor::SequenceEnd   );

      visitor.visit(mserialize::Visitor::String        );

      visitor.visit(mserialize::Visitor::TupleBegin    );
      visitor.visit(mserialize::Visitor::TupleEnd      );

      visitor.visit(mserialize::Visitor::VariantBegin  );
      visitor.visit(mserialize::Visitor::VariantEnd    );
      visitor.visit(mserialize::Visitor::Null          );

      visitor.visit(mserialize::Visitor::StructBegin   );
      visitor.visit(mserialize::Visitor::StructEnd     );

      visitor.visit(mserialize::Visitor::FieldBegin    );
      visitor.visit(mserialize::Visitor::FieldEnd      );

      visitor.visit(mserialize::Visitor::Enum          );

      visitor.visit(mserialize::Visitor::RepeatBegin   );
      visitor.visit(mserialize::Visitor::RepeatEnd     );
    };

# References

## Type tags

The table below describes the type tags of supported types.
In the first column, `T` refers to any supported type, and `T...` to any pack of supported types.
In the second column, `t` refers to the tag of `T` in the cell left of it, and `t...` to the
concatenated tags of the `T...` pack.

<table class="table">
<thead>
  <tr> <th>Type</th> <th>Type Tag</th> </tr>
</thead>
<tbody>
  <tr><td><code>bool</code></td>      <td><code>y</code></td></tr>
  <tr><td><code>char</code></td>      <td><code>c</code></td></tr>
  <tr><td><code>int8_t</code></td>    <td><code>b</code></td></tr>
  <tr><td><code>int16_t</code></td>   <td><code>s</code></td></tr>
  <tr><td><code>int32_t</code></td>   <td><code>i</code></td></tr>
  <tr><td><code>int64_t</code></td>   <td><code>l</code></td></tr>
  <tr><td><code>uint8_t</code></td>   <td><code>B</code></td></tr>
  <tr><td><code>uint16_t</code></td>  <td><code>S</code></td></tr>
  <tr><td><code>uint32_t</code></td>  <td><code>I</code></td></tr>
  <tr><td><code>uint64_t</code></td>  <td><code>L</code></td></tr>
  <tr><td><code>float</code></td>     <td><code>f</code></td></tr>
  <tr><td><code>double</code></td>    <td><code>d</code></td></tr>
  <tr><td><code>long double</code></td>    <td><code>D</code></td></tr>
  <tr><td>Array of <code>T</code></td><td><code>[t</code></td></tr>
  <tr><td>Tuple of <code>T...</code></td><td><code>(t...)</code></td></tr>
  <tr><td>Variant of <code>T...</code></td><td><code>&lt;t...&gt;</code></td></tr>
  <tr><td><code>void</code> (only to indicate empty state of a variant)</td>      <td><code>0</code></td></tr>
  <tr><td>Adapted <code>enum E : T { a, b = 123, c}</code></td><td>
  <code>/t`E'0`a'7B`b'7C`c'\</code> (see below)
  </td></tr>
  <tr><td>Adapted <code>struct Foo { T1 a; T2 b; }</code></td><td>
  <code>{Foo`a't1`b't2}</code> (see below)
  </td></tr>
</tbody>
</table>

### Type tag of Adapted Enum

    <EnumTag> ::= /<UnderlyingTypeTag><EnumName><Enumerator>*\
    <UnderlyingTypeTag> ::= b|s|i|l|B|S|I|L
    <EnumName> ::= `Typename'
    <Enumerator> ::= ValueInHex `EnumeratorName'

### Type tag of Adapted Struct

    <StructTag> ::= {<StructName><StructField>*}
    <StructName> ::= `Typename'
    <StructField> ::= `FieldName' FieldTag

## Serialized format

By default, serializable types are mapped to type tags,
and serialized according to that type tag, as described below.
User defined serializers are allowed to use different serialization schemas,
not described here.

<table class="table">
<thead>
  <tr> <th>Type</th> <th>Serialized format</th> </tr>
</thead>
<tbody>
  <tr><td>Arithmetic types (<code>y,c,b,s,i,l,B,S,I,L,f,d,D</code>)</td>
  <td>Serialized as if by memcpy</td></tr>

  <tr><td>Array of <code>T</code></td>
  <td>4 bytes (host endian) size of the array, followed by the serialized array elements</td></tr>

  <tr><td>Tuple of <code>T...</code></td>
  <td>Elements are serialized in order, without additional decoration</td></tr>

  <tr><td>Variant of <code>T...</code></td>
  <td>1 byte discriminator, followed by the serialized active option</td></tr>

  <tr><td>Adapted enum</td>
  <td>Serialized as if by memcpy</td></tr>

  <tr><td>Adapted user defined type</td>
  <td>Members are serialized in order, without additional decoration</td></tr>
</tbody>
</table>
