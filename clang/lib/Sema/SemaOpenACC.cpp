//===--- SemaOpenACC.cpp - Semantic Analysis for OpenACC constructs -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// This file implements semantic analysis for OpenACC directives and
/// clauses.
///
//===----------------------------------------------------------------------===//

#include "TreeTransform.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTMutationListener.h"
#include "clang/AST/CXXInheritance.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclOpenMP.h"
#include "clang/AST/StmtCXX.h"
#include "clang/AST/StmtOpenACC.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/AST/TypeOrdering.h"
#include "clang/Basic/OpenACCKinds.h"
#include "clang/Basic/OpenMPKinds.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Sema/Initialization.h"
#include "clang/Sema/Lookup.h"
#include "clang/Sema/Scope.h"
#include "clang/Sema/ScopeInfo.h"
#include "clang/Sema/SemaInternal.h"
#include "llvm/ADT/DenseMap.h"
using namespace clang;

//===----------------------------------------------------------------------===//
// Stack of data-sharing attributes for variables
//===----------------------------------------------------------------------===//

namespace {
/// How a loop is partitioned.
///
/// All fields false indicates either a sequential loop or not a loop.
class PartitioningKind {
  bool Gang : 1;
  bool Worker : 1;
  bool Vector : 1;
  bool ExplicitIndependent : 1;
  bool ImplicitIndependent : 1;
public:
  PartitioningKind()
      : Gang(false), Worker(false), Vector(false), ExplicitIndependent(false),
        ImplicitIndependent(false) {}
  void setGang() { Gang = true; }
  void setWorker() { Worker = true; }
  void setVector() { Vector = true; }
  void setExplicitIndependent() {
    assert(!ImplicitIndependent &&
           "expected implicit and explicit to be mutually exclusive");
    ExplicitIndependent = true;
  }
  void setImplicitIndependent() {
    assert(!ExplicitIndependent &&
           "expected implicit and explicit to be mutually exclusive");
    ImplicitIndependent = true;
  }
  bool hasGang() const { return Gang; }
  bool hasWorker() const { return Worker; }
  bool hasVector() const { return Vector; }
  /// True if hasExplicitIndependent() or hasImplicitIndependent().
  bool hasIndependent() const { return ExplicitIndependent ||
                                       ImplicitIndependent; }
  /// True if there's an explicit independent clause.
  bool hasExplicitIndependent() const { return ExplicitIndependent; }
  /// True if there's no explicit auto, seq, or independent clause.
  bool hasImplicitIndependent() const { return ImplicitIndependent; }
};

/// Stack for tracking declarations used in OpenACC directives and
/// clauses and their data-sharing attributes.
class DSAStackTy final {
public:
  /// There are four cases:
  ///
  /// 1. If this DSAVarData is for an explicit data sharing attribute (and thus
  ///    addDSA has already been called for it within the associated
  ///    ActOnOpenACC*Clause), then CKind!=ACCC_unknown, and RefExpr is the
  ///    referencing expression appearing in the clause.
  /// 2. If both (a) this DSAVarData is for an implicit data sharing attribute
  ///    and (b) addDSA has already been called within the associated
  ///    ActOnOpenACC*Clause call, then CKind!=ACCC_unknown, and RefExpr is a
  ///    referencing expression within the directive's region.
  /// 3. If 2a is true but 2b is not true, then CKind!=ACCC_unknown and
  ///    RefExpr=nullptr.
  /// 4. If no data sharing attribute has been computed, then
  ///    CKind=ACCC_unknown and RefExpr=nullptr.
  ///
  /// Because getTopDSA looks for a DSAVarData for which addDSA has already
  /// been called, getTopDSA's result is never case 3, and so its
  /// CKind=ACCC_unknown iff RefExpr=nullptr.
  struct DSAVarData final {
    OpenACCClauseKind CKind = ACCC_unknown;
    Expr *RefExpr = nullptr;
    DSAVarData() {}
  };

private:
  typedef llvm::DenseMap<VarDecl *, DSAVarData> DeclSAMapTy;
  struct SharingMapTy final {
    DeclSAMapTy SharingMap;
    VarDecl *LCV = nullptr;
    OpenACCDirectiveKind Directive = ACCD_unknown;
    PartitioningKind LoopDirectiveKind;
    SourceLocation ConstructLoc;
    // FIXME: LoopBreakLoc isn't directly for data sharing attributes, but I
    // don't know of a better place to put it as it's tied to the Directive
    // recorded here in   Perhaps we should just rename DSAStack, etc. to
    // something more general?  We are considering making the presence of a
    // break statement influence the decision of whether to partition in the
    // case of "acc loop auto", and that could influence data sharing of the
    // loop control variable, but that seems like a weak justification for
    // putting it here.
    SourceLocation LoopBreakLoc; // invalid if no break statement or not loop
    unsigned AssociatedLoops = 1;
    SharingMapTy(OpenACCDirectiveKind DKind, SourceLocation Loc)
        : Directive(DKind), ConstructLoc(Loc) {}
    SharingMapTy() {}
  };

  typedef SmallVector<SharingMapTy, 4> StackTy;

  /// Stack of used declaration and their data-sharing attributes.
  StackTy Stack;

  bool isStackEmpty() const {
    return Stack.empty();
  }

public:
  explicit DSAStackTy(Sema &S) : Stack(1) {}

  void push(OpenACCDirectiveKind DKind, SourceLocation Loc) {
    Stack.push_back(SharingMapTy(DKind, Loc));
  }

  void pop() {
    assert(Stack.size() > 1 && "Data-sharing attributes stack is empty!");
    Stack.pop_back();
  }

  /// Register break statement in current acc loop.
  void addLoopBreakStatement(SourceLocation BreakLoc) {
    assert(!isStackEmpty() && "Data-sharing attributes stack is empty");
    assert(isOpenACCLoopDirective(getCurrentDirective())
           && "Break statement must be added only to loop directive");
    assert(BreakLoc.isValid() && "Expected valid break location");
    if (Stack.back().LoopBreakLoc.isInvalid()) // just the first one we see
      Stack.back().LoopBreakLoc = BreakLoc;
  }
  /// Return the location of the first break statement for this loop
  /// directive or return an invalid location if none.
  SourceLocation getLoopBreakStatement() const {
    assert(!isStackEmpty() && "Data-sharing attributes stack is empty");
    return Stack.back().LoopBreakLoc;
  }
  /// Register specified variable as acc loop control variable that is
  /// assigned but not declared in for loop init.
  void addLoopControlVariable(VarDecl *VD) {
    assert(!isStackEmpty() && "Data-sharing attributes stack is empty");
    assert(isOpenACCLoopDirective(getCurrentDirective())
           && "Loop control variable must be added only to loop directive");
    Stack.back().LCV = VD->getCanonicalDecl();
  }
  /// Get the canonical declaration for the loop control variable that is
  /// assigned but not declared in the init of the for loop associated with the
  /// current directive, or return nullptr if none.
  VarDecl *getLoopControlVariable() const {
    assert(!isStackEmpty() && "Data-sharing attributes stack is empty");
    return Stack.back().LCV;
  }
  /// Register the current directive's loop partitioning kind.
  void setLoopPartitioning(PartitioningKind Kind) {
    assert(!isStackEmpty() && "Data-sharing attributes stack is empty");
    Stack.back().LoopDirectiveKind = Kind;
  }
  /// Get the current directive's loop partitioning kind.
  ///
  /// Must be called only after the point where \c setLoopPartitioning would be
  /// called, which is after explicit clauses for the current directive have
  /// been parsed.
  PartitioningKind getLoopPartitioning() const {
    assert(!isStackEmpty() && "Data-sharing attributes stack is empty");
    return Stack.back().LoopDirectiveKind;
  }
  /// Get the parent directive's loop partitioning kind and location.
  PartitioningKind getParentLoopPartitioning(SourceLocation &ParentLoc) const {
    // If there's no parent acc directive, we get the dummy Stack entry.
    assert(Stack.size() > 1 && "Data-sharing attributes stack is empty");
    ParentLoc = Stack[Stack.size() - 2].ConstructLoc;
    return Stack[Stack.size() - 2].LoopDirectiveKind;
  }

  /// Adds explicit data sharing attribute to the specified declaration.
  void addDSA(VarDecl *D, Expr *E, OpenACCClauseKind A);

  /// Returns data sharing attributes from top of the stack for the
  /// specified declaration.
  DSAVarData getTopDSA(VarDecl *VD);
  /// Returns data-sharing attributes for the specified declaration.
  DSAVarData getImplicitDSA(VarDecl *D);

  /// Finds a directive which matches specified \a DPred predicate.
  bool hasDirective(const llvm::function_ref<bool(OpenACCDirectiveKind,
                                                  SourceLocation)> &DPred);

  /// Returns currently analyzed directive.
  OpenACCDirectiveKind getCurrentDirective() const {
    return isStackEmpty() ? ACCD_unknown : Stack.back().Directive;
  }

