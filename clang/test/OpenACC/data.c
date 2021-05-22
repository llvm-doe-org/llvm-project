// Check "acc data".
//
// Interactions with "acc enter data" and "acc exit data" are checked in
// enter-exit-data.c and fopenacc-structured-ref-count-omp.c.  Subarray
// extension errors are checked in subarray-errors.c, and other runtime errors
// from data clauses are checked in no-create.c and present.c.

// Check -ast-dump before and after AST serialization.
//
// RUN: %clang -Xclang -verify -Xclang -ast-dump -fsyntax-only -fopenacc %s \
// RUN: | FileCheck -check-prefix=DMP %s
// RUN: %clang -Xclang -verify -fopenacc -emit-ast -o %t.ast %s
// RUN: %clang_cc1 -ast-dump-all %t.ast \
// RUN: | FileCheck -check-prefixes=DMP %s

// Check -ast-print and -fopenacc[-ast]-print.
//
// RUN: %clang -Xclang -verify -Xclang -ast-print -fsyntax-only %s \
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
// RUN:   %clang -Xclang -verify %[prt] %t-acc.c \
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
// RUN: %clang -Xclang -verify -fopenacc -emit-ast %t-acc.c -o %t.ast
// RUN: %for prt-args {
// RUN:   %clang %[prt] %t.ast 2>&1 \
// RUN:          -Wno-openacc-omp-map-hold -Wno-openacc-omp-map-present \
// RUN:          -Wno-openacc-omp-map-no-alloc \
// RUN:   | FileCheck -check-prefixes=%[prt-chk] %s
// RUN: }

// Can we print the OpenMP source code, compile, and run it successfully?
//
// RUN: %for prt-opts {
// RUN:   %clang -Xclang -verify %[prt-opt]=omp %s > %t-omp.c \
// RUN:          -Wno-openacc-omp-map-hold -Wno-openacc-omp-map-present \
// RUN:          -Wno-openacc-omp-map-no-alloc
// RUN:   echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN:   %clang -Xclang -verify -fopenmp %fopenmp-version -o %t %t-omp.c
// RUN:   %t 2 2>&1 | FileCheck -check-prefixes=EXE,EXE-HOST %s
// RUN: }

// Check execution with normal compilation.
//
// RUN: %data tgts {
// RUN:   (run-if=                tgt-cflags=                                     host-or-dev=HOST)
// RUN:   (run-if=%run-if-x86_64  tgt-cflags=-fopenmp-targets=%run-x86_64-triple  host-or-dev=DEV )
// RUN:   (run-if=%run-if-ppc64le tgt-cflags=-fopenmp-targets=%run-ppc64le-triple host-or-dev=DEV )
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags=-fopenmp-targets=%run-nvptx64-triple host-or-dev=DEV )
// RUN: }
// RUN: %for tgts {
// RUN:   %[run-if] %clang -Xclang -verify -fopenacc %s -o %t %[tgt-cflags]
// RUN:   %[run-if] %t > %t.out 2>&1
// RUN:   %[run-if] FileCheck -input-file %t.out %s -strict-whitespace \
// RUN:                       -check-prefixes=EXE,EXE-%[host-or-dev]
// RUN: }

// END.

// expected-no-diagnostics

#include <stdio.h>

struct T { int i; };

int pr;
int prArr[1];
struct T prStruct;

void test();

int main() {
  // This encloses all tests so we can check the effect of the present clause on
  // statically nested directives without a runtime error.
  //
  //      DMP: ACCDataDirective
  // DMP-NEXT:   ACCCreateClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'pr' 'int'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'prArr' 'int [1]'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'prStruct' 'struct T'
  // DMP-NEXT:   impl: OMPTargetDataDirective
  // DMP-NEXT:     OMPMapClause
  //  DMP-NOT:       <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 'pr' 'int'
  // DMP-NEXT:       DeclRefExpr {{.*}} 'prArr' 'int [1]'
  // DMP-NEXT:       DeclRefExpr {{.*}} 'prStruct' 'struct T'
  #pragma acc data create(pr,prArr,prStruct)
  test();
}

void prSet() {
  pr = 90;
  prArr[0] = 90;
  prStruct.i = 90;
  // We call this between tests.
  //
  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:   ACCPresentClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'pr' 'int'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'prArr' 'int [1]'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'prStruct' 'struct T'
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'pr' 'int'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'prArr' 'int [1]'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'prStruct' 'struct T'
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:     OMPMapClause
  //  DMP-NOT:       <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 'pr' 'int'
  // DMP-NEXT:       DeclRefExpr {{.*}} 'prArr' 'int [1]'
  // DMP-NEXT:       DeclRefExpr {{.*}} 'prStruct' 'struct T'
  #pragma acc parallel num_gangs(1) present(pr, prArr, prStruct)
  {
    pr = 90;
    prArr[0] = 90;
    prStruct.i = 90;
  }
}

