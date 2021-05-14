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

#include "clang/AST/StmtVisitor.h"
#include "clang/Sema/ScopeInfo.h"
#include "clang/Sema/SemaInternal.h"
using namespace clang;

//===----------------------------------------------------------------------===//
// OpenACC directive stack
//===----------------------------------------------------------------------===//

namespace {
/// Stack for tracking OpenACC directives and their various properties, such
/// as data attributes.
///
/// In the case of a combined directive, we push two entries on the stack, one
/// for each effective directive.  They are not popped at the same time, so
/// outer effective directives are sometimes present when inner effective
/// directives are not.
class DirStackTy final {
public:
  Sema &SemaRef;

  /// Represents a variable's data attributes (DAs) on a directive.
  ///
  /// There are two DAs per variable: a DMA and a DSA.  Each can be
  /// undetermined, explicitly determined, predetermined, or implicitly
  /// determined (except that there currently is no case of a predetermined
  /// DMA).  Each will eventually be determined per variable referenced in a
  /// construct to which it is relevant (for example, DMAs are irrelevant to
  /// acc loop).
  struct DAVarData final {
    /// If the DMA is undetermined, then ACC_DMA_unknown.  Otherwise, not
    /// ACC_DMA_unknown.
    OpenACCDMAKind DMAKind = ACC_DMA_unknown;
    /// If the DSA is undetermined, then ACC_DSA_unknown.  Otherwise, not
    /// ACC_DSA_unknown.
    OpenACCDSAKind DSAKind = ACC_DSA_unknown;
    /// If the DMA is undetermined, then nullptr.  If explicit, then the
    /// referencing expression appearing in the associated clause.  If
    /// implicit, then a referencing expression within the directive's region.
    Expr *DMARefExpr = nullptr;
    /// If the DSA is undetermined, then nullptr.  If explicit, then the
    /// referencing expression appearing in the associated clause.  If
    /// predetermined or implicit, then a referencing expression within the
    /// directive's region.
    Expr *DSARefExpr = nullptr;
    /// If DSAKind is ACC_DSA_reduction, then the reduction ID.  Otherwise,
    /// undefined.
    DeclarationNameInfo ReductionId;
    DAVarData() {}
  };

private:
  struct DirStackEntryTy final {
    llvm::DenseMap<VarDecl *, DAVarData> DAMap;
    llvm::DenseMap<VarDecl *, Expr *> UpdateVarSet;
    llvm::SmallVector<Expr *, 4> ReductionVarsOnEffectiveOrCombined;
    llvm::DenseSet<VarDecl *> LCVSet;
    llvm::SmallVector<std::pair<Expr *, VarDecl *>, 4> LCVVec;
    /// The real directive kind.  In the case of a combined directive, there
    /// are two consecutive entries: the outer has RealDKind as the combined
    /// directive kind, the inner has RealDKind has ACCD_unknown, and both
    /// have EffectiveDKind != ACCD_unknown.
    OpenACCDirectiveKind RealDKind;
    /// The effective directive kind, which is always the same as RealDKind in
    /// the case of a non-combined directive.
    OpenACCDirectiveKind EffectiveDKind;
    ACCPartitioningKind LoopDirectiveKind;
    SourceLocation ConstructLoc;
    SourceLocation LoopBreakLoc; // invalid if no break statement or not loop
    unsigned AssociatedLoops = 1; // from collapse clause
    unsigned AssociatedLoopsParsed = 0; // how many have been parsed so far
    // True if this is an effective compute or loop directive and has an
    // effective nested loop directive with explicit gang partitioning.
    //
    // Implicit gang clauses are later added by ImplicitGangAdder after the
    // entire parallel construct is parsed and thus after this stack is popped
    // for all effective nested loop directives, so we don't bother to update
    // this then.
    bool NestedExplicitGangPartitioning = false;
    // True if this is an effective compute or loop directive and has an
    // effective nested loop directive with worker partitioning.
    bool NestedWorkerPartitioning = false;
    DirStackEntryTy(OpenACCDirectiveKind RealDKind,
                    OpenACCDirectiveKind EffectiveDKind, SourceLocation Loc)
        : RealDKind(RealDKind), EffectiveDKind(EffectiveDKind),
          ConstructLoc(Loc) {}
  };

  /// The underlying directive stack.
  SmallVector<DirStackEntryTy, 4> Stack;

public:
  explicit DirStackTy(Sema &S) : SemaRef(S) {}

  void push(OpenACCDirectiveKind RealDKind,
            OpenACCDirectiveKind EffectiveDKind, SourceLocation Loc) {
    Stack.push_back(DirStackEntryTy(RealDKind, EffectiveDKind, Loc));
  }

  void pop() {
    assert(!Stack.empty() && "expected non-empty directive stack");
    Stack.pop_back();
  }

