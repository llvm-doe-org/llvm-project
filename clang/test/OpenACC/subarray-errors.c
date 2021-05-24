// Check various cases of subarray extension errors.
//
// The various cases covered here should be kept consistent with present.c,
// no-create.c, and update.c.  The present and no_create clauses are tested
// separately because they have special behavior.  This test file is meant to
// cover copy, copyin, copyout, and create, which we just cycle through across
// the various subarray extension cases.

// Define some interrelated data we use several times below.
//
// RUN: %data tgts {
// RUN:   (run-if=                tgt-cflags=                                     host-or-off=HOST not-crash-if-off=             )
// RUN:   (run-if=%run-if-x86_64  tgt-cflags=-fopenmp-targets=%run-x86_64-triple  host-or-off=OFF  not-crash-if-off='not --crash')
// RUN:   (run-if=%run-if-ppc64le tgt-cflags=-fopenmp-targets=%run-ppc64le-triple host-or-off=OFF  not-crash-if-off='not --crash')
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags=-fopenmp-targets=%run-nvptx64-triple host-or-off=OFF  not-crash-if-off='not --crash')
// RUN: }
//      # "acc parallel loop" should be about the same as "acc parallel", so a
//      # few cases are probably sufficient for it.
// RUN: %data cases {
// RUN:   (case=caseDataSubarrayPresent          not-crash-if-fail=                   )
// RUN:   (case=caseDataSubarrayDisjoint         not-crash-if-fail=                   )
// RUN:   (case=caseDataSubarrayOverlapStart     not-crash-if-fail=%[not-crash-if-off])
// RUN:   (case=caseDataSubarrayOverlapEnd       not-crash-if-fail=%[not-crash-if-off])
// RUN:   (case=caseDataSubarrayConcat2          not-crash-if-fail=%[not-crash-if-off])
// RUN:   (case=caseDataSubarrayNonSubarray      not-crash-if-fail=%[not-crash-if-off])
// RUN:   (case=caseParallelSubarrayPresent      not-crash-if-fail=                   )
// RUN:   (case=caseParallelSubarrayDisjoint     not-crash-if-fail=                   )
// RUN:   (case=caseParallelSubarrayOverlapStart not-crash-if-fail=%[not-crash-if-off])
// RUN:   (case=caseParallelSubarrayOverlapEnd   not-crash-if-fail=%[not-crash-if-off])
// RUN:   (case=caseParallelSubarrayConcat2      not-crash-if-fail=%[not-crash-if-off])
// RUN:   (case=caseParallelSubarrayNonSubarray  not-crash-if-fail=%[not-crash-if-off])
// RUN: }
// RUN: echo '#define FOREACH_CASE(Macro) \' > %t-cases.h
// RUN: %for cases {
// RUN:   echo '  Macro(%[case]) \' >> %t-cases.h
// RUN: }
// RUN: echo '  /*end of FOREACH_CASE*/' >> %t-cases.h

// Check execution with normal compilation.
//
// RUN: %for tgts {
// RUN:   %[run-if] %clang -Xclang -verify -fopenacc \
// RUN:             %[tgt-cflags] %acc-includes \
// RUN:             -DCASES_HEADER='"%t-cases.h"' -o %t.exe %s
// RUN:   %for cases {
// RUN:     %[run-if] %[not-crash-if-fail] %t.exe %[case] > %t.out 2>&1
// RUN:     %[run-if] FileCheck -input-file %t.out %s \
// RUN:       -match-full-lines -allow-empty \
// RUN:       -check-prefixes=EXE,EXE-%[case] \
// RUN:       -check-prefixes=EXE-%[host-or-off],EXE-%[case]-%[host-or-off]
// RUN:   }
// RUN: }

// END.

// expected-no-diagnostics

#include <stdio.h>
#include <string.h>

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

// EXE-caseDataSubarrayPresent-NOT: {{.}}
CASE(caseDataSubarrayPresent) {
  int all[10], same[10], beg[10], mid[10], end[10];
  #pragma acc data copy(all,same[3:6],beg[2:5],mid[1:8],end[0:5])
  #pragma acc data copy(all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
  ;
}

// EXE-caseDataSubarrayDisjoint-NOT: {{.}}
CASE(caseDataSubarrayDisjoint) {
  int arr[4];
  #pragma acc data copy(arr[0:2])
  #pragma acc data copyin(arr[2:2])
  ;
}

//      EXE-caseDataSubarrayOverlapStart-NOT: {{.}}
//          EXE-caseDataSubarrayOverlapStart: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//     EXE-caseDataSubarrayOverlapStart-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
// EXE-caseDataSubarrayOverlapStart-OFF-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
// EXE-caseDataSubarrayOverlapStart-OFF-NEXT: Libomptarget error: Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping).
// EXE-caseDataSubarrayOverlapStart-OFF-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
// EXE-caseDataSubarrayOverlapStart-OFF-NEXT: {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                            # An abort message usually follows.
//  EXE-caseDataSubarrayOverlapStart-OFF-NOT: Libomptarget
CASE(caseDataSubarrayOverlapStart) {
  int arr[5];
  PRINT_SUBARRAY_INFO(arr, 1, 2);
  PRINT_SUBARRAY_INFO(arr, 0, 2);
  #pragma acc data copyin(arr[1:2])
  #pragma acc data copyout(arr[0:2])
  ;
}

