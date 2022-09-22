#include <binlog/binlog.hpp>

#include <iostream>

// Empty

template<typename T>
concept Empty = T::is_empty::value;

#ifdef __clang__
  // Since C++20, passing no arguments to a variadic macro is allowed,
  // and we are testing wheter it works. clang 10 produces an incorrect warning.
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

BINLOG_ADAPT_CONCEPT(Empty)

#ifdef __clang__
  #pragma clang diagnostic pop
#endif

struct Nothing {
  using is_empty = std::true_type;
};

// Concept

//[concept
template<typename T>
concept Stringable = requires(T a)
{
  { a.str() } -> std::convertible_to<std::string>;
};

BINLOG_ADAPT_CONCEPT(Stringable, str)

struct Foo {
  std::string str() const { return "bar"; }
};
//]

// Concept shadowed by BINLOG_ADAPT_STRUCT

struct Baz {
  std::string str() const { return "baz"; }
  int id() const { return 123; }
};

BINLOG_ADAPT_STRUCT(Baz, id)

static_assert(Stringable<Baz>);

// Concept shadowed by BINLOG_ADAPT_TEMPLATE

template <typename A>
struct Wrap
{
  std::string str() const { return "wrap"; }

  A a{};
};

BINLOG_ADAPT_TEMPLATE((typename A), (Wrap<A>), a)

static_assert(Stringable<Wrap<int>>);

int main()
{
  BINLOG_INFO("{}", Nothing{});
  // Outputs: Empty

  //[concept
  BINLOG_INFO("{}", Foo{});
  // Outputs: Stringable{ str: bar }
  //]

  BINLOG_INFO("{}", Baz{});
  // Outputs: Baz{ id: 123 }

  BINLOG_INFO("{}", Wrap<int>{456});
  // Outputs: Wrap{ a: 456 }

  binlog::consume(std::cout);
  return 0;
}
