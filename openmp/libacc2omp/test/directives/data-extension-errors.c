// Check various cases of data extension errors.
//
// The various cases covered here should be kept consistent with present.c,
// no-create.c, and update.c from clang/test/OpenACC/directives.  The present
// and no_create clauses are tested separately because they have special
// behavior.  This test file is meant to cover copy, copyin, copyout, and
// create, which we just cycle through across the various subarray extension
// cases.

// Redefine this to specify how %{check-cases} expands for each case.
//
// - CASE = the case name used in the enum and as a command line argument.
// - NOT_CRASH_IF_FAIL = 'not --crash' if the case is expected to fail at run
//   time, or the empty string otherwise.
//
// DEFINE: %{check-case}( CASE %, NOT_CRASH_IF_FAIL %) =

// Substitution to run %{check-case} for each case.
//
// If a case is listed here but is not covered in the code, that case will fail.
// If a case is covered in the code but not listed here, the code will not
// compile because this list produces the enum used by the code.
//
// "acc parallel loop" should be about the same as "acc parallel", so a few
// cases are probably sufficient for it.
//
// DEFINE: %{check-cases} =                                                                   \
//                          CASE                                NOT_CRASH_IF_FAIL
// DEFINE:   %{check-case}( caseDataMemberPresent            %,                         %) && \
// DEFINE:   %{check-case}( caseDataMemberAbsent             %,                         %) && \
// DEFINE:   %{check-case}( caseDataMembersDisjoint          %,                         %) && \
// DEFINE:   %{check-case}( caseDataMembersConcat2           %, %if-host<|%not --crash> %) && \
// DEFINE:   %{check-case}( caseDataMemberFullStruct         %, %if-host<|%not --crash> %) && \
// DEFINE:   %{check-case}( caseDataSubarrayPresent          %,                         %) && \
// DEFINE:   %{check-case}( caseDataSubarrayDisjoint         %,                         %) && \
// DEFINE:   %{check-case}( caseDataSubarrayOverlapStart     %, %if-host<|%not --crash> %) && \
// DEFINE:   %{check-case}( caseDataSubarrayOverlapEnd       %, %if-host<|%not --crash> %) && \
// DEFINE:   %{check-case}( caseDataSubarrayConcat2          %, %if-host<|%not --crash> %) && \
// DEFINE:   %{check-case}( caseDataSubarrayNonSubarray      %, %if-host<|%not --crash> %) && \
// DEFINE:   %{check-case}( caseParallelMemberPresent        %,                         %) && \
// DEFINE:   %{check-case}( caseParallelMemberAbsent         %,                         %) && \
// DEFINE:   %{check-case}( caseParallelMembersDisjoint      %,                         %) && \
// DEFINE:   %{check-case}( caseParallelMembersConcat2       %, %if-host<|%not --crash> %) && \
// DEFINE:   %{check-case}( caseParallelMemberFullStruct     %, %if-host<|%not --crash> %) && \
// DEFINE:   %{check-case}( caseParallelSubarrayPresent      %,                         %) && \
// DEFINE:   %{check-case}( caseParallelSubarrayDisjoint     %,                         %) && \
// DEFINE:   %{check-case}( caseParallelSubarrayOverlapStart %, %if-host<|%not --crash> %) && \
// DEFINE:   %{check-case}( caseParallelSubarrayOverlapEnd   %, %if-host<|%not --crash> %) && \
// DEFINE:   %{check-case}( caseParallelSubarrayConcat2      %, %if-host<|%not --crash> %) && \
// DEFINE:   %{check-case}( caseParallelSubarrayNonSubarray  %, %if-host<|%not --crash> %)

// Generate the enum of cases.
//
//      RUN: echo '#define FOREACH_CASE(Macro) \' > %t-cases.h
// REDEFINE: %{check-case}( CASE %, NOT_CRASH_IF_FAIL %) = \
// REDEFINE: echo '  Macro(%{CASE}) \' >> %t-cases.h
//      RUN: %{check-cases}
//      RUN: echo '  /*end of FOREACH_CASE*/' >> %t-cases.h

