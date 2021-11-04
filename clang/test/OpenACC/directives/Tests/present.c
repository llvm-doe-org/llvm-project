// Check present clauses on different constructs and with different values of
// -fopenacc-present-omp.
//
// Diagnostics about the present map type modifier in the translation are
// checked in diagnostics/warn-acc-omp-map-present.c.  Diagnostics about bad
// -fopenacc-present-omp arguments are checked in
// diagnostics/fopenacc-present-omp.c.
//
// The various cases covered here should be kept consistent with no-create.c,
// update.c, and subarray-errors.c (the last is located in
// openmp/libacc2omp/test/directives).  For example, a subarray that extends a
// subarray already present is consistently considered not present, so the
// present clause produces a runtime error and the no_create clause doesn't
// allocate.

// RUN: %data present-opts {
// RUN:   (present-opt=                                 present-mt=present,ompx_hold,alloc not-if-present=not not-crash-if-present='not --crash')
// RUN:   (present-opt=-fopenacc-present-omp=present    present-mt=present,ompx_hold,alloc not-if-present=not not-crash-if-present='not --crash')
// RUN:   (present-opt=-fopenacc-present-omp=no-present present-mt=ompx_hold,alloc         not-if-present=    not-crash-if-present=             )
// RUN: }
// RUN: %data use-vars {
// RUN:   (use-var-cflags=            )
// RUN:   (use-var-cflags=-DDO_USE_VAR)
// RUN: }

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
// RUN: %data cases {
// RUN:   (case=caseDataScalarPresent            not-if-fail=                                   not-crash-if-fail=                                         not-if-presentError=                                   not-if-arrayExtError=                     construct=data    )
// RUN:   (case=caseDataScalarAbsent             not-if-fail='%if-tgt-host<|%[not-if-present]>' not-crash-if-fail='%if-tgt-host<|%[not-crash-if-present]>' not-if-presentError='%if-tgt-host<|%[not-if-present]>' not-if-arrayExtError=                     construct=data    )
// RUN:   (case=caseDataArrayPresent             not-if-fail=                                   not-crash-if-fail=                                         not-if-presentError=                                   not-if-arrayExtError=                     construct=data    )
// RUN:   (case=caseDataArrayAbsent              not-if-fail='%if-tgt-host<|%[not-if-present]>' not-crash-if-fail='%if-tgt-host<|%[not-crash-if-present]>' not-if-presentError='%if-tgt-host<|%[not-if-present]>' not-if-arrayExtError=                     construct=data    )
// RUN:   (case=caseDataSubarrayPresent          not-if-fail=                                   not-crash-if-fail=                                         not-if-presentError=                                   not-if-arrayExtError=                     construct=data    )
// RUN:   (case=caseDataSubarrayDisjoint         not-if-fail='%if-tgt-host<|%[not-if-present]>' not-crash-if-fail='%if-tgt-host<|%[not-crash-if-present]>' not-if-presentError='%if-tgt-host<|%[not-if-present]>' not-if-arrayExtError=                     construct=data    )
// RUN:   (case=caseDataSubarrayOverlapStart     not-if-fail='%if-tgt-host<|not>'               not-crash-if-fail='%if-tgt-host<|not --crash>'             not-if-presentError='%if-tgt-host<|%[not-if-present]>' not-if-arrayExtError='%if-tgt-host<|not>' construct=data    )
// RUN:   (case=caseDataSubarrayOverlapEnd       not-if-fail='%if-tgt-host<|not>'               not-crash-if-fail='%if-tgt-host<|not --crash>'             not-if-presentError='%if-tgt-host<|%[not-if-present]>' not-if-arrayExtError='%if-tgt-host<|not>' construct=data    )
// RUN:   (case=caseDataSubarrayConcat2          not-if-fail='%if-tgt-host<|not>'               not-crash-if-fail='%if-tgt-host<|not --crash>'             not-if-presentError='%if-tgt-host<|%[not-if-present]>' not-if-arrayExtError='%if-tgt-host<|not>' construct=data    )
// RUN:   (case=caseDataSubarrayNonSubarray      not-if-fail='%if-tgt-host<|not>'               not-crash-if-fail='%if-tgt-host<|not --crash>'             not-if-presentError='%if-tgt-host<|%[not-if-present]>' not-if-arrayExtError='%if-tgt-host<|not>' construct=data    )
// RUN:   (case=caseParallelScalarPresent        not-if-fail=                                   not-crash-if-fail=                                         not-if-presentError=                                   not-if-arrayExtError=                     construct=parallel)
// RUN:   (case=caseParallelScalarAbsent         not-if-fail='%if-tgt-host<|%[not-if-present]>' not-crash-if-fail='%if-tgt-host<|%[not-crash-if-present]>' not-if-presentError='%if-tgt-host<|%[not-if-present]>' not-if-arrayExtError=                     construct=parallel)
// RUN:   (case=caseParallelArrayPresent         not-if-fail=                                   not-crash-if-fail=                                         not-if-presentError=                                   not-if-arrayExtError=                     construct=parallel)
// RUN:   (case=caseParallelArrayAbsent          not-if-fail='%if-tgt-host<|%[not-if-present]>' not-crash-if-fail='%if-tgt-host<|%[not-crash-if-present]>' not-if-presentError='%if-tgt-host<|%[not-if-present]>' not-if-arrayExtError=                     construct=parallel)
// RUN:   (case=caseParallelSubarrayPresent      not-if-fail=                                   not-crash-if-fail=                                         not-if-presentError=                                   not-if-arrayExtError=                     construct=parallel)
// RUN:   (case=caseParallelSubarrayDisjoint     not-if-fail='%if-tgt-host<|%[not-if-present]>' not-crash-if-fail='%if-tgt-host<|%[not-crash-if-present]>' not-if-presentError='%if-tgt-host<|%[not-if-present]>' not-if-arrayExtError=                     construct=parallel)
// RUN:   (case=caseParallelSubarrayOverlapStart not-if-fail='%if-tgt-host<|not>'               not-crash-if-fail='%if-tgt-host<|not --crash>'             not-if-presentError='%if-tgt-host<|%[not-if-present]>' not-if-arrayExtError='%if-tgt-host<|not>' construct=parallel)
// RUN:   (case=caseParallelSubarrayOverlapEnd   not-if-fail='%if-tgt-host<|not>'               not-crash-if-fail='%if-tgt-host<|not --crash>'             not-if-presentError='%if-tgt-host<|%[not-if-present]>' not-if-arrayExtError='%if-tgt-host<|not>' construct=parallel)
// RUN:   (case=caseParallelSubarrayConcat2      not-if-fail='%if-tgt-host<|not>'               not-crash-if-fail='%if-tgt-host<|not --crash>'             not-if-presentError='%if-tgt-host<|%[not-if-present]>' not-if-arrayExtError='%if-tgt-host<|not>' construct=parallel)
// RUN:   (case=caseParallelSubarrayNonSubarray  not-if-fail='%if-tgt-host<|not>'               not-crash-if-fail='%if-tgt-host<|not --crash>'             not-if-presentError='%if-tgt-host<|%[not-if-present]>' not-if-arrayExtError='%if-tgt-host<|not>' construct=parallel)
// RUN:   (case=caseParallelLoopScalarPresent    not-if-fail=                                   not-crash-if-fail=                                         not-if-presentError=                                   not-if-arrayExtError=                     construct=parallel)
// RUN:   (case=caseParallelLoopScalarAbsent     not-if-fail='%if-tgt-host<|%[not-if-present]>' not-crash-if-fail='%if-tgt-host<|%[not-crash-if-present]>' not-if-presentError='%if-tgt-host<|%[not-if-present]>' not-if-arrayExtError=                     construct=parallel)
// RUN:   (case=caseConstPresent                 not-if-fail=                                   not-crash-if-fail=                                         not-if-presentError=                                   not-if-arrayExtError=                     construct=parallel)
// RUN:   (case=caseConstAbsent                  not-if-fail='%if-tgt-host<|%[not-if-present]>' not-crash-if-fail='%if-tgt-host<|%[not-crash-if-present]>' not-if-presentError='%if-tgt-host<|%[not-if-present]>' not-if-arrayExtError=                     construct=parallel)
// RUN: }
// RUN: echo '#define FOREACH_CASE(Macro) \' > %t-cases.h
// RUN: %for cases {
// RUN:   echo '  Macro(%[case]) \' >> %t-cases.h
// RUN: }
// RUN: echo '  /*end of FOREACH_CASE*/' >> %t-cases.h

