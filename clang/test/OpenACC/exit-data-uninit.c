// Check the case where "acc exit data" occurs before runtime initialization.

// RUN: %data tgts {
// RUN:   (run-if=                tgt-cflags=                                    )
// RUN:   (run-if=%run-if-x86_64  tgt-cflags=-fopenmp-targets=%run-x86_64-triple )
// RUN:   (run-if=%run-if-ppc64le tgt-cflags=-fopenmp-targets=%run-ppc64le-triple)
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags=-fopenmp-targets=%run-nvptx64-triple)
// RUN: }
// RUN: %data dirs {
// RUN:   (dir=enter clause=copyin )
// RUN:   (dir=enter clause=create )
// RUN:   (dir=exit  clause=copyout)
// RUN:   (dir=exit  clause=delete )
// RUN: }
// RUN: %for tgts {
// RUN:   %for dirs {
// RUN:     %[run-if] %clang -Xclang -verify -fopenacc %[tgt-cflags] \
// RUN:                      -DDIR=%[dir] -DCLAUSE=%[clause] %s -o %t
// RUN:     %[run-if] %t > %t.out 2>&1
// RUN:     %[run-if] FileCheck -input-file %t.out -allow-empty %s \
// RUN:                         -check-prefixes=EXE
// RUN:   }
// RUN: }

// END.

// expected-no-diagnostics

#include <stdio.h>

int main() {
  int i;
  // EXE-NOT: {{.}}
  #pragma acc DIR data CLAUSE(i)
  return 0;
}
