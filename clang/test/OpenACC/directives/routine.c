// Check "acc routine".

// RUN: %data tgts {
// RUN:   (run-if=                tgt-cflags=                                     host-or-dev=HOST verify=expected)
// RUN:   (run-if=%run-if-x86_64  tgt-cflags=-fopenmp-targets=%run-x86_64-triple  host-or-dev=DEV  verify=expected)
// RUN:   (run-if=%run-if-ppc64le tgt-cflags=-fopenmp-targets=%run-ppc64le-triple host-or-dev=DEV  verify=expected)
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags=-fopenmp-targets=%run-nvptx64-triple host-or-dev=DEV  verify=nvptx64 )
// RUN:   (run-if=%run-if-amdgcn  tgt-cflags=-fopenmp-targets=%run-amdgcn-triple  host-or-dev=DEV  verify=expected)
// RUN: }

// Check -ast-dump before and after AST serialization.
//
// RUN: %clang -Xclang -verify -Xclang -ast-dump -fsyntax-only \
// RUN:        %acc-includes -fopenacc %s \
// RUN: | FileCheck -check-prefixes=DMP %s \
// RUN:             -implicit-check-not=ACCRoutineDeclAttr \
// RUN:             -implicit-check-not=OMPDeclareTargetDeclAttr
// RUN: %clang -Xclang -verify -fopenacc -emit-ast %acc-includes -o %t.ast %s
// RUN: %clang_cc1 -ast-dump-all %t.ast \
// RUN: | FileCheck -check-prefixes=DMP %s \
// RUN:             -implicit-check-not=ACCRoutineDeclAttr \
// RUN:             -implicit-check-not=OMPDeclareTargetDeclAttr

// Check -ast-print and -fopenacc[-ast]-print.
//
// Strip comments and blank lines so checking -fopenacc-print output is easier.
// RUN: echo "// expected""-no-diagnostics" > %t-acc.c
// RUN: grep -v '^ *\(//.*\)\?$' %s | sed 's, *\(//.*\)\?$,,' >> %t-acc.c
//
// RUN: %data prt-args {
// RUN:   (prt='-Xclang -ast-print -fsyntax-only -fopenacc' prt-chk=PRT,PRT-A,PRT-AST,PRT-AST-A)
// RUN:   (prt=-fopenacc-ast-print=acc                      prt-chk=PRT,PRT-A,PRT-AST,PRT-AST-A)
// RUN:   (prt=-fopenacc-ast-print=omp                      prt-chk=PRT,PRT-O,PRT-AST,PRT-AST-O)
// RUN:   (prt=-fopenacc-ast-print=acc-omp                  prt-chk=PRT,PRT-A,PRT-AO,PRT-AST,PRT-AST-A,PRT-AST-AO)
// RUN:   (prt=-fopenacc-ast-print=omp-acc                  prt-chk=PRT,PRT-O,PRT-OA,PRT-AST,PRT-AST-O,PRT-AST-OA)
// RUN:   (prt=-fopenacc-print=acc                          prt-chk=PRT,PRT-A,PRT-SRC,PRT-SRC-A)
// RUN:   (prt=-fopenacc-print=omp                          prt-chk=PRT,PRT-O,PRT-SRC,PRT-SRC-O)
// RUN:   (prt=-fopenacc-print=acc-omp                      prt-chk=PRT,PRT-A,PRT-AO,PRT-SRC,PRT-SRC-A,PRT-SRC-AO)
// RUN:   (prt=-fopenacc-print=omp-acc                      prt-chk=PRT,PRT-O,PRT-OA,PRT-SRC,PRT-SRC-O,PRT-SRC-OA)
// RUN: }
// RUN: %for prt-args {
// RUN:   %clang -Xclang -verify %[prt] %acc-includes %t-acc.c \
// RUN:     -Wno-openacc-omp-map-ompx-hold \
// RUN:   | FileCheck -check-prefixes=%[prt-chk] %s
// RUN: }

// Check -ast-print after AST serialization.
//
// Some data related to printing (where to print comments about discarded
// directives) is serialized and deserialized, so it's worthwhile to try all
// OpenACC printing modes.
//
// RUN: %clang -Xclang -verify -fopenacc -emit-ast %acc-includes %t-acc.c \
// RUN:        -o %t.ast
// RUN: %for prt-args {
// RUN:   %clang %[prt] %t.ast 2>&1 \
// RUN:   | FileCheck -check-prefixes=%[prt-chk] %s
// RUN: }

