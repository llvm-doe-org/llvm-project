// Check gang-redundant mode for "acc parallel".
//
// When ADD_LOOP_TO_PAR is not set, this file checks gang-redundant mode for
// "acc parallel" without "loop".
//
// When ADD_LOOP_TO_PAR is set, it adds "loop" and a for loop to those "acc
// parallel" directives in order to check gang-redundant mode for combined "acc
// parallel loop" directives.
//
// RUN: %data directives {
// RUN:   (dir=PAR     dir-cflags=                 )
// RUN:   (dir=PARLOOP dir-cflags=-DADD_LOOP_TO_PAR)
// RUN: }

// Check ASTDumper.
//
// RUN: %for directives {
// RUN:   %clang -Xclang -verify -Xclang -ast-dump -fsyntax-only -fopenacc %s \
// RUN:          %[dir-cflags] \
// RUN:   | FileCheck -check-prefixes=DMP,DMP-%[dir] %s
// RUN: }

// Check -ast-print and -fopenacc-print.
//
// RUN: %for directives {
// RUN:   %clang -Xclang -verify -Xclang -ast-print -fsyntax-only %s \
// RUN:          %[dir-cflags] \
// RUN:   | FileCheck -check-prefixes=PRT,PRT-%[dir] %s
// RUN: }
//
// RUN: %data prints {
// RUN:   (print='-Xclang -ast-print -fsyntax-only -fopenacc'
// RUN:    prt=PRT,PRT-%[dir],PRT-A,PRT-A-%[dir])
// RUN:   (print=-fopenacc-print=acc
// RUN:    prt=PRT,PRT-%[dir],PRT-A,PRT-A-%[dir])
// RUN:   (print=-fopenacc-print=omp
// RUN:    prt=PRT,PRT-%[dir],PRT-O,PRT-O-%[dir])
// RUN:   (print=-fopenacc-print=acc-omp
// RUN:    prt=PRT,PRT-%[dir],PRT-A,PRT-A-%[dir],PRT-AO,PRT-AO-%[dir])
// RUN:   (print=-fopenacc-print=omp-acc
// RUN:    prt=PRT,PRT-%[dir],PRT-O,PRT-O-%[dir],PRT-OA,PRT-OA-%[dir])
// RUN: }
// RUN: %for directives {
// RUN:   %for prints {
// RUN:     %clang -Xclang -verify %[print] %[dir-cflags] %s \
// RUN:     | FileCheck -check-prefixes=%[prt] %s
// RUN:   }
// RUN: }

// Check ASTWriterStmt, ASTReaderStmt, StmtPrinter, and
// ACCLoopDirective::CreateEmpty (used by ASTReaderStmt).  Some data related to
// printing (where to print comments about discarded directives) is serialized
// and deserialized, so it's worthwhile to try all OpenACC printing modes.
//
// RUN: %for directives {
// RUN:   %for prints {
// RUN:     %clang -Xclang -verify -fopenacc -emit-ast %[dir-cflags] %s \
// RUN:            -o %t.ast
// RUN:     %clang %[print] %t.ast 2>&1 \
// RUN:     | FileCheck -check-prefixes=%[prt] %s
// RUN:   }
// RUN: }

// Can we -ast-print the OpenMP source code, compile, and run it successfully?
//
// RUN: %for directives {
// RUN:   %clang -Xclang -verify -fopenacc-print=omp %[dir-cflags] %s \
// RUN:          > %t-omp.c
// RUN:   echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN:   %clang -Xclang -verify -fopenmp -o %t %t-omp.c
// RUN:   %t 2 2>&1 | FileCheck -check-prefixes=EXE,EXE-%[dir] %s
// RUN: }

// Check execution with normal compilation.
//
// RUN: %for directives {
// RUN:   %clang -Xclang -verify -fopenacc %[dir-cflags] %s -o %t
// RUN:   %t 2 2>&1 | FileCheck -check-prefixes=EXE,EXE-%[dir] %s
// RUN: }

// END.

// expected-no-diagnostics

#include <stdio.h>
#include <stdlib.h>

#if !ADD_LOOP_TO_PAR
# define LOOP
# define FORLOOP_HEAD
#else
# define LOOP loop
# define FORLOOP_HEAD for (int i = 0; i < 2; ++i)
#endif

