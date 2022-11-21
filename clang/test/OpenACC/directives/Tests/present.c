// Check present clauses on different constructs and with different values of
// -fopenacc-present-omp.
//
// Diagnostics about the present map type modifier in the translation are
// checked in diagnostics/warn-acc-omp-map-present.c.  Diagnostics about bad
// -fopenacc-present-omp arguments are checked in
// diagnostics/fopenacc-present-omp.c.

// Redefine this to specify how %{check-cases} expands for each case.
//
// - LINE = the line number of the caller.
// - CASE = the case name used in the enum and as a command line argument.
// - CONSTRUCT = the name of the OpenACC construct being tested.
// - NOT_CRASH_IF_FAIL = 'not --crash' if the case is expected to fail a
//   presence check or array extension check, and the empty string otherwise.
// - NOT_IF_FAIL = same as NOT_CRASH_IF_FAIL except without the '--crash'.  It's
//   the logical-or of NOT_IF_PRESENT_FAIL and NOT_IF_ARRAYEXT_FAIL.
// - NOT_IF_PRESENT_FAIL = same as NOT_IF_FAIL but only for presence checks.
// - NOT_IF_ARRAYEXT_FAIL = same as NOT_IF_FAIL but only for array extension
//   checks.
//
// DEFINE: %{check-case}( LINE %, CASE %, CONSTRUCT %, NOT_CRASH_IF_FAIL %,    \
// DEFINE:                NOT_IF_FAIL %, NOT_IF_PRESENT_FAIL %,                \
// DEFINE:                NOT_IF_ARRAYEXT_FAIL  %) =

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
// The various cases covered here should be kept consistent with no-create.c,
// update.c, and data-extension-errors.c (the last is located in
// openmp/libacc2omp/test/directives).  For example, a subarray that extends a
// subarray already present is consistently considered not present, so the
// present clause produces a runtime error and the no_create clause doesn't
// allocate.
//
// Due to a bug in Clang's OpenMP implementation, codegen and runtime behavior
// used to behave differently for "present" clauses on "acc data" vs. "acc
// parallel" if -fopenacc-present-omp=no-present were specified, so check all
// interesting cases for each.  Specifically, without the "present" modifier, a
// map type for an unused variable within "omp target teams" was dropped by
// Clang, so there was no runtime error for collisions with prior mappings.
// However, in the case of "omp target data", a variable in a map clause was
// always mapped, so such runtime errors did occur.  Both now behave like the
// latter.
// 
// "acc parallel loop" should be about the same as "acc parallel", so a few
// cases are probably sufficient.
//
// DEFINE: %{check-cases}( LINE %, NOT_CRASH_IF_PRESENT %, NOT_IF_PRESENT %) =                                                                                                                                                              \
//                          LINE                CASE                                CONSTRUCT    NOT_CRASH_IF_FAIL                         NOT_IF_FAIL                         NOT_IF_PRESENT_FAIL                 NOT_IF_ARRAYEXT_FAIL
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseDataScalarPresent            %, data      %,                                        %,                                  %,                                  %,                    %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseDataScalarAbsent             %, data      %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %,                    %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseDataStructPresent            %, data      %,                                        %,                                  %,                                  %,                    %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseDataStructAbsent             %, data      %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %,                    %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseDataArrayPresent             %, data      %,                                        %,                                  %,                                  %,                    %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseDataArrayAbsent              %, data      %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %,                    %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseDataMemberPresent            %, data      %,                                        %,                                  %,                                  %,                    %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseDataMemberAbsent             %, data      %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %,                    %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseDataMembersDisjoint          %, data      %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %,                    %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseDataMembersConcat2           %, data      %, %if-tgt-host<|not --crash>             %, %if-tgt-host<|not>               %, %if-tgt-host<|%{NOT_IF_PRESENT}> %, %if-tgt-host<|not> %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseDataMemberFullStruct         %, data      %, %if-tgt-host<|not --crash>             %, %if-tgt-host<|not>               %, %if-tgt-host<|%{NOT_IF_PRESENT}> %, %if-tgt-host<|not> %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseDataSubarrayPresent          %, data      %,                                        %,                                  %,                                  %,                    %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseDataSubarrayDisjoint         %, data      %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %,                    %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseDataSubarrayOverlapStart     %, data      %, %if-tgt-host<|not --crash>             %, %if-tgt-host<|not>               %, %if-tgt-host<|%{NOT_IF_PRESENT}> %, %if-tgt-host<|not> %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseDataSubarrayOverlapEnd       %, data      %, %if-tgt-host<|not --crash>             %, %if-tgt-host<|not>               %, %if-tgt-host<|%{NOT_IF_PRESENT}> %, %if-tgt-host<|not> %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseDataSubarrayConcat2          %, data      %, %if-tgt-host<|not --crash>             %, %if-tgt-host<|not>               %, %if-tgt-host<|%{NOT_IF_PRESENT}> %, %if-tgt-host<|not> %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseDataSubarrayNonSubarray      %, data      %, %if-tgt-host<|not --crash>             %, %if-tgt-host<|not>               %, %if-tgt-host<|%{NOT_IF_PRESENT}> %, %if-tgt-host<|not> %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseParallelScalarPresent        %, parallel  %,                                        %,                                  %,                                  %,                    %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseParallelScalarAbsent         %, parallel  %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %,                    %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseParallelStructPresent        %, parallel  %,                                        %,                                  %,                                  %,                    %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseParallelStructAbsent         %, parallel  %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %,                    %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseParallelArrayPresent         %, parallel  %,                                        %,                                  %,                                  %,                    %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseParallelArrayAbsent          %, parallel  %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %,                    %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseParallelMemberPresent        %, parallel  %,                                        %,                                  %,                                  %,                    %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseParallelMemberAbsent         %, parallel  %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %,                    %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseParallelMembersDisjoint      %, parallel  %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %,                    %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseParallelMembersConcat2       %, parallel  %, %if-tgt-host<|not --crash>             %, %if-tgt-host<|not>               %, %if-tgt-host<|%{NOT_IF_PRESENT}> %, %if-tgt-host<|not> %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseParallelMemberFullStruct     %, parallel  %, %if-tgt-host<|not --crash>             %, %if-tgt-host<|not>               %, %if-tgt-host<|%{NOT_IF_PRESENT}> %, %if-tgt-host<|not> %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseParallelSubarrayPresent      %, parallel  %,                                        %,                                  %,                                  %,                    %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseParallelSubarrayDisjoint     %, parallel  %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %,                    %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseParallelSubarrayOverlapStart %, parallel  %, %if-tgt-host<|not --crash>             %, %if-tgt-host<|not>               %, %if-tgt-host<|%{NOT_IF_PRESENT}> %, %if-tgt-host<|not> %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseParallelSubarrayOverlapEnd   %, parallel  %, %if-tgt-host<|not --crash>             %, %if-tgt-host<|not>               %, %if-tgt-host<|%{NOT_IF_PRESENT}> %, %if-tgt-host<|not> %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseParallelSubarrayConcat2      %, parallel  %, %if-tgt-host<|not --crash>             %, %if-tgt-host<|not>               %, %if-tgt-host<|%{NOT_IF_PRESENT}> %, %if-tgt-host<|not> %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseParallelSubarrayNonSubarray  %, parallel  %, %if-tgt-host<|not --crash>             %, %if-tgt-host<|not>               %, %if-tgt-host<|%{NOT_IF_PRESENT}> %, %if-tgt-host<|not> %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseParallelLoopScalarPresent    %, parallel  %,                                        %,                                  %,                                  %,                    %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseParallelLoopScalarAbsent     %, parallel  %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %,                    %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseConstPresent                 %, parallel  %,                                        %,                                  %,                                  %,                    %) && \
// DEFINE:   %{check-case}( %{LINE}, %(line) %, caseConstAbsent                  %, parallel  %, %if-tgt-host<|%{NOT_CRASH_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %, %if-tgt-host<|%{NOT_IF_PRESENT}> %,                    %)

