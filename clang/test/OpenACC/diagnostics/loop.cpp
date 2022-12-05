// Check "acc loop" diagnostic cases not covered by loop.c because they are
// specific to C++.
//
// When PARENT=PARENT_ORPHANED, this file checks diagnostics for "acc loop"
// outside of any parent OpenACC construct.  Various diagnostics involving both
// "acc loop" and "acc routine" are checked in diagnostics/routine.c.
//
// When PARENT=PARENT_SEPARATE, it checks diagnostics for "acc loop" enclosed
// within "acc parallel".
//
// When PARENT=PARENT_COMBINED, it combines those "acc parallel" and "acc loop"
// directives in order to check the same diagnostics but for combined "acc
// parallel loop" directives.

// RUN: %clang_cc1 -fopenacc -Wno-openacc-and-cxx %s                           \
// RUN:   -DPARENT=PARENT_ORPHANED -verify=expected,orph,orph-sep
// RUN: %clang_cc1 -fopenacc -Wno-openacc-and-cxx %s                           \
// RUN:   -DPARENT=PARENT_SEPARATE -verify=expected,sep,orph-sep,sep-cmb
// RUN: %clang_cc1 -fopenacc -Wno-openacc-and-cxx %s                           \
// RUN:   -DPARENT=PARENT_COMBINED -verify=expected,cmb,sep-cmb

// END.

#include <limits.h>
#include <stdint.h>

#define PARENT_ORPHANED 1
#define PARENT_SEPARATE 2
#define PARENT_COMBINED 3

#if PARENT == PARENT_ORPHANED || PARENT == PARENT_SEPARATE
# define CMB_PAR
#elif PARENT == PARENT_COMBINED
# define CMB_PAR parallel
#else
# error undefined PARENT
#endif

struct S {
  int i = 0;
  int fn();
};

class C {
public:
  int i = 0;
  const int constMember = 0; // #C_constMember
  int fn();
  S s;
};

class Main {
  int thisMember = 0;
  const int thisConstMember = 0; // #Main_thisConstMember
  S thisStructMember;
  void thisMemberFn();

  //----------------------------------------------------------------------------
  // Associated for loop
  //----------------------------------------------------------------------------

#if PARENT == PARENT_ORPHANED
  #pragma acc routine gang
#endif
  void testAssociatedForLoop() {
    class C local;

#if PARENT == PARENT_SEPARATE
    #pragma acc parallel
#endif
    {
      // Clang currently diagnoses most invalid OpenACC loop control variable
      // expressions with a generic OpenMP canonical form diagnostic (see
      // loop.c), but it has a specific OpenACC diagnostic for member
      // expressions.  Eventually, more cases should become OpenACC diagnostics.

      // expected-error-re@+2 {{as OpenACC loop control variable, member expression is not supported{{$}}}}
      #pragma acc CMB_PAR loop independent gang
      for (local.i = 0; local.i < 5; ++local.i)
        ;
      // expected-error-re@+2 {{as OpenACC loop control variable, member expression is not supported{{$}}}}
      #pragma acc CMB_PAR loop independent gang
      for (this->thisMember = 0; this->thisMember < 5; ++this->thisMember)
        ;
      // expected-error-re@+2 {{as OpenACC loop control variable, member expression is not supported even on implicit C++ 'this' pointer{{$}}}}
      #pragma acc CMB_PAR loop independent gang
      for (thisMember = 0; thisMember < 5; ++thisMember)
        ;
    }
  }