//       EXE-caseDataSubarrayOverlapEnd-NOT: {{.}}
//           EXE-caseDataSubarrayOverlapEnd: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//      EXE-caseDataSubarrayOverlapEnd-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
//  EXE-caseDataSubarrayOverlapEnd-OFF-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//  EXE-caseDataSubarrayOverlapEnd-OFF-NEXT: Libomptarget error: Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping).
//  EXE-caseDataSubarrayOverlapEnd-OFF-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
//  EXE-caseDataSubarrayOverlapEnd-OFF-NEXT: {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                           # An abort message usually follows.
//  EXE-caseDataSubarrayOverlapEnd-OFF-NOT: Libomptarget
CASE(caseDataSubarrayOverlapEnd) {
  int arr[5];
  PRINT_SUBARRAY_INFO(arr, 1, 2);
  PRINT_SUBARRAY_INFO(arr, 2, 2);
  #pragma acc data copyin(arr[1:2])
  #pragma acc data copy(arr[2:2])
  ;
}

//      EXE-caseDataSubarrayConcat2-NOT: {{.}}
//          EXE-caseDataSubarrayConcat2: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//     EXE-caseDataSubarrayConcat2-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
// EXE-caseDataSubarrayConcat2-OFF-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
// EXE-caseDataSubarrayConcat2-OFF-NEXT: Libomptarget error: Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping).
// EXE-caseDataSubarrayConcat2-OFF-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
// EXE-caseDataSubarrayConcat2-OFF-NEXT: {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                       # An abort message usually follows.
//  EXE-caseDataSubarrayConcat2-OFF-NOT: Libomptarget
CASE(caseDataSubarrayConcat2) {
  int arr[4];
  PRINT_SUBARRAY_INFO(arr, 0, 2);
  PRINT_SUBARRAY_INFO(arr, 0, 4);
  #pragma acc data copyout(arr[0:2])
  #pragma acc data copy(arr[2:2])
  #pragma acc data copy(arr[0:4])
  ;
}

//      EXE-caseDataSubarrayNonSubarray-NOT: {{.}}
//          EXE-caseDataSubarrayNonSubarray: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//     EXE-caseDataSubarrayNonSubarray-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
// EXE-caseDataSubarrayNonSubarray-OFF-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
// EXE-caseDataSubarrayNonSubarray-OFF-NEXT: Libomptarget error: Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping).
// EXE-caseDataSubarrayNonSubarray-OFF-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
// EXE-caseDataSubarrayNonSubarray-OFF-NEXT: {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                           # An abort message usually follows.
//  EXE-caseDataSubarrayNonSubarray-OFF-NOT: Libomptarget
CASE(caseDataSubarrayNonSubarray) {
  int arr[5];
  PRINT_SUBARRAY_INFO(arr, 1, 2);
  PRINT_SUBARRAY_INFO(arr, 0, 5);
  #pragma acc data copyin(arr[1:2])
  #pragma acc data copyin(arr)
  ;
}

// EXE-caseParallelSubarrayPresent-NOT: {{.}}
CASE(caseParallelSubarrayPresent) {
  int all[10], same[10], beg[10], mid[10], end[10];
  #pragma acc data copy(all,same[3:6],beg[2:5],mid[1:8],end[0:5])
  #pragma acc parallel copyout(all[0:10],same[3:6],beg[2:2],mid[3:3],end[4:1])
  {
    all[0] = 1; same[3] = 1; beg[2] = 1; mid[3] = 1; end[4] = 1;
  }
}

// EXE-caseParallelSubarrayDisjoint-NOT: {{.}}
CASE(caseParallelSubarrayDisjoint) {
  int arr[4];
  int use = 0;
  #pragma acc data copy(arr[0:2])
  #pragma acc parallel copyin(arr[2:2])
  if (use) arr[2] = 1;
}

//      EXE-caseParallelSubarrayOverlapStart-NOT: {{.}}
//          EXE-caseParallelSubarrayOverlapStart: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//     EXE-caseParallelSubarrayOverlapStart-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
// EXE-caseParallelSubarrayOverlapStart-OFF-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//                                                # FIXME: Names like getOrAllocTgtPtr are meaningless to users.
// EXE-caseParallelSubarrayOverlapStart-OFF-NEXT: Libomptarget error: Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping).
// EXE-caseParallelSubarrayOverlapStart-OFF-NEXT: Libomptarget error: Call to targetDataBegin failed, abort target.
// EXE-caseParallelSubarrayOverlapStart-OFF-NEXT: Libomptarget error: Failed to process data before launching the kernel.
// EXE-caseParallelSubarrayOverlapStart-OFF-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
// EXE-caseParallelSubarrayOverlapStart-OFF-NEXT: {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                                # An abort message usually follows.
//  EXE-caseParallelSubarrayOverlapStart-OFF-NOT: Libomptarget
CASE(caseParallelSubarrayOverlapStart) {
  int arr[10];
  PRINT_SUBARRAY_INFO(arr, 5, 4);
  PRINT_SUBARRAY_INFO(arr, 4, 4);
  int use = 0;
  #pragma acc data copyin(arr[5:4])
  #pragma acc parallel copy(arr[4:4])
  if (use) arr[4] = 1;
}

