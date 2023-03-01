// This file contains declarations of the functions that are used in users.inc
// and whose routine directives appear in usee-defs.inc.

#ifndef FUNC_USEE_DECLS_INC
#define FUNC_USEE_DECLS_INC

#ifndef USEE_ADD_DEF_TO_DECL
# error USEE_ADD_DEF_TO_DECL not defined
# define USEE_ADD_DEF_TO_DECL 1 // to pacify editors
#endif

#if USEE_ADD_DEF_TO_DECL
# ifndef USEE_ROUTINE_DIR
#  error USEE_ROUTINE_DIR not defined
#  define USEE_ROUTINE_DIR // to pacify editors
# endif
# define USEE_BODY(BODY) { BODY }
#else
# define USEE_BODY(BODY) ;
#endif

// File-scope and namespace functions we check in the usee role.
//
// They're normally defined in usee-defs.inc, but that's not included if
// USEE_ADD_DEF_TO_DECL.
//
// Each fnUseeIn*Lambda is meant to have only one user, a single lambda, so
// that we can be sure that user is sufficient to trigger certain diagnostics.
void fnUsee();
void fnUseeInNestedHostLambda();
void fnUseeInHostLambda();
void fnUseeInNestedAccRoutineLambda();
void fnUseeInAccRoutineLambda();
void fnUseeInAccParallelLambda();
void fnUseeInAccLoopLambda();
namespace NamespaceUsees { void fn(); }
#if USEE_ADD_DEF_TO_DECL
USEE_ROUTINE_DIR // #fnUsee_routine
void fnUsee() {}
USEE_ROUTINE_DIR // #fnUseeInNestedHostLambda_routine
void fnUseeInNestedHostLambda() {}
USEE_ROUTINE_DIR // #fnUseeInHostLambda_routine
void fnUseeInHostLambda() {}
USEE_ROUTINE_DIR // #fnUseeInNestedAccRoutineLambda_routine
void fnUseeInNestedAccRoutineLambda() {}
USEE_ROUTINE_DIR // #fnUseeInAccRoutineLambda_routine
void fnUseeInAccRoutineLambda() {}
USEE_ROUTINE_DIR // #fnUseeInAccParallelLambda_routine
void fnUseeInAccParallelLambda() {}
USEE_ROUTINE_DIR // #fnUseeInAccLoopLambda_routine
void fnUseeInAccLoopLambda() {}
namespace NamespaceUsees {
  USEE_ROUTINE_DIR // #NamespaceUsees_fn_routine
  void fn() {}
}
// Because lambdas currently cannot have explicit routine directives, this
// function is called within lambdas to imply their routine directives instead.
USEE_ROUTINE_DIR // #lambdaUseeUsee_routine
void lambdaUseeUsee() {}
auto lambdaUsee = []() {
  lambdaUseeUsee(); // #lambdaUsee_lambdaUseeUsee_call
};
#endif

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
#if USEE_ADD_DEF_TO_DECL
  USEE_ROUTINE_DIR // #MemberUsees_ctor_routine
#endif
  MemberUsees() USEE_BODY() // default ctor
#if USEE_ADD_DEF_TO_DECL
  USEE_ROUTINE_DIR // #MemberUsees_ctorCopy_routine
#endif
  MemberUsees(MemberUsees &) USEE_BODY() // non-trivial copy ctor
  MemberUsees(CtorDelegator); // ctor delegator
#if USEE_ADD_DEF_TO_DECL
  USEE_ROUTINE_DIR // #MemberUsees_ctorInherited_routine
#endif
  MemberUsees(CtorInherited) USEE_BODY() // inherited ctor
#if USEE_ADD_DEF_TO_DECL
  USEE_ROUTINE_DIR // #MemberUsees_dtor_routine
#endif
  ~MemberUsees() USEE_BODY() // non-trivial dtor
#if USEE_ADD_DEF_TO_DECL
  USEE_ROUTINE_DIR // #MemberUsees_new_routine
#endif
  void *operator new(unsigned long s) // new operator
  USEE_BODY(return (void*)1;) // fake address that won't produce a warning
#if USEE_ADD_DEF_TO_DECL
  USEE_ROUTINE_DIR // #MemberUsees_delete_routine
#endif
  void operator delete(void *p) USEE_BODY() // delete operator
#if USEE_ADD_DEF_TO_DECL
  USEE_ROUTINE_DIR // #MemberUsees_opUnary_routine
#endif
  int operator-() const USEE_BODY(return -1;) // unary operator
#if USEE_ADD_DEF_TO_DECL
  USEE_ROUTINE_DIR // #MemberUsees_opBinary_routine
#endif
  int operator+(int) const USEE_BODY(return 0;) // binary operator
#if USEE_ADD_DEF_TO_DECL
  USEE_ROUTINE_DIR // #MemberUsees_opSubscript_routine
#endif
  int operator[](int) const USEE_BODY(return 0;) // subscript operator
#if USEE_ADD_DEF_TO_DECL
  USEE_ROUTINE_DIR // #MemberUsees_opCall_routine
#endif
  int operator()() const USEE_BODY(return 0;) // call operator
  struct ArrowResult {
    struct ArrowResultInner {
      int x;
    } arrowResultInner;
#if USEE_ADD_DEF_TO_DECL
    USEE_ROUTINE_DIR // #MemberUsees_ArrowResult_opArrow_routine
#endif
    ArrowResultInner *operator->()
    USEE_BODY(return &arrowResultInner;) // arrow operator
  } arrowResult;
#if USEE_ADD_DEF_TO_DECL
  USEE_ROUTINE_DIR // #MemberUsees_opArrow_routine
#endif
  ArrowResult operator->()
  USEE_BODY(return arrowResult;) // chained arrow operator
#if USEE_ADD_DEF_TO_DECL
  USEE_ROUTINE_DIR // #MemberUsees_opConvert_routine
#endif
  operator int() const USEE_BODY(return 0;) // conversion operator
#if USEE_ADD_DEF_TO_DECL
  USEE_ROUTINE_DIR // #MemberUsees_opConvertSurrogate_routine
#endif
  operator FnPtr() const
  USEE_BODY(return 0;) // conversion for surrogate call function
#if USEE_ADD_DEF_TO_DECL
  USEE_ROUTINE_DIR // #MemberUsees_begin_routine
#endif
  int *begin() USEE_BODY(return nullptr;) // begin for range-based for loop
#if USEE_ADD_DEF_TO_DECL
  USEE_ROUTINE_DIR // #MemberUsees_end_routine
#endif
  int *end() USEE_BODY(return nullptr;) // end for range-based for loop
};

// Trivial class containing member functions we check in the usee role.
struct [[clang::trivial_abi]] MemberUseesTrivial {
#if USEE_ADD_DEF_TO_DECL
  USEE_ROUTINE_DIR // #MemberUseesTrivial_ctor_routine
#endif
  MemberUseesTrivial() USEE_BODY()
#if USEE_ADD_DEF_TO_DECL
  USEE_ROUTINE_DIR // #MemberUseesTrivial_ctorCopy_routine
#endif
  MemberUseesTrivial(MemberUseesTrivial &) USEE_BODY() // trivial copy ctor
#if USEE_ADD_DEF_TO_DECL
  USEE_ROUTINE_DIR // #MemberUseesTrivial_dtor_routine
#endif
  ~MemberUseesTrivial() USEE_BODY() // trivial dtor
};

#endif
