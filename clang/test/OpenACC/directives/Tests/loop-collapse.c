// Check collapse clause on "acc parallel loop", on "acc loop" within
// "acc parallel", and on orphaned "acc loop".

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

#if TGT_USE_STDIO
# define TGT_PRINTF(...) printf(__VA_ARGS__)
#else
# define TGT_PRINTF(...)
#endif

#pragma acc routine gang
void withinGangFn();
#pragma acc routine worker
void withinWorkerFn();
#pragma acc routine vector
void withinVectorFn();
#pragma acc routine seq
void withinSeqFn();

//------------------------------------------------------------------------------
// Check collapse clauses within parallel constructs.
//------------------------------------------------------------------------------

// PRT: int main() {
int main() {
  //............................................................................
  // Explicit seq.
  //............................................................................

  // DMP-LABEL: StringLiteral {{.*}} "loop seq\n"
  // PRT-LABEL: printf("loop seq\n");
  // EXE-LABEL: loop seq
  printf("loop seq\n");

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) num_workers(4){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) num_workers(4){{$}}
  #pragma acc parallel num_gangs(2) num_workers(4)
  // DMP:      ACCLoopDirective
  // DMP-NEXT:   ACCSeqClause
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   impl: ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc loop seq collapse(2)
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  // PRT-A-SAME:  {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq collapse(2) // discarded in OpenMP translation{{$}}
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc loop seq collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      //
      // For each gang, the entire loop nest must be sequential.
      // EXE-TGT-USE-STDIO: 0, 0
      // EXE-TGT-USE-STDIO: 0, 1
      // EXE-TGT-USE-STDIO: 1, 0
      // EXE-TGT-USE-STDIO: 1, 1
      TGT_PRINTF("%d, %d\n", i, j);

  // DMP-LABEL: StringLiteral {{.*}} "parallel loop seq\n"
  // PRT-LABEL: printf("parallel loop seq\n");
  // EXE-LABEL: parallel loop seq
  printf("parallel loop seq\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCSeqClause
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCSeqClause
  // DMP-NEXT:       ACCCollapseClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 2
  // DMP-NEXT:       impl: ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(1) vector_length(4) seq collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(1) vector_length(4) seq collapse(2){{$}}
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc parallel loop num_gangs(1) vector_length(4) seq collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-NEXT: 0, 0
      // EXE-TGT-USE-STDIO-NEXT: 0, 1
      // EXE-TGT-USE-STDIO-NEXT: 1, 0
      // EXE-TGT-USE-STDIO-NEXT: 1, 1
      TGT_PRINTF("%d, %d\n", i, j);

  //............................................................................
  // Implicit independent, implicit gang partitioning.
  //............................................................................

  // DMP-LABEL: StringLiteral {{.*}} "loop\n"
  // PRT-LABEL: printf("loop\n");
  // EXE-LABEL: loop
  printf("loop\n");

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1) num_workers(4){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) num_workers(4){{$}}
  #pragma acc parallel num_gangs(1) num_workers(4)
  // DMP:      ACCLoopDirective
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 2
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc loop collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop collapse(2){{$}}
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc loop collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-NEXT: 0, 0
      // EXE-TGT-USE-STDIO-NEXT: 0, 1
      // EXE-TGT-USE-STDIO-NEXT: 1, 0
      // EXE-TGT-USE-STDIO-NEXT: 1, 1
      TGT_PRINTF("%d, %d\n", i, j);

  // DMP-LABEL: StringLiteral {{.*}} "parallel loop\n"
  // PRT-LABEL: printf("parallel loop\n");
  // EXE-LABEL: parallel loop
  printf("parallel loop\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCCollapseClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 2
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeDirective
  // DMP-NEXT:         OMPCollapseClause
  // DMP-NEXT:           ConstantExpr {{.*}} 'int'
  // DMP-NEXT:             value: Int 2
  // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 2
  // DMP:              ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(1) vector_length(4) collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(1) vector_length(4) collapse(2){{$}}
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc parallel loop num_gangs(1) vector_length(4) collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-NEXT: 0, 0
      // EXE-TGT-USE-STDIO-NEXT: 0, 1
      // EXE-TGT-USE-STDIO-NEXT: 1, 0
      // EXE-TGT-USE-STDIO-NEXT: 1, 1
      TGT_PRINTF("%d, %d\n", i, j);

  //............................................................................
  // Explicit independent, implicit gang partitioning.
  //............................................................................

  // DMP-LABEL: StringLiteral {{.*}} "loop independent\n"
  // PRT-LABEL: printf("loop independent\n");
  // EXE-LABEL: loop independent
  printf("loop independent\n");

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1) num_workers(4){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) num_workers(4){{$}}
  #pragma acc parallel num_gangs(1) num_workers(4)
  // DMP:      ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause
  // DMP-NOT:      <implicit>
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 2
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc loop independent collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop independent collapse(2){{$}}
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc loop independent collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-NEXT: 0, 0
      // EXE-TGT-USE-STDIO-NEXT: 0, 1
      // EXE-TGT-USE-STDIO-NEXT: 1, 0
      // EXE-TGT-USE-STDIO-NEXT: 1, 1
      TGT_PRINTF("%d, %d\n", i, j);

  // DMP-LABEL: StringLiteral {{.*}} "parallel loop independent\n"
  // PRT-LABEL: printf("parallel loop independent\n");
  // EXE-LABEL: parallel loop independent
  printf("parallel loop independent\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCIndependentClause
  // DMP-NOT:      <implicit>
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCIndependentClause
  // DMP-NOT:          <implicit>
  // DMP-NEXT:       ACCCollapseClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 2
  // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeDirective
  // DMP-NEXT:         OMPCollapseClause
  // DMP-NEXT:           ConstantExpr {{.*}} 'int'
  // DMP-NEXT:             value: Int 2
  // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 2
  // DMP:              ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(1) vector_length(4) independent collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(1) vector_length(4) independent collapse(2){{$}}
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc parallel loop num_gangs(1) vector_length(4) independent collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-NEXT: 0, 0
      // EXE-TGT-USE-STDIO-NEXT: 0, 1
      // EXE-TGT-USE-STDIO-NEXT: 1, 0
      // EXE-TGT-USE-STDIO-NEXT: 1, 1
      TGT_PRINTF("%d, %d\n", i, j);

  //............................................................................
  // Explicit auto with partitioning.
  //............................................................................

  // DMP-LABEL: StringLiteral {{.*}} "loop auto\n"
  // PRT-LABEL: printf("loop auto\n");
  // EXE-LABEL: loop auto
  printf("loop auto\n");

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1) num_workers(4){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) num_workers(4){{$}}
  #pragma acc parallel num_gangs(1) num_workers(4)
  // DMP:      ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  // DMP-NEXT:   ACCWorkerClause
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   impl: ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc loop auto worker collapse(2)
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  // PRT-A-SAME:  {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto worker collapse(2) // discarded in OpenMP translation{{$}}
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc loop auto worker collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-NEXT: 0, 0
      // EXE-TGT-USE-STDIO-NEXT: 0, 1
      // EXE-TGT-USE-STDIO-NEXT: 1, 0
      // EXE-TGT-USE-STDIO-NEXT: 1, 1
      TGT_PRINTF("%d, %d\n", i, j);

  // DMP-LABEL: StringLiteral {{.*}} "parallel loop auto\n"
  // PRT-LABEL: printf("parallel loop auto\n");
  // EXE-LABEL: parallel loop auto
  printf("parallel loop auto\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCAutoClause
  // DMP-NEXT:   ACCVectorClause
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCAutoClause
  // DMP-NEXT:       ACCVectorClause
  // DMP-NEXT:       ACCCollapseClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 2
  // DMP-NEXT:       impl: ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) vector_length(4) auto vector collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) vector_length(4) auto vector collapse(2){{$}}
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc parallel loop num_gangs(2) vector_length(4) auto vector collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      //
      // For each gang, the entire loop nest must be sequential.
      // EXE-TGT-USE-STDIO: 0, 0
      // EXE-TGT-USE-STDIO: 0, 1
      // EXE-TGT-USE-STDIO: 1, 0
      // EXE-TGT-USE-STDIO: 1, 1
      TGT_PRINTF("%d, %d\n", i, j);

  //............................................................................
  // Worker or vector partitioned.
  //............................................................................

  // DMP-LABEL: StringLiteral {{.*}} "loop worker\n"
  // PRT-LABEL: printf("loop worker\n");
  // EXE-LABEL: loop worker
  printf("loop worker\n");

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1) num_workers(4){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) num_workers(4){{$}}
  #pragma acc parallel num_gangs(1) num_workers(4)
  // DMP:      ACCLoopDirective
  // DMP-NEXT:   ACCWorkerClause
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeParallelForDirective
  // DMP-NEXT:     OMPNum_threadsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 4
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 2
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(4) collapse(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute parallel for num_threads(4) collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker collapse(2){{$}}
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc loop worker collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG: 0, 0
      // EXE-TGT-USE-STDIO-DAG: 0, 1
      // EXE-TGT-USE-STDIO-DAG: 1, 0
      // EXE-TGT-USE-STDIO-DAG: 1, 1
      TGT_PRINTF("%d, %d\n", i, j);

  // DMP-LABEL: StringLiteral {{.*}} "parallel loop vector\n"
  // PRT-LABEL: printf("parallel loop vector\n");
  // EXE-LABEL: parallel loop vector
  printf("parallel loop vector\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorClause
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCVectorClause
  // DMP-NEXT:       ACCCollapseClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 2
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeSimdDirective
  // DMP-NEXT:         OMPSimdlenClause
  // DMP-NEXT:           ConstantExpr {{.*}} 'int'
  // DMP-NEXT:             value: Int 4
  // DMP-NEXT:             IntegerLiteral {{.*}} 4
  // DMP-NEXT:         OMPCollapseClause
  // DMP-NEXT:           ConstantExpr {{.*}} 'int'
  // DMP-NEXT:             value: Int 2
  // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 2
  // DMP:              ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(1) vector_length(4) vector collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd simdlen(4) collapse(2){{$}}
  //
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute simd simdlen(4) collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(1) vector_length(4) vector collapse(2){{$}}
  //
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc parallel loop num_gangs(1) vector_length(4) vector collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG: 0, 0
      // EXE-TGT-USE-STDIO-DAG: 0, 1
      // EXE-TGT-USE-STDIO-DAG: 1, 0
      // EXE-TGT-USE-STDIO-DAG: 1, 1
      TGT_PRINTF("%d, %d\n", i, j);

  //............................................................................
  // Explicit gang partitioned.
  //............................................................................

  // DMP-LABEL: StringLiteral {{.*}} "loop gang\n"
  // PRT-LABEL: printf("loop gang\n");
  // EXE-LABEL: loop gang
  printf("loop gang\n");

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(4){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(4){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(4){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(4){{$}}
  #pragma acc parallel num_gangs(4)
  // DMP:      ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  // DMP-NOT:      <implicit>
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 2
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang collapse(2){{$}}
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc loop gang collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG: 0, 0
      // EXE-TGT-USE-STDIO-DAG: 0, 1
      // EXE-TGT-USE-STDIO-DAG: 1, 0
      // EXE-TGT-USE-STDIO-DAG: 1, 1
      TGT_PRINTF("%d, %d\n", i, j);

  // DMP-LABEL: StringLiteral {{.*}} "parallel loop gang\n"
  // PRT-LABEL: printf("parallel loop gang\n");
  // EXE-LABEL: parallel loop gang
  printf("parallel loop gang\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   ACCGangClause
  // DMP-NOT:      <implicit>
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 3
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 8
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCGangClause
  // DMP-NOT:          <implicit>
  // DMP-NEXT:       ACCCollapseClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 3
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeDirective
  // DMP-NEXT:         OMPCollapseClause
  // DMP-NEXT:           ConstantExpr {{.*}} 'int'
  // DMP-NEXT:             value: Int 3
  // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 3
  // DMP:              ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(8) gang collapse(3){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(8){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(3){{$}}
  //
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(8){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute collapse(3){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(8) gang collapse(3){{$}}
  //
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc parallel loop num_gangs(8) gang collapse(3)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: ForStmt
      // PRT-NEXT: for (int k ={{.*}})
      for (int k = 0; k < 2; ++k)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // EXE-TGT-USE-STDIO-DAG: 0, 0, 0
        // EXE-TGT-USE-STDIO-DAG: 0, 0, 1
        // EXE-TGT-USE-STDIO-DAG: 0, 1, 0
        // EXE-TGT-USE-STDIO-DAG: 0, 1, 1
        // EXE-TGT-USE-STDIO-DAG: 1, 0, 0
        // EXE-TGT-USE-STDIO-DAG: 1, 0, 1
        // EXE-TGT-USE-STDIO-DAG: 1, 1, 0
        // EXE-TGT-USE-STDIO-DAG: 1, 1, 1
        TGT_PRINTF("%d, %d, %d\n", i, j, k);

  //............................................................................
  // More loops than necessary.
  //............................................................................

  // DMP-LABEL: StringLiteral {{.*}} "loop 1 {1}\n"
  // PRT-LABEL: printf("loop 1 {1}\n");
  // EXE-LABEL: loop 1 {1}
  printf("loop 1 {1}\n");

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 16
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 16
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(16){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(16){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(16){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(16){{$}}
  #pragma acc parallel num_gangs(16)
  // DMP:      ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  // DMP-NOT:      <implicit>
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 1
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 1
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
  // DMP:          ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang collapse(1){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(1){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute collapse(1){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang collapse(1){{$}}
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc loop gang collapse(1)
  for (int i = 0; i < 4; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 4; ++j)
      // DMP: CallExpr
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // For any i, values of j must print in ascending order.
      // EXE-TGT-USE-STDIO: 0, 0
      // EXE-TGT-USE-STDIO: 0, 1
      // EXE-TGT-USE-STDIO: 0, 2
      // EXE-TGT-USE-STDIO: 0, 3
      TGT_PRINTF("%d, %d\n", i, j);

  // DMP-LABEL: StringLiteral {{.*}} "loop 2 {1}\n"
  // PRT-LABEL: printf("loop 2 {1}\n");
  // EXE-LABEL: loop 2 {1}
  printf("loop 2 {1}\n");

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(8){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(8){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(8){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(8){{$}}
  #pragma acc parallel num_gangs(8)
  // DMP:      ACCLoopDirective
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 2
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc loop collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop collapse(2){{$}}
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc loop collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: ForStmt
      // PRT-NEXT: for (int k ={{.*}})
      for (int k = 0; k < 2; ++k)
        // DMP: CallExpr
        // PRT-NEXT: {{TGT_PRINTF|printf}}
        // For any pair (i,j), values of k must print in ascending order.
        // EXE-TGT-USE-STDIO: 1, 0, 0
        // EXE-TGT-USE-STDIO: 1, 0, 1
        TGT_PRINTF("%d, %d, %d\n", i, j, k);

  // DMP-LABEL: StringLiteral {{.*}} "parallel loop 2 {2}\n"
  // PRT-LABEL: printf("parallel loop 2 {2}\n");
  // EXE-LABEL: parallel loop 2 {2}
  printf("parallel loop 2 {2}\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 16
  // DMP-NEXT:   ACCGangClause
  // DMP-NOT:      <implicit>
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 16
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 16
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCGangClause
  // DMP-NOT:          <implicit>
  // DMP-NEXT:       ACCCollapseClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 2
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeDirective
  // DMP-NEXT:         OMPCollapseClause
  // DMP-NEXT:           ConstantExpr {{.*}} 'int'
  // DMP-NEXT:             value: Int 2
  // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 2
  // DMP:              ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(16) gang collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(16){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  //
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(16){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(16) gang collapse(2){{$}}
  //
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc parallel loop num_gangs(16) gang collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: ForStmt
      // PRT-NEXT: for (int k ={{.*}})
      for (int k = 0; k < 2; ++k)
        // DMP: ForStmt
        // PRT-NEXT: for (int l ={{.*}})
        for (int l = 0; l < 2; ++l)
          // DMP: CallExpr
          // PRT-NEXT: {{TGT_PRINTF|printf}}
          // For any pair (i,j), values of (k,l) must print in ascending order.
          // EXE-TGT-USE-STDIO: 0, 1, 0, 0
          // EXE-TGT-USE-STDIO: 0, 1, 0, 1
          // EXE-TGT-USE-STDIO: 0, 1, 1, 0
          // EXE-TGT-USE-STDIO: 0, 1, 1, 1
          TGT_PRINTF("%d, %d, %d, %d\n", i, j, k, l);

  //............................................................................
  // Nested collapse.
  //............................................................................

  // DMP-LABEL: StringLiteral {{.*}} "loop 2 {loop 2}\n"
  // PRT-LABEL: printf("loop 2 {loop 2}\n");
  // EXE-LABEL: loop 2 {loop 2}
  printf("loop 2 {loop 2}\n");

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(4) num_workers(4){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(4){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(4){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(4) num_workers(4){{$}}
  #pragma acc parallel num_gangs(4) num_workers(4)
  // DMP:      ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  // DMP-NOT:      <implicit>
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 2
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang collapse(2){{$}}
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc loop gang collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCCollapseClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 2
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
      // DMP-NEXT:   impl: OMPParallelForDirective
      // DMP-NEXT:     OMPNum_threadsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 4
      // DMP-NEXT:     OMPCollapseClause
      // DMP-NEXT:       ConstantExpr {{.*}} 'int'
      // DMP-NEXT:         value: Int 2
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
      //  DMP-NOT:     OMP
      //      DMP:     ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker collapse(2){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for num_threads(4) collapse(2){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for num_threads(4) collapse(2){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker collapse(2){{$}}
      // PRT-NEXT:    for (int k ={{.*}})
      #pragma acc loop worker collapse(2)
      for (int k = 0; k < 2; ++k)
        // DMP: ForStmt
        // PRT-NEXT: for (int l ={{.*}})
        for (int l = 0; l < 2; ++l)
          // DMP: CallExpr
          // PRT-NEXT: {{TGT_PRINTF|printf}}
          // EXE-TGT-USE-STDIO-DAG: 0, 0, 0, 0
          // EXE-TGT-USE-STDIO-DAG: 0, 0, 0, 1
          // EXE-TGT-USE-STDIO-DAG: 0, 0, 1, 0
          // EXE-TGT-USE-STDIO-DAG: 0, 0, 1, 1
          // EXE-TGT-USE-STDIO-DAG: 0, 1, 0, 0
          // EXE-TGT-USE-STDIO-DAG: 0, 1, 0, 1
          // EXE-TGT-USE-STDIO-DAG: 0, 1, 1, 0
          // EXE-TGT-USE-STDIO-DAG: 0, 1, 1, 1
          // EXE-TGT-USE-STDIO-DAG: 1, 0, 0, 0
          // EXE-TGT-USE-STDIO-DAG: 1, 0, 0, 1
          // EXE-TGT-USE-STDIO-DAG: 1, 0, 1, 0
          // EXE-TGT-USE-STDIO-DAG: 1, 0, 1, 1
          // EXE-TGT-USE-STDIO-DAG: 1, 1, 0, 0
          // EXE-TGT-USE-STDIO-DAG: 1, 1, 0, 1
          // EXE-TGT-USE-STDIO-DAG: 1, 1, 1, 0
          // EXE-TGT-USE-STDIO-DAG: 1, 1, 1, 1
          TGT_PRINTF("%d, %d, %d, %d\n", i, j, k, l);

  // DMP-LABEL: StringLiteral {{.*}} "parallel loop 2 {loop 2 {loop 2}}\n"
  // PRT-LABEL: printf("parallel loop 2 {loop 2 {loop 2}}\n");
  // EXE-LABEL: parallel loop 2 {loop 2 {loop 2}}
  printf("parallel loop 2 {loop 2 {loop 2}}\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     ACCNumWorkersClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 4
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCCollapseClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 2
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeDirective
  // DMP-NEXT:         OMPCollapseClause
  // DMP-NEXT:           ConstantExpr {{.*}} 'int'
  // DMP-NEXT:             value: Int 2
  // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 2
  // DMP:              ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(4) num_workers(4) vector_length(4) collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(4){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  //
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(4){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(4) num_workers(4) vector_length(4) collapse(2){{$}}
  //
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc parallel loop num_gangs(4) num_workers(4) vector_length(4) collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCCollapseClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 2
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
      // DMP-NEXT:   impl: OMPParallelForDirective
      // DMP-NEXT:     OMPNum_threadsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 4
      // DMP-NEXT:     OMPCollapseClause
      // DMP-NEXT:       ConstantExpr {{.*}} 'int'
      // DMP-NEXT:         value: Int 2
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
      //  DMP-NOT:     OMP
      //      DMP:     ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker collapse(2){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for num_threads(4) collapse(2){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for num_threads(4) collapse(2){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker collapse(2){{$}}
      // PRT-NEXT:    for (int k ={{.*}})
      #pragma acc loop worker collapse(2)
      for (int k = 0; k < 2; ++k)
        // DMP: ForStmt
        // PRT-NEXT: for (int l ={{.*}})
        for (int l = 0; l < 2; ++l)
          //      DMP: ACCLoopDirective
          // DMP-NEXT:   ACCVectorClause
          // DMP-NEXT:   ACCCollapseClause
          // DMP-NEXT:     IntegerLiteral {{.*}} 2
          // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
          // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
          // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
          // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
          // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
          // DMP-NEXT:     DeclRefExpr {{.*}} 'l' 'int'
          // DMP-NEXT:   impl: OMPSimdDirective
          // DMP-NEXT:     OMPSimdlenClause
          // DMP-NEXT:       ConstantExpr {{.*}} 'int'
          // DMP-NEXT:         value: Int 4
          // DMP-NEXT:         IntegerLiteral {{.*}} 4
          // DMP-NEXT:     OMPCollapseClause
          // DMP-NEXT:       ConstantExpr {{.*}} 'int'
          // DMP-NEXT:         value: Int 2
          // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
          //  DMP-NOT:     OMP
          //      DMP:     ForStmt
          //
          // PRT-A-NEXT:  {{^ *}}#pragma acc loop vector collapse(2){{$}}
          // PRT-AO-NEXT: {{^ *}}// #pragma omp simd simdlen(4) collapse(2){{$}}
          // PRT-O-NEXT:  {{^ *}}#pragma omp simd simdlen(4) collapse(2){{$}}
          // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector collapse(2){{$}}
          // PRT-NEXT:    for (int m ={{.*}})
          #pragma acc loop vector collapse(2)
          for (int m = 0; m < 2; ++m)
            // DMP: ForStmt
            // PRT-NEXT: for (int n ={{.*}})
            for (int n = 0; n < 2; ++n)
              // DMP: CallExpr
              // PRT-NEXT: {{TGT_PRINTF|printf}}({{.*}}
              // EXE-TGT-USE-STDIO-DAG: 0, 0, 0, 0, 0, 0
              // EXE-TGT-USE-STDIO-DAG: 0, 0, 0, 0, 0, 1
              // EXE-TGT-USE-STDIO-DAG: 0, 0, 0, 0, 1, 0
              // EXE-TGT-USE-STDIO-DAG: 0, 0, 0, 0, 1, 1
              // EXE-TGT-USE-STDIO-DAG: 0, 0, 0, 1, 0, 0
              // EXE-TGT-USE-STDIO-DAG: 0, 0, 0, 1, 0, 1
              // EXE-TGT-USE-STDIO-DAG: 0, 0, 0, 1, 1, 0
              // EXE-TGT-USE-STDIO-DAG: 0, 0, 0, 1, 1, 1
              // EXE-TGT-USE-STDIO-DAG: 0, 0, 1, 0, 0, 0
              // EXE-TGT-USE-STDIO-DAG: 0, 0, 1, 0, 0, 1
              // EXE-TGT-USE-STDIO-DAG: 0, 0, 1, 0, 1, 0
              // EXE-TGT-USE-STDIO-DAG: 0, 0, 1, 0, 1, 1
              // EXE-TGT-USE-STDIO-DAG: 0, 0, 1, 1, 0, 0
              // EXE-TGT-USE-STDIO-DAG: 0, 0, 1, 1, 0, 1
              // EXE-TGT-USE-STDIO-DAG: 0, 0, 1, 1, 1, 0
              // EXE-TGT-USE-STDIO-DAG: 0, 0, 1, 1, 1, 1
              // EXE-TGT-USE-STDIO-DAG: 0, 1, 0, 0, 0, 0
              // EXE-TGT-USE-STDIO-DAG: 0, 1, 0, 0, 0, 1
              // EXE-TGT-USE-STDIO-DAG: 0, 1, 0, 0, 1, 0
              // EXE-TGT-USE-STDIO-DAG: 0, 1, 0, 0, 1, 1
              // EXE-TGT-USE-STDIO-DAG: 0, 1, 0, 1, 0, 0
              // EXE-TGT-USE-STDIO-DAG: 0, 1, 0, 1, 0, 1
              // EXE-TGT-USE-STDIO-DAG: 0, 1, 0, 1, 1, 0
              // EXE-TGT-USE-STDIO-DAG: 0, 1, 0, 1, 1, 1
              // EXE-TGT-USE-STDIO-DAG: 0, 1, 1, 0, 0, 0
              // EXE-TGT-USE-STDIO-DAG: 0, 1, 1, 0, 0, 1
              // EXE-TGT-USE-STDIO-DAG: 0, 1, 1, 0, 1, 0
              // EXE-TGT-USE-STDIO-DAG: 0, 1, 1, 0, 1, 1
              // EXE-TGT-USE-STDIO-DAG: 0, 1, 1, 1, 0, 0
              // EXE-TGT-USE-STDIO-DAG: 0, 1, 1, 1, 0, 1
              // EXE-TGT-USE-STDIO-DAG: 0, 1, 1, 1, 1, 0
              // EXE-TGT-USE-STDIO-DAG: 0, 1, 1, 1, 1, 1
              // EXE-TGT-USE-STDIO-DAG: 1, 0, 0, 0, 0, 0
              // EXE-TGT-USE-STDIO-DAG: 1, 0, 0, 0, 0, 1
              // EXE-TGT-USE-STDIO-DAG: 1, 0, 0, 0, 1, 0
              // EXE-TGT-USE-STDIO-DAG: 1, 0, 0, 0, 1, 1
              // EXE-TGT-USE-STDIO-DAG: 1, 0, 0, 1, 0, 0
              // EXE-TGT-USE-STDIO-DAG: 1, 0, 0, 1, 0, 1
              // EXE-TGT-USE-STDIO-DAG: 1, 0, 0, 1, 1, 0
              // EXE-TGT-USE-STDIO-DAG: 1, 0, 0, 1, 1, 1
              // EXE-TGT-USE-STDIO-DAG: 1, 0, 1, 0, 0, 0
              // EXE-TGT-USE-STDIO-DAG: 1, 0, 1, 0, 0, 1
              // EXE-TGT-USE-STDIO-DAG: 1, 0, 1, 0, 1, 0
              // EXE-TGT-USE-STDIO-DAG: 1, 0, 1, 0, 1, 1
              // EXE-TGT-USE-STDIO-DAG: 1, 0, 1, 1, 0, 0
              // EXE-TGT-USE-STDIO-DAG: 1, 0, 1, 1, 0, 1
              // EXE-TGT-USE-STDIO-DAG: 1, 0, 1, 1, 1, 0
              // EXE-TGT-USE-STDIO-DAG: 1, 0, 1, 1, 1, 1
              // EXE-TGT-USE-STDIO-DAG: 1, 1, 0, 0, 0, 0
              // EXE-TGT-USE-STDIO-DAG: 1, 1, 0, 0, 0, 1
              // EXE-TGT-USE-STDIO-DAG: 1, 1, 0, 0, 1, 0
              // EXE-TGT-USE-STDIO-DAG: 1, 1, 0, 0, 1, 1
              // EXE-TGT-USE-STDIO-DAG: 1, 1, 0, 1, 0, 0
              // EXE-TGT-USE-STDIO-DAG: 1, 1, 0, 1, 0, 1
              // EXE-TGT-USE-STDIO-DAG: 1, 1, 0, 1, 1, 0
              // EXE-TGT-USE-STDIO-DAG: 1, 1, 0, 1, 1, 1
              // EXE-TGT-USE-STDIO-DAG: 1, 1, 1, 0, 0, 0
              // EXE-TGT-USE-STDIO-DAG: 1, 1, 1, 0, 0, 1
              // EXE-TGT-USE-STDIO-DAG: 1, 1, 1, 0, 1, 0
              // EXE-TGT-USE-STDIO-DAG: 1, 1, 1, 0, 1, 1
              // EXE-TGT-USE-STDIO-DAG: 1, 1, 1, 1, 0, 0
              // EXE-TGT-USE-STDIO-DAG: 1, 1, 1, 1, 0, 1
              // EXE-TGT-USE-STDIO-DAG: 1, 1, 1, 1, 1, 0
              // EXE-TGT-USE-STDIO-DAG: 1, 1, 1, 1, 1, 1
              TGT_PRINTF("%d, %d, %d, %d, %d, %d\n", i, j, k, l, m, n);

  //............................................................................
  // Private for loop control variable that is assigned not declared in init
  // of for loop that is attached via collapse.
  //
  // The first case checks gang partitioning only.
  //
  // The second case checks worker partitioning only (plus implicit gang
  // partitioning).
  //
  // The third case checks (1) a combined directive, (2) vector partitioning
  // only (plus implicit gang partitioning) (3) that privacy of a simd loop
  // control variable is handled via a compound statement and local declaration
  // instead of a private clause, and (4) that a declared loop control variable
  // doesn't throw off the count and cause nested but not associated loop
  // control variables to be private.
  //............................................................................

  // DMP-LABEL: StringLiteral {{.*}} "loop gang private control var\n"
  // PRT-LABEL: printf("loop gang private control var\n");
  // EXE-LABEL: loop gang private control var
  printf("loop gang private control var\n");

  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    // PRT: int j = 99;
    // PRT: int k = 99;
    int i = 99;
    int j = 99;
    int k = 99;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(8){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(8) firstprivate(k){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(8) firstprivate(k){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(8){{$}}
    #pragma acc parallel num_gangs(8)
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCCollapseClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 2
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   impl: OMPDistributeDirective
    // DMP-NEXT:     OMPCollapseClause
    // DMP-NEXT:       ConstantExpr {{.*}} 'int'
    // DMP-NEXT:         value: Int 2
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPPrivateClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang collapse(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2) private(i,j){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute collapse(2) private(i,j){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang collapse(2){{$}}
    // PRT-NEXT:    for (i ={{.*}})
    #pragma acc loop gang collapse(2)
    for (i = 0; i < 2; ++i)
      // DMP: ForStmt
      // PRT-NEXT: for (j ={{.*}})
      for (j = 0; j < 2; ++j)
        // DMP: ForStmt
        // PRT-NEXT: for (k ={{.*}})
        for (k = 0; k < 2; ++k)
          // DMP: CallExpr
          // PRT-NEXT: {{TGT_PRINTF|printf}}
          // EXE-TGT-USE-STDIO-DAG: 0, 0, 0
          // EXE-TGT-USE-STDIO-DAG: 0, 0, 1
          // EXE-TGT-USE-STDIO-DAG: 0, 1, 0
          // EXE-TGT-USE-STDIO-DAG: 0, 1, 1
          // EXE-TGT-USE-STDIO-DAG: 1, 0, 0
          // EXE-TGT-USE-STDIO-DAG: 1, 0, 1
          // EXE-TGT-USE-STDIO-DAG: 1, 1, 0
          // EXE-TGT-USE-STDIO-DAG: 1, 1, 1
          TGT_PRINTF("%d, %d, %d\n", i, j, k);
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "loop worker private control var\n"
  // PRT-LABEL: printf("loop worker private control var\n");
  // EXE-LABEL: loop worker private control var
  printf("loop worker private control var\n");

  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    // PRT: int j = 99;
    // PRT: int k = 99;
    int i = 99;
    int j = 99;
    int k = 99;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCNumWorkersClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1) num_workers(8){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1) firstprivate(k){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1) firstprivate(k){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) num_workers(8){{$}}
    #pragma acc parallel num_gangs(1) num_workers(8)
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCCollapseClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 2
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForDirective
    // DMP-NEXT:     OMPNum_threadsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 8
    // DMP-NEXT:     OMPCollapseClause
    // DMP-NEXT:       ConstantExpr {{.*}} 'int'
    // DMP-NEXT:         value: Int 2
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPPrivateClause {{.*}}
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //      DMP:       ForStmt
    //      DMP:         CallExpr
    //      DMP:         ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker collapse(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(8) collapse(2) private(i,j){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for num_threads(8) collapse(2) private(i,j){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker collapse(2){{$}}
    //    PRT-NEXT: for (i ={{.*}}) {
    //    PRT-NEXT:   for (j ={{.*}}) {
    //    PRT-NEXT:     {{TGT_PRINTF|printf}}
    //    PRT-NEXT:     for (k ={{.*}})
    //    PRT-NEXT:       {{TGT_PRINTF|printf}}
    //    PRT-NEXT:   }
    //    PRT-NEXT: }
    //
    // Because k is shared among workers, writes to it are a race, and so the k
    // loop might not have any iterations for some (i,j) pairs.
    // EXE-TGT-USE-STDIO-DAG: {{^0, 0$}}
    // EXE-TGT-USE-STDIO-DAG: {{^0, 1$}}
    // EXE-TGT-USE-STDIO-DAG: {{^1, 0$}}
    // EXE-TGT-USE-STDIO-DAG: {{^1, 1$}}
    // EXE-TGT-USE-STDIO-DAG: {{^[0-9], [0-9], [0-9]$}}
    #pragma acc loop worker collapse(2)
    for (i = 0; i < 2; ++i) {
      for (j = 0; j < 2; ++j) {
        TGT_PRINTF("%d, %d\n", i, j);
        for (k = 0; k < 2; ++k)
          TGT_PRINTF("%d, %d, %d\n", i, j, k);
      }
    }
  } // PRT-NEXT: }

  // DMP-LABEL: StringLiteral {{.*}} "parallel loop vector private control var\n"
  // PRT-LABEL: printf("parallel loop vector private control var\n");
  // EXE-LABEL: parallel loop vector private control var
  printf("parallel loop vector private control var\n");

  // PRT-NEXT: {
  {
    // PRT: int j = 99;
    // PRT: int k = 99;
    int j = 99;
    int k = 99;
    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCVectorLengthClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCCollapseClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 2
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:     ACCVectorLengthClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
    // DMP-NEXT:     ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:     ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    //      DMP:     ACCLoopDirective
    // DMP-NEXT:       ACCVectorClause
    // DMP-NEXT:       ACCCollapseClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 2
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCPrivateClause {{.*}} <predetermined>
    // DMP-NEXT:         DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:         DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:       impl: CompoundStmt
    // DMP-NEXT:         DeclStmt
    // DMP-NEXT:           VarDecl {{.*}} j 'int'
    // DMP-NEXT:         OMPDistributeSimdDirective
    // DMP-NEXT:           OMPSimdlenClause
    // DMP-NEXT:             ConstantExpr {{.*}} 'int'
    // DMP-NEXT:               value: Int 8
    // DMP-NEXT:               IntegerLiteral {{.*}} 8
    // DMP-NEXT:           OMPCollapseClause
    // DMP-NEXT:             ConstantExpr {{.*}} 'int'
    // DMP-NEXT:               value: Int 2
    // DMP-NEXT:               IntegerLiteral {{.*}} 'int' 2
    //  DMP-NOT:           OMP
    //      DMP:           ForStmt
    //      DMP:             ForStmt
    //      DMP:               CallExpr
    //      DMP:               ForStmt
    //
    // PRT-NOACC-NEXT: for (int i ={{.*}}) {
    // PRT-NOACC-NEXT:   for (j ={{.*}}) {
    // PRT-NOACC-NEXT:     {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT:     for (k ={{.*}})
    // PRT-NOACC-NEXT:       ;
    // PRT-NOACC-NEXT:   }
    // PRT-NOACC-NEXT: }
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    // PRT-A-NEXT:  #pragma acc parallel loop num_gangs(1) vector_length(8) vector collapse(2){{$}}
    // PRT-A-NEXT:  for (int i ={{.*}}) {
    // PRT-A-NEXT:    for (j ={{.*}}) {
    // PRT-A-NEXT:      {{TGT_PRINTF|printf}}
    // PRT-A-NEXT:      for (k ={{.*}})
    // PRT-A-NEXT:        ;
    // PRT-A-NEXT:    }
    // PRT-A-NEXT:  }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(1) firstprivate(k){{$}}
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int j;
    // PRT-AO-NEXT: //   #pragma omp distribute simd simdlen(8) collapse(2){{$}}
    // PRT-AO-NEXT: //   for (int i ={{.*}}) {
    // PRT-AO-NEXT: //     for (j ={{.*}}) {
    // PRT-AO-NEXT: //       {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //       for (k ={{.*}})
    // PRT-AO-NEXT: //         ;
    // PRT-AO-NEXT: //     }
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    // PRT-O-NEXT:  #pragma omp target teams num_teams(1) firstprivate(k){{$}}
    // PRT-O-NEXT:  {
    // PRT-O-NEXT:    int j;
    // PRT-O-NEXT:    #pragma omp distribute simd simdlen(8) collapse(2){{$}}
    // PRT-O-NEXT:    for (int i ={{.*}}) {
    // PRT-O-NEXT:      for (j ={{.*}}) {
    // PRT-O-NEXT:        {{TGT_PRINTF|printf}}
    // PRT-O-NEXT:        for (k ={{.*}})
    // PRT-O-NEXT:          ;
    // PRT-O-NEXT:      }
    // PRT-O-NEXT:    }
    // PRT-O-NEXT:  }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc parallel loop num_gangs(1) vector_length(8) vector collapse(2){{$}}
    // PRT-OA-NEXT: // for (int i ={{.*}}) {
    // PRT-OA-NEXT: //   for (j ={{.*}}) {
    // PRT-OA-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: //     for (k ={{.*}})
    // PRT-OA-NEXT: //       ;
    // PRT-OA-NEXT: //   }
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // Because k is shared among vector lanes, writes to it are a race, and so
    // the k loop might not have any iterations for some (i,j) pairs.
    // EXE-TGT-USE-STDIO-DAG: 0, 0
    // EXE-TGT-USE-STDIO-DAG: 0, 1
    // EXE-TGT-USE-STDIO-DAG: 1, 0
    // EXE-TGT-USE-STDIO-DAG: 1, 1
    #pragma acc parallel loop num_gangs(1) vector_length(8) vector collapse(2)
    for (int i = 0; i < 2; ++i) {
      for (j = 0; j < 2; ++j) {
        TGT_PRINTF("%d, %d\n", i, j);
        for (k = 0; k < 2; ++k)
          ;
      }
    }
  } // PRT-NEXT: }

  //............................................................................
  // Check collapse clauses within accelerator routines.
  //............................................................................

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
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(2) num_workers(2) vector_length(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2){{$}}
  //    PRT-NEXT: {
  //    PRT-NEXT:   withinGangFn();
  //    PRT-NEXT:   withinWorkerFn();
  //    PRT-NEXT:   withinVectorFn();
  //    PRT-NEXT:   withinSeqFn();
  //    PRT-NEXT: }
  #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2)
  {
    withinGangFn();
    withinWorkerFn();
    withinVectorFn();
    withinSeqFn();
  }

  // PRT-NEXT: return 0;
  return 0;
} // PRT-NEXT: }

//------------------------------------------------------------------------------
// Check collapse clauses within a gang function.
//
// This repeats some of the above testing but within a gang function and without
// statically enclosing parallel constructs.
//
// For execution checks, we just focus on checking that all loops iterations are
// executed at least the right number of times.  There are some clever checks on
// order in the corresponding checks above, but those don't look for all
// iterations.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} withinGangFn
// PRT-LABEL: void withinGangFn() {
void withinGangFn() {
  //............................................................................
  // Explicit seq.
  //............................................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCSeqClause
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq collapse(2)
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq collapse(2) // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop seq: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop seq: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop seq: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop seq: 1, 1
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop seq: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop seq: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop seq: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop seq: 1, 1
  #pragma acc loop seq collapse(2)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinGangFn: acc loop seq: %d, %d\n", i, j);

  //............................................................................
  // Explicit auto.
  //............................................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto collapse(2)
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto collapse(2) // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop auto: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop auto: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop auto: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop auto: 1, 1
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop auto: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop auto: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop auto: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop auto: 1, 1
  #pragma acc loop auto collapse(2)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinGangFn: acc loop auto: %d, %d\n", i, j);

  //............................................................................
  // Implicit gang partitioning.
  //............................................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 2
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ForStmt
  //      DMP:       ForStmt
  //      DMP:         CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop collapse(2){{$}}
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop: 1, 1
  #pragma acc loop collapse(2)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinGangFn: acc loop: %d, %d\n", i, j);

  //............................................................................
  // Explicit gang partitioning.
  //............................................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 2
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ForStmt
  //      DMP:       ForStmt
  //      DMP:         CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang collapse(2){{$}}
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop gang: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop gang: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop gang: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop gang: 1, 1
  #pragma acc loop gang collapse(2)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinGangFn: acc loop gang: %d, %d\n", i, j);

  //............................................................................
  // Worker partitioned.
  //............................................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCWorkerClause
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeParallelForDirective
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 2
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ForStmt
  //      DMP:       ForStmt
  //      DMP:         CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for collapse(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker collapse(2){{$}}
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop worker: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop worker: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop worker: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop worker: 1, 1
  #pragma acc loop worker collapse(2)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinGangFn: acc loop worker: %d, %d\n", i, j);

  //............................................................................
  // Vector partitioned.
  //............................................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCVectorClause
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeSimdDirective
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 2
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ForStmt
  //      DMP:       ForStmt
  //      DMP:         CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd collapse(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute simd collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector collapse(2){{$}}
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop vector: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop vector: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop vector: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop vector: 1, 1
  #pragma acc loop vector collapse(2)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinGangFn: acc loop vector: %d, %d\n", i, j);

  //............................................................................
  // More loops than necessary.
  //............................................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  //  DMP-NOT:   <implicit>
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 1
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 1
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
  //      DMP:     ForStmt
  //      DMP:       ForStmt
  //      DMP:         CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang collapse(1){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(1){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute collapse(1){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang collapse(1){{$}}
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 1 {1}: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 1 {1}: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 1 {1}: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 1 {1}: 1, 1
  #pragma acc loop gang collapse(1)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinGangFn: loop 1 {1}: %d, %d\n", i, j);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 2
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ForStmt
  //      DMP:       ForStmt
  //      DMP:         ForStmt
  //      DMP:           CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop collapse(2){{$}}
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     for (int k ={{.*}})
  //    PRT-NEXT:       {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {1}: 0, 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {1}: 0, 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {1}: 0, 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {1}: 0, 1, 1
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {1}: 1, 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {1}: 1, 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {1}: 1, 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {1}: 1, 1, 1
  #pragma acc loop collapse(2)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      for (int k = 0; k < 2; ++k)
        TGT_PRINTF("withinGangFn: loop 2 {1}: %d, %d, %d\n", i, j, k);

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  //  DMP-NOT:   <implicit>
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 2
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ForStmt
  //      DMP:       ForStmt
  //      DMP:         ForStmt
  //      DMP:           ForStmt
  //      DMP:             CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang collapse(2){{$}}
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     for (int k ={{.*}})
  //    PRT-NEXT:       for (int l ={{.*}})
  //    PRT-NEXT:         {{TGT_PRINTF|printf}}({{.*}});
  //
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {2}: 0, 0, 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {2}: 0, 0, 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {2}: 0, 0, 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {2}: 0, 0, 1, 1
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {2}: 0, 1, 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {2}: 0, 1, 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {2}: 0, 1, 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {2}: 0, 1, 1, 1
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {2}: 1, 0, 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {2}: 1, 0, 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {2}: 1, 0, 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {2}: 1, 0, 1, 1
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {2}: 1, 1, 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {2}: 1, 1, 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {2}: 1, 1, 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {2}: 1, 1, 1, 1
  #pragma acc loop gang collapse(2)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      for (int k = 0; k < 2; ++k)
        for (int l = 0; l < 2; ++l)
          TGT_PRINTF("withinGangFn: loop 2 {2}: %d, %d, %d, %d\n", i, j, k, l);

  //............................................................................
  // Nested collapse.
  //............................................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  //  DMP-NOT:   <implicit>
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 2
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ForStmt
  //      DMP:       ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang collapse(2){{$}}
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  #pragma acc loop gang collapse(2)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCCollapseClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 2
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
      // DMP-NEXT:   impl: OMPParallelForDirective
      // DMP-NEXT:     OMPCollapseClause
      // DMP-NEXT:       ConstantExpr {{.*}} 'int'
      // DMP-NEXT:         value: Int 2
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
      //  DMP-NOT:     OMP
      //      DMP:     ForStmt
      //      DMP:       ForStmt
      //      DMP:         CallExpr
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker collapse(2){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for collapse(2){{$}}
      //  PRT-O-NEXT: {{^ *}}#pragma omp parallel for collapse(2){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker collapse(2){{$}}
      //    PRT-NEXT: for (int k ={{.*}})
      //    PRT-NEXT:   for (int l ={{.*}})
      //    PRT-NEXT:     {{TGT_PRINTF|printf}}({{.*}});
      //
      // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {loop 2}: 0, 0, 0, 0
      // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {loop 2}: 0, 0, 0, 1
      // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {loop 2}: 0, 0, 1, 0
      // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {loop 2}: 0, 0, 1, 1
      // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {loop 2}: 0, 1, 0, 0
      // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {loop 2}: 0, 1, 0, 1
      // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {loop 2}: 0, 1, 1, 0
      // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {loop 2}: 0, 1, 1, 1
      // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {loop 2}: 1, 0, 0, 0
      // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {loop 2}: 1, 0, 0, 1
      // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {loop 2}: 1, 0, 1, 0
      // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {loop 2}: 1, 0, 1, 1
      // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {loop 2}: 1, 1, 0, 0
      // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {loop 2}: 1, 1, 0, 1
      // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {loop 2}: 1, 1, 1, 0
      // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop 2 {loop 2}: 1, 1, 1, 1
      #pragma acc loop worker collapse(2)
      for (int k = 0; k < 2; ++k)
        for (int l = 0; l < 2; ++l)
          TGT_PRINTF("withinGangFn: loop 2 {loop 2}: %d, %d, %d, %d\n", i, j, k, l);

  //............................................................................
  // Private for loop control variable that is assigned not declared in init of
  // for loop that is attached via collapse.
  //
  // The first case checks gang partitioning only.
  //
  // The second case checks worker partitioning only (plus implicit gang
  // partitioning).
  //
  // The third case checks (1) vector partitioning only (plus implicit gang
  // partitioning), (2) that privacy of a simd loop control variable is handled
  // via a compound statement and local declaration instead of a private clause,
  // and (3) that a declared loop control variable doesn't throw off the count
  // and cause nested but not associated loop control variables to be private.
  //............................................................................

  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    // PRT: int j = 99;
    // PRT: int k = 99;
    int i = 99;
    int j = 99;
    int k = 99;
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    //  DMP-NOT:   <implicit>
    // DMP-NEXT:   ACCCollapseClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 2
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   impl: OMPDistributeDirective
    // DMP-NEXT:     OMPCollapseClause
    // DMP-NEXT:       ConstantExpr {{.*}} 'int'
    // DMP-NEXT:         value: Int 2
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPPrivateClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //      DMP:       ForStmt
    //      DMP:         ForStmt
    //      DMP:           CallExpr
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang collapse(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2) private(i,j){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute collapse(2) private(i,j){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang collapse(2){{$}}
    //    PRT-NEXT: for (i ={{.*}})
    //    PRT-NEXT:   for (j ={{.*}})
    //    PRT-NEXT:     for (k ={{.*}})
    //    PRT-NEXT:       {{TGT_PRINTF|printf}}
    //
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop gang private control var: 0, 0, 0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop gang private control var: 0, 0, 1
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop gang private control var: 0, 1, 0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop gang private control var: 0, 1, 1
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop gang private control var: 1, 0, 0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop gang private control var: 1, 0, 1
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop gang private control var: 1, 1, 0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop gang private control var: 1, 1, 1
    #pragma acc loop gang collapse(2)
    for (i = 0; i < 2; ++i)
      for (j = 0; j < 2; ++j)
        for (k = 0; k < 2; ++k)
          TGT_PRINTF("withinGangFn: loop gang private control var: %d, %d, %d\n", i, j, k);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    // PRT: int j = 99;
    // PRT: int k = 99;
    int i = 99;
    int j = 99;
    int k = 99;
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCCollapseClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 2
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForDirective
    // DMP-NEXT:     OMPCollapseClause
    // DMP-NEXT:       ConstantExpr {{.*}} 'int'
    // DMP-NEXT:         value: Int 2
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPPrivateClause {{.*}}
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //      DMP:       ForStmt
    //      DMP:         CallExpr
    //      DMP:         ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker collapse(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for collapse(2) private(i,j){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for collapse(2) private(i,j){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker collapse(2){{$}}
    //    PRT-NEXT: for (i ={{.*}}) {
    //    PRT-NEXT:   for (j ={{.*}}) {
    //    PRT-NEXT:     {{TGT_PRINTF|printf}}
    //    PRT-NEXT:     for (k ={{.*}})
    //    PRT-NEXT:       ;
    //    PRT-NEXT:   }
    //    PRT-NEXT: }
    //
    // Because k is shared among workers, writes to it are a race, and so the k
    // loop might not have any iterations for some (i,j) pairs.
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop worker private control var: 0, 0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop worker private control var: 0, 1
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop worker private control var: 1, 0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop worker private control var: 1, 1
    #pragma acc loop worker collapse(2)
    for (i = 0; i < 2; ++i) {
      for (j = 0; j < 2; ++j) {
        TGT_PRINTF("withinGangFn: loop worker private control var: %d, %d\n", i, j);
        for (k = 0; k < 2; ++k)
          ;
      }
    }
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT: int j = 99;
    // PRT: int k = 99;
    int j = 99;
    int k = 99;
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCCollapseClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 2
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} j 'int'
    // DMP-NEXT:     OMPDistributeSimdDirective
    // DMP-NEXT:       OMPCollapseClause
    // DMP-NEXT:         ConstantExpr {{.*}} 'int'
    // DMP-NEXT:           value: Int 2
    // DMP-NEXT:           IntegerLiteral {{.*}} 'int' 2
    //  DMP-NOT:       OMP
    //      DMP:       ForStmt
    //      DMP:         ForStmt
    //      DMP:           CallExpr
    //      DMP:           ForStmt
    //
    // PRT-NOACC-NEXT: for (int i ={{.*}}) {
    // PRT-NOACC-NEXT:   for (j ={{.*}}) {
    // PRT-NOACC-NEXT:     {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT:     for (k ={{.*}})
    // PRT-NOACC-NEXT:       ;
    // PRT-NOACC-NEXT:   }
    // PRT-NOACC-NEXT: }
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: #pragma acc loop vector collapse(2){{$}}
    //  PRT-A-NEXT: for (int i ={{.*}}) {
    //  PRT-A-NEXT:   for (j ={{.*}}) {
    //  PRT-A-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT:     for (k ={{.*}})
    //  PRT-A-NEXT:       ;
    //  PRT-A-NEXT:   }
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int j;
    // PRT-AO-NEXT: //   #pragma omp distribute simd collapse(2){{$}}
    // PRT-AO-NEXT: //   for (int i ={{.*}}) {
    // PRT-AO-NEXT: //     for (j ={{.*}}) {
    // PRT-AO-NEXT: //       {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //       for (k ={{.*}})
    // PRT-AO-NEXT: //         ;
    // PRT-AO-NEXT: //     }
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int j;
    //  PRT-O-NEXT:   #pragma omp distribute simd collapse(2){{$}}
    //  PRT-O-NEXT:   for (int i ={{.*}}) {
    //  PRT-O-NEXT:     for (j ={{.*}}) {
    //  PRT-O-NEXT:       {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:       for (k ={{.*}})
    //  PRT-O-NEXT:         ;
    //  PRT-O-NEXT:     }
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop vector collapse(2){{$}}
    // PRT-OA-NEXT: // for (int i ={{.*}}) {
    // PRT-OA-NEXT: //   for (j ={{.*}}) {
    // PRT-OA-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: //     for (k ={{.*}})
    // PRT-OA-NEXT: //       ;
    // PRT-OA-NEXT: //   }
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // Because k is shared among vector lanes, writes to it are a race, and so
    // the k loop might not have any iterations for some (i,j) pairs.
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop vector private control var: 0, 0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop vector private control var: 0, 1
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop vector private control var: 1, 0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: loop vector private control var: 1, 1
    #pragma acc loop vector collapse(2)
    for (int i = 0; i < 2; ++i) {
      for (j = 0; j < 2; ++j) {
        TGT_PRINTF("withinGangFn: loop vector private control var: %d, %d\n", i, j);
        for (k = 0; k < 2; ++k)
          ;
      }
    }
  } // PRT-NEXT: }
} // PRT-NEXT: }

//------------------------------------------------------------------------------
// Check collapse clauses within a worker function.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} withinWorkerFn
// PRT-LABEL: void withinWorkerFn() {
void withinWorkerFn() {
  //............................................................................
  // Explicit seq.
  //............................................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCSeqClause
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq collapse(2)
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq collapse(2) // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop seq: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop seq: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop seq: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop seq: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop seq: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop seq: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop seq: 1, 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop seq: 1, 1
  #pragma acc loop seq collapse(2)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinWorkerFn: acc loop seq: %d, %d\n", i, j);

  //............................................................................
  // Explicit auto.
  //............................................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto collapse(2)
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto collapse(2) // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop auto: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop auto: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop auto: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop auto: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop auto: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop auto: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop auto: 1, 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop auto: 1, 1
  #pragma acc loop auto collapse(2)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinWorkerFn: acc loop auto: %d, %d\n", i, j);

  //............................................................................
  // No level of parallelism specified.
  //............................................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop collapse(2)
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop collapse(2) // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop: 1, 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop: 1, 1
  #pragma acc loop collapse(2)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinWorkerFn: acc loop: %d, %d\n", i, j);

  //............................................................................
  // Worker partitioned.
  //............................................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCWorkerClause
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPParallelForDirective
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 2
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ForStmt
  //      DMP:       ForStmt
  //      DMP:         CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for collapse(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp parallel for collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker collapse(2){{$}}
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop worker: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop worker: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop worker: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop worker: 1, 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop worker: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop worker: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop worker: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop worker: 1, 1
  #pragma acc loop worker collapse(2)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinWorkerFn: acc loop worker: %d, %d\n", i, j);

  //............................................................................
  // Vector partitioned.
  //............................................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCVectorClause
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPSimdDirective
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 2
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ForStmt
  //      DMP:       ForStmt
  //      DMP:         CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp simd collapse(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp simd collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector collapse(2){{$}}
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop vector: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop vector: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop vector: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop vector: 1, 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop vector: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop vector: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop vector: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop vector: 1, 1
  #pragma acc loop vector collapse(2)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinWorkerFn: acc loop vector: %d, %d\n", i, j);

  //............................................................................
  // More loops than necessary.
  //............................................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 1
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPParallelForDirective
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 1
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
  //      DMP:     ForStmt
  //      DMP:       ForStmt
  //      DMP:         CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker collapse(1){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for collapse(1){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp parallel for collapse(1){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker collapse(1){{$}}
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     {{TGT_PRINTF|printf}}({{.*}});
  //
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 1 {1}: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 1 {1}: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 1 {1}: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 1 {1}: 1, 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 1 {1}: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 1 {1}: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 1 {1}: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 1 {1}: 1, 1
  #pragma acc loop worker collapse(1)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinWorkerFn: loop 1 {1}: %d, %d\n", i, j);

  //............................................................................
  // Nested collapse.
  //............................................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:   <implicit>
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPParallelForDirective
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 2
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ForStmt
  //      DMP:       ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for collapse(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp parallel for collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker collapse(2){{$}}
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  #pragma acc loop worker collapse(2)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCCollapseClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 2
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
      // DMP-NEXT:   impl: OMPSimdDirective
      // DMP-NEXT:     OMPCollapseClause
      // DMP-NEXT:       ConstantExpr {{.*}} 'int'
      // DMP-NEXT:         value: Int 2
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
      //  DMP-NOT:     OMP
      //      DMP:     ForStmt
      //      DMP:       ForStmt
      //      DMP:         CallExpr
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector collapse(2){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp simd collapse(2){{$}}
      //  PRT-O-NEXT: {{^ *}}#pragma omp simd collapse(2){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector collapse(2){{$}}
      //    PRT-NEXT: for (int k ={{.*}})
      //    PRT-NEXT:   for (int l ={{.*}})
      //    PRT-NEXT:     {{TGT_PRINTF|printf}}({{.*}});
      //
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 0, 0, 0, 0
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 0, 0, 0, 1
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 0, 0, 1, 0
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 0, 0, 1, 1
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 0, 1, 0, 0
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 0, 1, 0, 1
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 0, 1, 1, 0
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 0, 1, 1, 1
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 1, 0, 0, 0
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 1, 0, 0, 1
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 1, 0, 1, 0
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 1, 0, 1, 1
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 1, 1, 0, 0
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 1, 1, 0, 1
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 1, 1, 1, 0
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 1, 1, 1, 1
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 0, 0, 0, 0
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 0, 0, 0, 1
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 0, 0, 1, 0
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 0, 0, 1, 1
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 0, 1, 0, 0
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 0, 1, 0, 1
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 0, 1, 1, 0
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 0, 1, 1, 1
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 1, 0, 0, 0
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 1, 0, 0, 1
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 1, 0, 1, 0
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 1, 0, 1, 1
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 1, 1, 0, 0
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 1, 1, 0, 1
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 1, 1, 1, 0
      // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop 2 {loop 2}: 1, 1, 1, 1
      #pragma acc loop vector collapse(2)
      for (int k = 0; k < 2; ++k)
        for (int l = 0; l < 2; ++l)
          TGT_PRINTF("withinWorkerFn: loop 2 {loop 2}: %d, %d, %d, %d\n", i, j, k, l);

  //............................................................................
  // Private for loop control variable that is assigned not declared in init of
  // for loop that is attached via collapse.
  //
  // The first case checks worker partitioning only.
  //
  // The second case checks (1) vector partitioning only, (2) that privacy
  // of a simd loop control variable is handled via a compound statement
  // and local declaration instead of a private clause, and (3) that a
  // declared loop control variable doesn't throw off the count and cause
  // nested but not associated loop control variables to be private.
  //............................................................................

  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    // PRT: int j = 99;
    // PRT: int k = 99;
    int i = 99;
    int j = 99;
    int k = 99;
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    //  DMP-NOT:   <implicit>
    // DMP-NEXT:   ACCCollapseClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 2
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   impl: OMPParallelForDirective
    // DMP-NEXT:     OMPCollapseClause
    // DMP-NEXT:       ConstantExpr {{.*}} 'int'
    // DMP-NEXT:         value: Int 2
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPPrivateClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
    //  DMP-NOT:     OMP
    //      DMP:     ForStmt
    //      DMP:       ForStmt
    //      DMP:         CallExpr
    //      DMP:           ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker collapse(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for collapse(2) private(i,j){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp parallel for collapse(2) private(i,j){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker collapse(2){{$}}
    //    PRT-NEXT: for (i ={{.*}}) {
    //    PRT-NEXT:   for (j ={{.*}}) {
    //    PRT-NEXT:     {{TGT_PRINTF|printf}}
    //    PRT-NEXT:     for (k ={{.*}})
    //    PRT-NEXT:       ;
    //    PRT-NEXT:   }
    //    PRT-NEXT: }
    //
    // Because k is shared among workers, writes to it are a race, and so the k
    // loop might not have any iterations for some (i,j) pairs.
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop worker private control var: 0, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop worker private control var: 0, 1{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop worker private control var: 1, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop worker private control var: 1, 1{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop worker private control var: 0, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop worker private control var: 0, 1{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop worker private control var: 1, 0{{$}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop worker private control var: 1, 1{{$}}
    #pragma acc loop worker collapse(2)
    for (i = 0; i < 2; ++i) {
      for (j = 0; j < 2; ++j) {
        TGT_PRINTF("withinWorkerFn: loop worker private control var: %d, %d\n", i, j);
        for (k = 0; k < 2; ++k)
          ;
      }
    }
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT: int j = 99;
    // PRT: int k = 99;
    int j = 99;
    int k = 99;
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCCollapseClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 2
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} j 'int'
    // DMP-NEXT:     OMPSimdDirective
    // DMP-NEXT:       OMPCollapseClause
    // DMP-NEXT:         ConstantExpr {{.*}} 'int'
    // DMP-NEXT:           value: Int 2
    // DMP-NEXT:           IntegerLiteral {{.*}} 'int' 2
    //  DMP-NOT:       OMP
    //      DMP:       ForStmt
    //      DMP:         ForStmt
    //      DMP:           CallExpr
    //      DMP:           ForStmt
    //
    // PRT-NOACC-NEXT: for (int i ={{.*}}) {
    // PRT-NOACC-NEXT:   for (j ={{.*}}) {
    // PRT-NOACC-NEXT:     {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT:     for (k ={{.*}})
    // PRT-NOACC-NEXT:       ;
    // PRT-NOACC-NEXT:   }
    // PRT-NOACC-NEXT: }
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: #pragma acc loop vector collapse(2){{$}}
    //  PRT-A-NEXT: for (int i ={{.*}}) {
    //  PRT-A-NEXT:   for (j ={{.*}}) {
    //  PRT-A-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT:     for (k ={{.*}})
    //  PRT-A-NEXT:       ;
    //  PRT-A-NEXT:   }
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int j;
    // PRT-AO-NEXT: //   #pragma omp simd collapse(2){{$}}
    // PRT-AO-NEXT: //   for (int i ={{.*}}) {
    // PRT-AO-NEXT: //     for (j ={{.*}}) {
    // PRT-AO-NEXT: //       {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //       for (k ={{.*}})
    // PRT-AO-NEXT: //         ;
    // PRT-AO-NEXT: //     }
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int j;
    //  PRT-O-NEXT:   #pragma omp simd collapse(2){{$}}
    //  PRT-O-NEXT:   for (int i ={{.*}}) {
    //  PRT-O-NEXT:     for (j ={{.*}}) {
    //  PRT-O-NEXT:       {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:       for (k ={{.*}})
    //  PRT-O-NEXT:         ;
    //  PRT-O-NEXT:     }
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop vector collapse(2){{$}}
    // PRT-OA-NEXT: // for (int i ={{.*}}) {
    // PRT-OA-NEXT: //   for (j ={{.*}}) {
    // PRT-OA-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: //     for (k ={{.*}})
    // PRT-OA-NEXT: //       ;
    // PRT-OA-NEXT: //   }
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // Because k is shared among vector lanes, writes to it are a race, and so
    // the k loop might not have any iterations for some (i,j) pairs.
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop vector private control var: 0, 0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop vector private control var: 0, 1
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop vector private control var: 1, 0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop vector private control var: 1, 1
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop vector private control var: 0, 0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop vector private control var: 0, 1
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop vector private control var: 1, 0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: loop vector private control var: 1, 1
    #pragma acc loop vector collapse(2)
    for (int i = 0; i < 2; ++i) {
      for (j = 0; j < 2; ++j) {
        TGT_PRINTF("withinWorkerFn: loop vector private control var: %d, %d\n", i, j);
        for (k = 0; k < 2; ++k)
          ;
      }
    }
  } // PRT-NEXT: }
} // PRT-NEXT: }

//------------------------------------------------------------------------------
// Check collapse clauses within a vector function.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} withinVectorFn
// PRT-LABEL: void withinVectorFn() {
void withinVectorFn() {
  //............................................................................
  // Explicit seq.
  //............................................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCSeqClause
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq collapse(2)
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq collapse(2) // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop seq: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop seq: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop seq: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop seq: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop seq: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop seq: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop seq: 1, 1
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop seq: 1, 1
  #pragma acc loop seq collapse(2)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinVectorFn: acc loop seq: %d, %d\n", i, j);

  //............................................................................
  // Explicit auto.
  //............................................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto collapse(2)
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto collapse(2) // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop auto: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop auto: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop auto: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop auto: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop auto: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop auto: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop auto: 1, 1
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop auto: 1, 1
  #pragma acc loop auto collapse(2)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinVectorFn: acc loop auto: %d, %d\n", i, j);

  //............................................................................
  // No level of parallelism specified.
  //............................................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop collapse(2)
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop collapse(2) // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop: 1, 1
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop: 1, 1
  #pragma acc loop collapse(2)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinVectorFn: acc loop: %d, %d\n", i, j);

  //............................................................................
  // Vector partitioned.
  //............................................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCVectorClause
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPSimdDirective
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 2
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //      DMP:     ForStmt
  //      DMP:       ForStmt
  //      DMP:         CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp simd collapse(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp simd collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector collapse(2){{$}}
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop vector: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop vector: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop vector: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop vector: 1, 1
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop vector: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop vector: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop vector: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop vector: 1, 1
  #pragma acc loop vector collapse(2)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinVectorFn: acc loop vector: %d, %d\n", i, j);

  //............................................................................
  // More loops than necessary.
  //............................................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 1
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPSimdDirective
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 1
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
  //      DMP:     ForStmt
  //      DMP:       ForStmt
  //      DMP:         CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector collapse(1){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp simd collapse(1){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp simd collapse(1){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector collapse(1){{$}}
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     {{TGT_PRINTF|printf}}({{.*}});
  //
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: loop 1 {1}: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: loop 1 {1}: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: loop 1 {1}: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: loop 1 {1}: 1, 1
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: loop 1 {1}: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: loop 1 {1}: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: loop 1 {1}: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: loop 1 {1}: 1, 1
  #pragma acc loop vector collapse(1)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinVectorFn: loop 1 {1}: %d, %d\n", i, j);

  //............................................................................
  // Private for loop control variable that is assigned not declared in init of
  // for loop that is attached via collapse.
  //
  // This checks (1) vector partitioning only, (2) that privacy of a simd loop
  // control variable is handled via a compound statement and local declaration
  // instead of a private clause, and (3) that a declared loop control variable
  // doesn't throw off the count and cause nested but not associated loop
  // control variables to be private.
  //............................................................................

  // PRT-NEXT: {
  {
    // PRT: int j = 99;
    // PRT: int k = 99;
    int j = 99;
    int k = 99;
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCCollapseClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 2
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} j 'int'
    // DMP-NEXT:     OMPSimdDirective
    // DMP-NEXT:       OMPCollapseClause
    // DMP-NEXT:         ConstantExpr {{.*}} 'int'
    // DMP-NEXT:           value: Int 2
    // DMP-NEXT:           IntegerLiteral {{.*}} 'int' 2
    //  DMP-NOT:       OMP
    //      DMP:       ForStmt
    //      DMP:         ForStmt
    //      DMP:           CallExpr
    //      DMP:           ForStmt
    //
    // PRT-NOACC-NEXT: for (int i ={{.*}}) {
    // PRT-NOACC-NEXT:   for (j ={{.*}}) {
    // PRT-NOACC-NEXT:     {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT:     for (k ={{.*}})
    // PRT-NOACC-NEXT:       ;
    // PRT-NOACC-NEXT:   }
    // PRT-NOACC-NEXT: }
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: #pragma acc loop vector collapse(2){{$}}
    //  PRT-A-NEXT: for (int i ={{.*}}) {
    //  PRT-A-NEXT:   for (j ={{.*}}) {
    //  PRT-A-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT:     for (k ={{.*}})
    //  PRT-A-NEXT:       ;
    //  PRT-A-NEXT:   }
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int j;
    // PRT-AO-NEXT: //   #pragma omp simd collapse(2){{$}}
    // PRT-AO-NEXT: //   for (int i ={{.*}}) {
    // PRT-AO-NEXT: //     for (j ={{.*}}) {
    // PRT-AO-NEXT: //       {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //       for (k ={{.*}})
    // PRT-AO-NEXT: //         ;
    // PRT-AO-NEXT: //     }
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int j;
    //  PRT-O-NEXT:   #pragma omp simd collapse(2){{$}}
    //  PRT-O-NEXT:   for (int i ={{.*}}) {
    //  PRT-O-NEXT:     for (j ={{.*}}) {
    //  PRT-O-NEXT:       {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:       for (k ={{.*}})
    //  PRT-O-NEXT:         ;
    //  PRT-O-NEXT:     }
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop vector collapse(2){{$}}
    // PRT-OA-NEXT: // for (int i ={{.*}}) {
    // PRT-OA-NEXT: //   for (j ={{.*}}) {
    // PRT-OA-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: //     for (k ={{.*}})
    // PRT-OA-NEXT: //       ;
    // PRT-OA-NEXT: //   }
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // Because k is shared among vector lanes, writes to it are a race, and so
    // the k loop might not have any iterations for some (i,j) pairs.
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: loop vector private control var: 0, 0
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: loop vector private control var: 0, 1
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: loop vector private control var: 1, 0
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: loop vector private control var: 1, 1
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: loop vector private control var: 0, 0
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: loop vector private control var: 0, 1
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: loop vector private control var: 1, 0
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: loop vector private control var: 1, 1
    #pragma acc loop vector collapse(2)
    for (int i = 0; i < 2; ++i) {
      for (j = 0; j < 2; ++j) {
        TGT_PRINTF("withinVectorFn: loop vector private control var: %d, %d\n", i, j);
        for (k = 0; k < 2; ++k)
          ;
      }
    }
  } // PRT-NEXT: }
} // PRT-NEXT: }

//------------------------------------------------------------------------------
// Check collapse clauses within a seq function.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} withinSeqFn
// PRT-LABEL: void withinSeqFn() {
void withinSeqFn() {
  //............................................................................
  // Explicit seq.
  //............................................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCSeqClause
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq collapse(2)
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq collapse(2) // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop seq: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop seq: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop seq: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop seq: 1, 1
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop seq: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop seq: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop seq: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop seq: 1, 1
  #pragma acc loop seq collapse(2)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinSeqFn: acc loop seq: %d, %d\n", i, j);

  //............................................................................
  // Explicit auto.
  //............................................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto collapse(2)
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto collapse(2) // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop auto: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop auto: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop auto: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop auto: 1, 1
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop auto: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop auto: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop auto: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop auto: 1, 1
  #pragma acc loop auto collapse(2)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinSeqFn: acc loop auto: %d, %d\n", i, j);

  //............................................................................
  // No level of parallelism specified.
  //............................................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop collapse(2)
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop collapse(2) // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     {{TGT_PRINTF|printf}}
  //
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop: 1, 1
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop: 1, 1
  #pragma acc loop collapse(2)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinSeqFn: acc loop: %d, %d\n", i, j);

  //............................................................................
  // More loops than necessary.
  //............................................................................

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCSeqClause
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 1
  // DMP-NEXT:   impl: ForStmt
  //      DMP:     ForStmt
  //      DMP:       CallExpr
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq collapse(1)
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq collapse(1) // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     {{TGT_PRINTF|printf}}({{.*}});
  //
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: loop 1 {1}: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: loop 1 {1}: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: loop 1 {1}: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: loop 1 {1}: 1, 1
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: loop 1 {1}: 0, 0
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: loop 1 {1}: 0, 1
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: loop 1 {1}: 1, 0
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: loop 1 {1}: 1, 1
  #pragma acc loop seq collapse(1)
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("withinSeqFn: loop 1 {1}: %d, %d\n", i, j);
} // PRT-NEXT: }

// EXE-NOT: {{.}}
