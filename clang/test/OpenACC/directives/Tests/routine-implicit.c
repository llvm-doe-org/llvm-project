// Check implicit "acc routine seq".
//
// Implicit routine directives are seen only in dump checks.  However, we also
// make sure there's no misprint or miscompilation when they're present.  We
// specifically avoid the clutter of checking that explicit routine directives
// dump and print correctly as that's checked in other routine-*.c.

// REDEFINE: %{exe:fc:args-stable} = -strict-whitespace

// REDEFINE: %{all:clang:args} = -DCOMPILE_OTHER
// REDEFINE: %{exe:base-name} = other
// RUN: %{acc-check-exe-compile-c}
// REDEFINE: %{exe:base-name} = main

// REDEFINE: %{all:clang:args} = -DUSE_CALL
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe-compile-c}
// REDEFINE: %{all:clang:args} = main.o other.o
// RUN: %{acc-check-exe-link}
// RUN: %{acc-check-exe-run}
// RUN: %{acc-check-exe-filecheck}

// REDEFINE: %{all:clang:args} = -DUSE_ADDR
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe-compile-c}
// REDEFINE: %{all:clang:args} = main.o other.o
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
    printf("%s: host=%d, not_host=%d\n", #Fn, Res.Host, Res.NotHost)

#ifndef COMPILE_OTHER

#if USE_CALL
# define USE1(Fn, ARG1)                                                        \
  do {                                                                         \
    Fn(ARG1);                                                                  \
  } while (0)
# define USE2(Fn, ARG1, ARG2)                                                  \
  do {                                                                         \
    Fn(ARG1, ARG2);                                                            \
  } while (0)
# define USE5(Fn, ARG1, ARG2, ARG3, ARG4, ARG5)                                \
  do {                                                                         \
    Fn(ARG1, ARG2, ARG3, ARG4, ARG5);                                          \
  } while (0)
#elif USE_ADDR
# define USE1(Fn, ARG1)                                                        \
  do {                                                                         \
    void (*fp)(Result *) = Fn;                                                 \
    fp(ARG1);                                                                  \
  } while (0)
# define USE2(Fn, ARG1, ARG2)                                                  \
  do {                                                                         \
    void (*fp)(Result *, Result *) = Fn;                                       \
    fp(ARG1, ARG2);                                                            \
  } while (0)
# define USE5(Fn, ARG1, ARG2, ARG3, ARG4, ARG5)                                \
  do {                                                                         \
    void (*fp)(Result *, Result *, Result *, Result *, Result *) = Fn;         \
    fp(ARG1, ARG2, ARG3, ARG4, ARG5);                                          \
  } while (0)
#else
# error no known USE_ macro is defined
#endif

// EXE-NOT: {{.}}

// This is just so we can use PRT*-NEXT and EXE-NEXT consistently from now on.
// PRT: void start() {
// PRT: }{{$}}
// EXE: start
void start() { printf("start\n"); }

//------------------------------------------------------------------------------
// Host use doesn't imply routine directive.
//------------------------------------------------------------------------------

//     DMP: FunctionDecl {{.*}} hostUseBeforeDef 'void (Result *)'
// DMP-NOT:   ACCRoutineDeclAttr
// DMP-NOT:   OMPDeclareTargetDeclAttr
//     DMP: FunctionDecl {{.*}} hostUseBeforeDef_run 'void ()'
//     DMP: FunctionDecl {{.*}} hostUseBeforeDef 'void (Result *)'
// DMP-NOT:   ACCRoutineDeclAttr
// DMP-NOT:   OMPDeclareTargetDeclAttr
//
// PRT-NEXT: void hostUseBeforeDef(Result *Res);
// PRT-NEXT: void hostUseBeforeDef_run() {
//      PRT: }{{$}}
// PRT-NEXT: void hostUseBeforeDef(Result *Res) {
//      PRT: }{{$}}
//
// EXE-NEXT: hostUseBeforeDef: host=1, not_host=0
// EXE-NEXT: hostUseBeforeDef: host=1, not_host=0
void hostUseBeforeDef(Result *Res);
void hostUseBeforeDef_run() {
  Result Res;
  USE1(hostUseBeforeDef, &Res);
  PRINT_RESULT(hostUseBeforeDef, Res);
  int dummy;
  #pragma acc data copy(dummy)
  USE1(hostUseBeforeDef, &Res);
  PRINT_RESULT(hostUseBeforeDef, Res);
}
void hostUseBeforeDef(Result *Res) { WRITE_RESULT(Res); }

//     DMP: FunctionDecl {{.*}} hostUseAfterDef 'void (Result *)'
// DMP-NOT:   ACCRoutineDeclAttr
// DMP-NOT:   OMPDeclareTargetDeclAttr
//     DMP: FunctionDecl {{.*}} hostUseAfterDef_run 'void ()'
//
// PRT-NEXT: void hostUseAfterDef(Result *Res) {
//      PRT: }{{$}}
// PRT-NEXT: void hostUseAfterDef_run() {
//      PRT: }{{$}}
//
// EXE-NEXT: hostUseAfterDef: host=1, not_host=0
// EXE-NEXT: hostUseAfterDef: host=1, not_host=0
void hostUseAfterDef(Result *Res) { WRITE_RESULT(Res); }
void hostUseAfterDef_run() {
  Result Res;
  USE1(hostUseAfterDef, &Res);
  PRINT_RESULT(hostUseAfterDef, Res);
  int dummy;
  #pragma acc data copy(dummy)
  USE1(hostUseAfterDef, &Res);
  PRINT_RESULT(hostUseAfterDef, Res);
}

//     DMP: FunctionDecl {{.*}} hostUseNoDef 'void (Result *)'
// DMP-NOT:   ACCRoutineDeclAttr
// DMP-NOT:   OMPDeclareTargetDeclAttr
//     DMP: FunctionDecl {{.*}} hostUseNoDef_run 'void ()'
//
// PRT-NEXT: void hostUseNoDef(Result *Res);
// PRT-NEXT: void hostUseNoDef_run() {
//      PRT: }{{$}}
//
// EXE-NEXT: hostUseNoDef: host=1, not_host=0
// EXE-NEXT: hostUseNoDef: host=1, not_host=0
void hostUseNoDef(Result *Res);
void hostUseNoDef_run() {
  Result Res;
  USE1(hostUseNoDef, &Res);
  PRINT_RESULT(hostUseNoDef, Res);
  int dummy;
  #pragma acc data copy(dummy)
  USE1(hostUseNoDef, &Res);
  PRINT_RESULT(hostUseNoDef, Res);
}

//     DMP: FunctionDecl {{.*}} hostUseRecur 'void (Result *)'
// DMP-NOT:   ACCRoutineDeclAttr
// DMP-NOT:   OMPDeclareTargetDeclAttr
//     DMP: FunctionDecl {{.*}} hostUseRecur_run 'void ()'
//
// PRT-NEXT: void hostUseRecur(Result *Res) {
//      PRT: }{{$}}
// PRT-NEXT: void hostUseRecur_run() {
//      PRT: }{{$}}
//
// EXE-NEXT: hostUseRecur: host=1, not_host=0
// EXE-NEXT: hostUseRecur: host=1, not_host=0
void hostUseRecur(Result *Res) {
  // Avoid infinite recursion during execution.
  if (!Res)
    return;
  WRITE_RESULT(Res);
  USE1(hostUseRecur, 0);
  int dummy;
  #pragma acc data copy(dummy)
  USE1(hostUseRecur, 0);
}
void hostUseRecur_run() {
  Result Res;
  USE1(hostUseRecur, &Res);
  PRINT_RESULT(hostUseRecur, Res);
  int dummy;
  #pragma acc data copy(dummy)
  USE1(hostUseRecur, &Res);
  PRINT_RESULT(hostUseRecur, Res);
}

//------------------------------------------------------------------------------
// Parallel construct use implies routine directive.
//------------------------------------------------------------------------------

//     DMP: FunctionDecl {{.*}} parUseBeforeDef 'void (Result *)'
// DMP-NOT: FunctionDecl
//     DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
// DMP-NOT:   OMPDeclareTargetDeclAttr
//     DMP: FunctionDecl {{.*}} parUseBeforeDef_run 'void ()'
//     DMP: FunctionDecl {{.*}} parUseBeforeDef 'void (Result *)'
// DMP-NOT: FunctionDecl
//     DMP:   ACCRoutineDeclAttr {{.*}}> Inherited Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
// DMP-NOT:   OMPDeclareTargetDeclAttr
//
// PRT-NEXT: void parUseBeforeDef(Result *Res);
// PRT-NEXT: void parUseBeforeDef_run() {
//      PRT: }{{$}}
// PRT-NEXT: void parUseBeforeDef(Result *Res) {
//      PRT: }{{$}}
//
// EXE-HOST-NEXT: parUseBeforeDef: host=1, not_host=0
//  EXE-OFF-NEXT: parUseBeforeDef: host=0, not_host=1
void parUseBeforeDef(Result *Res);
void parUseBeforeDef_run() {
  Result Res;
  #pragma acc parallel num_gangs(1) copyout(Res)
  USE1(parUseBeforeDef, &Res);
  PRINT_RESULT(parUseBeforeDef, Res);
}
void parUseBeforeDef(Result *Res) { WRITE_RESULT(Res); }

//     DMP: FunctionDecl {{.*}} parUseOfLocalBeforeDef_run 'void ()'
//     DMP:   FunctionDecl {{.*}} parUseOfLocalBeforeDef 'void (Result *)'
// DMP-NOT:   FunctionDecl
//     DMP:     ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
// DMP-NOT:     OMPDeclareTargetDeclAttr
//     DMP: FunctionDecl {{.*}} parUseOfLocalBeforeDef 'void (Result *)'
// DMP-NOT: FunctionDecl
//     DMP:   ACCRoutineDeclAttr {{.*}}> Inherited Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
// DMP-NOT:   OMPDeclareTargetDeclAttr
//
// PRT-NEXT: void parUseOfLocalBeforeDef_run() {
// PRT-NEXT:   void parUseOfLocalBeforeDef(Result *Res);
//      PRT: }{{$}}
// PRT-NEXT: void parUseOfLocalBeforeDef(Result *Res) {
//      PRT: }{{$}}
//
// EXE-HOST-NEXT: parUseOfLocalBeforeDef: host=1, not_host=0
//  EXE-OFF-NEXT: parUseOfLocalBeforeDef: host=0, not_host=1
void parUseOfLocalBeforeDef_run() {
  void parUseOfLocalBeforeDef(Result *Res);
  Result Res;
  #pragma acc parallel num_gangs(1) copyout(Res)
  USE1(parUseOfLocalBeforeDef, &Res);
  PRINT_RESULT(parUseOfLocalBeforeDef, Res);
}
void parUseOfLocalBeforeDef(Result *Res) { WRITE_RESULT(Res); }

//     DMP: FunctionDecl {{.*}} parUseAfterDef 'void (Result *)'
// DMP-NOT: FunctionDecl
//     DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
// DMP-NOT:   OMPDeclareTargetDeclAttr
//     DMP: FunctionDecl {{.*}} parUseAfterDef_run 'void ()'
//
// PRT-NEXT: void parUseAfterDef(Result *Res) {
//      PRT: }{{$}}
// PRT-NEXT: void parUseAfterDef_run() {
//      PRT: }{{$}}
//
// EXE-HOST-NEXT: parUseAfterDef: host=1, not_host=0
//  EXE-OFF-NEXT: parUseAfterDef: host=0, not_host=1
void parUseAfterDef(Result *Res) { WRITE_RESULT(Res); }
void parUseAfterDef_run() {
  Result Res;
  #pragma acc parallel num_gangs(1) copyout(Res)
  USE1(parUseAfterDef, &Res);
  PRINT_RESULT(parUseAfterDef, Res);
}

//     DMP: FunctionDecl {{.*}} parUseOfLocalAfterDef 'void (Result *)'
// DMP-NOT:   ACCRoutineDeclAttr
// DMP-NOT:   OMPDeclareTargetDeclAttr
//     DMP: FunctionDecl {{.*}} parUseOfLocalAfterDef_run 'void ()'
//     DMP:   FunctionDecl {{.*}} parUseOfLocalAfterDef 'void (Result *)'
// DMP-NOT:   FunctionDecl
//     DMP:     ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
// DMP-NOT:     OMPDeclareTargetDeclAttr
//
// PRT-NEXT: void parUseOfLocalAfterDef(Result *Res) {
//      PRT: }{{$}}
// PRT-NEXT: void parUseOfLocalAfterDef_run() {
// PRT-NEXT:   void parUseOfLocalAfterDef(Result *Res);
//      PRT: }{{$}}
//
// EXE-HOST-NEXT: parUseOfLocalAfterDef: host=1, not_host=0
//  EXE-OFF-NEXT: parUseOfLocalAfterDef: host=0, not_host=1
void parUseOfLocalAfterDef(Result *Res) { WRITE_RESULT(Res); }
void parUseOfLocalAfterDef_run() {
  void parUseOfLocalAfterDef(Result *Res);
  Result Res;
  #pragma acc parallel num_gangs(1) copyout(Res)
  USE1(parUseOfLocalAfterDef, &Res);
  PRINT_RESULT(parUseOfLocalAfterDef, Res);
}

//     DMP: FunctionDecl {{.*}} parUseNoDef 'void (Result *)'
// DMP-NOT: FunctionDecl
//     DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
// DMP-NOT:   OMPDeclareTargetDeclAttr
//     DMP: FunctionDecl {{.*}} parUseNoDef_run 'void ()'
//
// PRT-NEXT: void parUseNoDef(Result *Res);
// PRT-NEXT: void parUseNoDef_run() {
//      PRT: }{{$}}
//
// EXE-HOST-NEXT: parUseNoDef: host=1, not_host=0
//  EXE-OFF-NEXT: parUseNoDef: host=0, not_host=1
void parUseNoDef(Result *Res);
void parUseNoDef_run() {
  Result Res;
  #pragma acc parallel num_gangs(1) copyout(Res)
  USE1(parUseNoDef, &Res);
  PRINT_RESULT(parUseNoDef, Res);
}

//     DMP: FunctionDecl {{.*}} parUseOfLocalNoDef_run 'void ()'
//     DMP:   FunctionDecl {{.*}} parUseOfLocalNoDef 'void (Result *)'
// DMP-NOT:   FunctionDecl
//     DMP:     ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
// DMP-NOT:     OMPDeclareTargetDeclAttr
//
// PRT-NEXT: void parUseOfLocalNoDef_run() {
// PRT-NEXT:   void parUseOfLocalNoDef(Result *Res);
//      PRT: }{{$}}
//
// EXE-HOST-NEXT: parUseOfLocalNoDef: host=1, not_host=0
//  EXE-OFF-NEXT: parUseOfLocalNoDef: host=0, not_host=1
void parUseOfLocalNoDef_run() {
  void parUseOfLocalNoDef(Result *Res);
  Result Res;
  #pragma acc parallel num_gangs(1) copyout(Res)
  USE1(parUseOfLocalNoDef, &Res);
  PRINT_RESULT(parUseOfLocalNoDef, Res);
}

//     DMP: FunctionDecl {{.*}} parUsesMultiple_fn1 'void (Result *)'
// DMP-NOT: FunctionDecl
//     DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
// DMP-NOT:   OMPDeclareTargetDeclAttr
//     DMP: FunctionDecl {{.*}} parUsesMultiple_fn2 'void (Result *)'
// DMP-NOT: FunctionDecl
//     DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
// DMP-NOT:   OMPDeclareTargetDeclAttr
//     DMP: FunctionDecl {{.*}} parUsesMultiple_run 'void ()'
//     DMP: FunctionDecl {{.*}} parUsesMultiple_fn2 'void (Result *)'
// DMP-NOT: FunctionDecl
//     DMP:   ACCRoutineDeclAttr {{.*}}> Inherited Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
// DMP-NOT:   OMPDeclareTargetDeclAttr
//
// PRT-NEXT: void parUsesMultiple_fn1(Result *Res) {
//      PRT: }{{$}}
// PRT-NEXT: void parUsesMultiple_fn2(Result *Res);
// PRT-NEXT: void parUsesMultiple_run() {
//      PRT:   {{[{]$}}
//      PRT:   {{[}]$}}
//      PRT: }{{$}}
// PRT-NEXT: void parUsesMultiple_fn2(Result *Res) {
//      PRT: }{{$}}
//
// EXE-HOST-NEXT: parUsesMultiple_fn1: host=1, not_host=0
// EXE-HOST-NEXT: parUsesMultiple_fn2: host=1, not_host=0
//  EXE-OFF-NEXT: parUsesMultiple_fn1: host=0, not_host=1
//  EXE-OFF-NEXT: parUsesMultiple_fn2: host=0, not_host=1
void parUsesMultiple_fn1(Result *Res) { WRITE_RESULT(Res); }
void parUsesMultiple_fn2(Result *Res);
void parUsesMultiple_run() {
  Result Res1, Res2;
  #pragma acc parallel num_gangs(1) copyout(Res1, Res2)
  {
    USE1(parUsesMultiple_fn1, &Res1);
    USE1(parUsesMultiple_fn2, &Res2);
  }
  PRINT_RESULT(parUsesMultiple_fn1, Res1);
  PRINT_RESULT(parUsesMultiple_fn2, Res2);
}
void parUsesMultiple_fn2(Result *Res) { WRITE_RESULT(Res); }

//------------------------------------------------------------------------------
// Use from offload function with explicit routine directive implies routine
// directive.
//------------------------------------------------------------------------------

//      DMP: FunctionDecl {{.*}} expOffFnUseAfterDef 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//  DMP-NOT:   OMPDeclareTargetDeclAttr
//      DMP: FunctionDecl {{.*}} expOffFnUseAfterDef_use 'void (Result *)'
//      DMP: FunctionDecl {{.*}} expOffFnUseAfterDef_run 'void ()'
//
// PRT-NEXT: void expOffFnUseAfterDef(Result *Res) {
//      PRT: }{{$}}
//      PRT: void expOffFnUseAfterDef_use(Result *Res) {
//      PRT: }{{$}}
//      PRT: void expOffFnUseAfterDef_run() {
//      PRT: }{{$}}
//
//      EXE-NEXT: expOffFnUseAfterDef: host=1, not_host=0
// EXE-HOST-NEXT: expOffFnUseAfterDef: host=1, not_host=0
//  EXE-OFF-NEXT: expOffFnUseAfterDef: host=0, not_host=1
void expOffFnUseAfterDef(Result *Res) { WRITE_RESULT(Res); }
#pragma acc routine seq
void expOffFnUseAfterDef_use(Result *Res) {
  USE1(expOffFnUseAfterDef, Res);
}
void expOffFnUseAfterDef_run() {
  Result Res;
  expOffFnUseAfterDef_use(&Res);
  PRINT_RESULT(expOffFnUseAfterDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  expOffFnUseAfterDef_use(&Res);
  PRINT_RESULT(expOffFnUseAfterDef, Res);
}

// FIXME: Strangely, this case works in traditional compilation mode but not in
// source-to-source mode followed by OpenMP compilation.  The generated OpenMP
// looks as we expect, but somehow Clang fails to see the implicit
// 'omp declare target' at the definition of expOffFnUseOfLocalAfterDef.
// Hopefully this usage is obscure enough in practice that we don't really need
// to fix it.
//
//      DMP: FunctionDecl {{.*}} expOffFnUseOfLocalAfterDef 'void (Result *)'
//  DMP-NOT:   ACCRoutineDeclAttr
//  DMP-NOT:   OMPDeclareTargetDeclAttr
//      DMP: FunctionDecl {{.*}} expOffFnUseOfLocalAfterDef_use 'void (Result *)'
//      DMP:   FunctionDecl {{.*}} expOffFnUseOfLocalAfterDef 'void (Result *)'
//  DMP-NOT:   FunctionDecl
//      DMP:     ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//  DMP-NOT:     OMPDeclareTargetDeclAttr
//      DMP:   ACCRoutineDeclAttr
// DMP-NEXT:   OMPDeclareTargetDeclAttr
//      DMP: FunctionDecl {{.*}} expOffFnUseOfLocalAfterDef_run 'void ()'
//
// PRT-SRC-NEXT: #if !EXE_S2S_PRT
//     PRT-NEXT: void expOffFnUseOfLocalAfterDef(Result *Res) {
//          PRT: }{{$}}
//          PRT: void expOffFnUseOfLocalAfterDef_use(Result *Res) {
//     PRT-NEXT:   void expOffFnUseOfLocalAfterDef(Result *Res);
//          PRT: }{{$}}
//          PRT: void expOffFnUseOfLocalAfterDef_run() {
//          PRT: }{{$}}
// PRT-SRC-NEXT: #else
//      PRT-SRC: #endif
//
//      EXE-NEXT: expOffFnUseOfLocalAfterDef: host=1, not_host=0
// EXE-HOST-NEXT: expOffFnUseOfLocalAfterDef: host=1, not_host=0
//  EXE-OFF-NEXT: expOffFnUseOfLocalAfterDef: host=0, not_host=1
#if !EXE_S2S_PRT
void expOffFnUseOfLocalAfterDef(Result *Res) { WRITE_RESULT(Res); }
#pragma acc routine seq
void expOffFnUseOfLocalAfterDef_use(Result *Res) {
  void expOffFnUseOfLocalAfterDef(Result *Res);
  USE1(expOffFnUseOfLocalAfterDef, Res);
}
void expOffFnUseOfLocalAfterDef_run() {
  Result Res;
  expOffFnUseOfLocalAfterDef_use(&Res);
  PRINT_RESULT(expOffFnUseOfLocalAfterDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  expOffFnUseOfLocalAfterDef_use(&Res);
  PRINT_RESULT(expOffFnUseOfLocalAfterDef, Res);
}
#else
void expOffFnUseOfLocalAfterDef_run() {
  Result Res;
  Res.Host = 1;
  Res.NotHost = 0;
  PRINT_RESULT(expOffFnUseOfLocalAfterDef, Res);
# if !TGT_HOST
  Res.Host = 0;
  Res.NotHost = 1;
# endif
  PRINT_RESULT(expOffFnUseOfLocalAfterDef, Res);
}
#endif

//      DMP: FunctionDecl {{.*}} expOffFnUseBeforeDef 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//  DMP-NOT:   OMPDeclareTargetDeclAttr
//      DMP: FunctionDecl {{.*}} expOffFnUseBeforeDef_use 'void (Result *)'
//      DMP: FunctionDecl {{.*}} expOffFnUseBeforeDef_run 'void ()'
//      DMP: FunctionDecl {{.*}} expOffFnUseBeforeDef 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Inherited Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//  DMP-NOT:   OMPDeclareTargetDeclAttr
//
// PRT-NEXT: void expOffFnUseBeforeDef(Result *Res);
//      PRT: void expOffFnUseBeforeDef_use(Result *Res) {
//      PRT: }{{$}}
//      PRT: void expOffFnUseBeforeDef_run() {
//      PRT: }{{$}}
// PRT-NEXT: void expOffFnUseBeforeDef(Result *Res) {
//      PRT: }{{$}}
//
//      EXE-NEXT: expOffFnUseBeforeDef: host=1, not_host=0
// EXE-HOST-NEXT: expOffFnUseBeforeDef: host=1, not_host=0
//  EXE-OFF-NEXT: expOffFnUseBeforeDef: host=0, not_host=1
void expOffFnUseBeforeDef(Result *Res);
#pragma acc routine seq
void expOffFnUseBeforeDef_use(Result *Res) {
  USE1(expOffFnUseBeforeDef, Res);
}
void expOffFnUseBeforeDef_run() {
  Result Res;
  expOffFnUseBeforeDef_use(&Res);
  PRINT_RESULT(expOffFnUseBeforeDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  expOffFnUseBeforeDef_use(&Res);
  PRINT_RESULT(expOffFnUseBeforeDef, Res);
}
void expOffFnUseBeforeDef(Result *Res) { WRITE_RESULT(Res); }

//      DMP: FunctionDecl {{.*}} expOffFnUseOfLocalBeforeDef_use 'void (Result *)'
//      DMP:   FunctionDecl {{.*}} expOffFnUseOfLocalBeforeDef 'void (Result *)'
//  DMP-NOT:   FunctionDecl
//      DMP:     ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//  DMP-NOT:     OMPDeclareTargetDeclAttr
//      DMP:   ACCRoutineDeclAttr
// DMP-NEXT:   OMPDeclareTargetDeclAttr
//      DMP: FunctionDecl {{.*}} expOffFnUseOfLocalBeforeDef_run 'void ()'
//      DMP: FunctionDecl {{.*}} expOffFnUseOfLocalBeforeDef 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Inherited Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//  DMP-NOT:   OMPDeclareTargetDeclAttr
//
//      PRT: void expOffFnUseOfLocalBeforeDef_use(Result *Res) {
// PRT-NEXT:   void expOffFnUseOfLocalBeforeDef(Result *Res);
//      PRT: }{{$}}
//      PRT: void expOffFnUseOfLocalBeforeDef_run() {
//      PRT: }{{$}}
// PRT-NEXT: void expOffFnUseOfLocalBeforeDef(Result *Res) {
//      PRT: }{{$}}
//
//      EXE-NEXT: expOffFnUseOfLocalBeforeDef: host=1, not_host=0
// EXE-HOST-NEXT: expOffFnUseOfLocalBeforeDef: host=1, not_host=0
//  EXE-OFF-NEXT: expOffFnUseOfLocalBeforeDef: host=0, not_host=1
#pragma acc routine seq
void expOffFnUseOfLocalBeforeDef_use(Result *Res) {
  void expOffFnUseOfLocalBeforeDef(Result *Res);
  USE1(expOffFnUseOfLocalBeforeDef, Res);
}
void expOffFnUseOfLocalBeforeDef_run() {
  Result Res;
  expOffFnUseOfLocalBeforeDef_use(&Res);
  PRINT_RESULT(expOffFnUseOfLocalBeforeDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  expOffFnUseOfLocalBeforeDef_use(&Res);
  PRINT_RESULT(expOffFnUseOfLocalBeforeDef, Res);
}
void expOffFnUseOfLocalBeforeDef(Result *Res) { WRITE_RESULT(Res); }

//      DMP: FunctionDecl {{.*}} expOffFnUseNoDef 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//  DMP-NOT:   OMPDeclareTargetDeclAttr
//      DMP: FunctionDecl {{.*}} expOffFnUseNoDef_use 'void (Result *)'
//      DMP: FunctionDecl {{.*}} expOffFnUseNoDef_run 'void ()'
//
// PRT-NEXT: void expOffFnUseNoDef(Result *Res);
//      PRT: void expOffFnUseNoDef_use(Result *Res) {
//      PRT: }{{$}}
//      PRT: void expOffFnUseNoDef_run() {
//      PRT: }{{$}}
//
//      EXE-NEXT: expOffFnUseNoDef: host=1, not_host=0
// EXE-HOST-NEXT: expOffFnUseNoDef: host=1, not_host=0
//  EXE-OFF-NEXT: expOffFnUseNoDef: host=0, not_host=1
void expOffFnUseNoDef(Result *Res);
#pragma acc routine seq
void expOffFnUseNoDef_use(Result *Res) {
  USE1(expOffFnUseNoDef, Res);
}
void expOffFnUseNoDef_run() {
  Result Res;
  expOffFnUseNoDef_use(&Res);
  PRINT_RESULT(expOffFnUseNoDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  expOffFnUseNoDef_use(&Res);
  PRINT_RESULT(expOffFnUseNoDef, Res);
}

//      DMP: FunctionDecl {{.*}} expOffFnUseOfLocalNoDef_use 'void (Result *)'
//      DMP:   FunctionDecl {{.*}} expOffFnUseOfLocalNoDef 'void (Result *)'
//  DMP-NOT:   FunctionDecl
//      DMP:     ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//  DMP-NOT:     OMPDeclareTargetDeclAttr
//      DMP:   ACCRoutineDeclAttr
// DMP-NEXT:   OMPDeclareTargetDeclAttr
//      DMP: FunctionDecl {{.*}} expOffFnUseOfLocalNoDef_run 'void ()'
//
//      PRT: void expOffFnUseOfLocalNoDef_use(Result *Res) {
// PRT-NEXT:   void expOffFnUseOfLocalNoDef(Result *Res);
//      PRT: }{{$}}
//      PRT: void expOffFnUseOfLocalNoDef_run() {
//      PRT: }{{$}}
//
//      EXE-NEXT: expOffFnUseOfLocalNoDef: host=1, not_host=0
// EXE-HOST-NEXT: expOffFnUseOfLocalNoDef: host=1, not_host=0
//  EXE-OFF-NEXT: expOffFnUseOfLocalNoDef: host=0, not_host=1
#pragma acc routine seq
void expOffFnUseOfLocalNoDef_use(Result *Res) {
  void expOffFnUseOfLocalNoDef(Result *Res);
  USE1(expOffFnUseOfLocalNoDef, Res);
}
void expOffFnUseOfLocalNoDef_run() {
  Result Res;
  expOffFnUseOfLocalNoDef_use(&Res);
  PRINT_RESULT(expOffFnUseOfLocalNoDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  expOffFnUseOfLocalNoDef_use(&Res);
  PRINT_RESULT(expOffFnUseOfLocalNoDef, Res);
}

//      DMP: FunctionDecl {{.*}} expOffFnUsesMultiple_fn1 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//  DMP-NOT:   OMPDeclareTargetDeclAttr
//      DMP: FunctionDecl {{.*}} expOffFnUsesMultiple_fn2 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//  DMP-NOT:   OMPDeclareTargetDeclAttr
//      DMP: FunctionDecl {{.*}} expOffFnUsesMultiple_use 'void (Result *, Result *)'
//      DMP: FunctionDecl {{.*}} expOffFnUsesMultiple_run 'void ()'
//      DMP: FunctionDecl {{.*}} expOffFnUsesMultiple_fn1 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Inherited Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//  DMP-NOT:   OMPDeclareTargetDeclAttr
//
// PRT-NEXT: void expOffFnUsesMultiple_fn1(Result *Res);
// PRT-NEXT: void expOffFnUsesMultiple_fn2(Result *Res) {
//      PRT: }{{$}}
//      PRT: void expOffFnUsesMultiple_use(Result *Res1, Result *Res2) {
//      PRT: }{{$}}
//      PRT: void expOffFnUsesMultiple_run() {
//      PRT: }{{$}}
// PRT-NEXT: void expOffFnUsesMultiple_fn1(Result *Res) {
//      PRT: }{{$}}
//
//      EXE-NEXT: expOffFnUsesMultiple_fn1: host=1, not_host=0
//      EXE-NEXT: expOffFnUsesMultiple_fn2: host=1, not_host=0
// EXE-HOST-NEXT: expOffFnUsesMultiple_fn1: host=1, not_host=0
// EXE-HOST-NEXT: expOffFnUsesMultiple_fn2: host=1, not_host=0
//  EXE-OFF-NEXT: expOffFnUsesMultiple_fn1: host=0, not_host=1
//  EXE-OFF-NEXT: expOffFnUsesMultiple_fn2: host=0, not_host=1
void expOffFnUsesMultiple_fn1(Result *Res);
void expOffFnUsesMultiple_fn2(Result *Res) { WRITE_RESULT(Res); }
#pragma acc routine seq
void expOffFnUsesMultiple_use(Result *Res1, Result *Res2) {
  USE1(expOffFnUsesMultiple_fn1, Res1);
  USE1(expOffFnUsesMultiple_fn2, Res2);
}
void expOffFnUsesMultiple_run() {
  Result Res1, Res2;
  expOffFnUsesMultiple_use(&Res1, &Res2);
  PRINT_RESULT(expOffFnUsesMultiple_fn1, Res1);
  PRINT_RESULT(expOffFnUsesMultiple_fn2, Res1);
  #pragma acc parallel num_gangs(1) copyout(Res1, Res2)
  expOffFnUsesMultiple_use(&Res1, &Res2);
  PRINT_RESULT(expOffFnUsesMultiple_fn1, Res1);
  PRINT_RESULT(expOffFnUsesMultiple_fn2, Res2);
}
void expOffFnUsesMultiple_fn1(Result *Res) { WRITE_RESULT(Res); }

//      DMP: FunctionDecl {{.*}} expOffFnUseEarlierDir_use 'void (Result *)'
//      DMP: FunctionDecl {{.*}} expOffFnUseEarlierDir 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//  DMP-NOT:   OMPDeclareTargetDeclAttr
//      DMP: FunctionDecl {{.*}} expOffFnUseEarlierDir_use 'void (Result *)'
//      DMP: FunctionDecl {{.*}} expOffFnUseEarlierDir_run 'void ()'
//
//      PRT: void expOffFnUseEarlierDir_use(Result *Res);
//      PRT: void expOffFnUseEarlierDir(Result *Res) {
//      PRT: }{{$}}
// PRT-NEXT: void expOffFnUseEarlierDir_use(Result *Res) {
//      PRT: }{{$}}
// PRT-NEXT: void expOffFnUseEarlierDir_run() {
//      PRT: }{{$}}
//
//      EXE-NEXT: expOffFnUseEarlierDir: host=1, not_host=0
// EXE-HOST-NEXT: expOffFnUseEarlierDir: host=1, not_host=0
//  EXE-OFF-NEXT: expOffFnUseEarlierDir: host=0, not_host=1
#pragma acc routine seq
void expOffFnUseEarlierDir_use(Result *Res);
void expOffFnUseEarlierDir(Result *Res) { WRITE_RESULT(Res); }
void expOffFnUseEarlierDir_use(Result *Res) {
  USE1(expOffFnUseEarlierDir, Res);
}
void expOffFnUseEarlierDir_run() {
  Result Res;
  expOffFnUseEarlierDir_use(&Res);
  PRINT_RESULT(expOffFnUseEarlierDir, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  expOffFnUseEarlierDir_use(&Res);
  PRINT_RESULT(expOffFnUseEarlierDir, Res);
}

//------------------------------------------------------------------------------
// Use from offload function with implicit routine directive implies routine
// directive.
//------------------------------------------------------------------------------

//      DMP: FunctionDecl {{.*}} impOffFnUse_fn5 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//  DMP-NOT:   OMPDeclareTargetDeclAttr
//      DMP: FunctionDecl {{.*}} impOffFnUse_fn4 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//  DMP-NOT:   OMPDeclareTargetDeclAttr
//      DMP: FunctionDecl {{.*}} impOffFnUse_fn3 'void (Result *, Result *)'
//  DMP-NOT:   ACCRoutineDeclAttr
//  DMP-NOT:   OMPDeclareTargetDeclAttr
//      DMP: FunctionDecl {{.*}} impOffFnUse_fn2 'void (Result *, Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//  DMP-NOT:   OMPDeclareTargetDeclAttr
//      DMP: FunctionDecl {{.*}} impOffFnUse_fn1 'void (Result *, Result *, Result *, Result *, Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//  DMP-NOT:   OMPDeclareTargetDeclAttr
//      DMP: FunctionDecl {{.*}} impOffFnUse_fn3 'void (Result *, Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//  DMP-NOT:   OMPDeclareTargetDeclAttr
//      DMP: FunctionDecl {{.*}} impOffFnUse_use 'void (Result *, Result *, Result *, Result *, Result *)'
//      DMP: FunctionDecl {{.*}} impOffFnUse_run 'void ()'
//
// PRT-NEXT: void impOffFnUse_fn5(Result *Res) {
//      PRT: }{{$}}
// PRT-NEXT: void impOffFnUse_fn4(Result *Res) {
//      PRT: }{{$}}
// PRT-NEXT: void impOffFnUse_fn3(Result *Res3, Result *Res5);
// PRT-NEXT: void impOffFnUse_fn2(Result *Res2, Result *Res4) {
//      PRT: }{{$}}
// PRT-NEXT: void impOffFnUse_fn1(Result *Res1, Result *Res2, Result *Res3, Result *Res4, Result *Res5) {
//      PRT: }{{$}}
// PRT-NEXT: void impOffFnUse_fn3(Result *Res3, Result *Res5) {
//      PRT: }{{$}}
//      PRT: void impOffFnUse_use(Result *Res1, Result *Res2, Result *Res3, Result *Res4, Result *Res5) {
//      PRT: }{{$}}
//      PRT: void impOffFnUse_run() {
//      PRT: }{{$}}
//
//      EXE-NEXT: impOffFnUse_fn1: host=1, not_host=0
//      EXE-NEXT: impOffFnUse_fn2: host=1, not_host=0
//      EXE-NEXT: impOffFnUse_fn3: host=1, not_host=0
//      EXE-NEXT: impOffFnUse_fn4: host=1, not_host=0
//      EXE-NEXT: impOffFnUse_fn5: host=1, not_host=0
// EXE-HOST-NEXT: impOffFnUse_fn1: host=1, not_host=0
// EXE-HOST-NEXT: impOffFnUse_fn2: host=1, not_host=0
// EXE-HOST-NEXT: impOffFnUse_fn3: host=1, not_host=0
// EXE-HOST-NEXT: impOffFnUse_fn4: host=1, not_host=0
// EXE-HOST-NEXT: impOffFnUse_fn5: host=1, not_host=0
//  EXE-OFF-NEXT: impOffFnUse_fn1: host=0, not_host=1
//  EXE-OFF-NEXT: impOffFnUse_fn2: host=0, not_host=1
//  EXE-OFF-NEXT: impOffFnUse_fn3: host=0, not_host=1
//  EXE-OFF-NEXT: impOffFnUse_fn4: host=0, not_host=1
//  EXE-OFF-NEXT: impOffFnUse_fn5: host=0, not_host=1
void impOffFnUse_fn5(Result *Res) { WRITE_RESULT(Res); }
void impOffFnUse_fn4(Result *Res) { WRITE_RESULT(Res); }
void impOffFnUse_fn3(Result *Res3, Result *Res5);
// At the definition for each of fn1, fn2, and fn3 below, Clang doesn't yet know
// whether the caller or the callees have routine directives, so it must record
// the caller-callee relationship in order to propagate implied routine
// directives later.  When that time comes, Clang must be careful to look up fn3
// by the correct FunctionDecl to retrieve fn3's callees: the fn3 FunctionDecl
// that was recorded for fn1->fn3 might not be the FunctionDecl that was
// recorded for fn3->fn5 as there was an additional fn3 FunctionDecl visible
// when the latter was recorded.
void impOffFnUse_fn2(Result *Res2, Result *Res4) {
  WRITE_RESULT(Res2);
  USE1(impOffFnUse_fn4, Res4);
}
void impOffFnUse_fn1(Result *Res1, Result *Res2, Result *Res3, Result *Res4, Result *Res5) {
  WRITE_RESULT(Res1);
  USE2(impOffFnUse_fn2, Res2, Res4);
  USE2(impOffFnUse_fn3, Res3, Res5);
}
void impOffFnUse_fn3(Result *Res3, Result *Res5) {
  WRITE_RESULT(Res3);
  USE1(impOffFnUse_fn5, Res5);
}
#pragma acc routine seq
void impOffFnUse_use(Result *Res1, Result *Res2, Result *Res3, Result *Res4, Result *Res5) {
  USE5(impOffFnUse_fn1, Res1, Res2, Res3, Res4, Res5);
}
void impOffFnUse_run() {
  Result Res1, Res2, Res3, Res4, Res5;
  impOffFnUse_use(&Res1, &Res2, &Res3, &Res4, &Res5);
  PRINT_RESULT(impOffFnUse_fn1, Res1);
  PRINT_RESULT(impOffFnUse_fn2, Res2);
  PRINT_RESULT(impOffFnUse_fn3, Res3);
  PRINT_RESULT(impOffFnUse_fn4, Res4);
  PRINT_RESULT(impOffFnUse_fn5, Res5);
  #pragma acc parallel num_gangs(1) copyout(Res1, Res2, Res3, Res4, Res5)
  impOffFnUse_use(&Res1, &Res2, &Res3, &Res4, &Res5);
  PRINT_RESULT(impOffFnUse_fn1, Res1);
  PRINT_RESULT(impOffFnUse_fn2, Res2);
  PRINT_RESULT(impOffFnUse_fn3, Res3);
  PRINT_RESULT(impOffFnUse_fn4, Res4);
  PRINT_RESULT(impOffFnUse_fn5, Res5);
}

//      DMP: FunctionDecl {{.*}} impOffFnUseMutual_fn1 'void (Result *, Result *)'
//  DMP-NOT:   ACCRoutineDeclAttr
//  DMP-NOT:   OMPDeclareTargetDeclAttr
//      DMP: FunctionDecl {{.*}} impOffFnUseMutual_fn2 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//  DMP-NOT:   OMPDeclareTargetDeclAttr
//      DMP: FunctionDecl {{.*}} impOffFnUseMutual_fn1 'void (Result *, Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//  DMP-NOT:   OMPDeclareTargetDeclAttr
//      DMP: FunctionDecl {{.*}} impOffFnUseMutual_use 'void (Result *, Result *)'
//      DMP: FunctionDecl {{.*}} impOffFnUseMutual_run 'void ()'
//
// PRT-NEXT: void impOffFnUseMutual_fn1(Result *Res1, Result *Res2);
// PRT-NEXT: void impOffFnUseMutual_fn2(Result *Res) {
//      PRT: }{{$}}
// PRT-NEXT: void impOffFnUseMutual_fn1(Result *Res1, Result *Res2) {
//      PRT: }{{$}}
//      PRT: void impOffFnUseMutual_use(Result *Res1, Result *Res2) {
//      PRT: }{{$}}
//      PRT: void impOffFnUseMutual_run() {
//      PRT: }{{$}}
//
//      EXE-NEXT: impOffFnUseMutual_fn1: host=1, not_host=0
//      EXE-NEXT: impOffFnUseMutual_fn2: host=1, not_host=0
// EXE-HOST-NEXT: impOffFnUseMutual_fn1: host=1, not_host=0
// EXE-HOST-NEXT: impOffFnUseMutual_fn2: host=1, not_host=0
//  EXE-OFF-NEXT: impOffFnUseMutual_fn1: host=0, not_host=1
//  EXE-OFF-NEXT: impOffFnUseMutual_fn2: host=0, not_host=1
void impOffFnUseMutual_fn1(Result *Res1, Result *Res2);
void impOffFnUseMutual_fn2(Result *Res) {
  WRITE_RESULT(Res);
  // FIXME: Recursion doesn't seem to be handled well on amdgcn.  In this case,
  // we see the following error at link time on ExCL's explorer:
  //
  //   llc: /path/to/llvm-git/llvm/lib/Target/AMDGPU/AMDGPUArgumentUsageInfo.cpp:181: const llvm::AMDGPUFunctionArgInfo& llvm::AMDGPUArgumentUsageInfo::lookupFuncArgInfo(const llvm::Function&) const: Assertion `F.isDeclaration()' failed.
#if !TGT_AMDGCN
  USE2(impOffFnUseMutual_fn1, 0, 0);
#endif
}
void impOffFnUseMutual_fn1(Result *Res1, Result *Res2) {
  // Avoid infinite recursion during execution.
  if (!Res1)
    return;
  WRITE_RESULT(Res1);
  // At this point, Clang doesn't know whether there is a routine directive for
  // either the caller (fn1) or the callee (fn2).  Later, when a routine
  // directive is implied for the caller, Clang has to be careful to avoid an
  // infinite recursion when computing implied routine directive's for the
  // callee chain (fn1 -> fn2 -> fn1 -> ...).
  USE1(impOffFnUseMutual_fn2, Res2);
}
#pragma acc routine seq
void impOffFnUseMutual_use(Result *Res1, Result *Res2) {
  USE2(impOffFnUseMutual_fn1, Res1, Res2);
}
void impOffFnUseMutual_run() {
  Result Res1, Res2;
  impOffFnUseMutual_use(&Res1, &Res2);
  PRINT_RESULT(impOffFnUseMutual_fn1, Res1);
  PRINT_RESULT(impOffFnUseMutual_fn2, Res2);
  #pragma acc parallel num_gangs(1) copyout(Res1, Res2)
  impOffFnUseMutual_use(&Res1, &Res2);
  PRINT_RESULT(impOffFnUseMutual_fn1, Res1);
  PRINT_RESULT(impOffFnUseMutual_fn2, Res2);
}

//      DMP: FunctionDecl {{.*}} impOffFnUseRecur 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//  DMP-NOT:   OMPDeclareTargetDeclAttr
//      DMP: FunctionDecl {{.*}} impOffFnUseRecur_use 'void (Result *)'
//      DMP: FunctionDecl {{.*}} impOffFnUseRecur_run 'void ()'
//
// PRT-NEXT: void impOffFnUseRecur(Result *Res) {
//      PRT: void impOffFnUseRecur_use(Result *Res) {
//      PRT: }{{$}}
//      PRT: void impOffFnUseRecur_run() {
//      PRT: }{{$}}
//
//      EXE-NEXT: impOffFnUseRecur: host=1, not_host=0
// EXE-HOST-NEXT: impOffFnUseRecur: host=1, not_host=0
//  EXE-OFF-NEXT: impOffFnUseRecur: host=0, not_host=1
void impOffFnUseRecur(Result *Res) {
  // Avoid infinite recursion during execution.
  if (!Res)
    return;
  WRITE_RESULT(Res);
  USE1(impOffFnUseRecur, 0);
}
#pragma acc routine seq
void impOffFnUseRecur_use(Result *Res) {
  USE1(impOffFnUseRecur, Res);
}
void impOffFnUseRecur_run() {
  Result Res;
  impOffFnUseRecur_use(&Res);
  PRINT_RESULT(impOffFnUseRecur, Res);
  // FIXME: Recursion doesn't seem to be handled well on amdgcn.  In this case,
  // we see the following error at run time on ExCL's explorer:
  //
  //   [GPU Memory Error] Addr: 0x7f0740000000 Reason: Page not present or supervisor privilege.
  //   Memory access fault by GPU node-8 (Agent handle: 0xd8c330) on address 0x7f0740000000. Reason: Page not present or supervisor privilege.
#if TGT_AMDGCN
  // Fake the result for now.
  Res.Host = 0;
  Res.NotHost = 1;
#else
  #pragma acc parallel num_gangs(1) copyout(Res)
  impOffFnUseRecur_use(&Res);
#endif
  PRINT_RESULT(impOffFnUseRecur, Res);
}

//------------------------------------------------------------------------------
// Multiple offload uses for same function.
//------------------------------------------------------------------------------

//      DMP: FunctionDecl {{.*}} multipleOffUse 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
//  DMP-NOT:   OMPDeclareTargetDeclAttr
//      DMP: FunctionDecl {{.*}} multipleOffUse_use 'void (Result *)'
//      DMP: FunctionDecl {{.*}} multipleOffUse_run 'void ()'
//
// PRT-NEXT: void multipleOffUse(Result *Res) {
//      PRT: }{{$}}
//      PRT: void multipleOffUse_use(Result *Res) {
//      PRT: }{{$}}
//      PRT: void multipleOffUse_run() {
//      PRT: }{{$}}
//
//      EXE-NEXT: multipleOffUse: host=1, not_host=0
// EXE-HOST-NEXT: multipleOffUse: host=1, not_host=0
// EXE-HOST-NEXT: multipleOffUse: host=1, not_host=0
//  EXE-OFF-NEXT: multipleOffUse: host=0, not_host=1
//  EXE-OFF-NEXT: multipleOffUse: host=0, not_host=1
void multipleOffUse(Result *Res) { WRITE_RESULT(Res); }
#pragma acc routine seq
void multipleOffUse_use(Result *Res) {
  USE1(multipleOffUse, Res);
}
void multipleOffUse_run() {
  Result Res;
  multipleOffUse_use(&Res);
  PRINT_RESULT(multipleOffUse, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  multipleOffUse_use(&Res);
  PRINT_RESULT(multipleOffUse, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  USE1(multipleOffUse, &Res);
  PRINT_RESULT(multipleOffUse, Res);
}

//------------------------------------------------------------------------------
// Host use before offload use doesn't suppress routine directive.
//
// At one point during development, no routine directive was implied here
// because all uses after the first use were skipped.
//------------------------------------------------------------------------------

//     DMP: FunctionDecl {{.*}} hostUseOffUse 'void (Result *)'
// DMP-NOT: FunctionDecl
//     DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
// DMP-NOT:   OMPDeclareTargetDeclAttr
//     DMP: FunctionDecl {{.*}} hostUseOffUse_run 'void ()'
//
// PRT-NEXT: void hostUseOffUse(Result *Res) {
//      PRT: }{{$}}
// PRT-NEXT: void hostUseOffUse_run() {
//      PRT: }{{$}}
//
//      EXE-NEXT: hostUseOffUse: host=1, not_host=0
// EXE-HOST-NEXT: hostUseOffUse: host=1, not_host=0
//  EXE-OFF-NEXT: hostUseOffUse: host=0, not_host=1
void hostUseOffUse(Result *Res) { WRITE_RESULT(Res); }
void hostUseOffUse_run() {
  Result Res;
  USE1(hostUseOffUse, &Res);
  PRINT_RESULT(hostUseOffUse, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  USE1(hostUseOffUse, &Res);
  PRINT_RESULT(hostUseOffUse, Res);
}

//------------------------------------------------------------------------------
// Offload use before host use works too.
//------------------------------------------------------------------------------

//     DMP: FunctionDecl {{.*}} offUseHostUse 'void (Result *)'
// DMP-NOT: FunctionDecl
//     DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
// DMP-NOT:   OMPDeclareTargetDeclAttr
//     DMP: FunctionDecl {{.*}} offUseHostUse_run 'void ()'
//
// PRT-NEXT: void offUseHostUse(Result *Res) {
//      PRT: }{{$}}
// PRT-NEXT: void offUseHostUse_run() {
//      PRT: }{{$}}
//
// EXE-HOST-NEXT: offUseHostUse: host=1, not_host=0
//  EXE-OFF-NEXT: offUseHostUse: host=0, not_host=1
//      EXE-NEXT: offUseHostUse: host=1, not_host=0
void offUseHostUse(Result *Res) { WRITE_RESULT(Res); }
void offUseHostUse_run() {
  Result Res;
  #pragma acc parallel num_gangs(1) copyout(Res)
  USE1(offUseHostUse, &Res);
  PRINT_RESULT(offUseHostUse, Res);
  USE1(offUseHostUse, &Res);
  PRINT_RESULT(offUseHostUse, Res);
}

//------------------------------------------------------------------------------
// Reference that is not a use does not imply routine directive.
//------------------------------------------------------------------------------

//     DMP: FunctionDecl {{.*}} refNotUseInPar 'void ()'
// DMP-NOT:   ACCRoutineDeclAttr
// DMP-NOT:   OMPDeclareTargetDeclAttr
//     DMP: FunctionDecl {{.*}} refNotUseInPar_run 'void ()'
//
// PRT-NEXT: void refNotUseInPar() {
//      PRT: }{{$}}
// PRT-NEXT: void refNotUseInPar_run() {
//      PRT: }{{$}}
//
// EXE-NEXT: sizeof: {{[^0]}}
void refNotUseInPar() {}
void refNotUseInPar_run() {
  size_t S;
  #pragma acc parallel num_gangs(1) copyout(S)
  S = sizeof(refNotUseInPar);
  printf("sizeof: %zu\n", S);
}

//     DMP: FunctionDecl {{.*}} refNotUseInExpOffFn 'void ()'
// DMP-NOT:   ACCRoutineDeclAttr
// DMP-NOT:   OMPDeclareTargetDeclAttr
//     DMP: FunctionDecl {{.*}} refNotUseInExpOffFn_use 'void (size_t *)'
//     DMP: FunctionDecl {{.*}} refNotUseInExpOffFn_run 'void ()'
//
// PRT-NEXT: void refNotUseInExpOffFn() {
//      PRT: }{{$}}
//      PRT: void refNotUseInExpOffFn_use(size_t *S) {
//      PRT: }{{$}}
//      PRT: void refNotUseInExpOffFn_run() {
//      PRT: }{{$}}
//
// EXE-NEXT: sizeof: {{[^0]}}
void refNotUseInExpOffFn() {}
#pragma acc routine seq
void refNotUseInExpOffFn_use(size_t *S) {
  *S = sizeof(refNotUseInExpOffFn);
}
void refNotUseInExpOffFn_run() {
  size_t S;
  #pragma acc parallel num_gangs(1) copyout(S)
  refNotUseInExpOffFn_use(&S);
  printf("sizeof: %zu\n", S);
}

//     DMP: FunctionDecl {{.*}} refNotUseInImpOffFn 'void ()'
// DMP-NOT:   ACCRoutineDeclAttr
// DMP-NOT:   OMPDeclareTargetDeclAttr
//     DMP: FunctionDecl {{.*}} refNotUseInImpOffFn_use 'void (size_t *)'
//     DMP: FunctionDecl {{.*}} refNotUseInImpOffFn_run 'void ()'
//
// PRT-NEXT: void refNotUseInImpOffFn() {
//      PRT: }{{$}}
//      PRT: void refNotUseInImpOffFn_use(size_t *S) {
//      PRT: }{{$}}
//      PRT: void refNotUseInImpOffFn_run() {
//      PRT: }{{$}}
//
// EXE-NEXT: sizeof: {{[^0]}}
void refNotUseInImpOffFn() {}
void refNotUseInImpOffFn_use(size_t *S) {
  *S = sizeof(refNotUseInImpOffFn);
}
void refNotUseInImpOffFn_run() {
  size_t S;
  #pragma acc parallel num_gangs(1) copyout(S)
  refNotUseInImpOffFn_use(&S);
  printf("sizeof: %zu\n", S);
}

int main(int argc, char *argv[]) {
  start();
  hostUseBeforeDef_run();
  hostUseAfterDef_run();
  hostUseNoDef_run();
  hostUseRecur_run();
  parUseBeforeDef_run();
  parUseOfLocalBeforeDef_run();
  parUseAfterDef_run();
  parUseOfLocalAfterDef_run();
  parUseNoDef_run();
  parUseOfLocalNoDef_run();
  parUsesMultiple_run();
  expOffFnUseAfterDef_run();
  expOffFnUseOfLocalAfterDef_run();
  expOffFnUseBeforeDef_run();
  expOffFnUseOfLocalBeforeDef_run();
  expOffFnUseNoDef_run();
  expOffFnUseOfLocalNoDef_run();
  expOffFnUsesMultiple_run();
  expOffFnUseEarlierDir_run();
  impOffFnUse_run();
  impOffFnUseMutual_run();
  impOffFnUseRecur_run();
  multipleOffUse_run();
  hostUseOffUse_run();
  offUseHostUse_run();
  refNotUseInPar_run();
  refNotUseInExpOffFn_run();
  refNotUseInImpOffFn_run();
  return 0;
}

// EXE-NOT: {{.}}

#else

#pragma acc routine seq
void hostUseNoDef(Result *Res) { WRITE_RESULT(Res); }

#pragma acc routine seq
void parUseNoDef(Result *Res) { WRITE_RESULT(Res); }

#pragma acc routine seq
void parUseOfLocalNoDef(Result *Res) { WRITE_RESULT(Res); }

#pragma acc routine seq
void expOffFnUseNoDef(Result *Res) { WRITE_RESULT(Res); }

#pragma acc routine seq
void expOffFnUseOfLocalNoDef(Result *Res) { WRITE_RESULT(Res); }

#endif
