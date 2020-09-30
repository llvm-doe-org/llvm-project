//===--- StmtOpenACC.cpp - Classes for OpenACC directives -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the subclasses of Stmt class declared in StmtOpenACC.h
//
//===----------------------------------------------------------------------===//

#include "clang/AST/StmtOpenACC.h"

#include "clang/AST/ASTContext.h"
#include "clang/AST/StmtOpenMP.h"

using namespace clang;

void ACCExecutableDirective::setClauses(ArrayRef<ACCClause *> Clauses) {
  assert(Clauses.size() == NumClauses &&
         "Number of clauses is not the same as specified during construction");
  std::copy(Clauses.begin(), Clauses.end(), getClauses().begin());
}

void ACCExecutableDirective::addClause(ACCClause *Clause) {
  assert(MaxAddClauses > 0 &&
         "Number of clauses is too many for the preallocated buffer");
  getClauseStorage()[NumClauses++] = Clause;
  --MaxAddClauses;
}

// The implementation of this check works as follows.  First, it prints the
// associated statements to strings using Policy.OpenACCPrint =
// OpenACCPrint_OMP.  That policy shouldn't matter for the OpenMP version
// because it contains no OpenACC because all OpenACC regions are transformed
// to OpenMP along with the outermost OpenACC region.  For the OpenACC version,
// that policy makes the nested OpenACC regions look the same as in the OpenMP
// version.  Second, it compares the two strings and returns the result.
//
// TODO: The implementation could be more efficient.  It has to print each
// OpenACC region 2N+1 times where N is the number of directives enclosing it.
// Without nesting, that means it prints a region 3 times (N=1).  With
// two-levels, it prints the code that is only in the outer region 3 times
// (N=1), and it prints the code that is only in the inner region 5 times
// (N=2).  This poor efficiency probably doesn't matter much if a user is using
// printing for just a manual inspection of the code (command-line -ast-print),
// but it might if users use printing as part of their compilation process.  A
// more efficient way probably amounts to some sort of AST diff (I believe I've
// read somewhere about a clang mechanism or extension for that) so that large
// strings wouldn't need to be generated and so that nested OpenACC regions
// could be skipped entirely during the diff.
bool ACCExecutableDirective::ompStmtPrintsDifferently(
    const PrintingPolicy &Policy, const ASTContext *Context) {
  if (!hasAssociatedStmt())
    return false;
  Stmt *ACCStmt = getAssociatedStmt();
  // If the associated statement is an OpenACC node, print its OpenMP node
  // directly instead of printing it via the OpenACC node.  This matters in the
  // case of a combined OpenACC construct, for which indentation of the OpenMP
  // translations of the effective OpenACC directives is aligned when printing
  // via the OpenACC node but progressively indented when printing the OpenMP
  // nodes directly.  A difference in indentation is not a good reason to
  // return true.
  if (auto *ACCDir = dyn_cast<ACCExecutableDirective>(ACCStmt)) {
    if (ACCDir->hasOMPNode())
      ACCStmt = ACCDir->getOMPNode();
  }
  Stmt *OMPStmt = getOMPNode();
  for (int i = 0, e = getOpenACCEffectiveDirectives(getDirectiveKind());
       i < e; ++i) {
    if (auto *OMPDir = dyn_cast<OMPExecutableDirective>(OMPStmt))
      OMPStmt = OMPDir->getInnermostCapturedStmt()->getCapturedStmt();
  }
  if (ACCStmt == OMPStmt)
    return false;
  PrintingPolicy PolicyOMP = Policy;
  PolicyOMP.OpenACCPrint = OpenACCPrint_OMP;
  std::string ACCStr, OMPStr;
  llvm::raw_string_ostream ACCStrStr(ACCStr), OMPStrStr(OMPStr);
  ACCStmt->printPretty(ACCStrStr, nullptr, PolicyOMP, 0, "\n", Context);
  OMPStmt->printPretty(OMPStrStr, nullptr, PolicyOMP, 0, "\n", Context);
  ACCStrStr.flush();
  OMPStrStr.flush();
  //llvm::errs() << '\n'
  //    << "<<<< (" << ACCStmt->getStmtClassName() << ")\n"
  //    << ACCStr << '\n'
  //    << "---- (" << OMPStmt->getStmtClassName() << ")\n"
  //    << OMPStr << '\n'
  //    << ">>>>" << '\n';
  return ACCStr != OMPStr;
}

ACCUpdateDirective *ACCUpdateDirective::Create(const ASTContext &C,
                                               SourceLocation StartLoc,
                                               SourceLocation EndLoc,
                                               ArrayRef<ACCClause *> Clauses) {
  unsigned Size =
      llvm::alignTo(sizeof(ACCUpdateDirective), alignof(ACCClause *));
  void *Mem =
      C.Allocate(Size + sizeof(ACCClause *) * Clauses.size() + sizeof(Stmt *));
  ACCUpdateDirective *Dir =
      new (Mem) ACCUpdateDirective(StartLoc, EndLoc, Clauses.size());
  Dir->setClauses(Clauses);
  return Dir;
}

