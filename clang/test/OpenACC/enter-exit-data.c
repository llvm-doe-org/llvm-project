// Check "acc enter data" and "acc exit data".
//
// fopenacc-structured-ref-count-omp.c checks these when "hold" is not used by
// "acc data" and "acc parallel".  Here, we focus on the normal translation
// using "hold".  exit-data-uninit.c checks the case where "acc exit data"
// occurs before runtime initialization.

// Check -ast-dump before and after AST serialization.
//
// RUN: %clang -Xclang -verify -Xclang -ast-dump -fsyntax-only -fopenacc \
// RUN:        %acc-includes %s \
// RUN: | FileCheck -check-prefix=DMP %s
// RUN: %clang -Xclang -verify -fopenacc -emit-ast -o %t.ast %acc-includes %s
// RUN: %clang_cc1 -ast-dump-all %t.ast \
// RUN: | FileCheck -check-prefixes=DMP %s

// Check -ast-print and -fopenacc[-ast]-print.
//
// RUN: %clang -Xclang -verify -Xclang -ast-print -fsyntax-only \
// RUN:        %acc-includes %s \
// RUN: | FileCheck -check-prefixes=PRT %s
//
// TODO: If lit were to support %for inside a %data, we could iterate prt-opts
// within prt-args after the first prt-args iteration, significantly shortening
// the prt-args definition.
//
// Strip comments and blank lines so checking -fopenacc-print output is easier.
// RUN: echo "// expected""-no-diagnostics" > %t-acc.c
// RUN: grep -v '^ *\(//.*\)\?$' %s | sed 's,//.*,,' >> %t-acc.c
//
// RUN: %data prt-opts {
// RUN:   (prt-opt=-fopenacc-ast-print)
// RUN:   (prt-opt=-fopenacc-print    )
// RUN: }
// RUN: %data prt-args {
// RUN:   (prt='-Xclang -ast-print -fsyntax-only -fopenacc' prt-chk=PRT-A,PRT)
// RUN:   (prt=-fopenacc-ast-print=acc                      prt-chk=PRT-A,PRT)
// RUN:   (prt=-fopenacc-ast-print=omp                      prt-chk=PRT-O,PRT)
// RUN:   (prt=-fopenacc-ast-print=acc-omp                  prt-chk=PRT-A,PRT-AO,PRT)
// RUN:   (prt=-fopenacc-ast-print=omp-acc                  prt-chk=PRT-O,PRT-OA,PRT)
// RUN:   (prt=-fopenacc-print=acc                          prt-chk=PRT-A,PRT)
// RUN:   (prt=-fopenacc-print=omp                          prt-chk=PRT-O,PRT)
// RUN:   (prt=-fopenacc-print=acc-omp                      prt-chk=PRT-A,PRT-AO,PRT)
// RUN:   (prt=-fopenacc-print=omp-acc                      prt-chk=PRT-O,PRT-OA,PRT)
// RUN: }
// RUN: %for prt-args {
// RUN:   %clang -Xclang -verify %[prt] %t-acc.c %acc-includes \
// RUN:          -Wno-openacc-omp-map-hold -Wno-openacc-omp-map-present \
// RUN:          -Wno-openacc-omp-map-no-alloc \
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
// RUN:          -Wno-openacc-omp-map-hold -Wno-openacc-omp-map-present \
// RUN:          -Wno-openacc-omp-map-no-alloc \
// RUN:   | FileCheck -check-prefixes=%[prt-chk] %s
// RUN: }

// Can we print the OpenMP source code, compile, and run it successfully?
//
// RUN: %data tgts {
// RUN:   (run-if=                tgt-cflags=                                     host-or-dev=HOST)
// RUN:   (run-if=%run-if-x86_64  tgt-cflags=-fopenmp-targets=%run-x86_64-triple  host-or-dev=DEV )
// RUN:   (run-if=%run-if-ppc64le tgt-cflags=-fopenmp-targets=%run-ppc64le-triple host-or-dev=DEV )
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags=-fopenmp-targets=%run-nvptx64-triple host-or-dev=DEV )
// RUN: }
// RUN: %for tgts {
// RUN:   %for prt-opts {
// RUN:     %[run-if] %clang -Xclang -verify %[prt-opt]=omp %acc-includes %s \
// RUN:                      > %t-omp.c \
// RUN:                      -Wno-openacc-omp-map-hold \
// RUN:                      -Wno-openacc-omp-map-present \
// RUN:                      -Wno-openacc-omp-map-no-alloc
// RUN:     %[run-if] echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN:     %[run-if] %clang -Xclang -verify -fopenmp %fopenmp-version \
// RUN:               %acc-includes -Wno-unused-function %[tgt-cflags] -o %t \
// RUN:               %t-omp.c %acc-libs
// RUN:     %[run-if] %t > %t.out 2>&1
// RUN:     %[run-if] FileCheck -input-file %t.out %s \
// RUN:                         -check-prefixes=EXE,EXE-%[host-or-dev]
// RUN:   }
// RUN: }

