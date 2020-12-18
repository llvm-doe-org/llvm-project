// Check data and memory management routines.

// RUN: %data tgts {
// RUN:   (run-if=                cflags=                                     tgt-host-or-off=HOST tgt-not-if-host='%not --crash' tgt-not-if-off=              )
// RUN:   (run-if=%run-if-x86_64  cflags=-fopenmp-targets=%run-x86_64-triple  tgt-host-or-off=OFF  tgt-not-if-host=               tgt-not-if-off='%not --crash')
// RUN:   (run-if=%run-if-ppc64le cflags=-fopenmp-targets=%run-ppc64le-triple tgt-host-or-off=OFF  tgt-not-if-host=               tgt-not-if-off='%not --crash')
// RUN:   (run-if=%run-if-nvptx64 cflags=-fopenmp-targets=%run-nvptx64-triple tgt-host-or-off=OFF  tgt-not-if-host=               tgt-not-if-off='%not --crash')
// RUN: }
// RUN: %data run-envs {
// RUN:   (run-env=                                  host-or-off=%[tgt-host-or-off] not-if-host=%[tgt-not-if-host] not-if-off=%[tgt-not-if-off])
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled' host-or-off=HOST               not-if-host='%not --crash'     not-if-off=                 )
// RUN: }
// RUN: %data cases {
// RUN:   (case=CASE_DEVICEPTR_SUCCESS              not-if-fail=              )
// RUN:   (case=CASE_IS_PRESENT_SUCCESS             not-if-fail=              )
// RUN:   (case=CASE_CLAUSE_LIKE_ROUTINES_SUCCESS   not-if-fail=              )
// RUN:   (case=CASE_COPYIN_EXTENDS_AFTER           not-if-fail=%[not-if-off] )
// RUN:   (case=CASE_COPYIN_EXTENDS_BEFORE          not-if-fail=%[not-if-off] )
// RUN:   (case=CASE_COPYIN_SUBSUMES                not-if-fail=%[not-if-off] )
// RUN:   (case=CASE_CREATE_EXTENDS_AFTER           not-if-fail=%[not-if-off] )
// RUN:   (case=CASE_CREATE_EXTENDS_BEFORE          not-if-fail=%[not-if-off] )
// RUN:   (case=CASE_CREATE_SUBSUMES                not-if-fail=%[not-if-off] )
// RUN:   (case=CASE_MALLOC_FREE_SUCCESS            not-if-fail=              )
// RUN:   (case=CASE_MAP_UNMAP_SUCCESS              not-if-fail=%[not-if-host])
// RUN:   (case=CASE_MAP_SAME_HOST_AS_STRUCTURED    not-if-fail='%not --crash')
// RUN:   (case=CASE_MAP_SAME_HOST_AS_DYNAMIC       not-if-fail='%not --crash')
// RUN:   (case=CASE_MAP_SAME                       not-if-fail='%not --crash')
// RUN:   (case=CASE_MAP_SAME_HOST                  not-if-fail='%not --crash')
// RUN:   (case=CASE_MAP_HOST_EXTENDS_AFTER         not-if-fail='%not --crash')
// RUN:   (case=CASE_MAP_HOST_EXTENDS_BEFORE        not-if-fail='%not --crash')
// RUN:   (case=CASE_MAP_HOST_SUBSUMES              not-if-fail='%not --crash')
// RUN:   (case=CASE_MAP_HOST_IS_SUBSUMED           not-if-fail='%not --crash')
// RUN:   (case=CASE_MAP_HOST_NULL                  not-if-fail='%not --crash')
// RUN:   (case=CASE_MAP_DEV_NULL                   not-if-fail='%not --crash')
// RUN:   (case=CASE_MAP_BYTES_ZERO                 not-if-fail='%not --crash')
// RUN:   (case=CASE_MAP_ALL_NULL                   not-if-fail='%not --crash')
// RUN:   (case=CASE_UNMAP_NULL                     not-if-fail='%not --crash')
// RUN:   (case=CASE_UNMAP_UNMAPPED                 not-if-fail='%not --crash')
// RUN:   (case=CASE_UNMAP_AFTER_ONLY_STRUCTURED    not-if-fail='%not --crash')
// RUN:   (case=CASE_UNMAP_AFTER_ONLY_DYNAMIC       not-if-fail='%not --crash')
// RUN:   (case=CASE_UNMAP_AFTER_MAP_AND_STRUCTURED not-if-fail='%not --crash')
// RUN:   (case=CASE_UNMAP_AFTER_ALL_THREE          not-if-fail='%not --crash')
// RUN: }
// RUN: echo '#define FOREACH_CASE(Macro) \' > %t-cases.h
// RUN: %for cases {
// RUN:   echo '  Macro(%[case]) \' >> %t-cases.h
// RUN: }
// RUN: echo '  /*end of FOREACH_CASE*/' >> %t-cases.h
// RUN: %for tgts {
// RUN:   %[run-if] %clang -Xclang -verify -fopenacc %acc-includes %[cflags] \
// RUN:                    -DCASES_HEADER='"%t-cases.h"' -o %t.exe %s
// RUN:   %for run-envs {
// RUN:     %for cases {
// RUN:       %[run-if] %[run-env] %[not-if-fail] %t.exe %[case] \
// RUN:                            > %t.out 2> %t.err
// RUN:       %[run-if] FileCheck \
// RUN:           -input-file %t.out %s -match-full-lines -allow-empty \
// RUN:           -check-prefixes=OUT,OUT-%[case],OUT-%[case]-%[host-or-off]
// RUN:       %[run-if] FileCheck \
// RUN:           -input-file %t.err %s -match-full-lines -allow-empty \
// RUN:           -check-prefixes=ERR,ERR-%[case],ERR-%[case]-%[host-or-off]
// RUN:     }
// RUN:   }
// RUN: }
//
// END.

// expected-no-diagnostics

#include <openacc.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include CASES_HEADER

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

bool printMap_(const char *Name, void *HostPtr, size_t Bytes) {
  bool IsPresent = acc_is_present(HostPtr, Bytes);
  printf("%s %s: %p -> %p", Name, IsPresent ? "present" : "absent",
         HostPtr, acc_deviceptr(HostPtr));
  return IsPresent;
}

void printMap(const char *Name, void *HostPtr, size_t Bytes) {
  printMap_(Name, HostPtr, Bytes);
  printf("\n");
}

void printInt(const char *Var, int *HostPtr, size_t Bytes) {
  int IsPresent = printMap_(Var, HostPtr, Bytes);
  printf(", %d", *HostPtr);
  if (IsPresent) {
    #pragma acc parallel num_gangs(1)
    printf(" -> %d", *HostPtr);
  }
  printf("\n");
}

