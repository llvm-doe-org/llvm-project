/*
 * api.cpp -- OpenACC Runtime Library API implementation.
 */

//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

/*****************************************************************************
 * System include files.
 ****************************************************************************/

// Don't include headers like assert.h, stdio.h, or iostream.  Instead, call
// functions declared in acc2omp-backend.h.
//#include <cstring>

/*****************************************************************************
 * OpenMP standard include files.
 ****************************************************************************/

#include "omp.h"

/*****************************************************************************
 * LLVM OpenMP include files.
 *
 * One of our goals is to be able to wrap any OpenMP runtime implementing OMPT.
 * For that reason, it's expected that anything included here doesn't actually
 * require linking with LLVM's OpenMP runtime.  Only definitions in the header
 * are needed.  If you do need to link something from the OpenMP runtime, see
 * acc2omp-backend.h.
 ****************************************************************************/

// FIXME: Will this be needed?
// - KMP_FALLTHROUGH
//#include "kmp_os.h"

/*****************************************************************************
 * OpenACC standard include files.
 ****************************************************************************/

#include "openacc.h"

/*****************************************************************************
 * Internal includes.
 ****************************************************************************/

#include "internal.h"

/*****************************************************************************
 * Data and memory management routines.
 *
 * TODO: We assume !omp_get_num_devices() is a good test for disabled
 * offloading.
 ****************************************************************************/

int acc_is_present(void *data_arg, size_t bytes) {
  // OpenACC 3.0, sec. 3.2.36 "acc_is_present", L3064-3065:
  // "The function returns .true. if the specified data is in shared memory or
  // is fully present, and .false. otherwise."
  //
  // If offloading is disabled, that means shared memory, so it's present.
  //
  // The spec doesn't say what should happen for a null pointer, but pgcc
  // 19.10-0 always returns true in that case (but, strangely, only if the
  // OpenACC runtime has started).  Moreover, the OpenACC technical committee
  // discussions suggest it should be true.
  if (!omp_get_num_devices() || !data_arg)
    return true;
  return omp_present_full ==
         omp_target_range_is_present(data_arg, bytes, omp_get_default_device());
}
