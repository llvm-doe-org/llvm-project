// RUN: %data {
// RUN:   (c=ERR_ACC     )
// RUN:   (c=ERR_OMP_INIT)
// RUN:   (c=ERR_OMP_COND)
// RUN:   (c=ERR_OMP_INC )
// RUN:   (c=ERR_OMP_INC0)
// RUN:   (c=ERR_OMP_VAR )
// RUN: }
// RUN: %for {
// RUN:   %clang_cc1 -verify -fsyntax-only -fopenacc -DERR=%[c] %s
// RUN: }

#include <stdint.h>

#define ERR_ACC       1
#define ERR_OMP_INIT  2
#define ERR_OMP_COND  3
#define ERR_OMP_INC   4
#define ERR_OMP_INC0  5
#define ERR_OMP_VAR   6

#ifdef __SIZEOF_INT128__
# define HAS_UINT128 1
#else
# define HAS_UINT128 0
#endif

#if ERR == ERR_ACC

int incomplete[];

void fn() {
  _Bool b;
  enum E { E1, E2 } e;
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
  #pragma acc parallel
  {
    // invalid clauses
    #pragma acc loop 500 // expected-warning {{extra tokens at the end of '#pragma acc loop' are ignored}}
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+3 {{unexpected OpenACC clause 'num_gangs' in directive '#pragma acc loop'}}
    // expected-error@+2 {{unexpected OpenACC clause 'num_workers' in directive '#pragma acc loop'}}
    // expected-error@+1 {{unexpected OpenACC clause 'vector_length' in directive '#pragma acc loop'}}
    #pragma acc loop num_gangs(3) num_workers(3) vector_length(3)
    for (int i = 0; i < 5; ++i)
      ;

    // partitionability clauses
    #pragma acc loop seq
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop independent
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop auto
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop seq() // expected-warning {{extra tokens at the end of '#pragma acc loop' are ignored}}
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop independent() // expected-warning {{extra tokens at the end of '#pragma acc loop' are ignored}}
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop auto() // expected-warning {{extra tokens at the end of '#pragma acc loop' are ignored}}
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop seq seq // expected-error {{directive '#pragma acc loop' cannot contain more than one 'seq' clause}}
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop independent independent // expected-error {{directive '#pragma acc loop' cannot contain more than one 'independent' clause}}
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop auto auto // expected-error {{directive '#pragma acc loop' cannot contain more than one 'auto' clause}}
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop seq independent // expected-error {{unexpected OpenACC 'independent' clause, 'seq' is specified already}}
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop auto seq // expected-error {{unexpected OpenACC 'seq' clause, 'auto' is specified already}}
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop independent auto // expected-error {{unexpected OpenACC 'auto' clause, 'independent' is specified already}}
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop auto seq independent // expected-error {{unexpected OpenACC 'seq' clause, 'auto' is specified already}}
                                          // expected-error@-1 {{unexpected OpenACC 'independent' clause, 'seq' is specified already}}
    for (int i = 0; i < 5; ++i)
      ;

    // partitioning clauses
    #pragma acc loop gang
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop worker
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop vector
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop gang() // expected-warning {{extra tokens at the end of '#pragma acc loop' are ignored}}
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop worker() // expected-warning {{extra tokens at the end of '#pragma acc loop' are ignored}}
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop vector() // expected-warning {{extra tokens at the end of '#pragma acc loop' are ignored}}
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop gang gang // expected-error {{directive '#pragma acc loop' cannot contain more than one 'gang' clause}}
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop independent worker worker // expected-error {{directive '#pragma acc loop' cannot contain more than one 'worker' clause}}
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop vector auto vector // expected-error {{directive '#pragma acc loop' cannot contain more than one 'vector' clause}}
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop gang worker vector
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop independent gang worker vector
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop gang worker vector auto
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop seq gang // expected-error {{unexpected OpenACC 'gang' clause, 'seq' is specified already}}
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop worker seq // expected-error {{unexpected OpenACC 'seq' clause, 'worker' is specified already}}
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop vector seq vector // expected-error {{unexpected OpenACC 'seq' clause, 'vector' is specified already}}
                                       // expected-error@-1 {{directive '#pragma acc loop' cannot contain more than one 'vector' clause}}
    for (int i = 0; i < 5; ++i)
      ;

    // for loop
    #pragma acc loop
      ; // expected-error {{statement after '#pragma acc loop' must be a for loop}}

    // break is fine when loop is executed sequentially.
    #pragma acc loop seq
    for (int i = 0; i < 5; ++i)
      break;

    // break forces sequential execution in the case of auto
    #pragma acc loop auto
    for (int i = 0; i < 5; ++i)
      break;

    // break is not permitted for implicit independent
    #pragma acc loop
    for (int i = 0; i < 5; ++i)
      break; // expected-error {{'break' statement cannot be used in partitionable OpenACC for loop}}

    // break is not permitted for explicit independent
    #pragma acc loop independent
    for (int i = 0; i < 5; ++i)
      break; // expected-error {{'break' statement cannot be used in partitionable OpenACC for loop}}

    // break is permitted in nested loops that are not partitioned
    #pragma acc loop independent
    for (int i = 0; i < 5; ++i)
      for (int j = 0; j < 5; ++j)
        break;

    // break is permitted in nested switches
    #pragma acc loop independent
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

    // nesting of acc loops: 2 levels
    #pragma acc loop gang // expected-note 4 {{parent '#pragma acc loop' is here}}
    for (int i = 0; i < 5; ++i) {
      #pragma acc loop gang // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop worker
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop vector
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop gang worker // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop vector gang // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop worker vector
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop gang worker vector // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      for (int j = 0; j < 5; ++j)
        ;
    }
    #pragma acc loop worker // expected-note 6 {{parent '#pragma acc loop' is here}}
    for (int i = 0; i < 5; ++i) {
      #pragma acc loop gang // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop worker // expected-error {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop vector
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop worker gang // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop gang vector // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop vector worker // expected-error {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop gang vector worker // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      for (int j = 0; j < 5; ++j)
        ;
    }
    #pragma acc loop vector // expected-note 7 {{parent '#pragma acc loop' is here}}
    for (int i = 0; i < 5; ++i) {
      #pragma acc loop gang // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop worker // expected-error {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop vector // expected-error {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop gang worker // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop vector gang // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop worker vector // expected-error {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop worker gang vector // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      for (int j = 0; j < 5; ++j)
        ;
    }
    #pragma acc loop gang worker // expected-note 6 {{parent '#pragma acc loop' is here}}
    for (int i = 0; i < 5; ++i) {
      #pragma acc loop gang // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop worker // expected-error {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop vector
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop worker gang // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop gang vector // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop vector worker // expected-error {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop worker vector gang // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      for (int j = 0; j < 5; ++j)
        ;
    }
    #pragma acc loop gang vector // expected-note 7 {{parent '#pragma acc loop' is here}}
    for (int i = 0; i < 5; ++i) {
      #pragma acc loop gang // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop worker // expected-error {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop vector // expected-error {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop gang worker // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop vector gang // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop worker vector // expected-error {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop vector gang worker // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      for (int j = 0; j < 5; ++j)
        ;
    }
    #pragma acc loop worker vector // expected-note 7 {{parent '#pragma acc loop' is here}}
    for (int i = 0; i < 5; ++i) {
      #pragma acc loop gang // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop worker // expected-error {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop vector // expected-error {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop worker gang // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop gang vector // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop vector worker // expected-error {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop vector worker gang // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      for (int j = 0; j < 5; ++j)
        ;
    }
    #pragma acc loop gang worker vector // expected-note 7 {{parent '#pragma acc loop' is here}}
    for (int i = 0; i < 5; ++i) {
      #pragma acc loop gang // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop worker // expected-error {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop vector // expected-error {{'#pragma acc loop' with 'vector' clause cannot be nested within '#pragma acc loop' with 'vector' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop gang worker // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop vector gang // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop worker vector // expected-error {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
      for (int j = 0; j < 5; ++j)
        ;
      #pragma acc loop gang worker vector // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'gang' clause}}
      for (int j = 0; j < 5; ++j)
        ;
    }

    // nesting of acc loops: 3 levels
    #pragma acc loop gang
    for (int i = 0; i < 5; ++i) {
      #pragma acc loop worker // expected-note 2 {{parent '#pragma acc loop' is here}}
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
      #pragma acc loop vector // expected-note 3 {{parent '#pragma acc loop' is here}}
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
    #pragma acc loop worker
    for (int i = 0; i < 5; ++i) {
      #pragma acc loop vector // expected-note 3 {{parent '#pragma acc loop' is here}}
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
    #pragma acc loop gang worker
    for (int i = 0; i < 5; ++i) {
      #pragma acc loop vector // expected-note 3 {{parent '#pragma acc loop' is here}}
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
    #pragma acc loop gang
    for (int i = 0; i < 5; ++i) {
      #pragma acc loop worker vector // expected-note 3 {{parent '#pragma acc loop' is here}}
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

    // nesting of acc loops: 4 levels
    #pragma acc loop gang
    for (int i = 0; i < 5; ++i) {
      #pragma acc loop worker
      for (int j = 0; j < 5; ++j) {
        #pragma acc loop vector // expected-note 3 {{parent '#pragma acc loop' is here}}
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

    // nesting of acc loops: other loops in between
    #pragma acc loop gang
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
    #pragma acc loop worker // expected-note 2 {{parent '#pragma acc loop' is here}}
    for (int i = 0; i < 5; ++i) {
      #pragma acc loop
      for (int i1 = 0; i1 < 5; ++i1) {
        #pragma acc loop independent
        for (int i2 = 0; i2 < 5; ++i2) {
          #pragma acc loop auto
          for (int i3 = 0; i3 < 5; ++i3) {
            #pragma acc loop seq
            for (int i4 = 0; i4 < 5; ++i4) {
              #pragma acc loop gang // expected-error {{'#pragma acc loop' with 'gang' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
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
              #pragma acc loop worker // expected-error {{'#pragma acc loop' with 'worker' clause cannot be nested within '#pragma acc loop' with 'worker' clause}}
                for (int j = 0; j < 5; ++j)
                  ;
            }
          }
        }
      }
    }

    // private and reduction clauses
    //
    // We sprinkle seq, auto, independent, gang, worker, and vector clauses
    // throughout these tests as the validation of the private and reduction
    // clauses should be independent of those clauses.  The only exception is
    // gang reductions, which interact with the parent parallel directive, so
    // we test that case more carefully at the end.

    #pragma acc loop private // expected-error {{expected '(' after 'private'}}
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop gang reduction // expected-error {{expected '(' after 'reduction'}}
    for (int i = 0; i < 5; ++i)
      ;

    // expected-error@+3 {{expected expression}}
    // expected-error@+2 {{expected ')'}}
    // expected-note@+1 {{to match this '('}}
    #pragma acc loop worker private(
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+4 {{expected reduction operator}}
    // expected-warning@+3 {{missing ':' after reduction operator - ignoring}}
    // expected-error@+2 {{expected ')'}}
    // expected-note@+1 {{to match this '('}}
    #pragma acc loop vector reduction(
    for (int i = 0; i < 5; ++i)
      ;

    // expected-error@+1 {{expected expression}}
    #pragma acc loop gang worker private()
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{expected reduction operator}}
    // expected-warning@+1 {{missing ':' after reduction operator - ignoring}}
    #pragma acc loop gang vector reduction( )
    for (int i = 0; i < 5; ++i)
      ;

    // expected-error@+1 {{expected reduction operator}}
    #pragma acc loop worker vector reduction( : )
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{expected reduction operator}}
    #pragma acc loop gang worker vector reduction(: i)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{expected reduction operator}}
    // expected-error@+1 {{use of undeclared identifier 'foo'}}
    #pragma acc loop reduction(:foo )
    for (int i = 0; i < 5; ++i)
      ;

    // expected-error@+2 {{expected reduction operator}}
    // expected-warning@+1 {{missing ':' after reduction operator - ignoring}}
    #pragma acc loop reduction(-) gang
    for (int i = 0; i < 5; ++i)
      ;
    // expected-warning@+2 {{missing ':' after reduction operator - ignoring}}
    // expected-error@+1 {{expected expression}}
    #pragma acc loop reduction(i) worker
    for (int i = 0; i < 5; ++i)
      ;
    // expected-warning@+2 {{missing ':' after reduction operator - ignoring}}
    // expected-error@+1 {{expected expression}}
    #pragma acc loop reduction(foo) vector
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{expected reduction operator}}
    #pragma acc loop reduction(-:) worker gang
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{expected expression}}
    #pragma acc loop reduction(foo:) vector gang
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{expected reduction operator}}
    #pragma acc loop reduction(-:i) vector worker
    for (int i = 0; i < 5; ++i)
      ;
    // OpenACC 2.6 sec. 2.5.12 line 774 mistypes "^" as "%", which is nonsense as a
    // reduction operator.
    // expected-error@+1 {{expected reduction operator}}
    #pragma acc loop reduction(% :i) gang vector worker
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{unknown reduction operator}}
    #pragma acc loop reduction(foo:i)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{use of undeclared identifier 'bar'}}
    #pragma acc loop gang reduction(foo:bar)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{unknown reduction operator}}
    #pragma acc loop worker reduction(foo : a[3])
    for (int i = 0; i < 5; ++i)
      ;

    // expected-warning@+2 {{missing ':' after reduction operator - ignoring}}
    // expected-error@+1 {{expected expression}}
    #pragma acc loop vector reduction(*)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-warning@+2 {{missing ':' after reduction operator - ignoring}}
    // expected-error@+1 {{expected expression}}
    #pragma acc loop gang reduction(max) worker
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{expected expression}}
    #pragma acc loop gang reduction(*:) vector
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{expected expression}}
    #pragma acc loop worker reduction(min:) vector
    for (int i = 0; i < 5; ++i)
      ;

    // expected-error@+1 {{expected ',' or ')' in 'private' clause}}
    #pragma acc loop worker private(jk i) gang vector
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{expected ',' or ')' in 'reduction' clause}}
    #pragma acc loop reduction(*:jk i)
    for (int i = 0; i < 5; ++i)
      ;

    // expected-error@+1 {{use of undeclared identifier 'foo'}}
    #pragma acc loop private(foo ) gang
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{use of undeclared identifier 'bar'}}
    #pragma acc loop reduction(*: bar ) worker
    for (int i = 0; i < 5; ++i)
      ;

    // expected-error@+1 {{expected expression}}
    #pragma acc loop private(jk ,) vector
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{expected expression}}
    #pragma acc loop worker reduction(+:i, ) gang
    for (int i = 0; i < 5; ++i)
      ;

    // expected-error@+2 {{expected variable name}}
    // expected-error@+1 {{expected variable name}}
    #pragma acc loop vector private(a[9], a[0:1]) gang
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{expected variable name}}
    // expected-error@+1 {{expected variable name}}
    #pragma acc loop vector reduction(*:a[3:5], a[10]) worker
    for (int i = 0; i < 5; ++i)
      ;

    // expected-error@+2 {{private variable cannot have incomplete type 'int []'}}
    // expected-error@+1 {{reduction variable cannot have incomplete type 'int []'}}
    #pragma acc loop worker private(incomplete) vector reduction(|:incomplete) gang
    for (int i = 0; i < 5; ++i)
      ;

    // expected-error@+4 {{const variable cannot be private because initialization is impossible}}
    // expected-error@+3 {{const variable cannot be private because initialization is impossible}}
    // expected-error@+2 {{reduction variable cannot be const}}
    // expected-error@+1 {{reduction variable cannot be const}}
    #pragma acc loop private(constI, constIDecl) reduction(+: constI, constIDecl)
    for (int i = 0; i < 5; ++i)
      ;
  }
  #pragma acc parallel
  {
    #pragma acc loop gang reduction(max:b,e,i,jk,f,d,p)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+6 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
    // expected-error@+5 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
    // expected-error@+4 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
    // expected-error@+3 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
    // expected-error@+2 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
    // expected-error@+1 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
    #pragma acc loop worker reduction(max:fc,dc,a,s,u,sDecl)
    for (int i = 0; i < 5; ++i)
      ;
  }
  #pragma acc parallel
  {
    #pragma acc loop vector reduction(min:b,e,i,jk,f,d,p)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+6 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
    // expected-error@+5 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
    // expected-error@+4 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
    // expected-error@+3 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
    // expected-error@+2 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
    // expected-error@+1 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
    #pragma acc loop worker gang reduction(min:fc,dc,a,s,u,sDecl)
    for (int i = 0; i < 5; ++i)
      ;
  }
  #pragma acc parallel
  {
    #pragma acc loop vector gang reduction(+:b,e,i,jk,f,d,fc,dc)
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+5 {{OpenACC reduction operator '+' argument must be of arithmetic type}}
    // expected-error@+4 {{OpenACC reduction operator '+' argument must be of arithmetic type}}
    // expected-error@+3 {{OpenACC reduction operator '+' argument must be of arithmetic type}}
    // expected-error@+2 {{OpenACC reduction operator '+' argument must be of arithmetic type}}
    // expected-error@+1 {{OpenACC reduction operator '+' argument must be of arithmetic type}}
    #pragma acc loop vector worker reduction(+:p,a,s,u,sDecl)
    for (int i = 0; i < 5; ++i)
      ;
  }
  #pragma acc parallel
  {
    #pragma acc loop vector gang reduction(*:b,e,i,jk,f,d,fc,dc) worker
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+5 {{OpenACC reduction operator '*' argument must be of arithmetic type}}
    // expected-error@+4 {{OpenACC reduction operator '*' argument must be of arithmetic type}}
    // expected-error@+3 {{OpenACC reduction operator '*' argument must be of arithmetic type}}
    // expected-error@+2 {{OpenACC reduction operator '*' argument must be of arithmetic type}}
    // expected-error@+1 {{OpenACC reduction operator '*' argument must be of arithmetic type}}
    #pragma acc loop reduction(*:p,a,s,u,sDecl)
    for (int i = 0; i < 5; ++i)
      ;
  }
  #pragma acc parallel
  {
    #pragma acc loop reduction(&&:b,e,i,jk,f,d,fc,dc) gang
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+5 {{OpenACC reduction operator '&&' argument must be of arithmetic type}}
    // expected-error@+4 {{OpenACC reduction operator '&&' argument must be of arithmetic type}}
    // expected-error@+3 {{OpenACC reduction operator '&&' argument must be of arithmetic type}}
    // expected-error@+2 {{OpenACC reduction operator '&&' argument must be of arithmetic type}}
    // expected-error@+1 {{OpenACC reduction operator '&&' argument must be of arithmetic type}}
    #pragma acc loop reduction(&&:p,a,s,u,sDecl) worker
    for (int i = 0; i < 5; ++i)
      ;
  }
  #pragma acc parallel
  {
    #pragma acc loop reduction(||:b,e,i,jk,f,d,fc,dc) vector
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+5 {{OpenACC reduction operator '||' argument must be of arithmetic type}}
    // expected-error@+4 {{OpenACC reduction operator '||' argument must be of arithmetic type}}
    // expected-error@+3 {{OpenACC reduction operator '||' argument must be of arithmetic type}}
    // expected-error@+2 {{OpenACC reduction operator '||' argument must be of arithmetic type}}
    // expected-error@+1 {{OpenACC reduction operator '||' argument must be of arithmetic type}}
    #pragma acc loop gang reduction(||:p,a,s,u,sDecl) worker
    for (int i = 0; i < 5; ++i)
      ;
  }
  #pragma acc parallel
  {
    #pragma acc loop gang reduction(&:b,e,i,jk) vector
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
    #pragma acc loop vector worker reduction(&:f,d,fc,dc,p,a,s,u,sDecl) gang
    for (int i = 0; i < 5; ++i)
      ;
  }
  #pragma acc parallel
  {
    #pragma acc loop reduction(|:b,e,i,jk)
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
    #pragma acc loop gang reduction(|:f,d,fc,dc,p,a,s,u,sDecl)
    for (int i = 0; i < 5; ++i)
      ;
  }
  #pragma acc parallel
  {
    #pragma acc loop worker reduction(^:b,e,i,jk)
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
    #pragma acc loop vector reduction(^:f,d,fc,dc,p,a,s,u,sDecl)
    for (int i = 0; i < 5; ++i)
      ;
  }
  #pragma acc parallel
  {
    // expected-error@+6 {{redundant 'max' reduction for variable 'i'}}
    // expected-note@+5 {{previous 'max' reduction here}}
    // expected-error@+4 {{redundant 'max' reduction for variable 'jk'}}
    // expected-note@+3 {{previous 'max' reduction here}}
    // expected-error@+2 {{conflicting '*' reduction for variable 'd'}}
    // expected-note@+1 {{previous 'max' reduction here}}
    #pragma acc loop gang reduction(max:i,i,jk,d) worker reduction(max:jk) reduction(*:d)
    for (int i = 0; i < 5; ++i)
      ;
  }
  #pragma acc parallel reduction(+:jk)
  {
    #pragma acc loop gang reduction(+:jk)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+2 {{redundant '+' reduction for variable 'jk'}}
    // expected-note@+1 {{previous '+' reduction here}}
    #pragma acc loop gang reduction(+:jk) reduction(+:jk)
    for (int j = 0; j < 5; ++j)
      ;
  }
  // expected-note@+2 2 {{while applying gang reduction to 'parallel' directive here}}
  // expected-note@+1 2 {{previous '+' reduction here}}
  #pragma acc parallel reduction(+:f)
  {
    // expected-error@+1 {{conflicting '*' reduction for variable 'f'}}
    #pragma acc loop gang reduction(*:f)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+3 {{redundant '*' reduction for variable 'f'}}
    // expected-note@+2 {{previous '*' reduction here}}
    // expected-error@+1 {{conflicting '*' reduction for variable 'f'}}
    #pragma acc loop gang worker reduction(*:f,f)
    for (int j = 0; j < 5; ++j)
      ;
  }
  #pragma acc parallel
  {
    #pragma acc loop gang vector reduction(max:d)
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+2 {{redundant 'max' reduction for variable 'd'}}
    // expected-note@+1 {{previous 'max' reduction here}}
    #pragma acc loop gang worker vector reduction(max:d,d)
    for (int j = 0; j < 5; ++j)
      ;
  }
  // expected-note@+1 {{while applying gang reduction to 'parallel' directive here}}
  #pragma acc parallel
  {
    // expected-note@+1 {{previous 'max' reduction here}}
    #pragma acc loop reduction(max:p) gang
    for (int j = 0; j < 5; ++j)
      ;
    // expected-error@+1 {{conflicting 'min' reduction for variable 'p'}}
    #pragma acc loop worker reduction(min:p) gang
    for (int j = 0; j < 5; ++j)
      ;
  }
  // expected-note@+1 {{while applying gang reduction to 'parallel' directive here}}
  #pragma acc parallel
  {
    #pragma acc loop
    for (int i0 = 0; i0 < 5; ++i0) {
      #pragma acc loop seq
      for (int i1 = 0; i1 < 5; ++i1) {
        #pragma acc loop independent
        for (int i2 = 0; i2 < 5; ++i2) {
          #pragma acc loop auto
          for (int i3 = 0; i3 < 5; ++i3) {
            // expected-note@+1 {{previous '&' reduction here}}
            #pragma acc loop vector reduction(&:b) gang
            for (int j = 0; j < 5; ++j)
              ;
          }
        }
      }
    }
    #pragma acc loop seq
    for (int i = 0; i < 5; ++i) {
      // expected-error@+1 {{conflicting '|' reduction for variable 'b'}}
      #pragma acc loop worker reduction(|:b) vector gang
      for (int j = 0; j < 5; ++j)
        ;
    }
  }
  // expected-note@+2 2 {{while applying gang reduction to 'parallel' directive here}}
  // expected-note@+1 {{previous '^' reduction here}}
  #pragma acc parallel reduction(^:e)
  {
    #pragma acc loop
    for (int i0 = 0; i0 < 5; ++i0) {
      enum E e;
      #pragma acc loop seq
      for (int i1 = 0; i1 < 5; ++i1) {
        #pragma acc loop independent
        for (int i2 = 0; i2 < 5; ++i2) {
          #pragma acc loop auto
          for (int i3 = 0; i3 < 5; ++i3) {
            double d, localOnly;
            #pragma acc loop gang reduction(&&:e,d,localOnly)
            for (int j = 0; j < 5; ++j)
              ;
          }
        }
      }
    }
    #pragma acc loop seq
    for (int i = 0; i < 5; ++i) {
      // expected-error@+2 {{conflicting '||' reduction for variable 'e'}}
      // expected-note@+1 {{previous '||' reduction here}}
      #pragma acc loop gang reduction(||:e,d)
      for (int j = 0; j < 5; ++j)
        ;
    }
    #pragma acc loop seq
    for (int i = 0; i < 5; ++i) {
      // expected-error@+1 {{conflicting '+' reduction for variable 'd'}}
      #pragma acc loop gang reduction(+:d)
      for (int j = 0; j < 5; ++j)
        ;
    }
  }
  #pragma acc parallel
  {
    // expected-error@+2 {{private variable cannot be reduction}}
    // expected-note@+1 {{defined as private}}
    #pragma acc loop private(i) gang reduction(&:i) vector
    for (int i = 0; i < 5; ++i)
      ;
    // expected-error@+2 {{reduction variable cannot be private}}
    // expected-note@+1 {{defined as reduction}}
    #pragma acc loop gang reduction(&&:i) worker private(i) vector
    for (int i = 0; i < 5; ++i)
      ;

    // expected-error@+1 {{OpenACC loop control variable cannot have reduction}}
    #pragma acc loop reduction(^:i)
    for (i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{OpenACC loop control variable cannot have reduction}}
    #pragma acc loop reduction(^:jk, i) gang
    for (i = 0; i < 5; ++i)
      ;
    // expected-error@+1 {{OpenACC loop control variable cannot have reduction}}
    #pragma acc loop reduction(^:jk) reduction(+:i) worker
    for (i = 0; i < 5; ++i)
      ;
  }
  #pragma acc loop // expected-error {{OpenACC loop outside of OpenACC parallel is not yet supported}}
  for (int i = 0; i < 5; ++i)
    ;
  ;
}

