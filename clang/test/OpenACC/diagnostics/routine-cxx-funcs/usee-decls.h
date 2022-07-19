// This file contains declarations of the functions that are used in users.inc
// and whose routine directives appear in usee-defs.inc.

#ifndef FUNC_USEE_DECLS_INC
#define FUNC_USEE_DECLS_INC

// File-scope function we check in the usee role.
void fnUsee();

// Each of the following is used as a constructor parameter (thus, the
// constructor is not a default constructor) to indicate what the constructor is
// being used to test:
// - CtorInherited: For testing constructor inheritance via a using declaration
//   in a derived class.
// - CtorExplicitInit: For testing explicit initialization of base classes or
//   members.
// - CtorDelegator: For testing delegation to another constructor
struct CtorInherited {};
struct CtorExplicitInit {};
struct CtorDelegator {};

// Used as type for conversion operators for surrogate call functions.
typedef int (*FnPtr)(int);

// Class containing member functions we check in the usee role.
struct MemberUsees {
  MemberUsees(); // default ctor
  MemberUsees(MemberUsees &); // non-trivial copy ctor
  MemberUsees(CtorDelegator); // ctor delegator
  MemberUsees(CtorInherited); // inherited ctor
  ~MemberUsees(); // non-trivial dtor
  void *operator new(unsigned long s); // new operator
  void operator delete(void *p); // delete operator
  int operator-() const; // unary operator
  int operator+(int) const; // binary operator
  int operator[](int) const; // subscript operator
  int operator()() const; // call operator
  struct ArrowResult {
    struct ArrowResultInner {
      int x;
    } arrowResultInner;
    ArrowResultInner *operator->(); // arrow operator
  } arrowResult;
  ArrowResult operator->(); // chained arrow operator
  operator int() const; // conversion operator
  operator FnPtr() const; // conversion for surrogate call function
  int *begin(); // begin for range-based for loop
  int *end(); // end for range-based for loop
};

// Trivial class containing member functions we check in the usee role.
struct [[clang::trivial_abi]] MemberUseesTrivial {
  MemberUseesTrivial();
  MemberUseesTrivial(MemberUseesTrivial &); // trivial copy ctor
  ~MemberUseesTrivial(); // trivial dtor
};

#endif