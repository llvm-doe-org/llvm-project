// Check that no callbacks are missed upon a device allocation failure.

// RUN: %data tgts {
// RUN:   (run-if=                tgt-cflags=                                     host-or-off-pre-env=HOST not-if-off-pre-env=              )
// RUN:   (run-if=%run-if-x86_64  tgt-cflags=-fopenmp-targets=%run-x86_64-triple  host-or-off-pre-env=OFF  not-if-off-pre-env='%not --crash')
// RUN:   (run-if=%run-if-ppc64le tgt-cflags=-fopenmp-targets=%run-ppc64le-triple host-or-off-pre-env=OFF  not-if-off-pre-env='%not --crash')
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags=-fopenmp-targets=%run-nvptx64-triple host-or-off-pre-env=OFF  not-if-off-pre-env='%not --crash')
// RUN: }
// RUN: %data run-envs {
// RUN:   (run-env=                                  host-or-off-post-env=%[host-or-off-pre-env] not-if-off-post-env=%[not-if-off-pre-env])
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled' host-or-off-post-env=HOST                   not-if-off-post-env=                     )
// RUN:   (run-env='env ACC_DEVICE_TYPE=host'        host-or-off-post-env=HOST                   not-if-off-post-env=                     )
// RUN: }
// RUN: %for tgts {
// RUN:   %[run-if] %clang -Xclang -verify -fopenacc %acc-includes %s -o %t \
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
#include <stdlib.h>
#include <string.h>

void acc_register_library(acc_prof_reg reg, acc_prof_reg unreg,
                          acc_prof_lookup lookup) {
  register_all_callbacks(reg);
}

#define PRINT_SUBARRAY_INFO(Arr, Start, Length) \
  fprintf(stderr, "addr=%p, size=%ld\n", &(Arr)[Start], \
          Length * sizeof (Arr[0]))

int main() {
  int arr[4] = {0, 1, 2, 3};
  PRINT_SUBARRAY_INFO(arr, 0, 2);
  PRINT_SUBARRAY_INFO(arr, 1, 2);
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

//               ERR-NOT:{{.}}
//                   ERR:addr=0x[[#%x,OLD_MAP_ADDR:]], size=[[#%u,OLD_MAP_SIZE:]]
//              ERR-NEXT:addr=0x[[#%x,NEW_MAP_ADDR:]], size=[[#%u,NEW_MAP_SIZE:]]
// ERR-OFF-POST-ENV-NEXT:Libomptarget message: explicit extension not allowed: host address specified is 0x{{0*}}[[#NEW_MAP_ADDR]] ([[#NEW_MAP_SIZE]] bytes), but device allocation maps to host at 0x{{0*}}[[#OLD_MAP_ADDR]] ([[#OLD_MAP_SIZE]] bytes)
//                       # FIXME: getOrAllocTgtPtr is meaningless to users.
// ERR-OFF-POST-ENV-NEXT:Libomptarget error: Call to getOrAllocTgtPtr returned null pointer (device failure or illegal mapping).
// ERR-OFF-POST-ENV-NEXT:Libomptarget error: run with env LIBOMPTARGET_INFO>1 to dump host-target pointer maps
// ERR-OFF-POST-ENV-NEXT:Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                       # An abort message usually follows.
//  ERR-OFF-POST-ENV-NOT:Libomptarget
