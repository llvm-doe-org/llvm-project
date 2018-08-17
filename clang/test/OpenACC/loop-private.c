// Abbreviations:
//   A      = OpenACC
//   AO     = commented OpenMP is printed after OpenACC
//   O      = OpenMP
//   OA     = commented OpenACC is printed after OpenMP
//   AOX    = either AO or OA
//   AIMP   = OpenACC implicit independent
//   ASEQ   = OpenACC seq clause
//   AG     = OpenACC gang clause
//   AW     = OpenACC worker clause
//   AV     = OpenACC vector clause
//   OPRG   = OpenACC pragma translated to OpenMP pragma
//   OPLC   = OPRG but any assigned loop control var becomes declared private
//            in an enclosing compound statement
//   OSEQ   = OpenACC loop seq discarded in translation to OpenMP
//   ONT1   = OpenMP num_threads(1)
//   GREDUN = gang redundancy
//
//   accc   = OpenACC clauses
//   ompdd  = OpenMP directive dump
//   ompdp  = OpenMP directive print
//   ompdk  = OpenMP directive kind (OPRG, OPLC, or OSEQ)
//   dmp    = additional FileCheck prefixes for dump
//   exe    = additional FileCheck prefixes for execution
//
// RUN: %data loop-clauses {
// RUN:   (accc=seq
// RUN:    ompdd=
// RUN:    ompdp=
// RUN:    ompdk=OSEQ
// RUN:    dmp=DMP-ASEQ
// RUN:    exe=EXE,EXE-GREDUN)
// RUN:   (accc=gang
// RUN:    ompdd=OMPDistributeDirective
// RUN:    ompdp=distribute
// RUN:    ompdk=OPRG
// RUN:    dmp=DMP-AIMP,DMP-AG
// RUN:    exe=EXE)
// RUN:   (accc=worker
// RUN:    ompdd=OMPParallelForDirective
// RUN:    ompdp='parallel for'
// RUN:    ompdk=OPRG
// RUN:    dmp=DMP-AIMP,DMP-AW
// RUN:    exe=EXE,EXE-GREDUN)
// RUN:   (accc=vector
// RUN:    ompdd=OMPParallelForSimdDirective
// RUN:    ompdp='parallel for simd num_threads(1)'
// RUN:    ompdk=OPLC
// RUN:    dmp=DMP-AIMP,DMP-AV,DMP-ONT1
// RUN:    exe=EXE,EXE-GREDUN)
// RUN:   (accc='gang worker'
// RUN:    ompdd=OMPDistributeParallelForDirective
// RUN:    ompdp='distribute parallel for'
// RUN:    ompdk=OPRG
// RUN:    dmp=DMP-AIMP,DMP-AG,DMP-AW
// RUN:    exe=EXE)
// RUN:   (accc='gang vector'
// RUN:    ompdd=OMPDistributeSimdDirective
// RUN:    ompdp='distribute simd'
// RUN:    ompdk=OPLC
// RUN:    dmp=DMP-AIMP,DMP-AG,DMP-AV
// RUN:    exe=EXE)
// RUN:   (accc='worker vector'
// RUN:    ompdd=OMPParallelForSimdDirective
// RUN:    ompdp='parallel for simd'
// RUN:    ompdk=OPLC
// RUN:    dmp=DMP-AIMP,DMP-AW,DMP-AV
// RUN:    exe=EXE)
// RUN:   (accc='gang worker vector'
// RUN:    ompdd=OMPDistributeParallelForSimdDirective
// RUN:    ompdp='distribute parallel for simd'
// RUN:    ompdk=OPLC
// RUN:    dmp=DMP-AIMP,DMP-AG,DMP-AW,DMP-AV
// RUN:    exe=EXE)
// RUN: }

// Check ASTDumper.
//
// RUN: %for loop-clauses {
// RUN:   %clang -Xclang -verify -Xclang -ast-dump -fsyntax-only -fopenacc \
// RUN:          -DACCC=%'accc' %s \
// RUN:   | FileCheck %s -check-prefixes=DMP,DMP-%[ompdk],%[dmp] \
// RUN:               -DOMPDD=%[ompdd]
// RUN: }

