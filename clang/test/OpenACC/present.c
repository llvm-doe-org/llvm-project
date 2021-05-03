// Check present clauses on different constructs and with different values of
// -fopenacc-present-omp.  Diagnostics about present in the translation are
// tested in warn-acc-omp-map-present.c.
//
// The various cases covered here should be kept consistent with no-create.c
// and update.c.  For example, a subarray that extends a subarray already
// present is consistently considered not present, so the present clause
// produces a runtime error and the no_create clause doesn't allocate.

// Check bad -fopenacc-present-omp values.
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
// RUN:     not %[cmd] -fopenacc-present-omp=%[val] %s 2>&1 \
// RUN:     | FileCheck -check-prefix=BAD-VAL -DVAL=%[val] %s
// RUN:   }
// RUN: }
//
// BAD-VAL: error: invalid value '[[VAL]]' in '-fopenacc-present-omp=[[VAL]]'

// Define some interrelated data we use several times below.
//
// RUN: %data present-opts {
// RUN:   (present-opt=-Wno-openacc-omp-map-present                                 present-mt=present,hold,alloc not-if-present=not not-crash-if-present='not --crash')
// RUN:   (present-opt='-fopenacc-present-omp=present -Wno-openacc-omp-map-present' present-mt=present,hold,alloc not-if-present=not not-crash-if-present='not --crash')
// RUN:   (present-opt=-fopenacc-present-omp=no-present                             present-mt=hold,alloc         not-if-present=    not-crash-if-present=             )
// RUN: }
// RUN: %data tgts {
// RUN:   (run-if=                tgt-cflags=                                     not-if-off-and-present=                  not-crash-if-off-and-present=                        not-if-off=    not-crash-if-off=             )
// RUN:   (run-if=%run-if-x86_64  tgt-cflags=-fopenmp-targets=%run-x86_64-triple  not-if-off-and-present=%[not-if-present] not-crash-if-off-and-present=%[not-crash-if-present] not-if-off=not not-crash-if-off='not --crash')
// RUN:   (run-if=%run-if-ppc64le tgt-cflags=-fopenmp-targets=%run-ppc64le-triple not-if-off-and-present=%[not-if-present] not-crash-if-off-and-present=%[not-crash-if-present] not-if-off=not not-crash-if-off='not --crash')
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags=-fopenmp-targets=%run-nvptx64-triple not-if-off-and-present=%[not-if-present] not-crash-if-off-and-present=%[not-crash-if-present] not-if-off=not not-crash-if-off='not --crash')
// RUN: }
// RUN: %data use-vars {
// RUN:   (use-var-cflags=            )
// RUN:   (use-var-cflags=-DDO_USE_VAR)
// RUN: }
//      # Due to a bug in Clang's OpenMP implementation, codegen and runtime
//      # behavior used to be differently for "present" clauses on "acc data"
//      # vs. "acc parallel" if -fopenacc-present-omp=no-present were specified,
//      # so check all interesting cases for each.  Specifically, without the
//      # "present" modifier, a map type for an unused variable within "omp
//      # target teams" was dropped by Clang, so there was no runtime error for
//      # collisions with prior mappings.  However, in the case of "omp target
//      # data", a variable in a map clause was always mapped, so such runtime
//      # errors did occur.  Both now behave like the latter.
//      #
//      # "acc parallel loop" should be about the same as "acc parallel", so a
//      # few cases are probably sufficient.
// RUN: %data cases {
// RUN:   (case=caseDataScalarPresent            not-if-fail=                          not-crash-if-fail=                                not-if-presentError=                          not-if-arrayExtError=              construct=data    )
// RUN:   (case=caseDataScalarAbsent             not-if-fail=%[not-if-off-and-present] not-crash-if-fail=%[not-crash-if-off-and-present] not-if-presentError=%[not-if-off-and-present] not-if-arrayExtError=              construct=data    )
// RUN:   (case=caseDataArrayPresent             not-if-fail=                          not-crash-if-fail=                                not-if-presentError=                          not-if-arrayExtError=              construct=data    )
// RUN:   (case=caseDataArrayAbsent              not-if-fail=%[not-if-off-and-present] not-crash-if-fail=%[not-crash-if-off-and-present] not-if-presentError=%[not-if-off-and-present] not-if-arrayExtError=              construct=data    )
// RUN:   (case=caseDataSubarrayPresent          not-if-fail=                          not-crash-if-fail=                                not-if-presentError=                          not-if-arrayExtError=              construct=data    )
// RUN:   (case=caseDataSubarrayDisjoint         not-if-fail=%[not-if-off-and-present] not-crash-if-fail=%[not-crash-if-off-and-present] not-if-presentError=%[not-if-off-and-present] not-if-arrayExtError=              construct=data    )
// RUN:   (case=caseDataSubarrayOverlapStart     not-if-fail=%[not-if-off]             not-crash-if-fail=%[not-crash-if-off]             not-if-presentError=%[not-if-off-and-present] not-if-arrayExtError=%[not-if-off] construct=data    )
// RUN:   (case=caseDataSubarrayOverlapEnd       not-if-fail=%[not-if-off]             not-crash-if-fail=%[not-crash-if-off]             not-if-presentError=%[not-if-off-and-present] not-if-arrayExtError=%[not-if-off] construct=data    )
// RUN:   (case=caseDataSubarrayConcat2          not-if-fail=%[not-if-off]             not-crash-if-fail=%[not-crash-if-off]             not-if-presentError=%[not-if-off-and-present] not-if-arrayExtError=%[not-if-off] construct=data    )
// RUN:   (case=caseParallelScalarPresent        not-if-fail=                          not-crash-if-fail=                                not-if-presentError=                          not-if-arrayExtError=              construct=parallel)
// RUN:   (case=caseParallelScalarAbsent         not-if-fail=%[not-if-off-and-present] not-crash-if-fail=%[not-crash-if-off-and-present] not-if-presentError=%[not-if-off-and-present] not-if-arrayExtError=              construct=parallel)
// RUN:   (case=caseParallelArrayPresent         not-if-fail=                          not-crash-if-fail=                                not-if-presentError=                          not-if-arrayExtError=              construct=parallel)
// RUN:   (case=caseParallelArrayAbsent          not-if-fail=%[not-if-off-and-present] not-crash-if-fail=%[not-crash-if-off-and-present] not-if-presentError=%[not-if-off-and-present] not-if-arrayExtError=              construct=parallel)
// RUN:   (case=caseParallelSubarrayPresent      not-if-fail=                          not-crash-if-fail=                                not-if-presentError=                          not-if-arrayExtError=              construct=parallel)
// RUN:   (case=caseParallelSubarrayDisjoint     not-if-fail=%[not-if-off-and-present] not-crash-if-fail=%[not-crash-if-off-and-present] not-if-presentError=%[not-if-off-and-present] not-if-arrayExtError=              construct=parallel)
// RUN:   (case=caseParallelSubarrayOverlapStart not-if-fail=%[not-if-off]             not-crash-if-fail=%[not-crash-if-off]             not-if-presentError=%[not-if-off-and-present] not-if-arrayExtError=%[not-if-off] construct=parallel)
// RUN:   (case=caseParallelSubarrayOverlapEnd   not-if-fail=%[not-if-off]             not-crash-if-fail=%[not-crash-if-off]             not-if-presentError=%[not-if-off-and-present] not-if-arrayExtError=%[not-if-off] construct=parallel)
// RUN:   (case=caseParallelSubarrayConcat2      not-if-fail=%[not-if-off]             not-crash-if-fail=%[not-crash-if-off]             not-if-presentError=%[not-if-off-and-present] not-if-arrayExtError=%[not-if-off] construct=parallel)
// RUN:   (case=caseParallelLoopScalarPresent    not-if-fail=                          not-crash-if-fail=                                not-if-presentError=                          not-if-arrayExtError=              construct=parallel)
// RUN:   (case=caseParallelLoopScalarAbsent     not-if-fail=%[not-if-off-and-present] not-crash-if-fail=%[not-crash-if-off-and-present] not-if-presentError=%[not-if-off-and-present] not-if-arrayExtError=              construct=parallel)
// RUN:   (case=caseConstPresent                 not-if-fail=                          not-crash-if-fail=                                not-if-presentError=                          not-if-arrayExtError=              construct=parallel)
// RUN:   (case=caseConstAbsent                  not-if-fail=%[not-if-off-and-present] not-crash-if-fail=%[not-crash-if-off-and-present] not-if-presentError=%[not-if-off-and-present] not-if-arrayExtError=              construct=parallel)
// RUN: }
// RUN: echo '#define FOREACH_CASE(Macro) \' > %t-cases.h
// RUN: %for cases {
// RUN:   echo '  Macro(%[case]) \' >> %t-cases.h
// RUN: }
// RUN: echo '  /*end of FOREACH_CASE*/' >> %t-cases.h

