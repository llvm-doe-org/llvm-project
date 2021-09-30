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
// RUN:   (opt=-O1)
// RUN:   (opt=-O2)
// RUN:   (opt=-O3)
// RUN:   (opt=-Ofast)
// RUN: }
// RUN: %data tgts {
// RUN:   (run-if=                tgt-cflags='                                     -Xclang -verify')
// RUN:   (run-if=%run-if-x86_64  tgt-cflags='-fopenmp-targets=%run-x86_64-triple  -Xclang -verify')
// RUN:   (run-if=%run-if-ppc64le tgt-cflags='-fopenmp-targets=%run-ppc64le-triple -Xclang -verify')
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags='-fopenmp-targets=%run-nvptx64-triple -Xclang -verify=nvptx64')
// RUN:   (run-if=%run-if-amdgcn  tgt-cflags='-fopenmp-targets=%run-amdgcn-triple  -Xclang -verify')
// RUN: }
// RUN: %data run-envs {
// RUN:   (run-env=                                 )
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled')
// RUN:   (run-env='env ACC_DEVICE_TYPE=host'       )
// RUN: }
// RUN: %for opts {
// RUN:   %for tgts {
// RUN:     %[run-if] %clang -fopenacc %acc-includes %[tgt-cflags] %[opt] %s \
// RUN:                      -o %t
// RUN:     %for run-envs {
// RUN:       %[run-if] %[run-env] %t > %t.out 2>&1
// RUN:       %[run-if] FileCheck -match-full-lines -input-file %t.out %s
// RUN:     }
// RUN:   }
// RUN: }
//
// END.

// expected-no-diagnostics

// FIXME: Clang produces spurious warning diagnostics for nvptx64 offload.  This
// issue is not limited to Clacc and is present upstream:
// nvptx64-warning@*:* 0+ {{Linking two modules of different data layouts}}
// nvptx64-warning@*:* 0+ {{Linking two modules of different target triples}}

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