// RUN: %for present-opts {
// RUN:   %acc-check-dmp{                                                      \
// RUN:     clang-args: %[present-opt] -DCASES_HEADER='"%t-cases.h"'}
// RUN:   %acc-check-prt{                                                      \
// RUN:     clang-args: %[present-opt] -DCASES_HEADER='"%t-cases.h"';          \
// RUN:     fc-args:    -DPRESENT_MT=%[present-mt]}
// RUN:   %for use-vars {
// RUN:     %acc-check-exe-compile{                                            \
// RUN:       clang-args: %[present-opt] -DCASES_HEADER='"%t-cases.h"'         \
// RUN:                   %[use-var-cflags] -gline-tables-only}
// RUN:     %for cases {
// RUN:       %acc-check-exe-run{                                              \
// RUN:         exe-args:  %[case];                                            \
// RUN:         cmd-start: %[not-crash-if-fail]}
// RUN:       %acc-check-exe-filecheck{                                        \
// RUN:         fc-args: ;                                                     \
// RUN:         fc-pres: %[not-if-fail]PASS,%[not-if-fail]PASS-%[construct],%[not-if-presentError]PRESENT,%[not-if-arrayExtError]ARRAYEXT,%[not-if-presentError]PRESENT-%[not-if-arrayExtError]ARRAYEXT}
// RUN:     }
// RUN:   }
// RUN: }

