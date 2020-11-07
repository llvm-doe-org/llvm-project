/*
 * acc2omp-proxy-internal.h -- Declarations used by libacc2omp internally to
 * interact with acc2omp-proxy implementations but not used by acc2omp-proxy
 * implementations.
 */

//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef ACC2OMP_PROXY_INTERNAL_H
#define ACC2OMP_PROXY_INTERNAL_H

#include "acc2omp-config.h"
#include "acc2omp-proxy.h"

/// Build an \c acc2omp_msg_t from an \c acc2omp_msgid_t.  For brevity, callers
/// should use \c ACC2OMP_MSG instead of \c acc2omp_msg.
acc2omp_msg_t acc2omp_msg(acc2omp_msgid_t MsgId);
#define ACC2OMP_MSG(MsgId) acc2omp_msg(acc2omp_msg_##MsgId)

/// Wrappers around acc2omp-proxy's \c acc2omp_assert that expand to nothing
/// when assertions are disabled in libacc2omp.
#ifdef ACC2OMP_USE_ASSERT
# define ACC2OMP_ASSERT(Cond, Msg)                                             \
  acc2omp_assert((Cond), (Msg), __FILE__, __LINE__);
# define ACC2OMP_UNREACHABLE(Msg) ACC2OMP_ASSERT(0, Msg)
#else
# define ACC2OMP_ASSERT(Cond, Msg)
# define ACC2OMP_UNREACHABLE(Msg)
#endif

#endif // ACC2OMP_PROXY_INTERNAL_H