// Try all cases with normal compilation.
//
// REDEFINE: %{check-case}( CASE %, NOT_CRASH_IF_FAIL %) =                     \
// REDEFINE:   : '---------- CASE: %{CASE} ----------' &&                      \
// REDEFINE:   %{NOT_CRASH_IF_FAIL} %t.exe %{CASE} > %t.out 2>&1 &&            \
// REDEFINE:   FileCheck -input-file %t.out %s                                 \
// REDEFINE:     -match-full-lines -allow-empty                                \
// REDEFINE:     -check-prefixes=EXE,EXE-%{CASE}                               \
// REDEFINE:     -check-prefixes=EXE-%if-host<HOST|OFF>                        \
// REDEFINE:     -check-prefixes=EXE-%{CASE}-%if-host<HOST|OFF>

// RUN: %clang-acc -DCASES_HEADER='"%t-cases.h"' -o %t.exe %s
// RUN: %{check-cases}

// END.

// expected-no-diagnostics

#include <stdio.h>
#include <string.h>

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

// EXE-caseDataMemberPresent-NOT: {{.}}
CASE(caseDataMemberPresent) {
  struct S { int x; int y; int z; };
  struct S same, beg, mid, end;
  #pragma acc data copy(same,beg,mid,end)
  #pragma acc data copy(same.x,same.y,same.z,beg.x,mid.y,end.z)
  ;
  #pragma acc data copy(same.y,same.z,beg.y,beg.z,mid.x,mid.y,mid.z,end.x,end.y)
  #pragma acc data copy(same.y,same.z,beg.y,mid.y,end.y)
  ;
}

// EXE-caseDataMemberAbsent-NOT: {{.}}
CASE(caseDataMemberAbsent) {
  struct S { int x; int y; int z; } s;
  #pragma acc data copy(s.y)
  ;
}

// EXE-caseDataMembersDisjoint-NOT: {{.}}
CASE(caseDataMembersDisjoint) {
  struct S { int x; int y; } s;
  #pragma acc data copy(s.x)
  #pragma acc data copyin(s.y)
  ;
}

//      EXE-caseDataMembersConcat2-NOT: {{.}}
//          EXE-caseDataMembersConcat2: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//     EXE-caseDataMembersConcat2-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
// EXE-caseDataMembersConcat2-OFF-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
// EXE-caseDataMembersConcat2-OFF-NEXT: Libomptarget error: Call to getTargetPointer returned null pointer (device failure or illegal mapping).
// EXE-caseDataMembersConcat2-OFF-NEXT: Libomptarget error: Consult {{.*}}
// EXE-caseDataMembersConcat2-OFF-NEXT: {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                       # An abort message usually follows.
//  EXE-caseDataMembersConcat2-OFF-NOT: Libomptarget
CASE(caseDataMembersConcat2) {
  struct S { int x; int y; } s;
  PRINT_VAR_INFO(s.x);
  PRINT_VAR_INFO(s);
  #pragma acc data copyout(s.x)
  #pragma acc data copy(s.y)
  #pragma acc data copy(s)
  ;
}

//      EXE-caseDataMemberFullStruct-NOT: {{.}}
//          EXE-caseDataMemberFullStruct: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//     EXE-caseDataMemberFullStruct-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
// EXE-caseDataMemberFullStruct-OFF-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
// EXE-caseDataMemberFullStruct-OFF-NEXT: Libomptarget error: Call to getTargetPointer returned null pointer (device failure or illegal mapping).
// EXE-caseDataMemberFullStruct-OFF-NEXT: Libomptarget error: Consult {{.*}}
// EXE-caseDataMemberFullStruct-OFF-NEXT: {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                           # An abort message usually follows.
//  EXE-caseDataMemberFullStruct-OFF-NOT: Libomptarget
CASE(caseDataMemberFullStruct) {
  struct S { int x; int y; } s;
  PRINT_VAR_INFO(s.x);
  PRINT_VAR_INFO(s);
  #pragma acc data copyin(s.x)
  #pragma acc data copyin(s)
  ;
}

