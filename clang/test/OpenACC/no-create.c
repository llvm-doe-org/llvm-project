// Check no_create clauses on different constructs and with different values of
// -fopenacc-no-create-omp.  Diagnostics about no_alloc in the translation are
// tested in warn-acc-omp-map-no-alloc.c.  data.c tests various interactions
// with explicit DAs and the defaultmap added for scalars with suppressed
// OpenACC implicit DAs.
//
// The various cases covered here should be kept consistent with present.c,
// update.c, and subarray-errors.c.  For example, a subarray that extends a
// subarray already present is consistently considered not present, so the
// present clause produces a runtime error and the no_create clause doesn't
// allocate.  However, INHERITED cases have no meaning for the present clause.
//
// In some cases, it's challenging to check when no_create actually doesn't
// allocate memory.  Specifically, calling acc_is_present or
// omp_target_is_present or specifying a present clause on a directive only
// works from the host, so it doesn't help for checking no_create on
// "acc parallel".  We could examine the output of LIBOMPTARGET_DEBUG=1, but it
// only works in debug builds.  Our solution is to utilize the OpenACC Profiling
// Interface, which we usually only exercise in the runtime test suite.

// Check bad -fopenacc-no-create-omp values.
//
// RUN: %data bad-vals {
// RUN:   (val=foo)
// RUN:   (val=   )
// RUN: }
// RUN: %data bad-vals-cmds {
// RUN:   (cmd='%clang -fopenacc'    )
// RUN:   (cmd='%clang_cc1 -fopenacc')
// RUN:   (cmd='%clang'              )
// RUN:   (cmd='%clang_cc1'          )
// RUN: }
// RUN: %for bad-vals {
// RUN:   %for bad-vals-cmds {
// RUN:     not %[cmd] -fopenacc-no-create-omp=%[val] %s 2>&1 \
// RUN:     | FileCheck -check-prefix=BAD-VAL -DVAL=%[val] %s
// RUN:   }
// RUN: }
//
// BAD-VAL: error: invalid value '[[VAL]]' in '-fopenacc-no-create-omp=[[VAL]]'

// Define some interrelated data we use several times below.
//
// RUN: %data no-create-opts {
// RUN:   (no-create-opt=-Wno-openacc-omp-map-no-alloc                                    no-create-mt=no_alloc,hold,alloc inherited-no-create-mt=no_alloc,alloc noAlloc-or-alloc=NO-ALLOC not-crash-if-alloc=             )
// RUN:   (no-create-opt='-fopenacc-no-create-omp=no_alloc -Wno-openacc-omp-map-no-alloc' no-create-mt=no_alloc,hold,alloc inherited-no-create-mt=no_alloc,alloc noAlloc-or-alloc=NO-ALLOC not-crash-if-alloc=             )
// RUN:   (no-create-opt=-fopenacc-no-create-omp=no-no_alloc                              no-create-mt=hold,alloc          inherited-no-create-mt=alloc          noAlloc-or-alloc=ALLOC    not-crash-if-alloc='not --crash')
// RUN: }
// RUN: %data tgts {
// RUN:   (run-if=                tgt-cflags=                                     host=-HOST not-crash-if-off-and-alloc=                     )
// RUN:   (run-if=%run-if-x86_64  tgt-cflags=-fopenmp-targets=%run-x86_64-triple  host=      not-crash-if-off-and-alloc=%[not-crash-if-alloc])
// RUN:   (run-if=%run-if-ppc64le tgt-cflags=-fopenmp-targets=%run-ppc64le-triple host=      not-crash-if-off-and-alloc=%[not-crash-if-alloc])
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags=-fopenmp-targets=%run-nvptx64-triple host=      not-crash-if-off-and-alloc=%[not-crash-if-alloc])
// RUN: }
//      # "acc parallel loop" should be about the same as "acc parallel", so a
//      # few cases are probably sufficient for it.
// RUN: %data cases {
// RUN:   (case=caseDataScalarPresent            not-crash-if-fail=                             )
// RUN:   (case=caseDataScalarAbsent             not-crash-if-fail=                             )
// RUN:   (case=caseDataArrayPresent             not-crash-if-fail=                             )
// RUN:   (case=caseDataArrayAbsent              not-crash-if-fail=                             )
// RUN:   (case=caseDataSubarrayPresent          not-crash-if-fail=                             )
// RUN:   (case=caseDataSubarrayDisjoint         not-crash-if-fail=                             )
// RUN:   (case=caseDataSubarrayOverlapStart     not-crash-if-fail=%[not-crash-if-off-and-alloc])
// RUN:   (case=caseDataSubarrayOverlapEnd       not-crash-if-fail=%[not-crash-if-off-and-alloc])
// RUN:   (case=caseDataSubarrayConcat2          not-crash-if-fail=%[not-crash-if-off-and-alloc])
// RUN:   (case=caseDataSubarrayNonSubarray      not-crash-if-fail=%[not-crash-if-off-and-alloc])
// RUN:   (case=caseParallelScalarPresent        not-crash-if-fail=                             )
// RUN:   (case=caseParallelScalarAbsent         not-crash-if-fail=                             )
// RUN:   (case=caseParallelArrayPresent         not-crash-if-fail=                             )
// RUN:   (case=caseParallelArrayAbsent          not-crash-if-fail=                             )
// RUN:   (case=caseParallelSubarrayPresent      not-crash-if-fail=                             )
// RUN:   (case=caseParallelSubarrayDisjoint     not-crash-if-fail=                             )
// RUN:   (case=caseParallelSubarrayOverlapStart not-crash-if-fail=%[not-crash-if-off-and-alloc])
// RUN:   (case=caseParallelSubarrayOverlapEnd   not-crash-if-fail=%[not-crash-if-off-and-alloc])
// RUN:   (case=caseParallelSubarrayConcat2      not-crash-if-fail=%[not-crash-if-off-and-alloc])
// RUN:   (case=caseParallelSubarrayNonSubarray  not-crash-if-fail=%[not-crash-if-off-and-alloc])
// RUN:   (case=caseParallelLoopScalarPresent    not-crash-if-fail=                             )
// RUN:   (case=caseParallelLoopScalarAbsent     not-crash-if-fail=                             )
// RUN:   (case=caseConstPresent                 not-crash-if-fail=                             )
// RUN:   (case=caseConstAbsent                  not-crash-if-fail=                             )
// RUN:   (case=caseInheritedPresent             not-crash-if-fail=                             )
// RUN:   (case=caseInheritedAbsent              not-crash-if-fail=                             )
// RUN:   (case=caseInheritedSubarrayPresent     not-crash-if-fail=                             )
// RUN:   (case=caseInheritedSubarrayAbsent      not-crash-if-fail=                             )
// RUN: }
// RUN: echo '#define FOREACH_CASE(Macro) \' > %t-cases.h
// RUN: %for cases {
// RUN:   echo '  Macro(%[case]) \' >> %t-cases.h
// RUN: }
// RUN: echo '  /*end of FOREACH_CASE*/' >> %t-cases.h

// Check -ast-dump before and after AST serialization.
//
// We include dump checking on only a few representative cases, which should be
// more than sufficient to show it's working for the no_create clause.
//
// RUN: %for no-create-opts {
// RUN:   %clang -Xclang -verify -Xclang -ast-dump -fsyntax-only -fopenacc %s \
// RUN:          %acc-includes %[no-create-opt] -DCASES_HEADER='"%t-cases.h"' \
// RUN:   | FileCheck -check-prefixes=DMP,DMP-%[noAlloc-or-alloc] %s
// RUN:   %clang -Xclang -verify -fopenacc -emit-ast -o %t.ast %s \
// RUN:          %acc-includes %[no-create-opt] -DCASES_HEADER='"%t-cases.h"'
// RUN:   %clang_cc1 -ast-dump-all %t.ast \
// RUN:   | FileCheck -check-prefixes=DMP,DMP-%[noAlloc-or-alloc] %s
// RUN: }