// PRT: void test() {
void test() {
  // PRT-NEXT: printf("start\n");
  // EXE: start
  printf("start\n");

  //--------------------------------------------------
  // Check acc data without statically nested directive.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: prSet();
    // PRT-NEXT: c =
    // PRT-NEXT: ci =
    // PRT-NEXT: co =
    // PRT-NEXT: cr =
    // PRT-NEXT: nc =
    prSet();
    int  c = 10;
    int ci = 20;
    int co = 30;
    int cr = 40;
    int nc = 50;
    // DMP:      ACCDataDirective
    // DMP-NEXT:   ACCPresentClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'pr' 'int'
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'int'
    // DMP-NEXT:   ACCCopyinClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'int'
    // DMP-NEXT:   ACCCopyoutClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'int'
    // DMP-NEXT:   ACCCreateClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'cr' 'int'
    // DMP-NEXT:   ACCNoCreateClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'nc' 'int'
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'pr' 'int'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'int'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'ci' 'int'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'co' 'int'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'cr' 'int'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'int'
    // DMP-NOT:      OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc data present(pr) copy(c) copyin(ci) copyout(co) create(cr) no_create(nc){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(present,hold,alloc: pr) map(hold,tofrom: c) map(hold,to: ci) map(hold,from: co) map(hold,alloc: cr) map(no_alloc,hold,alloc: nc){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target data map(present,hold,alloc: pr) map(hold,tofrom: c) map(hold,to: ci) map(hold,from: co) map(hold,alloc: cr) map(no_alloc,hold,alloc: nc){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data present(pr) copy(c) copyin(ci) copyout(co) create(cr) no_create(nc){{$}}
    #pragma acc data present(pr) copy(c) copyin(ci) copyout(co) create(cr) no_create(nc)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: pr +=
      // PRT-NEXT: c +=
      // PRT-NEXT: ci +=
      // PRT-NEXT: co +=
      // PRT-NEXT: cr +=
      // PRT-NEXT: nc +=
      pr += 100;
       c += 100;
      ci += 100;
      co += 100;
      cr += 100;
      nc += 100;
      // PRT-NEXT: printf(
      // PRT:      );
      //      EXE-NEXT: Inside acc data:
      // EXE-HOST-NEXT:   pr=190, c=110, ci=120, co=130, cr=140, nc=150
      //  EXE-DEV-NEXT:   pr=190, c=110, ci=120, co=130, cr=140, nc=150
      printf("Inside acc data:\n"
             "  pr=%3d, c=%3d, ci=%3d, co=%3d, cr=%3d, nc=%3d\n",
             pr, c, ci, co, cr, nc);
    } // PRT-NEXT: }
    // PRT-NEXT: printf(
    // PRT:      );
    //      EXE-NEXT: After acc data:
    // EXE-HOST-NEXT:   pr=190, c=110, ci=120, co=130, cr=140, nc=150
    //  EXE-DEV-NEXT:   pr=190, c= 10, ci=120, co={{.+}}, cr=140, nc=150
    printf("After acc data:\n"
           "  pr=%3d, c=%3d, ci=%3d, co=%3d, cr=%3d, nc=%3d\n",
           pr, c, ci, co, cr, nc);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Check acc data with nested acc data.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: c0 =
    // PRT-NEXT: c1 =
    // PRT-NEXT: c2 =
    int c0 = 10;
    int c1 = 20;
    int c2 = 30;
    // DMP:      ACCDataDirective
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'c0' 'int'
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'c0' 'int'
    // DMP-NOT:      OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc data copy(c0){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(hold,tofrom: c0){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target data map(hold,tofrom: c0){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data copy(c0){{$}}
    #pragma acc data copy(c0)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: c0 +=
      // PRT-NEXT: c1 +=
      // PRT-NEXT: c2 +=
      c0 += 100;
      c1 += 100;
      c2 += 100;
      // DMP:      ACCDataDirective
      // DMP-NEXT:   ACCCopyClause
      // DMP-NOT:      <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c1' 'int'
      // DMP-NEXT:   impl: OMPTargetDataDirective
      // DMP-NEXT:     OMPMapClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c0' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c1' 'int'
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc data copy(c0,c1){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(hold,tofrom: c0,c1){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp target data map(hold,tofrom: c0,c1){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc data copy(c0,c1){{$}}
      #pragma acc data copy(c0,c1)
      // DMP: CompoundStmt
      // PRT-NEXT: {
      {
        // PRT-NEXT: c0 +=
        // PRT-NEXT: c1 +=
        // PRT-NEXT: c2 +=
        c0 += 1000;
        c1 += 1000;
        c2 += 1000;
        // DMP:      ACCDataDirective
        // DMP-NEXT:   ACCCopyClause
        // DMP-NOT:      <implicit>
        // DMP-NEXT:     DeclRefExpr {{.*}} 'c0' 'int'
        // DMP-NEXT:     DeclRefExpr {{.*}} 'c1' 'int'
        // DMP-NEXT:     DeclRefExpr {{.*}} 'c2' 'int'
        // DMP-NEXT:   impl: OMPTargetDataDirective
        // DMP-NEXT:     OMPMapClause
        // DMP-NOT:        <implicit>
        // DMP-NEXT:       DeclRefExpr {{.*}} 'c0' 'int'
        // DMP-NEXT:       DeclRefExpr {{.*}} 'c1' 'int'
        // DMP-NEXT:       DeclRefExpr {{.*}} 'c2' 'int'
        // DMP-NOT:      OMP
        //
        // PRT-A-NEXT:  {{^ *}}#pragma acc data copy(c0,c1,c2){{$}}
        // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(hold,tofrom: c0,c1,c2){{$}}
        // PRT-O-NEXT:  {{^ *}}#pragma omp target data map(hold,tofrom: c0,c1,c2){{$}}
        // PRT-OA-NEXT: {{^ *}}// #pragma acc data copy(c0,c1,c2){{$}}
        #pragma acc data copy(c0,c1,c2)
        // DMP: CompoundStmt
        // PRT-NEXT: {
        {
          // PRT-NEXT: c0 +=
          // PRT-NEXT: c1 +=
          // PRT-NEXT: c2 +=
          c0 += 10000;
          c1 += 10000;
          c2 += 10000;
        } // PRT-NEXT: }
      } // PRT-NEXT: }
    } // PRT-NEXT: }
    // PRT-NEXT: printf(
    // PRT:      );
    //      EXE-NEXT: After acc data:
    // EXE-HOST-NEXT:   c0=11110, c1=11120, c2=11130
    //  EXE-DEV-NEXT:   c0=   10, c1=  120, c2= 1130
    printf("After acc data:\n"
           "  c0=%5d, c1=%5d, c2=%5d\n",
           c0, c1, c2);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Check that every data clause is accepted and suppresses data clauses on
  // nested acc parallel (and on its OpenMP translation).
  //
  // For clauses on acc data, check all clause aliases.
  // Check scalar types.
  // Check suppression of inner copy clause data transfers.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: prSet();
    // PRT: nc =
    prSet();
    int  c0 = 10,  c1 = 11,  c2 = 12;
    int ci0 = 20, ci1 = 21, ci2 = 22;
    int co0 = 30, co1 = 31, co2 = 32;
    int cr0 = 40, cr1 = 41, cr2 = 42;
    int nc = 50;
    // DMP:      ACCDataDirective
    // DMP-NEXT:   ACCPresentClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'pr' 'int'
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'c0' 'int'
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'c1' 'int'
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'c2' 'int'
    // DMP-NEXT:   ACCCopyinClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'ci0' 'int'
    // DMP-NEXT:   ACCCopyinClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'ci1' 'int'
    // DMP-NEXT:   ACCCopyinClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'ci2' 'int'
    // DMP-NEXT:   ACCCopyoutClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'co0' 'int'
    // DMP-NEXT:   ACCCopyoutClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'co1' 'int'
    // DMP-NEXT:   ACCCopyoutClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'co2' 'int'
    // DMP-NEXT:   ACCCreateClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'cr0' 'int'
    // DMP-NEXT:   ACCCreateClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'cr1' 'int'
    // DMP-NEXT:   ACCCreateClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'cr2' 'int'
    // DMP-NEXT:   ACCNoCreateClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'nc' 'int'
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'pr' 'int'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'c0' 'int'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'c1' 'int'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'c2' 'int'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'ci0' 'int'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'ci1' 'int'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'ci2' 'int'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'co0' 'int'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'co1' 'int'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'co2' 'int'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'cr0' 'int'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'cr1' 'int'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'cr2' 'int'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'int'
    // DMP-NOT:      OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc data present(pr){{( *\\$[[:space:]])?}}
    //  PRT-A-SAME: {{^ *}}copy(c0) pcopy(c1) present_or_copy(c2){{( *\\$[[:space:]])?}}
    //  PRT-A-SAME: {{^ *}}copyin(ci0) pcopyin(ci1) present_or_copyin(ci2){{( *\\$[[:space:]])?}}
    //  PRT-A-SAME: {{^ *}}copyout(co0) pcopyout(co1) present_or_copyout(co2){{( *\\$[[:space:]])?}}
    //  PRT-A-SAME: {{^ *}}create(cr0) pcreate(cr1) present_or_create(cr2){{( *\\$[[:space:]])?}}
    //  PRT-A-SAME: {{^ *}}no_create(nc){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(present,hold,alloc: pr)
    // PRT-AO-SAME: {{^ *}}map(hold,tofrom: c0) map(hold,tofrom: c1) map(hold,tofrom: c2)
    // PRT-AO-SAME: {{^ *}}map(hold,to: ci0) map(hold,to: ci1) map(hold,to: ci2)
    // PRT-AO-SAME: {{^ *}}map(hold,from: co0) map(hold,from: co1) map(hold,from: co2)
    // PRT-AO-SAME: {{^ *}}map(hold,alloc: cr0) map(hold,alloc: cr1) map(hold,alloc: cr2)
    // PRT-AO-SAME: {{^ *}}map(no_alloc,hold,alloc: nc){{$}}
    //
    //  PRT-O-NEXT: {{^ *}}#pragma omp target data map(present,hold,alloc: pr)
    //  PRT-O-SAME: {{^ *}}map(hold,tofrom: c0) map(hold,tofrom: c1) map(hold,tofrom: c2)
    //  PRT-O-SAME: {{^ *}}map(hold,to: ci0) map(hold,to: ci1) map(hold,to: ci2)
    //  PRT-O-SAME: {{^ *}}map(hold,from: co0) map(hold,from: co1) map(hold,from: co2)
    //  PRT-O-SAME: {{^ *}}map(hold,alloc: cr0) map(hold,alloc: cr1) map(hold,alloc: cr2)
    //  PRT-O-SAME: {{^ *}}map(no_alloc,hold,alloc: nc){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data present(pr){{( *\\$[[:space:]] *//)?}}
    // PRT-OA-SAME: {{^ *}}copy(c0) pcopy(c1) present_or_copy(c2){{( *\\$[[:space:]] *//)?}}
    // PRT-OA-SAME: {{^ *}}copyin(ci0) pcopyin(ci1) present_or_copyin(ci2){{( *\\$[[:space:]] *//)?}}
    // PRT-OA-SAME: {{^ *}}copyout(co0) pcopyout(co1) present_or_copyout(co2){{( *\\$[[:space:]] *//)?}}
    // PRT-OA-SAME: {{^ *}}create(cr0) pcreate(cr1) present_or_create(cr2){{( *\\$[[:space:]] *//)?}}
    // PRT-OA-SAME: {{^ *}}no_create(nc){{$}}
    #pragma acc data present(pr)                                        \
                     copy(c0) pcopy(c1) present_or_copy(c2)             \
                     copyin(ci0) pcopyin(ci1) present_or_copyin(ci2)    \
                     copyout(co0) pcopyout(co1) present_or_copyout(co2) \
                     create(cr0) pcreate(cr1) present_or_create(cr2)    \
                     no_create(nc)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: pr +=
      // PRT: nc +=
       pr += 100;
       c0 += 100;  c1 += 100;  c2 += 100;
      ci0 += 100; ci1 += 100; ci2 += 100;
      co0 += 100; co1 += 100; co2 += 100;
      cr0 += 100; cr1 += 100; cr2 += 100;
       nc += 100;

      // Check suppression of implicit data clauses.
      //
      // DMP:      ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co2' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'cr0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'cr1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'cr2' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'nc' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'pr' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c2' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci2' 'int'
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co2' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'cr0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'cr1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'cr2' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'nc' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'pr' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c2' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci2' 'int'
      // DMP-NEXT:   impl: OMPTargetTeamsDirective
      // DMP-NEXT:     OMPNum_teamsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPMapClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'int'
      // DMP-NEXT:     OMPSharedClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co0' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co1' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co2' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'cr0' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'cr1' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'cr2' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'pr' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c0' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c1' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c2' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci0' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci1' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci2' 'int'
      // DMP-NEXT:     OMPDefaultmapClause
      // DMP-NOT:        <implicit>
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1)
      // PRT-AO-SAME: {{^ *}}map(no_alloc,alloc: nc)
      // PRT-AO-SAME: {{^ *}}shared(co0,co1,co2,cr0,cr1,cr2,nc,pr,c0,c1,c2,ci0,ci1,ci2)
      // PRT-AO-SAME: {{^ *}}defaultmap(tofrom: scalar){{$}}
      //
      // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1)
      // PRT-O-SAME:  {{^ *}}map(no_alloc,alloc: nc)
      // PRT-O-SAME:  {{^ *}}shared(co0,co1,co2,cr0,cr1,cr2,nc,pr,c0,c1,c2,ci0,ci1,ci2)
      // PRT-O-SAME:  {{^ *}}defaultmap(tofrom: scalar){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1){{$}}
      #pragma acc parallel num_gangs(1)
      // DMP: CompoundStmt
      // PRT-NEXT: {
      {
        // Don't use these uninitialized.
        // PRT-NEXT: co0 =
        // PRT: cr2 =
        co0 = 35; co1 = 36; co2 = 37;
        cr0 = 45; cr1 = 46; cr2 = 47;
        // Reference nc to trigger any implicit attributes, but nc shouldn't be
        // allocated, so don't actually access it at run time.
        // PRT-NEXT: if
        // PRT-NEXT: nc =
        if (!cr0)
          nc = 55;
        // PRT-NEXT: printf(
        // PRT:      );
        //      EXE-NEXT: In first acc parallel:
        // EXE-HOST-NEXT:    pr= 190
        // EXE-HOST-NEXT:    c0= 110,  c1= 111,  c2= 112
        // EXE-HOST-NEXT:   ci0= 120, ci1= 121, ci2= 122
        // EXE-HOST-NEXT:   co0=  35, co1=  36, co2=  37
        // EXE-HOST-NEXT:   cr0=  45, cr1=  46, cr2=  47
        //  EXE-DEV-NEXT:    pr=  90
        //  EXE-DEV-NEXT:    c0=  10,  c1=  11,  c2=  12
        //  EXE-DEV-NEXT:   ci0=  20, ci1=  21, ci2=  22
        //  EXE-DEV-NEXT:   co0=  35, co1=  36, co2=  37
        //  EXE-DEV-NEXT:   cr0=  45, cr1=  46, cr2=  47
        printf("In first acc parallel:\n"
               "   pr=%4d\n"
               "   c0=%4d,  c1=%4d,  c2=%4d\n"
               "  ci0=%4d, ci1=%4d, ci2=%4d\n"
               "  co0=%4d, co1=%4d, co2=%4d\n"
               "  cr0=%4d, cr1=%4d, cr2=%4d\n",
               pr, c0, c1, c2, ci0, ci1, ci2, co0, co1, co2, cr0, cr1, cr2);
        // PRT-NEXT: pr +=
        // PRT: cr2 +=
         pr += 1000;
         c0 += 1000;  c1 += 1000;  c2 += 1000;
        ci0 += 1000; ci1 += 1000; ci2 += 1000;
        co0 += 1000; co1 += 1000; co2 += 1000;
        cr0 += 1000; cr1 += 1000; cr2 += 1000;
      } // PRT-NEXT: }

      // PRT-NEXT: printf(
      // PRT:      );
      //      EXE-NEXT: After first acc parallel:
      // EXE-HOST-NEXT:    pr=1190
      // EXE-HOST-NEXT:    c0=1110,  c1=1111,  c2=1112
      // EXE-HOST-NEXT:   ci0=1120, ci1=1121, ci2=1122
      // EXE-HOST-NEXT:   co0=1035, co1=1036, co2=1037
      // EXE-HOST-NEXT:   cr0=1045, cr1=1046, cr2=1047
      // EXE-HOST-NEXT:    nc= 150
      //  EXE-DEV-NEXT:    pr= 190
      //  EXE-DEV-NEXT:    c0= 110,  c1= 111,  c2= 112
      //  EXE-DEV-NEXT:   ci0= 120, ci1= 121, ci2= 122
      //  EXE-DEV-NEXT:   co0= 130, co1= 131, co2= 132
      //  EXE-DEV-NEXT:   cr0= 140, cr1= 141, cr2= 142
      //  EXE-DEV-NEXT:    nc= 150
      printf("After first acc parallel:\n"
             "   pr=%4d\n"
             "   c0=%4d,  c1=%4d,  c2=%4d\n"
             "  ci0=%4d, ci1=%4d, ci2=%4d\n"
             "  co0=%4d, co1=%4d, co2=%4d\n"
             "  cr0=%4d, cr1=%4d, cr2=%4d\n"
             "   nc=%4d\n",
             pr, c0, c1, c2, ci0, ci1, ci2, co0, co1, co2, cr0, cr1, cr2, nc);
      // PRT-NEXT: pr +=
      // PRT: nc +=
       pr += 100;
       c0 += 100;  c1 += 100;  c2 += 100;
      ci0 += 100; ci1 += 100; ci2 += 100;
      co0 += 100; co1 += 100; co2 += 100;
      cr0 += 100; cr1 += 100; cr2 += 100;
       nc += 100;

      // Check suppression of explicit data clauses (only data transfers are
      // suppressed except no_create suppresses nothing).
      //
      // DMP:      ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCCopyClause
      // DMP-NOT:      <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'pr' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c2' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci2' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co2' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'cr0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'cr1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'cr2' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'nc' 'int'
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'pr' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c2' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci2' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co2' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'cr0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'cr1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'cr2' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'nc' 'int'
      // DMP-NEXT:   impl: OMPTargetTeamsDirective
      // DMP-NEXT:     OMPNum_teamsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPMapClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'pr' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c0' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c1' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c2' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci0' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci1' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci2' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co0' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co1' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co2' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'cr0' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'cr1' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'cr2' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'int'
      // DMP-NEXT:     OMPSharedClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'pr' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c0' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c1' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c2' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci0' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci1' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci2' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co0' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co1' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co2' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'cr0' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'cr1' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'cr2' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'int'
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1)
      // PRT-A-SAME:  {{^ *}}copy(pr,c0,c1,c2,ci0,ci1,ci2,co0,co1,co2,cr0,cr1,cr2,nc){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1)
      // PRT-AO-SAME: {{^ *}}map(hold,tofrom: pr,c0,c1,c2,ci0,ci1,ci2,co0,co1,co2,cr0,cr1,cr2,nc)
      // PRT-AO-SAME: {{^ *}}shared(pr,c0,c1,c2,ci0,ci1,ci2,co0,co1,co2,cr0,cr1,cr2,nc){{$}}
      //
      // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1)
      // PRT-O-SAME:  {{^ *}}map(hold,tofrom: pr,c0,c1,c2,ci0,ci1,ci2,co0,co1,co2,cr0,cr1,cr2,nc)
      // PRT-O-SAME:  {{^ *}}shared(pr,c0,c1,c2,ci0,ci1,ci2,co0,co1,co2,cr0,cr1,cr2,nc){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1)
      // PRT-OA-SAME: {{^ *}}copy(pr,c0,c1,c2,ci0,ci1,ci2,co0,co1,co2,cr0,cr1,cr2,nc){{$}}
      #pragma acc parallel num_gangs(1) copy(pr,c0,c1,c2,ci0,ci1,ci2,co0,co1,co2,cr0,cr1,cr2,nc)
      // DMP: CompoundStmt
      // PRT-NEXT: {
      {
        // PRT-NEXT: printf(
        // PRT:      );
        //      EXE-NEXT: In second acc parallel:
        // EXE-HOST-NEXT:    pr=1290
        // EXE-HOST-NEXT:    c0=1210,  c1=1211,  c2=1212
        // EXE-HOST-NEXT:   ci0=1220, ci1=1221, ci2=1222
        // EXE-HOST-NEXT:   co0=1135, co1=1136, co2=1137
        // EXE-HOST-NEXT:   cr0=1145, cr1=1146, cr2=1147
        // EXE-HOST-NEXT:    nc= 250
        //  EXE-DEV-NEXT:    pr=1090
        //  EXE-DEV-NEXT:    c0=1010,  c1=1011,  c2=1012
        //  EXE-DEV-NEXT:   ci0=1020, ci1=1021, ci2=1022
        //  EXE-DEV-NEXT:   co0=1035, co1=1036, co2=1037
        //  EXE-DEV-NEXT:   cr0=1045, cr1=1046, cr2=1047
        //  EXE-DEV-NEXT:    nc= 250
        printf("In second acc parallel:\n"
               "   pr=%4d\n"
               "   c0=%4d,  c1=%4d,  c2=%4d\n"
               "  ci0=%4d, ci1=%4d, ci2=%4d\n"
               "  co0=%4d, co1=%4d, co2=%4d\n"
               "  cr0=%4d, cr1=%4d, cr2=%4d\n"
               "   nc=%4d\n",
               pr, c0, c1, c2, ci0, ci1, ci2, co0, co1, co2, cr0, cr1, cr2, nc);
        // PRT-NEXT: pr +=
        // PRT: nc +=
         pr += 1000;
         c0 += 1000;  c1 += 1000;  c2 += 1000;
        ci0 += 1000; ci1 += 1000; ci2 += 1000;
        co0 += 1000; co1 += 1000; co2 += 1000;
        cr0 += 1000; cr1 += 1000; cr2 += 1000;
         nc += 1000;
      } // PRT-NEXT: }

      // PRT-NEXT: printf(
      // PRT:      );
      //      EXE-NEXT: After second acc parallel:
      // EXE-HOST-NEXT:    pr=2290
      // EXE-HOST-NEXT:    c0=2210,  c1=2211,  c2=2212
      // EXE-HOST-NEXT:   ci0=2220, ci1=2221, ci2=2222
      // EXE-HOST-NEXT:   co0=2135, co1=2136, co2=2137
      // EXE-HOST-NEXT:   cr0=2145, cr1=2146, cr2=2147
      // EXE-HOST-NEXT:    nc=1250
      //  EXE-DEV-NEXT:    pr= 290
      //  EXE-DEV-NEXT:    c0= 210,  c1= 211,  c2= 212
      //  EXE-DEV-NEXT:   ci0= 220, ci1= 221, ci2= 222
      //  EXE-DEV-NEXT:   co0= 230, co1= 231, co2= 232
      //  EXE-DEV-NEXT:   cr0= 240, cr1= 241, cr2= 242
      //  EXE-DEV-NEXT:    nc=1250
      printf("After second acc parallel:\n"
             "   pr=%4d\n"
             "   c0=%4d,  c1=%4d,  c2=%4d\n"
             "  ci0=%4d, ci1=%4d, ci2=%4d\n"
             "  co0=%4d, co1=%4d, co2=%4d\n"
             "  cr0=%4d, cr1=%4d, cr2=%4d\n"
             "   nc=%4d\n",
             pr, c0, c1, c2, ci0, ci1, ci2, co0, co1, co2, cr0, cr1, cr2, nc);
      // PRT-NEXT: pr +=
      // PRT: nc +=
       pr += 100;
       c0 += 100;  c1 += 100;  c2 += 100;
      ci0 += 100; ci1 += 100; ci2 += 100;
      co0 += 100; co1 += 100; co2 += 100;
      cr0 += 100; cr1 += 100; cr2 += 100;
       nc += 100;
    } // PRT-NEXT: }

    // PRT-NEXT: printf(
    // PRT:      );
    //      EXE-NEXT: After acc data:
    // EXE-HOST-NEXT:    pr=2390
    // EXE-HOST-NEXT:    c0=2310,  c1=2311,  c2=2312
    // EXE-HOST-NEXT:   ci0=2320, ci1=2321, ci2=2322
    // EXE-HOST-NEXT:   co0=2235, co1=2236, co2=2237
    // EXE-HOST-NEXT:   cr0=2245, cr1=2246, cr2=2247
    // EXE-HOST-NEXT:    nc=1350
    //  EXE-DEV-NEXT:    pr= 390
    //  EXE-DEV-NEXT:    c0=2010,  c1=2011,  c2=2012
    //  EXE-DEV-NEXT:   ci0= 320, ci1= 321, ci2= 322
    //  EXE-DEV-NEXT:   co0=2035, co1=2036, co2=2037
    //  EXE-DEV-NEXT:   cr0= 340, cr1= 341, cr2= 342
    //  EXE-DEV-NEXT:    nc=1350
    printf("After acc data:\n"
           "   pr=%4d\n"
           "   c0=%4d,  c1=%4d,  c2=%4d\n"
           "  ci0=%4d, ci1=%4d, ci2=%4d\n"
           "  co0=%4d, co1=%4d, co2=%4d\n"
           "  cr0=%4d, cr1=%4d, cr2=%4d\n"
           "   nc=%4d\n",
           pr, c0, c1, c2, ci0, ci1, ci2, co0, co1, co2, cr0, cr1, cr2, nc);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Repeat except...
  //
  // Don't bother checking clause aliases again.
  // Check array types (including not adding defaultmap for scalars with
  // suppressed OpenACC implicit DAs).
  // Check suppression of inner copyin clause data transfers.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: prSet();
    // PRT: nc[] = {50};
    prSet();
    int  c[] = {10};
    int ci[] = {20};
    int co[] = {30};
    int cr[] = {40};
    int nc[] = {50};
    // DMP:      ACCDataDirective
    // DMP-NEXT:   ACCPresentClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'prArr' 'int [1]'
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'int [1]'
    // DMP-NEXT:   ACCCopyinClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'int [1]'
    // DMP-NEXT:   ACCCopyoutClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'int [1]'
    // DMP-NEXT:   ACCCreateClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'cr' 'int [1]'
    // DMP-NEXT:   ACCNoCreateClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'nc' 'int [1]'
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'prArr' 'int [1]'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'int [1]'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'ci' 'int [1]'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'co' 'int [1]'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'cr' 'int [1]'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'int [1]'
    // DMP-NOT:      OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc data present(prArr){{( *\\$[[:space:]])?}}
    //  PRT-A-SAME: {{^ *}}copy(c) copyin(ci) copyout(co){{( *\\$[[:space:]])?}}
    //  PRT-A-SAME: {{^ *}}create(cr) no_create(nc){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(present,hold,alloc: prArr)
    // PRT-AO-SAME: {{^  *}}map(hold,tofrom: c) map(hold,to: ci) map(hold,from: co)
    // PRT-AO-SAME: {{^  *}}map(hold,alloc: cr) map(no_alloc,hold,alloc: nc){{$}}
    //
    //  PRT-O-NEXT: {{^ *}}#pragma omp target data map(present,hold,alloc: prArr)
    //  PRT-O-SAME: {{^  *}}map(hold,tofrom: c) map(hold,to: ci) map(hold,from: co)
    //  PRT-O-SAME: {{^  *}}map(hold,alloc: cr) map(no_alloc,hold,alloc: nc){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data present(prArr){{( *\\$[[:space:]] *//)?}}
    // PRT-OA-SAME: {{^ *}}copy(c) copyin(ci) copyout(co){{( *\\$[[:space:]] *//)?}}
    // PRT-OA-SAME: {{^ *}}create(cr) no_create(nc){{$}}
    #pragma acc data present(prArr) \
                     copy(c) copyin(ci) copyout(co) \
                     create(cr) no_create(nc)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: prArr[0] +=
      // PRT: nc[0] +=
      prArr[0] += 100;
          c[0] += 100;
         ci[0] += 100;
         co[0] += 100;
         cr[0] += 100;
         nc[0] += 100;

      // Check suppression of implicit data clauses.
      //
      // DMP:      ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'cr' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'nc' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'prArr' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'int [1]'
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'cr' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'nc' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'prArr' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'int [1]'
      // DMP-NEXT:   impl: OMPTargetTeamsDirective
      // DMP-NEXT:     OMPNum_teamsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPMapClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'int [1]'
      // DMP-NEXT:     OMPSharedClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co' 'int [1]'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'cr' 'int [1]'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'int [1]'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'prArr' 'int [1]'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'int [1]'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci' 'int [1]'
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1)
      // PRT-AO-SAME: {{^ *}}map(no_alloc,alloc: nc)
      // PRT-AO-SAME: {{^ *}}shared(co,cr,nc,prArr,c,ci){{$}}
      //
      // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1)
      // PRT-O-SAME:  {{^ *}}map(no_alloc,alloc: nc)
      // PRT-O-SAME:  {{^ *}}shared(co,cr,nc,prArr,c,ci){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1){{$}}
      #pragma acc parallel num_gangs(1)
      // DMP: CompoundStmt
      // PRT-NEXT: {
      {
        // Don't use these uninitialized.
        // PRT-NEXT: co[0] =
        // PRT: cr[0] =
        co[0] = 35;
        cr[0] = 45;
        // Reference nc to trigger any implicit attributes, but nc shouldn't be
        // allocated, so don't actually access it at run time.
        // PRT-NEXT: if
        // PRT-NEXT: nc[0] = 55;
        if (!cr[0])
          nc[0] = 55;
        // PRT-NEXT: printf(
        // PRT:      );
        //      EXE-NEXT: In first acc parallel:
        // EXE-HOST-NEXT:   prArr[0]= 190
        // EXE-HOST-NEXT:       c[0]= 110, ci[0]= 120, co[0]=  35
        // EXE-HOST-NEXT:      cr[0]=  45
        //  EXE-DEV-NEXT:   prArr[0]=  90
        //  EXE-DEV-NEXT:       c[0]=  10, ci[0]=  20, co[0]=  35
        //  EXE-DEV-NEXT:      cr[0]=  45
        printf("In first acc parallel:\n"
               "  prArr[0]=%4d\n"
               "      c[0]=%4d, ci[0]=%4d, co[0]=%4d\n"
               "     cr[0]=%4d\n",
               prArr[0], c[0], ci[0], co[0], cr[0]);
        // PRT-NEXT: prArr[0] +=
        // PRT: cr[0] +=
        prArr[0] += 1000;
            c[0] += 1000;
           ci[0] += 1000;
           co[0] += 1000;
           cr[0] += 1000;
      } // PRT-NEXT: }

      // PRT-NEXT: printf(
      // PRT:      );
      //      EXE-NEXT: After first acc parallel:
      // EXE-HOST-NEXT:   prArr[0]=1190
      // EXE-HOST-NEXT:       c[0]=1110, ci[0]=1120, co[0]=1035
      // EXE-HOST-NEXT:      cr[0]=1045, nc[0]= 150
      //  EXE-DEV-NEXT:   prArr[0]= 190
      //  EXE-DEV-NEXT:       c[0]= 110, ci[0]= 120, co[0]= 130
      //  EXE-DEV-NEXT:      cr[0]= 140, nc[0]= 150
      printf("After first acc parallel:\n"
             "  prArr[0]=%4d\n"
             "      c[0]=%4d, ci[0]=%4d, co[0]=%4d\n"
             "     cr[0]=%4d, nc[0]=%4d\n",
             prArr[0], c[0], ci[0], co[0], cr[0], nc[0]);
      // PRT-NEXT: prArr[0] +=
      // PRT: nc[0] +=
      prArr[0] += 100;
          c[0] += 100;
         ci[0] += 100;
         co[0] += 100;
         cr[0] += 100;
         nc[0] += 100;

      // Check suppression of explicit data clauses (only data transfers are
      // suppressed except no_create suppresses nothing).
      //
      // DMP:      ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCCopyinClause
      // DMP-NOT:      <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'prArr' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'cr' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'nc' 'int [1]'
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'prArr' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'cr' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'nc' 'int [1]'
      // DMP-NEXT:   impl: OMPTargetTeamsDirective
      // DMP-NEXT:     OMPNum_teamsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPMapClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'prArr' 'int [1]'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'int [1]'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci' 'int [1]'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co' 'int [1]'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'cr' 'int [1]'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'int [1]'
      // DMP-NEXT:     OMPSharedClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'prArr' 'int [1]'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'int [1]'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci' 'int [1]'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co' 'int [1]'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'cr' 'int [1]'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'int [1]'
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1) copyin(prArr,c,ci,co,cr,nc){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1)
      // PRT-AO-SAME: {{^ *}}map(hold,to: prArr,c,ci,co,cr,nc) shared(prArr,c,ci,co,cr,nc){{$}}
      //
      // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1)
      // PRT-O-SAME:  {{^ *}}map(hold,to: prArr,c,ci,co,cr,nc) shared(prArr,c,ci,co,cr,nc){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) copyin(prArr,c,ci,co,cr,nc){{$}}
      #pragma acc parallel num_gangs(1) copyin(prArr,c,ci,co,cr,nc)
      // DMP: CompoundStmt
      // PRT-NEXT: {
      {
        // PRT-NEXT: printf(
        // PRT:      );
        //      EXE-NEXT: In second acc parallel:
        // EXE-HOST-NEXT:   prArr[0]=1290
        // EXE-HOST-NEXT:       c[0]=1210, ci[0]=1220, co[0]=1135
        // EXE-HOST-NEXT:      cr[0]=1145, nc[0]= 250
        //  EXE-DEV-NEXT:   prArr[0]=1090
        //  EXE-DEV-NEXT:       c[0]=1010, ci[0]=1020, co[0]=1035
        //  EXE-DEV-NEXT:      cr[0]=1045, nc[0]= 250
        printf("In second acc parallel:\n"
               "  prArr[0]=%4d\n"
               "      c[0]=%4d, ci[0]=%4d, co[0]=%4d\n"
               "     cr[0]=%4d, nc[0]=%4d\n",
               prArr[0], c[0], ci[0], co[0], cr[0], nc[0]);
        // PRT-NEXT: prArr[0] +=
        // PRT: nc[0] +=
        prArr[0] += 1000;
            c[0] += 1000;
           ci[0] += 1000;
           co[0] += 1000;
           cr[0] += 1000;
           nc[0] += 1000;
      } // PRT-NEXT: }

      // PRT-NEXT: printf(
      // PRT:      );
      //      EXE-NEXT: After second acc parallel:
      // EXE-HOST-NEXT:   prArr[0]=2290
      // EXE-HOST-NEXT:       c[0]=2210, ci[0]=2220, co[0]=2135
      // EXE-HOST-NEXT:      cr[0]=2145, nc[0]=1250
      //  EXE-DEV-NEXT:   prArr[0]= 290
      //  EXE-DEV-NEXT:       c[0]= 210, ci[0]= 220, co[0]= 230
      //  EXE-DEV-NEXT:      cr[0]= 240, nc[0]= 250
      printf("After second acc parallel:\n"
             "   prArr[0]=%4d\n"
             "       c[0]=%4d, ci[0]=%4d, co[0]=%4d\n"
             "      cr[0]=%4d, nc[0]=%4d\n",
             prArr[0], c[0], ci[0], co[0], cr[0], nc[0]);
      // PRT-NEXT: prArr[0] +=
      // PRT: nc[0] +=
      prArr[0] += 100;
          c[0] += 100;
         ci[0] += 100;
         co[0] += 100;
         cr[0] += 100;
         nc[0] += 100;
    } // PRT-NEXT: }

    // PRT-NEXT: printf(
    // PRT:      );
    //      EXE-NEXT: After acc data:
    // EXE-HOST-NEXT:   prArr[0]=2390
    // EXE-HOST-NEXT:       c[0]=2310, ci[0]=2320, co[0]=2235
    // EXE-HOST-NEXT:      cr[0]=2245, nc[0]=1350
    //  EXE-DEV-NEXT:   prArr[0]= 390
    //  EXE-DEV-NEXT:       c[0]=2010, ci[0]= 320, co[0]=2035
    //  EXE-DEV-NEXT:      cr[0]= 340, nc[0]= 350
    printf("After acc data:\n"
           "   prArr[0]=%4d\n"
           "       c[0]=%4d, ci[0]=%4d, co[0]=%4d\n"
           "      cr[0]=%4d, nc[0]=%4d\n",
           prArr[0], c[0], ci[0], co[0], cr[0], nc[0]);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Repeat except...
  //
  // Check struct types (including not adding defaultmap for scalars with
  // suppressed OpenACC implicit DAs).
  // Check suppression of inner copyout clause data transfers.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: prSet();
    // PRT: nc = {50};
    prSet();
    struct T  c = {10};
    struct T ci = {20};
    struct T co = {30};
    struct T cr = {40};
    struct T nc = {50};
    // DMP:      ACCDataDirective
    // DMP-NEXT:   ACCPresentClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'prStruct' 'struct T'
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'struct T'
    // DMP-NEXT:   ACCCopyinClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'struct T'
    // DMP-NEXT:   ACCCopyoutClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'struct T'
    // DMP-NEXT:   ACCCreateClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'cr' 'struct T'
    // DMP-NEXT:   ACCNoCreateClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'nc' 'struct T'
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'prStruct' 'struct T'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'struct T'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'ci' 'struct T'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'co' 'struct T'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'cr' 'struct T'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'struct T'
    // DMP-NOT:      OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc data present(prStruct){{( *\\$[[:space:]])?}}
    //  PRT-A-SAME: {{^ *}}copy(c) copyin(ci) copyout(co){{( *\\$[[:space:]])?}}
    //  PRT-A-SAME: {{^ *}}create(cr) no_create(nc){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(present,hold,alloc: prStruct)
    // PRT-AO-SAME: {{^  *}}map(hold,tofrom: c) map(hold,to: ci) map(hold,from: co)
    // PRT-AO-SAME: {{^  *}}map(hold,alloc: cr) map(no_alloc,hold,alloc: nc){{$}}
    //
    //  PRT-O-NEXT: {{^ *}}#pragma omp target data map(present,hold,alloc: prStruct)
    //  PRT-O-SAME: {{^  *}}map(hold,tofrom: c) map(hold,to: ci) map(hold,from: co)
    //  PRT-O-SAME: {{^  *}}map(hold,alloc: cr) map(no_alloc,hold,alloc: nc){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data present(prStruct){{( *\\$[[:space:]] *//)?}}
    // PRT-OA-SAME: {{^ *}}copy(c) copyin(ci) copyout(co){{( *\\$[[:space:]] *//)?}}
    // PRT-OA-SAME: {{^ *}}create(cr) no_create(nc){{$}}
    #pragma acc data present(prStruct) \
                     copy(c) copyin(ci) copyout(co) \
                     create(cr) no_create(nc)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: prStruct.i +=
      // PRT: nc.i +=
      prStruct.i += 100;
             c.i += 100;
            ci.i += 100;
            co.i += 100;
            cr.i += 100;
            nc.i += 100;

      // Check suppression of implicit data clauses.
      //
      // DMP:      ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'cr' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'nc' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'prStruct' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'struct T'
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'cr' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'nc' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'prStruct' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'struct T'
      // DMP-NEXT:   impl: OMPTargetTeamsDirective
      // DMP-NEXT:     OMPNum_teamsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPMapClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'struct T'
      // DMP-NEXT:     OMPSharedClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co' 'struct T'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'cr' 'struct T'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'struct T'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'prStruct' 'struct T'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'struct T'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci' 'struct T'
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1)
      // PRT-AO-SAME: {{^ *}}map(no_alloc,alloc: nc)
      // PRT-AO-SAME: {{^ *}}shared(co,cr,nc,prStruct,c,ci){{$}}
      //
      // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1)
      // PRT-O-SAME:  {{^ *}}map(no_alloc,alloc: nc)
      // PRT-O-SAME:  {{^ *}}shared(co,cr,nc,prStruct,c,ci){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1){{$}}
      #pragma acc parallel num_gangs(1)
      // DMP: CompoundStmt
      // PRT-NEXT: {
      {
        // Don't use this uninitialized.
        // PRT-NEXT: co.i =
        // PRT: cr.i =
        co.i = 35;
        cr.i = 45;
        // Reference nc to trigger any implicit attributes, but nc shouldn't be
        // allocated, so don't actually access it at run time.
        // PRT-NEXT: if
        // PRT-NEXT: nc.i = 55;
        if (!cr.i)
          nc.i = 55;
        // PRT-NEXT: printf(
        // PRT:      );
        //      EXE-NEXT: In first acc parallel:
        // EXE-HOST-NEXT:   prStruct.i= 190
        // EXE-HOST-NEXT:          c.i= 110, ci.i= 120, co.i=  35
        // EXE-HOST-NEXT:         cr.i=  45
        //  EXE-DEV-NEXT:   prStruct.i=  90
        //  EXE-DEV-NEXT:          c.i=  10, ci.i=  20, co.i=  35
        //  EXE-DEV-NEXT:         cr.i=  45
        printf("In first acc parallel:\n"
               "  prStruct.i=%4d\n"
               "         c.i=%4d, ci.i=%4d, co.i=%4d\n"
               "        cr.i=%4d\n",
               prStruct.i, c.i, ci.i, co.i, cr.i);
        // PRT-NEXT: prStruct.i +=
        // PRT: cr.i +=
        prStruct.i += 1000;
               c.i += 1000;
              ci.i += 1000;
              co.i += 1000;
              cr.i += 1000;
      } // PRT-NEXT: }

      // PRT-NEXT: printf(
      // PRT:      );
      //      EXE-NEXT: After first acc parallel:
      // EXE-HOST-NEXT:   prStruct.i=1190
      // EXE-HOST-NEXT:          c.i=1110, ci.i=1120, co.i=1035
      // EXE-HOST-NEXT:         cr.i=1045, nc.i= 150
      //  EXE-DEV-NEXT:   prStruct.i= 190
      //  EXE-DEV-NEXT:          c.i= 110, ci.i= 120, co.i= 130
      //  EXE-DEV-NEXT:         cr.i= 140, nc.i= 150
      printf("After first acc parallel:\n"
             "  prStruct.i=%4d\n"
             "         c.i=%4d, ci.i=%4d, co.i=%4d\n"
             "        cr.i=%4d, nc.i=%4d\n",
             prStruct.i, c.i, ci.i, co.i, cr.i, nc.i);
      // PRT-NEXT: prStruct.i +=
      // PRT: nc.i +=
      prStruct.i += 100;
             c.i += 100;
            ci.i += 100;
            co.i += 100;
            cr.i += 100;
            nc.i += 100;

      // Check suppression of explicit data clauses (only data transfers are
      // suppressed except no_create suppresses nothing).
      //
      // DMP:      ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCCopyoutClause
      // DMP-NOT:      <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'prStruct' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'cr' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'nc' 'struct T'
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'nc' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'prStruct' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'cr' 'struct T'
      // DMP-NEXT:   impl: OMPTargetTeamsDirective
      // DMP-NEXT:     OMPNum_teamsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPMapClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'prStruct' 'struct T'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'struct T'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci' 'struct T'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co' 'struct T'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'cr' 'struct T'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'struct T'
      // DMP-NEXT:     OMPSharedClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'nc' 'struct T'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'prStruct' 'struct T'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'struct T'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci' 'struct T'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co' 'struct T'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'cr' 'struct T'
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1) copyout(prStruct,c,ci,co,cr,nc){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1)
      // PRT-AO-SAME: {{^ *}}map(hold,from: prStruct,c,ci,co,cr,nc) shared(nc,prStruct,c,ci,co,cr){{$}}
      //
      // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1)
      // PRT-O-SAME:  {{^ *}}map(hold,from: prStruct,c,ci,co,cr,nc) shared(nc,prStruct,c,ci,co,cr){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) copyout(prStruct,c,ci,co,cr,nc){{$}}
      #pragma acc parallel num_gangs(1) copyout(prStruct,c,ci,co,cr,nc)
      // DMP: CompoundStmt
      // PRT-NEXT: {
      {
        // Don't use this uninitialized.
        // PRT-NEXT: nc.i =
        nc.i = 55;
        // PRT-NEXT: printf(
        // PRT:      );
        //      EXE-NEXT: In second acc parallel:
        // EXE-HOST-NEXT:   prStruct.i=1290
        // EXE-HOST-NEXT:          c.i=1210, ci.i=1220, co.i=1135
        // EXE-HOST-NEXT:         cr.i=1145, nc.i=  55
        //  EXE-DEV-NEXT:   prStruct.i=1090
        //  EXE-DEV-NEXT:          c.i=1010, ci.i=1020, co.i=1035
        //  EXE-DEV-NEXT:         cr.i=1045, nc.i=  55
        printf("In second acc parallel:\n"
               "  prStruct.i=%4d\n"
               "         c.i=%4d, ci.i=%4d, co.i=%4d\n"
               "        cr.i=%4d, nc.i=%4d\n",
               prStruct.i, c.i, ci.i, co.i, cr.i, nc.i);
        // PRT-NEXT: prStruct.i +=
        // PRT: nc.i +=
        prStruct.i += 1000;
               c.i += 1000;
              ci.i += 1000;
              co.i += 1000;
              cr.i += 1000;
              nc.i += 1000;
      } // PRT-NEXT: }

      // PRT-NEXT: printf(
      // PRT:      );
      //      EXE-NEXT: After second acc parallel:
      // EXE-HOST-NEXT:   prStruct.i=2290
      // EXE-HOST-NEXT:          c.i=2210, ci.i=2220, co.i=2135
      // EXE-HOST-NEXT:         cr.i=2145, nc.i=1055
      //  EXE-DEV-NEXT:   prStruct.i= 290
      //  EXE-DEV-NEXT:          c.i= 210, ci.i= 220, co.i= 230
      //  EXE-DEV-NEXT:         cr.i= 240, nc.i=1055
      printf("After second acc parallel:\n"
             "   prStruct.i=%4d\n"
             "          c.i=%4d, ci.i=%4d, co.i=%4d\n"
             "         cr.i=%4d, nc.i=%4d\n",
             prStruct.i, c.i, ci.i, co.i, cr.i, nc.i);
      // PRT-NEXT: prStruct.i +=
      // PRT: nc.i +=
      prStruct.i += 100;
             c.i += 100;
            ci.i += 100;
            co.i += 100;
            cr.i += 100;
            nc.i += 100;
    } // PRT-NEXT: }

    // PRT-NEXT: printf(
    // PRT:      );
    //      EXE-NEXT: After acc data:
    // EXE-HOST-NEXT:   prStruct.i=2390
    // EXE-HOST-NEXT:          c.i=2310, ci.i=2320, co.i=2235
    // EXE-HOST-NEXT:         cr.i=2245, nc.i=1155
    //  EXE-DEV-NEXT:   prStruct.i= 390
    //  EXE-DEV-NEXT:          c.i=2010, ci.i= 320, co.i=2035
    //  EXE-DEV-NEXT:         cr.i= 340, nc.i=1155
    printf("After acc data:\n"
           "   prStruct.i=%4d\n"
           "          c.i=%4d, ci.i=%4d, co.i=%4d\n"
           "         cr.i=%4d, nc.i=%4d\n",
           prStruct.i, c.i, ci.i, co.i, cr.i, nc.i);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Check that all data clauses from multiple enclosing acc data directives
  // suppress data clauses on nested acc parallel directives.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: c =
    // PRT: co = 30;
    int  c = 10;
    int ci = 20;
    int co = 30;
    // DMP:      ACCDataDirective
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'int'
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'int'
    // DMP-NOT:      OMP{{.*}}Clause
    // DMP:          CapturedStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc data copy(c){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(hold,tofrom: c){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target data map(hold,tofrom: c){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data copy(c){{$}}
    #pragma acc data copy(c)
    // DMP:      ACCDataDirective
    // DMP-NEXT:   ACCCopyinClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'int'
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'ci' 'int'
    // DMP-NOT:      OMP{{.*}}Clause
    // DMP:          CapturedStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc data copyin(ci){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(hold,to: ci){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target data map(hold,to: ci){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data copyin(ci){{$}}
    #pragma acc data copyin(ci)
    // DMP:      ACCDataDirective
    // DMP-NEXT:   ACCCopyoutClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'int'
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'co' 'int'
    // DMP-NOT:      OMP{{.*}}Clause
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc data copyout(co){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(hold,from: co){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target data map(hold,from: co){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data copyout(co){{$}}
    #pragma acc data copyout(co)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: c +=
      // PRT: co +=
       c += 100;
      ci += 100;
      co += 100;

      // Check suppression of implicit data clauses.
      //
      // DMP:      ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'int'
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'int'
      // DMP-NEXT:   impl: OMPTargetTeamsDirective
      // DMP-NEXT:     OMPNum_teamsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPSharedClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci' 'int'
      // DMP-NEXT:     OMPDefaultmapClause
      // DMP-NOT:        <implicit>
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1)
      // PRT-AO-SAME: {{^ *}}shared(co,c,ci) defaultmap(tofrom: scalar){{$}}
      //
      // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1)
      // PRT-O-SAME:  {{^ *}}shared(co,c,ci) defaultmap(tofrom: scalar){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1){{$}}
      #pragma acc parallel num_gangs(1)
      // DMP: CompoundStmt
      // PRT-NEXT: {
      {
        // Don't use this uninitialized.
        // PRT-NEXT: co =
        co = 40;
        // PRT-NEXT: printf(
        // PRT:      );
        //      EXE-NEXT: In first acc parallel:
        // EXE-HOST-NEXT:   c= 110, ci= 120, co=  40
        //  EXE-DEV-NEXT:   c=  10, ci=  20, co=  40
        printf("In first acc parallel:\n"
               "  c=%4d, ci=%4d, co=%4d\n",
               c, ci, co);
        // PRT-NEXT: c +=
        // PRT: co +=
        c += 1000; ci += 1000; co += 1000;
      } // PRT-NEXT: }

      // PRT-NEXT: printf(
      // PRT:      );
      //      EXE-NEXT: After first acc parallel:
      // EXE-HOST-NEXT:   c=1110, ci=1120, co=1040
      //  EXE-DEV-NEXT:   c= 110, ci= 120, co= 130
      printf("After first acc parallel:\n"
             "  c=%4d, ci=%4d, co=%4d\n",
             c, ci, co);
      // PRT-NEXT: c +=
      // PRT: co +=
      c += 100; ci += 100; co += 100;

      // Check suppression of explicit data clauses (only data transfers are
      // suppressed).
      //
      // DMP:      ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCCopyClause
      // DMP-NOT:      <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'int'
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'int'
      // DMP-NEXT:   impl: OMPTargetTeamsDirective
      // DMP-NEXT:     OMPNum_teamsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPMapClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co' 'int'
      // DMP-NEXT:     OMPSharedClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co' 'int'
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1) copy(c,ci,co){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1)
      // PRT-AO-SAME: {{^ *}}map(hold,tofrom: c,ci,co) shared(c,ci,co){{$}}
      //
      // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1)
      // PRT-O-SAME:  {{^ *}}map(hold,tofrom: c,ci,co) shared(c,ci,co){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) copy(c,ci,co){{$}}
      #pragma acc parallel num_gangs(1) copy(c,ci,co)
      // DMP: CompoundStmt
      // PRT-NEXT: {
      {
        // PRT-NEXT: printf(
        // PRT:      );
        //      EXE-NEXT: In second acc parallel:
        // EXE-HOST-NEXT:   c=1210, ci=1220, co=1140
        //  EXE-DEV-NEXT:   c=1010, ci=1020, co=1040
        printf("In second acc parallel:\n"
               "  c=%4d, ci=%4d, co=%4d\n",
               c, ci, co);
        // PRT-NEXT: c +=
        // PRT: co +=
         c += 1000; ci += 1000; co += 1000;
      } // PRT-NEXT: }

      // PRT-NEXT: printf(
      // PRT:      );
      //      EXE-NEXT: After second acc parallel:
      // EXE-HOST-NEXT:   c=2210, ci=2220, co=2140
      //  EXE-DEV-NEXT:   c= 210, ci= 220, co= 230
      printf("After second acc parallel:\n"
             "   c=%4d, ci=%4d, co=%4d\n",
             c, ci, co);
      // PRT-NEXT: c +=
      // PRT: co +=
      c += 100; ci += 100; co += 100;
    } // PRT-NEXT: }

    // PRT-NEXT: printf(
    // PRT:      );
    //      EXE-NEXT: After acc data:
    // EXE-HOST-NEXT:   c=2310, ci=2320, co=2240
    //  EXE-DEV-NEXT:   c=2010, ci= 320, co=2040
    printf("After acc data:\n"
           "  c=%4d, ci=%4d, co=%4d\n",
           c, ci, co);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Check that a data clause for a subarray from an enclosing acc data
  // directive suppresses data clauses on nested acc parallel.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int arr[] = {{.*}};
    int arr[] = {10, 20, 30, 40, 50};
    // DMP:      ACCDataDirective
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:         DeclRefExpr {{.*}} 'arr' 'int [5]'
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       OMPArraySectionExpr
    // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:           DeclRefExpr {{.*}} 'arr' 'int [5]'
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:         <<<NULL>>>
    // DMP-NOT:      OMP{{.*}}Clause
    // DMP:          CapturedStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc data copy(arr[1:3]){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(hold,tofrom: arr[1:3]){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target data map(hold,tofrom: arr[1:3]){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data copy(arr[1:3]){{$}}
    #pragma acc data copy(arr[1:3])
    // PRT-NEXT: {
    {
      // PRT-NEXT: arr[1] +=
      // PRT-NEXT: arr[2] +=
      // PRT-NEXT: arr[3] +=
      arr[1] += 100;
      arr[2] += 100;
      arr[3] += 100;

      // Check suppression of implicit data clauses.
      //
      // DMP:      ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'arr' 'int [5]'
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'arr' 'int [5]'
      // DMP-NEXT:   impl: OMPTargetTeamsDirective
      // DMP-NEXT:     OMPNum_teamsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPSharedClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'arr' 'int [5]'
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1)
      // PRT-AO-SAME: {{^ *}}shared(arr){{$}}
      //
      // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1)
      // PRT-O-SAME:  {{^ *}}shared(arr){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1){{$}}
      #pragma acc parallel num_gangs(1)
      // DMP: CompoundStmt
      // PRT-NEXT: {
      {
        // PRT-NEXT: printf(
        // PRT:      );
        //      EXE-NEXT: In first acc parallel:
        // EXE-HOST-NEXT:   arr[1]= 120, arr[2]= 130, arr[3]= 140
        //  EXE-DEV-NEXT:   arr[1]=  20, arr[2]=  30, arr[3]=  40
        printf("In first acc parallel:\n"
               "  arr[1]=%4d, arr[2]=%4d, arr[3]=%4d\n",
               arr[1], arr[2], arr[3]);
        // PRT-NEXT: arr[1] +=
        // PRT: arr[3] +=
        arr[1] += 1000; arr[2] += 1000; arr[3] += 1000;
      } // PRT-NEXT: }

      // PRT-NEXT: printf(
      // PRT:      );
      //      EXE-NEXT: After first acc parallel:
      // EXE-HOST-NEXT:   arr[1]=1120, arr[2]=1130, arr[3]=1140
      //  EXE-DEV-NEXT:   arr[1]= 120, arr[2]= 130, arr[3]= 140
      printf("After first acc parallel:\n"
             "  arr[1]=%4d, arr[2]=%4d, arr[3]=%4d\n",
             arr[1], arr[2], arr[3]);
      // PRT-NEXT: arr[1] +=
      // PRT: arr[3] +=
      arr[1] += 100; arr[2] += 100; arr[3] += 100;

      // Check suppression of explicit data clauses (only data transfers are
      // suppressed).
      //
      // DMP:      ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCCopyClause
      // DMP-NOT:      <implicit>
      // DMP-NEXT:     OMPArraySectionExpr
      // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      // DMP-NEXT:         DeclRefExpr {{.*}} 'arr' 'int [5]'
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
      // DMP-NEXT:       <<<NULL>>>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'arr' 'int [5]'
      // DMP-NEXT:   impl: OMPTargetTeamsDirective
      // DMP-NEXT:     OMPNum_teamsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPMapClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       OMPArraySectionExpr
      // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      // DMP-NEXT:           DeclRefExpr {{.*}} 'arr' 'int [5]'
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
      // DMP-NEXT:         <<<NULL>>>
      // DMP-NEXT:     OMPSharedClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'arr' 'int [5]'
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1) copy(arr[1:3]){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1)
      // PRT-AO-SAME: {{^ *}}map(hold,tofrom: arr[1:3]) shared(arr){{$}}
      //
      // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1)
      // PRT-O-SAME:  {{^ *}}map(hold,tofrom: arr[1:3]) shared(arr){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) copy(arr[1:3]){{$}}
      #pragma acc parallel num_gangs(1) copy(arr[1:3])
      // DMP: CompoundStmt
      // PRT-NEXT: {
      {
        // PRT-NEXT: printf(
        // PRT:      );
        //      EXE-NEXT: In second acc parallel:
        // EXE-HOST-NEXT:   arr[1]=1220, arr[2]=1230, arr[3]=1240
        //  EXE-DEV-NEXT:   arr[1]=1020, arr[2]=1030, arr[3]=1040
        printf("In second acc parallel:\n"
               "  arr[1]=%4d, arr[2]=%4d, arr[3]=%4d\n",
               arr[1], arr[2], arr[3]);
        // PRT-NEXT: arr[1] +=
        // PRT: arr[3] +=
        arr[1] += 1000; arr[2] += 1000; arr[3] += 1000;
      } // PRT-NEXT: }

      // PRT-NEXT: printf(
      // PRT:      );
      //      EXE-NEXT: After second acc parallel:
      // EXE-HOST-NEXT:   arr[1]=2220, arr[2]=2230, arr[3]=2240
      //  EXE-DEV-NEXT:   arr[1]= 220, arr[2]= 230, arr[3]= 240
      printf("After second acc parallel:\n"
             "  arr[1]=%4d, arr[2]=%4d, arr[3]=%4d\n",
             arr[1], arr[2], arr[3]);
      // PRT-NEXT: arr[1] +=
      // PRT: arr[3] +=
      arr[1] += 100; arr[2] += 100; arr[3] += 100;
    } // PRT-NEXT: }

    // PRT-NEXT: printf(
    // PRT:      );
    //      EXE-NEXT: After acc data:
    // EXE-HOST-NEXT:   arr[1]=2320, arr[2]=2330, arr[3]=2340
    //  EXE-DEV-NEXT:   arr[1]=2020, arr[2]=2030, arr[3]=2040
    printf("After acc data:\n"
           "  arr[1]=%4d, arr[2]=%4d, arr[3]=%4d\n",
           arr[1], arr[2], arr[3]);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Repeat that but with non-constant start/length subarray expressions.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int s0 =
    int s0 = 1, l0 = 3;
    // PRT-NEXT: int s1 =
    int s1 = 1, l1 = 3;
    // PRT-NEXT: int arr[] = {{.*}};
    int arr[] = {10, 20, 30, 40, 50};
    // DMP:      ACCDataDirective
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:         DeclRefExpr {{.*}} 'arr' 'int [5]'
    // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
    // DMP-NEXT:         DeclRefExpr {{.*}} 's0' 'int'
    // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
    // DMP-NEXT:         DeclRefExpr {{.*}} 'l0' 'int'
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       OMPArraySectionExpr
    // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:           DeclRefExpr {{.*}} 'arr' 'int [5]'
    // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
    // DMP-NEXT:           DeclRefExpr {{.*}} 's0' 'int'
    // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
    // DMP-NEXT:           DeclRefExpr {{.*}} 'l0' 'int'
    // DMP-NEXT:         <<<NULL>>>
    // DMP-NOT:      OMP{{.*}}Clause
    // DMP:          CapturedStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc data copy(arr[s0:l0]){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(hold,tofrom: arr[s0:l0]){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target data map(hold,tofrom: arr[s0:l0]){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data copy(arr[s0:l0]){{$}}
    #pragma acc data copy(arr[s0:l0])
    // PRT-NEXT: {
    {
      // PRT-NEXT: arr[1] +=
      // PRT-NEXT: arr[2] +=
      // PRT-NEXT: arr[3] +=
      arr[1] += 100;
      arr[2] += 100;
      arr[3] += 100;

      // Check suppression of implicit data clauses.
      //
      // DMP:      ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'arr' 'int [5]'
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'arr' 'int [5]'
      // DMP-NEXT:   impl: OMPTargetTeamsDirective
      // DMP-NEXT:     OMPNum_teamsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPSharedClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'arr' 'int [5]'
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1)
      // PRT-AO-SAME: {{^ *}}shared(arr){{$}}
      //
      // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1)
      // PRT-O-SAME:  {{^ *}}shared(arr){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1){{$}}
      #pragma acc parallel num_gangs(1)
      // DMP: CompoundStmt
      // PRT-NEXT: {
      {
        // PRT-NEXT: printf(
        // PRT:      );
        //      EXE-NEXT: In first acc parallel:
        // EXE-HOST-NEXT:   arr[1]= 120, arr[2]= 130, arr[3]= 140
        //  EXE-DEV-NEXT:   arr[1]=  20, arr[2]=  30, arr[3]=  40
        printf("In first acc parallel:\n"
               "  arr[1]=%4d, arr[2]=%4d, arr[3]=%4d\n",
               arr[1], arr[2], arr[3]);
        // PRT-NEXT: arr[1] +=
        // PRT: arr[3] +=
        arr[1] += 1000; arr[2] += 1000; arr[3] += 1000;
      } // PRT-NEXT: }

      // PRT-NEXT: printf(
      // PRT:      );
      //      EXE-NEXT: After first acc parallel:
      // EXE-HOST-NEXT:   arr[1]=1120, arr[2]=1130, arr[3]=1140
      //  EXE-DEV-NEXT:   arr[1]= 120, arr[2]= 130, arr[3]= 140
      printf("After first acc parallel:\n"
             "  arr[1]=%4d, arr[2]=%4d, arr[3]=%4d\n",
             arr[1], arr[2], arr[3]);
      // PRT-NEXT: arr[1] +=
      // PRT: arr[3] +=
      arr[1] += 100; arr[2] += 100; arr[3] += 100;

      // Check suppression of explicit data clauses (only data transfers are
      // suppressed).
      //
      // DMP:      ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCCopyClause
      // DMP-NOT:      <implicit>
      // DMP-NEXT:     OMPArraySectionExpr
      // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      // DMP-NEXT:         DeclRefExpr {{.*}} 'arr' 'int [5]'
      // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      // DMP-NEXT:         DeclRefExpr {{.*}} 's1' 'int'
      // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      // DMP-NEXT:         DeclRefExpr {{.*}} 'l1' 'int'
      // DMP-NEXT:       <<<NULL>>>
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'arr' 'int [5]'
      // DMP-NEXT:   impl: OMPTargetTeamsDirective
      // DMP-NEXT:     OMPNum_teamsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPMapClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       OMPArraySectionExpr
      // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      // DMP-NEXT:           DeclRefExpr {{.*}} 'arr' 'int [5]'
      // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      // DMP-NEXT:           DeclRefExpr {{.*}} 's1' 'int'
      // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      // DMP-NEXT:           DeclRefExpr {{.*}} 'l1' 'int'
      // DMP-NEXT:         <<<NULL>>>
      // DMP-NEXT:     OMPSharedClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'arr' 'int [5]'
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1) copy(arr[s1:l1]){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1)
      // PRT-AO-SAME: {{^ *}}map(hold,tofrom: arr[s1:l1]) shared(arr){{$}}
      //
      // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1)
      // PRT-O-SAME:  {{^ *}}map(hold,tofrom: arr[s1:l1]) shared(arr){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) copy(arr[s1:l1]){{$}}
      #pragma acc parallel num_gangs(1) copy(arr[s1:l1])
      // DMP: CompoundStmt
      // PRT-NEXT: {
      {
        // PRT-NEXT: printf(
        // PRT:      );
        //      EXE-NEXT: In second acc parallel:
        // EXE-HOST-NEXT:   arr[1]=1220, arr[2]=1230, arr[3]=1240
        //  EXE-DEV-NEXT:   arr[1]=1020, arr[2]=1030, arr[3]=1040
        printf("In second acc parallel:\n"
               "  arr[1]=%4d, arr[2]=%4d, arr[3]=%4d\n",
               arr[1], arr[2], arr[3]);
        // PRT-NEXT: arr[1] +=
        // PRT: arr[3] +=
        arr[1] += 1000; arr[2] += 1000; arr[3] += 1000;
      } // PRT-NEXT: }

      // PRT-NEXT: printf(
      // PRT:      );
      //      EXE-NEXT: After second acc parallel:
      // EXE-HOST-NEXT:   arr[1]=2220, arr[2]=2230, arr[3]=2240
      //  EXE-DEV-NEXT:   arr[1]= 220, arr[2]= 230, arr[3]= 240
      printf("After second acc parallel:\n"
             "  arr[1]=%4d, arr[2]=%4d, arr[3]=%4d\n",
             arr[1], arr[2], arr[3]);
      // PRT-NEXT: arr[1] +=
      // PRT: arr[3] +=
      arr[1] += 100; arr[2] += 100; arr[3] += 100;
    } // PRT-NEXT: }

    // PRT-NEXT: printf(
    // PRT:      );
    //      EXE-NEXT: After acc data:
    // EXE-HOST-NEXT:   arr[1]=2320, arr[2]=2330, arr[3]=2340
    //  EXE-DEV-NEXT:   arr[1]=2020, arr[2]=2030, arr[3]=2040
    printf("After acc data:\n"
           "  arr[1]=%4d, arr[2]=%4d, arr[3]=%4d\n",
           arr[1], arr[2], arr[3]);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // What is mapped when two outer acc data directives specify different
  // (non-overlapping) subarrays and there is no data clause on the acc
  // parallel?
  //
  // The behavior in this case does not appear to be well defined in OpenACC
  // 3.0 or OpenMP 5.0.  We assume the subarray earliest in memory specifies the
  // mapping, and we check that it is indeed mapped for the acc parallel.  We
  // could manually check that the later subarray isn't mapped by writing to it
  // and observing that the new value isn't copied back to the host, but writing
  // to unmapped memory could introduce races, so we don't check that in the
  // test suite.
  //
  // The purpose of this test is to detect changes in OpenACC behavior as Clang
  // evolves to conform to future versions of the OpenMP spec.  See the section
  // "Basic Data Attributes" in the Clang OpenACC design document for further
  // discussion.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int arr0[] = {{.*}};
    // PRT-NEXT: int arr1[] = {{.*}};
    int arr0[] = {10, 20, 30, 40, 50};
    int arr1[] = {11, 21, 31, 41, 51};
    // DMP:      ACCDataDirective
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:         DeclRefExpr {{.*}} 'arr0' 'int [5]'
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:         DeclRefExpr {{.*}} 'arr1' 'int [5]'
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       OMPArraySectionExpr
    // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:           DeclRefExpr {{.*}} 'arr0' 'int [5]'
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:         <<<NULL>>>
    // DMP-NEXT:       OMPArraySectionExpr
    // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:           DeclRefExpr {{.*}} 'arr1' 'int [5]'
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:         <<<NULL>>>
    // DMP-NOT:      OMP{{.*}}Clause
    // DMP:          CapturedStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc data copy(arr0[0:2],arr1[2:3]){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(hold,tofrom: arr0[0:2],arr1[2:3]){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target data map(hold,tofrom: arr0[0:2],arr1[2:3]){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data copy(arr0[0:2],arr1[2:3]){{$}}
    #pragma acc data copy(arr0[0:2],arr1[2:3])
    // DMP:      ACCDataDirective
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:         DeclRefExpr {{.*}} 'arr0' 'int [5]'
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:         DeclRefExpr {{.*}} 'arr1' 'int [5]'
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       OMPArraySectionExpr
    // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:           DeclRefExpr {{.*}} 'arr0' 'int [5]'
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:         <<<NULL>>>
    // DMP-NEXT:       OMPArraySectionExpr
    // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:           DeclRefExpr {{.*}} 'arr1' 'int [5]'
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:         <<<NULL>>>
    // DMP-NOT:      OMP{{.*}}Clause
    // DMP:          CapturedStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc data copy(arr0[2:3],arr1[0:2]){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(hold,tofrom: arr0[2:3],arr1[0:2]){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target data map(hold,tofrom: arr0[2:3],arr1[0:2]){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data copy(arr0[2:3],arr1[0:2]){{$}}
    #pragma acc data copy(arr0[2:3],arr1[0:2])
    // Check suppression of implicit data clauses.
    //
    // DMP:      ACCParallelDirective
    // DMP-NEXT:   ACCNumGangsClause
    // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'arr0' 'int [5]'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'arr1' 'int [5]'
    // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'arr0' 'int [5]'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'arr1' 'int [5]'
    // DMP-NEXT:   impl: OMPTargetTeamsDirective
    // DMP-NEXT:     OMPNum_teamsClause
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:     OMPSharedClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'arr0' 'int [5]'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'arr1' 'int [5]'
    // DMP-NOT:      OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1)
    // PRT-AO-SAME: {{^ *}}shared(arr0,arr1){{$}}
    //
    // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1)
    // PRT-O-SAME:  {{^ *}}shared(arr0,arr1){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1){{$}}
    #pragma acc parallel num_gangs(1)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: arr0[0] +=
      // PRT-NEXT: arr1[0] +=
      arr0[0] += 1000;
      arr1[0] += 1000;
    } // PRT-NEXT: }
    // PRT-NEXT: printf(
    // PRT:      );
    // EXE-NEXT: After acc data:
    // EXE-NEXT:   arr0[0]=1010, arr1[0]=1011
    printf("After acc data:\n"
           "  arr0[0]=%4d, arr1[0]=%4d\n",
           arr0[0], arr1[0]);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // defaultmap for scalars with suppressed OpenACC implicit DAs: Check that an
  // inherited no_alloc prevents adding it.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int x =
    // PRT-NEXT: int use =
    int x = 10;
    int use = 0;
    //  PRT-A-NEXT: {{^ *}}#pragma acc data no_create(x){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(no_alloc,hold,alloc: x){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target data map(no_alloc,hold,alloc: x){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data no_create(x){{$}}
    #pragma acc data no_create(x)
    // PRT-NEXT: {
    {
      // no_alloc inherited from parent.
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(1) copy(use){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1) map(hold,tofrom: use) map(no_alloc,alloc: x) shared(use,x){{$}}
      //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(1) map(hold,tofrom: use) map(no_alloc,alloc: x) shared(use,x){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) copy(use){{$}}
      #pragma acc parallel num_gangs(1) copy(use)
      // PRT-NEXT: if (use)
      // PRT-NEXT:   x +=
      if (use)
        x += 100;

      // no_alloc inherited from grandparent.
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc data copy(use){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(hold,tofrom: use){{$}}
      //  PRT-O-NEXT: {{^ *}}#pragma omp target data map(hold,tofrom: use){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc data copy(use){{$}}
      #pragma acc data copy(use)
      // PRT-NEXT: {
      {
        //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(1) copy(use){{$}}
        // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1) map(hold,tofrom: use) map(no_alloc,alloc: x) shared(use,x){{$}}
        //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(1) map(hold,tofrom: use) map(no_alloc,alloc: x) shared(use,x){{$}}
        // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) copy(use){{$}}
        #pragma acc parallel num_gangs(1) copy(use)
        // PRT-NEXT: if (use)
        // PRT-NEXT:   x +=
        if (use)
          x += 100;
      } // PRT-NEXT: }
    } // PRT-NEXT: }
    // PRT-NEXT: printf(
    // EXE-NEXT: x=10
    printf("x=%d\n", x);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // defaultmap for scalars with suppressed OpenACC implicit DAs: Check that an
  // an explicit DSA prevents adding it.
  //
  // The case where implicit DAs are not suppressed by an enclosing acc data and
  // thus prevent defaultmap is tested in, for example, parallel-da.c.
  //
  // Cases where it isn't added because the variable is non-scalar are tested
  // above.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int x =
    int x = 10;
    //  PRT-A-NEXT: {{^ *}}#pragma acc data copy(x){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(hold,tofrom: x){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target data map(hold,tofrom: x){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data copy(x){{$}}
    #pragma acc data copy(x)
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(1) firstprivate(x){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1) firstprivate(x){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(1) firstprivate(x){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) firstprivate(x){{$}}
    #pragma acc parallel num_gangs(1) firstprivate(x)
    // PRT-NEXT: x +=
    x += 100;
    // PRT-NEXT: printf(
    // EXE-NEXT: x=10
    printf("x=%d\n", x);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // defaultmap for scalars with suppressed OpenACC implicit DAs: Check that the
  // record of the need for it doesn't accidentally persist to a sibling acc
  // parallel.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int x =
    // PRT-NEXT: int y =
    int x = 10;
    int y = 20;
    //  PRT-A-NEXT: {{^ *}}#pragma acc data copy(x){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(hold,tofrom: x){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target data map(hold,tofrom: x){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data copy(x){{$}}
    #pragma acc data copy(x)
    // PRT-NEXT: {
    {
      //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(1){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1) shared(x) defaultmap(tofrom: scalar){{$}}
      //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(1) shared(x) defaultmap(tofrom: scalar){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1){{$}}
      #pragma acc parallel num_gangs(1)
      // PRT-NEXT: x +=
      x += 100;
      //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(1){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1) firstprivate(y){{$}}
      //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(1) firstprivate(y){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1){{$}}
      #pragma acc parallel num_gangs(1)
      // PRT-NEXT: y +=
      y += 200;
    } // PRT-NEXT: }
    // PRT-NEXT: printf(
    // EXE-NEXT: x=110, y=20
    printf("x=%d, y=%d\n", x, y);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Inherited no_alloc: Check that an explicit DMA or DSA prevents it.
  //
  // The case where -fopenacc-no-create-omp=no-no_alloc prevents it is checked
  // in no-create.c.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: int x =
    // PRT-NEXT: int y =
    int x = 10;
    int y = 20;
    //  PRT-A-NEXT: {{^ *}}#pragma acc data no_create(x,y){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(no_alloc,hold,alloc: x,y){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target data map(no_alloc,hold,alloc: x,y){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data no_create(x,y){{$}}
    #pragma acc data no_create(x,y)
    //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(1) copy(x) firstprivate(y){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1) map(hold,tofrom: x) firstprivate(y) shared(x){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(1) map(hold,tofrom: x) firstprivate(y) shared(x){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) copy(x) firstprivate(y){{$}}
    #pragma acc parallel num_gangs(1) copy(x) firstprivate(y)
    // PRT-NEXT: {
    {
      // PRT-NEXT: x +=
      // PRT-NEXT: y +=
      x += 100;
      y += 200;
    } // PRT-NEXT: }
    // PRT-NEXT: printf(
    // EXE-NEXT: x=110, y=20
    printf("x=%d, y=%d\n", x, y);
  } // PRT-NEXT: }
} // PRT-NEXT: }
// EXE-NOT: {{.}}