  //----------------------------------------------------------------------------
  // Data clauses: arg semantics in non-const member function.
  //----------------------------------------------------------------------------

#if PARENT == PARENT_ORPHANED
  #pragma acc routine gang
#endif
  void testInNonConstMember() {
    class C local;
    const class C constLocal; // #testInNonConstMember_constLocal

    //..........................................................................
    // Member expression not permitted: data member.

#if PARENT == PARENT_SEPARATE
    #pragma acc parallel copy(thisMember)
#endif
    {
      // orph-sep-error-re@+4 {{in 'private' clause on '#pragma acc loop', member expression is not supported{{$}}}}
      // orph-sep-error-re@+4 {{in 'private' clause on '#pragma acc loop', member expression is not supported{{$}}}}
      // cmb-error-re@+2 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
      // cmb-error-re@+2 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
      #pragma acc CMB_PAR loop gang private(local.i) \
                                    private(this->thisMember)
      for (int i = 0; i < 5; ++i)
        ;
      // orph-sep-error-re@+2 {{in 'private' clause on '#pragma acc loop', member expression is not supported even on implicit C++ 'this' pointer{{$}}}}
      // cmb-error-re@+1 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported even on implicit C++ 'this' pointer{{$}}}}
      #pragma acc CMB_PAR loop gang private(thisMember)
      for (int i = 0; i < 5; ++i)
        ;
      // orph-sep-error-re@+2 {{in 'reduction' clause on '#pragma acc loop', member expression is not supported except on C++ 'this' pointer{{$}}}}
      // cmb-error-re@+1 {{in 'reduction' clause on '#pragma acc parallel loop', member expression is not supported except on C++ 'this' pointer{{$}}}}
      #pragma acc CMB_PAR loop gang reduction(+:local.i)
      for (int i = 0; i < 5; ++i)
        ;
      // It's not generally possible to determine if 'this' is private.
      // orph-error-re@+1 {{orphaned loop reduction variable 'Main::thisMember' is not gang-private{{$}}}}
      #pragma acc CMB_PAR loop gang reduction(*:this->thisMember)
      for (int i = 0; i < 5; ++i)
        ;
      // orph-error-re@+1 {{orphaned loop reduction variable 'Main::thisMember' is not gang-private{{$}}}}
      #pragma acc CMB_PAR loop gang reduction(*:thisMember)
      for (int i = 0; i < 5; ++i)
        ;
    }

    //..........................................................................
    // Member expression not permitted: member function.

#if PARENT == PARENT_SEPARATE
    #pragma acc parallel
#endif
    {
      // orph-sep-error-re@+3 {{in 'reduction' clause on '#pragma acc loop', member expression is not supported except on C++ 'this' pointer{{$}}}}
      // cmb-error-re@+2 {{in 'reduction' clause on '#pragma acc parallel loop', member expression is not supported except on C++ 'this' pointer{{$}}}}
      // expected-error@+2 {{expected data member}}
      #pragma acc CMB_PAR loop worker reduction(+:local.    \
                                                        fn  \
                                                          )
      for (int i = 0; i < 5; ++i)
        ;
      // expected-error@+2 {{expected data member}}
      #pragma acc CMB_PAR loop worker reduction(+:this->             \
                                                        thisMemberFn \
                                                                    )
      for (int i = 0; i < 5; ++i)
        ;
      // expected-error-re@+1 {{expected variable name or data member expression{{$}}}}
      #pragma acc CMB_PAR loop worker reduction(+:thisMemberFn)
      for (int i = 0; i < 5; ++i)
        ;

      // orph-sep-error-re@+3 {{in 'private' clause on '#pragma acc loop', member expression is not supported{{$}}}}
      // cmb-error-re@+2 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
      // expected-error@+2 {{expected data member}}
      #pragma acc CMB_PAR loop vector private(local.    \
                                                    fn  \
                                                      )
      for (int i = 0; i < 5; ++i)
        ;
      // orph-sep-error-re@+3 {{in 'private' clause on '#pragma acc loop', member expression is not supported{{$}}}}
      // cmb-error-re@+2 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
      // expected-error@+2 {{expected data member}}
      #pragma acc CMB_PAR loop vector private(this->             \
                                                    thisMemberFn \
                                                                )
      for (int i = 0; i < 5; ++i)
        ;
      // orph-sep-error-re@+3 {{in 'private' clause on '#pragma acc loop', member expression is not supported even on implicit C++ 'this' pointer{{$}}}}
      // cmb-error-re@+2 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported even on implicit C++ 'this' pointer{{$}}}}
      // expected-error-re@+1 {{expected variable name{{$}}}}
      #pragma acc CMB_PAR loop vector private(thisMemberFn)
      for (int i = 0; i < 5; ++i)
        ;
    }

    //..........................................................................
    // Member expression not permitted: member function call.

#if PARENT == PARENT_SEPARATE
    #pragma acc parallel
