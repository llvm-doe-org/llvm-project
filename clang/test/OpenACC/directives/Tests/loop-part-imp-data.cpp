// Check "acc loop" partitioning and implicit data attribute cases not covered
// by loop-part-imp-data.c because they are specific to C++.
//
// REDEFINE: %{dmp:fc:args} = -strict-whitespace
// REDEFINE: %{exe:fc:args} = -strict-whitespace -match-full-lines \
// REDEFINE:                  -implicit-check-not='{{.}}'
// RUN: %{acc-check-dmp-cxx}
// RUN: %{acc-check-prt-cxx}
// RUN: %{acc-check-exe-cxx}

// END.

/* expected-no-diagnostics */

#include <stdio.h>

//------------------------------------------------------------------------------
// Check that template instantiation computes loop variable DA correctly.
//
// At one time, Clang tracked loop variables properly while parsing a template
// but not while instantiating it.  The effect was that, in the template
// instantiation, the loop var had a shared DA (instead of predetermined
// private) on the loop and then, as a result, an unnecessary firstprivate DA
// (instead of no DA) on the parallel construct.
//
// Dump checks are most important here as they show the computed attributes on
// the template instantiation.  Print checks don't show the template
// instantiation.  Execution checks didn't reveal the bug because OpenMP
// determines private for the loop var anyway.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionTemplateDecl {{.*}} loopVarDAInTemplateInstantiation
//  DMP-NEXT:   TemplateTypeParmDecl {{.*}} T
//  DMP-NEXT:   FunctionDecl {{.*}} loopVarDAInTemplateInstantiation 'void ()'
//  DMP-NEXT:     CompoundStmt
//       DMP:       ACCParallelLoopDirective
//  DMP-NEXT:         ACCGangClause
//  DMP-NEXT:         ACCNumGangsClause
//  DMP-NEXT:           IntegerLiteral {{.*}} 'int' 1
//  DMP-NEXT:         effect: ACCParallelDirective
//  DMP-NEXT:           ACCNumGangsClause
//  DMP-NEXT:             IntegerLiteral {{.*}} 'int' 1
//  DMP-NEXT:           impl: OMPTargetTeamsDirective
//  DMP-NEXT:             OMPNum_teamsClause
//  DMP-NEXT:               IntegerLiteral {{.*}} 'int' 1
//       DMP:           ACCLoopDirective
//  DMP-NEXT:             ACCGangClause
//  DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:             ACCPrivateClause {{.*}} <predetermined>
//  DMP-NEXT:               DeclRefExpr {{.*}} 'i' 'T'
//  DMP-NEXT:             impl: OMPDistributeDirective
//  DMP-NEXT:               OMPPrivateClause
//  DMP-NEXT:                 DeclRefExpr {{.*}} 'i' 'T'
//   DMP-NOT:               OMP
//       DMP:   FunctionDecl {{.*}} loopVarDAInTemplateInstantiation 'void ()'
//  DMP-NEXT:     TemplateArgument type 'int'
//  DMP-NEXT:       BuiltinType {{.*}} 'int'
//  DMP-NEXT:     CompoundStmt
//       DMP:       ACCParallelLoopDirective
//  DMP-NEXT:         ACCGangClause
//  DMP-NEXT:         ACCNumGangsClause
//  DMP-NEXT:           IntegerLiteral {{.*}} 'int' 1
//  DMP-NEXT:         effect: ACCParallelDirective
//  DMP-NEXT:           ACCNumGangsClause
//  DMP-NEXT:             IntegerLiteral {{.*}} 'int' 1
//  DMP-NEXT:           impl: OMPTargetTeamsDirective
//  DMP-NEXT:             OMPNum_teamsClause
//  DMP-NEXT:               IntegerLiteral {{.*}} 'int' 1
//       DMP:           ACCLoopDirective
//  DMP-NEXT:             ACCGangClause
//  DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:             ACCPrivateClause {{.*}} <predetermined>
//  DMP-NEXT:               DeclRefExpr {{.*}} 'i' 'int'
//  DMP-NEXT:             impl: OMPDistributeDirective
//  DMP-NEXT:               OMPPrivateClause
//  DMP-NEXT:                 DeclRefExpr {{.*}} 'i' 'int'
//   DMP-NOT:               OMP
//
//   PRT-LABEL: void loopVarDAInTemplateInstantiation() {
//    PRT-NEXT:   printf
//    PRT-NEXT:   T i =
//  PRT-A-NEXT:   {{^ *}}#pragma acc parallel loop gang num_gangs(1){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp target teams num_teams(1){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp distribute private(i){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp target teams num_teams(1){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp distribute private(i){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc parallel loop gang num_gangs(1){{$}}
//    PRT-NEXT:   for ({{.*}})
//    PRT-NEXT:     ;
//    PRT-NEXT:   printf
//    PRT-NEXT: }
//
// EXE-LABEL:loopVarDAInTemplateInstantiation
//  EXE-NEXT:i=99
template <typename T>
void loopVarDAInTemplateInstantiation() {
  printf("loopVarDAInTemplateInstantiation\n");
  T i = 99;
  #pragma acc parallel loop gang num_gangs(1)
  for (i = 0; i < 2; ++i)
    ;
  printf("i=%d\n", i);
}

int main() {
  loopVarDAInTemplateInstantiation<int>();
  return 0;
}