  /// Set collapse value for the region.
  void setAssociatedLoops(unsigned Val) {
    assert(!isStackEmpty());
    Stack.back().AssociatedLoops = Val;
  }
  /// Return collapse value for region.
  unsigned getAssociatedLoops() const {
    return isStackEmpty() ? 0 : Stack.back().AssociatedLoops;
  }

  SourceLocation getConstructLoc() { return Stack.back().ConstructLoc; }

  unsigned getNestingLevel() const {
    assert(!isStackEmpty());
    return Stack.size() - 1;
  }
};
} // namespace

void DSAStackTy::addDSA(VarDecl *VD, Expr *E, OpenACCClauseKind A) {
  VD = VD->getCanonicalDecl();
  assert(Stack.size() > 1 && "Data-sharing attributes stack is empty");
  auto &Data = Stack.back().SharingMap[VD];
  assert(Data.CKind == ACCC_unknown || (A == Data.CKind));
  Data.CKind = A;
  Data.RefExpr = E;
}

DSAStackTy::DSAVarData DSAStackTy::getImplicitDSA(VarDecl *VD) {
  // TODO: Check for inherited clauses.
  VD = VD->getCanonicalDecl();
  DSAVarData DVar;
  DVar.RefExpr = nullptr;
  if (VD == getLoopControlVariable()) {
    PartitioningKind LoopKind = getLoopPartitioning();
    // OpenACC 2.6 [2.6.1]:
    //   "The loop variable in a C for statement [...] that is associated
    //   with a loop directive is predetermined to be private to each thread
    //   that will execute each iteration of the loop."
    //
    //   If the loop control variable is assigned but not defined in the for
    //   init, we need to specify a data sharing clause.  The OpenACC spec does
    //   not appear to clarify the end of the scope of the private copy, so we
    //   make what appears to be the intuitive choices for each case below.
    if (LoopKind.hasGang() || LoopKind.hasWorker() || LoopKind.hasVector())
      // Private is consistent with OpenMP in all cases here except
      // LoopKind.hasVector.  That is, OpenMP simd makes it predetermined
      // linear, which has lastprivate-like semantics.  However, for
      // consistency, we assume the intuitive semantics of private in all cases
      // here: the private copy goes out of scope at the end of the loop.
      DVar.CKind = ACCC_private;
    else
      // In the case of a sequential loop (perhaps explicitly declared with
      // seq), we assume the normal behavior of a sequential loop in C: the
      // loop control variable takes its final value computed by the loop.
      // We implement this behavior by making the loop control variable
      // implicitly shared instead of predetermined private, and this
      // doesn't seem to contradict the specification that it be private to
      // the one thread that executes the loop.
      DVar.CKind = ACCC_shared;
  }
  // OpenACC 2.5 [2.5.1.588-590]:
  //   "A scalar variable referenced in the parallel construct that does not
  //   appear in a data clause for the construct or any enclosing data
  //   construct will be treated as if it appeared in a firstprivate clause."
  // OpenACC 2.5 [Glossary.2870]:
  //   "Scalar datatype - an intrinsic or built-in datatype that is not an
  //   array or aggregate datatype."
  // OpenACC 2.5 [Glossary.2871-2874]:
  //   "In C, scalar datatypes are char (signed or unsigned), int (signed or
  //   unsigned, with optional short, long or long long attribute), enum,
  //   float, double, long double, Complex (with optional float or long
  //   attribute) or any pointer datatype."
  else if (VD->getType()->isScalarType()
           && getCurrentDirective() == ACCD_parallel)
    DVar.CKind = ACCC_firstprivate;
  else
    DVar.CKind = ACCC_shared;
  return DVar;
}

bool DSAStackTy::hasDirective(
    const llvm::function_ref<bool(OpenACCDirectiveKind, SourceLocation)>
        &DPred) {
  // We look only in the enclosing region.
  if (isStackEmpty())
    return false;
  auto StartI = std::next(Stack.rbegin());
  auto EndI = Stack.rend();
  for (auto I = StartI, EE = EndI; I != EE; ++I) {
    if (DPred(I->Directive, I->ConstructLoc))
      return true;
  }
  return false;
}

DSAStackTy::DSAVarData DSAStackTy::getTopDSA(VarDecl *VD) {
  VD = VD->getCanonicalDecl();
  DSAVarData DVar;

  if (Stack.size() == 1) {
    // Not in OpenACC execution region and top scope was already checked.
    return DVar;
  }

  auto I = Stack.rbegin();
  if (I->SharingMap.count(VD))
    DVar = I->SharingMap[VD];

  return DVar;
}

void Sema::InitOpenACCDataSharingAttributesStack() {
  OpenACCDataSharingAttributesStack = new DSAStackTy(*this);
}

#define DSAStack static_cast<DSAStackTy *>(OpenACCDataSharingAttributesStack)

void Sema::DestroyOpenACCDataSharingAttributesStack() { delete DSAStack; }

void Sema::StartOpenACCDSABlock(OpenACCDirectiveKind DKind,
                                SourceLocation Loc) {
  DSAStack->push(DKind, Loc);
  PushExpressionEvaluationContext(
      ExpressionEvaluationContext::PotentiallyEvaluated);
}

void Sema::StartOpenACCClause(OpenACCClauseKind K) {
}

ACCClause *Sema::ActOnOpenACCVarListClause(
    OpenACCClauseKind Kind, ArrayRef<Expr *> VarList,
    SourceLocation StartLoc, SourceLocation LParenLoc, SourceLocation ColonLoc,
    SourceLocation EndLoc, const DeclarationNameInfo &ReductionId) {
  ACCClause *Res = nullptr;
  switch (Kind) {
  case ACCC_private:
    Res = ActOnOpenACCPrivateClause(VarList, StartLoc, LParenLoc, EndLoc);
    break;
  case ACCC_firstprivate:
    Res = ActOnOpenACCFirstprivateClause(VarList, StartLoc, LParenLoc, EndLoc);
    break;
  case ACCC_reduction:
    Res = ActOnOpenACCReductionClause(VarList, StartLoc, LParenLoc, ColonLoc,
                                      EndLoc, ReductionId);
    break;
  case ACCC_shared:
  case ACCC_num_gangs:
  case ACCC_seq:
  case ACCC_independent:
  case ACCC_auto:
  case ACCC_gang:
  case ACCC_worker:
  case ACCC_vector:
  case ACCC_unknown:
    llvm_unreachable("Clause is not allowed.");
  }
  return Res;
}

void Sema::EndOpenACCClause() {
}

void Sema::EndOpenACCDSABlock() {
  DSAStack->pop();
  DiscardCleanupsInEvaluationContext();
  PopExpressionEvaluationContext();
}

static void ReportOriginalDSA(Sema &SemaRef, DSAStackTy *Stack,
                              const ValueDecl *D,
                              DSAStackTy::DSAVarData DVar) {
  assert(DVar.RefExpr && "OpenACC data sharing attribute needs expression");
  SemaRef.Diag(DVar.RefExpr->getExprLoc(), diag::note_acc_explicit_dsa)
      << getOpenACCClauseName(DVar.CKind);
}

namespace {
class DSAAttrChecker : public StmtVisitor<DSAAttrChecker, void> {
  typedef StmtVisitor<DSAAttrChecker, void> BaseVisitor;
  DSAStackTy *Stack;
  llvm::SmallVector<Expr *, 8> ImplicitShared;
  llvm::SmallVector<Expr *, 8> ImplicitPrivate;
  llvm::SmallVector<Expr *, 8> ImplicitFirstprivate;
  llvm::DenseSet<ValueDecl *> ImplicitDeclarations;
  llvm::DenseSet<Decl *> LocalDefinitions;

public:
  void VisitDeclStmt(DeclStmt *S) {
    for (auto I = S->decl_begin(), E = S->decl_end(); I != E; ++I)
      LocalDefinitions.insert(*I);
    BaseVisitor::VisitDeclStmt(S);
  }
  void VisitDeclRefExpr(DeclRefExpr *E) {
    if (auto *VD = dyn_cast<VarDecl>(E->getDecl())) {
      // Skip internally declared variables.
      if (LocalDefinitions.count(VD))
        return;

      auto DVar = Stack->getTopDSA(VD);
      // Stop analysis if the variable has DSA already set.
      if (DVar.CKind != ACCC_unknown
          || !ImplicitDeclarations.insert(VD).second)
        return;

      // Compute implicit (perhaps inherited) data sharing attribute.
      DVar = Stack->getImplicitDSA(VD);
      if (DVar.CKind == ACCC_shared)
        ImplicitShared.push_back(E);
      else if (DVar.CKind == ACCC_private)
        ImplicitPrivate.push_back(E);
      else if (DVar.CKind == ACCC_firstprivate)
        ImplicitFirstprivate.push_back(E);
    }
  }
  void VisitMemberExpr(MemberExpr *E) {
    Visit(E->getBase());
  }
  void VisitACCExecutableDirective(ACCExecutableDirective *S) {
    // TODO: Handle clauses in nested OpenACC directives.
    for (auto *C : S->children()) {
      if (C)
        Visit(C);
    }
  }
  void VisitStmt(Stmt *S) {
    for (auto *C : S->children()) {
      if (C)
        Visit(C);
    }
  }

