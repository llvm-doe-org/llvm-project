// Check no_create clauses on different constructs and with different values of
// -fopenacc-no-create-omp.
//
// data.c checks various interactions with explicit DAs and the defaultmap added
// for scalars with suppressed OpenACC implicit DAs.  Diagnostics about
// ompx_no_alloc in the translation are checked in
// diagnostics/warn-acc-omp-map-ompx-no-alloc.c.  Diagnostics about bad
// -fopenacc-no-create-omp arguments are checked in
// diagnostics/fopenacc-no-create-omp.c.
//
// In some cases, it's challenging to check when no_create actually doesn't
// allocate memory.  Specifically, calling acc_is_present or
// omp_target_is_present or specifying a present clause on a directive only
// works from the host, so it doesn't help for checking no_create on
// "acc parallel".  We could examine the output of LIBOMPTARGET_DEBUG=1, but it
// only works in debug builds.  Our solution is to utilize the OpenACC Profiling
// Interface, which we usually only exercise in the runtime test suite.
//
// We include dump and print checking on only a few representative cases, which
// should be more than sufficient to show it's working for the no_create clause.

// Redefine this to specify how %{check-cases} expands for each case.
//
// - CASE = the case name used in the enum and as a command line argument.
// - NO_ALLOC_OR_ALLOC = 'NO-ALLOC' if the no_create clause will create
//   allocations, and 'ALLOC' otherwise.
// - NOT_CRASH_IF_FAIL = 'not --crash' if the case is expected to fail an
//   array extension check, and the empty string otherwise.
//
// DEFINE: %{check-case}( CASE %, NO_ALLOC_OR_ALLOC %, NOT_CRASH_IF_FAIL %) =

// Substitution to run %{check-case} for each case.
//
// - NO_ALLOC_OR_ALLOC = 'NO-ALLOC' if the no_create clause will create
//   allocations, and 'ALLOC' otherwise.
// - NOT_CRASH_IF_OFF_AND_ALLOC = 'not --crash' if the no_create clause will
//   create allocations, and the empty string otherwise.  Each case will use
//   this if it is expected to fail an array extension check.
//
// If a case is listed here but is not covered in the code, that case will fail.
// If a case is covered in the code but not listed here, the code will not
// compile because this list produces the enum used by the code.
//
// The various cases covered here should be kept consistent with present.c,
// update.c, and data-extension-errors.c (the last is located in
// openmp/libacc2omp/test/directives).  For example, a subarray that extends a
// subarray already present is consistently considered not present, so the
// present clause produces a runtime error and the no_create clause doesn't
// allocate.  However, INHERITED cases have no meaning for the present clause.
//
// "acc parallel loop" should be about the same as "acc parallel", so a few
// cases are probably sufficient.
//
// DEFINE: %{check-cases}( NO_ALLOC_OR_ALLOC %, NOT_CRASH_IF_OFF_AND_ALLOC %) =                                             \
//                          CASE                                NO_ALLOC_OR_ALLOC       NOT_CRASH_IF_FAIL
// DEFINE:   %{check-case}( caseDataScalarPresent            %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseDataScalarAbsent             %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseDataStructPresent            %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseDataStructAbsent             %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseDataArrayPresent             %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseDataArrayAbsent              %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseDataMemberPresent            %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseDataMemberAbsent             %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseDataMembersDisjoint          %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseDataMembersConcat2           %, %{NO_ALLOC_OR_ALLOC} %, %{NOT_CRASH_IF_OFF_AND_ALLOC} %) && \
// DEFINE:   %{check-case}( caseDataMemberFullStruct         %, %{NO_ALLOC_OR_ALLOC} %, %{NOT_CRASH_IF_OFF_AND_ALLOC} %) && \
// DEFINE:   %{check-case}( caseDataSubarrayPresent          %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseDataSubarrayDisjoint         %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseDataSubarrayOverlapStart     %, %{NO_ALLOC_OR_ALLOC} %, %{NOT_CRASH_IF_OFF_AND_ALLOC} %) && \
// DEFINE:   %{check-case}( caseDataSubarrayOverlapEnd       %, %{NO_ALLOC_OR_ALLOC} %, %{NOT_CRASH_IF_OFF_AND_ALLOC} %) && \
// DEFINE:   %{check-case}( caseDataSubarrayConcat2          %, %{NO_ALLOC_OR_ALLOC} %, %{NOT_CRASH_IF_OFF_AND_ALLOC} %) && \
// DEFINE:   %{check-case}( caseDataSubarrayNonSubarray      %, %{NO_ALLOC_OR_ALLOC} %, %{NOT_CRASH_IF_OFF_AND_ALLOC} %) && \
// DEFINE:   %{check-case}( caseParallelScalarPresent        %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseParallelScalarAbsent         %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseParallelStructPresent        %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseParallelStructAbsent         %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseParallelArrayPresent         %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseParallelArrayAbsent          %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseParallelMemberPresent        %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseParallelMemberAbsent         %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseParallelMembersDisjoint      %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseParallelMembersConcat2       %, %{NO_ALLOC_OR_ALLOC} %, %{NOT_CRASH_IF_OFF_AND_ALLOC} %) && \
// DEFINE:   %{check-case}( caseParallelMemberFullStruct     %, %{NO_ALLOC_OR_ALLOC} %, %{NOT_CRASH_IF_OFF_AND_ALLOC} %) && \
// DEFINE:   %{check-case}( caseParallelSubarrayPresent      %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseParallelSubarrayDisjoint     %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseParallelSubarrayOverlapStart %, %{NO_ALLOC_OR_ALLOC} %, %{NOT_CRASH_IF_OFF_AND_ALLOC} %) && \
// DEFINE:   %{check-case}( caseParallelSubarrayOverlapEnd   %, %{NO_ALLOC_OR_ALLOC} %, %{NOT_CRASH_IF_OFF_AND_ALLOC} %) && \
// DEFINE:   %{check-case}( caseParallelSubarrayConcat2      %, %{NO_ALLOC_OR_ALLOC} %, %{NOT_CRASH_IF_OFF_AND_ALLOC} %) && \
// DEFINE:   %{check-case}( caseParallelSubarrayNonSubarray  %, %{NO_ALLOC_OR_ALLOC} %, %{NOT_CRASH_IF_OFF_AND_ALLOC} %) && \
// DEFINE:   %{check-case}( caseParallelLoopScalarPresent    %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseParallelLoopScalarAbsent     %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseConstPresent                 %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseConstAbsent                  %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseInheritedPresent             %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseInheritedAbsent              %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseInheritedSubarrayPresent     %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseInheritedSubarrayAbsent      %, %{NO_ALLOC_OR_ALLOC} %,                               %)

// Generate the enum of cases.
//
//      RUN: echo '#define FOREACH_CASE(Macro) \' > %t-cases.h
// REDEFINE: %{check-case}( CASE %, NO_ALLOC_OR_ALLOC %, NOT_CRASH_IF_FAIL %) = \
// REDEFINE: echo '  Macro(%{CASE}) \' >> %t-cases.h
//      RUN: %{check-cases}(%,%)
//      RUN: echo '  /*end of FOREACH_CASE*/' >> %t-cases.h

// Prepare substitutions for trying all cases many times while varying the value
// of -fopenacc-no-create-omp.
//
// REDEFINE: %{all:clang:args-stable} = \
// REDEFINE:   -DCASES_HEADER='"%t-cases.h"' -gline-tables-only
// REDEFINE: %{exe:fc:args-stable} = -match-full-lines -allow-empty
// REDEFINE: %{check-case}( CASE %, NO_ALLOC_OR_ALLOC %, NOT_CRASH_IF_FAIL %) =          \
// REDEFINE:   : '------------ CASE: %{CASE}, %{NO_ALLOC_OR_ALLOC} ------------'      && \
// REDEFINE:   %{acc-check-exe-run-fn}( %{NOT_CRASH_IF_FAIL} %, %{CASE} %)            && \
// REDEFINE:   %{acc-check-exe-filecheck-fn}( %{CASE},%{CASE}-%{NO_ALLOC_OR_ALLOC} %)

// REDEFINE: %{all:clang:args} =
// REDEFINE: %{dmp:fc:pres} = NO-ALLOC
// REDEFINE: %{prt:fc:pres} = NO-ALLOC
// REDEFINE: %{prt:fc:args} = -DNO_CREATE_MT=ompx_no_alloc,ompx_hold,alloc \
// REDEFINE:                  -DINHERITED_NO_CREATE_MT=ompx_no_alloc,alloc
//      RUN: %{acc-check-dmp}
//      RUN: %{acc-check-prt}
//      RUN: %{acc-check-exe-compile}
//      RUN: %{check-cases}( NO-ALLOC %, %)

// REDEFINE: %{all:clang:args} = -fopenacc-no-create-omp=ompx-no-alloc
// REDEFINE: %{dmp:fc:pres} = NO-ALLOC
// REDEFINE: %{prt:fc:pres} = NO-ALLOC
// REDEFINE: %{prt:fc:args} = -DNO_CREATE_MT=ompx_no_alloc,ompx_hold,alloc \
// REDEFINE:                  -DINHERITED_NO_CREATE_MT=ompx_no_alloc,alloc
//      RUN: %{acc-check-dmp}
//      RUN: %{acc-check-prt}
//      RUN: %{acc-check-exe-compile}
//      RUN: %{check-cases}( NO-ALLOC %, %)

// REDEFINE: %{all:clang:args} = -fopenacc-no-create-omp=no-ompx-no-alloc
// REDEFINE: %{dmp:fc:pres} = ALLOC
// REDEFINE: %{prt:fc:pres} = ALLOC
// REDEFINE: %{prt:fc:args} = -DNO_CREATE_MT=ompx_hold,alloc \
// REDEFINE:                  -DINHERITED_NO_CREATE_MT=alloc
//      RUN: %{acc-check-dmp}
//      RUN: %{acc-check-prt}
//      RUN: %{acc-check-exe-compile}
//      RUN: %{check-cases}( ALLOC %, %if-tgt-host<|not --crash> %)

// END.

/* expected-no-diagnostics */

#include <acc_prof.h>
#include <stdio.h>
#include <string.h>

