// Check that we see the expected callbacks for the various data clauses, when
// data is already present and when it is not.
//
// To keep this test simpler, we don't check the following dimensions of
// behavior here, which are checked in other tests in general terms:
// - Callback data: all-events.c checks this.
// - Targeting host: all-events.c checks this.
// - Using OMP_TARGET_OFFLOAD=disabled: all-events.c checks this.
// - Runtime errors from "present": The Clang OpenACC test suite checks this.
// - Data types and subarray cases: The Clang OpenACC test suite checks this.

// RUN: %data tgts {
// RUN:   (run-if=%run-if-x86_64  tgt-cflags=-fopenmp-targets=%run-x86_64-triple )
// RUN:   (run-if=%run-if-ppc64le tgt-cflags=-fopenmp-targets=%run-ppc64le-triple)
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags=-fopenmp-targets=%run-nvptx64-triple)
// RUN: }
// RUN: %data directives {
// RUN:   (directive=data     data-or-par=DATA)
// RUN:   (directive=parallel data-or-par=PAR )
// RUN: }
// RUN: %for tgts {
// RUN:   %for directives {
// RUN:     %[run-if] %clang -Xclang -verify -fopenacc %acc-includes %s -o %t \
// RUN:         %[tgt-cflags] -DDIRECTIVE=%'directive'
// RUN:     %[run-if] %t > %t.out 2> %t.err
// RUN:     %[run-if] FileCheck -input-file %t.err -allow-empty %s \
// RUN:         -check-prefixes=ERR
// RUN:     %[run-if] FileCheck -input-file %t.out %s \
// RUN:         -match-full-lines -strict-whitespace \
// RUN:         -implicit-check-not=acc_ev_ \
// RUN:         -check-prefixes=OUT,OUT-%[data-or-par]
// RUN:   }
// RUN: }
//
// END.

// expected-no-diagnostics

#include <stdio.h>
#include <string.h>
#include "callbacks.h"

void acc_register_library(acc_prof_reg reg, acc_prof_reg unreg,
                          acc_prof_lookup lookup) {
  register_all_callbacks(reg);
}

// OUT-NOT:{{.}}
// ERR-NOT:{{.}}

