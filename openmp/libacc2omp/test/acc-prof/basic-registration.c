// Check various basic registration/unregistration scenarios.

// RUN: %data tgts {
// RUN:   (run-if=                tgt-cflags=                                     fc=HOST       )
// RUN:   (run-if=%run-if-x86_64  tgt-cflags=-fopenmp-targets=%run-x86_64-triple  fc=OFF,X86_64 )
// RUN:   (run-if=%run-if-ppc64le tgt-cflags=-fopenmp-targets=%run-ppc64le-triple fc=OFF,PPC64LE)
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags=-fopenmp-targets=%run-nvptx64-triple fc=OFF,NVPTX64)
// RUN: }
// RUN: %for tgts {
// RUN:   %[run-if] %clang -Xclang -verify -fopenacc %acc-includes %s -o %t \
// RUN:                    %[tgt-cflags]
// RUN:   %[run-if] %t > %t.out 2> %t.err
// RUN:   %[run-if] FileCheck -input-file %t.err %s \
// RUN:       -match-full-lines -strict-whitespace -check-prefix=ERR
// RUN:   %[run-if] FileCheck -input-file %t.out %s \
// RUN:       -match-full-lines -strict-whitespace \
// RUN:       -implicit-check-not=acc_ev_ -check-prefixes=CHECK,%[fc]
// RUN: }
//
// END.

// expected-no-diagnostics

#include "callbacks.h"

static void on_device_init_end_dup(acc_prof_info *pi, acc_event_info *ei,
                                   acc_api_info *ai) {
  printf("acc_ev_device_init_end duplicate\n");
  print_info(pi, ei, ai);
}

