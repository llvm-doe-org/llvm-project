// Check that the libacc2omp backend controls reporting of warnings.

// Build libacc2omp-backend-test.
// RUN: %clang-lib %S/Inputs/acc2omp-backend-test.c -o %t.so

// Build the OpenACC application in source-to-source mode followed by OpenMP
// compilation.  We use our Clang as the OpenMP compiler because it's the only
// one we have.  Otherwise, this is expected to be the normal case for alternate
// OpenMP runtimes and thus alternate libacc2omp backends.
// RUN: %clang-acc-prt-omp %s > %t-omp.c
// RUN: %clang-omp %t-omp.c -o %t.exe

// Run the compiled application.  Force it to use libacc2omp-backend-test
// instead of LLVM's libacc2omp backend, but use LLVM's OpenMP runtime because
// it's the only one we have.
//
// RUN: %preload-t.so %t.exe > %t.out 2> %t.err
// RUN: FileCheck -input-file %t.err -check-prefixes=ERR -strict-whitespace \
// RUN:   -match-full-lines %s
// RUN: FileCheck -input-file %t.out -check-prefixes=OUT -strict-whitespace \
// RUN:   -match-full-lines %s

// END.

// expected-error 0 {{}}

// FIXME: Clang produces spurious warning diagnostics for nvptx64 offload.  This
// issue is not limited to Clacc and is present upstream:
// nvptx64-warning@*:* 0+ {{Linking two modules of different target triples}}

//  OUT-NOT:{{.}}
//      OUT:before kernel
// OUT-NEXT:kernelRan=1
// OUT-NEXT:after kernel
//  OUT-NOT:{{.}}

// Check entire message to be sure libacc2omp-backend-test is printing it and
// that all components of the message are passed correctly.
//
// ERR-NOT:{{.}}
//     ERR:libacc2omp-backend-test: warning: attempt to unregister event not previously registered: acc_ev_device_init_start
// ERR-NOT:{{.}}

#include <acc_prof.h>
#include <stdbool.h>
#include <stdio.h>

static void cb(acc_prof_info *pi, acc_event_info *ei, acc_api_info *ai) {}

void acc_register_library(acc_prof_reg reg, acc_prof_reg unreg,
                          acc_prof_lookup lookup) {
  unreg(acc_ev_device_init_start, cb, acc_reg);
}

int main() {
  printf("before kernel\n");
  // Trigger runtime startup so that acc_register_library is called.
  bool kernelRan = false;
  #pragma acc parallel num_gangs(1) copyout(kernelRan)
  kernelRan = true;
  printf("kernelRan=%d\n", kernelRan);
  printf("after kernel\n");
  return 0;
}
