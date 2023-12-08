/*
 * parallel-for-antidep-dynamic.c -- Archer testcase
 */
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
//
// See tools/archer/LICENSE.txt for details.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// RUN: %libarcher-compile && env ARCHER_OPTIONS=dispatch_fibers=1 %libarcher-run-race | FileCheck %s
// RUN: %libarcher-compile && env ARCHER_OPTIONS=dispatch_fibers=1:ignore_serial=1 %libarcher-run-race | FileCheck %s
// REQUIRES: tsan
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define LEN 6

int main(int argc, char *argv[]) {
  int indeces[LEN] = {1, 3, 5, 7, 9, 13};
  double base[26];
  double *p = base;
  double *q = p + 12;
  int i;

  for (i = 1; i < 26; ++i)
    base[i] = 0.5 * i;

#pragma omp parallel for num_threads(2) schedule(dynamic, 1)
  for (i = 0; i < LEN; ++i) {
    int idx = indeces[i];
    p[idx] += 1.0 + i;
    q[idx] += 3.0 + i;
  }

  fprintf(stderr, "DONE.\n");
  return 0;
}

// CHECK: WARNING: ThreadSanitizer: data race
// CHECK:   Write of size 8
// CHECK-NEXT: #0 {{.*}}parallel-for-antidep-dynamic.c:3{{4|5}}
// CHECK:   Previous write of size 8
// CHECK-NEXT: #0 {{.*}}parallel-for-antidep-dynamic.c:3{{4|5}}
// CHECK: DONE
// CHECK: ThreadSanitizer: reported {{[0-9]+}} warnings
