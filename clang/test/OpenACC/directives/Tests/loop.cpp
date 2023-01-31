// Check simple "acc loop" cases that are specific to C++ but don't fit the
// focus of any other loop-*.cpp test.

// REDEFINE: %{all:clang:args} = -Wno-openacc-routine-cxx-lambda
// REDEFINE: %{dmp:fc:args} = -strict-whitespace
// REDEFINE: %{exe:fc:args} = -strict-whitespace -match-full-lines \
// REDEFINE:                  -implicit-check-not='{{.}}'
// RUN: %{acc-check-dmp-cxx}
// RUN: %{acc-check-prt-cxx}
// RUN: %{acc-check-exe-cxx}

// END.

/* expected-no-diagnostics */

#include <stdio.h>

// EXE-NOT:{{.}}

//------------------------------------------------------------------------------
// Check that a lambda can be defined and assigned to a variable within a loop
// construct.
//
// The transformation to OpenMP used to produce a spurious type conversion error
// diagnostic because the variable's type wasn't transformed to the transformed
// lambda's type.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} lambdaAssignInAccLoop
//
//   PRT-LABEL: void lambdaAssignInAccLoop() {
//    PRT-NEXT:   printf
//    PRT-NEXT:   int x =
//  PRT-A-NEXT:   {{^ *}}#pragma acc parallel num_gangs(1) num_workers(1) vector_length(1) copy(x){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: x){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp target teams num_teams(1) map(ompx_hold,tofrom: x){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc parallel num_gangs(1) num_workers(1) vector_length(1) copy(x){{$}}
//  PRT-A-NEXT:   {{^ *}}#pragma acc loop gang worker vector{{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp distribute parallel for simd num_threads(1) simdlen(1){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp distribute parallel for simd num_threads(1) simdlen(1){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc loop gang worker vector{{$}}
//    PRT-NEXT:   for (int i = 0; i < 8; ++i) {
//    PRT-NEXT:     auto lambda = [&x]() {
//    PRT-NEXT:       x *=
//    PRT-NEXT:     };
//    PRT-NEXT:     lambda();
//    PRT-NEXT:   }
//    PRT-NEXT:   printf
//    PRT-NEXT: }
//
// EXE-LABEL:lambdaAssignInAccLoop
//  EXE-NEXT:x=256
//   EXE-NOT:{{.}}
void lambdaAssignInAccLoop() {
  printf("lambdaAssignInAccLoop\n");
  int x = 1;
  #pragma acc parallel num_gangs(1) num_workers(1) vector_length(1) copy(x)
  #pragma acc loop gang worker vector
  for (int i = 0; i < 8; ++i) {
    auto lambda = [&x]() {
      x *= 2;
    };
    lambda();
  }
  printf("x=%d\n", x);
}

// DMP-LABEL: FunctionDecl {{.*}} lambdaAssignInAccLoopCombined
//
//   PRT-LABEL: void lambdaAssignInAccLoopCombined() {
//    PRT-NEXT:   printf
//    PRT-NEXT:   int x =
//  PRT-A-NEXT:   {{^ *}}#pragma acc parallel loop num_gangs(1) num_workers(1) vector_length(1) copy(x) gang worker vector{{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: x){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp distribute parallel for simd num_threads(1) simdlen(1){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp target teams num_teams(1) map(ompx_hold,tofrom: x){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp distribute parallel for simd num_threads(1) simdlen(1){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc parallel loop num_gangs(1) num_workers(1) vector_length(1) copy(x) gang worker vector{{$}}
//    PRT-NEXT:   for (int i = 0; i < 8; ++i) {
//    PRT-NEXT:     auto lambda = [&x]() {
//    PRT-NEXT:       x *=
//    PRT-NEXT:     };
//    PRT-NEXT:     lambda();
//    PRT-NEXT:   }
//    PRT-NEXT:   printf
//    PRT-NEXT: }
//
// EXE-LABEL:lambdaAssignInAccLoopCombined
//  EXE-NEXT:x=256
//   EXE-NOT:{{.}}
void lambdaAssignInAccLoopCombined() {
  printf("lambdaAssignInAccLoopCombined\n");
  int x = 1;
  #pragma acc parallel loop num_gangs(1) num_workers(1) vector_length(1) copy(x) gang worker vector
  for (int i = 0; i < 8; ++i) {
    auto lambda = [&x]() {
      x *= 2;
    };
    lambda();
  }
  printf("x=%d\n", x);
}

