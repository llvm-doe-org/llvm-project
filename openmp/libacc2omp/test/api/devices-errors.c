// Check device management routines with runtime errors.

// RUN: %data tgts {
// RUN:   (run-if=                cflags='                                     -Xclang -verify'         tgt-host-or-off=HOST tgt-not-if-host='%not --crash')
// RUN:   (run-if=%run-if-x86_64  cflags='-fopenmp-targets=%run-x86_64-triple  -Xclang -verify'         tgt-host-or-off=OFF  tgt-not-if-host=              )
// RUN:   (run-if=%run-if-ppc64le cflags='-fopenmp-targets=%run-ppc64le-triple -Xclang -verify'         tgt-host-or-off=OFF  tgt-not-if-host=              )
// RUN:   (run-if=%run-if-nvptx64 cflags='-fopenmp-targets=%run-nvptx64-triple -Xclang -verify=nvptx64' tgt-host-or-off=OFF  tgt-not-if-host=              )
// RUN:   (run-if=%run-if-amdgcn  cflags='-fopenmp-targets=%run-amdgcn-triple  -Xclang -verify'         tgt-host-or-off=OFF  tgt-not-if-host=              )
// RUN: }
// RUN: %data run-envs {
// RUN:   (run-env=                                  host-or-off=%[tgt-host-or-off] not-if-host=%[tgt-not-if-host])
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled' host-or-off=HOST               not-if-host='%not --crash'    )
// RUN: }
// RUN: %data cases {
// RUN:   (case=caseGetNumDevicesInvalidTypePos not-if-fail='%not --crash')
// RUN:   (case=caseGetNumDevicesInvalidTypeNeg not-if-fail='%not --crash')
// RUN:   (case=caseSetDeviceTypeInvalidTypePos not-if-fail='%not --crash')
// RUN:   (case=caseSetDeviceTypeInvalidTypeNeg not-if-fail='%not --crash')
// RUN:   (case=caseSetDeviceTypeNone           not-if-fail='%not --crash')
// RUN:   (case=caseSetDeviceTypeNoNotHost      not-if-fail=%[not-if-host])
// RUN:   (case=caseSetDeviceNumInvalidTypePos  not-if-fail='%not --crash')
// RUN:   (case=caseSetDeviceNumInvalidTypeNeg  not-if-fail='%not --crash')
// RUN:   (case=caseSetDeviceNumNegOneNone      not-if-fail='%not --crash')
// RUN:   (case=caseSetDeviceNumZeroNone        not-if-fail='%not --crash')
// RUN:   (case=caseSetDeviceNumNegOneHost      not-if-fail='%not --crash')
// RUN:   (case=caseSetDeviceNumOneHost         not-if-fail='%not --crash')
// RUN:   (case=caseSetDeviceNumNegOneNotHost   not-if-fail='%not --crash')
// RUN:   (case=caseSetDeviceNumTooLargeNotHost not-if-fail='%not --crash')
// RUN:   (case=caseSetDeviceNumNegOneDefault   not-if-fail='%not --crash')
// RUN:   (case=caseSetDeviceNumTooLargeDefault not-if-fail='%not --crash')
// RUN:   (case=caseSetDeviceNumCurrent         not-if-fail=              )
// RUN:   (case=caseSetDeviceNumNegOneNVIDIA    not-if-fail='%not --crash')
// RUN:   (case=caseSetDeviceNumTooLargeNVIDIA  not-if-fail='%not --crash')
// RUN:   (case=caseSetDeviceNumNegOneRADEON    not-if-fail='%not --crash')
// RUN:   (case=caseSetDeviceNumTooLargeRADEON  not-if-fail='%not --crash')
// RUN:   (case=caseSetDeviceNumNegOneX86_64    not-if-fail='%not --crash')
// RUN:   (case=caseSetDeviceNumTooLargeX86_64  not-if-fail='%not --crash')
// RUN:   (case=caseSetDeviceNumNegOnePPC64LE   not-if-fail='%not --crash')
// RUN:   (case=caseSetDeviceNumTooLargePPC64LE not-if-fail='%not --crash')
// RUN:   (case=caseGetDeviceNumInvalidTypePos  not-if-fail='%not --crash')
// RUN:   (case=caseGetDeviceNumInvalidTypeNeg  not-if-fail='%not --crash')
// RUN: }
// RUN: echo '#define FOREACH_CASE(Macro) \' > %t-cases.h
// RUN: %for cases {
// RUN:   echo '  Macro(%[case]) \' >> %t-cases.h
// RUN: }
// RUN: echo '  /*end of FOREACH_CASE*/' >> %t-cases.h
// RUN: %for tgts {
// RUN:   %[run-if] %clang -fopenacc %acc-includes %[cflags] \
// RUN:                    -DCASES_HEADER='"%t-cases.h"' -o %t.exe %s
// RUN:   %for run-envs {
// RUN:     %for cases {
// RUN:       %[run-if] %[not-if-fail] %[run-env] %t.exe %[case] \
// RUN:           > %t.out 2> %t.err
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