// Check -ast-dump before and after AST serialization.
//
// We include dump checking on only a few representative cases, which should be
// more than sufficient to show it's working for the present clause.
//
// RUN: %for present-opts {
// RUN:   %clang -Xclang -verify -Xclang -ast-dump -fsyntax-only -fopenacc %s \
// RUN:          %[present-opt] -DCASES_HEADER='"%t-cases.h"' \
// RUN:   | FileCheck -check-prefixes=DMP %s
// RUN:   %clang -Xclang -verify -fopenacc -emit-ast -o %t.ast %s \
// RUN:          %[present-opt] -DCASES_HEADER='"%t-cases.h"'
// RUN:   %clang_cc1 -ast-dump-all %t.ast \
// RUN:   | FileCheck -check-prefixes=DMP %s
// RUN: }

// Check -ast-print and -fopenacc[-ast]-print.
//
// We include print checking on only a few representative cases, which should be
// more than sufficient to show it's working for the present clause.
//
// RUN: %clang -Xclang -verify -Xclang -ast-print -fsyntax-only \
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
// RUN:   (prt='-Xclang -ast-print -fsyntax-only -fopenacc' prt-chk=PRT,PRT-A       )
// RUN:   (prt=-fopenacc-ast-print=acc                      prt-chk=PRT,PRT-A       )
// RUN:   (prt=-fopenacc-ast-print=omp                      prt-chk=PRT,PRT-O       )
// RUN:   (prt=-fopenacc-ast-print=acc-omp                  prt-chk=PRT,PRT-A,PRT-AO)
// RUN:   (prt=-fopenacc-ast-print=omp-acc                  prt-chk=PRT,PRT-O,PRT-OA)
// RUN:   (prt=-fopenacc-print=acc                          prt-chk=PRT,PRT-A       )
// RUN:   (prt=-fopenacc-print=omp                          prt-chk=PRT,PRT-O       )
// RUN:   (prt=-fopenacc-print=acc-omp                      prt-chk=PRT,PRT-A,PRT-AO)
// RUN:   (prt=-fopenacc-print=omp-acc                      prt-chk=PRT,PRT-O,PRT-OA)
// RUN: }
// RUN: %for present-opts {
// RUN:   %for prt-args {
// RUN:     %clang -Xclang -verify %[prt] %[present-opt] %t-acc.c \
// RUN:            -DCASES_HEADER='"%t-cases.h"' -Wno-openacc-omp-map-hold \
// RUN:     | FileCheck -check-prefixes=%[prt-chk] -DPRESENT_MT=%[present-mt] %s
// RUN:   }
// RUN: }