  ArrayRef<Expr *> getImplicitShared() { return ImplicitShared; }
  ArrayRef<Expr *> getImplicitPrivate() { return ImplicitPrivate; }
  ArrayRef<Expr *> getImplicitFirstprivate() { return ImplicitFirstprivate; }

  DSAAttrChecker(DSAStackTy *S) : Stack(S) {}
};
} // namespace

bool Sema::ActOnOpenACCRegionStart(
    OpenACCDirectiveKind DKind, ArrayRef<ACCClause *> Clauses, Scope *CurScope,
    SourceLocation StartLoc, SourceLocation EndLoc) {
  bool ErrorFound = false;
  if (DKind == ACCD_loop) {
    bool HasSeqOrAuto = false;
    bool HasReduction = false;
    PartitioningKind LoopKind;
    for (ACCClause *C : Clauses) {
      OpenACCClauseKind CKind = C->getClauseKind();
      if (CKind == ACCC_seq || CKind == ACCC_auto)
        HasSeqOrAuto = true;
      else if (CKind == ACCC_independent)
        LoopKind.setExplicitIndependent();
      else if (CKind == ACCC_gang)
        LoopKind.setGang();
      else if (CKind == ACCC_worker)
        LoopKind.setWorker();
      else if (CKind == ACCC_vector)
        LoopKind.setVector();
      else if (CKind == ACCC_reduction)
        HasReduction = true;
    }
    assert((!HasSeqOrAuto || !LoopKind.hasExplicitIndependent()) &&
           "Expected parser to reject combinations of seq, independent, and"
           " auto");
    SourceLocation ParentLoopLoc;
    PartitioningKind ParentLoopKind =
        DSAStack->getParentLoopPartitioning(ParentLoopLoc);
    if (ParentLoopKind.hasGang() && LoopKind.hasGang()) {
      // OpenACC 2.6, sec. 2.9.2:
      // "The region of a loop with the gang clause may not contain another
      // loop with the gang clause unless within a nested compute region."
      Diag(StartLoc, diag::err_acc_loop_bad_nested_partitioning)
          << "gang" << "gang";
      Diag(ParentLoopLoc, diag::note_acc_loop_bad_nested_partitioning);
      ErrorFound = true;
    }
    else if (ParentLoopKind.hasWorker() && (LoopKind.hasGang() ||
                                            LoopKind.hasWorker())) {
      // OpenACC 2.6, sec. 2.9.3:
      // "The region of a loop with the worker clause may not contain a loop
      // with a gang or worker clause unless within a nested compute region."
      Diag(StartLoc, diag::err_acc_loop_bad_nested_partitioning)
          << "worker" << (LoopKind.hasGang() ? "gang" : "worker");
      Diag(ParentLoopLoc, diag::note_acc_loop_bad_nested_partitioning);
      ErrorFound = true;
    }
    else if (ParentLoopKind.hasVector() && (LoopKind.hasGang() ||
                                            LoopKind.hasWorker() ||
                                            LoopKind.hasVector())) {
      // OpenACC 2.6, sec. 2.9.4:
      // "The region of a loop with the vector clause may not contain a loop
      // with the gang, worker, or vector clause unless within a nested compute
      // region."
      Diag(StartLoc, diag::err_acc_loop_bad_nested_partitioning)
          << "vector" << (LoopKind.hasGang() ? "gang"
                                             : LoopKind.hasWorker() ? "worker"
                                                                    : "vector");
      Diag(ParentLoopLoc, diag::note_acc_loop_bad_nested_partitioning);
      ErrorFound = true;
    }
    // OpenACC 2.6, sec. 2.9.9:
    // "When the parent compute construct is a parallel construct, the
    // independent clause is implied on all loop constructs without a seq or
    // auto clause."
    if (!HasSeqOrAuto && !LoopKind.hasExplicitIndependent())
      LoopKind.setImplicitIndependent();
    if (!LoopKind.hasIndependent())
      LoopKind = PartitioningKind();
    // TODO: Remove this, HasReduction variable, and
    // diag::err_acc_reduction_on_partitioned_loop when reductions are
    // implemented for partitioned loops.
    if (HasReduction &&
        (LoopKind.hasGang() || LoopKind.hasWorker() || LoopKind.hasVector())) {
      Diag(StartLoc, diag::err_acc_reduction_on_partitioned_loop);
      ErrorFound = true;
    }
    DSAStack->setLoopPartitioning(LoopKind);
  }
  return ErrorFound;
}

StmtResult Sema::ActOnOpenACCRegionEnd(StmtResult S) {
  if (!S.isUsable())
    return StmtError();
  return S.get();
}

StmtResult Sema::ActOnOpenACCExecutableDirective(
    OpenACCDirectiveKind DKind, ArrayRef<ACCClause *> Clauses,
    Stmt *AStmt, SourceLocation StartLoc, SourceLocation EndLoc) {
  StmtResult Res = StmtError();

  llvm::SmallVector<ACCClause *, 8> ClausesWithImplicit;
  bool ErrorFound = false;
  ClausesWithImplicit.append(Clauses.begin(), Clauses.end());
  PartitioningKind LoopKind = DSAStack->getLoopPartitioning();
  if (LoopKind.hasImplicitIndependent()) {
    ACCClause *Implicit = ActOnOpenACCIndependentClause(SourceLocation(),
                                                        SourceLocation());
    assert(Implicit);
    ClausesWithImplicit.push_back(Implicit);
  }
  if (LoopKind.hasIndependent()) {
    SourceLocation BreakLoc = DSAStack->getLoopBreakStatement();
    if (BreakLoc.isValid()) {
      Diag(BreakLoc, diag::err_acc_loop_cannot_use_stmt) << "break";
      ErrorFound = true;
    }
    // TODO: In the case of auto, break statements will force sequential
    // execution, probably with a warning, or is an error better for that case?
  }
  if (AStmt) {
    // Check default data sharing attributes for referenced variables.
    DSAAttrChecker DSAChecker(DSAStack);
    DSAChecker.Visit(AStmt);
    if (!DSAChecker.getImplicitShared().empty()) {
      ACCClause *Implicit = ActOnOpenACCSharedClause(
          DSAChecker.getImplicitShared());
      assert(Implicit);
      ClausesWithImplicit.push_back(Implicit);
    }
    if (!DSAChecker.getImplicitPrivate().empty()) {
      ACCClause *Implicit = ActOnOpenACCPrivateClause(
          DSAChecker.getImplicitPrivate(), SourceLocation(),
          SourceLocation(), SourceLocation());
      assert(Implicit);
      ClausesWithImplicit.push_back(Implicit);
    }
    if (!DSAChecker.getImplicitFirstprivate().empty()) {
      ACCClause *Implicit = ActOnOpenACCFirstprivateClause(
          DSAChecker.getImplicitFirstprivate(), SourceLocation(),
          SourceLocation(), SourceLocation());
      assert(Implicit);
      ClausesWithImplicit.push_back(Implicit);
    }
  }

  switch (DKind) {
  case ACCD_parallel:
    Res = ActOnOpenACCParallelDirective(ClausesWithImplicit, AStmt, StartLoc,
                                        EndLoc);
    break;
  case ACCD_loop:
    Res = ActOnOpenACCLoopDirective(ClausesWithImplicit, AStmt, StartLoc,
                                    EndLoc, DSAStack->getLoopControlVariable());
    break;
  case ACCD_unknown:
    llvm_unreachable("Unknown OpenACC directive");
  }

  if (ErrorFound)
    return StmtError();
  return Res;
}

StmtResult Sema::ActOnOpenACCParallelDirective(ArrayRef<ACCClause *> Clauses,
                                               Stmt *AStmt,
                                               SourceLocation StartLoc,
                                               SourceLocation EndLoc) {
  if (!AStmt)
    return StmtError();

  getCurFunction()->setHasBranchProtectedScope();

  return ACCParallelDirective::Create(Context, StartLoc, EndLoc, Clauses,
                                      AStmt);
}

void Sema::ActOnOpenACCLoopInitialization(SourceLocation ForLoc, Stmt *Init) {
  assert(getLangOpts().OpenACC && "OpenACC is not active.");
  assert(Init && "Expected loop in canonical form.");
  unsigned AssociatedLoops = DSAStack->getAssociatedLoops();
  if (AssociatedLoops &&
      isOpenACCLoopDirective(DSAStack->getCurrentDirective())) {
    if (Expr *E = dyn_cast<Expr>(Init))
      Init = E->IgnoreParens();
    if (auto *BO = dyn_cast<BinaryOperator>(Init)) {
      if (BO->getOpcode() == BO_Assign) {
        auto *LHS = BO->getLHS()->IgnoreParens();
        if (auto *DRE = dyn_cast<DeclRefExpr>(LHS)) {
          auto *VD = dyn_cast<VarDecl>(DRE->getDecl());
          assert(VD);
          DSAStack->addLoopControlVariable(VD);
        }
      }
    }
    DSAStack->setAssociatedLoops(AssociatedLoops - 1);
  }
}

