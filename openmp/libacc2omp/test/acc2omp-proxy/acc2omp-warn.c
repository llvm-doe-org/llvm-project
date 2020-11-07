// Check that the acc2omp-proxy implementation controls reporting of warnings.

// Build acc2omp-proxy-test.
// RUN: %clang -Xclang -verify %acc-includes -shared -fPIC \
// RUN:        %S/Inputs/acc2omp-proxy-test.c -o %t.so

// Build the OpenACC application in source-to-source mode followed by OpenMP
// compilation.  We use our Clang as the OpenMP compiler because it's the only
// one we have.  Otherwise, this is expected to be the normal case for alternate
// OpenMP runtimes and thus alternate acc2omp-proxy implementations.
// RUN: %clang -Xclang -verify -fopenacc-print=omp %acc-includes %s > %t-omp.c
// RUN: %clang -Xclang -verify -fopenmp %fopenmp-version %acc-includes \
// RUN:        %t-omp.c %acc-libs -o %t

// Run the compiled application.  Force it to use acc2omp-proxy-test instead of
// acc2omp-proxy-llvm, but use LLVM's OpenMP runtime because it's the only one
// we have.
//
// RUN: %preload-t.so %t > %t.out 2> %t.err
// RUN: FileCheck -input-file %t.err -check-prefixes=ERR -strict-whitespace \
// RUN:           -match-full-lines %s
// RUN: FileCheck -input-file %t.out -check-prefixes=OUT -strict-whitespace \
// RUN:           -match-full-lines %s

// END.

// expected-no-diagnostics

//  OUT-NOT:{{.}}
//      OUT:before kernel
// OUT-NEXT:in kernel
// OUT-NEXT:after kernel
//  OUT-NOT:{{.}}

// Check entire message to be sure acc2omp-proxy-test is printing it and that
// all components of the message are passed correctly.
//
// ERR-NOT:{{.}}
//     ERR:acc2omp-proxy-test: warning: attempt to unregister event not previously registered: acc_ev_device_init_start
// ERR-NOT:{{.}}

#include <acc_prof.h>
#include <stdio.h>

static void cb(acc_prof_info *pi, acc_event_info *ei, acc_api_info *ai) {}

void acc_register_library(acc_prof_reg reg, acc_prof_reg unreg,
                          acc_prof_lookup lookup) {
  unreg(acc_ev_device_init_start, cb, acc_reg);
}

int main() {
  printf("before kernel\n");
  // Trigger runtime startup so that acc_register_library is called.
  #pragma acc parallel num_gangs(1)
  printf("in kernel\n");
  printf("after kernel\n");
  return 0;
}
