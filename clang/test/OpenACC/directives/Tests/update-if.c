// Check the "acc update" "if" clause.
//
// FIXME: There are several unfortunate cases where warnings about the "if"
// condition are reported twice, once while parsing OpenACC, and once while
// transforming it to OpenMP.

// We include print checking on only a few representative cases, which should be
// more than sufficient to show it's working for the "if" clause.
//
// REDEFINE: %{exe:fc:args} = -strict-whitespace
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}

// END.

/* expected-error 0 {{}} */

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
  int sCopy, hCopy, dCopy;
  #pragma acc parallel num_gangs(1) copyout(sCopy, hCopy, dCopy)
  {
    sCopy = *sp;
    hCopy = *hp;
    dCopy = *dp;
  }
  printf("  device s=%d, h=%d, d=%d\n", sCopy, hCopy, dCopy);
}

// EXE-NOT: {{.}}

int main(int argc, char *argv[]) {
  // PRT: printf("start\n");
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
  //  EXE-OFF-NEXT:     host s=11, h=21, d=30{{$}}
  //  EXE-OFF-NEXT:   device s=11, h=21, d=30{{$}}
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
  //  EXE-OFF-NEXT:     host s=11, h=21, d=30{{$}}
  //  EXE-OFF-NEXT:   device s=11, h=21, d=30{{$}}
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
  //  EXE-OFF-NEXT:     host s=10, h=20, d=30{{$}}
  //  EXE-OFF-NEXT:   device s=11, h=21, d=31{{$}}
  printf("if(0)\n");
  setValues();
  #pragma acc update self(s) host(h) device(d) if(0)
  printValues();

  //      EXE-NEXT: if(-1)
  // EXE-HOST-NEXT:     host s=11, h=21, d=31{{$}}
  // EXE-HOST-NEXT:   device s=11, h=21, d=31{{$}}
  //  EXE-OFF-NEXT:     host s=11, h=21, d=30{{$}}
  //  EXE-OFF-NEXT:   device s=11, h=21, d=30{{$}}
  printf("if(-1)\n");
  setValues();
  #pragma acc update self(s) host(h) device(d) if(-1)
  printValues();

  //      EXE-NEXT: if(37)
  // EXE-HOST-NEXT:     host s=11, h=21, d=31{{$}}
  // EXE-HOST-NEXT:   device s=11, h=21, d=31{{$}}
  //  EXE-OFF-NEXT:     host s=11, h=21, d=30{{$}}
  //  EXE-OFF-NEXT:   device s=11, h=21, d=30{{$}}
  printf("if(37)\n");
  setValues();
  #pragma acc update self(s) host(h) device(d) if(37)
  printValues();

  //      EXE-NEXT: if(-9)
  // EXE-HOST-NEXT:     host s=11, h=21, d=31{{$}}
  // EXE-HOST-NEXT:   device s=11, h=21, d=31{{$}}
  //  EXE-OFF-NEXT:     host s=11, h=21, d=30{{$}}
  //  EXE-OFF-NEXT:   device s=11, h=21, d=30{{$}}
  printf("if(-9)\n");
  setValues();
  #pragma acc update self(s) host(h) device(d) if(-9)
  printValues();

  //      EXE-NEXT: if(0.0)
  // EXE-HOST-NEXT:     host s=11, h=21, d=31{{$}}
  // EXE-HOST-NEXT:   device s=11, h=21, d=31{{$}}
  //  EXE-OFF-NEXT:     host s=10, h=20, d=30{{$}}
  //  EXE-OFF-NEXT:   device s=11, h=21, d=31{{$}}
  printf("if(0.0)\n");
  setValues();
  #pragma acc update self(s) host(h) device(d) if(0.0)
  printValues();

  //      EXE-NEXT: if(0.1)
  // EXE-HOST-NEXT:     host s=11, h=21, d=31{{$}}
  // EXE-HOST-NEXT:   device s=11, h=21, d=31{{$}}
  //  EXE-OFF-NEXT:     host s=11, h=21, d=30{{$}}
  //  EXE-OFF-NEXT:   device s=11, h=21, d=30{{$}}
  printf("if(0.1)\n");
  setValues();
  /* omp-warning-re@+2 {{implicit conversion from 'double' to '{{_Bool|bool}}' changes value from 0.1 to true}} */
  /* acc-warning-re@+1 2 {{implicit conversion from 'double' to '{{_Bool|bool}}' changes value from 0.1 to true}} */
  #pragma acc update self(s) host(h) device(d) if(0.1)
  printValues();

  //      EXE-NEXT: if(-19.)
  // EXE-HOST-NEXT:     host s=11, h=21, d=31{{$}}
  // EXE-HOST-NEXT:   device s=11, h=21, d=31{{$}}
  //  EXE-OFF-NEXT:     host s=11, h=21, d=30{{$}}
  //  EXE-OFF-NEXT:   device s=11, h=21, d=30{{$}}
  printf("if(-19.)\n");
  setValues();
  /* omp-warning-re@+2 {{implicit conversion from 'double' to '{{_Bool|bool}}' changes value from -19 to true}} */
  /* acc-warning-re@+1 2 {{implicit conversion from 'double' to '{{_Bool|bool}}' changes value from -19 to true}} */
  #pragma acc update self(s) host(h) device(d) if(-19.)
  printValues();

  //      EXE-NEXT: if(NULL)
  // EXE-HOST-NEXT:     host s=11, h=21, d=31{{$}}
  // EXE-HOST-NEXT:   device s=11, h=21, d=31{{$}}
  //  EXE-OFF-NEXT:     host s=10, h=20, d=30{{$}}
  //  EXE-OFF-NEXT:   device s=11, h=21, d=31{{$}}
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
  //  EXE-OFF-NEXT:     host s=10, h=20, d=30{{$}}
  //  EXE-OFF-NEXT:   device s=11, h=21, d=31{{$}}
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
  //  EXE-OFF-NEXT:     host s=10, h=20, d=30{{$}}
  //  EXE-OFF-NEXT:   device s=11, h=21, d=31{{$}}
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
  //  EXE-OFF-NEXT:     host s=11, h=21, d=30{{$}}
  //  EXE-OFF-NEXT:   device s=11, h=21, d=30{{$}}
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
  //  EXE-OFF-NEXT:     host s=11, h=21, d=30{{$}}
  //  EXE-OFF-NEXT:   device s=11, h=21, d=30{{$}}
  printf("if(&i)\n");
  setValues();
  {
    int i;
    /* omp-warning@+2 {{address of 'i' will always evaluate to 'true'}} */
    /* acc-warning@+1 2 {{address of 'i' will always evaluate to 'true'}} */
    #pragma acc update self(s) host(h) device(d) if(&i)
  }
  printValues();

  //      EXE-NEXT: if(s.i)
  //
  // EXE-HOST-NEXT:     host s=11, h=21, d=31{{$}}
  // EXE-HOST-NEXT:   device s=11, h=21, d=31{{$}}
  //  EXE-OFF-NEXT:     host s=10, h=20, d=30{{$}}
  //  EXE-OFF-NEXT:   device s=11, h=21, d=31{{$}}
  //
  // EXE-HOST-NEXT:     host s=11, h=21, d=31{{$}}
  // EXE-HOST-NEXT:   device s=11, h=21, d=31{{$}}
  //  EXE-OFF-NEXT:     host s=11, h=21, d=30{{$}}
  //  EXE-OFF-NEXT:   device s=11, h=21, d=30{{$}}
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
