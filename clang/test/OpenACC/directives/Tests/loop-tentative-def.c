// Check an assigned loop control variable that has only a tentative definition.

// RUN: %acc-check-dmp{}
// RUN: %acc-check-prt{}
// RUN: %acc-check-exe{}

// END.

/* expected-error 0 {{}} */

// FIXME: Clang produces spurious warning diagnostics for nvptx64 offload.  This
// issue is not limited to Clacc and is present upstream:
/* nvptx64-warning@*:* 0+ {{Linking two modules of different target triples}} */

#include <stdio.h>

#if TGT_USE_STDIO
# define TGT_PRINTF(...) printf(__VA_ARGS__)
#else
# define TGT_PRINTF(...)
#endif

int tentativeDef;

int main() {
  //----------------------------------------------------------------------------
  // vector loop.
  //
  // Inserting a local definition to make a tentatively defined loop control
  // variable private for a vector-partitioned loop used to fail an assert
  // because VarDecl::getDefinition returned nullptr for the tentative
  // definition.
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "acc loop vector\n"
  // PRT-LABEL: printf("acc loop vector\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: acc loop vector{{$}}
  //   EXE-NOT: {{.}}
  printf("acc loop vector\n");

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
    //  PRT-O-NEXT:           for (tentativeDef ={{.*}}) {
    //  PRT-O-NEXT:             {{printf|TGT_PRINTF}}
    //  PRT-O-NEXT:           }
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

  // DMP-LABEL: StringLiteral {{.*}} "acc parallel loop vector\n"
  // PRT-LABEL: printf("acc parallel loop vector\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: acc parallel loop vector{{$}}
  //   EXE-NOT: {{.}}
  printf("acc parallel loop vector\n");

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

  return 0;
}

// EXE-NOT: {{.}}
