// Check that optimizations don't break basic OpenACC functionality.
//
// For example, building the OpenMP runtime with Clang is required to generate
// libomptarget-*.bc libraries.  Without these libraries, supposedly the worst
// that can happen is degraded performance.  However, at the time this test was
// written, we also saw consistent failures at run time after compiling a simple
// empty "acc parallel" construct at -O2 or above for offloading to nvptx64
// (from x86_64 or ppc64le).  Building these libraries fixed the failures.

// DEFINE: %{check-run-env}( RUN_ENV %) =                                      \
// DEFINE:   : '---------- RUN_ENV: %{RUN_ENV} ----------'                  && \
// DEFINE:   %{RUN_ENV} %t.exe > %t.out 2>&1                                && \
// DEFINE:   FileCheck -match-full-lines -input-file=%t.out %s

// DEFINE: %{check-run-envs} =                                                 \
// DEFINE:   %{check-run-env}(                                 %)           && \
// DEFINE:   %{check-run-env}( env OMP_TARGET_OFFLOAD=disabled %)           && \
// DEFINE:   %{check-run-env}( env ACC_DEVICE_TYPE=host        %)

// DEFINE: %{check-opt}( OPT %) =                                              \
// DEFINE:   %clang-acc %{OPT} %s -o %t.exe                                 && \
// DEFINE:   %{check-run-envs}

// RUN: %{check-opt}( -O0    %)
// RUN: %{check-opt}( -O1    %)
// RUN: %{check-opt}( -O2    %)
// RUN: %{check-opt}( -O3    %)
// RUN: %{check-opt}( -Ofast %)
//
// END.

// expected-no-diagnostics

#include <openacc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// CHECK-NOT: {{.}}
int main() {
  int i = 0;
  // CHECK: before: 0
  printf("before: %d\n", i);
  #pragma acc parallel copy(i)
  i = 5;
  // CHECK-NEXT: after: 5
  printf("after: %d\n", i);
  return 0;
}
// CHECK-NOT: {{.}}