void Sema::ActOnOpenACCLoopBreakStatement(SourceLocation BreakLoc,
                                          Scope *CurScope) {
  assert(getLangOpts().OpenACC && "OpenACC is not active.");
  if (CurScope->getBreakParent()->isOpenACCLoopScope())
    DSAStack->addLoopBreakStatement(BreakLoc);
}

StmtResult Sema::ActOnOpenACCLoopDirective(
    ArrayRef<ACCClause *> Clauses, Stmt *AStmt, SourceLocation StartLoc,
    SourceLocation EndLoc, VarDecl *LCVar) {
  if (!AStmt)
    return StmtError();

  auto *For = dyn_cast_or_null<ForStmt>(AStmt);
  if (!For) {
    Diag(AStmt->getLocStart(), diag::err_acc_not_for)
        << getOpenACCDirectiveName(ACCD_loop);
    return StmtError();
  }
  assert(For->getBody());

  // Complain if we have an acc loop control variable.
  for (ACCClause *C : Clauses) {
    if (ACCReductionClause *RC = dyn_cast_or_null<ACCReductionClause>(C)) {
      for (Expr *VR : RC->varlists()) {
        VarDecl *VD = cast<VarDecl>(cast<DeclRefExpr>(VR)->getDecl())
                      ->getCanonicalDecl();
        if (VD == DSAStack->getLoopControlVariable()) {
           Diag(VR->getLocEnd(), diag::err_acc_reduction_on_loop_control_var)
               << VR->getSourceRange();
           return StmtError();
        }
      }
    }
  }

  // FIXME: Much more validation of the for statement should be performed.  To
  // save implementation time, for now we just depend on the OpenMP
  // implementation to handle that when transforming to OpenMP.  However, that
  // means diagnostics often mention OpenMP constructs not OpenACC constructs,
  // and it means any useful results of the analysis are not available to
  // OpenACC-level tools.

  if (!DSAStack->hasDirective(
           [](OpenACCDirectiveKind K, SourceLocation) -> bool {
             return K == ACCD_parallel;
           }))
  {
    Diag(EndLoc, diag::err_acc_loop_outside_parallel);
    return StmtError();
  }

  return ACCLoopDirective::Create(Context, StartLoc, EndLoc, Clauses, AStmt,
                                  LCVar);
}

ACCClause *Sema::ActOnOpenACCSingleExprClause(OpenACCClauseKind Kind, Expr *Expr,
                                              SourceLocation StartLoc,
                                              SourceLocation LParenLoc,
                                              SourceLocation EndLoc) {
  ACCClause *Res = nullptr;
  switch (Kind) {
  case ACCC_num_gangs:
    Res = ActOnOpenACCNumGangsClause(Expr, StartLoc, LParenLoc, EndLoc);
    break;
  case ACCC_shared:
  case ACCC_private:
  case ACCC_firstprivate:
  case ACCC_reduction:
  case ACCC_seq:
  case ACCC_independent:
  case ACCC_auto:
  case ACCC_gang:
  case ACCC_worker:
  case ACCC_vector:
  case ACCC_unknown:
    llvm_unreachable("Clause is not allowed.");
  }
  return Res;
}

ACCClause *Sema::ActOnOpenACCSharedClause(ArrayRef<Expr *> VarList) {
  SmallVector<Expr *, 8> Vars;
  for (auto &RefExpr : VarList) {
    assert(RefExpr && "NULL expr in OpenACC implicit shared clause.");
    SourceLocation ELoc;
    SourceRange ERange;
    auto *DE = dyn_cast_or_null<DeclRefExpr>(RefExpr->IgnoreParens());
    assert(DE && "OpenACC implicit shared clause for non-DeclRefExpr");
    auto *VD = dyn_cast_or_null<VarDecl>(DE->getDecl());
    assert(VD && "OpenACC implicit shared clause for non-VarDecl");

    // Assert that the variable does not appear in an explicit data sharing
    // clause on the same directive.  If the OpenACC spec grows an explicit
    // shared clause one day, this check should evolve to handle conflicting
    // clauses.
    DSAStackTy::DSAVarData DVar = DSAStack->getTopDSA(VD);
    assert(DVar.CKind == ACCC_unknown
           && "OpenACC implicit shared clause for variable also in explicit"
              " data sharing clause");

    DSAStack->addDSA(VD, RefExpr->IgnoreParens(), ACCC_shared);
    Vars.push_back(RefExpr->IgnoreParens());
  }

  if (Vars.empty())
    return nullptr;

  return ACCSharedClause::Create(Context, Vars);
}

static VarDecl *
getPrivateItem(Sema &S, Expr *&RefExpr, SourceLocation &ELoc,
               SourceRange &ERange) {
  RefExpr = RefExpr->IgnoreParens();
  ELoc = RefExpr->getExprLoc();
  ERange = RefExpr->getSourceRange();
  RefExpr = RefExpr->IgnoreParenImpCasts();
  auto *DE = dyn_cast_or_null<DeclRefExpr>(RefExpr);
  if (!DE || !isa<VarDecl>(DE->getDecl())) {
    // This is reported for uses of, for example, subscript or subarray syntax.
    S.Diag(ELoc, diag::err_acc_expected_var_name_expr)
        << ERange;
    return nullptr;
  }
  return cast<VarDecl>(DE->getDecl());
}

ACCClause *Sema::ActOnOpenACCPrivateClause(ArrayRef<Expr *> VarList,
                                           SourceLocation StartLoc,
                                           SourceLocation LParenLoc,
                                           SourceLocation EndLoc) {
  SmallVector<Expr *, 8> Vars;
  for (auto &RefExpr : VarList) {
    assert(RefExpr && "NULL expr in OpenACC private clause.");
    SourceLocation ELoc;
    SourceRange ERange;
    Expr *SimpleRefExpr = RefExpr;
    VarDecl *VD = getPrivateItem(*this, SimpleRefExpr, ELoc, ERange);
    if (!VD)
      continue;

    QualType Type = VD->getType();

    // The OpenACC 2.5 spec doesn't say, as far as I know, that a private
    // variable must have a complete type.  However, you cannot copy data if it
    // doesn't have a size, and OpenMP does have this restriction.
    if (RequireCompleteType(ELoc, Type, diag::err_acc_private_incomplete_type))
      continue;

    // Complain if a variable appears in more than one data sharing clause on
    // the same directive.
    DSAStackTy::DSAVarData DVar = DSAStack->getTopDSA(VD);
    if (DVar.CKind != ACCC_unknown && DVar.CKind != ACCC_private) {
      Diag(ELoc, diag::err_acc_wrong_dsa)
          << getOpenACCClauseName(DVar.CKind)
          << getOpenACCClauseName(ACCC_private);
      ReportOriginalDSA(*this, DSAStack, VD, DVar);
      continue;
    }

    // The OpenACC 2.5 spec doesn't say, as far as I know, that a const
    // variable cannot be private.  However, you can never initialize the
    // private version of such a variable, and OpenMP does have this
    // restriction.
    if (VD->getType().isConstQualified()) {
      Diag(ELoc, diag::err_acc_const_private);
      Diag(VD->getLocation(), diag::note_acc_const_private)
          << VD;
      continue;
    }

    DSAStack->addDSA(VD, RefExpr->IgnoreParens(), ACCC_private);
    Vars.push_back(RefExpr->IgnoreParens());
  }

  if (Vars.empty())
    return nullptr;

  return ACCPrivateClause::Create(Context, StartLoc, LParenLoc, EndLoc, Vars);
}

