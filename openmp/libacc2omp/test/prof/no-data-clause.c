// Check that we don't see data callbacks for compute constructs without data
// regions, as implied by OpenACC 3.1 sec. 2.6.3 L1183-1184:
// 
// "When the program encounters a compute construct with explicit data
// clauses or with implicit data allocation added by the compiler, it
// creates a data region that has a duration of the compute construct."
//
// Also check that synchronization at the end of the kernel works correctly even
// though there's no data synchronization: acc_ev_compute_construct_end occurs
// after the kernel terminates.

// RUN: %data tgts {
// RUN:   (run-if=                cflags=                                     host-or-off=HOST)
// RUN:   (run-if=%run-if-x86_64  cflags=-fopenmp-targets=%run-x86_64-triple  host-or-off=OFF )
// RUN:   (run-if=%run-if-ppc64le cflags=-fopenmp-targets=%run-ppc64le-triple host-or-off=OFF )
// RUN:   (run-if=%run-if-nvptx64 cflags=-fopenmp-targets=%run-nvptx64-triple host-or-off=OFF )
// RUN: }
// RUN: %for tgts {
// RUN:   %[run-if] %clang -Xclang -verify -fopenacc %acc-includes %s -o %t \
// RUN:       %[cflags]
// RUN:   %[run-if] %t > %t.out 2>&1
// RUN:   %[run-if] FileCheck -input-file %t.out %s \
// RUN:       -match-full-lines -strict-whitespace \
// RUN:       -implicit-check-not=acc_ev_ \
// RUN:       -check-prefixes=CHECK,%[host-or-off]
// RUN: }
//
// END.

// expected-no-diagnostics

#include "stdio.h"
#include "callbacks.h"

void acc_register_library(acc_prof_reg reg, acc_prof_reg unreg,
                          acc_prof_lookup lookup) {
  register_all_callbacks(reg);
}

//   OFF:acc_ev_device_init_start
//   OFF:acc_ev_device_init_end
// CHECK:acc_ev_compute_construct_start
// CHECK:acc_ev_enqueue_launch_start
// CHECK:acc_ev_enqueue_launch_end
// CHECK:hello world
// CHECK:acc_ev_compute_construct_end
//   OFF:acc_ev_device_shutdown_start
//   OFF:acc_ev_device_shutdown_end
// CHECK:acc_ev_runtime_shutdown
int main() {
  #pragma acc parallel
  printf("hello world\n");
  return 0;
}
