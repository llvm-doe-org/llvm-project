// Check nested acc loops.
//
// When CMB is not set, this file checks cases when the outermost "acc loop"
// is separate from the enclosing "acc parallel".
//
// When CMB is set, it combines each "acc parallel" with its outermost "acc
// loop" directive in order to check the same cases but for combined "acc
// parallel loop" directives.
//
// Abbreviations:
//   A     = OpenACC
//   AO    = commented OpenMP is printed after OpenACC
//   O     = OpenMP
//   OA    = commented OpenACC is printed after OpenMP
//
// RUN: %data cmbs {
// RUN:   (cmb-cflags=      cmb=SEP)
// RUN:   (cmb-cflags=-DCMB cmb=CMB)
// RUN: }

// Check -ast-dump before and after AST serialization.
//
// RUN: %for cmbs {
// RUN:   %clang -Xclang -verify -Xclang -ast-dump -fsyntax-only -fopenacc %s \
// RUN:          %[cmb-cflags] \
// RUN:   | FileCheck %s -check-prefixes=DMP,DMP-%[cmb]
// RUN:   %clang -Xclang -verify -fopenacc -emit-ast %[cmb-cflags] -o %t.ast %s
// RUN:   %clang_cc1 -ast-dump-all %t.ast \
// RUN:   | FileCheck %s -check-prefixes=DMP,DMP-%[cmb]
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
// RUN:   (prt='-Xclang -ast-print -fsyntax-only -fopenacc' prt-chk=PRT,PRT-%[cmb],PRT-A,PRT-A-%[cmb])
// RUN:   (prt=-fopenacc-ast-print=acc                      prt-chk=PRT,PRT-%[cmb],PRT-A,PRT-A-%[cmb])
// RUN:   (prt=-fopenacc-ast-print=omp                      prt-chk=PRT,PRT-%[cmb],PRT-O,PRT-O-%[cmb])
// RUN:   (prt=-fopenacc-ast-print=acc-omp                  prt-chk=PRT,PRT-%[cmb],PRT-A,PRT-A-%[cmb],PRT-AO,PRT-AO-%[cmb])
// RUN:   (prt=-fopenacc-ast-print=omp-acc                  prt-chk=PRT,PRT-%[cmb],PRT-O,PRT-O-%[cmb],PRT-OA,PRT-OA-%[cmb])
// RUN:   (prt=-fopenacc-print=acc                          prt-chk=PRT-PRE,PRT-PRE-%[cmb],PRT,PRT-%[cmb],PRT-A,PRT-A-%[cmb])
// RUN:   (prt=-fopenacc-print=omp                          prt-chk=PRT-PRE,PRT-PRE-%[cmb],PRT,PRT-%[cmb],PRT-O,PRT-O-%[cmb])
// RUN:   (prt=-fopenacc-print=acc-omp                      prt-chk=PRT-PRE,PRT-PRE-%[cmb],PRT,PRT-%[cmb],PRT-A,PRT-A-%[cmb],PRT-AO,PRT-AO-%[cmb])
// RUN:   (prt=-fopenacc-print=omp-acc                      prt-chk=PRT-PRE,PRT-PRE-%[cmb],PRT,PRT-%[cmb],PRT-O,PRT-O-%[cmb],PRT-OA,PRT-OA-%[cmb])
// RUN: }
// RUN: %for cmbs {
// RUN:   %for prt-args {
// RUN:     %clang -Xclang -verify %[prt] %t-acc.c %[cmb-cflags] \
// RUN:     | FileCheck -check-prefixes=%[prt-chk] %s
// RUN:   }
// RUN: }

// Check -ast-print after AST serialization.
//
// Some data related to printing (where to print comments about discarded
// directives) is serialized and deserialized, so it's worthwhile to try all
// OpenACC printing modes.
//
// RUN: %for cmbs {
// RUN:   %clang -Xclang -verify -fopenacc -emit-ast %t-acc.c -o %t.ast \
// RUN:          %[cmb-cflags]
// RUN:   %for prt-args {
// RUN:     %clang %[prt] %t.ast 2>&1 \
// RUN:     | FileCheck -check-prefixes=%[prt-chk] %s
// RUN:   }
// RUN: }

// Can we print the OpenMP source code, compile, and run it successfully?
//
// RUN: %for cmbs {
// RUN:   %for prt-opts {
// RUN:     %clang -Xclang -verify %[prt-opt]=omp %s > %t-omp.c %[cmb-cflags]
// RUN:     echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN:     %clang -Xclang -verify -fopenmp %fopenmp-version -o %t %t-omp.c \
// RUN:             %[cmb-cflags]
// RUN:     %t 2>&1 \
// RUN:     | FileCheck -check-prefixes=EXE %s
// RUN:   }
// RUN: }

// Check execution with normal compilation.
//
// RUN: %data tgts {
// RUN:   (run-if=                tgt-cflags=                                    )
// RUN:   (run-if=%run-if-x86_64  tgt-cflags=-fopenmp-targets=%run-x86_64-triple )
// RUN:   (run-if=%run-if-ppc64le tgt-cflags=-fopenmp-targets=%run-ppc64le-triple)
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags=-fopenmp-targets=%run-nvptx64-triple)
// RUN: }
// RUN: %for cmbs {
// RUN:   %for tgts {
// RUN:     %[run-if] %clang -Xclang -verify -fopenacc %s -o %t \
// RUN:                      %[cmb-cflags] %[tgt-cflags]
// RUN:     %[run-if] %t > %t.out 2>&1
// RUN:     %[run-if] FileCheck -input-file %t.out %s -check-prefixes=EXE
// RUN:   }
// RUN: }

// END.

