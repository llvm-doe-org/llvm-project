// Check nested acc loops.

// When CMB is not set, this file checks cases when the outermost "acc loop"
// is separate from the enclosing "acc parallel".
//
// When CMB is set, it combines each "acc parallel" with its outermost "acc
// loop" directive in order to check the same cases but for combined "acc
// parallel loop" directives.
//
// RUN: %data cmbs {
// RUN:   (cmb-cflags=      cmb=SEP)
// RUN:   (cmb-cflags=-DCMB cmb=CMB)
// RUN: }

// FIXME: amdgcn doesn't yet support printf in a kernel.  Unfortunately, that
// means our execution checks on amdgcn don't verify much except that nothing
// crashes.
//
// RUN: %for cmbs {
// RUN:   %acc-check-dmp{                                                      \
// RUN:     clang-args: %[cmb-cflags];                                         \
// RUN:     fc-args:    ;                                                      \
// RUN:     fc-pres:    %[cmb]}
// RUN:   %acc-check-prt{                                                      \
// RUN:     clang-args: %[cmb-cflags];                                         \
// RUN:     fc-args:    ;                                                      \
// RUN:     fc-pres:    %[cmb]}
// RUN:   %acc-check-exe{                                                      \
// RUN:     clang-args: %[cmb-cflags]}
// RUN: }

// END.

/* expected-error 0 {{}} */

