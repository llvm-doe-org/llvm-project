// Check private clause on "acc loop" and on "acc parallel loop".
//
// Abbreviations:
//   A      = OpenACC
//   AO     = commented OpenMP is printed after OpenACC
//   O      = OpenMP
//   OA     = commented OpenACC is printed after OpenMP
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
// RUN:    exe=EXE,EXE-GREDUN)
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
// (which would need additional fields) within prt-args after the first
// prt-args iteration, significantly shortening the prt-args definition.
//
// RUN: %data prt-opts {
// RUN:   (prt-opt=-fopenacc-ast-print)
// RUN:   (prt-opt=-fopenacc-print    )
// RUN: }
// RUN: %data prt-args {
// RUN:   (prt='-Xclang -ast-print -fsyntax-only -fopenacc' ACCC="%'accc'" prt-chk=PRT,PRT-A)
// RUN:   (prt=-fopenacc-ast-print=acc                      ACCC="%'accc'" prt-chk=PRT,PRT-A)
// RUN:   (prt=-fopenacc-ast-print=omp                      ACCC="%'accc'" prt-chk=PRT,PRT-O,PRT-O-%[ompdk])
// RUN:   (prt=-fopenacc-ast-print=acc-omp                  ACCC="%'accc'" prt-chk=PRT,PRT-A,PRT-AO,PRT-AO-%[ompdk])
// RUN:   (prt=-fopenacc-ast-print=omp-acc                  ACCC="%'accc'" prt-chk=PRT,PRT-O,PRT-O-%[ompdk],PRT-OA,PRT-OA-%[ompdk])
// RUN:   (prt=-fopenacc-print=acc                          ACCC=ACCC      prt-chk=PRT,PRT-A)
// RUN:   (prt=-fopenacc-print=omp                          ACCC=ACCC      prt-chk=PRT,PRT-O,PRT-O-%[ompdk])
// RUN:   (prt=-fopenacc-print=acc-omp                      ACCC=ACCC      prt-chk=PRT,PRT-A,PRT-AO,PRT-AO-%[ompdk])
// RUN:   (prt=-fopenacc-print=omp-acc                      ACCC=ACCC      prt-chk=PRT,PRT-O,PRT-O-%[ompdk],PRT-OA,PRT-OA-%[ompdk])
// RUN: }
// RUN: %for loop-clauses {
// RUN:   %for prt-args {
// RUN:     %clang -Xclang -verify %[prt] -DACCC=%'accc' %t-acc.c \
// RUN:     | FileCheck -check-prefixes=%[prt-chk] -DACCC=' '%[ACCC] \
// RUN:                 -DOMPDP=%'ompdp' %s
// RUN:   }
// RUN: }

// Check ASTWriterStmt, ASTReaderStmt, StmtPrinter, and
// ACCLoopDirective::CreateEmpty (used by ASTReaderStmt).  Some data related to
// printing (where to print comments about discarded directives) is serialized
// and deserialized, so it's worthwhile to try all OpenACC printing modes.
//
// RUN: %for loop-clauses {
// RUN:   %clang -Xclang -verify -fopenacc -emit-ast %t-acc.c -DACCC=%'accc' \
// RUN:          -o %t.ast
// RUN:   %for prt-args {
// RUN:     %clang %[prt] %t.ast 2>&1 \
// RUN:     | FileCheck -check-prefixes=%[prt-chk] -DACCC=' '%[ACCC] \
// RUN:                 -DOMPDP=%'ompdp' %s
// RUN:   }
// RUN: }

// Can we print the OpenMP source code, compile, and run it successfully?
//
// RUN: %for loop-clauses {
// RUN:   %for prt-opts {
// RUN:     %clang -Xclang -verify %[prt-opt]=omp -DACCC=%'accc' %s > %t-omp.c
// RUN:     echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN:     %clang -Xclang -verify -fopenmp -o %t %t-omp.c
// RUN:     %t | FileCheck -check-prefixes=%[exe] %s
// RUN:   }
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

// PRT: int tentativeDef;
int tentativeDef;

