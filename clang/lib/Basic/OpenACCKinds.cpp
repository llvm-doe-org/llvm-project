//===--- OpenACCKinds.cpp - Token Kinds Support --------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// This file implements the OpenACC enum and support functions.
///
//===----------------------------------------------------------------------===//

#include "clang/Basic/OpenACCKinds.h"
#include "clang/Basic/IdentifierTable.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/ErrorHandling.h"
#include <cassert>

using namespace clang;

OpenACCDirectiveKind clang::getOpenACCDirectiveKind(StringRef Str) {
  return llvm::StringSwitch<OpenACCDirectiveKind>(Str)
#define OPENACC_DIRECTIVE(Name) .Case(#Name, ACCD_##Name)
#include "clang/Basic/OpenACCKinds.def"
      .Default(ACCD_unknown);
}

const char *clang::getOpenACCDirectiveName(OpenACCDirectiveKind Kind) {
  assert(Kind <= ACCD_unknown);
  switch (Kind) {
  case ACCD_unknown:
    return "unknown";
#define OPENACC_DIRECTIVE(Name)                                                \
  case ACCD_##Name:                                                            \
    return #Name;
#define OPENACC_DIRECTIVE_EXT(Name, Str)                                       \
  case ACCD_##Name:                                                            \
    return Str;
#include "clang/Basic/OpenACCKinds.def"
    break;
  }
  llvm_unreachable("Invalid OpenACC directive kind");
}

OpenACCClauseKind clang::getOpenACCClauseKind(StringRef Str) {
  return llvm::StringSwitch<OpenACCClauseKind>(Str)
#define OPENACC_CLAUSE(Name, Class) .Case(#Name, ACCC_##Name)
#include "clang/Basic/OpenACCKinds.def"
      .Default(ACCC_unknown);
}

const char *clang::getOpenACCClauseName(OpenACCClauseKind Kind) {
  assert(Kind <= ACCC_unknown);
  switch (Kind) {
  case ACCC_unknown:
    return "unknown";
#define OPENACC_CLAUSE(Name, Class)                                            \
  case ACCC_##Name:                                                            \
    return #Name;
#include "clang/Basic/OpenACCKinds.def"
  }
  llvm_unreachable("Invalid OpenACC clause kind");
}

bool clang::isAllowedClauseForDirective(OpenACCDirectiveKind DKind,
                                        OpenACCClauseKind CKind) {
  assert(DKind <= ACCD_unknown);
  assert(CKind <= ACCC_unknown);
  switch (DKind) {
  case ACCD_parallel:
    switch (CKind) {
#define OPENACC_PARALLEL_CLAUSE(Name)                                         \
    case ACCC_##Name:                                                         \
      return true;
#include "clang/Basic/OpenACCKinds.def"
    default:
      break;
    }
    break;
  case ACCD_loop:
    switch (CKind) {
#define OPENACC_LOOP_CLAUSE(Name)                                             \
  case ACCC_##Name:                                                           \
    return true;
#include "clang/Basic/OpenACCKinds.def"
    default:
      break;
    }
    break;
  case ACCD_unknown:
    break;
  }
  return false;
}

bool clang::isOpenACCLoopDirective(OpenACCDirectiveKind DKind) {
  return DKind == ACCD_loop;
}