// FIXME: Clang produces spurious warning diagnostics for nvptx64 offload.  This
// issue is not limited to Clacc and is present upstream:
/* nvptx64-warning@*:* 0+ {{Linking two modules of different target triples}} */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if TGT_USE_STDIO
# define TGT_PRINTF(...) printf(__VA_ARGS__)
#else
# define TGT_PRINTF(...)
#endif

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
  // PRT-SRC-SEP-NEXT: #if !CMB
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
  // PRT-SRC-SEP-NEXT: #else
  // PRT-SRC-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-SRC-SEP-NEXT: #endif
  //
  // PRT-SRC-CMB-NEXT: #if !CMB
  // PRT-SRC-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-SRC-CMB-NEXT:   {
  // PRT-SRC-CMB-NEXT:     #pragma acc loop seq{{$}}
  // PRT-SRC-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-SRC-CMB-NEXT: #endif
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
  //         PRT-NEXT:           {{TGT_PRINTF|printf}}
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-SRC-NEXT: #if !CMB
  //     PRT-SRC-NEXT:   }
  //     PRT-SRC-NEXT: #endif
  //
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 1
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
          TGT_PRINTF("%d, %d, %d\n", i, j, k);
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
  // PRT-SRC-SEP-NEXT: #if !CMB
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
  // PRT-SRC-SEP-NEXT: #else
  // PRT-SRC-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-SRC-SEP-NEXT: #endif
  //
  // PRT-SRC-CMB-NEXT: #if !CMB
  // PRT-SRC-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-SRC-CMB-NEXT:   {
  // PRT-SRC-CMB-NEXT:     #pragma acc loop seq{{$}}
  // PRT-SRC-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-SRC-CMB-NEXT: #endif
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
  //         PRT-NEXT:           {{TGT_PRINTF|printf}}
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-SRC-NEXT: #if !CMB
  //     PRT-SRC-NEXT:   }
  //     PRT-SRC-NEXT: #endif
  //
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 1
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
          TGT_PRINTF("%d, %d, %d\n", i, j, k);
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
  // PRT-SRC-SEP-NEXT: #if !CMB
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
  // PRT-SRC-SEP-NEXT: #else
  // PRT-SRC-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-SRC-SEP-NEXT: #endif
  //
  // PRT-SRC-CMB-NEXT: #if !CMB
  // PRT-SRC-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-SRC-CMB-NEXT:   {
  // PRT-SRC-CMB-NEXT:     #pragma acc loop seq{{$}}
  // PRT-SRC-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-SRC-CMB-NEXT: #endif
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
  //         PRT-NEXT:           {{TGT_PRINTF|printf}}
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-SRC-NEXT: #if !CMB
  //     PRT-SRC-NEXT:   }
  //     PRT-SRC-NEXT: #endif
  //
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 2, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 2, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 3, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 3, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 2, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 2, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 3, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 3, 1
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
          TGT_PRINTF("%d, %d, %d\n", i, j, k);
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
  // PRT-SRC-SEP-NEXT: #if !CMB
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
  // PRT-SRC-SEP-NEXT: #else
  // PRT-SRC-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-SRC-SEP-NEXT: #endif
  //
  // PRT-SRC-CMB-NEXT: #if !CMB
  // PRT-SRC-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-SRC-CMB-NEXT:   {
  // PRT-SRC-CMB-NEXT:     #pragma acc loop seq{{$}}
  // PRT-SRC-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-SRC-CMB-NEXT: #endif
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
  //         PRT-NEXT:           {{TGT_PRINTF|printf}}
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-SRC-NEXT: #if !CMB
  //     PRT-SRC-NEXT:   }
  //     PRT-SRC-NEXT: #endif
  //
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 2, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 2, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 3, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 3, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 2, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 2, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 3, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 3, 1
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
          TGT_PRINTF("%d, %d, %d\n", i, j, k);
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
  // PRT-SRC-SEP-NEXT: #if !CMB
  //   PRT-A-SEP-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  //  PRT-AO-SEP-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-SEP-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-SEP-NEXT:   // #pragma acc parallel num_gangs(2){{$}}
  //     PRT-SEP-NEXT:   {
  //   PRT-A-SEP-NEXT:     #pragma acc loop gang{{$}}
  //  PRT-AO-SEP-NEXT:     // #pragma omp distribute{{$}}
  //   PRT-O-SEP-NEXT:     #pragma omp distribute{{$}}
  //  PRT-OA-SEP-NEXT:     // #pragma acc loop gang{{$}}
  // PRT-SRC-SEP-NEXT: #else
  // PRT-SRC-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) gang{{$}}
  // PRT-SRC-SEP-NEXT: #endif
  //
  // PRT-SRC-CMB-NEXT: #if !CMB
  // PRT-SRC-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-SRC-CMB-NEXT:   {
  // PRT-SRC-CMB-NEXT:     #pragma acc loop gang{{$}}
  // PRT-SRC-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) gang{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp distribute{{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp distribute{{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) gang{{$}}
  // PRT-SRC-CMB-NEXT: #endif
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
  //         PRT-NEXT:           {{TGT_PRINTF|printf}}
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-SRC-NEXT: #if !CMB
  //     PRT-SRC-NEXT:   }
  //     PRT-SRC-NEXT: #endif
  //
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 1
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
          TGT_PRINTF("%d, %d, %d\n", i, j, k);
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
  // PRT-SRC-SEP-NEXT: #if !CMB
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
  // PRT-SRC-SEP-NEXT: #else
  // PRT-SRC-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-SRC-SEP-NEXT: #endif
  //
  // PRT-SRC-CMB-NEXT: #if !CMB
  // PRT-SRC-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-SRC-CMB-NEXT:   {
  // PRT-SRC-CMB-NEXT:     #pragma acc loop seq{{$}}
  // PRT-SRC-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-SRC-CMB-NEXT: #endif
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
  //         PRT-NEXT:           {{TGT_PRINTF|printf}}
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-SRC-NEXT: #if !CMB
  //     PRT-SRC-NEXT:   }
  //     PRT-SRC-NEXT: #endif
  //
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 2, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 2, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 3, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 3, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 2, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 2, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 3, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 3, 1
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
          TGT_PRINTF("%d, %d, %d\n", i, j, k);
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
  // PRT-SRC-SEP-NEXT: #if !CMB
  //   PRT-A-SEP-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  //  PRT-AO-SEP-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-SEP-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-SEP-NEXT:   // #pragma acc parallel num_gangs(2){{$}}
  //     PRT-SEP-NEXT:   {
  //   PRT-A-SEP-NEXT:     #pragma acc loop gang{{$}}
  //  PRT-AO-SEP-NEXT:     // #pragma omp distribute{{$}}
  //   PRT-O-SEP-NEXT:     #pragma omp distribute{{$}}
  //  PRT-OA-SEP-NEXT:     // #pragma acc loop gang{{$}}
  // PRT-SRC-SEP-NEXT: #else
  // PRT-SRC-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) gang{{$}}
  // PRT-SRC-SEP-NEXT: #endif
  //
  // PRT-SRC-CMB-NEXT: #if !CMB
  // PRT-SRC-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-SRC-CMB-NEXT:   {
  // PRT-SRC-CMB-NEXT:     #pragma acc loop gang{{$}}
  // PRT-SRC-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) gang{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp distribute{{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp distribute{{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) gang{{$}}
  // PRT-SRC-CMB-NEXT: #endif
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
  //         PRT-NEXT:           {{TGT_PRINTF|printf}}
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-SRC-NEXT: #if !CMB
  //     PRT-SRC-NEXT:   }
  //     PRT-SRC-NEXT: #endif
  //
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 1
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
          TGT_PRINTF("%d, %d, %d\n", i, j, k);
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
  // PRT-SRC-SEP-NEXT: #if !CMB
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
  // PRT-SRC-SEP-NEXT: #else
  // PRT-SRC-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-SRC-SEP-NEXT: #endif
  //
  // PRT-SRC-CMB-NEXT: #if !CMB
  // PRT-SRC-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-SRC-CMB-NEXT:   {
  // PRT-SRC-CMB-NEXT:     #pragma acc loop seq{{$}}
  // PRT-SRC-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-SRC-CMB-NEXT: #endif
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
  //         PRT-NEXT:           {{TGT_PRINTF|printf}}
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-SRC-NEXT: #if !CMB
  //     PRT-SRC-NEXT:   }
  //     PRT-SRC-NEXT: #endif
  //
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 2, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 2, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 3, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 3, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 2, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 2, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 3, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 3, 1
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
          TGT_PRINTF("%d, %d, %d\n", i, j, k);
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
  // PRT-SRC-SEP-NEXT: #if !CMB
  //   PRT-A-SEP-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  //  PRT-AO-SEP-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-SEP-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-SEP-NEXT:   // #pragma acc parallel num_gangs(2){{$}}
  //     PRT-SEP-NEXT:   {
  //   PRT-A-SEP-NEXT:     #pragma acc loop worker{{$}}
  //  PRT-AO-SEP-NEXT:     // #pragma omp distribute parallel for{{$}}
  //   PRT-O-SEP-NEXT:     #pragma omp distribute parallel for{{$}}
  //  PRT-OA-SEP-NEXT:     // #pragma acc loop worker{{$}}
  // PRT-SRC-SEP-NEXT: #else
  // PRT-SRC-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) worker{{$}}
  // PRT-SRC-SEP-NEXT: #endif
  //
  // PRT-SRC-CMB-NEXT: #if !CMB
  // PRT-SRC-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-SRC-CMB-NEXT:   {
  // PRT-SRC-CMB-NEXT:     #pragma acc loop worker{{$}}
  // PRT-SRC-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) worker{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp distribute parallel for{{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp distribute parallel for{{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) worker{{$}}
  // PRT-SRC-CMB-NEXT: #endif
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
  //         PRT-NEXT:           {{TGT_PRINTF|printf}}
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-SRC-NEXT: #if !CMB
  //     PRT-SRC-NEXT:   }
  //     PRT-SRC-NEXT: #endif
  //
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 2, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 2, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 2, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 2, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 3, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 3, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 3, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 3, 1, 1
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
          TGT_PRINTF("%d, %d, %d\n", i, j, k);
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
  // PRT-SRC-SEP-NEXT: #if !CMB
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
  // PRT-SRC-SEP-NEXT: #else
  // PRT-SRC-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-SRC-SEP-NEXT: #endif
  //
  // PRT-SRC-CMB-NEXT: #if !CMB
  // PRT-SRC-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-SRC-CMB-NEXT:   {
  // PRT-SRC-CMB-NEXT:     #pragma acc loop seq{{$}}
  // PRT-SRC-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-SRC-CMB-NEXT: #endif
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
  //         PRT-NEXT:           {{TGT_PRINTF|printf}}
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-SRC-NEXT: #if !CMB
  //     PRT-SRC-NEXT:   }
  //     PRT-SRC-NEXT: #endif
  //
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 2, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 2, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 3, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 3, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 4, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 4, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 5, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 5, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 2, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 2, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 3, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 3, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 4, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 4, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 5, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 5, 1
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
          TGT_PRINTF("%d, %d, %d\n", i, j, k);
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
  // PRT-SRC-SEP-NEXT: #if !CMB
  //   PRT-A-SEP-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  //  PRT-AO-SEP-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-SEP-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-SEP-NEXT:   // #pragma acc parallel num_gangs(2){{$}}
  //     PRT-SEP-NEXT:   {
  //   PRT-A-SEP-NEXT:     #pragma acc loop gang{{$}}
  //  PRT-AO-SEP-NEXT:     // #pragma omp distribute{{$}}
  //   PRT-O-SEP-NEXT:     #pragma omp distribute{{$}}
  //  PRT-OA-SEP-NEXT:     // #pragma acc loop gang{{$}}
  // PRT-SRC-SEP-NEXT: #else
  // PRT-SRC-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) gang{{$}}
  // PRT-SRC-SEP-NEXT: #endif
  //
  // PRT-SRC-CMB-NEXT: #if !CMB
  // PRT-SRC-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-SRC-CMB-NEXT:   {
  // PRT-SRC-CMB-NEXT:     #pragma acc loop gang{{$}}
  // PRT-SRC-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) gang{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp distribute{{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp distribute{{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) gang{{$}}
  // PRT-SRC-CMB-NEXT: #endif
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
  //         PRT-NEXT:           {{TGT_PRINTF|printf}}
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-SRC-NEXT: #if !CMB
  //     PRT-SRC-NEXT:   }
  //     PRT-SRC-NEXT: #endif
  //
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 1
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
          TGT_PRINTF("%d, %d, %d\n", i, j, k);
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
  // PRT-SRC-SEP-NEXT: #if !CMB
  //   PRT-A-SEP-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  //  PRT-AO-SEP-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-SEP-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-SEP-NEXT:   // #pragma acc parallel num_gangs(2){{$}}
  //     PRT-SEP-NEXT:   {
  //   PRT-A-SEP-NEXT:     #pragma acc loop gang worker{{$}}
  //  PRT-AO-SEP-NEXT:     // #pragma omp distribute parallel for{{$}}
  //   PRT-O-SEP-NEXT:     #pragma omp distribute parallel for{{$}}
  //  PRT-OA-SEP-NEXT:     // #pragma acc loop gang worker{{$}}
  // PRT-SRC-SEP-NEXT: #else
  // PRT-SRC-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) gang worker{{$}}
  // PRT-SRC-SEP-NEXT: #endif
  //
  // PRT-SRC-CMB-NEXT: #if !CMB
  // PRT-SRC-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-SRC-CMB-NEXT:   {
  // PRT-SRC-CMB-NEXT:     #pragma acc loop gang worker{{$}}
  // PRT-SRC-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) gang worker{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp distribute parallel for{{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp distribute parallel for{{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) gang worker{{$}}
  // PRT-SRC-CMB-NEXT: #endif
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
  //         PRT-NEXT:           {{TGT_PRINTF|printf}}
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-SRC-NEXT: #if !CMB
  //     PRT-SRC-NEXT:   }
  //     PRT-SRC-NEXT: #endif
  //
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 2, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 2, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 2, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 2, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 3, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 3, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 3, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 3, 1, 1
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
          TGT_PRINTF("%d, %d, %d\n", i, j, k);
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
  // PRT-SRC-SEP-NEXT: #if !CMB
  //   PRT-A-SEP-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  //  PRT-AO-SEP-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-SEP-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-SEP-NEXT:   // #pragma acc parallel num_gangs(2){{$}}
  //     PRT-SEP-NEXT:   {
  //   PRT-A-SEP-NEXT:     #pragma acc loop gang{{$}}
  //  PRT-AO-SEP-NEXT:     // #pragma omp distribute{{$}}
  //   PRT-O-SEP-NEXT:     #pragma omp distribute{{$}}
  //  PRT-OA-SEP-NEXT:     // #pragma acc loop gang{{$}}
  // PRT-SRC-SEP-NEXT: #else
  // PRT-SRC-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) gang{{$}}
  // PRT-SRC-SEP-NEXT: #endif
  //
  // PRT-SRC-CMB-NEXT: #if !CMB
  // PRT-SRC-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-SRC-CMB-NEXT:   {
  // PRT-SRC-CMB-NEXT:     #pragma acc loop gang{{$}}
  // PRT-SRC-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) gang{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp distribute{{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp distribute{{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) gang{{$}}
  // PRT-SRC-CMB-NEXT: #endif
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
  //         PRT-NEXT:           {{TGT_PRINTF|printf}}
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-SRC-NEXT: #if !CMB
  //     PRT-SRC-NEXT:   }
  //     PRT-SRC-NEXT: #endif
  //
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 2, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 2, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 2, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 2, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 3, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 3, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 3, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 3, 1, 1
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
          TGT_PRINTF("%d, %d, %d\n", i, j, k);
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
  // PRT-SRC-SEP-NEXT: #if !CMB
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
  // PRT-SRC-SEP-NEXT: #else
  // PRT-SRC-SEP-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-SRC-SEP-NEXT: #endif
  //
  // PRT-SRC-CMB-NEXT: #if !CMB
  // PRT-SRC-CMB-NEXT:   #pragma acc parallel num_gangs(2){{$}}
  // PRT-SRC-CMB-NEXT:   {
  // PRT-SRC-CMB-NEXT:     #pragma acc loop seq{{$}}
  // PRT-SRC-CMB-NEXT: #else
  //   PRT-A-CMB-NEXT:   #pragma acc parallel loop num_gangs(2) seq{{$}}
  //  PRT-AO-CMB-NEXT:   // #pragma omp target teams num_teams(2){{$}}
  //   PRT-O-CMB-NEXT:   #pragma omp target teams num_teams(2){{$}}
  //  PRT-OA-CMB-NEXT:   // #pragma acc parallel loop num_gangs(2) seq{{$}}
  // PRT-SRC-CMB-NEXT: #endif
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
  //         PRT-NEXT:           {{TGT_PRINTF|printf}}
  //         PRT-NEXT:       }
  //         PRT-NEXT:     }
  //
  //     PRT-SRC-NEXT: #if !CMB
  //     PRT-SRC-NEXT:   }
  //     PRT-SRC-NEXT: #endif
  //
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 2, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 2, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 3, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 3, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 4, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 4, 1
  // EXE-TGT-USE-STDIO-DAG: 0, 5, 0
  // EXE-TGT-USE-STDIO-DAG: 0, 5, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 0, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 1, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 2, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 2, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 3, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 3, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 4, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 4, 1
  // EXE-TGT-USE-STDIO-DAG: 1, 5, 0
  // EXE-TGT-USE-STDIO-DAG: 1, 5, 1
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
          TGT_PRINTF("%d, %d, %d\n", i, j, k);
      }
    }
#if !CMB
  }
#endif

  // EXE-NOT: {{.}}
  return 0;
}
