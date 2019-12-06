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
#define OPENACC_DIRECTIVE_EXT(Name, Str) .Case(Str, ACCD_##Name)
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
#define OPENACC_CLAUSE_ALIAS(ClauseAlias, AliasedClause, Class) \
    .Case(#ClauseAlias, ACCC_##ClauseAlias)
#include "clang/Basic/OpenACCKinds.def"
      .Default(ACCC_unknown);
}

const char *clang::getOpenACCBaseDAName(OpenACCBaseDAKind Kind) {
  assert(Kind <= ACC_BASE_DA_unknown);
  switch (Kind) {
  case ACC_BASE_DA_unknown:
    return "unknown";
#define OPENACC_BASE_DA(Name) \
  case ACC_BASE_DA_##Name:    \
    return #Name;
#include "clang/Basic/OpenACCKinds.def"
  }
  llvm_unreachable("Invalid OpenACC DA kind");
}

const char *clang::getOpenACCClauseName(OpenACCClauseKind Kind) {
  assert(Kind <= ACCC_unknown);
  switch (Kind) {
  case ACCC_unknown:
    return "unknown";
#define OPENACC_CLAUSE(Name, Class) \
  case ACCC_##Name:                 \
    return #Name;
#define OPENACC_CLAUSE_ALIAS(AliasedClause, ClauseAlias, Class) \
  case ACCC_##AliasedClause:                                    \
    return #AliasedClause;
#include "clang/Basic/OpenACCKinds.def"
  }
  llvm_unreachable("Invalid OpenACC clause kind");
}

bool clang::isAllowedBaseDAForReduction(OpenACCBaseDAKind BaseDAKind) {
  assert(BaseDAKind <= ACC_BASE_DA_unknown);
  switch (BaseDAKind) {
  case ACC_BASE_DA_unknown:
    return true;
#define OPENACC_REDUCTION_BASE_DA(Name) \
  case ACC_BASE_DA_##Name:              \
    return true;
#include "clang/Basic/OpenACCKinds.def"
  default:
    break;
  }
  return false;
}

bool clang::isAllowedBaseDAForDirective(OpenACCDirectiveKind DKind,
                                        OpenACCBaseDAKind BaseDAKind) {
  assert(BaseDAKind <= ACC_BASE_DA_unknown);
  switch (DKind) {
  case ACCD_parallel:
    switch (BaseDAKind) {
#define OPENACC_PARALLEL_BASE_DA(Name) \
    case ACC_BASE_DA_##Name:           \
      return true;
#include "clang/Basic/OpenACCKinds.def"
    default:
      break;
    }
    break;
  case ACCD_loop:
    switch (BaseDAKind) {
#define OPENACC_LOOP_BASE_DA(Name) \
  case ACC_BASE_DA_##Name:         \
    return true;
#include "clang/Basic/OpenACCKinds.def"
    default:
      break;
    }
    break;
  case ACCD_parallel_loop:
    switch (BaseDAKind) {
#define OPENACC_PARALLEL_LOOP_BASE_DA(Name) \
  case ACC_BASE_DA_##Name:                  \
    return true;
#include "clang/Basic/OpenACCKinds.def"
    default:
      break;
    }
    break;
  case ACCD_unknown:
    llvm_unreachable("unexpected unknown directive");
  }
  return false;
}

bool clang::isAllowedClauseForDirective(OpenACCDirectiveKind DKind,
                                        OpenACCClauseKind CKind) {
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
  case ACCD_parallel_loop:
    switch (CKind) {
#define OPENACC_PARALLEL_LOOP_CLAUSE(Name)                                    \
  case ACCC_##Name:                                                           \
    return true;
#include "clang/Basic/OpenACCKinds.def"
    default:
      break;
    }
    break;
  case ACCD_unknown:
    llvm_unreachable("unexpected unknown directive");
  }
  return false;
}

bool clang::isAllowedParentForDirective(OpenACCDirectiveKind DKind,
                                        OpenACCDirectiveKind ParentDKind) {
  switch (DKind) {
  case ACCD_parallel:
    switch (ParentDKind) {
#define OPENACC_PARALLEL_PARENT(Name)                                         \
    case ACCD_##Name:                                                         \
      return true;
#include "clang/Basic/OpenACCKinds.def"
    default:
      break;
    }
    break;
  case ACCD_loop:
    switch (ParentDKind) {
#define OPENACC_LOOP_PARENT(Name)                                             \
  case ACCD_##Name:                                                           \
    return true;
#include "clang/Basic/OpenACCKinds.def"
    default:
      break;
    }
    break;
  case ACCD_parallel_loop:
    switch (ParentDKind) {
#define OPENACC_PARALLEL_LOOP_PARENT(Name)                                    \
  case ACCD_##Name:                                                           \
    return true;
#include "clang/Basic/OpenACCKinds.def"
    default:
      break;
    }
    break;
  case ACCD_unknown:
    llvm_unreachable("unexpected unknown directive");
  }
  return false;
}

int clang::getOpenACCEffectiveDirectives(OpenACCDirectiveKind DKind) {
  switch (DKind) {
  case ACCD_parallel:
  case ACCD_loop:
    return 1;
  case ACCD_parallel_loop:
    return 2;
  case ACCD_unknown:
    llvm_unreachable("unexpected unknown directive");
  }
}

bool clang::isOpenACCParallelDirective(OpenACCDirectiveKind DKind) {
  return DKind == ACCD_parallel || DKind == ACCD_parallel_loop;
}

bool clang::isOpenACCLoopDirective(OpenACCDirectiveKind DKind) {
  return DKind == ACCD_loop || DKind == ACCD_parallel_loop;
}

bool clang::isOpenACCComputeDirective(OpenACCDirectiveKind DKind) {
  return isOpenACCParallelDirective(DKind);
}

bool clang::isOpenACCCombinedDirective(OpenACCDirectiveKind DKind) {
  return DKind == ACCD_parallel_loop;
}
