// Check diagnostics for "acc loop".
//
// When CMB is not set, this file checks diagnostics for "acc loop" nested
// within "acc parallel".
//
// When CMB is set, it combines those "acc parallel" and "acc loop" directives
// in order to check the same diagnostics but for combined "acc parallel loop"
// directives.  In some cases (reduction inter-directive conflicts), combining
// them would defeat the purpose of the test, so it instead adds "loop" and a
// for loop to the outer "acc parallel"
//
// For checking present, copy, copyin, copyout, create, no_create, and their
// aliases we just run the entire test case once for each.  That seems the
// easiest way to maintain this without losing cases.  This test is fast, so the
// performance impact isn't terrible.

// RUN: %data {
// RUN:   (cflags='-DERR=ERR_ACC -DDATA=present'           )
// RUN:   (cflags='-DERR=ERR_ACC -DDATA=copy'              )
// RUN:   (cflags='-DERR=ERR_ACC -DDATA=pcopy'             )
// RUN:   (cflags='-DERR=ERR_ACC -DDATA=present_or_copy'   )
// RUN:   (cflags='-DERR=ERR_ACC -DDATA=copyin'            )
// RUN:   (cflags='-DERR=ERR_ACC -DDATA=pcopyin'           )
// RUN:   (cflags='-DERR=ERR_ACC -DDATA=present_or_copyin' )
// RUN:   (cflags='-DERR=ERR_ACC -DDATA=copyout'           )
// RUN:   (cflags='-DERR=ERR_ACC -DDATA=pcopyout'          )
// RUN:   (cflags='-DERR=ERR_ACC -DDATA=present_or_copyout')
// RUN:   (cflags='-DERR=ERR_ACC -DDATA=create'            )
// RUN:   (cflags='-DERR=ERR_ACC -DDATA=pcreate'           )
// RUN:   (cflags='-DERR=ERR_ACC -DDATA=present_or_create' )
// RUN:   (cflags='-DERR=ERR_ACC -DDATA=no_create'         )
// RUN:   (cflags=-DERR=ERR_OMP_INIT                       )
// RUN:   (cflags=-DERR=ERR_OMP_COND                       )
// RUN:   (cflags=-DERR=ERR_OMP_INC                        )
// RUN:   (cflags=-DERR=ERR_OMP_INC0                       )
// RUN:   (cflags=-DERR=ERR_OMP_VAR                        )
// RUN: }
// RUN: %for {
// RUN:   %clang_cc1 -fopenacc %[cflags] %s -verify=expected,sep
// RUN:   %clang_cc1 -fopenacc %[cflags] %s -verify=expected,cmb -DCMB
// RUN: }
//
// END.

#include <limits.h>
#include <stdint.h>

#define ERR_ACC       1
#define ERR_OMP_INIT  2
#define ERR_OMP_COND  3
#define ERR_OMP_INC   4
#define ERR_OMP_INC0  5
#define ERR_OMP_VAR   6

#if !CMB
# define CMB_PAR
# define CMB_LOOP
# define CMB_LOOP_COLLAPSE(N)
# define CMB_LOOP_SEQ
# define CMB_FORLOOP_HEAD
#else
# define CMB_PAR parallel
# define CMB_LOOP loop
# define CMB_LOOP_COLLAPSE(N) loop collapse(N)
# define CMB_LOOP_SEQ loop seq
# define CMB_FORLOOP_HEAD for (int fli = 0; fli < 2; ++fli)
#endif

#ifdef __SIZEOF_INT128__
# define HAS_UINT128 1
#else
# define HAS_UINT128 0
#endif

#if ERR == ERR_ACC

int incomplete[];

void fn() {
  _Bool b;
  enum E { E0, E1 } e;
  int i, jk, a[2], *p; // expected-note 9 {{'a' defined here}}
                       // expected-note@-1 7 {{'p' defined here}}
  float f; // expected-note 3 {{'f' defined here}}
  double d; // expected-note 3 {{'d' defined here}}
  float _Complex fc; // expected-note 5 {{'fc' defined here}}
  double _Complex dc; // expected-note 5 {{'dc' defined here}}
  const int constI = 5; // expected-note {{variable 'constI' declared const here}}
                        // expected-note@-1 {{'constI' defined here}}
  const extern int constIDecl; // expected-note {{variable 'constIDecl' declared const here}}
                               // expected-note@-1 {{'constIDecl' declared here}}
  struct S { int i; } s; // expected-note 9 {{'s' defined here}}
  union U { int i; } u; // expected-note 9 {{'u' defined here}}
  extern struct S sDecl; // expected-note 9 {{'sDecl' declared here}}

  //--------------------------------------------------
  // No clauses
  //--------------------------------------------------

#if !CMB
  #pragma acc parallel
#endif
  {
    #pragma acc CMB_PAR loop
    for (int i = 0; i < 5; ++i)
      ;
  }

  //--------------------------------------------------
  // Unrecognized clauses
  //--------------------------------------------------

#if !CMB
  #pragma acc parallel
#endif
  {
    // Bogus clauses.

    // sep-warning@+2 {{extra tokens at the end of '#pragma acc loop' are ignored}}
    // cmb-warning@+1 {{extra tokens at the end of '#pragma acc parallel loop' are ignored}}
    #pragma acc CMB_PAR loop 500
    for (int i = 0; i < 5; ++i)
      ;

    // Well formed clauses not permitted here.

    // sep-error-re@+1 {{unexpected OpenACC clause '{{present|copy|pcopy|present_or_copy|copyin|pcopyin|present_or_copyin|copyout|pcopyout|present_or_copyout|create|pcreate|present_or_create|no_create}}' in directive '#pragma acc loop'}}
    #pragma acc CMB_PAR loop DATA(i)
    for (int i = 0; i < 5; ++i)
      ;
    // sep-error@+4 {{unexpected OpenACC clause 'nomap' in directive '#pragma acc loop'}}
    // cmb-error@+3 {{unexpected OpenACC clause 'nomap' in directive '#pragma acc parallel loop'}}
    // sep-error@+2 {{unexpected OpenACC clause 'delete' in directive '#pragma acc loop'}}
    // cmb-error@+1 {{unexpected OpenACC clause 'delete' in directive '#pragma acc parallel loop'}}
    #pragma acc CMB_PAR loop nomap(i) delete(i)
    for (int i = 0; i < 5; ++i)
      ;
    // sep-error@+2 {{unexpected OpenACC clause 'shared' in directive '#pragma acc loop'}}
    // cmb-error@+1 {{unexpected OpenACC clause 'shared' in directive '#pragma acc parallel loop'}}
    #pragma acc CMB_PAR loop shared(i)
    for (int i = 0; i < 5; ++i)
      ;
    // sep-error@+1 {{unexpected OpenACC clause 'firstprivate' in directive '#pragma acc loop'}}
    #pragma acc CMB_PAR loop firstprivate(i)
    for (int i = 0; i < 5; ++i)
      ;
    // sep-error@+10 {{unexpected OpenACC clause 'if' in directive '#pragma acc loop'}}
    // cmb-error@+9 {{unexpected OpenACC clause 'if' in directive '#pragma acc parallel loop'}}
    // sep-error@+8 {{unexpected OpenACC clause 'if_present' in directive '#pragma acc loop'}}
    // cmb-error@+7 {{unexpected OpenACC clause 'if_present' in directive '#pragma acc parallel loop'}}
    // sep-error@+6 {{unexpected OpenACC clause 'self' in directive '#pragma acc loop'}}
    // cmb-error@+5 {{unexpected OpenACC clause 'self' in directive '#pragma acc parallel loop'}}
    // sep-error@+4 {{unexpected OpenACC clause 'host' in directive '#pragma acc loop'}}
    // cmb-error@+3 {{unexpected OpenACC clause 'host' in directive '#pragma acc parallel loop'}}
    // sep-error@+2 {{unexpected OpenACC clause 'device' in directive '#pragma acc loop'}}
    // cmb-error@+1 {{unexpected OpenACC clause 'device' in directive '#pragma acc parallel loop'}}
    #pragma acc CMB_PAR loop if(1) if_present self(i) host(i) device(i)
    for (int i = 0; i < 5; ++i)
      ;
    // sep-error@+3 {{unexpected OpenACC clause 'num_gangs' in directive '#pragma acc loop'}}
    // sep-error@+2 {{unexpected OpenACC clause 'num_workers' in directive '#pragma acc loop'}}
    // sep-error@+1 {{unexpected OpenACC clause 'vector_length' in directive '#pragma acc loop'}}
    #pragma acc CMB_PAR loop num_gangs(3) num_workers(3) vector_length(3)
    for (int i = 0; i < 5; ++i)
      ;
  }

  //--------------------------------------------------
  // Partitionability clauses
  //--------------------------------------------------

#if !CMB
  #pragma acc parallel
#endif
  {
    #pragma acc CMB_PAR loop seq
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc CMB_PAR loop independent
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc CMB_PAR loop auto
    for (int i = 0; i < 5; ++i)
      ;
    // sep-warning@+2 {{extra tokens at the end of '#pragma acc loop' are ignored}}
    // cmb-warning@+1 {{extra tokens at the end of '#pragma acc parallel loop' are ignored}}
    #pragma acc CMB_PAR loop seq()
    for (int i = 0; i < 5; ++i)
      ;
    // sep-warning@+2 {{extra tokens at the end of '#pragma acc loop' are ignored}}
    // cmb-warning@+1 {{extra tokens at the end of '#pragma acc parallel loop' are ignored}}
    #pragma acc CMB_PAR loop independent()
    for (int i = 0; i < 5; ++i)
      ;
    // sep-warning@+2 {{extra tokens at the end of '#pragma acc loop' are ignored}}
    // cmb-warning@+1 {{extra tokens at the end of '#pragma acc parallel loop' are ignored}}
    #pragma acc CMB_PAR loop auto()
    for (int i = 0; i < 5; ++i)
      ;
    // sep-error@+2 {{directive '#pragma acc loop' cannot contain more than one 'seq' clause}}
    // cmb-error@+1 {{directive '#pragma acc parallel loop' cannot contain more than one 'seq' clause}}
    #pragma acc CMB_PAR loop seq seq
    for (int i = 0; i < 5; ++i)
      ;
    // sep-error@+2 {{directive '#pragma acc loop' cannot contain more than one 'independent' clause}}
    // cmb-error@+1 {{directive '#pragma acc parallel loop' cannot contain more than one 'independent' clause}}
    #pragma acc CMB_PAR loop independent independent
    for (int i = 0; i < 5; ++i)
      ;
    // sep-error@+2 {{directive '#pragma acc loop' cannot contain more than one 'auto' clause}}
    // cmb-error@+1 {{directive '#pragma acc parallel loop' cannot contain more than one 'auto' clause}}
    #pragma acc CMB_PAR loop auto auto
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{unexpected OpenACC clause 'independent', 'seq' is specified already}}
    #pragma acc CMB_PAR loop seq independent
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{unexpected OpenACC clause 'seq', 'auto' is specified already}}
    #pragma acc CMB_PAR loop auto seq
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{unexpected OpenACC clause 'auto', 'independent' is specified already}}
    #pragma acc CMB_PAR loop independent auto
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{unexpected OpenACC clause 'seq', 'auto' is specified already}}
    // expected-error@+1 {{unexpected OpenACC clause 'independent', 'seq' is specified already}}
    #pragma acc CMB_PAR loop auto seq independent
    for (int i = 0; i < 5; ++i)
      ;
  }

  //--------------------------------------------------
  // Partitioning clauses
  //--------------------------------------------------

#if !CMB
  #pragma acc parallel
#endif
  {
    #pragma acc CMB_PAR loop gang
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc CMB_PAR loop worker
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc CMB_PAR loop vector
    for (int i = 0; i < 5; ++i)
      ;
    // sep-warning@+2 {{extra tokens at the end of '#pragma acc loop' are ignored}}
    // cmb-warning@+1 {{extra tokens at the end of '#pragma acc parallel loop' are ignored}}
    #pragma acc CMB_PAR loop gang()
    for (int i = 0; i < 5; ++i)
      ;
    // sep-warning@+2 {{extra tokens at the end of '#pragma acc loop' are ignored}}
    // cmb-warning@+1 {{extra tokens at the end of '#pragma acc parallel loop' are ignored}}
    #pragma acc CMB_PAR loop worker()
    for (int i = 0; i < 5; ++i)
      ;
    // sep-warning@+2 {{extra tokens at the end of '#pragma acc loop' are ignored}}
    // cmb-warning@+1 {{extra tokens at the end of '#pragma acc parallel loop' are ignored}}
    #pragma acc CMB_PAR loop vector()
    for (int i = 0; i < 5; ++i)
      ;
    // sep-error@+2 {{directive '#pragma acc loop' cannot contain more than one 'gang' clause}}
    // cmb-error@+1 {{directive '#pragma acc parallel loop' cannot contain more than one 'gang' clause}}
    #pragma acc CMB_PAR loop gang gang
    for (int i = 0; i < 5; ++i)
      ;
    // sep-error@+2 {{directive '#pragma acc loop' cannot contain more than one 'worker' clause}}
    // cmb-error@+1 {{directive '#pragma acc parallel loop' cannot contain more than one 'worker' clause}}
    #pragma acc CMB_PAR loop independent worker worker
    for (int i = 0; i < 5; ++i)
      ;
    // sep-error@+2 {{directive '#pragma acc loop' cannot contain more than one 'vector' clause}}
    // cmb-error@+1 {{directive '#pragma acc parallel loop' cannot contain more than one 'vector' clause}}
    #pragma acc CMB_PAR loop vector auto vector
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc CMB_PAR loop gang worker vector
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc CMB_PAR loop independent gang worker vector
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc CMB_PAR loop gang worker vector auto
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{unexpected OpenACC clause 'gang', 'seq' is specified already}}
    #pragma acc CMB_PAR loop seq gang
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{unexpected OpenACC clause 'seq', 'worker' is specified already}}
    #pragma acc CMB_PAR loop worker seq
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+3 {{unexpected OpenACC clause 'seq', 'vector' is specified already}}
    // sep-error@+2 {{directive '#pragma acc loop' cannot contain more than one 'vector' clause}}
    // cmb-error@+1 {{directive '#pragma acc parallel loop' cannot contain more than one 'vector' clause}}
    #pragma acc CMB_PAR loop vector seq vector
    for (int i = 0; i < 5; ++i)
      ;
  }

  //--------------------------------------------------
  // collapse clause
  //--------------------------------------------------

  // Syntax