// Check -ast-print after AST serialization.
//
// Some data related to printing (where to print comments about discarded
// directives) is serialized and deserialized, so it's worthwhile to try all
// OpenACC printing modes.
//
// RUN: %for present-opts {
// RUN:   %clang -Xclang -verify -fopenacc %[present-opt] -emit-ast -o %t.ast \
// RUN:          -DCASES_HEADER='"%t-cases.h"' %t-acc.c
// RUN:   %for prt-args {
// RUN:     %clang %[prt] %t.ast 2>&1 \
// RUN:     | FileCheck -check-prefixes=%[prt-chk] -DPRESENT_MT=%[present-mt] %s
// RUN:   }
// RUN: }

// Can we print the OpenMP source code, compile, and run it successfully?
//
// We don't normally bother to check this for offloading, but the present clause
// has no effect when not offloading (that is, for shared memory), and one of
// the main issues with the present clause is the various ways it can be
// translated so it can be used in source-to-source when targeting other
// compilers.  That is, we want to be sure source-to-source mode produces
// working translations of the present clause in all cases.
//
// RUN: %for present-opts {
// RUN:   %for tgts {
// RUN:     %for use-vars {
// RUN:       %for prt-opts {
// RUN:         %[run-if] %clang -Xclang -verify %[prt-opt]=omp %[present-opt] \
// RUN:                   %[use-var-cflags] %s > %t-omp.c \
// RUN:                   -DCASES_HEADER='"%t-cases.h"' \
// RUN:                   -Wno-openacc-omp-map-hold
// RUN:         %[run-if] echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN:         %[run-if] %clang -Xclang -verify -fopenmp %fopenmp-version \
// RUN:                   %[tgt-cflags] %[use-var-cflags] -o %t.exe %t-omp.c \
// RUN:                   -DCASES_HEADER='"%t-cases.h"'
// RUN:         %for cases {
// RUN:           %[run-if] %[not-crash-if-fail] %t.exe %[case] \
// RUN:                     > %t.out 2> %t.err
// RUN:           %[run-if] FileCheck -input-file %t.err -allow-empty %s \
// RUN:             -check-prefixes=EXE-ERR,EXE-ERR-%[not-if-fail]PASS \
// RUN:             -check-prefixes=EXE-ERR-%[not-if-fail]PASS-%[construct] \
// RUN:             -check-prefixes=EXE-ERR-%[not-if-presentError]PRESENT \
// RUN:             -check-prefixes=EXE-ERR-%[not-if-arrayExtError]ARRAYEXT \
// RUN:             -check-prefixes=EXE-ERR-%[not-if-presentError]PRESENT-%[not-if-arrayExtError]ARRAYEXT
// RUN:           %[run-if] FileCheck -input-file %t.out -allow-empty %s \
// RUN:             -check-prefixes=EXE-OUT,EXE-OUT-%[not-if-fail]PASS
// RUN:         }
// RUN:       }
// RUN:     }
// RUN:   }
// RUN: }

