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

//------------------------------------------------------------------------------
// Check that reduction vars whose types are template parameters are compiled
// correctly.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionTemplateDecl {{.*}} tmplTypeVar
//  DMP-NEXT:   TemplateTypeParmDecl {{.*}} T
//  DMP-NEXT:   FunctionDecl {{.*}} tmplTypeVar 'void (T)'
//  DMP-NEXT:     ParmVarDecl {{.*}} x 'T'
//  DMP-NEXT:     CompoundStmt
//  DMP-NEXT:       CallExpr
//       DMP:         DeclRefExpr {{.*}} 'x'
//  DMP-NEXT:       ACCParallelDirective
//  DMP-NEXT:         ACCNumGangsClause
//  DMP-NEXT:           IntegerLiteral {{.*}} 4
//  DMP-NEXT:         ACCReductionClause {{.*}} '*'
//  DMP-NEXT:           DeclRefExpr {{.*}} 'x' 'T'
//  DMP-NEXT:         ACCCopyClause {{.*}} <implicit>
//  DMP-NEXT:           DeclRefExpr {{.*}} 'x' 'T'
//  DMP-NEXT:         impl: OMPTargetTeamsDirective
//  DMP-NEXT:           OMPNum_teamsClause
//  DMP-NEXT:             IntegerLiteral {{.*}} 4
//  DMP-NEXT:           OMPReductionClause
//  DMP-NEXT:             DeclRefExpr {{.*}} 'x' 'T'
//  DMP-NEXT:           OMPMapClause
//  DMP-NEXT:             DeclRefExpr {{.*}} 'x' 'T'
//   DMP-NOT:           OMP
//       DMP:   FunctionDecl {{.*}} tmplTypeVar 'void (int)'
//  DMP-NEXT:     TemplateArgument type 'int'
//  DMP-NEXT:       BuiltinType {{.*}} 'int'
//  DMP-NEXT:     ParmVarDecl {{.*}} x 'int':'int'
//  DMP-NEXT:     CompoundStmt
//  DMP-NEXT:       CallExpr
//       DMP:         DeclRefExpr {{.*}} 'x'
//  DMP-NEXT:       ACCParallelDirective
//  DMP-NEXT:         ACCNumGangsClause
//  DMP-NEXT:           IntegerLiteral {{.*}} 4
//  DMP-NEXT:         ACCReductionClause {{.*}} '*'
//  DMP-NEXT:           DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:         ACCCopyClause {{.*}} <implicit>
//  DMP-NEXT:           DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:         impl: OMPTargetTeamsDirective
//  DMP-NEXT:           OMPNum_teamsClause
//  DMP-NEXT:             IntegerLiteral {{.*}} 4
//  DMP-NEXT:           OMPReductionClause
//  DMP-NEXT:             DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:           OMPMapClause
//  DMP-NEXT:             DeclRefExpr {{.*}} 'x' 'int'
//   DMP-NOT:           OMP{{.*}}Clause
//       DMP:   FunctionDecl {{.*}} tmplTypeVar 'void (double)'
//  DMP-NEXT:     TemplateArgument type 'double'
//  DMP-NEXT:       BuiltinType {{.*}} 'double'
//  DMP-NEXT:     ParmVarDecl {{.*}} x 'double':'double'
//  DMP-NEXT:     CompoundStmt
//  DMP-NEXT:       CallExpr
//       DMP:         DeclRefExpr {{.*}} 'x'
//  DMP-NEXT:       ACCParallelDirective
//  DMP-NEXT:         ACCNumGangsClause
//  DMP-NEXT:           IntegerLiteral {{.*}} 4
//  DMP-NEXT:         ACCReductionClause {{.*}} '*'
//  DMP-NEXT:           DeclRefExpr {{.*}} 'x' 'double'
//  DMP-NEXT:         ACCCopyClause {{.*}} <implicit>
//  DMP-NEXT:           DeclRefExpr {{.*}} 'x' 'double'
//  DMP-NEXT:         impl: OMPTargetTeamsDirective
//  DMP-NEXT:           OMPNum_teamsClause
//  DMP-NEXT:             IntegerLiteral {{.*}} 4
//  DMP-NEXT:           OMPReductionClause
//  DMP-NEXT:             DeclRefExpr {{.*}} 'x' 'double'
//  DMP-NEXT:           OMPMapClause
//  DMP-NEXT:             DeclRefExpr {{.*}} 'x' 'double'
//   DMP-NOT:           OMP{{.*}}Clause
//
//   PRT-LABEL: template <typename T> void tmplTypeVar(T x) {
//    PRT-NEXT:   printf
//  PRT-A-NEXT:   {{^ *}}#pragma acc parallel num_gangs(4) reduction(*: x){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp target teams num_teams(4) reduction(*: x) map(ompx_hold,tofrom: x){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp target teams num_teams(4) reduction(*: x) map(ompx_hold,tofrom: x){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc parallel num_gangs(4) reduction(*: x){{$}}
//    PRT-NEXT:   x *=
//    PRT-NEXT:   printf
//    PRT-NEXT: }
//
// EXE-LABEL:tmplTypeVar(3)
//  EXE-NEXT:  x=48
//   EXE-NOT:{{.}}
//
// EXE-LABEL:tmplTypeVar(3)
//  EXE-NEXT:  x=49
//   EXE-NOT:{{.}}
template <typename T> void tmplTypeVar(T x) {
  printf("tmplTypeVar(%d)\n", (int)x);
  #pragma acc parallel num_gangs(4) reduction(*: x)
  x *= 2;
  printf("  x=%d\n", (int)x);
}

int main() {
  refVars();
  tmplTypeVar(3);
  tmplTypeVar(3.1);
  return 0;
}