#endif
    {
      // orph-sep-error-re@+3 {{in 'reduction' clause on '#pragma acc loop', member expression is not supported except on C++ 'this' pointer{{$}}}}
      // cmb-error-re@+2 {{in 'reduction' clause on '#pragma acc parallel loop', member expression is not supported except on C++ 'this' pointer{{$}}}}
      // expected-error@+2 {{expected data member}}
      #pragma acc CMB_PAR loop gang reduction(+:local.      \
                                                      fn    \
                                                        ())
      for (int i = 0; i < 5; ++i)
        ;
      // expected-error@+2 {{expected data member}}
      #pragma acc CMB_PAR loop gang reduction(+:this->                \
                                                      thisMemberFn    \
                                                                  ())
      for (int i = 0; i < 5; ++i)
        ;
      // expected-error-re@+2 {{expected variable name or data member expression{{$}}}}
      #pragma acc CMB_PAR loop gang reduction(+:                \
                                                thisMemberFn    \
                                                            ())
      for (int i = 0; i < 5; ++i)
        ;

      // orph-sep-error-re@+3 {{in 'private' clause on '#pragma acc loop', member expression is not supported{{$}}}}
      // cmb-error-re@+2 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
      // expected-error@+2 {{expected data member}}
      #pragma acc CMB_PAR loop gang private(local.      \
                                                  fn    \
                                                    ())
      for (int i = 0; i < 5; ++i)
        ;
      // orph-sep-error-re@+3 {{in 'private' clause on '#pragma acc loop', member expression is not supported{{$}}}}
      // cmb-error-re@+2 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
      // expected-error@+2 {{expected data member}}
      #pragma acc CMB_PAR loop gang private(this->                \
                                                  thisMemberFn    \
                                                              ())
      for (int i = 0; i < 5; ++i)
        ;
      // orph-sep-error-re@+4 {{in 'private' clause on '#pragma acc loop', member expression is not supported even on implicit C++ 'this' pointer{{$}}}}
      // cmb-error-re@+3 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported even on implicit C++ 'this' pointer{{$}}}}
      // expected-error-re@+2 {{expected variable name{{$}}}}
      #pragma acc CMB_PAR loop gang private(                \
                                            thisMemberFn    \
                                                        ())
      for (int i = 0; i < 5; ++i)
        ;
    }

    //..........................................................................
    // Nested member expressions not permitted: data member.

#if PARENT == PARENT_SEPARATE
    #pragma acc parallel copy(thisMember)
#endif
    {
      // expected-error-re@+1 {{nested member expression is not supported{{$}}}} // range is for local.s
      #pragma acc CMB_PAR loop reduction(+:local.s.i)
      for (int i = 0; i < 5; ++i)
        ;
      // expected-error-re@+1 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
      #pragma acc CMB_PAR loop reduction(+:this->thisStructMember.i)
      for (int i = 0; i < 5; ++i)
        ;
      // expected-error-re@+1 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
      #pragma acc CMB_PAR loop reduction(+:thisStructMember.i)
      for (int i = 0; i < 5; ++i)
        ;

      // orph-sep-error-re@+3 {{in 'private' clause on '#pragma acc loop', member expression is not supported{{$}}}}
      // cmb-error-re@+2 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
      // expected-error-re@+1 {{nested member expression is not supported{{$}}}} // range is for local.s
      #pragma acc CMB_PAR loop private(local.s.i)
      for (int i = 0; i < 5; ++i)
        ;
      // orph-sep-error-re@+3 {{in 'private' clause on '#pragma acc loop', member expression is not supported{{$}}}}
      // cmb-error-re@+2 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
      // expected-error-re@+1 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
      #pragma acc CMB_PAR loop private(this->thisStructMember.i)
      for (int i = 0; i < 5; ++i)
        ;
      // orph-sep-error-re@+3 {{in 'private' clause on '#pragma acc loop', member expression is not supported{{$}}}}
      // cmb-error-re@+2 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
      // expected-error-re@+1 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
      #pragma acc CMB_PAR loop private(thisStructMember.i)
      for (int i = 0; i < 5; ++i)
        ;
    }

    //..........................................................................
    // Nested member expressions not permitted: member function call.

#if PARENT == PARENT_SEPARATE
    #pragma acc parallel copy(thisMember)
