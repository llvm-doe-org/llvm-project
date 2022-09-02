// Check private clause on "acc parallel loop", on "acc loop" within
// "acc parallel", and on orphaned "acc loop".
//
// For each case, we check every possible loop partitioning:
// - seq
// - gang (explicit)
// - gang (implicit), worker
// - gang (implicit), vector
// - gang (implicit), worker, vector
//
// The choices of explicit vs. implicit gang clauses above seem reasonable to
// check that the analyses see both while not requiring us to duplicate all
// gang loop partitionings checks, which would nearly double the size of the
// test file.

// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}

// END.

/* expected-no-diagnostics */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

// PRT: int tentativeDef;
int tentativeDef;

//------------------------------------------------------------------------------
// Check private clauses within parallel constructs.
//------------------------------------------------------------------------------

// PRT-NEXT: int main() {
int main() {
  //............................................................................
  // Check loop private for scalar that is local to enclosing "acc parallel".
  //............................................................................

  // DMP-LABEL: StringLiteral {{.*}} "parallel-local loop-private scalar\n"
  // PRT-LABEL: printf("parallel-local loop-private scalar\n");
  // EXE-LABEL: parallel-local loop-private scalar
  printf("parallel-local loop-private scalar\n");

  //  PRT-A-NEXT: #pragma acc parallel
  // PRT-AO-NEXT: // #pragma omp target teams
  //  PRT-O-NEXT: #pragma omp target teams
  // PRT-OA-NEXT: // #pragma acc parallel
  //    PRT-NEXT: {
  #pragma acc parallel num_gangs(2)
  {
    // PRT-NEXT: int i = 99;
    int i = 99;

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq private(i){{$}}
    //  PRT-A-NEXT: for (int j = 0; j < 2; ++j) {
    //  PRT-A-NEXT:   i = j;
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   for (int j = 0; j < 2; ++j) {
    // PRT-AO-NEXT: //     i = j;
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   for (int j = 0; j < 2; ++j) {
    //  PRT-O-NEXT:     i = j;
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop seq private(i){{$}}
    // PRT-OA-NEXT: // for (int j = 0; j < 2; ++j) {
    // PRT-OA-NEXT: //   i = j;
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (int j = 0; j < 2; ++j) {
    // PRT-NOACC-NEXT:   i = j;
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private scalar, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private scalar, seq: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private scalar, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private scalar, seq: in loop: 1
    #pragma acc loop seq private(i)
    for (int j = 0; j < 2; ++j) {
      i = j;
      TGT_PRINTF("parallel-local loop-private scalar, seq: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private scalar, seq: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private scalar, seq: after loop: 99
    TGT_PRINTF("parallel-local loop-private scalar, seq: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang private(i){{$}}
    //    PRT-NEXT: for (int j = 0; j < 2; ++j) {
    //    PRT-NEXT:   i = j;
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private scalar, gang: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private scalar, gang: in loop: 1
    #pragma acc loop gang private(i)
    for (int j = 0; j < 2; ++j) {
      i = j;
      TGT_PRINTF("parallel-local loop-private scalar, gang: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private scalar, gang: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private scalar, gang: after loop: 99
    TGT_PRINTF("parallel-local loop-private scalar, gang: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker private(i){{$}}
    //    PRT-NEXT: for (int j = 0; j < 2; ++j) {
    //    PRT-NEXT:   i = j;
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private scalar, worker: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private scalar, worker: in loop: 1
    #pragma acc loop worker private(i)
    for (int j = 0; j < 2; ++j) {
      i = j;
      TGT_PRINTF("parallel-local loop-private scalar, worker: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private scalar, worker: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private scalar, worker: after loop: 99
    TGT_PRINTF("parallel-local loop-private scalar, worker: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeSimdDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute simd private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector private(i){{$}}
    //    PRT-NEXT: for (int j = 0; j < 2; ++j) {
    //    PRT-NEXT:   i = j;
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private scalar, vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private scalar, vector: in loop: 1
    #pragma acc loop vector private(i)
    for (int j = 0; j < 2; ++j) {
      i = j;
      TGT_PRINTF("parallel-local loop-private scalar, vector: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private scalar, vector: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private scalar, vector: after loop: 99
    TGT_PRINTF("parallel-local loop-private scalar, vector: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector private(i){{$}}
    //    PRT-NEXT: for (int j = 0; j < 2; ++j) {
    //    PRT-NEXT:   i = j;
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private scalar, worker vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private scalar, worker vector: in loop: 1
    #pragma acc loop worker vector private(i)
    for (int j = 0; j < 2; ++j) {
      i = j;
      TGT_PRINTF("parallel-local loop-private scalar, worker vector: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private scalar, worker vector: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private scalar, worker vector: after loop: 99
    TGT_PRINTF("parallel-local loop-private scalar, worker vector: after loop: %d\n", i);
  } // PRT-NEXT: }

  //............................................................................
  // Repeat that, but the scalar is local to an enclosing "acc parallel loop"
  // instead.  The scalar would be firstprivate instead of local for the
  // effective enclosing "acc parallel", but it's not firstprivate because the
  // private clause means the original is never actually referenced.
  //............................................................................

  // DMP-LABEL: StringLiteral {{.*}} "parallel-would-be-firstprivate loop-private scalar\n"
  // PRT-LABEL: printf("parallel-would-be-firstprivate loop-private scalar\n");
  // EXE-LABEL: parallel-would-be-firstprivate loop-private scalar
  printf("parallel-would-be-firstprivate loop-private scalar\n");

  // PRT-NEXT: {
  {
    // PRT-NEXT: int i = 99;
    int i = 99;
    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //  DMP-NOT:       OMPFirstprivateClause
    //      DMP:     ACCLoopDirective
    // DMP-NEXT:       ACCSeqClause
    //  DMP-NOT:         <implicit>
    // DMP-NEXT:       ACCPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:       impl: CompoundStmt
    // DMP-NEXT:         DeclStmt
    // DMP-NEXT:           VarDecl {{.*}} i 'int'
    // DMP-NEXT:         ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) seq private(i){{$}}
    //  PRT-A-NEXT: for (int j = 0; j < 2; ++j) {
    //  PRT-A-NEXT:   i = j;
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   for (int j = 0; j < 2; ++j) {
    // PRT-AO-NEXT: //     i = j;
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   for (int j = 0; j < 2; ++j) {
    //  PRT-O-NEXT:     i = j;
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc parallel loop num_gangs(2) seq private(i){{$}}
    // PRT-OA-NEXT: // for (int j = 0; j < 2; ++j) {
    // PRT-OA-NEXT: //   i = j;
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (int j = 0; j < 2; ++j) {
    // PRT-NOACC-NEXT:   i = j;
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-would-be-firstprivate loop-private scalar, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-would-be-firstprivate loop-private scalar, seq: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: parallel-would-be-firstprivate loop-private scalar, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-would-be-firstprivate loop-private scalar, seq: in loop: 1
    #pragma acc parallel loop num_gangs(2) seq private(i)
    for (int j = 0; j < 2; ++j) {
      i = j;
      TGT_PRINTF("parallel-would-be-firstprivate loop-private scalar, seq: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-NEXT: parallel-would-be-firstprivate loop-private scalar, seq: after loop: 99
    TGT_PRINTF("parallel-would-be-firstprivate loop-private scalar, seq: after loop: %d\n", i);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: int i = 99;
    int i = 99;
    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //  DMP-NOT:       OMPFirstprivateClause
    //      DMP:     ACCLoopDirective
    // DMP-NEXT:       ACCGangClause
    // DMP-NEXT:       ACCPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeDirective
    // DMP-NEXT:         OMPPrivateClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:         ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) gang private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) gang private(i){{$}}
    //    PRT-NEXT: for (int j = 0; j < 2; ++j) {
    //    PRT-NEXT:   i = j;
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-would-be-firstprivate loop-private scalar, gang: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-would-be-firstprivate loop-private scalar, gang: in loop: 1
    #pragma acc parallel loop num_gangs(2) gang private(i)
    for (int j = 0; j < 2; ++j) {
      i = j;
      TGT_PRINTF("parallel-would-be-firstprivate loop-private scalar, gang: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-NEXT: parallel-would-be-firstprivate loop-private scalar, gang: after loop: 99
    TGT_PRINTF("parallel-would-be-firstprivate loop-private scalar, gang: after loop: %d\n", i);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: int i = 99;
    int i = 99;
    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //  DMP-NOT:       OMPFirstprivateClause
    //      DMP:     ACCLoopDirective
    // DMP-NEXT:       ACCWorkerClause
    // DMP-NEXT:       ACCPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeParallelForDirective
    // DMP-NEXT:         OMPPrivateClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:         ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) worker private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) worker private(i){{$}}
    //    PRT-NEXT: for (int j = 0; j < 2; ++j) {
    //    PRT-NEXT:   i = j;
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-would-be-firstprivate loop-private scalar, worker: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-would-be-firstprivate loop-private scalar, worker: in loop: 1
    #pragma acc parallel loop num_gangs(2) worker private(i)
    for (int j = 0; j < 2; ++j) {
      i = j;
      TGT_PRINTF("parallel-would-be-firstprivate loop-private scalar, worker: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-NEXT: parallel-would-be-firstprivate loop-private scalar, worker: after loop: 99
    TGT_PRINTF("parallel-would-be-firstprivate loop-private scalar, worker: after loop: %d\n", i);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: int i = 99;
    int i = 99;
    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //  DMP-NOT:       OMPFirstprivateClause
    //      DMP:     ACCLoopDirective
    // DMP-NEXT:       ACCVectorClause
    // DMP-NEXT:       ACCPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeSimdDirective
    // DMP-NEXT:         OMPPrivateClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:         ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) vector private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute simd private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) vector private(i){{$}}
    //    PRT-NEXT: for (int j = 0; j < 2; ++j) {
    //    PRT-NEXT:   i = j;
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-would-be-firstprivate loop-private scalar, vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-would-be-firstprivate loop-private scalar, vector: in loop: 1
    #pragma acc parallel loop num_gangs(2) vector private(i)
    for (int j = 0; j < 2; ++j) {
      i = j;
      TGT_PRINTF("parallel-would-be-firstprivate loop-private scalar, vector: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-NEXT: parallel-would-be-firstprivate loop-private scalar, vector: after loop: 99
    TGT_PRINTF("parallel-would-be-firstprivate loop-private scalar, vector: after loop: %d\n", i);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: int i = 99;
    int i = 99;
    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //  DMP-NOT:       OMPFirstprivateClause
    //      DMP:     ACCLoopDirective
    // DMP-NEXT:       ACCWorkerClause
    // DMP-NEXT:       ACCVectorClause
    // DMP-NEXT:       ACCPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeParallelForSimdDirective
    // DMP-NEXT:         OMPPrivateClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:         ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) worker vector private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) worker vector private(i){{$}}
    //    PRT-NEXT: for (int j = 0; j < 2; ++j) {
    //    PRT-NEXT:   i = j;
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-would-be-firstprivate loop-private scalar, worker vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-would-be-firstprivate loop-private scalar, worker vector: in loop: 1
    #pragma acc parallel loop num_gangs(2) worker vector private(i)
    for (int j = 0; j < 2; ++j) {
      i = j;
      TGT_PRINTF("parallel-would-be-firstprivate loop-private scalar, worker vector: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-NEXT: parallel-would-be-firstprivate loop-private scalar, worker vector: after loop: 99
    TGT_PRINTF("parallel-would-be-firstprivate loop-private scalar, worker vector: after loop: %d\n", i);
  } // PRT-NEXT: }

  //............................................................................
  // Now variables are declared private on the "acc loop" but are also
  // implicitly firstprivate on the "acc parallel" due to references within the
  // "acc parallel".
  //............................................................................

  // DMP-LABEL: StringLiteral {{.*}} "parallel-firstprivate loop-private scalar\n"
  // PRT-LABEL: printf("parallel-firstprivate loop-private scalar\n");
  // EXE-LABEL: parallel-firstprivate loop-private scalar
  printf("parallel-firstprivate loop-private scalar\n");
  // PRT-NEXT: {
  {
    // PRT-NEXT: int i = 99;
    // PRT-NEXT: int j = 88;
    int i = 99;
    int j = 88;

    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) firstprivate(i,j){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2) firstprivate(i,j){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2){{$}}
    #pragma acc parallel num_gangs(2)
    // PRT-NEXT: {
    {
      // PRT-NEXT: ++i;
      ++i;

      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCSeqClause
      //  DMP-NOT:     <implicit>
      // DMP-NEXT:   ACCPrivateClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
      // DMP-NEXT:   impl: CompoundStmt
      // DMP-NEXT:     DeclStmt
      // DMP-NEXT:       VarDecl {{.*}} i 'int'
      // DMP-NEXT:     DeclStmt
      // DMP-NEXT:       VarDecl {{.*}} j 'int'
      // DMP-NEXT:     ForStmt
      //
      // PRT-AO-NEXT: // v----------ACC----------v
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq private(i,j){{$}}
      //  PRT-A-NEXT: for (j = 0; j < 2; ++j) {
      //  PRT-A-NEXT:   i = j;
      //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
      //  PRT-A-NEXT: }
      // PRT-AO-NEXT: // ---------ACC->OMP--------
      // PRT-AO-NEXT: // {
      // PRT-AO-NEXT: //   int i;
      // PRT-AO-NEXT: //   int j;
      // PRT-AO-NEXT: //   for (j = 0; j < 2; ++j) {
      // PRT-AO-NEXT: //     i = j;
      // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
      // PRT-AO-NEXT: //   }
      // PRT-AO-NEXT: // }
      // PRT-AO-NEXT: // ^----------OMP----------^
      //
      // PRT-OA-NEXT: // v----------OMP----------v
      //  PRT-O-NEXT: {
      //  PRT-O-NEXT:   int i;
      //  PRT-O-NEXT:   int j;
      //  PRT-O-NEXT:   for (j = 0; j < 2; ++j) {
      //  PRT-O-NEXT:     i = j;
      //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
      //  PRT-O-NEXT:   }
      //  PRT-O-NEXT: }
      // PRT-OA-NEXT: // ---------OMP<-ACC--------
      // PRT-OA-NEXT: // #pragma acc loop seq private(i,j){{$}}
      // PRT-OA-NEXT: // for (j = 0; j < 2; ++j) {
      // PRT-OA-NEXT: //   i = j;
      // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
      // PRT-OA-NEXT: // }
      // PRT-OA-NEXT: // ^----------ACC----------^
      //
      // PRT-NOACC-NEXT: for (j = 0; j < 2; ++j) {
      // PRT-NOACC-NEXT:   i = j;
      // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
      // PRT-NOACC-NEXT: }
      //
      // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private scalar, seq: in loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private scalar, seq: in loop: 1, 1
      // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private scalar, seq: in loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private scalar, seq: in loop: 1, 1
      #pragma acc loop seq private(i,j)
      for (j = 0; j < 2; ++j) {
        i = j;
        TGT_PRINTF("parallel-firstprivate loop-private scalar, seq: in loop: %d, %d\n", i, j);
      }
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private scalar, seq: after loop: 100, 88
      // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private scalar, seq: after loop: 100, 88
      TGT_PRINTF("parallel-firstprivate loop-private scalar, seq: after loop: %d, %d\n", i, j);

      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCPrivateClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NEXT:     OMPPrivateClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
      //      DMP:     ForStmt
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang private(i,j){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute private(i,j){{$}}
      //  PRT-O-NEXT: {{^ *}}#pragma omp distribute private(i,j){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang private(i,j){{$}}
      //    PRT-NEXT: for (j = 0; j < 2; ++j) {
      //    PRT-NEXT:   i = j;
      //    PRT-NEXT:   {{TGT_PRINTF|printf}}
      //    PRT-NEXT: }
      //
      // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private scalar, gang: in loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private scalar, gang: in loop: 1, 1
      #pragma acc loop gang private(i,j)
      for (j = 0; j < 2; ++j) {
        i = j;
        TGT_PRINTF("parallel-firstprivate loop-private scalar, gang: in loop: %d, %d\n", i, j);
      }
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private scalar, gang: after loop: 100, 88
      // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private scalar, gang: after loop: 100, 88
      TGT_PRINTF("parallel-firstprivate loop-private scalar, gang: after loop: %d, %d\n", i, j);

      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCPrivateClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeParallelForDirective
      // DMP-NEXT:     OMPPrivateClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
      //      DMP:     ForStmt
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker private(i,j){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for private(i,j){{$}}
      //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for private(i,j){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker private(i,j){{$}}
      //    PRT-NEXT: for (j = 0; j < 2; ++j) {
      //    PRT-NEXT:   i = j;
      //    PRT-NEXT:   {{TGT_PRINTF|printf}}
      //    PRT-NEXT: }
      //
      // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private scalar, worker: in loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private scalar, worker: in loop: 1, 1
      #pragma acc loop worker private(i,j)
      for (j = 0; j < 2; ++j) {
        i = j;
        TGT_PRINTF("parallel-firstprivate loop-private scalar, worker: in loop: %d, %d\n", i, j);
      }
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private scalar, worker: after loop: 100, 88
      // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private scalar, worker: after loop: 100, 88
      TGT_PRINTF("parallel-firstprivate loop-private scalar, worker: after loop: %d, %d\n", i, j);

      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCPrivateClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: CompoundStmt
      // DMP-NEXT:     DeclStmt
      // DMP-NEXT:       VarDecl {{.*}} j 'int'
      // DMP-NEXT:     OMPDistributeSimdDirective
      // DMP-NEXT:       OMPPrivateClause
      // DMP-NEXT:         DeclRefExpr {{.*}} 'i' 'int'
      //      DMP:       ForStmt
      //
      // PRT-AO-NEXT: // v----------ACC----------v
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector private(i,j){{$}}
      //  PRT-A-NEXT: for (j = 0; j < 2; ++j) {
      //  PRT-A-NEXT:   i = j;
      //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
      //  PRT-A-NEXT: }
      // PRT-AO-NEXT: // ---------ACC->OMP--------
      // PRT-AO-NEXT: // {
      // PRT-AO-NEXT: //   int j;
      // PRT-AO-NEXT: //   #pragma omp distribute simd private(i){{$}}
      // PRT-AO-NEXT: //   for (j = 0; j < 2; ++j) {
      // PRT-AO-NEXT: //     i = j;
      // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
      // PRT-AO-NEXT: //   }
      // PRT-AO-NEXT: // }
      // PRT-AO-NEXT: // ^----------OMP----------^
      //
      // PRT-OA-NEXT: // v----------OMP----------v
      //  PRT-O-NEXT: {
      //  PRT-O-NEXT:   int j;
      //  PRT-O-NEXT:   {{^ *}}#pragma omp distribute simd private(i){{$}}
      //  PRT-O-NEXT:   for (j = 0; j < 2; ++j) {
      //  PRT-O-NEXT:     i = j;
      //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
      //  PRT-O-NEXT:   }
      //  PRT-O-NEXT: }
      // PRT-OA-NEXT: // ---------OMP<-ACC--------
      // PRT-OA-NEXT: // #pragma acc loop vector private(i,j){{$}}
      // PRT-OA-NEXT: // for (j = 0; j < 2; ++j) {
      // PRT-OA-NEXT: //   i = j;
      // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
      // PRT-OA-NEXT: // }
      // PRT-OA-NEXT: // ^----------ACC----------^
      //
      // PRT-NOACC-NEXT: for (j = 0; j < 2; ++j) {
      // PRT-NOACC-NEXT:   i = j;
      // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
      // PRT-NOACC-NEXT: }
      //
      // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private scalar, vector: in loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private scalar, vector: in loop: 1, 1
      #pragma acc loop vector private(i,j)
      for (j = 0; j < 2; ++j) {
        i = j;
        TGT_PRINTF("parallel-firstprivate loop-private scalar, vector: in loop: %d, %d\n", i, j);
      }
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private scalar, vector: after loop: 100, 88
      // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private scalar, vector: after loop: 100, 88
      TGT_PRINTF("parallel-firstprivate loop-private scalar, vector: after loop: %d, %d\n", i, j);

      //      DMP: ACCLoopDirective
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCPrivateClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: CompoundStmt
      // DMP-NEXT:     DeclStmt
      // DMP-NEXT:       VarDecl {{.*}} j 'int'
      // DMP-NEXT:     OMPDistributeParallelForSimdDirective
      // DMP-NEXT:       OMPPrivateClause
      // DMP-NEXT:         DeclRefExpr {{.*}} 'i' 'int'
      //      DMP:       ForStmt
      //
      // PRT-AO-NEXT: // v----------ACC----------v
      //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector private(i,j){{$}}
      //  PRT-A-NEXT: for (j = 0; j < 2; ++j) {
      //  PRT-A-NEXT:   i = j;
      //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
      //  PRT-A-NEXT: }
      // PRT-AO-NEXT: // ---------ACC->OMP--------
      // PRT-AO-NEXT: // {
      // PRT-AO-NEXT: //   int j;
      // PRT-AO-NEXT: //   #pragma omp distribute parallel for simd private(i){{$}}
      // PRT-AO-NEXT: //   for (j = 0; j < 2; ++j) {
      // PRT-AO-NEXT: //     i = j;
      // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
      // PRT-AO-NEXT: //   }
      // PRT-AO-NEXT: // }
      // PRT-AO-NEXT: // ^----------OMP----------^
      //
      // PRT-OA-NEXT: // v----------OMP----------v
      //  PRT-O-NEXT: {
      //  PRT-O-NEXT:   int j;
      //  PRT-O-NEXT:   {{^ *}}#pragma omp distribute parallel for simd private(i){{$}}
      //  PRT-O-NEXT:   for (j = 0; j < 2; ++j) {
      //  PRT-O-NEXT:     i = j;
      //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
      //  PRT-O-NEXT:   }
      //  PRT-O-NEXT: }
      // PRT-OA-NEXT: // ---------OMP<-ACC--------
      // PRT-OA-NEXT: // #pragma acc loop worker vector private(i,j){{$}}
      // PRT-OA-NEXT: // for (j = 0; j < 2; ++j) {
      // PRT-OA-NEXT: //   i = j;
      // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
      // PRT-OA-NEXT: // }
      // PRT-OA-NEXT: // ^----------ACC----------^
      //
      // PRT-NOACC-NEXT:   for (j = 0; j < 2; ++j) {
      // PRT-NOACC-NEXT:     i = j;
      // PRT-NOACC-NEXT:     {{TGT_PRINTF|printf}}
      // PRT-NOACC-NEXT:   }
      //
      // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private scalar, worker vector: in loop: 0, 0
      // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private scalar, worker vector: in loop: 1, 1
      #pragma acc loop worker vector private(i,j)
      for (j = 0; j < 2; ++j) {
        i = j;
        TGT_PRINTF("parallel-firstprivate loop-private scalar, worker vector: in loop: %d, %d\n", i, j);
      }
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private scalar, worker vector: after loop: 100, 88
      // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private scalar, worker vector: after loop: 100, 88
      TGT_PRINTF("parallel-firstprivate loop-private scalar, worker vector: after loop: %d, %d\n", i, j);

      // PRT-NEXT: --j;
      --j;
      // PRT-NEXT: {{TGT_PRINTF|printf}}
      // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private scalar: after all loops: 100, 87
      // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private scalar: after all loops: 100, 87
      TGT_PRINTF("parallel-firstprivate loop-private scalar: after all loops: %d, %d\n", i, j);
    } // PRT-NEXT: }

    // PRT-NEXT: printf
    // EXE-NEXT: parallel-firstprivate loop-private scalar: after parallel: 99, 88
    printf("parallel-firstprivate loop-private scalar: after parallel: %d, %d\n", i, j);
  } // PRT-NEXT: }

  //............................................................................
  // Check private for scalar that, so far, has only a tentative definition.
  //
  // Inserting a local definition to make that scalar variable private for a
  // sequential loop used to fail an assert because VarDecl::getDefinition
  // returned nullptr for the tentative definition.
  //............................................................................

  // DMP-LABEL: StringLiteral {{.*}} "tentatively defined loop-private scalar\n"
  // PRT-LABEL: printf("tentatively defined loop-private scalar\n");
  // EXE-LABEL: tentatively defined loop-private scalar
  printf("tentatively defined loop-private scalar\n");

  //  PRT-A-NEXT: #pragma acc parallel
  // PRT-AO-NEXT: // #pragma omp target teams
  //  PRT-O-NEXT: #pragma omp target teams
  // PRT-OA-NEXT: // #pragma acc parallel
  #pragma acc parallel num_gangs(2)
  // PRT-NEXT: {
  {
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'tentativeDef' 'int'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} tentativeDef 'int'
    // DMP-NEXT:     ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq private(tentativeDef){{$}}
    //  PRT-A-NEXT: for (int j = 0; j < 2; ++j) {
    //  PRT-A-NEXT:   tentativeDef = j;
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int tentativeDef;
    // PRT-AO-NEXT: //   for (int j = 0; j < 2; ++j) {
    // PRT-AO-NEXT: //     tentativeDef = j;
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int tentativeDef;
    //  PRT-O-NEXT:   for (int j = 0; j < 2; ++j) {
    //  PRT-O-NEXT:     tentativeDef = j;
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop seq private(tentativeDef){{$}}
    // PRT-OA-NEXT: // for (int j = 0; j < 2; ++j) {
    // PRT-OA-NEXT: //   tentativeDef = j;
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (int j = 0; j < 2; ++j) {
    // PRT-NOACC-NEXT:   tentativeDef = j;
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar, seq: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar, seq: in loop: 1
    #pragma acc loop seq private(tentativeDef)
    for (int j = 0; j < 2; ++j) {
      tentativeDef = j;
      TGT_PRINTF("tentatively defined loop-private scalar, seq: in loop: %d\n", tentativeDef);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar, seq: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar, seq: after loop: 99
    TGT_PRINTF("tentatively defined loop-private scalar, seq: after loop: %d\n", tentativeDef);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'tentativeDef' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'tentativeDef' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang private(tentativeDef){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute private(tentativeDef){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute private(tentativeDef){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang private(tentativeDef){{$}}
    //    PRT-NEXT: for (int j = 0; j < 2; ++j) {
    //    PRT-NEXT:   tentativeDef = j;
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar, gang: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar, gang: in loop: 1
    #pragma acc loop gang private(tentativeDef)
    for (int j = 0; j < 2; ++j) {
      tentativeDef = j;
      TGT_PRINTF("tentatively defined loop-private scalar, gang: in loop: %d\n", tentativeDef);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar, gang: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar, gang: after loop: 99
    TGT_PRINTF("tentatively defined loop-private scalar, gang: after loop: %d\n", tentativeDef);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'tentativeDef' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'tentativeDef' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker private(tentativeDef){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for private(tentativeDef){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for private(tentativeDef){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker private(tentativeDef){{$}}
    //    PRT-NEXT: for (int j = 0; j < 2; ++j) {
    //    PRT-NEXT:   tentativeDef = j;
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar, worker: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar, worker: in loop: 1
    #pragma acc loop worker private(tentativeDef)
    for (int j = 0; j < 2; ++j) {
      tentativeDef = j;
      TGT_PRINTF("tentatively defined loop-private scalar, worker: in loop: %d\n", tentativeDef);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar, worker: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar, worker: after loop: 99
    TGT_PRINTF("tentatively defined loop-private scalar, worker: after loop: %d\n", tentativeDef);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'tentativeDef' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeSimdDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'tentativeDef' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector private(tentativeDef){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd private(tentativeDef){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute simd private(tentativeDef){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector private(tentativeDef){{$}}
    //    PRT-NEXT: for (int j = 0; j < 2; ++j) {
    //    PRT-NEXT:   tentativeDef = j;
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar, vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar, vector: in loop: 1
    #pragma acc loop vector private(tentativeDef)
    for (int j = 0; j < 2; ++j) {
      tentativeDef = j;
      TGT_PRINTF("tentatively defined loop-private scalar, vector: in loop: %d\n", tentativeDef);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar, vector: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar, vector: after loop: 99
    TGT_PRINTF("tentatively defined loop-private scalar, vector: after loop: %d\n", tentativeDef);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'tentativeDef' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'tentativeDef' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector private(tentativeDef){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd private(tentativeDef){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd private(tentativeDef){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector private(tentativeDef){{$}}
    //    PRT-NEXT: for (int j = 0; j < 2; ++j) {
    //    PRT-NEXT:   tentativeDef = j;
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar, worker vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar, worker vector: in loop: 1
    #pragma acc loop worker vector private(tentativeDef)
    for (int j = 0; j < 2; ++j) {
      tentativeDef = j;
      TGT_PRINTF("tentatively defined loop-private scalar, worker vector: in loop: %d\n", tentativeDef);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar, worker vector: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar, worker vector: after loop: 99
    TGT_PRINTF("tentatively defined loop-private scalar, worker vector: after loop: %d\n", tentativeDef);
  } // PRT-NEXT: }

  //............................................................................
  // Repeat that but for "acc parallel loop".
  //............................................................................

  // DMP-LABEL: StringLiteral {{.*}} "tentatively defined loop-private scalar (combined directive)\n"
  // PRT-LABEL: printf("tentatively defined loop-private scalar (combined directive)\n");
  // EXE-LABEL: tentatively defined loop-private scalar (combined directive)
  printf("tentatively defined loop-private scalar (combined directive)\n");

  // PRT-NEXT: {
  {
    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'tentativeDef' 'int'
    //      DMP:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //      DMP:     ACCLoopDirective
    // DMP-NEXT:       ACCSeqClause
    //  DMP-NOT:         <implicit>
    // DMP-NEXT:       ACCPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'tentativeDef' 'int'
    // DMP-NEXT:       impl: CompoundStmt
    // DMP-NEXT:         DeclStmt
    // DMP-NEXT:           VarDecl {{.*}} tentativeDef 'int'
    // DMP-NEXT:         ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) seq private(tentativeDef){{$}}
    //  PRT-A-NEXT: for (int j = 0; j < 2; ++j) {
    //  PRT-A-NEXT:   tentativeDef = j;
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int tentativeDef;
    // PRT-AO-NEXT: //   for (int j = 0; j < 2; ++j) {
    // PRT-AO-NEXT: //     tentativeDef = j;
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int tentativeDef;
    //  PRT-O-NEXT:   for (int j = 0; j < 2; ++j) {
    //  PRT-O-NEXT:     tentativeDef = j;
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc parallel loop num_gangs(2) seq private(tentativeDef){{$}}
    // PRT-OA-NEXT: // for (int j = 0; j < 2; ++j) {
    // PRT-OA-NEXT: //   tentativeDef = j;
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (int j = 0; j < 2; ++j) {
    // PRT-NOACC-NEXT:   tentativeDef = j;
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar (combined directive), seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar (combined directive), seq: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar (combined directive), seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar (combined directive), seq: in loop: 1
    #pragma acc parallel loop num_gangs(2) seq private(tentativeDef)
    for (int j = 0; j < 2; ++j) {
      tentativeDef = j;
      TGT_PRINTF("tentatively defined loop-private scalar (combined directive), seq: in loop: %d\n", tentativeDef);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-NEXT: tentatively defined loop-private scalar (combined directive), seq: after loop: 99
    TGT_PRINTF("tentatively defined loop-private scalar (combined directive), seq: after loop: %d\n", tentativeDef);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    //       DMP: ACCParallelLoopDirective
    //  DMP-NEXT:   ACCNumGangsClause
    //  DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    //  DMP-NEXT:   ACCGangClause
    //  DMP-NEXT:   ACCPrivateClause
    //  DMP-NEXT:     DeclRefExpr {{.*}} 'tentativeDef' 'int'
    //       DMP:   effect: ACCParallelDirective
    //  DMP-NEXT:     ACCNumGangsClause
    //  DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    //  DMP-NEXT:     impl: OMPTargetTeamsDirective
    //  DMP-NEXT:       OMPNum_teamsClause
    //  DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //       DMP:     ACCLoopDirective
    //  DMP-NEXT:       ACCGangClause
    //  DMP-NEXT:       ACCPrivateClause
    //  DMP-NEXT:         DeclRefExpr {{.*}} 'tentativeDef' 'int'
    //  DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    //  DMP-NEXT:       impl: OMPDistributeDirective
    //  DMP-NEXT:         OMPPrivateClause
    //  DMP-NEXT:           DeclRefExpr {{.*}} 'tentativeDef' 'int'
    //       DMP:         ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) gang private(tentativeDef){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute private(tentativeDef){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute private(tentativeDef){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) gang private(tentativeDef){{$}}
    //    PRT-NEXT: for (int j = 0; j < 2; ++j) {
    //    PRT-NEXT:   tentativeDef = j;
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar (combined directive), gang: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar (combined directive), gang: in loop: 1
    #pragma acc parallel loop num_gangs(2) gang private(tentativeDef)
    for (int j = 0; j < 2; ++j) {
      tentativeDef = j;
      TGT_PRINTF("tentatively defined loop-private scalar (combined directive), gang: in loop: %d\n", tentativeDef);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-NEXT: tentatively defined loop-private scalar (combined directive), gang: after loop: 99
    TGT_PRINTF("tentatively defined loop-private scalar (combined directive), gang: after loop: %d\n", tentativeDef);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    //       DMP: ACCParallelLoopDirective
    //  DMP-NEXT:   ACCNumGangsClause
    //  DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    //  DMP-NEXT:   ACCWorkerClause
    //  DMP-NEXT:   ACCPrivateClause
    //  DMP-NEXT:     DeclRefExpr {{.*}} 'tentativeDef' 'int'
    //       DMP:   effect: ACCParallelDirective
    //  DMP-NEXT:     ACCNumGangsClause
    //  DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    //  DMP-NEXT:     impl: OMPTargetTeamsDirective
    //  DMP-NEXT:       OMPNum_teamsClause
    //  DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //       DMP:     ACCLoopDirective
    //  DMP-NEXT:       ACCWorkerClause
    //  DMP-NEXT:       ACCPrivateClause
    //  DMP-NEXT:         DeclRefExpr {{.*}} 'tentativeDef' 'int'
    //  DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    //  DMP-NEXT:       ACCGangClause {{.*}} <implicit>
    //  DMP-NEXT:       impl: OMPDistributeParallelForDirective
    //  DMP-NEXT:         OMPPrivateClause
    //  DMP-NEXT:           DeclRefExpr {{.*}} 'tentativeDef' 'int'
    //       DMP:         ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) worker private(tentativeDef){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for private(tentativeDef){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for private(tentativeDef){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) worker private(tentativeDef){{$}}
    //    PRT-NEXT: for (int j = 0; j < 2; ++j) {
    //    PRT-NEXT:   tentativeDef = j;
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar (combined directive), worker: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar (combined directive), worker: in loop: 1
    #pragma acc parallel loop num_gangs(2) worker private(tentativeDef)
    for (int j = 0; j < 2; ++j) {
      tentativeDef = j;
      TGT_PRINTF("tentatively defined loop-private scalar (combined directive), worker: in loop: %d\n", tentativeDef);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-NEXT: tentatively defined loop-private scalar (combined directive), worker: after loop: 99
    TGT_PRINTF("tentatively defined loop-private scalar (combined directive), worker: after loop: %d\n", tentativeDef);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    //       DMP: ACCParallelLoopDirective
    //  DMP-NEXT:   ACCNumGangsClause
    //  DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    //  DMP-NEXT:   ACCVectorClause
    //  DMP-NEXT:   ACCPrivateClause
    //  DMP-NEXT:     DeclRefExpr {{.*}} 'tentativeDef' 'int'
    //       DMP:   effect: ACCParallelDirective
    //  DMP-NEXT:     ACCNumGangsClause
    //  DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    //  DMP-NEXT:     impl: OMPTargetTeamsDirective
    //  DMP-NEXT:       OMPNum_teamsClause
    //  DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //       DMP:     ACCLoopDirective
    //  DMP-NEXT:       ACCVectorClause
    //  DMP-NEXT:       ACCPrivateClause
    //  DMP-NEXT:         DeclRefExpr {{.*}} 'tentativeDef' 'int'
    //  DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    //  DMP-NEXT:       ACCGangClause {{.*}} <implicit>
    //  DMP-NEXT:       impl: OMPDistributeSimdDirective
    //  DMP-NEXT:         OMPPrivateClause
    //  DMP-NEXT:           DeclRefExpr {{.*}} 'tentativeDef' 'int'
    //       DMP:         ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) vector private(tentativeDef){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd private(tentativeDef){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute simd private(tentativeDef){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) vector private(tentativeDef){{$}}
    //    PRT-NEXT: for (int j = 0; j < 2; ++j) {
    //    PRT-NEXT:   tentativeDef = j;
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar (combined directive), vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar (combined directive), vector: in loop: 1
    #pragma acc parallel loop num_gangs(2) vector private(tentativeDef)
    for (int j = 0; j < 2; ++j) {
      tentativeDef = j;
      TGT_PRINTF("tentatively defined loop-private scalar (combined directive), vector: in loop: %d\n", tentativeDef);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-NEXT: tentatively defined loop-private scalar (combined directive), vector: after loop: 99
    TGT_PRINTF("tentatively defined loop-private scalar (combined directive), vector: after loop: %d\n", tentativeDef);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    //       DMP: ACCParallelLoopDirective
    //  DMP-NEXT:   ACCNumGangsClause
    //  DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    //  DMP-NEXT:   ACCWorkerClause
    //  DMP-NEXT:   ACCVectorClause
    //  DMP-NEXT:   ACCPrivateClause
    //  DMP-NEXT:     DeclRefExpr {{.*}} 'tentativeDef' 'int'
    //       DMP:   effect: ACCParallelDirective
    //  DMP-NEXT:     ACCNumGangsClause
    //  DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    //  DMP-NEXT:     impl: OMPTargetTeamsDirective
    //  DMP-NEXT:       OMPNum_teamsClause
    //  DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //       DMP:     ACCLoopDirective
    //  DMP-NEXT:       ACCWorkerClause
    //  DMP-NEXT:       ACCVectorClause
    //  DMP-NEXT:       ACCPrivateClause
    //  DMP-NEXT:         DeclRefExpr {{.*}} 'tentativeDef' 'int'
    //  DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    //  DMP-NEXT:       ACCGangClause {{.*}} <implicit>
    //  DMP-NEXT:       impl: OMPDistributeParallelForSimdDirective
    //  DMP-NEXT:         OMPPrivateClause
    //  DMP-NEXT:           DeclRefExpr {{.*}} 'tentativeDef' 'int'
    //       DMP:         ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) worker vector private(tentativeDef){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd private(tentativeDef){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd private(tentativeDef){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) worker vector private(tentativeDef){{$}}
    //    PRT-NEXT: for (int j = 0; j < 2; ++j) {
    //    PRT-NEXT:   tentativeDef = j;
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar (combined directive), worker vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: tentatively defined loop-private scalar (combined directive), worker vector: in loop: 1
    #pragma acc parallel loop num_gangs(2) worker vector private(tentativeDef)
    for (int j = 0; j < 2; ++j) {
      tentativeDef = j;
      TGT_PRINTF("tentatively defined loop-private scalar (combined directive), worker vector: in loop: %d\n", tentativeDef);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-NEXT: tentatively defined loop-private scalar (combined directive), worker vector: after loop: 99
    TGT_PRINTF("tentatively defined loop-private scalar (combined directive), worker vector: after loop: %d\n", tentativeDef);
  } // PRT-NEXT: }

  //............................................................................
  // Check private for loop control variable that is declared not assigned in
  // init of attached for loop.
  //............................................................................

  // DMP-LABEL: StringLiteral {{.*}} "parallel-local loop-private declared loop control\n"
  // PRT-LABEL: printf("parallel-local loop-private declared loop control\n");
  // EXE-LABEL: parallel-local loop-private declared loop control
  printf("parallel-local loop-private declared loop control\n");

  //  PRT-A-NEXT: #pragma acc parallel
  // PRT-AO-NEXT: // #pragma omp target teams
  //  PRT-O-NEXT: #pragma omp target teams
  // PRT-OA-NEXT: // #pragma acc parallel
  #pragma acc parallel num_gangs(2)
  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    int i = 99;

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq private(i){{$}}
    //  PRT-A-NEXT: for (int i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   for (int i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   for (int i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop seq private(i){{$}}
    // PRT-OA-NEXT: // for (int i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT:for (int i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:  {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT:}
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private declared loop control, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private declared loop control, seq: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private declared loop control, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private declared loop control, seq: in loop: 1
    #pragma acc loop seq private(i)
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("parallel-local loop-private declared loop control, seq: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private declared loop control, seq: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private declared loop control, seq: after loop: 99
    TGT_PRINTF("parallel-local loop-private declared loop control, seq: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang private(i){{$}}
    //    PRT-NEXT: for (int i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private declared loop control, gang: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private declared loop control, gang: in loop: 1
    #pragma acc loop gang private(i)
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("parallel-local loop-private declared loop control, gang: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private declared loop control, gang: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private declared loop control, gang: after loop: 99
    TGT_PRINTF("parallel-local loop-private declared loop control, gang: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker private(i){{$}}
    //    PRT-NEXT: for (int i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private declared loop control, worker: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private declared loop control, worker: in loop: 1
    #pragma acc loop worker private(i)
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("parallel-local loop-private declared loop control, worker: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private declared loop control, worker: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private declared loop control, worker: after loop: 99
    TGT_PRINTF("parallel-local loop-private declared loop control, worker: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeSimdDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute simd private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector private(i){{$}}
    //    PRT-NEXT: for (int i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private declared loop control, vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private declared loop control, vector: in loop: 1
    #pragma acc loop vector private(i)
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("parallel-local loop-private declared loop control, vector: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private declared loop control, vector: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private declared loop control, vector: after loop: 99
    TGT_PRINTF("parallel-local loop-private declared loop control, vector: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector private(i){{$}}
    //    PRT-NEXT: for (int i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private declared loop control, worker vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private declared loop control, worker vector: in loop: 1
    #pragma acc loop worker vector private(i)
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("parallel-local loop-private declared loop control, worker vector: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private declared loop control, worker vector: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private declared loop control, worker vector: after loop: 99
    TGT_PRINTF("parallel-local loop-private declared loop control, worker vector: after loop: %d\n", i);
  } // PRT-NEXT: }

  //............................................................................
  // Repeat that but with "acc parallel loop".
  //............................................................................

  // DMP-LABEL: StringLiteral {{.*}} "parallel-firstprivate loop-private declared loop control\n"
  // PRT-LABEL: printf("parallel-firstprivate loop-private declared loop control\n");
  // EXE-LABEL: parallel-firstprivate loop-private declared loop control
  printf("parallel-firstprivate loop-private declared loop control\n");

  // PRT-NEXT: {
  {
    // PRT-NEXT: int i = 99;
    int i = 99;
    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //      DMP:     ACCLoopDirective
    // DMP-NEXT:       ACCSeqClause
    //  DMP-NOT:         <implicit>
    // DMP-NEXT:       ACCPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:       impl: CompoundStmt
    // DMP-NEXT:         DeclStmt
    // DMP-NEXT:           VarDecl {{.*}} i 'int'
    // DMP-NEXT:         ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) seq private(i){{$}}
    //  PRT-A-NEXT: for (int i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   for (int i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   for (int i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc parallel loop num_gangs(2) seq private(i){{$}}
    // PRT-OA-NEXT: // for (int i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (int i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private declared loop control, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private declared loop control, seq: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private declared loop control, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private declared loop control, seq: in loop: 1
    #pragma acc parallel loop num_gangs(2) seq private(i)
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("parallel-firstprivate loop-private declared loop control, seq: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-NEXT: parallel-firstprivate loop-private declared loop control, seq: after loop: 99
    TGT_PRINTF("parallel-firstprivate loop-private declared loop control, seq: after loop: %d\n", i);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: int i = 99;
    int i = 99;
    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //      DMP:     ACCLoopDirective
    // DMP-NEXT:       ACCGangClause
    // DMP-NEXT:       ACCPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeDirective
    // DMP-NEXT:         OMPPrivateClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:         ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) gang private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) gang private(i){{$}}
    //    PRT-NEXT: for (int i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private declared loop control, gang: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private declared loop control, gang: in loop: 1
    #pragma acc parallel loop num_gangs(2) gang private(i)
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("parallel-firstprivate loop-private declared loop control, gang: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-NEXT: parallel-firstprivate loop-private declared loop control, gang: after loop: 99
    TGT_PRINTF("parallel-firstprivate loop-private declared loop control, gang: after loop: %d\n", i);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: int i = 99;
    int i = 99;
    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //      DMP:     ACCLoopDirective
    // DMP-NEXT:       ACCWorkerClause
    // DMP-NEXT:       ACCPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeParallelForDirective
    // DMP-NEXT:         OMPPrivateClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:         ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) worker private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) worker private(i){{$}}
    //    PRT-NEXT: for (int i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private declared loop control, worker: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private declared loop control, worker: in loop: 1
    #pragma acc parallel loop num_gangs(2) worker private(i)
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("parallel-firstprivate loop-private declared loop control, worker: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-NEXT: parallel-firstprivate loop-private declared loop control, worker: after loop: 99
    TGT_PRINTF("parallel-firstprivate loop-private declared loop control, worker: after loop: %d\n", i);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: int i = 99;
    int i = 99;
    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //      DMP:     ACCLoopDirective
    // DMP-NEXT:       ACCVectorClause
    // DMP-NEXT:       ACCPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeSimdDirective
    // DMP-NEXT:         OMPPrivateClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:         ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) vector private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute simd private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) vector private(i){{$}}
    //    PRT-NEXT: for (int i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private declared loop control, vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private declared loop control, vector: in loop: 1
    #pragma acc parallel loop num_gangs(2) vector private(i)
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("parallel-firstprivate loop-private declared loop control, vector: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-NEXT: parallel-firstprivate loop-private declared loop control, vector: after loop: 99
    TGT_PRINTF("parallel-firstprivate loop-private declared loop control, vector: after loop: %d\n", i);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: int i = 99;
    int i = 99;
    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //      DMP:     ACCLoopDirective
    // DMP-NEXT:       ACCWorkerClause
    // DMP-NEXT:       ACCVectorClause
    // DMP-NEXT:       ACCPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeParallelForSimdDirective
    // DMP-NEXT:         OMPPrivateClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:         ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) worker vector private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) worker vector private(i){{$}}
    //    PRT-NEXT: for (int i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private declared loop control, worker vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private declared loop control, worker vector: in loop: 1
    #pragma acc parallel loop num_gangs(2) worker vector private(i)
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("parallel-firstprivate loop-private declared loop control, worker vector: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-NEXT: parallel-firstprivate loop-private declared loop control, worker vector: after loop: 99
    TGT_PRINTF("parallel-firstprivate loop-private declared loop control, worker vector: after loop: %d\n", i);
  } // PRT-NEXT: }

  //............................................................................
  // Check private for loop control variable that is assigned not declared in
  // init of attached for loop.
  //............................................................................

  // DMP-LABEL: StringLiteral {{.*}} "parallel-local loop-private assigned loop control\n"
  // PRT-LABEL: printf("parallel-local loop-private assigned loop control\n");
  // EXE-LABEL: parallel-local loop-private assigned loop control
  printf("parallel-local loop-private assigned loop control\n");

  //  PRT-A-NEXT: #pragma acc parallel
  // PRT-AO-NEXT: // #pragma omp target teams
  //  PRT-O-NEXT: #pragma omp target teams
  // PRT-OA-NEXT: // #pragma acc parallel
  #pragma acc parallel num_gangs(2)
  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    int i = 99;

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq private(i){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop seq private(i){{$}}
    // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private assigned loop control, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private assigned loop control, seq: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private assigned loop control, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private assigned loop control, seq: in loop: 1
    #pragma acc loop seq private(i)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("parallel-local loop-private assigned loop control, seq: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private assigned loop control, seq: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private assigned loop control, seq: after loop: 99
    TGT_PRINTF("parallel-local loop-private assigned loop control, seq: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang private(i){{$}}
    //    PRT-NEXT: for (i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private assigned loop control, gang: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private assigned loop control, gang: in loop: 1
    #pragma acc loop gang private(i)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("parallel-local loop-private assigned loop control, gang: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private assigned loop control, gang: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private assigned loop control, gang: after loop: 99
    TGT_PRINTF("parallel-local loop-private assigned loop control, gang: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker private(i){{$}}
    //    PRT-NEXT: for (i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private assigned loop control, worker: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private assigned loop control, worker: in loop: 1
    #pragma acc loop worker private(i)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("parallel-local loop-private assigned loop control, worker: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private assigned loop control, worker: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private assigned loop control, worker: after loop: 99
    TGT_PRINTF("parallel-local loop-private assigned loop control, worker: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     OMPDistributeSimdDirective
    //      DMP:       ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector private(i){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   #pragma omp distribute simd{{$}}
    // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   {{^ *}}#pragma omp distribute simd{{$}}
    //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop vector private(i){{$}}
    // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private assigned loop control, vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private assigned loop control, vector: in loop: 1
    #pragma acc loop vector private(i)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("parallel-local loop-private assigned loop control, vector: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private assigned loop control, vector: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private assigned loop control, vector: after loop: 99
    TGT_PRINTF("parallel-local loop-private assigned loop control, vector: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     OMPDistributeParallelForSimdDirective
    //      DMP:       ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector private(i){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   #pragma omp distribute parallel for simd{{$}}
    // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   {{^ *}}#pragma omp distribute parallel for simd{{$}}
    //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop worker vector private(i){{$}}
    // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private assigned loop control, worker vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private assigned loop control, worker vector: in loop: 1
    #pragma acc loop worker vector private(i)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("parallel-local loop-private assigned loop control, worker vector: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private assigned loop control, worker vector: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: parallel-local loop-private assigned loop control, worker vector: after loop: 99
    TGT_PRINTF("parallel-local loop-private assigned loop control, worker vector: after loop: %d\n", i);
  } // PRT-NEXT: }

  //............................................................................
  // Repeat that but with "acc parallel loop".
  //............................................................................

  // DMP-LABEL: StringLiteral {{.*}} "parallel-firstprivate loop-private assigned loop control\n"
  // PRT-LABEL: printf("parallel-firstprivate loop-private assigned loop control\n");
  // EXE-LABEL: parallel-firstprivate loop-private assigned loop control
  printf("parallel-firstprivate loop-private assigned loop control\n");

  // PRT-NEXT: {
  {
    // PRT-NEXT: int i = 99;
    int i = 99;
    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //      DMP:     ACCLoopDirective
    // DMP-NEXT:       ACCSeqClause
    //  DMP-NOT:         <implicit>
    // DMP-NEXT:       ACCPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:       impl: CompoundStmt
    // DMP-NEXT:         DeclStmt
    // DMP-NEXT:           VarDecl {{.*}} i 'int'
    // DMP-NEXT:         ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) seq private(i){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc parallel loop num_gangs(2) seq private(i){{$}}
    // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private assigned loop control, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private assigned loop control, seq: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private assigned loop control, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private assigned loop control, seq: in loop: 1
    #pragma acc parallel loop num_gangs(2) seq private(i)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("parallel-firstprivate loop-private assigned loop control, seq: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-NEXT: parallel-firstprivate loop-private assigned loop control, seq: after loop: 99
    TGT_PRINTF("parallel-firstprivate loop-private assigned loop control, seq: after loop: %d\n", i);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: int i = 99;
    int i = 99;
    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //      DMP:     ACCLoopDirective
    // DMP-NEXT:       ACCGangClause
    // DMP-NEXT:       ACCPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeDirective
    // DMP-NEXT:         OMPPrivateClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:         ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) gang private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute private(i){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    //
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) gang private(i){{$}}
    //  PRT-O-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT: }
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private assigned loop control, gang: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private assigned loop control, gang: in loop: 1
    #pragma acc parallel loop num_gangs(2) gang private(i)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("parallel-firstprivate loop-private assigned loop control, gang: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-NEXT: parallel-firstprivate loop-private assigned loop control, gang: after loop: 99
    TGT_PRINTF("parallel-firstprivate loop-private assigned loop control, gang: after loop: %d\n", i);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: int i = 99;
    int i = 99;
    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //      DMP:     ACCLoopDirective
    // DMP-NEXT:       ACCWorkerClause
    // DMP-NEXT:       ACCPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeParallelForDirective
    // DMP-NEXT:         OMPPrivateClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:         ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) worker private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for private(i){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    //
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) worker private(i){{$}}
    //  PRT-O-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT: }
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private assigned loop control, worker: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private assigned loop control, worker: in loop: 1
    #pragma acc parallel loop num_gangs(2) worker private(i)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("parallel-firstprivate loop-private assigned loop control, worker: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-NEXT: parallel-firstprivate loop-private assigned loop control, worker: after loop: 99
    TGT_PRINTF("parallel-firstprivate loop-private assigned loop control, worker: after loop: %d\n", i);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: int i = 99;
    int i = 99;
    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //      DMP:     ACCLoopDirective
    // DMP-NEXT:       ACCVectorClause
    // DMP-NEXT:       ACCPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:       impl: CompoundStmt
    // DMP-NEXT:         DeclStmt
    // DMP-NEXT:           VarDecl {{.*}} i 'int'
    // DMP-NEXT:         OMPDistributeSimdDirective
    //      DMP:           ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) vector private(i){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   #pragma omp distribute simd{{$}}
    // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   {{^ *}}#pragma omp distribute simd{{$}}
    //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc parallel loop num_gangs(2) vector private(i){{$}}
    // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private assigned loop control, vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private assigned loop control, vector: in loop: 1
    #pragma acc parallel loop num_gangs(2) vector private(i)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("parallel-firstprivate loop-private assigned loop control, vector: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-NEXT: parallel-firstprivate loop-private assigned loop control, vector: after loop: 99
    TGT_PRINTF("parallel-firstprivate loop-private assigned loop control, vector: after loop: %d\n", i);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: int i = 99;
    int i = 99;
    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //      DMP:     ACCLoopDirective
    // DMP-NEXT:       ACCWorkerClause
    // DMP-NEXT:       ACCVectorClause
    // DMP-NEXT:       ACCPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:       impl: CompoundStmt
    // DMP-NEXT:         DeclStmt
    // DMP-NEXT:           VarDecl {{.*}} i 'int'
    // DMP-NEXT:         OMPDistributeParallelForSimdDirective
    //      DMP:           ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) worker vector private(i){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   #pragma omp distribute parallel for simd{{$}}
    // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   {{^ *}}#pragma omp distribute parallel for simd{{$}}
    //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc parallel loop num_gangs(2) worker vector private(i){{$}}
    // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private assigned loop control, worker vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: parallel-firstprivate loop-private assigned loop control, worker vector: in loop: 1
    #pragma acc parallel loop num_gangs(2) worker vector private(i)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("parallel-firstprivate loop-private assigned loop control, worker vector: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-NEXT: parallel-firstprivate loop-private assigned loop control, worker vector: after loop: 99
    TGT_PRINTF("parallel-firstprivate loop-private assigned loop control, worker vector: after loop: %d\n", i);
  } // PRT-NEXT: }

  //............................................................................
  // Check multiple privates in same clause and different clauses, including
  // private parallel-local scalar, private assigned loop control variable, and
  // private clauses that become empty or just smaller in translation to
  // OpenMP.
  //............................................................................

  // DMP-LABEL: StringLiteral {{.*}} "multiple privates on acc loop\n"
  // PRT-LABEL: printf("multiple privates on acc loop\n");
  // EXE-LABEL: multiple privates on acc loop
  printf("multiple privates on acc loop\n");

  //  PRT-A-NEXT: #pragma acc parallel
  // PRT-AO-NEXT: // #pragma omp target teams
  //  PRT-O-NEXT: #pragma omp target teams
  // PRT-OA-NEXT: // #pragma acc parallel
  #pragma acc parallel num_gangs(2)
  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    // PRT: int j = 88;
    // PRT: int k = 77;
    int i = 99;
    int j = 88;
    int k = 77;

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} j 'int'
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} k 'int'
    // DMP-NEXT:     ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq private(j,i) private(k){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT:   j = k = 55;
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int j;
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   int k;
    // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //     j = k = 55;
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int j;
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   int k;
    //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:     j = k = 55;
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop seq private(j,i) private(k){{$}}
    // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: //   j = k = 55;
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT:   j = k = 55;
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc loop, seq: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc loop, seq: in loop: i=1
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc loop, seq: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc loop, seq: in loop: i=1
    #pragma acc loop seq private(j,i) private(k)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("multiple privates on acc loop, seq: in loop: i=%d\n", i);
      j = k = 55;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc loop, seq: after loop: i=99, j=88, k=77
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc loop, seq: after loop: i=99, j=88, k=77
    TGT_PRINTF("multiple privates on acc loop, seq: after loop: i=%d, j=%d, k=%d\n", i, j, k);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'k' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang private(j,i) private(k){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute private(j,i) private(k){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute private(j,i) private(k){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang private(j,i) private(k){{$}}
    //    PRT-NEXT: for (i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT:   j = k = 55;
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc loop, gang: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc loop, gang: in loop: i=1
    #pragma acc loop gang private(j,i) private(k)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("multiple privates on acc loop, gang: in loop: i=%d\n", i);
      j = k = 55;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc loop, gang: after loop: i=99, j=88, k=77
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc loop, gang: after loop: i=99, j=88, k=77
    TGT_PRINTF("multiple privates on acc loop, gang: after loop: i=%d, j=%d, k=%d\n", i, j, k);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'k' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker private(j,i) private(k){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for private(j,i) private(k){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for private(j,i) private(k){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker private(j,i) private(k){{$}}
    //    PRT-NEXT: for (i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT:   j = k = 55;
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc loop, worker: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc loop, worker: in loop: i=1
    #pragma acc loop worker private(j,i) private(k)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("multiple privates on acc loop, worker: in loop: i=%d\n", i);
      j = k = 55;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc loop, worker: after loop: i=99, j=88, k=77
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc loop, worker: after loop: i=99, j=88, k=77
    TGT_PRINTF("multiple privates on acc loop, worker: after loop: i=%d, j=%d, k=%d\n", i, j, k);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     OMPDistributeSimdDirective
    // DMP-NEXT:       OMPPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:       OMPPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'k' 'int'
    //      DMP:       ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector private(j,i) private(k){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT:   j = k = 55;
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   #pragma omp distribute simd private(j) private(k){{$}}
    // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //     j = k = 55;
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   {{^ *}}#pragma omp distribute simd private(j) private(k){{$}}
    //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:     j = k = 55;
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop vector private(j,i) private(k){{$}}
    // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: //   j = k = 55;
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT:   j = k = 55;
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc loop, vector: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc loop, vector: in loop: i=1
    #pragma acc loop vector private(j,i) private(k)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("multiple privates on acc loop, vector: in loop: i=%d\n", i);
      j = k = 55;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc loop, vector: after loop: i=99, j=88, k=77
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc loop, vector: after loop: i=99, j=88, k=77
    TGT_PRINTF("multiple privates on acc loop, vector: after loop: i=%d, j=%d, k=%d\n", i, j, k);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     OMPDistributeParallelForSimdDirective
    // DMP-NEXT:       OMPPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:       OMPPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'k' 'int'
    //      DMP:       ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector private(j,i) private(k){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT:   j = k = 55;
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   #pragma omp distribute parallel for simd private(j) private(k){{$}}
    // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //     j = k = 55;
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   {{^ *}}#pragma omp distribute parallel for simd private(j) private(k){{$}}
    //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:     j = k = 55;
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop worker vector private(j,i) private(k){{$}}
    // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: //   j = k = 55;
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT:   j = k = 55;
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc loop, worker vector: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc loop, worker vector: in loop: i=1
    #pragma acc loop worker vector private(j,i) private(k)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("multiple privates on acc loop, worker vector: in loop: i=%d\n", i);
      j = k = 55;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc loop, worker vector: after loop: i=99, j=88, k=77
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc loop, worker vector: after loop: i=99, j=88, k=77
    TGT_PRINTF("multiple privates on acc loop, worker vector: after loop: i=%d, j=%d, k=%d\n", i, j, k);
  } // PRT-NEXT: }

  //............................................................................
  // Repeat that but with "acc parallel loop".
  //............................................................................

  // DMP-LABEL: StringLiteral {{.*}} "multiple privates on acc parallel loop\n"
  // PRT-LABEL: printf("multiple privates on acc parallel loop\n");
  // EXE-LABEL: multiple privates on acc parallel loop
  printf("multiple privates on acc parallel loop\n");

  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    // PRT: int j = 88;
    // PRT: int k = 77;
    int i = 99;
    int j = 88;
    int k = 77;
    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //      DMP:       ACCLoopDirective
    // DMP-NEXT:         ACCSeqClause
    //  DMP-NOT:           <implicit>
    // DMP-NEXT:         ACCPrivateClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:           DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:         ACCPrivateClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:         impl: CompoundStmt
    // DMP-NEXT:           DeclStmt
    // DMP-NEXT:             VarDecl {{.*}} i 'int'
    // DMP-NEXT:           DeclStmt
    // DMP-NEXT:             VarDecl {{.*}} j 'int'
    // DMP-NEXT:           DeclStmt
    // DMP-NEXT:             VarDecl {{.*}} k 'int'
    // DMP-NEXT:           ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) seq private(i,j) private(k){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT:   j = k = 55;
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   int j;
    // PRT-AO-NEXT: //   int k;
    // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //     j = k = 55;
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   int j;
    //  PRT-O-NEXT:   int k;
    //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:     j = k = 55;
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc parallel loop num_gangs(2) seq private(i,j) private(k){{$}}
    // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: //   j = k = 55;
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT:   j = k = 55;
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc parallel loop, seq: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc parallel loop, seq: in loop: i=1
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc parallel loop, seq: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc parallel loop, seq: in loop: i=1
    #pragma acc parallel loop num_gangs(2) seq private(i,j) private(k)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("multiple privates on acc parallel loop, seq: in loop: i=%d\n", i);
      j = k = 55;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-NEXT: multiple privates on acc parallel loop, seq: after loop: i=99, j=88, k=77
    TGT_PRINTF("multiple privates on acc parallel loop, seq: after loop: i=%d, j=%d, k=%d\n", i, j, k);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    // PRT: int j = 88;
    // PRT: int k = 77;
    int i = 99;
    int j = 88;
    int k = 77;
    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //      DMP:       ACCLoopDirective
    // DMP-NEXT:         ACCGangClause
    // DMP-NEXT:         ACCPrivateClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:           DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:         ACCPrivateClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:         impl: OMPDistributeDirective
    // DMP-NEXT:           OMPPrivateClause
    // DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:             DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:           OMPPrivateClause
    // DMP-NEXT:             DeclRefExpr {{.*}} 'k' 'int'
    //      DMP:           ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) gang private(i,j) private(k){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute private(i,j) private(k){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute private(i,j) private(k){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) gang private(i,j) private(k){{$}}
    //    PRT-NEXT: for (i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT:   j = k = 55;
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc parallel loop, gang: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc parallel loop, gang: in loop: i=1
    #pragma acc parallel loop num_gangs(2) gang private(i,j) private(k)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("multiple privates on acc parallel loop, gang: in loop: i=%d\n", i);
      j = k = 55;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-NEXT: multiple privates on acc parallel loop, gang: after loop: i=99, j=88, k=77
    TGT_PRINTF("multiple privates on acc parallel loop, gang: after loop: i=%d, j=%d, k=%d\n", i, j, k);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    // PRT: int j = 88;
    // PRT: int k = 77;
    int i = 99;
    int j = 88;
    int k = 77;
    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //      DMP:       ACCLoopDirective
    // DMP-NEXT:         ACCWorkerClause
    // DMP-NEXT:         ACCPrivateClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:           DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:         ACCPrivateClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:         ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:         impl: OMPDistributeParallelForDirective
    // DMP-NEXT:           OMPPrivateClause
    // DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:             DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:           OMPPrivateClause
    // DMP-NEXT:             DeclRefExpr {{.*}} 'k' 'int'
    //      DMP:           ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) worker private(i,j) private(k){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for private(i,j) private(k){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for private(i,j) private(k){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) worker private(i,j) private(k){{$}}
    //    PRT-NEXT: for (i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT:   j = k = 55;
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc parallel loop, worker: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc parallel loop, worker: in loop: i=1
    #pragma acc parallel loop num_gangs(2) worker private(i,j) private(k)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("multiple privates on acc parallel loop, worker: in loop: i=%d\n", i);
      j = k = 55;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-NEXT: multiple privates on acc parallel loop, worker: after loop: i=99, j=88, k=77
    TGT_PRINTF("multiple privates on acc parallel loop, worker: after loop: i=%d, j=%d, k=%d\n", i, j, k);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    // PRT: int j = 88;
    // PRT: int k = 77;
    int i = 99;
    int j = 88;
    int k = 77;
    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //      DMP:       ACCLoopDirective
    // DMP-NEXT:         ACCVectorClause
    // DMP-NEXT:         ACCPrivateClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:           DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:         ACCPrivateClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:         ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:         impl: CompoundStmt
    // DMP-NEXT:           DeclStmt
    // DMP-NEXT:             VarDecl {{.*}} i 'int'
    // DMP-NEXT:           OMPDistributeSimdDirective
    // DMP-NEXT:             OMPPrivateClause
    // DMP-NEXT:               DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:             OMPPrivateClause
    // DMP-NEXT:               DeclRefExpr {{.*}} 'k' 'int'
    //      DMP:             ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) vector private(i,j) private(k){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT:   j = k = 55;
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   #pragma omp distribute simd private(j) private(k){{$}}
    // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //     j = k = 55;
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   {{^ *}}#pragma omp distribute simd private(j) private(k){{$}}
    //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:     j = k = 55;
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc parallel loop num_gangs(2) vector private(i,j) private(k){{$}}
    // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: //   j = k = 55;
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT:   j = k = 55;
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc parallel loop, vector: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc parallel loop, vector: in loop: i=1
    #pragma acc parallel loop num_gangs(2) vector private(i,j) private(k)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("multiple privates on acc parallel loop, vector: in loop: i=%d\n", i);
      j = k = 55;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-NEXT: multiple privates on acc parallel loop, vector: after loop: i=99, j=88, k=77
    TGT_PRINTF("multiple privates on acc parallel loop, vector: after loop: i=%d, j=%d, k=%d\n", i, j, k);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    // PRT: int j = 88;
    // PRT: int k = 77;
    int i = 99;
    int j = 88;
    int k = 77;
    //      DMP: ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //      DMP:       ACCLoopDirective
    // DMP-NEXT:         ACCWorkerClause
    // DMP-NEXT:         ACCVectorClause
    // DMP-NEXT:         ACCPrivateClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:           DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:         ACCPrivateClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:         ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:         impl: CompoundStmt
    // DMP-NEXT:           DeclStmt
    // DMP-NEXT:             VarDecl {{.*}} i 'int'
    // DMP-NEXT:           OMPDistributeParallelForSimdDirective
    // DMP-NEXT:             OMPPrivateClause
    // DMP-NEXT:               DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:             OMPPrivateClause
    // DMP-NEXT:               DeclRefExpr {{.*}} 'k' 'int'
    //      DMP:             ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) worker vector private(i,j) private(k){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT:   j = k = 55;
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   #pragma omp distribute parallel for simd private(j) private(k){{$}}
    // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //     j = k = 55;
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2){{$}}
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   {{^ *}}#pragma omp distribute parallel for simd private(j) private(k){{$}}
    //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:     j = k = 55;
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc parallel loop num_gangs(2) worker vector private(i,j) private(k){{$}}
    // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: //   j = k = 55;
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT:   j = k = 55;
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc parallel loop, worker vector: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: multiple privates on acc parallel loop, worker vector: in loop: i=1
    #pragma acc parallel loop num_gangs(2) worker vector private(i,j) private(k)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("multiple privates on acc parallel loop, worker vector: in loop: i=%d\n", i);
      j = k = 55;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-NEXT: multiple privates on acc parallel loop, worker vector: after loop: i=99, j=88, k=77
    TGT_PRINTF("multiple privates on acc parallel loop, worker vector: after loop: i=%d, j=%d, k=%d\n", i, j, k);
  } // PRT-NEXT: }

  //............................................................................
  // Check private clauses within accelerator routines.
  //............................................................................

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

//------------------------------------------------------------------------------
// Check private clauses within a gang function.
//
// This repeats most of the above checks but within a gang function and without
// statically enclosing parallel constructs.
//
// TODO: Replicate the tentative definition checks here once we support the
// declare directive.  Likewise in withinWorkerFn, etc.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} withinGangFn
// PRT-LABEL: void withinGangFn() {
void withinGangFn() {
  //............................................................................
  // Check loop private for scalar that is local to enclosing function.
  //............................................................................

  // PRT-NEXT: {
  {
    // PRT-NEXT: int i = 99;
    int i = 99;

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq private(i){{$}}
    //  PRT-A-NEXT: for (int j = 0; j < 2; ++j) {
    //  PRT-A-NEXT:   i = j;
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   for (int j = 0; j < 2; ++j) {
    // PRT-AO-NEXT: //     i = j;
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   for (int j = 0; j < 2; ++j) {
    //  PRT-O-NEXT:     i = j;
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop seq private(i){{$}}
    // PRT-OA-NEXT: // for (int j = 0; j < 2; ++j) {
    // PRT-OA-NEXT: //   i = j;
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (int j = 0; j < 2; ++j) {
    // PRT-NOACC-NEXT:   i = j;
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private scalar, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private scalar, seq: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private scalar, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private scalar, seq: in loop: 1
    #pragma acc loop seq private(i)
    for (int j = 0; j < 2; ++j) {
      i = j;
      TGT_PRINTF("withinGangFn: parallel-local loop-private scalar, seq: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private scalar, seq: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private scalar, seq: after loop: 99
    TGT_PRINTF("withinGangFn: parallel-local loop-private scalar, seq: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang private(i){{$}}
    //    PRT-NEXT: for (int j = 0; j < 2; ++j) {
    //    PRT-NEXT:   i = j;
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private scalar, gang: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private scalar, gang: in loop: 1
    #pragma acc loop gang private(i)
    for (int j = 0; j < 2; ++j) {
      i = j;
      TGT_PRINTF("withinGangFn: parallel-local loop-private scalar, gang: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private scalar, gang: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private scalar, gang: after loop: 99
    TGT_PRINTF("withinGangFn: parallel-local loop-private scalar, gang: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker private(i){{$}}
    //    PRT-NEXT: for (int j = 0; j < 2; ++j) {
    //    PRT-NEXT:   i = j;
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private scalar, worker: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private scalar, worker: in loop: 1
    #pragma acc loop worker private(i)
    for (int j = 0; j < 2; ++j) {
      i = j;
      TGT_PRINTF("withinGangFn: parallel-local loop-private scalar, worker: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private scalar, worker: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private scalar, worker: after loop: 99
    TGT_PRINTF("withinGangFn: parallel-local loop-private scalar, worker: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeSimdDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute simd private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector private(i){{$}}
    //    PRT-NEXT: for (int j = 0; j < 2; ++j) {
    //    PRT-NEXT:   i = j;
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private scalar, vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private scalar, vector: in loop: 1
    #pragma acc loop vector private(i)
    for (int j = 0; j < 2; ++j) {
      i = j;
      TGT_PRINTF("withinGangFn: parallel-local loop-private scalar, vector: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private scalar, vector: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private scalar, vector: after loop: 99
    TGT_PRINTF("withinGangFn: parallel-local loop-private scalar, vector: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector private(i){{$}}
    //    PRT-NEXT: for (int j = 0; j < 2; ++j) {
    //    PRT-NEXT:   i = j;
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private scalar, worker vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private scalar, worker vector: in loop: 1
    #pragma acc loop worker vector private(i)
    for (int j = 0; j < 2; ++j) {
      i = j;
      TGT_PRINTF("withinGangFn: parallel-local loop-private scalar, worker vector: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private scalar, worker vector: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private scalar, worker vector: after loop: 99
    TGT_PRINTF("withinGangFn: parallel-local loop-private scalar, worker vector: after loop: %d\n", i);
  } // PRT-NEXT: }

  //............................................................................
  // Check private for loop control variable that is declared not assigned in
  // init of attached for loop.
  //............................................................................

  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    int i = 99;

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq private(i){{$}}
    //  PRT-A-NEXT: for (int i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   for (int i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   for (int i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop seq private(i){{$}}
    // PRT-OA-NEXT: // for (int i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT:for (int i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:  {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT:}
    //
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private declared loop control, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private declared loop control, seq: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private declared loop control, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private declared loop control, seq: in loop: 1
    #pragma acc loop seq private(i)
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("withinGangFn: parallel-local loop-private declared loop control, seq: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private declared loop control, seq: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private declared loop control, seq: after loop: 99
    TGT_PRINTF("withinGangFn: parallel-local loop-private declared loop control, seq: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang private(i){{$}}
    //    PRT-NEXT: for (int i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private declared loop control, gang: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private declared loop control, gang: in loop: 1
    #pragma acc loop gang private(i)
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("withinGangFn: parallel-local loop-private declared loop control, gang: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private declared loop control, gang: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private declared loop control, gang: after loop: 99
    TGT_PRINTF("withinGangFn: parallel-local loop-private declared loop control, gang: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker private(i){{$}}
    //    PRT-NEXT: for (int i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private declared loop control, worker: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private declared loop control, worker: in loop: 1
    #pragma acc loop worker private(i)
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("withinGangFn: parallel-local loop-private declared loop control, worker: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private declared loop control, worker: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private declared loop control, worker: after loop: 99
    TGT_PRINTF("withinGangFn: parallel-local loop-private declared loop control, worker: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeSimdDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute simd private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector private(i){{$}}
    //    PRT-NEXT: for (int i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private declared loop control, vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private declared loop control, vector: in loop: 1
    #pragma acc loop vector private(i)
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("withinGangFn: parallel-local loop-private declared loop control, vector: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private declared loop control, vector: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private declared loop control, vector: after loop: 99
    TGT_PRINTF("withinGangFn: parallel-local loop-private declared loop control, vector: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector private(i){{$}}
    //    PRT-NEXT: for (int i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private declared loop control, worker vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private declared loop control, worker vector: in loop: 1
    #pragma acc loop worker vector private(i)
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("withinGangFn: parallel-local loop-private declared loop control, worker vector: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private declared loop control, worker vector: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private declared loop control, worker vector: after loop: 99
    TGT_PRINTF("withinGangFn: parallel-local loop-private declared loop control, worker vector: after loop: %d\n", i);
  } // PRT-NEXT: }

  //............................................................................
  // Check private for loop control variable that is assigned not declared in
  // init of attached for loop.
  //............................................................................

  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    int i = 99;

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq private(i){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop seq private(i){{$}}
    // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private assigned loop control, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private assigned loop control, seq: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private assigned loop control, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private assigned loop control, seq: in loop: 1
    #pragma acc loop seq private(i)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("withinGangFn: parallel-local loop-private assigned loop control, seq: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private assigned loop control, seq: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private assigned loop control, seq: after loop: 99
    TGT_PRINTF("withinGangFn: parallel-local loop-private assigned loop control, seq: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang private(i){{$}}
    //    PRT-NEXT: for (i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private assigned loop control, gang: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private assigned loop control, gang: in loop: 1
    #pragma acc loop gang private(i)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("withinGangFn: parallel-local loop-private assigned loop control, gang: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private assigned loop control, gang: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private assigned loop control, gang: after loop: 99
    TGT_PRINTF("withinGangFn: parallel-local loop-private assigned loop control, gang: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker private(i){{$}}
    //    PRT-NEXT: for (i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private assigned loop control, worker: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private assigned loop control, worker: in loop: 1
    #pragma acc loop worker private(i)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("withinGangFn: parallel-local loop-private assigned loop control, worker: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private assigned loop control, worker: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private assigned loop control, worker: after loop: 99
    TGT_PRINTF("withinGangFn: parallel-local loop-private assigned loop control, worker: after loop: %d\n", i);

     //      DMP: ACCLoopDirective
     // DMP-NEXT:   ACCVectorClause
     // DMP-NEXT:   ACCPrivateClause
     // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
     // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
     // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
     // DMP-NEXT:   impl: CompoundStmt
     // DMP-NEXT:     DeclStmt
     // DMP-NEXT:       VarDecl {{.*}} i 'int'
     // DMP-NEXT:     OMPDistributeSimdDirective
     //      DMP:       ForStmt
     //
     // PRT-AO-NEXT: // v----------ACC----------v
     //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector private(i){{$}}
     //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
     //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
     //  PRT-A-NEXT: }
     // PRT-AO-NEXT: // ---------ACC->OMP--------
     // PRT-AO-NEXT: // {
     // PRT-AO-NEXT: //   int i;
     // PRT-AO-NEXT: //   #pragma omp distribute simd{{$}}
     // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
     // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
     // PRT-AO-NEXT: //   }
     // PRT-AO-NEXT: // }
     // PRT-AO-NEXT: // ^----------OMP----------^
     //
     // PRT-OA-NEXT: // v----------OMP----------v
     //  PRT-O-NEXT: {
     //  PRT-O-NEXT:   int i;
     //  PRT-O-NEXT:   {{^ *}}#pragma omp distribute simd{{$}}
     //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
     //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
     //  PRT-O-NEXT:   }
     //  PRT-O-NEXT: }
     // PRT-OA-NEXT: // ---------OMP<-ACC--------
     // PRT-OA-NEXT: // #pragma acc loop vector private(i){{$}}
     // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
     // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
     // PRT-OA-NEXT: // }
     // PRT-OA-NEXT: // ^----------ACC----------^
     //
     // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
     // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
     // PRT-NOACC-NEXT: }
     //
     // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private assigned loop control, vector: in loop: 0
     // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private assigned loop control, vector: in loop: 1
     #pragma acc loop vector private(i)
     for (i = 0; i < 2; ++i) {
       TGT_PRINTF("withinGangFn: parallel-local loop-private assigned loop control, vector: in loop: %d\n", i);
     }
     // PRT-NEXT: {{TGT_PRINTF|printf}}
     // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private assigned loop control, vector: after loop: 99
     // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private assigned loop control, vector: after loop: 99
     TGT_PRINTF("withinGangFn: parallel-local loop-private assigned loop control, vector: after loop: %d\n", i);

     //      DMP: ACCLoopDirective
     // DMP-NEXT:   ACCWorkerClause
     // DMP-NEXT:   ACCVectorClause
     // DMP-NEXT:   ACCPrivateClause
     // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
     // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
     // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
     // DMP-NEXT:   impl: CompoundStmt
     // DMP-NEXT:     DeclStmt
     // DMP-NEXT:       VarDecl {{.*}} i 'int'
     // DMP-NEXT:     OMPDistributeParallelForSimdDirective
     //      DMP:       ForStmt
     //
     // PRT-AO-NEXT: // v----------ACC----------v
     //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector private(i){{$}}
     //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
     //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
     //  PRT-A-NEXT: }
     // PRT-AO-NEXT: // ---------ACC->OMP--------
     // PRT-AO-NEXT: // {
     // PRT-AO-NEXT: //   int i;
     // PRT-AO-NEXT: //   #pragma omp distribute parallel for simd{{$}}
     // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
     // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
     // PRT-AO-NEXT: //   }
     // PRT-AO-NEXT: // }
     // PRT-AO-NEXT: // ^----------OMP----------^
     //
     // PRT-OA-NEXT: // v----------OMP----------v
     //  PRT-O-NEXT: {
     //  PRT-O-NEXT:   int i;
     //  PRT-O-NEXT:   {{^ *}}#pragma omp distribute parallel for simd{{$}}
     //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
     //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
     //  PRT-O-NEXT:   }
     //  PRT-O-NEXT: }
     // PRT-OA-NEXT: // ---------OMP<-ACC--------
     // PRT-OA-NEXT: // #pragma acc loop worker vector private(i){{$}}
     // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
     // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
     // PRT-OA-NEXT: // }
     // PRT-OA-NEXT: // ^----------ACC----------^
     //
     // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
     // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
     // PRT-NOACC-NEXT: }
     //
     // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private assigned loop control, worker vector: in loop: 0
     // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private assigned loop control, worker vector: in loop: 1
     #pragma acc loop worker vector private(i)
     for (i = 0; i < 2; ++i) {
       TGT_PRINTF("withinGangFn: parallel-local loop-private assigned loop control, worker vector: in loop: %d\n", i);
     }
     // PRT-NEXT: {{TGT_PRINTF|printf}}
     // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private assigned loop control, worker vector: after loop: 99
     // EXE-TGT-USE-STDIO-DAG: withinGangFn: parallel-local loop-private assigned loop control, worker vector: after loop: 99
     TGT_PRINTF("withinGangFn: parallel-local loop-private assigned loop control, worker vector: after loop: %d\n", i);
  } // PRT-NEXT: }

  //............................................................................
  // Check multiple privates in same clause and different clauses, including
  // private function-local scalar, private assigned loop control variable, and
  // private clauses that become empty or just smaller in translation to
  // OpenMP.
  //............................................................................

  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    // PRT: int j = 88;
    // PRT: int k = 77;
    int i = 99;
    int j = 88;
    int k = 77;

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} j 'int'
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} k 'int'
    // DMP-NEXT:     ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq private(j,i) private(k){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT:   j = k = 55;
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int j;
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   int k;
    // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //     j = k = 55;
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int j;
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   int k;
    //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:     j = k = 55;
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop seq private(j,i) private(k){{$}}
    // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: //   j = k = 55;
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT:   j = k = 55;
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: multiple privates on acc loop, seq: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: multiple privates on acc loop, seq: in loop: i=1
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: multiple privates on acc loop, seq: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: multiple privates on acc loop, seq: in loop: i=1
    #pragma acc loop seq private(j,i) private(k)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("withinGangFn: multiple privates on acc loop, seq: in loop: i=%d\n", i);
      j = k = 55;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: multiple privates on acc loop, seq: after loop: i=99, j=88, k=77
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: multiple privates on acc loop, seq: after loop: i=99, j=88, k=77
    TGT_PRINTF("withinGangFn: multiple privates on acc loop, seq: after loop: i=%d, j=%d, k=%d\n", i, j, k);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'k' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang private(j,i) private(k){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute private(j,i) private(k){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute private(j,i) private(k){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang private(j,i) private(k){{$}}
    //    PRT-NEXT: for (i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT:   j = k = 55;
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: multiple privates on acc loop, gang: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: multiple privates on acc loop, gang: in loop: i=1
    #pragma acc loop gang private(j,i) private(k)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("withinGangFn: multiple privates on acc loop, gang: in loop: i=%d\n", i);
      j = k = 55;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: multiple privates on acc loop, gang: after loop: i=99, j=88, k=77
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: multiple privates on acc loop, gang: after loop: i=99, j=88, k=77
    TGT_PRINTF("withinGangFn: multiple privates on acc loop, gang: after loop: i=%d, j=%d, k=%d\n", i, j, k);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'k' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker private(j,i) private(k){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for private(j,i) private(k){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for private(j,i) private(k){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker private(j,i) private(k){{$}}
    //    PRT-NEXT: for (i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT:   j = k = 55;
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: multiple privates on acc loop, worker: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: multiple privates on acc loop, worker: in loop: i=1
    #pragma acc loop worker private(j,i) private(k)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("withinGangFn: multiple privates on acc loop, worker: in loop: i=%d\n", i);
      j = k = 55;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: multiple privates on acc loop, worker: after loop: i=99, j=88, k=77
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: multiple privates on acc loop, worker: after loop: i=99, j=88, k=77
    TGT_PRINTF("withinGangFn: multiple privates on acc loop, worker: after loop: i=%d, j=%d, k=%d\n", i, j, k);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     OMPDistributeSimdDirective
    // DMP-NEXT:       OMPPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:       OMPPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'k' 'int'
    //      DMP:       ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector private(j,i) private(k){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT:   j = k = 55;
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   #pragma omp distribute simd private(j) private(k){{$}}
    // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //     j = k = 55;
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   {{^ *}}#pragma omp distribute simd private(j) private(k){{$}}
    //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:     j = k = 55;
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop vector private(j,i) private(k){{$}}
    // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: //   j = k = 55;
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT:   j = k = 55;
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: multiple privates on acc loop, vector: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: multiple privates on acc loop, vector: in loop: i=1
    #pragma acc loop vector private(j,i) private(k)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("withinGangFn: multiple privates on acc loop, vector: in loop: i=%d\n", i);
      j = k = 55;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: multiple privates on acc loop, vector: after loop: i=99, j=88, k=77
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: multiple privates on acc loop, vector: after loop: i=99, j=88, k=77
    TGT_PRINTF("withinGangFn: multiple privates on acc loop, vector: after loop: i=%d, j=%d, k=%d\n", i, j, k);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     OMPDistributeParallelForSimdDirective
    // DMP-NEXT:       OMPPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:       OMPPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'k' 'int'
    //      DMP:       ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector private(j,i) private(k){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT:   j = k = 55;
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   #pragma omp distribute parallel for simd private(j) private(k){{$}}
    // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //     j = k = 55;
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   {{^ *}}#pragma omp distribute parallel for simd private(j) private(k){{$}}
    //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:     j = k = 55;
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop worker vector private(j,i) private(k){{$}}
    // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: //   j = k = 55;
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT:   j = k = 55;
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: multiple privates on acc loop, worker vector: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: multiple privates on acc loop, worker vector: in loop: i=1
    #pragma acc loop worker vector private(j,i) private(k)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("withinGangFn: multiple privates on acc loop, worker vector: in loop: i=%d\n", i);
      j = k = 55;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: multiple privates on acc loop, worker vector: after loop: i=99, j=88, k=77
    // EXE-TGT-USE-STDIO-DAG: withinGangFn: multiple privates on acc loop, worker vector: after loop: i=99, j=88, k=77
    TGT_PRINTF("withinGangFn: multiple privates on acc loop, worker vector: after loop: i=%d, j=%d, k=%d\n", i, j, k);
  } // PRT-NEXT: }
} // PRT-NEXT: }

//------------------------------------------------------------------------------
// Check private clauses within a worker function.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} withinWorkerFn
// PRT-LABEL: void withinWorkerFn() {
void withinWorkerFn() {
  //............................................................................
  // Check loop private for scalar that is local to enclosing function.
  //............................................................................

  // PRT-NEXT: {
  {
    // PRT-NEXT: int i = 99;
    int i = 99;

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq private(i){{$}}
    //  PRT-A-NEXT: for (int j = 0; j < 2; ++j) {
    //  PRT-A-NEXT:   i = j;
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   for (int j = 0; j < 2; ++j) {
    // PRT-AO-NEXT: //     i = j;
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   for (int j = 0; j < 2; ++j) {
    //  PRT-O-NEXT:     i = j;
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop seq private(i){{$}}
    // PRT-OA-NEXT: // for (int j = 0; j < 2; ++j) {
    // PRT-OA-NEXT: //   i = j;
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (int j = 0; j < 2; ++j) {
    // PRT-NOACC-NEXT:   i = j;
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private scalar, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private scalar, seq: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private scalar, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private scalar, seq: in loop: 1
    #pragma acc loop seq private(i)
    for (int j = 0; j < 2; ++j) {
      i = j;
      TGT_PRINTF("withinWorkerFn: parallel-local loop-private scalar, seq: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private scalar, seq: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private scalar, seq: after loop: 99
    TGT_PRINTF("withinWorkerFn: parallel-local loop-private scalar, seq: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPParallelForDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp parallel for private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker private(i){{$}}
    //    PRT-NEXT: for (int j = 0; j < 2; ++j) {
    //    PRT-NEXT:   i = j;
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private scalar, worker: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private scalar, worker: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private scalar, worker: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private scalar, worker: in loop: 1
    #pragma acc loop worker private(i)
    for (int j = 0; j < 2; ++j) {
      i = j;
      TGT_PRINTF("withinWorkerFn: parallel-local loop-private scalar, worker: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private scalar, worker: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private scalar, worker: after loop: 99
    TGT_PRINTF("withinWorkerFn: parallel-local loop-private scalar, worker: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPSimdDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp simd private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp simd private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector private(i){{$}}
    //    PRT-NEXT: for (int j = 0; j < 2; ++j) {
    //    PRT-NEXT:   i = j;
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private scalar, vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private scalar, vector: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private scalar, vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private scalar, vector: in loop: 1
    #pragma acc loop vector private(i)
    for (int j = 0; j < 2; ++j) {
      i = j;
      TGT_PRINTF("withinWorkerFn: parallel-local loop-private scalar, vector: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private scalar, vector: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private scalar, vector: after loop: 99
    TGT_PRINTF("withinWorkerFn: parallel-local loop-private scalar, vector: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPParallelForSimdDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for simd private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp parallel for simd private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector private(i){{$}}
    //    PRT-NEXT: for (int j = 0; j < 2; ++j) {
    //    PRT-NEXT:   i = j;
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private scalar, worker vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private scalar, worker vector: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private scalar, worker vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private scalar, worker vector: in loop: 1
    #pragma acc loop worker vector private(i)
    for (int j = 0; j < 2; ++j) {
      i = j;
      TGT_PRINTF("withinWorkerFn: parallel-local loop-private scalar, worker vector: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private scalar, worker vector: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private scalar, worker vector: after loop: 99
    TGT_PRINTF("withinWorkerFn: parallel-local loop-private scalar, worker vector: after loop: %d\n", i);
  } // PRT-NEXT: }

  //............................................................................
  // Check private for loop control variable that is declared not assigned in
  // init of attached for loop.
  //............................................................................

  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    int i = 99;

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq private(i){{$}}
    //  PRT-A-NEXT: for (int i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   for (int i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   for (int i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop seq private(i){{$}}
    // PRT-OA-NEXT: // for (int i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT:for (int i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:  {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT:}
    //
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private declared loop control, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private declared loop control, seq: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private declared loop control, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private declared loop control, seq: in loop: 1
    #pragma acc loop seq private(i)
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("withinWorkerFn: parallel-local loop-private declared loop control, seq: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private declared loop control, seq: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private declared loop control, seq: after loop: 99
    TGT_PRINTF("withinWorkerFn: parallel-local loop-private declared loop control, seq: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPParallelForDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp parallel for private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker private(i){{$}}
    //    PRT-NEXT: for (int i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private declared loop control, worker: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private declared loop control, worker: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private declared loop control, worker: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private declared loop control, worker: in loop: 1
    #pragma acc loop worker private(i)
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("withinWorkerFn: parallel-local loop-private declared loop control, worker: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private declared loop control, worker: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private declared loop control, worker: after loop: 99
    TGT_PRINTF("withinWorkerFn: parallel-local loop-private declared loop control, worker: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPSimdDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp simd private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp simd private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector private(i){{$}}
    //    PRT-NEXT: for (int i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private declared loop control, vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private declared loop control, vector: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private declared loop control, vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private declared loop control, vector: in loop: 1
    #pragma acc loop vector private(i)
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("withinWorkerFn: parallel-local loop-private declared loop control, vector: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private declared loop control, vector: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private declared loop control, vector: after loop: 99
    TGT_PRINTF("withinWorkerFn: parallel-local loop-private declared loop control, vector: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPParallelForSimdDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for simd private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp parallel for simd private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector private(i){{$}}
    //    PRT-NEXT: for (int i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private declared loop control, worker vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private declared loop control, worker vector: in loop: 1
    #pragma acc loop worker vector private(i)
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("withinWorkerFn: parallel-local loop-private declared loop control, worker vector: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private declared loop control, worker vector: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private declared loop control, worker vector: after loop: 99
    TGT_PRINTF("withinWorkerFn: parallel-local loop-private declared loop control, worker vector: after loop: %d\n", i);
  } // PRT-NEXT: }

  //............................................................................
  // Check private for loop control variable that is assigned not declared in
  // init of attached for loop.
  //............................................................................

  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    int i = 99;

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq private(i){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop seq private(i){{$}}
    // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private assigned loop control, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private assigned loop control, seq: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private assigned loop control, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private assigned loop control, seq: in loop: 1
    #pragma acc loop seq private(i)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("withinWorkerFn: parallel-local loop-private assigned loop control, seq: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private assigned loop control, seq: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private assigned loop control, seq: after loop: 99
    TGT_PRINTF("withinWorkerFn: parallel-local loop-private assigned loop control, seq: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPParallelForDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp parallel for private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker private(i){{$}}
    //    PRT-NEXT: for (i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private assigned loop control, worker: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private assigned loop control, worker: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private assigned loop control, worker: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private assigned loop control, worker: in loop: 1
    #pragma acc loop worker private(i)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("withinWorkerFn: parallel-local loop-private assigned loop control, worker: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private assigned loop control, worker: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private assigned loop control, worker: after loop: 99
    TGT_PRINTF("withinWorkerFn: parallel-local loop-private assigned loop control, worker: after loop: %d\n", i);

     //      DMP: ACCLoopDirective
     // DMP-NEXT:   ACCVectorClause
     // DMP-NEXT:   ACCPrivateClause
     // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
     // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
     // DMP-NEXT:   impl: CompoundStmt
     // DMP-NEXT:     DeclStmt
     // DMP-NEXT:       VarDecl {{.*}} i 'int'
     // DMP-NEXT:     OMPSimdDirective
     //      DMP:       ForStmt
     //
     // PRT-AO-NEXT: // v----------ACC----------v
     //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector private(i){{$}}
     //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
     //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
     //  PRT-A-NEXT: }
     // PRT-AO-NEXT: // ---------ACC->OMP--------
     // PRT-AO-NEXT: // {
     // PRT-AO-NEXT: //   int i;
     // PRT-AO-NEXT: //   #pragma omp simd{{$}}
     // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
     // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
     // PRT-AO-NEXT: //   }
     // PRT-AO-NEXT: // }
     // PRT-AO-NEXT: // ^----------OMP----------^
     //
     // PRT-OA-NEXT: // v----------OMP----------v
     //  PRT-O-NEXT: {
     //  PRT-O-NEXT:   int i;
     //  PRT-O-NEXT:   {{^ *}}#pragma omp simd{{$}}
     //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
     //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
     //  PRT-O-NEXT:   }
     //  PRT-O-NEXT: }
     // PRT-OA-NEXT: // ---------OMP<-ACC--------
     // PRT-OA-NEXT: // #pragma acc loop vector private(i){{$}}
     // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
     // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
     // PRT-OA-NEXT: // }
     // PRT-OA-NEXT: // ^----------ACC----------^
     //
     // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
     // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
     // PRT-NOACC-NEXT: }
     //
     // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private assigned loop control, vector: in loop: 0
     // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private assigned loop control, vector: in loop: 1
     // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private assigned loop control, vector: in loop: 0
     // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private assigned loop control, vector: in loop: 1
     #pragma acc loop vector private(i)
     for (i = 0; i < 2; ++i) {
       TGT_PRINTF("withinWorkerFn: parallel-local loop-private assigned loop control, vector: in loop: %d\n", i);
     }
     // PRT-NEXT: {{TGT_PRINTF|printf}}
     // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private assigned loop control, vector: after loop: 99
     // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private assigned loop control, vector: after loop: 99
     TGT_PRINTF("withinWorkerFn: parallel-local loop-private assigned loop control, vector: after loop: %d\n", i);

     //      DMP: ACCLoopDirective
     // DMP-NEXT:   ACCWorkerClause
     // DMP-NEXT:   ACCVectorClause
     // DMP-NEXT:   ACCPrivateClause
     // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
     // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
     // DMP-NEXT:   impl: CompoundStmt
     // DMP-NEXT:     DeclStmt
     // DMP-NEXT:       VarDecl {{.*}} i 'int'
     // DMP-NEXT:     OMPParallelForSimdDirective
     //      DMP:       ForStmt
     //
     // PRT-AO-NEXT: // v----------ACC----------v
     //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector private(i){{$}}
     //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
     //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
     //  PRT-A-NEXT: }
     // PRT-AO-NEXT: // ---------ACC->OMP--------
     // PRT-AO-NEXT: // {
     // PRT-AO-NEXT: //   int i;
     // PRT-AO-NEXT: //   #pragma omp parallel for simd{{$}}
     // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
     // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
     // PRT-AO-NEXT: //   }
     // PRT-AO-NEXT: // }
     // PRT-AO-NEXT: // ^----------OMP----------^
     //
     // PRT-OA-NEXT: // v----------OMP----------v
     //  PRT-O-NEXT: {
     //  PRT-O-NEXT:   int i;
     //  PRT-O-NEXT:   {{^ *}}#pragma omp parallel for simd{{$}}
     //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
     //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
     //  PRT-O-NEXT:   }
     //  PRT-O-NEXT: }
     // PRT-OA-NEXT: // ---------OMP<-ACC--------
     // PRT-OA-NEXT: // #pragma acc loop worker vector private(i){{$}}
     // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
     // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
     // PRT-OA-NEXT: // }
     // PRT-OA-NEXT: // ^----------ACC----------^
     //
     // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
     // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
     // PRT-NOACC-NEXT: }
     //
     // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private assigned loop control, worker vector: in loop: 0
     // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private assigned loop control, worker vector: in loop: 1
     // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private assigned loop control, worker vector: in loop: 0
     // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private assigned loop control, worker vector: in loop: 1
     #pragma acc loop worker vector private(i)
     for (i = 0; i < 2; ++i) {
       TGT_PRINTF("withinWorkerFn: parallel-local loop-private assigned loop control, worker vector: in loop: %d\n", i);
     }
     // PRT-NEXT: {{TGT_PRINTF|printf}}
     // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private assigned loop control, worker vector: after loop: 99
     // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: parallel-local loop-private assigned loop control, worker vector: after loop: 99
     TGT_PRINTF("withinWorkerFn: parallel-local loop-private assigned loop control, worker vector: after loop: %d\n", i);
  } // PRT-NEXT: }

  //............................................................................
  // Check multiple privates in same clause and different clauses, including
  // private function-local scalar, private assigned loop control variable, and
  // private clauses that become empty or just smaller in translation to
  // OpenMP.
  //............................................................................

  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    // PRT: int j = 88;
    // PRT: int k = 77;
    int i = 99;
    int j = 88;
    int k = 77;

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} j 'int'
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} k 'int'
    // DMP-NEXT:     ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq private(j,i) private(k){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT:   j = k = 55;
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int j;
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   int k;
    // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //     j = k = 55;
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int j;
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   int k;
    //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:     j = k = 55;
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop seq private(j,i) private(k){{$}}
    // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: //   j = k = 55;
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT:   j = k = 55;
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: multiple privates on acc loop, seq: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: multiple privates on acc loop, seq: in loop: i=1
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: multiple privates on acc loop, seq: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: multiple privates on acc loop, seq: in loop: i=1
    #pragma acc loop seq private(j,i) private(k)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("withinWorkerFn: multiple privates on acc loop, seq: in loop: i=%d\n", i);
      j = k = 55;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: multiple privates on acc loop, seq: after loop: i=99, j=88, k=77
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: multiple privates on acc loop, seq: after loop: i=99, j=88, k=77
    TGT_PRINTF("withinWorkerFn: multiple privates on acc loop, seq: after loop: i=%d, j=%d, k=%d\n", i, j, k);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPParallelForDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'k' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker private(j,i) private(k){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for private(j,i) private(k){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp parallel for private(j,i) private(k){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker private(j,i) private(k){{$}}
    //    PRT-NEXT: for (i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT:   j = k = 55;
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: multiple privates on acc loop, worker: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: multiple privates on acc loop, worker: in loop: i=1
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: multiple privates on acc loop, worker: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: multiple privates on acc loop, worker: in loop: i=1
    #pragma acc loop worker private(j,i) private(k)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("withinWorkerFn: multiple privates on acc loop, worker: in loop: i=%d\n", i);
      j = k = 55;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: multiple privates on acc loop, worker: after loop: i=99, j=88, k=77
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: multiple privates on acc loop, worker: after loop: i=99, j=88, k=77
    TGT_PRINTF("withinWorkerFn: multiple privates on acc loop, worker: after loop: i=%d, j=%d, k=%d\n", i, j, k);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     OMPSimdDirective
    // DMP-NEXT:       OMPPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:       OMPPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'k' 'int'
    //      DMP:       ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector private(j,i) private(k){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT:   j = k = 55;
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   #pragma omp simd private(j) private(k){{$}}
    // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //     j = k = 55;
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   {{^ *}}#pragma omp simd private(j) private(k){{$}}
    //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:     j = k = 55;
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop vector private(j,i) private(k){{$}}
    // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: //   j = k = 55;
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT:   j = k = 55;
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: multiple privates on acc loop, vector: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: multiple privates on acc loop, vector: in loop: i=1
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: multiple privates on acc loop, vector: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: multiple privates on acc loop, vector: in loop: i=1
    #pragma acc loop vector private(j,i) private(k)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("withinWorkerFn: multiple privates on acc loop, vector: in loop: i=%d\n", i);
      j = k = 55;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: multiple privates on acc loop, vector: after loop: i=99, j=88, k=77
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: multiple privates on acc loop, vector: after loop: i=99, j=88, k=77
    TGT_PRINTF("withinWorkerFn: multiple privates on acc loop, vector: after loop: i=%d, j=%d, k=%d\n", i, j, k);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     OMPParallelForSimdDirective
    // DMP-NEXT:       OMPPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:       OMPPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'k' 'int'
    //      DMP:       ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector private(j,i) private(k){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT:   j = k = 55;
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   #pragma omp parallel for simd private(j) private(k){{$}}
    // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //     j = k = 55;
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   {{^ *}}#pragma omp parallel for simd private(j) private(k){{$}}
    //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:     j = k = 55;
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop worker vector private(j,i) private(k){{$}}
    // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: //   j = k = 55;
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT:   j = k = 55;
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: multiple privates on acc loop, worker vector: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: multiple privates on acc loop, worker vector: in loop: i=1
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: multiple privates on acc loop, worker vector: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: multiple privates on acc loop, worker vector: in loop: i=1
    #pragma acc loop worker vector private(j,i) private(k)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("withinWorkerFn: multiple privates on acc loop, worker vector: in loop: i=%d\n", i);
      j = k = 55;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: multiple privates on acc loop, worker vector: after loop: i=99, j=88, k=77
    // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: multiple privates on acc loop, worker vector: after loop: i=99, j=88, k=77
    TGT_PRINTF("withinWorkerFn: multiple privates on acc loop, worker vector: after loop: i=%d, j=%d, k=%d\n", i, j, k);
  } // PRT-NEXT: }
} // PRT-NEXT: }

//------------------------------------------------------------------------------
// Check private clauses within a vector function.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} withinVectorFn
// PRT-LABEL: void withinVectorFn() {
void withinVectorFn() {
  //............................................................................
  // Check loop private for scalar that is local to enclosing function.
  //............................................................................

  // PRT-NEXT: {
  {
    // PRT-NEXT: int i = 99;
    int i = 99;

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq private(i){{$}}
    //  PRT-A-NEXT: for (int j = 0; j < 2; ++j) {
    //  PRT-A-NEXT:   i = j;
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   for (int j = 0; j < 2; ++j) {
    // PRT-AO-NEXT: //     i = j;
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   for (int j = 0; j < 2; ++j) {
    //  PRT-O-NEXT:     i = j;
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop seq private(i){{$}}
    // PRT-OA-NEXT: // for (int j = 0; j < 2; ++j) {
    // PRT-OA-NEXT: //   i = j;
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (int j = 0; j < 2; ++j) {
    // PRT-NOACC-NEXT:   i = j;
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private scalar, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private scalar, seq: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private scalar, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private scalar, seq: in loop: 1
    #pragma acc loop seq private(i)
    for (int j = 0; j < 2; ++j) {
      i = j;
      TGT_PRINTF("withinVectorFn: parallel-local loop-private scalar, seq: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private scalar, seq: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private scalar, seq: after loop: 99
    TGT_PRINTF("withinVectorFn: parallel-local loop-private scalar, seq: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPSimdDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp simd private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp simd private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector private(i){{$}}
    //    PRT-NEXT: for (int j = 0; j < 2; ++j) {
    //    PRT-NEXT:   i = j;
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private scalar, vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private scalar, vector: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private scalar, vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private scalar, vector: in loop: 1
    #pragma acc loop vector private(i)
    for (int j = 0; j < 2; ++j) {
      i = j;
      TGT_PRINTF("withinVectorFn: parallel-local loop-private scalar, vector: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private scalar, vector: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private scalar, vector: after loop: 99
    TGT_PRINTF("withinVectorFn: parallel-local loop-private scalar, vector: after loop: %d\n", i);
  } // PRT-NEXT: }

  //............................................................................
  // Check private for loop control variable that is declared not assigned in
  // init of attached for loop.
  //............................................................................

  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    int i = 99;

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq private(i){{$}}
    //  PRT-A-NEXT: for (int i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   for (int i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   for (int i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop seq private(i){{$}}
    // PRT-OA-NEXT: // for (int i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT:for (int i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:  {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT:}
    //
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private declared loop control, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private declared loop control, seq: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private declared loop control, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private declared loop control, seq: in loop: 1
    #pragma acc loop seq private(i)
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("withinVectorFn: parallel-local loop-private declared loop control, seq: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private declared loop control, seq: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private declared loop control, seq: after loop: 99
    TGT_PRINTF("withinVectorFn: parallel-local loop-private declared loop control, seq: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPSimdDirective
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector private(i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp simd private(i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp simd private(i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector private(i){{$}}
    //    PRT-NEXT: for (int i = 0; i < 2; ++i) {
    //    PRT-NEXT:   {{TGT_PRINTF|printf}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private declared loop control, vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private declared loop control, vector: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private declared loop control, vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private declared loop control, vector: in loop: 1
    #pragma acc loop vector private(i)
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("withinVectorFn: parallel-local loop-private declared loop control, vector: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private declared loop control, vector: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private declared loop control, vector: after loop: 99
    TGT_PRINTF("withinVectorFn: parallel-local loop-private declared loop control, vector: after loop: %d\n", i);
  } // PRT-NEXT: }

  //............................................................................
  // Check private for loop control variable that is assigned not declared in
  // init of attached for loop.
  //............................................................................

  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    int i = 99;

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq private(i){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop seq private(i){{$}}
    // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private assigned loop control, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private assigned loop control, seq: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private assigned loop control, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private assigned loop control, seq: in loop: 1
    #pragma acc loop seq private(i)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("withinVectorFn: parallel-local loop-private assigned loop control, seq: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private assigned loop control, seq: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private assigned loop control, seq: after loop: 99
    TGT_PRINTF("withinVectorFn: parallel-local loop-private assigned loop control, seq: after loop: %d\n", i);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     OMPSimdDirective
    //      DMP:       ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector private(i){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   #pragma omp simd{{$}}
    // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   {{^ *}}#pragma omp simd{{$}}
    //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop vector private(i){{$}}
    // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private assigned loop control, vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private assigned loop control, vector: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private assigned loop control, vector: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private assigned loop control, vector: in loop: 1
    #pragma acc loop vector private(i)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("withinVectorFn: parallel-local loop-private assigned loop control, vector: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private assigned loop control, vector: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: parallel-local loop-private assigned loop control, vector: after loop: 99
    TGT_PRINTF("withinVectorFn: parallel-local loop-private assigned loop control, vector: after loop: %d\n", i);
  } // PRT-NEXT: }

  //............................................................................
  // Check multiple privates in same clause and different clauses, including
  // private function-local scalar, private assigned loop control variable, and
  // private clauses that become empty or just smaller in translation to
  // OpenMP.
  //............................................................................

  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    // PRT: int j = 88;
    // PRT: int k = 77;
    int i = 99;
    int j = 88;
    int k = 77;

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} j 'int'
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} k 'int'
    // DMP-NEXT:     ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq private(j,i) private(k){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT:   j = k = 55;
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int j;
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   int k;
    // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //     j = k = 55;
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int j;
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   int k;
    //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:     j = k = 55;
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop seq private(j,i) private(k){{$}}
    // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: //   j = k = 55;
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT:   j = k = 55;
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: multiple privates on acc loop, seq: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: multiple privates on acc loop, seq: in loop: i=1
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: multiple privates on acc loop, seq: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: multiple privates on acc loop, seq: in loop: i=1
    #pragma acc loop seq private(j,i) private(k)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("withinVectorFn: multiple privates on acc loop, seq: in loop: i=%d\n", i);
      j = k = 55;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: multiple privates on acc loop, seq: after loop: i=99, j=88, k=77
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: multiple privates on acc loop, seq: after loop: i=99, j=88, k=77
    TGT_PRINTF("withinVectorFn: multiple privates on acc loop, seq: after loop: i=%d, j=%d, k=%d\n", i, j, k);

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     OMPSimdDirective
    // DMP-NEXT:       OMPPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:       OMPPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'k' 'int'
    //      DMP:       ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector private(j,i) private(k){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT:   j = k = 55;
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   #pragma omp simd private(j) private(k){{$}}
    // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //     j = k = 55;
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   {{^ *}}#pragma omp simd private(j) private(k){{$}}
    //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:     j = k = 55;
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop vector private(j,i) private(k){{$}}
    // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: //   j = k = 55;
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT:   j = k = 55;
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: multiple privates on acc loop, vector: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: multiple privates on acc loop, vector: in loop: i=1
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: multiple privates on acc loop, vector: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: multiple privates on acc loop, vector: in loop: i=1
    #pragma acc loop vector private(j,i) private(k)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("withinVectorFn: multiple privates on acc loop, vector: in loop: i=%d\n", i);
      j = k = 55;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: multiple privates on acc loop, vector: after loop: i=99, j=88, k=77
    // EXE-TGT-USE-STDIO-DAG: withinVectorFn: multiple privates on acc loop, vector: after loop: i=99, j=88, k=77
    TGT_PRINTF("withinVectorFn: multiple privates on acc loop, vector: after loop: i=%d, j=%d, k=%d\n", i, j, k);
  } // PRT-NEXT: }
} // PRT-NEXT: }

//------------------------------------------------------------------------------
// Check private clauses within a seq function.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} withinSeqFn
// PRT-LABEL: void withinSeqFn() {
void withinSeqFn() {
  //............................................................................
  // Check loop private for scalar that is local to enclosing function.
  //............................................................................

  // PRT-NEXT: {
  {
    // PRT-NEXT: int i = 99;
    int i = 99;

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq private(i){{$}}
    //  PRT-A-NEXT: for (int j = 0; j < 2; ++j) {
    //  PRT-A-NEXT:   i = j;
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   for (int j = 0; j < 2; ++j) {
    // PRT-AO-NEXT: //     i = j;
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   for (int j = 0; j < 2; ++j) {
    //  PRT-O-NEXT:     i = j;
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop seq private(i){{$}}
    // PRT-OA-NEXT: // for (int j = 0; j < 2; ++j) {
    // PRT-OA-NEXT: //   i = j;
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (int j = 0; j < 2; ++j) {
    // PRT-NOACC-NEXT:   i = j;
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: parallel-local loop-private scalar, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: parallel-local loop-private scalar, seq: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: parallel-local loop-private scalar, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: parallel-local loop-private scalar, seq: in loop: 1
    #pragma acc loop seq private(i)
    for (int j = 0; j < 2; ++j) {
      i = j;
      TGT_PRINTF("withinSeqFn: parallel-local loop-private scalar, seq: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: parallel-local loop-private scalar, seq: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: parallel-local loop-private scalar, seq: after loop: 99
    TGT_PRINTF("withinSeqFn: parallel-local loop-private scalar, seq: after loop: %d\n", i);
  } // PRT-NEXT: }

  //............................................................................
  // Check private for loop control variable that is declared not assigned in
  // init of attached for loop.
  //............................................................................

  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    int i = 99;

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq private(i){{$}}
    //  PRT-A-NEXT: for (int i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   for (int i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   for (int i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop seq private(i){{$}}
    // PRT-OA-NEXT: // for (int i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT:for (int i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:  {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT:}
    //
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: parallel-local loop-private declared loop control, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: parallel-local loop-private declared loop control, seq: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: parallel-local loop-private declared loop control, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: parallel-local loop-private declared loop control, seq: in loop: 1
    #pragma acc loop seq private(i)
    for (int i = 0; i < 2; ++i) {
      TGT_PRINTF("withinSeqFn: parallel-local loop-private declared loop control, seq: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: parallel-local loop-private declared loop control, seq: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: parallel-local loop-private declared loop control, seq: after loop: 99
    TGT_PRINTF("withinSeqFn: parallel-local loop-private declared loop control, seq: after loop: %d\n", i);
  } // PRT-NEXT: }

  //............................................................................
  // Check private for loop control variable that is assigned not declared in
  // init of attached for loop.
  //............................................................................

  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    int i = 99;

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq private(i){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop seq private(i){{$}}
    // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: parallel-local loop-private assigned loop control, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: parallel-local loop-private assigned loop control, seq: in loop: 1
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: parallel-local loop-private assigned loop control, seq: in loop: 0
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: parallel-local loop-private assigned loop control, seq: in loop: 1
    #pragma acc loop seq private(i)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("withinSeqFn: parallel-local loop-private assigned loop control, seq: in loop: %d\n", i);
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: parallel-local loop-private assigned loop control, seq: after loop: 99
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: parallel-local loop-private assigned loop control, seq: after loop: 99
    TGT_PRINTF("withinSeqFn: parallel-local loop-private assigned loop control, seq: after loop: %d\n", i);
  } // PRT-NEXT: }

  //............................................................................
  // Check multiple privates in same clause and different clauses, including
  // private function-local scalar, private assigned loop control variable, and
  // private clauses that become empty or just smaller in translation to
  // OpenMP.
  //............................................................................

  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    // PRT: int j = 88;
    // PRT: int k = 77;
    int i = 99;
    int j = 88;
    int k = 77;

    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCSeqClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} j 'int'
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} k 'int'
    // DMP-NEXT:     ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq private(j,i) private(k){{$}}
    //  PRT-A-NEXT: for (i = 0; i < 2; ++i) {
    //  PRT-A-NEXT:   {{TGT_PRINTF|printf}}
    //  PRT-A-NEXT:   j = k = 55;
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int j;
    // PRT-AO-NEXT: //   int i;
    // PRT-AO-NEXT: //   int k;
    // PRT-AO-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: //     {{TGT_PRINTF|printf}}
    // PRT-AO-NEXT: //     j = k = 55;
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int j;
    //  PRT-O-NEXT:   int i;
    //  PRT-O-NEXT:   int k;
    //  PRT-O-NEXT:   for (i = 0; i < 2; ++i) {
    //  PRT-O-NEXT:     {{TGT_PRINTF|printf}}
    //  PRT-O-NEXT:     j = k = 55;
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop seq private(j,i) private(k){{$}}
    // PRT-OA-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: //   {{TGT_PRINTF|printf}}
    // PRT-OA-NEXT: //   j = k = 55;
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:   {{TGT_PRINTF|printf}}
    // PRT-NOACC-NEXT:   j = k = 55;
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: multiple privates on acc loop, seq: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: multiple privates on acc loop, seq: in loop: i=1
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: multiple privates on acc loop, seq: in loop: i=0
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: multiple privates on acc loop, seq: in loop: i=1
    #pragma acc loop seq private(j,i) private(k)
    for (i = 0; i < 2; ++i) {
      TGT_PRINTF("withinSeqFn: multiple privates on acc loop, seq: in loop: i=%d\n", i);
      j = k = 55;
    }
    // PRT-NEXT: {{TGT_PRINTF|printf}}
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: multiple privates on acc loop, seq: after loop: i=99, j=88, k=77
    // EXE-TGT-USE-STDIO-DAG: withinSeqFn: multiple privates on acc loop, seq: after loop: i=99, j=88, k=77
    TGT_PRINTF("withinSeqFn: multiple privates on acc loop, seq: after loop: i=%d, j=%d, k=%d\n", i, j, k);
  } // PRT-NEXT: }
} // PRT-NEXT: }

// EXE-NOT: {{.}}

int tentativeDef = 99;
