//===--- StmtOpenACC.cpp - Classes for OpenACC directives -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the subclesses of Stmt class declared in StmtOpenACC.h
//
//===----------------------------------------------------------------------===//

#include "clang/AST/StmtOpenACC.h"

#include "clang/AST/ASTContext.h"
#include "clang/AST/StmtOpenMP.h"

using namespace clang;

void ACCExecutableDirective::setClauses(ArrayRef<ACCClause *> Clauses) {
  assert(Clauses.size() == getNumClauses() &&
         "Number of clauses is not the same as the preallocated buffer");
  std::copy(Clauses.begin(), Clauses.end(), getClauses().begin());
}

ACCParallelDirective *ACCParallelDirective::Create(
    const ASTContext &C, SourceLocation StartLoc, SourceLocation EndLoc,
    ArrayRef<ACCClause *> Clauses, Stmt *AssociatedStmt,
    bool NestedWorkerPartitioning) {
  unsigned Size =
      llvm::alignTo(sizeof(ACCParallelDirective), alignof(ACCClause *));
  void *Mem =
      C.Allocate(Size + sizeof(ACCClause *) * Clauses.size() + sizeof(Stmt *));
  ACCParallelDirective *Dir =
      new (Mem) ACCParallelDirective(StartLoc, EndLoc, Clauses.size());
  Dir->setClauses(Clauses);
  Dir->setAssociatedStmt(AssociatedStmt);
  Dir->setNestedWorkerPartitioning(NestedWorkerPartitioning);
  return Dir;
}

ACCParallelDirective *ACCParallelDirective::CreateEmpty(const ASTContext &C,
                                                        unsigned NumClauses,
                                                        EmptyShell) {
  unsigned Size =
      llvm::alignTo(sizeof(ACCParallelDirective), alignof(ACCClause *));
  void *Mem =
      C.Allocate(Size + sizeof(ACCClause *) * NumClauses + sizeof(Stmt *));
  return new (Mem) ACCParallelDirective(NumClauses);
}

ACCLoopDirective *
ACCLoopDirective::Create(
    const ASTContext &C, SourceLocation StartLoc, SourceLocation EndLoc,
    ArrayRef<ACCClause *> Clauses, Stmt *AssociatedStmt,
    const llvm::DenseSet<VarDecl *> &LCVars,
    OpenACCClauseKind ParentLoopPartitioning) {
  unsigned Size = llvm::alignTo(sizeof(ACCLoopDirective), alignof(ACCClause *));
  void *Mem =
      C.Allocate(Size + sizeof(ACCClause *) * Clauses.size() + sizeof(Stmt *));
  ACCLoopDirective *Dir =
      new (Mem) ACCLoopDirective(StartLoc, EndLoc, Clauses.size());
  Dir->setClauses(Clauses);
  Dir->setAssociatedStmt(AssociatedStmt);
  Dir->setLoopControlVariables(LCVars);
  Dir->setParentLoopPartitioning(ParentLoopPartitioning);
  return Dir;
}

ACCLoopDirective *ACCLoopDirective::CreateEmpty(const ASTContext &C,
                                                unsigned NumClauses,
                                                EmptyShell) {
  unsigned Size = llvm::alignTo(sizeof(ACCLoopDirective), alignof(ACCClause *));
  void *Mem =
      C.Allocate(Size + sizeof(ACCClause *) * NumClauses + sizeof(Stmt *));
  return new (Mem) ACCLoopDirective(NumClauses);
}

ACCParallelLoopDirective *
ACCParallelLoopDirective::Create(
    const ASTContext &C, SourceLocation StartLoc, SourceLocation EndLoc,
    ArrayRef<ACCClause *> Clauses, Stmt *AssociatedStmt,
    ACCParallelDirective *ParallelDir)
{
  unsigned Size = llvm::alignTo(sizeof(ACCParallelLoopDirective),
                                alignof(ACCClause *));
  void *Mem =
      C.Allocate(Size + sizeof(ACCClause *) * Clauses.size() + sizeof(Stmt *));
  ACCParallelLoopDirective *Dir =
      new (Mem) ACCParallelLoopDirective(StartLoc, EndLoc, Clauses.size());
  Dir->setClauses(Clauses);
  Dir->setAssociatedStmt(AssociatedStmt);
  Dir->setEffectiveDirective(ParallelDir);
  return Dir;
}

ACCParallelLoopDirective *ACCParallelLoopDirective::CreateEmpty(
    const ASTContext &C, unsigned NumClauses, EmptyShell) {
  unsigned Size = llvm::alignTo(sizeof(ACCParallelLoopDirective),
                                alignof(ACCClause *));
  void *Mem =
      C.Allocate(Size + sizeof(ACCClause *) * NumClauses + sizeof(Stmt *));
  return new (Mem) ACCParallelLoopDirective(NumClauses);
}