// Can we print the OpenMP source code, compile, and run it successfully?
//
// RUN: %for tgts {
// RUN:   %[run-if] %clang -Xclang -verify -fopenacc-print=omp %acc-includes \
// RUN:     -Wno-openacc-omp-map-ompx-hold %s > %t-omp.c
// RUN:   %[run-if] echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN:   %[run-if] %clang -fopenacc-print=omp %acc-includes -DCOMPILE_OTHER \
// RUN:                    %s > %t-other-omp.c
// RUN:   %[run-if] %clang -fopenmp %fopenmp-version %[tgt-cflags] \
// RUN:                    -DCOMPILE_OTHER %acc-includes -c -o %t-other.o \
// RUN:                    %t-other-omp.c
// RUN:   %[run-if] %clang -Xclang -verify=%[verify] -fopenmp %fopenmp-version \
// RUN:                    %[tgt-cflags] %acc-includes -o %t.exe %t-omp.c \
// RUN:                    %t-other.o
// RUN:   %[run-if] %t.exe > %t.out 2>&1
// RUN:   %[run-if] FileCheck -input-file %t.out %s \
// RUN:     -strict-whitespace -check-prefixes=EXE,EXE-%[host-or-dev]
// RUN: }

// Check execution with normal compilation.
//
// RUN: %for tgts {
// RUN:   %[run-if] %clang -fopenacc %[tgt-cflags] %acc-includes -c \
// RUN:                    -o %t-other.o -DCOMPILE_OTHER %s
// RUN:   %[run-if] %clang -Xclang -verify=%[verify] -fopenacc %[tgt-cflags] \
// RUN:                    %acc-includes -o %t.exe %s %t-other.o
// RUN:   %[run-if] %t.exe > %t.out 2>&1
// RUN:   %[run-if] FileCheck -input-file %t.out %s \
// RUN:     -strict-whitespace -check-prefixes=EXE,EXE-%[host-or-dev]
// RUN: }

// END.

// expected-no-diagnostics

// FIXME: Clang produces spurious warning diagnostics for nvptx64 offload.  This
// issue is not limited to Clacc and is present upstream:
// nvptx64-warning@*:* 0+ {{Linking two modules of different data layouts}}
// nvptx64-warning@*:* 0+ {{Linking two modules of different target triples}}

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