ACCUpdateDirective *ACCUpdateDirective::CreateEmpty(const ASTContext &C,
                                                    unsigned NumClauses,
                                                    EmptyShell) {
  unsigned Size =
      llvm::alignTo(sizeof(ACCUpdateDirective), alignof(ACCClause *));
  void *Mem =
      C.Allocate(Size + sizeof(ACCClause *) * NumClauses + sizeof(Stmt *));
  return new (Mem) ACCUpdateDirective(NumClauses);
}

ACCEnterDataDirective *
ACCEnterDataDirective::Create(const ASTContext &C, SourceLocation StartLoc,
                              SourceLocation EndLoc,
                              ArrayRef<ACCClause *> Clauses) {
  unsigned Size =
      llvm::alignTo(sizeof(ACCEnterDataDirective), alignof(ACCClause *));
  void *Mem =
      C.Allocate(Size + sizeof(ACCClause *) * Clauses.size() + sizeof(Stmt *));
  ACCEnterDataDirective *Dir =
      new (Mem) ACCEnterDataDirective(StartLoc, EndLoc, Clauses.size());
  Dir->setClauses(Clauses);
  return Dir;
}

ACCEnterDataDirective *ACCEnterDataDirective::CreateEmpty(const ASTContext &C,
                                                          unsigned NumClauses,
                                                          EmptyShell) {
  unsigned Size =
      llvm::alignTo(sizeof(ACCEnterDataDirective), alignof(ACCClause *));
  void *Mem =
      C.Allocate(Size + sizeof(ACCClause *) * NumClauses + sizeof(Stmt *));
  return new (Mem) ACCEnterDataDirective(NumClauses);
}

ACCExitDataDirective *
ACCExitDataDirective::Create(const ASTContext &C, SourceLocation StartLoc,
                             SourceLocation EndLoc,
                             ArrayRef<ACCClause *> Clauses) {
  unsigned Size =
      llvm::alignTo(sizeof(ACCExitDataDirective), alignof(ACCClause *));
  void *Mem =
      C.Allocate(Size + sizeof(ACCClause *) * Clauses.size() + sizeof(Stmt *));
  ACCExitDataDirective *Dir =
      new (Mem) ACCExitDataDirective(StartLoc, EndLoc, Clauses.size());
  Dir->setClauses(Clauses);
  return Dir;
}

ACCExitDataDirective *ACCExitDataDirective::CreateEmpty(const ASTContext &C,
                                                        unsigned NumClauses,
                                                        EmptyShell) {
  unsigned Size =
      llvm::alignTo(sizeof(ACCExitDataDirective), alignof(ACCClause *));
  void *Mem =
      C.Allocate(Size + sizeof(ACCClause *) * NumClauses + sizeof(Stmt *));
  return new (Mem) ACCExitDataDirective(NumClauses);
}

ACCDataDirective *ACCDataDirective::Create(
    const ASTContext &C, SourceLocation StartLoc, SourceLocation EndLoc,
    ArrayRef<ACCClause *> Clauses, Stmt *AssociatedStmt) {
  unsigned Size =
      llvm::alignTo(sizeof(ACCDataDirective), alignof(ACCClause *));
  void *Mem =
      C.Allocate(Size + sizeof(ACCClause *) * Clauses.size() + sizeof(Stmt *));
  ACCDataDirective *Dir =
      new (Mem) ACCDataDirective(StartLoc, EndLoc, Clauses.size());
  Dir->setClauses(Clauses);
  Dir->setAssociatedStmt(AssociatedStmt);
  return Dir;
}

ACCDataDirective *ACCDataDirective::CreateEmpty(const ASTContext &C,
                                                unsigned NumClauses,
                                                EmptyShell) {
  unsigned Size =
      llvm::alignTo(sizeof(ACCDataDirective), alignof(ACCClause *));
  void *Mem =
      C.Allocate(Size + sizeof(ACCClause *) * NumClauses + sizeof(Stmt *));
  return new (Mem) ACCDataDirective(NumClauses);
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
    const ArrayRef<VarDecl *> &LCVs, ACCPartitioningKind Partitioning,
    bool NestedGangPartitioning) {
  unsigned Size = llvm::alignTo(sizeof(ACCLoopDirective),
                                alignof(ACCClause *));
  void *Mem = C.Allocate(Size + sizeof(ACCClause *) * (Clauses.size() + 1) +
                         sizeof(Stmt *) + sizeof(VarDecl *) * LCVs.size());
  ACCLoopDirective *Dir =
      new (Mem) ACCLoopDirective(StartLoc, EndLoc, Clauses.size(),
                                 LCVs.size());
  Dir->setClauses(Clauses);
  Dir->setAssociatedStmt(AssociatedStmt);
  Dir->setLoopControlVariables(LCVs);
  Dir->setPartitioning(Partitioning);
  Dir->setNestedGangPartitioning(NestedGangPartitioning);
  return Dir;
}

ACCLoopDirective *ACCLoopDirective::CreateEmpty(
    const ASTContext &C, unsigned NumClauses, unsigned NumLCVs, EmptyShell) {
  unsigned Size = llvm::alignTo(sizeof(ACCLoopDirective),
                                alignof(ACCClause *));
  void *Mem = C.Allocate(Size + sizeof(ACCClause *) * NumClauses +
                         sizeof(Stmt *) + sizeof(VarDecl *) * NumLCVs);
  return new (Mem) ACCLoopDirective(NumClauses, NumLCVs);
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
