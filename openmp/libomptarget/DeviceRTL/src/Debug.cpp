//===--- Debug.cpp -------- Debug utilities ----------------------- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains debug utilities
//
//===----------------------------------------------------------------------===//

#include "Debug.h"
#include "Configuration.h"
#include "Interface.h"
#include "Mapping.h"
#include "Types.h"

using namespace _OMP;

#pragma omp declare target

extern "C" {
void __assert_assume(bool condition) { __builtin_assume(condition); }

void __assert_fail(const char *assertion, const char *file, unsigned line,
                   const char *function) {
  PRINTF("%s:%u: %s: Assertion `%s' failed.\n", file, line, function,
         assertion);
  __builtin_trap();
}
}

/// Current indentation level for the function trace. Only accessed by thread 0.
static uint32_t Level = 0;
// FIXME: Suppressing this directive for amdgcn suppresses an amdgcn-link error
// when compiling the application:
//
//   <unknown>:0: error: _ZL5Level: unsupported initializer for address space
//   clang-14: error: amdgcn-link command failed with exit code 1 (use -v to see invocation)
//
// We're not sure what impact losing this directive has, but amdgcn doesn't
// currently support printing to stdio anyway, so it probably doesn't matter
// too much.  This workaround does not appear upstream.  We're hoping AMD will
// determine an appropriate fix when the new device runtime becomes the default.
#ifndef __AMDGCN__
#pragma omp allocate(Level) allocator(omp_pteam_mem_alloc)
#endif

DebugEntryRAII::DebugEntryRAII(const char *File, const unsigned Line,
                               const char *Function) {
  if (config::isDebugMode(config::DebugKind::FunctionTracing) &&
      mapping::getThreadIdInBlock() == 0 && mapping::getBlockId() == 0) {

    for (int I = 0; I < Level; ++I)
      PRINTF("%s", "  ");

    PRINTF("%s:%u: Thread %u Entering %s\n", File, Line,
           mapping::getThreadIdInBlock(), Function);
    Level++;
  }
}

DebugEntryRAII::~DebugEntryRAII() {
  if (config::isDebugMode(config::DebugKind::FunctionTracing) &&
      mapping::getThreadIdInBlock() == 0 && mapping::getBlockId() == 0)
    Level--;
}

#pragma omp end declare target
