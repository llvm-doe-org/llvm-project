// Check order of acc_register_library calls when using ACC_PROFLIB.
//
// The runtime first calls any linked acc_register_library, and then it calls
// the acc_register_library in every library listed in ACC_PROFLIB in the order
// the libraries are specified.
//
// Because our OpenACC Profiling Interface support doesn't yet support
// registering multiple callbacks for the same event, we check the order of the
// acc_register_library calls by checking the order of warnings that previously
// registered callbacks cannot be overwritten.
//
// As a final sanity check, we make sure all the original callbacks were
// registered.  This will become more important once multiple callbacks can be
// registered as we'll then need to check their dispatch order.

// RUN: %data proflibs {
// RUN:   (cpp=-DPROG=PROG_PROFLIB1 file=%t.proflib1)
// RUN:   (cpp=-DPROG=PROG_PROFLIB2 file=%t.proflib2)
// RUN: }
// RUN: %for proflibs {
// RUN:   %clang -Xclang -verify %acc-includes -shared -fpic %[cpp] %s \
// RUN:          -o %[file]
// RUN: }
// RUN: %data tgts {
//        Whether the target is host or device affects at least when
//        ompt_start_tool and thus acc_register_library executes, so it seems
//        worthwhile to make sure ACC_PROFLIB works in all cases.
// RUN:   (run-if=                tgt-cflags=                                     fc=HOST)
// RUN:   (run-if=%run-if-x86_64  tgt-cflags=-fopenmp-targets=%run-x86_64-triple  fc=OFF )
// RUN:   (run-if=%run-if-ppc64le tgt-cflags=-fopenmp-targets=%run-ppc64le-triple fc=OFF )
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags=-fopenmp-targets=%run-nvptx64-triple fc=OFF )
// RUN: }
// RUN: %for tgts {
// RUN:   %[run-if] %clang -Xclang -verify -fopenacc %acc-includes \
// RUN:                    -DPROG=PROG_APP %s -o %t %[tgt-cflags]
// RUN:   %[run-if] env ACC_PROFLIB='%t.proflib1;%t.proflib2' %t > %t.out 2>&1
// RUN:   %[run-if] FileCheck -input-file %t.out %s \
// RUN:       -match-full-lines -strict-whitespace -check-prefixes=CHECK,%[fc]
// RUN: }
//
// END.

// expected-no-diagnostics

#include <acc_prof.h>
#include <stdio.h>

#define DEF_CALLBACK(Event)                                   \
static void on_##Event(acc_prof_info *pi, acc_event_info *ei, \
                       acc_api_info *ai) {                    \
  fprintf(stderr, "acc_ev_" #Event ": %s\n", ProgName);       \
}

#define PROG_APP      0
#define PROG_PROFLIB1 1
#define PROG_PROFLIB2 2

// CHECK-NOT:{{.}}

#if PROG == PROG_APP

const char ProgName[] = "app";
DEF_CALLBACK(device_init_start)
DEF_CALLBACK(device_init_end)
DEF_CALLBACK(compute_construct_start)
DEF_CALLBACK(compute_construct_end)

void acc_register_library(acc_prof_reg reg, acc_prof_reg unreg,
                          acc_prof_lookup lookup) {
  reg(acc_ev_device_init_start, on_device_init_start, acc_reg);
  reg(acc_ev_device_init_end, on_device_init_end, acc_reg);
  reg(acc_ev_compute_construct_start, on_compute_construct_start, acc_reg);
  reg(acc_ev_compute_construct_end, on_compute_construct_end, acc_reg);
}

#elif PROG == PROG_PROFLIB1

const char ProgName[] = "proflib1";
DEF_CALLBACK(device_init_end)
DEF_CALLBACK(compute_construct_end)
DEF_CALLBACK(enqueue_launch_start)
DEF_CALLBACK(enqueue_launch_end)
DEF_CALLBACK(runtime_shutdown)

void acc_register_library(acc_prof_reg reg, acc_prof_reg unreg,
                          acc_prof_lookup lookup) {
  //      CHECK:Warning: registering already registered events is not supported: acc_ev_device_init_end
  // CHECK-NEXT:Warning: registering already registered events is not supported: acc_ev_compute_construct_end
  reg(acc_ev_device_init_end, on_device_init_end, acc_reg);
  reg(acc_ev_compute_construct_end, on_compute_construct_end, acc_reg);
  reg(acc_ev_enqueue_launch_start, on_enqueue_launch_start, acc_reg);
  reg(acc_ev_enqueue_launch_end, on_enqueue_launch_end, acc_reg);
  reg(acc_ev_runtime_shutdown, on_runtime_shutdown, acc_reg);
}

#elif PROG == PROG_PROFLIB2

const char ProgName[] = "proflib2";
DEF_CALLBACK(enqueue_launch_end)
DEF_CALLBACK(runtime_shutdown)
DEF_CALLBACK(device_shutdown_start)
DEF_CALLBACK(device_shutdown_end)

void acc_register_library(acc_prof_reg reg, acc_prof_reg unreg,
                          acc_prof_lookup lookup) {
  // CHECK-NEXT:Warning: registering already registered events is not supported: acc_ev_enqueue_launch_end
  // CHECK-NEXT:Warning: registering already registered events is not supported: acc_ev_runtime_shutdown
  reg(acc_ev_enqueue_launch_end, on_enqueue_launch_end, acc_reg);
  reg(acc_ev_runtime_shutdown, on_runtime_shutdown, acc_reg);
  reg(acc_ev_device_shutdown_start, on_device_shutdown_start, acc_reg);
  reg(acc_ev_device_shutdown_end, on_device_shutdown_end, acc_reg);
}

#else
# error "unknown PROG"
#endif

#if PROG == PROG_APP
int main() {
  //   OFF-NEXT:acc_ev_device_init_start: app
  //   OFF-NEXT:acc_ev_device_init_end: app
  // CHECK-NEXT:acc_ev_compute_construct_start: app
  // CHECK-NEXT:acc_ev_enqueue_launch_start: proflib1
  // CHECK-NEXT:acc_ev_enqueue_launch_end: proflib1
  #pragma acc parallel
  ;
  // CHECK-NEXT:acc_ev_compute_construct_end: app
  //   OFF-NEXT:acc_ev_device_shutdown_start: proflib2
  //   OFF-NEXT:acc_ev_device_shutdown_end: proflib2
  // CHECK-NEXT:acc_ev_runtime_shutdown: proflib1
  return 0;
}
#endif

// CHECK-NOT:{{.}}
