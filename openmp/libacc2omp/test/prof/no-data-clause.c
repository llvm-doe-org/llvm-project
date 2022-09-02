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

// RUN: %clang-acc %s -o %t.exe -DTGT_USE_STDIO=%if-tgt-stdio<1|0>
// RUN: %t.exe > %t.out 2>&1
// RUN: FileCheck -input-file %t.out %s \
// RUN:   -match-full-lines -strict-whitespace \
// RUN:   -implicit-check-not=acc_ev_ \
// RUN:   -check-prefixes=CHECK,%if-host<HOST|OFF> \
// RUN:   -check-prefixes=%if-tgt-stdio<TGT-USE-STDIO|NO-TGT-USE-STDIO>
//
// END.

// expected-no-diagnostics

#include "stdio.h"
#include "callbacks.h"

void acc_register_library(acc_prof_reg reg, acc_prof_reg unreg,
                          acc_prof_lookup lookup) {
  register_all_callbacks(reg);
}

//           OFF:acc_ev_device_init_start
//           OFF:acc_ev_device_init_end
//         CHECK:acc_ev_compute_construct_start
//         CHECK:acc_ev_enqueue_launch_start
//         CHECK:acc_ev_enqueue_launch_end
// TGT-USE-STDIO:hello world
//         CHECK:acc_ev_compute_construct_end
//           OFF:acc_ev_device_shutdown_start
//           OFF:acc_ev_device_shutdown_end
//         CHECK:acc_ev_runtime_shutdown
int main() {
  #pragma acc parallel
#if TGT_USE_STDIO
  printf("hello world\n")
#endif
  ;
  return 0;
}