  /// Register break statement in current acc loop.
  void addLoopBreakStatement(SourceLocation BreakLoc) {
    assert(!Stack.empty() && "expected non-empty directive stack");
    assert(isOpenACCLoopDirective(getEffectiveDirective())
           && "Break statement must be added only to loop directive");
    assert(BreakLoc.isValid() && "Expected valid break location");
    if (Stack.back().LoopBreakLoc.isInvalid()) // just the first one we see
      Stack.back().LoopBreakLoc = BreakLoc;
  }
  /// Return the location of the first break statement for this loop
  /// directive or return an invalid location if none.
  SourceLocation getLoopBreakStatement() const {
    assert(!Stack.empty() && "expected non-empty directive stack");
    return Stack.back().LoopBreakLoc;
  }
  /// Get all reduction variables on the effective or combined directive.  In
  /// the latter case, the effective directive with the reduction might have
  /// been popped off the stack already.
  const llvm::SmallVectorImpl<Expr *> &getReductionVars() const {
    assert(!Stack.empty() && "expected non-empty directive stack");
    return Stack.back().ReductionVarsOnEffectiveOrCombined;
  }
  /// Register the given expression as a loop control variable reference in an
  /// assignment but not a declaration in a for loop init associated with the
  /// current acc loop directive.
  void addLoopControlVariable(DeclRefExpr *DRE) {
    assert(!Stack.empty() && "expected non-empty directive stack");
    assert(isOpenACCLoopDirective(getEffectiveDirective()) &&
           "expected loop control variable to be added to loop directive");
    auto *VD = dyn_cast<VarDecl>(DRE->getDecl());
    assert(VD && "expected loop control variable to have a VarDecl");
    VD = VD->getCanonicalDecl();
    Stack.back().LCVSet.insert(VD);
    Stack.back().LCVVec.emplace_back(DRE, VD);
  }
  /// Is the specified variable a loop control variable that is assigned but
  /// not declared in the init of a for loop associated with the current
  /// directive?
  bool hasLoopControlVariable(VarDecl *VD) const {
    assert(!Stack.empty() && "expected non-empty directive stack");
    return Stack.back().LCVSet.count(VD->getCanonicalDecl());
  }
  /// Get the loop control variables that are assigned but not declared in the
  /// inits of the for loops associated with the current directive, or return
  /// an empty list if none.
  ///
  /// Each item in the list is a pair consisting of (a) the expression that
  /// references the variable in the assignment and (b) the canonical
  /// declaration for the variable.
  ArrayRef<std::pair<Expr *, VarDecl *>> getLoopControlVariables() const {
    assert(!Stack.empty() && "expected non-empty directive stack");
    return Stack.back().LCVVec;
  }
  /// Register the current directive's loop partitioning kind.
  ///
  /// As part of that, mark all effective ancestor compute or loop directives
  /// as containing any explicit gang or worker partitioning.
  void setLoopPartitioning(ACCPartitioningKind Kind) {
    assert(!Stack.empty() && "expected non-empty directive stack");
    Stack.back().LoopDirectiveKind = Kind;
    if (Kind.hasGangPartitioning() || Kind.hasWorkerPartitioning()) {
      assert(isOpenACCLoopDirective(getEffectiveDirective()) &&
             "expected gang/worker partitioning to be on a loop directive");
      for (auto I = std::next(Stack.rbegin()), E = Stack.rend(); I != E; ++I) {
        assert((isOpenACCLoopDirective(I->EffectiveDKind) ||
                isOpenACCComputeDirective(I->EffectiveDKind)) &&
               "expected gang/worker partitioning to be nested in acc loop or"
               " compute directive");
        if (Kind.hasGangPartitioning())
          I->NestedExplicitGangPartitioning = true;
        if (Kind.hasWorkerPartitioning())
          I->NestedWorkerPartitioning = true;
        // Outside a compute directive, we expect only data directives, which
        // need not be marked.
        if (isOpenACCComputeDirective(I->EffectiveDKind))
          break;
      }
    }
  }
  /// Get the current directive's loop partitioning kind.
  ///
  /// Must be called only after the point where \c setLoopPartitioning would be
  /// called, which is after explicit clauses for the current directive have
  /// been parsed.  This also does not include implicit gang clauses, which are
  /// not computed until the enclosing compute construct is fully parsed.
  ACCPartitioningKind getLoopPartitioning() const {
    assert(!Stack.empty() && "expected non-empty directive stack");
    return Stack.back().LoopDirectiveKind;
  }
  /// Iterate through the ancestor directives until finding either (1) an acc
  /// loop directive or combined loop directive with gang, worker, or vector
  /// clauses, (2) a compute directive, or (3) or the start of the stack.
  /// If case 1, return that directive's loop partitioning kind, and record its
  /// real directive kind and location.  Else if case 2 or 3, return no
  /// partitioning, and don't record a directive kind or location.
  ACCPartitioningKind getParentLoopPartitioning(
      OpenACCDirectiveKind &ParentDir, SourceLocation &ParentLoc) const {
    for (auto I = std::next(Stack.rbegin()), E = Stack.rend(); I != E; ++I) {
      if (isOpenACCComputeDirective(I->EffectiveDKind))
        return ACCPartitioningKind();
      const ACCPartitioningKind &ParentKind = I->LoopDirectiveKind;
      if (ParentKind.hasGangClause() || ParentKind.hasWorkerClause() ||
          ParentKind.hasVectorClause())
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
  /// Is this an effective compute or loop directive with an effective nested
  /// loop directive with explicit gang partitioning?
  ///
  /// Implicit gang clauses are later added by ImplicitGangAdder after the
  /// entire parallel construct is parsed and thus after this stack is popped
  /// for all effective nested loop directives, so we don't bother to update
  /// this then.
  bool getNestedExplicitGangPartitioning() const {
    assert(!Stack.empty() && "expected non-empty directive stack");
    return Stack.back().NestedExplicitGangPartitioning;
  }
  /// Is this an effective compute or loop directive with an effective nested
  /// loop directive with worker partitioning?
  bool getNestedWorkerPartitioning() const {
    assert(!Stack.empty() && "expected non-empty directive stack");
    return Stack.back().NestedWorkerPartitioning;
  }

private:
  struct DMATraits {
    typedef OpenACCDMAKind KindTy;
    static const OpenACCDMAKind KindUnknown = ACC_DMA_unknown;
    static OpenACCDMAKind &Kind(DAVarData &DAVar) { return DAVar.DMAKind; }
    static Expr * &RefExpr(DAVarData &DAVar) { return DAVar.DMARefExpr; }
    static void checkReductionIdArg(OpenACCDMAKind K,
                                    const DeclarationNameInfo &ReductionId) {
      assert(ReductionId.getName().isEmpty() &&
             "expected reduction ID for and only for reduction");
    }
    static bool checkReductionConflict(
        Sema &SemaRef, VarDecl *VD, Expr *E, OpenACCDMAKind K, DAVarData DVar,
        const DeclarationNameInfo &ReductionId) {
      return false;
    }
    static void setReductionFields(bool AddHere, OpenACCDMAKind K,
                                   DirStackEntryTy &StackEntry,
                                   DAVarData &DVar, Expr *RefExpr,
                                   const DeclarationNameInfo &ReductionId) {
    }
  };
  struct DSATraits {
    typedef OpenACCDSAKind KindTy;
    static const OpenACCDSAKind KindUnknown = ACC_DSA_unknown;
    static OpenACCDSAKind &Kind(DAVarData &DAVar) { return DAVar.DSAKind; }
    static Expr * &RefExpr(DAVarData &DAVar) { return DAVar.DSARefExpr; }
    static void checkReductionIdArg(OpenACCDSAKind K,
                                    const DeclarationNameInfo &ReductionId) {
      assert(((K == ACC_DSA_reduction) == !ReductionId.getName().isEmpty()) &&
             "expected reduction ID for and only for reduction");
    }
    static bool checkReductionConflict(
        Sema &SemaRef, VarDecl *VD, Expr *E, OpenACCDSAKind K, DAVarData DVar,
        const DeclarationNameInfo &ReductionId) {
      if (K == ACC_DSA_reduction && Kind(DVar) == ACC_DSA_reduction) {
        SemaRef.Diag(E->getExprLoc(), diag::err_acc_conflicting_reduction)
            << (DVar.ReductionId.getName() == ReductionId.getName())
            << ACCReductionClause::printReductionOperatorToString(ReductionId)
            << VD;
        SemaRef.Diag(DVar.DSARefExpr->getExprLoc(),
                     diag::note_acc_previous_reduction)
            << ACCReductionClause::printReductionOperatorToString(
                DVar.ReductionId);
        return true;
      }
      return false;
    }
    static void setReductionFields(bool AddHere, OpenACCDSAKind K,
                                   DirStackEntryTy &StackEntry,
                                   DAVarData &DVar, Expr *RefExpr,
                                   const DeclarationNameInfo &ReductionId) {
      if (K != ACC_DSA_reduction)
        return;
      if (AddHere)
        DVar.ReductionId = ReductionId;
      StackEntry.ReductionVarsOnEffectiveOrCombined.push_back(RefExpr);
    }
  };
  /// Encapsulates commonalities between implement addDMA and addDSA.  Each
  /// of DA and OtherDA is one of DMATraits or DSATraits.
  template <typename DA, typename OtherDA>
  bool addDA(VarDecl *VD, Expr *E, typename DA::KindTy DAKind,
             OpenACCDetermination Determination,
             const DeclarationNameInfo &ReductionId = DeclarationNameInfo());
public:
  ///@{
  /// For the given variable and given DA, selects the innermost effective
  /// directive to which it applies within the current real directive, records
  /// the DA on that effective directive, and diagnoses conflicts with existing
  /// DAs on any effective directive within the current real directive.  If the
  /// given DA is a reduction, marks every effective directive of the current
  /// real directive as being part of a real directive with a reduction.
  ///
  /// To select the directive to which the DA applies, we have to climb the
  /// directive stack because we parse and "act on" all explicit clauses before
  /// we pop the inner effective directive off the stack, and not all explicit
  /// clauses apply there.  The approach of applying to the first allowed is
  /// based on the comments in Sema::ActOnOpenACCParallelLoopDirective, and it
  /// must be kept in sync with the approach there.
  ///
  /// Generally, must not be called for a DA that should be suppressed by
  /// another DA for the same variable applied to the same effective directive,
  /// or a spurious diagnostic will be produced.  Specifically, it is the
  /// caller's responsibility to suppress (1) a predetermined DA if the same
  /// explicit DA is specified and (2) an implicit DA if any explicit or
  /// predetermined DA is specified.
  ///
  /// Must be called for an implicit DA only when the effective directive to
  /// which it applies (as opposed to a child effective directive in the same
  /// combined directive) is at the top of the stack.
  ///
  /// The given DA must not be ACC_DMA_unknown or ACC_DSA_unknown.
  bool addDMA(VarDecl *VD, Expr *E, OpenACCDMAKind DMAKind,
              OpenACCDetermination Determination) {
    return addDA<DMATraits, DSATraits>(VD, E, DMAKind, Determination);
  }
  bool addDSA(VarDecl *VD, Expr *E, OpenACCDSAKind DSAKind,
              OpenACCDetermination Determination,
              const DeclarationNameInfo &ReductionId = DeclarationNameInfo()) {
    return addDA<DSATraits, DMATraits>(VD, E, DSAKind, Determination,
                                       ReductionId);
  }
  ///@}

  /// Returns data attributes from top of the stack for the specified
  /// declaration.
  DAVarData getTopDA(VarDecl *VD);

  /// Returns true if the declaration has a DMA anywhere on the stack.
  bool hasVisibleDMA(VarDecl *VD);

  /// Records a variable appearing in an update directive clause and returns
  /// false, or complains and returns true if it already appeared in one for
  /// the current directive.
  bool addUpdateVar(VarDecl *VD, Expr *E) {
    assert(!Stack.empty() && "expected non-empty directive stack");
    auto Itr = Stack.rbegin();
    VD = VD->getCanonicalDecl();
    if (Expr *OldExpr = Itr->UpdateVarSet[VD]) {
      SemaRef.Diag(E->getExprLoc(), diag::err_acc_update_same_var)
          << VD->getName() << E->getSourceRange();
      SemaRef.Diag(OldExpr->getExprLoc(), diag::note_acc_update_var)
          << OldExpr->getSourceRange();
      return true;
    }
    Itr->UpdateVarSet[VD] = E;
    return false;
  }

  /// Returns currently analyzed directive.
  OpenACCDirectiveKind getRealDirective() const {
    auto I = Stack.rbegin();
    if (I == Stack.rend())
      return ACCD_unknown;
    while (I->RealDKind == ACCD_unknown)
      I = std::next(I);
    return I->RealDKind;
  }

  /// Returns the effective directive currently being analyzed (always the
  /// same as getRealDirective unless the latter is a combined directive).
  OpenACCDirectiveKind getEffectiveDirective() const {
    return Stack.empty() ? ACCD_unknown : Stack.back().EffectiveDKind;
  }

  /// Returns the real directive for construct enclosing currently analyzed
  /// real directive or returns ACCD_unknown if none.  In the former case only,
  /// set ParentLoc as the returned directive's location.
  OpenACCDirectiveKind getRealParentDirective(SourceLocation &ParentLoc) const {
    // Find real directive.
    auto I = Stack.rbegin();
    if (I == Stack.rend())
      return ACCD_unknown;
    while (I->RealDKind == ACCD_unknown)
      I = std::next(I);
    // Find real parent directive.
    I = std::next(I);
    if (I == Stack.rend())
      return ACCD_unknown;
    while (I->RealDKind == ACCD_unknown)
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
    assert(!Stack.empty() && "expected non-empty directive stack");
    assert(Stack.back().AssociatedLoops == 1);
    assert(Stack.back().AssociatedLoopsParsed == 0);
    Stack.back().AssociatedLoops = Val;
  }
  /// Return associated loop count (collapse value) for the region.
  unsigned getAssociatedLoops() const {
    return Stack.empty() ? 0 : Stack.back().AssociatedLoops;
  }
  /// Increment associated loops parsed in region so far.
  void incAssociatedLoopsParsed() {
    assert(!Stack.empty() && "expected non-empty directive stack");
    ++Stack.back().AssociatedLoopsParsed;
  }
  /// Get associated loops parsed in region so far.
  unsigned getAssociatedLoopsParsed() {
    return Stack.empty() ? 0 : Stack.back().AssociatedLoopsParsed;
  }

  SourceLocation getConstructLoc() { return Stack.back().ConstructLoc; }
};
} // namespace

template <typename DA, typename OtherDA>
bool DirStackTy::addDA(VarDecl *VD, Expr *E, typename DA::KindTy DAKind,
                       OpenACCDetermination Determination,
                       const DeclarationNameInfo &ReductionId) {
  VD = VD->getCanonicalDecl();
  assert(!Stack.empty() && "expected non-empty directive stack");
  assert(DAKind != DA::KindUnknown && "expected known DA");
  DA::checkReductionIdArg(DAKind, ReductionId);

  // If this is not a combined directive, or if this is an implicit clause,
  // visit just this directive.  Otherwise, climb through all effective
  // directives for the current combined directive.
  bool Added = false;
  for (auto Itr = Stack.rbegin(), End = Stack.rend(); Itr != End; ++Itr) {
    DAVarData &DVar = Itr->DAMap[VD];
    // Complain for conflict with existing DA.
    //
    // See the section "Basic Data Attributes" in the Clang OpenACC design
    // document.
    //
    // Some DAs should be suppressed by rather than conflict with existing DAs,
    // but addDMA and addDSA have preconditions to avoid those scenarios.
    if (DA::Kind(DVar) != DA::KindUnknown) {
      if (DA::checkReductionConflict(SemaRef, VD, E, DAKind, DVar,
                                     ReductionId))
        return true;
      SemaRef.Diag(E->getExprLoc(), diag::err_acc_conflicting_da)
          << (DA::Kind(DVar) == DAKind) << getOpenACCName(DA::Kind(DVar))
          << Determination << getOpenACCName(DAKind);
      SemaRef.Diag(DA::RefExpr(DVar)->getExprLoc(), diag::note_acc_explicit_da)
          << getOpenACCName(DA::Kind(DVar));
      return true;
    }
    if (!isAllowedDSAForDMA(DAKind, OtherDA::Kind(DVar))) {
      SemaRef.Diag(E->getExprLoc(), diag::err_acc_conflicting_da)
          << false << getOpenACCName(OtherDA::Kind(DVar))
          << Determination << getOpenACCName(DAKind);
      SemaRef.Diag(OtherDA::RefExpr(DVar)->getExprLoc(),
                   diag::note_acc_explicit_da)
          << getOpenACCName(OtherDA::Kind(DVar));
      return true;
    }
    // Complain for conflict with directive.
    if (isAllowedDAForDirective(Itr->EffectiveDKind, DAKind)) {
      DA::setReductionFields(!Added, DAKind, *Itr, DVar, E, ReductionId);
      if (!Added) {
        DA::Kind(DVar) = DAKind;
        DA::RefExpr(DVar) = E;
        Added = true;
        if (Determination == ACC_IMPLICIT)
          // Stop climbing or there might be spurious diagnostics about
          // conflicts with effective ancestor directives within the same real
          // directive.  For example, implicit shared on an effective acc loop
          // might conflict with explicit firstprivate on an effective parent
          // acc parallel.
          break;
      }
    }
    // As a precondition, an implicit clause is added when the effective
    // directive to which it applies is at the top of the stack.  Otherwise,
    // there might be spurious diagnostics about conflicts with effective
    // descendant directives within the same real directive.  For example,
    // implicit firstprivate on an effective acc parallel might conflict with
    // implicit shared on an effective child acc loop.
    assert(Determination != ACC_IMPLICIT &&
           "expected implicit clause to be allowed on directive at top of "
           "stack");
    if (Itr->RealDKind != ACCD_unknown)
      // Either this is not a combined directive, or we've reached the
      // outermost effective directive, so stop climbing.
      break;
  }

  assert(Added && "expected DA to be allowed for directive");
  return false;
}

DirStackTy::DAVarData DirStackTy::getTopDA(VarDecl *VD) {
  VD = VD->getCanonicalDecl();
  assert(!Stack.empty() && "expected non-empty directive stack");
  auto I = Stack.rbegin();
  auto DAItr = I->DAMap.find(VD);
  if (DAItr != I->DAMap.end())
    return DAItr->second;
  return DAVarData();
}

bool DirStackTy::hasVisibleDMA(VarDecl *VD) {
  VD = VD->getCanonicalDecl();
  assert(!Stack.empty() && "expected non-empty directive stack");
  for (auto I = Stack.rbegin(), E = Stack.rend(); I != E; ++I) {
    auto DAItr = I->DAMap.find(VD);
    if (DAItr != I->DAMap.end() && DAItr->second.DMAKind != ACC_DMA_unknown)
      return true;
  }
  return false;
}

void Sema::InitOpenACCDirectiveStack() {
  OpenACCDirectiveStack = new DirStackTy(*this);
}

#define DirStack static_cast<DirStackTy *>(OpenACCDirectiveStack)

void Sema::DestroyOpenACCDirectiveStack() { delete DirStack; }

void Sema::StartOpenACCDABlock(OpenACCDirectiveKind RealDKind,
                               SourceLocation Loc) {
  switch (RealDKind) {
  case ACCD_update:
  case ACCD_enter_data:
  case ACCD_exit_data:
  case ACCD_data:
  case ACCD_parallel:
  case ACCD_loop:
    DirStack->push(RealDKind, RealDKind, Loc);
    break;
  case ACCD_parallel_loop:
    DirStack->push(RealDKind, ACCD_parallel, Loc);
    DirStack->push(ACCD_unknown, ACCD_loop, Loc);
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
  case ACCC_present:
    Res = ActOnOpenACCPresentClause(VarList, ACC_EXPLICIT, StartLoc, LParenLoc,
                                    EndLoc);
    break;
#define OPENACC_CLAUSE_ALIAS_copy(Name) \
  case ACCC_##Name:
#include "clang/Basic/OpenACCKinds.def"
    Res = ActOnOpenACCCopyClause(Kind, VarList, ACC_EXPLICIT, StartLoc,
                                 LParenLoc, EndLoc);
    break;
#define OPENACC_CLAUSE_ALIAS_copyin(Name) \
  case ACCC_##Name:
#include "clang/Basic/OpenACCKinds.def"
    Res = ActOnOpenACCCopyinClause(Kind, VarList, StartLoc, LParenLoc, EndLoc);
    break;
#define OPENACC_CLAUSE_ALIAS_copyout(Name) \
  case ACCC_##Name:
#include "clang/Basic/OpenACCKinds.def"
    Res = ActOnOpenACCCopyoutClause(Kind, VarList, StartLoc, LParenLoc, EndLoc);
    break;
#define OPENACC_CLAUSE_ALIAS_create(Name) \
  case ACCC_##Name:
#include "clang/Basic/OpenACCKinds.def"
    Res = ActOnOpenACCCreateClause(Kind, VarList, StartLoc, LParenLoc, EndLoc);
    break;
  case ACCC_no_create:
    Res = ActOnOpenACCNoCreateClause(VarList, StartLoc, LParenLoc, EndLoc);
    break;
  case ACCC_delete:
    Res = ActOnOpenACCDeleteClause(VarList, StartLoc, LParenLoc, EndLoc);
    break;
  case ACCC_private:
    Res = ActOnOpenACCPrivateClause(VarList, ACC_EXPLICIT, StartLoc, LParenLoc,
                                    EndLoc);
    break;
  case ACCC_firstprivate:
    Res = ActOnOpenACCFirstprivateClause(VarList, ACC_EXPLICIT, StartLoc,
                                         LParenLoc, EndLoc);
    break;
  case ACCC_reduction:
    Res = ActOnOpenACCReductionClause(VarList, ACC_EXPLICIT, StartLoc,
                                      LParenLoc, ColonLoc, EndLoc,
                                      ReductionId);
    break;
#define OPENACC_CLAUSE_ALIAS_self(Name) \
  case ACCC_##Name:
#include "clang/Basic/OpenACCKinds.def"
    Res = ActOnOpenACCSelfClause(Kind, VarList, StartLoc, LParenLoc, EndLoc);
    break;
  case ACCC_device:
    Res = ActOnOpenACCDeviceClause(VarList, StartLoc, LParenLoc, EndLoc);
    break;
  case ACCC_nomap:
  case ACCC_shared:
  case ACCC_if:
  case ACCC_if_present:
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
    llvm_unreachable("expected explicit clause that accepts a variable list");
  }
  return Res;
}

void Sema::EndOpenACCClause() {
}

void Sema::EndOpenACCDABlock() {
  // For a combined directive, the first DirStack->pop() happens after the
  // inner effective directive is "acted upon" (AST node is constructed), so
  // this is the second DirStack->pop(), which happens after the entire
  // combined directive is acted upon.  However, if there was an error, we
  // need to pop the entire combined directive.
  for (OpenACCDirectiveKind DKind = ACCD_unknown; DKind == ACCD_unknown;
       DirStack->pop())
    DKind = DirStack->getEffectiveDirective();
  DiscardCleanupsInEvaluationContext();
  PopExpressionEvaluationContext();
}

namespace {
/// See the section "Implicit Gang Clauses" in the Clang OpenACC design
/// document.
class ImplicitGangAdder : public StmtVisitor<ImplicitGangAdder> {
public:
  void VisitACCExecutableDirective(ACCExecutableDirective *D) {
    if (isOpenACCLoopDirective(D->getDirectiveKind())) {
      auto *LD = cast<ACCLoopDirective>(D);
      ACCPartitioningKind Part = LD->getPartitioning();
      // If this loop is gang-partitioned, don't add an implicit gang clause,
      // which would be redundant, and don't continue to descendants because
      // they cannot then be gang-partitioned.
      if (Part.hasGangPartitioning())
        return;
      // We haven't encountered any enclosing gang-partitioned loop.  If there's
      // also no enclosed gang-partitioned loop and if this loop has been
      // determined to be independent, then this is the outermost loop meeting
      // all those conditions.  Thus, an implicit gang clause belongs here, and
      // don't continue to descendants because they cannot then be
      // gang-partitioned.
      if (!LD->getNestedGangPartitioning() && Part.hasIndependent()) {
        LD->addImplicitGangClause();
        return;
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
};

/// Table to track implicit DAs on a directive as they're computed by
/// \c ImplicitDAAdder and \c ImplicitGangReductionAdder.
///
/// Implicit DAs are not converted to clauses on the directive or stored as DAs
/// on the directive stack until all implicit DAs on the directive have been
/// computed.  The reason is that sometimes an implicit DA previously computed
/// must be overridden by a new implicit DA (for example, implicit gang
/// reduction overrides implicit shared).  This table's main benefit is its
/// facility to remove previously recorded implicit DAs.  Once both implicit DA
/// passes are finished, implicit DAs are extracted from this table, converted
/// to clauses on the directive, and stored as DAs on the directive stack, and
/// this table is discarded.
class ImplicitDATable {
public:
  struct Implier {
    /// The referencing expression that produced the implicit DA.
    DeclRefExpr *DRE;
    /// The reduction clause that produced the implicit DA, or nullptr if the
    /// implicit DA is not a reduction.
    ACCReductionClause *ReductionClause;
    Implier(DeclRefExpr *DRE, ACCReductionClause *C = nullptr)
      : DRE(DRE), ReductionClause(C) {}
  };
  typedef std::list<Implier> ImpliersTy;

private:
  ///@{
  /// This is where we build all the data that will be needed to convert
  /// implicit DAs to clauses on the directive and to store them on the
  /// directive stack.  For the sake of the test suite at least, we maintain
  /// these in a deterministic order (specifically the order computed), and we
  /// choose linked lists so we can efficiently remove implicit DAs when
  /// overridden.
  ImpliersTy NomapImpliers;
  ImpliersTy CopyImpliers;
  ImpliersTy SharedImpliers;
  ImpliersTy ReductionImpliers;
  ImpliersTy FirstprivateImpliers;
  ///@}

public:
  /// A map entry recording a single variable's implicit DMA and DSA, if any,
  /// and their locations in the above lists.
  class Entry {
    OpenACCDMAKind DMAKind;
    OpenACCDSAKind DSAKind;
    ImpliersTy::iterator DMAImplier;
    ImpliersTy::iterator DSAImplier;
    ImplicitDATable *Table;
  public:
    Entry(ImplicitDATable *Table)
        : DMAKind(ACC_DMA_unknown), DSAKind(ACC_DSA_unknown),
          DMAImplier(), DSAImplier(), Table(Table) {}
    OpenACCDMAKind getDMAKind() const { return DMAKind; }
    OpenACCDSAKind getDSAKind() const { return DSAKind; }
    void unsetDMA() {
      assert(DMAKind != ACC_DMA_unknown && "expected set DMA");
      Table->getImpliers(DMAKind).erase(DMAImplier);
      DMAKind = ACC_DMA_unknown;
    }
    void unsetDSA() {
      assert(DSAKind != ACC_DSA_unknown && "expected set DSA");
      Table->getImpliers(DSAKind).erase(DSAImplier);
      DSAKind = ACC_DSA_unknown;
    }
    void setDMA(OpenACCDMAKind Kind, DeclRefExpr *E) {
      assert(DMAKind == ACC_DMA_unknown && "expected unset DMA");
      DMAKind = Kind;
      ImpliersTy &Impliers = Table->getImpliers(DMAKind);
      Impliers.emplace_back(E);
      DMAImplier = std::prev(Impliers.end());
    }
    void setDSA(OpenACCDSAKind Kind, DeclRefExpr *E,
                ACCReductionClause *ReductionClause = nullptr) {
      assert(DSAKind == ACC_DSA_unknown && "expected unset DSA");
      assert((Kind == ACC_DSA_reduction) == (ReductionClause != nullptr) &&
             "expected clause for and only for reduction DSA");
      DSAKind = Kind;
      ImpliersTy &Impliers = Table->getImpliers(DSAKind);
      Impliers.emplace_back(E, ReductionClause);
      DSAImplier = std::prev(Impliers.end());
    }
  };

private:
  llvm::DenseMap<VarDecl *, Entry> Map;

public:
  Entry &lookup(VarDecl *VD) {
    return Map.try_emplace(VD, Entry(this)).first->second;
  }
  ImpliersTy &getImpliers(OpenACCDMAKind DMAKind) {
    switch (DMAKind) {
    case ACC_DMA_nomap:
      return NomapImpliers;
    case ACC_DMA_copy:
      return CopyImpliers;
    case ACC_DMA_present:
    case ACC_DMA_copyin:
    case ACC_DMA_copyout:
    case ACC_DMA_create:
    case ACC_DMA_no_create:
    case ACC_DMA_delete:
    case ACC_DMA_unknown:
      llvm_unreachable("expected DMA that can be implicit");
    }
  }
  ImpliersTy &getImpliers(OpenACCDSAKind DSAKind) {
    switch (DSAKind) {
    case ACC_DSA_shared:
      return SharedImpliers;
    case ACC_DSA_reduction:
      return ReductionImpliers;
    case ACC_DSA_firstprivate:
      return FirstprivateImpliers;
    case ACC_DSA_private:
    case ACC_DSA_unknown:
      llvm_unreachable("expected DSA that can be implicit");
    }
  }
  template <typename DAKindTy>
  bool buildRefList(DAKindTy DAKind, SmallVectorImpl<Expr *> &ExprVec) {
    ExprVec.clear();
    for (auto I : getImpliers(DAKind))
      ExprVec.push_back(I.DRE);
    return !ExprVec.empty();
  }
};

/// Computes all implicit DAs except implicit reductions, which are computed
/// by \c ImplicitGangReductionAdder.
class ImplicitDAAdder : public StmtVisitor<ImplicitDAAdder> {
  typedef StmtVisitor<ImplicitDAAdder> BaseVisitor;
  DirStackTy *Stack;
  ImplicitDATable &ImplicitDAs;
  llvm::SmallVector<llvm::DenseSet<const Decl *>, 8> LocalDefinitions;

public:
  void VisitDeclStmt(DeclStmt *S) {
    for (auto I = S->decl_begin(), E = S->decl_end(); I != E; ++I)
      LocalDefinitions.back().insert(*I);
    BaseVisitor::VisitDeclStmt(S);
  }
  void VisitDeclRefExpr(DeclRefExpr *E) {
    if (auto *VD = dyn_cast<VarDecl>(E->getDecl())) {
      // Skip variable if it is locally declared or privatized by a clause.
      if (LocalDefinitions.back().count(VD))
        return;

      // Skip variable if an implicit DA has already been computed.
      ImplicitDATable::Entry &ImplicitDA = ImplicitDAs.lookup(VD);
      if (ImplicitDA.getDMAKind() != ACC_DMA_unknown ||
          ImplicitDA.getDSAKind() != ACC_DSA_unknown)
        return;

      // Get predetermined and explicit DAs.
      const DirStackTy::DAVarData &DVar = Stack->getTopDA(VD);

      // Compute implicit DAs.
      VD = VD->getCanonicalDecl();
      OpenACCDMAKind DMAKind = ACC_DMA_unknown;
      OpenACCDSAKind DSAKind = ACC_DSA_unknown;
      if (isOpenACCLoopDirective(Stack->getEffectiveDirective())) {
        if (DVar.DSAKind == ACC_DSA_unknown) {
          // OpenACC 3.0 sec. 2.6.1 "Variables with Predetermined Data
          // Attributes" L1038-1039:
          //   "The loop variable in a C for statement [...] that is associated
          //   with a loop directive is predetermined to be private to each
          //   thread that will execute each iteration of the loop."
          //
          // See the section "Loop Control Variables" in the Clang OpenACC
          // design document for the interpretation used here.
          // Sema::ActOnOpenACCExecutableDirective handles the case without a
          // seq clause.
          assert((!Stack->hasLoopControlVariable(VD) ||
                  Stack->getLoopPartitioning().hasSeqExplicit()) &&
                 "expected predetermined private for loop control variable "
                 "with explicit seq");
          // See the section "Basic Data Attributes" in the Clang OpenACC
          // design document for discussion of the shared data attribute.
          DSAKind = ACC_DSA_shared;
        }
      } else if (isOpenACCParallelDirective(Stack->getEffectiveDirective())) {
        if (Stack->hasVisibleDMA(VD) || DVar.DSAKind != ACC_DSA_unknown) {
          // There's a visible explicit DMA, or there's a predetermined or
          // explicit DSA, so there's no implicit DA other than the defaults.
        } else if (!VD->getType()->isScalarType()) {
          // OpenACC 3.0 sec. 2.5.1 "Parallel Construct" L830-833:
          //   "If there is no default(present) clause on the construct, an
          //   array or composite variable referenced in the parallel construct
          //   that does not appear in a data clause for the construct or any
          //   enclosing data construct will be treated as if it appeared in a
          //   copy clause for the parallel construct."
          DMAKind = ACC_DMA_copy;
        } else {
          // OpenACC 3.0 sec. 2.5.1 "Parallel Construct" L835-838:
          //   "A scalar variable referenced in the parallel construct that
          //   does not appear in a data clause for the construct or any
          //   enclosing data construct will be treated as if it appeared in a
          //   firstprivate clause unless a reduction would otherwise imply a
          //   copy clause for it."
          // OpenACC 3.0 sec. 6 "Glossary" L3841-3845:
          //   "Scalar datatype - an intrinsic or built-in datatype that is not
          //   an array or aggregate datatype. [...] In C, scalar datatypes are
          //   char (signed or unsigned), int (signed or unsigned, with
          //   optional short, long or long long attribute), enum, float,
          //   double, long double, _Complex (with optional float or long
          //   attribute), or any pointer datatype."
          DSAKind = ACC_DSA_firstprivate;
        }
        if (DMAKind == ACC_DMA_unknown && DVar.DMAKind == ACC_DMA_unknown)
          DMAKind = ACC_DMA_nomap;
        if (DSAKind == ACC_DSA_unknown && DVar.DSAKind == ACC_DSA_unknown)
          DSAKind = ACC_DSA_shared;
      } else
        llvm_unreachable("unexpected effective directive");

      // Store implicit DAs for later conversion to clause objects.
      if (DMAKind != ACC_DMA_unknown)
        ImplicitDA.setDMA(DMAKind, E);
      if (DSAKind != ACC_DSA_unknown)
        ImplicitDA.setDSA(DSAKind, E);
    }
  }
  void VisitMemberExpr(MemberExpr *E) {
    Visit(E->getBase());
  }
  void VisitACCExecutableDirective(ACCExecutableDirective *D) {
    // Push space for local definitions in this construct.  It's important to
    // force a copy construct call here so that, if the vector is resized, the
    // pass-by-reference parameter isn't invalidated before it's copied.
    LocalDefinitions.emplace_back(
        llvm::DenseSet<const Decl *>(LocalDefinitions.back()));

    // Do reductions here imply copy clauses because we're computing implicit
    // clauses for an acc parallel and this is a gang-partitioned acc loop?
    bool ReductionsImplyCopy =
        isOpenACCComputeDirective(Stack->getEffectiveDirective()) &&
        isOpenACCLoopDirective(D->getDirectiveKind()) &&
        cast<ACCLoopDirective>(D)->getPartitioning().hasGangPartitioning();

    // Search this directive's clauses for reductions implying copy clauses and
    // for privatizations.
    for (ACCClause *C : D->clauses()) {
      if (!C)
        return;

      // Add implicit copy to acc parallel for gang-partitioned loop reduction.
      if (ReductionsImplyCopy && C->getClauseKind() == ACCC_reduction) {
        for (Expr *VR : cast<ACCReductionClause>(C)->varlists()) {
          DeclRefExpr *DRE = cast<DeclRefExpr>(VR);
          VarDecl *VD = cast<VarDecl>(DRE->getDecl())->getCanonicalDecl();
          // Skip variable if it is locally declared or privatized by a clause.
          if (LocalDefinitions.back().count(VD))
            continue;
          // Skip variable if there's a conflicting explicit DA or reduction
          // already.  Generally, all DMAs conflict with each other.  There
          // happens to exist no explicit DSA that doesn't conflict with copy
          // except reduction, but that implies copy anyway.  (And there's no
          // possible predetermined DA on a compute directive.)
          DirStackTy::DAVarData DVar = Stack->getTopDA(VD);
          if (DVar.DMAKind != ACC_DMA_unknown ||
              DVar.DSAKind != ACC_DSA_unknown) {
            assert((DVar.DSAKind == ACC_DSA_unknown ||
                    DVar.DSAKind == ACC_DSA_reduction ||
                    !isAllowedDSAForDMA(DVar.DSAKind, ACC_DMA_copy)) &&
                   "expected explicit DSA to conflict with copy");
            continue;
          }
          // Skip variable if an implicit copy clause has already been
          // computed.
          ImplicitDATable::Entry &ImplicitDA = ImplicitDAs.lookup(VD);
          if (ImplicitDA.getDMAKind() == ACC_DMA_copy)
            continue;
          // Override any implicit firstprivate clause.
          if (ImplicitDA.getDSAKind() == ACC_DSA_firstprivate) {
            assert(ImplicitDA.getDMAKind() == ACC_DMA_nomap &&
                   "expected acc parallel implicit DMA for firstprivate"
                   " variable to be nomap");
            ImplicitDA.unsetDMA();
            ImplicitDA.unsetDSA();
          } else
            assert(ImplicitDA.getDMAKind() == ACC_DMA_unknown &&
                   ImplicitDA.getDSAKind() == ACC_DSA_unknown &&
                   "expected acc parallel implicit DA for child acc loop"
                   " reduction variable to be copy, firstprivate, or none");
          ImplicitDA.setDMA(ACC_DMA_copy, DRE);
          ImplicitDA.setDSA(ACC_DSA_shared, DRE);
        }
      }

      // For clauses other than private, compute DA for the variables due to
      // the references within the clause itself.  This is necessary because at
      // least the variables' original values are needed.
      if (C->getClauseKind() != ACCC_private) {
        for (Stmt *Child : C->children()) {
          if (Child)
            Visit(Child);
        }
      }

      // Record variables privatized by this directive as local definitions so
      // that they are skipped while computing DAs implied by nested
      // references.
      for (const Expr *VR : getPrivateVarsFromClause(C)) {
        const DeclRefExpr *DRE = cast<DeclRefExpr>(VR);
        const VarDecl *VD =
            cast<VarDecl>(DRE->getDecl())->getCanonicalDecl();
        LocalDefinitions.back().insert(VD);
      }
    }

    // Recurse to children.
    for (auto *Child : D->children()) {
      if (Child)
        Visit(Child);
    }

    // Pop local definitions in this construct.
    LocalDefinitions.pop_back();
  }
  void VisitStmt(Stmt *S) {
    for (Stmt *C : S->children()) {
      if (C)
        Visit(C);
    }
  }

  ImplicitDAAdder(DirStackTy *S, ImplicitDATable &T)
      : Stack(S), ImplicitDAs(T) {
    // OpenACC 3.0 sec. 2.5.13 "reduction clause" L984-985:
    //   "It implies a copy data clause for each reduction var, unless a data
    //   clause for that variable appears on the compute construct."
    // OpenACC 3.0 sec. 2.11 "Combined Constructs" L1958-1959:
    //   "In addition, a reduction clause on a combined construct implies a
    //   copy data clause for each reduction variable, unless a data clause for
    //   that variable appears on the combined construct."
    // A copy DMA is implied by a reduction clause even for a reduction
    // variable that is not referenced within the construct, so it is handled
    // here rather than in VisitDeclRefExpr.
    if (Stack->getEffectiveDirective() == ACCD_parallel) {
      for (Expr *VR : Stack->getReductionVars()) {
        DeclRefExpr *DRE = cast<DeclRefExpr>(VR);
        VarDecl *VD = cast<VarDecl>(DRE->getDecl())->getCanonicalDecl();
        const DirStackTy::DAVarData &DVar = Stack->getTopDA(VD);
        if (DVar.DMAKind != ACC_DMA_unknown)
          continue;
        ImplicitDATable::Entry &ImplicitDA = ImplicitDAs.lookup(VD);
        assert(ImplicitDA.getDMAKind() == ACC_DMA_unknown &&
               ImplicitDA.getDSAKind() == ACC_DSA_unknown &&
               "expected no implicit DA to be set yet");
        ImplicitDA.setDMA(ACC_DMA_copy, DRE);
        if (DVar.DSAKind == ACC_DSA_unknown)
          ImplicitDA.setDSA(ACC_DSA_shared, DRE);
        else
          assert(DVar.DSAKind == ACC_DSA_reduction &&
                 "expected any explicit DSA for reduction var to be "
                 "reduction");
      }
    }
    LocalDefinitions.emplace_back();
  }
};

// OpenACC 3.0 sec. 2.9.11 L1753-1757:
// "After the loop, the values for each thread are combined using the specified
// reduction operator, and the result combined with the value of the original
// var and stored in the original var.  If the original var is not private,
// this update occurs by the end of the compute region, and any access to the
// original var is undefined within the compute region.  Otherwise, the update
// occurs at the end of the loop."
//
// Clang implements the case of the non-private original var by adding an
// (implied) gang reduction to the parent compute construct.  This means that
// the original var becomes privatized (per gang) within the compute region.
// Non-portable OpenACC code can take advantage of that behavior, but portable
// OpenACC code must regard accesses to it there as undefined.
class ImplicitGangReductionAdder
    : public StmtVisitor<ImplicitGangReductionAdder> {
  typedef StmtVisitor<ImplicitGangReductionAdder> BaseVisitor;
  DirStackTy *Stack;
  ImplicitDATable &ImplicitDAs;
  llvm::SmallVector<llvm::DenseSet<const Decl *>, 8> LocalDefinitions;

public:
  void VisitDeclStmt(DeclStmt *S) {
    for (auto I = S->decl_begin(), E = S->decl_end(); I != E; ++I)
      LocalDefinitions.back().insert(*I);
    BaseVisitor::VisitDeclStmt(S);
  }
  void VisitACCExecutableDirective(ACCExecutableDirective *D) {
    // If this is an acc loop, compute gang reductions implied here.
    if (isOpenACCLoopDirective(D->getDirectiveKind())) {
      for (ACCReductionClause *C : D->getClausesOfKind<ACCReductionClause>()) {
        for (Expr *VR : C->varlists()) {
          DeclRefExpr *DRE = cast<DeclRefExpr>(VR);
          VarDecl *VD = cast<VarDecl>(DRE->getDecl())->getCanonicalDecl();

          // Skip variable if it is declared locally in the acc parallel or if
          // it is privatized by an enclosing acc loop.
          if (LocalDefinitions.back().count(VD))
            continue;

          // Skip variable if it is gang-private at the acc loop due to an
          // explicit reduction at the acc parallel or due to a previously
          // computed implicit reduction at the acc parallel.  In either case,
          // if there's a reduction here with a conflicting operator, that will
          // be diagnosed by NestedReductionChecker.
          //
          // It is not actually important for application behavior whether a
          // reduction (implicit or explicit) at the acc parallel suppresses
          // other implicit gang reductions with the same reduction operator:
          // once you have one gang reduction for a variable, all other gang
          // reductions for it with the same reduction operator are redundant,
          // so you can include or skip the others without consequence.
          //
          // When computing an implicit gang reduction, we do not consider
          // variable privatization due to itself.  Otherwise we would have a
          // self-contradictory definition for implicit gang reductions: an acc
          // loop's gang-shared reduction variable implies a gang reduction at
          // the acc parallel, which makes the variable gang-private at the acc
          // loop, so there's no implicit gang reduction, which makes the
          // variable gang-shared at the acc loop, so there is an implicit gang
          // reduction, and so on.
          DirStackTy::DAVarData DVar = Stack->getTopDA(VD);
          ImplicitDATable::Entry &ImplicitDA = ImplicitDAs.lookup(VD);
          if (DVar.DSAKind == ACC_DSA_reduction ||
              ImplicitDA.getDSAKind() == ACC_DSA_reduction)
            continue;

          // The reduction can be added regardless of the DMA.
          assert(isAllowedDSAForDMA(ACC_DSA_reduction, DVar.DMAKind) &&
                 isAllowedDSAForDMA(ACC_DSA_reduction, ImplicitDA.getDMAKind())
                 && "expected reduction not to conflict with DMA");

          // If it's not privatized, then record the reduction.
          if (ImplicitDA.getDSAKind() == ACC_DSA_shared) {
            assert(DVar.DSAKind == ACC_DSA_unknown &&
                   "expected no implicit DSA with explicit/predetermined DSA");
            ImplicitDA.unsetDSA();
            ImplicitDA.setDSA(ACC_DSA_reduction, DRE, C);
          }
        }
      }
    }

    // Push space for local definitions in this construct.  It's important to
    // force a copy construct call here so that, if the vector is resized, the
    // pass-by-reference parameter isn't invalidated before it's copied.
    LocalDefinitions.emplace_back(
        llvm::DenseSet<const Decl *>(LocalDefinitions.back()));

    // Record variables privatized by this directive as local definitions so
    // that they are skipped while computing gang reductions implied by
    // nested directives.
    for (ACCClause *C : D->clauses()) {
      if (!C)
        return;
      for (const Expr *VR : getPrivateVarsFromClause(C)) {
        const DeclRefExpr *DRE = cast<DeclRefExpr>(VR);
        const VarDecl *VD =
            cast<VarDecl>(DRE->getDecl())->getCanonicalDecl();
        LocalDefinitions.back().insert(VD);
      }
    }

    // Recurse to children.
    for (auto *Child : D->children()) {
      if (Child)
        Visit(Child);
    }

    // Pop local definitions in this construct.
    LocalDefinitions.pop_back();
  }
  void VisitStmt(Stmt *S) {
    for (Stmt *C : S->children()) {
      if (C)
        Visit(C);
    }
  }
  ImplicitGangReductionAdder(DirStackTy *S, ImplicitDATable &T)
      : Stack(S), ImplicitDAs(T) {
    LocalDefinitions.emplace_back();
  }
};

// OpenACC 2.7 sec. 2.9.11 L1580-1581:
// "Reduction clauses on nested constructs for the same reduction var must
// have the same reduction operator."
// This examines reductions nested within implicit gang reductions as well, but
// that is not required by OpenACC 2.7.
class NestedReductionChecker : public StmtVisitor<NestedReductionChecker> {
  struct ReductionVar {
    ACCReductionClause *ReductionClause;
    DeclRefExpr *RE;
    ReductionVar() : ReductionClause(nullptr), RE(nullptr) {}
    ReductionVar(ACCReductionClause *C, DeclRefExpr *RE)
        : ReductionClause(C), RE(RE) {}
  };
  typedef StmtVisitor<NestedReductionChecker> BaseVisitor;
  DirStackTy *Stack;
  llvm::SmallVector<llvm::DenseMap<VarDecl *, ReductionVar>, 8> Privates;
  bool Error = false;

public:
  void VisitACCExecutableDirective(ACCExecutableDirective *D) {
    // Push space for privates in this construct.  It's important to force a
    // copy construct call here so that, if the vector is resized, the
    // pass-by-reference parameter isn't invalidated before it's copied.
    Privates.emplace_back(
        llvm::DenseMap<VarDecl *, ReductionVar>(Privates.back()));

    // Record variables privatized by this directive, and complain for any
    // reduction conflicting with its immediately enclosing reduction.
    for (ACCClause *C : D->clauses()) {
      if (!C)
        return;
      if (C->getClauseKind() == ACCC_reduction) {
        ACCReductionClause *ReductionClause = cast<ACCReductionClause>(C);
        for (Expr *VR : ReductionClause->varlists()) {
          // Try to record this variable's reduction.
          DeclRefExpr *DRE = cast<DeclRefExpr>(VR);
          VarDecl *VD = cast<VarDecl>(DRE->getDecl())->getCanonicalDecl();
          auto Insert = Privates.back().try_emplace(VD, ReductionClause, DRE);

          // If successfully recorded it because there was no enclosing clause
          // privatizing it, then there's nothing to compare against.
          if (Insert.second)
            continue;

          // If the enclosing privatizing clause is a reduction, complain if
          // the reduction operator conflicts.
          ReductionVar &EnclosingPrivate = Insert.first->second;
          if (EnclosingPrivate.ReductionClause &&
              ReductionClause->getNameInfo().getName() !=
              EnclosingPrivate.ReductionClause->getNameInfo().getName()) {
            Error = true;
            Stack->SemaRef.Diag(DRE->getExprLoc(),
                                diag::err_acc_conflicting_reduction)
                << false
                << ACCReductionClause::printReductionOperatorToString(
                    ReductionClause->getNameInfo())
                << VD;
            Stack->SemaRef.Diag(EnclosingPrivate.RE->getExprLoc(),
                                diag::note_acc_enclosing_reduction)
                << ACCReductionClause::printReductionOperatorToString(
                    EnclosingPrivate.ReductionClause->getNameInfo());
            if (EnclosingPrivate.ReductionClause->getBeginLoc().isInvalid())
              Stack->SemaRef.Diag(Stack->getConstructLoc(),
                                  diag::note_acc_implied_as_gang_reduction);
          }

          // Record this reduction.
          //
          // In the case that the enclosing privatizing clause is a reduction,
          // replacing its record with this reduction means every reduction
          // is compared against its immediately enclosing reduction rather
          // than the outermost reduction.  The diagnostics seem more intuitive
          // that way.
          EnclosingPrivate = ReductionVar(ReductionClause, DRE);
        }
      } else {
        // Record variables privatized by clauses other than reductions.
        for (Expr *VR : getPrivateVarsFromClause(C)) {
          DeclRefExpr *DRE = cast<DeclRefExpr>(VR);
          VarDecl *VD = cast<VarDecl>(DRE->getDecl())->getCanonicalDecl();
          Privates.back()[VD] = ReductionVar();
        }
      }
    }

    // Recurse to children.
    for (auto *Child : D->children()) {
      if (Child)
        Visit(Child);
    }

    // Pop privates in this construct.
    Privates.pop_back();
  }
  void VisitStmt(Stmt *S) {
    for (Stmt *C : S->children()) {
      if (C)
        Visit(C);
    }
  }
  bool hasError() { return Error; }
  NestedReductionChecker(DirStackTy *S) : Stack(S) { Privates.emplace_back(); }
};
} // namespace

bool Sema::ActOnOpenACCRegionStart(OpenACCDirectiveKind DKind,
                                   ArrayRef<ACCClause *> Clauses,
                                   SourceLocation StartLoc) {
  bool ErrorFound = false;
  // Check directive nesting.
  SourceLocation ParentLoc;
  OpenACCDirectiveKind ParentDKind =
      DirStack->getRealParentDirective(ParentLoc);
  if (!isAllowedParentForDirective(DKind, ParentDKind)) {
    if (ParentDKind == ACCD_unknown)
      Diag(StartLoc, diag::err_acc_orphaned_directive)
          << getOpenACCName(DKind);
    else {
      Diag(StartLoc, diag::err_acc_directive_bad_nesting)
          << getOpenACCName(ParentDKind) << getOpenACCName(DKind);
      Diag(ParentLoc, diag::note_acc_enclosing_directive)
          << getOpenACCName(ParentDKind);
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
        DirStack->getParentLoopPartitioning(ParentDKind, ParentLoopLoc);
    if (ParentLoopKind.hasGangClause() && LoopKind.hasGangClause()) {
      // OpenACC 2.6, sec. 2.9.2:
      // "The region of a loop with the gang clause may not contain another
      // loop with the gang clause unless within a nested compute region."
      Diag(StartLoc, diag::err_acc_loop_bad_nested_partitioning)
          << getOpenACCName(ParentDKind) << "gang"
          << getOpenACCName(DKind) << "gang";
      Diag(ParentLoopLoc, diag::note_acc_enclosing_directive)
          << getOpenACCName(ParentDKind);
      ErrorFound = true;
    }
    else if (ParentLoopKind.hasWorkerClause() &&
             (LoopKind.hasGangClause() || LoopKind.hasWorkerClause())) {
      // OpenACC 2.6, sec. 2.9.3:
      // "The region of a loop with the worker clause may not contain a loop
      // with a gang or worker clause unless within a nested compute region."
      Diag(StartLoc, diag::err_acc_loop_bad_nested_partitioning)
          << getOpenACCName(ParentDKind) << "worker" << getOpenACCName(DKind)
          << (LoopKind.hasGangClause() ? "gang" : "worker");
      Diag(ParentLoopLoc, diag::note_acc_enclosing_directive)
          << getOpenACCName(ParentDKind);
      ErrorFound = true;
    }
    else if (ParentLoopKind.hasVectorClause() &&
             (LoopKind.hasGangClause() || LoopKind.hasWorkerClause() ||
              LoopKind.hasVectorClause())) {
      // OpenACC 2.6, sec. 2.9.4:
      // "The region of a loop with the vector clause may not contain a loop
      // with the gang, worker, or vector clause unless within a nested compute
      // region."
      Diag(StartLoc, diag::err_acc_loop_bad_nested_partitioning)
          << getOpenACCName(ParentDKind) << "vector" << getOpenACCName(DKind)
          << (LoopKind.hasGangClause()
                  ? "gang"
                  : LoopKind.hasWorkerClause() ? "worker" : "vector");
      Diag(ParentLoopLoc, diag::note_acc_enclosing_directive)
          << getOpenACCName(ParentDKind);
      ErrorFound = true;
    }

    // TODO: For now, we prescriptively map auto to sequential execution, but
    // obviously an OpenACC compiler is meant to sometimes determine that
    // independent is possible.
    if (LoopKind.hasAuto())
      LoopKind.setSeqComputed();

    // Record partitioning on stack.
    DirStack->setLoopPartitioning(LoopKind);
  }
  return ErrorFound;
}

bool Sema::ActOnOpenACCRegionEnd() { return false; }

StmtResult Sema::ActOnOpenACCExecutableDirective(
    OpenACCDirectiveKind DKind, ArrayRef<ACCClause *> Clauses,
    Stmt *AStmt, SourceLocation StartLoc, SourceLocation EndLoc) {
  StmtResult Res = StmtError();

  // Our strategy for combined directives is to "act on" the clauses (already
  // done) as if they're on the same directive so that we can catch conflicts
  // (for data attributes, for example) that wouldn't arise if they were on
  // separate nested directives.  Then we act on the directive as if it's two
  // separate nested directives, acting on the effective innermost directive
  // first.  The AST node for the combined directive contains AST nodes for the
  // effective nested directives, facilitating translation to OpenMP.
  if (isOpenACCCombinedDirective(DKind)) {
    switch (DKind) {
    case ACCD_parallel_loop:
      return ActOnOpenACCParallelLoopDirective(Clauses, AStmt, StartLoc,
                                               EndLoc);
    case ACCD_update:
    case ACCD_enter_data:
    case ACCD_exit_data:
    case ACCD_data:
    case ACCD_parallel:
    case ACCD_loop:
    case ACCD_unknown:
      llvm_unreachable("expected combined OpenACC directive");
    }
  }

  // Compute implicit independent clause.
  llvm::SmallVector<ACCClause *, 8> ComputedClauses;
  bool ErrorFound = false;
  ComputedClauses.append(Clauses.begin(), Clauses.end());
  ACCPartitioningKind LoopKind = DirStack->getLoopPartitioning();
  if (LoopKind.hasIndependentImplicit()) {
    ACCClause *Implicit = ActOnOpenACCIndependentClause(
        ACC_IMPLICIT, SourceLocation(), SourceLocation());
    assert(Implicit);
    ComputedClauses.push_back(Implicit);
  }

  // Complain for break statement in loop with independent clause.
  if (LoopKind.hasIndependent()) {
    SourceLocation BreakLoc = DirStack->getLoopBreakStatement();
    if (BreakLoc.isValid()) {
      Diag(BreakLoc, diag::err_acc_loop_cannot_use_stmt) << "break";
      ErrorFound = true;
    }
    // TODO: In the case of auto, break statements will force sequential
    // execution, probably with a warning, or is an error better for that
    // case?
  }

  // Compute implicit gang clauses, predetermined DAs, and implicit DAs.
  // Complain about reductions for loop control variables.
  if (AStmt && (isOpenACCParallelDirective(DKind) ||
                isOpenACCLoopDirective(DKind))) {
    // Add implicit gang clauses to acc loops nested in the acc parallel.
    if (isOpenACCParallelDirective(DKind))
      ImplicitGangAdder().Visit(AStmt);

    // Iterate acc loop control variables.
    llvm::SmallVector<Expr *, 8> PrePrivate;
    for (std::pair<Expr *, VarDecl *> LCV :
             DirStack->getLoopControlVariables()) {
      Expr *RefExpr = LCV.first;
      VarDecl *VD = LCV.second;
      const DirStackTy::DAVarData &DVar = DirStack->getTopDA(VD);

      // Complain for any reduction.
      //
      // This restriction is not clearly specified in OpenACC 3.0.  See the
      // section "Loop Control Variables" in the Clang OpenACC design document.
      if (DVar.DSAKind == ACC_DSA_reduction) {
        Diag(DVar.DSARefExpr->getEndLoc(),
             diag::err_acc_reduction_on_loop_control_var)
            << VD->getName() << DVar.DSARefExpr->getSourceRange();
        Diag(RefExpr->getExprLoc(), diag::note_acc_loop_control_var)
            << VD->getName() << RefExpr->getSourceRange();
        ErrorFound = true;
        continue;
      }

      // OpenACC 3.0 sec. 2.6.1 L1038-1039:
      //   "The loop variable in a C for statement [...] that is associated
      //   with a loop directive is predetermined to be private to each thread
      //   that will execute each iteration of the loop."
      //
      // See the section "Loop Control Variables" in the Clang OpenACC design
      // document for the interpretation used here.  ImplicitDAAdder handles
      // the case with a seq clause.
      //
      // To avoid a spurious diagnostic about a conflicting (redundant) DA,
      // skip any variable for which there's already an explicit private
      // clause.
      if (!DirStack->getLoopPartitioning().hasSeqExplicit() &&
          DVar.DSAKind != ACC_DSA_private) {
        // OpenACC 3.0 sec. 2.6 "Data Environment" L1023-1024:
        //   "Variables with predetermined data attributes may not appear in a
        //   data clause that conflicts with that data attribute."
        // As far as we know, it's not actually possible to create such a
        // conflict except with a DA on the parent effective directive in a
        // combined directive.  See the section "Basic Data Attributes" in the
        // Clang OpenACC design document for further discussion.
        assert(DVar.DSAKind == ACC_DSA_unknown &&
               "expected no explicit attribute when predetermined attribute");
        PrePrivate.push_back(RefExpr);
      }
    }
    if (!PrePrivate.empty()) {
      ACCClause *Pre = ActOnOpenACCPrivateClause(
          PrePrivate, ACC_PREDETERMINED, SourceLocation(), SourceLocation(),
          SourceLocation());
      if (Pre)
        ComputedClauses.push_back(Pre);
      else
        ErrorFound = true;
    }

    // ImplicitDAAdder assumes predetermined and explicit DAs have no errors.
    if (ErrorFound)
      return StmtError();

    // For referenced variables, add implicit DAs other than reductions,
    // possibly influenced by any implicit gang clauses added above.
    ImplicitDATable ImplicitDAs;
    ImplicitDAAdder Adder(DirStack, ImplicitDAs);
    Adder.Visit(AStmt);

    // Add implicit gang reductions, possibly due to any implicit copy clauses
    // added above.
    if (isOpenACCParallelDirective(DKind)) {
      ImplicitGangReductionAdder ReductionAdder(DirStack, ImplicitDAs);
      ReductionAdder.Visit(AStmt);
    }

    // Act on all implicit data attributes to convert them to clauses and to
    // record them on the directive stack.
    SmallVector<Expr *, 8> ExprVec;
    if (ImplicitDAs.buildRefList(ACC_DMA_nomap, ExprVec)) {
      ACCClause *Implicit = ActOnOpenACCNomapClause(ExprVec);
      assert(Implicit && "expected successful implicit nomap");
      ComputedClauses.push_back(Implicit);
    }
    if (ImplicitDAs.buildRefList(ACC_DMA_copy, ExprVec)) {
      ACCClause *Implicit = ActOnOpenACCCopyClause(
          ACCC_copy, ExprVec, ACC_IMPLICIT, SourceLocation(), SourceLocation(),
          SourceLocation());
      // Implicit copy can be for an array, which can be of incomplete type,
      // which isn't permitted in copy, so we check for failure here.
      if (Implicit)
        ComputedClauses.push_back(Implicit);
    }
    if (ImplicitDAs.buildRefList(ACC_DSA_shared, ExprVec)) {
      ACCClause *Implicit = ActOnOpenACCSharedClause(ExprVec);
      assert(Implicit && "expected successful implicit shared");
      ComputedClauses.push_back(Implicit);
    }
    if (ImplicitDAs.buildRefList(ACC_DSA_firstprivate, ExprVec)) {
      ACCClause *Implicit = ActOnOpenACCFirstprivateClause(
          ExprVec, ACC_IMPLICIT, SourceLocation(), SourceLocation(),
          SourceLocation());
      assert(Implicit && "expected successful implicit firstprivate");
      ComputedClauses.push_back(Implicit);
    }
    for (auto I : ImplicitDAs.getImpliers(ACC_DSA_reduction)) {
      ACCClause *Implicit = ActOnOpenACCReductionClause(
          I.DRE, ACC_IMPLICIT, SourceLocation(), SourceLocation(),
          SourceLocation(), SourceLocation(),
          I.ReductionClause->getNameInfo());
      // Implicit reductions are copied from explicit reductions, which are
      // validated already.
      assert(Implicit && "expected successful implicit reduction");
      ComputedClauses.push_back(Implicit);
    }
  }

  // Act on the directive.
  switch (DKind) {
  case ACCD_update:
    assert(!AStmt &&
           "expected no associated statement for 'acc update' directive");
    Res = ActOnOpenACCUpdateDirective(ComputedClauses, StartLoc, EndLoc);
    break;
  case ACCD_enter_data:
    assert(!AStmt &&
           "expected no associated statement for 'acc enter data' directive");
    Res = ActOnOpenACCEnterDataDirective(ComputedClauses, StartLoc, EndLoc);
    break;
  case ACCD_exit_data:
    assert(!AStmt &&
           "expected no associated statement for 'acc exit data' directive");
    Res = ActOnOpenACCExitDataDirective(ComputedClauses, StartLoc, EndLoc);
    break;
  case ACCD_data:
    Res = ActOnOpenACCDataDirective(ComputedClauses, AStmt, StartLoc, EndLoc);
    break;
  case ACCD_parallel:
    Res = ActOnOpenACCParallelDirective(
        ComputedClauses, AStmt, StartLoc, EndLoc,
        DirStack->getNestedWorkerPartitioning());
    if (!Res.isInvalid()) {
      NestedReductionChecker Checker(DirStack);
      Checker.Visit(Res.get());
      if (Checker.hasError())
        ErrorFound = true;
    }
    break;
  case ACCD_loop: {
    SmallVector<VarDecl *, 5> LCVVars;
    for (std::pair<Expr *, VarDecl *> LCV :
             DirStack->getLoopControlVariables())
      LCVVars.push_back(LCV.second);
    Res = ActOnOpenACCLoopDirective(
        ComputedClauses, AStmt, StartLoc, EndLoc, LCVVars,
        DirStack->getLoopPartitioning(),
        DirStack->getNestedExplicitGangPartitioning());
    break;
  }
  case ACCD_parallel_loop:
  case ACCD_unknown:
    llvm_unreachable("expected non-combined OpenACC directive");
  }

  if (ErrorFound)
    return StmtError();
  return Res;
}

StmtResult Sema::ActOnOpenACCUpdateDirective(ArrayRef<ACCClause *> Clauses,
                                             SourceLocation StartLoc,
                                             SourceLocation EndLoc) {
  // OpenACC 3.0 sec. 2.14.4 "Update Directive" L1117-1118:
  // "At least one self, host, or device clause must appear on an update
  // directive."
  bool Found = false;
  for (ACCClause *C : Clauses) {
    switch (C->getClauseKind()) {
#define OPENACC_CLAUSE_ALIAS_self(Name) \
    case ACCC_##Name:
#include "clang/Basic/OpenACCKinds.def"
    case ACCC_device:
      Found = true;
      break;
    default:
      break;
    }
    if (Found)
      break;
  }
  if (!Found) {
    Diag(StartLoc, diag::err_acc_no_self_host_device_clause);
    return StmtError();
  }

  return ACCUpdateDirective::Create(Context, StartLoc, EndLoc, Clauses);
}

StmtResult Sema::ActOnOpenACCEnterDataDirective(ArrayRef<ACCClause *> Clauses,
                                                SourceLocation StartLoc,
                                                SourceLocation EndLoc) {
  // OpenACC 3.0 sec. 2.6.6 "Enter Data and Exit Data Directives" L1159-1160:
  // "At least one copyin, create, or attach clause must appear on an enter data
  // directive."
  bool Found = false;
  for (ACCClause *C : Clauses) {
    switch (C->getClauseKind()) {
#define OPENACC_CLAUSE_ALIAS_copyin(Name) \
    case ACCC_##Name:
#define OPENACC_CLAUSE_ALIAS_create(Name) \
    case ACCC_##Name:
#include "clang/Basic/OpenACCKinds.def"
      Found = true;
      break;
    default:
      llvm_unreachable("expected clause permitted on 'acc enter data' "
                       "directive");
    }
    if (Found)
      break;
  }
  if (!Found) {
    Diag(StartLoc, diag::err_acc_no_data_clause)
        << getOpenACCName(ACCD_enter_data);
    return StmtError();
  }

  return ACCEnterDataDirective::Create(Context, StartLoc, EndLoc, Clauses);
}

StmtResult Sema::ActOnOpenACCExitDataDirective(ArrayRef<ACCClause *> Clauses,
                                               SourceLocation StartLoc,
                                               SourceLocation EndLoc) {
  // OpenACC 3.0 sec. 2.6.6 "Enter Data and Exit Data Directives" L1161-1162:
  // "At least one copyout, delete, or detach clause must appear on an exit data
  // direcive."
  bool Found = false;
  for (ACCClause *C : Clauses) {
    switch (C->getClauseKind()) {
#define OPENACC_CLAUSE_ALIAS_copyout(Name) \
    case ACCC_##Name:
#include "clang/Basic/OpenACCKinds.def"
    case ACCC_delete:
      Found = true;
      break;
    default:
      llvm_unreachable("expected clause permitted on 'acc exit data' "
                       "directive");
    }
    if (Found)
      break;
  }
  if (!Found) {
    Diag(StartLoc, diag::err_acc_no_data_clause)
        << getOpenACCName(ACCD_exit_data);
    return StmtError();
  }

  return ACCExitDataDirective::Create(Context, StartLoc, EndLoc, Clauses);
}

StmtResult Sema::ActOnOpenACCDataDirective(
    ArrayRef<ACCClause *> Clauses, Stmt *AStmt, SourceLocation StartLoc,
    SourceLocation EndLoc) {
  if (!AStmt)
    return StmtError();

  // OpenACC 3.0 sec. 2.6.5 "Data Construct" L1117-1118:
  // "At least one copy, copyin, copyout, create, no_create, present,
  // deviceptr, attach, or default clause must appear on a data construct."
  bool Found = false;
  for (ACCClause *C : Clauses) {
    switch (C->getClauseKind()) {
    case ACCC_present:
#define OPENACC_CLAUSE_ALIAS_copy(Name) \
    case ACCC_##Name:
#define OPENACC_CLAUSE_ALIAS_copyin(Name) \
    case ACCC_##Name:
#define OPENACC_CLAUSE_ALIAS_copyout(Name) \
    case ACCC_##Name:
#define OPENACC_CLAUSE_ALIAS_create(Name) \
    case ACCC_##Name:
#include "clang/Basic/OpenACCKinds.def"
    case ACCC_no_create:
      Found = true;
      break;
    default:
      llvm_unreachable("expected clause permitted on 'acc data' directive");
    }
    if (Found)
      break;
  }
  if (!Found) {
    Diag(StartLoc, diag::err_acc_no_data_clause)
        << getOpenACCName(ACCD_data);
    return StmtError();
  }

  getCurFunction()->setHasBranchProtectedScope();

  return ACCDataDirective::Create(Context, StartLoc, EndLoc, Clauses, AStmt);
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
  if (DirStack->getAssociatedLoopsParsed() < DirStack->getAssociatedLoops() &&
      isOpenACCLoopDirective(DirStack->getEffectiveDirective())) {
    if (Expr *E = dyn_cast<Expr>(Init))
      Init = E->IgnoreParens();
    if (auto *BO = dyn_cast<BinaryOperator>(Init)) {
      if (BO->getOpcode() == BO_Assign) {
        auto *LHS = BO->getLHS()->IgnoreParens();
        if (auto *DRE = dyn_cast<DeclRefExpr>(LHS))
          DirStack->addLoopControlVariable(DRE);
      }
    }
    DirStack->incAssociatedLoopsParsed();
  }
}

void Sema::ActOnOpenACCLoopBreakStatement(SourceLocation BreakLoc,
                                          Scope *CurScope) {
  assert(getLangOpts().OpenACC && "OpenACC is not active.");
  if (Scope *S = CurScope->getBreakParent()) {
    if (S->isOpenACCLoopScope())
      DirStack->addLoopBreakStatement(BreakLoc);
  }
}

StmtResult Sema::ActOnOpenACCLoopDirective(
    ArrayRef<ACCClause *> Clauses, Stmt *AStmt, SourceLocation StartLoc,
    SourceLocation EndLoc, const ArrayRef<VarDecl *> &LCVs,
    ACCPartitioningKind Partitioning, bool NestedExplicitGangPartitioning) {
  if (!AStmt)
    return StmtError();

  // OpenACC 2.7 sec. 2.9.1, lines 1441-1442:
  // "The collapse clause is used to specify how many tightly nested loops are
  // associated with the loop construct."
  // Complain if there aren't enough.
  Stmt *LoopStmt = AStmt;
  for (unsigned LoopI = 0, LoopCount = DirStack->getAssociatedLoops();
       LoopI < LoopCount; ++LoopI)
  {
    LoopStmt = LoopStmt->IgnoreContainers();
    auto *LoopFor = dyn_cast_or_null<ForStmt>(LoopStmt);
    if (!LoopFor) {
      Diag(LoopStmt->getBeginLoc(), diag::err_acc_not_for)
          << getOpenACCName(DirStack->getRealDirective());
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

  // FIXME: Much more validation of the for statement should be performed.  To
  // save implementation time, for now we just depend on the OpenMP
  // implementation to handle that when transforming to OpenMP.  However, that
  // means diagnostics often mention OpenMP constructs not OpenACC constructs,
  // and it means any useful results of the analysis are not available to
  // OpenACC-level tools.

  return ACCLoopDirective::Create(Context, StartLoc, EndLoc, Clauses, AStmt,
                                  LCVs, Partitioning,
                                  NestedExplicitGangPartitioning);
}

StmtResult Sema::ActOnOpenACCParallelLoopDirective(
    ArrayRef<ACCClause *> Clauses, Stmt *AStmt, SourceLocation StartLoc,
    SourceLocation EndLoc) {
  // OpenACC 2.7 sec. 2.11, "Combined Constructs":
  // - Lines 1617-1618:
  //   "Any clause that is allowed on a parallel or loop construct is allowed
  //   on the parallel loop construct;"
  // - Lines 1633-1634:
  //   "A private or reduction clause on a combined construct is treated as if
  //   it appeared on the loop construct."
  // We assume clauses apply only where they're allowed.  private and reduction
  // are singled out above because they apply in both places, so the above
  // clarifies that they are applied to the inner directive.  This
  // understanding is critical to the implementations for DirStack::addDMA and
  // DirStack::addDSA as well.
  SmallVector<ACCClause *, 5> ParallelClauses;
  SmallVector<ACCClause *, 5> LoopClauses;
  for (ACCClause *C : Clauses) {
    if (isAllowedClauseForDirective(ACCD_loop, C->getClauseKind()))
      LoopClauses.push_back(C);
    else {
      assert(isAllowedClauseForDirective(ACCD_parallel, C->getClauseKind()) &&
             "expected clause to be allowed on acc parallel");
      ParallelClauses.push_back(C);
    }
  }

  // Build the effective loop directive.
  StmtResult Res = ActOnOpenACCExecutableDirective(ACCD_loop, LoopClauses,
                                                   AStmt, StartLoc, EndLoc);
  // The second DirStack->pop() happens in EndOpenACCDABlock.
  DirStack->pop();
  if (Res.isInvalid())
    return StmtError();
  ACCLoopDirective *LoopDir = cast<ACCLoopDirective>(Res.get());

  // Build the effective parallel directive.
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
  case ACCC_if:
    Res = ActOnOpenACCIfClause(Expr, StartLoc, LParenLoc, EndLoc);
    break;
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
  case ACCC_nomap:
  case ACCC_present:
#define OPENACC_CLAUSE_ALIAS_copy(Name) \
  case ACCC_##Name:
#define OPENACC_CLAUSE_ALIAS_copyin(Name) \
  case ACCC_##Name:
#define OPENACC_CLAUSE_ALIAS_copyout(Name) \
  case ACCC_##Name:
#define OPENACC_CLAUSE_ALIAS_create(Name) \
  case ACCC_##Name:
#include "clang/Basic/OpenACCKinds.def"
  case ACCC_no_create:
  case ACCC_delete:
  case ACCC_shared:
  case ACCC_private:
  case ACCC_firstprivate:
  case ACCC_reduction:
  case ACCC_if_present:
#define OPENACC_CLAUSE_ALIAS_self(Name) \
  case ACCC_##Name:
#include "clang/Basic/OpenACCKinds.def"
  case ACCC_device:
  case ACCC_seq:
  case ACCC_independent:
  case ACCC_auto:
  case ACCC_gang:
  case ACCC_worker:
  case ACCC_vector:
  case ACCC_unknown:
    llvm_unreachable("expected clause that takes a single expression");
  }
  return Res;
}

static VarDecl *
getVarDeclFromVarList(Sema &S, OpenACCClauseKind CKind, Expr *&RefExpr,
                      SourceLocation &ELoc, SourceRange &ERange,
                      bool AllowSubarray = false) {
  ELoc = RefExpr->getExprLoc();
  ERange = RefExpr->getSourceRange();
  RefExpr = RefExpr->IgnoreParenImpCasts();

  // If it's a subarray, extract the base variable, and complain about any
  // subscript (masquerading as a subarray), for which OpenACC 2.7 doesn't
  // specify a behavior.
  bool IsSubarray = false;
  if (auto *OASE = dyn_cast_or_null<OMPArraySectionExpr>(RefExpr)) {
    IsSubarray = true;
    do {
      if (OASE->getColonLocFirst().isInvalid()) {
        S.Diag(OASE->getExprLoc(), diag::err_acc_subarray_without_colon)
            << OASE->getSourceRange();
        if (!AllowSubarray)
          S.Diag(OASE->getExprLoc(), diag::err_acc_unsupported_subarray)
              << getOpenACCName(CKind) << OASE->getSourceRange();
        return nullptr;
      }
      RefExpr = OASE->getBase()->IgnoreParenImpCasts();
    } while ((OASE = dyn_cast<OMPArraySectionExpr>(RefExpr)));
  }

  // Complain about any subscript, for which OpenACC 2.7 doesn't specify a
  // behavior.
  if (auto *ASE = dyn_cast_or_null<ArraySubscriptExpr>(RefExpr)) {
    S.Diag(ASE->getExprLoc(), diag::err_acc_subarray_without_colon)
        << ASE->getSourceRange();
    if (!AllowSubarray)
      S.Diag(ASE->getExprLoc(), diag::err_acc_unsupported_subarray)
          << getOpenACCName(CKind) << ASE->getSourceRange();
    return nullptr;
  }

  // Complain if we didn't find a base variable.
  RefExpr = RefExpr->IgnoreParenImpCasts();
  auto *DE = dyn_cast_or_null<DeclRefExpr>(RefExpr);
  if (!DE || !isa<VarDecl>(DE->getDecl())) {
    if (IsSubarray) {
      S.Diag(ELoc, diag::err_acc_expected_base_var_name) << ERange;
      if (!AllowSubarray)
        S.Diag(ELoc, diag::err_acc_unsupported_subarray)
            << getOpenACCName(CKind) << ERange;
    }
    else
      S.Diag(ELoc, diag::err_acc_expected_var_name_or_subarray_expr)
          << AllowSubarray << ERange;
    return nullptr;
  }

  // Complain if we found a subarray but it's not allowed.
  if (IsSubarray && !AllowSubarray) {
    S.Diag(ELoc, diag::err_acc_unsupported_subarray)
        << getOpenACCName(CKind) << ERange;
    return nullptr;
  }

  // Return the base variable.
  return cast<VarDecl>(DE->getDecl());
}

static bool RequireCompleteTypeACC(
    Sema &SemaRef, QualType Type, OpenACCDetermination Determination,
    OpenACCClauseKind CKind, SourceLocation ConstructLoc, SourceLocation ELoc)
{
  if (SemaRef.RequireCompleteType(ELoc, Type, diag::err_acc_incomplete_type,
                                  Determination, getOpenACCName(CKind)))
  {
    if (Determination != ACC_EXPLICIT)
      SemaRef.Diag(ConstructLoc, diag::note_acc_clause_determination)
          << Determination << getOpenACCName(CKind);
    return true;
  }
  return false;
}

ACCClause *Sema::ActOnOpenACCNomapClause(ArrayRef<Expr *> VarList) {
  SmallVector<Expr *, 8> Vars;
  for (auto &RefExpr : VarList) {
    assert(RefExpr && "NULL expr in OpenACC implicit nomap clause.");
    auto *DE = dyn_cast_or_null<DeclRefExpr>(RefExpr->IgnoreParens());
    assert(DE && "OpenACC implicit nomap clause for non-DeclRefExpr");
    auto *VD = dyn_cast_or_null<VarDecl>(DE->getDecl());
    assert(VD && "OpenACC implicit nomap clause for non-VarDecl");

    if (!DirStack->addDMA(VD, RefExpr->IgnoreParens(), ACC_DMA_nomap,
                          ACC_IMPLICIT))
      Vars.push_back(RefExpr->IgnoreParens());
    else
      // Assert that the variable does not appear in an explicit data clause on
      // the same directive.  If the OpenACC spec grows an explicit nomap
      // clause one day, this assert should be removed.
      llvm_unreachable("implicit nomap clause unexpectedly generated for"
                       " variable in conflicting explicit data clause");
  }

  if (Vars.empty())
    return nullptr;

  return ACCNomapClause::Create(Context, Vars);
}

ACCClause *Sema::ActOnOpenACCPresentClause(
    ArrayRef<Expr *> VarList, OpenACCDetermination Determination,
    SourceLocation StartLoc, SourceLocation LParenLoc, SourceLocation EndLoc) {
  SmallVector<Expr *, 8> Vars;
  for (auto &RefExpr : VarList) {
    assert(RefExpr && "NULL expr in OpenACC present clause.");
    SourceLocation ELoc;
    SourceRange ERange;
    Expr *SimpleRefExpr = RefExpr;
    VarDecl *VD = getVarDeclFromVarList(*this, ACCC_present, SimpleRefExpr,
                                        ELoc, ERange, /*AllowSubarray=*/true);
    if (!VD)
      continue;

    QualType Type = VD->getType();

    // The OpenACC 3.0 spec doesn't say, as far as I know, that a variable
    // in a present clause must have a complete type.  However, without its
    // size, we cannot know if it's fully present.  Besides, it translates to
    // an OpenMP map clause, which does require a complete type.
    if (RequireCompleteTypeACC(*this, Type, Determination, ACCC_present,
                               DirStack->getConstructLoc(), ELoc))
      continue;

    if (!DirStack->addDMA(VD, RefExpr->IgnoreParens(), ACC_DMA_present,
                          Determination))
      Vars.push_back(RefExpr->IgnoreParens());
  }

  if (Vars.empty())
    return nullptr;

  return ACCPresentClause::Create(Context, Determination, StartLoc, LParenLoc,
                                  EndLoc, Vars);
}

ACCClause *Sema::ActOnOpenACCCopyClause(
    OpenACCClauseKind Kind, ArrayRef<Expr *> VarList,
    OpenACCDetermination Determination, SourceLocation StartLoc,
    SourceLocation LParenLoc, SourceLocation EndLoc) {
  assert(ACCCopyClause::isClauseKind(Kind) &&
         "expected copy clause or alias");
  SmallVector<Expr *, 8> Vars;

  for (auto &RefExpr : VarList) {
    assert(RefExpr && "NULL expr in OpenACC copy clause.");
    SourceLocation ELoc;
    SourceRange ERange;
    Expr *SimpleRefExpr = RefExpr;
    VarDecl *VD = getVarDeclFromVarList(*this, Kind, SimpleRefExpr, ELoc,
                                        ERange, /*AllowSubarray=*/true);
    if (!VD)
      continue;

    QualType Type = VD->getType();

    // The OpenACC 2.7 spec doesn't say, as far as I know, that a variable
    // in a copy clause must have a complete type.  However, you cannot copy
    // data if it doesn't have a size, and the OpenMP implementation does have
    // this restriction for map clauses.
    if (RequireCompleteTypeACC(*this, Type, Determination, Kind,
                               DirStack->getConstructLoc(), ELoc))
      continue;

    if (!DirStack->addDMA(VD, RefExpr->IgnoreParens(), ACC_DMA_copy,
                          Determination))
      Vars.push_back(RefExpr->IgnoreParens());
  }

  if (Vars.empty())
    return nullptr;

  return ACCCopyClause::Create(Context, Kind, Determination, StartLoc,
                               LParenLoc, EndLoc, Vars);
}

ACCClause *Sema::ActOnOpenACCCopyinClause(
    OpenACCClauseKind Kind, ArrayRef<Expr *> VarList, SourceLocation StartLoc,
    SourceLocation LParenLoc, SourceLocation EndLoc) {
  assert(ACCCopyinClause::isClauseKind(Kind) &&
         "expected copyin clause or alias");
  SmallVector<Expr *, 8> Vars;

  for (auto &RefExpr : VarList) {
    assert(RefExpr && "NULL expr in OpenACC copyin clause.");
    SourceLocation ELoc;
    SourceRange ERange;
    Expr *SimpleRefExpr = RefExpr;
    VarDecl *VD = getVarDeclFromVarList(*this, Kind, SimpleRefExpr, ELoc,
                                        ERange, /*AllowSubarray=*/true);
    if (!VD)
      continue;

    QualType Type = VD->getType();

    // The OpenACC 2.7 spec doesn't say, as far as I know, that a variable
    // in a copyin clause must have a complete type.  However, you cannot copy
    // data if it doesn't have a size, and the OpenMP implementation does have
    // this restriction for map clauses.
    if (RequireCompleteTypeACC(*this, Type, ACC_EXPLICIT, Kind,
                               DirStack->getConstructLoc(), ELoc))
      continue;

    if (!DirStack->addDMA(VD, RefExpr->IgnoreParens(), ACC_DMA_copyin,
                          ACC_EXPLICIT))
      Vars.push_back(RefExpr->IgnoreParens());
  }

  if (Vars.empty())
    return nullptr;

  return ACCCopyinClause::Create(Context, Kind, StartLoc, LParenLoc, EndLoc,
                                 Vars);
}

ACCClause *Sema::ActOnOpenACCCopyoutClause(
    OpenACCClauseKind Kind, ArrayRef<Expr *> VarList, SourceLocation StartLoc,
    SourceLocation LParenLoc, SourceLocation EndLoc) {
  assert(ACCCopyoutClause::isClauseKind(Kind) &&
         "expected copyout clause or alias");
  SmallVector<Expr *, 8> Vars;

  for (auto &RefExpr : VarList) {
    assert(RefExpr && "NULL expr in OpenACC copyout clause.");
    SourceLocation ELoc;
    SourceRange ERange;
    Expr *SimpleRefExpr = RefExpr;
    VarDecl *VD = getVarDeclFromVarList(*this, Kind, SimpleRefExpr, ELoc,
                                        ERange, /*AllowSubarray=*/true);
    if (!VD)
      continue;

    QualType Type = VD->getType();

    // The OpenACC 2.7 spec doesn't say, as far as I know, that a variable
    // in a copyout clause must have a complete type.  However, you cannot copy
    // data if it doesn't have a size, and the OpenMP implementation does have
    // this restriction for map clauses.
    if (RequireCompleteTypeACC(*this, Type, ACC_EXPLICIT, Kind,
                               DirStack->getConstructLoc(), ELoc))
      continue;

    // The OpenACC 3.0 spec doesn't say, as far as I know, that a const
    // variable cannot be copyout.  However, you can never initialize the
    // device copy of such a variable (except that, in the case of acc exit
    // data, another directive might have initialized it), and the value has to
    // be written back to the host.
    if (Type.isConstant(Context)) {
      Diag(ELoc, diag::err_acc_const_var_write)
          << getOpenACCName(Kind) << ERange;
      Diag(VD->getLocation(), diag::note_acc_const) << VD;
      continue;
    }

    if (!DirStack->addDMA(VD, RefExpr->IgnoreParens(), ACC_DMA_copyout,
                          ACC_EXPLICIT))
      Vars.push_back(RefExpr->IgnoreParens());
  }

  if (Vars.empty())
    return nullptr;

  return ACCCopyoutClause::Create(Context, Kind, StartLoc, LParenLoc, EndLoc,
                                  Vars);
}

ACCClause *Sema::ActOnOpenACCCreateClause(
    OpenACCClauseKind Kind, ArrayRef<Expr *> VarList, SourceLocation StartLoc,
    SourceLocation LParenLoc, SourceLocation EndLoc) {
  assert(ACCCreateClause::isClauseKind(Kind) &&
         "expected create clause or alias");
  SmallVector<Expr *, 8> Vars;

  for (auto &RefExpr : VarList) {
    assert(RefExpr && "NULL expr in OpenACC create clause.");
    SourceLocation ELoc;
    SourceRange ERange;
    Expr *SimpleRefExpr = RefExpr;
    VarDecl *VD = getVarDeclFromVarList(*this, Kind, SimpleRefExpr, ELoc,
                                        ERange, /*AllowSubarray=*/true);
    if (!VD)
      continue;

    QualType Type = VD->getType();

    // The OpenACC 2.7 spec doesn't say, as far as I know, that a variable in a
    // create clause must have a complete type.  However, you cannot allocate
    // data if it doesn't have a size, and the OpenMP implementation does have
    // this restriction for map clauses.
    if (RequireCompleteTypeACC(*this, Type, ACC_EXPLICIT, Kind,
                               DirStack->getConstructLoc(), ELoc))
      continue;

    // The OpenACC 3.0 spec doesn't say, as far as I know, that a const variable
    // cannot be create.  However, you can never initialize the device copy of
    // such a variable.
    if (Type.isConstant(Context)) {
      Diag(ELoc, diag::err_acc_const_da)
          << getOpenACCName(ACCC_create) << ERange;
      Diag(VD->getLocation(), diag::note_acc_const) << VD;
      continue;
    }

    if (!DirStack->addDMA(VD, RefExpr->IgnoreParens(), ACC_DMA_create,
                          ACC_EXPLICIT))
      Vars.push_back(RefExpr->IgnoreParens());
  }

  if (Vars.empty())
    return nullptr;

  return ACCCreateClause::Create(Context, Kind, StartLoc, LParenLoc, EndLoc,
                                 Vars);
}

ACCClause *Sema::ActOnOpenACCNoCreateClause(
    ArrayRef<Expr *> VarList, SourceLocation StartLoc, SourceLocation LParenLoc,
    SourceLocation EndLoc) {
  SmallVector<Expr *, 8> Vars;
  for (auto &RefExpr : VarList) {
    assert(RefExpr && "NULL expr in OpenACC no_create clause.");
    SourceLocation ELoc;
    SourceRange ERange;
    Expr *SimpleRefExpr = RefExpr;
    VarDecl *VD = getVarDeclFromVarList(*this, ACCC_no_create, SimpleRefExpr,
                                        ELoc, ERange, /*AllowSubarray=*/true);
    if (!VD)
      continue;

    QualType Type = VD->getType();

    // The OpenACC 3.0 spec doesn't say, as far as I know, that a variable
    // in a no_create clause must have a complete type.  However, without its
    // size, we cannot know if it's fully present.  Besides, it translates to
    // an OpenMP map clause, which does require a complete type.
    if (RequireCompleteTypeACC(*this, Type, ACC_EXPLICIT, ACCC_no_create,
                               DirStack->getConstructLoc(), ELoc))
      continue;

    if (!DirStack->addDMA(VD, RefExpr->IgnoreParens(), ACC_DMA_no_create,
                          ACC_EXPLICIT))
      Vars.push_back(RefExpr->IgnoreParens());
  }

  if (Vars.empty())
    return nullptr;

  return ACCNoCreateClause::Create(Context, StartLoc, LParenLoc, EndLoc, Vars);
}

ACCClause *Sema::ActOnOpenACCDeleteClause(ArrayRef<Expr *> VarList,
                                          SourceLocation StartLoc,
                                          SourceLocation LParenLoc,
                                          SourceLocation EndLoc) {
  SmallVector<Expr *, 8> Vars;
  for (auto &RefExpr : VarList) {
    assert(RefExpr && "NULL expr in OpenACC delete clause.");
    SourceLocation ELoc;
    SourceRange ERange;
    Expr *SimpleRefExpr = RefExpr;
    VarDecl *VD = getVarDeclFromVarList(*this, ACCC_delete, SimpleRefExpr, ELoc,
                                        ERange, /*AllowSubarray=*/true);
    if (!VD)
      continue;

    QualType Type = VD->getType();

    // The OpenACC 3.0 spec doesn't say, as far as I know, that a variable
    // in a delete clause must have a complete type.  However, without its
    // size, we cannot know if it's fully present.  Besides, it translates to
    // an OpenMP map clause, which does require a complete type.
    if (RequireCompleteTypeACC(*this, Type, ACC_EXPLICIT, ACCC_delete,
                               DirStack->getConstructLoc(), ELoc))
      continue;

    if (!DirStack->addDMA(VD, RefExpr->IgnoreParens(), ACC_DMA_delete,
                          ACC_EXPLICIT))
      Vars.push_back(RefExpr->IgnoreParens());
  }

  if (Vars.empty())
    return nullptr;

  return ACCDeleteClause::Create(Context, StartLoc, LParenLoc, EndLoc, Vars);
}

ACCClause *Sema::ActOnOpenACCSharedClause(ArrayRef<Expr *> VarList) {
  SmallVector<Expr *, 8> Vars;
  for (auto &RefExpr : VarList) {
    assert(RefExpr && "NULL expr in OpenACC implicit shared clause.");
    auto *DE = dyn_cast_or_null<DeclRefExpr>(RefExpr->IgnoreParens());
    assert(DE && "OpenACC implicit shared clause for non-DeclRefExpr");
    auto *VD = dyn_cast_or_null<VarDecl>(DE->getDecl());
    assert(VD && "OpenACC implicit shared clause for non-VarDecl");

    if (!DirStack->addDSA(VD, RefExpr->IgnoreParens(), ACC_DSA_shared,
                          ACC_IMPLICIT))
      Vars.push_back(RefExpr->IgnoreParens());
    else
      // Assert that the variable does not appear in an explicit data clause on
      // the same directive.  If the OpenACC spec grows an explicit shared
      // clause one day, this assert should be removed.
      llvm_unreachable("implicit shared clause unexpectedly generated for"
                       " variable in conflicting explicit data clause");
  }

  if (Vars.empty())
    return nullptr;

  return ACCSharedClause::Create(Context, Vars);
}

ACCClause *Sema::ActOnOpenACCPrivateClause(
    ArrayRef<Expr *> VarList, OpenACCDetermination Determination,
    SourceLocation StartLoc, SourceLocation LParenLoc, SourceLocation EndLoc) {
  SmallVector<Expr *, 8> Vars;
  for (auto &RefExpr : VarList) {
    assert(RefExpr && "NULL expr in OpenACC private clause.");
    SourceLocation ELoc;
    SourceRange ERange;
    Expr *SimpleRefExpr = RefExpr;
    VarDecl *VD = getVarDeclFromVarList(*this, ACCC_private, SimpleRefExpr,
                                        ELoc, ERange);
    if (!VD)
      continue;

    QualType Type = VD->getType();

    // The OpenACC 2.5 spec doesn't say, as far as I know, that a private
    // variable must have a complete type.  However, you cannot copy data if it
    // doesn't have a size, and OpenMP does have this restriction.
    if (RequireCompleteTypeACC(*this, Type, Determination, ACCC_private,
                               DirStack->getConstructLoc(), ELoc))
      continue;

    // The OpenACC 2.5 spec doesn't say, as far as I know, that a const
    // variable cannot be private.  However, you can never initialize the
    // private version of such a variable, and OpenMP does have this
    // restriction.
    if (Type.isConstant(Context)) {
      Diag(ELoc, diag::err_acc_const_da)
          << getOpenACCName(ACCC_private) << ERange;
      Diag(VD->getLocation(), diag::note_acc_const) << VD;
      // Implicit private is for loop control variables, which cannot be const.
      assert(Determination == ACC_EXPLICIT &&
             "unexpected const type for computed private");
      continue;
    }

    if (!DirStack->addDSA(VD, RefExpr->IgnoreParens(), ACC_DSA_private,
                          Determination))
      Vars.push_back(RefExpr->IgnoreParens());
  }

  if (Vars.empty())
    return nullptr;

  return ACCPrivateClause::Create(Context, Determination, StartLoc, LParenLoc,
                                  EndLoc, Vars);
}

ACCClause *Sema::ActOnOpenACCFirstprivateClause(
    ArrayRef<Expr *> VarList, OpenACCDetermination Determination,
    SourceLocation StartLoc, SourceLocation LParenLoc, SourceLocation EndLoc) {
  SmallVector<Expr *, 8> Vars;

  for (auto &RefExpr : VarList) {
    assert(RefExpr && "NULL expr in OpenACC firstprivate clause.");
    SourceLocation ELoc;
    SourceRange ERange;
    Expr *SimpleRefExpr = RefExpr;
    VarDecl *VD = getVarDeclFromVarList(*this, ACCC_firstprivate,
                                        SimpleRefExpr, ELoc, ERange);
    if (!VD)
      continue;

    QualType Type = VD->getType();

    // The OpenACC 2.5 spec doesn't say, as far as I know, that a private
    // variable must have a complete type.  However, you cannot copy data if it
    // doesn't have a size, and OpenMP does have this restriction.
    if (RequireCompleteTypeACC(*this, Type, Determination, ACCC_firstprivate,
                               DirStack->getConstructLoc(), ELoc))
      continue;

    if (!DirStack->addDSA(VD, RefExpr->IgnoreParens(), ACC_DSA_firstprivate,
                          Determination))
      Vars.push_back(RefExpr->IgnoreParens());
  }

  if (Vars.empty())
    return nullptr;

  return ACCFirstprivateClause::Create(Context, Determination, StartLoc,
                                       LParenLoc, EndLoc, Vars);
}

ACCClause *Sema::ActOnOpenACCReductionClause(
    ArrayRef<Expr *> VarList, OpenACCDetermination Determination,
    SourceLocation StartLoc, SourceLocation LParenLoc, SourceLocation ColonLoc,
    SourceLocation EndLoc, const DeclarationNameInfo &ReductionId) {
  DeclarationName DN = ReductionId.getName();
  OverloadedOperatorKind OOK = DN.getCXXOverloadedOperator();
  SmallVector<Expr *, 8> Vars;

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
    VarDecl *VD = getVarDeclFromVarList(*this, ACCC_reduction, SimpleRefExpr,
                                        ELoc, ERange);
    if (!VD)
      continue;

    QualType Type = VD->getType();

    // The OpenACC 2.6 spec doesn't say, as far as I know, that a private
    // variable must have a complete type.  However, you cannot copy data if
    // it doesn't have a size, and OpenMP does have this restriction.
    if (RequireCompleteTypeACC(*this, Type, Determination, ACCC_reduction,
                               DirStack->getConstructLoc(), ELoc))
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
      // Implicit reductions are copied from explicit reductions, which are
      // validated already.
      assert(Determination == ACC_EXPLICIT &&
             "unexpected const type for computed reduction");
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
    if (!DirStack->addDSA(VD, RefExpr->IgnoreParens(), ACC_DSA_reduction,
                          Determination, ReductionId))
      Vars.push_back(RefExpr);
  }
  if (Vars.empty())
    return nullptr;
  return ACCReductionClause::Create(Context, Determination, StartLoc,
                                    LParenLoc, ColonLoc, EndLoc, Vars,
                                    ReductionId);
}

ACCClause *Sema::ActOnOpenACCIfClause(Expr *Condition, SourceLocation StartLoc,
                                      SourceLocation LParenLoc,
                                      SourceLocation EndLoc) {
  // OpenACC 3.0 sec. 2.14.4 "Update Directive", "Restrictions", L2303:
  //   "in C or C++, the condition must evaluate to a scalar integer value."
  if (CheckBooleanCondition(StartLoc, Condition).isInvalid())
    return nullptr;
  return new (Context) ACCIfClause(Condition, StartLoc, LParenLoc, EndLoc);
}

ACCClause *Sema::ActOnOpenACCIfPresentClause(SourceLocation StartLoc,
                                             SourceLocation EndLoc) {
  return new (Context) ACCIfPresentClause(StartLoc, EndLoc);
}

ACCClause *Sema::ActOnOpenACCSelfClause(OpenACCClauseKind Kind,
                                        ArrayRef<Expr *> VarList,
                                        SourceLocation StartLoc,
                                        SourceLocation LParenLoc,
                                        SourceLocation EndLoc) {
  SmallVector<Expr *, 8> Vars;
  for (auto &RefExpr : VarList) {
    assert(RefExpr && "NULL expr in OpenACC self clause.");
    SourceLocation ELoc;
    SourceRange ERange;
    Expr *SimpleRefExpr = RefExpr;
    VarDecl *VD = getVarDeclFromVarList(*this, Kind, SimpleRefExpr, ELoc,
                                        ERange, /*AllowSubarray=*/true);
    if (!VD)
      continue;

    QualType Type = VD->getType();

    // The OpenACC 2.7 spec doesn't say, as far as I know, that a variable
    // in a self clause must have a complete type.  However, it could not
    // have been allocated on the device copy if it didn't have a size.
    if (RequireCompleteTypeACC(*this, Type, ACC_EXPLICIT, Kind,
                               DirStack->getConstructLoc(), ELoc))
      continue;

    // The OpenACC 3.0 spec doesn't say, as far as I know, that a const
    // variable cannot be in a self clause, but updating it seems to be a
    // violation of const.
    if (Type.isConstant(Context)) {
      Diag(ELoc, diag::err_acc_const_var_write)
          << getOpenACCName(Kind) << ERange;
      Diag(VD->getLocation(), diag::note_acc_const) << VD;
      continue;
    }

    if (!DirStack->addUpdateVar(VD, RefExpr->IgnoreParens()))
      Vars.push_back(RefExpr->IgnoreParens());
  }

  if (Vars.empty())
    return nullptr;

  return ACCSelfClause::Create(Context, Kind, StartLoc, LParenLoc, EndLoc,
                               Vars);
}

ACCClause *Sema::ActOnOpenACCDeviceClause(ArrayRef<Expr *> VarList,
                                          SourceLocation StartLoc,
                                          SourceLocation LParenLoc,
                                          SourceLocation EndLoc) {
  SmallVector<Expr *, 8> Vars;
  for (auto &RefExpr : VarList) {
    assert(RefExpr && "NULL expr in OpenACC device clause.");
    SourceLocation ELoc;
    SourceRange ERange;
    Expr *SimpleRefExpr = RefExpr;
    VarDecl *VD = getVarDeclFromVarList(*this, ACCC_device, SimpleRefExpr, ELoc,
                                        ERange, /*AllowSubarray=*/true);
    if (!VD)
      continue;

    QualType Type = VD->getType();

    // The OpenACC 2.7 spec doesn't say, as far as I know, that a variable
    // in a device clause must have a complete type.  However, it could not
    // have been allocated on the device copy if it didn't have a size.
    if (RequireCompleteTypeACC(*this, Type, ACC_EXPLICIT, ACCC_device,
                               DirStack->getConstructLoc(), ELoc))
      continue;

    // The OpenACC 3.0 spec doesn't say, as far as I know, that a const
    // variable cannot be in a device clause, but updating it seems to be a
    // violation of const.
    if (Type.isConstant(Context)) {
      Diag(ELoc, diag::err_acc_const_var_write)
          << getOpenACCName(ACCC_device) << ERange;
      Diag(VD->getLocation(), diag::note_acc_const) << VD;
      continue;
    }

    if (!DirStack->addUpdateVar(VD, RefExpr->IgnoreParens()))
      Vars.push_back(RefExpr->IgnoreParens());
  }

  if (Vars.empty())
    return nullptr;

  return ACCDeviceClause::Create(Context, StartLoc, LParenLoc, EndLoc, Vars);
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

  // ValExpr->getIntegerConstantExpr() fails if ValExpr->isValueDependent(),
  // which might be true after an error with typo correction.
  if (ValExpr->isValueDependent())
    return PosIntError;

  ValExpr = Value.get();
  // The expression must evaluate to a non-negative integer value.
  Optional<llvm::APSInt> Result =
      ValExpr->getIntegerConstantExpr(SemaRef.Context);
  if (!Result) {
    if (ErrorIfNotConst) {
      SemaRef.Diag(Loc, diag::err_acc_clause_not_ice)
          << getOpenACCName(CKind) << ValExpr->getSourceRange();
      return PosIntError;
    }
    return PosIntNonConst;
  } else if (!Result->isStrictlyPositive()) {
    SemaRef.Diag(Loc, diag::err_acc_clause_not_positive_ice)
        << getOpenACCName(CKind) << ValExpr->getSourceRange();
    return PosIntError;
  }

  return PosIntConst;
}

ACCClause *Sema::ActOnOpenACCClause(OpenACCClauseKind Kind,
                                    SourceLocation StartLoc,
                                    SourceLocation EndLoc) {
  ACCClause *Res = nullptr;
  switch (Kind) {
  case ACCC_if_present:
    Res = ActOnOpenACCIfPresentClause(StartLoc, EndLoc);
    break;
  case ACCC_seq:
    Res = ActOnOpenACCSeqClause(StartLoc, EndLoc);
    break;
  case ACCC_independent:
    Res = ActOnOpenACCIndependentClause(ACC_EXPLICIT, StartLoc, EndLoc);
    break;
  case ACCC_auto:
    Res = ActOnOpenACCAutoClause(StartLoc, EndLoc);
    break;
  case ACCC_gang:
    Res = ActOnOpenACCGangClause(ACC_EXPLICIT, StartLoc, EndLoc);
    break;
  case ACCC_worker:
    Res = ActOnOpenACCWorkerClause(StartLoc, EndLoc);
    break;
  case ACCC_vector:
    Res = ActOnOpenACCVectorClause(StartLoc, EndLoc);
    break;
  case ACCC_nomap:
  case ACCC_present:
#define OPENACC_CLAUSE_ALIAS_copy(Name) \
  case ACCC_##Name:
#define OPENACC_CLAUSE_ALIAS_copyin(Name) \
  case ACCC_##Name:
#define OPENACC_CLAUSE_ALIAS_copyout(Name) \
  case ACCC_##Name:
#define OPENACC_CLAUSE_ALIAS_create(Name) \
  case ACCC_##Name:
#include "clang/Basic/OpenACCKinds.def"
  case ACCC_no_create:
  case ACCC_delete:
  case ACCC_shared:
  case ACCC_private:
  case ACCC_firstprivate:
  case ACCC_reduction:
  case ACCC_if:
#define OPENACC_CLAUSE_ALIAS_self(Name) \
  case ACCC_##Name:
#include "clang/Basic/OpenACCKinds.def"
  case ACCC_device:
  case ACCC_num_gangs:
  case ACCC_num_workers:
  case ACCC_vector_length:
  case ACCC_collapse:
  case ACCC_unknown:
    llvm_unreachable("expected clause that accepts no arguments");
  }
  return Res;
}

ACCClause *Sema::ActOnOpenACCSeqClause(SourceLocation StartLoc,
                                       SourceLocation EndLoc) {
  return new (Context) ACCSeqClause(StartLoc, EndLoc);
}

ACCClause *Sema::ActOnOpenACCIndependentClause(
    OpenACCDetermination Determination, SourceLocation StartLoc,
    SourceLocation EndLoc) {
  return new (Context) ACCIndependentClause(Determination, StartLoc, EndLoc);
}

ACCClause *Sema::ActOnOpenACCAutoClause(SourceLocation StartLoc,
                                        SourceLocation EndLoc) {
  return new (Context) ACCAutoClause(StartLoc, EndLoc);
}

ACCClause *Sema::ActOnOpenACCGangClause(OpenACCDetermination Determination,
                                        SourceLocation StartLoc,
                                        SourceLocation EndLoc) {
  return new (Context) ACCGangClause(Determination, StartLoc, EndLoc);
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
        << getOpenACCName(ACCC_vector_length)
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
  DirStack->setAssociatedLoops(
      Collapse->EvaluateKnownConstInt(Context).getExtValue());
  return new (Context) ACCCollapseClause(Collapse, StartLoc, LParenLoc,
                                         EndLoc);
}

bool Sema::isInOpenACCDirective() {
  return static_cast<DirStackTy *>(OpenACCDirectiveStack)->getRealDirective() !=
      ACCD_unknown;
}