ACCClause *Sema::ActOnOpenACCFirstprivateClause(ArrayRef<Expr *> VarList,
                                                SourceLocation StartLoc,
                                                SourceLocation LParenLoc,
                                                SourceLocation EndLoc) {
  SmallVector<Expr *, 8> Vars;
  bool IsImplicitClause =
      StartLoc.isInvalid() && LParenLoc.isInvalid() && EndLoc.isInvalid();
  auto ImplicitClauseLoc = DSAStack->getConstructLoc();

  for (auto &RefExpr : VarList) {
    assert(RefExpr && "NULL expr in OpenACC firstprivate clause.");
    SourceLocation ELoc;
    SourceRange ERange;
    Expr *SimpleRefExpr = RefExpr;
    VarDecl *VD = getPrivateItem(*this, SimpleRefExpr, ELoc, ERange);
    if (!VD)
      continue;

    ELoc = IsImplicitClause ? ImplicitClauseLoc : ELoc;
    QualType Type = VD->getType();

    // The OpenACC 2.5 spec doesn't say, as far as I know, that a private
    // variable must have a complete type.  However, you cannot copy data if it
    // doesn't have a size, and OpenMP does have this restriction.
    if (RequireCompleteType(ELoc, Type,
                            diag::err_acc_firstprivate_incomplete_type))
      continue;

    // If an implicit firstprivate variable found it was checked already.
    if (!IsImplicitClause) {
      // Complain if a variable appears in more than one (explicit) data
      // sharing clause on the same directive.
      DSAStackTy::DSAVarData DVar = DSAStack->getTopDSA(VD);
      if (DVar.CKind != ACCC_unknown && DVar.CKind != ACCC_firstprivate) {
        Diag(ELoc, diag::err_acc_wrong_dsa)
            << getOpenACCClauseName(DVar.CKind)
            << getOpenACCClauseName(ACCC_firstprivate);
        ReportOriginalDSA(*this, DSAStack, VD, DVar);
        continue;
      }
    }

    DSAStack->addDSA(VD, RefExpr->IgnoreParens(), ACCC_firstprivate);
    Vars.push_back(RefExpr->IgnoreParens());
  }

  if (Vars.empty())
    return nullptr;

  return ACCFirstprivateClause::Create(Context, StartLoc, LParenLoc, EndLoc,
                                       Vars);
}

static bool actOnACCReductionKindClause(
    Sema &S, DSAStackTy *Stack, OpenACCClauseKind ClauseKind,
    ArrayRef<Expr *> VarList, SourceLocation StartLoc, SourceLocation LParenLoc,
    SourceLocation ColonLoc, SourceLocation EndLoc,
    const DeclarationNameInfo &ReductionId) {
  DeclarationName DN = ReductionId.getName();
  OverloadedOperatorKind OOK = DN.getCXXOverloadedOperator();
  ASTContext &Context = S.Context;

  // OpenACC 2.6 [2.5.12, reduction clause, line 774]:
  // The list of reduction operators is here.
  //
  // OpenACC 2.6 [2.5.12, reduction clause, lines 771-773]:
  // "Supported data types are the numerical data types in C (char, int,
  // float, double, _Complex)."
  //
  // We do not follow that specification exactly.  Instead, for each reduction
  // operator, we choose the permitted data types based on (1) what operand
  // types the corresponding C operator supports ("x < y ? x : y" for "min",
  // and "x > y ? x : y" for "max"), (2) what operand types make sense for a
  // reduction (the result type and both operand types all must be the same
  // type), and (3) what clang's OpenMP implementation supports.
  //
  // For "+", "*", "&&", and "||", we assume the "numerical data types in C"
  // are exactly C's arithmetic types, even those types that are not listed
  // explicitly by the OpenACC spec as quoted above.  For example, C specifies
  // _Bool and enum types as integer types and thus arithmetic types.
  //
  // "&", "|", and "^" are unique in that we constrain them from arithmetic
  // types to integer types.  This follows C11's operator restrictions, OpenACC
  // and OpenMP as implemented by gcc 7.2.0 and pgcc 18.4, and OpenMP as
  // implemented by clang.
  //
  // "max" and "min" are unique in that we (a) extend them from arithmetic
  // types to include pointer types, which do not appear to be implied by
  // "numerical data types" or the explicit list of types from the OpenACC
  // quote above, and (b) constrain them not to include complex types, which
  // are listed by the quote above but which C's relational operators do not
  // support.  Because of #2 above, these are the only reduction operators for
  // which pointer types make sense.  gcc 7.2.0 and pgcc 18.4 do not support
  // pointer types for OpenACC or OpenMP "max" or "min" reductions, but clang
  // does support them for OpenMP "max" and "min" reductions, so we extend that
  // support to OpenACC as well.
  //
  // Note that OpenACC 2.6 mistypes "^" as "%", which would be nonsense as a
  // reduction operator.  gcc 7.2.0 and pgcc 18.4-0 report syntax errors for
  // "%" as an OpenACC reduction operator, but both accept "^".  Also, OpenMP
  // specifies "^" but not "%" as a reduction operator.
  enum {
    /// accidentally unset
    RedOpInvalid,
    /// C's integer types
    RedOpInteger,
    /// C's arithmetic types (integer and floating)
    RedOpArithmetic,
    /// C's scalar types (floating and pointer) except complex
    RedOpRealOrPointer,
  } RedOpType = RedOpInvalid;
  switch (OOK) {
  case OO_Plus:
  case OO_Star:
  case OO_AmpAmp:
  case OO_PipePipe:
    RedOpType = RedOpArithmetic;
    break;
  case OO_Amp:
  case OO_Pipe:
  case OO_Caret:
    RedOpType = RedOpInteger;
    break;
  case OO_Minus:
  case OO_New:
  case OO_Delete:
  case OO_Array_New:
  case OO_Array_Delete:
  case OO_Slash:
  case OO_Percent:
  case OO_Tilde:
  case OO_Exclaim:
  case OO_Equal:
  case OO_Less:
  case OO_Greater:
  case OO_LessEqual:
  case OO_GreaterEqual:
  case OO_PlusEqual:
  case OO_MinusEqual:
  case OO_StarEqual:
  case OO_SlashEqual:
  case OO_PercentEqual:
  case OO_CaretEqual:
  case OO_AmpEqual:
  case OO_PipeEqual:
  case OO_LessLess:
  case OO_GreaterGreater:
  case OO_LessLessEqual:
  case OO_GreaterGreaterEqual:
  case OO_EqualEqual:
  case OO_ExclaimEqual:
  case OO_Spaceship:
  case OO_PlusPlus:
  case OO_MinusMinus:
  case OO_Comma:
  case OO_ArrowStar:
  case OO_Arrow:
  case OO_Call:
  case OO_Subscript:
  case OO_Conditional:
  case OO_Coawait:
  case NUM_OVERLOADED_OPERATORS:
    llvm_unreachable("Unexpected reduction operator");
  case OO_None: {
    IdentifierInfo *II = DN.getAsIdentifierInfo();
    assert(II && "expected identifier for reduction operator");
    if (!II->isStr("max") && !II->isStr("min")) {
      S.Diag(EndLoc, diag::err_acc_unknown_reduction_operator);
      return true;
    }
    RedOpType = RedOpRealOrPointer;
    break;
  }
  }

  bool FirstIter = true;
  size_t Count = 0;
  for (Expr *RefExpr : VarList) {
    assert(RefExpr && "nullptr expr in OpenACC reduction clause.");
    FirstIter = false;
    SourceLocation ELoc;
    SourceRange ERange;
    Expr *SimpleRefExpr = RefExpr;
    VarDecl *VD = getPrivateItem(S, SimpleRefExpr, ELoc, ERange);
    if (!VD)
      continue;

    QualType Type = VD->getType();

    // The OpenACC 2.6 spec doesn't say, as far as I know, that a private
    // variable must have a complete type.  However, you cannot copy data if it
    // doesn't have a size, and OpenMP does have this restriction.
    if (S.RequireCompleteType(ELoc, Type,
                              diag::err_acc_reduction_incomplete_type))
      continue;

    // The OpenACC 2.6 spec doesn't say, as far as I know, that a const
    // variable cannot be private.  However, you can never initialize the
    // private version of such a variable, and OpenMP does have this
    // restriction.
    if (Type.isConstant(Context)) {
      S.Diag(ELoc, diag::err_acc_const_reduction_list_item) << ERange;
      bool IsDecl = VD->isThisDeclarationADefinition(Context) ==
                        VarDecl::DeclarationOnly;
      S.Diag(VD->getLocation(),
             IsDecl ? diag::note_previous_decl : diag::note_defined_here)
          << VD;
      continue;
    }

    // TODO: When adding support for arrays, this should be on the base element
    // type of the array.  See the OpenMP implementation for an example.

    unsigned Diag = diag::NUM_BUILTIN_SEMA_DIAGNOSTICS;
    switch (RedOpType) {
    case RedOpInteger:
      if (!Type->isIntegerType())
        Diag = diag::err_acc_reduction_not_integer_type;
      break;
    case RedOpArithmetic:
      if (!Type->isArithmeticType())
        Diag = diag::err_acc_reduction_not_arithmetic_type;
      break;
    case RedOpRealOrPointer:
      if (!Type->isRealType() && !Type->isPointerType())
        Diag = diag::err_acc_reduction_not_real_or_pointer_type;
      break;
    case RedOpInvalid:
      llvm_unreachable("expected reduction operand type");
    }
    if (Diag != diag::NUM_BUILTIN_SEMA_DIAGNOSTICS) {
      S.Diag(ELoc, Diag)
          << ACCReductionClause::printReductionOperatorToString(ReductionId);
      bool IsDecl = VD->isThisDeclarationADefinition(Context) ==
                        VarDecl::DeclarationOnly;
      S.Diag(VD->getLocation(),
             IsDecl ? diag::note_previous_decl : diag::note_defined_here)
          << VD;
      continue;
    }

    // Complain if a variable appears in more than one data sharing clause on
    // the same directive.
    //
    // The OpenACC 2.6 spec doesn't say, as far as I know, that a variable
    // cannot be a reduction variable and be either private or firstprivate on
    // the same construct.  However, a reduction implies visibility at the
    // beginning and end of the construct, so private and firstprivate don't
    // make sense.  The OpenMP implementation also has this restriction.
    DSAStackTy::DSAVarData DVar = Stack->getTopDSA(VD);
    if (DVar.CKind == ACCC_reduction) {
      S.Diag(ELoc, diag::err_acc_once_referenced)
          << getOpenACCClauseName(ClauseKind);
      if (DVar.RefExpr)
        S.Diag(DVar.RefExpr->getExprLoc(), diag::note_acc_referenced);
      continue;
    }
    if (DVar.CKind != ACCC_unknown) {
      S.Diag(ELoc, diag::err_acc_wrong_dsa)
          << getOpenACCClauseName(DVar.CKind)
          << getOpenACCClauseName(ACCC_reduction);
      ReportOriginalDSA(S, Stack, VD, DVar);
      continue;
    }

    // Record reduction item.
    Stack->addDSA(VD, RefExpr->IgnoreParens(), ACCC_reduction);
    ++Count;
  }
  return !Count;
}

