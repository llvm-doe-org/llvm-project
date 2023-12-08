/*
 * loop-level.c -- Archer testcase
 */
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
//
// See tools/archer/LICENSE.txt for details.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// RUN: %libarcher-compile && env ARCHER_OPTIONS=dispatch_fibers=1 %libarcher-run | FileCheck %s
// RUN: %libarcher-compile && env ARCHER_OPTIONS=dispatch_fibers=1:ignore_serial=1 %libarcher-run | FileCheck %s
// REQUIRES: tsan
// XFAIL: *
#include <omp.h>
#include <stdio.h>
#include <unistd.h>

#define NUM_THREADS 2

int main(int argc, char *argv[]) {
  int vars[NUM_THREADS] = {0};
  int i;
  const int len = 10;

#pragma omp parallel for num_threads(NUM_THREADS) shared(vars)                 \
    schedule(dynamic, 1)
  for (i = 0; i < len; i++) {
    vars[omp_get_thread_num()]++;
  }

  fprintf(stderr, "DONE\n");
  return 0;
}

// CHECK-NOT: ThreadSanitizer: data race
// CHECK-NOT: ThreadSanitizer: reported
// CHECK: DONE
