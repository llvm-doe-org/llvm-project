// Check the "acc update" "if" clause.
//
// FIXME: There are several unfortunate cases where warnings about the "if"
// condition are reported twice, once while parsing OpenACC, and once while
// transforming it to OpenMP.

// RUN: %data tgts {
// RUN:   (run-if=                tgt-cflags=                                     host-or-dev=HOST)
// RUN:   (run-if=%run-if-x86_64  tgt-cflags=-fopenmp-targets=%run-x86_64-triple  host-or-dev=DEV )
// RUN:   (run-if=%run-if-ppc64le tgt-cflags=-fopenmp-targets=%run-ppc64le-triple host-or-dev=DEV )
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags=-fopenmp-targets=%run-nvptx64-triple host-or-dev=DEV )
// RUN: }

// Check -ast-dump before and after AST serialization.
//
// RUN: %clang -Xclang -verify=expected,acc -Xclang -ast-dump -fsyntax-only \
// RUN:        -fopenacc %s \
// RUN: | FileCheck -check-prefixes=DMP %s
// RUN: %clang -Xclang -verify=expected,acc -fopenacc -emit-ast -o %t.ast %s
// RUN: %clang_cc1 -ast-dump-all %t.ast \
// RUN: | FileCheck -check-prefixes=DMP %s

// Check -ast-print and -fopenacc[-ast]-print.
//
// We include print checking on only a few representative cases, which should be
// more than sufficient to show it's working for the "if" clause.
//
// TODO: If lit were to support %for inside a %data, we could iterate prt-opts
// within prt-args after the first prt-args iteration, significantly shortening
// the prt-args definition.
//
// Strip comments and blank lines so checking -fopenacc-print output is easier.
// RUN: grep -v '^ *\(//.*\)\?$' %s | sed 's,//.*,,' > %t-acc.c
//
// RUN: %data prt-opts {
// RUN:   (prt-opt=-fopenacc-ast-print)
// RUN:   (prt-opt=-fopenacc-print    )
// RUN: }
// RUN: %data prt-args {
// RUN:   (prt='-Xclang -ast-print -fsyntax-only -fopenacc' prt-chk=PRT-A)
// RUN:   (prt=-fopenacc-ast-print=acc                      prt-chk=PRT-A)
// RUN:   (prt=-fopenacc-ast-print=omp                      prt-chk=PRT-O)
// RUN:   (prt=-fopenacc-ast-print=acc-omp                  prt-chk=PRT-A,PRT-AO)
// RUN:   (prt=-fopenacc-ast-print=omp-acc                  prt-chk=PRT-O,PRT-OA)
// RUN:   (prt=-fopenacc-print=acc                          prt-chk=PRT-A)
// RUN:   (prt=-fopenacc-print=omp                          prt-chk=PRT-O)
// RUN:   (prt=-fopenacc-print=acc-omp                      prt-chk=PRT-A,PRT-AO)
// RUN:   (prt=-fopenacc-print=omp-acc                      prt-chk=PRT-O,PRT-OA)
// RUN: }
// RUN: %for prt-args {
// RUN:   %clang -Xclang -verify=expected,acc %[prt] %t-acc.c \
// RUN:          -Wno-openacc-omp-update-present -Wno-openacc-omp-map-hold \
// RUN:   | FileCheck -check-prefixes=%[prt-chk] %s
// RUN: }

// Check -ast-print after AST serialization.
//
// Some data related to printing (where to print comments about discarded
// directives) is serialized and deserialized, so it's worthwhile to try all
// OpenACC printing modes.
//
// RUN: %clang -Xclang -verify=expected,acc -fopenacc -emit-ast %t-acc.c \
// RUN:        -o %t.ast
// RUN: %for prt-args {
// RUN:   %clang %[prt] %t.ast 2>&1 \
// RUN:   | FileCheck -check-prefixes=%[prt-chk] %s
// RUN: }