// Check -ast-print and -fopenacc[-ast]-print.
//
// We include print checking on only a few representative cases, which should be
// more than sufficient to show it's working for the no_create clause.
//
// RUN: %clang -Xclang -verify -Xclang -ast-print -fsyntax-only %acc-includes \
// RUN:        -DCASES_HEADER='"%t-cases.h"' %s \
// RUN: | FileCheck -check-prefixes=PRT %s
//
// TODO: If lit were to support %for inside a %data, we could iterate prt-opts
// within prt-args after the first prt-args iteration, significantly shortening
// the prt-args definition.
//
// Strip comments and blank lines so checking -fopenacc-print output is easier.
// RUN: echo "// expected""-no-diagnostics" > %t-acc.c
// RUN: grep -v '^ *\(//.*\)\?$' %s | sed 's,//.*,,' >> %t-acc.c
//
// RUN: %data prt-opts {
// RUN:   (prt-opt=-fopenacc-ast-print)
// RUN:   (prt-opt=-fopenacc-print    )
// RUN: }
// RUN: %data prt-args {
// RUN:   (prt='-Xclang -ast-print -fsyntax-only -fopenacc' prt-chk=PRT,PRT-%[noAlloc-or-alloc],PRT-A,PRT-A-%[noAlloc-or-alloc]                                      )
// RUN:   (prt=-fopenacc-ast-print=acc                      prt-chk=PRT,PRT-%[noAlloc-or-alloc],PRT-A,PRT-A-%[noAlloc-or-alloc]                                      )
// RUN:   (prt=-fopenacc-ast-print=omp                      prt-chk=PRT,PRT-%[noAlloc-or-alloc],PRT-O,PRT-O-%[noAlloc-or-alloc]                                      )
// RUN:   (prt=-fopenacc-ast-print=acc-omp                  prt-chk=PRT,PRT-%[noAlloc-or-alloc],PRT-A,PRT-A-%[noAlloc-or-alloc],PRT-AO,PRT-AO-%[noAlloc-or-alloc])
// RUN:   (prt=-fopenacc-ast-print=omp-acc                  prt-chk=PRT,PRT-%[noAlloc-or-alloc],PRT-O,PRT-O-%[noAlloc-or-alloc],PRT-OA,PRT-OA-%[noAlloc-or-alloc])
// RUN:   (prt=-fopenacc-print=acc                          prt-chk=PRT,PRT-%[noAlloc-or-alloc],PRT-A,PRT-A-%[noAlloc-or-alloc]                                      )
// RUN:   (prt=-fopenacc-print=omp                          prt-chk=PRT,PRT-%[noAlloc-or-alloc],PRT-O,PRT-O-%[noAlloc-or-alloc]                                      )
// RUN:   (prt=-fopenacc-print=acc-omp                      prt-chk=PRT,PRT-%[noAlloc-or-alloc],PRT-A,PRT-A-%[noAlloc-or-alloc],PRT-AO,PRT-AO-%[noAlloc-or-alloc])
// RUN:   (prt=-fopenacc-print=omp-acc                      prt-chk=PRT,PRT-%[noAlloc-or-alloc],PRT-O,PRT-O-%[noAlloc-or-alloc],PRT-OA,PRT-OA-%[noAlloc-or-alloc])
// RUN: }
// RUN: %for no-create-opts {
// RUN:   %for prt-args {
// RUN:     %clang -Xclang -verify %[prt] %[no-create-opt] %acc-includes \
// RUN:            -DCASES_HEADER='"%t-cases.h"' %t-acc.c \
// RUN:            -Wno-openacc-omp-map-hold \
// RUN:     | FileCheck -check-prefixes=%[prt-chk] \
// RUN:                 -DNO_CREATE_MT=%[no-create-mt] \
// RUN:                 -DINHERITED_NO_CREATE_MT=%[inherited-no-create-mt] %s
// RUN:   }
// RUN: }

// Check -ast-print after AST serialization.
//
// Some data related to printing (where to print comments about discarded
// directives) is serialized and deserialized, so it's worthwhile to try all
// OpenACC printing modes.
//
// RUN: %for no-create-opts {
// RUN:   %clang -Xclang -verify -fopenacc %[no-create-opt] -emit-ast \
// RUN:          %acc-includes -DCASES_HEADER='"%t-cases.h"' -o %t.ast %t-acc.c
// RUN:   %for prt-args {
// RUN:     %clang %[prt] %t.ast 2>&1 \
// RUN:     | FileCheck -check-prefixes=%[prt-chk] \
// RUN:                 -DNO_CREATE_MT=%[no-create-mt] \
// RUN:                 -DINHERITED_NO_CREATE_MT=%[inherited-no-create-mt] %s
// RUN:   }
// RUN: }

// Can we print the OpenMP source code, compile, and run it successfully?
//
// We don't normally bother to check this for offloading, but the no_create
// clause has no effect when not offloading (that is, for shared memory), and
// one of the main issues with the no_create clause is the various ways it can
// be translated so it can be used in source-to-source when targeting other
// compilers.  That is, we want to be sure source-to-source mode produces
// working translations of the no_create clause in all cases.
//
// RUN: %for no-create-opts {
// RUN:   %for tgts {
// RUN:     %for prt-opts {
// RUN:       %[run-if] %clang -Xclang -verify %[prt-opt]=omp \
// RUN:                 %[no-create-opt] %acc-includes \
// RUN:                 -DCASES_HEADER='"%t-cases.h"' %s > %t-omp.c \
// RUN:                 -Wno-openacc-omp-map-hold
// RUN:       %[run-if] echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN:       %[run-if] %clang -Xclang -verify -fopenmp %fopenmp-version \
// RUN:                 %[tgt-cflags] -Wno-unused-function %acc-includes \
// RUN:                 -DCASES_HEADER='"%t-cases.h"' -gline-tables-only \
// RUN:                 -o %t.exe %t-omp.c %acc-libs
// RUN:       %for cases {
// RUN:         %[run-if] %[not-crash-if-fail] %t.exe %[case] > %t.out 2>&1
// RUN:         %[run-if] FileCheck -input-file %t.out %s \
// RUN:           -match-full-lines -allow-empty \
// RUN:           -check-prefixes=EXE%[host],EXE-%[case]%[host] \
// RUN:           -check-prefixes=EXE-%[case]-%[noAlloc-or-alloc]%[host]
// RUN:       }
// RUN:     }
// RUN:   }
// RUN: }

// Check execution with normal compilation.
//
// RUN: %for no-create-opts {
// RUN:   %for tgts {
// RUN:     %[run-if] %clang -Xclang -verify -fopenacc %[no-create-opt] \
// RUN:               %[tgt-cflags] %acc-includes \
// RUN:               -DCASES_HEADER='"%t-cases.h"' -o %t.exe %s
// RUN:     %for cases {
// RUN:       %[run-if] %[not-crash-if-fail] %t.exe %[case] > %t.out 2>&1
// RUN:       %[run-if] FileCheck -input-file %t.out %s \
// RUN:         -match-full-lines -allow-empty \
// RUN:         -check-prefixes=EXE%[host],EXE-%[case]%[host] \
// RUN:         -check-prefixes=EXE-%[case]-%[noAlloc-or-alloc]%[host]
// RUN:     }
// RUN:   }
// RUN: }

// END.

