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
// RUN:   (prt-opt=-fopenacc-ast-print prt-kind=ast-prt)
// RUN:   (prt-opt=-fopenacc-print     prt-kind=prt    )
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
// RUN:     -Wno-openacc-omp-map-ompx-hold \
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
// RUN:   %clang %[prt] -Wno-openacc-omp-map-ompx-hold %t.ast 2>&1 \
// RUN:   | FileCheck -check-prefixes=%[prt-chk] -match-full-lines %s
// RUN: }

// Can we print the OpenMP source code, compile, and run it successfully?
//
// RUN: %data tgts {
// RUN:   (run-if=                tgt-cflags='                                               -Xclang -verify'         nvptx64=NO-NVPTX64)
// RUN:   (run-if=%run-if-x86_64  tgt-cflags='-fopenmp-targets=%run-x86_64-triple            -Xclang -verify'         nvptx64=NO-NVPTX64)
// RUN:   (run-if=%run-if-ppc64le tgt-cflags='-fopenmp-targets=%run-ppc64le-triple           -Xclang -verify'         nvptx64=NO-NVPTX64)
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags='-fopenmp-targets=%run-nvptx64-triple -DNVPTX64 -Xclang -verify=nvptx64' nvptx64=NVPTX64   )
// RUN:   (run-if=%run-if-amdgcn  tgt-cflags='-fopenmp-targets=%run-amdgcn-triple            -Xclang -verify'         nvptx64=NO-NVPTX64)
// RUN: }
// RUN: %for prt-opts {
// RUN:   %clang -Xclang -verify %[prt-opt]=omp -Wno-openacc-omp-map-ompx-hold \
// RUN:     %s > %t-%[prt-kind]-omp.c
// RUN:   echo "// expected""-no-diagnostics" >> %t-%[prt-kind]-omp.c
// RUN: }
// RUN: %clang -Xclang -verify -fopenmp %fopenmp-version \
// RUN:   -Wno-unused-function -o %t %t-ast-prt-omp.c
// RUN: %t 2 > %t.out 2>&1
// RUN: FileCheck -input-file %t.out \
// RUN:   -check-prefixes=EXE,EXE-NO-NVPTX64 -match-full-lines %s
// RUN: %for tgts {
// RUN:   %[run-if] %clang -fopenmp %fopenmp-version %[tgt-cflags] -o %t \
// RUN:     %t-prt-omp.c
// RUN:   %[run-if] %t 2 > %t.out 2>&1
// RUN:   %[run-if] FileCheck -input-file %t.out \
// RUN:       -check-prefixes=EXE,EXE-%[nvptx64] -match-full-lines %s
// RUN: }