int main() {

  // OUT:acc_ev_device_init_start
  // OUT:acc_ev_device_init_end

  int x;

  //--------------------------------------------------
  // Not present.
  //--------------------------------------------------

  // OUT-PAR:acc_ev_compute_construct_start
  //     OUT:acc_ev_enter_data_start
  //     OUT:acc_ev_alloc
  //     OUT:acc_ev_create
  //     OUT:acc_ev_enqueue_upload_start
  //     OUT:acc_ev_enqueue_upload_end
  //     OUT:acc_ev_enter_data_end
  // OUT-PAR:acc_ev_enqueue_launch_start
  // OUT-PAR:acc_ev_enqueue_launch_end
  #pragma acc DIRECTIVE copy(x)
  x = 5;
  //     OUT:acc_ev_exit_data_start
  //     OUT:acc_ev_enqueue_download_start
  //     OUT:acc_ev_enqueue_download_end
  //     OUT:acc_ev_delete
  //     OUT:acc_ev_free
  //     OUT:acc_ev_exit_data_end
  // OUT-PAR:acc_ev_compute_construct_end

  // OUT-PAR:acc_ev_compute_construct_start
  //     OUT:acc_ev_enter_data_start
  //     OUT:acc_ev_alloc
  //     OUT:acc_ev_create
  //     OUT:acc_ev_enqueue_upload_start
  //     OUT:acc_ev_enqueue_upload_end
  //     OUT:acc_ev_enter_data_end
  // OUT-PAR:acc_ev_enqueue_launch_start
  // OUT-PAR:acc_ev_enqueue_launch_end
  #pragma acc DIRECTIVE copyin(x)
  x = 5;
  //     OUT:acc_ev_exit_data_start
  //     OUT:acc_ev_delete
  //     OUT:acc_ev_free
  //     OUT:acc_ev_exit_data_end
  // OUT-PAR:acc_ev_compute_construct_end

  // OUT-PAR:acc_ev_compute_construct_start
  //     OUT:acc_ev_enter_data_start
  //     OUT:acc_ev_alloc
  //     OUT:acc_ev_create
  //     OUT:acc_ev_enter_data_end
  // OUT-PAR:acc_ev_enqueue_launch_start
  // OUT-PAR:acc_ev_enqueue_launch_end
  #pragma acc DIRECTIVE copyout(x)
  x = 5;
  //     OUT:acc_ev_exit_data_start
  //     OUT:acc_ev_enqueue_download_start
  //     OUT:acc_ev_enqueue_download_end
  //     OUT:acc_ev_delete
  //     OUT:acc_ev_free
  //     OUT:acc_ev_exit_data_end
  // OUT-PAR:acc_ev_compute_construct_end

  // OUT-PAR:acc_ev_compute_construct_start
  //     OUT:acc_ev_enter_data_start
  //     OUT:acc_ev_alloc
  //     OUT:acc_ev_create
  //     OUT:acc_ev_enter_data_end
  // OUT-PAR:acc_ev_enqueue_launch_start
  // OUT-PAR:acc_ev_enqueue_launch_end
  #pragma acc DIRECTIVE create(x)
  x = 5;
  //     OUT:acc_ev_exit_data_start
  //     OUT:acc_ev_delete
  //     OUT:acc_ev_free
  //     OUT:acc_ev_exit_data_end
  // OUT-PAR:acc_ev_compute_construct_end

  // OUT-PAR:acc_ev_compute_construct_start
  //     OUT:acc_ev_enter_data_start
  //     OUT:acc_ev_enter_data_end
  // OUT-PAR:acc_ev_enqueue_launch_start
  // OUT-PAR:acc_ev_enqueue_launch_end
  int use = 0;
  #pragma acc DIRECTIVE no_create(x)
  if (use)
    x = 5;
  //     OUT:acc_ev_exit_data_start
  //     OUT:acc_ev_exit_data_end
  // OUT-PAR:acc_ev_compute_construct_end

  //--------------------------------------------------
  // Already present.
  //--------------------------------------------------

  // OUT:acc_ev_enter_data_start
  // OUT:acc_ev_alloc
  // OUT:acc_ev_create
  // OUT:acc_ev_enqueue_upload_start
  // OUT:acc_ev_enqueue_upload_end
  // OUT:acc_ev_enter_data_end
  #pragma acc data copy(x)
  {
    // OUT-PAR:acc_ev_compute_construct_start
    //     OUT:acc_ev_enter_data_start
    //     OUT:acc_ev_enter_data_end
    // OUT-PAR:acc_ev_enqueue_launch_start
    // OUT-PAR:acc_ev_enqueue_launch_end
    #pragma acc DIRECTIVE present(x)
    x = 5;
    //     OUT:acc_ev_exit_data_start
    //     OUT:acc_ev_exit_data_end
    // OUT-PAR:acc_ev_compute_construct_end

    // OUT-PAR:acc_ev_compute_construct_start
    //     OUT:acc_ev_enter_data_start
    //     OUT:acc_ev_enter_data_end
    // OUT-PAR:acc_ev_enqueue_launch_start
    // OUT-PAR:acc_ev_enqueue_launch_end
    #pragma acc DIRECTIVE copy(x)
    x = 5;
    //     OUT:acc_ev_exit_data_start
    //     OUT:acc_ev_exit_data_end
    // OUT-PAR:acc_ev_compute_construct_end

    // OUT-PAR:acc_ev_compute_construct_start
    //     OUT:acc_ev_enter_data_start
    //     OUT:acc_ev_enter_data_end
    // OUT-PAR:acc_ev_enqueue_launch_start
    // OUT-PAR:acc_ev_enqueue_launch_end
    #pragma acc DIRECTIVE copyin(x)
    x = 5;
    //     OUT:acc_ev_exit_data_start
    //     OUT:acc_ev_exit_data_end
    // OUT-PAR:acc_ev_compute_construct_end

    // OUT-PAR:acc_ev_compute_construct_start
    //     OUT:acc_ev_enter_data_start
    //     OUT:acc_ev_enter_data_end
    // OUT-PAR:acc_ev_enqueue_launch_start
    // OUT-PAR:acc_ev_enqueue_launch_end
    #pragma acc DIRECTIVE copyout(x)
    x = 5;
    //     OUT:acc_ev_exit_data_start
    //     OUT:acc_ev_exit_data_end
    // OUT-PAR:acc_ev_compute_construct_end

    // OUT-PAR:acc_ev_compute_construct_start
    //     OUT:acc_ev_enter_data_start
    //     OUT:acc_ev_enter_data_end
    // OUT-PAR:acc_ev_enqueue_launch_start
    // OUT-PAR:acc_ev_enqueue_launch_end
    #pragma acc DIRECTIVE create(x)
    x = 5;
    //     OUT:acc_ev_exit_data_start
    //     OUT:acc_ev_exit_data_end
    // OUT-PAR:acc_ev_compute_construct_end

    // OUT-PAR:acc_ev_compute_construct_start
    //     OUT:acc_ev_enter_data_start
    //     OUT:acc_ev_enter_data_end
    // OUT-PAR:acc_ev_enqueue_launch_start
    // OUT-PAR:acc_ev_enqueue_launch_end
    #pragma acc DIRECTIVE no_create(x)
    x = 5;
    //     OUT:acc_ev_exit_data_start
    //     OUT:acc_ev_exit_data_end
    // OUT-PAR:acc_ev_compute_construct_end
  }
  // OUT:acc_ev_exit_data_start
  // OUT:acc_ev_enqueue_download_start
  // OUT:acc_ev_enqueue_download_end
  // OUT:acc_ev_delete
  // OUT:acc_ev_free
  // OUT:acc_ev_exit_data_end

  // OUT:acc_ev_device_shutdown_start
  // OUT:acc_ev_device_shutdown_end
  // OUT:acc_ev_runtime_shutdown

  return 0;
}
