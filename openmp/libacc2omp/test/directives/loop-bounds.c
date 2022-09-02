// Check some special cases for acc loop bounds and increments.
//
// Clang's loop-part-imp-data.c test thoroughly checks what clauses produce
// different kinds of loops and how they dump and print, so here we just focus
// on behavior of loops for a few representative cases.

// RUN: %clang-acc %s -o %t.exe -DTGT_USE_STDIO=%if-tgt-stdio<1|0> \
// RUN:            -DTGT_%dev-type-0-OMP
// RUN: %t.exe > %t.out 2>&1
// RUN: FileCheck -input-file %t.out %s \
// RUN:   -check-prefixes=EXE,EXE-%if-tgt-stdio<|NO->TGT-USE-STDIO

// END.

/* expected-no-diagnostics */

#include <stdio.h>

#if TGT_USE_STDIO
# define TGT_PRINTF(...) printf(__VA_ARGS__)
#else
# define TGT_PRINTF(...)
#endif

#pragma acc routine gang
static void withinGangFn();
#pragma acc routine worker
static void withinWorkerFn();
#pragma acc routine vector
static void withinVectorFn();
#pragma acc routine seq
static void withinSeqFn();

//==============================================================================
// Check within parallel constructs.
//==============================================================================

int main() {
  //----------------------------------------------------------------------------
  // Modify loop control variable in body of loop.
  //
  // A sequential loop should be able to affect its number of iterations when
  // modifying its control variable in its body.  A partitioned loop just uses
  // the number of iterations specified in the for init, cond, and incr.
  //----------------------------------------------------------------------------

  //                EXE-NOT: {{.}}
  //              EXE-LABEL: modify loop var in body: acc loop seq
  //                EXE-NOT: {{.}}
  // EXE-TGT-USE-STDIO-NEXT: > 0{{$}}
  // EXE-TGT-USE-STDIO-NEXT: > 0{{$}}
  // EXE-TGT-USE-STDIO-NEXT: > 0{{$}}
  // EXE-TGT-USE-STDIO-NEXT: > 0{{$}}
  printf("modify loop var in body: acc loop seq\n");
  #pragma acc parallel num_gangs(4)
  #pragma acc loop seq
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("> %d\n", i);
    i = 2;
  }

  //                EXE-NOT: {{.}}
  //              EXE-LABEL: modify loop var in body: acc parallel loop auto
  //                EXE-NOT: {{.}}
  // EXE-TGT-USE-STDIO-NEXT: > 0{{$}}
  // EXE-TGT-USE-STDIO-NEXT: > 0{{$}}
  // EXE-TGT-USE-STDIO-NEXT: > 0{{$}}
  // EXE-TGT-USE-STDIO-NEXT: > 0{{$}}
  printf("modify loop var in body: acc parallel loop auto\n");
  #pragma acc parallel loop num_gangs(4) auto
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("> %d\n", i);
    i = 2;
  }

  //               EXE-NOT: {{.}}
  //             EXE-LABEL: modify loop var in body: acc loop gang
  //               EXE-NOT: {{.}}
  // EXE-TGT-USE-STDIO-DAG: > 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 1{{$}}
  printf("modify loop var in body: acc loop gang\n");
  #pragma acc parallel num_gangs(4)
  #pragma acc loop gang
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("> %d\n", i);
    i = 2;
  }

  //               EXE-NOT: {{.}}
  //             EXE-LABEL: modify loop var in body: acc parallel loop gang vector
  //               EXE-NOT: {{.}}
  // EXE-TGT-USE-STDIO-DAG: > 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 1{{$}}
  printf("modify loop var in body: acc parallel loop gang vector\n");
  #pragma acc parallel loop num_gangs(4) gang vector
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("> %d\n", i);
    i = 2;
  }

  //--------------------------------------------------
  // Make sure that an init!=0 or inc!=1 work correctly.
  //
  // The latter is specifically relevant to simd directives, which depend on
  // OpenMP's predetermined linear attribute with a step that is the loop inc.
  //--------------------------------------------------

  //               EXE-NOT: {{.}}
  //             EXE-LABEL: init!=0 and inc!=1: acc loop seq
  //               EXE-NOT: {{.}}
  // EXE-TGT-USE-STDIO-DAG: > 7{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 5{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: > -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 7{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 5{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: > -1{{$}}
  printf("init!=0 and inc!=1: acc loop seq\n");
  #pragma acc parallel num_gangs(2)
  {
    int j;
    #pragma acc loop seq
    for (j = 7; j > -2; j -= 2)
      TGT_PRINTF("> %d\n", j);
  }

  //               EXE-NOT: {{.}}
  //             EXE-LABEL: init!=0 and inc!=1: acc parallel loop auto
  //               EXE-NOT: {{.}}
  // EXE-TGT-USE-STDIO-DAG: > 7{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 5{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: > -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 7{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 5{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: > -1{{$}}
  printf("init!=0 and inc!=1: acc parallel loop auto\n");
  {
    int j;
    #pragma acc parallel loop num_gangs(2) auto
    for (j = 7; j > -2; j -= 2)
      TGT_PRINTF("> %d\n", j);
  }

  //               EXE-NOT: {{.}}
  //             EXE-LABEL: init!=0 and inc!=1: acc loop gang
  //               EXE-NOT: {{.}}
  // EXE-TGT-USE-STDIO-DAG: > 7{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 5{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: > -1{{$}}
  printf("init!=0 and inc!=1: acc loop gang\n");
  #pragma acc parallel num_gangs(2)
  {
    int j;
    #pragma acc loop gang
    for (j = 7; j > -2; j -= 2)
      TGT_PRINTF("> %d\n", j);
  }

  //               EXE-NOT: {{.}}
  //             EXE-LABEL: init!=0 and inc!=1: acc parallel loop gang vector
  //               EXE-NOT: {{.}}
  // EXE-TGT-USE-STDIO-DAG: > 7{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 5{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: > -1{{$}}
  printf("init!=0 and inc!=1: acc parallel loop gang vector\n");
  {
    int j;
    #pragma acc parallel loop num_gangs(2) gang vector
    for (j = 7; j > -2; j -= 2)
      TGT_PRINTF("> %d\n", j);
  }

  //----------------------------------------------------------------------------
  // Check within accelerator routines.
  //----------------------------------------------------------------------------

  //   EXE-NOT: {{.}}
  // EXE-LABEL: within accelerator routines
  //   EXE-NOT: {{.}}
  printf("within accelerator routines\n");

  #pragma acc parallel num_gangs(4)
  {
    withinGangFn();
    withinWorkerFn();
    withinVectorFn();
    withinSeqFn();
  }

  return 0;
}

