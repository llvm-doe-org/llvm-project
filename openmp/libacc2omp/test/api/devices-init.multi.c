// Check selection of the initial current device, via ACC_DEVICE_TYPE and
// ACC_DEVICE_NUM, for example.
//
// Assume device management routines (acc_get_device_type and
// acc_get_device_num) work correctly, as checked in devices.c.

// RUN: %data run-envs {
//        # Default.
// RUN:   (run-env=                                                                                                 type=%dev-type-0-acc num=0                   )
// RUN:   (run-env='env ACC_DEVICE_TYPE= ACC_DEVICE_NUM='                                                           type=%dev-type-0-acc num=0                   )
//
//        # Disabled offloading.
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled'                                                                type=host            num=0                   )
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=host ACC_DEVICE_NUM=0'                          type=host            num=0                   )
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=host'                                           type=host            num=0                   )
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_NUM=0'                                               type=host            num=0                   )
//
//        # OMP_DEFAULT_DEVICE only.  Just check enough to be sure it's not
//        # ignored.  It's up to OpenMP testing to check every scenario.
// RUN:   (run-env='env OMP_DEFAULT_DEVICE=0'                                                                       type=%dev-type-0-acc num=0                   )
// RUN:   (run-env='env OMP_DEFAULT_DEVICE=%dev-type-0-last-dev'                                                    type=%dev-type-0-acc num=%dev-type-0-last-dev)
//
//        # ACC_DEVICE_TYPE and ACC_DEVICE_NUM.
// RUN:   (run-env='env ACC_DEVICE_TYPE=host                     ACC_DEVICE_NUM=0'                                  type=host            num=0                   )
// RUN:   (run-env='env ACC_DEVICE_TYPE=%dev-type-0-acc          ACC_DEVICE_NUM=0'                                  type=%dev-type-0-acc num=0                   )
// RUN:   (run-env='env ACC_DEVICE_TYPE=%dev-type-0-acc          ACC_DEVICE_NUM=%dev-type-0-last-dev'               type=%dev-type-0-acc num=%dev-type-0-last-dev)
// RUN:   (run-env='env ACC_DEVICE_TYPE=%dev-type-1-acc          ACC_DEVICE_NUM=0'                                  type=%dev-type-1-acc num=0                   )
// RUN:   (run-env='env ACC_DEVICE_TYPE=%dev-type-1-acc          ACC_DEVICE_NUM=%dev-type-1-last-dev'               type=%dev-type-1-acc num=%dev-type-1-last-dev)
// RUN:   (run-env='env ACC_DEVICE_TYPE=%if-host(host, not_host) ACC_DEVICE_NUM=0'                                  type=%dev-type-0-acc num=0                   )
// RUN:   (run-env='env ACC_DEVICE_TYPE=%if-host(host, not_host) ACC_DEVICE_NUM=%dev-type-0-last-dev'               type=%dev-type-0-acc num=%dev-type-0-last-dev)
//        # When not compiling for host, this checks the ability to select
//        # devices beyond the first device type via not_host.
// RUN:   (run-env='env ACC_DEVICE_TYPE=%if-host(host, not_host) ACC_DEVICE_NUM=%if-multi(%dev-type-0-num-devs, 0)' type=%dev-type-1-acc num=0                   )
//
//        # ACC_DEVICE_TYPE only.
// RUN:   (run-env='env ACC_DEVICE_TYPE=host'                                                                       type=host            num=0                   )
// RUN:   (run-env='env ACC_DEVICE_TYPE=%dev-type-0-acc'                                                            type=%dev-type-0-acc num=0                   )
// RUN:   (run-env='env ACC_DEVICE_TYPE=%dev-type-1-acc'                                                            type=%dev-type-1-acc num=0                   )
// RUN:   (run-env='env ACC_DEVICE_TYPE=%if-host(host, not_host)'                                                   type=%dev-type-0-acc num=0                   )
// RUN:   (run-env='env ACC_DEVICE_TYPE=%if-host(host, not_host) ACC_DEVICE_NUM='                                   type=%dev-type-0-acc num=0                   )
//
//        # ACC_DEVICE_NUM only.
// RUN:   (run-env='env ACC_DEVICE_NUM=0'                                                                           type=%dev-type-0-acc num=0                   )
// RUN:   (run-env='env ACC_DEVICE_NUM=%dev-type-0-last-dev'                                                        type=%dev-type-0-acc num=%dev-type-0-last-dev)
// RUN:   (run-env='env ACC_DEVICE_NUM=%dev-type-0-last-dev ACC_DEVICE_TYPE='                                       type=%dev-type-0-acc num=%dev-type-0-last-dev)
//
//        # OMP_DEFAULT_DEVICE has precedence.
// RUN:   (run-env='env OMP_DEFAULT_DEVICE=0 ACC_DEVICE_TYPE=host'                                                  type=%dev-type-0-acc num=0                   )
// RUN:   (run-env='env OMP_DEFAULT_DEVICE=0 ACC_DEVICE_TYPE=%dev-type-0-acc'                                       type=%dev-type-0-acc num=0                   )
// RUN:   (run-env='env OMP_DEFAULT_DEVICE=0 ACC_DEVICE_TYPE=%dev-type-1-acc'                                       type=%dev-type-0-acc num=0                   )
// RUN:   (run-env='env OMP_DEFAULT_DEVICE=0 ACC_DEVICE_TYPE=%if-host(host, not_host)'                              type=%dev-type-0-acc num=0                   )
// RUN:   (run-env='env OMP_DEFAULT_DEVICE=0 ACC_DEVICE_NUM=999'                                                    type=%dev-type-0-acc num=0                   )
//
//        # ACC_DEVICE_TYPE is case-insensitive.
// RUN:   (run-env='env ACC_DEVICE_TYPE=%dev-type-0-ACC ACC_DEVICE_NUM=0'                                           type=%dev-type-0-acc num=0                   )
// RUN:   (run-env='env ACC_DEVICE_TYPE=%dev-type-1-ACC ACC_DEVICE_NUM=0'                                           type=%dev-type-1-acc num=0                   )
// RUN:   (run-env='env ACC_DEVICE_TYPE=%if-host(Host, Not_Host) ACC_DEVICE_NUM=0'                                  type=%dev-type-0-acc num=0                   )
// RUN: }
// RUN: %clang-acc -o %t.exe %s
// RUN: %for run-envs {
// RUN:   %[run-env] %t.exe > %t.out 2>&1
// RUN:   FileCheck -input-file %t.out %s \
// RUN:     -strict-whitespace -match-full-lines \
// RUN:     -DDEV_TYPE_INIT=acc_device_%[type] -D#DEV_NUM_INIT=%[num]
// RUN: }
//
// END.

// expected-error 0 {{}}

// FIXME: Clang produces spurious warning diagnostics for nvptx64 offload.  This
// issue is not limited to Clacc and is present upstream:
// nvptx64-warning@*:* 0+ {{Linking two modules of different target triples}}

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
