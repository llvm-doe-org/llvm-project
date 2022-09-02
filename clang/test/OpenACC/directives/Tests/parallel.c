// Check gang-redundant mode for "acc parallel".

// When ADD_LOOP_TO_PAR is not set, this file checks gang-redundant mode for
// "acc parallel" without "loop".
//
// When ADD_LOOP_TO_PAR is set, it adds "loop seq" and a for loop to those "acc
// parallel" directives in order to check gang-redundant mode for combined "acc
// parallel loop" directives.

// FIXME: amdgcn doesn't yet support printf in a kernel.  Unfortunately, that
// means our execution checks on amdgcn don't verify much except that nothing
// crashes.

// REDEFINE: %{exe:args} = 2

// REDEFINE: %{all:clang:args} = 
// REDEFINE: %{all:fc:pres} = PAR
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}

// REDEFINE: %{all:clang:args} = -DADD_LOOP_TO_PAR
// REDEFINE: %{all:fc:pres} = PARLOOP
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}

// END.

/* expected-no-diagnostics */

#include <stdio.h>
#include <stdlib.h>

#if !ADD_LOOP_TO_PAR
# define LOOP
# define FORLOOP_HEAD
#else
# define LOOP loop seq
# define FORLOOP_HEAD for (int i = 0; i < 2; ++i)
#endif

#if TGT_USE_STDIO
# define TGT_PRINTF(...) printf(__VA_ARGS__)
#else
# define TGT_PRINTF(...)
#endif

