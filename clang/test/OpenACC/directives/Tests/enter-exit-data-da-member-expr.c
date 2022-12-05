// Check data clauses for member expressions on "acc enter data" and
// "acc exit data".
//
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}

// END.

/* expected-no-diagnostics */

#include <openacc.h>
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
  printf("    host %s %d\n", Name, *Var);
}
void printDeviceInt_(const char *Name, int *Var) {
  printf("  device %s ", Name);
  if (!acc_is_present(Var, sizeof(*Var)))
    printf("absent");
  else {
    int TgtVal;
    #pragma acc parallel num_gangs(1) copyout(TgtVal)
    TgtVal = *Var;
    printf("present %d", TgtVal);
  }
  printf("\n");
}

// EXE-NOT: {{.}}

int main() {
  //----------------------------------------------------------------------------
  // Check action suppression in the case of member expressions.
  //----------------------------------------------------------------------------

  // DMP-LABEL: "member expressions\n"
  // PRT-LABEL: "member expressions\n"
  // EXE-LABEL: member expressions
  printf("member expressions\n");

  {
    struct T { int before; int i; int after; };
    struct T dyn, str;
    //      DMP: ACCEnterDataDirective
    // DMP-NEXT:   ACCCopyinClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     MemberExpr {{.* .i }}
    // DMP-NEXT:       DeclRefExpr {{.*}} 'dyn'
    // DMP-NEXT:   impl: OMPTargetEnterDataDirective
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       MemberExpr {{.* .i }}
    // DMP-NEXT:         DeclRefExpr {{.*}} 'dyn'
    //      DMP: ACCDataDirective
    //
    //       PRT-A: {{^ *}}#pragma acc enter data copyin(dyn.i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target enter data map(to: dyn.i){{$}}
    //       PRT-O: {{^ *}}#pragma omp target enter data map(to: dyn.i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc enter data copyin(dyn.i){{$}}
    #pragma acc enter data copyin(dyn.i)
    #pragma acc data copy(str.i)
    {
      // Making a single member present shouldn't make the larger struct
      // present.
      // EXE-HOST-NEXT: dyn is present: 1
      // EXE-HOST-NEXT: str is present: 1
      //  EXE-OFF-NEXT: dyn is present: 0
      //  EXE-OFF-NEXT: str is present: 0
      printf("dyn is present: %d\n", acc_is_present(&dyn, sizeof dyn));
      printf("str is present: %d\n", acc_is_present(&str, sizeof str));

      setHostInt(dyn.i, 10);
      setHostInt(str.i, 20);
      setDeviceInt(dyn.i, 11);
      setDeviceInt(str.i, 21);

      // Actions for the same member expression should have no effect except to
      // adjust the dynamic reference count.
      //
      //      DMP: ACCDataDirective
      //      DMP: ACCEnterDataDirective
      // DMP-NEXT:   ACCCopyinClause
      //  DMP-NOT:     <implicit>
      // DMP-NEXT:     MemberExpr {{.* .i }}
      // DMP-NEXT:       DeclRefExpr {{.*}} 'dyn'
      // DMP-NEXT:     MemberExpr {{.* .i }}
      // DMP-NEXT:       DeclRefExpr {{.*}} 'str'
      // DMP-NEXT:   impl: OMPTargetEnterDataDirective
      // DMP-NEXT:     OMPMapClause
      //  DMP-NOT:       <implicit>
      // DMP-NEXT:       MemberExpr {{.* .i }}
      // DMP-NEXT:         DeclRefExpr {{.*}} 'dyn'
      // DMP-NEXT:       MemberExpr {{.* .i }}
      // DMP-NEXT:         DeclRefExpr {{.*}} 'str'
      //      DMP: ACCExitDataDirective
      // DMP-NEXT:   ACCCopyoutClause
      //  DMP-NOT:     <implicit>
      // DMP-NEXT:     MemberExpr {{.* .i }}
      // DMP-NEXT:       DeclRefExpr {{.*}} 'dyn'
      // DMP-NEXT:     MemberExpr {{.* .i }}
      // DMP-NEXT:       DeclRefExpr {{.*}} 'str'
      // DMP-NEXT:   impl: OMPTargetExitDataDirective
      // DMP-NEXT:     OMPMapClause
      //  DMP-NOT:       <implicit>
      // DMP-NEXT:       MemberExpr {{.* .i }}
      // DMP-NEXT:         DeclRefExpr {{.*}} 'dyn'
      // DMP-NEXT:       MemberExpr {{.* .i }}
      // DMP-NEXT:         DeclRefExpr {{.*}} 'str'
      //      DMP: ACCExitDataDirective
      //      DMP: ACCEnterDataDirective
      //      DMP: ACCExitDataDirective
      //      DMP: ACCExitDataDirective
      //
      //         PRT: int fc_prt_start;
      //  PRT-A-NEXT: {{^ *}}#pragma acc enter data copyin(dyn.i,str.i){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target enter data map(to: dyn.i,str.i){{$}}
      //  PRT-O-NEXT: {{^ *}}#pragma omp target enter data map(to: dyn.i,str.i){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc enter data copyin(dyn.i,str.i){{$}}
      //  PRT-A-NEXT: {{^ *}}#pragma acc exit data copyout(dyn.i,str.i){{$}}
      // PRT-AO-NEXT: {{^ *}}// #pragma omp target exit data map(from: dyn.i,str.i){{$}}
      //  PRT-O-NEXT: {{^ *}}#pragma omp target exit data map(from: dyn.i,str.i){{$}}
      // PRT-OA-NEXT: {{^ *}}// #pragma acc exit data copyout(dyn.i,str.i){{$}}
      //    PRT-NEXT: int fc_prt_end;
      #pragma acc data copy(dyn.i)
      ;
      int fc_prt_start;
      #pragma acc enter data copyin(dyn.i,str.i)
      #pragma acc exit data copyout(dyn.i,str.i)
      int fc_prt_end;
      #pragma acc exit data copyout(str.i)
      #pragma acc enter data create(dyn.i,str.i)
      #pragma acc exit data delete(dyn.i,str.i)
      #pragma acc exit data delete(str.i)

      // EXE-HOST-NEXT:   host dyn.i         11{{$}}
      // EXE-HOST-NEXT:   host str.i         21{{$}}
      //  EXE-OFF-NEXT:   host dyn.i         10{{$}}
      //  EXE-OFF-NEXT:   host str.i         20{{$}}
      //      EXE-NEXT: device dyn.i present 11{{$}}
      //      EXE-NEXT: device str.i present 21{{$}}
      printHostInt(dyn.i);
      printHostInt(str.i);
      printDeviceInt(dyn.i);
      printDeviceInt(str.i);

      // acc exit data should have no effect for entire struct when only a
      // member is present.
      //
      // DMP: ACCExitDataDirective
      // DMP: ACCExitDataDirective
      // DMP: ACCExitDataDirective
      // DMP: ACCExitDataDirective
      #pragma acc exit data copyout(dyn)
      #pragma acc exit data delete(dyn)
      #pragma acc exit data copyout(str)
      #pragma acc exit data delete(str)

      // EXE-HOST-NEXT:   host dyn.i         11{{$}}
      // EXE-HOST-NEXT:   host str.i         21{{$}}
      //  EXE-OFF-NEXT:   host dyn.i         10{{$}}
      //  EXE-OFF-NEXT:   host str.i         20{{$}}
      //      EXE-NEXT: device dyn.i present 11{{$}}
      //      EXE-NEXT: device str.i present 21{{$}}
      printHostInt(dyn.i);
      printHostInt(str.i);
      printDeviceInt(dyn.i);
      printDeviceInt(str.i);
    }

    // acc exit data should have an effect on a member that's present.
    //
    //      DMP: ACCExitDataDirective
    // DMP-NEXT:   ACCCopyoutClause
    //  DMP-NOT:     <implicit>
    // DMP-NEXT:     MemberExpr {{.* .i }}
    // DMP-NEXT:       DeclRefExpr {{.*}} 'dyn'
    // DMP-NEXT:   impl: OMPTargetExitDataDirective
    // DMP-NEXT:     OMPMapClause
    //  DMP-NOT:       <implicit>
    // DMP-NEXT:       MemberExpr {{.* .i }}
    // DMP-NEXT:         DeclRefExpr {{.*}} 'dyn'
    //
    //         PRT: int fc_prt_final;
    //  PRT-A-NEXT: {{^ *}}#pragma acc exit data copyout(dyn.i){{$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target exit data map(from: dyn.i){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target exit data map(from: dyn.i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc exit data copyout(dyn.i){{$}}
    int fc_prt_final;
    #pragma acc exit data copyout(dyn.i)

    // EXE-HOST-NEXT:   host dyn.i         11{{$}}
    //  EXE-OFF-NEXT:   host dyn.i         11{{$}}
    // EXE-HOST-NEXT: device dyn.i present 11{{$}}
    //  EXE-OFF-NEXT: device dyn.i absent{{$}}
    printHostInt(dyn.i);
    printDeviceInt(dyn.i);
  }

  return 0;
}

// EXE-NOT: {{.}}