// expected-no-diagnostics

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
// PRT-AO-NEXT:   // #pragma omp target data map(hold,tofrom: x){{$}}
//  PRT-A-NEXT:   #pragma acc data no_create(x){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: x){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(hold,tofrom: x){{$}}
// PRT-OA-NEXT:   // #pragma acc data copy(x){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: x){{$}}
// PRT-OA-NEXT:   // #pragma acc data no_create(x){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
//
//  EXE-caseDataScalarPresent-NOT: {{.}}
//      EXE-caseDataScalarPresent: acc_ev_enter_data_start
// EXE-caseDataScalarPresent-NEXT:   acc_ev_alloc
// EXE-caseDataScalarPresent-NEXT:   acc_ev_create
// EXE-caseDataScalarPresent-NEXT:   acc_ev_enter_data_start
// EXE-caseDataScalarPresent-NEXT:   acc_ev_exit_data_start
// EXE-caseDataScalarPresent-NEXT: acc_ev_exit_data_start
// EXE-caseDataScalarPresent-NEXT:   acc_ev_delete
// EXE-caseDataScalarPresent-NEXT:   acc_ev_free
//  EXE-caseDataScalarPresent-NOT: {{.}}
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
//        EXE-caseDataScalarAbsent-NOT: {{.}}
//            EXE-caseDataScalarAbsent: acc_ev_enter_data_start
// EXE-caseDataScalarAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-caseDataScalarAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-caseDataScalarAbsent-NEXT: acc_ev_exit_data_start
// EXE-caseDataScalarAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-caseDataScalarAbsent-ALLOC-NEXT:   acc_ev_free
//        EXE-caseDataScalarAbsent-NOT: {{.}}
CASE(caseDataScalarAbsent) {
  int x;
  #pragma acc data no_create(x)
  ;
}

//   PRT-LABEL: {{.*}}caseDataArrayPresent{{.*}} {
//    PRT-NEXT:   int arr[3];
//
//  PRT-A-NEXT:   #pragma acc data copy(arr){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map(hold,tofrom: arr){{$}}
//  PRT-A-NEXT:   #pragma acc data no_create(arr){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: arr){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(hold,tofrom: arr){{$}}
// PRT-OA-NEXT:   // #pragma acc data copy(arr){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: arr){{$}}
// PRT-OA-NEXT:   // #pragma acc data no_create(arr){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
//
//  EXE-caseDataArrayPresent-NOT: {{.}}
//      EXE-caseDataArrayPresent: acc_ev_enter_data_start
// EXE-caseDataArrayPresent-NEXT:   acc_ev_alloc
// EXE-caseDataArrayPresent-NEXT:   acc_ev_create
// EXE-caseDataArrayPresent-NEXT:   acc_ev_enter_data_start
// EXE-caseDataArrayPresent-NEXT:   acc_ev_exit_data_start
// EXE-caseDataArrayPresent-NEXT: acc_ev_exit_data_start
// EXE-caseDataArrayPresent-NEXT:   acc_ev_delete
// EXE-caseDataArrayPresent-NEXT:   acc_ev_free
//  EXE-caseDataArrayPresent-NOT: {{.}}
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
//        EXE-caseDataArrayAbsent-NOT: {{.}}
//            EXE-caseDataArrayAbsent: acc_ev_enter_data_start
// EXE-caseDataArrayAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-caseDataArrayAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-caseDataArrayAbsent-NEXT: acc_ev_exit_data_start
// EXE-caseDataArrayAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-caseDataArrayAbsent-ALLOC-NEXT:   acc_ev_free
//        EXE-caseDataArrayAbsent-NOT: {{.}}
CASE(caseDataArrayAbsent) {
  int arr[3];
  #pragma acc data no_create(arr)
  ;
}