// FIXME: Clang produces spurious warning diagnostics for nvptx64 offload.  This
// issue is not limited to Clacc and is present upstream:
// nvptx64-warning@*:* 0+ {{Linking two modules of different data layouts}}
// nvptx64-warning@*:* 0+ {{Linking two modules of different target triples}}

#include <assert.h>
#include <openacc.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// OUT-NOT: {{.}}
// ERR-NOT: {{.}}

static int devTypeInvalidPos() {
  int i = 999;
#define DEVICE_CASE(DevType)                                                   \
  assert(i != acc_device_##DevType && "expected invalid device type");
  ACC2OMP_FOREACH_DEVICE(DEVICE_CASE)
#undef DEVICE_CASE
  return i;
}

static int devTypeInvalidNeg() {
  return -1;
}

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

  // OUT: start out
  // ERR: start err
  fprintf(stdout, "start out\n");
  fprintf(stderr, "start err\n");
  fflush(stdout);
  fflush(stderr);

  // ERR-NEXT: devTypeInvalidPos() = [[#%u,DEV_TYPE_INVALID_POS:]]
  // ERR-NEXT: devTypeInvalidNeg() = [[#%d,DEV_TYPE_INVALID_NEG:]]
  fprintf(stderr, "devTypeInvalidPos() = %d\n", devTypeInvalidPos());
  fprintf(stderr, "devTypeInvalidNeg() = %d\n", devTypeInvalidNeg());

  caseFn();
  return 0;
}

CASE(caseGetNumDevicesInvalidTypePos) {
  // ERR-caseGetNumDevicesInvalidTypePos-NEXT: OMP: Error #[[#]]: acc_get_num_devices called for invalid device type [[#DEV_TYPE_INVALID_POS]]
  acc_get_num_devices(devTypeInvalidPos());
}
CASE(caseGetNumDevicesInvalidTypeNeg) {
  // ERR-caseGetNumDevicesInvalidTypeNeg-NEXT: OMP: Error #[[#]]: acc_get_num_devices called for invalid device type [[#DEV_TYPE_INVALID_NEG]]
  acc_get_num_devices(devTypeInvalidNeg());
}

CASE(caseSetDeviceTypeInvalidTypePos) {
  // ERR-caseSetDeviceTypeInvalidTypePos-NEXT: OMP: Error #[[#]]: acc_set_device_type called for invalid device type [[#DEV_TYPE_INVALID_POS]]
  acc_set_device_type(devTypeInvalidPos());
}
CASE(caseSetDeviceTypeInvalidTypeNeg) {
  // ERR-caseSetDeviceTypeInvalidTypeNeg-NEXT: OMP: Error #[[#]]: acc_set_device_type called for invalid device type [[#DEV_TYPE_INVALID_NEG]]
  acc_set_device_type(devTypeInvalidNeg());
}