// Generate the enum of cases.
//
//      RUN: echo '#define FOREACH_CASE(Macro) \' > %t-cases.h
// REDEFINE: %{check-case}( LINE %, CASE %, CONSTRUCT %, NOT_CRASH_IF_FAIL %,  \
// REDEFINE:                NOT_IF_FAIL %, NOT_IF_PRESENT_FAIL %,              \
// REDEFINE:                NOT_IF_ARRAYEXT_FAIL %) =                          \
// REDEFINE: echo '  Macro(%{CASE}) \' >> %t-cases.h
//      RUN: %{check-cases}( %, %, %)
//      RUN: echo '  /*end of FOREACH_CASE*/' >> %t-cases.h

// Prepare substitutions for trying all cases many times while varying the value
// of -fopenacc-present-omp and whether the variable is used in the construct.
//
// REDEFINE: %{all:clang:args-stable} = \
// REDEFINE:   -DCASES_HEADER='"%t-cases.h"' -gline-tables-only
// REDEFINE: %{check-case}( LINE %, CASE %, CONSTRUCT %, NOT_CRASH_IF_FAIL %,  \
// REDEFINE:                NOT_IF_FAIL %, NOT_IF_PRESENT_FAIL %,              \
// REDEFINE:                NOT_IF_ARRAYEXT_FAIL %) =                          \
// REDEFINE:   : '---------- CASE=%{CASE} at lines %{LINE} ----------'     &&  \
// REDEFINE:   %{acc-check-exe-run-fn}( %{NOT_CRASH_IF_FAIL} %, %{CASE} %) &&  \
// REDEFINE:   %{acc-check-exe-filecheck-fn}( %{NOT_IF_FAIL}PASS,%{NOT_IF_FAIL}PASS-%{CONSTRUCT},%{NOT_IF_PRESENT_FAIL}PRESENT,%{NOT_IF_ARRAYEXT_FAIL}ARRAYEXT,%{NOT_IF_PRESENT_FAIL}PRESENT-%{NOT_IF_ARRAYEXT_FAIL}ARRAYEXT %)
// DEFINE: %{check-exe-present}( LINE %) =                                     \
// DEFINE:   %{acc-check-exe-compile} &&                                       \
// DEFINE:   %{check-cases}( %{LINE} %, not --crash %, not %)
// DEFINE: %{check-exe-not-present}( LINE %) =                                 \
// DEFINE:   %{acc-check-exe-compile} &&                                       \
// DEFINE:   %{check-cases}( %{LINE} %, %, %)

// REDEFINE: %{all:clang:args} =
// REDEFINE: %{prt:fc:args} = -DPRESENT_MT=present,ompx_hold,alloc
//      RUN: %{acc-check-dmp}
//      RUN: %{acc-check-prt}
//      RUN: %{check-exe-present}( %(line) %)
// REDEFINE: %{all:clang:args} = -DDO_USE_VAR
//      RUN: %{check-exe-present}( %(line) %)

