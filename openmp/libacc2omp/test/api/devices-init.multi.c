// Check selection of the initial current device, via ACC_DEVICE_TYPE and
// ACC_DEVICE_NUM, for example.
//
// Assume device management routines (acc_get_device_type and
// acc_get_device_num) work correctly, as checked in devices.c.

// RUN: %clang-acc -o %t.exe %s

// DEFINE: %{check}( RUN_ENV %, TYPE %, NUM %) =                               \
// DEFINE:   %{RUN_ENV} %t.exe > %t.out 2>&1 &&                                \
// DEFINE:   FileCheck -input-file %t.out %s                                   \
// DEFINE:     -strict-whitespace -match-full-lines                            \
// DEFINE:     -DDEV_TYPE_INIT=acc_device_%{TYPE} -D#DEV_NUM_INIT=%{NUM}

//                RUN_ENV                                                                                         TYPE               NUM
// Default.
// RUN: %{check}(                                                                                              %, %dev-type-0-acc %, 0                    %)
// RUN: %{check}( env ACC_DEVICE_TYPE= ACC_DEVICE_NUM=                                                         %, %dev-type-0-acc %, 0                    %)
//
// Disabled offloading.
// RUN: %{check}(env OMP_TARGET_OFFLOAD=disabled                                                               %, host            %, 0                    %)
// RUN: %{check}(env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=host ACC_DEVICE_NUM=0                         %, host            %, 0                    %)
// RUN: %{check}(env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=host                                          %, host            %, 0                    %)
// RUN: %{check}(env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_NUM=0                                              %, host            %, 0                    %)
//
// OMP_DEFAULT_DEVICE only.  Just check enough to be sure it's not ignored.
// It's up to OpenMP testing to check every scenario.
// RUN: %{check}(env OMP_DEFAULT_DEVICE=0                                                                      %, %dev-type-0-acc %, 0                    %)
// RUN: %{check}(env OMP_DEFAULT_DEVICE=%dev-type-0-last-dev                                                   %, %dev-type-0-acc %, %dev-type-0-last-dev %)
//
// ACC_DEVICE_TYPE and ACC_DEVICE_NUM.
// RUN: %{check}(env ACC_DEVICE_TYPE=host                     ACC_DEVICE_NUM=0                                 %, host            %, 0                    %)
// RUN: %{check}(env ACC_DEVICE_TYPE=%dev-type-0-acc          ACC_DEVICE_NUM=0                                 %, %dev-type-0-acc %, 0                    %)
// RUN: %{check}(env ACC_DEVICE_TYPE=%dev-type-0-acc          ACC_DEVICE_NUM=%dev-type-0-last-dev              %, %dev-type-0-acc %, %dev-type-0-last-dev %)
// RUN: %{check}(env ACC_DEVICE_TYPE=%dev-type-1-acc          ACC_DEVICE_NUM=0                                 %, %dev-type-1-acc %, 0                    %)
// RUN: %{check}(env ACC_DEVICE_TYPE=%dev-type-1-acc          ACC_DEVICE_NUM=%dev-type-1-last-dev              %, %dev-type-1-acc %, %dev-type-1-last-dev %)
// RUN: %{check}(env ACC_DEVICE_TYPE=%if-host<host|not_host>  ACC_DEVICE_NUM=0                                 %, %dev-type-0-acc %, 0                    %)
// RUN: %{check}(env ACC_DEVICE_TYPE=%if-host<host|not_host>  ACC_DEVICE_NUM=%dev-type-0-last-dev              %, %dev-type-0-acc %, %dev-type-0-last-dev %)
// When not compiling for host, this checks the ability to select devices beyond
// the first device type via not_host.
// RUN: %{check}(env ACC_DEVICE_TYPE=%if-host<host|not_host>  ACC_DEVICE_NUM=%if-multi<%dev-type-0-num-devs|0> %, %dev-type-1-acc %, 0                    %)
//
// ACC_DEVICE_TYPE only.
// RUN: %{check}(env ACC_DEVICE_TYPE=host                                                                      %, host            %, 0                    %)
// RUN: %{check}(env ACC_DEVICE_TYPE=%dev-type-0-acc                                                           %, %dev-type-0-acc %, 0                    %)
// RUN: %{check}(env ACC_DEVICE_TYPE=%dev-type-1-acc                                                           %, %dev-type-1-acc %, 0                    %)
// RUN: %{check}(env ACC_DEVICE_TYPE=%if-host<host|not_host>                                                   %, %dev-type-0-acc %, 0                    %)
// RUN: %{check}(env ACC_DEVICE_TYPE=%if-host<host|not_host> ACC_DEVICE_NUM=                                   %, %dev-type-0-acc %, 0                    %)
//
// ACC_DEVICE_NUM only.
// RUN: %{check}(env ACC_DEVICE_NUM=0                                                                          %, %dev-type-0-acc %, 0                    %)
// RUN: %{check}(env ACC_DEVICE_NUM=%dev-type-0-last-dev                                                       %, %dev-type-0-acc %, %dev-type-0-last-dev %)
// RUN: %{check}(env ACC_DEVICE_NUM=%dev-type-0-last-dev ACC_DEVICE_TYPE=                                      %, %dev-type-0-acc %, %dev-type-0-last-dev %)
//
// OMP_DEFAULT_DEVICE has precedence.
// RUN: %{check}(env OMP_DEFAULT_DEVICE=0 ACC_DEVICE_TYPE=host                                                 %, %dev-type-0-acc %, 0                    %)
// RUN: %{check}(env OMP_DEFAULT_DEVICE=0 ACC_DEVICE_TYPE=%dev-type-0-acc                                      %, %dev-type-0-acc %, 0                    %)
// RUN: %{check}(env OMP_DEFAULT_DEVICE=0 ACC_DEVICE_TYPE=%dev-type-1-acc                                      %, %dev-type-0-acc %, 0                    %)
// RUN: %{check}(env OMP_DEFAULT_DEVICE=0 ACC_DEVICE_TYPE=%if-host<host|not_host>                              %, %dev-type-0-acc %, 0                    %)
// RUN: %{check}(env OMP_DEFAULT_DEVICE=0 ACC_DEVICE_NUM=999                                                   %, %dev-type-0-acc %, 0                    %)
//
// ACC_DEVICE_TYPE is case-insensitive.
// RUN: %{check}(env ACC_DEVICE_TYPE=%dev-type-0-ACC ACC_DEVICE_NUM=0                                          %, %dev-type-0-acc %, 0                    %)
// RUN: %{check}(env ACC_DEVICE_TYPE=%dev-type-1-ACC ACC_DEVICE_NUM=0                                          %, %dev-type-1-acc %, 0                    %)
// RUN: %{check}(env ACC_DEVICE_TYPE=%if-host<Host|Not_Host> ACC_DEVICE_NUM=0                                  %, %dev-type-0-acc %, 0                    %)
//
// END.

// expected-no-diagnostics

#include <openacc.h>
#include <stdio.h>

// CHECK-NOT:{{.}}
//      CHECK:acc_get_device_type() = [[DEV_TYPE_INIT]]
// CHECK-NEXT:acc_get_device_num([[DEV_TYPE_INIT]]) = [[#DEV_NUM_INIT]]
// CHECK-NOT:{{.}}

static const char *deviceTypeToStr(acc_device_t DevType) {
  switch (DevType) {
#define DEVCASE(DevTypeEnum)                                                   \
  case acc_device_##DevTypeEnum:                                               \
    return "acc_device_" #DevTypeEnum;
  ACC2OMP_FOREACH_DEVICE(DEVCASE)
#undef DEVCASE
  }
  return "<unknown acc_device_t>";
}

int main() {
  acc_device_t DevTypeInit = acc_get_device_type();
  const char *DevTypeInitStr = deviceTypeToStr(DevTypeInit);
  int DevNumInit = acc_get_device_num(DevTypeInit);
  printf("acc_get_device_type() = %s\n"
         "acc_get_device_num(%s) = %d\n",
         DevTypeInitStr, DevTypeInitStr, DevNumInit);
  return 0;
}
