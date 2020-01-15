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

/// How OpenACC clauses or data attributes are determined.
///
/// Some diagnostics depend on the exact values here, so do not reorder.
enum OpenACCDetermination {
  ACC_UNDETERMINED,  ///< undetermined
  ACC_EXPLICIT,      ///< explicitly determined
  ACC_PREDETERMINED, ///< predetermined
  ACC_IMPLICIT       ///< implicitly determined
};

/// OpenACC data mapping attributes.
enum OpenACCDMAKind {
#define OPENACC_DMA(Name) \
  ACC_DMA_##Name,
#include "clang/Basic/OpenACCKinds.def"
  ACC_DMA_unknown
};

/// OpenACC data sharing attributes.
enum OpenACCDSAKind {
#define OPENACC_DSA_MAPPABLE(Name) \
  ACC_DSA_##Name,
#define OPENACC_DSA_UNMAPPABLE(Name) \
  ACC_DSA_##Name,
#include "clang/Basic/OpenACCKinds.def"
  ACC_DSA_unknown
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
const char *getOpenACCName(OpenACCDirectiveKind Kind);

OpenACCClauseKind getOpenACCClauseKind(llvm::StringRef Str);
const char *getOpenACCName(OpenACCDMAKind Kind);
const char *getOpenACCName(OpenACCDSAKind Kind);
const char *getOpenACCName(OpenACCClauseKind Kind);

/// Is DSAKind permitted to be combined with the given DMA?
bool isAllowedDSAForDMA(OpenACCDSAKind DSAKind, OpenACCDMAKind DMAKind);
bool isAllowedDSAForDMA(OpenACCDMAKind DMAKind, OpenACCDSAKind DSAKind);

/// Is DMAKind allowed as a DMA on DKind?  Must not be ACCD_unknown or
/// ACC_DMA_unknown.
bool isAllowedDAForDirective(OpenACCDirectiveKind DKind,
                             OpenACCDMAKind DMAKind);

/// Is DSAKind allowed as a DSA on DKind?  Must not be ACCD_unknown or
/// ACC_DSA_unknown.
bool isAllowedDAForDirective(OpenACCDirectiveKind DKind,
                             OpenACCDSAKind DSAKind);

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