// REDEFINE: %{all:clang:args} = -fopenacc-present-omp=present
// REDEFINE: %{prt:fc:args} = -DPRESENT_MT=present,ompx_hold,alloc
//      RUN: %{acc-check-dmp}
//      RUN: %{acc-check-prt}
//      RUN: %{check-exe-present}( %(line) %)
// REDEFINE: %{all:clang:args} = -fopenacc-present-omp=present -DDO_USE_VAR
//      RUN: %{check-exe-present}( %(line) %)

// REDEFINE: %{all:clang:args} = -fopenacc-present-omp=no-present
// REDEFINE: %{prt:fc:args} = -DPRESENT_MT=ompx_hold,alloc
//      RUN: %{acc-check-dmp}
//      RUN: %{acc-check-prt}
//      RUN: %{check-exe-not-present}( %(line) %)
// REDEFINE: %{all:clang:args} = -fopenacc-present-omp=no-present -DDO_USE_VAR
//      RUN: %{check-exe-not-present}( %(line) %)

// END.

/* expected-no-diagnostics */

#include <stdio.h>
#include <string.h>

#ifdef DO_USE_VAR
# define USE_VAR(X) X
#else
# define USE_VAR(X)
#endif

#define PRINT_VAR_INFO(Var) \
  fprintf(stderr, "addr=%p, size=%ld\n", &(Var), sizeof (Var))

#define PRINT_SUBARRAY_INFO(Arr, Start, Length) \
  fprintf(stderr, "addr=%p, size=%ld\n", &(Arr)[Start], \
          Length * sizeof (Arr[0]))

// Make each static to ensure we get a compile warning if it's never called.
#include CASES_HEADER
#define CASE(CaseName) static void CaseName(void)
#define AddCase(CaseName) CASE(CaseName);
FOREACH_CASE(AddCase)
#undef AddCase

//                      EXE-NOT: {{.}}
//                          EXE: start
//                     EXE-NEXT: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//                     EXE-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//         EXE-notARRAYEXT-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//          EXE-notPRESENT-NEXT: Libomptarget message: device mapping required by 'present' map type modifier does not exist for host address 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes)
//                                   # FIXME: Names like getTargetPointer are meaningless to users.
//          EXE-notPRESENT-NEXT: Libomptarget error: Call to getTargetPointer returned null pointer ('present' map type modifier).
// EXE-PRESENT-notARRAYEXT-NEXT: Libomptarget error: Call to getTargetPointer returned null pointer (device failure or illegal mapping)
//    EXE-notPASS-parallel-NEXT: Libomptarget error: Call to targetDataBegin failed, abort target.
//    EXE-notPASS-parallel-NEXT: Libomptarget error: Failed to process data before launching the kernel.
//             EXE-notPASS-NEXT: Libomptarget error: Consult {{.*}}
//             EXE-notPASS-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                   # An abort message usually follows.
//              EXE-notPASS-NOT: Libomptarget
//                EXE-PASS-NEXT: end
//                 EXE-PASS-NOT: {{.}}
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

  printf("start\n");
  fflush(stdout);
  caseFn();
  printf("end\n");
  fflush(stdout);
  return 0;
}

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseDataScalarPresent
//       DMP: ACCDataDirective
//  DMP-NEXT:   ACCCopyClause
//  DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:   impl: OMPTargetDataDirective
//  DMP-NEXT:     OMPMapClause
//  DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'int'
//       DMP:   ACCDataDirective
//  DMP-NEXT:     ACCPresentClause
//  DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:     impl: OMPTargetDataDirective
//  DMP-NEXT:       OMPMapClause
//  DMP-NEXT:         DeclRefExpr {{.*}} 'x' 'int'
//
//   PRT-LABEL: {{.*}}caseDataScalarPresent{{.*}} {
//    PRT-NEXT:   int x;
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//
//  PRT-A-NEXT:   #pragma acc data copy(x){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: x){{$}}
//  PRT-A-NEXT:   #pragma acc data present(x){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[PRESENT_MT]]: x){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: x){{$}}
// PRT-OA-NEXT:   // #pragma acc data copy(x){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[PRESENT_MT]]: x){{$}}
// PRT-OA-NEXT:   // #pragma acc data present(x){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
CASE(caseDataScalarPresent) {
  int x;
  PRINT_VAR_INFO(x);
  PRINT_VAR_INFO(x);
  #pragma acc data copy(x)
  #pragma acc data present(x)
  USE_VAR(x = 1);
}

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseDataScalarAbsent
//       DMP: ACCDataDirective
//  DMP-NEXT:   ACCPresentClause
//  DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:   impl: OMPTargetDataDirective
//  DMP-NEXT:     OMPMapClause
//  DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'int'
//
//   PRT-LABEL: {{.*}}caseDataScalarAbsent{{.*}} {
//    PRT-NEXT:   int x;
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//  PRT-A-NEXT:   #pragma acc data present(x){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[PRESENT_MT]]: x){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[PRESENT_MT]]: x){{$}}
// PRT-OA-NEXT:   // #pragma acc data present(x){{$}}
//    PRT-NEXT:   ;
//    PRT-NEXT: }
CASE(caseDataScalarAbsent) {
  int x;
  PRINT_VAR_INFO(x);
  PRINT_VAR_INFO(x);
  #pragma acc data present(x)
  USE_VAR(x = 1);
}

//   PRT-LABEL: {{.*}}caseDataStructPresent{{.*}} {
//    PRT-NEXT:   struct S {
//         PRT:   } s;
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//
//  PRT-A-NEXT:   #pragma acc data copy(s){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: s){{$}}
//  PRT-A-NEXT:   #pragma acc data present(s){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[PRESENT_MT]]: s){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: s){{$}}
// PRT-OA-NEXT:   // #pragma acc data copy(s){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[PRESENT_MT]]: s){{$}}
// PRT-OA-NEXT:   // #pragma acc data present(s){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
CASE(caseDataStructPresent) {
  struct S { int i; int j; } s;
  PRINT_VAR_INFO(s);
  PRINT_VAR_INFO(s);
  #pragma acc data copy(s)
  #pragma acc data present(s)
  USE_VAR(s.i = 1);
}

