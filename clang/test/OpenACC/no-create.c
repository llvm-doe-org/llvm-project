// Check no_create clauses on different constructs and with different values of
// -fopenacc-no-create-omp.  Diagnostics about no_alloc in the translation are
// tested in warn-acc-omp-map-no-alloc.c.  data.c tests various interactions
// with explicit DAs and the defaultmap added for scalars with suppressed
// OpenACC implicit DAs.
//
// The various cases covered here should be kept consistent with present.c and
// update.c.  For example, a subarray that extends a subarray already present is
// consistently considered not present, so the present clause produces a runtime
// error and the no_create clause doesn't allocate.  However, INHERITED cases
// have no meaning for the present clause.
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
// RUN:   (case=CASE_DATA_SCALAR_PRESENT             not-crash-if-fail=                             )
// RUN:   (case=CASE_DATA_SCALAR_ABSENT              not-crash-if-fail=                             )
// RUN:   (case=CASE_DATA_ARRAY_PRESENT              not-crash-if-fail=                             )
// RUN:   (case=CASE_DATA_ARRAY_ABSENT               not-crash-if-fail=                             )
// RUN:   (case=CASE_DATA_SUBARRAY_PRESENT           not-crash-if-fail=                             )
// RUN:   (case=CASE_DATA_SUBARRAY_DISJOINT          not-crash-if-fail=                             )
// RUN:   (case=CASE_DATA_SUBARRAY_OVERLAP_START     not-crash-if-fail=%[not-crash-if-off-and-alloc])
// RUN:   (case=CASE_DATA_SUBARRAY_OVERLAP_END       not-crash-if-fail=%[not-crash-if-off-and-alloc])
// RUN:   (case=CASE_DATA_SUBARRAY_CONCAT2           not-crash-if-fail=%[not-crash-if-off-and-alloc])
// RUN:   (case=CASE_PARALLEL_SCALAR_PRESENT         not-crash-if-fail=                             )
// RUN:   (case=CASE_PARALLEL_SCALAR_ABSENT          not-crash-if-fail=                             )
// RUN:   (case=CASE_PARALLEL_ARRAY_PRESENT          not-crash-if-fail=                             )
// RUN:   (case=CASE_PARALLEL_ARRAY_ABSENT           not-crash-if-fail=                             )
// RUN:   (case=CASE_PARALLEL_SUBARRAY_PRESENT       not-crash-if-fail=                             )
// RUN:   (case=CASE_PARALLEL_SUBARRAY_DISJOINT      not-crash-if-fail=                             )
// RUN:   (case=CASE_PARALLEL_SUBARRAY_OVERLAP_START not-crash-if-fail=%[not-crash-if-off-and-alloc])
// RUN:   (case=CASE_PARALLEL_SUBARRAY_OVERLAP_END   not-crash-if-fail=%[not-crash-if-off-and-alloc])
// RUN:   (case=CASE_PARALLEL_SUBARRAY_CONCAT2       not-crash-if-fail=%[not-crash-if-off-and-alloc])
// RUN:   (case=CASE_PARALLEL_LOOP_SCALAR_PRESENT    not-crash-if-fail=                             )
// RUN:   (case=CASE_PARALLEL_LOOP_SCALAR_ABSENT     not-crash-if-fail=                             )
// RUN:   (case=CASE_CONST_PRESENT                   not-crash-if-fail=                             )
// RUN:   (case=CASE_CONST_ABSENT                    not-crash-if-fail=                             )
// RUN:   (case=CASE_INHERITED_PRESENT               not-crash-if-fail=                             )
// RUN:   (case=CASE_INHERITED_ABSENT                not-crash-if-fail=                             )
// RUN:   (case=CASE_INHERITED_SUBARRAY_PRESENT      not-crash-if-fail=                             )
// RUN:   (case=CASE_INHERITED_SUBARRAY_ABSENT       not-crash-if-fail=                             )
// RUN: }

// Check -ast-dump before and after AST serialization.
//
// We include dump checking on only a few representative cases, which should be
// more than sufficient to show it's working for the no_create clause.
//
// RUN: %for no-create-opts {
// RUN:   %clang -Xclang -verify -Xclang -ast-dump -fsyntax-only -fopenacc %s \
// RUN:          %acc_includes %[no-create-opt] \
// RUN:   | FileCheck -check-prefixes=DMP,DMP-%[noAlloc-or-alloc] %s
// RUN:   %clang -Xclang -verify -fopenacc -emit-ast -o %t.ast %s \
// RUN:          %acc_includes %[no-create-opt]
// RUN:   %clang_cc1 -ast-dump-all %t.ast \
// RUN:   | FileCheck -check-prefixes=DMP,DMP-%[noAlloc-or-alloc] %s
// RUN: }

// Check -ast-print and -fopenacc[-ast]-print.
//
// We include print checking on only a few representative cases, which should be
// more than sufficient to show it's working for the no_create clause.
//
// RUN: %clang -Xclang -verify -Xclang -ast-print -fsyntax-only %acc_includes \
// RUN:        %s \
// RUN: | FileCheck -check-prefixes=PRT,PRT-NOACC %s
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
// RUN:     %clang -Xclang -verify %[prt] %[no-create-opt] %acc_includes \
// RUN:            %t-acc.c -Wno-openacc-omp-map-hold \
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
// RUN:          %acc_includes -o %t.ast %t-acc.c
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
// RUN:                 %[no-create-opt] %acc_includes %s > %t-omp.c \
// RUN:                 -Wno-openacc-omp-map-hold
// RUN:       %[run-if] echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN:       %[run-if] %clang -Xclang -verify -fopenmp %fopenmp-version \
// RUN:                 %[tgt-cflags] %acc_includes -o %t.exe %t-omp.c
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
// RUN:               %[tgt-cflags] %acc_includes -o %t.exe %s
// RUN:     rm -f %t.actual-cases && touch %t.actual-cases
// RUN:     %for cases {
// RUN:       %[run-if] %[not-crash-if-fail] %t.exe %[case] > %t.out 2>&1
// RUN:       %[run-if] FileCheck -input-file %t.out %s \
// RUN:         -match-full-lines -allow-empty \
// RUN:         -check-prefixes=EXE%[host],EXE-%[case]%[host] \
// RUN:         -check-prefixes=EXE-%[case]-%[noAlloc-or-alloc]%[host]
// RUN:       echo '%[case]' >> %t.actual-cases
// RUN:     }
// RUN:   }
// RUN: }

// Make sure %data cases didn't omit any cases defined in the code.
//
// RUN: %t.exe -dump-cases > %t.expected-cases
// RUN: diff -u %t.expected-cases %t.actual-cases >&2

// END.

// expected-no-diagnostics

#include <acc_prof.h>
#include <stdio.h>
#include <string.h>

