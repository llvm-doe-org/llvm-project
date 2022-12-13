// Check implicit gang clause on "acc parallel loop", on "acc loop" within
// "acc parallel", and on orphaned "acc loop".
//
// Positive testing for gang reductions appears in loop-reduction.c.  Testing
// for diagnostics about conflicting gang reductions appears in
// diagnostics/loop.c.

// FIXME: amdgcn doesn't yet support printf in a kernel.  Unfortunately, that
// means our execution checks on amdgcn don't verify much except that nothing
// crashes.
//
// REDEFINE: %{exe:fc:args-stable} = -match-full-lines
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}

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
void gangFn(const char *msg, int i) { TGT_PRINTF("%s: %d\n", msg, i); }
#pragma acc routine worker
void workerFn(const char *msg, int i) { TGT_PRINTF("%s: %d\n", msg, i); }
#pragma acc routine vector
void vectorFn(const char *msg, int i) { TGT_PRINTF("%s: %d\n", msg, i); }
#pragma acc routine seq
void seqFn(const char *msg, int i) { TGT_PRINTF("%s: %d\n", msg, i); }
void impFn(const char *msg, int i) { TGT_PRINTF("%s: %d\n", msg, i); }

// These are used to check implicit gang clauses within functions with routine
// directives.
#pragma acc routine gang
void withinGangFn();
#pragma acc routine worker
void withinWorkerFn();
#pragma acc routine vector
void withinVectorFn();
#pragma acc routine seq
void withinSeqFn();

//--------------------------------------------------
// Parallel constructs can contain implicit gang clauses.
//--------------------------------------------------

