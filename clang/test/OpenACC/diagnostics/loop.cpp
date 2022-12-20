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

#if PARENT == PARENT_ORPHANED
#pragma acc routine gang
#endif
template <typename T>
void reductionPlusTmplTypeVar(T var) { // #reductionPlusTmplTypeVar_var
  // Any valid loop gang reduction (unless orphaned) must be copied to the
  // parallel construct.  When the template is instantiated with a T such that
  // '+' cannot be used as a reduction operator for var, Clang should diagnose
  // that mismatch exactly once, at the original gang loop reduction, which
  // Clang should then discard so that it never copies it to the parallel
  // construct.  The risk is that, if Clang copies the reduction while
  // processing the original template and does not ignore that copy after
  // diagnosing the problem with the loop reduction within the instantiation, it
  // might duplicate the diagnostic at the copied reduction.
#if PARENT == PARENT_SEPARATE
  #pragma acc parallel copy(var)
#endif
  #pragma acc CMB_PAR loop gang reduction(+: var) // #reductionPlusTmplTypeVar_reduction
  for (int i = 0; i < 5; ++i)
    var += i;
}

#if PARENT == PARENT_ORPHANED
#pragma acc routine gang
#endif
template <typename T>
void loopReductionNeedsDataClause(T var) { // #loopReductionNeedsDataClause_var
  // Where the template is instantiated such that T is a scalar, we should see a
  // diagnostic that var needs a data clause on the acc parallel.
  //
  // Where the template is instantiated such that T is not a scalar, we should
  // see a diagnostic that non-scalars are not yet supported in reductions.
  //
  // The template itself should produce neither diagnostic as it doesn't
  // determine whether T is a scalar.
#if PARENT == PARENT_SEPARATE
  #pragma acc parallel // #loopReductionNeedsDataClause_parallel
#endif
  {
    #pragma acc CMB_PAR loop gang worker vector reduction(+:var) // #loopReductionNeedsDataClause_reduction
    for (int i = 0; i < 5; ++i)
      var += i;
  }
}

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
    // Reduction operator doesn't permit variable type, but whether it's a
    // reference type is irrelevant.

#if PARENT == PARENT_SEPARATE
    #pragma acc parallel
