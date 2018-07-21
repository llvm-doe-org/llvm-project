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
// RUN: %clang -Xclang -verify -fopenacc-print=omp %s -DCOMPILE > %t-omp.c
// RUN: echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN: %clang -Xclang -verify -fopenmp -o %t %t-omp.c
// RUN: %t 2 2>&1 | FileCheck -check-prefix=EXE %s

// Check execution with normal compilation.
//
// RUN: %clang -Xclang -verify -fopenacc %s -o %t -DCOMPILE
// RUN: %t 2 2>&1 | FileCheck -check-prefix=EXE %s

// END.

// expected-no-diagnostics

#include <assert.h>
#include <complex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// PRT: int main() {
int main() {
  // PRT: printf
  // EXE: start
  printf("start\n");

  //--------------------------------------------------
  // Reduction operator '+'
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int acc = 10;
    // PRT-NEXT: int val = 2;
    int acc = 10;
    int val = 2;
    // DMP-LABEL: ACCParallelDirective
    // DMP-NEXT:    ACCNum_gangsClause
    // DMP-NEXT:      IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:    ACCReductionClause {{.*}} '+'
    // DMP-NEXT:      DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:    ACCFirstprivateClause {{.*}}
    // DMP-NOT:       <implicit>
    // DMP-NEXT:      DeclRefExpr {{.*}} 'val' 'int'
    // DMP-NEXT:    impl: OMPTargetTeamsDirective
    // DMP-NEXT:      OMPNum_teamsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:      OMPReductionClause
    // DMP-NEXT:        DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:      OMPFirstprivateClause
    // DMP-NOT:         <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'val' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(4) reduction(+: acc) firstprivate(val){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) reduction(+: acc) firstprivate(val){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(4) reduction(+: acc) firstprivate(val){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(4) reduction(+: acc) firstprivate(val){{$}}
    #pragma acc parallel num_gangs(4) reduction(+:acc) firstprivate(val)
    // DMP: CompoundAssignOperator {{.*}} 'int' '+='
    // PRT-NEXT: acc += val;
    acc += val;
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: acc = 18
    printf("acc = %d\n", acc);
  }
  // PRT-NEXT: }

  //--------------------------------------------------
  // Reduction operator '*'
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: float acc = 10;
    // PRT-NEXT: float val = 2;
    float acc = 10;
    float val = 2;
    // DMP-LABEL: ACCParallelDirective
    // DMP-NEXT:    ACCNum_gangsClause
    // DMP-NEXT:      IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:    ACCReductionClause {{.*}} '*'
    // DMP-NEXT:      DeclRefExpr {{.*}} 'acc' 'float'
    // DMP-NEXT:    ACCFirstprivateClause {{.*}}
    // DMP-NOT:       <implicit>
    // DMP-NEXT:      DeclRefExpr {{.*}} 'val' 'float'
    // DMP-NEXT:    impl: OMPTargetTeamsDirective
    // DMP-NEXT:      OMPNum_teamsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:      OMPReductionClause
    // DMP-NEXT:        DeclRefExpr {{.*}} 'acc' 'float'
    // DMP-NEXT:      OMPFirstprivateClause
    // DMP-NOT:         <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'val' 'float'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(4) reduction(*: acc) firstprivate(val){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) reduction(*: acc) firstprivate(val){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(4) reduction(*: acc) firstprivate(val){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(4) reduction(*: acc) firstprivate(val){{$}}
    #pragma acc parallel num_gangs(4) reduction(*:acc) firstprivate(val)
    // DMP: CompoundAssignOperator {{.*}} 'float' '*='
    // PRT-NEXT: acc *= val;
    acc *= val;
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: acc = 160.0
    printf("acc = %.1f\n", acc);
  }
  // PRT-NEXT: }

  //--------------------------------------------------
  // Reduction operator 'max'
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int arr[]
    // PRT-NEXT: int *acc
    // PRT-NEXT: int *val
    int arr[] = {0, 1, 2};
    int *acc = arr;
    int *val = arr + 2;
    // DMP-LABEL: ACCParallelDirective
    // DMP-NEXT:    ACCNum_gangsClause
    // DMP-NEXT:      IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:    ACCReductionClause {{.*}} 'max'
    // DMP-NEXT:      DeclRefExpr {{.*}} 'acc' 'int *'
    // DMP-NEXT:    ACCFirstprivateClause {{.*}}
    // DMP-NOT:       <implicit>
    // DMP-NEXT:      DeclRefExpr {{.*}} 'val' 'int *'
    // DMP-NEXT:    impl: OMPTargetTeamsDirective
    // DMP-NEXT:      OMPNum_teamsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:      OMPReductionClause
    // DMP-NEXT:        DeclRefExpr {{.*}} 'acc' 'int *'
    // DMP-NEXT:      OMPFirstprivateClause
    // DMP-NOT:         <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'val' 'int *'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(4) reduction(max: acc) firstprivate(val){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) reduction(max: acc) firstprivate(val){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(4) reduction(max: acc) firstprivate(val){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(4) reduction(max: acc) firstprivate(val){{$}}
    #pragma acc parallel num_gangs(4) reduction(max:acc) firstprivate(val)
    // DMP: ConditionalOperator {{.*}} 'int *'
    // PRT-NEXT: acc = val > acc ? val : acc;
    acc = val > acc ? val : acc;
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: acc == arr + 2: 1
    printf("acc == arr + 2: %d\n", acc == arr + 2);
  }
  // PRT-NEXT: }

  //--------------------------------------------------
  // Reduction operator 'min'
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: double acc = 10;
    // PRT-NEXT: double val = 2;
    double acc = 10;
    double val = 2;
    // DMP-LABEL: ACCParallelDirective
    // DMP-NEXT:    ACCNum_gangsClause
    // DMP-NEXT:      IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:    ACCReductionClause {{.*}} 'min'
    // DMP-NEXT:      DeclRefExpr {{.*}} 'acc' 'double'
    // DMP-NEXT:    ACCFirstprivateClause {{.*}}
    // DMP-NOT:       <implicit>
    // DMP-NEXT:      DeclRefExpr {{.*}} 'val' 'double'
    // DMP-NEXT:    impl: OMPTargetTeamsDirective
    // DMP-NEXT:      OMPNum_teamsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:      OMPReductionClause
    // DMP-NEXT:        DeclRefExpr {{.*}} 'acc' 'double'
    // DMP-NEXT:      OMPFirstprivateClause
    // DMP-NOT:         <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'val' 'double'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(4) reduction(min: acc) firstprivate(val){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) reduction(min: acc) firstprivate(val){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(4) reduction(min: acc) firstprivate(val){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(4) reduction(min: acc) firstprivate(val){{$}}
    #pragma acc parallel num_gangs(4) reduction(min:acc) firstprivate(val)
    // DMP: ConditionalOperator {{.*}} 'double'
    // PRT-NEXT: acc = val < acc ? val : acc;
    acc = val < acc ? val : acc;
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: acc = 2.0
    printf("acc = %.1f\n", acc);
  }
  // PRT-NEXT: }

  //--------------------------------------------------
  // Reduction operator '&'
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: char acc = 10;
    // PRT-NEXT: char val = 3;
    char acc = 10; // 1010
    char val = 3;  // 0011
    // DMP-LABEL: ACCParallelDirective
    // DMP-NEXT:    ACCNum_gangsClause
    // DMP-NEXT:      IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:    ACCReductionClause {{.*}} '&'
    // DMP-NEXT:      DeclRefExpr {{.*}} 'acc' 'char'
    // DMP-NEXT:    ACCFirstprivateClause {{.*}}
    // DMP-NOT:       <implicit>
    // DMP-NEXT:      DeclRefExpr {{.*}} 'val' 'char'
    // DMP-NEXT:    impl: OMPTargetTeamsDirective
    // DMP-NEXT:      OMPNum_teamsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:      OMPReductionClause
    // DMP-NEXT:        DeclRefExpr {{.*}} 'acc' 'char'
    // DMP-NEXT:      OMPFirstprivateClause
    // DMP-NOT:         <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'val' 'char'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(4) reduction(&: acc) firstprivate(val){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) reduction(&: acc) firstprivate(val){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(4) reduction(&: acc) firstprivate(val){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(4) reduction(&: acc) firstprivate(val){{$}}
    #pragma acc parallel num_gangs(4) reduction(&:acc) firstprivate(val)
    // DMP: CompoundAssignOperator {{.*}} 'char' '&='
    // PRT-NEXT: acc &= val;
    acc &= val;
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: acc = 2
    printf("acc = %d\n", acc);
  }
  // PRT-NEXT: }

  //--------------------------------------------------
  // Reduction operator '|'
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int acc = 10;
    // PRT-NEXT: int val = 3;
    int acc = 10; // 1010
    int val = 3;  // 0011
    // DMP-LABEL: ACCParallelDirective
    // DMP-NEXT:    ACCNum_gangsClause
    // DMP-NEXT:      IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:    ACCReductionClause {{.*}} '|'
    // DMP-NEXT:      DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:    ACCFirstprivateClause {{.*}}
    // DMP-NOT:       <implicit>
    // DMP-NEXT:      DeclRefExpr {{.*}} 'val' 'int'
    // DMP-NEXT:    impl: OMPTargetTeamsDirective
    // DMP-NEXT:      OMPNum_teamsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:      OMPReductionClause
    // DMP-NEXT:        DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:      OMPFirstprivateClause
    // DMP-NOT:         <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'val' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(4) reduction(|: acc) firstprivate(val){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) reduction(|: acc) firstprivate(val){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(4) reduction(|: acc) firstprivate(val){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(4) reduction(|: acc) firstprivate(val){{$}}
    #pragma acc parallel num_gangs(4) reduction(|:acc) firstprivate(val)
    // DMP: CompoundAssignOperator {{.*}} 'int' '|='
    // PRT-NEXT: acc |= val;
    acc |= val;
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: acc = 11
    printf("acc = %d\n", acc);
  }
  // PRT-NEXT: }

  //--------------------------------------------------
  // Reduction operator '^'
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int acc = 10;
    // PRT-NEXT: int val = 3;
    int acc = 10; // 1010
    int val = 3;  // 0011
    // DMP-LABEL: ACCParallelDirective
    // DMP-NEXT:    ACCNum_gangsClause
    // DMP-NEXT:      IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:    ACCReductionClause {{.*}} '^'
    // DMP-NEXT:      DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:    ACCFirstprivateClause {{.*}}
    // DMP-NOT:       <implicit>
    // DMP-NEXT:      DeclRefExpr {{.*}} 'val' 'int'
    // DMP-NEXT:    impl: OMPTargetTeamsDirective
    // DMP-NEXT:      OMPNum_teamsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:      OMPReductionClause
    // DMP-NEXT:        DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:      OMPFirstprivateClause
    // DMP-NOT:         <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'val' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(3) reduction(^: acc) firstprivate(val){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(3) reduction(^: acc) firstprivate(val){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(3) reduction(^: acc) firstprivate(val){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(3) reduction(^: acc) firstprivate(val){{$}}
    #pragma acc parallel num_gangs(3) reduction(^:acc) firstprivate(val)
    // DMP: CompoundAssignOperator {{.*}} 'int' '^='
    // PRT-NEXT: acc ^= val;
    acc ^= val;
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: acc = 9
    printf("acc = %d\n", acc);
  }
  // PRT-NEXT: }

  //--------------------------------------------------
  // Reduction operator '&&'
  //--------------------------------------------------

  // PRT-NEXT: int inis[] = {{.*}};
  // PRT-NEXT: int vals[] = {{.*}};
  int inis[] = {0, 10, 0, 2};
  int vals[] = {0, 0,  1, 3};
  assert(sizeof(inis) == sizeof(vals));

  // PRT: for ({{.*}}) {
  for (int i = 0; i < sizeof(inis)/sizeof(*inis); ++i) {
    // PRT-NEXT: acc = inis[i];
    // PRT-NEXT: val = vals[i];
    int acc = inis[i];
    int val = vals[i];
    // DMP-LABEL: ACCParallelDirective
    // DMP-NEXT:    ACCNum_gangsClause
    // DMP-NEXT:      IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:    ACCReductionClause {{.*}} '&&'
    // DMP-NEXT:      DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:    ACCFirstprivateClause {{.*}}
    // DMP-NOT:       <implicit>
    // DMP-NEXT:      DeclRefExpr {{.*}} 'val' 'int'
    // DMP-NEXT:    impl: OMPTargetTeamsDirective
    // DMP-NEXT:      OMPNum_teamsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:      OMPReductionClause
    // DMP-NEXT:        DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:      OMPFirstprivateClause
    // DMP-NOT:         <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'val' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(4) reduction(&&: acc) firstprivate(val){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) reduction(&&: acc) firstprivate(val){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(4) reduction(&&: acc) firstprivate(val){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(4) reduction(&&: acc) firstprivate(val){{$}}
    #pragma acc parallel num_gangs(4) reduction(&&:acc) firstprivate(val)
    // DMP: BinaryOperator {{.*}} 'int' '&&'
    // PRT-NEXT: acc = acc && val;
    acc = acc && val;
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: acc = 0
    // EXE-NEXT: acc = 0
    // EXE-NEXT: acc = 0
    // EXE-NEXT: acc = 1
    printf("acc = %d\n", acc);
  }
  // PRT-NEXT: }

  //--------------------------------------------------
  // Reduction operator '||'
  //--------------------------------------------------

  // PRT-NEXT: for ({{.*}}) {
  for (int i = 0; i < sizeof(inis)/sizeof(*inis); ++i) {
    // PRT-NEXT: int acc = inis[i];
    // PRT-NEXT: int val = vals[i];
    int acc = inis[i];
    int val = vals[i];
    // DMP-LABEL: ACCParallelDirective
    // DMP-NEXT:    ACCNum_gangsClause
    // DMP-NEXT:      IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:    ACCReductionClause {{.*}} '||'
    // DMP-NEXT:      DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:    ACCFirstprivateClause {{.*}}
    // DMP-NOT:       <implicit>
    // DMP-NEXT:      DeclRefExpr {{.*}} 'val' 'int'
    // DMP-NEXT:    impl: OMPTargetTeamsDirective
    // DMP-NEXT:      OMPNum_teamsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:      OMPReductionClause
    // DMP-NEXT:        DeclRefExpr {{.*}} 'acc' 'int'
    // DMP-NEXT:      OMPFirstprivateClause
    // DMP-NOT:         <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'val' 'int'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(4) reduction(||: acc) firstprivate(val){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) reduction(||: acc) firstprivate(val){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(4) reduction(||: acc) firstprivate(val){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(4) reduction(||: acc) firstprivate(val){{$}}
    #pragma acc parallel num_gangs(4) reduction(||:acc) firstprivate(val)
    // DMP: BinaryOperator {{.*}} 'int' '||'
    // PRT-NEXT: acc = acc || val;
    acc = acc || val;
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: acc = 0
    // EXE-NEXT: acc = 1
    // EXE-NEXT: acc = 1
    // EXE-NEXT: acc = 1
    printf("acc = %d\n", acc);
  }
  // PRT-NEXT: }

  //--------------------------------------------------
  // Enumerated argument type
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: enum E {
    // PRT: E10
    // PRT-NEXT: };
    enum E {E0, E1, E2, E3, E4, E5, E6, E7, E8, E9, E10};
    // PRT-NEXT: enum E acc = E3;
    // PRT-NEXT: enum E val = E10;
    enum E acc = E3;
    enum E val = E10;
    // DMP-LABEL: ACCParallelDirective
    // DMP-NEXT:    ACCNum_gangsClause
    // DMP-NEXT:      IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:    ACCReductionClause {{.*}} '&'
    // DMP-NEXT:      DeclRefExpr {{.*}} 'acc' 'enum E'
    // DMP-NEXT:    ACCFirstprivateClause {{.*}}
    // DMP-NOT:       <implicit>
    // DMP-NEXT:      DeclRefExpr {{.*}} 'val' 'enum E'
    // DMP-NEXT:    impl: OMPTargetTeamsDirective
    // DMP-NEXT:      OMPNum_teamsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:      OMPReductionClause
    // DMP-NEXT:        DeclRefExpr {{.*}} 'acc' 'enum E'
    // DMP-NEXT:      OMPFirstprivateClause
    // DMP-NOT:         <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'val' 'enum E'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(4) reduction(&: acc) firstprivate(val){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) reduction(&: acc) firstprivate(val){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(4) reduction(&: acc) firstprivate(val){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(4) reduction(&: acc) firstprivate(val){{$}}
    #pragma acc parallel num_gangs(4) reduction(&:acc) firstprivate(val)
    // DMP: CompoundAssignOperator {{.*}}'enum E' '&='
    // PRT-NEXT: acc &= val;
    acc &= val;
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: acc == E2: 1
    printf("acc == E2: %d\n", acc == E2);
  }
  // PRT-NEXT: }

  //--------------------------------------------------
  // bool argument type
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: _Bool acc = 3;
    // PRT-NEXT: _Bool val = 0;
    bool acc = 3;
    bool val = 0;
    // DMP-LABEL: ACCParallelDirective
    // DMP-NEXT:    ACCNum_gangsClause
    // DMP-NEXT:      IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:    ACCReductionClause {{.*}} '|'
    // DMP-NEXT:      DeclRefExpr {{.*}} 'acc' 'bool'
    // DMP-NEXT:    ACCFirstprivateClause {{.*}}
    // DMP-NOT:       <implicit>
    // DMP-NEXT:      DeclRefExpr {{.*}} 'val' 'bool'
    // DMP-NEXT:    impl: OMPTargetTeamsDirective
    // DMP-NEXT:      OMPNum_teamsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:      OMPReductionClause
    // DMP-NEXT:        DeclRefExpr {{.*}} 'acc' 'bool'
    // DMP-NEXT:      OMPFirstprivateClause
    // DMP-NOT:         <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'val' 'bool'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(4) reduction(|: acc) firstprivate(val){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) reduction(|: acc) firstprivate(val){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(4) reduction(|: acc) firstprivate(val){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(4) reduction(|: acc) firstprivate(val){{$}}
    #pragma acc parallel num_gangs(4) reduction(|:acc) firstprivate(val)
    // DMP: CompoundAssignOperator {{.*}} 'bool' '|='
    // PRT-NEXT: acc |= val;
    acc |= val;
    // DMP: CallExpr
    // PRT-NEXT: printf
    // EXE-NEXT: acc: 1
    printf("acc: %d\n", acc);
  }
  // PRT-NEXT: }

  //--------------------------------------------------
  // Complex argument type
  //--------------------------------------------------

  // FIXME: OpenMP fails to link the generated code, which does appear to be
  // correct, so we suppress full compilation for now.  Be sure to change XEXE
  // below to EXE when adding this back.
