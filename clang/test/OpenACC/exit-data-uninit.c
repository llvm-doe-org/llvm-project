// Check the case where "acc exit data" occurs before runtime initialization.

// RUN: %data tgts {
// RUN:   (run-if=                tgt-cflags='                                     -Xclang -verify'        )
// RUN:   (run-if=%run-if-x86_64  tgt-cflags='-fopenmp-targets=%run-x86_64-triple  -Xclang -verify'        )
// RUN:   (run-if=%run-if-ppc64le tgt-cflags='-fopenmp-targets=%run-ppc64le-triple -Xclang -verify'        )
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags='-fopenmp-targets=%run-nvptx64-triple -Xclang -verify=nvptx64')
// RUN:   (run-if=%run-if-amdgcn  tgt-cflags='-fopenmp-targets=%run-amdgcn-triple  -Xclang -verify'        )
// RUN: }
// RUN: %data dirs {
// RUN:   (dir=enter clause=copyin )
// RUN:   (dir=enter clause=create )
// RUN:   (dir=exit  clause=copyout)
// RUN:   (dir=exit  clause=delete )
// RUN: }
// RUN: %for tgts {
// RUN:   %for dirs {
// RUN:     %[run-if] %clang -fopenacc %[tgt-cflags] \
// RUN:                      -DDIR=%[dir] -DCLAUSE=%[clause] %s -o %t
// RUN:     %[run-if] %t > %t.out 2>&1
// RUN:     %[run-if] FileCheck -input-file %t.out -allow-empty %s \
// RUN:                         -check-prefixes=EXE
// RUN:   }
// RUN: }

// END.

// expected-no-diagnostics

// FIXME: Clang produces spurious warning diagnostics for nvptx64 offload.  This
// issue is not limited to Clacc and is present upstream:
// nvptx64-warning@*:* 0+ {{Linking two modules of different data layouts}}
// nvptx64-warning@*:* 0+ {{Linking two modules of different target triples}}

#include <stdio.h>

int main() {
  int i;
  // EXE-NOT: {{.}}
  #pragma acc DIR data CLAUSE(i)
  return 0;
}
