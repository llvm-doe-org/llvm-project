// Check data and memory management routines.

// RUN: %data tgts {
// RUN:   (run-if=                cflags=                                     tgt-host-or-off=HOST tgt-not-if-off=              )
// RUN:   (run-if=%run-if-x86_64  cflags=-fopenmp-targets=%run-x86_64-triple  tgt-host-or-off=OFF  tgt-not-if-off='%not --crash')
// RUN:   (run-if=%run-if-ppc64le cflags=-fopenmp-targets=%run-ppc64le-triple tgt-host-or-off=OFF  tgt-not-if-off='%not --crash')
// RUN:   (run-if=%run-if-nvptx64 cflags=-fopenmp-targets=%run-nvptx64-triple tgt-host-or-off=OFF  tgt-not-if-off='%not --crash')
// RUN: }
// RUN: %data run-envs {
// RUN:   (run-env=                                  host-or-off=%[tgt-host-or-off] not-if-off=%[tgt-not-if-off])
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled' host-or-off=HOST               not-if-off=                 )
// RUN: }
// RUN: %data cases {
// RUN:   (case=CASE_DEVICEPTR_SUCCESS              not-if-fail=              )
// RUN:   (case=CASE_IS_PRESENT_SUCCESS             not-if-fail=              )
// RUN:   (case=CASE_MAP_UNMAP_SUCCESS              not-if-fail=              )
// RUN:   (case=CASE_MAP_SAME_HOST_AS_STRUCTURED    not-if-fail=%[not-if-off] )
// RUN:   (case=CASE_MAP_SAME_HOST_AS_DYNAMIC       not-if-fail=%[not-if-off] )
// RUN:   (case=CASE_MAP_SAME                       not-if-fail=%[not-if-off] )
// RUN:   (case=CASE_MAP_SAME_HOST                  not-if-fail=%[not-if-off] )
// RUN:   (case=CASE_MAP_HOST_EXTENDS_AFTER         not-if-fail=%[not-if-off] )
// RUN:   (case=CASE_MAP_HOST_EXTENDS_BEFORE        not-if-fail=%[not-if-off] )
// RUN:   (case=CASE_MAP_HOST_SUBSUMES              not-if-fail=%[not-if-off] )
// RUN:   (case=CASE_MAP_HOST_IS_SUBSUMED           not-if-fail=%[not-if-off] )
// RUN:   (case=CASE_MAP_HOST_NULL                  not-if-fail='%not --crash')
// RUN:   (case=CASE_MAP_DEV_NULL                   not-if-fail='%not --crash')
// RUN:   (case=CASE_MAP_BYTES_ZERO                 not-if-fail='%not --crash')
// RUN:   (case=CASE_MAP_ALL_NULL                   not-if-fail='%not --crash')
// RUN:   (case=CASE_UNMAP_NULL                     not-if-fail='%not --crash')
// RUN:   (case=CASE_UNMAP_UNMAPPED                 not-if-fail=%[not-if-off] )
// RUN:   (case=CASE_UNMAP_AFTER_ONLY_STRUCTURED    not-if-fail=%[not-if-off] )
// RUN:   (case=CASE_UNMAP_AFTER_ONLY_DYNAMIC       not-if-fail=%[not-if-off] )
// RUN:   (case=CASE_UNMAP_AFTER_MAP_AND_STRUCTURED not-if-fail=%[not-if-off] )
// RUN:   (case=CASE_UNMAP_AFTER_ALL_THREE          not-if-fail=%[not-if-off] )
// RUN: }
// RUN: %for tgts {
// RUN:   %[run-if] %clang -Xclang -verify -fopenacc %acc-includes %[cflags] \
// RUN:             -o %t.exe %s
// RUN:   %for run-envs {
// RUN:     rm -f %t.actual-cases && touch %t.actual-cases
// RUN:     %for cases {
// RUN:       %[run-if] %[run-env] %[not-if-fail] %t.exe %[case] \
// RUN:                            > %t.out 2> %t.err
// RUN:       %[run-if] FileCheck \
// RUN:           -input-file %t.out %s -match-full-lines -allow-empty \
// RUN:           -check-prefixes=OUT,OUT-%[case],OUT-%[case]-%[host-or-off]
// RUN:       %[run-if] FileCheck \
// RUN:           -input-file %t.err %s -match-full-lines -allow-empty \
// RUN:           -check-prefixes=ERR,ERR-%[case],ERR-%[case]-%[host-or-off]
// RUN:       echo '%[case]' >> %t.actual-cases
// RUN:     }
// RUN:   }
// RUN: }
//
// Make sure %data cases didn't omit any cases defined in the code.
//
// RUN: %t.exe -dump-cases > %t.expected-cases
// RUN: diff -u %t.expected-cases %t.actual-cases >&2
//
// END.

// expected-no-diagnostics

#include <openacc.h>
#include <stdio.h>
#include <string.h>

