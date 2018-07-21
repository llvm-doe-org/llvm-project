// Check ASTDumper.
//
// RUN: %clang -Xclang -verify -Xclang -ast-dump -fsyntax-only -fopenacc %s \
// RUN: | FileCheck -check-prefix DMP %s

// Check -ast-print and -fopenacc-print.
//
// RUN: %clang -Xclang -verify -Xclang -ast-print -fsyntax-only %s \
// RUN: | FileCheck -check-prefix PRT-NOACC %s
//
// RUN: %data {
// RUN:   (print='-Xclang -ast-print -fsyntax-only -fopenacc' prt=PRT-A,PRT)
// RUN:   (print=-fopenacc-print=acc                          prt=PRT-A,PRT)
// RUN:   (print=-fopenacc-print=omp                          prt=PRT-O,PRT)
// RUN:   (print=-fopenacc-print=acc-omp                      prt=PRT-A,PRT-AO,PRT)
// RUN:   (print=-fopenacc-print=omp-acc                      prt=PRT-O,PRT-OA,PRT)
// RUN: }
// RUN: %for {
// RUN:   %clang -Xclang -verify %[print] %s \
// RUN:   | FileCheck -check-prefixes=%[prt] %s
// RUN: }

// Check ASTWriterStmt, ASTReaderStmt, StmtPrinter, and
// ACCLoopDirective::CreateEmpty (used by ASTReaderStmt).  Some data related to
// printing (where to print comments about discarded directives) is serialized
// and deserialized, so it's worthwhile to try all OpenACC printing modes.
//
// RUN: %for {
// RUN:   %clang -Xclang -verify -fopenacc -emit-ast %s -o %t.ast
// RUN:   %clang %[print] %t.ast 2>&1 \
// RUN:   | FileCheck -check-prefixes=%[prt] %s
// RUN: }

// Can we -ast-print the OpenMP source code, compile, and run it successfully?
//
// RUN: %clang -Xclang -verify -fopenacc-print=omp %s > %t-omp.c
// RUN: echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN: %clang -Xclang -verify -fopenmp -o %t %t-omp.c
// RUN: %t 2 2>&1 | FileCheck -check-prefix EXE %s

// Check execution with normal compilation.
//
// RUN: %clang -Xclang -verify -fopenacc %s -o %t
// RUN: %t 2 2>&1 | FileCheck -check-prefix EXE %s

// END.

// expected-no-diagnostics

#include <stdio.h>
#include <stdlib.h>

// PRT-NOACC: int main
// PRT:       int main
int main(int argc, char *argv[]) {
  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNum_gangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(4){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(4){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(4){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(4){{$}}
  #pragma acc parallel num_gangs(/*literal*/4)
  // DMP: CallExpr
  //
  // PRT-NOACC-NEXT: printf
  // PRT-NEXT:       printf
  //
  // EXE:      hello world
  // EXE-NEXT: hello world
  // EXE-NEXT: hello world
  // EXE-NEXT: hello world
  printf("hello world\n");
  // DMP:      ACCParallelDirective
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams{{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel{{$}}
  #pragma acc parallel
  // DMP: CallExpr
  //
  // PRT-NOACC-NEXT: printf
  // PRT-NEXT:       printf
  //
  // EXE-NEXT: crazy world
  printf("crazy world\n");
  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNum_gangsClause
  // DMP-NEXT:     CallExpr {{.*}} 'int'
  // DMP:        impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       ImplicitCastExpr
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(atoi(argv[1])){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(atoi(argv[1])){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(atoi(argv[1])){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(atoi(argv[1])){{$}}
  #pragma acc parallel num_gangs(/*expr with var*/atoi(argv[1]))
  // DMP: CallExpr
  //
  // PRT-NOACC-NEXT: printf
  // PRT-NEXT:       printf
  // EXE-NEXT: goodbye world
  // EXE-NEXT: goodbye world
  printf("goodbye world\n");
  // PRT-NOACC-NEXT: return 0;
  // PRT-NEXT:       return 0;
  return 0;
  // EXE-NOT:  {{.}}
}
