// RUN: %clang_cc1 -std=c++2a -verify %s -fcxx-exceptions -triple=x86_64-linux-gnu -Wno-mismatched-new-delete

#include "Inputs/std-compare.h"

namespace std {
  struct type_info;
  struct destroying_delete_t {
    explicit destroying_delete_t() = default;
  } inline constexpr destroying_delete{};
  struct nothrow_t {
    explicit nothrow_t() = default;
  } inline constexpr nothrow{};
  using size_t = decltype(sizeof(0));
  enum class align_val_t : size_t {};
};

[[nodiscard]] void *operator new(std::size_t, const std::nothrow_t&) noexcept;
[[nodiscard]] void *operator new(std::size_t, std::align_val_t, const std::nothrow_t&) noexcept;
[[nodiscard]] void *operator new[](std::size_t, const std::nothrow_t&) noexcept;
[[nodiscard]] void *operator new[](std::size_t, std::align_val_t, const std::nothrow_t&) noexcept;
void operator delete(void*, const std::nothrow_t&) noexcept;
void operator delete(void*, std::align_val_t, const std::nothrow_t&) noexcept;
void operator delete[](void*, const std::nothrow_t&) noexcept;
void operator delete[](void*, std::align_val_t, const std::nothrow_t&) noexcept;

// Helper to print out values for debugging.
constexpr void not_defined();
template<typename T> constexpr void print(T) { not_defined(); }

namespace ThreeWayComparison {
  struct A {
    int n;
    constexpr friend int operator<=>(const A &a, const A &b) {
      return a.n < b.n ? -1 : a.n > b.n ? 1 : 0;
    }
  };
  static_assert(A{1} <=> A{2} < 0);
  static_assert(A{2} <=> A{1} > 0);
  static_assert(A{2} <=> A{2} == 0);

  static_assert(1 <=> 2 < 0);
  static_assert(2 <=> 1 > 0);
  static_assert(1 <=> 1 == 0);
  constexpr int k = (1 <=> 1, 0);
  // expected-warning@-1 {{three-way comparison result unused}}

  static_assert(std::strong_ordering::equal == 0);

  constexpr void f() {
    void(1 <=> 1);
  }

  struct MemPtr {
    void foo() {}
    void bar() {}
    int data;
    int data2;
    long data3;
  };

  struct MemPtr2 {
    void foo() {}
    void bar() {}
    int data;
    int data2;
    long data3;
  };
  using MemPtrT = void (MemPtr::*)();

  using FnPtrT = void (*)();

