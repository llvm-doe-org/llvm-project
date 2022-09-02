// Check device management routines with runtime errors.

// Redefine this to specify how %{check-cases} expands for each case.
//
// - CASE = the case name used in the enum and as a command line argument.
// - NOT_CRASH_IF_FAIL = 'not --crash' if the case is expected to fail at run
//   time, or the empty string otherwise.
// - RUN_ENV = an "env" command to set run-time environment variables, or the
//   empty string if none.
// - HOST_OR_OFF = 'HOST' if compute regions will not be offloaded, or 'OFF' if
//   they will.
//
// DEFINE: %{check-case}( CASE %, NOT_CRASH_IF_FAIL %, RUN_ENV %, HOST_OR_OFF %) =

// Substitution to run %{check-case} for each case.
//
// - RUN_ENV = an "env" command to set run-time environment variables, or the
//   empty string if none.
// - HOST_OR_OFF = 'HOST' if compute regions will not be offloaded, or 'OFF' if
//   they will.
// - NOT_IF_HOST = 'not --crash' if HOST_OR_OFF is 'HOST'.
//
// If a case is listed here but is not covered in the code, that case will fail.
// If a case is covered in the code but not listed here, the code will not
// compile because this list produces the enum used by the code.
//
// DEFINE: %{check-cases}( RUN_ENV %, HOST_OR_OFF %, NOT_IF_HOST %) =                                                  \
//                          CASE                               NOT_CRASH_IF_FAIL    RUN_ENV       HOST_OR_OFF
// DEFINE:   %{check-case}( caseGetNumDevicesInvalidTypePos %, %not --crash      %, %{RUN_ENV} %, %{HOST_OR_OFF} %) && \
// DEFINE:   %{check-case}( caseGetNumDevicesInvalidTypeNeg %, %not --crash      %, %{RUN_ENV} %, %{HOST_OR_OFF} %) && \
// DEFINE:   %{check-case}( caseSetDeviceTypeInvalidTypePos %, %not --crash      %, %{RUN_ENV} %, %{HOST_OR_OFF} %) && \
// DEFINE:   %{check-case}( caseSetDeviceTypeInvalidTypeNeg %, %not --crash      %, %{RUN_ENV} %, %{HOST_OR_OFF} %) && \
// DEFINE:   %{check-case}( caseSetDeviceTypeNone           %, %not --crash      %, %{RUN_ENV} %, %{HOST_OR_OFF} %) && \
// DEFINE:   %{check-case}( caseSetDeviceTypeNoNotHost      %, %{NOT_IF_HOST}    %, %{RUN_ENV} %, %{HOST_OR_OFF} %) && \
// DEFINE:   %{check-case}( caseSetDeviceNumInvalidTypePos  %, %not --crash      %, %{RUN_ENV} %, %{HOST_OR_OFF} %) && \
// DEFINE:   %{check-case}( caseSetDeviceNumInvalidTypeNeg  %, %not --crash      %, %{RUN_ENV} %, %{HOST_OR_OFF} %) && \
// DEFINE:   %{check-case}( caseSetDeviceNumNegOneNone      %, %not --crash      %, %{RUN_ENV} %, %{HOST_OR_OFF} %) && \
// DEFINE:   %{check-case}( caseSetDeviceNumZeroNone        %, %not --crash      %, %{RUN_ENV} %, %{HOST_OR_OFF} %) && \
// DEFINE:   %{check-case}( caseSetDeviceNumNegOneHost      %, %not --crash      %, %{RUN_ENV} %, %{HOST_OR_OFF} %) && \
// DEFINE:   %{check-case}( caseSetDeviceNumOneHost         %, %not --crash      %, %{RUN_ENV} %, %{HOST_OR_OFF} %) && \
// DEFINE:   %{check-case}( caseSetDeviceNumNegOneNotHost   %, %not --crash      %, %{RUN_ENV} %, %{HOST_OR_OFF} %) && \
// DEFINE:   %{check-case}( caseSetDeviceNumTooLargeNotHost %, %not --crash      %, %{RUN_ENV} %, %{HOST_OR_OFF} %) && \
// DEFINE:   %{check-case}( caseSetDeviceNumNegOneDefault   %, %not --crash      %, %{RUN_ENV} %, %{HOST_OR_OFF} %) && \
// DEFINE:   %{check-case}( caseSetDeviceNumTooLargeDefault %, %not --crash      %, %{RUN_ENV} %, %{HOST_OR_OFF} %) && \
// DEFINE:   %{check-case}( caseSetDeviceNumCurrent         %,                   %, %{RUN_ENV} %, %{HOST_OR_OFF} %) && \
// DEFINE:   %{check-case}( caseSetDeviceNumNegOneNVIDIA    %, %not --crash      %, %{RUN_ENV} %, %{HOST_OR_OFF} %) && \
// DEFINE:   %{check-case}( caseSetDeviceNumTooLargeNVIDIA  %, %not --crash      %, %{RUN_ENV} %, %{HOST_OR_OFF} %) && \
// DEFINE:   %{check-case}( caseSetDeviceNumNegOneRADEON    %, %not --crash      %, %{RUN_ENV} %, %{HOST_OR_OFF} %) && \
// DEFINE:   %{check-case}( caseSetDeviceNumTooLargeRADEON  %, %not --crash      %, %{RUN_ENV} %, %{HOST_OR_OFF} %) && \
// DEFINE:   %{check-case}( caseSetDeviceNumNegOneX86_64    %, %not --crash      %, %{RUN_ENV} %, %{HOST_OR_OFF} %) && \
// DEFINE:   %{check-case}( caseSetDeviceNumTooLargeX86_64  %, %not --crash      %, %{RUN_ENV} %, %{HOST_OR_OFF} %) && \
// DEFINE:   %{check-case}( caseSetDeviceNumNegOnePPC64LE   %, %not --crash      %, %{RUN_ENV} %, %{HOST_OR_OFF} %) && \
// DEFINE:   %{check-case}( caseSetDeviceNumTooLargePPC64LE %, %not --crash      %, %{RUN_ENV} %, %{HOST_OR_OFF} %) && \
// DEFINE:   %{check-case}( caseGetDeviceNumInvalidTypePos  %, %not --crash      %, %{RUN_ENV} %, %{HOST_OR_OFF} %) && \
// DEFINE:   %{check-case}( caseGetDeviceNumInvalidTypeNeg  %, %not --crash      %, %{RUN_ENV} %, %{HOST_OR_OFF} %)

