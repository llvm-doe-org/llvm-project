// Check that declarations are handled correctly during OpenACC construct
// transformations.
//
// Specifically, TransformACCToOMP uses TransformContext to update the
// DeclContext of each declaration in an OpenACC construct.  Without that,
// spurious compiler errors or assert fails can occur during Sema or CodeGen.
// Additionally, unhandled declaration kinds produce an assert fail directly
// within TransformContext.
//
// We check printing of the computed OpenMP source as any easy way to see if any
// components of the declarations are lost by the transformations.

// Check -ast-print and -fopenacc[-ast]-print.
//
// RUN: %clang -Xclang -verify -Xclang -ast-print -fsyntax-only %s \
// RUN: | FileCheck -check-prefixes=PRT -match-full-lines %s
//
// Strip comments and blank lines so checking -fopenacc-print output is easier.
// RUN: echo "// expected""-no-diagnostics" > %t-acc.c
// RUN: grep -v '^ *\(//.*\)\?$' %s | sed 's,//.*,,' >> %t-acc.c
//
// TODO: If lit were to support %for inside a %data, we could iterate prt-opts
// within prt-args after the first prt-args iteration, significantly shortening
// the prt-args definition.
//
// RUN: %data prt-opts {
// RUN:   (prt-opt=-fopenacc-ast-print)
// RUN:   (prt-opt=-fopenacc-print    )
// RUN: }
// RUN: %data prt-args {
// RUN:   (prt='-Xclang -ast-print -fsyntax-only -fopenacc' prt-chk=PRT,PRT-A       )
// RUN:   (prt=-fopenacc-ast-print=acc                      prt-chk=PRT,PRT-A       )
// RUN:   (prt=-fopenacc-ast-print=omp                      prt-chk=PRT,PRT-O       )
// RUN:   (prt=-fopenacc-ast-print=acc-omp                  prt-chk=PRT,PRT-A,PRT-AO)
// RUN:   (prt=-fopenacc-ast-print=omp-acc                  prt-chk=PRT,PRT-O,PRT-OA)
// RUN:   (prt=-fopenacc-print=acc                          prt-chk=PRT,PRT-A       )
// RUN:   (prt=-fopenacc-print=omp                          prt-chk=PRT,PRT-O       )
// RUN:   (prt=-fopenacc-print=acc-omp                      prt-chk=PRT,PRT-A,PRT-AO)
// RUN:   (prt=-fopenacc-print=omp-acc                      prt-chk=PRT,PRT-O,PRT-OA)
// RUN: }
// RUN: %for prt-args {
// RUN:   %clang -Xclang -verify %[prt] %t-acc.c \
// RUN:   | FileCheck -check-prefixes=%[prt-chk] -match-full-lines %s
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
// RUN:   | FileCheck -check-prefixes=%[prt-chk] -match-full-lines %s
// RUN: }

// Can we print the OpenMP source code, compile, and run it successfully?
//
// RUN: %for prt-opts {
// RUN:   %clang -Xclang -verify %[prt-opt]=omp %s > %t-omp.c
// RUN:   echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN:   %clang -Xclang -verify -fopenmp %fopenmp-version \
// RUN:          -Wno-unused-function -o %t %t-omp.c
// RUN:   %t 2 2>&1 \
// RUN:   | FileCheck -check-prefixes=EXE,EXE-NO-NVPTX64 -match-full-lines %s
// RUN: }

// Check execution with normal compilation.
//
// RUN: %data tgts {
// RUN:   (run-if=                tgt-cflags='                                               -Xclang -verify'  nvptx64=NO-NVPTX64)
// RUN:   (run-if=%run-if-x86_64  tgt-cflags='-fopenmp-targets=%run-x86_64-triple            -Xclang -verify'  nvptx64=NO-NVPTX64)
// RUN:   (run-if=%run-if-ppc64le tgt-cflags='-fopenmp-targets=%run-ppc64le-triple           -Xclang -verify'  nvptx64=NO-NVPTX64)
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags='-fopenmp-targets=%run-nvptx64-triple -DNVPTX64 -Xclang -verify=nvptx64' nvptx64=NVPTX64   )
// RUN: }
// RUN: %for tgts {
// RUN:   %[run-if] %clang -fopenacc %[tgt-cflags] -o %t %s
// RUN:   %[run-if] %t 2 > %t.out 2>&1
// RUN:   %[run-if] FileCheck -input-file %t.out \
// RUN:       -check-prefixes=EXE,EXE-%[nvptx64] -match-full-lines %s
// RUN: }

// END.

// expected-no-diagnostics

// FIXME: Clang produces spurious warning diagnostics for nvptx64 offload.  This
// issue is not limited to Clacc and is present upstream:
// nvptx64-warning@*:* 0+ {{Linking two modules of different data layouts}}
// nvptx64-warning@*:* 0+ {{Linking two modules of different target triples}}

#include <stdio.h>

