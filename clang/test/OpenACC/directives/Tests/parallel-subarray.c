// Check subarrays in clauses on "acc parallel" except the present and no_create
// clauses are checked in present.c and no-create.c.  Subarray extension errors
// are checked in data-extension-errors.c
// (in openmp/libacc2omp/test/directives).
//
// We cannot access elements that aren't mapped without risking run-time
// crashes, so we cannot check that those elements actually aren't mapped
// without looking at the generated LLVM IR.  Leave that to OpenMP testing.
// However, we can test that mapped elements can be written without crashing
// and whether the written elements are copied back out.

// REDEFINE: %{all:clang:args} = -DACCC=copy
// REDEFINE: %{all:fc:pres} = C
// REDEFINE: %{prt:fc:args} = -DACCC=copy -DOMPMT=ompx_hold,tofrom
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}

// REDEFINE: %{all:clang:args} = -DACCC=copyin
// REDEFINE: %{all:fc:pres} = CI
// REDEFINE: %{prt:fc:args} = -DACCC=copyin -DOMPMT=ompx_hold,to
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}

// REDEFINE: %{all:clang:args} = -DACCC=copyout
// REDEFINE: %{all:fc:pres} = CO
// REDEFINE: %{prt:fc:args} = -DACCC=copyout -DOMPMT=ompx_hold,from
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}

// REDEFINE: %{all:clang:args} = -DACCC=create
// REDEFINE: %{all:fc:pres} = CR
// REDEFINE: %{prt:fc:args} = -DACCC=create -DOMPMT=ompx_hold,alloc
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}

// END.

/* expected-no-diagnostics */

#include <stdio.h>

void printArr1_(int *arr1, int n) {
  printf("<");
  for (int i = 0; i < n; ++i) {
    if (i > 0)
      printf(", ");
    printf("%d", arr1[i]);
  }
  printf(">");
}

void printArr2_(int arr2[][2], int n) {
  printf("<");
  for (int i = 0; i < n; ++i) {
    if (i > 0)
      printf(", ");
    printArr1_(arr2[i], 2);
  }
  printf(">");
}

void printArr1(const char *pre, int *arr1, int n) {
  printf("%s: ", pre);
  printArr1_(arr1, n);
  printf("\n");
}

void printArr2(const char *pre, int arr2[][2], int n) {
  printf("%s: ", pre);
  printArr2_(arr2, n);
  printf("\n");
}