  void FnPtr1() {}
  void FnPtr2() {}

#define CHECK(...) ((__VA_ARGS__) ? void() : throw "error")
#define CHECK_TYPE(...) static_assert(__is_same(__VA_ARGS__));

constexpr bool test_constexpr_success = [] {
  {
    auto &EQ = std::strong_ordering::equal;
    auto &LESS = std::strong_ordering::less;
    auto &GREATER = std::strong_ordering::greater;
    using SO = std::strong_ordering;
    auto eq = (42 <=> 42);
    CHECK_TYPE(decltype(eq), SO);
    CHECK(eq.test_eq(EQ));

    auto less = (-1 <=> 0);
    CHECK_TYPE(decltype(less), SO);
    CHECK(less.test_eq(LESS));

    auto greater = (42l <=> 1u);
    CHECK_TYPE(decltype(greater), SO);
    CHECK(greater.test_eq(GREATER));
  }
  {
    using PO = std::partial_ordering;
    auto EQUIV = PO::equivalent;
    auto LESS = PO::less;
    auto GREATER = PO::greater;

    auto eq = (42.0 <=> 42.0);
    CHECK_TYPE(decltype(eq), PO);
    CHECK(eq.test_eq(EQUIV));

    auto less = (39.0 <=> 42.0);
    CHECK_TYPE(decltype(less), PO);
    CHECK(less.test_eq(LESS));

    auto greater = (-10.123 <=> -101.1);
    CHECK_TYPE(decltype(greater), PO);
    CHECK(greater.test_eq(GREATER));
  }
  {
    using SE = std::strong_equality;
    auto EQ = SE::equal;
    auto NEQ = SE::nonequal;

    MemPtrT P1 = &MemPtr::foo;
    MemPtrT P12 = &MemPtr::foo;
    MemPtrT P2 = &MemPtr::bar;
    MemPtrT P3 = nullptr;

    auto eq = (P1 <=> P12);
    CHECK_TYPE(decltype(eq), SE);
    CHECK(eq.test_eq(EQ));

    auto neq = (P1 <=> P2);
    CHECK_TYPE(decltype(eq), SE);
    CHECK(neq.test_eq(NEQ));

    auto eq2 = (P3 <=> nullptr);
    CHECK_TYPE(decltype(eq2), SE);
    CHECK(eq2.test_eq(EQ));
  }
  {
    using SE = std::strong_equality;
    auto EQ = SE::equal;
    auto NEQ = SE::nonequal;

    FnPtrT F1 = &FnPtr1;
    FnPtrT F12 = &FnPtr1;
    FnPtrT F2 = &FnPtr2;
    FnPtrT F3 = nullptr;

    auto eq = (F1 <=> F12);
    CHECK_TYPE(decltype(eq), SE);
    CHECK(eq.test_eq(EQ));

    auto neq = (F1 <=> F2);
    CHECK_TYPE(decltype(neq), SE);
    CHECK(neq.test_eq(NEQ));
  }
  { // mixed nullptr tests
    using SO = std::strong_ordering;
    using SE = std::strong_equality;

    int x = 42;
    int *xp = &x;

    MemPtrT mf = nullptr;
    MemPtrT mf2 = &MemPtr::foo;
    auto r3 = (mf <=> nullptr);
    CHECK_TYPE(decltype(r3), std::strong_equality);
    CHECK(r3.test_eq(SE::equal));
  }

  return true;
}();

template <auto LHS, auto RHS, bool ExpectTrue = false>
constexpr bool test_constexpr() {
  using nullptr_t = decltype(nullptr);
  using LHSTy = decltype(LHS);
  using RHSTy = decltype(RHS);
  // expected-note@+1 {{subexpression not valid in a constant expression}}
  auto Res = (LHS <=> RHS);
  if constexpr (__is_same(LHSTy, nullptr_t) || __is_same(RHSTy, nullptr_t)) {
    CHECK_TYPE(decltype(Res), std::strong_equality);
  }
  if (ExpectTrue)
    return Res == 0;
  return Res != 0;
}
int dummy = 42;
int dummy2 = 101;

constexpr bool tc1 = test_constexpr<nullptr, &dummy>();
constexpr bool tc2 = test_constexpr<&dummy, nullptr>();

// OK, equality comparison only
constexpr bool tc3 = test_constexpr<&MemPtr::foo, nullptr>();
constexpr bool tc4 = test_constexpr<nullptr, &MemPtr::foo>();
constexpr bool tc5 = test_constexpr<&MemPtr::foo, &MemPtr::bar>();

constexpr bool tc6 = test_constexpr<&MemPtr::data, nullptr>();
constexpr bool tc7 = test_constexpr<nullptr, &MemPtr::data>();
constexpr bool tc8 = test_constexpr<&MemPtr::data, &MemPtr::data2>();

// expected-error@+1 {{must be initialized by a constant expression}}
constexpr bool tc9 = test_constexpr<&dummy, &dummy2>(); // expected-note {{in call}}

template <class T, class R, class I>
constexpr T makeComplex(R r, I i) {
  T res{r, i};
  return res;
};

template <class T, class ResultT>
constexpr bool complex_test(T x, T y, ResultT Expect) {
  auto res = x <=> y;
  CHECK_TYPE(decltype(res), ResultT);
  return res.test_eq(Expect);
}
static_assert(complex_test(makeComplex<_Complex double>(0.0, 0.0),
                           makeComplex<_Complex double>(0.0, 0.0),
                           std::weak_equality::equivalent));
static_assert(complex_test(makeComplex<_Complex double>(0.0, 0.0),
                           makeComplex<_Complex double>(1.0, 0.0),
                           std::weak_equality::nonequivalent));
static_assert(complex_test(makeComplex<_Complex double>(0.0, 0.0),
                           makeComplex<_Complex double>(0.0, 1.0),
                           std::weak_equality::nonequivalent));
static_assert(complex_test(makeComplex<_Complex int>(0, 0),
                           makeComplex<_Complex int>(0, 0),
                           std::strong_equality::equal));
static_assert(complex_test(makeComplex<_Complex int>(0, 0),
                           makeComplex<_Complex int>(1, 0),
                           std::strong_equality::nonequal));
// TODO: defaulted operator <=>
} // namespace ThreeWayComparison

constexpr bool for_range_init() {
  int k = 0;
  for (int arr[3] = {1, 2, 3}; int n : arr) k += n;
  return k == 6;
}
static_assert(for_range_init());

namespace Virtual {
  struct NonZeroOffset { int padding = 123; };

  constexpr void assert(bool b) { if (!b) throw 0; }

  // Ensure that we pick the right final overrider during construction.
  struct A {
    virtual constexpr char f() const { return 'A'; }
    char a = f();
    constexpr ~A() { assert(f() == 'A'); }
  };
  struct NoOverrideA : A {};
  struct B : NonZeroOffset, NoOverrideA {
    virtual constexpr char f() const { return 'B'; }
    char b = f();
    constexpr ~B() { assert(f() == 'B'); }
  };
  struct NoOverrideB : B {};
  struct C : NonZeroOffset, A {
    virtual constexpr char f() const { return 'C'; }
    A *pba;
    char c = ((A*)this)->f();
    char ba = pba->f();
    constexpr C(A *pba) : pba(pba) {}
    constexpr ~C() { assert(f() == 'C'); }
  };
  struct D : NonZeroOffset, NoOverrideB, C { // expected-warning {{inaccessible}}
    virtual constexpr char f() const { return 'D'; }
    char d = f();
    constexpr D() : C((B*)this) {}
    constexpr ~D() { assert(f() == 'D'); }
  };
  constexpr int n = (D(), 0);
  constexpr D d;
  static_assert(((B&)d).a == 'A');
  static_assert(((C&)d).a == 'A');
  static_assert(d.b == 'B');
  static_assert(d.c == 'C');
  // During the construction of C, the dynamic type of B's A is B.
  static_assert(d.ba == 'B');
  static_assert(d.d == 'D');
  static_assert(d.f() == 'D');
  constexpr const A &a = (B&)d;
  constexpr const B &b = d;
  static_assert(a.f() == 'D');
  static_assert(b.f() == 'D');

