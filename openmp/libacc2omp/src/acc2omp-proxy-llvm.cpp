/*
 * acc2omp-proxy-llvm.cpp -- acc2omp-proxy implementation for LLVM's OpenMP
 * runtime.  Builds as libacc2omp-proxy.so.
 */

//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "../../runtime/src/kmp.h"

#include <cstdarg>
#include <cstdlib>

#include "acc2omp-proxy.h"

static kmp_i18n_id_t acc2omp_msg_to_llvm(acc2omp_msgid_t MsgId) {
  switch (MsgId) {
  case acc2omp_msg_alloc_fail:
    return kmp_i18n_msg_MemoryAllocFailed;
  case acc2omp_msg_acc_proflib_fail:
    return kmp_i18n_msg_AccProflibFail;
  }
  KMP_ASSERT2(0, "unexpected acc2omp_msg_t");
  return kmp_i18n_null;
}

void acc2omp_fatal(acc2omp_msg_t Msg, ...) {
  va_list Args;
  va_start(Args, Msg);
  kmp_msg_t KmpMsg = __kmp_msg_vformat(acc2omp_msg_to_llvm(Msg.Id), Args);
  va_end(Args);
  __kmp_fatal(KmpMsg, __kmp_msg_null);
}

void acc2omp_assert(int Cond, const char *Msg, const char *File, int Line) {
  // The cmake variable LLVM_ENABLE_ASSERTIONS controls both whether libacc2omp
  // ever calls acc2omp_assert and also whether KMP_ASSERT4 is a no-op, so we
  // need not implement a fallback assertion mechanism here as discussed in
  // acc2omp_assert documentation in acc-proxy.h
  KMP_ASSERT4(Cond, Msg, File, Line);
}
