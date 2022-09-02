// Check an assigned loop control variable that has only a tentative definition.

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

int tentativeDef;

#pragma acc routine gang
static void withinGangFn();
#pragma acc routine worker
static void withinWorkerFn();
#pragma acc routine vector
static void withinVectorFn();
#pragma acc routine seq
static void withinSeqFn();

//==============================================================================
// Check within parallel constructs.
//==============================================================================

int main() {
  //----------------------------------------------------------------------------
  // gang vector loop
  //
  // Inserting a local definition to make a tentatively defined loop control
  // variable private for a vector-partitioned loop used to fail an assert
  // because VarDecl::getDefinition returned nullptr for the tentative
  // definition.
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "acc loop gang vector\n"
  // PRT-LABEL: printf("acc loop gang vector\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: acc loop gang vector{{$}}
  //   EXE-NOT: {{.}}
  printf("acc loop gang vector\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
  //  DMP-NOT:     OMP
  //      DMP:     CompoundStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(3){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(3){{$}}
  //    PRT-NEXT: {
  #pragma acc parallel num_gangs(3)
  {
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCVectorClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'tentativeDef' 'int'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} tentativeDef 'int'
    // DMP-NEXT:     OMPDistributeSimdDirective
    //      DMP:       ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang vector{{$}}
    //  PRT-A-NEXT: for (tentativeDef ={{.*}}) {
    //  PRT-A-NEXT:   {{printf|TGT_PRINTF}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int tentativeDef;
    // PRT-AO-NEXT: //   #pragma omp distribute simd{{$}}
    // PRT-AO-NEXT: //   for (tentativeDef ={{.*}}) {
    // PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int tentativeDef;
    //  PRT-O-NEXT:   {{^ *}}#pragma omp distribute simd{{$}}
    //  PRT-O-NEXT:   for (tentativeDef ={{.*}}) {
    //  PRT-O-NEXT:     {{printf|TGT_PRINTF}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop gang vector{{$}}
    // PRT-OA-NEXT: // for (tentativeDef ={{.*}}) {
    // PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (tentativeDef ={{.*}}) {
    // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    #pragma acc loop gang vector
    for (tentativeDef = 1; tentativeDef < 3; ++tentativeDef) {
      TGT_PRINTF("loop iteration\n");
    }
  } // PRT: }

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop gang vector\n"
  // PRT-LABEL: printf("acc parallel loop gang vector\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: acc parallel loop gang vector{{$}}
  //   EXE-NOT: {{.}}
  printf("acc parallel loop gang vector\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
  // DMP-NEXT:   ACCGangClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCGangClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCVectorClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       ACCPrivateClause {{.*}} <predetermined>
  // DMP-NEXT:         DeclRefExpr {{.*}} 'tentativeDef' 'int'
  // DMP-NEXT:       impl: CompoundStmt
  // DMP-NEXT:         DeclStmt
  // DMP-NEXT:           VarDecl {{.*}} tentativeDef 'int'
  // DMP-NEXT:         OMPDistributeSimdDirective
  //      DMP:           ForStmt
  //
  // PRT-AO-NEXT: // v----------ACC----------v
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(3) gang vector{{$}}
  //  PRT-A-NEXT: for (tentativeDef ={{.*}}) {
  //  PRT-A-NEXT:   {{printf|TGT_PRINTF}}
  //  PRT-A-NEXT: }
  // PRT-AO-NEXT: // ---------ACC->OMP--------
  // PRT-AO-NEXT: // #pragma omp target teams num_teams(3){{$}}
  // PRT-AO-NEXT: // {
  // PRT-AO-NEXT: //   int tentativeDef;
  // PRT-AO-NEXT: //   #pragma omp distribute simd{{$}}
  // PRT-AO-NEXT: //   for (tentativeDef ={{.*}}) {
  // PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
  // PRT-AO-NEXT: //   }
  // PRT-AO-NEXT: // }
  // PRT-AO-NEXT: // ^----------OMP----------^
  //
  // PRT-OA-NEXT: // v----------OMP----------v
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3){{$}}
  //  PRT-O-NEXT: {
  //  PRT-O-NEXT:   int tentativeDef;
  //  PRT-O-NEXT:   {{^ *}}#pragma omp distribute simd{{$}}
  //  PRT-O-NEXT:   for (tentativeDef ={{.*}}) {
  //  PRT-O-NEXT:     {{printf|TGT_PRINTF}}
  //  PRT-O-NEXT:   }
  //  PRT-O-NEXT: }
  //  PRT-OA-NEXT:// ---------OMP<-ACC--------
  // PRT-OA-NEXT:// #pragma acc parallel loop num_gangs(3) gang vector{{$}}
  //  PRT-OA-NEXT:// for (tentativeDef ={{.*}}) {
  //  PRT-OA-NEXT://   {{printf|TGT_PRINTF}}
  //  PRT-OA-NEXT:// }
  //  PRT-OA-NEXT:// ^----------ACC----------^
  //
  // PRT-NOACC-NEXT: for (tentativeDef ={{.*}}) {
  // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
  // PRT-NOACC-NEXT: }
  //
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel loop num_gangs(3) gang vector
  for (tentativeDef = 1; tentativeDef < 3; ++tentativeDef) {
    TGT_PRINTF("loop iteration\n");
  }

  //----------------------------------------------------------------------------
  // auto->seq loop.
  //
  // This is similar to the vector loop case that used to fail an assert because
  // it requires inserting a local definition to make the loop control variable
  // private.
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "acc loop auto\n"
  // PRT-LABEL: printf("acc loop auto\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: acc loop auto{{$}}
  //   EXE-NOT: {{.}}
  printf("acc loop auto\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
  //  DMP-NOT:     OMP
  //      DMP:     CompoundStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(3){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(3){{$}}
  //    PRT-NEXT: {
  #pragma acc parallel num_gangs(3)
  {
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCAutoClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'tentativeDef' 'int'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} tentativeDef 'int'
    // DMP-NEXT:       ForStmt
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto{{$}}
    //  PRT-A-NEXT: for (tentativeDef ={{.*}}) {
    //  PRT-A-NEXT:   {{printf|TGT_PRINTF}}
    //  PRT-A-NEXT: }
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int tentativeDef;
    // PRT-AO-NEXT: //   for (tentativeDef ={{.*}}) {
    // PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
    // PRT-AO-NEXT: //   }
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    //  PRT-O-NEXT: {
    //  PRT-O-NEXT:   int tentativeDef;
    //  PRT-O-NEXT:   for (tentativeDef ={{.*}}) {
    //  PRT-O-NEXT:     {{printf|TGT_PRINTF}}
    //  PRT-O-NEXT:   }
    //  PRT-O-NEXT: }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc loop auto{{$}}
    // PRT-OA-NEXT: // for (tentativeDef ={{.*}}) {
    // PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
    // PRT-OA-NEXT: // }
    // PRT-OA-NEXT: // ^----------ACC----------^
    //
    // PRT-NOACC-NEXT: for (tentativeDef ={{.*}}) {
    // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
    // PRT-NOACC-NEXT: }
    //
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    #pragma acc loop auto
    for (tentativeDef = 1; tentativeDef < 3; ++tentativeDef) {
      TGT_PRINTF("loop iteration\n");
    }
  } // PRT: }

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop auto\n"
  // PRT-LABEL: printf("acc parallel loop auto\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: acc parallel loop auto{{$}}
  //   EXE-NOT: {{.}}
  printf("acc parallel loop auto\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCAutoClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCPrivateClause {{.*}} <predetermined>
  // DMP-NEXT:         DeclRefExpr {{.*}} 'tentativeDef' 'int'
  // DMP-NEXT:       impl: CompoundStmt
  // DMP-NEXT:         DeclStmt
  // DMP-NEXT:           VarDecl {{.*}} tentativeDef 'int'
  // DMP-NEXT:         ForStmt
  //
  // PRT-AO-NEXT: // v----------ACC----------v
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(3) auto{{$}}
  //  PRT-A-NEXT: for (tentativeDef ={{.*}}) {
  //  PRT-A-NEXT:   {{printf|TGT_PRINTF}}
  //  PRT-A-NEXT: }
  // PRT-AO-NEXT: // ---------ACC->OMP--------
  // PRT-AO-NEXT: // #pragma omp target teams num_teams(3){{$}}
  // PRT-AO-NEXT: // {
  // PRT-AO-NEXT: //   int tentativeDef;
  // PRT-AO-NEXT: //   for (tentativeDef ={{.*}}) {
  // PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
  // PRT-AO-NEXT: //   }
  // PRT-AO-NEXT: // }
  // PRT-AO-NEXT: // ^----------OMP----------^
  //
  // PRT-OA-NEXT: // v----------OMP----------v
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3){{$}}
  //  PRT-O-NEXT: {
  //  PRT-O-NEXT:   int tentativeDef;
  //  PRT-O-NEXT:   for (tentativeDef ={{.*}}) {
  //  PRT-O-NEXT:     {{printf|TGT_PRINTF}}
  //  PRT-O-NEXT:   }
  //  PRT-O-NEXT: }
  // PRT-OA-NEXT: // ---------OMP<-ACC--------
  // PRT-OA-NEXT: // #pragma acc parallel loop num_gangs(3) auto{{$}}
  // PRT-OA-NEXT: // for (tentativeDef ={{.*}}) {
  // PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
  // PRT-OA-NEXT: // }
  // PRT-OA-NEXT: // ^----------ACC----------^
  //
  // PRT-NOACC-NEXT: for (tentativeDef ={{.*}}) {
  // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
  // PRT-NOACC-NEXT: }
  //
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel loop num_gangs(3) auto
  for (tentativeDef = 1; tentativeDef < 3; ++tentativeDef) {
    TGT_PRINTF("loop iteration\n");
  }

  //----------------------------------------------------------------------------
  // gang loop.
  //
  // The point here is to check some case that doesn't require inserting a local
  // definition of the loop control variable.
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "acc loop gang\n"
  // PRT-LABEL: printf("acc loop gang\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: acc loop gang{{$}}
  //   EXE-NOT: {{.}}
  printf("acc loop gang\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
  //  DMP-NOT:     OMP
  //     DMP:      CompoundStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(3){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(3){{$}}
  //    PRT-NEXT: {
  #pragma acc parallel num_gangs(3)
  {
    //      DMP: ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'tentativeDef' 'int'
    // DMP-NEXT:   impl: OMPDistributeDirective
    // DMP-NEXT:     OMPPrivateClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'tentativeDef' 'int'
    //      DMP:     ForStmt
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang{{$}}
    // PRT-AO-NEXT: // #pragma omp distribute private(tentativeDef){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp distribute private(tentativeDef){{$}}
    // PRT-OA-NEXT: // #pragma acc loop gang{{$}}
    //    PRT-NEXT: for (tentativeDef ={{.*}}) {
    //    PRT-NEXT:   {{printf|TGT_PRINTF}}
    //    PRT-NEXT: }
    //
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    // EXE-TGT-USE-STDIO-NEXT: loop iteration
    #pragma acc loop gang
    for (tentativeDef = 1; tentativeDef < 3; ++tentativeDef) {
      TGT_PRINTF("loop iteration\n");
    }
  } // PRT: }

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop gang\n"
  // PRT-LABEL: printf("acc parallel loop gang\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: acc parallel loop gang{{$}}
  //   EXE-NOT: {{.}}
  printf("acc parallel loop gang\n");

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
  // DMP-NEXT:   ACCGangClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCGangClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       ACCPrivateClause {{.*}} <predetermined>
  // DMP-NEXT:         DeclRefExpr {{.*}} 'tentativeDef' 'int'
  // DMP-NEXT:       impl: OMPDistributeDirective
  // DMP-NEXT:         OMPPrivateClause
  //  DMP-NOT:           <implicit>
  // DMP-NEXT:           DeclRefExpr {{.*}} 'tentativeDef' 'int'
  //      DMP:         ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(3) gang{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute private(tentativeDef){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(3){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute private(tentativeDef){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(3) gang{{$}}
  //    PRT-NEXT: for (tentativeDef ={{.*}}) {
  //    PRT-NEXT:   {{printf|TGT_PRINTF}}
  //    PRT-NEXT: }
  //
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  // EXE-TGT-USE-STDIO-NEXT: loop iteration
  #pragma acc parallel loop num_gangs(3) gang
  for (tentativeDef = 1; tentativeDef < 3; ++tentativeDef) {
    TGT_PRINTF("loop iteration\n");
  }

  //----------------------------------------------------------------------------
  // Check within accelerator routines.
  //----------------------------------------------------------------------------

  //   EXE-NOT: {{.}}
  // EXE-LABEL: within accelerator routines
  //   EXE-NOT: {{.}}
  printf("within accelerator routines\n");

  #pragma acc parallel num_gangs(3)
  {
    withinGangFn();
    withinWorkerFn();
    withinVectorFn();
    withinSeqFn();
  }

  return 0;
}

//==============================================================================
// Check within gang functions.
//
// This repeats the above testing but within a gang function and without
// statically enclosing parallel constructs.
//
// Because all cases here privatize tentativeDef, we don't seem to need a
// declare directive for it.  That's helpful as we haven't implemented the
// declare directive at the time of this writing.  An "acc loop seq" would not
// privatize tentativeDef and would require a declare directive, so we don't
// attempt to check it.
//==============================================================================

// DMP-LABEL: FunctionDecl {{.*}} withinGangFn
// PRT-LABEL: static void withinGangFn() {
static void withinGangFn() {
  //----------------------------------------------------------------------------
  // gang vector loop.
  //----------------------------------------------------------------------------

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'tentativeDef' 'int'
  // DMP-NEXT:   impl: CompoundStmt
  // DMP-NEXT:     DeclStmt
  // DMP-NEXT:       VarDecl {{.*}} tentativeDef 'int'
  // DMP-NEXT:     OMPDistributeSimdDirective
  //      DMP:       ForStmt
  //
  // PRT-AO-NEXT: // v----------ACC----------v
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang vector{{$}}
  //  PRT-A-NEXT: for (tentativeDef ={{.*}}) {
  //  PRT-A-NEXT:   {{printf|TGT_PRINTF}}
  //  PRT-A-NEXT: }
  // PRT-AO-NEXT: // ---------ACC->OMP--------
  // PRT-AO-NEXT: // {
  // PRT-AO-NEXT: //   int tentativeDef;
  // PRT-AO-NEXT: //   #pragma omp distribute simd{{$}}
  // PRT-AO-NEXT: //   for (tentativeDef ={{.*}}) {
  // PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
  // PRT-AO-NEXT: //   }
  // PRT-AO-NEXT: // }
  // PRT-AO-NEXT: // ^----------OMP----------^
  //
  // PRT-OA-NEXT: // v----------OMP----------v
  //  PRT-O-NEXT: {
  //  PRT-O-NEXT:   int tentativeDef;
  //  PRT-O-NEXT:   {{^ *}}#pragma omp distribute simd{{$}}
  //  PRT-O-NEXT:   for (tentativeDef ={{.*}}) {
  //  PRT-O-NEXT:     {{printf|TGT_PRINTF}}
  //  PRT-O-NEXT:   }
  //  PRT-O-NEXT: }
  // PRT-OA-NEXT: // ---------OMP<-ACC--------
  // PRT-OA-NEXT: // #pragma acc loop gang vector{{$}}
  // PRT-OA-NEXT: // for (tentativeDef ={{.*}}) {
  // PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
  // PRT-OA-NEXT: // }
  // PRT-OA-NEXT: // ^----------ACC----------^
  //
  // PRT-NOACC-NEXT: for (tentativeDef ={{.*}}) {
  // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
  // PRT-NOACC-NEXT: }
  //
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop gang vector: iteration 1
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop gang vector: iteration 2
  #pragma acc loop gang vector
  for (tentativeDef = 1; tentativeDef < 3; ++tentativeDef) {
    TGT_PRINTF("withinGangFn: acc loop gang vector: iteration %d\n", tentativeDef);
  }

  //----------------------------------------------------------------------------
  // auto->seq loop.
  //----------------------------------------------------------------------------

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'tentativeDef' 'int'
  // DMP-NEXT:   impl: CompoundStmt
  // DMP-NEXT:     DeclStmt
  // DMP-NEXT:       VarDecl {{.*}} tentativeDef 'int'
  // DMP-NEXT:       ForStmt
  //
  // PRT-AO-NEXT: // v----------ACC----------v
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto{{$}}
  //  PRT-A-NEXT: for (tentativeDef ={{.*}}) {
  //  PRT-A-NEXT:   {{printf|TGT_PRINTF}}
  //  PRT-A-NEXT: }
  // PRT-AO-NEXT: // ---------ACC->OMP--------
  // PRT-AO-NEXT: // {
  // PRT-AO-NEXT: //   int tentativeDef;
  // PRT-AO-NEXT: //   for (tentativeDef ={{.*}}) {
  // PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
  // PRT-AO-NEXT: //   }
  // PRT-AO-NEXT: // }
  // PRT-AO-NEXT: // ^----------OMP----------^
  //
  // PRT-OA-NEXT: // v----------OMP----------v
  //  PRT-O-NEXT: {
  //  PRT-O-NEXT:   int tentativeDef;
  //  PRT-O-NEXT:   for (tentativeDef ={{.*}}) {
  //  PRT-O-NEXT:     {{printf|TGT_PRINTF}}
  //  PRT-O-NEXT:   }
  //  PRT-O-NEXT: }
  // PRT-OA-NEXT: // ---------OMP<-ACC--------
  // PRT-OA-NEXT: // #pragma acc loop auto{{$}}
  // PRT-OA-NEXT: // for (tentativeDef ={{.*}}) {
  // PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
  // PRT-OA-NEXT: // }
  // PRT-OA-NEXT: // ^----------ACC----------^
  //
  // PRT-NOACC-NEXT: for (tentativeDef ={{.*}}) {
  // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
  // PRT-NOACC-NEXT: }
  //
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop auto: iteration 1
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop auto: iteration 2
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop auto: iteration 1
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop auto: iteration 2
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop auto: iteration 1
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop auto: iteration 2
  #pragma acc loop auto
  for (tentativeDef = 1; tentativeDef < 3; ++tentativeDef) {
    TGT_PRINTF("withinGangFn: acc loop auto: iteration %d\n", tentativeDef);
  }

  //----------------------------------------------------------------------------
  // gang loop.
  //----------------------------------------------------------------------------

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'tentativeDef' 'int'
  // DMP-NEXT:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPPrivateClause
  //  DMP-NOT:       <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 'tentativeDef' 'int'
  //      DMP:     ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang{{$}}
  // PRT-AO-NEXT: // #pragma omp distribute private(tentativeDef){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute private(tentativeDef){{$}}
  // PRT-OA-NEXT: // #pragma acc loop gang{{$}}
  //    PRT-NEXT: for (tentativeDef ={{.*}}) {
  //    PRT-NEXT:   {{printf|TGT_PRINTF}}
  //    PRT-NEXT: }
  //
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop gang: iteration 1
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: acc loop gang: iteration 2
  #pragma acc loop gang
  for (tentativeDef = 1; tentativeDef < 3; ++tentativeDef) {
    TGT_PRINTF("withinGangFn: acc loop gang: iteration %d\n", tentativeDef);
  }
} // PRT-NEXT: }

//==============================================================================
// Check within worker functions.
//
// This repeats the above testing but within a worker function, so gang clauses
// are changed to worker clauses.
//==============================================================================

// DMP-LABEL: FunctionDecl {{.*}} withinWorkerFn
// PRT-LABEL: static void withinWorkerFn() {
static void withinWorkerFn() {
  //----------------------------------------------------------------------------
  // worker vector loop.
  //----------------------------------------------------------------------------

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'tentativeDef' 'int'
  // DMP-NEXT:   impl: CompoundStmt
  // DMP-NEXT:     DeclStmt
  // DMP-NEXT:       VarDecl {{.*}} tentativeDef 'int'
  // DMP-NEXT:     OMPParallelForSimdDirective
  //      DMP:       ForStmt
  //
  // PRT-AO-NEXT: // v----------ACC----------v
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector{{$}}
  //  PRT-A-NEXT: for (tentativeDef ={{.*}}) {
  //  PRT-A-NEXT:   {{printf|TGT_PRINTF}}
  //  PRT-A-NEXT: }
  // PRT-AO-NEXT: // ---------ACC->OMP--------
  // PRT-AO-NEXT: // {
  // PRT-AO-NEXT: //   int tentativeDef;
  // PRT-AO-NEXT: //   #pragma omp parallel for simd{{$}}
  // PRT-AO-NEXT: //   for (tentativeDef ={{.*}}) {
  // PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
  // PRT-AO-NEXT: //   }
  // PRT-AO-NEXT: // }
  // PRT-AO-NEXT: // ^----------OMP----------^
  //
  // PRT-OA-NEXT: // v----------OMP----------v
  //  PRT-O-NEXT: {
  //  PRT-O-NEXT:   int tentativeDef;
  //  PRT-O-NEXT:   {{^ *}}#pragma omp parallel for simd{{$}}
  //  PRT-O-NEXT:   for (tentativeDef ={{.*}}) {
  //  PRT-O-NEXT:     {{printf|TGT_PRINTF}}
  //  PRT-O-NEXT:   }
  //  PRT-O-NEXT: }
  // PRT-OA-NEXT: // ---------OMP<-ACC--------
  // PRT-OA-NEXT: // #pragma acc loop worker vector{{$}}
  // PRT-OA-NEXT: // for (tentativeDef ={{.*}}) {
  // PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
  // PRT-OA-NEXT: // }
  // PRT-OA-NEXT: // ^----------ACC----------^
  //
  // PRT-NOACC-NEXT: for (tentativeDef ={{.*}}) {
  // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
  // PRT-NOACC-NEXT: }
  //
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop worker vector: iteration 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop worker vector: iteration 2
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop worker vector: iteration 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop worker vector: iteration 2
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop worker vector: iteration 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop worker vector: iteration 2
  #pragma acc loop worker vector
  for (tentativeDef = 1; tentativeDef < 3; ++tentativeDef) {
    TGT_PRINTF("withinWorkerFn: acc loop worker vector: iteration %d\n", tentativeDef);
  }

  //----------------------------------------------------------------------------
  // auto->seq loop.
  //----------------------------------------------------------------------------

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'tentativeDef' 'int'
  // DMP-NEXT:   impl: CompoundStmt
  // DMP-NEXT:     DeclStmt
  // DMP-NEXT:       VarDecl {{.*}} tentativeDef 'int'
  // DMP-NEXT:       ForStmt
  //
  // PRT-AO-NEXT: // v----------ACC----------v
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto{{$}}
  //  PRT-A-NEXT: for (tentativeDef ={{.*}}) {
  //  PRT-A-NEXT:   {{printf|TGT_PRINTF}}
  //  PRT-A-NEXT: }
  // PRT-AO-NEXT: // ---------ACC->OMP--------
  // PRT-AO-NEXT: // {
  // PRT-AO-NEXT: //   int tentativeDef;
  // PRT-AO-NEXT: //   for (tentativeDef ={{.*}}) {
  // PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
  // PRT-AO-NEXT: //   }
  // PRT-AO-NEXT: // }
  // PRT-AO-NEXT: // ^----------OMP----------^
  //
  // PRT-OA-NEXT: // v----------OMP----------v
  //  PRT-O-NEXT: {
  //  PRT-O-NEXT:   int tentativeDef;
  //  PRT-O-NEXT:   for (tentativeDef ={{.*}}) {
  //  PRT-O-NEXT:     {{printf|TGT_PRINTF}}
  //  PRT-O-NEXT:   }
  //  PRT-O-NEXT: }
  // PRT-OA-NEXT: // ---------OMP<-ACC--------
  // PRT-OA-NEXT: // #pragma acc loop auto{{$}}
  // PRT-OA-NEXT: // for (tentativeDef ={{.*}}) {
  // PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
  // PRT-OA-NEXT: // }
  // PRT-OA-NEXT: // ^----------ACC----------^
  //
  // PRT-NOACC-NEXT: for (tentativeDef ={{.*}}) {
  // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
  // PRT-NOACC-NEXT: }
  //
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop auto: iteration 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop auto: iteration 2
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop auto: iteration 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop auto: iteration 2
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop auto: iteration 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop auto: iteration 2
  #pragma acc loop auto
  for (tentativeDef = 1; tentativeDef < 3; ++tentativeDef) {
    TGT_PRINTF("withinWorkerFn: acc loop auto: iteration %d\n", tentativeDef);
  }

  //----------------------------------------------------------------------------
  // worker loop.
  //----------------------------------------------------------------------------

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'tentativeDef' 'int'
  // DMP-NEXT:   impl: OMPParallelForDirective
  // DMP-NEXT:     OMPPrivateClause
  //  DMP-NOT:       <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 'tentativeDef' 'int'
  //      DMP:     ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker{{$}}
  // PRT-AO-NEXT: // #pragma omp parallel for private(tentativeDef){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp parallel for private(tentativeDef){{$}}
  // PRT-OA-NEXT: // #pragma acc loop worker{{$}}
  //    PRT-NEXT: for (tentativeDef ={{.*}}) {
  //    PRT-NEXT:   {{printf|TGT_PRINTF}}
  //    PRT-NEXT: }
  //
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop worker: iteration 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop worker: iteration 2
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop worker: iteration 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop worker: iteration 2
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop worker: iteration 1
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: acc loop worker: iteration 2
  #pragma acc loop worker
  for (tentativeDef = 1; tentativeDef < 3; ++tentativeDef) {
    TGT_PRINTF("withinWorkerFn: acc loop worker: iteration %d\n", tentativeDef);
  }
} // PRT-NEXT: }

//==============================================================================
// Check within vector functions.
//
// This repeats the above testing but within a vector function, so worker
// clauses are dropped, and the worker loop case is dropped.
//==============================================================================

// DMP-LABEL: FunctionDecl {{.*}} withinVectorFn
// PRT-LABEL: static void withinVectorFn() {
static void withinVectorFn() {
  //----------------------------------------------------------------------------
  // vector loop.
  //----------------------------------------------------------------------------

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'tentativeDef' 'int'
  // DMP-NEXT:   impl: CompoundStmt
  // DMP-NEXT:     DeclStmt
  // DMP-NEXT:       VarDecl {{.*}} tentativeDef 'int'
  // DMP-NEXT:     OMPSimdDirective
  //      DMP:       ForStmt
  //
  // PRT-AO-NEXT: // v----------ACC----------v
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector{{$}}
  //  PRT-A-NEXT: for (tentativeDef ={{.*}}) {
  //  PRT-A-NEXT:   {{printf|TGT_PRINTF}}
  //  PRT-A-NEXT: }
  // PRT-AO-NEXT: // ---------ACC->OMP--------
  // PRT-AO-NEXT: // {
  // PRT-AO-NEXT: //   int tentativeDef;
  // PRT-AO-NEXT: //   #pragma omp simd{{$}}
  // PRT-AO-NEXT: //   for (tentativeDef ={{.*}}) {
  // PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
  // PRT-AO-NEXT: //   }
  // PRT-AO-NEXT: // }
  // PRT-AO-NEXT: // ^----------OMP----------^
  //
  // PRT-OA-NEXT: // v----------OMP----------v
  //  PRT-O-NEXT: {
  //  PRT-O-NEXT:   int tentativeDef;
  //  PRT-O-NEXT:   {{^ *}}#pragma omp simd{{$}}
  //  PRT-O-NEXT:   for (tentativeDef ={{.*}}) {
  //  PRT-O-NEXT:     {{printf|TGT_PRINTF}}
  //  PRT-O-NEXT:   }
  //  PRT-O-NEXT: }
  // PRT-OA-NEXT: // ---------OMP<-ACC--------
  // PRT-OA-NEXT: // #pragma acc loop vector{{$}}
  // PRT-OA-NEXT: // for (tentativeDef ={{.*}}) {
  // PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
  // PRT-OA-NEXT: // }
  // PRT-OA-NEXT: // ^----------ACC----------^
  //
  // PRT-NOACC-NEXT: for (tentativeDef ={{.*}}) {
  // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
  // PRT-NOACC-NEXT: }
  //
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop vector: iteration 1
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop vector: iteration 2
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop vector: iteration 1
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop vector: iteration 2
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop vector: iteration 1
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop vector: iteration 2
  #pragma acc loop vector
  for (tentativeDef = 1; tentativeDef < 3; ++tentativeDef) {
    TGT_PRINTF("withinVectorFn: acc loop vector: iteration %d\n", tentativeDef);
  }

  //----------------------------------------------------------------------------
  // auto->seq loop.
  //----------------------------------------------------------------------------

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'tentativeDef' 'int'
  // DMP-NEXT:   impl: CompoundStmt
  // DMP-NEXT:     DeclStmt
  // DMP-NEXT:       VarDecl {{.*}} tentativeDef 'int'
  // DMP-NEXT:       ForStmt
  //
  // PRT-AO-NEXT: // v----------ACC----------v
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto{{$}}
  //  PRT-A-NEXT: for (tentativeDef ={{.*}}) {
  //  PRT-A-NEXT:   {{printf|TGT_PRINTF}}
  //  PRT-A-NEXT: }
  // PRT-AO-NEXT: // ---------ACC->OMP--------
  // PRT-AO-NEXT: // {
  // PRT-AO-NEXT: //   int tentativeDef;
  // PRT-AO-NEXT: //   for (tentativeDef ={{.*}}) {
  // PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
  // PRT-AO-NEXT: //   }
  // PRT-AO-NEXT: // }
  // PRT-AO-NEXT: // ^----------OMP----------^
  //
  // PRT-OA-NEXT: // v----------OMP----------v
  //  PRT-O-NEXT: {
  //  PRT-O-NEXT:   int tentativeDef;
  //  PRT-O-NEXT:   for (tentativeDef ={{.*}}) {
  //  PRT-O-NEXT:     {{printf|TGT_PRINTF}}
  //  PRT-O-NEXT:   }
  //  PRT-O-NEXT: }
  // PRT-OA-NEXT: // ---------OMP<-ACC--------
  // PRT-OA-NEXT: // #pragma acc loop auto{{$}}
  // PRT-OA-NEXT: // for (tentativeDef ={{.*}}) {
  // PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
  // PRT-OA-NEXT: // }
  // PRT-OA-NEXT: // ^----------ACC----------^
  //
  // PRT-NOACC-NEXT: for (tentativeDef ={{.*}}) {
  // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
  // PRT-NOACC-NEXT: }
  //
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop auto: iteration 1
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop auto: iteration 2
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop auto: iteration 1
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop auto: iteration 2
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop auto: iteration 1
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: acc loop auto: iteration 2
  #pragma acc loop auto
  for (tentativeDef = 1; tentativeDef < 3; ++tentativeDef) {
    TGT_PRINTF("withinVectorFn: acc loop auto: iteration %d\n", tentativeDef);
  }
} // PRT-NEXT: }

//==============================================================================
// Check within seq functions.
//
// This repeats the above testing but within a seq function, so the vector loop
// case is dropped.
//==============================================================================

// DMP-LABEL: FunctionDecl {{.*}} withinSeqFn
// PRT-LABEL: static void withinSeqFn() {
static void withinSeqFn() {
  //----------------------------------------------------------------------------
  // auto->seq loop.
  //----------------------------------------------------------------------------

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'tentativeDef' 'int'
  // DMP-NEXT:   impl: CompoundStmt
  // DMP-NEXT:     DeclStmt
  // DMP-NEXT:       VarDecl {{.*}} tentativeDef 'int'
  // DMP-NEXT:       ForStmt
  //
  // PRT-AO-NEXT: // v----------ACC----------v
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto{{$}}
  //  PRT-A-NEXT: for (tentativeDef ={{.*}}) {
  //  PRT-A-NEXT:   {{printf|TGT_PRINTF}}
  //  PRT-A-NEXT: }
  // PRT-AO-NEXT: // ---------ACC->OMP--------
  // PRT-AO-NEXT: // {
  // PRT-AO-NEXT: //   int tentativeDef;
  // PRT-AO-NEXT: //   for (tentativeDef ={{.*}}) {
  // PRT-AO-NEXT: //     {{printf|TGT_PRINTF}}
  // PRT-AO-NEXT: //   }
  // PRT-AO-NEXT: // }
  // PRT-AO-NEXT: // ^----------OMP----------^
  //
  // PRT-OA-NEXT: // v----------OMP----------v
  //  PRT-O-NEXT: {
  //  PRT-O-NEXT:   int tentativeDef;
  //  PRT-O-NEXT:   for (tentativeDef ={{.*}}) {
  //  PRT-O-NEXT:     {{printf|TGT_PRINTF}}
  //  PRT-O-NEXT:   }
  //  PRT-O-NEXT: }
  // PRT-OA-NEXT: // ---------OMP<-ACC--------
  // PRT-OA-NEXT: // #pragma acc loop auto{{$}}
  // PRT-OA-NEXT: // for (tentativeDef ={{.*}}) {
  // PRT-OA-NEXT: //   {{printf|TGT_PRINTF}}
  // PRT-OA-NEXT: // }
  // PRT-OA-NEXT: // ^----------ACC----------^
  //
  // PRT-NOACC-NEXT: for (tentativeDef ={{.*}}) {
  // PRT-NOACC-NEXT:   {{printf|TGT_PRINTF}}
  // PRT-NOACC-NEXT: }
  //
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop auto: iteration 1
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop auto: iteration 2
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop auto: iteration 1
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop auto: iteration 2
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop auto: iteration 1
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: acc loop auto: iteration 2
  #pragma acc loop auto
  for (tentativeDef = 1; tentativeDef < 3; ++tentativeDef) {
    TGT_PRINTF("withinSeqFn: acc loop auto: iteration %d\n", tentativeDef);
  }
} // PRT-NEXT: }

// EXE-NOT: {{.}}