// ERR-NOT:{{.}}
void acc_register_library(acc_prof_reg reg, acc_prof_reg unreg,
                          acc_prof_lookup lookup) {
  // Check registering multiple callbacks for the same event.  Currently, this
  // is not supported, and only the first callback remains registered.
  //      ERR:OMP: Warning #[[#]]: registering already registered events is not yet supported: acc_ev_device_init_start
  // ERR-NEXT:OMP: Warning #[[#]]: registering already registered events is not yet supported: acc_ev_device_init_start
  // ERR-NEXT:OMP: Warning #[[#]]: registering already registered events is not yet supported: acc_ev_device_init_end
  // ERR-NEXT:OMP: Warning #[[#]]: registering already registered events is not yet supported: acc_ev_device_init_end
  // ERR-NEXT:OMP: Warning #[[#]]: registering already registered events is not yet supported: acc_ev_device_init_end
  reg(acc_ev_device_init_start, on_device_init_start, acc_reg);
  reg(acc_ev_device_init_start, on_device_init_start, acc_reg);
  reg(acc_ev_device_init_start, on_device_init_start, acc_reg);
  reg(acc_ev_device_init_end, on_device_init_end, acc_reg);
  reg(acc_ev_device_init_end, on_device_init_end_dup, acc_reg);
  reg(acc_ev_device_init_end, on_device_init_end, acc_reg);
  reg(acc_ev_device_init_end, on_device_init_end_dup, acc_reg);

  // Check unregistering a previously registered callback.
  reg(acc_ev_device_shutdown_start, on_device_shutdown_start, acc_reg);
  unreg(acc_ev_device_shutdown_start, on_device_shutdown_start, acc_reg);

  // Check re-registering a previously unregistered callback.
  reg(acc_ev_device_shutdown_end, on_device_shutdown_end, acc_reg);
  unreg(acc_ev_device_shutdown_end, on_device_shutdown_end, acc_reg);
  reg(acc_ev_device_shutdown_end, on_device_shutdown_end, acc_reg);

  // Check unregistering a callback for an event for which no callback has
  // previously been registered.  This leaves nothing registered.  Check we can
  // then register after that.
  // ERR-NEXT:OMP: Warning #[[#]]: attempt to unregister event not previously registered: acc_ev_runtime_shutdown
  // ERR-NEXT:OMP: Warning #[[#]]: attempt to unregister event not previously registered: acc_ev_create
  unreg(acc_ev_runtime_shutdown, on_runtime_shutdown, acc_reg);
  unreg(acc_ev_create, on_create, acc_reg);
  reg(acc_ev_create, on_create, acc_reg);

  // Check unregistering the wrong callback for an event.  The previously
  // registered callback is left registered.  Check we can then unregister the
  // correct callback after that.
  // ERR-NEXT:OMP: Warning #[[#]]: attempt to unregister wrong callback for event: acc_ev_delete
  // ERR-NEXT:OMP: Warning #[[#]]: attempt to unregister wrong callback for event: acc_ev_alloc
  reg(acc_ev_delete, on_delete, acc_reg);
  unreg(acc_ev_delete, on_create, acc_reg);
  reg(acc_ev_alloc, on_alloc, acc_reg);
  unreg(acc_ev_alloc, on_delete, acc_reg);
  unreg(acc_ev_alloc, on_alloc, acc_reg);

  // Check use of acc_toggle or acc_toggle_per_thread.  Currently, these are
  // not supported and their use has no effect except a warning.
  //
  // First, check for events without previously registered callbacks.
  // ERR-NEXT:OMP: Warning #[[#]]: toggling events is not yet supported: acc_ev_free
  // ERR-NEXT:OMP: Warning #[[#]]: toggling events is not yet supported: acc_ev_enter_data_start
  // ERR-NEXT:OMP: Warning #[[#]]: toggling events is not yet supported: acc_ev_enter_data_end
  // ERR-NEXT:OMP: Warning #[[#]]: toggling events is not yet supported: acc_ev_exit_data_start
  reg(acc_ev_free, on_free, acc_toggle);
  reg(acc_ev_enter_data_start, on_enter_data_start, acc_toggle_per_thread);
  unreg(acc_ev_enter_data_end, on_enter_data_end, acc_toggle);
  unreg(acc_ev_exit_data_start, on_exit_data_start, acc_toggle_per_thread);
  // Second, check for events with previously registered callbacks.
  // ERR-NEXT:OMP: Warning #[[#]]: toggling events is not yet supported: acc_ev_exit_data_end
  // ERR-NEXT:OMP: Warning #[[#]]: toggling events is not yet supported: acc_ev_compute_construct_start
  // ERR-NEXT:OMP: Warning #[[#]]: toggling events is not yet supported: acc_ev_compute_construct_end
  // ERR-NEXT:OMP: Warning #[[#]]: toggling events is not yet supported: acc_ev_enqueue_launch_start
  reg(acc_ev_exit_data_end, on_exit_data_end, acc_reg);
  reg(acc_ev_compute_construct_start, on_compute_construct_start, acc_reg);
  reg(acc_ev_compute_construct_end, on_compute_construct_end, acc_reg);
  reg(acc_ev_enqueue_launch_start, on_enqueue_launch_start, acc_reg);
  reg(acc_ev_exit_data_end, on_exit_data_end, acc_toggle);
  reg(acc_ev_compute_construct_start, on_compute_construct_start, acc_toggle_per_thread);
  unreg(acc_ev_compute_construct_end, on_compute_construct_end, acc_toggle);
  unreg(acc_ev_enqueue_launch_start, on_enqueue_launch_start, acc_toggle_per_thread);

  // Check (un)registering unimplemented callbacks.
  // ERR-NEXT:OMP: Warning #[[#]]: attempt to unregister event that is not yet supported: acc_ev_wait_start
  // ERR-NEXT:OMP: Warning #[[#]]: attempt to register event that is not yet supported: acc_ev_wait_end
  unreg(acc_ev_wait_start, on_wait_start, acc_reg);
  reg(acc_ev_wait_end, on_wait_end, acc_reg);

  // Remaining events that could be used in future extensions to this test.
  reg(acc_ev_update_start, on_update_start, acc_reg);
  reg(acc_ev_update_end, on_update_end, acc_reg);
  reg(acc_ev_enqueue_launch_end, on_enqueue_launch_end, acc_reg);
  reg(acc_ev_enqueue_upload_start, on_enqueue_upload_start, acc_reg);
  reg(acc_ev_enqueue_upload_end, on_enqueue_upload_end, acc_reg);
  reg(acc_ev_enqueue_download_start, on_enqueue_download_start, acc_reg);
  reg(acc_ev_enqueue_download_end, on_enqueue_download_end, acc_reg);
}
// ERR-NOT:{{.}}

int main() {
  int arr[2] = {10, 11};
  //   OFF:acc_ev_device_init_start
  //   OFF:acc_ev_device_init_end
  // CHECK:acc_ev_compute_construct_start
  //       acc_ev_enter_data_start not registered
  //       acc_ev_alloc not registered
  //   OFF:acc_ev_create
  //   OFF:acc_ev_enqueue_upload_start
  //   OFF:acc_ev_enqueue_upload_end
  //       acc_ev_enter_data_end not registered
  // CHECK:acc_ev_enqueue_launch_start
  // CHECK:acc_ev_enqueue_launch_end
  #pragma acc parallel copy(arr) num_gangs(1)
  for (int j = 0; j < 2; ++j)
    printf("arr[%d]=%d\n", j, arr[j]);
  //       acc_ev_exit_data_start not registered
  //   OFF:acc_ev_enqueue_download_start
  //   OFF:acc_ev_enqueue_download_end
  //   OFF:acc_ev_delete
  //       acc_ev_free not registered
  //   OFF:acc_ev_exit_data_end
  // CHECK:acc_ev_compute_construct_end
  return 0;
}
//     acc_ev_device_shutdown_start not registered
// OFF:acc_ev_device_shutdown_end
//     acc_ev_runtime_shutdown not registered
