/*
 * acc2omp-internal.h -- Declarations used across various libacc2omp internal
 * components.
 */

//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef ACC2OMP_INTERNAL_H
#define ACC2OMP_INTERNAL_H

#include "acc2omp-config.h"
#include "acc2omp-proxy-internal.h"

/// The value used for acc_prof_info's version field.
///
/// Keep this in sync with Clang's definition of \c _OPENACC and both test
/// suites' \c %acc-version LIT substitutions.
#define ACC2OMP_OPENACC_VERSION 202011

#endif // ACC2OMP_INTERNAL_H
