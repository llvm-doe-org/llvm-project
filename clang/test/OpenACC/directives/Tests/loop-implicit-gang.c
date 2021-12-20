// Check implicit gang clause on "acc loop" and on "acc parallel loop".
//
// This mostly checks cases of nested acc loops as loop.c checks each
// combination of clauses on an unnested acc loop.
//
// Positive testing for gang reductions appears in loop-reduction.c.  Testing
// for diagnostics about conflicting gang reductions appears in
// loop-messages.c.

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

// PRT: int main() {
int main() {
  //--------------------------------------------------
  // gang, worker, vector, seq, auto on enclosing loop.
  //--------------------------------------------------

  // First for acc parallel and acc loop separately.

  // DMP-LABEL: StringLiteral {{.*}} "acc loop enclosing\n"
  // PRT-LABEL: printf("acc loop enclosing\n");
  // EXE-LABEL: {{^}}acc loop enclosing{{$}}
  // EXE-NOT: {{.}}
  printf("acc loop enclosing\n");

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
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang enclosing: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang enclosing: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang enclosing: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang enclosing: 1, 1{{$}}
        TGT_PRINTF("acc loop gang enclosing: %d, %d\n", i, j);
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
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop worker enclosing: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop worker enclosing: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop worker enclosing: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop worker enclosing: 1, 1{{$}}
        TGT_PRINTF("acc loop worker enclosing: %d, %d\n", i, j);

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
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop vector enclosing: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop vector enclosing: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop vector enclosing: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop vector enclosing: 1, 1{{$}}
        TGT_PRINTF("acc loop vector enclosing: %d, %d\n", i, j);

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
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop seq enclosing: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop seq enclosing: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop seq enclosing: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop seq enclosing: 1, 1{{$}}
        TGT_PRINTF("acc loop seq enclosing: %d, %d\n", i, j);
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
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto enclosing: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto enclosing: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto enclosing: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto enclosing: 1, 1{{$}}
        TGT_PRINTF("acc loop auto enclosing: %d, %d\n", i, j);
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
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto gang enclosing: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto gang enclosing: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto gang enclosing: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto gang enclosing: 1, 1{{$}}
        TGT_PRINTF("acc loop auto gang enclosing: %d, %d\n", i, j);
    } // PRT-NEXT: }
  } // PRT-NEXT: }
// PRT-SRC-NEXT: #endif
#endif

  // Repeat for acc parallel loop.

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop gang enclosing\n"
  // PRT-LABEL: printf("acc parallel loop gang enclosing\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop gang enclosing{{$}}
  // EXE-NOT: {{.}}
  printf("acc parallel loop gang enclosing\n");

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
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang enclosing: 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang enclosing: 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang enclosing: 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang enclosing: 1, 1{{$}}
      TGT_PRINTF("acc parallel loop gang enclosing: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop worker enclosing\n"
  // PRT-LABEL: printf("acc parallel loop worker enclosing\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop worker enclosing{{$}}
  // EXE-NOT: {{.}}
  printf("acc parallel loop worker enclosing\n");

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
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop worker enclosing: 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop worker enclosing: 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop worker enclosing: 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop worker enclosing: 1, 1{{$}}
      TGT_PRINTF("acc parallel loop worker enclosing: %d, %d\n", i, j);
  } // PRT-NEXT: }
