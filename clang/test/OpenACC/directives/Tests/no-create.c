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
// update.c, and subarray-errors.c (the last is located in
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
// DEFINE:   %{check-case}( caseDataArrayPresent             %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseDataArrayAbsent              %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseDataSubarrayPresent          %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseDataSubarrayDisjoint         %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseDataSubarrayOverlapStart     %, %{NO_ALLOC_OR_ALLOC} %, %{NOT_CRASH_IF_OFF_AND_ALLOC} %) && \
// DEFINE:   %{check-case}( caseDataSubarrayOverlapEnd       %, %{NO_ALLOC_OR_ALLOC} %, %{NOT_CRASH_IF_OFF_AND_ALLOC} %) && \
// DEFINE:   %{check-case}( caseDataSubarrayConcat2          %, %{NO_ALLOC_OR_ALLOC} %, %{NOT_CRASH_IF_OFF_AND_ALLOC} %) && \
// DEFINE:   %{check-case}( caseDataSubarrayNonSubarray      %, %{NO_ALLOC_OR_ALLOC} %, %{NOT_CRASH_IF_OFF_AND_ALLOC} %) && \
// DEFINE:   %{check-case}( caseParallelScalarPresent        %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseParallelScalarAbsent         %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseParallelArrayPresent         %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
// DEFINE:   %{check-case}( caseParallelArrayAbsent          %, %{NO_ALLOC_OR_ALLOC} %,                               %) && \
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
// REDEFINE:   : '----------------- CASE: %{CASE} -----------------'                  && \
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

