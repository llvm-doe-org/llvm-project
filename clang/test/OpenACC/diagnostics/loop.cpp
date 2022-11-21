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

class C {
public:
  int i;
  int fn();
} obj;

#if PARENT == PARENT_ORPHANED
#pragma acc routine gang
#endif
int main() {
  //----------------------------------------------------------------------------
  // Data clauses: arg semantics
  //----------------------------------------------------------------------------

  // Member expression not permitted: data member.
#if PARENT == PARENT_SEPARATE
  #pragma acc parallel
#endif
  {
    // orph-sep-error@+4 {{in 'private' clause on '#pragma acc loop', member expression is not supported}}
    // orph-sep-error@+4 {{in 'reduction' clause on '#pragma acc loop', member expression is not supported}}
    // cmb-error@+2 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported}}
    // cmb-error@+2 {{in 'reduction' clause on '#pragma acc parallel loop', member expression is not supported}}
    #pragma acc CMB_PAR loop gang private(obj.i) \
                                  reduction(+:obj.i)
    for (int i = 0; i < 5; ++i)
      ;
  }

  // Member expression not permitted: member function.
#if PARENT == PARENT_SEPARATE
  #pragma acc parallel
#endif
  {
    // orph-sep-error@+3 {{in 'reduction' clause on '#pragma acc loop', member expression is not supported}}
    // cmb-error@+2 {{in 'reduction' clause on '#pragma acc parallel loop', member expression is not supported}}
    // expected-error@+2 {{expected data member}}
    #pragma acc CMB_PAR loop worker reduction(+:obj.    \
                                                    fn  \
                                                      )
    for (int i = 0; i < 5; ++i)
      ;
    // orph-sep-error@+3 {{in 'private' clause on '#pragma acc loop', member expression is not supported}}
    // cmb-error@+2 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported}}
    // expected-error@+2 {{expected data member}}
    #pragma acc CMB_PAR loop vector private(obj.    \
                                                fn  \
                                                  )
    for (int i = 0; i < 5; ++i)
      ;
  }

  // Member expression not permitted: member function call.
#if PARENT == PARENT_SEPARATE
  #pragma acc parallel
#endif
  {
    // orph-sep-error@+6 {{in 'private' clause on '#pragma acc loop', member expression is not supported}}
    // orph-sep-error@+6 {{in 'reduction' clause on '#pragma acc loop', member expression is not supported}}
    // cmb-error@+4 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported}}
    // cmb-error@+4 {{in 'reduction' clause on '#pragma acc parallel loop', member expression is not supported}}
    // expected-error@+2 {{expected data member}}
    // expected-error@+2 {{expected data member}}
    #pragma acc CMB_PAR loop gang private(obj.fn()) \
                                  reduction(+:obj.fn())
    for (int i = 0; i < 5; ++i)
      ;
  }

  return 0;
}
