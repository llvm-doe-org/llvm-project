// Check "acc parallel" reductions across redundant gangs.

// When ADD_LOOP_TO_PAR is not set, this file checks gang reductions for "acc
// parallel" without "loop".
//
// When ADD_LOOP_TO_PAR is set, it adds "loop" and a for loop to those "acc
// parallel" directives in order to check gang reductions for combined "acc
// parallel loop" directives.

// REDEFINE: %{all:clang:args} = 
// REDEFINE: %{all:fc:pres} = PAR
// REDEFINE: %{prt:fc:args} = -DLOOP=
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}

// REDEFINE: %{all:clang:args} = -DADD_LOOP_TO_PAR
// REDEFINE: %{all:fc:pres} = PARLOOP
// REDEFINE: %{prt:fc:args} = -DLOOP=' loop seq'
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}

// END.

/* expected-no-diagnostics */

#include <assert.h>

// FIXME: For amdgcn, we get the following compile error:
//
//   File /home/jdenny/llvm/build/lib/clang/14.0.0/include/__clang_hip_math.h Line 23: 'omp.h' file not found
#if !TGT_AMDGCN
# include <complex.h>
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#if !ADD_LOOP_TO_PAR
# define LOOP
# define FORLOOP_HEAD
#else
# define LOOP loop seq
# define FORLOOP_HEAD for (int i = 0; i < 2; ++i)
#endif

// PRT: int main() {
int main() {
  // PRT: printf
  // EXE: start
  printf("start\n");

  //--------------------------------------------------
  // Reduction operator '+'
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int acc = 10;
    // PRT-NEXT: int val = 2;
    int acc = 10;
    int val = 2;
    // DMP-PAR:           ACCParallelDirective
    // DMP-PARLOOP:       ACCParallelLoopDirective
    // DMP-PARLOOP-NEXT:    ACCSeqClause
    // DMP-NEXT:            ACCNumGangsClause
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:            ACCFirstprivateClause {{.*}}
    // DMP-NOT:               <implicit>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'val' 'int'
    // DMP-NEXT:            ACCReductionClause {{.*}} '+'
    // DMP-NEXT:              DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:    effect: ACCParallelDirective
    // DMP-PARLOOP-NEXT:      ACCNumGangsClause
    // DMP-PARLOOP-NEXT:        IntegerLiteral {{.*}} 'int' 4
    // DMP-PARLOOP-NEXT:      ACCFirstprivateClause {{.*}}
    // DMP-PARLOOP-NOT:         <implicit>
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'val' 'int'
    // DMP-NEXT:              ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'val' 'int'
    // DMP-NEXT:              ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:      ACCReductionClause {{.*}} <implicit> '+'
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:              impl: OMPTargetTeamsDirective
    // DMP-NEXT:                OMPNum_teamsClause
    // DMP-NEXT:                  IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:                OMPFirstprivateClause
    // DMP-NOT:                   <implicit>
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'val' 'int'
    // DMP-PAR-NEXT:            OMPReductionClause
    // DMP-PAR-NEXT:              DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:                OMPMapClause
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:        OMPReductionClause
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP:           ACCLoopDirective
    // DMP-PARLOOP-NEXT:        ACCSeqClause
    // DMP-PARLOOP-NEXT:        ACCReductionClause {{.*}} '+'
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:        ACCSharedClause {{.*}} <implicit>
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'val' 'int'
    //
    // PRT-A-AST-NEXT:      {{^ *}}#pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(+: acc){{$}}
    // PRT-A-SRC-NEXT:      {{^ *}}#pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(+: acc){{$}}
    // PRT-AO-PAR-NEXT:     {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) reduction(+: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-AO-PARLOOP-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(+: acc){{$}}
    //
    // PRT-O-PAR-NEXT:     {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) reduction(+: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-O-PARLOOP-NEXT: {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(+: acc){{$}}
    // PRT-OA-AST-NEXT:    {{^ *}}// #pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(+: acc){{$}}
    // PRT-OA-SRC-NEXT:    {{^ *}}// #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(+: acc){{$}}
    #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(+: acc)
    // DMP-PAR-NOT:      ForStmt
    // DMP-PARLOOP-NEXT: impl: ForStmt
    // PRT-PAR-SAME: {{$([[:space:]] *FORLOOP_HEAD)?}}
    // PRT-PARLOOP-NEXT: {{for \(.*\)|FORLOOP_HEAD}}
    FORLOOP_HEAD
      // DMP: CompoundAssignOperator {{.*}} 'int' '+='
      // PRT-NEXT: acc += val;
      acc += val;
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-PAR-NEXT: acc = 18
    // EXE-PARLOOP-NEXT: acc = 26
    printf("acc = %d\n", acc);
  }
  // PRT-NEXT: }

  //--------------------------------------------------
  // Reduction operator '*'
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: float acc = 10;
    // PRT-NEXT: float val = 2;
    float acc = 10;
    float val = 2;
    // DMP-PAR:           ACCParallelDirective
    // DMP-PARLOOP:       ACCParallelLoopDirective
    // DMP-PARLOOP-NEXT:    ACCSeqClause
    // DMP-NEXT:            ACCNumGangsClause
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:            ACCFirstprivateClause {{.*}}
    // DMP-NOT:               <implicit>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'val' 'float'
    // DMP-NEXT:            ACCReductionClause {{.*}} '*'
    // DMP-NEXT:              DeclRefExpr {{.*}} 'acc' 'float'
    // DMP-PARLOOP-NEXT:    effect: ACCParallelDirective
    // DMP-PARLOOP-NEXT:      ACCNumGangsClause
    // DMP-PARLOOP-NEXT:        IntegerLiteral {{.*}} 'int' 4
    // DMP-PARLOOP-NEXT:      ACCFirstprivateClause {{.*}}
    // DMP-PARLOOP-NOT:         <implicit>
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'val' 'float'
    // DMP-NEXT:              ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'val' 'float'
    // DMP-NEXT:              ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'acc' 'float'
    // DMP-PARLOOP-NEXT:      ACCReductionClause {{.*}} <implicit> '*'
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'acc' 'float'
    // DMP-NEXT:              impl: OMPTargetTeamsDirective
    // DMP-NEXT:                OMPNum_teamsClause
    // DMP-NEXT:                  IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:                OMPFirstprivateClause
    // DMP-NOT:                   <implicit>
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'val' 'float'
    // DMP-PAR-NEXT:            OMPReductionClause
    // DMP-PAR-NEXT:              DeclRefExpr {{.*}} 'acc' 'float'
    // DMP-NEXT:                OMPMapClause
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'acc' 'float'
    // DMP-PARLOOP-NEXT:        OMPReductionClause
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'float'
    // DMP-PARLOOP:           ACCLoopDirective
    // DMP-PARLOOP-NEXT:        ACCSeqClause
    // DMP-PARLOOP-NEXT:        ACCReductionClause {{.*}} '*'
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'float'
    // DMP-PARLOOP-NEXT:        ACCSharedClause {{.*}} <implicit>
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'val' 'float'
    //
    // PRT-A-AST-NEXT:      {{^ *}}#pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(*: acc){{$}}
    // PRT-A-SRC-NEXT:      {{^ *}}#pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(*: acc){{$}}
    // PRT-AO-PAR-NEXT:     {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) reduction(*: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-AO-PARLOOP-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(*: acc){{$}}
    //
    // PRT-O-PAR-NEXT:      {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) reduction(*: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-O-PARLOOP-NEXT:  {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(*: acc){{$}}
    // PRT-OA-AST-NEXT:     {{^ *}}// #pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(*: acc){{$}}
    // PRT-OA-SRC-NEXT:     {{^ *}}// #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(*: acc){{$}}
    #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(*: acc)
    // DMP-PAR-NOT:      ForStmt
    // DMP-PARLOOP-NEXT: impl: ForStmt
    // PRT-PAR-SAME: {{$([[:space:]] *FORLOOP_HEAD)?}}
    // PRT-PARLOOP-NEXT: {{for (.*)|FORLOOP_HEAD}}
    FORLOOP_HEAD
      // DMP: CompoundAssignOperator {{.*}} 'float' '*='
      // PRT-NEXT: acc *= val;
      acc *= val;
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-PAR-NEXT:     acc = 160.0
    // EXE-PARLOOP-NEXT: acc = 2560.0
    printf("acc = %.1f\n", acc);
  }
  // PRT-NEXT: }

  //--------------------------------------------------
  // Reduction operator 'max'
  //--------------------------------------------------

  // FIXME: When OpenMP offloading is activated by -fopenmp-targets, pointers
  // pass into acc parallel as null, but otherwise they pass in just fine.
  // What does the OpenMP spec say is supposed to happen?

