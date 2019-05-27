// RUN: %data accs {
// RUN:   (acc=-fopenacc                               )
// RUN:   (acc='-fopenacc-ast-print=acc     >/dev/null')
// RUN:   (acc='-fopenacc-ast-print=omp     >/dev/null')
// RUN:   (acc='-fopenacc-ast-print=acc-omp >/dev/null')
// RUN:   (acc='-fopenacc-ast-print=omp-acc >/dev/null')
// RUN:   (acc='-fopenacc-print=acc         >/dev/null')
// RUN:   (acc='-fopenacc-print=omp         >/dev/null')
// RUN:   (acc='-fopenacc-print=acc-omp     >/dev/null')
// RUN:   (acc='-fopenacc-print=omp-acc     >/dev/null')
// RUN: }
// RUN: %data prts {
// RUN:   (acc='-Xclang -ast-print -fsyntax-only -fopenacc' prt=PRT,PRT-A)
// RUN:   (acc=-fopenacc-ast-print=acc                      prt=PRT,PRT-A)
// RUN:   (acc=-fopenacc-ast-print=omp                      prt=PRT,PRT-O)
// RUN:   (acc=-fopenacc-ast-print=acc-omp                  prt=PRT,PRT-A,PRT-AO)
// RUN:   (acc=-fopenacc-ast-print=omp-acc                  prt=PRT,PRT-O,PRT-OA)
// RUN:   (acc=-fopenacc-print=acc                          prt=PRT,PRT-A,PRT-SRC,PRT-A-SRC)
// RUN:   (acc=-fopenacc-print=omp                          prt=PRT,PRT-O,PRT-SRC,PRT-O-SRC)
// RUN:   (acc=-fopenacc-print=acc-omp                      prt=PRT,PRT-A,PRT-AO,PRT-SRC,PRT-A-SRC,PRT-AO-SRC)
// RUN:   (acc=-fopenacc-print=omp-acc                      prt=PRT,PRT-O,PRT-OA,PRT-SRC,PRT-O-SRC,PRT-OA-SRC)
// RUN: }
// RUN: %for accs {
// RUN:   %clang_cc1 -verify -Wsource-uses-openmp %[acc] -DCC1 %s
// RUN:   %clang -Xclang -verify -Wsource-uses-openmp %[acc] %s
// RUN:   %clang -Xclang -verify -Wsource-uses-openmp %[acc] %s -DONLY_IN_ACC
// RUN: }
// RUN: %clang -Xclang -verify -Wsource-uses-openmp -fopenacc %s \
// RUN:        -Xclang -ast-dump -fsyntax-only \
// RUN: | FileCheck -check-prefix DMP %s
// RUN: %for prts {
// RUN:   %clang -Xclang -verify -Wsource-uses-openmp %[acc] %s \
// RUN:   | FileCheck -check-prefixes=%[prt] %s
// RUN: }
// RUN: %clang -Xclang -verify -Wsource-uses-openmp -fopenacc %s -o %t
// RUN: %t 2 2>&1 | FileCheck -check-prefixes=EXE %s
// END.

#if CC1
int printf(const char *, ...);
#else
# include <stdio.h>
#endif

