// Check the effect of various combinations of
// -f[no-]openacc-implicit-{worker,vector}[=KIND].
//
// Placement of implicit gang, worker, and vector clauses is thoroughly checked
// in loop-implicit-gang-worker-vector.c and routine-implicit.cpp.  The goal
// here is just to confirm that the correct algorithm is chosen by the command
// line options, so we check just enough of placement to confirm that.
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
// REDEFINE: %{all:clang:args} = \
// REDEFINE:   -fopenacc-implicit-worker=outer -fopenacc-implicit-worker=none \
// REDEFINE:   -fopenacc-implicit-vector=outer -fopenacc-implicit-vector=none
// RUN: %{acc-check-dmp}
//
// REDEFINE: %{all:clang:args} = \
// REDEFINE:   -fopenacc-implicit-worker -fopenacc-implicit-worker=none \
// REDEFINE:   -fopenacc-implicit-vector -fopenacc-implicit-vector=none
// RUN: %{acc-check-dmp}
//
// REDEFINE: %{all:clang:args} = \
// REDEFINE:   -fopenacc-implicit-worker=vector -fno-openacc-implicit-worker \
// REDEFINE:   -fopenacc-implicit-vector=outer -fno-openacc-implicit-vector
// RUN: %{acc-check-dmp}
//
// REDEFINE: %{all:clang:args} = \
// REDEFINE:   -fopenacc-implicit-worker -fno-openacc-implicit-worker \
// REDEFINE:   -fopenacc-implicit-vector -fno-openacc-implicit-vector
// RUN: %{acc-check-dmp}
//------------------------------------------------------------------------------
// gang and worker=vector.
// REDEFINE: %{all:fc:pres} = GWv
//
// REDEFINE: %{all:clang:args} = -fopenacc-implicit-worker=vector
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}
//
// REDEFINE: %{all:clang:args} = -fno-openacc-implicit-worker \
// REDEFINE:                     -fopenacc-implicit-worker=vector
// RUN: %{acc-check-dmp}
//
// REDEFINE: %{all:clang:args} = -fopenacc-implicit-worker=none \
// REDEFINE:                     -fopenacc-implicit-worker=vector
// RUN: %{acc-check-dmp}
//
// REDEFINE: %{all:clang:args} = -fopenacc-implicit-worker=outer \
// REDEFINE:                     -fopenacc-implicit-worker=vector
// RUN: %{acc-check-dmp}
//
// REDEFINE: %{all:clang:args} = -fopenacc-implicit-worker=vector-outer \
// REDEFINE:                     -fopenacc-implicit-worker=vector
// RUN: %{acc-check-dmp}
//------------------------------------------------------------------------------
// gang and worker=outer.
// REDEFINE: %{all:fc:pres} = GWo
//
// REDEFINE: %{all:clang:args} = -fopenacc-implicit-worker=outer
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}
//
// REDEFINE: %{all:clang:args} = -fno-openacc-implicit-worker \
// REDEFINE:                     -fopenacc-implicit-worker=outer
// RUN: %{acc-check-dmp}
//
// REDEFINE: %{all:clang:args} = -fopenacc-implicit-worker=none \
// REDEFINE:                     -fopenacc-implicit-worker=outer
// RUN: %{acc-check-dmp}
//
// REDEFINE: %{all:clang:args} = -fopenacc-implicit-worker=vector \
// REDEFINE:                     -fopenacc-implicit-worker=outer
// RUN: %{acc-check-dmp}
//
// REDEFINE: %{all:clang:args} = -fopenacc-implicit-worker=vector-outer \
// REDEFINE:                     -fopenacc-implicit-worker=outer
// RUN: %{acc-check-dmp}
//
// REDEFINE: %{all:clang:args} = -fno-openacc-implicit-worker \
// REDEFINE:                     -fopenacc-implicit-worker
// RUN: %{acc-check-dmp}
//
// REDEFINE: %{all:clang:args} = -fopenacc-implicit-worker=none \
// REDEFINE:                     -fopenacc-implicit-worker
// RUN: %{acc-check-dmp}
//
// REDEFINE: %{all:clang:args} = -fopenacc-implicit-worker=vector \
// REDEFINE:                     -fopenacc-implicit-worker
// RUN: %{acc-check-dmp}
//
// REDEFINE: %{all:clang:args} = -fopenacc-implicit-worker=vector-outer \
// REDEFINE:                     -fopenacc-implicit-worker
// RUN: %{acc-check-dmp}
//------------------------------------------------------------------------------
// gang and worker=vector-outer.
// REDEFINE: %{all:fc:pres} = GWvo
//
// REDEFINE: %{all:clang:args} = -fopenacc-implicit-worker=vector-outer
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}
//
// REDEFINE: %{all:clang:args} = -fno-openacc-implicit-worker \
// REDEFINE:                     -fopenacc-implicit-worker=vector-outer
// RUN: %{acc-check-dmp}
//
// REDEFINE: %{all:clang:args} = -fopenacc-implicit-worker=none \
// REDEFINE:                     -fopenacc-implicit-worker=vector-outer
// RUN: %{acc-check-dmp}
//
// REDEFINE: %{all:clang:args} = -fopenacc-implicit-worker=vector \
// REDEFINE:                     -fopenacc-implicit-worker=vector-outer
// RUN: %{acc-check-dmp}
//
// REDEFINE: %{all:clang:args} = -fopenacc-implicit-worker=outer \
// REDEFINE:                     -fopenacc-implicit-worker=vector-outer
// RUN: %{acc-check-dmp}
//------------------------------------------------------------------------------
// gang and vector.
// REDEFINE: %{all:fc:pres} = GV
//
// REDEFINE: %{all:clang:args} = -fopenacc-implicit-vector=outer
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}
//
// REDEFINE: %{all:clang:args} = -fno-openacc-implicit-vector \
// REDEFINE:                     -fopenacc-implicit-vector=outer
// RUN: %{acc-check-dmp}
//
// REDEFINE: %{all:clang:args} = -fopenacc-implicit-vector=none \
// REDEFINE:                     -fopenacc-implicit-vector=outer
// RUN: %{acc-check-dmp}
//
// REDEFINE: %{all:clang:args} = -fno-openacc-implicit-vector \
// REDEFINE:                     -fopenacc-implicit-vector
// RUN: %{acc-check-dmp}
//
// REDEFINE: %{all:clang:args} = -fopenacc-implicit-vector=none \
// REDEFINE:                     -fopenacc-implicit-vector
// RUN: %{acc-check-dmp}
//------------------------------------------------------------------------------
// gang, worker=vector, and vector.
// REDEFINE: %{all:fc:pres} = GWvV
//
// REDEFINE: %{all:clang:args} = -fopenacc-implicit-worker=vector \
// REDEFINE:                     -fopenacc-implicit-vector=outer
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}
//------------------------------------------------------------------------------
// gang, worker=outer, and vector.
// REDEFINE: %{all:fc:pres} = GWoV
//
// REDEFINE: %{all:clang:args} = -fopenacc-implicit-worker=outer \
// REDEFINE:                     -fopenacc-implicit-vector=outer
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}
//------------------------------------------------------------------------------
// gang, worker=vector-outer, and vector.
// REDEFINE: %{all:fc:pres} = GWvoV
//
// REDEFINE: %{all:clang:args} = -fopenacc-implicit-worker=vector-outer \
// REDEFINE:                     -fopenacc-implicit-vector=outer
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}
//
// END.