// PRT-SRC-NEXT: #if TGT_HOST
#if TGT_HOST
  // PRT-NEXT: {
  {
    // PRT-NEXT: int arr[]
    // PRT-NEXT: int *acc
    // PRT-NEXT: int *val
    int arr[] = {0, 1, 2};
    int *acc = arr;
    int *val = arr + 2;
    // DMP-PAR:           ACCParallelDirective
    // DMP-PARLOOP:       ACCParallelLoopDirective
    // DMP-PARLOOP-NEXT:    ACCSeqClause
    // DMP-NEXT:            ACCNumGangsClause
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:            ACCFirstprivateClause {{.*}}
    // DMP-NOT:               <implicit>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'val' 'int *'
    // DMP-NEXT:            ACCReductionClause {{.*}} 'max'
    // DMP-NEXT:              DeclRefExpr {{.*}} 'acc' 'int *'
    // DMP-PARLOOP-NEXT:    effect: ACCParallelDirective
    // DMP-PARLOOP-NEXT:      ACCNumGangsClause
    // DMP-PARLOOP-NEXT:        IntegerLiteral {{.*}} 'int' 4
    // DMP-PARLOOP-NEXT:      ACCFirstprivateClause {{.*}}
    // DMP-PARLOOP-NOT:         <implicit>
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'val' 'int *'
    // DMP-NEXT:              ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'val' 'int *'
    // DMP-NEXT:              ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'acc' 'int *'
    // DMP-PARLOOP-NEXT:      ACCReductionClause {{.*}} <implicit> 'max'
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'acc' 'int *'
    // DMP-NEXT:              impl: OMPTargetTeamsDirective
    // DMP-NEXT:                OMPNum_teamsClause
    // DMP-NEXT:                  IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:                OMPFirstprivateClause
    // DMP-NOT:                   <implicit>
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'val' 'int *'
    // DMP-PAR-NEXT:            OMPReductionClause
    // DMP-PAR-NEXT:              DeclRefExpr {{.*}} 'acc' 'int *'
    // DMP-NEXT:                OMPMapClause
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'acc' 'int *'
    // DMP-PARLOOP-NEXT:        OMPReductionClause
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'int *'
    // DMP-PARLOOP:           ACCLoopDirective
    // DMP-PARLOOP-NEXT:        ACCSeqClause
    // DMP-PARLOOP-NEXT:        ACCReductionClause {{.*}} 'max'
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'int *'
    // DMP-PARLOOP-NEXT:        ACCSharedClause {{.*}} <implicit>
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'val' 'int *'
    //
    // PRT-A-AST-NEXT:      {{^ *}}#pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(max: acc){{$}}
    // PRT-A-SRC-NEXT:      {{^ *}}#pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(max: acc){{$}}
    // PRT-AO-PAR-NEXT:     {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) reduction(max: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-AO-PARLOOP-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(max: acc){{$}}
    //
    // PRT-O-PAR-NEXT:      {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) reduction(max: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-O-PARLOOP-NEXT:  {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(max: acc){{$}}
    // PRT-OA-AST-NEXT:     {{^ *}}// #pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(max: acc){{$}}
    // PRT-OA-SRC-NEXT:     {{^ *}}// #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(max: acc){{$}}
    #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(max: acc)
    // DMP-PAR-NOT:      ForStmt
    // DMP-PARLOOP-NEXT: impl: ForStmt
    // PRT-PAR-SAME: {{$([[:space:]] *FORLOOP_HEAD)?}}
    // PRT-PARLOOP-NEXT: {{for (.*)|FORLOOP_HEAD}}
    FORLOOP_HEAD
      // DMP: ConditionalOperator {{.*}} 'int *'
      // PRT-NEXT: acc = val > acc ? val : acc;
      acc = val > acc ? val : acc;
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-HOST-NEXT: acc == arr + 2: 1
    printf("acc == arr + 2: %d\n", acc == arr + 2);
  }
  // PRT-NEXT: }