int main() {
  // PRT: printf("start\n");
  // EXE: start
  printf("start\n");

  //--------------------------------------------------
  // OpenMP without OpenACC
  //--------------------------------------------------

  // DMP-NOT: OMP
  //
  // PRT-SRC:      {{^ *}}#if !ONLY_IN_ACC
  // PRT-SRC-NEXT: {{^ *}}  // expected
  // PRT-SRC-NEXT: {{^ *}}  #pragma omp target teams num_teams(2)
  // PRT-NEXT:     {{^ *}}  {
  // PRT-NEXT:     {{^ *}}    printf("hello world\n");
  // PRT-SRC-NEXT: {{^ *}}    #pragma omp distribute
  // PRT-NEXT:     {{^ *}}    for (int i = 0; i < 2; ++i)
  // PRT-NEXT:     {{^ *}}      printf("i=%d\n", i);
  // PRT-NEXT:     {{^ *}}  }
  // PRT-SRC-NEXT: {{^ *}}#endif
  //
  // If teams had an effect, "hello world" would print twice, and the order of
  // all these prints wouldn't be guaranteed.
  // EXE-NEXT: hello world
  // EXE-NEXT: i=0
  // EXE-NEXT: i=1
#if !ONLY_IN_ACC
  // expected-warning@+1 {{unexpected '#pragma omp ...' in program}}
  #pragma omp target teams num_teams(2)
  {
    printf("hello world\n");
    #pragma omp distribute
    for (int i = 0; i < 2; ++i)
      printf("i=%d\n", i);
  }
#endif

  //--------------------------------------------------
  // OpenMP within OpenACC where only directive is rewritten when printing
  //--------------------------------------------------

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNum_gangsClause
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
  // PRT-NEXT:     {{^ *}}  printf("i=%d\n", i);
  //
  // If distribute had an effect, each value of i would print just once.
  // EXE-DAG: i=0
  // EXE-DAG: i=1
  // EXE-DAG: i=0
  // EXE-DAG: i=1
#if ONLY_IN_ACC
  // expected-warning@+3 {{unexpected '#pragma omp ...' in program}}
#endif
  #pragma acc parallel num_gangs(2)
  #pragma omp distribute
  for (int i = 0; i < 2; ++i)
    printf("i=%d\n", i);

  //--------------------------------------------------
  // OpenMP within OpenACC where full construct is rewritten when printing
  //
  // For the original OpenACC, -fopenacc-print prints the source OpenMP
  // directive.  As in other examples, -fopenacc-ast-print doesn't because the
  // source OpenMP directive was never parsed into the AST.  However, for the
  // OpenMP translation, even with -fopenacc-print, the rewritten construct is
  // printed from the AST, so the source OpenMP directive is not printed.
  //--------------------------------------------------

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNum_workersClause
  // DMP-NEXT:     ParenExpr
  // DMP-NEXT:       BinaryOperator
  // DMP-NEXT:         DeclRefExpr
  // DMP-NEXT:           IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:   impl: CompoundStmt
  // DMP-NEXT:     DeclStmt
  // DMP-NEXT:       VarDecl {{.*}} __clang_acc_num_workers__
  // DMP-NEXT:       ParenExpr
  // DMP-NEXT:         BinaryOperator
  // DMP-NEXT:           DeclRefExpr
  // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 2
  // DMP-NEXT:     OMPTargetTeamsDirective
  // DMP-NEXT:       OMPFirstprivateClause {{.*}} <implicit>
  // DMP-NEXT:         DeclRefExpr {{.*}} '__clang_acc_num_workers__'
  // DMP:        ACCLoopDirective
  // DMP-NEXT:     ACCWorkerClause
  // DMP-NEXT:     ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:     impl: OMPParallelForDirective
  // DMP-NEXT:       OMPNum_threadsClause
  // DMP-NEXT:         DeclRefExpr {{.*}} '__clang_acc_num_workers__'
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
  // PRT-A-NEXT:      {{^ *}}#pragma acc parallel num_workers((x = 2))
  // PRT-A-NEXT:      {{^ *}}#pragma acc loop worker
  // PRT-A-NEXT:      {{^ *}}for (int i = 0; i < 2; ++i)
  // PRT-A-SRC-NEXT:  {{^ *}}  #pragma omp simd
  // PRT-A-NEXT:      {{^ *}}  for (int j = 0; j < 2; ++j)
  // PRT-A-NEXT:      {{^ *}}    printf("i=%d, j=%d\n", i, j);
  // PRT-AO-NEXT:     {{^ *}}// ---------ACC->OMP--------
  // PRT-AO-NEXT:     {{^ *}}// {
  // PRT-AO-NEXT:     {{^ *}}//   const int __clang_acc_num_workers__ = (x = 2);
  // PRT-AO-NEXT:     {{^ *}}//   #pragma omp target teams
  // PRT-AO-NEXT:     {{^ *}}//   #pragma omp parallel for num_threads(__clang_acc_num_workers__)
  // PRT-AO-NEXT:     {{^ *}}//   for (int i = 0; i < 2; ++i)
  // PRT-AO-NEXT:     {{^ *}}//     for (int j = 0; j < 2; ++j)
  // PRT-AO-NEXT:     {{^ *}}//       printf("i=%d, j=%d\n", i, j);
  // PRT-AO-NEXT:     {{^ *}}// }
  // PRT-AO-NEXT:     {{^ *}}// ^----------OMP----------^

  // PRT-OA-NEXT:     {{^ *}}// v----------OMP----------v
  // PRT-O-NEXT:      {{^ *}}{
  // PRT-O-NEXT:      {{^ *}}  const int __clang_acc_num_workers__ = (x = 2);
  // PRT-O-NEXT:      {{^ *}}  #pragma omp target teams
  // PRT-O-NEXT:      {{^ *}}  #pragma omp parallel for num_threads(__clang_acc_num_workers__)
  // PRT-O-NEXT:      {{^ *}}  for (int i = 0; i < 2; ++i)
  // PRT-O-NEXT:      {{^ *}}    for (int j = 0; j < 2; ++j)
  // PRT-O-NEXT:      {{^ *}}      printf("i=%d, j=%d\n", i, j);
  // PRT-O-NEXT:      {{^ *}}}
  // PRT-OA-NEXT:     {{^ *}}// ---------OMP<-ACC--------
  // PRT-OA-NEXT:     {{^ *}}// #pragma acc parallel num_workers((x = 2))
  // PRT-OA-NEXT:     {{^ *}}// #pragma acc loop worker
  // PRT-OA-NEXT:     {{^ *}}// for (int i = 0; i < 2; ++i)
  // PRT-OA-SRC-NEXT: {{^ *}}//   #pragma omp simd
  // PRT-OA-NEXT:     {{^ *}}//   for (int j = 0; j < 2; ++j)
  // PRT-OA-NEXT:     {{^ *}}//     printf("i=%d, j=%d\n", i, j);
  // PRT-OA-NEXT:     {{^ *}}// ^----------ACC----------^
  //
  // EXE-DAG: i=0, j=0
  // EXE-DAG: i=0, j=1
  // EXE-DAG: i=1, j=0
  // EXE-DAG: i=1, j=1
  int x = 0;
  #pragma acc parallel num_workers((x = 2))
  #pragma acc loop worker
  for (int i = 0; i < 2; ++i)
    #pragma omp simd
    for (int j = 0; j < 2; ++j)
      printf("i=%d, j=%d\n", i, j);

  // EXE-NOT: {{.}}
  return 0;
}