#define DEF_CALLBACK(Event)                                   \
static void on_##Event(acc_prof_info *pi, acc_event_info *ei, \
                       acc_api_info *ai) {                    \
  fprintf(stderr, "acc_ev_" #Event "\n");                     \
}

#define REG_CALLBACK(Event) reg(acc_ev_##Event, on_##Event, acc_reg)

DEF_CALLBACK(create)
DEF_CALLBACK(delete)
DEF_CALLBACK(alloc)
DEF_CALLBACK(free)
DEF_CALLBACK(enter_data_start)
DEF_CALLBACK(exit_data_start)

void acc_register_library(acc_prof_reg reg, acc_prof_reg unreg,
                          acc_prof_lookup lookup) {
  REG_CALLBACK(create);
  REG_CALLBACK(delete);
  REG_CALLBACK(alloc);
  REG_CALLBACK(free);
  REG_CALLBACK(enter_data_start);
  REG_CALLBACK(exit_data_start);
}

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

  caseFn();
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
//  DMP-NEXT:     ACCNoCreateClause
//  DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:     impl: OMPTargetDataDirective
//  DMP-NEXT:       OMPMapClause
//  DMP-NEXT:         DeclRefExpr {{.*}} 'x' 'int'
//
//   PRT-LABEL: {{.*}}caseDataScalarPresent{{.*}} {
//    PRT-NEXT:   int x;
//
//  PRT-A-NEXT:   #pragma acc data copy(x){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: x){{$}}
//  PRT-A-NEXT:   #pragma acc data no_create(x){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: x){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: x){{$}}
// PRT-OA-NEXT:   // #pragma acc data copy(x){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: x){{$}}
// PRT-OA-NEXT:   // #pragma acc data no_create(x){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
//
//  EXE-OFF-caseDataScalarPresent-NOT: {{.}}
//      EXE-OFF-caseDataScalarPresent: acc_ev_enter_data_start
// EXE-OFF-caseDataScalarPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataScalarPresent-NEXT:   acc_ev_create
// EXE-OFF-caseDataScalarPresent-NEXT:   acc_ev_enter_data_start
// EXE-OFF-caseDataScalarPresent-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseDataScalarPresent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseDataScalarPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseDataScalarPresent-NEXT:   acc_ev_free
//  EXE-OFF-caseDataScalarPresent-NOT: {{.}}
CASE(caseDataScalarPresent) {
  int x;
  #pragma acc data copy(x)
  #pragma acc data no_create(x)
  ;
}

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseDataScalarAbsent
//       DMP: ACCDataDirective
//  DMP-NEXT:   ACCNoCreateClause
//  DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:   impl: OMPTargetDataDirective
//  DMP-NEXT:     OMPMapClause
//  DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'int'
//
//   PRT-LABEL: {{.*}}caseDataScalarAbsent{{.*}} {
//    PRT-NEXT:   int x;
//  PRT-A-NEXT:   #pragma acc data no_create(x){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: x){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: x){{$}}
// PRT-OA-NEXT:   // #pragma acc data no_create(x){{$}}
//    PRT-NEXT:   ;
//    PRT-NEXT: }
//
//        EXE-OFF-caseDataScalarAbsent-NOT: {{.}}
//            EXE-OFF-caseDataScalarAbsent: acc_ev_enter_data_start
// EXE-OFF-caseDataScalarAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataScalarAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-OFF-caseDataScalarAbsent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseDataScalarAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseDataScalarAbsent-ALLOC-NEXT:   acc_ev_free
//        EXE-OFF-caseDataScalarAbsent-NOT: {{.}}
CASE(caseDataScalarAbsent) {
  int x;
  #pragma acc data no_create(x)
  ;
}

//   PRT-LABEL: {{.*}}caseDataStructPresent{{.*}} {
//    PRT-NEXT:   struct S {
//         PRT:   } s;
//
//  PRT-A-NEXT:   #pragma acc data copy(s){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: s){{$}}
//  PRT-A-NEXT:   #pragma acc data no_create(s){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: s){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: s){{$}}
// PRT-OA-NEXT:   // #pragma acc data copy(s){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: s){{$}}
// PRT-OA-NEXT:   // #pragma acc data no_create(s){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
//
//  EXE-OFF-caseDataStructPresent-NOT: {{.}}
//      EXE-OFF-caseDataStructPresent: acc_ev_enter_data_start
// EXE-OFF-caseDataStructPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataStructPresent-NEXT:   acc_ev_create
// EXE-OFF-caseDataStructPresent-NEXT:   acc_ev_enter_data_start
// EXE-OFF-caseDataStructPresent-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseDataStructPresent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseDataStructPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseDataStructPresent-NEXT:   acc_ev_free
//  EXE-OFF-caseDataStructPresent-NOT: {{.}}
CASE(caseDataStructPresent) {
  struct S { int i; int j; } s;
  #pragma acc data copy(s)
  #pragma acc data no_create(s)
  ;
}

//   PRT-LABEL: {{.*}}caseDataStructAbsent{{.*}} {
//    PRT-NEXT:   struct S {
//         PRT:   } s;
//  PRT-A-NEXT:   #pragma acc data no_create(s){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: s){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: s){{$}}
// PRT-OA-NEXT:   // #pragma acc data no_create(s){{$}}
//    PRT-NEXT:   ;
//    PRT-NEXT: }
//
//        EXE-OFF-caseDataStructAbsent-NOT: {{.}}
//            EXE-OFF-caseDataStructAbsent: acc_ev_enter_data_start
// EXE-OFF-caseDataStructAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataStructAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-OFF-caseDataStructAbsent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseDataStructAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseDataStructAbsent-ALLOC-NEXT:   acc_ev_free
//        EXE-OFF-caseDataStructAbsent-NOT: {{.}}
CASE(caseDataStructAbsent) {
  struct S { int i; int j; } s;
  #pragma acc data no_create(s)
  ;
}

//   PRT-LABEL: {{.*}}caseDataArrayPresent{{.*}} {
//    PRT-NEXT:   int arr[3];
//
//  PRT-A-NEXT:   #pragma acc data copy(arr){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: arr){{$}}
//  PRT-A-NEXT:   #pragma acc data no_create(arr){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: arr){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: arr){{$}}
// PRT-OA-NEXT:   // #pragma acc data copy(arr){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: arr){{$}}
// PRT-OA-NEXT:   // #pragma acc data no_create(arr){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
//
//  EXE-OFF-caseDataArrayPresent-NOT: {{.}}
//      EXE-OFF-caseDataArrayPresent: acc_ev_enter_data_start
// EXE-OFF-caseDataArrayPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataArrayPresent-NEXT:   acc_ev_create
// EXE-OFF-caseDataArrayPresent-NEXT:   acc_ev_enter_data_start
// EXE-OFF-caseDataArrayPresent-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseDataArrayPresent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseDataArrayPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseDataArrayPresent-NEXT:   acc_ev_free
//  EXE-OFF-caseDataArrayPresent-NOT: {{.}}
CASE(caseDataArrayPresent) {
  int arr[3];
  #pragma acc data copy(arr)
  #pragma acc data no_create(arr)
  ;
}

//   PRT-LABEL: {{.*}}caseDataArrayAbsent{{.*}} {
//    PRT-NEXT:   int arr[3];
//  PRT-A-NEXT:   #pragma acc data no_create(arr){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: arr){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: arr){{$}}
// PRT-OA-NEXT:   // #pragma acc data no_create(arr){{$}}
//    PRT-NEXT:   ;
//    PRT-NEXT: }
//
//        EXE-OFF-caseDataArrayAbsent-NOT: {{.}}
//            EXE-OFF-caseDataArrayAbsent: acc_ev_enter_data_start
// EXE-OFF-caseDataArrayAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataArrayAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-OFF-caseDataArrayAbsent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseDataArrayAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseDataArrayAbsent-ALLOC-NEXT:   acc_ev_free
//        EXE-OFF-caseDataArrayAbsent-NOT: {{.}}
CASE(caseDataArrayAbsent) {
  int arr[3];
  #pragma acc data no_create(arr)
  ;
}

//   PRT-LABEL: {{.*}}caseDataMemberPresent{{.*}} {
//    PRT-NEXT:   struct S {
//         PRT:   };
//    PRT-NEXT:   struct S same, beg, mid, end;
//
//  PRT-A-NEXT:   #pragma acc data copy(same,beg,mid,end)
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: same,beg,mid,end)
//  PRT-A-NEXT:   #pragma acc data no_create(same.x,same.y,same.z,beg.x,mid.y,end.z)
// PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: same.x,same.y,same.z,beg.x,mid.y,end.z)
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: same,beg,mid,end)
// PRT-OA-NEXT:   // #pragma acc data copy(same,beg,mid,end)
//  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: same.x,same.y,same.z,beg.x,mid.y,end.z)
// PRT-OA-NEXT:   // #pragma acc data no_create(same.x,same.y,same.z,beg.x,mid.y,end.z)
//
//    PRT-NEXT:   ;
//
//  PRT-A-NEXT:   #pragma acc data copy(same.y,same.z,beg.y,beg.z,mid.x,mid.y,mid.z,end.x,end.y)
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: same.y,same.z,beg.y,beg.z,mid.x,mid.y,mid.z,end.x,end.y)
//  PRT-A-NEXT:   #pragma acc data no_create(same.y,same.z,beg.y,mid.y,end.y)
// PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: same.y,same.z,beg.y,mid.y,end.y)
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: same.y,same.z,beg.y,beg.z,mid.x,mid.y,mid.z,end.x,end.y)
// PRT-OA-NEXT:   // #pragma acc data copy(same.y,same.z,beg.y,beg.z,mid.x,mid.y,mid.z,end.x,end.y)
//  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: same.y,same.z,beg.y,mid.y,end.y)
// PRT-OA-NEXT:   // #pragma acc data no_create(same.y,same.z,beg.y,mid.y,end.y)
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
//
//  EXE-OFF-caseDataMemberPresent-NOT: {{.}}
//      EXE-OFF-caseDataMemberPresent: acc_ev_enter_data_start
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_create
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_create
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_create
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_create
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_enter_data_start
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseDataMemberPresent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_free
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_free
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_free
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_free
// EXE-OFF-caseDataMemberPresent-NEXT: acc_ev_enter_data_start
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_create
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_create
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_create
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_create
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_enter_data_start
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseDataMemberPresent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_free
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_free
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_free
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseDataMemberPresent-NEXT:   acc_ev_free
//  EXE-OFF-caseDataMemberPresent-NOT: {{.}}
CASE(caseDataMemberPresent) {
  struct S { int x; int y; int z; };
  struct S same, beg, mid, end;
  #pragma acc data copy(same,beg,mid,end)
  #pragma acc data no_create(same.x,same.y,same.z,beg.x,mid.y,end.z)
  ;
  #pragma acc data copy(same.y,same.z,beg.y,beg.z,mid.x,mid.y,mid.z,end.x,end.y)
  #pragma acc data no_create(same.y,same.z,beg.y,mid.y,end.y)
  ;
}

//  EXE-OFF-caseDataMemberAbsent-NOT: {{.}}
//                EXE-caseDataMemberAbsent: allocations not expected
//       EXE-OFF-caseDataMemberAbsent-NEXT: acc_ev_enter_data_start
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-OFF-caseDataMemberAbsent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_free
//       EXE-OFF-caseDataMemberAbsent-NEXT: acc_ev_enter_data_start
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-OFF-caseDataMemberAbsent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_free
//       EXE-OFF-caseDataMemberAbsent-NEXT: acc_ev_enter_data_start
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-OFF-caseDataMemberAbsent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_free
//       EXE-OFF-caseDataMemberAbsent-NEXT: acc_ev_enter_data_start
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-OFF-caseDataMemberAbsent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_free
//       EXE-OFF-caseDataMemberAbsent-NEXT: acc_ev_enter_data_start
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-OFF-caseDataMemberAbsent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_free
//       EXE-OFF-caseDataMemberAbsent-NEXT: acc_ev_enter_data_start
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-OFF-caseDataMemberAbsent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_free
//       EXE-OFF-caseDataMemberAbsent-NEXT: acc_ev_enter_data_start
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-OFF-caseDataMemberAbsent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseDataMemberAbsent-ALLOC-NEXT:   acc_ev_free
//           EXE-caseDataMemberAbsent-NEXT: allocations expected
//       EXE-OFF-caseDataMemberAbsent-NEXT: acc_ev_enter_data_start
//       EXE-OFF-caseDataMemberAbsent-NEXT:   acc_ev_alloc
//       EXE-OFF-caseDataMemberAbsent-NEXT:   acc_ev_create
//       EXE-OFF-caseDataMemberAbsent-NEXT: acc_ev_exit_data_start
//       EXE-OFF-caseDataMemberAbsent-NEXT:   acc_ev_delete
//       EXE-OFF-caseDataMemberAbsent-NEXT:   acc_ev_free
//       EXE-OFF-caseDataMemberAbsent-NEXT: acc_ev_enter_data_start
//       EXE-OFF-caseDataMemberAbsent-NEXT:   acc_ev_alloc
//       EXE-OFF-caseDataMemberAbsent-NEXT:   acc_ev_create
//       EXE-OFF-caseDataMemberAbsent-NEXT: acc_ev_exit_data_start
//       EXE-OFF-caseDataMemberAbsent-NEXT:   acc_ev_delete
//       EXE-OFF-caseDataMemberAbsent-NEXT:   acc_ev_free
//        EXE-OFF-caseDataMemberAbsent-NOT: {{.}}
CASE(caseDataMemberAbsent) {
  struct S { int x; int y; int z; } s;
  fprintf(stderr, "allocations not expected\n");
  #pragma acc data no_create(s.x)
  ;
  #pragma acc data no_create(s.y)
  ;
  #pragma acc data no_create(s.z)
  ;
  // Clang OpenMP codegen generates a map type entry for the struct as a whole
  // when multiple members are specified, so we need to be sure the struct as a
  // whole is handled as ompx_no_alloc.
  #pragma acc data no_create(s.x) no_create(s.y)
  ;
  #pragma acc data no_create(s.y) no_create(s.z)
  ;
  #pragma acc data no_create(s.x) no_create(s.z)
  ;
  #pragma acc data no_create(s.x, s.y, s.z)
  ;
  fprintf(stderr, "allocations expected\n");
  // If only some members have ompx_no_alloc, then the struct as a whole must
  // *not* be handled as ompx_no_alloc.
  #pragma acc data no_create(s.x) copy(s.y)
  ;
  #pragma acc data copy(s.x) no_create(s.y)
  ;
}