// PRT-SRC-NEXT: #endif
#endif

  // Also test 'max' for ints, which works regardless of offloading.

  // PRT-NEXT: {
  {
    // PRT-NEXT: int acc
    // PRT-NEXT: int val
    int acc = 0;
    int val = 2;
    // DMP-PAR:           ACCParallelDirective
    // DMP-PARLOOP:       ACCParallelLoopDirective
    // DMP-PARLOOP-NEXT:    ACCSeqClause
    // DMP-NEXT:            ACCNumGangsClause
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:            ACCFirstprivateClause {{.*}}
    // DMP-NOT:               <implicit>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'val' 'int'
    // DMP-NEXT:            ACCReductionClause {{.*}} 'max'
    // DMP-NEXT:              DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:    effect: ACCParallelDirective
    // DMP-PARLOOP-NEXT:      ACCNumGangsClause
    // DMP-PARLOOP-NEXT:        IntegerLiteral {{.*}} 'int' 4
    // DMP-PARLOOP-NEXT:      ACCFirstprivateClause {{.*}}
    // DMP-PARLOOP-NOT:         <implicit>
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'val' 'int'
    // DMP-NEXT:              ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'val' 'int'
    // DMP-NEXT:              ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:      ACCReductionClause {{.*}} <implicit> 'max'
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:              impl: OMPTargetTeamsDirective
    // DMP-NEXT:                OMPNum_teamsClause
    // DMP-NEXT:                  IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:                OMPFirstprivateClause
    // DMP-NOT:                   <implicit>
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'val' 'int'
    // DMP-PAR-NEXT:            OMPReductionClause
    // DMP-PAR-NEXT:              DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:                OMPMapClause
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:        OMPReductionClause
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP:           ACCLoopDirective
    // DMP-PARLOOP-NEXT:        ACCSeqClause
    // DMP-PARLOOP-NEXT:        ACCReductionClause {{.*}} 'max'
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:        ACCSharedClause {{.*}} <implicit>
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'val' 'int'
    //
    // PRT-A-AST-NEXT:      {{^ *}}#pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(max: acc){{$}}
    // PRT-A-SRC-NEXT:      {{^ *}}#pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(max: acc){{$}}
    // PRT-AO-PAR-NEXT:     {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) reduction(max: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-AO-PARLOOP-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(max: acc){{$}}
    //
    // PRT-O-PAR-NEXT:      {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) reduction(max: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-O-PARLOOP-NEXT:  {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(max: acc){{$}}
    // PRT-OA-AST-NEXT:     {{^ *}}// #pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(max: acc){{$}}
    // PRT-OA-SRC-NEXT:     {{^ *}}// #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(max: acc){{$}}
    #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(max: acc)
    // DMP-PAR-NOT:      ForStmt
    // DMP-PARLOOP-NEXT: impl: ForStmt
    // PRT-PAR-SAME: {{$([[:space:]] *FORLOOP_HEAD)?}}
    // PRT-PARLOOP-NEXT: {{for (.*)|FORLOOP_HEAD}}
    FORLOOP_HEAD
      // DMP: ConditionalOperator {{.*}} 'int'
      // PRT-NEXT: acc = val > acc ? val : acc;
      acc = val > acc ? val : acc;
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: acc: 2
    printf("acc: %d\n", acc);
  }
  // PRT-NEXT: }

  //--------------------------------------------------
  // Reduction operator 'min'
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: double acc = 10;
    // PRT-NEXT: double val = 2;
    double acc = 10;
    double val = 2;
    // DMP-PAR:           ACCParallelDirective
    // DMP-PARLOOP:       ACCParallelLoopDirective
    // DMP-PARLOOP-NEXT:    ACCSeqClause
    // DMP-NEXT:            ACCNumGangsClause
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:            ACCFirstprivateClause {{.*}}
    // DMP-NOT:               <implicit>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'val' 'double'
    // DMP-NEXT:            ACCReductionClause {{.*}} 'min'
    // DMP-NEXT:              DeclRefExpr {{.*}} 'acc' 'double'
    // DMP-PARLOOP-NEXT:    effect: ACCParallelDirective
    // DMP-PARLOOP-NEXT:      ACCNumGangsClause
    // DMP-PARLOOP-NEXT:        IntegerLiteral {{.*}} 'int' 4
    // DMP-PARLOOP-NEXT:      ACCFirstprivateClause {{.*}}
    // DMP-PARLOOP-NOT:         <implicit>
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'val' 'double'
    // DMP-NEXT:              ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'val' 'double'
    // DMP-NEXT:              ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'acc' 'double'
    // DMP-PARLOOP-NEXT:      ACCReductionClause {{.*}} <implicit> 'min'
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'acc' 'double'
    // DMP-NEXT:              impl: OMPTargetTeamsDirective
    // DMP-NEXT:                OMPNum_teamsClause
    // DMP-NEXT:                  IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:                OMPFirstprivateClause
    // DMP-NOT:                   <implicit>
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'val' 'double'
    // DMP-PAR-NEXT:            OMPReductionClause
    // DMP-PAR-NEXT:              DeclRefExpr {{.*}} 'acc' 'double'
    // DMP-NEXT:                OMPMapClause
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'acc' 'double'
    // DMP-PARLOOP-NEXT:        OMPReductionClause
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'double'
    // DMP-PARLOOP:           ACCLoopDirective
    // DMP-PARLOOP-NEXT:        ACCSeqClause
    // DMP-PARLOOP-NEXT:        ACCReductionClause {{.*}} 'min'
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'double'
    // DMP-PARLOOP-NEXT:        ACCSharedClause {{.*}} <implicit>
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'val' 'double'
    //
    // PRT-A-AST-NEXT:      {{^ *}}#pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(min: acc){{$}}
    // PRT-A-SRC-NEXT:      {{^ *}}#pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(min: acc){{$}}
    // PRT-AO-PAR-NEXT:     {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) reduction(min: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-AO-PARLOOP-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(min: acc){{$}}
    //
    // PRT-O-PAR-NEXT:     {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) reduction(min: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-O-PARLOOP-NEXT: {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(min: acc){{$}}
    // PRT-OA-AST-NEXT:    {{^ *}}// #pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(min: acc){{$}}
    // PRT-OA-SRC-NEXT:    {{^ *}}// #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(min: acc){{$}}
    #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(min: acc)
    // DMP-PAR-NOT:      ForStmt
    // DMP-PARLOOP-NEXT: impl: ForStmt
    // PRT-PAR-SAME: {{$([[:space:]] *FORLOOP_HEAD)?}}
    // PRT-PARLOOP-NEXT: {{for (.*)|FORLOOP_HEAD}}
    FORLOOP_HEAD
      // DMP: ConditionalOperator {{.*}} 'double'
      // PRT-NEXT: acc = val < acc ? val : acc;
      acc = val < acc ? val : acc;
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: acc = 2.0
    printf("acc = %.1f\n", acc);
  }
  // PRT-NEXT: }

  //--------------------------------------------------
  // Reduction operator '&'
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: char acc = 10;
    // PRT-NEXT: char val = 3;
    char acc = 10; // 1010
    char val = 3;  // 0011
    // DMP-PAR:           ACCParallelDirective
    // DMP-PARLOOP:       ACCParallelLoopDirective
    // DMP-PARLOOP-NEXT:    ACCSeqClause
    // DMP-NEXT:            ACCNumGangsClause
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:            ACCFirstprivateClause {{.*}}
    // DMP-NOT:               <implicit>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'val' 'char'
    // DMP-NEXT:            ACCReductionClause {{.*}} '&'
    // DMP-NEXT:              DeclRefExpr {{.*}} 'acc' 'char'
    // DMP-PARLOOP-NEXT:    effect: ACCParallelDirective
    // DMP-PARLOOP-NEXT:      ACCNumGangsClause
    // DMP-PARLOOP-NEXT:        IntegerLiteral {{.*}} 'int' 4
    // DMP-PARLOOP-NEXT:      ACCFirstprivateClause {{.*}}
    // DMP-PARLOOP-NOT:         <implicit>
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'val' 'char'
    // DMP-NEXT:              ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'val' 'char'
    // DMP-NEXT:              ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'acc' 'char'
    // DMP-PARLOOP-NEXT:      ACCReductionClause {{.*}} <implicit> '&'
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'acc' 'char'
    // DMP-NEXT:              impl: OMPTargetTeamsDirective
    // DMP-NEXT:                OMPNum_teamsClause
    // DMP-NEXT:                  IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:                OMPFirstprivateClause
    // DMP-NOT:                   <implicit>
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'val' 'char'
    // DMP-PAR-NEXT:            OMPReductionClause
    // DMP-PAR-NEXT:              DeclRefExpr {{.*}} 'acc' 'char'
    // DMP-NEXT:                OMPMapClause
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'acc' 'char'
    // DMP-PARLOOP-NEXT:        OMPReductionClause
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'char'
    // DMP-PARLOOP:           ACCLoopDirective
    // DMP-PARLOOP-NEXT:        ACCSeqClause
    // DMP-PARLOOP-NEXT:        ACCReductionClause {{.*}} '&'
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'char'
    // DMP-PARLOOP-NEXT:        ACCSharedClause {{.*}} <implicit>
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'val' 'char'
    //
    // PRT-A-AST-NEXT:      {{^ *}}#pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(&: acc){{$}}
    // PRT-A-SRC-NEXT:      {{^ *}}#pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(&: acc){{$}}
    // PRT-AO-PAR-NEXT:     {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) reduction(&: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-AO-PARLOOP-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(&: acc){{$}}
    //
    // PRT-O-PAR-NEXT:     {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) reduction(&: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-O-PARLOOP-NEXT: {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(&: acc){{$}}
    // PRT-OA-AST-NEXT:    {{^ *}}// #pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(&: acc){{$}}
    // PRT-OA-SRC-NEXT:    {{^ *}}// #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(&: acc){{$}}
    #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(&: acc)
    // DMP-PAR-NOT:      ForStmt
    // DMP-PARLOOP-NEXT: impl: ForStmt
    // PRT-PAR-SAME: {{$([[:space:]] *FORLOOP_HEAD)?}}
    // PRT-PARLOOP-NEXT: {{for (.*)|FORLOOP_HEAD}}
    FORLOOP_HEAD
      // DMP: CompoundAssignOperator {{.*}} 'char' '&='
      // PRT-NEXT: acc &= val;
      acc &= val;
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: acc = 2
    printf("acc = %d\n", acc);
  }
  // PRT-NEXT: }

  //--------------------------------------------------
  // Reduction operator '|'
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int acc = 10;
    // PRT-NEXT: int val = 3;
    int acc = 10; // 1010
    int val = 3;  // 0011
    // DMP-PAR:           ACCParallelDirective
    // DMP-PARLOOP:       ACCParallelLoopDirective
    // DMP-PARLOOP-NEXT:    ACCSeqClause
    // DMP-NEXT:            ACCNumGangsClause
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:            ACCFirstprivateClause {{.*}}
    // DMP-NOT:               <implicit>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'val' 'int'
    // DMP-NEXT:            ACCReductionClause {{.*}} '|'
    // DMP-NEXT:              DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:    effect: ACCParallelDirective
    // DMP-PARLOOP-NEXT:      ACCNumGangsClause
    // DMP-PARLOOP-NEXT:        IntegerLiteral {{.*}} 'int' 4
    // DMP-PARLOOP-NEXT:      ACCFirstprivateClause {{.*}}
    // DMP-PARLOOP-NOT:         <implicit>
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'val' 'int'
    // DMP-NEXT:              ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'val' 'int'
    // DMP-NEXT:              ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:      ACCReductionClause {{.*}} <implicit> '|'
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:              impl: OMPTargetTeamsDirective
    // DMP-NEXT:                OMPNum_teamsClause
    // DMP-NEXT:                  IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:                OMPFirstprivateClause
    // DMP-NOT:                   <implicit>
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'val' 'int'
    // DMP-PAR-NEXT:            OMPReductionClause
    // DMP-PAR-NEXT:              DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:                OMPMapClause
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:        OMPReductionClause
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP:           ACCLoopDirective
    // DMP-PARLOOP-NEXT:        ACCSeqClause
    // DMP-PARLOOP-NEXT:        ACCReductionClause {{.*}} '|'
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:        ACCSharedClause {{.*}} <implicit>
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'val' 'int'
    //
    // PRT-A-AST-NEXT:      {{^ *}}#pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(|: acc){{$}}
    // PRT-A-SRC-NEXT:      {{^ *}}#pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(|: acc){{$}}
    // PRT-AO-PAR-NEXT:     {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) reduction(|: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-AO-PARLOOP-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(|: acc){{$}}
    //
    // PRT-O-PAR-NEXT:     {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) reduction(|: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-O-PARLOOP-NEXT: {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(|: acc){{$}}
    // PRT-OA-AST-NEXT:    {{^ *}}// #pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(|: acc){{$}}
    // PRT-OA-SRC-NEXT:    {{^ *}}// #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(|: acc){{$}}
    #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(|: acc)
    // DMP-PAR-NOT:      ForStmt
    // DMP-PARLOOP-NEXT: impl: ForStmt
    // PRT-PAR-SAME: {{$([[:space:]] *FORLOOP_HEAD)?}}
    // PRT-PARLOOP-NEXT: {{for (.*)|FORLOOP_HEAD}}
    FORLOOP_HEAD
      // DMP: CompoundAssignOperator {{.*}} 'int' '|='
      // PRT-NEXT: acc |= val;
      acc |= val;
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: acc = 11
    printf("acc = %d\n", acc);
  }
  // PRT-NEXT: }

  //--------------------------------------------------
  // Reduction operator '^'
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int acc = 10;
    // PRT-NEXT: int val = 3;
    int acc = 10; // 1010
    int val = 3;  // 0011
    // DMP-PAR:           ACCParallelDirective
    // DMP-PARLOOP:       ACCParallelLoopDirective
    // DMP-PARLOOP-NEXT:    ACCSeqClause
    // DMP-NEXT:            ACCNumGangsClause
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:            ACCFirstprivateClause {{.*}}
    // DMP-NOT:               <implicit>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'val' 'int'
    // DMP-NEXT:            ACCReductionClause {{.*}} '^'
    // DMP-NEXT:              DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:    effect: ACCParallelDirective
    // DMP-PARLOOP-NEXT:      ACCNumGangsClause
    // DMP-PARLOOP-NEXT:        IntegerLiteral {{.*}} 'int' 3
    // DMP-PARLOOP-NEXT:      ACCFirstprivateClause {{.*}}
    // DMP-PARLOOP-NOT:         <implicit>
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'val' 'int'
    // DMP-NEXT:              ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'val' 'int'
    // DMP-NEXT:              ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:      ACCReductionClause {{.*}} <implicit> '^'
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:              impl: OMPTargetTeamsDirective
    // DMP-NEXT:                OMPNum_teamsClause
    // DMP-NEXT:                  IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:                OMPFirstprivateClause
    // DMP-NOT:                   <implicit>
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'val' 'int'
    // DMP-PAR-NEXT:            OMPReductionClause
    // DMP-PAR-NEXT:              DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:                OMPMapClause
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:        OMPReductionClause
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP:           ACCLoopDirective
    // DMP-PARLOOP-NEXT:        ACCSeqClause
    // DMP-PARLOOP-NEXT:        ACCReductionClause {{.*}} '^'
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:        ACCSharedClause {{.*}} <implicit>
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'val' 'int'
    //
    // PRT-A-AST-NEXT:      {{^ *}}#pragma acc parallel[[LOOP]] num_gangs(3) firstprivate(val) reduction(^: acc){{$}}
    // PRT-A-SRC-NEXT:      {{^ *}}#pragma acc parallel LOOP num_gangs(3) firstprivate(val) reduction(^: acc){{$}}
    // PRT-AO-PAR-NEXT:     {{^ *}}// #pragma omp target teams num_teams(3) firstprivate(val) reduction(^: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-AO-PARLOOP-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(^: acc){{$}}
    //
    // PRT-O-PAR-NEXT:      {{^ *}}#pragma omp target teams num_teams(3) firstprivate(val) reduction(^: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-O-PARLOOP-NEXT:  {{^ *}}#pragma omp target teams num_teams(3) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(^: acc){{$}}
    // PRT-OA-AST-NEXT:     {{^ *}}// #pragma acc parallel[[LOOP]] num_gangs(3) firstprivate(val) reduction(^: acc){{$}}
    // PRT-OA-SRC-NEXT:     {{^ *}}// #pragma acc parallel LOOP num_gangs(3) firstprivate(val) reduction(^: acc){{$}}
    #pragma acc parallel LOOP num_gangs(3) firstprivate(val) reduction(^: acc)
    // DMP-PAR-NOT:      ForStmt
    // DMP-PARLOOP-NEXT: impl: ForStmt
    // PRT-PAR-SAME: {{$([[:space:]] *FORLOOP_HEAD)?}}
    // PRT-PARLOOP-NEXT: {{for (.*)|FORLOOP_HEAD}}
    FORLOOP_HEAD
      // DMP: CompoundAssignOperator {{.*}} 'int' '^='
      // PRT-NEXT: acc ^= val;
      acc ^= val;
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-PAR-NEXT:     acc = 9
    // EXE-PARLOOP-NEXT: acc = 10
    printf("acc = %d\n", acc);
  }
  // PRT-NEXT: }

  //--------------------------------------------------
  // Reduction operator '&&'
  //--------------------------------------------------

  // PRT-NEXT: int inis[] = {{.*}};
  // PRT-NEXT: int vals[] = {{.*}};
  int inis[] = {0, 10, 0, 2};
  int vals[] = {0, 0,  1, 3};
  assert(sizeof(inis) == sizeof(vals));

  // PRT: for ({{.*}}) {
  for (int i = 0; i < sizeof(inis)/sizeof(*inis); ++i) {
    // PRT-NEXT: acc = inis[i];
    // PRT-NEXT: val = vals[i];
    int acc = inis[i];
    int val = vals[i];
    // DMP-PAR:           ACCParallelDirective
    // DMP-PARLOOP:       ACCParallelLoopDirective
    // DMP-PARLOOP-NEXT:    ACCSeqClause
    // DMP-NEXT:            ACCNumGangsClause
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:            ACCFirstprivateClause {{.*}}
    // DMP-NOT:               <implicit>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'val' 'int'
    // DMP-NEXT:            ACCReductionClause {{.*}} '&&'
    // DMP-NEXT:              DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:    effect: ACCParallelDirective
    // DMP-PARLOOP-NEXT:      ACCNumGangsClause
    // DMP-PARLOOP-NEXT:        IntegerLiteral {{.*}} 'int' 4
    // DMP-PARLOOP-NEXT:      ACCFirstprivateClause {{.*}}
    // DMP-PARLOOP-NOT:         <implicit>
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'val' 'int'
    // DMP-NEXT:              ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'val' 'int'
    // DMP-NEXT:              ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:      ACCReductionClause {{.*}} <implicit> '&&'
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:              impl: OMPTargetTeamsDirective
    // DMP-NEXT:                OMPNum_teamsClause
    // DMP-NEXT:                  IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:                OMPFirstprivateClause
    // DMP-NOT:                   <implicit>
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'val' 'int'
    // DMP-PAR-NEXT:            OMPReductionClause
    // DMP-PAR-NEXT:              DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:                OMPMapClause
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:        OMPReductionClause
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP:           ACCLoopDirective
    // DMP-PARLOOP-NEXT:        ACCSeqClause
    // DMP-PARLOOP-NEXT:        ACCReductionClause {{.*}} '&&'
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:        ACCSharedClause {{.*}} <implicit>
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'val' 'int'
    //
    // PRT-A-AST-NEXT:      {{^ *}}#pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(&&: acc){{$}}
    // PRT-A-SRC-NEXT:      {{^ *}}#pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(&&: acc){{$}}
    // PRT-AO-PAR-NEXT:     {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) reduction(&&: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-AO-PARLOOP-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(&&: acc){{$}}
    //
    // PRT-O-PAR-NEXT:      {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) reduction(&&: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-O-PARLOOP-NEXT:  {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(&&: acc){{$}}
    // PRT-OA-AST-NEXT:     {{^ *}}// #pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(&&: acc){{$}}
    // PRT-OA-SRC-NEXT:     {{^ *}}// #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(&&: acc){{$}}
    #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(&&: acc)
    // DMP-PAR-NOT:      ForStmt
    // DMP-PARLOOP-NEXT: impl: ForStmt
    // PRT-PAR-SAME: {{$([[:space:]] *FORLOOP_HEAD)?}}
    // PRT-PARLOOP-NEXT: {{for (.*)|FORLOOP_HEAD}}
    FORLOOP_HEAD
      // DMP: BinaryOperator {{.*}} 'int' '&&'
      // PRT-NEXT: acc = acc && val;
      acc = acc && val;
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: acc = 0
    // EXE-NEXT: acc = 0
    // EXE-NEXT: acc = 0
    // EXE-NEXT: acc = 1
    printf("acc = %d\n", acc);
  }
  // PRT-NEXT: }

  //--------------------------------------------------
  // Reduction operator '||'
  //--------------------------------------------------

  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < sizeof(inis)/sizeof(*inis); ++i) {
    // PRT-NEXT: int acc = inis[i];
    // PRT-NEXT: int val = vals[i];
    int acc = inis[i];
    int val = vals[i];
    // DMP-PAR:           ACCParallelDirective
    // DMP-PARLOOP:       ACCParallelLoopDirective
    // DMP-PARLOOP-NEXT:    ACCSeqClause
    // DMP-NEXT:            ACCNumGangsClause
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:            ACCFirstprivateClause {{.*}}
    // DMP-NOT:               <implicit>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'val' 'int'
    // DMP-NEXT:            ACCReductionClause {{.*}} '||'
    // DMP-NEXT:              DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:    effect: ACCParallelDirective
    // DMP-PARLOOP-NEXT:      ACCNumGangsClause
    // DMP-PARLOOP-NEXT:        IntegerLiteral {{.*}} 'int' 4
    // DMP-PARLOOP-NEXT:      ACCFirstprivateClause {{.*}}
    // DMP-PARLOOP-NOT:         <implicit>
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'val' 'int'
    // DMP-NEXT:              ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'val' 'int'
    // DMP-NEXT:              ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:      ACCReductionClause {{.*}} <implicit> '||'
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:              impl: OMPTargetTeamsDirective
    // DMP-NEXT:                OMPNum_teamsClause
    // DMP-NEXT:                  IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:                OMPFirstprivateClause
    // DMP-NOT:                   <implicit>
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'val' 'int'
    // DMP-PAR-NEXT:            OMPReductionClause
    // DMP-PAR-NEXT:              DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:                OMPMapClause
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:        OMPReductionClause
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP:           ACCLoopDirective
    // DMP-PARLOOP-NEXT:        ACCSeqClause
    // DMP-PARLOOP-NEXT:        ACCReductionClause {{.*}} '||'
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:        ACCSharedClause {{.*}} <implicit>
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'val' 'int'
    //
    // PRT-A-AST-NEXT:      {{^ *}}#pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(||: acc){{$}}
    // PRT-A-SRC-NEXT:      {{^ *}}#pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(||: acc){{$}}
    // PRT-AO-PAR-NEXT:     {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) reduction(||: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-AO-PARLOOP-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(||: acc){{$}}
    //
    // PRT-O-PAR-NEXT:      {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) reduction(||: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-O-PARLOOP-NEXT:  {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(||: acc){{$}}
    // PRT-OA-AST-NEXT:     {{^ *}}// #pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(||: acc){{$}}
    // PRT-OA-SRC-NEXT:     {{^ *}}// #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(||: acc){{$}}
    #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(||: acc)
    // DMP-PAR-NOT:      ForStmt
    // DMP-PARLOOP-NEXT: impl: ForStmt
    // PRT-PAR-SAME: {{$([[:space:]] *FORLOOP_HEAD)?}}
    // PRT-PARLOOP-NEXT: {{for (.*)|FORLOOP_HEAD}}
    FORLOOP_HEAD
      // DMP: BinaryOperator {{.*}} 'int' '||'
      // PRT-NEXT: acc = acc || val;
      acc = acc || val;
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: acc = 0
    // EXE-NEXT: acc = 1
    // EXE-NEXT: acc = 1
    // EXE-NEXT: acc = 1
    printf("acc = %d\n", acc);
  }
  // PRT-NEXT: }

  //--------------------------------------------------
  // Enumerated argument type
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: enum E {
    // PRT: E10{{[[:space:]]*}}
    // PRT-SAME: };
    enum E {E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10};
    // PRT-NEXT: enum E acc = E3;
    // PRT-NEXT: enum E val = E10;
    enum E acc = E3;
    enum E val = E10;
    // DMP-PAR:           ACCParallelDirective
    // DMP-PARLOOP:       ACCParallelLoopDirective
    // DMP-PARLOOP-NEXT:    ACCSeqClause
    // DMP-NEXT:            ACCNumGangsClause
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:            ACCFirstprivateClause {{.*}}
    // DMP-NOT:               <implicit>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'val' 'enum E'
    // DMP-NEXT:            ACCReductionClause {{.*}} '&'
    // DMP-NEXT:              DeclRefExpr {{.*}} 'acc' 'enum E'
    // DMP-PARLOOP-NEXT:    effect: ACCParallelDirective
    // DMP-PARLOOP-NEXT:      ACCNumGangsClause
    // DMP-PARLOOP-NEXT:        IntegerLiteral {{.*}} 'int' 4
    // DMP-PARLOOP-NEXT:      ACCFirstprivateClause {{.*}}
    // DMP-PARLOOP-NOT:         <implicit>
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'val' 'enum E'
    // DMP-NEXT:              ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'val' 'enum E'
    // DMP-NEXT:              ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'acc' 'enum E'
    // DMP-PARLOOP-NEXT:      ACCReductionClause {{.*}} <implicit> '&'
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'acc' 'enum E'
    // DMP-NEXT:              impl: OMPTargetTeamsDirective
    // DMP-NEXT:                OMPNum_teamsClause
    // DMP-NEXT:                  IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:                OMPFirstprivateClause
    // DMP-NOT:                   <implicit>
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'val' 'enum E'
    // DMP-PAR-NEXT:            OMPReductionClause
    // DMP-PAR-NEXT:              DeclRefExpr {{.*}} 'acc' 'enum E'
    // DMP-NEXT:                OMPMapClause
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'acc' 'enum E'
    // DMP-PARLOOP-NEXT:        OMPReductionClause
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'enum E'
    // DMP-PARLOOP:           ACCLoopDirective
    // DMP-PARLOOP-NEXT:        ACCSeqClause
    // DMP-PARLOOP-NEXT:        ACCReductionClause {{.*}} '&'
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'enum E'
    // DMP-PARLOOP-NEXT:        ACCSharedClause {{.*}} <implicit>
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'val' 'enum E'
    //
    // PRT-A-AST-NEXT:      {{^ *}}#pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(&: acc){{$}}
    // PRT-A-SRC-NEXT:      {{^ *}}#pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(&: acc){{$}}
    // PRT-AO-PAR-NEXT:     {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) reduction(&: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-AO-PARLOOP-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(&: acc){{$}}
    //
    // PRT-O-PAR-NEXT:      {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) reduction(&: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-O-PARLOOP-NEXT:  {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(&: acc){{$}}
    // PRT-OA-AST-NEXT:     {{^ *}}// #pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(&: acc){{$}}
    // PRT-OA-SRC-NEXT:     {{^ *}}// #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(&: acc){{$}}
    #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(&: acc)
    // DMP-PAR-NOT:      ForStmt
    // DMP-PARLOOP-NEXT: impl: ForStmt
    // PRT-PAR-SAME: {{$([[:space:]] *FORLOOP_HEAD)?}}
    // PRT-PARLOOP-NEXT: {{for (.*)|FORLOOP_HEAD}}
    FORLOOP_HEAD
      // DMP: CompoundAssignOperator {{.*}}'enum E' '&='
      // PRT-NEXT: acc &= val;
      acc &= val;
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: acc == E2: 1
    printf("acc == E2: %d\n", acc == E2);
  }
  // PRT-NEXT: }

  //--------------------------------------------------
  // bool argument type
  //--------------------------------------------------

  // FIXME: OpenMP offloading for nvptx64 doesn't store bool correctly for
  // reductions.
  // FIXME: Clang fails an assert here for amdgcn offloading.

