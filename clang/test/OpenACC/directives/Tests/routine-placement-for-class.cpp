// Check that "acc routine seq" behaves correctly when placed on a class member
// function prototype, definition, neither, or both.  Also check various ways
// "acc routine seq" might be implied by other member functions or non-member
// functions.

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

// DMP: CXXRecordDecl
//
//      PRT: class Test {
// PRT-NEXT: public:
class Test {
public:
  //----------------------------------------------------------------------------
  // Member users before usees are declared.

  // PRT-NEXT: void userBefore() {
  //      PRT: }
  void userBefore() {
    // Here, we call all member functions that have explicit routine directives
    // and only some member functions that don't to be sure their calls
    // elsewhere are sufficient to imply routine directives.
    Result Res;
    //           EXE: declAfterClassUser: host=1, not_host=0
    //      EXE-NEXT:  defAfterClassUser: host=1, not_host=0
    //      EXE-NEXT:             onDecl: host=1, not_host=0
    //      EXE-NEXT:              onDef: host=1, not_host=0
    //      EXE-NEXT:          onDeclDef: host=1, not_host=0
    //      EXE-NEXT:        onDeclOnDef: host=1, not_host=0
    declAfterClassUser(&Res); PRINT_RESULT(declAfterClassUser, Res);
    defAfterClassUser(&Res); PRINT_RESULT(defAfterClassUser, Res);
    onDecl(&Res); PRINT_RESULT(onDecl, Res);
    onDef(&Res); PRINT_RESULT(onDef, Res);
    onDeclDef(&Res); PRINT_RESULT(onDeclDef, Res);
    onDeclOnDef(&Res); PRINT_RESULT(onDeclOnDef, Res);
    // EXE-HOST-NEXT: declAfterClassUser: host=1, not_host=0
    // EXE-HOST-NEXT:  defAfterClassUser: host=1, not_host=0
    // EXE-HOST-NEXT:             onDecl: host=1, not_host=0
    // EXE-HOST-NEXT:              onDef: host=1, not_host=0
    // EXE-HOST-NEXT:          onDeclDef: host=1, not_host=0
    // EXE-HOST-NEXT:        onDeclOnDef: host=1, not_host=0
    //  EXE-OFF-NEXT: declAfterClassUser: host=0, not_host=1
    //  EXE-OFF-NEXT:  defAfterClassUser: host=0, not_host=1
    //  EXE-OFF-NEXT:             onDecl: host=0, not_host=1
    //  EXE-OFF-NEXT:              onDef: host=0, not_host=1
    //  EXE-OFF-NEXT:          onDeclDef: host=0, not_host=1
    //  EXE-OFF-NEXT:        onDeclOnDef: host=0, not_host=1
    #pragma acc parallel num_gangs(1) copyout(Res)
    declAfterClassUser(&Res); PRINT_RESULT(declAfterClassUser, Res);
    #pragma acc parallel num_gangs(1) copyout(Res)
    defAfterClassUser(&Res); PRINT_RESULT(defAfterClassUser, Res);
    #pragma acc parallel num_gangs(1) copyout(Res)
    onDecl(&Res); PRINT_RESULT(onDecl, Res);
    #pragma acc parallel num_gangs(1) copyout(Res)
    onDef(&Res); PRINT_RESULT(onDef, Res);
    #pragma acc parallel num_gangs(1) copyout(Res)
    onDeclDef(&Res); PRINT_RESULT(onDeclDef, Res);
    #pragma acc parallel num_gangs(1) copyout(Res)
    onDeclOnDef(&Res); PRINT_RESULT(onDeclOnDef, Res);
  }

  //      DMP: CXXMethodDecl {{.*}} accUserBefore 'void (Result *)'
  //  DMP-NOT: CXXMethodDecl
  //      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
  // DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
  //    PRT-NEXT: void accUserBefore(Result *Res) {
  //         PRT: }
  //  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
  #pragma acc routine seq
  void accUserBefore(Result *Res) {
    // EXE-HOST-NEXT: accUserBefore: host=1, not_host=0
    //  EXE-OFF-NEXT: accUserBefore: host=0, not_host=1
    defAfterClassAccUser(Res);
  }