#if !COMPILE
  // PRT-NEXT: {
  {
    // PRT-NEXT: _Complex float acc
    // PRT-NEXT: _Complex float val
    float complex acc = 10 + 1*I;
    float complex val = 2 + 3*I;
    // DMP-LABEL: ACCParallelDirective
    // DMP-NEXT:    ACCNum_gangsClause
    // DMP-NEXT:      IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:    ACCReductionClause {{.*}} '+'
    // DMP-NEXT:      DeclRefExpr {{.*}} 'acc' '_Complex float'
    // DMP-NEXT:    ACCFirstprivateClause {{.*}}
    // DMP-NOT:       <implicit>
    // DMP-NEXT:      DeclRefExpr {{.*}} 'val' '_Complex float'
    // DMP-NEXT:    impl: OMPTargetTeamsDirective
    // DMP-NEXT:      OMPNum_teamsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:      OMPReductionClause
    // DMP-NEXT:        DeclRefExpr {{.*}} 'acc' '_Complex float'
    // DMP-NEXT:      OMPFirstprivateClause
    // DMP-NOT:         <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'val' '_Complex float'
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(4) reduction(+: acc) firstprivate(val){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(4) reduction(+: acc) firstprivate(val){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(4) reduction(+: acc) firstprivate(val){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(4) reduction(+: acc) firstprivate(val){{$}}
    #pragma acc parallel num_gangs(4) reduction(+:acc) firstprivate(val)
    // DMP: CompoundAssignOperator {{.*}} '_Complex float' '+='
    // PRT-NEXT: acc += val;
    acc += val;
    // DMP: CallExpr
    // PRT-NEXT: printf
    // XEXE-NEXT: acc = 18.0 + 13.0i
    printf("acc = %.1f + %.1fi\n", creal(acc), cimag(acc));
  }
  // PRT-NEXT: }
#endif

  // PRT-NEXT: return 0;
  return 0;
}
// PRT-NEXT: }
// EXE-NOT: {{.}}