  // FIXME: It is unclear whether this should be permitted.
  D d_not_constexpr;
  static_assert(d_not_constexpr.f() == 'D'); // expected-error {{constant expression}} expected-note {{virtual function called on object 'd_not_constexpr' whose dynamic type is not constant}}

  // Check that we apply a proper adjustment for a covariant return type.
  struct Covariant1 {
    D d;
    virtual const A *f() const;
  };
  template<typename T>
  struct Covariant2 : Covariant1 {
    virtual const T *f() const;
  };
  template<typename T>
  struct Covariant3 : Covariant2<T> {
    constexpr virtual const D *f() const { return &this->d; }
  };

  constexpr Covariant3<B> cb;
  constexpr Covariant3<C> cc;

  constexpr const Covariant1 *cb1 = &cb;
  constexpr const Covariant2<B> *cb2 = &cb;
  static_assert(cb1->f()->a == 'A');
  static_assert(cb1->f() == (B*)&cb.d);
  static_assert(cb1->f()->f() == 'D');
  static_assert(cb2->f()->b == 'B');
  static_assert(cb2->f() == &cb.d);
  static_assert(cb2->f()->f() == 'D');

  constexpr const Covariant1 *cc1 = &cc;
  constexpr const Covariant2<C> *cc2 = &cc;
  static_assert(cc1->f()->a == 'A');
  static_assert(cc1->f() == (C*)&cc.d);
  static_assert(cc1->f()->f() == 'D');
  static_assert(cc2->f()->c == 'C');
  static_assert(cc2->f() == &cc.d);
  static_assert(cc2->f()->f() == 'D');

  static_assert(cb.f()->d == 'D');
  static_assert(cc.f()->d == 'D');

  struct Abstract {
    constexpr virtual void f() = 0; // expected-note {{declared here}}
    constexpr Abstract() { do_it(); } // expected-note {{in call to}}
    constexpr void do_it() { f(); } // expected-note {{pure virtual function 'Virtual::Abstract::f' called}}
  };
  struct PureVirtualCall : Abstract { void f(); }; // expected-note {{in call to 'Abstract}}
  constexpr PureVirtualCall pure_virtual_call; // expected-error {{constant expression}} expected-note {{in call to 'PureVirtualCall}}
}

namespace DynamicCast {
  struct A2 { virtual void a2(); };
  struct A : A2 { virtual void a(); };
  struct B : A {};
  struct C2 { virtual void c2(); };
  struct C : A, C2 { A *c = dynamic_cast<A*>(static_cast<C2*>(this)); };
  struct D { virtual void d(); };
  struct E { virtual void e(); };
  struct F : B, C, D, private E { void *f = dynamic_cast<void*>(static_cast<D*>(this)); };
  struct Padding { virtual void padding(); };
  struct G : Padding, F {};

  constexpr G g;

  // During construction of C, A is unambiguous subobject of dynamic type C.
  static_assert(g.c == (C*)&g);
  // ... but in the complete object, the same is not true, so the runtime fails.
  static_assert(dynamic_cast<const A*>(static_cast<const C2*>(&g)) == nullptr);

  // dynamic_cast<void*> produces a pointer to the object of the dynamic type.
  static_assert(g.f == (void*)(F*)&g);
  static_assert(dynamic_cast<const void*>(static_cast<const D*>(&g)) == &g);

  // expected-note@+1 {{reference dynamic_cast failed: 'DynamicCast::A' is an ambiguous base class of dynamic type 'DynamicCast::G' of operand}}
  constexpr int d_a = (dynamic_cast<const A&>(static_cast<const D&>(g)), 0); // expected-error {{}}

  // Can navigate from A2 to its A...
  static_assert(&dynamic_cast<A&>((A2&)(B&)g) == &(A&)(B&)g);
  // ... and from B to its A ...
  static_assert(&dynamic_cast<A&>((B&)g) == &(A&)(B&)g);
  // ... but not from D.
  // expected-note@+1 {{reference dynamic_cast failed: 'DynamicCast::A' is an ambiguous base class of dynamic type 'DynamicCast::G' of operand}}
  static_assert(&dynamic_cast<A&>((D&)g) == &(A&)(B&)g); // expected-error {{}}

  // Can cast from A2 to sibling class D.
  static_assert(&dynamic_cast<D&>((A2&)(B&)g) == &(D&)g);

  // Cannot cast from private base E to derived class F.
  // expected-note@+1 {{reference dynamic_cast failed: static type 'DynamicCast::E' of operand is a non-public base class of dynamic type 'DynamicCast::G'}}
  constexpr int e_f = (dynamic_cast<F&>((E&)g), 0); // expected-error {{}}