// PRT-SRC-NEXT: #endif
#endif

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop vector enclosing\n"
  // PRT-LABEL: printf("acc parallel loop vector enclosing\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop vector enclosing{{$}}
  // EXE-NOT: {{.}}
  printf("acc parallel loop vector enclosing\n");

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
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop vector enclosing: 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop vector enclosing: 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop vector enclosing: 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop vector enclosing: 1, 1{{$}}
      TGT_PRINTF("acc parallel loop vector enclosing: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop seq enclosing\n"
  // PRT-LABEL: printf("acc parallel loop seq enclosing\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop seq enclosing{{$}}
  // EXE-NOT: {{.}}
  printf("acc parallel loop seq enclosing\n");

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
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop seq enclosing: 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop seq enclosing: 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop seq enclosing: 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop seq enclosing: 1, 1{{$}}
      TGT_PRINTF("acc parallel loop seq enclosing: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop auto enclosing\n"
  // PRT-LABEL: printf("acc parallel loop auto enclosing\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop auto enclosing{{$}}
  // EXE-NOT: {{.}}
  printf("acc parallel loop auto enclosing\n");

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
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop auto enclosing: 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop auto enclosing: 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop auto enclosing: 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop auto enclosing: 1, 1{{$}}
      TGT_PRINTF("acc parallel loop auto enclosing: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop gang auto enclosing\n"
  // PRT-LABEL: printf("acc parallel loop gang auto enclosing\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop gang auto enclosing{{$}}
  // EXE-NOT: {{.}}
  printf("acc parallel loop gang auto enclosing\n");

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
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang auto enclosing: 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang auto enclosing: 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang auto enclosing: 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang auto enclosing: 1, 1{{$}}
      TGT_PRINTF("acc parallel loop gang auto enclosing: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // gang, worker, vector, seq, auto on enclosed loop.
  //--------------------------------------------------

  // First for acc parallel and acc loop separately.

  // DMP-LABEL: StringLiteral {{.*}} "acc loop enclosed\n"
  // PRT-LABEL: printf("acc loop enclosed\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc loop enclosed{{$}}
  // EXE-NOT: {{.}}
  printf("acc loop enclosed\n");

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
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang enclosed: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang enclosed: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang enclosed: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang enclosed: 1, 1{{$}}
        TGT_PRINTF("acc loop gang enclosed: %d, %d\n", i, j);
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
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop worker enclosed: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop worker enclosed: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop worker enclosed: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop worker enclosed: 1, 1{{$}}
        TGT_PRINTF("acc loop worker enclosed: %d, %d\n", i, j);
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
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop vector enclosed: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop vector enclosed: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop vector enclosed: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop vector enclosed: 1, 1{{$}}
        TGT_PRINTF("acc loop vector enclosed: %d, %d\n", i, j);
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
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop seq enclosed: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop seq enclosed: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop seq enclosed: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop seq enclosed: 1, 1{{$}}
        TGT_PRINTF("acc loop seq enclosed: %d, %d\n", i, j);
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
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto enclosed: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto enclosed: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto enclosed: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop auto enclosed: 1, 1{{$}}
        TGT_PRINTF("acc loop auto enclosed: %d, %d\n", i, j);
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
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang auto enclosed: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang auto enclosed: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang auto enclosed: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}acc loop gang auto enclosed: 1, 1{{$}}
        TGT_PRINTF("acc loop gang auto enclosed: %d, %d\n", i, j);
    } // PRT-NEXT: }
  } // PRT-NEXT: }
// PRT-SRC-NEXT: #endif
#endif

  // Repeat for acc parallel loop.

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop gang enclosed\n"
  // PRT-LABEL: printf("acc parallel loop gang enclosed\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop gang enclosed{{$}}
  // EXE-NOT: {{.}}
  printf("acc parallel loop gang enclosed\n");

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
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang enclosed: 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang enclosed: 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang enclosed: 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang enclosed: 1, 1{{$}}
      TGT_PRINTF("acc parallel loop gang enclosed: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop worker enclosed\n"
  // PRT-LABEL: printf("acc parallel loop worker enclosed\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop worker enclosed{{$}}
  // EXE-NOT: {{.}}
  printf("acc parallel loop worker enclosed\n");

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
  // PRT-NEXT: for ({{.*}})
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
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop worker enclosed: 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop worker enclosed: 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop worker enclosed: 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop worker enclosed: 1, 1{{$}}
      TGT_PRINTF("acc parallel loop worker enclosed: %d, %d\n", i, j);
  } // PRT-NEXT: }
// PRT-SRC-NEXT: #endif
#endif

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop vector enclosed\n"
  // PRT-LABEL: printf("acc parallel loop vector enclosed\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop vector enclosed{{$}}
  // EXE-NOT: {{.}}
  printf("acc parallel loop vector enclosed\n");

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
  // PRT-NEXT: for ({{.*}})
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
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop vector enclosed: 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop vector enclosed: 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop vector enclosed: 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop vector enclosed: 1, 1{{$}}
      TGT_PRINTF("acc parallel loop vector enclosed: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop seq enclosed\n"
  // PRT-LABEL: printf("acc parallel loop seq enclosed\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop seq enclosed{{$}}
  // EXE-NOT: {{.}}
  printf("acc parallel loop seq enclosed\n");

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
  // PRT-NEXT: for ({{.*}})
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
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop seq enclosed: 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop seq enclosed: 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop seq enclosed: 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop seq enclosed: 1, 1{{$}}
      TGT_PRINTF("acc parallel loop seq enclosed: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop auto enclosed\n"
  // PRT-LABEL: printf("acc parallel loop auto enclosed\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop auto enclosed{{$}}
  // EXE-NOT: {{.}}
  printf("acc parallel loop auto enclosed\n");

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
  // PRT-NEXT: for ({{.*}})
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
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop auto enclosed: 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop auto enclosed: 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop auto enclosed: 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop auto enclosed: 1, 1{{$}}
      TGT_PRINTF("acc parallel loop auto enclosed: %d, %d\n", i, j);
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop gang auto enclosed\n"
  // PRT-LABEL: printf("acc parallel loop gang auto enclosed\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}acc parallel loop gang auto enclosed{{$}}
  // EXE-NOT: {{.}}
  printf("acc parallel loop gang auto enclosed\n");

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
  // PRT-NEXT: for ({{.*}})
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
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang auto enclosed: 0, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang auto enclosed: 0, 1{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang auto enclosed: 1, 0{{$}}
      // EXE-TGT-USE-STDIO-DAG: {{^}}acc parallel loop gang auto enclosed: 1, 1{{$}}
      TGT_PRINTF("acc parallel loop gang auto enclosed: %d, %d\n", i, j);
  } // PRT-NEXT: }

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

  // DMP-LABEL: StringLiteral {{.*}} "siblings, separate, gang first\n"
  // PRT-LABEL: printf("siblings, separate, gang first\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}siblings, separate, gang first{{$}}
  // EXE-NOT: {{.}}
  printf("siblings, separate, gang first\n");

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
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang first, first loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang first, first loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang first, first loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang first, first loop: 1, 1{{$}}
        TGT_PRINTF("siblings, separate, gang first, first loop: %d, %d\n",
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
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang first, second loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang first, second loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang first, second loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang first, second loop: 1, 1{{$}}
        TGT_PRINTF("siblings, separate, gang first, second loop: %d, %d\n",
                   i, j);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "siblings, combined, gang first\n"
  // PRT-LABEL: printf("siblings, combined, gang first\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}siblings, combined, gang first{{$}}
  // EXE-NOT: {{.}}
  printf("siblings, combined, gang first\n");

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
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang first, first loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang first, first loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang first, first loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang first, first loop: 1, 1{{$}}
        TGT_PRINTF("siblings, combined, gang first, first loop: %d, %d\n",
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
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang first, second loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang first, second loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang first, second loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang first, second loop: 1, 1{{$}}
        TGT_PRINTF("siblings, combined, gang first, second loop: %d, %d\n",
                   i, j);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "siblings, separate, gang second\n"
  // PRT-LABEL: printf("siblings, separate, gang second\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}siblings, separate, gang second{{$}}
  // EXE-NOT: {{.}}
  printf("siblings, separate, gang second\n");

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
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang second, first loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang second, first loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang second, first loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang second, first loop: 1, 1{{$}}
        TGT_PRINTF("siblings, separate, gang second, first loop: %d, %d\n",
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
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang second, second loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang second, second loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang second, second loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, separate, gang second, second loop: 1, 1{{$}}
        TGT_PRINTF("siblings, separate, gang second, second loop: %d, %d\n",
                   i, j);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "siblings, combined, gang second\n"
  // PRT-LABEL: printf("siblings, combined, gang second\n");
  // EXE-NOT: {{.}}
  // EXE-LABEL: {{^}}siblings, combined, gang second{{$}}
  // EXE-NOT: {{.}}
  printf("siblings, combined, gang second\n");

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
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang second, second loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang second, second loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang second, second loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang second, second loop: 1, 1{{$}}
        TGT_PRINTF("siblings, combined, gang second, second loop: %d, %d\n",
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
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang second, first loop: 0, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang second, first loop: 0, 1{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang second, first loop: 1, 0{{$}}
        // EXE-TGT-USE-STDIO-DAG: {{^}}siblings, combined, gang second, first loop: 1, 1{{$}}
        TGT_PRINTF("siblings, combined, gang second, first loop: %d, %d\n",
                   i, j);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // PRT-NEXT: return 0;
  return 0;
} // PRT-NEXT: }
// EXE-NOT: {{.}}
