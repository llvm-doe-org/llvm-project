// Check "acc data".
//
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
// RUN: | FileCheck -check-prefixes=PRT,PRT-NOACC %s
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
// RUN:   (prt=-fopenacc-print=acc                          prt-chk=PRT-A,PRT,PRT-SRC)
// RUN:   (prt=-fopenacc-print=omp                          prt-chk=PRT-O,PRT,PRT-SRC)
// RUN:   (prt=-fopenacc-print=acc-omp                      prt-chk=PRT-A,PRT-AO,PRT,PRT-SRC)
// RUN:   (prt=-fopenacc-print=omp-acc                      prt-chk=PRT-O,PRT-OA,PRT,PRT-SRC)
// RUN: }
// RUN: %for prt-args {
// RUN:   %clang -Xclang -verify %[prt] %t-acc.c \
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
// RUN:   | FileCheck -check-prefixes=%[prt-chk] %s
// RUN: }

// Can we print the OpenMP source code, compile, and run it successfully?
//
// RUN: %for prt-opts {
// RUN:   %clang -Xclang -verify %[prt-opt]=omp %s > %t-omp.c
// RUN:   echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN:   %clang -Xclang -verify -fopenmp %fopenmp-version -o %t %t-omp.c
// RUN:   %t 2 2>&1 | FileCheck -check-prefixes=EXE,EXE-TGT-HOST,EXE-HOST %s
// RUN: }

// Check execution with normal compilation.
//
// RUN: %data tgts {
// RUN:   (run-if=                tgt=HOST    tgt-cflags=                                     host-or-dev=HOST)
// RUN:   (run-if=%run-if-x86_64  tgt=X86_64  tgt-cflags=-fopenmp-targets=%run-x86_64-triple  host-or-dev=DEV )
// RUN:   (run-if=%run-if-ppc64le tgt=PPC64LE tgt-cflags=-fopenmp-targets=%run-ppc64le-triple host-or-dev=DEV )
// RUN:   (run-if=%run-if-nvptx64 tgt=NVPTX64 tgt-cflags=-fopenmp-targets=%run-nvptx64-triple host-or-dev=DEV )
// RUN: }
// RUN: %for tgts {
// RUN:   %[run-if] %clang -Xclang -verify -fopenacc %s -o %t %[tgt-cflags]
// RUN:   %[run-if] %t 2 > %t.out 2>&1
// RUN:   %[run-if] FileCheck -input-file %t.out %s -strict-whitespace \
// RUN:                       -check-prefixes=EXE,EXE-TGT-%[tgt],EXE-%[host-or-dev]
// RUN: }

// END.

// expected-no-diagnostics

#include <stdio.h>

