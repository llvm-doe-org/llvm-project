// Check "acc routine".

// RUN: %acc-check-dmp{                                                        \
// RUN:   clang-args: ;                                                        \
// RUN:   fc-args:    -implicit-check-not=ACCRoutineDeclAttr                   \
// RUN:               -implicit-check-not=OMPDeclareTargetDeclAttr}
// RUN: %acc-check-prt{}
//
// RUN: %acc-check-exe-compile-c{base-name: main}
// RUN: %acc-check-exe-compile-c{base-name: other; clang-args: -DCOMPILE_OTHER}
// RUN: %acc-check-exe-link{clang-args: main.o other.o}
// RUN: %acc-check-exe-run{}
// RUN: %acc-check-exe-filecheck{fc-args: -strict-whitespace}

// END.

/* expected-error 0 {{}} */

// FIXME: Clang produces spurious warning diagnostics for nvptx64 offload.  This
// issue is not limited to Clacc and is present upstream:
/* nvptx64-warning@*:* 0+ {{Linking two modules of different target triples}} */

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

// This is just so we can use PRT*-NEXT consistently from now on.
// PRT: int PrtStart;
int PrtStart;

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
//    PRT-NEXT: void onDef(Result *Res) {
//         PRT: }{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
#pragma acc routine seq
void onDef(Result *Res) { WRITE_RESULT(Res); }

//      DMP: FunctionDecl [[#%#x,onDeclDecl:]] {{.*}} onDeclDecl 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//      DMP: FunctionDecl {{.*}} prev [[#onDeclDecl]] {{.*}} onDeclDecl 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Inherited Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> Inherited MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
//    PRT-NEXT: void onDeclDecl(Result *);
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
//    PRT-NEXT: void onDeclDecl(Result *);
#pragma acc routine seq
void onDeclDecl(Result *);
void onDeclDecl(Result *);

//      DMP: FunctionDecl [[#%#x,onDeclDef:]] {{.*}} onDeclDef 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//      DMP: FunctionDecl {{.*}} prev [[#onDeclDef]] {{.*}} onDeclDef 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Inherited Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> Inherited MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
//    PRT-NEXT: void onDeclDef(Result *);
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
//    PRT-NEXT: void onDeclDef(Result *Res) {
//         PRT: }{{$}}
#pragma acc routine seq
void onDeclDef(Result *);
void onDeclDef(Result *Res) { WRITE_RESULT(Res); }

//      DMP: FunctionDecl [[#%#x,onDefDecl:]] {{.*}} onDefDecl 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//      DMP: FunctionDecl {{.*}} prev [[#onDefDecl]] {{.*}} onDefDecl 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Inherited Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> Inherited MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
//    PRT-NEXT: void onDefDecl(Result *Res) {
//         PRT: }{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
//    PRT-NEXT: void onDefDecl(Result *);
#pragma acc routine seq
void onDefDecl(Result *Res) { WRITE_RESULT(Res); }
void onDefDecl(Result *);

//      DMP: FunctionDecl [[#%#x,declOnDecl:]] {{.*}} declOnDecl 'void (Result *)'
//      DMP: FunctionDecl {{.*}} prev [[#declOnDecl]] {{.*}} declOnDecl 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//    PRT-NEXT: void declOnDecl(Result *);
//  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
//    PRT-NEXT: void declOnDecl(Result *);
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
void declOnDecl(Result *);
#pragma acc routine seq
void declOnDecl(Result *);

//      DMP: FunctionDecl [[#%#x,declOnDef:]] {{.*}} declOnDef 'void (Result *)'
//      DMP: FunctionDecl {{.*}} prev [[#declOnDef]] {{.*}} declOnDef 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//    PRT-NEXT: void declOnDef(Result *);
//  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
//    PRT-NEXT: void declOnDef(Result *Res) {
//         PRT: }{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
void declOnDef(Result *);
#pragma acc routine seq
void declOnDef(Result *Res) { WRITE_RESULT(Res); }

//      DMP: FunctionDecl [[#%#x,onDeclOnDecl:]] {{.*}} onDeclOnDecl 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//      DMP: FunctionDecl {{.*}} prev [[#onDeclOnDecl]] {{.*}} onDeclOnDecl 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
//    PRT-NEXT: void onDeclOnDecl(Result *);
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
//  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
//    PRT-NEXT: void onDeclOnDecl(Result *);
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
#pragma acc routine seq
void onDeclOnDecl(Result *);
#pragma acc routine seq
void onDeclOnDecl(Result *);

//      DMP: FunctionDecl [[#%#x,onDeclOnDecl:]] {{.*}} onDeclOnDecl 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//      DMP: FunctionDecl {{.*}} prev [[#onDeclOnDecl]] {{.*}} onDeclOnDecl 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
//    PRT-NEXT: void onDeclOnDecl(Result *);
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
//
//  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
//    PRT-NEXT: void onDeclOnDecl(Result *);
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
#pragma acc routine seq
void onDeclOnDecl(Result *);
#pragma acc routine seq
void onDeclOnDecl(Result *);

