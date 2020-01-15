// Check reduction clause on "acc loop" and on "acc parallel loop".
//
// For aliased clauses, we arbitrarily cycle through the aliases throughout
// this test to give confidence they all are handled equivalently in sema and
// codegen.  We do not attempt to check every alias in every scenario as that
// would make the test much slower and more difficult to maintain.

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
// TODO: If lit were to support %for inside a %data, we could iterate prt-opts
// within prt-args after the first prt-args iteration, significantly shortening
// the prt-args definition.
//
// Strip comments and blank lines so checking -fopenacc-print output is easier.
// RUN: echo "// expected""-no-diagnostics" > %t-acc.c
// RUN: grep -v '^ *\(//.*\)\?$' %s | sed 's,//.*,,' >> %t-acc.c
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
// RUN:   (prt=-fopenacc-print=acc                          prt-chk=PRT-A,PRT,PRT-SRC)
// RUN:   (prt=-fopenacc-print=omp                          prt-chk=PRT-O,PRT,PRT-SRC)
// RUN:   (prt=-fopenacc-print=acc-omp                      prt-chk=PRT-A,PRT-AO,PRT,PRT-SRC)
// RUN:   (prt=-fopenacc-print=omp-acc                      prt-chk=PRT-O,PRT-OA,PRT,PRT-SRC)
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

// For some Linux platforms, -latomic is required for OpenMP support for
// reductions on complex types.

// Can we print the OpenMP source code, compile, and run it successfully?
//
// RUN: %for prt-opts {
// RUN:   %clang -Xclang -verify %[prt-opt]=omp %s > %t-omp.c
// RUN:   echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN:   %clang -Xclang -verify -fopenmp %fopenmp-version -o %t %t-omp.c \
// RUN:          %libatomic
// RUN:   %t 2 2>&1 | FileCheck -check-prefixes=EXE,EXE-TGT-HOST %s
// RUN: }

// Check execution with normal compilation.
//
// RUN: %data tgts {
// RUN:   (run-if=                tgt=HOST    tgt-cflags=                                    )
// RUN:   (run-if=%run-if-x86_64  tgt=X86_64  tgt-cflags=-fopenmp-targets=%run-x86_64-triple )
// RUN:   (run-if=%run-if-nvptx64 tgt=NVPTX64 tgt-cflags=-fopenmp-targets=%run-nvptx64-triple)
// RUN: }
// RUN: %for tgts {
// RUN:   %[run-if] %clang -Xclang -verify -fopenacc %s -o %t %libatomic \
// RUN:                    %[tgt-cflags] -DTGT_%[tgt]_EXE
// RUN:   %[run-if] %t 2 > %t.out 2>&1
// RUN:   %[run-if] FileCheck -input-file %t.out %s \
// RUN:                       -check-prefixes=EXE,EXE-TGT-%[tgt]
// RUN: }

// END.

// expected-no-diagnostics