//   PRT-LABEL: {{.*}}caseDataMembersDisjoint{{.*}} {
//    PRT-NEXT:   struct S {
//         PRT:   } s;
//
//  PRT-A-NEXT:   #pragma acc data copy(s.x){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: s.x){{$}}
//  PRT-A-NEXT:   #pragma acc data no_create(s.y){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: s.y){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: s.x){{$}}
// PRT-OA-NEXT:   // #pragma acc data copy(s.x){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: s.y){{$}}
// PRT-OA-NEXT:   // #pragma acc data no_create(s.y){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
//
//        EXE-OFF-caseDataMembersDisjoint-NOT: {{.}}
//            EXE-OFF-caseDataMembersDisjoint: acc_ev_enter_data_start
//       EXE-OFF-caseDataMembersDisjoint-NEXT:   acc_ev_alloc
//       EXE-OFF-caseDataMembersDisjoint-NEXT:   acc_ev_create
//       EXE-OFF-caseDataMembersDisjoint-NEXT:   acc_ev_enter_data_start
// EXE-OFF-caseDataMembersDisjoint-ALLOC-NEXT:     acc_ev_alloc
// EXE-OFF-caseDataMembersDisjoint-ALLOC-NEXT:     acc_ev_create
//       EXE-OFF-caseDataMembersDisjoint-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseDataMembersDisjoint-ALLOC-NEXT:     acc_ev_delete
// EXE-OFF-caseDataMembersDisjoint-ALLOC-NEXT:     acc_ev_free
//       EXE-OFF-caseDataMembersDisjoint-NEXT: acc_ev_exit_data_start
//       EXE-OFF-caseDataMembersDisjoint-NEXT:   acc_ev_delete
//       EXE-OFF-caseDataMembersDisjoint-NEXT:   acc_ev_free
//        EXE-OFF-caseDataMembersDisjoint-NOT: {{.}}
CASE(caseDataMembersDisjoint) {
  struct S { int x; int y; } s;
  #pragma acc data copy(s.x)
  #pragma acc data no_create(s.y)
  ;
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
//  PRT-A-NEXT:   #pragma acc data no_create(s){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: s){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,from: s.x){{$}}
// PRT-OA-NEXT:   // #pragma acc data copyout(s.x){{$}}
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: s.y){{$}}
// PRT-OA-NEXT:   // #pragma acc data copy(s.y){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: s){{$}}
// PRT-OA-NEXT:   // #pragma acc data no_create(s){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
//
//  EXE-HOST-caseDataMembersConcat2-NOT: {{.}}
//      EXE-HOST-caseDataMembersConcat2: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
// EXE-HOST-caseDataMembersConcat2-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//
//           EXE-OFF-caseDataMembersConcat2-NOT: {{.}}
//               EXE-OFF-caseDataMembersConcat2: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//          EXE-OFF-caseDataMembersConcat2-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//          EXE-OFF-caseDataMembersConcat2-NEXT: acc_ev_enter_data_start
//          EXE-OFF-caseDataMembersConcat2-NEXT:   acc_ev_alloc
//          EXE-OFF-caseDataMembersConcat2-NEXT:   acc_ev_create
//          EXE-OFF-caseDataMembersConcat2-NEXT:   acc_ev_enter_data_start
//          EXE-OFF-caseDataMembersConcat2-NEXT:     acc_ev_alloc
//          EXE-OFF-caseDataMembersConcat2-NEXT:     acc_ev_create
//          EXE-OFF-caseDataMembersConcat2-NEXT:     acc_ev_enter_data_start
//    EXE-OFF-caseDataMembersConcat2-ALLOC-NEXT:     Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//    EXE-OFF-caseDataMembersConcat2-ALLOC-NEXT:     Libomptarget error: Call to getTargetPointer returned null pointer (device failure or illegal mapping).
//    EXE-OFF-caseDataMembersConcat2-ALLOC-NEXT:     Libomptarget error: Consult {{.*}}
//    EXE-OFF-caseDataMembersConcat2-ALLOC-NEXT:     {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                                    # An abort message usually follows.
//     EXE-OFF-caseDataMembersConcat2-ALLOC-NOT:     Libomptarget
// EXE-OFF-caseDataMembersConcat2-NO-ALLOC-NEXT:     acc_ev_exit_data_start
// EXE-OFF-caseDataMembersConcat2-NO-ALLOC-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseDataMembersConcat2-NO-ALLOC-NEXT:     acc_ev_delete
// EXE-OFF-caseDataMembersConcat2-NO-ALLOC-NEXT:     acc_ev_free
// EXE-OFF-caseDataMembersConcat2-NO-ALLOC-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseDataMembersConcat2-NO-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseDataMembersConcat2-NO-ALLOC-NEXT:   acc_ev_free
CASE(caseDataMembersConcat2) {
  struct S { int x; int y; } s;
  PRINT_VAR_INFO(s.x);
  PRINT_VAR_INFO(s);
  #pragma acc data copyout(s.x)
  #pragma acc data copy(s.y)
  #pragma acc data no_create(s)
  ;
}

//  EXE-HOST-caseDataMemberFullStruct-NOT: {{.}}
//      EXE-HOST-caseDataMemberFullStruct: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
// EXE-HOST-caseDataMemberFullStruct-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//
//           EXE-OFF-caseDataMemberFullStruct-NOT: {{.}}
//               EXE-OFF-caseDataMemberFullStruct: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//          EXE-OFF-caseDataMemberFullStruct-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//          EXE-OFF-caseDataMemberFullStruct-NEXT: acc_ev_enter_data_start
//          EXE-OFF-caseDataMemberFullStruct-NEXT:   acc_ev_alloc
//          EXE-OFF-caseDataMemberFullStruct-NEXT:   acc_ev_create
//          EXE-OFF-caseDataMemberFullStruct-NEXT:   acc_ev_enter_data_start
//    EXE-OFF-caseDataMemberFullStruct-ALLOC-NEXT:   Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//    EXE-OFF-caseDataMemberFullStruct-ALLOC-NEXT:   Libomptarget error: Call to getTargetPointer returned null pointer (device failure or illegal mapping).
//    EXE-OFF-caseDataMemberFullStruct-ALLOC-NEXT:   Libomptarget error: Consult {{.*}}
//    EXE-OFF-caseDataMemberFullStruct-ALLOC-NEXT:   {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                                      # An abort message usually follows.
//     EXE-OFF-caseDataMemberFullStruct-ALLOC-NOT:   Libomptarget
// EXE-OFF-caseDataMemberFullStruct-NO-ALLOC-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseDataMemberFullStruct-NO-ALLOC-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseDataMemberFullStruct-NO-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseDataMemberFullStruct-NO-ALLOC-NEXT:   acc_ev_free
CASE(caseDataMemberFullStruct) {
  struct S { int x; int y; int z; } s;
  PRINT_VAR_INFO(s.y);
  PRINT_VAR_INFO(s);
  #pragma acc data copyin(s.y)
  #pragma acc data no_create(s)
  ;
}

//   PRT-LABEL: {{.*}}caseDataSubarrayPresent{{.*}} {
//    PRT-NEXT:   int same[10], beg[10], mid[10], end[10];
//
//  PRT-A-NEXT:   #pragma acc data copy(same,beg,mid,end)
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: same,beg,mid,end)
//  PRT-A-NEXT:   #pragma acc data no_create(same[0:10],beg[0:3],mid[3:3],end[8:2])
// PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: same[0:10],beg[0:3],mid[3:3],end[8:2])
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: same,beg,mid,end)
// PRT-OA-NEXT:   // #pragma acc data copy(same,beg,mid,end)
//  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: same[0:10],beg[0:3],mid[3:3],end[8:2])
// PRT-OA-NEXT:   // #pragma acc data no_create(same[0:10],beg[0:3],mid[3:3],end[8:2])
//
//    PRT-NEXT:   ;
//
//  PRT-A-NEXT:   #pragma acc data copy(same[3:6],beg[2:5],mid[1:8],end[0:5])
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: same[3:6],beg[2:5],mid[1:8],end[0:5])
//  PRT-A-NEXT:   #pragma acc data no_create(same[3:6],beg[2:2],mid[3:3],end[4:1])
// PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: same[3:6],beg[2:2],mid[3:3],end[4:1])
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: same[3:6],beg[2:5],mid[1:8],end[0:5])
// PRT-OA-NEXT:   // #pragma acc data copy(same[3:6],beg[2:5],mid[1:8],end[0:5])
//  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: same[3:6],beg[2:2],mid[3:3],end[4:1])
// PRT-OA-NEXT:   // #pragma acc data no_create(same[3:6],beg[2:2],mid[3:3],end[4:1])
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
//
//  EXE-OFF-caseDataSubarrayPresent-NOT: {{.}}
//      EXE-OFF-caseDataSubarrayPresent: acc_ev_enter_data_start
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_create
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_create
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_create
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_create
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_enter_data_start
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseDataSubarrayPresent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_free
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_free
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_free
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_free
// EXE-OFF-caseDataSubarrayPresent-NEXT: acc_ev_enter_data_start
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_create
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_create
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_create
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_create
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_enter_data_start
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseDataSubarrayPresent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_free
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_free
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_free
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_free
//  EXE-OFF-caseDataSubarrayPresent-NOT: {{.}}
CASE(caseDataSubarrayPresent) {
  int same[10], beg[10], mid[10], end[10];
  #pragma acc data copy(same,beg,mid,end)
  #pragma acc data no_create(same[0:10],beg[0:3],mid[3:3],end[8:2])
  ;
  #pragma acc data copy(same[3:6],beg[2:5],mid[1:8],end[0:5])
  #pragma acc data no_create(same[3:6],beg[2:2],mid[3:3],end[4:1])
  ;
}

//   PRT-LABEL: {{.*}}caseDataSubarrayDisjoint{{.*}} {
//    PRT-NEXT:   int arr[4];
//
//  PRT-A-NEXT:   #pragma acc data copy(arr[0:2]){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: arr[0:2]){{$}}
//  PRT-A-NEXT:   #pragma acc data no_create(arr[2:2]){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: arr[2:2]){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: arr[0:2]){{$}}
// PRT-OA-NEXT:   // #pragma acc data copy(arr[0:2]){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: arr[2:2]){{$}}
// PRT-OA-NEXT:   // #pragma acc data no_create(arr[2:2]){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
//
//        EXE-OFF-caseDataSubarrayDisjoint-NOT: {{.}}
//            EXE-OFF-caseDataSubarrayDisjoint: acc_ev_enter_data_start
//       EXE-OFF-caseDataSubarrayDisjoint-NEXT:   acc_ev_alloc
//       EXE-OFF-caseDataSubarrayDisjoint-NEXT:   acc_ev_create
//       EXE-OFF-caseDataSubarrayDisjoint-NEXT:   acc_ev_enter_data_start
// EXE-OFF-caseDataSubarrayDisjoint-ALLOC-NEXT:     acc_ev_alloc
// EXE-OFF-caseDataSubarrayDisjoint-ALLOC-NEXT:     acc_ev_create
//       EXE-OFF-caseDataSubarrayDisjoint-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseDataSubarrayDisjoint-ALLOC-NEXT:     acc_ev_delete
// EXE-OFF-caseDataSubarrayDisjoint-ALLOC-NEXT:     acc_ev_free
//       EXE-OFF-caseDataSubarrayDisjoint-NEXT: acc_ev_exit_data_start
//       EXE-OFF-caseDataSubarrayDisjoint-NEXT:   acc_ev_delete
//       EXE-OFF-caseDataSubarrayDisjoint-NEXT:   acc_ev_free
//        EXE-OFF-caseDataSubarrayDisjoint-NOT: {{.}}
CASE(caseDataSubarrayDisjoint) {
  int arr[4];
  #pragma acc data copy(arr[0:2])
  #pragma acc data no_create(arr[2:2])
  ;
}

