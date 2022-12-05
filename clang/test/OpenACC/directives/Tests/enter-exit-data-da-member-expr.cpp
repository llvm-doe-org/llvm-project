// Check data clauses for member expressions and for 'this' on "acc enter data"
// and "acc exit data", but only check cases that are not covered by
// enter-exit-data-da-member-expr.c because they are specific to C++.

// REDEFINE: %{dmp:fc:args} = -strict-whitespace
// REDEFINE: %{exe:fc:args} = -strict-whitespace -match-full-lines \
// REDEFINE:                  -implicit-check-not='{{.}}'
// RUN: %{acc-check-dmp-cxx}
// RUN: %{acc-check-prt-cxx}
// RUN: %{acc-check-exe-cxx}

// END.

/* expected-no-diagnostics */

#include <openacc.h>
#include <stdio.h>

#define printInt(Var) printInt_(#Var, &Var)
void printInt_(const char *Name, int *Var) {
  printf("%s: %d -> ", Name, *Var);
  if (!acc_is_present(Var, sizeof(*Var)))
    printf("absent");
  else {
    int TgtVal;
    #pragma acc parallel num_gangs(1) copyout(TgtVal)
    TgtVal = *Var;
    printf("%d", TgtVal);
  }
  printf("\n");
}

class Test {
public:
  int firstDataMember;
  int i;
  int lastDataMember;
  void run() {
    //--------------------------------------------------------------------------
    // Check that explicit data clause for this[0:1] behaves.
    //--------------------------------------------------------------------------

    // DMP-LABEL:"explicit DA for this\n"
    // PRT-LABEL:"explicit DA for this\n"
    // EXE-LABEL:explicit DA for this
    printf("explicit DA for this\n");

    firstDataMember = 10;
    i = 20;
    lastDataMember = 30;

    //      DMP: ACCEnterDataDirective
    // DMP-NEXT:   ACCCopyinClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:   impl: OMPTargetEnterDataDirective
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       OMPArraySectionExpr
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:         <<<NULL>>>
    //
    //       PRT-A: {{^ *}}#pragma acc enter data copyin(this[0:1]){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target enter data map(to: this[0:1]){{$}}
    //       PRT-O: {{^ *}}#pragma omp target enter data map(to: this[0:1]){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc enter data copyin(this[0:1]){{$}}
    //
    // EXE-NEXT:firstDataMember: 10 -> 10
    // EXE-NEXT:i: 20 -> 20
    // EXE-NEXT:lastDataMember: 30 -> 30
    #pragma acc enter data copyin(this[0:1])
    printInt(firstDataMember);
    printInt(i);
    printInt(lastDataMember);

    #pragma acc parallel num_gangs(1)
    {
      firstDataMember += 100;
      i += 200;
      lastDataMember += 300;
    }

    //      DMP: ACCExitDataDirective
    // DMP-NEXT:   ACCCopyoutClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:   impl: OMPTargetExitDataDirective
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       OMPArraySectionExpr
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:         <<<NULL>>>
    //
    //       PRT-A: {{^ *}}#pragma acc exit data copyout(this[0:1]){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target exit data map(from: this[0:1]){{$}}
    //       PRT-O: {{^ *}}#pragma omp target exit data map(from: this[0:1]){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc exit data copyout(this[0:1]){{$}}
    //
    // EXE-HOST-NEXT:firstDataMember: 110 -> 110
    // EXE-HOST-NEXT:i: 220 -> 220
    // EXE-HOST-NEXT:lastDataMember: 330 -> 330
    //  EXE-OFF-NEXT:firstDataMember: 110 -> absent
    //  EXE-OFF-NEXT:i: 220 -> absent
    //  EXE-OFF-NEXT:lastDataMember: 330 -> absent
    #pragma acc exit data copyout(this[0:1])
    printInt(firstDataMember);
    printInt(i);
    printInt(lastDataMember);

    //--------------------------------------------------------------------------
    // Check explicit data clause for a data member of 'this'.
    //--------------------------------------------------------------------------

    // DMP-LABEL:"explicit DAs for data member on explicit this\n"
    // PRT-LABEL:"explicit DAs for data member on explicit this\n"
    // EXE-LABEL:explicit DAs for data member on explicit this
    printf("explicit DAs for data member on explicit this\n");

    firstDataMember = 10;
    i = 20;
    lastDataMember = 30;

    //      DMP: ACCEnterDataDirective
    // DMP-NEXT:   ACCCopyinClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     MemberExpr {{.* ->i }}
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:   impl: OMPTargetEnterDataDirective
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       MemberExpr {{.* ->i }}
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' this
    //
    //       PRT-A: {{^ *}}#pragma acc enter data copyin(this->i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target enter data map(to: this->i){{$}}
    //       PRT-O: {{^ *}}#pragma omp target enter data map(to: this->i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc enter data copyin(this->i){{$}}
    //
    // EXE-HOST-NEXT:firstDataMember: 10 -> 10
    // EXE-HOST-NEXT:i: 20 -> 20
    // EXE-HOST-NEXT:lastDataMember: 30 -> 30
    //  EXE-OFF-NEXT:firstDataMember: 10 -> absent
    //  EXE-OFF-NEXT:i: 20 -> 20
    //  EXE-OFF-NEXT:lastDataMember: 30 -> absent
    #pragma acc enter data copyin(this->i)
    printInt(firstDataMember);
    printInt(i);
    printInt(lastDataMember);

    #pragma acc parallel num_gangs(1) copy(this->i)
    i += 200;

    //      DMP: ACCExitDataDirective
    // DMP-NEXT:   ACCCopyoutClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     MemberExpr {{.* ->i }}
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:   impl: OMPTargetExitDataDirective
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       MemberExpr {{.* ->i }}
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' this
    //
    //       PRT-A: {{^ *}}#pragma acc exit data copyout(this->i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target exit data map(from: this->i){{$}}
    //       PRT-O: {{^ *}}#pragma omp target exit data map(from: this->i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc exit data copyout(this->i){{$}}
    //
    // EXE-HOST-NEXT:firstDataMember: 10 -> 10
    // EXE-HOST-NEXT:i: 220 -> 220
    // EXE-HOST-NEXT:lastDataMember: 30 -> 30
    //  EXE-OFF-NEXT:firstDataMember: 10 -> absent
    //  EXE-OFF-NEXT:i: 220 -> absent
    //  EXE-OFF-NEXT:lastDataMember: 30 -> absent
    #pragma acc exit data copyout(this->i)
    printInt(firstDataMember);
    printInt(i);
    printInt(lastDataMember);

    // DMP-LABEL:"explicit DAs for data member on implicit this\n"
    // PRT-LABEL:"explicit DAs for data member on implicit this\n"
    // EXE-LABEL:explicit DAs for data member on implicit this
    printf("explicit DAs for data member on implicit this\n");

    firstDataMember = 10;
    i = 20;
    lastDataMember = 30;

    //      DMP: ACCEnterDataDirective
    // DMP-NEXT:   ACCCopyinClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     MemberExpr {{.* ->i }}
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this
    // DMP-NEXT:   impl: OMPTargetEnterDataDirective
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       MemberExpr {{.* ->i }}
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' implicit this
    //
    //       PRT-A-AST: {{^ *}}#pragma acc enter data copyin(this->i){{$}}
    //       PRT-A-SRC: {{^ *}}#pragma acc enter data copyin(i){{$}}
    //     PRT-AO-NEXT: {{^ *}}// #pragma omp target enter data map(to: this->i){{$}}
    //           PRT-O: {{^ *}}#pragma omp target enter data map(to: this->i){{$}}
    // PRT-OA-AST-NEXT: {{^ *}}// #pragma acc enter data copyin(this->i){{$}}
    // PRT-OA-SRC-NEXT: {{^ *}}// #pragma acc enter data copyin(i){{$}}
    //
    // EXE-HOST-NEXT:firstDataMember: 10 -> 10
    // EXE-HOST-NEXT:i: 20 -> 20
    // EXE-HOST-NEXT:lastDataMember: 30 -> 30
    //  EXE-OFF-NEXT:firstDataMember: 10 -> absent
    //  EXE-OFF-NEXT:i: 20 -> 20
    //  EXE-OFF-NEXT:lastDataMember: 30 -> absent
    #pragma acc enter data copyin(i)
    printInt(firstDataMember);
    printInt(i);
    printInt(lastDataMember);

    #pragma acc parallel num_gangs(1) copy(this->i)
    i += 200;

    //      DMP: ACCExitDataDirective
    // DMP-NEXT:   ACCCopyoutClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     MemberExpr {{.* ->i }}
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this
    // DMP-NEXT:   impl: OMPTargetExitDataDirective
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       MemberExpr {{.* ->i }}
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' implicit this
    //
    //       PRT-A-AST: {{^ *}}#pragma acc exit data copyout(this->i){{$}}
    //       PRT-A-SRC: {{^ *}}#pragma acc exit data copyout(i){{$}}
    //     PRT-AO-NEXT: {{^ *}}// #pragma omp target exit data map(from: this->i){{$}}
    //           PRT-O: {{^ *}}#pragma omp target exit data map(from: this->i){{$}}
    // PRT-OA-AST-NEXT: {{^ *}}// #pragma acc exit data copyout(this->i){{$}}
    // PRT-OA-SRC-NEXT: {{^ *}}// #pragma acc exit data copyout(i){{$}}
    //
    // EXE-HOST-NEXT:firstDataMember: 10 -> 10
    // EXE-HOST-NEXT:i: 220 -> 220
    // EXE-HOST-NEXT:lastDataMember: 30 -> 30
    //  EXE-OFF-NEXT:firstDataMember: 10 -> absent
    //  EXE-OFF-NEXT:i: 220 -> absent
    //  EXE-OFF-NEXT:lastDataMember: 30 -> absent
    #pragma acc exit data copyout(i)
    printInt(firstDataMember);
    printInt(i);
    printInt(lastDataMember);
  }
};

int main() {
  Test test;
  test.run();
  return 0;
}
