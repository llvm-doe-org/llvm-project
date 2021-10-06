// Check basic ACC_PROFLIB functionality and diagnostics.

// RUN: %data proflibs {
// RUN:   (cpp=-DPROG=PROG_PROFLIB_BAD  file=%t.proflib-bad )
// RUN:   (cpp=-DPROG=PROG_PROFLIB_GOOD file=%t.proflib-good)
// RUN: }
// RUN: %for proflibs {
// RUN:   %clang -Xclang -verify %acc-includes -shared -fpic %[cpp] %s \
// RUN:          -o %[file]
// RUN: }
// RUN: %data tgts {
//        Whether the target is host or device affects at least when
//        ompt_start_tool and thus acc_register_library executes, so it seems
//        worthwhile to make sure ACC_PROFLIB works in all cases.
// RUN:   (run-if=                tgt-cflags='                                     -Xclang -verify'                   fc=HOST,TGT-USE-STDIO  )
// RUN:   (run-if=%run-if-x86_64  tgt-cflags='-fopenmp-targets=%run-x86_64-triple  -Xclang -verify'                   fc=OFF,TGT-USE-STDIO   )
// RUN:   (run-if=%run-if-ppc64le tgt-cflags='-fopenmp-targets=%run-ppc64le-triple -Xclang -verify'                   fc=OFF,TGT-USE-STDIO   )
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags='-fopenmp-targets=%run-nvptx64-triple -Xclang -verify=nvptx64'           fc=OFF,TGT-USE-STDIO   )
//        # FIXME: Add back the target printf check once amdgcn supports it.
// RUN:   (run-if=%run-if-amdgcn  tgt-cflags='-fopenmp-targets=%run-amdgcn-triple  -Xclang -verify -DTGT_USE_STDIO=0' fc=OFF,NO-TGT-USE-STDIO)
// RUN: }
// RUN: %for tgts {
// RUN:   %[run-if] %clang -fopenacc %acc-includes \
// RUN:                    -DPROG=PROG_APP %s -o %t %[tgt-cflags]
//
//        # Check diagnostic if library doesn't exist.
// RUN:   %[run-if] rm -f %t.non-existent
// RUN:   %[run-if] env ACC_PROFLIB=%t.non-existent %not --crash %t > %t.out 2> %t.err
// RUN:   %[run-if] FileCheck -allow-empty -check-prefix=EMPTY %s < %t.out
// RUN:   %[run-if] FileCheck -input-file %t.err -DFILE='%t.non-existent' %s \
// RUN:                       -check-prefixes=ERR
//
//        # Check diagnostic if library has no acc_register_library.
// RUN:   %[run-if] env ACC_PROFLIB=%t.proflib-bad %not --crash %t > %t.out 2> %t.err
// RUN:   %[run-if] FileCheck -allow-empty -check-prefix=EMPTY %s < %t.out
// RUN:   %[run-if] FileCheck -input-file %t.err -DFILE='%t.proflib-bad' %s \
// RUN:                       -check-prefixes=ERR,ERR-SYM
//
//        # Check for success otherwise.
// RUN:   %[run-if] env ACC_PROFLIB=%t.proflib-good %t > %t.out 2> %t.err
// RUN:   %[run-if] FileCheck -allow-empty -check-prefix=EMPTY %s < %t.err
// RUN:   %[run-if] FileCheck -input-file %t.out -match-full-lines %s \
// RUN:       -implicit-check-not=acc_ev_ -check-prefixes=CHECK,%[fc]
// RUN: }
//
// END.

// expected-no-diagnostics

// FIXME: Clang produces spurious warning diagnostics for nvptx64 offload.  This
// issue is not limited to Clacc and is present upstream:
// nvptx64-warning@*:* 0+ {{Linking two modules of different data layouts}}
// nvptx64-warning@*:* 0+ {{Linking two modules of different target triples}}

// EMPTY-NOT:{{.}}

// Hopefully all dlopen and dlsym implementations give descriptive error
// messages that include the library name and the symbol name if it's undefined.
//         ERR: OMP: Error #[[#]]: failure using library from ACC_PROFLIB:
//     ERR-DAG: [[FILE]]
// ERR-SYM-DAG: acc_register_library

#define PROG_APP          0
#define PROG_PROFLIB_BAD  1
#define PROG_PROFLIB_GOOD 2

#ifndef TGT_USE_STDIO
# define TGT_USE_STDIO 1
#endif

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
