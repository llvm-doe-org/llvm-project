// Check that OpenMP is ignored when OpenACC is enabled.

// Checking compilation after translation to OpenMP doesn't make sense for this
// test: -fopenacc-print=omp keeps all original source code including the
// original OpenMP directives, which would then be compiled.  However, this test
// is designed to ensure OpenMP directives are *not* effective, and so it only
// makes sense for traditional compilation.
//
// FIXME: amdgcn doesn't yet support printf in a kernel.  Unfortunately, that
// means most of our execution checks on amdgcn don't verify much except that
// nothing crashes.
//
// REDEFINE: %{all:clang:args-stable} = \
// REDEFINE:   -Wsource-uses-openmp -Wno-openacc-ignored-clause
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe-no-s2s}
//
// The warning about OpenMP is only triggered on the first occurrence, so above
// we saw it only for OpenMP outside OpenACC constructs.  Try again with OpenMP
// only in OpenACC constructs.
// REDEFINE: %{all:clang:args} = -DOMP_ONLY_IN_ACC
// RUN: %{acc-check-prt}

// END.

#include <stdio.h>

#if TGT_USE_STDIO
# define TGT_PRINTF(...) printf(__VA_ARGS__)
#else
# define TGT_PRINTF(...)
#endif

int main() {
  // PRT: printf("start\n");
  // EXE: start
  printf("start\n");

  //----------------------------------------------------------------------------
  // OpenMP without OpenACC
  //----------------------------------------------------------------------------

  // DMP-NOT: OMP{{[^ ]*}}Directive
  // DMP-NOT: OMP{{[^ ]*}}Clause
  //
  // PRT-SRC:      {{^ *}}#if !OMP_ONLY_IN_ACC
  // PRT-SRC-NEXT: {{^ *}}  /* noacc-
  // PRT-SRC-NEXT: {{^ *}}  /* acc-
  // PRT-SRC-NEXT: {{^ *}}  #pragma omp target teams num_teams(2)
  // PRT-SRC-NEXT: {{^ *}}#endif
  // PRT-NEXT:     {{^ *}}  {
  // PRT-NEXT:     {{^ *}}    printf("hello world\n");
  // PRT-SRC-NEXT: {{^ *}}#if !OMP_ONLY_IN_ACC
  // PRT-SRC-NEXT: {{^ *}}    #pragma omp distribute
  // PRT-SRC-NEXT: {{^ *}}#endif
  // PRT-NEXT:     {{^ *}}    for (int i = 0; i < 2; ++i)
  // PRT-NEXT:     {{^ *}}      printf("i=%d\n", i);
  // PRT-NEXT:     {{^ *}}  }
  //
  // If teams had an effect, "hello world" would print twice, and the order of
  // all these prints wouldn't be guaranteed.
  // EXE-NEXT: hello world
  // EXE-NEXT: i=0
  // EXE-NEXT: i=1
#if !OMP_ONLY_IN_ACC
  /* noacc-warning@+2 {{unexpected '#pragma omp ...' in program}} */
  /* acc-warning@+1 {{unexpected '#pragma omp ...' in program}} */
  #pragma omp target teams num_teams(2)
#endif
  {
    printf("hello world\n");
#if !OMP_ONLY_IN_ACC
    #pragma omp distribute
#endif
    for (int i = 0; i < 2; ++i)
      printf("i=%d\n", i);
  }

  //----------------------------------------------------------------------------
  // OpenMP within OpenACC where only directive is rewritten when printing
  //----------------------------------------------------------------------------

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-NOT:  OMP
  //
  // PRT-A:        {{^ *}}#pragma acc parallel num_gangs(2)
  // PRT-AO-NEXT:  {{^ *}}// #pragma omp target teams num_teams(2)
  // PRT-O:        {{^ *}}#pragma omp target teams num_teams(2)
  // PRT-OA-NEXT:  {{^ *}}// #pragma acc parallel num_gangs(2)
  // PRT-SRC-NEXT: {{^ *}}#pragma omp distribute
  // PRT-NEXT:     {{^ *}}for (int i = 0; i < 2; ++i)
  // PRT-NEXT:     {{^ *}}  {{printf|TGT_PRINTF}}("i=%d\n", i);
  //
  // If distribute had an effect, each value of i would print just once.
  // EXE-TGT-USE-STDIO-DAG: i=0
  // EXE-TGT-USE-STDIO-DAG: i=1
  // EXE-TGT-USE-STDIO-DAG: i=0
  // EXE-TGT-USE-STDIO-DAG: i=1
#if OMP_ONLY_IN_ACC
  /* noacc-warning@+4 {{unexpected '#pragma omp ...' in program}} */
  /* acc-warning@+3 {{unexpected '#pragma omp ...' in program}} */
#endif
  #pragma acc parallel num_gangs(2)
  #pragma omp distribute
  for (int i = 0; i < 2; ++i)
    TGT_PRINTF("i=%d\n", i);

  //----------------------------------------------------------------------------
  // OpenMP within OpenACC where full construct is rewritten when printing
  //
  // For the original OpenACC, -fopenacc-print prints the source OpenMP
  // directive.  As in other examples, -fopenacc-ast-print doesn't because the
  // source OpenMP directive was never parsed into the AST.  However, for the
  // OpenMP translation, even with -fopenacc-print, the rewritten construct is
  // printed from the AST, so the source OpenMP directive is not printed.
  //----------------------------------------------------------------------------

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     ParenExpr
  // DMP-NEXT:       BinaryOperator
  // DMP-NEXT:         DeclRefExpr
  // DMP-NEXT:           IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   impl: CompoundStmt
  // DMP-NEXT:     CStyleCastExpr {{.*}} 'void' <ToVoid>
  // DMP-NEXT:       ParenExpr
  // DMP-NEXT:         BinaryOperator
  // DMP-NEXT:           DeclRefExpr
  // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     OMPTargetTeamsDirective
  // DMP:        ACCLoopDirective
  // DMP-NEXT:     ACCVectorClause
  // DMP-NEXT:     ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:     ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:     impl: OMPDistributeSimdDirective
  // DMP:            ForStmt
  // DMP-NOT:          OMP
  // DMP:              ForStmt
  // DMP-NOT:            OMP
  // DMP:                CallExpr
  // DMP-NOT:  OMP
  //
  // PRT:             {{^ *}}int x = 0;
  //
  // PRT-AO-NEXT:     {{^ *}}// v----------ACC----------v
  // PRT-A-NEXT:      {{^ *}}#pragma acc parallel vector_length((x = 2))
  // PRT-A-NEXT:      {{^ *}}#pragma acc loop vector
  // PRT-A-NEXT:      {{^ *}}for (int i = 0; i < 2; ++i)
  // PRT-A-SRC-NEXT:  {{^ *}}  #pragma omp simd
  // PRT-A-NEXT:      {{^ *}}  for (int j = 0; j < 2; ++j)
  // PRT-A-NEXT:      {{^ *}}    {{printf|TGT_PRINTF}}("i=%d, j=%d\n", i, j);
  // PRT-AO-NEXT:     {{^ *}}// ---------ACC->OMP--------
  // PRT-AO-NEXT:     {{^ *}}// {
  // PRT-AO-NEXT:     {{^ *}}//   (void)(x = 2);
  // PRT-AO-NEXT:     {{^ *}}//   #pragma omp target teams
  // PRT-AO-NEXT:     {{^ *}}//   #pragma omp distribute simd
  // PRT-AO-NEXT:     {{^ *}}//   for (int i = 0; i < 2; ++i)
  // PRT-AO-NEXT:     {{^ *}}//     for (int j = 0; j < 2; ++j)
  // PRT-AO-NEXT:     {{^ *}}//       printf("i=%d, j=%d\n", i, j);
  // PRT-AO-NEXT:     {{^ *}}// }
  // PRT-AO-NEXT:     {{^ *}}// ^----------OMP----------^

  // PRT-OA-NEXT:     {{^ *}}// v----------OMP----------v
  // PRT-O-NEXT:      {{^ *}}{
  // PRT-O-NEXT:      {{^ *}}  (void)(x = 2);
  // PRT-O-NEXT:      {{^ *}}  #pragma omp target teams
  // PRT-O-NEXT:      {{^ *}}  #pragma omp distribute simd
  // PRT-O-NEXT:      {{^ *}}  for (int i = 0; i < 2; ++i)
  // PRT-O-NEXT:      {{^ *}}    for (int j = 0; j < 2; ++j)
  // PRT-O-NEXT:      {{^ *}}      printf("i=%d, j=%d\n", i, j);
  // PRT-O-NEXT:      {{^ *}}}
  // PRT-OA-NEXT:     {{^ *}}// ---------OMP<-ACC--------
  // PRT-OA-NEXT:     {{^ *}}// #pragma acc parallel vector_length((x = 2))
  // PRT-OA-NEXT:     {{^ *}}// #pragma acc loop vector
  // PRT-OA-NEXT:     {{^ *}}// for (int i = 0; i < 2; ++i)
  // PRT-OA-SRC-NEXT: {{^ *}}//   #pragma omp simd
  // PRT-OA-NEXT:     {{^ *}}//   for (int j = 0; j < 2; ++j)
  // PRT-OA-NEXT:     {{^ *}}//     {{printf|TGT_PRINTF}}("i=%d, j=%d\n", i, j);
  // PRT-OA-NEXT:     {{^ *}}// ^----------ACC----------^
  //
  // EXE-TGT-USE-STDIO-DAG: i=0, j=0
  // EXE-TGT-USE-STDIO-DAG: i=0, j=1
  // EXE-TGT-USE-STDIO-DAG: i=1, j=0
  // EXE-TGT-USE-STDIO-DAG: i=1, j=1
  int x = 0;
  #pragma acc parallel vector_length((x = 2))
  #pragma acc loop vector
  for (int i = 0; i < 2; ++i)
    #pragma omp simd
    for (int j = 0; j < 2; ++j)
      TGT_PRINTF("i=%d, j=%d\n", i, j);

  // EXE-NOT: {{.}}
  return 0;
}
