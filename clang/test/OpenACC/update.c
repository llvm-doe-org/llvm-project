// Check "acc update".
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
// RUN:          -Wno-openacc-omp-map-no-alloc -Wno-openacc-omp-map-present \
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
// RUN:          -Wno-openacc-omp-map-no-alloc -Wno-openacc-omp-map-present \
// RUN:   | FileCheck -check-prefixes=%[prt-chk] %s
// RUN: }

// Can we print the OpenMP source code, compile, and run it successfully?
//
// RUN: %for prt-opts {
// RUN:   %clang -Xclang -verify %[prt-opt]=omp %s > %t-omp.c \
// RUN:          -Wno-openacc-omp-map-no-alloc -Wno-openacc-omp-map-present
// RUN:   echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN:   %clang -Xclang -verify -fopenmp %fopenmp-version -o %t %t-omp.c
// RUN:   %t 2>&1 | FileCheck -check-prefixes=EXE,EXE-TGT-HOST,EXE-HOST %s
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
  printf("    host %s=%d\n", Name, *Var);
}
void printDeviceInt_(const char *Name, int *Var) {
  printf("  device %s=", Name);
  #pragma acc parallel num_gangs(1)
  printf("%d\n", *Var);
}

// DMP-LABEL: FunctionDecl {{.*}} updateNotNested
// PRT-LABEL: void updateNotNested(
void updateNotNested(int *s, int *h, int *d) {
  //      DMP: ACCUpdateDirective
  // DMP-NEXT:   ACCSelfClause
  // DMP-NEXT:     OMPArraySectionExpr
  // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <LValueToRValue>
  // DMP-NEXT:         DeclRefExpr {{.*}} 's' 'int *'
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:       <<<NULL>>>
  // DMP-NEXT:   ACCSelfClause
  // DMP-NEXT:     OMPArraySectionExpr
  // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <LValueToRValue>
  // DMP-NEXT:         DeclRefExpr {{.*}} 'h' 'int *'
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:       <<<NULL>>>
  // DMP-NEXT:   ACCDeviceClause
  // DMP-NEXT:     OMPArraySectionExpr
  // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <LValueToRValue>
  // DMP-NEXT:         DeclRefExpr {{.*}} 'd' 'int *'
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:       <<<NULL>>>
  // DMP-NEXT:   impl: OMPTargetUpdateDirective
  // DMP-NEXT:     OMPFromClause
  // DMP-NEXT:       OMPArraySectionExpr
  // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <LValueToRValue>
  // DMP-NEXT:           DeclRefExpr {{.*}} 's' 'int *'
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:         <<<NULL>>>
  // DMP-NEXT:     OMPFromClause
  // DMP-NEXT:       OMPArraySectionExpr
  // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <LValueToRValue>
  // DMP-NEXT:           DeclRefExpr {{.*}} 'h' 'int *'
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:         <<<NULL>>>
  // DMP-NEXT:     OMPToClause
  // DMP-NEXT:       OMPArraySectionExpr
  // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <LValueToRValue>
  // DMP-NEXT:           DeclRefExpr {{.*}} 'd' 'int *'
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:         <<<NULL>>>
  //  DMP-NOT: OMP
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc update self(s[0:1]) host(h[0:1]) device(d[0:1]){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target update from(s[0:1]) from(h[0:1]) to(d[0:1]){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target update from(s[0:1]) from(h[0:1]) to(d[0:1]){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc update self(s[0:1]) host(h[0:1]) device(d[0:1]){{$}}
  #pragma acc update self(s[0:1]) host(h[0:1]) device(d[0:1])

  // DMP: CallExpr
  printf("updated\n");
}

int main() {
  //--------------------------------------------------
  // Check acc update not statically nested within another construct.
  //
  // Check with scalar type via subarray on pointer.
  //--------------------------------------------------

  {
    int s, h, d;

    // DMP-LABEL: not nested:
    // PRT-LABEL: not nested:
    // EXE-LABEL: not nested:
    printf("not nested:\n");

    #pragma acc data create(s,h,d)
    {
      setHostInt(s, 10);
      setHostInt(h, 20);
      setHostInt(d, 30);
      setDeviceInt(s, 11);
      setDeviceInt(h, 21);
      setDeviceInt(d, 31);

      // EXE-NEXT: updated
      updateNotNested(&s, &h, &d);

      //      EXE-NEXT:   host s=11{{$}}
      //      EXE-NEXT:   host h=21{{$}}
      //  EXE-DEV-NEXT:   host d=30{{$}}
      // EXE-HOST-NEXT:   host d=31{{$}}
      //      EXE-NEXT: device s=11{{$}}
      //      EXE-NEXT: device h=21{{$}}
      //  EXE-DEV-NEXT: device d=30{{$}}
      // EXE-HOST-NEXT: device d=31{{$}}
      printHostInt(s);
      printHostInt(h);
      printHostInt(d);
      printDeviceInt(s);
      printDeviceInt(h);
      printDeviceInt(d);
    }
  }

  //--------------------------------------------------
  // Check acc update nested statically within acc data.
  //
  // Check that dumping and printing integrate correctly with the outer
  // directive.
  //
  // Check with scalar type.
  //--------------------------------------------------

  {
    int s, h, d;

    // DMP-LABEL: "nested:\n"
    // PRT-LABEL: "nested:\n"
    // EXE-LABEL: nested:
    printf("nested:\n");

    //      DMP: ACCDataDirective
    // DMP-NEXT:   ACCCreateClause
    // DMP-NEXT:     DeclRefExpr {{.*}} 's' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'h' 'int'
    // DMP-NEXT:     DeclRefExpr {{.*}} 'd' 'int'
    // DMP-NEXT:   impl: OMPTargetDataDirective
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       DeclRefExpr {{.*}} 's' 'int'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'h' 'int'
    // DMP-NEXT:       DeclRefExpr {{.*}} 'd' 'int'
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc data create(s,h,d){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(alloc: s,h,d){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target data map(alloc: s,h,d){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc data create(s,h,d){{$}}
    #pragma acc data create(s,h,d)
    // PRT-NEXT: {
    {
      // PRT-NEXT: setHostInt
      // PRT-NEXT: setHostInt
      // PRT-NEXT: setHostInt
      // PRT-NEXT: setDeviceInt
      // PRT-NEXT: setDeviceInt
      // PRT-NEXT: setDeviceInt
      setHostInt(s, 10);
      setHostInt(h, 20);
      setHostInt(d, 30);
      setDeviceInt(s, 11);
      setDeviceInt(h, 21);
      setDeviceInt(d, 31);

      //      DMP: ACCUpdateDirective
      // DMP-NEXT:   ACCSelfClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 's' 'int'
      // DMP-NEXT:   ACCSelfClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 'h' 'int'
      // DMP-NEXT:   ACCDeviceClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 'd' 'int'
      // DMP-NEXT:   impl: OMPTargetUpdateDirective
      // DMP-NEXT:     OMPFromClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 's' 'int'
      // DMP-NEXT:     OMPFromClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'h' 'int'
      // DMP-NEXT:     OMPToClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'd' 'int'
      //  DMP-NOT: OMP
      //
      //  PRT-A-NEXT: {{^ *}}#pragma acc update self(s) host(h) device(d){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target update from(s) from(h) to(d){{$}}
      //  PRT-O-NEXT: {{^ *}}#pragma omp target update from(s) from(h) to(d){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc update self(s) host(h) device(d){{$}}
      #pragma acc update self(s) host(h) device(d)

      // PRT-NEXT: printHostInt
      // PRT-NEXT: printHostInt
      // PRT-NEXT: printHostInt
      // PRT-NEXT: printDeviceInt
      // PRT-NEXT: printDeviceInt
      // PRT-NEXT: printDeviceInt
      //      EXE-NEXT:   host s=11{{$}}
      //      EXE-NEXT:   host h=21{{$}}
      //  EXE-DEV-NEXT:   host d=30{{$}}
      // EXE-HOST-NEXT:   host d=31{{$}}
      //      EXE-NEXT: device s=11{{$}}
      //      EXE-NEXT: device h=21{{$}}
      //  EXE-DEV-NEXT: device d=30{{$}}
      // EXE-HOST-NEXT: device d=31{{$}}
      printHostInt(s);
      printHostInt(h);
      printHostInt(d);
      printDeviceInt(s);
      printDeviceInt(h);
      printDeviceInt(d);
    } // PRT-NEXT: }
  }

  //--------------------------------------------------
  // Check struct.
  //--------------------------------------------------

  {
    struct S {
      int i;
      int j;
    } s, h, d;

    // DMP-LABEL: "struct:\n"
    // PRT-LABEL: "struct:\n"
    // EXE-LABEL: struct:
    printf("struct:\n");

    #pragma acc data create(s, h, d)
    {
      setHostInt(s.i, 10);
      setHostInt(s.j, 20);
      setHostInt(h.i, 30);
      setHostInt(h.j, 40);
      setHostInt(d.i, 50);
      setHostInt(d.j, 60);
      setDeviceInt(s.i, 11);
      setDeviceInt(s.j, 21);
      setDeviceInt(h.i, 31);
      setDeviceInt(h.j, 41);
      setDeviceInt(d.i, 51);
      setDeviceInt(d.j, 61);

      //      DMP: ACCUpdateDirective
      // DMP-NEXT:   ACCSelfClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 's' 'struct S'
      // DMP-NEXT:   ACCSelfClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 'h' 'struct S'
      // DMP-NEXT:   ACCDeviceClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 'd' 'struct S'
      // DMP-NEXT:   impl: OMPTargetUpdateDirective
      // DMP-NEXT:     OMPFromClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 's' 'struct S'
      // DMP-NEXT:     OMPFromClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'h' 'struct S'
      // DMP-NEXT:     OMPToClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'd' 'struct S'
      //  DMP-NOT: OMP
      //
      //       PRT-A: {{^ *}}#pragma acc update self(s) host(h) device(d){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target update from(s) from(h) to(d){{$}}
      //       PRT-O: {{^ *}}#pragma omp target update from(s) from(h) to(d){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc update self(s) host(h) device(d){{$}}
      #pragma acc update self(s) host(h) device(d)

      //      EXE-NEXT:   host s.i=11{{$}}
      //      EXE-NEXT:   host s.j=21{{$}}
      //      EXE-NEXT:   host h.i=31{{$}}
      //      EXE-NEXT:   host h.j=41{{$}}
      //  EXE-DEV-NEXT:   host d.i=50{{$}}
      //  EXE-DEV-NEXT:   host d.j=60{{$}}
      // EXE-HOST-NEXT:   host d.i=51{{$}}
      // EXE-HOST-NEXT:   host d.j=61{{$}}
      //      EXE-NEXT: device s.i=11{{$}}
      //      EXE-NEXT: device s.j=21{{$}}
      //      EXE-NEXT: device h.i=31{{$}}
      //      EXE-NEXT: device h.j=41{{$}}
      //  EXE-DEV-NEXT: device d.i=50{{$}}
      //  EXE-DEV-NEXT: device d.j=60{{$}}
      // EXE-HOST-NEXT: device d.i=51{{$}}
      // EXE-HOST-NEXT: device d.j=61{{$}}
      printHostInt(s.i);
      printHostInt(s.j);
      printHostInt(h.i);
      printHostInt(h.j);
      printHostInt(d.i);
      printHostInt(d.j);
      printDeviceInt(s.i);
      printDeviceInt(s.j);
      printDeviceInt(h.i);
      printDeviceInt(h.j);
      printDeviceInt(d.i);
      printDeviceInt(d.j);
    }
  }

  //--------------------------------------------------
  // Check array.
  //--------------------------------------------------

  {
    int s[2];
    int h[2];
    int d[2];

    // DMP-LABEL: "array:\n"
    // PRT-LABEL: "array:\n"
    // EXE-LABEL: array:
    printf("array:\n");

    #pragma acc data create(s, h, d)
    {
      setHostInt(s[0], 10);
      setHostInt(s[1], 20);
      setHostInt(h[0], 30);
      setHostInt(h[1], 40);
      setHostInt(d[0], 50);
      setHostInt(d[1], 60);
      setDeviceInt(s[0], 11);
      setDeviceInt(s[1], 21);
      setDeviceInt(h[0], 31);
      setDeviceInt(h[1], 41);
      setDeviceInt(d[0], 51);
      setDeviceInt(d[1], 61);

      //      DMP: ACCUpdateDirective
      // DMP-NEXT:   ACCSelfClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 's' 'int [2]'
      // DMP-NEXT:   ACCSelfClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 'h' 'int [2]'
      // DMP-NEXT:   ACCDeviceClause
      // DMP-NEXT:     DeclRefExpr {{.*}} 'd' 'int [2]'
      // DMP-NEXT:   impl: OMPTargetUpdateDirective
      // DMP-NEXT:     OMPFromClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 's' 'int [2]'
      // DMP-NEXT:     OMPFromClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'h' 'int [2]'
      // DMP-NEXT:     OMPToClause
      // DMP-NEXT:       DeclRefExpr {{.*}} 'd' 'int [2]'
      //  DMP-NOT: OMP
      //
      //       PRT-A: {{^ *}}#pragma acc update self(s) host(h) device(d){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target update from(s) from(h) to(d){{$}}
      //       PRT-O: {{^ *}}#pragma omp target update from(s) from(h) to(d){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc update self(s) host(h) device(d){{$}}
      #pragma acc update self(s) host(h) device(d)

      //      EXE-NEXT:   host s[0]=11{{$}}
      //      EXE-NEXT:   host s[1]=21{{$}}
      //      EXE-NEXT:   host h[0]=31{{$}}
      //      EXE-NEXT:   host h[1]=41{{$}}
      //  EXE-DEV-NEXT:   host d[0]=50{{$}}
      //  EXE-DEV-NEXT:   host d[1]=60{{$}}
      // EXE-HOST-NEXT:   host d[0]=51{{$}}
      // EXE-HOST-NEXT:   host d[1]=61{{$}}
      //      EXE-NEXT: device s[0]=11{{$}}
      //      EXE-NEXT: device s[1]=21{{$}}
      //      EXE-NEXT: device h[0]=31{{$}}
      //      EXE-NEXT: device h[1]=41{{$}}
      //  EXE-DEV-NEXT: device d[0]=50{{$}}
      //  EXE-DEV-NEXT: device d[1]=60{{$}}
      // EXE-HOST-NEXT: device d[0]=51{{$}}
      // EXE-HOST-NEXT: device d[1]=61{{$}}
      printHostInt(s[0]);
      printHostInt(s[1]);
      printHostInt(h[0]);
      printHostInt(h[1]);
      printHostInt(d[0]);
      printHostInt(d[1]);
      printDeviceInt(s[0]);
      printDeviceInt(s[1]);
      printDeviceInt(h[0]);
      printDeviceInt(h[1]);
      printDeviceInt(d[0]);
      printDeviceInt(d[1]);
    }
  }

  //--------------------------------------------------
  // Check subarray not encompassing full array.
  //
  // Subarrays are checked thoroughly for acc data and acc parallel in data.c
  // parallel-subarray.c.  Check just a few cases here to be sure it basically
  // works for acc update too.
  //--------------------------------------------------

  {
    int s[4];
    int h[4];
    int d[4];

    // DMP-LABEL: "subarray with constant expressions:\n"
    // PRT-LABEL: "subarray with constant expressions:\n"
    // EXE-LABEL: subarray with constant expressions:
    printf("subarray with constant expressions:\n");

    #pragma acc data create(s, h, d)
    {
      setHostInt(s[0], 10);
      setHostInt(s[1], 20);
      setHostInt(s[2], 30);
      setHostInt(s[3], 40);
      setHostInt(h[0], 50);
      setHostInt(h[1], 60);
      setHostInt(h[2], 70);
      setHostInt(h[3], 80);
      setHostInt(d[0], 90);
      setHostInt(d[1], 100);
      setHostInt(d[2], 110);
      setHostInt(d[3], 120);
      setDeviceInt(s[0], 11);
      setDeviceInt(s[1], 21);
      setDeviceInt(s[2], 31);
      setDeviceInt(s[3], 41);
      setDeviceInt(h[0], 51);
      setDeviceInt(h[1], 61);
      setDeviceInt(h[2], 71);
      setDeviceInt(h[3], 81);
      setDeviceInt(d[0], 91);
      setDeviceInt(d[1], 101);
      setDeviceInt(d[2], 111);
      setDeviceInt(d[3], 121);

      //      DMP: ACCUpdateDirective
      // DMP-NEXT:   ACCSelfClause
      // DMP-NEXT:     OMPArraySectionExpr
      // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      // DMP-NEXT:         DeclRefExpr {{.*}} 's' 'int [4]'
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
      // DMP-NEXT:       <<<NULL>>>
      // DMP-NEXT:   ACCSelfClause
      // DMP-NEXT:     OMPArraySectionExpr
      // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      // DMP-NEXT:         DeclRefExpr {{.*}} 'h' 'int [4]'
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
      // DMP-NEXT:       <<<NULL>>>
      // DMP-NEXT:   ACCDeviceClause
      // DMP-NEXT:     OMPArraySectionExpr
      // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      // DMP-NEXT:         DeclRefExpr {{.*}} 'd' 'int [4]'
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
      // DMP-NEXT:       <<<NULL>>>
      // DMP-NEXT:   impl: OMPTargetUpdateDirective
      // DMP-NEXT:     OMPFromClause
      // DMP-NEXT:       OMPArraySectionExpr
      // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      // DMP-NEXT:           DeclRefExpr {{.*}} 's' 'int [4]'
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
      // DMP-NEXT:         <<<NULL>>>
      // DMP-NEXT:     OMPFromClause
      // DMP-NEXT:       OMPArraySectionExpr
      // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      // DMP-NEXT:           DeclRefExpr {{.*}} 'h' 'int [4]'
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
      // DMP-NEXT:         <<<NULL>>>
      // DMP-NEXT:     OMPToClause
      // DMP-NEXT:       OMPArraySectionExpr
      // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      // DMP-NEXT:           DeclRefExpr {{.*}} 'd' 'int [4]'
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
      // DMP-NEXT:         <<<NULL>>>
      //  DMP-NOT: OMP
      //
      //       PRT-A: {{^ *}}#pragma acc update self(s[1:2]) host(h[1:2]) device(d[1:2]){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target update from(s[1:2]) from(h[1:2]) to(d[1:2]){{$}}
      //       PRT-O: {{^ *}}#pragma omp target update from(s[1:2]) from(h[1:2]) to(d[1:2]){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc update self(s[1:2]) host(h[1:2]) device(d[1:2]){{$}}
      #pragma acc update self(s[1:2]) host(h[1:2]) device(d[1:2])

      //  EXE-DEV-NEXT:   host s[0]=10{{$}}
      //  EXE-DEV-NEXT:   host s[1]=21{{$}}
      //  EXE-DEV-NEXT:   host s[2]=31{{$}}
      //  EXE-DEV-NEXT:   host s[3]=40{{$}}
      //  EXE-DEV-NEXT:   host h[0]=50{{$}}
      //  EXE-DEV-NEXT:   host h[1]=61{{$}}
      //  EXE-DEV-NEXT:   host h[2]=71{{$}}
      //  EXE-DEV-NEXT:   host h[3]=80{{$}}
      //  EXE-DEV-NEXT:   host d[0]=90{{$}}
      //  EXE-DEV-NEXT:   host d[1]=100{{$}}
      //  EXE-DEV-NEXT:   host d[2]=110{{$}}
      //  EXE-DEV-NEXT:   host d[3]=120{{$}}
      // EXE-HOST-NEXT:   host s[0]=11{{$}}
      // EXE-HOST-NEXT:   host s[1]=21{{$}}
      // EXE-HOST-NEXT:   host s[2]=31{{$}}
      // EXE-HOST-NEXT:   host s[3]=41{{$}}
      // EXE-HOST-NEXT:   host h[0]=51{{$}}
      // EXE-HOST-NEXT:   host h[1]=61{{$}}
      // EXE-HOST-NEXT:   host h[2]=71{{$}}
      // EXE-HOST-NEXT:   host h[3]=81{{$}}
      // EXE-HOST-NEXT:   host d[0]=91{{$}}
      // EXE-HOST-NEXT:   host d[1]=101{{$}}
      // EXE-HOST-NEXT:   host d[2]=111{{$}}
      // EXE-HOST-NEXT:   host d[3]=121{{$}}
      //  EXE-DEV-NEXT: device s[0]=11{{$}}
      //  EXE-DEV-NEXT: device s[1]=21{{$}}
      //  EXE-DEV-NEXT: device s[2]=31{{$}}
      //  EXE-DEV-NEXT: device s[3]=41{{$}}
      //  EXE-DEV-NEXT: device h[0]=51{{$}}
      //  EXE-DEV-NEXT: device h[1]=61{{$}}
      //  EXE-DEV-NEXT: device h[2]=71{{$}}
      //  EXE-DEV-NEXT: device h[3]=81{{$}}
      //  EXE-DEV-NEXT: device d[0]=91{{$}}
      //  EXE-DEV-NEXT: device d[1]=100{{$}}
      //  EXE-DEV-NEXT: device d[2]=110{{$}}
      //  EXE-DEV-NEXT: device d[3]=121{{$}}
      // EXE-HOST-NEXT: device s[0]=11{{$}}
      // EXE-HOST-NEXT: device s[1]=21{{$}}
      // EXE-HOST-NEXT: device s[2]=31{{$}}
      // EXE-HOST-NEXT: device s[3]=41{{$}}
      // EXE-HOST-NEXT: device h[0]=51{{$}}
      // EXE-HOST-NEXT: device h[1]=61{{$}}
      // EXE-HOST-NEXT: device h[2]=71{{$}}
      // EXE-HOST-NEXT: device h[3]=81{{$}}
      // EXE-HOST-NEXT: device d[0]=91{{$}}
      // EXE-HOST-NEXT: device d[1]=101{{$}}
      // EXE-HOST-NEXT: device d[2]=111{{$}}
      // EXE-HOST-NEXT: device d[3]=121{{$}}
      printHostInt(s[0]);
      printHostInt(s[1]);
      printHostInt(s[2]);
      printHostInt(s[3]);
      printHostInt(h[0]);
      printHostInt(h[1]);
      printHostInt(h[2]);
      printHostInt(h[3]);
      printHostInt(d[0]);
      printHostInt(d[1]);
      printHostInt(d[2]);
      printHostInt(d[3]);
      printDeviceInt(s[0]);
      printDeviceInt(s[1]);
      printDeviceInt(s[2]);
      printDeviceInt(s[3]);
      printDeviceInt(h[0]);
      printDeviceInt(h[1]);
      printDeviceInt(h[2]);
      printDeviceInt(h[3]);
      printDeviceInt(d[0]);
      printDeviceInt(d[1]);
      printDeviceInt(d[2]);
      printDeviceInt(d[3]);
    }
  }

  {
    int s[4];
    int h[4];
    int d[4];

    // DMP-LABEL: "subarray with non-constant expressions:\n"
    // PRT-LABEL: "subarray with non-constant expressions:\n"
    // EXE-LABEL: subarray with non-constant expressions:
    printf("subarray with non-constant expressions:\n");

    int start = 1, length = 2;

    #pragma acc data create(s, h, d)
    {
      setHostInt(s[0], 10);
      setHostInt(s[1], 20);
      setHostInt(s[2], 30);
      setHostInt(s[3], 40);
      setHostInt(h[0], 50);
      setHostInt(h[1], 60);
      setHostInt(h[2], 70);
      setHostInt(h[3], 80);
      setHostInt(d[0], 90);
      setHostInt(d[1], 100);
      setHostInt(d[2], 110);
      setHostInt(d[3], 120);
      setDeviceInt(s[0], 11);
      setDeviceInt(s[1], 21);
      setDeviceInt(s[2], 31);
      setDeviceInt(s[3], 41);
      setDeviceInt(h[0], 51);
      setDeviceInt(h[1], 61);
      setDeviceInt(h[2], 71);
      setDeviceInt(h[3], 81);
      setDeviceInt(d[0], 91);
      setDeviceInt(d[1], 101);
      setDeviceInt(d[2], 111);
      setDeviceInt(d[3], 121);

      //      DMP: ACCUpdateDirective
      // DMP-NEXT:   ACCSelfClause
      // DMP-NEXT:     OMPArraySectionExpr
      // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      // DMP-NEXT:         DeclRefExpr {{.*}} 's' 'int [4]'
      // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      // DMP-NEXT:       `-DeclRefExpr {{.*}} 'start' 'int'
      // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      // DMP-NEXT:       `-DeclRefExpr {{.*}} 'length' 'int'
      // DMP-NEXT:       <<<NULL>>>
      // DMP-NEXT:   ACCSelfClause
      // DMP-NEXT:     OMPArraySectionExpr
      // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      // DMP-NEXT:         DeclRefExpr {{.*}} 'h' 'int [4]'
      // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      // DMP-NEXT:       `-DeclRefExpr {{.*}} 'start' 'int'
      // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      // DMP-NEXT:       `-DeclRefExpr {{.*}} 'length' 'int'
      // DMP-NEXT:       <<<NULL>>>
      // DMP-NEXT:   ACCDeviceClause
      // DMP-NEXT:     OMPArraySectionExpr
      // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      // DMP-NEXT:         DeclRefExpr {{.*}} 'd' 'int [4]'
      // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      // DMP-NEXT:       `-DeclRefExpr {{.*}} 'start' 'int'
      // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      // DMP-NEXT:       `-DeclRefExpr {{.*}} 'length' 'int'
      // DMP-NEXT:       <<<NULL>>>
      // DMP-NEXT:   impl: OMPTargetUpdateDirective
      // DMP-NEXT:     OMPFromClause
      // DMP-NEXT:       OMPArraySectionExpr
      // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      // DMP-NEXT:           DeclRefExpr {{.*}} 's' 'int [4]'
      // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      // DMP-NEXT:         `-DeclRefExpr {{.*}} 'start' 'int'
      // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      // DMP-NEXT:         `-DeclRefExpr {{.*}} 'length' 'int'
      // DMP-NEXT:         <<<NULL>>>
      // DMP-NEXT:     OMPFromClause
      // DMP-NEXT:       OMPArraySectionExpr
      // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      // DMP-NEXT:           DeclRefExpr {{.*}} 'h' 'int [4]'
      // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      // DMP-NEXT:         `-DeclRefExpr {{.*}} 'start' 'int'
      // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      // DMP-NEXT:         `-DeclRefExpr {{.*}} 'length' 'int'
      // DMP-NEXT:         <<<NULL>>>
      // DMP-NEXT:     OMPToClause
      // DMP-NEXT:       OMPArraySectionExpr
      // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      // DMP-NEXT:           DeclRefExpr {{.*}} 'd' 'int [4]'
      // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      // DMP-NEXT:         `-DeclRefExpr {{.*}} 'start' 'int'
      // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
      // DMP-NEXT:         `-DeclRefExpr {{.*}} 'length' 'int'
      // DMP-NEXT:         <<<NULL>>>
      //  DMP-NOT: OMP
      //
      //       PRT-A: {{^ *}}#pragma acc update self(s[start:length]) host(h[start:length]) device(d[start:length]){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target update from(s[start:length]) from(h[start:length]) to(d[start:length]){{$}}
      //       PRT-O: {{^ *}}#pragma omp target update from(s[start:length]) from(h[start:length]) to(d[start:length]){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc update self(s[start:length]) host(h[start:length]) device(d[start:length]){{$}}
      #pragma acc update self(s[start:length]) host(h[start:length]) device(d[start:length])

      //  EXE-DEV-NEXT:   host s[0]=10{{$}}
      //  EXE-DEV-NEXT:   host s[1]=21{{$}}
      //  EXE-DEV-NEXT:   host s[2]=31{{$}}
      //  EXE-DEV-NEXT:   host s[3]=40{{$}}
      //  EXE-DEV-NEXT:   host h[0]=50{{$}}
      //  EXE-DEV-NEXT:   host h[1]=61{{$}}
      //  EXE-DEV-NEXT:   host h[2]=71{{$}}
      //  EXE-DEV-NEXT:   host h[3]=80{{$}}
      //  EXE-DEV-NEXT:   host d[0]=90{{$}}
      //  EXE-DEV-NEXT:   host d[1]=100{{$}}
      //  EXE-DEV-NEXT:   host d[2]=110{{$}}
      //  EXE-DEV-NEXT:   host d[3]=120{{$}}
      // EXE-HOST-NEXT:   host s[0]=11{{$}}
      // EXE-HOST-NEXT:   host s[1]=21{{$}}
      // EXE-HOST-NEXT:   host s[2]=31{{$}}
      // EXE-HOST-NEXT:   host s[3]=41{{$}}
      // EXE-HOST-NEXT:   host h[0]=51{{$}}
      // EXE-HOST-NEXT:   host h[1]=61{{$}}
      // EXE-HOST-NEXT:   host h[2]=71{{$}}
      // EXE-HOST-NEXT:   host h[3]=81{{$}}
      // EXE-HOST-NEXT:   host d[0]=91{{$}}
      // EXE-HOST-NEXT:   host d[1]=101{{$}}
      // EXE-HOST-NEXT:   host d[2]=111{{$}}
      // EXE-HOST-NEXT:   host d[3]=121{{$}}
      //  EXE-DEV-NEXT: device s[0]=11{{$}}
      //  EXE-DEV-NEXT: device s[1]=21{{$}}
      //  EXE-DEV-NEXT: device s[2]=31{{$}}
      //  EXE-DEV-NEXT: device s[3]=41{{$}}
      //  EXE-DEV-NEXT: device h[0]=51{{$}}
      //  EXE-DEV-NEXT: device h[1]=61{{$}}
      //  EXE-DEV-NEXT: device h[2]=71{{$}}
      //  EXE-DEV-NEXT: device h[3]=81{{$}}
      //  EXE-DEV-NEXT: device d[0]=91{{$}}
      //  EXE-DEV-NEXT: device d[1]=100{{$}}
      //  EXE-DEV-NEXT: device d[2]=110{{$}}
      //  EXE-DEV-NEXT: device d[3]=121{{$}}
      // EXE-HOST-NEXT: device s[0]=11{{$}}
      // EXE-HOST-NEXT: device s[1]=21{{$}}
      // EXE-HOST-NEXT: device s[2]=31{{$}}
      // EXE-HOST-NEXT: device s[3]=41{{$}}
      // EXE-HOST-NEXT: device h[0]=51{{$}}
      // EXE-HOST-NEXT: device h[1]=61{{$}}
      // EXE-HOST-NEXT: device h[2]=71{{$}}
      // EXE-HOST-NEXT: device h[3]=81{{$}}
      // EXE-HOST-NEXT: device d[0]=91{{$}}
      // EXE-HOST-NEXT: device d[1]=101{{$}}
      // EXE-HOST-NEXT: device d[2]=111{{$}}
      // EXE-HOST-NEXT: device d[3]=121{{$}}
      printHostInt(s[0]);
      printHostInt(s[1]);
      printHostInt(s[2]);
      printHostInt(s[3]);
      printHostInt(h[0]);
      printHostInt(h[1]);
      printHostInt(h[2]);
      printHostInt(h[3]);
      printHostInt(d[0]);
      printHostInt(d[1]);
      printHostInt(d[2]);
      printHostInt(d[3]);
      printDeviceInt(s[0]);
      printDeviceInt(s[1]);
      printDeviceInt(s[2]);
      printDeviceInt(s[3]);
      printDeviceInt(h[0]);
      printDeviceInt(h[1]);
      printDeviceInt(h[2]);
      printDeviceInt(h[3]);
      printDeviceInt(d[0]);
      printDeviceInt(d[1]);
      printDeviceInt(d[2]);
      printDeviceInt(d[3]);
    }
  }

  //--------------------------------------------------
  // Check when data is not present.
  //
  // For now, no data transfer occurs, so just check that execution doesn't
  // fail and host values don't change.  In the future, this behavior might
  // change (see the Clang OpenACC status document).
  //--------------------------------------------------

  // Scalar.
  {
    int s, h, d;

    // DMP-LABEL: scalar not present:
    // PRT-LABEL: scalar not present:
    // EXE-LABEL: scalar not present:
    printf("scalar not present:\n");

    setHostInt(s, 10);
    setHostInt(h, 20);
    setHostInt(d, 30);

    #pragma acc update self(s) host(h) device(d)

    // EXE-NEXT: host s=10{{$}}
    // EXE-NEXT: host h=20{{$}}
    // EXE-NEXT: host d=30{{$}}
    printHostInt(s);
    printHostInt(h);
    printHostInt(d);
  }

  // Partial subarray.
  {
    int s[4];
    int h[4];
    int d[4];

    // DMP-LABEL: "subarray partially not present:\n"
    // PRT-LABEL: "subarray partially not present:\n"
    // EXE-LABEL: subarray partially not present:
    printf("subarray partially not present:\n");

    #pragma acc data create(s[1:2], h[1:2], d[1:2])
    {
      setHostInt(s[0], 10);
      setHostInt(s[1], 20);
      setHostInt(s[2], 30);
      setHostInt(s[3], 40);
      setHostInt(h[0], 50);
      setHostInt(h[1], 60);
      setHostInt(h[2], 70);
      setHostInt(h[3], 80);
      setHostInt(d[0], 90);
      setHostInt(d[1], 100);
      setHostInt(d[2], 110);
      setHostInt(d[3], 120);
      setDeviceInt(s[1], 21);
      setDeviceInt(s[2], 31);
      setDeviceInt(h[1], 61);
      setDeviceInt(h[2], 71);
      setDeviceInt(d[1], 101);
      setDeviceInt(d[2], 111);

      //      DMP: ACCUpdateDirective
      // DMP-NEXT:   ACCSelfClause
      // DMP-NEXT:     OMPArraySectionExpr
      // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      // DMP-NEXT:         DeclRefExpr {{.*}} 's' 'int [4]'
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
      // DMP-NEXT:       <<<NULL>>>
      // DMP-NEXT:   ACCSelfClause
      // DMP-NEXT:     OMPArraySectionExpr
      // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      // DMP-NEXT:         DeclRefExpr {{.*}} 'h' 'int [4]'
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
      // DMP-NEXT:       <<<NULL>>>
      // DMP-NEXT:   ACCDeviceClause
      // DMP-NEXT:     OMPArraySectionExpr
      // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      // DMP-NEXT:         DeclRefExpr {{.*}} 'd' 'int [4]'
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
      // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 3
      // DMP-NEXT:       <<<NULL>>>
      // DMP-NEXT:   impl: OMPTargetUpdateDirective
      // DMP-NEXT:     OMPFromClause
      // DMP-NEXT:       OMPArraySectionExpr
      // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      // DMP-NEXT:           DeclRefExpr {{.*}} 's' 'int [4]'
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
      // DMP-NEXT:         <<<NULL>>>
      // DMP-NEXT:     OMPFromClause
      // DMP-NEXT:       OMPArraySectionExpr
      // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      // DMP-NEXT:           DeclRefExpr {{.*}} 'h' 'int [4]'
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
      // DMP-NEXT:         <<<NULL>>>
      // DMP-NEXT:     OMPToClause
      // DMP-NEXT:       OMPArraySectionExpr
      // DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
      // DMP-NEXT:           DeclRefExpr {{.*}} 'd' 'int [4]'
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
      // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 3
      // DMP-NEXT:         <<<NULL>>>
      //  DMP-NOT: OMP
      //
      //       PRT-A: {{^ *}}#pragma acc update self(s[0:3]) host(h[0:3]) device(d[0:3]){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target update from(s[0:3]) from(h[0:3]) to(d[0:3]){{$}}
      //       PRT-O: {{^ *}}#pragma omp target update from(s[0:3]) from(h[0:3]) to(d[0:3]){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc update self(s[0:3]) host(h[0:3]) device(d[0:3]){{$}}
      #pragma acc update self(s[0:3]) host(h[0:3]) device(d[0:3])

      //  EXE-DEV-NEXT:   host s[0]=10{{$}}
      //  EXE-DEV-NEXT:   host s[1]=20{{$}}
      //  EXE-DEV-NEXT:   host s[2]=30{{$}}
      //  EXE-DEV-NEXT:   host s[3]=40{{$}}
      //  EXE-DEV-NEXT:   host h[0]=50{{$}}
      //  EXE-DEV-NEXT:   host h[1]=60{{$}}
      //  EXE-DEV-NEXT:   host h[2]=70{{$}}
      //  EXE-DEV-NEXT:   host h[3]=80{{$}}
      //  EXE-DEV-NEXT:   host d[0]=90{{$}}
      //  EXE-DEV-NEXT:   host d[1]=100{{$}}
      //  EXE-DEV-NEXT:   host d[2]=110{{$}}
      //  EXE-DEV-NEXT:   host d[3]=120{{$}}
      // EXE-HOST-NEXT:   host s[0]=10{{$}}
      // EXE-HOST-NEXT:   host s[1]=21{{$}}
      // EXE-HOST-NEXT:   host s[2]=31{{$}}
      // EXE-HOST-NEXT:   host s[3]=40{{$}}
      // EXE-HOST-NEXT:   host h[0]=50{{$}}
      // EXE-HOST-NEXT:   host h[1]=61{{$}}
      // EXE-HOST-NEXT:   host h[2]=71{{$}}
      // EXE-HOST-NEXT:   host h[3]=80{{$}}
      // EXE-HOST-NEXT:   host d[0]=90{{$}}
      // EXE-HOST-NEXT:   host d[1]=101{{$}}
      // EXE-HOST-NEXT:   host d[2]=111{{$}}
      // EXE-HOST-NEXT:   host d[3]=120{{$}}
      //  EXE-DEV-NEXT: device s[1]=21{{$}}
      //  EXE-DEV-NEXT: device s[2]=31{{$}}
      //  EXE-DEV-NEXT: device h[1]=61{{$}}
      //  EXE-DEV-NEXT: device h[2]=71{{$}}
      //  EXE-DEV-NEXT: device d[1]=101{{$}}
      //  EXE-DEV-NEXT: device d[2]=111{{$}}
      // EXE-HOST-NEXT: device s[1]=21{{$}}
      // EXE-HOST-NEXT: device s[2]=31{{$}}
      // EXE-HOST-NEXT: device h[1]=61{{$}}
      // EXE-HOST-NEXT: device h[2]=71{{$}}
      // EXE-HOST-NEXT: device d[1]=101{{$}}
      // EXE-HOST-NEXT: device d[2]=111{{$}}
      printHostInt(s[0]);
      printHostInt(s[1]);
      printHostInt(s[2]);
      printHostInt(s[3]);
      printHostInt(h[0]);
      printHostInt(h[1]);
      printHostInt(h[2]);
      printHostInt(h[3]);
      printHostInt(d[0]);
      printHostInt(d[1]);
      printHostInt(d[2]);
      printHostInt(d[3]);
      printDeviceInt(s[1]);
      printDeviceInt(s[2]);
      printDeviceInt(h[1]);
      printDeviceInt(h[2]);
      printDeviceInt(d[1]);
      printDeviceInt(d[2]);
    }
  }

  // PRT-LABEL: return 0;
  return 0;
}

// EXE-NOT: {{.}}
