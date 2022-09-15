// Check that, when an explicit "acc routine seq" inherits a translation
// (an OpenMP directive or a discard) from a previous "acc routine seq", that
// translation is replaced as needed for the new context.
//
// REDEFINE: %{dmp:fc:args} =                                                  \
// REDEFINE:   -implicit-check-not=ACCRoutineDeclAttr                          \
// REDEFINE:   -implicit-check-not=OMPDeclareTargetDeclAttr
// REDEFINE: %{prt:fc:args} =                                                  \
// REDEFINE:   -implicit-check-not='{{#pragma *acc *routine}}'                 \
// REDEFINE:   -implicit-check-not='{{#pragma *omp *declare *target}}'         \
// REDEFINE:   -implicit-check-not='{{#pragma *omp *end *declare *target}}'
// REDEFINE: %{exe:fc:args} =                                                  \
// REDEFINE:   -implicit-check-not='{{[^[:space:]]}}'
//
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}
//
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
    printf("  %s: host=%d, not_host=%d\n", #Fn, Res.Host, Res.NotHost)

// DMP-LABEL: declBeforeDef_userStart
//  DMP-NEXT: FunctionDecl {{.*}} declBeforeDef_user 'void (Result *)'
//  DMP-NEXT:   ParmVarDecl
//  DMP-NEXT:   CompoundStmt
//  DMP-NEXT:     DeclStmt
//  DMP-NEXT:       FunctionDecl {{.*}} declBeforeDef 'void (Result *)'
//  DMP-NEXT:         ParmVarDecl
//  DMP-NEXT:         ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//   DMP-NOT:   FunctionDecl
//       DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
//  DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//   PRT-LABEL: declBeforeDef_userStart
//  PRT-A-NEXT: #pragma acc routine
// PRT-AO-NEXT: // #pragma omp declare target
//  PRT-O-NEXT: #pragma omp declare target
// PRT-OA-NEXT: // #pragma acc routine
//    PRT-NEXT: void declBeforeDef_user(Result *Res) {
//  PRT-A-NEXT:   {{^ *}}#pragma acc routine seq
// PRT-AO-SAME:   {{^}} // discarded in OpenMP translation
//  PRT-A-SAME:   {{^$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc routine seq // discarded in OpenMP translation{{$}}
//    PRT-NEXT:   void declBeforeDef(Result *Res);
//    PRT-NEXT:   declBeforeDef(Res);
//    PRT-NEXT: }
// PRT-AO-NEXT: // #pragma omp end declare target
//  PRT-O-NEXT: #pragma omp end declare target
int declBeforeDef_userStart;
#pragma acc routine seq
void declBeforeDef_user(Result *Res) {
  #pragma acc routine seq
  void declBeforeDef(Result *Res);
  declBeforeDef(Res);
}

// DMP-LABEL: declBeforeDef_start
//  DMP-NEXT: FunctionDecl {{.*}} declBeforeDef 'void (Result *)'
//   DMP-NOT: FunctionDecl
//       DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
//  DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//   PRT-LABEL: declBeforeDef_start
//  PRT-A-NEXT: #pragma acc routine
// PRT-AO-NEXT: // #pragma omp declare target
//  PRT-O-NEXT: #pragma omp declare target
// PRT-OA-NEXT: // #pragma acc routine
//         PRT: void declBeforeDef(Result *Res) {
//         PRT: }{{$}}
// PRT-AO-NEXT: // #pragma omp end declare target
//  PRT-O-NEXT: #pragma omp end declare target
int declBeforeDef_start;
#pragma acc routine seq
void declBeforeDef(Result *Res) { WRITE_RESULT(Res); }

// DMP-LABEL: defBeforeDecl_start
//  DMP-NEXT: FunctionDecl {{.*}} defBeforeDecl 'void (Result *)'
//   DMP-NOT: FunctionDecl
//       DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
//  DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//   PRT-LABEL: defBeforeDecl_start
//  PRT-A-NEXT: #pragma acc routine
// PRT-AO-NEXT: // #pragma omp declare target
//  PRT-O-NEXT: #pragma omp declare target
// PRT-OA-NEXT: // #pragma acc routine
//         PRT: void defBeforeDecl(Result *Res) {
//         PRT: }{{$}}
// PRT-AO-NEXT: // #pragma omp end declare target
//  PRT-O-NEXT: #pragma omp end declare target
int defBeforeDecl_start;
#pragma acc routine seq
void defBeforeDecl(Result *Res) { WRITE_RESULT(Res); }

// DMP-LABEL: defBeforeDecl_userStart
//  DMP-NEXT: FunctionDecl {{.*}} defBeforeDecl_user 'void (Result *)'
//  DMP-NEXT:   ParmVarDecl
//  DMP-NEXT:   CompoundStmt
//  DMP-NEXT:     DeclStmt
//  DMP-NEXT:       FunctionDecl {{.*}} defBeforeDecl 'void (Result *)'
//  DMP-NEXT:         ParmVarDecl
//  DMP-NEXT:         ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//   DMP-NOT:   FunctionDecl
//       DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
//  DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//   PRT-LABEL: defBeforeDecl_userStart
//  PRT-A-NEXT: #pragma acc routine
// PRT-AO-NEXT: // #pragma omp declare target
//  PRT-O-NEXT: #pragma omp declare target
// PRT-OA-NEXT: // #pragma acc routine
//    PRT-NEXT: void defBeforeDecl_user(Result *Res) {
//  PRT-A-NEXT:   {{^ *}}#pragma acc routine seq
// PRT-AO-SAME:   {{^}} // discarded in OpenMP translation
//  PRT-A-SAME:   {{^$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc routine seq // discarded in OpenMP translation{{$}}
//    PRT-NEXT:   void defBeforeDecl(Result *Res);
//    PRT-NEXT:   defBeforeDecl(Res);
//    PRT-NEXT: }
// PRT-AO-NEXT: // #pragma omp end declare target
//  PRT-O-NEXT: #pragma omp end declare target
int defBeforeDecl_userStart;
#pragma acc routine seq
void defBeforeDecl_user(Result *Res) {
  #pragma acc routine seq
  void defBeforeDecl(Result *Res);
  defBeforeDecl(Res);
}

int main() {
  Result Res;

  //     EXE-LABEL: declBeforeDef start:
  //      EXE-NEXT:   declBeforeDef: host=1, not_host=0
  // EXE-HOST-NEXT:   declBeforeDef: host=1, not_host=0
  //  EXE-OFF-NEXT:   declBeforeDef: host=0, not_host=1
  printf("declBeforeDef start:\n");
  declBeforeDef(&Res); PRINT_RESULT(declBeforeDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  declBeforeDef(&Res); PRINT_RESULT(declBeforeDef, Res);

  //     EXE-LABEL: defBeforeDecl start:
  //      EXE-NEXT:   defBeforeDecl: host=1, not_host=0
  // EXE-HOST-NEXT:   defBeforeDecl: host=1, not_host=0
  //  EXE-OFF-NEXT:   defBeforeDecl: host=0, not_host=1
  printf("defBeforeDecl start:\n");
  defBeforeDecl(&Res); PRINT_RESULT(defBeforeDecl, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  defBeforeDecl(&Res); PRINT_RESULT(defBeforeDecl, Res);

  return 0;
}
