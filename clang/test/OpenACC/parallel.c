// Check gang-redundant mode for "acc parallel".
//
// When ADD_LOOP_TO_PAR is not set, this file checks gang-redundant mode for
// "acc parallel" without "loop".
//
// When ADD_LOOP_TO_PAR is set, it adds "loop seq" and a for loop to those "acc
// parallel" directives in order to check gang-redundant mode for combined "acc
// parallel loop" directives.
//
// RUN: %data directives {
// RUN:   (dir=PAR     dir-cflags=                 )
// RUN:   (dir=PARLOOP dir-cflags=-DADD_LOOP_TO_PAR)
// RUN: }

// Check -ast-dump before and after AST serialization.
//
// RUN: %for directives {
// RUN:   %clang -Xclang -verify -Xclang -ast-dump -fsyntax-only -fopenacc %s \
// RUN:          %[dir-cflags] \
// RUN:   | FileCheck -check-prefixes=DMP,DMP-%[dir] %s
// RUN:   %clang -Xclang -verify -fopenacc -emit-ast -o %t.ast %[dir-cflags] %s
// RUN:   %clang_cc1 -ast-dump-all %t.ast \
// RUN:   | FileCheck -check-prefixes=DMP,DMP-%[dir] %s
// RUN: }

// Check -ast-print and -fopenacc[-ast]-print.
//
// RUN: %for directives {
// RUN:   %clang -Xclang -verify -Xclang -ast-print -fsyntax-only %s \
// RUN:          %[dir-cflags] \
// RUN:   | FileCheck -check-prefixes=PRT,PRT-%[dir] %s
// RUN: }
//
// Strip comments and blank lines so checking -fopenacc-print output is easier.
// RUN: echo "// expected""-no-diagnostics" > %t-acc.c
// RUN: grep -v '^ *\(//.*\)\?$' %s | sed 's,//.*,,' >> %t-acc.c
//
// TODO: If lit were to support %for inside a %data, we could iterate prt-opts
// within prt-args after the first prt-args iteration, significantly shortening
// the prt-args definition.
//
// RUN: %data prt-opts {
// RUN:   (prt-opt=-fopenacc-ast-print prt-kind=ast-prt)
// RUN:   (prt-opt=-fopenacc-print     prt-kind=prt    )
// RUN: }
// RUN: %data prt-args {
// RUN:   (prt='-Xclang -ast-print -fsyntax-only -fopenacc' prt-chk=PRT,PRT-%[dir],PRT-A,PRT-A-%[dir])
// RUN:   (prt=-fopenacc-ast-print=acc                      prt-chk=PRT,PRT-%[dir],PRT-A,PRT-A-%[dir])
// RUN:   (prt=-fopenacc-ast-print=omp                      prt-chk=PRT,PRT-%[dir],PRT-O,PRT-O-%[dir])
// RUN:   (prt=-fopenacc-ast-print=acc-omp                  prt-chk=PRT,PRT-%[dir],PRT-A,PRT-A-%[dir],PRT-AO,PRT-AO-%[dir])
// RUN:   (prt=-fopenacc-ast-print=omp-acc                  prt-chk=PRT,PRT-%[dir],PRT-O,PRT-O-%[dir],PRT-OA,PRT-OA-%[dir])
// RUN:   (prt=-fopenacc-print=acc                          prt-chk=PRT,PRT-%[dir],PRT-A,PRT-A-%[dir])
// RUN:   (prt=-fopenacc-print=omp                          prt-chk=PRT,PRT-%[dir],PRT-O,PRT-O-%[dir])
// RUN:   (prt=-fopenacc-print=acc-omp                      prt-chk=PRT,PRT-%[dir],PRT-A,PRT-A-%[dir],PRT-AO,PRT-AO-%[dir])
// RUN:   (prt=-fopenacc-print=omp-acc                      prt-chk=PRT,PRT-%[dir],PRT-O,PRT-O-%[dir],PRT-OA,PRT-OA-%[dir])
// RUN: }
// RUN: %for directives {
// RUN:   %for prt-args {
// RUN:     %clang -Xclang -verify %[prt] %[dir-cflags] %t-acc.c \
// RUN:     | FileCheck -check-prefixes=%[prt-chk] %s
// RUN:   }
// RUN: }

// Check -ast-print after AST serialization.
//
// Some data related to printing (where to print comments about discarded
// directives) is serialized and deserialized, so it's worthwhile to try all
// OpenACC printing modes.
//
// RUN: %for directives {
// RUN:   %clang -Xclang -verify -fopenacc -emit-ast %t-acc.c -o %t.ast \
// RUN:          %[dir-cflags]
// RUN:   %for prt-args {
// RUN:     %clang %[prt] %t.ast 2>&1 \
// RUN:     | FileCheck -check-prefixes=%[prt-chk] %s
// RUN:   }
// RUN: }

