/*
 * acc2omp-config.h -- Feature macros
 */
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef ACC2OMP_CONFIG_H
#define ACC2OMP_CONFIG_H

#cmakedefine01 LLVM_ENABLE_ASSERTIONS
#define ACC2OMP_USE_ASSERT LLVM_ENABLE_ASSERTIONS

#endif // ACC2OMP_CONFIG_H
