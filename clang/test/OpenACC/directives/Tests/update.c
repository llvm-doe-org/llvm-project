// Check "acc update" with different values of -fopenacc-update-present-omp and
// sometimes with if_present.
//
// Diagnostics about the present motion modifier in the translation are checked
// in diagnostics/warn-acc-omp-update-present.c.  Diagnostics about bad
// -fopenacc-update-present-omp arguments are checked in
// diagnostics/fopenacc-update-present-omp.c.  The "if" clause is checked in
// update-if.c.
//
// We include print checking on only a few representative cases, which should be
// more than sufficient to show it's working for the update directive.

// Redefine this to specify how %{check-cases} expands for each case.
//
// - CASE = the case name used in the enum and as a command line argument.
// - NOT_CRASH_IF_FAIL = 'not --crash' if the case is expected to fail a
//   presence check, and the empty string otherwise.
// - NOT_IF_FAIL = same as NOT_CRASH_IF_FAIL except without the '--crash'.
//
// DEFINE: %{check-case}( CASE %, NOT_CRASH_IF_FAIL %, NOT_IF_FAIL %) =

// Substitution to run %{check-case} for each case.
//
// - NOT_CRASH_IF_PRESENT = 'not --crash' if presence checks are generally
//   enabled, and the empty string otherwise.  Each case will use this if it is
//   expected to fail a presence check.
// - NOT_IF_PRESENT = same as NOT_CRASH_IF_PRESENT except without the '--crash'.
//
// If a case is listed here but is not covered in the code, that case will fail.
// If a case is covered in the code but not listed here, the code will not
// compile because this list produces the enum used by the code.
//
// The various cases covered here should be kept consistent with present.c,
// no-create.c, and data-extension-errors.c (the last is located in
// openmp/libacc2omp/test/directives).  For example, a subarray that extends a
// subarray already present is consistently considered not present, so the
// present clause produces a runtime error and the no_create clause doesn't
// allocate.
//
// DEFINE: %{check-cases}( NOT_CRASH_IF_PRESENT %, NOT_IF_PRESENT %) =                                                                   \
//                          CASE                        NOT_CRASH_IF_FAIL                         NOT_IF_FAIL
// DEFINE:   %{check-case}( caseNoParentPresent      %,                                        %,                                  %) && \
// DEFINE:   %{check-case}( caseNoParentAbsent       %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %) && \
// DEFINE:   %{check-case}( caseScalarPresent        %,                                        %,                                  %) && \
// DEFINE:   %{check-case}( caseScalarAbsent         %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %) && \
// DEFINE:   %{check-case}( caseStructPresent        %,                                        %,                                  %) && \
// DEFINE:   %{check-case}( caseStructAbsent         %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %) && \
// DEFINE:   %{check-case}( caseArrayPresent         %,                                        %,                                  %) && \
// DEFINE:   %{check-case}( caseArrayAbsent          %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %) && \
// DEFINE:   %{check-case}( caseMemberPresent        %,                                        %,                                  %) && \
// DEFINE:   %{check-case}( caseMemberAbsent         %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %) && \
// DEFINE:   %{check-case}( caseMembersDisjoint      %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %) && \
// DEFINE:   %{check-case}( caseMembersConcat2       %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %) && \
// DEFINE:   %{check-case}( caseMemberFullStruct     %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %) && \
// DEFINE:   %{check-case}( caseSubarrayPresent      %,                                        %,                                  %) && \
// DEFINE:   %{check-case}( caseSubarrayPresent2     %,                                        %,                                  %) && \
// DEFINE:   %{check-case}( caseSubarrayDisjoint     %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %) && \
// DEFINE:   %{check-case}( caseSubarrayOverlapStart %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %) && \
// DEFINE:   %{check-case}( caseSubarrayOverlapEnd   %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %) && \
// DEFINE:   %{check-case}( caseSubarrayConcat2      %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %) && \
// DEFINE:   %{check-case}( caseSubarrayNonSubarray  %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %)

// Generate the enum of cases.
//
//      RUN: echo '#define FOREACH_CASE(Macro) \' > %t-cases.h
// REDEFINE: %{check-case}( CASE %, NOT_CRASH_IF_FAIL %, NOT_IF_FAIL %) =      \
// REDEFINE: echo '  Macro(%{CASE}) \' >> %t-cases.h
//      RUN: %{check-cases}( %, %)
//      RUN: echo '  /*end of FOREACH_CASE*/' >> %t-cases.h

// Prepare substitutions for trying all cases many times while varying the
// value of -fopenacc-update-present-omp and whether if_present is specified.
//
// REDEFINE: %{all:clang:args-stable} = \
// REDEFINE:   -DCASES_HEADER='"%t-cases.h"' -gline-tables-only
// REDEFINE: %{exe:fc:args-stable} = -strict-whitespace
// REDEFINE: %{check-case}( CASE %, NOT_CRASH_IF_FAIL %, NOT_IF_FAIL %) =                                 \
// REDEFINE:   : '----------------- CASE: %{CASE} -----------------'                                   && \
// REDEFINE:   %{acc-check-exe-run-fn}( %{NOT_CRASH_IF_FAIL} %, %{CASE} %)                             && \
// REDEFINE:   %{acc-check-exe-filecheck-fn}( %{CASE},%{NOT_IF_FAIL}PASS,%{CASE}-%{NOT_IF_FAIL}PASS %)
//   DEFINE: %{check-present} =                                                \
//   DEFINE:   %{acc-check-dmp}                                             && \
//   DEFINE:   %{acc-check-prt}                                             && \
//   DEFINE:   %{acc-check-exe-compile}                                     && \
//   DEFINE:   %{check-cases}( not --crash %, not %)
//   DEFINE: %{check-no-present} =                                             \
//   DEFINE:   %{acc-check-dmp}                                             && \
//   DEFINE:   %{acc-check-prt}                                             && \
//   DEFINE:   %{acc-check-exe-compile}                                     && \
//   DEFINE:   %{check-cases}( %, %)

// REDEFINE: %{all:clang:args} = -DIF_PRESENT=
// REDEFINE: %{dmp:fc:pres} = NOT_IF_PRESENT
// REDEFINE: %{prt:fc:args} = -DPRESENT='present: '
//      RUN: %{check-present}

// REDEFINE: %{all:clang:args} =                                               \
// REDEFINE:   -DIF_PRESENT= -fopenacc-update-present-omp=present
// REDEFINE: %{dmp:fc:pres} = NOT_IF_PRESENT
// REDEFINE: %{prt:fc:args} = -DPRESENT='present: '
//      RUN: %{check-present}

// REDEFINE: %{all:clang:args} =                                               \
// REDEFINE:   -DIF_PRESENT= -fopenacc-update-present-omp=no-present
// REDEFINE: %{dmp:fc:pres} = NOT_IF_PRESENT
// REDEFINE: %{prt:fc:args} = -DPRESENT=
//      RUN: %{check-no-present}

// REDEFINE: %{all:clang:args} = -DIF_PRESENT=if_present
// REDEFINE: %{dmp:fc:pres} = IF_PRESENT
// REDEFINE: %{prt:fc:args} = -DPRESENT=
//      RUN: %{check-no-present}

// REDEFINE: %{all:clang:args} =                                               \
// REDEFINE:   -DIF_PRESENT=if_present -fopenacc-update-present-omp=present
// REDEFINE: %{dmp:fc:pres} = IF_PRESENT
// REDEFINE: %{prt:fc:args} = -DPRESENT=
//      RUN: %{check-no-present}

// REDEFINE: %{all:clang:args} =                                               \
// REDEFINE:   -DIF_PRESENT=if_present -fopenacc-update-present-omp=no-present
// REDEFINE: %{dmp:fc:pres} = IF_PRESENT
// REDEFINE: %{prt:fc:args} = -DPRESENT=
//      RUN: %{check-no-present}

// END.

/* expected-no-diagnostics */

#include <stdio.h>
#include <string.h>

#define PRINT_VAR_INFO(Var) \
  fprintf(stderr, "addr=%p, size=%ld\n", &(Var), sizeof (Var))

#define PRINT_SUBARRAY_INFO(Arr, Start, Length) \
  fprintf(stderr, "addr=%p, size=%ld\n", &(Arr)[Start], \
          Length * sizeof (Arr[0]))

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
  fprintf(stderr, "    host %s=%d\n", Name, *Var);
}
void printDeviceInt_(const char *Name, int *Var) {
  fprintf(stderr, "  device %s=", Name);
  int VarCopy;
  #pragma acc parallel num_gangs(1) copyout(VarCopy)
  VarCopy = *Var;
  fprintf(stderr, "%d\n", VarCopy);
}