// Can we print the OpenMP source code, compile, and run it successfully?
//
// We don't normally bother to check this for offloading, but the update
// directive has no effect when not offloading (that is, for shared memory), and
// one of the main issues with the update directive is the various ways it can
// be translated so it can be used in source-to-source when targeting other
// compilers.  That is, we want to be sure source-to-source mode produces
// working translations of the update directive in all cases.
//
// RUN: %for tgts {
// RUN:   %for prt-opts {
// RUN:     %[run-if] %clang -Xclang -verify=expected,acc %[prt-opt]=omp %s \
// RUN:                      -Wno-openacc-omp-update-present \
// RUN:                      -Wno-openacc-omp-map-hold \
// RUN:                      > %t-omp.c
// RUN:     %[run-if] echo "// none""-no-diagnostics" >> %t-omp.c
// RUN:     %[run-if] %clang -Xclang -verify=none -fopenmp %fopenmp-version \
// RUN:                      %[tgt-cflags] -o %t.exe %t-omp.c \
// RUN:                      -Wno-literal-conversion \
// RUN:                      -Wno-pointer-bool-conversion
// RUN:     %[run-if] %t.exe > %t.out 2>&1
// RUN:     %[run-if] FileCheck -input-file %t.out %s \
// RUN:       -strict-whitespace -check-prefixes=EXE,EXE-%[host-or-dev]
// RUN:   }
// RUN: }

// Check execution with normal compilation.
//
// RUN: %for tgts {
// RUN:   %[run-if] %clang -Xclang -verify=expected,acc -fopenacc \
// RUN:             %[tgt-cflags] -o %t.exe %s
// RUN:   %[run-if] %t.exe > %t.out 2>&1
// RUN:   %[run-if] FileCheck -input-file %t.out %s\
// RUN:     -strict-whitespace -check-prefixes=EXE,EXE-%[host-or-dev]
// RUN: }

// END.

#include <stdio.h>

#define setValues() setValues_(&s, &h, &d)

void setValues_(int *sp, int *hp, int *dp) {
  *sp = 10;
  *hp = 20;
  *dp = 30;
  #pragma acc parallel num_gangs(1)
  {
    *sp = 11;
    *hp = 21;
    *dp = 31;
  }
}

#define printValues() printValues_(&s, &h, &d)

void printValues_(int *sp, int *hp, int *dp) {
  printf("    host s=%d, h=%d, d=%d\n", *sp, *hp, *dp);
  #pragma acc parallel num_gangs(1)
  printf("  device s=%d, h=%d, d=%d\n", *sp, *hp, *dp);
}

// EXE-NOT: {{.}}

