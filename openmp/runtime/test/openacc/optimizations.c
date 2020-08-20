// Check that optimizations don't break basic OpenACC functionality.
//
// For example, building the OpenMP runtime with Clang is required to generate
// libomptarget-*.bc libraries.  Without these libraries, supposedly the worst
// that can happen is degraded performance.  However, at the time this test was
// written, we also saw consistent failures at run time after compiling a simple
// empty "acc parallel" construct at -O2 or above for offloading to nvptx64
// (from x86_64 or ppc64le).  Building these libraries fixed the failures.

// RUN: %data opts {
// RUN:   (opt=-O0)
//        # Process seems to hang at -O1, so skip it for now.
// XUN:   (opt=-O1)
// RUN:   (opt=-O2)
// RUN:   (opt=-O3)
// RUN:   (opt=-Ofast)
// RUN: }
// RUN: %data tgts {
// RUN:   (run-if=                tgt=                                    )
// RUN:   (run-if=%run-if-x86_64  tgt=-fopenmp-targets=%run-x86_64-triple )
// RUN:   (run-if=%run-if-ppc64le tgt=-fopenmp-targets=%run-ppc64le-triple)
// RUN:   (run-if=%run-if-nvptx64 tgt=-fopenmp-targets=%run-nvptx64-triple)
// RUN: }
// RUN: %data run-envs {
// RUN:   (run-env=                                 )
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled')
// RUN: }
// RUN: %for opts {
// RUN:   %for tgts {
// RUN:     %[run-if] %clang -Xclang -verify -fopenacc %[tgt] %[opt] %s -o %t
// RUN:     %for run-envs {
// RUN:       %[run-if] %[run-env] %t > %t.out 2>&1
// RUN:       %[run-if] FileCheck -match-full-lines -input-file %t.out %s
// RUN:     }
// RUN:   }
// RUN: }
//
// END.

// expected-no-diagnostics

#include <stdio.h>

// CHECK-NOT: {{.}}
int main() {
  // CHECK: before
  printf("before\n");
  #pragma acc parallel
  // CHECK-NEXT: inside{{($[[:space:]]inside)*}}
  printf("inside\n");
  ;
  // CHECK-NEXT: after
  printf("after\n");
  return 0;
}
// CHECK-NOT: {{.}}
