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
// RUN:   (run-env='env ACC_DEVICE_TYPE=host'        host-or-off=HOST               not-if-host='%not --crash'     not-if-off=                 )
// RUN: }
// RUN: %data cases {
// RUN:   (case=caseDeviceptrSuccess           not-if-fail=              )
// RUN:   (case=caseHostptrSuccess             not-if-fail=              )
// RUN:   (case=caseIsPresentSuccess           not-if-fail=              )
// RUN:   (case=caseEnterExitRoutinesSuccess   not-if-fail=              )
// RUN:   (case=caseCopyinExtendsAfter         not-if-fail=%[not-if-off] )
// RUN:   (case=caseCopyinExtendsBefore        not-if-fail=%[not-if-off] )
// RUN:   (case=caseCopyinSubsumes             not-if-fail=%[not-if-off] )
// RUN:   (case=caseCopyinConcat2              not-if-fail=%[not-if-off] )
// RUN:   (case=caseCreateExtendsAfter         not-if-fail=%[not-if-off] )
// RUN:   (case=caseCreateExtendsBefore        not-if-fail=%[not-if-off] )
// RUN:   (case=caseCreateSubsumes             not-if-fail=%[not-if-off] )
// RUN:   (case=caseCreateConcat2              not-if-fail=%[not-if-off] )
// RUN:   (case=caseUpdateRoutinesSuccess      not-if-fail=              )
// RUN:   (case=caseUpdateDeviceAbsent         not-if-fail=%[not-if-off] )
// RUN:   (case=caseUpdateDeviceExtendsAfter   not-if-fail=%[not-if-off] )
// RUN:   (case=caseUpdateDeviceExtendsBefore  not-if-fail=%[not-if-off] )
// RUN:   (case=caseUpdateDeviceSubsumes       not-if-fail=%[not-if-off] )
// RUN:   (case=caseUpdateDeviceConcat2        not-if-fail=%[not-if-off] )
// RUN:   (case=caseUpdateSelfAbsent           not-if-fail=%[not-if-off] )
// RUN:   (case=caseUpdateSelfExtendsAfter     not-if-fail=%[not-if-off] )
// RUN:   (case=caseUpdateSelfExtendsBefore    not-if-fail=%[not-if-off] )
// RUN:   (case=caseUpdateSelfSubsumes         not-if-fail=%[not-if-off] )
// RUN:   (case=caseUpdateSelfConcat2          not-if-fail=%[not-if-off] )
// RUN:   (case=caseMallocFreeSuccess          not-if-fail=              )
// RUN:   (case=caseMapUnmapSuccess            not-if-fail=%[not-if-host])
// RUN:   (case=caseMapBytesZero               not-if-fail=              )
// RUN:   (case=caseMapSameHostAsStructured    not-if-fail='%not --crash')
// RUN:   (case=caseMapSameHostAsDynamic       not-if-fail='%not --crash')
// RUN:   (case=caseMapSame                    not-if-fail='%not --crash')
// RUN:   (case=caseMapSameHost                not-if-fail='%not --crash')
// RUN:   (case=caseMapHostExtendsAfter        not-if-fail='%not --crash')
// RUN:   (case=caseMapHostExtendsBefore       not-if-fail='%not --crash')
// RUN:   (case=caseMapHostSubsumes            not-if-fail='%not --crash')
// RUN:   (case=caseMapHostIsSubsumed          not-if-fail='%not --crash')
// RUN:   (case=caseMapHostNull                not-if-fail='%not --crash')
// RUN:   (case=caseMapDevNull                 not-if-fail='%not --crash')
// RUN:   (case=caseMapBothNull                not-if-fail='%not --crash')
// RUN:   (case=caseUnmapNull                  not-if-fail='%not --crash')
// RUN:   (case=caseUnmapUnmapped              not-if-fail='%not --crash')
// RUN:   (case=caseUnmapAfterOnlyStructured   not-if-fail='%not --crash')
// RUN:   (case=caseUnmapAfterOnlyDynamic      not-if-fail='%not --crash')
// RUN:   (case=caseUnmapAfterMapAndStructured not-if-fail='%not --crash')
// RUN:   (case=caseUnmapAfterAllThree         not-if-fail='%not --crash')
// RUN:   (case=caseMemcpySuccess              not-if-fail=              )
// RUN:   (case=caseMemcpyToDeviceDestNull     not-if-fail='%not --crash')
// RUN:   (case=caseMemcpyToDeviceSrcNull      not-if-fail='%not --crash')
// RUN:   (case=caseMemcpyToDeviceBothNull     not-if-fail='%not --crash')
// RUN:   (case=caseMemcpyFromDeviceDestNull   not-if-fail='%not --crash')
// RUN:   (case=caseMemcpyFromDeviceSrcNull    not-if-fail='%not --crash')
// RUN:   (case=caseMemcpyFromDeviceBothNull   not-if-fail='%not --crash')
// RUN:   (case=caseMemcpyDeviceDestNull       not-if-fail='%not --crash')
// RUN:   (case=caseMemcpyDeviceSrcNull        not-if-fail='%not --crash')
// RUN:   (case=caseMemcpyDeviceBothNull       not-if-fail='%not --crash')
// RUN:   (case=caseMemcpyD2dDestNull          not-if-fail='%not --crash')
// RUN:   (case=caseMemcpyD2dSrcNull           not-if-fail='%not --crash')
// RUN:   (case=caseMemcpyD2dBothNull          not-if-fail='%not --crash')
// RUN:   (case=caseMemcpyD2dDestDevInvalid    not-if-fail='%not --crash')
// RUN:   (case=caseMemcpyD2dDestDevInvalidNeg not-if-fail='%not --crash')
// RUN:   (case=caseMemcpyD2dSrcDevInvalid     not-if-fail='%not --crash')
// RUN:   (case=caseMemcpyD2dSrcDevInvalidNeg  not-if-fail='%not --crash')
// RUN:   (case=caseMemcpyD2dBothDevInvalid    not-if-fail='%not --crash')
// RUN:   (case=caseMemcpyD2dSameDevInvalid    not-if-fail='%not --crash')
// RUN:   (case=caseMemcpyD2dDestAbsent        not-if-fail=%[not-if-off] )
// RUN:   (case=caseMemcpyD2dSrcAbsent         not-if-fail=%[not-if-off] )
// RUN:   (case=caseMemcpyD2dDestExtendsAfter  not-if-fail=%[not-if-off] )
// RUN:   (case=caseMemcpyD2dSrcExtendsAfter   not-if-fail=%[not-if-off] )
// RUN:   (case=caseMemcpyD2dDestExtendsBefore not-if-fail=%[not-if-off] )
// RUN:   (case=caseMemcpyD2dSrcExtendsBefore  not-if-fail=%[not-if-off] )
// RUN:   (case=caseMemcpyD2dDestSubsumes      not-if-fail=%[not-if-off] )
// RUN:   (case=caseMemcpyD2dSrcSubsumes       not-if-fail=%[not-if-off] )
// RUN:   (case=caseMemcpyD2dDestConcat2       not-if-fail=%[not-if-off] )
// RUN:   (case=caseMemcpyD2dSrcConcat2        not-if-fail=%[not-if-off] )
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
// RUN:                            %[host-or-off] > %t.out 2> %t.err
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
#include <stdlib.h>
#include <string.h>

// OUT-NOT: {{.}}
// ERR-NOT: {{.}}

bool printMap_(FILE *File, const char *Name, void *HostPtr, size_t Bytes) {
  bool IsPresent = acc_is_present(HostPtr, Bytes);
  void *DevPtr = acc_deviceptr(HostPtr);
  fprintf(File, "%s %s: %p -> %p", Name, IsPresent ? "present" : "absent",
          HostPtr, DevPtr);
  void *HostPtrChk = acc_hostptr(DevPtr);
  void *HostPtrExpected = DevPtr ? HostPtr : NULL;
  if (HostPtrChk != HostPtrExpected) {
    fprintf(stderr, "acc_hostptr(%p) returned %p but %p was expected\n",
            DevPtr, HostPtrChk, HostPtrExpected);
    abort();
  }
  return IsPresent;
}

void *printDevMap_(FILE *File, const char *Name, void *DevPtr, size_t Bytes) {
  void *HostPtr = acc_hostptr(DevPtr);
  fprintf(File, "%s %s: %p <- %p", Name, HostPtr ? "mapped" : "unmapped",
          DevPtr, HostPtr);
  void *DevPtrChk = acc_deviceptr(HostPtr);
  void *DevPtrExpected = HostPtr ? DevPtr : NULL;
  if (DevPtrChk != DevPtrExpected) {
    fprintf(stderr, "acc_deviceptr(%p) returned %p but %p was expected\n",
            HostPtr, DevPtrChk, DevPtrExpected);
    abort();
  }
  return HostPtr;
}

void printMap(const char *Name, void *HostPtr, size_t Bytes) {
  printMap_(stdout, Name, HostPtr, Bytes);
  fprintf(stdout, "\n");
}

void printInt(FILE *File, const char *Var, int *HostPtr) {
  int IsPresent = printMap_(File, Var, HostPtr, sizeof *HostPtr);
  fprintf(File, ", %d", *HostPtr);
  if (IsPresent) {
    int DevVal;
    #pragma acc parallel num_gangs(1) copyout(DevVal)
    DevVal = *HostPtr;
    fprintf(File, " -> %d", DevVal);
  }
  fprintf(File, "\n");
}

void printDevInt(FILE *File, const char *Var, int *DevPtr) {
  int *HostPtr = printDevMap_(File, Var, DevPtr, sizeof *DevPtr);
  fprintf(File, ", ");
  if (HostPtr) {
    int DevVal;
    #pragma acc parallel num_gangs(1) copyout(DevVal)
    DevVal = *HostPtr;
    fprintf(File, "%d <- %d", DevVal, *HostPtr);
  } else {
    int DevVal;
    acc_map_data(&DevVal, DevPtr, sizeof *DevPtr);
    #pragma acc update self(DevVal)
    acc_unmap_data(&DevVal);
    fprintf(File, "%d", DevVal);
  }
  fprintf(File, "\n");
}

#define PRINT_INT(Var) printInt(stdout, #Var, &(Var))
#define PRINT_INT_STDERR(Var) printInt(stderr, #Var, &(Var))
#define PRINT_DEV_INT(Var) printDevInt(stdout, #Var, &(Var))

static inline int getDevNum() { return acc_get_device_num(acc_device_default); }
static inline int getDevNumInvalid() {
  return acc_get_num_devices(acc_device_default);
}
static inline int getDevNumInvalidNeg() { return -1; }
static const char *deviceTypeToStr(acc_device_t DevType) {
  switch (DevType) {
#define DEVCASE(DevTypeEnum)                                                   \
  case acc_device_##DevTypeEnum:                                               \
    return "acc_device_" #DevTypeEnum;
  ACC2OMP_FOREACH_DEVICE(DEVCASE)
#undef DEVCASE
  }
}
static const char *getDevTypeStr() {
  return deviceTypeToStr(acc_get_device_type());
}

// Make each static to ensure we get a compile warning if it's never called.
#include CASES_HEADER
#define CASE(CaseName) static int CaseName(bool Offloading)
#define AddCase(CaseName) CASE(CaseName);
FOREACH_CASE(AddCase)
#undef AddCase