// Check execution with normal compilation.
//
// RUN: %for tgts {
// RUN:   %[run-if] %clang -Xclang -verify -fopenacc %acc-includes %s -o %t \
// RUN:                    %[tgt-cflags]
// RUN:   %[run-if] %t > %t.out 2>&1
// RUN:   %[run-if] FileCheck -input-file %t.out %s \
// RUN:                       -check-prefixes=EXE,EXE-%[host-or-dev]
// RUN: }

// END.

// expected-no-diagnostics

#include <openacc.h>
#include <stdbool.h>
#include <stdio.h>

#define setHostInt(Var, Val)   setHostInt_  (&(Var), Val)
#define setDeviceInt(Var, Val) setDeviceInt_(&(Var), Val)

void setHostInt_(int *p, int v) {
  *p = v;
}

void setDeviceInt_(int *p, int v) {
  #pragma acc parallel num_gangs(1)
  *p = v;
}

#define printHostInt(Var)   printHostInt_  (#Var, &Var)
#define printDeviceInt(Var) printDeviceInt_(#Var, &Var)

void printHostInt_(const char *Name, int *Var) {
  printf("    host %s %d\n", Name, *Var);
}
void printDeviceInt_(const char *Name, int *Var) {
  printf("  device %s ", Name);
  if (!acc_is_present(Var, 0))
    printf("absent");
  else {
    #pragma acc parallel num_gangs(1)
    printf("present %d", *Var);
  }
  printf("\n");
}

// EXE-NOT: {{.}}