// Check execution with normal compilation.
//
// RUN: %for present-opts {
// RUN:   %for tgts {
// RUN:     %for use-vars {
// RUN:       %[run-if] %clang -Xclang -verify -fopenacc %[present-opt] \
// RUN:                 %[tgt-cflags] %[use-var-cflags] -o %t.exe %s \
// RUN:                 -DCASES_HEADER='"%t-cases.h"'
// RUN:       %for cases {
// RUN:         %[run-if] %[not-crash-if-fail] %t.exe %[case] > %t.out 2> %t.err
// RUN:         %[run-if] FileCheck -input-file %t.err -allow-empty %s \
// RUN:           -check-prefixes=EXE-ERR,EXE-ERR-%[not-if-fail]PASS \
// RUN:           -check-prefixes=EXE-ERR-%[not-if-fail]PASS-%[construct] \
// RUN:           -check-prefixes=EXE-ERR-%[not-if-presentError]PRESENT \
// RUN:           -check-prefixes=EXE-ERR-%[not-if-arrayExtError]ARRAYEXT \
// RUN:           -check-prefixes=EXE-ERR-%[not-if-presentError]PRESENT-%[not-if-arrayExtError]ARRAYEXT
// RUN:         %[run-if] FileCheck -input-file %t.out -allow-empty %s \
// RUN:           -check-prefixes=EXE-OUT,EXE-OUT-%[not-if-fail]PASS
// RUN:       }
// RUN:     }
// RUN:   }
// RUN: }

// END.

// expected-no-diagnostics

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

//                      EXE-ERR-NOT: {{.}}
//                          EXE-ERR: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//                     EXE-ERR-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//         EXE-ERR-notARRAYEXT-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//          EXE-ERR-notPRESENT-NEXT: Libomptarget message: device mapping required by 'present' map type modifier does not exist for host address 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes)
//                                   # FIXME: Names like getOrAllocTgtPtr are meaningless to users.
//          EXE-ERR-notPRESENT-NEXT: Libomptarget error: Call to getOrAllocTgtPtr returned null pointer ('present' map type modifier).
// EXE-ERR-PRESENT-notARRAYEXT-NEXT: Libomptarget error: Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping)
//    EXE-ERR-notPASS-parallel-NEXT: Libomptarget error: Call to targetDataBegin failed, abort target.
//    EXE-ERR-notPASS-parallel-NEXT: Libomptarget error: Failed to process data before launching the kernel.
//             EXE-ERR-notPASS-NEXT: Libomptarget error: run with env LIBOMPTARGET_INFO>1 to dump host-target pointer maps
//             EXE-ERR-notPASS-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                   # An abort message usually follows.
//              EXE-ERR-notPASS-NOT: Libomptarget
//                 EXE-ERR-PASS-NOT: {{.}}

