#ifndef TEST_UNIT_MSERIALIZE_TEST_STRUCTS_HPP
#define TEST_UNIT_MSERIALIZE_TEST_STRUCTS_HPP

#include <string>
#include <utility>

struct Base1
{
  int a = 0;
  Base1() = default;
  explicit Base1(int a_) :a(a_) {}
};

struct Base2 : Base1
{
  int b = 0;

  Base2() = default;
  Base2(int a_, int b_)
    :Base1{a_},
     b(b_)
  {}
};

struct Base3 { std::string c; };
struct Derived1 : Base2, Base3
{
  int d = 0;
  int e = 0;

  Derived1() = default;
  Derived1(int a_, int b_, std::string c_, int d_, int e_)
    :Base2(a_, b_),
     Base3{std::move(c_)},
     d(d_),
     e(e_)
  {}
};

struct Derived2 : Derived1
{
  Derived2() = default;
  Derived2(int a_, int b_, std::string c_, int d_, int e_)
    :Derived1(a_, b_, std::move(c_), d_, e_)
  {}

private:
  friend bool operator==(const Derived2& x, const Derived2& y)
  {
    return x.a == y.a
      &&   x.b == y.b
      &&   x.c == y.c
      &&   x.d == y.d
      &&   x.e == y.e;
  }
};

#endif // TEST_UNIT_MSERIALIZE_TEST_STRUCTS_HPP