#define FOREACH_CASE(Macro)                                                    \
  Macro(CASE_DEVICEPTR_SUCCESS)                                                \
  Macro(CASE_IS_PRESENT_SUCCESS)                                               \
  Macro(CASE_MAP_UNMAP_SUCCESS)                                                \
  Macro(CASE_MAP_SAME_HOST_AS_STRUCTURED)                                      \
  Macro(CASE_MAP_SAME_HOST_AS_DYNAMIC)                                         \
  Macro(CASE_MAP_SAME)                                                         \
  Macro(CASE_MAP_SAME_HOST)                                                    \
  Macro(CASE_MAP_HOST_EXTENDS_AFTER)                                           \
  Macro(CASE_MAP_HOST_EXTENDS_BEFORE)                                          \
  Macro(CASE_MAP_HOST_SUBSUMES)                                                \
  Macro(CASE_MAP_HOST_IS_SUBSUMED)                                             \
  Macro(CASE_MAP_HOST_NULL)                                                    \
  Macro(CASE_MAP_DEV_NULL)                                                     \
  Macro(CASE_MAP_BYTES_ZERO)                                                   \
  Macro(CASE_MAP_ALL_NULL)                                                     \
  Macro(CASE_UNMAP_NULL)                                                       \
  Macro(CASE_UNMAP_UNMAPPED)                                                   \
  Macro(CASE_UNMAP_AFTER_ONLY_STRUCTURED)                                      \
  Macro(CASE_UNMAP_AFTER_ONLY_DYNAMIC)                                         \
  Macro(CASE_UNMAP_AFTER_MAP_AND_STRUCTURED)                                   \
  Macro(CASE_UNMAP_AFTER_ALL_THREE)

enum Case {
#define AddCase(CaseName) \
  CaseName,
FOREACH_CASE(AddCase)
#undef AddCase
  CASE_END
};

const char *CaseNames[] = {
#define AddCase(CaseName) \
  #CaseName,
FOREACH_CASE(AddCase)
#undef AddCase
};