// Check execution with normal compilation.
//
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
  //    PRT-NEXT: {
  //    PRT-NEXT:   int noInitCopy, initCopy;
  //  PRT-A-NEXT:   #pragma acc parallel num_gangs(1) copyout(noInitCopy,initCopy)
  // PRT-AO-NEXT:   // #pragma omp target teams num_teams(1) map(ompx_hold,from: noInitCopy,initCopy) shared(noInitCopy,initCopy)
  //  PRT-O-NEXT:   #pragma omp target teams num_teams(1) map(ompx_hold,from: noInitCopy,initCopy) shared(noInitCopy,initCopy)
  // PRT-OA-NEXT:   // #pragma acc parallel num_gangs(1) copyout(noInitCopy,initCopy)
  //    PRT-NEXT:   {
  //    PRT-NEXT:     int noInit;
  //    PRT-NEXT:     noInit = 4;
  //    PRT-NEXT:     int init = 5;
  //    PRT-NEXT:     noInitCopy = noInit;
  //    PRT-NEXT:     initCopy = init;
  //    PRT-NEXT:   }
  //    PRT-NEXT:   printf("noInit: %d\n", noInitCopy);
  //    PRT-NEXT:   printf("init: %d\n", initCopy);
  //    PRT-NEXT: }
  //
  // EXE-NEXT: noInit: 4
  // EXE-NEXT: init: 5
  {
    int noInitCopy, initCopy;
    #pragma acc parallel num_gangs(1) copyout(noInitCopy,initCopy)
    {
      int noInit;
      noInit = 4;
      int init = 5;
      noInitCopy = noInit;
      initCopy = init;
    }
    printf("noInit: %d\n", noInitCopy);
    printf("init: %d\n", initCopy);
  }

  // Check typedef declarations.
  //
  //    PRT-NEXT: {
  //    PRT-NEXT:   int iCopy;
  //  PRT-A-NEXT:   #pragma acc parallel num_gangs(1) copyout(iCopy)
  // PRT-AO-NEXT:   // #pragma omp target teams num_teams(1) map(ompx_hold,from: iCopy) shared(iCopy)
  //  PRT-O-NEXT:   #pragma omp target teams num_teams(1) map(ompx_hold,from: iCopy) shared(iCopy)
  // PRT-OA-NEXT:   // #pragma acc parallel num_gangs(1) copyout(iCopy)
  //    PRT-NEXT:   {
  //    PRT-NEXT:     typedef int Number;
  //    PRT-NEXT:     Number i = 809;
  //    PRT-NEXT:     iCopy = i;
  //    PRT-NEXT:   }
  //    PRT-NEXT:   printf("i: %d\n", iCopy);
  //    PRT-NEXT: }
  //
  // EXE-NEXT: i: 809
  {
    int iCopy;
    #pragma acc parallel num_gangs(1) copyout(iCopy)
    {
      typedef int Number;
      Number i = 809;
      iCopy = i;
    }
    printf("i: %d\n", iCopy);
  }

  // Check enum declarations.
  //
  //    PRT-NEXT: {
  //    PRT-NEXT:   int CCopy;
  //  PRT-A-NEXT:   #pragma acc parallel num_gangs(1) copyout(CCopy)
  // PRT-AO-NEXT:   // #pragma omp target teams num_teams(1) map(ompx_hold,from: CCopy) shared(CCopy)
  //  PRT-O-NEXT:   #pragma omp target teams num_teams(1) map(ompx_hold,from: CCopy) shared(CCopy)
  // PRT-OA-NEXT:   // #pragma acc parallel num_gangs(1) copyout(CCopy)
  //    PRT-NEXT:   {
  //    PRT-NEXT:     enum {
  //    PRT-NEXT:       A = 3
  //    PRT-NEXT:     };
  //    PRT-NEXT:     enum E;
  //    PRT-NEXT:     enum E {
  //    PRT-NEXT:       B = A,
  //    PRT-NEXT:       C = B + 1
  //    PRT-NEXT:     };
  //    PRT-NEXT:     CCopy = C;
  //    PRT-NEXT:   }
  //    PRT-NEXT:   printf("C: %d\n", CCopy);
  //    PRT-NEXT: }
  //
  // EXE-NEXT: C: 4
  {
    int CCopy;
    #pragma acc parallel num_gangs(1) copyout(CCopy)
    {
      enum {
        A = 3
      };
      enum E;
      enum E {
        B = A,
        C = B + 1
      };
      CCopy = C;
    }
    printf("C: %d\n", CCopy);
  }

  // Check struct declarations.
  //
  //    PRT-NEXT: {{(^#if !NVPTX64$[[:space:]]*)?;}}
  //    PRT-NEXT: {
  //    PRT-NEXT:   int siCopy;
  //    PRT-NEXT:   int sxiCopy;
  //    PRT-NEXT:   int sxjCopy;
  //  PRT-A-NEXT:   #pragma acc parallel num_gangs(1) copyout(siCopy,sxiCopy,sxjCopy)
  // PRT-AO-NEXT:   // #pragma omp target teams num_teams(1) map(ompx_hold,from: siCopy,sxiCopy,sxjCopy) shared(siCopy,sxiCopy,sxjCopy)
  //  PRT-O-NEXT:   #pragma omp target teams num_teams(1) map(ompx_hold,from: siCopy,sxiCopy,sxjCopy) shared(siCopy,sxiCopy,sxjCopy)
  // PRT-OA-NEXT:   // #pragma acc parallel num_gangs(1) copyout(siCopy,sxiCopy,sxjCopy)
  //    PRT-NEXT:   {
  //    PRT-NEXT:     struct S;
  //    PRT-NEXT:     struct S {
  //    PRT-NEXT:       int i;
  //    PRT-NEXT:       struct {
  //    PRT-NEXT:         int i;
  //    PRT-NEXT:         int j;
  //    PRT-NEXT:       } x;
  //    PRT-NEXT:     };
  //    PRT-NEXT:     struct S s = {23, {918, 17}};
  //    PRT-NEXT:     siCopy = s.i;
  //    PRT-NEXT:     sxiCopy = s.x.i;
  //    PRT-NEXT:     sxjCopy = s.x.j;
  //    PRT-NEXT:   }
  //    PRT-NEXT:   printf("s.i=%d\n", siCopy);
  //    PRT-NEXT:   printf("s.x.i=%d\n", sxiCopy);
  //    PRT-NEXT:   printf("s.x.j=%d\n", sxjCopy);
  //    PRT-NEXT: }
  //    PRT-NEXT: {{(^#endif$[[:space:]]*)?;}}
  //
  // EXE-NO-NVPTX64-NEXT: s.i=23
  // EXE-NO-NVPTX64-NEXT: s.x.i=918
  // EXE-NO-NVPTX64-NEXT: s.x.j=17