//   PRT-LABEL: {{.*}}caseDataSubarrayOverlapStart{{.*}} {
//    PRT-NEXT:   int arr[5];
//    PRT-NEXT:   {{(PRINT_SUBARRAY_INFO|fprintf)\(.*\);}}
//    PRT-NEXT:   {{(PRINT_SUBARRAY_INFO|fprintf)\(.*\);}}
//
//  PRT-A-NEXT:   #pragma acc data copyin(arr[1:2]){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,to: arr[1:2]){{$}}
//  PRT-A-NEXT:   #pragma acc data no_create(arr[0:2]){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: arr[0:2]){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,to: arr[1:2]){{$}}
// PRT-OA-NEXT:   // #pragma acc data copyin(arr[1:2]){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: arr[0:2]){{$}}
// PRT-OA-NEXT:   // #pragma acc data no_create(arr[0:2]){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
//
//  EXE-HOST-caseDataSubarrayOverlapStart-NOT: {{.}}
//      EXE-HOST-caseDataSubarrayOverlapStart: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
// EXE-HOST-caseDataSubarrayOverlapStart-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//
//           EXE-OFF-caseDataSubarrayOverlapStart-NOT: {{.}}
//               EXE-OFF-caseDataSubarrayOverlapStart: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//          EXE-OFF-caseDataSubarrayOverlapStart-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//          EXE-OFF-caseDataSubarrayOverlapStart-NEXT: acc_ev_enter_data_start
//          EXE-OFF-caseDataSubarrayOverlapStart-NEXT:   acc_ev_alloc
//          EXE-OFF-caseDataSubarrayOverlapStart-NEXT:   acc_ev_create
//          EXE-OFF-caseDataSubarrayOverlapStart-NEXT:   acc_ev_enter_data_start
//    EXE-OFF-caseDataSubarrayOverlapStart-ALLOC-NEXT:   Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//    EXE-OFF-caseDataSubarrayOverlapStart-ALLOC-NEXT:   Libomptarget error: Call to getTargetPointer returned null pointer (device failure or illegal mapping).
//    EXE-OFF-caseDataSubarrayOverlapStart-ALLOC-NEXT:   Libomptarget error: Consult {{.*}}
//    EXE-OFF-caseDataSubarrayOverlapStart-ALLOC-NEXT:   {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                                       # An abort message usually follows.
//     EXE-OFF-caseDataSubarrayOverlapStart-ALLOC-NOT:   Libomptarget
// EXE-OFF-caseDataSubarrayOverlapStart-NO-ALLOC-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseDataSubarrayOverlapStart-NO-ALLOC-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseDataSubarrayOverlapStart-NO-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseDataSubarrayOverlapStart-NO-ALLOC-NEXT:   acc_ev_free
CASE(caseDataSubarrayOverlapStart) {
  int arr[5];
  PRINT_SUBARRAY_INFO(arr, 1, 2);
  PRINT_SUBARRAY_INFO(arr, 0, 2);
  #pragma acc data copyin(arr[1:2])
  #pragma acc data no_create(arr[0:2])
  ;
}

//   PRT-LABEL: {{.*}}caseDataSubarrayOverlapEnd{{.*}} {
//    PRT-NEXT:   int arr[5];
//    PRT-NEXT:   {{(PRINT_SUBARRAY_INFO|fprintf)\(.*\);}}
//    PRT-NEXT:   {{(PRINT_SUBARRAY_INFO|fprintf)\(.*\);}}
//
//  PRT-A-NEXT:   #pragma acc data copyin(arr[1:2]){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,to: arr[1:2]){{$}}
//  PRT-A-NEXT:   #pragma acc data no_create(arr[2:2]){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: arr[2:2]){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,to: arr[1:2]){{$}}
// PRT-OA-NEXT:   // #pragma acc data copyin(arr[1:2]){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: arr[2:2]){{$}}
// PRT-OA-NEXT:   // #pragma acc data no_create(arr[2:2]){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
//
//  EXE-HOST-caseDataSubarrayOverlapEnd-NOT: {{.}}
//      EXE-HOST-caseDataSubarrayOverlapEnd: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
// EXE-HOST-caseDataSubarrayOverlapEnd-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//
//           EXE-OFF-caseDataSubarrayOverlapEnd-NOT: {{.}}
//               EXE-OFF-caseDataSubarrayOverlapEnd: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//          EXE-OFF-caseDataSubarrayOverlapEnd-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//          EXE-OFF-caseDataSubarrayOverlapEnd-NEXT: acc_ev_enter_data_start
//          EXE-OFF-caseDataSubarrayOverlapEnd-NEXT:   acc_ev_alloc
//          EXE-OFF-caseDataSubarrayOverlapEnd-NEXT:   acc_ev_create
//          EXE-OFF-caseDataSubarrayOverlapEnd-NEXT:   acc_ev_enter_data_start
//    EXE-OFF-caseDataSubarrayOverlapEnd-ALLOC-NEXT:   Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//    EXE-OFF-caseDataSubarrayOverlapEnd-ALLOC-NEXT:   Libomptarget error: Call to getTargetPointer returned null pointer (device failure or illegal mapping).
//    EXE-OFF-caseDataSubarrayOverlapEnd-ALLOC-NEXT:   Libomptarget error: Consult {{.*}}
//    EXE-OFF-caseDataSubarrayOverlapEnd-ALLOC-NEXT:   {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                                     # An abort message usually follows.
//     EXE-OFF-caseDataSubarrayOverlapEnd-ALLOC-NOT:   Libomptarget
// EXE-OFF-caseDataSubarrayOverlapEnd-NO-ALLOC-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseDataSubarrayOverlapEnd-NO-ALLOC-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseDataSubarrayOverlapEnd-NO-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseDataSubarrayOverlapEnd-NO-ALLOC-NEXT:   acc_ev_free
CASE(caseDataSubarrayOverlapEnd) {
  int arr[5];
  PRINT_SUBARRAY_INFO(arr, 1, 2);
  PRINT_SUBARRAY_INFO(arr, 2, 2);
  #pragma acc data copyin(arr[1:2])
  #pragma acc data no_create(arr[2:2])
  ;
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
//  PRT-A-NEXT:   #pragma acc data no_create(arr[0:4]){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: arr[0:4]){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,from: arr[0:2]){{$}}
// PRT-OA-NEXT:   // #pragma acc data copyout(arr[0:2]){{$}}
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: arr[2:2]){{$}}
// PRT-OA-NEXT:   // #pragma acc data copy(arr[2:2]){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: arr[0:4]){{$}}
// PRT-OA-NEXT:   // #pragma acc data no_create(arr[0:4]){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
//
//  EXE-HOST-caseDataSubarrayConcat2-NOT: {{.}}
//      EXE-HOST-caseDataSubarrayConcat2: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
// EXE-HOST-caseDataSubarrayConcat2-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//
//           EXE-OFF-caseDataSubarrayConcat2-NOT: {{.}}
//               EXE-OFF-caseDataSubarrayConcat2: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//          EXE-OFF-caseDataSubarrayConcat2-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//          EXE-OFF-caseDataSubarrayConcat2-NEXT: acc_ev_enter_data_start
//          EXE-OFF-caseDataSubarrayConcat2-NEXT:   acc_ev_alloc
//          EXE-OFF-caseDataSubarrayConcat2-NEXT:   acc_ev_create
//          EXE-OFF-caseDataSubarrayConcat2-NEXT:   acc_ev_enter_data_start
//          EXE-OFF-caseDataSubarrayConcat2-NEXT:     acc_ev_alloc
//          EXE-OFF-caseDataSubarrayConcat2-NEXT:     acc_ev_create
//          EXE-OFF-caseDataSubarrayConcat2-NEXT:     acc_ev_enter_data_start
//    EXE-OFF-caseDataSubarrayConcat2-ALLOC-NEXT:     Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//    EXE-OFF-caseDataSubarrayConcat2-ALLOC-NEXT:     Libomptarget error: Call to getTargetPointer returned null pointer (device failure or illegal mapping).
//    EXE-OFF-caseDataSubarrayConcat2-ALLOC-NEXT:     Libomptarget error: Consult {{.*}}
//    EXE-OFF-caseDataSubarrayConcat2-ALLOC-NEXT:     {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                                    # An abort message usually follows.
//     EXE-OFF-caseDataSubarrayConcat2-ALLOC-NOT:     Libomptarget
// EXE-OFF-caseDataSubarrayConcat2-NO-ALLOC-NEXT:     acc_ev_exit_data_start
// EXE-OFF-caseDataSubarrayConcat2-NO-ALLOC-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseDataSubarrayConcat2-NO-ALLOC-NEXT:     acc_ev_delete
// EXE-OFF-caseDataSubarrayConcat2-NO-ALLOC-NEXT:     acc_ev_free
// EXE-OFF-caseDataSubarrayConcat2-NO-ALLOC-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseDataSubarrayConcat2-NO-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseDataSubarrayConcat2-NO-ALLOC-NEXT:   acc_ev_free
CASE(caseDataSubarrayConcat2) {
  int arr[4];
  PRINT_SUBARRAY_INFO(arr, 0, 2);
  PRINT_SUBARRAY_INFO(arr, 0, 4);
  #pragma acc data copyout(arr[0:2])
  #pragma acc data copy(arr[2:2])
  #pragma acc data no_create(arr[0:4])
  ;
}

//  EXE-HOST-caseDataSubarrayNonSubarray-NOT: {{.}}
//      EXE-HOST-caseDataSubarrayNonSubarray: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
// EXE-HOST-caseDataSubarrayNonSubarray-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//
//           EXE-OFF-caseDataSubarrayNonSubarray-NOT: {{.}}
//               EXE-OFF-caseDataSubarrayNonSubarray: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//          EXE-OFF-caseDataSubarrayNonSubarray-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//          EXE-OFF-caseDataSubarrayNonSubarray-NEXT: acc_ev_enter_data_start
//          EXE-OFF-caseDataSubarrayNonSubarray-NEXT:   acc_ev_alloc
//          EXE-OFF-caseDataSubarrayNonSubarray-NEXT:   acc_ev_create
//          EXE-OFF-caseDataSubarrayNonSubarray-NEXT:   acc_ev_enter_data_start
//    EXE-OFF-caseDataSubarrayNonSubarray-ALLOC-NEXT:   Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//    EXE-OFF-caseDataSubarrayNonSubarray-ALLOC-NEXT:   Libomptarget error: Call to getTargetPointer returned null pointer (device failure or illegal mapping).
//    EXE-OFF-caseDataSubarrayNonSubarray-ALLOC-NEXT:   Libomptarget error: Consult {{.*}}
//    EXE-OFF-caseDataSubarrayNonSubarray-ALLOC-NEXT:   {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                                      # An abort message usually follows.
//     EXE-OFF-caseDataSubarrayNonSubarray-ALLOC-NOT:   Libomptarget
// EXE-OFF-caseDataSubarrayNonSubarray-NO-ALLOC-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseDataSubarrayNonSubarray-NO-ALLOC-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseDataSubarrayNonSubarray-NO-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseDataSubarrayNonSubarray-NO-ALLOC-NEXT:   acc_ev_free
CASE(caseDataSubarrayNonSubarray) {
  int arr[5];
  PRINT_SUBARRAY_INFO(arr, 1, 2);
  PRINT_SUBARRAY_INFO(arr, 0, 5);
  #pragma acc data copyin(arr[1:2])
  #pragma acc data no_create(arr)
  ;
}

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseParallelScalarPresent
//       DMP: ACCDataDirective
//  DMP-NEXT:   ACCCopyClause
//  DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:   impl: OMPTargetDataDirective
//  DMP-NEXT:     OMPMapClause
//  DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'int'
//       DMP: ACCParallelDirective
//  DMP-NEXT:   ACCNoCreateClause
//  DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:   impl: OMPTargetTeamsDirective
//  DMP-NEXT:     OMPMapClause
//  DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'int'
//
//   PRT-LABEL: {{.*}}caseParallelScalarPresent{{.*}} {
//    PRT-NEXT:   int x;
//
//  PRT-A-NEXT:   #pragma acc data copy(x){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: x){{$}}
//  PRT-A-NEXT:   #pragma acc parallel no_create(x){{$}}
// PRT-AO-NEXT:   // #pragma omp target teams map([[NO_CREATE_MT]]: x) shared(x){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: x){{$}}
// PRT-OA-NEXT:   // #pragma acc data copy(x){{$}}
//  PRT-O-NEXT:   #pragma omp target teams map([[NO_CREATE_MT]]: x) shared(x){{$}}
// PRT-OA-NEXT:   // #pragma acc parallel no_create(x){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
//
//  EXE-OFF-caseParallelScalarPresent-NOT: {{.}}
//      EXE-OFF-caseParallelScalarPresent: acc_ev_enter_data_start
// EXE-OFF-caseParallelScalarPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelScalarPresent-NEXT:   acc_ev_create
// EXE-OFF-caseParallelScalarPresent-NEXT:   acc_ev_enter_data_start
// EXE-OFF-caseParallelScalarPresent-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseParallelScalarPresent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseParallelScalarPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelScalarPresent-NEXT:   acc_ev_free
//  EXE-OFF-caseParallelScalarPresent-NOT: {{.}}
CASE(caseParallelScalarPresent) {
  int x;
  #pragma acc data copy(x)
  #pragma acc parallel no_create(x)
  x = 5;
}

//        EXE-OFF-caseParallelScalarAbsent-NOT: {{.}}
//            EXE-OFF-caseParallelScalarAbsent: acc_ev_enter_data_start
// EXE-OFF-caseParallelScalarAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelScalarAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-OFF-caseParallelScalarAbsent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseParallelScalarAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelScalarAbsent-ALLOC-NEXT:   acc_ev_free
//        EXE-OFF-caseParallelScalarAbsent-NOT: {{.}}
CASE(caseParallelScalarAbsent) {
  int x;
  int use = 0;
  #pragma acc parallel no_create(x)
  if (use) x = 5;
}