#include <complex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// PRT: int main() {
int main() {
  // PRT-NEXT: printf
  // EXE: start
  printf("start\n");

  //--------------------------------------------------
  // Explicit seq.
  //--------------------------------------------------

  // Reduction var is private, explicitly or implicitly.

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out0 = 10;
    // PRT-NEXT: int out1 = 20;
    // PRT-NEXT: int out2 = 30;
    // PRT-NEXT: int out3 = 40;
    int out0 = 10;
    int out1 = 20;
    int out2 = 30;
    int out3 = 40;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} '*'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'int'
    // DMP-NEXT:   ACCFirstprivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'int'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out3' 'int'
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out3' 'int'
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'int'
    // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'int'
    // DMP-NEXT:     OMPFirstprivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'int'
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out3' 'int'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'int'
    // DMP-NEXT:     OMPFirstprivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) reduction(*: out0) firstprivate(out2) private(out3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(*: out0) firstprivate(out2) private(out3) map(tofrom: out0) firstprivate(out1){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(*: out0) firstprivate(out2) private(out3) map(tofrom: out0) firstprivate(out1){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) reduction(*: out0) firstprivate(out2) private(out3){{$}}
    #pragma acc parallel num_gangs(2) reduction(*: out0) firstprivate(out2) private(out3)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: float in = 10;
      float in = 10;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCSeqClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '*'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out3' 'int'
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'float'
      // DMP-NEXT:   impl: ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop seq reduction(*: out0,out1,out2,out3) reduction(+: in)
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:  {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq reduction(*: out0,out1,out2,out3) reduction(+: in) // discarded in OpenMP translation{{$}}
      #pragma acc loop seq reduction(*: out0,out1,out2,out3) reduction(+: in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP: CompoundAssignOperator {{.*}} 'int' '*='
        // PRT-NEXT: out0 *= 2;
        out0 *= 2;
        // DMP: CompoundAssignOperator {{.*}} 'int' '*='
        // PRT-NEXT: out1 *= 2;
        out1 *= 2;
        // DMP: CompoundAssignOperator {{.*}} 'int' '*='
        // PRT-NEXT: out2 *= 2;
        out2 *= 2;
        // DMP: CompoundAssignOperator {{.*}} 'int' '*='
        // PRT-NEXT: out3 *= 2;
        out3 *= 2;
        // DMP: CompoundAssignOperator {{.*}} 'float' '+='
        // PRT-NEXT: in += 2;
        in += 2;
      } // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-DAG: out1 = 80
      // EXE-DAG: out1 = 80
      printf("out1 = %d\n", out1);
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-DAG: out2 = 120
      // EXE-DAG: out2 = 120
      printf("out2 = %d\n", out2);
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-DAG: in = 14.0
      // EXE-DAG: in = 14.0
      printf("in = %.1f\n", in);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out0 = 160
    printf("out0 = %d\n", out0);
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out1 = 20
    printf("out1 = %d\n", out1);
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out2 = 30
    printf("out2 = %d\n", out2);
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out3 = 40
    printf("out3 = %d\n", out3);
  } // PRT-NEXT: }

  // Reduction var is not private due to explicit copy/copyin/copyout clause.

  // PRT-NEXT: {
  {
    // PRT-NEXT: double out0 =
    // PRT-NEXT: double out1 =
    // PRT-NEXT: double out2 =
    double out0 = 0.3;
    double out1 = 1.4;
    double out2 = 2.5;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:   ACCCopyinClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:   ACCCopyoutClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'double'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) copy(out0) copyin(out1) copyout(out2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: out0) map(to: out1) map(from: out2) reduction(+: out0) reduction(+: out1) reduction(+: out2){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: out0) map(to: out1) map(from: out2) reduction(+: out0) reduction(+: out1) reduction(+: out2){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) copy(out0) copyin(out1) copyout(out2){{$}}
    #pragma acc parallel num_gangs(2) copy(out0) copyin(out1) copyout(out2)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // Make sure accessing the variables before the reduction doesn't
      // produce a conflicting implicit firstprivate clause.
      // DMP: CallExpr
      // DMP: CallExpr
      // DMP: CallExpr
      // PRT-NEXT: printf
      // PRT-NEXT: printf
      // PRT-NEXT: printf
      // EXE-DAG: out0 init = 0.0{{$}}
      // EXE-DAG: out0 init = 0.0{{$}}
      // EXE-DAG: out1 init = 0.0{{$}}
      // EXE-DAG: out1 init = 0.0{{$}}
      // EXE-DAG: out2 init = 0.0{{$}}
      // EXE-DAG: out2 init = 0.0{{$}}
      printf("out0 init = %.1f\n", out0);
      printf("out1 init = %.1f\n", out1);
      printf("out2 init = %.1f\n", out2);
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCSeqClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
      // DMP-NEXT:   impl: ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop seq reduction(+: out0,out1,out2)
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:  {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq reduction(+: out0,out1,out2) // discarded in OpenMP translation{{$}}
      #pragma acc loop seq reduction(+: out0,out1,out2)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 4; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // DMP: CompoundAssignOperator {{.*}} '+='
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out0 +=
        // PRT-NEXT: out1 +=
        // PRT-NEXT: out2 +=
        out0 += -1.1;
        out1 += -1.1;
        out2 += -1.1;
      } // PRT-NEXT: }
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // EXE-NEXT: out0 = -8.5
    // EXE-TGT-HOST-NEXT: out1 = -7.4
    // EXE-TGT-HOST-NEXT: out2 = -6.3
    // EXE-TGT-X86_64-NEXT: out1 = 1.4
    // EXE-TGT-X86_64-NEXT: out2 =
    // EXE-TGT-NVPTX64-NEXT: out1 = 1.4
    // EXE-TGT-NVPTX64-NEXT: out2 =
    printf("out0 = %.1f\n", out0);
    printf("out1 = %.1f\n", out1);
    printf("out2 = %.1f\n", out2);
  } // PRT-NEXT: }

  // Reduction var is not private due to copy clause implied by reduction on
  // combined construct.

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 10;
    int out = 10;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCSeqClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '*'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNum_gangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     ACCReductionClause {{.*}} <implicit> '*'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCSeqClause
    // DMP-NEXT:       ACCReductionClause {{.*}} '*'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       impl: ForStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) seq reduction(*: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: out) reduction(*: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: out) reduction(*: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) seq reduction(*: out){{$}}
    //
    // PRT-NEXT: for ({{.*}}) {
    #pragma acc parallel loop num_gangs(2) seq reduction(*: out)
    for (int i = 0; i < 2; ++i) {
      // DMP: CompoundAssignOperator {{.*}} 'int' '*='
      // PRT-NEXT: out *= 2;
      out *= 2;
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 160
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Implicit independent, implicit gang partitioning.
  //--------------------------------------------------

  // acc parallel has reduction already.

  // PRT-NEXT: {
  {
    // PRT-NEXT: enum E {{{[[:space:]]*}}
    // PRT-SAME:   E0,{{[[:space:]]*}}
    // PRT-SAME:   E1{{[[:space:]]*}}
    // PRT-SAME: } out = E0;
    enum E {E0, E1} out = E0;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} 'max'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'enum E'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) reduction(max: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(max: out) map(tofrom: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(max: out) map(tofrom: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) reduction(max: out){{$}}
    #pragma acc parallel num_gangs(2) reduction(max: out)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: int arr[] = {10000, 1};
      // PRT-NEXT: int *in = arr;
      int arr[] = {10000, 1};
      int *in = arr;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCReductionClause {{.*}} 'max'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'enum E'
      // DMP-NEXT:   ACCReductionClause {{.*}} 'min'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'int *'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'arr' 'int [2]'
      // DMP-NEXT:   ACCGangClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'arr' 'int [2]'
      // DMP:          ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop reduction(max: out) reduction(min: in){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop reduction(max: out) reduction(min: in){{$}}
      #pragma acc loop reduction(max: out) reduction(min: in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP: ConditionalOperator
        // PRT-NEXT: out = E1 > out ? E1 : out;
        out = E1 > out ? E1 : out;
        // DMP: ConditionalOperator
        // PRT-NEXT: in = arr + 1 < in ? arr + 1 : in;
        in = arr + 1 < in ? arr + 1 : in;
      } // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-NEXT: in == arr: 1
      // EXE-NEXT: in == arr: 1
      printf("in == arr: %d\n", in == arr);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 1
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  // Only acc loop has gang reduction.

  // PRT-NEXT: {
  {
    // PRT-NEXT: enum E {{{[[:space:]]*}}
    // PRT-SAME:   E0,{{[[:space:]]*}}
    // PRT-SAME:   E1{{[[:space:]]*}}
    // PRT-SAME: } out = E0;
    enum E {E0, E1} out = E0;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> 'max'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP:        ACCLoopDirective
    // DMP-NEXT:     ACCReductionClause {{.*}} 'max'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:     ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:     ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:     impl: OMPDistributeDirective
    // DMP:            ForStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2)
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: out) reduction(max: out){{$}}
    // PRT-A-NEXT:  {{^ *}}#pragma acc loop reduction(max: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: out) reduction(max: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc loop reduction(max: out){{$}}
    //
    // PRT-NEXT: for ({{.*}}) {
    #pragma acc parallel num_gangs(2)
    #pragma acc loop reduction(max: out)
    for (int i = 0; i < 2; ++i) {
      // DMP: ConditionalOperator
      // PRT-NEXT: out = E1 > out ? E1 : out;
      out = E1 > out ? E1 : out;
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 1
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  // acc parallel loop with gang reduction.

  // PRT-NEXT: {
  {
    // PRT-NEXT: enum E {{{[[:space:]]*}}
    // PRT-SAME:   E0,{{[[:space:]]*}}
    // PRT-SAME:   E1{{[[:space:]]*}}
    // PRT-SAME: } out = E0;
    enum E {E0, E1} out = E0;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} 'max'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNum_gangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:     ACCReductionClause {{.*}} <implicit> 'max'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCReductionClause {{.*}} 'max'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCGangClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeDirective
    // DMP:              ForStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) reduction(max: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: out) reduction(max: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: out) reduction(max: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) reduction(max: out){{$}}
    //
    // PRT-NEXT: for ({{.*}}) {
    #pragma acc parallel loop num_gangs(2) reduction(max: out)
    for (int i = 0; i < 2; ++i) {
      // DMP: ConditionalOperator
      // PRT-NEXT: out = E1 > out ? E1 : out;
      out = E1 > out ? E1 : out;
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 1
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Explicit auto with partitioning.
  //--------------------------------------------------

  // Reduction var is private.
  // FIXME: OpenMP offloading for nvptx64 doesn't store bool correctly for
  // reductions.

// PRT-SRC-NEXT: #if !TGT_NVPTX64_EXE
#if !TGT_NVPTX64_EXE
  // PRT-NEXT: {
  {
    // PRT-NEXT: _Bool out0 = 1;
    // PRT-NEXT: _Bool out1 = 1;
    // PRT-NEXT: _Bool out2 = 1;
    // PRT-NEXT: _Bool out3 = 1;
    _Bool out0 = 1;
    _Bool out1 = 1;
    _Bool out2 = 1;
    _Bool out3 = 1;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} '&'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' '_Bool'
    // DMP-NEXT:   ACCFirstprivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' '_Bool'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out3' '_Bool'
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' '_Bool'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' '_Bool'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out3' '_Bool'
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' '_Bool'
    // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' '_Bool'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' '_Bool'
    // DMP-NEXT:     OMPFirstprivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' '_Bool'
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out3' '_Bool'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' '_Bool'
    // DMP-NEXT:     OMPFirstprivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' '_Bool'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) reduction(&: out0) firstprivate(out2) private(out3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(&: out0) firstprivate(out2) private(out3) map(tofrom: out0) firstprivate(out1){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(&: out0) firstprivate(out2) private(out3) map(tofrom: out0) firstprivate(out1){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) reduction(&: out0) firstprivate(out2) private(out3){{$}}
    #pragma acc parallel num_gangs(2) reduction(&: out0) firstprivate(out2) private(out3)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: short in = 3;
      short in = 3;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCAutoClause
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '&'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' '_Bool'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' '_Bool'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' '_Bool'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out3' '_Bool'
      // DMP-NEXT:   ACCReductionClause {{.*}} '|'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'short'
      // DMP-NEXT:   impl: ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop auto worker reduction(&: out0,out1,out2,out3) reduction(|: in)
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:  {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto worker reduction(&: out0,out1,out2,out3) reduction(|: in) // discarded in OpenMP translation{{$}}
      #pragma acc loop auto worker reduction(&: out0,out1,out2,out3) reduction(|: in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '_Bool' '&='
        // PRT-NEXT: out0 &= 0;
        out0 &= 0;
        // DMP: CompoundAssignOperator {{.*}} '_Bool' '&='
        // PRT-NEXT: out1 &= 0;
        out1 &= 0;
        // DMP: CompoundAssignOperator {{.*}} '_Bool' '&='
        // PRT-NEXT: out2 &= 0;
        out2 &= 0;
        // DMP: CompoundAssignOperator {{.*}} '_Bool' '&='
        // PRT-NEXT: out3 &= 0;
        out3 &= 0;
        // DMP: CompoundAssignOperator {{.*}} 'short' '|='
        // PRT-NEXT: in |= 10;
        in |= 10;
      } // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-TGT-HOST-DAG: out1 = 0
      // EXE-TGT-HOST-DAG: out1 = 0
      // EXE-TGT-X86_64-DAG: out1 = 0
      // EXE-TGT-X86_64-DAG: out1 = 0
      printf("out1 = %d\n", out1);
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-TGT-HOST-DAG: out2 = 0
      // EXE-TGT-HOST-DAG: out2 = 0
      // EXE-TGT-X86_64-DAG: out2 = 0
      // EXE-TGT-X86_64-DAG: out2 = 0
      printf("out2 = %d\n", out2);
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-TGT-HOST-DAG: in: 11
      // EXE-TGT-HOST-DAG: in: 11
      // EXE-TGT-X86_64-DAG: in: 11
      // EXE-TGT-X86_64-DAG: in: 11
      printf("in: %d\n", in);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-TGT-HOST-NEXT: out0 = 0
    // EXE-TGT-X86_64-NEXT: out0 = 0
    printf("out0 = %d\n", out0);
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-TGT-HOST-NEXT: out1 = 1
    // EXE-TGT-X86_64-NEXT: out1 = 1
    printf("out1 = %d\n", out1);
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-TGT-HOST-NEXT: out2 = 1
    // EXE-TGT-X86_64-NEXT: out2 = 1
    printf("out2 = %d\n", out2);
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-TGT-HOST-NEXT: out3 = 1
    // EXE-TGT-X86_64-NEXT: out3 = 1
    printf("out3 = %d\n", out3);
  } // PRT-NEXT: }
// PRT-SRC-NEXT: #endif
#endif

  // Reduction var is not private due to explicit copy/copyin/copyout clause.

  // PRT-NEXT: {
  {
    // PRT-NEXT: double out0 =
    // PRT-NEXT: double out1 =
    // PRT-NEXT: double out2 =
    double out0 = 0.3;
    double out1 = 1.4;
    double out2 = 2.5;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:   ACCCopyinClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:   ACCCopyoutClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'double'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) pcopy(out0) pcopyin(out1) pcopyout(out2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: out0) map(to: out1) map(from: out2) reduction(+: out0) reduction(+: out1) reduction(+: out2){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: out0) map(to: out1) map(from: out2) reduction(+: out0) reduction(+: out1) reduction(+: out2){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) pcopy(out0) pcopyin(out1) pcopyout(out2){{$}}
    #pragma acc parallel num_gangs(2) pcopy(out0) pcopyin(out1) pcopyout(out2)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // Make sure accessing the variables before the reduction doesn't
      // produce a conflicting implicit firstprivate clause.
      // DMP: CallExpr
      // DMP: CallExpr
      // DMP: CallExpr
      // PRT-NEXT: printf
      // PRT-NEXT: printf
      // PRT-NEXT: printf
      // EXE-DAG: out0 init = 0.0{{$}}
      // EXE-DAG: out0 init = 0.0{{$}}
      // EXE-DAG: out1 init = 0.0{{$}}
      // EXE-DAG: out1 init = 0.0{{$}}
      // EXE-DAG: out2 init = 0.0{{$}}
      // EXE-DAG: out2 init = 0.0{{$}}
      printf("out0 init = %.1f\n", out0);
      printf("out1 init = %.1f\n", out1);
      printf("out2 init = %.1f\n", out2);
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCAutoClause
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
      // DMP-NEXT:   impl: ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop auto worker vector reduction(+: out0,out1,out2)
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:  {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto worker vector reduction(+: out0,out1,out2) // discarded in OpenMP translation{{$}}
      #pragma acc loop auto worker vector reduction(+: out0,out1,out2)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 4; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // DMP: CompoundAssignOperator {{.*}} '+='
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out0 +=
        // PRT-NEXT: out1 +=
        // PRT-NEXT: out2 +=
        out0 += -1.1;
        out1 += -1.1;
        out2 += -1.1;
      } // PRT-NEXT: }
    } // PRT-NEXT: }
    // DMP: CallExpr
    // DMP: CallExpr
    // DMP: CallExpr
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // EXE-NEXT: out0 = -8.5
    // EXE-TGT-HOST-NEXT: out1 = -7.4
    // EXE-TGT-HOST-NEXT: out2 = -6.3
    // EXE-TGT-X86_64-NEXT: out1 = 1.4
    // EXE-TGT-X86_64-NEXT: out2 =
    // EXE-TGT-NVPTX64-NEXT: out1 = 1.4
    // EXE-TGT-NVPTX64-NEXT: out2 =
    printf("out0 = %.1f\n", out0);
    printf("out1 = %.1f\n", out1);
    printf("out2 = %.1f\n", out2);
  } // PRT-NEXT: }

  // Reduction var is not private due to copy clause implied by reduction on
  // combined construct.