// expected-no-diagnostics

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {

  //--------------------------------------------------
  // Loop nest is entirely sequential.
  //--------------------------------------------------

  // DMP_LABEL: CallExpr
  // PRT-LABEL: printf("seq > seq > seq\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: seq > seq > seq
  printf("seq > seq > seq\n");

  //      DMP-CMB: ACCParallelLoopDirective
  // DMP-CMB-NEXT:   ACCNumGangsClause
  // DMP-CMB-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-CMB-NEXT:   ACCSeqClause
  //      DMP-SEP:   ACCParallelDirective
  // DMP-CMB-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCSeqClause
  //     DMP-NEXT:       impl: ForStmt
  //      DMP-NOT:         OMP
  //          DMP:       ACCLoopDirective
  //     DMP-NEXT:         ACCSeqClause
  //     DMP-NEXT:         ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:         impl: ForStmt
  //      DMP-NOT:           OMP
  //          DMP:         ACCLoopDirective
  //     DMP-NEXT:           ACCSeqClause
  //     DMP-NEXT:           ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'j' 'int'
  //     DMP-NEXT:           impl: ForStmt
  //      DMP-NOT:             OMP
  //      DMP-CMB:   ACCLoopDirective
  //      DMP-CMB:     ACCLoopDirective
  //
  // PRT-PRE-SEP-NEXT: #if !CMB
  //   PRT-A-SEP-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  //  PRT-AO-SEP-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-SEP-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-SEP-NEXT:   // #pragma acc parallel num_gangs(2){{$}}
  //     PRT-SEP-NEXT:   {
  //   PRT-A-SEP-NEXT:     #pragma acc loop seq
  //  PRT-AO-SEP-SAME:     {{^}} // discarded in OpenMP translation
  //   PRT-A-SEP-SAME:     {{^$}}
  //  PRT-OA-SEP-NEXT:     // #pragma acc loop seq
  //  PRT-OA-SEP-SAME:     {{^}} // discarded in OpenMP translation
  //  PRT-OA-SEP-SAME:     {{^$}}
  // PRT-PRE-SEP-NEXT: #else
  // PRT-PRE-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-PRE-SEP-NEXT: #endif
  //
  // PRT-PRE-CMB-NEXT: #if !CMB
  // PRT-PRE-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-PRE-CMB-NEXT:   {
  // PRT-PRE-CMB-NEXT:     #pragma acc loop seq{{$}}
  // PRT-PRE-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-PRE-CMB-NEXT: #endif
  //
  //        PRT-NOACC:   {
  //
  //         PRT-NEXT:     for (int i = 0; i < 2; ++i) {
  //       PRT-A-NEXT:       #pragma acc loop seq
  //      PRT-AO-SAME:       {{^}} // discarded in OpenMP translation
  //       PRT-A-SAME:       {{^$}}
  //      PRT-OA-NEXT:       // #pragma acc loop seq
  //      PRT-OA-SAME:       {{^}} // discarded in OpenMP translation
  //      PRT-OA-SAME:       {{^$}}
  //         PRT-NEXT:       for (int j = 0; j < 2; ++j) {
  //       PRT-A-NEXT:         #pragma acc loop seq
  //      PRT-AO-SAME:         {{^}} // discarded in OpenMP translation
  //       PRT-A-SAME:         {{^$}}
  //      PRT-OA-NEXT:         // #pragma acc loop seq
  //      PRT-OA-SAME:         {{^}} // discarded in OpenMP translation
  //      PRT-OA-SAME:         {{^$}}
  //         PRT-NEXT:         for (int k = 0; k < 2; ++k)
  //         PRT-NEXT:           printf
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-PRE-NEXT: #if !CMB
  //     PRT-PRE-NEXT:   }
  //     PRT-PRE-NEXT: #endif
  //
  // EXE-DAG: 0, 0, 0
  // EXE-DAG: 0, 0, 1
  // EXE-DAG: 0, 1, 0
  // EXE-DAG: 0, 1, 1
  // EXE-DAG: 1, 0, 0
  // EXE-DAG: 1, 0, 1
  // EXE-DAG: 1, 1, 0
  // EXE-DAG: 1, 1, 1
  // EXE-DAG: 0, 0, 0
  // EXE-DAG: 0, 0, 1
  // EXE-DAG: 0, 1, 0
  // EXE-DAG: 0, 1, 1
  // EXE-DAG: 1, 0, 0
  // EXE-DAG: 1, 0, 1
  // EXE-DAG: 1, 1, 0
  // EXE-DAG: 1, 1, 1
#if !CMB
  #pragma acc parallel num_gangs(2)
  {
    #pragma acc loop seq
#else
    #pragma acc parallel loop num_gangs(2) seq
#endif
    for (int i = 0; i < 2; ++i) {
      #pragma acc loop seq
      for (int j = 0; j < 2; ++j) {
        #pragma acc loop seq
        for (int k = 0; k < 2; ++k)
          printf("%d, %d, %d\n", i, j, k);
      }
    }
#if !CMB
  }
#endif

  //--------------------------------------------------
  // Loop nest has explicit gang only.
  //--------------------------------------------------

  // DMP_LABEL: CallExpr
  // PRT-LABEL: printf("seq > gang > seq\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: seq > gang > seq
  printf("seq > gang > seq\n");

  //      DMP-CMB: ACCParallelLoopDirective
  // DMP-CMB-NEXT:   ACCNumGangsClause
  // DMP-CMB-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-CMB-NEXT:   ACCSeqClause
  //      DMP-SEP:   ACCParallelDirective
  // DMP-CMB-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCSeqClause
  //     DMP-NEXT:       impl: ForStmt
  //          DMP:       ACCLoopDirective
  //     DMP-NEXT:         ACCGangClause
  //     DMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:         ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:         impl: OMPDistributeDirective
  //     DMP-NEXT:           OMPSharedClause {{.*}} <implicit>
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
  //      DMP-NOT:           OMP
  //          DMP:         ACCLoopDirective
  //     DMP-NEXT:           ACCSeqClause
  //     DMP-NEXT:           ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'j' 'int'
  //     DMP-NEXT:           impl: ForStmt
  //      DMP-NOT:             OMP
  //      DMP-CMB:   ACCLoopDirective
  //      DMP-CMB:     ACCLoopDirective
  //
  // PRT-PRE-SEP-NEXT: #if !CMB
  //   PRT-A-SEP-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  //  PRT-AO-SEP-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-SEP-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-SEP-NEXT:   // #pragma acc parallel num_gangs(2){{$}}
  //     PRT-SEP-NEXT:   {
  //   PRT-A-SEP-NEXT:     #pragma acc loop seq
  //  PRT-AO-SEP-SAME:     {{^}} // discarded in OpenMP translation
  //   PRT-A-SEP-SAME:     {{^$}}
  //  PRT-OA-SEP-NEXT:     // #pragma acc loop seq
  //  PRT-OA-SEP-SAME:     {{^}} // discarded in OpenMP translation
  //  PRT-OA-SEP-SAME:     {{^$}}
  // PRT-PRE-SEP-NEXT: #else
  // PRT-PRE-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-PRE-SEP-NEXT: #endif
  //
  // PRT-PRE-CMB-NEXT: #if !CMB
  // PRT-PRE-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-PRE-CMB-NEXT:   {
  // PRT-PRE-CMB-NEXT:     #pragma acc loop seq{{$}}
  // PRT-PRE-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-PRE-CMB-NEXT: #endif
  //
  //        PRT-NOACC:   {
  //
  //         PRT-NEXT:     for (int i = 0; i < 2; ++i) {
  //       PRT-A-NEXT:       #pragma acc loop gang{{$}}
  //      PRT-AO-NEXT:       // #pragma omp distribute{{$}}
  //       PRT-O-NEXT:       #pragma omp distribute{{$}}
  //      PRT-OA-NEXT:       // #pragma acc loop gang{{$}}
  //         PRT-NEXT:       for (int j = 0; j < 2; ++j) {
  //       PRT-A-NEXT:         #pragma acc loop seq
  //      PRT-AO-SAME:         {{^}} // discarded in OpenMP translation
  //       PRT-A-SAME:         {{^$}}
  //      PRT-OA-NEXT:         // #pragma acc loop seq
  //      PRT-OA-SAME:         {{^}} // discarded in OpenMP translation
  //      PRT-OA-SAME:         {{^$}}
  //         PRT-NEXT:         for (int k = 0; k < 2; ++k)
  //         PRT-NEXT:           printf
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-PRE-NEXT: #if !CMB
  //     PRT-PRE-NEXT:   }
  //     PRT-PRE-NEXT: #endif
  //
  // EXE-DAG: 0, 0, 0
  // EXE-DAG: 0, 0, 1
  // EXE-DAG: 0, 1, 0
  // EXE-DAG: 0, 1, 1
  // EXE-DAG: 1, 0, 0
  // EXE-DAG: 1, 0, 1
  // EXE-DAG: 1, 1, 0
  // EXE-DAG: 1, 1, 1
#if !CMB
  #pragma acc parallel num_gangs(2)
  {
    #pragma acc loop seq
#else
    #pragma acc parallel loop num_gangs(2) seq
#endif
    for (int i = 0; i < 2; ++i) {
      #pragma acc loop gang
      for (int j = 0; j < 2; ++j) {
        #pragma acc loop seq
        for (int k = 0; k < 2; ++k)
          printf("%d, %d, %d\n", i, j, k);
      }
    }
#if !CMB
  }
#endif

  //--------------------------------------------------
  // Loop nest has explicit worker only.
  //--------------------------------------------------

  // DMP_LABEL: CallExpr
  // PRT-LABEL: printf("seq > worker > seq\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: seq > worker > seq
  printf("seq > worker > seq\n");

  //      DMP-CMB: ACCParallelLoopDirective
  // DMP-CMB-NEXT:   ACCNumGangsClause
  // DMP-CMB-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-CMB-NEXT:   ACCSeqClause
  //      DMP-SEP:   ACCParallelDirective
  // DMP-CMB-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCSeqClause
  //     DMP-NEXT:       impl: ForStmt
  //          DMP:       ACCLoopDirective
  //     DMP-NEXT:         ACCWorkerClause
  //     DMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:         ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:         ACCGangClause {{.*}} <implicit>
  //     DMP-NEXT:         impl: OMPDistributeParallelForDirective
  //     DMP-NEXT:           OMPSharedClause
  //      DMP-NOT:             <implicit>
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
  //      DMP-NOT:           OMP
  //          DMP:         ACCLoopDirective
  //     DMP-NEXT:           ACCSeqClause
  //     DMP-NEXT:           ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'j' 'int'
  //     DMP-NEXT:           impl: ForStmt
  //      DMP-NOT:             OMP
  //      DMP-CMB:   ACCLoopDirective
  //      DMP-CMB:     ACCLoopDirective
  //
  // PRT-PRE-SEP-NEXT: #if !CMB
  //   PRT-A-SEP-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  //  PRT-AO-SEP-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-SEP-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-SEP-NEXT:   // #pragma acc parallel num_gangs(2){{$}}
  //     PRT-SEP-NEXT:   {
  //   PRT-A-SEP-NEXT:     #pragma acc loop seq
  //  PRT-AO-SEP-SAME:     {{^}} // discarded in OpenMP translation
  //   PRT-A-SEP-SAME:     {{^$}}
  //  PRT-OA-SEP-NEXT:     // #pragma acc loop seq
  //  PRT-OA-SEP-SAME:     {{^}} // discarded in OpenMP translation
  //  PRT-OA-SEP-SAME:     {{^$}}
  // PRT-PRE-SEP-NEXT: #else
  // PRT-PRE-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-PRE-SEP-NEXT: #endif
  //
  // PRT-PRE-CMB-NEXT: #if !CMB
  // PRT-PRE-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-PRE-CMB-NEXT:   {
  // PRT-PRE-CMB-NEXT:     #pragma acc loop seq{{$}}
  // PRT-PRE-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-PRE-CMB-NEXT: #endif
  //
  //        PRT-NOACC:   {
  //
  //         PRT-NEXT:     for (int i = 0; i < 2; ++i) {
  //       PRT-A-NEXT:       #pragma acc loop worker{{$}}
  //      PRT-AO-NEXT:       // #pragma omp distribute parallel for shared(i){{$}}
  //       PRT-O-NEXT:       #pragma omp distribute parallel for shared(i){{$}}
  //      PRT-OA-NEXT:       // #pragma acc loop worker{{$}}
  //         PRT-NEXT:       for (int j = 0; j < 4; ++j) {
  //       PRT-A-NEXT:         #pragma acc loop seq
  //      PRT-AO-SAME:         {{^}} // discarded in OpenMP translation
  //       PRT-A-SAME:         {{^$}}
  //      PRT-OA-NEXT:         // #pragma acc loop seq
  //      PRT-OA-SAME:         {{^}} // discarded in OpenMP translation
  //      PRT-OA-SAME:         {{^$}}
  //         PRT-NEXT:         for (int k = 0; k < 2; ++k)
  //         PRT-NEXT:           printf
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-PRE-NEXT: #if !CMB
  //     PRT-PRE-NEXT:   }
  //     PRT-PRE-NEXT: #endif
  //
  // EXE-DAG: 0, 0, 0
  // EXE-DAG: 0, 0, 1
  // EXE-DAG: 0, 1, 0
  // EXE-DAG: 0, 1, 1
  // EXE-DAG: 0, 2, 0
  // EXE-DAG: 0, 2, 1
  // EXE-DAG: 0, 3, 0
  // EXE-DAG: 0, 3, 1
  // EXE-DAG: 1, 0, 0
  // EXE-DAG: 1, 0, 1
  // EXE-DAG: 1, 1, 0
  // EXE-DAG: 1, 1, 1
  // EXE-DAG: 1, 2, 0
  // EXE-DAG: 1, 2, 1
  // EXE-DAG: 1, 3, 0
  // EXE-DAG: 1, 3, 1
#if !CMB
  #pragma acc parallel num_gangs(2)
  {
    #pragma acc loop seq
#else
    #pragma acc parallel loop num_gangs(2) seq
#endif
    for (int i = 0; i < 2; ++i) {
      #pragma acc loop worker
      for (int j = 0; j < 4; ++j) {
        #pragma acc loop seq
        for (int k = 0; k < 2; ++k)
          printf("%d, %d, %d\n", i, j, k);
      }
    }
#if !CMB
  }
#endif

  //--------------------------------------------------
  // Loop nest has explicit vector only.
  //--------------------------------------------------

  // DMP_LABEL: CallExpr
  // PRT-LABEL: printf("seq > vector > seq\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: seq > vector > seq
  printf("seq > vector > seq\n");

  //      DMP-CMB: ACCParallelLoopDirective
  // DMP-CMB-NEXT:   ACCNumGangsClause
  // DMP-CMB-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-CMB-NEXT:   ACCSeqClause
  //      DMP-SEP:   ACCParallelDirective
  // DMP-CMB-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCSeqClause
  //     DMP-NEXT:       impl: ForStmt
  //          DMP:       ACCLoopDirective
  //     DMP-NEXT:         ACCVectorClause
  //     DMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:         ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:         ACCGangClause {{.*}} <implicit>
  //     DMP-NEXT:         impl: OMPDistributeSimdDirective
  //     DMP-NEXT:           OMPSharedClause {{.*}} <implicit>
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
  //      DMP-NOT:           OMP
  //          DMP:         ACCLoopDirective
  //     DMP-NEXT:           ACCSeqClause
  //     DMP-NEXT:           ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'j' 'int'
  //     DMP-NEXT:           impl: ForStmt
  //      DMP-NOT:             OMP
  //      DMP-CMB:   ACCLoopDirective
  //      DMP-CMB:     ACCLoopDirective
  //
  // PRT-PRE-SEP-NEXT: #if !CMB
  //   PRT-A-SEP-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  //  PRT-AO-SEP-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-SEP-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-SEP-NEXT:   // #pragma acc parallel num_gangs(2){{$}}
  //     PRT-SEP-NEXT:   {
  //   PRT-A-SEP-NEXT:     #pragma acc loop seq
  //  PRT-AO-SEP-SAME:     {{^}} // discarded in OpenMP translation
  //   PRT-A-SEP-SAME:     {{^$}}
  //  PRT-OA-SEP-NEXT:     // #pragma acc loop seq
  //  PRT-OA-SEP-SAME:     {{^}} // discarded in OpenMP translation
  //  PRT-OA-SEP-SAME:     {{^$}}
  // PRT-PRE-SEP-NEXT: #else
  // PRT-PRE-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-PRE-SEP-NEXT: #endif
  //
  // PRT-PRE-CMB-NEXT: #if !CMB
  // PRT-PRE-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-PRE-CMB-NEXT:   {
  // PRT-PRE-CMB-NEXT:     #pragma acc loop seq{{$}}
  // PRT-PRE-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-PRE-CMB-NEXT: #endif
  //
  //        PRT-NOACC:   {
  //
  //         PRT-NEXT:     for (int i = 0; i < 2; ++i) {
  //       PRT-A-NEXT:       #pragma acc loop vector{{$}}
  //      PRT-AO-NEXT:       // #pragma omp distribute simd{{$}}
  //       PRT-O-NEXT:       #pragma omp distribute simd{{$}}
  //      PRT-OA-NEXT:       // #pragma acc loop vector{{$}}
  //         PRT-NEXT:       for (int j = 0; j < 4; ++j) {
  //       PRT-A-NEXT:         #pragma acc loop seq
  //      PRT-AO-SAME:         {{^}} // discarded in OpenMP translation
  //       PRT-A-SAME:         {{^$}}
  //      PRT-OA-NEXT:         // #pragma acc loop seq
  //      PRT-OA-SAME:         {{^}} // discarded in OpenMP translation
  //      PRT-OA-SAME:         {{^$}}
  //         PRT-NEXT:         for (int k = 0; k < 2; ++k)
  //         PRT-NEXT:           printf
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-PRE-NEXT: #if !CMB
  //     PRT-PRE-NEXT:   }
  //     PRT-PRE-NEXT: #endif
  //
  // EXE-DAG: 0, 0, 0
  // EXE-DAG: 0, 0, 1
  // EXE-DAG: 0, 1, 0
  // EXE-DAG: 0, 1, 1
  // EXE-DAG: 0, 2, 0
  // EXE-DAG: 0, 2, 1
  // EXE-DAG: 0, 3, 0
  // EXE-DAG: 0, 3, 1
  // EXE-DAG: 1, 0, 0
  // EXE-DAG: 1, 0, 1
  // EXE-DAG: 1, 1, 0
  // EXE-DAG: 1, 1, 1
  // EXE-DAG: 1, 2, 0
  // EXE-DAG: 1, 2, 1
  // EXE-DAG: 1, 3, 0
  // EXE-DAG: 1, 3, 1
#if !CMB
  #pragma acc parallel num_gangs(2)
  {
    #pragma acc loop seq
#else
    #pragma acc parallel loop num_gangs(2) seq
#endif
    for (int i = 0; i < 2; ++i) {
      #pragma acc loop vector
      for (int j = 0; j < 4; ++j) {
        #pragma acc loop seq
        for (int k = 0; k < 2; ++k)
          printf("%d, %d, %d\n", i, j, k);
      }
    }
#if !CMB
  }
#endif

  //--------------------------------------------------
  // Loop nest has explicit gang and worker.
  //--------------------------------------------------

  // DMP_LABEL: CallExpr
  // PRT-LABEL: printf("gang > seq > worker\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: gang > seq > worker
  printf("gang > seq > worker\n");

  //      DMP-CMB: ACCParallelLoopDirective
  // DMP-CMB-NEXT:   ACCNumGangsClause
  // DMP-CMB-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-CMB-NEXT:   ACCGangClause
  //      DMP-SEP:   ACCParallelDirective
  // DMP-CMB-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCGangClause
  //     DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:       impl: OMPDistributeDirective
  //      DMP-NOT:         OMP
  //          DMP:         ForStmt
  //          DMP:       ACCLoopDirective
  //     DMP-NEXT:         ACCSeqClause
  //     DMP-NEXT:         ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:         impl: ForStmt
  //          DMP:         ACCLoopDirective
  //     DMP-NEXT:           ACCWorkerClause
  //     DMP-NEXT:           ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:           ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'j' 'int'
  //     DMP-NEXT:           impl: OMPParallelForDirective
  //     DMP-NEXT:             OMPSharedClause
  //      DMP-NOT:               <implicit>
  //     DMP-NEXT:               DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:               DeclRefExpr {{.*}} 'j' 'int'
  //      DMP-NOT:             OMP
  //          DMP:             ForStmt
  //      DMP-NOT:               OMP
  //      DMP-CMB:   ACCLoopDirective
  //      DMP-CMB:     ACCLoopDirective
  //
  // PRT-PRE-SEP-NEXT: #if !CMB
  //   PRT-A-SEP-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  //  PRT-AO-SEP-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-SEP-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-SEP-NEXT:   // #pragma acc parallel num_gangs(2){{$}}
  //     PRT-SEP-NEXT:   {
  //   PRT-A-SEP-NEXT:     #pragma acc loop gang{{$}}
  //  PRT-AO-SEP-NEXT:     // #pragma omp distribute{{$}}
  //   PRT-O-SEP-NEXT:     #pragma omp distribute{{$}}
  //  PRT-OA-SEP-NEXT:     // #pragma acc loop gang{{$}}
  // PRT-PRE-SEP-NEXT: #else
  // PRT-PRE-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) gang{{$}}
  // PRT-PRE-SEP-NEXT: #endif
  //
  // PRT-PRE-CMB-NEXT: #if !CMB
  // PRT-PRE-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-PRE-CMB-NEXT:   {
  // PRT-PRE-CMB-NEXT:     #pragma acc loop gang{{$}}
  // PRT-PRE-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) gang{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp distribute{{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp distribute{{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) gang{{$}}
  // PRT-PRE-CMB-NEXT: #endif
  //
  //        PRT-NOACC:   {
  //
  //         PRT-NEXT:     for (int i = 0; i < 2; ++i) {
  //       PRT-A-NEXT:       #pragma acc loop seq
  //      PRT-AO-SAME:       {{^}} // discarded in OpenMP translation
  //       PRT-A-SAME:       {{^$}}
  //      PRT-OA-NEXT:       // #pragma acc loop seq
  //      PRT-OA-SAME:       {{^}} // discarded in OpenMP translation
  //      PRT-OA-SAME:       {{^$}}
  //         PRT-NEXT:       for (int j = 0; j < 2; ++j) {
  //       PRT-A-NEXT:         #pragma acc loop worker{{$}}
  //      PRT-AO-NEXT:         // #pragma omp parallel for shared(i,j){{$}}
  //       PRT-O-NEXT:         #pragma omp parallel for shared(i,j){{$}}
  //      PRT-OA-NEXT:         // #pragma acc loop worker{{$}}
  //         PRT-NEXT:         for (int k = 0; k < 2; ++k)
  //         PRT-NEXT:           printf
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-PRE-NEXT: #if !CMB
  //     PRT-PRE-NEXT:   }
  //     PRT-PRE-NEXT: #endif
  //
  // EXE-DAG: 0, 0, 0
  // EXE-DAG: 0, 0, 1
  // EXE-DAG: 0, 1, 0
  // EXE-DAG: 0, 1, 1
  // EXE-DAG: 1, 0, 0
  // EXE-DAG: 1, 0, 1
  // EXE-DAG: 1, 1, 0
  // EXE-DAG: 1, 1, 1
#if !CMB
  #pragma acc parallel num_gangs(2)
  {
    #pragma acc loop gang
#else
    #pragma acc parallel loop num_gangs(2) gang
#endif
    for (int i = 0; i < 2; ++i) {
      #pragma acc loop seq
      for (int j = 0; j < 2; ++j) {
        #pragma acc loop worker
        for (int k = 0; k < 2; ++k)
          printf("%d, %d, %d\n", i, j, k);
      }
    }
#if !CMB
  }
#endif

  // DMP_LABEL: CallExpr
  // PRT-LABEL: printf("seq > gang worker > seq\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: seq > gang worker > seq
  printf("seq > gang worker > seq\n");

  //      DMP-CMB: ACCParallelLoopDirective
  // DMP-CMB-NEXT:   ACCNumGangsClause
  // DMP-CMB-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-CMB-NEXT:   ACCSeqClause
  //      DMP-SEP:   ACCParallelDirective
  // DMP-CMB-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCSeqClause
  //     DMP-NEXT:       impl: ForStmt
  //          DMP:       ACCLoopDirective
  //     DMP-NEXT:         ACCGangClause
  //     DMP-NEXT:         ACCWorkerClause
  //     DMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:         ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:         impl: OMPDistributeParallelForDirective
  //     DMP-NEXT:           OMPSharedClause
  //      DMP-NOT:             <implicit>
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
  //      DMP-NOT:           OMP
  //          DMP:         ACCLoopDirective
  //     DMP-NEXT:           ACCSeqClause
  //     DMP-NEXT:           ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'j' 'int'
  //     DMP-NEXT:           impl: ForStmt
  //      DMP-NOT:             OMP
  //      DMP-CMB:   ACCLoopDirective
  //      DMP-CMB:     ACCLoopDirective
  //
  // PRT-PRE-SEP-NEXT: #if !CMB
  //   PRT-A-SEP-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  //  PRT-AO-SEP-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-SEP-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-SEP-NEXT:   // #pragma acc parallel num_gangs(2){{$}}
  //     PRT-SEP-NEXT:   {
  //   PRT-A-SEP-NEXT:     #pragma acc loop seq
  //  PRT-AO-SEP-SAME:     {{^}} // discarded in OpenMP translation
  //   PRT-A-SEP-SAME:     {{^$}}
  //  PRT-OA-SEP-NEXT:     // #pragma acc loop seq
  //  PRT-OA-SEP-SAME:     {{^}} // discarded in OpenMP translation
  //  PRT-OA-SEP-SAME:     {{^$}}
  // PRT-PRE-SEP-NEXT: #else
  // PRT-PRE-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-PRE-SEP-NEXT: #endif
  //
  // PRT-PRE-CMB-NEXT: #if !CMB
  // PRT-PRE-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-PRE-CMB-NEXT:   {
  // PRT-PRE-CMB-NEXT:     #pragma acc loop seq{{$}}
  // PRT-PRE-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-PRE-CMB-NEXT: #endif
  //
  //        PRT-NOACC:   {
  //
  //         PRT-NEXT:     for (int i = 0; i < 2; ++i) {
  //       PRT-A-NEXT:       #pragma acc loop gang worker{{$}}
  //      PRT-AO-NEXT:       // #pragma omp distribute parallel for shared(i){{$}}
  //       PRT-O-NEXT:       #pragma omp distribute parallel for shared(i){{$}}
  //      PRT-OA-NEXT:       // #pragma acc loop gang worker{{$}}
  //         PRT-NEXT:       for (int j = 0; j < 4; ++j) {
  //       PRT-A-NEXT:         #pragma acc loop seq
  //      PRT-AO-SAME:         {{^}} // discarded in OpenMP translation
  //       PRT-A-SAME:         {{^$}}
  //      PRT-OA-NEXT:         // #pragma acc loop seq
  //      PRT-OA-SAME:         {{^}} // discarded in OpenMP translation
  //      PRT-OA-SAME:         {{^$}}
  //         PRT-NEXT:         for (int k = 0; k < 2; ++k)
  //         PRT-NEXT:           printf
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-PRE-NEXT: #if !CMB
  //     PRT-PRE-NEXT:   }
  //     PRT-PRE-NEXT: #endif
  //
  // EXE-DAG: 0, 0, 0
  // EXE-DAG: 0, 0, 1
  // EXE-DAG: 0, 1, 0
  // EXE-DAG: 0, 1, 1
  // EXE-DAG: 0, 2, 0
  // EXE-DAG: 0, 2, 1
  // EXE-DAG: 0, 3, 0
  // EXE-DAG: 0, 3, 1
  // EXE-DAG: 1, 0, 0
  // EXE-DAG: 1, 0, 1
  // EXE-DAG: 1, 1, 0
  // EXE-DAG: 1, 1, 1
  // EXE-DAG: 1, 2, 0
  // EXE-DAG: 1, 2, 1
  // EXE-DAG: 1, 3, 0
  // EXE-DAG: 1, 3, 1
#if !CMB
  #pragma acc parallel num_gangs(2)
  {
    #pragma acc loop seq
#else
    #pragma acc parallel loop num_gangs(2) seq
#endif
    for (int i = 0; i < 2; ++i) {
      #pragma acc loop gang worker
      for (int j = 0; j < 4; ++j) {
        #pragma acc loop seq
        for (int k = 0; k < 2; ++k)
          printf("%d, %d, %d\n", i, j, k);
      }
    }
#if !CMB
  }
#endif

  //--------------------------------------------------
  // Loop nest has explicit gang and vector.
  //--------------------------------------------------

  // DMP_LABEL: CallExpr
  // PRT-LABEL: printf("gang > seq > vector\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: gang > seq > vector
  printf("gang > seq > vector\n");

  //      DMP-CMB: ACCParallelLoopDirective
  // DMP-CMB-NEXT:   ACCNumGangsClause
  // DMP-CMB-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-CMB-NEXT:   ACCGangClause
  //      DMP-SEP:   ACCParallelDirective
  // DMP-CMB-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCGangClause
  //     DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:       impl: OMPDistributeDirective
  //      DMP-NOT:         OMP
  //          DMP:         ForStmt
  //          DMP:       ACCLoopDirective
  //     DMP-NEXT:         ACCSeqClause
  //     DMP-NEXT:         ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:         impl: ForStmt
  //          DMP:         ACCLoopDirective
  //     DMP-NEXT:           ACCVectorClause
  //     DMP-NEXT:           ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:           ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'j' 'int'
  //     DMP-NEXT:           impl: OMPSimdDirective
  //     DMP-NEXT:             OMPSharedClause {{.*}} <implicit>
  //     DMP-NEXT:               DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:               DeclRefExpr {{.*}} 'j' 'int'
  //      DMP-NOT:             OMP
  //          DMP:             ForStmt
  //      DMP-NOT:               OMP
  //      DMP-CMB:   ACCLoopDirective
  //      DMP-CMB:     ACCLoopDirective
  //
  // PRT-PRE-SEP-NEXT: #if !CMB
  //   PRT-A-SEP-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  //  PRT-AO-SEP-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-SEP-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-SEP-NEXT:   // #pragma acc parallel num_gangs(2){{$}}
  //     PRT-SEP-NEXT:   {
  //   PRT-A-SEP-NEXT:     #pragma acc loop gang{{$}}
  //  PRT-AO-SEP-NEXT:     // #pragma omp distribute{{$}}
  //   PRT-O-SEP-NEXT:     #pragma omp distribute{{$}}
  //  PRT-OA-SEP-NEXT:     // #pragma acc loop gang{{$}}
  // PRT-PRE-SEP-NEXT: #else
  // PRT-PRE-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) gang{{$}}
  // PRT-PRE-SEP-NEXT: #endif
  //
  // PRT-PRE-CMB-NEXT: #if !CMB
  // PRT-PRE-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-PRE-CMB-NEXT:   {
  // PRT-PRE-CMB-NEXT:     #pragma acc loop gang{{$}}
  // PRT-PRE-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) gang{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp distribute{{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp distribute{{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) gang{{$}}
  // PRT-PRE-CMB-NEXT: #endif
  //
  //        PRT-NOACC:   {
  //
  //         PRT-NEXT:     for (int i = 0; i < 2; ++i) {
  //       PRT-A-NEXT:       #pragma acc loop seq
  //      PRT-AO-SAME:       {{^}} // discarded in OpenMP translation
  //       PRT-A-SAME:       {{^$}}
  //      PRT-OA-NEXT:       // #pragma acc loop seq
  //      PRT-OA-SAME:       {{^}} // discarded in OpenMP translation
  //      PRT-OA-SAME:       {{^$}}
  //         PRT-NEXT:       for (int j = 0; j < 2; ++j) {
  //       PRT-A-NEXT:         #pragma acc loop vector{{$}}
  //      PRT-AO-NEXT:         // #pragma omp simd{{$}}
  //       PRT-O-NEXT:         #pragma omp simd{{$}}
  //      PRT-OA-NEXT:         // #pragma acc loop vector{{$}}
  //         PRT-NEXT:         for (int k = 0; k < 2; ++k)
  //         PRT-NEXT:           printf
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-PRE-NEXT: #if !CMB
  //     PRT-PRE-NEXT:   }
  //     PRT-PRE-NEXT: #endif
  //
  // EXE-DAG: 0, 0, 0
  // EXE-DAG: 0, 0, 1
  // EXE-DAG: 0, 1, 0
  // EXE-DAG: 0, 1, 1
  // EXE-DAG: 1, 0, 0
  // EXE-DAG: 1, 0, 1
  // EXE-DAG: 1, 1, 0
  // EXE-DAG: 1, 1, 1
#if !CMB
  #pragma acc parallel num_gangs(2)
  {
    #pragma acc loop gang
#else
    #pragma acc parallel loop num_gangs(2) gang
#endif
    for (int i = 0; i < 2; ++i) {
      #pragma acc loop seq
      for (int j = 0; j < 2; ++j) {
        #pragma acc loop vector
        for (int k = 0; k < 2; ++k)
          printf("%d, %d, %d\n", i, j, k);
      }
    }
#if !CMB
  }
#endif

  // DMP_LABEL: CallExpr
  // PRT-LABEL: printf("seq > gang vector > seq\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: seq > gang vector > seq
  printf("seq > gang vector > seq\n");

  //      DMP-CMB: ACCParallelLoopDirective
  // DMP-CMB-NEXT:   ACCNumGangsClause
  // DMP-CMB-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-CMB-NEXT:   ACCSeqClause
  //      DMP-SEP:   ACCParallelDirective
  // DMP-CMB-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCSeqClause
  //     DMP-NEXT:       impl: ForStmt
  //          DMP:       ACCLoopDirective
  //     DMP-NEXT:         ACCGangClause
  //     DMP-NEXT:         ACCVectorClause
  //     DMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:         ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:         impl: OMPDistributeSimdDirective
  //     DMP-NEXT:           OMPSharedClause {{.*}} <implicit>
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
  //      DMP-NOT:           OMP
  //          DMP:         ACCLoopDirective
  //     DMP-NEXT:           ACCSeqClause
  //     DMP-NEXT:           ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'j' 'int'
  //     DMP-NEXT:           impl: ForStmt
  //      DMP-NOT:             OMP
  //      DMP-CMB:   ACCLoopDirective
  //      DMP-CMB:     ACCLoopDirective
  //
  // PRT-PRE-SEP-NEXT: #if !CMB
  //   PRT-A-SEP-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  //  PRT-AO-SEP-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-SEP-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-SEP-NEXT:   // #pragma acc parallel num_gangs(2){{$}}
  //     PRT-SEP-NEXT:   {
  //   PRT-A-SEP-NEXT:     #pragma acc loop seq
  //  PRT-AO-SEP-SAME:     {{^}} // discarded in OpenMP translation
  //   PRT-A-SEP-SAME:     {{^$}}
  //  PRT-OA-SEP-NEXT:     // #pragma acc loop seq
  //  PRT-OA-SEP-SAME:     {{^}} // discarded in OpenMP translation
  //  PRT-OA-SEP-SAME:     {{^$}}
  // PRT-PRE-SEP-NEXT: #else
  // PRT-PRE-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-PRE-SEP-NEXT: #endif
  //
  // PRT-PRE-CMB-NEXT: #if !CMB
  // PRT-PRE-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-PRE-CMB-NEXT:   {
  // PRT-PRE-CMB-NEXT:     #pragma acc loop seq{{$}}
  // PRT-PRE-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-PRE-CMB-NEXT: #endif
  //
  //        PRT-NOACC:   {
  //
  //         PRT-NEXT:     for (int i = 0; i < 2; ++i) {
  //       PRT-A-NEXT:       #pragma acc loop gang vector{{$}}
  //      PRT-AO-NEXT:       // #pragma omp distribute simd{{$}}
  //       PRT-O-NEXT:       #pragma omp distribute simd{{$}}
  //      PRT-OA-NEXT:       // #pragma acc loop gang vector{{$}}
  //         PRT-NEXT:       for (int j = 0; j < 4; ++j) {
  //       PRT-A-NEXT:         #pragma acc loop seq
  //      PRT-AO-SAME:         {{^}} // discarded in OpenMP translation
  //       PRT-A-SAME:         {{^$}}
  //      PRT-OA-NEXT:         // #pragma acc loop seq
  //      PRT-OA-SAME:         {{^}} // discarded in OpenMP translation
  //      PRT-OA-SAME:         {{^$}}
  //         PRT-NEXT:         for (int k = 0; k < 2; ++k)
  //         PRT-NEXT:           printf
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-PRE-NEXT: #if !CMB
  //     PRT-PRE-NEXT:   }
  //     PRT-PRE-NEXT: #endif
  //
  // EXE-DAG: 0, 0, 0
  // EXE-DAG: 0, 0, 1
  // EXE-DAG: 0, 1, 0
  // EXE-DAG: 0, 1, 1
  // EXE-DAG: 0, 2, 0
  // EXE-DAG: 0, 2, 1
  // EXE-DAG: 0, 3, 0
  // EXE-DAG: 0, 3, 1
  // EXE-DAG: 1, 0, 0
  // EXE-DAG: 1, 0, 1
  // EXE-DAG: 1, 1, 0
  // EXE-DAG: 1, 1, 1
  // EXE-DAG: 1, 2, 0
  // EXE-DAG: 1, 2, 1
  // EXE-DAG: 1, 3, 0
  // EXE-DAG: 1, 3, 1
#if !CMB
  #pragma acc parallel num_gangs(2)
  {
    #pragma acc loop seq
#else
    #pragma acc parallel loop num_gangs(2) seq
#endif
    for (int i = 0; i < 2; ++i) {
      #pragma acc loop gang vector
      for (int j = 0; j < 4; ++j) {
        #pragma acc loop seq
        for (int k = 0; k < 2; ++k)
          printf("%d, %d, %d\n", i, j, k);
      }
    }
#if !CMB
  }
#endif

  //--------------------------------------------------
  // Loop nest has explicit worker and vector.
  //--------------------------------------------------

  // DMP_LABEL: CallExpr
  // PRT-LABEL: printf("worker > seq > vector\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: worker > seq > vector
  printf("worker > seq > vector\n");

  //      DMP-CMB: ACCParallelLoopDirective
  // DMP-CMB-NEXT:   ACCNumGangsClause
  // DMP-CMB-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-CMB-NEXT:   ACCWorkerClause
  //      DMP-SEP:   ACCParallelDirective
  // DMP-CMB-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCWorkerClause
  //     DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:       ACCGangClause {{.*}} <implicit>
  //     DMP-NEXT:       impl: OMPDistributeParallelForDirective
  //      DMP-NOT:         OMP
  //          DMP:         ForStmt
  //          DMP:       ACCLoopDirective
  //     DMP-NEXT:         ACCSeqClause
  //     DMP-NEXT:         ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:         impl: ForStmt
  //          DMP:         ACCLoopDirective
  //     DMP-NEXT:           ACCVectorClause
  //     DMP-NEXT:           ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:           ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'j' 'int'
  //     DMP-NEXT:           impl: OMPSimdDirective
  //     DMP-NEXT:             OMPSharedClause {{.*}} <implicit>
  //     DMP-NEXT:               DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:               DeclRefExpr {{.*}} 'j' 'int'
  //      DMP-NOT:             OMP
  //          DMP:             ForStmt
  //      DMP-NOT:               OMP
  //      DMP-CMB:   ACCLoopDirective
  //      DMP-CMB:     ACCLoopDirective
  //
  // PRT-PRE-SEP-NEXT: #if !CMB
  //   PRT-A-SEP-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  //  PRT-AO-SEP-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-SEP-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-SEP-NEXT:   // #pragma acc parallel num_gangs(2){{$}}
  //     PRT-SEP-NEXT:   {
  //   PRT-A-SEP-NEXT:     #pragma acc loop worker{{$}}
  //  PRT-AO-SEP-NEXT:     // #pragma omp distribute parallel for{{$}}
  //   PRT-O-SEP-NEXT:     #pragma omp distribute parallel for{{$}}
  //  PRT-OA-SEP-NEXT:     // #pragma acc loop worker{{$}}
  // PRT-PRE-SEP-NEXT: #else
  // PRT-PRE-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) worker{{$}}
  // PRT-PRE-SEP-NEXT: #endif
  //
  // PRT-PRE-CMB-NEXT: #if !CMB
  // PRT-PRE-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-PRE-CMB-NEXT:   {
  // PRT-PRE-CMB-NEXT:     #pragma acc loop worker{{$}}
  // PRT-PRE-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) worker{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp distribute parallel for{{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp distribute parallel for{{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) worker{{$}}
  // PRT-PRE-CMB-NEXT: #endif
  //
  //        PRT-NOACC:   {
  //
  //         PRT-NEXT:     for (int i = 0; i < 4; ++i) {
  //       PRT-A-NEXT:       #pragma acc loop seq
  //      PRT-AO-SAME:       {{^}} // discarded in OpenMP translation
  //       PRT-A-SAME:       {{^$}}
  //      PRT-OA-NEXT:       // #pragma acc loop seq
  //      PRT-OA-SAME:       {{^}} // discarded in OpenMP translation
  //      PRT-OA-SAME:       {{^$}}
  //         PRT-NEXT:       for (int j = 0; j < 2; ++j) {
  //       PRT-A-NEXT:         #pragma acc loop vector{{$}}
  //      PRT-AO-NEXT:         // #pragma omp simd{{$}}
  //       PRT-O-NEXT:         #pragma omp simd{{$}}
  //      PRT-OA-NEXT:         // #pragma acc loop vector{{$}}
  //         PRT-NEXT:         for (int k = 0; k < 2; ++k)
  //         PRT-NEXT:           printf
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-PRE-NEXT: #if !CMB
  //     PRT-PRE-NEXT:   }
  //     PRT-PRE-NEXT: #endif
  //
  // EXE-DAG: 0, 0, 0
  // EXE-DAG: 0, 0, 1
  // EXE-DAG: 0, 1, 0
  // EXE-DAG: 0, 1, 1
  // EXE-DAG: 1, 0, 0
  // EXE-DAG: 1, 0, 1
  // EXE-DAG: 1, 1, 0
  // EXE-DAG: 1, 1, 1
  // EXE-DAG: 2, 0, 0
  // EXE-DAG: 2, 0, 1
  // EXE-DAG: 2, 1, 0
  // EXE-DAG: 2, 1, 1
  // EXE-DAG: 3, 0, 0
  // EXE-DAG: 3, 0, 1
  // EXE-DAG: 3, 1, 0
  // EXE-DAG: 3, 1, 1
#if !CMB
  #pragma acc parallel num_gangs(2)
  {
    #pragma acc loop worker
#else
    #pragma acc parallel loop num_gangs(2) worker
#endif
    for (int i = 0; i < 4; ++i) {
      #pragma acc loop seq
      for (int j = 0; j < 2; ++j) {
        #pragma acc loop vector
        for (int k = 0; k < 2; ++k)
          printf("%d, %d, %d\n", i, j, k);
      }
    }
#if !CMB
  }
#endif

  // DMP_LABEL: CallExpr
  // PRT-LABEL: printf("seq > worker vector > seq\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: seq > worker vector > seq
  printf("seq > worker vector > seq\n");

  //      DMP-CMB: ACCParallelLoopDirective
  // DMP-CMB-NEXT:   ACCNumGangsClause
  // DMP-CMB-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-CMB-NEXT:   ACCSeqClause
  //      DMP-SEP:   ACCParallelDirective
  // DMP-CMB-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCSeqClause
  //     DMP-NEXT:       impl: ForStmt
  //          DMP:       ACCLoopDirective
  //     DMP-NEXT:         ACCWorkerClause
  //     DMP-NEXT:         ACCVectorClause
  //     DMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:         ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:         ACCGangClause {{.*}} <implicit>
  //     DMP-NEXT:         impl: OMPDistributeParallelForSimdDirective
  //     DMP-NEXT:           OMPSharedClause
  //      DMP-NOT:             <implicit>
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
  //      DMP-NOT:           OMP
  //          DMP:         ACCLoopDirective
  //     DMP-NEXT:           ACCSeqClause
  //     DMP-NEXT:           ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'j' 'int'
  //     DMP-NEXT:           impl: ForStmt
  //      DMP-NOT:             OMP
  //      DMP-CMB:   ACCLoopDirective
  //      DMP-CMB:     ACCLoopDirective
  //
  // PRT-PRE-SEP-NEXT: #if !CMB
  //   PRT-A-SEP-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  //  PRT-AO-SEP-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-SEP-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-SEP-NEXT:   // #pragma acc parallel num_gangs(2){{$}}
  //     PRT-SEP-NEXT:   {
  //   PRT-A-SEP-NEXT:     #pragma acc loop seq
  //  PRT-AO-SEP-SAME:     {{^}} // discarded in OpenMP translation
  //   PRT-A-SEP-SAME:     {{^$}}
  //  PRT-OA-SEP-NEXT:     // #pragma acc loop seq
  //  PRT-OA-SEP-SAME:     {{^}} // discarded in OpenMP translation
  //  PRT-OA-SEP-SAME:     {{^$}}
  // PRT-PRE-SEP-NEXT: #else
  // PRT-PRE-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-PRE-SEP-NEXT: #endif
  //
  // PRT-PRE-CMB-NEXT: #if !CMB
  // PRT-PRE-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-PRE-CMB-NEXT:   {
  // PRT-PRE-CMB-NEXT:     #pragma acc loop seq{{$}}
  // PRT-PRE-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-PRE-CMB-NEXT: #endif
  //
  //        PRT-NOACC:   {
  //
  //         PRT-NEXT:     for (int i = 0; i < 2; ++i) {
  //       PRT-A-NEXT:       #pragma acc loop worker vector{{$}}
  //      PRT-AO-NEXT:       // #pragma omp distribute parallel for simd shared(i){{$}}
  //       PRT-O-NEXT:       #pragma omp distribute parallel for simd shared(i){{$}}
  //      PRT-OA-NEXT:       // #pragma acc loop worker vector{{$}}
  //         PRT-NEXT:       for (int j = 0; j < 6; ++j) {
  //       PRT-A-NEXT:         #pragma acc loop seq
  //      PRT-AO-SAME:         {{^}} // discarded in OpenMP translation
  //       PRT-A-SAME:         {{^$}}
  //      PRT-OA-NEXT:         // #pragma acc loop seq
  //      PRT-OA-SAME:         {{^}} // discarded in OpenMP translation
  //      PRT-OA-SAME:         {{^$}}
  //         PRT-NEXT:         for (int k = 0; k < 2; ++k)
  //         PRT-NEXT:           printf
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-PRE-NEXT: #if !CMB
  //     PRT-PRE-NEXT:   }
  //     PRT-PRE-NEXT: #endif
  //
  // EXE-DAG: 0, 0, 0
  // EXE-DAG: 0, 0, 1
  // EXE-DAG: 0, 1, 0
  // EXE-DAG: 0, 1, 1
  // EXE-DAG: 0, 2, 0
  // EXE-DAG: 0, 2, 1
  // EXE-DAG: 0, 3, 0
  // EXE-DAG: 0, 3, 1
  // EXE-DAG: 0, 4, 0
  // EXE-DAG: 0, 4, 1
  // EXE-DAG: 0, 5, 0
  // EXE-DAG: 0, 5, 1
  // EXE-DAG: 1, 0, 0
  // EXE-DAG: 1, 0, 1
  // EXE-DAG: 1, 1, 0
  // EXE-DAG: 1, 1, 1
  // EXE-DAG: 1, 2, 0
  // EXE-DAG: 1, 2, 1
  // EXE-DAG: 1, 3, 0
  // EXE-DAG: 1, 3, 1
  // EXE-DAG: 1, 4, 0
  // EXE-DAG: 1, 4, 1
  // EXE-DAG: 1, 5, 0
  // EXE-DAG: 1, 5, 1
#if !CMB
  #pragma acc parallel num_gangs(2)
  {
    #pragma acc loop seq
#else
    #pragma acc parallel loop num_gangs(2) seq
#endif
    for (int i = 0; i < 2; ++i) {
      #pragma acc loop worker vector
      for (int j = 0; j < 6; ++j) {
        #pragma acc loop seq
        for (int k = 0; k < 2; ++k)
          printf("%d, %d, %d\n", i, j, k);
      }
    }
#if !CMB
  }
#endif

  //--------------------------------------------------
  // Loop nest has explicit gang, worker, and vector.
  //--------------------------------------------------

  // DMP_LABEL: CallExpr
  // PRT-LABEL: printf("gang > worker > vector\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: gang > worker > vector
  printf("gang > worker > vector\n");

  //      DMP-CMB: ACCParallelLoopDirective
  // DMP-CMB-NEXT:   ACCNumGangsClause
  // DMP-CMB-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-CMB-NEXT:   ACCGangClause
  //      DMP-SEP:   ACCParallelDirective
  // DMP-CMB-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCGangClause
  //     DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:       impl: OMPDistributeDirective
  //      DMP-NOT:         OMP
  //          DMP:         ForStmt
  //          DMP:       ACCLoopDirective
  //     DMP-NEXT:         ACCWorkerClause
  //     DMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:         ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:         impl: OMPParallelForDirective
  //     DMP-NEXT:           OMPSharedClause
  //      DMP-NOT:             <implicit>
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
  //      DMP-NOT:           OMP
  //          DMP:           ForStmt
  //          DMP:         ACCLoopDirective
  //     DMP-NEXT:           ACCVectorClause
  //     DMP-NEXT:           ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:           ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'j' 'int'
  //     DMP-NEXT:           impl: OMPSimdDirective
  //     DMP-NEXT:             OMPSharedClause {{.*}} <implicit>
  //     DMP-NEXT:               DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:               DeclRefExpr {{.*}} 'j' 'int'
  //      DMP-NOT:             OMP
  //          DMP:             ForStmt
  //      DMP-NOT:               OMP
  //      DMP-CMB:   ACCLoopDirective
  //      DMP-CMB:     ACCLoopDirective
  //
  // PRT-PRE-SEP-NEXT: #if !CMB
  //   PRT-A-SEP-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  //  PRT-AO-SEP-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-SEP-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-SEP-NEXT:   // #pragma acc parallel num_gangs(2){{$}}
  //     PRT-SEP-NEXT:   {
  //   PRT-A-SEP-NEXT:     #pragma acc loop gang{{$}}
  //  PRT-AO-SEP-NEXT:     // #pragma omp distribute{{$}}
  //   PRT-O-SEP-NEXT:     #pragma omp distribute{{$}}
  //  PRT-OA-SEP-NEXT:     // #pragma acc loop gang{{$}}
  // PRT-PRE-SEP-NEXT: #else
  // PRT-PRE-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) gang{{$}}
  // PRT-PRE-SEP-NEXT: #endif
  //
  // PRT-PRE-CMB-NEXT: #if !CMB
  // PRT-PRE-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-PRE-CMB-NEXT:   {
  // PRT-PRE-CMB-NEXT:     #pragma acc loop gang{{$}}
  // PRT-PRE-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) gang{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp distribute{{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp distribute{{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) gang{{$}}
  // PRT-PRE-CMB-NEXT: #endif
  //
  //        PRT-NOACC:   {
  //
  //         PRT-NEXT:     for (int i = 0; i < 2; ++i) {
  //       PRT-A-NEXT:       #pragma acc loop worker{{$}}
  //      PRT-AO-NEXT:       // #pragma omp parallel for shared(i){{$}}
  //       PRT-O-NEXT:       #pragma omp parallel for shared(i){{$}}
  //      PRT-OA-NEXT:       // #pragma acc loop worker{{$}}
  //         PRT-NEXT:       for (int j = 0; j < 2; ++j) {
  //       PRT-A-NEXT:         #pragma acc loop vector{{$}}
  //      PRT-AO-NEXT:         // #pragma omp simd{{$}}
  //       PRT-O-NEXT:         #pragma omp simd{{$}}
  //      PRT-OA-NEXT:         // #pragma acc loop vector{{$}}
  //         PRT-NEXT:         for (int k = 0; k < 2; ++k)
  //         PRT-NEXT:           printf
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-PRE-NEXT: #if !CMB
  //     PRT-PRE-NEXT:   }
  //     PRT-PRE-NEXT: #endif
  //
  // EXE-DAG: 0, 0, 0
  // EXE-DAG: 0, 0, 1
  // EXE-DAG: 0, 1, 0
  // EXE-DAG: 0, 1, 1
  // EXE-DAG: 1, 0, 0
  // EXE-DAG: 1, 0, 1
  // EXE-DAG: 1, 1, 0
  // EXE-DAG: 1, 1, 1
#if !CMB
  #pragma acc parallel num_gangs(2)
  {
    #pragma acc loop gang
#else
    #pragma acc parallel loop num_gangs(2) gang
#endif
    for (int i = 0; i < 2; ++i) {
      #pragma acc loop worker
      for (int j = 0; j < 2; ++j) {
        #pragma acc loop vector
        for (int k = 0; k < 2; ++k)
          printf("%d, %d, %d\n", i, j, k);
      }
    }
#if !CMB
  }
#endif

  // DMP_LABEL: CallExpr
  // PRT-LABEL: printf("gang worker > seq > vector\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: gang worker > seq > vector
  printf("gang worker > seq > vector\n");

  //      DMP-CMB: ACCParallelLoopDirective
  // DMP-CMB-NEXT:   ACCNumGangsClause
  // DMP-CMB-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-CMB-NEXT:   ACCGangClause
  // DMP-CMB-NEXT:   ACCWorkerClause
  //      DMP-SEP:   ACCParallelDirective
  // DMP-CMB-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCGangClause
  //     DMP-NEXT:       ACCWorkerClause
  //     DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:       impl: OMPDistributeParallelForDirective
  //      DMP-NOT:         OMP
  //          DMP:         ForStmt
  //          DMP:       ACCLoopDirective
  //     DMP-NEXT:         ACCSeqClause
  //     DMP-NEXT:         ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:         impl: ForStmt
  //          DMP:         ACCLoopDirective
  //     DMP-NEXT:           ACCVectorClause
  //     DMP-NEXT:           ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:           ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'j' 'int'
  //     DMP-NEXT:           impl: OMPSimdDirective
  //     DMP-NEXT:             OMPSharedClause {{.*}} <implicit>
  //     DMP-NEXT:               DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:               DeclRefExpr {{.*}} 'j' 'int'
  //      DMP-NOT:             OMP
  //          DMP:             ForStmt
  //      DMP-NOT:               OMP
  //      DMP-CMB:   ACCLoopDirective
  //      DMP-CMB:     ACCLoopDirective
  //
  // PRT-PRE-SEP-NEXT: #if !CMB
  //   PRT-A-SEP-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  //  PRT-AO-SEP-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-SEP-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-SEP-NEXT:   // #pragma acc parallel num_gangs(2){{$}}
  //     PRT-SEP-NEXT:   {
  //   PRT-A-SEP-NEXT:     #pragma acc loop gang worker{{$}}
  //  PRT-AO-SEP-NEXT:     // #pragma omp distribute parallel for{{$}}
  //   PRT-O-SEP-NEXT:     #pragma omp distribute parallel for{{$}}
  //  PRT-OA-SEP-NEXT:     // #pragma acc loop gang worker{{$}}
  // PRT-PRE-SEP-NEXT: #else
  // PRT-PRE-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) gang worker{{$}}
  // PRT-PRE-SEP-NEXT: #endif
  //
  // PRT-PRE-CMB-NEXT: #if !CMB
  // PRT-PRE-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-PRE-CMB-NEXT:   {
  // PRT-PRE-CMB-NEXT:     #pragma acc loop gang worker{{$}}
  // PRT-PRE-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) gang worker{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp distribute parallel for{{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp distribute parallel for{{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) gang worker{{$}}
  // PRT-PRE-CMB-NEXT: #endif
  //
  //        PRT-NOACC:   {
  //
  //         PRT-NEXT:     for (int i = 0; i < 4; ++i) {
  //       PRT-A-NEXT:       #pragma acc loop seq
  //      PRT-AO-SAME:       {{^}} // discarded in OpenMP translation
  //       PRT-A-SAME:       {{^$}}
  //      PRT-OA-NEXT:       // #pragma acc loop seq
  //      PRT-OA-SAME:       {{^}} // discarded in OpenMP translation
  //      PRT-OA-SAME:       {{^$}}
  //         PRT-NEXT:       for (int j = 0; j < 2; ++j) {
  //       PRT-A-NEXT:         #pragma acc loop vector{{$}}
  //      PRT-AO-NEXT:         // #pragma omp simd{{$}}
  //       PRT-O-NEXT:         #pragma omp simd{{$}}
  //      PRT-OA-NEXT:         // #pragma acc loop vector{{$}}
  //         PRT-NEXT:         for (int k = 0; k < 2; ++k)
  //         PRT-NEXT:           printf
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-PRE-NEXT: #if !CMB
  //     PRT-PRE-NEXT:   }
  //     PRT-PRE-NEXT: #endif
  //
  // EXE-DAG: 0, 0, 0
  // EXE-DAG: 0, 0, 1
  // EXE-DAG: 0, 1, 0
  // EXE-DAG: 0, 1, 1
  // EXE-DAG: 1, 0, 0
  // EXE-DAG: 1, 0, 1
  // EXE-DAG: 1, 1, 0
  // EXE-DAG: 1, 1, 1
  // EXE-DAG: 2, 0, 0
  // EXE-DAG: 2, 0, 1
  // EXE-DAG: 2, 1, 0
  // EXE-DAG: 2, 1, 1
  // EXE-DAG: 3, 0, 0
  // EXE-DAG: 3, 0, 1
  // EXE-DAG: 3, 1, 0
  // EXE-DAG: 3, 1, 1
#if !CMB
  #pragma acc parallel num_gangs(2)
  {
    #pragma acc loop gang worker
#else
    #pragma acc parallel loop num_gangs(2) gang worker
#endif
    for (int i = 0; i < 4; ++i) {
      #pragma acc loop seq
      for (int j = 0; j < 2; ++j) {
        #pragma acc loop vector
        for (int k = 0; k < 2; ++k)
          printf("%d, %d, %d\n", i, j, k);
      }
    }
#if !CMB
  }
#endif

  // DMP_LABEL: CallExpr
  // PRT-LABEL: printf("gang > seq > worker vector\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: gang > seq > worker vector
  printf("gang > seq > worker vector\n");

  //      DMP-CMB: ACCParallelLoopDirective
  // DMP-CMB-NEXT:   ACCNumGangsClause
  // DMP-CMB-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-CMB-NEXT:   ACCGangClause
  //      DMP-SEP:   ACCParallelDirective
  // DMP-CMB-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCGangClause
  //     DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:       impl: OMPDistributeDirective
  //      DMP-NOT:         OMP
  //          DMP:         ForStmt
  //          DMP:       ACCLoopDirective
  //     DMP-NEXT:         ACCSeqClause
  //     DMP-NEXT:         ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:         impl: ForStmt
  //          DMP:         ACCLoopDirective
  //     DMP-NEXT:           ACCWorkerClause
  //     DMP-NEXT:           ACCVectorClause
  //     DMP-NEXT:           ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:           ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'j' 'int'
  //     DMP-NEXT:           impl: OMPParallelForSimdDirective
  //     DMP-NEXT:             OMPSharedClause
  //      DMP-NOT:               <implicit>
  //     DMP-NEXT:               DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:               DeclRefExpr {{.*}} 'j' 'int'
  //      DMP-NOT:             OMP
  //          DMP:             ForStmt
  //      DMP-NOT:               OMP
  //      DMP-CMB:   ACCLoopDirective
  //      DMP-CMB:     ACCLoopDirective
  //
  // PRT-PRE-SEP-NEXT: #if !CMB
  //   PRT-A-SEP-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  //  PRT-AO-SEP-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-SEP-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-SEP-NEXT:   // #pragma acc parallel num_gangs(2){{$}}
  //     PRT-SEP-NEXT:   {
  //   PRT-A-SEP-NEXT:     #pragma acc loop gang{{$}}
  //  PRT-AO-SEP-NEXT:     // #pragma omp distribute{{$}}
  //   PRT-O-SEP-NEXT:     #pragma omp distribute{{$}}
  //  PRT-OA-SEP-NEXT:     // #pragma acc loop gang{{$}}
  // PRT-PRE-SEP-NEXT: #else
  // PRT-PRE-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) gang{{$}}
  // PRT-PRE-SEP-NEXT: #endif
  //
  // PRT-PRE-CMB-NEXT: #if !CMB
  // PRT-PRE-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-PRE-CMB-NEXT:   {
  // PRT-PRE-CMB-NEXT:     #pragma acc loop gang{{$}}
  // PRT-PRE-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) gang{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp distribute{{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp distribute{{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) gang{{$}}
  // PRT-PRE-CMB-NEXT: #endif
  //
  //        PRT-NOACC:   {
  //
  //         PRT-NEXT:     for (int i = 0; i < 4; ++i) {
  //       PRT-A-NEXT:       #pragma acc loop seq
  //      PRT-AO-SAME:       {{^}} // discarded in OpenMP translation
  //       PRT-A-SAME:       {{^$}}
  //      PRT-OA-NEXT:       // #pragma acc loop seq
  //      PRT-OA-SAME:       {{^}} // discarded in OpenMP translation
  //      PRT-OA-SAME:       {{^$}}
  //         PRT-NEXT:       for (int j = 0; j < 2; ++j) {
  //       PRT-A-NEXT:         #pragma acc loop worker vector{{$}}
  //      PRT-AO-NEXT:         // #pragma omp parallel for simd shared(i,j){{$}}
  //       PRT-O-NEXT:         #pragma omp parallel for simd shared(i,j){{$}}
  //      PRT-OA-NEXT:         // #pragma acc loop worker vector{{$}}
  //         PRT-NEXT:         for (int k = 0; k < 2; ++k)
  //         PRT-NEXT:           printf
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-PRE-NEXT: #if !CMB
  //     PRT-PRE-NEXT:   }
  //     PRT-PRE-NEXT: #endif
  //
  // EXE-DAG: 0, 0, 0
  // EXE-DAG: 0, 0, 1
  // EXE-DAG: 0, 1, 0
  // EXE-DAG: 0, 1, 1
  // EXE-DAG: 1, 0, 0
  // EXE-DAG: 1, 0, 1
  // EXE-DAG: 1, 1, 0
  // EXE-DAG: 1, 1, 1
  // EXE-DAG: 2, 0, 0
  // EXE-DAG: 2, 0, 1
  // EXE-DAG: 2, 1, 0
  // EXE-DAG: 2, 1, 1
  // EXE-DAG: 3, 0, 0
  // EXE-DAG: 3, 0, 1
  // EXE-DAG: 3, 1, 0
  // EXE-DAG: 3, 1, 1
#if !CMB
  #pragma acc parallel num_gangs(2)
  {
    #pragma acc loop gang
#else
    #pragma acc parallel loop num_gangs(2) gang
#endif
    for (int i = 0; i < 4; ++i) {
      #pragma acc loop seq
      for (int j = 0; j < 2; ++j) {
        #pragma acc loop worker vector
        for (int k = 0; k < 2; ++k)
          printf("%d, %d, %d\n", i, j, k);
      }
    }
#if !CMB
  }
#endif

  // DMP_LABEL: CallExpr
  // PRT-LABEL: printf("seq > gang worker vector > seq\n");
  //   EXE-NOT: {{.}}
  // EXE-LABEL: seq > gang worker vector > seq
  printf("seq > gang worker vector > seq\n");

  //      DMP-CMB: ACCParallelLoopDirective
  // DMP-CMB-NEXT:   ACCNumGangsClause
  // DMP-CMB-NEXT:     IntegerLiteral {{.*}} 'int' 2
  // DMP-CMB-NEXT:   ACCSeqClause
  //      DMP-SEP:   ACCParallelDirective
  // DMP-CMB-NEXT:   effect: ACCParallelDirective
  //     DMP-NEXT:     ACCNumGangsClause
  //     DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  //     DMP-NEXT:     impl: OMPTargetTeamsDirective
  //     DMP-NEXT:       OMPNum_teamsClause
  //     DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //          DMP:     ACCLoopDirective
  //     DMP-NEXT:       ACCSeqClause
  //     DMP-NEXT:       impl: ForStmt
  //          DMP:       ACCLoopDirective
  //     DMP-NEXT:         ACCGangClause
  //     DMP-NEXT:         ACCWorkerClause
  //     DMP-NEXT:         ACCVectorClause
  //     DMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
  //     DMP-NEXT:         ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:           DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:         impl: OMPDistributeParallelForSimdDirective
  //     DMP-NEXT:           OMPSharedClause
  //      DMP-NOT:             <implicit>
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
  //      DMP-NOT:           OMP
  //          DMP:         ACCLoopDirective
  //     DMP-NEXT:           ACCSeqClause
  //     DMP-NEXT:           ACCSharedClause {{.*}} <implicit>
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'i' 'int'
  //     DMP-NEXT:             DeclRefExpr {{.*}} 'j' 'int'
  //     DMP-NEXT:           impl: ForStmt
  //      DMP-NOT:             OMP
  //      DMP-CMB:   ACCLoopDirective
  //      DMP-CMB:     ACCLoopDirective
  //
  // PRT-PRE-SEP-NEXT: #if !CMB
  //   PRT-A-SEP-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  //  PRT-AO-SEP-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-SEP-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-SEP-NEXT:   // #pragma acc parallel num_gangs(2){{$}}
  //     PRT-SEP-NEXT:   {
  //   PRT-A-SEP-NEXT:     #pragma acc loop seq
  //  PRT-AO-SEP-SAME:     {{^}} // discarded in OpenMP translation
  //   PRT-A-SEP-SAME:     {{^$}}
  //  PRT-OA-SEP-NEXT:     // #pragma acc loop seq
  //  PRT-OA-SEP-SAME:     {{^}} // discarded in OpenMP translation
  //  PRT-OA-SEP-SAME:     {{^$}}
  // PRT-PRE-SEP-NEXT: #else
  // PRT-PRE-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-PRE-SEP-NEXT: #endif
  //
  // PRT-PRE-CMB-NEXT: #if !CMB
  // PRT-PRE-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-PRE-CMB-NEXT:   {
  // PRT-PRE-CMB-NEXT:     #pragma acc loop seq{{$}}
  // PRT-PRE-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-PRE-CMB-NEXT: #endif
  //
  //        PRT-NOACC:   {
  //
  //         PRT-NEXT:     for (int i = 0; i < 2; ++i) {
  //       PRT-A-NEXT:       #pragma acc loop gang worker vector{{$}}
  //      PRT-AO-NEXT:       // #pragma omp distribute parallel for simd shared(i){{$}}
  //       PRT-O-NEXT:       #pragma omp distribute parallel for simd shared(i){{$}}
  //      PRT-OA-NEXT:       // #pragma acc loop gang worker vector{{$}}
  //         PRT-NEXT:       for (int j = 0; j < 6; ++j) {
  //       PRT-A-NEXT:         #pragma acc loop seq
  //      PRT-AO-SAME:         {{^}} // discarded in OpenMP translation
  //       PRT-A-SAME:         {{^$}}
  //      PRT-OA-NEXT:         // #pragma acc loop seq
  //      PRT-OA-SAME:         {{^}} // discarded in OpenMP translation
  //      PRT-OA-SAME:         {{^$}}
  //         PRT-NEXT:         for (int k = 0; k < 2; ++k)
  //         PRT-NEXT:           printf
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-PRE-NEXT: #if !CMB
  //     PRT-PRE-NEXT:   }
  //     PRT-PRE-NEXT: #endif
  //
  // EXE-DAG: 0, 0, 0
  // EXE-DAG: 0, 0, 1
  // EXE-DAG: 0, 1, 0
  // EXE-DAG: 0, 1, 1
  // EXE-DAG: 0, 2, 0
  // EXE-DAG: 0, 2, 1
  // EXE-DAG: 0, 3, 0
  // EXE-DAG: 0, 3, 1
  // EXE-DAG: 0, 4, 0
  // EXE-DAG: 0, 4, 1
  // EXE-DAG: 0, 5, 0
  // EXE-DAG: 0, 5, 1
  // EXE-DAG: 1, 0, 0
  // EXE-DAG: 1, 0, 1
  // EXE-DAG: 1, 1, 0
  // EXE-DAG: 1, 1, 1
  // EXE-DAG: 1, 2, 0
  // EXE-DAG: 1, 2, 1
  // EXE-DAG: 1, 3, 0
  // EXE-DAG: 1, 3, 1
  // EXE-DAG: 1, 4, 0
  // EXE-DAG: 1, 4, 1
  // EXE-DAG: 1, 5, 0
  // EXE-DAG: 1, 5, 1
#if !CMB
  #pragma acc parallel num_gangs(2)
  {
    #pragma acc loop seq
#else
    #pragma acc parallel loop num_gangs(2) seq
#endif
    for (int i = 0; i < 2; ++i) {
      #pragma acc loop gang worker vector
      for (int j = 0; j < 6; ++j) {
        #pragma acc loop seq
        for (int k = 0; k < 2; ++k)
          printf("%d, %d, %d\n", i, j, k);
      }
    }
#if !CMB
  }
#endif

  // EXE-NOT: {{.}}
  return 0;
}
