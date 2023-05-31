// Check the effect of various combinations of
// -f[no-]openacc-implicit-{worker,vector}.
//
// Placement of implicit gang, worker, and vector clauses are thoroughly checked
// in loop-implicit-gang-worker-vector.c and routine-implicit.cpp.
//
// REDEFINE: %{dmp:fc:args} = -strict-whitespace
// REDEFINE: %{exe:fc:args} = -strict-whitespace -match-full-lines \
// REDEFINE:                  -implicit-check-not='{{.}}'
//
//------------------------------------------------------------------------------
// gang only.
// REDEFINE: %{all:fc:pres} = G
//
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}
//
// REDEFINE: %{all:clang:args} =                                      \
// REDEFINE:   -fopenacc-implicit-worker -fno-openacc-implicit-worker \
// REDEFINE:   -fopenacc-implicit-vector -fno-openacc-implicit-vector
// RUN: %{acc-check-dmp}
//------------------------------------------------------------------------------
// gang and worker.
// REDEFINE: %{all:fc:pres} = GW
//
// REDEFINE: %{all:clang:args} = -fopenacc-implicit-worker
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}
//
// REDEFINE: %{all:clang:args} =                                      \
// REDEFINE:   -fno-openacc-implicit-worker -fopenacc-implicit-worker \
// REDEFINE:   -fopenacc-implicit-vector -fno-openacc-implicit-vector
// RUN: %{acc-check-dmp}
//------------------------------------------------------------------------------
// gang and vector.
// REDEFINE: %{all:fc:pres} = GV
//
// REDEFINE: %{all:clang:args} = -fopenacc-implicit-vector
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}
//
// REDEFINE: %{all:clang:args} =                                      \
// REDEFINE:   -fopenacc-implicit-worker -fno-openacc-implicit-worker \
// REDEFINE:   -fno-openacc-implicit-vector -fopenacc-implicit-vector
// RUN: %{acc-check-dmp}
//------------------------------------------------------------------------------
// gang, worker, and vector.
// REDEFINE: %{all:fc:pres} = GWV
//
// REDEFINE: %{all:clang:args} = -fopenacc-implicit-worker \
// REDEFINE:                     -fopenacc-implicit-vector
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}
//
// REDEFINE: %{all:clang:args} =                                      \
// REDEFINE:   -fno-openacc-implicit-worker -fopenacc-implicit-worker \
// REDEFINE:   -fno-openacc-implicit-vector -fopenacc-implicit-vector
// RUN: %{acc-check-dmp}
//
// END.

/* expected-no-diagnostics */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  //          DMP: ACCParallelDirective
  //     DMP-NEXT:   ACCNumGangsClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:   ACCNumWorkersClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:   ACCVectorLengthClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
  //     DMP-NEXT:     DeclRefExpr {{.*}} 'count'
  //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:     DeclRefExpr {{.*}} 'count'
  //     DMP-NEXT:   impl: OMPTargetTeamsDirective
  //     DMP-NEXT:     OMPNum_teamsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     OMPThread_limitClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 4
  //     DMP-NEXT:     OMPMapClause
  //     DMP-NEXT:       DeclRefExpr {{.*}} 'count'
  //          DMP:   ACCLoopDirective
  //     DMP-NEXT:     ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:     ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:       DeclRefExpr {{.*}} 'count'
  //     DMP-NEXT:     ACCGangClause {{.*}} <implicit>
  //  DMP-GW-NEXT:     ACCWorkerClause {{.*}} <implicit>
  //  DMP-GV-NEXT:     ACCVectorClause {{.*}} <implicit>
  // DMP-GWV-NEXT:     ACCWorkerClause {{.*}} <implicit>
  // DMP-GWV-NEXT:     ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:     impl: OMPDistributeDirective
  //  DMP-GW-NEXT:     impl: OMPDistributeParallelForDirective
  //  DMP-GV-NEXT:     impl: OMPDistributeSimdDirective
  //  DMP-GV-NEXT:       OMPSimdlenClause
  //  DMP-GV-NEXT:         ConstantExpr
  //  DMP-GV-NEXT:           value: Int 8
  //  DMP-GV-NEXT:           IntegerLiteral {{.*}} 'int' 8
  // DMP-GWV-NEXT:     impl: OMPDistributeParallelForSimdDirective
  // DMP-GWV-NEXT:       OMPSimdlenClause
  // DMP-GWV-NEXT:         ConstantExpr
  // DMP-GWV-NEXT:           value: Int 8
  // DMP-GWV-NEXT:           IntegerLiteral {{.*}} 'int' 8
  //      DMP-NOT:       OMP
  //          DMP:       ForStmt
  //          DMP:         CallExpr
  //
  //             PRT: int count[8];
  //        PRT-NEXT: for ({{.*}})
  //        PRT-NEXT:   count[i] = 0;
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  //     PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) thread_limit(4) map(ompx_hold,tofrom: count){{$}}
  //      PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2) thread_limit(4) map(ompx_hold,tofrom: count){{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-AO-GW-NEXT: {{^ *}}// #pragma omp distribute parallel for{{$}}
  //  PRT-AO-GV-NEXT: {{^ *}}// #pragma omp distribute simd simdlen(8){{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd simdlen(8){{$}}
  //
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //   PRT-O-GW-NEXT: {{^ *}}#pragma omp distribute parallel for{{$}}
  //   PRT-O-GV-NEXT: {{^ *}}#pragma omp distribute simd simdlen(8){{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd simdlen(8){{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
  //
  //        PRT-NEXT: for ({{.*}})
  //        PRT-NEXT:   ++count[i];
  //
  //      EXE:count[0] = 1
  // EXE-NEXT:count[1] = 1
  // EXE-NEXT:count[2] = 1
  // EXE-NEXT:count[3] = 1
  // EXE-NEXT:count[4] = 1
  // EXE-NEXT:count[5] = 1
  // EXE-NEXT:count[6] = 1
  // EXE-NEXT:count[7] = 1
  int count[8];
  for (int i = 0; i < 8; ++i)
    count[i] = 0;
  #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8)
  #pragma acc loop
  for (int i = 0; i < 8; ++i)
    ++count[i];
  for (int i = 0; i < 8; ++i)
    printf("count[%d] = %d\n", i, count[i]);
  return 0;
}
