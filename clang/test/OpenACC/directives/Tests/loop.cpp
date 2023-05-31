// Check simple "acc loop" cases that are specific to C++ but don't fit the
// focus of any other loop-*.cpp test.

// REDEFINE: %{dmp:fc:args} = -strict-whitespace
// REDEFINE: %{exe:fc:args} = -strict-whitespace -match-full-lines \
// REDEFINE:                  -implicit-check-not='{{.}}'
// RUN: %{acc-check-dmp-cxx}
// RUN: %{acc-check-prt-cxx}
// RUN: %{acc-check-exe-cxx}

// END.

/* expected-no-diagnostics */

#include <stdio.h>

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
// PRT-AO-NEXT:   {{^ *}}// #pragma omp target teams num_teams(1) thread_limit(1) map(ompx_hold,tofrom: x){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp target teams num_teams(1) thread_limit(1) map(ompx_hold,tofrom: x){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc parallel num_gangs(1) num_workers(1) vector_length(1) copy(x){{$}}
//  PRT-A-NEXT:   {{^ *}}#pragma acc loop gang worker vector{{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp distribute parallel for simd simdlen(1){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp distribute parallel for simd simdlen(1){{$}}
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
// PRT-AO-NEXT:   {{^ *}}// #pragma omp target teams num_teams(1) thread_limit(1) map(ompx_hold,tofrom: x){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp distribute parallel for simd simdlen(1){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp target teams num_teams(1) thread_limit(1) map(ompx_hold,tofrom: x){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp distribute parallel for simd simdlen(1){{$}}
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

int main() {
  lambdaAssignInAccLoop();
  lambdaAssignInAccLoopCombined();
  return 0;
}