// PRT-SRC-NEXT: #if !TGT_NVPTX64_EXE
#if !TGT_NVPTX64_EXE
  // PRT-NEXT: {
  {
    // PRT-NEXT: _Bool out = 1;
    _Bool out = 1;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCReductionClause {{.*}} '&'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' '_Bool'
    // DMP-NEXT:   ACCAutoClause
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNum_gangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' '_Bool'
    // DMP-NEXT:     ACCReductionClause {{.*}} <implicit> '&'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' '_Bool'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' '_Bool'
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' '_Bool'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCReductionClause {{.*}} '&'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' '_Bool'
    // DMP-NEXT:       ACCAutoClause
    // DMP-NEXT:       ACCWorkerClause
    // DMP-NEXT:       impl: ForStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop reduction(&: out) auto worker num_gangs(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: out) reduction(&: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: out) reduction(&: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop reduction(&: out) auto worker num_gangs(2){{$}}
    //
    // PRT-NEXT: for ({{.*}}) {
    #pragma acc parallel loop reduction(&: out) auto worker num_gangs(2)
    for (int i = 0; i < 2; ++i) {
      // DMP: CompoundAssignOperator {{.*}} '_Bool' '&='
      // PRT-NEXT: out &= 0;
      out &= 0;
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-TGT-HOST-NEXT: out = 0
    // EXE-TGT-X86_64-NEXT: out = 0
    printf("out = %d\n", out);
  } // PRT-NEXT: }
// PRT-SRC-NEXT: #endif
#endif

  //--------------------------------------------------
  // Worker partitioned.
  //--------------------------------------------------

  // Reduction var is private.

  // PRT-NEXT: {
  {
    // PRT-NEXT: _Complex double out0 = 2;
    // PRT-NEXT: _Complex double out1 = 3;
    // PRT-NEXT: _Complex double out2 = 4;
    // PRT-NEXT: _Complex double out3 = 5;
    _Complex double out0 = 2;
    _Complex double out1 = 3;
    _Complex double out2 = 4;
    _Complex double out3 = 5;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} '&&'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' '_Complex double'
    // DMP-NEXT:   ACCFirstprivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' '_Complex double'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out3' '_Complex double'
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' '_Complex double'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' '_Complex double'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out3' '_Complex double'
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' '_Complex double'
    // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' '_Complex double'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' '_Complex double'
    // DMP-NEXT:     OMPFirstprivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' '_Complex double'
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out3' '_Complex double'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' '_Complex double'
    // DMP-NEXT:     OMPFirstprivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' '_Complex double'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) reduction(&&: out0) firstprivate(out2) private(out3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(&&: out0) firstprivate(out2) private(out3) map(tofrom: out0) firstprivate(out1){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(&&: out0) firstprivate(out2) private(out3) map(tofrom: out0) firstprivate(out1){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) reduction(&&: out0) firstprivate(out2) private(out3){{$}}
    #pragma acc parallel num_gangs(2) reduction(&&: out0) firstprivate(out2) private(out3)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: double in = 0;
      double in = 0;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '&&'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' '_Complex double'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' '_Complex double'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' '_Complex double'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out3' '_Complex double'
      // DMP-NEXT:   ACCReductionClause {{.*}} '||'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'double'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPParallelForDirective
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' '_Complex double'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' '_Complex double'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' '_Complex double'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out3' '_Complex double'
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'in' 'double'
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker reduction(&&: out0,out1,out2,out3) reduction(||: in){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for reduction(&&: out0,out1,out2,out3) reduction(||: in){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for reduction(&&: out0,out1,out2,out3) reduction(||: in){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker reduction(&&: out0,out1,out2,out3) reduction(||: in){{$}}
      #pragma acc loop worker reduction(&&: out0,out1,out2,out3) reduction(||: in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP: BinaryOperator {{.*}} '&&'
        // PRT-NEXT: out0 = out0 && 0;
        out0 = out0 && 0;
        // DMP: BinaryOperator {{.*}} '&&'
        // PRT-NEXT: out1 = out1 && 0;
        out1 = out1 && 0;
        // DMP: BinaryOperator {{.*}} '&&'
        // PRT-NEXT: out2 = out2 && 0;
        out2 = out2 && 0;
        // DMP: BinaryOperator {{.*}} '&&'
        // PRT-NEXT: out3 = out3 && 0;
        out3 = out3 && 0;
        // DMP: BinaryOperator {{.*}} '||'
        // PRT-NEXT: in = in || 10;
        in = in || 10;
      } // PRT-NEXT: }
// PRT-SRC-NEXT: #if !TGT_NVPTX64_EXE
#if !TGT_NVPTX64_EXE
      // FIXME: OpenMP offloading for nvptx64 doesn't seem to support creal
      // and cimag.  The result is a runtime error:
      //
      //   Libomptarget fatal error 1: failure of target construct while
      //   offloading is mandatory
      // PRT-NEXT: printf
      // EXE-TGT-HOST-DAG: out1: 0.0 + 0.0i
      // EXE-TGT-HOST-DAG: out1: 0.0 + 0.0i
      // EXE-TGT-X86_64-DAG: out1: 0.0 + 0.0i
      // EXE-TGT-X86_64-DAG: out1: 0.0 + 0.0i
      printf("out1: %.1f + %.1fi\n", creal(out1), cimag(out1));
      // PRT-NEXT: printf
      // EXE-TGT-HOST-DAG: out2: 0.0 + 0.0i
      // EXE-TGT-HOST-DAG: out2: 0.0 + 0.0i
      // EXE-TGT-X86_64-DAG: out2: 0.0 + 0.0i
      // EXE-TGT-X86_64-DAG: out2: 0.0 + 0.0i
      printf("out2: %.1f + %.1fi\n", creal(out2), cimag(out2));
// PRT-SRC-NEXT: #endif
#endif
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-DAG: in: 1.0
      // EXE-DAG: in: 1.0
      printf("in: %.1f\n", in);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out0 = 0.0 + 0.0i
    printf("out0 = %.1f + %.1fi\n", creal(out0), cimag(out0));
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out1 = 3.0 + 0.0i
    printf("out1 = %.1f + %.1fi\n", creal(out1), cimag(out1));
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out2 = 4.0 + 0.0i
    printf("out2 = %.1f + %.1fi\n", creal(out2), cimag(out2));
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out3 = 5.0 + 0.0i
    printf("out3 = %.1f + %.1fi\n", creal(out3), cimag(out3));
  } // PRT-NEXT: }

  // Reduction var is not private due to explicit copy/copyin/copyout clause.

  // PRT-NEXT: {
  {
    // PRT-NEXT: double out0 =
    // PRT-NEXT: double out1 =
    // PRT-NEXT: double out2 =
    double out0 = 0.3;
    double out1 = 1.4;
    double out2 = 2.5;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:   ACCCopyinClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:   ACCCopyoutClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'double'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) present_or_copy(out0) present_or_copyin(out1) present_or_copyout(out2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: out0) map(to: out1) map(from: out2) reduction(+: out0) reduction(+: out1) reduction(+: out2){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: out0) map(to: out1) map(from: out2) reduction(+: out0) reduction(+: out1) reduction(+: out2){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) present_or_copy(out0) present_or_copyin(out1) present_or_copyout(out2){{$}}
    #pragma acc parallel num_gangs(2) present_or_copy(out0) present_or_copyin(out1) present_or_copyout(out2)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // Make sure accessing the variables before the reduction doesn't
      // produce a conflicting implicit firstprivate clause.
      // DMP: CallExpr
      // DMP: CallExpr
      // DMP: CallExpr
      // PRT-NEXT: printf
      // PRT-NEXT: printf
      // PRT-NEXT: printf
      // EXE-DAG: out0 init = 0.0{{$}}
      // EXE-DAG: out0 init = 0.0{{$}}
      // EXE-DAG: out1 init = 0.0{{$}}
      // EXE-DAG: out1 init = 0.0{{$}}
      // EXE-DAG: out2 init = 0.0{{$}}
      // EXE-DAG: out2 init = 0.0{{$}}
      printf("out0 init = %.1f\n", out0);
      printf("out1 init = %.1f\n", out1);
      printf("out2 init = %.1f\n", out2);
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPParallelForDirective
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'double'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'double'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'double'
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker reduction(+: out0,out1,out2){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for reduction(+: out0,out1,out2){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for reduction(+: out0,out1,out2){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker reduction(+: out0,out1,out2){{$}}
      #pragma acc loop worker reduction(+: out0,out1,out2)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 4; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // DMP: CompoundAssignOperator {{.*}} '+='
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out0 +=
        // PRT-NEXT: out1 +=
        // PRT-NEXT: out2 +=
        out0 += -1.1;
        out1 += -1.1;
        out2 += -1.1;
      } // PRT-NEXT: }
    } // PRT-NEXT: }
    // DMP: CallExpr
    // DMP: CallExpr
    // DMP: CallExpr
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // EXE-NEXT: out0 = -8.5
    // EXE-TGT-HOST-NEXT: out1 = -7.4
    // EXE-TGT-HOST-NEXT: out2 = -6.3
    // EXE-TGT-X86_64-NEXT: out1 = 1.4
    // EXE-TGT-X86_64-NEXT: out2 =
    // EXE-TGT-NVPTX64-NEXT: out1 = 1.4
    // EXE-TGT-NVPTX64-NEXT: out2 =
    printf("out0 = %.1f\n", out0);
    printf("out1 = %.1f\n", out1);
    printf("out2 = %.1f\n", out2);
  } // PRT-NEXT: }

  // Reduction var is not private due to copy clause implied by reduction on
  // combined construct.

  // PRT-NEXT: {
  {
    // PRT-NEXT: _Complex double out = 2;
    _Complex double out = 2;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '&&'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' '_Complex double'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNum_gangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' '_Complex double'
    // DMP-NEXT:     ACCReductionClause {{.*}} <implicit> '&&'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' '_Complex double'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' '_Complex double'
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' '_Complex double'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCWorkerClause
    // DMP-NEXT:       ACCReductionClause {{.*}} '&&'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' '_Complex double'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPParallelForDirective
    // DMP-NEXT:         OMPReductionClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'out' '_Complex double'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) worker reduction(&&: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: out) reduction(&&: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for reduction(&&: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: out) reduction(&&: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for reduction(&&: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) worker reduction(&&: out){{$}}
    #pragma acc parallel loop num_gangs(2) worker reduction(&&: out)
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      // DMP: BinaryOperator {{.*}} '&&'
      // PRT-NEXT: out = out && 0;
      out = out && 0;
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 0.0 + 0.0i
    printf("out = %.1f + %.1fi\n", creal(out), cimag(out));
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Vector partitioned.
  //--------------------------------------------------

  // Reduction var is private.
  // OpenMP offloading from x86_64 to nvptx64 isn't supported for long double.

// PRT-SRC-NEXT: #if !TGT_NVPTX64_EXE
#if !TGT_NVPTX64_EXE
  // PRT-NEXT: {
  {
    // PRT-NEXT: long out0 = 5;
    // PRT-NEXT: long out1 = 6;
    // PRT-NEXT: long out2 = 7;
    // PRT-NEXT: long out3 = 8;
    long out0 = 5;
    long out1 = 6;
    long out2 = 7;
    long out3 = 8;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'long'
    // DMP-NEXT:   ACCFirstprivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'long'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out3' 'long'
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'long'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'long'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out3' 'long'
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'long'
    // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'long'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'long'
    // DMP-NEXT:     OMPFirstprivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'long'
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out3' 'long'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'long'
    // DMP-NEXT:     OMPFirstprivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'long'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) reduction(+: out0) firstprivate(out2) private(out3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(+: out0) firstprivate(out2) private(out3) map(tofrom: out0) firstprivate(out1){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(+: out0) firstprivate(out2) private(out3) map(tofrom: out0) firstprivate(out1){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) reduction(+: out0) firstprivate(out2) private(out3){{$}}
    #pragma acc parallel num_gangs(2) reduction(+: out0) firstprivate(out2) private(out3)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: long double in = -1;
      long double in = -1;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'long'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'long'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'long'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out3' 'long'
      // DMP-NEXT:   ACCReductionClause {{.*}} '*
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'long double'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPParallelForSimdDirective
      // DMP-NEXT:     OMPNum_threadsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'long'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'long'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'long'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out3' 'long'
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'in' 'long double'
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop vector reduction(+: out0,out1,out2,out3) reduction(*: in){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for simd num_threads(1) reduction(+: out0,out1,out2,out3) reduction(*: in){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for simd num_threads(1) reduction(+: out0,out1,out2,out3) reduction(*: in){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector reduction(+: out0,out1,out2,out3) reduction(*: in){{$}}
      #pragma acc loop vector reduction(+: out0,out1,out2,out3) reduction(*: in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out0 += 10;
        out0 += 10;
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out1 += 10;
        out1 += 10;
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out2 += 10;
        out2 += 10;
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out3 += 10;
        out3 += 10;
        // DMP: CompoundAssignOperator {{.*}} '*='
        // PRT-NEXT: in *= 3;
        in *= 3;
      } // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-TGT-HOST-DAG:   out1 = 26
      // EXE-TGT-HOST-DAG:   out1 = 26
      // EXE-TGT-X86_64-DAG: out1 = 26
      // EXE-TGT-X86_64-DAG: out1 = 26
      printf("out1 = %ld\n", out1);
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-TGT-HOST-DAG:   out2 = 27
      // EXE-TGT-HOST-DAG:   out2 = 27
      // EXE-TGT-X86_64-DAG: out2 = 27
      // EXE-TGT-X86_64-DAG: out2 = 27
      printf("out2 = %ld\n", out2);
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-TGT-HOST-DAG:   in: -9.0
      // EXE-TGT-HOST-DAG:   in: -9.0
      // EXE-TGT-X86_64-DAG: in: -9.0
      // EXE-TGT-X86_64-DAG: in: -9.0
      printf("in: %.1f\n", (double)in);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-TGT-HOST-NEXT:   out0 = 45
    // EXE-TGT-X86_64-NEXT: out0 = 45
    printf("out0 = %ld\n", out0);
  } // PRT-NEXT: }
// PRT-SRC-NEXT: #endif
#endif

  // Reduction var is not private due to explicit copy/copyin/copyout clause.

  // PRT-NEXT: {
  {
    // PRT-NEXT: double out0 =
    // PRT-NEXT: double out1 =
    // PRT-NEXT: double out2 =
    double out0 = 0.3;
    double out1 = 1.4;
    double out2 = 2.5;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:   ACCCopyinClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:   ACCCopyoutClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'double'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) copy(out0) copyin(out1) copyout(out2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: out0) map(to: out1) map(from: out2) reduction(+: out0) reduction(+: out1) reduction(+: out2){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: out0) map(to: out1) map(from: out2) reduction(+: out0) reduction(+: out1) reduction(+: out2){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) copy(out0) copyin(out1) copyout(out2){{$}}
    #pragma acc parallel num_gangs(2) copy(out0) copyin(out1) copyout(out2)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // Make sure accessing the variables before the reduction doesn't
      // produce a conflicting implicit firstprivate clause.
      // DMP: CallExpr
      // DMP: CallExpr
      // DMP: CallExpr
      // PRT-NEXT: printf
      // PRT-NEXT: printf
      // PRT-NEXT: printf
      // EXE-DAG: out0 init = 0.0{{$}}
      // EXE-DAG: out0 init = 0.0{{$}}
      // EXE-DAG: out1 init = 0.0{{$}}
      // EXE-DAG: out1 init = 0.0{{$}}
      // EXE-DAG: out2 init = 0.0{{$}}
      // EXE-DAG: out2 init = 0.0{{$}}
      printf("out0 init = %.1f\n", out0);
      printf("out1 init = %.1f\n", out1);
      printf("out2 init = %.1f\n", out2);
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPParallelForSimdDirective
      // DMP-NEXT:     OMPNum_threadsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'double'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'double'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'double'
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop vector reduction(+: out0,out1,out2){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for simd num_threads(1) reduction(+: out0,out1,out2){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for simd num_threads(1) reduction(+: out0,out1,out2){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector reduction(+: out0,out1,out2){{$}}
      #pragma acc loop vector reduction(+: out0,out1,out2)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 4; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // DMP: CompoundAssignOperator {{.*}} '+='
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out0 +=
        // PRT-NEXT: out1 +=
        // PRT-NEXT: out2 +=
        out0 += -1.1;
        out1 += -1.1;
        out2 += -1.1;
      } // PRT-NEXT: }
    } // PRT-NEXT: }
    // DMP: CallExpr
    // DMP: CallExpr
    // DMP: CallExpr
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // EXE-NEXT: out0 = -8.5
    // EXE-TGT-HOST-NEXT: out1 = -7.4
    // EXE-TGT-HOST-NEXT: out2 = -6.3
    // EXE-TGT-X86_64-NEXT: out1 = 1.4
    // EXE-TGT-X86_64-NEXT: out2 =
    // EXE-TGT-NVPTX64-NEXT: out1 = 1.4
    // EXE-TGT-NVPTX64-NEXT: out2 =
    printf("out0 = %.1f\n", out0);
    printf("out1 = %.1f\n", out1);
    printf("out2 = %.1f\n", out2);
  } // PRT-NEXT: }

  // Reduction var is not private due to copy clause implied by reduction on
  // combined construct.

  // PRT-NEXT: {
  {
    // PRT-NEXT: long out = 5;
    long out = 5;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'long'
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNum_gangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'long'
    // DMP-NEXT:     ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'long'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'long'
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'long'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCReductionClause {{.*}} '+
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'long'
    // DMP-NEXT:       ACCVectorClause
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPParallelForSimdDirective
    // DMP-NEXT:         OMPNum_threadsClause
    // DMP-NEXT:           IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:         OMPReductionClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'out' 'long'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) reduction(+: out) vector{{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for simd num_threads(1) reduction(+: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for simd num_threads(1) reduction(+: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) reduction(+: out) vector{{$}}
    #pragma acc parallel loop num_gangs(2) reduction(+: out) vector
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      // DMP: CompoundAssignOperator {{.*}} '+='
      // PRT-NEXT: out += 10;
      out += 10;
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 45
    printf("out = %ld\n", out);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Worker and vector partitioned.
  //--------------------------------------------------

  // Reduction var is private.

  // PRT-NEXT: {
  {
    // PRT-NEXT: float out0 = -5;
    // PRT-NEXT: float out1 = -6;
    // PRT-NEXT: float out2 = -7;
    // PRT-NEXT: float out3 = -8;
    float out0 = -5;
    float out1 = -6;
    float out2 = -7;
    float out3 = -8;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} '*'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'float'
    // DMP-NEXT:   ACCFirstprivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'float'
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out3' 'float'
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'float'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'float'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out3' 'float'
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'float'
    // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'float'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'float'
    // DMP-NEXT:     OMPFirstprivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'float'
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out3' 'float'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'float'
    // DMP-NEXT:     OMPFirstprivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'float'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) reduction(*: out0) firstprivate(out2) private(out3){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(*: out0) firstprivate(out2) private(out3) map(tofrom: out0) firstprivate(out1){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(*: out0) firstprivate(out2) private(out3) map(tofrom: out0) firstprivate(out1){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) reduction(*: out0) firstprivate(out2) private(out3){{$}}
    #pragma acc parallel num_gangs(2) reduction(*: out0) firstprivate(out2) private(out3)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: int in = 3;
      int in = 3;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '*'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'float'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'float'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'float'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out3' 'float'
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPParallelForSimdDirective
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'float'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'float'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'float'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out3' 'float'
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'in' 'int'
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker vector reduction(*: out0,out1,out2,out3) reduction(+: in){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for simd reduction(*: out0,out1,out2,out3) reduction(+: in){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for simd reduction(*: out0,out1,out2,out3) reduction(+: in){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector reduction(*: out0,out1,out2,out3) reduction(+: in){{$}}
      #pragma acc loop worker vector reduction(*: out0,out1,out2,out3) reduction(+: in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 4; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '*='
        // PRT-NEXT: out0 *= 3;
        out0 *= 3;
        // DMP: CompoundAssignOperator {{.*}} '*='
        // PRT-NEXT: out1 *= 3;
        out1 *= 3;
        // DMP: CompoundAssignOperator {{.*}} '*='
        // PRT-NEXT: out2 *= 3;
        out2 *= 3;
        // DMP: CompoundAssignOperator {{.*}} '*='
        // PRT-NEXT: out3 *= 3;
        out3 *= 3;
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: in += 4;
        in += 4;
      } // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-DAG: out1 = -486.0
      printf("out1 = %.1f\n", out1);
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-DAG: out2 = -567.0
      printf("out2 = %.1f\n", out2);
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-DAG: in: 19
      // EXE-DAG: in: 19
      printf("in: %d\n", in);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out0 = -32805.0
    printf("out0 = %.1f\n", out0);
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out1 = -6.0
    printf("out1 = %.1f\n", out1);
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out2 = -7.0
    printf("out2 = %.1f\n", out2);
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out3 = -8.0
    printf("out3 = %.1f\n", out3);
  } // PRT-NEXT: }

  // Reduction var is not private due to explicit copy/copyin/copyout clause.

  // PRT-NEXT: {
  {
    // PRT-NEXT: double out0 =
    // PRT-NEXT: double out1 =
    // PRT-NEXT: double out2 =
    double out0 = 0.3;
    double out1 = 1.4;
    double out2 = 2.5;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:   ACCCopyinClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:   ACCCopyoutClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'double'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) pcopy(out0) pcopyin(out1) pcopyout(out2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: out0) map(to: out1) map(from: out2) reduction(+: out0) reduction(+: out1) reduction(+: out2){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: out0) map(to: out1) map(from: out2) reduction(+: out0) reduction(+: out1) reduction(+: out2){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) pcopy(out0) pcopyin(out1) pcopyout(out2){{$}}
    #pragma acc parallel num_gangs(2) pcopy(out0) pcopyin(out1) pcopyout(out2)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // Make sure accessing the variables before the reduction doesn't
      // produce a conflicting implicit firstprivate clause.
      // DMP: CallExpr
      // DMP: CallExpr
      // DMP: CallExpr
      // PRT-NEXT: printf
      // PRT-NEXT: printf
      // PRT-NEXT: printf
      // EXE-DAG: out0 init = 0.0{{$}}
      // EXE-DAG: out0 init = 0.0{{$}}
      // EXE-DAG: out1 init = 0.0{{$}}
      // EXE-DAG: out1 init = 0.0{{$}}
      // EXE-DAG: out2 init = 0.0{{$}}
      // EXE-DAG: out2 init = 0.0{{$}}
      printf("out0 init = %.1f\n", out0);
      printf("out1 init = %.1f\n", out1);
      printf("out2 init = %.1f\n", out2);
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'double'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'double'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'double'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPParallelForSimdDirective
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'double'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'double'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'double'
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker vector reduction(+: out0,out1,out2){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for simd reduction(+: out0,out1,out2){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for simd reduction(+: out0,out1,out2){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector reduction(+: out0,out1,out2){{$}}
      #pragma acc loop worker vector reduction(+: out0,out1,out2)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 4; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // DMP: CompoundAssignOperator {{.*}} '+='
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out0 +=
        // PRT-NEXT: out1 +=
        // PRT-NEXT: out2 +=
        out0 += -1.1;
        out1 += -1.1;
        out2 += -1.1;
      } // PRT-NEXT: }
    } // PRT-NEXT: }
    // DMP: CallExpr
    // DMP: CallExpr
    // DMP: CallExpr
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // EXE-NEXT: out0 = -8.5
    // EXE-TGT-HOST-NEXT: out1 = -7.4
    // EXE-TGT-HOST-NEXT: out2 = -6.3
    // EXE-TGT-X86_64-NEXT: out1 = 1.4
    // EXE-TGT-X86_64-NEXT: out2 =
    // EXE-TGT-NVPTX64-NEXT: out1 = 1.4
    // EXE-TGT-NVPTX64-NEXT: out2 =
    printf("out0 = %.1f\n", out0);
    printf("out1 = %.1f\n", out1);
    printf("out2 = %.1f\n", out2);
  } // PRT-NEXT: }

  // Reduction var is not private due to copy clause implied by reduction on
  // combined construct.

  // PRT-NEXT: {
  {
    // PRT-NEXT: float out = -5;
    float out = -5;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '*'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'float'
    // DMP-NEXT:   ACCParallelDirective
    // DMP-NEXT:     ACCNum_gangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'float'
    // DMP-NEXT:     ACCReductionClause {{.*}} '*'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'float'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'float'
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'float'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCWorkerClause
    // DMP-NEXT:       ACCVectorClause
    // DMP-NEXT:       ACCReductionClause {{.*}} '*'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'float'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPParallelForSimdDirective
    // DMP-NEXT:         OMPReductionClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'out' 'float'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) worker vector reduction(*: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: out) reduction(*: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for simd reduction(*: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: out) reduction(*: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for simd reduction(*: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) worker vector reduction(*: out){{$}}
    #pragma acc parallel loop num_gangs(2) worker vector reduction(*: out)
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 4; ++i) {
      // DMP: CompoundAssignOperator {{.*}} '*='
      // PRT-NEXT: out *= 3;
      out *= 3;
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = -32805.0
    printf("out = %.1f\n", out);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Gang partitioned, explicit acc parallel reduction.
  //
  // Notice that the reduction clause for the gang-local variable "in" is
  // completely discarded during translation to OpenMP because gang reductions
  // cannot affect gang-local variables.  Thus, each gang operates on its own
  // copy of "in" as if the reduction were not specified.
  //
  // It's impossible to have an acc parallel loop version of this because the
  // reduction then applies to the acc loop not the acc parallel.  Thus, that
  // version is tested in the next section instead.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: double out =
    double out = 0.3;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'double'
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'double'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'double'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'double'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(+: out) map(tofrom: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(+: out) map(tofrom: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) reduction(+: out){{$}}
    #pragma acc parallel num_gangs(2) reduction(+: out)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: int in = 3;
      int in = 3;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'double'
      // DMP-NEXT:   ACCReductionClause {{.*}} '*'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NOT:      OMPReductionClause
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang reduction(+: out) reduction(*: in){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang reduction(+: out) reduction(*: in){{$}}
      #pragma acc loop gang reduction(+: out) reduction(*: in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 4; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out +=
        out += -1.1;
        // DMP: CompoundAssignOperator {{.*}} '*='
        // PRT-NEXT: in *=
        in *= 2;
      } // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-NEXT: in = 12{{$}}
      // EXE-NEXT: in = 12{{$}}
      printf("in = %d\n", in);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = -4.1
    printf("out = %.1f\n", out);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Gang partitioned, implicit acc parallel reduction.
  //
  // The acc parallel loop version of this was just tested.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: double out =
    double out = 0.3;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'double'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'double'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'double'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'double'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2){{$}}
    #pragma acc parallel num_gangs(2)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // Make sure accessing out before the reduction doesn't produce a
      // conflicting implicit firstprivate clause.
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-NEXT: out init = 0.0{{$}}
      // EXE-NEXT: out init = 0.0{{$}}
      printf("out init = %.1f\n", out);
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'double'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NOT:      OMPReductionClause
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang reduction(+: out){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang reduction(+: out){{$}}
      #pragma acc loop gang reduction(+: out)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 4; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out +=
        out += -1.1;
      } // PRT-NEXT: }
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = -4.1
    printf("out = %.1f\n", out);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: double out =
    double out = 0.3;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'double'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNum_gangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'double'
    // DMP-NEXT:     ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'double'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'double'
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'double'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCGangClause
    // DMP-NEXT:       ACCReductionClause {{.*}} '+'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'double'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeDirective
    // DMP-NOT:          OMPReductionClause
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) gang reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) gang reduction(+: out){{$}}
    #pragma acc parallel loop num_gangs(2) gang reduction(+: out)
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 4; ++i) {
      // DMP: CompoundAssignOperator {{.*}} '+='
      // PRT-NEXT: out +=
      out += -1.1;
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = -4.1
    printf("out = %.1f\n", out);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Gang and worker partitioned, explicit acc parallel reduction.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 3;
    int out = 3;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(+: out) map(tofrom: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(+: out) map(tofrom: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) reduction(+: out){{$}}
    #pragma acc parallel num_gangs(2) reduction(+: out)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: int in = 3;
      int in = 3;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeParallelForDirective
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'in' 'int'
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang worker reduction(+: out,in){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for reduction(+: out,in){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute parallel for reduction(+: out,in){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang worker reduction(+: out,in){{$}}
      #pragma acc loop gang worker reduction(+: out,in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 4; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out +=
        out += 2;
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: in +=
        in += 2;
      } // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-NEXT: in = 7{{$}}
      // EXE-NEXT: in = 7{{$}}
      printf("in = %d\n", in);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 11
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 3;
    int out = 3;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNum_gangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCGangClause
    // DMP-NEXT:       ACCWorkerClause
    // DMP-NEXT:       ACCReductionClause {{.*}} '+'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeParallelForDirective
    // DMP-NEXT:         OMPReductionClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop gang num_gangs(2) worker reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for reduction(+: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute parallel for reduction(+: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop gang num_gangs(2) worker reduction(+: out){{$}}
    #pragma acc parallel loop gang num_gangs(2) worker reduction(+: out)
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 4; ++i) {
      // DMP: CompoundAssignOperator {{.*}} '+='
      // PRT-NEXT: out +=
      out += 2;
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 11
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Gang and vector partitioned, implicit acc parallel reduction.
  //
  // For acc parallel loop, we extend this test too, not to distinguish
  // implicit from the explicit version in the previous test, but to
  // distinguish vector from the worker version in the previous test.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 3;
    int out = 3;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2){{$}}
    #pragma acc parallel num_gangs(2)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: int in = 3;
      int in = 3;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeSimdDirective
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'in' 'int'
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang vector reduction(+: out,in){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd reduction(+: out,in){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute simd reduction(+: out,in){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang vector reduction(+: out,in){{$}}
      #pragma acc loop gang vector reduction(+: out,in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 4; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out +=
        out += 2;
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: in +=
        in += 2;
      } // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-NEXT: in = 7{{$}}
      // EXE-NEXT: in = 7{{$}}
      printf("in = %d\n", in);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 11
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 3;
    int out = 3;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNum_gangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCVectorClause
    // DMP-NEXT:       ACCGangClause
    // DMP-NEXT:       ACCReductionClause {{.*}} '+'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeSimdDirective
    // DMP-NEXT:         OMPReductionClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop vector gang reduction(+: out) num_gangs(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd reduction(+: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute simd reduction(+: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop vector gang reduction(+: out) num_gangs(2){{$}}
    #pragma acc parallel loop vector gang reduction(+: out) num_gangs(2)
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 4; ++i) {
      // DMP: CompoundAssignOperator {{.*}} '+='
      // PRT-NEXT: out +=
      out += 2;
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 11
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Gang, worker, and vector partitioned, single loop.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 3;
    int out = 3;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2){{$}}
    #pragma acc parallel num_gangs(2)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: int in = 3;
      int in = 3;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeParallelForSimdDirective
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'in' 'int'
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang worker vector reduction(+: out,in){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd reduction(+: out,in){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute parallel for simd reduction(+: out,in){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang worker vector reduction(+: out,in){{$}}
      #pragma acc loop gang worker vector reduction(+: out,in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 4; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out +=
        out += 2;
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: in +=
        in += 2;
      } // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-NEXT: in = 7{{$}}
      // EXE-NEXT: in = 7{{$}}
      printf("in = %d\n", in);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 11
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 3;
    int out = 3;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCWorkerClause
    // DMP-NEXT:   ACCVectorClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNum_gangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCGangClause
    // DMP-NEXT:       ACCWorkerClause
    // DMP-NEXT:       ACCVectorClause
    // DMP-NEXT:       ACCReductionClause {{.*}} '+'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeParallelForSimdDirective
    // DMP-NEXT:         OMPReductionClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) gang worker vector reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd reduction(+: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute parallel for simd reduction(+: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) gang worker vector reduction(+: out){{$}}
    #pragma acc parallel loop num_gangs(2) gang worker vector reduction(+: out)
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 4; ++i) {
      // DMP: CompoundAssignOperator {{.*}} '+='
      // PRT-NEXT: out +=
      out += 2;
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 11
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Gang, worker, and vector partitioned, separate nested loops.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 3;
    int out = 3;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2){{$}}
    #pragma acc parallel num_gangs(2)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: int in = 3;
      int in = 3;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NOT:      OMPReductionClause
      // DMP:          Stmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang reduction(+: out,in){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang reduction(+: out,in){{$}}
      #pragma acc loop gang reduction(+: out,in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP:      ACCLoopDirective
        // DMP-NEXT:   ACCWorkerClause
        // DMP-NEXT:   ACCReductionClause {{.*}} '+'
        // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
        // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'int'
        // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
        // DMP-NEXT:   impl: OMPParallelForDirective
        // DMP-NEXT:     OMPReductionClause
        // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
        // DMP-NEXT:       DeclRefExpr {{.*}} 'in' 'int'
        //
        // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker reduction(+: out,in){{$}}
        // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for reduction(+: out,in){{$}}
        // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for reduction(+: out,in){{$}}
        // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker reduction(+: out,in){{$}}
        #pragma acc loop worker reduction(+: out,in)
        // PRT-NEXT: for ({{.*}}) {
        for (int j = 0; j < 2; ++j) {
          // DMP:      ACCLoopDirective
          // DMP-NEXT:   ACCVectorClause
          // DMP-NEXT:   ACCReductionClause {{.*}} '+'
          // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
          // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'int'
          // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
          // DMP-NEXT:   impl: OMPSimdDirective
          // DMP-NEXT:     OMPReductionClause
          // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
          // DMP-NEXT:       DeclRefExpr {{.*}} 'in' 'int'
          //
          // PRT-A-NEXT:  {{^ *}}#pragma acc loop vector reduction(+: out,in){{$}}
          // PRT-AO-NEXT: {{^ *}}// #pragma omp simd reduction(+: out,in){{$}}
          // PRT-O-NEXT:  {{^ *}}#pragma omp simd reduction(+: out,in){{$}}
          // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector reduction(+: out,in){{$}}
          #pragma acc loop vector reduction(+: out,in)
          // PRT-NEXT: for ({{.*}}) {
          for (int k = 0; k < 2; ++k) {
            // DMP: CompoundAssignOperator {{.*}} '+='
            // PRT-NEXT: out +=
            out += 2;
            // DMP: CompoundAssignOperator {{.*}} '+='
            // PRT-NEXT: in +=
            in += 2;
          } // PRT-NEXT: }
        } // PRT-NEXT: }
      } // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-NEXT: in = 11{{$}}
      // EXE-NEXT: in = 11{{$}}
      printf("in = %d\n", in);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 19
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 3;
    int out = 3;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCGangClause
    // DMP-NEXT:   ACCReductionClause {{.*}} '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNum_gangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCGangClause
    // DMP-NEXT:       ACCReductionClause {{.*}} '+'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       impl: OMPDistributeDirective
    // DMP-NOT:          OMPReductionClause
    // DMP-NEXT:         Stmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) gang reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) gang reduction(+: out){{$}}
    #pragma acc parallel loop num_gangs(2) gang reduction(+: out)
    // PRT-NEXT: for ({{.*}}) {
    for (int i = 0; i < 2; ++i) {
      // PRT-NEXT: int in = 3;
      int in = 3;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPParallelForDirective
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'in' 'int'
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker reduction(+: out,in){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for reduction(+: out,in){{$}}
      //
      // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for reduction(+: out,in){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker reduction(+: out,in){{$}}
      #pragma acc loop worker reduction(+: out,in)
      // PRT-NEXT: for ({{.*}}) {
      for (int j = 0; j < 2; ++j) {
        // DMP:      ACCLoopDirective
        // DMP-NEXT:   ACCVectorClause
        // DMP-NEXT:   ACCReductionClause {{.*}} '+'
        // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
        // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'int'
        // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
        // DMP-NEXT:   impl: OMPSimdDirective
        // DMP-NEXT:     OMPReductionClause
        // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
        // DMP-NEXT:       DeclRefExpr {{.*}} 'in' 'int'
        //
        // PRT-A-NEXT:  {{^ *}}#pragma acc loop vector reduction(+: out,in){{$}}
        // PRT-AO-NEXT: {{^ *}}// #pragma omp simd reduction(+: out,in){{$}}
        //
        // PRT-O-NEXT:  {{^ *}}#pragma omp simd reduction(+: out,in){{$}}
        // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector reduction(+: out,in){{$}}
        #pragma acc loop vector reduction(+: out,in)
        // PRT-NEXT: for ({{.*}}) {
        for (int k = 0; k < 2; ++k) {
          // DMP: CompoundAssignOperator {{.*}} '+='
          // PRT-NEXT: out +=
          out += 2;
          // DMP: CompoundAssignOperator {{.*}} '+='
          // PRT-NEXT: in +=
          in += 2;
        } // PRT-NEXT: }
      } // PRT-NEXT: }
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 19
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Gang reductions from sibling loops.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 5;
    int out = 5;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2){{$}}
    #pragma acc parallel num_gangs(2)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NOT:      OMPReductionClause
      // DMP:          Stmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang reduction(+: out){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang reduction(+: out){{$}}
      #pragma acc loop gang reduction(+: out)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out += 2;
        out += 2;
      } // PRT-NEXT: }
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NOT:      OMPReductionClause
      // DMP:          Stmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang reduction(+: out){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang reduction(+: out){{$}}
      #pragma acc loop gang reduction(+: out)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out += 3;
        out += 3;
      } // PRT-NEXT: }
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 15
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Gang reductions, shadowing local variable out of scope.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 5;
    int out = 5;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2){{$}}
    #pragma acc parallel num_gangs(2)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: {
      {
        // PRT-NEXT: int out = 5;
        int out = 5;
        // DMP:      ACCLoopDirective
        // DMP-NEXT:   ACCGangClause
        // DMP-NEXT:   ACCReductionClause {{.*}} '+'
        // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
        // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
        // DMP-NEXT:   impl: OMPDistributeDirective
        // DMP-NOT:      OMPReductionClause
        // DMP:          Stmt
        //
        // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang reduction(+: out){{$}}
        // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
        // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
        // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang reduction(+: out){{$}}
        #pragma acc loop gang reduction(+: out)
        // PRT-NEXT: for ({{.*}}) {
        for (int i = 0; i < 2; ++i) {
          // DMP: CompoundAssignOperator {{.*}} '+='
          // PRT-NEXT: out += 2;
          out += 2;
        } // PRT-NEXT: }
      } // PRT-NEXT: }
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NOT:      OMPReductionClause
      // DMP:          Stmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang reduction(+: out){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang reduction(+: out){{$}}
      #pragma acc loop gang reduction(+: out)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out += 3;
        out += 3;
      } // PRT-NEXT: }
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 11
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Gang reduction from within sequential acc loop.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 5;
    int out = 5;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2){{$}}
    #pragma acc parallel num_gangs(2)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:   impl: ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:  {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
      #pragma acc loop
      // PRT-NEXT: for ({{.*}}) {
      for (int i0 = 0; i0 < 2; ++i0) {
        // DMP:      ACCLoopDirective
        // DMP-NEXT:   ACCSeqClause
        // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
        // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
        // DMP-NEXT:   impl: ForStmt
        //
        // PRT-A-NEXT:  {{^ *}}#pragma acc loop seq
        // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
        // PRT-A-SAME:  {{^$}}
        // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq // discarded in OpenMP translation{{$}}
        #pragma acc loop seq
        // PRT-NEXT: for ({{.*}}) {
        for (int i1 = 0; i1 < 2; ++i1) {
          // DMP:      ACCLoopDirective
          // DMP-NEXT:   ACCIndependentClause
          // DMP-NOT:      <implicit>
          // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
          // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
          // DMP-NEXT:   impl: ForStmt
          //
          // PRT-A-NEXT:  {{^ *}}#pragma acc loop independent
          // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
          // PRT-A-SAME:  {{^$}}
          // PRT-OA-NEXT: {{^ *}}// #pragma acc loop independent // discarded in OpenMP translation{{$}}
          #pragma acc loop independent
          // PRT-NEXT: for ({{.*}}) {
          for (int i2 = 0; i2 < 2; ++i2) {
            // DMP:      ACCLoopDirective
            // DMP-NEXT:   ACCAutoClause
            // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
            // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
            // DMP-NEXT:   impl: ForStmt
            //
            // PRT-A-NEXT:  {{^ *}}#pragma acc loop auto
            // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
            // PRT-A-SAME:  {{^$}}
            // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto // discarded in OpenMP translation{{$}}
            #pragma acc loop auto
            // PRT-NEXT: for ({{.*}}) {
            for (int i3 = 0; i3 < 2; ++i3) {
              // DMP:      ACCLoopDirective
              // DMP-NEXT:   ACCGangClause
              // DMP-NEXT:   ACCReductionClause {{.*}} '+'
              // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
              // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
              // DMP-NEXT:   impl: OMPDistributeDirective
              // DMP-NOT:      OMPReductionClause
              // DMP:          Stmt
              //
              // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang reduction(+: out){{$}}
              // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
              // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
              // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang reduction(+: out){{$}}
              #pragma acc loop gang reduction(+: out)
              // PRT-NEXT: for ({{.*}}) {
              for (int j = 0; j < 2; ++j) {
                // DMP: CompoundAssignOperator {{.*}} '+='
                // PRT-NEXT: out += 2;
                out += 2;
              } // PRT-NEXT: }
            } // PRT-NEXT: }
          } // PRT-NEXT: }
        } // PRT-NEXT: }
      } // PRT-NEXT: }
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 69
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 5;
    int out = 5;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNum_gangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       impl: ForStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: out) reduction(+: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2){{$}}
    #pragma acc parallel loop num_gangs(2)
    // PRT-NEXT: for ({{.*}}) {
    for (int i0 = 0; i0 < 2; ++i0) {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCSeqClause
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:   impl: ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop seq
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:  {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq // discarded in OpenMP translation{{$}}
      #pragma acc loop seq
      // PRT-NEXT: for ({{.*}}) {
      for (int i1 = 0; i1 < 2; ++i1) {
        // DMP:      ACCLoopDirective
        // DMP-NEXT:   ACCIndependentClause
        // DMP-NOT:      <implicit>
        // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
        // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
        // DMP-NEXT:   impl: ForStmt
        //
        // PRT-A-NEXT:  {{^ *}}#pragma acc loop independent
        // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
        // PRT-A-SAME:  {{^$}}
        // PRT-OA-NEXT: {{^ *}}// #pragma acc loop independent // discarded in OpenMP translation{{$}}
        #pragma acc loop independent
        // PRT-NEXT: for ({{.*}}) {
        for (int i2 = 0; i2 < 2; ++i2) {
          // DMP:      ACCLoopDirective
          // DMP-NEXT:   ACCAutoClause
          // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
          // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
          // DMP-NEXT:   impl: ForStmt
          //
          // PRT-A-NEXT:  {{^ *}}#pragma acc loop auto
          // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
          // PRT-A-SAME:  {{^$}}
          // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto // discarded in OpenMP translation{{$}}
          #pragma acc loop auto
          // PRT-NEXT: for ({{.*}}) {
          for (int i3 = 0; i3 < 2; ++i3) {
            // DMP:      ACCLoopDirective
            // DMP-NEXT:   ACCGangClause
            // DMP-NEXT:   ACCReductionClause {{.*}} '+'
            // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
            // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
            // DMP-NEXT:   impl: OMPDistributeDirective
            // DMP-NOT:      OMPReductionClause
            //
            // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang reduction(+: out){{$}}
            // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
            // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
            // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang reduction(+: out){{$}}
            #pragma acc loop gang reduction(+: out)
            // DMP: ForStmt
            // PRT-NEXT: for ({{.*}}) {
            for (int j = 0; j < 2; ++j) {
              // DMP: CompoundAssignOperator {{.*}} '+='
              // PRT-NEXT: out += 2;
              out += 2;
            } // PRT-NEXT: }
          } // PRT-NEXT: }
        } // PRT-NEXT: }
      } // PRT-NEXT: }
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 69
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Gang reduction on variable explicitly gang-private at acc parallel.
  //
  // TODO: Once we decide how to deal with private/firstprivate and reduction
  // on the same variable, this might be interesting to extend for acc parallel
  // loop.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out0 = 5;
    // PRT-NEXT: int out1 = 5;
    int out0 = 5;
    int out1 = 5;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'int'
    // DMP-NEXT:   ACCFirstprivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPPrivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'int'
    // DMP-NEXT:     OMPFirstprivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) private(out0) firstprivate(out1){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) private(out0) firstprivate(out1){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) private(out0) firstprivate(out1){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) private(out0) firstprivate(out1){{$}}
    #pragma acc parallel num_gangs(2) private(out0) firstprivate(out1)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCGangClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPDistributeDirective
      // DMP-NOT:      OMPReductionClause
      // DMP:          Stmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop gang reduction(+: out0,out1){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp distribute{{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang reduction(+: out0,out1){{$}}
      #pragma acc loop gang reduction(+: out0,out1)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out0 += 2;
        // PRT-NEXT: out1 += 2;
        out0 += 2;
        out1 += 2;
      } // PRT-NEXT: }
    } // PRT-NEXT: }
    // DMP: CallExpr
    // DMP: CallExpr
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // EXE-NEXT: out0 = 5
    // EXE-NEXT: out1 = 5
    printf("out0 = %d\n", out0);
    printf("out1 = %d\n", out1);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Gang reduction on variable explicitly privatized at enclosing acc loop not
  // at an enclosing acc parallel.
  //--------------------------------------------------

  // Each of out0, out1, and out2 is privatized at an acc loop enclosing the
  // acc loop that would otherwise create a gang reduction for it.  In the
  // case of out0 and out1, the latter loop is a gang loop.  In the case of
  // out2, it's a worker loop where out2 would otherwise be gang-shared due to
  // its copy clause on the acc parallel.
  //
  // Because out0 and out2 are in private clauses but out1 is in a reduction
  // clause on an outer acc loop, the acc parallel doesn't need a data clause
  // for out0 or out2, but it does need out1 in an (implicit) firstprivate in
  // order to access its original value.

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out0 = 5;
    // PRT-NEXT: int out1 = 6;
    // PRT-NEXT: int out2 = 7;
    int out0 = 5;
    int out1 = 6;
    int out2 = 7;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCCopyClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'int'
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:   ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPMapClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'int'
    // DMP-NEXT:     OMPFirstprivateClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NOT:      OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) present_or_copy(out2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: out2) firstprivate(out1){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: out2) firstprivate(out1){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) present_or_copy(out2){{$}}
    #pragma acc parallel num_gangs(2) present_or_copy(out2)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCSeqClause
      // DMP-NEXT:   ACCPrivateClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'int'
      // DMP-NEXT:   ACCReductionClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'int'
      // DMP-NEXT:   impl: CompoundStmt
      // DMP-NEXT:     DeclStmt
      // DMP-NEXT:       VarDecl {{.*}} out0 'int'
      // DMP-NEXT:     ForStmt
      // DMP:        ACCLoopDirective
      // DMP-NEXT:     ACCGangClause
      // DMP-NEXT:     ACCReductionClause {{.*}} '+'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out0' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'int'
      // DMP-NEXT:     ACCPrivateClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'int'
      // DMP-NEXT:     ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:     impl: OMPDistributeDirective
      // DMP-NEXT:       OMPPrivateClause
      // DMP-NOT:          <implicit>
      // DMP-NEXT:         DeclRefExpr {{.*}} 'out2' 'int'
      // DMP-NOT:        OMP
      // DMP:            ForStmt
      // DMP:          ACCLoopDirective
      // DMP-NEXT:       ACCWorkerClause
      // DMP-NEXT:       ACCReductionClause {{.*}} '+'
      // DMP-NEXT:         DeclRefExpr {{.*}} 'out0' 'int'
      // DMP-NEXT:         DeclRefExpr {{.*}} 'out1' 'int'
      // DMP-NEXT:         DeclRefExpr {{.*}} 'out2' 'int'
      // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:       impl: OMPParallelForDirective
      // DMP-NEXT:         OMPReductionClause
      // DMP-NOT:            <implicit>
      // DMP-NEXT:           DeclRefExpr {{.*}} 'out0' 'int'
      // DMP-NEXT:           DeclRefExpr {{.*}} 'out1' 'int'
      // DMP-NEXT:           DeclRefExpr {{.*}} 'out2' 'int'
      // DMP-NOT:          OMP
      // DMP:              ForStmt
      // DMP:                CompoundAssignOperator {{.*}} '+='
      //
      // PRT-NOACC-NEXT: {{^ *}}for (int i = 0; i < 2; ++i) {
      // PRT-NOACC-NEXT: {{^ *}}  for (int j = 0; j < 2; ++j) {
      // PRT-NOACC-NEXT: {{^ *}}    for (int k = 0; k < 2; ++k) {
      // PRT-NOACC-NEXT: {{^ *}}      out0 += 2;
      // PRT-NOACC-NEXT: {{^ *}}      out1 += 2;
      // PRT-NOACC-NEXT: {{^ *}}      out2 += 2;
      // PRT-NOACC-NEXT: {{^ *}}    }
      // PRT-NOACC-NEXT: {{^ *}}  }
      // PRT-NOACC-NEXT: {{^ *}}}
      //
      // PRT-AO-NEXT: {{^ *}}// v----------ACC----------v
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop seq private(out0) reduction(+: out1){{$}}
      // PRT-A-NEXT:  {{^ *}}for (int i = 0; i < 2; ++i) {
      // PRT-A-NEXT:  {{^ *}}  #pragma acc loop gang reduction(+: out0,out1) private(out2){{$}}
      // PRT-A-NEXT:  {{^ *}}  for (int j = 0; j < 2; ++j) {
      // PRT-A-NEXT:  {{^ *}}    #pragma acc loop worker reduction(+: out0,out1,out2){{$}}
      // PRT-A-NEXT:  {{^ *}}    for (int k = 0; k < 2; ++k) {
      // PRT-A-NEXT:  {{^ *}}      out0 += 2;
      // PRT-A-NEXT:  {{^ *}}      out1 += 2;
      // PRT-A-NEXT:  {{^ *}}      out2 += 2;
      // PRT-A-NEXT:  {{^ *}}    }
      // PRT-A-NEXT:  {{^ *}}  }
      // PRT-A-NEXT:  {{^ *}}}
      // PRT-AO-NEXT: {{^ *}}// ---------ACC->OMP--------
      // PRT-AO-NEXT: {{^ *}}// {
      // PRT-AO-NEXT: {{^ *}}//   int out0;
      // PRT-AO-NEXT: {{^ *}}//   for (int i = 0; i < 2; ++i) {
      // PRT-AO-NEXT: {{^ *}}//     #pragma omp distribute private(out2){{$}}
      // PRT-AO-NEXT: {{^ *}}//     for (int j = 0; j < 2; ++j) {
      // PRT-AO-NEXT: {{^ *}}//       #pragma omp parallel for reduction(+: out0,out1,out2){{$}}
      // PRT-AO-NEXT: {{^ *}}//       for (int k = 0; k < 2; ++k) {
      // PRT-AO-NEXT: {{^ *}}//         out0 += 2;
      // PRT-AO-NEXT: {{^ *}}//         out1 += 2;
      // PRT-AO-NEXT: {{^ *}}//         out2 += 2;
      // PRT-AO-NEXT: {{^ *}}//       }
      // PRT-AO-NEXT: {{^ *}}//     }
      // PRT-AO-NEXT: {{^ *}}//   }
      // PRT-AO-NEXT: {{^ *}}// }
      // PRT-AO-NEXT: {{^ *}}// ^----------OMP----------^
      //
      // PRT-OA-NEXT: {{^ *}}// v----------OMP----------v
      // PRT-O-NEXT:  {{^ *}}{
      // PRT-O-NEXT:  {{^ *}}  int out0;
      // PRT-O-NEXT:  {{^ *}}  for (int i = 0; i < 2; ++i) {
      // PRT-O-NEXT:  {{^ *}}    #pragma omp distribute private(out2){{$}}
      // PRT-O-NEXT:  {{^ *}}    for (int j = 0; j < 2; ++j) {
      // PRT-O-NEXT:  {{^ *}}      #pragma omp parallel for reduction(+: out0,out1,out2){{$}}
      // PRT-O-NEXT:  {{^ *}}      for (int k = 0; k < 2; ++k) {
      // PRT-O-NEXT:  {{^ *}}        out0 += 2;
      // PRT-O-NEXT:  {{^ *}}        out1 += 2;
      // PRT-O-NEXT:  {{^ *}}        out2 += 2;
      // PRT-O-NEXT:  {{^ *}}      }
      // PRT-O-NEXT:  {{^ *}}    }
      // PRT-O-NEXT:  {{^ *}}  }
      // PRT-O-NEXT:  {{^ *}}}
      // PRT-OA-NEXT: {{^ *}}// ---------OMP<-ACC--------
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq private(out0) reduction(+: out1){{$}}
      // PRT-OA-NEXT: {{^ *}}// for (int i = 0; i < 2; ++i) {
      // PRT-OA-NEXT: {{^ *}}//   #pragma acc loop gang reduction(+: out0,out1) private(out2){{$}}
      // PRT-OA-NEXT: {{^ *}}//   for (int j = 0; j < 2; ++j) {
      // PRT-OA-NEXT: {{^ *}}//     #pragma acc loop worker reduction(+: out0,out1,out2){{$}}
      // PRT-OA-NEXT: {{^ *}}//     for (int k = 0; k < 2; ++k) {
      // PRT-OA-NEXT: {{^ *}}//       out0 += 2;
      // PRT-OA-NEXT: {{^ *}}//       out1 += 2;
      // PRT-OA-NEXT: {{^ *}}//       out2 += 2;
      // PRT-OA-NEXT: {{^ *}}//     }
      // PRT-OA-NEXT: {{^ *}}//   }
      // PRT-OA-NEXT: {{^ *}}// }
      // PRT-OA-NEXT: {{^ *}}// ^----------ACC----------^
      #pragma acc loop seq private(out0) reduction(+: out1)
      for (int i = 0; i < 2; ++i) {
        #pragma acc loop gang reduction(+: out0,out1) private(out2)
        for (int j = 0; j < 2; ++j) {
          #pragma acc loop worker reduction(+: out0,out1,out2)
          for (int k = 0; k < 2; ++k) {
            out0 += 2;
            out1 += 2;
            out2 += 2;
          }
        }
      }
    } // PRT-NEXT: }
    // DMP: CallExpr
    // DMP: CallExpr
    // DMP: CallExpr
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // EXE-NEXT: out0 = 5
    // EXE-NEXT: out1 = 6
    // EXE-NEXT: out2 = 7
    printf("out0 = %d\n", out0);
    printf("out1 = %d\n", out1);
    printf("out2 = %d\n", out2);
  } // PRT-NEXT: }

  // This is the same as the previous test except the outer acc loop is now
  // combined with the enclosing acc parallel.  Thus, out1's reduction on the
  // outer acc loop now implies a copy clause for out1 on the acc parallel,
  // making out1 a gang-shared variable at the outer acc loop's out1 reduction,
  // which thus becomes a gang reduction.  out0 and out2 still have no gang
  // reductions for the same reasons as in the previous test.
  //
  // Also, the explicit copy has been replaced with copyin, which still
  // shouldn't have any observable effect.

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out0 = 5;
    // PRT-NEXT: int out1 = 6;
    // PRT-NEXT: int out2 = 7;
    int out0 = 5;
    int out1 = 6;
    int out2 = 7;
    // DMP:      ACCParallelLoopDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCSeqClause
    // DMP-NEXT:   ACCPrivateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out0' 'int'
    // DMP-NEXT:   ACCReductionClause {{.*}} '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:   ACCCopyinClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out2' 'int'
    // DMP-NEXT:   effect: ACCParallelDirective
    // DMP-NEXT:     ACCNum_gangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     ACCCopyinClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out2' 'int'
    // DMP-NEXT:     ACCCopyClause {{.*}} <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:     ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out2' 'int'
    // DMP-NEXT:       OMPMapClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NOT:        OMP
    // DMP:            CompoundStmt
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCSeqClause
    // DMP-NEXT:       ACCPrivateClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out0' 'int'
    // DMP-NEXT:       ACCReductionClause {{.*}} '+'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:       impl: CompoundStmt
    // DMP-NEXT:         DeclStmt
    // DMP-NEXT:           VarDecl {{.*}} out0 'int'
    // DMP-NEXT:         ForStmt
    // DMP:            ACCLoopDirective
    // DMP-NEXT:         ACCGangClause
    // DMP-NEXT:         ACCReductionClause {{.*}} '+'
    // DMP-NEXT:           DeclRefExpr {{.*}} 'out0' 'int'
    // DMP-NEXT:           DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:         ACCPrivateClause
    // DMP-NEXT:           DeclRefExpr {{.*}} 'out2' 'int'
    // DMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:         impl: OMPDistributeDirective
    // DMP-NEXT:           OMPPrivateClause
    // DMP-NOT:              <implicit>
    // DMP-NEXT:             DeclRefExpr {{.*}} 'out2' 'int'
    // DMP-NOT:            OMP
    // DMP:                ForStmt
    // DMP:              ACCLoopDirective
    // DMP-NEXT:           ACCWorkerClause
    // DMP-NEXT:           ACCReductionClause {{.*}} '+'
    // DMP-NEXT:             DeclRefExpr {{.*}} 'out0' 'int'
    // DMP-NEXT:             DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:             DeclRefExpr {{.*}} 'out2' 'int'
    // DMP-NEXT:           ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:           impl: OMPParallelForDirective
    // DMP-NEXT:             OMPReductionClause
    // DMP-NOT:                <implicit>
    // DMP-NEXT:               DeclRefExpr {{.*}} 'out0' 'int'
    // DMP-NEXT:               DeclRefExpr {{.*}} 'out1' 'int'
    // DMP-NEXT:               DeclRefExpr {{.*}} 'out2' 'int'
    // DMP-NOT:              OMP
    // DMP:                  ForStmt
    // DMP:                    CompoundAssignOperator {{.*}} '+='
    //
    // PRT-NOACC-NEXT: {{^ *}}for (int i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT: {{^ *}}  for (int j = 0; j < 2; ++j) {
    // PRT-NOACC-NEXT: {{^ *}}    for (int k = 0; k < 2; ++k) {
    // PRT-NOACC-NEXT: {{^ *}}      out0 += 2;
    // PRT-NOACC-NEXT: {{^ *}}      out1 += 2;
    // PRT-NOACC-NEXT: {{^ *}}      out2 += 2;
    // PRT-NOACC-NEXT: {{^ *}}    }
    // PRT-NOACC-NEXT: {{^ *}}  }
    // PRT-NOACC-NEXT: {{^ *}}}
    //
    // PRT-AO-NEXT: {{^ *}}// v----------ACC----------v
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) seq private(out0) reduction(+: out1) copyin(out2){{$}}
    // PRT-A-NEXT:  {{^ *}}for (int i = 0; i < 2; ++i) {
    // PRT-A-NEXT:  {{^ *}}  #pragma acc loop gang reduction(+: out0,out1) private(out2){{$}}
    // PRT-A-NEXT:  {{^ *}}  for (int j = 0; j < 2; ++j) {
    // PRT-A-NEXT:  {{^ *}}    #pragma acc loop worker reduction(+: out0,out1,out2){{$}}
    // PRT-A-NEXT:  {{^ *}}    for (int k = 0; k < 2; ++k) {
    // PRT-A-NEXT:  {{^ *}}      out0 += 2;
    // PRT-A-NEXT:  {{^ *}}      out1 += 2;
    // PRT-A-NEXT:  {{^ *}}      out2 += 2;
    // PRT-A-NEXT:  {{^ *}}    }
    // PRT-A-NEXT:  {{^ *}}  }
    // PRT-A-NEXT:  {{^ *}}}
    // PRT-AO-NEXT: {{^ *}}// ---------ACC->OMP--------
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) map(to: out2) map(tofrom: out1) reduction(+: out1){{$}}
    // PRT-AO-NEXT: {{^ *}}// {
    // PRT-AO-NEXT: {{^ *}}//   int out0;
    // PRT-AO-NEXT: {{^ *}}//   for (int i = 0; i < 2; ++i) {
    // PRT-AO-NEXT: {{^ *}}//     #pragma omp distribute private(out2){{$}}
    // PRT-AO-NEXT: {{^ *}}//     for (int j = 0; j < 2; ++j) {
    // PRT-AO-NEXT: {{^ *}}//       #pragma omp parallel for reduction(+: out0,out1,out2){{$}}
    // PRT-AO-NEXT: {{^ *}}//       for (int k = 0; k < 2; ++k) {
    // PRT-AO-NEXT: {{^ *}}//         out0 += 2;
    // PRT-AO-NEXT: {{^ *}}//         out1 += 2;
    // PRT-AO-NEXT: {{^ *}}//         out2 += 2;
    // PRT-AO-NEXT: {{^ *}}//       }
    // PRT-AO-NEXT: {{^ *}}//     }
    // PRT-AO-NEXT: {{^ *}}//   }
    // PRT-AO-NEXT: {{^ *}}// }
    // PRT-AO-NEXT: {{^ *}}// ^----------OMP----------^
    //
    // PRT-OA-NEXT: {{^ *}}// v----------OMP----------v
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) map(to: out2) map(tofrom: out1) reduction(+: out1){{$}}
    // PRT-O-NEXT:  {{^ *}}{
    // PRT-O-NEXT:  {{^ *}}  int out0;
    // PRT-O-NEXT:  {{^ *}}  for (int i = 0; i < 2; ++i) {
    // PRT-O-NEXT:  {{^ *}}    #pragma omp distribute private(out2){{$}}
    // PRT-O-NEXT:  {{^ *}}    for (int j = 0; j < 2; ++j) {
    // PRT-O-NEXT:  {{^ *}}      #pragma omp parallel for reduction(+: out0,out1,out2){{$}}
    // PRT-O-NEXT:  {{^ *}}      for (int k = 0; k < 2; ++k) {
    // PRT-O-NEXT:  {{^ *}}        out0 += 2;
    // PRT-O-NEXT:  {{^ *}}        out1 += 2;
    // PRT-O-NEXT:  {{^ *}}        out2 += 2;
    // PRT-O-NEXT:  {{^ *}}      }
    // PRT-O-NEXT:  {{^ *}}    }
    // PRT-O-NEXT:  {{^ *}}  }
    // PRT-O-NEXT:  {{^ *}}}
    // PRT-OA-NEXT: {{^ *}}// ---------OMP<-ACC--------
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2) seq private(out0) reduction(+: out1) copyin(out2){{$}}
    // PRT-OA-NEXT: {{^ *}}// for (int i = 0; i < 2; ++i) {
    // PRT-OA-NEXT: {{^ *}}//   #pragma acc loop gang reduction(+: out0,out1) private(out2){{$}}
    // PRT-OA-NEXT: {{^ *}}//   for (int j = 0; j < 2; ++j) {
    // PRT-OA-NEXT: {{^ *}}//     #pragma acc loop worker reduction(+: out0,out1,out2){{$}}
    // PRT-OA-NEXT: {{^ *}}//     for (int k = 0; k < 2; ++k) {
    // PRT-OA-NEXT: {{^ *}}//       out0 += 2;
    // PRT-OA-NEXT: {{^ *}}//       out1 += 2;
    // PRT-OA-NEXT: {{^ *}}//       out2 += 2;
    // PRT-OA-NEXT: {{^ *}}//     }
    // PRT-OA-NEXT: {{^ *}}//   }
    // PRT-OA-NEXT: {{^ *}}// }
    // PRT-OA-NEXT: {{^ *}}// ^----------ACC----------^
    #pragma acc parallel loop num_gangs(2) seq private(out0) reduction(+: out1) copyin(out2)
    for (int i = 0; i < 2; ++i) {
      #pragma acc loop gang reduction(+: out0,out1) private(out2)
      for (int j = 0; j < 2; ++j) {
        #pragma acc loop worker reduction(+: out0,out1,out2)
        for (int k = 0; k < 2; ++k) {
          out0 += 2;
          out1 += 2;
          out2 += 2;
        }
      }
    }
    // DMP: CallExpr
    // DMP: CallExpr
    // DMP: CallExpr
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // PRT-NEXT: printf
    // EXE-NEXT: out0 = 5
    // EXE-NEXT: out1 = 22
    // EXE-NEXT: out2 = 7
    printf("out0 = %d\n", out0);
    printf("out1 = %d\n", out1);
    printf("out2 = %d\n", out2);
  } // PRT-NEXT: }

  // PRT-NEXT: return 0;
  return 0;
} // PRT-NEXT: }
// EXE-NOT: {{.}}