/* expected-no-diagnostics */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  // PRT: count[i][j][k] = 0;
  int count[8][2][8];
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 2; ++j)
      for (int k = 0; k < 8; ++k)
        count[i][j][k] = 0;
  //  PRT-A-NEXT: #pragma acc parallel {{.*}}
  // PRT-AO-NEXT: // #pragma omp target teams {{.*}}
  //  PRT-O-NEXT: #pragma omp target teams {{.*}}
  // PRT-OA-NEXT: // #pragma acc parallel {{.*}}
  #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8)
  //            DMP: ACCLoopDirective
  //       DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //       DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //       DMP-NEXT:     DeclRefExpr {{.*}} 'count'
  //       DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  //   DMP-GWo-NEXT:   ACCWorkerClause {{.*}} <implicit>
  //  DMP-GWoV-NEXT:   ACCWorkerClause {{.*}} <implicit>
  //     DMP-G-NEXT:   impl: OMPDistributeDirective
  //   DMP-GWv-NEXT:   impl: OMPDistributeDirective
  //   DMP-GWo-NEXT:   impl: OMPDistributeParallelForDirective
  //  DMP-GWvo-NEXT:   impl: OMPDistributeDirective
  //    DMP-GV-NEXT:   impl: OMPDistributeDirective
  //  DMP-GWvV-NEXT:   impl: OMPDistributeDirective
  //  DMP-GWoV-NEXT:   impl: OMPDistributeParallelForDirective
  // DMP-GWvoV-NEXT:   impl: OMPDistributeDirective
  //        DMP-NOT:     OMP
  //            DMP:     ForStmt
  //
  //        PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
  //     PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //   PRT-AO-GWv-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //   PRT-AO-GWo-NEXT: {{^ *}}// #pragma omp distribute parallel for{{$}}
  //  PRT-AO-GWvo-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //    PRT-AO-GV-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-AO-GWvV-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-AO-GWoV-NEXT: {{^ *}}// #pragma omp distribute parallel for{{$}}
  // PRT-AO-GWvoV-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //
  //      PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //    PRT-O-GWv-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //    PRT-O-GWo-NEXT: {{^ *}}#pragma omp distribute parallel for{{$}}
  //   PRT-O-GWvo-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //     PRT-O-GV-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //   PRT-O-GWvV-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //   PRT-O-GWoV-NEXT: {{^ *}}#pragma omp distribute parallel for{{$}}
  //  PRT-O-GWvoV-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //       PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
  //
  //          PRT-NEXT: for ({{.*}}) {
  #pragma acc loop
  for (int i = 0; i < 8; ++i) {
    //            DMP: ACCLoopDirective
    //       DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //       DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    //       DMP-NEXT:     DeclRefExpr {{.*}} 'count'
    //       DMP-NEXT:     DeclRefExpr {{.*}} 'i'
    //  DMP-GWvo-NEXT:   ACCWorkerClause {{.*}} <implicit>
    //    DMP-GV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //  DMP-GWvV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //  DMP-GWoV-NEXT:   ACCVectorClause {{.*}} <implicit>
    // DMP-GWvoV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    // DMP-GWvoV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //     DMP-G-NEXT:   impl: ForStmt
    //   DMP-GWv-NEXT:   impl: ForStmt
    //   DMP-GWo-NEXT:   impl: ForStmt
    //  DMP-GWvo-NEXT:   impl: OMPParallelForDirective
    //   DMP-GWvo-NOT:     OMP
    //       DMP-GWvo:     ForStmt
    //    DMP-GV-NEXT:   impl: OMPSimdDirective
    //    DMP-GV-NEXT:     OMPSimdlenClause
    //    DMP-GV-NEXT:       ConstantExpr
    //    DMP-GV-NEXT:         value: Int 8
    //    DMP-GV-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //     DMP-GV-NOT:     OMP
    //         DMP-GV:     ForStmt
    //  DMP-GWvV-NEXT:   impl: OMPSimdDirective
    //  DMP-GWvV-NEXT:     OMPSimdlenClause
    //  DMP-GWvV-NEXT:       ConstantExpr
    //  DMP-GWvV-NEXT:         value: Int 8
    //  DMP-GWvV-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //   DMP-GWvV-NOT:     OMP
    //       DMP-GWvV:     ForStmt
    //  DMP-GWoV-NEXT:   impl: OMPSimdDirective
    //  DMP-GWoV-NEXT:     OMPSimdlenClause
    //  DMP-GWoV-NEXT:       ConstantExpr
    //  DMP-GWoV-NEXT:         value: Int 8
    //  DMP-GWoV-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //   DMP-GWoV-NOT:     OMP
    //       DMP-GWoV:     ForStmt
    // DMP-GWvoV-NEXT:   impl: OMPParallelForSimdDirective
    // DMP-GWvoV-NEXT:     OMPSimdlenClause
    // DMP-GWvoV-NEXT:       ConstantExpr
    // DMP-GWvoV-NEXT:         value: Int 8
    // DMP-GWvoV-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //  DMP-GWvoV-NOT:     OMP
    //      DMP-GWvoV:     ForStmt
    //
    //        PRT-A-NEXT: {{^ *}}#pragma acc loop
    //     PRT-AO-G-SAME: {{^}} // discarded in OpenMP translation
    //   PRT-AO-GWv-SAME: {{^}} // discarded in OpenMP translation
    //   PRT-AO-GWo-SAME: {{^}} // discarded in OpenMP translation
    //        PRT-A-SAME: {{^$}}
    //  PRT-AO-GWvo-NEXT: {{^ *}}// #pragma omp parallel for{{$}}
    //    PRT-AO-GV-NEXT: {{^ *}}// #pragma omp simd simdlen(8){{$}}
    //  PRT-AO-GWvV-NEXT: {{^ *}}// #pragma omp simd simdlen(8){{$}}
    //  PRT-AO-GWoV-NEXT: {{^ *}}// #pragma omp simd simdlen(8){{$}}
    // PRT-AO-GWvoV-NEXT: {{^ *}}// #pragma omp parallel for simd simdlen(8){{$}}
    //
    //   PRT-O-GWvo-NEXT: {{^ *}}#pragma omp parallel for{{$}}
    //     PRT-O-GV-NEXT: {{^ *}}#pragma omp simd simdlen(8){{$}}
    //   PRT-O-GWvV-NEXT: {{^ *}}#pragma omp simd simdlen(8){{$}}
    //   PRT-O-GWoV-NEXT: {{^ *}}#pragma omp simd simdlen(8){{$}}
    //  PRT-O-GWvoV-NEXT: {{^ *}}#pragma omp parallel for simd simdlen(8){{$}}
    //       PRT-OA-NEXT: {{^ *}}// #pragma acc loop
    //     PRT-OA-G-SAME: {{^}} // discarded in OpenMP translation
    //   PRT-OA-GWv-SAME: {{^}} // discarded in OpenMP translation
    //   PRT-OA-GWo-SAME: {{^}} // discarded in OpenMP translation
    //       PRT-OA-SAME: {{^$}}
    //
    //          PRT-NEXT: for ({{.*}})
    //          PRT-NEXT:   ++count{{.*}};
    #pragma acc loop
    for (int j = 0; j < 8; ++j)
      ++count[i][0][j];
    //            DMP: ACCLoopDirective
    //       DMP-NEXT:   ACCVectorClause
    //        DMP-NOT:     <implicit>
    //       DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //       DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    //       DMP-NEXT:     DeclRefExpr {{.*}} 'count'
    //       DMP-NEXT:     DeclRefExpr {{.*}} 'i'
    //   DMP-GWv-NEXT:   ACCWorkerClause {{.*}} <implicit>
    //  DMP-GWvo-NEXT:   ACCWorkerClause {{.*}} <implicit>
    //  DMP-GWvV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    // DMP-GWvoV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    //     DMP-G-NEXT:   impl: OMPSimdDirective
    //   DMP-GWv-NEXT:   impl: OMPParallelForSimdDirective
    //  DMP-GWvo-NEXT:   impl: OMPParallelForSimdDirective
    //   DMP-GWo-NEXT:   impl: OMPSimdDirective
    //    DMP-GV-NEXT:   impl: OMPSimdDirective
    //  DMP-GWvV-NEXT:   impl: OMPParallelForSimdDirective
    //  DMP-GWoV-NEXT:   impl: OMPSimdDirective
    // DMP-GWvoV-NEXT:   impl: OMPParallelForSimdDirective
    //       DMP-NEXT:     SimdlenClause
    //       DMP-NEXT:       ConstantExpr
    //       DMP-NEXT:         value: Int 8
    //       DMP-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //        DMP-NOT:     OMP
    //            DMP:     ForStmt
    //
    //        PRT-A-NEXT: {{^ *}}#pragma acc loop vector{{$}}
    //     PRT-AO-G-NEXT: {{^ *}}// #pragma omp simd simdlen(8){{$}}
    //   PRT-AO-GWv-NEXT: {{^ *}}// #pragma omp parallel for simd simdlen(8){{$}}
    //  PRT-AO-GWvo-NEXT: {{^ *}}// #pragma omp parallel for simd simdlen(8){{$}}
    //   PRT-AO-GWo-NEXT: {{^ *}}// #pragma omp simd simdlen(8){{$}}
    //    PRT-AO-GV-NEXT: {{^ *}}// #pragma omp simd simdlen(8){{$}}
    //  PRT-AO-GWvV-NEXT: {{^ *}}// #pragma omp parallel for simd simdlen(8){{$}}
    //  PRT-AO-GWoV-NEXT: {{^ *}}// #pragma omp simd simdlen(8){{$}}
    // PRT-AO-GWvoV-NEXT: {{^ *}}// #pragma omp parallel for simd simdlen(8){{$}}
    //
    //     PRT-O-G-NEXT: {{^ *}}#pragma omp simd simdlen(8){{$}}
    //   PRT-O-GWv-NEXT: {{^ *}}#pragma omp parallel for simd simdlen(8){{$}}
    //  PRT-O-GWvo-NEXT: {{^ *}}#pragma omp parallel for simd simdlen(8){{$}}
    //   PRT-O-GWo-NEXT: {{^ *}}#pragma omp simd simdlen(8){{$}}
    //    PRT-O-GV-NEXT: {{^ *}}#pragma omp simd simdlen(8){{$}}
    //  PRT-O-GWvV-NEXT: {{^ *}}#pragma omp parallel for simd simdlen(8){{$}}
    //  PRT-O-GWoV-NEXT: {{^ *}}#pragma omp simd simdlen(8){{$}}
    // PRT-O-GWvoV-NEXT: {{^ *}}#pragma omp parallel for simd simdlen(8){{$}}
    //      PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector{{$}}
    //
    //     PRT-NEXT: for ({{.*}})
    //     PRT-NEXT:   ++count{{.*}};
    #pragma acc loop vector
    for (int j = 0; j < 8; ++j)
      ++count[i][1][j];
  } // PRT: }
  int total = 0;
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 2; ++j)
      for (int k = 0; k < 8; ++k)
        total += count[i][j][k];
  // EXE:total = 128
  printf("total = %d\n", total);
  return 0;
}
