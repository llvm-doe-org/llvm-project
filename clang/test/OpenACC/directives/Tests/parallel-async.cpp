// Check async clause on "acc parallel", but only check cases that are not
// covered by parallel-async.c because they are specific to C++.
//
// REDEFINE: %{all:fc:args} = -DACC_ASYNC_SYNC=-1 -DACC_ASYNC_NOVAL=-2
// REDEFINE: %{dmp:fc:args} = -strict-whitespace
// REDEFINE: %{prt:fc:args} = -match-full-lines
// REDEFINE: %{exe:fc:args} = -strict-whitespace -match-full-lines \
// REDEFINE:                  -implicit-check-not='{{.}}'
//
// FIXME: Skip execution checks when not offloading because we occasionally see
// the following runtime error in that case:
//
//   Assertion failure at kmp_tasking.cpp(4182): task_team != __null.
//   OMP: Error #13: Assertion failure at kmp_tasking.cpp(4182).
//
// RUN: %{acc-check-dmp-cxx}
// RUN: %{acc-check-prt-cxx}
// RUN: %{acc-check-exe-cxx}
//
// END.

/* expected-no-diagnostics */

// We don't include openacc.h because we don't want Clang parsing/sema to pick
// up its acc2omp_async2dep, potentially hiding any acc2omp_async2dep lookup
// problems above.
#include <stdio.h>

// We call this to create a boundary for FileCheck directives.
static inline void fcBoundary() {}

//------------------------------------------------------------------------------
// Check acc2omp_async2dep lookup vs. template instantiation.
//
// Clang currently requires acc2omp_async2dep to be visible at any use of the
// async clause.  Clang must perform the acc2omp_async2dep lookup while
// originally parsing f below and must not try to repeat the lookup during the
// later f instantiation, where acc2omp_async2dep is out of scope.  That is,
// Clang must perform the acc2omp_async2dep lookup in the parser not in sema
// actions, which are called during both parsing and template instantiation.
//------------------------------------------------------------------------------

namespace ScopedAsync2dep {
  char *acc2omp_async2dep(int async_arg) {
    // The printf proves that it's actually this function instead of
    // libacc2omp's version that's being called.  It is not really our goal to
    // support alternate versions of acc2omp_async2dep, but Clang does not try
    // to prevent it.  We use an alternate version here as a means to check
    // that scoping works correctly during template instantiation.
    printf("local acc2omp_async2dep called\n");
    static char q;
    return &q;
  }
  template <typename T>
  void f() {
    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCAsyncClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     IntegerLiteral {{.*}} 0
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNowaitClause
    // DMP-NEXT:     OMPDependClause
    // DMP-NEXT:       UnaryOperator {{.*}} 'char' lvalue prefix '*'
    // DMP-NEXT:         CallExpr {{.*}} 'char *'
    // DMP-NEXT:           ImplicitCastExpr {{.*}} 'char *(*)(int)' <FunctionToPointerDecay>
    // DMP-NEXT:             DeclRefExpr {{.*}} 'acc2omp_async2dep' 'char *(int)'
    // DMP-NEXT:           IntegerLiteral {{.*}} 0
    //  DMP-NOT:     OMP
    //      DMP:     NullStmt
    //
    //         PRT: fcBoundary();
    //  PRT-A-NEXT: #pragma acc parallel async(0)
    // PRT-AO-NEXT: // #pragma omp target teams nowait depend(inout : *acc2omp_async2dep(0))
    //  PRT-O-NEXT: #pragma omp target teams nowait depend(inout : *acc2omp_async2dep(0))
    // PRT-OA-NEXT: // #pragma acc parallel async(0)
    //    PRT-NEXT: ;
    //    PRT-NEXT: fcBoundary();
    fcBoundary();
    #pragma acc parallel async(0)
    ;
    fcBoundary();
    #pragma acc wait
  }
}

// Another attempt to distract Clang from the right acc2omp_async2dep during
// instantiation of f.
int acc2omp_async2dep;

int main() {
  // EXE:start
  printf("start\n");
  // EXE-NEXT:local acc2omp_async2dep called
  ScopedAsync2dep::f<int>();
  return 0;
}
