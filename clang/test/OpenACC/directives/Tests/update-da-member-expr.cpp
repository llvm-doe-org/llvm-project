// Check motion clause for member expressions and for 'this' on "acc update",
// but only check cases that are not covered by update.c because they are
// specific to C++.

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
  int x;
  int y;
  int z;
  void run() {
    //--------------------------------------------------------------------------
    // Check that explicit data clause for this[0:1] behaves.
    //--------------------------------------------------------------------------

    // DMP-LABEL:"explicit DA for this\n"
    // PRT-LABEL:"explicit DA for this\n"
    // EXE-LABEL:explicit DA for this
    printf("explicit DA for this\n");

    // EXE-HOST-NEXT:x: 110 -> 110
    // EXE-HOST-NEXT:y: 220 -> 220
    // EXE-HOST-NEXT:z: 330 -> 330
    //  EXE-OFF-NEXT:x: 110 -> 10
    //  EXE-OFF-NEXT:y: 220 -> 20
    //  EXE-OFF-NEXT:z: 330 -> 30
    x = 10;
    y = 20;
    z = 30;
    #pragma acc enter data copyin(this[0:1])
    x += 100;
    y += 200;
    z += 300;
    printInt(x);
    printInt(y);
    printInt(z);

    //      DMP: ACCUpdateDirective
    // DMP-NEXT:   ACCDeviceClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:   impl: OMPTargetUpdateDirective
    // DMP-NEXT:     OMPToClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       OMPArraySectionExpr
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:         <<<NULL>>>
    //
    //       PRT-A: {{^ *}}#pragma acc update device(this[0:1]){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target update to(present: this[0:1]){{$}}
    //       PRT-O: {{^ *}}#pragma omp target update to(present: this[0:1]){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc update device(this[0:1]){{$}}
    //
    // EXE-NEXT:x: 110 -> 110
    // EXE-NEXT:y: 220 -> 220
    // EXE-NEXT:z: 330 -> 330
    #pragma acc update device(this[0:1])
    printInt(x);
    printInt(y);
    printInt(z);

    // EXE-HOST-NEXT:x: 1110 -> 1110
    // EXE-HOST-NEXT:y: 2220 -> 2220
    // EXE-HOST-NEXT:z: 3330 -> 3330
    //  EXE-OFF-NEXT:x: 1110 -> 110
    //  EXE-OFF-NEXT:y: 2220 -> 220
    //  EXE-OFF-NEXT:z: 3330 -> 330
    x += 1000;
    y += 2000;
    z += 3000;
    printInt(x);
    printInt(y);
    printInt(z);

    //      DMP: ACCUpdateDirective
    // DMP-NEXT:   ACCSelfClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     OMPArraySectionExpr
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:       <<<NULL>>>
    // DMP-NEXT:   impl: OMPTargetUpdateDirective
    // DMP-NEXT:     OMPFromClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       OMPArraySectionExpr
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
    // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    // DMP-NEXT:         <<<NULL>>>
    //
    //       PRT-A: {{^ *}}#pragma acc update self(this[0:1]){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target update from(present: this[0:1]){{$}}
    //       PRT-O: {{^ *}}#pragma omp target update from(present: this[0:1]){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc update self(this[0:1]){{$}}
    //
    // EXE-HOST-NEXT:x: 1110 -> 1110
    // EXE-HOST-NEXT:y: 2220 -> 2220
    // EXE-HOST-NEXT:z: 3330 -> 3330
    //  EXE-OFF-NEXT:x: 110 -> 110
    //  EXE-OFF-NEXT:y: 220 -> 220
    //  EXE-OFF-NEXT:z: 330 -> 330
    #pragma acc update self(this[0:1])
    printInt(x);
    printInt(y);
    printInt(z);

    #pragma acc exit data delete(this[0:1])

    //--------------------------------------------------------------------------
    // Check explicit data clause for a data member of 'this'.
    //
    // Check explicit 'this' and implicit 'this'.  Also check when that doesn't
    //--------------------------------------------------------------------------

    // DMP-LABEL:"explicit DAs for data member on this\n"
    // PRT-LABEL:"explicit DAs for data member on this\n"
    // EXE-LABEL:explicit DAs for data member on this
    printf("explicit DAs for data member on this\n");

    // EXE-HOST-NEXT:x: 110 -> 110
    // EXE-HOST-NEXT:y: 220 -> 220
    // EXE-HOST-NEXT:z: 330 -> 330
    //  EXE-OFF-NEXT:x: 110 -> 10
    //  EXE-OFF-NEXT:y: 220 -> 20
    //  EXE-OFF-NEXT:z: 330 -> 30
    x = 10;
    y = 20;
    z = 30;
    #pragma acc enter data copyin(this[0:1])
    x += 100;
    y += 200;
    z += 300;
    printInt(x);
    printInt(y);
    printInt(z);

    //      DMP: ACCUpdateDirective
    // DMP-NEXT:   ACCDeviceClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     MemberExpr {{.* ->y }}
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:     MemberExpr {{.* ->z }}
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this
    // DMP-NEXT:   impl: OMPTargetUpdateDirective
    // DMP-NEXT:     OMPToClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       MemberExpr {{.* ->y }}
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:       MemberExpr {{.* ->z }}
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' implicit this
    //
    //       PRT-A-AST: {{^ *}}#pragma acc update device(this->y,this->z){{$}}
    //       PRT-A-SRC: {{^ *}}#pragma acc update device(this->y,z){{$}}
    //     PRT-AO-NEXT: {{^ *}}// #pragma omp target update to(present: this->y,this->z){{$}}
    //           PRT-O: {{^ *}}#pragma omp target update to(present: this->y,this->z){{$}}
    // PRT-OA-AST-NEXT: {{^ *}}// #pragma acc update device(this->y,this->z){{$}}
    // PRT-OA-SRC-NEXT: {{^ *}}// #pragma acc update device(this->y,z){{$}}
    //
    // EXE-HOST-NEXT:x: 110 -> 110
    // EXE-HOST-NEXT:y: 220 -> 220
    // EXE-HOST-NEXT:z: 330 -> 330
    //  EXE-OFF-NEXT:x: 110 -> 10
    //  EXE-OFF-NEXT:y: 220 -> 220
    //  EXE-OFF-NEXT:z: 330 -> 330
    #pragma acc update device(this->y,z)
    printInt(x);
    printInt(y);
    printInt(z);

    // EXE-HOST-NEXT:x: 1110 -> 1110
    // EXE-HOST-NEXT:y: 2220 -> 2220
    // EXE-HOST-NEXT:z: 3330 -> 3330
    //  EXE-OFF-NEXT:x: 1110 -> 10
    //  EXE-OFF-NEXT:y: 2220 -> 220
    //  EXE-OFF-NEXT:z: 3330 -> 330
    x += 1000;
    y += 2000;
    z += 3000;
    printInt(x);
    printInt(y);
    printInt(z);

    //      DMP: ACCUpdateDirective
    // DMP-NEXT:   ACCSelfClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     MemberExpr {{.* ->x }}
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' implicit this
    // DMP-NEXT:   ACCSelfClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     MemberExpr {{.* ->z }}
    // DMP-NEXT:       CXXThisExpr {{.*}} 'Test *' this
    // DMP-NEXT:   impl: OMPTargetUpdateDirective
    // DMP-NEXT:     OMPFromClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       MemberExpr {{.* ->x }}
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' implicit this
    // DMP-NEXT:     OMPFromClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       MemberExpr {{.* ->z }}
    // DMP-NEXT:         CXXThisExpr {{.*}} 'Test *' this
    //
    //       PRT-A-AST: {{^ *}}#pragma acc update self(this->x) host(this->z){{$}}
    //       PRT-A-SRC: {{^ *}}#pragma acc update self(x) host(this->z){{$}}
    //     PRT-AO-NEXT: {{^ *}}// #pragma omp target update from(present: this->x) from(present: this->z){{$}}
    //           PRT-O: {{^ *}}#pragma omp target update from(present: this->x) from(present: this->z){{$}}
    // PRT-OA-AST-NEXT: {{^ *}}// #pragma acc update self(this->x) host(this->z){{$}}
    // PRT-OA-SRC-NEXT: {{^ *}}// #pragma acc update self(x) host(this->z){{$}}
    //
    // EXE-HOST-NEXT:x: 1110 -> 1110
    // EXE-HOST-NEXT:y: 2220 -> 2220
    // EXE-HOST-NEXT:z: 3330 -> 3330
    //  EXE-OFF-NEXT:x: 10 -> 10
    //  EXE-OFF-NEXT:y: 2220 -> 220
    //  EXE-OFF-NEXT:z: 330 -> 330
    #pragma acc update self(x) host(this->z)
    printInt(x);
    printInt(y);
    printInt(z);

    #pragma acc exit data delete(this[0:1])
  }
};

int main() {
  Test test;
  test.run();
  return 0;
}