#if !CMB
  #pragma acc parallel
#endif
  {
    // expected-error@+1 {{expected '(' after 'collapse'}}
    #pragma acc CMB_PAR loop collapse
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+3 {{expected expression}}
    // expected-error@+2 {{expected ')'}}
    // expected-note@+1 {{to match this '('}}
    #pragma acc CMB_PAR loop independent collapse(
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{expected expression}}
    #pragma acc CMB_PAR loop seq collapse()
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{expected ')'}}
    // expected-note@+1 {{to match this '('}}
    #pragma acc CMB_PAR loop auto collapse(2, 3)
    for (int i = 0; i < 5; ++i)
      for (int j = 0; j < 5; ++j)
        ;
    // sep-error@+2 {{directive '#pragma acc loop' cannot contain more than one 'collapse' clause}}
    // cmb-error@+1 {{directive '#pragma acc parallel loop' cannot contain more than one 'collapse' clause}}
    #pragma acc CMB_PAR loop gang collapse(1) worker collapse(1) vector
    for (int i = 0; i < 5; ++i)
      ;
  }

  // Invalid argument
#if !CMB
  #pragma acc parallel
#endif
  {
    // Not integer type and not constant expression

    // expected-error@+2 {{expression must have integral or unscoped enumeration type, not 'int []'}}
    // expected-error@+1 {{argument to 'collapse' clause must be an integer constant expression}}
    #pragma acc CMB_PAR loop collapse(incomplete)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{expression must have integral or unscoped enumeration type, not 'int [2]'}}
    // expected-error@+1 {{argument to 'collapse' clause must be an integer constant expression}}
    #pragma acc CMB_PAR loop independent collapse(a)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{expression must have integral or unscoped enumeration type, not 'int *'}}
    // expected-error@+1 {{argument to 'collapse' clause must be an integer constant expression}}
    #pragma acc CMB_PAR loop seq collapse(p)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{expression must have integral or unscoped enumeration type, not 'float'}}
    // expected-error@+1 {{argument to 'collapse' clause must be an integer constant expression}}
    #pragma acc CMB_PAR loop auto collapse(f)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{expression must have integral or unscoped enumeration type, not 'double'}}
    // expected-error@+1 {{argument to 'collapse' clause must be an integer constant expression}}
    #pragma acc CMB_PAR loop gang collapse(d)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{expression must have integral or unscoped enumeration type, not '_Complex float'}}
    // expected-error@+1 {{argument to 'collapse' clause must be an integer constant expression}}
    #pragma acc CMB_PAR loop worker collapse(fc)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{expression must have integral or unscoped enumeration type, not '_Complex double'}}
    // expected-error@+1 {{argument to 'collapse' clause must be an integer constant expression}}
    #pragma acc CMB_PAR loop vector collapse(dc)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{expression must have integral or unscoped enumeration type, not 'struct S'}}
    // expected-error@+1 {{argument to 'collapse' clause must be an integer constant expression}}
    #pragma acc CMB_PAR loop gang worker collapse(s)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{expression must have integral or unscoped enumeration type, not 'union U'}}
    // expected-error@+1 {{argument to 'collapse' clause must be an integer constant expression}}
    #pragma acc CMB_PAR loop gang vector collapse(u)
    for (int i = 0; i < 5; ++i)
      ;

    // Integer type but not constant expression

    // expected-error@+1 {{argument to 'collapse' clause must be an integer constant expression}}
    #pragma acc CMB_PAR loop worker vector collapse(b)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{argument to 'collapse' clause must be an integer constant expression}}
    #pragma acc CMB_PAR loop gang worker vector collapse(e)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{argument to 'collapse' clause must be an integer constant expression}}
    #pragma acc CMB_PAR loop collapse(*p)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{argument to 'collapse' clause must be an integer constant expression}}
    #pragma acc CMB_PAR loop collapse(i) independent
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{argument to 'collapse' clause must be an integer constant expression}}
    #pragma acc CMB_PAR loop collapse(constI) seq
    for (int i = 0; i < 5; ++i)
      ;

    // Constant expression but not integer type

    // expected-error@+2 {{expression must have integral or unscoped enumeration type, not 'int [2]'}}
    // expected-error@+1 {{argument to 'collapse' clause must be an integer constant expression}}
    #pragma acc CMB_PAR loop collapse((int[]){1, 2}) auto
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{expression must have integral or unscoped enumeration type, not 'int *'}}
    // expected-error@+1 {{argument to 'collapse' clause must be an integer constant expression}}
    #pragma acc CMB_PAR loop collapse((int*)0) gang
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{expression must have integral or unscoped enumeration type, not 'float'}}
    // expected-error@+1 {{argument to 'collapse' clause must be an integer constant expression}}
    #pragma acc CMB_PAR loop collapse(1.3f) worker
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{expression must have integral or unscoped enumeration type, not 'double'}}
    // expected-error@+1 {{argument to 'collapse' clause must be an integer constant expression}}
    #pragma acc CMB_PAR loop collapse(0.2) vector
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{expression must have integral or unscoped enumeration type, not '_Complex float'}}
    // expected-error@+1 {{argument to 'collapse' clause must be an integer constant expression}}
    #pragma acc CMB_PAR loop collapse((float _Complex)0.09f) gang worker
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{expression must have integral or unscoped enumeration type, not '_Complex double'}}
    // expected-error@+1 {{argument to 'collapse' clause must be an integer constant expression}}
    #pragma acc CMB_PAR loop collapse((double _Complex)-10.0) gang vector
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{expression must have integral or unscoped enumeration type, not 'struct S'}}
    // expected-error@+1 {{argument to 'collapse' clause must be an integer constant expression}}
    #pragma acc CMB_PAR loop collapse((struct S){5}) gang worker vector
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{expression must have integral or unscoped enumeration type, not 'union U'}}
    // expected-error@+1 {{argument to 'collapse' clause must be an integer constant expression}}
    #pragma acc CMB_PAR loop independent gang collapse((union U){5})
    for (int i = 0; i < 5; ++i)
      ;

    // Integer type and constant expression but not positive

    // expected-error@+1 {{argument to 'collapse' clause must be a strictly positive integer value}}
    #pragma acc CMB_PAR loop auto worker collapse((_Bool)0)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{argument to 'collapse' clause must be a strictly positive integer value}}
    #pragma acc CMB_PAR loop vector collapse(0) independent
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{argument to 'collapse' clause must be a strictly positive integer value}}
    #pragma acc CMB_PAR loop collapse(0u) auto gang
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{argument to 'collapse' clause must be a strictly positive integer value}}
    #pragma acc CMB_PAR loop worker independent collapse(E0)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{argument to 'collapse' clause must be a strictly positive integer value}}
    #pragma acc CMB_PAR loop gang worker vector collapse(-5L) auto
    for (int i = 0; i < 5; ++i)
      ;

  }

  // Not enough loops.
#if !CMB
  #pragma acc parallel
#endif
  {
    // 1 too few.

    // expected-note@+1 {{as specified in 'collapse' clause}}
    #pragma acc CMB_PAR loop collapse((_Bool)1)
      // sep-error@+2 {{statement after '#pragma acc loop' must be a for loop}}
      // cmb-error@+1 {{statement after '#pragma acc parallel loop' must be a for loop}}
      ;
    // expected-note@+1 {{as specified in 'collapse' clause}}
    #pragma acc CMB_PAR loop independent collapse(E1)
      // sep-error@+2 {{statement after '#pragma acc loop' must be a for loop}}
      // cmb-error@+1 {{statement after '#pragma acc parallel loop' must be a for loop}}
      while (1)
        ;
    // expected-note@+1 {{as specified in 'collapse' clause}}
    #pragma acc CMB_PAR loop seq collapse(1)
    // sep-error@+2 {{statement after '#pragma acc loop' must be a for loop}}
    // cmb-error@+1 {{statement after '#pragma acc parallel loop' must be a for loop}}
    {
      int i;
      for (i = 0; i < 5; ++i)
        ;
    }
    // expected-note@+1 {{as specified in 'collapse' clause}}
    #pragma acc CMB_PAR loop auto collapse(2)
    for (i = 0; i < 5; ++i) {
      // sep-error@+2 {{statement after '#pragma acc loop' must be a for loop}}
      // cmb-error@+1 {{statement after '#pragma acc parallel loop' must be a for loop}}
      ;
    }
    // expected-note@+1 {{as specified in 'collapse' clause}}
    #pragma acc CMB_PAR loop gang collapse(3)
    for (i = 0; i < 5; ++i)
      for (int j = 0; j < 5; ++j)
      // sep-error@+2 {{statement after '#pragma acc loop' must be a for loop}}
      // cmb-error@+1 {{statement after '#pragma acc parallel loop' must be a for loop}}
      {
        for (int k = 0; k < 5; ++k)
          ;
        ;
      }
    // expected-note@+1 {{as specified in 'collapse' clause}}
    #pragma acc CMB_PAR loop worker collapse(4u)
    for (int i = 0; i < 5; ++i) {
      for (jk = 0; jk < 5; ++jk) {
        for (int k = 0; k < 5; ++k) {
          // sep-error@+2 {{statement after '#pragma acc loop' must be a for loop}}
          // cmb-error@+1 {{statement after '#pragma acc parallel loop' must be a for loop}}
          f += 0.5;
        }
      }
    }

    // 2 too few.

    // expected-note@+1 {{as specified in 'collapse' clause}}
    #pragma acc CMB_PAR loop vector collapse(5l)
    for (int i = 0; i < 5; ++i)
      for (int j = 0; j < 5; ++j)
        for (jk = 0; jk < 5; ++jk)
          // sep-error@+2 {{statement after '#pragma acc loop' must be a for loop}}
          // cmb-error@+1 {{statement after '#pragma acc parallel loop' must be a for loop}}
          ;

    // 3 too few.

    // expected-note@+1 {{as specified in 'collapse' clause}}
    #pragma acc CMB_PAR loop gang worker collapse(7ll)
    for (i = 0; i < 5; ++i)
      for (jk = 0; jk < 5; ++jk)
        for (int k = 0; k < 5; ++k)
          for (int l = 0; l < 5; ++l)
            // sep-error@+2 {{statement after '#pragma acc loop' must be a for loop}}
            // cmb-error@+1 {{statement after '#pragma acc parallel loop' must be a for loop}}
            ;

    // Many too few.  High bits are set in these values, but they shouldn't be
    // rejected as negative values.

    // expected-note@+1 {{as specified in 'collapse' clause}}
    #pragma acc CMB_PAR loop gang vector collapse((unsigned char)0xa3)
    for (int i = 0; i < 5; ++i)
      for (int j = 0; j < 5; ++j)
        // sep-error@+2 {{statement after '#pragma acc loop' must be a for loop}}
        // cmb-error@+1 {{statement after '#pragma acc parallel loop' must be a for loop}}
        ;
    // expected-note@+1 {{as specified in 'collapse' clause}}
    #pragma acc CMB_PAR loop gang worker vector collapse(UINT_MAX)
    for (i = 0; i < 5; ++i)
      // sep-error@+2 {{statement after '#pragma acc loop' must be a for loop}}
      // cmb-error@+1 {{statement after '#pragma acc parallel loop' must be a for loop}}
      ;

    // Enough but some have their own loop directives.

    // expected-note@+1 {{as specified in 'collapse' clause}}
    #pragma acc CMB_PAR loop gang collapse(2)
    for (i = 0; i < 5; ++i)
      // sep-error@+2 {{statement after '#pragma acc loop' must be a for loop}}
      // cmb-error@+1 {{statement after '#pragma acc parallel loop' must be a for loop}}
      #pragma acc loop worker
      for (int j = 0; j < 5; ++j)
        ;
    // expected-note@+1 {{as specified in 'collapse' clause}}
    #pragma acc CMB_PAR loop gang collapse(3)
    for (int i = 0; i < 5; ++i)
      for (int j = 0; j < 5; ++j)
        // sep-error@+2 {{statement after '#pragma acc loop' must be a for loop}}
        // cmb-error@+1 {{statement after '#pragma acc parallel loop' must be a for loop}}
        #pragma acc loop vector collapse(2)
        for (int k = 0; k < 5; ++k)
          for (int l = 0; l < 5; ++l)
            ;
    // expected-note@+1 {{as specified in 'collapse' clause}}
    #pragma acc CMB_PAR loop worker collapse(3)
    for (int i = 0; i < 5; ++i)
      for (int j = 0; j < 5; ++j)
        // sep-error@+2 {{statement after '#pragma acc loop' must be a for loop}}
        // cmb-error@+1 {{statement after '#pragma acc parallel loop' must be a for loop}}
        #pragma acc loop vector
        for (int k = 0; k < 5; ++k)
          for (int l = 0; l < 5; ++l)
            ;
  }

  //--------------------------------------------------
  // associated for loop
  //--------------------------------------------------

