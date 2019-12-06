//===--- OpenACCKinds.h - OpenACC enums -------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// Defines some OpenACC-specific enums and functions.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_BASIC_OPENACCKINDS_H
#define LLVM_CLANG_BASIC_OPENACCKINDS_H

#include "clang/Basic/OpenMPKinds.h"
#include "llvm/ADT/StringRef.h"

namespace clang {

/// OpenACC directives.
enum OpenACCDirectiveKind {
#define OPENACC_DIRECTIVE(Name) \
  ACCD_##Name,
#define OPENACC_DIRECTIVE_EXT(Name, Str) \
  ACCD_##Name,
#include "clang/Basic/OpenACCKinds.def"
  ACCD_unknown
};

/// OpenACC base data attributes.
enum OpenACCBaseDSAKind {
#define OPENACC_BASE_DSA(Name) \
  ACC_BASE_DSA_##Name,
#include "clang/Basic/OpenACCKinds.def"
  ACC_BASE_DSA_unknown
};

/// OpenACC clauses.
enum OpenACCClauseKind {
#define OPENACC_CLAUSE(Name, Class) \
  ACCC_##Name,
#define OPENACC_CLAUSE_ALIAS(ClauseAlias, AliasedClause, Class) \
  ACCC_##ClauseAlias,
#include "clang/Basic/OpenACCKinds.def"
  ACCC_unknown
};

OpenACCDirectiveKind getOpenACCDirectiveKind(llvm::StringRef Str);
const char *getOpenACCDirectiveName(OpenACCDirectiveKind Kind);

OpenACCClauseKind getOpenACCClauseKind(llvm::StringRef Str);
const char *getOpenACCBaseDSAName(OpenACCBaseDSAKind Kind);
const char *getOpenACCClauseName(OpenACCClauseKind Kind);

/// Is BaseDSAKind allowed as a base DSA for a reduction?
bool isAllowedBaseDSAForReduction(OpenACCBaseDSAKind BaseDSAKind);

/// Is BaseDSAKind allowed as a DSA on DKind?  Must not be ACCD_unknown or
/// ACC_BASE_DSA_unknown.
bool isAllowedBaseDSAForDirective(OpenACCDirectiveKind DKind,
                                  OpenACCBaseDSAKind BaseDSAKind);

/// Is CKind allowed as a clause for DKind?  Must not be ACCD_unknown or
/// ACCC_unknown.
bool isAllowedClauseForDirective(OpenACCDirectiveKind DKind,
                                 OpenACCClauseKind CKind);

/// Is ParentDKind allowed as a real parent directive for DKind?  DKind must
/// not be ACCD_unknown, but ParentDKind = ACCCD_unknown checks if DKind can
/// have no parent.
bool isAllowedParentForDirective(OpenACCDirectiveKind DKind,
                                 OpenACCDirectiveKind ParentDKind);

/// How many effective directives does this directive represent?  Always 1
/// for non-combined directive.
int getOpenACCEffectiveDirectives(OpenACCDirectiveKind DKind);

/// Is the specified directive an 'acc parallel' or 'acc parallel loop'
/// directive?
bool isOpenACCParallelDirective(OpenACCDirectiveKind DKind);

/// Is the specified directive an 'acc loop' or 'acc parallel loop' directive?
bool isOpenACCLoopDirective(OpenACCDirectiveKind DKind);

/// Is the specified directive a compute directive or combined compute
/// directive?
bool isOpenACCComputeDirective(OpenACCDirectiveKind DKind);

/// Is the specified directive a combined directive?
bool isOpenACCCombinedDirective(OpenACCDirectiveKind DKind);

}

#endif