// This case is special because codegen is performed on the function definition,
// but the OpenACC routine directive hasn't been seen yet at that point.
//
// Thus, the OpenACC routine directive's attribute must be implied on the
// function definition in the in-memory AST in order for traditional OpenACC
// compilation to generate device code.
//
// FIXME: The OpenMP declare target directive must also be printed explicitly by
// OpenACC source-to-source mode, or the same problem would occur during OpenMP
// compilation.  The problem here is that upstream Clang does not appear to
// fully support OpenMP 5.1, sec. 2.14.7 "Declare Target Directive", p. 212,
// L15-16:
// "If a function appears in a to clause in the same compilation unit in which
// the definition of the function occurs then a device-specific version of the
// function is created."
//
// In either case, omitting the attribute/directive would cause a linking error.
//
//      DMP: FunctionDecl [[#%#x,defOnDecl:]] {{.*}} defOnDecl 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> Implicit MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//      DMP: FunctionDecl {{.*}} prev [[#defOnDecl]] {{.*}} defOnDecl 'void (Result *)'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Seq OMPNodeKind=OMPDeclareTargetDecl{{$}}
// DMP-NEXT:   OMPDeclareTargetDeclAttr {{.*}}> MT_To DT_Any [[#]] IsOpenACCTranslation{{$}}
//
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//    PRT-NEXT: void defOnDecl(Result *Res) {
//         PRT: }{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
//  PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//  PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
// PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
//    PRT-NEXT: void defOnDecl(Result *);
//  PRT-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
void defOnDecl(Result *Res) { WRITE_RESULT(Res); }
#pragma acc routine seq
void defOnDecl(Result *);

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
//      PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
//     PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//      PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
//     PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
//        PRT-NEXT: struct fnDefAddsType *fnDefAddsType(Result *Res) {
//             PRT: }{{$}}
//    PRT-AST-NEXT: ;
//  PRT-SRC-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-SRC-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
#pragma acc routine seq
struct fnDefAddsType *fnDefAddsType(Result *Res) { // type decl not in function's DeclGroup
  WRITE_RESULT(Res);
  return 0;
}

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
//      PRT-A-NEXT: {{^ *}}#pragma acc routine seq{{$}}
//     PRT-AO-NEXT: {{^ *}}// #pragma omp declare target{{$}}
//      PRT-O-NEXT: {{^ *}}#pragma omp declare target{{$}}
//     PRT-OA-NEXT: {{^ *}}// #pragma acc routine seq{{$}}
//    PRT-SRC-NEXT: struct fnDeclAddsType *fnDeclAddsType(Result *);
//  PRT-SRC-O-NEXT: {{^ *}}#pragma omp end declare target{{$}}
// PRT-SRC-AO-NEXT: {{^ *}}// #pragma omp end declare target{{$}}
//    PRT-SRC-NEXT: struct fnDeclAddsType *fnDeclAddsType(Result *Res) {
//    PRT-AST-NEXT: struct fnDeclAddsType *fnDeclAddsType(Result *), *fnDeclAddsType(Result *Res) {
//             PRT: }{{$}}
//    PRT-AST-NEXT: ;
#pragma acc routine seq
struct fnDeclAddsType *fnDeclAddsType(Result *); // type decl in function's Declgroup
struct fnDeclAddsType *fnDeclAddsType(Result *Res) { // type isn't new so has no decl
  WRITE_RESULT(Res);
  return 0;
}

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
  // EXE-NEXT:      defOnDecl: host=1, not_host=0
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
  defOnDecl(&Res); PRINT_RESULT(defOnDecl, Res);
  onDeclOnDecl(&Res); PRINT_RESULT(onDeclOnDecl, Res);
  onDeclOnDef(&Res); PRINT_RESULT(onDeclOnDef, Res);
  onDefOnDecl(&Res); PRINT_RESULT(onDefOnDecl, Res);
  fnDefAddsType(&Res); PRINT_RESULT(fnDefAddsType, Res);
  fnDeclAddsType(&Res); PRINT_RESULT(fnDeclAddsType, Res);
  // EXE-HOST-NEXT:         onDecl: host=1, not_host=0
  // EXE-HOST-NEXT:          onDef: host=1, not_host=0
  // EXE-HOST-NEXT:     onDeclDecl: host=1, not_host=0
  // EXE-HOST-NEXT:      onDeclDef: host=1, not_host=0
  // EXE-HOST-NEXT:      onDefDecl: host=1, not_host=0
  // EXE-HOST-NEXT:     declOnDecl: host=1, not_host=0
  // EXE-HOST-NEXT:      declOnDef: host=1, not_host=0
  // EXE-HOST-NEXT:      defOnDecl: host=1, not_host=0
  // EXE-HOST-NEXT:   onDeclOnDecl: host=1, not_host=0
  // EXE-HOST-NEXT:    onDeclOnDef: host=1, not_host=0
  // EXE-HOST-NEXT:    onDefOnDecl: host=1, not_host=0
  // EXE-HOST-NEXT:  fnDefAddsType: host=1, not_host=0
  // EXE-HOST-NEXT: fnDeclAddsType: host=1, not_host=0
  //  EXE-DEV-NEXT:         onDecl: host=0, not_host=1
  //  EXE-DEV-NEXT:          onDef: host=0, not_host=1
  //  EXE-DEV-NEXT:     onDeclDecl: host=0, not_host=1
  //  EXE-DEV-NEXT:      onDeclDef: host=0, not_host=1
  //  EXE-DEV-NEXT:      onDefDecl: host=0, not_host=1
  //  EXE-DEV-NEXT:     declOnDecl: host=0, not_host=1
  //  EXE-DEV-NEXT:      declOnDef: host=0, not_host=1
  //  EXE-DEV-NEXT:      defOnDecl: host=0, not_host=1
  //  EXE-DEV-NEXT:   onDeclOnDecl: host=0, not_host=1
  //  EXE-DEV-NEXT:    onDeclOnDef: host=0, not_host=1
  //  EXE-DEV-NEXT:    onDefOnDecl: host=0, not_host=1
  //  EXE-DEV-NEXT:  fnDefAddsType: host=0, not_host=1
  //  EXE-DEV-NEXT: fnDeclAddsType: host=0, not_host=1
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
  defOnDecl(&Res); PRINT_RESULT(defOnDecl, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  onDeclOnDecl(&Res); PRINT_RESULT(onDeclOnDecl, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  onDeclOnDef(&Res); PRINT_RESULT(onDeclOnDef, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  onDefOnDecl(&Res); PRINT_RESULT(onDefOnDecl, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  fnDefAddsType(&Res); PRINT_RESULT(fnDefAddsType, Res);
  #pragma acc parallel num_gangs(1) copyout(Res)
  fnDeclAddsType(&Res); PRINT_RESULT(fnDeclAddsType, Res);
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
