//===--- OpenACCClause.cpp - Classes for OpenACC clauses ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the subclesses of Stmt class declared in OpenACCClause.h
//
//===----------------------------------------------------------------------===//

#include "clang/AST/OpenACCClause.h"

#include "clang/AST/ASTContext.h"

using namespace clang;

ACCClause::child_range ACCClause::children() {
  switch (getClauseKind()) {
  default:
    break;
#define OPENACC_CLAUSE(Name, Class)                                           \
  case ACCC_##Name:                                                           \
    return static_cast<Class *>(this)->children();
#include "clang/Basic/OpenACCKinds.def"
  }
  llvm_unreachable("unknown ACCClause");
}

ACCSharedClause *
ACCSharedClause::Create(const ASTContext &C, ArrayRef<Expr *> VL) {
  // Allocate space for variables.
  void *Mem = C.Allocate(totalSizeToAlloc<Expr *>(VL.size()));
  ACCSharedClause *Clause = new (Mem) ACCSharedClause(VL.size());
  Clause->setVarRefs(VL);
  return Clause;
}

ACCSharedClause *ACCSharedClause::CreateEmpty(const ASTContext &C,
                                              unsigned N) {
  void *Mem = C.Allocate(totalSizeToAlloc<Expr *>(N));
  return new (Mem) ACCSharedClause(N);
}

ACCPrivateClause *
ACCPrivateClause::Create(const ASTContext &C, SourceLocation StartLoc,
                         SourceLocation LParenLoc, SourceLocation EndLoc,
                         ArrayRef<Expr *> VL) {
  // Allocate space for private variables and initializer expressions.
  void *Mem = C.Allocate(totalSizeToAlloc<Expr *>(VL.size()));
  ACCPrivateClause *Clause =
      new (Mem) ACCPrivateClause(StartLoc, LParenLoc, EndLoc, VL.size());
  Clause->setVarRefs(VL);
  return Clause;
}

ACCPrivateClause *ACCPrivateClause::CreateEmpty(const ASTContext &C,
                                                unsigned N) {
  void *Mem = C.Allocate(totalSizeToAlloc<Expr *>(N));
  return new (Mem) ACCPrivateClause(N);
}

ACCFirstprivateClause *
ACCFirstprivateClause::Create(const ASTContext &C, SourceLocation StartLoc,
                              SourceLocation LParenLoc, SourceLocation EndLoc,
                              ArrayRef<Expr *> VL) {
  void *Mem = C.Allocate(totalSizeToAlloc<Expr *>(VL.size()));
  ACCFirstprivateClause *Clause =
      new (Mem) ACCFirstprivateClause(StartLoc, LParenLoc, EndLoc, VL.size());
  Clause->setVarRefs(VL);
  return Clause;
}

ACCFirstprivateClause *ACCFirstprivateClause::CreateEmpty(const ASTContext &C,
                                                          unsigned N) {
  void *Mem = C.Allocate(totalSizeToAlloc<Expr *>(N));
  return new (Mem) ACCFirstprivateClause(N);
}

ACCReductionClause *
ACCReductionClause::Create(const ASTContext &C, SourceLocation StartLoc,
                           SourceLocation LParenLoc, SourceLocation ColonLoc,
                           SourceLocation EndLoc, ArrayRef<Expr *> VL,
                           const DeclarationNameInfo &NameInfo) {
  void *Mem = C.Allocate(totalSizeToAlloc<Expr *>(VL.size()));
  ACCReductionClause *Clause =
      new (Mem) ACCReductionClause(StartLoc, LParenLoc, ColonLoc, EndLoc, VL.size(),
                                   NameInfo);
  Clause->setVarRefs(VL);
  return Clause;
}

ACCReductionClause *ACCReductionClause::CreateEmpty(const ASTContext &C,
                                                    unsigned N) {
  void *Mem = C.Allocate(totalSizeToAlloc<Expr *>(N));
  return new (Mem) ACCReductionClause(N);
}