//   PRT-LABEL: {{.*}}caseDataStructAbsent{{.*}} {
//    PRT-NEXT:   struct S {
//         PRT:   } s;
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//  PRT-A-NEXT:   #pragma acc data present(s){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[PRESENT_MT]]: s){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[PRESENT_MT]]: s){{$}}
// PRT-OA-NEXT:   // #pragma acc data present(s){{$}}
//    PRT-NEXT:   ;
//    PRT-NEXT: }
CASE(caseDataStructAbsent) {
  struct S { int i; int j; } s;
  PRINT_VAR_INFO(s);
  PRINT_VAR_INFO(s);
  #pragma acc data present(s)
  USE_VAR(s.i = 1);
}

//   PRT-LABEL: {{.*}}caseDataArrayPresent{{.*}} {
//    PRT-NEXT:   int arr[3];
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//
//  PRT-A-NEXT:   #pragma acc data copy(arr){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: arr){{$}}
//  PRT-A-NEXT:   #pragma acc data present(arr){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[PRESENT_MT]]: arr){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: arr){{$}}
// PRT-OA-NEXT:   // #pragma acc data copy(arr){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[PRESENT_MT]]: arr){{$}}
// PRT-OA-NEXT:   // #pragma acc data present(arr){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
CASE(caseDataArrayPresent) {
  int arr[3];
  PRINT_VAR_INFO(arr);
  PRINT_VAR_INFO(arr);
  #pragma acc data copy(arr)
  #pragma acc data present(arr)
  USE_VAR(arr[0] = 1);
}

//   PRT-LABEL: {{.*}}caseDataArrayAbsent{{.*}} {
//    PRT-NEXT:   int arr[3];
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//  PRT-A-NEXT:   #pragma acc data present(arr){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[PRESENT_MT]]: arr){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[PRESENT_MT]]: arr){{$}}
// PRT-OA-NEXT:   // #pragma acc data present(arr){{$}}
//    PRT-NEXT:   ;
//    PRT-NEXT: }
CASE(caseDataArrayAbsent) {
  int arr[3];
  PRINT_VAR_INFO(arr);
  PRINT_VAR_INFO(arr);
  #pragma acc data present(arr)
  USE_VAR(arr[0] = 1);
}

//   PRT-LABEL: {{.*}}caseDataMemberPresent{{.*}} {
//    PRT-NEXT:   struct S {
//         PRT:   };
//    PRT-NEXT:   struct S same, beg, mid, end;
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//
//  PRT-A-NEXT:   #pragma acc data copy(same,beg,mid,end)
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: same,beg,mid,end)
//  PRT-A-NEXT:   #pragma acc data present(same.x,same.y,same.z,beg.x,mid.y,end.z)
// PRT-AO-NEXT:   // #pragma omp target data map([[PRESENT_MT]]: same.x,same.y,same.z,beg.x,mid.y,end.z)
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: same,beg,mid,end)
// PRT-OA-NEXT:   // #pragma acc data copy(same,beg,mid,end)
//  PRT-O-NEXT:   #pragma omp target data map([[PRESENT_MT]]: same.x,same.y,same.z,beg.x,mid.y,end.z)
// PRT-OA-NEXT:   // #pragma acc data present(same.x,same.y,same.z,beg.x,mid.y,end.z)
//
//    PRT-NEXT:   {
//    PRT-NEXT:     ;
//    PRT-NEXT:   }

//  PRT-A-NEXT:   #pragma acc data copy(same.y,same.z,beg.y,beg.z,mid.x,mid.y,mid.z,end.x,end.y)
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: same.y,same.z,beg.y,beg.z,mid.x,mid.y,mid.z,end.x,end.y)
//  PRT-A-NEXT:   #pragma acc data present(same.y,same.z,beg.y,mid.y,end.y)
// PRT-AO-NEXT:   // #pragma omp target data map([[PRESENT_MT]]: same.y,same.z,beg.y,mid.y,end.y)
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: same.y,same.z,beg.y,beg.z,mid.x,mid.y,mid.z,end.x,end.y)
// PRT-OA-NEXT:   // #pragma acc data copy(same.y,same.z,beg.y,beg.z,mid.x,mid.y,mid.z,end.x,end.y)
//  PRT-O-NEXT:   #pragma omp target data map([[PRESENT_MT]]: same.y,same.z,beg.y,mid.y,end.y)
// PRT-OA-NEXT:   // #pragma acc data present(same.y,same.z,beg.y,mid.y,end.y)
//
//    PRT-NEXT:   {
//    PRT-NEXT:     ;
//    PRT-NEXT:   }
//    PRT-NEXT: }
CASE(caseDataMemberPresent) {
  struct S { int x; int y; int z; };
  struct S same, beg, mid, end;
  PRINT_VAR_INFO(same);
  PRINT_VAR_INFO(same);
  #pragma acc data copy(same,beg,mid,end)
  #pragma acc data present(same.x,same.y,same.z,beg.x,mid.y,end.z)
  {
    USE_VAR(same.x = 1; beg.x = 1; mid.y = 1; end.z = 1);
  }
  #pragma acc data copy(same.y,same.z,beg.y,beg.z,mid.x,mid.y,mid.z,end.x,end.y)
  #pragma acc data present(same.y,same.z,beg.y,mid.y,end.y)
  {
    USE_VAR(same.y = 1; beg.y = 1; mid.y = 1; end.y = 1);
  }
}

