// Check that "acc routine" behaves correctly when placed in a function and
// possibly within an OpenACC construct.
//
// We vary the level of parallelism some among the cases to check that it
// doesn't affect the translation.  For "acc routine" within a function, that
// checks some of what routine-levels.c checks for "acc routine" at file scope.
//
// Below, we check with and without the function definition in the same
// compilation unit.  For "acc routine" within a function, that checks some of
// what routine-placement.c checks for "acc routine" at file scope.
//
// Implicit "acc routine" on a function prototype within a function is checked
// in routine-implicit.c.

// REDEFINE: %{dmp:fc:args} =                                                  \
// REDEFINE:   -implicit-check-not=ACCRoutineDeclAttr                          \
// REDEFINE:   -implicit-check-not=OMPDeclareTargetDeclAttr
// REDEFINE: %{prt:fc:args} =                                                  \
// REDEFINE:   -implicit-check-not='{{#pragma *acc *routine}}'                 \
// REDEFINE:   -implicit-check-not='{{#pragma *omp *declare *target}}'         \
// REDEFINE:   -implicit-check-not='{{#pragma *omp *end *declare *target}}'
// REDEFINE: %{exe:fc:args} =                                                  \
// REDEFINE:   -implicit-check-not='{{[^[:space:]]}}'

// Check case where definition appears in the same compilation unit and inherits
// the routine directive from within a previous function.
//
// REDEFINE: %{all:clang:args} = -DMODE=COMPILE_TOGETHER
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}
// REDEFINE: %{all:clang:args} =

// Check case where definition appears in a separate compilation unit and has
// its own routine directive.  Dump and print checks are unlikely to show
// anything new in this case.
//
// REDEFINE: %{exe:base-name} = main
// REDEFINE: %{exe:clang:args} = -DMODE=COMPILE_MAIN
// RUN: %{acc-check-exe-compile-c}
//
// REDEFINE: %{exe:base-name} = other
// REDEFINE: %{exe:clang:args} = -DMODE=COMPILE_OTHER
// RUN: %{acc-check-exe-compile-c}
//
// REDEFINE: %{exe:clang:args} = main.o other.o
// RUN: %{acc-check-exe-link}
// RUN: %{acc-check-exe-run}
// RUN: %{acc-check-exe-filecheck}

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

#define COMPILE_MAIN     1
#define COMPILE_OTHER    2
#define COMPILE_TOGETHER 3
#if MODE != COMPILE_MAIN && MODE != COMPILE_OTHER && MODE != COMPILE_TOGETHER
# error unknown MODE
# define MODE COMPILE_TOGETHER // for syntax highlighting
#endif

#if MODE == COMPILE_MAIN || MODE == COMPILE_TOGETHER

// DMP-LABEL: inRoutine_start
//  DMP-NEXT: FunctionDecl {{.*}} routine 'void (Result *)'
//  DMP-NEXT:   ParmVarDecl
//  DMP-NEXT:   CompoundStmt
//  DMP-NEXT:     DeclStmt
//  DMP-NEXT:       FunctionDecl {{.*}} inRoutine 'void (Result *)'
//  DMP-NEXT:         ParmVarDecl
//  DMP-NEXT:         ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//   DMP-NOT:   FunctionDecl
//       DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
//  DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//   PRT-LABEL: inRoutine_start
//  PRT-A-NEXT: #pragma acc routine
// PRT-AO-NEXT: // #pragma omp declare target
//  PRT-O-NEXT: #pragma omp declare target
// PRT-OA-NEXT: // #pragma acc routine
//    PRT-NEXT: void routine(Result *Res) {
//  PRT-A-NEXT:   {{^ *}}#pragma acc routine seq
// PRT-AO-SAME:   {{^}} // discarded in OpenMP translation
//  PRT-A-SAME:   {{^$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc routine seq // discarded in OpenMP translation{{$}}
//    PRT-NEXT:   void inRoutine(Result *);
//    PRT-NEXT:   inRoutine(Res);
//    PRT-NEXT: }
// PRT-AO-NEXT: // #pragma omp end declare target
//  PRT-O-NEXT: #pragma omp end declare target
int inRoutine_start;
#pragma acc routine seq
void routine(Result *Res) {
  #pragma acc routine seq
  void inRoutine(Result *);
  inRoutine(Res);
}