  //----------------------------------------------------------------------------
  // Usees with routine directives implied only by other members.

  //     DMP: CXXMethodDecl {{.*}} declAfterClassUser 'void (Result *)'
  // DMP-NOT: CXXMethodDecl
  //     DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
  // DMP-NOT:   OMPDeclareTargetDeclAttr
  //
  // PRT-NEXT: void declAfterClassUser(Result *);
  void declAfterClassUser(Result *);

  //     DMP: CXXMethodDecl {{.*}} declBeforeClassUser 'void (Result *)'
  // DMP-NOT: CXXMethodDecl
  //     DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
  // DMP-NOT:   OMPDeclareTargetDeclAttr
  //
  // PRT-NEXT: void declBeforeClassUser(Result *);
  void declBeforeClassUser(Result *);

  //     DMP: CXXMethodDecl {{.*}} declWithAfterClassUser 'void (Result *)'
  // DMP-NOT: CXXMethodDecl
  //     DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
  // DMP-NOT:   OMPDeclareTargetDeclAttr
  //
  // PRT-NEXT: void declWithAfterClassUser(Result *);
  void declWithAfterClassUser(Result *);

  //     DMP: CXXMethodDecl {{.*}} defAfterClassUser 'void (Result *)'
  // DMP-NOT: CXXMethodDecl
  //     DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
  // DMP-NOT:   OMPDeclareTargetDeclAttr
  //
  // PRT-NEXT: void defAfterClassUser(Result *Res) {
  //      PRT: }{{$}}
  void defAfterClassUser(Result *Res) { WRITE_RESULT(Res); }

  //     DMP: CXXMethodDecl {{.*}} defAfterClassAccUser 'void (Result *)'
  // DMP-NOT: CXXMethodDecl
  //     DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
  // DMP-NOT:   OMPDeclareTargetDeclAttr
  //
  // PRT-NEXT: void defAfterClassAccUser(Result *Res) {
  //      PRT: }{{$}}
  void defAfterClassAccUser(Result *Res) { WRITE_RESULT(Res); }

  //     DMP: CXXMethodDecl {{.*}} defBeforeClassUser 'void (Result *)'
  // DMP-NOT: CXXMethodDecl
  //     DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
  // DMP-NOT:   OMPDeclareTargetDeclAttr
  //
  // PRT-NEXT: void defBeforeClassUser(Result *Res) {
  //      PRT: }{{$}}
  void defBeforeClassUser(Result *Res) { WRITE_RESULT(Res); }

  //     DMP: CXXMethodDecl {{.*}} defBeforeClassAccUser 'void (Result *)'
  // DMP-NOT: CXXMethodDecl
  //     DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
  // DMP-NOT:   OMPDeclareTargetDeclAttr
  //
  // PRT-NEXT: void defBeforeClassAccUser(Result *Res) {
  //      PRT: }{{$}}
  void defBeforeClassAccUser(Result *Res) { WRITE_RESULT(Res); }

  //     DMP: CXXMethodDecl {{.*}} defWithAfterClassUser 'void (Result *)'
  // DMP-NOT: CXXMethodDecl
  //     DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
  // DMP-NOT:   OMPDeclareTargetDeclAttr
  //
  // PRT-NEXT: void defWithAfterClassUser(Result *Res) {
  //      PRT: }{{$}}
  void defWithAfterClassUser(Result *Res) { WRITE_RESULT(Res); }

  //----------------------------------------------------------------------------
  // Usees with routine directives implied only by non-members.