#if !CMB
  #pragma acc parallel
#endif
  {
    // for loop
    #pragma acc CMB_PAR loop
      // sep-error@+2 {{statement after '#pragma acc loop' must be a for loop}}
      // cmb-error@+1 {{statement after '#pragma acc parallel loop' must be a for loop}}
      ;

    // break is fine when loop is executed sequentially
    #pragma acc CMB_PAR loop seq
    for (int i = 0; i < 5; ++i)
      break;

    // break forces sequential execution in the case of auto
    #pragma acc CMB_PAR loop auto
    for (int i = 0; i < 5; ++i)
      break;

    // break is not permitted for implicit independent
    #pragma acc CMB_PAR loop
    for (int i = 0; i < 5; ++i)
      break; // expected-error {{'break' statement cannot be used in partitionable OpenACC for loop}}

    // break is not permitted for explicit independent
    #pragma acc CMB_PAR loop independent
    for (int i = 0; i < 5; ++i)
      break; // expected-error {{'break' statement cannot be used in partitionable OpenACC for loop}}

    // break is permitted in nested loops that are not partitioned
    #pragma acc CMB_PAR loop independent
    for (int i = 0; i < 5; ++i)
      for (int j = 0; j < 5; ++j)
        break;
    // FIXME: This should probably be an error, but the OpenMP implementation
    // currently doesn't catch it either for omp parallel for either.
    #pragma acc CMB_PAR loop independent collapse(2)
    for (int i = 0; i < 5; ++i)
      for (int j = 0; j < 5; ++j)
        break;

    // break is permitted in nested switches
    #pragma acc CMB_PAR loop independent
    for (int i = 0; i < 5; ++i) {
      switch (i) {
      case 0:
        break;
      case 1:
        break;
      default:
        break;
      }
    }

    // A break in a context where it's not permitted in C used to cause the
    // OpenACC implementation to fail because the expected "break parent"
    // doesn't exist.
    // expected-error@+1 {{'break' statement not in loop or switch statement}}
    break;
  }

  //--------------------------------------------------
  // nesting of acc loops: 2 levels
  //--------------------------------------------------

#if !CMB
  #pragma acc parallel
#endif
  {
    // sep-note@+2 4 {{enclosing '#pragma acc loop' here}}
    // cmb-note@+1 4 {{enclosing '#pragma acc parallel loop' here}}
    #pragma acc CMB_PAR loop gang
    for (int i = 0; i < 5; ++i) {
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop gang
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop worker
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop gang worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop vector gang
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop worker vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop gang worker vector
      for (int j = 0; j < 5; ++j)
        ;
    }

    // sep-note@+2 6 {{enclosing '#pragma acc loop' here}}
    // cmb-note@+1 6 {{enclosing '#pragma acc parallel loop' here}}
    #pragma acc CMB_PAR loop worker
    for (int i = 0; i < 5; ++i) {
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop worker
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop worker gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop gang vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop vector worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop gang vector worker
      for (int j = 0; j < 5; ++j)
        ;
    }

    // sep-note@+2 7 {{enclosing '#pragma acc loop' here}}
    // cmb-note@+1 7 {{enclosing '#pragma acc parallel loop' here}}
    #pragma acc CMB_PAR loop vector
    for (int i = 0; i < 5; ++i) {
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop gang worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop vector gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop worker vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop worker gang vector
      for (int j = 0; j < 5; ++j)
        ;
    }

    // sep-note@+2 6 {{enclosing '#pragma acc loop' here}}
    // cmb-note@+1 6 {{enclosing '#pragma acc parallel loop' here}}
    #pragma acc CMB_PAR loop gang worker
    for (int i = 0; i < 5; ++i) {
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop worker
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop worker gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop gang vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop vector worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop worker vector gang
      for (int j = 0; j < 5; ++j)
        ;
    }

    // sep-note@+2 7 {{enclosing '#pragma acc loop' here}}
    // cmb-note@+1 7 {{enclosing '#pragma acc parallel loop' here}}
    #pragma acc CMB_PAR loop gang vector
    for (int i = 0; i < 5; ++i) {
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop gang worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop vector gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop worker vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop vector gang worker
      for (int j = 0; j < 5; ++j)
        ;
    }

    // sep-note@+2 7 {{enclosing '#pragma acc loop' here}}
    // cmb-note@+1 7 {{enclosing '#pragma acc parallel loop' here}}
    #pragma acc CMB_PAR loop worker vector
    for (int i = 0; i < 5; ++i) {
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop worker gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop gang vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop vector worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop vector worker gang
      for (int j = 0; j < 5; ++j)
        ;
    }

    // sep-note@+2 7 {{enclosing '#pragma acc loop' here}}
    // cmb-note@+1 7 {{enclosing '#pragma acc parallel loop' here}}
    #pragma acc CMB_PAR loop gang worker vector
    for (int i = 0; i < 5; ++i) {
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop gang worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop vector gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop worker vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop gang worker vector
      for (int j = 0; j < 5; ++j)
        ;
    }
  }

  //--------------------------------------------------
  // nesting of acc loops: 2 levels with auto on outer level
  //--------------------------------------------------

#if !CMB
  #pragma acc parallel
#endif
  {
    // sep-note@+2 4 {{enclosing '#pragma acc loop' here}}
    // cmb-note@+1 4 {{enclosing '#pragma acc parallel loop' here}}
    #pragma acc CMB_PAR loop auto gang
    for (int i = 0; i < 5; ++i) {
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop gang
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop worker
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop worker gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop gang vector
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop vector worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop vector worker gang
      for (int j = 0; j < 5; ++j)
        ;
    }

    // sep-note@+2 6 {{enclosing '#pragma acc loop' here}}
    // cmb-note@+1 6 {{enclosing '#pragma acc parallel loop' here}}
    #pragma acc CMB_PAR loop worker auto
    for (int i = 0; i < 5; ++i) {
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop worker
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop gang worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop vector gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop worker vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop vector gang worker
      for (int j = 0; j < 5; ++j)
        ;
    }

    // sep-note@+2 7 {{enclosing '#pragma acc loop' here}}
    // cmb-note@+1 7 {{enclosing '#pragma acc parallel loop' here}}
    #pragma acc CMB_PAR loop auto vector
    for (int i = 0; i < 5; ++i) {
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop worker gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop gang vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop vector worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop worker vector gang
      for (int j = 0; j < 5; ++j)
        ;
    }

    // sep-note@+2 6 {{enclosing '#pragma acc loop' here}}
    // cmb-note@+1 6 {{enclosing '#pragma acc parallel loop' here}}
    #pragma acc CMB_PAR loop gang auto worker
    for (int i = 0; i < 5; ++i) {
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop worker
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop gang worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop vector gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop worker vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop worker gang vector
      for (int j = 0; j < 5; ++j)
        ;
    }

    // sep-note@+2 7 {{enclosing '#pragma acc loop' here}}
    // cmb-note@+1 7 {{enclosing '#pragma acc parallel loop' here}}
    #pragma acc CMB_PAR loop gang vector auto
    for (int i = 0; i < 5; ++i) {
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop worker gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop gang vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop vector worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop gang vector worker
      for (int j = 0; j < 5; ++j)
        ;
    }

    // sep-note@+2 7 {{enclosing '#pragma acc loop' here}}
    // cmb-note@+1 7 {{enclosing '#pragma acc parallel loop' here}}
    #pragma acc CMB_PAR loop auto worker vector
    for (int i = 0; i < 5; ++i) {
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop gang worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop vector gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop worker vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop gang worker vector
      for (int j = 0; j < 5; ++j)
        ;
    }

    // sep-note@+2 7 {{enclosing '#pragma acc loop' here}}
    // cmb-note@+1 7 {{enclosing '#pragma acc parallel loop' here}}
    #pragma acc CMB_PAR loop gang auto worker vector
    for (int i = 0; i < 5; ++i) {
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop worker gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop gang vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop vector worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop vector worker gang
      for (int j = 0; j < 5; ++j)
        ;
    }
  }

  //--------------------------------------------------
  // nesting of acc loops: 2 levels with auto on inner level
  //--------------------------------------------------

#if !CMB
  #pragma acc parallel
#endif
  {
    // sep-note@+2 4 {{enclosing '#pragma acc loop' here}}
    // cmb-note@+1 4 {{enclosing '#pragma acc parallel loop' here}}
    #pragma acc CMB_PAR loop gang
    for (int i = 0; i < 5; ++i) {
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop auto gang
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop worker auto
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop auto vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop worker auto gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop gang vector auto
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop auto vector worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop vector auto worker gang
      for (int j = 0; j < 5; ++j)
        ;
    }

    // sep-note@+2 6 {{enclosing '#pragma acc loop' here}}
    // cmb-note@+1 6 {{enclosing '#pragma acc parallel loop' here}}
    #pragma acc CMB_PAR loop worker
    for (int i = 0; i < 5; ++i) {
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop gang auto
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop auto worker
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop vector auto
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop auto gang worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop vector auto gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop worker vector auto
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop auto vector gang worker
      for (int j = 0; j < 5; ++j)
        ;
    }

    // sep-note@+2 7 {{enclosing '#pragma acc loop' here}}
    // cmb-note@+1 7 {{enclosing '#pragma acc parallel loop' here}}
    #pragma acc CMB_PAR loop vector
    for (int i = 0; i < 5; ++i) {
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop auto gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop worker auto
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop auto vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop worker gang auto
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop gang auto vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop auto vector worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop worker vector gang auto
      for (int j = 0; j < 5; ++j)
        ;
    }

    // sep-note@+2 6 {{enclosing '#pragma acc loop' here}}
    // cmb-note@+1 6 {{enclosing '#pragma acc parallel loop' here}}
    #pragma acc CMB_PAR loop gang worker
    for (int i = 0; i < 5; ++i) {
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop gang auto
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop auto worker
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop vector auto
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop auto gang worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop vector gang auto
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop worker auto vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop auto worker gang vector
      for (int j = 0; j < 5; ++j)
        ;
    }

    // sep-note@+2 7 {{enclosing '#pragma acc loop' here}}
    // cmb-note@+1 7 {{enclosing '#pragma acc parallel loop' here}}
    #pragma acc CMB_PAR loop gang vector
    for (int i = 0; i < 5; ++i) {
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop gang auto
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop auto worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop vector auto
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop auto worker gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop gang auto vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop vector worker auto
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop auto gang vector worker
      for (int j = 0; j < 5; ++j)
        ;
    }

    // sep-note@+2 7 {{enclosing '#pragma acc loop' here}}
    // cmb-note@+1 7 {{enclosing '#pragma acc parallel loop' here}}
    #pragma acc CMB_PAR loop worker vector
    for (int i = 0; i < 5; ++i) {
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop auto gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop auto worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop vector auto
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop gang auto worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop vector gang auto
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop worker vector auto
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop gang worker vector auto
      for (int j = 0; j < 5; ++j)
        ;
    }

    // sep-note@+2 7 {{enclosing '#pragma acc loop' here}}
    // cmb-note@+1 7 {{enclosing '#pragma acc parallel loop' here}}
    #pragma acc CMB_PAR loop gang worker vector
    for (int i = 0; i < 5; ++i) {
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop gang auto
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop worker auto
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc parallel loop' with 'vector' clause}}
      #pragma acc loop auto vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop auto worker gang
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop gang auto vector
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
      #pragma acc loop vector auto worker
      for (int j = 0; j < 5; ++j)
        ;
      // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'gang' clause}}
      #pragma acc loop vector worker auto gang
      for (int j = 0; j < 5; ++j)
        ;
    }
  }

  //--------------------------------------------------
  // nesting of acc loops: 3 levels
  //--------------------------------------------------

#if !CMB
  #pragma acc parallel
#endif
  {
    #pragma acc CMB_PAR loop gang
    for (int i = 0; i < 5; ++i) {
      #pragma acc loop worker // expected-note 2 {{enclosing '#pragma acc loop' here}}
      for (int j = 0; j < 5; ++j) {
        #pragma acc loop gang // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
        for (int k = 0; k < 5; ++k)
          ;
        #pragma acc loop worker // expected-error {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
        for (int k = 0; k < 5; ++k)
          ;
        #pragma acc loop vector
        for (int k = 0; k < 5; ++k)
          ;
      }
      #pragma acc loop vector // expected-note 3 {{enclosing '#pragma acc loop' here}}
      for (int j = 0; j < 5; ++j) {
        #pragma acc loop gang // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
        for (int k = 0; k < 5; ++k)
          ;
        #pragma acc loop worker // expected-error {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
        for (int k = 0; k < 5; ++k)
          ;
        #pragma acc loop vector // expected-error {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
        for (int k = 0; k < 5; ++k)
          ;
      }
    }
    #pragma acc CMB_PAR loop worker
    for (int i = 0; i < 5; ++i) {
      #pragma acc loop vector // expected-note 3 {{enclosing '#pragma acc loop' here}}
      for (int j = 0; j < 5; ++j) {
        #pragma acc loop gang // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
        for (int k = 0; k < 5; ++k)
          ;
        #pragma acc loop worker // expected-error {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
        for (int k = 0; k < 5; ++k)
          ;
        #pragma acc loop vector // expected-error {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
        for (int k = 0; k < 5; ++k)
          ;
      }
    }
    #pragma acc CMB_PAR loop gang worker
    for (int i = 0; i < 5; ++i) {
      #pragma acc loop vector // expected-note 3 {{enclosing '#pragma acc loop' here}}
      for (int j = 0; j < 5; ++j) {
        #pragma acc loop gang // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
        for (int k = 0; k < 5; ++k)
          ;
        #pragma acc loop worker // expected-error {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
        for (int k = 0; k < 5; ++k)
          ;
        #pragma acc loop vector // expected-error {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
        for (int k = 0; k < 5; ++k)
          ;
      }
    }
    #pragma acc CMB_PAR loop gang
    for (int i = 0; i < 5; ++i) {
      #pragma acc loop worker vector // expected-note 3 {{enclosing '#pragma acc loop' here}}
      for (int j = 0; j < 5; ++j) {
        #pragma acc loop gang // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
        for (int k = 0; k < 5; ++k)
          ;
        #pragma acc loop worker // expected-error {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
        for (int k = 0; k < 5; ++k)
          ;
        #pragma acc loop vector // expected-error {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
        for (int k = 0; k < 5; ++k)
          ;
      }
    }
  }

  //--------------------------------------------------
  // nesting of acc loops: 4 levels
  //--------------------------------------------------