// Check -ast-print and -fopenacc-print.
//
// RUN: %clang -Xclang -verify -Xclang -ast-print -fsyntax-only %s \
// RUN: | FileCheck -check-prefixes=PRT %s
//
// RUN: %data prints {
// RUN:   (print='-Xclang -ast-print -fsyntax-only -fopenacc' prt=PRT,PRT-A)
// RUN:   (print=-fopenacc-print=acc     prt=PRT,PRT-A)
// RUN:   (print=-fopenacc-print=omp     prt=PRT,PRT-O,PRT-O-%[ompdk])
// RUN:   (print=-fopenacc-print=acc-omp prt=PRT,PRT-A,PRT-AOX,PRT-AOX-%[ompdk],PRT-AO,PRT-AO-%[ompdk])
// RUN:   (print=-fopenacc-print=omp-acc prt=PRT,PRT-O,PRT-O-%[ompdk],PRT-AOX,PRT-AOX-%[ompdk],PRT-OA,PRT-OA-%[ompdk])
// RUN: }
// RUN: %for loop-clauses {
// RUN:   %for prints {
// RUN:     %clang -Xclang -verify %[print] -DACCC=%'accc' %s \
// RUN:     | FileCheck -check-prefixes=%[prt] -DACCC=' '%'accc' \
// RUN:                 -DOMPDP=%'ompdp' %s
// RUN:   }
// RUN: }

// Check ASTWriterStmt, ASTReaderStmt, StmtPrinter, and
// ACCLoopDirective::CreateEmpty (used by ASTReaderStmt).  Some data related to
// printing (where to print comments about discarded directives) is serialized
// and deserialized, so it's worthwhile to try all OpenACC printing modes.
//
// RUN: %for loop-clauses {
// RUN:   %clang -Xclang -verify -fopenacc -emit-ast %s -DACCC=%'accc' \
// RUN:          -o %t.ast
// RUN:   %for prints {
// RUN:     %clang %[print] %t.ast 2>&1 \
// RUN:     | FileCheck -check-prefixes=%[prt] -DACCC=' '%'accc' \
// RUN:                 -DOMPDP=%'ompdp' %s
// RUN:   }
// RUN: }

// Can we -ast-print the OpenMP source code, compile, and run it successfully?
//
// RUN: %for loop-clauses {
// RUN:   %clang -Xclang -verify -fopenacc-print=omp -DACCC=%'accc' %s \
// RUN:          > %t-omp.c
// RUN:   echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN:   %clang -Xclang -verify -fopenmp -o %t %t-omp.c
// RUN:   %t | FileCheck -check-prefixes=%[exe] %s
// RUN: }

// Check execution with normal compilation.
//
// RUN: %for loop-clauses {
// RUN:   %clang -Xclang -verify -fopenacc -DACCC=%'accc' %s -o %t
// RUN:   %t 2 2>&1 | FileCheck -check-prefixes=%[exe] %s
// RUN: }

// END.

