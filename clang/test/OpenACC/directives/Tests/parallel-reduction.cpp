// Check "acc parallel" reduction cases not covered by parallel-reduction.c
// because they are specific to C++.

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
// with various operators.  Here, we just pick one combination and check that it
// works.
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

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCReductionClause {{.*}} '+'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'ref' 'int &'
  // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'ref' 'int &'
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     OMPReductionClause
  // DMP-NEXT:       DeclRefExpr {{.*}} 'ref' 'int &'
  // DMP-NEXT:     OMPMapClause
  // DMP-NEXT:       DeclRefExpr {{.*}} 'ref' 'int &'
  //  DMP-NOT:     OMP
  //      DMP:     CompoundAssignOperator {{.*}} '+='
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(4) reduction(+: ref){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) reduction(+: ref) map(ompx_hold,tofrom: ref){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(4) reduction(+: ref) map(ompx_hold,tofrom: ref){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(4) reduction(+: ref){{$}}
  //    PRT-NEXT: ref +=
  //
  // EXE-NEXT:tgt = 50
  //  EXE-NOT:{{.}}
  #pragma acc parallel num_gangs(4) reduction(+: ref)
  ref += 10;
  printf("tgt = %d\n", tgt);
}

int main() {
  refVars();
  return 0;
}