#if !CMB
  #pragma acc parallel
#endif
  {
    #pragma acc CMB_PAR loop gang
    for (int i = 0; i < 5; ++i) {
      #pragma acc loop worker
      for (int j = 0; j < 5; ++j) {
        #pragma acc loop vector // expected-note 3 {{enclosing '#pragma acc loop' here}}
        for (int k = 0; k < 5; ++k) {
          #pragma acc loop gang // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
          for (int l = 0; l < 5; ++l)
            ;
          #pragma acc loop worker // expected-error {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
          for (int l = 0; l < 5; ++l)
            ;
          #pragma acc loop vector // expected-error {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
          for (int l = 0; l < 5; ++l)
            ;
        }
      }
    }
  }

  //--------------------------------------------------
  // nesting of acc loops: other loops in between
  //--------------------------------------------------

#if !CMB
  #pragma acc parallel
#endif
  {
    #pragma acc CMB_PAR loop gang
    for (int i = 0; i < 5; ++i) {
      #pragma acc loop
      for (int i1 = 0; i1 < 5; ++i1) {
        #pragma acc loop worker
        for (int j = 0; j < 5; ++j) {
          #pragma acc loop independent
          for (int j1 = 0; j1 < 5; ++j1) {
            #pragma acc loop auto
            for (int j2 = 0; j2 < 5; ++j2) {
              #pragma acc loop seq
              for (int j3 = 0; j3 < 5; ++j3) {
                #pragma acc loop vector
                for (int k = 0; k < 5; ++k)
                  ;
              }
            }
          }
        }
      }
    }
    // sep-note@+2 2 {{enclosing '#pragma acc loop' here}}
    // cmb-note@+1 2 {{enclosing '#pragma acc parallel loop' here}}
    #pragma acc CMB_PAR loop worker
    for (int i = 0; i < 5; ++i) {
      #pragma acc loop
      for (int i1 = 0; i1 < 5; ++i1) {
        #pragma acc loop independent
        for (int i2 = 0; i2 < 5; ++i2) {
          #pragma acc loop auto
          for (int i3 = 0; i3 < 5; ++i3) {
            #pragma acc loop seq
            for (int i4 = 0; i4 < 5; ++i4) {
              // sep-error@+2 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
              // cmb-error@+1 {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
              #pragma acc loop gang
                for (int j = 0; j < 5; ++j)
                  ;
            }
          }
        }
      }
      #pragma acc loop
      for (int i1 = 0; i1 < 5; ++i1) {
        #pragma acc loop independent
        for (int i2 = 0; i2 < 5; ++i2) {
          #pragma acc loop auto
          for (int i3 = 0; i3 < 5; ++i3) {
            #pragma acc loop seq
            for (int i4 = 0; i4 < 5; ++i4) {
              // sep-error@+2 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
              // cmb-error@+1 {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc parallel loop' with 'worker' clause}}
              #pragma acc loop worker
                for (int j = 0; j < 5; ++j)
                  ;
            }
          }
        }
      }
    }
  }

  //--------------------------------------------------
  // nesting of acc loops: very deeply nested
  //
  // At one time, Clang OpenACC analyses failed assertions when resizing arrays
  // based on the number of levels of nested loops.
  //--------------------------------------------------

#if !CMB
  #pragma acc parallel
