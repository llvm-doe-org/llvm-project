// Check "acc wait" in the trivial case that there's nothing to wait on.
// Basically, make sure it doesn't hang.
//
// REDEFINE: %{dmp:fc:args} = -strict-whitespace
// REDEFINE: %{prt:fc:args} = -match-full-lines
// REDEFINE: %{exe:fc:args} = -strict-whitespace -match-full-lines \
// REDEFINE:                  -implicit-check-not='{{.}}'
//
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}
//
// END.

/* expected-no-diagnostics */

#include <stdio.h>

// We call this to create a boundary for FileCheck directives.
static inline void fcBoundary() {}

#define N 4

int main() {
  int x[N];

  //----------------------------------------------------------------------------
  // Check within function.
  //----------------------------------------------------------------------------

  // DMP-LABEL:"within function\n"
  // PRT-LABEL:{{.*}}"within function\n"{{.*}}
  // EXE-LABEL:within function
  printf("within function\n");
  for (int i = 0; i < N; ++i)
    x[i] = 0;

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

  #pragma acc parallel loop
  for (int i = 0; i < 4; ++i)
    ++x[i];

  #pragma acc wait

  // EXE-NEXT:x[0] = 1
  // EXE-NEXT:x[1] = 1
  // EXE-NEXT:x[2] = 1
  // EXE-NEXT:x[3] = 1
  for (int i = 0; i < N; ++i)
    printf("x[%d] = %d\n", i, x[i]);

  //----------------------------------------------------------------------------
  // Check within data construct.
  //----------------------------------------------------------------------------

  // DMP-LABEL:"within data construct\n"
  // PRT-LABEL:{{.*}}"within data construct\n"{{.*}}
  // EXE-LABEL:within data construct
  printf("within data construct\n");
  for (int i = 0; i < N; ++i)
    x[i] = 0;

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
  #pragma acc data copy(x)
  {
    fcBoundary();
    #pragma acc wait
    fcBoundary();
    #pragma acc parallel loop
    for (int i = 0; i < 4; ++i)
      ++x[i];
    #pragma acc wait
  }

  // EXE-NEXT:x[0] = 1
  // EXE-NEXT:x[1] = 1
  // EXE-NEXT:x[2] = 1
  // EXE-NEXT:x[3] = 1
  for (int i = 0; i < N; ++i)
    printf("x[%d] = %d\n", i, x[i]);

  return 0;
}
