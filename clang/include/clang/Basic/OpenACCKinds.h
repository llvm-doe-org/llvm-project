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
#include "clang/Basic/OpenACCKinds.def"
  ACCD_unknown
};

/// OpenACC clauses.
enum OpenACCClauseKind {
#define OPENACC_CLAUSE(Name, Class) \
  ACCC_##Name,
#include "clang/Basic/OpenACCKinds.def"
  ACCC_unknown
};

OpenACCDirectiveKind getOpenACCDirectiveKind(llvm::StringRef Str);
const char *getOpenACCDirectiveName(OpenACCDirectiveKind Kind);

OpenACCClauseKind getOpenACCClauseKind(llvm::StringRef Str);
const char *getOpenACCClauseName(OpenACCClauseKind Kind);

bool isAllowedClauseForDirective(OpenACCDirectiveKind DKind,
                                 OpenACCClauseKind CKind);

/// Checks if the specified directive is a directive with an associated
/// loop construct.
/// \param DKind Specified directive.
/// \return true - the directive is a loop-associated directive like 'acc loop'
/// directive, otherwise - false.
bool isOpenACCLoopDirective(OpenACCDirectiveKind DKind);
}

#endif

