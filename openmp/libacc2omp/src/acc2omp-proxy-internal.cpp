/*
 * acc2omp-proxy-internal.cpp -- acc2omp-proxy-internal.h implementation.
 */

//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "acc2omp-proxy-internal.h"

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <dlfcn.h>

acc2omp_msg_t acc2omp_msg(acc2omp_msgid_t MsgId) {
  acc2omp_msg_t Msg = {MsgId, nullptr};
  // Keep this in a switch instead of an array indexed on acc2omp_msg_id_t to
  // help avoid mismatching IDs and formats.
  switch (MsgId) {
  case acc2omp_msg_alloc_fail:
    Msg.DefaultFmt = "memory allocation failed\n";
    break;
  case acc2omp_msg_acc_proflib_fail:
    Msg.DefaultFmt = "failure using library from ACC_PROFLIB: %s\n";
    break;
  }
  assert(Msg.DefaultFmt && "expected acc2omp_msg_t to be handled in switch");
  return Msg;
};
