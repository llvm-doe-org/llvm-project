// Check that the libacc2omp backend controls fatal error handling.

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
// First, make sure it's fine when no error is expected.
// RUN: %preload-t.so %t.exe > %t.out 2> %t.err
// RUN: FileCheck -input-file %t.err -allow-empty -check-prefixes=EMPTY %s
// RUN: FileCheck -input-file %t.out -check-prefixes=OUT %s
//
// Second, check an acc2omp_fatal call.
// RUN: env ACC_PROFLIB=%t.non-existent %preload-t.so %not %t.exe \
// RUN:   > %t.out 2> %t.err
// RUN: FileCheck -input-file %t.err -check-prefixes=ERR \
// RUN:   -DFILE='%t.non-existent' %s
// RUN: FileCheck -input-file %t.out -allow-empty -check-prefixes=EMPTY %s

// END.

// expected-no-diagnostics

// OUT-NOT: {{.}}
//     OUT: kernelRan=1
// OUT-NOT: {{.}}

// EMPTY-NOT: {{.}}

// Check that the message contains:
// - The libacc2omp-backend-test prefix.
// - The default message for acc2omp_msg_acc_proflib_fail.
// - The file name, which demonstrates that the message received its argument.
//   (If any dlopen-like implementation doesn't include the file name in the
//   diagnostic produced by the corresponding dlerror-like implementation, this
//   test will unintentionally fail for it, but hopefully no such implementation
//   exists.)
//
// ERR-NOT: {{.}}
//     ERR: {{^}}libacc2omp-backend-test: fatal error: failure using library from ACC_PROFLIB:{{.*}}[[FILE]]{{.*$}}
// ERR-NOT: {{.}}

#include <stdbool.h>
#include <stdio.h>
int main() {
  // Trigger runtime startup so that ACC_PROFLIB is read.
  bool kernelRan = false;
  #pragma acc parallel num_gangs(1) copyout(kernelRan)
  kernelRan = true;
  printf("kernelRan=%d\n", kernelRan);
  return 0;
}