//   PRT-LABEL: {{.*}}caseDataMemberAbsent{{.*}} {
//    PRT-NEXT:   struct S {
//         PRT:   } s;
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//  PRT-A-NEXT:   #pragma acc data present(s){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[PRESENT_MT]]: s){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[PRESENT_MT]]: s){{$}}
// PRT-OA-NEXT:   // #pragma acc data present(s){{$}}
//    PRT-NEXT:   ;
//    PRT-NEXT: }
CASE(caseDataMemberAbsent) {
  struct S { int x; int y; int z; } s;
  PRINT_VAR_INFO(s);
  PRINT_VAR_INFO(s);
  #pragma acc data present(s)
  USE_VAR(s.y = 1);
}

//   PRT-LABEL: {{.*}}caseDataMembersDisjoint{{.*}} {
//    PRT-NEXT:   struct S {
//         PRT:   } s;
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//
//  PRT-A-NEXT:   #pragma acc data copy(s.x){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: s.x){{$}}
//  PRT-A-NEXT:   #pragma acc data present(s.y){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[PRESENT_MT]]: s.y){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: s.x){{$}}
// PRT-OA-NEXT:   // #pragma acc data copy(s.x){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[PRESENT_MT]]: s.y){{$}}
// PRT-OA-NEXT:   // #pragma acc data present(s.y){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
CASE(caseDataMembersDisjoint) {
  struct S { int x; int y; } s;
  PRINT_VAR_INFO(s.x);
  PRINT_VAR_INFO(s.y);
  #pragma acc data copy(s.x)
  #pragma acc data present(s.y)
  USE_VAR(s.y = 1);
}

//   PRT-LABEL: {{.*}}caseDataMembersConcat2{{.*}} {
//    PRT-NEXT:   struct S {
//         PRT:   } s;
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//
//  PRT-A-NEXT:   #pragma acc data copyout(s.x){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,from: s.x){{$}}
//  PRT-A-NEXT:   #pragma acc data copy(s.y){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: s.y){{$}}
//  PRT-A-NEXT:   #pragma acc data present(s){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[PRESENT_MT]]: s){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,from: s.x){{$}}
// PRT-OA-NEXT:   // #pragma acc data copyout(s.x){{$}}
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: s.y){{$}}
// PRT-OA-NEXT:   // #pragma acc data copy(s.y){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[PRESENT_MT]]: s){{$}}
// PRT-OA-NEXT:   // #pragma acc data present(s){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
CASE(caseDataMembersConcat2) {
  struct S { int x; int y; } s;
  PRINT_VAR_INFO(s.x);
  PRINT_VAR_INFO(s);
  #pragma acc data copyout(s.x)
  #pragma acc data copy(s.y)
  #pragma acc data present(s)
  USE_VAR(s.x = 1);
}

CASE(caseDataMemberFullStruct) {
  struct S { int x; int y; int z; } s;
  PRINT_VAR_INFO(s.y);
  PRINT_VAR_INFO(s);
  #pragma acc data copyin(s.y)
  #pragma acc data present(s)
  USE_VAR(s.y = 1);
}

//   PRT-LABEL: {{.*}}caseDataSubarrayPresent{{.*}} {
//    PRT-NEXT:   int same[10], beg[10], mid[10], end[10];
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//
//  PRT-A-NEXT:   #pragma acc data copy(same,beg,mid,end)
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: same,beg,mid,end)
//  PRT-A-NEXT:   #pragma acc data present(same[0:10],beg[0:3],mid[3:3],end[8:2])
// PRT-AO-NEXT:   // #pragma omp target data map([[PRESENT_MT]]: same[0:10],beg[0:3],mid[3:3],end[8:2])
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: same,beg,mid,end)
// PRT-OA-NEXT:   // #pragma acc data copy(same,beg,mid,end)
//  PRT-O-NEXT:   #pragma omp target data map([[PRESENT_MT]]: same[0:10],beg[0:3],mid[3:3],end[8:2])
// PRT-OA-NEXT:   // #pragma acc data present(same[0:10],beg[0:3],mid[3:3],end[8:2])
//
//    PRT-NEXT:   {
//    PRT-NEXT:     ;
//    PRT-NEXT:   }
//
//  PRT-A-NEXT:   #pragma acc data copy(same[3:6],beg[2:5],mid[1:8],end[0:5])
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: same[3:6],beg[2:5],mid[1:8],end[0:5])
//  PRT-A-NEXT:   #pragma acc data present(same[3:6],beg[2:2],mid[3:3],end[4:1])
// PRT-AO-NEXT:   // #pragma omp target data map([[PRESENT_MT]]: same[3:6],beg[2:2],mid[3:3],end[4:1])
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: same[3:6],beg[2:5],mid[1:8],end[0:5])
// PRT-OA-NEXT:   // #pragma acc data copy(same[3:6],beg[2:5],mid[1:8],end[0:5])
//  PRT-O-NEXT:   #pragma omp target data map([[PRESENT_MT]]: same[3:6],beg[2:2],mid[3:3],end[4:1])
// PRT-OA-NEXT:   // #pragma acc data present(same[3:6],beg[2:2],mid[3:3],end[4:1])
//
//    PRT-NEXT:   {
//    PRT-NEXT:     ;
//    PRT-NEXT:   }
//    PRT-NEXT: }
CASE(caseDataSubarrayPresent) {
  int same[10], beg[10], mid[10], end[10];
  PRINT_VAR_INFO(same);
  PRINT_VAR_INFO(same);
  #pragma acc data copy(same,beg,mid,end)
  #pragma acc data present(same[0:10],beg[0:3],mid[3:3],end[8:2])
  {
    USE_VAR(same[0] = 1; beg[0] = 1; mid[3] = 1; end[8] = 1);
  }
  #pragma acc data copy(same[3:6],beg[2:5],mid[1:8],end[0:5])
  #pragma acc data present(same[3:6],beg[2:2],mid[3:3],end[4:1])
  {
    USE_VAR(same[3] = 1; beg[2] = 1; mid[3] = 1; end[4] = 1);
  }
}

