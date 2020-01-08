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
#define OPENACC_CLAUSE(Name, Class) \
  case ACCC_##Name:                 \
    return static_cast<Class *>(this)->children();
#define OPENACC_CLAUSE_ALIAS(ClauseAlias, AliasedClause, Class) \
  case ACCC_##ClauseAlias:                                    \
    return static_cast<Class *>(this)->children();
#include "clang/Basic/OpenACCKinds.def"
  }
  llvm_unreachable("unknown ACCClause");
}

llvm::iterator_range<ArrayRef<Expr *>::iterator>
clang::getPrivateVarsFromClause(ACCClause *C) {
  switch (C->getClauseKind()) {
  case ACCC_private:
    return cast<ACCPrivateClause>(C)->varlists();
  case ACCC_firstprivate:
    return cast<ACCFirstprivateClause>(C)->varlists();
  case ACCC_reduction:
    return cast<ACCReductionClause>(C)->varlists();
#define OPENACC_CLAUSE_ALIAS_copy(Name) \
  case ACCC_##Name:
#define OPENACC_CLAUSE_ALIAS_copyin(Name) \
  case ACCC_##Name:
#define OPENACC_CLAUSE_ALIAS_copyout(Name) \
  case ACCC_##Name:
#include "clang/Basic/OpenACCKinds.def"
  case ACCC_shared:
  case ACCC_num_gangs:
  case ACCC_num_workers:
  case ACCC_vector_length:
  case ACCC_seq:
  case ACCC_independent:
  case ACCC_auto:
  case ACCC_gang:
  case ACCC_worker:
  case ACCC_vector:
  case ACCC_collapse:
    return llvm::iterator_range<ArrayRef<Expr *>::iterator>(
        llvm::makeArrayRef<Expr *>(nullptr, (int)0));
  case ACCC_unknown:
    llvm_unreachable("unexpected unknown ACCClause");
  }
  llvm_unreachable("unexpected ACCClause");
}

ACCCopyClause *
ACCCopyClause::Create(const ASTContext &C, OpenACCClauseKind Kind,
                      OpenACCDetermination Determination,
                      SourceLocation StartLoc, SourceLocation LParenLoc,
                      SourceLocation EndLoc, ArrayRef<Expr *> VL) {
  // Allocate space for private variables and initializer expressions.
  void *Mem = C.Allocate(totalSizeToAlloc<Expr *>(VL.size()));
  ACCCopyClause *Clause = new (Mem) ACCCopyClause(
      Kind, Determination, StartLoc, LParenLoc, EndLoc, VL.size());
  Clause->setVarRefs(VL);
  return Clause;
}

ACCCopyClause *ACCCopyClause::CreateEmpty(const ASTContext &C,
                                          OpenACCClauseKind Kind, unsigned N) {
  void *Mem = C.Allocate(totalSizeToAlloc<Expr *>(N));
  return new (Mem) ACCCopyClause(Kind, N);
}

ACCCopyinClause *
ACCCopyinClause::Create(const ASTContext &C, OpenACCClauseKind Kind,
                        SourceLocation StartLoc, SourceLocation LParenLoc,
                        SourceLocation EndLoc, ArrayRef<Expr *> VL) {
  // Allocate space for private variables and initializer expressions.
  void *Mem = C.Allocate(totalSizeToAlloc<Expr *>(VL.size()));
  ACCCopyinClause *Clause = new (Mem) ACCCopyinClause(
      Kind, StartLoc, LParenLoc, EndLoc, VL.size());
  Clause->setVarRefs(VL);
  return Clause;
}

ACCCopyinClause *ACCCopyinClause::CreateEmpty(
    const ASTContext &C, OpenACCClauseKind Kind, unsigned N) {
  void *Mem = C.Allocate(totalSizeToAlloc<Expr *>(N));
  return new (Mem) ACCCopyinClause(Kind, N);
}

ACCCopyoutClause *
ACCCopyoutClause::Create(const ASTContext &C, OpenACCClauseKind Kind,
                         SourceLocation StartLoc, SourceLocation LParenLoc,
                         SourceLocation EndLoc, ArrayRef<Expr *> VL) {
  // Allocate space for private variables and initializer expressions.
  void *Mem = C.Allocate(totalSizeToAlloc<Expr *>(VL.size()));
  ACCCopyoutClause *Clause = new (Mem) ACCCopyoutClause(
      Kind, StartLoc, LParenLoc, EndLoc, VL.size());
  Clause->setVarRefs(VL);
  return Clause;
}

ACCCopyoutClause *ACCCopyoutClause::CreateEmpty(
    const ASTContext &C, OpenACCClauseKind Kind, unsigned N) {
  void *Mem = C.Allocate(totalSizeToAlloc<Expr *>(N));
  return new (Mem) ACCCopyoutClause(Kind, N);
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
ACCPrivateClause::Create(
    const ASTContext &C, OpenACCDetermination Determination,
    SourceLocation StartLoc, SourceLocation LParenLoc, SourceLocation EndLoc,
    ArrayRef<Expr *> VL) {
  // Allocate space for private variables and initializer expressions.
  void *Mem = C.Allocate(totalSizeToAlloc<Expr *>(VL.size()));
  ACCPrivateClause *Clause =
      new (Mem) ACCPrivateClause(Determination, StartLoc, LParenLoc, EndLoc,
                                 VL.size());
  Clause->setVarRefs(VL);
  return Clause;
}

ACCPrivateClause *ACCPrivateClause::CreateEmpty(const ASTContext &C,
                                                unsigned N) {
  void *Mem = C.Allocate(totalSizeToAlloc<Expr *>(N));
  return new (Mem) ACCPrivateClause(N);
}

ACCFirstprivateClause *
ACCFirstprivateClause::Create(
    const ASTContext &C, OpenACCDetermination Determination,
    SourceLocation StartLoc, SourceLocation LParenLoc, SourceLocation EndLoc,
    ArrayRef<Expr *> VL) {
  void *Mem = C.Allocate(totalSizeToAlloc<Expr *>(VL.size()));
  ACCFirstprivateClause *Clause =
      new (Mem) ACCFirstprivateClause(Determination, StartLoc, LParenLoc,
                                      EndLoc, VL.size());
  Clause->setVarRefs(VL);
  return Clause;
}

ACCFirstprivateClause *ACCFirstprivateClause::CreateEmpty(const ASTContext &C,
                                                          unsigned N) {
  void *Mem = C.Allocate(totalSizeToAlloc<Expr *>(N));
  return new (Mem) ACCFirstprivateClause(N);
}

ACCReductionClause *
ACCReductionClause::Create(
    const ASTContext &C, OpenACCDetermination Determination,
    SourceLocation StartLoc, SourceLocation LParenLoc, SourceLocation ColonLoc,
    SourceLocation EndLoc, ArrayRef<Expr *> VL,
    const DeclarationNameInfo &NameInfo) {
  void *Mem = C.Allocate(totalSizeToAlloc<Expr *>(VL.size()));
  ACCReductionClause *Clause =
      new (Mem) ACCReductionClause(Determination, StartLoc, LParenLoc,
                                   ColonLoc, EndLoc, VL.size(), NameInfo);
  Clause->setVarRefs(VL);
  return Clause;
}

ACCReductionClause *ACCReductionClause::CreateEmpty(const ASTContext &C,
                                                    unsigned N) {
  void *Mem = C.Allocate(totalSizeToAlloc<Expr *>(N));
  return new (Mem) ACCReductionClause(N);
}