//      EXE-caseParallelSubarrayOverlapEnd-NOT: {{.}}
//          EXE-caseParallelSubarrayOverlapEnd: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//     EXE-caseParallelSubarrayOverlapEnd-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
// EXE-caseParallelSubarrayOverlapEnd-OFF-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//                                              # FIXME: Names like getOrAllocTgtPtr are meaningless to users.
// EXE-caseParallelSubarrayOverlapEnd-OFF-NEXT: Libomptarget error: Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping).
// EXE-caseParallelSubarrayOverlapEnd-OFF-NEXT: Libomptarget error: Call to targetDataBegin failed, abort target.
// EXE-caseParallelSubarrayOverlapEnd-OFF-NEXT: Libomptarget error: Failed to process data before launching the kernel.
// EXE-caseParallelSubarrayOverlapEnd-OFF-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
// EXE-caseParallelSubarrayOverlapEnd-OFF-NEXT: {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                              # An abort message usually follows.
//  EXE-caseParallelSubarrayOverlapEnd-OFF-NOT: Libomptarget
CASE(caseParallelSubarrayOverlapEnd) {
  int arr[10];
  PRINT_SUBARRAY_INFO(arr, 3, 4);
  PRINT_SUBARRAY_INFO(arr, 4, 4);
  int use = 0;
  #pragma acc data copyin(arr[3:4])
  #pragma acc parallel copyin(arr[4:4])
  if (use) arr[4] = 1;
}

//      EXE-caseParallelSubarrayConcat2-NOT: {{.}}
//          EXE-caseParallelSubarrayConcat2: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//     EXE-caseParallelSubarrayConcat2-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
// EXE-caseParallelSubarrayConcat2-OFF-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//                                           # FIXME: Names like getOrAllocTgtPtr are meaningless to users.
// EXE-caseParallelSubarrayConcat2-OFF-NEXT: Libomptarget error: Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping).
// EXE-caseParallelSubarrayConcat2-OFF-NEXT: Libomptarget error: Call to targetDataBegin failed, abort target.
// EXE-caseParallelSubarrayConcat2-OFF-NEXT: Libomptarget error: Failed to process data before launching the kernel.
// EXE-caseParallelSubarrayConcat2-OFF-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
// EXE-caseParallelSubarrayConcat2-OFF-NEXT: {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                           # An abort message usually follows.
//  EXE-caseParallelSubarrayConcat2-OFF-NOT: Libomptarget
CASE(caseParallelSubarrayConcat2) {
  int arr[4];
  PRINT_SUBARRAY_INFO(arr, 0, 2);
  PRINT_SUBARRAY_INFO(arr, 0, 4);
  int use = 0;
  #pragma acc data copyout(arr[0:2])
  #pragma acc data copy(arr[2:2])
  #pragma acc parallel copyout(arr[0:4])
  if (use) arr[0] = 1;
}

//      EXE-caseParallelSubarrayNonSubarray-NOT: {{.}}
//          EXE-caseParallelSubarrayNonSubarray: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//     EXE-caseParallelSubarrayNonSubarray-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
// EXE-caseParallelSubarrayNonSubarray-OFF-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//                                               # FIXME: Names like getOrAllocTgtPtr are meaningless to users.
// EXE-caseParallelSubarrayNonSubarray-OFF-NEXT: Libomptarget error: Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping).
// EXE-caseParallelSubarrayNonSubarray-OFF-NEXT: Libomptarget error: Call to targetDataBegin failed, abort target.
// EXE-caseParallelSubarrayNonSubarray-OFF-NEXT: Libomptarget error: Failed to process data before launching the kernel.
// EXE-caseParallelSubarrayNonSubarray-OFF-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
// EXE-caseParallelSubarrayNonSubarray-OFF-NEXT: {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                               # An abort message usually follows.
//  EXE-caseParallelSubarrayNonSubarray-OFF-NOT: Libomptarget
CASE(caseParallelSubarrayNonSubarray) {
  int arr[4];
  PRINT_SUBARRAY_INFO(arr, 1, 2);
  PRINT_SUBARRAY_INFO(arr, 0, 4);
  int use = 0;
  #pragma acc data copy(arr[1:2])
  #pragma acc parallel copyout(arr)
  if (use) arr[0] = 1;
}

// EXE-HOST-NOT: {{.}}
