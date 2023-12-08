/*
 * parallel-for-antidep.c -- Archer testcase
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

#include <stdio.h>

int main(int argc,char *argv[]) {
  int i, j, len = 20; 
  double a[len][len];

  for (i = 0; i < len; ++i)
    for (j = 0; j < len; ++j)
      a[i][j] = i * j; 

#pragma omp parallel for private(j)
  for (i = 0; i < len - 1; ++i) {
    for (j = 0; j < len ; ++j) {
      a[i][j] += a[i + 1][j];
    }
  }

  fprintf(stderr, "DONE.\n");
  return 0;
}

// CHECK: WARNING: ThreadSanitizer: data race
// CHECK:   {{(Write|Read)}} of size 8
// CHECK-NEXT: #0 {{.*}}parallel-for-antidep.c:30
// CHECK:   Previous {{write|read}} of size 8
// CHECK-NEXT: #0 {{.*}}parallel-for-antidep.c:30
// CHECK: DONE
// CHECK: ThreadSanitizer: reported {{[0-9]+}} warnings