//==============================================================================
// Check within gang functions.
//
// This repeats the above testing but within a gang function and without
// statically enclosing parallel constructs.
//==============================================================================

static void withinGangFn() {
  //----------------------------------------------------------------------------
  // Modify loop control variable in body of loop.
  //----------------------------------------------------------------------------

  // EXE-TGT-USE-STDIO-DAG: withinGangFn: modify loop var in body: acc loop seq: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: modify loop var in body: acc loop seq: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: modify loop var in body: acc loop seq: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: modify loop var in body: acc loop seq: 0{{$}}
  #pragma acc loop seq
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("withinGangFn: modify loop var in body: acc loop seq: %d\n", i);
    i = 2;
  }

  // EXE-TGT-USE-STDIO-DAG: withinGangFn: modify loop var in body: acc loop auto: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: modify loop var in body: acc loop auto: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: modify loop var in body: acc loop auto: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: modify loop var in body: acc loop auto: 0{{$}}
  #pragma acc loop auto
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("withinGangFn: modify loop var in body: acc loop auto: %d\n", i);
    i = 2;
  }

  // EXE-TGT-USE-STDIO-DAG: withinGangFn: modify loop var in body: acc loop gang: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: modify loop var in body: acc loop gang: 1{{$}}
  #pragma acc loop gang
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("withinGangFn: modify loop var in body: acc loop gang: %d\n", i);
    i = 2;
  }

  // EXE-TGT-USE-STDIO-DAG: withinGangFn: modify loop var in body: acc loop gang vector: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: modify loop var in body: acc loop gang vector: 1{{$}}
  #pragma acc loop gang vector
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("withinGangFn: modify loop var in body: acc loop gang vector: %d\n", i);
    i = 2;
  }

  //--------------------------------------------------
  // Make sure that an init!=0 or inc!=1 work correctly.
  //--------------------------------------------------

  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop seq: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop seq: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop seq: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop seq: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop seq: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop seq: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop seq: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop seq: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop seq: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop seq: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop seq: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop seq: -1{{$}}
  #pragma acc loop seq
  for (int j = 3; j > -2; j -= 2)
    TGT_PRINTF("withinGangFn: init!=0 and inc!=1: acc loop seq: %d\n", j);

  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop auto: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop auto: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop auto: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop auto: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop auto: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop auto: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop auto: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop auto: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop auto: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop auto: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop auto: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop auto: -1{{$}}
  #pragma acc loop auto
  for (int j = 3; j > -2; j -= 2)
    TGT_PRINTF("withinGangFn: init!=0 and inc!=1: acc loop auto: %d\n", j);

  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop gang: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop gang: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop gang: -1{{$}}
  #pragma acc loop gang
  for (int j = 3; j > -2; j -= 2)
    TGT_PRINTF("withinGangFn: init!=0 and inc!=1: acc loop gang: %d\n", j);

  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop gang vector: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop gang vector: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinGangFn: init!=0 and inc!=1: acc loop gang vector: -1{{$}}
  #pragma acc loop gang vector
  for (int j = 3; j > -2; j -= 2)
    TGT_PRINTF("withinGangFn: init!=0 and inc!=1: acc loop gang vector: %d\n", j);
}