// PRT-SRC-NEXT: #if !TGT_NVPTX64 && !TGT_AMDGCN
#if !TGT_NVPTX64 && !TGT_AMDGCN
  // PRT-NEXT: {
  {
    // PRT-NEXT: {{_Bool|bool}} acc = 3;
    // PRT-NEXT: {{_Bool|bool}} val = 0;
    bool acc = 3;
    bool val = 0;
    // DMP-PAR:           ACCParallelDirective
    // DMP-PARLOOP:       ACCParallelLoopDirective
    // DMP-PARLOOP-NEXT:    ACCSeqClause
    // DMP-NEXT:            ACCNumGangsClause
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:            ACCFirstprivateClause {{.*}}
    // DMP-NOT:               <implicit>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'val' '{{bool|_Bool}}'
    // DMP-NEXT:            ACCReductionClause {{.*}} '|'
    // DMP-NEXT:              DeclRefExpr {{.*}} 'acc' '{{bool|_Bool}}'
    // DMP-PARLOOP-NEXT:    effect: ACCParallelDirective
    // DMP-PARLOOP-NEXT:      ACCNumGangsClause
    // DMP-PARLOOP-NEXT:        IntegerLiteral {{.*}} 'int' 4
    // DMP-PARLOOP-NEXT:      ACCFirstprivateClause {{.*}}
    // DMP-PARLOOP-NOT:         <implicit>
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'val' '{{bool|_Bool}}'
    // DMP-NEXT:              ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'val' '{{bool|_Bool}}'
    // DMP-NEXT:              ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'acc' '{{bool|_Bool}}'
    // DMP-PARLOOP-NEXT:      ACCReductionClause {{.*}} <implicit> '|'
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'acc' '{{bool|_Bool}}'
    // DMP-NEXT:              impl: OMPTargetTeamsDirective
    // DMP-NEXT:                OMPNum_teamsClause
    // DMP-NEXT:                  IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:                OMPFirstprivateClause
    // DMP-NOT:                   <implicit>
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'val' '{{bool|_Bool}}'
    // DMP-PAR-NEXT:            OMPReductionClause
    // DMP-PAR-NEXT:              DeclRefExpr {{.*}} 'acc' '{{bool|_Bool}}'
    // DMP-NEXT:                OMPMapClause
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'acc' '{{bool|_Bool}}'
    // DMP-PARLOOP-NEXT:        OMPReductionClause
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' '{{bool|_Bool}}'
    // DMP-PARLOOP:           ACCLoopDirective
    // DMP-PARLOOP-NEXT:        ACCSeqClause
    // DMP-PARLOOP-NEXT:        ACCReductionClause {{.*}} '|'
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' '{{bool|_Bool}}'
    // DMP-PARLOOP-NEXT:        ACCSharedClause {{.*}} <implicit>
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'val' '{{bool|_Bool}}'
    //
    // PRT-A-AST-NEXT:      {{^ *}}#pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(|: acc){{$}}
    // PRT-A-SRC-NEXT:      {{^ *}}#pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(|: acc){{$}}
    // PRT-AO-PAR-NEXT:     {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) reduction(|: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-AO-PARLOOP-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(|: acc){{$}}
    //
    // PRT-O-PAR-NEXT:      {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) reduction(|: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-O-PARLOOP-NEXT:  {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(|: acc){{$}}
    // PRT-OA-AST-NEXT:     {{^ *}}// #pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(|: acc){{$}}
    // PRT-OA-SRC-NEXT:     {{^ *}}// #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(|: acc){{$}}
    #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(|: acc)
    // DMP-PAR-NOT:      ForStmt
    // DMP-PARLOOP-NEXT: impl: ForStmt
    // PRT-PAR-SAME: {{$([[:space:]] *FORLOOP_HEAD)?}}
    // PRT-PARLOOP-NEXT: {{for (.*)|FORLOOP_HEAD}}
    FORLOOP_HEAD
      // DMP: CompoundAssignOperator {{.*}} '{{bool|_Bool}}' '|='
      // PRT-NEXT: acc |= val;
      acc |= val;
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-HOST-NEXT: acc: 1
    // EXE-TGT-X86_64-NEXT: acc: 1
    // EXE-TGT-PPC64LE-NEXT: acc: 1
    printf("acc: %d\n", acc);
  }
  // PRT-NEXT: }