int main(int argc, char *argv[]) {
  // EXE: start
  printf("start\n");

  int s, h, d;
  #pragma acc enter data create(s,h,d)

  //--------------------------------------------------
  // Check if(1).
  //
  // Check printing and dumping for constant condition.
  //--------------------------------------------------

  //      DMP: ACCUpdateDirective
  // DMP-NEXT:   ACCSelfClause
  // DMP-NEXT:     DeclRefExpr {{.*}} 's' 'int'
  // DMP-NEXT:   ACCSelfClause
  // DMP-NEXT:     DeclRefExpr {{.*}} 'h' 'int'
  // DMP-NEXT:   ACCDeviceClause
  // DMP-NEXT:     DeclRefExpr {{.*}} 'd' 'int'
  // DMP-NEXT:   ACCIfClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 'int' 1
  // DMP-NEXT:   impl: OMPTargetUpdateDirective
  // DMP-NEXT:     OMPFromClause
  // DMP-NEXT:       DeclRefExpr {{.*}} 's' 'int'
  // DMP-NEXT:     OMPFromClause
  // DMP-NEXT:       DeclRefExpr {{.*}} 'h' 'int'
  // DMP-NEXT:     OMPToClause
  // DMP-NEXT:       DeclRefExpr {{.*}} 'd' 'int'
  // DMP-NEXT:     OMPIfClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  //  DMP-NOT: -{{ACC|OMP}}
  //
  //       PRT-A: {{^ *}}#pragma acc update self(s) host(h) device(d) if(1){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target update from(present: s) from(present: h) to(present: d) if(1){{$}}
  //       PRT-O: {{^ *}}#pragma omp target update from(present: s) from(present: h) to(present: d) if(1){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc update self(s) host(h) device(d) if(1){{$}}
  //
  //      EXE-NEXT: if(1)
  // EXE-HOST-NEXT:     host s=11, h=21, d=31{{$}}
  // EXE-HOST-NEXT:   device s=11, h=21, d=31{{$}}
  //  EXE-DEV-NEXT:     host s=11, h=21, d=30{{$}}
  //  EXE-DEV-NEXT:   device s=11, h=21, d=30{{$}}
  printf("if(1)\n");
  setValues();
  #pragma acc update self(s) host(h) device(d) if(1)
  printValues();

  //--------------------------------------------------
  // Check if(intOne).
  //
  // Check printing and dumping for non-constant condition.
  //--------------------------------------------------

  //      DMP: ACCUpdateDirective
  // DMP-NEXT:   ACCSelfClause
  // DMP-NEXT:     DeclRefExpr {{.*}} 's' 'int'
  // DMP-NEXT:   ACCSelfClause
  // DMP-NEXT:     DeclRefExpr {{.*}} 'h' 'int'
  // DMP-NEXT:   ACCDeviceClause
  // DMP-NEXT:     DeclRefExpr {{.*}} 'd' 'int'
  // DMP-NEXT:   ACCIfClause
  // DMP-NEXT:     DeclRefExpr {{.*}} 'intOne' 'int'
  // DMP-NEXT:   impl: OMPTargetUpdateDirective
  // DMP-NEXT:     OMPFromClause
  // DMP-NEXT:       DeclRefExpr {{.*}} 's' 'int'
  // DMP-NEXT:     OMPFromClause
  // DMP-NEXT:       DeclRefExpr {{.*}} 'h' 'int'
  // DMP-NEXT:     OMPToClause
  // DMP-NEXT:       DeclRefExpr {{.*}} 'd' 'int'
  // DMP-NEXT:     OMPIfClause
  // DMP-NEXT:       ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
  // DMP-NEXT:         DeclRefExpr {{.*}} 'int' lvalue OMPCapturedExpr {{.*}} '.capture_expr.' 'int'
  //  DMP-NOT: -{{ACC|OMP}}
  //
  //       PRT-A: {{^ *}}#pragma acc update self(s) host(h) device(d) if(intOne){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target update from(present: s) from(present: h) to(present: d) if(intOne){{$}}
  //       PRT-O: {{^ *}}#pragma omp target update from(present: s) from(present: h) to(present: d) if(intOne){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc update self(s) host(h) device(d) if(intOne){{$}}
  //
  //      EXE-NEXT: if(intOne)
  // EXE-HOST-NEXT:     host s=11, h=21, d=31{{$}}
  // EXE-HOST-NEXT:   device s=11, h=21, d=31{{$}}
  //  EXE-DEV-NEXT:     host s=11, h=21, d=30{{$}}
  //  EXE-DEV-NEXT:   device s=11, h=21, d=30{{$}}
  printf("if(intOne)\n");
  setValues();
  {
    int intOne = 1;
    #pragma acc update self(s) host(h) device(d) if(intOne)
  }
  printValues();

  // DMP: ACCUpdateDirective
  // DMP: OMPTargetUpdateDirective

  //--------------------------------------------------
  // Check various constants besides 1.
  //--------------------------------------------------

  //      EXE-NEXT: if(0)
  // EXE-HOST-NEXT:     host s=11, h=21, d=31{{$}}
  // EXE-HOST-NEXT:   device s=11, h=21, d=31{{$}}
  //  EXE-DEV-NEXT:     host s=10, h=20, d=30{{$}}
  //  EXE-DEV-NEXT:   device s=11, h=21, d=31{{$}}
  printf("if(0)\n");
  setValues();
  #pragma acc update self(s) host(h) device(d) if(0)
  printValues();

  //      EXE-NEXT: if(-1)
  // EXE-HOST-NEXT:     host s=11, h=21, d=31{{$}}
  // EXE-HOST-NEXT:   device s=11, h=21, d=31{{$}}
  //  EXE-DEV-NEXT:     host s=11, h=21, d=30{{$}}
  //  EXE-DEV-NEXT:   device s=11, h=21, d=30{{$}}
  printf("if(-1)\n");
  setValues();
  #pragma acc update self(s) host(h) device(d) if(-1)
  printValues();

  //      EXE-NEXT: if(37)
  // EXE-HOST-NEXT:     host s=11, h=21, d=31{{$}}
  // EXE-HOST-NEXT:   device s=11, h=21, d=31{{$}}
  //  EXE-DEV-NEXT:     host s=11, h=21, d=30{{$}}
  //  EXE-DEV-NEXT:   device s=11, h=21, d=30{{$}}
  printf("if(37)\n");
  setValues();
  #pragma acc update self(s) host(h) device(d) if(37)
  printValues();

  //      EXE-NEXT: if(-9)
  // EXE-HOST-NEXT:     host s=11, h=21, d=31{{$}}
  // EXE-HOST-NEXT:   device s=11, h=21, d=31{{$}}
  //  EXE-DEV-NEXT:     host s=11, h=21, d=30{{$}}
  //  EXE-DEV-NEXT:   device s=11, h=21, d=30{{$}}
  printf("if(-9)\n");
  setValues();
  #pragma acc update self(s) host(h) device(d) if(-9)
  printValues();

  //      EXE-NEXT: if(0.0)
  // EXE-HOST-NEXT:     host s=11, h=21, d=31{{$}}
  // EXE-HOST-NEXT:   device s=11, h=21, d=31{{$}}
  //  EXE-DEV-NEXT:     host s=10, h=20, d=30{{$}}
  //  EXE-DEV-NEXT:   device s=11, h=21, d=31{{$}}
  printf("if(0.0)\n");
  setValues();
  #pragma acc update self(s) host(h) device(d) if(0.0)
  printValues();

  //      EXE-NEXT: if(0.1)
  // EXE-HOST-NEXT:     host s=11, h=21, d=31{{$}}
  // EXE-HOST-NEXT:   device s=11, h=21, d=31{{$}}
  //  EXE-DEV-NEXT:     host s=11, h=21, d=30{{$}}
  //  EXE-DEV-NEXT:   device s=11, h=21, d=30{{$}}
  printf("if(0.1)\n");
  setValues();
  /* expected-warning@+2 {{implicit conversion from 'double' to '_Bool' changes value from 0.1 to true}} */
  /* acc-warning@+1 {{implicit conversion from 'double' to '_Bool' changes value from 0.1 to true}} */
  #pragma acc update self(s) host(h) device(d) if(0.1)
  printValues();

  //      EXE-NEXT: if(-19.)
  // EXE-HOST-NEXT:     host s=11, h=21, d=31{{$}}
  // EXE-HOST-NEXT:   device s=11, h=21, d=31{{$}}
  //  EXE-DEV-NEXT:     host s=11, h=21, d=30{{$}}
  //  EXE-DEV-NEXT:   device s=11, h=21, d=30{{$}}
  printf("if(-19.)\n");
  setValues();
  /* expected-warning@+2 {{implicit conversion from 'double' to '_Bool' changes value from -19 to true}} */
  /* acc-warning@+1 {{implicit conversion from 'double' to '_Bool' changes value from -19 to true}} */
  #pragma acc update self(s) host(h) device(d) if(-19.)
  printValues();

  //      EXE-NEXT: if(NULL)
  // EXE-HOST-NEXT:     host s=11, h=21, d=31{{$}}
  // EXE-HOST-NEXT:   device s=11, h=21, d=31{{$}}
  //  EXE-DEV-NEXT:     host s=10, h=20, d=30{{$}}
  //  EXE-DEV-NEXT:   device s=11, h=21, d=31{{$}}
  printf("if(NULL)\n");
  setValues();
  #pragma acc update self(s) host(h) device(d) if(NULL)
  printValues();

  //--------------------------------------------------
  // Check various non-constant expressions besides intOne.
  //--------------------------------------------------

  //      EXE-NEXT: if(intZero)
  // EXE-HOST-NEXT:     host s=11, h=21, d=31{{$}}
  // EXE-HOST-NEXT:   device s=11, h=21, d=31{{$}}
  //  EXE-DEV-NEXT:     host s=10, h=20, d=30{{$}}
  //  EXE-DEV-NEXT:   device s=11, h=21, d=31{{$}}
  printf("if(intZero)\n");
  setValues();
  {
    int intZero = 0;
    #pragma acc update self(s) host(h) device(d) if(intZero)
  }
  printValues();

  //      EXE-NEXT: if(floatZero)
  // EXE-HOST-NEXT:     host s=11, h=21, d=31{{$}}
  // EXE-HOST-NEXT:   device s=11, h=21, d=31{{$}}
  //  EXE-DEV-NEXT:     host s=10, h=20, d=30{{$}}
  //  EXE-DEV-NEXT:   device s=11, h=21, d=31{{$}}
  printf("if(floatZero)\n");
  setValues();
  {
    float floatZero = 0;
    #pragma acc update self(s) host(h) device(d) if(floatZero)
  }
  printValues();

  //      EXE-NEXT: if(floatNonZero)
  // EXE-HOST-NEXT:     host s=11, h=21, d=31{{$}}
  // EXE-HOST-NEXT:   device s=11, h=21, d=31{{$}}
  //  EXE-DEV-NEXT:     host s=11, h=21, d=30{{$}}
  //  EXE-DEV-NEXT:   device s=11, h=21, d=30{{$}}
  printf("if(floatNonZero)\n");
  setValues();
  {
    float floatNonZero = -0.1;
    #pragma acc update self(s) host(h) device(d) if(floatNonZero)
  }
  printValues();

  //      EXE-NEXT: if(&i)
  // EXE-HOST-NEXT:     host s=11, h=21, d=31{{$}}
  // EXE-HOST-NEXT:   device s=11, h=21, d=31{{$}}
  //  EXE-DEV-NEXT:     host s=11, h=21, d=30{{$}}
  //  EXE-DEV-NEXT:   device s=11, h=21, d=30{{$}}
  printf("if(&i)\n");
  setValues();
  {
    int i;
    /* expected-warning@+2 {{address of 'i' will always evaluate to 'true'}} */
    /* acc-warning@+1 {{address of 'i' will always evaluate to 'true'}} */
    #pragma acc update self(s) host(h) device(d) if(&i)
  }
  printValues();

  //      EXE-NEXT: if(s.i)
  //
  // EXE-HOST-NEXT:     host s=11, h=21, d=31{{$}}
  // EXE-HOST-NEXT:   device s=11, h=21, d=31{{$}}
  //  EXE-DEV-NEXT:     host s=10, h=20, d=30{{$}}
  //  EXE-DEV-NEXT:   device s=11, h=21, d=31{{$}}
  //
  // EXE-HOST-NEXT:     host s=11, h=21, d=31{{$}}
  // EXE-HOST-NEXT:   device s=11, h=21, d=31{{$}}
  //  EXE-DEV-NEXT:     host s=11, h=21, d=30{{$}}
  //  EXE-DEV-NEXT:   device s=11, h=21, d=30{{$}}
  printf("if(s.i)\n");
  setValues();
  {
    struct ST { int i; } st = {0};
    #pragma acc update self(s) host(h) device(d) if(st.i)
    printValues();
    st.i = 1;
    #pragma acc update self(s) host(h) device(d) if(st.i)
  }
  printValues();

  //--------------------------------------------------
  // Finally, check that presence checks are suppressed if condition is zero.
  // That is, there should be no runtime error.
  //--------------------------------------------------

  // EXE-NEXT: suppression of presence checks
  printf("suppression of presence checks\n");
  #pragma acc exit data delete(s,h,d)
  #pragma acc update self(s) host(h) device(d) if(0)
  {
    int intZero = 0;
    #pragma acc update self(s) host(h) device(d) if(intZero)
  }

  return 0;
}

// EXE-NOT: {{.}}