//   PRT-LABEL: {{.*}}caseDataSubarrayPresent{{.*}} {
//    PRT-NEXT:   int all[10], same[10], beg[10], mid[10], end[10];
//
//  PRT-A-NEXT:   #pragma acc data copy(all,same[3:6],beg[2:5],mid[1:8],end[0:5])
// PRT-AO-NEXT:   // #pragma omp target data map(hold,tofrom: all,same[3:6],beg[2:5],mid[1:8],end[0:5])
//  PRT-A-NEXT:   #pragma acc data no_create(all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
// PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
//
//  PRT-O-NEXT:   #pragma omp target data map(hold,tofrom: all,same[3:6],beg[2:5],mid[1:8],end[0:5])
// PRT-OA-NEXT:   // #pragma acc data copy(all,same[3:6],beg[2:5],mid[1:8],end[0:5])
//  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
// PRT-OA-NEXT:   // #pragma acc data no_create(all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
//
//  EXE-caseDataSubarrayPresent-NOT: {{.}}
//      EXE-caseDataSubarrayPresent: acc_ev_enter_data_start
// EXE-caseDataSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-caseDataSubarrayPresent-NEXT:   acc_ev_create
// EXE-caseDataSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-caseDataSubarrayPresent-NEXT:   acc_ev_create
// EXE-caseDataSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-caseDataSubarrayPresent-NEXT:   acc_ev_create
// EXE-caseDataSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-caseDataSubarrayPresent-NEXT:   acc_ev_create
// EXE-caseDataSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-caseDataSubarrayPresent-NEXT:   acc_ev_create
// EXE-caseDataSubarrayPresent-NEXT:   acc_ev_enter_data_start
// EXE-caseDataSubarrayPresent-NEXT:   acc_ev_exit_data_start
// EXE-caseDataSubarrayPresent-NEXT: acc_ev_exit_data_start
// EXE-caseDataSubarrayPresent-NEXT:   acc_ev_delete
// EXE-caseDataSubarrayPresent-NEXT:   acc_ev_free
// EXE-caseDataSubarrayPresent-NEXT:   acc_ev_delete
// EXE-caseDataSubarrayPresent-NEXT:   acc_ev_free
// EXE-caseDataSubarrayPresent-NEXT:   acc_ev_delete
// EXE-caseDataSubarrayPresent-NEXT:   acc_ev_free
// EXE-caseDataSubarrayPresent-NEXT:   acc_ev_delete
// EXE-caseDataSubarrayPresent-NEXT:   acc_ev_free
// EXE-caseDataSubarrayPresent-NEXT:   acc_ev_delete
// EXE-caseDataSubarrayPresent-NEXT:   acc_ev_free
//  EXE-caseDataSubarrayPresent-NOT: {{.}}
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
// PRT-AO-NEXT:   // #pragma omp target data map(hold,tofrom: arr[0:2]){{$}}
//  PRT-A-NEXT:   #pragma acc data no_create(arr[2:2]){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: arr[2:2]){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(hold,tofrom: arr[0:2]){{$}}
// PRT-OA-NEXT:   // #pragma acc data copy(arr[0:2]){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: arr[2:2]){{$}}
// PRT-OA-NEXT:   // #pragma acc data no_create(arr[2:2]){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
//
//        EXE-caseDataSubarrayDisjoint-NOT: {{.}}
//            EXE-caseDataSubarrayDisjoint: acc_ev_enter_data_start
//       EXE-caseDataSubarrayDisjoint-NEXT:   acc_ev_alloc
//       EXE-caseDataSubarrayDisjoint-NEXT:   acc_ev_create
//       EXE-caseDataSubarrayDisjoint-NEXT:   acc_ev_enter_data_start
// EXE-caseDataSubarrayDisjoint-ALLOC-NEXT:     acc_ev_alloc
// EXE-caseDataSubarrayDisjoint-ALLOC-NEXT:     acc_ev_create
//       EXE-caseDataSubarrayDisjoint-NEXT:   acc_ev_exit_data_start
// EXE-caseDataSubarrayDisjoint-ALLOC-NEXT:     acc_ev_delete
// EXE-caseDataSubarrayDisjoint-ALLOC-NEXT:     acc_ev_free
//       EXE-caseDataSubarrayDisjoint-NEXT: acc_ev_exit_data_start
//       EXE-caseDataSubarrayDisjoint-NEXT:   acc_ev_delete
//       EXE-caseDataSubarrayDisjoint-NEXT:   acc_ev_free
//        EXE-caseDataSubarrayDisjoint-NOT: {{.}}
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
// PRT-AO-NEXT:   // #pragma omp target data map(hold,to: arr[1:2]){{$}}
//  PRT-A-NEXT:   #pragma acc data no_create(arr[0:2]){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: arr[0:2]){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(hold,to: arr[1:2]){{$}}
// PRT-OA-NEXT:   // #pragma acc data copyin(arr[1:2]){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: arr[0:2]){{$}}
// PRT-OA-NEXT:   // #pragma acc data no_create(arr[0:2]){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
//
//  EXE-caseDataSubarrayOverlapStart-HOST-NOT: {{.}}
//      EXE-caseDataSubarrayOverlapStart-HOST: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
// EXE-caseDataSubarrayOverlapStart-HOST-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//
//           EXE-caseDataSubarrayOverlapStart-NOT: {{.}}
//               EXE-caseDataSubarrayOverlapStart: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//          EXE-caseDataSubarrayOverlapStart-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//          EXE-caseDataSubarrayOverlapStart-NEXT: acc_ev_enter_data_start
//          EXE-caseDataSubarrayOverlapStart-NEXT:   acc_ev_alloc
//          EXE-caseDataSubarrayOverlapStart-NEXT:   acc_ev_create
//          EXE-caseDataSubarrayOverlapStart-NEXT:   acc_ev_enter_data_start
//    EXE-caseDataSubarrayOverlapStart-ALLOC-NEXT:   Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//    EXE-caseDataSubarrayOverlapStart-ALLOC-NEXT:   Libomptarget error: Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping).
//    EXE-caseDataSubarrayOverlapStart-ALLOC-NEXT:   Libomptarget error: Run with LIBOMPTARGET_DEBUG=4 to dump host-target pointer mappings.
//    EXE-caseDataSubarrayOverlapStart-ALLOC-NEXT:   {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                                   # An abort message usually follows.
//     EXE-caseDataSubarrayOverlapStart-ALLOC-NOT:   Libomptarget
// EXE-caseDataSubarrayOverlapStart-NO-ALLOC-NEXT:   acc_ev_exit_data_start
// EXE-caseDataSubarrayOverlapStart-NO-ALLOC-NEXT: acc_ev_exit_data_start
// EXE-caseDataSubarrayOverlapStart-NO-ALLOC-NEXT:   acc_ev_delete
// EXE-caseDataSubarrayOverlapStart-NO-ALLOC-NEXT:   acc_ev_free
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
// PRT-AO-NEXT:   // #pragma omp target data map(hold,to: arr[1:2]){{$}}
//  PRT-A-NEXT:   #pragma acc data no_create(arr[2:2]){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: arr[2:2]){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(hold,to: arr[1:2]){{$}}
// PRT-OA-NEXT:   // #pragma acc data copyin(arr[1:2]){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: arr[2:2]){{$}}
// PRT-OA-NEXT:   // #pragma acc data no_create(arr[2:2]){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
//
//  EXE-caseDataSubarrayOverlapEnd-HOST-NOT: {{.}}
//      EXE-caseDataSubarrayOverlapEnd-HOST: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
// EXE-caseDataSubarrayOverlapEnd-HOST-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//
//           EXE-caseDataSubarrayOverlapEnd-NOT: {{.}}
//               EXE-caseDataSubarrayOverlapEnd: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//          EXE-caseDataSubarrayOverlapEnd-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//          EXE-caseDataSubarrayOverlapEnd-NEXT: acc_ev_enter_data_start
//          EXE-caseDataSubarrayOverlapEnd-NEXT:   acc_ev_alloc
//          EXE-caseDataSubarrayOverlapEnd-NEXT:   acc_ev_create
//          EXE-caseDataSubarrayOverlapEnd-NEXT:   acc_ev_enter_data_start
//    EXE-caseDataSubarrayOverlapEnd-ALLOC-NEXT:   Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//    EXE-caseDataSubarrayOverlapEnd-ALLOC-NEXT:   Libomptarget error: Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping).
//    EXE-caseDataSubarrayOverlapEnd-ALLOC-NEXT:   Libomptarget error: Run with LIBOMPTARGET_DEBUG=4 to dump host-target pointer mappings.
//    EXE-caseDataSubarrayOverlapEnd-ALLOC-NEXT:   {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                                 # An abort message usually follows.
//     EXE-caseDataSubarrayOverlapEnd-ALLOC-NOT:   Libomptarget
// EXE-caseDataSubarrayOverlapEnd-NO-ALLOC-NEXT:   acc_ev_exit_data_start
// EXE-caseDataSubarrayOverlapEnd-NO-ALLOC-NEXT: acc_ev_exit_data_start
// EXE-caseDataSubarrayOverlapEnd-NO-ALLOC-NEXT:   acc_ev_delete
// EXE-caseDataSubarrayOverlapEnd-NO-ALLOC-NEXT:   acc_ev_free
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
// PRT-AO-NEXT:   // #pragma omp target data map(hold,from: arr[0:2]){{$}}
//  PRT-A-NEXT:   #pragma acc data copy(arr[2:2]){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map(hold,tofrom: arr[2:2]){{$}}
//  PRT-A-NEXT:   #pragma acc data no_create(arr[0:4]){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: arr[0:4]){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(hold,from: arr[0:2]){{$}}
// PRT-OA-NEXT:   // #pragma acc data copyout(arr[0:2]){{$}}
//  PRT-O-NEXT:   #pragma omp target data map(hold,tofrom: arr[2:2]){{$}}
// PRT-OA-NEXT:   // #pragma acc data copy(arr[2:2]){{$}}
//  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: arr[0:4]){{$}}
// PRT-OA-NEXT:   // #pragma acc data no_create(arr[0:4]){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
//
//  EXE-caseDataSubarrayConcat2-HOST-NOT: {{.}}
//      EXE-caseDataSubarrayConcat2-HOST: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
// EXE-caseDataSubarrayConcat2-HOST-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//
//           EXE-caseDataSubarrayConcat2-NOT: {{.}}
//               EXE-caseDataSubarrayConcat2: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//          EXE-caseDataSubarrayConcat2-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//          EXE-caseDataSubarrayConcat2-NEXT: acc_ev_enter_data_start
//          EXE-caseDataSubarrayConcat2-NEXT:   acc_ev_alloc
//          EXE-caseDataSubarrayConcat2-NEXT:   acc_ev_create
//          EXE-caseDataSubarrayConcat2-NEXT:   acc_ev_enter_data_start
//          EXE-caseDataSubarrayConcat2-NEXT:     acc_ev_alloc
//          EXE-caseDataSubarrayConcat2-NEXT:     acc_ev_create
//          EXE-caseDataSubarrayConcat2-NEXT:     acc_ev_enter_data_start
//    EXE-caseDataSubarrayConcat2-ALLOC-NEXT:     Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//    EXE-caseDataSubarrayConcat2-ALLOC-NEXT:     Libomptarget error: Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping).
//    EXE-caseDataSubarrayConcat2-ALLOC-NEXT:     Libomptarget error: Run with LIBOMPTARGET_DEBUG=4 to dump host-target pointer mappings.
//    EXE-caseDataSubarrayConcat2-ALLOC-NEXT:     {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                                # An abort message usually follows.
//     EXE-caseDataSubarrayConcat2-ALLOC-NOT:     Libomptarget
// EXE-caseDataSubarrayConcat2-NO-ALLOC-NEXT:     acc_ev_exit_data_start
// EXE-caseDataSubarrayConcat2-NO-ALLOC-NEXT:   acc_ev_exit_data_start
// EXE-caseDataSubarrayConcat2-NO-ALLOC-NEXT:     acc_ev_delete
// EXE-caseDataSubarrayConcat2-NO-ALLOC-NEXT:     acc_ev_free
// EXE-caseDataSubarrayConcat2-NO-ALLOC-NEXT: acc_ev_exit_data_start
// EXE-caseDataSubarrayConcat2-NO-ALLOC-NEXT:   acc_ev_delete
// EXE-caseDataSubarrayConcat2-NO-ALLOC-NEXT:   acc_ev_free
CASE(caseDataSubarrayConcat2) {
  int arr[4];
  PRINT_SUBARRAY_INFO(arr, 0, 2);
  PRINT_SUBARRAY_INFO(arr, 0, 4);
  #pragma acc data copyout(arr[0:2])
  #pragma acc data copy(arr[2:2])
  #pragma acc data no_create(arr[0:4])
  ;
}

