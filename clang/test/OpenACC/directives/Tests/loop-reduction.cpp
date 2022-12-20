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

//------------------------------------------------------------------------------
// Check that reduction vars whose types are template parameters are compiled
// correctly.
//------------------------------------------------------------------------------

//..............................................................................
// Combined construct.

// DMP-LABEL: FunctionTemplateDecl {{.*}} tmplTypeVarOnCombined
//  DMP-NEXT:   TemplateTypeParmDecl {{.*}} T
//  DMP-NEXT:   FunctionDecl {{.*}} tmplTypeVarOnCombined 'void (T)'
//  DMP-NEXT:     ParmVarDecl {{.*}} x 'T'
//  DMP-NEXT:     CompoundStmt
//  DMP-NEXT:       CallExpr
//       DMP:         DeclRefExpr {{.*}} 'x'
//  DMP-NEXT:       ACCParallelLoopDirective
//  DMP-NEXT:         ACCNumGangsClause
//  DMP-NEXT:           IntegerLiteral {{.*}} 'int' 2
//  DMP-NEXT:         ACCGangClause
//  DMP-NEXT:         ACCWorkerClause
//  DMP-NEXT:         ACCVectorClause
//  DMP-NEXT:         ACCReductionClause {{.*}} '*'
//  DMP-NEXT:           DeclRefExpr {{.*}} 'x' 'T'
//  DMP-NEXT:         effect: ACCParallelDirective
//  DMP-NEXT:           ACCNumGangsClause
//  DMP-NEXT:             IntegerLiteral {{.*}} 'int' 2
//  DMP-NEXT:           ACCCopyClause {{.*}} <implicit>
//  DMP-NEXT:             DeclRefExpr {{.*}} 'x' 'T'
//  DMP-NEXT:           ACCReductionClause {{.*}} <implicit> '*'
//  DMP-NEXT:             DeclRefExpr {{.*}} 'x' 'T'
//  DMP-NEXT:           impl: OMPTargetTeamsDirective
//  DMP-NEXT:             OMPNum_teamsClause
//  DMP-NEXT:               IntegerLiteral {{.*}} 'int' 2
//  DMP-NEXT:             OMPMapClause
//  DMP-NEXT:               DeclRefExpr {{.*}} 'x' 'T'
//  DMP-NEXT:             OMPReductionClause
//  DMP-NEXT:               DeclRefExpr {{.*}} 'x' 'T'
//       DMP:           ACCLoopDirective
//  DMP-NEXT:             ACCGangClause
//  DMP-NEXT:             ACCWorkerClause
//  DMP-NEXT:             ACCVectorClause
//  DMP-NEXT:             ACCReductionClause {{.*}} '*'
//  DMP-NEXT:               DeclRefExpr {{.*}} 'x' 'T'
//  DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:             impl: OMPDistributeParallelForSimdDirective
//  DMP-NEXT:               OMPReductionClause
//  DMP-NEXT:                 DeclRefExpr {{.*}} 'x' 'T'
//   DMP-NOT:             OMP
//       DMP:             ForStmt
//       DMP:   FunctionDecl {{.*}} tmplTypeVarOnCombined 'void (int)'
//  DMP-NEXT:     TemplateArgument type 'int'
//  DMP-NEXT:       BuiltinType {{.*}} 'int'
//  DMP-NEXT:     ParmVarDecl {{.*}} x 'int':'int'
//  DMP-NEXT:     CompoundStmt
//  DMP-NEXT:       CallExpr
//       DMP:         DeclRefExpr {{.*}} 'x'
//  DMP-NEXT:       ACCParallelLoopDirective
//  DMP-NEXT:         ACCNumGangsClause
//  DMP-NEXT:           IntegerLiteral {{.*}} 'int' 2
//  DMP-NEXT:         ACCGangClause
//  DMP-NEXT:         ACCWorkerClause
//  DMP-NEXT:         ACCVectorClause
//  DMP-NEXT:         ACCReductionClause {{.*}} '*'
//  DMP-NEXT:           DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:         effect: ACCParallelDirective
//  DMP-NEXT:           ACCNumGangsClause
//  DMP-NEXT:             IntegerLiteral {{.*}} 'int' 2
//  DMP-NEXT:           ACCCopyClause {{.*}} <implicit>
//  DMP-NEXT:             DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:           ACCReductionClause {{.*}} <implicit> '*'
//  DMP-NEXT:             DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:           impl: OMPTargetTeamsDirective
//  DMP-NEXT:             OMPNum_teamsClause
//  DMP-NEXT:               IntegerLiteral {{.*}} 'int' 2
//  DMP-NEXT:             OMPMapClause
//  DMP-NEXT:               DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:             OMPReductionClause
//  DMP-NEXT:               DeclRefExpr {{.*}} 'x' 'int'
//       DMP:           ACCLoopDirective
//  DMP-NEXT:             ACCGangClause
//  DMP-NEXT:             ACCWorkerClause
//  DMP-NEXT:             ACCVectorClause
//  DMP-NEXT:             ACCReductionClause {{.*}} '*'
//  DMP-NEXT:               DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:             impl: OMPDistributeParallelForSimdDirective
//  DMP-NEXT:               OMPReductionClause
//  DMP-NEXT:                 DeclRefExpr {{.*}} 'x' 'int'
//   DMP-NOT:             OMP
//       DMP:             ForStmt
//       DMP:   FunctionDecl {{.*}} tmplTypeVarOnCombined 'void (double)'
//  DMP-NEXT:     TemplateArgument type 'double'
//  DMP-NEXT:       BuiltinType {{.*}} 'double'
//  DMP-NEXT:     ParmVarDecl {{.*}} x 'double':'double'
//  DMP-NEXT:     CompoundStmt
//  DMP-NEXT:       CallExpr
//       DMP:         DeclRefExpr {{.*}} 'x'
//  DMP-NEXT:       ACCParallelLoopDirective
//  DMP-NEXT:         ACCNumGangsClause
//  DMP-NEXT:           IntegerLiteral {{.*}} 'int' 2
//  DMP-NEXT:         ACCGangClause
//  DMP-NEXT:         ACCWorkerClause
//  DMP-NEXT:         ACCVectorClause
//  DMP-NEXT:         ACCReductionClause {{.*}} '*'
//  DMP-NEXT:           DeclRefExpr {{.*}} 'x' 'double'
//  DMP-NEXT:         effect: ACCParallelDirective
//  DMP-NEXT:           ACCNumGangsClause
//  DMP-NEXT:             IntegerLiteral {{.*}} 'int' 2
//  DMP-NEXT:           ACCCopyClause {{.*}} <implicit>
//  DMP-NEXT:             DeclRefExpr {{.*}} 'x' 'double'
//  DMP-NEXT:           ACCReductionClause {{.*}} <implicit> '*'
//  DMP-NEXT:             DeclRefExpr {{.*}} 'x' 'double'
//  DMP-NEXT:           impl: OMPTargetTeamsDirective
//  DMP-NEXT:             OMPNum_teamsClause
//  DMP-NEXT:               IntegerLiteral {{.*}} 'int' 2
//  DMP-NEXT:             OMPMapClause
//  DMP-NEXT:               DeclRefExpr {{.*}} 'x' 'double'
//  DMP-NEXT:             OMPReductionClause
//  DMP-NEXT:               DeclRefExpr {{.*}} 'x' 'double'
//       DMP:           ACCLoopDirective
//  DMP-NEXT:             ACCGangClause
//  DMP-NEXT:             ACCWorkerClause
//  DMP-NEXT:             ACCVectorClause
//  DMP-NEXT:             ACCReductionClause {{.*}} '*'
//  DMP-NEXT:               DeclRefExpr {{.*}} 'x' 'double'
//  DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:             impl: OMPDistributeParallelForSimdDirective
//  DMP-NEXT:               OMPReductionClause
//  DMP-NEXT:                 DeclRefExpr {{.*}} 'x' 'double'
//   DMP-NOT:             OMP
//       DMP:             ForStmt
//
//   PRT-LABEL: template <typename T> void tmplTypeVarOnCombined(T x) {
//    PRT-NEXT:   printf
//  PRT-A-NEXT:   {{^ *}}#pragma acc parallel loop num_gangs(2) gang worker vector reduction(*: x){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: x) reduction(*: x){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp distribute parallel for simd reduction(*: x){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: x) reduction(*: x){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp distribute parallel for simd reduction(*: x){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc parallel loop num_gangs(2) gang worker vector reduction(*: x){{$}}
//    PRT-NEXT:   for ({{.*}})
//    PRT-NEXT:     x *=
//    PRT-NEXT:   printf
//    PRT-NEXT: }
//
// EXE-LABEL:tmplTypeVarOnCombined(3)
//  EXE-NEXT:  x=48
//   EXE-NOT:{{.}}
//
// EXE-LABEL:tmplTypeVarOnCombined(3)
//  EXE-NEXT:  x=49
//   EXE-NOT:{{.}}
template <typename T> void tmplTypeVarOnCombined(T x) {
  printf("tmplTypeVarOnCombined(%d)\n", (int)x);
  #pragma acc parallel loop num_gangs(2) gang worker vector reduction(*: x)
  for (int i = 0; i < 4; ++i)
    x *= 2;
  printf("  x=%d\n", (int)x);
}