//   PRT-LABEL: {{.*}}caseDataSubarrayDisjoint{{.*}} {
//    PRT-NEXT:   int arr[4];
//    PRT-NEXT:   {{(PRINT_SUBARRAY_INFO|fprintf)\(.*\);}}
//    PRT-NEXT:   {{(PRINT_SUBARRAY_INFO|fprintf)\(.*\);}}
//
//  PRT-A-NEXT:   #pragma acc data copy(arr[0:2]){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: arr[0:2]){{$}}
//  PRT-A-NEXT:   #pragma acc data present(arr[2:2]){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[PRESENT_MT]]: arr[2:2]){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: arr[0:2]){{$}}
// PRT-OA-NEXT:   // #pragma acc data copy(arr[0:2]){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[PRESENT_MT]]: arr[2:2]){{$}}
// PRT-OA-NEXT:   // #pragma acc data present(arr[2:2]){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
CASE(caseDataSubarrayDisjoint) {
  int arr[4];
  PRINT_SUBARRAY_INFO(arr, 0, 2);
  PRINT_SUBARRAY_INFO(arr, 2, 2);
  #pragma acc data copy(arr[0:2])
  #pragma acc data present(arr[2:2])
  USE_VAR(arr[2] = 1);
}

//   PRT-LABEL: {{.*}}caseDataSubarrayOverlapStart{{.*}} {
//    PRT-NEXT:   int arr[5];
//    PRT-NEXT:   {{(PRINT_SUBARRAY_INFO|fprintf)\(.*\);}}
//    PRT-NEXT:   {{(PRINT_SUBARRAY_INFO|fprintf)\(.*\);}}
//
//  PRT-A-NEXT:   #pragma acc data copyin(arr[1:2]){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,to: arr[1:2]){{$}}
//  PRT-A-NEXT:   #pragma acc data present(arr[0:2]){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[PRESENT_MT]]: arr[0:2]){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,to: arr[1:2]){{$}}
// PRT-OA-NEXT:   // #pragma acc data copyin(arr[1:2]){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[PRESENT_MT]]: arr[0:2]){{$}}
// PRT-OA-NEXT:   // #pragma acc data present(arr[0:2]){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
CASE(caseDataSubarrayOverlapStart) {
  int arr[5];
  PRINT_SUBARRAY_INFO(arr, 1, 2);
  PRINT_SUBARRAY_INFO(arr, 0, 2);
  #pragma acc data copyin(arr[1:2])
  #pragma acc data present(arr[0:2])
  USE_VAR(arr[0] = 1);
}

//   PRT-LABEL: {{.*}}caseDataSubarrayOverlapEnd{{.*}} {
//    PRT-NEXT:   int arr[5];
//    PRT-NEXT:   {{(PRINT_SUBARRAY_INFO|fprintf)\(.*\);}}
//    PRT-NEXT:   {{(PRINT_SUBARRAY_INFO|fprintf)\(.*\);}}
//
//  PRT-A-NEXT:   #pragma acc data copyin(arr[1:2]){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,to: arr[1:2]){{$}}
//  PRT-A-NEXT:   #pragma acc data present(arr[2:2]){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[PRESENT_MT]]: arr[2:2]){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,to: arr[1:2]){{$}}
// PRT-OA-NEXT:   // #pragma acc data copyin(arr[1:2]){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[PRESENT_MT]]: arr[2:2]){{$}}
// PRT-OA-NEXT:   // #pragma acc data present(arr[2:2]){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
CASE(caseDataSubarrayOverlapEnd) {
  int arr[5];
  PRINT_SUBARRAY_INFO(arr, 1, 2);
  PRINT_SUBARRAY_INFO(arr, 2, 2);
  #pragma acc data copyin(arr[1:2])
  #pragma acc data present(arr[2:2])
  USE_VAR(arr[2] = 1);
}

//   PRT-LABEL: {{.*}}caseDataSubarrayConcat2{{.*}} {
//    PRT-NEXT:   int arr[4];
//    PRT-NEXT:   {{(PRINT_SUBARRAY_INFO|fprintf)\(.*\);}}
//    PRT-NEXT:   {{(PRINT_SUBARRAY_INFO|fprintf)\(.*\);}}
//
//  PRT-A-NEXT:   #pragma acc data copyout(arr[0:2]){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,from: arr[0:2]){{$}}
//  PRT-A-NEXT:   #pragma acc data copy(arr[2:2]){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: arr[2:2]){{$}}
//  PRT-A-NEXT:   #pragma acc data present(arr[0:4]){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[PRESENT_MT]]: arr[0:4]){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,from: arr[0:2]){{$}}
// PRT-OA-NEXT:   // #pragma acc data copyout(arr[0:2]){{$}}
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: arr[2:2]){{$}}
// PRT-OA-NEXT:   // #pragma acc data copy(arr[2:2]){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[PRESENT_MT]]: arr[0:4]){{$}}
// PRT-OA-NEXT:   // #pragma acc data present(arr[0:4]){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
CASE(caseDataSubarrayConcat2) {
  int arr[4];
  PRINT_SUBARRAY_INFO(arr, 0, 2);
  PRINT_SUBARRAY_INFO(arr, 0, 4);
  #pragma acc data copyout(arr[0:2])
  #pragma acc data copy(arr[2:2])
  #pragma acc data present(arr[0:4])
  USE_VAR(arr[0] = 1);
}