  // Cannot cast from B to private sibling E.
  // expected-note@+1 {{reference dynamic_cast failed: 'DynamicCast::E' is a non-public base class of dynamic type 'DynamicCast::G' of operand}}
  constexpr int b_e = (dynamic_cast<E&>((B&)g), 0); // expected-error {{}}

  struct Unrelated { virtual void unrelated(); };
  // expected-note@+1 {{reference dynamic_cast failed: dynamic type 'DynamicCast::G' of operand does not have a base class of type 'DynamicCast::Unrelated'}}
  constexpr int b_unrelated = (dynamic_cast<Unrelated&>((B&)g), 0); // expected-error {{}}
  // expected-note@+1 {{reference dynamic_cast failed: dynamic type 'DynamicCast::G' of operand does not have a base class of type 'DynamicCast::Unrelated'}}
  constexpr int e_unrelated = (dynamic_cast<Unrelated&>((E&)g), 0); // expected-error {{}}
}

namespace TypeId {
  struct A {
    const std::type_info &ti = typeid(*this);
  };
  struct A2 : A {};
  static_assert(&A().ti == &typeid(A));
  static_assert(&typeid((A2())) == &typeid(A2));
  extern A2 extern_a2;
  static_assert(&typeid(extern_a2) == &typeid(A2));

  constexpr A2 a2;
  constexpr const A &a1 = a2;
  static_assert(&typeid(a1) == &typeid(A));

  struct B {
    virtual void f();
    const std::type_info &ti1 = typeid(*this);
  };
  struct B2 : B {
    const std::type_info &ti2 = typeid(*this);
  };
  static_assert(&B2().ti1 == &typeid(B));
  static_assert(&B2().ti2 == &typeid(B2));
  extern B2 extern_b2;
  // expected-note@+1 {{typeid applied to object 'extern_b2' whose dynamic type is not constant}}
  static_assert(&typeid(extern_b2) == &typeid(B2)); // expected-error {{constant expression}}

  constexpr B2 b2;
  constexpr const B &b1 = b2;
  static_assert(&typeid(b1) == &typeid(B2));

  constexpr bool side_effects() {
    // Not polymorphic nor a glvalue.
    bool OK = true;
    (void)typeid(OK = false, A2()); // expected-warning {{has no effect}}
    if (!OK) return false;

    // Not polymorphic.
    A2 a2;
    (void)typeid(OK = false, a2); // expected-warning {{has no effect}}
    if (!OK) return false;

    // Not a glvalue.
    (void)typeid(OK = false, B2()); // expected-warning {{has no effect}}
    if (!OK) return false;

    // Polymorphic glvalue: operand evaluated.
    OK = false;
    B2 b2;
    (void)typeid(OK = true, b2); // expected-warning {{will be evaluated}}
    return OK;
  }
  static_assert(side_effects());
}

namespace Union {
  struct Base {
    int y; // expected-note 2{{here}}
  };
  struct A : Base {
    int x;
    int arr[3];
    union { int p, q; };
  };
  union B {
    A a;
    int b;
  };
  constexpr int read_wrong_member() { // expected-error {{never produces a constant}}
    B b = {.b = 1};
    return b.a.x; // expected-note {{read of member 'a' of union with active member 'b'}}
  }
  constexpr int change_member() {
    B b = {.b = 1};
    b.a.x = 1;
    return b.a.x;
  }
  static_assert(change_member() == 1);
  constexpr int change_member_then_read_wrong_member() { // expected-error {{never produces a constant}}
    B b = {.b = 1};
    b.a.x = 1;
    return b.b; // expected-note {{read of member 'b' of union with active member 'a'}}
  }
  constexpr int read_wrong_member_indirect() { // expected-error {{never produces a constant}}
    B b = {.b = 1};
    int *p = &b.a.y;
    return *p; // expected-note {{read of member 'a' of union with active member 'b'}}
  }
  constexpr int read_uninitialized() {
    B b = {.b = 1};
    int *p = &b.a.y;
    b.a.x = 1;
    return *p; // expected-note {{read of uninitialized object}}
  }
  static_assert(read_uninitialized() == 0); // expected-error {{constant}} expected-note {{in call}}
  constexpr void write_wrong_member_indirect() { // expected-error {{never produces a constant}}
    B b = {.b = 1};
    int *p = &b.a.y;
    *p = 1; // expected-note {{assignment to member 'a' of union with active member 'b'}}
  }
  constexpr int write_uninitialized() {
    B b = {.b = 1};
    int *p = &b.a.y;
    b.a.x = 1;
    *p = 1;
    return *p;
  }
  static_assert(write_uninitialized() == 1);
  constexpr int change_member_indirectly() {
    B b = {.b = 1};
    b.a.arr[1] = 1;
    int &r = b.a.y;
    r = 123;

    b.b = 2;
    b.a.y = 3;
    b.a.arr[2] = 4;
    return b.a.arr[2];
  }
  static_assert(change_member_indirectly() == 4);
  constexpr B return_uninit() {
    B b = {.b = 1};
    b.a.x = 2;
    return b;
  }
  constexpr B uninit = return_uninit(); // expected-error {{constant expression}} expected-note {{subobject of type 'int' is not initialized}}
  static_assert(return_uninit().a.x == 2);
  constexpr A return_uninit_struct() {
    B b = {.b = 1};
    b.a.x = 2;
    return b.a; // expected-note {{in call to 'A(b.a)'}} expected-note {{subobject of type 'int' is not initialized}}
  }
  // Note that this is rejected even though return_uninit() is accepted, and
  // return_uninit() copies the same stuff wrapped in a union.
  //
  // Copying a B involves copying the object representation of the union, but
  // copying an A invokes a copy constructor that copies the object
  // elementwise, and reading from b.a.y is undefined.
  static_assert(return_uninit_struct().x == 2); // expected-error {{constant expression}} expected-note {{in call}}
  constexpr B return_init_all() {
    B b = {.b = 1};
    b.a.x = 2;
    b.a.y = 3;
    b.a.arr[0] = 4;
    b.a.arr[1] = 5;
    b.a.arr[2] = 6;
    return b;
  }
  static_assert(return_init_all().a.x == 2);
  static_assert(return_init_all().a.y == 3);
  static_assert(return_init_all().a.arr[0] == 4);
  static_assert(return_init_all().a.arr[1] == 5);
  static_assert(return_init_all().a.arr[2] == 6);
  static_assert(return_init_all().a.p == 7); // expected-error {{}} expected-note {{read of member 'p' of union with no active member}}
  static_assert(return_init_all().a.q == 8); // expected-error {{}} expected-note {{read of member 'q' of union with no active member}}
  constexpr B init_all = return_init_all();

