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
  Stmt *OMPStmt = getOMPNode();
  for (int i = 0, e = getOpenACCEffectiveDirectives(getDirectiveKind());
       i < e; ++i) {
    if (auto *OMPDir = dyn_cast<OMPExecutableDirective>(OMPStmt))
      OMPStmt = OMPDir->getInnermostCapturedStmt()->getCapturedStmt();
  }
  if (ACCStmt == OMPStmt) // we probably will never implement such a case
    return false;
  PrintingPolicy PolicyOMP = Policy;
  PolicyOMP.OpenACCPrint = OpenACCPrint_OMP;
  std::string ACCStr, OMPStr;
  llvm::raw_string_ostream ACCStrStr(ACCStr), OMPStrStr(OMPStr);
  ACCStmt->printPretty(ACCStrStr, nullptr, PolicyOMP, 0, "\n", Context);
  OMPStmt->printPretty(OMPStrStr, nullptr, PolicyOMP, 0, "\n", Context);
  ACCStrStr.flush();
  OMPStrStr.flush();
  // llvm::errs() << '\n' << "<<<<" << '\n' << ACCStr << '\n' << "----" << '\n'
  //     << OMPStr << '\n' << ">>>>" << '\n';
  return ACCStr != OMPStr;
}

SourceRange ACCExecutableDirective::getConstructRange(
    bool *FinalSemicolonIsNext) const {
  // TODO: We haven't found a clean way to determine the location of the last
  // token from an AST node, so we implemented the following.  The switch below
  // has cases for AST node types that are known to require special handling,
  // and the default case employs a general approach that so far works for
  // other AST node types.  It might need new cases if we discover bugs.  When
  // OpenACC support is upstreamed, this switch statement should probably
  // migrate into a Stmt member function, perhaps named getFullSourceRange,
  // such that the Stmt implementation comes from the default case and other
  // AST node types' overriding implementations come from the other cases.
  //
  // Instead of all this, we previously tried to use getSourceRange (after
  // fixing it for ACCExecutableDirective), but it doesn't work when a
  // ForStmt's body is an ACCExecutableDirective because ForStmt's
  // getSourceRange then calls the ACCExecutableDirective's getEndLoc instead
  // of its getSourceRange.
  //
  // We also tried to implement one general approach that works for all AST
  // node types by descending through the AST looking for the node with the
  // last location.  However, do you use spelling, expansion, or presumed
  // locations (as documented in SourceManager.h)?  Spelling locations don't
  // work because, if the last token is expanded from a macro, its spelling
  // location might be before any other token.  Expansion locations don't work
  // because tokens expanded from the same macro invocation have the same
  // location.  Presumed locations don't work because #line can make tokens
  // appear to be in a different order.  Using operator< for original
  // SourceLocation objects before conversion to any of those forms doesn't
  // work because tokens expanded from macros then appear to occur after other
  // tokens.  Instead, we just use AST node types to determine where to find
  // the last token's location, and we return the original SourceLocation
  // objects there to the caller to convert to any location form the caller
  // desires.
  const Stmt *S = this;
  for (const Stmt *SPrev = nullptr; S != SPrev;) {
    SPrev = S;
    switch (S->getStmtClass()) {
    default:
      // For most AST node types, the last child has the last token.  In at
      // least some cases, calling getEndLoc on the node itself doesn't return
      // that token's location.  For example, when ACCExecutableDirective has
      // an associated statement (which is then its child), getEndLoc on the
      // ACCExecutableDirective points to the end of the directive only, so the
      // child must be examined.
      //
      // TODO: It should be more efficient to add a special case for every AST
      // node type that has multiple children such that the child with the last
      // token can be retrieved by a fixed member function or such that
      // getEndLoc on the node itself suffices, but we haven't yet identified
      // all those AST node types.
      for (const Stmt *Child : S->children()) {
        // Some nodes, such as DeclStmt, have nullptr for some children, so
        // skip those.
        if (Child)
          S = Child;
      }
      break;
    case ArraySubscriptExprClass:
    case CallExprClass:
    case CompoundStmtClass:
    case GenericSelectionExprClass:
    case InitListExprClass:
    case ParenExprClass:
    case UnaryExprOrTypeTraitExprClass:
      // In most cases, getEndLoc refers to the closing bracket, brace, or
      // parenthesis, which is not contained in any of the children.
      //
      // UnaryExprOrTypeTraitExpr, which represents sizeof or _Alignof, has
      // many special cases: no child in the case of most type arguments, child
      // whose getEndLoc doesn't refer to the closing parenthesis in the case
      // of a variable array type argument, and either no closing parenthesis
      // or a closing parenthesis from the child in the case of an expression
      // argument.  In all cases, it seems getEndLoc on
      // UnaryExprOrTypeTraitExpr works fine.
      break;
    }
  }
  if (FinalSemicolonIsNext)
    *FinalSemicolonIsNext = dyn_cast_or_null<Expr>(S);
  return SourceRange(getBeginLoc(), S->getEndLoc());
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
