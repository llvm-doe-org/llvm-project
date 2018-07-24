// Check ASTDumper.
//
// RUN: %clang -Xclang -verify -Xclang -ast-dump -fsyntax-only -fopenacc %s \
// RUN: | FileCheck -check-prefix=DMP %s

// Check -ast-print and -fopenacc-print.
//
// RUN: %clang -Xclang -verify -Xclang -ast-print -fsyntax-only %s \
// RUN: | FileCheck -check-prefix=PRT %s
//
// RUN: %data prints {
// RUN:   (print='-Xclang -ast-print -fsyntax-only -fopenacc' prt=PRT-A,PRT)
// RUN:   (print=-fopenacc-print=acc                          prt=PRT-A,PRT)
// RUN:   (print=-fopenacc-print=omp                          prt=PRT-O,PRT)
// RUN:   (print=-fopenacc-print=acc-omp                      prt=PRT-A,PRT-AO,PRT)
// RUN:   (print=-fopenacc-print=omp-acc                      prt=PRT-O,PRT-OA,PRT)
// RUN: }
// RUN: %for prints {
// RUN:   %clang -Xclang -verify %[print] %s \
// RUN:   | FileCheck -check-prefixes=%[prt] %s
// RUN: }

// Check ASTWriterStmt, ASTReaderStmt, StmtPrinter, and
// ACCLoopDirective::CreateEmpty (used by ASTReaderStmt).  Some data related to
// printing (where to print comments about discarded directives) is serialized
// and deserialized, so it's worthwhile to try all OpenACC printing modes.
//
// RUN: %for prints {
// RUN:   %clang -Xclang -verify -fopenacc -emit-ast %s -o %t.ast
// RUN:   %clang %[print] %t.ast 2>&1 \
// RUN:   | FileCheck -check-prefixes=%[prt] %s
// RUN: }

// Can we -ast-print the OpenMP source code, compile, and run it successfully?
//
// RUN: %clang -Xclang -verify -fopenacc-print=omp %s > %t-omp.c
// RUN: echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN: %clang -Xclang -verify -fopenmp -o %t %t-omp.c
// RUN: %t 2 2>&1 | FileCheck -check-prefix=EXE %s

// Check execution with normal compilation.
//
// RUN: %clang -Xclang -verify -fopenacc %s -o %t
// RUN: %t 2 2>&1 | FileCheck -check-prefix=EXE %s

// END.

// expected-no-diagnostics

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// PRT: int main() {
int main() {
  // PRT: printf
  // EXE: start
  printf("start\n");

  //--------------------------------------------------
  // Explicit seq
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int out = 10;
    int out = 10;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} '*'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) reduction(*: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(*: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(*: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) reduction(*: out){{$}}
    #pragma acc parallel num_gangs(2) reduction(*:out)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: float in = 10;
      float in = 10;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCSeqClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '*'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'float'
      // DMP-NEXT:   impl: ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop seq reduction(*: out) reduction(+: in)
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:  {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq reduction(*: out) reduction(+: in) // discarded in OpenMP translation{{$}}
      #pragma acc loop seq reduction(*:out) reduction(+:in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP: CompoundAssignOperator {{.*}} 'int' '*='
        // PRT-NEXT: out *= 2;
        out *= 2;
        // DMP: CompoundAssignOperator {{.*}} 'float' '+='
        // PRT-NEXT: in += 2;
        in += 2;
      }
      // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-NEXT: in = 14.0
      // EXE-NEXT: in = 14.0
      printf("in = %f\n", in);
    }
    // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 160
    printf("out = %d\n", out);
  }
  // PRT-NEXT: }

  //--------------------------------------------------
  // Implicit independent, no partitioning
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: enum E {
    // PRT-NEXT:   E0,
    // PRT-NEXT:   E1
    // PRT-NEXT: } out = E0;
    enum E {E0, E1} out = E0;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} 'max'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'enum E'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) reduction(max: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(max: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(max: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) reduction(max: out){{$}}
    #pragma acc parallel num_gangs(2) reduction(max:out)
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
      // DMP-NEXT:   impl: ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop reduction(max: out) reduction(min: in)
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:  {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop reduction(max: out) reduction(min: in) // discarded in OpenMP translation{{$}}
      #pragma acc loop reduction(max:out) reduction(min:in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP: ConditionalOperator
        // PRT-NEXT: out = E1 > out ? E1 : out;
        out = E1 > out ? E1 : out;
        // DMP: ConditionalOperator
        // PRT-NEXT: in = arr + 1 < in ? arr + 1 : in;
        in = arr + 1 < in ? arr + 1 : in;
      }
      // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-NEXT: in == arr: 1
      // EXE-NEXT: in == arr: 1
      printf("in == arr: %d\n", in == arr);
    }
    // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 1
    printf("out = %d\n", out);
  }
  // PRT-NEXT: }

  //--------------------------------------------------
  // Explicit auto with partitioning
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: _Bool out = 1;
    _Bool out = 1;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} '&'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' '_Bool'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' '_Bool'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) reduction(&: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(&: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(&: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) reduction(&: out){{$}}
    #pragma acc parallel num_gangs(2) reduction(&:out)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: short in = 3;
      short in = 3;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCAutoClause
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '&'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' '_Bool'
      // DMP-NEXT:   ACCReductionClause {{.*}} '|'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'short'
      // DMP-NEXT:   impl: ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop auto worker reduction(&: out) reduction(|: in)
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:  {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto worker reduction(&: out) reduction(|: in) // discarded in OpenMP translation{{$}}
      #pragma acc loop auto worker reduction(&:out) reduction(|:in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '_Bool' '&='
        // PRT-NEXT: out &= 0;
        out &= 0;
        // DMP: CompoundAssignOperator {{.*}} 'short' '|='
        // PRT-NEXT: in |= 10;
        in |= 10;
      }
      // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-NEXT: in: 11
      // EXE-NEXT: in: 11
      printf("in: %d\n", 11);
    }
    // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 0
    printf("out = %d\n", out);
  }
  // PRT-NEXT: }

  // PRT-NEXT: return 0;
  return 0;
}
// PRT-NEXT: }
// EXE-NOT: {{.}}