//      DMP: FunctionDecl [[#%#x,onDeclOnDef:]] {{.*}} onDeclOnDef 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//      DMP: FunctionDecl {{.*}} prev [[#onDeclOnDef]] {{.*}} onDeclOnDef 'void (Result *)'
//  DMP-NOT: FunctionDecl
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
//
//  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
//    PRT-NEXT: void onDeclOnDef(Result *Res) {
//         PRT: }{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
#pragma acc routine seq
void onDeclOnDef(Result *);
#pragma acc routine seq
void onDeclOnDef(Result *Res) { WRITE_RESULT(Res); }

//      DMP: FunctionDecl [[#%#x,onDefOnDecl:]] {{.*}} onDefOnDecl 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//      DMP: FunctionDecl {{.*}} prev [[#onDefOnDecl]] {{.*}} onDefOnDecl 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
//    PRT-NEXT: void onDefOnDecl(Result *Res) {
//         PRT: }{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
//
//  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
//    PRT-NEXT: void onDefOnDecl(Result *);
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
#pragma acc routine seq
void onDefOnDecl(Result *Res) { WRITE_RESULT(Res); }
#pragma acc routine seq
void onDefOnDecl(Result *);

// In a function prototype but not definition, Clang represents new types within
// the same DeclGroup as the function.  Thus, the routine directive
// implementation has to search the group to find the function.
//
//      DMP: FunctionDecl {{.*}} fnDefAddsType 'struct fnDefAddsType *(Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
// FIXME: AST printing is broken due to the struct.
//
//    PRT-SRC-NEXT: #if !EXE_S2S_AST_PRT
//      PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
//     PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//      PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
//     PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
//        PRT-NEXT: struct fnDefAddsType *fnDefAddsType(Result *Res) {
//             PRT: }{{$}}
//    PRT-AST-NEXT: ;
//  PRT-O-SRC-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-SRC-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
//    PRT-SRC-NEXT: #endif
#if !EXE_S2S_AST_PRT
#pragma acc routine seq
struct fnDefAddsType *fnDefAddsType(Result *Res) { // type decl not in function's DeclGroup
  WRITE_RESULT(Res);
  return 0;
}
#endif

//      DMP: FunctionDecl [[#%#x,fnDeclAddsType:]] {{.*}} fnDeclAddsType 'struct fnDeclAddsType *(Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//      DMP: FunctionDecl {{.*}} prev [[#fnDeclAddsType]] {{.*}} fnDeclAddsType 'struct fnDeclAddsType *(Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Inherited Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> Inherited MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
// FIXME: AST printing is broken due to the struct.
//
//    PRT-SRC-NEXT: #if !EXE_S2S_AST_PRT
//      PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
//     PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//      PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
//     PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
//    PRT-SRC-NEXT: struct fnDeclAddsType *fnDeclAddsType(Result *);
//  PRT-O-SRC-NEXT: {{^ *}}#pragma omp end declare target{{ *$}}
// PRT-AO-SRC-NEXT: {{^ *}}// #pragma omp end declare target{{ $}}
//    PRT-SRC-NEXT: struct fnDeclAddsType *fnDeclAddsType(Result *Res) {
//    PRT-AST-NEXT: struct fnDeclAddsType *fnDeclAddsType(Result *), *fnDeclAddsType(Result *Res) {
//             PRT: }{{$}}
//    PRT-AST-NEXT: ;
//    PRT-SRC-NEXT: #endif
#if !EXE_S2S_AST_PRT
#pragma acc routine seq
struct fnDeclAddsType *fnDeclAddsType(Result *); // type decl in function's Declgroup
struct fnDeclAddsType *fnDeclAddsType(Result *Res) { // type isn't new so has no decl
  WRITE_RESULT(Res);
  return 0;
}
#endif

