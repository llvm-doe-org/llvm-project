// Check collapse clause on "acc loop" and on "acc parallel loop".

// Check -ast-dump before and after AST serialization.
//
// RUN: %clang -Xclang -verify -Xclang -ast-dump -fsyntax-only -fopenacc %s \
// RUN: | FileCheck -check-prefix=DMP %s
// RUN: %clang -Xclang -verify -fopenacc -emit-ast -o %t.ast %s
// RUN: %clang_cc1 -ast-dump-all %t.ast \
// RUN: | FileCheck -check-prefixes=DMP %s

// Check -ast-print and -fopenacc[-ast]-print.
//
// RUN: %clang -Xclang -verify -Xclang -ast-print -fsyntax-only %s \
// RUN: | FileCheck -check-prefixes=PRT,PRT-NOACC %s
//
// Strip comments and blank lines so checking -fopenacc-print output is easier.
// RUN: echo "// expected""-no-diagnostics" > %t-acc.c
// RUN: grep -v '^ *\(//.*\)\?$' %s | sed 's,//.*,,' >> %t-acc.c
//
// TODO: If lit were to support %for inside a %data, we could iterate prt-opts
// within prt-args after the first prt-args iteration, significantly shortening
// the prt-args definition.
//
// RUN: %data prt-opts {
// RUN:   (prt-opt=-fopenacc-ast-print)
// RUN:   (prt-opt=-fopenacc-print    )
// RUN: }
// RUN: %data prt-args {
// RUN:   (prt='-Xclang -ast-print -fsyntax-only -fopenacc' prt-chk=PRT-A,PRT)
// RUN:   (prt=-fopenacc-ast-print=acc                      prt-chk=PRT-A,PRT)
// RUN:   (prt=-fopenacc-ast-print=omp                      prt-chk=PRT-O,PRT)
// RUN:   (prt=-fopenacc-ast-print=acc-omp                  prt-chk=PRT-A,PRT-AO,PRT)
// RUN:   (prt=-fopenacc-ast-print=omp-acc                  prt-chk=PRT-O,PRT-OA,PRT)
// RUN:   (prt=-fopenacc-print=acc                          prt-chk=PRT-A,PRT)
// RUN:   (prt=-fopenacc-print=omp                          prt-chk=PRT-O,PRT)
// RUN:   (prt=-fopenacc-print=acc-omp                      prt-chk=PRT-A,PRT-AO,PRT)
// RUN:   (prt=-fopenacc-print=omp-acc                      prt-chk=PRT-O,PRT-OA,PRT)
// RUN: }
// RUN: %for prt-args {
// RUN:   %clang -Xclang -verify %[prt] %t-acc.c \
// RUN:   | FileCheck -check-prefixes=%[prt-chk] %s
// RUN: }

// Check -ast-print after AST serialization.
//
// Some data related to printing (where to print comments about discarded
// directives) is serialized and deserialized, so it's worthwhile to try all
// OpenACC printing modes.
//
// RUN: %clang -Xclang -verify -fopenacc -emit-ast %t-acc.c -o %t.ast
// RUN: %for prt-args {
// RUN:   %clang %[prt] %t.ast 2>&1 \
// RUN:   | FileCheck -check-prefixes=%[prt-chk] %s
// RUN: }

// Can we print the OpenMP source code, compile, and run it successfully?
//
// RUN: %for prt-opts {
// RUN:   %clang -Xclang -verify %[prt-opt]=omp %s > %t-omp.c
// RUN:   echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN:   %clang -Xclang -verify -fopenmp %fopenmp-version -o %t %t-omp.c
// RUN:   %t 2 2>&1 | FileCheck -check-prefix=EXE -match-full-lines %s
// RUN: }
//
// Check execution with normal compilation.
//
// RUN: %data tgts {
// RUN:   (run-if=                tgt-cflags=                                    )
// RUN:   (run-if=%run-if-x86_64  tgt-cflags=-fopenmp-targets=%run-x86_64-triple )
// RUN:   (run-if=%run-if-ppc64le tgt-cflags=-fopenmp-targets=%run-ppc64le-triple)
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags=-fopenmp-targets=%run-nvptx64-triple)
// RUN: }
// RUN: %for tgts {
// RUN:   %[run-if] %clang -Xclang -verify -fopenacc %s -o %t %[tgt-cflags]
// RUN:   %[run-if] %t 2 > %t.out 2>&1
// RUN:   %[run-if] FileCheck -input-file %t.out %s -match-full-lines \
// RUN:                       -check-prefixes=EXE
// RUN: }

// END.

// expected-no-diagnostics

#include <stdio.h>

