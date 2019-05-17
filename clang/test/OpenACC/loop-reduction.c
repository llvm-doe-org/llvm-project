// Check reduction clause on "acc loop" and on "acc parallel loop".

// Check ASTDumper.
//
// RUN: %clang -Xclang -verify -Xclang -ast-dump -fsyntax-only -fopenacc %s \
// RUN: | FileCheck -check-prefix=DMP %s

// Check -ast-print and -fopenacc[-ast]-print.
//
// RUN: %clang -Xclang -verify -Xclang -ast-print -fsyntax-only %s \
// RUN: | FileCheck -check-prefix=PRT %s
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
// RUN:   (prt=-fopenacc-print=acc                          prt-chk=PRT-A,PRT)
// RUN:   (prt=-fopenacc-print=omp                          prt-chk=PRT-O,PRT)
// RUN:   (prt=-fopenacc-print=acc-omp                      prt-chk=PRT-A,PRT-AO,PRT)
// RUN:   (prt=-fopenacc-print=omp-acc                      prt-chk=PRT-O,PRT-OA,PRT)
// RUN: }
// RUN: %for prt-args {
// RUN:   %clang -Xclang -verify %[prt] %t-acc.c \
// RUN:   | FileCheck -check-prefixes=%[prt-chk] %s
// RUN: }

// Check ASTWriterStmt, ASTReaderStmt, StmtPrinter, and
// ACCLoopDirective::CreateEmpty (used by ASTReaderStmt).  Some data related to
// printing (where to print comments about discarded directives) is serialized
// and deserialized, so it's worthwhile to try all OpenACC printing modes.
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
// RUN:   %clang -Xclang -verify -fopenmp -o %t %t-omp.c %libatomic
// RUN:   %t 2 2>&1 | FileCheck -check-prefix=EXE %s
// RUN: }