int main() {
  int dummy;
  Result Res;

  //     EXE-LABEL: inRoutine start:
  //      EXE-NEXT:   inRoutine: host=1, not_host=0
  // EXE-HOST-NEXT:   inRoutine: host=1, not_host=0
  //  EXE-OFF-NEXT:   inRoutine: host=0, not_host=1
  printf("inRoutine start:\n");
  routine(&Res); PRINT_RESULT(inRoutine, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  routine(&Res); PRINT_RESULT(inRoutine, Res);

  // DMP-LABEL: inFunc start:
  //  DMP-NEXT: DeclStmt
  //  DMP-NEXT:   FunctionDecl {{.*}} inFunc 'void (Result *)'
  //  DMP-NEXT:     ParmVarDecl
  //  DMP-NEXT:     ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
  //
  //   PRT-LABEL: inFunc start:
  //  PRT-A-NEXT: {{^ *}}#pragma acc routine seq
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: void inFunc(Result *);
  //
  //     EXE-LABEL: inFunc start:
  //      EXE-NEXT:   inFunc: host=1, not_host=0
  // EXE-HOST-NEXT:   inFunc: host=1, not_host=0
  //  EXE-OFF-NEXT:   inFunc: host=0, not_host=1
  printf("inFunc start:\n");
  #pragma acc routine seq
  void inFunc(Result *);
  inFunc(&Res); PRINT_RESULT(inFunc, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  inFunc(&Res); PRINT_RESULT(inFunc, Res);

  // DMP-LABEL: inData start:
  //  DMP-NEXT: DeclStmt
  //  DMP-NEXT:   FunctionDecl {{.*}} inData 'void (Result *)'
  //  DMP-NEXT:     ParmVarDecl
  //  DMP-NEXT:     ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
  //
  // The AST for the OpenMP translation of acc data has this too.
  // DMP: ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
  //
  //   PRT-LABEL: inData start:
  //  PRT-A-NEXT: {{^ *}}#pragma acc routine seq
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: void inData(Result *);
  //
  //     EXE-LABEL: inData start:
  //      EXE-NEXT:   inData: host=1, not_host=0
  // EXE-HOST-NEXT:   inData: host=1, not_host=0
  //  EXE-OFF-NEXT:   inData: host=0, not_host=1
  #pragma acc data copy(dummy)
  {
    printf("inData start:\n");
    #pragma acc routine seq
    void inData(Result *);
    inData(&Res); PRINT_RESULT(inData, Res);
    #pragma acc parallel num_gangs(1) copyout(Res)
    inData(&Res); PRINT_RESULT(inData, Res);
  }

  // DMP-LABEL: inPar start:
  //       DMP: DeclStmt
  //  DMP-NEXT: FunctionDecl {{.*}} inPar 'void (Result *)'
  //  DMP-NEXT:   ParmVarDecl
  //  DMP-NEXT:   ACCRoutineDeclAttr {{.*}}> Gang OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
  //
  // The AST for the OpenMP translation of acc data has this multiple times due
  // to captures.
  // DMP-COUNT-4: ACCRoutineDeclAttr {{.*}}> Gang OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
  //
  //   PRT-LABEL: inPar start:
  //         PRT: {
  //  PRT-A-NEXT:   {{^ *}}#pragma acc routine gang
  // PRT-AO-SAME:   {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME:   {{^$}}
  // PRT-OA-NEXT:   {{^ *}}// #pragma acc routine gang // discarded in OpenMP translation{{$}}
  //    PRT-NEXT:   void inPar(Result *);
  //         PRT: }
  //
  //     EXE-LABEL: inPar start:
  // EXE-HOST-NEXT:   inPar: host=1, not_host=0
  //  EXE-OFF-NEXT:   inPar: host=0, not_host=1
  printf("inPar start:\n");
  #pragma acc parallel num_gangs(1) copyout(Res)
  {
    #pragma acc routine gang
    void inPar(Result *);
    inPar(&Res);
  }
  PRINT_RESULT(inPar, Res);

  // DMP-LABEL: inLoop start:
  //       DMP: ForStmt
  //       DMP:   CompoundStmt
  //       DMP:     DeclStmt
  //  DMP-NEXT:       FunctionDecl {{.*}} inLoop 'void (Result *)'
  //  DMP-NEXT:         ParmVarDecl
  //  DMP-NEXT:         ACCRoutineDeclAttr {{.*}}> Worker OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
  //
  // The AST for the OpenMP translation of acc data has this multiple times due
  // to captures.
  // DMP-COUNT-9: ACCRoutineDeclAttr {{.*}}> Worker OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
  //
  //   PRT-LABEL: inLoop start:
  //         PRT: for ({{.*}}) {
  //  PRT-A-NEXT:   {{^ *}}#pragma acc routine worker
  // PRT-AO-SAME:   {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME:   {{^$}}
  // PRT-OA-NEXT:   {{^ *}}// #pragma acc routine worker // discarded in OpenMP translation{{$}}
  //    PRT-NEXT:   void inLoop(Result *);
  //         PRT: }
  //
  //     EXE-LABEL: inLoop start:
  // EXE-HOST-NEXT:   inLoop: host=1, not_host=0
  //  EXE-OFF-NEXT:   inLoop: host=0, not_host=1
  printf("inLoop start:\n");
  #pragma acc parallel num_gangs(1) copyout(Res)
  {
    #pragma acc loop gang
    for (int i = 0; i < 1; ++i) {
      #pragma acc routine worker
      void inLoop(Result *);
      inLoop(&Res);
    }
  }
  PRINT_RESULT(inLoop, Res);

  // DMP-LABEL: inParLoop start:
  //       DMP: ForStmt
  //       DMP:   CompoundStmt
  //       DMP:     DeclStmt
  //  DMP-NEXT:       FunctionDecl {{.*}} inParLoop 'void (Result *)'
  //  DMP-NEXT:         ParmVarDecl
  //  DMP-NEXT:         ACCRoutineDeclAttr {{.*}}> Vector OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
  //
  // The AST for the OpenMP translation of acc data has this multiple times due
  // to captures.
  // DMP-COUNT-10: ACCRoutineDeclAttr {{.*}}> Vector OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
  //
  //   PRT-LABEL: inParLoop start:
  //         PRT: for ({{.*}}) {
  //  PRT-A-NEXT:   {{^ *}}#pragma acc routine vector
  // PRT-AO-SAME:   {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME:   {{^$}}
  // PRT-OA-NEXT:   {{^ *}}// #pragma acc routine vector // discarded in OpenMP translation{{$}}
  //    PRT-NEXT:   void inParLoop(Result *);
  //         PRT: }
  //
  //     EXE-LABEL: inParLoop start:
  // EXE-HOST-NEXT:   inParLoop: host=1, not_host=0
  //  EXE-OFF-NEXT:   inParLoop: host=0, not_host=1
  printf("inParLoop start:\n");
  #pragma acc parallel loop num_gangs(1) gang copyout(Res)
  for (int i = 0; i < 1; ++i) {
    #pragma acc routine vector
    void inParLoop(Result *);
    inParLoop(&Res);
  }
  PRINT_RESULT(inParLoop, Res);

  return 0;
}

#endif

// Below, we only run print and dump checks when MODE == COMPILE_TOGETHER so we
// can check how the inherited attributes are handled.
#if MODE == COMPILE_OTHER || MODE == COMPILE_TOGETHER

//     DMP: FunctionDecl {{.*}} inRoutine 'void (Result *)'
// DMP-NOT: FunctionDecl
//     DMP:   ACCRoutineDeclAttr {{.*}}> Inherited Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//
// PRT-SRC: #pragma acc routine seq
//     PRT: void inRoutine(Result *Res) {
//     PRT: }{{$}}
#if MODE == COMPILE_OTHER
#pragma acc routine seq
#endif
void inRoutine(Result *Res) { WRITE_RESULT(Res); }

//     DMP: FunctionDecl {{.*}} inFunc 'void (Result *)'
// DMP-NOT: FunctionDecl
//     DMP:   ACCRoutineDeclAttr {{.*}}> Inherited Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//
// PRT-SRC: #pragma acc routine seq
//     PRT: void inFunc(Result *Res) {
//     PRT: }{{$}}
#if MODE == COMPILE_OTHER
#pragma acc routine seq
#endif
void inFunc(Result *Res) { WRITE_RESULT(Res); }

//     DMP: FunctionDecl {{.*}} inData 'void (Result *)'
// DMP-NOT: FunctionDecl
//     DMP:   ACCRoutineDeclAttr {{.*}}> Inherited Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//
// PRT-SRC: #pragma acc routine seq
//     PRT: void inData(Result *Res) {
//     PRT: }{{$}}
#if MODE == COMPILE_OTHER
#pragma acc routine seq
#endif
void inData(Result *Res) { WRITE_RESULT(Res); }

//     DMP: FunctionDecl {{.*}} inPar 'void (Result *)'
// DMP-NOT: FunctionDecl
//     DMP:   ACCRoutineDeclAttr {{.*}}> Inherited Gang OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//
// PRT-SRC: #pragma acc routine gang
//     PRT: void inPar(Result *Res) {
//     PRT: }{{$}}
#if MODE == COMPILE_OTHER
#pragma acc routine gang
#endif
void inPar(Result *Res) { WRITE_RESULT(Res); }

//     DMP: FunctionDecl {{.*}} inLoop 'void (Result *)'
// DMP-NOT: FunctionDecl
//     DMP:   ACCRoutineDeclAttr {{.*}}> Inherited Worker OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//
// PRT-SRC: #pragma acc routine worker
//     PRT: void inLoop(Result *Res) {
//     PRT: }{{$}}
#if MODE == COMPILE_OTHER
#pragma acc routine worker
#endif
void inLoop(Result *Res) { WRITE_RESULT(Res); }

//     DMP: FunctionDecl {{.*}} inParLoop 'void (Result *)'
// DMP-NOT: FunctionDecl
//     DMP:   ACCRoutineDeclAttr {{.*}}> Inherited Vector OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//
// PRT-SRC: #pragma acc routine vector
//     PRT: void inParLoop(Result *Res) {
//     PRT: }{{$}}
#if MODE == COMPILE_OTHER
#pragma acc routine vector
#endif
void inParLoop(Result *Res) { WRITE_RESULT(Res); }

#endif