//  EXE-caseDataSubarrayNonSubarray-HOST-NOT: {{.}}
//      EXE-caseDataSubarrayNonSubarray-HOST: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
// EXE-caseDataSubarrayNonSubarray-HOST-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//
//           EXE-caseDataSubarrayNonSubarray-NOT: {{.}}
//               EXE-caseDataSubarrayNonSubarray: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//          EXE-caseDataSubarrayNonSubarray-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//          EXE-caseDataSubarrayNonSubarray-NEXT: acc_ev_enter_data_start
//          EXE-caseDataSubarrayNonSubarray-NEXT:   acc_ev_alloc
//          EXE-caseDataSubarrayNonSubarray-NEXT:   acc_ev_create
//          EXE-caseDataSubarrayNonSubarray-NEXT:   acc_ev_enter_data_start
//    EXE-caseDataSubarrayNonSubarray-ALLOC-NEXT:   Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//    EXE-caseDataSubarrayNonSubarray-ALLOC-NEXT:   Libomptarget error: Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping).
//    EXE-caseDataSubarrayNonSubarray-ALLOC-NEXT:   Libomptarget error: Run with LIBOMPTARGET_DEBUG=4 to dump host-target pointer mappings.
//    EXE-caseDataSubarrayNonSubarray-ALLOC-NEXT:   {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                                  # An abort message usually follows.
//     EXE-caseDataSubarrayNonSubarray-ALLOC-NOT:   Libomptarget
// EXE-caseDataSubarrayNonSubarray-NO-ALLOC-NEXT:   acc_ev_exit_data_start
// EXE-caseDataSubarrayNonSubarray-NO-ALLOC-NEXT: acc_ev_exit_data_start
// EXE-caseDataSubarrayNonSubarray-NO-ALLOC-NEXT:   acc_ev_delete
// EXE-caseDataSubarrayNonSubarray-NO-ALLOC-NEXT:   acc_ev_free
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
// PRT-AO-NEXT:   // #pragma omp target data map(hold,tofrom: x){{$}}
//  PRT-A-NEXT:   #pragma acc parallel no_create(x){{$}}
// PRT-AO-NEXT:   // #pragma omp target teams map([[NO_CREATE_MT]]: x) shared(x){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(hold,tofrom: x){{$}}
// PRT-OA-NEXT:   // #pragma acc data copy(x){{$}}
//  PRT-O-NEXT:   #pragma omp target teams map([[NO_CREATE_MT]]: x) shared(x){{$}}
// PRT-OA-NEXT:   // #pragma acc parallel no_create(x){{$}}
//
//    PRT-NEXT:   ;
//    PRT-NEXT: }
//
//  EXE-caseParallelScalarPresent-NOT: {{.}}
//      EXE-caseParallelScalarPresent: acc_ev_enter_data_start
// EXE-caseParallelScalarPresent-NEXT:   acc_ev_alloc
// EXE-caseParallelScalarPresent-NEXT:   acc_ev_create
// EXE-caseParallelScalarPresent-NEXT:   acc_ev_enter_data_start
// EXE-caseParallelScalarPresent-NEXT:   acc_ev_exit_data_start
// EXE-caseParallelScalarPresent-NEXT: acc_ev_exit_data_start
// EXE-caseParallelScalarPresent-NEXT:   acc_ev_delete
// EXE-caseParallelScalarPresent-NEXT:   acc_ev_free
//  EXE-caseParallelScalarPresent-NOT: {{.}}
CASE(caseParallelScalarPresent) {
  int x;
  #pragma acc data copy(x)
  #pragma acc parallel no_create(x)
  x = 5;
}

//        EXE-caseParallelScalarAbsent-NOT: {{.}}
//            EXE-caseParallelScalarAbsent: acc_ev_enter_data_start
// EXE-caseParallelScalarAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-caseParallelScalarAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-caseParallelScalarAbsent-NEXT: acc_ev_exit_data_start
// EXE-caseParallelScalarAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-caseParallelScalarAbsent-ALLOC-NEXT:   acc_ev_free
//        EXE-caseParallelScalarAbsent-NOT: {{.}}
CASE(caseParallelScalarAbsent) {
  int x;
  int use = 0;
  #pragma acc parallel no_create(x)
  if (use) x = 5;
}

//  EXE-caseParallelArrayPresent-NOT: {{.}}
//      EXE-caseParallelArrayPresent: acc_ev_enter_data_start
// EXE-caseParallelArrayPresent-NEXT:   acc_ev_alloc
// EXE-caseParallelArrayPresent-NEXT:   acc_ev_create
// EXE-caseParallelArrayPresent-NEXT:   acc_ev_enter_data_start
// EXE-caseParallelArrayPresent-NEXT:   acc_ev_exit_data_start
// EXE-caseParallelArrayPresent-NEXT: acc_ev_exit_data_start
// EXE-caseParallelArrayPresent-NEXT:   acc_ev_delete
// EXE-caseParallelArrayPresent-NEXT:   acc_ev_free
//  EXE-caseParallelArrayPresent-NOT: {{.}}
CASE(caseParallelArrayPresent) {
  int arr[3];
  #pragma acc data copy(arr)
  #pragma acc parallel no_create(arr)
  arr[0] = 5;
}

//        EXE-caseParallelArrayAbsent-NOT: {{.}}
//            EXE-caseParallelArrayAbsent: acc_ev_enter_data_start
// EXE-caseParallelArrayAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-caseParallelArrayAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-caseParallelArrayAbsent-NEXT: acc_ev_exit_data_start
// EXE-caseParallelArrayAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-caseParallelArrayAbsent-ALLOC-NEXT:   acc_ev_free
//        EXE-caseParallelArrayAbsent-NOT: {{.}}
CASE(caseParallelArrayAbsent) {
  int arr[3];
  int use = 0;
  #pragma acc parallel no_create(arr)
  if (use) arr[0] = 5;
}