// DMP-LABEL: FunctionDecl {{.*}} updateNotNested
// PRT-LABEL: void updateNotNested(
void updateNotNested(int *s, int *h, int *d) {
  //                 DMP: ACCUpdateDirective
  //            DMP-NEXT:   ACCSelfClause
  //            DMP-NEXT:     OMPArraySectionExpr
  //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <LValueToRValue>
  //            DMP-NEXT:         DeclRefExpr {{.*}} 's' 'int *'
  //            DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
  //            DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  //            DMP-NEXT:       <<<NULL>>>
  //            DMP-NEXT:   ACCSelfClause
  //            DMP-NEXT:     OMPArraySectionExpr
  //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <LValueToRValue>
  //            DMP-NEXT:         DeclRefExpr {{.*}} 'h' 'int *'
  //            DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
  //            DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  //            DMP-NEXT:       <<<NULL>>>
  //            DMP-NEXT:   ACCDeviceClause
  //            DMP-NEXT:     OMPArraySectionExpr
  //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <LValueToRValue>
  //            DMP-NEXT:         DeclRefExpr {{.*}} 'd' 'int *'
  //            DMP-NEXT:       IntegerLiteral {{.*}} 'int' 0
  //            DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  //            DMP-NEXT:       <<<NULL>>>
  // DMP-IF_PRESENT-NEXT:   ACCIfPresentClause
  //            DMP-NEXT:   impl: OMPTargetUpdateDirective
  //            DMP-NEXT:     OMPFromClause
  //            DMP-NEXT:       OMPArraySectionExpr
  //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <LValueToRValue>
  //            DMP-NEXT:           DeclRefExpr {{.*}} 's' 'int *'
  //            DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
  //            DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
  //            DMP-NEXT:         <<<NULL>>>
  //            DMP-NEXT:     OMPFromClause
  //            DMP-NEXT:       OMPArraySectionExpr
  //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <LValueToRValue>
  //            DMP-NEXT:           DeclRefExpr {{.*}} 'h' 'int *'
  //            DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
  //            DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
  //            DMP-NEXT:         <<<NULL>>>
  //            DMP-NEXT:     OMPToClause
  //            DMP-NEXT:       OMPArraySectionExpr
  //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <LValueToRValue>
  //            DMP-NEXT:           DeclRefExpr {{.*}} 'd' 'int *'
  //            DMP-NEXT:         IntegerLiteral {{.*}} 'int' 0
  //            DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
  //            DMP-NEXT:         <<<NULL>>>
  //             DMP-NOT: OMP
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc update self(s[0:1]) host(h[0:1]) device(d[0:1]){{( IF_PRESENT| if_present)?$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target update from([[PRESENT]]s[0:1]) from([[PRESENT]]h[0:1]) to([[PRESENT]]d[0:1]){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target update from([[PRESENT]]s[0:1]) from([[PRESENT]]h[0:1]) to([[PRESENT]]d[0:1]){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc update self(s[0:1]) host(h[0:1]) device(d[0:1]){{( IF_PRESENT| if_present)?$}}
  #pragma acc update self(s[0:1]) host(h[0:1]) device(d[0:1]) IF_PRESENT

  // DMP: CallExpr
  fprintf(stderr, "updated\n");
}

// Make each static to ensure we get a compile warning if it's never called.
#include CASES_HEADER
#define CASE(CaseName) static void CaseName(void)
#define AddCase(CaseName) CASE(CaseName);
FOREACH_CASE(AddCase)
#undef AddCase

int main(int argc, char *argv[]) {
  CASE((*caseFn));
  if (argc != 2) {
    fprintf(stderr, "expected one argument\n");
    return 1;
  }
#define AddCase(CaseName)                                                      \
  else if (!strcmp(argv[1], #CaseName))                                        \
    caseFn = CaseName;
  FOREACH_CASE(AddCase)
#undef AddCase
  else {
    fprintf(stderr, "unexpected case: %s\n", argv[1]);
    return 1;
  }

  fprintf(stderr, "start\n");
  fflush(stderr);
  caseFn();
  return 0;
}

//  EXE-NOT: {{.}}
//      EXE: start
// EXE-NEXT: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
// EXE-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]

//------------------------------------------------------------------------------
// Check acc update not statically nested within another construct.
//
// Check with scalar type via subarray on pointer.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseNoParentPresent
// PRT-LABEL: {{.*}}caseNoParentPresent{{.*}} {
CASE(caseNoParentPresent) {
  int s, h, d;
  PRINT_VAR_INFO(s);
  PRINT_VAR_INFO(s);
  #pragma acc data create(s,h,d)
  {
    setHostInt(s, 10);
    setHostInt(h, 20);
    setHostInt(d, 30);
    setDeviceInt(s, 11);
    setDeviceInt(h, 21);
    setDeviceInt(d, 31);

    // EXE-caseNoParentPresent-NEXT: updated
    updateNotNested(&s, &h, &d);

    //      EXE-caseNoParentPresent-NEXT:   host s=11{{$}}
    //      EXE-caseNoParentPresent-NEXT:   host h=21{{$}}
    //  EXE-OFF-caseNoParentPresent-NEXT:   host d=30{{$}}
    // EXE-HOST-caseNoParentPresent-NEXT:   host d=31{{$}}
    //      EXE-caseNoParentPresent-NEXT: device s=11{{$}}
    //      EXE-caseNoParentPresent-NEXT: device h=21{{$}}
    //  EXE-OFF-caseNoParentPresent-NEXT: device d=30{{$}}
    // EXE-HOST-caseNoParentPresent-NEXT: device d=31{{$}}
    printHostInt(s);
    printHostInt(h);
    printHostInt(d);
    printDeviceInt(s);
    printDeviceInt(h);
    printDeviceInt(d);
  }
}

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseNoParentAbsent
// PRT-LABEL: {{.*}}caseNoParentAbsent{{.*}} {
CASE(caseNoParentAbsent) {
  int s, h, d;
  PRINT_VAR_INFO(d);
  PRINT_VAR_INFO(d);
  setHostInt(s, 10);
  setHostInt(h, 20);
  setHostInt(d, 30);

  // EXE-caseNoParentAbsent-PASS-NEXT: updated
  updateNotNested(&s, &h, &d);

  // EXE-caseNoParentAbsent-PASS-NEXT: host s=10{{$}}
  // EXE-caseNoParentAbsent-PASS-NEXT: host h=20{{$}}
  // EXE-caseNoParentAbsent-PASS-NEXT: host d=30{{$}}
  printHostInt(s);
  printHostInt(h);
  printHostInt(d);
}

//------------------------------------------------------------------------------
// Check acc update nested statically within acc data.
//
// Check that dumping and printing integrate correctly with the outer directive.
//
// Check with scalar type.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseScalarPresent
// PRT-LABEL: {{.*}}caseScalarPresent{{.*}} {
CASE(caseScalarPresent) {
  // PRT-NEXT: int s, h, d;
  int s, h, d;
  // PRT-NEXT: {{PRINT_VAR_INFO|fprintf}}
  // PRT-NEXT: {{PRINT_VAR_INFO|fprintf}}
  PRINT_VAR_INFO(s);
  PRINT_VAR_INFO(s);

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
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target data map(ompx_hold,alloc: s,h,d){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target data map(ompx_hold,alloc: s,h,d){{$}}
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

    //                 DMP: ACCUpdateDirective
    //            DMP-NEXT:   ACCSelfClause
    //            DMP-NEXT:     DeclRefExpr {{.*}} 's' 'int'
    //            DMP-NEXT:   ACCSelfClause
    //            DMP-NEXT:     DeclRefExpr {{.*}} 'h' 'int'
    //            DMP-NEXT:   ACCDeviceClause
    //            DMP-NEXT:     DeclRefExpr {{.*}} 'd' 'int'
    // DMP-IF_PRESENT-NEXT:   ACCIfPresentClause
    //            DMP-NEXT:   impl: OMPTargetUpdateDirective
    //            DMP-NEXT:     OMPFromClause
    //            DMP-NEXT:       DeclRefExpr {{.*}} 's' 'int'
    //            DMP-NEXT:     OMPFromClause
    //            DMP-NEXT:       DeclRefExpr {{.*}} 'h' 'int'
    //            DMP-NEXT:     OMPToClause
    //            DMP-NEXT:       DeclRefExpr {{.*}} 'd' 'int'
    //             DMP-NOT: OMP
    //
    //  PRT-A-NEXT: {{^ *}}#pragma acc update self(s) host(h) device(d){{( IF_PRESENT| if_present)?$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target update from([[PRESENT]]s) from([[PRESENT]]h) to([[PRESENT]]d){{$}}
    //  PRT-O-NEXT: {{^ *}}#pragma omp target update from([[PRESENT]]s) from([[PRESENT]]h) to([[PRESENT]]d){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc update self(s) host(h) device(d){{( IF_PRESENT| if_present)?$}}
    #pragma acc update self(s) host(h) device(d) IF_PRESENT

    // PRT-NEXT: printHostInt
    // PRT-NEXT: printHostInt
    // PRT-NEXT: printHostInt
    // PRT-NEXT: printDeviceInt
    // PRT-NEXT: printDeviceInt
    // PRT-NEXT: printDeviceInt
    //      EXE-caseScalarPresent-NEXT:   host s=11{{$}}
    //      EXE-caseScalarPresent-NEXT:   host h=21{{$}}
    //  EXE-OFF-caseScalarPresent-NEXT:   host d=30{{$}}
    // EXE-HOST-caseScalarPresent-NEXT:   host d=31{{$}}
    //      EXE-caseScalarPresent-NEXT: device s=11{{$}}
    //      EXE-caseScalarPresent-NEXT: device h=21{{$}}
    //  EXE-OFF-caseScalarPresent-NEXT: device d=30{{$}}
    // EXE-HOST-caseScalarPresent-NEXT: device d=31{{$}}
    printHostInt(s);
    printHostInt(h);
    printHostInt(d);
    printDeviceInt(s);
    printDeviceInt(h);
    printDeviceInt(d);
  } // PRT-NEXT: }
} // PRT-NEXT: }

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseScalarAbsent
// PRT-LABEL: {{.*}}caseScalarAbsent{{.*}} {
CASE(caseScalarAbsent) {
  int s, h, d;
  PRINT_VAR_INFO(s);
  PRINT_VAR_INFO(s);

  setHostInt(s, 10);
  setHostInt(h, 20);
  setHostInt(d, 30);

  // We need multiple directives here so we can control which clause produces
  // the runtime error.  We vary which clause produces the runtime error
  // across the various CASE_* that produce it.
  #pragma acc update self(s) IF_PRESENT
  #pragma acc update host(h) device(d) IF_PRESENT

  // EXE-caseScalarAbsent-PASS-NEXT: host s=10{{$}}
  // EXE-caseScalarAbsent-PASS-NEXT: host h=20{{$}}
  // EXE-caseScalarAbsent-PASS-NEXT: host d=30{{$}}
  printHostInt(s);
  printHostInt(h);
  printHostInt(d);
}

//------------------------------------------------------------------------------
// Check struct.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseStructPresent
// PRT-LABEL: {{.*}}caseStructPresent{{.*}} {
CASE(caseStructPresent) {
  struct S { int i; int j; } s, h, d;
  PRINT_VAR_INFO(s);
  PRINT_VAR_INFO(s);

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

    //                 DMP: ACCUpdateDirective
    //            DMP-NEXT:   ACCSelfClause
    //            DMP-NEXT:     DeclRefExpr {{.*}} 's' 'struct S'
    //            DMP-NEXT:   ACCSelfClause
    //            DMP-NEXT:     DeclRefExpr {{.*}} 'h' 'struct S'
    //            DMP-NEXT:   ACCDeviceClause
    //            DMP-NEXT:     DeclRefExpr {{.*}} 'd' 'struct S'
    // DMP-IF_PRESENT-NEXT:   ACCIfPresentClause
    //            DMP-NEXT:   impl: OMPTargetUpdateDirective
    //            DMP-NEXT:     OMPFromClause
    //            DMP-NEXT:       DeclRefExpr {{.*}} 's' 'struct S'
    //            DMP-NEXT:     OMPFromClause
    //            DMP-NEXT:       DeclRefExpr {{.*}} 'h' 'struct S'
    //            DMP-NEXT:     OMPToClause
    //            DMP-NEXT:       DeclRefExpr {{.*}} 'd' 'struct S'
    //             DMP-NOT: OMP
    //
    //       PRT-A: {{^ *}}#pragma acc update self(s) host(h) device(d){{( IF_PRESENT| if_present)?$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target update from([[PRESENT]]s) from([[PRESENT]]h) to([[PRESENT]]d){{$}}
    //       PRT-O: {{^ *}}#pragma omp target update from([[PRESENT]]s) from([[PRESENT]]h) to([[PRESENT]]d){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc update self(s) host(h) device(d){{( IF_PRESENT| if_present)?$}}
    #pragma acc update self(s) host(h) device(d) IF_PRESENT

    //      EXE-caseStructPresent-NEXT:   host s.i=11{{$}}
    //      EXE-caseStructPresent-NEXT:   host s.j=21{{$}}
    //      EXE-caseStructPresent-NEXT:   host h.i=31{{$}}
    //      EXE-caseStructPresent-NEXT:   host h.j=41{{$}}
    //  EXE-OFF-caseStructPresent-NEXT:   host d.i=50{{$}}
    //  EXE-OFF-caseStructPresent-NEXT:   host d.j=60{{$}}
    // EXE-HOST-caseStructPresent-NEXT:   host d.i=51{{$}}
    // EXE-HOST-caseStructPresent-NEXT:   host d.j=61{{$}}
    //      EXE-caseStructPresent-NEXT: device s.i=11{{$}}
    //      EXE-caseStructPresent-NEXT: device s.j=21{{$}}
    //      EXE-caseStructPresent-NEXT: device h.i=31{{$}}
    //      EXE-caseStructPresent-NEXT: device h.j=41{{$}}
    //  EXE-OFF-caseStructPresent-NEXT: device d.i=50{{$}}
    //  EXE-OFF-caseStructPresent-NEXT: device d.j=60{{$}}
    // EXE-HOST-caseStructPresent-NEXT: device d.i=51{{$}}
    // EXE-HOST-caseStructPresent-NEXT: device d.j=61{{$}}
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

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseStructAbsent
// PRT-LABEL: {{.*}}caseStructAbsent{{.*}} {
CASE(caseStructAbsent) {
  struct S { int i; int j; } s, h, d;
  PRINT_VAR_INFO(h);
  PRINT_VAR_INFO(h);

  setHostInt(s.i, 10);
  setHostInt(s.j, 20);
  setHostInt(h.i, 30);
  setHostInt(h.j, 40);
  setHostInt(d.i, 50);
  setHostInt(d.j, 60);

  // We need multiple directives here so we can control which clause produces
  // the runtime error.  We vary which clause produces the runtime error
  // across the various CASE_* that produce it.
  #pragma acc update host(h) IF_PRESENT
  #pragma acc update device(d) self(s) IF_PRESENT

  // EXE-caseStructAbsent-PASS-NEXT: host s.i=10{{$}}
  // EXE-caseStructAbsent-PASS-NEXT: host s.j=20{{$}}
  // EXE-caseStructAbsent-PASS-NEXT: host h.i=30{{$}}
  // EXE-caseStructAbsent-PASS-NEXT: host h.j=40{{$}}
  // EXE-caseStructAbsent-PASS-NEXT: host d.i=50{{$}}
  // EXE-caseStructAbsent-PASS-NEXT: host d.j=60{{$}}
  printHostInt(s.i);
  printHostInt(s.j);
  printHostInt(h.i);
  printHostInt(h.j);
  printHostInt(d.i);
  printHostInt(d.j);
}

//------------------------------------------------------------------------------
// Check array.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseArrayPresent
// PRT-LABEL: {{.*}}caseArrayPresent{{.*}} {
CASE(caseArrayPresent) {
  int s[2];
  int h[2];
  int d[2];
  PRINT_VAR_INFO(s);
  PRINT_VAR_INFO(s);

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

    //                 DMP: ACCUpdateDirective
    //            DMP-NEXT:   ACCSelfClause
    //            DMP-NEXT:     DeclRefExpr {{.*}} 's' 'int[2]'
    //            DMP-NEXT:   ACCSelfClause
    //            DMP-NEXT:     DeclRefExpr {{.*}} 'h' 'int[2]'
    //            DMP-NEXT:   ACCDeviceClause
    //            DMP-NEXT:     DeclRefExpr {{.*}} 'd' 'int[2]'
    // DMP-IF_PRESENT-NEXT:   ACCIfPresentClause
    //            DMP-NEXT:   impl: OMPTargetUpdateDirective
    //            DMP-NEXT:     OMPFromClause
    //            DMP-NEXT:       DeclRefExpr {{.*}} 's' 'int[2]'
    //            DMP-NEXT:     OMPFromClause
    //            DMP-NEXT:       DeclRefExpr {{.*}} 'h' 'int[2]'
    //            DMP-NEXT:     OMPToClause
    //            DMP-NEXT:       DeclRefExpr {{.*}} 'd' 'int[2]'
    //             DMP-NOT: OMP
    //
    //       PRT-A: {{^ *}}#pragma acc update self(s) host(h) device(d){{( IF_PRESENT| if_present)?$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target update from([[PRESENT]]s) from([[PRESENT]]h) to([[PRESENT]]d){{$}}
    //       PRT-O: {{^ *}}#pragma omp target update from([[PRESENT]]s) from([[PRESENT]]h) to([[PRESENT]]d){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc update self(s) host(h) device(d){{( IF_PRESENT| if_present)?$}}
    #pragma acc update self(s) host(h) device(d) IF_PRESENT

    //      EXE-caseArrayPresent-NEXT:   host s[0]=11{{$}}
    //      EXE-caseArrayPresent-NEXT:   host s[1]=21{{$}}
    //      EXE-caseArrayPresent-NEXT:   host h[0]=31{{$}}
    //      EXE-caseArrayPresent-NEXT:   host h[1]=41{{$}}
    //  EXE-OFF-caseArrayPresent-NEXT:   host d[0]=50{{$}}
    //  EXE-OFF-caseArrayPresent-NEXT:   host d[1]=60{{$}}
    // EXE-HOST-caseArrayPresent-NEXT:   host d[0]=51{{$}}
    // EXE-HOST-caseArrayPresent-NEXT:   host d[1]=61{{$}}
    //      EXE-caseArrayPresent-NEXT: device s[0]=11{{$}}
    //      EXE-caseArrayPresent-NEXT: device s[1]=21{{$}}
    //      EXE-caseArrayPresent-NEXT: device h[0]=31{{$}}
    //      EXE-caseArrayPresent-NEXT: device h[1]=41{{$}}
    //  EXE-OFF-caseArrayPresent-NEXT: device d[0]=50{{$}}
    //  EXE-OFF-caseArrayPresent-NEXT: device d[1]=60{{$}}
    // EXE-HOST-caseArrayPresent-NEXT: device d[0]=51{{$}}
    // EXE-HOST-caseArrayPresent-NEXT: device d[1]=61{{$}}
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

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseArrayAbsent
// PRT-LABEL: {{.*}}caseArrayAbsent{{.*}} {
CASE(caseArrayAbsent) {
  int s[2];
  int h[2];
  int d[2];
  PRINT_VAR_INFO(d);
  PRINT_VAR_INFO(d);

  setHostInt(s[0], 10);
  setHostInt(s[1], 20);
  setHostInt(h[0], 30);
  setHostInt(h[1], 40);
  setHostInt(d[0], 50);
  setHostInt(d[1], 60);

  // We need multiple directives here so we can control which clause produces
  // the runtime error.  We vary which clause produces the runtime error
  // across the various CASE_* that produce it.
  #pragma acc update device(d) IF_PRESENT
  #pragma acc update self(s) host(h) IF_PRESENT

  // EXE-caseArrayAbsent-PASS-NEXT: host s[0]=10{{$}}
  // EXE-caseArrayAbsent-PASS-NEXT: host s[1]=20{{$}}
  // EXE-caseArrayAbsent-PASS-NEXT: host h[0]=30{{$}}
  // EXE-caseArrayAbsent-PASS-NEXT: host h[1]=40{{$}}
  // EXE-caseArrayAbsent-PASS-NEXT: host d[0]=50{{$}}
  // EXE-caseArrayAbsent-PASS-NEXT: host d[1]=60{{$}}
  printHostInt(s[0]);
  printHostInt(s[1]);
  printHostInt(h[0]);
  printHostInt(h[1]);
  printHostInt(d[0]);
  printHostInt(d[1]);
}

//------------------------------------------------------------------------------
// Check struct member not encompassing full struct.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseMemberPresent
// PRT-LABEL: {{.*}}caseMemberPresent{{.*}} {
CASE(caseMemberPresent) {
  struct S { int before; int i; int after; } s, h, d;
  PRINT_VAR_INFO(s);
  PRINT_VAR_INFO(s);

  #pragma acc data create(s.i, h.i, d.i)
  {
    setHostInt(s.i, 10);
    setHostInt(h.i, 20);
    setHostInt(d.i, 30);
    setDeviceInt(s.i, 11);
    setDeviceInt(h.i, 21);
    setDeviceInt(d.i, 31);

    //                 DMP: ACCUpdateDirective
    //            DMP-NEXT:   ACCSelfClause
    //            DMP-NEXT:     MemberExpr {{.* .i .*}}
    //            DMP-NEXT:       DeclRefExpr {{.*}} 's'
    //            DMP-NEXT:   ACCSelfClause
    //            DMP-NEXT:     MemberExpr {{.* .i .*}}
    //            DMP-NEXT:       DeclRefExpr {{.*}} 'h'
    //            DMP-NEXT:   ACCDeviceClause
    //            DMP-NEXT:     MemberExpr {{.* .i .*}}
    //            DMP-NEXT:       DeclRefExpr {{.*}} 'd'
    // DMP-IF_PRESENT-NEXT:   ACCIfPresentClause
    //            DMP-NEXT:   impl: OMPTargetUpdateDirective
    //            DMP-NEXT:     OMPFromClause
    //            DMP-NEXT:       MemberExpr {{.* .i .*}}
    //            DMP-NEXT:         DeclRefExpr {{.*}} 's'
    //            DMP-NEXT:     OMPFromClause
    //            DMP-NEXT:       MemberExpr {{.* .i .*}}
    //            DMP-NEXT:         DeclRefExpr {{.*}} 'h'
    //            DMP-NEXT:     OMPToClause
    //            DMP-NEXT:       MemberExpr {{.* .i .*}}
    //            DMP-NEXT:         DeclRefExpr {{.*}} 'd'
    //             DMP-NOT: OMP
    //
    //       PRT-A: {{^ *}}#pragma acc update self(s.i) host(h.i) device(d.i){{( IF_PRESENT| if_present)?$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target update from([[PRESENT]]s.i) from([[PRESENT]]h.i) to([[PRESENT]]d.i){{$}}
    //       PRT-O: {{^ *}}#pragma omp target update from([[PRESENT]]s.i) from([[PRESENT]]h.i) to([[PRESENT]]d.i){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc update self(s.i) host(h.i) device(d.i){{( IF_PRESENT| if_present)?$}}
    #pragma acc update self(s.i) host(h.i) device(d.i) IF_PRESENT

    //  EXE-OFF-caseMemberPresent-NEXT:   host s.i=11{{$}}
    //  EXE-OFF-caseMemberPresent-NEXT:   host h.i=21{{$}}
    //  EXE-OFF-caseMemberPresent-NEXT:   host d.i=30{{$}}
    // EXE-HOST-caseMemberPresent-NEXT:   host s.i=11{{$}}
    // EXE-HOST-caseMemberPresent-NEXT:   host h.i=21{{$}}
    // EXE-HOST-caseMemberPresent-NEXT:   host d.i=31{{$}}
    //  EXE-OFF-caseMemberPresent-NEXT: device s.i=11{{$}}
    //  EXE-OFF-caseMemberPresent-NEXT: device h.i=21{{$}}
    //  EXE-OFF-caseMemberPresent-NEXT: device d.i=30{{$}}
    // EXE-HOST-caseMemberPresent-NEXT: device s.i=11{{$}}
    // EXE-HOST-caseMemberPresent-NEXT: device h.i=21{{$}}
    // EXE-HOST-caseMemberPresent-NEXT: device d.i=31{{$}}
    printHostInt(s.i);
    printHostInt(h.i);
    printHostInt(d.i);
    printDeviceInt(s.i);
    printDeviceInt(h.i);
    printDeviceInt(d.i);
  }
}

//------------------------------------------------------------------------------
// Check when struct member is not present.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseMemberAbsent
// PRT-LABEL: {{.*}}caseMemberAbsent{{.*}} {
CASE(caseMemberAbsent) {
  struct S { int i; int j; } s, h, d;
  PRINT_VAR_INFO(s.i);
  PRINT_VAR_INFO(s.i);

  setHostInt(s.i, 10);
  setHostInt(s.j, 20);
  setHostInt(h.i, 30);
  setHostInt(h.j, 40);
  setHostInt(d.i, 50);
  setHostInt(d.j, 60);

  // We need multiple directives here so we can control which clause produces
  // the runtime error.  We vary which clause produces the runtime error
  // across the various CASE_* that produce it.
  #pragma acc update self(s.i) IF_PRESENT
  #pragma acc update host(h.i) device(d.i) IF_PRESENT

  // EXE-caseMemberAbsent-PASS-NEXT: host s.i=10{{$}}
  // EXE-caseMemberAbsent-PASS-NEXT: host s.j=20{{$}}
  // EXE-caseMemberAbsent-PASS-NEXT: host h.i=30{{$}}
  // EXE-caseMemberAbsent-PASS-NEXT: host h.j=40{{$}}
  // EXE-caseMemberAbsent-PASS-NEXT: host d.i=50{{$}}
  // EXE-caseMemberAbsent-PASS-NEXT: host d.j=60{{$}}
  printHostInt(s.i);
  printHostInt(s.j);
  printHostInt(h.i);
  printHostInt(h.j);
  printHostInt(d.i);
  printHostInt(d.j);
}

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseMembersDisjoint
// PRT-LABEL: {{.*}}caseMembersDisjoint{{.*}} {
CASE(caseMembersDisjoint) {
  struct S { int i; int j; } s, h, d;
  PRINT_VAR_INFO(s.i);
  PRINT_VAR_INFO(s.j);

  #pragma acc data create(s.i, h.i, d.i)
  {
    setHostInt(s.i, 10);
    setHostInt(s.j, 20);
    setHostInt(h.i, 30);
    setHostInt(h.j, 40);
    setHostInt(d.i, 50);
    setHostInt(d.j, 60);
    setDeviceInt(s.i, 11);
    setDeviceInt(h.i, 31);
    setDeviceInt(d.i, 51);

    // We need multiple directives here so we can control which clause
    // produces the runtime error.  We vary which clause produces the runtime
    // error across the various CASE_* that produce it.
    #pragma acc update self(s.j) IF_PRESENT
    #pragma acc update host(s.j) device(h.j) IF_PRESENT

    //  EXE-OFF-caseMembersDisjoint-PASS-NEXT:   host s.i=10{{$}}
    //  EXE-OFF-caseMembersDisjoint-PASS-NEXT:   host s.j=20{{$}}
    //  EXE-OFF-caseMembersDisjoint-PASS-NEXT:   host h.i=30{{$}}
    //  EXE-OFF-caseMembersDisjoint-PASS-NEXT:   host h.j=40{{$}}
    //  EXE-OFF-caseMembersDisjoint-PASS-NEXT:   host d.i=50{{$}}
    //  EXE-OFF-caseMembersDisjoint-PASS-NEXT:   host d.j=60{{$}}
    // EXE-HOST-caseMembersDisjoint-PASS-NEXT:   host s.i=11{{$}}
    // EXE-HOST-caseMembersDisjoint-PASS-NEXT:   host s.j=20{{$}}
    // EXE-HOST-caseMembersDisjoint-PASS-NEXT:   host h.i=31{{$}}
    // EXE-HOST-caseMembersDisjoint-PASS-NEXT:   host h.j=40{{$}}
    // EXE-HOST-caseMembersDisjoint-PASS-NEXT:   host d.i=51{{$}}
    // EXE-HOST-caseMembersDisjoint-PASS-NEXT:   host d.j=60{{$}}
    //      EXE-caseMembersDisjoint-PASS-NEXT: device s.i=11{{$}}
    //      EXE-caseMembersDisjoint-PASS-NEXT: device h.i=31{{$}}
    //      EXE-caseMembersDisjoint-PASS-NEXT: device d.i=51{{$}}
    printHostInt(s.i);
    printHostInt(s.j);
    printHostInt(h.i);
    printHostInt(h.j);
    printHostInt(d.i);
    printHostInt(d.j);
    printDeviceInt(s.i);
    printDeviceInt(h.i);
    printDeviceInt(d.i);
  }
}

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseMembersConcat2
// PRT-LABEL: {{.*}}caseMembersConcat2{{.*}} {
CASE(caseMembersConcat2) {
  struct S { int i; int j; } s, h, d;
  PRINT_VAR_INFO(h.i);
  PRINT_VAR_INFO(h);

  #pragma acc data create(s.i, h.i, d.i)
  #pragma acc data create(s.j, h.j, d.j)
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

    // We need multiple directives here so we can control which clause
    // produces the runtime error.  We vary which clause produces the runtime
    // error across the various CASE_* that produce it.
    #pragma acc update host(h) IF_PRESENT
    #pragma acc update self(s) device(d) IF_PRESENT

    //  EXE-OFF-caseMembersConcat2-PASS-NEXT:   host s.i=10{{$}}
    //  EXE-OFF-caseMembersConcat2-PASS-NEXT:   host s.j=20{{$}}
    //  EXE-OFF-caseMembersConcat2-PASS-NEXT:   host h.i=30{{$}}
    //  EXE-OFF-caseMembersConcat2-PASS-NEXT:   host h.j=40{{$}}
    //  EXE-OFF-caseMembersConcat2-PASS-NEXT:   host d.i=50{{$}}
    //  EXE-OFF-caseMembersConcat2-PASS-NEXT:   host d.j=60{{$}}
    // EXE-HOST-caseMembersConcat2-PASS-NEXT:   host s.i=11{{$}}
    // EXE-HOST-caseMembersConcat2-PASS-NEXT:   host s.j=21{{$}}
    // EXE-HOST-caseMembersConcat2-PASS-NEXT:   host h.i=31{{$}}
    // EXE-HOST-caseMembersConcat2-PASS-NEXT:   host h.j=41{{$}}
    // EXE-HOST-caseMembersConcat2-PASS-NEXT:   host d.i=51{{$}}
    // EXE-HOST-caseMembersConcat2-PASS-NEXT:   host d.j=61{{$}}
    //      EXE-caseMembersConcat2-PASS-NEXT: device s.i=11{{$}}
    //      EXE-caseMembersConcat2-PASS-NEXT: device s.j=21{{$}}
    //      EXE-caseMembersConcat2-PASS-NEXT: device h.i=31{{$}}
    //      EXE-caseMembersConcat2-PASS-NEXT: device h.j=41{{$}}
    //      EXE-caseMembersConcat2-PASS-NEXT: device d.i=51{{$}}
    //      EXE-caseMembersConcat2-PASS-NEXT: device d.j=61{{$}}
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

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseMemberFullStruct
// PRT-LABEL: {{.*}}caseMemberFullStruct{{.*}} {
CASE(caseMemberFullStruct) {
  struct S { int i; int j; } s, h, d;
  PRINT_VAR_INFO(s.i);
  PRINT_VAR_INFO(s);

  #pragma acc data create(s.i, h.i, d.i)
  {
    setHostInt(s.i, 10);
    setHostInt(s.j, 20);
    setHostInt(h.i, 30);
    setHostInt(h.j, 40);
    setHostInt(d.i, 50);
    setHostInt(d.j, 60);
    setDeviceInt(s.i, 11);
    setDeviceInt(h.i, 31);
    setDeviceInt(d.i, 51);

    // We need multiple directives here so we can control which clause
    // produces the runtime error.  We vary which clause produces the runtime
    // error across the various CASE_* that produce it.
    #pragma acc update self(s) IF_PRESENT
    #pragma acc update device(d) host(h) IF_PRESENT

    //  EXE-OFF-caseMemberFullStruct-PASS-NEXT:   host s.i=10{{$}}
    //  EXE-OFF-caseMemberFullStruct-PASS-NEXT:   host s.j=20{{$}}
    //  EXE-OFF-caseMemberFullStruct-PASS-NEXT:   host h.i=30{{$}}
    //  EXE-OFF-caseMemberFullStruct-PASS-NEXT:   host h.j=40{{$}}
    //  EXE-OFF-caseMemberFullStruct-PASS-NEXT:   host d.i=50{{$}}
    //  EXE-OFF-caseMemberFullStruct-PASS-NEXT:   host d.j=60{{$}}
    // EXE-HOST-caseMemberFullStruct-PASS-NEXT:   host s.i=11{{$}}
    // EXE-HOST-caseMemberFullStruct-PASS-NEXT:   host s.j=20{{$}}
    // EXE-HOST-caseMemberFullStruct-PASS-NEXT:   host h.i=31{{$}}
    // EXE-HOST-caseMemberFullStruct-PASS-NEXT:   host h.j=40{{$}}
    // EXE-HOST-caseMemberFullStruct-PASS-NEXT:   host d.i=51{{$}}
    // EXE-HOST-caseMemberFullStruct-PASS-NEXT:   host d.j=60{{$}}
    //      EXE-caseMemberFullStruct-PASS-NEXT: device s.i=11{{$}}
    //      EXE-caseMemberFullStruct-PASS-NEXT: device h.i=31{{$}}
    //      EXE-caseMemberFullStruct-PASS-NEXT: device d.i=51{{$}}
    printHostInt(s.i);
    printHostInt(s.j);
    printHostInt(h.i);
    printHostInt(h.j);
    printHostInt(d.i);
    printHostInt(d.j);
    printDeviceInt(s.i);
    printDeviceInt(h.i);
    printDeviceInt(d.i);
  }
}

//------------------------------------------------------------------------------
// Check subarray not encompassing full array.
//
// Subarrays are checked thoroughly for acc data and acc parallel in data.c
// parallel-subarray.c.  Check just a few cases here to be sure it basically
// works for acc update too.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseSubarrayPresent
// PRT-LABEL: {{.*}}caseSubarrayPresent{{.*}} {
CASE(caseSubarrayPresent) {
  int s[4];
  int h[4];
  int d[4];
  PRINT_SUBARRAY_INFO(s, 0, 4);
  PRINT_SUBARRAY_INFO(s, 1, 2);

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

    //                 DMP: ACCUpdateDirective
    //            DMP-NEXT:   ACCSelfClause
    //            DMP-NEXT:     OMPArraySectionExpr
    //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    //            DMP-NEXT:         DeclRefExpr {{.*}} 's' 'int[4]'
    //            DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    //            DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    //            DMP-NEXT:       <<<NULL>>>
    //            DMP-NEXT:   ACCSelfClause
    //            DMP-NEXT:     OMPArraySectionExpr
    //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    //            DMP-NEXT:         DeclRefExpr {{.*}} 'h' 'int[4]'
    //            DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    //            DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    //            DMP-NEXT:       <<<NULL>>>
    //            DMP-NEXT:   ACCDeviceClause
    //            DMP-NEXT:     OMPArraySectionExpr
    //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    //            DMP-NEXT:         DeclRefExpr {{.*}} 'd' 'int[4]'
    //            DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
    //            DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2
    //            DMP-NEXT:       <<<NULL>>>
    // DMP-IF_PRESENT-NEXT:   ACCIfPresentClause
    //            DMP-NEXT:   impl: OMPTargetUpdateDirective
    //            DMP-NEXT:     OMPFromClause
    //            DMP-NEXT:       OMPArraySectionExpr
    //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    //            DMP-NEXT:           DeclRefExpr {{.*}} 's' 'int[4]'
    //            DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    //            DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //            DMP-NEXT:         <<<NULL>>>
    //            DMP-NEXT:     OMPFromClause
    //            DMP-NEXT:       OMPArraySectionExpr
    //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    //            DMP-NEXT:           DeclRefExpr {{.*}} 'h' 'int[4]'
    //            DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    //            DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //            DMP-NEXT:         <<<NULL>>>
    //            DMP-NEXT:     OMPToClause
    //            DMP-NEXT:       OMPArraySectionExpr
    //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    //            DMP-NEXT:           DeclRefExpr {{.*}} 'd' 'int[4]'
    //            DMP-NEXT:         IntegerLiteral {{.*}} 'int' 1
    //            DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
    //            DMP-NEXT:         <<<NULL>>>
    //             DMP-NOT: OMP
    //
    //       PRT-A: {{^ *}}#pragma acc update self(s[1:2]) host(h[1:2]) device(d[1:2]){{( IF_PRESENT| if_present)?$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target update from([[PRESENT]]s[1:2]) from([[PRESENT]]h[1:2]) to([[PRESENT]]d[1:2]){{$}}
    //       PRT-O: {{^ *}}#pragma omp target update from([[PRESENT]]s[1:2]) from([[PRESENT]]h[1:2]) to([[PRESENT]]d[1:2]){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc update self(s[1:2]) host(h[1:2]) device(d[1:2]){{( IF_PRESENT| if_present)?$}}
    #pragma acc update self(s[1:2]) host(h[1:2]) device(d[1:2]) IF_PRESENT

    //  EXE-OFF-caseSubarrayPresent-NEXT:   host s[0]=10{{$}}
    //  EXE-OFF-caseSubarrayPresent-NEXT:   host s[1]=21{{$}}
    //  EXE-OFF-caseSubarrayPresent-NEXT:   host s[2]=31{{$}}
    //  EXE-OFF-caseSubarrayPresent-NEXT:   host s[3]=40{{$}}
    //  EXE-OFF-caseSubarrayPresent-NEXT:   host h[0]=50{{$}}
    //  EXE-OFF-caseSubarrayPresent-NEXT:   host h[1]=61{{$}}
    //  EXE-OFF-caseSubarrayPresent-NEXT:   host h[2]=71{{$}}
    //  EXE-OFF-caseSubarrayPresent-NEXT:   host h[3]=80{{$}}
    //  EXE-OFF-caseSubarrayPresent-NEXT:   host d[0]=90{{$}}
    //  EXE-OFF-caseSubarrayPresent-NEXT:   host d[1]=100{{$}}
    //  EXE-OFF-caseSubarrayPresent-NEXT:   host d[2]=110{{$}}
    //  EXE-OFF-caseSubarrayPresent-NEXT:   host d[3]=120{{$}}
    // EXE-HOST-caseSubarrayPresent-NEXT:   host s[0]=11{{$}}
    // EXE-HOST-caseSubarrayPresent-NEXT:   host s[1]=21{{$}}
    // EXE-HOST-caseSubarrayPresent-NEXT:   host s[2]=31{{$}}
    // EXE-HOST-caseSubarrayPresent-NEXT:   host s[3]=41{{$}}
    // EXE-HOST-caseSubarrayPresent-NEXT:   host h[0]=51{{$}}
    // EXE-HOST-caseSubarrayPresent-NEXT:   host h[1]=61{{$}}
    // EXE-HOST-caseSubarrayPresent-NEXT:   host h[2]=71{{$}}
    // EXE-HOST-caseSubarrayPresent-NEXT:   host h[3]=81{{$}}
    // EXE-HOST-caseSubarrayPresent-NEXT:   host d[0]=91{{$}}
    // EXE-HOST-caseSubarrayPresent-NEXT:   host d[1]=101{{$}}
    // EXE-HOST-caseSubarrayPresent-NEXT:   host d[2]=111{{$}}
    // EXE-HOST-caseSubarrayPresent-NEXT:   host d[3]=121{{$}}
    //  EXE-OFF-caseSubarrayPresent-NEXT: device s[0]=11{{$}}
    //  EXE-OFF-caseSubarrayPresent-NEXT: device s[1]=21{{$}}
    //  EXE-OFF-caseSubarrayPresent-NEXT: device s[2]=31{{$}}
    //  EXE-OFF-caseSubarrayPresent-NEXT: device s[3]=41{{$}}
    //  EXE-OFF-caseSubarrayPresent-NEXT: device h[0]=51{{$}}
    //  EXE-OFF-caseSubarrayPresent-NEXT: device h[1]=61{{$}}
    //  EXE-OFF-caseSubarrayPresent-NEXT: device h[2]=71{{$}}
    //  EXE-OFF-caseSubarrayPresent-NEXT: device h[3]=81{{$}}
    //  EXE-OFF-caseSubarrayPresent-NEXT: device d[0]=91{{$}}
    //  EXE-OFF-caseSubarrayPresent-NEXT: device d[1]=100{{$}}
    //  EXE-OFF-caseSubarrayPresent-NEXT: device d[2]=110{{$}}
    //  EXE-OFF-caseSubarrayPresent-NEXT: device d[3]=121{{$}}
    // EXE-HOST-caseSubarrayPresent-NEXT: device s[0]=11{{$}}
    // EXE-HOST-caseSubarrayPresent-NEXT: device s[1]=21{{$}}
    // EXE-HOST-caseSubarrayPresent-NEXT: device s[2]=31{{$}}
    // EXE-HOST-caseSubarrayPresent-NEXT: device s[3]=41{{$}}
    // EXE-HOST-caseSubarrayPresent-NEXT: device h[0]=51{{$}}
    // EXE-HOST-caseSubarrayPresent-NEXT: device h[1]=61{{$}}
    // EXE-HOST-caseSubarrayPresent-NEXT: device h[2]=71{{$}}
    // EXE-HOST-caseSubarrayPresent-NEXT: device h[3]=81{{$}}
    // EXE-HOST-caseSubarrayPresent-NEXT: device d[0]=91{{$}}
    // EXE-HOST-caseSubarrayPresent-NEXT: device d[1]=101{{$}}
    // EXE-HOST-caseSubarrayPresent-NEXT: device d[2]=111{{$}}
    // EXE-HOST-caseSubarrayPresent-NEXT: device d[3]=121{{$}}
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

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseSubarrayPresent2
// PRT-LABEL: {{.*}}caseSubarrayPresent2{{.*}} {
CASE(caseSubarrayPresent2) {
  int s[4];
  int h[4];
  int d[4];
  int start = 1, length = 2;
  PRINT_SUBARRAY_INFO(s, 0, 4);
  PRINT_SUBARRAY_INFO(s, start, length);

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

    //                 DMP: ACCUpdateDirective
    //            DMP-NEXT:   ACCSelfClause
    //            DMP-NEXT:     OMPArraySectionExpr
    //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    //            DMP-NEXT:         DeclRefExpr {{.*}} 's' 'int[4]'
    //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
    //            DMP-NEXT:       `-DeclRefExpr {{.*}} 'start' 'int'
    //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
    //            DMP-NEXT:       `-DeclRefExpr {{.*}} 'length' 'int'
    //            DMP-NEXT:       <<<NULL>>>
    //            DMP-NEXT:   ACCSelfClause
    //            DMP-NEXT:     OMPArraySectionExpr
    //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    //            DMP-NEXT:         DeclRefExpr {{.*}} 'h' 'int[4]'
    //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
    //            DMP-NEXT:       `-DeclRefExpr {{.*}} 'start' 'int'
    //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
    //            DMP-NEXT:       `-DeclRefExpr {{.*}} 'length' 'int'
    //            DMP-NEXT:       <<<NULL>>>
    //            DMP-NEXT:   ACCDeviceClause
    //            DMP-NEXT:     OMPArraySectionExpr
    //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    //            DMP-NEXT:         DeclRefExpr {{.*}} 'd' 'int[4]'
    //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
    //            DMP-NEXT:       `-DeclRefExpr {{.*}} 'start' 'int'
    //            DMP-NEXT:       ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
    //            DMP-NEXT:       `-DeclRefExpr {{.*}} 'length' 'int'
    //            DMP-NEXT:       <<<NULL>>>
    // DMP-IF_PRESENT-NEXT:   ACCIfPresentClause
    //            DMP-NEXT:   impl: OMPTargetUpdateDirective
    //            DMP-NEXT:     OMPFromClause
    //            DMP-NEXT:       OMPArraySectionExpr
    //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    //            DMP-NEXT:           DeclRefExpr {{.*}} 's' 'int[4]'
    //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
    //            DMP-NEXT:         `-DeclRefExpr {{.*}} 'start' 'int'
    //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
    //            DMP-NEXT:         `-DeclRefExpr {{.*}} 'length' 'int'
    //            DMP-NEXT:         <<<NULL>>>
    //            DMP-NEXT:     OMPFromClause
    //            DMP-NEXT:       OMPArraySectionExpr
    //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    //            DMP-NEXT:           DeclRefExpr {{.*}} 'h' 'int[4]'
    //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
    //            DMP-NEXT:         `-DeclRefExpr {{.*}} 'start' 'int'
    //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
    //            DMP-NEXT:         `-DeclRefExpr {{.*}} 'length' 'int'
    //            DMP-NEXT:         <<<NULL>>>
    //            DMP-NEXT:     OMPToClause
    //            DMP-NEXT:       OMPArraySectionExpr
    //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int *' <ArrayToPointerDecay>
    //            DMP-NEXT:           DeclRefExpr {{.*}} 'd' 'int[4]'
    //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
    //            DMP-NEXT:         `-DeclRefExpr {{.*}} 'start' 'int'
    //            DMP-NEXT:         ImplicitCastExpr {{.*}} 'int' <LValueToRValue>
    //            DMP-NEXT:         `-DeclRefExpr {{.*}} 'length' 'int'
    //            DMP-NEXT:         <<<NULL>>>
    //             DMP-NOT: OMP
    //
    //       PRT-A: {{^ *}}#pragma acc update self(s[start:length]) host(h[start:length]) device(d[start:length]){{( IF_PRESENT| if_present)?$}}
    // PRT-AO-NEXT: {{^ *}}// #pragma omp target update from([[PRESENT]]s[start:length]) from([[PRESENT]]h[start:length]) to([[PRESENT]]d[start:length]){{$}}
    //       PRT-O: {{^ *}}#pragma omp target update from([[PRESENT]]s[start:length]) from([[PRESENT]]h[start:length]) to([[PRESENT]]d[start:length]){{$}}
    // PRT-OA-NEXT: {{^ *}}// #pragma acc update self(s[start:length]) host(h[start:length]) device(d[start:length]){{( IF_PRESENT| if_present)?$}}
    #pragma acc update self(s[start:length]) host(h[start:length]) device(d[start:length]) IF_PRESENT

    //  EXE-OFF-caseSubarrayPresent2-NEXT:   host s[0]=10{{$}}
    //  EXE-OFF-caseSubarrayPresent2-NEXT:   host s[1]=21{{$}}
    //  EXE-OFF-caseSubarrayPresent2-NEXT:   host s[2]=31{{$}}
    //  EXE-OFF-caseSubarrayPresent2-NEXT:   host s[3]=40{{$}}
    //  EXE-OFF-caseSubarrayPresent2-NEXT:   host h[0]=50{{$}}
    //  EXE-OFF-caseSubarrayPresent2-NEXT:   host h[1]=61{{$}}
    //  EXE-OFF-caseSubarrayPresent2-NEXT:   host h[2]=71{{$}}
    //  EXE-OFF-caseSubarrayPresent2-NEXT:   host h[3]=80{{$}}
    //  EXE-OFF-caseSubarrayPresent2-NEXT:   host d[0]=90{{$}}
    //  EXE-OFF-caseSubarrayPresent2-NEXT:   host d[1]=100{{$}}
    //  EXE-OFF-caseSubarrayPresent2-NEXT:   host d[2]=110{{$}}
    //  EXE-OFF-caseSubarrayPresent2-NEXT:   host d[3]=120{{$}}
    // EXE-HOST-caseSubarrayPresent2-NEXT:   host s[0]=11{{$}}
    // EXE-HOST-caseSubarrayPresent2-NEXT:   host s[1]=21{{$}}
    // EXE-HOST-caseSubarrayPresent2-NEXT:   host s[2]=31{{$}}
    // EXE-HOST-caseSubarrayPresent2-NEXT:   host s[3]=41{{$}}
    // EXE-HOST-caseSubarrayPresent2-NEXT:   host h[0]=51{{$}}
    // EXE-HOST-caseSubarrayPresent2-NEXT:   host h[1]=61{{$}}
    // EXE-HOST-caseSubarrayPresent2-NEXT:   host h[2]=71{{$}}
    // EXE-HOST-caseSubarrayPresent2-NEXT:   host h[3]=81{{$}}
    // EXE-HOST-caseSubarrayPresent2-NEXT:   host d[0]=91{{$}}
    // EXE-HOST-caseSubarrayPresent2-NEXT:   host d[1]=101{{$}}
    // EXE-HOST-caseSubarrayPresent2-NEXT:   host d[2]=111{{$}}
    // EXE-HOST-caseSubarrayPresent2-NEXT:   host d[3]=121{{$}}
    //  EXE-OFF-caseSubarrayPresent2-NEXT: device s[0]=11{{$}}
    //  EXE-OFF-caseSubarrayPresent2-NEXT: device s[1]=21{{$}}
    //  EXE-OFF-caseSubarrayPresent2-NEXT: device s[2]=31{{$}}
    //  EXE-OFF-caseSubarrayPresent2-NEXT: device s[3]=41{{$}}
    //  EXE-OFF-caseSubarrayPresent2-NEXT: device h[0]=51{{$}}
    //  EXE-OFF-caseSubarrayPresent2-NEXT: device h[1]=61{{$}}
    //  EXE-OFF-caseSubarrayPresent2-NEXT: device h[2]=71{{$}}
    //  EXE-OFF-caseSubarrayPresent2-NEXT: device h[3]=81{{$}}
    //  EXE-OFF-caseSubarrayPresent2-NEXT: device d[0]=91{{$}}
    //  EXE-OFF-caseSubarrayPresent2-NEXT: device d[1]=100{{$}}
    //  EXE-OFF-caseSubarrayPresent2-NEXT: device d[2]=110{{$}}
    //  EXE-OFF-caseSubarrayPresent2-NEXT: device d[3]=121{{$}}
    // EXE-HOST-caseSubarrayPresent2-NEXT: device s[0]=11{{$}}
    // EXE-HOST-caseSubarrayPresent2-NEXT: device s[1]=21{{$}}
    // EXE-HOST-caseSubarrayPresent2-NEXT: device s[2]=31{{$}}
    // EXE-HOST-caseSubarrayPresent2-NEXT: device s[3]=41{{$}}
    // EXE-HOST-caseSubarrayPresent2-NEXT: device h[0]=51{{$}}
    // EXE-HOST-caseSubarrayPresent2-NEXT: device h[1]=61{{$}}
    // EXE-HOST-caseSubarrayPresent2-NEXT: device h[2]=71{{$}}
    // EXE-HOST-caseSubarrayPresent2-NEXT: device h[3]=81{{$}}
    // EXE-HOST-caseSubarrayPresent2-NEXT: device d[0]=91{{$}}
    // EXE-HOST-caseSubarrayPresent2-NEXT: device d[1]=101{{$}}
    // EXE-HOST-caseSubarrayPresent2-NEXT: device d[2]=111{{$}}
    // EXE-HOST-caseSubarrayPresent2-NEXT: device d[3]=121{{$}}
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

//------------------------------------------------------------------------------
// Check when subarray is not present.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseSubarrayDisjoint
// PRT-LABEL: {{.*}}caseSubarrayDisjoint{{.*}} {
CASE(caseSubarrayDisjoint) {
  int s[4];
  int h[4];
  int d[4];
  PRINT_SUBARRAY_INFO(s, 0, 2);
  PRINT_SUBARRAY_INFO(s, 2, 2);

  #pragma acc data create(s[0:2], h[0:2], d[0:2])
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
    setDeviceInt(h[0], 51);
    setDeviceInt(h[1], 61);
    setDeviceInt(d[0], 91);
    setDeviceInt(d[1], 101);

    // We need multiple directives here so we can control which clause
    // produces the runtime error.  We vary which clause produces the runtime
    // error across the various CASE_* that produce it.
    #pragma acc update self(s[2:2]) IF_PRESENT
    #pragma acc update host(s[2:2]) device(h[2:2]) IF_PRESENT

    //  EXE-OFF-caseSubarrayDisjoint-PASS-NEXT:   host s[0]=10{{$}}
    //  EXE-OFF-caseSubarrayDisjoint-PASS-NEXT:   host s[1]=20{{$}}
    //  EXE-OFF-caseSubarrayDisjoint-PASS-NEXT:   host s[2]=30{{$}}
    //  EXE-OFF-caseSubarrayDisjoint-PASS-NEXT:   host s[3]=40{{$}}
    //  EXE-OFF-caseSubarrayDisjoint-PASS-NEXT:   host h[0]=50{{$}}
    //  EXE-OFF-caseSubarrayDisjoint-PASS-NEXT:   host h[1]=60{{$}}
    //  EXE-OFF-caseSubarrayDisjoint-PASS-NEXT:   host h[2]=70{{$}}
    //  EXE-OFF-caseSubarrayDisjoint-PASS-NEXT:   host h[3]=80{{$}}
    //  EXE-OFF-caseSubarrayDisjoint-PASS-NEXT:   host d[0]=90{{$}}
    //  EXE-OFF-caseSubarrayDisjoint-PASS-NEXT:   host d[1]=100{{$}}
    //  EXE-OFF-caseSubarrayDisjoint-PASS-NEXT:   host d[2]=110{{$}}
    //  EXE-OFF-caseSubarrayDisjoint-PASS-NEXT:   host d[3]=120{{$}}
    // EXE-HOST-caseSubarrayDisjoint-PASS-NEXT:   host s[0]=11{{$}}
    // EXE-HOST-caseSubarrayDisjoint-PASS-NEXT:   host s[1]=21{{$}}
    // EXE-HOST-caseSubarrayDisjoint-PASS-NEXT:   host s[2]=30{{$}}
    // EXE-HOST-caseSubarrayDisjoint-PASS-NEXT:   host s[3]=40{{$}}
    // EXE-HOST-caseSubarrayDisjoint-PASS-NEXT:   host h[0]=51{{$}}
    // EXE-HOST-caseSubarrayDisjoint-PASS-NEXT:   host h[1]=61{{$}}
    // EXE-HOST-caseSubarrayDisjoint-PASS-NEXT:   host h[2]=70{{$}}
    // EXE-HOST-caseSubarrayDisjoint-PASS-NEXT:   host h[3]=80{{$}}
    // EXE-HOST-caseSubarrayDisjoint-PASS-NEXT:   host d[0]=91{{$}}
    // EXE-HOST-caseSubarrayDisjoint-PASS-NEXT:   host d[1]=101{{$}}
    // EXE-HOST-caseSubarrayDisjoint-PASS-NEXT:   host d[2]=110{{$}}
    // EXE-HOST-caseSubarrayDisjoint-PASS-NEXT:   host d[3]=120{{$}}
    //      EXE-caseSubarrayDisjoint-PASS-NEXT: device s[0]=11{{$}}
    //      EXE-caseSubarrayDisjoint-PASS-NEXT: device s[1]=21{{$}}
    //      EXE-caseSubarrayDisjoint-PASS-NEXT: device h[0]=51{{$}}
    //      EXE-caseSubarrayDisjoint-PASS-NEXT: device h[1]=61{{$}}
    //      EXE-caseSubarrayDisjoint-PASS-NEXT: device d[0]=91{{$}}
    //      EXE-caseSubarrayDisjoint-PASS-NEXT: device d[1]=101{{$}}
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
    printDeviceInt(h[0]);
    printDeviceInt(h[1]);
    printDeviceInt(d[0]);
    printDeviceInt(d[1]);
  }
}

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseSubarrayOverlapStart
// PRT-LABEL: {{.*}}caseSubarrayOverlapStart{{.*}} {
CASE(caseSubarrayOverlapStart) {
  int s[4];
  int h[4];
  int d[4];
  PRINT_SUBARRAY_INFO(h, 1, 2);
  PRINT_SUBARRAY_INFO(h, 0, 3);

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

    // We need multiple directives here so we can control which clause
    // produces the runtime error.  We vary which clause produces the runtime
    // error across the various CASE_* that produce it.
    #pragma acc update host(h[0:3]) IF_PRESENT
    #pragma acc update device(d[0:3]) self(s[0:3]) IF_PRESENT

    //  EXE-OFF-caseSubarrayOverlapStart-PASS-NEXT:   host s[0]=10{{$}}
    //  EXE-OFF-caseSubarrayOverlapStart-PASS-NEXT:   host s[1]=20{{$}}
    //  EXE-OFF-caseSubarrayOverlapStart-PASS-NEXT:   host s[2]=30{{$}}
    //  EXE-OFF-caseSubarrayOverlapStart-PASS-NEXT:   host s[3]=40{{$}}
    //  EXE-OFF-caseSubarrayOverlapStart-PASS-NEXT:   host h[0]=50{{$}}
    //  EXE-OFF-caseSubarrayOverlapStart-PASS-NEXT:   host h[1]=60{{$}}
    //  EXE-OFF-caseSubarrayOverlapStart-PASS-NEXT:   host h[2]=70{{$}}
    //  EXE-OFF-caseSubarrayOverlapStart-PASS-NEXT:   host h[3]=80{{$}}
    //  EXE-OFF-caseSubarrayOverlapStart-PASS-NEXT:   host d[0]=90{{$}}
    //  EXE-OFF-caseSubarrayOverlapStart-PASS-NEXT:   host d[1]=100{{$}}
    //  EXE-OFF-caseSubarrayOverlapStart-PASS-NEXT:   host d[2]=110{{$}}
    //  EXE-OFF-caseSubarrayOverlapStart-PASS-NEXT:   host d[3]=120{{$}}
    // EXE-HOST-caseSubarrayOverlapStart-PASS-NEXT:   host s[0]=10{{$}}
    // EXE-HOST-caseSubarrayOverlapStart-PASS-NEXT:   host s[1]=21{{$}}
    // EXE-HOST-caseSubarrayOverlapStart-PASS-NEXT:   host s[2]=31{{$}}
    // EXE-HOST-caseSubarrayOverlapStart-PASS-NEXT:   host s[3]=40{{$}}
    // EXE-HOST-caseSubarrayOverlapStart-PASS-NEXT:   host h[0]=50{{$}}
    // EXE-HOST-caseSubarrayOverlapStart-PASS-NEXT:   host h[1]=61{{$}}
    // EXE-HOST-caseSubarrayOverlapStart-PASS-NEXT:   host h[2]=71{{$}}
    // EXE-HOST-caseSubarrayOverlapStart-PASS-NEXT:   host h[3]=80{{$}}
    // EXE-HOST-caseSubarrayOverlapStart-PASS-NEXT:   host d[0]=90{{$}}
    // EXE-HOST-caseSubarrayOverlapStart-PASS-NEXT:   host d[1]=101{{$}}
    // EXE-HOST-caseSubarrayOverlapStart-PASS-NEXT:   host d[2]=111{{$}}
    // EXE-HOST-caseSubarrayOverlapStart-PASS-NEXT:   host d[3]=120{{$}}
    //      EXE-caseSubarrayOverlapStart-PASS-NEXT: device s[1]=21{{$}}
    //      EXE-caseSubarrayOverlapStart-PASS-NEXT: device s[2]=31{{$}}
    //      EXE-caseSubarrayOverlapStart-PASS-NEXT: device h[1]=61{{$}}
    //      EXE-caseSubarrayOverlapStart-PASS-NEXT: device h[2]=71{{$}}
    //      EXE-caseSubarrayOverlapStart-PASS-NEXT: device d[1]=101{{$}}
    //      EXE-caseSubarrayOverlapStart-PASS-NEXT: device d[2]=111{{$}}
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

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseSubarrayOverlapEnd
// PRT-LABEL: {{.*}}caseSubarrayOverlapEnd{{.*}} {
CASE(caseSubarrayOverlapEnd) {
  int s[4];
  int h[4];
  int d[4];
  PRINT_SUBARRAY_INFO(d, 1, 2);
  PRINT_SUBARRAY_INFO(d, 2, 2);

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

    // We need multiple directives here so we can control which clause
    // produces the runtime error.  We vary which clause produces the runtime
    // error across the various CASE_* that produce it.
    #pragma acc update device(d[2:2]) IF_PRESENT
    #pragma acc update self(s[2:2]) host(h[2:2]) IF_PRESENT

    //  EXE-OFF-caseSubarrayOverlapEnd-PASS-NEXT:   host s[0]=10{{$}}
    //  EXE-OFF-caseSubarrayOverlapEnd-PASS-NEXT:   host s[1]=20{{$}}
    //  EXE-OFF-caseSubarrayOverlapEnd-PASS-NEXT:   host s[2]=30{{$}}
    //  EXE-OFF-caseSubarrayOverlapEnd-PASS-NEXT:   host s[3]=40{{$}}
    //  EXE-OFF-caseSubarrayOverlapEnd-PASS-NEXT:   host h[0]=50{{$}}
    //  EXE-OFF-caseSubarrayOverlapEnd-PASS-NEXT:   host h[1]=60{{$}}
    //  EXE-OFF-caseSubarrayOverlapEnd-PASS-NEXT:   host h[2]=70{{$}}
    //  EXE-OFF-caseSubarrayOverlapEnd-PASS-NEXT:   host h[3]=80{{$}}
    //  EXE-OFF-caseSubarrayOverlapEnd-PASS-NEXT:   host d[0]=90{{$}}
    //  EXE-OFF-caseSubarrayOverlapEnd-PASS-NEXT:   host d[1]=100{{$}}
    //  EXE-OFF-caseSubarrayOverlapEnd-PASS-NEXT:   host d[2]=110{{$}}
    //  EXE-OFF-caseSubarrayOverlapEnd-PASS-NEXT:   host d[3]=120{{$}}
    // EXE-HOST-caseSubarrayOverlapEnd-PASS-NEXT:   host s[0]=10{{$}}
    // EXE-HOST-caseSubarrayOverlapEnd-PASS-NEXT:   host s[1]=21{{$}}
    // EXE-HOST-caseSubarrayOverlapEnd-PASS-NEXT:   host s[2]=31{{$}}
    // EXE-HOST-caseSubarrayOverlapEnd-PASS-NEXT:   host s[3]=40{{$}}
    // EXE-HOST-caseSubarrayOverlapEnd-PASS-NEXT:   host h[0]=50{{$}}
    // EXE-HOST-caseSubarrayOverlapEnd-PASS-NEXT:   host h[1]=61{{$}}
    // EXE-HOST-caseSubarrayOverlapEnd-PASS-NEXT:   host h[2]=71{{$}}
    // EXE-HOST-caseSubarrayOverlapEnd-PASS-NEXT:   host h[3]=80{{$}}
    // EXE-HOST-caseSubarrayOverlapEnd-PASS-NEXT:   host d[0]=90{{$}}
    // EXE-HOST-caseSubarrayOverlapEnd-PASS-NEXT:   host d[1]=101{{$}}
    // EXE-HOST-caseSubarrayOverlapEnd-PASS-NEXT:   host d[2]=111{{$}}
    // EXE-HOST-caseSubarrayOverlapEnd-PASS-NEXT:   host d[3]=120{{$}}
    //      EXE-caseSubarrayOverlapEnd-PASS-NEXT: device s[1]=21{{$}}
    //      EXE-caseSubarrayOverlapEnd-PASS-NEXT: device s[2]=31{{$}}
    //      EXE-caseSubarrayOverlapEnd-PASS-NEXT: device h[1]=61{{$}}
    //      EXE-caseSubarrayOverlapEnd-PASS-NEXT: device h[2]=71{{$}}
    //      EXE-caseSubarrayOverlapEnd-PASS-NEXT: device d[1]=101{{$}}
    //      EXE-caseSubarrayOverlapEnd-PASS-NEXT: device d[2]=111{{$}}
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

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseSubarrayConcat2
// PRT-LABEL: {{.*}}caseSubarrayConcat2{{.*}} {
CASE(caseSubarrayConcat2) {
  int s[4];
  int h[4];
  int d[4];
  PRINT_SUBARRAY_INFO(s, 0, 2);
  PRINT_SUBARRAY_INFO(s, 0, 4);

  #pragma acc data create(s[0:2], h[0:2], d[0:2])
  #pragma acc data create(s[2:2], h[2:2], d[2:2])
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

    // We need multiple directives here so we can control which clause
    // produces the runtime error.  We vary which clause produces the runtime
    // error across the various CASE_* that produce it.
    #pragma acc update self(s[0:4]) IF_PRESENT
    #pragma acc update host(h[0:4]) device(d[0:4]) IF_PRESENT

    //  EXE-OFF-caseSubarrayConcat2-PASS-NEXT:   host s[0]=10{{$}}
    //  EXE-OFF-caseSubarrayConcat2-PASS-NEXT:   host s[1]=20{{$}}
    //  EXE-OFF-caseSubarrayConcat2-PASS-NEXT:   host s[2]=30{{$}}
    //  EXE-OFF-caseSubarrayConcat2-PASS-NEXT:   host s[3]=40{{$}}
    //  EXE-OFF-caseSubarrayConcat2-PASS-NEXT:   host h[0]=50{{$}}
    //  EXE-OFF-caseSubarrayConcat2-PASS-NEXT:   host h[1]=60{{$}}
    //  EXE-OFF-caseSubarrayConcat2-PASS-NEXT:   host h[2]=70{{$}}
    //  EXE-OFF-caseSubarrayConcat2-PASS-NEXT:   host h[3]=80{{$}}
    //  EXE-OFF-caseSubarrayConcat2-PASS-NEXT:   host d[0]=90{{$}}
    //  EXE-OFF-caseSubarrayConcat2-PASS-NEXT:   host d[1]=100{{$}}
    //  EXE-OFF-caseSubarrayConcat2-PASS-NEXT:   host d[2]=110{{$}}
    //  EXE-OFF-caseSubarrayConcat2-PASS-NEXT:   host d[3]=120{{$}}
    // EXE-HOST-caseSubarrayConcat2-PASS-NEXT:   host s[0]=11{{$}}
    // EXE-HOST-caseSubarrayConcat2-PASS-NEXT:   host s[1]=21{{$}}
    // EXE-HOST-caseSubarrayConcat2-PASS-NEXT:   host s[2]=31{{$}}
    // EXE-HOST-caseSubarrayConcat2-PASS-NEXT:   host s[3]=41{{$}}
    // EXE-HOST-caseSubarrayConcat2-PASS-NEXT:   host h[0]=51{{$}}
    // EXE-HOST-caseSubarrayConcat2-PASS-NEXT:   host h[1]=61{{$}}
    // EXE-HOST-caseSubarrayConcat2-PASS-NEXT:   host h[2]=71{{$}}
    // EXE-HOST-caseSubarrayConcat2-PASS-NEXT:   host h[3]=81{{$}}
    // EXE-HOST-caseSubarrayConcat2-PASS-NEXT:   host d[0]=91{{$}}
    // EXE-HOST-caseSubarrayConcat2-PASS-NEXT:   host d[1]=101{{$}}
    // EXE-HOST-caseSubarrayConcat2-PASS-NEXT:   host d[2]=111{{$}}
    // EXE-HOST-caseSubarrayConcat2-PASS-NEXT:   host d[3]=121{{$}}
    //      EXE-caseSubarrayConcat2-PASS-NEXT: device s[0]=11{{$}}
    //      EXE-caseSubarrayConcat2-PASS-NEXT: device s[1]=21{{$}}
    //      EXE-caseSubarrayConcat2-PASS-NEXT: device s[2]=31{{$}}
    //      EXE-caseSubarrayConcat2-PASS-NEXT: device s[3]=41{{$}}
    //      EXE-caseSubarrayConcat2-PASS-NEXT: device h[0]=51{{$}}
    //      EXE-caseSubarrayConcat2-PASS-NEXT: device h[1]=61{{$}}
    //      EXE-caseSubarrayConcat2-PASS-NEXT: device h[2]=71{{$}}
    //      EXE-caseSubarrayConcat2-PASS-NEXT: device h[3]=81{{$}}
    //      EXE-caseSubarrayConcat2-PASS-NEXT: device d[0]=91{{$}}
    //      EXE-caseSubarrayConcat2-PASS-NEXT: device d[1]=101{{$}}
    //      EXE-caseSubarrayConcat2-PASS-NEXT: device d[2]=111{{$}}
    //      EXE-caseSubarrayConcat2-PASS-NEXT: device d[3]=121{{$}}
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

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseSubarrayNonSubarray
// PRT-LABEL: {{.*}}caseSubarrayNonSubarray{{.*}} {
CASE(caseSubarrayNonSubarray) {
  int s[4];
  int h[4];
  int d[4];
  PRINT_SUBARRAY_INFO(h, 1, 2);
  PRINT_SUBARRAY_INFO(h, 0, 4);

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

    // We need multiple directives here so we can control which clause
    // produces the runtime error.  We vary which clause produces the runtime
    // error across the various CASE_* that produce it.
    #pragma acc update host(h) IF_PRESENT
    #pragma acc update device(d) self(s) IF_PRESENT

    //  EXE-OFF-caseSubarrayNonSubarray-PASS-NEXT:   host s[0]=10{{$}}
    //  EXE-OFF-caseSubarrayNonSubarray-PASS-NEXT:   host s[1]=20{{$}}
    //  EXE-OFF-caseSubarrayNonSubarray-PASS-NEXT:   host s[2]=30{{$}}
    //  EXE-OFF-caseSubarrayNonSubarray-PASS-NEXT:   host s[3]=40{{$}}
    //  EXE-OFF-caseSubarrayNonSubarray-PASS-NEXT:   host h[0]=50{{$}}
    //  EXE-OFF-caseSubarrayNonSubarray-PASS-NEXT:   host h[1]=60{{$}}
    //  EXE-OFF-caseSubarrayNonSubarray-PASS-NEXT:   host h[2]=70{{$}}
    //  EXE-OFF-caseSubarrayNonSubarray-PASS-NEXT:   host h[3]=80{{$}}
    //  EXE-OFF-caseSubarrayNonSubarray-PASS-NEXT:   host d[0]=90{{$}}
    //  EXE-OFF-caseSubarrayNonSubarray-PASS-NEXT:   host d[1]=100{{$}}
    //  EXE-OFF-caseSubarrayNonSubarray-PASS-NEXT:   host d[2]=110{{$}}
    //  EXE-OFF-caseSubarrayNonSubarray-PASS-NEXT:   host d[3]=120{{$}}
    // EXE-HOST-caseSubarrayNonSubarray-PASS-NEXT:   host s[0]=10{{$}}
    // EXE-HOST-caseSubarrayNonSubarray-PASS-NEXT:   host s[1]=21{{$}}
    // EXE-HOST-caseSubarrayNonSubarray-PASS-NEXT:   host s[2]=31{{$}}
    // EXE-HOST-caseSubarrayNonSubarray-PASS-NEXT:   host s[3]=40{{$}}
    // EXE-HOST-caseSubarrayNonSubarray-PASS-NEXT:   host h[0]=50{{$}}
    // EXE-HOST-caseSubarrayNonSubarray-PASS-NEXT:   host h[1]=61{{$}}
    // EXE-HOST-caseSubarrayNonSubarray-PASS-NEXT:   host h[2]=71{{$}}
    // EXE-HOST-caseSubarrayNonSubarray-PASS-NEXT:   host h[3]=80{{$}}
    // EXE-HOST-caseSubarrayNonSubarray-PASS-NEXT:   host d[0]=90{{$}}
    // EXE-HOST-caseSubarrayNonSubarray-PASS-NEXT:   host d[1]=101{{$}}
    // EXE-HOST-caseSubarrayNonSubarray-PASS-NEXT:   host d[2]=111{{$}}
    // EXE-HOST-caseSubarrayNonSubarray-PASS-NEXT:   host d[3]=120{{$}}
    //      EXE-caseSubarrayNonSubarray-PASS-NEXT: device s[1]=21{{$}}
    //      EXE-caseSubarrayNonSubarray-PASS-NEXT: device s[2]=31{{$}}
    //      EXE-caseSubarrayNonSubarray-PASS-NEXT: device h[1]=61{{$}}
    //      EXE-caseSubarrayNonSubarray-PASS-NEXT: device h[2]=71{{$}}
    //      EXE-caseSubarrayNonSubarray-PASS-NEXT: device d[1]=101{{$}}
    //      EXE-caseSubarrayNonSubarray-PASS-NEXT: device d[2]=111{{$}}
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

// EXE-notPASS-NEXT: Libomptarget message: device mapping required by 'present' motion modifier does not exist for host address 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes)
// EXE-notPASS-NEXT: Libomptarget error: Consult {{.*}}
// EXE-notPASS-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                   # An abort message usually follows.
//  EXE-notPASS-NOT: Libomptarget
//     EXE-PASS-NOT: {{.}}