  constexpr bool test_no_member_change =  []{
    union U { char dummy = {}; };
    U u1;
    U u2;
    u1 = u2;
    return true;
  }();

  struct S1 {
    int n;
  };
  struct S2 : S1 {};
  struct S3 : S2 {};
  void f() {
    S3 s;
    s.n = 0;
  }
}

namespace TwosComplementShifts {
  using uint32 = __UINT32_TYPE__;
  using int32 = __INT32_TYPE__;
  static_assert(uint32(int32(0x1234) << 16) == 0x12340000);
  static_assert(uint32(int32(0x1234) << 19) == 0x91a00000);
  static_assert(uint32(int32(0x1234) << 20) == 0x23400000); // expected-warning {{requires 34 bits}}
  static_assert(uint32(int32(0x1234) << 24) == 0x34000000); // expected-warning {{requires 38 bits}}
  static_assert(uint32(int32(-1) << 31) == 0x80000000);

  static_assert(-1 >> 1 == -1);
  static_assert(-1 >> 31 == -1);
  static_assert(-2 >> 1 == -1);
  static_assert(-3 >> 1 == -2);
  static_assert(-4 >> 1 == -2);
}

namespace Uninit {
  constexpr int f(bool init) {
    int a;
    if (init)
      a = 1;
    return a; // expected-note {{read of uninitialized object}}
  }
  static_assert(f(true) == 1);
  static_assert(f(false) == 1); // expected-error {{constant expression}} expected-note {{in call}}

  struct X {
    int n; // expected-note {{declared here}}
    constexpr X(bool init) {
      if (init) n = 123;
    }
  };
  constinit X x1(true);
  constinit X x2(false); // expected-error {{constant initializer}} expected-note {{constinit}} expected-note {{subobject of type 'int' is not initialized}}

  struct Y {
    struct Z { int n; }; // expected-note {{here}}
    Z z1;
    Z z2;
    Z z3;
    // OK: the lifetime of z1 (and its members) start before the initializer of
    // z2 runs.
    constexpr Y() : z2{ (z1.n = 1, z1.n + 1) } { z3.n = 3; }
    // Not OK: z3 is not in its lifetime when the initializer of z2 runs.
    constexpr Y(int) : z2{
      (z3.n = 1, // expected-note {{assignment to object outside its lifetime}}
       z3.n + 1) // expected-warning {{uninitialized}}
    } { z1.n = 3; }
    constexpr Y(int, int) : z2{} {}
  };
  // FIXME: This is working around clang not implementing DR2026. With that
  // fixed, we should be able to test this without the injected copy.
  constexpr Y copy(Y y) { return y; } // expected-note {{in call to 'Y(y)'}} expected-note {{subobject of type 'int' is not initialized}}
  constexpr Y y1 = copy(Y());
  static_assert(y1.z1.n == 1 && y1.z2.n == 2 && y1.z3.n == 3);

  constexpr Y y2 = copy(Y(0)); // expected-error {{constant expression}} expected-note {{in call}}

  static_assert(Y(0,0).z2.n == 0);
  static_assert(Y(0,0).z1.n == 0); // expected-error {{constant expression}} expected-note {{read of uninitialized object}}
  static_assert(Y(0,0).z3.n == 0); // expected-error {{constant expression}} expected-note {{read of uninitialized object}}

  static_assert(copy(Y(0,0)).z2.n == 0); // expected-error {{constant expression}} expected-note {{in call}}

  constexpr unsigned char not_even_unsigned_char() {
    unsigned char c;
    return c; // expected-note {{read of uninitialized object}}
  }
  constexpr unsigned char x = not_even_unsigned_char(); // expected-error {{constant expression}} expected-note {{in call}}

