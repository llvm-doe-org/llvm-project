// Check "acc wait".
//
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
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %if host %{%} %else %{ %{acc-check-exe} %}
//
// END.

/* expected-no-diagnostics */

// FIXME: Currently needed for acc2omp_async2dep in OpenMP translation.
#include <openacc.h>
#include <stdio.h>

// We call this to create a boundary for FileCheck directives.
static inline void fcBoundary() {}

#define N 4

int main() {
  int x[N];
  int y[N];
  int z[N];

  //----------------------------------------------------------------------------
  // Check within function.
  //----------------------------------------------------------------------------

  // DMP-LABEL:"within function\n"
  // PRT-LABEL:{{.*}}"within function\n"{{.*}}
  // EXE-LABEL:within function
  printf("within function\n");
  for (int i = 0; i < N; ++i)
    x[i] = y[i] = z[i] = 0;

  // Nothing yet to wait on: shouldn't hang.
  //
  //      DMP: ACCWaitDirective
  // DMP-NEXT:   impl: OMPTaskwaitDirective
  // DMP-NEXT: CallExpr
  //
  //         PRT: fcBoundary();
  //  PRT-A-NEXT: #pragma acc wait
  // PRT-AO-NEXT: // #pragma omp taskwait
  //  PRT-O-NEXT: #pragma omp taskwait
  // PRT-OA-NEXT: // #pragma acc wait
  //    PRT-NEXT: fcBoundary();
  fcBoundary();
  #pragma acc wait
  fcBoundary();

  #pragma acc parallel loop async(0)
  for (int i = 0; i < N; ++i)
    ++x[i];
  #pragma acc parallel loop async(1)
  for (int i = 0; i < N; ++i)
    ++y[i];
  #pragma acc parallel loop async(2)
  for (int i = 0; i < N; ++i)
    ++z[i];

  // Plenty to wait on.
  #pragma acc wait

  // EXE-NEXT:x[0] = 1, y[0] = 1, z[0] = 1
  // EXE-NEXT:x[1] = 1, y[1] = 1, z[1] = 1
  // EXE-NEXT:x[2] = 1, y[2] = 1, z[2] = 1
  // EXE-NEXT:x[3] = 1, y[3] = 1, z[3] = 1
  for (int i = 0; i < N; ++i)
    printf("x[%d] = %d, y[%d] = %d, z[%d] = %d\n", i, x[i], i, y[i], i, z[i]);

  //----------------------------------------------------------------------------
  // Check within data construct.
  //----------------------------------------------------------------------------

  // DMP-LABEL:"within data construct\n"
  // PRT-LABEL:{{.*}}"within data construct\n"{{.*}}
  // EXE-LABEL:within data construct
  printf("within data construct\n");
  for (int i = 0; i < N; ++i)
    x[i] = y[i] = z[i] = 0;

  #pragma acc data copy(x, y, z)
  {
    // Nothing yet to wait on: shouldn't hang.
    //
    //      DMP: ACCWaitDirective
    // DMP-NEXT:   impl: OMPTaskwaitDirective
    // DMP-NEXT: CallExpr
    //
    //         PRT: fcBoundary();
    //  PRT-A-NEXT: #pragma acc wait
    // PRT-AO-NEXT: // #pragma omp taskwait
    //  PRT-O-NEXT: #pragma omp taskwait
    // PRT-OA-NEXT: // #pragma acc wait
    //    PRT-NEXT: fcBoundary();
    fcBoundary();
    #pragma acc wait
    fcBoundary();

    #pragma acc parallel loop async(0)
    for (int i = 0; i < N; ++i)
      ++x[i];
    #pragma acc parallel loop async(1)
    for (int i = 0; i < N; ++i)
      ++y[i];
    #pragma acc parallel loop async(2)
    for (int i = 0; i < N; ++i)
      ++z[i];

    // Plenty to wait on.
    #pragma acc wait
  }

  // EXE-NEXT:x[0] = 1, y[0] = 1, z[0] = 1
  // EXE-NEXT:x[1] = 1, y[1] = 1, z[1] = 1
  // EXE-NEXT:x[2] = 1, y[2] = 1, z[2] = 1
  // EXE-NEXT:x[3] = 1, y[3] = 1, z[3] = 1
  for (int i = 0; i < N; ++i)
    printf("x[%d] = %d, y[%d] = %d, z[%d] = %d\n", i, x[i], i, y[i], i, z[i]);

  return 0;
}