// Generate the enum of cases.
//
//      RUN: echo '#define FOREACH_CASE(Macro) \' > %t-cases.h
// REDEFINE: %{check-case}( CASE %, NOT_CRASH_IF_FAIL %, RUN_ENV %,            \
// REDEFINE:                HOST_OR_OFF %) =                                   \
// REDEFINE: echo '  Macro(%{CASE}) \' >> %t-cases.h
//      RUN: %{check-cases}(%,%,%)
//      RUN: echo '  /*end of FOREACH_CASE*/' >> %t-cases.h

// Try all cases multiple times while varying the run-time environment.
//
// REDEFINE: %{check-case}( CASE %, NOT_CRASH_IF_FAIL %, RUN_ENV %,            \
// REDEFINE:                HOST_OR_OFF %) =                                   \
// REDEFINE:   : '---------- RUN_ENV: %{RUN_ENV}; CASE: %{CASE} ----------' && \
// REDEFINE:   %{NOT_CRASH_IF_FAIL} %{RUN_ENV} %t.exe %{CASE}                  \
// REDEFINE:     > %t.out 2> %t.err &&                                         \
// REDEFINE:   FileCheck -input-file %t.out %s -match-full-lines -allow-empty  \
// REDEFINE:     -check-prefixes=OUT,OUT-%{CASE},OUT-%{CASE}-%{HOST_OR_OFF} && \
// REDEFINE:   FileCheck -input-file %t.err %s -match-full-lines -allow-empty  \
// REDEFINE:     -check-prefixes=ERR,ERR-%{CASE},ERR-%{CASE}-%{HOST_OR_OFF}
//
// RUN: %clang-acc -DCASES_HEADER='"%t-cases.h"' -o %t.exe %s
//
//                      RUN_ENV                            HOST_OR_OFF           NOT_IF_HOST
// RUN: %{check-cases}(                                 %, %if-host<HOST|OFF> %, %if-host<%not --crash|> %)
// RUN: %{check-cases}( env OMP_TARGET_OFFLOAD=disabled %, HOST               %, %not --crash            %)

//
// END.

// expected-no-diagnostics

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