int main() {

  //--------------------------------------------------
  // arr1[0:1]
  //--------------------------------------------------

  {
    int n = 6;
    int arr1[n];
    for (int i = 0; i < n; ++i)
      arr1[i] = 11;

    // DMP-LABEL: "arr1[0:1] before"
    // PRT-LABEL: printArr1("arr1[0:1] before"
    // EXE-LABEL: arr1[0:1] before:
    // EXE-SAME:  <11, 11, 11, 11, 11, 11>
    printArr1("arr1[0:1] before", arr1, n);

    // DMP:         ACCParallelDirective
    // DMP-NEXT:      ACCNumGangsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 1
    // DMP-C-NEXT:    ACCCopyClause
    // DMP-CI-NEXT:   ACCCopyinClause
    // DMP-CO-NEXT:   ACCCopyoutClause
    // DMP-CR-NEXT:   ACCCreateClause
    // DMP-NEXT:        OMPArraySectionExpr
    // DMP-NEXT:          ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:            DeclRefExpr {{.*}} 'arr1' 'int[n]'
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:          <<<NULL>>>
    // DMP-NEXT:      ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'arr1' 'int[n]'
    // DMP-NEXT:      impl: OMPTargetTeamsDirective
    // DMP-NEXT:        OMPNum_teamsClause
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:        OMPMapClause
    // DMP-NEXT:          OMPArraySectionExpr
    // DMP-NEXT:            ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'arr1' 'int[n]'
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:        OMPSharedClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'arr1' 'int[n]'
    //
    //  PRT-A-AST-NEXT: #pragma acc parallel num_gangs(1) [[ACCC]](arr1[0:1]){{$}}
    //  PRT-A-SRC-NEXT: #pragma acc parallel num_gangs(1) ACCC(arr1[0:1]){{$}}
    //     PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map([[OMPMT]]: arr1[0:1]) shared(arr1){{$}}
    //      PRT-O-NEXT: #pragma omp target teams num_teams(1) map([[OMPMT]]: arr1[0:1]) shared(arr1){{$}}
    // PRT-OA-AST-NEXT: // #pragma acc parallel num_gangs(1) [[ACCC]](arr1[0:1]){{$}}
    // PRT-OA-SRC-NEXT: // #pragma acc parallel num_gangs(1) ACCC(arr1[0:1]){{$}}
    #pragma acc parallel num_gangs(1) ACCC(arr1[0:1])
    for (int i = 0; i < 1; ++i)
      arr1[i] = 99;

    //       EXE-C-NEXT: arr1[0:1] after: <99, 11, 11, 11, 11, 11>
    //      EXE-CO-NEXT: arr1[0:1] after: <99, 11, 11, 11, 11, 11>
    // EXE-HOST-CI-NEXT: arr1[0:1] after: <99, 11, 11, 11, 11, 11>
    // EXE-HOST-CR-NEXT: arr1[0:1] after: <99, 11, 11, 11, 11, 11>
    //  EXE-OFF-CI-NEXT: arr1[0:1] after: <11, 11, 11, 11, 11, 11>
    //  EXE-OFF-CR-NEXT: arr1[0:1] after: <11, 11, 11, 11, 11, 11>
    printArr1("arr1[0:1] after", arr1, n);
  }

  //--------------------------------------------------
  // arr1[0:2]
  //--------------------------------------------------

  {
    int n = 6;
    int arr1[n];
    for (int i = 0; i < n; ++i)
      arr1[i] = 11;

    // DMP-LABEL: "arr1[0:2] before"
    // PRT-LABEL: printArr1("arr1[0:2] before"
    // EXE-LABEL: arr1[0:2] before:
    // EXE-SAME:  <11, 11, 11, 11, 11, 11>
    printArr1("arr1[0:2] before", arr1, n);

    // DMP:         ACCParallelDirective
    // DMP-NEXT:      ACCNumGangsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 1
    // DMP-C-NEXT:    ACCCopyClause
    // DMP-CI-NEXT:   ACCCopyinClause
    // DMP-CO-NEXT:   ACCCopyoutClause
    // DMP-CR-NEXT:   ACCCreateClause
    // DMP-NEXT:        OMPArraySectionExpr
    // DMP-NEXT:          ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:            DeclRefExpr {{.*}} 'arr1' 'int[n]'
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:          <<<NULL>>>
    // DMP-NEXT:      ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'arr1' 'int[n]'
    // DMP-NEXT:      impl: OMPTargetTeamsDirective
    // DMP-NEXT:        OMPNum_teamsClause
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:        OMPMapClause
    // DMP-NEXT:          OMPArraySectionExpr
    // DMP-NEXT:            ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'arr1' 'int[n]'
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:        OMPSharedClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'arr1' 'int[n]'
    //
    //  PRT-A-AST-NEXT: #pragma acc parallel num_gangs(1) [[ACCC]](arr1[0:2]){{$}}
    //  PRT-A-SRC-NEXT: #pragma acc parallel num_gangs(1) ACCC(arr1[0:2]){{$}}
    //     PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map([[OMPMT]]: arr1[0:2]) shared(arr1){{$}}
    //      PRT-O-NEXT: #pragma omp target teams num_teams(1) map([[OMPMT]]: arr1[0:2]) shared(arr1){{$}}
    // PRT-OA-AST-NEXT: // #pragma acc parallel num_gangs(1) [[ACCC]](arr1[0:2]){{$}}
    // PRT-OA-SRC-NEXT: // #pragma acc parallel num_gangs(1) ACCC(arr1[0:2]){{$}}
    #pragma acc parallel num_gangs(1) ACCC(arr1[0:2])
    for (int i = 0; i < 2; ++i)
      arr1[i] = 99;

    //       EXE-C-NEXT: arr1[0:2] after: <99, 99, 11, 11, 11, 11>
    //      EXE-CO-NEXT: arr1[0:2] after: <99, 99, 11, 11, 11, 11>
    // EXE-HOST-CI-NEXT: arr1[0:2] after: <99, 99, 11, 11, 11, 11>
    // EXE-HOST-CR-NEXT: arr1[0:2] after: <99, 99, 11, 11, 11, 11>
    //  EXE-OFF-CI-NEXT: arr1[0:2] after: <11, 11, 11, 11, 11, 11>
    //  EXE-OFF-CR-NEXT: arr1[0:2] after: <11, 11, 11, 11, 11, 11>
    printArr1("arr1[0:2] after", arr1, n);
  }

  //--------------------------------------------------
  // pi[0:3]
  //--------------------------------------------------

  {
    int n = 6;
    int arr1[n];
    for (int i = 0; i < n; ++i)
      arr1[i] = 11;
    int *pi = arr1;

    // DMP-LABEL: "pi[0:3] before"
    // PRT-LABEL: printArr1("pi[0:3] before"
    // EXE-LABEL: pi[0:3] before:
    // EXE-SAME:  <11, 11, 11, 11, 11, 11>
    printArr1("pi[0:3] before", arr1, n);

    // DMP:         ACCParallelDirective
    // DMP-NEXT:      ACCNumGangsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 1
    // DMP-C-NEXT:    ACCCopyClause
    // DMP-CI-NEXT:   ACCCopyinClause
    // DMP-CO-NEXT:   ACCCopyoutClause
    // DMP-CR-NEXT:   ACCCreateClause
    // DMP-NEXT:        OMPArraySectionExpr
    // DMP-NEXT:          ImplicitCastExpr {{.*}} 'int *' <LValueToRValue>
    // DMP-NEXT:            DeclRefExpr {{.*}} 'pi' 'int *'
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:          <<<NULL>>>
    // DMP-NEXT:      ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'pi' 'int *'
    // DMP-NEXT:      impl: OMPTargetTeamsDirective
    // DMP-NEXT:        OMPNum_teamsClause
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:        OMPMapClause
    // DMP-NEXT:          OMPArraySectionExpr
    // DMP-NEXT:            ImplicitCastExpr {{.*}} 'int *' <LValueToRValue>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'pi' 'int *'
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:        OMPSharedClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'pi' 'int *'
    //
    //  PRT-A-AST-NEXT: #pragma acc parallel num_gangs(1) [[ACCC]](pi[0:3]){{$}}
    //  PRT-A-SRC-NEXT: #pragma acc parallel num_gangs(1) ACCC(pi[0:3]){{$}}
    //     PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map([[OMPMT]]: pi[0:3]) shared(pi){{$}}
    //      PRT-O-NEXT: #pragma omp target teams num_teams(1) map([[OMPMT]]: pi[0:3]) shared(pi){{$}}
    // PRT-OA-AST-NEXT: // #pragma acc parallel num_gangs(1) [[ACCC]](pi[0:3]){{$}}
    // PRT-OA-SRC-NEXT: // #pragma acc parallel num_gangs(1) ACCC(pi[0:3]){{$}}
    #pragma acc parallel num_gangs(1) ACCC(pi[0:3])
    for (int i = 0; i < 3; ++i)
      pi[i] = 99;

    //       EXE-C-NEXT: pi[0:3] after: <99, 99, 99, 11, 11, 11>
    //      EXE-CO-NEXT: pi[0:3] after: <99, 99, 99, 11, 11, 11>
    // EXE-HOST-CI-NEXT: pi[0:3] after: <99, 99, 99, 11, 11, 11>
    // EXE-HOST-CR-NEXT: pi[0:3] after: <99, 99, 99, 11, 11, 11>
    //  EXE-OFF-CI-NEXT: pi[0:3] after: <11, 11, 11, 11, 11, 11>
    //  EXE-OFF-CR-NEXT: pi[0:3] after: <11, 11, 11, 11, 11, 11>
    printArr1("pi[0:3] after", arr1, n);
  }

  //--------------------------------------------------
  // arr[1:3]
  //--------------------------------------------------

  {
    int n = 6;
    int arr1[n];
    for (int i = 0; i < n; ++i)
      arr1[i] = 11;

    // DMP-LABEL: "arr1[1:3] before"
    // PRT-LABEL: printArr1("arr1[1:3] before"
    // EXE-LABEL: arr1[1:3] before:
    // EXE-SAME:  <11, 11, 11, 11, 11, 11>
    printArr1("arr1[1:3] before", arr1, n);

    // DMP:         ACCParallelDirective
    // DMP-NEXT:      ACCNumGangsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 1
    // DMP-C-NEXT:    ACCCopyClause
    // DMP-CI-NEXT:   ACCCopyinClause
    // DMP-CO-NEXT:   ACCCopyoutClause
    // DMP-CR-NEXT:   ACCCreateClause
    // DMP-NEXT:        OMPArraySectionExpr
    // DMP-NEXT:          ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:            DeclRefExpr {{.*}} 'arr1' 'int[n]'
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:          <<<NULL>>>
    // DMP-NEXT:      ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'arr1' 'int[n]'
    // DMP-NEXT:      impl: OMPTargetTeamsDirective
    // DMP-NEXT:        OMPNum_teamsClause
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:        OMPMapClause
    // DMP-NEXT:          OMPArraySectionExpr
    // DMP-NEXT:            ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'arr1' 'int[n]'
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:        OMPSharedClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'arr1' 'int[n]'
    //
    //  PRT-A-AST-NEXT: #pragma acc parallel num_gangs(1) [[ACCC]](arr1[1:3]){{$}}
    //  PRT-A-SRC-NEXT: #pragma acc parallel num_gangs(1) ACCC(arr1[1:3]){{$}}
    //     PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map([[OMPMT]]: arr1[1:3]) shared(arr1){{$}}
    //      PRT-O-NEXT: #pragma omp target teams num_teams(1) map([[OMPMT]]: arr1[1:3]) shared(arr1){{$}}
    // PRT-OA-AST-NEXT: // #pragma acc parallel num_gangs(1) [[ACCC]](arr1[1:3]){{$}}
    // PRT-OA-SRC-NEXT: // #pragma acc parallel num_gangs(1) ACCC(arr1[1:3]){{$}}
    #pragma acc parallel num_gangs(1) ACCC(arr1[1:3])
    for (int i = 1; i < 4; ++i)
      arr1[i] = 99;

    //       EXE-C-NEXT: arr1[1:3] after: <11, 99, 99, 99, 11, 11>
    //      EXE-CO-NEXT: arr1[1:3] after: <11, 99, 99, 99, 11, 11>
    // EXE-HOST-CI-NEXT: arr1[1:3] after: <11, 99, 99, 99, 11, 11>
    // EXE-HOST-CR-NEXT: arr1[1:3] after: <11, 99, 99, 99, 11, 11>
    //  EXE-OFF-CI-NEXT: arr1[1:3] after: <11, 11, 11, 11, 11, 11>
    //  EXE-OFF-CR-NEXT: arr1[1:3] after: <11, 11, 11, 11, 11, 11>
    printArr1("arr1[1:3] after", arr1, n);
  }

  //--------------------------------------------------
  // arr1[start:length] with start=2 and length=3
  //--------------------------------------------------

  {
    int n = 6;
    int arr1[n];
    for (int i = 0; i < n; ++i)
      arr1[i] = 11;
    int start = 2, length = 3;

    // DMP-LABEL: "arr1[start:length] before"
    // PRT-LABEL: printArr1("arr1[start:length] before"
    // EXE-LABEL: arr1[start:length] before:
    // EXE-SAME:  <11, 11, 11, 11, 11, 11>
    printArr1("arr1[start:length] before", arr1, n);

    // DMP:         ACCParallelDirective
    // DMP-NEXT:      ACCNumGangsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 1
    // DMP-C-NEXT:    ACCCopyClause
    // DMP-CI-NEXT:   ACCCopyinClause
    // DMP-CO-NEXT:   ACCCopyoutClause
    // DMP-CR-NEXT:   ACCCreateClause
    // DMP-NEXT:        OMPArraySectionExpr
    // DMP-NEXT:          ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:            DeclRefExpr {{.*}} 'arr1' 'int[n]'
    // DMP-NEXT:          ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
    // DMP-NEXT:            DeclRefExpr {{.*}} 'start' 'int'
    // DMP-NEXT:          ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
    // DMP-NEXT:            DeclRefExpr {{.*}} 'length' 'int'
    // DMP-NEXT:          <<<NULL>>>
    // DMP-NEXT:      ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'arr1' 'int[n]'
    // DMP-NEXT:      impl: OMPTargetTeamsDirective
    // DMP-NEXT:        OMPNum_teamsClause
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:        OMPMapClause
    // DMP-NEXT:          OMPArraySectionExpr
    // DMP-NEXT:            ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'arr1' 'int[n]'
    // DMP-NEXT:            ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'start' 'int'
    // DMP-NEXT:            ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'length' 'int'
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:        OMPSharedClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'arr1' 'int[n]'
    //
    //  PRT-A-AST-NEXT: #pragma acc parallel num_gangs(1) [[ACCC]](arr1[start:length]){{$}}
    //  PRT-A-SRC-NEXT: #pragma acc parallel num_gangs(1) ACCC(arr1[start:length]){{$}}
    //     PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map([[OMPMT]]: arr1[start:length]) shared(arr1){{$}}
    //      PRT-O-NEXT: #pragma omp target teams num_teams(1) map([[OMPMT]]: arr1[start:length]) shared(arr1){{$}}
    // PRT-OA-AST-NEXT: // #pragma acc parallel num_gangs(1) [[ACCC]](arr1[start:length]){{$}}
    // PRT-OA-SRC-NEXT: // #pragma acc parallel num_gangs(1) ACCC(arr1[start:length]){{$}}
    #pragma acc parallel num_gangs(1) ACCC(arr1[start:length])
    for (int i = 2; i < 5; ++i)
      arr1[i] = 99;

    //       EXE-C-NEXT: arr1[start:length] after: <11, 11, 99, 99, 99, 11>
    //      EXE-CO-NEXT: arr1[start:length] after: <11, 11, 99, 99, 99, 11>
    // EXE-HOST-CI-NEXT: arr1[start:length] after: <11, 11, 99, 99, 99, 11>
    // EXE-HOST-CR-NEXT: arr1[start:length] after: <11, 11, 99, 99, 99, 11>
    //  EXE-OFF-CI-NEXT: arr1[start:length] after: <11, 11, 11, 11, 11, 11>
    //  EXE-OFF-CR-NEXT: arr1[start:length] after: <11, 11, 11, 11, 11, 11>
    printArr1("arr1[start:length] after", arr1, n);
  }

  //--------------------------------------------------
  // arr1[:length] with length=5
  //--------------------------------------------------

  {
    int n = 6;
    int arr1[n];
    for (int i = 0; i < n; ++i)
      arr1[i] = 11;
    int length = 5;

    // DMP-LABEL: "arr1[:length] before"
    // PRT-LABEL: printArr1("arr1[:length] before"
    // EXE-LABEL: arr1[:length] before:
    // EXE-SAME:  <11, 11, 11, 11, 11, 11>
    printArr1("arr1[:length] before", arr1, n);

    // DMP:         ACCParallelDirective
    // DMP-NEXT:      ACCNumGangsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 1
    // DMP-C-NEXT:    ACCCopyClause
    // DMP-CI-NEXT:   ACCCopyinClause
    // DMP-CO-NEXT:   ACCCopyoutClause
    // DMP-CR-NEXT:   ACCCreateClause
    // DMP-NEXT:        OMPArraySectionExpr
    // DMP-NEXT:          ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:            DeclRefExpr {{.*}} 'arr1' 'int[n]'
    // DMP-NEXT:          <<<NULL>>>
    // DMP-NEXT:          ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
    // DMP-NEXT:            DeclRefExpr {{.*}} 'length' 'int'
    // DMP-NEXT:          <<<NULL>>>
    // DMP-NEXT:      ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'arr1' 'int[n]'
    // DMP-NEXT:      impl: OMPTargetTeamsDirective
    // DMP-NEXT:        OMPNum_teamsClause
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:        OMPMapClause
    // DMP-NEXT:          OMPArraySectionExpr
    // DMP-NEXT:            ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'arr1' 'int[n]'
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:            ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'length' 'int'
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:        OMPSharedClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'arr1' 'int[n]'
    //
    //  PRT-A-AST-NEXT: #pragma acc parallel num_gangs(1) [[ACCC]](arr1[:length]){{$}}
    //  PRT-A-SRC-NEXT: #pragma acc parallel num_gangs(1) ACCC(arr1[:length]){{$}}
    //     PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map([[OMPMT]]: arr1[:length]) shared(arr1){{$}}
    //      PRT-O-NEXT: #pragma omp target teams num_teams(1) map([[OMPMT]]: arr1[:length]) shared(arr1){{$}}
    // PRT-OA-AST-NEXT: // #pragma acc parallel num_gangs(1) [[ACCC]](arr1[:length]){{$}}
    // PRT-OA-SRC-NEXT: // #pragma acc parallel num_gangs(1) ACCC(arr1[:length]){{$}}
    #pragma acc parallel num_gangs(1) ACCC(arr1[:length])
    for (int i = 0; i < 5; ++i)
      arr1[i] = 99;

    //       EXE-C-NEXT: arr1[:length] after: <99, 99, 99, 99, 99, 11>
    //      EXE-CO-NEXT: arr1[:length] after: <99, 99, 99, 99, 99, 11>
    // EXE-HOST-CI-NEXT: arr1[:length] after: <99, 99, 99, 99, 99, 11>
    // EXE-HOST-CR-NEXT: arr1[:length] after: <99, 99, 99, 99, 99, 11>
    //  EXE-OFF-CI-NEXT: arr1[:length] after: <11, 11, 11, 11, 11, 11>
    //  EXE-OFF-CR-NEXT: arr1[:length] after: <11, 11, 11, 11, 11, 11>
    printArr1("arr1[:length] after", arr1, n);
  }

  //--------------------------------------------------
  // arr1[start:] with start=3
  //--------------------------------------------------

  {
    int n = 6;
    int arr1[n];
    for (int i = 0; i < n; ++i)
      arr1[i] = 11;
    int start = 3;

    // DMP-LABEL: "arr1[start:] before"
    // PRT-LABEL: printArr1("arr1[start:] before"
    // EXE-LABEL: arr1[start:] before:
    // EXE-SAME:  <11, 11, 11, 11, 11, 11>
    printArr1("arr1[start:] before", arr1, n);

    // DMP:         ACCParallelDirective
    // DMP-NEXT:      ACCNumGangsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 1
    // DMP-C-NEXT:    ACCCopyClause
    // DMP-CI-NEXT:   ACCCopyinClause
    // DMP-CO-NEXT:   ACCCopyoutClause
    // DMP-CR-NEXT:   ACCCreateClause
    // DMP-NEXT:        OMPArraySectionExpr
    // DMP-NEXT:          ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:            DeclRefExpr {{.*}} 'arr1' 'int[n]'
    // DMP-NEXT:          ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
    // DMP-NEXT:            DeclRefExpr {{.*}} 'start' 'int'
    // DMP-NEXT:          <<<NULL>>>
    // DMP-NEXT:          <<<NULL>>>
    // DMP-NEXT:      ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'arr1' 'int[n]'
    // DMP-NEXT:      impl: OMPTargetTeamsDirective
    // DMP-NEXT:        OMPNum_teamsClause
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:        OMPMapClause
    // DMP-NEXT:          OMPArraySectionExpr
    // DMP-NEXT:            ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'arr1' 'int[n]'
    // DMP-NEXT:            ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'start' 'int'
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:        OMPSharedClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'arr1' 'int[n]'
    //
    //  PRT-A-AST-NEXT: #pragma acc parallel num_gangs(1) [[ACCC]](arr1[start:]){{$}}
    //  PRT-A-SRC-NEXT: #pragma acc parallel num_gangs(1) ACCC(arr1[start:]){{$}}
    //     PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map([[OMPMT]]: arr1[start:]) shared(arr1){{$}}
    //      PRT-O-NEXT: #pragma omp target teams num_teams(1) map([[OMPMT]]: arr1[start:]) shared(arr1){{$}}
    // PRT-OA-AST-NEXT: // #pragma acc parallel num_gangs(1) [[ACCC]](arr1[start:]){{$}}
    // PRT-OA-SRC-NEXT: // #pragma acc parallel num_gangs(1) ACCC(arr1[start:]){{$}}
    #pragma acc parallel num_gangs(1) ACCC(arr1[start:])
    for (int i = 3; i < 6; ++i)
      arr1[i] = 99;

    //       EXE-C-NEXT: arr1[start:] after: <11, 11, 11, 99, 99, 99>
    //      EXE-CO-NEXT: arr1[start:] after: <11, 11, 11, 99, 99, 99>
    // EXE-HOST-CI-NEXT: arr1[start:] after: <11, 11, 11, 99, 99, 99>
    // EXE-HOST-CR-NEXT: arr1[start:] after: <11, 11, 11, 99, 99, 99>
    //  EXE-OFF-CI-NEXT: arr1[start:] after: <11, 11, 11, 11, 11, 11>
    //  EXE-OFF-CR-NEXT: arr1[start:] after: <11, 11, 11, 11, 11, 11>
    printArr1("arr1[start:] after", arr1, n);
  }

  //--------------------------------------------------
  // arr1[:]
  //--------------------------------------------------

  {
    int n = 6;
    int arr1[n];
    int arr2[n][2];
    for (int i = 0; i < n; ++i)
      arr1[i] = 11;

    // DMP-LABEL: "arr1[:] before"
    // PRT-LABEL: printArr1("arr1[:] before"
    // EXE-LABEL: arr1[:] before:
    // EXE-SAME:  <11, 11, 11, 11, 11, 11>
    printArr1("arr1[:] before", arr1, n);

    // DMP:         ACCParallelDirective
    // DMP-NEXT:      ACCNumGangsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 1
    // DMP-C-NEXT:    ACCCopyClause
    // DMP-CI-NEXT:   ACCCopyinClause
    // DMP-CO-NEXT:   ACCCopyoutClause
    // DMP-CR-NEXT:   ACCCreateClause
    // DMP-NEXT:        OMPArraySectionExpr
    // DMP-NEXT:          ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:            DeclRefExpr {{.*}} 'arr1' 'int[n]'
    // DMP-NEXT:          <<<NULL>>>
    // DMP-NEXT:          <<<NULL>>>
    // DMP-NEXT:          <<<NULL>>>
    // DMP-NEXT:      ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'arr1' 'int[n]'
    // DMP-NEXT:      impl: OMPTargetTeamsDirective
    // DMP-NEXT:        OMPNum_teamsClause
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:        OMPMapClause
    // DMP-NEXT:          OMPArraySectionExpr
    // DMP-NEXT:            ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'arr1' 'int[n]'
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:        OMPSharedClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'arr1' 'int[n]'
    //
    //  PRT-A-AST-NEXT: #pragma acc parallel num_gangs(1) [[ACCC]](arr1[:]){{$}}
    //  PRT-A-SRC-NEXT: #pragma acc parallel num_gangs(1) ACCC(arr1[:]){{$}}
    //     PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map([[OMPMT]]: arr1[:]) shared(arr1){{$}}
    //      PRT-O-NEXT: #pragma omp target teams num_teams(1) map([[OMPMT]]: arr1[:]) shared(arr1){{$}}
    // PRT-OA-AST-NEXT: // #pragma acc parallel num_gangs(1) [[ACCC]](arr1[:]){{$}}
    // PRT-OA-SRC-NEXT: // #pragma acc parallel num_gangs(1) ACCC(arr1[:]){{$}}
    #pragma acc parallel num_gangs(1) ACCC(arr1[:])
    for (int i = 0; i < 6; ++i)
      arr1[i] = 99;

    //       EXE-C-NEXT: arr1[:] after: <99, 99, 99, 99, 99, 99>
    //      EXE-CO-NEXT: arr1[:] after: <99, 99, 99, 99, 99, 99>
    // EXE-HOST-CI-NEXT: arr1[:] after: <99, 99, 99, 99, 99, 99>
    // EXE-HOST-CR-NEXT: arr1[:] after: <99, 99, 99, 99, 99, 99>
    //  EXE-OFF-CI-NEXT: arr1[:] after: <11, 11, 11, 11, 11, 11>
    //  EXE-OFF-CR-NEXT: arr1[:] after: <11, 11, 11, 11, 11, 11>
    printArr1("arr1[:] after", arr1, n);
  }

  //--------------------------------------------------
  // arr2[2:3][0:2]
  //--------------------------------------------------

  {
    int n = 6;
    int arr2[n][2];
    for (int i = 0; i < n; ++i)
      for (int j = 0; j < 2; ++j)
        arr2[i][j] = 11;

    // DMP-LABEL: "arr2[2:3][0:2] before"
    // PRT-LABEL: printArr2("arr2[2:3][0:2] before"
    // EXE-LABEL: arr2[2:3][0:2] before:
    // EXE-SAME:  <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>>
    printArr2("arr2[2:3][0:2] before", arr2, n);

    // DMP:         ACCParallelDirective
    // DMP-NEXT:      ACCNumGangsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 1
    // DMP-C-NEXT:    ACCCopyClause
    // DMP-CI-NEXT:   ACCCopyinClause
    // DMP-CO-NEXT:   ACCCopyoutClause
    // DMP-CR-NEXT:   ACCCreateClause
    // DMP-NEXT:        OMPArraySectionExpr
    // DMP-NEXT:          OMPArraySectionExpr
    // DMP-NEXT:            ImplicitCastExpr {{.*}} 'int (*)[2]' <ArrayToPointerDecay>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'arr2' 'int[n][2]'
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:          <<<NULL>>>
    // DMP-NEXT:      ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'arr2' 'int[n][2]'
    // DMP-NEXT:      impl: OMPTargetTeamsDirective
    // DMP-NEXT:        OMPNum_teamsClause
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:        OMPMapClause
    // DMP-NEXT:          OMPArraySectionExpr
    // DMP-NEXT:            OMPArraySectionExpr
    // DMP-NEXT:              ImplicitCastExpr {{.*}} 'int (*)[2]' <ArrayToPointerDecay>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'arr2' 'int[n][2]'
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:              <<<NULL>>>
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:        OMPSharedClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'arr2' 'int[n][2]'
    //
    //  PRT-A-AST-NEXT: #pragma acc parallel num_gangs(1) [[ACCC]](arr2[2:3][0:2]){{$}}
    //  PRT-A-SRC-NEXT: #pragma acc parallel num_gangs(1) ACCC(arr2[2:3][0:2]){{$}}
    //     PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map([[OMPMT]]: arr2[2:3][0:2]) shared(arr2){{$}}
    //      PRT-O-NEXT: #pragma omp target teams num_teams(1) map([[OMPMT]]: arr2[2:3][0:2]) shared(arr2){{$}}
    // PRT-OA-AST-NEXT: // #pragma acc parallel num_gangs(1) [[ACCC]](arr2[2:3][0:2]){{$}}
    // PRT-OA-SRC-NEXT: // #pragma acc parallel num_gangs(1) ACCC(arr2[2:3][0:2]){{$}}
    #pragma acc parallel num_gangs(1) ACCC(arr2[2:3][0:2])
    for (int i = 2; i < 5; ++i)
      for (int j = 0; j < 2; ++j)
        arr2[i][j] = 99;

    //       EXE-C-NEXT: arr2[2:3][0:2] after: <<11, 11>, <11, 11>, <99, 99>, <99, 99>, <99, 99>, <11, 11>>
    //      EXE-CO-NEXT: arr2[2:3][0:2] after: <<11, 11>, <11, 11>, <99, 99>, <99, 99>, <99, 99>, <11, 11>>
    // EXE-HOST-CI-NEXT: arr2[2:3][0:2] after: <<11, 11>, <11, 11>, <99, 99>, <99, 99>, <99, 99>, <11, 11>>
    // EXE-HOST-CR-NEXT: arr2[2:3][0:2] after: <<11, 11>, <11, 11>, <99, 99>, <99, 99>, <99, 99>, <11, 11>>
    //  EXE-OFF-CI-NEXT: arr2[2:3][0:2] after: <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>>
    //  EXE-OFF-CR-NEXT: arr2[2:3][0:2] after: <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>>
    printArr2("arr2[2:3][0:2] after", arr2, n);
  }

  //--------------------------------------------------
  // arr2[3:1][1:1]
  //--------------------------------------------------

  {
    int n = 6;
    int arr2[n][2];
    for (int i = 0; i < n; ++i)
      for (int j = 0; j < 2; ++j)
        arr2[i][j] = 11;

    // DMP-LABEL: "arr2[3:1][1:1] before"
    // PRT-LABEL: printArr2("arr2[3:1][1:1] before"
    // EXE-LABEL: arr2[3:1][1:1] before:
    // EXE-SAME:  <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>>
    printArr2("arr2[3:1][1:1] before", arr2, n);

    // DMP:         ACCParallelDirective
    // DMP-NEXT:      ACCNumGangsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 1
    // DMP-C-NEXT:    ACCCopyClause
    // DMP-CI-NEXT:   ACCCopyinClause
    // DMP-CO-NEXT:   ACCCopyoutClause
    // DMP-CR-NEXT:   ACCCreateClause
    // DMP-NEXT:        OMPArraySectionExpr
    // DMP-NEXT:          OMPArraySectionExpr
    // DMP-NEXT:            ImplicitCastExpr {{.*}} 'int (*)[2]' <ArrayToPointerDecay>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'arr2' 'int[n][2]'
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:          <<<NULL>>>
    // DMP-NEXT:      ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'arr2' 'int[n][2]'
    // DMP-NEXT:      impl: OMPTargetTeamsDirective
    // DMP-NEXT:        OMPNum_teamsClause
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:        OMPMapClause
    // DMP-NEXT:          OMPArraySectionExpr
    // DMP-NEXT:            OMPArraySectionExpr
    // DMP-NEXT:              ImplicitCastExpr {{.*}} 'int (*)[2]' <ArrayToPointerDecay>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'arr2' 'int[n][2]'
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:              <<<NULL>>>
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:        OMPSharedClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'arr2' 'int[n][2]'
    //
    //  PRT-A-AST-NEXT: #pragma acc parallel num_gangs(1) [[ACCC]](arr2[3:1][1:1]){{$}}
    //  PRT-A-SRC-NEXT: #pragma acc parallel num_gangs(1) ACCC(arr2[3:1][1:1]){{$}}
    //     PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map([[OMPMT]]: arr2[3:1][1:1]) shared(arr2){{$}}
    //      PRT-O-NEXT: #pragma omp target teams num_teams(1) map([[OMPMT]]: arr2[3:1][1:1]) shared(arr2){{$}}
    // PRT-OA-AST-NEXT: // #pragma acc parallel num_gangs(1) [[ACCC]](arr2[3:1][1:1]){{$}}
    // PRT-OA-SRC-NEXT: // #pragma acc parallel num_gangs(1) ACCC(arr2[3:1][1:1]){{$}}
    #pragma acc parallel num_gangs(1) ACCC(arr2[3:1][1:1])
    for (int i = 3; i < 4; ++i)
      for (int j = 1; j < 2; ++j)
        arr2[i][j] = 99;

    //       EXE-C-NEXT: arr2[2:3][0:2] after: <<11, 11>, <11, 11>, <11, 11>, <11, 99>, <11, 11>, <11, 11>>
    //      EXE-CO-NEXT: arr2[2:3][0:2] after: <<11, 11>, <11, 11>, <11, 11>, <11, 99>, <11, 11>, <11, 11>>
    // EXE-HOST-CI-NEXT: arr2[2:3][0:2] after: <<11, 11>, <11, 11>, <11, 11>, <11, 99>, <11, 11>, <11, 11>>
    // EXE-HOST-CR-NEXT: arr2[2:3][0:2] after: <<11, 11>, <11, 11>, <11, 11>, <11, 99>, <11, 11>, <11, 11>>
    //  EXE-OFF-CI-NEXT: arr2[2:3][0:2] after: <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>>
    //  EXE-OFF-CR-NEXT: arr2[2:3][0:2] after: <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>>
    printArr2("arr2[2:3][0:2] after", arr2, n);
  }

  //--------------------------------------------------
  // pa[1:3][:]
  //--------------------------------------------------

  {
    int n = 6;
    int arr2[n][2];
    for (int i = 0; i < n; ++i)
      for (int j = 0; j < 2; ++j)
        arr2[i][j] = 11;
    int (*pa)[2] = arr2;

    // DMP-LABEL: "pa[1:3][:] before"
    // PRT-LABEL: printArr2("pa[1:3][:] before"
    // EXE-LABEL: pa[1:3][:] before:
    // EXE-SAME:  <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>>
    printArr2("pa[1:3][:] before", arr2, n);

    // DMP:         ACCParallelDirective
    // DMP-NEXT:      ACCNumGangsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 1
    // DMP-C-NEXT:    ACCCopyClause
    // DMP-CI-NEXT:   ACCCopyinClause
    // DMP-CO-NEXT:   ACCCopyoutClause
    // DMP-CR-NEXT:   ACCCreateClause
    // DMP-NEXT:        OMPArraySectionExpr
    // DMP-NEXT:          OMPArraySectionExpr
    // DMP-NEXT:            ImplicitCastExpr {{.*}} 'int (*)[2]' <LValueToRValue>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'pa' 'int (*)[2]'
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:          <<<NULL>>>
    // DMP-NEXT:          <<<NULL>>>
    // DMP-NEXT:          <<<NULL>>>
    // DMP-NEXT:      ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'pa' 'int (*)[2]'
    // DMP-NEXT:      impl: OMPTargetTeamsDirective
    // DMP-NEXT:        OMPNum_teamsClause
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:        OMPMapClause
    // DMP-NEXT:          OMPArraySectionExpr
    // DMP-NEXT:            OMPArraySectionExpr
    // DMP-NEXT:              ImplicitCastExpr {{.*}} 'int (*)[2]' <LValueToRValue>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'pa' 'int (*)[2]'
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:              <<<NULL>>>
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:        OMPSharedClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'pa' 'int (*)[2]'
    //
    //  PRT-A-AST-NEXT: #pragma acc parallel num_gangs(1) [[ACCC]](pa[1:3][:]){{$}}
    //  PRT-A-SRC-NEXT: #pragma acc parallel num_gangs(1) ACCC(pa[1:3][:]){{$}}
    //     PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map([[OMPMT]]: pa[1:3][:]) shared(pa){{$}}
    //      PRT-O-NEXT: #pragma omp target teams num_teams(1) map([[OMPMT]]: pa[1:3][:]) shared(pa){{$}}
    // PRT-OA-AST-NEXT: // #pragma acc parallel num_gangs(1) [[ACCC]](pa[1:3][:]){{$}}
    // PRT-OA-SRC-NEXT: // #pragma acc parallel num_gangs(1) ACCC(pa[1:3][:]){{$}}
    #pragma acc parallel num_gangs(1) ACCC(pa[1:3][:])
    for (int i = 1; i < 4; ++i)
      for (int j = 0; j < 2; ++j)
        pa[i][j] = 99;

    //       EXE-C-NEXT: pa[1:3][:] after: <<11, 11>, <99, 99>, <99, 99>, <99, 99>, <11, 11>, <11, 11>>
    //      EXE-CO-NEXT: pa[1:3][:] after: <<11, 11>, <99, 99>, <99, 99>, <99, 99>, <11, 11>, <11, 11>>
    // EXE-HOST-CI-NEXT: pa[1:3][:] after: <<11, 11>, <99, 99>, <99, 99>, <99, 99>, <11, 11>, <11, 11>>
    // EXE-HOST-CR-NEXT: pa[1:3][:] after: <<11, 11>, <99, 99>, <99, 99>, <99, 99>, <11, 11>, <11, 11>>
    //  EXE-OFF-CI-NEXT: pa[1:3][:] after: <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>>
    //  EXE-OFF-CR-NEXT: pa[1:3][:] after: <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>>
    printArr2("pa[1:3][:] after", arr2, n);
  }

  //--------------------------------------------------
  // ap[2:1][0:2]
  //
  // Must be a single row or it's non-contiguous, which OpenMP doesn't permit.
  //--------------------------------------------------

  {
    int arr2[6][2];
    for (int i = 0; i < 6; ++i)
      for (int j = 0; j < 2; ++j)
        arr2[i][j] = 11;
    int *ap[6];
    for (int i = 0; i < 6; ++i)
      ap[i] = arr2[i];

    // DMP-LABEL: "ap[2:1][0:2] before"
    // PRT-LABEL: printArr2("ap[2:1][0:2] before"
    // EXE-LABEL: ap[2:1][0:2] before:
    // EXE-SAME:  <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>>
    printArr2("ap[2:1][0:2] before", arr2, 6);

    // DMP:         ACCParallelDirective
    // DMP-NEXT:      ACCNumGangsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 1
    // DMP-C-NEXT:    ACCCopyClause
    // DMP-CI-NEXT:   ACCCopyinClause
    // DMP-CO-NEXT:   ACCCopyoutClause
    // DMP-CR-NEXT:   ACCCreateClause
    // DMP-NEXT:        OMPArraySectionExpr
    // DMP-NEXT:          OMPArraySectionExpr
    // DMP-NEXT:            ImplicitCastExpr {{.*}} 'int **' <ArrayToPointerDecay>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'ap' 'int *[6]'
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:          <<<NULL>>>
    // DMP-NEXT:      ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'ap' 'int *[6]'
    // DMP-NEXT:      impl: OMPTargetTeamsDirective
    // DMP-NEXT:        OMPNum_teamsClause
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:        OMPMapClause
    // DMP-NEXT:          OMPArraySectionExpr
    // DMP-NEXT:            OMPArraySectionExpr
    // DMP-NEXT:              ImplicitCastExpr {{.*}} 'int **' <ArrayToPointerDecay>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'ap' 'int *[6]'
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:              <<<NULL>>>
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:        OMPSharedClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'ap' 'int *[6]'
    //
    //  PRT-A-AST-NEXT: #pragma acc parallel num_gangs(1) [[ACCC]](ap[2:1][0:2]){{$}}
    //  PRT-A-SRC-NEXT: #pragma acc parallel num_gangs(1) ACCC(ap[2:1][0:2]){{$}}
    //     PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map([[OMPMT]]: ap[2:1][0:2]) shared(ap){{$}}
    //      PRT-O-NEXT: #pragma omp target teams num_teams(1) map([[OMPMT]]: ap[2:1][0:2]) shared(ap){{$}}
    // PRT-OA-AST-NEXT: // #pragma acc parallel num_gangs(1) [[ACCC]](ap[2:1][0:2]){{$}}
    // PRT-OA-SRC-NEXT: // #pragma acc parallel num_gangs(1) ACCC(ap[2:1][0:2]){{$}}
    #pragma acc parallel num_gangs(1) ACCC(ap[2:1][0:2])
    for (int i = 2; i < 3; ++i)
      for (int j = 0; j < 2; ++j)
        ap[i][j] = 99;

    //       EXE-C-NEXT: ap[2:1][0:2] after: <<11, 11>, <11, 11>, <99, 99>, <11, 11>, <11, 11>, <11, 11>>
    //      EXE-CO-NEXT: ap[2:1][0:2] after: <<11, 11>, <11, 11>, <99, 99>, <11, 11>, <11, 11>, <11, 11>>
    // EXE-HOST-CI-NEXT: ap[2:1][0:2] after: <<11, 11>, <11, 11>, <99, 99>, <11, 11>, <11, 11>, <11, 11>>
    // EXE-HOST-CR-NEXT: ap[2:1][0:2] after: <<11, 11>, <11, 11>, <99, 99>, <11, 11>, <11, 11>, <11, 11>>
    //  EXE-OFF-CI-NEXT: ap[2:1][0:2] after: <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>>
    //  EXE-OFF-CR-NEXT: ap[2:1][0:2] after: <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>>
    printArr2("ap[2:1][0:2] after", arr2, 6);
  }

  //--------------------------------------------------
  // ap[5:1][0:1]
  //
  // Must be a single row or it's non-contiguous, which OpenMP doesn't permit.
  //--------------------------------------------------

  {
    int arr2[6][2];
    for (int i = 0; i < 6; ++i)
      for (int j = 0; j < 2; ++j)
        arr2[i][j] = 11;
    int *ap[6];
    for (int i = 0; i < 6; ++i)
      ap[i] = arr2[i];

    // DMP-LABEL: "ap[5:1][0:1] before"
    // PRT-LABEL: printArr2("ap[5:1][0:1] before"
    // EXE-LABEL: ap[5:1][0:1] before:
    // EXE-SAME:  <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>>
    printArr2("ap[5:1][0:1] before", arr2, 6);

    // DMP:         ACCParallelDirective
    // DMP-NEXT:      ACCNumGangsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 1
    // DMP-C-NEXT:    ACCCopyClause
    // DMP-CI-NEXT:   ACCCopyinClause
    // DMP-CO-NEXT:   ACCCopyoutClause
    // DMP-CR-NEXT:   ACCCreateClause
    // DMP-NEXT:        OMPArraySectionExpr
    // DMP-NEXT:          OMPArraySectionExpr
    // DMP-NEXT:            ImplicitCastExpr {{.*}} 'int **' <ArrayToPointerDecay>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'ap' 'int *[6]'
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 5
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:          <<<NULL>>>
    // DMP-NEXT:      ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'ap' 'int *[6]'
    // DMP-NEXT:      impl: OMPTargetTeamsDirective
    // DMP-NEXT:        OMPNum_teamsClause
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:        OMPMapClause
    // DMP-NEXT:          OMPArraySectionExpr
    // DMP-NEXT:            OMPArraySectionExpr
    // DMP-NEXT:              ImplicitCastExpr {{.*}} 'int **' <ArrayToPointerDecay>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'ap' 'int *[6]'
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 5
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:              <<<NULL>>>
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:        OMPSharedClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'ap' 'int *[6]'
    //
    //  PRT-A-AST-NEXT: #pragma acc parallel num_gangs(1) [[ACCC]](ap[5:1][0:1]){{$}}
    //  PRT-A-SRC-NEXT: #pragma acc parallel num_gangs(1) ACCC(ap[5:1][0:1]){{$}}
    //     PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map([[OMPMT]]: ap[5:1][0:1]) shared(ap){{$}}
    //      PRT-O-NEXT: #pragma omp target teams num_teams(1) map([[OMPMT]]: ap[5:1][0:1]) shared(ap){{$}}
    // PRT-OA-AST-NEXT: // #pragma acc parallel num_gangs(1) [[ACCC]](ap[5:1][0:1]){{$}}
    // PRT-OA-SRC-NEXT: // #pragma acc parallel num_gangs(1) ACCC(ap[5:1][0:1]){{$}}
    #pragma acc parallel num_gangs(1) ACCC(ap[5:1][0:1])
    for (int i = 5; i < 6; ++i)
      for (int j = 0; j < 1; ++j)
        ap[i][j] = 99;

    //       EXE-C-NEXT: ap[5:1][0:1] after: <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>, <99, 11>>
    //      EXE-CO-NEXT: ap[5:1][0:1] after: <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>, <99, 11>>
    // EXE-HOST-CI-NEXT: ap[5:1][0:1] after: <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>, <99, 11>>
    // EXE-HOST-CR-NEXT: ap[5:1][0:1] after: <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>, <99, 11>>
    //  EXE-OFF-CI-NEXT: ap[5:1][0:1] after: <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>>
    //  EXE-OFF-CR-NEXT: ap[5:1][0:1] after: <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>>
    printArr2("ap[5:1][0:1] after", arr2, 6);
  }

  //--------------------------------------------------
  // pp[3:1][0:2]
  //
  // Must be a single row or it's non-contiguous, which OpenMP doesn't permit.
  //--------------------------------------------------

  {
    int arr2[6][2];
    for (int i = 0; i < 6; ++i)
      for (int j = 0; j < 2; ++j)
        arr2[i][j] = 11;
    int *ap[6];
    for (int i = 0; i < 6; ++i)
      ap[i] = arr2[i];
    int **pp = ap;

    // DMP-LABEL: "pp[3:1][0:2] before"
    // PRT-LABEL: printArr2("pp[3:1][0:2] before"
    // EXE-LABEL: pp[3:1][0:2] before:
    // EXE-SAME:  <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>>
    printArr2("pp[3:1][0:2] before", arr2, 6);

    // DMP:         ACCParallelDirective
    // DMP-NEXT:      ACCNumGangsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 1
    // DMP-C-NEXT:    ACCCopyClause
    // DMP-CI-NEXT:   ACCCopyinClause
    // DMP-CO-NEXT:   ACCCopyoutClause
    // DMP-CR-NEXT:   ACCCreateClause
    // DMP-NEXT:        OMPArraySectionExpr
    // DMP-NEXT:          OMPArraySectionExpr
    // DMP-NEXT:            ImplicitCastExpr {{.*}} 'int **' <LValueToRValue>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'pp' 'int **'
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:          <<<NULL>>>
    // DMP-NEXT:      ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'pp' 'int **'
    // DMP-NEXT:      impl: OMPTargetTeamsDirective
    // DMP-NEXT:        OMPNum_teamsClause
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:        OMPMapClause
    // DMP-NEXT:          OMPArraySectionExpr
    // DMP-NEXT:            OMPArraySectionExpr
    // DMP-NEXT:              ImplicitCastExpr {{.*}} 'int **' <LValueToRValue>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'pp' 'int **'
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 3
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:              <<<NULL>>>
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 2
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:        OMPSharedClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'pp' 'int **'
    //
    //  PRT-A-AST-NEXT: #pragma acc parallel num_gangs(1) [[ACCC]](pp[3:1][0:2]){{$}}
    //  PRT-A-SRC-NEXT: #pragma acc parallel num_gangs(1) ACCC(pp[3:1][0:2]){{$}}
    //     PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map([[OMPMT]]: pp[3:1][0:2]) shared(pp){{$}}
    //      PRT-O-NEXT: #pragma omp target teams num_teams(1) map([[OMPMT]]: pp[3:1][0:2]) shared(pp){{$}}
    // PRT-OA-AST-NEXT: // #pragma acc parallel num_gangs(1) [[ACCC]](pp[3:1][0:2]){{$}}
    // PRT-OA-SRC-NEXT: // #pragma acc parallel num_gangs(1) ACCC(pp[3:1][0:2]){{$}}
    #pragma acc parallel num_gangs(1) ACCC(pp[3:1][0:2])
    for (int i = 3; i < 4; ++i)
      for (int j = 0; j < 2; ++j)
        pp[i][j] = 99;

    //       EXE-C-NEXT: pp[3:1][0:2] after: <<11, 11>, <11, 11>, <11, 11>, <99, 99>, <11, 11>, <11, 11>>
    //      EXE-CO-NEXT: pp[3:1][0:2] after: <<11, 11>, <11, 11>, <11, 11>, <99, 99>, <11, 11>, <11, 11>>
    // EXE-HOST-CI-NEXT: pp[3:1][0:2] after: <<11, 11>, <11, 11>, <11, 11>, <99, 99>, <11, 11>, <11, 11>>
    // EXE-HOST-CR-NEXT: pp[3:1][0:2] after: <<11, 11>, <11, 11>, <11, 11>, <99, 99>, <11, 11>, <11, 11>>
    //  EXE-OFF-CI-NEXT: pp[3:1][0:2] after: <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>>
    //  EXE-OFF-CR-NEXT: pp[3:1][0:2] after: <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>>
    printArr2("pp[3:1][0:2] after", arr2, 6);
  }

  //--------------------------------------------------
  // pp[4:1][1:1]
  //
  // Must be a single row or it's non-contiguous, which OpenMP doesn't permit.
  //--------------------------------------------------

  {
    int arr2[6][2];
    for (int i = 0; i < 6; ++i)
      for (int j = 0; j < 2; ++j)
        arr2[i][j] = 11;
    int *ap[6];
    for (int i = 0; i < 6; ++i)
      ap[i] = arr2[i];
    int **pp = ap;

    // DMP-LABEL: "pp[4:1][1:1] before"
    // PRT-LABEL: printArr2("pp[4:1][1:1] before"
    // EXE-LABEL: pp[4:1][1:1] before:
    // EXE-SAME:  <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>>
    printArr2("pp[4:1][1:1] before", arr2, 6);

    // DMP:         ACCParallelDirective
    // DMP-NEXT:      ACCNumGangsClause
    // DMP-NEXT:        IntegerLiteral {{.*}} 'int' 1
    // DMP-C-NEXT:    ACCCopyClause
    // DMP-CI-NEXT:   ACCCopyinClause
    // DMP-CO-NEXT:   ACCCopyoutClause
    // DMP-CR-NEXT:   ACCCreateClause
    // DMP-NEXT:        OMPArraySectionExpr
    // DMP-NEXT:          OMPArraySectionExpr
    // DMP-NEXT:            ImplicitCastExpr {{.*}} 'int **' <LValueToRValue>
    // DMP-NEXT:              DeclRefExpr {{.*}} 'pp' 'int **'
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:          <<<NULL>>>
    // DMP-NEXT:      ACCSharedClause {{.*}} <implicit>
    // DMP-NEXT:        DeclRefExpr {{.*}} 'pp' 'int **'
    // DMP-NEXT:      impl: OMPTargetTeamsDirective
    // DMP-NEXT:        OMPNum_teamsClause
    // DMP-NEXT:          IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:        OMPMapClause
    // DMP-NEXT:          OMPArraySectionExpr
    // DMP-NEXT:            OMPArraySectionExpr
    // DMP-NEXT:              ImplicitCastExpr {{.*}} 'int **' <LValueToRValue>
    // DMP-NEXT:                DeclRefExpr {{.*}} 'pp' 'int **'
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 4
    // DMP-NEXT:              IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:              <<<NULL>>>
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:            IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:            <<<NULL>>>
    // DMP-NEXT:        OMPSharedClause
    // DMP-NEXT:          DeclRefExpr {{.*}} 'pp' 'int **'
    //
    //  PRT-A-AST-NEXT: #pragma acc parallel num_gangs(1) [[ACCC]](pp[4:1][1:1]){{$}}
    //  PRT-A-SRC-NEXT: #pragma acc parallel num_gangs(1) ACCC(pp[4:1][1:1]){{$}}
    //     PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map([[OMPMT]]: pp[4:1][1:1]) shared(pp){{$}}
    //      PRT-O-NEXT: #pragma omp target teams num_teams(1) map([[OMPMT]]: pp[4:1][1:1]) shared(pp){{$}}
    // PRT-OA-AST-NEXT: // #pragma acc parallel num_gangs(1) [[ACCC]](pp[4:1][1:1]){{$}}
    // PRT-OA-SRC-NEXT: // #pragma acc parallel num_gangs(1) ACCC(pp[4:1][1:1]){{$}}
    #pragma acc parallel num_gangs(1) ACCC(pp[4:1][1:1])
    for (int i = 4; i < 5; ++i)
      for (int j = 1; j < 2; ++j)
        pp[i][j] = 99;

    //       EXE-C-NEXT: pp[4:1][1:1] after: <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 99>, <11, 11>>
    //      EXE-CO-NEXT: pp[4:1][1:1] after: <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 99>, <11, 11>>
    // EXE-HOST-CI-NEXT: pp[4:1][1:1] after: <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 99>, <11, 11>>
    // EXE-HOST-CR-NEXT: pp[4:1][1:1] after: <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 99>, <11, 11>>
    //  EXE-OFF-CI-NEXT: pp[4:1][1:1] after: <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>>
    //  EXE-OFF-CR-NEXT: pp[4:1][1:1] after: <<11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>, <11, 11>>
    printArr2("pp[4:1][1:1] after", arr2, 6);
  }

  // EXE-NOT: {{.}}
  return 0;
}