//  EXE-caseParallelSubarrayPresent-NOT: {{.}}
//      EXE-caseParallelSubarrayPresent: acc_ev_enter_data_start
// EXE-caseParallelSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-caseParallelSubarrayPresent-NEXT:   acc_ev_create
// EXE-caseParallelSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-caseParallelSubarrayPresent-NEXT:   acc_ev_create
// EXE-caseParallelSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-caseParallelSubarrayPresent-NEXT:   acc_ev_create
// EXE-caseParallelSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-caseParallelSubarrayPresent-NEXT:   acc_ev_create
// EXE-caseParallelSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-caseParallelSubarrayPresent-NEXT:   acc_ev_create
// EXE-caseParallelSubarrayPresent-NEXT:   acc_ev_enter_data_start
// EXE-caseParallelSubarrayPresent-NEXT:   acc_ev_exit_data_start
// EXE-caseParallelSubarrayPresent-NEXT: acc_ev_exit_data_start
// EXE-caseParallelSubarrayPresent-NEXT:   acc_ev_delete
// EXE-caseParallelSubarrayPresent-NEXT:   acc_ev_free
// EXE-caseParallelSubarrayPresent-NEXT:   acc_ev_delete
// EXE-caseParallelSubarrayPresent-NEXT:   acc_ev_free
// EXE-caseParallelSubarrayPresent-NEXT:   acc_ev_delete
// EXE-caseParallelSubarrayPresent-NEXT:   acc_ev_free
// EXE-caseParallelSubarrayPresent-NEXT:   acc_ev_delete
// EXE-caseParallelSubarrayPresent-NEXT:   acc_ev_free
// EXE-caseParallelSubarrayPresent-NEXT:   acc_ev_delete
// EXE-caseParallelSubarrayPresent-NEXT:   acc_ev_free
//  EXE-caseParallelSubarrayPresent-NOT: {{.}}
CASE(caseParallelSubarrayPresent) {
  int all[10], same[10], beg[10], mid[10], end[10];
  #pragma acc data copy(all,same[3:6],beg[2:5],mid[1:8],end[0:5])
  #pragma acc parallel no_create(all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
  {
    all[0] = 1; same[3] = 1; beg[2] = 1; mid[3] = 1; end[4] = 1;
  }
}

//        EXE-caseParallelSubarrayDisjoint-NOT: {{.}}
//            EXE-caseParallelSubarrayDisjoint: acc_ev_enter_data_start
//       EXE-caseParallelSubarrayDisjoint-NEXT:   acc_ev_alloc
//       EXE-caseParallelSubarrayDisjoint-NEXT:   acc_ev_create
//       EXE-caseParallelSubarrayDisjoint-NEXT:   acc_ev_enter_data_start
// EXE-caseParallelSubarrayDisjoint-ALLOC-NEXT:     acc_ev_alloc
// EXE-caseParallelSubarrayDisjoint-ALLOC-NEXT:     acc_ev_create
//       EXE-caseParallelSubarrayDisjoint-NEXT:   acc_ev_exit_data_start
// EXE-caseParallelSubarrayDisjoint-ALLOC-NEXT:     acc_ev_delete
// EXE-caseParallelSubarrayDisjoint-ALLOC-NEXT:     acc_ev_free
//       EXE-caseParallelSubarrayDisjoint-NEXT: acc_ev_exit_data_start
//       EXE-caseParallelSubarrayDisjoint-NEXT:   acc_ev_delete
//       EXE-caseParallelSubarrayDisjoint-NEXT:   acc_ev_free
//        EXE-caseParallelSubarrayDisjoint-NOT: {{.}}
CASE(caseParallelSubarrayDisjoint) {
  int arr[4];
  int use = 0;
  #pragma acc data copy(arr[0:2])
  #pragma acc parallel no_create(arr[2:2])
  if (use) arr[2] = 1;
}

//  EXE-caseParallelSubarrayOverlapStart-HOST-NOT: {{.}}
//      EXE-caseParallelSubarrayOverlapStart-HOST: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
// EXE-caseParallelSubarrayOverlapStart-HOST-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//
//
//           EXE-caseParallelSubarrayOverlapStart-NOT: {{.}}
//               EXE-caseParallelSubarrayOverlapStart: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//          EXE-caseParallelSubarrayOverlapStart-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//               EXE-caseParallelSubarrayOverlapStart: acc_ev_enter_data_start
//          EXE-caseParallelSubarrayOverlapStart-NEXT:   acc_ev_alloc
//          EXE-caseParallelSubarrayOverlapStart-NEXT:   acc_ev_create
//          EXE-caseParallelSubarrayOverlapStart-NEXT:   acc_ev_enter_data_start
//    EXE-caseParallelSubarrayOverlapStart-ALLOC-NEXT:   Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//                                                       # FIXME: Names like getOrAllocTgtPtr are meaningless to users.
//    EXE-caseParallelSubarrayOverlapStart-ALLOC-NEXT:   Libomptarget error: Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping).
//    EXE-caseParallelSubarrayOverlapStart-ALLOC-NEXT:   Libomptarget error: Call to targetDataBegin failed, abort target.
//    EXE-caseParallelSubarrayOverlapStart-ALLOC-NEXT:   Libomptarget error: Failed to process data before launching the kernel.
//    EXE-caseParallelSubarrayOverlapStart-ALLOC-NEXT:   Libomptarget error: Run with LIBOMPTARGET_DEBUG=4 to dump host-target pointer mappings.
//    EXE-caseParallelSubarrayOverlapStart-ALLOC-NEXT:   {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                                       # An abort message usually follows.
//     EXE-caseParallelSubarrayOverlapStart-ALLOC-NOT:   Libomptarget
// EXE-caseParallelSubarrayOverlapStart-NO-ALLOC-NEXT:   acc_ev_exit_data_start
// EXE-caseParallelSubarrayOverlapStart-NO-ALLOC-NEXT: acc_ev_exit_data_start
// EXE-caseParallelSubarrayOverlapStart-NO-ALLOC-NEXT:   acc_ev_delete
// EXE-caseParallelSubarrayOverlapStart-NO-ALLOC-NEXT:   acc_ev_free
CASE(caseParallelSubarrayOverlapStart) {
  int arr[10];
  PRINT_SUBARRAY_INFO(arr, 5, 4);
  PRINT_SUBARRAY_INFO(arr, 4, 4);
  int use = 0;
  #pragma acc data copyin(arr[5:4])
  #pragma acc parallel no_create(arr[4:4])
  if (use) arr[4] = 1;
}

//  EXE-caseParallelSubarrayOverlapEnd-HOST-NOT: {{.}}
//      EXE-caseParallelSubarrayOverlapEnd-HOST: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
// EXE-caseParallelSubarrayOverlapEnd-HOST-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//
//           EXE-caseParallelSubarrayOverlapEnd-NOT: {{.}}
//               EXE-caseParallelSubarrayOverlapEnd: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//          EXE-caseParallelSubarrayOverlapEnd-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//          EXE-caseParallelSubarrayOverlapEnd-NEXT: acc_ev_enter_data_start
//          EXE-caseParallelSubarrayOverlapEnd-NEXT:   acc_ev_alloc
//          EXE-caseParallelSubarrayOverlapEnd-NEXT:   acc_ev_create
//          EXE-caseParallelSubarrayOverlapEnd-NEXT:   acc_ev_enter_data_start
//    EXE-caseParallelSubarrayOverlapEnd-ALLOC-NEXT:   Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//                                                     # FIXME: Names like getOrAllocTgtPtr are meaningless to users.
//    EXE-caseParallelSubarrayOverlapEnd-ALLOC-NEXT:   Libomptarget error: Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping).
//    EXE-caseParallelSubarrayOverlapEnd-ALLOC-NEXT:   Libomptarget error: Call to targetDataBegin failed, abort target.
//    EXE-caseParallelSubarrayOverlapEnd-ALLOC-NEXT:   Libomptarget error: Failed to process data before launching the kernel.
//    EXE-caseParallelSubarrayOverlapEnd-ALLOC-NEXT:   Libomptarget error: Run with LIBOMPTARGET_DEBUG=4 to dump host-target pointer mappings.
//    EXE-caseParallelSubarrayOverlapEnd-ALLOC-NEXT:   {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                                     # An abort message usually follows.
//     EXE-caseParallelSubarrayOverlapEnd-ALLOC-NOT:   Libomptarget
// EXE-caseParallelSubarrayOverlapEnd-NO-ALLOC-NEXT:   acc_ev_exit_data_start
// EXE-caseParallelSubarrayOverlapEnd-NO-ALLOC-NEXT: acc_ev_exit_data_start
// EXE-caseParallelSubarrayOverlapEnd-NO-ALLOC-NEXT:   acc_ev_delete
// EXE-caseParallelSubarrayOverlapEnd-NO-ALLOC-NEXT:   acc_ev_free
CASE(caseParallelSubarrayOverlapEnd) {
  int arr[10];
  PRINT_SUBARRAY_INFO(arr, 3, 4);
  PRINT_SUBARRAY_INFO(arr, 4, 4);
  int use = 0;
  #pragma acc data copyin(arr[3:4])
  #pragma acc parallel no_create(arr[4:4])
  if (use) arr[4] = 1;
}