// EXE-caseDataSubarrayPresent-NOT: {{.}}
CASE(caseDataSubarrayPresent) {
  int same[10], beg[10], mid[10], end[10];
  #pragma acc data copy(same,beg,mid,end)
  #pragma acc data copy(same[0:10],beg[0:3],mid[3:3],end[8:2])
  ;
  #pragma acc data copy(same[3:6],beg[2:5],mid[1:8],end[0:5])
  #pragma acc data copy(same[3:6],beg[2:2],mid[3:3],end[4:1])
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
// EXE-caseDataSubarrayOverlapStart-OFF-NEXT: Libomptarget error: Call to getTargetPointer returned null pointer (device failure or illegal mapping).
// EXE-caseDataSubarrayOverlapStart-OFF-NEXT: Libomptarget error: Consult {{.*}}
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
//  EXE-caseDataSubarrayOverlapEnd-OFF-NEXT: Libomptarget error: Call to getTargetPointer returned null pointer (device failure or illegal mapping).
//  EXE-caseDataSubarrayOverlapEnd-OFF-NEXT: Libomptarget error: Consult {{.*}}
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
// EXE-caseDataSubarrayConcat2-OFF-NEXT: Libomptarget error: Call to getTargetPointer returned null pointer (device failure or illegal mapping).
// EXE-caseDataSubarrayConcat2-OFF-NEXT: Libomptarget error: Consult {{.*}}
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
// EXE-caseDataSubarrayNonSubarray-OFF-NEXT: Libomptarget error: Call to getTargetPointer returned null pointer (device failure or illegal mapping).
// EXE-caseDataSubarrayNonSubarray-OFF-NEXT: Libomptarget error: Consult {{.*}}
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

// EXE-caseParallelMemberPresent-NOT: {{.}}
CASE(caseParallelMemberPresent) {
  struct S { int x; int y; int z; };
  struct S same, beg, mid, end;
  #pragma acc data copy(same,beg,mid,end)
  #pragma acc parallel copyout(same.x,same.y,same.z,beg.x,mid.y,end.z)
  {
    same.x = 1; beg.x = 1; mid.y = 1; end.z = 1;
  }
  #pragma acc data copy(same.y,beg.y,beg.z,mid.x,mid.y,mid.z,end.x,end.y)
  #pragma acc parallel copyout(same.y,beg.y,mid.y,end.y)
  {
    same.y = 1; beg.y = 1; mid.y = 1; end.y = 1;
  }
}

// EXE-caseParallelMemberAbsent-NOT: {{.}}
CASE(caseParallelMemberAbsent) {
  struct S { int x; int y; int z; } s;
  #pragma acc data copy(s.y)
  s.y = 1;
}

// EXE-caseParallelMembersDisjoint-NOT: {{.}}
CASE(caseParallelMembersDisjoint) {
  struct S { int w; int x; int y; int z; } s;
  int use = 0;
  #pragma acc data copy(s.w, s.x)
  #pragma acc parallel copyin(s.y, s.z)
  if (use) s.y = 1;
}

//      EXE-caseParallelMembersConcat2-NOT: {{.}}
//          EXE-caseParallelMembersConcat2: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//     EXE-caseParallelMembersConcat2-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
// EXE-caseParallelMembersConcat2-OFF-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//                                           # FIXME: Names like getTargetPointer are meaningless to users.
// EXE-caseParallelMembersConcat2-OFF-NEXT: Libomptarget error: Call to getTargetPointer returned null pointer (device failure or illegal mapping).
// EXE-caseParallelMembersConcat2-OFF-NEXT: Libomptarget error: Call to targetDataBegin failed, abort target.
// EXE-caseParallelMembersConcat2-OFF-NEXT: Libomptarget error: Failed to process data before launching the kernel.
// EXE-caseParallelMembersConcat2-OFF-NEXT: Libomptarget error: Consult {{.*}}
// EXE-caseParallelMembersConcat2-OFF-NEXT: {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                           # An abort message usually follows.
//  EXE-caseParallelMembersConcat2-OFF-NOT: Libomptarget
CASE(caseParallelMembersConcat2) {
  struct S { int x; int y; } s;
  PRINT_VAR_INFO(s.x);
  PRINT_VAR_INFO(s);
  int use = 0;
  #pragma acc data copyout(s.x)
  #pragma acc data copy(s.y)
  #pragma acc parallel copyout(s)
  if (use) s.x = 1;
}

//      EXE-caseParallelMemberFullStruct-NOT: {{.}}
//          EXE-caseParallelMemberFullStruct: addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//     EXE-caseParallelMemberFullStruct-NEXT: addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
// EXE-caseParallelMemberFullStruct-OFF-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//                                               # FIXME: Names like getTargetPointer are meaningless to users.
// EXE-caseParallelMemberFullStruct-OFF-NEXT: Libomptarget error: Call to getTargetPointer returned null pointer (device failure or illegal mapping).
// EXE-caseParallelMemberFullStruct-OFF-NEXT: Libomptarget error: Call to targetDataBegin failed, abort target.
// EXE-caseParallelMemberFullStruct-OFF-NEXT: Libomptarget error: Failed to process data before launching the kernel.
// EXE-caseParallelMemberFullStruct-OFF-NEXT: Libomptarget error: Consult {{.*}}
// EXE-caseParallelMemberFullStruct-OFF-NEXT: {{.*:[0-9]+:[0-9]+}}: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                                               # An abort message usually follows.
//  EXE-caseParallelMemberFullStruct-OFF-NOT: Libomptarget
CASE(caseParallelMemberFullStruct) {
  struct S { int x; int y; } s;
  PRINT_VAR_INFO(s.y);
  PRINT_VAR_INFO(s);
  int use = 0;
  #pragma acc data copy(s.y)
  #pragma acc parallel copyout(s)
  if (use) s.y = 1;
  ;
}

// EXE-caseParallelSubarrayPresent-NOT: {{.}}
CASE(caseParallelSubarrayPresent) {
  int same[10], beg[10], mid[10], end[10];
  #pragma acc data copy(same,beg,mid,end)
  #pragma acc parallel copyout(same[0:10],beg[0:3],mid[3:3],end[8:2])
  {
    same[0] = 1; beg[0] = 1; mid[3] = 1; end[8] = 1;
  }
  #pragma acc data copy(same[3:6],beg[2:5],mid[1:8],end[0:5])
  #pragma acc parallel copyout(same[3:6],beg[2:2],mid[3:3],end[4:1])
  {
    same[3] = 1; beg[2] = 1; mid[3] = 1; end[4] = 1;
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
//                                                # FIXME: Names like getTargetPointer are meaningless to users.
// EXE-caseParallelSubarrayOverlapStart-OFF-NEXT: Libomptarget error: Call to getTargetPointer returned null pointer (device failure or illegal mapping).
// EXE-caseParallelSubarrayOverlapStart-OFF-NEXT: Libomptarget error: Call to targetDataBegin failed, abort target.
// EXE-caseParallelSubarrayOverlapStart-OFF-NEXT: Libomptarget error: Failed to process data before launching the kernel.
// EXE-caseParallelSubarrayOverlapStart-OFF-NEXT: Libomptarget error: Consult {{.*}}
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
//                                              # FIXME: Names like getTargetPointer are meaningless to users.
// EXE-caseParallelSubarrayOverlapEnd-OFF-NEXT: Libomptarget error: Call to getTargetPointer returned null pointer (device failure or illegal mapping).
// EXE-caseParallelSubarrayOverlapEnd-OFF-NEXT: Libomptarget error: Call to targetDataBegin failed, abort target.
// EXE-caseParallelSubarrayOverlapEnd-OFF-NEXT: Libomptarget error: Failed to process data before launching the kernel.
// EXE-caseParallelSubarrayOverlapEnd-OFF-NEXT: Libomptarget error: Consult {{.*}}
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
//                                           # FIXME: Names like getTargetPointer are meaningless to users.
// EXE-caseParallelSubarrayConcat2-OFF-NEXT: Libomptarget error: Call to getTargetPointer returned null pointer (device failure or illegal mapping).
// EXE-caseParallelSubarrayConcat2-OFF-NEXT: Libomptarget error: Call to targetDataBegin failed, abort target.
// EXE-caseParallelSubarrayConcat2-OFF-NEXT: Libomptarget error: Failed to process data before launching the kernel.
// EXE-caseParallelSubarrayConcat2-OFF-NEXT: Libomptarget error: Consult {{.*}}
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
//                                               # FIXME: Names like getTargetPointer are meaningless to users.
// EXE-caseParallelSubarrayNonSubarray-OFF-NEXT: Libomptarget error: Call to getTargetPointer returned null pointer (device failure or illegal mapping).
// EXE-caseParallelSubarrayNonSubarray-OFF-NEXT: Libomptarget error: Call to targetDataBegin failed, abort target.
// EXE-caseParallelSubarrayNonSubarray-OFF-NEXT: Libomptarget error: Failed to process data before launching the kernel.
// EXE-caseParallelSubarrayNonSubarray-OFF-NEXT: Libomptarget error: Consult {{.*}}
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
