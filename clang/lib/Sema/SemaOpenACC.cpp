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
/// Stack for tracking declarations used in OpenACC directives and
/// clauses and their data-sharing attributes.
///
/// FIXME: This is no longer about just data sharing attributes.  We should
/// probably rename it to something more general.
///
/// In the case of a combined directive, we push two entries on the stack, one
/// for each effective directive.  Because the entries are pushed and popped
/// together, an iteration of the stack that encounters one entry for a
/// combined directive can always assume another entry follows.
class DSAStackTy final {
public:
  Sema &SemaRef;

  /// There are four cases:
  ///
  /// 1. If this DSAVarData is for an explicit data sharing attribute (and thus
  ///    addDSA has already been called for it within the associated
  ///    ActOnOpenACC*Clause), then CKind!=ACCC_unknown, RefExpr is the
  ///    referencing expression appearing in the clause, and ReductionId is the
  ///    reduction ID if a reduction or is default constructed otherwise.
  /// 2. If both (a) this DSAVarData is for an implicit data sharing attribute
  ///    and (b) addDSA has already been called within the associated
  ///    ActOnOpenACC*Clause call, then CKind!=ACCC_unknown, and RefExpr is a
  ///    referencing expression within the directive's region, and ReductionId
  ///    is the reduction ID if a reduction or is default constructed
  ///    otherwise.
  /// 3. If 2a is true but 2b is not true, then CKind!=ACCC_unknown,
  ///    RefExpr=nullptr, and ReductionId is default constructed.
  /// 4. If no data sharing attribute has been computed, then
  ///    CKind=ACCC_unknown, RefExpr=nullptr, and ReductionId is default
  ///    constructed.
  ///
  /// Because getTopDSA looks for a DSAVarData for which addDSA has already
  /// been called, getTopDSA's result is never case 3, and so its
  /// CKind=ACCC_unknown iff RefExpr=nullptr.
  struct DSAVarData final {
    OpenACCClauseKind CKind = ACCC_unknown;
    Expr *RefExpr = nullptr;
    DeclarationNameInfo ReductionId;
    DSAVarData() {}
  };

private:
  typedef llvm::DenseMap<VarDecl *, DSAVarData> DeclSAMapTy;
  struct SharingMapTy final {
    DeclSAMapTy SharingMap;
    llvm::DenseSet<VarDecl *> LCVs;
    /// The real directive kind.  In the case of a combined directive, there
    /// are two consecutive entries: the outer has RealDKind as the combined
    /// directive kind, the inner has RealDKind has ACCD_unknown, and both
    /// have EffectiveDKind != ACCD_unknown.
    OpenACCDirectiveKind RealDKind = ACCD_unknown;
    /// The effective directive kind, which is always the same as RealDKind in
    /// the case of a non-combined directive.
    OpenACCDirectiveKind EffectiveDKind = ACCD_unknown;
    ACCPartitioningKind LoopDirectiveKind;
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
    unsigned AssociatedLoops = 1; // from collapse clause
    unsigned AssociatedLoopsParsed = 0; // how many have been parsed so far
    // True if this directive has a (separate or combined with this directive)
    // nested acc loop directive with worker partitioning.
    bool NestedWorkerPartitioning = false;
    SharingMapTy(OpenACCDirectiveKind RealDKind,
                 OpenACCDirectiveKind EffectiveDKind, SourceLocation Loc)
        : RealDKind(RealDKind), EffectiveDKind(EffectiveDKind),
          ConstructLoc(Loc) {}
    SharingMapTy() {}
  };

  typedef SmallVector<SharingMapTy, 4> StackTy;

  /// Stack of used declaration and their data-sharing attributes.
  StackTy Stack;

  bool isStackEmpty() const {
    return Stack.empty();
  }

public:
  explicit DSAStackTy(Sema &S) : SemaRef(S), Stack(1) {}

  void push(OpenACCDirectiveKind RealDKind,
            OpenACCDirectiveKind EffectiveDKind, SourceLocation Loc) {
    Stack.push_back(SharingMapTy(RealDKind, EffectiveDKind, Loc));
  }

  void pop() {
    assert(Stack.size() > 1 && "Data-sharing attributes stack is empty!");
    Stack.pop_back();
  }

  /// Register break statement in current acc loop.
  void addLoopBreakStatement(SourceLocation BreakLoc) {
    assert(!isStackEmpty() && "Data-sharing attributes stack is empty");
    assert(isOpenACCLoopDirective(getEffectiveDirective())
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
    assert(isOpenACCLoopDirective(getEffectiveDirective())
           && "Loop control variable must be added only to loop directive");
    Stack.back().LCVs.insert(VD->getCanonicalDecl());
  }
  /// Get the canonical declarations for the loop control variables that are
  /// assigned but not declared in the inits of the for loops associated with
  /// the current directive, or return an empty set if none.
  const llvm::DenseSet<VarDecl *> &getLoopControlVariables() const {
    assert(!isStackEmpty() && "Data-sharing attributes stack is empty");
    return Stack.back().LCVs;
  }
  /// Register the current directive's loop partitioning kind.
  void setLoopPartitioning(ACCPartitioningKind Kind) {
    assert(!isStackEmpty() && "Data-sharing attributes stack is empty");
    Stack.back().LoopDirectiveKind = Kind;
  }
  /// Get the current directive's loop partitioning kind.
  ///
  /// Must be called only after the point where \c setLoopPartitioning would be
  /// called, which is after explicit clauses for the current directive have
  /// been parsed.
  ACCPartitioningKind getLoopPartitioning() const {
    assert(!isStackEmpty() && "Data-sharing attributes stack is empty");
    return Stack.back().LoopDirectiveKind;
  }
  /// Iterate through the ancestor directives until finding either (1) an acc
  /// loop directive or combined loop directive with gang, worker, or vector
  /// partitioning, (2) a compute directive, or (3) or the start of the stack.
  /// If case 1, return that directive's loop partitioning kind, and record its
  /// real directive kind and location.  Else if case 2 or 3, return no
  /// partitioning, and don't record a directive kind or location.
  ACCPartitioningKind getParentLoopPartitioning(
      OpenACCDirectiveKind &ParentDir, SourceLocation &ParentLoc) const {
    for (auto I = std::next(Stack.rbegin()), E = Stack.rend(); I != E; ++I) {
      const ACCPartitioningKind &ParentKind = I->LoopDirectiveKind;
      if (isOpenACCComputeDirective(I->EffectiveDKind) ||
          ParentKind.hasGangPartitioning() ||
          ParentKind.hasWorkerPartitioning() ||
          ParentKind.hasVectorPartitioning())
      {
        while (I->RealDKind == ACCD_unknown)
          I = std::next(I);
        ParentDir = I->RealDKind;
        ParentLoc = I->ConstructLoc;
        return ParentKind;
      }
    }
    return ACCPartitioningKind();
  }

  /// Adds data sharing attribute to the specified declaration.
  bool addDSA(VarDecl *D, Expr *E, OpenACCClauseKind A, bool IsImplicit,
              const DeclarationNameInfo &ReductionId = DeclarationNameInfo(),
              bool CopiedGangReduction = false, int StartLevel = 0);

  /// Returns data sharing attributes from top of the stack for the
  /// specified declaration.
  DSAVarData getTopDSA(VarDecl *VD);
  /// Returns data-sharing attributes for the specified declaration.
  DSAVarData getImplicitDSA(VarDecl *D);

  /// Returns currently analyzed directive.
  OpenACCDirectiveKind getRealDirective() const {
    auto I = Stack.rbegin();
    if (I == Stack.rend())
      return ACCD_unknown;
    while (I->RealDKind == ACCD_unknown && I->EffectiveDKind != ACCD_unknown)
      I = std::next(I);
    return I->RealDKind;
  }

  /// Returns the effective directive currently being analyzed (always the
  /// same as getRealDirective unless the latter is a combined directive).
  OpenACCDirectiveKind getEffectiveDirective() const {
    return isStackEmpty() ? ACCD_unknown : Stack.back().EffectiveDKind;
  }

  /// Returns the real directive for construct enclosing currently analyzed
  /// real directive or returns ACCD_unknown if none.  In the former case only,
  /// set ParentLoc as the returned directive's location.
  OpenACCDirectiveKind getRealParentDirective(SourceLocation &ParentLoc) const {
    // Find real directive.
    auto I = Stack.rbegin();
    if (I == Stack.rend())
      return ACCD_unknown;
    while (I->RealDKind == ACCD_unknown && I->EffectiveDKind != ACCD_unknown)
      I = std::next(I);
    // Find real parent directive.
    I = std::next(I);
    if (I == Stack.rend())
      return ACCD_unknown;
    while (I->RealDKind == ACCD_unknown && I->EffectiveDKind != ACCD_unknown)
      I = std::next(I);
    ParentLoc = I->ConstructLoc;
    return I->RealDKind;
  }

  /// Returns effective directive for construct enclosing currently analyzed
  /// effective directive or returns ACCD_unknown if none.
  OpenACCDirectiveKind getEffectiveParentDirective() const {
    auto I = Stack.rbegin();
    if (I == Stack.rend())
      return ACCD_unknown;
    I = std::next(I);
    if (I == Stack.rend())
      return ACCD_unknown;
    return I->EffectiveDKind;
  }

  /// Set associated loop count (collapse value) for the region.
  void setAssociatedLoops(unsigned Val) {
    assert(!isStackEmpty());
    assert(Stack.back().AssociatedLoops == 1);
    assert(Stack.back().AssociatedLoopsParsed == 0);
    Stack.back().AssociatedLoops = Val;
  }
  /// Return associated loop count (collapse value) for the region.
  unsigned getAssociatedLoops() const {
    return isStackEmpty() ? 0 : Stack.back().AssociatedLoops;
  }
  /// Increment associated loops parsed in region so far.
  void incAssociatedLoopsParsed() {
    assert(!isStackEmpty());
    ++Stack.back().AssociatedLoopsParsed;
  }
  /// Get associated loops parsed in region so far.
  unsigned getAssociatedLoopsParsed() {
    return isStackEmpty() ? 0 : Stack.back().AssociatedLoopsParsed;
  }

  SourceLocation getConstructLoc() { return Stack.back().ConstructLoc; }

  /// Mark all ancestor directives (including the effective parent directive
  /// if this is the effective child directive in a combined directive) as
  /// containing worker partitioning.
  void setWorkerPartitioning() {
    assert(isOpenACCLoopDirective(getEffectiveDirective()) &&
           "expected worker partitioning to be on a loop directive");
    assert(getEffectiveParentDirective() != ACCD_unknown &&
           "unexpected orphaned acc loop directive");
    for (auto I = std::next(Stack.rbegin()), E = std::prev(Stack.rend());
         I != E; ++I) {
      assert((isOpenACCLoopDirective(I->EffectiveDKind) ||
              isOpenACCParallelDirective(I->EffectiveDKind)) &&
             "expected worker partitioning to be nested in acc loop or"
             " parallel directive");
      I->NestedWorkerPartitioning = true;
    }
  }
  /// Does this directive have a nested acc loop directive (either a separate
  /// directive or an effective child directive in a combined directive) with
  /// worker partitioning?
  bool getNestedWorkerPartitioning() const {
    assert(!isStackEmpty());
    return Stack.back().NestedWorkerPartitioning;
  }
};
} // namespace

