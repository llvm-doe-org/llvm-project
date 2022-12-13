// Check that declarations are handled correctly during OpenACC construct
// transformations.
//
// Specifically, TransformACCToOMP uses TransformContext to update the
// DeclContext of each declaration in an OpenACC construct.  Without that,
// spurious compiler errors or assert fails can occur during Sema or CodeGen.
// Additionally, unhandled declaration kinds produce an assert fail directly
// within TransformContext.

// We have written no dump checks, so the generated dump test is a dummy.
//
// We check printing of the computed OpenMP source as any easy way to see if any
// components of the declarations are lost by the transformations.
//
// REDEFINE: %{prt:fc:args} = -match-full-lines
// REDEFINE: %{exe:fc:args} = -match-full-lines
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}

// END.

/* expected-no-diagnostics */

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
  //  PRT-A-NEXT:   #pragma acc parallel num_gangs(1) copyout(noInitCopy,initCopy){{$}}
  // PRT-AO-NEXT:   // #pragma omp target teams num_teams(1) map(ompx_hold,from: noInitCopy,initCopy){{$}}
  //  PRT-O-NEXT:   #pragma omp target teams num_teams(1) map(ompx_hold,from: noInitCopy,initCopy){{$}}
  // PRT-OA-NEXT:   // #pragma acc parallel num_gangs(1) copyout(noInitCopy,initCopy){{$}}
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
  //  PRT-A-NEXT:   #pragma acc parallel num_gangs(1) copyout(iCopy){{$}}
  // PRT-AO-NEXT:   // #pragma omp target teams num_teams(1) map(ompx_hold,from: iCopy){{$}}
  //  PRT-O-NEXT:   #pragma omp target teams num_teams(1) map(ompx_hold,from: iCopy){{$}}
  // PRT-OA-NEXT:   // #pragma acc parallel num_gangs(1) copyout(iCopy){{$}}
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
  //  PRT-A-NEXT:   #pragma acc parallel num_gangs(1) copyout(CCopy){{$}}
  // PRT-AO-NEXT:   // #pragma omp target teams num_teams(1) map(ompx_hold,from: CCopy){{$}}
  //  PRT-O-NEXT:   #pragma omp target teams num_teams(1) map(ompx_hold,from: CCopy){{$}}
  // PRT-OA-NEXT:   // #pragma acc parallel num_gangs(1) copyout(CCopy){{$}}
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
  //    PRT-NEXT: {{(^#if !TGT_NVPTX64$[[:space:]]*)?;}}
  //    PRT-NEXT: {
  //    PRT-NEXT:   int siCopy;
  //    PRT-NEXT:   int sxiCopy;
  //    PRT-NEXT:   int sxjCopy;
  //  PRT-A-NEXT:   #pragma acc parallel num_gangs(1) copyout(siCopy,sxiCopy,sxjCopy){{$}}
  // PRT-AO-NEXT:   // #pragma omp target teams num_teams(1) map(ompx_hold,from: siCopy,sxiCopy,sxjCopy){{$}}
  //  PRT-O-NEXT:   #pragma omp target teams num_teams(1) map(ompx_hold,from: siCopy,sxiCopy,sxjCopy){{$}}
  // PRT-OA-NEXT:   // #pragma acc parallel num_gangs(1) copyout(siCopy,sxiCopy,sxjCopy){{$}}
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
  //        EXE-HOST-NEXT: s.i=23
  //        EXE-HOST-NEXT: s.x.i=918
  //        EXE-HOST-NEXT: s.x.j=17
  //  EXE-TGT-X86_64-NEXT: s.i=23
  //  EXE-TGT-X86_64-NEXT: s.x.i=918
  //  EXE-TGT-X86_64-NEXT: s.x.j=17
  // EXE-TGT-PPC64LE-NEXT: s.i=23
  // EXE-TGT-PPC64LE-NEXT: s.x.i=918
  // EXE-TGT-PPC64LE-NEXT: s.x.j=17
  //  EXE-TGT-AMDGCN-NEXT: s.i=23
  //  EXE-TGT-AMDGCN-NEXT: s.x.i=918
  //  EXE-TGT-AMDGCN-NEXT: s.x.j=17
#if !TGT_NVPTX64
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
  //  PRT-A-NEXT:   #pragma acc parallel num_gangs(1) copyout(uxiCopy){{$}}
  // PRT-AO-NEXT:   // #pragma omp target teams num_teams(1) map(ompx_hold,from: uxiCopy){{$}}
  //  PRT-O-NEXT:   #pragma omp target teams num_teams(1) map(ompx_hold,from: uxiCopy){{$}}
  // PRT-OA-NEXT:   // #pragma acc parallel num_gangs(1) copyout(uxiCopy){{$}}
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
