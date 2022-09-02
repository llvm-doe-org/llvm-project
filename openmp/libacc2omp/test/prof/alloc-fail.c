// Check that no callbacks are missed upon a device allocation failure.

// RUN: %clang-acc %s -o %t.exe

// DEFINE: %{check}( RUN_ENV %, HOST_OR_OFF_POST_ENV %,                        \
// DEFINE:           NOT_IF_OFF_POST_ENV %) =                                  \
// DEFINE:   %{NOT_IF_OFF_POST_ENV} %{RUN_ENV} %t.exe > %t.out 2> %t.err &&    \
// DEFINE:   FileCheck -input-file %t.err -allow-empty %s                      \
// DEFINE:     -check-prefixes=ERR,ERR-%{HOST_OR_OFF_POST_ENV}-POST-ENV &&     \
// DEFINE:   FileCheck -input-file %t.out %s                                   \
// DEFINE:     -match-full-lines -strict-whitespace -allow-empty               \
// DEFINE:     -implicit-check-not=acc_ev_                                     \
// DEFINE:     -check-prefixes=OUT,OUT-%if-host<HOST|OFF>-PRE-ENV              \
// DEFINE:     -check-prefixes=OUT-%{HOST_OR_OFF_POST_ENV}-POST-ENV            \
// DEFINE:     -check-prefixes=OUT-%if-host<HOST|OFF>-THEN-%{HOST_OR_OFF_POST_ENV}

// RUN: %{check}(                                 %, %if-host<HOST|OFF> %, %if-host<|%not --crash> %)
// RUN: %{check}( env OMP_TARGET_OFFLOAD=disabled %, HOST               %,                         %)
// RUN: %{check}( env ACC_DEVICE_TYPE=host        %, HOST               %,                         %)
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
//                       # FIXME: getTargetPointer is meaningless to users.
// ERR-OFF-POST-ENV-NEXT:Libomptarget error: Call to getTargetPointer returned null pointer (device failure or illegal mapping).
// ERR-OFF-POST-ENV-NEXT:Libomptarget error: Consult {{.*}}
// ERR-OFF-POST-ENV-NEXT:Libomptarget fatal error 1: failure of target construct while offloading is mandatory
//                       # An abort message usually follows.
//  ERR-OFF-POST-ENV-NOT:Libomptarget