CASE(caseSetDeviceTypeNone) {
  // ERR-caseSetDeviceTypeNone-NEXT: OMP: Error #[[#]]: acc_set_device_type called for acc_device_none, which has no available devices
  acc_set_device_type(acc_device_none);
}
CASE(caseSetDeviceTypeNoNotHost) {
  // ERR-caseSetDeviceTypeNoNotHost-HOST-NEXT: OMP: Error #[[#]]: acc_set_device_type called for acc_device_not_host, which has no available devices
  // OUT-caseSetDeviceTypeNoNotHost-OFF-NEXT: success
  acc_set_device_type(acc_device_not_host);
  printf("success\n");
}

CASE(caseSetDeviceNumInvalidTypePos) {
  // ERR-caseSetDeviceNumInvalidTypePos-NEXT: OMP: Error #[[#]]: acc_set_device_num called for invalid device type [[#DEV_TYPE_INVALID_POS]]
  acc_set_device_num(0, devTypeInvalidPos());
}
CASE(caseSetDeviceNumInvalidTypeNeg) {
  // ERR-caseSetDeviceNumInvalidTypeNeg-NEXT: OMP: Error #[[#]]: acc_set_device_num called for invalid device type [[#DEV_TYPE_INVALID_NEG]]
  acc_set_device_num(0, devTypeInvalidNeg());
}