// EXE-NOT: {{.}}
int main(int argc, char *argv[]) {
  Result Res;
  //      EXE:         onDecl: host=1, not_host=0
  // EXE-NEXT:          onDef: host=1, not_host=0
  // EXE-NEXT:     onDeclDecl: host=1, not_host=0
  // EXE-NEXT:      onDeclDef: host=1, not_host=0
  // EXE-NEXT:      onDefDecl: host=1, not_host=0
  // EXE-NEXT:     declOnDecl: host=1, not_host=0
  // EXE-NEXT:      declOnDef: host=1, not_host=0
  // EXE-NEXT:   onDeclOnDecl: host=1, not_host=0
  // EXE-NEXT:    onDeclOnDef: host=1, not_host=0
  // EXE-NEXT:    onDefOnDecl: host=1, not_host=0
  // EXE-NEXT:  fnDefAddsType: host=1, not_host=0
  // EXE-NEXT: fnDeclAddsType: host=1, not_host=0
  onDecl(&Res); PRINT_RESULT(onDecl, Res);
  onDef(&Res); PRINT_RESULT(onDef, Res);
  onDeclDecl(&Res); PRINT_RESULT(onDeclDecl, Res);
  onDeclDef(&Res); PRINT_RESULT(onDeclDef, Res);
  onDefDecl(&Res); PRINT_RESULT(onDefDecl, Res);
  declOnDecl(&Res); PRINT_RESULT(declOnDecl, Res);
  declOnDef(&Res); PRINT_RESULT(declOnDef, Res);
  onDeclOnDecl(&Res); PRINT_RESULT(onDeclOnDecl, Res);
  onDeclOnDef(&Res); PRINT_RESULT(onDeclOnDef, Res);
  onDefOnDecl(&Res); PRINT_RESULT(onDefOnDecl, Res);
#if !EXE_S2S_AST_PRT
  fnDefAddsType(&Res); PRINT_RESULT(fnDefAddsType, Res);
  fnDeclAddsType(&Res); PRINT_RESULT(fnDeclAddsType, Res);
#else
  printf("fnDefAddsType: host=1, not_host=0\n");
  printf("fnDeclAddsType: host=1, not_host=0\n");
#endif
  // EXE-HOST-NEXT:         onDecl: host=1, not_host=0
  // EXE-HOST-NEXT:          onDef: host=1, not_host=0
  // EXE-HOST-NEXT:     onDeclDecl: host=1, not_host=0
  // EXE-HOST-NEXT:      onDeclDef: host=1, not_host=0
  // EXE-HOST-NEXT:      onDefDecl: host=1, not_host=0
  // EXE-HOST-NEXT:     declOnDecl: host=1, not_host=0
  // EXE-HOST-NEXT:      declOnDef: host=1, not_host=0
  // EXE-HOST-NEXT:   onDeclOnDecl: host=1, not_host=0
  // EXE-HOST-NEXT:    onDeclOnDef: host=1, not_host=0
  // EXE-HOST-NEXT:    onDefOnDecl: host=1, not_host=0
  // EXE-HOST-NEXT:  fnDefAddsType: host=1, not_host=0
  // EXE-HOST-NEXT: fnDeclAddsType: host=1, not_host=0
  //  EXE-OFF-NEXT:         onDecl: host=0, not_host=1
  //  EXE-OFF-NEXT:          onDef: host=0, not_host=1
  //  EXE-OFF-NEXT:     onDeclDecl: host=0, not_host=1
  //  EXE-OFF-NEXT:      onDeclDef: host=0, not_host=1
  //  EXE-OFF-NEXT:      onDefDecl: host=0, not_host=1
  //  EXE-OFF-NEXT:     declOnDecl: host=0, not_host=1
  //  EXE-OFF-NEXT:      declOnDef: host=0, not_host=1
  //  EXE-OFF-NEXT:   onDeclOnDecl: host=0, not_host=1
  //  EXE-OFF-NEXT:    onDeclOnDef: host=0, not_host=1
  //  EXE-OFF-NEXT:    onDefOnDecl: host=0, not_host=1
  //  EXE-OFF-NEXT:  fnDefAddsType: host=0, not_host=1
  //  EXE-OFF-NEXT: fnDeclAddsType: host=0, not_host=1
  #pragma acc parallel num_gangs(1) copyout(Res)
  onDecl(&Res); PRINT_RESULT(onDecl, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  onDef(&Res); PRINT_RESULT(onDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  onDeclDecl(&Res); PRINT_RESULT(onDeclDecl, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  onDeclDef(&Res); PRINT_RESULT(onDeclDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  onDefDecl(&Res); PRINT_RESULT(onDefDecl, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  declOnDecl(&Res); PRINT_RESULT(declOnDecl, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  declOnDef(&Res); PRINT_RESULT(declOnDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  onDeclOnDecl(&Res); PRINT_RESULT(onDeclOnDecl, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  onDeclOnDef(&Res); PRINT_RESULT(onDeclOnDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  onDefOnDecl(&Res); PRINT_RESULT(onDefOnDecl, Res);
#if !EXE_S2S_AST_PRT
  #pragma acc parallel num_gangs(1) copyout(Res)
  fnDefAddsType(&Res); PRINT_RESULT(fnDefAddsType, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  fnDeclAddsType(&Res); PRINT_RESULT(fnDeclAddsType, Res);
#elif TGT_HOST
  printf("fnDefAddsType: host=1, not_host=0\n");
  printf("fnDeclAddsType: host=1, not_host=0\n");
#else
  printf("fnDefAddsType: host=0, not_host=1\n");
  printf("fnDeclAddsType: host=0, not_host=1\n");
#endif
  return 0;
}
// EXE-NOT: {{.}}

#else

#pragma acc routine seq
void onDecl(Result *Res) { WRITE_RESULT(Res); }

#pragma acc routine seq
void onDeclDecl(Result *Res) { WRITE_RESULT(Res); }

#pragma acc routine seq
void declOnDecl(Result *Res) { WRITE_RESULT(Res); }

#pragma acc routine seq
void onDeclOnDecl(Result *Res) { WRITE_RESULT(Res); }

#endif