#endif
    {
      // expected-error-re@+2 {{nested member expression is not supported{{$}}}} // range is for local.s
      // expected-error@+1 {{expected data member}}
      #pragma acc CMB_PAR loop reduction(+:local.s.fn())
      for (int i = 0; i < 5; ++i)
        ;
      // expected-error-re@+2 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
      // expected-error@+1 {{expected data member}}
      #pragma acc CMB_PAR loop reduction(+:this->thisStructMember.fn())
      for (int i = 0; i < 5; ++i)
        ;
      // expected-error-re@+2 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
      // expected-error@+1 {{expected data member}}
      #pragma acc CMB_PAR loop reduction(+:thisStructMember.fn())
      for (int i = 0; i < 5; ++i)
        ;

      // orph-sep-error-re@+4 {{in 'private' clause on '#pragma acc loop', member expression is not supported{{$}}}}
      // cmb-error-re@+3 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
      // expected-error-re@+2 {{nested member expression is not supported{{$}}}} // range is for local.s
      // expected-error@+1 {{expected data member}}
      #pragma acc CMB_PAR loop private(local.s.fn())
      for (int i = 0; i < 5; ++i)
        ;
      // orph-sep-error-re@+4 {{in 'private' clause on '#pragma acc loop', member expression is not supported{{$}}}}
      // cmb-error-re@+3 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
      // expected-error-re@+2 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
      // expected-error@+1 {{expected data member}}
      #pragma acc CMB_PAR loop private(this->thisStructMember.fn())
      for (int i = 0; i < 5; ++i)
        ;
      // orph-sep-error-re@+4 {{in 'private' clause on '#pragma acc loop', member expression is not supported{{$}}}}
      // cmb-error-re@+3 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
      // expected-error-re@+2 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
      // expected-error@+1 {{expected data member}}
      #pragma acc CMB_PAR loop private(thisStructMember.fn())
      for (int i = 0; i < 5; ++i)
        ;
    }

    //..........................................................................
    // Subarray not permitted: even though this[0:1] is special, it is still
    // treated as a subarray in this case.

#if PARENT == PARENT_SEPARATE
    #pragma acc parallel
#endif
    {
      // expected-error@+5 {{subarray syntax must include ':'}}
      // orph-sep-error@+3 {{in 'private' clause on '#pragma acc loop', subarray is not supported}}
      // cmb-error@+2 {{in 'private' clause on '#pragma acc parallel loop', subarray is not supported}}
      // expected-error-re@+1 {{expected variable name{{$}}}}
      #pragma acc CMB_PAR loop vector private(this[0   \
                                                    ]  \
                                                     )
      for (int i = 0; i < 5; ++i)
        ;
      // orph-sep-error@+8 {{in 'private' clause on '#pragma acc loop', subarray is not supported}}
      // orph-sep-error@+8 {{in 'private' clause on '#pragma acc loop', subarray is not supported}}
      // orph-sep-error@+8 {{subarray length is unspecified and cannot be inferred because subscripted value is not an array}}
      // orph-sep-error@+8 {{subarray length is unspecified and cannot be inferred because subscripted value is not an array}}
      // cmb-error@+4 {{in 'private' clause on '#pragma acc parallel loop', subarray is not supported}}
      // cmb-error@+4 {{in 'private' clause on '#pragma acc parallel loop', subarray is not supported}}
      // cmb-error@+4 {{subarray length is unspecified and cannot be inferred because subscripted value is not an array}}
      // cmb-error@+4 {{subarray length is unspecified and cannot be inferred because subscripted value is not an array}}
      #pragma acc CMB_PAR loop vector private(this[0:1], \
                                              this[:1],  \
                                              this[0:],  \
                                              this[:]) gang
      for (int i = 0; i < 5; ++i)
        ;
      // expected-error@+16 {{subarray syntax must include ':'}}
      // orph-sep-error@+11 {{in 'reduction' clause on '#pragma acc loop', subarray is not supported}}
      // orph-sep-error@+11 {{in 'reduction' clause on '#pragma acc loop', subarray is not supported}}
      // orph-sep-error@+11 {{subarray length is unspecified and cannot be inferred because subscripted value is not an array}}
      // orph-sep-error@+11 {{subarray length is unspecified and cannot be inferred because subscripted value is not an array}}
      // orph-sep-error@+11 {{in 'reduction' clause on '#pragma acc loop', subarray is not supported}}
      // cmb-error@+6 {{in 'reduction' clause on '#pragma acc parallel loop', subarray is not supported}}
      // cmb-error@+6 {{in 'reduction' clause on '#pragma acc parallel loop', subarray is not supported}}
      // cmb-error@+6 {{subarray length is unspecified and cannot be inferred because subscripted value is not an array}}
      // cmb-error@+6 {{subarray length is unspecified and cannot be inferred because subscripted value is not an array}}
      // cmb-error@+6 {{in 'reduction' clause on '#pragma acc parallel loop', subarray is not supported}}
      // expected-error-re@+5 {{expected variable name or data member expression{{$}}}}
      #pragma acc CMB_PAR loop vector reduction(*:this[0:1], \
                                                  this[:1],  \
                                                  this[0:],  \
                                                  this[:],   \
                                                  this[0]) worker
      for (int i = 0; i < 5; ++i)
        ;
    }

    //..........................................................................
    // Bare 'this' not permitted because it's not a variable/lvalue.