//==============================================================================
// Check within worker functions.
//
// This repeats the above testing but within a worker function, so gang clauses
// become worker clauses.
//==============================================================================

static void withinWorkerFn() {
  //----------------------------------------------------------------------------
  // Modify loop control variable in body of loop.
  //----------------------------------------------------------------------------

  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: modify loop var in body: acc loop seq: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: modify loop var in body: acc loop seq: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: modify loop var in body: acc loop seq: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: modify loop var in body: acc loop seq: 0{{$}}
  #pragma acc loop seq
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("withinWorkerFn: modify loop var in body: acc loop seq: %d\n", i);
    i = 2;
  }

  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: modify loop var in body: acc loop auto: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: modify loop var in body: acc loop auto: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: modify loop var in body: acc loop auto: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: modify loop var in body: acc loop auto: 0{{$}}
  #pragma acc loop auto
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("withinWorkerFn: modify loop var in body: acc loop auto: %d\n", i);
    i = 2;
  }

  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: modify loop var in body: acc loop worker: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: modify loop var in body: acc loop worker: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: modify loop var in body: acc loop worker: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: modify loop var in body: acc loop worker: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: modify loop var in body: acc loop worker: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: modify loop var in body: acc loop worker: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: modify loop var in body: acc loop worker: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: modify loop var in body: acc loop worker: 1{{$}}
  #pragma acc loop worker
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("withinWorkerFn: modify loop var in body: acc loop worker: %d\n", i);
    i = 2;
  }

  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: modify loop var in body: acc loop worker vector: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: modify loop var in body: acc loop worker vector: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: modify loop var in body: acc loop worker vector: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: modify loop var in body: acc loop worker vector: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: modify loop var in body: acc loop worker vector: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: modify loop var in body: acc loop worker vector: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: modify loop var in body: acc loop worker vector: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: modify loop var in body: acc loop worker vector: 1{{$}}
  #pragma acc loop worker vector
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("withinWorkerFn: modify loop var in body: acc loop worker vector: %d\n", i);
    i = 2;
  }

  //--------------------------------------------------
  // Make sure that an init!=0 or inc!=1 work correctly.
  //--------------------------------------------------

  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop seq: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop seq: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop seq: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop seq: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop seq: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop seq: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop seq: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop seq: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop seq: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop seq: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop seq: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop seq: -1{{$}}
  #pragma acc loop seq
  for (int j = 3; j > -2; j -= 2)
    TGT_PRINTF("withinWorkerFn: init!=0 and inc!=1: acc loop seq: %d\n", j);

  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop auto: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop auto: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop auto: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop auto: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop auto: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop auto: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop auto: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop auto: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop auto: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop auto: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop auto: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop auto: -1{{$}}
  #pragma acc loop auto
  for (int j = 3; j > -2; j -= 2)
    TGT_PRINTF("withinWorkerFn: init!=0 and inc!=1: acc loop auto: %d\n", j);

  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop worker: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop worker: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop worker: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop worker: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop worker: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop worker: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop worker: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop worker: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop worker: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop worker: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop worker: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop worker: -1{{$}}
  #pragma acc loop worker
  for (int j = 3; j > -2; j -= 2)
    TGT_PRINTF("withinWorkerFn: init!=0 and inc!=1: acc loop worker: %d\n", j);

  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop worker vector: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop worker vector: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop worker vector: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop worker vector: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop worker vector: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop worker vector: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop worker vector: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop worker vector: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop worker vector: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop worker vector: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop worker vector: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinWorkerFn: init!=0 and inc!=1: acc loop worker vector: -1{{$}}
  #pragma acc loop worker vector
  for (int j = 3; j > -2; j -= 2)
    TGT_PRINTF("withinWorkerFn: init!=0 and inc!=1: acc loop worker vector: %d\n", j);
}