//  EXE-OFF-caseParallelStructPresent-NOT: {{.}}
//      EXE-OFF-caseParallelStructPresent: acc_ev_enter_data_start
// EXE-OFF-caseParallelStructPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelStructPresent-NEXT:   acc_ev_create
// EXE-OFF-caseParallelStructPresent-NEXT:   acc_ev_enter_data_start
// EXE-OFF-caseParallelStructPresent-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseParallelStructPresent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseParallelStructPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelStructPresent-NEXT:   acc_ev_free
//  EXE-OFF-caseParallelStructPresent-NOT: {{.}}
CASE(caseParallelStructPresent) {
  struct S { int i; int j; } s;
  #pragma acc data copy(s)
  #pragma acc parallel no_create(s)
  s.i = 5;
}

//        EXE-OFF-caseParallelStructAbsent-NOT: {{.}}
//            EXE-OFF-caseParallelStructAbsent: acc_ev_enter_data_start
// EXE-OFF-caseParallelStructAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelStructAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-OFF-caseParallelStructAbsent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseParallelStructAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelStructAbsent-ALLOC-NEXT:   acc_ev_free
//        EXE-OFF-caseParallelStructAbsent-NOT: {{.}}
CASE(caseParallelStructAbsent) {
  struct S { int i; int j; } s;
  int use = 0;
  #pragma acc parallel no_create(s)
  if (use) s.i = 5;
}

//  EXE-OFF-caseParallelArrayPresent-NOT: {{.}}
//      EXE-OFF-caseParallelArrayPresent: acc_ev_enter_data_start
// EXE-OFF-caseParallelArrayPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelArrayPresent-NEXT:   acc_ev_create
// EXE-OFF-caseParallelArrayPresent-NEXT:   acc_ev_enter_data_start
// EXE-OFF-caseParallelArrayPresent-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseParallelArrayPresent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseParallelArrayPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelArrayPresent-NEXT:   acc_ev_free
//  EXE-OFF-caseParallelArrayPresent-NOT: {{.}}
CASE(caseParallelArrayPresent) {
  int arr[3];
  #pragma acc data copy(arr)
  #pragma acc parallel no_create(arr)
  arr[0] = 5;
}

//        EXE-OFF-caseParallelArrayAbsent-NOT: {{.}}
//            EXE-OFF-caseParallelArrayAbsent: acc_ev_enter_data_start
// EXE-OFF-caseParallelArrayAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelArrayAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-OFF-caseParallelArrayAbsent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseParallelArrayAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelArrayAbsent-ALLOC-NEXT:   acc_ev_free
//        EXE-OFF-caseParallelArrayAbsent-NOT: {{.}}
CASE(caseParallelArrayAbsent) {
  int arr[3];
  int use = 0;
  #pragma acc parallel no_create(arr)
  if (use) arr[0] = 5;
}

//  EXE-OFF-caseParallelMemberPresent-NOT: {{.}}
//      EXE-OFF-caseParallelMemberPresent: acc_ev_enter_data_start
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_create
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_create
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_create
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_create
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_enter_data_start
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseParallelMemberPresent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_free
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_free
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_free
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_free
// EXE-OFF-caseParallelMemberPresent-NEXT: acc_ev_enter_data_start
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_create
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_create
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_create
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_create
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_enter_data_start
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseParallelMemberPresent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_free
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_free
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_free
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelMemberPresent-NEXT:   acc_ev_free
//  EXE-OFF-caseParallelMemberPresent-NOT: {{.}}
CASE(caseParallelMemberPresent) {
  struct S { int x; int y; int z; };
  struct S same, beg, mid, end;
  #pragma acc data copy(same,beg,mid,end)
  #pragma acc parallel no_create(same.x,same.y,same.z,beg.x,mid.y,end.z)
  {
    same.x = 1; beg.x = 1; mid.y = 1; end.z = 1;
  }
  #pragma acc data copy(same.y,beg.y,beg.z,mid.x,mid.y,mid.z,end.x,end.y)
  #pragma acc parallel no_create(same.y,beg.y,mid.y,end.y)
  {
    same.y = 1; beg.y = 1; mid.y = 1; end.y = 1;
  }
}

//  EXE-OFF-caseParallelMemberAbsent-NOT: {{.}}
//                EXE-caseParallelMemberAbsent: allocations not expected
//       EXE-OFF-caseParallelMemberAbsent-NEXT: acc_ev_enter_data_start
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-OFF-caseParallelMemberAbsent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_free
//       EXE-OFF-caseParallelMemberAbsent-NEXT: acc_ev_enter_data_start
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-OFF-caseParallelMemberAbsent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_free
//       EXE-OFF-caseParallelMemberAbsent-NEXT: acc_ev_enter_data_start
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-OFF-caseParallelMemberAbsent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_free
//       EXE-OFF-caseParallelMemberAbsent-NEXT: acc_ev_enter_data_start
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-OFF-caseParallelMemberAbsent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_free
//       EXE-OFF-caseParallelMemberAbsent-NEXT: acc_ev_enter_data_start
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-OFF-caseParallelMemberAbsent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_free
//       EXE-OFF-caseParallelMemberAbsent-NEXT: acc_ev_enter_data_start
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-OFF-caseParallelMemberAbsent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_free
//       EXE-OFF-caseParallelMemberAbsent-NEXT: acc_ev_enter_data_start
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-OFF-caseParallelMemberAbsent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelMemberAbsent-ALLOC-NEXT:   acc_ev_free
//           EXE-caseParallelMemberAbsent-NEXT: allocations expected
//       EXE-OFF-caseParallelMemberAbsent-NEXT: acc_ev_enter_data_start
//       EXE-OFF-caseParallelMemberAbsent-NEXT:   acc_ev_alloc
//       EXE-OFF-caseParallelMemberAbsent-NEXT:   acc_ev_create
//       EXE-OFF-caseParallelMemberAbsent-NEXT: acc_ev_exit_data_start
//       EXE-OFF-caseParallelMemberAbsent-NEXT:   acc_ev_delete
//       EXE-OFF-caseParallelMemberAbsent-NEXT:   acc_ev_free
//       EXE-OFF-caseParallelMemberAbsent-NEXT: acc_ev_enter_data_start
//       EXE-OFF-caseParallelMemberAbsent-NEXT:   acc_ev_alloc
//       EXE-OFF-caseParallelMemberAbsent-NEXT:   acc_ev_create
//       EXE-OFF-caseParallelMemberAbsent-NEXT: acc_ev_exit_data_start
//       EXE-OFF-caseParallelMemberAbsent-NEXT:   acc_ev_delete
//       EXE-OFF-caseParallelMemberAbsent-NEXT:   acc_ev_free
//        EXE-OFF-caseParallelMemberAbsent-NOT: {{.}}
CASE(caseParallelMemberAbsent) {
  struct S { int x; int y; int z; } s;
  int use = 0;
  fprintf(stderr, "allocations not expected\n");
  #pragma acc parallel no_create(s.x)
  if (use) s.x = 1;
  #pragma acc parallel no_create(s.y)
  if (use) s.y = 1;
  #pragma acc parallel no_create(s.z)
  if (use) s.z = 1;
  // Clang OpenMP codegen generates a map type entry for the struct as a whole
  // when multiple members are specified, so we need to be sure the struct as a
  // whole is handled as ompx_no_alloc.
  #pragma acc parallel no_create(s.x) no_create(s.y)
  if (use) s.x = 1;
  #pragma acc parallel no_create(s.y) no_create(s.z)
  if (use) s.y = 1;
  #pragma acc parallel no_create(s.x) no_create(s.z)
  if (use) s.z = 1;
  #pragma acc parallel no_create(s.x, s.y, s.z)
  if (use) s.y = 1;
  fprintf(stderr, "allocations expected\n");
  // If only some members have ompx_no_alloc, then the struct as a whole must
  // *not* be handled as ompx_no_alloc.
  #pragma acc parallel no_create(s.x) copy(s.y)
  if (use) s.x = 1;
  #pragma acc parallel copy(s.x) no_create(s.y)
  if (use) s.y = 1;
}

//        EXE-OFF-caseParallelMembersDisjoint-NOT: {{.}}
//            EXE-OFF-caseParallelMembersDisjoint: acc_ev_enter_data_start
//       EXE-OFF-caseParallelMembersDisjoint-NEXT:   acc_ev_alloc
//       EXE-OFF-caseParallelMembersDisjoint-NEXT:   acc_ev_create
//       EXE-OFF-caseParallelMembersDisjoint-NEXT:   acc_ev_enter_data_start
// EXE-OFF-caseParallelMembersDisjoint-ALLOC-NEXT:     acc_ev_alloc
// EXE-OFF-caseParallelMembersDisjoint-ALLOC-NEXT:     acc_ev_create
//       EXE-OFF-caseParallelMembersDisjoint-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseParallelMembersDisjoint-ALLOC-NEXT:     acc_ev_delete
// EXE-OFF-caseParallelMembersDisjoint-ALLOC-NEXT:     acc_ev_free
//       EXE-OFF-caseParallelMembersDisjoint-NEXT: acc_ev_exit_data_start
//       EXE-OFF-caseParallelMembersDisjoint-NEXT:   acc_ev_delete
//       EXE-OFF-caseParallelMembersDisjoint-NEXT:   acc_ev_free
//        EXE-OFF-caseParallelMembersDisjoint-NOT: {{.}}
CASE(caseParallelMembersDisjoint) {
  struct S { int w; int x; int y; int z; } s;
  int use = 0;
  #pragma acc data copy(s.w, s.x)
  #pragma acc parallel no_create(s.y, s.z)
  if (use) s.y = 1;
}

//  EXE-HOST-caseParallelMembersConcat2-NOT: {{.}}
//      EXE-HOST-caseParallelMembersConcat2: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
// EXE-HOST-caseParallelMembersConcat2-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//
//           EXE-OFF-caseParallelMembersConcat2-NOT: {{.}}
//               EXE-OFF-caseParallelMembersConcat2: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//          EXE-OFF-caseParallelMembersConcat2-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//          EXE-OFF-caseParallelMembersConcat2-NEXT: acc_ev_enter_data_start
//          EXE-OFF-caseParallelMembersConcat2-NEXT:   acc_ev_alloc
//          EXE-OFF-caseParallelMembersConcat2-NEXT:   acc_ev_create
//          EXE-OFF-caseParallelMembersConcat2-NEXT:   acc_ev_enter_data_start
//          EXE-OFF-caseParallelMembersConcat2-NEXT:     acc_ev_alloc
//          EXE-OFF-caseParallelMembersConcat2-NEXT:     acc_ev_create
//          EXE-OFF-caseParallelMembersConcat2-NEXT:     acc_ev_enter_data_start
//    EXE-OFF-caseParallelMembersConcat2-ALLOC-NEXT:     Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//                                                        # FIXME: Names like getTargetPointer are meaningless to users.
//    EXE-OFF-caseParallelMembersConcat2-ALLOC-NEXT:     Libomptarget error: Call to getTargetPointer returned null pointer (device failure or illegal mapping).
//    EXE-OFF-caseParallelMembersConcat2-ALLOC-NEXT:     Libomptarget error: Call to targetDataBegin failed, abort target.
//    EXE-OFF-caseParallelMembersConcat2-ALLOC-NEXT:     Libomptarget error: Failed to process data before launching the kernel.
//    EXE-OFF-caseParallelMembersConcat2-ALLOC-NEXT:     Libomptarget error: Consult {{.*}}
//    EXE-OFF-caseParallelMembersConcat2-ALLOC-NEXT:     {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                                        # An abort message usually follows.
//     EXE-OFF-caseParallelMembersConcat2-ALLOC-NOT:     Libomptarget
// EXE-OFF-caseParallelMembersConcat2-NO-ALLOC-NEXT:     acc_ev_exit_data_start
// EXE-OFF-caseParallelMembersConcat2-NO-ALLOC-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseParallelMembersConcat2-NO-ALLOC-NEXT:     acc_ev_delete
// EXE-OFF-caseParallelMembersConcat2-NO-ALLOC-NEXT:     acc_ev_free
// EXE-OFF-caseParallelMembersConcat2-NO-ALLOC-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseParallelMembersConcat2-NO-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelMembersConcat2-NO-ALLOC-NEXT:   acc_ev_free
CASE(caseParallelMembersConcat2) {
  struct S { int x; int y; } s;
  PRINT_VAR_INFO(s.x);
  PRINT_VAR_INFO(s);
  int use = 0;
  #pragma acc data copyout(s.x)
  #pragma acc data copy(s.y)
  #pragma acc parallel no_create(s)
  if (use) s.x = 1;
}

