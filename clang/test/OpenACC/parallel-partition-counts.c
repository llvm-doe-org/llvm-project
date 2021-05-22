// Check correct usage of num_workers and vector_length on "acc parallel" and
// "acc parallel loop".  Correct usage of num_gangs is checked in parallel.c
// and loop.c.

// Check -ast-dump before and after AST serialization.
//
// RUN: %clang -Xclang -verify -Xclang -ast-dump -fsyntax-only -fopenacc %s \
// RUN: | FileCheck -check-prefix=DMP %s
// RUN: %clang -Xclang -verify -fopenacc -emit-ast -o %t.ast %s
// RUN: %clang_cc1 -ast-dump-all %t.ast \
// RUN: | FileCheck -check-prefixes=DMP %s

// Check -ast-print and -fopenacc[-ast]-print.
//
// RUN: %clang -Xclang -verify=noacc -Xclang -ast-print -fsyntax-only %s \
// RUN: | FileCheck -check-prefixes=PRT,PRT-NOACC %s
//
// Strip comments and blank lines so checking -fopenacc-print output is easier.
// RUN: grep -v '^ *\(//.*\)\?$' %s | sed 's,//.*,,' > %t-acc.c
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
// RUN:   (prt='-Xclang -ast-print -fsyntax-only -fopenacc' prt-chk=PRT,PRT-A)
// RUN:   (prt=-fopenacc-ast-print=acc                      prt-chk=PRT,PRT-A)
// RUN:   (prt=-fopenacc-ast-print=omp                      prt-chk=PRT,PRT-O)
// RUN:   (prt=-fopenacc-ast-print=acc-omp                  prt-chk=PRT,PRT-A,PRT-AO)
// RUN:   (prt=-fopenacc-ast-print=omp-acc                  prt-chk=PRT,PRT-O,PRT-OA)
// RUN:   (prt=-fopenacc-print=acc                          prt-chk=PRT-PRE,PRT,PRT-A)
// RUN:   (prt=-fopenacc-print=omp                          prt-chk=PRT-PRE,PRT,PRT-O)
// RUN:   (prt=-fopenacc-print=acc-omp                      prt-chk=PRT-PRE,PRT,PRT-A,PRT-AO)
// RUN:   (prt=-fopenacc-print=omp-acc                      prt-chk=PRT-PRE,PRT,PRT-O,PRT-OA)
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
// RUN:   echo "// noacc""-no-diagnostics" >> %t-omp.c
// RUN:   %clang -Xclang -verify=noacc -fopenmp %fopenmp-version \
// RUN:          -Wno-unused-function -o %t %t-omp.c
// RUN:   %t 2 2>&1 | FileCheck -check-prefix=EXE %s
// RUN: }

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
// RUN:   %[run-if] FileCheck -input-file %t.out %s -check-prefixes=EXE
// RUN: }

// END.

// noacc-no-diagnostics

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int foo() { printf("foo\n"); return 5; }
int bar() { printf("bar\n"); return 6; }

