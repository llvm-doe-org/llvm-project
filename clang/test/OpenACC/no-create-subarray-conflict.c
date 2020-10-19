// Check what happens in the strange case that no_create and a data clause on
// a statically enclosing acc data specify different parts of the same array.
// OpenACC 3.0-next seems to permit this because no_create then just doesn't
// allocate.  Clacc also permits this, but only when using the no_alloc
// translation of no_create.  When using the alloc translation, OpenMP support
// in the Clang frontend complains... but only in the case that no_create
// specifies the full array.

// RUN: %clang -Xclang -verify=no-no_alloc -fsyntax-only -fopenacc \
// RUN:        -fopenacc-no-create-omp=no-no_alloc -o %t.exe %s

// RUN: %data tgts {
// RUN:   (run-if=                cflags=                                    )
// RUN:   (run-if=%run-if-x86_64  cflags=-fopenmp-targets=%run-x86_64-triple )
// RUN:   (run-if=%run-if-ppc64le cflags=-fopenmp-targets=%run-ppc64le-triple)
// RUN:   (run-if=%run-if-nvptx64 cflags=-fopenmp-targets=%run-nvptx64-triple)
// RUN: }
// RUN: %for tgts {
// RUN:   %[run-if] %clang -Xclang -verify=no_alloc -fopenacc %[cflags] \
// RUN:                    -o %t.exe %s
// RUN:   %[run-if] %t.exe > %t.out 2>&1
// RUN:   %[run-if] FileCheck -input-file %t.out -check-prefixes=EXE %s
// RUN: }

// END.

// no_alloc-no-diagnostics

#include <stdio.h>

// EXE-NOT: {{.}}
int main() {
  int arr[] = {10, 20, 30, 40, 50};
  // no-no_alloc-note@+1 {{used here}}
  #pragma acc data copy(arr[1:2])
  // no-no_alloc-error@+1 {{original storage of expression in data environment is shared but data environment do not fully contain mapped expression storage}}
  #pragma acc parallel num_gangs(1) no_create(arr)
  arr[1] += 100;
  // EXE: 120
  printf("%d\n", arr[1]);
  return 0;
}
// EXE-NOT: {{.}}