CASE(caseDataSubarrayNonSubarray) {
  int arr[5];
  PRINT_SUBARRAY_INFO(arr, 1, 2);
  PRINT_SUBARRAY_INFO(arr, 0, 5);
  #pragma acc data copyin(arr[1:2])
  #pragma acc data present(arr)
  USE_VAR(arr[2] = 1);
}

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseParallelScalarPresent
//       DMP: ACCDataDirective
//  DMP-NEXT:   ACCCopyClause
//  DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:   impl: OMPTargetDataDirective
//  DMP-NEXT:     OMPMapClause
//  DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'int'
//       DMP: ACCParallelDirective
//  DMP-NEXT:   ACCPresentClause
//  DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:   impl: OMPTargetTeamsDirective
//  DMP-NEXT:     OMPMapClause
//  DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'int'
//
//   PRT-LABEL: {{.*}}caseParallelScalarPresent{{.*}} {
//    PRT-NEXT:   int x;
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//
//  PRT-A-NEXT:   #pragma acc data copy(x){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: x){{$}}
//  PRT-A-NEXT:   #pragma acc parallel present(x){{$}}
// PRT-AO-NEXT:   // #pragma omp target teams map([[PRESENT_MT]]: x){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: x){{$}}
// PRT-OA-NEXT:   // #pragma acc data copy(x){{$}}
//  PRT-O-NEXT:   #pragma omp target teams map([[PRESENT_MT]]: x){{$}}
// PRT-OA-NEXT:   // #pragma acc parallel present(x){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
CASE(caseParallelScalarPresent) {
  int x;
  PRINT_VAR_INFO(x);
  PRINT_VAR_INFO(x);
  #pragma acc data copy(x)
  #pragma acc parallel present(x)
  USE_VAR(x = 1);
}

CASE(caseParallelScalarAbsent) {
  int x;
  PRINT_VAR_INFO(x);
  PRINT_VAR_INFO(x);
  #pragma acc parallel present(x)
  USE_VAR(x = 1);
}

CASE(caseParallelStructPresent) {
  struct S { int i; int j; } s;
  PRINT_VAR_INFO(s);
  PRINT_VAR_INFO(s);
  #pragma acc data copy(s)
  #pragma acc parallel present(s)
  USE_VAR(s.i = 1);
}

CASE(caseParallelStructAbsent) {
  struct S { int i; int j; } s;
  PRINT_VAR_INFO(s);
  PRINT_VAR_INFO(s);
  #pragma acc parallel present(s)
  USE_VAR(s.i = 1);
}

CASE(caseParallelArrayPresent) {
  int arr[3];
  PRINT_VAR_INFO(arr);
  PRINT_VAR_INFO(arr);
  #pragma acc data copy(arr)
  #pragma acc parallel present(arr)
  USE_VAR(arr[0] = 1);
}

CASE(caseParallelArrayAbsent) {
  int arr[3];
  PRINT_VAR_INFO(arr);
  PRINT_VAR_INFO(arr);
  #pragma acc parallel present(arr)
  USE_VAR(arr[0] = 1);
}

CASE(caseParallelMemberPresent) {
  struct S { int x; int y; int z; };
  struct S same, beg, mid, end;
  PRINT_VAR_INFO(same);
  PRINT_VAR_INFO(same);
  #pragma acc data copy(same,beg,mid,end)
  #pragma acc parallel present(same.x,same.y,same.z,beg.x,mid.y,end.z)
  {
    USE_VAR(same.x = 1; beg.x = 1; mid.y = 1; end.z = 1);
  }
  #pragma acc data copy(same.y,same.z,beg.y,beg.z,mid.x,mid.y,mid.z,end.x,end.y)
  #pragma acc parallel present(same.y,same.z,beg.y,mid.y,end.y)
  {
    USE_VAR(same.y = 1; beg.y = 1; mid.y = 1; end.y = 1);
  }
}

CASE(caseParallelMemberAbsent) {
  struct S { int x; int y; int z; } s;
  PRINT_VAR_INFO(s);
  PRINT_VAR_INFO(s);
  #pragma acc parallel present(s)
  USE_VAR(s.y = 1);
}

CASE(caseParallelMembersDisjoint) {
  struct S { int x; int y; } s;
  PRINT_VAR_INFO(s.x);
  PRINT_VAR_INFO(s.y);
  #pragma acc data copy(s.x)
  #pragma acc parallel present(s.y)
  USE_VAR(s.y = 1);
}

CASE(caseParallelMembersConcat2) {
  struct S { int x; int y; } s;
  PRINT_VAR_INFO(s.x);
  PRINT_VAR_INFO(s);
  #pragma acc data copyout(s.x)
  #pragma acc data copy(s.y)
  #pragma acc parallel present(s)
  USE_VAR(s.x = 1);
}

CASE(caseParallelMemberFullStruct) {
  struct S { int x; int y; int z; } s;
  PRINT_VAR_INFO(s.y);
  PRINT_VAR_INFO(s);
  #pragma acc data copyin(s.y)
  #pragma acc parallel present(s)
  USE_VAR(s.y = 1);
}