//..............................................................................
// Separate loop construct.

// DMP-LABEL: FunctionTemplateDecl {{.*}} tmplTypeVarOnSeparate
//  DMP-NEXT:   TemplateTypeParmDecl {{.*}} T
//  DMP-NEXT:   FunctionDecl {{.*}} tmplTypeVarOnSeparate 'void (T)'
//  DMP-NEXT:     ParmVarDecl {{.*}} x 'T'
//  DMP-NEXT:     CompoundStmt
//  DMP-NEXT:       CallExpr
//       DMP:         DeclRefExpr {{.*}} 'x'
//  DMP-NEXT:       ACCParallelDirective
//  DMP-NEXT:         ACCNumGangsClause
//  DMP-NEXT:           IntegerLiteral {{.*}} 'int' 2
//  DMP-NEXT:         ACCCopyClause
//   DMP-NOT:           <implicit>
//  DMP-NEXT:           DeclRefExpr {{.*}} 'x' 'T'
//  DMP-NEXT:         ACCReductionClause
//   DMP-NOT:           <implicit>
//  DMP-NEXT:           DeclRefExpr {{.*}} 'x' 'T'
//  DMP-NEXT:         impl: OMPTargetTeamsDirective
//  DMP-NEXT:           OMPNum_teamsClause
//  DMP-NEXT:             IntegerLiteral {{.*}} 'int' 2
//  DMP-NEXT:           OMPMapClause
//  DMP-NEXT:             DeclRefExpr {{.*}} 'x' 'T'
//  DMP-NEXT:           OMPReductionClause
//  DMP-NEXT:             DeclRefExpr {{.*}} 'x' 'T'
//       DMP:         ACCLoopDirective
//  DMP-NEXT:           ACCGangClause
//  DMP-NEXT:           ACCWorkerClause
//  DMP-NEXT:           ACCVectorClause
//  DMP-NEXT:           ACCReductionClause {{.*}} '*'
//  DMP-NEXT:             DeclRefExpr {{.*}} 'x' 'T'
//  DMP-NEXT:           ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:           impl: OMPDistributeParallelForSimdDirective
//  DMP-NEXT:             OMPReductionClause
//  DMP-NEXT:               DeclRefExpr {{.*}} 'x' 'T'
//   DMP-NOT:           OMP
//       DMP:           ForStmt
//       DMP:   FunctionDecl {{.*}} tmplTypeVarOnSeparate 'void (int)'
//  DMP-NEXT:     TemplateArgument type 'int'
//  DMP-NEXT:       BuiltinType {{.*}} 'int'
//  DMP-NEXT:     ParmVarDecl {{.*}} x 'int':'int'
//  DMP-NEXT:     CompoundStmt
//  DMP-NEXT:       CallExpr
//       DMP:         DeclRefExpr {{.*}} 'x'
//  DMP-NEXT:       ACCParallelDirective
//  DMP-NEXT:         ACCNumGangsClause
//  DMP-NEXT:           IntegerLiteral {{.*}} 'int' 2
//  DMP-NEXT:         ACCCopyClause {{.*}}
//   DMP-NOT:           <implicit>
//  DMP-NEXT:           DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:         ACCReductionClause
//   DMP-NOT:           <implicit>
//  DMP-NEXT:           DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:         impl: OMPTargetTeamsDirective
//  DMP-NEXT:           OMPNum_teamsClause
//  DMP-NEXT:             IntegerLiteral {{.*}} 'int' 2
//  DMP-NEXT:           OMPMapClause
//  DMP-NEXT:             DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:           OMPReductionClause
//  DMP-NEXT:             DeclRefExpr {{.*}} 'x' 'int'
//       DMP:         ACCLoopDirective
//  DMP-NEXT:           ACCGangClause
//  DMP-NEXT:           ACCWorkerClause
//  DMP-NEXT:           ACCVectorClause
//  DMP-NEXT:           ACCReductionClause {{.*}} '*'
//  DMP-NEXT:             DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:           ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:           impl: OMPDistributeParallelForSimdDirective
//  DMP-NEXT:             OMPReductionClause
//  DMP-NEXT:               DeclRefExpr {{.*}} 'x' 'int'
//   DMP-NOT:           OMP
//       DMP:           ForStmt
//       DMP:   FunctionDecl {{.*}} tmplTypeVarOnSeparate 'void (double)'
//  DMP-NEXT:     TemplateArgument type 'double'
//  DMP-NEXT:       BuiltinType {{.*}} 'double'
//  DMP-NEXT:     ParmVarDecl {{.*}} x 'double':'double'
//  DMP-NEXT:     CompoundStmt
//  DMP-NEXT:       CallExpr
//       DMP:         DeclRefExpr {{.*}} 'x'
//  DMP-NEXT:       ACCParallelDirective
//  DMP-NEXT:         ACCNumGangsClause
//  DMP-NEXT:           IntegerLiteral {{.*}} 'int' 2
//  DMP-NEXT:         ACCCopyClause
//   DMP-NOT:           <implicit>
//  DMP-NEXT:           DeclRefExpr {{.*}} 'x' 'double'
//  DMP-NEXT:         ACCReductionClause
//   DMP-NOT:           <implicit>
//  DMP-NEXT:           DeclRefExpr {{.*}} 'x' 'double'
//  DMP-NEXT:         impl: OMPTargetTeamsDirective
//  DMP-NEXT:           OMPNum_teamsClause
//  DMP-NEXT:             IntegerLiteral {{.*}} 'int' 2
//  DMP-NEXT:           OMPMapClause
//  DMP-NEXT:             DeclRefExpr {{.*}} 'x' 'double'
//  DMP-NEXT:           OMPReductionClause
//  DMP-NEXT:             DeclRefExpr {{.*}} 'x' 'double'
//       DMP:         ACCLoopDirective
//  DMP-NEXT:           ACCGangClause
//  DMP-NEXT:           ACCWorkerClause
//  DMP-NEXT:           ACCVectorClause
//  DMP-NEXT:           ACCReductionClause {{.*}} '*'
//  DMP-NEXT:             DeclRefExpr {{.*}} 'x' 'double'
//  DMP-NEXT:           ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:           impl: OMPDistributeParallelForSimdDirective
//  DMP-NEXT:             OMPReductionClause
//  DMP-NEXT:               DeclRefExpr {{.*}} 'x' 'double'
//   DMP-NOT:           OMP
//       DMP:           ForStmt
//
//   PRT-LABEL: template <typename T> void tmplTypeVarOnSeparate(T x) {
//    PRT-NEXT:   printf
//  PRT-A-NEXT:   {{^ *}}#pragma acc parallel num_gangs(2) copy(x) reduction(*: x){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: x) reduction(*: x){{$}}
//  PRT-A-NEXT:   {{^ *}}#pragma acc loop gang worker vector reduction(*: x){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp distribute parallel for simd reduction(*: x){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: x) reduction(*: x){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc parallel num_gangs(2) copy(x) reduction(*: x){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp distribute parallel for simd reduction(*: x){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc loop gang worker vector reduction(*: x){{$}}
//    PRT-NEXT:   for ({{.*}})
//    PRT-NEXT:     x *=
//    PRT-NEXT:   printf
//    PRT-NEXT: }
//
// EXE-LABEL:tmplTypeVarOnSeparate(3)
//  EXE-NEXT:  x=48
//   EXE-NOT:{{.}}
//
// EXE-LABEL:tmplTypeVarOnSeparate(3)
//  EXE-NEXT:  x=49
//   EXE-NOT:{{.}}
template <typename T> void tmplTypeVarOnSeparate(T x) {
  printf("tmplTypeVarOnSeparate(%d)\n", (int)x);
  #pragma acc parallel num_gangs(2) copy(x) reduction(*: x)
  #pragma acc loop gang worker vector reduction(*: x)
  for (int i = 0; i < 4; ++i)
    x *= 2;
  printf("  x=%d\n", (int)x);
}

