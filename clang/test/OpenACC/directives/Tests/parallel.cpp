// Check simple "acc parallel" cases not covered by parallel.c because they
// are specific to C++.
//
// More complex cases involving specific clauses are checked elsewhere.

// REDEFINE: %{dmp:fc:args} = -strict-whitespace
// REDEFINE: %{exe:fc:args} = -strict-whitespace -match-full-lines \
// REDEFINE:                  -implicit-check-not='{{.}}'
// RUN: %{acc-check-dmp-cxx}
// RUN: %{acc-check-prt-cxx}
// RUN: %{acc-check-exe-cxx}

// END.

/* expected-no-diagnostics */

#include <stdio.h>

#if TGT_USE_STDIO
# define TGT_PRINTF(...) printf(__VA_ARGS__)
#else
# define TGT_PRINTF(...)
#endif

// EXE-NOT:{{.}}

//------------------------------------------------------------------------------
// Check a very simple acc parallel in a template.
//
// That is, check if template processing can correctly compile a simple OpenACC
// directive for which the OpenACC analysis doesn't have to worry about template
// parameters.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionTemplateDecl {{.*}} simple
//  DMP-NEXT:   TemplateTypeParmDecl {{.*}} T
//  DMP-NEXT:   FunctionDecl {{.*}} simple 'void (T)'
//  DMP-NEXT:     ParmVarDecl {{.*}} x 'T'
//  DMP-NEXT:     CompoundStmt
//  DMP-NEXT:       CallExpr
//       DMP:         DeclRefExpr {{.*}} 'x'
//  DMP-NEXT:       ACCParallelDirective
//  DMP-NEXT:         ACCNumGangsClause
//  DMP-NEXT:           IntegerLiteral {{.*}} 2
//  DMP-NEXT:         impl: OMPTargetTeamsDirective
//  DMP-NEXT:           OMPNum_teamsClause
//  DMP-NEXT:             IntegerLiteral {{.*}} 2
//   DMP-NOT:           OMP
//       DMP:   FunctionDecl {{.*}} simple 'void (int)'
//  DMP-NEXT:     TemplateArgument type 'int'
//  DMP-NEXT:       BuiltinType {{.*}} 'int'
//  DMP-NEXT:     ParmVarDecl {{.*}} x 'int':'int'
//  DMP-NEXT:     CompoundStmt
//  DMP-NEXT:       CallExpr
//       DMP:         DeclRefExpr {{.*}} 'x'
//  DMP-NEXT:       ACCParallelDirective
//  DMP-NEXT:         ACCNumGangsClause
//  DMP-NEXT:           IntegerLiteral {{.*}} 2
//  DMP-NEXT:         impl: OMPTargetTeamsDirective
//  DMP-NEXT:           OMPNum_teamsClause
//  DMP-NEXT:             IntegerLiteral {{.*}} 2
//   DMP-NOT:           OMP
//       DMP:   FunctionDecl {{.*}} simple 'void (double)'
//  DMP-NEXT:     TemplateArgument type 'double'
//  DMP-NEXT:       BuiltinType {{.*}} 'double'
//  DMP-NEXT:     ParmVarDecl {{.*}} x 'double':'double'
//  DMP-NEXT:     CompoundStmt
//  DMP-NEXT:       CallExpr
//       DMP:         DeclRefExpr {{.*}} 'x'
//  DMP-NEXT:       ACCParallelDirective
//  DMP-NEXT:         ACCNumGangsClause
//  DMP-NEXT:           IntegerLiteral {{.*}} 2
//  DMP-NEXT:         impl: OMPTargetTeamsDirective
//  DMP-NEXT:           OMPNum_teamsClause
//  DMP-NEXT:             IntegerLiteral {{.*}} 2
//   DMP-NOT:           OMP
//
//   PRT-LABEL: template <typename T> void simple(T x) {
//    PRT-NEXT:   printf
//  PRT-A-NEXT:   {{^ *}}#pragma acc parallel num_gangs(2){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp target teams num_teams(2){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp target teams num_teams(2){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc parallel num_gangs(2){{$}}
//    PRT-NEXT:   {{TGT_PRINTF|printf}}("hello world\n");
//    PRT-NEXT: }
//
//              EXE-LABEL:simple(3)
// EXE-TGT-USE-STDIO-NEXT:hello world
// EXE-TGT-USE-STDIO-NEXT:hello world
//                EXE-NOT:{{.}}
//
//              EXE-LABEL:simple(4)
// EXE-TGT-USE-STDIO-NEXT:hello world
// EXE-TGT-USE-STDIO-NEXT:hello world
//                EXE-NOT:{{.}}
template <typename T> void simple(T x) {
  printf("simple(%d)\n", (int)x);
  #pragma acc parallel num_gangs(2)
  TGT_PRINTF("hello world\n");
}

int main() {
  simple(3);
  simple(4.2);
  return 0;
}