//==============================================================================
// Check within vector functions.
//
// This repeats the above testing but within a vector function, so worker clauses
// become vector clauses, and worker vector loops are dropped.
//==============================================================================

static void withinVectorFn() {
  //----------------------------------------------------------------------------
  // Modify loop control variable in body of loop.
  //----------------------------------------------------------------------------

  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: modify loop var in body: acc loop seq: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: modify loop var in body: acc loop seq: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: modify loop var in body: acc loop seq: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: modify loop var in body: acc loop seq: 0{{$}}
  #pragma acc loop seq
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("withinVectorFn: modify loop var in body: acc loop seq: %d\n", i);
    i = 2;
  }

  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: modify loop var in body: acc loop auto: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: modify loop var in body: acc loop auto: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: modify loop var in body: acc loop auto: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: modify loop var in body: acc loop auto: 0{{$}}
  #pragma acc loop auto
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("withinVectorFn: modify loop var in body: acc loop auto: %d\n", i);
    i = 2;
  }

  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: modify loop var in body: acc loop vector: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: modify loop var in body: acc loop vector: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: modify loop var in body: acc loop vector: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: modify loop var in body: acc loop vector: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: modify loop var in body: acc loop vector: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: modify loop var in body: acc loop vector: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: modify loop var in body: acc loop vector: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: modify loop var in body: acc loop vector: 1{{$}}
  #pragma acc loop vector
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("withinVectorFn: modify loop var in body: acc loop vector: %d\n", i);
    i = 2;
  }

  //--------------------------------------------------
  // Make sure that an init!=0 or inc!=1 work correctly.
  //--------------------------------------------------

  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop seq: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop seq: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop seq: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop seq: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop seq: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop seq: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop seq: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop seq: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop seq: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop seq: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop seq: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop seq: -1{{$}}
  #pragma acc loop seq
  for (int j = 3; j > -2; j -= 2)
    TGT_PRINTF("withinVectorFn: init!=0 and inc!=1: acc loop seq: %d\n", j);

  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop auto: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop auto: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop auto: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop auto: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop auto: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop auto: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop auto: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop auto: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop auto: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop auto: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop auto: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop auto: -1{{$}}
  #pragma acc loop auto
  for (int j = 3; j > -2; j -= 2)
    TGT_PRINTF("withinVectorFn: init!=0 and inc!=1: acc loop auto: %d\n", j);

  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop vector: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop vector: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop vector: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop vector: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop vector: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop vector: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop vector: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop vector: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop vector: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop vector: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop vector: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinVectorFn: init!=0 and inc!=1: acc loop vector: -1{{$}}
  #pragma acc loop vector
  for (int j = 3; j > -2; j -= 2)
    TGT_PRINTF("withinVectorFn: init!=0 and inc!=1: acc loop vector: %d\n", j);
}

