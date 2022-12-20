// Check "acc loop" reduction cases not covered by loop-reduction.c because they
// are specific to C++.

// REDEFINE: %{dmp:fc:args} = -strict-whitespace
// REDEFINE: %{exe:fc:args} = -strict-whitespace -match-full-lines \
// REDEFINE:                  -implicit-check-not='{{.}}'
// RUN: %{acc-check-dmp-cxx}
// RUN: %{acc-check-prt-cxx}
// RUN: %{acc-check-exe-cxx}

// END.

/* expected-no-diagnostics */

#include <stdio.h>

// EXE-NOT:{{.}}

//------------------------------------------------------------------------------
// Check that reduction var type checking isn't confused by a reference type.
//
// Diagnostics tests more exhaustively check various reduction variable types
// with various operators.  Here, we just pick one combination and check that
// it works.
//------------------------------------------------------------------------------

void refVars() {
  // DMP-LABEL:"ref vars\n"
  // PRT-LABEL:"ref vars\n"
  // EXE-LABEL:ref vars
  printf("ref vars\n");

  // PRT-NEXT: int tgt =
  // PRT-NEXT: int &ref = tgt;
  int tgt = 10;
  int &ref = tgt;

  //      DMP: ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   ACCGangClause
  // DMP-NEXT:   ACCWorkerClause
  // DMP-NEXT:   ACCVectorClause
  // DMP-NEXT:   ACCReductionClause {{.*}} '*'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'ref' 'int &'
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 'ref' 'int &'
  // DMP-NEXT:     ACCReductionClause {{.*}} <implicit> '*'
  // DMP-NEXT:       DeclRefExpr {{.*}} 'ref' 'int &'
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:       OMPMapClause
  // DMP-NEXT:         DeclRefExpr {{.*}} 'ref' 'int &'
  // DMP-NEXT:       OMPReductionClause
  // DMP-NEXT:         DeclRefExpr {{.*}} 'ref' 'int &'
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCGangClause
  // DMP-NEXT:       ACCWorkerClause
  // DMP-NEXT:       ACCVectorClause
  // DMP-NEXT:       ACCReductionClause {{.*}} '*'
  // DMP-NEXT:         DeclRefExpr {{.*}} 'ref' 'int &'
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeParallelForSimdDirective
  // DMP-NEXT:         OMPReductionClause
  // DMP-NEXT:           DeclRefExpr {{.*}} 'ref' 'int &'
  //  DMP-NOT:       OMP
  //      DMP:       ForStmt
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs(2) gang worker vector reduction(*: ref){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: ref) reduction(*: ref){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd reduction(*: ref){{$}}
  //
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: ref) reduction(*: ref){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for simd reduction(*: ref){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) gang worker vector reduction(*: ref){{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   ref *=
  //
  // EXE-NEXT:tgt = 100000
  //  EXE-NOT:{{.}}
  #pragma acc parallel loop num_gangs(2) gang worker vector reduction(*: ref)
  for (int i = 0; i < 4; ++i)
    ref *= 10;
  printf("tgt = %d\n", tgt);
}

int main() {
  refVars();
  return 0;
}