  //     DMP: CXXMethodDecl {{.*}} declWithNonClassUser 'void (Result *)'
  // DMP-NOT: CXXMethodDecl
  //     DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
  // DMP-NOT:   OMPDeclareTargetDeclAttr
  //
  // PRT-NEXT: void declWithNonClassUser(Result *);
  void declWithNonClassUser(Result *);

  //     DMP: CXXMethodDecl {{.*}} defWithNonClassUser 'void (Result *)'
  // DMP-NOT: CXXMethodDecl
  //     DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP{{$}}
  // DMP-NOT:   OMPDeclareTargetDeclAttr
  //
  // PRT-NEXT: void defWithNonClassUser(Result *Res) {
  //      PRT: }{{$}}
  void defWithNonClassUser(Result *Res) { WRITE_RESULT(Res); }

  //----------------------------------------------------------------------------
  // Usees with explicit routine directives.

  //      DMP: CXXMethodDecl {{.*}} onDecl 'void (Result *)'
  //  DMP-NOT: CXXMethodDecl
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

  //      DMP: CXXMethodDecl {{.*}} onDef 'void (Result *)'
  //  DMP-NOT: CXXMethodDecl
  //      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
  // DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
  //    PRT-NEXT: void onDef(Result *Res) {
  //         PRT: }{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
  #pragma acc routine seq
  void onDef(Result *Res) { WRITE_RESULT(Res); }

  //      DMP: CXXMethodDecl [[#%#x,onDeclDef:]] {{.*}} onDeclDef 'void (Result *)'
  //  DMP-NOT: CXXMethodDecl
  //      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
  // DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
  //    PRT-NEXT: void onDeclDef(Result *);
  //  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
  #pragma acc routine seq
  void onDeclDef(Result *);

  // DMP: CXXMethodDecl [[#%#x,declOnDef:]] {{.*}} declOnDef 'void (Result *)'
  //
  // PRT-NEXT: void declOnDef(Result *);
  void declOnDef(Result *);

  //      DMP: CXXMethodDecl [[#%#x,onDeclOnDef:]] {{.*}} onDeclOnDef 'void (Result *)'
  //  DMP-NOT: CXXMethodDecl
  //      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
  // DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
  //    PRT-NEXT: void onDeclOnDef(Result *);
  //  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
  #pragma acc routine seq
  void onDeclOnDef(Result *);

  //----------------------------------------------------------------------------
  // Member users after usees are declared.

  // PRT-NEXT: void userAfter() {
  //      PRT: }
  void userAfter() {
    // Here, we call all member functions that have explicit routine directives
    // and only some member functions that don't to be sure their calls
    // elsewhere are sufficient to imply routine directives.
    Result Res;
    //      EXE-NEXT: declBeforeClassUser: host=1, not_host=0
    //      EXE-NEXT:  defBeforeClassUser: host=1, not_host=0
    //      EXE-NEXT:              onDecl: host=1, not_host=0
    //      EXE-NEXT:               onDef: host=1, not_host=0
    //      EXE-NEXT:           onDeclDef: host=1, not_host=0
    //      EXE-NEXT:         onDeclOnDef: host=1, not_host=0
    declBeforeClassUser(&Res); PRINT_RESULT(declBeforeClassUser, Res);
    defBeforeClassUser(&Res); PRINT_RESULT(defBeforeClassUser, Res);
    onDecl(&Res); PRINT_RESULT(onDecl, Res);
    onDef(&Res); PRINT_RESULT(onDef, Res);
    onDeclDef(&Res); PRINT_RESULT(onDeclDef, Res);
    onDeclOnDef(&Res); PRINT_RESULT(onDeclOnDef, Res);
    // EXE-HOST-NEXT: declBeforeClassUser: host=1, not_host=0
    // EXE-HOST-NEXT:  defBeforeClassUser: host=1, not_host=0
    // EXE-HOST-NEXT:              onDecl: host=1, not_host=0
    // EXE-HOST-NEXT:               onDef: host=1, not_host=0
    // EXE-HOST-NEXT:           onDeclDef: host=1, not_host=0
    // EXE-HOST-NEXT:         onDeclOnDef: host=1, not_host=0
    //  EXE-OFF-NEXT: declBeforeClassUser: host=0, not_host=1
    //  EXE-OFF-NEXT:  defBeforeClassUser: host=0, not_host=1
    //  EXE-OFF-NEXT:              onDecl: host=0, not_host=1
    //  EXE-OFF-NEXT:               onDef: host=0, not_host=1
    //  EXE-OFF-NEXT:           onDeclDef: host=0, not_host=1
    //  EXE-OFF-NEXT:         onDeclOnDef: host=0, not_host=1
    #pragma acc parallel num_gangs(1) copyout(Res)
    declBeforeClassUser(&Res); PRINT_RESULT(declBeforeClassUser, Res);
    #pragma acc parallel num_gangs(1) copyout(Res)
    defBeforeClassUser(&Res); PRINT_RESULT(defBeforeClassUser, Res);
    #pragma acc parallel num_gangs(1) copyout(Res)
    onDecl(&Res); PRINT_RESULT(onDecl, Res);
    #pragma acc parallel num_gangs(1) copyout(Res)
    onDef(&Res); PRINT_RESULT(onDef, Res);
    #pragma acc parallel num_gangs(1) copyout(Res)
    onDeclDef(&Res); PRINT_RESULT(onDeclDef, Res);
    #pragma acc parallel num_gangs(1) copyout(Res)
    onDeclOnDef(&Res); PRINT_RESULT(onDeclOnDef, Res);
  }