//  EXE-HOST-caseParallelMemberFullStruct-NOT: {{.}}
//      EXE-HOST-caseParallelMemberFullStruct: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
// EXE-HOST-caseParallelMemberFullStruct-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//
//           EXE-OFF-caseParallelMemberFullStruct-NOT: {{.}}
//               EXE-OFF-caseParallelMemberFullStruct: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//          EXE-OFF-caseParallelMemberFullStruct-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//          EXE-OFF-caseParallelMemberFullStruct-NEXT: acc_ev_enter_data_start
//          EXE-OFF-caseParallelMemberFullStruct-NEXT:   acc_ev_alloc
//          EXE-OFF-caseParallelMemberFullStruct-NEXT:   acc_ev_create
//          EXE-OFF-caseParallelMemberFullStruct-NEXT:   acc_ev_enter_data_start
//    EXE-OFF-caseParallelMemberFullStruct-ALLOC-NEXT:   Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//                                                          # FIXME: Names like getTargetPointer are meaningless to users.
//    EXE-OFF-caseParallelMemberFullStruct-ALLOC-NEXT:   Libomptarget error: Call to getTargetPointer returned null pointer (device failure or illegal mapping).
//    EXE-OFF-caseParallelMemberFullStruct-ALLOC-NEXT:   Libomptarget error: Call to targetDataBegin failed, abort target.
//    EXE-OFF-caseParallelMemberFullStruct-ALLOC-NEXT:   Libomptarget error: Failed to process data before launching the kernel.
//    EXE-OFF-caseParallelMemberFullStruct-ALLOC-NEXT:   Libomptarget error: Consult {{.*}}
//    EXE-OFF-caseParallelMemberFullStruct-ALLOC-NEXT:   {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                                          # An abort message usually follows.
//     EXE-OFF-caseParallelMemberFullStruct-ALLOC-NOT:   Libomptarget
// EXE-OFF-caseParallelMemberFullStruct-NO-ALLOC-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseParallelMemberFullStruct-NO-ALLOC-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseParallelMemberFullStruct-NO-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelMemberFullStruct-NO-ALLOC-NEXT:   acc_ev_free
CASE(caseParallelMemberFullStruct) {
  struct S { int x; int y; int z; } s;
  PRINT_VAR_INFO(s.y);
  PRINT_VAR_INFO(s);
  int use = 0;
  #pragma acc data copy(s.y)
  #pragma acc parallel no_create(s)
  if (use) s.y = 1;
}

//  EXE-OFF-caseParallelSubarrayPresent-NOT: {{.}}
//      EXE-OFF-caseParallelSubarrayPresent: acc_ev_enter_data_start
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_create
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_create
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_create
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_create
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_enter_data_start
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseParallelSubarrayPresent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_free
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_free
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_free
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_free
// EXE-OFF-caseParallelSubarrayPresent-NEXT: acc_ev_enter_data_start
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_create
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_create
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_create
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_create
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_enter_data_start
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseParallelSubarrayPresent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_free
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_free
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_free
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_free
//  EXE-OFF-caseParallelSubarrayPresent-NOT: {{.}}
CASE(caseParallelSubarrayPresent) {
  int same[10], beg[10], mid[10], end[10];
  #pragma acc data copy(same,beg,mid,end)
  #pragma acc parallel no_create(same[0:10],beg[0:3],mid[3:3],end[8:2])
  {
    same[0] = 1; beg[0] = 1; mid[3] = 1; end[8] = 1;
  }
  #pragma acc data copy(same[3:6],beg[2:5],mid[1:8],end[0:5])
  #pragma acc parallel no_create(same[3:6],beg[2:2],mid[3:3],end[4:1])
  {
    same[3] = 1; beg[2] = 1; mid[3] = 1; end[4] = 1;
  }
}

//        EXE-OFF-caseParallelSubarrayDisjoint-NOT: {{.}}
//            EXE-OFF-caseParallelSubarrayDisjoint: acc_ev_enter_data_start
//       EXE-OFF-caseParallelSubarrayDisjoint-NEXT:   acc_ev_alloc
//       EXE-OFF-caseParallelSubarrayDisjoint-NEXT:   acc_ev_create
//       EXE-OFF-caseParallelSubarrayDisjoint-NEXT:   acc_ev_enter_data_start
// EXE-OFF-caseParallelSubarrayDisjoint-ALLOC-NEXT:     acc_ev_alloc
// EXE-OFF-caseParallelSubarrayDisjoint-ALLOC-NEXT:     acc_ev_create
//       EXE-OFF-caseParallelSubarrayDisjoint-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseParallelSubarrayDisjoint-ALLOC-NEXT:     acc_ev_delete
// EXE-OFF-caseParallelSubarrayDisjoint-ALLOC-NEXT:     acc_ev_free
//       EXE-OFF-caseParallelSubarrayDisjoint-NEXT: acc_ev_exit_data_start
//       EXE-OFF-caseParallelSubarrayDisjoint-NEXT:   acc_ev_delete
//       EXE-OFF-caseParallelSubarrayDisjoint-NEXT:   acc_ev_free
//        EXE-OFF-caseParallelSubarrayDisjoint-NOT: {{.}}
CASE(caseParallelSubarrayDisjoint) {
  int arr[4];
  int use = 0;
  #pragma acc data copy(arr[0:2])
  #pragma acc parallel no_create(arr[2:2])
  if (use) arr[2] = 1;
}

//  EXE-HOST-caseParallelSubarrayOverlapStart-NOT: {{.}}
//      EXE-HOST-caseParallelSubarrayOverlapStart: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
// EXE-HOST-caseParallelSubarrayOverlapStart-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//
//
//           EXE-OFF-caseParallelSubarrayOverlapStart-NOT: {{.}}
//               EXE-OFF-caseParallelSubarrayOverlapStart: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//          EXE-OFF-caseParallelSubarrayOverlapStart-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//               EXE-OFF-caseParallelSubarrayOverlapStart: acc_ev_enter_data_start
//          EXE-OFF-caseParallelSubarrayOverlapStart-NEXT:   acc_ev_alloc
//          EXE-OFF-caseParallelSubarrayOverlapStart-NEXT:   acc_ev_create
//          EXE-OFF-caseParallelSubarrayOverlapStart-NEXT:   acc_ev_enter_data_start
//    EXE-OFF-caseParallelSubarrayOverlapStart-ALLOC-NEXT:   Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//                                                           # FIXME: Names like getTargetPointer are meaningless to users.
//    EXE-OFF-caseParallelSubarrayOverlapStart-ALLOC-NEXT:   Libomptarget error: Call to getTargetPointer returned null pointer (device failure or illegal mapping).
//    EXE-OFF-caseParallelSubarrayOverlapStart-ALLOC-NEXT:   Libomptarget error: Call to targetDataBegin failed, abort target.
//    EXE-OFF-caseParallelSubarrayOverlapStart-ALLOC-NEXT:   Libomptarget error: Failed to process data before launching the kernel.
//    EXE-OFF-caseParallelSubarrayOverlapStart-ALLOC-NEXT:   Libomptarget error: Consult {{.*}}
//    EXE-OFF-caseParallelSubarrayOverlapStart-ALLOC-NEXT:   {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                                           # An abort message usually follows.
//     EXE-OFF-caseParallelSubarrayOverlapStart-ALLOC-NOT:   Libomptarget
// EXE-OFF-caseParallelSubarrayOverlapStart-NO-ALLOC-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseParallelSubarrayOverlapStart-NO-ALLOC-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseParallelSubarrayOverlapStart-NO-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelSubarrayOverlapStart-NO-ALLOC-NEXT:   acc_ev_free
CASE(caseParallelSubarrayOverlapStart) {
  int arr[10];
  PRINT_SUBARRAY_INFO(arr, 5, 4);
  PRINT_SUBARRAY_INFO(arr, 4, 4);
  int use = 0;
  #pragma acc data copyin(arr[5:4])
  #pragma acc parallel no_create(arr[4:4])
  if (use) arr[4] = 1;
}

//  EXE-HOST-caseParallelSubarrayOverlapEnd-NOT: {{.}}
//      EXE-HOST-caseParallelSubarrayOverlapEnd: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
// EXE-HOST-caseParallelSubarrayOverlapEnd-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//
//           EXE-OFF-caseParallelSubarrayOverlapEnd-NOT: {{.}}
//               EXE-OFF-caseParallelSubarrayOverlapEnd: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//          EXE-OFF-caseParallelSubarrayOverlapEnd-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//          EXE-OFF-caseParallelSubarrayOverlapEnd-NEXT: acc_ev_enter_data_start
//          EXE-OFF-caseParallelSubarrayOverlapEnd-NEXT:   acc_ev_alloc
//          EXE-OFF-caseParallelSubarrayOverlapEnd-NEXT:   acc_ev_create
//          EXE-OFF-caseParallelSubarrayOverlapEnd-NEXT:   acc_ev_enter_data_start
//    EXE-OFF-caseParallelSubarrayOverlapEnd-ALLOC-NEXT:   Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//                                                         # FIXME: Names like getTargetPointer are meaningless to users.
//    EXE-OFF-caseParallelSubarrayOverlapEnd-ALLOC-NEXT:   Libomptarget error: Call to getTargetPointer returned null pointer (device failure or illegal mapping).
//    EXE-OFF-caseParallelSubarrayOverlapEnd-ALLOC-NEXT:   Libomptarget error: Call to targetDataBegin failed, abort target.
//    EXE-OFF-caseParallelSubarrayOverlapEnd-ALLOC-NEXT:   Libomptarget error: Failed to process data before launching the kernel.
//    EXE-OFF-caseParallelSubarrayOverlapEnd-ALLOC-NEXT:   Libomptarget error: Consult {{.*}}
//    EXE-OFF-caseParallelSubarrayOverlapEnd-ALLOC-NEXT:   {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                                         # An abort message usually follows.
//     EXE-OFF-caseParallelSubarrayOverlapEnd-ALLOC-NOT:   Libomptarget
// EXE-OFF-caseParallelSubarrayOverlapEnd-NO-ALLOC-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseParallelSubarrayOverlapEnd-NO-ALLOC-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseParallelSubarrayOverlapEnd-NO-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelSubarrayOverlapEnd-NO-ALLOC-NEXT:   acc_ev_free
CASE(caseParallelSubarrayOverlapEnd) {
  int arr[10];
  PRINT_SUBARRAY_INFO(arr, 3, 4);
  PRINT_SUBARRAY_INFO(arr, 4, 4);
  int use = 0;
  #pragma acc data copyin(arr[3:4])
  #pragma acc parallel no_create(arr[4:4])
  if (use) arr[4] = 1;
}

//  EXE-HOST-caseParallelSubarrayConcat2-NOT: {{.}}
//      EXE-HOST-caseParallelSubarrayConcat2: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
// EXE-HOST-caseParallelSubarrayConcat2-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//
//           EXE-OFF-caseParallelSubarrayConcat2-NOT: {{.}}
//               EXE-OFF-caseParallelSubarrayConcat2: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//          EXE-OFF-caseParallelSubarrayConcat2-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//          EXE-OFF-caseParallelSubarrayConcat2-NEXT: acc_ev_enter_data_start
//          EXE-OFF-caseParallelSubarrayConcat2-NEXT:   acc_ev_alloc
//          EXE-OFF-caseParallelSubarrayConcat2-NEXT:   acc_ev_create
//          EXE-OFF-caseParallelSubarrayConcat2-NEXT:   acc_ev_enter_data_start
//          EXE-OFF-caseParallelSubarrayConcat2-NEXT:     acc_ev_alloc
//          EXE-OFF-caseParallelSubarrayConcat2-NEXT:     acc_ev_create
//          EXE-OFF-caseParallelSubarrayConcat2-NEXT:     acc_ev_enter_data_start
//    EXE-OFF-caseParallelSubarrayConcat2-ALLOC-NEXT:     Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//                                                        # FIXME: Names like getTargetPointer are meaningless to users.
//    EXE-OFF-caseParallelSubarrayConcat2-ALLOC-NEXT:     Libomptarget error: Call to getTargetPointer returned null pointer (device failure or illegal mapping).
//    EXE-OFF-caseParallelSubarrayConcat2-ALLOC-NEXT:     Libomptarget error: Call to targetDataBegin failed, abort target.
//    EXE-OFF-caseParallelSubarrayConcat2-ALLOC-NEXT:     Libomptarget error: Failed to process data before launching the kernel.
//    EXE-OFF-caseParallelSubarrayConcat2-ALLOC-NEXT:     Libomptarget error: Consult {{.*}}
//    EXE-OFF-caseParallelSubarrayConcat2-ALLOC-NEXT:     {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                                        # An abort message usually follows.
//     EXE-OFF-caseParallelSubarrayConcat2-ALLOC-NOT:     Libomptarget
// EXE-OFF-caseParallelSubarrayConcat2-NO-ALLOC-NEXT:     acc_ev_exit_data_start
// EXE-OFF-caseParallelSubarrayConcat2-NO-ALLOC-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseParallelSubarrayConcat2-NO-ALLOC-NEXT:     acc_ev_delete
// EXE-OFF-caseParallelSubarrayConcat2-NO-ALLOC-NEXT:     acc_ev_free
// EXE-OFF-caseParallelSubarrayConcat2-NO-ALLOC-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseParallelSubarrayConcat2-NO-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelSubarrayConcat2-NO-ALLOC-NEXT:   acc_ev_free
CASE(caseParallelSubarrayConcat2) {
  int arr[4];
  PRINT_SUBARRAY_INFO(arr, 0, 2);
  PRINT_SUBARRAY_INFO(arr, 0, 4);
  int use = 0;
  #pragma acc data copyout(arr[0:2])
  #pragma acc data copy(arr[2:2])
  #pragma acc parallel no_create(arr[0:4])
  if (use) arr[0] = 1;
}