// Can we print the OpenMP source code, compile, and run it successfully?
//
// FIXME: amdgcn doesn't yet support printf in a kernel.  Unfortunately, That
// means our execution checks on amdgcn don't verify much except that nothing
// crashes.
//
// RUN: %data tgts {
// RUN:   (run-if=                tgt-cflags='                                     -Xclang -verify'                   tgt-use-stdio=TGT-USE-STDIO   )
// RUN:   (run-if=%run-if-x86_64  tgt-cflags='-fopenmp-targets=%run-x86_64-triple  -Xclang -verify'                   tgt-use-stdio=TGT-USE-STDIO   )
// RUN:   (run-if=%run-if-ppc64le tgt-cflags='-fopenmp-targets=%run-ppc64le-triple -Xclang -verify'                   tgt-use-stdio=TGT-USE-STDIO   )
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags='-fopenmp-targets=%run-nvptx64-triple -Xclang -verify=nvptx64'           tgt-use-stdio=TGT-USE-STDIO   )
// RUN:   (run-if=%run-if-amdgcn  tgt-cflags='-fopenmp-targets=%run-amdgcn-triple  -Xclang -verify -DTGT_USE_STDIO=0' tgt-use-stdio=NO-TGT-USE-STDIO)
// RUN: }
// RUN: %for directives {
// RUN:   %for prt-opts {
// RUN:     %clang -Xclang -verify %[prt-opt]=omp %s > %t-%[prt-kind]-omp.c \
// RUN:       %[dir-cflags]
// RUN:     echo "// expected""-no-diagnostics" >> %t-%[prt-kind]-omp.c
// RUN:   }
// RUN:   %clang -Xclang -verify -fopenmp %fopenmp-version  \
// RUN:          -Wno-unused-function %[dir-cflags] -o %t %t-ast-prt-omp.c
// RUN:   %t 2 > %t.out 2>&1
// RUN:   FileCheck -input-file %t.out %s \
// RUN:     -check-prefixes=EXE,EXE-%[dir] \
// RUN:     -check-prefixes=EXE-TGT-USE-STDIO,EXE-%[dir]-TGT-USE-STDIO
// RUN:   %for tgts {
// RUN:     %[run-if] %clang -fopenmp %fopenmp-version %[tgt-cflags] \
// RUN:       %[dir-cflags] -o %t %t-prt-omp.c
// RUN:     %[run-if] %t 2 > %t.out 2>&1
// RUN:     %[run-if] FileCheck -input-file %t.out %s \
// RUN:       -check-prefixes=EXE,EXE-%[dir] \
// RUN:       -check-prefixes=EXE-%[tgt-use-stdio],EXE-%[dir]-%[tgt-use-stdio]
// RUN:   }
// RUN: }

// Check execution with normal compilation.
//
// RUN: %for directives {
// RUN:   %for tgts {
// RUN:     %[run-if] %clang -fopenacc -o %t %[tgt-cflags] %[dir-cflags] %s
// RUN:     %[run-if] %t 2 > %t.out 2>&1
// RUN:     %[run-if] FileCheck -input-file %t.out %s \
// RUN:       -check-prefixes=EXE,EXE-%[dir] \
// RUN:       -check-prefixes=EXE-%[tgt-use-stdio],EXE-%[dir]-%[tgt-use-stdio]
// RUN:   }
// RUN: }

// END.

// expected-no-diagnostics

// FIXME: Clang produces spurious warning diagnostics for nvptx64 offload.  This
// issue is not limited to Clacc and is present upstream:
// nvptx64-warning@*:* 0+ {{Linking two modules of different data layouts}}
// nvptx64-warning@*:* 0+ {{Linking two modules of different target triples}}

#include <stdio.h>
#include <stdlib.h>

#if !ADD_LOOP_TO_PAR
# define LOOP
# define FORLOOP_HEAD
#else
# define LOOP loop seq
# define FORLOOP_HEAD for (int i = 0; i < 2; ++i)
#endif

#ifndef TGT_USE_STDIO
# define TGT_USE_STDIO 1
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
    // EXE-PARLOOP-TGT-USE-STDIO-NEXT: hello world
    // EXE-PARLOOP-TGT-USE-STDIO-NEXT: hello world
    // EXE-PARLOOP-TGT-USE-STDIO-NEXT: hello world
    // EXE-PARLOOP-TGT-USE-STDIO-NEXT: hello world
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
    // EXE-PARLOOP-TGT-USE-STDIO-NEXT: crazy world
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
    // EXE-PARLOOP-TGT-USE-STDIO-NEXT: goodbye world
    // EXE-PARLOOP-TGT-USE-STDIO-NEXT: goodbye world
    TGT_PRINTF("goodbye world\n");

  // PRT-NEXT: return 0;
  return 0;
} // PRT-NEXT: }
// EXE-NOT:  {{.}}