//..............................................................................
// Orphaned loop construct.

// DMP-LABEL: FunctionTemplateDecl {{.*}} tmplTypeVarOnOrphaned_loop
//  DMP-NEXT:   TemplateTypeParmDecl {{.*}} T
//  DMP-NEXT:   FunctionDecl {{.*}} tmplTypeVarOnOrphaned_loop 'void (T *)'
//  DMP-NEXT:     ParmVarDecl {{.*}} xp 'T *'
//  DMP-NEXT:     CompoundStmt
//       DMP:       ACCLoopDirective
//  DMP-NEXT:         ACCGangClause
//  DMP-NEXT:         ACCWorkerClause
//  DMP-NEXT:         ACCVectorClause
//  DMP-NEXT:         ACCReductionClause {{.*}} '*'
//  DMP-NEXT:           DeclRefExpr {{.*}} 'x' 'T'
//  DMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:         impl: OMPDistributeParallelForSimdDirective
//  DMP-NEXT:           OMPReductionClause
//  DMP-NEXT:             DeclRefExpr {{.*}} 'x' 'T'
//   DMP-NOT:         OMP
//       DMP:         ForStmt
//       DMP:   FunctionDecl {{.*}} tmplTypeVarOnOrphaned_loop 'void (int *)'
//  DMP-NEXT:     TemplateArgument type 'int'
//  DMP-NEXT:       BuiltinType {{.*}} 'int'
//  DMP-NEXT:     ParmVarDecl {{.*}} xp 'int *'
//  DMP-NEXT:     CompoundStmt
//       DMP:       ACCLoopDirective
//  DMP-NEXT:         ACCGangClause
//  DMP-NEXT:         ACCWorkerClause
//  DMP-NEXT:         ACCVectorClause
//  DMP-NEXT:         ACCReductionClause {{.*}} '*'
//  DMP-NEXT:           DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:         impl: OMPDistributeParallelForSimdDirective
//  DMP-NEXT:           OMPReductionClause
//  DMP-NEXT:             DeclRefExpr {{.*}} 'x' 'int'
//   DMP-NOT:         OMP
//       DMP:         ForStmt
//       DMP:   FunctionDecl {{.*}} tmplTypeVarOnOrphaned_loop 'void (double *)'
//  DMP-NEXT:     TemplateArgument type 'double'
//  DMP-NEXT:       BuiltinType {{.*}} 'double'
//  DMP-NEXT:     ParmVarDecl {{.*}} xp 'double *'
//  DMP-NEXT:     CompoundStmt
//       DMP:       ACCLoopDirective
//  DMP-NEXT:         ACCGangClause
//  DMP-NEXT:         ACCWorkerClause
//  DMP-NEXT:         ACCVectorClause
//  DMP-NEXT:         ACCReductionClause {{.*}} '*'
//  DMP-NEXT:           DeclRefExpr {{.*}} 'x' 'double'
//  DMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:         impl: OMPDistributeParallelForSimdDirective
//  DMP-NEXT:           OMPReductionClause
//  DMP-NEXT:             DeclRefExpr {{.*}} 'x' 'double'
//   DMP-NOT:         OMP
//       DMP:         ForStmt
//
//   PRT-LABEL: template <typename T> void tmplTypeVarOnOrphaned_loop(T *xp) {
//    PRT-NEXT:   T x = *xp;
//  PRT-A-NEXT:   {{^ *}}#pragma acc loop gang worker vector reduction(*: x){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp distribute parallel for simd reduction(*: x){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp distribute parallel for simd reduction(*: x){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc loop gang worker vector reduction(*: x){{$}}
//    PRT-NEXT:   for ({{.*}})
//    PRT-NEXT:     x *=
//    PRT-NEXT:   *xp = x;
//    PRT-NEXT: }
//
// EXE-LABEL:tmplTypeVarOnOrphaned(3)
//  EXE-NEXT:  x=48
//   EXE-NOT:{{.}}
//
// EXE-LABEL:tmplTypeVarOnOrphaned(3)
//  EXE-NEXT:  x=49
//   EXE-NOT:{{.}}
#pragma acc routine gang
template <typename T> void tmplTypeVarOnOrphaned_loop(T *xp) {
  T x = *xp;
  #pragma acc loop gang worker vector reduction(*: x)
  for (int i = 0; i < 4; ++i)
    x *= 2;
  *xp = x;
}
template <typename T> void tmplTypeVarOnOrphaned(T x) {
  printf("tmplTypeVarOnOrphaned(%d)\n", (int)x);
  #pragma acc parallel num_gangs(2) copy(x) reduction(*: x)
  tmplTypeVarOnOrphaned_loop(&x);
  printf("  x=%d\n", (int)x);
}

int main() {
  refVars();
  tmplTypeVarOnCombined(3);
  tmplTypeVarOnCombined(3.1);
  tmplTypeVarOnSeparate(3);
  tmplTypeVarOnSeparate(3.1);
  tmplTypeVarOnOrphaned(3);
  tmplTypeVarOnOrphaned(3.1);
  return 0;
}