// PRT-SRC-NEXT: #endif
#endif

  //--------------------------------------------------
  // Complex argument type
  //--------------------------------------------------

  // FIXME: We couldn't include complex.h for amdgcn (see above).
// PRT-SRC-NEXT: #if !TGT_AMDGCN
#if !TGT_AMDGCN
  // PRT-NEXT: {
  {
    // PRT-NEXT: {{_Complex float|float complex}} acc
    // PRT-NEXT: {{_Complex float|float complex}} val
    float complex acc = 10 + 1*I;
    float complex val = 2 + 3*I;
    // DMP-PAR:           ACCParallelDirective
    // DMP-PARLOOP:       ACCParallelLoopDirective
    // DMP-PARLOOP-NEXT:    ACCSeqClause
    // DMP-NEXT:            ACCNumGangsClause
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:            ACCFirstprivateClause {{.*}}
    // DMP-NOT:               <implicit>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'val' '_Complex float'
    // DMP-NEXT:            ACCReductionClause {{.*}} '+'
    // DMP-NEXT:              DeclRefExpr {{.*}} 'acc' '_Complex float'
    // DMP-PARLOOP-NEXT:    effect: ACCParallelDirective
    // DMP-PARLOOP-NEXT:      ACCNumGangsClause
    // DMP-PARLOOP-NEXT:        IntegerLiteral {{.*}} 'int' 4
    // DMP-PARLOOP-NEXT:      ACCFirstprivateClause {{.*}}
    // DMP-PARLOOP-NOT:         <implicit>
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'val' '_Complex float'
    // DMP-NEXT:              ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'val' '_Complex float'
    // DMP-NEXT:              ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'acc' '_Complex float'
    // DMP-PARLOOP-NEXT:      ACCReductionClause {{.*}} <implicit> '+'
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'acc' '_Complex float'
    // DMP-NEXT:              impl: OMPTargetTeamsDirective
    // DMP-NEXT:                OMPNum_teamsClause
    // DMP-NEXT:                  IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:                OMPFirstprivateClause
    // DMP-NOT:                   <implicit>
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'val' '_Complex float'
    // DMP-PAR-NEXT:            OMPReductionClause
    // DMP-PAR-NEXT:              DeclRefExpr {{.*}} 'acc' '_Complex float'
    // DMP-NEXT:                OMPMapClause
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'acc' '_Complex float'
    // DMP-PARLOOP-NEXT:        OMPReductionClause
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' '_Complex float'
    // DMP-PARLOOP:           ACCLoopDirective
    // DMP-PARLOOP-NEXT:        ACCSeqClause
    // DMP-PARLOOP-NEXT:        ACCReductionClause {{.*}} '+'
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' '_Complex float'
    // DMP-PARLOOP-NEXT:        ACCSharedClause {{.*}} <implicit>
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'val' '_Complex float'
    //
    // PRT-A-AST-NEXT:      {{^ *}}#pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(+: acc){{$}}
    // PRT-A-SRC-NEXT:      {{^ *}}#pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(+: acc){{$}}
    // PRT-AO-PAR-NEXT:     {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) reduction(+: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-AO-PARLOOP-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(+: acc){{$}}
    //
    // PRT-O-PAR-NEXT:      {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) reduction(+: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-O-PARLOOP-NEXT:  {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(+: acc){{$}}
    // PRT-OA-AST-NEXT:     {{^ *}}// #pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(+: acc){{$}}
    // PRT-OA-SRC-NEXT:     {{^ *}}// #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(+: acc){{$}}
    #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(+: acc)
    // DMP-PAR-NOT:      ForStmt
    // DMP-PARLOOP-NEXT: impl: ForStmt
    // PRT-PAR-SAME: {{$([[:space:]] *FORLOOP_HEAD)?}}
    // PRT-PARLOOP-NEXT: {{for (.*)|FORLOOP_HEAD}}
    FORLOOP_HEAD
      // DMP: CompoundAssignOperator {{.*}} '_Complex float' '+='
      // PRT-NEXT: acc += val;
      acc += val;
    // DMP: CallExpr
    // PRT-NEXT: printf
    //            EXE-HOST-PAR-NEXT: acc = 18.0 + 13.0i
    //      EXE-TGT-X86_64-PAR-NEXT: acc = 18.0 + 13.0i
    //     EXE-TGT-PPC64LE-PAR-NEXT: acc = 18.0 + 13.0i
    //     EXE-TGT-NVPTX64-PAR-NEXT: acc = 18.0 + 13.0i
    //        EXE-HOST-PARLOOP-NEXT: acc = 26.0 + 25.0i
    //  EXE-TGT-X86_64-PARLOOP-NEXT: acc = 26.0 + 25.0i
    // EXE-TGT-PPC64LE-PARLOOP-NEXT: acc = 26.0 + 25.0i
    // EXE-TGT-NVPTX64-PARLOOP-NEXT: acc = 26.0 + 25.0i
    printf("acc = %.1f + %.1fi\n", creal(acc), cimag(acc));
  }
  // PRT-NEXT: }
