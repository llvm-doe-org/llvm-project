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

void ACCDirectiveStmt::setClauses(ArrayRef<ACCClause *> Clauses) {
  assert(Clauses.size() == NumClauses &&
         "Number of clauses is not the same as specified during construction");
  std::copy(Clauses.begin(), Clauses.end(), getClauses().begin());
}

void ACCDirectiveStmt::addClause(ACCClause *Clause) {
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
bool ACCDirectiveStmt::ompStmtPrintsDifferently(const PrintingPolicy &Policy,
                                                const ASTContext *Context) {
  if (!hasAssociatedStmt())
    return false;
  // Get the OpenACC directive's associated statement.  If that's also an
  // OpenACC directive, get its OpenMP translation so we can compare by node
  // address and potentially avoid wasting time printing and comparing.
  Stmt *ACCStmt = getAssociatedStmt();
  if (auto *ACCDir = dyn_cast<ACCDirectiveStmt>(ACCStmt)) {
    if (ACCDir->hasOMPNode())
      ACCStmt = ACCDir->getOMPNode();
  }
  // Find the corresponding associated statement on the OpenMP side by
  // traversing all of the OpenACC directive's corresponding OpenMP directives.
  // If those OpenMP directives are not consecutive, then there is intervening
  // code in the OpenMP version, so return true.
  Stmt *OMPStmt = getOMPNode();
  for (int I = 0, E = getOMPDirectiveCount(); I < E; ++I) {
    OMPExecutableDirective *OMPDir = dyn_cast<OMPExecutableDirective>(OMPStmt);
    if (!OMPDir)
      return true;
    OMPStmt = OMPDir->getAssociatedStmt();
    if (isa<CapturedStmt>(OMPStmt))
      OMPStmt = OMPDir->getInnermostCapturedStmt()->getCapturedStmt();
  }
  // Skip printing and comparing if possible.
  if (ACCStmt == OMPStmt)
    return false;
  PrintingPolicy PolicyOMP = Policy;
  // A difference in where indentation is added is not a good reason to return
  // true, so suppress all indentation to avoid such differences.  For example,
  // at least at one time, in the case of a combined OpenACC construct, the
  // indentation of OpenMP directives was aligned when printing via the OpenACC
  // node but progressively indented when printing the OpenMP nodes directly.
  PolicyOMP.Indentation = 0;
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
  void *Mem = C.Allocate(Size + sizeof(ACCClause *) * Clauses.size());
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
  void *Mem = C.Allocate(Size + sizeof(ACCClause *) * NumClauses);
  return new (Mem) ACCUpdateDirective(NumClauses);
}

ACCEnterDataDirective *
ACCEnterDataDirective::Create(const ASTContext &C, SourceLocation StartLoc,
                              SourceLocation EndLoc,
                              ArrayRef<ACCClause *> Clauses) {
  unsigned Size =
      llvm::alignTo(sizeof(ACCEnterDataDirective), alignof(ACCClause *));
  void *Mem = C.Allocate(Size + sizeof(ACCClause *) * Clauses.size());
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
  void *Mem = C.Allocate(Size + sizeof(ACCClause *) * NumClauses);
  return new (Mem) ACCEnterDataDirective(NumClauses);
}

ACCExitDataDirective *
ACCExitDataDirective::Create(const ASTContext &C, SourceLocation StartLoc,
                             SourceLocation EndLoc,
                             ArrayRef<ACCClause *> Clauses) {
  unsigned Size =
      llvm::alignTo(sizeof(ACCExitDataDirective), alignof(ACCClause *));
  void *Mem = C.Allocate(Size + sizeof(ACCClause *) * Clauses.size());
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
  void *Mem = C.Allocate(Size + sizeof(ACCClause *) * NumClauses);
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
    bool NestedExplicitWorkerPartitioning) {
  unsigned Size =
      llvm::alignTo(sizeof(ACCParallelDirective), alignof(ACCClause *));
  void *Mem =
      C.Allocate(Size + sizeof(ACCClause *) * Clauses.size() + sizeof(Stmt *));
  ACCParallelDirective *Dir =
      new (Mem) ACCParallelDirective(StartLoc, EndLoc, Clauses.size());
  Dir->setClauses(Clauses);
  Dir->setAssociatedStmt(AssociatedStmt);
  Dir->setNestedExplicitWorkerPartitioning(NestedExplicitWorkerPartitioning);
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

ACCLoopDirective *ACCLoopDirective::Create(
    const ASTContext &C, SourceLocation StartLoc, SourceLocation EndLoc,
    ArrayRef<ACCClause *> Clauses, Stmt *AssociatedStmt,
    const ArrayRef<VarDecl *> &LCVs, ACCPartitioningKind Partitioning,
    bool NestedExplicitGangPartitioning, bool NestedExplicitWorkerPartitioning,
    bool NestedExplicitVectorPartitioning) {
  unsigned Size = llvm::alignTo(sizeof(ACCLoopDirective),
                                alignof(ACCClause *));
  void *Mem = C.Allocate(Size + sizeof(ACCClause *) * (Clauses.size() + 3) +
                         sizeof(Stmt *) + sizeof(VarDecl *) * LCVs.size());
  ACCLoopDirective *Dir =
      new (Mem) ACCLoopDirective(StartLoc, EndLoc, Clauses.size(),
                                 LCVs.size());
  Dir->setClauses(Clauses);
  Dir->setAssociatedStmt(AssociatedStmt);
  Dir->setLoopControlVariables(LCVs);
  Dir->setPartitioning(Partitioning);
  Dir->setNestedExplicitGangPartitioning(NestedExplicitGangPartitioning);
  Dir->setNestedExplicitWorkerPartitioning(NestedExplicitWorkerPartitioning);
  Dir->setNestedExplicitVectorPartitioning(NestedExplicitVectorPartitioning);
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

ACCAtomicDirective *ACCAtomicDirective::Create(const ASTContext &C,
                                               SourceLocation StartLoc,
                                               SourceLocation EndLoc,
                                               ArrayRef<ACCClause *> Clauses,
                                               Stmt *AssociatedStmt) {
  unsigned Size =
      llvm::alignTo(sizeof(ACCAtomicDirective), alignof(ACCClause *));
  void *Mem = C.Allocate(Size + sizeof(ACCClause *) * (Clauses.size() + 1) +
                         sizeof(Stmt *));
  ACCAtomicDirective *Dir =
      new (Mem) ACCAtomicDirective(StartLoc, EndLoc, Clauses.size());
  Dir->setClauses(Clauses);
  Dir->setAssociatedStmt(AssociatedStmt);
  return Dir;
}

ACCAtomicDirective *ACCAtomicDirective::CreateEmpty(const ASTContext &C,
                                                    unsigned NumClauses,
                                                    EmptyShell) {
  unsigned Size =
      llvm::alignTo(sizeof(ACCAtomicDirective), alignof(ACCClause *));
  void *Mem =
      C.Allocate(Size + sizeof(ACCClause *) * NumClauses + sizeof(Stmt *));
  return new (Mem) ACCAtomicDirective(NumClauses);
}