// PRT-NEXT: int main() {
int main() {

  //--------------------------------------------------
  // Check private for scalar that is local to enclosing "acc parallel".
  //--------------------------------------------------

  // PRT-NEXT: printf
  // EXE: parallel-local loop-private scalar
  printf("parallel-local loop-private scalar\n");
  // PRT-A-NEXT:  #pragma acc parallel
  // PRT-AO-NEXT: // #pragma omp target teams
  // PRT-O-NEXT:  #pragma omp target teams
  // PRT-OA-NEXT: // #pragma acc parallel
  #pragma acc parallel num_gangs(2)
  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    int i = 99;
    // DMP:           ACCLoopDirective
    // DMP-ASEQ-NEXT:   ACCSeqClause
    // DMP-ASEQ-NOT:      <implicit>
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
    // PRT-A-NEXT:       {{^ *}}#pragma acc loop[[ACCC]] private(i){{$}}
    // PRT-AO-OPRG-NEXT: {{^ *}}// #pragma omp [[OMPDP]] private(i){{$}}
    // PRT-AO-OPLC-NEXT: {{^ *}}// #pragma omp [[OMPDP]] private(i){{$}}
    // PRT-A-NEXT:       for (int j = 0; j < 2; ++j) {
    // PRT-A-NEXT:         i = j;
    // PRT-A-NEXT:         printf
    // PRT-A-NEXT:       }
    // PRT-AO-OSEQ-NEXT: // {
    // PRT-AO-OSEQ-NEXT: //   int i;
    // PRT-AO-OSEQ-NEXT: //   for (int j = 0; j < 2; ++j) {
    // PRT-AO-OSEQ-NEXT: //     i = j;
    // PRT-AO-OSEQ-NEXT: //     printf
    // PRT-AO-OSEQ-NEXT: //   }
    // PRT-AO-OSEQ-NEXT: // }
    //
    // PRT-O-OPRG-NEXT:  {{^ *}}#pragma omp [[OMPDP]] private(i){{$}}
    // PRT-O-OPLC-NEXT:  {{^ *}}#pragma omp [[OMPDP]] private(i){{$}}
    // PRT-OA-OPRG-NEXT: {{^ *}}// #pragma acc loop[[ACCC]] private(i){{$}}
    // PRT-OA-OPLC-NEXT: {{^ *}}// #pragma acc loop[[ACCC]] private(i){{$}}
    // PRT-O-OSEQ-NEXT:  {
    // PRT-O-OSEQ-NEXT:    int i;
    // PRT-O-NEXT:         for (int j = 0; j < 2; ++j) {
    // PRT-O-NEXT:           i = j;
    // PRT-O-NEXT:           printf
    // PRT-O-NEXT:         }
    // PRT-O-OSEQ-NEXT:  }
    // PRT-OA-OSEQ-NEXT: // #pragma acc loop[[ACCC]] private(i){{$}}
    // PRT-OA-OSEQ-NEXT: // for (int j = 0; j < 2; ++j) {
    // PRT-OA-OSEQ-NEXT: //   i = j;
    // PRT-OA-OSEQ-NEXT: //   printf
    // PRT-OA-OSEQ-NEXT: // }
    //
    // PRT-NOACC-NEXT:   for (int j = 0; j < 2; ++j) {
    // PRT-NOACC-NEXT:     i = j;
    // PRT-NOACC-NEXT:     printf
    // PRT-NOACC-NEXT:   }
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
  } // PRT-NEXT: }

  // Repeat that but for "acc parallel loop", so scalar is firstprivate instead
  // of local for effective enclosing "acc parallel"

  // PRT-NEXT: printf
  // EXE-NEXT: parallel-firstprivate loop-private scalar
  printf("parallel-firstprivate loop-private scalar\n");
  // PRT-NEXT: {
  {
    // PRT-NEXT: int i = 99;
    int i = 99;
    // DMP:           ACCParallelLoopDirective
    // DMP-NEXT:        ACCNum_gangsClause
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 2
    // DMP-ASEQ-NEXT:   ACCSeqClause
    // DMP-ASEQ-NOT:      <implicit>
    // DMP-AG-NEXT:     ACCGangClause
    // DMP-AW-NEXT:     ACCWorkerClause
    // DMP-AV-NEXT:     ACCVectorClause
    // DMP-NEXT:        ACCPrivateClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'i' 'int'
    // DMP:             effect: ACCParallelDirective
    // DMP-NEXT:          ACCNum_gangsClause
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:          ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:            DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:          impl: OMPTargetTeamsDirective
    // DMP-NEXT:            OMPNum_teamsClause
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:            OMPFirstprivateClause
    // DMP-NOT:               <implicit>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'i' 'int'
    // DMP:               ACCLoopDirective
    // DMP-ASEQ-NEXT:       ACCSeqClause
    // DMP-ASEQ-NOT:          <implicit>
    // DMP-AG-NEXT:         ACCGangClause
    // DMP-AW-NEXT:         ACCWorkerClause
    // DMP-AV-NEXT:         ACCVectorClause
    // DMP-NEXT:            ACCPrivateClause
    // DMP-NEXT:              DeclRefExpr {{.*}} 'i' 'int'
    // DMP-AIMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-OPRG-NEXT:       impl: [[OMPDD]]
    // DMP-OPRG-NEXT:         OMPPrivateClause
    // DMP-OPRG-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
    // DMP-OPRG:              ForStmt
    // DMP-OPLC-NEXT:       impl: [[OMPDD]]
    // DMP-ONT1-NEXT:         OMPNum_threadsClause
    // DMP-ONT1-NEXT:           IntegerLiteral {{.*}} 'int' 1
    // DMP-OPLC-NEXT:         OMPPrivateClause
    // DMP-OPLC-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
    // DMP-OPLC:              ForStmt
    // DMP-OSEQ-NEXT:       impl: CompoundStmt
    // DMP-OSEQ-NEXT:         DeclStmt
    // DMP-OSEQ-NEXT:           VarDecl {{.*}} i 'int'
    // DMP-OSEQ-NEXT:         ForStmt
    //
    // PRT-A-NEXT:       {{^ *}}#pragma acc parallel loop num_gangs(2)[[ACCC]] private(i){{$}}
    // PRT-AO-OPRG-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) firstprivate(i){{$}}
    // PRT-AO-OPLC-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) firstprivate(i){{$}}
    // PRT-AO-OPRG-NEXT: {{^ *}}// #pragma omp [[OMPDP]] private(i){{$}}
    // PRT-AO-OPLC-NEXT: {{^ *}}// #pragma omp [[OMPDP]] private(i){{$}}
    // PRT-A-NEXT:       for (int j = 0; j < 2; ++j) {
    // PRT-A-NEXT:         i = j;
    // PRT-A-NEXT:         printf
    // PRT-A-NEXT:       }
    // PRT-AO-OSEQ-NEXT: // #pragma omp target teams num_teams(2) firstprivate(i){{$}}
    // PRT-AO-OSEQ-NEXT: // {
    // PRT-AO-OSEQ-NEXT: //   int i;
    // PRT-AO-OSEQ-NEXT: //   for (int j = 0; j < 2; ++j) {
    // PRT-AO-OSEQ-NEXT: //     i = j;
    // PRT-AO-OSEQ-NEXT: //     printf
    // PRT-AO-OSEQ-NEXT: //   }
    // PRT-AO-OSEQ-NEXT: // }
    //
    // PRT-O-NEXT:       {{^ *}}#pragma omp target teams num_teams(2) firstprivate(i){{$}}
    // PRT-O-OPRG-NEXT:  {{^ *}}#pragma omp [[OMPDP]] private(i){{$}}
    // PRT-O-OPLC-NEXT:  {{^ *}}#pragma omp [[OMPDP]] private(i){{$}}
    // PRT-OA-OPRG-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2)[[ACCC]] private(i){{$}}
    // PRT-OA-OPLC-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2)[[ACCC]] private(i){{$}}
    // PRT-O-OSEQ-NEXT:  {
    // PRT-O-OSEQ-NEXT:    int i;
    // PRT-O-NEXT:         for (int j = 0; j < 2; ++j) {
    // PRT-O-NEXT:           i = j;
    // PRT-O-NEXT:           printf
    // PRT-O-NEXT:         }
    // PRT-O-OSEQ-NEXT:  }
    // PRT-OA-OSEQ-NEXT: // #pragma acc parallel loop num_gangs(2)[[ACCC]] private(i){{$}}
    // PRT-OA-OSEQ-NEXT: // for (int j = 0; j < 2; ++j) {
    // PRT-OA-OSEQ-NEXT: //   i = j;
    // PRT-OA-OSEQ-NEXT: //   printf
    // PRT-OA-OSEQ-NEXT: // }
    //
    // PRT-NOACC-NEXT:   for (int j = 0; j < 2; ++j) {
    // PRT-NOACC-NEXT:     i = j;
    // PRT-NOACC-NEXT:     printf
    // PRT-NOACC-NEXT:   }
    #pragma acc parallel loop num_gangs(2) ACCC private(i)
    for (int j = 0; j < 2; ++j) {
      i = j;
      // EXE-DAG:        in loop: 0
      // EXE-DAG:        in loop: 1
      // EXE-GREDUN-DAG: in loop: 0
      // EXE-GREDUN-DAG: in loop: 1
      printf("in loop: %d\n", i);
    }
    // PRT-NEXT: printf
    // EXE-NEXT: after loop: 99
    printf("after loop: %d\n", i);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Check private for scalar that, so far, has only a tentative definition.
  //
  // Inserting a local definition to make that scalar variable private for a
  // sequential loop used to fail an assert because VarDecl::getDefinition
  // returned nullptr for the tentative definition.
  //--------------------------------------------------

  // PRT-NEXT: printf
  // EXE-NEXT: tentatively defined loop-private scalar
  printf("tentatively defined loop-private scalar\n");
  // PRT-A-NEXT:  #pragma acc parallel
  // PRT-AO-NEXT: // #pragma omp target teams
  // PRT-O-NEXT:  #pragma omp target teams
  // PRT-OA-NEXT: // #pragma acc parallel
  #pragma acc parallel num_gangs(2)
  // PRT-NEXT: {
  {
    // DMP:           ACCLoopDirective
    // DMP-ASEQ-NEXT:   ACCSeqClause
    // DMP-ASEQ-NOT:      <implicit>
    // DMP-AG-NEXT:     ACCGangClause
    // DMP-AW-NEXT:     ACCWorkerClause
    // DMP-AV-NEXT:     ACCVectorClause
    // DMP-NEXT:        ACCPrivateClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'tentativeDef' 'int'
    // DMP-AIMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
    // DMP-OPRG-NEXT:   impl: [[OMPDD]]
    // DMP-OPRG-NEXT:     OMPPrivateClause
    // DMP-OPRG-NEXT:       DeclRefExpr {{.*}} 'tentativeDef' 'int'
    // DMP-OPRG:          ForStmt
    // DMP-OPLC-NEXT:   impl: [[OMPDD]]
    // DMP-ONT1-NEXT:     OMPNum_threadsClause
    // DMP-ONT1-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-OPLC-NEXT:     OMPPrivateClause
    // DMP-OPLC-NEXT:       DeclRefExpr {{.*}} 'tentativeDef' 'int'
    // DMP-OPLC:          ForStmt
    // DMP-OSEQ-NEXT:   impl: CompoundStmt
    // DMP-OSEQ-NEXT:     DeclStmt
    // DMP-OSEQ-NEXT:       VarDecl {{.*}} tentativeDef 'int'
    // DMP-OSEQ-NEXT:     ForStmt
    //
    // PRT-A-NEXT:       {{^ *}}#pragma acc loop[[ACCC]] private(tentativeDef){{$}}
    // PRT-AO-OPRG-NEXT: {{^ *}}// #pragma omp [[OMPDP]] private(tentativeDef){{$}}
    // PRT-AO-OPLC-NEXT: {{^ *}}// #pragma omp [[OMPDP]] private(tentativeDef){{$}}
    // PRT-A-NEXT:       for (int j = 0; j < 2; ++j) {
    // PRT-A-NEXT:         tentativeDef = j;
    // PRT-A-NEXT:         printf
    // PRT-A-NEXT:       }
    // PRT-AO-OSEQ-NEXT: // {
    // PRT-AO-OSEQ-NEXT: //   int tentativeDef;
    // PRT-AO-OSEQ-NEXT: //   for (int j = 0; j < 2; ++j) {
    // PRT-AO-OSEQ-NEXT: //     tentativeDef = j;
    // PRT-AO-OSEQ-NEXT: //     printf
    // PRT-AO-OSEQ-NEXT: //   }
    // PRT-AO-OSEQ-NEXT: // }
    //
    // PRT-O-OPRG-NEXT:  {{^ *}}#pragma omp [[OMPDP]] private(tentativeDef){{$}}
    // PRT-O-OPLC-NEXT:  {{^ *}}#pragma omp [[OMPDP]] private(tentativeDef){{$}}
    // PRT-OA-OPRG-NEXT: {{^ *}}// #pragma acc loop[[ACCC]] private(tentativeDef){{$}}
    // PRT-OA-OPLC-NEXT: {{^ *}}// #pragma acc loop[[ACCC]] private(tentativeDef){{$}}
    // PRT-O-OSEQ-NEXT:  {
    // PRT-O-OSEQ-NEXT:    int tentativeDef;
    // PRT-O-NEXT:         for (int j = 0; j < 2; ++j) {
    // PRT-O-NEXT:           tentativeDef = j;
    // PRT-O-NEXT:           printf
    // PRT-O-NEXT:         }
    // PRT-O-OSEQ-NEXT:  }
    // PRT-OA-OSEQ-NEXT: // #pragma acc loop[[ACCC]] private(tentativeDef){{$}}
    // PRT-OA-OSEQ-NEXT: // for (int j = 0; j < 2; ++j) {
    // PRT-OA-OSEQ-NEXT: //   tentativeDef = j;
    // PRT-OA-OSEQ-NEXT: //   printf
    // PRT-OA-OSEQ-NEXT: // }
    //
    // PRT-NOACC-NEXT:   for (int j = 0; j < 2; ++j) {
    // PRT-NOACC-NEXT:     tentativeDef = j;
    // PRT-NOACC-NEXT:     printf
    // PRT-NOACC-NEXT:   }
    #pragma acc loop ACCC private(tentativeDef)
    for (int j = 0; j < 2; ++j) {
      tentativeDef = j;
      // EXE-DAG:        in loop: 0
      // EXE-DAG:        in loop: 1
      // EXE-GREDUN-DAG: in loop: 0
      // EXE-GREDUN-DAG: in loop: 1
      printf("in loop: %d\n", tentativeDef);
    }
    // PRT-NEXT:      printf
    // EXE-DAG: after loop: 99
    // EXE-DAG: after loop: 99
    printf("after loop: %d\n", tentativeDef);
  } // PRT-NEXT: }

  // Repeat that but for "acc parallel loop".

  // PRT-NEXT: printf
  // EXE-NEXT: tentatively defined loop-private scalar (combined directive)
  printf("tentatively defined loop-private scalar (combined directive)\n");
  // PRT-NEXT: {
  {
    // DMP:           ACCParallelLoopDirective
    // DMP-NEXT:        ACCNum_gangsClause
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 2
    // DMP-ASEQ-NEXT:   ACCSeqClause
    // DMP-ASEQ-NOT:      <implicit>
    // DMP-AG-NEXT:     ACCGangClause
    // DMP-AW-NEXT:     ACCWorkerClause
    // DMP-AV-NEXT:     ACCVectorClause
    // DMP-NEXT:        ACCPrivateClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'tentativeDef' 'int'
    // DMP:             effect: ACCParallelDirective
    // DMP-NEXT:          ACCNum_gangsClause
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:          ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:            DeclRefExpr {{.*}} 'tentativeDef' 'int'
    // DMP-NEXT:          impl: OMPTargetTeamsDirective
    // DMP-NEXT:            OMPNum_teamsClause
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:            OMPFirstprivateClause
    // DMP-NOT:               <implicit>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'tentativeDef' 'int'
    // DMP:               ACCLoopDirective
    // DMP-ASEQ-NEXT:       ACCSeqClause
    // DMP-ASEQ-NOT:          <implicit>
    // DMP-AG-NEXT:         ACCGangClause
    // DMP-AW-NEXT:         ACCWorkerClause
    // DMP-AV-NEXT:         ACCVectorClause
    // DMP-NEXT:            ACCPrivateClause
    // DMP-NEXT:              DeclRefExpr {{.*}} 'tentativeDef' 'int'
    // DMP-AIMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-OPRG-NEXT:       impl: [[OMPDD]]
    // DMP-OPRG-NEXT:         OMPPrivateClause
    // DMP-OPRG-NEXT:           DeclRefExpr {{.*}} 'tentativeDef' 'int'
    // DMP-OPRG:              ForStmt
    // DMP-OPLC-NEXT:       impl: [[OMPDD]]
    // DMP-ONT1-NEXT:         OMPNum_threadsClause
    // DMP-ONT1-NEXT:           IntegerLiteral {{.*}} 'int' 1
    // DMP-OPLC-NEXT:         OMPPrivateClause
    // DMP-OPLC-NEXT:           DeclRefExpr {{.*}} 'tentativeDef' 'int'
    // DMP-OPLC:              ForStmt
    // DMP-OSEQ-NEXT:       impl: CompoundStmt
    // DMP-OSEQ-NEXT:         DeclStmt
    // DMP-OSEQ-NEXT:           VarDecl {{.*}} tentativeDef 'int'
    // DMP-OSEQ-NEXT:         ForStmt
    //
    // PRT-A-NEXT:       {{^ *}}#pragma acc parallel loop num_gangs(2)[[ACCC]] private(tentativeDef){{$}}
    // PRT-AO-OPRG-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) firstprivate(tentativeDef){{$}}
    // PRT-AO-OPLC-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) firstprivate(tentativeDef){{$}}
    // PRT-AO-OPRG-NEXT: {{^ *}}// #pragma omp [[OMPDP]] private(tentativeDef){{$}}
    // PRT-AO-OPLC-NEXT: {{^ *}}// #pragma omp [[OMPDP]] private(tentativeDef){{$}}
    // PRT-A-NEXT:       for (int j = 0; j < 2; ++j) {
    // PRT-A-NEXT:         tentativeDef = j;
    // PRT-A-NEXT:         printf
    // PRT-A-NEXT:       }
    // PRT-AO-OSEQ-NEXT: // #pragma omp target teams num_teams(2) firstprivate(tentativeDef){{$}}
    // PRT-AO-OSEQ-NEXT: // {
    // PRT-AO-OSEQ-NEXT: //   int tentativeDef;
    // PRT-AO-OSEQ-NEXT: //   for (int j = 0; j < 2; ++j) {
    // PRT-AO-OSEQ-NEXT: //     tentativeDef = j;
    // PRT-AO-OSEQ-NEXT: //     printf
    // PRT-AO-OSEQ-NEXT: //   }
    // PRT-AO-OSEQ-NEXT: // }
    //
    // PRT-O-NEXT:       {{^ *}}#pragma omp target teams num_teams(2) firstprivate(tentativeDef){{$}}
    // PRT-O-OPRG-NEXT:  {{^ *}}#pragma omp [[OMPDP]] private(tentativeDef){{$}}
    // PRT-O-OPLC-NEXT:  {{^ *}}#pragma omp [[OMPDP]] private(tentativeDef){{$}}
    // PRT-OA-OPRG-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2)[[ACCC]] private(tentativeDef){{$}}
    // PRT-OA-OPLC-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2)[[ACCC]] private(tentativeDef){{$}}
    // PRT-O-OSEQ-NEXT:  {
    // PRT-O-OSEQ-NEXT:    int tentativeDef;
    // PRT-O-NEXT:         for (int j = 0; j < 2; ++j) {
    // PRT-O-NEXT:           tentativeDef = j;
    // PRT-O-NEXT:           printf
    // PRT-O-NEXT:         }
    // PRT-O-OSEQ-NEXT:  }
    // PRT-OA-OSEQ-NEXT: // #pragma acc parallel loop num_gangs(2)[[ACCC]] private(tentativeDef){{$}}
    // PRT-OA-OSEQ-NEXT: // for (int j = 0; j < 2; ++j) {
    // PRT-OA-OSEQ-NEXT: //   tentativeDef = j;
    // PRT-OA-OSEQ-NEXT: //   printf
    // PRT-OA-OSEQ-NEXT: // }
    //
    // PRT-NOACC-NEXT:   for (int j = 0; j < 2; ++j) {
    // PRT-NOACC-NEXT:     tentativeDef = j;
    // PRT-NOACC-NEXT:     printf
    // PRT-NOACC-NEXT:   }
    #pragma acc parallel loop num_gangs(2) ACCC private(tentativeDef)
    for (int j = 0; j < 2; ++j) {
      tentativeDef = j;
      // EXE-DAG:        in loop: 0
      // EXE-DAG:        in loop: 1
      // EXE-GREDUN-DAG: in loop: 0
      // EXE-GREDUN-DAG: in loop: 1
      printf("in loop: %d\n", tentativeDef);
    }
    // PRT-NEXT: printf
    // EXE-NEXT: after loop: 99
    printf("after loop: %d\n", tentativeDef);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Check private for loop control variable that is declared not assigned in
  // init of attached for loop.
  //--------------------------------------------------

  // PRT-NEXT: printf
  // EXE-NEXT: parallel-local loop-private declared loop control
  printf("parallel-local loop-private declared loop control\n");
  // PRT-A-NEXT:  #pragma acc parallel
  // PRT-AO-NEXT: // #pragma omp target teams
  // PRT-O-NEXT:  #pragma omp target teams
  // PRT-OA-NEXT: // #pragma acc parallel
  #pragma acc parallel num_gangs(2)
  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    int i = 99;
    // DMP:           ACCLoopDirective
    // DMP-ASEQ-NEXT:   ACCSeqClause
    // DMP-ASEQ-NOT:      <implicit>
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
    // PRT-A-NEXT:       {{^ *}}#pragma acc loop[[ACCC]] private(i){{$}}
    // PRT-AO-OPRG-NEXT: {{^ *}}// #pragma omp [[OMPDP]] private(i){{$}}
    // PRT-AO-OPLC-NEXT: {{^ *}}// #pragma omp [[OMPDP]] private(i){{$}}
    // PRT-A-NEXT:       for (int i = 0; i < 2; ++i) {
    // PRT-A-NEXT:         printf
    // PRT-A-NEXT:       }
    // PRT-AO-OSEQ-NEXT: // {
    // PRT-AO-OSEQ-NEXT: //   int i;
    // PRT-AO-OSEQ-NEXT: //   for (int i = 0; i < 2; ++i) {
    // PRT-AO-OSEQ-NEXT: //     printf
    // PRT-AO-OSEQ-NEXT: //   }
    // PRT-AO-OSEQ-NEXT: // }
    //
    // PRT-O-OPRG-NEXT:  {{^ *}}#pragma omp [[OMPDP]] private(i){{$}}
    // PRT-O-OPLC-NEXT:  {{^ *}}#pragma omp [[OMPDP]] private(i){{$}}
    // PRT-OA-OPRG-NEXT: {{^ *}}// #pragma acc loop[[ACCC]] private(i){{$}}
    // PRT-OA-OPLC-NEXT: {{^ *}}// #pragma acc loop[[ACCC]] private(i){{$}}
    // PRT-O-OSEQ-NEXT:  {
    // PRT-O-OSEQ-NEXT:    int i;
    // PRT-O-NEXT:         for (int i = 0; i < 2; ++i) {
    // PRT-O-NEXT:           printf
    // PRT-O-NEXT:         }
    // PRT-O-OSEQ-NEXT:  }
    // PRT-OA-OSEQ-NEXT: // #pragma acc loop[[ACCC]] private(i){{$}}
    // PRT-OA-OSEQ-NEXT: // for (int i = 0; i < 2; ++i) {
    // PRT-OA-OSEQ-NEXT: //   printf
    // PRT-OA-OSEQ-NEXT: // }
    //
    // PRT-NOACC-NEXT:   for (int i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:     printf
    // PRT-NOACC-NEXT:   }
    #pragma acc loop ACCC private(i)
    for (int i = 0; i < 2; ++i) {
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
  } // PRT-NEXT: }

  // Repeat that but with "acc parallel loop".

  // PRT-NEXT: printf
  // EXE-NEXT: parallel-firstprivate loop-private declared loop control
  printf("parallel-firstprivate loop-private declared loop control\n");
  // PRT-NEXT: {
  {
    // PRT-NEXT: int i = 99;
    int i = 99;
    // DMP:           ACCParallelLoopDirective
    // DMP-NEXT:        ACCNum_gangsClause
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 2
    // DMP-ASEQ-NEXT:   ACCSeqClause
    // DMP-ASEQ-NOT:      <implicit>
    // DMP-AG-NEXT:     ACCGangClause
    // DMP-AW-NEXT:     ACCWorkerClause
    // DMP-AV-NEXT:     ACCVectorClause
    // DMP-NEXT:        ACCPrivateClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'i' 'int'
    // DMP:             effect: ACCParallelDirective
    // DMP-NEXT:          ACCNum_gangsClause
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:          impl: OMPTargetTeamsDirective
    // DMP-NEXT:            OMPNum_teamsClause
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 2
    // DMP:               ACCLoopDirective
    // DMP-ASEQ-NEXT:       ACCSeqClause
    // DMP-ASEQ-NOT:          <implicit>
    // DMP-AG-NEXT:         ACCGangClause
    // DMP-AW-NEXT:         ACCWorkerClause
    // DMP-AV-NEXT:         ACCVectorClause
    // DMP-NEXT:            ACCPrivateClause
    // DMP-NEXT:              DeclRefExpr {{.*}} 'i' 'int'
    // DMP-AIMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-OPRG-NEXT:       impl: [[OMPDD]]
    // DMP-OPRG-NEXT:         OMPPrivateClause
    // DMP-OPRG-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
    // DMP-OPRG:              ForStmt
    // DMP-OPLC-NEXT:       impl: [[OMPDD]]
    // DMP-ONT1-NEXT:         OMPNum_threadsClause
    // DMP-ONT1-NEXT:           IntegerLiteral {{.*}} 'int' 1
    // DMP-OPLC-NEXT:         OMPPrivateClause
    // DMP-OPLC-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
    // DMP-OPLC:              ForStmt
    // DMP-OSEQ-NEXT:       impl: CompoundStmt
    // DMP-OSEQ-NEXT:         DeclStmt
    // DMP-OSEQ-NEXT:           VarDecl {{.*}} i 'int'
    // DMP-OSEQ-NEXT:         ForStmt
    //
    // PRT-A-NEXT:       {{^ *}}#pragma acc parallel loop num_gangs(2)[[ACCC]] private(i){{$}}
    // PRT-AO-OPRG-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-OPLC-NEXT: {{^ *}}// #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-OPRG-NEXT: {{^ *}}// #pragma omp [[OMPDP]] private(i){{$}}
    // PRT-AO-OPLC-NEXT: {{^ *}}// #pragma omp [[OMPDP]] private(i){{$}}
    // PRT-A-NEXT:       for (int i = 0; i < 2; ++i) {
    // PRT-A-NEXT:         printf
    // PRT-A-NEXT:       }
    // PRT-AO-OSEQ-NEXT: // #pragma omp target teams num_teams(2){{$}}
    // PRT-AO-OSEQ-NEXT: // {
    // PRT-AO-OSEQ-NEXT: //   int i;
    // PRT-AO-OSEQ-NEXT: //   for (int i = 0; i < 2; ++i) {
    // PRT-AO-OSEQ-NEXT: //     printf
    // PRT-AO-OSEQ-NEXT: //   }
    // PRT-AO-OSEQ-NEXT: // }
    //
    // PRT-O-NEXT:       {{^ *}}#pragma omp target teams num_teams(2){{$}}
    // PRT-O-OPRG-NEXT:  {{^ *}}#pragma omp [[OMPDP]] private(i){{$}}
    // PRT-O-OPLC-NEXT:  {{^ *}}#pragma omp [[OMPDP]] private(i){{$}}
    // PRT-OA-OPRG-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2)[[ACCC]] private(i){{$}}
    // PRT-OA-OPLC-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2)[[ACCC]] private(i){{$}}
    // PRT-O-OSEQ-NEXT:  {
    // PRT-O-OSEQ-NEXT:    int i;
    // PRT-O-NEXT:         for (int i = 0; i < 2; ++i) {
    // PRT-O-NEXT:           printf
    // PRT-O-NEXT:         }
    // PRT-O-OSEQ-NEXT:  }
    // PRT-OA-OSEQ-NEXT: // #pragma acc parallel loop num_gangs(2)[[ACCC]] private(i){{$}}
    // PRT-OA-OSEQ-NEXT: // for (int i = 0; i < 2; ++i) {
    // PRT-OA-OSEQ-NEXT: //   printf
    // PRT-OA-OSEQ-NEXT: // }
    //
    // PRT-NOACC-NEXT:   for (int i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:     printf
    // PRT-NOACC-NEXT:   }
    #pragma acc parallel loop num_gangs(2) ACCC private(i)
    for (int i = 0; i < 2; ++i) {
      // EXE-DAG:        in loop: 0
      // EXE-DAG:        in loop: 1
      // EXE-GREDUN-DAG: in loop: 0
      // EXE-GREDUN-DAG: in loop: 1
      printf("in loop: %d\n", i);
    }
    // PRT-NEXT: printf
    // EXE-NEXT: after loop: 99
    printf("after loop: %d\n", i);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Check private for loop control variable that is assigned not declared in
  // init of attached for loop.
  //--------------------------------------------------

  // PRT-NEXT: printf
  // EXE-NEXT: parallel-local loop-private assigned loop control
  printf("parallel-local loop-private assigned loop control\n");
  // PRT-A-NEXT:  #pragma acc parallel
  // PRT-AO-NEXT: // #pragma omp target teams
  // PRT-O-NEXT:  #pragma omp target teams
  // PRT-OA-NEXT: // #pragma acc parallel
  #pragma acc parallel num_gangs(2)
  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    int i = 99;
    // DMP:           ACCLoopDirective
    // DMP-ASEQ-NEXT:   ACCSeqClause
    // DMP-ASEQ-NOT:      <implicit>
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
    // PRT-A-NEXT:       {{^ *}}#pragma acc loop[[ACCC]] private(i){{$}}
    // PRT-AO-OPRG-NEXT: {{^ *}}// #pragma omp [[OMPDP]] private(i){{$}}
    // PRT-A-NEXT:       for (i = 0; i < 2; ++i) {
    // PRT-A-NEXT:         printf
    // PRT-A-NEXT:       }
    // PRT-AO-OPLC-NEXT: // {
    // PRT-AO-OPLC-NEXT: //   int i;
    // PRT-AO-OPLC-NEXT: //   #pragma omp [[OMPDP]]{{$}}
    // PRT-AO-OPLC-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-OPLC-NEXT: //     printf
    // PRT-AO-OPLC-NEXT: //   }
    // PRT-AO-OPLC-NEXT: // }
    // PRT-AO-OSEQ-NEXT: // {
    // PRT-AO-OSEQ-NEXT: //   int i;
    // PRT-AO-OSEQ-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-OSEQ-NEXT: //     printf
    // PRT-AO-OSEQ-NEXT: //   }
    // PRT-AO-OSEQ-NEXT: // }
    //
    // PRT-O-OPRG-NEXT:  {{^ *}}#pragma omp [[OMPDP]] private(i){{$}}
    // PRT-OA-OPRG-NEXT: {{^ *}}// #pragma acc loop[[ACCC]] private(i){{$}}
    // PRT-O-OPRG-NEXT:  for (i = 0; i < 2; ++i) {
    // PRT-O-OPRG-NEXT:    printf
    // PRT-O-OPRG-NEXT:  }
    // PRT-O-OPLC-NEXT:  {
    // PRT-O-OPLC-NEXT:    int i;
    // PRT-O-OPLC-NEXT:    {{^ *}}#pragma omp [[OMPDP]]{{$}}
    // PRT-O-OPLC-NEXT:    for (i = 0; i < 2; ++i) {
    // PRT-O-OPLC-NEXT:      printf
    // PRT-O-OPLC-NEXT:    }
    // PRT-O-OPLC-NEXT:  }
    // PRT-O-OSEQ-NEXT:  {
    // PRT-O-OSEQ-NEXT:    int i;
    // PRT-O-OSEQ-NEXT:    for (i = 0; i < 2; ++i) {
    // PRT-O-OSEQ-NEXT:      printf
    // PRT-O-OSEQ-NEXT:    }
    // PRT-O-OSEQ-NEXT:  }
    // PRT-OA-OPLC-NEXT: // #pragma acc loop[[ACCC]] private(i){{$}}
    // PRT-OA-OPLC-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-OPLC-NEXT: //   printf
    // PRT-OA-OPLC-NEXT: // }
    // PRT-OA-OSEQ-NEXT: // #pragma acc loop[[ACCC]] private(i){{$}}
    // PRT-OA-OSEQ-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-OSEQ-NEXT: //   printf
    // PRT-OA-OSEQ-NEXT: // }
    //
    // PRT-NOACC-NEXT:   for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:     printf
    // PRT-NOACC-NEXT:   }
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
  } // PRT-NEXT: }

  // Repeat that but with "acc parallel loop".

  // PRT-NEXT: printf
  // EXE-NEXT: parallel-firstprivate loop-private assigned loop control
  printf("parallel-firstprivate loop-private assigned loop control\n");
  // PRT-NEXT: {
  {
    // PRT-NEXT: int i = 99;
    int i = 99;
    // DMP:           ACCParallelLoopDirective
    // DMP-NEXT:        ACCNum_gangsClause
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 2
    // DMP-ASEQ-NEXT:   ACCSeqClause
    // DMP-ASEQ-NOT:      <implicit>
    // DMP-AG-NEXT:     ACCGangClause
    // DMP-AW-NEXT:     ACCWorkerClause
    // DMP-AV-NEXT:     ACCVectorClause
    // DMP-NEXT:        ACCPrivateClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:        effect: ACCParallelDirective
    // DMP-NEXT:          ACCNum_gangsClause
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:          ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:            DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:          impl: OMPTargetTeamsDirective
    // DMP-NEXT:            OMPNum_teamsClause
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:            OMPFirstprivateClause
    // DMP-NOT:               <implicit>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'i' 'int'
    // DMP:               ACCLoopDirective
    // DMP-ASEQ-NEXT:       ACCSeqClause
    // DMP-ASEQ-NOT:          <implicit>
    // DMP-AG-NEXT:         ACCGangClause
    // DMP-AW-NEXT:         ACCWorkerClause
    // DMP-AV-NEXT:         ACCVectorClause
    // DMP-NEXT:            ACCPrivateClause
    // DMP-NEXT:              DeclRefExpr {{.*}} 'i' 'int'
    // DMP-AIMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
    // DMP-OPRG-NEXT:       impl: [[OMPDD]]
    // DMP-OPRG-NEXT:         OMPPrivateClause
    // DMP-OPRG-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
    // DMP-OPRG:              ForStmt
    // DMP-OPLC-NEXT:       impl: CompoundStmt
    // DMP-OPLC-NEXT:         DeclStmt
    // DMP-OPLC-NEXT:           VarDecl {{.*}} i 'int'
    // DMP-OPLC-NEXT:         [[OMPDD]]
    // DMP-ONT1-NEXT:           OMPNum_threadsClause
    // DMP-ONT1-NEXT:             IntegerLiteral {{.*}} 'int' 1
    // DMP-OPLC:                ForStmt
    // DMP-OSEQ-NEXT:       impl: CompoundStmt
    // DMP-OSEQ-NEXT:         DeclStmt
    // DMP-OSEQ-NEXT:           VarDecl {{.*}} i 'int'
    // DMP-OSEQ-NEXT:         ForStmt
    //
    // PRT-A-NEXT:       {{^ *}}#pragma acc parallel loop num_gangs(2)[[ACCC]] private(i){{$}}
    // PRT-AO-OPRG-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) firstprivate(i){{$}}
    // PRT-AO-OPRG-NEXT: {{^ *}}// #pragma omp [[OMPDP]] private(i){{$}}
    // PRT-A-NEXT:       for (i = 0; i < 2; ++i) {
    // PRT-A-NEXT:         printf
    // PRT-A-NEXT:       }
    // PRT-AO-OPLC-NEXT: // #pragma omp target teams num_teams(2) firstprivate(i){{$}}
    // PRT-AO-OPLC-NEXT: // {
    // PRT-AO-OPLC-NEXT: //   int i;
    // PRT-AO-OPLC-NEXT: //   #pragma omp [[OMPDP]]{{$}}
    // PRT-AO-OPLC-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-OPLC-NEXT: //     printf
    // PRT-AO-OPLC-NEXT: //   }
    // PRT-AO-OPLC-NEXT: // }
    // PRT-AO-OSEQ-NEXT: // #pragma omp target teams num_teams(2) firstprivate(i){{$}}
    // PRT-AO-OSEQ-NEXT: // {
    // PRT-AO-OSEQ-NEXT: //   int i;
    // PRT-AO-OSEQ-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-OSEQ-NEXT: //     printf
    // PRT-AO-OSEQ-NEXT: //   }
    // PRT-AO-OSEQ-NEXT: // }
    //
    // PRT-O-NEXT:       {{^ *}}#pragma omp target teams num_teams(2) firstprivate(i){{$}}
    // PRT-O-OPRG-NEXT:  {{^ *}}#pragma omp [[OMPDP]] private(i){{$}}
    // PRT-OA-OPRG-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2)[[ACCC]] private(i){{$}}
    // PRT-O-OPRG-NEXT:  for (i = 0; i < 2; ++i) {
    // PRT-O-OPRG-NEXT:    printf
    // PRT-O-OPRG-NEXT:  }
    // PRT-O-OPLC-NEXT:  {
    // PRT-O-OPLC-NEXT:    int i;
    // PRT-O-OPLC-NEXT:    {{^ *}}#pragma omp [[OMPDP]]{{$}}
    // PRT-O-OPLC-NEXT:    for (i = 0; i < 2; ++i) {
    // PRT-O-OPLC-NEXT:      printf
    // PRT-O-OPLC-NEXT:    }
    // PRT-O-OPLC-NEXT:  }
    // PRT-O-OSEQ-NEXT:  {
    // PRT-O-OSEQ-NEXT:    int i;
    // PRT-O-OSEQ-NEXT:    for (i = 0; i < 2; ++i) {
    // PRT-O-OSEQ-NEXT:      printf
    // PRT-O-OSEQ-NEXT:    }
    // PRT-O-OSEQ-NEXT:  }
    // PRT-OA-OPLC-NEXT: // #pragma acc parallel loop num_gangs(2)[[ACCC]] private(i){{$}}
    // PRT-OA-OPLC-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-OPLC-NEXT: //   printf
    // PRT-OA-OPLC-NEXT: // }
    // PRT-OA-OSEQ-NEXT: // #pragma acc parallel loop num_gangs(2)[[ACCC]] private(i){{$}}
    // PRT-OA-OSEQ-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-OSEQ-NEXT: //   printf
    // PRT-OA-OSEQ-NEXT: // }
    //
    // PRT-NOACC-NEXT:   for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:     printf
    // PRT-NOACC-NEXT:   }
    #pragma acc parallel loop num_gangs(2) ACCC private(i)
    for (i = 0; i < 2; ++i) {
      // EXE-DAG:        in loop: 0
      // EXE-DAG:        in loop: 1
      // EXE-GREDUN-DAG: in loop: 0
      // EXE-GREDUN-DAG: in loop: 1
      printf("in loop: %d\n", i);
    }
    // PRT-NEXT: printf
    // EXE-NEXT: after loop: 99
    printf("after loop: %d\n", i);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Check multiple privates in same clause and different clauses, including
  // private parallel-local scalar, private assigned loop control variable, and
  // private clauses that become empty or just smaller in translation to
  // OpenMP.
  //--------------------------------------------------

  // PRT-NEXT: printf
  // EXE-NEXT: multiple privates on acc loop
  printf("multiple privates on acc loop\n");
  // PRT-A-NEXT:  #pragma acc parallel
  // PRT-AO-NEXT: // #pragma omp target teams
  // PRT-O-NEXT:  #pragma omp target teams
  // PRT-OA-NEXT: // #pragma acc parallel
  #pragma acc parallel num_gangs(2)
  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    // PRT: int j = 88;
    // PRT: int k = 77;
    int i = 99;
    int j = 88;
    int k = 77;
    // DMP:           ACCLoopDirective
    // DMP-ASEQ-NEXT:   ACCSeqClause
    // DMP-ASEQ-NOT:      <implicit>
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
    // PRT-A-NEXT:       {{^ *}}#pragma acc loop[[ACCC]] private(j,i,k) private(i){{$}}
    // PRT-AO-OPRG-NEXT: {{^ *}}// #pragma omp [[OMPDP]] private(j,i,k) private(i){{$}}
    // PRT-A-NEXT:       for (i = 0; i < 2; ++i) {
    // PRT-A-NEXT:         printf
    // PRT-A-NEXT:         j = k = 55;
    // PRT-A-NEXT:       }
    // PRT-AO-OPLC-NEXT: // {
    // PRT-AO-OPLC-NEXT: //   int i;
    // PRT-AO-OPLC-NEXT: //   #pragma omp [[OMPDP]] private(j,k){{$}}
    // PRT-AO-OPLC-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-OPLC-NEXT: //     printf
    // PRT-AO-OPLC-NEXT: //     j = k = 55;
    // PRT-AO-OPLC-NEXT: //   }
    // PRT-AO-OPLC-NEXT: // }
    // PRT-AO-OSEQ-NEXT: // {
    // PRT-AO-OSEQ-NEXT: //   int j;
    // PRT-AO-OSEQ-NEXT: //   int i;
    // PRT-AO-OSEQ-NEXT: //   int k;
    // PRT-AO-OSEQ-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-OSEQ-NEXT: //     printf
    // PRT-AO-OSEQ-NEXT: //     j = k = 55;
    // PRT-AO-OSEQ-NEXT: //   }
    // PRT-AO-OSEQ-NEXT: // }
    //
    // PRT-O-OPRG-NEXT:  {{^ *}}#pragma omp [[OMPDP]] private(j,i,k) private(i){{$}}
    // PRT-OA-OPRG-NEXT: {{^ *}}// #pragma acc loop[[ACCC]] private(j,i,k) private(i){{$}}
    // PRT-O-OPRG-NEXT:  for (i = 0; i < 2; ++i) {
    // PRT-O-OPRG-NEXT:    printf
    // PRT-O-OPRG-NEXT:    j = k = 55;
    // PRT-O-OPRG-NEXT:  }
    // PRT-O-OPLC-NEXT:  {
    // PRT-O-OPLC-NEXT:    int i;
    // PRT-O-OPLC-NEXT:    {{^ *}}#pragma omp [[OMPDP]] private(j,k){{$}}
    // PRT-O-OPLC-NEXT:    for (i = 0; i < 2; ++i) {
    // PRT-O-OPLC-NEXT:      printf
    // PRT-O-OPLC-NEXT:      j = k = 55;
    // PRT-O-OPLC-NEXT:    }
    // PRT-O-OPLC-NEXT:  }
    // PRT-O-OSEQ-NEXT:  {
    // PRT-O-OSEQ-NEXT:    int j;
    // PRT-O-OSEQ-NEXT:    int i;
    // PRT-O-OSEQ-NEXT:    int k;
    // PRT-O-OSEQ-NEXT:    for (i = 0; i < 2; ++i) {
    // PRT-O-OSEQ-NEXT:      printf
    // PRT-O-OSEQ-NEXT:      j = k = 55;
    // PRT-O-OSEQ-NEXT:    }
    // PRT-O-OSEQ-NEXT:  }
    // PRT-OA-OPLC-NEXT: // #pragma acc loop[[ACCC]] private(j,i,k) private(i){{$}}
    // PRT-OA-OPLC-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-OPLC-NEXT: //   printf
    // PRT-OA-OPLC-NEXT: //   j = k = 55;
    // PRT-OA-OPLC-NEXT: // }
    // PRT-OA-OSEQ-NEXT: // #pragma acc loop[[ACCC]] private(j,i,k) private(i){{$}}
    // PRT-OA-OSEQ-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-OSEQ-NEXT: //   printf
    // PRT-OA-OSEQ-NEXT: //   j = k = 55;
    // PRT-OA-OSEQ-NEXT: // }
    //
    // PRT-NOACC-NEXT:   for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:     printf
    // PRT-NOACC-NEXT:     j = k = 55;
    // PRT-NOACC-NEXT:   }
    #pragma acc loop ACCC private(j,i,k) private(i)
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
  } // PRT-NEXT: }

  // Repeat that but with "acc parallel loop".

  // PRT-NEXT: printf
  // EXE-NEXT: multiple privates on acc parallel loop
  printf("multiple privates on acc parallel loop\n");
  // PRT-NEXT: {
  {
    // PRT: int i = 99;
    // PRT: int j = 88;
    // PRT: int k = 77;
    int i = 99;
    int j = 88;
    int k = 77;
    // DMP:           ACCParallelLoopDirective
    // DMP-NEXT:        ACCNum_gangsClause
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 2
    // DMP-ASEQ-NEXT:   ACCSeqClause
    // DMP-ASEQ-NOT:      <implicit>
    // DMP-AG-NEXT:     ACCGangClause
    // DMP-AW-NEXT:     ACCWorkerClause
    // DMP-AV-NEXT:     ACCVectorClause
    // DMP-NEXT:        ACCPrivateClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:          DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:          DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:        ACCPrivateClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:        effect: ACCParallelDirective
    // DMP-NEXT:          ACCNum_gangsClause
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:          ACCFirstprivateClause {{.*}} <implicit>
    // DMP-NEXT:            DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:            DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:            DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:          impl: OMPTargetTeamsDirective
    // DMP-NEXT:            OMPNum_teamsClause
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:            OMPFirstprivateClause
    // DMP-NOT:               <implicit>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:              DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:              DeclRefExpr {{.*}} 'k' 'int'
    // DMP:                 ACCLoopDirective
    // DMP-ASEQ-NEXT:         ACCSeqClause
    // DMP-ASEQ-NOT:            <implicit>
    // DMP-AG-NEXT:           ACCGangClause
    // DMP-AW-NEXT:           ACCWorkerClause
    // DMP-AV-NEXT:           ACCVectorClause
    // DMP-NEXT:              ACCPrivateClause
    // DMP-NEXT:                DeclRefExpr {{.*}} 'j' 'int'
    // DMP-NEXT:                DeclRefExpr {{.*}} 'i' 'int'
    // DMP-NEXT:                DeclRefExpr {{.*}} 'k' 'int'
    // DMP-NEXT:              ACCPrivateClause
    // DMP-NEXT:                DeclRefExpr {{.*}} 'i' 'int'
    // DMP-AIMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
    // DMP-OPRG-NEXT:         impl: [[OMPDD]]
    // DMP-OPRG-NEXT:           OMPPrivateClause
    // DMP-OPRG-NEXT:             DeclRefExpr {{.*}} 'j' 'int'
    // DMP-OPRG-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
    // DMP-OPRG-NEXT:             DeclRefExpr {{.*}} 'k' 'int'
    // DMP-OPRG-NEXT:           OMPPrivateClause
    // DMP-OPRG-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
    // DMP-OPRG:                ForStmt
    // DMP-OPLC-NEXT:         impl: CompoundStmt
    // DMP-OPLC-NEXT:           DeclStmt
    // DMP-OPLC-NEXT:             VarDecl {{.*}} i 'int'
    // DMP-OPLC-NEXT:           [[OMPDD]]
    // DMP-ONT1-NEXT:             OMPNum_threadsClause
    // DMP-ONT1-NEXT:               IntegerLiteral {{.*}} 'int' 1
    // DMP-OPLC-NEXT:             OMPPrivateClause
    // DMP-OPLC-NEXT:               DeclRefExpr {{.*}} 'j' 'int'
    // DMP-OPLC-NEXT:               DeclRefExpr {{.*}} 'k' 'int'
    // DMP-OPLC:                  ForStmt
    // DMP-OSEQ-NEXT:         impl: CompoundStmt
    // DMP-OSEQ-NEXT:           DeclStmt
    // DMP-OSEQ-NEXT:             VarDecl {{.*}} j 'int'
    // DMP-OSEQ-NEXT:           DeclStmt
    // DMP-OSEQ-NEXT:             VarDecl {{.*}} i 'int'
    // DMP-OSEQ-NEXT:           DeclStmt
    // DMP-OSEQ-NEXT:             VarDecl {{.*}} k 'int'
    // DMP-OSEQ-NEXT:           ForStmt
    //
    // PRT-A-NEXT:       {{^ *}}#pragma acc parallel loop num_gangs(2)[[ACCC]] private(j,i,k) private(i){{$}}
    // PRT-AO-OPRG-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) firstprivate(i,j,k){{$}}
    // PRT-AO-OPRG-NEXT: {{^ *}}// #pragma omp [[OMPDP]] private(j,i,k) private(i){{$}}
    // PRT-A-NEXT:       for (i = 0; i < 2; ++i) {
    // PRT-A-NEXT:         printf
    // PRT-A-NEXT:         j = k = 55;
    // PRT-A-NEXT:       }
    // PRT-AO-OPLC-NEXT: // #pragma omp target teams num_teams(2) firstprivate(i,j,k){{$}}
    // PRT-AO-OPLC-NEXT: // {
    // PRT-AO-OPLC-NEXT: //   int i;
    // PRT-AO-OPLC-NEXT: //   #pragma omp [[OMPDP]] private(j,k){{$}}
    // PRT-AO-OPLC-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-OPLC-NEXT: //     printf
    // PRT-AO-OPLC-NEXT: //     j = k = 55;
    // PRT-AO-OPLC-NEXT: //   }
    // PRT-AO-OPLC-NEXT: // }
    // PRT-AO-OSEQ-NEXT: // #pragma omp target teams num_teams(2) firstprivate(i,j,k){{$}}
    // PRT-AO-OSEQ-NEXT: // {
    // PRT-AO-OSEQ-NEXT: //   int j;
    // PRT-AO-OSEQ-NEXT: //   int i;
    // PRT-AO-OSEQ-NEXT: //   int k;
    // PRT-AO-OSEQ-NEXT: //   for (i = 0; i < 2; ++i) {
    // PRT-AO-OSEQ-NEXT: //     printf
    // PRT-AO-OSEQ-NEXT: //     j = k = 55;
    // PRT-AO-OSEQ-NEXT: //   }
    // PRT-AO-OSEQ-NEXT: // }
    //
    // PRT-O-NEXT:       {{^ *}}#pragma omp target teams num_teams(2) firstprivate(i,j,k){{$}}
    // PRT-O-OPRG-NEXT:  {{^ *}}#pragma omp [[OMPDP]] private(j,i,k) private(i){{$}}
    // PRT-OA-OPRG-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs(2)[[ACCC]] private(j,i,k) private(i){{$}}
    // PRT-O-OPRG-NEXT:  for (i = 0; i < 2; ++i) {
    // PRT-O-OPRG-NEXT:    printf
    // PRT-O-OPRG-NEXT:    j = k = 55;
    // PRT-O-OPRG-NEXT:  }
    // PRT-O-OPLC-NEXT:  {
    // PRT-O-OPLC-NEXT:    int i;
    // PRT-O-OPLC-NEXT:    {{^ *}}#pragma omp [[OMPDP]] private(j,k){{$}}
    // PRT-O-OPLC-NEXT:    for (i = 0; i < 2; ++i) {
    // PRT-O-OPLC-NEXT:      printf
    // PRT-O-OPLC-NEXT:      j = k = 55;
    // PRT-O-OPLC-NEXT:    }
    // PRT-O-OPLC-NEXT:  }
    // PRT-O-OSEQ-NEXT:  {
    // PRT-O-OSEQ-NEXT:    int j;
    // PRT-O-OSEQ-NEXT:    int i;
    // PRT-O-OSEQ-NEXT:    int k;
    // PRT-O-OSEQ-NEXT:    for (i = 0; i < 2; ++i) {
    // PRT-O-OSEQ-NEXT:      printf
    // PRT-O-OSEQ-NEXT:      j = k = 55;
    // PRT-O-OSEQ-NEXT:    }
    // PRT-O-OSEQ-NEXT:  }
    // PRT-OA-OPLC-NEXT: // #pragma acc parallel loop num_gangs(2)[[ACCC]] private(j,i,k) private(i){{$}}
    // PRT-OA-OPLC-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-OPLC-NEXT: //   printf
    // PRT-OA-OPLC-NEXT: //   j = k = 55;
    // PRT-OA-OPLC-NEXT: // }
    // PRT-OA-OSEQ-NEXT: // #pragma acc parallel loop num_gangs(2)[[ACCC]] private(j,i,k) private(i){{$}}
    // PRT-OA-OSEQ-NEXT: // for (i = 0; i < 2; ++i) {
    // PRT-OA-OSEQ-NEXT: //   printf
    // PRT-OA-OSEQ-NEXT: //   j = k = 55;
    // PRT-OA-OSEQ-NEXT: // }
    //
    // PRT-NOACC-NEXT:   for (i = 0; i < 2; ++i) {
    // PRT-NOACC-NEXT:     printf
    // PRT-NOACC-NEXT:     j = k = 55;
    // PRT-NOACC-NEXT:   }
    #pragma acc parallel loop num_gangs(2) ACCC private(j,i,k) private(i)
    for (i = 0; i < 2; ++i) {
      // EXE-DAG:        in loop: i=0
      // EXE-DAG:        in loop: i=1
      // EXE-GREDUN-DAG: in loop: i=0
      // EXE-GREDUN-DAG: in loop: i=1
      printf("in loop: i=%d\n", i);
      j = k = 55;
    }
    // PRT-NEXT: printf
    // EXE-NEXT: after loop: i=99, j=88, k=77
    printf("after loop: i=%d, j=%d, k=%d\n", i, j, k);
  } // PRT-NEXT: }

  // PRT-NEXT: return 0;
  return 0;
} // PRT-NEXT: }
// EXE-NOT: {{.}}

int tentativeDef = 99;
