// Check implicit gang, worker, and vector clause on "acc parallel loop", on
// "acc loop" within "acc parallel", and on orphaned "acc loop".
//
// Implicit gang, worker, and vector clauses permitted by implicit routine
// directives are checked in routine-implicit.cpp.
//
// We check with implicit worker and vector clauses both enabled or both
// disabled.  fopenacc-implicit-worker-vector.c checks various combinations of
// -f[no-]openacc-implicit-{worker,vector}.
//
// Positive testing for gang reductions appears in loop-reduction.c.  Testing
// for diagnostics about conflicting gang reductions appears in
// diagnostics/loop.c.
//
// FIXME: amdgcn doesn't yet support printf in a kernel.  Unfortunately, that
// means our execution checks on amdgcn don't verify much except that nothing
// crashes.
//
// REDEFINE: %{dmp:fc:args} = -strict-whitespace
// REDEFINE: %{exe:fc:args} = -strict-whitespace -match-full-lines \
// REDEFINE:                  -implicit-check-not='{{.}}'
//
// REDEFINE: %{all:fc:pres} = G
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}
//
// REDEFINE: %{all:clang:args} = -fopenacc-implicit-worker \
// REDEFINE:                     -fopenacc-implicit-vector
// REDEFINE: %{all:fc:pres} = GWV
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}
//
// END.

/* expected-no-diagnostics */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if TGT_USE_STDIO
# define TGT_PRINTF(...) printf(__VA_ARGS__)
#else
# define TGT_PRINTF(...)
#endif

// These are used to check implicit gang clauses on loops enclosing calls to
// functions with routine directives.
#pragma acc routine gang
static void gangFn(const char *msg, int i) { TGT_PRINTF("%s: %d\n", msg, i); }
#pragma acc routine worker
static void workerFn(const char *msg, int i) { TGT_PRINTF("%s: %d\n", msg, i); }
#pragma acc routine vector
static void vectorFn(const char *msg, int i) { TGT_PRINTF("%s: %d\n", msg, i); }
#pragma acc routine seq
static void seqFn(const char *msg, int i) { TGT_PRINTF("%s: %d\n", msg, i); }
static void impFn(const char *msg, int i) { TGT_PRINTF("%s: %d\n", msg, i); }

// These are used to check implicit gang clauses within functions with routine
// directives.
#pragma acc routine gang
static void withinGangFn();
#pragma acc routine worker
static void withinWorkerFn();
#pragma acc routine vector
static void withinVectorFn();
#pragma acc routine seq
static void withinSeqFn();

//==============================================================================
// Parallel constructs can contain implicit gang, worker, and vector clauses.
//==============================================================================