// expected-no-diagnostics

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  // Check private for scalar that is local to enclosing acc parallel.
  // EXE: private parallel-local scalar
  printf("private parallel-local scalar\n");
  // PRT-A: #pragma acc parallel
  // PRT-O: #pragma omp target teams
  #pragma acc parallel num_gangs(2)
  // PRT: {
  {
    // PRT: int i = 99;
    int i = 99;
    // DMP:           ACCLoopDirective
    // DMP-ASEQ-NEXT:   ACCSeqClause
    // DMP-ASEQ-NOT:    <implicit>
    // DMP-AG-NEXT:     ACCGangClause
    // DMP-AW-NEXT:     ACCWorkerClause
    // DMP-AV-NEXT:     ACCVectorClause
    // DMP-NEXT:        ACCPrivateClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'i' 'int'
    // DMP-AIMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-OPRG-NEXT:   impl: [[OMPDD]]
    // DMP-OPRG-NEXT:     OMPPrivateClause
    // DMP-OPRG-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    // DMP-OPRG:          ForStmt
    // DMP-OPLC-NEXT:   impl: [[OMPDD]]
    // DMP-ONT1-NEXT:     OMPNum_threadsClause
    // DMP-ONT1-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-OPLC-NEXT:     OMPPrivateClause
    // DMP-OPLC-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    // DMP-OPLC:          ForStmt
    // DMP-OSEQ-NEXT:   impl: CompoundStmt
    // DMP-OSEQ-NEXT:     DeclStmt
    // DMP-OSEQ-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-OSEQ-NEXT:     ForStmt
    //
    // Print uncommented directive.
    // PRT-A-NEXT:       {{^ *}}#pragma acc loop[[ACCC]] private(i)
    // PRT-AO-OSEQ-SAME: {{^}} // discarded in OpenMP translation
    // PRT-A-SAME:       {{^$}}
    // PRT-O-OPRG-NEXT:  {{^ *}}#pragma omp [[OMPDP]] private(i){{$}}
    // PRT-O-OPLC-NEXT:  {{^ *}}#pragma omp [[OMPDP]] private(i){{$}}
    //
    // Print commented directive when attached statements are the same.
    // PRT-AO-OPRG-NEXT: {{^ *}}// #pragma omp [[OMPDP]] private(i){{$}}
    // PRT-AO-OPLC-NEXT: {{^ *}}// #pragma omp [[OMPDP]] private(i){{$}}
    // PRT-OA-OPRG-NEXT: {{^ *}}// #pragma acc loop[[ACCC]] private(i){{$}}
    // PRT-OA-OPLC-NEXT: {{^ *}}// #pragma acc loop[[ACCC]] private(i){{$}}
    //
    // Print uncommented attached statement.
    // PRT-O-OSEQ-NEXT: {
    // PRT-O-OSEQ-NEXT:   int i;
    // PRT-NEXT:          for (int j = 0; j < 2; ++j) {
    // PRT-NEXT:            i = j;
    // PRT-NEXT:            printf
    // PRT-NEXT:          }
    // PRT-O-OSEQ-NEXT: }
    //
    // Print commented directive and attached statement when attached
    // statements are different.
    // PRT-AO-OSEQ-NEXT:  // {
    // PRT-AO-OSEQ-NEXT:  //   int i;
    // PRT-OA-OSEQ-NEXT:  //   #pragma acc loop[[ACCC]] private(i) // discarded in OpenMP translation{{$}}
    // PRT-AOX-OSEQ-NEXT: //   for (int j = 0; j < 2; ++j) {
    // PRT-AOX-OSEQ-NEXT: //     i = j;
    // PRT-AOX-OSEQ-NEXT: //     printf
    // PRT-AOX-OSEQ-NEXT: //   }
    // PRT-AO-OSEQ-NEXT:  // }
    #pragma acc loop ACCC private(i)
    for (int j = 0; j < 2; ++j) {
      i = j;
      // EXE-DAG:        in loop: 0
      // EXE-DAG:        in loop: 1
      // EXE-GREDUN-DAG: in loop: 0
      // EXE-GREDUN-DAG: in loop: 1
      printf("in loop: %d\n", i);
    }
    // PRT-NEXT:      printf
    // EXE-DAG: after loop: 99
    // EXE-DAG: after loop: 99
    printf("after loop: %d\n", i);
  // PRT-NEXT: }
  }

  // Check private for loop control variable that is declared not assigned in
  // init of attached for loop.
  // EXE: private declared loop control
  printf("private declared loop control\n");
  // PRT-A: #pragma acc parallel
  // PRT-O: #pragma omp target teams
  #pragma acc parallel num_gangs(2)
  // PRT: {
  {
    // PRT: int i = 99;
    int i = 99;
    // DMP:           ACCLoopDirective
    // DMP-ASEQ-NEXT:   ACCSeqClause
    // DMP-ASEQ-NOT:    <implicit>
    // DMP-AG-NEXT:     ACCGangClause
    // DMP-AW-NEXT:     ACCWorkerClause
    // DMP-AV-NEXT:     ACCVectorClause
    // DMP-NEXT:        ACCPrivateClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'i' 'int'
    // DMP-AIMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-OPRG-NEXT:   impl: [[OMPDD]]
    // DMP-OPRG-NEXT:     OMPPrivateClause
    // DMP-OPRG-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    // DMP-OPRG:          ForStmt
    // DMP-OPLC-NEXT:   impl: [[OMPDD]]
    // DMP-ONT1-NEXT:     OMPNum_threadsClause
    // DMP-ONT1-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-OPLC-NEXT:     OMPPrivateClause
    // DMP-OPLC-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    // DMP-OPLC:          ForStmt
    // DMP-OSEQ-NEXT:   impl: CompoundStmt
    // DMP-OSEQ-NEXT:     DeclStmt
    // DMP-OSEQ-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-OSEQ-NEXT:     ForStmt
    //
    // Print uncommented directive.
    // PRT-A-NEXT:       {{^ *}}#pragma acc loop[[ACCC]] private(i)
    // PRT-AO-OSEQ-SAME: {{^}} // discarded in OpenMP translation
    // PRT-A-SAME:       {{^$}}
    // PRT-O-OPRG-NEXT:  {{^ *}}#pragma omp [[OMPDP]] private(i){{$}}
    // PRT-O-OPLC-NEXT:  {{^ *}}#pragma omp [[OMPDP]] private(i){{$}}
    //
    // Print commented directive when attached statements are the same.
    // PRT-AO-OPRG-NEXT: {{^ *}}// #pragma omp [[OMPDP]] private(i){{$}}
    // PRT-AO-OPLC-NEXT: {{^ *}}// #pragma omp [[OMPDP]] private(i){{$}}
    // PRT-OA-OPRG-NEXT: {{^ *}}// #pragma acc loop[[ACCC]] private(i){{$}}
    // PRT-OA-OPLC-NEXT: {{^ *}}// #pragma acc loop[[ACCC]] private(i){{$}}
    //
    // Print uncommented attached statement.
    // PRT-O-OSEQ-NEXT: {
    // PRT-O-OSEQ-NEXT:   int i;
    // PRT-NEXT:          for (int i = 0; i < 2; ++i) {
    // PRT-NEXT:            printf
    // PRT-NEXT:          }
    // PRT-O-OSEQ-NEXT: }
    //
    // Print commented directive and attached statement when attached
    // statements are different.
    // PRT-AO-OSEQ-NEXT:  // {
    // PRT-AO-OSEQ-NEXT:  //   int i;
    // PRT-OA-OSEQ-NEXT:  //   #pragma acc loop[[ACCC]] private(i) // discarded in OpenMP translation{{$}}
    // PRT-AOX-OSEQ-NEXT: //   for (int i = 0; i < 2; ++i) {
    // PRT-AOX-OSEQ-NEXT: //     printf
    // PRT-AOX-OSEQ-NEXT: //   }
    // PRT-AO-OSEQ-NEXT:  // }
    #pragma acc loop ACCC private(i)
    for (int i = 0; i < 2; ++i) {
      // EXE-DAG:        in loop: 0
      // EXE-DAG:        in loop: 1
      // EXE-GREDUN-DAG: in loop: 0
      // EXE-GREDUN-DAG: in loop: 1
      printf("in loop: %d\n", i);
    }
    // PRT-NEXT:         printf
    // EXE-DAG: after loop: 99
    // EXE-DAG: after loop: 99
    printf("after loop: %d\n", i);
  // PRT-NEXT: }
  }

  // Check private for loop control variable that is assigned not declared in
  // init of attached for loop.
  // EXE: private assigned loop control
  printf("private assigned loop control\n");
  // PRT-A: #pragma acc parallel
  // PRT-O: #pragma omp target teams
  #pragma acc parallel num_gangs(2)
  // PRT: {
  {
    // PRT: int i = 99;
    int i = 99;
    // DMP:           ACCLoopDirective
    // DMP-ASEQ-NEXT:   ACCSeqClause
    // DMP-ASEQ-NOT:    <implicit>
    // DMP-AG-NEXT:     ACCGangClause
    // DMP-AW-NEXT:     ACCWorkerClause
    // DMP-AV-NEXT:     ACCVectorClause
    // DMP-NEXT:        ACCPrivateClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'i' 'int'
    // DMP-AIMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-OPRG-NEXT:   impl: [[OMPDD]]
    // DMP-OPRG-NEXT:     OMPPrivateClause
    // DMP-OPRG-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    // DMP-OPRG:          ForStmt
    // DMP-OPLC-NEXT:   impl: CompoundStmt
    // DMP-OPLC-NEXT:     DeclStmt
    // DMP-OPLC-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-OPLC-NEXT:     [[OMPDD]]
    // DMP-ONT1-NEXT:       OMPNum_threadsClause
    // DMP-ONT1-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP-OPLC:            ForStmt
    // DMP-OSEQ-NEXT:   impl: CompoundStmt
    // DMP-OSEQ-NEXT:     DeclStmt
    // DMP-OSEQ-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-OSEQ-NEXT:     ForStmt
    //
    // Print uncommented directive.
    // PRT-A-NEXT:       {{^ *}}#pragma acc loop[[ACCC]] private(i)
    // PRT-AO-OSEQ-SAME: {{^}} // discarded in OpenMP translation
    // PRT-A-SAME:       {{^$}}
    // PRT-O-OPRG-NEXT:  {{^ *}}#pragma omp [[OMPDP]] private(i){{$}}
    //
    // Print commented directive when attached statements are the same.
    // PRT-AO-OPRG-NEXT: {{^ *}}// #pragma omp [[OMPDP]] private(i){{$}}
    // PRT-OA-OPRG-NEXT: {{^ *}}// #pragma acc loop[[ACCC]] private(i){{$}}
    //
    // Print uncommented attached statement.
    // PRT-O-OPLC-NEXT: {
    // PRT-O-OSEQ-NEXT: {
    // PRT-O-OPLC-NEXT:   int i;
    // PRT-O-OSEQ-NEXT:   int i;
    // PRT-O-OPLC-NEXT:   {{^ *}}#pragma omp [[OMPDP]]{{$}}
    // PRT-NEXT:          for (i = 0; i < 2; ++i) {
    // PRT-NEXT:            printf
    // PRT-NEXT:          }
    // PRT-O-OPLC-NEXT: }
    // PRT-O-OSEQ-NEXT: }
    //
    // Print commented directive and attached statement when attached
    // statements are different.
    // PRT-AO-OPLC-NEXT:  // {
    // PRT-AO-OPLC-NEXT:  //   int i;
    // PRT-AO-OPLC-NEXT:  //   #pragma omp [[OMPDP]]{{$}}
    // PRT-OA-OPLC-NEXT:  //   #pragma acc loop[[ACCC]] private(i){{$}}
    // PRT-AOX-OPLC-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AOX-OPLC-NEXT: //     printf
    // PRT-AOX-OPLC-NEXT: //   }
    // PRT-AO-OPLC-NEXT:  // }
    // PRT-AO-OSEQ-NEXT:  // {
    // PRT-AO-OSEQ-NEXT:  //   int i;
    // PRT-OA-OSEQ-NEXT:  //   #pragma acc loop[[ACCC]] private(i) // discarded in OpenMP translation{{$}}
    // PRT-AOX-OSEQ-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AOX-OSEQ-NEXT: //     printf
    // PRT-AOX-OSEQ-NEXT: //   }
    // PRT-AO-OSEQ-NEXT:  // }
    #pragma acc loop ACCC private(i)
    for (i = 0; i < 2; ++i) {
      // EXE-DAG:        in loop: 0
      // EXE-DAG:        in loop: 1
      // EXE-GREDUN-DAG: in loop: 0
      // EXE-GREDUN-DAG: in loop: 1
      printf("in loop: %d\n", i);
    }
    // PRT-NEXT: printf
    // EXE-DAG: after loop: 99
    // EXE-DAG: after loop: 99
    printf("after loop: %d\n", i);
  // PRT-NEXT: }
  }

  // Check multiple privates in same clause and different clauses, including
  // private parallel-local scalar, private assigned loop control variable, and
  // private clauses that become empty or just smaller in translation to
  // OpenMP.
  // EXE: multiple privates
  printf("multiple privates\n");
  // PRT-A: #pragma acc parallel
  // PRT-O: #pragma omp target teams
  #pragma acc parallel num_gangs(2)
  // PRT: {
  {
    // PRT: int i = 99;
    // PRT: int j = 88;
    // PRT: int k = 77;
    int i = 99;
    int j = 88;
    int k = 77;
    // DMP:           ACCLoopDirective
    // DMP-ASEQ-NEXT:   ACCSeqClause
    // DMP-ASEQ-NOT:    <implicit>
    // DMP-AG-NEXT:     ACCGangClause
    // DMP-AW-NEXT:     ACCWorkerClause
    // DMP-AV-NEXT:     ACCVectorClause
    // DMP-NEXT:        ACCPrivateClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:          DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:          DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:        ACCPrivateClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'i' 'int'
    // DMP-AIMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-OPRG-NEXT:   impl: [[OMPDD]]
    // DMP-OPRG-NEXT:     OMPPrivateClause
    // DMP-OPRG-NEXT:       DeclRefExpr {{.*}} 'j' 'int'
    // DMP-OPRG-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    // DMP-OPRG-NEXT:       DeclRefExpr {{.*}} 'k' 'int'
    // DMP-OPRG-NEXT:     OMPPrivateClause
    // DMP-OPRG-NEXT:       DeclRefExpr {{.*}} 'i' 'int'
    // DMP-OPRG:          ForStmt
    // DMP-OPLC-NEXT:   impl: CompoundStmt
    // DMP-OPLC-NEXT:     DeclStmt
    // DMP-OPLC-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-OPLC-NEXT:     [[OMPDD]]
    // DMP-ONT1-NEXT:       OMPNum_threadsClause
    // DMP-ONT1-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP-OPLC-NEXT:       OMPPrivateClause
    // DMP-OPLC-NEXT:         DeclRefExpr {{.*}} 'j' 'int'
    // DMP-OPLC-NEXT:         DeclRefExpr {{.*}} 'k' 'int'
    // DMP-OPLC:            ForStmt
    // DMP-OSEQ-NEXT:   impl: CompoundStmt
    // DMP-OSEQ-NEXT:     DeclStmt
    // DMP-OSEQ-NEXT:       VarDecl {{.*}} j 'int'
    // DMP-OSEQ-NEXT:     DeclStmt
    // DMP-OSEQ-NEXT:       VarDecl {{.*}} i 'int'
    // DMP-OSEQ-NEXT:     DeclStmt
    // DMP-OSEQ-NEXT:       VarDecl {{.*}} k 'int'
    // DMP-OSEQ-NEXT:     ForStmt
    //
    // Print uncommented directive.
    // PRT-A-NEXT:       {{^ *}}#pragma acc loop[[ACCC]] private(j,i,k) private(i)
    // PRT-AO-OSEQ-SAME: {{^}} // discarded in OpenMP translation
    // PRT-A-SAME:       {{^$}}
    // PRT-O-OPRG-NEXT:  {{^ *}}#pragma omp [[OMPDP]] private(j,i,k) private(i){{$}}
    //
    // Print commented directive when attached statements are the same.
    // PRT-AO-OPRG-NEXT: {{^ *}}// #pragma omp [[OMPDP]] private(j,i,k) private(i){{$}}
    // PRT-OA-OPRG-NEXT: {{^ *}}// #pragma acc loop[[ACCC]] private(j,i,k) private(i){{$}}
    //
    // Print uncommented attached statement.
    // PRT-O-OPLC-NEXT: {
    // PRT-O-OSEQ-NEXT: {
    // PRT-O-OSEQ-NEXT:   int j;
    // PRT-O-OPLC-NEXT:   int i;
    // PRT-O-OSEQ-NEXT:   int i;
    // PRT-O-OSEQ-NEXT:   int k;
    // PRT-O-OPLC-NEXT:   {{^ *}}#pragma omp [[OMPDP]] private(j,k){{$}}
    // PRT-NEXT:          for (i = 0; i < 2; ++i) {
    // PRT-NEXT:            printf
    // PRT-NEXT:            j = k = 55;
    // PRT-NEXT:          }
    // PRT-O-OPLC-NEXT: }
    // PRT-O-OSEQ-NEXT: }
    //
    // Print commented directive and attached statement when attached
    // statements are different.
    // PRT-AO-OPLC-NEXT:  // {
    // PRT-AO-OPLC-NEXT:  //   int i;
    // PRT-AO-OPLC-NEXT:  //   #pragma omp [[OMPDP]] private(j,k){{$}}
    // PRT-OA-OPLC-NEXT:  //   #pragma acc loop[[ACCC]] private(j,i,k) private(i){{$}}
    // PRT-AOX-OPLC-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AOX-OPLC-NEXT: //     printf
    // PRT-AOX-OPLC-NEXT: //     j = k = 55;
    // PRT-AOX-OPLC-NEXT: //   }
    // PRT-AO-OPLC-NEXT:  // }
    // PRT-AO-OSEQ-NEXT:  // {
    // PRT-AO-OSEQ-NEXT:  //   int j;
    // PRT-AO-OSEQ-NEXT:  //   int i;
    // PRT-AO-OSEQ-NEXT:  //   int k;
    // PRT-OA-OSEQ-NEXT:  //   #pragma acc loop[[ACCC]] private(j,i,k) private(i) // discarded in OpenMP translation{{$}}
    // PRT-AOX-OSEQ-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AOX-OSEQ-NEXT: //     printf
    // PRT-AOX-OSEQ-NEXT: //     j = k = 55;
    // PRT-AOX-OSEQ-NEXT: //   }
    // PRT-AO-OSEQ-NEXT:  // }
    #pragma acc loop ACCC private(j, i, k) private(i)
    for (i = 0; i < 2; ++i) {
      // EXE-DAG:        in loop: i=0
      // EXE-DAG:        in loop: i=1
      // EXE-GREDUN-DAG: in loop: i=0
      // EXE-GREDUN-DAG: in loop: i=1
      printf("in loop: i=%d\n", i);
      j = k = 55;
    }
    // PRT-NEXT: printf
    // EXE-DAG: after loop: i=99, j=88, k=77
    // EXE-DAG: after loop: i=99, j=88, k=77
    printf("after loop: i=%d, j=%d, k=%d\n", i, j, k);
  // PRT-NEXT: }
  }

  return 0;
  // EXE-NOT: {{.}}
}
