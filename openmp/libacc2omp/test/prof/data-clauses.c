// Check that we see the expected callbacks for the various data clauses, when
// data is already present and when it is not.
//
// To keep this test simpler, we don't check the following dimensions of
// behavior here, which are checked in other tests in general terms:
// - Callback data: all-events.c checks this.
// - Using OMP_TARGET_OFFLOAD=disabled: all-events.c checks this.
// - Runtime errors from "present": The Clang OpenACC test suite checks this.
// - Data types and subarray cases: The Clang OpenACC test suite checks this.

// DEFINE: %{check}( DIRECTIVE %, DATA_OR_PAR %) =                             \
// DEFINE:   %clang-acc %s -o %t.exe -DDIRECTIVE=%{DIRECTIVE} &&               \
// DEFINE:   %t.exe > %t.out 2> %t.err &&                                      \
// DEFINE:   FileCheck -input-file %t.err -allow-empty %s                      \
// DEFINE:     -check-prefixes=ERR &&                                          \
// DEFINE:   FileCheck -input-file %t.out %s                                   \
// DEFINE:     -match-full-lines -strict-whitespace -allow-empty               \
// DEFINE:     -implicit-check-not=acc_ev_                                     \
// DEFINE:     -check-prefixes=OUT,OUT-%if-host<HOST|OFF>,OUT-%{DATA_OR_PAR}   \
// DEFINE:     -check-prefixes=OUT-%if-host<HOST|OFF>-%{DATA_OR_PAR}
// 
// RUN: %{check}( data     %, DATA %)
// RUN: %{check}( parallel %, PAR  %)
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

  // OUT-OFF:acc_ev_device_init_start
  // OUT-OFF:acc_ev_device_init_end

  int x;

  //--------------------------------------------------
  // Not present.
  //--------------------------------------------------

  // OUT-PAR:acc_ev_compute_construct_start
  // OUT-OFF:acc_ev_enter_data_start
  // OUT-OFF:acc_ev_alloc
  // OUT-OFF:acc_ev_create
  // OUT-OFF:acc_ev_enqueue_upload_start
  // OUT-OFF:acc_ev_enqueue_upload_end
  // OUT-OFF:acc_ev_enter_data_end
  // OUT-PAR:acc_ev_enqueue_launch_start
  // OUT-PAR:acc_ev_enqueue_launch_end
  #pragma acc DIRECTIVE copy(x)
  x = 5;
  // OUT-OFF:acc_ev_exit_data_start
  // OUT-OFF:acc_ev_enqueue_download_start
  // OUT-OFF:acc_ev_enqueue_download_end
  // OUT-OFF:acc_ev_delete
  // OUT-OFF:acc_ev_free
  // OUT-OFF:acc_ev_exit_data_end
  // OUT-PAR:acc_ev_compute_construct_end

  // OUT-PAR:acc_ev_compute_construct_start
  // OUT-OFF:acc_ev_enter_data_start
  // OUT-OFF:acc_ev_alloc
  // OUT-OFF:acc_ev_create
  // OUT-OFF:acc_ev_enqueue_upload_start
  // OUT-OFF:acc_ev_enqueue_upload_end
  // OUT-OFF:acc_ev_enter_data_end
  // OUT-PAR:acc_ev_enqueue_launch_start
  // OUT-PAR:acc_ev_enqueue_launch_end
  #pragma acc DIRECTIVE copyin(x)
  x = 5;
  // OUT-OFF:acc_ev_exit_data_start
  // OUT-OFF:acc_ev_delete
  // OUT-OFF:acc_ev_free
  // OUT-OFF:acc_ev_exit_data_end
  // OUT-PAR:acc_ev_compute_construct_end

  // OUT-PAR:acc_ev_compute_construct_start
  // OUT-OFF:acc_ev_enter_data_start
  // OUT-OFF:acc_ev_alloc
  // OUT-OFF:acc_ev_create
  // OUT-OFF:acc_ev_enter_data_end
  // OUT-PAR:acc_ev_enqueue_launch_start
  // OUT-PAR:acc_ev_enqueue_launch_end
  #pragma acc DIRECTIVE copyout(x)
  x = 5;
  // OUT-OFF:acc_ev_exit_data_start
  // OUT-OFF:acc_ev_enqueue_download_start
  // OUT-OFF:acc_ev_enqueue_download_end
  // OUT-OFF:acc_ev_delete
  // OUT-OFF:acc_ev_free
  // OUT-OFF:acc_ev_exit_data_end
  // OUT-PAR:acc_ev_compute_construct_end

  // OUT-PAR:acc_ev_compute_construct_start
  // OUT-OFF:acc_ev_enter_data_start
  // OUT-OFF:acc_ev_alloc
  // OUT-OFF:acc_ev_create
  // OUT-OFF:acc_ev_enter_data_end
  // OUT-PAR:acc_ev_enqueue_launch_start
  // OUT-PAR:acc_ev_enqueue_launch_end
  #pragma acc DIRECTIVE create(x)
  x = 5;
  // OUT-OFF:acc_ev_exit_data_start
  // OUT-OFF:acc_ev_delete
  // OUT-OFF:acc_ev_free
  // OUT-OFF:acc_ev_exit_data_end
  // OUT-PAR:acc_ev_compute_construct_end

  // OUT-PAR:acc_ev_compute_construct_start
  // OUT-OFF:acc_ev_enter_data_start
  // OUT-OFF:acc_ev_enter_data_end
  // OUT-PAR:acc_ev_enqueue_launch_start
  // OUT-PAR:acc_ev_enqueue_launch_end
  int use = 0;
  #pragma acc DIRECTIVE no_create(x)
  if (use)
    x = 5;
  // OUT-OFF:acc_ev_exit_data_start
  // OUT-OFF:acc_ev_exit_data_end
  // OUT-PAR:acc_ev_compute_construct_end

  //--------------------------------------------------
  // Already present.
  //--------------------------------------------------

  // OUT-OFF:acc_ev_enter_data_start
  // OUT-OFF:acc_ev_alloc
  // OUT-OFF:acc_ev_create
  // OUT-OFF:acc_ev_enqueue_upload_start
  // OUT-OFF:acc_ev_enqueue_upload_end
  // OUT-OFF:acc_ev_enter_data_end
  #pragma acc data copy(x)
  {
    // OUT-PAR:acc_ev_compute_construct_start
    // OUT-OFF:acc_ev_enter_data_start
    // OUT-OFF:acc_ev_enter_data_end
    // OUT-PAR:acc_ev_enqueue_launch_start
    // OUT-PAR:acc_ev_enqueue_launch_end
    #pragma acc DIRECTIVE present(x)
    x = 5;
    // OUT-OFF:acc_ev_exit_data_start
    // OUT-OFF:acc_ev_exit_data_end
    // OUT-PAR:acc_ev_compute_construct_end

    // OUT-PAR:acc_ev_compute_construct_start
    // OUT-OFF:acc_ev_enter_data_start
    // OUT-OFF:acc_ev_enter_data_end
    // OUT-PAR:acc_ev_enqueue_launch_start
    // OUT-PAR:acc_ev_enqueue_launch_end
    #pragma acc DIRECTIVE copy(x)
    x = 5;
    // OUT-OFF:acc_ev_exit_data_start
    // OUT-OFF:acc_ev_exit_data_end
    // OUT-PAR:acc_ev_compute_construct_end

    // OUT-PAR:acc_ev_compute_construct_start
    // OUT-OFF:acc_ev_enter_data_start
    // OUT-OFF:acc_ev_enter_data_end
    // OUT-PAR:acc_ev_enqueue_launch_start
    // OUT-PAR:acc_ev_enqueue_launch_end
    #pragma acc DIRECTIVE copyin(x)
    x = 5;
    // OUT-OFF:acc_ev_exit_data_start
    // OUT-OFF:acc_ev_exit_data_end
    // OUT-PAR:acc_ev_compute_construct_end

    // OUT-PAR:acc_ev_compute_construct_start
    // OUT-OFF:acc_ev_enter_data_start
    // OUT-OFF:acc_ev_enter_data_end
    // OUT-PAR:acc_ev_enqueue_launch_start
    // OUT-PAR:acc_ev_enqueue_launch_end
    #pragma acc DIRECTIVE copyout(x)
    x = 5;
    // OUT-OFF:acc_ev_exit_data_start
    // OUT-OFF:acc_ev_exit_data_end
    // OUT-PAR:acc_ev_compute_construct_end

    // OUT-PAR:acc_ev_compute_construct_start
    // OUT-OFF:acc_ev_enter_data_start
    // OUT-OFF:acc_ev_enter_data_end
    // OUT-PAR:acc_ev_enqueue_launch_start
    // OUT-PAR:acc_ev_enqueue_launch_end
    #pragma acc DIRECTIVE create(x)
    x = 5;
    // OUT-OFF:acc_ev_exit_data_start
    // OUT-OFF:acc_ev_exit_data_end
    // OUT-PAR:acc_ev_compute_construct_end

    // OUT-PAR:acc_ev_compute_construct_start
    // OUT-OFF:acc_ev_enter_data_start
    // OUT-OFF:acc_ev_enter_data_end
    // OUT-PAR:acc_ev_enqueue_launch_start
    // OUT-PAR:acc_ev_enqueue_launch_end
    #pragma acc DIRECTIVE no_create(x)
    x = 5;
    // OUT-OFF:acc_ev_exit_data_start
    // OUT-OFF:acc_ev_exit_data_end
    // OUT-PAR:acc_ev_compute_construct_end
  }
  // OUT-OFF:acc_ev_exit_data_start
  // OUT-OFF:acc_ev_enqueue_download_start
  // OUT-OFF:acc_ev_enqueue_download_end
  // OUT-OFF:acc_ev_delete
  // OUT-OFF:acc_ev_free
  // OUT-OFF:acc_ev_exit_data_end

  // OUT-OFF:acc_ev_device_shutdown_start
  // OUT-OFF:acc_ev_device_shutdown_end
  //
  //      OUT-OFF:acc_ev_runtime_shutdown
  // OUT-HOST-PAR:acc_ev_runtime_shutdown

  return 0;
}