// PRT: int main(int argc, char *argv[]) {
int main(int argc, char *argv[]) {
  // PRT-NEXT: printf("start\n");
  // EXE: start
  printf("start\n");

  // DMP-PAR:          ACCParallelDirective
  // DMP-PARLOOP:      ACCParallelLoopDirective
  // DMP-PARLOOP-NEXT:   ACCSeqClause
  // DMP-NEXT:           ACCNumGangsClause
  // DMP-NEXT:             IntegerLiteral {{.*}} 'int' 4
  // DMP-PARLOOP-NEXT:   effect: ACCParallelDirective
  // DMP-PARLOOP-NEXT:     ACCNumGangsClause
  // DMP-PARLOOP-NEXT:       IntegerLiteral {{.*}} 'int' 4
  // DMP-NEXT:             impl: OMPTargetTeamsDirective
  // DMP-NEXT:               OMPNum_teamsClause
  // DMP-NEXT:                 IntegerLiteral {{.*}} 'int' 4
  // DMP-PARLOOP:          ACCLoopDirective
  // DMP-PARLOOP-NEXT:       ACCSeqClause
  //
  // PRT-A-PAR-NEXT:      {{^ *}}#pragma acc parallel{{ LOOP | }}num_gangs(4){{ ?$}}
  // PRT-A-PARLOOP-NEXT:  {{^ *}}#pragma acc parallel {{LOOP|loop seq}} num_gangs(4){{ ?$}}
  // PRT-AO-NEXT:         {{^ *}}// #pragma omp target teams num_teams(4){{$}}
  // PRT-O-NEXT:          {{^ *}}#pragma omp target teams num_teams(4){{$}}
  // PRT-OA-PAR-NEXT:     {{^ *}}// #pragma acc parallel{{ LOOP | }}num_gangs(4){{ ?$}}
  // PRT-OA-PARLOOP-NEXT: {{^ *}}// #pragma acc parallel {{LOOP|loop seq}} num_gangs(4){{ ?$}}
  #pragma acc parallel LOOP num_gangs(4) // literal num_gangs argument
  // DMP-PAR-NOT:      ForStmt
  // DMP-PARLOOP-NEXT: impl: ForStmt
  // PRT-PAR-SAME: {{$([[:space:]] *FORLOOP_HEAD)?}}
  // PRT-PARLOOP-NEXT: {{FORLOOP_HEAD|for \(.*\)}}
  FORLOOP_HEAD
    // DMP: CallExpr
    // PRT-NEXT: {{TGT_PRINTF|printf}}("hello world\n");
    //
    //         EXE-TGT-USE-STDIO-NEXT: hello world
    //         EXE-TGT-USE-STDIO-NEXT: hello world
    //         EXE-TGT-USE-STDIO-NEXT: hello world
    //         EXE-TGT-USE-STDIO-NEXT: hello world
    // EXE-TGT-USE-STDIO-PARLOOP-NEXT: hello world
    // EXE-TGT-USE-STDIO-PARLOOP-NEXT: hello world
    // EXE-TGT-USE-STDIO-PARLOOP-NEXT: hello world
    // EXE-TGT-USE-STDIO-PARLOOP-NEXT: hello world
    TGT_PRINTF("hello world\n");

  // DMP-PAR:          ACCParallelDirective
  // DMP-PARLOOP:      ACCParallelLoopDirective
  // DMP-PARLOOP-NEXT:   ACCSeqClause
  // DMP-PARLOOP-NEXT:   effect: ACCParallelDirective
  // DMP-NEXT:             impl: OMPTargetTeamsDirective
  // DMP-PARLOOP:          ACCLoopDirective
  // DMP-PARLOOP-NEXT:       ACCSeqClause
  //
  // PRT-A-PAR-NEXT:      {{^ *}}#pragma acc parallel{{ LOOP$|$}}
  // PRT-A-PARLOOP-NEXT:  {{^ *}}#pragma acc parallel {{(LOOP|loop seq)$}}
  // PRT-AO-NEXT:         {{^ *}}// #pragma omp target teams{{$}}
  // PRT-O-NEXT:          {{^ *}}#pragma omp target teams{{$}}
  // PRT-OA-PAR-NEXT:     {{^ *}}// #pragma acc parallel{{ LOOP$|$}}
  // PRT-OA-PARLOOP-NEXT: {{^ *}}// #pragma acc parallel {{(LOOP|loop seq)$}}
  #pragma acc parallel LOOP
  // DMP-PAR-NOT:      ForStmt
  // DMP-PARLOOP-NEXT: impl: ForStmt
  // PRT-PAR-SAME: {{$([[:space:]] *FORLOOP_HEAD)?}}
  // PRT-PARLOOP-NEXT: {{FORLOOP_HEAD|for \(.*\)}}
  FORLOOP_HEAD
    // DMP: CallExpr
    // PRT-NEXT: {{TGT_PRINTF|printf}}("crazy world\n");
    //         EXE-TGT-USE-STDIO-NEXT: crazy world
    // EXE-TGT-USE-STDIO-PARLOOP-NEXT: crazy world
    TGT_PRINTF("crazy world\n");

  // The number of gangs for the previous parallel construct is determined by
  // the implementation, so we don't know how many times it will print, so
  // we use this to find the end before strictly checking the next set.
  //
  // DMP: CallExpr
  // PRT-NEXT: printf("barrier\n");
  // EXE: barrier
  printf("barrier\n");

  // DMP-PAR:          ACCParallelDirective
  // DMP-PARLOOP:      ACCParallelLoopDirective
  // DMP-PARLOOP-NEXT:   ACCSeqClause
  // DMP-NEXT:           ACCNumGangsClause
  // DMP-NEXT:             CallExpr {{.*}} 'int'
  // DMP-PARLOOP:        effect: ACCParallelDirective
  // DMP-PARLOOP-NEXT:     ACCNumGangsClause
  // DMP-PARLOOP-NEXT:       CallExpr {{.*}} 'int'
  // DMP:                  impl: OMPTargetTeamsDirective
  // DMP-NEXT:               OMPNum_teamsClause
  // DMP-NEXT:                 ImplicitCastExpr
  // DMP-PARLOOP:           ACCLoopDirective
  // DMP-PARLOOP-NEXT:        ACCSeqClause
  //
  // PRT-A-PAR-NEXT:      {{^ *}}#pragma acc parallel{{ LOOP | }}num_gangs(atoi(argv[1])){{ ?$}}
  // PRT-A-PARLOOP-NEXT:  {{^ *}}#pragma acc parallel {{LOOP|loop seq}} num_gangs(atoi(argv[1])){{ ?$}}
  // PRT-AO-NEXT:         {{^ *}}// #pragma omp target teams num_teams(atoi(argv[1])){{$}}
  // PRT-O-NEXT:          {{^ *}}#pragma omp target teams num_teams(atoi(argv[1])){{$}}
  // PRT-OA-PAR-NEXT:     {{^ *}}// #pragma acc parallel{{ LOOP | }}num_gangs(atoi(argv[1])){{ ?$}}
  // PRT-OA-PARLOOP-NEXT: {{^ *}}// #pragma acc parallel {{LOOP|loop seq}} num_gangs(atoi(argv[1])){{ ?$}}
  #pragma acc parallel LOOP num_gangs(atoi(argv[1])) // num_gangs expr with var
  // DMP-PAR-NOT:      ForStmt
  // DMP-PARLOOP-NEXT: impl: ForStmt
  // PRT-PAR-SAME: {{$([[:space:]] *FORLOOP_HEAD)?}}
  // PRT-PARLOOP-NEXT: {{FORLOOP_HEAD|for \(.*\)}}
  FORLOOP_HEAD
    // DMP: CallExpr
    // PRT-NEXT: {{TGT_PRINTF|printf}}("goodbye world\n");
    //         EXE-TGT-USE-STDIO-NEXT: goodbye world
    //         EXE-TGT-USE-STDIO-NEXT: goodbye world
    // EXE-TGT-USE-STDIO-PARLOOP-NEXT: goodbye world
    // EXE-TGT-USE-STDIO-PARLOOP-NEXT: goodbye world
    TGT_PRINTF("goodbye world\n");

  // PRT-NEXT: return 0;
  return 0;
} // PRT-NEXT: }
// EXE-NOT:  {{.}}
