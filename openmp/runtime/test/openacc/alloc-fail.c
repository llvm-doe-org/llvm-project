// Check that no callbacks are missed upon a device allocation failure.

// REQUIRES: ompt
//
// RUN: %data tgts {
// RUN:   (run-if=                tgt-cflags=                                     host-or-off-pre-env=HOST not-if-off-pre-env=              )
// RUN:   (run-if=%run-if-x86_64  tgt-cflags=-fopenmp-targets=%run-x86_64-triple  host-or-off-pre-env=OFF  not-if-off-pre-env='%not --crash')
// RUN:   (run-if=%run-if-ppc64le tgt-cflags=-fopenmp-targets=%run-ppc64le-triple host-or-off-pre-env=OFF  not-if-off-pre-env='%not --crash')
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags=-fopenmp-targets=%run-nvptx64-triple host-or-off-pre-env=OFF  not-if-off-pre-env='%not --crash')
// RUN: }
// RUN: %data run-envs {
// RUN:   (run-env=                                  host-or-off-post-env=%[host-or-off-pre-env] not-if-off-post-env=%[not-if-off-pre-env])
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled' host-or-off-post-env=HOST                   not-if-off-post-env=                     )
// RUN: }
// RUN: %for tgts {
// RUN:   %[run-if] %clang -Xclang -verify -fopenacc %flags %s -o %t \
// RUN:                    %[tgt-cflags]
// RUN:   %for run-envs {
// RUN:     %[run-if] %[not-if-off-post-env] %[run-env] %t > %t.out 2> %t.err
// RUN:     %[run-if] FileCheck -input-file %t.err -allow-empty %s \
// RUN:       -check-prefixes=ERR,ERR-%[host-or-off-post-env]-POST-ENV
// RUN:     %[run-if] FileCheck -input-file %t.out %s \
// RUN:       -match-full-lines -strict-whitespace -allow-empty \
// RUN:       -implicit-check-not=acc_ev_ \
// RUN:       -check-prefixes=OUT,OUT-%[host-or-off-pre-env]-PRE-ENV \
// RUN:       -check-prefixes=OUT-%[host-or-off-post-env]-POST-ENV \
// RUN:       -check-prefixes=OUT-%[host-or-off-pre-env]-THEN-%[host-or-off-post-env]
// RUN:   }
// RUN: }
//
// END.

// expected-no-diagnostics

#include "callbacks.h"

void acc_register_library(acc_prof_reg reg, acc_prof_reg unreg,
                          acc_prof_lookup lookup) {
  register_all_callbacks(reg);
}

int main() {
  int arr[4] = {0, 1, 2, 3};
  #pragma acc data copy(arr[0:2])
  #pragma acc data copy(arr[1:2])
  ;
  return 0;
}

//           OUT-NOT:{{.}}
//  OUT-OFF-POST-ENV:acc_ev_device_init_start
//  OUT-OFF-POST-ENV:acc_ev_device_init_end
//  OUT-OFF-POST-ENV:acc_ev_enter_data_start
//  OUT-OFF-POST-ENV:acc_ev_alloc
//  OUT-OFF-POST-ENV:acc_ev_create
//  OUT-OFF-POST-ENV:acc_ev_enqueue_upload_start
//  OUT-OFF-POST-ENV:acc_ev_enqueue_upload_end
//  OUT-OFF-POST-ENV:acc_ev_enter_data_end
//  OUT-OFF-POST-ENV:acc_ev_enter_data_start
//  OUT-OFF-POST-ENV:acc_ev_enter_data_end
// OUT-OFF-THEN-HOST:acc_ev_runtime_shutdown

//          ERR-NOT:{{.}}
// ERR-OFF-POST-ENV:Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                  # An abort message usually follows.