  constexpr int switch_var(int n) {
    switch (n) {
    case 1:
      int a;
      a = n;
      return a;

    case 2:
      a = n;
      return a;
    }
  }
  constexpr int s1 = switch_var(1);
  constexpr int s2 = switch_var(2);
  static_assert(s1 == 1 && s2 == 2);

  constexpr bool switch_into_init_stmt() {
    switch (1) {
      if (int n; false) {
        for (int m; false;) {
        case 1:
          n = m = 1;
          return n == 1 && m == 1;
        }
      }
    }
  }
  static_assert(switch_into_init_stmt());
}

namespace dtor {
  void lifetime_extension() {
    struct X { constexpr ~X() {} };
    X &&a = X();
  }

  template<typename T> constexpr T &&ref(T &&t) { return (T&&)t; }

  struct Buf {
    char buf[64];
    int n = 0;
    constexpr void operator+=(char c) { buf[n++] = c; }
    constexpr bool operator==(const char *str) const {
      return str[n] == 0 && __builtin_memcmp(str, buf, n) == 0;
    }
    constexpr bool operator!=(const char *str) const { return !operator==(str); }
  };

  struct A {
    constexpr A(Buf &buf, char c) : buf(buf), c(c) { buf += c; }
    constexpr ~A() { buf += c; }
    constexpr operator bool() const { return true; }
    Buf &buf;
    char c;
  };

  constexpr bool dtor_calls_dtor() {
    union U {
      constexpr U(Buf &buf) : u(buf, 'u') { buf += 'U'; }
      constexpr ~U() { u.buf += 'U'; }
      A u, v;
    };

    struct B : A {
      A c, &&d, e;
      union {
        A f;
      };
      U u;
      constexpr B(Buf &buf)
          : A(buf, 'a'), c(buf, 'c'), d(ref(A(buf, 'd'))), e(A(buf, 'e')), f(buf, 'f'), u(buf) {
        buf += 'b';
      }
      constexpr ~B() {
        buf += 'b';
      }
    };

    Buf buf;
    {
      B b(buf);
      if (buf != "acddefuUb")
        return false;
    }
    if (buf != "acddefuUbbUeca")
      return false;
    return true;
  }
  static_assert(dtor_calls_dtor());

  constexpr void abnormal_termination(Buf &buf) {
    struct Indestructible {
      constexpr ~Indestructible(); // not defined
    };

    A a(buf, 'a');
    A(buf, 'b');
    int n = 0;
    for (A &&c = A(buf, 'c'); A d = A(buf, 'd'); A(buf, 'e')) {
      switch (A f(buf, 'f'); A g = A(buf, 'g')) { // expected-warning {{boolean}}
      case false: {
        A x(buf, 'x');
      }

      case true: {
        A h(buf, 'h');
        switch (n++) {
        case 0:
          break;
        case 1:
          continue;
        case 2:
          return;
        }
        break;
      }

      default:
        Indestructible indest;
      }

      A j = (A(buf, 'i'), A(buf, 'j'));
    }
  }

  constexpr bool check_abnormal_termination() {
    Buf buf = {};
    abnormal_termination(buf);
    return buf ==
      "abbc"
        "dfgh" /*break*/ "hgfijijeed"
        "dfgh" /*continue*/ "hgfeed"
        "dfgh" /*return*/ "hgfd"
      "ca";
  }
  static_assert(check_abnormal_termination());

  constexpr bool run_dtors_on_array_filler() {
    struct S {
      int times_destroyed = 0;
      constexpr ~S() { if (++times_destroyed != 1) throw "oops"; }
    };
    S s[3];
    return true;
  }
  static_assert(run_dtors_on_array_filler());
}

namespace dynamic_alloc {
  constexpr int *p = // expected-error {{constant}} expected-note {{pointer to heap-allocated object is not a constant expression}}
    new int; // expected-note {{heap allocation performed here}}

  constexpr int f(int n) {
    int *p = new int[n];
    for (int i = 0; i != n; ++i) {
      p[i] = i;
    }
    int k = 0;
    for (int i = 0; i != n; ++i) {
      k += p[i];
    }
    delete[] p;
    return k;
  }
  static_assert(f(123) == 123 * 122 / 2);

  constexpr bool nvdtor() { // expected-error {{never produces a constant expression}}
    struct S {
      constexpr ~S() {}
    };
    struct T : S {};
    delete (S*)new T; // expected-note {{delete of object with dynamic type 'T' through pointer to base class type 'S' with non-virtual destructor}}
    return true;
  }

  constexpr int vdtor_1() {
    int a;
    struct S {
      constexpr S(int *p) : p(p) {}
      constexpr virtual ~S() { *p = 1; }
      int *p;
    };
    struct T : S {
      // implicit destructor defined eagerly because it is constexpr and virtual
      using S::S;
    };
    delete (S*)new T(&a);
    return a;
  }
  static_assert(vdtor_1() == 1);