// The remaining diagnostics are currently produced by OpenMP sema during the
// transform from OpenACC to OpenMP, which is skipped if there have been any
// previous diagnostics.  Thus, each must be tested in a separate compilation.
//
// We don't check every OpenMP diagnostic case as the OpenMP tests should do
// that.  We just check some of them (roughly one of each diagnostic kind) to
// give us some confidence that they're handled properly by the OpenACC
// implementation.

#elif ERR == ERR_OMP_INIT

void fn(int k) {
  #pragma acc parallel
  {
    int j;
    #pragma acc loop independent gang
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop independent gang
    for (j = 0; j < 5; ++j)
      ;
    #pragma acc loop independent gang
    for (k = 0; k < 5; ++k)
      ;
    #pragma acc loop independent gang
    for (int i; i < 5; ++i) // expected-error {{initialization clause of OpenMP for loop is not in canonical form ('var = init' or 'T var = init')}}
      ;
  }
}

#elif ERR == ERR_OMP_COND

void fn(int j) {
  #pragma acc parallel
  {
    #pragma acc loop independent gang
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop independent gang
    for (int i = 0; 5 > i; ++i)
      ;
    #pragma acc loop independent gang
    for (int i = 0; i <= 5; ++i)
      ;
    #pragma acc loop independent gang
    for (int i = 0; 5 >= i; ++i)
      ;
    #pragma acc loop independent gang
    for (int i = 5; i > 0; --i)
      ;
    #pragma acc loop independent gang
    for (int i = 5; 0 < i; --i)
      ;
    #pragma acc loop independent gang
    for (int i = 5; i >= 0; --i)
      ;
    #pragma acc loop independent gang
    for (int i = 5; 0 <= i; --i)
      ;
    #pragma acc loop independent gang
    for (int i = 5; i && j; --i) // expected-error {{condition of OpenMP for loop must be a relational comparison ('<', '<=', '>', or '>=') of loop variable 'i'}}
      ;
  }
}