#if !NVPTX64
  ;
  {
    int siCopy;
    int sxiCopy;
    int sxjCopy;
    #pragma acc parallel num_gangs(1) copyout(siCopy,sxiCopy,sxjCopy)
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
      siCopy = s.i;
      sxiCopy = s.x.i;
      sxjCopy = s.x.j;
    }
    printf("s.i=%d\n", siCopy);
    printf("s.x.i=%d\n", sxiCopy);
    printf("s.x.j=%d\n", sxjCopy);
  }
#endif
  ;

  // Check union declarations.
  //
  //    PRT-NEXT: {
  //    PRT-NEXT:   int uxiCopy;
  //  PRT-A-NEXT:   #pragma acc parallel num_gangs(1) copyout(uxiCopy)
  // PRT-AO-NEXT:   // #pragma omp target teams num_teams(1) map(ompx_hold,from: uxiCopy) shared(uxiCopy)
  //  PRT-O-NEXT:   #pragma omp target teams num_teams(1) map(ompx_hold,from: uxiCopy) shared(uxiCopy)
  // PRT-OA-NEXT:   // #pragma acc parallel num_gangs(1) copyout(uxiCopy)
  //    PRT-NEXT:   {
  //    PRT-NEXT:     union U;
  //    PRT-NEXT:     union U {
  //    PRT-NEXT:       int i;
  //    PRT-NEXT:       union {
  //    PRT-NEXT:         int i;
  //    PRT-NEXT:         int j;
  //    PRT-NEXT:       } x;
  //    PRT-NEXT:     };
  //    PRT-NEXT:     union U u;
  //    PRT-NEXT:     u.x.i = 783;
  //    PRT-NEXT:     uxiCopy = u.x.i;
  //    PRT-NEXT:   }
  //    PRT-NEXT:   printf("u.x.i=%d\n", uxiCopy);
  //    PRT-NEXT: }
  //
  // EXE-NEXT: u.x.i=783
  {
    int uxiCopy;
    #pragma acc parallel num_gangs(1) copyout(uxiCopy)
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
      uxiCopy = u.x.i;
    }
    printf("u.x.i=%d\n", uxiCopy);
  }

  // PRT-NEXT: printf("end\n");
  // EXE-NEXT: end
  // EXE-NOT: {{.}}
  printf("end\n");
  return 0;
}