CASE(caseParallelSubarrayPresent) {
  int same[10], beg[10], mid[10], end[10];
  PRINT_VAR_INFO(same);
  PRINT_VAR_INFO(same);
  #pragma acc data copy(same,beg,mid,end)
  #pragma acc parallel present(same[0:10],beg[0:3],mid[3:3],end[8:2])
  {
    USE_VAR(same[0] = 1; beg[0] = 1; mid[3] = 1; end[8] = 1);
  }
  #pragma acc data copy(same[3:6],beg[2:5],mid[1:8],end[0:5])
  #pragma acc parallel present(same[3:6],beg[2:2],mid[3:3],end[4:1])
  {
    USE_VAR(same[3] = 1; beg[2] = 1; mid[3] = 1; end[4] = 1);
  }
}

CASE(caseParallelSubarrayDisjoint) {
  int arr[4];
  PRINT_SUBARRAY_INFO(arr, 0, 2);
  PRINT_SUBARRAY_INFO(arr, 2, 2);
  #pragma acc data copy(arr[0:2])
  #pragma acc parallel present(arr[2:2])
  USE_VAR(arr[2] = 1);
}

CASE(caseParallelSubarrayOverlapStart) {
  int arr[10];
  PRINT_SUBARRAY_INFO(arr, 5, 4);
  PRINT_SUBARRAY_INFO(arr, 4, 4);
  #pragma acc data copyin(arr[5:4])
  #pragma acc parallel present(arr[4:4])
  USE_VAR(arr[4] = 1);
}

CASE(caseParallelSubarrayOverlapEnd) {
  int arr[10];
  PRINT_SUBARRAY_INFO(arr, 3, 4);
  PRINT_SUBARRAY_INFO(arr, 4, 4);
  #pragma acc data copyin(arr[3:4])
  #pragma acc parallel present(arr[4:4])
  USE_VAR(arr[4] = 1);
}

CASE(caseParallelSubarrayConcat2) {
  int arr[4];
  PRINT_SUBARRAY_INFO(arr, 0, 2);
  PRINT_SUBARRAY_INFO(arr, 0, 4);
  #pragma acc data copyout(arr[0:2])
  #pragma acc data copy(arr[2:2])
  #pragma acc parallel present(arr[0:4])
  USE_VAR(arr[0] = 1);
}

CASE(caseParallelSubarrayNonSubarray) {
  int arr[10];
  PRINT_SUBARRAY_INFO(arr, 3, 4);
  PRINT_SUBARRAY_INFO(arr, 0, 10);
  #pragma acc data copyin(arr[3:4])
  #pragma acc parallel present(arr)
  USE_VAR(arr[4] = 1);
}

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseParallelLoopScalarPresent
//       DMP: ACCDataDirective
//  DMP-NEXT:   ACCCopyClause
//  DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:   impl: OMPTargetDataDirective
//  DMP-NEXT:     OMPMapClause
//  DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'int'
//       DMP:   ACCParallelLoopDirective
//  DMP-NEXT:     ACCPresentClause
//  DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:     effect: ACCParallelDirective
//  DMP-NEXT:       ACCPresentClause
//  DMP-NEXT:         DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:       impl: OMPTargetTeamsDirective
//  DMP-NEXT:         OMPMapClause
//  DMP-NEXT:           DeclRefExpr {{.*}} 'x' 'int'
//       DMP:       ACCLoopDirective
//  DMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:         ACCGangClause {{.*}} <implicit>
//  DMP-NEXT:         impl: OMPDistributeDirective
//   DMP-NOT:           OMP
//       DMP:           ForStmt
//
//   PRT-LABEL: {{.*}}caseParallelLoopScalarPresent{{.*}} {
//    PRT-NEXT:   int x;
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//
//  PRT-A-NEXT:   #pragma acc data copy(x){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: x){{$}}
//  PRT-A-NEXT:   #pragma acc parallel loop present(x){{$}}
// PRT-AO-NEXT:   // #pragma omp target teams map([[PRESENT_MT]]: x){{$}}
// PRT-AO-NEXT:   // #pragma omp distribute{{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: x){{$}}
// PRT-OA-NEXT:   // #pragma acc data copy(x){{$}}
//  PRT-O-NEXT:   #pragma omp target teams map([[PRESENT_MT]]: x){{$}}
//  PRT-O-NEXT:   #pragma omp distribute{{$}}
// PRT-OA-NEXT:   // #pragma acc parallel loop present(x){{$}}
//
//    PRT-NEXT:   for ({{.*}})
//    PRT-NEXT:     ;
//    PRT-NEXT: }
CASE(caseParallelLoopScalarPresent) {
  int x;
  PRINT_VAR_INFO(x);
  PRINT_VAR_INFO(x);
  #pragma acc data copy(x)
  #pragma acc parallel loop present(x)
  for (int i = 0; i < 2; ++i)
  USE_VAR(x = 1);
}

CASE(caseParallelLoopScalarAbsent) {
  int x;
  PRINT_VAR_INFO(x);
  PRINT_VAR_INFO(x);
  #pragma acc parallel loop present(x)
  for (int i = 0; i < 2; ++i)
  USE_VAR(x = 1);
}

CASE(caseConstPresent) {
  const int x;
  PRINT_VAR_INFO(x);
  PRINT_VAR_INFO(x);
  int y;
  #pragma acc data copy(x)
  #pragma acc parallel loop present(x)
  for (int i = 0; i < 2; ++i)
  USE_VAR(y = x);
}

CASE(caseConstAbsent) {
  const int x;
  PRINT_VAR_INFO(x);
  PRINT_VAR_INFO(x);
  int y;
  #pragma acc parallel loop present(x)
  for (int i = 0; i < 2; ++i)
  USE_VAR(y = x);
}