// Check execution with normal compilation.
//
// RUN: %clang -Xclang -verify -fopenacc %s -o %t %libatomic
// RUN: %t 2 2>&1 | FileCheck -check-prefix=EXE %s

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
    #pragma acc parallel num_gangs(2) reduction(*: out)
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
      #pragma acc loop seq reduction(*: out) reduction(+: in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP: CompoundAssignOperator {{.*}} 'int' '*='
        // PRT-NEXT: out *= 2;
        out *= 2;
        // DMP: CompoundAssignOperator {{.*}} 'float' '+='
        // PRT-NEXT: in += 2;
        in += 2;
      } // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-NEXT: in = 14.0
      // EXE-NEXT: in = 14.0
      printf("in = %.1f\n", in);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 160
    printf("out = %d\n", out);
  } // PRT-NEXT: }

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
    // DMP-NEXT:     ACCReductionClause {{.*}} '*'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCSeqClause
    // DMP-NEXT:       ACCReductionClause {{.*}} '*'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       impl: ForStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) seq reduction(*: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(*: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(*: out){{$}}
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
  // Implicit independent, no partitioning.
  //--------------------------------------------------

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
      // DMP-NEXT:   impl: ForStmt
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop reduction(max: out) reduction(min: in)
      // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
      // PRT-A-SAME:  {{^$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop reduction(max: out) reduction(min: in) // discarded in OpenMP translation{{$}}
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
    // DMP-NEXT:     ACCReductionClause {{.*}} 'max'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCReductionClause {{.*}} 'max'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'enum E'
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       impl: ForStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2) reduction(max: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(max: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(max: out){{$}}
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
    #pragma acc parallel num_gangs(2) reduction(&: out)
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
      #pragma acc loop auto worker reduction(&: out) reduction(|: in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '_Bool' '&='
        // PRT-NEXT: out &= 0;
        out &= 0;
        // DMP: CompoundAssignOperator {{.*}} 'short' '|='
        // PRT-NEXT: in |= 10;
        in |= 10;
      } // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-NEXT: in: 11
      // EXE-NEXT: in: 11
      printf("in: %d\n", in);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 0
    printf("out = %d\n", out);
  } // PRT-NEXT: }

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
    // DMP-NEXT:     ACCReductionClause {{.*}} '&'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' '_Bool'
    // DMP-NEXT:     ACCNum_gangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' '_Bool'
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCReductionClause {{.*}} '&'
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' '_Bool'
    // DMP-NEXT:       ACCAutoClause
    // DMP-NEXT:       ACCWorkerClause
    // DMP-NEXT:       impl: ForStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop reduction(&: out) auto worker num_gangs(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams reduction(&: out) num_teams(2){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams reduction(&: out) num_teams(2){{$}}
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
    // EXE-NEXT: out = 0
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Worker partitioned.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: _Complex double out = 2;
    _Complex double out = 2;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} '&&'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' '_Complex double'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' '_Complex double'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) reduction(&&: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(&&: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(&&: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) reduction(&&: out){{$}}
    #pragma acc parallel num_gangs(2) reduction(&&: out)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: double in = 0;
      double in = 0;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '&&'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' '_Complex double'
      // DMP-NEXT:   ACCReductionClause {{.*}} '||'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'double'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPParallelForDirective
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out' '_Complex double'
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'in' 'double'
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker reduction(&&: out) reduction(||: in){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for reduction(&&: out) reduction(||: in){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for reduction(&&: out) reduction(||: in){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker reduction(&&: out) reduction(||: in){{$}}
      #pragma acc loop worker reduction(&&: out) reduction(||: in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP: BinaryOperator {{.*}} '&&'
        // PRT-NEXT: out = out && 0;
        out = out && 0;
        // DMP: BinaryOperator {{.*}} '||'
        // PRT-NEXT: in = in || 10;
        in = in || 10;
      } // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-NEXT: in: 1.0
      // EXE-NEXT: in: 1.0
      printf("in: %.1f\n", in);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 0.0 + 0.0i
    printf("out = %.1f + %.1fi\n", creal(out), cimag(out));
  } // PRT-NEXT: }

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
    // DMP-NEXT:     ACCReductionClause {{.*}} '&&'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' '_Complex double'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
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
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(&&: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for reduction(&&: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(&&: out){{$}}
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

  // PRT-NEXT: {
  {
    // PRT-NEXT: long out = 5;
    long out = 5;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'long'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'long'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(+: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) reduction(+: out){{$}}
    #pragma acc parallel num_gangs(2) reduction(+: out)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: long double in = -1;
      long double in = -1;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '+
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'long'
      // DMP-NEXT:   ACCReductionClause {{.*}} '*
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'long double'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPParallelForSimdDirective
      // DMP-NEXT:     OMPNum_threadsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'long'
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'in' 'long double'
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop vector reduction(+: out) reduction(*: in){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for simd num_threads(1) reduction(+: out) reduction(*: in){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for simd num_threads(1) reduction(+: out) reduction(*: in){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector reduction(+: out) reduction(*: in){{$}}
      #pragma acc loop vector reduction(+: out) reduction(*: in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 2; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: out += 10;
        out += 10;
        // DMP: CompoundAssignOperator {{.*}} '*='
        // PRT-NEXT: in *= 3;
        in *= 3;
      } // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-NEXT: in: -9.0
      // EXE-NEXT: in: -9.0
      printf("in: %.1Lf\n", in);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 45
    printf("out = %ld\n", out);
  } // PRT-NEXT: }

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
    // DMP-NEXT:     ACCReductionClause {{.*}} '+'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'long'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
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
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for simd num_threads(1) reduction(+: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(+: out){{$}}
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

  // PRT-NEXT: {
  {
    // PRT-NEXT: float out = -5;
    float out = -5;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} '*'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'float'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'float'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) reduction(*: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(*: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(*: out){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) reduction(*: out){{$}}
    #pragma acc parallel num_gangs(2) reduction(*: out)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: int in = 3;
      int in = 3;
      // DMP:      ACCLoopDirective
      // DMP-NEXT:   ACCWorkerClause
      // DMP-NEXT:   ACCVectorClause
      // DMP-NEXT:   ACCReductionClause {{.*}} '*'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'float'
      // DMP-NEXT:   ACCReductionClause {{.*}} '+'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'in' 'int'
      // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
      // DMP-NEXT:   impl: OMPParallelForSimdDirective
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'float'
      // DMP-NEXT:     OMPReductionClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'in' 'int'
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc loop worker vector reduction(*: out) reduction(+: in){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for simd reduction(*: out) reduction(+: in){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp parallel for simd reduction(*: out) reduction(+: in){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector reduction(*: out) reduction(+: in){{$}}
      #pragma acc loop worker vector reduction(*: out) reduction(+: in)
      // PRT-NEXT: for ({{.*}}) {
      for (int i = 0; i < 4; ++i) {
        // DMP: CompoundAssignOperator {{.*}} '*='
        // PRT-NEXT: out *= 3;
        out *= 3;
        // DMP: CompoundAssignOperator {{.*}} '+='
        // PRT-NEXT: in += 4;
        in += 4;
      } // PRT-NEXT: }
      // DMP: CallExpr
      // PRT-NEXT: printf
      // EXE-NEXT: in: 19
      // EXE-NEXT: in: 19
      printf("in: %d\n", in);
    } // PRT-NEXT: }
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = -32805.0
    printf("out = %.1f\n", out);
  } // PRT-NEXT: }

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
    // DMP-NEXT:     ACCReductionClause {{.*}} '*'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'float'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
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
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(*: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp parallel for simd reduction(*: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(*: out){{$}}
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
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'double'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(+: out){{$}}
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
    // DMP-NEXT:     ACCReductionClause {{.*}} '+'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'double'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
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
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(+: out){{$}}
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
  // Gang partitioned, implicit acc parallel reduction.
  //
  // There's nothing much new to test here for acc parallel loop.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: double out =
    double out = 0.3;
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNum_gangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'double'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'double'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(+: out){{$}}
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
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2) reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(+: out){{$}}
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
    // DMP-NEXT:     ACCReductionClause {{.*}} '+'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
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
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for reduction(+: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(+: out){{$}}
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
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(+: out){{$}}
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
    // DMP-NEXT:     ACCReductionClause {{.*}} '+'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     ACCNum_gangsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
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
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams reduction(+: out) num_teams(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute simd reduction(+: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams reduction(+: out) num_teams(2){{$}}
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
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(+: out){{$}}
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
    // DMP-NEXT:     ACCReductionClause {{.*}} '+'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
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
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for simd reduction(+: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(+: out){{$}}
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
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(+: out){{$}}
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
    // DMP-NEXT:     ACCReductionClause {{.*}} '+'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
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
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(+: out){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(+: out){{$}}
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
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(+: out){{$}}
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
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(+: out){{$}}
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
    // DMP-NEXT:   ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:     OMPReductionClause
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(+: out){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(+: out){{$}}
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
    // DMP-NEXT:     ACCReductionClause {{.*}} <implicit> '+'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:     impl: OMPTargetTeamsDirective
    // DMP-NEXT:       OMPNum_teamsClause
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPReductionClause
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP:          ACCLoopDirective
    // DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:         DeclRefExpr {{.*}} 'out' 'int'
    // DMP-NEXT:       impl: ForStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel loop num_gangs(2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) reduction(+: out){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(2) reduction(+: out){{$}}
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
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: out = 69
    printf("out = %d\n", out);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Gang reduction on gang-local private copies of variables.
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

  // PRT-NEXT: return 0;
  return 0;
} // PRT-NEXT: }
// EXE-NOT: {{.}}