//  EXE-HOST-caseParallelSubarrayNonSubarray-NOT: {{.}}
//      EXE-HOST-caseParallelSubarrayNonSubarray: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
// EXE-HOST-caseParallelSubarrayNonSubarray-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//
//           EXE-OFF-caseParallelSubarrayNonSubarray-NOT: {{.}}
//               EXE-OFF-caseParallelSubarrayNonSubarray: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//          EXE-OFF-caseParallelSubarrayNonSubarray-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//          EXE-OFF-caseParallelSubarrayNonSubarray-NEXT: acc_ev_enter_data_start
//          EXE-OFF-caseParallelSubarrayNonSubarray-NEXT:   acc_ev_alloc
//          EXE-OFF-caseParallelSubarrayNonSubarray-NEXT:   acc_ev_create
//          EXE-OFF-caseParallelSubarrayNonSubarray-NEXT:   acc_ev_enter_data_start
//    EXE-OFF-caseParallelSubarrayNonSubarray-ALLOC-NEXT:   Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//                                                          # FIXME: Names like getTargetPointer are meaningless to users.
//    EXE-OFF-caseParallelSubarrayNonSubarray-ALLOC-NEXT:   Libomptarget error: Call to getTargetPointer returned null pointer (device failure or illegal mapping).
//    EXE-OFF-caseParallelSubarrayNonSubarray-ALLOC-NEXT:   Libomptarget error: Call to targetDataBegin failed, abort target.
//    EXE-OFF-caseParallelSubarrayNonSubarray-ALLOC-NEXT:   Libomptarget error: Failed to process data before launching the kernel.
//    EXE-OFF-caseParallelSubarrayNonSubarray-ALLOC-NEXT:   Libomptarget error: Consult {{.*}}
//    EXE-OFF-caseParallelSubarrayNonSubarray-ALLOC-NEXT:   {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                                          # An abort message usually follows.
//     EXE-OFF-caseParallelSubarrayNonSubarray-ALLOC-NOT:   Libomptarget
// EXE-OFF-caseParallelSubarrayNonSubarray-NO-ALLOC-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseParallelSubarrayNonSubarray-NO-ALLOC-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseParallelSubarrayNonSubarray-NO-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelSubarrayNonSubarray-NO-ALLOC-NEXT:   acc_ev_free
CASE(caseParallelSubarrayNonSubarray) {
  int arr[4];
  PRINT_SUBARRAY_INFO(arr, 1, 2);
  PRINT_SUBARRAY_INFO(arr, 0, 4);
  int use = 0;
  #pragma acc data copy(arr[1:2])
  #pragma acc parallel no_create(arr)
  if (use) arr[0] = 1;
}

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseParallelLoopScalarPresent
//       DMP: ACCDataDirective
//  DMP-NEXT:   ACCCopyClause
//  DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:   impl: OMPTargetDataDirective
//  DMP-NEXT:     OMPMapClause
//  DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'int'
//       DMP:   ACCParallelLoopDirective
//  DMP-NEXT:     ACCNoCreateClause
//  DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:     effect: ACCParallelDirective
//  DMP-NEXT:       ACCNoCreateClause
//  DMP-NEXT:         DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:         DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:       impl: OMPTargetTeamsDirective
//  DMP-NEXT:         OMPMapClause
//  DMP-NEXT:           DeclRefExpr {{.*}} 'x' 'int'
//       DMP:       ACCLoopDirective
//  DMP-NEXT:         ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:         ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:           DeclRefExpr {{.*}} 'x' 'int'
//  DMP-NEXT:         ACCGangClause {{.*}} <implicit>
//  DMP-NEXT:         impl: OMPDistributeDirective
//  DMP-NEXT:           OMPSharedClause {{.*}} <implicit>
//  DMP-NEXT:             DeclRefExpr {{.*}} 'x' 'int'
//   DMP-NOT:           OMP
//       DMP:           ForStmt
//
//   PRT-LABEL: {{.*}}caseParallelLoopScalarPresent{{.*}} {
//    PRT-NEXT:   int x;
//
//  PRT-A-NEXT:   #pragma acc data copy(x){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: x){{$}}
//  PRT-A-NEXT:   #pragma acc parallel loop no_create(x){{$}}
// PRT-AO-NEXT:   // #pragma omp target teams map([[NO_CREATE_MT]]: x) shared(x){{$}}
// PRT-AO-NEXT:   // #pragma omp distribute{{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: x){{$}}
// PRT-OA-NEXT:   // #pragma acc data copy(x){{$}}
//  PRT-O-NEXT:   #pragma omp target teams map([[NO_CREATE_MT]]: x) shared(x){{$}}
//  PRT-O-NEXT:   #pragma omp distribute{{$}}
// PRT-OA-NEXT:   // #pragma acc parallel loop no_create(x){{$}}
//
//    PRT-NEXT:   for ({{.*}})
//    PRT-NEXT:     ;
//    PRT-NEXT: }
//
//  EXE-OFF-caseParallelLoopScalarPresent-NOT: {{.}}
//      EXE-OFF-caseParallelLoopScalarPresent: acc_ev_enter_data_start
// EXE-OFF-caseParallelLoopScalarPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelLoopScalarPresent-NEXT:   acc_ev_create
// EXE-OFF-caseParallelLoopScalarPresent-NEXT:   acc_ev_enter_data_start
// EXE-OFF-caseParallelLoopScalarPresent-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseParallelLoopScalarPresent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseParallelLoopScalarPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelLoopScalarPresent-NEXT:   acc_ev_free
//  EXE-OFF-caseParallelLoopScalarPresent-NOT: {{.}}
CASE(caseParallelLoopScalarPresent) {
  int x;
  #pragma acc data copy(x)
  #pragma acc parallel loop no_create(x)
  for (int i = 0; i < 2; ++i)
  x = 1;
}

//        EXE-OFF-caseParallelLoopScalarAbsent-NOT: {{.}}
//            EXE-OFF-caseParallelLoopScalarAbsent: acc_ev_enter_data_start
// EXE-OFF-caseParallelLoopScalarAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseParallelLoopScalarAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-OFF-caseParallelLoopScalarAbsent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseParallelLoopScalarAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelLoopScalarAbsent-ALLOC-NEXT:   acc_ev_free
//        EXE-OFF-caseParallelLoopScalarAbsent-NOT: {{.}}
CASE(caseParallelLoopScalarAbsent) {
  int x;
  int use = 0;
  #pragma acc parallel loop no_create(x)
  for (int i = 0; i < 2; ++i)
  if (use) x = 1;
}

//  EXE-OFF-caseConstPresent-NOT: {{.}}
//      EXE-OFF-caseConstPresent: acc_ev_enter_data_start
// EXE-OFF-caseConstPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseConstPresent-NEXT:   acc_ev_create
// EXE-OFF-caseConstPresent-NEXT:   acc_ev_enter_data_start
// EXE-OFF-caseConstPresent-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseConstPresent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseConstPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseConstPresent-NEXT:   acc_ev_free
//  EXE-OFF-caseConstPresent-NOT: {{.}}
CASE(caseConstPresent) {
  const int x;
  int y;
  #pragma acc data copy(x)
  #pragma acc parallel loop no_create(x)
  for (int i = 0; i < 2; ++i)
  y = x;
}

//        EXE-OFF-caseConstAbsent-NOT: {{.}}
//            EXE-OFF-caseConstAbsent: acc_ev_enter_data_start
// EXE-OFF-caseConstAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseConstAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-OFF-caseConstAbsent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseConstAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseConstAbsent-ALLOC-NEXT:   acc_ev_free
//        EXE-OFF-caseConstAbsent-NOT: {{.}}
CASE(caseConstAbsent) {
  const int x;
  int y;
  int use = 0;
  #pragma acc parallel loop no_create(x)
  for (int i = 0; i < 2; ++i)
  if (use) y = x;
}

//         DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseInheritedPresent
//               DMP: ACCDataDirective
//          DMP-NEXT:   ACCCreateClause
//          DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'int'
//          DMP-NEXT:   impl: OMPTargetDataDirective
//          DMP-NEXT:     OMPMapClause
//          DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'int'
//               DMP:   ACCDataDirective
//          DMP-NEXT:     ACCNoCreateClause
//          DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'int'
//          DMP-NEXT:     impl: OMPTargetDataDirective
//          DMP-NEXT:       OMPMapClause
//          DMP-NEXT:         DeclRefExpr {{.*}} 'x' 'int'
//               DMP:     ACCParallelDirective
//          DMP-NEXT:       ACCNomapClause
//          DMP-NEXT:         DeclRefExpr {{.*}} 'x' 'int'
//          DMP-NEXT:       ACCSharedClause
//          DMP-NEXT:         DeclRefExpr {{.*}} 'x' 'int'
//          DMP-NEXT:       impl: OMPTargetTeamsDirective
// DMP-NO-ALLOC-NEXT:         OMPMapClause
// DMP-NO-ALLOC-NEXT:           DeclRefExpr {{.*}} 'x' 'int'
//          DMP-NEXT:         OMPSharedClause
//          DMP-NEXT:           DeclRefExpr {{.*}} 'x' 'int'
//    DMP-ALLOC-NEXT:         DefaultmapClause
//
//            PRT-LABEL: {{.*}}caseInheritedPresent{{.*}} {
//             PRT-NEXT:   int x;
//
//           PRT-A-NEXT:   #pragma acc data create(x){{$}}
//          PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,alloc: x){{$}}
//           PRT-A-NEXT:   #pragma acc data no_create(x){{$}}
//          PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: x){{$}}
//           PRT-A-NEXT:   #pragma acc parallel{{$}}
// PRT-AO-NO-ALLOC-NEXT:   // #pragma omp target teams map([[INHERITED_NO_CREATE_MT]]: x) shared(x){{$}}
//    PRT-AO-ALLOC-NEXT:   // #pragma omp target teams shared(x) defaultmap(tofrom: scalar){{$}}
//
//           PRT-O-NEXT:   #pragma omp target data map(ompx_hold,alloc: x){{$}}
//          PRT-OA-NEXT:   // #pragma acc data create(x){{$}}
//           PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: x){{$}}
//          PRT-OA-NEXT:   // #pragma acc data no_create(x){{$}}
//  PRT-O-NO-ALLOC-NEXT:   #pragma omp target teams map([[INHERITED_NO_CREATE_MT]]: x) shared(x){{$}}
//     PRT-O-ALLOC-NEXT:   #pragma omp target teams shared(x) defaultmap(tofrom: scalar){{$}}
//          PRT-OA-NEXT:   // #pragma acc parallel{{$}}
//
//             PRT-NEXT:   x = 1;
//             PRT-NEXT: }
//
//  EXE-OFF-caseInheritedPresent-NOT: {{.}}
//      EXE-OFF-caseInheritedPresent: acc_ev_enter_data_start
// EXE-OFF-caseInheritedPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseInheritedPresent-NEXT:   acc_ev_create
// EXE-OFF-caseInheritedPresent-NEXT:   acc_ev_enter_data_start
// EXE-OFF-caseInheritedPresent-NEXT:     acc_ev_enter_data_start
// EXE-OFF-caseInheritedPresent-NEXT:     acc_ev_exit_data_start
// EXE-OFF-caseInheritedPresent-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseInheritedPresent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseInheritedPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseInheritedPresent-NEXT:   acc_ev_free
//  EXE-OFF-caseInheritedPresent-NOT: {{.}}
CASE(caseInheritedPresent) {
  int x;
  #pragma acc data create(x)
  #pragma acc data no_create(x)
  #pragma acc parallel
  x = 1;
}