ACCClause *Sema::ActOnOpenACCReductionClause(
    ArrayRef<Expr *> VarList, SourceLocation StartLoc, SourceLocation LParenLoc,
    SourceLocation ColonLoc, SourceLocation EndLoc,
    const DeclarationNameInfo &ReductionId) {
  if (actOnACCReductionKindClause(*this, DSAStack, ACCC_reduction, VarList,
                                  StartLoc, LParenLoc, ColonLoc, EndLoc,
                                  ReductionId))
    return nullptr;

  return ACCReductionClause::Create(Context, StartLoc, LParenLoc, ColonLoc,
                                    EndLoc, VarList, ReductionId);
}

static bool IsNonNegativeIntegerValue(Expr *&ValExpr, Sema &SemaRef,
                                      OpenACCClauseKind CKind) {
  SourceLocation Loc = ValExpr->getExprLoc();
  // This uses err_omp_* diagnostics, but none currently mention OpenMP or
  // OpenMP-specific constructs, so they should work fine for OpenACC.
  ExprResult Value =
      SemaRef.PerformOpenMPImplicitIntegerConversion(Loc, ValExpr);
  if (Value.isInvalid())
    return false;

  ValExpr = Value.get();
  // The expression must evaluate to a non-negative integer value.
  llvm::APSInt Result;
  if (ValExpr->isIntegerConstantExpr(Result, SemaRef.Context) &&
      Result.isSigned() && !Result.isStrictlyPositive()) {
    SemaRef.Diag(Loc, diag::err_acc_negative_expression_in_clause)
        << getOpenACCClauseName(CKind) << ValExpr->getSourceRange();
    return false;
  }

  return true;
}

ACCClause *Sema::ActOnOpenACCClause(OpenACCClauseKind Kind,
                                    SourceLocation StartLoc,
                                    SourceLocation EndLoc) {
  ACCClause *Res = nullptr;
  switch (Kind) {
  case ACCC_seq:
    Res = ActOnOpenACCSeqClause(StartLoc, EndLoc);
    break;
  case ACCC_independent:
    Res = ActOnOpenACCIndependentClause(StartLoc, EndLoc);
    break;
  case ACCC_auto:
    Res = ActOnOpenACCAutoClause(StartLoc, EndLoc);
    break;
  case ACCC_gang:
    Res = ActOnOpenACCGangClause(StartLoc, EndLoc);
    break;
  case ACCC_worker:
    Res = ActOnOpenACCWorkerClause(StartLoc, EndLoc);
    break;
  case ACCC_vector:
    Res = ActOnOpenACCVectorClause(StartLoc, EndLoc);
    break;
  case ACCC_shared:
  case ACCC_private:
  case ACCC_firstprivate:
  case ACCC_reduction:
  case ACCC_num_gangs:
  case ACCC_unknown:
    llvm_unreachable("Clause is not allowed.");
  }
  return Res;
}

ACCClause *Sema::ActOnOpenACCSeqClause(SourceLocation StartLoc,
                                       SourceLocation EndLoc) {
  return new (Context) ACCSeqClause(StartLoc, EndLoc);
}

ACCClause *Sema::ActOnOpenACCIndependentClause(SourceLocation StartLoc,
                                               SourceLocation EndLoc) {
  return new (Context) ACCIndependentClause(StartLoc, EndLoc);
}

ACCClause *Sema::ActOnOpenACCAutoClause(SourceLocation StartLoc,
                                        SourceLocation EndLoc) {
  return new (Context) ACCAutoClause(StartLoc, EndLoc);
}

ACCClause *Sema::ActOnOpenACCGangClause(SourceLocation StartLoc,
                                        SourceLocation EndLoc) {
  return new (Context) ACCGangClause(StartLoc, EndLoc);
}

ACCClause *Sema::ActOnOpenACCWorkerClause(SourceLocation StartLoc,
                                          SourceLocation EndLoc) {
  return new (Context) ACCWorkerClause(StartLoc, EndLoc);
}

ACCClause *Sema::ActOnOpenACCVectorClause(SourceLocation StartLoc,
                                          SourceLocation EndLoc) {
  return new (Context) ACCVectorClause(StartLoc, EndLoc);
}

ACCClause *Sema::ActOnOpenACCNumGangsClause(Expr *NumGangs,
                                            SourceLocation StartLoc,
                                            SourceLocation LParenLoc,
                                            SourceLocation EndLoc) {
  // OpenMP says num_teams must evaluate to a positive integer value.
  // OpenACC doesn't specify such a restriction that I see for num_gangs, but
  // it seems reasonable.
  if (!IsNonNegativeIntegerValue(NumGangs, *this, ACCC_num_gangs))
    return nullptr;
  return new (Context) ACCNumGangsClause(NumGangs, StartLoc, LParenLoc,
                                         EndLoc);
}

namespace {
class TransformACCToOMP : public TransformContext<TransformACCToOMP> {
  typedef TransformContext<TransformACCToOMP> BaseTransform;
public:
  TransformACCToOMP(Sema &SemaRef) : BaseTransform(SemaRef) {}

private:
  using OMPClauseResult = ActionResult<OMPClause *>;
  inline OMPClauseResult OMPClauseError() { return OMPClauseResult(true); }
  inline OMPClauseResult OMPClauseEmpty() { return OMPClauseResult(false); }

  void transformACCClauses(ACCExecutableDirective *D,
                           OpenMPDirectiveKind TDKind,
                           llvm::SmallVectorImpl<OMPClause *> &TClauses,
                           size_t &TClausesEmptyCount) {
    TClausesEmptyCount = 0;
    ArrayRef<ACCClause *> Clauses = D->clauses();
    TClauses.reserve(Clauses.size());
    for (ArrayRef<ACCClause *>::iterator I = Clauses.begin(), E = Clauses.end();
         I != E; ++I) {
      assert(*I);
      OMPClauseResult ClauseResult = getDerived().TransformACCClause(D, TDKind,
                                                                     *I);
      if (ClauseResult.isUnset())
        ++TClausesEmptyCount;
      else if (!ClauseResult.isInvalid())
        TClauses.push_back(ClauseResult.get());
    }
  }

  StmtResult transformACCAssociatedStmt(ACCExecutableDirective *D,
                                        OpenMPDirectiveKind TKind,
                                        ArrayRef<OMPClause *> TClauses) {
    getSema().ActOnOpenMPRegionStart(TKind, /*CurScope=*/nullptr);
    StmtResult Body;
    {
      Sema::CompoundScopeRAII CompoundScope(getSema());
      Stmt *CS = D->getAssociatedStmt();
      Body = getDerived().TransformStmt(CS);
    }
    return getSema().ActOnOpenMPRegionEnd(Body, TClauses);
  }

  StmtResult transformACCExecutableDirective(ACCExecutableDirective *D,
                                             OpenMPDirectiveKind TDKind) {
    llvm::SmallVector<OMPClause *, 16> TClauses;
    size_t TClausesEmptyCount;
    transformACCClauses(D, TDKind, TClauses, TClausesEmptyCount);
    StmtResult AssociatedStmt = transformACCAssociatedStmt(D, TDKind,
                                                           TClauses);
    if (AssociatedStmt.isInvalid() ||
        TClauses.size() != D->clauses().size() - TClausesEmptyCount)
      return StmtError();
    StmtResult Res = getDerived().RebuildOMPExecutableDirective(
        TDKind, DeclarationNameInfo(), OMPD_unknown, TClauses,
        AssociatedStmt.get(), D->getLocStart(), D->getLocEnd());
    if (!Res.isInvalid())
      D->setOMPNode(Res.get());
    return Res;
  }