  //      DMP: CXXMethodDecl {{.*}} accUserAfter 'void (Result *)'
  //  DMP-NOT: CXXMethodDecl
  //      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
  // DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
  //    PRT-NEXT: void accUserAfter(Result *Res) {
  //         PRT: }
  //  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
  #pragma acc routine seq
  void accUserAfter(Result *Res) {
    // EXE-HOST-NEXT: accUserAfter: host=1, not_host=0
    //  EXE-OFF-NEXT: accUserAfter: host=0, not_host=1
    defBeforeClassAccUser(Res);
  }

  // PRT-NEXT: void userAfterClass();
  void userAfterClass();
}; // PRT-NEXT: };

// PRT-SRC-NEXT: #ifndef COMPILE_OTHER
#ifndef COMPILE_OTHER

//      DMP: CXXMethodDecl {{.*}} prev [[#declOnDef]] {{.*}} declOnDef 'void (Result *)'
//  DMP-NOT: CXXMethodDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
//    PRT-NEXT: void Test::declOnDef(Result *Res) {
//         PRT: }{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
#pragma acc routine seq
void Test::declOnDef(Result *Res) { WRITE_RESULT(Res); }

// PRT-NEXT: void Test::userAfterClass() {
//      PRT: }{{$}}
void Test::userAfterClass() {
  // Here, we call all member functions that have explicit routine directives
  // and only some member functions that don't to be sure their calls
  // elsewhere are sufficient to imply routine directives.
  Result Res;
  //      EXE-NEXT: declWithAfterClassUser: host=1, not_host=0
  //      EXE-NEXT:  defWithAfterClassUser: host=1, not_host=0
  //      EXE-NEXT:                 onDecl: host=1, not_host=0
  //      EXE-NEXT:                  onDef: host=1, not_host=0
  //      EXE-NEXT:              onDeclDef: host=1, not_host=0
  //      EXE-NEXT:              declOnDef: host=1, not_host=0
  //      EXE-NEXT:            onDeclOnDef: host=1, not_host=0
  declWithAfterClassUser(&Res); PRINT_RESULT(declWithAfterClassUser, Res);
  defWithAfterClassUser(&Res); PRINT_RESULT(defWithAfterClassUser, Res);
  onDecl(&Res); PRINT_RESULT(onDecl, Res);
  onDef(&Res); PRINT_RESULT(onDef, Res);
  onDeclDef(&Res); PRINT_RESULT(onDeclDef, Res);
  declOnDef(&Res); PRINT_RESULT(declOnDef, Res);
  onDeclOnDef(&Res); PRINT_RESULT(onDeclOnDef, Res);
  // EXE-HOST-NEXT: declWithAfterClassUser: host=1, not_host=0
  // EXE-HOST-NEXT:  defWithAfterClassUser: host=1, not_host=0
  // EXE-HOST-NEXT:                 onDecl: host=1, not_host=0
  // EXE-HOST-NEXT:                  onDef: host=1, not_host=0
  // EXE-HOST-NEXT:              onDeclDef: host=1, not_host=0
  // EXE-HOST-NEXT:              declOnDef: host=1, not_host=0
  // EXE-HOST-NEXT:            onDeclOnDef: host=1, not_host=0
  //  EXE-OFF-NEXT: declWithAfterClassUser: host=0, not_host=1
  //  EXE-OFF-NEXT:  defWithAfterClassUser: host=0, not_host=1
  //  EXE-OFF-NEXT:                 onDecl: host=0, not_host=1
  //  EXE-OFF-NEXT:                  onDef: host=0, not_host=1
  //  EXE-OFF-NEXT:              onDeclDef: host=0, not_host=1
  //  EXE-OFF-NEXT:              declOnDef: host=0, not_host=1
  //  EXE-OFF-NEXT:            onDeclOnDef: host=0, not_host=1
  #pragma acc parallel num_gangs(1) copyout(Res)
  declWithAfterClassUser(&Res); PRINT_RESULT(declWithAfterClassUser, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  defWithAfterClassUser(&Res); PRINT_RESULT(defWithAfterClassUser, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  onDecl(&Res); PRINT_RESULT(onDecl, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  onDef(&Res); PRINT_RESULT(onDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  onDeclDef(&Res); PRINT_RESULT(onDeclDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  declOnDef(&Res); PRINT_RESULT(declOnDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  onDeclOnDef(&Res); PRINT_RESULT(onDeclOnDef, Res);
}

//      DMP: CXXMethodDecl {{.*}} prev [[#onDeclDef]] {{.*}} onDeclDef 'void (Result *)'
//  DMP-NOT: CXXMethodDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Inherited Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> Inherited MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//    PRT-NEXT: void Test::onDeclDef(Result *Res) {
//         PRT: }{{$}}
void Test::onDeclDef(Result *Res) { WRITE_RESULT(Res); }

//      DMP: CXXMethodDecl {{.*}} prev [[#onDeclOnDef]] {{.*}} onDeclOnDef 'void (Result *)'
//  DMP-NOT: CXXMethodDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
//    PRT-NEXT: void Test::onDeclOnDef(Result *Res) {
//         PRT: }{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
#pragma acc routine seq
void Test::onDeclOnDef(Result *Res) { WRITE_RESULT(Res); }

// EXE-NOT: {{.}}
int main(int argc, char *argv[]) {
  // Here, we call all member functions that have explicit routine directives
  // and only some member functions that don't to be sure their calls
  // elsewhere are sufficient to imply routine directives.
  Result Res;
  Test test;
  test.userBefore();
  #pragma acc parallel num_gangs(1) copyout(Res)
  test.accUserBefore(&Res); PRINT_RESULT(accUserBefore, Res);
  test.userAfter();
  #pragma acc parallel num_gangs(1) copyout(Res)
  test.accUserAfter(&Res); PRINT_RESULT(accUserAfter, Res);
  test.userAfterClass();
  //      EXE-NEXT:        declWithNonClassUser: host=1, not_host=0
  //      EXE-NEXT:         defWithNonClassUser: host=1, not_host=0
  //      EXE-NEXT:                      onDecl: host=1, not_host=0
  //      EXE-NEXT:                       onDef: host=1, not_host=0
  //      EXE-NEXT:                   onDeclDef: host=1, not_host=0
  //      EXE-NEXT:                   declOnDef: host=1, not_host=0
  //      EXE-NEXT:                 onDeclOnDef: host=1, not_host=0
  test.declWithNonClassUser(&Res); PRINT_RESULT(declWithNonClassUser, Res);
  test.defWithNonClassUser(&Res); PRINT_RESULT(defWithNonClassUser, Res);
  test.onDecl(&Res); PRINT_RESULT(onDecl, Res);
  test.onDef(&Res); PRINT_RESULT(onDef, Res);
  test.onDeclDef(&Res); PRINT_RESULT(onDeclDef, Res);
  test.declOnDef(&Res); PRINT_RESULT(declOnDef, Res);
  test.onDeclOnDef(&Res); PRINT_RESULT(onDeclOnDef, Res);
  // EXE-HOST-NEXT: declWithNonClassUser: host=1, not_host=0
  // EXE-HOST-NEXT:  defWithNonClassUser: host=1, not_host=0
  // EXE-HOST-NEXT:               onDecl: host=1, not_host=0
  // EXE-HOST-NEXT:                onDef: host=1, not_host=0
  // EXE-HOST-NEXT:            onDeclDef: host=1, not_host=0
  // EXE-HOST-NEXT:            declOnDef: host=1, not_host=0
  // EXE-HOST-NEXT:          onDeclOnDef: host=1, not_host=0
  //  EXE-OFF-NEXT: declWithNonClassUser: host=0, not_host=1
  //  EXE-OFF-NEXT:  defWithNonClassUser: host=0, not_host=1
  //  EXE-OFF-NEXT:               onDecl: host=0, not_host=1
  //  EXE-OFF-NEXT:                onDef: host=0, not_host=1
  //  EXE-OFF-NEXT:            onDeclDef: host=0, not_host=1
  //  EXE-OFF-NEXT:            declOnDef: host=0, not_host=1
  //  EXE-OFF-NEXT:          onDeclOnDef: host=0, not_host=1
  #pragma acc parallel num_gangs(1) copyout(Res)
  test.declWithNonClassUser(&Res); PRINT_RESULT(declWithNonClassUser, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  test.defWithNonClassUser(&Res); PRINT_RESULT(defWithNonClassUser, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  test.onDecl(&Res); PRINT_RESULT(onDecl, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  test.onDef(&Res); PRINT_RESULT(onDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  test.onDeclDef(&Res); PRINT_RESULT(onDeclDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  test.declOnDef(&Res); PRINT_RESULT(declOnDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  test.onDeclOnDef(&Res); PRINT_RESULT(onDeclOnDef, Res);
  return 0;
}
// EXE-NOT: {{.}}

#else

// These are not used in this compilation unit, so they need routine directives.
#pragma acc routine seq
void Test::declWithNonClassUser(Result *Res) { WRITE_RESULT(Res); }
#pragma acc routine seq
void Test::declWithAfterClassUser(Result *Res) { WRITE_RESULT(Res); }

// These are used in Test::user*'s compute regions in this compilation unit, so
// they should have implicit routine directives.
//
// FIXME: Because Test::user*'s definitions are inline, their compute region
// uses of Test::decl{After,Before}ClassUser do not imply an omp declare target
// for Test::decl{After,Before}ClassUser under Clang's current OpenMP support
// unless Test::user* are used in the same compilation unit.  Either that
// shouldn't be required, or Clang's OpenACC support should diagnose it in the
// Clang front end.
void use_user() {
  Test test;
  test.userBefore();
  test.userAfter();
}
void Test::declAfterClassUser(Result *Res) { WRITE_RESULT(Res); }
void Test::declBeforeClassUser(Result *Res) { WRITE_RESULT(Res); }

// This already has an explicit routine directive in this compilation unit.
void Test::onDecl(Result *Res) { WRITE_RESULT(Res); }

#endif
