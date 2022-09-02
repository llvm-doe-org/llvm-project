// Check errors during selection of the initial current device, via
// ACC_DEVICE_TYPE and ACC_DEVICE_NUM, for example.

// RUN: %clang-acc -o %t.exe %s

// DEFINE: %{check}(RUN_ENV %, ERR_MSG %) =                                    \
// DEFINE:   %{RUN_ENV} %not --crash %t.exe > %t.out 2> %t.err &&              \
// DEFINE:   FileCheck -input-file %t.out %s -allow-empty -check-prefix=OUT && \
// DEFINE:   FileCheck -input-file %t.err %s -match-full-lines                 \
// DEFINE:             -check-prefix=ERR -DERR='%{ERR_MSG}'

// Invalid ACC_DEVICE_TYPE.
// RUN: %{check}(env ACC_DEVICE_TYPE=foobar  %, ACC_DEVICE_TYPE is invalid: foobar  %)
// RUN: %{check}(env ACC_DEVICE_TYPE=none    %, ACC_DEVICE_TYPE is invalid: none    %)
// RUN: %{check}(env ACC_DEVICE_TYPE=default %, ACC_DEVICE_TYPE is invalid: default %)
// RUN: %{check}(env ACC_DEVICE_TYPE=current %, ACC_DEVICE_TYPE is invalid: current %)
//
// ACC_DEVICE_NUM parse error.
// RUN: %{check}(env ACC_DEVICE_NUM=foobar %, ACC_DEVICE_NUM is not a non-negative integer: foobar %)
// RUN: %{check}(env ACC_DEVICE_NUM=-1     %, ACC_DEVICE_NUM is not a non-negative integer: -1     %)
// RUN: %{check}(env ACC_DEVICE_NUM=-983   %, ACC_DEVICE_NUM is not a non-negative integer: -983   %)
// RUN: %{check}(env ACC_DEVICE_NUM=1.0    %, ACC_DEVICE_NUM is not a non-negative integer: 1.0    %)
// RUN: %{check}(env ACC_DEVICE_NUM=01     %, ACC_DEVICE_NUM is not a non-negative integer: 01     %)
//
// Invalid default ACC_DEVICE_NUM=0 for ACC_DEVICE_TYPE with offloading disabled.
// RUN: %{check}( env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=not_host %, ACC_DEVICE_NUM=0 (default device number) is invalid for ACC_DEVICE_TYPE=not_host %)
// RUN: %{check}( env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=x86_64   %,   ACC_DEVICE_NUM=0 (default device number) is invalid for ACC_DEVICE_TYPE=x86_64 %)
// RUN: %{check}( env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=ppc64le  %,  ACC_DEVICE_NUM=0 (default device number) is invalid for ACC_DEVICE_TYPE=ppc64le %)
// RUN: %{check}( env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=nvidia   %,   ACC_DEVICE_NUM=0 (default device number) is invalid for ACC_DEVICE_TYPE=nvidia %)
// RUN: %{check}( env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=radeon   %,   ACC_DEVICE_NUM=0 (default device number) is invalid for ACC_DEVICE_TYPE=radeon %)
//
// Invalid explicit ACC_DEVICE_NUM for ACC_DEVICE_TYPE with offloading disabled.
// RUN: %{check}( env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_NUM=1                          %, ACC_DEVICE_NUM=1 is invalid for ACC_DEVICE_TYPE=host (default device type) %)
// RUN: %{check}( env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=host ACC_DEVICE_NUM=1     %, ACC_DEVICE_NUM=1 is invalid for ACC_DEVICE_TYPE=host                       %)
// RUN: %{check}( env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=not_host ACC_DEVICE_NUM=0 %, ACC_DEVICE_NUM=0 is invalid for ACC_DEVICE_TYPE=not_host                   %)
// RUN: %{check}( env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=x86_64 ACC_DEVICE_NUM=0   %, ACC_DEVICE_NUM=0 is invalid for ACC_DEVICE_TYPE=x86_64                     %)
// RUN: %{check}( env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=ppc64le ACC_DEVICE_NUM=0  %, ACC_DEVICE_NUM=0 is invalid for ACC_DEVICE_TYPE=ppc64le                    %)
// RUN: %{check}( env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=nvidia ACC_DEVICE_NUM=0   %, ACC_DEVICE_NUM=0 is invalid for ACC_DEVICE_TYPE=nvidia                     %)
// RUN: %{check}( env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=radeon ACC_DEVICE_NUM=0   %, ACC_DEVICE_NUM=0 is invalid for ACC_DEVICE_TYPE=radeon                     %)
//
// Invalid ACC_DEVICE_NUM for ACC_DEVICE_TYPE with no devices.
// RUN: %{check}( env ACC_DEVICE_TYPE=%bad-dev-type-acc                  %, ACC_DEVICE_NUM=0 (default device number) is invalid for ACC_DEVICE_TYPE=%bad-dev-type-acc %)
// RUN: %{check}( env ACC_DEVICE_TYPE=%bad-dev-type-acc ACC_DEVICE_NUM=0 %, ACC_DEVICE_NUM=0 is invalid for ACC_DEVICE_TYPE=%bad-dev-type-acc                         %)
// RUN: %{check}( env ACC_DEVICE_TYPE=%bad-dev-type-acc ACC_DEVICE_NUM=1 %, ACC_DEVICE_NUM=1 is invalid for ACC_DEVICE_TYPE=%bad-dev-type-acc                         %)
//
// Invalid ACC_DEVICE_NUM for ACC_DEVICE_TYPE with devices.
// RUN: %{check}( env ACC_DEVICE_NUM=%dev-type-0-num-devs                                 %, ACC_DEVICE_NUM=%dev-type-0-num-devs is invalid for ACC_DEVICE_TYPE=%dev-type-0-acc (default device type) %)
// RUN: %{check}( env ACC_DEVICE_TYPE=%dev-type-0-acc ACC_DEVICE_NUM=%dev-type-0-num-devs %, ACC_DEVICE_NUM=%dev-type-0-num-devs is invalid for ACC_DEVICE_TYPE=%dev-type-0-acc                       %)
// RUN: %{check}( env ACC_DEVICE_TYPE=%dev-type-1-acc ACC_DEVICE_NUM=%dev-type-1-num-devs %, ACC_DEVICE_NUM=%dev-type-1-num-devs is invalid for ACC_DEVICE_TYPE=%dev-type-1-acc                       %)
// RUN: %{check}( env ACC_DEVICE_TYPE=not_host ACC_DEVICE_NUM=%not-host-num-devs          %, ACC_DEVICE_NUM=%not-host-num-devs is invalid for ACC_DEVICE_TYPE=not_host                                %)

// END.

// expected-no-diagnostics

// OUT-NOT:{{.}}

// ERR-NOT:{{.}}
//     ERR: OMP: Error #[[#]]: [[ERR]]
// An abort messages usually follows any error.
// ERR-NOT: {{(OMP:|Libomptarget)}}

#include <openacc.h>

int main() {
  // Make the runtime initialize.
  acc_get_device_type();
  return 0;
}