  /// A RAII object to conditionally build a compound scope and statement.
  class ConditionalCompoundStmtRAII {
    TransformACCToOMP &Tx;
    bool Started : 1;
    bool Finalized : 1;
    bool Err : 1;
    SmallVector<Stmt*, 8> Stmts;
    llvm::DenseMap<Decl *, Decl *> OldDeclTxs;
  public:
    ConditionalCompoundStmtRAII(TransformACCToOMP &Tx)
        : Tx(Tx), Started(false), Finalized(false), Err(false) {}
    void addPrivateDecl(SourceLocation StartLoc, SourceLocation EndLoc,
                        VarDecl *VD) {
      VarDecl *Def = VD->getDefinition();
      assert(Def && "expected private variable to have definition");
      if (!Started) {
        Tx.getSema().ActOnStartOfCompoundStmt(false);
        Started = true;
      }
      // Record old transformation for Def.  If we've already done that, that
      // means Def appeared multiple times in private clauses.  Once is enough,
      // and we don't want to lose the old transformation.
      Decl *OldDeclTx = Tx.getDerived().TransformDecl(StartLoc, Def);
      if (!OldDeclTxs.try_emplace(Def, OldDeclTx).second)
        return;
      // Revert the old transformation for D so we can create a new one.
      Tx.getDerived().transformedLocalDecl(Def, Def);
      // Transform D and create a declaration statement for it.
      Decl *DPrivate = Tx.getDerived().TransformDefinition(StartLoc, Def,
                                                           /*DropInit*/true);
      StmtResult Res = Tx.getSema().ActOnDeclStmt(
          Tx.getSema().ConvertDeclToDeclGroup(DPrivate), StartLoc, EndLoc);
      if (Res.isInvalid())
        Err = true;
      else
        Stmts.push_back(Res.get());
    }
    void finalize(StmtResult &S) {
      assert(!Finalized && "expected only one finalization");
      Finalized = true;
      if (Err || S.isInvalid()) {
        Err = true;
        S = StmtError();
        return;
      }
      if (!Started) {
        assert(Stmts.empty() &&
               "expected only final statement because compound statement never"
               " started");
        return;
      }
      Stmts.push_back(S.get());
      S = Tx.getSema().ActOnCompoundStmt(Stmts.front()->getLocStart(),
                                         Stmts.back()->getLocEnd(), Stmts,
                                         false);
    }
    ~ConditionalCompoundStmtRAII() {
      assert((Err || Finalized) &&
             "expected compound statement to be finalized");
      if (!Started) {
        assert(OldDeclTxs.empty() &&
               "expected no declarations to restore because compound statement"
               " was never started");
        return;
      }
      for (auto OldDeclTx : OldDeclTxs)
        Tx.getDerived().transformedLocalDecl(OldDeclTx.first,
                                             OldDeclTx.second);
      Tx.getSema().ActOnFinishOfCompoundStmt();
    }
  };

public:
  StmtResult TransformACCParallelDirective(ACCParallelDirective *D) {
    getSema().StartOpenMPDSABlock(OMPD_target_teams, DeclarationNameInfo(),
                                  /*CurScope=*/nullptr, D->getLocStart());
    StmtResult Res = getDerived().transformACCExecutableDirective(
        D, OMPD_target_teams);
    getSema().EndOpenMPDSABlock(Res.get());
    return Res;
  }

  StmtResult TransformACCLoopDirective(ACCLoopDirective *D) {
    // What OpenACC clauses do we have?
    bool HasIndependent = false;
    bool HasGang = false;
    bool HasWorker = false;
    bool HasVector = false;
    for (ACCClause *C : D->clauses()) {
      OpenACCClauseKind CKind = C->getClauseKind();
      if (CKind == ACCC_independent)
        HasIndependent = true; // implicit or explicit independent
      else if (CKind == ACCC_gang)
        HasGang = true;
      else if (CKind == ACCC_worker)
        HasWorker = true;
      else if (CKind == ACCC_vector)
        HasVector = true;
    }

    // What kind of OpenMP directive should we build?
    // OMPD_unknown means none (so sequential).
    OpenMPDirectiveKind TDKind;
    bool AddNumThreads1 = false;
    bool AddScopeWithLCVPrivate = false;
    bool AddScopeWithAllPrivates = false;
    if (!HasIndependent) {
      // sequential loop
      TDKind = OMPD_unknown;
      AddScopeWithAllPrivates = true;
    } else if (!HasGang) {
      if (!HasWorker) {
        if (!HasVector) {
          // sequential loop
          TDKind = OMPD_unknown;
          AddScopeWithAllPrivates = true;
        } else { // HasVector
          TDKind = OMPD_parallel_for_simd;
          AddNumThreads1 = true;
          AddScopeWithLCVPrivate = true;
        }
      } else { // HasWorker
        if (!HasVector)
          TDKind = OMPD_parallel_for;
        else { // HasVector
          TDKind = OMPD_parallel_for_simd;
          AddScopeWithLCVPrivate = true;
        }
      }
    } else { // HasGang
      if (!HasWorker) {
        if (!HasVector)
          TDKind = OMPD_distribute;
        else { // HasVector
          TDKind = OMPD_distribute_simd;
          AddScopeWithLCVPrivate = true;
        }
      } else { // HasWorker
        if (!HasVector)
          TDKind = OMPD_distribute_parallel_for;
        else { // HasVector
          TDKind = OMPD_distribute_parallel_for_simd;
          AddScopeWithLCVPrivate = true;
        }
      }
    }

    // Build enclosing compound statement for privates if needed.
    ConditionalCompoundStmtRAII EnclosingCompoundStmt(*this);
    if (AddScopeWithLCVPrivate || AddScopeWithAllPrivates) {
      for (ACCPrivateClause *C : D->getClausesOfKind<ACCPrivateClause>()) {
        for (Expr *VR : C->varlists()) {
          VarDecl *VD = cast<VarDecl>(cast<DeclRefExpr>(VR)->getDecl())
                        ->getCanonicalDecl();
          if ((AddScopeWithLCVPrivate && D->getLoopControlVariable() == VD) ||
              AddScopeWithAllPrivates)
            EnclosingCompoundStmt.addPrivateDecl(VR->getLocStart(),
                                                 VR->getLocEnd(), VD);
        }
      }
    }

    // Handle case of no OpenMP directive.
    if (TDKind == OMPD_unknown) {
      Sema::CompoundScopeRAII CompoundScope(getSema());
      Stmt *CS = D->getAssociatedStmt();
      StmtResult Res = getDerived().TransformStmt(CS);
      EnclosingCompoundStmt.finalize(Res);
      if (!Res.isInvalid())
        D->setOMPNode(Res.get(), true);
      return Res;
    }

    // Build OpenMP construct.
    getSema().StartOpenMPDSABlock(TDKind, DeclarationNameInfo(),
                                  /*CurScope=*/nullptr, D->getLocStart());
    llvm::SmallVector<OMPClause *, 16> TClauses;
    if (AddNumThreads1) {
      OpenMPStartEndClauseRAII ClauseRAII(getSema(), OMPC_num_threads);
      ExprResult One = getSema().ActOnIntegerConstant(D->getLocEnd(), 1);
      assert(!One.isInvalid());
      TClauses.push_back(getDerived().RebuildOMPNumThreadsClause(
          One.get(), D->getLocEnd(), D->getLocEnd(), D->getLocEnd()));
    }
    size_t TClausesEmptyCount;
    transformACCClauses(D, TDKind, TClauses, TClausesEmptyCount);
    StmtResult AssociatedStmt = transformACCAssociatedStmt(D, TDKind,
                                                           TClauses);
    StmtResult Res;
    if (AssociatedStmt.isInvalid() ||
        TClauses.size() != D->clauses().size() - TClausesEmptyCount +
                           AddNumThreads1)
      Res = StmtError();
    else
      Res = getDerived().RebuildOMPExecutableDirective(
          TDKind, DeclarationNameInfo(), OMPD_unknown, TClauses,
          AssociatedStmt.get(), D->getLocStart(), D->getLocEnd());
    EnclosingCompoundStmt.finalize(Res);
    if (!Res.isInvalid())
      D->setOMPNode(Res.get());
    getSema().EndOpenMPDSABlock(Res.get());
    return Res;
  }

  OMPClauseResult TransformACCClause(ACCExecutableDirective *D,
                                     OpenMPDirectiveKind TDKind,
                                     ACCClause *C) {
    if (!C)
      return OMPClauseError();

    switch (C->getClauseKind()) {
    // Transform individual clause nodes
  #define OPENACC_CLAUSE(Name, Class)                                         \
    case ACCC_ ## Name :                                                      \
      return getDerived().Transform ## Class(D, TDKind, cast<Class>(C));
  #include "clang/Basic/OpenACCKinds.def"
    case ACCC_unknown:
      llvm_unreachable("Unexpected OpenACC directive");
    }

  }

private:
  // This RAII ensures that we call Sema::EndOpenMPClause.
  class OpenMPStartEndClauseRAII {
  private:
    Sema &S;
  public:
    OpenMPStartEndClauseRAII(Sema &S, OpenMPClauseKind TKind) : S(S) {
       S.StartOpenMPClause(TKind);
    }
    ~OpenMPStartEndClauseRAII() { S.EndOpenMPClause(); }
  };