static void ReportOriginalDSA(Sema &SemaRef, DSAStackTy *Stack,
                              const ValueDecl *D,
                              DSAStackTy::DSAVarData DVar) {
  assert(DVar.RefExpr && "OpenACC data sharing attribute needs expression");
  SemaRef.Diag(DVar.RefExpr->getExprLoc(), diag::note_acc_explicit_dsa)
      << getOpenACCClauseName(DVar.CKind);
}

bool DSAStackTy::addDSA(
    VarDecl *VD, Expr *E, OpenACCClauseKind CKind, bool IsImplicit,
    const DeclarationNameInfo &ReductionId, bool CopiedGangReduction,
    int StartLevel) {
  VD = VD->getCanonicalDecl();
  assert(Stack.size() > 1 && "Data-sharing attributes stack is empty");

  // For any explicit clause, climb effective directives within a combined
  // directive until we find the one on which it's allowed.  We have to climb
  // because we parse and "act on" all explicit clauses before we pop the
  // DSAStack for the inner effective directive.  This approach is derived from
  // OpenACC 2.6 sec. 2.11 lines 1534-1537:
  //
  // "Any of the parallel or loop clauses valid in a parallel region may
  // appear. The private and reduction clauses, which can appear on both a
  // parallel construct and a loop construct, are treated on a parallel loop
  // construct as if they appeared on the loop construct."
  //
  // TODO: Currently there is one exception: we add reductions to all effective
  // directives.  See todo in Sema::ActOnOpenACCParallelLoopDirective for
  // details.
  //
  // We always generate any implicit clause when the effective directive to
  // which it applies is at the top of the stack, so we don't need to climb to
  // find the right place to apply it.  Specifically, skipping the climb for
  // implicit clauses handles shared, which is not a real clause and so isn't
  // allowed explicitly on any directive.
  auto I = Stack.rbegin();
  for (int J = 0; J < StartLevel; ++J)
    I = std::next(I);
  if (!IsImplicit) {
    auto E = Stack.rend();
    while (I != E && !isAllowedClauseForDirective(I->EffectiveDKind, CKind)) {
      assert(I->RealDKind == ACCD_unknown && I->EffectiveDKind != ACCD_unknown
             && "expected combined directive when clause isn't allowed on"
                " effective directive");
      I = std::next(I);
      ++StartLevel;
    }
    assert(I != E && isAllowedClauseForDirective(I->EffectiveDKind, CKind) &&
           "expected clause to be allowed for directive");
  }

  // Complain if a variable appears in more than one (explicit) data
  // sharing clause on the same directive.
  //
  // In the case of a combined directive, because we've climbed, some
  // clauses that might appear to conflict don't.  For example, firstprivate
  // and private on acc parallel loop do not conflict because OpenACC 2.6
  // says firstprivate applies to the effective acc parallel and private
  // applies to the effective acc loop.
  //
  // The OpenACC 2.6 spec doesn't say, as far as I know, that a variable
  // cannot be a reduction variable and be either private or firstprivate on
  // the same effective construct.  However, a reduction implies visibility
  // at the beginning and end of the construct, so private and firstprivate
  // don't make sense.  The OpenMP implementation also has this restriction.
  DSAStackTy::DSAVarData DVar;
  if (I->SharingMap.count(VD))
    DVar = I->SharingMap[VD];
  if (DVar.CKind != ACCC_unknown) {
    if (DVar.CKind != CKind) {
      SemaRef.Diag(E->getExprLoc(), diag::err_acc_wrong_dsa)
          << getOpenACCClauseName(DVar.CKind) << getOpenACCClauseName(CKind);
      if (CopiedGangReduction)
        SemaRef.Diag(getConstructLoc(), diag::note_acc_gang_reduction_apply)
            << getOpenACCDirectiveName(getRealDirective());
      ReportOriginalDSA(SemaRef, this, VD, DVar);
      return true;
    } else if (CKind == ACCC_reduction) {
      SemaRef.Diag(E->getExprLoc(), diag::err_acc_conflicting_reduction)
          << (DVar.ReductionId.getName() == ReductionId.getName())
          << ACCReductionClause::printReductionOperatorToString(ReductionId)
          << VD;
      if (CopiedGangReduction)
        SemaRef.Diag(getConstructLoc(), diag::note_acc_gang_reduction_apply)
            << getOpenACCDirectiveName(getRealDirective());
      SemaRef.Diag(DVar.RefExpr->getExprLoc(),
                   diag::note_acc_previous_reduction)
          << ACCReductionClause::printReductionOperatorToString(
              DVar.ReductionId);
      return true;
    }
  }

  auto &Data = I->SharingMap[VD];
  assert(Data.CKind == ACCC_unknown || (CKind == Data.CKind));
  Data.CKind = CKind;
  Data.RefExpr = E;
  Data.ReductionId = ReductionId;

  // Handle that one exception mentioned in the todo above.
  if (CKind == ACCC_reduction && I->RealDKind == ACCD_unknown)
    return addDSA(VD, E, CKind, IsImplicit, ReductionId, CopiedGangReduction,
                  StartLevel + 1);

  return false;
}