//------------------------------------------------------------------------------
// Check that a lambda can contain an orphaned loop construct.
//
// This works only because we set -Wno-openacc-routine-cxx-lambda above.  In the
// future, we expect to compute an implicit routine directive for the lambda
// based on its orphaned loop construct's level of parallelism.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} orphanedLoopInLambda_defineAndCallLambda 'void (int *)'
//  DMP-NEXT: ParmVarDecl {{.* a}}
//  DMP-NEXT: CompoundStmt
//       DMP:   LambdaExpr
//  DMP-NEXT:     CXXRecordDecl
//       DMP:       CXXMethodDecl
//  DMP-NEXT:         CompoundStmt
//       DMP:           ACCLoopDirective
//  DMP-NEXT:             ACCGangClause
//  DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:               DeclRefExpr {{.*}} 'a'
//  DMP-NEXT:             impl: OMPDistributeDirective
//   DMP-NOT:               OMP
//       DMP:               ForStmt
//
//   PRT-LABEL: void orphanedLoopInLambda_defineAndCallLambda(int *a) {
//    PRT-NEXT:   [&]() {
//  PRT-A-NEXT:     {{^ *}}#pragma acc loop gang{{$}}
// PRT-AO-NEXT:     {{^ *}}// #pragma omp distribute{{$}}
//  PRT-O-NEXT:     {{^ *}}#pragma omp distribute{{$}}
// PRT-OA-NEXT:     {{^ *}}// #pragma acc loop gang{{$}}
//    PRT-NEXT:     for (int i = {{.*}})
//    PRT-NEXT:       a[i] +=
//    PRT-NEXT:   }();
//    PRT-NEXT: }
//
// EXE-LABEL:orphanedLoopInLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
//   EXE-NOT:{{.}}
#pragma acc routine gang
void orphanedLoopInLambda_defineAndCallLambda(int *a) {
  [&]() {
    #pragma acc loop gang
    for (int i = 0; i < 4; ++i)
      a[i] += 10;
  }();
}
void orphanedLoopInLambda() {
  printf("orphanedLoopInLambda\n");
  int a[4];
  for (int i = 0; i < 4; ++i)
    a[i] = i;
  #pragma acc parallel num_gangs(4)
  orphanedLoopInLambda_defineAndCallLambda(a);
  for (int i = 0; i < 4; ++i)
    printf("a[%d]=%d\n", i, a[i]);
}

// DMP-LABEL: FunctionDecl {{.*}} orphanedLoopInLambdaInParallel 'void ()'
//       DMP: LambdaExpr
//  DMP-NEXT:   CXXRecordDecl
//       DMP:     CXXMethodDecl
//  DMP-NEXT:       CompoundStmt
//       DMP:         ACCLoopDirective
//  DMP-NEXT:           ACCGangClause
//  DMP-NEXT:           ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:           ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:             DeclRefExpr {{.*}} 'a'
//  DMP-NEXT:           impl: OMPDistributeDirective
//   DMP-NOT:             OMP
//       DMP:             ForStmt
//
//   PRT-LABEL: void orphanedLoopInLambdaInParallel() {
//    PRT-NEXT:   printf
//    PRT-NEXT:   int a
//    PRT-NEXT:   for (int i = {{.*}})
//    PRT-NEXT:     a[i] =
//  PRT-A-NEXT:   {{^ *}}#pragma acc parallel num_gangs(4){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp target teams num_teams(4) map(ompx_hold,tofrom: a){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp target teams num_teams(4) map(ompx_hold,tofrom: a){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc parallel num_gangs(4){{$}}
//    PRT-NEXT:   {
//    PRT-NEXT:     [&]() {
//  PRT-A-NEXT:       {{^ *}}#pragma acc loop gang{{$}}
// PRT-AO-NEXT:       {{^ *}}// #pragma omp distribute{{$}}
//  PRT-O-NEXT:       {{^ *}}#pragma omp distribute{{$}}
// PRT-OA-NEXT:       {{^ *}}// #pragma acc loop gang{{$}}
//    PRT-NEXT:       for (int i = {{.*}})
//    PRT-NEXT:         a[i] +=
//    PRT-NEXT:     }();
//    PRT-NEXT:   }
//    PRT-NEXT:   for (int i = {{.*}})
//    PRT-NEXT:     printf
//    PRT-NEXT: }
//
// EXE-LABEL:orphanedLoopInLambdaInParallel
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
//   EXE-NOT:{{.}}
void orphanedLoopInLambdaInParallel() {
  printf("orphanedLoopInLambdaInParallel\n");
  int a[4];
  for (int i = 0; i < 4; ++i)
    a[i] = i;
  #pragma acc parallel num_gangs(4)
  {
    [&]() {
      #pragma acc loop gang
      for (int i = 0; i < 4; ++i)
        a[i] += 10;
    }();
  }
  for (int i = 0; i < 4; ++i)
    printf("a[%d]=%d\n", i, a[i]);
}

int main() {
  lambdaAssignInAccLoop();
  lambdaAssignInAccLoopCombined();
  orphanedLoopInLambda();
  orphanedLoopInLambdaInParallel();
  return 0;
}
