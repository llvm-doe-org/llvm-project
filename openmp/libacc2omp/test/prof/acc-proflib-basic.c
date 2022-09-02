// Check basic ACC_PROFLIB functionality and diagnostics.
//
// Whether the target is host or device affects at least when ompt_start_tool
// and thus acc_register_library executes, so it seems worthwhile to make sure
// ACC_PROFLIB works in all offload cases.

// RUN: %clang-lib -DPROG=PROG_PROFLIB_BAD  -o %t.proflib-bad  %s
// RUN: %clang-lib -DPROG=PROG_PROFLIB_GOOD -o %t.proflib-good %s
// RUN: %clang-acc -DPROG=PROG_APP -DTGT_USE_STDIO=%if-tgt-stdio<1|0> %s \
// RUN:   -o %t.exe
//
//      # Check diagnostic if library doesn't exist.
// RUN: rm -f %t.non-existent
// RUN: env ACC_PROFLIB=%t.non-existent %not --crash %t.exe > %t.out 2> %t.err
// RUN: FileCheck -allow-empty -check-prefix=EMPTY %s < %t.out
// RUN: FileCheck -input-file %t.err -DFILE='%t.non-existent' %s \
// RUN:   -check-prefixes=ERR
//
//      # Check diagnostic if library has no acc_register_library.
// RUN: env ACC_PROFLIB=%t.proflib-bad %not --crash %t.exe > %t.out 2> %t.err
// RUN: FileCheck -allow-empty -check-prefix=EMPTY %s < %t.out
// RUN: FileCheck -input-file %t.err -DFILE='%t.proflib-bad' %s \
// RUN:   -check-prefixes=ERR,ERR-SYM
//
//      # Check for success otherwise.
// RUN: env ACC_PROFLIB=%t.proflib-good %t.exe > %t.out 2> %t.err
// RUN: FileCheck -allow-empty -check-prefix=EMPTY %s < %t.err
// RUN: FileCheck -input-file %t.out -match-full-lines %s \
// RUN:   -implicit-check-not=acc_ev_ \
// RUN:   -check-prefixes=CHECK,%if-host<HOST|OFF> \
// RUN:   -check-prefixes=%if-tgt-stdio<TGT-USE-STDIO|NO-TGT-USE-STDIO>
//
// END.

// expected-no-diagnostics

// EMPTY-NOT:{{.}}

// Hopefully all dlopen and dlsym implementations give descriptive error
// messages that include the library name and the symbol name if it's undefined.
//         ERR: OMP: Error #[[#]]: failure using library from ACC_PROFLIB:
//     ERR-DAG: [[FILE]]
// ERR-SYM-DAG: acc_register_library

#define PROG_APP          0
#define PROG_PROFLIB_BAD  1
#define PROG_PROFLIB_GOOD 2

#if PROG == PROG_APP

#include <stdio.h>

int main() {
  //   OFF: acc_ev_device_init_start
  //   OFF: acc_ev_device_init_end
  // CHECK: acc_ev_compute_construct_start
  // CHECK: acc_ev_enqueue_launch_start
  // CHECK: acc_ev_enqueue_launch_end
  #pragma acc parallel num_gangs(1)
  // After an ACC_PROFLIB error, the kernel should not execute, and neither
  // should host code after the kernel, and so stdout should remain empty.
  // TGT-USE-STDIO: in kernel
#if TGT_USE_STDIO
  printf("in kernel\n")
#endif
  //     CHECK: acc_ev_compute_construct_end
  ;
  //     CHECK: after kernel
  printf("after kernel\n");
  //       OFF: acc_ev_device_shutdown_start
  //       OFF: acc_ev_device_shutdown_end
  //     CHECK: acc_ev_runtime_shutdown
  return 0;
}

#elif PROG == PROG_PROFLIB_BAD

void has_no_acc_register_library() {}

#elif PROG == PROG_PROFLIB_GOOD

#include "callbacks.h"
void acc_register_library(acc_prof_reg reg, acc_prof_reg unreg,
                          acc_prof_lookup lookup) {
  register_all_callbacks(reg);
}

#else
# error "unknown PROG"
#endif