DSAStackTy::DSAVarData DSAStackTy::getImplicitDSA(VarDecl *VD) {
  VD = VD->getCanonicalDecl();
  DSAVarData DVar;
  DVar.RefExpr = nullptr;
  if (getLoopControlVariables().count(VD)) {
    ACCPartitioningKind LoopKind = getLoopPartitioning();
    // OpenACC 2.6 [2.6.1]:
    //   "The loop variable in a C for statement [...] that is associated
    //   with a loop directive is predetermined to be private to each thread
    //   that will execute each iteration of the loop."
    //
    //   If the loop control variable is assigned but not defined in the for
    //   init, we need to specify a data sharing clause.  The OpenACC spec does
    //   not appear to clarify the end of the scope of the private copy, so we
    //   make what appears to be the intuitive choices for each case below.
    if (LoopKind.hasGangPartitioning() || LoopKind.hasWorkerPartitioning() ||
        LoopKind.hasVectorPartitioning())
      // Private is consistent with OpenMP in all cases here except vector
      // partitioning.  That is, OpenMP simd makes it predetermined linear,
      // which has lastprivate-like semantics.  However, for consistency, we
      // assume the intuitive semantics of private in all cases here: the
      // private copy goes out of scope at the end of the loop.
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
  else if (VD->getType()->isScalarType() &&
           isOpenACCParallelDirective(getEffectiveDirective()))
    DVar.CKind = ACCC_firstprivate;
  else
    DVar.CKind = ACCC_shared;
  return DVar;
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

void Sema::StartOpenACCDSABlock(OpenACCDirectiveKind RealDKind,
                                SourceLocation Loc) {
  switch (RealDKind) {
  case ACCD_parallel:
  case ACCD_loop:
    DSAStack->push(RealDKind, RealDKind, Loc);
    break;
  case ACCD_parallel_loop:
    DSAStack->push(RealDKind, ACCD_parallel, Loc);
    DSAStack->push(ACCD_unknown, ACCD_loop, Loc);
    break;
  case ACCD_unknown:
    llvm_unreachable("expected acc directive");
  }
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
  case ACCC_num_workers:
  case ACCC_vector_length:
  case ACCC_seq:
  case ACCC_independent:
  case ACCC_auto:
  case ACCC_gang:
  case ACCC_worker:
  case ACCC_vector:
  case ACCC_collapse:
  case ACCC_unknown:
    llvm_unreachable("Clause is not allowed.");
  }
  return Res;
}

void Sema::EndOpenACCClause() {
}

void Sema::EndOpenACCDSABlock() {
  // For a combined directive, the first DSAStack->pop() happens after the
  // inner effective directive is "acted upon" (AST node is constructed), so
  // this is the second DSAStack->pop(), which happens after the entire
  // combined directive is acted upon.  However, if there was an error, we
  // need to pop the entire combined directive.
  for (OpenACCDirectiveKind DKind = ACCD_unknown; DKind == ACCD_unknown;
       DSAStack->pop())
    DKind = DSAStack->getEffectiveDirective();
  DiscardCleanupsInEvaluationContext();
  PopExpressionEvaluationContext();
}

namespace {
struct ReductionVar {
  ACCReductionClause * const C;
  DeclRefExpr * const RE;
  ReductionVar(ACCReductionClause *C, DeclRefExpr *RE) : C(C), RE(RE) {}
};
class DSAAttrChecker : public StmtVisitor<DSAAttrChecker> {
  typedef StmtVisitor<DSAAttrChecker> BaseVisitor;
  DSAStackTy *Stack;
  llvm::SmallVector<Expr *, 8> ImplicitShared;
  llvm::SmallVector<Expr *, 8> ImplicitPrivate;
  llvm::SmallVector<Expr *, 8> ImplicitFirstprivate;
  llvm::DenseSet<VarDecl *> ImplicitPrivacyVarDecls;
  llvm::SmallVector<ReductionVar, 8> ImplicitGangReductions;
  llvm::DenseMap<VarDecl *, ACCReductionClause *>  ImplicitGangReductionMap;
  llvm::DenseSet<Decl *> LocalDefinitions;
  void pruneImplicitPrivacyClause(llvm::SmallVector<Expr *, 8> &VarDecls) {
    auto Write = VarDecls.begin();
    for (auto Read = VarDecls.begin(), End = VarDecls.end(); Read != End;
         ++Read) {
      auto *DE = dyn_cast_or_null<DeclRefExpr>((*Read)->IgnoreParens());
      assert(DE && "OpenACC implicit privacy clause for non-DeclRefExpr");
      auto *VD = dyn_cast_or_null<VarDecl>(DE->getDecl());
      assert(VD && "OpenACC implicit privacy clause for non-VarDecl");
      if (!ImplicitGangReductionMap.count(VD))
        *Write++ = *Read;
    }
    VarDecls.resize(Write - VarDecls.begin());
  }

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
      if (DVar.CKind != ACCC_unknown ||
          !ImplicitPrivacyVarDecls.insert(VD).second)
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
  void VisitACCExecutableDirective(ACCExecutableDirective *D) {
    // For acc parallel directive, record gang reductions from descendant acc
    // loop directives.
    if (isOpenACCParallelDirective(Stack->getEffectiveDirective()) &&
        isOpenACCLoopDirective(D->getDirectiveKind()) &&
        D->hasClausesOfKind<ACCGangClause>()) {
      for (ACCReductionClause *C : D->getClausesOfKind<ACCReductionClause>()) {
        for (Expr *VR : C->varlists()) {
          DeclRefExpr *DRE = cast<DeclRefExpr>(VR);
          VarDecl *VD = cast<VarDecl>(DRE->getDecl())->getCanonicalDecl();

          // Skip variables declared gang-local.
          if (LocalDefinitions.count(VD))
            continue;

          // Skip variables that have gang-local private copies or that already
          // have the same reduction.
          auto DVar = Stack->getTopDSA(VD);
          switch (DVar.CKind) {
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
            llvm_unreachable("unexpected as explicit data attribute clause");
          case ACCC_private:
          case ACCC_firstprivate:
            // The variable is gang-local at the acc loop due to an explicit
            // private or firstprivate at the acc parallel.
            continue;
          case ACCC_reduction: {
            // Skip if we have the same reduction explicitly specified on the
            // acc parallel.
            if (DVar.ReductionId.getName() == C->getNameInfo().getName())
              continue;
            // We have a conflicting reduction operator.  Record in
            // ImplicitGangReductions to produce a diagnostic later.  Don't
            // record in ImplicitGangReductionMap, where it could suppress
            // diagnostics for additional conflicts.
            break;
          }
          case ACCC_unknown: {
            // Skip if we have the same reduction implicitly specified by
            // another acc loop.
            auto COld = ImplicitGangReductionMap.try_emplace(VD, C);
            if (!COld.second && COld.first->second->getNameInfo().getName() ==
                                C->getNameInfo().getName())
              continue;
            // We have a conflicting implicit reduction operator.  Same
            // behavior as for the explicit case.
            break;
          }
          }

          // Record the reduction.  Conflicting reduction operators will be
          // reported when we generate the implicit clause.
          ImplicitGangReductions.emplace_back(C, DRE);
        }
      }
    }
    for (auto *C : D->children()) {
      if (C)
        Visit(C);
    }
  }
  void VisitStmt(Stmt *S) {
    for (Stmt *C : S->children()) {
      if (C)
        Visit(C);
    }
  }

  // Remove variables for implicit gang reductions from other implicit clauses,
  // over which they have priority, and with which they would conflict if we
  // kept both.  This is necessary because, during this single subtree
  // traversal, we add privacy clauses upon any use of a variable, but we might
  // discover a nested acc loop with a gang reduction afterward.
  void pruneImplicitPrivacyClauses() {
    pruneImplicitPrivacyClause(ImplicitShared);
    pruneImplicitPrivacyClause(ImplicitPrivate);
    pruneImplicitPrivacyClause(ImplicitFirstprivate);
  }
  ArrayRef<Expr *> getImplicitShared() { return ImplicitShared; }
  ArrayRef<Expr *> getImplicitPrivate() { return ImplicitPrivate; }
  ArrayRef<Expr *> getImplicitFirstprivate() { return ImplicitFirstprivate; }
  ArrayRef<ReductionVar> getImplicitGangReductions() {
    return ImplicitGangReductions;
  }

  DSAAttrChecker(DSAStackTy *S) : Stack(S) {}
};
} // namespace

bool Sema::ActOnOpenACCRegionStart(
    OpenACCDirectiveKind DKind, ArrayRef<ACCClause *> Clauses, Scope *CurScope,
    SourceLocation StartLoc, SourceLocation EndLoc) {
  bool ErrorFound = false;
  // Check directive nesting.
  SourceLocation ParentLoc;
  OpenACCDirectiveKind ParentDKind =
      DSAStack->getRealParentDirective(ParentLoc);
  if (!isAllowedParentForDirective(DKind, ParentDKind)) {
    // The OpenACC 2.6 spec doesn't say that an acc parallel or acc parallel
    // loop cannot be nested within another acc construct, but gcc 7.3.0 and
    // pgcc 18.4-0 don't permit that for simple cases I've tried.
    if (ParentDKind == ACCD_unknown)
      Diag(StartLoc, diag::err_acc_orphaned_directive)
          << getOpenACCDirectiveName(DKind);
    else {
      Diag(StartLoc, diag::err_acc_directive_bad_nesting)
          << getOpenACCDirectiveName(ParentDKind)
          << getOpenACCDirectiveName(DKind);
      Diag(ParentLoc, diag::note_acc_enclosing_directive)
          << getOpenACCDirectiveName(ParentDKind);
    }
    ErrorFound = true;
  }
  if (isOpenACCLoopDirective(DKind)) {
    ACCPartitioningKind LoopKind;
    // Set implicit partitionability.
    //
    // OpenACC 2.6, sec. 2.9.9:
    // "When the parent compute construct is a parallel construct, the
    // independent clause is implied on all loop constructs without a seq or
    // auto clause."
    // TODO: This will need to be adjusted when not enclosed in a parallel
    // directive.
    LoopKind.setIndependentImplicit();

    // Set explicit clauses.
    for (ACCClause *C : Clauses) {
      OpenACCClauseKind CKind = C->getClauseKind();
      if (CKind == ACCC_seq)
        LoopKind.setSeqExplicit();
      else if (CKind == ACCC_auto)
        LoopKind.setAutoExplicit();
      else if (CKind == ACCC_independent)
        LoopKind.setIndependentExplicit();
      else if (CKind == ACCC_gang)
        LoopKind.setGang();
      else if (CKind == ACCC_worker)
        LoopKind.setWorker();
      else if (CKind == ACCC_vector)
        LoopKind.setVector();
    }

    // Validate partitioning level against parent.
    OpenACCDirectiveKind ParentDKind;
    SourceLocation ParentLoopLoc;
    ACCPartitioningKind ParentLoopKind =
        DSAStack->getParentLoopPartitioning(ParentDKind, ParentLoopLoc);
    if (ParentLoopKind.hasGangPartitioning() && LoopKind.hasGangClause()) {
      // OpenACC 2.6, sec. 2.9.2:
      // "The region of a loop with the gang clause may not contain another
      // loop with the gang clause unless within a nested compute region."
      Diag(StartLoc, diag::err_acc_loop_bad_nested_partitioning)
          << getOpenACCDirectiveName(ParentDKind) << "gang"
          << getOpenACCDirectiveName(DKind) << "gang";
      Diag(ParentLoopLoc, diag::note_acc_enclosing_directive)
          << getOpenACCDirectiveName(ParentDKind);
      ErrorFound = true;
    }
    else if (ParentLoopKind.hasWorkerPartitioning() &&
             (LoopKind.hasGangClause() || LoopKind.hasWorkerClause())) {
      // OpenACC 2.6, sec. 2.9.3:
      // "The region of a loop with the worker clause may not contain a loop
      // with a gang or worker clause unless within a nested compute region."
      Diag(StartLoc, diag::err_acc_loop_bad_nested_partitioning)
          << getOpenACCDirectiveName(ParentDKind) << "worker"
          << getOpenACCDirectiveName(DKind)
          << (LoopKind.hasGangClause() ? "gang" : "worker");
      Diag(ParentLoopLoc, diag::note_acc_enclosing_directive)
          << getOpenACCDirectiveName(ParentDKind);
      ErrorFound = true;
    }
    else if (ParentLoopKind.hasVectorPartitioning() &&
             (LoopKind.hasGangClause() || LoopKind.hasWorkerClause() ||
              LoopKind.hasVectorClause())) {
      // OpenACC 2.6, sec. 2.9.4:
      // "The region of a loop with the vector clause may not contain a loop
      // with the gang, worker, or vector clause unless within a nested compute
      // region."
      Diag(StartLoc, diag::err_acc_loop_bad_nested_partitioning)
          << getOpenACCDirectiveName(ParentDKind) << "vector"
          << getOpenACCDirectiveName(DKind)
          << (LoopKind.hasGangClause()
                  ? "gang"
                  : LoopKind.hasWorkerClause() ? "worker" : "vector");
      Diag(ParentLoopLoc, diag::note_acc_enclosing_directive)
          << getOpenACCDirectiveName(ParentDKind);
      ErrorFound = true;
    }

    // TODO: For now, we prescriptively map auto to sequential execution, but
    // obviously an OpenACC compiler is meant to sometimes determine that
    // independent is possible.
    if (LoopKind.hasAuto())
      LoopKind.setSeqComputed();

    // Record partitioning on stack.
    DSAStack->setLoopPartitioning(LoopKind);
    // Record for parent construct any worker partitioning here.
    if (LoopKind.hasWorkerPartitioning())
      DSAStack->setWorkerPartitioning();
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

  // Our strategy for combined directives is to "act on" the clauses (already
  // done) as if they're on the same directive so that we can catch conflicts
  // (for data sharing attributes, for example) that wouldn't arise if they
  // were on separate nested directives.  Then we act on the directive as if
  // it's two separate nested directives, acting on the effective innermost
  // directive first.  The AST node for the combined directive contains AST
  // nodes for the effective nested directives, facilitating translation to
  // OpenMP.
  if (isOpenACCCombinedDirective(DKind)) {
    switch (DKind) {
    case ACCD_parallel_loop:
      return ActOnOpenACCParallelLoopDirective(Clauses, AStmt, StartLoc,
                                               EndLoc);
    case ACCD_parallel:
    case ACCD_loop:
    case ACCD_unknown:
      llvm_unreachable("expected combined OpenACC directive");
    }
  }

  llvm::SmallVector<ACCClause *, 8> ClausesWithImplicit;
  bool ErrorFound = false;
  ClausesWithImplicit.append(Clauses.begin(), Clauses.end());
  ACCPartitioningKind LoopKind = DSAStack->getLoopPartitioning();
  if (LoopKind.hasIndependentImplicit()) {
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
    // execution, probably with a warning, or is an error better for that
    // case?
  }
  if (AStmt) {
    // Check default data sharing attributes for referenced variables.
    DSAAttrChecker DSAChecker(DSAStack);
    DSAChecker.Visit(AStmt);
    DSAChecker.pruneImplicitPrivacyClauses();
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
    for (ReductionVar RV : DSAChecker.getImplicitGangReductions()) {
      ACCClause *Implicit = ActOnOpenACCReductionClause(
          RV.RE, SourceLocation(), SourceLocation(), SourceLocation(),
          SourceLocation(), RV.C->getNameInfo(), true);
      if (!Implicit)
        ErrorFound = true;
      else
        ClausesWithImplicit.push_back(Implicit);
    }
  }

  switch (DKind) {
  case ACCD_parallel:
    Res = ActOnOpenACCParallelDirective(
        ClausesWithImplicit, AStmt, StartLoc, EndLoc,
        DSAStack->getNestedWorkerPartitioning());
    break;
  case ACCD_loop:
    Res = ActOnOpenACCLoopDirective(
        ClausesWithImplicit, AStmt, StartLoc, EndLoc,
        DSAStack->getLoopControlVariables(), DSAStack->getLoopPartitioning());
    break;
  case ACCD_parallel_loop:
  case ACCD_unknown:
    llvm_unreachable("expected non-combined OpenACC directive");
  }

  if (ErrorFound)
    return StmtError();
  return Res;
}

StmtResult Sema::ActOnOpenACCParallelDirective(
    ArrayRef<ACCClause *> Clauses, Stmt *AStmt, SourceLocation StartLoc,
    SourceLocation EndLoc, bool NestedWorkerPartitioning) {
  if (!AStmt)
    return StmtError();

  getCurFunction()->setHasBranchProtectedScope();

  return ACCParallelDirective::Create(Context, StartLoc, EndLoc, Clauses,
                                      AStmt, NestedWorkerPartitioning);
}

void Sema::ActOnOpenACCLoopInitialization(SourceLocation ForLoc, Stmt *Init) {
  assert(getLangOpts().OpenACC && "OpenACC is not active.");
  assert(Init && "Expected loop in canonical form.");
  if (DSAStack->getAssociatedLoopsParsed() < DSAStack->getAssociatedLoops() &&
      isOpenACCLoopDirective(DSAStack->getEffectiveDirective())) {
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
    DSAStack->incAssociatedLoopsParsed();
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
    SourceLocation EndLoc, const llvm::DenseSet<VarDecl *> &LCVars,
    ACCPartitioningKind Partitioning) {
  if (!AStmt)
    return StmtError();

  // OpenACC 2.7 sec. 2.9.1, lines 1441-1442:
  // "The collapse clause is used to specify how many tightly nested loops are
  // associated with the loop construct."
  // Complain if there aren't enough.
  Stmt *LoopStmt = AStmt;
  for (unsigned LoopI = 0, LoopCount = DSAStack->getAssociatedLoops();
       LoopI < LoopCount; ++LoopI)
  {
    LoopStmt = LoopStmt->IgnoreContainers();
    auto *LoopFor = dyn_cast_or_null<ForStmt>(LoopStmt);
    if (!LoopFor) {
      Diag(LoopStmt->getBeginLoc(), diag::err_acc_not_for)
          << getOpenACCDirectiveName(DSAStack->getRealDirective());
      auto CollapseClauses =
          ACCExecutableDirective::getClausesOfKind<ACCCollapseClause>(Clauses);
      if (CollapseClauses.begin() != CollapseClauses.end()) {
        Expr *E = CollapseClauses.begin()->getCollapse();
        Diag(E->getExprLoc(), diag::note_acc_collapse_expr)
            << E->getSourceRange();
      }
      return StmtError();
    }
    LoopStmt = LoopFor->getBody();
    assert(LoopStmt);
  }

  // Complain if we have a reduction on an acc loop control variable.
  for (ACCClause *C : Clauses) {
    if (ACCReductionClause *RC = dyn_cast_or_null<ACCReductionClause>(C)) {
      for (Expr *VR : RC->varlists()) {
        VarDecl *VD = cast<VarDecl>(cast<DeclRefExpr>(VR)->getDecl())
                      ->getCanonicalDecl();
        if (DSAStack->getLoopControlVariables().count(VD)) {
           Diag(VR->getEndLoc(), diag::err_acc_reduction_on_loop_control_var)
               << VD->getName() << VR->getSourceRange();
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

  return ACCLoopDirective::Create(Context, StartLoc, EndLoc, Clauses, AStmt,
                                  LCVars, Partitioning);
}

StmtResult Sema::ActOnOpenACCParallelLoopDirective(
    ArrayRef<ACCClause *> Clauses, Stmt *AStmt, SourceLocation StartLoc,
    SourceLocation EndLoc) {
  // OpenACC 2.6 sec. 2.11 lines 1534-1537:
  // "The associated structured block is the loop which must immediately follow
  // the directive. Any of the parallel or loop clauses valid in a parallel
  // region may appear. The private and reduction clauses, which can appear on
  // both a parallel construct and a loop construct, are treated on a parallel
  // loop construct as if they appeared on the loop construct."
  //
  // TODO: OpenACC 2.7 will likely specify that a reduction on an acc parallel
  // loop implies a copy clause, thus a gang-shared reduction variable, and thus
  // a gang-like reduction (acc loop seq/worker/vector reduction on gang-shared
  // variables become gang-like reductions).  The main difference between that
  // behavior and the behavior achieved by copying the reduction to both the
  // effective acc parallel directive and the effective acc loop directive is
  // whether the reduction variable is gang-shared or gang-private within the
  // effective acc parallel but outside the effective acc loop, but the user can
  // only add code within the effective acc loop in the case of an acc parallel
  // loop directive.  Another difference is that we currently consider a
  // reduction clause to conflict with a firstprivate or private clause, so
  // adding an additional reduction clause could create a conflict, but that's
  // really a separate bug to be fixed in clacc (or maybe the spec just doesn't
  // make sense there).  Because we have not yet implemented the copy clause,
  // implicit or explicit, it's easiest for us to copy the reduction clause to
  // both the effective acc parallel and acc loop directives.  Matching logic
  // also appears in DSAStackTy::addDSA (see todo there).
  SmallVector<ACCClause *, 5> ParallelClauses;
  SmallVector<ACCClause *, 5> LoopClauses;
  for (ACCClause *C : Clauses) {
    bool AddLoopClause = false;
    bool AddParallelClause = false;
    if (C->getClauseKind() == ACCC_reduction) {
      AddLoopClause = true;
      AddParallelClause = true;
    }
    else if (isAllowedClauseForDirective(ACCD_loop, C->getClauseKind()))
      AddLoopClause = true;
    else
      AddParallelClause = true;
    if (AddLoopClause) {
      assert(isAllowedClauseForDirective(ACCD_loop, C->getClauseKind()) &&
             "expected clause to be allowed on acc loop");
      LoopClauses.push_back(C);
    }
    if (AddParallelClause) {
      assert(isAllowedClauseForDirective(ACCD_parallel, C->getClauseKind()) &&
             "expected clause to be allowed on acc parallel");
      ParallelClauses.push_back(C);
    }
  }

  // Build the effective directives.
  StmtResult Res = ActOnOpenACCExecutableDirective(ACCD_loop, LoopClauses,
                                                   AStmt, StartLoc, EndLoc);
  // The second DSAStack->pop() happens in EndOpenACCDSABlock.
  DSAStack->pop();
  if (Res.isInvalid())
    return StmtError();
  ACCLoopDirective *LoopDir = cast<ACCLoopDirective>(Res.get());
  Res = ActOnOpenACCExecutableDirective(ACCD_parallel, ParallelClauses,
                                        LoopDir, StartLoc, EndLoc);
  if (Res.isInvalid())
    return StmtError();
  ACCParallelDirective *ParallelDir = cast<ACCParallelDirective>(Res.get());

  // Build the real directive.
  return ACCParallelLoopDirective::Create(Context, StartLoc, EndLoc, Clauses,
                                          AStmt, ParallelDir);
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
  case ACCC_num_workers:
    Res = ActOnOpenACCNumWorkersClause(Expr, StartLoc, LParenLoc, EndLoc);
    break;
  case ACCC_vector_length:
    Res = ActOnOpenACCVectorLengthClause(Expr, StartLoc, LParenLoc, EndLoc);
    break;
  case ACCC_collapse:
    Res = ActOnOpenACCCollapseClause(Expr, StartLoc, LParenLoc, EndLoc);
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
    auto *DE = dyn_cast_or_null<DeclRefExpr>(RefExpr->IgnoreParens());
    assert(DE && "OpenACC implicit shared clause for non-DeclRefExpr");
    auto *VD = dyn_cast_or_null<VarDecl>(DE->getDecl());
    assert(VD && "OpenACC implicit shared clause for non-VarDecl");

    if (!DSAStack->addDSA(VD, RefExpr->IgnoreParens(), ACCC_shared, true))
      Vars.push_back(RefExpr->IgnoreParens());
    else
      // Assert that the variable does not appear in an explicit data sharing
      // clause on the same directive.  If the OpenACC spec grows an explicit
      // shared clause one day, this assert should be removed.
      llvm_unreachable("implicit shared clause unexpectedly generated for"
                       " variable in explicit data sharing clause");
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
  bool IsImplicitClause = StartLoc.isInvalid();
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

    // The OpenACC 2.5 spec doesn't say, as far as I know, that a const
    // variable cannot be private.  However, you can never initialize the
    // private version of such a variable, and OpenMP does have this
    // restriction.
    // TODO: Should this be isConstant?
    if (VD->getType().isConstQualified()) {
      Diag(ELoc, diag::err_acc_const_private);
      Diag(VD->getLocation(), diag::note_acc_const_private)
          << VD;
      continue;
    }

    if (!DSAStack->addDSA(VD, RefExpr->IgnoreParens(), ACCC_private,
                          IsImplicitClause))
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
  bool IsImplicitClause = StartLoc.isInvalid();
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

    if (!DSAStack->addDSA(VD, RefExpr->IgnoreParens(), ACCC_firstprivate,
                          IsImplicitClause))
      Vars.push_back(RefExpr->IgnoreParens());
  }

  if (Vars.empty())
    return nullptr;

  return ACCFirstprivateClause::Create(Context, StartLoc, LParenLoc, EndLoc,
                                       Vars);
}

ACCClause *Sema::ActOnOpenACCReductionClause(
    ArrayRef<Expr *> VarList, SourceLocation StartLoc, SourceLocation LParenLoc,
    SourceLocation ColonLoc, SourceLocation EndLoc,
    const DeclarationNameInfo &ReductionId, bool CopiedGangReduction) {
  DeclarationName DN = ReductionId.getName();
  OverloadedOperatorKind OOK = DN.getCXXOverloadedOperator();
  SmallVector<Expr *, 8> Vars;
  bool IsImplicitClause = StartLoc.isInvalid();

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
      Diag(EndLoc, diag::err_acc_unknown_reduction_operator);
      return nullptr;
    }
    RedOpType = RedOpRealOrPointer;
    break;
  }
  }

  bool FirstIter = true;
  for (Expr *RefExpr : VarList) {
    assert(RefExpr && "nullptr expr in OpenACC reduction clause.");
    FirstIter = false;
    SourceLocation ELoc;
    SourceRange ERange;
    Expr *SimpleRefExpr = RefExpr;
    VarDecl *VD = getPrivateItem(*this, SimpleRefExpr, ELoc, ERange);
    if (!VD)
      continue;

    QualType Type = VD->getType();

    // The OpenACC 2.6 spec doesn't say, as far as I know, that a private
    // variable must have a complete type.  However, you cannot copy data if
    // it doesn't have a size, and OpenMP does have this restriction.
    if (RequireCompleteType(ELoc, Type,
                            diag::err_acc_reduction_incomplete_type))
      continue;

    // The OpenACC 2.6 spec doesn't say, as far as I know, that a const
    // variable cannot be private.  However, you can never initialize the
    // private version of such a variable, and OpenMP does have this
    // restriction.
    if (Type.isConstant(Context)) {
      Diag(ELoc, diag::err_acc_const_reduction_list_item) << ERange;
      bool IsDecl = VD->isThisDeclarationADefinition(Context) ==
                        VarDecl::DeclarationOnly;
      Diag(VD->getLocation(),
           IsDecl ? diag::note_previous_decl : diag::note_defined_here)
          << VD;
      continue;
    }

    // Is the reduction variable type reasonable for the reduction operator?
    //
    // TODO: When adding support for arrays, this should be on the base element
    // type of the array.  See the OpenMP implementation for an example.
    unsigned DiagN = diag::NUM_BUILTIN_SEMA_DIAGNOSTICS;
    switch (RedOpType) {
    case RedOpInteger:
      if (!Type->isIntegerType())
        DiagN = diag::err_acc_reduction_not_integer_type;
      break;
    case RedOpArithmetic:
      if (!Type->isArithmeticType())
        DiagN = diag::err_acc_reduction_not_arithmetic_type;
      break;
    case RedOpRealOrPointer:
      if (!Type->isRealType() && !Type->isPointerType())
        DiagN = diag::err_acc_reduction_not_real_or_pointer_type;
      break;
    case RedOpInvalid:
      llvm_unreachable("expected reduction operand type");
    }
    if (DiagN != diag::NUM_BUILTIN_SEMA_DIAGNOSTICS) {
      Diag(ELoc, DiagN)
          << ACCReductionClause::printReductionOperatorToString(ReductionId);
      bool IsDecl = VD->isThisDeclarationADefinition(Context) ==
                        VarDecl::DeclarationOnly;
      Diag(VD->getLocation(),
             IsDecl ? diag::note_previous_decl : diag::note_defined_here)
          << VD;
      continue;
    }

    // Record reduction item.
    if (!DSAStack->addDSA(VD, RefExpr->IgnoreParens(), ACCC_reduction,
                          IsImplicitClause, ReductionId, CopiedGangReduction))
      Vars.push_back(RefExpr);
  }
  if (Vars.empty())
    return nullptr;
  return ACCReductionClause::Create(Context, StartLoc, LParenLoc, ColonLoc,
                                    EndLoc, Vars, ReductionId);
}

enum PosIntResult {PosIntConst, PosIntNonConst, PosIntError};

static PosIntResult IsPositiveIntegerValue(Expr *&ValExpr, Sema &SemaRef,
                                           OpenACCClauseKind CKind,
                                           bool ErrorIfNotConst) {
  SourceLocation Loc = ValExpr->getExprLoc();
  // This uses err_omp_* diagnostics, but none currently mention OpenMP or
  // OpenMP-specific constructs, so they should work fine for OpenACC.
  ExprResult Value =
      SemaRef.PerformOpenMPImplicitIntegerConversion(Loc, ValExpr);
  if (Value.isInvalid())
    return PosIntError;

  ValExpr = Value.get();
  // The expression must evaluate to a non-negative integer value.
  llvm::APSInt Result;
  if (!ValExpr->isIntegerConstantExpr(Result, SemaRef.Context)) {
    if (ErrorIfNotConst) {
      SemaRef.Diag(Loc, diag::err_acc_clause_not_ice)
          << getOpenACCClauseName(CKind) << ValExpr->getSourceRange();
      return PosIntError;
    }
    return PosIntNonConst;
  } else if (!Result.isStrictlyPositive()) {
    SemaRef.Diag(Loc, diag::err_acc_clause_not_positive_ice)
        << getOpenACCClauseName(CKind) << ValExpr->getSourceRange();
    return PosIntError;
  }

  return PosIntConst;
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
  case ACCC_num_workers:
  case ACCC_vector_length:
  case ACCC_collapse:
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
  if (PosIntError == IsPositiveIntegerValue(NumGangs, *this, ACCC_num_gangs,
                                            false))
    return nullptr;
  return new (Context) ACCNumGangsClause(NumGangs, StartLoc, LParenLoc,
                                         EndLoc);
}

ACCClause *Sema::ActOnOpenACCNumWorkersClause(Expr *NumWorkers,
                                              SourceLocation StartLoc,
                                              SourceLocation LParenLoc,
                                              SourceLocation EndLoc) {
  // OpenMP says num_threads must evaluate to a positive integer value.
  // OpenACC doesn't specify such a restriction that I see for num_workers, but
  // it seems reasonable.
  if (PosIntError == IsPositiveIntegerValue(NumWorkers, *this,
                                            ACCC_num_workers, false))
    return nullptr;
  return new (Context) ACCNumWorkersClause(NumWorkers, StartLoc, LParenLoc,
                                           EndLoc);
}

ACCClause *Sema::ActOnOpenACCVectorLengthClause(Expr *VectorLength,
                                                SourceLocation StartLoc,
                                                SourceLocation LParenLoc,
                                                SourceLocation EndLoc) {
  // OpenMP says simdlen must be a constant positive integer.  OpenACC doesn't
  // specify such a restriction for vector_length.  It seems reasonable for it
  // to have to be a positive integer, so we require that.  However, according
  // to our users, some existing OpenACC applications do use non-constant
  // expressions here.  For that case, we ignore the clause, as permitted by
  // OpenACC 2.7 sec. 2.5.10 L805-806:
  //
  // "The implementation may use a different value than specified based on
  // limitations imposed by the target architecture."
  //
  // However, the translation to OpenMP is careful to evaluate the expression
  // if it might have side effects.
  //
  PosIntResult Res = IsPositiveIntegerValue(VectorLength, *this,
                                            ACCC_vector_length, false);
  if (Res == PosIntError)
    return nullptr;
  if (Res == PosIntNonConst)
    Diag(VectorLength->getExprLoc(), diag::warn_acc_clause_ignored_not_ice)
        << getOpenACCClauseName(ACCC_vector_length)
        << VectorLength->getSourceRange();
  return new (Context) ACCVectorLengthClause(VectorLength, StartLoc, LParenLoc,
                                             EndLoc);
}

ACCClause *Sema::ActOnOpenACCCollapseClause(Expr *Collapse,
                                            SourceLocation StartLoc,
                                            SourceLocation LParenLoc,
                                            SourceLocation EndLoc) {
  if (PosIntError == IsPositiveIntegerValue(Collapse, *this, ACCC_collapse,
                                            true))
    return nullptr;
  DSAStack->setAssociatedLoops(
      Collapse->EvaluateKnownConstInt(Context).getExtValue());
  return new (Context) ACCCollapseClause(Collapse, StartLoc, LParenLoc,
                                         EndLoc);
}

namespace {
class TransformACCToOMP : public TransformContext<TransformACCToOMP> {
  typedef TransformContext<TransformACCToOMP> BaseTransform;
  /// Before translating the associated statement of an acc parallel directive,
  /// some of the following variables might be set.  They are set back to
  /// nullptr after the translation of the associated statement.  Nested acc
  /// parallel directives are not permitted, so we don't need a stack of these
  /// variables.
  ///
  /// If the acc parallel directive has a num_workers or constant-expression
  /// vector_length clause, then NumWorkersExpr or VectorLengthExpr,
  /// respectively, is set to its value.
  ///
  /// If the acc parallel directive has a num_workers clause with a
  /// non-constant expression and it has a (separate or combined with this
  /// directive) nested acc loop directive with worker partitioning, then
  /// NumWorkersVarDecl is set to the declaration of a constant variable
  /// generated for the sake of OpenMP and initialized with NumWorkersExpr.
  VarDecl *NumWorkersVarDecl = nullptr;
  Expr *NumWorkersExpr = nullptr;
  Expr *VectorLengthExpr = nullptr;

  /// Before translating the associated statement of an OpenACC directive that
  /// translates to an OpenMP loop-related directive (acc loop seq, for
  /// example, does not), the following variable is set to the type of that
  /// OpenMP directive.  It is set back to its previous value after the
  /// translation of the associated statement.
  OpenMPDirectiveKind ParentLoopOMPKind = OMPD_unknown;

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
    void addPrivateDecl(SourceLocation RefStartLoc, SourceLocation RefEndLoc,
                        VarDecl *VD) {
      VD = VD->getCanonicalDecl();
      if (!Started) {
        Tx.getSema().ActOnStartOfCompoundStmt(false);
        Started = true;
      }
      // Record old transformation for VD.  If we've already done that, that
      // means VD appeared multiple times in private clauses.  Once is enough,
      // and we don't want to lose the old transformation.
      Decl *OldDeclTx = Tx.getDerived().TransformDecl(RefStartLoc, VD);
      if (!OldDeclTxs.try_emplace(VD, OldDeclTx).second)
        return;
      // Revert the old transformation for VD so we can create a new one.
      Tx.getDerived().transformedLocalDecl(VD, VD);
      // Transform Def and create a declaration statement for it.
      Decl *DPrivate = Tx.getDerived().TransformDefinition(RefStartLoc, VD,
                                                           /*DropInit*/true);
      StmtResult Res = Tx.getSema().ActOnDeclStmt(
          Tx.getSema().ConvertDeclToDeclGroup(DPrivate), RefStartLoc,
          RefEndLoc);
      if (Res.isInvalid())
        Err = true;
      else
        Stmts.push_back(Res.get());
    }
    VarDecl *addNewPrivateDecl(StringRef Name, QualType Ty, Expr *Init,
                               SourceLocation Loc) {
      if (!Started) {
        Tx.getSema().ActOnStartOfCompoundStmt(false);
        Started = true;
      }
      ASTContext &Context = Tx.getSema().getASTContext();
      DeclContext *DC = Tx.getSema().CurContext;
      IdentifierInfo *II = &Tx.getSema().PP.getIdentifierTable().get(Name);
      TypeSourceInfo *TInfo = Context.getTrivialTypeSourceInfo(Ty, Loc);
      VarDecl *VD = VarDecl::Create(Context, DC, Loc, Loc, II, Ty, TInfo,
                                    SC_None);
      Tx.getSema().AddInitializerToDecl(VD, Init, false);
      StmtResult Res = Tx.getSema().ActOnDeclStmt(
          Tx.getSema().ConvertDeclToDeclGroup(VD), VD->getBeginLoc(),
          VD->getEndLoc());
      if (Res.isInvalid())
        Err = true;
      else
        Stmts.push_back(Res.get());
      return VD;
    }
    void addUnusedExpr(Expr *E) {
      if (!Started) {
        Tx.getSema().ActOnStartOfCompoundStmt(false);
        Started = true;
      }
      ExprResult Res = Tx.getDerived().TransformExpr(E);
      if (Res.isInvalid()) {
        Err = true;
        return;
      }
      ASTContext &Context = Tx.getSema().getASTContext();
      SourceLocation Loc = E->getExprLoc();
      TypeSourceInfo *TInfo = Context.getTrivialTypeSourceInfo(Context.VoidTy,
                                                               Loc);
      Res = Tx.getSema().BuildCStyleCastExpr(Loc, TInfo, Loc, Res.get());
      if (Res.isInvalid()) {
        Err = true;
        return;
      }
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
      S = Tx.getSema().ActOnCompoundStmt(Stmts.front()->getBeginLoc(),
                                         Stmts.back()->getEndLoc(), Stmts,
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
    ConditionalCompoundStmtRAII EnclosingCompoundStmt(*this);
    ASTContext &Context = this->getSema().getASTContext();

    // Declare a num_workers variable in an enclosing compound statement, if
    // needed.
    assert(NumWorkersVarDecl == nullptr && NumWorkersExpr == nullptr &&
            VectorLengthExpr == nullptr &&
           "unexpected nested acc parallel directive");
    auto NumWorkersClauses = D->getClausesOfKind<ACCNumWorkersClause>();
    if (NumWorkersClauses.begin() != NumWorkersClauses.end()) {
      NumWorkersExpr = NumWorkersClauses.begin()->getNumWorkers();
      if (D->getNestedWorkerPartitioning()) {
        if (!NumWorkersExpr->isIntegerConstantExpr(Context))
          NumWorkersVarDecl = EnclosingCompoundStmt.addNewPrivateDecl(
              "__clang_acc_num_workers__",
              NumWorkersExpr->getType().withConst(), NumWorkersExpr,
              NumWorkersExpr->getBeginLoc());
      } else if (NumWorkersExpr->HasSideEffects(Context))
        EnclosingCompoundStmt.addUnusedExpr(NumWorkersExpr);
    }
    auto VectorLengthClauses = D->getClausesOfKind<ACCVectorLengthClause>();
    if (VectorLengthClauses.begin() != VectorLengthClauses.end()) {
      Expr *E = VectorLengthClauses.begin()->getVectorLength();
      if (E->isIntegerConstantExpr(Context))
        VectorLengthExpr = E;
      else if (E->HasSideEffects(Context))
        EnclosingCompoundStmt.addUnusedExpr(E);
    }

    // Start OpenMP DSA block.
    getSema().StartOpenMPDSABlock(OMPD_target_teams, DeclarationNameInfo(),
                                  /*CurScope=*/nullptr, D->getBeginLoc());

    // Transform OpenACC clauses.
    llvm::SmallVector<OMPClause *, 16> TClauses;
    size_t TClausesEmptyCount;
    size_t NumClausesAdded = 0;
    transformACCClauses(D, OMPD_target_teams, TClauses, TClausesEmptyCount);

    // Transform associated statement.
    StmtResult AssociatedStmt = transformACCAssociatedStmt(D, OMPD_target_teams,
                                                           TClauses);
    NumWorkersVarDecl = nullptr;
    NumWorkersExpr = nullptr;
    VectorLengthExpr = nullptr;

    // Build OpenMP directive and finalize enclosing compound statement, if
    // any.
    StmtResult Res;
    if (AssociatedStmt.isInvalid() ||
        TClauses.size() != D->clauses().size() - TClausesEmptyCount +
                           NumClausesAdded)
      Res = StmtError();
    else
      Res = getDerived().RebuildOMPExecutableDirective(
          OMPD_target_teams, DeclarationNameInfo(), OMPD_unknown, TClauses,
          AssociatedStmt.get(), D->getBeginLoc(), D->getEndLoc());
    getSema().EndOpenMPDSABlock(Res.get());
    EnclosingCompoundStmt.finalize(Res);
    if (!Res.isInvalid())
      D->setOMPNode(Res.get());
    return Res;
  }

  StmtResult TransformACCLoopDirective(ACCLoopDirective *D) {
    // What OpenACC clauses do we have?
    ACCPartitioningKind Partitioning = D->getPartitioning();

    // What kind of OpenMP directive should we build?
    // OMPD_unknown means none (so sequential).
    OpenMPDirectiveKind TDKind;
    bool AddNumThreads1 = false;
    Expr *AddNumThreadsExpr = nullptr;
    Expr *AddSimdlenExpr = nullptr;
    bool AddScopeWithLCVPrivate = false;
    bool AddScopeWithAllPrivates = false;
    assert((Partitioning.hasSeq() || Partitioning.hasIndependent()) &&
           "expected acc loop to have either seq or independent");
    if (Partitioning.hasSeq()) {
      // sequential loop
      TDKind = OMPD_unknown;
      AddScopeWithAllPrivates = true;
    } else {
      if (Partitioning.hasWorkerPartitioning()) {
        if (NumWorkersVarDecl) {
          ExprResult Res = getSema().BuildDeclRefExpr(
              NumWorkersVarDecl,
              NumWorkersVarDecl->getType().getNonReferenceType(), VK_RValue,
              D->getEndLoc());
          assert(!Res.isInvalid() &&
                 "expected valid reference to num_workers variable");
          AddNumThreadsExpr = Res.get();
        } else
          AddNumThreadsExpr = NumWorkersExpr;
      }
      if (Partitioning.hasVectorPartitioning()) {
        AddSimdlenExpr = VectorLengthExpr;
        AddScopeWithLCVPrivate = true;
      }
      if (!Partitioning.hasGangPartitioning()) {
        if (!Partitioning.hasWorkerPartitioning()) {
          if (!Partitioning.hasVectorPartitioning()) {
            // sequential loop
            TDKind = OMPD_unknown;
            AddScopeWithAllPrivates = true;
          } else { // hasVectorPartitioning
            if (ParentLoopOMPKind == OMPD_unknown) {
              TDKind = OMPD_parallel_for_simd;
              AddNumThreads1 = true;
            }
            else
              TDKind = OMPD_simd;
          }
        } else { // hasWorkerPartitioning
          if (!Partitioning.hasVectorPartitioning())
            TDKind = OMPD_parallel_for;
          else // hasVectorPartitioning
            TDKind = OMPD_parallel_for_simd;
        }
      } else { // hasGangPartitioning
        if (!Partitioning.hasWorkerPartitioning()) {
          if (!Partitioning.hasVectorPartitioning())
            TDKind = OMPD_distribute;
          else // hasVectorPartitioning
            TDKind = OMPD_distribute_simd;
        } else { // hasWorkerPartitioning
          if (!Partitioning.hasVectorPartitioning())
            TDKind = OMPD_distribute_parallel_for;
          else // hasVectorPartitioning
            TDKind = OMPD_distribute_parallel_for_simd;
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
          if ((AddScopeWithLCVPrivate &&
               D->getLoopControlVariables().count(VD)) ||
              AddScopeWithAllPrivates)
            EnclosingCompoundStmt.addPrivateDecl(VR->getBeginLoc(),
                                                 VR->getEndLoc(), VD);
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

    // Start OpenMP DSA block.
    getSema().StartOpenMPDSABlock(TDKind, DeclarationNameInfo(),
                                  /*CurScope=*/nullptr, D->getBeginLoc());

    // Add num_threads and simdlen clauses, as needed.
    llvm::SmallVector<OMPClause *, 16> TClauses;
    assert((!AddNumThreads1 || !AddNumThreadsExpr) &&
           "did not expect to add conflicting num_threads clauses");
    size_t NumClausesAdded = 0;
    if (AddNumThreads1) {
      OpenMPStartEndClauseRAII ClauseRAII(getSema(), OMPC_num_threads);
      ExprResult One = getSema().ActOnIntegerConstant(D->getEndLoc(), 1);
      assert(!One.isInvalid());
      TClauses.push_back(getDerived().RebuildOMPNumThreadsClause(
          One.get(), D->getEndLoc(), D->getEndLoc(), D->getEndLoc()));
      ++NumClausesAdded;
    } else if (AddNumThreadsExpr) {
      OpenMPStartEndClauseRAII ClauseRAII(getSema(), OMPC_num_threads);
      TClauses.push_back(getDerived().RebuildOMPNumThreadsClause(
          AddNumThreadsExpr, AddNumThreadsExpr->getBeginLoc(),
          AddNumThreadsExpr->getBeginLoc(), AddNumThreadsExpr->getEndLoc()));
      ++NumClausesAdded;
    }
    if (AddSimdlenExpr) {
      OpenMPStartEndClauseRAII ClauseRAII(getSema(), OMPC_simdlen);
      TClauses.push_back(getDerived().RebuildOMPSimdlenClause(
          AddSimdlenExpr, AddSimdlenExpr->getBeginLoc(),
          AddSimdlenExpr->getBeginLoc(), AddSimdlenExpr->getEndLoc()));
      ++NumClausesAdded;
    }

    // Transform OpenACC clauses.
    size_t TClausesEmptyCount;
    transformACCClauses(D, TDKind, TClauses, TClausesEmptyCount);

    // Transform associated statement.
    OpenMPDirectiveKind ParentLoopOMPKindOld = ParentLoopOMPKind;
    if (isOpenMPLoopDirective(TDKind))
      ParentLoopOMPKind = TDKind;
    StmtResult AssociatedStmt = transformACCAssociatedStmt(D, TDKind,
                                                           TClauses);
    ParentLoopOMPKind = ParentLoopOMPKindOld;

    // Build OpenMP directive and finalize enclosing compound statement, if
    // any.
    StmtResult Res;
    if (AssociatedStmt.isInvalid() ||
        TClauses.size() != D->clauses().size() - TClausesEmptyCount +
                           NumClausesAdded)
      Res = StmtError();
    else
      Res = getDerived().RebuildOMPExecutableDirective(
          TDKind, DeclarationNameInfo(), OMPD_unknown, TClauses,
          AssociatedStmt.get(), D->getBeginLoc(), D->getEndLoc());
    getSema().EndOpenMPDSABlock(Res.get());
    EnclosingCompoundStmt.finalize(Res);
    if (!Res.isInvalid())
      D->setOMPNode(Res.get());
    return Res;
  }

  StmtResult TransformACCParallelLoopDirective(ACCParallelLoopDirective *D) {
    StmtResult Res = TransformACCParallelDirective(
        cast<ACCParallelDirective>(D->getEffectiveDirective()));
    if (!Res.isInvalid())
      D->setOMPNode(Res.get());
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
      : LocStart(D && C->getBeginLoc().isInvalid() ? D->getEndLoc()
                                                   : C->getBeginLoc()),
        LParenLoc(LParenLoc), ColonLoc(ColonLoc), LocEnd(C->getEndLoc())
    {
      // So far, we have found that, if the clause's LocStart is invalid, all
      // the clause's locations are invalid.  Otherwise, setting LocStart to
      // the directive end might produce locations that are out of order.
      assert((!C->getBeginLoc().isInvalid() ||
              (LParenLoc.isInvalid() && ColonLoc.isInvalid() &&
               C->getEndLoc().isInvalid())) &&
              "Inconsistent location validity");
      assert((!D || !D->getEndLoc().isInvalid()) &&
             "Invalid directive location");
    }
  };

  template<typename Derived>
  bool transformACCVarList(
      ACCExecutableDirective *D, ACCVarListClause<Derived> *C,
      const llvm::DenseSet<VarDecl *> &SkipVars,
      llvm::SmallVector<Expr *, 16> &Vars) {
    Vars.reserve(C->varlist_size());
    for (auto *VE : C->varlists()) {
      if (SkipVars.count(cast<VarDecl>(cast<DeclRefExpr>(VE)->getDecl())
                             ->getCanonicalDecl()))
        continue;
      ExprResult EVar = getDerived().TransformExpr(cast<Expr>(VE));
      if (EVar.isInvalid())
        return true;
      Vars.push_back(EVar.get());
    }
    return false;
  }

  template<typename Derived>
  bool transformACCVarList(
      ACCExecutableDirective *D, ACCVarListClause<Derived> *C,
      llvm::SmallVector<Expr *, 16> &Vars) {
    return transformACCVarList(D, C, llvm::DenseSet<VarDecl *>(), Vars);
  }

  template<typename Derived>
  OMPClauseResult transformACCVarListClause(
      ACCExecutableDirective *D, ACCVarListClause<Derived> *C,
      OpenMPClauseKind TCKind, const llvm::DenseSet<VarDecl *> &SkipVars,
      OMPClause *(TransformACCToOMP::*Rebuilder)(
          ArrayRef<Expr *>, SourceLocation, SourceLocation, SourceLocation)) {
    OpenMPStartEndClauseRAII ClauseRAII(getSema(), TCKind);
    llvm::SmallVector<Expr *, 16> Vars;
    if (transformACCVarList(D, C, SkipVars, Vars))
      return OMPClauseError();
    ExplicitClauseLocs L(D, C, C->getLParenLoc());
    return (getDerived().*Rebuilder)(Vars, L.LocStart, L.LParenLoc, L.LocEnd);
  }

  template<typename Derived>
  OMPClauseResult transformACCVarListClause(
      ACCExecutableDirective *D, ACCVarListClause<Derived> *C,
      OpenMPClauseKind TCKind,
      OMPClause *(TransformACCToOMP::*Rebuilder)(
          ArrayRef<Expr *>, SourceLocation, SourceLocation, SourceLocation)) {
    return transformACCVarListClause(D, C, TCKind, llvm::DenseSet<VarDecl *>(),
                                     Rebuilder);
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

  OMPClauseResult TransformACCNumWorkersClause(ACCExecutableDirective *D,
                                               OpenMPDirectiveKind TDKind,
                                               ACCNumWorkersClause *C) {
    return OMPClauseEmpty();
  }

  OMPClauseResult TransformACCVectorLengthClause(ACCExecutableDirective *D,
                                                 OpenMPDirectiveKind TDKind,
                                                 ACCVectorLengthClause *C) {
    return OMPClauseEmpty();
  }

  OMPClauseResult TransformACCCollapseClause(ACCExecutableDirective *D,
                                             OpenMPDirectiveKind TDKind,
                                             ACCCollapseClause *C) {
    ExplicitClauseLocs L(D, C, C->getLParenLoc());
    return getDerived().RebuildOMPCollapseClause(
        C->getCollapse(), L.LocStart, L.LParenLoc, L.LocEnd);
  }

  OMPClauseResult TransformACCSharedClause(ACCExecutableDirective *D,
                                           OpenMPDirectiveKind TDKind,
                                           ACCSharedClause *C) {
    // OpenMP distribute or simd directives without parallel do not accept
    // shared clauses, so let our implicit OpenACC shared clauses stay implicit
    // in OpenMP.
    bool RequireImplicit = (isOpenMPDistributeDirective(TDKind) ||
                            isOpenMPSimdDirective(TDKind)) &&
                           !isOpenMPParallelDirective(TDKind);
    // Currently, there's no such thing as an explicit OpenACC shared clause.
    // If there were and RequireImplicit=true, we would need to complain to the
    // user as the OpenMP implementation might not complain unless we print the
    // generated OpenMP and recompile it.
    assert(C->getBeginLoc().isInvalid()
           && "Unexpected explicit OpenACC shared clause");
    return transformACCVarListClause<ACCSharedClause>(
        RequireImplicit ? nullptr : D, C, OMPC_shared,
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
    llvm::DenseSet<VarDecl *> SkipVars;
    if (isOpenMPSimdDirective(TDKind)) {
      ACCLoopDirective *LD = cast<ACCLoopDirective>(D);
      assert(LD && "expected omp simd directive to translate from acc loop"
                   " directive");
      SkipVars = LD->getLoopControlVariables();
    }
    return transformACCVarListClause<ACCPrivateClause>(
        D, C, OMPC_private, SkipVars,
        &TransformACCToOMP::RebuildOMPPrivateClause);
  }

  OMPClauseResult TransformACCFirstprivateClause(ACCExecutableDirective *D,
                                                 OpenMPDirectiveKind TDKind,
                                                 ACCFirstprivateClause *C) {
    return transformACCVarListClause<ACCFirstprivateClause>(
        D, C, OMPC_firstprivate,
        &TransformACCToOMP::RebuildOMPFirstprivateClause);
  }

  OMPClauseResult TransformACCReductionClause(ACCExecutableDirective *D,
                                              OpenMPDirectiveKind TDKind,
                                              ACCReductionClause *C) {
    // OpenMP distribute directives without parallel and simd do not accept
    // reduction clauses, but we copy such reductions to the enclosing acc
    // parallel to translate to a reduction on omp target teams.
    if (isOpenMPDistributeDirective(TDKind) &&
        !isOpenMPParallelDirective(TDKind) && !isOpenMPSimdDirective(TDKind))
      return OMPClauseEmpty();
    OpenMPStartEndClauseRAII ClauseRAII(getSema(), OMPC_reduction);
    llvm::SmallVector<Expr *, 16> Vars;
    if (transformACCVarList(D, C, Vars))
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
  if (DSAStack->getRealDirective() != ACCD_unknown)
    return false;
  if (TransformACCToOMP(*this).TransformStmt(D).isInvalid())
    return true;
  return false;
}
