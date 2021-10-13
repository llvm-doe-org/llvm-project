// Check errors during selection of the initial current device, via
// ACC_DEVICE_TYPE and ACC_DEVICE_NUM, for example.

// RUN: %data run-envs {
//        # Invalid ACC_DEVICE_TYPE.
// RUN:   (run-env='env ACC_DEVICE_TYPE=foobar'  err='ACC_DEVICE_TYPE is invalid: foobar')
// RUN:   (run-env='env ACC_DEVICE_TYPE=none'    err='ACC_DEVICE_TYPE is invalid: none')
// RUN:   (run-env='env ACC_DEVICE_TYPE=default' err='ACC_DEVICE_TYPE is invalid: default')
// RUN:   (run-env='env ACC_DEVICE_TYPE=current' err='ACC_DEVICE_TYPE is invalid: current')
//
//        # ACC_DEVICE_NUM parse error.
// RUN:   (run-env='env ACC_DEVICE_NUM=foobar' err='ACC_DEVICE_NUM is not a non-negative integer: foobar')
// RUN:   (run-env='env ACC_DEVICE_NUM=-1'     err='ACC_DEVICE_NUM is not a non-negative integer: -1'    )
// RUN:   (run-env='env ACC_DEVICE_NUM=-983'   err='ACC_DEVICE_NUM is not a non-negative integer: -983'  )
// RUN:   (run-env='env ACC_DEVICE_NUM=1.0'    err='ACC_DEVICE_NUM is not a non-negative integer: 1.0'   )
// RUN:   (run-env='env ACC_DEVICE_NUM=01'     err='ACC_DEVICE_NUM is not a non-negative integer: 01'    )
//
//        # Invalid default ACC_DEVICE_NUM=0 for ACC_DEVICE_TYPE with offloading
//        # disabled.
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=not_host'
// RUN:    err='ACC_DEVICE_NUM=0 (default device number) is invalid for ACC_DEVICE_TYPE=not_host')
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=x86_64'
// RUN:    err='ACC_DEVICE_NUM=0 (default device number) is invalid for ACC_DEVICE_TYPE=x86_64')
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=ppc64le'
// RUN:    err='ACC_DEVICE_NUM=0 (default device number) is invalid for ACC_DEVICE_TYPE=ppc64le')
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=nvidia'
// RUN:    err='ACC_DEVICE_NUM=0 (default device number) is invalid for ACC_DEVICE_TYPE=nvidia')
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=radeon'
// RUN:    err='ACC_DEVICE_NUM=0 (default device number) is invalid for ACC_DEVICE_TYPE=radeon')
//
//        # Invalid explicit ACC_DEVICE_NUM for ACC_DEVICE_TYPE with offloading
//        # disabled.
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_NUM=1'
// RUN:    err='ACC_DEVICE_NUM=1 is invalid for ACC_DEVICE_TYPE=host (default device type)')
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=host ACC_DEVICE_NUM=1'
// RUN:    err='ACC_DEVICE_NUM=1 is invalid for ACC_DEVICE_TYPE=host')
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=not_host ACC_DEVICE_NUM=0'
// RUN:    err='ACC_DEVICE_NUM=0 is invalid for ACC_DEVICE_TYPE=not_host')
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=x86_64 ACC_DEVICE_NUM=0'
// RUN:    err='ACC_DEVICE_NUM=0 is invalid for ACC_DEVICE_TYPE=x86_64')
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=ppc64le ACC_DEVICE_NUM=0'
// RUN:    err='ACC_DEVICE_NUM=0 is invalid for ACC_DEVICE_TYPE=ppc64le')
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=nvidia ACC_DEVICE_NUM=0'
// RUN:    err='ACC_DEVICE_NUM=0 is invalid for ACC_DEVICE_TYPE=nvidia')
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled ACC_DEVICE_TYPE=radeon ACC_DEVICE_NUM=0'
// RUN:    err='ACC_DEVICE_NUM=0 is invalid for ACC_DEVICE_TYPE=radeon')
//
//        # Invalid ACC_DEVICE_NUM for ACC_DEVICE_TYPE with no devices.
// RUN:   (run-env='env ACC_DEVICE_TYPE=%bad-dev-type-acc'
// RUN:    err='ACC_DEVICE_NUM=0 (default device number) is invalid for ACC_DEVICE_TYPE=%bad-dev-type-acc')
// RUN:   (run-env='env ACC_DEVICE_TYPE=%bad-dev-type-acc ACC_DEVICE_NUM=0'
// RUN:    err='ACC_DEVICE_NUM=0 is invalid for ACC_DEVICE_TYPE=%bad-dev-type-acc')
// RUN:   (run-env='env ACC_DEVICE_TYPE=%bad-dev-type-acc ACC_DEVICE_NUM=1'
// RUN:    err='ACC_DEVICE_NUM=1 is invalid for ACC_DEVICE_TYPE=%bad-dev-type-acc')
//
//        # Invalid ACC_DEVICE_NUM for ACC_DEVICE_TYPE with devices.
// RUN:   (run-env='env ACC_DEVICE_NUM=%dev-type-0-num-devs'
// RUN:    err='ACC_DEVICE_NUM=%dev-type-0-num-devs is invalid for ACC_DEVICE_TYPE=%dev-type-0-acc (default device type)')
// RUN:   (run-env='env ACC_DEVICE_TYPE=%dev-type-0-acc ACC_DEVICE_NUM=%dev-type-0-num-devs'
// RUN:    err='ACC_DEVICE_NUM=%dev-type-0-num-devs is invalid for ACC_DEVICE_TYPE=%dev-type-0-acc')
// RUN:   (run-env='env ACC_DEVICE_TYPE=%dev-type-1-acc ACC_DEVICE_NUM=%dev-type-1-num-devs'
// RUN:    err='ACC_DEVICE_NUM=%dev-type-1-num-devs is invalid for ACC_DEVICE_TYPE=%dev-type-1-acc')
// RUN:   (run-env='env ACC_DEVICE_TYPE=not_host ACC_DEVICE_NUM=%not-host-num-devs'
// RUN:    err='ACC_DEVICE_NUM=%not-host-num-devs is invalid for ACC_DEVICE_TYPE=not_host')
// RUN: }
// RUN: %clang-acc -o %t.exe %s
// RUN: %for run-envs {
// RUN:   %[run-env] %not --crash %t.exe > %t.out 2> %t.err
// RUN:   FileCheck -input-file %t.out %s -allow-empty -check-prefix=OUT
// RUN:   FileCheck -input-file %t.err %s -match-full-lines -check-prefix=ERR \
// RUN:     -DERR=%'err'
// RUN: }
//
// END.

// expected-error 0 {{}}

// FIXME: Clang produces spurious warning diagnostics for nvptx64 offload.  This
// issue is not limited to Clacc and is present upstream:
// nvptx64-warning@*:* 0+ {{Linking two modules of different data layouts}}
// nvptx64-warning@*:* 0+ {{Linking two modules of different target triples}}

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