// END.

/* expected-error 0 {{}} */

// FIXME: Clang produces spurious warning diagnostics for nvptx64 offload.  This
// issue is not limited to Clacc and is present upstream:
/* nvptx64-warning@*:* 0+ {{Linking two modules of different target triples}} */

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
//             EXE-notPASS-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
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

//   PRT-LABEL: {{.*}}caseDataSubarrayPresent{{.*}} {
//    PRT-NEXT:   int all[10], same[10], beg[10], mid[10], end[10];
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//    PRT-NEXT:   {{(PRINT_VAR_INFO|fprintf)\(.*\);}}
//
//  PRT-A-NEXT:   #pragma acc data copy(all,same[3:6],beg[2:5],mid[1:8],end[0:5])
// PRT-AO-NEXT:   // #pragma omp target data map(ompx_hold,tofrom: all,same[3:6],beg[2:5],mid[1:8],end[0:5])
//  PRT-A-NEXT:   #pragma acc data present(all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
// PRT-AO-NEXT:   // #pragma omp target data map([[PRESENT_MT]]: all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
//
//  PRT-O-NEXT:   #pragma omp target data map(ompx_hold,tofrom: all,same[3:6],beg[2:5],mid[1:8],end[0:5])
// PRT-OA-NEXT:   // #pragma acc data copy(all,same[3:6],beg[2:5],mid[1:8],end[0:5])
//  PRT-O-NEXT:   #pragma omp target data map([[PRESENT_MT]]: all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
// PRT-OA-NEXT:   // #pragma acc data present(all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
//
//    PRT-NEXT:   {
//    PRT-NEXT:     ;
//    PRT-NEXT:   }
//    PRT-NEXT: }
CASE(caseDataSubarrayPresent) {
  int all[10], same[10], beg[10], mid[10], end[10];
  PRINT_VAR_INFO(all);
  PRINT_VAR_INFO(all);
  #pragma acc data copy(all,same[3:6],beg[2:5],mid[1:8],end[0:5])
  #pragma acc data present(all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
  {
    USE_VAR(all[0] = 1; same[3] = 1; beg[2] = 1; mid[3] = 1; end[4] = 1);
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

CASE(caseParallelSubarrayPresent) {
  int all[10], same[10], beg[10], mid[10], end[10];
  PRINT_VAR_INFO(all);
  PRINT_VAR_INFO(all);
  #pragma acc data copy(all,same[3:6],beg[2:5],mid[1:8],end[0:5])
  #pragma acc parallel present(all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
  {
    USE_VAR(all[0] = 1; same[3] = 1; beg[2] = 1; mid[3] = 1; end[4] = 1);
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