//  EXE-caseParallelSubarrayConcat2-HOST-NOT: {{.}}
//      EXE-caseParallelSubarrayConcat2-HOST: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
// EXE-caseParallelSubarrayConcat2-HOST-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//
//           EXE-caseParallelSubarrayConcat2-NOT: {{.}}
//               EXE-caseParallelSubarrayConcat2: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//          EXE-caseParallelSubarrayConcat2-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//          EXE-caseParallelSubarrayConcat2-NEXT: acc_ev_enter_data_start
//          EXE-caseParallelSubarrayConcat2-NEXT:   acc_ev_alloc
//          EXE-caseParallelSubarrayConcat2-NEXT:   acc_ev_create
//          EXE-caseParallelSubarrayConcat2-NEXT:   acc_ev_enter_data_start
//          EXE-caseParallelSubarrayConcat2-NEXT:     acc_ev_alloc
//          EXE-caseParallelSubarrayConcat2-NEXT:     acc_ev_create
//          EXE-caseParallelSubarrayConcat2-NEXT:     acc_ev_enter_data_start
//    EXE-caseParallelSubarrayConcat2-ALLOC-NEXT:     Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//                                                    # FIXME: Names like getOrAllocTgtPtr are meaningless to users.
//    EXE-caseParallelSubarrayConcat2-ALLOC-NEXT:     Libomptarget error: Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping).
//    EXE-caseParallelSubarrayConcat2-ALLOC-NEXT:     Libomptarget error: Call to targetDataBegin failed, abort target.
//    EXE-caseParallelSubarrayConcat2-ALLOC-NEXT:     Libomptarget error: Failed to process data before launching the kernel.
//    EXE-caseParallelSubarrayConcat2-ALLOC-NEXT:     Libomptarget error: Run with LIBOMPTARGET_DEBUG=4 to dump host-target pointer mappings.
//    EXE-caseParallelSubarrayConcat2-ALLOC-NEXT:     {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                                    # An abort message usually follows.
//     EXE-caseParallelSubarrayConcat2-ALLOC-NOT:     Libomptarget
// EXE-caseParallelSubarrayConcat2-NO-ALLOC-NEXT:     acc_ev_exit_data_start
// EXE-caseParallelSubarrayConcat2-NO-ALLOC-NEXT:   acc_ev_exit_data_start
// EXE-caseParallelSubarrayConcat2-NO-ALLOC-NEXT:     acc_ev_delete
// EXE-caseParallelSubarrayConcat2-NO-ALLOC-NEXT:     acc_ev_free
// EXE-caseParallelSubarrayConcat2-NO-ALLOC-NEXT: acc_ev_exit_data_start
// EXE-caseParallelSubarrayConcat2-NO-ALLOC-NEXT:   acc_ev_delete
// EXE-caseParallelSubarrayConcat2-NO-ALLOC-NEXT:   acc_ev_free
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

//  EXE-caseParallelSubarrayNonSubarray-HOST-NOT: {{.}}
//      EXE-caseParallelSubarrayNonSubarray-HOST: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
// EXE-caseParallelSubarrayNonSubarray-HOST-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//
//           EXE-caseParallelSubarrayNonSubarray-NOT: {{.}}
//               EXE-caseParallelSubarrayNonSubarray: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//          EXE-caseParallelSubarrayNonSubarray-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//          EXE-caseParallelSubarrayNonSubarray-NEXT: acc_ev_enter_data_start
//          EXE-caseParallelSubarrayNonSubarray-NEXT:   acc_ev_alloc
//          EXE-caseParallelSubarrayNonSubarray-NEXT:   acc_ev_create
//          EXE-caseParallelSubarrayNonSubarray-NEXT:   acc_ev_enter_data_start
//    EXE-caseParallelSubarrayNonSubarray-ALLOC-NEXT:   Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//                                                      # FIXME: Names like getOrAllocTgtPtr are meaningless to users.
//    EXE-caseParallelSubarrayNonSubarray-ALLOC-NEXT:   Libomptarget error: Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping).
//    EXE-caseParallelSubarrayNonSubarray-ALLOC-NEXT:   Libomptarget error: Call to targetDataBegin failed, abort target.
//    EXE-caseParallelSubarrayNonSubarray-ALLOC-NEXT:   Libomptarget error: Failed to process data before launching the kernel.
//    EXE-caseParallelSubarrayNonSubarray-ALLOC-NEXT:   Libomptarget error: Run with LIBOMPTARGET_DEBUG=4 to dump host-target pointer mappings.
//    EXE-caseParallelSubarrayNonSubarray-ALLOC-NEXT:   {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                                      # An abort message usually follows.
//     EXE-caseParallelSubarrayNonSubarray-ALLOC-NOT:   Libomptarget
// EXE-caseParallelSubarrayNonSubarray-NO-ALLOC-NEXT:   acc_ev_exit_data_start
// EXE-caseParallelSubarrayNonSubarray-NO-ALLOC-NEXT: acc_ev_exit_data_start
// EXE-caseParallelSubarrayNonSubarray-NO-ALLOC-NEXT:   acc_ev_delete
// EXE-caseParallelSubarrayNonSubarray-NO-ALLOC-NEXT:   acc_ev_free
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
// PRT-AO-NEXT:   // #pragma omp target data map(hold,tofrom: x){{$}}
//  PRT-A-NEXT:   #pragma acc parallel loop no_create(x){{$}}
// PRT-AO-NEXT:   // #pragma omp target teams map([[NO_CREATE_MT]]: x) shared(x){{$}}
// PRT-AO-NEXT:   // #pragma omp distribute{{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(hold,tofrom: x){{$}}
// PRT-OA-NEXT:   // #pragma acc data copy(x){{$}}
//  PRT-O-NEXT:   #pragma omp target teams map([[NO_CREATE_MT]]: x) shared(x){{$}}
//  PRT-O-NEXT:   #pragma omp distribute{{$}}
// PRT-OA-NEXT:   // #pragma acc parallel loop no_create(x){{$}}
//
//    PRT-NEXT:   for ({{.*}})
//    PRT-NEXT:     ;
//    PRT-NEXT: }
//
//  EXE-caseParallelLoopScalarPresent-NOT: {{.}}
//      EXE-caseParallelLoopScalarPresent: acc_ev_enter_data_start
// EXE-caseParallelLoopScalarPresent-NEXT:   acc_ev_alloc
// EXE-caseParallelLoopScalarPresent-NEXT:   acc_ev_create
// EXE-caseParallelLoopScalarPresent-NEXT:   acc_ev_enter_data_start
// EXE-caseParallelLoopScalarPresent-NEXT:   acc_ev_exit_data_start
// EXE-caseParallelLoopScalarPresent-NEXT: acc_ev_exit_data_start
// EXE-caseParallelLoopScalarPresent-NEXT:   acc_ev_delete
// EXE-caseParallelLoopScalarPresent-NEXT:   acc_ev_free
//  EXE-caseParallelLoopScalarPresent-NOT: {{.}}
CASE(caseParallelLoopScalarPresent) {
  int x;
  #pragma acc data copy(x)
  #pragma acc parallel loop no_create(x)
  for (int i = 0; i < 2; ++i)
  x = 1;
}