CASE(caseSetDeviceNumNegOneNone) {
  // ERR-caseSetDeviceNumNegOneNone-NEXT: OMP: Error #[[#]]: acc_set_device_num called with invalid device number -1 for acc_device_none
  acc_set_device_num(-1, acc_device_none);
}
CASE(caseSetDeviceNumZeroNone) {
  // ERR-caseSetDeviceNumZeroNone-NEXT: OMP: Error #[[#]]: acc_set_device_num called with invalid device number 0 for acc_device_none
  acc_set_device_num(0, acc_device_none);
}
CASE(caseSetDeviceNumNegOneHost) {
  // ERR-caseSetDeviceNumNegOneHost-NEXT: OMP: Error #[[#]]: acc_set_device_num called with invalid device number -1 for acc_device_host
  acc_set_device_num(-1, acc_device_host);
}
CASE(caseSetDeviceNumOneHost) {
  // ERR-caseSetDeviceNumOneHost-NEXT: OMP: Error #[[#]]: acc_set_device_num called with invalid device number 1 for acc_device_host
  acc_set_device_num(1, acc_device_host);
}
CASE(caseSetDeviceNumNegOneNotHost) {
  // ERR-caseSetDeviceNumNegOneNotHost-NEXT: OMP: Error #[[#]]: acc_set_device_num called with invalid device number -1 for acc_device_not_host
  acc_set_device_num(-1, acc_device_not_host);
}
CASE(caseSetDeviceNumTooLargeNotHost) {
  // ERR-caseSetDeviceNumTooLargeNotHost-NEXT: OMP: Error #[[#]]: acc_set_device_num called with invalid device number [[#%u,]] for acc_device_not_host
  acc_set_device_num(acc_get_num_devices(acc_device_not_host),
                     acc_device_not_host);
}
CASE(caseSetDeviceNumNegOneDefault) {
  // ERR-caseSetDeviceNumNegOneDefault-NEXT: OMP: Error #[[#]]: acc_set_device_num called with invalid device number -1 for acc_device_{{host|nvidia|radeon|x86_64|ppc64le}} (from acc_device_default)
  acc_set_device_num(-1, acc_device_default);
}
CASE(caseSetDeviceNumTooLargeDefault) {
  // ERR-caseSetDeviceNumTooLargeDefault-NEXT: OMP: Error #[[#]]: acc_set_device_num called with invalid device number [[#%u,]] for acc_device_{{host|nvidia|radeon|x86_64|ppc64le}} (from acc_device_default)
  acc_set_device_num(acc_get_num_devices(acc_device_default),
                     acc_device_default);
}
CASE(caseSetDeviceNumCurrent) {
  // acc_device_current makes it a no-op regardless of the device number.
  // OUT-caseSetDeviceNumCurrent-NEXT: type=[[#DEV_TYPE:]], num=[[#DEV_NUM:]]
  // OUT-caseSetDeviceNumCurrent-NEXT: type=[[#DEV_TYPE]], num=[[#DEV_NUM]]
  acc_device_t DevType = acc_get_device_type();
  int DevNum = acc_get_device_num(DevType);
  printf("type=%d, num=%d\n", DevType, DevNum);
  acc_set_device_num(-84, acc_device_current);
  acc_set_device_num(-1, acc_device_current);
  acc_set_device_num(0, acc_device_current);
  acc_set_device_num(1, acc_device_current);
  acc_set_device_num(105, acc_device_current);
  DevType = acc_get_device_type();
  DevNum = acc_get_device_num(DevType);
  printf("type=%d, num=%d\n", DevType, DevNum);
}
CASE(caseSetDeviceNumNegOneNVIDIA) {
  // ERR-caseSetDeviceNumNegOneNVIDIA-NEXT: OMP: Error #[[#]]: acc_set_device_num called with invalid device number -1 for acc_device_nvidia
  acc_set_device_num(-1, acc_device_nvidia);
}
CASE(caseSetDeviceNumTooLargeNVIDIA) {
  // ERR-caseSetDeviceNumTooLargeNVIDIA-NEXT: OMP: Error #[[#]]: acc_set_device_num called with invalid device number [[#%u,]] for acc_device_nvidia
  acc_set_device_num(acc_get_num_devices(acc_device_nvidia), acc_device_nvidia);
}
CASE(caseSetDeviceNumNegOneRADEON) {
  // ERR-caseSetDeviceNumNegOneRADEON-NEXT: OMP: Error #[[#]]: acc_set_device_num called with invalid device number -1 for acc_device_radeon
  acc_set_device_num(-1, acc_device_radeon);
}
CASE(caseSetDeviceNumTooLargeRADEON) {
  // ERR-caseSetDeviceNumTooLargeRADEON-NEXT: OMP: Error #[[#]]: acc_set_device_num called with invalid device number [[#%u,]] for acc_device_radeon
  acc_set_device_num(acc_get_num_devices(acc_device_radeon), acc_device_radeon);
}
CASE(caseSetDeviceNumNegOneX86_64) {
  // ERR-caseSetDeviceNumNegOneX86_64-NEXT: OMP: Error #[[#]]: acc_set_device_num called with invalid device number -1 for acc_device_x86_64
  acc_set_device_num(-1, acc_device_x86_64);
}
CASE(caseSetDeviceNumTooLargeX86_64) {
  // ERR-caseSetDeviceNumTooLargeX86_64-NEXT: OMP: Error #[[#]]: acc_set_device_num called with invalid device number [[#%u,]] for acc_device_x86_64
  acc_set_device_num(acc_get_num_devices(acc_device_x86_64), acc_device_x86_64);
}
CASE(caseSetDeviceNumNegOnePPC64LE) {
  // ERR-caseSetDeviceNumNegOnePPC64LE-NEXT: OMP: Error #[[#]]: acc_set_device_num called with invalid device number -1 for acc_device_ppc64le
  acc_set_device_num(-1, acc_device_ppc64le);
}
CASE(caseSetDeviceNumTooLargePPC64LE) {
  // ERR-caseSetDeviceNumTooLargePPC64LE-NEXT: OMP: Error #[[#]]: acc_set_device_num called with invalid device number [[#%u,]] for acc_device_ppc64le
  acc_set_device_num(acc_get_num_devices(acc_device_ppc64le),
                     acc_device_ppc64le);
}

CASE(caseGetDeviceNumInvalidTypePos) {
  // ERR-caseGetDeviceNumInvalidTypePos-NEXT: OMP: Error #[[#]]: acc_get_device_num called for invalid device type [[#DEV_TYPE_INVALID_POS]]
  acc_get_device_num(devTypeInvalidPos());
}
CASE(caseGetDeviceNumInvalidTypeNeg) {
  // ERR-caseGetDeviceNumInvalidTypeNeg-NEXT: OMP: Error #[[#]]: acc_get_device_num called for invalid device type [[#DEV_TYPE_INVALID_NEG]]
  acc_get_device_num(devTypeInvalidNeg());
}

// OUT-NOT: {{.}}
// An abort messages usually follows any error.
// ERR-NOT: {{(OMP:|Libomptarget)}}