  constexpr int vdtor_2() {
    int a = 0;
    struct S { constexpr virtual ~S() {} };
    struct T : S {
      constexpr T(int *p) : p(p) {}
      constexpr ~T() { ++*p; }
      int *p;
    };
    S *p = new T{&a};
    delete p;
    return a;
  }
  static_assert(vdtor_2() == 1);

  constexpr int vdtor_3(int mode) {
    int a = 0;
    struct S { constexpr virtual ~S() {} };
    struct T : S {
      constexpr T(int *p) : p(p) {}
      constexpr ~T() { ++*p; }
      int *p;
    };
    S *p = new T[3]{&a, &a, &a}; // expected-note 2{{heap allocation}}
    switch (mode) {
    case 0:
      delete p; // expected-note {{non-array delete used to delete pointer to array object of type 'T [3]'}}
      break;
    case 1:
      // FIXME: This diagnosic isn't great; we should mention the cast to S*
      // somewhere in here.
      delete[] p; // expected-note {{delete of pointer to subobject '&{*new T [3]#0}[0]'}}
      break;
    case 2:
      delete (T*)p; // expected-note {{non-array delete used to delete pointer to array object of type 'T [3]'}}
      break;
    case 3:
      delete[] (T*)p;
      break;
    }
    return a;
  }
  static_assert(vdtor_3(0) == 3); // expected-error {{}} expected-note {{in call}}
  static_assert(vdtor_3(1) == 3); // expected-error {{}} expected-note {{in call}}
  static_assert(vdtor_3(2) == 3); // expected-error {{}} expected-note {{in call}}
  static_assert(vdtor_3(3) == 3);

  constexpr void delete_mismatch() { // expected-error {{never produces a constant expression}}
    delete[] // expected-note {{array delete used to delete pointer to non-array object of type 'int'}}
      new int; // expected-note {{allocation}}
  }

  template<typename T>
  constexpr T dynarray(int elems, int i) {
    T *p;
    if constexpr (sizeof(T) == 1)
      p = new T[elems]{"fox"}; // expected-note {{evaluated array bound 3 is too small to hold 4 explicitly initialized elements}}
    else
      p = new T[elems]{1, 2, 3}; // expected-note {{evaluated array bound 2 is too small to hold 3 explicitly initialized elements}}
    T n = p[i]; // expected-note 4{{past-the-end}}
    delete [] p;
    return n;
  }
  static_assert(dynarray<int>(4, 0) == 1);
  static_assert(dynarray<int>(4, 1) == 2);
  static_assert(dynarray<int>(4, 2) == 3);
  static_assert(dynarray<int>(4, 3) == 0);
  static_assert(dynarray<int>(4, 4) == 0); // expected-error {{constant expression}} expected-note {{in call}}
  static_assert(dynarray<int>(3, 2) == 3);
  static_assert(dynarray<int>(3, 3) == 0); // expected-error {{constant expression}} expected-note {{in call}}
  static_assert(dynarray<int>(2, 1) == 0); // expected-error {{constant expression}} expected-note {{in call}}
  static_assert(dynarray<char>(5, 0) == 'f');
  static_assert(dynarray<char>(5, 1) == 'o');
  static_assert(dynarray<char>(5, 2) == 'x');
  static_assert(dynarray<char>(5, 3) == 0); // (from string)
  static_assert(dynarray<char>(5, 4) == 0); // (from filler)
  static_assert(dynarray<char>(5, 5) == 0); // expected-error {{constant expression}} expected-note {{in call}}
  static_assert(dynarray<char>(4, 0) == 'f');
  static_assert(dynarray<char>(4, 1) == 'o');
  static_assert(dynarray<char>(4, 2) == 'x');
  static_assert(dynarray<char>(4, 3) == 0);
  static_assert(dynarray<char>(4, 4) == 0); // expected-error {{constant expression}} expected-note {{in call}}
  static_assert(dynarray<char>(3, 2) == 'x'); // expected-error {{constant expression}} expected-note {{in call}}

  constexpr bool run_dtors_on_array_filler() {
    struct S {
      int times_destroyed = 0;
      constexpr ~S() { if (++times_destroyed != 1) throw "oops"; }
    };
    delete[] new S[3];
    return true;
  }
  static_assert(run_dtors_on_array_filler());

  constexpr bool erroneous_array_bound(long long n) {
    delete[] new int[n]; // expected-note {{array bound -1 is negative}} expected-note {{array bound 4611686018427387904 is too large}}
    return true;
  }
  static_assert(erroneous_array_bound(3));
  static_assert(erroneous_array_bound(0));
  static_assert(erroneous_array_bound(-1)); // expected-error {{constant expression}} expected-note {{in call}}
  static_assert(erroneous_array_bound(1LL << 62)); // expected-error {{constant expression}} expected-note {{in call}}

  constexpr bool erroneous_array_bound_nothrow(long long n) {
    int *p = new (std::nothrow) int[n];
    bool result = p != 0;
    delete[] p;
    return result;
  }
  static_assert(erroneous_array_bound_nothrow(3));
  static_assert(erroneous_array_bound_nothrow(0));
  static_assert(!erroneous_array_bound_nothrow(-1));
  static_assert(!erroneous_array_bound_nothrow(1LL << 62));