// PRT: int main() {
int main() {
  // PRT-NEXT: printf("start\n");
  // EXE: start
  printf("start\n");

  //--------------------------------------------------
  // Check acc data without nested directive.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: c =
    // PRT-NEXT: ci =
    // PRT-NEXT: co =
    int  c = 10;
    int ci = 20;
    int co = 30;
    // DMP:      ACCDataDirective
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'int'
    // DMP-NEXT:   ACCCopyinClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'int'
    // DMP-NEXT:   ACCCopyoutClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'int'
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'int'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'ci' 'int'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'co' 'int'
    // DMP-NOT:      OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc data copy(c) copyin(ci) copyout(co){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(tofrom: c) map(to: ci) map(from: co){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target data map(tofrom: c) map(to: ci) map(from: co){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data copy(c) copyin(ci) copyout(co){{$}}
    #pragma acc data copy(c) copyin(ci) copyout(co)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: c +=
      // PRT-NEXT: ci +=
      // PRT-NEXT: co +=
       c += 100;
      ci += 100;
      co += 100;
      // PRT-NEXT: printf(
      // PRT:      );
      //      EXE-NEXT: Inside acc data:
      // EXE-HOST-NEXT:   c=110, ci=120, co=130
      //  EXE-DEV-NEXT:   c=110, ci=120, co=130
      printf("Inside acc data:\n"
             "  c=%3d, ci=%3d, co=%3d\n",
             c, ci, co);
    } // PRT-NEXT: }
    // PRT-NEXT: printf(
    // PRT:      );
    //      EXE-NEXT: After acc data:
    // EXE-HOST-NEXT:   c=110, ci=120, co=130
    //  EXE-DEV-NEXT:   c= 10, ci=120, co=
    printf("After acc data:\n"
           "  c=%3d, ci=%3d, co=%3d\n",
           c, ci, co);
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
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(tofrom: c0){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target data map(tofrom: c0){{$}}
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
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(tofrom: c0,c1){{$}}
      // PRT-O-NEXT:  {{^ *}}#pragma omp target data map(tofrom: c0,c1){{$}}
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
        // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(tofrom: c0,c1,c2){{$}}
        // PRT-O-NEXT:  {{^ *}}#pragma omp target data map(tofrom: c0,c1,c2){{$}}
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
    // PRT-NEXT: c0 =
    // PRT: co2 =
    int  c0 = 10,  c1 = 11,  c2 = 12;
    int ci0 = 20, ci1 = 21, ci2 = 22;
    int co0 = 30, co1 = 31, co2 = 32;
    // DMP:      ACCDataDirective
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
    // DMP-NEXT:   impl: OMPTargetDataDirective
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
    // DMP-NOT:      OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc data copy(c0) pcopy(c1) present_or_copy(c2){{( *\\$[[:space:]])?}}
    // PRT-A-SAME:  {{^ *}}copyin(ci0) pcopyin(ci1) present_or_copyin(ci2){{( *\\$[[:space:]])?}}
    // PRT-A-SAME:  {{^ *}}copyout(co0) pcopyout(co1) present_or_copyout(co2){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(tofrom: c0) map(tofrom: c1) map(tofrom: c2)
    // PRT-AO-SAME: {{^ *}}map(to: ci0) map(to: ci1) map(to: ci2)
    // PRT-AO-SAME: {{^ *}}map(from: co0) map(from: co1) map(from: co2){{$}}
    //
    // PRT-O-NEXT: {{^ *}}#pragma omp target data map(tofrom: c0) map(tofrom: c1) map(tofrom: c2)
    // PRT-O-SAME: {{^ *}}map(to: ci0) map(to: ci1) map(to: ci2)
    // PRT-O-SAME: {{^ *}}map(from: co0) map(from: co1) map(from: co2){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data copy(c0) pcopy(c1) present_or_copy(c2){{( *\\$[[:space:]] *//)?}}
    // PRT-OA-SAME: {{^ *}}copyin(ci0) pcopyin(ci1) present_or_copyin(ci2){{( *\\$[[:space:]] *//)?}}
    // PRT-OA-SAME: {{^ *}}copyout(co0) pcopyout(co1) present_or_copyout(co2){{$}}
    #pragma acc data copy(c0) pcopy(c1) present_or_copy(c2)             \
                     copyin(ci0) pcopyin(ci1) present_or_copyin(ci2)    \
                     copyout(co0) pcopyout(co1) present_or_copyout(co2)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: c0 +=
      // PRT: co2 +=
       c0 += 100;  c1 += 100;  c2 += 100;
      ci0 += 100; ci1 += 100; ci2 += 100;
      co0 += 100; co1 += 100; co2 += 100;

      // Check suppression of implicit data clauses.
      //
      // DMP:      ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co2' 'int'
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
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c2' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci2' 'int'
      // DMP-NEXT:   impl: OMPTargetTeamsDirective
      // DMP-NEXT:     OMPNum_teamsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPSharedClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co0' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co1' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co2' 'int'
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
      // PRT-AO-SAME: {{^ *}}shared(co0,co1,co2,c0,c1,c2,ci0,ci1,ci2)
      // PRT-AO-SAME: {{^ *}}defaultmap(tofrom: scalar){{$}}
      //
      // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1)
      // PRT-O-SAME:  {{^ *}}shared(co0,co1,co2,c0,c1,c2,ci0,ci1,ci2)
      // PRT-O-SAME:  {{^ *}}defaultmap(tofrom: scalar){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1){{$}}
      #pragma acc parallel num_gangs(1)
      // DMP: CompoundStmt
      // PRT-NEXT: {
      {
        // Don't use these uninitialized.
        // PRT-NEXT: co0 =
        // PRT: co2 =
        co0 = 40; co1 = 41; co2 = 42;
        // PRT-NEXT: printf(
        // PRT:      );
        //      EXE-NEXT: In first acc parallel:
        // EXE-HOST-NEXT:    c0= 110,  c1= 111,  c2= 112
        // EXE-HOST-NEXT:   ci0= 120, ci1= 121, ci2= 122
        // EXE-HOST-NEXT:   co0=  40, co1=  41, co2=  42
        //  EXE-DEV-NEXT:    c0=  10,  c1=  11,  c2=  12
        //  EXE-DEV-NEXT:   ci0=  20, ci1=  21, ci2=  22
        //  EXE-DEV-NEXT:   co0=  40, co1=  41, co2=  42
        printf("In first acc parallel:\n"
               "   c0=%4d,  c1=%4d,  c2=%4d\n"
               "  ci0=%4d, ci1=%4d, ci2=%4d\n"
               "  co0=%4d, co1=%4d, co2=%4d\n",
               c0, c1, c2, ci0, ci1, ci2, co0, co1, co2);
        // PRT-NEXT: c0 +=
        // PRT: co2 +=
         c0 += 1000;  c1 += 1000;  c2 += 1000;
        ci0 += 1000; ci1 += 1000; ci2 += 1000;
        co0 += 1000; co1 += 1000; co2 += 1000;
      } // PRT-NEXT: }

      // PRT-NEXT: printf(
      // PRT:      );
      //      EXE-NEXT: After first acc parallel:
      // EXE-HOST-NEXT:    c0=1110,  c1=1111,  c2=1112
      // EXE-HOST-NEXT:   ci0=1120, ci1=1121, ci2=1122
      // EXE-HOST-NEXT:   co0=1040, co1=1041, co2=1042
      //  EXE-DEV-NEXT:    c0= 110,  c1= 111,  c2= 112
      //  EXE-DEV-NEXT:   ci0= 120, ci1= 121, ci2= 122
      //  EXE-DEV-NEXT:   co0= 130, co1= 131, co2= 132
      printf("After first acc parallel:\n"
             "   c0=%4d,  c1=%4d,  c2=%4d\n"
             "  ci0=%4d, ci1=%4d, ci2=%4d\n"
             "  co0=%4d, co1=%4d, co2=%4d\n",
             c0, c1, c2, ci0, ci1, ci2, co0, co1, co2);
      // PRT-NEXT: c0 +=
      // PRT: co2 +=
       c0 += 100;  c1 += 100;  c2 += 100;
      ci0 += 100; ci1 += 100; ci2 += 100;
      co0 += 100; co1 += 100; co2 += 100;

      // Check suppression of explicit data clauses (only data transfers are
      // suppressed).
      //
      // DMP:      ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCCopyClause
      // DMP-NOT:      <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c2' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci2' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co2' 'int'
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c2' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci2' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co0' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co1' 'int'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co2' 'int'
      // DMP-NEXT:   impl: OMPTargetTeamsDirective
      // DMP-NEXT:     OMPNum_teamsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPMapClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c0' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c1' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c2' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci0' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci1' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci2' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co0' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co1' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co2' 'int'
      // DMP-NEXT:     OMPSharedClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c0' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c1' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c2' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci0' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci1' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci2' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co0' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co1' 'int'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co2' 'int'
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1)
      // PRT-A-SAME:  {{^ *}}copy(c0,c1,c2,ci0,ci1,ci2,co0,co1,co2){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1)
      // PRT-AO-SAME: {{^ *}}map(tofrom: c0,c1,c2,ci0,ci1,ci2,co0,co1,co2)
      // PRT-AO-SAME: {{^ *}}shared(c0,c1,c2,ci0,ci1,ci2,co0,co1,co2){{$}}
      //
      // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1)
      // PRT-O-SAME:  {{^ *}}map(tofrom: c0,c1,c2,ci0,ci1,ci2,co0,co1,co2)
      // PRT-O-SAME:  {{^ *}}shared(c0,c1,c2,ci0,ci1,ci2,co0,co1,co2){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1)
      // PRT-OA-SAME: {{^ *}}copy(c0,c1,c2,ci0,ci1,ci2,co0,co1,co2){{$}}
      #pragma acc parallel num_gangs(1) copy(c0,c1,c2,ci0,ci1,ci2,co0,co1,co2)
      // DMP: CompoundStmt
      // PRT-NEXT: {
      {
        // PRT-NEXT: printf(
        // PRT:      );
        //      EXE-NEXT: In second acc parallel:
        // EXE-HOST-NEXT:    c0=1210,  c1=1211,  c2=1212
        // EXE-HOST-NEXT:   ci0=1220, ci1=1221, ci2=1222
        // EXE-HOST-NEXT:   co0=1140, co1=1141, co2=1142
        //  EXE-DEV-NEXT:    c0=1010,  c1=1011,  c2=1012
        //  EXE-DEV-NEXT:   ci0=1020, ci1=1021, ci2=1022
        //  EXE-DEV-NEXT:   co0=1040, co1=1041, co2=1042
        printf("In second acc parallel:\n"
               "   c0=%4d,  c1=%4d,  c2=%4d\n"
               "  ci0=%4d, ci1=%4d, ci2=%4d\n"
               "  co0=%4d, co1=%4d, co2=%4d\n",
               c0, c1, c2, ci0, ci1, ci2, co0, co1, co2);
        // PRT-NEXT: c0 +=
        // PRT: co2 +=
         c0 += 1000;  c1 += 1000;  c2 += 1000;
        ci0 += 1000; ci1 += 1000; ci2 += 1000;
        co0 += 1000; co1 += 1000; co2 += 1000;
      } // PRT-NEXT: }

      // PRT-NEXT: printf(
      // PRT:      );
      //      EXE-NEXT: After second acc parallel:
      // EXE-HOST-NEXT:    c0=2210,  c1=2211,  c2=2212
      // EXE-HOST-NEXT:   ci0=2220, ci1=2221, ci2=2222
      // EXE-HOST-NEXT:   co0=2140, co1=2141, co2=2142
      //  EXE-DEV-NEXT:    c0= 210,  c1= 211,  c2= 212
      //  EXE-DEV-NEXT:   ci0= 220, ci1= 221, ci2= 222
      //  EXE-DEV-NEXT:   co0= 230, co1= 231, co2= 232
      printf("After second acc parallel:\n"
             "   c0=%4d,  c1=%4d,  c2=%4d\n"
             "  ci0=%4d, ci1=%4d, ci2=%4d\n"
             "  co0=%4d, co1=%4d, co2=%4d\n",
             c0, c1, c2, ci0, ci1, ci2, co0, co1, co2);
      // PRT-NEXT: c0 +=
      // PRT: co2 +=
       c0 += 100;  c1 += 100;  c2 += 100;
      ci0 += 100; ci1 += 100; ci2 += 100;
      co0 += 100; co1 += 100; co2 += 100;
    } // PRT-NEXT: }

    // PRT-NEXT: printf(
    // PRT:      );
    //      EXE-NEXT: After acc data:
    // EXE-HOST-NEXT:    c0=2310,  c1=2311,  c2=2312
    // EXE-HOST-NEXT:   ci0=2320, ci1=2321, ci2=2322
    // EXE-HOST-NEXT:   co0=2240, co1=2241, co2=2242
    //  EXE-DEV-NEXT:    c0=2010,  c1=2011,  c2=2012
    //  EXE-DEV-NEXT:   ci0= 320, ci1= 321, ci2= 322
    //  EXE-DEV-NEXT:   co0=2040, co1=2041, co2=2042
    printf("After acc data:\n"
           "   c0=%4d,  c1=%4d,  c2=%4d\n"
           "  ci0=%4d, ci1=%4d, ci2=%4d\n"
           "  co0=%4d, co1=%4d, co2=%4d\n",
           c0, c1, c2, ci0, ci1, ci2, co0, co1, co2);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Repeat except...
  //
  // Don't bother checking clause aliases again.
  // Check array types.
  // Check suppression of inner copyin clause data transfers.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: c[] =
    // PRT: co[] = {30};
    int  c[] = {10};
    int ci[] = {20};
    int co[] = {30};
    // DMP:      ACCDataDirective
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'int [1]'
    // DMP-NEXT:   ACCCopyinClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'int [1]'
    // DMP-NEXT:   ACCCopyoutClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'int [1]'
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'int [1]'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'ci' 'int [1]'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'co' 'int [1]'
    // DMP-NOT:      OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc data copy(c) copyin(ci) copyout(co){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(tofrom: c) map(to: ci) map(from: co){{$}}
    //
    // PRT-O-NEXT: {{^ *}}#pragma omp target data map(tofrom: c) map(to: ci) map(from: co){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data copy(c) copyin(ci) copyout(co){{$}}
    #pragma acc data copy(c) copyin(ci) copyout(co)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: c[0] +=
      // PRT: co[0] +=
       c[0] += 100;
      ci[0] += 100;
      co[0] += 100;

      // Check suppression of implicit data clauses.
      //
      // DMP:      ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'int [1]'
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'int [1]'
      // DMP-NEXT:   impl: OMPTargetTeamsDirective
      // DMP-NEXT:     OMPNum_teamsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPSharedClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co' 'int [1]'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'int [1]'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci' 'int [1]'
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1)
      // PRT-AO-SAME: {{^ *}}shared(co,c,ci){{$}}
      //
      // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1)
      // PRT-O-SAME:  {{^ *}}shared(co,c,ci){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1){{$}}
      #pragma acc parallel num_gangs(1)
      // DMP: CompoundStmt
      // PRT-NEXT: {
      {
        // Don't use this uninitialized.
        // PRT-NEXT: co[0] =
        co[0] = 40;
        // PRT-NEXT: printf(
        // PRT:      );
        //      EXE-NEXT: In first acc parallel:
        // EXE-HOST-NEXT:   c[0]= 110, ci[0]= 120, co[0]=  40
        //  EXE-DEV-NEXT:   c[0]=  10, ci[0]=  20, co[0]=  40
        printf("In first acc parallel:\n"
               "  c[0]=%4d, ci[0]=%4d, co[0]=%4d\n",
               c[0], ci[0], co[0]);
        // PRT-NEXT: c[0] +=
        // PRT: co[0] +=
        c[0] += 1000; ci[0] += 1000; co[0] += 1000;
      } // PRT-NEXT: }

      // PRT-NEXT: printf(
      // PRT:      );
      //      EXE-NEXT: After first acc parallel:
      // EXE-HOST-NEXT:   c[0]=1110, ci[0]=1120, co[0]=1040
      //  EXE-DEV-NEXT:   c[0]= 110, ci[0]= 120, co[0]= 130
      printf("After first acc parallel:\n"
             "  c[0]=%4d, ci[0]=%4d, co[0]=%4d\n",
             c[0], ci[0], co[0]);
      // PRT-NEXT: c[0] +=
      // PRT: co[0] +=
      c[0] += 100; ci[0] += 100; co[0] += 100;

      // Check suppression of explicit data clauses (only data transfers are
      // suppressed).
      //
      // DMP:      ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCCopyinClause
      // DMP-NOT:      <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'int [1]'
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'int [1]'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'int [1]'
      // DMP-NEXT:   impl: OMPTargetTeamsDirective
      // DMP-NEXT:     OMPNum_teamsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPMapClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'int [1]'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci' 'int [1]'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co' 'int [1]'
      // DMP-NEXT:     OMPSharedClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'int [1]'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci' 'int [1]'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co' 'int [1]'
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1) copyin(c,ci,co){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1)
      // PRT-AO-SAME: {{^ *}}map(to: c,ci,co) shared(c,ci,co){{$}}
      //
      // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1)
      // PRT-O-SAME:  {{^ *}}map(to: c,ci,co) shared(c,ci,co){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) copyin(c,ci,co){{$}}
      #pragma acc parallel num_gangs(1) copyin(c,ci,co)
      // DMP: CompoundStmt
      // PRT-NEXT: {
      {
        // PRT-NEXT: printf(
        // PRT:      );
        //      EXE-NEXT: In second acc parallel:
        // EXE-HOST-NEXT:   c[0]=1210, ci[0]=1220, co[0]=1140
        //  EXE-DEV-NEXT:   c[0]=1010, ci[0]=1020, co[0]=1040
        printf("In second acc parallel:\n"
               "  c[0]=%4d, ci[0]=%4d, co[0]=%4d\n",
               c[0], ci[0], co[0]);
        // PRT-NEXT: c[0] +=
        // PRT: co[0] +=
         c[0] += 1000; ci[0] += 1000; co[0] += 1000;
      } // PRT-NEXT: }

      // PRT-NEXT: printf(
      // PRT:      );
      //      EXE-NEXT: After second acc parallel:
      // EXE-HOST-NEXT:   c[0]=2210, ci[0]=2220, co[0]=2140
      //  EXE-DEV-NEXT:   c[0]= 210, ci[0]= 220, co[0]= 230
      printf("After second acc parallel:\n"
             "   c[0]=%4d, ci[0]=%4d, co[0]=%4d\n",
             c[0], ci[0], co[0]);
      // PRT-NEXT: c[0] +=
      // PRT: co[0] +=
      c[0] += 100; ci[0] += 100; co[0] += 100;
    } // PRT-NEXT: }

    // PRT-NEXT: printf(
    // PRT:      );
    //      EXE-NEXT: After acc data:
    // EXE-HOST-NEXT:   c[0]=2310, ci[0]=2320, co[0]=2240
    //  EXE-DEV-NEXT:   c[0]=2010, ci[0]= 320, co[0]=2040
    printf("After acc data:\n"
           "  c[0]=%4d, ci[0]=%4d, co[0]=%4d\n",
           c[0], ci[0], co[0]);
  } // PRT-NEXT: }

  //--------------------------------------------------
  // Repeat except...
  //
  // Check struct types.
  // Check suppression of inner copyout clause data transfers.
  //--------------------------------------------------

  // PRT-NEXT: {
  {
    // PRT-NEXT: struct T {
    // PRT: }
    struct T { int i; };
    // PRT-NEXT: c =
    // PRT: co = {30};
    struct T  c = {10};
    struct T ci = {20};
    struct T co = {30};
    // DMP:      ACCDataDirective
    // DMP-NEXT:   ACCCopyClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'struct T'
    // DMP-NEXT:   ACCCopyinClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'struct T'
    // DMP-NEXT:   ACCCopyoutClause
    // DMP-NOT:      <implicit>
    // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'struct T'
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'struct T'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'ci' 'struct T'
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 'co' 'struct T'
    // DMP-NOT:      OMP
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc data copy(c) copyin(ci) copyout(co){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(tofrom: c) map(to: ci) map(from: co){{$}}
    //
    // PRT-O-NEXT: {{^ *}}#pragma omp target data map(tofrom: c) map(to: ci) map(from: co){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data copy(c) copyin(ci) copyout(co){{$}}
    #pragma acc data copy(c) copyin(ci) copyout(co)
    // DMP: CompoundStmt
    // PRT-NEXT: {
    {
      // PRT-NEXT: c.i +=
      // PRT: co.i +=
       c.i += 100;
      ci.i += 100;
      co.i += 100;

      // Check suppression of implicit data clauses.
      //
      // DMP:      ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCNomapClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'struct T'
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'struct T'
      // DMP-NEXT:   impl: OMPTargetTeamsDirective
      // DMP-NEXT:     OMPNum_teamsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPSharedClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co' 'struct T'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'struct T'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci' 'struct T'
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1)
      // PRT-AO-SAME: {{^ *}}shared(co,c,ci){{$}}
      //
      // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1)
      // PRT-O-SAME:  {{^ *}}shared(co,c,ci){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1){{$}}
      #pragma acc parallel num_gangs(1)
      // DMP: CompoundStmt
      // PRT-NEXT: {
      {
        // Don't use this uninitialized.
        // PRT-NEXT: co.i =
        co.i = 40;
        // PRT-NEXT: printf(
        // PRT:      );
        //      EXE-NEXT: In first acc parallel:
        // EXE-HOST-NEXT:   c.i= 110, ci.i= 120, co.i=  40
        //  EXE-DEV-NEXT:   c.i=  10, ci.i=  20, co.i=  40
        printf("In first acc parallel:\n"
               "  c.i=%4d, ci.i=%4d, co.i=%4d\n",
               c.i, ci.i, co.i);
        // PRT-NEXT: c.i +=
        // PRT: co.i +=
        c.i += 1000; ci.i += 1000; co.i += 1000;
      } // PRT-NEXT: }

      // PRT-NEXT: printf(
      // PRT:      );
      //      EXE-NEXT: After first acc parallel:
      // EXE-HOST-NEXT:   c.i=1110, ci.i=1120, co.i=1040
      //  EXE-DEV-NEXT:   c.i= 110, ci.i= 120, co.i= 130
      printf("After first acc parallel:\n"
             "  c.i=%4d, ci.i=%4d, co.i=%4d\n",
             c.i, ci.i, co.i);
      // PRT-NEXT: c.i +=
      // PRT: co.i +=
      c.i += 100; ci.i += 100; co.i += 100;

      // Check suppression of explicit data clauses (only data transfers are
      // suppressed).
      //
      // DMP:      ACCParallelDirective
      // DMP-NEXT:   ACCNumGangsClause
      // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:   ACCCopyoutClause
      // DMP-NOT:      <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'struct T'
      // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
      // DMP-NEXT:     DeclRefExpr {{.*}} 'c' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'ci' 'struct T'
      // DMP-NEXT:     DeclRefExpr {{.*}} 'co' 'struct T'
      // DMP-NEXT:   impl: OMPTargetTeamsDirective
      // DMP-NEXT:     OMPNum_teamsClause
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:     OMPMapClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'struct T'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci' 'struct T'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co' 'struct T'
      // DMP-NEXT:     OMPSharedClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'c' 'struct T'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'ci' 'struct T'
      // DMP-NEXT:       DeclRefExpr {{.*}} 'co' 'struct T'
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1) copyout(c,ci,co){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1)
      // PRT-AO-SAME: {{^ *}}map(from: c,ci,co) shared(c,ci,co){{$}}
      //
      // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1)
      // PRT-O-SAME:  {{^ *}}map(from: c,ci,co) shared(c,ci,co){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) copyout(c,ci,co){{$}}
      #pragma acc parallel num_gangs(1) copyout(c,ci,co)
      // DMP: CompoundStmt
      // PRT-NEXT: {
      {
        // PRT-NEXT: printf(
        // PRT:      );
        //      EXE-NEXT: In second acc parallel:
        // EXE-HOST-NEXT:   c.i=1210, ci.i=1220, co.i=1140
        //  EXE-DEV-NEXT:   c.i=1010, ci.i=1020, co.i=1040
        printf("In second acc parallel:\n"
               "  c.i=%4d, ci.i=%4d, co.i=%4d\n",
               c.i, ci.i, co.i);
        // PRT-NEXT: c.i +=
        // PRT: co.i +=
         c.i += 1000; ci.i += 1000; co.i += 1000;
      } // PRT-NEXT: }

      // PRT-NEXT: printf(
      // PRT:      );
      //      EXE-NEXT: After second acc parallel:
      // EXE-HOST-NEXT:   c.i=2210, ci.i=2220, co.i=2140
      //  EXE-DEV-NEXT:   c.i= 210, ci.i= 220, co.i= 230
      printf("After second acc parallel:\n"
             "   c.i=%4d, ci.i=%4d, co.i=%4d\n",
             c.i, ci.i, co.i);
      // PRT-NEXT: c.i +=
      // PRT: co.i +=
      c.i += 100; ci.i += 100; co.i += 100;
    } // PRT-NEXT: }

    // PRT-NEXT: printf(
    // PRT:      );
    //      EXE-NEXT: After acc data:
    // EXE-HOST-NEXT:   c.i=2310, ci.i=2320, co.i=2240
    //  EXE-DEV-NEXT:   c.i=2010, ci.i= 320, co.i=2040
    printf("After acc data:\n"
           "  c.i=%4d, ci.i=%4d, co.i=%4d\n",
           c.i, ci.i, co.i);
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
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(tofrom: c){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target data map(tofrom: c){{$}}
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
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(to: ci){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target data map(to: ci){{$}}
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
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(from: co){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target data map(from: co){{$}}
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
      // PRT-AO-SAME: {{^ *}}map(tofrom: c,ci,co) shared(c,ci,co){{$}}
      //
      // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1)
      // PRT-O-SAME:  {{^ *}}map(tofrom: c,ci,co) shared(c,ci,co){{$}}
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
  // directives suppresses an implicit data clause on a nested acc parallel.
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
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       OMPArraySectionExpr
    // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:           DeclRefExpr {{.*}} 'arr' 'int [5]'
    // DMP-NEXT:           IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:           IntegerLiteral {{.*}} 'int' 3
    // DMP-NOT:      OMP{{.*}}Clause
    // DMP:          CapturedStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc data copy(arr[1:3]){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(tofrom: arr[1:3]){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target data map(tofrom: arr[1:3]){{$}}
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
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
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
      // DMP-NEXT:           IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:           IntegerLiteral {{.*}} 'int' 3
      // DMP-NEXT:     OMPSharedClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'arr' 'int [5]'
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1) copy(arr[1:3]){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1)
      // PRT-AO-SAME: {{^ *}}map(tofrom: arr[1:3]) shared(arr){{$}}
      //
      // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1)
      // PRT-O-SAME:  {{^ *}}map(tofrom: arr[1:3]) shared(arr){{$}}
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
    // DMP-NOT:      OMP{{.*}}Clause
    // DMP:          CapturedStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc data copy(arr[s0:l0]){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(tofrom: arr[s0:l0]){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target data map(tofrom: arr[s0:l0]){{$}}
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
      // DMP-NEXT:     OMPSharedClause
      // DMP-NOT:        <implicit>
      // DMP-NEXT:       DeclRefExpr {{.*}} 'arr' 'int [5]'
      // DMP-NOT:      OMP
      //
      // PRT-A-NEXT:  {{^ *}}#pragma acc parallel num_gangs(1) copy(arr[s1:l1]){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1)
      // PRT-AO-SAME: {{^ *}}map(tofrom: arr[s1:l1]) shared(arr){{$}}
      //
      // PRT-O-NEXT:  {{^ *}}#pragma omp target teams num_teams(1)
      // PRT-O-SAME:  {{^ *}}map(tofrom: arr[s1:l1]) shared(arr){{$}}
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
  // 3.0 or OpenMP 5.0.  We assume the innermost subarray specifies the
  // mapping, and we check that it is indeed mapped for the acc parallel.  We
  // could manually check that the outermost subarray isn't mapped by writing
  // to it and observing that the new value isn't copied back to the host, but
  // writing to unmapped memory could introduce races, so we don't check that
  // in the test suite.
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
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:         DeclRefExpr {{.*}} 'arr1' 'int [5]'
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       OMPArraySectionExpr
    // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:           DeclRefExpr {{.*}} 'arr0' 'int [5]'
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:       OMPArraySectionExpr
    // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:           DeclRefExpr {{.*}} 'arr1' 'int [5]'
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
    // DMP-NOT:      OMP{{.*}}Clause
    // DMP:          CapturedStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc data copy(arr0[0:2],arr1[2:3]){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(tofrom: arr0[0:2],arr1[2:3]){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target data map(tofrom: arr0[0:2],arr1[2:3]){{$}}
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
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:         DeclRefExpr {{.*}} 'arr1' 'int [5]'
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    // DMP-NOT:        <implicit>
    // DMP-NEXT:       OMPArraySectionExpr
    // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:           DeclRefExpr {{.*}} 'arr0' 'int [5]'
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:       OMPArraySectionExpr
    // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:           DeclRefExpr {{.*}} 'arr1' 'int [5]'
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    // DMP-NOT:      OMP{{.*}}Clause
    // DMP:          CapturedStmt
    //
    // PRT-A-NEXT:  {{^ *}}#pragma acc data copy(arr0[2:3],arr1[0:2]){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(tofrom: arr0[2:3],arr1[0:2]){{$}}
    // PRT-O-NEXT:  {{^ *}}#pragma omp target data map(tofrom: arr0[2:3],arr1[0:2]){{$}}
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
      // PRT-NEXT: arr0[3] +=
      // PRT-NEXT: arr1[0] +=
      arr0[3] += 1000;
      arr1[0] += 1000;
    } // PRT-NEXT: }
    // PRT-NEXT: printf(
    // PRT:      );
    // EXE-NEXT: After acc data:
    // EXE-NEXT:   arr0[3]=1040, arr1[0]=1011
    printf("After acc data:\n"
           "  arr0[3]=%4d, arr1[0]=%4d\n",
           arr0[3], arr1[0]);
  } // PRT-NEXT: }

  // PRT-NEXT: return 0;
  return 0;
} // PRT-NEXT: }
// EXE-NOT: {{.}}