//==============================================================================
// Check within seq functions.
//
// This repeats the above testing but within a seq function, so vector loops are
// dropped.
//==============================================================================

static void withinSeqFn() {
  //----------------------------------------------------------------------------
  // Modify loop control variable in body of loop.
  //----------------------------------------------------------------------------

  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: modify loop var in body: acc loop seq: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: modify loop var in body: acc loop seq: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: modify loop var in body: acc loop seq: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: modify loop var in body: acc loop seq: 0{{$}}
  #pragma acc loop seq
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("withinSeqFn: modify loop var in body: acc loop seq: %d\n", i);
    i = 2;
  }

  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: modify loop var in body: acc loop auto: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: modify loop var in body: acc loop auto: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: modify loop var in body: acc loop auto: 0{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: modify loop var in body: acc loop auto: 0{{$}}
  #pragma acc loop auto
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("withinSeqFn: modify loop var in body: acc loop auto: %d\n", i);
    i = 2;
  }

  //--------------------------------------------------
  // Make sure that an init!=0 or inc!=1 work correctly.
  //--------------------------------------------------

  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: init!=0 and inc!=1: acc loop seq: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: init!=0 and inc!=1: acc loop seq: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: init!=0 and inc!=1: acc loop seq: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: init!=0 and inc!=1: acc loop seq: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: init!=0 and inc!=1: acc loop seq: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: init!=0 and inc!=1: acc loop seq: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: init!=0 and inc!=1: acc loop seq: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: init!=0 and inc!=1: acc loop seq: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: init!=0 and inc!=1: acc loop seq: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: init!=0 and inc!=1: acc loop seq: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: init!=0 and inc!=1: acc loop seq: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: init!=0 and inc!=1: acc loop seq: -1{{$}}
  #pragma acc loop seq
  for (int j = 3; j > -2; j -= 2)
    TGT_PRINTF("withinSeqFn: init!=0 and inc!=1: acc loop seq: %d\n", j);

  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: init!=0 and inc!=1: acc loop auto: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: init!=0 and inc!=1: acc loop auto: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: init!=0 and inc!=1: acc loop auto: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: init!=0 and inc!=1: acc loop auto: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: init!=0 and inc!=1: acc loop auto: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: init!=0 and inc!=1: acc loop auto: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: init!=0 and inc!=1: acc loop auto: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: init!=0 and inc!=1: acc loop auto: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: init!=0 and inc!=1: acc loop auto: -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: init!=0 and inc!=1: acc loop auto: 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: init!=0 and inc!=1: acc loop auto: 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: withinSeqFn: init!=0 and inc!=1: acc loop auto: -1{{$}}
  #pragma acc loop auto
  for (int j = 3; j > -2; j -= 2)
    TGT_PRINTF("withinSeqFn: init!=0 and inc!=1: acc loop auto: %d\n", j);
}

// EXE-NOT: {{.}}