// PRT: int main(int argc, char *argv[]) {
int main(int argc, char *argv[]) {
  // PRT-NEXT: printf("start\n");
  // EXE: start
  printf("start\n");

  // DMP-PAR:          ACCParallelDirective
  // DMP-PARLOOP:      ACCParallelLoopDirective
  // DMP-NEXT:           ACCNum_gangsClause
  // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 4
  // DMP-PARLOOP-NEXT:   effect: ACCParallelDirective
  // DMP-PARLOOP-NEXT:     ACCNum_gangsClause
  // DMP-PARLOOP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:             impl: OMPTargetTeamsDirective
  // DMP-NEXT:               OMPNum_teamsClause
  // DMP-NEXT:                 IntegerLiteral {{.*}} 'int' 4
  // DMP-PARLOOP:          ACCLoopDirective
  // DMP-PARLOOP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  //
  // PRT-A-PAR-NEXT:      {{^ *}}#pragma acc parallel num_gangs(4){{$}}
  // PRT-A-PARLOOP-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(4){{$}}
  // PRT-AO-NEXT:         {{^ *}}// #pragma omp target teams num_teams(4){{$}}
  // PRT-O-NEXT:          {{^ *}}#pragma omp target teams num_teams(4){{$}}
  // PRT-OA-PAR-NEXT:     {{^ *}}// #pragma acc parallel num_gangs(4){{$}}
  // PRT-OA-PARLOOP-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(4){{$}}
  #pragma acc parallel LOOP num_gangs(/*literal*/4)
  // DMP-PAR-NOT:      ForStmt
  // DMP-PARLOOP-NEXT: impl: ForStmt
  // PRT-PARLOOP: for ({{.*}})
  FORLOOP_HEAD
    // DMP: CallExpr
    // PRT-NEXT: printf("hello world\n");
    //
    // EXE-NEXT: hello world
    // EXE-NEXT: hello world
    // EXE-NEXT: hello world
    // EXE-NEXT: hello world
    // EXE-PARLOOP-NEXT: hello world
    // EXE-PARLOOP-NEXT: hello world
    // EXE-PARLOOP-NEXT: hello world
    // EXE-PARLOOP-NEXT: hello world
    printf("hello world\n");

  // DMP-PAR:          ACCParallelDirective
  // DMP-PARLOOP:      ACCParallelLoopDirective
  // DMP-PARLOOP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:             impl: OMPTargetTeamsDirective
  // DMP-PARLOOP:          ACCLoopDirective
  // DMP-PARLOOP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  //
  // PRT-A-PAR-NEXT:      {{^ *}}#pragma acc parallel{{$}}
  // PRT-A-PARLOOP-NEXT:  {{^ *}}#pragma acc parallel loop{{$}}
  // PRT-AO-NEXT:         {{^ *}}// #pragma omp target teams{{$}}
  // PRT-O-NEXT:          {{^ *}}#pragma omp target teams{{$}}
  // PRT-OA-PAR-NEXT:     {{^ *}}// #pragma acc parallel{{$}}
  // PRT-OA-PARLOOP-NEXT: {{^ *}}// #pragma acc parallel loop{{$}}
  #pragma acc parallel LOOP
  // DMP-PAR-NOT:      ForStmt
  // DMP-PARLOOP-NEXT: impl: ForStmt
  // PRT-PARLOOP: for ({{.*}})
  FORLOOP_HEAD
    // DMP: CallExpr
    // PRT-NEXT: printf("crazy world\n");
    // EXE-NEXT:         crazy world
    // EXE-PARLOOP-NEXT: crazy world
    printf("crazy world\n");

  // DMP-PAR:          ACCParallelDirective
  // DMP-PARLOOP:      ACCParallelLoopDirective
  // DMP-NEXT:           ACCNum_gangsClause
  // DMP-NEXT:             CallExpr {{.*}} 'int'
  // DMP-PARLOOP:        effect: ACCParallelDirective
  // DMP-PARLOOP-NEXT:     ACCNum_gangsClause
  // DMP-PARLOOP-NEXT:       CallExpr {{.*}} 'int'
  // DMP:                  impl: OMPTargetTeamsDirective
  // DMP-NEXT:               OMPNum_teamsClause
  // DMP-NEXT:                 ImplicitCastExpr
  // DMP-PARLOOP:           ACCLoopDirective
  // DMP-PARLOOP-NEXT:        ACCIndependentClause {{.*}} <implicit>
  //
  // PRT-A-PAR-NEXT:      {{^ *}}#pragma acc parallel num_gangs(atoi(argv[1])){{$}}
  // PRT-A-PARLOOP-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(atoi(argv[1])){{$}}
  // PRT-AO-NEXT:         {{^ *}}// #pragma omp target teams num_teams(atoi(argv[1])){{$}}
  // PRT-O-NEXT:          {{^ *}}#pragma omp target teams num_teams(atoi(argv[1])){{$}}
  // PRT-OA-PAR-NEXT:     {{^ *}}// #pragma acc parallel num_gangs(atoi(argv[1])){{$}}
  // PRT-OA-PARLOOP-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(atoi(argv[1])){{$}}
  #pragma acc parallel LOOP num_gangs(/*expr with var*/atoi(argv[1]))
  // DMP-PAR-NOT:      ForStmt
  // DMP-PARLOOP-NEXT: impl: ForStmt
  // PRT-PARLOOP:      for ({{.*}})
  FORLOOP_HEAD
    // DMP: CallExpr
    // PRT-NEXT: printf("goodbye world\n");
    // EXE-NEXT:         goodbye world
    // EXE-NEXT:         goodbye world
    // EXE-PARLOOP-NEXT: goodbye world
    // EXE-PARLOOP-NEXT: goodbye world
    printf("goodbye world\n");

  // PRT-NEXT: return 0;
  return 0;
} // PRT-NEXT: }
// EXE-NOT:  {{.}}