#define PRINT_INT(Var) printInt(#Var, &(Var), sizeof (Var))

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "expected one argument\n");
    return 1;
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
    //
    // acc_deviceptr with data-clause-like routines (acc_copyin, acc_create,
    // etc.) is checked in CASE_CLAUSE_LIKE_ROUTINES_SUCCESS.

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
    //
    // acc_is_present with data-clause-like routines (acc_copyin, acc_create,
    // etc.) is checked in CASE_CLAUSE_LIKE_ROUTINES_SUCCESS.

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
    //
    // Specified array is not present.
    // OUT-CASE_IS_PRESENT_SUCCESS-HOST-NEXT: is_present: 1
    //  OUT-CASE_IS_PRESENT_SUCCESS-OFF-NEXT: is_present: 0
    printf("is_present: %d\n", acc_is_present(arr, 0));
    #pragma acc data create(arr)
    {
      // Specified array is entirely present.
      // OUT-CASE_IS_PRESENT_SUCCESS-NEXT: is_present: 1
      printf("is_present: %d\n", acc_is_present(arr, 0));
    }
    #pragma acc data create(arr[0:1])
    {
      // Only subarray of specified array is present, including start.
      // OUT-CASE_IS_PRESENT_SUCCESS-NEXT: is_present: 1
      printf("is_present: %d\n", acc_is_present(arr, 0));
    }
    #pragma acc data create(arr[1:])
    {
      // Only subarray of specified array is present, not including start.
      // OUT-CASE_IS_PRESENT_SUCCESS-HOST-NEXT: is_present: 1
      //  OUT-CASE_IS_PRESENT_SUCCESS-OFF-NEXT: is_present: 0
      printf("is_present: %d\n", acc_is_present(arr, 0));
      // Check at valid host address immediately before what's present.
      // OUT-CASE_IS_PRESENT_SUCCESS-HOST-NEXT: is_present: 1
      //  OUT-CASE_IS_PRESENT_SUCCESS-OFF-NEXT: is_present: 0
      printf("is_present: %d\n", acc_is_present(((char*)&arr[1]) - 1, 0));
    }
    #pragma acc data create(arr[0:2])
    {
      // Check at valid host address immediately after what's present.
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

  case CASE_CLAUSE_LIKE_ROUTINES_SUCCESS: {
    // These data-clause-like routines are checked with
    // acc_map_data/acc_unmap_data is checked in CASE_MAP_UNMAP_SUCCESS.

    // Check the case where both ref counts are initially zero.
    //
    // Also, check:
    // - All aliases, which shouldn't need to be checked further.
    // - Allocations and transfers by using all possible pairings of
    //   enter-data-like routines and exit-data-like routines for a single
    //   variable, including cases with just one of those.
    // - That finalize versions actually zero the dynamic reference count.
    {
      int ci    =  10;
      int cico  =  20;
      int cicof =  30;
      int cidl  =  40;
      int cidlf =  50;
      int cr    =  60;
      int crco  =  70;
      int crcof =  80;
      int crdl  =  90;
      int crdlf = 100;
      int co    = 110;
      int cof   = 120;
      int dl    = 130;
      int dlf   = 140;
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT:    ci: 0x[[#%x,CI:]]
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT:  cico: 0x[[#%x,CICO:]]
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT: cicof: 0x[[#%x,CICOF:]]
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT:  cidl: 0x[[#%x,CIDL:]]
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT: cidlf: 0x[[#%x,CIDLF:]]
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT:    cr: 0x[[#%x,CR:]]
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT:  crco: 0x[[#%x,CRCO:]]
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT: crcof: 0x[[#%x,CRCOF:]]
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT:  crdl: 0x[[#%x,CRDL:]]
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT: crdlf: 0x[[#%x,CRDLF:]]
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT:    co: 0x[[#%x,CO:]]
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT:   cof: 0x[[#%x,COF:]]
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT:    dl: 0x[[#%x,DL:]]
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT:   dlf: 0x[[#%x,DLF:]]
      printf("   ci: %p\n", &ci);
      printf(" cico: %p\n", &cico);
      printf("cicof: %p\n", &cicof);
      printf(" cidl: %p\n", &cidl);
      printf("cidlf: %p\n", &cidlf);
      printf("   cr: %p\n", &cr);
      printf(" crco: %p\n", &crco);
      printf("crcof: %p\n", &crcof);
      printf(" crdl: %p\n", &crdl);
      printf("crdlf: %p\n", &crdlf);
      printf("   co: %p\n", &co);
      printf("  cof: %p\n", &cof);
      printf("   dl: %p\n", &dl);
      printf("  dlf: %p\n", &dlf);
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT:    ci_dev: 0x[[#CI]]
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT:  cico_dev: 0x[[#CICO]]
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cicof_dev: 0x[[#CICOF]]
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT:  cidl_dev: 0x[[#CIDL]]
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cidlf_dev: 0x[[#CIDLF]]
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT:    cr_dev: 0x[[#CR]]
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT:  crco_dev: 0x[[#CRCO]]
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: crcof_dev: 0x[[#CRCOF]]
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT:  crdl_dev: 0x[[#CRDL]]
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: crdlf_dev: 0x[[#CRDLF]]
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:    ci_dev: 0x[[#%x,CI_DEV:]]
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  cico_dev: 0x[[#%x,CICO_DEV:]]
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cicof_dev: 0x[[#%x,CICOF_DEV:]]
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  cidl_dev: 0x[[#%x,CIDL_DEV:]]
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cidlf_dev: 0x[[#%x,CIDLF_DEV:]]
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:    cr_dev: 0x[[#%x,CR_DEV:]]
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  crco_dev: 0x[[#%x,CRCO_DEV:]]
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: crcof_dev: 0x[[#%x,CRCOF_DEV:]]
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  crdl_dev: 0x[[#%x,CRDL_DEV:]]
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: crdlf_dev: 0x[[#%x,CRDLF_DEV:]]
      printf("   ci_dev: %p\n", acc_copyin(&ci, sizeof ci));
      printf(" cico_dev: %p\n", acc_pcopyin(&cico, sizeof cico));
      printf("cicof_dev: %p\n", acc_present_or_copyin(&cicof, sizeof cicof));
      printf(" cidl_dev: %p\n", acc_copyin(&cidl, sizeof cidl));
      printf("cidlf_dev: %p\n", acc_pcopyin(&cidlf, sizeof cidlf));
      printf("   cr_dev: %p\n", acc_create(&cr, sizeof cr));
      printf(" crco_dev: %p\n", acc_pcreate(&crco, sizeof crco));
      printf("crcof_dev: %p\n", acc_present_or_create(&crcof, sizeof crcof));
      printf(" crdl_dev: %p\n", acc_create(&crdl, sizeof crdl));
      printf("crdlf_dev: %p\n", acc_pcreate(&crdlf, sizeof crdlf));
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT:    ci present: 0x[[#CI]]    -> 0x[[#CI]],          10 ->  10
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT:  cico present: 0x[[#CICO]]  -> 0x[[#CICO]],        20 ->  20
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cicof present: 0x[[#CICOF]] -> 0x[[#CICOF]],       30 ->  30
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT:  cidl present: 0x[[#CIDL]]  -> 0x[[#CIDL]],        40 ->  40
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cidlf present: 0x[[#CIDLF]] -> 0x[[#CIDLF]],       50 ->  50
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT:    cr present: 0x[[#CR]]    -> 0x[[#CR]],          60 ->  60
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT:  crco present: 0x[[#CRCO]]  -> 0x[[#CRCO]],        70 ->  70
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: crcof present: 0x[[#CRCOF]] -> 0x[[#CRCOF]],       80 ->  80
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT:  crdl present: 0x[[#CRDL]]  -> 0x[[#CRDL]],        90 ->  90
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: crdlf present: 0x[[#CRDLF]] -> 0x[[#CRDLF]],      100 -> 100
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT:    co present: 0x[[#CO]]    -> 0x[[#CO]],         110 -> 110
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT:   cof present: 0x[[#COF]]   -> 0x[[#COF]],        120 -> 120
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT:    dl present: 0x[[#DL]]    -> 0x[[#DL]],         130 -> 130
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT:   dlf present: 0x[[#DLF]]   -> 0x[[#DLF]],        140 -> 140
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:    ci present: 0x[[#CI]]    -> 0x[[#CI_DEV]],     10 ->  10
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  cico present: 0x[[#CICO]]  -> 0x[[#CICO_DEV]],   20 ->  20
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cicof present: 0x[[#CICOF]] -> 0x[[#CICOF_DEV]],  30 ->  30
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  cidl present: 0x[[#CIDL]]  -> 0x[[#CIDL_DEV]],   40 ->  40
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cidlf present: 0x[[#CIDLF]] -> 0x[[#CIDLF_DEV]],  50 ->  50
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:    cr present: 0x[[#CR]]    -> 0x[[#CR_DEV]],     60 -> {{.*}}
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  crco present: 0x[[#CRCO]]  -> 0x[[#CRCO_DEV]],   70 -> {{.*}}
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: crcof present: 0x[[#CRCOF]] -> 0x[[#CRCOF_DEV]],  80 -> {{.*}}
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  crdl present: 0x[[#CRDL]]  -> 0x[[#CRDL_DEV]],   90 -> {{.*}}
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: crdlf present: 0x[[#CRDLF]] -> 0x[[#CRDLF_DEV]], 100 -> {{.*}}
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:    co  absent: 0x[[#CO]]    -> (nil),            110
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:   cof  absent: 0x[[#COF]]   -> (nil),            120
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:    dl  absent: 0x[[#DL]]    -> (nil),            130
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:   dlf  absent: 0x[[#DLF]]   -> (nil),            140
      PRINT_INT(ci);
      PRINT_INT(cico);
      PRINT_INT(cicof);
      PRINT_INT(cidl);
      PRINT_INT(cidlf);
      PRINT_INT(cr);
      PRINT_INT(crco);
      PRINT_INT(crcof);
      PRINT_INT(crdl);
      PRINT_INT(crdlf);
      PRINT_INT(co);
      PRINT_INT(cof);
      PRINT_INT(dl);
      PRINT_INT(dlf);
      #pragma acc parallel num_gangs(1) \
        present(ci, cico, cicof, cidl, cidlf, cr, crco, crcof, crdl, crdlf)
      {
        ci    =  11;
        cico  =  21;
        cicof =  31;
        cidl  =  41;
        cidlf =  51;
        cr    =  61;
        crco  =  71;
        crcof =  81;
        crdl  =  91;
        crdlf = 101;
      }
      acc_copyout(&cico, sizeof cico);
      acc_copyout_finalize(&cicof, sizeof cicof);
      acc_delete(&cidl, sizeof cidl);
      acc_delete_finalize(&cidlf, sizeof cidlf);
      acc_copyout(&crco, sizeof crco);
      acc_copyout_finalize(&crcof, sizeof crcof);
      acc_delete(&crdl, sizeof crdl);
      acc_delete_finalize(&crdlf, sizeof crdlf);
      acc_copyout(&co, sizeof co);
      acc_copyout_finalize(&cof, sizeof cof);
      acc_delete(&dl, sizeof dl);
      acc_delete_finalize(&dlf, sizeof dlf);
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT:    ci present: 0x[[#CI]]    -> 0x[[#CI]],       11 ->  11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT:  cico present: 0x[[#CICO]]  -> 0x[[#CICO]],     21 ->  21
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cicof present: 0x[[#CICOF]] -> 0x[[#CICOF]],    31 ->  31
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT:  cidl present: 0x[[#CIDL]]  -> 0x[[#CIDL]],     41 ->  41
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cidlf present: 0x[[#CIDLF]] -> 0x[[#CIDLF]],    51 ->  51
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT:    cr present: 0x[[#CR]]    -> 0x[[#CR]],       61 ->  61
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT:  crco present: 0x[[#CRCO]]  -> 0x[[#CRCO]],     71 ->  71
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: crcof present: 0x[[#CRCOF]] -> 0x[[#CRCOF]],    81 ->  81
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT:  crdl present: 0x[[#CRDL]]  -> 0x[[#CRDL]],     91 ->  91
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: crdlf present: 0x[[#CRDLF]] -> 0x[[#CRDLF]],   101 -> 101
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT:    co present: 0x[[#CO]]    -> 0x[[#CO]],      110 -> 110
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT:   cof present: 0x[[#COF]]   -> 0x[[#COF]],     120 -> 120
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT:    dl present: 0x[[#DL]]    -> 0x[[#DL]],      130 -> 130
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT:   dlf present: 0x[[#DLF]]   -> 0x[[#DLF]],     140 -> 140
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:    ci present: 0x[[#CI]]    -> 0x[[#CI_DEV]],  10 ->  11
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  cico  absent: 0x[[#CICO]]  -> (nil),          21
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cicof  absent: 0x[[#CICOF]] -> (nil),          31
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  cidl  absent: 0x[[#CIDL]]  -> (nil),          40
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cidlf  absent: 0x[[#CIDLF]] -> (nil),          50
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:    cr present: 0x[[#CR]]    -> 0x[[#CR_DEV]],  60 -> {{.*}}
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  crco  absent: 0x[[#CRCO]]  -> (nil),          71
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: crcof  absent: 0x[[#CRCOF]] -> (nil),          81
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  crdl  absent: 0x[[#CRDL]]  -> (nil),          90
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: crdlf  absent: 0x[[#CRDLF]] -> (nil),          100
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:    co  absent: 0x[[#CO]]    -> (nil),          110
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:   cof  absent: 0x[[#COF]]   -> (nil),          120
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:    dl  absent: 0x[[#DL]]    -> (nil),          130
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:   dlf  absent: 0x[[#DLF]]   -> (nil),          140
      PRINT_INT(ci);
      PRINT_INT(cico);
      PRINT_INT(cicof);
      PRINT_INT(cidl);
      PRINT_INT(cidlf);
      PRINT_INT(cr);
      PRINT_INT(crco);
      PRINT_INT(crcof);
      PRINT_INT(crdl);
      PRINT_INT(crdlf);
      PRINT_INT(co);
      PRINT_INT(cof);
      PRINT_INT(dl);
      PRINT_INT(dlf);

      // Check the dynamic reference counts by seeing how many decs it takes to
      // reach zero.
      #pragma acc exit data delete(ci, cr)
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: ci present: 0x[[#CI]] -> 0x[[#CI]], 11 -> 11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cr present: 0x[[#CR]] -> 0x[[#CR]], 61 -> 61
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: ci  absent: 0x[[#CI]] -> (nil),     10
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cr  absent: 0x[[#CR]] -> (nil),     60
      PRINT_INT(ci);
      PRINT_INT(cr);
    }

    // Check the case where the structured ref count only is one: enclosed in
    // acc data.  That is, check its suppression of allocations and transfers
    // from enter-data-like and exit-data-like routines, check that those
    // routines return but don't affect the device addresses already mapped, and
    // check those routines' effect on the dynamic ref count.
    {
      int ci  = 10;
      int cr  = 20;
      int co  = 30;
      int cof = 40;
      int dl  = 50;
      int dlf = 60;
      #pragma acc data create(ci, cr, co, cof, dl, dlf)
      {
        #pragma acc parallel num_gangs(1) present(ci, cr, co, cof, dl, dlf)
        {
          ci  = 11;
          cr  = 21;
          co  = 31;
          cof = 41;
          dl  = 51;
          dlf = 61;
        }
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: ci  present: 0x[[#%x,CI:]]  -> 0x{{[^,]*}},        11 -> 11
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cr  present: 0x[[#%x,CR:]]  -> 0x{{[^,]*}},        21 -> 21
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: co  present: 0x[[#%x,CO:]]  -> 0x{{[^,]*}},        31 -> 31
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cof present: 0x[[#%x,COF:]] -> 0x{{[^,]*}},        41 -> 41
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dl  present: 0x[[#%x,DL:]]  -> 0x{{[^,]*}},        51 -> 51
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dlf present: 0x[[#%x,DLF:]] -> 0x{{[^,]*}},        61 -> 61
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: ci  present: 0x[[#%x,CI:]]  -> 0x[[#%x,CI_DEV:]],  10 -> 11
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cr  present: 0x[[#%x,CR:]]  -> 0x[[#%x,CR_DEV:]],  20 -> 21
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: co  present: 0x[[#%x,CO:]]  -> 0x[[#%x,CO_DEV:]],  30 -> 31
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cof present: 0x[[#%x,COF:]] -> 0x[[#%x,COF_DEV:]], 40 -> 41
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dl  present: 0x[[#%x,DL:]]  -> 0x[[#%x,DL_DEV:]],  50 -> 51
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dlf present: 0x[[#%x,DLF:]] -> 0x[[#%x,DLF_DEV:]], 60 -> 61
        PRINT_INT(ci);
        PRINT_INT(cr);
        PRINT_INT(co);
        PRINT_INT(cof);
        PRINT_INT(dl);
        PRINT_INT(dlf);

        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: acc_copyin of ci: 0x[[#CI]]
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: acc_create of cr: 0x[[#CR]]
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: acc_copyin of ci: 0x[[#CI_DEV]]
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: acc_create of cr: 0x[[#CR_DEV]]
        printf("acc_copyin of ci: %p\n", acc_copyin(&ci, sizeof ci));
        printf("acc_create of cr: %p\n", acc_create(&cr, sizeof cr));
        acc_copyout(&co, sizeof co);
        acc_copyout_finalize(&cof, sizeof cof);
        acc_delete(&dl, sizeof dl);
        acc_delete_finalize(&dlf, sizeof dlf);
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI]],      11 -> 11
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR]],      21 -> 21
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: co  present: 0x[[#CO]]  -> 0x[[#CO]],      31 -> 31
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cof present: 0x[[#COF]] -> 0x[[#COF]],     41 -> 41
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dl  present: 0x[[#DL]]  -> 0x[[#DL]],      51 -> 51
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dlf present: 0x[[#DLF]] -> 0x[[#DLF]],     61 -> 61
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI_DEV]],  10 -> 11
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR_DEV]],  20 -> 21
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: co  present: 0x[[#CO]]  -> 0x[[#CO_DEV]],  30 -> 31
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cof present: 0x[[#COF]] -> 0x[[#COF_DEV]], 40 -> 41
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dl  present: 0x[[#DL]]  -> 0x[[#DL_DEV]],  50 -> 51
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dlf present: 0x[[#DLF]] -> 0x[[#DLF_DEV]], 60 -> 61
        PRINT_INT(ci);
        PRINT_INT(cr);
        PRINT_INT(co);
        PRINT_INT(cof);
        PRINT_INT(dl);
        PRINT_INT(dlf);
      }

      // Check the dynamic reference counts by seeing how many decs it takes to
      // reach zero.
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI]],     11 -> 11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR]],     21 -> 21
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: co  present: 0x[[#CO]]  -> 0x[[#CO]],     31 -> 31
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cof present: 0x[[#COF]] -> 0x[[#COF]],    41 -> 41
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dl  present: 0x[[#DL]]  -> 0x[[#DL]],     51 -> 51
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dlf present: 0x[[#DLF]] -> 0x[[#DLF]],    61 -> 61
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI_DEV]], 10 -> 11
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR_DEV]], 20 -> 21
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: co  absent:  0x[[#CO]]  -> (nil),         30
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cof absent:  0x[[#COF]] -> (nil),         40
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dl  absent:  0x[[#DL]]  -> (nil),         50
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dlf absent:  0x[[#DLF]] -> (nil),         60
      PRINT_INT(ci);
      PRINT_INT(cr);
      PRINT_INT(co);
      PRINT_INT(cof);
      PRINT_INT(dl);
      PRINT_INT(dlf);
      #pragma acc exit data delete(ci, cr)
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: ci present: 0x[[#CI]] -> 0x[[#CI]], 11 -> 11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cr present: 0x[[#CR]] -> 0x[[#CR]], 21 -> 21
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: ci  absent: 0x[[#CI]] -> (nil),     10
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cr  absent: 0x[[#CR]] -> (nil),     20
      PRINT_INT(ci);
      PRINT_INT(cr);
    }

    // Repeat but where both the structured and dynamic ref counter are
    // initially one.  This checks cases like decrementing the dynamic ref count
    // from one to zero while the structured ref count is one to be sure that
    // doesn't trigger deallocations or transfers.
    {
      int ci  = 10;
      int cr  = 20;
      int co  = 30;
      int cof = 40;
      int dl  = 50;
      int dlf = 60;
      #pragma acc enter data create(ci, cr, co, cof, dl, dlf)
      #pragma acc data create(ci, cr, co, cof, dl, dlf)
      {
        #pragma acc parallel num_gangs(1) present(ci, cr, co, cof, dl, dlf)
        {
          ci  = 11;
          cr  = 21;
          co  = 31;
          cof = 41;
          dl  = 51;
          dlf = 61;
        }
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: ci  present: 0x[[#%x,CI:]]  -> 0x{{[^,]*}},        11 -> 11
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cr  present: 0x[[#%x,CR:]]  -> 0x{{[^,]*}},        21 -> 21
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: co  present: 0x[[#%x,CO:]]  -> 0x{{[^,]*}},        31 -> 31
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cof present: 0x[[#%x,COF:]] -> 0x{{[^,]*}},        41 -> 41
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dl  present: 0x[[#%x,DL:]]  -> 0x{{[^,]*}},        51 -> 51
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dlf present: 0x[[#%x,DLF:]] -> 0x{{[^,]*}},        61 -> 61
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: ci  present: 0x[[#%x,CI:]]  -> 0x[[#%x,CI_DEV:]],  10 -> 11
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cr  present: 0x[[#%x,CR:]]  -> 0x[[#%x,CR_DEV:]],  20 -> 21
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: co  present: 0x[[#%x,CO:]]  -> 0x[[#%x,CO_DEV:]],  30 -> 31
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cof present: 0x[[#%x,COF:]] -> 0x[[#%x,COF_DEV:]], 40 -> 41
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dl  present: 0x[[#%x,DL:]]  -> 0x[[#%x,DL_DEV:]],  50 -> 51
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dlf present: 0x[[#%x,DLF:]] -> 0x[[#%x,DLF_DEV:]], 60 -> 61
        PRINT_INT(ci);
        PRINT_INT(cr);
        PRINT_INT(co);
        PRINT_INT(cof);
        PRINT_INT(dl);
        PRINT_INT(dlf);

        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: acc_copyin of ci: 0x[[#CI]]
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: acc_create of cr: 0x[[#CR]]
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: acc_copyin of ci: 0x[[#CI_DEV]]
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: acc_create of cr: 0x[[#CR_DEV]]
        printf("acc_copyin of ci: %p\n", acc_copyin(&ci, sizeof ci));
        printf("acc_create of cr: %p\n", acc_create(&cr, sizeof cr));
        acc_copyout(&co, sizeof co);
        acc_copyout_finalize(&cof, sizeof cof);
        acc_delete(&dl, sizeof dl);
        acc_delete_finalize(&dlf, sizeof dlf);
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI]],      11 -> 11
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR]],      21 -> 21
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: co  present: 0x[[#CO]]  -> 0x[[#CO]],      31 -> 31
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cof present: 0x[[#COF]] -> 0x[[#COF]],     41 -> 41
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dl  present: 0x[[#DL]]  -> 0x[[#DL]],      51 -> 51
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dlf present: 0x[[#DLF]] -> 0x[[#DLF]],     61 -> 61
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI_DEV]],  10 -> 11
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR_DEV]],  20 -> 21
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: co  present: 0x[[#CO]]  -> 0x[[#CO_DEV]],  30 -> 31
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cof present: 0x[[#COF]] -> 0x[[#COF_DEV]], 40 -> 41
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dl  present: 0x[[#DL]]  -> 0x[[#DL_DEV]],  50 -> 51
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dlf present: 0x[[#DLF]] -> 0x[[#DLF_DEV]], 60 -> 61
        PRINT_INT(ci);
        PRINT_INT(cr);
        PRINT_INT(co);
        PRINT_INT(cof);
        PRINT_INT(dl);
        PRINT_INT(dlf);
      }

      // Check the dynamic reference counts by seeing how many decs it takes to
      // reach zero.
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI]],     11 -> 11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR]],     21 -> 21
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: co  present: 0x[[#CO]]  -> 0x[[#CO]],     31 -> 31
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cof present: 0x[[#COF]] -> 0x[[#COF]],    41 -> 41
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dl  present: 0x[[#DL]]  -> 0x[[#DL]],     51 -> 51
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dlf present: 0x[[#DLF]] -> 0x[[#DLF]],    61 -> 61
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI_DEV]], 10 -> 11
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR_DEV]], 20 -> 21
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: co  absent:  0x[[#CO]]  -> (nil),         30
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cof absent:  0x[[#COF]] -> (nil),         40
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dl  absent:  0x[[#DL]]  -> (nil),         50
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dlf absent:  0x[[#DLF]] -> (nil),         60
      PRINT_INT(ci);
      PRINT_INT(cr);
      PRINT_INT(co);
      PRINT_INT(cof);
      PRINT_INT(dl);
      PRINT_INT(dlf);
      #pragma acc exit data delete(ci, cr)
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI]],     11 -> 11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR]],     21 -> 21
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI_DEV]], 10 -> 11
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR_DEV]], 20 -> 21
      PRINT_INT(ci);
      PRINT_INT(cr);
      #pragma acc exit data delete(ci, cr)
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: ci present: 0x[[#CI]] -> 0x[[#CI]], 11 -> 11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cr present: 0x[[#CR]] -> 0x[[#CR]], 21 -> 21
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: ci  absent: 0x[[#CI]] -> (nil),     10
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cr  absent: 0x[[#CR]] -> (nil),     20
      PRINT_INT(ci);
      PRINT_INT(cr);
    }

    // Repeat but where the dynamic ref counter only is initially one.
    {
      int ci  = 10;
      int cr  = 20;
      int co  = 30;
      int cof = 40;
      int dl  = 50;
      int dlf = 60;
      #pragma acc enter data create(ci, cr, co, cof, dl, dlf)
      #pragma acc parallel num_gangs(1) present(ci, cr, co, cof, dl, dlf)
      {
        ci  = 11;
        cr  = 21;
        co  = 31;
        cof = 41;
        dl  = 51;
        dlf = 61;
      }
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: ci  present: 0x[[#%x,CI:]]  -> 0x{{[^,]*}},        11 -> 11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cr  present: 0x[[#%x,CR:]]  -> 0x{{[^,]*}},        21 -> 21
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: co  present: 0x[[#%x,CO:]]  -> 0x{{[^,]*}},        31 -> 31
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cof present: 0x[[#%x,COF:]] -> 0x{{[^,]*}},        41 -> 41
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dl  present: 0x[[#%x,DL:]]  -> 0x{{[^,]*}},        51 -> 51
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dlf present: 0x[[#%x,DLF:]] -> 0x{{[^,]*}},        61 -> 61
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: ci  present: 0x[[#%x,CI:]]  -> 0x[[#%x,CI_DEV:]],  10 -> 11
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cr  present: 0x[[#%x,CR:]]  -> 0x[[#%x,CR_DEV:]],  20 -> 21
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: co  present: 0x[[#%x,CO:]]  -> 0x[[#%x,CO_DEV:]],  30 -> 31
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cof present: 0x[[#%x,COF:]] -> 0x[[#%x,COF_DEV:]], 40 -> 41
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dl  present: 0x[[#%x,DL:]]  -> 0x[[#%x,DL_DEV:]],  50 -> 51
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dlf present: 0x[[#%x,DLF:]] -> 0x[[#%x,DLF_DEV:]], 60 -> 61
      PRINT_INT(ci);
      PRINT_INT(cr);
      PRINT_INT(co);
      PRINT_INT(cof);
      PRINT_INT(dl);
      PRINT_INT(dlf);

      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: acc_copyin of ci: 0x[[#CI]]
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: acc_create of cr: 0x[[#CR]]
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: acc_copyin of ci: 0x[[#CI_DEV]]
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: acc_create of cr: 0x[[#CR_DEV]]
      printf("acc_copyin of ci: %p\n", acc_copyin(&ci, sizeof ci));
      printf("acc_create of cr: %p\n", acc_create(&cr, sizeof cr));
      acc_copyout(&co, sizeof co);
      acc_copyout_finalize(&cof, sizeof cof);
      acc_delete(&dl, sizeof dl);
      acc_delete_finalize(&dlf, sizeof dlf);
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI]],     11 -> 11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR]],     21 -> 21
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: co  present: 0x[[#CO]]  -> 0x[[#CO]],     31 -> 31
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cof present: 0x[[#COF]] -> 0x[[#COF]],    41 -> 41
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dl  present: 0x[[#DL]]  -> 0x[[#DL]],     51 -> 51
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dlf present: 0x[[#DLF]] -> 0x[[#DLF]],    61 -> 61
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI_DEV]], 10 -> 11
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR_DEV]], 20 -> 21
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: co  absent:  0x[[#CO]]  -> (nil),         31
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cof absent:  0x[[#COF]] -> (nil),         41
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dl  absent:  0x[[#DL]]  -> (nil),         50
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dlf absent:  0x[[#DLF]] -> (nil),         60
      PRINT_INT(ci);
      PRINT_INT(cr);
      PRINT_INT(co);
      PRINT_INT(cof);
      PRINT_INT(dl);
      PRINT_INT(dlf);

      // Check the remaining dynamic reference counts by seeing how many decs it
      // takes to reach zero.
      #pragma acc exit data delete(ci, cr)
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: ci present: 0x[[#CI]] -> 0x[[#CI]],     11 -> 11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cr present: 0x[[#CR]] -> 0x[[#CR]],     21 -> 21
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: ci present: 0x[[#CI]] -> 0x[[#CI_DEV]], 10 -> 11
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cr present: 0x[[#CR]] -> 0x[[#CR_DEV]], 20 -> 21
      PRINT_INT(ci);
      PRINT_INT(cr);
      #pragma acc exit data delete(ci, cr)
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: ci present: 0x[[#CI]] -> 0x[[#CI]], 11 -> 11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cr present: 0x[[#CR]] -> 0x[[#CR]], 21 -> 21
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: ci  absent: 0x[[#CI]] -> (nil),     10
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cr  absent: 0x[[#CR]] -> (nil),     20
      PRINT_INT(ci);
      PRINT_INT(cr);
    }

    // Repeat but where the dynamic ref counter only is initially two.  The
    // difference between this and the previous case checks the effect of
    // _finalize.
    {
      int ci  = 10;
      int cr  = 20;
      int co  = 30;
      int cof = 40;
      int dl  = 50;
      int dlf = 60;
      #pragma acc enter data create(ci, cr, co, cof, dl, dlf)
      #pragma acc enter data create(ci, cr, co, cof, dl, dlf)
      #pragma acc parallel num_gangs(1) present(ci, cr, co, cof, dl, dlf)
      {
        ci  = 11;
        cr  = 21;
        co  = 31;
        cof = 41;
        dl  = 51;
        dlf = 61;
      }
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: ci  present: 0x[[#%x,CI:]]  -> 0x{{[^,]*}},        11 -> 11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cr  present: 0x[[#%x,CR:]]  -> 0x{{[^,]*}},        21 -> 21
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: co  present: 0x[[#%x,CO:]]  -> 0x{{[^,]*}},        31 -> 31
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cof present: 0x[[#%x,COF:]] -> 0x{{[^,]*}},        41 -> 41
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dl  present: 0x[[#%x,DL:]]  -> 0x{{[^,]*}},        51 -> 51
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dlf present: 0x[[#%x,DLF:]] -> 0x{{[^,]*}},        61 -> 61
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: ci  present: 0x[[#%x,CI:]]  -> 0x[[#%x,CI_DEV:]],  10 -> 11
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cr  present: 0x[[#%x,CR:]]  -> 0x[[#%x,CR_DEV:]],  20 -> 21
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: co  present: 0x[[#%x,CO:]]  -> 0x[[#%x,CO_DEV:]],  30 -> 31
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cof present: 0x[[#%x,COF:]] -> 0x[[#%x,COF_DEV:]], 40 -> 41
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dl  present: 0x[[#%x,DL:]]  -> 0x[[#%x,DL_DEV:]],  50 -> 51
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dlf present: 0x[[#%x,DLF:]] -> 0x[[#%x,DLF_DEV:]], 60 -> 61
      PRINT_INT(ci);
      PRINT_INT(cr);
      PRINT_INT(co);
      PRINT_INT(cof);
      PRINT_INT(dl);
      PRINT_INT(dlf);

      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: acc_copyin of ci: 0x[[#CI]]
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: acc_create of cr: 0x[[#CR]]
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: acc_copyin of ci: 0x[[#CI_DEV]]
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: acc_create of cr: 0x[[#CR_DEV]]
      printf("acc_copyin of ci: %p\n", acc_copyin(&ci, sizeof ci));
      printf("acc_create of cr: %p\n", acc_create(&cr, sizeof cr));
      acc_copyout(&co, sizeof co);
      acc_copyout_finalize(&cof, sizeof cof);
      acc_delete(&dl, sizeof dl);
      acc_delete_finalize(&dlf, sizeof dlf);
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI]],     11 -> 11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR]],     21 -> 21
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: co  present: 0x[[#CO]]  -> 0x[[#CO]],     31 -> 31
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cof present: 0x[[#COF]] -> 0x[[#COF]],    41 -> 41
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dl  present: 0x[[#DL]]  -> 0x[[#DL]],     51 -> 51
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dlf present: 0x[[#DLF]] -> 0x[[#DLF]],    61 -> 61
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI_DEV]], 10 -> 11
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR_DEV]], 20 -> 21
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: co  present: 0x[[#CO]]  -> 0x[[#CO_DEV]], 30 -> 31
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cof absent:  0x[[#COF]] -> (nil),         41
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dl  present: 0x[[#DL]]  -> 0x[[#DL_DEV]], 50 -> 51
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dlf absent:  0x[[#DLF]] -> (nil),         60
      PRINT_INT(ci);
      PRINT_INT(cr);
      PRINT_INT(co);
      PRINT_INT(cof);
      PRINT_INT(dl);
      PRINT_INT(dlf);

      // Check the remaining dynamic reference counts by seeing how many decs it
      // takes to reach zero.
      #pragma acc exit data delete(co, dl)
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: co present: 0x[[#CO]] -> 0x[[#CO]], 31 -> 31
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dl present: 0x[[#DL]] -> 0x[[#DL]], 51 -> 51
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: co  absent: 0x[[#CO]] -> (nil),     30
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dl  absent: 0x[[#DL]] -> (nil),     50
      PRINT_INT(co);
      PRINT_INT(dl);
      #pragma acc exit data delete(ci, cr)
      #pragma acc exit data delete(ci, cr)
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: ci present: 0x[[#CI]] -> 0x[[#CI]],     11 -> 11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cr present: 0x[[#CR]] -> 0x[[#CR]],     21 -> 21
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: ci present: 0x[[#CI]] -> 0x[[#CI_DEV]], 10 -> 11
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cr present: 0x[[#CR]] -> 0x[[#CR_DEV]], 20 -> 21
      PRINT_INT(ci);
      PRINT_INT(cr);
      #pragma acc exit data delete(ci, cr)
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: ci present: 0x[[#CI]] -> 0x[[#CI]], 11 -> 11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: cr present: 0x[[#CR]] -> 0x[[#CR]], 21 -> 21
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: ci  absent: 0x[[#CI]] -> (nil),     10
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: cr  absent: 0x[[#CR]] -> (nil),     20
      PRINT_INT(ci);
      PRINT_INT(cr);
    }

    // Check case of zero bytes.
    {
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT: x: 0x[[#%x,X:]]
      int x = 10;
      printf("x: %p\n", &x);

      // When data is absent.
      //
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: acc_copyin(&x, 0): (nil)
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]], 10 -> 10
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: acc_create(&x, 0): (nil)
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]], 10 -> 10
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]], 10 -> 10
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]], 10 -> 10
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]], 10 -> 10
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]], 10 -> 10
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  acc_copyin(&x, 0): (nil)
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  x  absent: 0x[[#X]] -> (nil),    10
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  acc_create(&x, 0): (nil)
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  x  absent: 0x[[#X]] -> (nil),    10
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  x  absent: 0x[[#X]] -> (nil),    10
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  x  absent: 0x[[#X]] -> (nil),    10
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  x  absent: 0x[[#X]] -> (nil),    10
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  x  absent: 0x[[#X]] -> (nil),    10
      printf("acc_copyin(&x, 0): %p\n", acc_copyin(&x, 0));
      PRINT_INT(x);
      printf("acc_create(&x, 0): %p\n", acc_create(&x, 0));
      PRINT_INT(x);
      acc_copyout(&x, 0);
      PRINT_INT(x);
      acc_copyout_finalize(&x, 0);
      PRINT_INT(x);
      acc_delete(&x, 0);
      PRINT_INT(x);
      acc_delete_finalize(&x, 0);
      PRINT_INT(x);

      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]],         11 -> 11
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: x present: 0x[[#X]] -> 0x[[#%x,X_DEV:]], 10 -> {{.*}}
      #pragma acc enter data create(x)
      #pragma acc parallel num_gangs(1) present(x)
      x = 11;
      PRINT_INT(x);

      // When data is present.
      //
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: acc_copyin(&x, 0): (nil)
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]],     11 -> 11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: acc_create(&x, 0): (nil)
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]],     11 -> 11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]],     11 -> 11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]],     11 -> 11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]],     11 -> 11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]],     11 -> 11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  acc_copyin(&x, 0): (nil)
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  x present: 0x[[#X]] -> 0x[[#X_DEV]], 10 -> 11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  acc_create(&x, 0): (nil)
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  x present: 0x[[#X]] -> 0x[[#X_DEV]], 10 -> 11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  x present: 0x[[#X]] -> 0x[[#X_DEV]], 10 -> 11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  x present: 0x[[#X]] -> 0x[[#X_DEV]], 10 -> 11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  x present: 0x[[#X]] -> 0x[[#X_DEV]], 10 -> 11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  x present: 0x[[#X]] -> 0x[[#X_DEV]], 10 -> 11
      printf("acc_copyin(&x, 0): %p\n", acc_copyin(&x, 0));
      PRINT_INT(x);
      printf("acc_create(&x, 0): %p\n", acc_create(&x, 0));
      PRINT_INT(x);
      acc_copyout(&x, 0);
      PRINT_INT(x);
      acc_copyout_finalize(&x, 0);
      PRINT_INT(x);
      acc_delete(&x, 0);
      PRINT_INT(x);
      acc_delete_finalize(&x, 0);
      PRINT_INT(x);

      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]], 11 -> 11
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT:  x  absent: 0x[[#X]] -> (nil),    10
      #pragma acc exit data delete(x)
      PRINT_INT(x);
    }

    // Check case of null pointer, with non-zero or zero bytes.
    //
    // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT: acc_copyin(NULL, 1): (nil)
    // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT: NULL present: (nil) -> (nil)
    // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT: acc_create(NULL, 1): (nil)
    // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT: NULL present: (nil) -> (nil)
    // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT: NULL present: (nil) -> (nil)
    // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT: NULL present: (nil) -> (nil)
    // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT: NULL present: (nil) -> (nil)
    // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT: NULL present: (nil) -> (nil)
    // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT: acc_copyin(NULL, 0): (nil)
    // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT: NULL present: (nil) -> (nil)
    // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT: acc_create(NULL, 0): (nil)
    // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT: NULL present: (nil) -> (nil)
    // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT: NULL present: (nil) -> (nil)
    // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT: NULL present: (nil) -> (nil)
    // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT: NULL present: (nil) -> (nil)
    // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT: NULL present: (nil) -> (nil)
    printf("acc_copyin(NULL, 1): %p\n", acc_copyin(NULL, 1));
    printMap("NULL", NULL, 1);
    printf("acc_create(NULL, 1): %p\n", acc_create(NULL, 1));
    printMap("NULL", NULL, 1);
    acc_copyout(NULL, 1);
    printMap("NULL", NULL, 1);
    acc_copyout_finalize(NULL, 1);
    printMap("NULL", NULL, 1);
    acc_delete(NULL, 1);
    printMap("NULL", NULL, 1);
    acc_delete_finalize(NULL, 1);
    printMap("NULL", NULL, 1);
    printf("acc_copyin(NULL, 0): %p\n", acc_copyin(NULL, 0));
    printMap("NULL", NULL, 0);
    printf("acc_create(NULL, 0): %p\n", acc_create(NULL, 0));
    printMap("NULL", NULL, 0);
    acc_copyout(NULL, 0);
    printMap("NULL", NULL, 0);
    acc_copyout_finalize(NULL, 0);
    printMap("NULL", NULL, 0);
    acc_delete(NULL, 0);
    printMap("NULL", NULL, 0);
    acc_delete_finalize(NULL, 0);
    printMap("NULL", NULL, 0);

    // Check action suppression and dynamic reference counting in the case of
    // subarrays.  Actions for the same subarray or a subset should have no
    // effect except to adjust the dynamic reference count.
    {
      int dyn[4] = {10, 20, 30, 40};
      int str[4] = {50, 60, 70, 80};
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT: dyn element size: [[#%u,DYN_ELE_SIZE:]]
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT: str element size: [[#%u,STR_ELE_SIZE:]]
      printf("dyn element size: %zu\n", sizeof *dyn);
      printf("str element size: %zu\n", sizeof *str);

      #pragma acc enter data create(dyn[1:2])
      #pragma acc data create(str[1:2])
      {
        #pragma acc parallel num_gangs(1) present(dyn[1:2], str[1:2])
        {
          dyn[1] = 21;
          dyn[2] = 31;
          str[1] = 61;
          str[2] = 71;
        }
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dyn[1] present: 0x[[#%x,DYN1:]] -> 0x{{[^,]*}},         21 -> 21
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: str[1] present: 0x[[#%x,STR1:]] -> 0x{{[^,]*}},         61 -> 61
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dyn[1] present: 0x[[#%x,DYN1:]] -> 0x[[#%x,DYN1_DEV:]], 20 -> 21
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: str[1] present: 0x[[#%x,STR1:]] -> 0x[[#%x,STR1_DEV:]], 60 -> 61
        PRINT_INT(dyn[1]);
        PRINT_INT(str[1]);
        // dyn[1:2] has dyn ref = 1, str ref = 0.
        // str[1:2] has dyn ref = 0, str ref = 1.

        acc_copyout(str + 1, 2 * sizeof *str);
        acc_copyout(str + 1, sizeof *str);
        acc_copyout(str + 2, sizeof *str);
        acc_copyout_finalize(str + 1, 2 * sizeof *str);
        acc_copyout_finalize(str + 1, sizeof *str);
        acc_copyout_finalize(str + 2, sizeof *str);
        acc_delete(str + 1, 2 * sizeof *str);
        acc_delete(str + 1, sizeof *str);
        acc_delete(str + 2, sizeof *str);
        acc_delete_finalize(str + 1, 2 * sizeof *str);
        acc_delete_finalize(str + 1, sizeof *str);
        acc_delete_finalize(str + 2, sizeof *str);
        // dyn[1:2] has dyn ref = 1, str ref = 0.
        // str[1:2] has dyn ref = 0, str ref = 1.

        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: acc_copyin of dyn[1:2]: 0x[[#DYN1]]
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: acc_copyin of str[1:2]: 0x[[#STR1]]
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: acc_create of dyn[1:2]: 0x[[#DYN1]]
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: acc_create of str[1:2]: 0x[[#STR1]]
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: acc_copyin of dyn[1:2]: 0x[[#DYN1_DEV]]
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: acc_copyin of str[1:2]: 0x[[#STR1_DEV]]
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: acc_create of dyn[1:2]: 0x[[#DYN1_DEV]]
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: acc_create of str[1:2]: 0x[[#STR1_DEV]]
        printf("acc_copyin of dyn[1:2]: %p\n",
               acc_copyin(dyn + 1, 2 * sizeof *dyn));
        printf("acc_copyin of str[1:2]: %p\n",
               acc_copyin(str + 1, 2 * sizeof *str));
        printf("acc_create of dyn[1:2]: %p\n",
               acc_create(dyn + 1, 2 * sizeof *dyn));
        printf("acc_create of str[1:2]: %p\n",
               acc_create(str + 1, 2 * sizeof *str));
        // dyn[1:2] has dyn ref = 3, str ref = 0.
        // str[1:2] has dyn ref = 2, str ref = 1.

        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: acc_copyin of dyn[2]: 0x[[#%x,DYN1 + DYN_ELE_SIZE]]
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: acc_copyin of str[2]: 0x[[#%x,STR1 + STR_ELE_SIZE]]
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: acc_create of dyn[2]: 0x[[#%x,DYN1 + DYN_ELE_SIZE]]
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: acc_create of str[2]: 0x[[#%x,STR1 + STR_ELE_SIZE]]
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: acc_copyin of dyn[2]: 0x[[#%x,DYN1_DEV + DYN_ELE_SIZE]]
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: acc_copyin of str[2]: 0x[[#%x,STR1_DEV + STR_ELE_SIZE]]
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: acc_create of dyn[2]: 0x[[#%x,DYN1_DEV + DYN_ELE_SIZE]]
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: acc_create of str[2]: 0x[[#%x,STR1_DEV + STR_ELE_SIZE]]
        printf("acc_copyin of dyn[2]: %p\n", acc_copyin(dyn + 2, sizeof *dyn));
        printf("acc_copyin of str[2]: %p\n", acc_copyin(str + 2, sizeof *str));
        printf("acc_create of dyn[2]: %p\n", acc_create(dyn + 2, sizeof *dyn));
        printf("acc_create of str[2]: %p\n", acc_create(str + 2, sizeof *str));
        // dyn[1:2] has dyn ref = 5, str ref = 0.
        // str[1:2] has dyn ref = 4, str ref = 1.

        // Don't decrement the dyn ref count to what was set by the pragmas
        // (1 for dyn, 0 for str) or we won't be able to tell if all the routine
        // calls had any net effect.
        acc_copyout(dyn + 1, 2 * sizeof *dyn);
        acc_copyout(dyn + 1, sizeof *dyn);
        acc_copyout(str + 1, 2 * sizeof *str);
        acc_delete(dyn + 2, sizeof *dyn);
        acc_delete(str + 2, sizeof *str);
        // dyn[1:2] has dyn ref = 2, str ref = 0.
        // str[1:2] has dyn ref = 2, str ref = 1.

        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dyn[0] present: 0x[[#%x,DYN1 - DYN_ELE_SIZE]]                -> 0x[[#%x,DYN1 - DYN_ELE_SIZE]],                10 -> 10
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dyn[1] present: 0x[[#%x,DYN1]]                               -> 0x[[#%x,DYN1]],                               21 -> 21
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dyn[2] present: 0x[[#%x,DYN1 + DYN_ELE_SIZE]]                -> 0x[[#%x,DYN1 + DYN_ELE_SIZE]],                31 -> 31
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dyn[3] present: 0x[[#%x,DYN1 + DYN_ELE_SIZE + DYN_ELE_SIZE]] -> 0x[[#%x,DYN1 + DYN_ELE_SIZE + DYN_ELE_SIZE]], 40 -> 40
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: str[0] present: 0x[[#%x,STR1 - STR_ELE_SIZE]]                -> 0x[[#%x,STR1 - STR_ELE_SIZE]],                50 -> 50
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: str[1] present: 0x[[#%x,STR1]]                               -> 0x[[#%x,STR1]],                               61 -> 61
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: str[2] present: 0x[[#%x,STR1 + STR_ELE_SIZE]]                -> 0x[[#%x,STR1 + STR_ELE_SIZE]],                71 -> 71
        // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: str[3] present: 0x[[#%x,STR1 + STR_ELE_SIZE + STR_ELE_SIZE]] -> 0x[[#%x,STR1 + STR_ELE_SIZE + STR_ELE_SIZE]], 80 -> 80
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dyn[0]  absent: 0x[[#%x,DYN1 - DYN_ELE_SIZE]]                -> (nil),                                        10
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dyn[1] present: 0x[[#%x,DYN1]]                               -> 0x[[#%x,DYN1_DEV]],                           20 -> 21
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dyn[2] present: 0x[[#%x,DYN1 + DYN_ELE_SIZE]]                -> 0x[[#%x,DYN1_DEV + DYN_ELE_SIZE]],            30 -> 31
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dyn[3]  absent: 0x[[#%x,DYN1 + DYN_ELE_SIZE + DYN_ELE_SIZE]] -> (nil),                                        40
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: str[0]  absent: 0x[[#%x,STR1 - STR_ELE_SIZE]]                -> (nil),                                        50
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: str[1] present: 0x[[#%x,STR1]]                               -> 0x[[#%x,STR1_DEV]],                           60 -> 61
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: str[2] present: 0x[[#%x,STR1 + STR_ELE_SIZE]]                -> 0x[[#%x,STR1_DEV + STR_ELE_SIZE]],            70 -> 71
        //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: str[3]  absent: 0x[[#%x,STR1 + STR_ELE_SIZE + STR_ELE_SIZE]] -> (nil),                                        80
        PRINT_INT(dyn[0]);
        PRINT_INT(dyn[1]);
        PRINT_INT(dyn[2]);
        PRINT_INT(dyn[3]);
        PRINT_INT(str[0]);
        PRINT_INT(str[1]);
        PRINT_INT(str[2]);
        PRINT_INT(str[3]);
      }

      // Check the remaining dynamic reference counts by seeing how many decs it
      // takes to reach zero.
      //
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dyn[0] present: 0x[[#%x,DYN1 - DYN_ELE_SIZE]]                -> 0x[[#%x,DYN1 - DYN_ELE_SIZE]],                10 -> 10
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dyn[1] present: 0x[[#%x,DYN1]]                               -> 0x[[#%x,DYN1]],                               21 -> 21
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dyn[2] present: 0x[[#%x,DYN1 + DYN_ELE_SIZE]]                -> 0x[[#%x,DYN1 + DYN_ELE_SIZE]],                31 -> 31
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dyn[3] present: 0x[[#%x,DYN1 + DYN_ELE_SIZE + DYN_ELE_SIZE]] -> 0x[[#%x,DYN1 + DYN_ELE_SIZE + DYN_ELE_SIZE]], 40 -> 40
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: str[0] present: 0x[[#%x,STR1 - STR_ELE_SIZE]]                -> 0x[[#%x,STR1 - STR_ELE_SIZE]],                50 -> 50
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: str[1] present: 0x[[#%x,STR1]]                               -> 0x[[#%x,STR1]],                               61 -> 61
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: str[2] present: 0x[[#%x,STR1 + STR_ELE_SIZE]]                -> 0x[[#%x,STR1 + STR_ELE_SIZE]],                71 -> 71
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: str[3] present: 0x[[#%x,STR1 + STR_ELE_SIZE + STR_ELE_SIZE]] -> 0x[[#%x,STR1 + STR_ELE_SIZE + STR_ELE_SIZE]], 80 -> 80
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dyn[0]  absent: 0x[[#%x,DYN1 - DYN_ELE_SIZE]]                -> (nil),                                        10
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dyn[1] present: 0x[[#%x,DYN1]]                               -> 0x[[#%x,DYN1_DEV]],                           20 -> 21
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dyn[2] present: 0x[[#%x,DYN1 + DYN_ELE_SIZE]]                -> 0x[[#%x,DYN1_DEV + DYN_ELE_SIZE]],            30 -> 31
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dyn[3]  absent: 0x[[#%x,DYN1 + DYN_ELE_SIZE + DYN_ELE_SIZE]] -> (nil),                                        40
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: str[0]  absent: 0x[[#%x,STR1 - STR_ELE_SIZE]]                -> (nil),                                        50
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: str[1] present: 0x[[#%x,STR1]]                               -> 0x[[#%x,STR1_DEV]],                           60 -> 61
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: str[2] present: 0x[[#%x,STR1 + STR_ELE_SIZE]]                -> 0x[[#%x,STR1_DEV + STR_ELE_SIZE]],            70 -> 71
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: str[3]  absent: 0x[[#%x,STR1 + STR_ELE_SIZE + STR_ELE_SIZE]] -> (nil),                                        80
      PRINT_INT(dyn[0]);
      PRINT_INT(dyn[1]);
      PRINT_INT(dyn[2]);
      PRINT_INT(dyn[3]);
      PRINT_INT(str[0]);
      PRINT_INT(str[1]);
      PRINT_INT(str[2]);
      PRINT_INT(str[3]);
      #pragma acc exit data delete(dyn[1:2], str[1:2])
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dyn[0] present: 0x[[#%x,DYN1 - DYN_ELE_SIZE]]                -> 0x[[#%x,DYN1 - DYN_ELE_SIZE]],                10 -> 10
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dyn[1] present: 0x[[#%x,DYN1]]                               -> 0x[[#%x,DYN1]],                               21 -> 21
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dyn[2] present: 0x[[#%x,DYN1 + DYN_ELE_SIZE]]                -> 0x[[#%x,DYN1 + DYN_ELE_SIZE]],                31 -> 31
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dyn[3] present: 0x[[#%x,DYN1 + DYN_ELE_SIZE + DYN_ELE_SIZE]] -> 0x[[#%x,DYN1 + DYN_ELE_SIZE + DYN_ELE_SIZE]], 40 -> 40
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: str[0] present: 0x[[#%x,STR1 - STR_ELE_SIZE]]                -> 0x[[#%x,STR1 - STR_ELE_SIZE]],                50 -> 50
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: str[1] present: 0x[[#%x,STR1]]                               -> 0x[[#%x,STR1]],                               61 -> 61
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: str[2] present: 0x[[#%x,STR1 + STR_ELE_SIZE]]                -> 0x[[#%x,STR1 + STR_ELE_SIZE]],                71 -> 71
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: str[3] present: 0x[[#%x,STR1 + STR_ELE_SIZE + STR_ELE_SIZE]] -> 0x[[#%x,STR1 + STR_ELE_SIZE + STR_ELE_SIZE]], 80 -> 80
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dyn[0]  absent: 0x[[#%x,DYN1 - DYN_ELE_SIZE]]                -> (nil),                                        10
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dyn[1] present: 0x[[#%x,DYN1]]                               -> 0x[[#%x,DYN1_DEV]],                           20 -> 21
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dyn[2] present: 0x[[#%x,DYN1 + DYN_ELE_SIZE]]                -> 0x[[#%x,DYN1_DEV + DYN_ELE_SIZE]],            30 -> 31
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dyn[3]  absent: 0x[[#%x,DYN1 + DYN_ELE_SIZE + DYN_ELE_SIZE]] -> (nil),                                        40
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: str[0]  absent: 0x[[#%x,STR1 - STR_ELE_SIZE]]                -> (nil),                                        50
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: str[1] present: 0x[[#%x,STR1]]                               -> 0x[[#%x,STR1_DEV]],                           60 -> 61
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: str[2] present: 0x[[#%x,STR1 + STR_ELE_SIZE]]                -> 0x[[#%x,STR1_DEV + STR_ELE_SIZE]],            70 -> 71
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: str[3]  absent: 0x[[#%x,STR1 + STR_ELE_SIZE + STR_ELE_SIZE]] -> (nil),                                        80
      PRINT_INT(dyn[0]);
      PRINT_INT(dyn[1]);
      PRINT_INT(dyn[2]);
      PRINT_INT(dyn[3]);
      PRINT_INT(str[0]);
      PRINT_INT(str[1]);
      PRINT_INT(str[2]);
      PRINT_INT(str[3]);
      #pragma acc exit data delete(dyn[1:2], str[1:2])
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dyn[0] present: 0x[[#%x,DYN1 - DYN_ELE_SIZE]]                -> 0x[[#%x,DYN1 - DYN_ELE_SIZE]],                10 -> 10
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dyn[1] present: 0x[[#%x,DYN1]]                               -> 0x[[#%x,DYN1]],                               21 -> 21
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dyn[2] present: 0x[[#%x,DYN1 + DYN_ELE_SIZE]]                -> 0x[[#%x,DYN1 + DYN_ELE_SIZE]],                31 -> 31
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: dyn[3] present: 0x[[#%x,DYN1 + DYN_ELE_SIZE + DYN_ELE_SIZE]] -> 0x[[#%x,DYN1 + DYN_ELE_SIZE + DYN_ELE_SIZE]], 40 -> 40
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: str[0] present: 0x[[#%x,STR1 - STR_ELE_SIZE]]                -> 0x[[#%x,STR1 - STR_ELE_SIZE]],                50 -> 50
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: str[1] present: 0x[[#%x,STR1]]                               -> 0x[[#%x,STR1]],                               61 -> 61
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: str[2] present: 0x[[#%x,STR1 + STR_ELE_SIZE]]                -> 0x[[#%x,STR1 + STR_ELE_SIZE]],                71 -> 71
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: str[3] present: 0x[[#%x,STR1 + STR_ELE_SIZE + STR_ELE_SIZE]] -> 0x[[#%x,STR1 + STR_ELE_SIZE + STR_ELE_SIZE]], 80 -> 80
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dyn[0]  absent: 0x[[#%x,DYN1 - DYN_ELE_SIZE]]                -> (nil),                                        10
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dyn[1]  absent: 0x[[#%x,DYN1]]                               -> (nil),                                        20
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dyn[2]  absent: 0x[[#%x,DYN1 + DYN_ELE_SIZE]]                -> (nil),                                        30
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: dyn[3]  absent: 0x[[#%x,DYN1 + DYN_ELE_SIZE + DYN_ELE_SIZE]] -> (nil),                                        40
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: str[0]  absent: 0x[[#%x,STR1 - STR_ELE_SIZE]]                -> (nil),                                        50
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: str[1]  absent: 0x[[#%x,STR1]]                               -> (nil),                                        60
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: str[2]  absent: 0x[[#%x,STR1 + STR_ELE_SIZE]]                -> (nil),                                        70
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: str[3]  absent: 0x[[#%x,STR1 + STR_ELE_SIZE + STR_ELE_SIZE]] -> (nil),                                        80
      PRINT_INT(dyn[0]);
      PRINT_INT(dyn[1]);
      PRINT_INT(dyn[2]);
      PRINT_INT(dyn[3]);
      PRINT_INT(str[0]);
      PRINT_INT(str[1]);
      PRINT_INT(str[2]);
      PRINT_INT(str[3]);
    }

    // Check action suppression from acc_(copyout|delete)[_finalize] when array
    // is present but specified subarray is not.  Errors from
    // acc_(copyin|create) for that case are checked in
    // CASE_COPYIN_EXTENDS_AFTER, CASE_CREATE_EXTENDS_AFTER, etc.
    {
      //      OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-NEXT: element size: [[#%u,ELE_SIZE:]]
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: arr[0] present: 0x[[#%x,ARR:]]                      -> 0x{{[^,]*}},                         10 -> 10
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: arr[1] present: 0x[[#%x,ARR + ELE_SIZE]]            -> 0x[[#%x,ARR + ELE_SIZE]],            21 -> 21
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: arr[2] present: 0x[[#%x,ARR + ELE_SIZE + ELE_SIZE]] -> 0x[[#%x,ARR + ELE_SIZE + ELE_SIZE]], 30 -> 30
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: arr[0]  absent: 0x[[#%x,ARR:]]                      -> (nil),                               10
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: arr[1] present: 0x[[#%x,ARR + ELE_SIZE]]            -> 0x[[#%x,ARR1_DEV:]],                 20 -> 21
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: arr[2]  absent: 0x[[#%x,ARR + ELE_SIZE + ELE_SIZE]] -> (nil),                               30
      int arr[] = {10, 20, 30};
      #pragma acc enter data create(arr[1:1])
      #pragma acc parallel num_gangs(1) present(arr[1:1])
      arr[1] = 21;
      printf("element size: %zu\n", sizeof *arr);
      PRINT_INT(arr[0]);
      PRINT_INT(arr[1]);
      PRINT_INT(arr[2]);

      // Extends after.
      acc_copyout(arr + 1, 2 * sizeof *arr);
      acc_copyout_finalize(arr + 1, 2 * sizeof *arr);
      acc_delete(arr + 1, 2 * sizeof *arr);
      acc_delete_finalize(arr + 1, 2 * sizeof *arr);

      // Extends before.
      acc_copyout(arr, 2 * sizeof *arr);
      acc_copyout_finalize(arr, 2 * sizeof *arr);
      acc_delete(arr, 2 * sizeof *arr);
      acc_delete_finalize(arr, 2 * sizeof *arr);

      // Subsumes.
      acc_copyout(arr, sizeof arr);
      acc_copyout_finalize(arr, sizeof arr);
      acc_delete(arr, sizeof arr);
      acc_delete_finalize(arr, sizeof arr);

      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: arr[0] present: 0x[[#%x,ARR]]                       -> 0x[[#%x,ARR]],                       10 -> 10
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: arr[1] present: 0x[[#%x,ARR + ELE_SIZE]]            -> 0x[[#%x,ARR + ELE_SIZE]],            21 -> 21
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: arr[2] present: 0x[[#%x,ARR + ELE_SIZE + ELE_SIZE]] -> 0x[[#%x,ARR + ELE_SIZE + ELE_SIZE]], 30 -> 30
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: arr[0]  absent: 0x[[#%x,ARR]]                       -> (nil),                               10
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: arr[1] present: 0x[[#%x,ARR + ELE_SIZE]]            -> 0x[[#%x,ARR1_DEV]],                  20 -> 21
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: arr[2]  absent: 0x[[#%x,ARR + ELE_SIZE + ELE_SIZE]] -> (nil),                               30
      PRINT_INT(arr[0]);
      PRINT_INT(arr[1]);
      PRINT_INT(arr[2]);

      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: arr[0] present: 0x[[#%x,ARR]]                       -> 0x[[#%x,ARR]],                       10 -> 10
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: arr[1] present: 0x[[#%x,ARR + ELE_SIZE]]            -> 0x[[#%x,ARR + ELE_SIZE]],            21 -> 21
      // OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-HOST-NEXT: arr[2] present: 0x[[#%x,ARR + ELE_SIZE + ELE_SIZE]] -> 0x[[#%x,ARR + ELE_SIZE + ELE_SIZE]], 30 -> 30
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: arr[0]  absent: 0x[[#%x,ARR]]                       -> (nil),                               10
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: arr[1]  absent: 0x[[#%x,ARR + ELE_SIZE]]            -> (nil),                               20
      //  OUT-CASE_CLAUSE_LIKE_ROUTINES_SUCCESS-OFF-NEXT: arr[2]  absent: 0x[[#%x,ARR + ELE_SIZE + ELE_SIZE]] -> (nil),                               30
      #pragma acc exit data delete(arr[1:1])
      PRINT_INT(arr[0]);
      PRINT_INT(arr[1]);
      PRINT_INT(arr[2]);
    }
    break;
  }

  case CASE_COPYIN_EXTENDS_AFTER: {
    int arr[] = {10, 20};
    #pragma acc data create(arr[0:1])
    {
      // ERR-CASE_COPYIN_EXTENDS_AFTER: 0x[[#%x,HOST:]], [[#%u,ELE_SIZE:]]
      // ERR-CASE_COPYIN_EXTENDS_AFTER-OFF-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#HOST]] ([[#ELE_SIZE + ELE_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#HOST]] ([[#ELE_SIZE]] bytes)
      // ERR-CASE_COPYIN_EXTENDS_AFTER-OFF-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
      fprintf(stderr, "%p, %ld\n", arr, sizeof *arr);
      acc_copyin(arr, sizeof arr);
    }
    break;
  }
  case CASE_COPYIN_EXTENDS_BEFORE: {
    int arr[] = {10, 20};
    #pragma acc data create(arr[1:1])
    {
      // ERR-CASE_COPYIN_EXTENDS_BEFORE: 0x[[#%x,HOST:]], [[#%u,ELE_SIZE:]]
      // ERR-CASE_COPYIN_EXTENDS_BEFORE-OFF-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#HOST]] ([[#ELE_SIZE + ELE_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#%x,HOST + ELE_SIZE]] ([[#ELE_SIZE]] bytes)
      // ERR-CASE_COPYIN_EXTENDS_BEFORE-OFF-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
      fprintf(stderr, "%p, %ld\n", arr, sizeof *arr);
      acc_copyin(arr, sizeof arr);
    }
    break;
  }
  case CASE_COPYIN_SUBSUMES: {
    int arr[] = {10, 20, 30};
    #pragma acc data create(arr[1:2])
    {
      // ERR-CASE_COPYIN_SUBSUMES: 0x[[#%x,HOST:]], [[#%u,ELE_SIZE:]]
      // ERR-CASE_COPYIN_SUBSUMES-OFF-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#HOST]] ([[#ELE_SIZE + ELE_SIZE + ELE_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#%x,HOST + ELE_SIZE]] ([[#ELE_SIZE + ELE_SIZE]] bytes)
      // ERR-CASE_COPYIN_SUBSUMES-OFF-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
      fprintf(stderr, "%p, %ld\n", arr, sizeof *arr);
      acc_copyin(arr, sizeof arr);
    }
    break;
  }

  case CASE_CREATE_EXTENDS_AFTER: {
    int arr[] = {10, 20};
    #pragma acc data create(arr[0:1])
    {
      // ERR-CASE_CREATE_EXTENDS_AFTER: 0x[[#%x,HOST:]], [[#%u,ELE_SIZE:]]
      // ERR-CASE_CREATE_EXTENDS_AFTER-OFF-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#HOST]] ([[#ELE_SIZE + ELE_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#HOST]] ([[#ELE_SIZE]] bytes)
      // ERR-CASE_CREATE_EXTENDS_AFTER-OFF-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
      fprintf(stderr, "%p, %ld\n", arr, sizeof *arr);
      acc_create(arr, sizeof arr);
    }
    break;
  }
  case CASE_CREATE_EXTENDS_BEFORE: {
    int arr[] = {10, 20};
    #pragma acc data create(arr[1:1])
    {
      // ERR-CASE_CREATE_EXTENDS_BEFORE: 0x[[#%x,HOST:]], [[#%u,ELE_SIZE:]]
      // ERR-CASE_CREATE_EXTENDS_BEFORE-OFF-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#HOST]] ([[#ELE_SIZE + ELE_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#%x,HOST + ELE_SIZE]] ([[#ELE_SIZE]] bytes)
      // ERR-CASE_CREATE_EXTENDS_BEFORE-OFF-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
      fprintf(stderr, "%p, %ld\n", arr, sizeof *arr);
      acc_create(arr, sizeof arr);
    }
    break;
  }
  case CASE_CREATE_SUBSUMES: {
    int arr[] = {10, 20, 30};
    #pragma acc data create(arr[1:2])
    {
      // ERR-CASE_CREATE_SUBSUMES: 0x[[#%x,HOST:]], [[#%u,ELE_SIZE:]]
      // ERR-CASE_CREATE_SUBSUMES-OFF-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#HOST]] ([[#ELE_SIZE + ELE_SIZE + ELE_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#%x,HOST + ELE_SIZE]] ([[#ELE_SIZE + ELE_SIZE]] bytes)
      // ERR-CASE_CREATE_SUBSUMES-OFF-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
      fprintf(stderr, "%p, %ld\n", arr, sizeof *arr);
      acc_create(arr, sizeof arr);
    }
    break;
  }

  case CASE_MALLOC_FREE_SUCCESS: {
    // OUT-CASE_MALLOC_FREE_SUCCESS-NEXT: acc_malloc(0): (nil)
    printf("acc_malloc(0): %p\n", acc_malloc(0));

    // OpenACC 3.1, sec. 3.2.25 "acc_free", L3444-3445:
    // "If the argument is a NULL pointer, no operation is performed."
    acc_free(NULL);

    // OUT-CASE_MALLOC_FREE_SUCCESS-NEXT: acc_malloc(1) != NULL: 1
    void *ptr = acc_malloc(1);
    printf("acc_malloc(1) != NULL: %d\n", ptr != NULL);
    acc_free(ptr);

    // OUT-CASE_MALLOC_FREE_SUCCESS-NEXT: acc_malloc(2) != NULL: 1
    ptr = acc_malloc(2);
    printf("acc_malloc(2) != NULL: %d\n", ptr != NULL);
    acc_free(ptr);

    // acc_malloc/acc_free with acc_map_data/acc_unmap_data is checked in
    // CASE_MAP_UNMAP_SUCCESS.

    break;
  }
  case CASE_MAP_UNMAP_SUCCESS: {
    int arr[] = {10, 20};
    int *arr_dev;

    // OUT-CASE_MAP_UNMAP_SUCCESS-NEXT: arr: 0x[[#%x,ARR:]]
    // OUT-CASE_MAP_UNMAP_SUCCESS-NEXT: element size: [[#%u,ELE_SIZE:]]
    printf("arr: %p\n", arr);
    printf("element size: %lu\n", sizeof *arr);

    // Check the sequence acc_malloc, acc_map_data, acc_unmap_data, and
    // acc_free.  Make sure data assignments and transfer work correctly between
    // acc_map_data and acc_unmap_data.
    arr_dev = acc_malloc(sizeof arr);
    if (!arr_dev) {
      fprintf(stderr, "acc_malloc failed\n");
      return 1;
    }
    // OUT-CASE_MAP_UNMAP_SUCCESS-NEXT: arr_dev: 0x[[#%x,ARR_DEV:]]
    printf("arr_dev: %p\n", arr_dev);
    fflush(stdout);
    // ERR-CASE_MAP_UNMAP_SUCCESS-HOST: OMP: Error #[[#]]: acc_map_data called for shared memory
    acc_map_data(arr, arr_dev, sizeof arr);
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr[0] present: 0x[[#ARR]]               -> 0x[[#ARR_DEV]],               10 -> {{.*}}
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr[1] present: 0x[[#%x,ARR + ELE_SIZE]] -> 0x[[#%x,ARR_DEV + ELE_SIZE]], 20 -> {{.*}}
    PRINT_INT(arr[0]);
    PRINT_INT(arr[1]);
    #pragma acc update device(arr)
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr[0] present: 0x[[#ARR]]               -> 0x[[#ARR_DEV]],               10 -> 10
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr[1] present: 0x[[#%x,ARR + ELE_SIZE]] -> 0x[[#%x,ARR_DEV + ELE_SIZE]], 20 -> 20
    PRINT_INT(arr[0]);
    PRINT_INT(arr[1]);
    #pragma acc parallel num_gangs(1)
    {
      arr[0] += 1;
      arr[1] += 1;
    }
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr[0] present: 0x[[#ARR]]               -> 0x[[#ARR_DEV]],               10 -> 11
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr[1] present: 0x[[#%x,ARR + ELE_SIZE]] -> 0x[[#%x,ARR_DEV + ELE_SIZE]], 20 -> 21
    PRINT_INT(arr[0]);
    PRINT_INT(arr[1]);
    #pragma acc update self(arr)
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr[0] present: 0x[[#ARR]]               -> 0x[[#ARR_DEV]],               11 -> 11
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr[1] present: 0x[[#%x,ARR + ELE_SIZE]] -> 0x[[#%x,ARR_DEV + ELE_SIZE]], 21 -> 21
    PRINT_INT(arr[0]);
    PRINT_INT(arr[1]);
    arr[0] = 10;
    arr[1] = 20;
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr[0] present: 0x[[#ARR]]               -> 0x[[#ARR_DEV]],               10 -> 11
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr[1] present: 0x[[#%x,ARR + ELE_SIZE]] -> 0x[[#%x,ARR_DEV + ELE_SIZE]], 20 -> 21
    PRINT_INT(arr[0]);
    PRINT_INT(arr[1]);
    // Structured reference count is zero.  Dynamic reference count would be
    // zero if not for the preceding acc_map_data.
    acc_unmap_data(arr);
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr[0]  absent: 0x[[#ARR]]               -> (nil),                        10
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr[1]  absent: 0x[[#%x,ARR + ELE_SIZE]] -> (nil),                        20
    PRINT_INT(arr[0]);
    PRINT_INT(arr[1]);
    acc_free(arr_dev);
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr[0]  absent: 0x[[#ARR]]               -> (nil),                        10
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr[1]  absent: 0x[[#%x,ARR + ELE_SIZE]] -> (nil),                        20
    PRINT_INT(arr[0]);
    PRINT_INT(arr[1]);

    // OpenACC 3.1, sec 3.2.33 "acc_unmap_data", L3655-3656:
    // "After unmapping memory the dynamic reference count for the pointer is
    // set to zero, but no data movement will occur."
    //
    // Check cases where the dynamic reference count would be =1 or >1 at the
    // acc_unmap_data if not for the preceding acc_map_data.
    arr_dev = acc_malloc(sizeof arr);
    if (!arr_dev) {
      fprintf(stderr, "acc_malloc failed\n");
      return 1;
    }
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr_dev: 0x[[#%x,ARR_DEV:]]
    printf("arr_dev: %p\n", arr_dev);
    acc_map_data(arr, arr_dev, sizeof arr);
    arr[0] = 10;
    arr[1] = 20;
    #pragma acc parallel num_gangs(1)
    {
      arr[0] = 11;
      arr[1] = 21;
    }
    #pragma acc enter data create(arr)
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr[0] present: 0x[[#ARR]]               -> 0x[[#ARR_DEV]],               10 -> 11
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr[1] present: 0x[[#%x,ARR + ELE_SIZE]] -> 0x[[#%x,ARR_DEV + ELE_SIZE]], 20 -> 21
    PRINT_INT(arr[0]);
    PRINT_INT(arr[1]);
    // Dynamic reference count would be 1 if not for acc_map_data.
    acc_unmap_data(arr);
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr[0]  absent: 0x[[#ARR]]               -> (nil),                        10
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr[1]  absent: 0x[[#%x,ARR + ELE_SIZE]] -> (nil),                        20
    PRINT_INT(arr[0]);
    PRINT_INT(arr[1]);
    acc_map_data(arr, arr_dev, sizeof arr);
    #pragma acc parallel num_gangs(1)
    {
      arr[0] = 11;
      arr[1] = 21;
    }
    #pragma acc enter data create(arr)
    acc_copyin(arr, sizeof arr);
    acc_create(arr, sizeof arr);
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr[0] present: 0x[[#ARR]]               -> 0x[[#ARR_DEV]],               10 -> 11
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr[1] present: 0x[[#%x,ARR + ELE_SIZE]] -> 0x[[#%x,ARR_DEV + ELE_SIZE]], 20 -> 21
    PRINT_INT(arr[0]);
    PRINT_INT(arr[1]);
    // Dynamic reference count would be > 1 if not for acc_map_data.
    acc_unmap_data(arr);
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr[0]  absent: 0x[[#ARR]]               -> (nil),                        10
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr[1]  absent: 0x[[#%x,ARR + ELE_SIZE]] -> (nil),                        20
    PRINT_INT(arr[0]);
    PRINT_INT(arr[1]);

    // OpenACC 3.1, sec. 3.2.32 "acc_map_data", L3641-3642:
    // "Memory mapped by acc_map_data may not have the associated dynamic
    // reference count decremented to zero, except by a call to acc_unmap_data."
    //
    // It's important to check all ways that the dynamic reference counter can
    // be decremented.  For example, at one time, only acc_copyout_finalize was
    // broken for this case: it mistakenly transferred data from device to host.
    acc_map_data(arr, arr_dev, sizeof arr);
    #pragma acc parallel num_gangs(1)
    {
      arr[0] = 11;
      arr[1] = 21;
    }
    #pragma exit data delete(arr)
    acc_copyout(arr, sizeof arr);
    acc_copyout_finalize(arr, sizeof arr);
    acc_delete(arr, sizeof arr);
    acc_delete_finalize(arr, sizeof arr);
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr[0] present: 0x[[#ARR]]               -> 0x[[#ARR_DEV]],               10 -> 11
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr[1] present: 0x[[#%x,ARR + ELE_SIZE]] -> 0x[[#%x,ARR_DEV + ELE_SIZE]], 20 -> 21
    PRINT_INT(arr[0]);
    PRINT_INT(arr[1]);
    acc_unmap_data(arr);
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr[0]  absent: 0x[[#ARR]]               -> (nil),                        10
    // OUT-CASE_MAP_UNMAP_SUCCESS-OFF-NEXT: arr[1]  absent: 0x[[#%x,ARR + ELE_SIZE]] -> (nil),                        20
    PRINT_INT(arr[0]);
    PRINT_INT(arr[1]);

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
    // ERR-CASE_MAP_SAME_HOST_AS_STRUCTURED-HOST: OMP: Error #[[#]]: acc_map_data called for shared memory
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
    // ERR-CASE_MAP_SAME_HOST_AS_DYNAMIC-HOST: OMP: Error #[[#]]: acc_map_data called for shared memory
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
    // ERR-CASE_MAP_SAME-HOST: OMP: Error #[[#]]: acc_map_data called for shared memory
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
    // ERR-CASE_MAP_SAME_HOST-HOST: OMP: Error #[[#]]: acc_map_data called for shared memory
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
    // ERR-CASE_MAP_HOST_EXTENDS_AFTER-HOST: OMP: Error #[[#]]: acc_map_data called for shared memory
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
    // ERR-CASE_MAP_HOST_EXTENDS_BEFORE-HOST: OMP: Error #[[#]]: acc_map_data called for shared memory
    acc_map_data(arr+1, arr_dev0, sizeof *arr);
    // ERR-CASE_MAP_HOST_EXTENDS_BEFORE-OFF: OMP: Error #[[#]]: acc_map_data called with host pointer that is already mapped
    acc_map_data(arr, arr_dev1, sizeof arr);
    break;
  }
  case CASE_MAP_HOST_SUBSUMES: {
    int arr[] = {10, 20, 30};
    int *arr_dev0 = acc_malloc(sizeof *arr);
    int *arr_dev1 = acc_malloc(sizeof arr);
    if (!arr_dev0 || !arr_dev1) {
      fprintf(stderr, "acc_malloc failed\n");
      return 1;
    }
    // ERR-CASE_MAP_HOST_SUBSUMES-HOST: OMP: Error #[[#]]: acc_map_data called for shared memory
    acc_map_data(arr+1, arr_dev0, sizeof *arr);
    // ERR-CASE_MAP_HOST_SUBSUMES-OFF: OMP: Error #[[#]]: acc_map_data called with host pointer that is already mapped
    acc_map_data(arr, arr_dev1, sizeof arr);
    break;
  }
  case CASE_MAP_HOST_IS_SUBSUMED: {
    int arr[] = {10, 20, 30};
    int *arr_dev0 = acc_malloc(sizeof arr);
    int *arr_dev1 = acc_malloc(sizeof *arr);
    if (!arr_dev0 || !arr_dev1) {
      fprintf(stderr, "acc_malloc failed\n");
      return 1;
    }
    // ERR-CASE_MAP_HOST_IS_SUBSUMED-HOST: OMP: Error #[[#]]: acc_map_data called for shared memory
    acc_map_data(arr, arr_dev0, sizeof arr);
    // ERR-CASE_MAP_HOST_IS_SUBSUMED-OFF: OMP: Error #[[#]]: acc_map_data called with host pointer that is already mapped
    acc_map_data(arr+1, arr_dev1, sizeof *arr);
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
    // ERR-CASE_UNMAP_UNMAPPED-HOST: OMP: Error #[[#]]: acc_unmap_data called for shared memory
    //  ERR-CASE_UNMAP_UNMAPPED-OFF: OMP: Error #[[#]]: acc_unmap_data failed
    acc_unmap_data(arr);
    break;
  }
  case CASE_UNMAP_AFTER_ONLY_STRUCTURED: {
    int arr[] = {10, 20};
    #pragma acc data create(arr)
    // ERR-CASE_UNMAP_AFTER_ONLY_STRUCTURED-HOST: OMP: Error #[[#]]: acc_unmap_data called for shared memory
    //  ERR-CASE_UNMAP_AFTER_ONLY_STRUCTURED-OFF: OMP: Error #[[#]]: acc_unmap_data failed
    acc_unmap_data(arr);
    break;
  }
  case CASE_UNMAP_AFTER_ONLY_DYNAMIC: {
    int arr[] = {10, 20};
    #pragma acc enter data create(arr)
    // ERR-CASE_UNMAP_AFTER_ONLY_DYNAMIC-HOST: OMP: Error #[[#]]: acc_unmap_data called for shared memory
    //  ERR-CASE_UNMAP_AFTER_ONLY_DYNAMIC-OFF: OMP: Error #[[#]]: acc_unmap_data failed
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
    // ERR-CASE_UNMAP_AFTER_MAP_AND_STRUCTURED-HOST: OMP: Error #[[#]]: acc_map_data called for shared memory
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
    // ERR-CASE_UNMAP_AFTER_ALL_THREE-HOST: OMP: Error #[[#]]: acc_map_data called for shared memory
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
