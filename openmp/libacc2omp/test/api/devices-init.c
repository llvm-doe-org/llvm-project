// Check selection of the initial current device, via ACC_DEVICE_TYPE and
// ACC_DEVICE_NUM, for example.
//
// Assume device management routines (acc_get_device_type and
// acc_get_device_num) work correctly, as checked in devices.c.

// RUN: %data tgts {
//        # tgt0-case checks case insensitivity for all values of
//        # ACC_DEVICE_TYPE.  ACC_DEVICE_TYPE=%[tgt0-case] and ACC_DEVICE_NUM=0
//        # should always select device type %[tgt0] and device num 0.
//        #
//        # tgt1-via-not-host checks the ability to select devices other than
//        # tgt0 via not_host.  ACC_DEVICE_TYPE=%[not_host] and
//        # ACC_DEVICE_NUM=%[tgt1-via-not-host] should always select device type
//        # %[tgt1] and device num 0.
// RUN:   (run-if=                                  cflags='                                                         -Xclang -verify'         tgt0=host    tgt1=host    not-host=host     tgt0-case=HOST     tgt1-via-not-host=0             )
// RUN:   (run-if=%run-if-x86_64                    cflags='-fopenmp-targets=%run-x86_64-triple                      -Xclang -verify'         tgt0=x86_64  tgt1=x86_64  not-host=not_host tgt0-case=X86_64   tgt1-via-not-host=0             )
// RUN:   (run-if=%run-if-ppc64le                   cflags='-fopenmp-targets=%run-ppc64le-triple                     -Xclang -verify'         tgt0=ppc64le tgt1=ppc64le not-host=not_host tgt0-case=PPC64LE  tgt1-via-not-host=0             )
// RUN:   (run-if=%run-if-nvptx64                   cflags='-fopenmp-targets=%run-nvptx64-triple                     -Xclang -verify=nvptx64' tgt0=nvidia  tgt1=nvidia  not-host=not_host tgt0-case=NVIDIA   tgt1-via-not-host=0             )
// RUN:   (run-if=%run-if-amdgcn                    cflags='-fopenmp-targets=%run-amdgcn-triple                      -Xclang -verify'         tgt0=radeon  tgt1=radeon  not-host=not_host tgt0-case=RADEON   tgt1-via-not-host=0             )
// RUN:   (run-if='%run-if-x86_64 %run-if-nvptx64'  cflags='-fopenmp-targets=%run-x86_64-triple,%run-nvptx64-triple  -Xclang -verify=nvptx64' tgt0=x86_64  tgt1=nvidia  not-host=not_host tgt0-case=NOT_HOST tgt1-via-not-host=%x86_64-ndevs )
// RUN:   (run-if='%run-if-x86_64 %run-if-amdgcn'   cflags='-fopenmp-targets=%run-x86_64-triple,%run-amdgcn-triple   -Xclang -verify'         tgt0=x86_64  tgt1=radeon  not-host=not_host tgt0-case=NOT_HOST tgt1-via-not-host=%x86_64-ndevs )
// RUN:   (run-if='%run-if-ppc64le %run-if-nvptx64' cflags='-fopenmp-targets=%run-ppc64le-triple,%run-nvptx64-triple -Xclang -verify=nvptx64' tgt0=ppc64le tgt1=nvidia  not-host=not_host tgt0-case=Not_Host tgt1-via-not-host=%ppc64le-ndevs)
// RUN: }
// RUN: %data run-envs {
//        # Default.
// RUN:   (run-env=                                                                        type=%[tgt0] num=0              )
// RUN:   (run-env='env ACC_DEVICE_TYPE= ACC_DEVICE_NUM='                                  type=%[tgt0] num=0              )
//
//        # Disabled offloading.
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled'                                       type=host    num=0              )
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=host ACC_DEVICE_NUM=0' type=host    num=0              )
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=host'                  type=host    num=0              )
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_NUM=0'                      type=host    num=0              )
//
//        # OMP_DEFAULT_DEVICE only.  Just check enough to be sure it's not
//        # ignored.  It's up to OpenMP testing to check every scenario.
// RUN:   (run-env='env OMP_DEFAULT_DEVICE=0'                                              type=%[tgt0] num=0              )
// RUN:   (run-env='env OMP_DEFAULT_DEVICE=%%[tgt0]-maxdev'                                type=%[tgt0] num=%%[tgt0]-maxdev)
//
//        # ACC_DEVICE_TYPE and ACC_DEVICE_NUM.
// RUN:   (run-env='env ACC_DEVICE_TYPE=host        ACC_DEVICE_NUM=0'                      type=host    num=0              )
// RUN:   (run-env='env ACC_DEVICE_TYPE=%[tgt0]     ACC_DEVICE_NUM=0'                      type=%[tgt0] num=0              )
// RUN:   (run-env='env ACC_DEVICE_TYPE=%[tgt0]     ACC_DEVICE_NUM=%%[tgt0]-maxdev'        type=%[tgt0] num=%%[tgt0]-maxdev)
// RUN:   (run-env='env ACC_DEVICE_TYPE=%[tgt1]     ACC_DEVICE_NUM=0'                      type=%[tgt1] num=0              )
// RUN:   (run-env='env ACC_DEVICE_TYPE=%[tgt1]     ACC_DEVICE_NUM=%%[tgt1]-maxdev'        type=%[tgt1] num=%%[tgt1]-maxdev)
// RUN:   (run-env='env ACC_DEVICE_TYPE=%[not-host] ACC_DEVICE_NUM=0'                      type=%[tgt0] num=0              )
// RUN:   (run-env='env ACC_DEVICE_TYPE=%[not-host] ACC_DEVICE_NUM=%%[tgt0]-maxdev'        type=%[tgt0] num=%%[tgt0]-maxdev)
// RUN:   (run-env='env ACC_DEVICE_TYPE=%[not-host] ACC_DEVICE_NUM=%[tgt1-via-not-host]'   type=%[tgt1] num=0              )
//
//        # ACC_DEVICE_TYPE only.
// RUN:   (run-env='env ACC_DEVICE_TYPE=host'                                              type=host    num=0              )
// RUN:   (run-env='env ACC_DEVICE_TYPE=%[tgt0]'                                           type=%[tgt0] num=0              )
// RUN:   (run-env='env ACC_DEVICE_TYPE=%[tgt1]'                                           type=%[tgt1] num=0              )
// RUN:   (run-env='env ACC_DEVICE_TYPE=%[not-host]'                                       type=%[tgt0] num=0              )
// RUN:   (run-env='env ACC_DEVICE_TYPE=%[not-host] ACC_DEVICE_NUM='                       type=%[tgt0] num=0              )
//
//        # ACC_DEVICE_NUM only.
// RUN:   (run-env='env ACC_DEVICE_NUM=0'                                                  type=%[tgt0] num=0              )
// RUN:   (run-env='env ACC_DEVICE_NUM=%%[tgt0]-maxdev'                                    type=%[tgt0] num=%%[tgt0]-maxdev)
// RUN:   (run-env='env ACC_DEVICE_NUM=%%[tgt0]-maxdev ACC_DEVICE_TYPE='                   type=%[tgt0] num=%%[tgt0]-maxdev)
//
//        # OMP_DEFAULT_DEVICE has precedence.
// RUN:   (run-env='env OMP_DEFAULT_DEVICE=0 ACC_DEVICE_TYPE=host'                         type=%[tgt0] num=0              )
// RUN:   (run-env='env OMP_DEFAULT_DEVICE=0 ACC_DEVICE_TYPE=%[tgt0]'                      type=%[tgt0] num=0              )
// RUN:   (run-env='env OMP_DEFAULT_DEVICE=0 ACC_DEVICE_TYPE=%[tgt1]'                      type=%[tgt0] num=0              )
// RUN:   (run-env='env OMP_DEFAULT_DEVICE=0 ACC_DEVICE_TYPE=%[not-host]'                  type=%[tgt0] num=0              )
// RUN:   (run-env='env OMP_DEFAULT_DEVICE=0 ACC_DEVICE_NUM=999'                           type=%[tgt0] num=0              )
//
//        # ACC_DEVICE_TYPE is case-insensitive.
// RUN:   (run-env='env ACC_DEVICE_TYPE=%[tgt0-case] ACC_DEVICE_NUM=0'                     type=%[tgt0] num=0              )
// RUN: }
// RUN: %for tgts {
// RUN:   %[run-if] %clang -fopenacc %acc-includes %[cflags] -o %t.exe %s
// RUN:   %for run-envs {
// RUN:     %[run-if] %[run-env] %t.exe > %t.out 2>&1
// RUN:     %[run-if] FileCheck -input-file %t.out %s \
// RUN:       -strict-whitespace -match-full-lines \
// RUN:       -DDEV_TYPE_INIT=acc_device_%[type] -D#DEV_NUM_INIT=%[num]
// RUN:   }
// RUN: }
//
// END.

// expected-no-diagnostics

// FIXME: Clang produces spurious warning diagnostics for nvptx64 offload.  This
// issue is not limited to Clacc and is present upstream:
// nvptx64-warning@*:* 0+ {{Linking two modules of different data layouts}}
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
