// Check implicit gang clause on "acc loop" and on "acc parallel loop".
//
// loop.c checks each combination of clauses on an unnested acc loop, so this
// file mostly checks cases of nested acc loops and enclosed calls to functions
// with routine directives.
//
// Positive testing for gang reductions appears in loop-reduction.c.  Testing
// for diagnostics about conflicting gang reductions appears in
// diagnostics/loop.c.

// FIXME: amdgcn doesn't yet support printf in a kernel.  Unfortunately, that
// means our execution checks on amdgcn don't verify much except that nothing
// crashes.
//
// RUN: %acc-check-dmp{}
// RUN: %acc-check-prt{}
// RUN: %acc-check-exe{                                                        \
// RUN:   clang-args: ;                                                        \
// RUN:   exe-args:   ;                                                        \
// RUN:   fc-args:    -match-full-lines}

// END.

/* expected-error 0 {{}} */

// FIXME: Clang produces spurious warning diagnostics for nvptx64 offload.  This
// issue is not limited to Clacc and is present upstream:
/* nvptx64-warning@*:* 0+ {{Linking two modules of different target triples}} */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if TGT_USE_STDIO
# define TGT_PRINTF(...) printf(__VA_ARGS__)
#else
# define TGT_PRINTF(...)
#endif

#pragma acc routine gang
void gangFn(const char *msg, int i) { TGT_PRINTF("%s: %d\n", msg, i); }
#pragma acc routine worker
void workerFn(const char *msg, int i) { TGT_PRINTF("%s: %d\n", msg, i); }
#pragma acc routine vector
void vectorFn(const char *msg, int i) { TGT_PRINTF("%s: %d\n", msg, i); }
#pragma acc routine seq
void seqFn(const char *msg, int i) { TGT_PRINTF("%s: %d\n", msg, i); }
void impFn(const char *msg, int i) { TGT_PRINTF("%s: %d\n", msg, i); }