//        EXE-caseParallelLoopScalarAbsent-NOT: {{.}}
//            EXE-caseParallelLoopScalarAbsent: acc_ev_enter_data_start
// EXE-caseParallelLoopScalarAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-caseParallelLoopScalarAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-caseParallelLoopScalarAbsent-NEXT: acc_ev_exit_data_start
// EXE-caseParallelLoopScalarAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-caseParallelLoopScalarAbsent-ALLOC-NEXT:   acc_ev_free
//        EXE-caseParallelLoopScalarAbsent-NOT: {{.}}
CASE(caseParallelLoopScalarAbsent) {
  int x;
  int use = 0;
  #pragma acc parallel loop no_create(x)
  for (int i = 0; i < 2; ++i)
  if (use) x = 1;
}

//  EXE-caseConstPresent-NOT: {{.}}
//      EXE-caseConstPresent: acc_ev_enter_data_start
// EXE-caseConstPresent-NEXT:   acc_ev_alloc
// EXE-caseConstPresent-NEXT:   acc_ev_create
// EXE-caseConstPresent-NEXT:   acc_ev_enter_data_start
// EXE-caseConstPresent-NEXT:   acc_ev_exit_data_start
// EXE-caseConstPresent-NEXT: acc_ev_exit_data_start
// EXE-caseConstPresent-NEXT:   acc_ev_delete
// EXE-caseConstPresent-NEXT:   acc_ev_free
//  EXE-caseConstPresent-NOT: {{.}}
CASE(caseConstPresent) {
  const int x;
  int y;
  #pragma acc data copy(x)
  #pragma acc parallel loop no_create(x)
  for (int i = 0; i < 2; ++i)
  y = x;
}

//        EXE-caseConstAbsent-NOT: {{.}}
//            EXE-caseConstAbsent: acc_ev_enter_data_start
// EXE-caseConstAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-caseConstAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-caseConstAbsent-NEXT: acc_ev_exit_data_start
// EXE-caseConstAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-caseConstAbsent-ALLOC-NEXT:   acc_ev_free
//        EXE-caseConstAbsent-NOT: {{.}}
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
//          PRT-AO-NEXT:   // #pragma omp target data map(hold,alloc: x){{$}}
//           PRT-A-NEXT:   #pragma acc data no_create(x){{$}}
//          PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: x){{$}}
//           PRT-A-NEXT:   #pragma acc parallel{{$}}
// PRT-AO-NO-ALLOC-NEXT:   // #pragma omp target teams map([[INHERITED_NO_CREATE_MT]]: x) shared(x){{$}}
//    PRT-AO-ALLOC-NEXT:   // #pragma omp target teams shared(x) defaultmap(tofrom: scalar){{$}}
//
//           PRT-O-NEXT:   #pragma omp target data map(hold,alloc: x){{$}}
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
//  EXE-caseInheritedPresent-NOT: {{.}}
//      EXE-caseInheritedPresent: acc_ev_enter_data_start
// EXE-caseInheritedPresent-NEXT:   acc_ev_alloc
// EXE-caseInheritedPresent-NEXT:   acc_ev_create
// EXE-caseInheritedPresent-NEXT:   acc_ev_enter_data_start
// EXE-caseInheritedPresent-NEXT:     acc_ev_enter_data_start
// EXE-caseInheritedPresent-NEXT:     acc_ev_exit_data_start
// EXE-caseInheritedPresent-NEXT:   acc_ev_exit_data_start
// EXE-caseInheritedPresent-NEXT: acc_ev_exit_data_start
// EXE-caseInheritedPresent-NEXT:   acc_ev_delete
// EXE-caseInheritedPresent-NEXT:   acc_ev_free
//  EXE-caseInheritedPresent-NOT: {{.}}
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
//        EXE-caseInheritedAbsent-NOT: {{.}}
//            EXE-caseInheritedAbsent: acc_ev_enter_data_start
// EXE-caseInheritedAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-caseInheritedAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-caseInheritedAbsent-NEXT:   acc_ev_enter_data_start
//       EXE-caseInheritedAbsent-NEXT:   acc_ev_exit_data_start
//       EXE-caseInheritedAbsent-NEXT: acc_ev_exit_data_start
// EXE-caseInheritedAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-caseInheritedAbsent-ALLOC-NEXT:   acc_ev_free
//        EXE-caseInheritedAbsent-NOT: {{.}}
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
//          PRT-AO-NEXT:   // #pragma omp target data map(hold,alloc: arr){{$}}
//           PRT-A-NEXT:   #pragma acc data no_create(arr[1:2]){{$}}
//          PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: arr[1:2]){{$}}
//           PRT-A-NEXT:   #pragma acc parallel{{$}}
// PRT-AO-NO-ALLOC-NEXT:   // #pragma omp target teams map([[INHERITED_NO_CREATE_MT]]: arr) shared(arr){{$}}
//    PRT-AO-ALLOC-NEXT:   // #pragma omp target teams shared(arr){{$}}
//
//           PRT-O-NEXT:   #pragma omp target data map(hold,alloc: arr){{$}}
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
//  EXE-caseInheritedSubarrayPresent-NOT: {{.}}
//      EXE-caseInheritedSubarrayPresent: acc_ev_enter_data_start
// EXE-caseInheritedSubarrayPresent-NEXT:   acc_ev_alloc
// EXE-caseInheritedSubarrayPresent-NEXT:   acc_ev_create
// EXE-caseInheritedSubarrayPresent-NEXT:   acc_ev_enter_data_start
// EXE-caseInheritedSubarrayPresent-NEXT:     acc_ev_enter_data_start
// EXE-caseInheritedSubarrayPresent-NEXT:     acc_ev_exit_data_start
// EXE-caseInheritedSubarrayPresent-NEXT:   acc_ev_exit_data_start
// EXE-caseInheritedSubarrayPresent-NEXT: acc_ev_exit_data_start
// EXE-caseInheritedSubarrayPresent-NEXT:   acc_ev_delete
// EXE-caseInheritedSubarrayPresent-NEXT:   acc_ev_free
//  EXE-caseInheritedSubarrayPresent-NOT: {{.}}
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
//        EXE-caseInheritedSubarrayAbsent-NOT: {{.}}
//            EXE-caseInheritedSubarrayAbsent: acc_ev_enter_data_start
// EXE-caseInheritedSubarrayAbsent-ALLOC-NEXT:   acc_ev_alloc
// EXE-caseInheritedSubarrayAbsent-ALLOC-NEXT:   acc_ev_create
//       EXE-caseInheritedSubarrayAbsent-NEXT:   acc_ev_enter_data_start
//       EXE-caseInheritedSubarrayAbsent-NEXT:   acc_ev_exit_data_start
//       EXE-caseInheritedSubarrayAbsent-NEXT: acc_ev_exit_data_start
// EXE-caseInheritedSubarrayAbsent-ALLOC-NEXT:   acc_ev_delete
// EXE-caseInheritedSubarrayAbsent-ALLOC-NEXT:   acc_ev_free
//        EXE-caseInheritedSubarrayAbsent-NOT: {{.}}
CASE(caseInheritedSubarrayAbsent) {
  int arr[] = {10, 20, 30, 40, 50};
  int use = 0;
  #pragma acc data no_create(arr[1:2])
  #pragma acc parallel
  if (use)
    arr[1] = 1;
}

// EXE-HOST-NOT: {{.}}
