// Check some special cases for acc loop bounds and increments.
//
// Clang's loop-part-imp-data.c test thoroughly checks what clauses produce
// different kinds of loops and how they dump and print, so here we just focus
// on behavior of loops for a few representative cases.

// RUN: %clang-acc %s -o %t.exe -DTGT_USE_STDIO=%if-tgt-stdio(1, 0)
// RUN: %t.exe > %t.out 2>&1
// RUN: FileCheck -input-file %t.out %s \
// RUN:   -check-prefixes=EXE,EXE-%if-tgt-stdio(,NO-)TGT-USE-STDIO

// END.

/* expected-error 0 {{}} */

// FIXME: Clang produces spurious warning diagnostics for nvptx64 offload.  This
// issue is not limited to Clacc and is present upstream:
/* nvptx64-warning@*:* 0+ {{Linking two modules of different target triples}} */

#include <stdio.h>

#if TGT_USE_STDIO
# define TGT_PRINTF(...) printf(__VA_ARGS__)
#else
# define TGT_PRINTF(...)
#endif

int main() {
  //----------------------------------------------------------------------------
  // Modify loop control variable in body of loop.
  //
  // A sequential loop should be able to affect its number of iterations when
  // modifying its control variable in its body.  A partitioned loop just uses
  // the number of iterations specified in the for init, cond, and incr.
  //----------------------------------------------------------------------------

  //   EXE-NOT: {{.}}
  // EXE-LABEL: modify loop var in body
  //   EXE-NOT: {{.}}
  printf("modify loop var in body\n");

  // EXE-TGT-USE-STDIO-NEXT: > acc loop seq: 0{{$}}
  // EXE-TGT-USE-STDIO-NEXT: > acc loop seq: 0{{$}}
  // EXE-TGT-USE-STDIO-NEXT: > acc loop seq: 0{{$}}
  // EXE-TGT-USE-STDIO-NEXT: > acc loop seq: 0{{$}}
  #pragma acc parallel num_gangs(4)
  #pragma acc loop seq
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("> acc loop seq: %d\n", i);
    i = 2;
  }

  // EXE-TGT-USE-STDIO-NEXT: > acc parallel loop auto: 0{{$}}
  // EXE-TGT-USE-STDIO-NEXT: > acc parallel loop auto: 0{{$}}
  // EXE-TGT-USE-STDIO-NEXT: > acc parallel loop auto: 0{{$}}
  // EXE-TGT-USE-STDIO-NEXT: > acc parallel loop auto: 0{{$}}
  #pragma acc parallel loop num_gangs(4) auto
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("> acc parallel loop auto: %d\n", i);
    i = 2;
  }

  // EXE-TGT-USE-STDIO-NEXT: > acc loop gang: {{[01]$}}
  // EXE-TGT-USE-STDIO-NEXT: > acc loop gang: {{[01]$}}
  #pragma acc parallel num_gangs(4)
  #pragma acc loop gang
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("> acc loop gang: %d\n", i);
    i = 2;
  }

  // EXE-TGT-USE-STDIO-NEXT: > acc parallel loop gang vector: {{[01]$}}
  // EXE-TGT-USE-STDIO-NEXT: > acc parallel loop gang vector: {{[01]$}}
  #pragma acc parallel loop num_gangs(4) gang vector
  for (int i = 0; i < 2; ++i) {
    TGT_PRINTF("> acc parallel loop gang vector: %d\n", i);
    i = 2;
  }

  //--------------------------------------------------
  // Make sure that an init!=0 or inc!=1 work correctly.
  //
  // The latter is specifically relevant to simd directives, which depend on
  // OpenMP's predetermined linear attribute with a step that is the loop inc.
  //--------------------------------------------------

  //   EXE-NOT: {{.}}
  // EXE-LABEL: acc loop seq: init!=0 and inc!= 1
  //   EXE-NOT: {{.}}
  printf("acc loop seq: init!=0 and inc!= 1\n");

  // EXE-TGT-USE-STDIO-DAG: > 7{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 7{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 5{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 5{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: > -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: > -1{{$}}
  #pragma acc parallel num_gangs(2)
  {
    int j;
    #pragma acc loop seq
    for (j = 7; j > -2; j -= 2)
      TGT_PRINTF("> %d\n", j);
  }

  //   EXE-NOT: {{.}}
  // EXE-LABEL: acc parallel loop auto: init!=0 and inc!= 1
  //   EXE-NOT: {{.}}
  printf("acc parallel loop auto: init!=0 and inc!= 1\n");

  // EXE-TGT-USE-STDIO-DAG: > 7{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 7{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 5{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 5{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: > -1{{$}}
  // EXE-TGT-USE-STDIO-DAG: > -1{{$}}
  {
    int j;
    #pragma acc parallel loop num_gangs(2) auto
    for (j = 7; j > -2; j -= 2)
      TGT_PRINTF("> %d\n", j);
  }

  //   EXE-NOT: {{.}}
  // EXE-LABEL: acc loop gang: init!=0 and inc!= 1
  //   EXE-NOT: {{.}}
  printf("acc loop gang: init!=0 and inc!= 1\n");

  // EXE-TGT-USE-STDIO-DAG: > 7{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 5{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: > -1{{$}}
  #pragma acc parallel num_gangs(2)
  {
    int j;
    #pragma acc loop gang
    for (j = 7; j > -2; j -= 2)
      TGT_PRINTF("> %d\n", j);
  }

  //   EXE-NOT: {{.}}
  // EXE-LABEL: acc parallel loop gang vector: init!=0 and inc!= 1
  //   EXE-NOT: {{.}}
  printf("acc parallel loop gang vector: init!=0 and inc!= 1\n");

  // EXE-TGT-USE-STDIO-DAG: > 7{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 5{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 3{{$}}
  // EXE-TGT-USE-STDIO-DAG: > 1{{$}}
  // EXE-TGT-USE-STDIO-DAG: > -1{{$}}
  {
    int j;
    #pragma acc parallel loop num_gangs(2) gang vector
    for (j = 7; j > -2; j -= 2)
      TGT_PRINTF("> %d\n", j);
  }

  return 0;
}

// EXE-NOT: {{.}}