// PRT: int main() {
int main() {
  //--------------------------------------------------
  // gang, worker, vector, seq, auto on enclosing acc loop.
  //--------------------------------------------------

  // First for acc parallel and acc loop separately.

  // DMP-LABEL: StringLiteral {{.*}} "acc loop clause enclosing acc loop\n"
  // PRT-LABEL: printf("acc loop clause enclosing acc loop\n");
  // EXE-LABEL: {{^}}acc loop clause enclosing acc loop{{$}}
  // EXE-NOT: {{.}}
  printf("acc loop clause enclosing acc loop\n");

// FIXME: amdgcn misbehaves sometimes for worker loops.
// PRT-SRC-NEXT: #if !TGT_AMDGCN
#if !TGT_AMDGCN
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
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-AO-SAME: {{^$}}
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
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-AO-SAME: {{^$}}
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
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-AO-SAME: {{^$}}
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
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop seq
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    // PRT-AO-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq // discarded in OpenMP translation{{$}}
    //
    // PRT-NEXT:    for ({{.*}}) {
    #pragma acc loop seq
    for (int i = 0; i < 2; ++i) {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NOT:      OMP
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
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop auto
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    // PRT-AO-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto // discarded in OpenMP translation{{$}}
    #pragma acc loop auto
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NOT:      OMP
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
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop auto gang
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    // PRT-AO-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto gang // discarded in OpenMP translation{{$}}
    #pragma acc loop auto gang
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NOT:      OMP
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
// PRT-SRC-NEXT: #endif
#endif

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
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    // PRT-AO-SAME: {{^$}}
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

// FIXME: amdgcn misbehaves sometimes for worker loops.
// PRT-SRC-NEXT: #if !TGT_AMDGCN
#if !TGT_AMDGCN
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
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    // PRT-AO-SAME: {{^$}}
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
// PRT-SRC-NEXT: #endif
#endif

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
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    // PRT-AO-SAME: {{^$}}
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
    // DMP:      ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NOT:      OMP
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
    // DMP:      ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NOT:      OMP
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
    // DMP:      ACCLoopDirective
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NOT:      OMP
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

  //--------------------------------------------------
  // gang, worker, vector, seq, auto on enclosed acc loop.
  //--------------------------------------------------

  // First for acc parallel and acc loop separately.

  // DMP-LABEL: StringLiteral {{.*}} "acc loop clause enclosed by acc loop\n"
  // PRT-LABEL: printf("acc loop clause enclosed by acc loop\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc loop clause enclosed by acc loop{{$}}
  // EXE-NOT: {{.}}
  printf("acc loop clause enclosed by acc loop\n");

// FIXME: amdgcn misbehaves sometimes for worker loops.
// PRT-SRC-NEXT: #if !TGT_AMDGCN
#if !TGT_AMDGCN
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
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    // PRT-AO-SAME: {{^$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
    //
    // PRT-NEXT:    for ({{.*}}) {
    #pragma acc loop
    for (int i = 0; i < 2; ++i) {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NOT:      <implicit>
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NOT:      OMP
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
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NOT:      <implicit>
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: OMPParallelForDirective
      // DMP-NEXT:     OMPNum_threadsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
      // DMP-NEXT:     OMPSharedClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for num_threads(2) shared(i){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for num_threads(2) shared(i){{$}}
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
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCVectorClause
      // DMP-NOT:      <implicit>
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: OMPSimdDirective
      // DMP-NEXT:     OMPSimdlenClause
      // DMP-NEXT:       ConstantExpr
      // DMP-NEXT:         value: Int 2
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
      // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NOT:      OMP
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
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop seq
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-AO-SAME: {{^$}}
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
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop auto
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-AO-SAME: {{^$}}
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
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang auto
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-AO-SAME: {{^$}}
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
// PRT-SRC-NEXT: #endif
#endif

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
    // DMP:      ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: OMPDistributeDirective
    // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NOT:      OMP
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

// FIXME: amdgcn misbehaves sometimes for worker loops.
// PRT-SRC-NEXT: #if !TGT_AMDGCN
#if !TGT_AMDGCN
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
    // DMP:      ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: OMPParallelForDirective
    // DMP-NEXT:     OMPNum_threadsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPSharedClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NOT:      OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for num_threads(2) shared(i){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for num_threads(2) shared(i){{$}}
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
// PRT-SRC-NEXT: #endif
#endif

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
    // DMP:      ACCLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: OMPSimdDirective
    // DMP-NEXT:     OMPSimdlenClause
    // DMP-NEXT:       ConstantExpr
    // DMP-NEXT:         value: Int 2
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NOT:      OMP
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
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop seq
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    // PRT-AO-SAME: {{^$}}
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
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop auto
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    // PRT-AO-SAME: {{^$}}
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
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang auto
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    // PRT-AO-SAME: {{^$}}
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

  //--------------------------------------------------
  // gang, worker, vector, seq function call enclosed.
  //--------------------------------------------------

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
    // PRT-AO-SAME: {{^$}}
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

  //--------------------------------------------------
  // Multiple levels are gang clause candidates.
  //--------------------------------------------------

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
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    // PRT-AO-SAME: {{^$}}
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
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-AO-SAME: {{^$}}
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
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop
    // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
    // PRT-AO-SAME: {{^$}}
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
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-AO-SAME: {{^$}}
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

  //--------------------------------------------------
  // gang clause effect on sibling loop via parent loop.
  //--------------------------------------------------

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
  // PRT-A-NEXT:  {{^ *}}#pragma acc loop
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  // PRT-AO-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
  //
  // PRT-NEXT:    for ({{.*}}) {
  #pragma acc loop
  for (int i = 0; i < 2; ++i) {
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NOT:      <implicit>
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NOT:      OMP
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

      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NOT:      OMP
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
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NOT:      <implicit>
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NOT:      OMP
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

      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NOT:      OMP
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
  // PRT-A-NEXT:  {{^ *}}#pragma acc loop
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  // PRT-AO-SAME: {{^$}}
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

      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NOT:      OMP
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

      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NOT:      OMP
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
  // PRT-A-NEXT:  {{^ *}}#pragma acc loop
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  // PRT-AO-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
  //
  // PRT-NEXT:    for ({{.*}}) {
  #pragma acc loop
  for (int i = 0; i < 2; ++i) {
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NOT:      OMP
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

      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NOT:      <implicit>
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NOT:      OMP
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
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NOT:      OMP
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

      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NOT:      <implicit>
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NOT:      OMP
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
  // PRT-A-NEXT:  {{^ *}}#pragma acc loop
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  // PRT-AO-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
  //
  // PRT-NEXT:    for ({{.*}}) {
  #pragma acc loop
  for (int i = 0; i < 2; ++i) {
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NOT:      OMP
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
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NOT:      OMP
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

  // PRT-NEXT: return 0;
  return 0;
} // PRT-NEXT: }
// EXE-NOT: {{.}}