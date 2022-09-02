// Clang used to fail an assertion for function calls at file scope because
// the OpenACC routine directive analysis assumed functions are called only
// within other functions, as in C.
//
// RUN: %{acc-check-dmp-cxx}
// RUN: %{acc-check-prt-cxx}
// RUN: %{acc-check-exe-cxx}
//
// END.

/* expected-no-diagnostics */

#include <stdio.h>

// EXE-NOT: {{.}}

// PRT: int HostFunc() {
// PRT: }{{$}}
int HostFunc() { printf("HostFunc called\n"); return 0; }

//      DMP: FunctionDecl {{.*}} SeqFunc 'int ()'
//  DMP-NOT: FunctionDecl
//  DMP-NOT: CXXConstructorDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
//    PRT-NEXT: int SeqFunc() {
//         PRT: }{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
#pragma acc routine seq
int SeqFunc() { printf("SeqFunc called\n"); return 0; }

struct HostCtor { HostCtor() { printf("HostCtor ctor called\n"); } };

//      DMP: CXXConstructorDecl {{.*}} SeqCtor
//      DMP: CXXConstructorDecl {{.*}} prev {{.*}} SeqCtor
//  DMP-NOT: FunctionDecl
//  DMP-NOT: CXXConstructorDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//         PRT: struct SeqCtor {
//         PRT: };
//  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
//    PRT-NEXT: SeqCtor::SeqCtor() {
//         PRT: }{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
struct SeqCtor { SeqCtor(); };
#pragma acc routine seq
SeqCtor::SeqCtor() { printf("SeqCtor ctor called\n"); }

//      EXE: HostFunc called
// EXE-NEXT: SeqFunc called
// EXE-NEXT: HostCtor ctor called
// EXE-NEXT: SeqCtor ctor called
int HostFunc_fileScopeUser = HostFunc();
int SeqFunc_fileScopeUser = SeqFunc();
HostCtor hostCtor;
SeqCtor seqCtor;

int main() {
  return 0;
}

// EXE-NOT: {{.}}