// PRT-SRC-NEXT: #endif
#endif

  //--------------------------------------------------
  // Explicit copy already present
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int acc = 10;
    // PRT-NEXT: int val = 2;
    int acc = 10;
    int val = 2;
    // DMP-PAR:           ACCParallelDirective
    // DMP-PARLOOP:       ACCParallelLoopDirective
    // DMP-PARLOOP-NEXT:    ACCSeqClause
    // DMP-NEXT:            ACCNumGangsClause
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:            ACCFirstprivateClause {{.*}}
    // DMP-NOT:               <implicit>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'val' 'int'
    // DMP-NEXT:            ACCReductionClause {{.*}} '+'
    // DMP-NEXT:              DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:            ACCCopyClause {{.*}}
    // DMP-NOT:               <implicit>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:    effect: ACCParallelDirective
    // DMP-PARLOOP-NEXT:      ACCNumGangsClause
    // DMP-PARLOOP-NEXT:        IntegerLiteral {{.*}} 'int' 4
    // DMP-PARLOOP-NEXT:      ACCFirstprivateClause {{.*}}
    // DMP-PARLOOP-NOT:         <implicit>
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'val' 'int'
    // DMP-PARLOOP-NEXT:      ACCCopyClause {{.*}}
    // DMP-PARLOOP-NOT:         <implicit>
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:              ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'val' 'int'
    // DMP-PARLOOP-NEXT:      ACCReductionClause {{.*}} <implicit> '+'
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:              impl: OMPTargetTeamsDirective
    // DMP-NEXT:                OMPNum_teamsClause
    // DMP-NEXT:                  IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:                OMPFirstprivateClause
    // DMP-NOT:                   <implicit>
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'val' 'int'
    // DMP-PAR-NEXT:            OMPReductionClause
    // DMP-PAR-NEXT:              DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:                OMPMapClause
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:        OMPReductionClause
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP:           ACCLoopDirective
    // DMP-PARLOOP-NEXT:        ACCSeqClause
    // DMP-PARLOOP-NEXT:        ACCReductionClause {{.*}} '+'
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:        ACCSharedClause {{.*}} <implicit>
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'val' 'int'
    //
    // PRT-A-AST-NEXT:      {{^ *}}#pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(+: acc) copy(acc){{$}}
    // PRT-A-SRC-NEXT:      {{^ *}}#pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(+: acc) copy(acc){{$}}
    // PRT-AO-PAR-NEXT:     {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) reduction(+: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-AO-PARLOOP-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(+: acc){{$}}
    //
    // PRT-O-PAR-NEXT:     {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) reduction(+: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-O-PARLOOP-NEXT: {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,tofrom: acc) reduction(+: acc){{$}}
    // PRT-OA-AST-NEXT:    {{^ *}}// #pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(+: acc) copy(acc){{$}}
    // PRT-OA-SRC-NEXT:    {{^ *}}// #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(+: acc) copy(acc){{$}}
    #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(+: acc) copy(acc)
    // DMP-PAR-NOT:      ForStmt
    // DMP-PARLOOP-NEXT: impl: ForStmt
    // PRT-PAR-SAME: {{$([[:space:]] *FORLOOP_HEAD)?}}
    // PRT-PARLOOP-NEXT: {{for \(.*\)|FORLOOP_HEAD}}
    FORLOOP_HEAD
      // DMP: CompoundAssignOperator {{.*}} 'int' '+='
      // PRT-NEXT: acc += val;
      acc += val;
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-PAR-NEXT: acc = 18
    // EXE-PARLOOP-NEXT: acc = 26
    printf("acc = %d\n", acc);
  }
  // PRT-NEXT: }

  //--------------------------------------------------
  // Explicit copyin already present
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int acc = 10;
    // PRT-NEXT: int val = 2;
    int acc = 10;
    int val = 2;
    // DMP-PAR:           ACCParallelDirective
    // DMP-PARLOOP:       ACCParallelLoopDirective
    // DMP-PARLOOP-NEXT:    ACCSeqClause
    // DMP-NEXT:            ACCNumGangsClause
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:            ACCFirstprivateClause {{.*}}
    // DMP-NOT:               <implicit>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'val' 'int'
    // DMP-NEXT:            ACCReductionClause {{.*}} '+'
    // DMP-NEXT:              DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:            ACCCopyinClause {{.*}}
    // DMP-NOT:               <implicit>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:    effect: ACCParallelDirective
    // DMP-PARLOOP-NEXT:      ACCNumGangsClause
    // DMP-PARLOOP-NEXT:        IntegerLiteral {{.*}} 'int' 4
    // DMP-PARLOOP-NEXT:      ACCFirstprivateClause {{.*}}
    // DMP-PARLOOP-NOT:         <implicit>
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'val' 'int'
    // DMP-PARLOOP-NEXT:      ACCCopyinClause {{.*}}
    // DMP-PARLOOP-NOT:         <implicit>
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:              ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'val' 'int'
    // DMP-PARLOOP-NEXT:      ACCReductionClause {{.*}} <implicit> '+'
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:              impl: OMPTargetTeamsDirective
    // DMP-NEXT:                OMPNum_teamsClause
    // DMP-NEXT:                  IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:                OMPFirstprivateClause
    // DMP-NOT:                   <implicit>
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'val' 'int'
    // DMP-PAR-NEXT:            OMPReductionClause
    // DMP-PAR-NEXT:              DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:                OMPMapClause
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:        OMPReductionClause
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP:           ACCLoopDirective
    // DMP-PARLOOP-NEXT:        ACCSeqClause
    // DMP-PARLOOP-NEXT:        ACCReductionClause {{.*}} '+'
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:        ACCSharedClause {{.*}} <implicit>
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'val' 'int'
    //
    // PRT-A-AST-NEXT:      {{^ *}}#pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(+: acc) copyin(acc){{$}}
    // PRT-A-SRC-NEXT:      {{^ *}}#pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(+: acc) copyin(acc){{$}}
    // PRT-AO-PAR-NEXT:     {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) reduction(+: acc) map(ompx_hold,to: acc){{$}}
    // PRT-AO-PARLOOP-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,to: acc) reduction(+: acc){{$}}
    //
    // PRT-O-PAR-NEXT:     {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) reduction(+: acc) map(ompx_hold,to: acc){{$}}
    // PRT-O-PARLOOP-NEXT: {{^ *}}#pragma omp target teams num_teams(4) firstprivate(val) map(ompx_hold,to: acc) reduction(+: acc){{$}}
    // PRT-OA-AST-NEXT:    {{^ *}}// #pragma acc parallel[[LOOP]] num_gangs(4) firstprivate(val) reduction(+: acc) copyin(acc){{$}}
    // PRT-OA-SRC-NEXT:    {{^ *}}// #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(+: acc) copyin(acc){{$}}
    #pragma acc parallel LOOP num_gangs(4) firstprivate(val) reduction(+: acc) copyin(acc)
    // DMP-PAR-NOT:      ForStmt
    // DMP-PARLOOP-NEXT: impl: ForStmt
    // PRT-PAR-SAME: {{$([[:space:]] *FORLOOP_HEAD)?}}
    // PRT-PARLOOP-NEXT: {{for \(.*\)|FORLOOP_HEAD}}
    FORLOOP_HEAD
      // DMP: CompoundAssignOperator {{.*}} 'int' '+='
      // PRT-NEXT: acc += val;
      acc += val;
    // DMP: CallExpr
    // PRT-NEXT: printf
    //     EXE-HOST-PAR-NEXT: acc = 18
    // EXE-HOST-PARLOOP-NEXT: acc = 26
    //   EXE-TGT-X86_64-NEXT: acc = 10
    //  EXE-TGT-PPC64LE-NEXT: acc = 10
    //  EXE-TGT-NVPTX64-NEXT: acc = 10
    //   EXE-TGT-AMDGCN-NEXT: acc = 10
    printf("acc = %d\n", acc);
  }
  // PRT-NEXT: }

  //--------------------------------------------------
  // Reduction implies copy even if reduction var is unreferenced
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int acc = 10;
    int acc = 10;
    // DMP-PAR:           ACCParallelDirective
    // DMP-PARLOOP:       ACCParallelLoopDirective
    // DMP-PARLOOP-NEXT:    ACCSeqClause
    // DMP-NEXT:            ACCNumGangsClause
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:            ACCReductionClause {{.*}} '+'
    // DMP-NEXT:              DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:    effect: ACCParallelDirective
    // DMP-PARLOOP-NEXT:      ACCNumGangsClause
    // DMP-PARLOOP-NEXT:        IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:              ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:      ACCReductionClause {{.*}} <implicit> '+'
    // DMP-PARLOOP-NEXT:        DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:              impl: OMPTargetTeamsDirective
    // DMP-NEXT:                OMPNum_teamsClause
    // DMP-NEXT:                  IntegerLiteral {{.*}} 'int' 4
    // DMP-PAR-NEXT:            OMPReductionClause
    // DMP-PAR-NEXT:              DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:                OMPMapClause
    // DMP-NEXT:                  DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP-NEXT:        OMPReductionClause
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-PARLOOP:           ACCLoopDirective
    // DMP-PARLOOP-NEXT:        ACCSeqClause
    // DMP-PARLOOP-NEXT:        ACCReductionClause {{.*}} '+'
    // DMP-PARLOOP-NEXT:          DeclRefExpr {{.*}} 'acc' 'int'
    //
    // PRT-A-AST-NEXT:      {{^ *}}#pragma acc parallel[[LOOP]] num_gangs(4) reduction(+: acc){{$}}
    // PRT-A-SRC-NEXT:      {{^ *}}#pragma acc parallel LOOP num_gangs(4) reduction(+: acc){{$}}
    // PRT-AO-PAR-NEXT:     {{^ *}}// #pragma omp target teams num_teams(4) reduction(+: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-AO-PARLOOP-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) map(ompx_hold,tofrom: acc) reduction(+: acc){{$}}
    //
    // PRT-O-PAR-NEXT:     {{^ *}}#pragma omp target teams num_teams(4) reduction(+: acc) map(ompx_hold,tofrom: acc){{$}}
    // PRT-O-PARLOOP-NEXT: {{^ *}}#pragma omp target teams num_teams(4) map(ompx_hold,tofrom: acc) reduction(+: acc){{$}}
    // PRT-OA-AST-NEXT:    {{^ *}}// #pragma acc parallel[[LOOP]] num_gangs(4) reduction(+: acc){{$}}
    // PRT-OA-SRC-NEXT:    {{^ *}}// #pragma acc parallel LOOP num_gangs(4) reduction(+: acc){{$}}
    #pragma acc parallel LOOP num_gangs(4) reduction(+: acc)
    // DMP-PAR-NOT:      ForStmt
    // DMP-PARLOOP-NEXT: impl: ForStmt
    // PRT-PAR-SAME: {{$([[:space:]] *FORLOOP_HEAD)?}}
    // PRT-PARLOOP-NEXT: {{for \(.*\)|FORLOOP_HEAD}}
    FORLOOP_HEAD
      // DMP: NullStmt
      // PRT-NEXT: ;
      ;
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: acc = 10
    printf("acc = %d\n", acc);
  }
  // PRT-NEXT: }

  // PRT-NEXT: return 0;
  return 0;
}
// PRT-NEXT: }
// EXE-NOT: {{.}}