//       EXE-OUT-NOT: {{.}}
//           EXE-OUT: start
// EXE-OUT-PASS-NEXT: end
//       EXE-OUT-NOT: {{.}}

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
// PRT-AO-NEXT:   // #pragma omp target data map(hold,tofrom: x){{$}}
//  PRT-A-NEXT:   #pragma acc data present(x){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[PRESENT_MT]]: x){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(hold,tofrom: x){{$}}
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
// PRT-AO-NEXT:   // #pragma omp target data map(hold,tofrom: arr){{$}}
//  PRT-A-NEXT:   #pragma acc data present(arr){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[PRESENT_MT]]: arr){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(hold,tofrom: arr){{$}}
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
// PRT-AO-NEXT:   // #pragma omp target data map(hold,tofrom: all,same[3:6],beg[2:5],mid[1:8],end[0:5])
//  PRT-A-NEXT:   #pragma acc data present(all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
// PRT-AO-NEXT:   // #pragma omp target data map([[PRESENT_MT]]: all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
//
//  PRT-O-NEXT:   #pragma omp target data map(hold,tofrom: all,same[3:6],beg[2:5],mid[1:8],end[0:5])
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
// PRT-AO-NEXT:   // #pragma omp target data map(hold,tofrom: arr[0:2]){{$}}
//  PRT-A-NEXT:   #pragma acc data present(arr[2:2]){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[PRESENT_MT]]: arr[2:2]){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(hold,tofrom: arr[0:2]){{$}}
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
// PRT-AO-NEXT:   // #pragma omp target data map(hold,to: arr[1:2]){{$}}
//  PRT-A-NEXT:   #pragma acc data present(arr[0:2]){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[PRESENT_MT]]: arr[0:2]){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(hold,to: arr[1:2]){{$}}
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
// PRT-AO-NEXT:   // #pragma omp target data map(hold,to: arr[1:2]){{$}}
//  PRT-A-NEXT:   #pragma acc data present(arr[2:2]){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[PRESENT_MT]]: arr[2:2]){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(hold,to: arr[1:2]){{$}}
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
// PRT-AO-NEXT:   // #pragma omp target data map(hold,from: arr[0:2]){{$}}
//  PRT-A-NEXT:   #pragma acc data copy(arr[2:2]){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map(hold,tofrom: arr[2:2]){{$}}
//  PRT-A-NEXT:   #pragma acc data present(arr[0:4]){{$}}
// PRT-AO-NEXT:   // #pragma omp target data map([[PRESENT_MT]]: arr[0:4]){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(hold,from: arr[0:2]){{$}}
// PRT-OA-NEXT:   // #pragma acc data copyout(arr[0:2]){{$}}
//  PRT-O-NEXT:   #pragma omp target data map(hold,tofrom: arr[2:2]){{$}}
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
// PRT-AO-NEXT:   // #pragma omp target data map(hold,tofrom: x){{$}}
//  PRT-A-NEXT:   #pragma acc parallel present(x){{$}}
// PRT-AO-NEXT:   // #pragma omp target teams map([[PRESENT_MT]]: x){{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(hold,tofrom: x){{$}}
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
// PRT-AO-NEXT:   // #pragma omp target data map(hold,tofrom: x){{$}}
//  PRT-A-NEXT:   #pragma acc parallel loop present(x){{$}}
// PRT-AO-NEXT:   // #pragma omp target teams map([[PRESENT_MT]]: x){{$}}
// PRT-AO-NEXT:   // #pragma omp distribute{{$}}
//
//  PRT-O-NEXT:   #pragma omp target data map(hold,tofrom: x){{$}}
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