// PRT: int main() {
int main() {
  //..................................................
  // gang, worker, vector, seq, auto combinations (without loop nesting, which
  // is checked later)
  //..................................................

  // First for acc parallel and acc loop separately.

  // DMP-LABEL: StringLiteral {{.*}} "acc loop clause combinations without nesting\n"
  // PRT-LABEL: printf("acc loop clause combinations without nesting\n");
  // EXE-LABEL: {{^}}acc loop clause combinations without nesting{{$}}
  // EXE-NOT: {{.}}
  printf("acc loop clause combinations without nesting\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //  DMP-NOT:     OMP
  //      DMP:     CompoundStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(2) num_workers(2) vector_length(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2){{$}}
  //    PRT-NEXT: {
  #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2)
  {
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //      DMP:     CallExpr
    //      DMP:       ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop without nesting: 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop without nesting: 1{{$}}
    #pragma acc loop
    for (int i = 0; i < 2; ++i)
      TGT_PRINTF("acc loop without nesting: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //      DMP:     CallExpr
    //      DMP:       ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang without nesting: 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang without nesting: 1{{$}}
    #pragma acc loop gang
    for (int i = 0; i < 2; ++i)
      TGT_PRINTF("acc loop gang without nesting: %d\n", i);

    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForDirective
    // DMP-NEXT:     OMPNum_threadsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //      DMP:       CallExpr
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(2){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for num_threads(2){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop worker without nesting: 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop worker without nesting: 1{{$}}
    #pragma acc loop worker
    for (int i = 0; i < 2; ++i)
      TGT_PRINTF("acc loop worker without nesting: %d\n", i);

    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCVectorClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeSimdDirective
    // DMP-NEXT:     OMPSimdlenClause
    // DMP-NEXT:       ConstantExpr
    // DMP-NEXT:         value: Int 2
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //      DMP:       CallExpr
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd simdlen(2){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute simd simdlen(2){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop vector without nesting: 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop vector without nesting: 1{{$}}
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
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop seq without nesting: 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop seq without nesting: 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop seq without nesting: 1{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop seq without nesting: 1{{$}}
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
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto without nesting: 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto without nesting: 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto without nesting: 1{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto without nesting: 1{{$}}
    #pragma acc loop auto
    for (int i = 0; i < 2; ++i)
      TGT_PRINTF("acc loop auto without nesting: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForDirective
    // DMP-NEXT:     OMPNum_threadsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    //  DMP-NOT:     OMP
    //      DMP:     CallExpr
    //      DMP:       ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang worker{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(2){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for num_threads(2){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang worker{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang worker without nesting: 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang worker without nesting: 1{{$}}
    #pragma acc loop gang worker
    for (int i = 0; i < 2; ++i)
      TGT_PRINTF("acc loop gang worker without nesting: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:   ACCVectorClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeSimdDirective
    // DMP-NEXT:     OMPSimdlenClause
    // DMP-NEXT:       ConstantExpr
    // DMP-NEXT:         value: Int 2
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //  DMP-NOT:     OMP
    //      DMP:     CallExpr
    //      DMP:       ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang vector{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd simdlen(2){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute simd simdlen(2){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang vector{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang vector without nesting: 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang vector without nesting: 1{{$}}
    #pragma acc loop gang vector
    for (int i = 0; i < 2; ++i)
      TGT_PRINTF("acc loop gang vector without nesting: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:   ACCVectorClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-NEXT:     OMPNum_threadsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPSimdlenClause
    // DMP-NEXT:       ConstantExpr
    // DMP-NEXT:         value: Int 2
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //  DMP-NOT:     OMP
    //      DMP:     CallExpr
    //      DMP:       ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(2) simdlen(2){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(2) simdlen(2){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop worker vector without nesting: 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop worker vector without nesting: 1{{$}}
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
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPSimdlenClause
    // DMP-NEXT:       ConstantExpr
    // DMP-NEXT:         value: Int 2
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //  DMP-NOT:     OMP
    //      DMP:     CallExpr
    //      DMP:       ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang worker vector{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(2) simdlen(2){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(2) simdlen(2){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang worker vector{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang worker vector without nesting: 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang worker vector without nesting: 1{{$}}
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
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto gang without nesting: 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto gang without nesting: 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto gang without nesting: 1{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto gang without nesting: 1{{$}}
    #pragma acc loop auto gang
    for (int i = 0; i < 2; ++i)
      TGT_PRINTF("acc loop auto gang without nesting: %d\n", i);
  } // PRT-NEXT: }

  // Repeat for acc parallel loop.

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop without nesting\n"
  // PRT-LABEL: printf("acc parallel loop without nesting\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop without nesting{{$}}
  //   EXE-NOT: {{.}}
  printf("acc parallel loop without nesting\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
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
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2){{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop without nesting: 1{{$}}
  #pragma acc parallel loop num_gangs(2)
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("acc parallel loop without nesting: %d\n", i);

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop gang without nesting\n"
  // PRT-LABEL: printf("acc parallel loop gang without nesting\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop gang without nesting{{$}}
  //   EXE-NOT: {{.}}
  printf("acc parallel loop gang without nesting\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCGangClause
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
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeDirective
  //  DMP-NOT:         OMP
  //      DMP:         ForStmt
  //      DMP:           CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) gang{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) gang{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang without nesting: 1{{$}}
  #pragma acc parallel loop num_gangs(2) gang
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("acc parallel loop gang without nesting: %d\n", i);

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop worker without nesting\n"
  // PRT-LABEL: printf("acc parallel loop worker without nesting\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop worker without nesting{{$}}
  //   EXE-NOT: {{.}}
  printf("acc parallel loop worker without nesting\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCNumWorkersClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCWorkerClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeParallelForDirective
  // DMP-NEXT:         OMPNum_threadsClause
  // DMP-NEXT:           IntegerLiteral {{.*}} 'int' 2
  //  DMP-NOT:         OMP
  //      DMP:         ForStmt
  //      DMP:           CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(2) worker{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for num_threads(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(2) worker{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop worker without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop worker without nesting: 1{{$}}
  #pragma acc parallel loop num_gangs(2) num_workers(2) worker
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("acc parallel loop worker without nesting: %d\n", i);

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop vector without nesting\n"
  // PRT-LABEL: printf("acc parallel loop vector without nesting\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop vector without nesting{{$}}
  //   EXE-NOT: {{.}}
  printf("acc parallel loop vector without nesting\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCVectorClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeSimdDirective
  // DMP-NEXT:         OMPSimdlenClause
  // DMP-NEXT:           ConstantExpr
  // DMP-NEXT:             value: Int 2
  // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 2
  //  DMP-NOT:         OMP
  //      DMP:         ForStmt
  //      DMP:           CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) vector_length(2) vector{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd simdlen(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute simd simdlen(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) vector_length(2) vector{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop vector without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop vector without nesting: 1{{$}}
  #pragma acc parallel loop num_gangs(2) vector_length(2) vector
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("acc parallel loop vector without nesting: %d\n", i);

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop seq without nesting\n"
  // PRT-LABEL: printf("acc parallel loop seq without nesting\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop seq without nesting{{$}}
  //   EXE-NOT: {{.}}
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
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop seq without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop seq without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop seq without nesting: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop seq without nesting: 1{{$}}
  #pragma acc parallel loop num_gangs(2) seq
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("acc parallel loop seq without nesting: %d\n", i);

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop auto without nesting\n"
  // PRT-LABEL: printf("acc parallel loop auto without nesting\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop auto without nesting{{$}}
  //   EXE-NOT: {{.}}
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
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop auto without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop auto without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop auto without nesting: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop auto without nesting: 1{{$}}
  #pragma acc parallel loop num_gangs(2) auto
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("acc parallel loop auto without nesting: %d\n", i);

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop gang worker without nesting\n"
  // PRT-LABEL: printf("acc parallel loop gang worker without nesting\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop gang worker without nesting{{$}}
  //   EXE-NOT: {{.}}
  printf("acc parallel loop gang worker without nesting\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCGangClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCNumWorkersClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCGangClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCWorkerClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeParallelForDirective
  // DMP-NEXT:         OMPNum_threadsClause
  // DMP-NEXT:           IntegerLiteral {{.*}} 'int' 2
  //  DMP-NOT:         OMP
  //      DMP:         ForStmt
  //      DMP:           CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(2) gang worker{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for num_threads(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(2) gang worker{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang worker without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang worker without nesting: 1{{$}}
  #pragma acc parallel loop num_gangs(2) num_workers(2) gang worker
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("acc parallel loop gang worker without nesting: %d\n", i);

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop gang vector without nesting\n"
  // PRT-LABEL: printf("acc parallel loop gang vector without nesting\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop gang vector without nesting{{$}}
  //   EXE-NOT: {{.}}
  printf("acc parallel loop gang vector without nesting\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCGangClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCGangClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCVectorClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeSimdDirective
  // DMP-NEXT:         OMPSimdlenClause
  // DMP-NEXT:           ConstantExpr
  // DMP-NEXT:             value: Int 2
  // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 2
  //  DMP-NOT:         OMP
  //      DMP:         ForStmt
  //      DMP:           CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) vector_length(2) gang vector{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd simdlen(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute simd simdlen(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) vector_length(2) gang vector{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang vector without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang vector without nesting: 1{{$}}
  #pragma acc parallel loop num_gangs(2) vector_length(2) gang vector
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("acc parallel loop gang vector without nesting: %d\n", i);

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop worker vector without nesting\n"
  // PRT-LABEL: printf("acc parallel loop worker vector without nesting\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop worker vector without nesting{{$}}
  //   EXE-NOT: {{.}}
  printf("acc parallel loop worker vector without nesting\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCNumWorkersClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
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
  // DMP-NEXT:           IntegerLiteral {{.*}} 'int' 2
  //  DMP-NOT:         OMP
  // DMP-NEXT:         OMPSimdlenClause
  // DMP-NEXT:           ConstantExpr
  // DMP-NEXT:             value: Int 2
  // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 2
  //      DMP:         ForStmt
  //      DMP:           CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(2) vector_length(2) worker vector{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(2) simdlen(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(2) simdlen(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(2) vector_length(2) worker vector{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop worker vector without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop worker vector without nesting: 1{{$}}
  #pragma acc parallel loop num_gangs(2) num_workers(2) vector_length(2) worker vector
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("acc parallel loop worker vector without nesting: %d\n", i);

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop gang worker vector without nesting\n"
  // PRT-LABEL: printf("acc parallel loop gang worker vector without nesting\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop gang worker vector without nesting{{$}}
  //   EXE-NOT: {{.}}
  printf("acc parallel loop gang worker vector without nesting\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
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
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
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
  // DMP-NEXT:           IntegerLiteral {{.*}} 'int' 2
  //  DMP-NOT:         OMP
  // DMP-NEXT:         OMPSimdlenClause
  // DMP-NEXT:           ConstantExpr
  // DMP-NEXT:             value: Int 2
  // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 2
  //      DMP:         ForStmt
  //      DMP:           CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(2) vector_length(2) gang worker vector{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(2) simdlen(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd num_threads(2) simdlen(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(2) vector_length(2) gang worker vector{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang worker vector without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang worker vector without nesting: 1{{$}}
  #pragma acc parallel loop num_gangs(2) num_workers(2) vector_length(2) gang worker vector
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("acc parallel loop gang worker vector without nesting: %d\n", i);

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop gang auto without nesting\n"
  // PRT-LABEL: printf("acc parallel loop gang auto without nesting\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop gang auto without nesting{{$}}
  //   EXE-NOT: {{.}}
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
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang auto without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang auto without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang auto without nesting: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang auto without nesting: 1{{$}}
  #pragma acc parallel loop num_gangs(2) gang auto
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("acc parallel loop gang auto without nesting: %d\n", i);

  //..................................................
  // gang, worker, vector, seq, auto on enclosing acc loop.
  //..................................................

  // First for acc parallel and acc loop separately.

  // DMP-LABEL: StringLiteral {{.*}} "acc loop clause enclosing acc loop\n"
  // PRT-LABEL: printf("acc loop clause enclosing acc loop\n");
  // EXE-LABEL: {{^}}acc loop clause enclosing acc loop{{$}}
  // EXE-NOT: {{.}}
  printf("acc loop clause enclosing acc loop\n");

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NOT:      OMP
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) num_workers(2) vector_length(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2){{$}}
  #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2)
  // DMP: CompoundStmt
  // PRT-NEXT: {
  {
    // DMP:      ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    // DMP-NOT:      OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
    #pragma acc loop gang
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
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang enclosing acc loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang enclosing acc loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang enclosing acc loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang enclosing acc loop: 1, 1{{$}}
        TGT_PRINTF("acc loop gang enclosing acc loop: %d, %d\n", i, j);
    } // PRT-NEXT: }

    // DMP-NOT:  OMP
    // DMP:      ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForDirective
    // DMP-NEXT:     OMPNum_threadsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NOT:      OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(2){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute parallel for num_threads(2){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker{{$}}
    #pragma acc loop worker
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}})
    for (int i = 0; i < 2; ++i)
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
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop worker enclosing acc loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop worker enclosing acc loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop worker enclosing acc loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop worker enclosing acc loop: 1, 1{{$}}
        TGT_PRINTF("acc loop worker enclosing acc loop: %d, %d\n", i, j);

    // DMP-NOT:  OMP
    // DMP:      ACCLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeSimdDirective
    // DMP-NEXT:     OMPSimdlenClause
    // DMP-NEXT:       ConstantExpr
    // DMP-NEXT:         value: Int 2
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NOT:      OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop vector{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd simdlen(2){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute simd simdlen(2){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector{{$}}
    #pragma acc loop vector
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}})
    for (int i = 0; i < 2; ++i)
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
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop vector enclosing acc loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop vector enclosing acc loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop vector enclosing acc loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop vector enclosing acc loop: 1, 1{{$}}
        TGT_PRINTF("acc loop vector enclosing acc loop: %d, %d\n", i, j);

    // DMP-NOT:  OMP
    // DMP:      ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:   impl: ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq // discarded in OpenMP translation{{$}}
    //
    // PRT-NEXT:    for ({{.*}}) {
    #pragma acc loop seq
    for (int i = 0; i < 2; ++i) {
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      //  DMP-NOT:     OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
      #pragma acc loop
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop seq enclosing acc loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop seq enclosing acc loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop seq enclosing acc loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop seq enclosing acc loop: 1, 1{{$}}
        TGT_PRINTF("acc loop seq enclosing acc loop: %d, %d\n", i, j);
    } // PRT-NEXT: }

    // DMP-NOT:  OMP
    // DMP:      ACCLoopDirective
    // DMP-NEXT:   ACCAutoClause
    // DMP-NOT:      <implicit>
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
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      //  DMP-NOT:     OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
      #pragma acc loop
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto enclosing acc loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto enclosing acc loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto enclosing acc loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto enclosing acc loop: 1, 1{{$}}
        TGT_PRINTF("acc loop auto enclosing acc loop: %d, %d\n", i, j);
    } // PRT-NEXT: }

    // DMP-NOT:  OMP
    // DMP:      ACCLoopDirective
    // DMP-NEXT:   ACCAutoClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:   ACCGangClause
    // DMP-NOT:      <implicit>
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
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      //  DMP-NOT:     OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
      #pragma acc loop
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto gang enclosing acc loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto gang enclosing acc loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto gang enclosing acc loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto gang enclosing acc loop: 1, 1{{$}}
        TGT_PRINTF("acc loop auto gang enclosing acc loop: %d, %d\n", i, j);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // Repeat for acc parallel loop.

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop gang enclosing acc loop\n"
  // PRT-LABEL: printf("acc parallel loop gang enclosing acc loop\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop gang enclosing acc loop{{$}}
  // EXE-NOT: {{.}}
  printf("acc parallel loop gang enclosing acc loop\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCGangClause
  // DMP-NOT:      <implicit>
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCGangClause
  // DMP-NOT:          <implicit>
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeDirective
  // DMP-NOT:          OMP
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) gang{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) gang{{$}}
  #pragma acc parallel loop num_gangs(2) gang
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
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang enclosing acc loop: 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang enclosing acc loop: 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang enclosing acc loop: 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang enclosing acc loop: 1, 1{{$}}
      TGT_PRINTF("acc parallel loop gang enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop worker enclosing acc loop\n"
  // PRT-LABEL: printf("acc parallel loop worker enclosing acc loop\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop worker enclosing acc loop{{$}}
  // EXE-NOT: {{.}}
  printf("acc parallel loop worker enclosing acc loop\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCWorkerClause
  // DMP-NOT:      <implicit>
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCNumWorkersClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCWorkerClause
  // DMP-NOT:          <implicit>
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeParallelForDirective
  // DMP-NEXT:         OMPNum_threadsClause
  // DMP-NEXT:           IntegerLiteral {{.*}} 'int' 2
  // DMP-NOT:          OMP
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(2) worker{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute parallel for num_threads(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(2) worker{{$}}
  #pragma acc parallel loop num_gangs(2) num_workers(2) worker
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
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop worker enclosing acc loop: 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop worker enclosing acc loop: 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop worker enclosing acc loop: 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop worker enclosing acc loop: 1, 1{{$}}
      TGT_PRINTF("acc parallel loop worker enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop vector enclosing acc loop\n"
  // PRT-LABEL: printf("acc parallel loop vector enclosing acc loop\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop vector enclosing acc loop{{$}}
  // EXE-NOT: {{.}}
  printf("acc parallel loop vector enclosing acc loop\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCVectorClause
  // DMP-NOT:      <implicit>
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCVectorClause
  // DMP-NOT:          <implicit>
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeSimdDirective
  // DMP-NEXT:         OMPSimdlenClause
  // DMP-NEXT:           ConstantExpr
  // DMP-NEXT:             value: Int 2
  // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 2
  // DMP-NOT:          OMP
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) vector_length(2) vector{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd simdlen(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute simd simdlen(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) vector_length(2) vector{{$}}
  #pragma acc parallel loop num_gangs(2) vector_length(2) vector
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
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop vector enclosing acc loop: 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop vector enclosing acc loop: 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop vector enclosing acc loop: 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop vector enclosing acc loop: 1, 1{{$}}
      TGT_PRINTF("acc parallel loop vector enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop seq enclosing acc loop\n"
  // PRT-LABEL: printf("acc parallel loop seq enclosing acc loop\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop seq enclosing acc loop{{$}}
  // EXE-NOT: {{.}}
  printf("acc parallel loop seq enclosing acc loop\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCSeqClause
  // DMP-NOT:      <implicit>
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCSeqClause
  // DMP-NOT:          <implicit>
  // DMP-NEXT:       impl: ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) seq{{$}}
  //
  // PRT-NEXT: for ({{.*}}) {
  #pragma acc parallel loop num_gangs(2) seq
  for (int i = 0; i < 2; ++i) {
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    //
    // PRT-NEXT:    for ({{.*}})
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop seq enclosing acc loop: 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop seq enclosing acc loop: 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop seq enclosing acc loop: 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop seq enclosing acc loop: 1, 1{{$}}
      TGT_PRINTF("acc parallel loop seq enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop auto enclosing acc loop\n"
  // PRT-LABEL: printf("acc parallel loop auto enclosing acc loop\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop auto enclosing acc loop{{$}}
  // EXE-NOT: {{.}}
  printf("acc parallel loop auto enclosing acc loop\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCAutoClause
  // DMP-NOT:      <implicit>
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCAutoClause
  // DMP-NOT:          <implicit>
  // DMP-NEXT:       impl: ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) auto{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) auto{{$}}
  //
  // PRT-NEXT: for ({{.*}}) {
  #pragma acc parallel loop num_gangs(2) auto
  for (int i = 0; i < 2; ++i) {
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    //
    // PRT-NEXT:    for ({{.*}})
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop auto enclosing acc loop: 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop auto enclosing acc loop: 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop auto enclosing acc loop: 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop auto enclosing acc loop: 1, 1{{$}}
      TGT_PRINTF("acc parallel loop auto enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop gang auto enclosing acc loop\n"
  // PRT-LABEL: printf("acc parallel loop gang auto enclosing acc loop\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop gang auto enclosing acc loop{{$}}
  // EXE-NOT: {{.}}
  printf("acc parallel loop gang auto enclosing acc loop\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCGangClause
  // DMP-NOT:      <implicit>
  // DMP-NEXT:   ACCAutoClause
  // DMP-NOT:      <implicit>
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCGangClause
  // DMP-NOT:          <implicit>
  // DMP-NEXT:       ACCAutoClause
  // DMP-NOT:          <implicit>
  // DMP-NEXT:       impl: ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) gang auto{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) gang auto{{$}}
  //
  // PRT-NEXT: for ({{.*}}) {
  #pragma acc parallel loop num_gangs(2) gang auto
  for (int i = 0; i < 2; ++i) {
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    //
    // PRT-NEXT:    for ({{.*}})
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang auto enclosing acc loop: 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang auto enclosing acc loop: 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang auto enclosing acc loop: 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang auto enclosing acc loop: 1, 1{{$}}
      TGT_PRINTF("acc parallel loop gang auto enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //..................................................
  // gang, worker, vector, seq, auto on enclosed acc loop.
  //..................................................

  // First for acc parallel and acc loop separately.

  // DMP-LABEL: StringLiteral {{.*}} "acc loop clause enclosed by acc loop\n"
  // PRT-LABEL: printf("acc loop clause enclosed by acc loop\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc loop clause enclosed by acc loop{{$}}
  // EXE-NOT: {{.}}
  printf("acc loop clause enclosed by acc loop\n");

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NOT:      OMP
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) num_workers(2) vector_length(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2){{$}}
  #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2)
  // DMP: CompoundStmt
  // PRT-NEXT: {
  {
    // DMP:      ACCLoopDirective
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
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: OMPDistributeDirective
      //  DMP-NOT:     OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
      #pragma acc loop gang
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang enclosed by acc loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang enclosed by acc loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang enclosed by acc loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang enclosed by acc loop: 1, 1{{$}}
        TGT_PRINTF("acc loop gang enclosed by acc loop: %d, %d\n", i, j);
    } // PRT-NEXT: }

    // DMP:      ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    // DMP-NOT:      OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    #pragma acc loop
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCWorkerClause
      //  DMP-NOT:     <implicit>
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: OMPParallelForDirective
      // DMP-NEXT:     OMPNum_threadsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
      //  DMP-NOT:     OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for num_threads(2){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for num_threads(2){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker{{$}}
      #pragma acc loop worker
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop worker enclosed by acc loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop worker enclosed by acc loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop worker enclosed by acc loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop worker enclosed by acc loop: 1, 1{{$}}
        TGT_PRINTF("acc loop worker enclosed by acc loop: %d, %d\n", i, j);
    } // PRT-NEXT: }

    // DMP:      ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    // DMP-NOT:      OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
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
      // DMP-NEXT:         value: Int 2
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
      //  DMP-NOT:     OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop vector{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp simd simdlen(2){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp simd simdlen(2){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector{{$}}
      #pragma acc loop vector
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop vector enclosed by acc loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop vector enclosed by acc loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop vector enclosed by acc loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop vector enclosed by acc loop: 1, 1{{$}}
        TGT_PRINTF("acc loop vector enclosed by acc loop: %d, %d\n", i, j);
    } // PRT-NEXT: }

    // DMP:      ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    // DMP-NOT:      OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    #pragma acc loop
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      // DMP-NOT:  OMP
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCSeqClause
      // DMP-NOT:      <implicit>
      // DMP-NEXT:   ACCSharedClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: ForStmt
      // DMP-NOT:      OMP
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
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop seq enclosed by acc loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop seq enclosed by acc loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop seq enclosed by acc loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop seq enclosed by acc loop: 1, 1{{$}}
        TGT_PRINTF("acc loop seq enclosed by acc loop: %d, %d\n", i, j);
    } // PRT-NEXT: }

    // DMP:      ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    // DMP-NOT:      OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    #pragma acc loop
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      // DMP-NOT:  OMP
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCAutoClause
      // DMP-NOT:      <implicit>
      // DMP-NEXT:   ACCSharedClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: ForStmt
      // DMP-NOT:      OMP
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      //  PRT-A-SAME: {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto // discarded in OpenMP translation{{$}}
      //
      // PRT-NEXT: for ({{.*}})
      #pragma acc loop auto
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto enclosed by acc loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto enclosed by acc loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto enclosed by acc loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto enclosed by acc loop: 1, 1{{$}}
        TGT_PRINTF("acc loop auto enclosed by acc loop: %d, %d\n", i, j);
    } // PRT-NEXT: }

    // DMP:      ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    // DMP-NOT:      OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    #pragma acc loop
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      // DMP-NOT:  OMP
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NOT:      <implicit>
      // DMP-NEXT:   ACCAutoClause
      // DMP-NOT:      <implicit>
      // DMP-NEXT:   ACCSharedClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: ForStmt
      // DMP-NOT:      OMP
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang auto
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      //  PRT-A-SAME: {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang auto // discarded in OpenMP translation{{$}}
      //
      // PRT-NEXT: for ({{.*}})
      #pragma acc loop gang auto
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang auto enclosed by acc loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang auto enclosed by acc loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang auto enclosed by acc loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang auto enclosed by acc loop: 1, 1{{$}}
        TGT_PRINTF("acc loop gang auto enclosed by acc loop: %d, %d\n", i, j);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // Repeat for acc parallel loop.

  // DMP-LABEL: StringLiteral {{.*}} "acc loop gang enclosed by acc parallel loop\n"
  // PRT-LABEL: printf("acc loop gang enclosed by acc parallel loop\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc loop gang enclosed by acc parallel loop{{$}}
  // EXE-NOT: {{.}}
  printf("acc loop gang enclosed by acc parallel loop\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       impl: ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2){{$}}
  //
  // PRT-NEXT:    for ({{.*}}) {
  #pragma acc parallel loop num_gangs(2)
  for (int i = 0; i < 2; ++i) {
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
    #pragma acc loop gang
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang enclosed by acc parallel loop: 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang enclosed by acc parallel loop: 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang enclosed by acc parallel loop: 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang enclosed by acc parallel loop: 1, 1{{$}}
      TGT_PRINTF("acc loop gang enclosed by acc parallel loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "acc loop worker enclosed by acc parallel loop\n"
  // PRT-LABEL: printf("acc loop worker enclosed by acc parallel loop\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc loop worker enclosed by acc parallel loop{{$}}
  // EXE-NOT: {{.}}
  printf("acc loop worker enclosed by acc parallel loop\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCNumWorkersClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeDirective
  // DMP-NOT:          OMP
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(2){{$}}
  #pragma acc parallel loop num_gangs(2) num_workers(2)
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: OMPParallelForDirective
    // DMP-NEXT:     OMPNum_threadsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    //  DMP-NOT:     OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for num_threads(2){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for num_threads(2){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker{{$}}
    #pragma acc loop worker
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop worker enclosed by acc parallel loop: 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop worker enclosed by acc parallel loop: 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop worker enclosed by acc parallel loop: 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop worker enclosed by acc parallel loop: 1, 1{{$}}
      TGT_PRINTF("acc loop worker enclosed by acc parallel loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "acc loop vector enclosed by acc parallel loop\n"
  // PRT-LABEL: printf("acc loop vector enclosed by acc parallel loop\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc loop vector enclosed by acc parallel loop{{$}}
  // EXE-NOT: {{.}}
  printf("acc loop vector enclosed by acc parallel loop\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeDirective
  // DMP-NOT:          OMP
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) vector_length(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) vector_length(2){{$}}
  #pragma acc parallel loop num_gangs(2) vector_length(2)
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
    // DMP-NEXT:         value: Int 2
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //  DMP-NOT:     OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop vector{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp simd simdlen(2){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp simd simdlen(2){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector{{$}}
    #pragma acc loop vector
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop vector enclosed by acc parallel loop: 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop vector enclosed by acc parallel loop: 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop vector enclosed by acc parallel loop: 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop vector enclosed by acc parallel loop: 1, 1{{$}}
      TGT_PRINTF("acc loop vector enclosed by acc parallel loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "acc loop seq enclosed by acc parallel loop\n"
  // PRT-LABEL: printf("acc loop seq enclosed by acc parallel loop\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc loop seq enclosed by acc parallel loop{{$}}
  // EXE-NOT: {{.}}
  printf("acc loop seq enclosed by acc parallel loop\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeDirective
  // DMP-NOT:          OMP
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2){{$}}
  #pragma acc parallel loop num_gangs(2)
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    // DMP-NOT:  OMP
    // DMP:      ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:   ACCSharedClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    // DMP-NOT:      OMP
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
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop seq enclosed by acc parallel loop: 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop seq enclosed by acc parallel loop: 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop seq enclosed by acc parallel loop: 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop seq enclosed by acc parallel loop: 1, 1{{$}}
      TGT_PRINTF("acc loop seq enclosed by acc parallel loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "acc loop auto enclosed by acc parallel loop\n"
  // PRT-LABEL: printf("acc loop auto enclosed by acc parallel loop\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc loop auto enclosed by acc parallel loop{{$}}
  // EXE-NOT: {{.}}
  printf("acc loop auto enclosed by acc parallel loop\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeDirective
  // DMP-NOT:          OMP
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2){{$}}
  #pragma acc parallel loop num_gangs(2)
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    // DMP-NOT:  OMP
    // DMP:      ACCLoopDirective
    // DMP-NEXT:   ACCAutoClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:   ACCSharedClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    // DMP-NOT:      OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto // discarded in OpenMP translation{{$}}
    //
    // PRT-NEXT: for ({{.*}})
    #pragma acc loop auto
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto enclosed by acc parallel loop: 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto enclosed by acc parallel loop: 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto enclosed by acc parallel loop: 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto enclosed by acc parallel loop: 1, 1{{$}}
      TGT_PRINTF("acc loop auto enclosed by acc parallel loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "acc loop gang auto enclosed by acc parallel loop\n"
  // PRT-LABEL: printf("acc loop gang auto enclosed by acc parallel loop\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc loop gang auto enclosed by acc parallel loop{{$}}
  // EXE-NOT: {{.}}
  printf("acc loop gang auto enclosed by acc parallel loop\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeDirective
  // DMP-NOT:          OMP
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2){{$}}
  #pragma acc parallel loop num_gangs(2)
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    // DMP-NOT:  OMP
    // DMP:      ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:   ACCAutoClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:   ACCSharedClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    // DMP-NOT:      OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang auto
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang auto // discarded in OpenMP translation{{$}}
    //
    // PRT-NEXT: for ({{.*}})
    #pragma acc loop gang auto
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang auto enclosed by acc parallel loop: 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang auto enclosed by acc parallel loop: 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang auto enclosed by acc parallel loop: 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang auto enclosed by acc parallel loop: 1, 1{{$}}
      TGT_PRINTF("acc loop gang auto enclosed by acc parallel loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //..................................................
  // gang, worker, vector, seq function call enclosed.
  //..................................................

  // First for acc parallel and acc loop separately.

  // DMP-LABEL: StringLiteral {{.*}} "function call enclosed by acc loop\n"
  // PRT-LABEL: printf("function call enclosed by acc loop\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}function call enclosed by acc loop{{$}}
  // EXE-NOT: {{.}}
  printf("function call enclosed by acc loop\n");

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NOT:      OMP
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) num_workers(2) vector_length(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2){{$}}
  #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2)
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
    // EXE-TGT-USE-STDIO-DAG: {{^}}gang function call enclosed by acc loop: 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}gang function call enclosed by acc loop: 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}gang function call enclosed by acc loop: 1{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}gang function call enclosed by acc loop: 1{{$}}
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
    // EXE-TGT-USE-STDIO-DAG: {{^}}worker function call enclosed by acc loop: 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}worker function call enclosed by acc loop: 1{{$}}
    #pragma acc loop
    for (int i = 0; i < 2; ++i)
      workerFn("worker function call enclosed by acc loop", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //      DMP:       CallExpr
    //      DMP:         DeclRefExpr {{.*}} 'vectorFn'
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   vectorFn({{.*}});
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}vector function call enclosed by acc loop: 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}vector function call enclosed by acc loop: 1{{$}}
    #pragma acc loop
    for (int i = 0; i < 2; ++i)
      vectorFn("vector function call enclosed by acc loop", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //      DMP:       CallExpr
    //      DMP:         DeclRefExpr {{.*}} 'seqFn'
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   seqFn({{.*}});
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}seq function call enclosed by acc loop: 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}seq function call enclosed by acc loop: 1{{$}}
    #pragma acc loop
    for (int i = 0; i < 2; ++i)
      seqFn("seq function call enclosed by acc loop", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //      DMP:       CallExpr
    //      DMP:         DeclRefExpr {{.*}} 'impFn'
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   impFn({{.*}});
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}implicit seq function call enclosed by acc loop: 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}implicit seq function call enclosed by acc loop: 1{{$}}
    #pragma acc loop
    for (int i = 0; i < 2; ++i)
      impFn("implicit seq function call enclosed by acc loop", i);
  } // PRT-NEXT: }

  // Repeat for acc parallel loop.

  // DMP-LABEL: StringLiteral {{.*}} "gang function call enclosed by acc parallel loop\n"
  // PRT-LABEL: printf("gang function call enclosed by acc parallel loop\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}gang function call enclosed by acc parallel loop{{$}}
  //   EXE-NOT: {{.}}
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
  // EXE-TGT-USE-STDIO-DAG: {{^}}gang function call enclosed by acc parallel loop: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}gang function call enclosed by acc parallel loop: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}gang function call enclosed by acc parallel loop: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}gang function call enclosed by acc parallel loop: 1{{$}}
  #pragma acc parallel loop num_gangs(2)
  for (int i = 0; i < 2; ++i)
    gangFn("gang function call enclosed by acc parallel loop", i);

  // DMP-LABEL: StringLiteral {{.*}} "worker function call enclosed by acc parallel loop\n"
  // PRT-LABEL: printf("worker function call enclosed by acc parallel loop\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}worker function call enclosed by acc parallel loop{{$}}
  //   EXE-NOT: {{.}}
  printf("worker function call enclosed by acc parallel loop\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCNumWorkersClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
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
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) num_workers(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) num_workers(2){{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   workerFn({{.*}});
  //
  // EXE-TGT-USE-STDIO-DAG: {{^}}worker function call enclosed by acc parallel loop: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}worker function call enclosed by acc parallel loop: 1{{$}}
  #pragma acc parallel loop num_gangs(2) num_workers(2)
  for (int i = 0; i < 2; ++i)
    workerFn("worker function call enclosed by acc parallel loop", i);

  // DMP-LABEL: StringLiteral {{.*}} "vector function call enclosed by acc parallel loop\n"
  // PRT-LABEL: printf("vector function call enclosed by acc parallel loop\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}vector function call enclosed by acc parallel loop{{$}}
  //   EXE-NOT: {{.}}
  printf("vector function call enclosed by acc parallel loop\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
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
  //      DMP:             DeclRefExpr {{.*}} 'vectorFn'
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) vector_length(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) vector_length(2){{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   vectorFn({{.*}});
  //
  // EXE-TGT-USE-STDIO-DAG: {{^}}vector function call enclosed by acc parallel loop: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}vector function call enclosed by acc parallel loop: 1{{$}}
  #pragma acc parallel loop num_gangs(2) vector_length(2)
  for (int i = 0; i < 2; ++i)
    vectorFn("vector function call enclosed by acc parallel loop", i);

  // DMP-LABEL: StringLiteral {{.*}} "seq function call enclosed by acc parallel loop\n"
  // PRT-LABEL: printf("seq function call enclosed by acc parallel loop\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}seq function call enclosed by acc parallel loop{{$}}
  //   EXE-NOT: {{.}}
  printf("seq function call enclosed by acc parallel loop\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
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
  //      DMP:             DeclRefExpr {{.*}} 'seqFn'
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2){{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   seqFn({{.*}});
  //
  // EXE-TGT-USE-STDIO-DAG: {{^}}seq function call enclosed by acc parallel loop: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}seq function call enclosed by acc parallel loop: 1{{$}}
  #pragma acc parallel loop num_gangs(2)
  for (int i = 0; i < 2; ++i)
    seqFn("seq function call enclosed by acc parallel loop", i);

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
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
  //      DMP:             DeclRefExpr {{.*}} 'impFn'
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2){{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   impFn({{.*}});
  //
  // EXE-TGT-USE-STDIO-DAG: {{^}}implicit seq function call enclosed by acc parallel loop: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}implicit seq function call enclosed by acc parallel loop: 1{{$}}
  #pragma acc parallel loop num_gangs(2)
  for (int i = 0; i < 2; ++i)
    impFn("implicit seq function call enclosed by acc parallel loop", i);

  //..................................................
  // Multiple levels are gang clause candidates.
  //..................................................

  // DMP-LABEL: StringLiteral {{.*}} "multilevel, separate\n"
  // PRT-LABEL: printf("multilevel, separate\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}multilevel, separate{{$}}
  // EXE-NOT: {{.}}
  printf("multilevel, separate\n");

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2){{$}}
  #pragma acc parallel num_gangs(2)
  // DMP:      ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  // DMP-NOT:      OMP
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc loop{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
  #pragma acc loop
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    // DMP-NOT:  OMP
    // DMP:      ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    // DMP-NOT:      OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
    //
    // PRT-NEXT: for ({{.*}}) {
    #pragma acc loop
    for (int j = 0; j < 2; ++j) {
      // DMP-NOT:  OMP
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
      // DMP-NEXT:   impl: ForStmt
      // DMP-NOT:      OMP
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      //  PRT-A-SAME: {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
      //
      // PRT-NEXT: for ({{.*}})
      #pragma acc loop
      for (int k = 0; k < 2; ++k)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}multilevel, separate: 0, 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}multilevel, separate: 0, 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}multilevel, separate: 0, 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}multilevel, separate: 0, 1, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}multilevel, separate: 1, 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}multilevel, separate: 1, 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}multilevel, separate: 1, 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}multilevel, separate: 1, 1, 1{{$}}
        TGT_PRINTF("multilevel, separate: %d, %d, %d\n", i, j, k);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeDirective
  // DMP-NOT:          OMP
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2){{$}}
  #pragma acc parallel loop num_gangs(2)
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    // DMP-NOT:  OMP
    // DMP:      ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    // DMP-NOT:      OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
    //
    // PRT-NEXT: for ({{.*}}) {
    #pragma acc loop
    for (int j = 0; j < 2; ++j) {
      // DMP-NOT:  OMP
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
      // DMP-NEXT:   impl: ForStmt
      // DMP-NOT:      OMP
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      //  PRT-A-SAME: {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
      //
      // PRT-NEXT: for ({{.*}})
      #pragma acc loop
      for (int k = 0; k < 2; ++k)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}multilevel, separate: 0, 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}multilevel, separate: 0, 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}multilevel, separate: 0, 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}multilevel, separate: 0, 1, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}multilevel, separate: 1, 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}multilevel, separate: 1, 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}multilevel, separate: 1, 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}multilevel, separate: 1, 1, 1{{$}}
        TGT_PRINTF("multilevel, separate: %d, %d, %d\n", i, j, k);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  //..................................................
  // gang clause effect on sibling loop via parent loop.
  //..................................................

  // DMP-LABEL: StringLiteral {{.*}} "siblings, separate, gang loop first\n"
  // PRT-LABEL: printf("siblings, separate, gang loop first\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}siblings, separate, gang loop first{{$}}
  // EXE-NOT: {{.}}
  printf("siblings, separate, gang loop first\n");

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2){{$}}
  #pragma acc parallel num_gangs(2)
  // DMP:      ACCLoopDirective
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
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      //  DMP-NOT:     <implicit>
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: OMPDistributeDirective
      //  DMP-NOT:     OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
      #pragma acc loop gang
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang loop first, first loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang loop first, first loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang loop first, first loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang loop first, first loop: 1, 1{{$}}
        TGT_PRINTF("siblings, separate, gang loop first, first loop: %d, %d\n",
                   i, j);

      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      //  DMP-NOT:     OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
      #pragma acc loop
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang loop first, second loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang loop first, second loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang loop first, second loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang loop first, second loop: 1, 1{{$}}
        TGT_PRINTF("siblings, separate, gang loop first, second loop: %d, %d\n",
                   i, j);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "siblings, combined, gang loop first\n"
  // PRT-LABEL: printf("siblings, combined, gang loop first\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}siblings, combined, gang loop first{{$}}
  // EXE-NOT: {{.}}
  printf("siblings, combined, gang loop first\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       impl: ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2){{$}}
  //
  // PRT-NEXT:    for ({{.*}}) {
  #pragma acc parallel loop num_gangs(2)
  for (int i = 0; i < 2; ++i) {
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      //  DMP-NOT:     <implicit>
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: OMPDistributeDirective
      //  DMP-NOT:     OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
      #pragma acc loop gang
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang loop first, first loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang loop first, first loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang loop first, first loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang loop first, first loop: 1, 1{{$}}
        TGT_PRINTF("siblings, combined, gang loop first, first loop: %d, %d\n",
                   i, j);

      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      //  DMP-NOT:     OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
      #pragma acc loop
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang loop first, second loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang loop first, second loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang loop first, second loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang loop first, second loop: 1, 1{{$}}
        TGT_PRINTF("siblings, combined, gang loop first, second loop: %d, %d\n",
                   i, j);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "siblings, separate, gang function first\n"
  // PRT-LABEL: printf("siblings, separate, gang function first\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}siblings, separate, gang function first{{$}}
  // EXE-NOT: {{.}}
  printf("siblings, separate, gang function first\n");

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2){{$}}
  #pragma acc parallel num_gangs(2)
  // DMP:      ACCLoopDirective
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
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // DMP: CallExpr
      // DMP:   DeclRefExpr {{.*}} 'gangFn'
      // PRT-NEXT: gangFn({{.*}});
      // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang function first, function: 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang function first, function: 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang function first, function: 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang function first, function: 1{{$}}
      gangFn("siblings, separate, gang function first, function", i);

      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      //  DMP-NOT:     OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
      #pragma acc loop
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang function first, loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang function first, loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang function first, loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang function first, loop: 1, 1{{$}}
        TGT_PRINTF("siblings, separate, gang function first, loop: %d, %d\n",
                   i, j);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "siblings, combined, gang function first\n"
  // PRT-LABEL: printf("siblings, combined, gang function first\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}siblings, combined, gang function first{{$}}
  // EXE-NOT: {{.}}
  printf("siblings, combined, gang function first\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       impl: ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2){{$}}
  //
  // PRT-NEXT:    for ({{.*}}) {
  #pragma acc parallel loop num_gangs(2)
  for (int i = 0; i < 2; ++i) {
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // DMP: CallExpr
      // DMP:   DeclRefExpr {{.*}} 'gangFn'
      // PRT-NEXT: gangFn({{.*}});
      // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang function first, function: 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang function first, function: 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang function first, function: 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang function first, function: 1{{$}}
      gangFn("siblings, combined, gang function first, function", i);

      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      //  DMP-NOT:     OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
      #pragma acc loop
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang function first, loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang function first, loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang function first, loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang function first, loop: 1, 1{{$}}
        TGT_PRINTF("siblings, combined, gang function first, loop: %d, %d\n",
                   i, j);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "siblings, separate, gang loop second\n"
  // PRT-LABEL: printf("siblings, separate, gang loop second\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}siblings, separate, gang loop second{{$}}
  // EXE-NOT: {{.}}
  printf("siblings, separate, gang loop second\n");

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2){{$}}
  #pragma acc parallel num_gangs(2)
  // DMP:      ACCLoopDirective
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
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      //  DMP-NOT:     OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
      #pragma acc loop
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang loop second, first loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang loop second, first loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang loop second, first loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang loop second, first loop: 1, 1{{$}}
        TGT_PRINTF("siblings, separate, gang loop second, first loop: %d, %d\n",
                   i, j);

      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      //  DMP-NOT:     <implicit>
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: OMPDistributeDirective
      //  DMP-NOT:     OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
      #pragma acc loop gang
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang loop second, second loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang loop second, second loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang loop second, second loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang loop second, second loop: 1, 1{{$}}
        TGT_PRINTF("siblings, separate, gang loop second, second loop: %d, %d\n",
                   i, j);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "siblings, combined, gang loop second\n"
  // PRT-LABEL: printf("siblings, combined, gang loop second\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}siblings, combined, gang loop second{{$}}
  // EXE-NOT: {{.}}
  printf("siblings, combined, gang loop second\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       impl: ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2){{$}}
  //
  // PRT-NEXT:    for ({{.*}}) {
  #pragma acc parallel loop num_gangs(2)
  for (int i = 0; i < 2; ++i) {
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      //  DMP-NOT:     OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
      #pragma acc loop
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang loop second, first loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang loop second, first loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang loop second, first loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang loop second, first loop: 1, 1{{$}}
        TGT_PRINTF("siblings, combined, gang loop second, first loop: %d, %d\n",
                   i, j);

      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      //  DMP-NOT:     <implicit>
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: OMPDistributeDirective
      //  DMP-NOT:     OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
      #pragma acc loop gang
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang loop second, second loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang loop second, second loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang loop second, second loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang loop second, second loop: 1, 1{{$}}
        TGT_PRINTF("siblings, combined, gang loop second, second loop: %d, %d\n",
                   i, j);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "siblings, separate, gang function second\n"
  // PRT-LABEL: printf("siblings, separate, gang function second\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}siblings, separate, gang function second{{$}}
  // EXE-NOT: {{.}}
  printf("siblings, separate, gang function second\n");

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2){{$}}
  #pragma acc parallel num_gangs(2)
  // DMP:      ACCLoopDirective
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
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      //  DMP-NOT:     OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
      #pragma acc loop
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang function second, loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang function second, loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang function second, loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang function second, loop: 1, 1{{$}}
        TGT_PRINTF("siblings, separate, gang function second, loop: %d, %d\n",
                   i, j);

      // DMP: CallExpr
      // DMP:   DeclRefExpr {{.*}} 'gangFn'
      // PRT-NEXT: gangFn({{.*}});
      // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang function second, function: 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang function second, function: 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang function second, function: 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang function second, function: 1{{$}}
      gangFn("siblings, separate, gang function second, function", i);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "siblings, combined, gang function second\n"
  // PRT-LABEL: printf("siblings, combined, gang function second\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}siblings, combined, gang function second{{$}}
  // EXE-NOT: {{.}}
  printf("siblings, combined, gang function second\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       impl: ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2){{$}}
  //
  // PRT-NEXT:    for ({{.*}}) {
  #pragma acc parallel loop num_gangs(2)
  for (int i = 0; i < 2; ++i) {
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      //  DMP-NOT:     OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
      #pragma acc loop
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}})
      for (int j = 0; j < 2; ++j)
        // DMP: CallExpr
        // PRT-NEXT: {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang function second, loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang function second, loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang function second, loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang function second, loop: 1, 1{{$}}
        TGT_PRINTF("siblings, combined, gang function second, loop: %d, %d\n",
                   i, j);

      // DMP: CallExpr
      // DMP:   DeclRefExpr {{.*}} 'gangFn'
      // PRT-NEXT: gangFn({{.*}});
      // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang function second, function: 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang function second, function: 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang function second, function: 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang function second, function: 1{{$}}
      gangFn("siblings, combined, gang function second, function", i);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  //..................................................
  // Check implicit gang clauses within accelerator routines.
  //..................................................

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2){{$}}
  //    PRT-NEXT: {
  //    PRT-NEXT:   withinGangFn();
  //    PRT-NEXT:   withinWorkerFn();
  //    PRT-NEXT:   withinVectorFn();
  //    PRT-NEXT:   withinSeqFn();
  //    PRT-NEXT: }
  #pragma acc parallel num_gangs(2)
  {
    withinGangFn();
    withinWorkerFn();
    withinVectorFn();
    withinSeqFn();
  }

  // PRT-NEXT: return 0;
  return 0;
} // PRT-NEXT: }

//--------------------------------------------------
// gang function can have implicit gang clauses.
//
// This repeats the above testing but within a gang function and without
// statically enclosing parallel constructs.
//--------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} withinGangFn
// PRT-LABEL: void withinGangFn() {
void withinGangFn() {
  //..................................................
  // gang, worker, vector, seq, auto combinations (without loop nesting, which
  // is checked later)
  //..................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop without nesting: 1{{$}}
  #pragma acc loop
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinGangFn: orphaned acc loop without nesting: %d\n", i);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop gang without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop gang without nesting: 1{{$}}
  #pragma acc loop gang
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinGangFn: orphaned acc loop gang without nesting: %d\n", i);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeParallelForDirective
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop worker without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop worker without nesting: 1{{$}}
  #pragma acc loop worker
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinGangFn: orphaned acc loop worker without nesting: %d\n", i);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeSimdDirective
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute simd{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop vector without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop vector without nesting: 1{{$}}
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
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop seq without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop seq without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop seq without nesting: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop seq without nesting: 1{{$}}
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
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop auto without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop auto without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop auto without nesting: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop auto without nesting: 1{{$}}
  #pragma acc loop auto
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinGangFn: orphaned acc loop auto without nesting: %d\n", i);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeParallelForDirective
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang worker{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang worker{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop gang worker without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop gang worker without nesting: 1{{$}}
  #pragma acc loop gang worker
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinGangFn: orphaned acc loop gang worker without nesting: %d\n", i);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeSimdDirective
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang vector{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute simd{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang vector{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop gang vector without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop gang vector without nesting: 1{{$}}
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
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop worker vector without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop worker vector without nesting: 1{{$}}
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
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop gang worker vector without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop gang worker vector without nesting: 1{{$}}
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
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop gang auto without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop gang auto without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop gang auto without nesting: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop gang auto without nesting: 1{{$}}
  #pragma acc loop gang auto
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinGangFn: orphaned acc loop gang auto without nesting: %d\n", i);

  //..................................................
  // gang, worker, vector, seq, auto on enclosing acc loop.
  //..................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  #pragma acc loop gang
  for (int i = 0; i < 2; ++i) {
    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    //  DMP-NOT:     OMP
    //      DMP:     CallExpr
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop gang enclosing acc loop: 0, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop gang enclosing acc loop: 0, 1{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop gang enclosing acc loop: 1, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop gang enclosing acc loop: 1, 1{{$}}
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinGangFn: orphaned acc loop gang enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeParallelForDirective
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  #pragma acc loop worker
  for (int i = 0; i < 2; ++i) {
    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    //  DMP-NOT:     OMP
    //      DMP:     CallExpr
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop worker enclosing acc loop: 0, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop worker enclosing acc loop: 0, 1{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop worker enclosing acc loop: 1, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop worker enclosing acc loop: 1, 1{{$}}
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinGangFn: orphaned acc loop worker enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeSimdDirective
  //  DMP-NOT:     OMP
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute simd{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector{{$}}
  #pragma acc loop vector
  // DMP: ForStmt
  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < 2; ++i) {
    // DMP-NOT:  OMP
    // DMP:      ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //      DMP:       CallExpr
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop vector enclosing acc loop: 0, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop vector enclosing acc loop: 0, 1{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop vector enclosing acc loop: 1, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop vector enclosing acc loop: 1, 1{{$}}
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
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
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //      DMP:       CallExpr
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT: {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop seq enclosing acc loop: 0, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop seq enclosing acc loop: 0, 1{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop seq enclosing acc loop: 1, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop seq enclosing acc loop: 1, 1{{$}}
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
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
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //      DMP:       CallExpr
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
      // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop auto enclosing acc loop: 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop auto enclosing acc loop: 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop auto enclosing acc loop: 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop auto enclosing acc loop: 1, 1{{$}}
      TGT_PRINTF("withinGangFn: orphaned acc loop auto enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  //  DMP-NOT:    <implicit>
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   impl: ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang auto
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang auto // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  #pragma acc loop gang auto
  for (int i = 0; i < 2; ++i) {
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //      DMP:       CallExpr
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop gang auto enclosing acc loop: 0, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop gang auto enclosing acc loop: 0, 1{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop gang auto enclosing acc loop: 1, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: orphaned acc loop gang auto enclosing acc loop: 1, 1{{$}}
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinGangFn: orphaned acc loop gang auto enclosing acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //..................................................
  // gang, worker, vector, seq, auto on enclosed acc loop.
  //..................................................

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
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //      DMP:       CallExpr
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: acc loop gang enclosed by orphaned acc loop: 0, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: acc loop gang enclosed by orphaned acc loop: 0, 1{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: acc loop gang enclosed by orphaned acc loop: 1, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: acc loop gang enclosed by orphaned acc loop: 1, 1{{$}}
    #pragma acc loop gang
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinGangFn: acc loop gang enclosed by orphaned acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  #pragma acc loop
  for (int i = 0; i < 2; ++i) {
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: OMPParallelForDirective
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //      DMP:       CallExpr
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for{{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp parallel for{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: acc loop worker enclosed by orphaned acc loop: 0, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: acc loop worker enclosed by orphaned acc loop: 0, 1{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: acc loop worker enclosed by orphaned acc loop: 1, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: acc loop worker enclosed by orphaned acc loop: 1, 1{{$}}
    #pragma acc loop worker
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinGangFn: acc loop worker enclosed by orphaned acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  #pragma acc loop
  for (int i = 0; i < 2; ++i) {
    //      DMP: ACCLoopDirective
    // DMP-NEXT:  ACCVectorClause
    //  DMP-NOT:    <implicit>
    // DMP-NEXT:  ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:  ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:    DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:  impl: OMPSimdDirective
    //  DMP-NOT:    OMP
    //      DMP:    ForStmt
    //      DMP:      CallExpr
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp simd{{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp simd{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: acc loop vector enclosed by orphaned acc loop: 0, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: acc loop vector enclosed by orphaned acc loop: 0, 1{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: acc loop vector enclosed by orphaned acc loop: 1, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: acc loop vector enclosed by orphaned acc loop: 1, 1{{$}}
    #pragma acc loop vector
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinGangFn: acc loop vector enclosed by orphaned acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  #pragma acc loop
  for (int i = 0; i < 2; ++i) {
    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCSharedClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    //  DMP-NOT:     OMP
    //      DMP:     CallExpr
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: acc loop seq enclosed by orphaned acc loop: 0, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: acc loop seq enclosed by orphaned acc loop: 0, 1{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: acc loop seq enclosed by orphaned acc loop: 1, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: acc loop seq enclosed by orphaned acc loop: 1, 1{{$}}
    #pragma acc loop seq
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinGangFn: acc loop seq enclosed by orphaned acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  #pragma acc loop
  for (int i = 0; i < 2; ++i) {
    //  DMP-NOT: OMP
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCAutoClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCSharedClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    //  DMP-NOT:     OMP
    //      DMP:     CallExpr
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: acc loop auto enclosed by orphaned acc loop: 0, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: acc loop auto enclosed by orphaned acc loop: 0, 1{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: acc loop auto enclosed by orphaned acc loop: 1, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: acc loop auto enclosed by orphaned acc loop: 1, 1{{$}}
    #pragma acc loop auto
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinGangFn: acc loop auto enclosed by orphaned acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  #pragma acc loop
  for (int i = 0; i < 2; ++i) {
    //  DMP-NOT: OMP
    //      DMP:   ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCAutoClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCSharedClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: ForStmt
    //  DMP-NOT:     OMP
    //      DMP:     CallExpr
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang auto
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    //  PRT-A-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang auto // discarded in OpenMP translation{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: acc loop gang auto enclosed by orphaned acc loop: 0, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: acc loop gang auto enclosed by orphaned acc loop: 0, 1{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: acc loop gang auto enclosed by orphaned acc loop: 1, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: acc loop gang auto enclosed by orphaned acc loop: 1, 1{{$}}
    #pragma acc loop gang auto
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinGangFn: acc loop gang auto enclosed by orphaned acc loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //..................................................
  // gang, worker, vector, seq function call enclosed.
  //..................................................

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
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: gang function call enclosed by orphaned acc loop: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: gang function call enclosed by orphaned acc loop: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: gang function call enclosed by orphaned acc loop: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: gang function call enclosed by orphaned acc loop: 1{{$}}
  #pragma acc loop
  for (int i = 0; i < 2; ++i)
    gangFn("withinGangFn: gang function call enclosed by orphaned acc loop", i);

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
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: worker function call enclosed by orphaned acc loop: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: worker function call enclosed by orphaned acc loop: 1{{$}}
  #pragma acc loop
  for (int i = 0; i < 2; ++i)
    workerFn("withinGangFn: worker function call enclosed by orphaned acc loop", i);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //      DMP:         DeclRefExpr {{.*}} 'vectorFn'
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   vectorFn({{.*}});
  //
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: vector function call enclosed by orphaned acc loop: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: vector function call enclosed by orphaned acc loop: 1{{$}}
  #pragma acc loop
  for (int i = 0; i < 2; ++i)
    vectorFn("withinGangFn: vector function call enclosed by orphaned acc loop", i);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //      DMP:         DeclRefExpr {{.*}} 'seqFn'
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   seqFn({{.*}});
  //
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: seq function call enclosed by orphaned acc loop: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: seq function call enclosed by orphaned acc loop: 1{{$}}
  #pragma acc loop
  for (int i = 0; i < 2; ++i)
    seqFn("withinGangFn: seq function call enclosed by orphaned acc loop", i);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //      DMP:         DeclRefExpr {{.*}} 'impFn'
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   impFn({{.*}});
  //
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: implicit seq function call enclosed by orphaned acc loop: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: implicit seq function call enclosed by orphaned acc loop: 1{{$}}
  #pragma acc loop
  for (int i = 0; i < 2; ++i)
    impFn("withinGangFn: implicit seq function call enclosed by orphaned acc loop", i);

  //..................................................
  // Multiple levels are gang clause candidates.
  //..................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
  //    PRT-NEXT: for ({{.*}}) {
  #pragma acc loop
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
      //      DMP:     CallExpr
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      //  PRT-A-SAME: {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
      //    PRT-NEXT: for ({{.*}})
      //    PRT-NEXT:   {{TGT_PRINTF|printf}}
      //
      // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: multilevel, orphaned loop: 0, 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: multilevel, orphaned loop: 0, 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: multilevel, orphaned loop: 0, 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: multilevel, orphaned loop: 0, 1, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: multilevel, orphaned loop: 1, 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: multilevel, orphaned loop: 1, 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: multilevel, orphaned loop: 1, 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: multilevel, orphaned loop: 1, 1, 1{{$}}
      #pragma acc loop
      for (int k = 0; k < 2; ++k)
        TGT_PRINTF("withinGangFn: multilevel, orphaned loop: %d, %d, %d\n", i, j, k);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  //..................................................
  // gang clause effect on sibling loop via parent loop.
  //..................................................

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
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //      DMP:       CallExpr
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang loop first, first loop: 0, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang loop first, first loop: 0, 1{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang loop first, first loop: 1, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang loop first, first loop: 1, 1{{$}}
    #pragma acc loop gang
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinGangFn: siblings, orphaned loop, gang loop first, first loop: %d, %d\n", i, j);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //      DMP:       CallExpr
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang loop first, second loop: 0, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang loop first, second loop: 0, 1{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang loop first, second loop: 1, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang loop first, second loop: 1, 1{{$}}
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinGangFn: siblings, orphaned loop, gang loop first, second loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

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
    // DMP: CallExpr
    // DMP:   DeclRefExpr {{.*}} 'gangFn'
    // PRT-NEXT: gangFn({{.*}});
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang function first, function: 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang function first, function: 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang function first, function: 1{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang function first, function: 1{{$}}
    gangFn("withinGangFn: siblings, orphaned loop, gang function first, function", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //      DMP:       CallExpr
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang function first, loop: 0, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang function first, loop: 0, 1{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang function first, loop: 1, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang function first, loop: 1, 1{{$}}
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinGangFn: siblings, orphaned loop, gang function first, loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

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
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //      DMP:       CallExpr
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang loop second, first loop: 0, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang loop second, first loop: 0, 1{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang loop second, first loop: 1, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang loop second, first loop: 1, 1{{$}}
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinGangFn: siblings, orphaned loop, gang loop second, first loop: %d, %d\n", i, j);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //      DMP:       CallExpr
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang loop second, second loop: 0, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang loop second, second loop: 0, 1{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang loop second, second loop: 1, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang loop second, second loop: 1, 1{{$}}
    #pragma acc loop gang
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinGangFn: siblings, orphaned loop, gang loop second, second loop: %d, %d\n", i, j);
  } // PRT-NEXT: }

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
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //      DMP:       CallExpr
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop{{$}}
    //    PRT-NEXT: for ({{.*}})
    //    PRT-NEXT:   {{(TGT_PRINTF|printf)([^)]|[[:space:]])*}});
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang function second, loop: 0, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang function second, loop: 0, 1{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang function second, loop: 1, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang function second, loop: 1, 1{{$}}
    #pragma acc loop
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinGangFn: siblings, orphaned loop, gang function second, loop: %d, %d\n", i, j);

    // DMP: CallExpr
    // DMP:   DeclRefExpr {{.*}} 'gangFn'
    //
    // PRT-NEXT: gangFn({{.*}});
    //
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang function second, function: 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang function second, function: 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang function second, function: 1{{$}}
    // EXE-TGT-USE-STDIO-DAG: {{^}}withinGangFn: siblings, orphaned loop, gang function second, function: 1{{$}}
    gangFn("withinGangFn: siblings, orphaned loop, gang function second, function", i);
  } // PRT-NEXT: }
} // PRT-NEXT: }

//--------------------------------------------------
// worker function: no implicit gang clauses.
//
// This repeats withinGangFn's worker, vector, seq, and auto combinations
// without loop nesting.  That should be sufficent at this point to check that
// implicit gang clauses generally are not added.
//--------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} withinWorkerFn
// PRT-LABEL: void withinWorkerFn() {
#pragma acc routine worker
void withinWorkerFn() {
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
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop without nesting: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop without nesting: 1{{$}}
  #pragma acc loop
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinWorkerFn: orphaned acc loop without nesting: %d\n", i);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPParallelForDirective
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp parallel for{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop worker without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop worker without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop worker without nesting: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop worker without nesting: 1{{$}}
  #pragma acc loop worker
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinWorkerFn: orphaned acc loop worker without nesting: %d\n", i);

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
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop vector without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop vector without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop vector without nesting: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop vector without nesting: 1{{$}}
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
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop seq without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop seq without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop seq without nesting: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop seq without nesting: 1{{$}}
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
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop auto without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop auto without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop auto without nesting: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop auto without nesting: 1{{$}}
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
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop worker vector without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop worker vector without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop worker vector without nesting: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop worker vector without nesting: 1{{$}}
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
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop worker auto without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop worker auto without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop worker auto without nesting: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinWorkerFn: orphaned acc loop worker auto without nesting: 1{{$}}
  #pragma acc loop worker auto
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinWorkerFn: orphaned acc loop worker auto without nesting: %d\n", i);
} // PRT-NEXT: }

//--------------------------------------------------
// vector function: no implicit gang clauses.
//
// This repeats withinGangFn's vector, seq, and auto combinations without loop
// nesting.  That should be sufficent at this point to check that implicit gang
// clauses generally are not added.
//--------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} withinVectorFn
// PRT-LABEL: void withinVectorFn() {
#pragma acc routine vector
void withinVectorFn() {
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
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinVectorFn: orphaned acc loop without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinVectorFn: orphaned acc loop without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinVectorFn: orphaned acc loop without nesting: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinVectorFn: orphaned acc loop without nesting: 1{{$}}
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
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinVectorFn: orphaned acc loop vector without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinVectorFn: orphaned acc loop vector without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinVectorFn: orphaned acc loop vector without nesting: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinVectorFn: orphaned acc loop vector without nesting: 1{{$}}
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
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinVectorFn: orphaned acc loop seq without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinVectorFn: orphaned acc loop seq without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinVectorFn: orphaned acc loop seq without nesting: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinVectorFn: orphaned acc loop seq without nesting: 1{{$}}
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
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinVectorFn: orphaned acc loop auto without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinVectorFn: orphaned acc loop auto without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinVectorFn: orphaned acc loop auto without nesting: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinVectorFn: orphaned acc loop auto without nesting: 1{{$}}
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
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinVectorFn: orphaned acc loop vector auto without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinVectorFn: orphaned acc loop vector auto without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinVectorFn: orphaned acc loop vector auto without nesting: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinVectorFn: orphaned acc loop vector auto without nesting: 1{{$}}
  #pragma acc loop vector auto
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinVectorFn: orphaned acc loop vector auto without nesting: %d\n", i);
} // PRT-NEXT: }

//--------------------------------------------------
// seq function: no implicit gang clauses.
//
// This repeats withinGangFn's seq and auto checks without loop nesting.  That
// should be sufficent at this point to check that implicit gang clauses
// generally are not added.
//--------------------------------------------------

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
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinSeqFn: orphaned acc loop without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinSeqFn: orphaned acc loop without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinSeqFn: orphaned acc loop without nesting: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinSeqFn: orphaned acc loop without nesting: 1{{$}}
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
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinSeqFn: orphaned acc loop seq without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinSeqFn: orphaned acc loop seq without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinSeqFn: orphaned acc loop seq without nesting: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinSeqFn: orphaned acc loop seq without nesting: 1{{$}}
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
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinSeqFn: orphaned acc loop auto without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinSeqFn: orphaned acc loop auto without nesting: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinSeqFn: orphaned acc loop auto without nesting: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: {{^}}withinSeqFn: orphaned acc loop auto without nesting: 1{{$}}
  #pragma acc loop auto
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("withinSeqFn: orphaned acc loop auto without nesting: %d\n", i);
} // PRT-NEXT: }

// EXE-NOT: {{.}}