  // This helper class computes valid locations for a clause so that -ast-dump
  // will not indicate that the clause is implicit.  This reflects the behavior
  // that implicit OpenACC clauses become explicit OpenMP clauses because
  // OpenMP has different rules for generating implicit clauses.
  //
  // To keep it implicit instead, specify D = nullptr to the constructor.
  struct ExplicitClauseLocs {
    const SourceLocation LocStart;
    const SourceLocation LParenLoc;
    const SourceLocation ColonLoc;
    const SourceLocation LocEnd;
    ExplicitClauseLocs(ACCExecutableDirective *D, ACCClause *C,
                       SourceLocation LParenLoc,
                       SourceLocation ColonLoc = SourceLocation())
      // So far, it appears that only LocStart is used to decide if the
      // directive is implicit.
      : LocStart(D && C->getLocStart().isInvalid() ? D->getLocEnd()
                                                   : C->getLocStart()),
        LParenLoc(LParenLoc), ColonLoc(ColonLoc), LocEnd(C->getLocEnd())
    {
      // So far, we have found that, if the clause's LocStart is invalid, all
      // the clause's locations are invalid.  Otherwise, setting LocStart to
      // the directive end might produce locations that are out of order.
      assert((!C->getLocStart().isInvalid() ||
              (LParenLoc.isInvalid() && ColonLoc.isInvalid() &&
               C->getLocEnd().isInvalid())) &&
              "Inconsistent location validity");
      assert((!D || !D->getLocEnd().isInvalid()) &&
             "Invalid directive location");
    }
  };

  template<typename Derived>
  bool transformACCVarList(
      ACCExecutableDirective *D, ACCVarListClause<Derived> *C,
      VarDecl *SkipVar, llvm::SmallVector<Expr *, 16> &Vars) {
    Vars.reserve(C->varlist_size());
    for (auto *VE : C->varlists()) {
      if (SkipVar == cast<VarDecl>(cast<DeclRefExpr>(VE)->getDecl())
                     ->getCanonicalDecl())
        continue;
      ExprResult EVar = getDerived().TransformExpr(cast<Expr>(VE));
      if (EVar.isInvalid())
        return true;
      Vars.push_back(EVar.get());
    }
    return false;
  }

  template<typename Derived>
  OMPClauseResult transformACCVarListClause(
      ACCExecutableDirective *D, ACCVarListClause<Derived> *C,
      OpenMPClauseKind TCKind, VarDecl *SkipVar,
      OMPClause *(TransformACCToOMP::*Rebuilder)(
          ArrayRef<Expr *>, SourceLocation, SourceLocation, SourceLocation)) {
    OpenMPStartEndClauseRAII ClauseRAII(getSema(), TCKind);
    llvm::SmallVector<Expr *, 16> Vars;
    if (transformACCVarList(D, C, SkipVar, Vars))
      return OMPClauseError();
    ExplicitClauseLocs L(D, C, C->getLParenLoc());
    return (getDerived().*Rebuilder)(Vars, L.LocStart, L.LParenLoc, L.LocEnd);
  }

public:
  OMPClauseResult TransformACCNumGangsClause(ACCExecutableDirective *D,
                                             OpenMPDirectiveKind TDKind,
                                             ACCNumGangsClause *C) {
    OpenMPStartEndClauseRAII ClauseRAII(getSema(), OMPC_num_teams);
    ExprResult E = getDerived().TransformExpr(C->getNumGangs());
    if (E.isInvalid())
      return OMPClauseError();
    ExplicitClauseLocs L(D, C, C->getLParenLoc());
    return getDerived().RebuildOMPNumTeamsClause(
        E.get(), L.LocStart, L.LParenLoc, L.LocEnd);
  }

  OMPClauseResult TransformACCSharedClause(ACCExecutableDirective *D,
                                           OpenMPDirectiveKind TDKind,
                                           ACCSharedClause *C) {
    // OpenMP distribute directives without parallel do not accept shared
    // clauses, so let our implicit OpenACC shared clauses stay implicit in
    // OpenMP.
    bool RequireImplicit = isOpenMPDistributeDirective(TDKind) &&
                           !isOpenMPParallelDirective(TDKind);
    // Currently, there's no such thing as an explicit OpenACC shared clause.
    // If there were and RequireImplicit=true, we would need to complain to the
    // user as the OpenMP implementation might not complain unless we print the
    // generated OpenMP and recompile it.
    assert(C->getLocStart().isInvalid()
           && "Unexpected explicit OpenACC shared clause");
    return transformACCVarListClause<ACCSharedClause>(
        RequireImplicit ? nullptr : D, C, OMPC_shared, nullptr,
        &TransformACCToOMP::RebuildOMPSharedClause);
  }

  OMPClauseResult TransformACCPrivateClause(ACCExecutableDirective *D,
                                            OpenMPDirectiveKind TDKind,
                                            ACCPrivateClause *C) {
    // An OpenACC loop control variable that is assigned but not declared in
    // the init of the attached for loop is implicitly or explicitly private.
    // However, if the directive translates to an OpenMP directive in the simd
    // family, then the variable is predetermined linear and the step is
    // implicitly the increment from the loop.  We make it private by adding an
    // enclosing variable declaration.  It might be nice to also add an
    // explicit linear clause to make our expectations clear in the generated
    // source or AST dump, but then we'd have to compute the increment from the
    // loop.  So that the OpenACC implementation doesn't have to replicate the
    // OpenMP implementation for that computation, we instead omit the linear
    // clause.
    VarDecl *SkipVar = nullptr;
    if (isOpenMPSimdDirective(TDKind)) {
      ACCLoopDirective *LD = cast<ACCLoopDirective>(D);
      assert(LD && "expected omp simd directive to translate from acc loop"
                   " directive");
      SkipVar = LD->getLoopControlVariable();
    }
    return transformACCVarListClause<ACCPrivateClause>(
        D, C, OMPC_private, SkipVar,
        &TransformACCToOMP::RebuildOMPPrivateClause);
  }

  OMPClauseResult TransformACCFirstprivateClause(ACCExecutableDirective *D,
                                                 OpenMPDirectiveKind TDKind,
                                                 ACCFirstprivateClause *C) {
    return transformACCVarListClause<ACCFirstprivateClause>(
        D, C, OMPC_firstprivate, nullptr,
        &TransformACCToOMP::RebuildOMPFirstprivateClause);
  }

  OMPClauseResult TransformACCReductionClause(ACCExecutableDirective *D,
                                              OpenMPDirectiveKind TDKind,
                                              ACCReductionClause *C) {
    OpenMPStartEndClauseRAII ClauseRAII(getSema(), OMPC_reduction);
    llvm::SmallVector<Expr *, 16> Vars;
    if (transformACCVarList(D, C, nullptr, Vars))
      return OMPClauseError();
    ExplicitClauseLocs L(D, C, C->getLParenLoc(), C->getColonLoc());
    CXXScopeSpec Spec;
    return getDerived().RebuildOMPReductionClause(
        Vars, L.LocStart, L.LParenLoc, L.ColonLoc, L.LocEnd, Spec,
        C->getNameInfo(), ArrayRef<Expr*>());
  }

  OMPClauseResult TransformACCSeqClause(ACCExecutableDirective *D,
                                        OpenMPDirectiveKind TDKind,
                                        ACCSeqClause *C) {
    return OMPClauseEmpty();
  }

  OMPClauseResult TransformACCIndependentClause(ACCExecutableDirective *D,
                                                OpenMPDirectiveKind TDKind,
                                                ACCIndependentClause *C) {
    return OMPClauseEmpty();
  }

  OMPClauseResult TransformACCAutoClause(ACCExecutableDirective *D,
                                         OpenMPDirectiveKind TDKind,
                                         ACCAutoClause *C) {
    return OMPClauseEmpty();
  }

  OMPClauseResult TransformACCGangClause(ACCExecutableDirective *D,
                                         OpenMPDirectiveKind TDKind,
                                         ACCGangClause *C) {
    return OMPClauseEmpty();
  }

  OMPClauseResult TransformACCWorkerClause(ACCExecutableDirective *D,
                                          OpenMPDirectiveKind TDKind,
                                           ACCWorkerClause *C) {
    return OMPClauseEmpty();
  }

  OMPClauseResult TransformACCVectorClause(ACCExecutableDirective *D,
                                           OpenMPDirectiveKind TDKind,
                                           ACCVectorClause *C) {
    return OMPClauseEmpty();
  }
};
} // namespace

bool Sema::transformACCToOMP(ACCExecutableDirective *D) {
  if (DSAStack->getNestingLevel() > 0)
    return false;
  if (TransformACCToOMP(*this).TransformStmt(D).isInvalid())
    return true;
  return false;
}