//         DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} caseInheritedAbsent
//               DMP: ACCDataDirective
//          DMP-NEXT:   ACCNoCreateClause
//          DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'int'
//          DMP-NEXT:   impl: OMPTargetDataDirective
//          DMP-NEXT:     OMPMapClause
//          DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'int'
//               DMP:   ACCParallelDirective
//          DMP-NEXT:     ACCNomapClause
//          DMP-NEXT:       DeclRefExpr {{.*}} 'use' 'int'
//          DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'int'
//          DMP-NEXT:     ACCSharedClause
//          DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'int'
//          DMP-NEXT:     ACCFirstprivateClause
//          DMP-NEXT:       DeclRefExpr {{.*}} 'use' 'int'
//          DMP-NEXT:     impl: OMPTargetTeamsDirective
// DMP-NO-ALLOC-NEXT:       OMPMapClause
// DMP-NO-ALLOC-NEXT:         DeclRefExpr {{.*}} 'x' 'int'
//          DMP-NEXT:       OMPSharedClause
//          DMP-NEXT:         DeclRefExpr {{.*}} 'x' 'int'
//          DMP-NEXT:       OMPFirstprivateClause
//          DMP-NEXT:         DeclRefExpr {{.*}} 'use' 'int'
//    DMP-ALLOC-NEXT:       DefaultmapClause
//
//            PRT-LABEL: {{.*}}caseInheritedAbsent{{.*}} {
//             PRT-NEXT:   int x;
//             PRT-NEXT:   int use = 0;
//
//           PRT-A-NEXT:   #pragma acc data no_create(x){{$}}
//          PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: x){{$}}
//           PRT-A-NEXT:   #pragma acc parallel{{$}}
// PRT-AO-NO-ALLOC-NEXT:   // #pragma omp target teams map([[INHERITED_NO_CREATE_MT]]: x) shared(x) firstprivate(use){{$}}
//    PRT-AO-ALLOC-NEXT:   // #pragma omp target teams shared(x) firstprivate(use) defaultmap(tofrom: scalar){{$}}
//
//           PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: x){{$}}
//          PRT-OA-NEXT:   // #pragma acc data no_create(x){{$}}
//  PRT-O-NO-ALLOC-NEXT:   #pragma omp target teams map([[INHERITED_NO_CREATE_MT]]: x) shared(x) firstprivate(use){{$}}
//     PRT-O-ALLOC-NEXT:   #pragma omp target teams shared(x) firstprivate(use) defaultmap(tofrom: scalar){{$}}
//          PRT-OA-NEXT:   // #pragma acc parallel{{$}}
//
//             PRT-NEXT:   if (use)
//             PRT-NEXT:     x = 1;
//             PRT-NEXT: }
//
//        EXE-OFF-caseInheritedAbsent-NOT: {{.}}
//            EXE-OFF-caseInheritedAbsent: acc_ev_enter_data_start
// EXE-OFF-caseInheritedAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseInheritedAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-OFF-caseInheritedAbsent-NEXT:   acc_ev_enter_data_start
//       EXE-OFF-caseInheritedAbsent-NEXT:   acc_ev_exit_data_start
//       EXE-OFF-caseInheritedAbsent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseInheritedAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseInheritedAbsent-ALLOC-NEXT:   acc_ev_free
//        EXE-OFF-caseInheritedAbsent-NOT: {{.}}
CASE(caseInheritedAbsent) {
  int x;
  int use = 0;
  #pragma acc data no_create(x)
  #pragma acc parallel
  if (use)
    x = 1;
}

//            PRT-LABEL: {{.*}}caseInheritedSubarrayPresent{{.*}} {
//             PRT-NEXT:   fprintf
//             PRT-NEXT:   int arr[] =
//             PRT-NEXT:   int arr2[] =
//             PRT-NEXT:   int *p =
//             PRT-NEXT:   int *p2 =
//
//           PRT-A-NEXT:   #pragma acc data copy(arr,p[0:5],p2){{$}}
//          PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: arr,p[0:5],p2){{$}}
//           PRT-A-NEXT:   #pragma acc data no_create(arr[1:2],p[2:1],p2){{$}}
//          PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: arr[1:2],p[2:1],p2){{$}}
//           PRT-A-NEXT:   #pragma acc parallel num_gangs(1){{$}}
// PRT-AO-NO-ALLOC-NEXT:   // #pragma omp target teams num_teams(1) map([[INHERITED_NO_CREATE_MT]]: arr,p[0:0],p2) shared(arr,p,p2){{$}}
//    PRT-AO-ALLOC-NEXT:   // #pragma omp target teams num_teams(1) map(alloc: p2) shared(arr,p,p2){{$}}
//
//           PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: arr,p[0:5],p2){{$}}
//          PRT-OA-NEXT:   // #pragma acc data copy(arr,p[0:5],p2){{$}}
//           PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: arr[1:2],p[2:1],p2){{$}}
//          PRT-OA-NEXT:   // #pragma acc data no_create(arr[1:2],p[2:1],p2){{$}}
//  PRT-O-NO-ALLOC-NEXT:   #pragma omp target teams num_teams(1) map([[INHERITED_NO_CREATE_MT]]: arr,p[0:0],p2) shared(arr,p,p2){{$}}
//     PRT-O-ALLOC-NEXT:   #pragma omp target teams num_teams(1) map(alloc: p2) shared(arr,p,p2){{$}}
//          PRT-OA-NEXT:   // #pragma acc parallel num_gangs(1){{$}}
//
//             PRT-NEXT:   {
//             PRT-NEXT:     arr[1] += 1;
//             PRT-NEXT:     p[2] += 2;
//             PRT-NEXT:     p2 += 1;
//             PRT-NEXT:   }
//             PRT-NEXT:   fprintf
//             PRT-NEXT:   fprintf
//             PRT-NEXT:   fprintf
//             PRT-NEXT: }
//
//      EXE-caseInheritedSubarrayPresent-NOT: {{.}}
//          EXE-caseInheritedSubarrayPresent: start
// EXE-OFF-caseInheritedSubarrayPresent-NEXT: acc_ev_enter_data_start
// EXE-OFF-caseInheritedSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseInheritedSubarrayPresent-NEXT:   acc_ev_create
// EXE-OFF-caseInheritedSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseInheritedSubarrayPresent-NEXT:   acc_ev_create
// EXE-OFF-caseInheritedSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseInheritedSubarrayPresent-NEXT:   acc_ev_create
// EXE-OFF-caseInheritedSubarrayPresent-NEXT:   acc_ev_enter_data_start
// EXE-OFF-caseInheritedSubarrayPresent-NEXT:     acc_ev_enter_data_start
// EXE-OFF-caseInheritedSubarrayPresent-NEXT:     acc_ev_exit_data_start
// EXE-OFF-caseInheritedSubarrayPresent-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseInheritedSubarrayPresent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseInheritedSubarrayPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseInheritedSubarrayPresent-NEXT:   acc_ev_free
// EXE-OFF-caseInheritedSubarrayPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseInheritedSubarrayPresent-NEXT:   acc_ev_free
// EXE-OFF-caseInheritedSubarrayPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseInheritedSubarrayPresent-NEXT:   acc_ev_free
//     EXE-caseInheritedSubarrayPresent-NEXT:   arr[1]: 21
//     EXE-caseInheritedSubarrayPresent-NEXT:   p[2]: 32
//     EXE-caseInheritedSubarrayPresent-NEXT:   p2 == arr + 1: 1
//      EXE-caseInheritedSubarrayPresent-NOT: {{.}}
//
// It's important that p2 doesn't pick up a subarray in the OpenMP translation.
// If it did, it would be incorrectly handled as firstprivate.
CASE(caseInheritedSubarrayPresent) {
  fprintf(stderr, "start\n");
  int arr[] = {10, 20, 30, 40, 50};
  int arr2[] = {10, 20, 30, 40, 50};
  int *p = arr2;
  int *p2 = arr;
  #pragma acc data copy(arr,p[0:5],p2)
  #pragma acc data no_create(arr[1:2],p[2:1],p2)
  #pragma acc parallel num_gangs(1)
  {
    arr[1] += 1;
    p[2] += 2;
    p2 += 1;
  }
  fprintf(stderr, "arr[1]: %d\n", arr[1]);
  fprintf(stderr, "p[2]: %d\n", p[2]);
  fprintf(stderr, "p2 == arr + 1: %d\n", p2 == arr + 1);
}

//            PRT-LABEL: {{.*}}caseInheritedSubarrayAbsent{{.*}} {
//             PRT-NEXT:   fprintf
//             PRT-NEXT:   int arr[] =
//             PRT-NEXT:   int arr2[] =
//             PRT-NEXT:   int *p =
//             PRT-NEXT:   int *p2 =
//             PRT-NEXT:   int use = 0;
//
//           PRT-A-NEXT:   #pragma acc data no_create(arr[1:2],p[2:1],p2){{$}}
//          PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: arr[1:2],p[2:1],p2){{$}}
//           PRT-A-NEXT:   #pragma acc parallel{{$}}
// PRT-AO-NO-ALLOC-NEXT:   // #pragma omp target teams map([[INHERITED_NO_CREATE_MT]]: arr,p[0:0],p2) shared(arr,p,p2) firstprivate(use){{$}}
//    PRT-AO-ALLOC-NEXT:   // #pragma omp target teams map(alloc: p2) shared(arr,p,p2) firstprivate(use){{$}}
//
//           PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: arr[1:2],p[2:1],p2){{$}}
//          PRT-OA-NEXT:   // #pragma acc data no_create(arr[1:2],p[2:1],p2){{$}}
//  PRT-O-NO-ALLOC-NEXT:   #pragma omp target teams map([[INHERITED_NO_CREATE_MT]]: arr,p[0:0],p2) shared(arr,p,p2) firstprivate(use){{$}}
//     PRT-O-ALLOC-NEXT:   #pragma omp target teams map(alloc: p2) shared(arr,p,p2) firstprivate(use){{$}}
//          PRT-OA-NEXT:   // #pragma acc parallel{{$}}
//
//             PRT-NEXT:   if (use) {
//             PRT-NEXT:     arr[1] = 1;
//             PRT-NEXT:     p[2] = 1;
//             PRT-NEXT:     p2 += 1;
//             PRT-NEXT:   }
//             PRT-NEXT:   fprintf
//             PRT-NEXT:   fprintf
//             PRT-NEXT:   fprintf
//             PRT-NEXT: }
//
//            EXE-caseInheritedSubarrayAbsent-NOT: {{.}}
//                EXE-caseInheritedSubarrayAbsent: start
//       EXE-OFF-caseInheritedSubarrayAbsent-NEXT: acc_ev_enter_data_start
// EXE-OFF-caseInheritedSubarrayAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseInheritedSubarrayAbsent-ALLOC-NEXT:   acc_ev_create
// EXE-OFF-caseInheritedSubarrayAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseInheritedSubarrayAbsent-ALLOC-NEXT:   acc_ev_create
// EXE-OFF-caseInheritedSubarrayAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseInheritedSubarrayAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-OFF-caseInheritedSubarrayAbsent-NEXT:   acc_ev_enter_data_start
//       EXE-OFF-caseInheritedSubarrayAbsent-NEXT:   acc_ev_exit_data_start
//       EXE-OFF-caseInheritedSubarrayAbsent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseInheritedSubarrayAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseInheritedSubarrayAbsent-ALLOC-NEXT:   acc_ev_free
// EXE-OFF-caseInheritedSubarrayAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseInheritedSubarrayAbsent-ALLOC-NEXT:   acc_ev_free
// EXE-OFF-caseInheritedSubarrayAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseInheritedSubarrayAbsent-ALLOC-NEXT:   acc_ev_free
//           EXE-caseInheritedSubarrayAbsent-NEXT:   arr[1]: 20
//           EXE-caseInheritedSubarrayAbsent-NEXT:   p[2]: 30
//           EXE-caseInheritedSubarrayAbsent-NEXT:   p2 == arr: 1
//            EXE-caseInheritedSubarrayAbsent-NOT: {{.}}
CASE(caseInheritedSubarrayAbsent) {
  fprintf(stderr, "start\n");
  int arr[] = {10, 20, 30, 40, 50};
  int arr2[] = {10, 20, 30, 40, 50};
  int *p = arr2;
  int *p2 = arr;
  int use = 0;
  #pragma acc data no_create(arr[1:2],p[2:1],p2)
  #pragma acc parallel
  if (use) {
    arr[1] = 1;
    p[2] = 1;
    p2 += 1;
  }
  fprintf(stderr, "arr[1]: %d\n", arr[1]);
  fprintf(stderr, "p[2]: %d\n", p[2]);
  fprintf(stderr, "p2 == arr: %d\n", p2 == arr);
}

// EXE-HOST-NOT: {{.}}