//   PRT-LABEL: {{.*}}caseDataSubarrayPresent{{.*}} {
//    PRT-NEXT:   int all[10], same[10], beg[10], mid[10], end[10];
//
//  PRT-A-NEXT:   #pragma acc data copy(all,same[3:6],beg[2:5],mid[1:8],end[0:5])
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: all,same[3:6],beg[2:5],mid[1:8],end[0:5])
//  PRT-A-NEXT:   #pragma acc data no_create(all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
// PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: all,same[3:6],beg[2:5],mid[1:8],end[0:5])
// PRT-OA-NEXT:   // #pragma acc data copy(all,same[3:6],beg[2:5],mid[1:8],end[0:5])
//  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
// PRT-OA-NEXT:   // #pragma acc data no_create(all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
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
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseDataSubarrayPresent-NEXT:   acc_ev_free
//  EXE-OFF-caseDataSubarrayPresent-NOT: {{.}}
CASE(caseDataSubarrayPresent) {
  int all[10], same[10], beg[10], mid[10], end[10];
  #pragma acc data copy(all,same[3:6],beg[2:5],mid[1:8],end[0:5])
  #pragma acc data no_create(all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
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
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseParallelSubarrayPresent-NEXT:   acc_ev_free
//  EXE-OFF-caseParallelSubarrayPresent-NOT: {{.}}
CASE(caseParallelSubarrayPresent) {
  int all[10], same[10], beg[10], mid[10], end[10];
  #pragma acc data copy(all,same[3:6],beg[2:5],mid[1:8],end[0:5])
  #pragma acc parallel no_create(all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
  {
    all[0] = 1; same[3] = 1; beg[2] = 1; mid[3] = 1; end[4] = 1;
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
//             PRT-NEXT:   int arr[] =
//
//           PRT-A-NEXT:   #pragma acc data create(arr){{$}}
//          PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,alloc: arr){{$}}
//           PRT-A-NEXT:   #pragma acc data no_create(arr[1:2]){{$}}
//          PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: arr[1:2]){{$}}
//           PRT-A-NEXT:   #pragma acc parallel{{$}}
// PRT-AO-NO-ALLOC-NEXT:   // #pragma omp target teams map([[INHERITED_NO_CREATE_MT]]: arr) shared(arr){{$}}
//    PRT-AO-ALLOC-NEXT:   // #pragma omp target teams shared(arr){{$}}
//
//           PRT-O-NEXT:   #pragma omp target data map(ompx_hold,alloc: arr){{$}}
//          PRT-OA-NEXT:   // #pragma acc data create(arr){{$}}
//           PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: arr[1:2]){{$}}
//          PRT-OA-NEXT:   // #pragma acc data no_create(arr[1:2]){{$}}
//  PRT-O-NO-ALLOC-NEXT:   #pragma omp target teams map([[INHERITED_NO_CREATE_MT]]: arr) shared(arr){{$}}
//     PRT-O-ALLOC-NEXT:   #pragma omp target teams shared(arr){{$}}
//          PRT-OA-NEXT:   // #pragma acc parallel{{$}}
//
//             PRT-NEXT:   arr[1] = 1;
//             PRT-NEXT: }
//
//  EXE-OFF-caseInheritedSubarrayPresent-NOT: {{.}}
//      EXE-OFF-caseInheritedSubarrayPresent: acc_ev_enter_data_start
// EXE-OFF-caseInheritedSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-OFF-caseInheritedSubarrayPresent-NEXT:   acc_ev_create
// EXE-OFF-caseInheritedSubarrayPresent-NEXT:   acc_ev_enter_data_start
// EXE-OFF-caseInheritedSubarrayPresent-NEXT:     acc_ev_enter_data_start
// EXE-OFF-caseInheritedSubarrayPresent-NEXT:     acc_ev_exit_data_start
// EXE-OFF-caseInheritedSubarrayPresent-NEXT:   acc_ev_exit_data_start
// EXE-OFF-caseInheritedSubarrayPresent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseInheritedSubarrayPresent-NEXT:   acc_ev_delete
// EXE-OFF-caseInheritedSubarrayPresent-NEXT:   acc_ev_free
//  EXE-OFF-caseInheritedSubarrayPresent-NOT: {{.}}
CASE(caseInheritedSubarrayPresent) {
  int arr[] = {10, 20, 30, 40, 50};
  #pragma acc data create(arr)
  #pragma acc data no_create(arr[1:2])
  #pragma acc parallel
  arr[1] = 1;
}

//            PRT-LABEL: {{.*}}caseInheritedSubarrayAbsent{{.*}} {
//             PRT-NEXT:   int arr[] =
//             PRT-NEXT:   int use = 0;
//
//           PRT-A-NEXT:   #pragma acc data no_create(arr[1:2]){{$}}
//          PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: arr[1:2]){{$}}
//           PRT-A-NEXT:   #pragma acc parallel{{$}}
// PRT-AO-NO-ALLOC-NEXT:   // #pragma omp target teams map([[INHERITED_NO_CREATE_MT]]: arr) shared(arr) firstprivate(use){{$}}
//    PRT-AO-ALLOC-NEXT:   // #pragma omp target teams shared(arr) firstprivate(use){{$}}
//
//           PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: arr[1:2]){{$}}
//          PRT-OA-NEXT:   // #pragma acc data no_create(arr[1:2]){{$}}
//  PRT-O-NO-ALLOC-NEXT:   #pragma omp target teams map([[INHERITED_NO_CREATE_MT]]: arr) shared(arr) firstprivate(use){{$}}
//     PRT-O-ALLOC-NEXT:   #pragma omp target teams shared(arr) firstprivate(use){{$}}
//          PRT-OA-NEXT:   // #pragma acc parallel{{$}}
//
//             PRT-NEXT:   if (use)
//             PRT-NEXT:     arr[1] = 1;
//             PRT-NEXT: }
//
//        EXE-OFF-caseInheritedSubarrayAbsent-NOT: {{.}}
//            EXE-OFF-caseInheritedSubarrayAbsent: acc_ev_enter_data_start
// EXE-OFF-caseInheritedSubarrayAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-OFF-caseInheritedSubarrayAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-OFF-caseInheritedSubarrayAbsent-NEXT:   acc_ev_enter_data_start
//       EXE-OFF-caseInheritedSubarrayAbsent-NEXT:   acc_ev_exit_data_start
//       EXE-OFF-caseInheritedSubarrayAbsent-NEXT: acc_ev_exit_data_start
// EXE-OFF-caseInheritedSubarrayAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-OFF-caseInheritedSubarrayAbsent-ALLOC-NEXT:   acc_ev_free
//        EXE-OFF-caseInheritedSubarrayAbsent-NOT: {{.}}
CASE(caseInheritedSubarrayAbsent) {
  int arr[] = {10, 20, 30, 40, 50};
  int use = 0;
  #pragma acc data no_create(arr[1:2])
  #pragma acc parallel
  if (use)
    arr[1] = 1;
}

// EXE-HOST-NOT: {{.}}