#endif
    {
      bool b0, &b = b0;
      enum { E1, E2 } e0, &e = e0;
      int i0, &i = i0, jk0, &jk = jk0;
      int a0[2], (&a)[2] = a0; // #reduction_ref_a
      int *p0, *&p = p0; // #reduction_ref_p
      float f0, &f = f0; // #reduction_ref_f
      double d0, &d = d0; // #reduction_ref_d
      float _Complex fc0, &fc = fc0; // #reduction_ref_fc
      double _Complex dc0, &dc = dc0; // #reduction_ref_dc
      struct S { int i; } s0, &s = s0; // #reduction_ref_s
      union U { int i; } u0, &u = u0; // #reduction_ref_u
      extern union U &uDecl; // #reduction_ref_uDecl

      #pragma acc CMB_PAR loop reduction(max:b,e,i,jk,f,d,p)
      for (int i = 0; i < 5; ++i)
        ;
      // expected-error@+7 6 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
      // expected-note@#reduction_ref_fc {{variable 'fc' declared here}}
      // expected-note@#reduction_ref_dc {{variable 'dc' declared here}}
      // expected-note@#reduction_ref_a {{variable 'a' declared here}}
      // expected-note@#reduction_ref_s {{variable 's' declared here}}
      // expected-note@#reduction_ref_u {{variable 'u' declared here}}
      // expected-note@#reduction_ref_uDecl {{variable 'uDecl' declared here}}
      #pragma acc CMB_PAR loop reduction(max:fc,dc,a,s,u,uDecl)
      for (int i = 0; i < 5; ++i)
        ;
      #pragma acc CMB_PAR loop reduction(min:b,e,i,jk,f,d,p)
      for (int i = 0; i < 5; ++i)
        ;
      // expected-error@+7 6 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
      // expected-note@#reduction_ref_fc {{variable 'fc' declared here}}
      // expected-note@#reduction_ref_dc {{variable 'dc' declared here}}
      // expected-note@#reduction_ref_a {{variable 'a' declared here}}
      // expected-note@#reduction_ref_s {{variable 's' declared here}}
      // expected-note@#reduction_ref_u {{variable 'u' declared here}}
      // expected-note@#reduction_ref_uDecl {{variable 'uDecl' declared here}}
      #pragma acc CMB_PAR loop reduction(min:fc,dc,a,s,u,uDecl)
      for (int i = 0; i < 5; ++i)
        ;
      #pragma acc CMB_PAR loop reduction(+:b,e,i,jk,f,d,fc,dc)
      for (int i = 0; i < 5; ++i)
        ;
      // expected-error@+6 5 {{OpenACC reduction operator '+' argument must be of arithmetic type}}
      // expected-note@#reduction_ref_p {{variable 'p' declared here}}
      // expected-note@#reduction_ref_a {{variable 'a' declared here}}
      // expected-note@#reduction_ref_s {{variable 's' declared here}}
      // expected-note@#reduction_ref_u {{variable 'u' declared here}}
      // expected-note@#reduction_ref_uDecl {{variable 'uDecl' declared here}}
      #pragma acc CMB_PAR loop reduction(+:p,a,s,u,uDecl)
      for (int i = 0; i < 5; ++i)
        ;
      #pragma acc CMB_PAR loop reduction(*:b,e,i,jk,f,d,fc,dc)
      for (int i = 0; i < 5; ++i)
        ;
      // expected-error@+6 5 {{OpenACC reduction operator '*' argument must be of arithmetic type}}
      // expected-note@#reduction_ref_p {{variable 'p' declared here}}
      // expected-note@#reduction_ref_a {{variable 'a' declared here}}
      // expected-note@#reduction_ref_s {{variable 's' declared here}}
      // expected-note@#reduction_ref_u {{variable 'u' declared here}}
      // expected-note@#reduction_ref_uDecl {{variable 'uDecl' declared here}}
      #pragma acc CMB_PAR loop reduction(*:p,a,s,u,uDecl)
      for (int i = 0; i < 5; ++i)
        ;
      #pragma acc CMB_PAR loop reduction(&&:b,e,i,jk,f,d,fc,dc)
      for (int i = 0; i < 5; ++i)
        ;
      // expected-error@+6 5 {{OpenACC reduction operator '&&' argument must be of arithmetic type}}
      // expected-note@#reduction_ref_p {{variable 'p' declared here}}
      // expected-note@#reduction_ref_a {{variable 'a' declared here}}
      // expected-note@#reduction_ref_s {{variable 's' declared here}}
      // expected-note@#reduction_ref_u {{variable 'u' declared here}}
      // expected-note@#reduction_ref_uDecl {{variable 'uDecl' declared here}}
      #pragma acc CMB_PAR loop reduction(&&:p,a,s,u,uDecl)
      for (int i = 0; i < 5; ++i)
        ;
      #pragma acc CMB_PAR loop reduction(||:b,e,i,jk,f,d,fc,dc)
      for (int i = 0; i < 5; ++i)
        ;
      // expected-error@+6 5 {{OpenACC reduction operator '||' argument must be of arithmetic type}}
      // expected-note@#reduction_ref_p {{variable 'p' declared here}}
      // expected-note@#reduction_ref_a {{variable 'a' declared here}}
      // expected-note@#reduction_ref_s {{variable 's' declared here}}
      // expected-note@#reduction_ref_u {{variable 'u' declared here}}
      // expected-note@#reduction_ref_uDecl {{variable 'uDecl' declared here}}
      #pragma acc CMB_PAR loop reduction(||:p,a,s,u,uDecl)
      for (int i = 0; i < 5; ++i)
        ;
      #pragma acc CMB_PAR loop reduction(&:b,e,i,jk)
      for (int i = 0; i < 5; ++i)
        ;
      // expected-error@+10 9 {{OpenACC reduction operator '&' argument must be of integer type}}
      // expected-note@#reduction_ref_f {{variable 'f' declared here}}
      // expected-note@#reduction_ref_d {{variable 'd' declared here}}
      // expected-note@#reduction_ref_fc {{variable 'fc' declared here}}
      // expected-note@#reduction_ref_dc {{variable 'dc' declared here}}
      // expected-note@#reduction_ref_p {{variable 'p' declared here}}
      // expected-note@#reduction_ref_a {{variable 'a' declared here}}
      // expected-note@#reduction_ref_s {{variable 's' declared here}}
      // expected-note@#reduction_ref_u {{variable 'u' declared here}}
      // expected-note@#reduction_ref_uDecl {{variable 'uDecl' declared here}}
      #pragma acc CMB_PAR loop reduction(&:f,d,fc,dc,p,a,s,u,uDecl)
      for (int i = 0; i < 5; ++i)
        ;
      #pragma acc CMB_PAR loop reduction(|:b,e,i,jk)
      for (int i = 0; i < 5; ++i)
        ;
      // expected-error@+10 9 {{OpenACC reduction operator '|' argument must be of integer type}}
      // expected-note@#reduction_ref_f {{variable 'f' declared here}}
      // expected-note@#reduction_ref_d {{variable 'd' declared here}}
      // expected-note@#reduction_ref_fc {{variable 'fc' declared here}}
      // expected-note@#reduction_ref_dc {{variable 'dc' declared here}}
      // expected-note@#reduction_ref_p {{variable 'p' declared here}}
      // expected-note@#reduction_ref_a {{variable 'a' declared here}}
      // expected-note@#reduction_ref_s {{variable 's' declared here}}
      // expected-note@#reduction_ref_u {{variable 'u' declared here}}
      // expected-note@#reduction_ref_uDecl {{variable 'uDecl' declared here}}
      #pragma acc CMB_PAR loop reduction(|:f,d,fc,dc,p,a,s,u,uDecl)
      for (int i = 0; i < 5; ++i)
        ;
      #pragma acc CMB_PAR loop reduction(^:b,e,i,jk)
      for (int i = 0; i < 5; ++i)
        ;
      // expected-error@+10 9 {{OpenACC reduction operator '^' argument must be of integer type}}
      // expected-note@#reduction_ref_f {{variable 'f' declared here}}
      // expected-note@#reduction_ref_d {{variable 'd' declared here}}
      // expected-note@#reduction_ref_fc {{variable 'fc' declared here}}
      // expected-note@#reduction_ref_dc {{variable 'dc' declared here}}
      // expected-note@#reduction_ref_p {{variable 'p' declared here}}
      // expected-note@#reduction_ref_a {{variable 'a' declared here}}
      // expected-note@#reduction_ref_s {{variable 's' declared here}}
      // expected-note@#reduction_ref_u {{variable 'u' declared here}}
      // expected-note@#reduction_ref_uDecl {{variable 'uDecl' declared here}}
      #pragma acc CMB_PAR loop reduction(^:f,d,fc,dc,p,a,s,u,uDecl)
      for (int i = 0; i < 5; ++i)
        ;
    }

    //..........................................................................
    // Reduction operator mismatch with variable type is diagnosed only when
    // template is instantiated with mismatched type.

    {
      int i, *p;
      float f;
      reductionPlusTmplTypeVar(i);
      // expected-error@#reductionPlusTmplTypeVar_reduction {{OpenACC reduction operator '+' argument must be of arithmetic type}}
      // expected-note@+2 {{in instantiation of function template specialization 'reductionPlusTmplTypeVar<int *>' requested here}}
      // expected-note@#reductionPlusTmplTypeVar_var {{variable 'var' declared here}}
      reductionPlusTmplTypeVar(p);
      reductionPlusTmplTypeVar(f);
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

  //----------------------------------------------------------------------------
  // Data clauses: data clauses required for scalar loop reduction var
  //
  // This is diagnosed only when template is instantiated with scalar type.
  //----------------------------------------------------------------------------

#if PARENT == PARENT_ORPHANED
  #pragma acc routine gang
#endif
  void testLoopReductionNeedsDataClause() {
    int i;
    int arr[8];
    // sep-error@#loopReductionNeedsDataClause_reduction {{scalar loop reduction variable 'var' requires data clause visible at parent '#pragma acc parallel'}}
    // sep-note@+3 {{in instantiation of function template specialization 'loopReductionNeedsDataClause<int>' requested here}}
    // sep-note@#loopReductionNeedsDataClause_parallel {{parent '#pragma acc parallel' appears here}}
    // sep-note@#loopReductionNeedsDataClause_reduction {{suggest 'copy(var)' because 'var' has gang reduction, specified here}}
    loopReductionNeedsDataClause(i);
    // expected-error@#loopReductionNeedsDataClause_reduction {{OpenACC reduction operator '+' argument must be of arithmetic type}}
    // expected-note@+2 {{in instantiation of function template specialization 'loopReductionNeedsDataClause<int *>' requested here}}
    // expected-note@#loopReductionNeedsDataClause_var {{variable 'var' declared here}}
    loopReductionNeedsDataClause(arr);
  }
};