// PRT: int main(int argc, char *argv[]) {
int main(int argc, char *argv[]) {
  // PRT-NEXT: argc == 2
  assert(argc == 2);

  // PRT: printf
  // EXE: start
  printf("start\n");

  //--------------------------------------------------
  // num_workers ice>1, vector_length ice>1, unused
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCNumWorkersClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCVectorLengthClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NOT:      OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1) num_workers(2) vector_length(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) num_workers(2) vector_length(2){{$}}
    #pragma acc parallel num_gangs(1) num_workers(2) vector_length(2)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCSeqClause
      // DMP-NEXT:   impl: ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop seq
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:  {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq // discarded in OpenMP translation{{$}}
      #pragma acc loop seq
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 8; ++i) {
        // sequential, so order is deterministic
        // DMP: CallExpr
        // PRT-NEXT: printf
        // EXE-NEXT: 0
        // EXE-NEXT: 1
        // EXE-NEXT: 2
        // EXE-NEXT: 3
        // EXE-NEXT: 4
        // EXE-NEXT: 5
        // EXE-NEXT: 6
        // EXE-NEXT: 7
        printf("%d\n", i);
      } // PRT-NEXT: }

      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
      #pragma acc loop gang
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 8; ++i) {
        // sequential as partitioned only across 1 gang
        // DMP: CallExpr
        // PRT-NEXT: printf
        // EXE-NEXT: 0
        // EXE-NEXT: 1
        // EXE-NEXT: 2
        // EXE-NEXT: 3
        // EXE-NEXT: 4
        // EXE-NEXT: 5
        // EXE-NEXT: 6
        // EXE-NEXT: 7
        printf("%d\n", i);
      } // PRT-NEXT: }
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCNumWorkersClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCVectorLengthClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCSeqClause
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:     ACCNumWorkersClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCVectorLengthClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCSeqClause
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(1) num_workers(2) vector_length(2) seq{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(1) num_workers(2) vector_length(2) seq{{$}}
    #pragma acc parallel loop num_gangs(1) num_workers(2) vector_length(2) seq
    // DMP-NEXT: impl: ForStmt
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
      #pragma acc loop gang
      // PRT-NEXT: for ({{.*}}) {
      for (int j = 0; j < 4; ++j) {
        // sequential as partitioned only across 1 gang
        // DMP: CallExpr
        // PRT-NEXT: printf
        // EXE-NEXT: 0
        // EXE-NEXT: 1
        // EXE-NEXT: 2
        // EXE-NEXT: 3
        // EXE-NEXT: 0
        // EXE-NEXT: 1
        // EXE-NEXT: 2
        // EXE-NEXT: 3
        printf("%d\n", j);
      } // PRT-NEXT: }
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCNumWorkersClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCVectorLengthClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:     ACCNumWorkersClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCVectorLengthClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCGangClause
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeDirective
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop gang num_gangs(1) num_workers(2) vector_length(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop gang num_gangs(1) num_workers(2) vector_length(2){{$}}
    #pragma acc parallel loop gang num_gangs(1) num_workers(2) vector_length(2)
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:  {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
      #pragma acc loop
      // PRT-NEXT: for ({{.*}}) {
      for (int j = 0; j < 4; ++j) {
        // sequential, so order is deterministic
        // DMP: CallExpr
        // PRT-NEXT: printf
        // EXE-NEXT: 0
        // EXE-NEXT: 1
        // EXE-NEXT: 2
        // EXE-NEXT: 3
        // EXE-NEXT: 0
        // EXE-NEXT: 1
        // EXE-NEXT: 2
        // EXE-NEXT: 3
        printf("%d\n", j);
      } // PRT-NEXT: }
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  //--------------------------------------------------
  // num_workers ice=1, vector_length ice=1, used separately
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCNumWorkersClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCVectorLengthClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NOT:      OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1) num_workers(1) vector_length(1){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) num_workers(1) vector_length(1){{$}}
    #pragma acc parallel num_gangs(1) num_workers(1) vector_length(1)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeParallelForDirective
      // DMP-NEXT:     OMPNum_threadsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(1){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute parallel for num_threads(1){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker{{$}}
      #pragma acc loop worker
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 8; ++i) {
        // sequential as partitioned only across 1 worker
        // DMP: CallExpr
        // PRT-NEXT: printf
        // EXE-NEXT: 0
        // EXE-NEXT: 1
        // EXE-NEXT: 2
        // EXE-NEXT: 3
        // EXE-NEXT: 4
        // EXE-NEXT: 5
        // EXE-NEXT: 6
        // EXE-NEXT: 7
        printf("%d\n", i);
      } // PRT-NEXT: }

      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeSimdDirective
      // DMP-NEXT:     OMPSimdlenClause
      // DMP-NEXT:       ConstantExpr {{.*}} 'int'
      // DMP-NEXT:         value: Int 1
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop vector{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd simdlen(1){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute simd simdlen(1){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector{{$}}
      #pragma acc loop vector
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 8; ++i) {
        // sequential as partitioned only across 1 vector lane
        // DMP: CallExpr
        // PRT-NEXT: printf
        // EXE-NEXT: 0
        // EXE-NEXT: 1
        // EXE-NEXT: 2
        // EXE-NEXT: 3
        // EXE-NEXT: 4
        // EXE-NEXT: 5
        // EXE-NEXT: 6
        // EXE-NEXT: 7
        printf("%d\n", i);
      } // PRT-NEXT: }

      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeParallelForDirective
      // DMP-NEXT:     OMPNum_threadsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang worker{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(1){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute parallel for num_threads(1){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang worker{{$}}
      #pragma acc loop gang worker
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 8; ++i) {
        // sequential as partitioned only across 1 gang, 1 worker
        // DMP: CallExpr
        // PRT-NEXT: printf
        // EXE-NEXT: 0
        // EXE-NEXT: 1
        // EXE-NEXT: 2
        // EXE-NEXT: 3
        // EXE-NEXT: 4
        // EXE-NEXT: 5
        // EXE-NEXT: 6
        // EXE-NEXT: 7
        printf("%d\n", i);
      } // PRT-NEXT: }

      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeSimdDirective
      // DMP-NEXT:     OMPSimdlenClause
      // DMP-NEXT:       ConstantExpr {{.*}} 'int'
      // DMP-NEXT:         value: Int 1
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang vector{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd simdlen(1){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute simd simdlen(1){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang vector{{$}}
      #pragma acc loop gang vector
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 8; ++i) {
        // sequential as partitioned only across 1 gang, 1 vector lane
        // DMP: CallExpr
        // PRT-NEXT: printf
        // EXE-NEXT: 0
        // EXE-NEXT: 1
        // EXE-NEXT: 2
        // EXE-NEXT: 3
        // EXE-NEXT: 4
        // EXE-NEXT: 5
        // EXE-NEXT: 6
        // EXE-NEXT: 7
        printf("%d\n", i);
      } // PRT-NEXT: }

      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
      #pragma acc loop gang
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP:      ACCLoopDirective
        // DMP-NEXT:   ACCWorkerClause
        // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
        // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
        // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
        // DMP-NEXT:   impl: OMPParallelForDirective
        // DMP-NEXT:     OMPNum_threadsClause
        // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
        // DMP-NEXT:     OMPSharedClause
        // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
        //
        // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker{{$}}
        // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for num_threads(1) shared(i){{$}}
        // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for num_threads(1) shared(i){{$}}
        // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker{{$}}
        #pragma acc loop worker
        // PRT-NEXT: for ({{.*}}) {
        for (int j = 0; j < 2; ++j) {
          // DMP:      ACCLoopDirective
          // DMP-NEXT:   ACCVectorClause
          // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
          // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
          // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
          // DMP-NEXT:     DeclRefExpr {{.*}} 'j' 'int'
          // DMP-NEXT:   impl: OMPSimdDirective
          // DMP-NEXT:     OMPSimdlenClause
          // DMP-NEXT:       ConstantExpr {{.*}} 'int'
          // DMP-NEXT:         value: Int 1
          // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
          // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
          // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
          // DMP-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
          // DMP-NOT:      OMP
          //
          // PRT-A-NEXT:  {{^ *}}#pragma acc loop vector{{$}}
          // PRT-AO-NEXT: {{^ *}}// #pragma omp simd simdlen(1){{$}}
          // PRT-O-NEXT:  {{^ *}}#pragma omp simd simdlen(1){{$}}
          // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector{{$}}
          #pragma acc loop vector
          // PRT-NEXT: for ({{.*}}) {
          for (int k = 0; k < 2; ++k) {
            // sequential as partitioned only across 1 gang, 1 worker, 1 vector
            // lane
            // DMP: CallExpr
            // PRT-NEXT: printf
            // EXE-NEXT: 0, 0, 0
            // EXE-NEXT: 0, 0, 1
            // EXE-NEXT: 0, 1, 0
            // EXE-NEXT: 0, 1, 1
            // EXE-NEXT: 1, 0, 0
            // EXE-NEXT: 1, 0, 1
            // EXE-NEXT: 1, 1, 0
            // EXE-NEXT: 1, 1, 1
            printf("%d, %d, %d\n", i, j, k);
          } // PRT-NEXT: }
        } // PRT-NEXT: }
      } // PRT-NEXT: }
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCNumWorkersClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCVectorLengthClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:     ACCNumWorkersClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:     ACCVectorLengthClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCWorkerClause
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeParallelForDirective
    // DMP-NEXT:         OMPNum_threadsClause
    // DMP-NEXT:           IntegerLiteral {{.*}} 'int' 1
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop worker num_gangs(1) num_workers(1) vector_length(1){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for num_threads(1){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute parallel for num_threads(1){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop worker num_gangs(1) num_workers(1) vector_length(1){{$}}
    #pragma acc parallel loop worker num_gangs(1) num_workers(1) vector_length(1)
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPSimdDirective
      // DMP-NEXT:     OMPSimdlenClause
      // DMP-NEXT:       ConstantExpr {{.*}} 'int'
      // DMP-NEXT:         value: Int 1
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop vector{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp simd simdlen(1){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp simd simdlen(1){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector{{$}}
      #pragma acc loop vector
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 4; ++i) {
        // sequential as partitioned only across 1 vector lane
        // DMP: CallExpr
        // PRT-NEXT: printf
        // EXE-NEXT: 0
        // EXE-NEXT: 1
        // EXE-NEXT: 2
        // EXE-NEXT: 3
        // EXE-NEXT: 0
        // EXE-NEXT: 1
        // EXE-NEXT: 2
        // EXE-NEXT: 3
        printf("%d\n", i);
      } // PRT-NEXT: }
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCNumWorkersClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCVectorLengthClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:     ACCNumWorkersClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:     ACCVectorLengthClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCVectorClause
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeSimdDirective
    // DMP-NEXT:         OMPSimdlenClause
    // DMP-NEXT:           ConstantExpr {{.*}} 'int'
    // DMP-NEXT:             value: Int 1
    // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 1
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop vector num_gangs(1) num_workers(1) vector_length(1){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd simdlen(1){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute simd simdlen(1){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop vector num_gangs(1) num_workers(1) vector_length(1){{$}}
    #pragma acc parallel loop vector num_gangs(1) num_workers(1) vector_length(1)
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:  {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
      #pragma acc loop
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 4; ++i) {
        // sequential as partitioned only across 1 vector lane
        // DMP: CallExpr
        // PRT-NEXT: printf
        // EXE-NEXT: 0
        // EXE-NEXT: 1
        // EXE-NEXT: 2
        // EXE-NEXT: 3
        // EXE-NEXT: 0
        // EXE-NEXT: 1
        // EXE-NEXT: 2
        // EXE-NEXT: 3
        printf("%d\n", i);
      } // PRT-NEXT: }
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  //--------------------------------------------------
  // num_workers ice>1, vector_length ice>1, used together
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCNumWorkersClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCVectorLengthClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NOT:      OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1) num_workers(2) vector_length(3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) num_workers(2) vector_length(3){{$}}
    #pragma acc parallel num_gangs(1) num_workers(2) vector_length(3)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
      // DMP-NEXT:     OMPNum_threadsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
      // DMP-NEXT:     OMPSimdlenClause
      // DMP-NEXT:       ConstantExpr {{.*}} 'int'
      // DMP-NEXT:         value: Int 3
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker vector{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(2) simdlen(3){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute parallel for simd num_threads(2) simdlen(3){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector{{$}}
      #pragma acc loop worker vector
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 8; ++i) {
        // not sequential
        // DMP: CallExpr
        // PRT-NEXT: printf
        // EXE-DAG: 0
        // EXE-DAG: 1
        // EXE-DAG: 2
        // EXE-DAG: 3
        // EXE-DAG: 4
        // EXE-DAG: 5
        // EXE-DAG: 6
        // EXE-DAG: 7
        printf("%d\n", i);
      } // PRT-NEXT: }

      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
      // DMP-NEXT:     OMPNum_threadsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
      // DMP-NEXT:     OMPSimdlenClause
      // DMP-NEXT:       ConstantExpr {{.*}} 'int'
      // DMP-NEXT:         value: Int 3
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang worker vector{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(2) simdlen(3){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute parallel for simd num_threads(2) simdlen(3){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang worker vector{{$}}
      #pragma acc loop gang worker vector
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 8; ++i) {
        // not sequential
        // DMP: CallExpr
        // PRT-NEXT: printf
        // EXE-DAG: 0
        // EXE-DAG: 1
        // EXE-DAG: 2
        // EXE-DAG: 3
        // EXE-DAG: 4
        // EXE-DAG: 5
        // EXE-DAG: 6
        // EXE-DAG: 7
        printf("%d\n", i);
      } // PRT-NEXT: }

      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
      #pragma acc loop gang
      // DMP: ForStmt
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP:      ACCLoopDirective
        // DMP-NEXT:   ACCWorkerClause
        // DMP-NEXT:   ACCVectorClause
        // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
        // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
        // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
        // DMP-NEXT:   impl: OMPParallelForSimdDirective
        // DMP-NEXT:     OMPNum_threadsClause
        // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
        // DMP-NEXT:     OMPSimdlenClause
        // DMP-NEXT:       ConstantExpr {{.*}} 'int'
        // DMP-NEXT:         value: Int 3
        // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
        // DMP-NEXT:     OMPSharedClause {{.*}}
        // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
        // DMP-NOT:      OMP
        //
        // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker vector{{$}}
        // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for simd num_threads(2) simdlen(3) shared(i){{$}}
        // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for simd num_threads(2) simdlen(3) shared(i){{$}}
        // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector{{$}}
        #pragma acc loop worker vector
        // PRT-NEXT: for ({{.*}}) {
        for (int j = 0; j < 4; ++j) {
          // not sequential
          // DMP: CallExpr
          // PRT-NEXT: printf
          // EXE-DAG: 0, 0
          // EXE-DAG: 0, 1
          // EXE-DAG: 0, 2
          // EXE-DAG: 0, 3
          // EXE-DAG: 1, 0
          // EXE-DAG: 1, 1
          // EXE-DAG: 1, 2
          // EXE-DAG: 1, 3
          printf("%d, %d\n", i, j);
        } // PRT-NEXT: }
      } // PRT-NEXT: }
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCNumWorkersClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCVectorLengthClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:     ACCNumWorkersClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCVectorLengthClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCWorkerClause
    // DMP-NEXT:       ACCVectorClause
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeParallelForSimdDirective
    // DMP-NEXT:         OMPNum_threadsClause
    // DMP-NEXT:           IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:         OMPSimdlenClause
    // DMP-NEXT:           ConstantExpr {{.*}} 'int'
    // DMP-NEXT:             value: Int 3
    // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 3
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop worker vector num_gangs(1) num_workers(2) vector_length(3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(2) simdlen(3){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute parallel for simd num_threads(2) simdlen(3){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop worker vector num_gangs(1) num_workers(2) vector_length(3){{$}}
    #pragma acc parallel loop worker vector num_gangs(1) num_workers(2) vector_length(3)
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 8; ++i) {
      // not sequential
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-DAG: 0
      // EXE-DAG: 1
      // EXE-DAG: 2
      // EXE-DAG: 3
      // EXE-DAG: 4
      // EXE-DAG: 5
      // EXE-DAG: 6
      // EXE-DAG: 7
      printf("%d\n", i);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCNumWorkersClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCVectorLengthClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:     ACCNumWorkersClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCVectorLengthClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCGangClause
    // DMP-NEXT:       ACCWorkerClause
    // DMP-NEXT:       ACCVectorClause
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeParallelForSimdDirective
    // DMP-NEXT:         OMPNum_threadsClause
    // DMP-NEXT:           IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:         OMPSimdlenClause
    // DMP-NEXT:           ConstantExpr {{.*}} 'int'
    // DMP-NEXT:             value: Int 3
    // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 3
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop gang worker vector num_gangs(1) num_workers(2) vector_length(3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd num_threads(2) simdlen(3){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute parallel for simd num_threads(2) simdlen(3){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop gang worker vector num_gangs(1) num_workers(2) vector_length(3){{$}}
    #pragma acc parallel loop gang worker vector num_gangs(1) num_workers(2) vector_length(3)
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 8; ++i) {
      // not sequential
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-DAG: 0
      // EXE-DAG: 1
      // EXE-DAG: 2
      // EXE-DAG: 3
      // EXE-DAG: 4
      // EXE-DAG: 5
      // EXE-DAG: 6
      // EXE-DAG: 7
      printf("%d\n", i);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCNumWorkersClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCVectorLengthClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:     ACCNumWorkersClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCVectorLengthClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCGangClause
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeDirective
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop gang num_gangs(1) num_workers(2) vector_length(3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop gang num_gangs(1) num_workers(2) vector_length(3){{$}}
    #pragma acc parallel loop gang num_gangs(1) num_workers(2) vector_length(3)
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NEXT:   impl: OMPParallelForSimdDirective
      // DMP-NEXT:     OMPNum_threadsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
      // DMP-NEXT:     OMPSimdlenClause
      // DMP-NEXT:       ConstantExpr {{.*}} 'int'
      // DMP-NEXT:         value: Int 3
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
      // DMP-NEXT:     OMPSharedClause {{.*}}
      // DMP-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker vector{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for simd num_threads(2) simdlen(3) shared(i){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for simd num_threads(2) simdlen(3) shared(i){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector{{$}}
      #pragma acc loop worker vector
      // PRT-NEXT: for ({{.*}}) {
      for (int j = 0; j < 4; ++j) {
        // not sequential
        // DMP: CallExpr
        // PRT-NEXT: printf
        // EXE-DAG: 0, 0
        // EXE-DAG: 0, 1
        // EXE-DAG: 0, 2
        // EXE-DAG: 0, 3
        // EXE-DAG: 1, 0
        // EXE-DAG: 1, 1
        // EXE-DAG: 1, 2
        // EXE-DAG: 1, 3
        printf("%d, %d\n", i, j);
      } // PRT-NEXT: }
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  //--------------------------------------------------
  // num_workers non-constant>1, no side effects, unused
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int nw = 2;
    int nw = 2;

    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCNumWorkersClause
    // DMP:          DeclRefExpr {{.*}} 'nw' 'int'
    // DMP-NEXT:   ACCVectorLengthClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NOT:      OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1) num_workers(nw) vector_length(1){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) num_workers(nw) vector_length(1){{$}}
    #pragma acc parallel num_gangs(1) num_workers(nw) vector_length(1)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCSeqClause
      // DMP-NEXT:   impl: ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop seq
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:  {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq // discarded in OpenMP translation{{$}}
      #pragma acc loop seq
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 8; ++i) {
        // sequential, so order is deterministic
        // DMP: CallExpr
        // PRT-NEXT: printf
        // EXE-NEXT: 0
        // EXE-NEXT: 1
        // EXE-NEXT: 2
        // EXE-NEXT: 3
        // EXE-NEXT: 4
        // EXE-NEXT: 5
        // EXE-NEXT: 6
        // EXE-NEXT: 7
        printf("%d\n", i);
      } // PRT-NEXT: }

      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang{{$}}
      #pragma acc loop gang
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 8; ++i) {
        // sequential as partitioned only across 1 gang
        // DMP: CallExpr
        // PRT-NEXT: printf
        // EXE-NEXT: 0
        // EXE-NEXT: 1
        // EXE-NEXT: 2
        // EXE-NEXT: 3
        // EXE-NEXT: 4
        // EXE-NEXT: 5
        // EXE-NEXT: 6
        // EXE-NEXT: 7
        printf("%d\n", i);
      } // PRT-NEXT: }

      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeSimdDirective
      // DMP-NEXT:     OMPSimdlenClause
      // DMP-NEXT:       ConstantExpr {{.*}} 'int'
      // DMP-NEXT:         value: Int 1
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop vector{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd simdlen(1){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute simd simdlen(1){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector{{$}}
      #pragma acc loop vector
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 8; ++i) {
        // sequential as partitioned only across 1 vector lane
        // DMP: CallExpr
        // PRT-NEXT: printf
        // EXE-NEXT: 0
        // EXE-NEXT: 1
        // EXE-NEXT: 2
        // EXE-NEXT: 3
        // EXE-NEXT: 4
        // EXE-NEXT: 5
        // EXE-NEXT: 6
        // EXE-NEXT: 7
        printf("%d\n", i);
      } // PRT-NEXT: }

      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCAutoClause
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   impl: ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop auto worker
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:  {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto worker // discarded in OpenMP translation{{$}}
      #pragma acc loop auto worker
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 8; ++i) {
        // sequential, so order is deterministic
        // DMP: CallExpr
        // PRT-NEXT: printf
        // EXE-NEXT: 0
        // EXE-NEXT: 1
        // EXE-NEXT: 2
        // EXE-NEXT: 3
        // EXE-NEXT: 4
        // EXE-NEXT: 5
        // EXE-NEXT: 6
        // EXE-NEXT: 7
        printf("%d\n", i);
      } // PRT-NEXT: }
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: int nw = 2;
    int nw = 2;

    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCNumWorkersClause
    // DMP:          DeclRefExpr {{.*}} 'nw' 'int'
    // DMP-NEXT:   ACCVectorLengthClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCSeqClause
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:     ACCNumWorkersClause
    // DMP:            DeclRefExpr {{.*}} 'nw' 'int'
    // DMP-NEXT:     ACCVectorLengthClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCSeqClause
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(1) num_workers(nw) vector_length(1) seq{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(1) num_workers(nw) vector_length(1) seq{{$}}
    #pragma acc parallel loop num_gangs(1) num_workers(nw) vector_length(1) seq
    // DMP-NEXT: impl: ForStmt
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 8; ++i) {
      // sequential, so order is deterministic
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-NEXT: 0
      // EXE-NEXT: 1
      // EXE-NEXT: 2
      // EXE-NEXT: 3
      // EXE-NEXT: 4
      // EXE-NEXT: 5
      // EXE-NEXT: 6
      // EXE-NEXT: 7
      printf("%d\n", i);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: int nw = 2;
    int nw = 2;

    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCNumWorkersClause
    // DMP:          DeclRefExpr {{.*}} 'nw' 'int'
    // DMP-NEXT:   ACCVectorLengthClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:     ACCNumWorkersClause
    // DMP:            DeclRefExpr {{.*}} 'nw' 'int'
    // DMP-NEXT:     ACCVectorLengthClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCGangClause
    // DMP-NEXT:       ACCVectorClause
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeSimdDirective
    // DMP-NEXT:         OMPSimdlenClause
    // DMP-NEXT:           ConstantExpr {{.*}} 'int'
    // DMP-NEXT:             value: Int 1
    // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 1
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop gang vector num_gangs(1) num_workers(nw) vector_length(1){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd simdlen(1){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute simd simdlen(1){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop gang vector num_gangs(1) num_workers(nw) vector_length(1){{$}}
    #pragma acc parallel loop gang vector num_gangs(1) num_workers(nw) vector_length(1)
    // DMP: ForStmt
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 8; ++i) {
      // sequential as partitioned only across 1 gang
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-NEXT: 0
      // EXE-NEXT: 1
      // EXE-NEXT: 2
      // EXE-NEXT: 3
      // EXE-NEXT: 4
      // EXE-NEXT: 5
      // EXE-NEXT: 6
      // EXE-NEXT: 7
      printf("%d\n", i);
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  //--------------------------------------------------
  // num_workers non-constant>1, side effects, unused
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCNumWorkersClause
    // DMP-NEXT:     CallExpr
    // DMP:            DeclRefExpr {{.*}} 'foo'
    // DMP-NEXT:   ACCVectorLengthClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     CStyleCastExpr {{.*}} 'void' <ToVoid>
    // DMP-NEXT:       CallExpr
    // DMP:              DeclRefExpr {{.*}} 'foo'
    // DMP-NEXT:     OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP-NOT:        OMP
    // DMP:        CompoundStmt
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCVectorClause
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeSimdDirective
    // DMP-NEXT:         OMPSimdlenClause
    // DMP-NEXT:           ConstantExpr {{.*}} 'int'
    // DMP-NEXT:             value: Int 1
    // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 1
    // DMP-NOT:          OMP
    // DMP:            CallExpr
    //
    // PRT-NOACC-NEXT: {
    // PRT-NOACC-NEXT:   for ({{.*}}) {
    // PRT-NOACC-NEXT:     printf
    // PRT-NOACC-NEXT:   }
    // PRT-NOACC-NEXT: }
    // PRT-AO-NEXT:    // v----------ACC----------v
    // PRT-A-NEXT:     {{^ *}}#pragma acc parallel num_gangs(1) num_workers(foo()) vector_length(1){{$}}
    // PRT-A-NEXT:     {
    // PRT-A-NEXT:       {{^ *}}#pragma acc loop vector{{$}}
    // PRT-A-NEXT:       for ({{.*}}) {
    // PRT-A-NEXT:         printf
    // PRT-A-NEXT:       }
    // PRT-A-NEXT:     }
    // PRT-AO-NEXT:    // ---------ACC->OMP--------
    // PRT-AO-NEXT:    // {
    // PRT-AO-NEXT:    //   (void)foo();
    // PRT-AO-NEXT:    //   #pragma omp target teams num_teams(1){{$}}
    // PRT-AO-NEXT:    //   {
    // PRT-AO-NEXT:    //     #pragma omp distribute simd simdlen(1){{$}}
    // PRT-AO-NEXT:    //     for ({{.*}}) {
    // PRT-AO-NEXT:    //       printf
    // PRT-AO-NEXT:    //     }
    // PRT-AO-NEXT:    //   }
    // PRT-AO-NEXT:    // }
    // PRT-AO-NEXT:    // ^----------OMP----------^
    // PRT-OA-NEXT:    // v----------OMP----------v
    // PRT-O-NEXT:     {
    // PRT-O-NEXT:       (void)foo();
    // PRT-O-NEXT:       {{^ *}}#pragma omp target teams num_teams(1){{$}}
    // PRT-O-NEXT:       {
    // PRT-O-NEXT:         {{^ *}}#pragma omp distribute simd simdlen(1){{$}}
    // PRT-O-NEXT:         for ({{.*}}) {
    // PRT-O-NEXT:           printf
    // PRT-O-NEXT:         }
    // PRT-O-NEXT:       }
    // PRT-O-NEXT:     }
    // PRT-OA-NEXT:    // ---------OMP<-ACC--------
    // PRT-OA-NEXT:    // #pragma acc parallel num_gangs(1) num_workers(foo()) vector_length(1){{$}}
    // PRT-OA-NEXT:    // {
    // PRT-OA-NEXT:    //   #pragma acc loop vector{{$}}
    // PRT-OA-NEXT:    //   for ({{.*}}) {
    // PRT-OA-NEXT:    //     printf
    // PRT-OA-NEXT:    //   }
    // PRT-OA-NEXT:    // }
    // PRT-OA-NEXT:    // ^----------ACC----------^
    //
    // EXE-NEXT: foo
    #pragma acc parallel num_gangs(1) num_workers(foo()) vector_length(1)
    {
      #pragma acc loop vector
      for (int i = 0; i < 8; ++i) {
        // sequential as partitioned only across 1 vector lane
        // EXE-NEXT: 0
        // EXE-NEXT: 1
        // EXE-NEXT: 2
        // EXE-NEXT: 3
        // EXE-NEXT: 4
        // EXE-NEXT: 5
        // EXE-NEXT: 6
        // EXE-NEXT: 7
        printf("%d\n", i);
      }
    }
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: int nw = 1;
    int nw = 1;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCNumWorkersClause
    // DMP-NEXT:     ParenExpr
    // DMP-NEXT:       BinaryOperator {{.*}} 'int' '='
    // DMP-NEXT:         DeclRefExpr {{.*}} 'nw' 'int'
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCVectorLengthClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:     ACCNumWorkersClause
    // DMP-NEXT:       ParenExpr
    // DMP-NEXT:         BinaryOperator {{.*}} 'int' '='
    // DMP-NEXT:           DeclRefExpr {{.*}} 'nw' 'int'
    // DMP-NEXT:           IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCVectorLengthClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:     impl: CompoundStmt
    // DMP-NEXT:       CStyleCastExpr {{.*}} 'void' <ToVoid>
    // DMP-NEXT:         ParenExpr
    // DMP-NEXT:           BinaryOperator {{.*}} 'int' '='
    // DMP-NEXT:             DeclRefExpr {{.*}} 'nw' 'int'
    // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPTargetTeamsDirective
    // DMP-NEXT:         OMPNum_teamsClause
    // DMP-NEXT:           IntegerLiteral {{.*}} 'int' 1
    // DMP-NOT:          OMP
    // DMP:              CapturedStmt
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCGangClause
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeDirective
    // DMP:            ForStmt
    // DMP:              ACCLoopDirective
    // DMP-NEXT:           ACCVectorClause
    // DMP-NEXT:           ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:           ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:           impl: OMPSimdDirective
    // DMP-NEXT:             OMPSimdlenClause
    // DMP-NEXT:               ConstantExpr {{.*}} 'int'
    // DMP-NEXT:                 value: Int 1
    // DMP-NEXT:                 IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:             OMPSharedClause {{.*}} <implicit>
    // DMP-NEXT:               DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NOT:              OMP
    // DMP:                  ForStmt
    // DMP:                    CallExpr
    //
    // PRT-NOACC-NEXT: for ({{.*}}) {
    // PRT-NOACC-NEXT:   for ({{.*}}) {
    // PRT-NOACC-NEXT:     printf
    // PRT-NOACC-NEXT:   }
    // PRT-NOACC-NEXT: }
    // PRT-AO-NEXT:    // v----------ACC----------v
    // PRT-A-NEXT:     {{^ *}}#pragma acc parallel loop gang num_gangs(1) num_workers((nw = 2)) vector_length(1){{$}}
    // PRT-A-NEXT:     for ({{.*}}) {
    // PRT-A-NEXT:       {{^ *}}#pragma acc loop vector{{$}}
    // PRT-A-NEXT:       for ({{.*}}) {
    // PRT-A-NEXT:         printf
    // PRT-A-NEXT:       }
    // PRT-A-NEXT:     }
    // PRT-AO-NEXT:    // ---------ACC->OMP--------
    // PRT-AO-NEXT:    // {
    // PRT-AO-NEXT:    //   (void)(nw = 2);
    // PRT-AO-NEXT:    //   #pragma omp target teams num_teams(1){{$}}
    // PRT-AO-NEXT:    //   #pragma omp distribute{{$}}
    // PRT-AO-NEXT:    //   for ({{.*}}) {
    // PRT-AO-NEXT:    //     #pragma omp simd simdlen(1){{$}}
    // PRT-AO-NEXT:    //     for ({{.*}}) {
    // PRT-AO-NEXT:    //       printf
    // PRT-AO-NEXT:    //     }
    // PRT-AO-NEXT:    //   }
    // PRT-AO-NEXT:    // }
    // PRT-AO-NEXT:    // ^----------OMP----------^
    // PRT-OA-NEXT:    // v----------OMP----------v
    // PRT-O-NEXT:     {
    // PRT-O-NEXT:       (void)(nw = 2);
    // PRT-O-NEXT:       {{^ *}}#pragma omp target teams num_teams(1){{$}}
    // PRT-O-NEXT:       {{^ *}}#pragma omp distribute{{$}}
    // PRT-O-NEXT:       for ({{.*}}) {
    // PRT-O-NEXT:         {{^ *}}#pragma omp simd simdlen(1){{$}}
    // PRT-O-NEXT:         for ({{.*}}) {
    // PRT-O-NEXT:           printf
    // PRT-O-NEXT:         }
    // PRT-O-NEXT:       }
    // PRT-O-NEXT:     }
    // PRT-OA-NEXT:    // ---------OMP<-ACC--------
    // PRT-OA-NEXT:    // #pragma acc parallel loop gang num_gangs(1) num_workers((nw = 2)) vector_length(1){{$}}
    // PRT-OA-NEXT:    // for ({{.*}}) {
    // PRT-OA-NEXT:    //   #pragma acc loop vector{{$}}
    // PRT-OA-NEXT:    //   for ({{.*}}) {
    // PRT-OA-NEXT:    //     printf
    // PRT-OA-NEXT:    //   }
    // PRT-OA-NEXT:    // }
    // PRT-OA-NEXT:    // ^----------ACC----------^
    #pragma acc parallel loop gang num_gangs(1) num_workers((nw = 2)) vector_length(1)
    for (int i = 0; i < 2; ++i) {
      #pragma acc loop vector
      for (int j = 0; j < 4; ++j) {
        // sequential as partitioned only across 1 gang, 1 vector lane
        // EXE-NEXT: 0, 0
        // EXE-NEXT: 0, 1
        // EXE-NEXT: 0, 2
        // EXE-NEXT: 0, 3
        // EXE-NEXT: 1, 0
        // EXE-NEXT: 1, 1
        // EXE-NEXT: 1, 2
        // EXE-NEXT: 1, 3
        printf("%d, %d\n", i, j);
      }
    }
  } // PRT-NEXT: }

  //--------------------------------------------------
  // num_workers non-constant>1, no side effects, used
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int nw = 2;
    int nw = 2;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCNumWorkersClause
    // DMP:          DeclRefExpr {{.*}} 'nw' 'int'
    // DMP-NEXT:   ACCVectorLengthClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} __clang_acc_num_workers__ 'const int'
    // DMP:              DeclRefExpr {{.*}} 'nw' 'int'
    // DMP-NEXT:     OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:       OMPFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:         DeclRefExpr {{.*}} '__clang_acc_num_workers__' 'const int'
    // DMP-NOT:        OMP
    // DMP:        CompoundStmt
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCWorkerClause
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeParallelForDirective
    // DMP-NEXT:         OMPNum_threadsClause
    // DMP-NEXT:           DeclRefExpr {{.*}} '__clang_acc_num_workers__' 'const int'
    // DMP-NOT:          OMP
    // DMP:            CallExpr
    //
    // PRT-NOACC-NEXT: {
    // PRT-NOACC-NEXT:   for ({{.*}}) {
    // PRT-NOACC-NEXT:     printf
    // PRT-NOACC-NEXT:   }
    // PRT-NOACC-NEXT: }
    // PRT-AO-NEXT:    // v----------ACC----------v
    // PRT-A-NEXT:     {{^ *}}#pragma acc parallel num_gangs(1) num_workers(nw) vector_length(1){{$}}
    // PRT-A-NEXT:     {
    // PRT-A-NEXT:       {{^ *}}#pragma acc loop worker{{$}}
    // PRT-A-NEXT:       for ({{.*}}) {
    // PRT-A-NEXT:         printf
    // PRT-A-NEXT:       }
    // PRT-A-NEXT:     }
    // PRT-AO-NEXT:    // ---------ACC->OMP--------
    // PRT-AO-NEXT:    // {
    // PRT-AO-NEXT:    //   const int __clang_acc_num_workers__ = nw;
    // PRT-AO-NEXT:    //   #pragma omp target teams num_teams(1){{$}}
    // PRT-AO-NEXT:    //   {
    // PRT-AO-NEXT:    //     #pragma omp distribute parallel for num_threads(__clang_acc_num_workers__){{$}}
    // PRT-AO-NEXT:    //     for ({{.*}}) {
    // PRT-AO-NEXT:    //       printf
    // PRT-AO-NEXT:    //     }
    // PRT-AO-NEXT:    //   }
    // PRT-AO-NEXT:    // }
    // PRT-AO-NEXT:    // ^----------OMP----------^
    // PRT-OA-NEXT:    // v----------OMP----------v
    // PRT-O-NEXT:     {
    // PRT-O-NEXT:       const int __clang_acc_num_workers__ = nw;
    // PRT-O-NEXT:       {{^ *}}#pragma omp target teams num_teams(1){{$}}
    // PRT-O-NEXT:       {
    // PRT-O-NEXT:         {{^ *}}#pragma omp distribute parallel for num_threads(__clang_acc_num_workers__){{$}}
    // PRT-O-NEXT:         for ({{.*}}) {
    // PRT-O-NEXT:           printf
    // PRT-O-NEXT:         }
    // PRT-O-NEXT:       }
    // PRT-O-NEXT:     }
    // PRT-OA-NEXT:    // ---------OMP<-ACC--------
    // PRT-OA-NEXT:    // #pragma acc parallel num_gangs(1) num_workers(nw) vector_length(1){{$}}
    // PRT-OA-NEXT:    // {
    // PRT-OA-NEXT:    //   #pragma acc loop worker{{$}}
    // PRT-OA-NEXT:    //   for ({{.*}}) {
    // PRT-OA-NEXT:    //     printf
    // PRT-OA-NEXT:    //   }
    // PRT-OA-NEXT:    // }
    // PRT-OA-NEXT:    // ^----------ACC----------^
    #pragma acc parallel num_gangs(1) num_workers(nw) vector_length(1)
    {
      #pragma acc loop worker
      for (int i = 0; i < 8; ++i) {
        // not sequential
        // EXE-DAG: 0
        // EXE-DAG: 1
        // EXE-DAG: 2
        // EXE-DAG: 3
        // EXE-DAG: 4
        // EXE-DAG: 5
        // EXE-DAG: 6
        // EXE-DAG: 7
        printf("%d\n", i);
      }
    }
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: int nw = {{.*}};
    int nw = atoi(argv[1]);

    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCNumWorkersClause
    // DMP:          DeclRefExpr {{.*}} 'nw' 'int'
    // DMP-NEXT:   ACCVectorLengthClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:     ACCNumWorkersClause
    // DMP:            DeclRefExpr {{.*}} 'nw' 'int'
    // DMP-NEXT:     ACCVectorLengthClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:     impl: CompoundStmt
    // DMP-NEXT:       DeclStmt
    // DMP-NEXT:         VarDecl {{.*}} __clang_acc_num_workers__ 'const int'
    // DMP:                DeclRefExpr {{.*}} 'nw' 'int'
    // DMP-NEXT:       OMPTargetTeamsDirective
    // DMP-NEXT:         OMPNum_teamsClause
    // DMP-NEXT:           IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:         OMPFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:           DeclRefExpr {{.*}} '__clang_acc_num_workers__' 'const int'
    // DMP-NOT:          OMP
    // DMP:              CapturedStmt
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCWorkerClause
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeParallelForDirective
    // DMP-NEXT:         OMPNum_threadsClause
    // DMP-NEXT:           DeclRefExpr {{.*}} '__clang_acc_num_workers__' 'const int'
    // DMP:            ForStmt
    // DMP:              ACCLoopDirective
    // DMP-NEXT:           ACCVectorClause
    // DMP-NEXT:           ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:           ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:           impl: OMPSimdDirective
    // DMP-NEXT:             OMPSimdlenClause
    // DMP-NEXT:               ConstantExpr {{.*}} 'int'
    // DMP-NEXT:                 value: Int 1
    // DMP-NEXT:                 IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:             OMPSharedClause {{.*}} <implicit>
    // DMP-NEXT:               DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NOT:              OMP
    // DMP:                  ForStmt
    // DMP:                    CallExpr
    //
    // PRT-NOACC-NEXT: for ({{.*}}) {
    // PRT-NOACC-NEXT:   for ({{.*}}) {
    // PRT-NOACC-NEXT:     printf
    // PRT-NOACC-NEXT:   }
    // PRT-NOACC-NEXT: }
    // PRT-AO-NEXT:    // v----------ACC----------v
    // PRT-A-NEXT:     {{^ *}}#pragma acc parallel loop worker num_gangs(1) num_workers(nw) vector_length(1){{$}}
    // PRT-A-NEXT:     for ({{.*}}) {
    // PRT-A-NEXT:       {{^ *}}#pragma acc loop vector{{$}}
    // PRT-A-NEXT:       for ({{.*}}) {
    // PRT-A-NEXT:         printf
    // PRT-A-NEXT:       }
    // PRT-A-NEXT:     }
    // PRT-AO-NEXT:    // ---------ACC->OMP--------
    // PRT-AO-NEXT:    // {
    // PRT-AO-NEXT:    //   const int __clang_acc_num_workers__ = nw;
    // PRT-AO-NEXT:    //   #pragma omp target teams num_teams(1){{$}}
    // PRT-AO-NEXT:    //   #pragma omp distribute parallel for num_threads(__clang_acc_num_workers__){{$}}
    // PRT-AO-NEXT:    //   for ({{.*}}) {
    // PRT-AO-NEXT:    //     #pragma omp simd simdlen(1){{$}}
    // PRT-AO-NEXT:    //     for ({{.*}}) {
    // PRT-AO-NEXT:    //       printf
    // PRT-AO-NEXT:    //     }
    // PRT-AO-NEXT:    //   }
    // PRT-AO-NEXT:    // }
    // PRT-AO-NEXT:    // ^----------OMP----------^
    // PRT-OA-NEXT:    // v----------OMP----------v
    // PRT-O-NEXT:     {
    // PRT-O-NEXT:       const int __clang_acc_num_workers__ = nw;
    // PRT-O-NEXT:       {{^ *}}#pragma omp target teams num_teams(1){{$}}
    // PRT-O-NEXT:       {{^ *}}#pragma omp distribute parallel for num_threads(__clang_acc_num_workers__){{$}}
    // PRT-O-NEXT:       for ({{.*}}) {
    // PRT-O-NEXT:         {{^ *}}#pragma omp simd simdlen(1){{$}}
    // PRT-O-NEXT:         for ({{.*}}) {
    // PRT-O-NEXT:           printf
    // PRT-O-NEXT:         }
    // PRT-O-NEXT:       }
    // PRT-O-NEXT:     }
    // PRT-OA-NEXT:    // ---------OMP<-ACC--------
    // PRT-OA-NEXT:    // #pragma acc parallel loop worker num_gangs(1) num_workers(nw) vector_length(1){{$}}
    // PRT-OA-NEXT:    // for ({{.*}}) {
    // PRT-OA-NEXT:    //   #pragma acc loop vector{{$}}
    // PRT-OA-NEXT:    //   for ({{.*}}) {
    // PRT-OA-NEXT:    //     printf
    // PRT-OA-NEXT:    //   }
    // PRT-OA-NEXT:    // }
    // PRT-OA-NEXT:    // ^----------ACC----------^
    #pragma acc parallel loop worker num_gangs(1) num_workers(nw) vector_length(1)
    for (int i = 0; i < 2; ++i) {
      #pragma acc loop vector
      for (int j = 0; j < 4; ++j) {
        // EXE-DAG: 0, 0
        // EXE-DAG: 0, 1
        // EXE-DAG: 0, 2
        // EXE-DAG: 0, 3
        // EXE-DAG: 1, 0
        // EXE-DAG: 1, 1
        // EXE-DAG: 1, 2
        // EXE-DAG: 1, 3
        printf("%d, %d\n", i, j);
      }
    }
  } // PRT-NEXT: }

  //--------------------------------------------------
  // num_workers non-constant>1, side effects, used deeply nested
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCNumWorkersClause
    // DMP-NEXT:     CallExpr
    // DMP:            DeclRefExpr {{.*}} 'foo'
    // DMP-NEXT:   ACCVectorLengthClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     DeclStmt
    // DMP-NEXT:       VarDecl {{.*}} __clang_acc_num_workers__ 'const int'
    // DMP-NEXT:         CallExpr
    // DMP:                DeclRefExpr {{.*}} 'foo'
    // DMP-NEXT:     OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:       OMPFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:         DeclRefExpr {{.*}} '__clang_acc_num_workers__' 'const int'
    // DMP-NOT:        OMP
    // DMP:        CompoundStmt
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCSeqClause
    // DMP-NEXT:       impl: ForStmt
    // DMP:            ACCLoopDirective
    // DMP-NEXT:         ACCGangClause
    // DMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:         ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:         impl: OMPDistributeDirective
    // DMP-NEXT:           OMPSharedClause {{.*}} <implicit>
    // DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
    // DMP:              ACCLoopDirective
    // DMP-NEXT:           ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:           ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:             DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:           impl: ForStmt
    // DMP:                ACCLoopDirective
    // DMP-NEXT:             ACCWorkerClause
    // DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:               DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:               DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:               DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:             impl: OMPParallelForDirective
    // DMP-NEXT:               OMPNum_threadsClause
    // DMP-NEXT:                 DeclRefExpr {{.*}} '__clang_acc_num_workers__' 'const int'
    // DMP-NEXT:               OMPSharedClause {{.*}}
    // DMP-NEXT:                 DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:                 DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:                 DeclRefExpr {{.*}} 'k' 'int'
    // DMP:                  ACCLoopDirective
    // DMP-NEXT:               ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:               ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:                 DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:                 DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:                 DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:                 DeclRefExpr {{.*}} 'l' 'int'
    // DMP-NEXT:               impl: ForStmt
    // DMP:                    ACCLoopDirective
    // DMP-NEXT:                 ACCVectorClause
    // DMP-NEXT:                 ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:                 ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:                   DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:                   DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:                   DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:                   DeclRefExpr {{.*}} 'l' 'int'
    // DMP-NEXT:                   DeclRefExpr {{.*}} 'm' 'int'
    // DMP-NEXT:                 impl: OMPSimdDirective
    // DMP-NEXT:                   OMPSimdlenClause
    // DMP-NEXT:                     ConstantExpr {{.*}} 'int'
    // DMP-NEXT:                       value: Int 1
    // DMP-NEXT:                       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:                   OMPSharedClause {{.*}} <implicit>
    // DMP-NEXT:                     DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:                     DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:                     DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:                     DeclRefExpr {{.*}} 'l' 'int'
    // DMP-NEXT:                     DeclRefExpr {{.*}} 'm' 'int'
    // DMP-NOT:                    OMP
    // DMP:            CallExpr
    //
    // PRT-NOACC-NEXT: {
    // PRT-NOACC-NEXT:   for (int i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:     for (int j = 0; j < 1; ++j) {
    // PRT-NOACC-NEXT:       for (int k = 0; k < 2; ++k) {
    // PRT-NOACC-NEXT:         for (int l = 0; l < 2; ++l) {
    // PRT-NOACC-NEXT:           for (int m = 0; m < 2; ++m) {
    // PRT-NOACC-NEXT:             for (int n = 0; n < 1; ++n)
    // PRT-NOACC-NEXT:               printf
    // PRT-NOACC-NEXT:           }
    // PRT-NOACC-NEXT:         }
    // PRT-NOACC-NEXT:       }
    // PRT-NOACC-NEXT:     }
    // PRT-NOACC-NEXT:   }
    // PRT-NOACC-NEXT: }
    // PRT-AO-NEXT:    // v----------ACC----------v
    // PRT-A-NEXT:     {{^ *}}#pragma acc parallel num_gangs(1) num_workers(foo()) vector_length(1){{$}}
    // PRT-A-NEXT:     {
    // PRT-A-NEXT:       {{^ *}}#pragma acc loop seq{{$}}
    // PRT-A-NEXT:       for (int i = 0; i < 2; ++i) {
    // PRT-A-NEXT:         {{^ *}}#pragma acc loop gang{{$}}
    // PRT-A-NEXT:         for (int j = 0; j < 1; ++j) {
    // PRT-A-NEXT:           {{^ *}}#pragma acc loop{{$}}
    // PRT-A-NEXT:           for (int k = 0; k < 2; ++k) {
    // PRT-A-NEXT:             {{^ *}}#pragma acc loop worker{{$}}
    // PRT-A-NEXT:             for (int l = 0; l < 2; ++l) {
    // PRT-A-NEXT:               {{^ *}}#pragma acc loop{{$}}
    // PRT-A-NEXT:               for (int m = 0; m < 2; ++m) {
    // PRT-A-NEXT:                 {{^ *}}#pragma acc loop vector{{$}}
    // PRT-A-NEXT:                 for (int n = 0; n < 1; ++n)
    // PRT-A-NEXT:                   printf
    // PRT-A-NEXT:               }
    // PRT-A-NEXT:             }
    // PRT-A-NEXT:           }
    // PRT-A-NEXT:         }
    // PRT-A-NEXT:       }
    // PRT-A-NEXT:     }
    // PRT-AO-NEXT:    // ---------ACC->OMP--------
    // PRT-AO-NEXT:    // {
    // PRT-AO-NEXT:    //   const int __clang_acc_num_workers__ = foo();
    // PRT-AO-NEXT:    //   #pragma omp target teams num_teams(1){{$}}
    // PRT-AO-NEXT:    //   {
    // PRT-AO-NEXT:    //     for (int i = 0; i < 2; ++i) {
    // PRT-AO-NEXT:    //       #pragma omp distribute{{$}}
    // PRT-AO-NEXT:    //       for (int j = 0; j < 1; ++j) {
    // PRT-AO-NEXT:    //         for (int k = 0; k < 2; ++k) {
    // PRT-AO-NEXT:    //           #pragma omp parallel for num_threads(__clang_acc_num_workers__) shared(i,j,k){{$}}
    // PRT-AO-NEXT:    //           for (int l = 0; l < 2; ++l) {
    // PRT-AO-NEXT:    //             for (int m = 0; m < 2; ++m) {
    // PRT-AO-NEXT:    //               #pragma omp simd simdlen(1){{$}}
    // PRT-AO-NEXT:    //               for (int n = 0; n < 1; ++n)
    // PRT-AO-NEXT:    //                 printf
    // PRT-AO-NEXT:    //             }
    // PRT-AO-NEXT:    //           }
    // PRT-AO-NEXT:    //         }
    // PRT-AO-NEXT:    //       }
    // PRT-AO-NEXT:    //     }
    // PRT-AO-NEXT:    //   }
    // PRT-AO-NEXT:    // }
    // PRT-AO-NEXT:    // ^----------OMP----------^
    // PRT-OA-NEXT:    // v----------OMP----------v
    // PRT-O-NEXT:     {
    // PRT-O-NEXT:       const int __clang_acc_num_workers__ = foo();
    // PRT-O-NEXT:       {{^ *}}#pragma omp target teams num_teams(1){{$}}
    // PRT-O-NEXT:       {
    // PRT-O-NEXT:         for (int i = 0; i < 2; ++i) {
    // PRT-O-NEXT:           {{^ *}}#pragma omp distribute{{$}}
    // PRT-O-NEXT:           for (int j = 0; j < 1; ++j) {
    // PRT-O-NEXT:             for (int k = 0; k < 2; ++k) {
    // PRT-O-NEXT:               {{^ *}}#pragma omp parallel for num_threads(__clang_acc_num_workers__) shared(i,j,k){{$}}
    // PRT-O-NEXT:               for (int l = 0; l < 2; ++l) {
    // PRT-O-NEXT:                 for (int m = 0; m < 2; ++m) {
    // PRT-O-NEXT:                   {{^ *}}#pragma omp simd simdlen(1){{$}}
    // PRT-O-NEXT:                   for (int n = 0; n < 1; ++n)
    // PRT-O-NEXT:                     printf
    // PRT-O-NEXT:                 }
    // PRT-O-NEXT:               }
    // PRT-O-NEXT:             }
    // PRT-O-NEXT:           }
    // PRT-O-NEXT:         }
    // PRT-O-NEXT:       }
    // PRT-O-NEXT:     }
    // PRT-OA-NEXT:    // ---------OMP<-ACC--------
    // PRT-OA-NEXT:    // #pragma acc parallel num_gangs(1) num_workers(foo()) vector_length(1){{$}}
    // PRT-OA-NEXT:    // {
    // PRT-OA-NEXT:    //   #pragma acc loop seq{{$}}
    // PRT-OA-NEXT:    //   for (int i = 0; i < 2; ++i) {
    // PRT-OA-NEXT:    //     #pragma acc loop gang{{$}}
    // PRT-OA-NEXT:    //     for (int j = 0; j < 1; ++j) {
    // PRT-OA-NEXT:    //       #pragma acc loop{{$}}
    // PRT-OA-NEXT:    //       for (int k = 0; k < 2; ++k) {
    // PRT-OA-NEXT:    //         #pragma acc loop worker{{$}}
    // PRT-OA-NEXT:    //         for (int l = 0; l < 2; ++l) {
    // PRT-OA-NEXT:    //           #pragma acc loop{{$}}
    // PRT-OA-NEXT:    //           for (int m = 0; m < 2; ++m) {
    // PRT-OA-NEXT:    //             #pragma acc loop vector{{$}}
    // PRT-OA-NEXT:    //             for (int n = 0; n < 1; ++n)
    // PRT-OA-NEXT:    //               printf
    // PRT-OA-NEXT:    //           }
    // PRT-OA-NEXT:    //         }
    // PRT-OA-NEXT:    //       }
    // PRT-OA-NEXT:    //     }
    // PRT-OA-NEXT:    //   }
    // PRT-OA-NEXT:    // }
    // PRT-OA-NEXT:    // ^----------ACC----------^
    //
    // EXE-NEXT: foo
    #pragma acc parallel num_gangs(1) num_workers(foo()) vector_length(1)
    {
      #pragma acc loop seq
      for (int i = 0; i < 2; ++i) {
        #pragma acc loop gang
        for (int j = 0; j < 1; ++j) {
          #pragma acc loop
          for (int k = 0; k < 2; ++k) {
            #pragma acc loop worker
            for (int l = 0; l < 2; ++l) {
              #pragma acc loop
              for (int m = 0; m < 2; ++m) {
                #pragma acc loop vector
                for (int n = 0; n < 1; ++n)
                  // not sequential
                  // EXE-DAG: 0, 0, 0, 0, 0, 0
                  // EXE-DAG: 0, 0, 0, 0, 1, 0
                  // EXE-DAG: 0, 0, 0, 1, 0, 0
                  // EXE-DAG: 0, 0, 0, 1, 1, 0
                  // EXE-DAG: 0, 0, 1, 0, 0, 0
                  // EXE-DAG: 0, 0, 1, 0, 1, 0
                  // EXE-DAG: 0, 0, 1, 1, 0, 0
                  // EXE-DAG: 0, 0, 1, 1, 1, 0
                  // EXE-DAG: 1, 0, 0, 0, 0, 0
                  // EXE-DAG: 1, 0, 0, 0, 1, 0
                  // EXE-DAG: 1, 0, 0, 1, 0, 0
                  // EXE-DAG: 1, 0, 0, 1, 1, 0
                  // EXE-DAG: 1, 0, 1, 0, 0, 0
                  // EXE-DAG: 1, 0, 1, 0, 1, 0
                  // EXE-DAG: 1, 0, 1, 1, 0, 0
                  // EXE-DAG: 1, 0, 1, 1, 1, 0
                  printf("%d, %d, %d, %d, %d, %d\n", i, j, k, l, m, n);
              }
            }
          }
        }
      }
    }
  } // PRT-NEXT: }

  //--------------------------------------------------
  // vector_length non-constant>1, no side effects, would be used but discarded
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int vl = 2;
    int vl = 2;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCNumWorkersClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCVectorLengthClause
    // DMP:          DeclRefExpr {{.*}} 'vl' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NOT:      OMP
    //
    // PRT-PRE-NEXT: /* expected{{-warning.*}} */
    // PRT-A-NEXT:   {{^ *}}#pragma acc parallel num_gangs(1) num_workers(1) vector_length(vl){{$}}
    // PRT-AO-NEXT:  {{^ *}}// #pragma omp target teams num_teams(1){{$}}
    // PRT-O-NEXT:   {{^ *}}#pragma omp target teams num_teams(1){{$}}
    // PRT-OA-NEXT:  {{^ *}}// #pragma acc parallel num_gangs(1) num_workers(1) vector_length(vl){{$}}
    /* expected-warning@+1 {{'vector_length' ignored because argument is not an integer constant expression}} */
    #pragma acc parallel num_gangs(1) num_workers(1) vector_length(vl)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeSimdDirective
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop vector{{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute simd{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector{{$}}
      #pragma acc loop vector
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 8; ++i) {
        // sequential as partitioned only across 1 vector lane
        // DMP: CallExpr
        // PRT-NEXT: printf
        // EXE-NEXT: 0
        // EXE-NEXT: 1
        // EXE-NEXT: 2
        // EXE-NEXT: 3
        // EXE-NEXT: 4
        // EXE-NEXT: 5
        // EXE-NEXT: 6
        // EXE-NEXT: 7
        printf("%d\n", i);
      } // PRT-NEXT: }
    } // PRT-NEXT: }
  } // PRT-NEXT: }

  //--------------------------------------------------
  // vector_length non-constant>1, side effects, would be used but ignored
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int vl = 1;
    int vl = 1;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCNumWorkersClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCVectorLengthClause
    // DMP-NEXT:     UnaryOperator {{.*}} 'int' prefix '++'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'vl' 'int'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNumGangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:     ACCNumWorkersClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:     ACCVectorLengthClause
    // DMP-NEXT:       UnaryOperator {{.*}} 'int' prefix '++'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'vl' 'int'
    // DMP-NEXT:     impl: CompoundStmt
    // DMP-NEXT:       CStyleCastExpr {{.*}} 'void' <ToVoid>
    // DMP-NEXT:         UnaryOperator {{.*}} 'int' prefix '++'
    // DMP-NEXT:           DeclRefExpr {{.*}} 'vl' 'int'
    // DMP-NEXT:       OMPTargetTeamsDirective
    // DMP-NEXT:         OMPNum_teamsClause
    // DMP-NEXT:           IntegerLiteral {{.*}} 'int' 1
    // DMP-NOT:          OMP
    // DMP:              CapturedStmt
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCVectorClause
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeSimdDirective
    // DMP-NOT:          OMP
    // DMP:              ForStmt
    // DMP:                CallExpr
    //
    // PRT-PRE-NEXT: /* expected{{-warning.*}} */
    //
    // PRT-NOACC-NEXT: for (int i = 0; i < 8; ++i) {
    // PRT-NOACC-NEXT:   printf
    // PRT-NOACC-NEXT: }
    //
    // PRT-AO-NEXT:    // v----------ACC----------v
    // PRT-A-NEXT:     {{^ *}}#pragma acc parallel loop vector num_gangs(1) num_workers(1) vector_length(++vl){{$}}
    // PRT-A-NEXT:     for (int i = 0; i < 8; ++i) {
    // PRT-A-NEXT:       printf
    // PRT-A-NEXT:     }
    // PRT-AO-NEXT:    // ---------ACC->OMP--------
    // PRT-AO-NEXT:    // {
    // PRT-AO-NEXT:    //   (void)++vl;
    // PRT-AO-NEXT:    //   #pragma omp target teams num_teams(1){{$}}
    // PRT-AO-NEXT:    //   #pragma omp distribute simd{{$}}
    // PRT-AO-NEXT:    //   for (int i = 0; i < 8; ++i) {
    // PRT-AO-NEXT:    //     printf
    // PRT-AO-NEXT:    //   }
    // PRT-AO-NEXT:    // }
    // PRT-AO-NEXT:    // ^----------OMP----------^
    //
    // PRT-OA-NEXT:    // v----------OMP----------v
    // PRT-O-NEXT:     {
    // PRT-O-NEXT:       (void)++vl;
    // PRT-O-NEXT:       {{^ *}}#pragma omp target teams num_teams(1){{$}}
    // PRT-O-NEXT:       {{^ *}}#pragma omp distribute simd{{$}}
    // PRT-O-NEXT:       for (int i = 0; i < 8; ++i) {
    // PRT-O-NEXT:         printf
    // PRT-O-NEXT:       }
    // PRT-O-NEXT:     }
    // PRT-OA-NEXT:    // ---------OMP<-ACC--------
    // PRT-OA-NEXT:    // #pragma acc parallel loop vector num_gangs(1) num_workers(1) vector_length(++vl){{$}}
    // PRT-OA-NEXT:    // for (int i = 0; i < 8; ++i) {
    // PRT-OA-NEXT:    //   printf
    // PRT-OA-NEXT:    // }
    // PRT-OA-NEXT:    // ^----------ACC----------^
    //
    /* expected-warning@+1 {{'vector_length' ignored because argument is not an integer constant expression}} */
    #pragma acc parallel loop vector num_gangs(1) num_workers(1) vector_length(++vl)
    for (int i = 0; i < 8; ++i) {
      // sequential as partitioned only across 1 vector lane
      // EXE-NEXT: 0
      // EXE-NEXT: 1
      // EXE-NEXT: 2
      // EXE-NEXT: 3
      // EXE-NEXT: 4
      // EXE-NEXT: 5
      // EXE-NEXT: 6
      // EXE-NEXT: 7
      printf("%d\n", i);
    }
  } // PRT-NEXT: }

  //--------------------------------------------------
  // vector_length non-constant>1, side effects, unused
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int vl = 3;
    int vl = 3;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCNumWorkersClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCVectorLengthClause
    // DMP-NEXT:     UnaryOperator {{.*}} 'int' postfix '--'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'vl' 'int'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     CStyleCastExpr {{.*}} 'void' <ToVoid>
    // DMP-NEXT:       UnaryOperator {{.*}} 'int' postfix '--'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'vl' 'int'
    // DMP-NEXT:     OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP-NOT:        OMP
    // DMP:            CapturedStmt
    // DMP:        ACCLoopDirective
    // DMP-NEXT:     ACCWorkerClause
    // DMP-NEXT:     ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:     ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:     impl: OMPDistributeParallelForDirective
    // DMP-NEXT:       OMPNum_threadsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP:            ForStmt
    // DMP:              CallExpr
    //
    // PRT-PRE-NEXT: /* expected{{-warning.*}} */
    //
    // PRT-NOACC-NEXT: for (int i = 0; i < 8; ++i) {
    // PRT-NOACC-NEXT:   printf
    // PRT-NOACC-NEXT: }
    //
    // PRT-AO-NEXT:    // v----------ACC----------v
    // PRT-A-NEXT:     {{^ *}}#pragma acc parallel num_gangs(1) num_workers(1) vector_length(vl--){{$}}
    // PRT-A-NEXT:     {{^ *}}#pragma acc loop worker{{$}}
    // PRT-A-NEXT:     for (int i = 0; i < 8; ++i) {
    // PRT-A-NEXT:       printf
    // PRT-A-NEXT:     }
    // PRT-AO-NEXT:    // ---------ACC->OMP--------
    // PRT-AO-NEXT:    // {
    // PRT-AO-NEXT:    //   (void)vl--;
    // PRT-AO-NEXT:    //   #pragma omp target teams num_teams(1){{$}}
    // PRT-AO-NEXT:    //   #pragma omp distribute parallel for num_threads(1){{$}}
    // PRT-AO-NEXT:    //   for (int i = 0; i < 8; ++i) {
    // PRT-AO-NEXT:    //     printf
    // PRT-AO-NEXT:    //   }
    // PRT-AO-NEXT:    // }
    // PRT-AO-NEXT:    // ^----------OMP----------^
    //
    // PRT-OA-NEXT:    // v----------OMP----------v
    // PRT-O-NEXT:     {
    // PRT-O-NEXT:       (void)vl--;
    // PRT-O-NEXT:       {{^ *}}#pragma omp target teams num_teams(1){{$}}
    // PRT-O-NEXT:       {{^ *}}#pragma omp distribute parallel for num_threads(1){{$}}
    // PRT-O-NEXT:       for (int i = 0; i < 8; ++i) {
    // PRT-O-NEXT:         printf
    // PRT-O-NEXT:       }
    // PRT-O-NEXT:     }
    // PRT-OA-NEXT:    // ---------OMP<-ACC--------
    // PRT-OA-NEXT:    // #pragma acc parallel num_gangs(1) num_workers(1) vector_length(vl--){{$}}
    // PRT-OA-NEXT:    // #pragma acc loop worker{{$}}
    // PRT-OA-NEXT:    // for (int i = 0; i < 8; ++i) {
    // PRT-OA-NEXT:    //   printf
    // PRT-OA-NEXT:    // }
    // PRT-OA-NEXT:    // ^----------ACC----------^
    //
    /* expected-warning@+1 {{'vector_length' ignored because argument is not an integer constant expression}} */
    #pragma acc parallel num_gangs(1) num_workers(1) vector_length(vl--)
    #pragma acc loop worker
    for (int i = 0; i < 8; ++i) {
      // sequential as partitioned only across 1 worker
      // EXE-NEXT: 0
      // EXE-NEXT: 1
      // EXE-NEXT: 2
      // EXE-NEXT: 3
      // EXE-NEXT: 4
      // EXE-NEXT: 5
      // EXE-NEXT: 6
      // EXE-NEXT: 7
      printf("%d\n", i);
    }
  } // PRT-NEXT: }

  //--------------------------------------------------
  // num_workers non-constant>1, side effects, unused
  // vector_length non-constant>1, side effects, unused
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCNumWorkersClause
    // DMP-NEXT:     CallExpr
    // DMP:            DeclRefExpr {{.*}} 'foo'
    // DMP-NEXT:   ACCVectorLengthClause
    // DMP-NEXT:     CallExpr
    // DMP:            DeclRefExpr {{.*}} 'bar'
    // DMP-NEXT:   impl: CompoundStmt
    // DMP-NEXT:     CStyleCastExpr {{.*}} 'void' <ToVoid>
    // DMP-NEXT:       CallExpr
    // DMP:              DeclRefExpr {{.*}} 'foo'
    // DMP-NEXT:     CStyleCastExpr {{.*}} 'void' <ToVoid>
    // DMP-NEXT:       CallExpr
    // DMP:              DeclRefExpr {{.*}} 'bar'
    // DMP-NEXT:     OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP-NOT:        OMP
    // DMP:            CapturedStmt
    // DMP:        ACCLoopDirective
    // DMP-NEXT:     ACCGangClause
    // DMP-NEXT:     ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:     impl: OMPDistributeDirective
    // DMP:            ForStmt
    // DMP:              CallExpr
    //
    // PRT-PRE-NEXT: /* expected{{-warning.*}} */
    //
    // PRT-NOACC-NEXT: for (int i = 0; i < 8; ++i) {
    // PRT-NOACC-NEXT:   printf
    // PRT-NOACC-NEXT: }
    //
    // PRT-AO-NEXT:    // v----------ACC----------v
    // PRT-A-NEXT:     {{^ *}}#pragma acc parallel num_gangs(1) num_workers(foo()) vector_length(bar()){{$}}
    // PRT-A-NEXT:     {{^ *}}#pragma acc loop gang{{$}}
    // PRT-A-NEXT:     for (int i = 0; i < 8; ++i) {
    // PRT-A-NEXT:       printf
    // PRT-A-NEXT:     }
    // PRT-AO-NEXT:    // ---------ACC->OMP--------
    // PRT-AO-NEXT:    // {
    // PRT-AO-NEXT:    //   (void)foo();
    // PRT-AO-NEXT:    //   (void)bar();
    // PRT-AO-NEXT:    //   #pragma omp target teams num_teams(1){{$}}
    // PRT-AO-NEXT:    //   #pragma omp distribute{{$}}
    // PRT-AO-NEXT:    //   for (int i = 0; i < 8; ++i) {
    // PRT-AO-NEXT:    //     printf
    // PRT-AO-NEXT:    //   }
    // PRT-AO-NEXT:    // }
    // PRT-AO-NEXT:    // ^----------OMP----------^
    //
    // PRT-OA-NEXT:    // v----------OMP----------v
    // PRT-O-NEXT:     {
    // PRT-O-NEXT:       (void)foo();
    // PRT-O-NEXT:       (void)bar();
    // PRT-O-NEXT:       {{^ *}}#pragma omp target teams num_teams(1){{$}}
    // PRT-O-NEXT:       {{^ *}}#pragma omp distribute{{$}}
    // PRT-O-NEXT:       for (int i = 0; i < 8; ++i) {
    // PRT-O-NEXT:         printf
    // PRT-O-NEXT:       }
    // PRT-O-NEXT:     }
    // PRT-OA-NEXT:    // ---------OMP<-ACC--------
    // PRT-OA-NEXT:    // #pragma acc parallel num_gangs(1) num_workers(foo()) vector_length(bar()){{$}}
    // PRT-OA-NEXT:    // #pragma acc loop gang{{$}}
    // PRT-OA-NEXT:    // for (int i = 0; i < 8; ++i) {
    // PRT-OA-NEXT:    //   printf
    // PRT-OA-NEXT:    // }
    // PRT-OA-NEXT:    // ^----------ACC----------^
    //
    // EXE-NEXT: foo
    // EXE-NEXT: bar
    //
    /* expected-warning@+1 {{'vector_length' ignored because argument is not an integer constant expression}} */
    #pragma acc parallel num_gangs(1) num_workers(foo()) vector_length(bar())
    #pragma acc loop gang
    for (int i = 0; i < 8; ++i) {
      // sequential as partitioned only across 1 gang
      // EXE-NEXT: 0
      // EXE-NEXT: 1
      // EXE-NEXT: 2
      // EXE-NEXT: 3
      // EXE-NEXT: 4
      // EXE-NEXT: 5
      // EXE-NEXT: 6
      // EXE-NEXT: 7
      printf("%d\n", i);
    }
  } // PRT-NEXT: }

  // PRT-NEXT: return 0;
  return 0;
} // PRT-NEXT: }
// EXE-NOT: {{.}}
