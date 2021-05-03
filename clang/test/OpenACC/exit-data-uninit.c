// Check the case where "acc exit data" occurs before runtime initialization.

// RUN: %data tgts {
// RUN:   (run-if=                tgt-cflags=                                     not-crash-if-dev=              not-if-dev=   )
// RUN:   (run-if=%run-if-x86_64  tgt-cflags=-fopenmp-targets=%run-x86_64-triple  not-crash-if-dev='not --crash' not-if-dev=not)
// RUN:   (run-if=%run-if-ppc64le tgt-cflags=-fopenmp-targets=%run-ppc64le-triple not-crash-if-dev='not --crash' not-if-dev=not)
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags=-fopenmp-targets=%run-nvptx64-triple not-crash-if-dev='not --crash' not-if-dev=not)
// RUN: }
// RUN: %data dirs {
// RUN:   (dir=enter clause=copyin  not-crash=                    not=             )
// RUN:   (dir=enter clause=create  not-crash=                    not=             )
// RUN:   (dir=exit  clause=copyout not-crash=%[not-crash-if-dev] not=%[not-if-dev])
// RUN:   (dir=exit  clause=delete  not-crash=%[not-crash-if-dev] not=%[not-if-dev])
// RUN: }
// RUN: %for tgts {
// RUN:   %for dirs {
// RUN:     %[run-if] %clang -Xclang -verify -fopenacc %[tgt-cflags] \
// RUN:                      -DDIR=%[dir] -DCLAUSE=%[clause] %s -o %t
// RUN:     %[run-if] %[not-crash] %t > %t.out 2>&1
// RUN:     %[run-if] FileCheck -input-file %t.out -allow-empty %s \
// RUN:                         -check-prefixes=EXE,EXE-%[not]PASS
// RUN:   }
// RUN: }

// END.

// expected-no-diagnostics

#include <stdio.h>

int main() {
  int i;
  //          EXE-NOT: {{.}}
  //      EXE-notPASS: Libomptarget error: run with env LIBOMPTARGET_INFO>1 to dump host-target pointer maps
  // EXE-notPASS-NEXT: Libomptarget fatal error 1: failure of target construct while offloading is mandatory
  //                   # An abort message usually follows.
  //  EXE-notPASS-NOT: Libomptarget
  #pragma acc DIR data CLAUSE(i)
  return 0;
}