#if PARENT == PARENT_SEPARATE
    #pragma acc parallel
#endif
    {
      // expected-error-re@+1 {{expected variable name{{$}}}}
      #pragma acc CMB_PAR loop private(this)
      for (int i = 0; i < 5; ++i)
        ;
      // expected-error-re@+1 {{expected variable name or data member expression{{$}}}}
      #pragma acc CMB_PAR loop reduction(min:this)
      for (int i = 0; i < 5; ++i)
        ;
    }

    //..........................................................................
    // Conflicting DSAs.

#if PARENT == PARENT_SEPARATE
    #pragma acc parallel copy(thisMember)
#endif
    {
      // orph-error-re@+3 2 {{orphaned loop reduction variable 'Main::thisMember' is not gang-private{{$}}}}
      // expected-error@+2 {{redundant '+' reduction for variable 'Main::thisMember'}}
      // expected-note@+1 {{previous '+' reduction}}
      #pragma acc CMB_PAR loop reduction(+:this->thisMember) reduction(+:thisMember)
      for (int i = 0; i < 5; ++i)
        ;
      // orph-error-re@+3 2 {{orphaned loop reduction variable 'Main::thisMember' is not gang-private{{$}}}}
      // expected-error@+2 {{conflicting '*' reduction for variable 'Main::thisMember'}}
      // expected-note@+1 {{previous '+' reduction}}
      #pragma acc CMB_PAR loop reduction(+:this->thisMember) reduction(*:this->thisMember)
      for (int i = 0; i < 5; ++i)
        ;
    }

    //..........................................................................
    // Const variables in non-const member function.

#if PARENT == PARENT_SEPARATE
    #pragma acc parallel copy(thisMember, thisConstMember)
#endif
    {
      // Composite is non-const, member is non-const.

      // orph-error@+1 {{orphaned loop reduction variable 'Main::thisMember' is not gang-private}}
      #pragma acc CMB_PAR loop reduction(+:this->thisMember)
      for (int i = 0; i < 5; ++i)
        ;
      // orph-error@+1 {{orphaned loop reduction variable 'Main::thisMember' is not gang-private}}
      #pragma acc CMB_PAR loop reduction(+:thisMember)
      for (int i = 0; i < 5; ++i)
        ;

      // Composite is non-const, member is const.

      // expected-error@+2 2 {{const variable cannot be written by 'reduction' clause}}
      // expected-note@#Main_thisConstMember 2 {{variable 'Main::thisConstMember' declared const here}}
      #pragma acc CMB_PAR loop reduction(+:this->thisConstMember, thisConstMember)
      for (int i = 0; i < 5; ++i)
        ;
    }
  }

  //----------------------------------------------------------------------------
  // Data clauses: arg semantics in const member function.
  //----------------------------------------------------------------------------

#if PARENT == PARENT_ORPHANED
  #pragma acc routine gang
#endif
  void testInConstMember() const { // #Main_testInConstMember
    class C local;
    const class C constLocal; // #Main_testInConstMember_constLocal

    //..........................................................................
    // Const variables in const member function.

#if PARENT == PARENT_SEPARATE
    #pragma acc parallel copy(thisMember, thisConstMember)
#endif
    {
      // Composite is const, member is non-const.

      // expected-error@+2 2 {{const variable cannot be written by 'reduction' clause}}
      // expected-note@#Main_testInConstMember 2 {{variable is const within member function 'Main::testInConstMember' declared const here}}
      #pragma acc CMB_PAR loop reduction(+:this->thisMember, thisMember)
      for (int i = 0; i < 5; ++i)
        ;

      // Composite is const, member is const.

      // expected-error@+2 2 {{const variable cannot be written by 'reduction' clause}}
      // expected-note@#Main_thisConstMember 2 {{variable 'Main::thisConstMember' declared const here}}
      #pragma acc CMB_PAR loop reduction(+:this->thisConstMember, thisConstMember)
      for (int i = 0; i < 5; ++i)
        ;
    }
  }
};