int main(int argc, char *argv[]) {
  CASE((*caseFn));
  if (argc != 3) {
    fprintf(stderr, "expected two arguments\n");
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
  bool Offloading;
  if (!strcmp(argv[2], "HOST"))
    Offloading = false;
  else if (!strcmp(argv[2], "OFF"))
    Offloading = true;
  else {
    fprintf(stderr, "invalid second argument: %s\n", argv[1]);
    return 1;
  }

  // OUT: start out
  // ERR: start err
  fprintf(stdout, "start out\n");
  fprintf(stderr, "start err\n");
  fflush(stdout);
  fflush(stderr);
  return caseFn(Offloading);
}

CASE(caseDeviceptrSuccess) {
  int arr[4];
  // OUT-caseDeviceptrSuccess-NEXT: arr: 0x[[#%x,ARR:]]
  // OUT-caseDeviceptrSuccess-NEXT: element size: [[#%u,ELE_SIZE:]]
  printf("arr: %p\n", arr);
  printf("element size: %zu\n", sizeof *arr);

  // acc_deviceptr with acc_map_data/acc_unmap_data is checked in
  // caseMapUnmapSuccess.
  //
  // acc_deviceptr with enter/exit-data-like routines (acc_copyin, acc_create,
  // etc.) is checked in caseEnterExitRoutinesSuccess.

  // Check with acc data.
  //
  // OUT-caseDeviceptrSuccess-HOST-NEXT: deviceptr: 0x[[#ARR]]
  //  OUT-caseDeviceptrSuccess-OFF-NEXT: deviceptr: (nil)
  printf("deviceptr: %p\n", acc_deviceptr(arr));
  #pragma acc data create(arr)
  {
    // OUT-caseDeviceptrSuccess-HOST-NEXT: deviceptr: 0x[[#ARR_DEV:ARR]]
    //  OUT-caseDeviceptrSuccess-OFF-NEXT: deviceptr: 0x[[#%x,ARR_DEV:]]
    //      OUT-caseDeviceptrSuccess-NEXT: deviceptr: 0x[[#ARR_DEV]]
    //      OUT-caseDeviceptrSuccess-NEXT: deviceptr: 0x[[#ARR_DEV]]
    printf("deviceptr: %p\n", acc_deviceptr(arr));
    #pragma acc data create(arr)
    {
      printf("deviceptr: %p\n", acc_deviceptr(arr));
    }
    printf("deviceptr: %p\n", acc_deviceptr(arr));
  }
  // OUT-caseDeviceptrSuccess-HOST-NEXT: deviceptr: 0x[[#ARR]]
  //  OUT-caseDeviceptrSuccess-OFF-NEXT: deviceptr: (nil)
  printf("deviceptr: %p\n", acc_deviceptr(arr));

  // Check with acc enter/exit data.
  //
  // OUT-caseDeviceptrSuccess-HOST-NEXT: deviceptr: 0x[[#ARR]]
  //  OUT-caseDeviceptrSuccess-OFF-NEXT: deviceptr: (nil)
  // OUT-caseDeviceptrSuccess-HOST-NEXT: deviceptr: 0x[[#ARR_DEV:ARR]]
  //  OUT-caseDeviceptrSuccess-OFF-NEXT: deviceptr: 0x[[#%x,ARR_DEV:]]
  //      OUT-caseDeviceptrSuccess-NEXT: deviceptr: 0x[[#ARR_DEV]]
  //      OUT-caseDeviceptrSuccess-NEXT: deviceptr: 0x[[#ARR_DEV]]
  // OUT-caseDeviceptrSuccess-HOST-NEXT: deviceptr: 0x[[#ARR]]
  //  OUT-caseDeviceptrSuccess-OFF-NEXT: deviceptr: (nil)
  printf("deviceptr: %p\n", acc_deviceptr(arr));
  #pragma acc enter data create(arr)
  printf("deviceptr: %p\n", acc_deviceptr(arr));
  #pragma acc enter data create(arr)
  printf("deviceptr: %p\n", acc_deviceptr(arr));
  #pragma acc exit data delete(arr)
  printf("deviceptr: %p\n", acc_deviceptr(arr));
  #pragma acc exit data delete(arr)
  printf("deviceptr: %p\n", acc_deviceptr(arr));

  // Check that the correct offset is computed when the address is within or
  // immediately beyond a larger allocation.  Check exactly one byte and one
  // byte after in case there are off-by-one errors in the implementation.
  #pragma acc data create(arr[1:])
  {
    // OUT-caseDeviceptrSuccess-HOST-NEXT: deviceptr: 0x[[#ARR_DEV0:ARR]]
    //  OUT-caseDeviceptrSuccess-OFF-NEXT: deviceptr: (nil)
    // OUT-caseDeviceptrSuccess-HOST-NEXT: deviceptr: 0x[[#%x,ARR + ELE_SIZE - 1]]
    //  OUT-caseDeviceptrSuccess-OFF-NEXT: deviceptr: (nil)
    // OUT-caseDeviceptrSuccess-HOST-NEXT: deviceptr: 0x[[#%x,ARR_DEV1:ARR_DEV0 + ELE_SIZE]]
    //  OUT-caseDeviceptrSuccess-OFF-NEXT: deviceptr: 0x[[#%x,ARR_DEV1:]]
    //      OUT-caseDeviceptrSuccess-NEXT: deviceptr: 0x[[#%x,ARR_DEV2:ARR_DEV1 + ELE_SIZE]]
    //      OUT-caseDeviceptrSuccess-NEXT: deviceptr: 0x[[#%x,ARR_DEV3:ARR_DEV2 + ELE_SIZE]]
    // OUT-caseDeviceptrSuccess-HOST-NEXT: deviceptr: 0x[[#%x,ARR_DEV4:ARR_DEV3 + ELE_SIZE]]
    //  OUT-caseDeviceptrSuccess-OFF-NEXT: deviceptr: (nil)
    printf("deviceptr: %p\n", acc_deviceptr(arr));
    printf("deviceptr: %p\n", acc_deviceptr((char *)(arr + 1) - 1));
    printf("deviceptr: %p\n", acc_deviceptr(arr + 1));
    printf("deviceptr: %p\n", acc_deviceptr(arr + 2));
    printf("deviceptr: %p\n", acc_deviceptr(arr + 3));
    printf("deviceptr: %p\n", acc_deviceptr(arr + 4));
  }

  // Check case of null pointer.
  //
  // OUT-caseDeviceptrSuccess-NEXT: deviceptr: (nil)
  printf("deviceptr: %p\n", acc_deviceptr(NULL));

  return 0;
}

CASE(caseHostptrSuccess) {
  int arr[4];
  // OUT-caseHostptrSuccess-NEXT: arr: 0x[[#%x,ARR:]]
  // OUT-caseHostptrSuccess-NEXT: element size: [[#%u,ELE_SIZE:]]
  printf("arr: %p\n", arr);
  printf("element size: %zu\n", sizeof *arr);

  // acc_hostptr with acc_map_data/acc_unmap_data is checked more thoroughly in
  // caseMapUnmapSuccess.
  //
  // acc_hostptr with enter/exit-data-like routines (acc_copyin, acc_create,
  // etc.) is checked in caseEnterExitRoutinesSuccess.

  // Check when not mapped.
  //
  //      OUT-caseHostptrSuccess-NEXT: (unmapped) arr_dev: 0x[[#%x,ARR_DEV:]]
  // OUT-caseHostptrSuccess-HOST-NEXT: (unmapped) hostptr: 0x[[#ARR_DEV]]
  //  OUT-caseHostptrSuccess-OFF-NEXT: (unmapped) hostptr: (nil)
  int *arr_dev = acc_malloc(sizeof arr);
  if (!arr_dev) {
    fprintf(stderr, "acc_malloc failed\n");
    return 1;
  }
  printf("(unmapped) arr_dev: %p\n", arr_dev);
  printf("(unmapped) hostptr: %p\n", acc_hostptr(arr_dev));
  acc_free(arr_dev);

  // Check with acc data.
  #pragma acc data create(arr)
  {
    arr_dev = acc_deviceptr(arr);
    // OUT-caseHostptrSuccess-NEXT: (acc data) hostptr: 0x[[#ARR]]
    // OUT-caseHostptrSuccess-NEXT: (acc data) hostptr: 0x[[#ARR]]
    // OUT-caseHostptrSuccess-NEXT: (acc data) hostptr: 0x[[#ARR]]
    printf("(acc data) hostptr: %p\n", acc_hostptr(arr_dev));
    #pragma acc data create(arr)
    {
      printf("(acc data) hostptr: %p\n", acc_hostptr(arr_dev));
    }
    printf("(acc data) hostptr: %p\n", acc_hostptr(arr_dev));
  }

  // Check with acc enter/exit data.
  //
  // OUT-caseHostptrSuccess-NEXT: (acc enter/exit data) hostptr: 0x[[#ARR]]
  // OUT-caseHostptrSuccess-NEXT: (acc enter/exit data) hostptr: 0x[[#ARR]]
  // OUT-caseHostptrSuccess-NEXT: (acc enter/exit data) hostptr: 0x[[#ARR]]
  #pragma acc enter data create(arr)
  arr_dev = acc_deviceptr(arr);
  printf("(acc enter/exit data) hostptr: %p\n", acc_hostptr(arr_dev));
  #pragma acc enter data create(arr)
  printf("(acc enter/exit data) hostptr: %p\n", acc_hostptr(arr_dev));
  #pragma acc exit data delete(arr)
  printf("(acc enter/exit data) hostptr: %p\n", acc_hostptr(arr_dev));
  #pragma acc exit data delete(arr)

  // Check that the correct offset is computed when the address is within or
  // immediately beyond a larger allocation.  Check exactly one byte and one
  // byte after in case there are off-by-one errors in the implementation.
  //
  // To check acc_hostptr immediately before the device mapping, we need to make
  // sure memory is allocated there.  acc_malloc plus acc_map_data is a way to
  // do that.  In the case of shared memory, acc_map_data fails but we don't
  // have to worry about the device memory not being allocated then.
  if (!Offloading) {
    arr_dev = arr;
  } else {
    arr_dev = acc_malloc(sizeof arr);
    acc_map_data(arr + 1, arr_dev + 1, sizeof arr - sizeof *arr);
  }
  // OUT-caseHostptrSuccess-HOST-NEXT: (offset) hostptr: 0x[[#%x,ARR]]
  //  OUT-caseHostptrSuccess-OFF-NEXT: (offset) hostptr: (nil)
  // OUT-caseHostptrSuccess-HOST-NEXT: (offset) hostptr: 0x[[#%x,ARR + ELE_SIZE - 1]]
  //  OUT-caseHostptrSuccess-OFF-NEXT: (offset) hostptr: (nil)
  //      OUT-caseHostptrSuccess-NEXT: (offset) hostptr: 0x[[#%x,ARR1:ARR + ELE_SIZE]]
  //      OUT-caseHostptrSuccess-NEXT: (offset) hostptr: 0x[[#%x,ARR2:ARR1 + ELE_SIZE]]
  //      OUT-caseHostptrSuccess-NEXT: (offset) hostptr: 0x[[#%x,ARR3:ARR2 + ELE_SIZE]]
  // OUT-caseHostptrSuccess-HOST-NEXT: (offset) hostptr: 0x[[#%x,ARR4:ARR3 + ELE_SIZE]]
  //  OUT-caseHostptrSuccess-OFF-NEXT: (offset) hostptr: (nil)
  printf("(offset) hostptr: %p\n", acc_hostptr(arr_dev));
  printf("(offset) hostptr: %p\n", acc_hostptr((char *)(arr_dev + 1) - 1));
  printf("(offset) hostptr: %p\n", acc_hostptr(arr_dev + 1));
  printf("(offset) hostptr: %p\n", acc_hostptr(arr_dev + 2));
  printf("(offset) hostptr: %p\n", acc_hostptr(arr_dev + 3));
  printf("(offset) hostptr: %p\n", acc_hostptr(arr_dev + 4));
  if (Offloading) {
    acc_unmap_data(arr + 1);
    acc_free(arr_dev);
  }

  // Check case of null pointer.
  //
  // OUT-caseHostptrSuccess-NEXT: (null) hostptr: (nil)
  printf("(null) hostptr: %p\n", acc_hostptr(NULL));

  return 0;
}

CASE(caseIsPresentSuccess) {
  int arr[] = {10, 20, 30};

  // acc_is_present with acc_map_data/acc_unmap_data is checked in
  // caseMapUnmapSuccess.
  //
  // acc_is_present with enter/exit-data-like routines (acc_copyin, acc_create,
  // etc.) is checked in caseEnterExitRoutinesSuccess.

  // Check with acc data.
  //
  // OUT-caseIsPresentSuccess-HOST-NEXT: is_present: 1
  //  OUT-caseIsPresentSuccess-OFF-NEXT: is_present: 0
  printf("is_present: %d\n", acc_is_present(arr, sizeof arr));
  #pragma acc data create(arr)
  {
    // OUT-caseIsPresentSuccess-NEXT: is_present: 1
    // OUT-caseIsPresentSuccess-NEXT: is_present: 1
    // OUT-caseIsPresentSuccess-NEXT: is_present: 1
    printf("is_present: %d\n", acc_is_present(arr, sizeof arr));
    #pragma acc data create(arr)
    {
      printf("is_present: %d\n", acc_is_present(arr, sizeof arr));
    }
    printf("is_present: %d\n", acc_is_present(arr, sizeof arr));
  }
  // OUT-caseIsPresentSuccess-HOST-NEXT: is_present: 1
  //  OUT-caseIsPresentSuccess-OFF-NEXT: is_present: 0
  printf("is_present: %d\n", acc_is_present(arr, sizeof arr));

  // Check with acc enter/exit data.
  //
  // OUT-caseIsPresentSuccess-HOST-NEXT: is_present: 1
  //  OUT-caseIsPresentSuccess-OFF-NEXT: is_present: 0
  //      OUT-caseIsPresentSuccess-NEXT: is_present: 1
  //      OUT-caseIsPresentSuccess-NEXT: is_present: 1
  //      OUT-caseIsPresentSuccess-NEXT: is_present: 1
  // OUT-caseIsPresentSuccess-HOST-NEXT: is_present: 1
  //  OUT-caseIsPresentSuccess-OFF-NEXT: is_present: 0
  printf("is_present: %d\n", acc_is_present(arr, sizeof arr));
  #pragma acc enter data create(arr)
  printf("is_present: %d\n", acc_is_present(arr, sizeof arr));
  #pragma acc enter data create(arr)
  printf("is_present: %d\n", acc_is_present(arr, sizeof arr));
  #pragma acc exit data delete(arr)
  printf("is_present: %d\n", acc_is_present(arr, sizeof arr));
  #pragma acc exit data delete(arr)
  printf("is_present: %d\n", acc_is_present(arr, sizeof arr));

  // Check when already present array is larger: extends after, extends before,
  // or both.
  //
  // OUT-caseIsPresentSuccess-HOST-NEXT: is_present: 1
  // OUT-caseIsPresentSuccess-HOST-NEXT: is_present: 1
  // OUT-caseIsPresentSuccess-HOST-NEXT: is_present: 1
  //  OUT-caseIsPresentSuccess-OFF-NEXT: is_present: 0
  //  OUT-caseIsPresentSuccess-OFF-NEXT: is_present: 0
  //  OUT-caseIsPresentSuccess-OFF-NEXT: is_present: 0
  printf("is_present: %d\n", acc_is_present(arr, 2 * sizeof *arr));
  printf("is_present: %d\n", acc_is_present(arr+1, 2 * sizeof *arr));
  printf("is_present: %d\n", acc_is_present(arr+1, sizeof *arr));
  #pragma acc data create(arr)
  {
    // OUT-caseIsPresentSuccess-NEXT: is_present: 1
    // OUT-caseIsPresentSuccess-NEXT: is_present: 1
    // OUT-caseIsPresentSuccess-NEXT: is_present: 1
    printf("is_present: %d\n", acc_is_present(arr, 2 * sizeof *arr));
    printf("is_present: %d\n", acc_is_present(arr+1, 2 * sizeof *arr));
    printf("is_present: %d\n", acc_is_present(arr+1, sizeof *arr));
  }

  // Check when already present array is smaller: extends after, extends before,
  // or both.
  //
  // OUT-caseIsPresentSuccess-HOST-NEXT: is_present: 1
  // OUT-caseIsPresentSuccess-HOST-NEXT: is_present: 1
  // OUT-caseIsPresentSuccess-HOST-NEXT: is_present: 1
  // OUT-caseIsPresentSuccess-HOST-NEXT: is_present: 1
  // OUT-caseIsPresentSuccess-HOST-NEXT: is_present: 1
  // OUT-caseIsPresentSuccess-HOST-NEXT: is_present: 1
  //  OUT-caseIsPresentSuccess-OFF-NEXT: is_present: 0
  //  OUT-caseIsPresentSuccess-OFF-NEXT: is_present: 0
  //  OUT-caseIsPresentSuccess-OFF-NEXT: is_present: 0
  //  OUT-caseIsPresentSuccess-OFF-NEXT: is_present: 0
  //  OUT-caseIsPresentSuccess-OFF-NEXT: is_present: 0
  //  OUT-caseIsPresentSuccess-OFF-NEXT: is_present: 0
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
  // "If the byte length is zero, the routine returns nonzero in C/C++ or .true.
  // in Fortran if the given address is in shared memory or is present at all in
  // the current device memory."
  //
  // Specified array is not present.
  // OUT-caseIsPresentSuccess-HOST-NEXT: is_present: 1
  //  OUT-caseIsPresentSuccess-OFF-NEXT: is_present: 0
  printf("is_present: %d\n", acc_is_present(arr, 0));
  #pragma acc data create(arr)
  {
    // Specified array is entirely present.
    // OUT-caseIsPresentSuccess-NEXT: is_present: 1
    printf("is_present: %d\n", acc_is_present(arr, 0));
  }
  #pragma acc data create(arr[0:1])
  {
    // Only subarray of specified array is present, including start.
    // OUT-caseIsPresentSuccess-NEXT: is_present: 1
    printf("is_present: %d\n", acc_is_present(arr, 0));
  }
  #pragma acc data create(arr[1:])
  {
    // Only subarray of specified array is present, not including start.
    // OUT-caseIsPresentSuccess-HOST-NEXT: is_present: 1
    //  OUT-caseIsPresentSuccess-OFF-NEXT: is_present: 0
    printf("is_present: %d\n", acc_is_present(arr, 0));
    // Check at valid host address immediately before what's present.
    // OUT-caseIsPresentSuccess-HOST-NEXT: is_present: 1
    //  OUT-caseIsPresentSuccess-OFF-NEXT: is_present: 0
    printf("is_present: %d\n", acc_is_present(((char*)&arr[1]) - 1, 0));
  }
  #pragma acc data create(arr[0:2])
  {
    // Check at valid host address immediately after what's present.
    // OUT-caseIsPresentSuccess-HOST-NEXT: is_present: 1
    //  OUT-caseIsPresentSuccess-OFF-NEXT: is_present: 0
    printf("is_present: %d\n", acc_is_present(arr+2, 0));
  }

  // Check case of null pointer.
  //
  // OUT-caseIsPresentSuccess-NEXT: is_present: 1
  // OUT-caseIsPresentSuccess-NEXT: is_present: 1
  printf("is_present: %d\n", acc_is_present(NULL, 4));
  printf("is_present: %d\n", acc_is_present(NULL, 0));

  return 0;
}

CASE(caseEnterExitRoutinesSuccess) {
  // These enter/exit-data-like routines are checked with
  // acc_map_data/acc_unmap_data is checked in caseMapUnmapSuccess.

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
    // OUT-caseEnterExitRoutinesSuccess-NEXT:    ci: 0x[[#%x,CI:]]
    // OUT-caseEnterExitRoutinesSuccess-NEXT:  cico: 0x[[#%x,CICO:]]
    // OUT-caseEnterExitRoutinesSuccess-NEXT: cicof: 0x[[#%x,CICOF:]]
    // OUT-caseEnterExitRoutinesSuccess-NEXT:  cidl: 0x[[#%x,CIDL:]]
    // OUT-caseEnterExitRoutinesSuccess-NEXT: cidlf: 0x[[#%x,CIDLF:]]
    // OUT-caseEnterExitRoutinesSuccess-NEXT:    cr: 0x[[#%x,CR:]]
    // OUT-caseEnterExitRoutinesSuccess-NEXT:  crco: 0x[[#%x,CRCO:]]
    // OUT-caseEnterExitRoutinesSuccess-NEXT: crcof: 0x[[#%x,CRCOF:]]
    // OUT-caseEnterExitRoutinesSuccess-NEXT:  crdl: 0x[[#%x,CRDL:]]
    // OUT-caseEnterExitRoutinesSuccess-NEXT: crdlf: 0x[[#%x,CRDLF:]]
    // OUT-caseEnterExitRoutinesSuccess-NEXT:    co: 0x[[#%x,CO:]]
    // OUT-caseEnterExitRoutinesSuccess-NEXT:   cof: 0x[[#%x,COF:]]
    // OUT-caseEnterExitRoutinesSuccess-NEXT:    dl: 0x[[#%x,DL:]]
    // OUT-caseEnterExitRoutinesSuccess-NEXT:   dlf: 0x[[#%x,DLF:]]
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
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT:    ci_dev: 0x[[#CI]]
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT:  cico_dev: 0x[[#CICO]]
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cicof_dev: 0x[[#CICOF]]
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT:  cidl_dev: 0x[[#CIDL]]
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cidlf_dev: 0x[[#CIDLF]]
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT:    cr_dev: 0x[[#CR]]
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT:  crco_dev: 0x[[#CRCO]]
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: crcof_dev: 0x[[#CRCOF]]
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT:  crdl_dev: 0x[[#CRDL]]
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: crdlf_dev: 0x[[#CRDLF]]
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:    ci_dev: 0x[[#%x,CI_DEV:]]
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  cico_dev: 0x[[#%x,CICO_DEV:]]
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cicof_dev: 0x[[#%x,CICOF_DEV:]]
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  cidl_dev: 0x[[#%x,CIDL_DEV:]]
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cidlf_dev: 0x[[#%x,CIDLF_DEV:]]
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:    cr_dev: 0x[[#%x,CR_DEV:]]
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  crco_dev: 0x[[#%x,CRCO_DEV:]]
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: crcof_dev: 0x[[#%x,CRCOF_DEV:]]
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  crdl_dev: 0x[[#%x,CRDL_DEV:]]
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: crdlf_dev: 0x[[#%x,CRDLF_DEV:]]
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
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT:    ci present: 0x[[#CI]]    -> 0x[[#CI]],          10 ->  10
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT:  cico present: 0x[[#CICO]]  -> 0x[[#CICO]],        20 ->  20
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cicof present: 0x[[#CICOF]] -> 0x[[#CICOF]],       30 ->  30
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT:  cidl present: 0x[[#CIDL]]  -> 0x[[#CIDL]],        40 ->  40
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cidlf present: 0x[[#CIDLF]] -> 0x[[#CIDLF]],       50 ->  50
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT:    cr present: 0x[[#CR]]    -> 0x[[#CR]],          60 ->  60
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT:  crco present: 0x[[#CRCO]]  -> 0x[[#CRCO]],        70 ->  70
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: crcof present: 0x[[#CRCOF]] -> 0x[[#CRCOF]],       80 ->  80
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT:  crdl present: 0x[[#CRDL]]  -> 0x[[#CRDL]],        90 ->  90
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: crdlf present: 0x[[#CRDLF]] -> 0x[[#CRDLF]],      100 -> 100
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT:    co present: 0x[[#CO]]    -> 0x[[#CO]],         110 -> 110
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT:   cof present: 0x[[#COF]]   -> 0x[[#COF]],        120 -> 120
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT:    dl present: 0x[[#DL]]    -> 0x[[#DL]],         130 -> 130
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT:   dlf present: 0x[[#DLF]]   -> 0x[[#DLF]],        140 -> 140
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:    ci present: 0x[[#CI]]    -> 0x[[#CI_DEV]],     10 ->  10
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  cico present: 0x[[#CICO]]  -> 0x[[#CICO_DEV]],   20 ->  20
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cicof present: 0x[[#CICOF]] -> 0x[[#CICOF_DEV]],  30 ->  30
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  cidl present: 0x[[#CIDL]]  -> 0x[[#CIDL_DEV]],   40 ->  40
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cidlf present: 0x[[#CIDLF]] -> 0x[[#CIDLF_DEV]],  50 ->  50
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:    cr present: 0x[[#CR]]    -> 0x[[#CR_DEV]],     60 -> {{.*}}
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  crco present: 0x[[#CRCO]]  -> 0x[[#CRCO_DEV]],   70 -> {{.*}}
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: crcof present: 0x[[#CRCOF]] -> 0x[[#CRCOF_DEV]],  80 -> {{.*}}
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  crdl present: 0x[[#CRDL]]  -> 0x[[#CRDL_DEV]],   90 -> {{.*}}
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: crdlf present: 0x[[#CRDLF]] -> 0x[[#CRDLF_DEV]], 100 -> {{.*}}
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:    co  absent: 0x[[#CO]]    -> (nil),            110
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:   cof  absent: 0x[[#COF]]   -> (nil),            120
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:    dl  absent: 0x[[#DL]]    -> (nil),            130
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:   dlf  absent: 0x[[#DLF]]   -> (nil),            140
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
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT:    ci present: 0x[[#CI]]    -> 0x[[#CI]],       11 ->  11
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT:  cico present: 0x[[#CICO]]  -> 0x[[#CICO]],     21 ->  21
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cicof present: 0x[[#CICOF]] -> 0x[[#CICOF]],    31 ->  31
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT:  cidl present: 0x[[#CIDL]]  -> 0x[[#CIDL]],     41 ->  41
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cidlf present: 0x[[#CIDLF]] -> 0x[[#CIDLF]],    51 ->  51
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT:    cr present: 0x[[#CR]]    -> 0x[[#CR]],       61 ->  61
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT:  crco present: 0x[[#CRCO]]  -> 0x[[#CRCO]],     71 ->  71
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: crcof present: 0x[[#CRCOF]] -> 0x[[#CRCOF]],    81 ->  81
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT:  crdl present: 0x[[#CRDL]]  -> 0x[[#CRDL]],     91 ->  91
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: crdlf present: 0x[[#CRDLF]] -> 0x[[#CRDLF]],   101 -> 101
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT:    co present: 0x[[#CO]]    -> 0x[[#CO]],      110 -> 110
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT:   cof present: 0x[[#COF]]   -> 0x[[#COF]],     120 -> 120
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT:    dl present: 0x[[#DL]]    -> 0x[[#DL]],      130 -> 130
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT:   dlf present: 0x[[#DLF]]   -> 0x[[#DLF]],     140 -> 140
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:    ci present: 0x[[#CI]]    -> 0x[[#CI_DEV]],  10 ->  11
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  cico  absent: 0x[[#CICO]]  -> (nil),          21
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cicof  absent: 0x[[#CICOF]] -> (nil),          31
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  cidl  absent: 0x[[#CIDL]]  -> (nil),          40
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cidlf  absent: 0x[[#CIDLF]] -> (nil),          50
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:    cr present: 0x[[#CR]]    -> 0x[[#CR_DEV]],  60 -> {{.*}}
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  crco  absent: 0x[[#CRCO]]  -> (nil),          71
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: crcof  absent: 0x[[#CRCOF]] -> (nil),          81
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  crdl  absent: 0x[[#CRDL]]  -> (nil),          90
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: crdlf  absent: 0x[[#CRDLF]] -> (nil),          100
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:    co  absent: 0x[[#CO]]    -> (nil),          110
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:   cof  absent: 0x[[#COF]]   -> (nil),          120
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:    dl  absent: 0x[[#DL]]    -> (nil),          130
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:   dlf  absent: 0x[[#DLF]]   -> (nil),          140
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
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: ci present: 0x[[#CI]] -> 0x[[#CI]], 11 -> 11
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cr present: 0x[[#CR]] -> 0x[[#CR]], 61 -> 61
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: ci  absent: 0x[[#CI]] -> (nil),     10
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cr  absent: 0x[[#CR]] -> (nil),     60
    PRINT_INT(ci);
    PRINT_INT(cr);
  }

  // Check the case where the structured ref count only is one: enclosed in acc
  // data.  That is, check its suppression of allocations and transfers from
  // enter-data-like and exit-data-like routines, check that those routines
  // return but don't affect the device addresses already mapped, and check
  // those routines' effect on the dynamic ref count.
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
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: ci  present: 0x[[#%x,CI:]]  -> 0x{{[^,]*}},        11 -> 11
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cr  present: 0x[[#%x,CR:]]  -> 0x{{[^,]*}},        21 -> 21
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: co  present: 0x[[#%x,CO:]]  -> 0x{{[^,]*}},        31 -> 31
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cof present: 0x[[#%x,COF:]] -> 0x{{[^,]*}},        41 -> 41
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dl  present: 0x[[#%x,DL:]]  -> 0x{{[^,]*}},        51 -> 51
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dlf present: 0x[[#%x,DLF:]] -> 0x{{[^,]*}},        61 -> 61
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: ci  present: 0x[[#%x,CI:]]  -> 0x[[#%x,CI_DEV:]],  10 -> 11
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cr  present: 0x[[#%x,CR:]]  -> 0x[[#%x,CR_DEV:]],  20 -> 21
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: co  present: 0x[[#%x,CO:]]  -> 0x[[#%x,CO_DEV:]],  30 -> 31
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cof present: 0x[[#%x,COF:]] -> 0x[[#%x,COF_DEV:]], 40 -> 41
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dl  present: 0x[[#%x,DL:]]  -> 0x[[#%x,DL_DEV:]],  50 -> 51
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dlf present: 0x[[#%x,DLF:]] -> 0x[[#%x,DLF_DEV:]], 60 -> 61
      PRINT_INT(ci);
      PRINT_INT(cr);
      PRINT_INT(co);
      PRINT_INT(cof);
      PRINT_INT(dl);
      PRINT_INT(dlf);

      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: acc_copyin of ci: 0x[[#CI]]
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: acc_create of cr: 0x[[#CR]]
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: acc_copyin of ci: 0x[[#CI_DEV]]
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: acc_create of cr: 0x[[#CR_DEV]]
      printf("acc_copyin of ci: %p\n", acc_copyin(&ci, sizeof ci));
      printf("acc_create of cr: %p\n", acc_create(&cr, sizeof cr));
      acc_copyout(&co, sizeof co);
      acc_copyout_finalize(&cof, sizeof cof);
      acc_delete(&dl, sizeof dl);
      acc_delete_finalize(&dlf, sizeof dlf);
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI]],      11 -> 11
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR]],      21 -> 21
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: co  present: 0x[[#CO]]  -> 0x[[#CO]],      31 -> 31
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cof present: 0x[[#COF]] -> 0x[[#COF]],     41 -> 41
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dl  present: 0x[[#DL]]  -> 0x[[#DL]],      51 -> 51
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dlf present: 0x[[#DLF]] -> 0x[[#DLF]],     61 -> 61
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI_DEV]],  10 -> 11
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR_DEV]],  20 -> 21
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: co  present: 0x[[#CO]]  -> 0x[[#CO_DEV]],  30 -> 31
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cof present: 0x[[#COF]] -> 0x[[#COF_DEV]], 40 -> 41
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dl  present: 0x[[#DL]]  -> 0x[[#DL_DEV]],  50 -> 51
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dlf present: 0x[[#DLF]] -> 0x[[#DLF_DEV]], 60 -> 61
      PRINT_INT(ci);
      PRINT_INT(cr);
      PRINT_INT(co);
      PRINT_INT(cof);
      PRINT_INT(dl);
      PRINT_INT(dlf);
    }

    // Check the dynamic reference counts by seeing how many decs it takes to
    // reach zero.
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI]],     11 -> 11
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR]],     21 -> 21
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: co  present: 0x[[#CO]]  -> 0x[[#CO]],     31 -> 31
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cof present: 0x[[#COF]] -> 0x[[#COF]],    41 -> 41
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dl  present: 0x[[#DL]]  -> 0x[[#DL]],     51 -> 51
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dlf present: 0x[[#DLF]] -> 0x[[#DLF]],    61 -> 61
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI_DEV]], 10 -> 11
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR_DEV]], 20 -> 21
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: co  absent:  0x[[#CO]]  -> (nil),         30
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cof absent:  0x[[#COF]] -> (nil),         40
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dl  absent:  0x[[#DL]]  -> (nil),         50
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dlf absent:  0x[[#DLF]] -> (nil),         60
    PRINT_INT(ci);
    PRINT_INT(cr);
    PRINT_INT(co);
    PRINT_INT(cof);
    PRINT_INT(dl);
    PRINT_INT(dlf);
    #pragma acc exit data delete(ci, cr)
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: ci present: 0x[[#CI]] -> 0x[[#CI]], 11 -> 11
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cr present: 0x[[#CR]] -> 0x[[#CR]], 21 -> 21
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: ci  absent: 0x[[#CI]] -> (nil),     10
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cr  absent: 0x[[#CR]] -> (nil),     20
    PRINT_INT(ci);
    PRINT_INT(cr);
  }

  // Repeat but where both the structured and dynamic ref counter are initially
  // one.  This checks cases like decrementing the dynamic ref count from one to
  // zero while the structured ref count is one to be sure that doesn't trigger
  // deallocations or transfers.
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
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: ci  present: 0x[[#%x,CI:]]  -> 0x{{[^,]*}},        11 -> 11
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cr  present: 0x[[#%x,CR:]]  -> 0x{{[^,]*}},        21 -> 21
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: co  present: 0x[[#%x,CO:]]  -> 0x{{[^,]*}},        31 -> 31
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cof present: 0x[[#%x,COF:]] -> 0x{{[^,]*}},        41 -> 41
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dl  present: 0x[[#%x,DL:]]  -> 0x{{[^,]*}},        51 -> 51
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dlf present: 0x[[#%x,DLF:]] -> 0x{{[^,]*}},        61 -> 61
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: ci  present: 0x[[#%x,CI:]]  -> 0x[[#%x,CI_DEV:]],  10 -> 11
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cr  present: 0x[[#%x,CR:]]  -> 0x[[#%x,CR_DEV:]],  20 -> 21
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: co  present: 0x[[#%x,CO:]]  -> 0x[[#%x,CO_DEV:]],  30 -> 31
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cof present: 0x[[#%x,COF:]] -> 0x[[#%x,COF_DEV:]], 40 -> 41
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dl  present: 0x[[#%x,DL:]]  -> 0x[[#%x,DL_DEV:]],  50 -> 51
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dlf present: 0x[[#%x,DLF:]] -> 0x[[#%x,DLF_DEV:]], 60 -> 61
      PRINT_INT(ci);
      PRINT_INT(cr);
      PRINT_INT(co);
      PRINT_INT(cof);
      PRINT_INT(dl);
      PRINT_INT(dlf);

      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: acc_copyin of ci: 0x[[#CI]]
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: acc_create of cr: 0x[[#CR]]
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: acc_copyin of ci: 0x[[#CI_DEV]]
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: acc_create of cr: 0x[[#CR_DEV]]
      printf("acc_copyin of ci: %p\n", acc_copyin(&ci, sizeof ci));
      printf("acc_create of cr: %p\n", acc_create(&cr, sizeof cr));
      acc_copyout(&co, sizeof co);
      acc_copyout_finalize(&cof, sizeof cof);
      acc_delete(&dl, sizeof dl);
      acc_delete_finalize(&dlf, sizeof dlf);
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI]],      11 -> 11
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR]],      21 -> 21
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: co  present: 0x[[#CO]]  -> 0x[[#CO]],      31 -> 31
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cof present: 0x[[#COF]] -> 0x[[#COF]],     41 -> 41
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dl  present: 0x[[#DL]]  -> 0x[[#DL]],      51 -> 51
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dlf present: 0x[[#DLF]] -> 0x[[#DLF]],     61 -> 61
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI_DEV]],  10 -> 11
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR_DEV]],  20 -> 21
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: co  present: 0x[[#CO]]  -> 0x[[#CO_DEV]],  30 -> 31
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cof present: 0x[[#COF]] -> 0x[[#COF_DEV]], 40 -> 41
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dl  present: 0x[[#DL]]  -> 0x[[#DL_DEV]],  50 -> 51
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dlf present: 0x[[#DLF]] -> 0x[[#DLF_DEV]], 60 -> 61
      PRINT_INT(ci);
      PRINT_INT(cr);
      PRINT_INT(co);
      PRINT_INT(cof);
      PRINT_INT(dl);
      PRINT_INT(dlf);
    }

    // Check the dynamic reference counts by seeing how many decs it takes to
    // reach zero.
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI]],     11 -> 11
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR]],     21 -> 21
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: co  present: 0x[[#CO]]  -> 0x[[#CO]],     31 -> 31
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cof present: 0x[[#COF]] -> 0x[[#COF]],    41 -> 41
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dl  present: 0x[[#DL]]  -> 0x[[#DL]],     51 -> 51
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dlf present: 0x[[#DLF]] -> 0x[[#DLF]],    61 -> 61
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI_DEV]], 10 -> 11
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR_DEV]], 20 -> 21
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: co  absent:  0x[[#CO]]  -> (nil),         30
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cof absent:  0x[[#COF]] -> (nil),         40
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dl  absent:  0x[[#DL]]  -> (nil),         50
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dlf absent:  0x[[#DLF]] -> (nil),         60
    PRINT_INT(ci);
    PRINT_INT(cr);
    PRINT_INT(co);
    PRINT_INT(cof);
    PRINT_INT(dl);
    PRINT_INT(dlf);
    #pragma acc exit data delete(ci, cr)
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI]],     11 -> 11
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR]],     21 -> 21
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI_DEV]], 10 -> 11
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR_DEV]], 20 -> 21
    PRINT_INT(ci);
    PRINT_INT(cr);
    #pragma acc exit data delete(ci, cr)
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: ci present: 0x[[#CI]] -> 0x[[#CI]], 11 -> 11
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cr present: 0x[[#CR]] -> 0x[[#CR]], 21 -> 21
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: ci  absent: 0x[[#CI]] -> (nil),     10
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cr  absent: 0x[[#CR]] -> (nil),     20
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
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: ci  present: 0x[[#%x,CI:]]  -> 0x{{[^,]*}},        11 -> 11
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cr  present: 0x[[#%x,CR:]]  -> 0x{{[^,]*}},        21 -> 21
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: co  present: 0x[[#%x,CO:]]  -> 0x{{[^,]*}},        31 -> 31
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cof present: 0x[[#%x,COF:]] -> 0x{{[^,]*}},        41 -> 41
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dl  present: 0x[[#%x,DL:]]  -> 0x{{[^,]*}},        51 -> 51
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dlf present: 0x[[#%x,DLF:]] -> 0x{{[^,]*}},        61 -> 61
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: ci  present: 0x[[#%x,CI:]]  -> 0x[[#%x,CI_DEV:]],  10 -> 11
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cr  present: 0x[[#%x,CR:]]  -> 0x[[#%x,CR_DEV:]],  20 -> 21
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: co  present: 0x[[#%x,CO:]]  -> 0x[[#%x,CO_DEV:]],  30 -> 31
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cof present: 0x[[#%x,COF:]] -> 0x[[#%x,COF_DEV:]], 40 -> 41
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dl  present: 0x[[#%x,DL:]]  -> 0x[[#%x,DL_DEV:]],  50 -> 51
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dlf present: 0x[[#%x,DLF:]] -> 0x[[#%x,DLF_DEV:]], 60 -> 61
    PRINT_INT(ci);
    PRINT_INT(cr);
    PRINT_INT(co);
    PRINT_INT(cof);
    PRINT_INT(dl);
    PRINT_INT(dlf);

    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: acc_copyin of ci: 0x[[#CI]]
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: acc_create of cr: 0x[[#CR]]
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: acc_copyin of ci: 0x[[#CI_DEV]]
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: acc_create of cr: 0x[[#CR_DEV]]
    printf("acc_copyin of ci: %p\n", acc_copyin(&ci, sizeof ci));
    printf("acc_create of cr: %p\n", acc_create(&cr, sizeof cr));
    acc_copyout(&co, sizeof co);
    acc_copyout_finalize(&cof, sizeof cof);
    acc_delete(&dl, sizeof dl);
    acc_delete_finalize(&dlf, sizeof dlf);
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI]],     11 -> 11
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR]],     21 -> 21
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: co  present: 0x[[#CO]]  -> 0x[[#CO]],     31 -> 31
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cof present: 0x[[#COF]] -> 0x[[#COF]],    41 -> 41
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dl  present: 0x[[#DL]]  -> 0x[[#DL]],     51 -> 51
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dlf present: 0x[[#DLF]] -> 0x[[#DLF]],    61 -> 61
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI_DEV]], 10 -> 11
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR_DEV]], 20 -> 21
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: co  absent:  0x[[#CO]]  -> (nil),         31
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cof absent:  0x[[#COF]] -> (nil),         41
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dl  absent:  0x[[#DL]]  -> (nil),         50
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dlf absent:  0x[[#DLF]] -> (nil),         60
    PRINT_INT(ci);
    PRINT_INT(cr);
    PRINT_INT(co);
    PRINT_INT(cof);
    PRINT_INT(dl);
    PRINT_INT(dlf);

    // Check the remaining dynamic reference counts by seeing how many decs it
    // takes to reach zero.
    #pragma acc exit data delete(ci, cr)
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: ci present: 0x[[#CI]] -> 0x[[#CI]],     11 -> 11
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cr present: 0x[[#CR]] -> 0x[[#CR]],     21 -> 21
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: ci present: 0x[[#CI]] -> 0x[[#CI_DEV]], 10 -> 11
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cr present: 0x[[#CR]] -> 0x[[#CR_DEV]], 20 -> 21
    PRINT_INT(ci);
    PRINT_INT(cr);
    #pragma acc exit data delete(ci, cr)
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: ci present: 0x[[#CI]] -> 0x[[#CI]], 11 -> 11
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cr present: 0x[[#CR]] -> 0x[[#CR]], 21 -> 21
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: ci  absent: 0x[[#CI]] -> (nil),     10
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cr  absent: 0x[[#CR]] -> (nil),     20
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
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: ci  present: 0x[[#%x,CI:]]  -> 0x{{[^,]*}},        11 -> 11
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cr  present: 0x[[#%x,CR:]]  -> 0x{{[^,]*}},        21 -> 21
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: co  present: 0x[[#%x,CO:]]  -> 0x{{[^,]*}},        31 -> 31
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cof present: 0x[[#%x,COF:]] -> 0x{{[^,]*}},        41 -> 41
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dl  present: 0x[[#%x,DL:]]  -> 0x{{[^,]*}},        51 -> 51
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dlf present: 0x[[#%x,DLF:]] -> 0x{{[^,]*}},        61 -> 61
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: ci  present: 0x[[#%x,CI:]]  -> 0x[[#%x,CI_DEV:]],  10 -> 11
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cr  present: 0x[[#%x,CR:]]  -> 0x[[#%x,CR_DEV:]],  20 -> 21
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: co  present: 0x[[#%x,CO:]]  -> 0x[[#%x,CO_DEV:]],  30 -> 31
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cof present: 0x[[#%x,COF:]] -> 0x[[#%x,COF_DEV:]], 40 -> 41
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dl  present: 0x[[#%x,DL:]]  -> 0x[[#%x,DL_DEV:]],  50 -> 51
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dlf present: 0x[[#%x,DLF:]] -> 0x[[#%x,DLF_DEV:]], 60 -> 61
    PRINT_INT(ci);
    PRINT_INT(cr);
    PRINT_INT(co);
    PRINT_INT(cof);
    PRINT_INT(dl);
    PRINT_INT(dlf);

    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: acc_copyin of ci: 0x[[#CI]]
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: acc_create of cr: 0x[[#CR]]
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: acc_copyin of ci: 0x[[#CI_DEV]]
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: acc_create of cr: 0x[[#CR_DEV]]
    printf("acc_copyin of ci: %p\n", acc_copyin(&ci, sizeof ci));
    printf("acc_create of cr: %p\n", acc_create(&cr, sizeof cr));
    acc_copyout(&co, sizeof co);
    acc_copyout_finalize(&cof, sizeof cof);
    acc_delete(&dl, sizeof dl);
    acc_delete_finalize(&dlf, sizeof dlf);
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI]],     11 -> 11
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR]],     21 -> 21
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: co  present: 0x[[#CO]]  -> 0x[[#CO]],     31 -> 31
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cof present: 0x[[#COF]] -> 0x[[#COF]],    41 -> 41
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dl  present: 0x[[#DL]]  -> 0x[[#DL]],     51 -> 51
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dlf present: 0x[[#DLF]] -> 0x[[#DLF]],    61 -> 61
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: ci  present: 0x[[#CI]]  -> 0x[[#CI_DEV]], 10 -> 11
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cr  present: 0x[[#CR]]  -> 0x[[#CR_DEV]], 20 -> 21
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: co  present: 0x[[#CO]]  -> 0x[[#CO_DEV]], 30 -> 31
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cof absent:  0x[[#COF]] -> (nil),         41
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dl  present: 0x[[#DL]]  -> 0x[[#DL_DEV]], 50 -> 51
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dlf absent:  0x[[#DLF]] -> (nil),         60
    PRINT_INT(ci);
    PRINT_INT(cr);
    PRINT_INT(co);
    PRINT_INT(cof);
    PRINT_INT(dl);
    PRINT_INT(dlf);

    // Check the remaining dynamic reference counts by seeing how many decs it
    // takes to reach zero.
    #pragma acc exit data delete(co, dl)
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: co present: 0x[[#CO]] -> 0x[[#CO]], 31 -> 31
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dl present: 0x[[#DL]] -> 0x[[#DL]], 51 -> 51
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: co  absent: 0x[[#CO]] -> (nil),     30
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dl  absent: 0x[[#DL]] -> (nil),     50
    PRINT_INT(co);
    PRINT_INT(dl);
    #pragma acc exit data delete(ci, cr)
    #pragma acc exit data delete(ci, cr)
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: ci present: 0x[[#CI]] -> 0x[[#CI]],     11 -> 11
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cr present: 0x[[#CR]] -> 0x[[#CR]],     21 -> 21
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: ci present: 0x[[#CI]] -> 0x[[#CI_DEV]], 10 -> 11
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cr present: 0x[[#CR]] -> 0x[[#CR_DEV]], 20 -> 21
    PRINT_INT(ci);
    PRINT_INT(cr);
    #pragma acc exit data delete(ci, cr)
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: ci present: 0x[[#CI]] -> 0x[[#CI]], 11 -> 11
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: cr present: 0x[[#CR]] -> 0x[[#CR]], 21 -> 21
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: ci  absent: 0x[[#CI]] -> (nil),     10
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: cr  absent: 0x[[#CR]] -> (nil),     20
    PRINT_INT(ci);
    PRINT_INT(cr);
  }

  // Check case of zero bytes.
  {
    // OUT-caseEnterExitRoutinesSuccess-NEXT: x: 0x[[#%x,X:]]
    int x = 10;
    printf("x: %p\n", &x);

    // When data is absent.
    //
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: acc_copyin(&x, 0): (nil)
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]], 10 -> 10
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: acc_create(&x, 0): (nil)
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]], 10 -> 10
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]], 10 -> 10
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]], 10 -> 10
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]], 10 -> 10
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]], 10 -> 10
    // OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  acc_copyin(&x, 0): (nil)
    // OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  x  absent: 0x[[#X]] -> (nil),    10
    // OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  acc_create(&x, 0): (nil)
    // OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  x  absent: 0x[[#X]] -> (nil),    10
    // OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  x  absent: 0x[[#X]] -> (nil),    10
    // OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  x  absent: 0x[[#X]] -> (nil),    10
    // OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  x  absent: 0x[[#X]] -> (nil),    10
    // OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  x  absent: 0x[[#X]] -> (nil),    10
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

    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]],         11 -> 11
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: x present: 0x[[#X]] -> 0x[[#%x,X_DEV:]], 10 -> 11
    #pragma acc enter data create(x)
    #pragma acc parallel num_gangs(1) present(x)
    x = 11;
    PRINT_INT(x);

    // When data is present.
    //
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: acc_copyin(&x, 0): (nil)
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]],     11 -> 11
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: acc_create(&x, 0): (nil)
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]],     11 -> 11
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]],     11 -> 11
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]],     11 -> 11
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]],     11 -> 11
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]],     11 -> 11
    // OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  acc_copyin(&x, 0): (nil)
    // OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  x present: 0x[[#X]] -> 0x[[#X_DEV]], 10 -> 11
    // OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  acc_create(&x, 0): (nil)
    // OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  x present: 0x[[#X]] -> 0x[[#X_DEV]], 10 -> 11
    // OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  x present: 0x[[#X]] -> 0x[[#X_DEV]], 10 -> 11
    // OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  x present: 0x[[#X]] -> 0x[[#X_DEV]], 10 -> 11
    // OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  x present: 0x[[#X]] -> 0x[[#X_DEV]], 10 -> 11
    // OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  x present: 0x[[#X]] -> 0x[[#X_DEV]], 10 -> 11
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

    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]], 11 -> 11
    // OUT-caseEnterExitRoutinesSuccess-OFF-NEXT:  x  absent: 0x[[#X]] -> (nil),    10
    #pragma acc exit data delete(x)
    PRINT_INT(x);
  }

  // Check case of null pointer, with non-zero or zero bytes.
  //
  // OUT-caseEnterExitRoutinesSuccess-NEXT: acc_copyin(NULL, 1): (nil)
  // OUT-caseEnterExitRoutinesSuccess-NEXT: NULL present: (nil) -> (nil)
  // OUT-caseEnterExitRoutinesSuccess-NEXT: acc_create(NULL, 1): (nil)
  // OUT-caseEnterExitRoutinesSuccess-NEXT: NULL present: (nil) -> (nil)
  // OUT-caseEnterExitRoutinesSuccess-NEXT: NULL present: (nil) -> (nil)
  // OUT-caseEnterExitRoutinesSuccess-NEXT: NULL present: (nil) -> (nil)
  // OUT-caseEnterExitRoutinesSuccess-NEXT: NULL present: (nil) -> (nil)
  // OUT-caseEnterExitRoutinesSuccess-NEXT: NULL present: (nil) -> (nil)
  // OUT-caseEnterExitRoutinesSuccess-NEXT: acc_copyin(NULL, 0): (nil)
  // OUT-caseEnterExitRoutinesSuccess-NEXT: NULL present: (nil) -> (nil)
  // OUT-caseEnterExitRoutinesSuccess-NEXT: acc_create(NULL, 0): (nil)
  // OUT-caseEnterExitRoutinesSuccess-NEXT: NULL present: (nil) -> (nil)
  // OUT-caseEnterExitRoutinesSuccess-NEXT: NULL present: (nil) -> (nil)
  // OUT-caseEnterExitRoutinesSuccess-NEXT: NULL present: (nil) -> (nil)
  // OUT-caseEnterExitRoutinesSuccess-NEXT: NULL present: (nil) -> (nil)
  // OUT-caseEnterExitRoutinesSuccess-NEXT: NULL present: (nil) -> (nil)
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
  // subarrays.  Actions for the same subarray or a subset should have no effect
  // except to adjust the dynamic reference count.
  {
    int dyn[4] = {10, 20, 30, 40};
    int str[4] = {50, 60, 70, 80};
    // OUT-caseEnterExitRoutinesSuccess-NEXT: dyn element size: [[#%u,DYN_ELE_SIZE:]]
    // OUT-caseEnterExitRoutinesSuccess-NEXT: str element size: [[#%u,STR_ELE_SIZE:]]
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
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dyn[1] present: 0x[[#%x,DYN1:]] -> 0x{{[^,]*}},         21 -> 21
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: str[1] present: 0x[[#%x,STR1:]] -> 0x{{[^,]*}},         61 -> 61
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dyn[1] present: 0x[[#%x,DYN1:]] -> 0x[[#%x,DYN1_DEV:]], 20 -> 21
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: str[1] present: 0x[[#%x,STR1:]] -> 0x[[#%x,STR1_DEV:]], 60 -> 61
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

      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: acc_copyin of dyn[1:2]: 0x[[#DYN1]]
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: acc_copyin of str[1:2]: 0x[[#STR1]]
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: acc_create of dyn[1:2]: 0x[[#DYN1]]
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: acc_create of str[1:2]: 0x[[#STR1]]
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: acc_copyin of dyn[1:2]: 0x[[#DYN1_DEV]]
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: acc_copyin of str[1:2]: 0x[[#STR1_DEV]]
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: acc_create of dyn[1:2]: 0x[[#DYN1_DEV]]
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: acc_create of str[1:2]: 0x[[#STR1_DEV]]
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

      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: acc_copyin of dyn[2]: 0x[[#%x,DYN1 + DYN_ELE_SIZE]]
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: acc_copyin of str[2]: 0x[[#%x,STR1 + STR_ELE_SIZE]]
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: acc_create of dyn[2]: 0x[[#%x,DYN1 + DYN_ELE_SIZE]]
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: acc_create of str[2]: 0x[[#%x,STR1 + STR_ELE_SIZE]]
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: acc_copyin of dyn[2]: 0x[[#%x,DYN1_DEV + DYN_ELE_SIZE]]
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: acc_copyin of str[2]: 0x[[#%x,STR1_DEV + STR_ELE_SIZE]]
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: acc_create of dyn[2]: 0x[[#%x,DYN1_DEV + DYN_ELE_SIZE]]
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: acc_create of str[2]: 0x[[#%x,STR1_DEV + STR_ELE_SIZE]]
      printf("acc_copyin of dyn[2]: %p\n", acc_copyin(dyn + 2, sizeof *dyn));
      printf("acc_copyin of str[2]: %p\n", acc_copyin(str + 2, sizeof *str));
      printf("acc_create of dyn[2]: %p\n", acc_create(dyn + 2, sizeof *dyn));
      printf("acc_create of str[2]: %p\n", acc_create(str + 2, sizeof *str));
      // dyn[1:2] has dyn ref = 5, str ref = 0.
      // str[1:2] has dyn ref = 4, str ref = 1.

      // Don't decrement the dyn ref count to what was set by the pragmas (1 for
      // dyn, 0 for str) or we won't be able to tell if all the routine calls
      // had any net effect.
      acc_copyout(dyn + 1, 2 * sizeof *dyn);
      acc_copyout(dyn + 1, sizeof *dyn);
      acc_copyout(str + 1, 2 * sizeof *str);
      acc_delete(dyn + 2, sizeof *dyn);
      acc_delete(str + 2, sizeof *str);
      // dyn[1:2] has dyn ref = 2, str ref = 0.
      // str[1:2] has dyn ref = 2, str ref = 1.

      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dyn[0] present: 0x[[#%x,DYN1 - DYN_ELE_SIZE]]                -> 0x[[#%x,DYN1 - DYN_ELE_SIZE]],                10 -> 10
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dyn[1] present: 0x[[#%x,DYN1]]                               -> 0x[[#%x,DYN1]],                               21 -> 21
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dyn[2] present: 0x[[#%x,DYN1 + DYN_ELE_SIZE]]                -> 0x[[#%x,DYN1 + DYN_ELE_SIZE]],                31 -> 31
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dyn[3] present: 0x[[#%x,DYN1 + DYN_ELE_SIZE + DYN_ELE_SIZE]] -> 0x[[#%x,DYN1 + DYN_ELE_SIZE + DYN_ELE_SIZE]], 40 -> 40
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: str[0] present: 0x[[#%x,STR1 - STR_ELE_SIZE]]                -> 0x[[#%x,STR1 - STR_ELE_SIZE]],                50 -> 50
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: str[1] present: 0x[[#%x,STR1]]                               -> 0x[[#%x,STR1]],                               61 -> 61
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: str[2] present: 0x[[#%x,STR1 + STR_ELE_SIZE]]                -> 0x[[#%x,STR1 + STR_ELE_SIZE]],                71 -> 71
      // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: str[3] present: 0x[[#%x,STR1 + STR_ELE_SIZE + STR_ELE_SIZE]] -> 0x[[#%x,STR1 + STR_ELE_SIZE + STR_ELE_SIZE]], 80 -> 80
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dyn[0]  absent: 0x[[#%x,DYN1 - DYN_ELE_SIZE]]                -> (nil),                                        10
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dyn[1] present: 0x[[#%x,DYN1]]                               -> 0x[[#%x,DYN1_DEV]],                           20 -> 21
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dyn[2] present: 0x[[#%x,DYN1 + DYN_ELE_SIZE]]                -> 0x[[#%x,DYN1_DEV + DYN_ELE_SIZE]],            30 -> 31
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dyn[3]  absent: 0x[[#%x,DYN1 + DYN_ELE_SIZE + DYN_ELE_SIZE]] -> (nil),                                        40
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: str[0]  absent: 0x[[#%x,STR1 - STR_ELE_SIZE]]                -> (nil),                                        50
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: str[1] present: 0x[[#%x,STR1]]                               -> 0x[[#%x,STR1_DEV]],                           60 -> 61
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: str[2] present: 0x[[#%x,STR1 + STR_ELE_SIZE]]                -> 0x[[#%x,STR1_DEV + STR_ELE_SIZE]],            70 -> 71
      //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: str[3]  absent: 0x[[#%x,STR1 + STR_ELE_SIZE + STR_ELE_SIZE]] -> (nil),                                        80
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
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dyn[0] present: 0x[[#%x,DYN1 - DYN_ELE_SIZE]]                -> 0x[[#%x,DYN1 - DYN_ELE_SIZE]],                10 -> 10
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dyn[1] present: 0x[[#%x,DYN1]]                               -> 0x[[#%x,DYN1]],                               21 -> 21
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dyn[2] present: 0x[[#%x,DYN1 + DYN_ELE_SIZE]]                -> 0x[[#%x,DYN1 + DYN_ELE_SIZE]],                31 -> 31
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dyn[3] present: 0x[[#%x,DYN1 + DYN_ELE_SIZE + DYN_ELE_SIZE]] -> 0x[[#%x,DYN1 + DYN_ELE_SIZE + DYN_ELE_SIZE]], 40 -> 40
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: str[0] present: 0x[[#%x,STR1 - STR_ELE_SIZE]]                -> 0x[[#%x,STR1 - STR_ELE_SIZE]],                50 -> 50
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: str[1] present: 0x[[#%x,STR1]]                               -> 0x[[#%x,STR1]],                               61 -> 61
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: str[2] present: 0x[[#%x,STR1 + STR_ELE_SIZE]]                -> 0x[[#%x,STR1 + STR_ELE_SIZE]],                71 -> 71
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: str[3] present: 0x[[#%x,STR1 + STR_ELE_SIZE + STR_ELE_SIZE]] -> 0x[[#%x,STR1 + STR_ELE_SIZE + STR_ELE_SIZE]], 80 -> 80
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dyn[0]  absent: 0x[[#%x,DYN1 - DYN_ELE_SIZE]]                -> (nil),                                        10
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dyn[1] present: 0x[[#%x,DYN1]]                               -> 0x[[#%x,DYN1_DEV]],                           20 -> 21
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dyn[2] present: 0x[[#%x,DYN1 + DYN_ELE_SIZE]]                -> 0x[[#%x,DYN1_DEV + DYN_ELE_SIZE]],            30 -> 31
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dyn[3]  absent: 0x[[#%x,DYN1 + DYN_ELE_SIZE + DYN_ELE_SIZE]] -> (nil),                                        40
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: str[0]  absent: 0x[[#%x,STR1 - STR_ELE_SIZE]]                -> (nil),                                        50
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: str[1] present: 0x[[#%x,STR1]]                               -> 0x[[#%x,STR1_DEV]],                           60 -> 61
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: str[2] present: 0x[[#%x,STR1 + STR_ELE_SIZE]]                -> 0x[[#%x,STR1_DEV + STR_ELE_SIZE]],            70 -> 71
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: str[3]  absent: 0x[[#%x,STR1 + STR_ELE_SIZE + STR_ELE_SIZE]] -> (nil),                                        80
    PRINT_INT(dyn[0]);
    PRINT_INT(dyn[1]);
    PRINT_INT(dyn[2]);
    PRINT_INT(dyn[3]);
    PRINT_INT(str[0]);
    PRINT_INT(str[1]);
    PRINT_INT(str[2]);
    PRINT_INT(str[3]);
    #pragma acc exit data delete(dyn[1:2], str[1:2])
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dyn[0] present: 0x[[#%x,DYN1 - DYN_ELE_SIZE]]                -> 0x[[#%x,DYN1 - DYN_ELE_SIZE]],                10 -> 10
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dyn[1] present: 0x[[#%x,DYN1]]                               -> 0x[[#%x,DYN1]],                               21 -> 21
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dyn[2] present: 0x[[#%x,DYN1 + DYN_ELE_SIZE]]                -> 0x[[#%x,DYN1 + DYN_ELE_SIZE]],                31 -> 31
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dyn[3] present: 0x[[#%x,DYN1 + DYN_ELE_SIZE + DYN_ELE_SIZE]] -> 0x[[#%x,DYN1 + DYN_ELE_SIZE + DYN_ELE_SIZE]], 40 -> 40
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: str[0] present: 0x[[#%x,STR1 - STR_ELE_SIZE]]                -> 0x[[#%x,STR1 - STR_ELE_SIZE]],                50 -> 50
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: str[1] present: 0x[[#%x,STR1]]                               -> 0x[[#%x,STR1]],                               61 -> 61
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: str[2] present: 0x[[#%x,STR1 + STR_ELE_SIZE]]                -> 0x[[#%x,STR1 + STR_ELE_SIZE]],                71 -> 71
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: str[3] present: 0x[[#%x,STR1 + STR_ELE_SIZE + STR_ELE_SIZE]] -> 0x[[#%x,STR1 + STR_ELE_SIZE + STR_ELE_SIZE]], 80 -> 80
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dyn[0]  absent: 0x[[#%x,DYN1 - DYN_ELE_SIZE]]                -> (nil),                                        10
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dyn[1] present: 0x[[#%x,DYN1]]                               -> 0x[[#%x,DYN1_DEV]],                           20 -> 21
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dyn[2] present: 0x[[#%x,DYN1 + DYN_ELE_SIZE]]                -> 0x[[#%x,DYN1_DEV + DYN_ELE_SIZE]],            30 -> 31
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dyn[3]  absent: 0x[[#%x,DYN1 + DYN_ELE_SIZE + DYN_ELE_SIZE]] -> (nil),                                        40
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: str[0]  absent: 0x[[#%x,STR1 - STR_ELE_SIZE]]                -> (nil),                                        50
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: str[1] present: 0x[[#%x,STR1]]                               -> 0x[[#%x,STR1_DEV]],                           60 -> 61
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: str[2] present: 0x[[#%x,STR1 + STR_ELE_SIZE]]                -> 0x[[#%x,STR1_DEV + STR_ELE_SIZE]],            70 -> 71
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: str[3]  absent: 0x[[#%x,STR1 + STR_ELE_SIZE + STR_ELE_SIZE]] -> (nil),                                        80
    PRINT_INT(dyn[0]);
    PRINT_INT(dyn[1]);
    PRINT_INT(dyn[2]);
    PRINT_INT(dyn[3]);
    PRINT_INT(str[0]);
    PRINT_INT(str[1]);
    PRINT_INT(str[2]);
    PRINT_INT(str[3]);
    #pragma acc exit data delete(dyn[1:2], str[1:2])
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dyn[0] present: 0x[[#%x,DYN1 - DYN_ELE_SIZE]]                -> 0x[[#%x,DYN1 - DYN_ELE_SIZE]],                10 -> 10
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dyn[1] present: 0x[[#%x,DYN1]]                               -> 0x[[#%x,DYN1]],                               21 -> 21
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dyn[2] present: 0x[[#%x,DYN1 + DYN_ELE_SIZE]]                -> 0x[[#%x,DYN1 + DYN_ELE_SIZE]],                31 -> 31
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: dyn[3] present: 0x[[#%x,DYN1 + DYN_ELE_SIZE + DYN_ELE_SIZE]] -> 0x[[#%x,DYN1 + DYN_ELE_SIZE + DYN_ELE_SIZE]], 40 -> 40
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: str[0] present: 0x[[#%x,STR1 - STR_ELE_SIZE]]                -> 0x[[#%x,STR1 - STR_ELE_SIZE]],                50 -> 50
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: str[1] present: 0x[[#%x,STR1]]                               -> 0x[[#%x,STR1]],                               61 -> 61
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: str[2] present: 0x[[#%x,STR1 + STR_ELE_SIZE]]                -> 0x[[#%x,STR1 + STR_ELE_SIZE]],                71 -> 71
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: str[3] present: 0x[[#%x,STR1 + STR_ELE_SIZE + STR_ELE_SIZE]] -> 0x[[#%x,STR1 + STR_ELE_SIZE + STR_ELE_SIZE]], 80 -> 80
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dyn[0]  absent: 0x[[#%x,DYN1 - DYN_ELE_SIZE]]                -> (nil),                                        10
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dyn[1]  absent: 0x[[#%x,DYN1]]                               -> (nil),                                        20
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dyn[2]  absent: 0x[[#%x,DYN1 + DYN_ELE_SIZE]]                -> (nil),                                        30
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: dyn[3]  absent: 0x[[#%x,DYN1 + DYN_ELE_SIZE + DYN_ELE_SIZE]] -> (nil),                                        40
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: str[0]  absent: 0x[[#%x,STR1 - STR_ELE_SIZE]]                -> (nil),                                        50
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: str[1]  absent: 0x[[#%x,STR1]]                               -> (nil),                                        60
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: str[2]  absent: 0x[[#%x,STR1 + STR_ELE_SIZE]]                -> (nil),                                        70
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: str[3]  absent: 0x[[#%x,STR1 + STR_ELE_SIZE + STR_ELE_SIZE]] -> (nil),                                        80
    PRINT_INT(dyn[0]);
    PRINT_INT(dyn[1]);
    PRINT_INT(dyn[2]);
    PRINT_INT(dyn[3]);
    PRINT_INT(str[0]);
    PRINT_INT(str[1]);
    PRINT_INT(str[2]);
    PRINT_INT(str[3]);
  }

  // Check action suppression from acc_(copyout|delete)[_finalize] when array is
  // present but specified subarray is not.  Errors from acc_(copyin|create) for
  // that case are checked in caseCopyinExtendsAfter,
  // caseCreateExtendsAfter, etc.
  {
    //      OUT-caseEnterExitRoutinesSuccess-NEXT: element size: [[#%u,ELE_SIZE:]]
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: arr[0] present: 0x[[#%x,ARR:]]                      -> 0x{{[^,]*}},                         10 -> 10
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: arr[1] present: 0x[[#%x,ARR + ELE_SIZE]]            -> 0x[[#%x,ARR + ELE_SIZE]],            21 -> 21
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: arr[2] present: 0x[[#%x,ARR + ELE_SIZE + ELE_SIZE]] -> 0x[[#%x,ARR + ELE_SIZE + ELE_SIZE]], 30 -> 30
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: arr[0]  absent: 0x[[#%x,ARR:]]                      -> (nil),                               10
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: arr[1] present: 0x[[#%x,ARR + ELE_SIZE]]            -> 0x[[#%x,ARR1_DEV:]],                 20 -> 21
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: arr[2]  absent: 0x[[#%x,ARR + ELE_SIZE + ELE_SIZE]] -> (nil),                               30
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

    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: arr[0] present: 0x[[#%x,ARR]]                       -> 0x[[#%x,ARR]],                       10 -> 10
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: arr[1] present: 0x[[#%x,ARR + ELE_SIZE]]            -> 0x[[#%x,ARR + ELE_SIZE]],            21 -> 21
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: arr[2] present: 0x[[#%x,ARR + ELE_SIZE + ELE_SIZE]] -> 0x[[#%x,ARR + ELE_SIZE + ELE_SIZE]], 30 -> 30
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: arr[0]  absent: 0x[[#%x,ARR]]                       -> (nil),                               10
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: arr[1] present: 0x[[#%x,ARR + ELE_SIZE]]            -> 0x[[#%x,ARR1_DEV]],                  20 -> 21
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: arr[2]  absent: 0x[[#%x,ARR + ELE_SIZE + ELE_SIZE]] -> (nil),                               30
    PRINT_INT(arr[0]);
    PRINT_INT(arr[1]);
    PRINT_INT(arr[2]);

    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: arr[0] present: 0x[[#%x,ARR]]                       -> 0x[[#%x,ARR]],                       10 -> 10
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: arr[1] present: 0x[[#%x,ARR + ELE_SIZE]]            -> 0x[[#%x,ARR + ELE_SIZE]],            21 -> 21
    // OUT-caseEnterExitRoutinesSuccess-HOST-NEXT: arr[2] present: 0x[[#%x,ARR + ELE_SIZE + ELE_SIZE]] -> 0x[[#%x,ARR + ELE_SIZE + ELE_SIZE]], 30 -> 30
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: arr[0]  absent: 0x[[#%x,ARR]]                       -> (nil),                               10
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: arr[1]  absent: 0x[[#%x,ARR + ELE_SIZE]]            -> (nil),                               20
    //  OUT-caseEnterExitRoutinesSuccess-OFF-NEXT: arr[2]  absent: 0x[[#%x,ARR + ELE_SIZE + ELE_SIZE]] -> (nil),                               30
    #pragma acc exit data delete(arr[1:1])
    PRINT_INT(arr[0]);
    PRINT_INT(arr[1]);
    PRINT_INT(arr[2]);
  }
  return 0;
}

CASE(caseCopyinExtendsAfter) {
  //      ERR-caseCopyinExtendsAfter-NEXT: 0x[[#%x,HOST:]], [[#%u,ELE_SIZE:]], [[#%u,SIZE:]]
  // ERR-caseCopyinExtendsAfter-HOST-NEXT: arr[0] present: 0x[[#HOST]]               -> 0x[[#HOST]],               10 -> 10
  // ERR-caseCopyinExtendsAfter-HOST-NEXT: arr[1] present: 0x[[#%x,HOST + ELE_SIZE]] -> 0x[[#%x,HOST + ELE_SIZE]], 20 -> 20
  //  ERR-caseCopyinExtendsAfter-OFF-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#HOST]] ([[#SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#HOST]] ([[#ELE_SIZE]] bytes)
  //                                       # FIXME: getOrAllocTgtPtr is meaningless to users.
  //  ERR-caseCopyinExtendsAfter-OFF-NEXT: Libomptarget error: Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping).
  //  ERR-caseCopyinExtendsAfter-OFF-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
  //  ERR-caseCopyinExtendsAfter-OFF-NEXT: Libomptarget error: Source location information not present. Compile with -g or -gline-tables-only.
  //  ERR-caseCopyinExtendsAfter-OFF-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
  int arr[] = {10, 20};
  fprintf(stderr, "%p, %ld, %ld\n", arr, sizeof *arr, sizeof arr);
  #pragma acc data create(arr[0:1])
  acc_copyin(arr, sizeof arr);
  PRINT_INT_STDERR(arr[0]);
  PRINT_INT_STDERR(arr[1]);
  return 0;
}
CASE(caseCopyinExtendsBefore) {
  //      ERR-caseCopyinExtendsBefore-NEXT: 0x[[#%x,HOST:]], [[#%u,ELE_SIZE:]], [[#%u,SIZE:]]
  // ERR-caseCopyinExtendsBefore-HOST-NEXT: arr[0] present: 0x[[#HOST]]               -> 0x[[#HOST]],               10 -> 10
  // ERR-caseCopyinExtendsBefore-HOST-NEXT: arr[1] present: 0x[[#%x,HOST + ELE_SIZE]] -> 0x[[#%x,HOST + ELE_SIZE]], 20 -> 20
  //  ERR-caseCopyinExtendsBefore-OFF-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#HOST]] ([[#SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#%x,HOST + ELE_SIZE]] ([[#ELE_SIZE]] bytes)
  //                                        # FIXME: getOrAllocTgtPtr is meaningless to users.
  //  ERR-caseCopyinExtendsBefore-OFF-NEXT: Libomptarget error: Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping).
  //  ERR-caseCopyinExtendsBefore-OFF-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
  //  ERR-caseCopyinExtendsBefore-OFF-NEXT: Libomptarget error: Source location information not present. Compile with -g or -gline-tables-only.
  //  ERR-caseCopyinExtendsBefore-OFF-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
  int arr[] = {10, 20};
  fprintf(stderr, "%p, %ld, %ld\n", arr, sizeof *arr, sizeof arr);
  #pragma acc data create(arr[1:1])
  acc_copyin(arr, sizeof arr);
  PRINT_INT_STDERR(arr[0]);
  PRINT_INT_STDERR(arr[1]);
  return 0;
}
CASE(caseCopyinSubsumes) {
  //      ERR-caseCopyinSubsumes-NEXT: 0x[[#%x,HOST:]], [[#%u,ELE_SIZE:]], [[#%u,SIZE:]]
  // ERR-caseCopyinSubsumes-HOST-NEXT: arr[0] present: 0x[[#HOST]]                          -> 0x[[#HOST]],                          10 -> 10
  // ERR-caseCopyinSubsumes-HOST-NEXT: arr[1] present: 0x[[#%x,HOST + ELE_SIZE]]            -> 0x[[#%x,HOST + ELE_SIZE]],            20 -> 20
  // ERR-caseCopyinSubsumes-HOST-NEXT: arr[2] present: 0x[[#%x,HOST + ELE_SIZE + ELE_SIZE]] -> 0x[[#%x,HOST + ELE_SIZE + ELE_SIZE]], 30 -> 30
  //  ERR-caseCopyinSubsumes-OFF-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#HOST]] ([[#SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#%x,HOST + ELE_SIZE]] ([[#ELE_SIZE]] bytes)
  //                                   # FIXME: getOrAllocTgtPtr is meaningless to users.
  //  ERR-caseCopyinSubsumes-OFF-NEXT: Libomptarget error: Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping).
  //  ERR-caseCopyinSubsumes-OFF-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
  //  ERR-caseCopyinSubsumes-OFF-NEXT: Libomptarget error: Source location information not present. Compile with -g or -gline-tables-only.
  //  ERR-caseCopyinSubsumes-OFF-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
  int arr[] = {10, 20, 30};
  fprintf(stderr, "%p, %ld, %ld\n", arr, sizeof *arr, sizeof arr);
  #pragma acc data create(arr[1:1])
  acc_copyin(arr, sizeof arr);
  PRINT_INT_STDERR(arr[0]);
  PRINT_INT_STDERR(arr[1]);
  PRINT_INT_STDERR(arr[2]);
  return 0;
}
CASE(caseCopyinConcat2) {
  //      ERR-caseCopyinConcat2-NEXT: 0x[[#%x,HOST:]], [[#%u,ELE_SIZE:]], [[#%u,SIZE:]]
  // ERR-caseCopyinConcat2-HOST-NEXT: arr[0] present: 0x[[#HOST]]                          -> 0x[[#HOST]],                          10 -> 10
  // ERR-caseCopyinConcat2-HOST-NEXT: arr[1] present: 0x[[#%x,HOST + ELE_SIZE]]            -> 0x[[#%x,HOST + ELE_SIZE]],            20 -> 20
  // ERR-caseCopyinConcat2-HOST-NEXT: arr[2] present: 0x[[#%x,HOST + ELE_SIZE + ELE_SIZE]] -> 0x[[#%x,HOST + ELE_SIZE + ELE_SIZE]], 30 -> 30
  //  ERR-caseCopyinConcat2-OFF-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#HOST]] ([[#SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#%x,HOST]] ([[#ELE_SIZE]] bytes)
  //                                  # FIXME: getOrAllocTgtPtr is meaningless to users.
  //  ERR-caseCopyinConcat2-OFF-NEXT: Libomptarget error: Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping).
  //  ERR-caseCopyinConcat2-OFF-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
  //  ERR-caseCopyinConcat2-OFF-NEXT: Libomptarget error: Source location information not present. Compile with -g or -gline-tables-only.
  //  ERR-caseCopyinConcat2-OFF-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
  int arr[] = {10, 20, 30};
  fprintf(stderr, "%p, %ld, %ld\n", arr, sizeof *arr, sizeof arr);
  #pragma acc data create(arr[0:1])
  #pragma acc data create(arr[1:2])
  acc_copyin(arr, sizeof arr);
  PRINT_INT_STDERR(arr[0]);
  PRINT_INT_STDERR(arr[1]);
  PRINT_INT_STDERR(arr[2]);
  return 0;
}

CASE(caseCreateExtendsAfter) {
  //      ERR-caseCreateExtendsAfter-NEXT: 0x[[#%x,HOST:]], [[#%u,ELE_SIZE:]], [[#%u,SIZE:]]
  // ERR-caseCreateExtendsAfter-HOST-NEXT: arr[0] present: 0x[[#HOST]]               -> 0x[[#HOST]],               10 -> 10
  // ERR-caseCreateExtendsAfter-HOST-NEXT: arr[1] present: 0x[[#%x,HOST + ELE_SIZE]] -> 0x[[#%x,HOST + ELE_SIZE]], 20 -> 20
  //  ERR-caseCreateExtendsAfter-OFF-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#HOST]] ([[#SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#HOST]] ([[#ELE_SIZE]] bytes)
  //                                       # FIXME: getOrAllocTgtPtr is meaningless to users.
  //  ERR-caseCreateExtendsAfter-OFF-NEXT: Libomptarget error: Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping).
  //  ERR-caseCreateExtendsAfter-OFF-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
  //  ERR-caseCreateExtendsAfter-OFF-NEXT: Libomptarget error: Source location information not present. Compile with -g or -gline-tables-only.
  //  ERR-caseCreateExtendsAfter-OFF-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
  int arr[] = {10, 20};
  fprintf(stderr, "%p, %ld, %ld\n", arr, sizeof *arr, sizeof arr);
  #pragma acc data create(arr[0:1])
  acc_create(arr, sizeof arr);
  PRINT_INT_STDERR(arr[0]);
  PRINT_INT_STDERR(arr[1]);
  return 0;
}
CASE(caseCreateExtendsBefore) {
  //      ERR-caseCreateExtendsBefore-NEXT: 0x[[#%x,HOST:]], [[#%u,ELE_SIZE:]], [[#%u,SIZE:]]
  // ERR-caseCreateExtendsBefore-HOST-NEXT: arr[0] present: 0x[[#HOST]]               -> 0x[[#HOST]],               10 -> 10
  // ERR-caseCreateExtendsBefore-HOST-NEXT: arr[1] present: 0x[[#%x,HOST + ELE_SIZE]] -> 0x[[#%x,HOST + ELE_SIZE]], 20 -> 20
  //  ERR-caseCreateExtendsBefore-OFF-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#HOST]] ([[#SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#%x,HOST + ELE_SIZE]] ([[#ELE_SIZE]] bytes)
  //                                        # FIXME: getOrAllocTgtPtr is meaningless to users.
  //  ERR-caseCreateExtendsBefore-OFF-NEXT: Libomptarget error: Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping).
  //  ERR-caseCreateExtendsBefore-OFF-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
  //  ERR-caseCreateExtendsBefore-OFF-NEXT: Libomptarget error: Source location information not present. Compile with -g or -gline-tables-only.
  //  ERR-caseCreateExtendsBefore-OFF-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
  int arr[] = {10, 20};
  fprintf(stderr, "%p, %ld, %ld\n", arr, sizeof *arr, sizeof arr);
  #pragma acc data create(arr[1:1])
  acc_create(arr, sizeof arr);
  PRINT_INT_STDERR(arr[0]);
  PRINT_INT_STDERR(arr[1]);
  return 0;
}
CASE(caseCreateSubsumes) {
  //      ERR-caseCreateSubsumes-NEXT: 0x[[#%x,HOST:]], [[#%u,ELE_SIZE:]], [[#%u,SIZE:]]
  // ERR-caseCreateSubsumes-HOST-NEXT: arr[0] present: 0x[[#HOST]]                          -> 0x[[#HOST]],                          10 -> 10
  // ERR-caseCreateSubsumes-HOST-NEXT: arr[1] present: 0x[[#%x,HOST + ELE_SIZE]]            -> 0x[[#%x,HOST + ELE_SIZE]],            20 -> 20
  // ERR-caseCreateSubsumes-HOST-NEXT: arr[2] present: 0x[[#%x,HOST + ELE_SIZE + ELE_SIZE]] -> 0x[[#%x,HOST + ELE_SIZE + ELE_SIZE]], 30 -> 30
  //  ERR-caseCreateSubsumes-OFF-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#HOST]] ([[#SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#%x,HOST + ELE_SIZE]] ([[#ELE_SIZE]] bytes)
  //                                   # FIXME: getOrAllocTgtPtr is meaningless to users.
  //  ERR-caseCreateSubsumes-OFF-NEXT: Libomptarget error: Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping).
  //  ERR-caseCreateSubsumes-OFF-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
  //  ERR-caseCreateSubsumes-OFF-NEXT: Libomptarget error: Source location information not present. Compile with -g or -gline-tables-only.
  //  ERR-caseCreateSubsumes-OFF-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
  int arr[] = {10, 20, 30};
  fprintf(stderr, "%p, %ld, %ld\n", arr, sizeof *arr, sizeof arr);
  #pragma acc data create(arr[1:1])
  acc_create(arr, sizeof arr);
  PRINT_INT_STDERR(arr[0]);
  PRINT_INT_STDERR(arr[1]);
  PRINT_INT_STDERR(arr[2]);
  return 0;
}
CASE(caseCreateConcat2) {
  //      ERR-caseCreateConcat2-NEXT: 0x[[#%x,HOST:]], [[#%u,ELE_SIZE:]], [[#%u,SIZE:]]
  // ERR-caseCreateConcat2-HOST-NEXT: arr[0] present: 0x[[#HOST]]                          -> 0x[[#HOST]],                          10 -> 10
  // ERR-caseCreateConcat2-HOST-NEXT: arr[1] present: 0x[[#%x,HOST + ELE_SIZE]]            -> 0x[[#%x,HOST + ELE_SIZE]],            20 -> 20
  // ERR-caseCreateConcat2-HOST-NEXT: arr[2] present: 0x[[#%x,HOST + ELE_SIZE + ELE_SIZE]] -> 0x[[#%x,HOST + ELE_SIZE + ELE_SIZE]], 30 -> 30
  //  ERR-caseCreateConcat2-OFF-NEXT: Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#HOST]] ([[#SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#%x,HOST]] ([[#ELE_SIZE]] bytes)
  //                                  # FIXME: getOrAllocTgtPtr is meaningless to users.
  //  ERR-caseCreateConcat2-OFF-NEXT: Libomptarget error: Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping).
  //  ERR-caseCreateConcat2-OFF-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
  //  ERR-caseCreateConcat2-OFF-NEXT: Libomptarget error: Source location information not present. Compile with -g or -gline-tables-only.
  //  ERR-caseCreateConcat2-OFF-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
  int arr[] = {10, 20, 30};
  fprintf(stderr, "%p, %ld, %ld\n", arr, sizeof *arr, sizeof arr);
  #pragma acc data create(arr[0:1])
  #pragma acc data create(arr[1:2])
  acc_create(arr, sizeof arr);
  PRINT_INT_STDERR(arr[0]);
  PRINT_INT_STDERR(arr[1]);
  PRINT_INT_STDERR(arr[2]);
  return 0;
}

CASE(caseUpdateRoutinesSuccess) {
  // Scalar.
  int d = 10, s = 20;
  #pragma acc data create(d, s)
  {
    #pragma acc parallel num_gangs(1)
    {
      d = 11;
      s = 21;
    }
    // OUT-caseUpdateRoutinesSuccess-NEXT: d: 0x[[#%x,D:]] -> 0x[[#%x,D_DEV:]]
    // OUT-caseUpdateRoutinesSuccess-NEXT: s: 0x[[#%x,S:]] -> 0x[[#%x,S_DEV:]]
    printf("d: %p -> %p\n", &d, acc_deviceptr(&d));
    printf("s: %p -> %p\n", &s, acc_deviceptr(&s));
    acc_update_device(&d, sizeof d);
    acc_update_self(&s, sizeof s);
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: d present: 0x[[#D]] -> 0x[[#D]],     11 -> 11
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: s present: 0x[[#S]] -> 0x[[#S]],     21 -> 21
    //  OUT-caseUpdateRoutinesSuccess-OFF-NEXT: d present: 0x[[#D]] -> 0x[[#D_DEV]], 10 -> 10
    //  OUT-caseUpdateRoutinesSuccess-OFF-NEXT: s present: 0x[[#S]] -> 0x[[#S_DEV]], 21 -> 21
    PRINT_INT(d);
    PRINT_INT(s);
  }

  // Array extends after.
  int darr[] = {10, 20, 30}, sarr[] = {40, 50, 60};
  #pragma acc data create(darr, sarr)
  {
    // OUT-caseUpdateRoutinesSuccess-NEXT: darr: 0x[[#%x,DARR0:]] -> 0x[[#%x,DARR0_DEV:]], [[#%u,D_ELE_SIZE:]]
    // OUT-caseUpdateRoutinesSuccess-NEXT: sarr: 0x[[#%x,SARR0:]] -> 0x[[#%x,SARR0_DEV:]], [[#%u,S_ELE_SIZE:]]
    printf("darr: %p -> %p, %zu\n", darr, acc_deviceptr(darr), sizeof *darr);
    printf("sarr: %p -> %p, %zu\n", sarr, acc_deviceptr(sarr), sizeof *sarr);
    #pragma acc parallel num_gangs(1)
    {
      darr[0] = 11;
      darr[1] = 21;
      darr[2] = 31;
      sarr[0] = 41;
      sarr[1] = 51;
      sarr[2] = 61;
    }
    acc_update_device(darr, 2 * sizeof *darr);
    acc_update_self(sarr, 2 * sizeof *sarr);
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: darr[0] present: 0x[[#%x,DARR0]]                    -> 0x[[#%x,DARR0]],                            11 -> 11
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: darr[1] present: 0x[[#%x,DARR1:DARR0 + D_ELE_SIZE]] -> 0x[[#%x,DARR0 + D_ELE_SIZE]],               21 -> 21
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: darr[2] present: 0x[[#%x,DARR2:DARR1 + D_ELE_SIZE]] -> 0x[[#%x,DARR1 + D_ELE_SIZE]],               31 -> 31
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: sarr[0] present: 0x[[#%x,SARR0]]                    -> 0x[[#%x,SARR0]],                            41 -> 41
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: sarr[1] present: 0x[[#%x,SARR1:SARR0 + S_ELE_SIZE]] -> 0x[[#%x,SARR0 + S_ELE_SIZE]],               51 -> 51
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: sarr[2] present: 0x[[#%x,SARR2:SARR1 + S_ELE_SIZE]] -> 0x[[#%x,SARR1 + S_ELE_SIZE]],               61 -> 61
    //  OUT-caseUpdateRoutinesSuccess-OFF-NEXT: darr[0] present: 0x[[#%x,DARR0]]                    -> 0x[[#%x,DARR0_DEV]],                        10 -> 10
    //  OUT-caseUpdateRoutinesSuccess-OFF-NEXT: darr[1] present: 0x[[#%x,DARR1:DARR0 + D_ELE_SIZE]] -> 0x[[#%x,DARR1_DEV:DARR0_DEV + D_ELE_SIZE]], 20 -> 20
    //  OUT-caseUpdateRoutinesSuccess-OFF-NEXT: darr[2] present: 0x[[#%x,DARR2:DARR1 + D_ELE_SIZE]] -> 0x[[#%x,DARR2_DEV:DARR1_DEV + D_ELE_SIZE]], 30 -> 31
    //  OUT-caseUpdateRoutinesSuccess-OFF-NEXT: sarr[0] present: 0x[[#%x,SARR0]]                    -> 0x[[#%x,SARR0_DEV]],                        41 -> 41
    //  OUT-caseUpdateRoutinesSuccess-OFF-NEXT: sarr[1] present: 0x[[#%x,SARR1:SARR0 + S_ELE_SIZE]] -> 0x[[#%x,SARR1_DEV:SARR0_DEV + S_ELE_SIZE]], 51 -> 51
    //  OUT-caseUpdateRoutinesSuccess-OFF-NEXT: sarr[2] present: 0x[[#%x,SARR2:SARR1 + S_ELE_SIZE]] -> 0x[[#%x,SARR2_DEV:SARR1_DEV + S_ELE_SIZE]], 60 -> 61
    PRINT_INT(darr[0]);
    PRINT_INT(darr[1]);
    PRINT_INT(darr[2]);
    PRINT_INT(sarr[0]);
    PRINT_INT(sarr[1]);
    PRINT_INT(sarr[2]);
  }

  // Array extends before.
  darr[0] = 10;
  darr[1] = 20;
  darr[2] = 30;
  sarr[0] = 40;
  sarr[1] = 50;
  sarr[2] = 60;
  #pragma acc data create(darr, sarr)
  {
    // OUT-caseUpdateRoutinesSuccess-NEXT: darr: 0x[[#%x,DARR0:]] -> 0x[[#%x,DARR0_DEV:]], [[#%u,D_ELE_SIZE:]]
    // OUT-caseUpdateRoutinesSuccess-NEXT: sarr: 0x[[#%x,SARR0:]] -> 0x[[#%x,SARR0_DEV:]], [[#%u,S_ELE_SIZE:]]
    printf("darr: %p -> %p, %zu\n", darr, acc_deviceptr(darr), sizeof *darr);
    printf("sarr: %p -> %p, %zu\n", sarr, acc_deviceptr(sarr), sizeof *sarr);
    #pragma acc parallel num_gangs(1)
    {
      darr[0] = 11;
      darr[1] = 21;
      darr[2] = 31;
      sarr[0] = 41;
      sarr[1] = 51;
      sarr[2] = 61;
    }
    acc_update_device(darr + 1, 2 * sizeof *darr);
    acc_update_self(sarr + 1, 2 * sizeof *sarr);
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: darr[0] present: 0x[[#%x,DARR0]]                    -> 0x[[#%x,DARR0]],                            11 -> 11
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: darr[1] present: 0x[[#%x,DARR1:DARR0 + D_ELE_SIZE]] -> 0x[[#%x,DARR0 + D_ELE_SIZE]],               21 -> 21
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: darr[2] present: 0x[[#%x,DARR2:DARR1 + D_ELE_SIZE]] -> 0x[[#%x,DARR1 + D_ELE_SIZE]],               31 -> 31
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: sarr[0] present: 0x[[#%x,SARR0]]                    -> 0x[[#%x,SARR0]],                            41 -> 41
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: sarr[1] present: 0x[[#%x,SARR1:SARR0 + S_ELE_SIZE]] -> 0x[[#%x,SARR0 + S_ELE_SIZE]],               51 -> 51
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: sarr[2] present: 0x[[#%x,SARR2:SARR1 + S_ELE_SIZE]] -> 0x[[#%x,SARR1 + S_ELE_SIZE]],               61 -> 61
    //  OUT-caseUpdateRoutinesSuccess-OFF-NEXT: darr[0] present: 0x[[#%x,DARR0]]                    -> 0x[[#%x,DARR0_DEV]],                        10 -> 11
    //  OUT-caseUpdateRoutinesSuccess-OFF-NEXT: darr[1] present: 0x[[#%x,DARR1:DARR0 + D_ELE_SIZE]] -> 0x[[#%x,DARR1_DEV:DARR0_DEV + D_ELE_SIZE]], 20 -> 20
    //  OUT-caseUpdateRoutinesSuccess-OFF-NEXT: darr[2] present: 0x[[#%x,DARR2:DARR1 + D_ELE_SIZE]] -> 0x[[#%x,DARR2_DEV:DARR1_DEV + D_ELE_SIZE]], 30 -> 30
    //  OUT-caseUpdateRoutinesSuccess-OFF-NEXT: sarr[0] present: 0x[[#%x,SARR0]]                    -> 0x[[#%x,SARR0_DEV]],                        40 -> 41
    //  OUT-caseUpdateRoutinesSuccess-OFF-NEXT: sarr[1] present: 0x[[#%x,SARR1:SARR0 + S_ELE_SIZE]] -> 0x[[#%x,SARR1_DEV:SARR0_DEV + S_ELE_SIZE]], 51 -> 51
    //  OUT-caseUpdateRoutinesSuccess-OFF-NEXT: sarr[2] present: 0x[[#%x,SARR2:SARR1 + S_ELE_SIZE]] -> 0x[[#%x,SARR2_DEV:SARR1_DEV + S_ELE_SIZE]], 61 -> 61
    PRINT_INT(darr[0]);
    PRINT_INT(darr[1]);
    PRINT_INT(darr[2]);
    PRINT_INT(sarr[0]);
    PRINT_INT(sarr[1]);
    PRINT_INT(sarr[2]);
  }

  // Array subsumes.
  darr[0] = 10;
  darr[1] = 20;
  darr[2] = 30;
  sarr[0] = 40;
  sarr[1] = 50;
  sarr[2] = 60;
  #pragma acc data create(darr, sarr)
  {
    // OUT-caseUpdateRoutinesSuccess-NEXT: darr: 0x[[#%x,DARR0:]] -> 0x[[#%x,DARR0_DEV:]], [[#%u,D_ELE_SIZE:]]
    // OUT-caseUpdateRoutinesSuccess-NEXT: sarr: 0x[[#%x,SARR0:]] -> 0x[[#%x,SARR0_DEV:]], [[#%u,S_ELE_SIZE:]]
    printf("darr: %p -> %p, %zu\n", darr, acc_deviceptr(darr), sizeof *darr);
    printf("sarr: %p -> %p, %zu\n", sarr, acc_deviceptr(sarr), sizeof *sarr);
    #pragma acc parallel num_gangs(1)
    {
      darr[0] = 11;
      darr[1] = 21;
      darr[2] = 31;
      sarr[0] = 41;
      sarr[1] = 51;
      sarr[2] = 61;
    }
    acc_update_device(darr + 1, sizeof *darr);
    acc_update_self(sarr + 1, sizeof *sarr);
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: darr[0] present: 0x[[#%x,DARR0]]                    -> 0x[[#%x,DARR0]],                            11 -> 11
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: darr[1] present: 0x[[#%x,DARR1:DARR0 + D_ELE_SIZE]] -> 0x[[#%x,DARR0 + D_ELE_SIZE]],               21 -> 21
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: darr[2] present: 0x[[#%x,DARR2:DARR1 + D_ELE_SIZE]] -> 0x[[#%x,DARR1 + D_ELE_SIZE]],               31 -> 31
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: sarr[0] present: 0x[[#%x,SARR0]]                    -> 0x[[#%x,SARR0]],                            41 -> 41
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: sarr[1] present: 0x[[#%x,SARR1:SARR0 + S_ELE_SIZE]] -> 0x[[#%x,SARR0 + S_ELE_SIZE]],               51 -> 51
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: sarr[2] present: 0x[[#%x,SARR2:SARR1 + S_ELE_SIZE]] -> 0x[[#%x,SARR1 + S_ELE_SIZE]],               61 -> 61
    //  OUT-caseUpdateRoutinesSuccess-OFF-NEXT: darr[0] present: 0x[[#%x,DARR0]]                    -> 0x[[#%x,DARR0_DEV]],                        10 -> 11
    //  OUT-caseUpdateRoutinesSuccess-OFF-NEXT: darr[1] present: 0x[[#%x,DARR1:DARR0 + D_ELE_SIZE]] -> 0x[[#%x,DARR1_DEV:DARR0_DEV + D_ELE_SIZE]], 20 -> 20
    //  OUT-caseUpdateRoutinesSuccess-OFF-NEXT: darr[2] present: 0x[[#%x,DARR2:DARR1 + D_ELE_SIZE]] -> 0x[[#%x,DARR2_DEV:DARR1_DEV + D_ELE_SIZE]], 30 -> 31
    //  OUT-caseUpdateRoutinesSuccess-OFF-NEXT: sarr[0] present: 0x[[#%x,SARR0]]                    -> 0x[[#%x,SARR0_DEV]],                        40 -> 41
    //  OUT-caseUpdateRoutinesSuccess-OFF-NEXT: sarr[1] present: 0x[[#%x,SARR1:SARR0 + S_ELE_SIZE]] -> 0x[[#%x,SARR1_DEV:SARR0_DEV + S_ELE_SIZE]], 51 -> 51
    //  OUT-caseUpdateRoutinesSuccess-OFF-NEXT: sarr[2] present: 0x[[#%x,SARR2:SARR1 + S_ELE_SIZE]] -> 0x[[#%x,SARR2_DEV:SARR1_DEV + S_ELE_SIZE]], 60 -> 61
    PRINT_INT(darr[0]);
    PRINT_INT(darr[1]);
    PRINT_INT(darr[2]);
    PRINT_INT(sarr[0]);
    PRINT_INT(sarr[1]);
    PRINT_INT(sarr[2]);
  }

  // Check case of zero bytes.
  {
    // OUT-caseUpdateRoutinesSuccess-NEXT: arr[0]: 0x[[#%x,ARR0:]]
    // OUT-caseUpdateRoutinesSuccess-NEXT: arr[1]: 0x[[#%x,ARR1:]]
    int arr[] = {10, 20};
    printf("arr[0]: %p\n", &arr[0]);
    printf("arr[1]: %p\n", &arr[1]);

    // When data is absent.
    //
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: arr[0] present: 0x[[#ARR0]] -> 0x[[#ARR0]], 10 -> 10
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: arr[1] present: 0x[[#ARR1]] -> 0x[[#ARR1]], 20 -> 20
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: arr[0] present: 0x[[#ARR0]] -> 0x[[#ARR0]], 10 -> 10
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: arr[1] present: 0x[[#ARR1]] -> 0x[[#ARR1]], 20 -> 20
    // OUT-caseUpdateRoutinesSuccess-OFF-NEXT:  arr[0]  absent: 0x[[#ARR0]] -> (nil),       10
    // OUT-caseUpdateRoutinesSuccess-OFF-NEXT:  arr[1]  absent: 0x[[#ARR1]] -> (nil),       20
    // OUT-caseUpdateRoutinesSuccess-OFF-NEXT:  arr[0]  absent: 0x[[#ARR0]] -> (nil),       10
    // OUT-caseUpdateRoutinesSuccess-OFF-NEXT:  arr[1]  absent: 0x[[#ARR1]] -> (nil),       20
    acc_update_device(&arr[1], 0);
    PRINT_INT(arr[0]);
    PRINT_INT(arr[1]);
    acc_update_self(&arr[1], 0);
    PRINT_INT(arr[0]);
    PRINT_INT(arr[1]);

    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: arr[0] present: 0x[[#ARR0]] -> 0x[[#ARR0]],         11 -> 11
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: arr[1] present: 0x[[#ARR1]] -> 0x[[#ARR1]],         21 -> 21
    //  OUT-caseUpdateRoutinesSuccess-OFF-NEXT: arr[0] present: 0x[[#ARR0]] -> 0x[[#%x,ARR0_DEV:]], 10 -> 11
    //  OUT-caseUpdateRoutinesSuccess-OFF-NEXT: arr[1] present: 0x[[#ARR1]] -> 0x[[#%x,ARR1_DEV:]], 20 -> 21
    #pragma acc enter data create(arr)
    #pragma acc parallel num_gangs(1)
    {
      arr[0] = 11;
      arr[1] = 21;
    }
    PRINT_INT(arr[0]);
    PRINT_INT(arr[1]);

    // When data is present.
    //
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: arr[0] present: 0x[[#ARR0]] -> 0x[[#ARR0]],         11 -> 11
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: arr[1] present: 0x[[#ARR1]] -> 0x[[#ARR1]],         21 -> 21
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: arr[0] present: 0x[[#ARR0]] -> 0x[[#ARR0]],         11 -> 11
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: arr[1] present: 0x[[#ARR1]] -> 0x[[#ARR1]],         21 -> 21
    //  OUT-caseUpdateRoutinesSuccess-OFF-NEXT: arr[0] present: 0x[[#ARR0]] -> 0x[[#%x,ARR0_DEV:]], 10 -> 11
    //  OUT-caseUpdateRoutinesSuccess-OFF-NEXT: arr[1] present: 0x[[#ARR1]] -> 0x[[#%x,ARR1_DEV:]], 20 -> 21
    //  OUT-caseUpdateRoutinesSuccess-OFF-NEXT: arr[0] present: 0x[[#ARR0]] -> 0x[[#%x,ARR0_DEV:]], 10 -> 11
    //  OUT-caseUpdateRoutinesSuccess-OFF-NEXT: arr[1] present: 0x[[#ARR1]] -> 0x[[#%x,ARR1_DEV:]], 20 -> 21
    acc_update_device(&arr[1], 0);
    PRINT_INT(arr[0]);
    PRINT_INT(arr[1]);
    acc_update_self(&arr[1], 0);
    PRINT_INT(arr[0]);
    PRINT_INT(arr[1]);

    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: arr[0] present: 0x[[#ARR0]] -> 0x[[#ARR0]], 11 -> 11
    // OUT-caseUpdateRoutinesSuccess-HOST-NEXT: arr[1] present: 0x[[#ARR1]] -> 0x[[#ARR1]], 21 -> 21
    // OUT-caseUpdateRoutinesSuccess-OFF-NEXT:  arr[0]  absent: 0x[[#ARR0]] -> (nil),       10
    // OUT-caseUpdateRoutinesSuccess-OFF-NEXT:  arr[1]  absent: 0x[[#ARR1]] -> (nil),       20
    #pragma acc exit data delete(arr)
    PRINT_INT(arr[0]);
    PRINT_INT(arr[1]);
  }

  // Check case of null pointer, with non-zero or zero bytes.
  //
  // OUT-caseUpdateRoutinesSuccess-NEXT: NULL present: (nil) -> (nil)
  // OUT-caseUpdateRoutinesSuccess-NEXT: NULL present: (nil) -> (nil)
  // OUT-caseUpdateRoutinesSuccess-NEXT: NULL present: (nil) -> (nil)
  // OUT-caseUpdateRoutinesSuccess-NEXT: NULL present: (nil) -> (nil)
  acc_update_device(NULL, 1);
  printMap("NULL", NULL, 1);
  acc_update_self(NULL, 1);
  printMap("NULL", NULL, 1);
  acc_update_device(NULL, 0);
  printMap("NULL", NULL, 0);
  acc_update_self(NULL, 0);
  printMap("NULL", NULL, 0);
  return 0;
}

CASE(caseUpdateDeviceAbsent) {
  //      ERR-caseUpdateDeviceAbsent-NEXT: 0x[[#%x,HOST:]], [[#%u,SIZE:]]
  // ERR-caseUpdateDeviceAbsent-HOST-NEXT: x present: 0x[[#HOST]] -> 0x[[#HOST]], 10 -> 10
  //  ERR-caseUpdateDeviceAbsent-OFF-NEXT: Libomptarget message: device mapping required by 'present' motion modifier does not exist for host address 0x{{0*}}[[#HOST]] ([[#SIZE]] bytes)
  //  ERR-caseUpdateDeviceAbsent-OFF-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
  //  ERR-caseUpdateDeviceAbsent-OFF-NEXT: Libomptarget error: Source location information not present. Compile with -g or -gline-tables-only.
  //  ERR-caseUpdateDeviceAbsent-OFF-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
  int x = 10;
  fprintf(stderr, "%p, %ld\n", &x, sizeof x);
  acc_update_device(&x, sizeof x);
  PRINT_INT_STDERR(x);
  return 0;
}
CASE(caseUpdateDeviceExtendsAfter) {
  //      ERR-caseUpdateDeviceExtendsAfter-NEXT: 0x[[#%x,HOST:]], [[#%u,ELE_SIZE:]], [[#%u,SIZE:]]
  // ERR-caseUpdateDeviceExtendsAfter-HOST-NEXT: arr[0] present: 0x[[#HOST]]               -> 0x[[#HOST]],               10 -> 10
  // ERR-caseUpdateDeviceExtendsAfter-HOST-NEXT: arr[1] present: 0x[[#%x,HOST + ELE_SIZE]] -> 0x[[#%x,HOST + ELE_SIZE]], 20 -> 20
  //  ERR-caseUpdateDeviceExtendsAfter-OFF-NEXT: Libomptarget message: device mapping required by 'present' motion modifier does not exist for host address 0x{{0*}}[[#HOST]] ([[#SIZE]] bytes)
  //  ERR-caseUpdateDeviceExtendsAfter-OFF-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
  //  ERR-caseUpdateDeviceExtendsAfter-OFF-NEXT: Libomptarget error: Source location information not present. Compile with -g or -gline-tables-only.
  //  ERR-caseUpdateDeviceExtendsAfter-OFF-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
  int arr[] = {10, 20};
  fprintf(stderr, "%p, %ld, %ld\n", arr, sizeof *arr, sizeof arr);
  #pragma acc data create(arr[0:1])
  acc_update_device(arr, sizeof arr);
  PRINT_INT_STDERR(arr[0]);
  PRINT_INT_STDERR(arr[1]);
  return 0;
}
CASE(caseUpdateDeviceExtendsBefore) {
  //      ERR-caseUpdateDeviceExtendsBefore-NEXT: 0x[[#%x,HOST:]], [[#%u,ELE_SIZE:]], [[#%u,SIZE:]]
  // ERR-caseUpdateDeviceExtendsBefore-HOST-NEXT: arr[0] present: 0x[[#HOST]]               -> 0x[[#HOST]],               10 -> 10
  // ERR-caseUpdateDeviceExtendsBefore-HOST-NEXT: arr[1] present: 0x[[#%x,HOST + ELE_SIZE]] -> 0x[[#%x,HOST + ELE_SIZE]], 20 -> 20
  //  ERR-caseUpdateDeviceExtendsBefore-OFF-NEXT: Libomptarget message: device mapping required by 'present' motion modifier does not exist for host address 0x{{0*}}[[#HOST]] ([[#SIZE]] bytes)
  //  ERR-caseUpdateDeviceExtendsBefore-OFF-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
  //  ERR-caseUpdateDeviceExtendsBefore-OFF-NEXT: Libomptarget error: Source location information not present. Compile with -g or -gline-tables-only.
  //  ERR-caseUpdateDeviceExtendsBefore-OFF-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
  int arr[] = {10, 20};
  fprintf(stderr, "%p, %ld, %ld\n", arr, sizeof *arr, sizeof arr);
  #pragma acc data create(arr[1:1])
  acc_update_device(arr, sizeof arr);
  PRINT_INT_STDERR(arr[0]);
  PRINT_INT_STDERR(arr[1]);
  return 0;
}
CASE(caseUpdateDeviceSubsumes) {
  //      ERR-caseUpdateDeviceSubsumes-NEXT: 0x[[#%x,HOST:]], [[#%u,ELE_SIZE:]], [[#%u,SIZE:]]
  // ERR-caseUpdateDeviceSubsumes-HOST-NEXT: arr[0] present: 0x[[#HOST]]                          -> 0x[[#HOST]],                          10 -> 10
  // ERR-caseUpdateDeviceSubsumes-HOST-NEXT: arr[1] present: 0x[[#%x,HOST + ELE_SIZE]]            -> 0x[[#%x,HOST + ELE_SIZE]],            20 -> 20
  // ERR-caseUpdateDeviceSubsumes-HOST-NEXT: arr[2] present: 0x[[#%x,HOST + ELE_SIZE + ELE_SIZE]] -> 0x[[#%x,HOST + ELE_SIZE + ELE_SIZE]], 30 -> 30
  //  ERR-caseUpdateDeviceSubsumes-OFF-NEXT: Libomptarget message: device mapping required by 'present' motion modifier does not exist for host address 0x{{0*}}[[#HOST]] ([[#SIZE]] bytes)
  //  ERR-caseUpdateDeviceSubsumes-OFF-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
  //  ERR-caseUpdateDeviceSubsumes-OFF-NEXT: Libomptarget error: Source location information not present. Compile with -g or -gline-tables-only.
  //  ERR-caseUpdateDeviceSubsumes-OFF-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
  int arr[] = {10, 20, 30};
  fprintf(stderr, "%p, %ld, %ld\n", arr, sizeof *arr, sizeof arr);
  #pragma acc data create(arr[1:1])
  acc_update_device(arr, sizeof arr);
  PRINT_INT_STDERR(arr[0]);
  PRINT_INT_STDERR(arr[1]);
  PRINT_INT_STDERR(arr[2]);
  return 0;
}
CASE(caseUpdateDeviceConcat2) {
  //      ERR-caseUpdateDeviceConcat2-NEXT: 0x[[#%x,HOST:]], [[#%u,ELE_SIZE:]], [[#%u,SIZE:]]
  // ERR-caseUpdateDeviceConcat2-HOST-NEXT: arr[0] present: 0x[[#HOST]]               -> 0x[[#HOST]],               10 -> 10
  // ERR-caseUpdateDeviceConcat2-HOST-NEXT: arr[1] present: 0x[[#%x,HOST + ELE_SIZE]] -> 0x[[#%x,HOST + ELE_SIZE]], 20 -> 20
  //  ERR-caseUpdateDeviceConcat2-OFF-NEXT: Libomptarget message: device mapping required by 'present' motion modifier does not exist for host address 0x{{0*}}[[#HOST]] ([[#SIZE]] bytes)
  //  ERR-caseUpdateDeviceConcat2-OFF-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
  //  ERR-caseUpdateDeviceConcat2-OFF-NEXT: Libomptarget error: Source location information not present. Compile with -g or -gline-tables-only.
  //  ERR-caseUpdateDeviceConcat2-OFF-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
  int arr[] = {10, 20};
  fprintf(stderr, "%p, %ld, %ld\n", arr, sizeof *arr, sizeof arr);
  #pragma acc data create(arr[0:1])
  #pragma acc data create(arr[1:1])
  acc_update_device(arr, sizeof arr);
  PRINT_INT_STDERR(arr[0]);
  PRINT_INT_STDERR(arr[1]);
  return 0;
}

CASE(caseUpdateSelfAbsent) {
  //      ERR-caseUpdateSelfAbsent-NEXT: 0x[[#%x,HOST:]], [[#%u,SIZE:]]
  // ERR-caseUpdateSelfAbsent-HOST-NEXT: x present: 0x[[#HOST]] -> 0x[[#HOST]], 10 -> 10
  //  ERR-caseUpdateSelfAbsent-OFF-NEXT: Libomptarget message: device mapping required by 'present' motion modifier does not exist for host address 0x{{0*}}[[#HOST]] ([[#SIZE]] bytes)
  //  ERR-caseUpdateSelfAbsent-OFF-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
  //  ERR-caseUpdateSelfAbsent-OFF-NEXT: Libomptarget error: Source location information not present. Compile with -g or -gline-tables-only.
  //  ERR-caseUpdateSelfAbsent-OFF-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
  int x = 10;
  fprintf(stderr, "%p, %ld\n", &x, sizeof x);
  acc_update_self(&x, sizeof x);
  PRINT_INT_STDERR(x);
  return 0;
}
CASE(caseUpdateSelfExtendsAfter) {
  //      ERR-caseUpdateSelfExtendsAfter-NEXT: 0x[[#%x,HOST:]], [[#%u,ELE_SIZE:]], [[#%u,SIZE:]]
  // ERR-caseUpdateSelfExtendsAfter-HOST-NEXT: arr[0] present: 0x[[#HOST]]               -> 0x[[#HOST]],               10 -> 10
  // ERR-caseUpdateSelfExtendsAfter-HOST-NEXT: arr[1] present: 0x[[#%x,HOST + ELE_SIZE]] -> 0x[[#%x,HOST + ELE_SIZE]], 20 -> 20
  //  ERR-caseUpdateSelfExtendsAfter-OFF-NEXT: Libomptarget message: device mapping required by 'present' motion modifier does not exist for host address 0x{{0*}}[[#HOST]] ([[#SIZE]] bytes)
  //  ERR-caseUpdateSelfExtendsAfter-OFF-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
  //  ERR-caseUpdateSelfExtendsAfter-OFF-NEXT: Libomptarget error: Source location information not present. Compile with -g or -gline-tables-only.
  //  ERR-caseUpdateSelfExtendsAfter-OFF-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
  int arr[] = {10, 20};
  fprintf(stderr, "%p, %ld, %ld\n", arr, sizeof *arr, sizeof arr);
  #pragma acc data create(arr[0:1])
  acc_update_self(arr, sizeof arr);
  PRINT_INT_STDERR(arr[0]);
  PRINT_INT_STDERR(arr[1]);
  return 0;
}
CASE(caseUpdateSelfExtendsBefore) {
  //      ERR-caseUpdateSelfExtendsBefore-NEXT: 0x[[#%x,HOST:]], [[#%u,ELE_SIZE:]], [[#%u,SIZE:]]
  // ERR-caseUpdateSelfExtendsBefore-HOST-NEXT: arr[0] present: 0x[[#HOST]]               -> 0x[[#HOST]],               10 -> 10
  // ERR-caseUpdateSelfExtendsBefore-HOST-NEXT: arr[1] present: 0x[[#%x,HOST + ELE_SIZE]] -> 0x[[#%x,HOST + ELE_SIZE]], 20 -> 20
  //  ERR-caseUpdateSelfExtendsBefore-OFF-NEXT: Libomptarget message: device mapping required by 'present' motion modifier does not exist for host address 0x{{0*}}[[#HOST]] ([[#SIZE]] bytes)
  //  ERR-caseUpdateSelfExtendsBefore-OFF-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
  //  ERR-caseUpdateSelfExtendsBefore-OFF-NEXT: Libomptarget error: Source location information not present. Compile with -g or -gline-tables-only.
  //  ERR-caseUpdateSelfExtendsBefore-OFF-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
  int arr[] = {10, 20};
  fprintf(stderr, "%p, %ld, %ld\n", arr, sizeof *arr, sizeof arr);
  #pragma acc data create(arr[1:1])
  acc_update_self(arr, sizeof arr);
  PRINT_INT_STDERR(arr[0]);
  PRINT_INT_STDERR(arr[1]);
  return 0;
}
CASE(caseUpdateSelfSubsumes) {
  //      ERR-caseUpdateSelfSubsumes-NEXT: 0x[[#%x,HOST:]], [[#%u,ELE_SIZE:]], [[#%u,SIZE:]]
  // ERR-caseUpdateSelfSubsumes-HOST-NEXT: arr[0] present: 0x[[#HOST]]                          -> 0x[[#HOST]],                          10 -> 10
  // ERR-caseUpdateSelfSubsumes-HOST-NEXT: arr[1] present: 0x[[#%x,HOST + ELE_SIZE]]            -> 0x[[#%x,HOST + ELE_SIZE]],            20 -> 20
  // ERR-caseUpdateSelfSubsumes-HOST-NEXT: arr[2] present: 0x[[#%x,HOST + ELE_SIZE + ELE_SIZE]] -> 0x[[#%x,HOST + ELE_SIZE + ELE_SIZE]], 30 -> 30
  //  ERR-caseUpdateSelfSubsumes-OFF-NEXT: Libomptarget message: device mapping required by 'present' motion modifier does not exist for host address 0x{{0*}}[[#HOST]] ([[#SIZE]] bytes)
  //  ERR-caseUpdateSelfSubsumes-OFF-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
  //  ERR-caseUpdateSelfSubsumes-OFF-NEXT: Libomptarget error: Source location information not present. Compile with -g or -gline-tables-only.
  //  ERR-caseUpdateSelfSubsumes-OFF-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
  int arr[] = {10, 20, 30};
  fprintf(stderr, "%p, %ld, %ld\n", arr, sizeof *arr, sizeof arr);
  #pragma acc data create(arr[1:1])
  acc_update_self(arr, sizeof arr);
  PRINT_INT_STDERR(arr[0]);
  PRINT_INT_STDERR(arr[1]);
  PRINT_INT_STDERR(arr[2]);
  return 0;
}
CASE(caseUpdateSelfConcat2) {
  //      ERR-caseUpdateSelfConcat2-NEXT: 0x[[#%x,HOST:]], [[#%u,ELE_SIZE:]], [[#%u,SIZE:]]
  // ERR-caseUpdateSelfConcat2-HOST-NEXT: arr[0] present: 0x[[#HOST]]               -> 0x[[#HOST]],               10 -> 10
  // ERR-caseUpdateSelfConcat2-HOST-NEXT: arr[1] present: 0x[[#%x,HOST + ELE_SIZE]] -> 0x[[#%x,HOST + ELE_SIZE]], 20 -> 20
  //  ERR-caseUpdateSelfConcat2-OFF-NEXT: Libomptarget message: device mapping required by 'present' motion modifier does not exist for host address 0x{{0*}}[[#HOST]] ([[#SIZE]] bytes)
  //  ERR-caseUpdateSelfConcat2-OFF-NEXT: Libomptarget error: Run with LIBOMPTARGET_INFO=4 to dump host-target pointer mappings.
  //  ERR-caseUpdateSelfConcat2-OFF-NEXT: Libomptarget error: Source location information not present. Compile with -g or -gline-tables-only.
  //  ERR-caseUpdateSelfConcat2-OFF-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
  int arr[] = {10, 20};
  fprintf(stderr, "%p, %ld, %ld\n", arr, sizeof *arr, sizeof arr);
  #pragma acc data create(arr[0:1])
  #pragma acc data create(arr[1:1])
  acc_update_self(arr, sizeof arr);
  PRINT_INT_STDERR(arr[0]);
  PRINT_INT_STDERR(arr[1]);
  return 0;
}

CASE(caseMallocFreeSuccess) {
  // OUT-caseMallocFreeSuccess-NEXT: acc_malloc(0): (nil)
  printf("acc_malloc(0): %p\n", acc_malloc(0));

  // OpenACC 3.1, sec. 3.2.25 "acc_free", L3444-3445:
  // "If the argument is a NULL pointer, no operation is performed."
  acc_free(NULL);

  // OUT-caseMallocFreeSuccess-NEXT: acc_malloc(1) != NULL: 1
  void *ptr = acc_malloc(1);
  printf("acc_malloc(1) != NULL: %d\n", ptr != NULL);
  acc_free(ptr);

  // OUT-caseMallocFreeSuccess-NEXT: acc_malloc(2) != NULL: 1
  ptr = acc_malloc(2);
  printf("acc_malloc(2) != NULL: %d\n", ptr != NULL);
  acc_free(ptr);

  // acc_malloc/acc_free with acc_map_data/acc_unmap_data is checked in
  // caseMapUnmapSuccess.

  return 0;
}

CASE(caseMapUnmapSuccess) {
  int arr[] = {10, 20};
  int *arr_dev;

  // OUT-caseMapUnmapSuccess-NEXT: arr: 0x[[#%x,ARR:]]
  // OUT-caseMapUnmapSuccess-NEXT: element size: [[#%u,ELE_SIZE:]]
  printf("arr: %p\n", arr);
  printf("element size: %lu\n", sizeof *arr);

  // Check the sequence acc_malloc, acc_map_data, acc_unmap_data, and acc_free.
  // Make sure data assignments and transfer work correctly between acc_map_data
  // and acc_unmap_data.
  arr_dev = acc_malloc(sizeof arr);
  if (!arr_dev) {
    fprintf(stderr, "acc_malloc failed\n");
    return 1;
  }
  // OUT-caseMapUnmapSuccess-NEXT: arr_dev: 0x[[#%x,ARR_DEV:]]
  printf("arr_dev: %p\n", arr_dev);
  fflush(stdout);
  // ERR-caseMapUnmapSuccess-HOST-NEXT: OMP: Error #[[#]]: acc_map_data called for shared memory
  acc_map_data(arr, arr_dev, sizeof arr);
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr[0] present: 0x[[#ARR]]               -> 0x[[#ARR_DEV]],               10 -> {{.*}}
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr[1] present: 0x[[#%x,ARR + ELE_SIZE]] -> 0x[[#%x,ARR_DEV + ELE_SIZE]], 20 -> {{.*}}
  PRINT_INT(arr[0]);
  PRINT_INT(arr[1]);
  #pragma acc update device(arr)
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr[0] present: 0x[[#ARR]]               -> 0x[[#ARR_DEV]],               10 -> 10
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr[1] present: 0x[[#%x,ARR + ELE_SIZE]] -> 0x[[#%x,ARR_DEV + ELE_SIZE]], 20 -> 20
  PRINT_INT(arr[0]);
  PRINT_INT(arr[1]);
  #pragma acc parallel num_gangs(1)
  {
    arr[0] += 1;
    arr[1] += 1;
  }
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr[0] present: 0x[[#ARR]]               -> 0x[[#ARR_DEV]],               10 -> 11
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr[1] present: 0x[[#%x,ARR + ELE_SIZE]] -> 0x[[#%x,ARR_DEV + ELE_SIZE]], 20 -> 21
  PRINT_INT(arr[0]);
  PRINT_INT(arr[1]);
  #pragma acc update self(arr)
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr[0] present: 0x[[#ARR]]               -> 0x[[#ARR_DEV]],               11 -> 11
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr[1] present: 0x[[#%x,ARR + ELE_SIZE]] -> 0x[[#%x,ARR_DEV + ELE_SIZE]], 21 -> 21
  PRINT_INT(arr[0]);
  PRINT_INT(arr[1]);
  arr[0] = 10;
  arr[1] = 20;
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr[0] present: 0x[[#ARR]]               -> 0x[[#ARR_DEV]],               10 -> 11
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr[1] present: 0x[[#%x,ARR + ELE_SIZE]] -> 0x[[#%x,ARR_DEV + ELE_SIZE]], 20 -> 21
  PRINT_INT(arr[0]);
  PRINT_INT(arr[1]);
  // Structured reference count is zero.  Dynamic reference count would be zero
  // if not for the preceding acc_map_data.
  acc_unmap_data(arr);
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr[0]  absent: 0x[[#ARR]]               -> (nil),                        10
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr[1]  absent: 0x[[#%x,ARR + ELE_SIZE]] -> (nil),                        20
  PRINT_INT(arr[0]);
  PRINT_INT(arr[1]);
  acc_free(arr_dev);
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr[0]  absent: 0x[[#ARR]]               -> (nil),                        10
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr[1]  absent: 0x[[#%x,ARR + ELE_SIZE]] -> (nil),                        20
  PRINT_INT(arr[0]);
  PRINT_INT(arr[1]);

  // OpenACC 3.1, sec 3.2.33 "acc_unmap_data", L3655-3656:
  // "After unmapping memory the dynamic reference count for the pointer is set
  // to zero, but no data movement will occur."
  //
  // Check cases where the dynamic reference count would be =1 or >1 at the
  // acc_unmap_data if not for the preceding acc_map_data.
  arr_dev = acc_malloc(sizeof arr);
  if (!arr_dev) {
    fprintf(stderr, "acc_malloc failed\n");
    return 1;
  }
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr_dev: 0x[[#%x,ARR_DEV:]]
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
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr[0] present: 0x[[#ARR]]               -> 0x[[#ARR_DEV]],               10 -> 11
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr[1] present: 0x[[#%x,ARR + ELE_SIZE]] -> 0x[[#%x,ARR_DEV + ELE_SIZE]], 20 -> 21
  PRINT_INT(arr[0]);
  PRINT_INT(arr[1]);
  // Dynamic reference count would be 1 if not for acc_map_data.
  acc_unmap_data(arr);
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr[0]  absent: 0x[[#ARR]]               -> (nil),                        10
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr[1]  absent: 0x[[#%x,ARR + ELE_SIZE]] -> (nil),                        20
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
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr[0] present: 0x[[#ARR]]               -> 0x[[#ARR_DEV]],               10 -> 11
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr[1] present: 0x[[#%x,ARR + ELE_SIZE]] -> 0x[[#%x,ARR_DEV + ELE_SIZE]], 20 -> 21
  PRINT_INT(arr[0]);
  PRINT_INT(arr[1]);
  // Dynamic reference count would be > 1 if not for acc_map_data.
  acc_unmap_data(arr);
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr[0]  absent: 0x[[#ARR]]               -> (nil),                        10
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr[1]  absent: 0x[[#%x,ARR + ELE_SIZE]] -> (nil),                        20
  PRINT_INT(arr[0]);
  PRINT_INT(arr[1]);

  // OpenACC 3.1, sec. 3.2.32 "acc_map_data", L3641-3642:
  // "Memory mapped by acc_map_data may not have the associated dynamic
  // reference count decremented to zero, except by a call to acc_unmap_data."
  //
  // It's important to check all ways that the dynamic reference counter can be
  // decremented.  For example, at one time, only acc_copyout_finalize was
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
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr[0] present: 0x[[#ARR]]               -> 0x[[#ARR_DEV]],               10 -> 11
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr[1] present: 0x[[#%x,ARR + ELE_SIZE]] -> 0x[[#%x,ARR_DEV + ELE_SIZE]], 20 -> 21
  PRINT_INT(arr[0]);
  PRINT_INT(arr[1]);
  acc_unmap_data(arr);
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr[0]  absent: 0x[[#ARR]]               -> (nil),                        10
  // OUT-caseMapUnmapSuccess-OFF-NEXT: arr[1]  absent: 0x[[#%x,ARR + ELE_SIZE]] -> (nil),                        20
  PRINT_INT(arr[0]);
  PRINT_INT(arr[1]);

  acc_free(arr_dev);
  return 0;
}
CASE(caseMapBytesZero) {
  int x = 10;
  int *dev = acc_malloc(sizeof *dev); // unmapped device address

  // OUT-caseMapBytesZero-NEXT: &x: 0x[[#%x,X:]]
  // OUT-caseMapBytesZero-NEXT: dev: 0x[[#%x,DEV:]]
  printf("&x: %p\n", &x);
  printf("dev: %p\n", dev);

  // Check when host and/or device addresses are already mapped.
  //
  // acc_map_data should then be a no-op: there should be no runtime error even
  // for null pointers, and the existing mapping should not be affected.
  #pragma acc enter data create(x)
  #pragma acc parallel num_gangs(1) present(x)
  x = 11;
  // OUT-caseMapBytesZero-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]],         11 -> 11
  //  OUT-caseMapBytesZero-OFF-NEXT: x present: 0x[[#X]] -> 0x[[#%x,X_DEV:]], 10 -> 11
  PRINT_INT(x);
  acc_map_data(&x, acc_deviceptr(&x), 0);
  acc_map_data(&x, NULL, 0);
  acc_map_data(NULL, acc_deviceptr(&x), 0);
  acc_map_data(NULL, NULL, 0);
  // OUT-caseMapBytesZero-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]],     11 -> 11
  //  OUT-caseMapBytesZero-OFF-NEXT: x present: 0x[[#X]] -> 0x[[#X_DEV]], 10 -> 11
  //      OUT-caseMapBytesZero-NEXT: NULL present: (nil) -> (nil)
  PRINT_INT(x);
  printMap("NULL", NULL, 0);
  #pragma acc exit data delete(x)

  // Again, but when host and/or device addresses are not already mapped.
  x = 10;
  // OUT-caseMapBytesZero-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]], 10 -> 10
  //  OUT-caseMapBytesZero-OFF-NEXT: x absent:  0x[[#X]] -> (nil),    10
  // OUT-caseMapBytesZero-HOST-NEXT: dev: 0x[[#DEV]] <- 0x[[#DEV]]
  //  OUT-caseMapBytesZero-OFF-NEXT: dev: 0x[[#DEV]] <- (nil)
  PRINT_INT(x);
  printf("dev: %p <- %p\n", dev, acc_hostptr(dev));
  acc_map_data(&x, dev, 0);
  acc_map_data(&x, NULL, 0);
  acc_map_data(NULL, dev, 0);
  acc_map_data(NULL, NULL, 0);
  // OUT-caseMapBytesZero-HOST-NEXT: x present: 0x[[#X]] -> 0x[[#X]], 10 -> 10
  //  OUT-caseMapBytesZero-OFF-NEXT: x absent:  0x[[#X]] -> (nil),    10
  // OUT-caseMapBytesZero-HOST-NEXT: dev: 0x[[#DEV]] <- 0x[[#DEV]]
  //  OUT-caseMapBytesZero-OFF-NEXT: dev: 0x[[#DEV]] <- (nil)
  //      OUT-caseMapBytesZero-NEXT: NULL present: (nil) -> (nil)
  PRINT_INT(x);
  printf("dev: %p <- %p\n", dev, acc_hostptr(dev));
  printMap("NULL", NULL, 0);

  acc_free(dev);
  return 0;
}
// OpenACC 3.1, sec. 3.2.32 "acc_map_data", L3637-3638:
// "It is an error to call acc_map_data for host data that is already present in
// the current device memory."
CASE(caseMapSameHostAsStructured) {
  int arr[] = {10, 20};
  int *arr_dev = acc_malloc(sizeof arr);
  if (!arr_dev) {
    fprintf(stderr, "acc_malloc failed\n");
    return 1;
  }
  // ERR-caseMapSameHostAsStructured-HOST-NEXT: OMP: Error #[[#]]: acc_map_data called for shared memory
  #pragma acc data create(arr)
  // ERR-caseMapSameHostAsStructured-OFF-NEXT: OMP: Error #[[#]]: acc_map_data called with host pointer that is already mapped
  acc_map_data(arr, arr_dev, sizeof arr);
  return 0;
}
CASE(caseMapSameHostAsDynamic) {
  int arr[] = {10, 20};
  int *arr_dev = acc_malloc(sizeof arr);
  if (!arr_dev) {
    fprintf(stderr, "acc_malloc failed\n");
    return 1;
  }
  // ERR-caseMapSameHostAsDynamic-HOST-NEXT: OMP: Error #[[#]]: acc_map_data called for shared memory
  #pragma acc enter data create(arr)
  // ERR-caseMapSameHostAsDynamic-OFF-NEXT: OMP: Error #[[#]]: acc_map_data called with host pointer that is already mapped
  acc_map_data(arr, arr_dev, sizeof arr);
  return 0;
}
CASE(caseMapSame) {
  int arr[] = {10, 20};
  int *arr_dev = acc_malloc(sizeof arr);
  if (!arr_dev) {
    fprintf(stderr, "acc_malloc failed\n");
    return 1;
  }
  // ERR-caseMapSame-HOST-NEXT: OMP: Error #[[#]]: acc_map_data called for shared memory
  acc_map_data(arr, arr_dev, sizeof arr);
  // ERR-caseMapSame-OFF-NEXT: OMP: Error #[[#]]: acc_map_data called with host pointer that is already mapped
  acc_map_data(arr, arr_dev, sizeof arr);
  return 0;
}
CASE(caseMapSameHost) {
  int arr[] = {10, 20};
  int *arr_dev0 = acc_malloc(sizeof arr);
  int *arr_dev1 = acc_malloc(sizeof arr);
  if (!arr_dev0 || !arr_dev1) {
    fprintf(stderr, "acc_malloc failed\n");
    return 1;
  }
  // ERR-caseMapSameHost-HOST-NEXT: OMP: Error #[[#]]: acc_map_data called for shared memory
  acc_map_data(arr, arr_dev0, sizeof arr);
  // ERR-caseMapSameHost-OFF-NEXT: OMP: Error #[[#]]: acc_map_data called with host pointer that is already mapped
  acc_map_data(arr, arr_dev1, sizeof arr);
  return 0;
}
CASE(caseMapHostExtendsAfter) {
  int arr[] = {10, 20};
  int *arr_dev0 = acc_malloc(sizeof *arr);
  int *arr_dev1 = acc_malloc(sizeof arr);
  if (!arr_dev0 || !arr_dev1) {
    fprintf(stderr, "acc_malloc failed\n");
    return 1;
  }
  // ERR-caseMapHostExtendsAfter-HOST-NEXT: OMP: Error #[[#]]: acc_map_data called for shared memory
  acc_map_data(arr, arr_dev0, sizeof *arr);
  // ERR-caseMapHostExtendsAfter-OFF-NEXT: OMP: Error #[[#]]: acc_map_data called with host pointer that is already mapped
  acc_map_data(arr, arr_dev1, sizeof arr);
  return 0;
}
CASE(caseMapHostExtendsBefore) {
  int arr[] = {10, 20};
  int *arr_dev0 = acc_malloc(sizeof *arr);
  int *arr_dev1 = acc_malloc(sizeof arr);
  if (!arr_dev0 || !arr_dev1) {
    fprintf(stderr, "acc_malloc failed\n");
    return 1;
  }
  // ERR-caseMapHostExtendsBefore-HOST-NEXT: OMP: Error #[[#]]: acc_map_data called for shared memory
  acc_map_data(arr+1, arr_dev0, sizeof *arr);
  // ERR-caseMapHostExtendsBefore-OFF-NEXT: OMP: Error #[[#]]: acc_map_data called with host pointer that is already mapped
  acc_map_data(arr, arr_dev1, sizeof arr);
  return 0;
}
CASE(caseMapHostSubsumes) {
  int arr[] = {10, 20, 30};
  int *arr_dev0 = acc_malloc(sizeof *arr);
  int *arr_dev1 = acc_malloc(sizeof arr);
  if (!arr_dev0 || !arr_dev1) {
    fprintf(stderr, "acc_malloc failed\n");
    return 1;
  }
  // ERR-caseMapHostSubsumes-HOST-NEXT: OMP: Error #[[#]]: acc_map_data called for shared memory
  acc_map_data(arr+1, arr_dev0, sizeof *arr);
  // ERR-caseMapHostSubsumes-OFF-NEXT: OMP: Error #[[#]]: acc_map_data called with host pointer that is already mapped
  acc_map_data(arr, arr_dev1, sizeof arr);
  return 0;
}
CASE(caseMapHostIsSubsumed) {
  int arr[] = {10, 20, 30};
  int *arr_dev0 = acc_malloc(sizeof arr);
  int *arr_dev1 = acc_malloc(sizeof *arr);
  if (!arr_dev0 || !arr_dev1) {
    fprintf(stderr, "acc_malloc failed\n");
    return 1;
  }
  // ERR-caseMapHostIsSubsumed-HOST-NEXT: OMP: Error #[[#]]: acc_map_data called for shared memory
  acc_map_data(arr, arr_dev0, sizeof arr);
  // ERR-caseMapHostIsSubsumed-OFF-NEXT: OMP: Error #[[#]]: acc_map_data called with host pointer that is already mapped
  acc_map_data(arr+1, arr_dev1, sizeof *arr);
  return 0;
}

CASE(caseMapHostNull) {
  int *arr_dev = acc_malloc(sizeof *arr_dev);
  if (!arr_dev) {
    fprintf(stderr, "acc_malloc failed\n");
    return 1;
  }
  // ERR-caseMapHostNull-NEXT: OMP: Error #[[#]]: acc_map_data called with null host pointer
  acc_map_data(NULL, arr_dev, sizeof *arr_dev);
  return 0;
}
CASE(caseMapDevNull) {
  int arr[] = {10, 20};
  // ERR-caseMapDevNull-NEXT: OMP: Error #[[#]]: acc_map_data called with null device pointer
  acc_map_data(arr, NULL, sizeof arr);
  return 0;
}
CASE(caseMapBothNull) {
  // ERR-caseMapBothNull-NEXT: OMP: Error #[[#]]: acc_map_data called with null host pointer
  acc_map_data(NULL, NULL, 1);
  return 0;
}
CASE(caseUnmapNull) {
  // ERR-caseUnmapNull-NEXT: OMP: Error #[[#]]: acc_unmap_data called with null pointer
  acc_unmap_data(NULL);
  return 0;
}

// OpenACC 3.1, sec. 3.2.33 "acc_unmap_data", L3653-3655:
// "It is undefined behavior to call acc_unmap_data with a host address unless
// that host address was mapped to device memory using acc_map_data."
CASE(caseUnmapUnmapped) {
  int arr[] = {10, 20};
  // ERR-caseUnmapUnmapped-HOST-NEXT: OMP: Error #[[#]]: acc_unmap_data called for shared memory
  //  ERR-caseUnmapUnmapped-OFF-NEXT: Libomptarget error: Association not found
  //  ERR-caseUnmapUnmapped-OFF-NEXT: OMP: Error #[[#]]: acc_unmap_data failed
  acc_unmap_data(arr);
  return 0;
}
CASE(caseUnmapAfterOnlyStructured) {
  int arr[] = {10, 20};
  #pragma acc data create(arr)
  // ERR-caseUnmapAfterOnlyStructured-HOST-NEXT: OMP: Error #[[#]]: acc_unmap_data called for shared memory
  //  ERR-caseUnmapAfterOnlyStructured-OFF-NEXT: Libomptarget error: Trying to disassociate a pointer with a non-zero hold reference count
  //  ERR-caseUnmapAfterOnlyStructured-OFF-NEXT: OMP: Error #[[#]]: acc_unmap_data failed
  acc_unmap_data(arr);
  return 0;
}
CASE(caseUnmapAfterOnlyDynamic) {
  int arr[] = {10, 20};
  #pragma acc enter data create(arr)
  // ERR-caseUnmapAfterOnlyDynamic-HOST-NEXT: OMP: Error #[[#]]: acc_unmap_data called for shared memory
  //  ERR-caseUnmapAfterOnlyDynamic-OFF-NEXT: Libomptarget error: Trying to disassociate a pointer which was not mapped via omp_target_associate_ptr
  //  ERR-caseUnmapAfterOnlyDynamic-OFF-NEXT: OMP: Error #[[#]]: acc_unmap_data failed
  acc_unmap_data(arr);
  return 0;
}

// OpenACC 3.1, sec 3.2.33 "acc_unmap_data", L3656-3657:
// "It is an error to call acc_unmap_data if the structured reference count for
// the pointer is not zero."
CASE(caseUnmapAfterMapAndStructured) {
  int arr[] = {10, 20};
  int *arr_dev = acc_malloc(sizeof arr);
  if (!arr_dev) {
    fprintf(stderr, "acc_malloc failed\n");
    return 1;
  }
  // ERR-caseUnmapAfterMapAndStructured-HOST-NEXT: OMP: Error #[[#]]: acc_map_data called for shared memory
  acc_map_data(arr, arr_dev, sizeof arr);
  #pragma acc data create(arr)
  // ERR-caseUnmapAfterMapAndStructured-OFF-NEXT: Libomptarget error: Trying to disassociate a pointer with a non-zero hold reference count
  // ERR-caseUnmapAfterMapAndStructured-OFF-NEXT: OMP: Error #[[#]]: acc_unmap_data failed
  acc_unmap_data(arr);
  return 0;
}
CASE(caseUnmapAfterAllThree) {
  int arr[] = {10, 20};
  int *arr_dev = acc_malloc(sizeof arr);
  if (!arr_dev) {
    fprintf(stderr, "acc_malloc failed\n");
    return 1;
  }
  // ERR-caseUnmapAfterAllThree-HOST-NEXT: OMP: Error #[[#]]: acc_map_data called for shared memory
  acc_map_data(arr, arr_dev, sizeof arr);
  #pragma acc enter data create(arr)
  #pragma acc data create(arr)
  // ERR-caseUnmapAfterAllThree-OFF-NEXT: Libomptarget error: Trying to disassociate a pointer with a non-zero hold reference count
  // ERR-caseUnmapAfterAllThree-OFF-NEXT: OMP: Error #[[#]]: acc_unmap_data failed
  acc_unmap_data(arr);
  return 0;
}

CASE(caseMemcpySuccess) {
  // Put some info about the devices in the output to help with debugging.
  //
  // OUT-caseMemcpySuccess-NEXT: devTypeInit = acc_device_{{.*}}
  // OUT-caseMemcpySuccess-NEXT: numDevsOfTypeInit = [[#]]
  // OUT-caseMemcpySuccess-NEXT: devNumInit = [[#]]
  // OUT-caseMemcpySuccess-NEXT: devNumOther = [[#]]
  // OUT-caseMemcpySuccess-NEXT: devNumInvalid = [[#]]
  //
  // The purpose of devNumOther is to enable testing acc_memcpy_d2d with
  // multiple devices (necessarily of the same type) when we have them.
  // When devNumInit = devNumOther (always true when not offloading), we
  // don't, so we just fake the expected results to make FileCheck happy.
  // TODO: Testing would be cleaner and more careful if we knew how many
  // devices we have before starting the test.
  acc_device_t devTypeInit = acc_get_device_type();
  int numDevsOfTypeInit = acc_get_num_devices(devTypeInit);
  int devNumInit = acc_get_device_num(devTypeInit);
  int devNumOther = (devNumInit + 1) % numDevsOfTypeInit;
  int devNumInvalid = getDevNumInvalid();
  printf("devTypeInit = %s\n", deviceTypeToStr(devTypeInit));
  printf("numDevsOfTypeInit = %d\n", numDevsOfTypeInit);
  printf("devNumInit = %d\n", devNumInit);
  printf("devNumOther = %d\n", devNumOther);
  printf("devNumInvalid = %d\n", devNumInvalid);
  fflush(stdout);

  // Copy between addresses that are not mapped to anything (unless shared
  // memory).  Unlike update routines, these routines shouldn't have errors.
  {
    // OUT-caseMemcpySuccess-NEXT: host[0]: 0x[[#%x,HOST0:]]
    // OUT-caseMemcpySuccess-NEXT: host[1]: 0x[[#%x,HOST1:]]
    // OUT-caseMemcpySuccess-NEXT: host[2]: 0x[[#%x,HOST2:]]
    // OUT-caseMemcpySuccess-NEXT:  dev[0]: 0x[[#%x,DEV0:]]
    // OUT-caseMemcpySuccess-NEXT:  dev[1]: 0x[[#%x,DEV1:]]
    // OUT-caseMemcpySuccess-NEXT:  dev[2]: 0x[[#%x,DEV2:]]
    int host[] = {10, 20, 30};
    int valsForDev[] = {41, 51, 61};
    int *dev = acc_malloc(sizeof valsForDev);
    if (Offloading) {
      acc_map_data(valsForDev, dev, sizeof valsForDev);
      #pragma acc update device(valsForDev)
      acc_unmap_data(valsForDev);
    } else {
      memcpy(dev, valsForDev, sizeof valsForDev);
    }
    printf("host[0]: %p\n", &host[0]);
    printf("host[1]: %p\n", &host[1]);
    printf("host[2]: %p\n", &host[2]);
    printf("dev[0]: %p\n", &dev[0]);
    printf("dev[1]: %p\n", &dev[1]);
    printf("dev[2]: %p\n", &dev[2]);

    // OUT-caseMemcpySuccess-HOST-NEXT: dev[1]   mapped: 0x[[#DEV1]] <- 0x[[#DEV1]], 10 <- 10
    //  OUT-caseMemcpySuccess-OFF-NEXT: dev[1] unmapped: 0x[[#DEV1]] <- (nil),       10
    acc_memcpy_to_device(&dev[1], &host[0], sizeof dev[1]);
    PRINT_DEV_INT(dev[1]);

    // OUT-caseMemcpySuccess-HOST-NEXT: host[1] present: 0x[[#HOST1]] -> 0x[[#HOST1]], 41 -> 41
    //  OUT-caseMemcpySuccess-OFF-NEXT: host[1]  absent: 0x[[#HOST1]] -> (nil),        41
    acc_memcpy_from_device(&host[1], &dev[0], sizeof host[1]);
    PRINT_INT(host[1]);

    // OUT-caseMemcpySuccess-HOST-NEXT: dev[1]   mapped: 0x[[#DEV1]] <- 0x[[#DEV1]], 41 <- 41
    //  OUT-caseMemcpySuccess-OFF-NEXT: dev[1] unmapped: 0x[[#DEV1]] <- (nil),       41
    acc_memcpy_device(&dev[1], &dev[0], sizeof dev[1]);
    PRINT_DEV_INT(dev[1]);

    // OUT-caseMemcpySuccess-HOST-NEXT: host[1] present: 0x[[#HOST1]] -> 0x[[#HOST1]], 10 -> 10
    //  OUT-caseMemcpySuccess-OFF-NEXT: host[1]  absent: 0x[[#HOST1]] -> (nil),        10
    acc_set_device_type(acc_device_host);
    acc_memcpy_d2d(&host[1], &host[0], sizeof host[1], 0, 0);
    acc_set_device_num(devNumInit, devTypeInit);
    PRINT_INT(host[1]);

    // No-op.
    // OUT-caseMemcpySuccess-HOST-NEXT: dev[1]   mapped: 0x[[#DEV1]] <- 0x[[#DEV1]], 41 <- 41
    //  OUT-caseMemcpySuccess-OFF-NEXT: dev[1] unmapped: 0x[[#DEV1]] <- (nil),       41
    acc_memcpy_device(&dev[1], &dev[1], sizeof dev[1]);
    PRINT_DEV_INT(dev[1]);

    // No-op.
    // OUT-caseMemcpySuccess-HOST-NEXT: host[1] present: 0x[[#HOST1]] -> 0x[[#HOST1]], 10 -> 10
    //  OUT-caseMemcpySuccess-OFF-NEXT: host[1]  absent: 0x[[#HOST1]] -> (nil),        10
    acc_set_device_type(acc_device_host);
    acc_memcpy_d2d(&host[1], &host[1], sizeof host[1], 0, 0);
    acc_set_device_num(devNumInit, devTypeInit);
    PRINT_INT(host[1]);

    // Check that we didn't disturb neighboring data or source data.
    // OUT-caseMemcpySuccess-HOST-NEXT: host[0]  present: 0x[[#HOST0]] -> 0x[[#HOST0]], 10 -> 10
    // OUT-caseMemcpySuccess-HOST-NEXT: host[2]  present: 0x[[#HOST2]] -> 0x[[#HOST2]], 30 -> 30
    // OUT-caseMemcpySuccess-HOST-NEXT: dev[0]    mapped: 0x[[#DEV0]]  <- 0x[[#DEV0]],  41 <- 41
    // OUT-caseMemcpySuccess-HOST-NEXT: dev[2]    mapped: 0x[[#DEV2]]  <- 0x[[#DEV2]],  61 <- 61
    //  OUT-caseMemcpySuccess-OFF-NEXT: host[0]   absent: 0x[[#HOST0]] -> (nil),        10
    //  OUT-caseMemcpySuccess-OFF-NEXT: host[2]   absent: 0x[[#HOST2]] -> (nil),        30
    //  OUT-caseMemcpySuccess-OFF-NEXT: dev[0]  unmapped: 0x[[#DEV0]]  <- (nil),        41
    //  OUT-caseMemcpySuccess-OFF-NEXT: dev[2]  unmapped: 0x[[#DEV2]]  <- (nil),        61
    PRINT_INT(host[0]);
    PRINT_INT(host[2]);
    PRINT_DEV_INT(dev[0]);
    PRINT_DEV_INT(dev[2]);

    acc_free(dev);
  }

  // Copy between addresses that are mapped but not to each other, are not
  // mapped to the same host address, and are not the same.
  {
    // OUT-caseMemcpySuccess-NEXT: dst[0]: 0x[[#%x,DST0:]] -> 0x[[#%x,DST0_DEV:]]
    // OUT-caseMemcpySuccess-NEXT: dst[1]: 0x[[#%x,DST1:]] -> 0x[[#%x,DST1_DEV:]]
    // OUT-caseMemcpySuccess-NEXT: dst[2]: 0x[[#%x,DST2:]] -> 0x[[#%x,DST2_DEV:]]
    // OUT-caseMemcpySuccess-NEXT: src[0]: 0x[[#%x,SRC0:]] -> 0x[[#%x,SRC0_DEV:]]
    // OUT-caseMemcpySuccess-NEXT: src[1]: 0x[[#%x,SRC1:]] -> 0x[[#%x,SRC1_DEV:]]
    // OUT-caseMemcpySuccess-NEXT: src[2]: 0x[[#%x,SRC2:]] -> 0x[[#%x,SRC2_DEV:]]
    int dst[] = {10, 20, 30};
    int src[] = {40, 50, 60};
    #pragma acc enter data create(dst, src)
    #pragma acc parallel num_gangs(1)
    {
      dst[0] = 11;
      dst[1] = 21;
      dst[2] = 31;
      src[0] = 41;
      src[1] = 51;
      src[2] = 61;
    }
    printf("dst[0]: %p -> %p\n", &dst[0], acc_deviceptr(&dst[0]));
    printf("dst[1]: %p -> %p\n", &dst[1], acc_deviceptr(&dst[1]));
    printf("dst[2]: %p -> %p\n", &dst[2], acc_deviceptr(&dst[2]));
    printf("src[0]: %p -> %p\n", &src[0], acc_deviceptr(&src[0]));
    printf("src[1]: %p -> %p\n", &src[1], acc_deviceptr(&src[1]));
    printf("src[2]: %p -> %p\n", &src[2], acc_deviceptr(&src[2]));
    // OUT-caseMemcpySuccess-OFF-NEXT: dst[0] on other device: 0x[[#%x,DST0_DEV2:]]
    // OUT-caseMemcpySuccess-OFF-NEXT: dst[1] on other device: 0x[[#%x,DST1_DEV2:]]
    // OUT-caseMemcpySuccess-OFF-NEXT: dst[2] on other device: 0x[[#%x,DST2_DEV2:]]
    // OUT-caseMemcpySuccess-OFF-NEXT: src[0] on other device: 0x[[#%x,SRC0_DEV2:]]
    // OUT-caseMemcpySuccess-OFF-NEXT: src[1] on other device: 0x[[#%x,SRC1_DEV2:]]
    // OUT-caseMemcpySuccess-OFF-NEXT: src[2] on other device: 0x[[#%x,SRC2_DEV2:]]
    if (devNumInit != devNumOther) {
      acc_set_device_num(devNumOther, devTypeInit);
      #pragma acc enter data create(dst, src)
      #pragma acc parallel num_gangs(1)
      {
        dst[0] = 12;
        dst[1] = 22;
        dst[2] = 32;
        src[0] = 42;
        src[1] = 52;
        src[2] = 62;
      }
      printf("dst[0] on other device: %p\n", acc_deviceptr(&dst[0]));
      printf("dst[1] on other device: %p\n", acc_deviceptr(&dst[1]));
      printf("dst[2] on other device: %p\n", acc_deviceptr(&dst[2]));
      printf("src[0] on other device: %p\n", acc_deviceptr(&src[0]));
      printf("src[1] on other device: %p\n", acc_deviceptr(&src[1]));
      printf("src[2] on other device: %p\n", acc_deviceptr(&src[2]));
      acc_set_device_num(devNumInit, devTypeInit);
    } else if (Offloading) {
      printf("dst[0] on other device: 0x0\n");
      printf("dst[1] on other device: 0x0\n");
      printf("dst[2] on other device: 0x0\n");
      printf("src[0] on other device: 0x0\n");
      printf("src[1] on other device: 0x0\n");
      printf("src[2] on other device: 0x0\n");
    }

    // OUT-caseMemcpySuccess-HOST-NEXT: dst[1] present: 0x[[#DST1]] -> 0x[[#DST1]],     51 -> 51
    //  OUT-caseMemcpySuccess-OFF-NEXT: dst[1] present: 0x[[#DST1]] -> 0x[[#DST1_DEV]], 20 -> 50
    acc_memcpy_to_device(acc_deviceptr(&dst[1]), &src[1], sizeof dst[1]);
    PRINT_INT(dst[1]);
    dst[1] = 20;
    #pragma acc parallel num_gangs(1)
    dst[1] = 21;

    // OUT-caseMemcpySuccess-HOST-NEXT: dst[1] present: 0x[[#DST1]] -> 0x[[#DST1]],     51 -> 51
    //  OUT-caseMemcpySuccess-OFF-NEXT: dst[1] present: 0x[[#DST1]] -> 0x[[#DST1_DEV]], 51 -> 21
    acc_memcpy_from_device(&dst[1], acc_deviceptr(&src[1]), sizeof dst[1]);
    PRINT_INT(dst[1]);
    dst[1] = 20;
    #pragma acc parallel num_gangs(1)
    dst[1] = 21;

    // OUT-caseMemcpySuccess-HOST-NEXT: dst[1] present: 0x[[#DST1]] -> 0x[[#DST1]],     51 -> 51
    //  OUT-caseMemcpySuccess-OFF-NEXT: dst[1] present: 0x[[#DST1]] -> 0x[[#DST1_DEV]], 20 -> 51
    acc_memcpy_device(acc_deviceptr(&dst[1]), acc_deviceptr(&src[1]), sizeof dst[1]);
    PRINT_INT(dst[1]);
    dst[1] = 20;
    #pragma acc parallel num_gangs(1)
    dst[1] = 21;

    // Source and destination are the current device, which is not host.
    // OUT-caseMemcpySuccess-HOST-NEXT: dst[1] present: 0x[[#DST1]] -> 0x[[#DST1]],     51 -> 51
    //  OUT-caseMemcpySuccess-OFF-NEXT: dst[1] present: 0x[[#DST1]] -> 0x[[#DST1_DEV]], 20 -> 51
    acc_memcpy_d2d(&dst[1], &src[1], sizeof dst[1], devNumInit, devNumInit);
    PRINT_INT(dst[1]);
    dst[1] = 20;
    #pragma acc parallel num_gangs(1)
    dst[1] = 21;

    // Source and destination are the current device, which is host.
    // OUT-caseMemcpySuccess-HOST-NEXT: dst[1] present: 0x[[#DST1]] -> 0x[[#DST1]],     51 -> 51
    //  OUT-caseMemcpySuccess-OFF-NEXT: dst[1] present: 0x[[#DST1]] -> 0x[[#DST1_DEV]], 50 -> 21
    acc_set_device_type(acc_device_host);
    acc_memcpy_d2d(&dst[1], &src[1], sizeof dst[1], 0, 0);
    acc_set_device_num(devNumInit, devTypeInit);
    PRINT_INT(dst[1]);
    dst[1] = 20;
    #pragma acc parallel num_gangs(1)
    dst[1] = 21;

    // Only destination is the current device.
    // OUT-caseMemcpySuccess-OFF-NEXT: dst[1] present: 0x[[#DST1]] -> 0x[[#DST1_DEV]], 20 -> 52
    if (devNumInit != devNumOther) {
      acc_memcpy_d2d(&dst[1], &src[1], sizeof dst[1], devNumInit, devNumOther);
      PRINT_INT(dst[1]);
      dst[1] = 20;
      #pragma acc parallel num_gangs(1)
      dst[1] = 21;
    } else if (Offloading) {
      printf("dst[1] present: %p -> %p, 20 -> 52\n", &dst[1],
             acc_deviceptr(&dst[1]));
    }

    // Only source is the current device.
    // OUT-caseMemcpySuccess-OFF-NEXT: dst[1] present: 0x[[#DST1]] -> 0x[[#DST1_DEV2]], 20 -> 51
    if (devNumInit != devNumOther) {
      acc_memcpy_d2d(&dst[1], &src[1], sizeof dst[1], devNumOther, devNumInit);
      acc_set_device_num(devNumOther, devTypeInit);
      PRINT_INT(dst[1]);
      dst[1] = 20;
      #pragma acc parallel num_gangs(1)
      dst[1] = 22;
      acc_set_device_num(devNumInit, devTypeInit);
    } else if (Offloading) {
      printf("dst[1] present: %p -> 0x0, 20 -> 51\n", &dst[1]);
    }

    // Check that we didn't disturb neighboring data or source data.
    // OUT-caseMemcpySuccess-HOST-NEXT: dst[0] present: 0x[[#DST0]] -> 0x[[#DST0]],     11 -> 11
    // OUT-caseMemcpySuccess-HOST-NEXT: dst[2] present: 0x[[#DST2]] -> 0x[[#DST2]],     31 -> 31
    // OUT-caseMemcpySuccess-HOST-NEXT: src[0] present: 0x[[#SRC0]] -> 0x[[#SRC0]],     41 -> 41
    // OUT-caseMemcpySuccess-HOST-NEXT: src[1] present: 0x[[#SRC1]] -> 0x[[#SRC1]],     51 -> 51
    // OUT-caseMemcpySuccess-HOST-NEXT: src[2] present: 0x[[#SRC2]] -> 0x[[#SRC2]],     61 -> 61
    //  OUT-caseMemcpySuccess-OFF-NEXT: dst[0] present: 0x[[#DST0]] -> 0x[[#DST0_DEV]], 10 -> 11
    //  OUT-caseMemcpySuccess-OFF-NEXT: dst[2] present: 0x[[#DST2]] -> 0x[[#DST2_DEV]], 30 -> 31
    //  OUT-caseMemcpySuccess-OFF-NEXT: src[0] present: 0x[[#SRC0]] -> 0x[[#SRC0_DEV]], 40 -> 41
    //  OUT-caseMemcpySuccess-OFF-NEXT: src[1] present: 0x[[#SRC1]] -> 0x[[#SRC1_DEV]], 50 -> 51
    //  OUT-caseMemcpySuccess-OFF-NEXT: src[2] present: 0x[[#SRC2]] -> 0x[[#SRC2_DEV]], 60 -> 61
    PRINT_INT(dst[0]);
    PRINT_INT(dst[2]);
    PRINT_INT(src[0]);
    PRINT_INT(src[1]);
    PRINT_INT(src[2]);
    // OUT-caseMemcpySuccess-OFF-NEXT: dst[0] present: 0x[[#DST0]] -> 0x[[#DST0_DEV2]], 10 -> 12
    // OUT-caseMemcpySuccess-OFF-NEXT: dst[2] present: 0x[[#DST2]] -> 0x[[#DST2_DEV2]], 30 -> 32
    // OUT-caseMemcpySuccess-OFF-NEXT: src[0] present: 0x[[#SRC0]] -> 0x[[#SRC0_DEV2]], 40 -> 42
    // OUT-caseMemcpySuccess-OFF-NEXT: src[1] present: 0x[[#SRC1]] -> 0x[[#SRC1_DEV2]], 50 -> 52
    // OUT-caseMemcpySuccess-OFF-NEXT: src[2] present: 0x[[#SRC2]] -> 0x[[#SRC2_DEV2]], 60 -> 62
    if (devNumInit != devNumOther) {
      acc_set_device_num(devNumOther, devTypeInit);
      PRINT_INT(dst[0]);
      PRINT_INT(dst[2]);
      PRINT_INT(src[0]);
      PRINT_INT(src[1]);
      PRINT_INT(src[2]);
      acc_set_device_num(devNumInit, devTypeInit);
    } else if (Offloading) {
      printf("dst[0] present: %p -> 0x0, 10 -> 12\n", &dst[0]);
      printf("dst[2] present: %p -> 0x0, 30 -> 32\n", &dst[2]);
      printf("src[0] present: %p -> 0x0, 40 -> 42\n", &src[0]);
      printf("src[1] present: %p -> 0x0, 50 -> 52\n", &src[1]);
      printf("src[2] present: %p -> 0x0, 60 -> 62\n", &src[2]);
    }

    #pragma acc exit data delete(dst, src)
    if (devNumInit != devNumOther) {
      acc_set_device_num(devNumOther, devTypeInit);
      #pragma acc exit data delete(dst, src)
      acc_set_device_num(devNumInit, devTypeInit);
    }
  }

  // Copy between addresses that are mapped to each other, are mapped to the
  // same host address, or are the same.  All cases are no-ops if shared memory.
  {
    // OUT-caseMemcpySuccess-NEXT: x[0]: 0x[[#%x,X0:]] -> 0x[[#%x,X0_DEV:]]
    // OUT-caseMemcpySuccess-NEXT: x[1]: 0x[[#%x,X1:]] -> 0x[[#%x,X1_DEV:]]
    // OUT-caseMemcpySuccess-NEXT: x[2]: 0x[[#%x,X2:]] -> 0x[[#%x,X2_DEV:]]
    int x[] = {10, 20, 30};
    #pragma acc enter data create(x)
    #pragma acc parallel num_gangs(1)
    {
      x[0] = 11;
      x[1] = 21;
      x[2] = 31;
    }
    printf("x[0]: %p -> %p\n", &x[0], acc_deviceptr(&x[0]));
    printf("x[1]: %p -> %p\n", &x[1], acc_deviceptr(&x[1]));
    printf("x[2]: %p -> %p\n", &x[2], acc_deviceptr(&x[2]));
    // OUT-caseMemcpySuccess-OFF-NEXT: x[0] on other device: 0x[[#%x,X0_DEV2:]]
    // OUT-caseMemcpySuccess-OFF-NEXT: x[1] on other device: 0x[[#%x,X1_DEV2:]]
    // OUT-caseMemcpySuccess-OFF-NEXT: x[2] on other device: 0x[[#%x,X2_DEV2:]]
    if (devNumInit != devNumOther) {
      acc_set_device_num(devNumOther, devTypeInit);
      #pragma acc enter data create(x)
      #pragma acc parallel num_gangs(1)
      {
        x[0] = 12;
        x[1] = 22;
        x[2] = 32;
      }
      printf("x[0] on other device: %p\n", acc_deviceptr(&x[0]));
      printf("x[1] on other device: %p\n", acc_deviceptr(&x[1]));
      printf("x[2] on other device: %p\n", acc_deviceptr(&x[2]));
      acc_set_device_num(devNumInit, devTypeInit);
    } else if (Offloading) {
      printf("x[0] on other device: 0x0\n");
      printf("x[1] on other device: 0x0\n");
      printf("x[2] on other device: 0x0\n");
    }

    // OUT-caseMemcpySuccess-HOST-NEXT: x[1] present: 0x[[#X1]] -> 0x[[#X1]],     21 -> 21
    //  OUT-caseMemcpySuccess-OFF-NEXT: x[1] present: 0x[[#X1]] -> 0x[[#X1_DEV]], 20 -> 20
    acc_memcpy_to_device(acc_deviceptr(&x[1]), &x[1], sizeof x[1]);
    PRINT_INT(x[1]);
    x[1] = 20;
    #pragma acc parallel num_gangs(1)
    x[1] = 21;

    // OUT-caseMemcpySuccess-HOST-NEXT: x[1] present: 0x[[#X1]] -> 0x[[#X1]],     21 -> 21
    //  OUT-caseMemcpySuccess-OFF-NEXT: x[1] present: 0x[[#X1]] -> 0x[[#X1_DEV]], 21 -> 21
    acc_memcpy_from_device(&x[1], acc_deviceptr(&x[1]), sizeof x[1]);
    PRINT_INT(x[1]);
    x[1] = 20;
    #pragma acc parallel num_gangs(1)
    x[1] = 21;

    // Always no-op.
    // OUT-caseMemcpySuccess-HOST-NEXT: x[1] present: 0x[[#X1]] -> 0x[[#X1]],     21 -> 21
    //  OUT-caseMemcpySuccess-OFF-NEXT: x[1] present: 0x[[#X1]] -> 0x[[#X1_DEV]], 20 -> 21
    acc_memcpy_device(acc_deviceptr(&x[1]), acc_deviceptr(&x[1]), sizeof x[1]);
    PRINT_INT(x[1]);

    // Source and destination are the current device, which is not host.
    // Always no-op.
    // OUT-caseMemcpySuccess-HOST-NEXT: x[1] present: 0x[[#X1]] -> 0x[[#X1]],     21 -> 21
    //  OUT-caseMemcpySuccess-OFF-NEXT: x[1] present: 0x[[#X1]] -> 0x[[#X1_DEV]], 20 -> 21
    acc_memcpy_d2d(&x[1], &x[1], sizeof x[1], devNumInit, devNumInit);
    PRINT_INT(x[1]);

    // Source and destination are the current device, which is host.
    // Always no-op.
    // OUT-caseMemcpySuccess-HOST-NEXT: x[1] present: 0x[[#X1]] -> 0x[[#X1]],     21 -> 21
    //  OUT-caseMemcpySuccess-OFF-NEXT: x[1] present: 0x[[#X1]] -> 0x[[#X1_DEV]], 20 -> 21
    acc_set_device_type(acc_device_host);
    acc_memcpy_d2d(&x[1], &x[1], sizeof x[1], 0, 0);
    acc_set_device_num(devNumInit, devTypeInit);
    PRINT_INT(x[1]);

    // Only destination is the current device.
    // OUT-caseMemcpySuccess-OFF-NEXT: x[1] present: 0x[[#X1]] -> 0x[[#X1_DEV]], 20 -> 22
    if (devNumInit != devNumOther) {
      acc_memcpy_d2d(&x[1], &x[1], sizeof x[1], devNumInit, devNumOther);
      PRINT_INT(x[1]);
      x[1] = 20;
      #pragma acc parallel num_gangs(1)
      x[1] = 21;
    } else if (Offloading) {
      printf("x[1] present: %p -> %p, 20 -> 22\n", &x[1], acc_deviceptr(&x[1]));
    }

    // Only source is the current device.
    // OUT-caseMemcpySuccess-OFF-NEXT: x[1] present: 0x[[#X1]] -> 0x[[#X1_DEV2]], 20 -> 21
    if (devNumInit != devNumOther) {
      acc_memcpy_d2d(&x[1], &x[1], sizeof x[1], devNumOther, devNumInit);
      acc_set_device_num(devNumOther, devTypeInit);
      PRINT_INT(x[1]);
      x[1] = 20;
      #pragma acc parallel num_gangs(1)
      x[1] = 22;
      acc_set_device_num(devNumInit, devTypeInit);
    } else if (Offloading) {
      printf("x[1] present: %p -> 0x0, 20 -> 21\n", &x[1]);
    }

    // Check that we didn't disturb neighboring data.
    // OUT-caseMemcpySuccess-HOST-NEXT: x[0] present: 0x[[#X0]] -> 0x[[#X0]],     11 -> 11
    // OUT-caseMemcpySuccess-HOST-NEXT: x[2] present: 0x[[#X2]] -> 0x[[#X2]],     31 -> 31
    //  OUT-caseMemcpySuccess-OFF-NEXT: x[0] present: 0x[[#X0]] -> 0x[[#X0_DEV]], 10 -> 11
    //  OUT-caseMemcpySuccess-OFF-NEXT: x[2] present: 0x[[#X2]] -> 0x[[#X2_DEV]], 30 -> 31
    PRINT_INT(x[0]);
    PRINT_INT(x[2]);
    // OUT-caseMemcpySuccess-OFF-NEXT: x[0] present: 0x[[#X0]] -> 0x[[#X0_DEV2]], 10 -> 12
    // OUT-caseMemcpySuccess-OFF-NEXT: x[2] present: 0x[[#X2]] -> 0x[[#X2_DEV2]], 30 -> 32
    if (devNumInit != devNumOther) {
      acc_set_device_num(devNumOther, devTypeInit);
      PRINT_INT(x[0]);
      PRINT_INT(x[2]);
      acc_set_device_num(devNumInit, devTypeInit);
    } else if (Offloading) {
      printf("x[0] present: %p -> 0x0, 10 -> 12\n", &x[0]);
      printf("x[2] present: %p -> 0x0, 30 -> 32\n", &x[2]);
    }

    #pragma acc exit data delete(x)
    if (devNumInit != devNumOther) {
      acc_set_device_num(devNumOther, devTypeInit);
      #pragma acc exit data delete(x)
      acc_set_device_num(devNumInit, devTypeInit);
    }

    // Always no-op even though data is inaccessible on the device.
    // OUT-caseMemcpySuccess-HOST-NEXT: x[1] present: 0x[[#X1]] -> 0x[[#X1]], 21 -> 21
    //  OUT-caseMemcpySuccess-OFF-NEXT: x[1]  absent: 0x[[#X1]] -> (nil),     20
    acc_memcpy_d2d(&x[1], &x[1], sizeof x[1], devNumInit, devNumInit);
    PRINT_INT(x[1]);
  }

  // Check that we have a no-op when bytes=0 and addresses are not mapped to
  // anything (unless shared memory).
  {
    // OUT-caseMemcpySuccess-NEXT: host[0]: 0x[[#%x,HOST0:]]
    // OUT-caseMemcpySuccess-NEXT: host[1]: 0x[[#%x,HOST1:]]
    // OUT-caseMemcpySuccess-NEXT:  dev[0]: 0x[[#%x,DEV0:]]
    // OUT-caseMemcpySuccess-NEXT:  dev[1]: 0x[[#%x,DEV1:]]
    int host[] = {10, 20};
    int valsForDev[] = {31, 41};
    int *dev = acc_malloc(sizeof valsForDev);
    if (Offloading) {
      acc_map_data(valsForDev, dev, sizeof valsForDev);
      #pragma acc update device(valsForDev)
      acc_unmap_data(valsForDev);
    } else {
      memcpy(dev, valsForDev, sizeof valsForDev);
    }
    printf("host[0]: %p\n", &host[0]);
    printf("host[1]: %p\n", &host[1]);
    printf("dev[0]: %p\n", &dev[0]);
    printf("dev[1]: %p\n", &dev[1]);
    // OUT-caseMemcpySuccess-OFF-NEXT: dev2[0]: 0x[[#%x,DEV20:]]
    // OUT-caseMemcpySuccess-OFF-NEXT: dev2[1]: 0x[[#%x,DEV21:]]
    int *dev2 = NULL;
    if (devNumInit != devNumOther) {
      int valsForDev2[] = {32, 42};
      acc_set_device_num(devNumOther, devTypeInit);
      dev2 = acc_malloc(sizeof valsForDev2);
      acc_map_data(valsForDev2, dev2, sizeof valsForDev2);
      #pragma acc update device(valsForDev2)
      acc_unmap_data(valsForDev2);
      acc_set_device_num(devNumInit, devTypeInit);
      printf("dev2[0]: %p\n", &dev2[0]);
      printf("dev2[1]: %p\n", &dev2[1]);
    } else if (Offloading) {
      printf("dev2[0]: 0x0\n");
      printf("dev2[1]: 0x0\n");
    }

    acc_memcpy_to_device(&dev[1], &host[0], 0);
    acc_memcpy_from_device(&host[1], &dev[0], 0);
    acc_memcpy_device(&dev[1], &dev[0], 0);
    acc_memcpy_device(&dev[1], &dev[1], 0);
    acc_set_device_type(acc_device_host);
    acc_memcpy_d2d(&host[1], &host[0], 0, 0, 0);
    acc_memcpy_d2d(&host[1], &host[1], 0, 0, 0);
    acc_set_device_num(devNumInit, devTypeInit);

    // OUT-caseMemcpySuccess-HOST-NEXT: host[0]  present: 0x[[#HOST0]] -> 0x[[#HOST0]], 10 -> 10
    // OUT-caseMemcpySuccess-HOST-NEXT: host[1]  present: 0x[[#HOST1]] -> 0x[[#HOST1]], 20 -> 20
    // OUT-caseMemcpySuccess-HOST-NEXT: dev[0]    mapped: 0x[[#DEV0]]  <- 0x[[#DEV0]],  31 <- 31
    // OUT-caseMemcpySuccess-HOST-NEXT: dev[1]    mapped: 0x[[#DEV1]]  <- 0x[[#DEV1]],  41 <- 41
    //  OUT-caseMemcpySuccess-OFF-NEXT: host[0]   absent: 0x[[#HOST0]] -> (nil),        10
    //  OUT-caseMemcpySuccess-OFF-NEXT: host[1]   absent: 0x[[#HOST1]] -> (nil),        20
    //  OUT-caseMemcpySuccess-OFF-NEXT: dev[0]  unmapped: 0x[[#DEV0]]  <- (nil),        31
    //  OUT-caseMemcpySuccess-OFF-NEXT: dev[1]  unmapped: 0x[[#DEV1]]  <- (nil),        41
    PRINT_INT(host[0]);
    PRINT_INT(host[1]);
    PRINT_DEV_INT(dev[0]);
    PRINT_DEV_INT(dev[1]);
    // OUT-caseMemcpySuccess-OFF-NEXT: dev2[0] unmapped: 0x[[#DEV20]] <- (nil), 32
    // OUT-caseMemcpySuccess-OFF-NEXT: dev2[1] unmapped: 0x[[#DEV21]] <- (nil), 42
    if (devNumInit != devNumOther) {
      acc_set_device_num(devNumOther, devTypeInit);
      PRINT_DEV_INT(dev2[0]);
      PRINT_DEV_INT(dev2[1]);
      acc_set_device_num(devNumInit, devTypeInit);
    } else if (Offloading) {
      printf("dev2[0] unmapped: 0x0 <- (nil), 32\n");
      printf("dev2[1] unmapped: 0x0 <- (nil), 42\n");
    }

    acc_free(dev);
    acc_set_device_num(devNumOther, devTypeInit);
    acc_free(dev2);
    acc_set_device_num(devNumInit, devTypeInit);
  }

  // Check that we have a no-op when bytes=0 and addresses are mapped, possibly
  // to each other or the same host address, or are the same.
  {
    // OUT-caseMemcpySuccess-NEXT: dst[0]: 0x[[#%x,DST0:]] -> 0x[[#%x,DST0_DEV:]]
    // OUT-caseMemcpySuccess-NEXT: dst[1]: 0x[[#%x,DST1:]] -> 0x[[#%x,DST1_DEV:]]
    // OUT-caseMemcpySuccess-NEXT: src[0]: 0x[[#%x,SRC0:]] -> 0x[[#%x,SRC0_DEV:]]
    // OUT-caseMemcpySuccess-NEXT: src[1]: 0x[[#%x,SRC1:]] -> 0x[[#%x,SRC1_DEV:]]
    int dst[] = {10, 20};
    int src[] = {30, 40};
    #pragma acc enter data create(dst, src)
    #pragma acc parallel num_gangs(1)
    {
      dst[0] = 11;
      dst[1] = 21;
      src[0] = 31;
      src[1] = 41;
    }
    printf("dst[0]: %p -> %p\n", &dst[0], acc_deviceptr(&dst[0]));
    printf("dst[1]: %p -> %p\n", &dst[1], acc_deviceptr(&dst[1]));
    printf("src[0]: %p -> %p\n", &src[0], acc_deviceptr(&src[0]));
    printf("src[1]: %p -> %p\n", &src[1], acc_deviceptr(&src[1]));
    // OUT-caseMemcpySuccess-OFF-NEXT: dst[0] on other device: 0x[[#%x,DST0_DEV2:]]
    // OUT-caseMemcpySuccess-OFF-NEXT: dst[1] on other device: 0x[[#%x,DST1_DEV2:]]
    // OUT-caseMemcpySuccess-OFF-NEXT: src[0] on other device: 0x[[#%x,SRC0_DEV2:]]
    // OUT-caseMemcpySuccess-OFF-NEXT: src[1] on other device: 0x[[#%x,SRC1_DEV2:]]
    if (devNumInit != devNumOther) {
      acc_set_device_num(devNumOther, devTypeInit);
      #pragma acc enter data create(dst, src)
      #pragma acc parallel num_gangs(1)
      {
        dst[0] = 12;
        dst[1] = 22;
        src[0] = 32;
        src[1] = 42;
      }
      printf("dst[0] on other device: %p\n", acc_deviceptr(&dst[0]));
      printf("dst[1] on other device: %p\n", acc_deviceptr(&dst[1]));
      printf("src[0] on other device: %p\n", acc_deviceptr(&src[0]));
      printf("src[1] on other device: %p\n", acc_deviceptr(&src[1]));
      acc_set_device_num(devNumInit, devTypeInit);
    } else if (Offloading) {
      printf("dst[0] on other device: 0x0\n");
      printf("dst[1] on other device: 0x0\n");
      printf("src[0] on other device: 0x0\n");
      printf("src[1] on other device: 0x0\n");
    }

    // Mapped but not to each other or the same host address.
    acc_memcpy_to_device(acc_deviceptr(&dst[1]), &src[1], 0);
    acc_memcpy_from_device(&dst[1], acc_deviceptr(&src[1]), 0);
    acc_memcpy_device(acc_deviceptr(&dst[1]), acc_deviceptr(&src[1]), 0);
    acc_memcpy_d2d(&dst[1], &src[1], 0, devNumInit, devNumInit);
    acc_set_device_type(acc_device_host);
    acc_memcpy_d2d(&dst[1], &src[1], 0, 0, 0);
    acc_set_device_num(devNumInit, devTypeInit);
    acc_memcpy_d2d(&dst[1], &src[1], 0, devNumInit, devNumOther);
    acc_memcpy_d2d(&dst[1], &src[1], 0, devNumOther, devNumInit);

    // Mapped to each other or the same host address.
    acc_memcpy_to_device(acc_deviceptr(&dst[1]), &dst[1], 0);
    acc_memcpy_from_device(&dst[1], acc_deviceptr(&dst[1]), 0);
    acc_memcpy_d2d(&dst[1], &dst[1], 0, devNumInit, devNumOther);
    acc_memcpy_d2d(&dst[1], &dst[1], 0, devNumOther, devNumInit);

    // The same.
    acc_memcpy_device(acc_deviceptr(&dst[1]), acc_deviceptr(&dst[1]), 0);
    acc_memcpy_d2d(&dst[1], &dst[1], 0, devNumInit, devNumInit);
    acc_set_device_type(acc_device_host);
    acc_memcpy_d2d(&dst[1], &dst[1], 0, 0, 0);
    acc_set_device_num(devNumInit, devTypeInit);

    // OUT-caseMemcpySuccess-HOST-NEXT: dst[0] present: 0x[[#DST0]] -> 0x[[#DST0]],     11 -> 11
    // OUT-caseMemcpySuccess-HOST-NEXT: dst[1] present: 0x[[#DST1]] -> 0x[[#DST1]],     21 -> 21
    // OUT-caseMemcpySuccess-HOST-NEXT: src[0] present: 0x[[#SRC0]] -> 0x[[#SRC0]],     31 -> 31
    // OUT-caseMemcpySuccess-HOST-NEXT: src[1] present: 0x[[#SRC1]] -> 0x[[#SRC1]],     41 -> 41
    //  OUT-caseMemcpySuccess-OFF-NEXT: dst[0] present: 0x[[#DST0]] -> 0x[[#DST0_DEV]], 10 -> 11
    //  OUT-caseMemcpySuccess-OFF-NEXT: dst[1] present: 0x[[#DST1]] -> 0x[[#DST1_DEV]], 20 -> 21
    //  OUT-caseMemcpySuccess-OFF-NEXT: src[0] present: 0x[[#SRC0]] -> 0x[[#SRC0_DEV]], 30 -> 31
    //  OUT-caseMemcpySuccess-OFF-NEXT: src[1] present: 0x[[#SRC1]] -> 0x[[#SRC1_DEV]], 40 -> 41
    PRINT_INT(dst[0]);
    PRINT_INT(dst[1]);
    PRINT_INT(src[0]);
    PRINT_INT(src[1]);
    // OUT-caseMemcpySuccess-OFF-NEXT: dst[0] present: 0x[[#DST0]] -> 0x[[#DST0_DEV2]], 10 -> 12
    // OUT-caseMemcpySuccess-OFF-NEXT: dst[1] present: 0x[[#DST1]] -> 0x[[#DST1_DEV2]], 20 -> 22
    // OUT-caseMemcpySuccess-OFF-NEXT: src[0] present: 0x[[#SRC0]] -> 0x[[#SRC0_DEV2]], 30 -> 32
    // OUT-caseMemcpySuccess-OFF-NEXT: src[1] present: 0x[[#SRC1]] -> 0x[[#SRC1_DEV2]], 40 -> 42
    if (devNumInit != devNumOther) {
      acc_set_device_num(devNumOther, devTypeInit);
      PRINT_INT(dst[0]);
      PRINT_INT(dst[1]);
      PRINT_INT(src[0]);
      PRINT_INT(src[1]);
      acc_set_device_num(devNumInit, devTypeInit);
    } else if (Offloading) {
      printf("dst[0] present: %p -> 0x0, 10 -> 12\n", &dst[0]);
      printf("dst[1] present: %p -> 0x0, 20 -> 22\n", &dst[1]);
      printf("src[0] present: %p -> 0x0, 30 -> 32\n", &src[0]);
      printf("src[1] present: %p -> 0x0, 40 -> 42\n", &src[1]);
    }

    #pragma acc exit data delete(dst, src)
    if (devNumInit != devNumOther) {
      acc_set_device_num(devNumOther, devTypeInit);
      #pragma acc exit data delete(dst, src)
      acc_set_device_num(devNumInit, devTypeInit);
    }
  }

  // Check that we have a no-op when bytes=0 and either addresses are null,
  // addresses are inaccessible, device numbers are invalid, or some combination
  // of those.
  acc_memcpy_to_device(NULL, NULL, 0);
  acc_memcpy_from_device(NULL, NULL, 0);
  acc_memcpy_device(NULL, NULL, 0);
  acc_memcpy_d2d(NULL, NULL, 0, devNumInit, devNumInit);
  acc_memcpy_d2d(NULL, NULL, 0, devNumInit, devNumOther);
  acc_memcpy_d2d(NULL, NULL, 0, devNumInvalid, devNumInvalid);
  {
    int x, y; // inaccessible
    acc_memcpy_d2d(&x, &y, 0, devNumInit, devNumInit);
    acc_memcpy_d2d(&x, &y, 0, devNumInit, devNumOther);
    acc_memcpy_d2d(&x, &y, 0, devNumInvalid, devNumInvalid);
    #pragma acc data create(x, y) // now accessible
    acc_memcpy_d2d(&x, &y, 0, devNumInvalid, devNumInvalid);
  }

  return 0;
}

CASE(caseMemcpyToDeviceDestNull) {
  int host;
  // ERR-caseMemcpyToDeviceDestNull-NEXT: OMP: Error #[[#]]: acc_memcpy_to_device called with null destination pointer
  acc_memcpy_to_device(NULL, &host, sizeof host);
  return 0;
}
CASE(caseMemcpyToDeviceSrcNull) {
  int *dev = acc_malloc(sizeof *dev);
  if (!dev) {
    fprintf(stderr, "acc_malloc failed\n");
    return 1;
  }
  // ERR-caseMemcpyToDeviceSrcNull-NEXT: OMP: Error #[[#]]: acc_memcpy_to_device called with null source pointer
  acc_memcpy_to_device(dev, NULL, sizeof *dev);
  return 0;
}
CASE(caseMemcpyToDeviceBothNull) {
  // ERR-caseMemcpyToDeviceBothNull-NEXT: OMP: Error #[[#]]: acc_memcpy_to_device called with null destination pointer
  acc_memcpy_to_device(NULL, NULL, 1);
  return 0;
}

CASE(caseMemcpyFromDeviceDestNull) {
  int *dev = acc_malloc(sizeof *dev);
  if (!dev) {
    fprintf(stderr, "acc_malloc failed\n");
    return 1;
  }
  acc_memcpy_from_device(NULL, dev, sizeof *dev);
  // ERR-caseMemcpyFromDeviceDestNull-NEXT: OMP: Error #[[#]]: acc_memcpy_from_device called with null destination pointer
  return 0;
}
CASE(caseMemcpyFromDeviceSrcNull) {
  int host;
  // ERR-caseMemcpyFromDeviceSrcNull-NEXT: OMP: Error #[[#]]: acc_memcpy_from_device called with null source pointer
  acc_memcpy_from_device(&host, NULL, sizeof host);
  return 0;
}
CASE(caseMemcpyFromDeviceBothNull) {
  // ERR-caseMemcpyFromDeviceBothNull-NEXT: OMP: Error #[[#]]: acc_memcpy_from_device called with null destination pointer
  acc_memcpy_from_device(NULL, NULL, 1);
  return 0;
}

CASE(caseMemcpyDeviceDestNull) {
  int *dev = acc_malloc(sizeof *dev);
  if (!dev) {
    fprintf(stderr, "acc_malloc failed\n");
    return 1;
  }
  acc_memcpy_device(NULL, dev, sizeof *dev);
  // ERR-caseMemcpyDeviceDestNull-NEXT: OMP: Error #[[#]]: acc_memcpy_device called with null destination pointer
  return 0;
}
CASE(caseMemcpyDeviceSrcNull) {
  int *dev = acc_malloc(sizeof *dev);
  if (!dev) {
    fprintf(stderr, "acc_malloc failed\n");
    return 1;
  }
  // ERR-caseMemcpyDeviceSrcNull-NEXT: OMP: Error #[[#]]: acc_memcpy_device called with null source pointer
  acc_memcpy_device(dev, NULL, sizeof *dev);
  return 0;
}
CASE(caseMemcpyDeviceBothNull) {
  // ERR-caseMemcpyDeviceBothNull-NEXT: OMP: Error #[[#]]: acc_memcpy_device called with null destination pointer
  acc_memcpy_device(NULL, NULL, 1);
  return 0;
}

CASE(caseMemcpyD2dDestNull) {
  int devNum = getDevNum();
  int host;
  #pragma acc data create(host)
  // ERR-caseMemcpyD2dDestNull-NEXT: OMP: Error #[[#]]: acc_memcpy_d2d called with null destination pointer
  acc_memcpy_d2d(NULL, &host, sizeof host, devNum, devNum);
  return 0;
}
CASE(caseMemcpyD2dSrcNull) {
  int devNum = getDevNum();
  int host;
  #pragma acc data create(host)
  // ERR-caseMemcpyD2dSrcNull-NEXT: OMP: Error #[[#]]: acc_memcpy_d2d called with null source pointer
  acc_memcpy_d2d(&host, NULL, sizeof host, devNum, devNum);
  return 0;
}
CASE(caseMemcpyD2dBothNull) {
  int devNum = getDevNum();
  // ERR-caseMemcpyD2dBothNull-NEXT: OMP: Error #[[#]]: acc_memcpy_d2d called with null destination pointer
  acc_memcpy_d2d(NULL, NULL, 1, devNum, devNum);
  return 0;
}

CASE(caseMemcpyD2dDestDevInvalid) {
  int x, y;
  // ERR-caseMemcpyD2dDestDevInvalid-NEXT: [[TYPE:[a-z0-9_]+]], [[#%d,INV:]]
  fprintf(stderr, "%s, %d\n", getDevTypeStr(), getDevNumInvalid());
  #pragma acc data create(x, y)
  // ERR-caseMemcpyD2dDestDevInvalid-NEXT: OMP: Error #[[#]]: acc_memcpy_d2d called with invalid destination device number [[#INV]] for the current device type [[TYPE]]
  acc_memcpy_d2d(&x, &y, sizeof x, getDevNumInvalid(), getDevNum());
  return 0;
}
CASE(caseMemcpyD2dDestDevInvalidNeg) {
  int x, y;
  // ERR-caseMemcpyD2dDestDevInvalidNeg-NEXT: [[TYPE:[a-z0-9_]+]], [[#%d,INV:]]
  fprintf(stderr, "%s, %d\n", getDevTypeStr(), getDevNumInvalidNeg());
  #pragma acc data create(x, y)
  // ERR-caseMemcpyD2dDestDevInvalidNeg-NEXT: OMP: Error #[[#]]: acc_memcpy_d2d called with invalid destination device number [[#INV]] for the current device type [[TYPE]]
  acc_memcpy_d2d(&x, &y, sizeof x, getDevNumInvalidNeg(), getDevNum());
  return 0;
}
CASE(caseMemcpyD2dSrcDevInvalid) {
  int x, y;
  // ERR-caseMemcpyD2dSrcDevInvalid-NEXT: [[TYPE:[a-z0-9_]+]], [[#%d,INV:]]
  fprintf(stderr, "%s, %d\n", getDevTypeStr(), getDevNumInvalid());
  #pragma acc data create(x, y)
  // ERR-caseMemcpyD2dSrcDevInvalid-NEXT: OMP: Error #[[#]]: acc_memcpy_d2d called with invalid source device number [[#INV]] for the current device type [[TYPE]]
  acc_memcpy_d2d(&x, &y, sizeof x, getDevNum(), getDevNumInvalid());
  return 0;
}
CASE(caseMemcpyD2dSrcDevInvalidNeg) {
  int x, y;
  // ERR-caseMemcpyD2dSrcDevInvalidNeg-NEXT: [[TYPE:[a-z0-9_]+]], [[#%d,INV:]]
  fprintf(stderr, "%s, %d\n", getDevTypeStr(), getDevNumInvalidNeg());
  #pragma acc data create(x, y)
  // ERR-caseMemcpyD2dSrcDevInvalidNeg-NEXT: OMP: Error #[[#]]: acc_memcpy_d2d called with invalid source device number [[#INV]] for the current device type [[TYPE]]
  acc_memcpy_d2d(&x, &y, sizeof x, getDevNum(), getDevNumInvalidNeg());
  return 0;
}
CASE(caseMemcpyD2dBothDevInvalid) {
  int x, y;
  // ERR-caseMemcpyD2dBothDevInvalid-NEXT: [[TYPE:[a-z0-9_]+]], [[#%d,INV:]]
  fprintf(stderr, "%s, %d\n", getDevTypeStr(), getDevNumInvalid());
  #pragma acc data create(x, y)
  // ERR-caseMemcpyD2dBothDevInvalid-NEXT: OMP: Error #[[#]]: acc_memcpy_d2d called with invalid destination device number [[#INV]] for the current device type [[TYPE]]
  acc_memcpy_d2d(&x, &y, sizeof x, getDevNumInvalid(), getDevNumInvalid());
  return 0;
}
CASE(caseMemcpyD2dSameDevInvalid) {
  int x;
  // ERR-caseMemcpyD2dSameDevInvalid-NEXT: [[TYPE:[a-z0-9_]+]], [[#%d,INV:]]
  fprintf(stderr, "%s, %d\n", getDevTypeStr(), getDevNumInvalid());
  #pragma acc data create(x)
  // ERR-caseMemcpyD2dSameDevInvalid-NEXT: OMP: Error #[[#]]: acc_memcpy_d2d called with invalid destination device number [[#INV]] for the current device type [[TYPE]]
  acc_memcpy_d2d(&x, &x, sizeof x, getDevNumInvalid(), getDevNumInvalid());
  return 0;
}

CASE(caseMemcpyD2dDestAbsent) {
  // ERR-caseMemcpyD2dDestAbsent-HOST-NEXT: dst present: 0x[[#%x,]] -> 0x[[#%x,]], 20 -> 20
  //  ERR-caseMemcpyD2dDestAbsent-OFF-NEXT: OMP: Error #[[#]]: acc_memcpy_d2d called with inaccessible destination data
  int dst = 10, src = 20;
  #pragma acc data create(src)
  acc_memcpy_d2d(&dst, &src, sizeof dst, getDevNum(), getDevNum());
  PRINT_INT_STDERR(dst);
  return 0;
}
CASE(caseMemcpyD2dSrcAbsent) {
  // ERR-caseMemcpyD2dSrcAbsent-HOST-NEXT: dst present: 0x[[#%x,]] -> 0x[[#%x,]], 20 -> 20
  //  ERR-caseMemcpyD2dSrcAbsent-OFF-NEXT: OMP: Error #[[#]]: acc_memcpy_d2d called with inaccessible source data
  int dst = 10, src = 20;
  #pragma acc data create(dst)
  acc_memcpy_d2d(&dst, &src, sizeof dst, getDevNum(), getDevNum());
  PRINT_INT_STDERR(dst);
  return 0;
}
CASE(caseMemcpyD2dDestExtendsAfter) {
  // ERR-caseMemcpyD2dDestExtendsAfter-HOST-NEXT: dst[0] present: 0x[[#%x,]] -> 0x[[#%x,]], 30 -> 30
  // ERR-caseMemcpyD2dDestExtendsAfter-HOST-NEXT: dst[1] present: 0x[[#%x,]] -> 0x[[#%x,]], 40 -> 40
  //  ERR-caseMemcpyD2dDestExtendsAfter-OFF-NEXT: OMP: Error #[[#]]: acc_memcpy_d2d called with inaccessible destination data
  int dst[] = {10, 20};
  int src[] = {30, 40};
  #pragma acc data create(dst[0:1], src)
  acc_memcpy_d2d(dst, src, sizeof dst, getDevNum(), getDevNum());
  PRINT_INT_STDERR(dst[0]);
  PRINT_INT_STDERR(dst[1]);
  return 0;
}
CASE(caseMemcpyD2dSrcExtendsAfter) {
  // ERR-caseMemcpyD2dSrcExtendsAfter-HOST-NEXT: dst[0] present: 0x[[#%x,]] -> 0x[[#%x,]], 30 -> 30
  // ERR-caseMemcpyD2dSrcExtendsAfter-HOST-NEXT: dst[1] present: 0x[[#%x,]] -> 0x[[#%x,]], 40 -> 40
  //  ERR-caseMemcpyD2dSrcExtendsAfter-OFF-NEXT: OMP: Error #[[#]]: acc_memcpy_d2d called with inaccessible source data
  int dst[] = {10, 20};
  int src[] = {30, 40};
  #pragma acc data create(dst, src[0:1])
  acc_memcpy_d2d(dst, src, sizeof dst, getDevNum(), getDevNum());
  PRINT_INT_STDERR(dst[0]);
  PRINT_INT_STDERR(dst[1]);
  return 0;
}
CASE(caseMemcpyD2dDestExtendsBefore) {
  // ERR-caseMemcpyD2dDestExtendsBefore-HOST-NEXT: dst[0] present: 0x[[#%x,]] -> 0x[[#%x,]], 30 -> 30
  // ERR-caseMemcpyD2dDestExtendsBefore-HOST-NEXT: dst[1] present: 0x[[#%x,]] -> 0x[[#%x,]], 40 -> 40
  //  ERR-caseMemcpyD2dDestExtendsBefore-OFF-NEXT: OMP: Error #[[#]]: acc_memcpy_d2d called with inaccessible destination data
  int dst[] = {10, 20};
  int src[] = {30, 40};
  #pragma acc data create(dst[1:1], src)
  acc_memcpy_d2d(dst, src, sizeof dst, getDevNum(), getDevNum());
  PRINT_INT_STDERR(dst[0]);
  PRINT_INT_STDERR(dst[1]);
  return 0;
}
CASE(caseMemcpyD2dSrcExtendsBefore) {
  // ERR-caseMemcpyD2dSrcExtendsBefore-HOST-NEXT: dst[0] present: 0x[[#%x,]] -> 0x[[#%x,]], 30 -> 30
  // ERR-caseMemcpyD2dSrcExtendsBefore-HOST-NEXT: dst[1] present: 0x[[#%x,]] -> 0x[[#%x,]], 40 -> 40
  //  ERR-caseMemcpyD2dSrcExtendsBefore-OFF-NEXT: OMP: Error #[[#]]: acc_memcpy_d2d called with inaccessible source data
  int dst[] = {10, 20};
  int src[] = {30, 40};
  #pragma acc data create(dst, src[1:1])
  acc_memcpy_d2d(dst, src, sizeof dst, getDevNum(), getDevNum());
  PRINT_INT_STDERR(dst[0]);
  PRINT_INT_STDERR(dst[1]);
  return 0;
}
CASE(caseMemcpyD2dDestSubsumes) {
  // ERR-caseMemcpyD2dDestSubsumes-HOST-NEXT: dst[0] present: 0x[[#%x,]] -> 0x[[#%x,]], 40 -> 40
  // ERR-caseMemcpyD2dDestSubsumes-HOST-NEXT: dst[1] present: 0x[[#%x,]] -> 0x[[#%x,]], 50 -> 50
  // ERR-caseMemcpyD2dDestSubsumes-HOST-NEXT: dst[2] present: 0x[[#%x,]] -> 0x[[#%x,]], 60 -> 60
  //  ERR-caseMemcpyD2dDestSubsumes-OFF-NEXT: OMP: Error #[[#]]: acc_memcpy_d2d called with inaccessible destination data
  int dst[] = {10, 20, 30};
  int src[] = {40, 50, 60};
  #pragma acc data create(dst[1:1], src)
  acc_memcpy_d2d(dst, src, sizeof dst, getDevNum(), getDevNum());
  PRINT_INT_STDERR(dst[0]);
  PRINT_INT_STDERR(dst[1]);
  PRINT_INT_STDERR(dst[2]);
  return 0;
}
CASE(caseMemcpyD2dSrcSubsumes) {
  // ERR-caseMemcpyD2dSrcSubsumes-HOST-NEXT: dst[0] present: 0x[[#%x,]] -> 0x[[#%x,]], 40 -> 40
  // ERR-caseMemcpyD2dSrcSubsumes-HOST-NEXT: dst[1] present: 0x[[#%x,]] -> 0x[[#%x,]], 50 -> 50
  // ERR-caseMemcpyD2dSrcSubsumes-HOST-NEXT: dst[2] present: 0x[[#%x,]] -> 0x[[#%x,]], 60 -> 60
  //  ERR-caseMemcpyD2dSrcSubsumes-OFF-NEXT: OMP: Error #[[#]]: acc_memcpy_d2d called with inaccessible source data
  int dst[] = {10, 20, 30};
  int src[] = {40, 50, 60};
  #pragma acc data create(dst, src[1:1])
  acc_memcpy_d2d(dst, src, sizeof dst, getDevNum(), getDevNum());
  PRINT_INT_STDERR(dst[0]);
  PRINT_INT_STDERR(dst[1]);
  PRINT_INT_STDERR(dst[2]);
  return 0;
}
CASE(caseMemcpyD2dDestConcat2) {
  // ERR-caseMemcpyD2dDestConcat2-HOST-NEXT: dst[0] present: 0x[[#%x,]] -> 0x[[#%x,]], 30 -> 30
  // ERR-caseMemcpyD2dDestConcat2-HOST-NEXT: dst[1] present: 0x[[#%x,]] -> 0x[[#%x,]], 40 -> 40
  //  ERR-caseMemcpyD2dDestConcat2-OFF-NEXT: OMP: Error #[[#]]: acc_memcpy_d2d called with inaccessible destination data
  int dst[] = {10, 20};
  int src[] = {30, 40};
  #pragma acc data create(dst[0:1], src)
  #pragma acc data create(dst[1:1], src)
  acc_memcpy_d2d(dst, src, sizeof dst, getDevNum(), getDevNum());
  PRINT_INT_STDERR(dst[0]);
  PRINT_INT_STDERR(dst[1]);
  return 0;
}
CASE(caseMemcpyD2dSrcConcat2) {
  // ERR-caseMemcpyD2dSrcConcat2-HOST-NEXT: dst[0] present: 0x[[#%x,]] -> 0x[[#%x,]], 30 -> 30
  // ERR-caseMemcpyD2dSrcConcat2-HOST-NEXT: dst[1] present: 0x[[#%x,]] -> 0x[[#%x,]], 40 -> 40
  //  ERR-caseMemcpyD2dSrcConcat2-OFF-NEXT: OMP: Error #[[#]]: acc_memcpy_d2d called with inaccessible source data
  int dst[] = {10, 20};
  int src[] = {30, 40};
  #pragma acc data create(dst, src[0:1])
  #pragma acc data create(dst, src[1:1])
  acc_memcpy_d2d(dst, src, sizeof dst, getDevNum(), getDevNum());
  PRINT_INT_STDERR(dst[0]);
  PRINT_INT_STDERR(dst[1]);
  return 0;
}

// OUT-NOT: {{.}}
// An abort messages usually follows any error.
// ERR-NOT: {{(OMP:|Libomptarget)}}