#define FOREACH_CASE(Macro)                   \
  Macro(CASE_DATA_SCALAR_PRESENT)             \
  Macro(CASE_DATA_SCALAR_ABSENT)              \
  Macro(CASE_DATA_ARRAY_PRESENT)              \
  Macro(CASE_DATA_ARRAY_ABSENT)               \
  Macro(CASE_DATA_SUBARRAY_PRESENT)           \
  Macro(CASE_DATA_SUBARRAY_DISJOINT)          \
  Macro(CASE_DATA_SUBARRAY_OVERLAP_START)     \
  Macro(CASE_DATA_SUBARRAY_OVERLAP_END)       \
  Macro(CASE_DATA_SUBARRAY_CONCAT2)           \
  Macro(CASE_PARALLEL_SCALAR_PRESENT)         \
  Macro(CASE_PARALLEL_SCALAR_ABSENT)          \
  Macro(CASE_PARALLEL_ARRAY_PRESENT)          \
  Macro(CASE_PARALLEL_ARRAY_ABSENT)           \
  Macro(CASE_PARALLEL_SUBARRAY_PRESENT)       \
  Macro(CASE_PARALLEL_SUBARRAY_DISJOINT)      \
  Macro(CASE_PARALLEL_SUBARRAY_OVERLAP_START) \
  Macro(CASE_PARALLEL_SUBARRAY_OVERLAP_END)   \
  Macro(CASE_PARALLEL_SUBARRAY_CONCAT2)       \
  Macro(CASE_PARALLEL_LOOP_SCALAR_PRESENT)    \
  Macro(CASE_PARALLEL_LOOP_SCALAR_ABSENT)     \
  Macro(CASE_CONST_PRESENT)                   \
  Macro(CASE_CONST_ABSENT)                    \
  Macro(CASE_INHERITED_PRESENT)               \
  Macro(CASE_INHERITED_ABSENT)                \
  Macro(CASE_INHERITED_SUBARRAY_PRESENT)      \
  Macro(CASE_INHERITED_SUBARRAY_ABSENT)

enum Case {
#define AddCase(CaseName) \
  CaseName,
FOREACH_CASE(AddCase)
  CASE_END
#undef AddCase
};

