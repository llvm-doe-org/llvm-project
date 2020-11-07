// Check that the acc2omp-proxy implementation controls fatal error handling.

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
// First, make sure it's fine when no error is expected.
// RUN: %preload-t.so %t > %t.out 2> %t.err
// RUN: FileCheck -input-file %t.err -allow-empty -check-prefixes=EMPTY %s
// RUN: FileCheck -input-file %t.out -check-prefixes=OUT %s
//
// Second, check an acc2omp_fatal call.
// RUN: env ACC_PROFLIB=%t.non-existent %preload-t.so %not %t > %t.out 2> %t.err
// RUN: FileCheck -input-file %t.err -check-prefixes=ERR \
// RUN:           -DFILE='%t.non-existent' %s
// RUN: FileCheck -input-file %t.out -allow-empty -check-prefixes=EMPTY %s

// END.

// expected-no-diagnostics

// OUT-NOT: {{.}}
//     OUT: hello world
// OUT-NOT: {{.}}

// EMPTY-NOT: {{.}}

// Check that the message contains:
// - The acc2omp-proxy-test prefix.
// - The default message for acc2omp_msg_acc_proflib_fail.
// - The file name, which demonstrates that the message received its argument.
//   (If any dlopen implementation doesn't include the file name in the
//   diagnostic produced by dlerror, this test will unintentionally fail for
//   it, but hopefully no such implementation exists.)
//
// ERR-NOT: {{.}}
//     ERR: {{^}}acc2omp-proxy-test: fatal error: failure using library from ACC_PROFLIB:{{.*}}[[FILE]]{{.*$}}
// ERR-NOT: {{.}}

#include <stdio.h>
int main() {
  // Trigger runtime startup so that ACC_PROFLIB is read.
  #pragma acc parallel num_gangs(1)
  printf("hello world\n");
  return 0;
}
