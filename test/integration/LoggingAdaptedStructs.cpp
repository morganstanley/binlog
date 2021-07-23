#include <binlog/binlog.hpp>

#include <array>
#include <iostream>
#include <string>

struct Empty {};
BINLOG_ADAPT_STRUCT(Empty)

struct Single { int a = 0; };
BINLOG_ADAPT_STRUCT(Single, a)

struct Pair { int a = 0; std::string b; };
BINLOG_ADAPT_STRUCT(Pair, a, b)

struct Getter
{
  int a() const { return 33; };
  std::string b() const { return "foo"; }

  // unused overloads to make life hard:
  std::string b() { return "foo"; }
  std::string b(int) const { return "foo"; }

};
BINLOG_ADAPT_STRUCT(Getter, a, b)

namespace app {

struct Namespaced { int a = 0; int b() const { return 2; } };

} // namespace app
BINLOG_ADAPT_STRUCT(app::Namespaced, a, b)

class Private
{
  int a = 3;
  int b = 4;

  BINLOG_ADAPT_STRUCT_FRIEND;
};
BINLOG_ADAPT_STRUCT(Private, a, b)

//[adapt_template
template <typename A, typename B, std::size_t C>
struct Triplet
{
  A a{};
  B b{};
  std::array<int, C> c{};
};
BINLOG_ADAPT_TEMPLATE((typename A, typename B, std::size_t C), (Triplet<A,B,C>), a, b, c)
//]

class first
{
public:
  class First
  {

  } business;
} way;

BINLOG_ADAPT_STRUCT(first::First)
BINLOG_ADAPT_STRUCT(first, business)

//[mixed
struct Foo
{
  int a = 0;
  std::string b;

  bool c() const { return true; }
};

BINLOG_ADAPT_STRUCT(Foo, a, b, c)
//]

int main()
{
  BINLOG_INFO("{}", Empty{});
  // Outputs: Empty

  BINLOG_INFO("{}", Single{123});
  // Outputs: Single{ a: 123 }

  BINLOG_INFO("{}", Pair{2, "three"});
  // Outputs: Pair{ a: 2, b: three }

  BINLOG_INFO("{}", Getter{});
  // Outputs: Getter{ a: 33, b: foo }

  BINLOG_INFO("{}", app::Namespaced{3});
  // Outputs: app::Namespaced{ a: 3, b: 2 }

  BINLOG_INFO("{}", Private{});
  // Outputs: Private{ a: 3, b: 4 }

  BINLOG_INFO("{}", Triplet<int, bool, 2>{1, true, {2,3}});
  // Outputs: Triplet{ a: 1, b: true, c: [2, 3] }

  BINLOG_INFO("{}", way);
  // Outputs: first{ business: first::First }

  //[mixed

  BINLOG_INFO("My foo: {}", Foo{1, "two"});
  // Outputs: My foo: Foo{ a: 1, b: two, c: true }
  //]

  binlog::consume(std::cout);
  return 0;
}
