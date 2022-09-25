// Check that "acc routine seq" behaves correctly when placed on a namespace
// member function prototype, definition, neither, or both.  Also check various
// ways "acc routine seq" might be implied by other member functions or
// non-member functions.

// REDEFINE: %{dmp:fc:args} = -implicit-check-not=ACCRoutineDeclAttr \
// REDEFINE:                  -implicit-check-not=OMPDeclareTargetDeclAttr
// RUN: %{acc-check-dmp-cxx}
// RUN: %{acc-check-prt-cxx}
//
// REDEFINE: %{exe:base-name} = main
// RUN: %{acc-check-exe-cxx-compile-c}
//
// REDEFINE: %{exe:base-name} = other
// REDEFINE: %{exe:clang:args} = -DCOMPILE_OTHER
// RUN: %{acc-check-exe-cxx-compile-c}

// REDEFINE: %{exe:clang:args} = main.o other.o
// REDEFINE: %{exe:fc:args} = -strict-whitespace
// RUN: %{acc-check-exe-cxx-link}
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
    printf("%s: host=%d, not_host=%d\n", #Fn, (Res).Host, (Res).NotHost)

// DMP: NamespaceDecl
//
// PRT: namespace Test {
namespace Test {
  //----------------------------------------------------------------------------
  // Usees with routine directives implied only by other members.

  //     DMP: FunctionDecl {{.*}} declWithNamespaceUser 'void (Result *)'
  // DMP-NOT: FunctionDecl
  //     DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
  // DMP-NOT:   OMPDeclareTargetDeclAttr
  //
  // PRT-NEXT: void declWithNamespaceUser(Result *);
  void declWithNamespaceUser(Result *);

  //     DMP: FunctionDecl {{.*}} declWithAfterNamespaceUser 'void (Result *)'
  // DMP-NOT: FunctionDecl
  //     DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
  // DMP-NOT:   OMPDeclareTargetDeclAttr
  //
  // PRT-NEXT: void declWithAfterNamespaceUser(Result *);
  void declWithAfterNamespaceUser(Result *);

  //     DMP: FunctionDecl {{.*}} defWithNamespaceUser 'void (Result *)'
  // DMP-NOT: FunctionDecl
  //     DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
  // DMP-NOT:   OMPDeclareTargetDeclAttr
  //
  // PRT-NEXT: inline void defWithNamespaceUser(Result *Res) {
  //      PRT: }{{$}}
  inline void defWithNamespaceUser(Result *Res) { WRITE_RESULT(Res); }

  //     DMP: FunctionDecl {{.*}} defWithNamespaceAccUser 'void (Result *)'
  // DMP-NOT: FunctionDecl
  //     DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
  // DMP-NOT:   OMPDeclareTargetDeclAttr
  //
  // PRT-NEXT: inline void defWithNamespaceAccUser(Result *Res) {
  //      PRT: }{{$}}
  inline void defWithNamespaceAccUser(Result *Res) { WRITE_RESULT(Res); }

  //     DMP: FunctionDecl {{.*}} defWithAfterNamespaceUser 'void (Result *)'
  // DMP-NOT: FunctionDecl
  //     DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
  // DMP-NOT:   OMPDeclareTargetDeclAttr
  //
  // PRT-NEXT: inline void defWithAfterNamespaceUser(Result *Res) {
  //      PRT: }{{$}}
  inline void defWithAfterNamespaceUser(Result *Res) { WRITE_RESULT(Res); }

  //----------------------------------------------------------------------------
  // Usees with routine directives implied only by non-members.

  //     DMP: FunctionDecl {{.*}} declWithNonNamespaceUser 'void (Result *)'
  // DMP-NOT: FunctionDecl
  //     DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
  // DMP-NOT:   OMPDeclareTargetDeclAttr
  //
  // PRT-NEXT: void declWithNonNamespaceUser(Result *);
  void declWithNonNamespaceUser(Result *);

  //     DMP: FunctionDecl {{.*}} defWithNonNamespaceUser 'void (Result *)'
  // DMP-NOT: FunctionDecl
  //     DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
  // DMP-NOT:   OMPDeclareTargetDeclAttr
  //
  // PRT-NEXT: inline void defWithNonNamespaceUser(Result *Res) {
  //      PRT: }{{$}}
  inline void defWithNonNamespaceUser(Result *Res) { WRITE_RESULT(Res); }

  //----------------------------------------------------------------------------
  // Usees with explicit routine directives.

  //      DMP: FunctionDecl {{.*}} onDecl 'void (Result *)'
  //  DMP-NOT: FunctionDecl
  //      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
  // DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
  //    PRT-NEXT: void onDecl(Result *);
  //  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
  #pragma acc routine seq
  void onDecl(Result *);

  //      DMP: FunctionDecl {{.*}} onDef 'void (Result *)'
  //  DMP-NOT: FunctionDecl
  //      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
  // DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
  //    PRT-NEXT: inline void onDef(Result *Res) {
  //         PRT: }{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
  #pragma acc routine seq
  inline void onDef(Result *Res) { WRITE_RESULT(Res); }

  //      DMP: FunctionDecl [[#%#x,onDeclInnerDef:]] {{.*}} onDeclInnerDef 'void (Result *)'
  //  DMP-NOT: FunctionDecl
  //      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
  // DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
  //    PRT-NEXT: void onDeclInnerDef(Result *);
  //  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
  #pragma acc routine seq
  void onDeclInnerDef(Result *);

  //      DMP: FunctionDecl [[#%#x,onDeclOuterDef:]] {{.*}} onDeclOuterDef 'void (Result *)'
  //  DMP-NOT: FunctionDecl
  //      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
  // DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
  //    PRT-NEXT: void onDeclOuterDef(Result *);
  //  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
  #pragma acc routine seq
  void onDeclOuterDef(Result *);

  // DMP: FunctionDecl [[#%#x,declOnInnerDef:]] {{.*}} declOnInnerDef 'void (Result *)'
  //
  // PRT-NEXT: void declOnInnerDef(Result *);
  void declOnInnerDef(Result *);

  //      DMP: FunctionDecl {{.*}} prev [[#declOnInnerDef]] {{.*}} declOnInnerDef 'void (Result *)'
  //  DMP-NOT: FunctionDecl
  //      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
  // DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
  //    PRT-NEXT: inline void declOnInnerDef(Result *Res) {
  //         PRT: }{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
  #pragma acc routine seq
  inline void declOnInnerDef(Result *Res) { WRITE_RESULT(Res); }

  // DMP: FunctionDecl [[#%#x,declOnOuterDef:]] {{.*}} declOnOuterDef 'void (Result *)'
  //
  // PRT-NEXT: void declOnOuterDef(Result *);
  void declOnOuterDef(Result *);

  //      DMP: FunctionDecl [[#%#x,onDeclOnInnerDef:]] {{.*}} onDeclOnInnerDef 'void (Result *)'
  //  DMP-NOT: FunctionDecl
  //      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
  // DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
  //    PRT-NEXT: void onDeclOnInnerDef(Result *);
  //  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
  #pragma acc routine seq
  void onDeclOnInnerDef(Result *);

  //      DMP: FunctionDecl [[#%#x,onDeclOnOuterDef:]] {{.*}} onDeclOnOuterDef 'void (Result *)'
  //  DMP-NOT: FunctionDecl
  //      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
  // DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
  //    PRT-NEXT: void onDeclOnOuterDef(Result *);
  //  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
  #pragma acc routine seq
  void onDeclOnOuterDef(Result *);

  //----------------------------------------------------------------------------
  // Member users after usees are declared.

  // PRT-NEXT: inline void user() {
  //      PRT: }
  inline void user() {
    // Here, we call all member functions that have explicit routine directives
    // and only some member functions that don't to be sure their calls
    // elsewhere are sufficient to imply routine directives.
    Result Res;
    //           EXE: declWithNamespaceUser: host=1, not_host=0
    //      EXE-NEXT:  defWithNamespaceUser: host=1, not_host=0
    //      EXE-NEXT:                onDecl: host=1, not_host=0
    //      EXE-NEXT:                 onDef: host=1, not_host=0
    //      EXE-NEXT:        onDeclInnerDef: host=1, not_host=0
    //      EXE-NEXT:        onDeclOuterDef: host=1, not_host=0
    //      EXE-NEXT:        declOnInnerDef: host=1, not_host=0
    //      EXE-NEXT:      onDeclOnInnerDef: host=1, not_host=0
    //      EXE-NEXT:      onDeclOnOuterDef: host=1, not_host=0
    declWithNamespaceUser(&Res); PRINT_RESULT(declWithNamespaceUser, Res);
    defWithNamespaceUser(&Res); PRINT_RESULT(defWithNamespaceUser, Res);
    onDecl(&Res); PRINT_RESULT(onDecl, Res);
    onDef(&Res); PRINT_RESULT(onDef, Res);
    onDeclInnerDef(&Res); PRINT_RESULT(onDeclInnerDef, Res);
    onDeclOuterDef(&Res); PRINT_RESULT(onDeclOuterDef, Res);
    declOnInnerDef(&Res); PRINT_RESULT(declOnInnerDef, Res);
    onDeclOnInnerDef(&Res); PRINT_RESULT(onDeclOnInnerDef, Res);
    onDeclOnOuterDef(&Res); PRINT_RESULT(onDeclOnOuterDef, Res);
    // EXE-HOST-NEXT: declWithNamespaceUser: host=1, not_host=0
    // EXE-HOST-NEXT:  defWithNamespaceUser: host=1, not_host=0
    // EXE-HOST-NEXT:                onDecl: host=1, not_host=0
    // EXE-HOST-NEXT:                 onDef: host=1, not_host=0
    // EXE-HOST-NEXT:        onDeclInnerDef: host=1, not_host=0
    // EXE-HOST-NEXT:        onDeclOuterDef: host=1, not_host=0
    // EXE-HOST-NEXT:        declOnInnerDef: host=1, not_host=0
    // EXE-HOST-NEXT:      onDeclOnInnerDef: host=1, not_host=0
    // EXE-HOST-NEXT:      onDeclOnOuterDef: host=1, not_host=0
    //  EXE-OFF-NEXT: declWithNamespaceUser: host=0, not_host=1
    //  EXE-OFF-NEXT:  defWithNamespaceUser: host=0, not_host=1
    //  EXE-OFF-NEXT:                onDecl: host=0, not_host=1
    //  EXE-OFF-NEXT:                 onDef: host=0, not_host=1
    //  EXE-OFF-NEXT:        onDeclInnerDef: host=0, not_host=1
    //  EXE-OFF-NEXT:        onDeclOuterDef: host=0, not_host=1
    //  EXE-OFF-NEXT:        declOnInnerDef: host=0, not_host=1
    //  EXE-OFF-NEXT:      onDeclOnInnerDef: host=0, not_host=1
    //  EXE-OFF-NEXT:      onDeclOnOuterDef: host=0, not_host=1
    #pragma acc parallel num_gangs(1) copyout(Res)
    declWithNamespaceUser(&Res); PRINT_RESULT(declWithNamespaceUser, Res);
    #pragma acc parallel num_gangs(1) copyout(Res)
    defWithNamespaceUser(&Res); PRINT_RESULT(defWithNamespaceUser, Res);
    #pragma acc parallel num_gangs(1) copyout(Res)
    onDecl(&Res); PRINT_RESULT(onDecl, Res);
    #pragma acc parallel num_gangs(1) copyout(Res)
    onDef(&Res); PRINT_RESULT(onDef, Res);
    #pragma acc parallel num_gangs(1) copyout(Res)
    onDeclInnerDef(&Res); PRINT_RESULT(onDeclInnerDef, Res);
    #pragma acc parallel num_gangs(1) copyout(Res)
    onDeclOuterDef(&Res); PRINT_RESULT(onDeclOuterDef, Res);
    #pragma acc parallel num_gangs(1) copyout(Res)
    declOnInnerDef(&Res); PRINT_RESULT(declOnInnerDef, Res);
    #pragma acc parallel num_gangs(1) copyout(Res)
    onDeclOnInnerDef(&Res); PRINT_RESULT(onDeclOnInnerDef, Res);
    #pragma acc parallel num_gangs(1) copyout(Res)
    onDeclOnOuterDef(&Res); PRINT_RESULT(onDeclOnOuterDef, Res);
  }

  //      DMP: FunctionDecl {{.*}} accUser 'void (Result *)'
  //  DMP-NOT: FunctionDecl
  //      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
  // DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
  //    PRT-NEXT: inline void accUser(Result *Res) {
  //         PRT: }
  //  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
  #pragma acc routine seq
  inline void accUser(Result *Res) {
    // EXE-HOST-NEXT: accUser: host=1, not_host=0
    //  EXE-OFF-NEXT: accUser: host=0, not_host=1
    defWithNamespaceAccUser(Res);
  }

  // PRT-NEXT: void userAfterNamespace();
  void userAfterNamespace();

  //----------------------------------------------------------------------------
  // Usee inner definitions after their inner uses.

  //      DMP: FunctionDecl {{.*}} prev [[#onDeclInnerDef]] {{.*}} onDeclInnerDef 'void (Result *)'
  //  DMP-NOT: FunctionDecl
  //      DMP:   ACCRoutineDeclAttr {{.*}}> Inherited Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
  // DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> Inherited MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
  //
  //    PRT-NEXT: inline void onDeclInnerDef(Result *Res) {
  //         PRT: }{{$}}
  inline void onDeclInnerDef(Result *Res) { WRITE_RESULT(Res); }

  //      DMP: FunctionDecl {{.*}} prev [[#onDeclOnInnerDef]] {{.*}} onDeclOnInnerDef 'void (Result *)'
  //  DMP-NOT: FunctionDecl
  //      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
  // DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
  //    PRT-NEXT: inline void onDeclOnInnerDef(Result *Res) {
  //         PRT: }{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
  #pragma acc routine seq
  inline void onDeclOnInnerDef(Result *Res) { WRITE_RESULT(Res); }
} // PRT-NEXT: }

// PRT-SRC-NEXT: #ifndef COMPILE_OTHER
#ifndef COMPILE_OTHER

//      DMP: FunctionDecl {{.*}} prev [[#declOnOuterDef]] {{.*}} declOnOuterDef 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
//    PRT-NEXT: void Test::declOnOuterDef(Result *Res) {
//         PRT: }{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
#pragma acc routine seq
void Test::declOnOuterDef(Result *Res) { WRITE_RESULT(Res); }

// PRT-NEXT: void Test::userAfterNamespace() {
//      PRT: }{{$}}
void Test::userAfterNamespace() {
  // Here, we call all member functions that have explicit routine directives
  // and only some member functions that don't to be sure their calls
  // elsewhere are sufficient to imply routine directives.
  Result Res;
  //      EXE-NEXT: declWithAfterNamespaceUser: host=1, not_host=0
  //      EXE-NEXT:  defWithAfterNamespaceUser: host=1, not_host=0
  //      EXE-NEXT:                     onDecl: host=1, not_host=0
  //      EXE-NEXT:                      onDef: host=1, not_host=0
  //      EXE-NEXT:             onDeclInnerDef: host=1, not_host=0
  //      EXE-NEXT:             onDeclOuterDef: host=1, not_host=0
  //      EXE-NEXT:             declOnInnerDef: host=1, not_host=0
  //      EXE-NEXT:             declOnOuterDef: host=1, not_host=0
  //      EXE-NEXT:           onDeclOnInnerDef: host=1, not_host=0
  //      EXE-NEXT:           onDeclOnOuterDef: host=1, not_host=0
  declWithAfterNamespaceUser(&Res); PRINT_RESULT(declWithAfterNamespaceUser, Res);
  defWithAfterNamespaceUser(&Res); PRINT_RESULT(defWithAfterNamespaceUser, Res);
  onDecl(&Res); PRINT_RESULT(onDecl, Res);
  onDef(&Res); PRINT_RESULT(onDef, Res);
  onDeclInnerDef(&Res); PRINT_RESULT(onDeclInnerDef, Res);
  onDeclOuterDef(&Res); PRINT_RESULT(onDeclOuterDef, Res);
  declOnInnerDef(&Res); PRINT_RESULT(declOnInnerDef, Res);
  declOnOuterDef(&Res); PRINT_RESULT(declOnOuterDef, Res);
  onDeclOnInnerDef(&Res); PRINT_RESULT(onDeclOnInnerDef, Res);
  onDeclOnOuterDef(&Res); PRINT_RESULT(onDeclOnOuterDef, Res);
  // EXE-HOST-NEXT: declWithAfterNamespaceUser: host=1, not_host=0
  // EXE-HOST-NEXT:  defWithAfterNamespaceUser: host=1, not_host=0
  // EXE-HOST-NEXT:                     onDecl: host=1, not_host=0
  // EXE-HOST-NEXT:                      onDef: host=1, not_host=0
  // EXE-HOST-NEXT:             onDeclInnerDef: host=1, not_host=0
  // EXE-HOST-NEXT:             onDeclOuterDef: host=1, not_host=0
  // EXE-HOST-NEXT:             declOnInnerDef: host=1, not_host=0
  // EXE-HOST-NEXT:             declOnOuterDef: host=1, not_host=0
  // EXE-HOST-NEXT:           onDeclOnInnerDef: host=1, not_host=0
  // EXE-HOST-NEXT:           onDeclOnOuterDef: host=1, not_host=0
  //  EXE-OFF-NEXT: declWithAfterNamespaceUser: host=0, not_host=1
  //  EXE-OFF-NEXT:  defWithAfterNamespaceUser: host=0, not_host=1
  //  EXE-OFF-NEXT:                     onDecl: host=0, not_host=1
  //  EXE-OFF-NEXT:                      onDef: host=0, not_host=1
  //  EXE-OFF-NEXT:             onDeclInnerDef: host=0, not_host=1
  //  EXE-OFF-NEXT:             onDeclOuterDef: host=0, not_host=1
  //  EXE-OFF-NEXT:             declOnInnerDef: host=0, not_host=1
  //  EXE-OFF-NEXT:             declOnOuterDef: host=0, not_host=1
  //  EXE-OFF-NEXT:           onDeclOnInnerDef: host=0, not_host=1
  //  EXE-OFF-NEXT:           onDeclOnOuterDef: host=0, not_host=1
  #pragma acc parallel num_gangs(1) copyout(Res)
  declWithAfterNamespaceUser(&Res); PRINT_RESULT(declWithAfterNamespaceUser, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  defWithAfterNamespaceUser(&Res); PRINT_RESULT(defWithAfterNamespaceUser, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  onDecl(&Res); PRINT_RESULT(onDecl, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  onDef(&Res); PRINT_RESULT(onDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  onDeclInnerDef(&Res); PRINT_RESULT(onDeclInnerDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  onDeclOuterDef(&Res); PRINT_RESULT(onDeclOuterDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  declOnInnerDef(&Res); PRINT_RESULT(declOnInnerDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  declOnOuterDef(&Res); PRINT_RESULT(declOnOuterDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  onDeclOnInnerDef(&Res); PRINT_RESULT(onDeclOnInnerDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  onDeclOnOuterDef(&Res); PRINT_RESULT(onDeclOnOuterDef, Res);
}

//      DMP: FunctionDecl {{.*}} prev [[#onDeclOuterDef]] {{.*}} onDeclOuterDef 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Inherited Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> Inherited MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//    PRT-NEXT: void Test::onDeclOuterDef(Result *Res) {
//         PRT: }{{$}}
void Test::onDeclOuterDef(Result *Res) { WRITE_RESULT(Res); }

//      DMP: FunctionDecl {{.*}} prev [[#onDeclOnOuterDef]] {{.*}} onDeclOnOuterDef 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
//    PRT-NEXT: void Test::onDeclOnOuterDef(Result *Res) {
//         PRT: }{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
#pragma acc routine seq
void Test::onDeclOnOuterDef(Result *Res) { WRITE_RESULT(Res); }

// EXE-NOT: {{.}}
int main(int argc, char *argv[]) {
  // Here, we call all member functions that have explicit routine directives
  // and only some member functions that don't to be sure their calls
  // elsewhere are sufficient to imply routine directives.
  Result Res;
  Test::user();
  #pragma acc parallel num_gangs(1) copyout(Res)
  Test::accUser(&Res); PRINT_RESULT(accUser, Res);
  Test::userAfterNamespace();
  //      EXE-NEXT: declWithNonNamespaceUser: host=1, not_host=0
  //      EXE-NEXT:  defWithNonNamespaceUser: host=1, not_host=0
  //      EXE-NEXT:                   onDecl: host=1, not_host=0
  //      EXE-NEXT:                    onDef: host=1, not_host=0
  //      EXE-NEXT:           onDeclOuterDef: host=1, not_host=0
  //      EXE-NEXT:           declOnInnerDef: host=1, not_host=0
  //      EXE-NEXT:           declOnOuterDef: host=1, not_host=0
  //      EXE-NEXT:         onDeclOnInnerDef: host=1, not_host=0
  //      EXE-NEXT:         onDeclOnOuterDef: host=1, not_host=0
  Test::declWithNonNamespaceUser(&Res); PRINT_RESULT(declWithNonNamespaceUser, Res);
  Test::defWithNonNamespaceUser(&Res); PRINT_RESULT(defWithNonNamespaceUser, Res);
  Test::onDecl(&Res); PRINT_RESULT(onDecl, Res);
  Test::onDef(&Res); PRINT_RESULT(onDef, Res);
  Test::onDeclOuterDef(&Res); PRINT_RESULT(onDeclOuterDef, Res);
  Test::declOnInnerDef(&Res); PRINT_RESULT(declOnInnerDef, Res);
  Test::declOnOuterDef(&Res); PRINT_RESULT(declOnOuterDef, Res);
  Test::onDeclOnInnerDef(&Res); PRINT_RESULT(onDeclOnInnerDef, Res);
  Test::onDeclOnOuterDef(&Res); PRINT_RESULT(onDeclOnOuterDef, Res);
  // EXE-HOST-NEXT: declWithNonNamespaceUser: host=1, not_host=0
  // EXE-HOST-NEXT:  defWithNonNamespaceUser: host=1, not_host=0
  // EXE-HOST-NEXT:                   onDecl: host=1, not_host=0
  // EXE-HOST-NEXT:                    onDef: host=1, not_host=0
  // EXE-HOST-NEXT:           onDeclOuterDef: host=1, not_host=0
  // EXE-HOST-NEXT:           declOnInnerDef: host=1, not_host=0
  // EXE-HOST-NEXT:           declOnOuterDef: host=1, not_host=0
  // EXE-HOST-NEXT:         onDeclOnInnerDef: host=1, not_host=0
  // EXE-HOST-NEXT:         onDeclOnOuterDef: host=1, not_host=0
  //  EXE-OFF-NEXT: declWithNonNamespaceUser: host=0, not_host=1
  //  EXE-OFF-NEXT:  defWithNonNamespaceUser: host=0, not_host=1
  //  EXE-OFF-NEXT:                   onDecl: host=0, not_host=1
  //  EXE-OFF-NEXT:                    onDef: host=0, not_host=1
  //  EXE-OFF-NEXT:           onDeclOuterDef: host=0, not_host=1
  //  EXE-OFF-NEXT:           declOnInnerDef: host=0, not_host=1
  //  EXE-OFF-NEXT:           declOnOuterDef: host=0, not_host=1
  //  EXE-OFF-NEXT:         onDeclOnInnerDef: host=0, not_host=1
  //  EXE-OFF-NEXT:         onDeclOnOuterDef: host=0, not_host=1
  #pragma acc parallel num_gangs(1) copyout(Res)
  Test::declWithNonNamespaceUser(&Res); PRINT_RESULT(declWithNonNamespaceUser, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  Test::defWithNonNamespaceUser(&Res); PRINT_RESULT(defWithNonNamespaceUser, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  Test::onDecl(&Res); PRINT_RESULT(onDecl, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  Test::onDef(&Res); PRINT_RESULT(onDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  Test::onDeclOuterDef(&Res); PRINT_RESULT(onDeclOuterDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  Test::declOnInnerDef(&Res); PRINT_RESULT(declOnInnerDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  Test::declOnOuterDef(&Res); PRINT_RESULT(declOnOuterDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  Test::onDeclOnInnerDef(&Res); PRINT_RESULT(onDeclOnInnerDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  Test::onDeclOnOuterDef(&Res); PRINT_RESULT(onDeclOnOuterDef, Res);
  return 0;
}
// EXE-NOT: {{.}}

#else

// These are not used in this compilation unit, so they need routine directives.
#pragma acc routine seq
void Test::declWithNonNamespaceUser(Result *Res) { WRITE_RESULT(Res); }
#pragma acc routine seq
void Test::declWithAfterNamespaceUser(Result *Res) { WRITE_RESULT(Res); }

// This is used in Test::user's compute region in this compilation unit, so it
// should have an implicit routine directive.
//
// FIXME: Because Test::user's definition is inline, its compute region use of
// Test::declWithNamespaceUser does not imply an omp declare target for
// Test::declWithNamespaceUser under Clang's current OpenMP support unless
// Test::user is used in the same compilation unit.  Either that shouldn't be
// required, or Clang's OpenACC support should diagnose it in the Clang front
// end.
void use_user() { Test::user(); }
void Test::declWithNamespaceUser(Result *Res) { WRITE_RESULT(Res); }

// This already has an explicit routine directive in this compilation unit.
void Test::onDecl(Result *Res) { WRITE_RESULT(Res); }

#endif
