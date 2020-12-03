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
// RUN:   (case=CASE_IS_PRESENT_SUCCESS             not-if-fail=             )
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
  Macro(CASE_IS_PRESENT_SUCCESS)

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
  case CASE_IS_PRESENT_SUCCESS: {
    int arr[] = {10, 20, 30};
    int *arr_dev;

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

    // OpenACC 3.0, sec. 3.2.36 "acc_is_present", L3066-3067:
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

  case CASE_END:
    fprintf(stderr, "unexpected CASE_END\n");
    break;
  }
  return 0;
}

// OUT-NOT: {{.}}
// ERR-NOT: {{.}}