const char *CaseNames[] = {
#define AddCase(CaseName) \
  #CaseName,
FOREACH_CASE(AddCase)
#undef AddCase
};

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

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "expected one argument\n");
    return 1;
  }
  if (!strcmp(argv[1], "-dump-cases")) {
    for (int i = 0; i < CASE_END; ++i)
      printf("%s\n", CaseNames[i]);
    return 0;
  }
  enum Case selectedCase;
  for (selectedCase = 0; selectedCase < CASE_END; ++selectedCase) {
    if (!strcmp(argv[1], CaseNames[selectedCase]))
      break;
  }
  if (selectedCase == CASE_END) {
    fprintf(stderr, "unexpected case: %s\n", argv[1]);
    return 1;
  }

  // DMP-LABEL: SwitchStmt
  // PRT-LABEL: switch (selectedCase)
  switch (selectedCase) {
  // DMP-LABEL: CASE_DATA_SCALAR_PRESENT
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
  //   PRT-LABEL: case CASE_DATA_SCALAR_PRESENT:
  //    PRT-NEXT: {
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
  //    PRT-NEXT:   break;
  //    PRT-NEXT: }
  //
  //  EXE-CASE_DATA_SCALAR_PRESENT-NOT: {{.}}
  //      EXE-CASE_DATA_SCALAR_PRESENT: acc_ev_enter_data_start
  // EXE-CASE_DATA_SCALAR_PRESENT-NEXT:   acc_ev_alloc
  // EXE-CASE_DATA_SCALAR_PRESENT-NEXT:   acc_ev_create
  // EXE-CASE_DATA_SCALAR_PRESENT-NEXT:   acc_ev_enter_data_start
  // EXE-CASE_DATA_SCALAR_PRESENT-NEXT:   acc_ev_exit_data_start
  // EXE-CASE_DATA_SCALAR_PRESENT-NEXT: acc_ev_exit_data_start
  // EXE-CASE_DATA_SCALAR_PRESENT-NEXT:   acc_ev_delete
  // EXE-CASE_DATA_SCALAR_PRESENT-NEXT:   acc_ev_free
  //  EXE-CASE_DATA_SCALAR_PRESENT-NOT: {{.}}
  case CASE_DATA_SCALAR_PRESENT:
  {
    int x;
    #pragma acc data copy(x)
    #pragma acc data no_create(x)
    ;
    break;
  }

  // DMP-LABEL: CASE_DATA_SCALAR_ABSENT
  //       DMP: ACCDataDirective
  //  DMP-NEXT:   ACCNoCreateClause
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'x' 'int'
  //  DMP-NEXT:   impl: OMPTargetDataDirective
  //  DMP-NEXT:     OMPMapClause
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'x' 'int'
  //
  //   PRT-LABEL: case CASE_DATA_SCALAR_ABSENT:
  //    PRT-NEXT: {
  //    PRT-NEXT:   int x;
  //  PRT-A-NEXT:   #pragma acc data no_create(x){{$}}
  // PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: x){{$}}
  //  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: x){{$}}
  // PRT-OA-NEXT:   // #pragma acc data no_create(x){{$}}
  //    PRT-NEXT:   ;
  //    PRT-NEXT:   break;
  //    PRT-NEXT: }
  //
  //        EXE-CASE_DATA_SCALAR_ABSENT-NOT: {{.}}
  //            EXE-CASE_DATA_SCALAR_ABSENT: acc_ev_enter_data_start
  // EXE-CASE_DATA_SCALAR_ABSENT-ALLOC-NEXT:   acc_ev_alloc
  // EXE-CASE_DATA_SCALAR_ABSENT-ALLOC-NEXT:   acc_ev_create
  //       EXE-CASE_DATA_SCALAR_ABSENT-NEXT: acc_ev_exit_data_start
  // EXE-CASE_DATA_SCALAR_ABSENT-ALLOC-NEXT:   acc_ev_delete
  // EXE-CASE_DATA_SCALAR_ABSENT-ALLOC-NEXT:   acc_ev_free
  //        EXE-CASE_DATA_SCALAR_ABSENT-NOT: {{.}}
  case CASE_DATA_SCALAR_ABSENT:
  {
    int x;
    #pragma acc data no_create(x)
    ;
    break;
  }

  //   PRT-LABEL: case CASE_DATA_ARRAY_PRESENT:
  //    PRT-NEXT: {
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
  //    PRT-NEXT:   break;
  //    PRT-NEXT: }
  //
  //  EXE-CASE_DATA_ARRAY_PRESENT-NOT: {{.}}
  //      EXE-CASE_DATA_ARRAY_PRESENT: acc_ev_enter_data_start
  // EXE-CASE_DATA_ARRAY_PRESENT-NEXT:   acc_ev_alloc
  // EXE-CASE_DATA_ARRAY_PRESENT-NEXT:   acc_ev_create
  // EXE-CASE_DATA_ARRAY_PRESENT-NEXT:   acc_ev_enter_data_start
  // EXE-CASE_DATA_ARRAY_PRESENT-NEXT:   acc_ev_exit_data_start
  // EXE-CASE_DATA_ARRAY_PRESENT-NEXT: acc_ev_exit_data_start
  // EXE-CASE_DATA_ARRAY_PRESENT-NEXT:   acc_ev_delete
  // EXE-CASE_DATA_ARRAY_PRESENT-NEXT:   acc_ev_free
  //  EXE-CASE_DATA_ARRAY_PRESENT-NOT: {{.}}
  case CASE_DATA_ARRAY_PRESENT:
  {
    int arr[3];
    #pragma acc data copy(arr)
    #pragma acc data no_create(arr)
    ;
    break;
  }

  //   PRT-LABEL: case CASE_DATA_ARRAY_ABSENT:
  //    PRT-NEXT: {
  //    PRT-NEXT:   int arr[3];
  //  PRT-A-NEXT:   #pragma acc data no_create(arr){{$}}
  // PRT-AO-NEXT:   // #pragma omp target data map([[NO_CREATE_MT]]: arr){{$}}
  //  PRT-O-NEXT:   #pragma omp target data map([[NO_CREATE_MT]]: arr){{$}}
  // PRT-OA-NEXT:   // #pragma acc data no_create(arr){{$}}
  //    PRT-NEXT:   ;
  //    PRT-NEXT:   break;
  //    PRT-NEXT: }
  //
  //        EXE-CASE_DATA_ARRAY_ABSENT-NOT: {{.}}
  //            EXE-CASE_DATA_ARRAY_ABSENT: acc_ev_enter_data_start
  // EXE-CASE_DATA_ARRAY_ABSENT-ALLOC-NEXT:   acc_ev_alloc
  // EXE-CASE_DATA_ARRAY_ABSENT-ALLOC-NEXT:   acc_ev_create
  //       EXE-CASE_DATA_ARRAY_ABSENT-NEXT: acc_ev_exit_data_start
  // EXE-CASE_DATA_ARRAY_ABSENT-ALLOC-NEXT:   acc_ev_delete
  // EXE-CASE_DATA_ARRAY_ABSENT-ALLOC-NEXT:   acc_ev_free
  //        EXE-CASE_DATA_ARRAY_ABSENT-NOT: {{.}}
  case CASE_DATA_ARRAY_ABSENT:
  {
    int arr[3];
    #pragma acc data no_create(arr)
    ;
    break;
  }

  //   PRT-LABEL: case CASE_DATA_SUBARRAY_PRESENT:
  //    PRT-NEXT: {
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
  //    PRT-NEXT:   break;
  //    PRT-NEXT: }
  //
  //  EXE-CASE_DATA_SUBARRAY_PRESENT-NOT: {{.}}
  //      EXE-CASE_DATA_SUBARRAY_PRESENT: acc_ev_enter_data_start
  // EXE-CASE_DATA_SUBARRAY_PRESENT-NEXT:   acc_ev_alloc
  // EXE-CASE_DATA_SUBARRAY_PRESENT-NEXT:   acc_ev_create
  // EXE-CASE_DATA_SUBARRAY_PRESENT-NEXT:   acc_ev_alloc
  // EXE-CASE_DATA_SUBARRAY_PRESENT-NEXT:   acc_ev_create
  // EXE-CASE_DATA_SUBARRAY_PRESENT-NEXT:   acc_ev_alloc
  // EXE-CASE_DATA_SUBARRAY_PRESENT-NEXT:   acc_ev_create
  // EXE-CASE_DATA_SUBARRAY_PRESENT-NEXT:   acc_ev_alloc
  // EXE-CASE_DATA_SUBARRAY_PRESENT-NEXT:   acc_ev_create
  // EXE-CASE_DATA_SUBARRAY_PRESENT-NEXT:   acc_ev_alloc
  // EXE-CASE_DATA_SUBARRAY_PRESENT-NEXT:   acc_ev_create
  // EXE-CASE_DATA_SUBARRAY_PRESENT-NEXT:   acc_ev_enter_data_start
  // EXE-CASE_DATA_SUBARRAY_PRESENT-NEXT:   acc_ev_exit_data_start
  // EXE-CASE_DATA_SUBARRAY_PRESENT-NEXT: acc_ev_exit_data_start
  // EXE-CASE_DATA_SUBARRAY_PRESENT-NEXT:   acc_ev_delete
  // EXE-CASE_DATA_SUBARRAY_PRESENT-NEXT:   acc_ev_free
  // EXE-CASE_DATA_SUBARRAY_PRESENT-NEXT:   acc_ev_delete
  // EXE-CASE_DATA_SUBARRAY_PRESENT-NEXT:   acc_ev_free
  // EXE-CASE_DATA_SUBARRAY_PRESENT-NEXT:   acc_ev_delete
  // EXE-CASE_DATA_SUBARRAY_PRESENT-NEXT:   acc_ev_free
  // EXE-CASE_DATA_SUBARRAY_PRESENT-NEXT:   acc_ev_delete
  // EXE-CASE_DATA_SUBARRAY_PRESENT-NEXT:   acc_ev_free
  // EXE-CASE_DATA_SUBARRAY_PRESENT-NEXT:   acc_ev_delete
  // EXE-CASE_DATA_SUBARRAY_PRESENT-NEXT:   acc_ev_free
  //  EXE-CASE_DATA_SUBARRAY_PRESENT-NOT: {{.}}
  case CASE_DATA_SUBARRAY_PRESENT:
  {
    int all[10], same[10], beg[10], mid[10], end[10];
    #pragma acc data copy(all,same[3:6],beg[2:5],mid[1:8],end[0:5])
    #pragma acc data no_create(all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
    ;
    break;
  }

  //   PRT-LABEL: case CASE_DATA_SUBARRAY_DISJOINT:
  //    PRT-NEXT: {
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
  //    PRT-NEXT:   break;
  //    PRT-NEXT: }
  //
  //        EXE-CASE_DATA_SUBARRAY_DISJOINT-NOT: {{.}}
  //            EXE-CASE_DATA_SUBARRAY_DISJOINT: acc_ev_enter_data_start
  //       EXE-CASE_DATA_SUBARRAY_DISJOINT-NEXT:   acc_ev_alloc
  //       EXE-CASE_DATA_SUBARRAY_DISJOINT-NEXT:   acc_ev_create
  //       EXE-CASE_DATA_SUBARRAY_DISJOINT-NEXT:   acc_ev_enter_data_start
  // EXE-CASE_DATA_SUBARRAY_DISJOINT-ALLOC-NEXT:     acc_ev_alloc
  // EXE-CASE_DATA_SUBARRAY_DISJOINT-ALLOC-NEXT:     acc_ev_create
  //       EXE-CASE_DATA_SUBARRAY_DISJOINT-NEXT:   acc_ev_exit_data_start
  // EXE-CASE_DATA_SUBARRAY_DISJOINT-ALLOC-NEXT:     acc_ev_delete
  // EXE-CASE_DATA_SUBARRAY_DISJOINT-ALLOC-NEXT:     acc_ev_free
  //       EXE-CASE_DATA_SUBARRAY_DISJOINT-NEXT: acc_ev_exit_data_start
  //       EXE-CASE_DATA_SUBARRAY_DISJOINT-NEXT:   acc_ev_delete
  //       EXE-CASE_DATA_SUBARRAY_DISJOINT-NEXT:   acc_ev_free
  //        EXE-CASE_DATA_SUBARRAY_DISJOINT-NOT: {{.}}
  case CASE_DATA_SUBARRAY_DISJOINT:
  {
    int arr[4];
    #pragma acc data copy(arr[0:2])
    #pragma acc data no_create(arr[2:2])
    ;
    break;
  }

  //   PRT-LABEL: case CASE_DATA_SUBARRAY_OVERLAP_START:
  //    PRT-NEXT: {
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
  //    PRT-NEXT:   break;
  //    PRT-NEXT: }
  //
  //  EXE-CASE_DATA_SUBARRAY_OVERLAP_START-HOST-NOT: {{.}}
  //      EXE-CASE_DATA_SUBARRAY_OVERLAP_START-HOST: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
  // EXE-CASE_DATA_SUBARRAY_OVERLAP_START-HOST-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
  //
  //           EXE-CASE_DATA_SUBARRAY_OVERLAP_START-NOT: {{.}}
  //               EXE-CASE_DATA_SUBARRAY_OVERLAP_START: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
  //          EXE-CASE_DATA_SUBARRAY_OVERLAP_START-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
  //          EXE-CASE_DATA_SUBARRAY_OVERLAP_START-NEXT: acc_ev_enter_data_start
  //          EXE-CASE_DATA_SUBARRAY_OVERLAP_START-NEXT:   acc_ev_alloc
  //          EXE-CASE_DATA_SUBARRAY_OVERLAP_START-NEXT:   acc_ev_create
  //          EXE-CASE_DATA_SUBARRAY_OVERLAP_START-NEXT:   acc_ev_enter_data_start
  //    EXE-CASE_DATA_SUBARRAY_OVERLAP_START-ALLOC-NEXT:   Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
  // EXE-CASE_DATA_SUBARRAY_OVERLAP_START-NO-ALLOC-NEXT:   acc_ev_exit_data_start
  // EXE-CASE_DATA_SUBARRAY_OVERLAP_START-NO-ALLOC-NEXT: acc_ev_exit_data_start
  // EXE-CASE_DATA_SUBARRAY_OVERLAP_START-NO-ALLOC-NEXT:   acc_ev_delete
  // EXE-CASE_DATA_SUBARRAY_OVERLAP_START-NO-ALLOC-NEXT:   acc_ev_free
  //    EXE-CASE_DATA_SUBARRAY_OVERLAP_START-ALLOC-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
  //                                                     # An abort message usually follows.
  //     EXE-CASE_DATA_SUBARRAY_OVERLAP_START-ALLOC-NOT: Libomptarget
  case CASE_DATA_SUBARRAY_OVERLAP_START:
  {
    int arr[5];
    PRINT_SUBARRAY_INFO(arr, 1, 2);
    PRINT_SUBARRAY_INFO(arr, 0, 2);
    #pragma acc data copyin(arr[1:2])
    #pragma acc data no_create(arr[0:2])
    ;
    break;
  }

  //   PRT-LABEL: case CASE_DATA_SUBARRAY_OVERLAP_END:
  //    PRT-NEXT: {
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
  //    PRT-NEXT:   break;
  //    PRT-NEXT: }
  //
  //  EXE-CASE_DATA_SUBARRAY_OVERLAP_END-HOST-NOT: {{.}}
  //      EXE-CASE_DATA_SUBARRAY_OVERLAP_END-HOST: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
  // EXE-CASE_DATA_SUBARRAY_OVERLAP_END-HOST-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
  //
  //           EXE-CASE_DATA_SUBARRAY_OVERLAP_END-NOT: {{.}}
  //               EXE-CASE_DATA_SUBARRAY_OVERLAP_END: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
  //          EXE-CASE_DATA_SUBARRAY_OVERLAP_END-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
  //          EXE-CASE_DATA_SUBARRAY_OVERLAP_END-NEXT: acc_ev_enter_data_start
  //          EXE-CASE_DATA_SUBARRAY_OVERLAP_END-NEXT:   acc_ev_alloc
  //          EXE-CASE_DATA_SUBARRAY_OVERLAP_END-NEXT:   acc_ev_create
  //          EXE-CASE_DATA_SUBARRAY_OVERLAP_END-NEXT:   acc_ev_enter_data_start
  //    EXE-CASE_DATA_SUBARRAY_OVERLAP_END-ALLOC-NEXT:   Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
  // EXE-CASE_DATA_SUBARRAY_OVERLAP_END-NO-ALLOC-NEXT:   acc_ev_exit_data_start
  // EXE-CASE_DATA_SUBARRAY_OVERLAP_END-NO-ALLOC-NEXT: acc_ev_exit_data_start
  // EXE-CASE_DATA_SUBARRAY_OVERLAP_END-NO-ALLOC-NEXT:   acc_ev_delete
  // EXE-CASE_DATA_SUBARRAY_OVERLAP_END-NO-ALLOC-NEXT:   acc_ev_free
  //    EXE-CASE_DATA_SUBARRAY_OVERLAP_END-ALLOC-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
  //                                                   # An abort message usually follows.
  //     EXE-CASE_DATA_SUBARRAY_OVERLAP_END-ALLOC-NOT: Libomptarget
  case CASE_DATA_SUBARRAY_OVERLAP_END:
  {
    int arr[5];
    PRINT_SUBARRAY_INFO(arr, 1, 2);
    PRINT_SUBARRAY_INFO(arr, 2, 2);
    #pragma acc data copyin(arr[1:2])
    #pragma acc data no_create(arr[2:2])
    ;
    break;
  }

  //   PRT-LABEL: case CASE_DATA_SUBARRAY_CONCAT2:
  //    PRT-NEXT: {
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
  //    PRT-NEXT:   break;
  //    PRT-NEXT: }
  //
  //  EXE-CASE_DATA_SUBARRAY_CONCAT2-HOST-NOT: {{.}}
  //      EXE-CASE_DATA_SUBARRAY_CONCAT2-HOST: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
  // EXE-CASE_DATA_SUBARRAY_CONCAT2-HOST-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
  //
  //           EXE-CASE_DATA_SUBARRAY_CONCAT2-NOT: {{.}}
  //               EXE-CASE_DATA_SUBARRAY_CONCAT2: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
  //          EXE-CASE_DATA_SUBARRAY_CONCAT2-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
  //          EXE-CASE_DATA_SUBARRAY_CONCAT2-NEXT: acc_ev_enter_data_start
  //          EXE-CASE_DATA_SUBARRAY_CONCAT2-NEXT:   acc_ev_alloc
  //          EXE-CASE_DATA_SUBARRAY_CONCAT2-NEXT:   acc_ev_create
  //          EXE-CASE_DATA_SUBARRAY_CONCAT2-NEXT:   acc_ev_enter_data_start
  //          EXE-CASE_DATA_SUBARRAY_CONCAT2-NEXT:     acc_ev_alloc
  //          EXE-CASE_DATA_SUBARRAY_CONCAT2-NEXT:     acc_ev_create
  //          EXE-CASE_DATA_SUBARRAY_CONCAT2-NEXT:     acc_ev_enter_data_start
  //    EXE-CASE_DATA_SUBARRAY_CONCAT2-ALLOC-NEXT:     Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
  // EXE-CASE_DATA_SUBARRAY_CONCAT2-NO-ALLOC-NEXT:     acc_ev_exit_data_start
  // EXE-CASE_DATA_SUBARRAY_CONCAT2-NO-ALLOC-NEXT:   acc_ev_exit_data_start
  // EXE-CASE_DATA_SUBARRAY_CONCAT2-NO-ALLOC-NEXT:     acc_ev_delete
  // EXE-CASE_DATA_SUBARRAY_CONCAT2-NO-ALLOC-NEXT:     acc_ev_free
  // EXE-CASE_DATA_SUBARRAY_CONCAT2-NO-ALLOC-NEXT: acc_ev_exit_data_start
  // EXE-CASE_DATA_SUBARRAY_CONCAT2-NO-ALLOC-NEXT:   acc_ev_delete
  // EXE-CASE_DATA_SUBARRAY_CONCAT2-NO-ALLOC-NEXT:   acc_ev_free
  //    EXE-CASE_DATA_SUBARRAY_CONCAT2-ALLOC-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
  //                                               # An abort message usually follows.
  //     EXE-CASE_DATA_SUBARRAY_CONCAT2-ALLOC-NOT: Libomptarget
  case CASE_DATA_SUBARRAY_CONCAT2:
  {
    int arr[4];
    PRINT_SUBARRAY_INFO(arr, 0, 2);
    PRINT_SUBARRAY_INFO(arr, 0, 4);
    #pragma acc data copyout(arr[0:2])
    #pragma acc data copy(arr[2:2])
    #pragma acc data no_create(arr[0:4])
    ;
    break;
  }

  // DMP-LABEL: CASE_PARALLEL_SCALAR_PRESENT
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
  //   PRT-LABEL: case CASE_PARALLEL_SCALAR_PRESENT:
  //    PRT-NEXT: {
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
  //    PRT-NEXT:   break;
  //    PRT-NEXT: }
  //
  //  EXE-CASE_PARALLEL_SCALAR_PRESENT-NOT: {{.}}
  //      EXE-CASE_PARALLEL_SCALAR_PRESENT: acc_ev_enter_data_start
  // EXE-CASE_PARALLEL_SCALAR_PRESENT-NEXT:   acc_ev_alloc
  // EXE-CASE_PARALLEL_SCALAR_PRESENT-NEXT:   acc_ev_create
  // EXE-CASE_PARALLEL_SCALAR_PRESENT-NEXT:   acc_ev_enter_data_start
  // EXE-CASE_PARALLEL_SCALAR_PRESENT-NEXT:   acc_ev_exit_data_start
  // EXE-CASE_PARALLEL_SCALAR_PRESENT-NEXT: acc_ev_exit_data_start
  // EXE-CASE_PARALLEL_SCALAR_PRESENT-NEXT:   acc_ev_delete
  // EXE-CASE_PARALLEL_SCALAR_PRESENT-NEXT:   acc_ev_free
  //  EXE-CASE_PARALLEL_SCALAR_PRESENT-NOT: {{.}}
  case CASE_PARALLEL_SCALAR_PRESENT:
  {
    int x;
    #pragma acc data copy(x)
    #pragma acc parallel no_create(x)
    x = 5;
    break;
  }

  //        EXE-CASE_PARALLEL_SCALAR_ABSENT-NOT: {{.}}
  //            EXE-CASE_PARALLEL_SCALAR_ABSENT: acc_ev_enter_data_start
  // EXE-CASE_PARALLEL_SCALAR_ABSENT-ALLOC-NEXT:   acc_ev_alloc
  // EXE-CASE_PARALLEL_SCALAR_ABSENT-ALLOC-NEXT:   acc_ev_create
  //       EXE-CASE_PARALLEL_SCALAR_ABSENT-NEXT: acc_ev_exit_data_start
  // EXE-CASE_PARALLEL_SCALAR_ABSENT-ALLOC-NEXT:   acc_ev_delete
  // EXE-CASE_PARALLEL_SCALAR_ABSENT-ALLOC-NEXT:   acc_ev_free
  //        EXE-CASE_PARALLEL_SCALAR_ABSENT-NOT: {{.}}
  case CASE_PARALLEL_SCALAR_ABSENT:
  {
    int x;
    int use = 0;
    #pragma acc parallel no_create(x)
    if (use) x = 5;
    break;
  }

  //  EXE-CASE_PARALLEL_ARRAY_PRESENT-NOT: {{.}}
  //      EXE-CASE_PARALLEL_ARRAY_PRESENT: acc_ev_enter_data_start
  // EXE-CASE_PARALLEL_ARRAY_PRESENT-NEXT:   acc_ev_alloc
  // EXE-CASE_PARALLEL_ARRAY_PRESENT-NEXT:   acc_ev_create
  // EXE-CASE_PARALLEL_ARRAY_PRESENT-NEXT:   acc_ev_enter_data_start
  // EXE-CASE_PARALLEL_ARRAY_PRESENT-NEXT:   acc_ev_exit_data_start
  // EXE-CASE_PARALLEL_ARRAY_PRESENT-NEXT: acc_ev_exit_data_start
  // EXE-CASE_PARALLEL_ARRAY_PRESENT-NEXT:   acc_ev_delete
  // EXE-CASE_PARALLEL_ARRAY_PRESENT-NEXT:   acc_ev_free
  //  EXE-CASE_PARALLEL_ARRAY_PRESENT-NOT: {{.}}
  case CASE_PARALLEL_ARRAY_PRESENT:
  {
    int arr[3];
    #pragma acc data copy(arr)
    #pragma acc parallel no_create(arr)
    arr[0] = 5;
    break;
  }

  //        EXE-CASE_PARALLEL_ARRAY_ABSENT-NOT: {{.}}
  //            EXE-CASE_PARALLEL_ARRAY_ABSENT: acc_ev_enter_data_start
  // EXE-CASE_PARALLEL_ARRAY_ABSENT-ALLOC-NEXT:   acc_ev_alloc
  // EXE-CASE_PARALLEL_ARRAY_ABSENT-ALLOC-NEXT:   acc_ev_create
  //       EXE-CASE_PARALLEL_ARRAY_ABSENT-NEXT: acc_ev_exit_data_start
  // EXE-CASE_PARALLEL_ARRAY_ABSENT-ALLOC-NEXT:   acc_ev_delete
  // EXE-CASE_PARALLEL_ARRAY_ABSENT-ALLOC-NEXT:   acc_ev_free
  //        EXE-CASE_PARALLEL_ARRAY_ABSENT-NOT: {{.}}
  case CASE_PARALLEL_ARRAY_ABSENT:
  {
    int arr[3];
    int use = 0;
    #pragma acc parallel no_create(arr)
    if (use) arr[0] = 5;
    break;
  }

  //  EXE-CASE_PARALLEL_SUBARRAY_PRESENT-NOT: {{.}}
  //      EXE-CASE_PARALLEL_SUBARRAY_PRESENT: acc_ev_enter_data_start
  // EXE-CASE_PARALLEL_SUBARRAY_PRESENT-NEXT:   acc_ev_alloc
  // EXE-CASE_PARALLEL_SUBARRAY_PRESENT-NEXT:   acc_ev_create
  // EXE-CASE_PARALLEL_SUBARRAY_PRESENT-NEXT:   acc_ev_alloc
  // EXE-CASE_PARALLEL_SUBARRAY_PRESENT-NEXT:   acc_ev_create
  // EXE-CASE_PARALLEL_SUBARRAY_PRESENT-NEXT:   acc_ev_alloc
  // EXE-CASE_PARALLEL_SUBARRAY_PRESENT-NEXT:   acc_ev_create
  // EXE-CASE_PARALLEL_SUBARRAY_PRESENT-NEXT:   acc_ev_alloc
  // EXE-CASE_PARALLEL_SUBARRAY_PRESENT-NEXT:   acc_ev_create
  // EXE-CASE_PARALLEL_SUBARRAY_PRESENT-NEXT:   acc_ev_alloc
  // EXE-CASE_PARALLEL_SUBARRAY_PRESENT-NEXT:   acc_ev_create
  // EXE-CASE_PARALLEL_SUBARRAY_PRESENT-NEXT:   acc_ev_enter_data_start
  // EXE-CASE_PARALLEL_SUBARRAY_PRESENT-NEXT:   acc_ev_exit_data_start
  // EXE-CASE_PARALLEL_SUBARRAY_PRESENT-NEXT: acc_ev_exit_data_start
  // EXE-CASE_PARALLEL_SUBARRAY_PRESENT-NEXT:   acc_ev_delete
  // EXE-CASE_PARALLEL_SUBARRAY_PRESENT-NEXT:   acc_ev_free
  // EXE-CASE_PARALLEL_SUBARRAY_PRESENT-NEXT:   acc_ev_delete
  // EXE-CASE_PARALLEL_SUBARRAY_PRESENT-NEXT:   acc_ev_free
  // EXE-CASE_PARALLEL_SUBARRAY_PRESENT-NEXT:   acc_ev_delete
  // EXE-CASE_PARALLEL_SUBARRAY_PRESENT-NEXT:   acc_ev_free
  // EXE-CASE_PARALLEL_SUBARRAY_PRESENT-NEXT:   acc_ev_delete
  // EXE-CASE_PARALLEL_SUBARRAY_PRESENT-NEXT:   acc_ev_free
  // EXE-CASE_PARALLEL_SUBARRAY_PRESENT-NEXT:   acc_ev_delete
  // EXE-CASE_PARALLEL_SUBARRAY_PRESENT-NEXT:   acc_ev_free
  //  EXE-CASE_PARALLEL_SUBARRAY_PRESENT-NOT: {{.}}
  case CASE_PARALLEL_SUBARRAY_PRESENT:
  {
    int all[10], same[10], beg[10], mid[10], end[10];
    #pragma acc data copy(all,same[3:6],beg[2:5],mid[1:8],end[0:5])
    #pragma acc parallel no_create(all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
    {
      all[0] = 1; same[3] = 1; beg[2] = 1; mid[3] = 1; end[4] = 1;
    }
    break;
  }

  //        EXE-CASE_PARALLEL_SUBARRAY_DISJOINT-NOT: {{.}}
  //            EXE-CASE_PARALLEL_SUBARRAY_DISJOINT: acc_ev_enter_data_start
  //       EXE-CASE_PARALLEL_SUBARRAY_DISJOINT-NEXT:   acc_ev_alloc
  //       EXE-CASE_PARALLEL_SUBARRAY_DISJOINT-NEXT:   acc_ev_create
  //       EXE-CASE_PARALLEL_SUBARRAY_DISJOINT-NEXT:   acc_ev_enter_data_start
  // EXE-CASE_PARALLEL_SUBARRAY_DISJOINT-ALLOC-NEXT:     acc_ev_alloc
  // EXE-CASE_PARALLEL_SUBARRAY_DISJOINT-ALLOC-NEXT:     acc_ev_create
  //       EXE-CASE_PARALLEL_SUBARRAY_DISJOINT-NEXT:   acc_ev_exit_data_start
  // EXE-CASE_PARALLEL_SUBARRAY_DISJOINT-ALLOC-NEXT:     acc_ev_delete
  // EXE-CASE_PARALLEL_SUBARRAY_DISJOINT-ALLOC-NEXT:     acc_ev_free
  //       EXE-CASE_PARALLEL_SUBARRAY_DISJOINT-NEXT: acc_ev_exit_data_start
  //       EXE-CASE_PARALLEL_SUBARRAY_DISJOINT-NEXT:   acc_ev_delete
  //       EXE-CASE_PARALLEL_SUBARRAY_DISJOINT-NEXT:   acc_ev_free
  //        EXE-CASE_PARALLEL_SUBARRAY_DISJOINT-NOT: {{.}}
  case CASE_PARALLEL_SUBARRAY_DISJOINT:
  {
    int arr[4];
    int use = 0;
    #pragma acc data copy(arr[0:2])
    #pragma acc parallel no_create(arr[2:2])
    if (use) arr[2] = 1;
    break;
  }

  //  EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_START-HOST-NOT: {{.}}
  //      EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_START-HOST: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
  // EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_START-HOST-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
  //
  //
  //           EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_START-NOT: {{.}}
  //               EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_START: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
  //          EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_START-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
  //               EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_START: acc_ev_enter_data_start
  //          EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_START-NEXT:   acc_ev_alloc
  //          EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_START-NEXT:   acc_ev_create
  //          EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_START-NEXT:   acc_ev_enter_data_start
  //    EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_START-ALLOC-NEXT:   Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
  // EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_START-NO-ALLOC-NEXT:   acc_ev_exit_data_start
  // EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_START-NO-ALLOC-NEXT: acc_ev_exit_data_start
  // EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_START-NO-ALLOC-NEXT:   acc_ev_delete
  // EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_START-NO-ALLOC-NEXT:   acc_ev_free
  //    EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_START-ALLOC-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
  //                                                         # An abort message usually follows.
  //     EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_START-ALLOC-NOT: Libomptarget
  case CASE_PARALLEL_SUBARRAY_OVERLAP_START:
  {
    int arr[10];
    PRINT_SUBARRAY_INFO(arr, 5, 4);
    PRINT_SUBARRAY_INFO(arr, 4, 4);
    int use = 0;
    #pragma acc data copyin(arr[5:4])
    #pragma acc parallel no_create(arr[4:4])
    if (use) arr[4] = 1;
    break;
  }

  //  EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_END-HOST-NOT: {{.}}
  //      EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_END-HOST: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
  // EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_END-HOST-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
  //
  //           EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_END-NOT: {{.}}
  //               EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_END: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
  //          EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_END-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
  //          EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_END-NEXT: acc_ev_enter_data_start
  //          EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_END-NEXT:   acc_ev_alloc
  //          EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_END-NEXT:   acc_ev_create
  //          EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_END-NEXT:   acc_ev_enter_data_start
  //    EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_END-ALLOC-NEXT:   Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
  // EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_END-NO-ALLOC-NEXT:   acc_ev_exit_data_start
  // EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_END-NO-ALLOC-NEXT: acc_ev_exit_data_start
  // EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_END-NO-ALLOC-NEXT:   acc_ev_delete
  // EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_END-NO-ALLOC-NEXT:   acc_ev_free
  //    EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_END-ALLOC-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
  //                                                       # An abort message usually follows.
  //     EXE-CASE_PARALLEL_SUBARRAY_OVERLAP_END-ALLOC-NOT: Libomptarget
  case CASE_PARALLEL_SUBARRAY_OVERLAP_END:
  {
    int arr[10];
    PRINT_SUBARRAY_INFO(arr, 3, 4);
    PRINT_SUBARRAY_INFO(arr, 4, 4);
    int use = 0;
    #pragma acc data copyin(arr[3:4])
    #pragma acc parallel no_create(arr[4:4])
    if (use) arr[4] = 1;
    break;
  }

  //  EXE-CASE_PARALLEL_SUBARRAY_CONCAT2-HOST-NOT: {{.}}
  //      EXE-CASE_PARALLEL_SUBARRAY_CONCAT2-HOST: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
  // EXE-CASE_PARALLEL_SUBARRAY_CONCAT2-HOST-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
  //
  //           EXE-CASE_PARALLEL_SUBARRAY_CONCAT2-NOT: {{.}}
  //               EXE-CASE_PARALLEL_SUBARRAY_CONCAT2: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
  //          EXE-CASE_PARALLEL_SUBARRAY_CONCAT2-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
  //          EXE-CASE_PARALLEL_SUBARRAY_CONCAT2-NEXT: acc_ev_enter_data_start
  //          EXE-CASE_PARALLEL_SUBARRAY_CONCAT2-NEXT:   acc_ev_alloc
  //          EXE-CASE_PARALLEL_SUBARRAY_CONCAT2-NEXT:   acc_ev_create
  //          EXE-CASE_PARALLEL_SUBARRAY_CONCAT2-NEXT:   acc_ev_enter_data_start
  //          EXE-CASE_PARALLEL_SUBARRAY_CONCAT2-NEXT:     acc_ev_alloc
  //          EXE-CASE_PARALLEL_SUBARRAY_CONCAT2-NEXT:     acc_ev_create
  //          EXE-CASE_PARALLEL_SUBARRAY_CONCAT2-NEXT:     acc_ev_enter_data_start
  //    EXE-CASE_PARALLEL_SUBARRAY_CONCAT2-ALLOC-NEXT:     Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
  // EXE-CASE_PARALLEL_SUBARRAY_CONCAT2-NO-ALLOC-NEXT:     acc_ev_exit_data_start
  // EXE-CASE_PARALLEL_SUBARRAY_CONCAT2-NO-ALLOC-NEXT:   acc_ev_exit_data_start
  // EXE-CASE_PARALLEL_SUBARRAY_CONCAT2-NO-ALLOC-NEXT:     acc_ev_delete
  // EXE-CASE_PARALLEL_SUBARRAY_CONCAT2-NO-ALLOC-NEXT:     acc_ev_free
  // EXE-CASE_PARALLEL_SUBARRAY_CONCAT2-NO-ALLOC-NEXT: acc_ev_exit_data_start
  // EXE-CASE_PARALLEL_SUBARRAY_CONCAT2-NO-ALLOC-NEXT:   acc_ev_delete
  // EXE-CASE_PARALLEL_SUBARRAY_CONCAT2-NO-ALLOC-NEXT:   acc_ev_free
  //    EXE-CASE_PARALLEL_SUBARRAY_CONCAT2-ALLOC-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
  //                                                   # An abort message usually follows.
  //     EXE-CASE_PARALLEL_SUBARRAY_CONCAT2-ALLOC-NOT: Libomptarget
  case CASE_PARALLEL_SUBARRAY_CONCAT2:
  {
    int arr[4];
    PRINT_SUBARRAY_INFO(arr, 0, 2);
    PRINT_SUBARRAY_INFO(arr, 0, 4);
    int use = 0;
    #pragma acc data copyout(arr[0:2])
    #pragma acc data copy(arr[2:2])
    #pragma acc parallel no_create(arr[0:4])
    if (use) arr[0] = 1;
    break;
  }

  // DMP-LABEL: CASE_PARALLEL_LOOP_SCALAR_PRESENT
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
  //   PRT-LABEL: case CASE_PARALLEL_LOOP_SCALAR_PRESENT
  //    PRT-NEXT: {
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
  //    PRT-NEXT:   break;
  //    PRT-NEXT: }
  //
  //  EXE-CASE_PARALLEL_LOOP_SCALAR_PRESENT-NOT: {{.}}
  //      EXE-CASE_PARALLEL_LOOP_SCALAR_PRESENT: acc_ev_enter_data_start
  // EXE-CASE_PARALLEL_LOOP_SCALAR_PRESENT-NEXT:   acc_ev_alloc
  // EXE-CASE_PARALLEL_LOOP_SCALAR_PRESENT-NEXT:   acc_ev_create
  // EXE-CASE_PARALLEL_LOOP_SCALAR_PRESENT-NEXT:   acc_ev_enter_data_start
  // EXE-CASE_PARALLEL_LOOP_SCALAR_PRESENT-NEXT:   acc_ev_exit_data_start
  // EXE-CASE_PARALLEL_LOOP_SCALAR_PRESENT-NEXT: acc_ev_exit_data_start
  // EXE-CASE_PARALLEL_LOOP_SCALAR_PRESENT-NEXT:   acc_ev_delete
  // EXE-CASE_PARALLEL_LOOP_SCALAR_PRESENT-NEXT:   acc_ev_free
  //  EXE-CASE_PARALLEL_LOOP_SCALAR_PRESENT-NOT: {{.}}
  case CASE_PARALLEL_LOOP_SCALAR_PRESENT:
  {
    int x;
    #pragma acc data copy(x)
    #pragma acc parallel loop no_create(x)
    for (int i = 0; i < 2; ++i)
    x = 1;
    break;
  }

  //        EXE-CASE_PARALLEL_LOOP_SCALAR_ABSENT-NOT: {{.}}
  //            EXE-CASE_PARALLEL_LOOP_SCALAR_ABSENT: acc_ev_enter_data_start
  // EXE-CASE_PARALLEL_LOOP_SCALAR_ABSENT-ALLOC-NEXT:   acc_ev_alloc
  // EXE-CASE_PARALLEL_LOOP_SCALAR_ABSENT-ALLOC-NEXT:   acc_ev_create
  //       EXE-CASE_PARALLEL_LOOP_SCALAR_ABSENT-NEXT: acc_ev_exit_data_start
  // EXE-CASE_PARALLEL_LOOP_SCALAR_ABSENT-ALLOC-NEXT:   acc_ev_delete
  // EXE-CASE_PARALLEL_LOOP_SCALAR_ABSENT-ALLOC-NEXT:   acc_ev_free
  //        EXE-CASE_PARALLEL_LOOP_SCALAR_ABSENT-NOT: {{.}}
  case CASE_PARALLEL_LOOP_SCALAR_ABSENT:
  {
    int x;
    int use = 0;
    #pragma acc parallel loop no_create(x)
    for (int i = 0; i < 2; ++i)
    if (use) x = 1;
    break;
  }

  //  EXE-CASE_CONST_PRESENT-NOT: {{.}}
  //      EXE-CASE_CONST_PRESENT: acc_ev_enter_data_start
  // EXE-CASE_CONST_PRESENT-NEXT:   acc_ev_alloc
  // EXE-CASE_CONST_PRESENT-NEXT:   acc_ev_create
  // EXE-CASE_CONST_PRESENT-NEXT:   acc_ev_enter_data_start
  // EXE-CASE_CONST_PRESENT-NEXT:   acc_ev_exit_data_start
  // EXE-CASE_CONST_PRESENT-NEXT: acc_ev_exit_data_start
  // EXE-CASE_CONST_PRESENT-NEXT:   acc_ev_delete
  // EXE-CASE_CONST_PRESENT-NEXT:   acc_ev_free
  //  EXE-CASE_CONST_PRESENT-NOT: {{.}}
  case CASE_CONST_PRESENT:
  {
    const int x;
    int y;
    #pragma acc data copy(x)
    #pragma acc parallel loop no_create(x)
    for (int i = 0; i < 2; ++i)
    y = x;
    break;
  }

  //        EXE-CASE_CONST_ABSENT-NOT: {{.}}
  //            EXE-CASE_CONST_ABSENT: acc_ev_enter_data_start
  // EXE-CASE_CONST_ABSENT-ALLOC-NEXT:   acc_ev_alloc
  // EXE-CASE_CONST_ABSENT-ALLOC-NEXT:   acc_ev_create
  //       EXE-CASE_CONST_ABSENT-NEXT: acc_ev_exit_data_start
  // EXE-CASE_CONST_ABSENT-ALLOC-NEXT:   acc_ev_delete
  // EXE-CASE_CONST_ABSENT-ALLOC-NEXT:   acc_ev_free
  //        EXE-CASE_CONST_ABSENT-NOT: {{.}}
  case CASE_CONST_ABSENT:
  {
    const int x;
    int y;
    int use = 0;
    #pragma acc parallel loop no_create(x)
    for (int i = 0; i < 2; ++i)
    if (use) y = x;
    break;
  }

  //         DMP-LABEL: CASE_INHERITED_PRESENT
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
  //            PRT-LABEL: case CASE_INHERITED_PRESENT:
  //             PRT-NEXT: {
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
  //             PRT-NEXT:   break;
  //             PRT-NEXT: }
  //
  //  EXE-CASE_INHERITED_PRESENT-NOT: {{.}}
  //      EXE-CASE_INHERITED_PRESENT: acc_ev_enter_data_start
  // EXE-CASE_INHERITED_PRESENT-NEXT:   acc_ev_alloc
  // EXE-CASE_INHERITED_PRESENT-NEXT:   acc_ev_create
  // EXE-CASE_INHERITED_PRESENT-NEXT:   acc_ev_enter_data_start
  // EXE-CASE_INHERITED_PRESENT-NEXT:     acc_ev_enter_data_start
  // EXE-CASE_INHERITED_PRESENT-NEXT:     acc_ev_exit_data_start
  // EXE-CASE_INHERITED_PRESENT-NEXT:   acc_ev_exit_data_start
  // EXE-CASE_INHERITED_PRESENT-NEXT: acc_ev_exit_data_start
  // EXE-CASE_INHERITED_PRESENT-NEXT:   acc_ev_delete
  // EXE-CASE_INHERITED_PRESENT-NEXT:   acc_ev_free
  //  EXE-CASE_INHERITED_PRESENT-NOT: {{.}}
  case CASE_INHERITED_PRESENT:
  {
    int x;
    #pragma acc data create(x)
    #pragma acc data no_create(x)
    #pragma acc parallel
    x = 1;
    break;
  }

  //         DMP-LABEL: CASE_INHERITED_ABSENT
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
  //            PRT-LABEL: case CASE_INHERITED_ABSENT:
  //             PRT-NEXT: {
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
  //             PRT-NEXT:   break;
  //             PRT-NEXT: }
  //
  //        EXE-CASE_INHERITED_ABSENT-NOT: {{.}}
  //            EXE-CASE_INHERITED_ABSENT: acc_ev_enter_data_start
  // EXE-CASE_INHERITED_ABSENT-ALLOC-NEXT:   acc_ev_alloc
  // EXE-CASE_INHERITED_ABSENT-ALLOC-NEXT:   acc_ev_create
  //       EXE-CASE_INHERITED_ABSENT-NEXT:   acc_ev_enter_data_start
  //       EXE-CASE_INHERITED_ABSENT-NEXT:   acc_ev_exit_data_start
  //       EXE-CASE_INHERITED_ABSENT-NEXT: acc_ev_exit_data_start
  // EXE-CASE_INHERITED_ABSENT-ALLOC-NEXT:   acc_ev_delete
  // EXE-CASE_INHERITED_ABSENT-ALLOC-NEXT:   acc_ev_free
  //        EXE-CASE_INHERITED_ABSENT-NOT: {{.}}
  case CASE_INHERITED_ABSENT:
  {
    int x;
    int use = 0;
    #pragma acc data no_create(x)
    #pragma acc parallel
    if (use)
      x = 1;
    break;
  }

  //            PRT-LABEL: case CASE_INHERITED_SUBARRAY_PRESENT:
  //             PRT-NEXT: {
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
  //             PRT-NEXT:   break;
  //             PRT-NEXT: }
  //
  //  EXE-CASE_INHERITED_SUBARRAY_PRESENT-NOT: {{.}}
  //      EXE-CASE_INHERITED_SUBARRAY_PRESENT: acc_ev_enter_data_start
  // EXE-CASE_INHERITED_SUBARRAY_PRESENT-NEXT:   acc_ev_alloc
  // EXE-CASE_INHERITED_SUBARRAY_PRESENT-NEXT:   acc_ev_create
  // EXE-CASE_INHERITED_SUBARRAY_PRESENT-NEXT:   acc_ev_enter_data_start
  // EXE-CASE_INHERITED_SUBARRAY_PRESENT-NEXT:     acc_ev_enter_data_start
  // EXE-CASE_INHERITED_SUBARRAY_PRESENT-NEXT:     acc_ev_exit_data_start
  // EXE-CASE_INHERITED_SUBARRAY_PRESENT-NEXT:   acc_ev_exit_data_start
  // EXE-CASE_INHERITED_SUBARRAY_PRESENT-NEXT: acc_ev_exit_data_start
  // EXE-CASE_INHERITED_SUBARRAY_PRESENT-NEXT:   acc_ev_delete
  // EXE-CASE_INHERITED_SUBARRAY_PRESENT-NEXT:   acc_ev_free
  //  EXE-CASE_INHERITED_SUBARRAY_PRESENT-NOT: {{.}}
  case CASE_INHERITED_SUBARRAY_PRESENT:
  {
    int arr[] = {10, 20, 30, 40, 50};
    #pragma acc data create(arr)
    #pragma acc data no_create(arr[1:2])
    #pragma acc parallel
    arr[1] = 1;
    break;
  }

  //            PRT-LABEL: case CASE_INHERITED_SUBARRAY_ABSENT:
  //             PRT-NEXT: {
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
  //             PRT-NEXT:   break;
  //             PRT-NEXT: }
  //
  //        EXE-CASE_INHERITED_SUBARRAY_ABSENT-NOT: {{.}}
  //            EXE-CASE_INHERITED_SUBARRAY_ABSENT: acc_ev_enter_data_start
  // EXE-CASE_INHERITED_SUBARRAY_ABSENT-ALLOC-NEXT:   acc_ev_alloc
  // EXE-CASE_INHERITED_SUBARRAY_ABSENT-ALLOC-NEXT:   acc_ev_create
  //       EXE-CASE_INHERITED_SUBARRAY_ABSENT-NEXT:   acc_ev_enter_data_start
  //       EXE-CASE_INHERITED_SUBARRAY_ABSENT-NEXT:   acc_ev_exit_data_start
  //       EXE-CASE_INHERITED_SUBARRAY_ABSENT-NEXT: acc_ev_exit_data_start
  // EXE-CASE_INHERITED_SUBARRAY_ABSENT-ALLOC-NEXT:   acc_ev_delete
  // EXE-CASE_INHERITED_SUBARRAY_ABSENT-ALLOC-NEXT:   acc_ev_free
  //        EXE-CASE_INHERITED_SUBARRAY_ABSENT-NOT: {{.}}
  case CASE_INHERITED_SUBARRAY_ABSENT:
  {
    int arr[] = {10, 20, 30, 40, 50};
    int use = 0;
    #pragma acc data no_create(arr[1:2])
    #pragma acc parallel
    if (use)
      arr[1] = 1;
    break;
  }

  case CASE_END:
    fprintf(stderr, "unexpected CASE_END\n");
    break;
  }

  // EXE-HOST-NOT: {{.}}

  return 0;
}
