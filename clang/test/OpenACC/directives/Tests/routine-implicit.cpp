// Check implicit "acc routine" directives not covered by routine-implicit.c
// because they are specific to C++.
//
// Implicit routine directives for C++ class/namespace member functions are
// checked as part of routine-placement-for-class.cpp and
// routine-placement-for-namespace.cpp.  Implicit routine directives for C++
// lambdas are checked in routine-implicit-lambda.cpp.

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
// Check that a lambda can contain a call to a non-seq routine.
//
// This works only because we set -Wno-openacc-routine-cxx-lambda above.  In the
// future, we expect to compute an implicit routine directive for the lambda
// based on the level of parallelism of the routine it calls.  For now, there is
// no implicit routine directive here.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} nonSeqRoutineInLambda_defineAndCallLambda 'void (int *)'
//  DMP-NEXT: ParmVarDecl {{.* a}}
//  DMP-NEXT: CompoundStmt
//       DMP:   LambdaExpr
//  DMP-NEXT:     CXXRecordDecl
//       DMP:       CXXMethodDecl
//  DMP-NEXT:         CompoundStmt
//  DMP-NEXT:           CallExpr
//  DMP-NEXT:             ImplicitCastExpr
//  DMP-NEXT:               DeclRefExpr {{.*}} 'nonSeqRoutineInLambda_gangFn'
//
// PRT-LABEL: void nonSeqRoutineInLambda_defineAndCallLambda(int *a) {
//  PRT-NEXT:   [&]() {
//  PRT-NEXT:     nonSeqRoutineInLambda_gangFn(a);
//  PRT-NEXT:   }();
//  PRT-NEXT: }
//
// EXE-LABEL:nonSeqRoutineInLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
//   EXE-NOT:{{.}}
#pragma acc routine gang
void nonSeqRoutineInLambda_gangFn(int *a) {
  #pragma acc loop gang
  for (int i = 0; i < 4; ++i)
    a[i] += 10;
}

#pragma acc routine gang
void nonSeqRoutineInLambda_defineAndCallLambda(int *a) {
  [&]() {
    nonSeqRoutineInLambda_gangFn(a);
  }();
}
void nonSeqRoutineInLambda() {
  printf("nonSeqRoutineInLambda\n");
  int a[4];
  for (int i = 0; i < 4; ++i)
    a[i] = i;
  #pragma acc parallel num_gangs(4)
  nonSeqRoutineInLambda_defineAndCallLambda(a);
  for (int i = 0; i < 4; ++i)
    printf("a[%d]=%d\n", i, a[i]);
}

// DMP-LABEL: FunctionDecl {{.*}} nonSeqRoutineInLambdaInParallel 'void ()'
//       DMP: LambdaExpr
//  DMP-NEXT:   CXXRecordDecl
//       DMP:     CXXMethodDecl
//  DMP-NEXT:       CompoundStmt
//  DMP-NEXT:         CallExpr
//  DMP-NEXT:           ImplicitCastExpr
//  DMP-NEXT:             DeclRefExpr {{.*}} 'nonSeqRoutineInLambdaInParallel_gangFn'
//
//   PRT-LABEL: void nonSeqRoutineInLambdaInParallel() {
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
//    PRT-NEXT:       nonSeqRoutineInLambdaInParallel_gangFn(a);
//    PRT-NEXT:     }();
//    PRT-NEXT:   }
//    PRT-NEXT:   for (int i = {{.*}})
//    PRT-NEXT:     printf
//    PRT-NEXT: }
//
// EXE-LABEL:nonSeqRoutineInLambdaInParallel
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
//   EXE-NOT:{{.}}
#pragma acc routine gang
void nonSeqRoutineInLambdaInParallel_gangFn(int *a) {
  #pragma acc loop gang
  for (int i = 0; i < 4; ++i)
    a[i] += 10;
}
void nonSeqRoutineInLambdaInParallel() {
  printf("nonSeqRoutineInLambdaInParallel\n");
  int a[4];
  for (int i = 0; i < 4; ++i)
    a[i] = i;
  #pragma acc parallel num_gangs(4)
  {
    [&]() {
      nonSeqRoutineInLambdaInParallel_gangFn(a);
    }();
  }
  for (int i = 0; i < 4; ++i)
    printf("a[%d]=%d\n", i, a[i]);
}

int main() {
  nonSeqRoutineInLambda();
  nonSeqRoutineInLambdaInParallel();
  return 0;
}
