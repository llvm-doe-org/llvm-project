// Check a case for which clang used to misbehave due to clang's implementation
// of the pre-OpenMP-3.1 predetermined shared attribute for variables with
// const-qualified type having no mutable members.

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

int main() {
  //----------------------------------------------------------------------------
  // acc loop seq
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "acc loop seq\n"
  // PRT-LABEL: printf("acc loop seq\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: acc loop seq{{$}}
  //   EXE-NOT: {{.}}
  printf("acc loop seq\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'const int'
  // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'const int'
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:     OMPFirstprivateClause
  //  DMP-NOT:       <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'const int'
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCSeqClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:         DeclRefExpr {{.*}} 'x' 'const int'
  // DMP-NEXT:       impl: ForStmt
  //
  //    PRT-NEXT: {
  //    PRT-NEXT:   const int x = 55;
  //  PRT-A-NEXT:   {{^ *}}#pragma acc parallel num_gangs(1){{$}}
  // PRT-AO-NEXT:   {{^ *}}// #pragma omp target teams num_teams(1) firstprivate(x){{$}}
  //  PRT-O-NEXT:   {{^ *}}#pragma omp target teams num_teams(1) firstprivate(x){{$}}
  // PRT-OA-NEXT:   {{^ *}}// #pragma acc parallel num_gangs(1){{$}}
  //    PRT-NEXT:   {
  //  PRT-A-NEXT:     {{^ *}}#pragma acc loop seq
  // PRT-AO-SAME:     {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME:     {{^$}}
  // PRT-OA-NEXT:     {{^ *}}// #pragma acc loop seq // discarded in OpenMP translation{{$}}
  //    PRT-NEXT:     for (int i ={{.*}})
  //    PRT-NEXT:       {{printf|TGT_PRINTF}}
  //    PRT-NEXT:   }
  //    PRT-NEXT: }   
  //
  // EXE-TGT-USE-STDIO-NEXT: 0: 55{{$}}
  // EXE-TGT-USE-STDIO-NEXT: 1: 55{{$}}
  {
    const int x = 55;
    #pragma acc parallel num_gangs(1)
    {
      #pragma acc loop seq
      for (int i = 0; i < 2; ++i)
        TGT_PRINTF("%d: %d\n", i, x);
    }
  }

  //----------------------------------------------------------------------------
  // acc loop auto
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "acc loop auto\n"
  // PRT-LABEL: printf("acc loop auto\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: acc loop auto{{$}}
  //   EXE-NOT: {{.}}
  printf("acc loop auto\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'const int'
  // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'const int'
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:     OMPFirstprivateClause
  //  DMP-NOT:       <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'const int'
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCAutoClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:         DeclRefExpr {{.*}} 'x' 'const int'
  // DMP-NEXT:       impl: ForStmt
  //
  //    PRT-NEXT: {
  //    PRT-NEXT:   const int x = 55;
  //  PRT-A-NEXT:   {{^ *}}#pragma acc parallel num_gangs(1){{$}}
  // PRT-AO-NEXT:   {{^ *}}// #pragma omp target teams num_teams(1) firstprivate(x){{$}}
  //  PRT-O-NEXT:   {{^ *}}#pragma omp target teams num_teams(1) firstprivate(x){{$}}
  // PRT-OA-NEXT:   {{^ *}}// #pragma acc parallel num_gangs(1){{$}}
  //    PRT-NEXT:   {
  //  PRT-A-NEXT:     {{^ *}}#pragma acc loop auto
  // PRT-AO-SAME:     {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME:     {{^$}}
  // PRT-OA-NEXT:     {{^ *}}// #pragma acc loop auto // discarded in OpenMP translation{{$}}
  //    PRT-NEXT:     for (int i ={{.*}})
  //    PRT-NEXT:       {{printf|TGT_PRINTF}}
  //    PRT-NEXT:   }
  //    PRT-NEXT: }   
  //
  // EXE-TGT-USE-STDIO-NEXT: 0: 55{{$}}
  // EXE-TGT-USE-STDIO-NEXT: 1: 55{{$}}
  {
    const int x = 55;
    #pragma acc parallel num_gangs(1)
    {
      #pragma acc loop auto
      for (int i = 0; i < 2; ++i)
        TGT_PRINTF("%d: %d\n", i, x);
    }
  }

  //----------------------------------------------------------------------------
  // acc loop independent gang
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "acc loop independent gang\n"
  // PRT-LABEL: printf("acc loop independent gang\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: acc loop independent gang{{$}}
  //   EXE-NOT: {{.}}
  printf("acc loop independent gang\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'const int'
  // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'const int'
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:     OMPFirstprivateClause
  //  DMP-NOT:       <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'const int'
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCIndependentClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCGangClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:         DeclRefExpr {{.*}} 'x' 'const int'
  // DMP-NEXT:       impl: OMPDistributeDirective
  //  DMP-NOT:         OMP
  //      DMP:         ForStmt
  //
  //    PRT-NEXT: {
  //    PRT-NEXT:   const int x = 55;
  //  PRT-A-NEXT:   {{^ *}}#pragma acc parallel num_gangs(1){{$}}
  // PRT-AO-NEXT:   {{^ *}}// #pragma omp target teams num_teams(1) firstprivate(x){{$}}
  //  PRT-O-NEXT:   {{^ *}}#pragma omp target teams num_teams(1) firstprivate(x){{$}}
  // PRT-OA-NEXT:   {{^ *}}// #pragma acc parallel num_gangs(1){{$}}
  //    PRT-NEXT:   {
  //  PRT-A-NEXT:     {{^ *}}#pragma acc loop independent gang
  //  PRT-A-SAME:     {{^$}}
  // PRT-AO-NEXT:     {{^ *}}// #pragma omp distribute{{$}}
  //  PRT-O-NEXT:     {{^ *}}#pragma omp distribute{{$}}
  // PRT-OA-NEXT:     {{^ *}}// #pragma acc loop independent gang{{$}}
  //    PRT-NEXT:     for (int i ={{.*}})
  //    PRT-NEXT:       {{printf|TGT_PRINTF}}
  //    PRT-NEXT:   }
  //    PRT-NEXT: }
  //
  // EXE-TGT-USE-STDIO-DAG: 0: 55{{$}}
  // EXE-TGT-USE-STDIO-DAG: 1: 55{{$}}
  {
    const int x = 55;
    #pragma acc parallel num_gangs(1)
    {
      #pragma acc loop independent gang
      for (int i = 0; i < 2; ++i)
        TGT_PRINTF("%d: %d\n", i, x);
    }
  }

  //----------------------------------------------------------------------------
  // acc loop independent gang worker
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "acc loop independent gang worker\n"
  // PRT-LABEL: printf("acc loop independent gang worker\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: acc loop independent gang worker{{$}}
  //   EXE-NOT: {{.}}
  printf("acc loop independent gang worker\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'const int'
  // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'const int'
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:     OMPFirstprivateClause
  // DMP-NOT:        <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'const int'
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCIndependentClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCGangClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCWorkerClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:         DeclRefExpr {{.*}} 'x' 'const int'
  // DMP-NEXT:       impl: OMPDistributeParallelForDirective
  //  DMP-NOT:         OMP
  //      DMP:         ForStmt
  //
  //    PRT-NEXT: {
  //    PRT-NEXT:   const int x = 55;
  //  PRT-A-NEXT:   {{^ *}}#pragma acc parallel num_gangs(1){{$}}
  // PRT-AO-NEXT:   {{^ *}}// #pragma omp target teams num_teams(1) firstprivate(x){{$}}
  //  PRT-O-NEXT:   {{^ *}}#pragma omp target teams num_teams(1) firstprivate(x){{$}}
  // PRT-OA-NEXT:   {{^ *}}// #pragma acc parallel num_gangs(1){{$}}
  //    PRT-NEXT:   {
  //  PRT-A-NEXT:     {{^ *}}#pragma acc loop independent gang worker{{$}}
  // PRT-AO-NEXT:     {{^ *}}// #pragma omp distribute parallel for{{$}}
  //  PRT-O-NEXT:     {{^ *}}#pragma omp distribute parallel for{{$}}
  // PRT-OA-NEXT:     {{^ *}}// #pragma acc loop independent gang worker{{$}}
  //    PRT-NEXT:     for (int i ={{.*}})
  //    PRT-NEXT:       {{printf|TGT_PRINTF}}
  //    PRT-NEXT:   }
  //    PRT-NEXT: }
  //
  // EXE-TGT-USE-STDIO-DAG: 0: 55{{$}}
  // EXE-TGT-USE-STDIO-DAG: 1: 55{{$}}
  {
    const int x = 55;
    #pragma acc parallel num_gangs(1)
    {
      #pragma acc loop independent gang worker
      for (int i = 0; i < 2; ++i)
        TGT_PRINTF("%d: %d\n", i, x);
    }
  }

  //----------------------------------------------------------------------------
  // acc loop independent gang vector
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "acc loop independent gang vector\n"
  // PRT-LABEL: printf("acc loop independent gang vector\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: acc loop independent gang vector{{$}}
  //   EXE-NOT: {{.}}
  printf("acc loop independent gang vector\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'const int'
  // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'const int'
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:     OMPFirstprivateClause
  // DMP-NOT:        <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'const int'
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCIndependentClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCGangClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCVectorClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:         DeclRefExpr {{.*}} 'x' 'const int'
  // DMP-NEXT:       impl: OMPDistributeSimdDirective
  //  DMP-NOT:         OMP
  //      DMP:         ForStmt
  //
  //    PRT-NEXT: {
  //    PRT-NEXT:   const int x = 55;
  //  PRT-A-NEXT:   {{^ *}}#pragma acc parallel num_gangs(1){{$}}
  // PRT-AO-NEXT:   {{^ *}}// #pragma omp target teams num_teams(1) firstprivate(x){{$}}
  //  PRT-O-NEXT:   {{^ *}}#pragma omp target teams num_teams(1) firstprivate(x){{$}}
  // PRT-OA-NEXT:   {{^ *}}// #pragma acc parallel num_gangs(1){{$}}
  //    PRT-NEXT:   {
  //  PRT-A-NEXT:     {{^ *}}#pragma acc loop independent gang vector{{$}}
  // PRT-AO-NEXT:     {{^ *}}// #pragma omp distribute simd{{$}}
  //  PRT-O-NEXT:     {{^ *}}#pragma omp distribute simd{{$}}
  // PRT-OA-NEXT:     {{^ *}}// #pragma acc loop independent gang vector{{$}}
  //    PRT-NEXT:     for (int i ={{.*}})
  //    PRT-NEXT:       {{printf|TGT_PRINTF}}
  //    PRT-NEXT:   }
  //    PRT-NEXT: }
  //
  // EXE-TGT-USE-STDIO-DAG: 0: 55{{$}}
  // EXE-TGT-USE-STDIO-DAG: 1: 55{{$}}
  {
    const int x = 55;
    #pragma acc parallel num_gangs(1)
    {
      #pragma acc loop independent gang vector
      for (int i = 0; i < 2; ++i)
        TGT_PRINTF("%d: %d\n", i, x);
    }
  }

  //----------------------------------------------------------------------------
  // acc loop independent gang worker vector
  //----------------------------------------------------------------------------

  // DMP-LABEL: StringLiteral {{.*}} "acc loop independent gang worker vector\n"
  // PRT-LABEL: printf("acc loop independent gang worker vector\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: acc loop independent gang worker vector{{$}}
  //   EXE-NOT: {{.}}
  printf("acc loop independent gang worker vector\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'const int'
  // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'const int'
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:     OMPFirstprivateClause
  // DMP-NOT:        <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'const int'
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCIndependentClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCGangClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCWorkerClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCVectorClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:         DeclRefExpr {{.*}} 'x' 'const int'
  // DMP-NEXT:       impl: OMPDistributeParallelForSimdDirective
  //  DMP-NOT:         OMP
  //      DMP:         ForStmt
  //
  //    PRT-NEXT: {
  //    PRT-NEXT:   const int x = 55;
  //  PRT-A-NEXT:   {{^ *}}#pragma acc parallel num_gangs(1){{$}}
  // PRT-AO-NEXT:   {{^ *}}// #pragma omp target teams num_teams(1) firstprivate(x){{$}}
  //  PRT-O-NEXT:   {{^ *}}#pragma omp target teams num_teams(1) firstprivate(x){{$}}
  // PRT-OA-NEXT:   {{^ *}}// #pragma acc parallel num_gangs(1){{$}}
  //    PRT-NEXT:   {
  //  PRT-A-NEXT:     {{^ *}}#pragma acc loop independent gang worker vector{{$}}
  // PRT-AO-NEXT:     {{^ *}}// #pragma omp distribute parallel for simd{{$}}
  //  PRT-O-NEXT:     {{^ *}}#pragma omp distribute parallel for simd{{$}}
  // PRT-OA-NEXT:     {{^ *}}// #pragma acc loop independent gang worker vector{{$}}
  //    PRT-NEXT:     for (int i ={{.*}})
  //    PRT-NEXT:       {{printf|TGT_PRINTF}}
  //    PRT-NEXT:   }
  //    PRT-NEXT: }
  //
  // EXE-TGT-USE-STDIO-DAG: 0: 55{{$}}
  // EXE-TGT-USE-STDIO-DAG: 1: 55{{$}}
  {
    const int x = 55;
    #pragma acc parallel num_gangs(1)
    {
      #pragma acc loop independent gang worker vector
      for (int i = 0; i < 2; ++i)
        TGT_PRINTF("%d: %d\n", i, x);
    }
  }

  return 0;
}

// EXE-NOT: {{.}}