// OUT-NOT: {{.}}
// ERR-NOT: {{.}}

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
  // OUT: start
  printf("start\n");
  fflush(stdout);
  switch (selectedCase) {
  case CASE_DEVICEPTR_SUCCESS: {
    int arr[3];
    // OUT-CASE_DEVICEPTR_SUCCESS-NEXT: arr: 0x[[#%x,ARR:]]
    // OUT-CASE_DEVICEPTR_SUCCESS-NEXT: element size: [[#%u,ELE_SIZE:]]
    printf("arr: %p\n", arr);
    printf("element size: %zu\n", sizeof *arr);

    // acc_deviceptr with acc_map_data/acc_unmap_data is checked in
    // CASE_MAP_UNMAP_SUCCESS.

    // Check with acc data.
    //
    // OUT-CASE_DEVICEPTR_SUCCESS-HOST-NEXT: deviceptr: 0x[[#ARR]]
    //  OUT-CASE_DEVICEPTR_SUCCESS-OFF-NEXT: deviceptr: (nil)
    printf("deviceptr: %p\n", acc_deviceptr(arr));
    #pragma acc data create(arr)
    {
      // OUT-CASE_DEVICEPTR_SUCCESS-HOST-NEXT: deviceptr: 0x[[#ARR_DEV:ARR]]
      //  OUT-CASE_DEVICEPTR_SUCCESS-OFF-NEXT: deviceptr: 0x[[#%x,ARR_DEV:]]
      //      OUT-CASE_DEVICEPTR_SUCCESS-NEXT: deviceptr: 0x[[#ARR_DEV]]
      //      OUT-CASE_DEVICEPTR_SUCCESS-NEXT: deviceptr: 0x[[#ARR_DEV]]
      printf("deviceptr: %p\n", acc_deviceptr(arr));
      #pragma acc data create(arr)
      {
        printf("deviceptr: %p\n", acc_deviceptr(arr));
      }
      printf("deviceptr: %p\n", acc_deviceptr(arr));
    }
    // OUT-CASE_DEVICEPTR_SUCCESS-HOST-NEXT: deviceptr: 0x[[#ARR]]
    //  OUT-CASE_DEVICEPTR_SUCCESS-OFF-NEXT: deviceptr: (nil)
    printf("deviceptr: %p\n", acc_deviceptr(arr));

    // Check with acc enter/exit data.
    //
    // OUT-CASE_DEVICEPTR_SUCCESS-HOST-NEXT: deviceptr: 0x[[#ARR]]
    //  OUT-CASE_DEVICEPTR_SUCCESS-OFF-NEXT: deviceptr: (nil)
    // OUT-CASE_DEVICEPTR_SUCCESS-HOST-NEXT: deviceptr: 0x[[#ARR_DEV:ARR]]
    //  OUT-CASE_DEVICEPTR_SUCCESS-OFF-NEXT: deviceptr: 0x[[#%x,ARR_DEV:]]
    //      OUT-CASE_DEVICEPTR_SUCCESS-NEXT: deviceptr: 0x[[#ARR_DEV]]
    //      OUT-CASE_DEVICEPTR_SUCCESS-NEXT: deviceptr: 0x[[#ARR_DEV]]
    // OUT-CASE_DEVICEPTR_SUCCESS-HOST-NEXT: deviceptr: 0x[[#ARR]]
    //  OUT-CASE_DEVICEPTR_SUCCESS-OFF-NEXT: deviceptr: (nil)
    printf("deviceptr: %p\n", acc_deviceptr(arr));
    #pragma acc enter data create(arr)
    printf("deviceptr: %p\n", acc_deviceptr(arr));
    #pragma acc enter data create(arr)
    printf("deviceptr: %p\n", acc_deviceptr(arr));
    #pragma acc exit data delete(arr)
    printf("deviceptr: %p\n", acc_deviceptr(arr));
    #pragma acc exit data delete(arr)
    printf("deviceptr: %p\n", acc_deviceptr(arr));

    // Check that the correct offset is computed when the address is within a
    // larger allocation.
    #pragma acc data create(arr)
    {
      // OUT-CASE_DEVICEPTR_SUCCESS-HOST-NEXT: deviceptr: 0x[[#ARR_DEV:ARR]]
      //  OUT-CASE_DEVICEPTR_SUCCESS-OFF-NEXT: deviceptr: 0x[[#%x,ARR_DEV:]]
      //      OUT-CASE_DEVICEPTR_SUCCESS-NEXT: deviceptr: 0x[[#%x,ARR_DEV + ELE_SIZE]]
      //      OUT-CASE_DEVICEPTR_SUCCESS-NEXT: deviceptr: 0x[[#%x,ARR_DEV + ELE_SIZE + ELE_SIZE]]
      // OUT-CASE_DEVICEPTR_SUCCESS-HOST-NEXT: deviceptr: 0x[[#%x,ARR_DEV + ELE_SIZE + ELE_SIZE + ELE_SIZE]]
      //  OUT-CASE_DEVICEPTR_SUCCESS-OFF-NEXT: deviceptr: (nil)
      printf("deviceptr: %p\n", acc_deviceptr(arr));
      printf("deviceptr: %p\n", acc_deviceptr(arr + 1));
      printf("deviceptr: %p\n", acc_deviceptr(arr + 2));
      printf("deviceptr: %p\n", acc_deviceptr(arr + 3));
    }

    // Check case of null pointer.
    //
    // OUT-CASE_DEVICEPTR_SUCCESS-NEXT: deviceptr: (nil)
    printf("deviceptr: %p\n", acc_deviceptr(NULL));

    break;
  }

  case CASE_IS_PRESENT_SUCCESS: {
    int arr[] = {10, 20, 30};

    // acc_is_present with acc_map_data/acc_unmap_data is checked in
    // CASE_MAP_UNMAP_SUCCESS.

    // Check with acc data.
    //
    // OUT-CASE_IS_PRESENT_SUCCESS-HOST-NEXT: is_present: 1
    //  OUT-CASE_IS_PRESENT_SUCCESS-OFF-NEXT: is_present: 0
    printf("is_present: %d\n", acc_is_present(arr, sizeof arr));
    #pragma acc data create(arr)
    {
      // OUT-CASE_IS_PRESENT_SUCCESS-NEXT: is_present: 1
      // OUT-CASE_IS_PRESENT_SUCCESS-NEXT: is_present: 1
      // OUT-CASE_IS_PRESENT_SUCCESS-NEXT: is_present: 1
      printf("is_present: %d\n", acc_is_present(arr, sizeof arr));
      #pragma acc data create(arr)
      {
        printf("is_present: %d\n", acc_is_present(arr, sizeof arr));
      }
      printf("is_present: %d\n", acc_is_present(arr, sizeof arr));
    }
    // OUT-CASE_IS_PRESENT_SUCCESS-HOST-NEXT: is_present: 1
    //  OUT-CASE_IS_PRESENT_SUCCESS-OFF-NEXT: is_present: 0
    printf("is_present: %d\n", acc_is_present(arr, sizeof arr));

    // Check with acc enter/exit data.
    //
    // OUT-CASE_IS_PRESENT_SUCCESS-HOST-NEXT: is_present: 1
    //  OUT-CASE_IS_PRESENT_SUCCESS-OFF-NEXT: is_present: 0
    //      OUT-CASE_IS_PRESENT_SUCCESS-NEXT: is_present: 1
    //      OUT-CASE_IS_PRESENT_SUCCESS-NEXT: is_present: 1
    //      OUT-CASE_IS_PRESENT_SUCCESS-NEXT: is_present: 1
    // OUT-CASE_IS_PRESENT_SUCCESS-HOST-NEXT: is_present: 1
    //  OUT-CASE_IS_PRESENT_SUCCESS-OFF-NEXT: is_present: 0
    printf("is_present: %d\n", acc_is_present(arr, sizeof arr));
    #pragma acc enter data create(arr)
    printf("is_present: %d\n", acc_is_present(arr, sizeof arr));
    #pragma acc enter data create(arr)
    printf("is_present: %d\n", acc_is_present(arr, sizeof arr));
    #pragma acc exit data delete(arr)
    printf("is_present: %d\n", acc_is_present(arr, sizeof arr));
    #pragma acc exit data delete(arr)
    printf("is_present: %d\n", acc_is_present(arr, sizeof arr));

    // Check when already present array is larger: extends after, extends
    // before, or both.
    //
    // OUT-CASE_IS_PRESENT_SUCCESS-HOST-NEXT: is_present: 1
    // OUT-CASE_IS_PRESENT_SUCCESS-HOST-NEXT: is_present: 1
    // OUT-CASE_IS_PRESENT_SUCCESS-HOST-NEXT: is_present: 1
    //  OUT-CASE_IS_PRESENT_SUCCESS-OFF-NEXT: is_present: 0
    //  OUT-CASE_IS_PRESENT_SUCCESS-OFF-NEXT: is_present: 0
    //  OUT-CASE_IS_PRESENT_SUCCESS-OFF-NEXT: is_present: 0
    printf("is_present: %d\n", acc_is_present(arr, 2 * sizeof *arr));
    printf("is_present: %d\n", acc_is_present(arr+1, 2 * sizeof *arr));
    printf("is_present: %d\n", acc_is_present(arr+1, sizeof *arr));
    #pragma acc data create(arr)
    {
      // OUT-CASE_IS_PRESENT_SUCCESS-NEXT: is_present: 1
      // OUT-CASE_IS_PRESENT_SUCCESS-NEXT: is_present: 1
      // OUT-CASE_IS_PRESENT_SUCCESS-NEXT: is_present: 1
      printf("is_present: %d\n", acc_is_present(arr, 2 * sizeof *arr));
      printf("is_present: %d\n", acc_is_present(arr+1, 2 * sizeof *arr));
      printf("is_present: %d\n", acc_is_present(arr+1, sizeof *arr));
    }

    // Check when already present array is smaller: extends after, extends
    // before, or both.
    //
    // OUT-CASE_IS_PRESENT_SUCCESS-HOST-NEXT: is_present: 1
    // OUT-CASE_IS_PRESENT_SUCCESS-HOST-NEXT: is_present: 1
    // OUT-CASE_IS_PRESENT_SUCCESS-HOST-NEXT: is_present: 1
    // OUT-CASE_IS_PRESENT_SUCCESS-HOST-NEXT: is_present: 1
    // OUT-CASE_IS_PRESENT_SUCCESS-HOST-NEXT: is_present: 1
    // OUT-CASE_IS_PRESENT_SUCCESS-HOST-NEXT: is_present: 1
    //  OUT-CASE_IS_PRESENT_SUCCESS-OFF-NEXT: is_present: 0
    //  OUT-CASE_IS_PRESENT_SUCCESS-OFF-NEXT: is_present: 0
    //  OUT-CASE_IS_PRESENT_SUCCESS-OFF-NEXT: is_present: 0
    //  OUT-CASE_IS_PRESENT_SUCCESS-OFF-NEXT: is_present: 0
    //  OUT-CASE_IS_PRESENT_SUCCESS-OFF-NEXT: is_present: 0
    //  OUT-CASE_IS_PRESENT_SUCCESS-OFF-NEXT: is_present: 0
    printf("is_present: %d\n", acc_is_present(arr, sizeof arr));
    #pragma acc data create(arr[0:2])
    {
      printf("is_present: %d\n", acc_is_present(arr, sizeof arr));
    }
    printf("is_present: %d\n", acc_is_present(arr, sizeof arr));
    #pragma acc data create(arr[1:2])
    {
      printf("is_present: %d\n", acc_is_present(arr, sizeof arr));
    }
    printf("is_present: %d\n", acc_is_present(arr, sizeof arr));
    #pragma acc data create(arr[1:1])
    {
      printf("is_present: %d\n", acc_is_present(arr, sizeof arr));
    }

    // OpenACC 3.1, sec. 3.2.36 "acc_is_present", L3698-3699:
    // "If the byte length is zero, the routine returns nonzero in C/C++ or
    // .true. in Fortran if the given address is in shared memory or is present
    // at all in the current device memory."
    // OUT-CASE_IS_PRESENT_SUCCESS-HOST-NEXT: is_present: 1
    //  OUT-CASE_IS_PRESENT_SUCCESS-OFF-NEXT: is_present: 0
    printf("is_present: %d\n", acc_is_present(arr, 0));
    #pragma acc data create(arr)
    {
      // OUT-CASE_IS_PRESENT_SUCCESS-NEXT: is_present: 1
      printf("is_present: %d\n", acc_is_present(arr, 0));
    }
    #pragma acc data create(arr[0:1])
    {
      // OUT-CASE_IS_PRESENT_SUCCESS-NEXT: is_present: 1
      printf("is_present: %d\n", acc_is_present(arr, 0));
    }
    #pragma acc data create(arr[1:])
    {
      // OUT-CASE_IS_PRESENT_SUCCESS-HOST-NEXT: is_present: 1
      //  OUT-CASE_IS_PRESENT_SUCCESS-OFF-NEXT: is_present: 0
      printf("is_present: %d\n", acc_is_present(arr, 0));
    }
    #pragma acc data create(arr[0:2])
    {
      // OUT-CASE_IS_PRESENT_SUCCESS-HOST-NEXT: is_present: 1
      //  OUT-CASE_IS_PRESENT_SUCCESS-OFF-NEXT: is_present: 0
      printf("is_present: %d\n", acc_is_present(arr+2, 0));
    }

    // Check case of null pointer.
    //
    // OUT-CASE_IS_PRESENT_SUCCESS-NEXT: is_present: 1
    // OUT-CASE_IS_PRESENT_SUCCESS-NEXT: is_present: 1
    printf("is_present: %d\n", acc_is_present(NULL, 4));
    printf("is_present: %d\n", acc_is_present(NULL, 0));

    break;
  }

  case CASE_MAP_UNMAP_SUCCESS: {
    int arr[] = {10, 20};
    int *arr_dev;

    // OUT-CASE_MAP_UNMAP_SUCCESS-NEXT: arr: 0x[[#%x,ARR:]]
    printf("arr: %p\n", arr);

    // OUT-CASE_MAP_UNMAP_SUCCESS-NEXT: acc_malloc(0): (nil)
    printf("acc_malloc(0): %p\n", acc_malloc(0));

    // OpenACC 3.1, sec. 3.2.25 "acc_free", L3444-3445:
    // "If the argument is a NULL pointer, no operation is performed."
    acc_free(NULL);

    // Check acc_malloc then acc_free without mapping.
    arr_dev = acc_malloc(sizeof arr);
    if (!arr_dev) {
      fprintf(stderr, "acc_malloc failed\n");
      return 1;
    }
    // OUT-CASE_MAP_UNMAP_SUCCESS-HOST-NEXT: is_present: 1, deviceptr: 0x[[#ARR]]
    //  OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: is_present: 0, deviceptr: (nil)
    printf("is_present: %d, deviceptr: %p\n", acc_is_present(arr, sizeof arr),
           acc_deviceptr(arr));
    acc_free(arr_dev);
    // OUT-CASE_MAP_UNMAP_SUCCESS-HOST-NEXT: is_present: 1, deviceptr: 0x[[#ARR]]
    //  OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: is_present: 0, deviceptr: (nil)
    printf("is_present: %d, deviceptr: %p\n", acc_is_present(arr, sizeof arr),
           acc_deviceptr(arr));

    // Check the sequence acc_malloc, acc_map_data, acc_unmap_data, and
    // acc_free.  Between acc_map_data and acc_unmap_data, both the structured
    // reference counter and dynamic reference counters (via enter data) are
    // non-zero but return to zero.
    arr_dev = acc_malloc(sizeof arr);
    if (!arr_dev) {
      fprintf(stderr, "acc_malloc failed\n");
      return 1;
    }
    // OUT-CASE_MAP_UNMAP_SUCCESS-NEXT: arr_dev: 0x[[#%x,ARR_DEV:]]
    printf("arr_dev: %p\n", arr_dev);
    acc_map_data(arr, arr_dev, sizeof arr);
    // OUT-CASE_MAP_UNMAP_SUCCESS-HOST-NEXT: is_present: 1, deviceptr: 0x[[#ARR]]
    //  OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: is_present: 1, deviceptr: 0x[[#ARR_DEV]]
    printf("is_present: %d, deviceptr: %p\n", acc_is_present(arr, sizeof arr),
           acc_deviceptr(arr));
    #pragma acc update device(arr)
    // OUT-CASE_MAP_UNMAP_SUCCESS-NEXT: 10
    // OUT-CASE_MAP_UNMAP_SUCCESS-NEXT: 20
    #pragma acc parallel num_gangs(1)
    for (int i = 0; i < 2; ++i) {
      printf("%d\n", arr[i]);
      arr[i] += 100;
    }
    // OUT-CASE_MAP_UNMAP_SUCCESS-HOST-NEXT: 110
    // OUT-CASE_MAP_UNMAP_SUCCESS-HOST-NEXT: 120
    //  OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: 10
    //  OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: 20
    for (int i = 0; i < 2; ++i)
      printf("%d\n", arr[i]);
    #pragma acc update self(arr)
    // OUT-CASE_MAP_UNMAP_SUCCESS-NEXT: 110
    // OUT-CASE_MAP_UNMAP_SUCCESS-NEXT: 120
    for (int i = 0; i < 2; ++i)
      printf("%d\n", arr[i]);
    #pragma acc enter data create(arr)
    #pragma acc exit data delete(arr)
    // OUT-CASE_MAP_UNMAP_SUCCESS-HOST-NEXT: is_present: 1, deviceptr: 0x[[#ARR]]
    //  OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: is_present: 1, deviceptr: 0x[[#ARR_DEV]]
    printf("is_present: %d, deviceptr: %p\n", acc_is_present(arr, sizeof arr),
           acc_deviceptr(arr));
    acc_unmap_data(arr);
    // OUT-CASE_MAP_UNMAP_SUCCESS-HOST-NEXT: is_present: 1, deviceptr: 0x[[#ARR]]
    //  OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: is_present: 0, deviceptr: (nil)
    printf("is_present: %d, deviceptr: %p\n", acc_is_present(arr, sizeof arr),
           acc_deviceptr(arr));
    // OUT-CASE_MAP_UNMAP_SUCCESS-NEXT: 110
    // OUT-CASE_MAP_UNMAP_SUCCESS-NEXT: 120
    for (int i = 0; i < 2; ++i)
      printf("%d\n", arr[i]);
    acc_free(arr_dev);
    // OUT-CASE_MAP_UNMAP_SUCCESS-HOST-NEXT: is_present: 1, deviceptr: 0x[[#ARR]]
    //  OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: is_present: 0, deviceptr: (nil)
    printf("is_present: %d, deviceptr: %p\n", acc_is_present(arr, sizeof arr),
           acc_deviceptr(arr));

    // OpenACC 3.1, sec 3.2.33 "acc_unmap_data", L3655-3656:
    // "After unmapping memory the dynamic reference count for the pointer is
    // set to zero, but no data movement will occur."
    arr_dev = acc_malloc(sizeof arr);
    if (!arr_dev) {
      fprintf(stderr, "acc_malloc failed\n");
      return 1;
    }
    // OUT-CASE_MAP_UNMAP_SUCCESS-NEXT: arr_dev: 0x[[#%x,ARR_DEV:]]
    printf("arr_dev: %p\n", arr_dev);
    acc_map_data(arr, arr_dev, sizeof arr);
    // OUT-CASE_MAP_UNMAP_SUCCESS-HOST-NEXT: is_present: 1, deviceptr: 0x[[#ARR]]
    //  OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: is_present: 1, deviceptr: 0x[[#ARR_DEV]]
    printf("is_present: %d, deviceptr: %p\n", acc_is_present(arr, sizeof arr),
           acc_deviceptr(arr));
    #pragma acc enter data create(arr)
    // OUT-CASE_MAP_UNMAP_SUCCESS-HOST-NEXT: is_present: 1, deviceptr: 0x[[#ARR]]
    //  OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: is_present: 1, deviceptr: 0x[[#ARR_DEV]]
    printf("is_present: %d, deviceptr: %p\n", acc_is_present(arr, sizeof arr),
           acc_deviceptr(arr));
    acc_unmap_data(arr);
    // OUT-CASE_MAP_UNMAP_SUCCESS-HOST-NEXT: is_present: 1, deviceptr: 0x[[#ARR]]
    //  OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: is_present: 0, deviceptr: (nil)
    printf("is_present: %d, deviceptr: %p\n", acc_is_present(arr, sizeof arr),
           acc_deviceptr(arr));
    acc_map_data(arr, arr_dev, sizeof arr);
    // OUT-CASE_MAP_UNMAP_SUCCESS-HOST-NEXT: is_present: 1, deviceptr: 0x[[#ARR]]
    //  OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: is_present: 1, deviceptr: 0x[[#ARR_DEV]]
    printf("is_present: %d, deviceptr: %p\n", acc_is_present(arr, sizeof arr),
           acc_deviceptr(arr));
    #pragma acc enter data create(arr)
    #pragma acc enter data create(arr)
    #pragma acc enter data create(arr)
    #pragma acc enter data create(arr)
    // OUT-CASE_MAP_UNMAP_SUCCESS-HOST-NEXT: is_present: 1, deviceptr: 0x[[#ARR]]
    //  OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: is_present: 1, deviceptr: 0x[[#ARR_DEV]]
    printf("is_present: %d, deviceptr: %p\n", acc_is_present(arr, sizeof arr),
           acc_deviceptr(arr));
    acc_unmap_data(arr);
    // OUT-CASE_MAP_UNMAP_SUCCESS-HOST-NEXT: is_present: 1, deviceptr: 0x[[#ARR]]
    //  OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: is_present: 0, deviceptr: (nil)
    printf("is_present: %d, deviceptr: %p\n", acc_is_present(arr, sizeof arr),
           acc_deviceptr(arr));

    // OpenACC 3.1, sec. 3.2.32 "acc_map_data", L3641-3642:
    // "Memory mapped by acc_map_data may not have the associated dynamic
    // reference count decremented to zero, except by a call to acc_unmap_data."
    acc_map_data(arr, arr_dev, sizeof arr);
    #pragma exit data delete(arr)
    #pragma exit data delete(arr)
    #pragma exit data delete(arr)
    // OUT-CASE_MAP_UNMAP_SUCCESS-HOST-NEXT: is_present: 1, deviceptr: 0x[[#ARR]]
    //  OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: is_present: 1, deviceptr: 0x[[#ARR_DEV]]
    printf("is_present: %d, deviceptr: %p\n", acc_is_present(arr, sizeof arr),
           acc_deviceptr(arr));
    acc_unmap_data(arr);
    // OUT-CASE_MAP_UNMAP_SUCCESS-HOST-NEXT: is_present: 1, deviceptr: 0x[[#ARR]]
    //  OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: is_present: 0, deviceptr: (nil)
    printf("is_present: %d, deviceptr: %p\n", acc_is_present(arr, sizeof arr),
           acc_deviceptr(arr));

    break;
  }

  // OpenACC 3.1, sec. 3.2.32 "acc_map_data", L3637-3638:
  // "It is an error to call acc_map_data for host data that is already present
  // in the current device memory."
  case CASE_MAP_SAME_HOST_AS_STRUCTURED: {
    int arr[] = {10, 20};
    int *arr_dev = acc_malloc(sizeof arr);
    if (!arr_dev) {
      fprintf(stderr, "acc_malloc failed\n");
      return 1;
    }
    #pragma acc data create(arr)
    // ERR-CASE_MAP_SAME_HOST_AS_STRUCTURED-OFF: OMP: Error #[[#]]: acc_map_data called with host pointer that is already mapped
    acc_map_data(arr, arr_dev, sizeof arr);
    break;
  }
  case CASE_MAP_SAME_HOST_AS_DYNAMIC: {
    int arr[] = {10, 20};
    int *arr_dev = acc_malloc(sizeof arr);
    if (!arr_dev) {
      fprintf(stderr, "acc_malloc failed\n");
      return 1;
    }
    #pragma acc enter data create(arr)
    // ERR-CASE_MAP_SAME_HOST_AS_DYNAMIC-OFF: OMP: Error #[[#]]: acc_map_data called with host pointer that is already mapped
    acc_map_data(arr, arr_dev, sizeof arr);
    break;
  }
  case CASE_MAP_SAME: {
    int arr[] = {10, 20};
    int *arr_dev = acc_malloc(sizeof arr);
    if (!arr_dev) {
      fprintf(stderr, "acc_malloc failed\n");
      return 1;
    }
    acc_map_data(arr, arr_dev, sizeof arr);
    // ERR-CASE_MAP_SAME-OFF: OMP: Error #[[#]]: acc_map_data called with host pointer that is already mapped
    acc_map_data(arr, arr_dev, sizeof arr);
    break;
  }
  case CASE_MAP_SAME_HOST: {
    int arr[] = {10, 20};
    int *arr_dev0 = acc_malloc(sizeof arr);
    int *arr_dev1 = acc_malloc(sizeof arr);
    if (!arr_dev0 || !arr_dev1) {
      fprintf(stderr, "acc_malloc failed\n");
      return 1;
    }
    acc_map_data(arr, arr_dev0, sizeof arr);
    // ERR-CASE_MAP_SAME_HOST-OFF: OMP: Error #[[#]]: acc_map_data called with host pointer that is already mapped
    acc_map_data(arr, arr_dev1, sizeof arr);
    break;
  }
  case CASE_MAP_HOST_EXTENDS_AFTER: {
    int arr[] = {10, 20};
    int *arr_dev0 = acc_malloc(sizeof *arr);
    int *arr_dev1 = acc_malloc(sizeof arr);
    if (!arr_dev0 || !arr_dev1) {
      fprintf(stderr, "acc_malloc failed\n");
      return 1;
    }
    acc_map_data(arr, arr_dev0, sizeof *arr);
    // ERR-CASE_MAP_HOST_EXTENDS_AFTER-OFF: OMP: Error #[[#]]: acc_map_data called with host pointer that is already mapped
    acc_map_data(arr, arr_dev1, sizeof arr);
    break;
  }
  case CASE_MAP_HOST_EXTENDS_BEFORE: {
    int arr[] = {10, 20};
    int *arr_dev0 = acc_malloc(sizeof *arr);
    int *arr_dev1 = acc_malloc(sizeof arr);
    if (!arr_dev0 || !arr_dev1) {
      fprintf(stderr, "acc_malloc failed\n");
      return 1;
    }
    acc_map_data(arr+1, arr_dev0, sizeof *arr);
    // ERR-CASE_MAP_HOST_EXTENDS_BEFORE-OFF: OMP: Error #[[#]]: acc_map_data called with host pointer that is already mapped
    acc_map_data(arr, arr_dev1, sizeof arr);
    break;
  }
  case CASE_MAP_HOST_SUBSUMES: {
    int arr[] = {10, 20, 30};
    int *arr_dev0 = acc_malloc(2 * sizeof *arr);
    int *arr_dev1 = acc_malloc(sizeof arr);
    if (!arr_dev0 || !arr_dev1) {
      fprintf(stderr, "acc_malloc failed\n");
      return 1;
    }
    acc_map_data(arr+1, arr_dev0, 2 * sizeof *arr);
    // ERR-CASE_MAP_HOST_SUBSUMES-OFF: OMP: Error #[[#]]: acc_map_data called with host pointer that is already mapped
    acc_map_data(arr, arr_dev1, sizeof arr);
    break;
  }
  case CASE_MAP_HOST_IS_SUBSUMED: {
    int arr[] = {10, 20, 30};
    int *arr_dev0 = acc_malloc(sizeof arr);
    int *arr_dev1 = acc_malloc(2 * sizeof *arr);
    if (!arr_dev0 || !arr_dev1) {
      fprintf(stderr, "acc_malloc failed\n");
      return 1;
    }
    acc_map_data(arr, arr_dev0, sizeof arr);
    // ERR-CASE_MAP_HOST_IS_SUBSUMED-OFF: OMP: Error #[[#]]: acc_map_data called with host pointer that is already mapped
    acc_map_data(arr+1, arr_dev1, 2 * sizeof *arr);
    break;
  }

  case CASE_MAP_HOST_NULL: {
    int *arr_dev = acc_malloc(sizeof *arr_dev);
    if (!arr_dev) {
      fprintf(stderr, "acc_malloc failed\n");
      return 1;
    }
    // ERR-CASE_MAP_HOST_NULL: OMP: Error #[[#]]: acc_map_data called with null host pointer
    acc_map_data(NULL, arr_dev, sizeof *arr_dev);
    break;
  }
  case CASE_MAP_DEV_NULL: {
    int arr[] = {10, 20};
    // ERR-CASE_MAP_DEV_NULL: OMP: Error #[[#]]: acc_map_data called with null device pointer
    acc_map_data(arr, NULL, sizeof arr);
    break;
  }
  case CASE_MAP_BYTES_ZERO: {
    int arr[] = {10, 20};
    int *arr_dev = acc_malloc(sizeof *arr_dev);
    if (!arr_dev) {
      fprintf(stderr, "acc_malloc failed\n");
      return 1;
    }
    // ERR-CASE_MAP_BYTES_ZERO: OMP: Error #[[#]]: acc_map_data called with zero bytes
    acc_map_data(arr, arr_dev, 0);
    break;
  }
  case CASE_MAP_ALL_NULL: {
    // ERR-CASE_MAP_ALL_NULL: OMP: Error #[[#]]: acc_map_data called with null host pointer
    acc_map_data(NULL, NULL, 0);
    break;
  }
  case CASE_UNMAP_NULL: {
    // ERR-CASE_UNMAP_NULL: OMP: Error #[[#]]: acc_unmap_data called with null pointer
    acc_unmap_data(NULL);
    break;
  }

  // OpenACC 3.1, sec. 3.2.33 "acc_unmap_data", L3653-3655:
  // "It is undefined behavior to call acc_unmap_data with a host address unless
  // that host address was mapped to device memory using acc_map_data."
  case CASE_UNMAP_UNMAPPED: {
    int arr[] = {10, 20};
    // ERR-CASE_UNMAP_UNMAPPED-OFF: OMP: Error #[[#]]: acc_unmap_data failed
    acc_unmap_data(arr);
    break;
  }
  case CASE_UNMAP_AFTER_ONLY_STRUCTURED: {
    int arr[] = {10, 20};
    #pragma acc data create(arr)
    // ERR-CASE_UNMAP_AFTER_ONLY_STRUCTURED-OFF: OMP: Error #[[#]]: acc_unmap_data failed
    acc_unmap_data(arr);
    break;
  }
  case CASE_UNMAP_AFTER_ONLY_DYNAMIC: {
    int arr[] = {10, 20};
    #pragma acc enter data create(arr)
    // ERR-CASE_UNMAP_AFTER_ONLY_DYNAMIC-OFF: OMP: Error #[[#]]: acc_unmap_data failed
    acc_unmap_data(arr);
    break;
  }

  // OpenACC 3.1, sec 3.2.33 "acc_unmap_data", L3656-3657:
  // "It is an error to call acc_unmap_data if the structured reference count
  // for the pointer is not zero."
  case CASE_UNMAP_AFTER_MAP_AND_STRUCTURED: {
    int arr[] = {10, 20};
    int *arr_dev = acc_malloc(sizeof arr);
    if (!arr_dev) {
      fprintf(stderr, "acc_malloc failed\n");
      return 1;
    }
    acc_map_data(arr, arr_dev, sizeof arr);
    #pragma acc data create(arr)
    // ERR-CASE_UNMAP_AFTER_MAP_AND_STRUCTURED-OFF: OMP: Error #[[#]]: acc_unmap_data failed
    acc_unmap_data(arr);
    break;
  }
  case CASE_UNMAP_AFTER_ALL_THREE: {
    int arr[] = {10, 20};
    int *arr_dev = acc_malloc(sizeof arr);
    if (!arr_dev) {
      fprintf(stderr, "acc_malloc failed\n");
      return 1;
    }
    acc_map_data(arr, arr_dev, sizeof arr);
    #pragma acc enter data create(arr)
    #pragma acc data create(arr)
    // ERR-CASE_UNMAP_AFTER_ALL_THREE-OFF: OMP: Error #[[#]]: acc_unmap_data failed
    acc_unmap_data(arr);
    break;
  }

  case CASE_END:
    fprintf(stderr, "unexpected CASE_END\n");
    break;
  }
  return 0;
}

// OUT-NOT: {{.}}
// An abort messages usually follows any error.
// ERR-NOT: {{(OMP:|Libomptarget)}}
