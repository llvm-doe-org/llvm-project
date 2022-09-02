// Check level-of-parallelism clauses for "acc routine".
//
// Check that each is translated properly.  Check that specifying each on
// multiple different routine directives for the same function is ok.

// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}

// END.

/* expected-no-diagnostics */

#include <openacc.h>
#include <stdio.h>

typedef struct {
  int Host;
  int NotHost;
} Result;

#define WRITE_RESULT(Res)                                                      \
  do {                                                                         \
    (Res)->Host = acc_on_device(acc_device_host);                              \
    (Res)->NotHost = acc_on_device(acc_device_not_host);                       \
  } while (0)

#define PRINT_RESULT(Fn, Res)                                                  \
    printf("%s: host=%d, not_host=%d\n", #Fn, Res.Host, Res.NotHost)

// EXE-NOT: {{.}}

// This is just so we can use PRT*-NEXT and EXE-NEXT consistently from now on.
// PRT: void start() {
// PRT: }{{$}}
// EXE: start
void start() { printf("start\n"); }

//--------------------------------------------------
// gang clause.  Definition before prototype.
//--------------------------------------------------

//      DMP: FunctionDecl {{.*}} gang 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Gang OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//      DMP: FunctionDecl {{.*}} gang 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Gang OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//  PRT-A-NEXT: {{^ *}}#pragma acc routine gang{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine gang{{$}}
//    PRT-NEXT: void gang(Result *Res) {
//         PRT: }{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
//
//  PRT-A-NEXT: {{^ *}}#pragma acc routine gang{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine gang{{$}}
//    PRT-NEXT: void gang(Result *Res);
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
//
// EXE-HOST-NEXT: gang: host=1, not_host=0
//  EXE-OFF-NEXT: gang: host=0, not_host=1
#pragma acc routine gang
void gang(Result *Res) { WRITE_RESULT(Res); }
#pragma acc routine gang
void gang(Result *Res);

//--------------------------------------------------
// worker clause.  Prototype before defintiion.
//--------------------------------------------------

//      DMP: FunctionDecl {{.*}} worker 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Worker OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//      DMP: FunctionDecl {{.*}} worker 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Worker OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//  PRT-A-NEXT: {{^ *}}#pragma acc routine worker{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine worker{{$}}
//    PRT-NEXT: void worker(Result *Res);
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
//
//  PRT-A-NEXT: {{^ *}}#pragma acc routine worker{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine worker{{$}}
//    PRT-NEXT: void worker(Result *Res) {
//         PRT: }{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
//
// EXE-HOST-NEXT: worker: host=1, not_host=0
//  EXE-OFF-NEXT: worker: host=0, not_host=1
#pragma acc routine worker
void worker(Result *Res);
#pragma acc routine worker
void worker(Result *Res) { WRITE_RESULT(Res); }

//--------------------------------------------------
// vector clause.  Prototype before and after definition.
//--------------------------------------------------

//      DMP: FunctionDecl {{.*}} vector 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Vector OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//      DMP: FunctionDecl {{.*}} vector 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Vector OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//      DMP: FunctionDecl {{.*}} vector 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Vector OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//  PRT-A-NEXT: {{^ *}}#pragma acc routine vector{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine vector{{$}}
//    PRT-NEXT: void vector(Result *Res);
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
//
//  PRT-A-NEXT: {{^ *}}#pragma acc routine vector{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine vector{{$}}
//    PRT-NEXT: void vector(Result *Res) {
//         PRT: }{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
//
//  PRT-A-NEXT: {{^ *}}#pragma acc routine vector{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine vector{{$}}
//    PRT-NEXT: void vector(Result *Res);
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
//
// EXE-HOST-NEXT: vector: host=1, not_host=0
//  EXE-OFF-NEXT: vector: host=0, not_host=1
#pragma acc routine vector
void vector(Result *Res);
#pragma acc routine vector
void vector(Result *Res) { WRITE_RESULT(Res); }
#pragma acc routine vector
void vector(Result *Res);

//--------------------------------------------------
// seq clause.  Definition only.
//--------------------------------------------------

//      DMP: FunctionDecl {{.*}} seq 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
//    PRT-NEXT: void seq(Result *Res) {
//         PRT: }{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
//
//      EXE-NEXT: seq: host=1, not_host=0
// EXE-HOST-NEXT: seq: host=1, not_host=0
//  EXE-OFF-NEXT: seq: host=0, not_host=1
#pragma acc routine seq
void seq(Result *Res) { WRITE_RESULT(Res); }

int main(int argc, char *argv[]) {
  start();
  Result Res;

  #pragma acc parallel num_gangs(1) copyout(Res)
  gang(&Res); PRINT_RESULT(gang, Res);

  #pragma acc parallel num_gangs(1) copyout(Res)
  worker(&Res); PRINT_RESULT(worker, Res);

  #pragma acc parallel num_gangs(1) copyout(Res)
  vector(&Res); PRINT_RESULT(vector, Res);

  seq(&Res); PRINT_RESULT(seq, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  seq(&Res); PRINT_RESULT(seq, Res);

  return 0;
}

// EXE-NOT: {{.}}