  constexpr bool evaluate_nothrow_arg() {
    bool ok = false;
    delete new ((ok = true, std::nothrow)) int;
    return ok;
  }
  static_assert(evaluate_nothrow_arg());

  constexpr void double_delete() { // expected-error {{never produces a constant expression}}
    int *p = new int;
    delete p;
    delete p; // expected-note {{delete of pointer that has already been deleted}}
  }
  constexpr bool super_secret_double_delete() {
    struct A {
      constexpr ~A() { delete this; } // expected-note {{destruction of object that is already being destroyed}} expected-note {{in call}}
    };
    delete new A; // expected-note {{in call}}
    return true;
  }
  static_assert(super_secret_double_delete()); // expected-error {{constant expression}} expected-note {{in call}}

  constexpr void use_after_free() { // expected-error {{never produces a constant expression}}
    int *p = new int;
    delete p;
    *p = 1; // expected-note {{assignment to heap allocated object that has been deleted}}
  }
  constexpr void use_after_free_2() { // expected-error {{never produces a constant expression}}
    struct X { constexpr void f() {} };
    X *p = new X;
    delete p;
    p->f(); // expected-note {{member call on heap allocated object that has been deleted}}
  }

  template<typename T> struct X {
    std::size_t n;
    char *p;
    void dependent();
  };
  template<typename T> void X<T>::dependent() {
    char *p;
    // Ensure that we don't try to evaluate these for overflow and crash. These
    // are all value-dependent expressions.
    p = new char[n];
    p = new (n) char[n];
    p = new char(n);
  }
}

struct placement_new_arg {};
void *operator new(std::size_t, placement_new_arg);
void operator delete(void*, placement_new_arg);

namespace placement_new_delete {
  struct ClassSpecificNew {
    void *operator new(std::size_t);
  };
  struct ClassSpecificDelete {
    void operator delete(void*);
  };
  struct DestroyingDelete {
    void operator delete(DestroyingDelete*, std::destroying_delete_t);
  };
  struct alignas(64) Overaligned {};

  constexpr bool ok() {
    delete new Overaligned;
    delete ::new ClassSpecificNew;
    ::delete new ClassSpecificDelete;
    ::delete new DestroyingDelete;
    return true;
  }
  static_assert(ok());

  constexpr bool bad(int which) {
    switch (which) {
    case 0:
      delete new (placement_new_arg{}) int; // expected-note {{call to placement 'operator new'}}
      break;

    case 1:
      delete new ClassSpecificNew; // expected-note {{call to class-specific 'operator new'}}
      break;

    case 2:
      delete new ClassSpecificDelete; // expected-note {{call to class-specific 'operator delete'}}
      break;

    case 3:
      delete new DestroyingDelete; // expected-note {{call to class-specific 'operator delete'}}
      break;

    case 4:
      // FIXME: This technically follows the standard's rules, but it seems
      // unreasonable to expect implementations to support this.
      delete new (std::align_val_t{64}) Overaligned; // expected-note {{placement new expression is not yet supported}}
      break;
    }

    return true;
  }
  static_assert(bad(0)); // expected-error {{constant expression}} expected-note {{in call}}
  static_assert(bad(1)); // expected-error {{constant expression}} expected-note {{in call}}
  static_assert(bad(2)); // expected-error {{constant expression}} expected-note {{in call}}
  static_assert(bad(3)); // expected-error {{constant expression}} expected-note {{in call}}
  static_assert(bad(4)); // expected-error {{constant expression}} expected-note {{in call}}
}

namespace delete_random_things {
  static_assert((delete new int, true));
  static_assert((delete (int*)0, true));
  int n; // expected-note {{declared here}}
  static_assert((delete &n, true)); // expected-error {{}} expected-note {{delete of pointer '&n' that does not point to a heap-allocated object}}
  struct A { int n; };
  static_assert((delete &(new A)->n, true)); // expected-error {{}} expected-note {{delete of pointer to subobject '&{*new delete_random_things::A#0}.n'}}
  static_assert((delete (new int + 1), true)); // expected-error {{}} expected-note {{delete of pointer '&{*new int#0} + 1' that does not point to complete object}}
  static_assert((delete[] (new int[3] + 1), true)); // expected-error {{}} expected-note {{delete of pointer to subobject '&{*new int [3]#0}[1]'}}
  static_assert((delete &(int&)(int&&)0, true)); // expected-error {{}} expected-note {{delete of pointer '&0' that does not point to a heap-allocated object}} expected-note {{temporary created here}}
}

namespace memory_leaks {
  static_assert(*new bool(true)); // expected-error {{}} expected-note {{allocation performed here was not deallocated}}

  constexpr bool *f() { return new bool(true); } // expected-note {{allocation performed here was not deallocated}}
  static_assert(*f()); // expected-error {{}}

  struct UP {
    bool *p;
    constexpr ~UP() { delete p; }
    constexpr bool &operator*() { return *p; }
  };
  constexpr UP g() { return {new bool(true)}; }
  static_assert(*g()); // ok

  constexpr bool h(UP p) { return *p; }
  static_assert(h({new bool(true)})); // ok
}
