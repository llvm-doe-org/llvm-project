// Check errors during selection of the initial current device, via
// ACC_DEVICE_TYPE and ACC_DEVICE_NUM, for example.

// RUN: %data tgts {
// RUN:   (run-if=                                  cflags='                                                         -Xclang -verify' tgt0=host    tgt1=host    bad-tgt=not_host not-host-ndevs=0                    )
// RUN:   (run-if=%run-if-x86_64                    cflags='-fopenmp-targets=%run-x86_64-triple                      -Xclang -verify' tgt0=x86_64  tgt1=x86_64  bad-tgt=ppc64le  not-host-ndevs=%x86_64-ndevs        )
// RUN:   (run-if=%run-if-ppc64le                   cflags='-fopenmp-targets=%run-ppc64le-triple                     -Xclang -verify' tgt0=ppc64le tgt1=ppc64le bad-tgt=nvidia   not-host-ndevs=%ppc64le-ndevs       )
// RUN:   (run-if=%run-if-nvptx64                   cflags='-fopenmp-targets=%run-nvptx64-triple                     -Xclang -verify=nvptx64' tgt0=nvidia  tgt1=nvidia  bad-tgt=x86_64   not-host-ndevs=%nvidia-ndevs        )
// RUN:   (run-if='%run-if-x86_64 %run-if-nvptx64'  cflags='-fopenmp-targets=%run-x86_64-triple,%run-nvptx64-triple  -Xclang -verify=nvptx64' tgt0=x86_64  tgt1=nvidia  bad-tgt=ppc64le  not-host-ndevs=%x86_64-nvidia-ndevs )
// RUN:   (run-if='%run-if-ppc64le %run-if-nvptx64' cflags='-fopenmp-targets=%run-ppc64le-triple,%run-nvptx64-triple -Xclang -verify=nvptx64' tgt0=ppc64le tgt1=nvidia  bad-tgt=x86_64   not-host-ndevs=%ppc64le-nvidia-ndevs)
// RUN: }
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
//
//        # Invalid ACC_DEVICE_NUM for ACC_DEVICE_TYPE with no devices.
// RUN:   (run-env='env ACC_DEVICE_TYPE=%[bad-tgt]'
// RUN:    err='ACC_DEVICE_NUM=0 (default device number) is invalid for ACC_DEVICE_TYPE=%[bad-tgt]')
// RUN:   (run-env='env ACC_DEVICE_TYPE=%[bad-tgt] ACC_DEVICE_NUM=0'
// RUN:    err='ACC_DEVICE_NUM=0 is invalid for ACC_DEVICE_TYPE=%[bad-tgt]')
// RUN:   (run-env='env ACC_DEVICE_TYPE=%[bad-tgt] ACC_DEVICE_NUM=1'
// RUN:    err='ACC_DEVICE_NUM=1 is invalid for ACC_DEVICE_TYPE=%[bad-tgt]')
//
//        # Invalid ACC_DEVICE_NUM for ACC_DEVICE_TYPE with devices.
// RUN:   (run-env='env ACC_DEVICE_NUM=%%[tgt0]-ndevs'
// RUN:    err='ACC_DEVICE_NUM=%%[tgt0]-ndevs is invalid for ACC_DEVICE_TYPE=%[tgt0] (default device type)')
// RUN:   (run-env='env ACC_DEVICE_TYPE=%[tgt0] ACC_DEVICE_NUM=%%[tgt0]-ndevs'
// RUN:    err='ACC_DEVICE_NUM=%%[tgt0]-ndevs is invalid for ACC_DEVICE_TYPE=%[tgt0]')
// RUN:   (run-env='env ACC_DEVICE_TYPE=%[tgt1] ACC_DEVICE_NUM=%%[tgt1]-ndevs'
// RUN:    err='ACC_DEVICE_NUM=%%[tgt1]-ndevs is invalid for ACC_DEVICE_TYPE=%[tgt1]')
// RUN:   (run-env='env ACC_DEVICE_TYPE=not_host ACC_DEVICE_NUM=%[not-host-ndevs]'
// RUN:    err='ACC_DEVICE_NUM=%[not-host-ndevs] is invalid for ACC_DEVICE_TYPE=not_host')
// RUN: }
// RUN: %for tgts {
// RUN:   %[run-if] %clang -fopenacc %acc-includes %[cflags] -o %t.exe %s
// RUN:   %for run-envs {
// RUN:     %[run-if] %[run-env] %not --crash %t.exe > %t.out 2> %t.err
// RUN:     %[run-if] FileCheck -input-file %t.out %s -allow-empty \
// RUN:       -check-prefix=OUT
// RUN:     %[run-if] FileCheck -input-file %t.err %s -match-full-lines \
// RUN:       -check-prefix=ERR -DERR=%'err'
// RUN:   }
// RUN: }
//
// END.

// expected-no-diagnostics

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