int main() {
  //--------------------------------------------------
  // Check the case where both ref counts are initially zero: not enclosed in
  // acc data and no prior acc enter data.
  //
  // Also, check:
  // - All clause aliases, which shouldn't need to be checked further.
  // - Dumping, which shouldn't need to be checked further.
  // - Printing when not enclosed in acc data.
  // - Allocations and transfers by using all possible pairings of acc enter
  //   data and acc exit data clauses for a single variable, including cases
  //   with just one of acc enter data and acc exit data.
  //--------------------------------------------------

  // PRT: printf("ref counts initially zero\n");
  // EXE-LABEL: ref counts initially zero
  printf("ref counts initially zero\n");

  // PRT-NEXT: {
  {
    // PRT-NEXT: int ci, cico, cidl, cr, crco, crdl, co, dl;
    int ci, cico, cidl, cr, crco, crdl, co, dl;
    // PRT-SAME: {{($[[:space:]] *setHostInt_?\(.*\);)+}}
    setHostInt(ci,   10);
    setHostInt(cico, 20);
    setHostInt(cidl, 30);
    setHostInt(cr,   40);
    setHostInt(crco, 50);
    setHostInt(crdl, 60);
    setHostInt(co,   70);
    setHostInt(dl,   80);

    //      DMP: ACCEnterDataDirective
    // DMP-NEXT:   ACCCopyinClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'int'
    // DMP-NEXT:   ACCCopyinClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'cico' 'int'
    // DMP-NEXT:   ACCCopyinClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'cidl' 'int'
    // DMP-NEXT:   ACCCreateClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'cr' 'int'
    // DMP-NEXT:   ACCCreateClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'crco' 'int'
    // DMP-NEXT:   ACCCreateClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'crdl' 'int'
    // DMP-NEXT:   impl: OMPTargetEnterDataDirective
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'ci' 'int'
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'cico' 'int'
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'cidl' 'int'
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'cr' 'int'
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'crco' 'int'
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'crdl' 'int'
    //  DMP-NOT: -{{ACC|OMP}}
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc enter data copyin(ci) pcopyin(cico) present_or_copyin(cidl) create(cr) pcreate(crco) present_or_create(crdl){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target enter data map(to: ci) map(to: cico) map(to: cidl) map(alloc: cr) map(alloc: crco) map(alloc: crdl){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target enter data map(to: ci) map(to: cico) map(to: cidl) map(alloc: cr) map(alloc: crco) map(alloc: crdl){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc enter data copyin(ci) pcopyin(cico) present_or_copyin(cidl) create(cr) pcreate(crco) present_or_create(crdl){{$}}
    #pragma acc enter data copyin(ci) pcopyin(cico) present_or_copyin(cidl) create(cr) pcreate(crco) present_or_create(crdl)

    // PRT-SAME: {{($[[:space:]] *print(Host|Device)Int_?\(.*\);)+}}
    //
    //      EXE-NEXT:   host ci           10{{$}}
    //      EXE-NEXT:   host cico         20{{$}}
    //      EXE-NEXT:   host cidl         30{{$}}
    //      EXE-NEXT:   host cr           40{{$}}
    //      EXE-NEXT:   host crco         50{{$}}
    //      EXE-NEXT:   host crdl         60{{$}}
    //      EXE-NEXT:   host   co         70{{$}}
    //      EXE-NEXT:   host   dl         80{{$}}
    // EXE-HOST-NEXT: device ci   present 10{{$}}
    // EXE-HOST-NEXT: device cico present 20{{$}}
    // EXE-HOST-NEXT: device cidl present 30{{$}}
    // EXE-HOST-NEXT: device cr   present 40{{$}}
    // EXE-HOST-NEXT: device crco present 50{{$}}
    // EXE-HOST-NEXT: device crdl present 60{{$}}
    // EXE-HOST-NEXT: device   co present 70{{$}}
    // EXE-HOST-NEXT: device   dl present 80{{$}}
    //  EXE-DEV-NEXT: device ci   present 10{{$}}
    //  EXE-DEV-NEXT: device cico present 20{{$}}
    //  EXE-DEV-NEXT: device cidl present 30{{$}}
    //  EXE-DEV-NEXT: device cr   present
    //  EXE-DEV-NEXT: device crco present
    //  EXE-DEV-NEXT: device crdl present
    //  EXE-DEV-NEXT: device   co absent{{$}}
    //  EXE-DEV-NEXT: device   dl absent{{$}}
    printHostInt(ci);
    printHostInt(cico);
    printHostInt(cidl);
    printHostInt(cr);
    printHostInt(crco);
    printHostInt(crdl);
    printHostInt(co);
    printHostInt(dl);
    printDeviceInt(ci);
    printDeviceInt(cico);
    printDeviceInt(cidl);
    printDeviceInt(cr);
    printDeviceInt(crco);
    printDeviceInt(crdl);
    printDeviceInt(co);
    printDeviceInt(dl);

    // PRT-SAME: {{($[[:space:]] *setDeviceInt_?\(.*\);)+}}
    setDeviceInt(ci,   11);
    setDeviceInt(cico, 21);
    setDeviceInt(cidl, 31);
    setDeviceInt(cr,   41);
    setDeviceInt(crco, 51);
    setDeviceInt(crdl, 61);

    //      DMP: ACCExitDataDirective
    // DMP-NEXT:   ACCCopyoutClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'cico' 'int'
    // DMP-NEXT:   ACCDeleteClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'cidl' 'int'
    // DMP-NEXT:   ACCCopyoutClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'crco' 'int'
    // DMP-NEXT:   ACCDeleteClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'crdl' 'int'
    // DMP-NEXT:   ACCCopyoutClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'int'
    // DMP-NEXT:   ACCDeleteClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'dl' 'int'
    // DMP-NEXT:   impl: OMPTargetExitDataDirective
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'cico' 'int'
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'cidl' 'int'
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'crco' 'int'
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'crdl' 'int'
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'co' 'int'
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'dl' 'int'
    //  DMP-NOT: -{{ACC|OMP}}
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc exit data copyout(cico) delete(cidl) pcopyout(crco) delete(crdl) present_or_copyout(co) delete(dl){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target exit data map(from: cico) map(release: cidl) map(from: crco) map(release: crdl) map(from: co) map(release: dl){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target exit data map(from: cico) map(release: cidl) map(from: crco) map(release: crdl) map(from: co) map(release: dl){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc exit data copyout(cico) delete(cidl) pcopyout(crco) delete(crdl) present_or_copyout(co) delete(dl){{$}}
    #pragma acc exit data copyout(cico) delete(cidl) pcopyout(crco) delete(crdl) present_or_copyout(co) delete(dl)

    // PRT-SAME: {{($[[:space:]] *print(Host|Device)Int_?\(.*\);)+}}
    //
    // EXE-HOST-NEXT: host ci     11{{$}}
    // EXE-HOST-NEXT: host cico   21{{$}}
    // EXE-HOST-NEXT: host cidl   31{{$}}
    // EXE-HOST-NEXT: host cr     41{{$}}
    // EXE-HOST-NEXT: host crco   51{{$}}
    // EXE-HOST-NEXT: host crdl   61{{$}}
    // EXE-HOST-NEXT: host   co   70{{$}}
    // EXE-HOST-NEXT: host   dl   80{{$}}
    // EXE-HOST-NEXT: device ci   present 11{{$}}
    // EXE-HOST-NEXT: device cico present 21{{$}}
    // EXE-HOST-NEXT: device cidl present 31{{$}}
    // EXE-HOST-NEXT: device cr   present 41{{$}}
    // EXE-HOST-NEXT: device crco present 51{{$}}
    // EXE-HOST-NEXT: device crdl present 61{{$}}
    // EXE-HOST-NEXT: device   co present 70{{$}}
    // EXE-HOST-NEXT: device   dl present 80{{$}}
    //
    //  EXE-DEV-NEXT: host ci     10{{$}}
    //  EXE-DEV-NEXT: host cico   21{{$}}
    //  EXE-DEV-NEXT: host cidl   30{{$}}
    //  EXE-DEV-NEXT: host cr     40{{$}}
    //  EXE-DEV-NEXT: host crco   51{{$}}
    //  EXE-DEV-NEXT: host crdl   60{{$}}
    //  EXE-DEV-NEXT: host   co   70{{$}}
    //  EXE-DEV-NEXT: host   dl   80{{$}}
    //  EXE-DEV-NEXT: device ci   present 11{{$}}
    //  EXE-DEV-NEXT: device cico absent{{$}}
    //  EXE-DEV-NEXT: device cidl absent{{$}}
    //  EXE-DEV-NEXT: device cr   present 41{{$}}
    //  EXE-DEV-NEXT: device crco absent{{$}}
    //  EXE-DEV-NEXT: device crdl absent{{$}}
    //  EXE-DEV-NEXT: device   co absent{{$}}
    //  EXE-DEV-NEXT: device   dl absent{{$}}
    printHostInt(ci);
    printHostInt(cico);
    printHostInt(cidl);
    printHostInt(cr);
    printHostInt(crco);
    printHostInt(crdl);
    printHostInt(co);
    printHostInt(dl);
    printDeviceInt(ci);
    printDeviceInt(cico);
    printDeviceInt(cidl);
    printDeviceInt(cr);
    printDeviceInt(crco);
    printDeviceInt(crdl);
    printDeviceInt(co);
    printDeviceInt(dl);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Check the case where the structured ref count only is initially non-zero:
  // enclosed in acc data.
  //
  // Also, check:
  // - Printing when enclosed in acc data.
  // - Structured ref counter's suppression of allocations and transfers from
  //   acc enter data and acc exit data clauses.
  //--------------------------------------------------

  // PRT: printf("structured ref count initially zero\n");
  // EXE-LABEL: structured ref count initially zero
  printf("structured ref count initially zero\n");

  // PRT-NEXT: {
  {
    // PRT-NEXT: int x;
    int x;
    // PRT-NEXT: setHostInt
    setHostInt(x, 10);
    // DMP: ACCDataDirective
    //  PRT-A-NEXT: {{^ *}}#pragma acc data create(x){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(hold,alloc: x){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target data map(hold,alloc: x){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data create(x){{$}}
    // PRT-NEXT: {
    #pragma acc data create(x)
    {
      // PRT-NEXT: setDeviceInt
      setDeviceInt(x, 11);

      // Inc dynamic ref count to 1.
      //  PRT-A-NEXT: {{^ *}}#pragma acc enter data copyin(x){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target enter data map(to: x){{$}}
      //  PRT-O-NEXT: {{^ *}}#pragma omp target enter data map(to: x){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc enter data copyin(x){{$}}
      #pragma acc enter data copyin(x)
      // PRT-NEXT: printHostInt
      // PRT-NEXT: printDeviceInt
      // EXE-HOST-NEXT:   host x         11{{$}}
      //  EXE-DEV-NEXT:   host x         10{{$}}
      //      EXE-NEXT: device x present 11{{$}}
      printHostInt(x);
      printDeviceInt(x);

      // Dec dynamic ref count to 0.
      //  PRT-A-NEXT: {{^ *}}#pragma acc exit data copyout(x){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target exit data map(from: x){{$}}
      //  PRT-O-NEXT: {{^ *}}#pragma omp target exit data map(from: x){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc exit data copyout(x){{$}}
      #pragma acc exit data copyout(x)
      // PRT-NEXT: printHostInt
      // PRT-NEXT: printDeviceInt
      // EXE-HOST-NEXT:   host x         11{{$}}
      //  EXE-DEV-NEXT:   host x         10{{$}}
      //      EXE-NEXT: device x present 11{{$}}
      printHostInt(x);
      printDeviceInt(x);

      // Inc dynamic ref count to 1.
      //  PRT-A-NEXT: {{^ *}}#pragma acc enter data create(x){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target enter data map(alloc: x){{$}}
      //  PRT-O-NEXT: {{^ *}}#pragma omp target enter data map(alloc: x){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc enter data create(x){{$}}
      #pragma acc enter data create(x)
      // PRT-NEXT: printHostInt
      // PRT-NEXT: printDeviceInt
      // EXE-HOST-NEXT:   host x         11{{$}}
      //  EXE-DEV-NEXT:   host x         10{{$}}
      //      EXE-NEXT: device x present 11{{$}}
      printHostInt(x);
      printDeviceInt(x);

      // Dec dynamic ref count to 0.
      //  PRT-A-NEXT: {{^ *}}#pragma acc exit data delete(x){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target exit data map(release: x){{$}}
      //  PRT-O-NEXT: {{^ *}}#pragma omp target exit data map(release: x){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc exit data delete(x){{$}}
      #pragma acc exit data delete(x)
      // PRT-NEXT: printHostInt
      // PRT-NEXT: printDeviceInt
      // EXE-HOST-NEXT:   host x         11{{$}}
      //  EXE-DEV-NEXT:   host x         10{{$}}
      //      EXE-NEXT: device x present 11{{$}}
      printHostInt(x);
      printDeviceInt(x);

      // Try to dec dynamic ref count past 0.  That is, check that the
      // structured ref count matters.
      //  PRT-A-NEXT: {{^ *}}#pragma acc exit data copyout(x){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target exit data map(from: x){{$}}
      //  PRT-O-NEXT: {{^ *}}#pragma omp target exit data map(from: x){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc exit data copyout(x){{$}}
      #pragma acc exit data copyout(x)
      //  PRT-A-NEXT: {{^ *}}#pragma acc exit data delete(x){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target exit data map(release: x){{$}}
      //  PRT-O-NEXT: {{^ *}}#pragma omp target exit data map(release: x){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc exit data delete(x){{$}}
      #pragma acc exit data delete(x)
      // PRT-NEXT: printHostInt
      // PRT-NEXT: printDeviceInt
      // EXE-HOST-NEXT:   host x         11{{$}}
      //  EXE-DEV-NEXT:   host x         10{{$}}
      //      EXE-NEXT: device x present 11{{$}}
      printHostInt(x);
      printDeviceInt(x);
    } // PRT-NEXT: }
    // PRT-NEXT: printHostInt
    // PRT-NEXT: printDeviceInt
    // EXE-HOST-NEXT:   host x         11{{$}}
    // EXE-HOST-NEXT: device x present 11{{$}}
    //  EXE-DEV-NEXT:   host x         10{{$}}
    //  EXE-DEV-NEXT: device x absent{{$}}
    printHostInt(x);
    printDeviceInt(x);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Check the case where the dynamic ref counter only is initially non-zero.
  // That is, check its suppression of allocations and transfers from acc enter
  // data, acc exit data, acc data, and acc parallel.
  //--------------------------------------------------

  // EXE-LABEL: dynamic ref count initially zero
  printf("dynamic ref count initially zero\n");

  {
    int x;
    setHostInt(x, 10);

    // Inc dynamic ref count to 1.
    #pragma acc enter data create(x)
    setDeviceInt(x, 11);

    // Inc and dec structured ref count.
    #pragma acc data copy(x)
    ;
    #pragma acc parallel copy(x)
    ;
    #pragma acc parallel loop copy(x)
    for (int i = 0; i < 5; ++i)
    ;
    #pragma acc data copy(x)
    #pragma acc parallel copy(x)
    ;
    #pragma acc data copy(x)
    #pragma acc parallel loop copy(x)
    for (int i = 0; i < 5; ++i)
    ;
    // EXE-HOST-NEXT:   host x         11{{$}}
    //  EXE-DEV-NEXT:   host x         10{{$}}
    //      EXE-NEXT: device x present 11{{$}}
    printHostInt(x);
    printDeviceInt(x);

    // Inc dynamic ref count to 2.
    #pragma acc enter data create(x)
    // EXE-HOST-NEXT:   host x         11{{$}}
    //  EXE-DEV-NEXT:   host x         10{{$}}
    //      EXE-NEXT: device x present 11{{$}}
    printHostInt(x);
    printDeviceInt(x);

    // Inc dynamic ref count to 3.
    #pragma acc enter data copyin(x)
    // EXE-HOST-NEXT:   host x         11{{$}}
    //  EXE-DEV-NEXT:   host x         10{{$}}
    //      EXE-NEXT: device x present 11{{$}}
    printHostInt(x);
    printDeviceInt(x);

    // Dec dynamic ref count to 2.
    #pragma acc exit data delete(x)
    // EXE-HOST-NEXT:   host x         11{{$}}
    //  EXE-DEV-NEXT:   host x         10{{$}}
    //      EXE-NEXT: device x present 11{{$}}
    printHostInt(x);
    printDeviceInt(x);

    // Dec dynamic ref count to 1.
    #pragma acc exit data copyout(x)
    // EXE-HOST-NEXT:   host x         11{{$}}
    //  EXE-DEV-NEXT:   host x         10{{$}}
    //      EXE-NEXT: device x present 11{{$}}
    printHostInt(x);
    printDeviceInt(x);

    // Dec dynamic ref count to 0.
    #pragma acc exit data copyout(x)
    // EXE-HOST-NEXT:   host x         11{{$}}
    // EXE-HOST-NEXT: device x present 11{{$}}
    //  EXE-DEV-NEXT:   host x         11{{$}}
    //  EXE-DEV-NEXT: device x absent{{$}}
    printHostInt(x);
    printDeviceInt(x);

    // Try to dec dynamic ref count past 0.  OpenACC 3.0 says this is a runtime
    // error, but a correction has been proposed to the spec to say no action
    // instead.
    #pragma acc exit data copyout(x)
    #pragma acc exit data delete(x)
    // EXE-HOST-NEXT:   host x         11{{$}}
    // EXE-HOST-NEXT: device x present 11{{$}}
    //  EXE-DEV-NEXT:   host x         11{{$}}
    //  EXE-DEV-NEXT: device x absent{{$}}
    printHostInt(x);
    printDeviceInt(x);
  }

  //--------------------------------------------------
  // Check action suppression in the case of subarrays.
  //--------------------------------------------------

  // EXE-LABEL: subarrays
  printf("subarrays\n");

  {
    int dyn[4], str[4];
    #pragma acc enter data copyin(dyn[1:2])
    #pragma acc data copy(str[1:2])
    {
      setHostInt(dyn[0], 10);
      setHostInt(dyn[1], 20);
      setHostInt(dyn[2], 30);
      setHostInt(dyn[3], 40);
      setHostInt(str[0], 50);
      setHostInt(str[1], 60);
      setHostInt(str[2], 70);
      setHostInt(str[3], 80);
      setDeviceInt(dyn[1], 21);
      setDeviceInt(dyn[2], 31);
      setDeviceInt(str[1], 61);
      setDeviceInt(str[2], 71);

      // Actions for the same subarray should have no effect except to adjust
      // the dynamic reference count.
      #pragma acc data copy(dyn[1:2])
      ;
      #pragma acc enter data copyin(dyn[1:2], str[1:2])
      #pragma acc exit data copyout(dyn[1:2], str[1:2])
      #pragma acc exit data copyout(str[1:2])
      #pragma acc enter data create(dyn[1:2], str[1:2])
      #pragma acc exit data delete(dyn[1:2], str[1:2])
      #pragma acc exit data delete(str[1:2])

      // Actions for a subset of the subarray should also have no effect except
      // to adjust the dynamic reference count.
      #pragma acc data copy(dyn[1:1])
      ;
      #pragma acc enter data copyin(dyn[1:1], str[1:1])
      #pragma acc exit data copyout(dyn[1:1], str[1:1])
      #pragma acc exit data copyout(str[1:1])
      #pragma acc enter data create(dyn[1:1], str[1:1])
      #pragma acc exit data delete(dyn[1:1], str[1:1])
      #pragma acc exit data delete(str[1:1])

      // EXE-HOST-NEXT:   host dyn[0]         10{{$}}
      // EXE-HOST-NEXT:   host dyn[1]         21{{$}}
      // EXE-HOST-NEXT:   host dyn[2]         31{{$}}
      // EXE-HOST-NEXT:   host dyn[3]         40{{$}}
      // EXE-HOST-NEXT:   host str[0]         50{{$}}
      // EXE-HOST-NEXT:   host str[1]         61{{$}}
      // EXE-HOST-NEXT:   host str[2]         71{{$}}
      // EXE-HOST-NEXT:   host str[3]         80{{$}}
      //  EXE-DEV-NEXT:   host dyn[0]         10{{$}}
      //  EXE-DEV-NEXT:   host dyn[1]         20{{$}}
      //  EXE-DEV-NEXT:   host dyn[2]         30{{$}}
      //  EXE-DEV-NEXT:   host dyn[3]         40{{$}}
      //  EXE-DEV-NEXT:   host str[0]         50{{$}}
      //  EXE-DEV-NEXT:   host str[1]         60{{$}}
      //  EXE-DEV-NEXT:   host str[2]         70{{$}}
      //  EXE-DEV-NEXT:   host str[3]         80{{$}}
      //      EXE-NEXT: device dyn[1] present 21{{$}}
      //      EXE-NEXT: device dyn[2] present 31{{$}}
      //      EXE-NEXT: device str[1] present 61{{$}}
      //      EXE-NEXT: device str[2] present 71{{$}}
      printHostInt(dyn[0]);
      printHostInt(dyn[1]);
      printHostInt(dyn[2]);
      printHostInt(dyn[3]);
      printHostInt(str[0]);
      printHostInt(str[1]);
      printHostInt(str[2]);
      printHostInt(str[3]);
      printDeviceInt(dyn[1]);
      printDeviceInt(dyn[2]);
      printDeviceInt(str[1]);
      printDeviceInt(str[2]);
    }

    // acc exit data should have no effect for subarray that extends after,
    // extends before, or subsumes a subarray that's present.
    #pragma acc exit data copyout(dyn[1:3])
    #pragma acc exit data delete(dyn[1:3])
    #pragma acc exit data copyout(dyn[0:3])
    #pragma acc exit data delete(dyn[0:3])
    #pragma acc exit data copyout(dyn)
    #pragma acc exit data delete(dyn)

    // EXE-HOST-NEXT:   host dyn[0]         10{{$}}
    // EXE-HOST-NEXT:   host dyn[1]         21{{$}}
    // EXE-HOST-NEXT:   host dyn[2]         31{{$}}
    // EXE-HOST-NEXT:   host dyn[3]         40{{$}}
    //  EXE-DEV-NEXT:   host dyn[0]         10{{$}}
    //  EXE-DEV-NEXT:   host dyn[1]         20{{$}}
    //  EXE-DEV-NEXT:   host dyn[2]         30{{$}}
    //  EXE-DEV-NEXT:   host dyn[3]         40{{$}}
    //      EXE-NEXT: device dyn[1] present 21{{$}}
    //      EXE-NEXT: device dyn[2] present 31{{$}}
    printHostInt(dyn[0]);
    printHostInt(dyn[1]);
    printHostInt(dyn[2]);
    printHostInt(dyn[3]);
    printDeviceInt(dyn[1]);
    printDeviceInt(dyn[2]);
  }

  return 0;
}

// EXE-NOT: {{.}}