// PRT: int main() {
int main() {
  //--------------------------------------------------
  // Explicit seq.
  //--------------------------------------------------

  // DMP: CallExpr
  // PRT-NEXT: printf
  // EXE-LABEL: loop seq
  printf("loop seq\n");

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1) num_workers(4){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) num_workers(4){{$}}
  #pragma acc parallel num_gangs(1) num_workers(4)
  // DMP:      ACCLoopDirective
  // DMP-NEXT:   ACCSeqClause
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   impl: ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc loop seq collapse(2)
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  // PRT-A-SAME:  {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq collapse(2) // discarded in OpenMP translation{{$}}
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc loop seq collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-NEXT: 0, 0
      // EXE-NEXT: 0, 1
      // EXE-NEXT: 1, 0
      // EXE-NEXT: 1, 1
      printf("%d, %d\n", i, j);

  // DMP: CallExpr
  // PRT-NEXT: printf
  // EXE-LABEL: parallel loop seq
  printf("parallel loop seq\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCSeqClause
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCSeqClause
  // DMP-NEXT:       ACCCollapseClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 2
  // DMP-NEXT:       impl: ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(1) vector_length(4) seq collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(1) vector_length(4) seq collapse(2){{$}}
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc parallel loop num_gangs(1) vector_length(4) seq collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-NEXT: 0, 0
      // EXE-NEXT: 0, 1
      // EXE-NEXT: 1, 0
      // EXE-NEXT: 1, 1
      printf("%d, %d\n", i, j);

  //--------------------------------------------------
  // Implicit independent, implicit gang partitioning.
  //--------------------------------------------------

  // DMP: CallExpr
  // PRT-NEXT: printf
  // EXE-LABEL: loop
  printf("loop\n");

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1) num_workers(4){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) num_workers(4){{$}}
  #pragma acc parallel num_gangs(1) num_workers(4)
  // DMP:      ACCLoopDirective
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 2
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc loop collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop collapse(2){{$}}
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc loop collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-NEXT: 0, 0
      // EXE-NEXT: 0, 1
      // EXE-NEXT: 1, 0
      // EXE-NEXT: 1, 1
      printf("%d, %d\n", i, j);

  // DMP: CallExpr
  // PRT-NEXT: printf
  // EXE-LABEL: parallel loop
  printf("parallel loop\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCCollapseClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 2
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeDirective
  // DMP-NEXT:         OMPCollapseClause
  // DMP-NEXT:           ConstantExpr {{.*}} 'int'
  // DMP-NEXT:             value: Int 2
  // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 2
  // DMP:              ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(1) vector_length(4) collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(1) vector_length(4) collapse(2){{$}}
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc parallel loop num_gangs(1) vector_length(4) collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-NEXT: 0, 0
      // EXE-NEXT: 0, 1
      // EXE-NEXT: 1, 0
      // EXE-NEXT: 1, 1
      printf("%d, %d\n", i, j);

  //--------------------------------------------------
  // Explicit independent, implicit gang partitioning.
  //--------------------------------------------------

  // DMP: CallExpr
  // PRT-NEXT: printf
  // EXE-LABEL: loop independent
  printf("loop independent\n");

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1) num_workers(4){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) num_workers(4){{$}}
  #pragma acc parallel num_gangs(1) num_workers(4)
  // DMP:      ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause
  // DMP-NOT:      <implicit>
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 2
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc loop independent collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop independent collapse(2){{$}}
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc loop independent collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-NEXT: 0, 0
      // EXE-NEXT: 0, 1
      // EXE-NEXT: 1, 0
      // EXE-NEXT: 1, 1
      printf("%d, %d\n", i, j);

  // DMP: CallExpr
  // PRT-NEXT: printf
  // EXE-LABEL: parallel loop independent
  printf("parallel loop independent\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCIndependentClause
  // DMP-NOT:      <implicit>
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCIndependentClause
  // DMP-NOT:          <implicit>
  // DMP-NEXT:       ACCCollapseClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 2
  // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeDirective
  // DMP-NEXT:         OMPCollapseClause
  // DMP-NEXT:           ConstantExpr {{.*}} 'int'
  // DMP-NEXT:             value: Int 2
  // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 2
  // DMP:              ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(1) vector_length(4) independent collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(1) vector_length(4) independent collapse(2){{$}}
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc parallel loop num_gangs(1) vector_length(4) independent collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-NEXT: 0, 0
      // EXE-NEXT: 0, 1
      // EXE-NEXT: 1, 0
      // EXE-NEXT: 1, 1
      printf("%d, %d\n", i, j);

  //--------------------------------------------------
  // Explicit auto with partitioning.
  //--------------------------------------------------

  // DMP: CallExpr
  // PRT-NEXT: printf
  // EXE-LABEL: loop auto
  printf("loop auto\n");

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1) num_workers(4){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) num_workers(4){{$}}
  #pragma acc parallel num_gangs(1) num_workers(4)
  // DMP:      ACCLoopDirective
  // DMP-NEXT:   ACCAutoClause
  // DMP-NEXT:   ACCWorkerClause
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   impl: ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc loop auto worker collapse(2)
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  // PRT-A-SAME:  {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto worker collapse(2) // discarded in OpenMP translation{{$}}
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc loop auto worker collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-NEXT: 0, 0
      // EXE-NEXT: 0, 1
      // EXE-NEXT: 1, 0
      // EXE-NEXT: 1, 1
      printf("%d, %d\n", i, j);

  // DMP: CallExpr
  // PRT-NEXT: printf
  // EXE-LABEL: parallel loop auto
  printf("parallel loop auto\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCAutoClause
  // DMP-NEXT:   ACCVectorClause
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCAutoClause
  // DMP-NEXT:       ACCVectorClause
  // DMP-NEXT:       ACCCollapseClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 2
  // DMP-NEXT:       impl: ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(1) vector_length(4) auto vector collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(1) vector_length(4) auto vector collapse(2){{$}}
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc parallel loop num_gangs(1) vector_length(4) auto vector collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-DAG: 0, 0
      // EXE-DAG: 0, 1
      // EXE-DAG: 1, 0
      // EXE-DAG: 1, 1
      printf("%d, %d\n", i, j);

  //--------------------------------------------------
  // Worker or vector partitioned.
  //--------------------------------------------------

  // DMP: CallExpr
  // PRT-NEXT: printf
  // EXE-LABEL: loop worker
  printf("loop worker\n");

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1) num_workers(4){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) num_workers(4){{$}}
  #pragma acc parallel num_gangs(1) num_workers(4)
  // DMP:      ACCLoopDirective
  // DMP-NEXT:   ACCWorkerClause
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeParallelForDirective
  // DMP-NEXT:     OMPNum_threadsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 4
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 2
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(4) collapse(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute parallel for num_threads(4) collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker collapse(2){{$}}
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc loop worker collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-DAG: 0, 0
      // EXE-DAG: 0, 1
      // EXE-DAG: 1, 0
      // EXE-DAG: 1, 1
      printf("%d, %d\n", i, j);

  // DMP: CallExpr
  // PRT-NEXT: printf
  // EXE-LABEL: parallel loop vector
  printf("parallel loop vector\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorClause
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCVectorClause
  // DMP-NEXT:       ACCCollapseClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 2
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeSimdDirective
  // DMP-NEXT:         OMPSimdlenClause
  // DMP-NEXT:           ConstantExpr {{.*}} 'int'
  // DMP-NEXT:             value: Int 4
  // DMP-NEXT:             IntegerLiteral {{.*}} 4
  // DMP-NEXT:         OMPCollapseClause
  // DMP-NEXT:           ConstantExpr {{.*}} 'int'
  // DMP-NEXT:             value: Int 2
  // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 2
  // DMP:              ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(1) vector_length(4) vector collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd simdlen(4) collapse(2){{$}}
  //
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute simd simdlen(4) collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(1) vector_length(4) vector collapse(2){{$}}
  //
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc parallel loop num_gangs(1) vector_length(4) vector collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-DAG: 0, 0
      // EXE-DAG: 0, 1
      // EXE-DAG: 1, 0
      // EXE-DAG: 1, 1
      printf("%d, %d\n", i, j);

  //--------------------------------------------------
  // Explicit gang partitioned.
  //--------------------------------------------------

  // DMP: CallExpr
  // PRT-NEXT: printf
  // EXE-LABEL: loop gang
  printf("loop gang\n");

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(4){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(4){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(4){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(4){{$}}
  #pragma acc parallel num_gangs(4)
  // DMP:      ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  // DMP-NOT:    <implicit>
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 2
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang collapse(2){{$}}
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc loop gang collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-DAG: 0, 0
      // EXE-DAG: 0, 1
      // EXE-DAG: 1, 0
      // EXE-DAG: 1, 1
      printf("%d, %d\n", i, j);

  // DMP: CallExpr
  // PRT-NEXT: printf
  // EXE-LABEL: parallel loop gang
  printf("parallel loop gang\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   ACCGangClause
  // DMP-NOT:    <implicit>
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 3
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 8
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCGangClause
  // DMP-NOT:        <implicit>
  // DMP-NEXT:       ACCCollapseClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 3
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeDirective
  // DMP-NEXT:         OMPCollapseClause
  // DMP-NEXT:           ConstantExpr {{.*}} 'int'
  // DMP-NEXT:             value: Int 3
  // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 3
  // DMP:              ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(8) gang collapse(3){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(8){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(3){{$}}
  //
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(8){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute collapse(3){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(8) gang collapse(3){{$}}
  //
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc parallel loop num_gangs(8) gang collapse(3)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: ForStmt
      // PRT-NEXT: for (int k ={{.*}})
      for (int k = 0; k < 2; ++k)
        // DMP: CallExpr
        // PRT-NEXT: printf
        // EXE-DAG: 0, 0, 0
        // EXE-DAG: 0, 0, 1
        // EXE-DAG: 0, 1, 0
        // EXE-DAG: 0, 1, 1
        // EXE-DAG: 1, 0, 0
        // EXE-DAG: 1, 0, 1
        // EXE-DAG: 1, 1, 0
        // EXE-DAG: 1, 1, 1
        printf("%d, %d, %d\n", i, j, k);

  //--------------------------------------------------
  // More loops than necessary.
  //--------------------------------------------------

  // DMP: CallExpr
  // PRT-NEXT: printf
  // EXE-LABEL: loop 1 {1}
  printf("loop 1 {1}\n");

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 16
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 16
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(16){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(16){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(16){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(16){{$}}
  #pragma acc parallel num_gangs(16)
  // DMP:      ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  // DMP-NOT:    <implicit>
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 1
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 1
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
  // DMP:          ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang collapse(1){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(1){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute collapse(1){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang collapse(1){{$}}
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc loop gang collapse(1)
  for (int i = 0; i < 4; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 4; ++j)
      // DMP: CallExpr
      // PRT-NEXT: printf
      // For any i, values of j must print in ascending order.
      // EXE: 0, 0
      // EXE: 0, 1
      // EXE: 0, 2
      // EXE: 0, 3
      printf("%d, %d\n", i, j);

  // DMP: CallExpr
  // PRT-NEXT: printf
  // EXE-LABEL: loop 2 {1}
  printf("loop 2 {1}\n");

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(8){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(8){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(8){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(8){{$}}
  #pragma acc parallel num_gangs(8)
  // DMP:      ACCLoopDirective
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 2
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc loop collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop collapse(2){{$}}
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc loop collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: ForStmt
      // PRT-NEXT: for (int k ={{.*}})
      for (int k = 0; k < 2; ++k)
        // DMP: CallExpr
        // PRT-NEXT: printf
        // For any pair (i,j), values of k must print in ascending order.
        // EXE: 1, 0, 0
        // EXE: 1, 0, 1
        printf("%d, %d, %d\n", i, j, k);

  // DMP: CallExpr
  // PRT-NEXT: printf
  // EXE-LABEL: parallel loop 2 {2}
  printf("parallel loop 2 {2}\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 16
  // DMP-NEXT:   ACCGangClause
  // DMP-NOT:    <implicit>
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 16
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 16
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCGangClause
  // DMP-NOT:        <implicit>
  // DMP-NEXT:       ACCCollapseClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 2
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeDirective
  // DMP-NEXT:         OMPCollapseClause
  // DMP-NEXT:           ConstantExpr {{.*}} 'int'
  // DMP-NEXT:             value: Int 2
  // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 2
  // DMP:              ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(16) gang collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(16){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  //
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(16){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(16) gang collapse(2){{$}}
  //
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc parallel loop num_gangs(16) gang collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP: ForStmt
      // PRT-NEXT: for (int k ={{.*}})
      for (int k = 0; k < 2; ++k)
        // DMP: ForStmt
        // PRT-NEXT: for (int l ={{.*}})
        for (int l = 0; l < 2; ++l)
          // DMP: CallExpr
          // PRT-NEXT: printf
          // For any pair (i,j), values of (k,l) must print in ascending order.
          // EXE: 0, 1, 0, 0
          // EXE: 0, 1, 0, 1
          // EXE: 0, 1, 1, 0
          // EXE: 0, 1, 1, 1
          printf("%d, %d, %d, %d\n", i, j, k, l);

  //--------------------------------------------------
  // Nested collapse.
  //--------------------------------------------------

  // DMP: CallExpr
  // PRT-NEXT: printf
  // EXE-LABEL: loop 2 {loop 2}
  printf("loop 2 {loop 2}\n");

  // DMP:      ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(4) num_workers(4){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(4){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(4){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(4) num_workers(4){{$}}
  #pragma acc parallel num_gangs(4) num_workers(4)
  // DMP:      ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  // DMP-NOT:    <implicit>
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPCollapseClause
  // DMP-NEXT:       ConstantExpr {{.*}} 'int'
  // DMP-NEXT:         value: Int 2
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP:          ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang collapse(2){{$}}
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc loop gang collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCCollapseClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 2
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
      // DMP-NEXT:   impl: OMPParallelForDirective
      // DMP-NEXT:     OMPNum_threadsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 4
      // DMP-NEXT:     OMPCollapseClause
      // DMP-NEXT:       ConstantExpr {{.*}} 'int'
      // DMP-NEXT:         value: Int 2
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
      // DMP-NEXT:     OMPSharedClause {{.*}}
      // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
      // DMP:          ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker collapse(2){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for num_threads(4) collapse(2) shared(i,j){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for num_threads(4) collapse(2) shared(i,j){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker collapse(2){{$}}
      // PRT-NEXT:    for (int k ={{.*}})
      #pragma acc loop worker collapse(2)
      for (int k = 0; k < 2; ++k)
        // DMP: ForStmt
        // PRT-NEXT: for (int l ={{.*}})
        for (int l = 0; l < 2; ++l)
          // DMP: CallExpr
          // PRT-NEXT: printf
          // EXE-DAG: 0, 0, 0, 0
          // EXE-DAG: 0, 0, 0, 1
          // EXE-DAG: 0, 0, 1, 0
          // EXE-DAG: 0, 0, 1, 1
          // EXE-DAG: 0, 1, 0, 0
          // EXE-DAG: 0, 1, 0, 1
          // EXE-DAG: 0, 1, 1, 0
          // EXE-DAG: 0, 1, 1, 1
          // EXE-DAG: 1, 0, 0, 0
          // EXE-DAG: 1, 0, 0, 1
          // EXE-DAG: 1, 0, 1, 0
          // EXE-DAG: 1, 0, 1, 1
          // EXE-DAG: 1, 1, 0, 0
          // EXE-DAG: 1, 1, 0, 1
          // EXE-DAG: 1, 1, 1, 0
          // EXE-DAG: 1, 1, 1, 1
          printf("%d, %d, %d, %d\n", i, j, k, l);

  // DMP: CallExpr
  // PRT-NEXT: printf
  // EXE-LABEL: parallel loop 2 {loop 2 {loop 2}}
  printf("parallel loop 2 {loop 2 {loop 2}}\n");

  // DMP:      ACCParallelLoopDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCNumWorkersClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCVectorLengthClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:   ACCCollapseClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 2
  // DMP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:     ACCNumGangsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     ACCNumWorkersClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     ACCVectorLengthClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:     impl: OMPTargetTeamsDirective
  // DMP-NEXT:       OMPNum_teamsClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 4
  // DMP:          ACCLoopDirective
  // DMP-NEXT:       ACCCollapseClause
  // DMP-NEXT:         IntegerLiteral {{.*}} 2
  // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  // DMP-NEXT:       impl: OMPDistributeDirective
  // DMP-NEXT:         OMPCollapseClause
  // DMP-NEXT:           ConstantExpr {{.*}} 'int'
  // DMP-NEXT:             value: Int 2
  // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 2
  // DMP:              ForStmt
  //
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(4) num_workers(4) vector_length(4) collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(4){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  //
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(4){{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp distribute collapse(2){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(4) num_workers(4) vector_length(4) collapse(2){{$}}
  //
  // PRT-NEXT:    for (int i ={{.*}})
  #pragma acc parallel loop num_gangs(4) num_workers(4) vector_length(4) collapse(2)
  for (int i = 0; i < 2; ++i)
    // DMP: ForStmt
    // PRT-NEXT: for (int j ={{.*}})
    for (int j = 0; j < 2; ++j)
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCCollapseClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 2
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
      // DMP-NEXT:   impl: OMPParallelForDirective
      // DMP-NEXT:     OMPNum_threadsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 4
      // DMP-NEXT:     OMPCollapseClause
      // DMP-NEXT:       ConstantExpr {{.*}} 'int'
      // DMP-NEXT:         value: Int 2
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
      // DMP-NEXT:     OMPSharedClause {{.*}}
      // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
      // DMP:          ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker collapse(2){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for num_threads(4) collapse(2) shared(i,j){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for num_threads(4) collapse(2) shared(i,j){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker collapse(2){{$}}
      // PRT-NEXT:    for (int k ={{.*}})
      #pragma acc loop worker collapse(2)
      for (int k = 0; k < 2; ++k)
        // DMP: ForStmt
        // PRT-NEXT: for (int l ={{.*}})
        for (int l = 0; l < 2; ++l)
          // DMP:      ACCLoopDirective
          // DMP-NEXT:   ACCVectorClause
          // DMP-NEXT:   ACCCollapseClause
          // DMP-NEXT:     IntegerLiteral {{.*}} 2
          // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
          // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
          // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
          // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
          // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
          // DMP-NEXT:     DeclRefExpr {{.*}} 'l' 'int'
          // DMP-NEXT:   impl: OMPSimdDirective
          // DMP-NEXT:     OMPSimdlenClause
          // DMP-NEXT:       ConstantExpr {{.*}} 'int'
          // DMP-NEXT:         value: Int 4
          // DMP-NEXT:         IntegerLiteral {{.*}} 4
          // DMP-NEXT:     OMPCollapseClause
          // DMP-NEXT:       ConstantExpr {{.*}} 'int'
          // DMP-NEXT:         value: Int 2
          // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
          // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
          // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
          // DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
          // DMP-NEXT:       DeclRefExpr {{.*}} 'k' 'int'
          // DMP-NEXT:       DeclRefExpr {{.*}} 'l' 'int'
          // DMP:          ForStmt
          //
          // PRT-A-NEXT:  {{^ *}}#pragma acc loop vector collapse(2){{$}}
          // PRT-AO-NEXT: {{^ *}}// #pragma omp simd simdlen(4) collapse(2){{$}}
          // PRT-O-NEXT:  {{^ *}}#pragma omp simd simdlen(4) collapse(2){{$}}
          // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector collapse(2){{$}}
          // PRT-NEXT:    for (int m ={{.*}})
          #pragma acc loop vector collapse(2)
          for (int m = 0; m < 2; ++m)
            // DMP: ForStmt
            // PRT-NEXT: for (int n ={{.*}})
            for (int n = 0; n < 2; ++n)
              // DMP: CallExpr
              // PRT-NEXT: printf({{.*}}
              // EXE-DAG: 0, 0, 0, 0, 0, 0
              // EXE-DAG: 0, 0, 0, 0, 0, 1
              // EXE-DAG: 0, 0, 0, 0, 1, 0
              // EXE-DAG: 0, 0, 0, 0, 1, 1
              // EXE-DAG: 0, 0, 0, 1, 0, 0
              // EXE-DAG: 0, 0, 0, 1, 0, 1
              // EXE-DAG: 0, 0, 0, 1, 1, 0
              // EXE-DAG: 0, 0, 0, 1, 1, 1
              // EXE-DAG: 0, 0, 1, 0, 0, 0
              // EXE-DAG: 0, 0, 1, 0, 0, 1
              // EXE-DAG: 0, 0, 1, 0, 1, 0
              // EXE-DAG: 0, 0, 1, 0, 1, 1
              // EXE-DAG: 0, 0, 1, 1, 0, 0
              // EXE-DAG: 0, 0, 1, 1, 0, 1
              // EXE-DAG: 0, 0, 1, 1, 1, 0
              // EXE-DAG: 0, 0, 1, 1, 1, 1
              // EXE-DAG: 0, 1, 0, 0, 0, 0
              // EXE-DAG: 0, 1, 0, 0, 0, 1
              // EXE-DAG: 0, 1, 0, 0, 1, 0
              // EXE-DAG: 0, 1, 0, 0, 1, 1
              // EXE-DAG: 0, 1, 0, 1, 0, 0
              // EXE-DAG: 0, 1, 0, 1, 0, 1
              // EXE-DAG: 0, 1, 0, 1, 1, 0
              // EXE-DAG: 0, 1, 0, 1, 1, 1
              // EXE-DAG: 0, 1, 1, 0, 0, 0
              // EXE-DAG: 0, 1, 1, 0, 0, 1
              // EXE-DAG: 0, 1, 1, 0, 1, 0
              // EXE-DAG: 0, 1, 1, 0, 1, 1
              // EXE-DAG: 0, 1, 1, 1, 0, 0
              // EXE-DAG: 0, 1, 1, 1, 0, 1
              // EXE-DAG: 0, 1, 1, 1, 1, 0
              // EXE-DAG: 0, 1, 1, 1, 1, 1
              // EXE-DAG: 1, 0, 0, 0, 0, 0
              // EXE-DAG: 1, 0, 0, 0, 0, 1
              // EXE-DAG: 1, 0, 0, 0, 1, 0
              // EXE-DAG: 1, 0, 0, 0, 1, 1
              // EXE-DAG: 1, 0, 0, 1, 0, 0
              // EXE-DAG: 1, 0, 0, 1, 0, 1
              // EXE-DAG: 1, 0, 0, 1, 1, 0
              // EXE-DAG: 1, 0, 0, 1, 1, 1
              // EXE-DAG: 1, 0, 1, 0, 0, 0
              // EXE-DAG: 1, 0, 1, 0, 0, 1
              // EXE-DAG: 1, 0, 1, 0, 1, 0
              // EXE-DAG: 1, 0, 1, 0, 1, 1
              // EXE-DAG: 1, 0, 1, 1, 0, 0
              // EXE-DAG: 1, 0, 1, 1, 0, 1
              // EXE-DAG: 1, 0, 1, 1, 1, 0
              // EXE-DAG: 1, 0, 1, 1, 1, 1
              // EXE-DAG: 1, 1, 0, 0, 0, 0
              // EXE-DAG: 1, 1, 0, 0, 0, 1
              // EXE-DAG: 1, 1, 0, 0, 1, 0
              // EXE-DAG: 1, 1, 0, 0, 1, 1
              // EXE-DAG: 1, 1, 0, 1, 0, 0
              // EXE-DAG: 1, 1, 0, 1, 0, 1
              // EXE-DAG: 1, 1, 0, 1, 1, 0
              // EXE-DAG: 1, 1, 0, 1, 1, 1
              // EXE-DAG: 1, 1, 1, 0, 0, 0
              // EXE-DAG: 1, 1, 1, 0, 0, 1
              // EXE-DAG: 1, 1, 1, 0, 1, 0
              // EXE-DAG: 1, 1, 1, 0, 1, 1
              // EXE-DAG: 1, 1, 1, 1, 0, 0
              // EXE-DAG: 1, 1, 1, 1, 0, 1
              // EXE-DAG: 1, 1, 1, 1, 1, 0
              // EXE-DAG: 1, 1, 1, 1, 1, 1
              printf("%d, %d, %d, %d, %d, %d\n", i, j, k, l, m, n);

  //--------------------------------------------------
  // Private for loop control variable that is assigned not declared in init
  // of for loop that is attached via collapse.
  //
  // The first case checks gang partitioning only.
  //
  // The second case checks worker partitioning only (plus implicit gang
  // partitioning).
  //
  // The third case checks (1) a combined directive, (2) vector partitioning
  // only (plus implicit gang partitioning) (3) that privacy of a simd loop
  // control variable is handled via a compound statement and local declaration
  // instead of a private clause, and (4) that a declared loop control variable
  // doesn't throw off the count and cause nested but not associated loop
  // control variables to be private.
  //--------------------------------------------------

  // DMP: CallExpr
  // PRT-NEXT: printf
  // EXE-LABEL: loop gang private control var
  printf("loop gang private control var\n");

  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    // PRT: int j = 99;
    // PRT: int k = 99;
    int i = 99;
    int j = 99;
    int k = 99;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(8){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(8) firstprivate(k){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(8) firstprivate(k){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(8){{$}}
    #pragma acc parallel num_gangs(8)
    // DMP:      ACCLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NOT:    <implicit>
    // DMP-NEXT:   ACCCollapseClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 2
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   impl: OMPDistributeDirective
    // DMP-NEXT:     OMPCollapseClause
    // DMP-NEXT:       ConstantExpr {{.*}} 'int'
    // DMP-NEXT:         value: Int 2
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'k' 'int'
    // DMP:          ForStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang collapse(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2) private(i,j){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute collapse(2) private(i,j){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang collapse(2){{$}}
    // PRT-NEXT:    for (i ={{.*}})
    #pragma acc loop gang collapse(2)
    for (i = 0; i < 2; ++i)
      // DMP: ForStmt
      // PRT-NEXT: for (j ={{.*}})
      for (j = 0; j < 2; ++j)
        // DMP: ForStmt
        // PRT-NEXT: for (k ={{.*}})
        for (k = 0; k < 2; ++k)
          // DMP: CallExpr
          // PRT-NEXT: printf
          // EXE-DAG: 0, 0, 0
          // EXE-DAG: 0, 0, 1
          // EXE-DAG: 0, 1, 0
          // EXE-DAG: 0, 1, 1
          // EXE-DAG: 1, 0, 0
          // EXE-DAG: 1, 0, 1
          // EXE-DAG: 1, 1, 0
          // EXE-DAG: 1, 1, 1
          printf("%d, %d, %d\n", i, j, k);
  } // PRT-NEXT: }

  // DMP: CallExpr
  // PRT-NEXT: printf
  // EXE-LABEL: loop worker private control var
  printf("loop worker private control var\n");

  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    // PRT: int j = 99;
    // PRT: int k = 99;
    int i = 99;
    int j = 99;
    int k = 99;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCNumWorkersClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1) num_workers(8){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1) firstprivate(k){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1) firstprivate(k){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) num_workers(8){{$}}
    #pragma acc parallel num_gangs(1) num_workers(8)
    // DMP:      ACCLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCCollapseClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 2
    // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:   impl: OMPDistributeParallelForDirective
    // DMP-NEXT:     OMPNum_threadsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 8
    // DMP-NEXT:     OMPCollapseClause
    // DMP-NEXT:       ConstantExpr {{.*}} 'int'
    // DMP-NEXT:         value: Int 2
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPPrivateClause {{.*}}
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:     OMPSharedClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'k' 'int'
    // DMP:          ForStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker collapse(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(8) collapse(2) private(i,j) shared(k){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute parallel for num_threads(8) collapse(2) private(i,j) shared(k){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker collapse(2){{$}}
    // PRT-NEXT:    for (i ={{.*}})
    #pragma acc loop worker collapse(2)
    for (i = 0; i < 2; ++i)
      // DMP: ForStmt
      // PRT-NEXT: for (j ={{.*}})
      for (j = 0; j < 2; ++j)
        // DMP: ForStmt
        // PRT-NEXT: for (k ={{.*}})
        // Because k is shared among workers, writes to it are a race.
        for (k = 0; k < 2; ++k)
          // DMP: CallExpr
          // PRT-NEXT: printf
          // EXE-DAG: 0, 0, {{.*}}
          // EXE-DAG: 0, 1, {{.*}}
          // EXE-DAG: 1, 0, {{.*}}
          // EXE-DAG: 1, 1, {{.*}}
          printf("%d, %d, %d\n", i, j, k);
  } // PRT-NEXT: }

  // DMP: CallExpr
  // PRT-NEXT: printf
  // EXE-LABEL: parallel loop vector private control var
  printf("parallel loop vector private control var\n");

  // PRT-NEXT: {
  {
    // PRT: int j = 99;
    // PRT: int k = 99;
    int j = 99;
    int k = 99;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCVectorLengthClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 8
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCCollapseClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 2
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:     ACCVectorLengthClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
    // DMP-NEXT:     ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:     ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCVectorClause
    // DMP-NEXT:       ACCCollapseClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 2
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCPrivateClause {{.*}} <predetermined>
    // DMP-NEXT:         DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:         DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:       impl: CompoundStmt
    // DMP-NEXT:         DeclStmt
    // DMP-NEXT:           VarDecl {{.*}} j 'int'
    // DMP-NEXT:         OMPDistributeSimdDirective
    // DMP-NEXT:           OMPSimdlenClause
    // DMP-NEXT:             ConstantExpr {{.*}} 'int'
    // DMP-NEXT:               value: Int 8
    // DMP-NEXT:               IntegerLiteral {{.*}} 8
    // DMP-NEXT:           OMPCollapseClause
    // DMP-NEXT:             ConstantExpr {{.*}} 'int'
    // DMP-NEXT:               value: Int 2
    // DMP-NEXT:               IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:           OMPSharedClause {{.*}} <implicit>
    // DMP-NEXT:             DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NOT:            OMPPrivateClause
    // DMP:                ForStmt
    // DMP:                  ForStmt
    // DMP:                    ForStmt
    // DMP:                      CallExpr
    //
    // PRT-NOACC-NEXT: for (int i ={{.*}})
    // PRT-NOACC-NEXT:   for (j ={{.*}})
    // PRT-NOACC-NEXT:     for (k ={{.*}})
    // PRT-NOACC-NEXT:       printf
    //
    // PRT-AO-NEXT: // v----------ACC----------v
    // PRT-A-NEXT:  #pragma acc parallel loop num_gangs(1) vector_length(8) vector collapse(2){{$}}
    // PRT-A-NEXT:  for (int i ={{.*}})
    // PRT-A-NEXT:    for (j ={{.*}})
    // PRT-A-NEXT:      for (k ={{.*}})
    // PRT-A-NEXT:        printf
    // PRT-AO-NEXT: // ---------ACC->OMP--------
    // PRT-AO-NEXT: // #pragma omp target teams num_teams(1) firstprivate(k){{$}}
    // PRT-AO-NEXT: // {
    // PRT-AO-NEXT: //   int j;
    // PRT-AO-NEXT: //   #pragma omp distribute simd simdlen(8) collapse(2){{$}}
    // PRT-AO-NEXT: //   for (int i ={{.*}})
    // PRT-AO-NEXT: //     for (j ={{.*}})
    // PRT-AO-NEXT: //       for (k ={{.*}})
    // PRT-AO-NEXT: //         printf
    // PRT-AO-NEXT: // }
    // PRT-AO-NEXT: // ^----------OMP----------^
    //
    // PRT-OA-NEXT: // v----------OMP----------v
    // PRT-O-NEXT:  #pragma omp target teams num_teams(1) firstprivate(k){{$}}
    // PRT-O-NEXT:  {
    // PRT-O-NEXT:    int j;
    // PRT-O-NEXT:    #pragma omp distribute simd simdlen(8) collapse(2){{$}}
    // PRT-O-NEXT:    for (int i ={{.*}})
    // PRT-O-NEXT:      for (j ={{.*}})
    // PRT-O-NEXT:        for (k ={{.*}})
    // PRT-O-NEXT:          printf
    // PRT-O-NEXT:  }
    // PRT-OA-NEXT: // ---------OMP<-ACC--------
    // PRT-OA-NEXT: // #pragma acc parallel loop num_gangs(1) vector_length(8) vector collapse(2){{$}}
    // PRT-OA-NEXT: // for (int i ={{.*}})
    // PRT-OA-NEXT: //   for (j ={{.*}})
    // PRT-OA-NEXT: //     for (k ={{.*}})
    // PRT-OA-NEXT: //       printf
    // PRT-OA-NEXT: // ^----------ACC----------^
    #pragma acc parallel loop num_gangs(1) vector_length(8) vector collapse(2)
    for (int i = 0; i < 2; ++i)
      for (j = 0; j < 2; ++j)
        for (k = 0; k < 2; ++k)
          // EXE-DAG: 0, 0, 0
          // EXE-DAG: 0, 0, 1
          // EXE-DAG: 0, 1, 0
          // EXE-DAG: 0, 1, 1
          // EXE-DAG: 1, 0, 0
          // EXE-DAG: 1, 0, 1
          // EXE-DAG: 1, 1, 0
          // EXE-DAG: 1, 1, 1
          printf("%d, %d, %d\n", i, j, k);
  } // PRT-NEXT: }

  // PRT-NEXT: return 0;
  return 0;
} // PRT-NEXT: }
// EXE-NOT: {{.}}