// PRT: int main() {
int main() {
  //----------------------------------------------------------------------------
  // gang, worker, vector, seq, auto combinations (without loop nesting, which
  // is checked later)
  //----------------------------------------------------------------------------

  //............................................................................
  // First for acc parallel and acc loop separately.

  // DMP-LABEL:"acc loop clause combinations without nesting\n"
  // PRT-LABEL:"acc loop clause combinations without nesting\n"
  // EXE-LABEL:acc loop clause combinations without nesting
  printf("acc loop clause combinations without nesting\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //  DMP-NOT:     OMP
  //      DMP:     CompoundStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  //    PRT-NEXT: {
  #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8)
  {
    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-GWV-NEXT:     OMPNum_threadsClause
    // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
    // DMP-GWV-NEXT:     OMPSimdlenClause
    // DMP-GWV-NEXT:       ConstantExpr
    // DMP-GWV-NEXT:         value: Int 8
    // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //      DMP-NOT:     OMP
    //          DMP:     CallExpr
    //          DMP:       ForStmt
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    //        PRT-NEXT: for ({{.*}})
    //        PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG:acc loop without nesting: 0
    // EXE-TGT-USE-STDIO-DAG:acc loop without nesting: 1
    #pragma acc loop
    for (int i = 0; i < 2; ++i)
      TGT_PRINTF("acc loop without nesting: %d\n", i);

    //      DMP-NOT: OMP
    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCGangClause
    //      DMP-NOT:     <implicit>
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-GWV-NEXT:     OMPNum_threadsClause
    // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
    // DMP-GWV-NEXT:     OMPSimdlenClause
    // DMP-GWV-NEXT:       ConstantExpr
    // DMP-GWV-NEXT:         value: Int 8
    // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //      DMP-NOT:     OMP
    //          DMP:     CallExpr
    //          DMP:       ForStmt
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop gang{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
    //        PRT-NEXT: for ({{.*}})
    //        PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG:acc loop gang without nesting: 0
    // EXE-TGT-USE-STDIO-DAG:acc loop gang without nesting: 1
    #pragma acc loop gang
    for (int i = 0; i < 2; ++i)
      TGT_PRINTF("acc loop gang without nesting: %d\n", i);

    //      DMP-NOT: OMP
    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCWorkerClause
    //      DMP-NOT:     <implicit>
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeParallelForDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    //     DMP-NEXT:     OMPNum_threadsClause
    //     DMP-NEXT:       IntegerLiteral {{.*}} 4
    // DMP-GWV-NEXT:     OMPSimdlenClause
    // DMP-GWV-NEXT:       ConstantExpr
    // DMP-GWV-NEXT:         value: Int 8
    // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //      DMP-NOT:     OMP
    //          DMP:     CallExpr
    //          DMP:       ForStmt
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop worker{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(4){{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute parallel for num_threads(4){{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker{{$}}
    //        PRT-NEXT: for ({{.*}})
    //        PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG:acc loop worker without nesting: 0
    // EXE-TGT-USE-STDIO-DAG:acc loop worker without nesting: 1
    #pragma acc loop worker
    for (int i = 0; i < 2; ++i)
      TGT_PRINTF("acc loop worker without nesting: %d\n", i);

    //      DMP-NOT: OMP
    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCVectorClause
    //      DMP-NOT:     <implicit>
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeSimdDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-GWV-NEXT:     OMPNum_threadsClause
    // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
    //     DMP-NEXT:     OMPSimdlenClause
    //     DMP-NEXT:       ConstantExpr
    //     DMP-NEXT:         value: Int 8
    //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //      DMP-NOT:     OMP
    //          DMP:     ForStmt
    //          DMP:       CallExpr
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop vector{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute simd simdlen(8){{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute simd simdlen(8){{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector{{$}}
    //        PRT-NEXT: for ({{.*}})
    //        PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG:acc loop vector without nesting: 0
    // EXE-TGT-USE-STDIO-DAG:acc loop vector without nesting: 1
    #pragma acc loop vector
    for (int i = 0; i < 2; ++i)
      TGT_PRINTF("acc loop vector without nesting: %d\n", i);

    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   impl: ForStmt
    //      DMP:     CallExpr
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG:acc loop seq without nesting: 0
    // EXE-TGT-USE-STDIO-DAG:acc loop seq without nesting: 0
    // EXE-TGT-USE-STDIO-DAG:acc loop seq without nesting: 1
    // EXE-TGT-USE-STDIO-DAG:acc loop seq without nesting: 1
    #pragma acc loop seq
    for (int i = 0; i < 2; ++i)
      TGT_PRINTF("acc loop seq without nesting: %d\n", i);

    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCAutoClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   impl: ForStmt
    //      DMP:     CallExpr
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG:acc loop auto without nesting: 0
    // EXE-TGT-USE-STDIO-DAG:acc loop auto without nesting: 0
    // EXE-TGT-USE-STDIO-DAG:acc loop auto without nesting: 1
    // EXE-TGT-USE-STDIO-DAG:acc loop auto without nesting: 1
    #pragma acc loop auto
    for (int i = 0; i < 2; ++i)
      TGT_PRINTF("acc loop auto without nesting: %d\n", i);

    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCGangClause
    //      DMP-NOT:     <implicit>
    //     DMP-NEXT:   ACCWorkerClause
    //      DMP-NOT:     <implicit>
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeParallelForDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    //     DMP-NEXT:     OMPNum_threadsClause
    //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
    // DMP-GWV-NEXT:     OMPSimdlenClause
    // DMP-GWV-NEXT:       ConstantExpr
    // DMP-GWV-NEXT:         value: Int 8
    // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //      DMP-NOT:     OMP
    //          DMP:     CallExpr
    //          DMP:       ForStmt
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop gang worker{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(4){{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute parallel for num_threads(4){{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang worker{{$}}
    //        PRT-NEXT: for ({{.*}})
    //        PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG:acc loop gang worker without nesting: 0
    // EXE-TGT-USE-STDIO-DAG:acc loop gang worker without nesting: 1
    #pragma acc loop gang worker
    for (int i = 0; i < 2; ++i)
      TGT_PRINTF("acc loop gang worker without nesting: %d\n", i);

    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCGangClause
    //      DMP-NOT:     <implicit>
    //     DMP-NEXT:   ACCVectorClause
    //      DMP-NOT:     <implicit>
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeSimdDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-GWV-NEXT:     OMPNum_threadsClause
    // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
    //     DMP-NEXT:     OMPSimdlenClause
    //     DMP-NEXT:       ConstantExpr
    //     DMP-NEXT:         value: Int 8
    //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //      DMP-NOT:     OMP
    //          DMP:     CallExpr
    //          DMP:       ForStmt
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop gang vector{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute simd simdlen(8){{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute simd simdlen(8){{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang vector{{$}}
    //        PRT-NEXT: for ({{.*}})
    //        PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG:acc loop gang vector without nesting: 0
    // EXE-TGT-USE-STDIO-DAG:acc loop gang vector without nesting: 1
    #pragma acc loop gang vector
    for (int i = 0; i < 2; ++i)
      TGT_PRINTF("acc loop gang vector without nesting: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCVectorClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-NEXT:     OMPNum_threadsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:     OMPSimdlenClause
    // DMP-NEXT:       ConstantExpr
    // DMP-NEXT:         value: Int 8
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //  DMP-NOT:     OMP
    //      DMP:     CallExpr
    //      DMP:       ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG:acc loop worker vector without nesting: 0
    // EXE-TGT-USE-STDIO-DAG:acc loop worker vector without nesting: 1
    #pragma acc loop worker vector
    for (int i = 0; i < 2; ++i)
      TGT_PRINTF("acc loop worker vector without nesting: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:   ACCVectorClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-NEXT:     OMPNum_threadsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:     OMPSimdlenClause
    // DMP-NEXT:       ConstantExpr
    // DMP-NEXT:         value: Int 8
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //  DMP-NOT:     OMP
    //      DMP:     CallExpr
    //      DMP:       ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang worker vector{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang worker vector{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG:acc loop gang worker vector without nesting: 0
    // EXE-TGT-USE-STDIO-DAG:acc loop gang worker vector without nesting: 1
    #pragma acc loop gang worker vector
    for (int i = 0; i < 2; ++i)
      TGT_PRINTF("acc loop gang worker vector without nesting: %d\n", i);

    //  DMP-NOT:  OMP
    //      DMP:  ACCLoopDirective
    // DMP-NEXT:   ACCAutoClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCGangClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   impl: ForStmt
    //      DMP:     CallExpr
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto gang
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto gang // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG:acc loop auto gang without nesting: 0
    // EXE-TGT-USE-STDIO-DAG:acc loop auto gang without nesting: 0
    // EXE-TGT-USE-STDIO-DAG:acc loop auto gang without nesting: 1
    // EXE-TGT-USE-STDIO-DAG:acc loop auto gang without nesting: 1
    #pragma acc loop auto gang
    for (int i = 0; i < 2; ++i)
      TGT_PRINTF("acc loop auto gang without nesting: %d\n", i);
  } // PRT-NEXT: }

  //............................................................................
  // Repeat for acc parallel loop.

  // DMP-LABEL:"acc parallel loop clause combinations without nesting\n"
  // PRT-LABEL:"acc parallel loop clause combinations without nesting\n"
  // EXE-LABEL:acc parallel loop clause combinations without nesting
  printf("acc parallel loop clause combinations without nesting\n");

  //          DMP: ACCParallelLoopDirective
  //     DMP-NEXT:   ACCNumGangsClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:   ACCNumWorkersClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:   ACCVectorLengthClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     ACCNumWorkersClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:     ACCVectorLengthClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCWorkerClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:       impl: OMPDistributeDirective
  // DMP-GWV-NEXT:       impl: OMPDistributeParallelForSimdDirective
  // DMP-GWV-NEXT:         OMPNum_threadsClause
  // DMP-GWV-NEXT:           IntegerLiteral {{.*}} 4
  // DMP-GWV-NEXT:         OMPSimdlenClause
  // DMP-GWV-NEXT:           ConstantExpr
  // DMP-GWV-NEXT:             value: Int 8
  // DMP-GWV-NEXT:             IntegerLiteral {{.*}} 'int' 8
  //      DMP-NOT:         OMP
  //          DMP:         ForStmt
  //          DMP:           CallExpr
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  //     PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //      PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  //        PRT-NEXT: for ({{.*}})
  //        PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop without nesting: 1
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8)
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("acc parallel loop without nesting: %d\n", i);

  // DMP-LABEL:"acc parallel loop gang without nesting\n"
  // PRT-LABEL:"acc parallel loop gang without nesting\n"
  // EXE-LABEL:acc parallel loop gang without nesting
  printf("acc parallel loop gang without nesting\n");

  //          DMP: ACCParallelLoopDirective
  //     DMP-NEXT:   ACCNumGangsClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:   ACCNumWorkersClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:   ACCVectorLengthClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:   ACCGangClause
  //      DMP-NOT:     <implicit>
  //     DMP-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     ACCNumWorkersClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:     ACCVectorLengthClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCGangClause
  //      DMP-NOT:         <implicit>
  //     DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCWorkerClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:       impl: OMPDistributeDirective
  // DMP-GWV-NEXT:       impl: OMPDistributeParallelForSimdDirective
  // DMP-GWV-NEXT:       OMPNum_threadsClause
  // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 4
  // DMP-GWV-NEXT:       OMPSimdlenClause
  // DMP-GWV-NEXT:         ConstantExpr
  // DMP-GWV-NEXT:           value: Int 8
  // DMP-GWV-NEXT:           IntegerLiteral {{.*}} 'int' 8
  //      DMP-NOT:         OMP
  //          DMP:         ForStmt
  //          DMP:           CallExpr
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang{{$}}
  //     PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //      PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang{{$}}
  //        PRT-NEXT: for ({{.*}})
  //        PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang without nesting: 1
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("acc parallel loop gang without nesting: %d\n", i);

  // DMP-LABEL:"acc parallel loop worker without nesting\n"
  // PRT-LABEL:"acc parallel loop worker without nesting\n"
  // EXE-LABEL:acc parallel loop worker without nesting
  printf("acc parallel loop worker without nesting\n");

  //          DMP: ACCParallelLoopDirective
  //     DMP-NEXT:   ACCNumGangsClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:   ACCNumWorkersClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:   ACCVectorLengthClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:   ACCWorkerClause
  //      DMP-NOT:     <implicit>
  //     DMP-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     ACCNumWorkersClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:     ACCVectorLengthClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCWorkerClause
  //      DMP-NOT:         <implicit>
  //     DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:       impl: OMPDistributeParallelForDirective
  // DMP-GWV-NEXT:       impl: OMPDistributeParallelForSimdDirective
  //     DMP-NEXT:         OMPNum_threadsClause
  //     DMP-NEXT:           IntegerLiteral {{.*}} 'int' 4
  // DMP-GWV-NEXT:         OMPSimdlenClause
  // DMP-GWV-NEXT:           ConstantExpr
  // DMP-GWV-NEXT:             value: Int 8
  // DMP-GWV-NEXT:             IntegerLiteral {{.*}} 'int' 8
  //      DMP-NOT:         OMP
  //          DMP:         ForStmt
  //          DMP:           CallExpr
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) worker{{$}}
  //     PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(4){{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //      PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute parallel for num_threads(4){{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) worker{{$}}
  //        PRT-NEXT: for ({{.*}})
  //        PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop worker without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop worker without nesting: 1
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) worker
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("acc parallel loop worker without nesting: %d\n", i);

  // DMP-LABEL:"acc parallel loop vector without nesting\n"
  // PRT-LABEL:"acc parallel loop vector without nesting\n"
  // EXE-LABEL:acc parallel loop vector without nesting
  printf("acc parallel loop vector without nesting\n");

  //          DMP: ACCParallelLoopDirective
  //     DMP-NEXT:   ACCNumGangsClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:   ACCNumWorkersClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:   ACCVectorLengthClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:   ACCVectorClause
  //      DMP-NOT:     <implicit>
  //     DMP-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     ACCNumWorkersClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:     ACCVectorLengthClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCVectorClause
  //      DMP-NOT:         <implicit>
  //     DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCWorkerClause {{.*}} <implicit>
  //   DMP-G-NEXT:       impl: OMPDistributeSimdDirective
  // DMP-GWV-NEXT:       impl: OMPDistributeParallelForSimdDirective
  // DMP-GWV-NEXT:         OMPNum_threadsClause
  // DMP-GWV-NEXT:           IntegerLiteral {{.*}} 4
  //     DMP-NEXT:         OMPSimdlenClause
  //     DMP-NEXT:           ConstantExpr
  //     DMP-NEXT:             value: Int 8
  //     DMP-NEXT:             IntegerLiteral {{.*}} 'int' 8
  //      DMP-NOT:         OMP
  //          DMP:         ForStmt
  //          DMP:           CallExpr
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) vector{{$}}
  //     PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute simd simdlen(8){{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //      PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute simd simdlen(8){{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) vector{{$}}
  //        PRT-NEXT: for ({{.*}})
  //        PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop vector without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop vector without nesting: 1
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) vector
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("acc parallel loop vector without nesting: %d\n", i);

  // DMP-LABEL:"acc parallel loop seq without nesting\n"
  // PRT-LABEL:"acc parallel loop seq without nesting\n"
  // EXE-LABEL:acc parallel loop seq without nesting
  printf("acc parallel loop seq without nesting\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCSeqClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCSeqClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       impl: ForStmt
  //      DMP:         CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) seq{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop seq without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop seq without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop seq without nesting: 1
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop seq without nesting: 1
  #pragma acc parallel loop num_gangs(2) seq
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("acc parallel loop seq without nesting: %d\n", i);

  // DMP-LABEL:"acc parallel loop auto without nesting\n"
  // PRT-LABEL:"acc parallel loop auto without nesting\n"
  // EXE-LABEL:acc parallel loop auto without nesting
  printf("acc parallel loop auto without nesting\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCAutoClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       impl: ForStmt
  //      DMP:         CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) auto{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) auto{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop auto without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop auto without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop auto without nesting: 1
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop auto without nesting: 1
  #pragma acc parallel loop num_gangs(2) auto
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("acc parallel loop auto without nesting: %d\n", i);

  // DMP-LABEL:"acc parallel loop gang worker without nesting\n"
  // PRT-LABEL:"acc parallel loop gang worker without nesting\n"
  // EXE-LABEL:acc parallel loop gang worker without nesting
  printf("acc parallel loop gang worker without nesting\n");

  //          DMP: ACCParallelLoopDirective
  //     DMP-NEXT:   ACCNumGangsClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:   ACCNumWorkersClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:   ACCVectorLengthClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:   ACCGangClause
  //      DMP-NOT:     <implicit>
  //     DMP-NEXT:   ACCWorkerClause
  //      DMP-NOT:     <implicit>
  //     DMP-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     ACCNumWorkersClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:     ACCVectorLengthClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCGangClause
  //      DMP-NOT:         <implicit>
  //     DMP-NEXT:       ACCWorkerClause
  //      DMP-NOT:         <implicit>
  //     DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:       impl: OMPDistributeParallelForDirective
  // DMP-GWV-NEXT:       impl: OMPDistributeParallelForSimdDirective
  //     DMP-NEXT:         OMPNum_threadsClause
  //     DMP-NEXT:           IntegerLiteral {{.*}} 'int' 4
  // DMP-GWV-NEXT:         OMPSimdlenClause
  // DMP-GWV-NEXT:           ConstantExpr
  // DMP-GWV-NEXT:             value: Int 8
  // DMP-GWV-NEXT:             IntegerLiteral {{.*}} 'int' 8
  //      DMP-NOT:         OMP
  //          DMP:         ForStmt
  //          DMP:           CallExpr
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang worker{{$}}
  //     PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(4){{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //      PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute parallel for num_threads(4){{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang worker{{$}}
  //        PRT-NEXT: for ({{.*}})
  //        PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang worker without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang worker without nesting: 1
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang worker
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("acc parallel loop gang worker without nesting: %d\n", i);

  // DMP-LABEL:"acc parallel loop gang vector without nesting\n"
  // PRT-LABEL:"acc parallel loop gang vector without nesting\n"
  // EXE-LABEL:acc parallel loop gang vector without nesting
  printf("acc parallel loop gang vector without nesting\n");

  //          DMP: ACCParallelLoopDirective
  //     DMP-NEXT:   ACCNumGangsClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:   ACCNumWorkersClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:   ACCVectorLengthClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:   ACCGangClause
  //      DMP-NOT:     <implicit>
  //     DMP-NEXT:   ACCVectorClause
  //      DMP-NOT:     <implicit>
  //     DMP-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     ACCNumWorkersClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:     ACCVectorLengthClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCGangClause
  //      DMP-NOT:         <implicit>
  //     DMP-NEXT:       ACCVectorClause
  //      DMP-NOT:         <implicit>
  //     DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCWorkerClause {{.*}} <implicit>
  //   DMP-G-NEXT:       impl: OMPDistributeSimdDirective
  // DMP-GWV-NEXT:       impl: OMPDistributeParallelForSimdDirective
  // DMP-GWV-NEXT:         OMPNum_threadsClause
  // DMP-GWV-NEXT:           IntegerLiteral {{.*}} 4
  //     DMP-NEXT:         OMPSimdlenClause
  //     DMP-NEXT:           ConstantExpr
  //     DMP-NEXT:             value: Int 8
  //     DMP-NEXT:             IntegerLiteral {{.*}} 'int' 8
  //      DMP-NOT:         OMP
  //          DMP:         ForStmt
  //          DMP:           CallExpr
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang vector{{$}}
  //     PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute simd simdlen(8){{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //      PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute simd simdlen(8){{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang vector{{$}}
  //        PRT-NEXT: for ({{.*}})
  //        PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang vector without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang vector without nesting: 1
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang vector
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("acc parallel loop gang vector without nesting: %d\n", i);

  // DMP-LABEL:"acc parallel loop worker vector without nesting\n"
  // PRT-LABEL:"acc parallel loop worker vector without nesting\n"
  // EXE-LABEL:acc parallel loop worker vector without nesting
  printf("acc parallel loop worker vector without nesting\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCNumWorkersClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCWorkerClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCVectorClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeParallelForSimdDirective
  // DMP-NEXT:         OMPNum_threadsClause
  // DMP-NEXT:           IntegerLiteral {{.*}} 'int' 4
  //  DMP-NOT:         OMP
  // DMP-NEXT:         OMPSimdlenClause
  // DMP-NEXT:           ConstantExpr
  // DMP-NEXT:             value: Int 8
  // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 8
  //      DMP:         ForStmt
  //      DMP:           CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) worker vector{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) worker vector{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop worker vector without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop worker vector without nesting: 1
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) worker vector
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("acc parallel loop worker vector without nesting: %d\n", i);

  // DMP-LABEL:"acc parallel loop gang worker vector without nesting\n"
  // PRT-LABEL:"acc parallel loop gang worker vector without nesting\n"
  // EXE-LABEL:acc parallel loop gang worker vector without nesting
  printf("acc parallel loop gang worker vector without nesting\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   ACCGangClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCNumWorkersClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCGangClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCWorkerClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCVectorClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeParallelForSimdDirective
  // DMP-NEXT:         OMPNum_threadsClause
  // DMP-NEXT:           IntegerLiteral {{.*}} 'int' 4
  //  DMP-NOT:         OMP
  // DMP-NEXT:         OMPSimdlenClause
  // DMP-NEXT:           ConstantExpr
  // DMP-NEXT:             value: Int 8
  // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 8
  //      DMP:         ForStmt
  //      DMP:           CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang worker vector{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang worker vector{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang worker vector without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang worker vector without nesting: 1
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang worker vector
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("acc parallel loop gang worker vector without nesting: %d\n", i);

  // DMP-LABEL:"acc parallel loop gang auto without nesting\n"
  // PRT-LABEL:"acc parallel loop gang auto without nesting\n"
  // EXE-LABEL:acc parallel loop gang auto without nesting
  printf("acc parallel loop gang auto without nesting\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCGangClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCGangClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCAutoClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       impl: ForStmt
  //      DMP:         CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) gang auto{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) gang auto{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang auto without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang auto without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang auto without nesting: 1
  // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang auto without nesting: 1
  #pragma acc parallel loop num_gangs(2) gang auto
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("acc parallel loop gang auto without nesting: %d\n", i);

  //----------------------------------------------------------------------------
  // gang, worker, vector, seq, auto combinations on enclosing acc loop.
  //----------------------------------------------------------------------------

  //............................................................................
  // First for acc parallel and acc loop separately.

  // DMP-LABEL:"acc loop clause combinations enclosing acc loop\n"
  // PRT-LABEL:"acc loop clause combinations enclosing acc loop\n"
  // EXE-LABEL:acc loop clause combinations enclosing acc loop
  printf("acc loop clause combinations enclosing acc loop\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //  DMP-NOT:     OMP
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8)
  // DMP: CompoundStmt
  // PRT-NEXT: {
  {
    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-GWV-NEXT:     OMPNum_threadsClause
    // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
    // DMP-GWV-NEXT:     OMPSimdlenClause
    // DMP-GWV-NEXT:       ConstantExpr
    // DMP-GWV-NEXT:         value: Int 8
    // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //      DMP-NOT:     OMP
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    #pragma acc loop
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      //  DMP-NOT: OMP
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: ForStmt
      //  DMP-NOT:     OMP
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      //  PRT-A-SAME: {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
      //    PRT-NEXT: for ({{.*}})
      #pragma acc loop
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG:acc loop enclosing acc loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop enclosing acc loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:acc loop enclosing acc loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop enclosing acc loop: 1, 1
        TGT_PRINTF("acc loop enclosing acc loop: %d, %d\n", i, j);
    } // PRT-NEXT: }

    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCGangClause
    //      DMP-NOT:     <implicit>
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-GWV-NEXT:     OMPNum_threadsClause
    // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
    // DMP-GWV-NEXT:     OMPSimdlenClause
    // DMP-GWV-NEXT:       ConstantExpr
    // DMP-GWV-NEXT:         value: Int 8
    // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //      DMP-NOT:     OMP
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop gang{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
    #pragma acc loop gang
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      //  DMP-NOT: OMP
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: ForStmt
      //  DMP-NOT:     OMP
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      //  PRT-A-SAME: {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
      //    PRT-NEXT: for ({{.*}})
      #pragma acc loop
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG:acc loop gang enclosing acc loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop gang enclosing acc loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:acc loop gang enclosing acc loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop gang enclosing acc loop: 1, 1
        TGT_PRINTF("acc loop gang enclosing acc loop: %d, %d\n", i, j);
    } // PRT-NEXT: }

    //      DMP-NOT: OMP
    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCWorkerClause
    //      DMP-NOT:     <implicit>
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeParallelForDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    //     DMP-NEXT:     OMPNum_threadsClause
    //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
    // DMP-GWV-NEXT:     OMPSimdlenClause
    // DMP-GWV-NEXT:       ConstantExpr
    // DMP-GWV-NEXT:         value: Int 8
    // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //      DMP-NOT:     OMP
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop worker{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(4){{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute parallel for num_threads(4){{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker{{$}}
    #pragma acc loop worker
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}})
    for (int i = 0; i < 2; ++i)
      //  DMP-NOT: OMP
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: ForStmt
      //  DMP-NOT:     OMP
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      //  PRT-A-SAME: {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
      //    PRT-NEXT: for ({{.*}})
      #pragma acc loop
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG:acc loop worker enclosing acc loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop worker enclosing acc loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:acc loop worker enclosing acc loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop worker enclosing acc loop: 1, 1
        TGT_PRINTF("acc loop worker enclosing acc loop: %d, %d\n", i, j);

    //      DMP-NOT: OMP
    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCVectorClause
    //      DMP-NOT:     <implicit>
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeSimdDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-GWV-NEXT:     OMPNum_threadsClause
    // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
    //     DMP-NEXT:     OMPSimdlenClause
    //     DMP-NEXT:       ConstantExpr
    //     DMP-NEXT:         value: Int 8
    //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //      DMP-NOT:     OMP
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop vector{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute simd simdlen(8){{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute simd simdlen(8){{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector{{$}}
    #pragma acc loop vector
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}})
    for (int i = 0; i < 2; ++i)
      //  DMP-NOT: OMP
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: ForStmt
      //  DMP-NOT:     OMP
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      //  PRT-A-SAME: {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
      //    PRT-NEXT: for ({{.*}})
      #pragma acc loop
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG:acc loop vector enclosing acc loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop vector enclosing acc loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:acc loop vector enclosing acc loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop vector enclosing acc loop: 1, 1
        TGT_PRINTF("acc loop vector enclosing acc loop: %d, %d\n", i, j);

    //  DMP-NOT: OMP
    //      DMP:  ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   impl: ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}}) {
    #pragma acc loop seq
    for (int i = 0; i < 2; ++i) {
      //          DMP: ACCLoopDirective
      //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
      // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
      //   DMP-G-NEXT:   impl: OMPDistributeDirective
      // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
      // DMP-GWV-NEXT:     OMPNum_threadsClause
      // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
      // DMP-GWV-NEXT:     OMPSimdlenClause
      // DMP-GWV-NEXT:       ConstantExpr
      // DMP-GWV-NEXT:         value: Int 8
      // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
      //      DMP-NOT:     OMP
      //
      //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
      //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
      //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
      #pragma acc loop
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG:acc loop seq enclosing acc loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop seq enclosing acc loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:acc loop seq enclosing acc loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop seq enclosing acc loop: 1, 1
        TGT_PRINTF("acc loop seq enclosing acc loop: %d, %d\n", i, j);
    } // PRT-NEXT: }

    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCAutoClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   impl: ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto // discarded in OpenMP translation{{$}}
    #pragma acc loop auto
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      //          DMP: ACCLoopDirective
      //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
      // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
      //   DMP-G-NEXT:   impl: OMPDistributeDirective
      // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
      // DMP-GWV-NEXT:     OMPNum_threadsClause
      // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
      // DMP-GWV-NEXT:     OMPSimdlenClause
      // DMP-GWV-NEXT:       ConstantExpr
      // DMP-GWV-NEXT:         value: Int 8
      // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
      //      DMP-NOT:     OMP
      //
      //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
      //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
      //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
      #pragma acc loop
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG:acc loop auto enclosing acc loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop auto enclosing acc loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:acc loop auto enclosing acc loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop auto enclosing acc loop: 1, 1
        TGT_PRINTF("acc loop auto enclosing acc loop: %d, %d\n", i, j);
    } // PRT-NEXT: }

    //      DMP-NOT: OMP
    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCGangClause
    //      DMP-NOT:     <implicit>
    //     DMP-NEXT:   ACCWorkerClause
    //      DMP-NOT:     <implicit>
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeParallelForDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    //     DMP-NEXT:     OMPNum_threadsClause
    //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
    // DMP-GWV-NEXT:     OMPSimdlenClause
    // DMP-GWV-NEXT:       ConstantExpr
    // DMP-GWV-NEXT:         value: Int 8
    // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //      DMP-NOT:     OMP
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop gang worker{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(4){{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute parallel for num_threads(4){{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang worker{{$}}
    #pragma acc loop gang worker
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}})
    for (int i = 0; i < 2; ++i)
      //  DMP-NOT: OMP
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: ForStmt
      //  DMP-NOT:     OMP
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      //  PRT-A-SAME: {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
      //    PRT-NEXT: for ({{.*}})
      #pragma acc loop
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG:acc loop gang worker enclosing acc loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop gang worker enclosing acc loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:acc loop gang worker enclosing acc loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop gang worker enclosing acc loop: 1, 1
        TGT_PRINTF("acc loop gang worker enclosing acc loop: %d, %d\n", i, j);

    //      DMP-NOT: OMP
    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCGangClause
    //      DMP-NOT:     <implicit>
    //     DMP-NEXT:   ACCVectorClause
    //      DMP-NOT:     <implicit>
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeSimdDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-GWV-NEXT:     OMPNum_threadsClause
    // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
    //     DMP-NEXT:     OMPSimdlenClause
    //     DMP-NEXT:       ConstantExpr
    //     DMP-NEXT:         value: Int 8
    //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //      DMP-NOT:     OMP
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop gang vector{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute simd simdlen(8){{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute simd simdlen(8){{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang vector{{$}}
    #pragma acc loop gang vector
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}})
    for (int i = 0; i < 2; ++i)
      //  DMP-NOT: OMP
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: ForStmt
      //  DMP-NOT:     OMP
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      //  PRT-A-SAME: {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
      //    PRT-NEXT: for ({{.*}})
      #pragma acc loop
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG:acc loop gang vector enclosing acc loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop gang vector enclosing acc loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:acc loop gang vector enclosing acc loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop gang vector enclosing acc loop: 1, 1
        TGT_PRINTF("acc loop gang vector enclosing acc loop: %d, %d\n", i, j);

    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCVectorClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-NEXT:     OMPNum_threadsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 4
    // DMP-NEXT:     OMPSimdlenClause
    // DMP-NEXT:       ConstantExpr
    // DMP-NEXT:         value: Int 8
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //  DMP-NOT:     OMP
    //
    //    PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //   PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector{{$}}
    #pragma acc loop worker vector
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}})
    for (int i = 0; i < 2; ++i)
      //  DMP-NOT: OMP
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: ForStmt
      //  DMP-NOT:     OMP
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      //  PRT-A-SAME: {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
      //    PRT-NEXT: for ({{.*}})
      #pragma acc loop
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG:acc loop worker vector enclosing acc loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop worker vector enclosing acc loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:acc loop worker vector enclosing acc loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop worker vector enclosing acc loop: 1, 1
        TGT_PRINTF("acc loop worker vector enclosing acc loop: %d, %d\n", i, j);

    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCWorkerClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCVectorClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-NEXT:     OMPNum_threadsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 4
    // DMP-NEXT:     OMPSimdlenClause
    // DMP-NEXT:       ConstantExpr
    // DMP-NEXT:         value: Int 8
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //  DMP-NOT:     OMP
    //
    //    PRT-A-NEXT: {{^ *}}#pragma acc loop gang worker vector{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //   PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang worker vector{{$}}
    #pragma acc loop gang worker vector
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}})
    for (int i = 0; i < 2; ++i)
      //  DMP-NOT: OMP
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: ForStmt
      //  DMP-NOT:     OMP
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      //  PRT-A-SAME: {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
      //    PRT-NEXT: for ({{.*}})
      #pragma acc loop
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG:acc loop gang worker vector enclosing acc loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop gang worker vector enclosing acc loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:acc loop gang worker vector enclosing acc loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop gang worker vector enclosing acc loop: 1, 1
        TGT_PRINTF("acc loop gang worker vector enclosing acc loop: %d, %d\n", i, j);

    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCAutoClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCGangClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   impl: ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto gang
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto gang // discarded in OpenMP translation{{$}}
    #pragma acc loop auto gang
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      //          DMP: ACCLoopDirective
      //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
      // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
      //   DMP-G-NEXT:   impl: OMPDistributeDirective
      // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
      // DMP-GWV-NEXT:     OMPNum_threadsClause
      // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
      // DMP-GWV-NEXT:     OMPSimdlenClause
      // DMP-GWV-NEXT:       ConstantExpr
      // DMP-GWV-NEXT:         value: Int 8
      // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
      //      DMP-NOT:     OMP
      //
      //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
      //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
      //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
      #pragma acc loop
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG:acc loop auto gang enclosing acc loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop auto gang enclosing acc loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:acc loop auto gang enclosing acc loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop auto gang enclosing acc loop: 1, 1
        TGT_PRINTF("acc loop auto gang enclosing acc loop: %d, %d\n", i, j);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  //............................................................................
  // Repeat for acc parallel loop.

  // DMP-LABEL:"acc parallel loop clause combinations enclosing acc loop\n"
  // PRT-LABEL:"acc parallel loop clause combinations enclosing acc loop\n"
  // EXE-LABEL:acc parallel loop clause combinations enclosing acc loop
  printf("acc parallel loop clause combinations enclosing acc loop\n");

  //          DMP: ACCParallelLoopDirective
  //     DMP-NEXT:   ACCNumGangsClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:   ACCNumWorkersClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:   ACCVectorLengthClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     ACCNumWorkersClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:     ACCVectorLengthClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCWorkerClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:       impl: OMPDistributeDirective
  // DMP-GWV-NEXT:       impl: OMPDistributeParallelForSimdDirective
  // DMP-GWV-NEXT:         OMPNum_threadsClause
  // DMP-GWV-NEXT:           IntegerLiteral {{.*}} 4
  // DMP-GWV-NEXT:         OMPSimdlenClause
  // DMP-GWV-NEXT:           ConstantExpr
  // DMP-GWV-NEXT:             value: Int 8
  // DMP-GWV-NEXT:             IntegerLiteral {{.*}} 'int' 8
  //      DMP-NOT:         OMP
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  //     PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //      PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8)
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop enclosing acc loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop enclosing acc loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop enclosing acc loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop enclosing acc loop: 1, 1
      TGT_PRINTF("acc parallel loop enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL:"acc parallel loop gang enclosing acc loop\n"
  // PRT-LABEL:"acc parallel loop gang enclosing acc loop\n"
  // EXE-LABEL:acc parallel loop gang enclosing acc loop
  printf("acc parallel loop gang enclosing acc loop\n");

  //          DMP: ACCParallelLoopDirective
  //     DMP-NEXT:   ACCNumGangsClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:   ACCNumWorkersClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:   ACCVectorLengthClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:   ACCGangClause
  //      DMP-NOT:     <implicit>
  //     DMP-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     ACCNumWorkersClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:     ACCVectorLengthClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCGangClause
  //      DMP-NOT:         <implicit>
  //     DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCWorkerClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:       impl: OMPDistributeDirective
  // DMP-GWV-NEXT:       impl: OMPDistributeParallelForSimdDirective
  // DMP-GWV-NEXT:         OMPNum_threadsClause
  // DMP-GWV-NEXT:           IntegerLiteral {{.*}} 4
  // DMP-GWV-NEXT:         OMPSimdlenClause
  // DMP-GWV-NEXT:           ConstantExpr
  // DMP-GWV-NEXT:             value: Int 8
  // DMP-GWV-NEXT:             IntegerLiteral {{.*}} 'int' 8
  //      DMP-NOT:         OMP
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang{{$}}
  //     PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //      PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang{{$}}
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang enclosing acc loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang enclosing acc loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang enclosing acc loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang enclosing acc loop: 1, 1
      TGT_PRINTF("acc parallel loop gang enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL:"acc parallel loop worker enclosing acc loop\n"
  // PRT-LABEL:"acc parallel loop worker enclosing acc loop\n"
  // EXE-LABEL:acc parallel loop worker enclosing acc loop
  printf("acc parallel loop worker enclosing acc loop\n");

  //          DMP: ACCParallelLoopDirective
  //     DMP-NEXT:   ACCNumGangsClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:   ACCNumWorkersClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:   ACCVectorLengthClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:   ACCWorkerClause
  //      DMP-NOT:     <implicit>
  //     DMP-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     ACCNumWorkersClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:     ACCVectorLengthClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCWorkerClause
  //      DMP-NOT:         <implicit>
  //     DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:       impl: OMPDistributeParallelForDirective
  // DMP-GWV-NEXT:       impl: OMPDistributeParallelForSimdDirective
  //     DMP-NEXT:         OMPNum_threadsClause
  //     DMP-NEXT:           IntegerLiteral {{.*}} 'int' 4
  // DMP-GWV-NEXT:         OMPSimdlenClause
  // DMP-GWV-NEXT:           ConstantExpr
  // DMP-GWV-NEXT:             value: Int 8
  // DMP-GWV-NEXT:             IntegerLiteral {{.*}} 'int' 8
  //      DMP-NOT:         OMP
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) worker{{$}}
  //     PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(4){{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //      PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute parallel for num_threads(4){{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) worker{{$}}
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) worker
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop worker enclosing acc loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop worker enclosing acc loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop worker enclosing acc loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop worker enclosing acc loop: 1, 1
      TGT_PRINTF("acc parallel loop worker enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL:"acc parallel loop vector enclosing acc loop\n"
  // PRT-LABEL:"acc parallel loop vector enclosing acc loop\n"
  // EXE-LABEL:acc parallel loop vector enclosing acc loop
  printf("acc parallel loop vector enclosing acc loop\n");

  //          DMP: ACCParallelLoopDirective
  //     DMP-NEXT:   ACCNumGangsClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:   ACCNumWorkersClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:   ACCVectorLengthClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:   ACCVectorClause
  //      DMP-NOT:     <implicit>
  //     DMP-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     ACCNumWorkersClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:     ACCVectorLengthClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCVectorClause
  //      DMP-NOT:         <implicit>
  //     DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCWorkerClause {{.*}} <implicit>
  //   DMP-G-NEXT:       impl: OMPDistributeSimdDirective
  // DMP-GWV-NEXT:       impl: OMPDistributeParallelForSimdDirective
  // DMP-GWV-NEXT:         OMPNum_threadsClause
  // DMP-GWV-NEXT:           IntegerLiteral {{.*}} 4
  //     DMP-NEXT:         OMPSimdlenClause
  //     DMP-NEXT:           ConstantExpr
  //     DMP-NEXT:             value: Int 8
  //     DMP-NEXT:             IntegerLiteral {{.*}} 'int' 8
  //      DMP-NOT:         OMP
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) vector{{$}}
  //     PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute simd simdlen(8){{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //      PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute simd simdlen(8){{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) vector{{$}}
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) vector
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    // DMP-NOT:  OMP
    // DMP:      ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    // DMP-NOT:      OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
    //
    // PRT-NEXT:    for ({{.*}})
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop vector enclosing acc loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop vector enclosing acc loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop vector enclosing acc loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop vector enclosing acc loop: 1, 1
      TGT_PRINTF("acc parallel loop vector enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL:"acc parallel loop seq enclosing acc loop\n"
  // PRT-LABEL:"acc parallel loop seq enclosing acc loop\n"
  // EXE-LABEL:acc parallel loop seq enclosing acc loop
  printf("acc parallel loop seq enclosing acc loop\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   ACCSeqClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCNumWorkersClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCSeqClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) seq{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) seq{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) seq
  for (int i = 0; i < 2; ++i) {
    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-GWV-NEXT:     OMPNum_threadsClause
    // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
    // DMP-GWV-NEXT:     OMPSimdlenClause
    // DMP-GWV-NEXT:       ConstantExpr
    // DMP-GWV-NEXT:         value: Int 8
    // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //      DMP-NOT:     OMP
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    //        PRT-NEXT: for ({{.*}})
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop seq enclosing acc loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop seq enclosing acc loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop seq enclosing acc loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop seq enclosing acc loop: 1, 1
      TGT_PRINTF("acc parallel loop seq enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL:"acc parallel loop auto enclosing acc loop\n"
  // PRT-LABEL:"acc parallel loop auto enclosing acc loop\n"
  // EXE-LABEL:acc parallel loop auto enclosing acc loop
  printf("acc parallel loop auto enclosing acc loop\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCNumWorkersClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCAutoClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) auto{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) auto{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) auto
  for (int i = 0; i < 2; ++i) {
    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-GWV-NEXT:     OMPNum_threadsClause
    // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
    // DMP-GWV-NEXT:     OMPSimdlenClause
    // DMP-GWV-NEXT:       ConstantExpr
    // DMP-GWV-NEXT:         value: Int 8
    // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //      DMP-NOT:     OMP
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    //        PRT-NEXT: for ({{.*}})
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop auto enclosing acc loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop auto enclosing acc loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop auto enclosing acc loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop auto enclosing acc loop: 1, 1
      TGT_PRINTF("acc parallel loop auto enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL:"acc parallel loop gang worker enclosing acc loop\n"
  // PRT-LABEL:"acc parallel loop gang worker enclosing acc loop\n"
  // EXE-LABEL:acc parallel loop gang worker enclosing acc loop
  printf("acc parallel loop gang worker enclosing acc loop\n");

  //          DMP: ACCParallelLoopDirective
  //     DMP-NEXT:   ACCNumGangsClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:   ACCNumWorkersClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:   ACCVectorLengthClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:   ACCGangClause
  //      DMP-NOT:     <implicit>
  //     DMP-NEXT:   ACCWorkerClause
  //      DMP-NOT:     <implicit>
  //     DMP-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     ACCNumWorkersClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:     ACCVectorLengthClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCGangClause
  //      DMP-NOT:         <implicit>
  //     DMP-NEXT:       ACCWorkerClause
  //      DMP-NOT:         <implicit>
  //     DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:       impl: OMPDistributeParallelForDirective
  // DMP-GWV-NEXT:       impl: OMPDistributeParallelForSimdDirective
  //     DMP-NEXT:         OMPNum_threadsClause
  //     DMP-NEXT:           IntegerLiteral {{.*}} 'int' 4
  // DMP-GWV-NEXT:         OMPSimdlenClause
  // DMP-GWV-NEXT:           ConstantExpr
  // DMP-GWV-NEXT:             value: Int 8
  // DMP-GWV-NEXT:             IntegerLiteral {{.*}} 'int' 8
  //      DMP-NOT:         OMP
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang worker{{$}}
  //     PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(4){{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //      PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute parallel for num_threads(4){{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang worker{{$}}
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang worker
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang worker enclosing acc loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang worker enclosing acc loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang worker enclosing acc loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang worker enclosing acc loop: 1, 1
      TGT_PRINTF("acc parallel loop gang worker enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL:"acc parallel loop gang vector enclosing acc loop\n"
  // PRT-LABEL:"acc parallel loop gang vector enclosing acc loop\n"
  // EXE-LABEL:acc parallel loop gang vector enclosing acc loop
  printf("acc parallel loop gang vector enclosing acc loop\n");

  //          DMP: ACCParallelLoopDirective
  //     DMP-NEXT:   ACCNumGangsClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:   ACCNumWorkersClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:   ACCVectorLengthClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:   ACCGangClause
  //      DMP-NOT:     <implicit>
  //     DMP-NEXT:   ACCVectorClause
  //      DMP-NOT:     <implicit>
  //     DMP-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     ACCNumWorkersClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:     ACCVectorLengthClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCGangClause
  //      DMP-NOT:         <implicit>
  //     DMP-NEXT:       ACCVectorClause
  //      DMP-NOT:         <implicit>
  //     DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCWorkerClause {{.*}} <implicit>
  //   DMP-G-NEXT:       impl: OMPDistributeSimdDirective
  // DMP-GWV-NEXT:       impl: OMPDistributeParallelForSimdDirective
  // DMP-GWV-NEXT:         OMPNum_threadsClause
  // DMP-GWV-NEXT:           IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:         OMPSimdlenClause
  //     DMP-NEXT:           ConstantExpr
  //     DMP-NEXT:             value: Int 8
  //     DMP-NEXT:             IntegerLiteral {{.*}} 'int' 8
  //      DMP-NOT:         OMP
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang vector{{$}}
  //     PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute simd simdlen(8){{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //      PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute simd simdlen(8){{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang vector{{$}}
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang vector
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang vector enclosing acc loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang vector enclosing acc loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang vector enclosing acc loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang vector enclosing acc loop: 1, 1
      TGT_PRINTF("acc parallel loop gang vector enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL:"acc parallel loop worker vector enclosing acc loop\n"
  // PRT-LABEL:"acc parallel loop worker vector enclosing acc loop\n"
  // EXE-LABEL:acc parallel loop worker vector enclosing acc loop
  printf("acc parallel loop worker vector enclosing acc loop\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCNumWorkersClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCWorkerClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCVectorClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeParallelForSimdDirective
  // DMP-NEXT:         OMPNum_threadsClause
  // DMP-NEXT:           IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:         OMPSimdlenClause
  // DMP-NEXT:           ConstantExpr
  // DMP-NEXT:             value: Int 8
  // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 8
  //  DMP-NOT:         OMP
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) worker vector{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) worker vector{{$}}
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) worker vector
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop worker vector enclosing acc loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop worker vector enclosing acc loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop worker vector enclosing acc loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop worker vector enclosing acc loop: 1, 1
      TGT_PRINTF("acc parallel loop worker vector enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL:"acc parallel loop gang worker vector enclosing acc loop\n"
  // PRT-LABEL:"acc parallel loop gang worker vector enclosing acc loop\n"
  // EXE-LABEL:acc parallel loop gang worker vector enclosing acc loop
  printf("acc parallel loop gang worker vector enclosing acc loop\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   ACCGangClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCNumWorkersClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCGangClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCWorkerClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCVectorClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeParallelForSimdDirective
  // DMP-NEXT:         OMPNum_threadsClause
  // DMP-NEXT:           IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:         OMPSimdlenClause
  // DMP-NEXT:           ConstantExpr
  // DMP-NEXT:             value: Int 8
  // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 8
  //  DMP-NOT:         OMP
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang worker vector{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang worker vector{{$}}
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang worker vector
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang worker vector enclosing acc loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang worker vector enclosing acc loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang worker vector enclosing acc loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang worker vector enclosing acc loop: 1, 1
      TGT_PRINTF("acc parallel loop gang worker vector enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL:"acc parallel loop gang auto enclosing acc loop\n"
  // PRT-LABEL:"acc parallel loop gang auto enclosing acc loop\n"
  // EXE-LABEL:acc parallel loop gang auto enclosing acc loop
  printf("acc parallel loop gang auto enclosing acc loop\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   ACCGangClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCNumWorkersClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCGangClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCAutoClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang auto{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang auto{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8) gang auto
  for (int i = 0; i < 2; ++i) {
    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-GWV-NEXT:     OMPNum_threadsClause
    // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
    // DMP-GWV-NEXT:     OMPSimdlenClause
    // DMP-GWV-NEXT:       ConstantExpr
    // DMP-GWV-NEXT:         value: Int 8
    // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //      DMP-NOT:     OMP
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    //        PRT-NEXT: for ({{.*}})
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang auto enclosing acc loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang auto enclosing acc loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang auto enclosing acc loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:acc parallel loop gang auto enclosing acc loop: 1, 1
      TGT_PRINTF("acc parallel loop gang auto enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // gang, worker, vector, seq, auto combinations on enclosed acc loop.
  //----------------------------------------------------------------------------

  //............................................................................
  // First for acc parallel and acc loop separately.

  // Above, we already covered the case of acc loop enclosed by acc loop.

  // DMP-LABEL:"acc loop clause combinations enclosed by acc loop\n"
  // PRT-LABEL:"acc loop clause combinations enclosed by acc loop\n"
  // EXE-LABEL:acc loop clause combinations enclosed by acc loop
  printf("acc loop clause combinations enclosed by acc loop\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //  DMP-NOT:     OMP
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8)
  // DMP: CompoundStmt
  // PRT-NEXT: {
  {
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
    //
    // PRT-NEXT:    for ({{.*}}) {
    #pragma acc loop
    for (int i = 0; i < 2; ++i) {
      //          DMP: ACCLoopDirective
      //     DMP-NEXT:   ACCGangClause
      //      DMP-NOT:     <implicit>
      //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
      // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
      //   DMP-G-NEXT:   impl: OMPDistributeDirective
      // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
      // DMP-GWV-NEXT:     OMPNum_threadsClause
      // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
      // DMP-GWV-NEXT:     OMPSimdlenClause
      // DMP-GWV-NEXT:       ConstantExpr
      // DMP-GWV-NEXT:         value: Int 8
      // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
      //      DMP-NOT:     OMP
      //
      //      PRT-A-NEXT: {{^ *}}#pragma acc loop gang{{$}}
      //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
      //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
      #pragma acc loop gang
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG:acc loop gang enclosed by acc loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop gang enclosed by acc loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:acc loop gang enclosed by acc loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop gang enclosed by acc loop: 1, 1
        TGT_PRINTF("acc loop gang enclosed by acc loop: %d, %d\n", i, j);
    } // PRT-NEXT: }

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    #pragma acc loop
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      //          DMP: ACCLoopDirective
      //     DMP-NEXT:   ACCWorkerClause
      //      DMP-NOT:     <implicit>
      //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
      //   DMP-G-NEXT:   impl: OMPParallelForDirective
      // DMP-GWV-NEXT:   impl: OMPParallelForSimdDirective
      //     DMP-NEXT:     OMPNum_threadsClause
      //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
      // DMP-GWV-NEXT:     OMPSimdlenClause
      // DMP-GWV-NEXT:       ConstantExpr
      // DMP-GWV-NEXT:         value: Int 8
      // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
      //      DMP-NOT:     OMP
      //
      //      PRT-A-NEXT: {{^ *}}#pragma acc loop worker{{$}}
      //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp parallel for num_threads(4){{$}}
      // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp parallel for simd num_threads(4) simdlen(8){{$}}
      //    PRT-O-G-NEXT: {{^ *}}#pragma omp parallel for num_threads(4){{$}}
      //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp parallel for simd num_threads(4) simdlen(8){{$}}
      //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker{{$}}
      #pragma acc loop worker
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG:acc loop worker enclosed by acc loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop worker enclosed by acc loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:acc loop worker enclosed by acc loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop worker enclosed by acc loop: 1, 1
        TGT_PRINTF("acc loop worker enclosed by acc loop: %d, %d\n", i, j);
    } // PRT-NEXT: }

    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForDirective
    // DMP-GWV-NEXT:     OMPNum_threadsClause
    // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
    //      DMP-NOT:     OMP
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(4){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for num_threads(4){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    #pragma acc loop
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCVectorClause
      //  DMP-NOT:     <implicit>
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: OMPSimdDirective
      // DMP-NEXT:     OMPSimdlenClause
      // DMP-NEXT:       ConstantExpr
      // DMP-NEXT:         value: Int 8
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 8
      //  DMP-NOT:     OMP
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp simd simdlen(8){{$}}
      //  PRT-O-NEXT: {{^ *}}#pragma omp simd simdlen(8){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector{{$}}
      #pragma acc loop vector
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG:acc loop vector enclosed by acc loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop vector enclosed by acc loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:acc loop vector enclosed by acc loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop vector enclosed by acc loop: 1, 1
        TGT_PRINTF("acc loop vector enclosed by acc loop: %d, %d\n", i, j);
    } // PRT-NEXT: }

    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-GWV-NEXT:     OMPNum_threadsClause
    // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
    // DMP-GWV-NEXT:     OMPSimdlenClause
    // DMP-GWV-NEXT:       ConstantExpr
    // DMP-GWV-NEXT:         value: Int 8
    // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //      DMP-NOT:     OMP
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    #pragma acc loop
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      //  DMP-NOT: OMP
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCSeqClause
      //  DMP-NOT:     <implicit>
      // DMP-NEXT:   ACCSharedClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: ForStmt
      //  DMP-NOT:     OMP
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      //  PRT-A-SAME: {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq // discarded in OpenMP translation{{$}}
      //
      // PRT-NEXT: for ({{.*}})
      #pragma acc loop seq
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG:acc loop seq enclosed by acc loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop seq enclosed by acc loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:acc loop seq enclosed by acc loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop seq enclosed by acc loop: 1, 1
        TGT_PRINTF("acc loop seq enclosed by acc loop: %d, %d\n", i, j);
    } // PRT-NEXT: }

    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-GWV-NEXT:     OMPNum_threadsClause
    // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
    // DMP-GWV-NEXT:     OMPSimdlenClause
    // DMP-GWV-NEXT:       ConstantExpr
    // DMP-GWV-NEXT:         value: Int 8
    // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //      DMP-NOT:     OMP
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    #pragma acc loop
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      //  DMP-NOT: OMP
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCAutoClause
      //  DMP-NOT:     <implicit>
      // DMP-NEXT:   ACCSharedClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: ForStmt
      //  DMP-NOT:     OMP
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      //  PRT-A-SAME: {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto // discarded in OpenMP translation{{$}}
      //    PRT-NEXT: for ({{.*}})
      #pragma acc loop auto
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG:acc loop auto enclosed by acc loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop auto enclosed by acc loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:acc loop auto enclosed by acc loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop auto enclosed by acc loop: 1, 1
        TGT_PRINTF("acc loop auto enclosed by acc loop: %d, %d\n", i, j);
    } // PRT-NEXT: }

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
    //
    // PRT-NEXT:    for ({{.*}}) {
    #pragma acc loop
    for (int i = 0; i < 2; ++i) {
      //          DMP: ACCLoopDirective
      //     DMP-NEXT:   ACCGangClause
      //      DMP-NOT:     <implicit>
      //     DMP-NEXT:   ACCWorkerClause
      //      DMP-NOT:     <implicit>
      //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
      //   DMP-G-NEXT:   impl: OMPDistributeParallelForDirective
      // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
      //     DMP-NEXT:     OMPNum_threadsClause
      //     DMP-NEXT:       IntegerLiteral {{.*}} 4
      // DMP-GWV-NEXT:     OMPSimdlenClause
      // DMP-GWV-NEXT:       ConstantExpr
      // DMP-GWV-NEXT:         value: Int 8
      // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
      //      DMP-NOT:     OMP
      //
      //      PRT-A-NEXT: {{^ *}}#pragma acc loop gang worker{{$}}
      //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(4){{$}}
      // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute parallel for num_threads(4){{$}}
      //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang worker{{$}}
      #pragma acc loop gang worker
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG:acc loop gang worker enclosed by acc loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop gang worker enclosed by acc loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:acc loop gang worker enclosed by acc loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop gang worker enclosed by acc loop: 1, 1
        TGT_PRINTF("acc loop gang worker enclosed by acc loop: %d, %d\n", i, j);
    } // PRT-NEXT: }

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
    //
    // PRT-NEXT:    for ({{.*}}) {
    #pragma acc loop
    for (int i = 0; i < 2; ++i) {
      //          DMP: ACCLoopDirective
      //     DMP-NEXT:   ACCGangClause
      //      DMP-NOT:     <implicit>
      //     DMP-NEXT:   ACCVectorClause
      //      DMP-NOT:     <implicit>
      //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
      //   DMP-G-NEXT:   impl: OMPDistributeSimdDirective
      // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
      // DMP-GWV-NEXT:     OMPNum_threadsClause
      // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
      //     DMP-NEXT:     OMPSimdlenClause
      //     DMP-NEXT:       ConstantExpr
      //     DMP-NEXT:         value: Int 8
      //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 8
      //      DMP-NOT:     OMP
      //
      //      PRT-A-NEXT: {{^ *}}#pragma acc loop gang vector{{$}}
      //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute simd simdlen(8){{$}}
      // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute simd simdlen(8){{$}}
      //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang vector{{$}}
      #pragma acc loop gang vector
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG:acc loop gang vector enclosed by acc loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop gang vector enclosed by acc loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:acc loop gang vector enclosed by acc loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop gang vector enclosed by acc loop: 1, 1
        TGT_PRINTF("acc loop gang vector enclosed by acc loop: %d, %d\n", i, j);
    } // PRT-NEXT: }

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    #pragma acc loop
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCWorkerClause
      //  DMP-NOT:     <implicit>
      // DMP-NEXT:   ACCVectorClause
      //  DMP-NOT:     <implicit>
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: OMPParallelForSimdDirective
      // DMP-NEXT:     OMPNum_threadsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
      // DMP-NEXT:     OMPSimdlenClause
      // DMP-NEXT:       ConstantExpr
      // DMP-NEXT:         value: Int 8
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 8
      //  DMP-NOT:     OMP
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for simd num_threads(4) simdlen(8){{$}}
      //  PRT-O-NEXT: {{^ *}}#pragma omp parallel for simd num_threads(4) simdlen(8){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector{{$}}
      #pragma acc loop worker vector
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG:acc loop worker vector enclosed by acc loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop worker vector enclosed by acc loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:acc loop worker vector enclosed by acc loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop worker vector enclosed by acc loop: 1, 1
        TGT_PRINTF("acc loop worker vector enclosed by acc loop: %d, %d\n", i, j);
    } // PRT-NEXT: }

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
    //
    // PRT-NEXT:    for ({{.*}}) {
    #pragma acc loop
    for (int i = 0; i < 2; ++i) {
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      //  DMP-NOT:     <implicit>
      // DMP-NEXT:   ACCWorkerClause
      //  DMP-NOT:     <implicit>
      // DMP-NEXT:   ACCVectorClause
      //  DMP-NOT:     <implicit>
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
      // DMP-NEXT:     OMPNum_threadsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 4
      // DMP-NEXT:     OMPSimdlenClause
      // DMP-NEXT:       ConstantExpr
      // DMP-NEXT:         value: Int 8
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 8
      //  DMP-NOT:     OMP
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang worker vector{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang worker vector{{$}}
      #pragma acc loop gang worker vector
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG:acc loop gang worker vector enclosed by acc loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop gang worker vector enclosed by acc loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:acc loop gang worker vector enclosed by acc loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop gang worker vector enclosed by acc loop: 1, 1
        TGT_PRINTF("acc loop gang worker vector enclosed by acc loop: %d, %d\n", i, j);
    } // PRT-NEXT: }

    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-GWV-NEXT:     OMPNum_threadsClause
    // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
    // DMP-GWV-NEXT:     OMPSimdlenClause
    // DMP-GWV-NEXT:       ConstantExpr
    // DMP-GWV-NEXT:         value: Int 8
    // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //      DMP-NOT:     OMP
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    #pragma acc loop
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      //  DMP-NOT: OMP
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      //  DMP-NOT:     <implicit>
      // DMP-NEXT:   ACCAutoClause
      //  DMP-NOT:     <implicit>
      // DMP-NEXT:   ACCSharedClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: ForStmt
      //  DMP-NOT:     OMP
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang auto
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      //  PRT-A-SAME: {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang auto // discarded in OpenMP translation{{$}}
      //    PRT-NEXT: for ({{.*}})
      #pragma acc loop gang auto
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG:acc loop gang auto enclosed by acc loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop gang auto enclosed by acc loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:acc loop gang auto enclosed by acc loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:acc loop gang auto enclosed by acc loop: 1, 1
        TGT_PRINTF("acc loop gang auto enclosed by acc loop: %d, %d\n", i, j);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  //............................................................................
  // Repeat for acc parallel loop.

  // Above, we already covered the case of acc loop enclosed by acc parallel
  // loop.

  // DMP-LABEL:"acc loop gang enclosed by acc parallel loop\n"
  // PRT-LABEL:"acc loop gang enclosed by acc parallel loop\n"
  // EXE-LABEL:acc loop gang enclosed by acc parallel loop
  printf("acc loop gang enclosed by acc parallel loop\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCNumWorkersClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  //    PRT-NEXT: for ({{.*}}) {
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8)
  for (int i = 0; i < 2; ++i) {
    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCGangClause
    //      DMP-NOT:     <implicit>
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-GWV-NEXT:     OMPNum_threadsClause
    // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
    // DMP-GWV-NEXT:     OMPSimdlenClause
    // DMP-GWV-NEXT:       ConstantExpr
    // DMP-GWV-NEXT:         value: Int 8
    // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //      DMP-NOT:     OMP
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop gang{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
    #pragma acc loop gang
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:acc loop gang enclosed by acc parallel loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:acc loop gang enclosed by acc parallel loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:acc loop gang enclosed by acc parallel loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:acc loop gang enclosed by acc parallel loop: 1, 1
      TGT_PRINTF("acc loop gang enclosed by acc parallel loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL:"acc loop worker enclosed by acc parallel loop\n"
  // PRT-LABEL:"acc loop worker enclosed by acc parallel loop\n"
  // EXE-LABEL:acc loop worker enclosed by acc parallel loop
  printf("acc loop worker enclosed by acc parallel loop\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCNumWorkersClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeDirective
  //  DMP-NOT:         OMP
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8)
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCWorkerClause
    //      DMP-NOT:     <implicit>
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPParallelForDirective
    // DMP-GWV-NEXT:   impl: OMPParallelForSimdDirective
    //     DMP-NEXT:     OMPNum_threadsClause
    //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
    // DMP-GWV-NEXT:     OMPSimdlenClause
    // DMP-GWV-NEXT:       ConstantExpr
    // DMP-GWV-NEXT:         value: Int 8
    // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //      DMP-NOT:     OMP
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop worker{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp parallel for num_threads(4){{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp parallel for simd num_threads(4) simdlen(8){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp parallel for num_threads(4){{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp parallel for simd num_threads(4) simdlen(8){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker{{$}}
    #pragma acc loop worker
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:acc loop worker enclosed by acc parallel loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:acc loop worker enclosed by acc parallel loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:acc loop worker enclosed by acc parallel loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:acc loop worker enclosed by acc parallel loop: 1, 1
      TGT_PRINTF("acc loop worker enclosed by acc parallel loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL:"acc loop vector enclosed by acc parallel loop\n"
  // PRT-LABEL:"acc loop vector enclosed by acc parallel loop\n"
  // EXE-LABEL:acc loop vector enclosed by acc parallel loop
  printf("acc loop vector enclosed by acc parallel loop\n");

  //          DMP: ACCParallelLoopDirective
  //     DMP-NEXT:   ACCNumGangsClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:   ACCNumWorkersClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:   ACCVectorLengthClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     ACCNumWorkersClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:     ACCVectorLengthClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCWorkerClause {{.*}} <implicit>
  //   DMP-G-NEXT:       impl: OMPDistributeDirective
  // DMP-GWV-NEXT:       impl: OMPDistributeParallelForDirective
  // DMP-GWV-NEXT:         OMPNum_threadsClause
  // DMP-GWV-NEXT:           IntegerLiteral {{.*}} 4
  //      DMP-NOT:         OMP
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  //     PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(4){{$}}
  //      PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for num_threads(4){{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8)
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCVectorClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: OMPSimdDirective
    // DMP-NEXT:     OMPSimdlenClause
    // DMP-NEXT:       ConstantExpr
    // DMP-NEXT:         value: Int 8
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp simd simdlen(8){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp simd simdlen(8){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector{{$}}
    #pragma acc loop vector
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:acc loop vector enclosed by acc parallel loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:acc loop vector enclosed by acc parallel loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:acc loop vector enclosed by acc parallel loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:acc loop vector enclosed by acc parallel loop: 1, 1
      TGT_PRINTF("acc loop vector enclosed by acc parallel loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL:"acc loop seq enclosed by acc parallel loop\n"
  // PRT-LABEL:"acc loop seq enclosed by acc parallel loop\n"
  // EXE-LABEL:acc loop seq enclosed by acc parallel loop
  printf("acc loop seq enclosed by acc parallel loop\n");

  //          DMP: ACCParallelLoopDirective
  //     DMP-NEXT:   ACCNumGangsClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:   ACCNumWorkersClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:   ACCVectorLengthClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     ACCNumWorkersClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:     ACCVectorLengthClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCWorkerClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:       impl: OMPDistributeDirective
  // DMP-GWV-NEXT:       impl: OMPDistributeParallelForSimdDirective
  // DMP-GWV-NEXT:         OMPNum_threadsClause
  // DMP-GWV-NEXT:           IntegerLiteral {{.*}} 4
  // DMP-GWV-NEXT:         OMPSimdlenClause
  // DMP-GWV-NEXT:           ConstantExpr
  // DMP-GWV-NEXT:             value: Int 8
  // DMP-GWV-NEXT:             IntegerLiteral {{.*}} 'int' 8
  //      DMP-NOT:         OMP
  //          DMP:         ForStmt
  //          DMP:           CallExpr
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  //     PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //      PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8)
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCSharedClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    #pragma acc loop seq
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:acc loop seq enclosed by acc parallel loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:acc loop seq enclosed by acc parallel loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:acc loop seq enclosed by acc parallel loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:acc loop seq enclosed by acc parallel loop: 1, 1
      TGT_PRINTF("acc loop seq enclosed by acc parallel loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL:"acc loop auto enclosed by acc parallel loop\n"
  // PRT-LABEL:"acc loop auto enclosed by acc parallel loop\n"
  // EXE-LABEL:acc loop auto enclosed by acc parallel loop
  printf("acc loop auto enclosed by acc parallel loop\n");

  //          DMP: ACCParallelLoopDirective
  //     DMP-NEXT:   ACCNumGangsClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:   ACCNumWorkersClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:   ACCVectorLengthClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     ACCNumWorkersClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:     ACCVectorLengthClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCWorkerClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:       impl: OMPDistributeDirective
  // DMP-GWV-NEXT:       impl: OMPDistributeParallelForSimdDirective
  // DMP-GWV-NEXT:         OMPNum_threadsClause
  // DMP-GWV-NEXT:           IntegerLiteral {{.*}} 4
  // DMP-GWV-NEXT:         OMPSimdlenClause
  // DMP-GWV-NEXT:           ConstantExpr
  // DMP-GWV-NEXT:             value: Int 8
  // DMP-GWV-NEXT:             IntegerLiteral {{.*}} 'int' 8
  //      DMP-NOT:         OMP
  //          DMP:         ForStmt
  //          DMP:           CallExpr
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  //     PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //      PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8)
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCAutoClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCSharedClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    #pragma acc loop auto
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:acc loop auto enclosed by acc parallel loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:acc loop auto enclosed by acc parallel loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:acc loop auto enclosed by acc parallel loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:acc loop auto enclosed by acc parallel loop: 1, 1
      TGT_PRINTF("acc loop auto enclosed by acc parallel loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL:"acc loop gang worker enclosed by acc parallel loop\n"
  // PRT-LABEL:"acc loop gang worker enclosed by acc parallel loop\n"
  // EXE-LABEL:acc loop gang worker enclosed by acc parallel loop
  printf("acc loop gang worker enclosed by acc parallel loop\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCNumWorkersClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  //    PRT-NEXT: for ({{.*}}) {
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8)
  for (int i = 0; i < 2; ++i) {
    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCGangClause
    //      DMP-NOT:     <implicit>
    //     DMP-NEXT:   ACCWorkerClause
    //      DMP-NOT:     <implicit>
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeParallelForDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    //     DMP-NEXT:     OMPNum_threadsClause
    //     DMP-NEXT:       IntegerLiteral {{.*}} 4
    // DMP-GWV-NEXT:     OMPSimdlenClause
    // DMP-GWV-NEXT:       ConstantExpr
    // DMP-GWV-NEXT:         value: Int 8
    // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //      DMP-NOT:     OMP
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop gang worker{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(4){{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute parallel for num_threads(4){{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang worker{{$}}
    #pragma acc loop gang worker
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:acc loop gang worker enclosed by acc parallel loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:acc loop gang worker enclosed by acc parallel loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:acc loop gang worker enclosed by acc parallel loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:acc loop gang worker enclosed by acc parallel loop: 1, 1
      TGT_PRINTF("acc loop gang worker enclosed by acc parallel loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL:"acc loop gang vector enclosed by acc parallel loop\n"
  // PRT-LABEL:"acc loop gang vector enclosed by acc parallel loop\n"
  // EXE-LABEL:acc loop gang vector enclosed by acc parallel loop
  printf("acc loop gang vector enclosed by acc parallel loop\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCNumWorkersClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  //    PRT-NEXT: for ({{.*}}) {
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8)
  for (int i = 0; i < 2; ++i) {
    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCGangClause
    //      DMP-NOT:     <implicit>
    //     DMP-NEXT:   ACCVectorClause
    //      DMP-NOT:     <implicit>
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeSimdDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-GWV-NEXT:     OMPNum_threadsClause
    // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
    //     DMP-NEXT:     OMPSimdlenClause
    //     DMP-NEXT:       ConstantExpr
    //     DMP-NEXT:         value: Int 8
    //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //      DMP-NOT:     OMP
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop gang vector{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute simd simdlen(8){{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute simd simdlen(8){{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang vector{{$}}
    #pragma acc loop gang vector
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:acc loop gang vector enclosed by acc parallel loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:acc loop gang vector enclosed by acc parallel loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:acc loop gang vector enclosed by acc parallel loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:acc loop gang vector enclosed by acc parallel loop: 1, 1
      TGT_PRINTF("acc loop gang vector enclosed by acc parallel loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL:"acc loop worker vector enclosed by acc parallel loop\n"
  // PRT-LABEL:"acc loop worker vector enclosed by acc parallel loop\n"
  // EXE-LABEL:acc loop worker vector enclosed by acc parallel loop
  printf("acc loop worker vector enclosed by acc parallel loop\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCNumWorkersClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeDirective
  //  DMP-NOT:         OMP
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8)
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCVectorClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: OMPParallelForSimdDirective
    // DMP-NEXT:     OMPNum_threadsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:     OMPSimdlenClause
    // DMP-NEXT:       ConstantExpr
    // DMP-NEXT:         value: Int 8
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for simd num_threads(4) simdlen(8){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp parallel for simd num_threads(4) simdlen(8){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector{{$}}
    #pragma acc loop worker vector
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:acc loop worker vector enclosed by acc parallel loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:acc loop worker vector enclosed by acc parallel loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:acc loop worker vector enclosed by acc parallel loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:acc loop worker vector enclosed by acc parallel loop: 1, 1
      TGT_PRINTF("acc loop worker vector enclosed by acc parallel loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL:"acc loop gang worker vector enclosed by acc parallel loop\n"
  // PRT-LABEL:"acc loop gang worker vector enclosed by acc parallel loop\n"
  // EXE-LABEL:acc loop gang worker vector enclosed by acc parallel loop
  printf("acc loop gang worker vector enclosed by acc parallel loop\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCNumWorkersClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  //    PRT-NEXT: for ({{.*}}) {
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8)
  for (int i = 0; i < 2; ++i) {
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCWorkerClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCVectorClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-NEXT:     OMPNum_threadsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 4
    // DMP-NEXT:     OMPSimdlenClause
    // DMP-NEXT:       ConstantExpr
    // DMP-NEXT:         value: Int 8
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang worker vector{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang worker vector{{$}}
    #pragma acc loop gang worker vector
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:acc loop gang worker vector enclosed by acc parallel loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:acc loop gang worker vector enclosed by acc parallel loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:acc loop gang worker vector enclosed by acc parallel loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:acc loop gang worker vector enclosed by acc parallel loop: 1, 1
      TGT_PRINTF("acc loop gang worker vector enclosed by acc parallel loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL:"acc loop gang auto enclosed by acc parallel loop\n"
  // PRT-LABEL:"acc loop gang auto enclosed by acc parallel loop\n"
  // EXE-LABEL:acc loop gang auto enclosed by acc parallel loop
  printf("acc loop gang auto enclosed by acc parallel loop\n");

  //          DMP: ACCParallelLoopDirective
  //     DMP-NEXT:   ACCNumGangsClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:   ACCNumWorkersClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:   ACCVectorLengthClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     ACCNumWorkersClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:     ACCVectorLengthClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCWorkerClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:       impl: OMPDistributeDirective
  // DMP-GWV-NEXT:       impl: OMPDistributeParallelForSimdDirective
  // DMP-GWV-NEXT:         OMPNum_threadsClause
  // DMP-GWV-NEXT:           IntegerLiteral {{.*}} 4
  // DMP-GWV-NEXT:         OMPSimdlenClause
  // DMP-GWV-NEXT:           ConstantExpr
  // DMP-GWV-NEXT:             value: Int 8
  // DMP-GWV-NEXT:             IntegerLiteral {{.*}} 'int' 8
  //      DMP-NOT:         OMP
  //          DMP:         ForStmt
  //          DMP:           CallExpr
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  //     PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //      PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8)
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCAutoClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCSharedClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang auto
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang auto // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    #pragma acc loop gang auto
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:acc loop gang auto enclosed by acc parallel loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:acc loop gang auto enclosed by acc parallel loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:acc loop gang auto enclosed by acc parallel loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:acc loop gang auto enclosed by acc parallel loop: 1, 1
      TGT_PRINTF("acc loop gang auto enclosed by acc parallel loop: %d, %d\n", i, j);
  } // PRT-NEXT: }


  //----------------------------------------------------------------------------
  // gang, worker, vector, seq function call enclosed.
  //----------------------------------------------------------------------------

  //............................................................................
  // First for acc parallel and acc loop separately.

  // DMP-LABEL:"function call enclosed by acc loop\n"
  // PRT-LABEL:"function call enclosed by acc loop\n"
  // EXE-LABEL:function call enclosed by acc loop
  printf("function call enclosed by acc loop\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //  DMP-NOT:     OMP
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8)
  // DMP: CompoundStmt
  // PRT-NEXT: {
  {
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: ForStmt
    //      DMP:     CallExpr
    //      DMP:       DeclRefExpr {{.*}} 'gangFn'
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   gangFn({{.*}});
    //
    // EXE-TGT-USE-STDIO-DAG:gang function call enclosed by acc loop: 0
    // EXE-TGT-USE-STDIO-DAG:gang function call enclosed by acc loop: 0
    // EXE-TGT-USE-STDIO-DAG:gang function call enclosed by acc loop: 1
    // EXE-TGT-USE-STDIO-DAG:gang function call enclosed by acc loop: 1
    #pragma acc loop
    for (int i = 0; i < 2; ++i)
      gangFn("gang function call enclosed by acc loop", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //      DMP:       CallExpr
    //      DMP:         DeclRefExpr {{.*}} 'workerFn'
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   workerFn({{.*}});
    //
    // EXE-TGT-USE-STDIO-DAG:worker function call enclosed by acc loop: 0
    // EXE-TGT-USE-STDIO-DAG:worker function call enclosed by acc loop: 1
    #pragma acc loop
    for (int i = 0; i < 2; ++i)
      workerFn("worker function call enclosed by acc loop", i);

    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForDirective
    // DMP-GWV-NEXT:     OMPNum_threadsClause
    // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
    //      DMP-NOT:     OMP
    //          DMP:     ForStmt
    //          DMP:       CallExpr
    //          DMP:         DeclRefExpr {{.*}} 'vectorFn'
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(4){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for num_threads(4){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    //        PRT-NEXT: for ({{.*}})
    //        PRT-NEXT:   vectorFn({{.*}});
    //
    // EXE-TGT-USE-STDIO-DAG:vector function call enclosed by acc loop: 0
    // EXE-TGT-USE-STDIO-DAG:vector function call enclosed by acc loop: 1
    #pragma acc loop
    for (int i = 0; i < 2; ++i)
      vectorFn("vector function call enclosed by acc loop", i);

    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-GWV-NEXT:     OMPNum_threadsClause
    // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
    // DMP-GWV-NEXT:     OMPSimdlenClause
    // DMP-GWV-NEXT:       ConstantExpr
    // DMP-GWV-NEXT:         value: Int 8
    // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //      DMP-NOT:     OMP
    //          DMP:     ForStmt
    //          DMP:       CallExpr
    //          DMP:         DeclRefExpr {{.*}} 'seqFn'
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    //        PRT-NEXT: for ({{.*}})
    //        PRT-NEXT:   seqFn({{.*}});
    //
    // EXE-TGT-USE-STDIO-DAG:seq function call enclosed by acc loop: 0
    // EXE-TGT-USE-STDIO-DAG:seq function call enclosed by acc loop: 1
    #pragma acc loop
    for (int i = 0; i < 2; ++i)
      seqFn("seq function call enclosed by acc loop", i);

    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-GWV-NEXT:     OMPNum_threadsClause
    // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
    // DMP-GWV-NEXT:     OMPSimdlenClause
    // DMP-GWV-NEXT:       ConstantExpr
    // DMP-GWV-NEXT:         value: Int 8
    // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //      DMP-NOT:     OMP
    //          DMP:     ForStmt
    //          DMP:       CallExpr
    //          DMP:         DeclRefExpr {{.*}} 'impFn'
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    //        PRT-NEXT: for ({{.*}})
    //        PRT-NEXT:   impFn({{.*}});
    //
    // EXE-TGT-USE-STDIO-DAG:implicit seq function call enclosed by acc loop: 0
    // EXE-TGT-USE-STDIO-DAG:implicit seq function call enclosed by acc loop: 1
    #pragma acc loop
    for (int i = 0; i < 2; ++i)
      impFn("implicit seq function call enclosed by acc loop", i);
  } // PRT-NEXT: }

  //............................................................................
  // Repeat for acc parallel loop.

  // DMP-LABEL:"gang function call enclosed by acc parallel loop\n"
  // PRT-LABEL:"gang function call enclosed by acc parallel loop\n"
  // EXE-LABEL:gang function call enclosed by acc parallel loop
  printf("gang function call enclosed by acc parallel loop\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //  DMP-NOT:       OMP
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       impl: ForStmt
  //      DMP:         CallExpr
  //      DMP:           DeclRefExpr {{.*}} 'gangFn'
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2){{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   gangFn({{.*}});
  //
  // EXE-TGT-USE-STDIO-DAG:gang function call enclosed by acc parallel loop: 0
  // EXE-TGT-USE-STDIO-DAG:gang function call enclosed by acc parallel loop: 0
  // EXE-TGT-USE-STDIO-DAG:gang function call enclosed by acc parallel loop: 1
  // EXE-TGT-USE-STDIO-DAG:gang function call enclosed by acc parallel loop: 1
  #pragma acc parallel loop num_gangs(2)
  for (int i = 0; i < 2; ++i)
    gangFn("gang function call enclosed by acc parallel loop", i);

  // DMP-LABEL:"worker function call enclosed by acc parallel loop\n"
  // PRT-LABEL:"worker function call enclosed by acc parallel loop\n"
  // EXE-LABEL:worker function call enclosed by acc parallel loop
  printf("worker function call enclosed by acc parallel loop\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCNumWorkersClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeDirective
  //  DMP-NOT:         OMP
  //      DMP:         ForStmt
  //      DMP:           CallExpr
  //      DMP:             DeclRefExpr {{.*}} 'workerFn'
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4){{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   workerFn({{.*}});
  //
  // EXE-TGT-USE-STDIO-DAG:worker function call enclosed by acc parallel loop: 0
  // EXE-TGT-USE-STDIO-DAG:worker function call enclosed by acc parallel loop: 1
  #pragma acc parallel loop num_gangs(2) num_workers(4)
  for (int i = 0; i < 2; ++i)
    workerFn("worker function call enclosed by acc parallel loop", i);

  // DMP-LABEL:"vector function call enclosed by acc parallel loop\n"
  // PRT-LABEL:"vector function call enclosed by acc parallel loop\n"
  // EXE-LABEL:vector function call enclosed by acc parallel loop
  printf("vector function call enclosed by acc parallel loop\n");

  //          DMP: ACCParallelLoopDirective
  //     DMP-NEXT:   ACCNumGangsClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:   ACCNumWorkersClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:   ACCVectorLengthClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     ACCNumWorkersClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:     ACCVectorLengthClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCWorkerClause {{.*}} <implicit>
  //   DMP-G-NEXT:       impl: OMPDistributeDirective
  // DMP-GWV-NEXT:       impl: OMPDistributeParallelForDirective
  // DMP-GWV-NEXT:         OMPNum_threadsClause
  // DMP-GWV-NEXT:           IntegerLiteral {{.*}} 4
  //      DMP-NOT:         OMP
  //          DMP:         ForStmt
  //          DMP:           CallExpr
  //          DMP:             DeclRefExpr {{.*}} 'vectorFn'
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  //     PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(4){{$}}
  //      PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for num_threads(4){{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  //        PRT-NEXT: for ({{.*}})
  //        PRT-NEXT:   vectorFn({{.*}});
  //
  // EXE-TGT-USE-STDIO-DAG:vector function call enclosed by acc parallel loop: 0
  // EXE-TGT-USE-STDIO-DAG:vector function call enclosed by acc parallel loop: 1
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8)
  for (int i = 0; i < 2; ++i)
    vectorFn("vector function call enclosed by acc parallel loop", i);

  // DMP-LABEL:"seq function call enclosed by acc parallel loop\n"
  // PRT-LABEL:"seq function call enclosed by acc parallel loop\n"
  // EXE-LABEL:seq function call enclosed by acc parallel loop
  printf("seq function call enclosed by acc parallel loop\n");

  //          DMP: ACCParallelLoopDirective
  //     DMP-NEXT:   ACCNumGangsClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:   ACCNumWorkersClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:   ACCVectorLengthClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     ACCNumWorkersClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:     ACCVectorLengthClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCWorkerClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:       impl: OMPDistributeDirective
  // DMP-GWV-NEXT:       impl: OMPDistributeParallelForSimdDirective
  // DMP-GWV-NEXT:         OMPNum_threadsClause
  // DMP-GWV-NEXT:           IntegerLiteral {{.*}} 4
  // DMP-GWV-NEXT:         OMPSimdlenClause
  // DMP-GWV-NEXT:           ConstantExpr
  // DMP-GWV-NEXT:             value: Int 8
  // DMP-GWV-NEXT:             IntegerLiteral {{.*}} 'int' 8
  //      DMP-NOT:         OMP
  //          DMP:         ForStmt
  //          DMP:           CallExpr
  //          DMP:             DeclRefExpr {{.*}} 'seqFn'
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  //     PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //      PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  //        PRT-NEXT: for ({{.*}})
  //        PRT-NEXT:   seqFn({{.*}});
  //
  // EXE-TGT-USE-STDIO-DAG:seq function call enclosed by acc parallel loop: 0
  // EXE-TGT-USE-STDIO-DAG:seq function call enclosed by acc parallel loop: 1
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8)
  for (int i = 0; i < 2; ++i)
    seqFn("seq function call enclosed by acc parallel loop", i);

  // DMP-LABEL:"implicit seq function call enclosed by acc parallel loop\n"
  // PRT-LABEL:"implicit seq function call enclosed by acc parallel loop\n"
  // EXE-LABEL:implicit seq function call enclosed by acc parallel loop
  printf("implicit seq function call enclosed by acc parallel loop\n");

  //          DMP: ACCParallelLoopDirective
  //     DMP-NEXT:   ACCNumGangsClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:   ACCNumWorkersClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:   ACCVectorLengthClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     ACCNumWorkersClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  //     DMP-NEXT:     ACCVectorLengthClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCWorkerClause {{.*}} <implicit>
  // DMP-GWV-NEXT:       ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:       impl: OMPDistributeDirective
  // DMP-GWV-NEXT:       impl: OMPDistributeParallelForSimdDirective
  // DMP-GWV-NEXT:         OMPNum_threadsClause
  // DMP-GWV-NEXT:           IntegerLiteral {{.*}} 4
  // DMP-GWV-NEXT:         OMPSimdlenClause
  // DMP-GWV-NEXT:           ConstantExpr
  // DMP-GWV-NEXT:             value: Int 8
  // DMP-GWV-NEXT:             IntegerLiteral {{.*}} 'int' 8
  //      DMP-NOT:         OMP
  //          DMP:         ForStmt
  //          DMP:           CallExpr
  //          DMP:             DeclRefExpr {{.*}} 'impFn'
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  //     PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //      PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8){{$}}
  //        PRT-NEXT: for ({{.*}})
  //        PRT-NEXT:   impFn({{.*}});
  //
  // EXE-TGT-USE-STDIO-DAG:implicit seq function call enclosed by acc parallel loop: 0
  // EXE-TGT-USE-STDIO-DAG:implicit seq function call enclosed by acc parallel loop: 1
  #pragma acc parallel loop num_gangs(2) num_workers(4) vector_length(8)
  for (int i = 0; i < 2; ++i)
    impFn("implicit seq function call enclosed by acc parallel loop", i);

  //----------------------------------------------------------------------------
  // Multiple levels that could have gang, worker, or vector clauses.
  //----------------------------------------------------------------------------

  // DMP-LABEL:"multilevel\n"
  // PRT-LABEL:"multilevel\n"
  // EXE-LABEL:multilevel
  printf("multilevel\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8)
  //          DMP: ACCLoopDirective
  //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:   impl: OMPDistributeDirective
  // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
  // DMP-GWV-NEXT:     OMPNum_threadsClause
  // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
  // DMP-GWV-NEXT:     OMPSimdlenClause
  // DMP-GWV-NEXT:       ConstantExpr
  // DMP-GWV-NEXT:         value: Int 8
  // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
  //      DMP-NOT:     OMP
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
  #pragma acc loop
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}}) {
    #pragma acc loop
    for (int j = 0; j < 2; ++j) {
      //  DMP-NOT: OMP
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
      // DMP-NEXT:   impl: ForStmt
      //  DMP-NOT:     OMP
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      //  PRT-A-SAME: {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
      //    PRT-NEXT: for ({{.*}})
      #pragma acc loop
      for (int k = 0; k < 2; ++k)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG:multilevel, separate: 0, 0, 0
        // EXE-TGT-USE-STDIO-DAG:multilevel, separate: 0, 0, 1
        // EXE-TGT-USE-STDIO-DAG:multilevel, separate: 0, 1, 0
        // EXE-TGT-USE-STDIO-DAG:multilevel, separate: 0, 1, 1
        // EXE-TGT-USE-STDIO-DAG:multilevel, separate: 1, 0, 0
        // EXE-TGT-USE-STDIO-DAG:multilevel, separate: 1, 0, 1
        // EXE-TGT-USE-STDIO-DAG:multilevel, separate: 1, 1, 0
        // EXE-TGT-USE-STDIO-DAG:multilevel, separate: 1, 1, 1
        TGT_PRINTF("multilevel, separate: %d, %d, %d\n", i, j, k);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // gang partitioning effect on sibling via parent loop.
  //----------------------------------------------------------------------------

  // DMP-LABEL:"siblings, gang loop first\n"
  // PRT-LABEL:"siblings, gang loop first\n"
  // EXE-LABEL:siblings, gang loop first
  printf("siblings, gang loop first\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8)
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  #pragma acc loop
  for (int i = 0; i < 2; ++i) {
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      //          DMP: ACCLoopDirective
      //     DMP-NEXT:   ACCGangClause
      //      DMP-NOT:     <implicit>
      //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
      // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
      //   DMP-G-NEXT:   impl: OMPDistributeDirective
      // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
      // DMP-GWV-NEXT:     OMPNum_threadsClause
      // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
      // DMP-GWV-NEXT:     OMPSimdlenClause
      // DMP-GWV-NEXT:       ConstantExpr
      // DMP-GWV-NEXT:         value: Int 8
      // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
      //      DMP-NOT:     OMP
      //
      //      PRT-A-NEXT: {{^ *}}#pragma acc loop gang{{$}}
      //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
      //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
      #pragma acc loop gang
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
        // EXE-TGT-USE-STDIO-DAG:siblings, gang loop first, first loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:siblings, gang loop first, first loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:siblings, gang loop first, first loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:siblings, gang loop first, first loop: 1, 1
        TGT_PRINTF("siblings, gang loop first, first loop: %d, %d\n",
                   i, j);

      //          DMP: ACCLoopDirective
      //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
      // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
      //   DMP-G-NEXT:   impl: OMPDistributeDirective
      // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
      // DMP-GWV-NEXT:     OMPNum_threadsClause
      // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
      // DMP-GWV-NEXT:     OMPSimdlenClause
      // DMP-GWV-NEXT:       ConstantExpr
      // DMP-GWV-NEXT:         value: Int 8
      // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
      //      DMP-NOT:     OMP
      //
      //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
      //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
      //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
      #pragma acc loop
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
        // EXE-TGT-USE-STDIO-DAG:siblings, gang loop first, second loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:siblings, gang loop first, second loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:siblings, gang loop first, second loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:siblings, gang loop first, second loop: 1, 1
        TGT_PRINTF("siblings, gang loop first, second loop: %d, %d\n",
                   i, j);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // DMP-LABEL:"siblings, gang function first\n"
  // PRT-LABEL:"siblings, gang function first\n"
  // EXE-LABEL:siblings, gang function first
  printf("siblings, gang function first\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8)
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  #pragma acc loop
  for (int i = 0; i < 2; ++i) {
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // DMP: CallExpr
      // DMP:   DeclRefExpr {{.*}} 'gangFn'
      // PRT-NEXT: gangFn({{.*}});
      // EXE-TGT-USE-STDIO-DAG:siblings, separate, gang function first, function: 0
      // EXE-TGT-USE-STDIO-DAG:siblings, separate, gang function first, function: 0
      // EXE-TGT-USE-STDIO-DAG:siblings, separate, gang function first, function: 1
      // EXE-TGT-USE-STDIO-DAG:siblings, separate, gang function first, function: 1
      gangFn("siblings, separate, gang function first, function", i);

      //          DMP: ACCLoopDirective
      //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
      // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
      //   DMP-G-NEXT:   impl: OMPDistributeDirective
      // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
      // DMP-GWV-NEXT:     OMPNum_threadsClause
      // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
      // DMP-GWV-NEXT:     OMPSimdlenClause
      // DMP-GWV-NEXT:       ConstantExpr
      // DMP-GWV-NEXT:         value: Int 8
      // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
      //      DMP-NOT:     OMP
      //
      //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
      //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
      //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
      #pragma acc loop
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
        // EXE-TGT-USE-STDIO-DAG:siblings, gang function first, loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:siblings, gang function first, loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:siblings, gang function first, loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:siblings, gang function first, loop: 1, 1
        TGT_PRINTF("siblings, gang function first, loop: %d, %d\n",
                   i, j);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // DMP-LABEL:"siblings, gang loop second\n"
  // PRT-LABEL:"siblings, gang loop second\n"
  // EXE-LABEL:siblings, gang loop second
  printf("siblings, gang loop second\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8)
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  #pragma acc loop
  for (int i = 0; i < 2; ++i) {
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      //          DMP: ACCLoopDirective
      //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
      // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
      //   DMP-G-NEXT:   impl: OMPDistributeDirective
      // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
      // DMP-GWV-NEXT:     OMPNum_threadsClause
      // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
      // DMP-GWV-NEXT:     OMPSimdlenClause
      // DMP-GWV-NEXT:       ConstantExpr
      // DMP-GWV-NEXT:         value: Int 8
      // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
      //      DMP-NOT:     OMP
      //
      //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
      //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
      //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
      #pragma acc loop
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
        // EXE-TGT-USE-STDIO-DAG:siblings, gang loop second, first loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:siblings, gang loop second, first loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:siblings, gang loop second, first loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:siblings, gang loop second, first loop: 1, 1
        TGT_PRINTF("siblings, gang loop second, first loop: %d, %d\n",
                   i, j);

      //          DMP: ACCLoopDirective
      //     DMP-NEXT:   ACCGangClause
      //      DMP-NOT:     <implicit>
      //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
      // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
      //   DMP-G-NEXT:   impl: OMPDistributeDirective
      // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
      // DMP-GWV-NEXT:     OMPNum_threadsClause
      // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
      // DMP-GWV-NEXT:     OMPSimdlenClause
      // DMP-GWV-NEXT:       ConstantExpr
      // DMP-GWV-NEXT:         value: Int 8
      // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
      //      DMP-NOT:     OMP
      //
      //      PRT-A-NEXT: {{^ *}}#pragma acc loop gang{{$}}
      //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
      //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
      #pragma acc loop gang
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
        // EXE-TGT-USE-STDIO-DAG:siblings, gang loop second, second loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:siblings, gang loop second, second loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:siblings, gang loop second, second loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:siblings, gang loop second, second loop: 1, 1
        TGT_PRINTF("siblings, gang loop second, second loop: %d, %d\n",
                   i, j);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // DMP-LABEL:"siblings, gang function second\n"
  // PRT-LABEL:"siblings, gang function second\n"
  // EXE-LABEL:siblings, gang function second
  printf("siblings, gang function second\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8)
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  #pragma acc loop
  for (int i = 0; i < 2; ++i) {
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      //          DMP: ACCLoopDirective
      //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
      // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
      //   DMP-G-NEXT:   impl: OMPDistributeDirective
      // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
      // DMP-GWV-NEXT:     OMPNum_threadsClause
      // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
      // DMP-GWV-NEXT:     OMPSimdlenClause
      // DMP-GWV-NEXT:       ConstantExpr
      // DMP-GWV-NEXT:         value: Int 8
      // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
      //      DMP-NOT:     OMP
      //
      //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
      //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
      //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
      #pragma acc loop
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
        // EXE-TGT-USE-STDIO-DAG:siblings, gang function second, loop: 0, 0
        // EXE-TGT-USE-STDIO-DAG:siblings, gang function second, loop: 0, 1
        // EXE-TGT-USE-STDIO-DAG:siblings, gang function second, loop: 1, 0
        // EXE-TGT-USE-STDIO-DAG:siblings, gang function second, loop: 1, 1
        TGT_PRINTF("siblings, gang function second, loop: %d, %d\n",
                   i, j);

      // DMP: CallExpr
      // DMP:   DeclRefExpr {{.*}} 'gangFn'
      // PRT-NEXT: gangFn({{.*}});
      // EXE-TGT-USE-STDIO-DAG:siblings, separate, gang function second, function: 0
      // EXE-TGT-USE-STDIO-DAG:siblings, separate, gang function second, function: 0
      // EXE-TGT-USE-STDIO-DAG:siblings, separate, gang function second, function: 1
      // EXE-TGT-USE-STDIO-DAG:siblings, separate, gang function second, function: 1
      gangFn("siblings, separate, gang function second, function", i);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // Implicit gang, worker, and vector each at a different level.
  //----------------------------------------------------------------------------

  // DMP-LABEL:"each at different level\n"
  // PRT-LABEL:"each at different level\n"
  // EXE-LABEL:each at different level
  printf("each at different level\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8)
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     CompoundStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  #pragma acc loop
  for (int i = 0; i < 2; ++i) {
    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCGangClause
    //      DMP-NOT:     <implicit>
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-GWV-NEXT:     OMPNum_threadsClause
    // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
    // DMP-GWV-NEXT:     OMPSimdlenClause
    // DMP-GWV-NEXT:       ConstantExpr
    // DMP-GWV-NEXT:         value: Int 8
    // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
    //      DMP-NOT:     OMP
    //          DMP:     ForStmt
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop gang{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
    //        PRT-NEXT: for ({{.*}})
    #pragma acc loop gang
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
      // EXE-TGT-USE-STDIO-DAG:each at different level, explicit gang loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:each at different level, explicit gang loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:each at different level, explicit gang loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:each at different level, explicit gang loop: 1, 1
      TGT_PRINTF("each at different level, explicit gang loop: %d, %d\n",
                 i, j);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    #pragma acc loop
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}}) {
    for (int j = 0; j < 2; ++j) {
      //          DMP: ACCLoopDirective
      //     DMP-NEXT:   ACCWorkerClause
      //      DMP-NOT:     <implicit>
      //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      //     DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
      // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
      //   DMP-G-NEXT:   impl: OMPParallelForDirective
      // DMP-GWV-NEXT:   impl: OMPParallelForSimdDirective
      //     DMP-NEXT:     OMPNum_threadsClause
      //     DMP-NEXT:       IntegerLiteral {{.*}} 4
      // DMP-GWV-NEXT:     OMPSimdlenClause
      // DMP-GWV-NEXT:       ConstantExpr
      // DMP-GWV-NEXT:         value: Int 8
      // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
      //      DMP-NOT:     OMP
      //          DMP:     ForStmt
      //
      //      PRT-A-NEXT: {{^ *}}#pragma acc loop worker{{$}}
      //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp parallel for num_threads(4){{$}}
      // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp parallel for simd num_threads(4) simdlen(8){{$}}
      //    PRT-O-G-NEXT: {{^ *}}#pragma omp parallel for num_threads(4){{$}}
      //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp parallel for simd num_threads(4) simdlen(8){{$}}
      //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker{{$}}
      //        PRT-NEXT: for ({{.*}})
      #pragma acc loop worker
      for (int k = 0; k < 2; ++k)
        // DMP: CallExpr
        // PRT-NEXT: {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
        // EXE-TGT-USE-STDIO-DAG:each at different level, explicit worker loop: 0, 0, 0
        // EXE-TGT-USE-STDIO-DAG:each at different level, explicit worker loop: 0, 0, 1
        // EXE-TGT-USE-STDIO-DAG:each at different level, explicit worker loop: 0, 1, 0
        // EXE-TGT-USE-STDIO-DAG:each at different level, explicit worker loop: 0, 1, 1
        // EXE-TGT-USE-STDIO-DAG:each at different level, explicit worker loop: 1, 0, 0
        // EXE-TGT-USE-STDIO-DAG:each at different level, explicit worker loop: 1, 0, 1
        // EXE-TGT-USE-STDIO-DAG:each at different level, explicit worker loop: 1, 1, 0
        // EXE-TGT-USE-STDIO-DAG:each at different level, explicit worker loop: 1, 1, 1
        TGT_PRINTF("each at different level, explicit worker loop: %d, %d, %d\n",
                   i, j, k);

      //          DMP: ACCLoopDirective
      //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      //     DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
      // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
      //   DMP-G-NEXT:   impl: ForStmt
      // DMP-GWV-NEXT:   impl: OMPParallelForDirective
      // DMP-GWV-NEXT:     OMPNum_threadsClause
      // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
      //  DMP-GWV-NOT:     OMP
      //      DMP-GWV:     ForStmt
      //
      //      PRT-A-NEXT: {{^ *}}#pragma acc loop
      //   PRT-AO-G-SAME: {{^}} // discarded in OpenMP translation
      //      PRT-A-SAME: {{^$}}
      // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp parallel for num_threads(4){{$}}
      //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp parallel for num_threads(4){{$}}
      //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop
      //   PRT-OA-G-SAME: {{^}} // discarded in OpenMP translation
      //     PRT-OA-SAME: {{^$}}
      //        PRT-NEXT: for ({{.*}}) {
      #pragma acc loop
      for (int k = 0; k < 2; ++k) {
        //      DMP: ACCLoopDirective
        // DMP-NEXT:   ACCVectorClause
        //  DMP-NOT:     <implicit>
        // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
        // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
        // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
        // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
        // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
        // DMP-NEXT:   impl: OMPSimdDirective
        // DMP-NEXT:     OMPSimdlenClause
        // DMP-NEXT:       ConstantExpr
        // DMP-NEXT:         value: Int 8
        // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 8
        //   MP-NOT:     OMP
        //      DMP:     ForStmt
        //
        //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector{{$}}
        // PRT-AO-NEXT: {{^ *}}// #pragma omp simd simdlen(8){{$}}
        //  PRT-O-NEXT: {{^ *}}#pragma omp simd simdlen(8){{$}}
        // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector{{$}}
        //    PRT-NEXT: for ({{.*}})
        #pragma acc loop vector
        for (int l = 0; l < 2; ++l)
          // DMP: CallExpr
          // PRT-NEXT: {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
          // EXE-TGT-USE-STDIO-DAG:each at different level, explicit vector loop: 0, 0, 0, 0
          // EXE-TGT-USE-STDIO-DAG:each at different level, explicit vector loop: 0, 0, 0, 1
          // EXE-TGT-USE-STDIO-DAG:each at different level, explicit vector loop: 0, 0, 1, 0
          // EXE-TGT-USE-STDIO-DAG:each at different level, explicit vector loop: 0, 0, 1, 1
          // EXE-TGT-USE-STDIO-DAG:each at different level, explicit vector loop: 0, 1, 0, 0
          // EXE-TGT-USE-STDIO-DAG:each at different level, explicit vector loop: 0, 1, 0, 1
          // EXE-TGT-USE-STDIO-DAG:each at different level, explicit vector loop: 0, 1, 1, 0
          // EXE-TGT-USE-STDIO-DAG:each at different level, explicit vector loop: 0, 1, 1, 1
          // EXE-TGT-USE-STDIO-DAG:each at different level, explicit vector loop: 1, 0, 0, 0
          // EXE-TGT-USE-STDIO-DAG:each at different level, explicit vector loop: 1, 0, 0, 1
          // EXE-TGT-USE-STDIO-DAG:each at different level, explicit vector loop: 1, 0, 1, 0
          // EXE-TGT-USE-STDIO-DAG:each at different level, explicit vector loop: 1, 0, 1, 1
          // EXE-TGT-USE-STDIO-DAG:each at different level, explicit vector loop: 1, 1, 0, 0
          // EXE-TGT-USE-STDIO-DAG:each at different level, explicit vector loop: 1, 1, 0, 1
          // EXE-TGT-USE-STDIO-DAG:each at different level, explicit vector loop: 1, 1, 1, 0
          // EXE-TGT-USE-STDIO-DAG:each at different level, explicit vector loop: 1, 1, 1, 1
          TGT_PRINTF("each at different level, explicit vector loop: %d, %d, %d, %d\n",
                     i, j, k, l);

        //          DMP: ACCLoopDirective
        //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
        //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
        //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
        //     DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
        //     DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
        // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
        //   DMP-G-NEXT:   impl: ForStmt
        // DMP-GWV-NEXT:   impl: OMPSimdDirective
        // DMP-GWV-NEXT:     OMPSimdlenClause
        // DMP-GWV-NEXT:       ConstantExpr
        // DMP-GWV-NEXT:         value: Int 8
        // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
        //  DMP-GWV-NOT:     OMP
        //      DMP-GWV:     ForStmt
        //
        //      PRT-A-NEXT: {{^ *}}#pragma acc loop
        //   PRT-AO-G-SAME: {{^}} // discarded in OpenMP translation
        //      PRT-A-SAME: {{^$}}
        // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp simd simdlen(8){{$}}
        //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp simd simdlen(8){{$}}
        //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop
        //   PRT-OA-G-SAME: {{^}} // discarded in OpenMP translation
        //     PRT-OA-SAME: {{^$}}
        //        PRT-NEXT: for ({{.*}})
        #pragma acc loop
        for (int l = 0; l < 2; ++l)
          // DMP: CallExpr
          // PRT-NEXT: {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
          // EXE-TGT-USE-STDIO-DAG:each at different level, innermost loop: 0, 0, 0, 0
          // EXE-TGT-USE-STDIO-DAG:each at different level, innermost loop: 0, 0, 0, 1
          // EXE-TGT-USE-STDIO-DAG:each at different level, innermost loop: 0, 0, 1, 0
          // EXE-TGT-USE-STDIO-DAG:each at different level, innermost loop: 0, 0, 1, 1
          // EXE-TGT-USE-STDIO-DAG:each at different level, innermost loop: 0, 1, 0, 0
          // EXE-TGT-USE-STDIO-DAG:each at different level, innermost loop: 0, 1, 0, 1
          // EXE-TGT-USE-STDIO-DAG:each at different level, innermost loop: 0, 1, 1, 0
          // EXE-TGT-USE-STDIO-DAG:each at different level, innermost loop: 0, 1, 1, 1
          // EXE-TGT-USE-STDIO-DAG:each at different level, innermost loop: 1, 0, 0, 0
          // EXE-TGT-USE-STDIO-DAG:each at different level, innermost loop: 1, 0, 0, 1
          // EXE-TGT-USE-STDIO-DAG:each at different level, innermost loop: 1, 0, 1, 0
          // EXE-TGT-USE-STDIO-DAG:each at different level, innermost loop: 1, 0, 1, 1
          // EXE-TGT-USE-STDIO-DAG:each at different level, innermost loop: 1, 1, 0, 0
          // EXE-TGT-USE-STDIO-DAG:each at different level, innermost loop: 1, 1, 0, 1
          // EXE-TGT-USE-STDIO-DAG:each at different level, innermost loop: 1, 1, 1, 0
          // EXE-TGT-USE-STDIO-DAG:each at different level, innermost loop: 1, 1, 1, 1
          TGT_PRINTF("each at different level, innermost loop: %d, %d, %d, %d\n",
                     i, j, k, l);
      } // PRT-NEXT: }
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // No num_workers or vector_length to copy to loops in translation.
  //----------------------------------------------------------------------------

  // DMP-LABEL:"no num_workers or vector_length, separate\n"
  // PRT-LABEL:"no num_workers or vector_length, separate\n"
  // EXE-LABEL:no num_workers or vector_length, separate
  printf("no num_workers or vector_length, separate\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //  DMP-NOT:     OMP
  //      DMP:     CompoundStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2){{$}}
  //    PRT-NEXT: {
  #pragma acc parallel num_gangs(2)
  {
    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    //      DMP-NOT:     OMP
    //          DMP:     CallExpr
    //          DMP:       ForStmt
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    //        PRT-NEXT: for ({{.*}})
    //        PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG:no num_workers or vector_length, separate: 0
    // EXE-TGT-USE-STDIO-DAG:no num_workers or vector_length, separate: 1
    #pragma acc loop
    for (int i = 0; i < 2; ++i)
      TGT_PRINTF("no num_workers or vector_length, separate: %d\n", i);
  } // PRT-NEXT: }

  // DMP-LABEL:"no num_workers or vector_length, combined\n"
  // PRT-LABEL:"no num_workers or vector_length, combined\n"
  // EXE-LABEL:no num_workers or vector_length, combined
  printf("no num_workers or vector_length, combined\n");

  //          DMP: ACCParallelDirective
  //     DMP-NEXT:   ACCNumGangsClause
  //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:   impl: OMPTargetTeamsDirective
  //     DMP-NEXT:     OMPNum_teamsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //          DMP:   ACCLoopDirective
  //     DMP-NEXT:     ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:     ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:     ACCWorkerClause {{.*}} <implicit>
  // DMP-GWV-NEXT:     ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:     impl: OMPDistributeDirective
  // DMP-GWV-NEXT:     impl: OMPDistributeParallelForSimdDirective
  //      DMP-NOT:       OMP
  //          DMP:       CallExpr
  //          DMP:         ForStmt
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2){{$}}
  //     PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
  //      PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2){{$}}
  //        PRT-NEXT: for ({{.*}})
  //        PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:no num_workers or vector_length, combined: 0
  // EXE-TGT-USE-STDIO-DAG:no num_workers or vector_length, combined: 1
  #pragma acc parallel loop num_gangs(2)
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("no num_workers or vector_length, combined: %d\n", i);

  //----------------------------------------------------------------------------
  // Interaction with tile clauses.
  //
  // During translation to OpenMP, our implementation currently discards any
  // parallelism that OpenACC specifies for the tile clause's generated element
  // loop.  Check that that works for implicit worker and vector clauses.  Check
  // that any implicit worker for the tile loop is not discarded.  (OpenACC
  // never specifies vector for the tile loop.)
  //----------------------------------------------------------------------------

  // DMP-LABEL:"tile, implicit vector discarded\n"
  // PRT-LABEL:"tile, implicit vector discarded\n"
  // EXE-LABEL:tile, implicit vector discarded
  printf("tile, implicit vector discarded\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //  DMP-NOT:     OMP
  //      DMP:     CompoundStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  //    PRT-NEXT: {
  #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8)
  {
    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCTileClause
    //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForDirective
    // DMP-GWV-NEXT:     OMPNum_threadsClause
    // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
    //          DMP:     OMPTileDirective
    //     DMP-NEXT:       OMPSizesClause
    //     DMP-NEXT:         IntegerLiteral {{.*}} 2
    //      DMP-NOT:     OMP
    //          DMP:     CallExpr
    //          DMP:       ForStmt
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop tile(2){{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(4){{$}}
    //     PRT-AO-NEXT: {{^ *}}// #pragma omp tile sizes(2){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for num_threads(4){{$}}
    //      PRT-O-NEXT: {{^ *}}#pragma omp tile sizes(2){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop tile(2){{$}}
    //        PRT-NEXT: for ({{.*}})
    //        PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG:tile, implicit vector discarded: 0
    // EXE-TGT-USE-STDIO-DAG:tile, implicit vector discarded: 1
    #pragma acc loop tile(2)
    for (int i = 0; i < 2; ++i)
      TGT_PRINTF("tile, implicit vector discarded: %d\n", i);
  } // PRT-NEXT: }

  // DMP-LABEL:"tile, implicit worker discarded\n"
  // PRT-LABEL:"tile, implicit worker discarded\n"
  // EXE-LABEL:tile, implicit worker discarded
  printf("tile, implicit worker discarded\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //  DMP-NOT:     OMP
  //      DMP:     CompoundStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  //    PRT-NEXT: {
  #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8)
  {
    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCTileClause
    //     DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    //     DMP-NEXT:   impl: OMPDistributeDirective
    //          DMP:     OMPTileDirective
    //     DMP-NEXT:       OMPSizesClause
    //     DMP-NEXT:         IntegerLiteral {{.*}} 2
    //      DMP-NOT:     OMP
    //          DMP:     CallExpr
    //          DMP:       ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop tile(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp tile sizes(2){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp tile sizes(2){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop tile(2){{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   vectorFn({{.*}});
    //
    // EXE-TGT-USE-STDIO-DAG:tile, implicit worker discarded: 0
    // EXE-TGT-USE-STDIO-DAG:tile, implicit worker discarded: 1
    #pragma acc loop tile(2)
    for (int i = 0; i < 2; ++i)
      vectorFn("tile, implicit worker discarded", i);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // For a loop with an implicit vector clause, check the translation of
  // predetermined private for a loop control variable that is assigned but not
  // declared in the init of the attached for loop.
  //----------------------------------------------------------------------------

  // DMP-LABEL:"predetermined private loop var with implicit vector\n"
  // PRT-LABEL:"predetermined private loop var with implicit vector\n"
  // EXE-LABEL:predetermined private loop var with implicit vector
  printf("predetermined private loop var with implicit vector\n");

  // PRT-NEXT: {
  {
    // PRT-NEXT: int i = 99;
    int i = 99;
    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCNumWorkersClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:   ACCVectorLengthClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    //  DMP-NOT:     OMP
    //      DMP:     CompoundStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
    //    PRT-NEXT: {
    #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8)
    {
      //          DMP: ACCLoopDirective
      //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      //     DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
      //     DMP-NEXT:     DeclRefExpr {{.*}} 'i'
      //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
      // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
      //   DMP-G-NEXT:   impl: OMPDistributeDirective
      //   DMP-G-NEXT:     OMPPrivateClause
      //   DMP-G-NEXT:       DeclRefExpr {{.*}} 'i'
      //    DMP-G-NOT:     OMP
      //        DMP-G:     ForStmt
      //        DMP-G:       CallExpr
      // DMP-GWV-NEXT:   impl: CompoundStmt
      // DMP-GWV-NEXT:     DeclStmt
      // DMP-GWV-NEXT:       VarDecl {{.*}} i 'int'
      // DMP-GWV-NEXT:     OMPDistributeParallelForSimdDirective
      // DMP-GWV-NEXT:       OMPNum_threadsClause
      // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 4
      // DMP-GWV-NEXT:       OMPSimdlenClause
      // DMP-GWV-NEXT:         ConstantExpr
      // DMP-GWV-NEXT:           value: Int 8
      // DMP-GWV-NEXT:           IntegerLiteral {{.*}} 'int' 8
      //  DMP-GWV-NOT:       OMP
      //      DMP-GWV:       ForStmt
      //      DMP-GWV:         CallExpr
      //
      //    PRT-A-G-NEXT: {{^ *}}#pragma acc loop{{$}}
      //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute private(i){{$}}
      //    PRT-A-G-NEXT: for ({{.*}})
      //    PRT-A-G-NEXT:   {{TGT_PRINTF|printf}}
      //
      //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute private(i){{$}}
      //   PRT-OA-G-NEXT: {{^ *}}// #pragma acc loop{{$}}
      //    PRT-O-G-NEXT: for ({{.*}})
      //    PRT-O-G-NEXT:   {{TGT_PRINTF|printf}}
      //
      // PRT-AO-GWV-NEXT: // v----------ACC----------v
      //  PRT-A-GWV-NEXT: {{^ *}}#pragma acc loop{{$}}
      //  PRT-A-GWV-NEXT: for ({{.*}})
      //  PRT-A-GWV-NEXT:   {{TGT_PRINTF|printf}}
      // PRT-AO-GWV-NEXT: // ---------ACC->OMP--------
      // PRT-AO-GWV-NEXT: // {
      // PRT-AO-GWV-NEXT: //   int i;
      // PRT-AO-GWV-NEXT: //   #pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      // PRT-AO-GWV-NEXT: //   for ({{.*}})
      // PRT-AO-GWV-NEXT: //     {{TGT_PRINTF|printf}}
      // PRT-AO-GWV-NEXT: // }
      // PRT-AO-GWV-NEXT: // ^----------OMP----------^
      //
      // PRT-OA-GWV-NEXT: // v----------OMP----------v
      //  PRT-O-GWV-NEXT: {
      //  PRT-O-GWV-NEXT:   int i;
      //  PRT-O-GWV-NEXT:   {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8){{$}}
      //  PRT-O-GWV-NEXT:   for ({{.*}})
      //  PRT-O-GWV-NEXT:     {{TGT_PRINTF|printf}}
      //  PRT-O-GWV-NEXT: }
      // PRT-OA-GWV-NEXT: // ---------OMP<-ACC--------
      // PRT-OA-GWV-NEXT: // #pragma acc loop{{$}}
      // PRT-OA-GWV-NEXT: // for ({{.*}})
      // PRT-OA-GWV-NEXT: //   {{TGT_PRINTF|printf}}
      // PRT-OA-GWV-NEXT: // ^----------ACC----------^
      //
      //  PRT-NOACC-NEXT: for ({{.*}})
      //  PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
      //
      // EXE-TGT-USE-STDIO-DAG:predetermined private loop var with implicit vector: 0
      // EXE-TGT-USE-STDIO-DAG:predetermined private loop var with implicit vector: 1
      #pragma acc loop
      for (i = 0; i < 2; ++i)
        TGT_PRINTF("predetermined private loop var with implicit vector: %d\n", i);
    } // PRT-NEXT: }
    // PRT-NEXT: printf
    // EXE-NEXT:predetermined private loop var with implicit vector: after i=99
    printf("predetermined private loop var with implicit vector: after i=%d\n", i);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // Check that a reduction is not discarded during translation if there are
  // implicit worker or vector clauses.
  //----------------------------------------------------------------------------

  // DMP-LABEL:"reduction discarded if only gang\n"
  // PRT-LABEL:"reduction discarded if only gang\n"
  // EXE-LABEL:reduction discarded if only gang
  printf("reduction discarded if only gang\n");

  // PRT-NEXT: {
  {
    // PRT-NEXT: int x = 0;
    int x = 0;
    //      DMP: ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCNumWorkersClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:   ACCVectorLengthClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
    // DMP-NEXT:   ACCCopyClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:   ACCReductionClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'x'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'x'
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(2) num_workers(4) vector_length(8) copy(x){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: x) reduction(+: x){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: x) reduction(+: x){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8) copy(x){{$}}
    #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8) copy(x)
    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCReductionClause {{.*}} '+'
    //     DMP-NEXT:     DeclRefExpr {{.*}} 'x'
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-GWV-NEXT:     OMPNum_threadsClause
    // DMP-GWV-NEXT:       IntegerLiteral {{.*}} 4
    // DMP-GWV-NEXT:     OMPSimdlenClause
    // DMP-GWV-NEXT:       ConstantExpr
    // DMP-GWV-NEXT:         value: Int 8
    // DMP-GWV-NEXT:         IntegerLiteral {{.*}} 'int' 8
    // DMP-GWV-NEXT:     OMPReductionClause
    // DMP-GWV-NEXT:       DeclRefExpr {{.*}} 'x'
    //      DMP-NOT:     OMP
    //          DMP:     ForStmt
    //          DMP:       CompoundAssignOperator
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop reduction(+: x){{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(4) simdlen(8) reduction(+: x){{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(4) simdlen(8) reduction(+: x){{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop reduction(+: x){{$}}
    //        PRT-NEXT: for ({{.*}})
    //        PRT-NEXT:   x += i;
    #pragma acc loop reduction(+: x)
    for (int i = 0; i < 256; ++i)
      x += i;
    // PRT-NEXT: printf
    // EXE-NEXT:reduction discarded if only gang: after x=32640
    printf("reduction discarded if only gang: after x=%d\n", x);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // Check implicit gang, worker, and vector clauses within accelerator
  // routines.
  //----------------------------------------------------------------------------

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8){{$}}
  //    PRT-NEXT: {
  //    PRT-NEXT:   withinGangFn();
  //    PRT-NEXT:   withinWorkerFn();
  //    PRT-NEXT:   withinVectorFn();
  //    PRT-NEXT:   withinSeqFn();
  //    PRT-NEXT: }
  #pragma acc parallel num_gangs(2) num_workers(4) vector_length(8)
  {
    withinGangFn();
    withinWorkerFn();
    withinVectorFn();
    withinSeqFn();
  }

  // PRT-NEXT: return 0;
  return 0;
} // PRT-NEXT: }

//==============================================================================
// gang function's loops can have implicit gang, worker, and vector clauses.
//
// This repeats enough of the above testing to be sure the same analyses work
// within a gang function.
//==============================================================================

// DMP-LABEL: FunctionDecl {{.*}} withinGangFn
// PRT-LABEL: void withinGangFn() {
void withinGangFn() {
  //----------------------------------------------------------------------------
  // gang, worker, vector, seq, auto combinations (without loop nesting, which
  // is checked later)
  //----------------------------------------------------------------------------

  //          DMP: ACCLoopDirective
  //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:   impl: OMPDistributeDirective
  // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
  //      DMP-NOT:     OMP
  //          DMP:     ForStmt
  //          DMP:       CallExpr
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
  //        PRT-NEXT: for ({{.*}})
  //        PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop without nesting: 1
  #pragma acc loop
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinGangFn: orphaned acc loop without nesting: %d\n", i);

  //          DMP: ACCLoopDirective
  //     DMP-NEXT:   ACCGangClause
  //      DMP-NOT:     <implicit>
  //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:   impl: OMPDistributeDirective
  // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
  //      DMP-NOT:     OMP
  //          DMP:     ForStmt
  //          DMP:       CallExpr
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc loop gang{{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
  //        PRT-NEXT: for ({{.*}})
  //        PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop gang without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop gang without nesting: 1
  #pragma acc loop gang
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinGangFn: orphaned acc loop gang without nesting: %d\n", i);

  //      DMP-NOT: OMP
  //          DMP: ACCLoopDirective
  //     DMP-NEXT:   ACCWorkerClause
  //      DMP-NOT:     <implicit>
  //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:   impl: OMPDistributeParallelForDirective
  // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
  //      DMP-NOT:     OMP
  //          DMP:     CallExpr
  //          DMP:       ForStmt
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc loop worker{{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute parallel for{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute parallel for{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker{{$}}
  //        PRT-NEXT: for ({{.*}})
  //        PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop worker without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop worker without nesting: 1
  #pragma acc loop worker
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinGangFn: orphaned acc loop worker without nesting: %d\n", i);

  //      DMP-NOT: OMP
  //          DMP: ACCLoopDirective
  //     DMP-NEXT:   ACCVectorClause
  //      DMP-NOT:     <implicit>
  //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
  //   DMP-G-NEXT:   impl: OMPDistributeSimdDirective
  // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
  //      DMP-NOT:     OMP
  //          DMP:     ForStmt
  //          DMP:       CallExpr
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc loop vector{{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute simd{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute simd{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector{{$}}
  //        PRT-NEXT: for ({{.*}})
  //        PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop vector without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop vector without nesting: 1
  #pragma acc loop vector
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinGangFn: orphaned acc loop vector without nesting: %d\n", i);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCSeqClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop seq without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop seq without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop seq without nesting: 1
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop seq without nesting: 1
  #pragma acc loop seq
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinGangFn: orphaned acc loop seq without nesting: %d\n", i);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop auto without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop auto without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop auto without nesting: 1
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop auto without nesting: 1
  #pragma acc loop auto
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinGangFn: orphaned acc loop auto without nesting: %d\n", i);

  //          DMP: ACCLoopDirective
  //     DMP-NEXT:   ACCGangClause
  //      DMP-NOT:     <implicit>
  //     DMP-NEXT:   ACCWorkerClause
  //      DMP-NOT:     <implicit>
  //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:   impl: OMPDistributeParallelForDirective
  // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
  //      DMP-NOT:     OMP
  //          DMP:     ForStmt
  //          DMP:       CallExpr
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc loop gang worker{{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute parallel for{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute parallel for{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang worker{{$}}
  //        PRT-NEXT: for ({{.*}})
  //        PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop gang worker without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop gang worker without nesting: 1
  #pragma acc loop gang worker
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinGangFn: orphaned acc loop gang worker without nesting: %d\n", i);

  //          DMP: ACCLoopDirective
  //     DMP-NEXT:   ACCGangClause
  //      DMP-NOT:     <implicit>
  //     DMP-NEXT:   ACCVectorClause
  //      DMP-NOT:     <implicit>
  //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
  //   DMP-G-NEXT:   impl: OMPDistributeSimdDirective
  // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
  //      DMP-NOT:     OMP
  //          DMP:     ForStmt
  //          DMP:       CallExpr
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc loop gang vector{{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute simd{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute simd{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang vector{{$}}
  //        PRT-NEXT: for ({{.*}})
  //        PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop gang vector without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop gang vector without nesting: 1
  #pragma acc loop gang vector
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinGangFn: orphaned acc loop gang vector without nesting: %d\n", i);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop worker vector without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop worker vector without nesting: 1
  #pragma acc loop worker vector
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinGangFn: orphaned acc loop worker vector without nesting: %d\n", i);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang worker vector{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang worker vector{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop gang worker vector without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop gang worker vector without nesting: 1
  #pragma acc loop gang worker vector
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinGangFn: orphaned acc loop gang worker vector without nesting: %d\n", i);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang auto
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang auto // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop gang auto without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop gang auto without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop gang auto without nesting: 1
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop gang auto without nesting: 1
  #pragma acc loop gang auto
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinGangFn: orphaned acc loop gang auto without nesting: %d\n", i);

  //----------------------------------------------------------------------------
  // gang, worker, vector, seq, auto on enclosing acc loop.
  //----------------------------------------------------------------------------

  //          DMP: ACCLoopDirective
  //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:   impl: OMPDistributeDirective
  // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
  //      DMP-NOT:     OMP
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
  #pragma acc loop
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop enclosing acc loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop enclosing acc loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop enclosing acc loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop enclosing acc loop: 1, 1
      TGT_PRINTF("withinGangFn: orphaned acc loop enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //          DMP: ACCLoopDirective
  //     DMP-NEXT:   ACCGangClause
  //      DMP-NOT:     <implicit>
  //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:   impl: OMPDistributeDirective
  // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
  //      DMP-NOT:     OMP
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc loop gang{{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
  #pragma acc loop gang
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop gang enclosing acc loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop gang enclosing acc loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop gang enclosing acc loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop gang enclosing acc loop: 1, 1
      TGT_PRINTF("withinGangFn: orphaned acc loop gang enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //      DMP-NOT: OMP
  //          DMP: ACCLoopDirective
  //     DMP-NEXT:   ACCWorkerClause
  //      DMP-NOT:     <implicit>
  //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:   impl: OMPDistributeParallelForDirective
  // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
  //      DMP-NOT:     OMP
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc loop worker{{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute parallel for{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute parallel for{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker{{$}}
  #pragma acc loop worker
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop worker enclosing acc loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop worker enclosing acc loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop worker enclosing acc loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop worker enclosing acc loop: 1, 1
      TGT_PRINTF("withinGangFn: orphaned acc loop worker enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //      DMP-NOT: OMP
  //          DMP: ACCLoopDirective
  //     DMP-NEXT:   ACCVectorClause
  //      DMP-NOT:     <implicit>
  //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
  //   DMP-G-NEXT:   impl: OMPDistributeSimdDirective
  // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
  //      DMP-NOT:     OMP
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc loop vector{{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute simd{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute simd{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector{{$}}
  #pragma acc loop vector
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop vector enclosing acc loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop vector enclosing acc loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop vector enclosing acc loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop vector enclosing acc loop: 1, 1
      TGT_PRINTF("withinGangFn: orphaned acc loop vector enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCSeqClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  #pragma acc loop seq
  for (int i = 0; i < 2; ++i) {
    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    //      DMP-NOT:     OMP
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    #pragma acc loop
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop seq enclosing acc loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop seq enclosing acc loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop seq enclosing acc loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop seq enclosing acc loop: 1, 1
      TGT_PRINTF("withinGangFn: orphaned acc loop seq enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  #pragma acc loop auto
  for (int i = 0; i < 2; ++i) {
    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    //      DMP-NOT:     OMP
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    #pragma acc loop
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop auto enclosing acc loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop auto enclosing acc loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop auto enclosing acc loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop auto enclosing acc loop: 1, 1
      TGT_PRINTF("withinGangFn: orphaned acc loop auto enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang auto
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang auto // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  #pragma acc loop gang auto
  for (int i = 0; i < 2; ++i) {
    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    //      DMP-NOT:     OMP
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    #pragma acc loop
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop gang auto enclosing acc loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop gang auto enclosing acc loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop gang auto enclosing acc loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: orphaned acc loop gang auto enclosing acc loop: 1, 1
      TGT_PRINTF("withinGangFn: orphaned acc loop gang auto enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // gang, worker, vector, seq, auto on enclosed acc loop.
  //----------------------------------------------------------------------------

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  #pragma acc loop
  for (int i = 0; i < 2; ++i) {
    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCGangClause
    //      DMP-NOT:     <implicit>
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPDistributeDirective
    // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
    //      DMP-NOT:     OMP
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop gang{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
    #pragma acc loop gang
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: acc loop gang enclosed by orphaned acc loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: acc loop gang enclosed by orphaned acc loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: acc loop gang enclosed by orphaned acc loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: acc loop gang enclosed by orphaned acc loop: 1, 1
      TGT_PRINTF("withinGangFn: acc loop gang enclosed by orphaned acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  //  DMP-NOT:     OMP
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
  #pragma acc loop
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    //          DMP: ACCLoopDirective
    //     DMP-NEXT:   ACCWorkerClause
    //      DMP-NOT:     <implicit>
    //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    //     DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    //     DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
    //   DMP-G-NEXT:   impl: OMPParallelForDirective
    // DMP-GWV-NEXT:   impl: OMPParallelForSimdDirective
    //      DMP-NOT:     OMP
    //
    //      PRT-A-NEXT: {{^ *}}#pragma acc loop worker{{$}}
    //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp parallel for{{$}}
    // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp parallel for simd{{$}}
    //    PRT-O-G-NEXT: {{^ *}}#pragma omp parallel for{{$}}
    //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp parallel for simd{{$}}
    //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker{{$}}
    #pragma acc loop worker
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: acc loop worker enclosed by orphaned acc loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: acc loop worker enclosed by orphaned acc loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: acc loop worker enclosed by orphaned acc loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: acc loop worker enclosed by orphaned acc loop: 1, 1
      TGT_PRINTF("withinGangFn: acc loop worker enclosed by orphaned acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //          DMP: ACCLoopDirective
  //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
  //   DMP-G-NEXT:   impl: OMPDistributeDirective
  // DMP-GWV-NEXT:   impl: OMPDistributeParallelForDirective
  //      DMP-NOT:     OMP
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for{{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for{{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
  #pragma acc loop
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCVectorClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: OMPSimdDirective
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp simd{{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp simd{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector{{$}}
    #pragma acc loop vector
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: acc loop vector enclosed by acc loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: acc loop vector enclosed by acc loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: acc loop vector enclosed by acc loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: acc loop vector enclosed by acc loop: 1, 1
      TGT_PRINTF("withinGangFn: acc loop vector enclosed by acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //          DMP: ACCLoopDirective
  //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:   impl: OMPDistributeDirective
  // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
  //      DMP-NOT:     OMP
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
  #pragma acc loop
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCSharedClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    #pragma acc loop seq
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: acc loop seq enclosed by orphaned acc loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: acc loop seq enclosed by orphaned acc loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: acc loop seq enclosed by orphaned acc loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: acc loop seq enclosed by orphaned acc loop: 1, 1
      TGT_PRINTF("withinGangFn: acc loop seq enclosed by orphaned acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //          DMP: ACCLoopDirective
  //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:   impl: OMPDistributeDirective
  // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
  //      DMP-NOT:     OMP
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
  #pragma acc loop
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCAutoClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCSharedClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    #pragma acc loop auto
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: acc loop auto enclosed by orphaned acc loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: acc loop auto enclosed by orphaned acc loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: acc loop auto enclosed by orphaned acc loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: acc loop auto enclosed by orphaned acc loop: 1, 1
      TGT_PRINTF("withinGangFn: acc loop auto enclosed by orphaned acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //          DMP: ACCLoopDirective
  //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:   impl: OMPDistributeDirective
  // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
  //      DMP-NOT:     OMP
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
  #pragma acc loop
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCAutoClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCSharedClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    //  DMP-NOT:     OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang auto
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang auto // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    #pragma acc loop gang auto
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: acc loop gang auto enclosed by orphaned acc loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: acc loop gang auto enclosed by orphaned acc loop: 0, 1
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: acc loop gang auto enclosed by orphaned acc loop: 1, 0
      // EXE-TGT-USE-STDIO-DAG:withinGangFn: acc loop gang auto enclosed by orphaned acc loop: 1, 1
      TGT_PRINTF("withinGangFn: acc loop gang auto enclosed by orphaned acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //----------------------------------------------------------------------------
  // gang, worker, vector, seq function call enclosed.
  //----------------------------------------------------------------------------

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     CallExpr
  //      DMP:       DeclRefExpr {{.*}} 'gangFn'
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   gangFn({{.*}});
  //
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: gang function call enclosed by acc loop: 0
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: gang function call enclosed by acc loop: 0
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: gang function call enclosed by acc loop: 1
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: gang function call enclosed by acc loop: 1
  #pragma acc loop
  for (int i = 0; i < 2; ++i)
    gangFn("withinGangFn: gang function call enclosed by acc loop", i);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //      DMP:         DeclRefExpr {{.*}} 'workerFn'
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   workerFn({{.*}});
  //
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: worker function call enclosed by acc loop: 0
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: worker function call enclosed by acc loop: 1
  #pragma acc loop
  for (int i = 0; i < 2; ++i)
    workerFn("withinGangFn: worker function call enclosed by acc loop", i);

  //          DMP: ACCLoopDirective
  //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
  //   DMP-G-NEXT:   impl: OMPDistributeDirective
  // DMP-GWV-NEXT:   impl: OMPDistributeParallelForDirective
  //      DMP-NOT:     OMP
  //          DMP:     ForStmt
  //          DMP:       CallExpr
  //          DMP:         DeclRefExpr {{.*}} 'vectorFn'
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for{{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for{{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
  //        PRT-NEXT: for ({{.*}})
  //        PRT-NEXT:   vectorFn({{.*}});
  //
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: vector function call enclosed by acc loop: 0
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: vector function call enclosed by acc loop: 1
  #pragma acc loop
  for (int i = 0; i < 2; ++i)
    vectorFn("withinGangFn: vector function call enclosed by acc loop", i);

  //          DMP: ACCLoopDirective
  //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:   impl: OMPDistributeDirective
  // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
  //      DMP-NOT:     OMP
  //          DMP:     ForStmt
  //          DMP:       CallExpr
  //          DMP:         DeclRefExpr {{.*}} 'seqFn'
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
  //        PRT-NEXT: for ({{.*}})
  //        PRT-NEXT:   seqFn({{.*}});
  //
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: seq function call enclosed by acc loop: 0
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: seq function call enclosed by acc loop: 1
  #pragma acc loop
  for (int i = 0; i < 2; ++i)
    seqFn("withinGangFn: seq function call enclosed by acc loop", i);

  //          DMP: ACCLoopDirective
  //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:   impl: OMPDistributeDirective
  // DMP-GWV-NEXT:   impl: OMPDistributeParallelForSimdDirective
  //      DMP-NOT:     OMP
  //          DMP:     ForStmt
  //          DMP:       CallExpr
  //          DMP:         DeclRefExpr {{.*}} 'impFn'
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp distribute parallel for simd{{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp distribute parallel for simd{{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
  //        PRT-NEXT: for ({{.*}})
  //        PRT-NEXT:   impFn({{.*}});
  //
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: implicit seq function call enclosed by acc loop: 0
  // EXE-TGT-USE-STDIO-DAG:withinGangFn: implicit seq function call enclosed by acc loop: 1
  #pragma acc loop
  for (int i = 0; i < 2; ++i)
    impFn("withinGangFn: implicit seq function call enclosed by acc loop", i);
} // PRT-NEXT: }

//==============================================================================
// worker function's loops can have implicit worker and vector clauses.
//
// withinGangFn verifies that implicit gang/worker/vector clause analysis
// basically works for orphaned loops.  withinWorkerFn repeats enough of it to
// be sure that, within worker functions, implicit gang clauses are not added
// but implicit worker and vector clauses are added.
//==============================================================================

// DMP-LABEL: FunctionDecl {{.*}} withinWorkerFn
// PRT-LABEL: void withinWorkerFn() {
#pragma acc routine worker
void withinWorkerFn() {
  //          DMP: ACCLoopDirective
  //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:   impl: ForStmt
  // DMP-GWV-NEXT:   impl: OMPParallelForSimdDirective
  //      DMP-NOT:     OMP
  //      DMP-GWV:     ForStmt
  //          DMP:       CallExpr
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc loop
  //   PRT-AO-G-SAME: {{^}} // discarded in OpenMP translation
  //      PRT-A-SAME: {{^$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp parallel for simd{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp parallel for simd{{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //   PRT-OA-G-SAME: {{^}} // discarded in OpenMP translation
  //     PRT-OA-SAME: {{^$}}
  //        PRT-NEXT: for ({{.*}})
  //        PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop without nesting: 1
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop without nesting: 1
  #pragma acc loop
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinWorkerFn: orphaned acc loop without nesting: %d\n", i);

  //          DMP: ACCLoopDirective
  //     DMP-NEXT:   ACCWorkerClause
  //      DMP-NOT:     <implicit>
  //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:   impl: OMPParallelForDirective
  // DMP-GWV-NEXT:   impl: OMPParallelForSimdDirective
  //      DMP-NOT:     OMP
  //          DMP:     ForStmt
  //          DMP:       CallExpr
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc loop worker{{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp parallel for{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp parallel for simd{{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp parallel for{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp parallel for simd{{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker{{$}}
  //        PRT-NEXT: for ({{.*}})
  //        PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop worker without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop worker without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop worker without nesting: 1
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop worker without nesting: 1
  #pragma acc loop worker
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinWorkerFn: orphaned acc loop worker without nesting: %d\n", i);

  //          DMP: ACCLoopDirective
  //     DMP-NEXT:   ACCVectorClause
  //      DMP-NOT:     <implicit>
  //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCWorkerClause {{.*}} <implicit>
  //   DMP-G-NEXT:   impl: OMPSimdDirective
  // DMP-GWV-NEXT:   impl: OMPParallelForSimdDirective
  //      DMP-NOT:     OMP
  //          DMP:     ForStmt
  //          DMP:       CallExpr
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc loop vector{{$}}
  //   PRT-AO-G-NEXT: {{^ *}}// #pragma omp simd{{$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp parallel for simd{{$}}
  //    PRT-O-G-NEXT: {{^ *}}#pragma omp simd{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp parallel for simd{{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector{{$}}
  //        PRT-NEXT: for ({{.*}})
  //        PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop vector without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop vector without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop vector without nesting: 1
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop vector without nesting: 1
  #pragma acc loop vector
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinWorkerFn: orphaned acc loop vector without nesting: %d\n", i);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCSeqClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop seq without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop seq without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop seq without nesting: 1
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop seq without nesting: 1
  #pragma acc loop seq
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinWorkerFn: orphaned acc loop seq without nesting: %d\n", i);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop auto without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop auto without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop auto without nesting: 1
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop auto without nesting: 1
  #pragma acc loop auto
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinWorkerFn: orphaned acc loop auto without nesting: %d\n", i);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPParallelForSimdDirective
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for simd{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp parallel for simd{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop worker vector without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop worker vector without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop worker vector without nesting: 1
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop worker vector without nesting: 1
  #pragma acc loop worker vector
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinWorkerFn: orphaned acc loop worker vector without nesting: %d\n", i);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker auto
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker auto // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop worker auto without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop worker auto without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop worker auto without nesting: 1
  // EXE-TGT-USE-STDIO-DAG:withinWorkerFn: orphaned acc loop worker auto without nesting: 1
  #pragma acc loop worker auto
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinWorkerFn: orphaned acc loop worker auto without nesting: %d\n", i);
} // PRT-NEXT: }

//==============================================================================
// vector function's loops can have implicit vector clauses.
//
// withinGangFn verifies that implicit gang/worker/vector clause analysis
// basically works for orphaned loops.  withinVectorFn repeats enough of it to
// be sure that, within vector functions, implicit gang and worker clauses are
// not added but implicit vector clauses are added.
//==============================================================================

// DMP-LABEL: FunctionDecl {{.*}} withinVectorFn
// PRT-LABEL: void withinVectorFn() {
#pragma acc routine vector
void withinVectorFn() {
  //          DMP: ACCLoopDirective
  //     DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-GWV-NEXT:   ACCVectorClause {{.*}} <implicit>
  //   DMP-G-NEXT:   impl: ForStmt
  // DMP-GWV-NEXT:   impl: OMPSimdDirective
  //      DMP-NOT:     OMP
  //      DMP-GWV:     ForStmt
  //          DMP:       CallExpr
  //
  //      PRT-A-NEXT: {{^ *}}#pragma acc loop
  //   PRT-AO-G-SAME: {{^}} // discarded in OpenMP translation
  //      PRT-A-SAME: {{^$}}
  // PRT-AO-GWV-NEXT: {{^ *}}// #pragma omp simd{{$}}
  //  PRT-O-GWV-NEXT: {{^ *}}#pragma omp simd{{$}}
  //     PRT-OA-NEXT: {{^ *}}// #pragma acc loop
  //   PRT-OA-G-SAME: {{^}} // discarded in OpenMP translation
  //     PRT-OA-SAME: {{^$}}
  //        PRT-NEXT: for ({{.*}})
  //        PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:withinVectorFn: orphaned acc loop without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinVectorFn: orphaned acc loop without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinVectorFn: orphaned acc loop without nesting: 1
  // EXE-TGT-USE-STDIO-DAG:withinVectorFn: orphaned acc loop without nesting: 1
  #pragma acc loop
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinVectorFn: orphaned acc loop without nesting: %d\n", i);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPSimdDirective
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp simd{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp simd{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:withinVectorFn: orphaned acc loop vector without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinVectorFn: orphaned acc loop vector without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinVectorFn: orphaned acc loop vector without nesting: 1
  // EXE-TGT-USE-STDIO-DAG:withinVectorFn: orphaned acc loop vector without nesting: 1
  #pragma acc loop vector
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinVectorFn: orphaned acc loop vector without nesting: %d\n", i);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCSeqClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:withinVectorFn: orphaned acc loop seq without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinVectorFn: orphaned acc loop seq without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinVectorFn: orphaned acc loop seq without nesting: 1
  // EXE-TGT-USE-STDIO-DAG:withinVectorFn: orphaned acc loop seq without nesting: 1
  #pragma acc loop seq
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinVectorFn: orphaned acc loop seq without nesting: %d\n", i);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:withinVectorFn: orphaned acc loop auto without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinVectorFn: orphaned acc loop auto without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinVectorFn: orphaned acc loop auto without nesting: 1
  // EXE-TGT-USE-STDIO-DAG:withinVectorFn: orphaned acc loop auto without nesting: 1
  #pragma acc loop auto
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinVectorFn: orphaned acc loop auto without nesting: %d\n", i);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector auto
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector auto // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:withinVectorFn: orphaned acc loop vector auto without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinVectorFn: orphaned acc loop vector auto without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinVectorFn: orphaned acc loop vector auto without nesting: 1
  // EXE-TGT-USE-STDIO-DAG:withinVectorFn: orphaned acc loop vector auto without nesting: 1
  #pragma acc loop vector auto
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinVectorFn: orphaned acc loop vector auto without nesting: %d\n", i);
} // PRT-NEXT: }

//==============================================================================
// seq function's loops cannot have implicit gang, worker, or vector clauses.
//
// withinVectorFn repeats enough of withinGangFn to be sure that, within vector
// functions, implicit gang, and worker clauses are not added.
//==============================================================================

// DMP-LABEL: FunctionDecl {{.*}} withinSeqFn
// PRT-LABEL: void withinSeqFn() {
#pragma acc routine seq
void withinSeqFn() {
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:withinSeqFn: orphaned acc loop without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinSeqFn: orphaned acc loop without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinSeqFn: orphaned acc loop without nesting: 1
  // EXE-TGT-USE-STDIO-DAG:withinSeqFn: orphaned acc loop without nesting: 1
  #pragma acc loop
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinSeqFn: orphaned acc loop without nesting: %d\n", i);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCSeqClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:withinSeqFn: orphaned acc loop seq without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinSeqFn: orphaned acc loop seq without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinSeqFn: orphaned acc loop seq without nesting: 1
  // EXE-TGT-USE-STDIO-DAG:withinSeqFn: orphaned acc loop seq without nesting: 1
  #pragma acc loop seq
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinSeqFn: orphaned acc loop seq without nesting: %d\n", i);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG:withinSeqFn: orphaned acc loop auto without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinSeqFn: orphaned acc loop auto without nesting: 0
  // EXE-TGT-USE-STDIO-DAG:withinSeqFn: orphaned acc loop auto without nesting: 1
  // EXE-TGT-USE-STDIO-DAG:withinSeqFn: orphaned acc loop auto without nesting: 1
  #pragma acc loop auto
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinSeqFn: orphaned acc loop auto without nesting: %d\n", i);
} // PRT-NEXT: }