#endif
  {
    #pragma acc CMB_PAR loop
    for (int i0 = 0; i0 < 5; ++i0) {
      #pragma acc loop
      for (int i1 = 0; i1 < 5; ++i1) {
        #pragma acc loop
        for (int i2 = 0; i2 < 5; ++i2) {
          #pragma acc loop
          for (int i3 = 0; i3 < 5; ++i3) {
            #pragma acc loop
            for (int i4 = 0; i4 < 5; ++i4) {
              #pragma acc loop
              for (int i5 = 0; i5 < 5; ++i5) {
                #pragma acc loop
                for (int i6 = 0; i6 < 5; ++i6) {
                  #pragma acc loop
                  for (int i7 = 0; i7 < 5; ++i7) {
                    #pragma acc loop
                    for (int i8 = 0; i8 < 5; ++i8) {
                      #pragma acc loop
                      for (int i9 = 0; i9 < 5; ++i9) {
                        #pragma acc loop
                        for (int i10 = 0; i10 < 5; ++i10) {
                          #pragma acc loop
                          for (int i11 = 0; i11 < 5; ++i11) {
                            #pragma acc loop
                            for (int i12 = 0; i12 < 5; ++i12) {
                              #pragma acc loop
                              for (int i13 = 0; i13 < 5; ++i13) {
                                #pragma acc loop
                                for (int i14 = 0; i14 < 5; ++i14) {
                                  #pragma acc loop
                                  for (int i15 = 0; i15 < 5; ++i15)
                                    ;
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  //--------------------------------------------------
  // We sprinkle seq, auto, independent, gang, worker, and vector clauses
  // throughout the following tests as the validation of the private,
  // reduction, and data clauses should be independent of those clauses.  The
  // only exception is gang reductions, which interact with the parent parallel
  // directive, so we test that case more carefully at the end.
  //--------------------------------------------------

  //--------------------------------------------------
  // Data clauses: syntax
  //--------------------------------------------------

#if !CMB
  #pragma acc parallel
#endif
  {
    #pragma acc CMB_PAR loop private // expected-error {{expected '(' after 'private'}}
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc CMB_PAR loop gang reduction // expected-error {{expected '(' after 'reduction'}}
    for (int i = 0; i < 5; ++i)
      ;

    // expected-error@+3 {{expected expression}}
    // expected-error@+2 {{expected ')'}}
    // expected-note@+1 {{to match this '('}}
    #pragma acc CMB_PAR loop worker private(
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+4 {{expected reduction operator}}
    // expected-warning@+3 {{missing ':' after reduction operator - ignoring}}
    // expected-error@+2 {{expected ')'}}
    // expected-note@+1 {{to match this '('}}
    #pragma acc CMB_PAR loop vector reduction(
    for (int i = 0; i < 5; ++i)
      ;

    // expected-error@+2 {{expected ')'}}
    // expected-note@+1 {{to match this '('}}
    #pragma acc CMB_PAR loop worker private(i
    for (int i = 0; i < 5; ++i)
      ;
    // expected-warning@+4 {{missing ':' after reduction operator - ignoring}}
    // expected-error@+3 {{expected expression}}
    // expected-error@+2 {{expected ')'}}
    // expected-note@+1 {{to match this '('}}
    #pragma acc CMB_PAR loop vector reduction(jk
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{expected ')'}}
    // expected-note@+1 {{to match this '('}}
    #pragma acc CMB_PAR loop vector reduction(+: jk
    for (int i = 0; i < 5; ++i)
      ;

    // expected-error@+1 {{expected expression}}
    #pragma acc CMB_PAR loop gang worker private()
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{expected reduction operator}}
    // expected-warning@+1 {{missing ':' after reduction operator - ignoring}}
    #pragma acc CMB_PAR loop gang vector reduction( )
    for (int i = 0; i < 5; ++i)
      ;

    // expected-error@+1 {{expected reduction operator}}
    #pragma acc CMB_PAR loop worker vector reduction( : )
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{expected reduction operator}}
    #pragma acc CMB_PAR loop gang worker vector reduction(: i)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{expected reduction operator}}
    // expected-error@+1 {{use of undeclared identifier 'foo'}}
    #pragma acc CMB_PAR loop reduction(:foo )
    for (int i = 0; i < 5; ++i)
      ;

    // expected-error@+2 {{expected reduction operator}}
    // expected-warning@+1 {{missing ':' after reduction operator - ignoring}}
    #pragma acc CMB_PAR loop reduction(-) gang
    for (int i = 0; i < 5; ++i)
      ;
    // expected-warning@+2 {{missing ':' after reduction operator - ignoring}}
    // expected-error@+1 {{expected expression}}
    #pragma acc CMB_PAR loop reduction(i) worker
    for (int i = 0; i < 5; ++i)
      ;
    // expected-warning@+2 {{missing ':' after reduction operator - ignoring}}
    // expected-error@+1 {{expected expression}}
    #pragma acc CMB_PAR loop reduction(foo) vector
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{expected reduction operator}}
    #pragma acc CMB_PAR loop reduction(-:) worker gang
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{expected expression}}
    #pragma acc CMB_PAR loop reduction(foo:) vector gang
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{expected reduction operator}}
    #pragma acc CMB_PAR loop reduction(-:i) vector worker
    for (int i = 0; i < 5; ++i)
      ;
    // OpenACC 2.6 sec. 2.5.12 line 774 mistypes "^" as "%", which is nonsense as a
    // reduction operator.
    // expected-error@+1 {{expected reduction operator}}
    #pragma acc CMB_PAR loop reduction(% :i) gang vector worker
    for (int i = 0; i < 5; ++i)
      ;

    // expected-warning@+2 {{missing ':' after reduction operator - ignoring}}
    // expected-error@+1 {{expected expression}}
    #pragma acc CMB_PAR loop vector reduction(*)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-warning@+2 {{missing ':' after reduction operator - ignoring}}
    // expected-error@+1 {{expected expression}}
    #pragma acc CMB_PAR loop gang reduction(max) worker
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{expected expression}}
    #pragma acc CMB_PAR loop gang reduction(*:) vector
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{expected expression}}
    #pragma acc CMB_PAR loop worker reduction(min:) vector
    for (int i = 0; i < 5; ++i)
      ;

    // expected-error@+1 {{expected ',' or ')' in 'private' clause}}
    #pragma acc CMB_PAR loop worker private(jk i) gang vector
    for (int i = 0; i < 5; ++i)
      ;
  }
#if !CMB
  #pragma acc parallel
#endif
  {
    // expected-error@+1 {{expected ',' or ')' in 'reduction' clause}}
    #pragma acc CMB_PAR loop seq reduction(*:jk i)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{expected expression}}
    #pragma acc CMB_PAR loop private(jk ,) vector
    for (int i = 0; i < 5; ++i)
      ;
  }
#if !CMB
  #pragma acc parallel
#endif
  {
    // expected-error@+1 {{expected expression}}
    #pragma acc CMB_PAR loop worker reduction(+:i, ) gang
    for (int i = 0; i < 5; ++i)
      ;
  }
#if !CMB
  #pragma acc parallel
#endif
  {
    // expected-error@+1 {{expected variable name}}
    #pragma acc CMB_PAR loop private(-i)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{expected variable name}}
    #pragma acc CMB_PAR loop reduction(+ : (int)i)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{expected variable name as base of subarray}}
    // expected-error@+1 {{subarray is not supported in 'private' clause}}
    #pragma acc CMB_PAR loop private(((int*)a)[0:2])
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{expected variable name as base of subarray}}
    // expected-error@+1 {{subarray is not supported in 'reduction' clause}}
    #pragma acc CMB_PAR loop reduction(*:((int*)a)[:2])
    for (int i = 0; i < 5; ++i)
      ;
  }

  //--------------------------------------------------
  // Data clauses: arg semantics
  //--------------------------------------------------

  // Unknown reduction operator.
#if !CMB
  #pragma acc parallel
#endif
  {
    // expected-error@+1 {{unknown reduction operator}}
    #pragma acc CMB_PAR loop reduction(foo:i)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{use of undeclared identifier 'bar'}}
    #pragma acc CMB_PAR loop gang reduction(foo:bar)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{unknown reduction operator}}
    #pragma acc CMB_PAR loop worker reduction(foo : a[3])
    for (int i = 0; i < 5; ++i)
      ;
  }

  // Unknown variable name.
#if !CMB
  #pragma acc parallel
#endif
  {
    // expected-error@+1 {{use of undeclared identifier 'foo'}}
    #pragma acc CMB_PAR loop private(foo ) gang
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{use of undeclared identifier 'bar'}}
    #pragma acc CMB_PAR loop reduction(*: bar ) worker
    for (int i = 0; i < 5; ++i)
      ;
  }

  // Subarrays not permitted.
#if !CMB
  #pragma acc parallel
#endif
  {
    // expected-error@+6 {{subarray syntax must include ':'}}
    // expected-error@+5 {{subarray is not supported in 'private' clause}}
    // expected-error@+5 {{subarray is not supported in 'private' clause}}
    // expected-error@+5 {{subarray is not supported in 'private' clause}}
    // expected-error@+5 {{subarray is not supported in 'private' clause}}
    // expected-error@+5 {{subarray is not supported in 'private' clause}}
    #pragma acc CMB_PAR loop vector private(a[9],   \
                                            a[0:1], \
                                            a[0:],  \
                                            a[:1],  \
                                            a[:]) gang
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+6 {{subarray is not supported in 'reduction' clause}}
    // expected-error@+6 {{subarray syntax must include ':'}}
    // expected-error@+5 {{subarray is not supported in 'reduction' clause}}
    // expected-error@+5 {{subarray is not supported in 'reduction' clause}}
    // expected-error@+5 {{subarray is not supported in 'reduction' clause}}
    // expected-error@+5 {{subarray is not supported in 'reduction' clause}}
    #pragma acc CMB_PAR loop vector reduction(*:a[3:5], \
                                                a[10],  \
                                                a[2:],  \
                                                a[:9],  \
                                                a[:]) worker
    for (int i = 0; i < 5; ++i)
      ;
  }

  // Variables of incomplete type.
#if !CMB
  #pragma acc parallel
#endif
  {
    // expected-error@+2 {{variable in 'private' clause cannot have incomplete type 'int []'}}
    // expected-error@+1 {{variable in 'reduction' clause cannot have incomplete type 'int []'}}
    #pragma acc CMB_PAR loop worker private(incomplete) vector reduction(|:incomplete) gang
    for (int i = 0; i < 5; ++i)
      ;
  }

  // Variables of const type.
#if !CMB
  #pragma acc parallel
#endif
  {
    // expected-error@+4 {{const variable cannot be private because initialization is impossible}}
    // expected-error@+3 {{const variable cannot be private because initialization is impossible}}
    // expected-error@+2 {{reduction variable cannot be const}}
    // expected-error@+1 {{reduction variable cannot be const}}
    #pragma acc CMB_PAR loop private(constI, constIDecl) reduction(+: constI, constIDecl)
    for (int i = 0; i < 5; ++i)
      ;
  }

  // Reduction on loop control variable.
#if !CMB
  #pragma acc parallel
#endif
  {
    // expected-error@+2 {{OpenACC loop control variable 'i' cannot have reduction}}
    // expected-note@+2 {{'i' is an OpenACC loop control variable here}}
    #pragma acc CMB_PAR loop reduction(^:i)
    for (i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{OpenACC loop control variable 'i' cannot have reduction}}
    // expected-note@+2 {{'i' is an OpenACC loop control variable here}}
    #pragma acc CMB_PAR loop reduction(^:jk, i) gang
    for (i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{OpenACC loop control variable 'i' cannot have reduction}}
    // expected-note@+2 {{'i' is an OpenACC loop control variable here}}
    #pragma acc CMB_PAR loop reduction(^:jk) reduction(+:i) worker
    for (i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{use of undeclared identifier 'foo'}}
    #pragma acc CMB_PAR loop reduction(^:foo)
    for (int foo = 0; foo < 5; ++foo)
      ;

    // expected-error@+2 {{OpenACC loop control variable 'i' cannot have reduction}}
    // expected-note@+2 {{'i' is an OpenACC loop control variable here}}
    #pragma acc CMB_PAR loop reduction(*:i) vector collapse(2)
    for (i = 0; i < 5; ++i)
      for (jk = 0; jk < 5; ++jk)
        ;
    // expected-error@+2 {{OpenACC loop control variable 'jk' cannot have reduction}}
    // expected-note@+3 {{'jk' is an OpenACC loop control variable here}}
    #pragma acc CMB_PAR loop reduction(max:jk) vector collapse(2)
    for (i = 0; i < 5; ++i)
      for (jk = 0; jk < 5; ++jk)
        ;
    {
      int i, j, k;
      // expected-error@+2 {{OpenACC loop control variable 'i' cannot have reduction}}
      // expected-note@+2 {{'i' is an OpenACC loop control variable here}}
      #pragma acc CMB_PAR loop reduction(min:i) vector collapse(3)
      for (i = 0; i < 5; ++i)
        for (j = 0; j < 5; ++j)
          for (k = 0; k < 5; ++k)
            ;
      // expected-error@+2 {{OpenACC loop control variable 'j' cannot have reduction}}
      // expected-note@+3 {{'j' is an OpenACC loop control variable here}}
      #pragma acc CMB_PAR loop reduction(&:i,j,k) vector collapse(3)
      for (int i = 0; i < 5; ++i)
        for (j = 0; j < 5; ++j)
          for (int k = 0; k < 5; ++k)
            ;
      // expected-error@+2 {{OpenACC loop control variable 'k' cannot have reduction}}
      // expected-note@+4 {{'k' is an OpenACC loop control variable here}}
      #pragma acc CMB_PAR loop reduction(|:k) vector collapse(3)
      for (i = 0; i < 5; ++i)
        for (j = 0; j < 5; ++j)
          for (k = 0; k < 5; ++k)
            ;
    }
  }

  // Reduction operator doesn't permit variable type.
#if !CMB
  #pragma acc parallel
#endif
  {
    #pragma acc CMB_PAR loop gang reduction(max:b,e,i,jk,f,d,p)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+6 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
    // expected-error@+5 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
    // expected-error@+4 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
    // expected-error@+3 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
    // expected-error@+2 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
    // expected-error@+1 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
    #pragma acc CMB_PAR loop worker reduction(max:fc,dc,a,s,u,sDecl)
    for (int i = 0; i < 5; ++i)
      ;
  }
#if !CMB
  #pragma acc parallel
#endif
  {
    #pragma acc CMB_PAR loop vector reduction(min:b,e,i,jk,f,d,p)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+6 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
    // expected-error@+5 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
    // expected-error@+4 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
    // expected-error@+3 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
    // expected-error@+2 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
    // expected-error@+1 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
    #pragma acc CMB_PAR loop worker gang reduction(min:fc,dc,a,s,u,sDecl)
    for (int i = 0; i < 5; ++i)
      ;
  }
#if !CMB
  #pragma acc parallel
#endif
  {
    #pragma acc CMB_PAR loop vector gang reduction(+:b,e,i,jk,f,d,fc,dc)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+5 {{OpenACC reduction operator '+' argument must be of arithmetic type}}
    // expected-error@+4 {{OpenACC reduction operator '+' argument must be of arithmetic type}}
    // expected-error@+3 {{OpenACC reduction operator '+' argument must be of arithmetic type}}
    // expected-error@+2 {{OpenACC reduction operator '+' argument must be of arithmetic type}}
    // expected-error@+1 {{OpenACC reduction operator '+' argument must be of arithmetic type}}
    #pragma acc CMB_PAR loop vector worker reduction(+:p,a,s,u,sDecl)
    for (int i = 0; i < 5; ++i)
      ;
  }
#if !CMB
  #pragma acc parallel
#endif
  {
    #pragma acc CMB_PAR loop vector gang reduction(*:b,e,i,jk,f,d,fc,dc) worker
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+5 {{OpenACC reduction operator '*' argument must be of arithmetic type}}
    // expected-error@+4 {{OpenACC reduction operator '*' argument must be of arithmetic type}}
    // expected-error@+3 {{OpenACC reduction operator '*' argument must be of arithmetic type}}
    // expected-error@+2 {{OpenACC reduction operator '*' argument must be of arithmetic type}}
    // expected-error@+1 {{OpenACC reduction operator '*' argument must be of arithmetic type}}
    #pragma acc CMB_PAR loop reduction(*:p,a,s,u,sDecl)
    for (int i = 0; i < 5; ++i)
      ;
  }
#if !CMB
  #pragma acc parallel
#endif
  {
    #pragma acc CMB_PAR loop reduction(&&:b,e,i,jk,f,d,fc,dc) gang
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+5 {{OpenACC reduction operator '&&' argument must be of arithmetic type}}
    // expected-error@+4 {{OpenACC reduction operator '&&' argument must be of arithmetic type}}
    // expected-error@+3 {{OpenACC reduction operator '&&' argument must be of arithmetic type}}
    // expected-error@+2 {{OpenACC reduction operator '&&' argument must be of arithmetic type}}
    // expected-error@+1 {{OpenACC reduction operator '&&' argument must be of arithmetic type}}
    #pragma acc CMB_PAR loop reduction(&&:p,a,s,u,sDecl) worker
    for (int i = 0; i < 5; ++i)
      ;
  }
#if !CMB
  #pragma acc parallel
#endif
  {
    #pragma acc CMB_PAR loop reduction(||:b,e,i,jk,f,d,fc,dc) vector
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+5 {{OpenACC reduction operator '||' argument must be of arithmetic type}}
    // expected-error@+4 {{OpenACC reduction operator '||' argument must be of arithmetic type}}
    // expected-error@+3 {{OpenACC reduction operator '||' argument must be of arithmetic type}}
    // expected-error@+2 {{OpenACC reduction operator '||' argument must be of arithmetic type}}
    // expected-error@+1 {{OpenACC reduction operator '||' argument must be of arithmetic type}}
    #pragma acc CMB_PAR loop gang reduction(||:p,a,s,u,sDecl) worker
    for (int i = 0; i < 5; ++i)
      ;
  }
#if !CMB
  #pragma acc parallel
#endif
  {
    #pragma acc CMB_PAR loop gang reduction(&:b,e,i,jk) vector
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+9 {{OpenACC reduction operator '&' argument must be of integer type}}
    // expected-error@+8 {{OpenACC reduction operator '&' argument must be of integer type}}
    // expected-error@+7 {{OpenACC reduction operator '&' argument must be of integer type}}
    // expected-error@+6 {{OpenACC reduction operator '&' argument must be of integer type}}
    // expected-error@+5 {{OpenACC reduction operator '&' argument must be of integer type}}
    // expected-error@+4 {{OpenACC reduction operator '&' argument must be of integer type}}
    // expected-error@+3 {{OpenACC reduction operator '&' argument must be of integer type}}
    // expected-error@+2 {{OpenACC reduction operator '&' argument must be of integer type}}
    // expected-error@+1 {{OpenACC reduction operator '&' argument must be of integer type}}
    #pragma acc CMB_PAR loop vector worker reduction(&:f,d,fc,dc,p,a,s,u,sDecl) gang
    for (int i = 0; i < 5; ++i)
      ;
  }
#if !CMB
  #pragma acc parallel
#endif
  {
    #pragma acc CMB_PAR loop reduction(|:b,e,i,jk)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+9 {{OpenACC reduction operator '|' argument must be of integer type}}
    // expected-error@+8 {{OpenACC reduction operator '|' argument must be of integer type}}
    // expected-error@+7 {{OpenACC reduction operator '|' argument must be of integer type}}
    // expected-error@+6 {{OpenACC reduction operator '|' argument must be of integer type}}
    // expected-error@+5 {{OpenACC reduction operator '|' argument must be of integer type}}
    // expected-error@+4 {{OpenACC reduction operator '|' argument must be of integer type}}
    // expected-error@+3 {{OpenACC reduction operator '|' argument must be of integer type}}
    // expected-error@+2 {{OpenACC reduction operator '|' argument must be of integer type}}
    // expected-error@+1 {{OpenACC reduction operator '|' argument must be of integer type}}
    #pragma acc CMB_PAR loop gang reduction(|:f,d,fc,dc,p,a,s,u,sDecl)
    for (int i = 0; i < 5; ++i)
      ;
  }
#if !CMB
  #pragma acc parallel
#endif
  {
    #pragma acc CMB_PAR loop worker reduction(^:b,e,i,jk)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+9 {{OpenACC reduction operator '^' argument must be of integer type}}
    // expected-error@+8 {{OpenACC reduction operator '^' argument must be of integer type}}
    // expected-error@+7 {{OpenACC reduction operator '^' argument must be of integer type}}
    // expected-error@+6 {{OpenACC reduction operator '^' argument must be of integer type}}
    // expected-error@+5 {{OpenACC reduction operator '^' argument must be of integer type}}
    // expected-error@+4 {{OpenACC reduction operator '^' argument must be of integer type}}
    // expected-error@+3 {{OpenACC reduction operator '^' argument must be of integer type}}
    // expected-error@+2 {{OpenACC reduction operator '^' argument must be of integer type}}
    // expected-error@+1 {{OpenACC reduction operator '^' argument must be of integer type}}
    #pragma acc CMB_PAR loop vector reduction(^:f,d,fc,dc,p,a,s,u,sDecl)
    for (int i = 0; i < 5; ++i)
      ;
  }

  // Redundant clauses.
#if !CMB
  #pragma acc parallel
#endif
  {
    // expected-error@+6 {{private variable defined again as private variable}}
    // expected-note@+5 {{previously defined as private variable here}}
    // expected-error@+5 {{private variable defined again as private variable}}
    // expected-note@+3 {{previously defined as private variable here}}
    // expected-error@+4 {{private variable defined again as private variable}}
    // expected-note@+1 {{previously defined as private variable here}}
    #pragma acc CMB_PAR loop private(i,i,jk,d) \
                             private(jk) \
                             private(d)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+6 {{redundant 'max' reduction for variable 'i'}}
    // expected-note@+5 {{previous 'max' reduction here}}
    // expected-error@+5 {{redundant 'max' reduction for variable 'jk'}}
    // expected-note@+3 {{previous 'max' reduction here}}
    // expected-error@+4 {{conflicting '*' reduction for variable 'd'}}
    // expected-note@+1 {{previous 'max' reduction here}}
    #pragma acc CMB_PAR loop gang reduction(max:i,i,jk,d) worker \
                                  reduction(max:jk) \
                                  reduction(*:d)
    for (int i = 0; i < 5; ++i)
      ;
  }

  // Conflicting clauses.
#if !CMB
  #pragma acc parallel
#endif
  {
    // expected-error@+3 {{private variable cannot be reduction variable}}
    // expected-note@+1 {{previously defined as private variable here}}
    #pragma acc CMB_PAR loop private(i) gang \
                             reduction(&:i) vector
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+3 {{reduction variable cannot be private variable}}
    // expected-note@+1 {{previously defined as reduction variable here}}
    #pragma acc CMB_PAR loop gang reduction(&&:i) worker \
                             private(i) vector
    for (int i = 0; i < 5; ++i)
      ;
  }

  // Conflicting predetermined private (for loop control variables).
  //
  // parallel-messages.c checks conflicts with explicit private.
  //
  // Reductions for loop control variables are a separate restriction and are
  // checked above.
  //
  // The only remaining conflicts are with clauses that are permitted only on
  // the parallel construct, and those conflicts only occur on a combined
  // construct.
  // cmb-error-re@+6 {{{{present|copy|copyin|copyout|create|no_create}} variable cannot be predetermined private variable}}
  // cmb-note-re@+1 {{previously defined as {{present|copy|copyin|copyout|create|no_create}} variable here}}
  #pragma acc parallel CMB_LOOP DATA(i)
#if !CMB
  #pragma acc loop
#endif
  for (i = 0; i < 5; ++i)
    ;
  // cmb-error@+6 {{firstprivate variable cannot be predetermined private variable}}
  // cmb-note@+1 {{previously defined as firstprivate variable here}}
  #pragma acc parallel CMB_LOOP firstprivate(i)
#if !CMB
  #pragma acc loop
#endif
  for (i = 0; i < 5; ++i)
    ;
  {
    int i, j, k, l, m;
    // cmb-error-re@+9 {{{{present|copy|copyin|copyout|create|no_create}} variable cannot be predetermined private variable}}
    // cmb-error@+9 {{firstprivate variable cannot be predetermined private variable}}
    // cmb-note-re@+2 {{previously defined as {{present|copy|copyin|copyout|create|no_create}} variable here}}
    // cmb-note@+1 {{previously defined as firstprivate variable here}}
    #pragma acc parallel CMB_LOOP_COLLAPSE(3) DATA(j, l) firstprivate(k, m)
#if !CMB
    #pragma acc loop collapse(3)
#endif
    for (i = 0; i < 5; ++i)
      for (j = 0; j < 5; ++j)
        for (k = 0; k < 5; ++k)
          for (l = 0; l < 5; ++l)
            for (m = 0; m < 5; ++m)
              ;
  }

  //--------------------------------------------------
  // Data clauses: reduction inter-directive conflicts
  //--------------------------------------------------

  // Explicit reduction on acc parallel, non-conflicting reductions on acc
  // loops.
  #pragma acc parallel CMB_LOOP_SEQ reduction(+:i,jk) DATA(jk)
  CMB_FORLOOP_HEAD
  {
    #pragma acc loop worker reduction(+:i,jk)
    for (int j = 0; j < 5; ++j)
      ;
    #pragma acc loop reduction(+:i,jk) gang
    for (int j = 0; j < 5; ++j)
      ;
    #pragma acc loop vector reduction(+:i,jk)
    for (int j = 0; j < 5; ++j)
      ;
    #pragma acc loop seq reduction(+:i,jk)
    for (int j = 0; j < 5; ++j)
      ;
    #pragma acc loop gang reduction(+:i,jk) worker
    for (int j = 0; j < 5; ++j)
      ;
    #pragma acc loop gang reduction(+:i,jk)
    for (int j = 0; j < 5; ++j) {
      #pragma acc loop worker reduction(+:i,jk)
      for (int j = 0; j < 5; ++j) {
        #pragma acc loop vector reduction(+:i,jk)
        for (int j = 0; j < 5; ++j)
          ;
      }
    }
    #pragma acc loop worker reduction(+:i,jk)
    for (int j = 0; j < 5; ++j) {
      #pragma acc loop vector reduction(+:i,jk)
      for (int j = 0; j < 5; ++j)
        ;
    }
  }
  // Again with no explicit gang loops.
  #pragma acc parallel CMB_LOOP_SEQ reduction(+:i,jk) DATA(jk)
  CMB_FORLOOP_HEAD
  {
    #pragma acc loop worker reduction(+:i,jk)
    for (int j = 0; j < 5; ++j)
      ;
    #pragma acc loop vector reduction(+:i,jk)
    for (int j = 0; j < 5; ++j)
      ;
    #pragma acc loop seq reduction(+:i,jk)
    for (int j = 0; j < 5; ++j)
      ;
    #pragma acc loop reduction(+:i,jk) vector worker
    for (int j = 0; j < 5; ++j)
      ;
    #pragma acc loop reduction(+:i,jk) worker
    for (int j = 0; j < 5; ++j) {
      #pragma acc loop reduction(+:i,jk) vector
      for (int j = 0; j < 5; ++j)
        ;
    }
  }
  // Again with no gang loops.
  #pragma acc parallel CMB_LOOP_SEQ reduction(+:i,jk) DATA(jk)
  CMB_FORLOOP_HEAD
  {
    #pragma acc loop seq reduction(+:i,jk)
    for (int j = 0; j < 5; ++j)
      ;
    #pragma acc loop auto reduction(+:i,jk)
    for (int j = 0; j < 5; ++j)
      ;
    #pragma acc loop reduction(+:i,jk) seq
    for (int j = 0; j < 5; ++j) {
      #pragma acc loop reduction(+:i,jk) auto
      for (int j = 0; j < 5; ++j)
        ;
    }
  }

  // Implicit reduction on acc parallel, non-conflicting reductions on acc
  // loops.
  #pragma acc parallel CMB_LOOP_SEQ DATA(b)
  CMB_FORLOOP_HEAD
  {
    #pragma acc loop worker reduction(max:p,b)
    for (int j = 0; j < 5; ++j)
      ;
    #pragma acc loop reduction(max:p,b) gang worker vector
    for (int j = 0; j < 5; ++j)
      ;
    #pragma acc loop reduction(max:p,b) vector
    for (int j = 0; j < 5; ++j)
      ;
    #pragma acc loop reduction(max:p,b) seq
    for (int j = 0; j < 5; ++j)
      ;
    #pragma acc loop gang reduction(max:p,b)
    for (int j = 0; j < 5; ++j)
      ;
    #pragma acc loop gang worker reduction(max:p,b)
    for (int j = 0; j < 5; ++j) {
      #pragma acc loop vector reduction(max:p,b)
      for (int j = 0; j < 5; ++j)
        ;
    }
    #pragma acc loop worker reduction(max:p,b)
    for (int j = 0; j < 5; ++j) {
      #pragma acc loop vector reduction(max:p,b)
      for (int j = 0; j < 5; ++j)
        ;
    }
  }
  // Again with no explicit gang loops.
  #pragma acc parallel CMB_LOOP_SEQ DATA(p)
  CMB_FORLOOP_HEAD
  {
    #pragma acc loop worker reduction(max:p,b)
    for (int j = 0; j < 5; ++j)
      ;
    #pragma acc loop reduction(max:p,b) vector
    for (int j = 0; j < 5; ++j)
      ;
    #pragma acc loop reduction(max:p,b) seq
    for (int j = 0; j < 5; ++j)
      ;
    #pragma acc loop reduction(max:p,b) worker
    for (int j = 0; j < 5; ++j) {
      #pragma acc loop vector reduction(max:p,b)
      for (int j = 0; j < 5; ++j)
        ;
    }
  }
  // Again with no gang loops.
  #pragma acc parallel CMB_LOOP_SEQ DATA(p)
  CMB_FORLOOP_HEAD
  {
    #pragma acc loop auto reduction(max:p,b)
    for (int j = 0; j < 5; ++j)
      ;
    #pragma acc loop reduction(max:p) seq reduction(min:b)
    for (int j = 0; j < 5; ++j)
      ;
    #pragma acc loop reduction(max:p) reduction(*:b) auto
    for (int j = 0; j < 5; ++j) {
      #pragma acc loop seq reduction(max:p) reduction(*:b)
      for (int j = 0; j < 5; ++j)
        ;
    }
  }

  // Explicit reduction on acc parallel, conflicting reductions on acc loops.
  // expected-note@+1 14 {{enclosing '+' reduction here}}
  #pragma acc parallel CMB_LOOP_SEQ reduction(+:f,d) DATA(d)
  CMB_FORLOOP_HEAD
  {
    // expected-error@+2 {{conflicting 'max' reduction for variable 'f'}}
    // expected-error@+1 {{conflicting 'max' reduction for variable 'd'}}
    #pragma acc loop worker reduction(max:f,d)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+2 {{conflicting '*' reduction for variable 'f'}}
    // expected-error@+1 {{conflicting '*' reduction for variable 'd'}}
    #pragma acc loop gang reduction(*:f,d)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+2 {{conflicting 'min' reduction for variable 'f'}}
    // expected-error@+1 {{conflicting 'min' reduction for variable 'd'}}
    #pragma acc loop vector reduction(min:f,d)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+2 {{conflicting '&&' reduction for variable 'f'}}
    // expected-error@+1 {{conflicting '&&' reduction for variable 'd'}}
    #pragma acc loop seq reduction(&&:f,d)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+5 {{redundant '||' reduction for variable 'f'}}
    // expected-error@+4 {{redundant '||' reduction for variable 'd'}}
    // expected-note@+3 2 {{previous '||' reduction here}}
    // expected-error@+2 {{conflicting '||' reduction for variable 'f'}}
    // expected-error@+1 {{conflicting '||' reduction for variable 'd'}}
    #pragma acc loop gang worker reduction(||:f,f,d,d)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+3 {{conflicting '||' reduction for variable 'f'}}
    // expected-error@+2 {{conflicting '||' reduction for variable 'd'}}
    // expected-note@+1 2 {{enclosing '||' reduction here}}
    #pragma acc loop gang reduction(||:f,d)
    for (int j = 0; j < 5; ++j) {
      // expected-error@+3 {{conflicting '&&' reduction for variable 'f'}}
      // expected-error@+2 {{conflicting '&&' reduction for variable 'd'}}
      // expected-note@+1 2 {{enclosing '&&' reduction here}}
      #pragma acc loop worker reduction(&&:f,d)
      for (int j = 0; j < 5; ++j) {
        // expected-error@+2 {{conflicting '*' reduction for variable 'f'}}
        // expected-error@+1 {{conflicting '*' reduction for variable 'd'}}
        #pragma acc loop vector reduction(*:f,d)
        for (int j = 0; j < 5; ++j)
          ;
      }
    }
    // expected-error@+3 {{conflicting '&&' reduction for variable 'f'}}
    // expected-error@+2 {{conflicting '&&' reduction for variable 'd'}}
    // expected-note@+1 2 {{enclosing '&&' reduction here}}
    #pragma acc loop worker reduction(&&:f,d)
    for (int j = 0; j < 5; ++j) {
      // expected-error@+2 {{conflicting '*' reduction for variable 'f'}}
      // expected-error@+1 {{conflicting '*' reduction for variable 'd'}}
      #pragma acc loop vector reduction(*:f,d)
      for (int j = 0; j < 5; ++j)
        ;
    }
  }
  // Again with no explicit gang loops.
  // expected-note@+1 10 {{enclosing '+' reduction here}}
  #pragma acc parallel CMB_LOOP_SEQ reduction(+:f,d) DATA(d)
  CMB_FORLOOP_HEAD
  {
    // expected-error@+2 {{conflicting 'max' reduction for variable 'f'}}
    // expected-error@+1 {{conflicting 'max' reduction for variable 'd'}}
    #pragma acc loop worker reduction(max:f,d)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+2 {{conflicting 'min' reduction for variable 'f'}}
    // expected-error@+1 {{conflicting 'min' reduction for variable 'd'}}
    #pragma acc loop vector reduction(min:f,d)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+2 {{conflicting '*' reduction for variable 'f'}}
    // expected-error@+1 {{conflicting '*' reduction for variable 'd'}}
    #pragma acc loop seq reduction(*:f,d)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+5 {{redundant '&&' reduction for variable 'f'}}
    // expected-error@+4 {{redundant '&&' reduction for variable 'd'}}
    // expected-note@+3 2 {{previous '&&' reduction here}}
    // expected-error@+2 {{conflicting '&&' reduction for variable 'f'}}
    // expected-error@+1 {{conflicting '&&' reduction for variable 'd'}}
    #pragma acc loop worker vector reduction(&&:f,f,d,d)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+3 {{conflicting '&&' reduction for variable 'f'}}
    // expected-error@+2 {{conflicting '&&' reduction for variable 'd'}}
    // expected-note@+1 2 {{enclosing '&&' reduction here}}
    #pragma acc loop worker reduction(&&:f,d)
    for (int j = 0; j < 5; ++j) {
      // expected-error@+2 {{conflicting '||' reduction for variable 'f'}}
      // expected-error@+1 {{conflicting '||' reduction for variable 'd'}}
      #pragma acc loop vector reduction(||:f,d)
      for (int j = 0; j < 5; ++j)
        ;
    }
  }
  // Again with no gang loops.
  // expected-note@+1 6 {{enclosing '+' reduction here}}
  #pragma acc parallel CMB_LOOP_SEQ reduction(+:f,d) DATA(d)
  CMB_FORLOOP_HEAD
  {
    // expected-error@+2 {{conflicting 'max' reduction for variable 'f'}}
    // expected-error@+1 {{conflicting 'max' reduction for variable 'd'}}
    #pragma acc loop seq reduction(max:f,d)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+2 {{conflicting '*' reduction for variable 'f'}}
    // expected-error@+1 {{conflicting '*' reduction for variable 'd'}}
    #pragma acc loop auto reduction(*:f,d)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+3 {{conflicting '&&' reduction for variable 'f'}}
    // expected-error@+2 {{conflicting '&&' reduction for variable 'd'}}
    // expected-note@+1 2 {{enclosing '&&' reduction here}}
    #pragma acc loop seq reduction(&&:f,d)
    for (int j = 0; j < 5; ++j) {
      // expected-error@+2 {{conflicting '||' reduction for variable 'f'}}
      // expected-error@+1 {{conflicting '||' reduction for variable 'd'}}
      #pragma acc loop seq reduction(||:f,d)
      for (int j = 0; j < 5; ++j)
        ;
    }
  }

  // Implicit reduction on acc parallel, conflicting reductions on acc loops.
  // expected-note@+1 12 {{implied as gang reduction here}}
  #pragma acc parallel CMB_LOOP_SEQ DATA(jk)
  CMB_FORLOOP_HEAD
  {
    // expected-note@+1 12 {{enclosing 'max' reduction here}}
    #pragma acc loop gang reduction(max:d,jk)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+2 {{conflicting 'min' reduction for variable 'd'}}
    // expected-error@+1 {{conflicting 'min' reduction for variable 'jk'}}
    #pragma acc loop worker reduction(min:d) reduction(min:jk)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+2 {{conflicting '&&' reduction for variable 'd'}}
    // expected-error@+1 {{conflicting '&&' reduction for variable 'jk'}}
    #pragma acc loop vector reduction(&&:d) reduction(&&:jk)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+2 {{conflicting '||' reduction for variable 'd'}}
    // expected-error@+1 {{conflicting '||' reduction for variable 'jk'}}
    #pragma acc loop seq reduction(||:d) reduction(||:jk)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+5 {{redundant '+' reduction for variable 'd'}}
    // expected-error@+4 {{redundant '+' reduction for variable 'jk'}}
    // expected-note@+3 2 {{previous '+' reduction here}}
    // expected-error@+2 {{conflicting '+' reduction for variable 'd'}}
    // expected-error@+1 {{conflicting '+' reduction for variable 'jk'}}
    #pragma acc loop gang worker vector reduction(+:d,jk) reduction(+:jk,d)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+3 {{conflicting '+' reduction for variable 'd'}}
    // expected-error@+2 {{conflicting '+' reduction for variable 'jk'}}
    // expected-note@+1 2 {{enclosing '+' reduction here}}
    #pragma acc loop gang reduction(+:d,jk)
    for (int j = 0; j < 5; ++j) {
      // expected-error@+3 {{conflicting '*' reduction for variable 'd'}}
      // expected-error@+2 {{conflicting '*' reduction for variable 'jk'}}
      // expected-note@+1 2 {{enclosing '*' reduction here}}
      #pragma acc loop worker reduction(*:d,jk)
      for (int j = 0; j < 5; ++j) {
        // expected-error@+2 {{conflicting 'max' reduction for variable 'd'}}
        // expected-error@+1 {{conflicting 'max' reduction for variable 'jk'}}
        #pragma acc loop vector reduction(max:d,jk)
        for (int j = 0; j < 5; ++j)
          ;
      }
    }
    // expected-error@+3 {{conflicting '*' reduction for variable 'd'}}
    // expected-error@+2 {{conflicting '*' reduction for variable 'jk'}}
    // expected-note@+1 2 {{enclosing '*' reduction here}}
    #pragma acc loop worker reduction(*:d,jk)
    for (int j = 0; j < 5; ++j) {
      // expected-error@+2 {{conflicting 'max' reduction for variable 'd'}}
      // expected-error@+1 {{conflicting 'max' reduction for variable 'jk'}}
      #pragma acc loop vector reduction(max:d,jk)
      for (int j = 0; j < 5; ++j)
        ;
    }
  }
  // Again with no explicit gang loops and worker first.
  // expected-note@+1 8 {{implied as gang reduction here}}
  #pragma acc parallel CMB_LOOP_SEQ DATA(jk)
  CMB_FORLOOP_HEAD
  {
    // expected-note@+1 8 {{enclosing 'max' reduction here}}
    #pragma acc loop worker reduction(max:d) reduction(max:jk)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+2 {{conflicting 'min' reduction for variable 'd'}}
    // expected-error@+1 {{conflicting 'min' reduction for variable 'jk'}}
    #pragma acc loop vector reduction(min:d) reduction(min:jk)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+2 {{conflicting '&&' reduction for variable 'd'}}
    // expected-error@+1 {{conflicting '&&' reduction for variable 'jk'}}
    #pragma acc loop seq reduction(&&:d) reduction(&&:jk)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+5 {{redundant '||' reduction for variable 'd'}}
    // expected-error@+4 {{redundant '||' reduction for variable 'jk'}}
    // expected-note@+3 2 {{previous '||' reduction here}}
    // expected-error@+2 {{conflicting '||' reduction for variable 'd'}}
    // expected-error@+1 {{conflicting '||' reduction for variable 'jk'}}
    #pragma acc loop worker vector reduction(||:d,jk) reduction(||:jk,d)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+3 {{conflicting '||' reduction for variable 'd'}}
    // expected-error@+2 {{conflicting '||' reduction for variable 'jk'}}
    // expected-note@+1 2 {{enclosing '||' reduction here}}
    #pragma acc loop worker reduction(||:d,jk)
    for (int j = 0; j < 5; ++j) {
      // expected-error@+2 {{conflicting '+' reduction for variable 'd'}}
      // expected-error@+1 {{conflicting '+' reduction for variable 'jk'}}
      #pragma acc loop vector reduction(+:d,jk)
      for (int j = 0; j < 5; ++j)
        ;
    }
  }
  // Again with no explicit gang loops and vector first.
  // expected-note@+1 8 {{implied as gang reduction here}}
  #pragma acc parallel CMB_LOOP_SEQ DATA(jk)
  CMB_FORLOOP_HEAD
  {
    // expected-note@+1 8 {{enclosing 'max' reduction here}}
    #pragma acc loop vector reduction(max:d) reduction(max:jk)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+2 {{conflicting 'min' reduction for variable 'd'}}
    // expected-error@+1 {{conflicting 'min' reduction for variable 'jk'}}
    #pragma acc loop seq reduction(min:d) reduction(min:jk)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+2 {{conflicting '&&' reduction for variable 'd'}}
    // expected-error@+1 {{conflicting '&&' reduction for variable 'jk'}}
    #pragma acc loop worker reduction(&&:d) reduction(&&:jk)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+5 {{redundant '||' reduction for variable 'd'}}
    // expected-error@+4 {{redundant '||' reduction for variable 'jk'}}
    // expected-note@+3 2 {{previous '||' reduction here}}
    // expected-error@+2 {{conflicting '||' reduction for variable 'd'}}
    // expected-error@+1 {{conflicting '||' reduction for variable 'jk'}}
    #pragma acc loop worker vector reduction(||:d,jk) reduction(||:jk,d)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+3 {{conflicting '||' reduction for variable 'd'}}
    // expected-error@+2 {{conflicting '||' reduction for variable 'jk'}}
    // expected-note@+1 2 {{enclosing '||' reduction here}}
    #pragma acc loop worker reduction(||:d,jk)
    for (int j = 0; j < 5; ++j) {
      // expected-error@+2 {{conflicting '+' reduction for variable 'd'}}
      // expected-error@+1 {{conflicting '+' reduction for variable 'jk'}}
      #pragma acc loop vector reduction(+:d,jk)
      for (int j = 0; j < 5; ++j)
        ;
    }
  }
  // Again with no explicit gang loops and seq first.
  // expected-note@+1 8 {{implied as gang reduction here}}
  #pragma acc parallel CMB_LOOP_SEQ DATA(f)
  CMB_FORLOOP_HEAD
  {
    // expected-note@+1 8 {{enclosing 'min' reduction here}}
    #pragma acc loop seq reduction(min:d) reduction(min:f)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+2 {{conflicting '&&' reduction for variable 'd'}}
    // expected-error@+1 {{conflicting '&&' reduction for variable 'f'}}
    #pragma acc loop vector reduction(&&:d) reduction(&&:f)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+2 {{conflicting 'max' reduction for variable 'd'}}
    // expected-error@+1 {{conflicting 'max' reduction for variable 'f'}}
    #pragma acc loop worker reduction(max:d) reduction(max:f)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+5 {{redundant '*' reduction for variable 'd'}}
    // expected-error@+4 {{redundant '*' reduction for variable 'f'}}
    // expected-note@+3 2 {{previous '*' reduction here}}
    // expected-error@+2 {{conflicting '*' reduction for variable 'd'}}
    // expected-error@+1 {{conflicting '*' reduction for variable 'f'}}
    #pragma acc loop worker vector reduction(*:d,f) reduction(*:f,d)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+3 {{conflicting '||' reduction for variable 'd'}}
    // expected-error@+2 {{conflicting '||' reduction for variable 'f'}}
    // expected-note@+1 2 {{enclosing '||' reduction here}}
    #pragma acc loop worker reduction(||:d,f)
    for (int j = 0; j < 5; ++j) {
      // expected-error@+2 {{conflicting '+' reduction for variable 'd'}}
      // expected-error@+1 {{conflicting '+' reduction for variable 'f'}}
      #pragma acc loop vector reduction(+:d,f)
      for (int j = 0; j < 5; ++j)
        ;
    }
  }
  // Again with no gang loops.
  // expected-note@+1 3 {{implied as gang reduction here}}
  #pragma acc parallel CMB_LOOP_SEQ DATA(jk)
  CMB_FORLOOP_HEAD
  {
    // expected-note@+1 3 {{enclosing 'max' reduction here}}
    #pragma acc loop seq reduction(max:d) reduction(max:jk)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+1 {{conflicting 'min' reduction for variable 'jk'}}
    #pragma acc loop auto reduction(min:d) reduction(min:jk)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+4 {{redundant '||' reduction for variable 'd'}}
    // expected-error@+3 {{redundant '||' reduction for variable 'jk'}}
    // expected-note@+2 2 {{previous '||' reduction here}}
    // expected-error@+1 {{conflicting '||' reduction for variable 'jk'}}
    #pragma acc loop seq reduction(||:d,jk) reduction(||:jk,d)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+2 {{conflicting '||' reduction for variable 'jk'}}
    // expected-note@+1 2 {{enclosing '||' reduction here}}
    #pragma acc loop seq reduction(||:d,jk)
    for (int j = 0; j < 5; ++j) {
      // expected-error@+2 {{conflicting '+' reduction for variable 'd'}}
      // expected-error@+1 {{conflicting '+' reduction for variable 'jk'}}
      #pragma acc loop auto reduction(+:d,jk)
      for (int j = 0; j < 5; ++j)
        ;
    }
  }

  // Implicit reduction on acc parallel, conflicting gang reductions on acc
  // loops in sibling loop nests.
  // expected-note@+1 2 {{implied as gang reduction here}}
  #pragma acc parallel CMB_LOOP_SEQ DATA(e)
  CMB_FORLOOP_HEAD
  {
    #pragma acc loop
    for (int i0 = 0; i0 < 5; ++i0) {
      #pragma acc loop seq
      for (int i1 = 0; i1 < 5; ++i1) {
        #pragma acc loop independent
        for (int i2 = 0; i2 < 5; ++i2) {
          #pragma acc loop auto
          for (int i3 = 0; i3 < 5; ++i3) {
            // b's first loop is gang-partitioned, and second is not.
            // expected-note@+1 {{enclosing '&' reduction here}}
            #pragma acc loop vector reduction(&:b) gang
            for (int j = 0; j < 5; ++j)
              ;
            // e's first loop is not gang-partitioned but has gang reduction.
            // expected-note@+1 {{enclosing '&&' reduction here}}
            #pragma acc loop seq reduction(&&:e)
            for (int j = 0; j < 5; ++j)
              ;
          }
        }
      }
    }
    #pragma acc loop seq
    for (int i = 0; i < 5; ++i) {
      // expected-error@+1 {{conflicting '|' reduction for variable 'e'}}
      #pragma acc loop worker reduction(|:e) vector gang
      for (int j = 0; j < 5; ++j)
        ;
      // expected-error@+1 {{conflicting '||' reduction for variable 'b'}}
      #pragma acc loop seq reduction(||:b)
      for (int j = 0; j < 5; ++j)
        ;
    }
  }

  // Local declarations prevent conflicts among gang reductions on acc loops,
  // but merely nesting in an acc loop seq doesn't.
  // expected-note@+2 6 {{implied as gang reduction here}}
  // expected-note@+1 6 {{enclosing '^' reduction here}}
  #pragma acc parallel CMB_LOOP_SEQ reduction(^:e,jk) DATA(jk,f)
  CMB_FORLOOP_HEAD
  {
    #pragma acc loop
    for (int i0 = 0; i0 < 5; ++i0) {
      enum E e;
      #pragma acc loop seq
      for (int i1 = 0; i1 < 5; ++i1) {
        int jk;
        #pragma acc loop independent
        for (int i2 = 0; i2 < 5; ++i2) {
          float f;
          #pragma acc loop auto
          for (int i3 = 0; i3 < 5; ++i3) {
            double d, localOnly;
            #pragma acc loop gang reduction(&&:e,d,localOnly,jk,f)
            for (int j = 0; j < 5; ++j)
              ;
            #pragma acc loop worker reduction(||:e,d,localOnly,jk,f)
            for (int j = 0; j < 5; ++j)
              ;
          }
        }
      }
    }
    #pragma acc loop seq
    for (int i = 0; i < 5; ++i) {
      // expected-error@+3 {{conflicting '||' reduction for variable 'e'}}
      // expected-error@+2 {{conflicting '||' reduction for variable 'jk'}}
      // expected-note@+1 6 {{enclosing '||' reduction here}}
      #pragma acc loop gang reduction(||:e,d,jk,f)
      for (int j = 0; j < 5; ++j)
        ;
      // expected-error@+4 {{conflicting 'max' reduction for variable 'e'}}
      // expected-error@+3 {{conflicting 'max' reduction for variable 'd'}}
      // expected-error@+2 {{conflicting 'max' reduction for variable 'jk'}}
      // expected-error@+1 {{conflicting 'max' reduction for variable 'f'}}
      #pragma acc loop worker reduction(max:e,d,jk,f)
      for (int j = 0; j < 5; ++j)
        ;
    }
    #pragma acc loop seq
    for (int i = 0; i < 5; ++i) {
      // expected-error@+2 {{conflicting '+' reduction for variable 'd'}}
      // expected-error@+1 {{conflicting '+' reduction for variable 'f'}}
      #pragma acc loop gang reduction(+:d,f)
      for (int j = 0; j < 5; ++j)
        ;
      // expected-error@+4 {{conflicting 'min' reduction for variable 'e'}}
      // expected-error@+3 {{conflicting 'min' reduction for variable 'd'}}
      // expected-error@+2 {{conflicting 'min' reduction for variable 'jk'}}
      // expected-error@+1 {{conflicting 'min' reduction for variable 'f'}}
      #pragma acc loop vector reduction(min:e,d,jk,f)
      for (int j = 0; j < 5; ++j)
        ;
    }
  }

  // Implicit gang clauses can produce implicit gang reductions.
  // expected-note@+1 {{implied as gang reduction here}}
  #pragma acc parallel CMB_LOOP_SEQ
  CMB_FORLOOP_HEAD
  {
    #pragma acc loop seq
    for (int i = 0; i < 5; ++i) {
      // expected-note@+1 {{enclosing '&&' reduction here}}
      #pragma acc loop reduction(&&:jk)
      for (int j = 0; j < 5; ++j)
        ;
    }
    #pragma acc loop seq
    for (int i = 0; i < 5; ++i) {
      // expected-error@+1 {{conflicting 'max' reduction for variable 'jk'}}
      #pragma acc loop reduction(max:jk)
      for (int j = 0; j < 5; ++j)
        ;
    }
  }
  // expected-note@+1 {{implied as gang reduction here}}
  #pragma acc parallel CMB_LOOP_SEQ
  CMB_FORLOOP_HEAD
  {
    #pragma acc loop seq
    for (int i = 0; i < 5; ++i) {
      // expected-note@+1 {{enclosing '&&' reduction here}}
      #pragma acc loop worker reduction(&&:jk)
      for (int j = 0; j < 5; ++j)
        ;
    }
    #pragma acc loop seq
    for (int i = 0; i < 5; ++i) {
      // expected-error@+1 {{conflicting 'max' reduction for variable 'jk'}}
      #pragma acc loop worker reduction(max:jk)
      for (int j = 0; j < 5; ++j)
        ;
    }
  }
  // expected-note@+1 {{implied as gang reduction here}}
  #pragma acc parallel CMB_LOOP_SEQ
  CMB_FORLOOP_HEAD
  {
    #pragma acc loop seq
    for (int i = 0; i < 5; ++i) {
      // expected-note@+1 {{enclosing '&&' reduction here}}
      #pragma acc loop vector reduction(&&:jk)
      for (int j = 0; j < 5; ++j)
        ;
    }
    #pragma acc loop seq
    for (int i = 0; i < 5; ++i) {
      // expected-error@+1 {{conflicting 'max' reduction for variable 'jk'}}
      #pragma acc loop vector reduction(max:jk)
      for (int j = 0; j < 5; ++j)
        ;
    }
  }
  // expected-note@+1 {{implied as gang reduction here}}
  #pragma acc parallel CMB_LOOP_SEQ
  CMB_FORLOOP_HEAD
  {
    #pragma acc loop seq
    for (int i = 0; i < 5; ++i) {
      // expected-note@+1 {{enclosing '&&' reduction here}}
      #pragma acc loop worker reduction(&&:jk)
      for (int j = 0; j < 5; ++j)
        ;
    }
    #pragma acc loop seq
    for (int i = 0; i < 5; ++i) {
      // expected-error@+1 {{conflicting 'max' reduction for variable 'jk'}}
      #pragma acc loop vector reduction(max:jk)
      for (int j = 0; j < 5; ++j)
        ;
    }
  }
  // expected-note@+1 {{implied as gang reduction here}}
  #pragma acc parallel CMB_LOOP_SEQ
  CMB_FORLOOP_HEAD
  {
    #pragma acc loop seq
    for (int i = 0; i < 5; ++i) {
      // expected-note@+1 {{enclosing '&&' reduction here}}
      #pragma acc loop worker vector reduction(&&:jk)
      for (int j = 0; j < 5; ++j)
        ;
    }
    #pragma acc loop seq
    for (int i = 0; i < 5; ++i) {
      // expected-error@+1 {{conflicting 'max' reduction for variable 'jk'}}
      #pragma acc loop worker vector reduction(max:jk)
      for (int j = 0; j < 5; ++j)
        ;
    }
  }

  // acc loop reductions for acc parallel firstprivate or private variables
  // don't imply gang reductions on acc parallel.
  #pragma acc parallel CMB_LOOP_SEQ firstprivate(i) private(jk)
  CMB_FORLOOP_HEAD
  {
    // expected-note@+1 2 {{enclosing '+' reduction here}}
    #pragma acc loop gang reduction(+:i,jk)
    for (int j = 0; j < 5; ++j) {
      // expected-error@+2 {{conflicting '*' reduction for variable 'i'}}
      // expected-error@+1 {{conflicting '*' reduction for variable 'jk'}}
      #pragma acc loop worker vector reduction(*:i,jk)
      for (int j = 0; j < 5; ++j)
        ;
    }
    // No conflict because no implied enclosing gang reduction.
    #pragma acc loop gang reduction(*:i,jk)
    for (int j = 0; j < 5; ++j)
      ;
    #pragma acc loop worker reduction(*:i,jk)
    for (int j = 0; j < 5; ++j)
      ;
    #pragma acc loop vector reduction(*:i,jk)
    for (int j = 0; j < 5; ++j)
      ;
  }

  // acc loop reductions for acc loop private variables don't imply gang
  // reductions on acc parallel.
  #pragma acc parallel CMB_LOOP_SEQ DATA(i,d)
  CMB_FORLOOP_HEAD
  {
    #pragma acc loop seq private(i,jk)
    for (int j = 0; j < 5; ++j) {
      // expected-note@+1 2 {{enclosing '+' reduction here}}
      #pragma acc loop gang reduction(+:i,jk) private(d)
      for (int j = 0; j < 5; ++j) {
        // expected-error@+2 {{conflicting '&&' reduction for variable 'i'}}
        // expected-error@+1 {{conflicting '&&' reduction for variable 'jk'}}
        #pragma acc loop worker vector reduction(&&:i,jk,d)
        for (int j = 0; j < 5; ++j)
          ;
      }
    }
    // No conflict because no implied enclosing gang reduction.
    #pragma acc loop gang reduction(*:jk,d)
    for (int j = 0; j < 5; ++j)
      ;
    #pragma acc loop worker reduction(*:i,d)
    for (int j = 0; j < 5; ++j)
      ;
  }

  //--------------------------------------------------
  // Orphaned acc loop
  //
  // Some of these cases used to fail an unnecessary assert in an analysis
  // required by the clauses.
  //--------------------------------------------------

  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop worker
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop vector
  for (int i = 0; i < 5; ++i)
    ;

  //--------------------------------------------------
  // Bad parent.
  //--------------------------------------------------

  // expected-error@+3 {{'#pragma acc loop' cannot be nested within '#pragma acc data'}}
  // expected-note@+1 {{enclosing '#pragma acc data'}}
  #pragma acc data DATA(i)
  #pragma acc loop
  for (int i = 0; i < 5; ++i)
    ;

  // expected-error@+4 {{'#pragma acc loop' cannot be nested within '#pragma acc data'}}
  // expected-note@+2 {{enclosing '#pragma acc data'}}
  #pragma acc data DATA(i)
  #pragma acc data DATA(jk)
  #pragma acc loop
  for (int i = 0; i < 5; ++i)
    ;

}

// Complete to suppress an additional warning, but it's too late for pragmas.
int incomplete[3];

//--------------------------------------------------
// The remaining diagnostics are currently produced by OpenMP sema during the
// transform from OpenACC to OpenMP, which is skipped if there have been any
// previous diagnostics.  Thus, each must be tested in a separate compilation.
//
// We don't check every OpenMP diagnostic case as the OpenMP tests should do
// that.  We just check some of them (roughly one of each diagnostic kind) to
// give us some confidence that they're handled properly by the OpenACC
// implementation.
//--------------------------------------------------

#elif ERR == ERR_OMP_INIT

void fn(int k) {
#if !CMB
  #pragma acc parallel
#endif
  {
    int j;
    #pragma acc CMB_PAR loop independent gang
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc CMB_PAR loop independent gang
    for (j = 0; j < 5; ++j)
      ;
    #pragma acc CMB_PAR loop independent gang
    for (k = 0; k < 5; ++k)
      ;
    #pragma acc CMB_PAR loop independent gang
    for (int i; i < 5; ++i) // expected-error {{initialization clause of OpenMP for loop is not in canonical form ('var = init' or 'T var = init')}}
      ;
  }
}

#elif ERR == ERR_OMP_COND

void fn(int j) {
#if !CMB
  #pragma acc parallel
#endif
  {
    #pragma acc CMB_PAR loop independent gang
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc CMB_PAR loop independent gang
    for (int i = 0; 5 > i; ++i)
      ;
    #pragma acc CMB_PAR loop independent gang
    for (int i = 0; i <= 5; ++i)
      ;
    #pragma acc CMB_PAR loop independent gang
    for (int i = 0; 5 >= i; ++i)
      ;
    #pragma acc CMB_PAR loop independent gang
    for (int i = 5; i > 0; --i)
      ;
    #pragma acc CMB_PAR loop independent gang
    for (int i = 5; 0 < i; --i)
      ;
    #pragma acc CMB_PAR loop independent gang
    for (int i = 5; i >= 0; --i)
      ;
    #pragma acc CMB_PAR loop independent gang
    for (int i = 5; 0 <= i; --i)
      ;
    #pragma acc CMB_PAR loop independent gang
    for (int i = 5; i && j; --i) // expected-error {{condition of OpenMP for loop must be a relational comparison ('<', '<=', '>', '>=', or '!=') of loop variable 'i'}}
      ;
  }
}

#elif ERR == ERR_OMP_INC

void fn(int j) {
#if !CMB
  #pragma acc parallel
#endif
  {
    #pragma acc CMB_PAR loop independent gang
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc CMB_PAR loop independent gang
    for (int i = 0; i < 5; i++)
      ;
    #pragma acc CMB_PAR loop independent gang
    for (int i = 0; i < 5; i += 2)
      ;
    #pragma acc CMB_PAR loop independent gang
    for (int i = 0; i < 5; i = i + 3)
      ;
    #pragma acc CMB_PAR loop independent gang
    for (int i = 5; i > 0; --i)
      ;
    #pragma acc CMB_PAR loop independent gang
    for (int i = 5; i > 0; i--)
      ;
    #pragma acc CMB_PAR loop independent gang
    for (int i = 5; i > 0; i -= 4)
      ;
    #pragma acc CMB_PAR loop independent gang
    for (int i = 5; i > 0; i = i - 5)
      ;
    #pragma acc CMB_PAR loop independent gang
    for (int i = 0; i < 5; i *= 2) // expected-error {{increment clause of OpenMP for loop must perform simple addition or subtraction on loop variable 'i'}}
      ;
  }
}

#elif ERR == ERR_OMP_INC0

void fn(int j) {
#if !CMB
  #pragma acc parallel
#endif
  {
    #pragma acc CMB_PAR loop independent gang
    // expected-error@+2 {{increment expression must cause 'i' to increase on each iteration of OpenMP for loop}}
    // expected-note@+1 {{loop step is expected to be positive due to this condition}}
    for (int i = 0; i < 5; i += 0)
      ;
  }
}

#elif ERR == ERR_OMP_VAR

void fn() {
#if !CMB
  #pragma acc parallel
#endif
  {
    int a[5];
    #pragma acc CMB_PAR loop independent gang
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc CMB_PAR loop independent gang
    for (int *p = a; p < a+5; ++p)
      ;
# if HAS_UINT128
    #pragma acc CMB_PAR loop independent gang
    for (__uint128_t i = 0; i < 5; ++i) // expected-warning {{OpenMP loop iteration variable cannot have more than 64 bits size and will be narrowed}}
      ;
# endif
    #pragma acc CMB_PAR loop independent gang
    for (float f = 0; f < 5; ++f) // expected-error {{variable must be of integer or pointer type}}
      ;
  }
}

#else
# error undefined ERR
#endif