#elif ERR == ERR_OMP_INC

void fn(int j) {
  #pragma acc parallel
  {
    #pragma acc loop independent gang
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop independent gang
    for (int i = 0; i < 5; i++)
      ;
    #pragma acc loop independent gang
    for (int i = 0; i < 5; i += 2)
      ;
    #pragma acc loop independent gang
    for (int i = 0; i < 5; i = i + 3)
      ;
    #pragma acc loop independent gang
    for (int i = 5; i > 0; --i)
      ;
    #pragma acc loop independent gang
    for (int i = 5; i > 0; i--)
      ;
    #pragma acc loop independent gang
    for (int i = 5; i > 0; i -= 4)
      ;
    #pragma acc loop independent gang
    for (int i = 5; i > 0; i = i - 5)
      ;
    #pragma acc loop independent gang
    for (int i = 0; i < 5; i *= 2) // expected-error {{increment clause of OpenMP for loop must perform simple addition or subtraction on loop variable 'i'}}
      ;
  }
}

#elif ERR == ERR_OMP_INC0

void fn(int j) {
  #pragma acc parallel
  {
    #pragma acc loop independent gang
    for (int i = 0; i < 5; i += 0) // expected-error {{increment expression must cause 'i' to increase on each iteration of OpenMP for loop}}
                                   // expected-note@-1 {{loop step is expected to be positive due to this condition}}
      ;
  }
}

#elif ERR == ERR_OMP_VAR

void fn() {
  #pragma acc parallel
  {
    int a[5];
    #pragma acc loop independent gang
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop independent gang
    for (int *p = a; p < a+5; ++p)
      ;
# if HAS_UINT128
    #pragma acc loop independent gang
    for (__uint128_t i = 0; i < 5; ++i) // expected-warning {{OpenMP loop iteration variable cannot have more than 64 bits size and will be narrowed}}
      ;
# endif
    #pragma acc loop independent gang
    for (float f = 0; f < 5; ++f) // expected-error {{variable must be of integer or pointer type}}
      ;
  }
}

#else
# error undefined ERR
#endif

// Complete to suppress an additional warning, but it's too late for pragmas.
int incomplete[3];