int main() {
  // PRT: printf("start\n");
  // EXE-NOT: {{.}}
  // EXE: start
  printf("start\n");

  // Check variable declarations.
  //
  //  PRT-A-NEXT: #pragma acc parallel num_gangs(1)
  // PRT-AO-NEXT: // #pragma omp target teams num_teams(1)
  //  PRT-O-NEXT: #pragma omp target teams num_teams(1)
  // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1)
  //    PRT-NEXT: {
  //    PRT-NEXT:   int noInit;
  //    PRT-NEXT:   noInit = 4;
  //    PRT-NEXT:   int init = 5;
  //    PRT-NEXT:   printf("noInit: %d\n", noInit);
  //    PRT-NEXT:   printf("init: %d\n", init);
  //    PRT-NEXT: }
  //
  // EXE-NEXT: noInit: 4
  // EXE-NEXT: init: 5
  #pragma acc parallel num_gangs(1)
  {
    int noInit;
    noInit = 4;
    int init = 5;
    printf("noInit: %d\n", noInit);
    printf("init: %d\n", init);
  }

  // Check typedef declarations.
  //
  //  PRT-A-NEXT: #pragma acc parallel num_gangs(1)
  // PRT-AO-NEXT: // #pragma omp target teams num_teams(1)
  //  PRT-O-NEXT: #pragma omp target teams num_teams(1)
  // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1)
  //    PRT-NEXT: {
  //    PRT-NEXT:   typedef int Number;
  //    PRT-NEXT:   Number i = 809;
  //    PRT-NEXT:   printf("i: %d\n", i);
  //    PRT-NEXT: }
  //
  // EXE-NEXT: i: 809
  #pragma acc parallel num_gangs(1)
  {
    typedef int Number;
    Number i = 809;
    printf("i: %d\n", i);
  }

  // Check enum declarations.
  //
  //  PRT-A-NEXT: #pragma acc parallel num_gangs(1)
  // PRT-AO-NEXT: // #pragma omp target teams num_teams(1)
  //  PRT-O-NEXT: #pragma omp target teams num_teams(1)
  // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1)
  //    PRT-NEXT: {
  //    PRT-NEXT:   enum {
  //    PRT-NEXT:     A = 3
  //    PRT-NEXT:   };
  //    PRT-NEXT:   enum E;
  //    PRT-NEXT:   enum E {
  //    PRT-NEXT:     B = A,
  //    PRT-NEXT:     C = B + 1
  //    PRT-NEXT:   };
  //    PRT-NEXT:   printf("C: %d\n", C);
  //    PRT-NEXT: }
  //
  // EXE-NEXT: C: 4
  #pragma acc parallel num_gangs(1)
  {
    enum {
      A = 3
    };
    enum E;
    enum E {
      B = A,
      C = B + 1
    };
    printf("C: %d\n", C);
  }

  // Check struct declarations.
  //
  //    PRT-NEXT: {{(^#if !NVPTX64$[[:space:]]*)?;}}
  //  PRT-A-NEXT: #pragma acc parallel num_gangs(1)
  // PRT-AO-NEXT: // #pragma omp target teams num_teams(1)
  //  PRT-O-NEXT: #pragma omp target teams num_teams(1)
  // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1)
  //    PRT-NEXT: {
  //    PRT-NEXT:   struct S;
  //    PRT-NEXT:   struct S {
  //    PRT-NEXT:     int i;
  //    PRT-NEXT:     struct {
  //    PRT-NEXT:       int i;
  //    PRT-NEXT:       int j;
  //    PRT-NEXT:     } x;
  //    PRT-NEXT:   };
  //    PRT-NEXT:   struct S s = {23, {918, 17}};
  //    PRT-NEXT:   printf("s.i=%d\n", s.i);
  //    PRT-NEXT:   printf("s.x.i=%d\n", s.x.i);
  //    PRT-NEXT:   printf("s.x.j=%d\n", s.x.j);
  //    PRT-NEXT: }
  //    PRT-NEXT: {{(^#endif$[[:space:]]*)?;}}
  //
  // EXE-NO-NVPTX64-NEXT: s.i=23
  // EXE-NO-NVPTX64-NEXT: s.x.i=918
  // EXE-NO-NVPTX64-NEXT: s.x.j=17
#if !NVPTX64
  ;
  #pragma acc parallel num_gangs(1)
  {
    struct S;
    struct S {
      int i;
      // FIXME: Clang codegen for nested struct definitions within omp target
      // teams fails on nvptx64 with an error like this:
      //
      //   fatal error: error in backend: Symbol name with unsupported characters
      //   clang-12: error: clang frontend command failed with exit code 70 (use -v to see invocation)
      //
      // This is reproducible when the source code is originally OpenMP and so
      // OpenACC support is not involved.
      struct {
        int i;
        int j;
      } x;
    };
    struct S s = {23, {918, 17}};
    printf("s.i=%d\n", s.i);
    printf("s.x.i=%d\n", s.x.i);
    printf("s.x.j=%d\n", s.x.j);
  }
#endif
  ;

  // Check union declarations.
  //
  //  PRT-A-NEXT: #pragma acc parallel num_gangs(1)
  // PRT-AO-NEXT: // #pragma omp target teams num_teams(1)
  //  PRT-O-NEXT: #pragma omp target teams num_teams(1)
  // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1)
  //    PRT-NEXT: {
  //    PRT-NEXT:   union U;
  //    PRT-NEXT:   union U {
  //    PRT-NEXT:     int i;
  //    PRT-NEXT:     union {
  //    PRT-NEXT:       int i;
  //    PRT-NEXT:       int j;
  //    PRT-NEXT:     } x;
  //    PRT-NEXT:   };
  //    PRT-NEXT:   union U u;
  //    PRT-NEXT:   u.x.i = 783;
  //    PRT-NEXT:   printf("u.x.i=%d\n", u.x.i);
  //    PRT-NEXT: }
  //
  // EXE-NEXT: u.x.i=783
  #pragma acc parallel num_gangs(1)
  {
    union U;
    union U {
      int i;
      union {
        int i;
        int j;
      } x;
    };
    union U u;
    u.x.i = 783;
    printf("u.x.i=%d\n", u.x.i);
  }

  // PRT-NEXT: printf("end\n");
  // EXE-NEXT: end
  // EXE-NOT: {{.}}
  printf("end\n");
  return 0;
}
