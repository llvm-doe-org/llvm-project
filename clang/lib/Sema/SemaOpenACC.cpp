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

#include "UsedDeclVisitor.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/Sema/ScopeInfo.h"
#include "clang/Sema/SemaInternal.h"
using namespace clang;

//===----------------------------------------------------------------------===//
// OpenACC directive stack
//===----------------------------------------------------------------------===//

namespace {
/// Stack for tracking explicit OpenACC directives and their various properties,
/// such as data attributes.
///
/// In the case of a combined directive, we push two entries on the stack, one
/// for each effective directive.  They are not popped at the same time, so
/// outer effective directives are sometimes present when inner effective
/// directives are not.
///
/// Implicit directives are not tracked here.  For example, to determine if a
/// function \c FD has a routine directive regardless of whether it's implicit
/// or explicit, call \c FD->hasAttr<ACCRoutineDeclAttr>() instead of checking
/// this stack.  \c ImplicitRoutineDirInfoTy below stores additional info about
/// implicit routine directives.
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
    /// The start of the directive.  For a combined directive, the same location
    /// is stored with all effective directives.
    SourceLocation DirectiveStartLoc;
    /// The end of the directive, after clauses and before any associated
    /// statement or declaration.  Invalid until all clauses have been parsed.
    /// For a combined directive, the same location is stored with all effective
    /// directives.
    SourceLocation DirectiveEndLoc;
    /// The clauses.  For a combined directive, the clauses are stored only with
    /// the outer effective directive.  That way, they live as long as any part
    /// of the real directive lives.
    SmallVector<ACCClause *, 5> Clauses;
    ACCPartitioningKind LoopDirectiveKind;
    SourceLocation LoopBreakLoc; // invalid if no break statement or not loop
    unsigned AssociatedLoops = 1; // from collapse clause
    unsigned AssociatedLoopsParsed = 0; // how many have been parsed so far
    // True if this is an effective compute or loop directive and has an
    // effective nested loop directive with explicit gang partitioning or
    // encloses a function call with a routine gang directive.
    //
    // Implicit gang clauses are later added by ImplicitGangAdder after the
    // entire parallel construct is parsed and thus after this stack is popped
    // for all effective nested loop directives, so we don't bother to update
    // this then.
    bool NestedExplicitGangPartitioning = false;
    // True if this is an effective compute or loop directive and has an
    // effective nested loop directive with worker partitioning or encloses a
    // function call with a routine worker directive.
    bool NestedWorkerPartitioning = false;
    DirStackEntryTy(OpenACCDirectiveKind RealDKind,
                    OpenACCDirectiveKind EffectiveDKind,
                    SourceLocation StartLoc)
        : RealDKind(RealDKind), EffectiveDKind(EffectiveDKind),
          DirectiveStartLoc(StartLoc) {}
  };

  /// The underlying directive stack.
  SmallVector<DirStackEntryTy, 4> Stack;

public:
  explicit DirStackTy(Sema &S) : SemaRef(S) {}

  void push(OpenACCDirectiveKind RealDKind, OpenACCDirectiveKind EffectiveDKind,
            SourceLocation Loc) {
    Stack.push_back(DirStackEntryTy(RealDKind, EffectiveDKind, Loc));
  }

  void pop(OpenACCDirectiveKind EffectiveDKind) {
    assert(!Stack.empty() && "expected non-empty directive stack");
    assert(Stack.back().EffectiveDKind == EffectiveDKind &&
           "popping wrong effective OpenACC directive");
    Stack.pop_back();
  }

  void addClause(ACCClause *C) {
    assert(!Stack.empty() && "expected non-empty directive stack");
    auto I = Stack.rbegin();
    while (I->RealDKind == ACCD_unknown)
      ++I;
    I->Clauses.push_back(C);
  }

  const ArrayRef<ACCClause *> getClauses() const {
    assert(!Stack.empty() && "expected non-empty directive stack");
    auto I = Stack.rbegin();
    while (I->RealDKind == ACCD_unknown)
      ++I;
    return I->Clauses;
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
  /// Register loop partitioning for the current loop directive if
  /// \a ForCurrentDir or for a function call otherwise.
  ///
  /// As part of that, mark all effective ancestor compute or loop directives
  /// as containing any explicit gang or worker partitioning.
  void setLoopPartitioning(ACCPartitioningKind Kind, bool ForCurrentDir) {
    auto I = Stack.rbegin();
    if (ForCurrentDir) {
      assert(isOpenACCLoopDirective(getEffectiveDirective()) &&
             "expected to set partitioning for a loop directive");
      I->LoopDirectiveKind = Kind;
      ++I;
    }
    if (Kind.hasGangPartitioning() || Kind.hasWorkerPartitioning()) {
      for (auto E = Stack.rend(); I != E; ++I) {
        bool IsComputeConstruct = isOpenACCComputeDirective(I->EffectiveDKind);
        if (!isOpenACCLoopDirective(I->EffectiveDKind) && !IsComputeConstruct)
          break;
        if (Kind.hasGangPartitioning())
          I->NestedExplicitGangPartitioning = true;
        if (Kind.hasWorkerPartitioning())
          I->NestedWorkerPartitioning = true;
        // Outside a compute directive, we expect only data directives, which
        // need not be marked.
        if (IsComputeConstruct)
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
  /// Iterate through the current directive and its ancestors until finding
  /// either (1) an acc loop directive or combined loop directive with gang,
  /// worker, or vector clauses, (2) a compute directive, or (3) or the start of
  /// the stack.  If case 1, return that directive's loop partitioning kind, and
  /// record its real directive kind and location.  Else if case 2 or 3, return
  /// no partitioning, and don't record a directive kind or location.
  ACCPartitioningKind
  getAncestorLoopPartitioning(OpenACCDirectiveKind &ParentDir,
                              SourceLocation &ParentLoc,
                              bool SkipCurrentDir) const {
    auto I = Stack.rbegin();
    if (SkipCurrentDir)
      ++I;
    for (auto E = Stack.rend(); I != E; ++I) {
      if (isOpenACCComputeDirective(I->EffectiveDKind))
        return ACCPartitioningKind();
      const ACCPartitioningKind &ParentKind = I->LoopDirectiveKind;
      if (ParentKind.hasGangClause() || ParentKind.hasWorkerClause() ||
          ParentKind.hasVectorClause())
      {
        while (I->RealDKind == ACCD_unknown)
          I = std::next(I);
        ParentDir = I->RealDKind;
        ParentLoc = I->DirectiveStartLoc;
        return ParentKind;
      }
    }
    return ACCPartitioningKind();
  }
  /// Is this an effective compute or loop directive with either (1) an
  /// effective nested loop directive with explicit gang partitioning or (2) an
  /// enclosed function call with a routine gang directive?
  ///
  /// Implicit gang clauses are later added by ImplicitGangAdder after the
  /// entire parallel construct is parsed and thus after this stack is popped
  /// for all effective nested loop directives, so we don't bother to update
  /// this then.
  bool getNestedExplicitGangPartitioning() const {
    assert(!Stack.empty() && "expected non-empty directive stack");
    return Stack.back().NestedExplicitGangPartitioning;
  }
  /// Is this an effective compute or loop directive with either (1) an
  /// effective nested loop directive with worker partitioning or (2) an
  /// enclosed function call with a routine worker directive?
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
  /// declaration.  (Predetermined and implicitly determined data attributes
  /// usually have not been computed yet.)
  DAVarData getTopDA(VarDecl *VD);

  /// Returns true if the declaration has a DMA anywhere on the stack.
  /// (Predetermined and implicitly determined data attributes usually have not
  /// been computed yet.)
  bool hasVisibleDMA(VarDecl *VD);

  /// Returns true if the declaration has a privatizing DSA on any loop
  /// construct on the stack beyond the current stack entry.
  ///
  /// Fails an assertion if encounters an ancestor stack entry that is not a
  /// loop construct or a routine directive.
  ///
  /// Checks for explicit DSAs and predetermined DSAs except in the case of
  /// local variable declarations, which must be checked separately.  Loop
  /// constructs have no implicitly determined DSAs.
  bool hasPrivatizingDSAOnAncestorOrphanedLoop(VarDecl *VD);

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

  /// If the parser is currently somewhere in a compute construct, returns the
  /// real kind of the compute construct.  If the parser is currently somewhere
  /// in a function for which an applying routine directive is already known,
  /// returns \c ACCD_routine.  Otherwise, returns \c ACCD_unknown even if a
  /// routine directive will be implied later.
  OpenACCDirectiveKind isInComputeRegion() const {
    for (auto I = Stack.rbegin(); I != Stack.rend(); ++I) {
      OpenACCDirectiveKind DKind = I->RealDKind;
      if (isOpenACCComputeDirective(DKind))
        return DKind;
    }
    if (SemaRef.getCurFunctionDecl()->hasAttr<ACCRoutineDeclAttr>())
      return ACCD_routine;
    return ACCD_unknown;
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

  /// Returns the real directive for the construct enclosing the currently
  /// analyzed real directive, or returns ACCD_unknown if none.  In the former
  /// case only, set ParentLoc as the returned directive's location.  A routine
  /// directive that is lexically attached to a function is modeled as an
  /// enclosing construct.
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
    ParentLoc = I->DirectiveStartLoc;
    return I->RealDKind;
  }

  /// Returns the effective directive for the construct enclosing the currently
  /// analyzed effective directive or returns ACCD_unknown if none.  A routine
  /// directive that is lexically attached to a function is modeled as an
  /// enclosing construct.
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

  SourceLocation getDirectiveStartLoc() const {
    assert(!Stack.empty() && "expected non-empty directive stack");
    return Stack.back().DirectiveStartLoc;
  }
  SourceLocation getDirectiveEndLoc() const {
    assert(!Stack.empty() && "expected non-empty directive stack");
    return Stack.back().DirectiveEndLoc;
  }
  void setDirectiveEndLoc(SourceLocation Loc) {
    assert(!Stack.empty() && "expected non-empty directive stack");
    auto I = Stack.rbegin();
    while (I->RealDKind == ACCD_unknown) {
      assert(I->DirectiveEndLoc.isInvalid() &&
             "expected directive end location to be set only once");
      I->DirectiveEndLoc = Loc;
      I = std::next(I);
    }
    assert(I->DirectiveEndLoc.isInvalid() &&
           "expected directive end location to be set only once");
    I->DirectiveEndLoc = Loc;
  }
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

bool DirStackTy::hasPrivatizingDSAOnAncestorOrphanedLoop(VarDecl *VD) {
  VD = VD->getCanonicalDecl();
  assert(!Stack.empty() && "expected non-empty directive stack");
  for (auto I = std::next(Stack.rbegin()), E = Stack.rend(); I != E; ++I) {
    assert(
        (I->EffectiveDKind == ACCD_loop || I->EffectiveDKind == ACCD_routine) &&
        "expected all ancestor constructs to be orphaned loop constructs");
    auto DAItr = I->DAMap.find(VD);
    if (DAItr != I->DAMap.end()) {
      switch (DAItr->second.DSAKind) {
      case clang::ACC_DSA_unknown:
      case clang::ACC_DSA_shared:
        break;
      case clang::ACC_DSA_reduction:
      case clang::ACC_DSA_private:
        return true;
      case clang::ACC_DSA_firstprivate:
        llvm_unreachable("unexpected firstprivate on loop construct");
      }
    }
    if (I->LCVSet.count(VD->getCanonicalDecl()))
      return true;
  }
  return false;
}

//===----------------------------------------------------------------------===//
// OpenACC implicit routine directive info.
//===----------------------------------------------------------------------===//

namespace {
/// Info recorded about functions that at first are not known to have routine
/// directives, but routine seq directives might be implied later.
class ImplicitRoutineDirInfoTy {
private:
  Sema &SemaRef;
  struct FunctionEntryTy {
    typedef std::pair<SourceLocation, PartialDiagnostic> DiagTy;
    /// Diagnostics to report if a routine seq directive is implied for this
    /// function later.  Diagnostics are stored here by \c DiagIfRoutineDir.
    std::list<DiagTy> Diags;
    /// This function's uses of other functions such that, in each case, the use
    /// does not appear in a compute construct and neither function was known to
    /// have a routine directive when the use was parsed.
    llvm::DenseMap<FunctionDecl *, SourceLocation> HostFunctionUses;
    /// The location of the use of this function that originally implied a
    /// routine seq directive for this function.  Invalid if no routine seq
    /// directive has been implied for this function yet.
    SourceLocation UseLoc;
    /// The function in which appeared the use of this function that originally
    /// implied a routine seq directive for this function.  \c nullptr if no
    /// routine seq directive has been implied for this function yet.
    FunctionDecl *UserFn = nullptr;
    /// The compute construct in which appeared the use of this function that
    /// originally implied a routine seq directive for this function.
    /// \c ACCD_routine if that use did not appear within a compute construct.
    /// \c ACCD_unknown if no routine seq directive has been implied for this
    /// function yet.
    OpenACCDirectiveKind UserComputeConstruct = ACCD_unknown;
  };
  llvm::DenseMap<FunctionDecl *, FunctionEntryTy> Map;

  /// Emit a diagnostic that is caused by a function's routine directive.
  void emitRoutineDirDiag(FunctionDecl *FD, SourceLocation Loc,
                          PartialDiagnostic Diag) {
    SemaRef.Diag(Loc, Diag);
    if (!DiagnosticIDs::isBuiltinNote(Diag.getDiagID()))
      emitNotesForRoutineDirChain(FD);
  }

public:
  ImplicitRoutineDirInfoTy(Sema &SemaRef) : SemaRef(SemaRef) {}

  /// Record use of \a Usee at \a UseLoc within \a User such that the use does
  /// not appear in a compute construct and neither function is yet known to
  /// have a routine directive.
  void addHostFunctionUse(FunctionDecl *User, FunctionDecl *Usee,
                          SourceLocation UseLoc) {
    FunctionEntryTy &Entry = Map[User->getCanonicalDecl()];
    assert(Entry.UseLoc.isInvalid() &&
           "unexpected host function use after implicit routine directive");
    Entry.HostFunctionUses.try_emplace(Usee->getCanonicalDecl(), UseLoc);
  }

  /// Add routine seq directive implied for \a FD by its use at \a UseLoc within
  /// \a UserFn and within a compute construct of kind \a UserComputeConstruct
  /// (must be \c ACCD_routine if the use is not within a compute construct).
  ///
  /// Emit diagnostics that were previously recorded for \a FD by
  /// \c DiagIfRoutineDir.  \c emitNotesForRoutineDirChain is called for \a FD,
  /// so its preconditions must be met.
  ///
  /// Recursively add routine seq directives for all functions recorded by
  /// \c addHostFunctionUse as used by \a User.
  ///
  /// \c addImplicitRoutineSeqDir must be called at most once per function.
  void addImplicitRoutineSeqDir(
      FunctionDecl *FD, SourceLocation UseLoc, FunctionDecl *UserFn,
      OpenACCDirectiveKind UserComputeConstruct) {
    assert((isOpenACCComputeDirective(UserComputeConstruct) ||
            UserComputeConstruct == ACCD_routine) &&
           "expected use to be in compute construct or function with a routine "
           "directive");
    assert(!FD->hasAttr<ACCRoutineDeclAttr>() &&
           "expected function not to have routine directive already");
    FunctionEntryTy &Entry = Map[FD->getCanonicalDecl()];
    assert(Entry.UseLoc.isInvalid() &&
           "unexpected second implicit routine seq directive for function");
    // Add the ACCRoutineDeclAttr to the most recent FunctionDecl for FD so that
    // it's inherited by any later FunctionDecl for FD and so that we see it in
    // the recursion check below, which examines the most recent FunctionDecl.
    SemaRef.ActOnOpenACCRoutineDirective(
      {SemaRef.ActOnOpenACCSeqClause(ACC_IMPLICIT, SourceLocation(),
                                     SourceLocation())},
      ACC_IMPLICIT, SourceLocation(), SourceLocation(),
      DeclGroupRef(FD->getMostRecentDecl()));
    Entry.UseLoc = UseLoc;
    Entry.UserFn = UserFn->getCanonicalDecl();
    Entry.UserComputeConstruct = UserComputeConstruct;
    for (auto Diag : Entry.Diags)
      emitRoutineDirDiag(FD, Diag.first, Diag.second);
    Entry.Diags.clear(); // don't need them anymore
    for (auto Use : Entry.HostFunctionUses) {
      FunctionDecl *Usee = Use.first;
      SourceLocation UseeUseLoc = Use.second;
      Usee = Usee->getMostRecentDecl();
      if (Usee->hasAttr<ACCRoutineDeclAttr>())
        continue; // avoid infinite recursion
      addImplicitRoutineSeqDir(Usee, UseeUseLoc, FD, ACCD_routine);
    }
    Entry.HostFunctionUses.clear(); // don't need them anymore
  }

  /// Emit notes identifying the origin of the routine directive for \a FD.
  /// If \a PreviousDir, then point out that it was a previous routine directive
  /// to contrast with a new routine directive.
  ///
  /// If \a FD has an explicit routine directive, report it.  Otherwise, report
  /// the first use of \a FD that implied its routine seq directive.  If that
  /// use does not appear in a compute construct, recursively emit notes
  /// identifying the origin of the routine directive for the function in which
  /// the use appears.
  ///
  /// All routine directives in that chain must already be attached to the
  /// functions to which they apply, and all that are implicit must already be
  /// recorded in this \c ImplicitRoutineDirInfoTy.
  void emitNotesForRoutineDirChain(FunctionDecl *FD, bool PreviousDir = false) {
    SourceLocation Loc;
    ACCRoutineDeclAttr *Attr =
        FD->getMostRecentDecl()->getAttr<ACCRoutineDeclAttr>();
    assert(Attr && "expected function to have routine directive");
    Loc = Attr->getLoc();

    // Handle explicit routine directive.
    if (Loc.isValid()) {
      SemaRef.Diag(Loc, diag::note_acc_routine_explicit)
          << PreviousDir << FD->getName();
      return;
    }

    // Handle implicit routine directive.
    const FunctionEntryTy &Entry = Map.lookup(FD->getCanonicalDecl());
    assert(Entry.UseLoc.isValid() && Entry.UserFn &&
           Entry.UserComputeConstruct != ACCD_unknown &&
           "expected function to have implicit seq routine directive");
    bool UserIsFunction = Entry.UserComputeConstruct == ACCD_routine;
    StringRef UserName = UserIsFunction
                             ? Entry.UserFn->getName()
                             : getOpenACCName(Entry.UserComputeConstruct);
    SemaRef.Diag(Entry.UseLoc, diag::note_acc_routine_seq_implicit)
        << PreviousDir << FD->getName() << UserIsFunction << UserName;
    if (UserIsFunction)
      emitNotesForRoutineDirChain(Entry.UserFn, /*PreviousDir=*/false);
  }
  /// Return the location of the use of this function that originally implied a
  /// routine seq directive for this function, or return an invalid location if
  /// no routine seq directive has been implied for this function yet.
  SourceLocation getUseLoc(FunctionDecl *FD) const {
    return Map.lookup(FD->getCanonicalDecl()).UseLoc;
  }

  /// A diagnostic for a function that is emitted either immediately if the
  /// function is already known to have a routine directive or later when a
  /// routine seq directive is implied for it, if ever.  In either case, notes
  /// identifying the origin of the routine directive are emitted afterward
  /// unless the diagnostic itself is a note.
  ///
  /// Instances cannot be copied.  Immediate emission or storage for later
  /// emission occurs upon destruction.
  class DiagIfRoutineDir : public PartialDiagnostic {
  private:
    ImplicitRoutineDirInfoTy &ImplicitRoutineDirInfo;
    FunctionDecl *FD;
    SourceLocation DiagLoc;
    bool *Emitted;

  public:
    /// The diagnostic to be emitted is \a DiagID at \a DiagLoc for function
    /// \a FD.
    ///
    /// If \a Emitted is not \c nullptr, \a *Emitted is set to true or false
    /// upon destruction to indicate whether the diagnostic was emitted
    /// immediately.
    DiagIfRoutineDir(ImplicitRoutineDirInfoTy &ImplicitRoutineDirInfo,
                     FunctionDecl *FD, SourceLocation DiagLoc, unsigned DiagID,
                     bool *Emitted = nullptr)
        : PartialDiagnostic(ImplicitRoutineDirInfo.SemaRef.PDiag(DiagID)),
          ImplicitRoutineDirInfo(ImplicitRoutineDirInfo), FD(FD),
          DiagLoc(DiagLoc), Emitted(Emitted) {}
    DiagIfRoutineDir(const DiagIfRoutineDir &) = delete;
    DiagIfRoutineDir &operator=(const DiagIfRoutineDir &) = delete;
    ~DiagIfRoutineDir() {
      if (FD->hasAttr<ACCRoutineDeclAttr>()) {
        ImplicitRoutineDirInfo.emitRoutineDirDiag(FD, DiagLoc, *this);
        if (Emitted)
          *Emitted = true;
        return;
      }
      if (Emitted)
        *Emitted = false;
      FunctionEntryTy &Entry =
          ImplicitRoutineDirInfo.Map[FD->getCanonicalDecl()];
      assert(Entry.UseLoc.isInvalid() &&
             "unexpected diagnostic after implicit routine directive");
      Entry.Diags.emplace_back(DiagLoc, *this);
    }
  };
};
} // namespace

using DiagIfRoutineDir = ImplicitRoutineDirInfoTy::DiagIfRoutineDir;

//===----------------------------------------------------------------------===//
// Container for above OpenACC data.
//===----------------------------------------------------------------------===//

struct Sema::OpenACCDataTy {
  /// Set to true to disable OpenACC actions while building OpenMP.
  bool TransformingOpenACC = false;
  DirStackTy DirStack;
  ImplicitRoutineDirInfoTy ImplicitRoutineDirInfo;
  OpenACCDataTy(Sema &SemaRef)
      : DirStack(SemaRef), ImplicitRoutineDirInfo(SemaRef) {}
};

//===----------------------------------------------------------------------===//
// Sema functions.
//===----------------------------------------------------------------------===//

void Sema::InitOpenACCData() { OpenACCData = new OpenACCDataTy(*this); }

void Sema::DestroyOpenACCData() { delete OpenACCData; }

bool Sema::StartOpenACCDirectiveAndAssociate(OpenACCDirectiveKind RealDKind,
                                             SourceLocation StartLoc) {
  DirStackTy &DirStack = OpenACCData->DirStack;
  // Push onto the directive stack.
  switch (RealDKind) {
  case ACCD_update:
  case ACCD_enter_data:
  case ACCD_exit_data:
  case ACCD_data:
  case ACCD_parallel:
  case ACCD_loop:
  case ACCD_routine:
    DirStack.push(RealDKind, RealDKind, StartLoc);
    break;
  case ACCD_parallel_loop:
    DirStack.push(RealDKind, ACCD_parallel, StartLoc);
    DirStack.push(ACCD_unknown, ACCD_loop, StartLoc);
    break;
  case ACCD_unknown:
    llvm_unreachable("expected OpenACC directive");
  }
  PushExpressionEvaluationContext(
      ExpressionEvaluationContext::PotentiallyEvaluated);

  // Check directive nesting.
  SourceLocation ParentLoc;
  OpenACCDirectiveKind ParentDKind = DirStack.getRealParentDirective(ParentLoc);
  FunctionDecl *CurFnDecl = getCurFunctionDecl();
  if (CurFnDecl && !isOpenACCDirectiveStmt(ParentDKind)) {
    assert((ParentDKind == ACCD_unknown || ParentDKind == ACCD_routine) &&
           "expected the only ACCDirectiveStmt to be the routine directive");
    ACCRoutineDeclAttr *Attr = CurFnDecl->getAttr<ACCRoutineDeclAttr>();
    assert((ParentDKind != ACCD_routine ||
            (Attr && ParentLoc == Attr->getLoc())) &&
           "expected lexically attached routine directive to be attached as "
           "ACCRoutineDeclAttr to FunctionDecl");
    assert(isAllowedParentForDirective(RealDKind, ACCD_unknown) &&
           "expected every OpenACC directive to be permitted without a "
           "lexically enclosing OpenACC directive");
    if (!isAllowedParentForDirective(RealDKind, ACCD_routine)) {
      bool Err;
      DiagIfRoutineDir(OpenACCData->ImplicitRoutineDirInfo, CurFnDecl, StartLoc,
                       diag::err_acc_routine_unexpected_directive, &Err)
          << getOpenACCName(RealDKind) << CurFnDecl->getName();
      return Err;
    }
    assert(RealDKind == ACCD_loop &&
           "expected only orphaned loop to be permitted in function with "
           "routine directive");
    // Proposed text for OpenACC after 3.2:
    // "A procedure definition containing an orphaned loop construct must be in
    // the scope of an explicit and applying routine directive."
    if (!Attr || !Attr->getLoc().isValid()) {
      Diag(StartLoc, diag::err_acc_routine_for_orphaned_loop)
          << CurFnDecl->getName();
      return true;
    }
    return false;
  }
  if (!isAllowedParentForDirective(RealDKind, ParentDKind)) {
    assert(ParentDKind != ACCD_unknown &&
           "expected only loop construct to require parent construct");
    Diag(StartLoc, diag::err_acc_directive_bad_nesting)
        << getOpenACCName(ParentDKind) << getOpenACCName(RealDKind);
    Diag(ParentLoc, diag::note_acc_enclosing_directive)
        << getOpenACCName(ParentDKind);
    return true;
  }
  return false;
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

void Sema::AddOpenACCClause(ACCClause *Clause) {
  OpenACCData->DirStack.addClause(Clause);
}

void Sema::EndOpenACCDirective(SourceLocation EndLoc) {
  OpenACCData->DirStack.setDirectiveEndLoc(EndLoc);
}

void Sema::EndOpenACCDirectiveAndAssociate(OpenACCDirectiveKind RealDKind) {
  DirStackTy &DirStack = OpenACCData->DirStack;
  assert(RealDKind == DirStack.getRealDirective() &&
         "ending wrong real OpenACC directive");
  // For a combined directive, the first DirStack.pop() happens after the
  // inner effective directive is "acted upon" (AST node is constructed), so
  // this is the second DirStack.pop(), which happens after the entire
  // combined directive is acted upon.  However, if there was an error, we
  // need to pop the entire combined directive.
  for (OpenACCDirectiveKind DKind = ACCD_unknown; DKind == ACCD_unknown;
       DirStack.pop(DKind))
    DKind = DirStack.getEffectiveDirective();
  DiscardCleanupsInEvaluationContext();
  PopExpressionEvaluationContext();
}

namespace {
/// Per \c Stmt helper for \c RoutineUseReporter.
class RoutineUsedDeclVisitor : public UsedDeclVisitor<RoutineUsedDeclVisitor> {
private:
  FunctionDecl *FD;
  bool FoundUse;

public:
  void visitUsedDecl(SourceLocation Loc, Decl *D) {
    if (D->getCanonicalDecl() != FD->getCanonicalDecl())
      return;
    FoundUse = true;
    S.Diag(Loc, diag::note_acc_routine_use) << FD->getName();
  }
  RoutineUsedDeclVisitor(Sema &SemaRef, FunctionDecl *FD)
      : UsedDeclVisitor<RoutineUsedDeclVisitor>(SemaRef), FD(FD),
        FoundUse(false) {}
  bool foundUse() const { return FoundUse; }
};

/// Emits note diagnostics for previous uses of a function.
class RoutineUseReporter : public RecursiveASTVisitor<RoutineUseReporter> {
private:
  RoutineUsedDeclVisitor RUDV;

public:
  bool TraverseStmt(Stmt *S) {
    if (S)
      RUDV.Visit(S);
    return true;
  }
  RoutineUseReporter(Sema &SemaRef, FunctionDecl *FD) : RUDV(SemaRef, FD) {}
  bool foundUse() const { return RUDV.foundUse(); }
};

/// See the section "Implicit Gang Clauses" in the Clang OpenACC design
/// document.
class ImplicitGangAdder : public StmtVisitor<ImplicitGangAdder> {
public:
  void VisitACCDirectiveStmt(ACCDirectiveStmt *D) {
    if (isOpenACCLoopDirective(D->getDirectiveKind())) {
      auto *LD = cast<ACCLoopDirective>(D);
      ACCPartitioningKind Part = LD->getPartitioning();
      // If this loop is gang-partitioned, don't add an implicit gang clause,
      // which would be redundant, and don't continue to descendants because
      // they cannot then be gang-partitioned.
      if (Part.hasGangPartitioning())
        return;
      // We haven't encountered any enclosing gang-partitioned loop.  If there's
      // also no enclosed gang-partitioned loop and no call to a function with a
      // routine gang directive, and if this loop has been determined to be
      // independent, then this is the outermost loop meeting all those
      // conditions.  Thus, an implicit gang clause belongs here, and don't
      // continue to descendants because they cannot then be gang-partitioned.
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
  DirStackTy &DirStack;
  ImplicitDATable &ImplicitDAs;
  llvm::SmallVector<llvm::DenseSet<const Decl *>, 8> LocalDefinitions;
  size_t ACCDirectiveStmtCount;
  std::list<const DeclRefExpr *> ScalarReductionVarDiags;
  llvm::DenseMap<const VarDecl *, SourceLocation> ScalarGangReductionVarDiags;

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
      const DirStackTy::DAVarData &DVar = DirStack.getTopDA(VD);

      // Compute implicit DAs.
      VD = VD->getCanonicalDecl();
      OpenACCDMAKind DMAKind = ACC_DMA_unknown;
      OpenACCDSAKind DSAKind = ACC_DSA_unknown;
      if (isOpenACCLoopDirective(DirStack.getEffectiveDirective())) {
        if (DVar.DSAKind == ACC_DSA_unknown) {
          // OpenACC 3.0 sec. 2.6.1 "Variables with Predetermined Data
          // Attributes" L1038-1039:
          //   "The loop variable in a C for statement [...] that is associated
          //   with a loop directive is predetermined to be private to each
          //   thread that will execute each iteration of the loop."
          //
          // See the section "Loop Control Variables" in the Clang OpenACC
          // design document for the interpretation used here.
          // Sema::ActOnOpenACCDirectiveStmt handles the case without a seq
          // clause.
          assert((!DirStack.hasLoopControlVariable(VD) ||
                  DirStack.getLoopPartitioning().hasSeqExplicit()) &&
                 "expected predetermined private for loop control variable "
                 "with explicit seq");
          // See the section "Basic Data Attributes" in the Clang OpenACC
          // design document for discussion of the shared data attribute.
          DSAKind = ACC_DSA_shared;
        }
      } else if (isOpenACCParallelDirective(DirStack.getEffectiveDirective())) {
        if (DirStack.hasVisibleDMA(VD) || DVar.DSAKind != ACC_DSA_unknown) {
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
  void VisitACCDirectiveStmt(ACCDirectiveStmt *D) {
    ++ACCDirectiveStmtCount;

    // Push space for local definitions in this construct.  It's important to
    // force a copy construct call here so that, if the vector is resized, the
    // pass-by-reference parameter isn't invalidated before it's copied.
    LocalDefinitions.emplace_back(
        llvm::DenseSet<const Decl *>(LocalDefinitions.back()));

    // OpenACC 3.2 sec. 2.6.2 "Variables with Implicitly Determined Data
    // Attributes":
    // - L1235-1239: "If a scalar variable appears in a reduction clause on a
    //   loop construct that has a parent parallel or serial construct, and if
    //   the reduction's access to the original variable is exposed to the
    //   parent compute construct, the variable must appear either in an
    //   explicit data clause visible at the compute construct or in a
    //   firstprivate, private, or reduction clause on the compute construct."
    // - L1213-1214: "On a compute or combined construct, if a variable appears
    //   in a reduction clause but no other data clause, it is treated as if it
    //   also appears in a copy clause."
    //
    // Here, we diagnose violations of the restriction from the first statement
    // above.  However, we do not enforce it when the loop and compute construct
    // are combined because that would make the second statement above useless
    // for scalar variables in acc parallel/serial loop reductions.  TODO: The
    // OpenACC spec should be clarified on that point.
    bool ScalarReductionRequiresDataClause =
        isOpenACCComputeDirective(DirStack.getEffectiveDirective()) &&
        isOpenACCLoopDirective(D->getDirectiveKind()) &&
        (!isOpenACCCombinedDirective(DirStack.getRealDirective()) ||
         ACCDirectiveStmtCount > 1);
    // Currently, Clang supports no compute construct but the parallel
    // construct.  The serial construct is handled the same but the kernels
    // construct is not.
    assert((!ScalarReductionRequiresDataClause ||
            DirStack.getEffectiveDirective() == ACCD_parallel) &&
           "expected compute construct to be parallel construct");
    bool IsGangReduction =
        cast<ACCLoopDirective>(D)->getPartitioning().hasGangPartitioning();

    // Search this directive's clauses for reductions requiring data clauses and
    // for privatizations.
    for (ACCClause *C : D->clauses()) {
      if (!C)
        continue;

      // Record diagnostics for missing data clauses for scalar loop reductions.
      if (ScalarReductionRequiresDataClause &&
          C->getClauseKind() == ACCC_reduction) {
        for (Expr *VR : cast<ACCReductionClause>(C)->varlists()) {
          DeclRefExpr *DRE = cast<DeclRefExpr>(VR);
          VarDecl *VD = cast<VarDecl>(DRE->getDecl())->getCanonicalDecl();
          // Skip variable if it is locally declared or privatized by a clause.
          if (LocalDefinitions.back().count(VD))
            continue;
          // Currently, Clang supports only scalar variables in OpenACC
          // reductions.
          assert(VD->getType()->isScalarType() &&
                 "expected reduction variable to be scalar");
          // If there's a visible explicit DMA or if there's an explicit DSA on
          // the compute construct, the restriction is satisfied.
          DirStackTy::DAVarData DVar = DirStack.getTopDA(VD);
          if (DirStack.hasVisibleDMA(VD) || DVar.DSAKind != ACC_DSA_unknown)
            continue;
          // Record diagnostic.
          ScalarReductionVarDiags.push_back(DRE);
          if (IsGangReduction)
            ScalarGangReductionVarDiags.try_emplace(VD, DRE->getExprLoc());
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

    --ACCDirectiveStmtCount;
  }
  void VisitStmt(Stmt *S) {
    for (Stmt *C : S->children()) {
      if (C)
        Visit(C);
    }
  }

  ImplicitDAAdder(DirStackTy &DirStack, ImplicitDATable &T)
      : DirStack(DirStack), ImplicitDAs(T), ACCDirectiveStmtCount(0) {
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
    if (DirStack.getEffectiveDirective() == ACCD_parallel) {
      for (Expr *VR : DirStack.getReductionVars()) {
        DeclRefExpr *DRE = cast<DeclRefExpr>(VR);
        VarDecl *VD = cast<VarDecl>(DRE->getDecl())->getCanonicalDecl();
        const DirStackTy::DAVarData &DVar = DirStack.getTopDA(VD);
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
  void reportDiags() const {
    for (const DeclRefExpr *DRE : ScalarReductionVarDiags) {
      const VarDecl *VD = cast<VarDecl>(DRE->getDecl())->getCanonicalDecl();
      DirStack.SemaRef.Diag(DRE->getExprLoc(),
                            diag::err_acc_loop_reduction_needs_data_clause)
          << VD->getName() << getOpenACCName(DirStack.getRealDirective());
      DirStack.SemaRef.Diag(DirStack.getDirectiveStartLoc(),
                            diag::note_acc_parent_compute_construct)
          << getOpenACCName(DirStack.getRealDirective());
      SourceLocation GangRedLoc = ScalarGangReductionVarDiags.lookup(VD);
      // OpenACC compilers typically implicitly determine copy clauses for gang
      // reductions, which are not so useful otherwise, so suggest that if there
      // is a gang reduction for this variable.  Otherwise, suggest
      // firstprivate, which makes sense when it's not reduced across gangs.
      if (GangRedLoc.isInvalid()) {
        DirStack.SemaRef.Diag(
            DirStack.getDirectiveStartLoc(),
            diag::note_acc_loop_reduction_suggest_firstprivate)
            << VD->getName() << getOpenACCName(DirStack.getRealDirective());
        continue;
      }
      DirStack.SemaRef.Diag(GangRedLoc,
                            diag::note_acc_loop_reduction_suggest_copy)
          << VD->getName();
    }
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
  DirStackTy &DirStack;
  ImplicitDATable &ImplicitDAs;
  llvm::SmallVector<llvm::DenseSet<const Decl *>, 8> LocalDefinitions;

public:
  void VisitDeclStmt(DeclStmt *S) {
    for (auto I = S->decl_begin(), E = S->decl_end(); I != E; ++I)
      LocalDefinitions.back().insert(*I);
    BaseVisitor::VisitDeclStmt(S);
  }
  void VisitACCDirectiveStmt(ACCDirectiveStmt *D) {
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
          DirStackTy::DAVarData DVar = DirStack.getTopDA(VD);
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
        continue;
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
  ImplicitGangReductionAdder(DirStackTy &DirStack, ImplicitDATable &T)
      : DirStack(DirStack), ImplicitDAs(T) {
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
  DirStackTy &DirStack;
  llvm::SmallVector<llvm::DenseMap<VarDecl *, ReductionVar>, 8> Privates;
  bool Error = false;

public:
  void VisitACCDirectiveStmt(ACCDirectiveStmt *D) {
    // Push space for privates in this construct.  It's important to force a
    // copy construct call here so that, if the vector is resized, the
    // pass-by-reference parameter isn't invalidated before it's copied.
    Privates.emplace_back(
        llvm::DenseMap<VarDecl *, ReductionVar>(Privates.back()));

    // Record variables privatized by this directive, and complain for any
    // reduction conflicting with its immediately enclosing reduction.
    for (ACCClause *C : D->clauses()) {
      if (!C)
        continue;
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
            DirStack.SemaRef.Diag(DRE->getExprLoc(),
                                  diag::err_acc_conflicting_reduction)
                << false
                << ACCReductionClause::printReductionOperatorToString(
                       ReductionClause->getNameInfo())
                << VD;
            DirStack.SemaRef.Diag(EnclosingPrivate.RE->getExprLoc(),
                                  diag::note_acc_enclosing_reduction)
                << ACCReductionClause::printReductionOperatorToString(
                       EnclosingPrivate.ReductionClause->getNameInfo());
            if (EnclosingPrivate.ReductionClause->getBeginLoc().isInvalid())
              DirStack.SemaRef.Diag(DirStack.getDirectiveStartLoc(),
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
  NestedReductionChecker(DirStackTy &DirStack) : DirStack(DirStack) {
    Privates.emplace_back();
  }
};
} // namespace

bool Sema::StartOpenACCAssociatedStatement() {
  DirStackTy &DirStack = OpenACCData->DirStack;
  OpenACCDirectiveKind DKind = DirStack.getRealDirective();
  SourceLocation StartLoc = DirStack.getDirectiveStartLoc();
  bool ErrorFound = false;
  if (isOpenACCLoopDirective(DKind)) {
    ACCPartitioningKind LoopKind;
    // Set implicit partitionability.
    //
    // OpenACC 3.2, sec. 2.9.6 "independent clause":
    // "A loop construct with no auto or seq clause is treated as if it has the
    // independent clause when it is an orphaned loop construct or its parent
    // compute construct is a parallel construct."
    LoopKind.setIndependentImplicit();

    // Set explicit clauses.
    for (ACCClause *C : OpenACCData->DirStack.getClauses()) {
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

    // Check any level-of-parallelism clause against any parent loop construct
    // or the enclosing function.
    ACCRoutineDeclAttr::PartitioningTy LoopLevel =
        LoopKind.getMaxParallelismLevel();
    if (LoopLevel != ACCRoutineDeclAttr::Seq) {
      OpenACCDirectiveKind ParentDKind;
      SourceLocation ParentLoopLoc;
      ACCRoutineDeclAttr::PartitioningTy ParentLoopLevel =
          OpenACCData->DirStack
              .getAncestorLoopPartitioning(ParentDKind, ParentLoopLoc,
                                           /*SkipCurrentDir=*/true)
              .getMinParallelismLevel();
      if (ParentLoopLevel != ACCRoutineDeclAttr::Seq) {
        // There's a parent loop with a level-of-parallelism clause.  Complain
        // if it's incompatible.
        if (ParentLoopLevel <= LoopLevel) {
          Diag(StartLoc, diag::err_acc_loop_loop_par_level)
              << getOpenACCName(ParentDKind)
              << ACCRoutineDeclAttr::ConvertPartitioningTyToStr(ParentLoopLevel)
              << getOpenACCName(DKind)
              << ACCRoutineDeclAttr::ConvertPartitioningTyToStr(LoopLevel);
          Diag(ParentLoopLoc, diag::note_acc_enclosing_directive)
              << getOpenACCName(ParentDKind);
          ErrorFound = true;
        }
      } else if (!isOpenACCComputeDirective(DirStack.isInComputeRegion())) {
        // We have an orphaned loop construct.  If there's no routine directive
        // for the enclosing function, we already complained about that.  If
        // there is, complain if the level of parallelism is incompatible.
        FunctionDecl *Fn = getCurFunctionDecl();
        assert(Fn && "expected acc loop to be in a function");
        ACCRoutineDeclAttr *FnAttr = Fn->getAttr<ACCRoutineDeclAttr>();
        if (FnAttr) {
          ACCRoutineDeclAttr::PartitioningTy FnLevel =
              FnAttr->getPartitioning();
          if (FnLevel < LoopLevel) {
            Diag(StartLoc, diag::err_acc_loop_func_par_level)
                << Fn->getName()
                << ACCRoutineDeclAttr::ConvertPartitioningTyToStr(FnLevel)
                << getOpenACCName(DKind)
                << ACCRoutineDeclAttr::ConvertPartitioningTyToStr(LoopLevel);
            OpenACCData->ImplicitRoutineDirInfo.emitNotesForRoutineDirChain(Fn);
          }
        }
      }
    }

    // TODO: For now, we prescriptively map auto to sequential execution, but
    // obviously an OpenACC compiler is meant to sometimes determine that
    // independent is possible.
    if (LoopKind.hasAuto())
      LoopKind.setSeqComputed();

    // Record partitioning on stack.
    OpenACCData->DirStack.setLoopPartitioning(LoopKind,
                                              /*ForCurrentDir=*/true);
  }
  return ErrorFound;
}

bool Sema::EndOpenACCAssociatedStatement() { return false; }

void Sema::StartOpenACCTransform() { OpenACCData->TransformingOpenACC = true; }
void Sema::EndOpenACCTransform() { OpenACCData->TransformingOpenACC = false; }

StmtResult Sema::ActOnOpenACCDirectiveStmt(Stmt *AStmt) {
  DirStackTy &DirStack = OpenACCData->DirStack;
  return ActOnOpenACCDirectiveStmt(DirStack.getRealDirective(),
                                   DirStack.getClauses(), AStmt);
}

StmtResult Sema::ActOnOpenACCDirectiveStmt(OpenACCDirectiveKind DKind,
                                           ArrayRef<ACCClause *> Clauses,
                                           Stmt *AStmt) {
  DirStackTy &DirStack = OpenACCData->DirStack;
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
      return ActOnOpenACCParallelLoopDirective(Clauses, AStmt);
    case ACCD_update:
    case ACCD_enter_data:
    case ACCD_exit_data:
    case ACCD_data:
    case ACCD_parallel:
    case ACCD_loop:
    case ACCD_routine:
    case ACCD_unknown:
      llvm_unreachable("expected combined OpenACC directive");
    }
  }

  // Compute implicit independent clause.
  llvm::SmallVector<ACCClause *, 8> ComputedClauses;
  bool ErrorFound = false;
  ComputedClauses.append(Clauses.begin(), Clauses.end());
  ACCPartitioningKind LoopKind = DirStack.getLoopPartitioning();
  if (LoopKind.hasIndependentImplicit()) {
    ACCClause *Implicit = ActOnOpenACCIndependentClause(
        ACC_IMPLICIT, SourceLocation(), SourceLocation());
    assert(Implicit);
    ComputedClauses.push_back(Implicit);
  }

  // Complain for break statement in loop with independent clause.
  if (LoopKind.hasIndependent()) {
    SourceLocation BreakLoc = DirStack.getLoopBreakStatement();
    if (BreakLoc.isValid()) {
      Diag(BreakLoc, diag::err_acc_loop_cannot_use_stmt) << "break";
      ErrorFound = true;
    }
    // TODO: In the case of auto, break statements will force sequential
    // execution, probably with a warning, or is an error better for that
    // case?
  }

  // Compute implicit gang clauses within compute constructs, predetermined DAs,
  // and implicit DAs.  Complain about reductions for loop control variables.
  if (AStmt && (isOpenACCParallelDirective(DKind) ||
                isOpenACCLoopDirective(DKind))) {
    // Add implicit gang clauses to acc loops nested in the acc parallel.
    if (isOpenACCParallelDirective(DKind))
      ImplicitGangAdder().Visit(AStmt);

    // Iterate acc loop control variables.
    llvm::SmallVector<Expr *, 8> PrePrivate;
    for (std::pair<Expr *, VarDecl *> LCV :
         DirStack.getLoopControlVariables()) {
      Expr *RefExpr = LCV.first;
      VarDecl *VD = LCV.second;
      const DirStackTy::DAVarData &DVar = DirStack.getTopDA(VD);

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
      if (!DirStack.getLoopPartitioning().hasSeqExplicit() &&
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
    Adder.reportDiags();

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
    Res = ActOnOpenACCUpdateDirective(ComputedClauses);
    break;
  case ACCD_enter_data:
    assert(!AStmt &&
           "expected no associated statement for 'acc enter data' directive");
    Res = ActOnOpenACCEnterDataDirective(ComputedClauses);
    break;
  case ACCD_exit_data:
    assert(!AStmt &&
           "expected no associated statement for 'acc exit data' directive");
    Res = ActOnOpenACCExitDataDirective(ComputedClauses);
    break;
  case ACCD_data:
    Res = ActOnOpenACCDataDirective(ComputedClauses, AStmt);
    break;
  case ACCD_parallel:
    Res = ActOnOpenACCParallelDirective(ComputedClauses, AStmt);
    if (!Res.isInvalid()) {
      NestedReductionChecker Checker(DirStack);
      Checker.Visit(Res.get());
      if (Checker.hasError())
        ErrorFound = true;
    }
    break;
  case ACCD_loop: {
    SmallVector<VarDecl *, 5> LCVVars;
    for (std::pair<Expr *, VarDecl *> LCV : DirStack.getLoopControlVariables())
      LCVVars.push_back(LCV.second);
    Res = ActOnOpenACCLoopDirective(ComputedClauses, AStmt, LCVVars);
    break;
  }
  case ACCD_parallel_loop:
    llvm_unreachable("expected non-combined OpenACC directive");
  case ACCD_routine:
    llvm_unreachable("expected OpenACC executable directive or construct");
  case ACCD_unknown:
    llvm_unreachable("expected OpenACC directive");
  }

  if (ErrorFound)
    return StmtError();

  return Res;
}

StmtResult Sema::ActOnOpenACCUpdateDirective(ArrayRef<ACCClause *> Clauses) {
  DirStackTy &DirStack = OpenACCData->DirStack;
  SourceLocation StartLoc = DirStack.getDirectiveStartLoc();
  SourceLocation EndLoc = DirStack.getDirectiveEndLoc();

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

StmtResult Sema::ActOnOpenACCEnterDataDirective(ArrayRef<ACCClause *> Clauses) {
  DirStackTy &DirStack = OpenACCData->DirStack;
  SourceLocation StartLoc = DirStack.getDirectiveStartLoc();
  SourceLocation EndLoc = DirStack.getDirectiveEndLoc();

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

StmtResult Sema::ActOnOpenACCExitDataDirective(ArrayRef<ACCClause *> Clauses) {
  DirStackTy &DirStack = OpenACCData->DirStack;
  SourceLocation StartLoc = DirStack.getDirectiveStartLoc();
  SourceLocation EndLoc = DirStack.getDirectiveEndLoc();

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

StmtResult Sema::ActOnOpenACCDataDirective(ArrayRef<ACCClause *> Clauses,
                                           Stmt *AStmt) {
  if (!AStmt)
    return StmtError();
  DirStackTy &DirStack = OpenACCData->DirStack;
  SourceLocation StartLoc = DirStack.getDirectiveStartLoc();
  SourceLocation EndLoc = DirStack.getDirectiveEndLoc();

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

StmtResult Sema::ActOnOpenACCParallelDirective(ArrayRef<ACCClause *> Clauses,
                                               Stmt *AStmt) {
  if (!AStmt)
    return StmtError();
  getCurFunction()->setHasBranchProtectedScope();
  DirStackTy &DirStack = OpenACCData->DirStack;
  return ACCParallelDirective::Create(
      Context, DirStack.getDirectiveStartLoc(), DirStack.getDirectiveEndLoc(),
      Clauses, AStmt, DirStack.getNestedWorkerPartitioning());
}

void Sema::ActOnOpenACCLoopInitialization(SourceLocation ForLoc, Stmt *Init) {
  DirStackTy &DirStack = OpenACCData->DirStack;
  assert(getLangOpts().OpenACC && "OpenACC is not active.");
  assert(Init && "Expected loop in canonical form.");
  if (DirStack.getAssociatedLoopsParsed() < DirStack.getAssociatedLoops() &&
      isOpenACCLoopDirective(DirStack.getEffectiveDirective())) {
    if (Expr *E = dyn_cast<Expr>(Init))
      Init = E->IgnoreParens();
    if (auto *BO = dyn_cast<BinaryOperator>(Init)) {
      if (BO->getOpcode() == BO_Assign) {
        auto *LHS = BO->getLHS()->IgnoreParens();
        if (auto *DRE = dyn_cast<DeclRefExpr>(LHS))
          DirStack.addLoopControlVariable(DRE);
      }
    }
    DirStack.incAssociatedLoopsParsed();
  }
}

void Sema::ActOnOpenACCLoopBreakStatement(SourceLocation BreakLoc,
                                          Scope *CurScope) {
  assert(getLangOpts().OpenACC && "OpenACC is not active.");
  if (Scope *S = CurScope->getBreakParent()) {
    if (S->isOpenACCLoopScope())
      OpenACCData->DirStack.addLoopBreakStatement(BreakLoc);
  }
}

void Sema::ActOnStartOfFunctionDefForOpenACC(FunctionDecl *FD) {
  // If a routine directive was just pushed onto the directive stack, then that
  // routine directive is lexically attached to (and applies to) FD, which we
  // just started parsing after pushing.  Attach the ACCDeclAttr to FD now.
  if (OpenACCData->DirStack.getEffectiveDirective() == ACCD_routine)
    ActOnOpenACCRoutineDirective(ACC_EXPLICIT, DeclGroupRef(FD));
}
void Sema::ActOnFunctionUseForOpenACC(FunctionDecl *Usee,
                                      SourceLocation UseLoc) {
  if (OpenACCData->TransformingOpenACC)
    return;
  DirStackTy &DirStack = OpenACCData->DirStack;
  ImplicitRoutineDirInfoTy &ImplicitRoutineDirInfo =
      OpenACCData->ImplicitRoutineDirInfo;

  // If Usee has no routine directive yet, then add an implicit routine seq
  // directive if it's an accelerator use, and record as host use otherwise.
  // Either way, seq is the highest level of parallelism Usee might eventually
  // have, so any level of parallelism at the use site is compatible.
  ACCRoutineDeclAttr *UseeAttr = Usee->getAttr<ACCRoutineDeclAttr>();
  if (!UseeAttr) {
    OpenACCDirectiveKind ComputeDKind = DirStack.isInComputeRegion();
    if (ComputeDKind != ACCD_unknown) {
      ImplicitRoutineDirInfo.addImplicitRoutineSeqDir(
          Usee, UseLoc, getCurFunctionDecl(), ComputeDKind);
      return;
    }
    ImplicitRoutineDirInfo.addHostFunctionUse(getCurFunctionDecl(), Usee,
                                              UseLoc);
  }
}
void Sema::ActOnCallExprForOpenACC(CallExpr *Call) {
  if (OpenACCData->TransformingOpenACC)
    return;
  DirStackTy &DirStack = OpenACCData->DirStack;
  ImplicitRoutineDirInfoTy &ImplicitRoutineDirInfo =
      OpenACCData->ImplicitRoutineDirInfo;
  FunctionDecl *Callee = Call->getDirectCallee();
  if (!Callee)
    return;
  ACCRoutineDeclAttr *CalleeAttr = Callee->getAttr<ACCRoutineDeclAttr>();
  if (!CalleeAttr)
    return;
  SourceLocation CallLoc = Call->getExprLoc();

  // Callee has a routine directive.  Record its level of parallelism on the
  // directive stack.  If there's an enclosing loop construct, complain if
  // Callee doesn't have a lower level of parallelism.
  ACCRoutineDeclAttr::PartitioningTy CalleePart = CalleeAttr->getPartitioning();
  ACCPartitioningKind CalleePartKind;
  CalleePartKind.setIndependentImplicit();
  switch (CalleePart) {
  case ACCRoutineDeclAttr::Gang:
    CalleePartKind.setGang();
    break;
  case ACCRoutineDeclAttr::Worker:
    CalleePartKind.setWorker();
    break;
  case ACCRoutineDeclAttr::Vector:
    CalleePartKind.setVector();
    break;
  case ACCRoutineDeclAttr::Seq:
    CalleePartKind.setSeqExplicit();
    break;
  }
  DirStack.setLoopPartitioning(CalleePartKind, /*ForCurrentDir=*/false);
  OpenACCDirectiveKind LoopDirKind;
  SourceLocation LoopLoc;
  ACCRoutineDeclAttr::PartitioningTy LoopPart =
      DirStack
          .getAncestorLoopPartitioning(LoopDirKind, LoopLoc,
                                       /*SkipCurrentDir=*/false)
          .getMinParallelismLevel();
  if (LoopPart != ACCRoutineDeclAttr::Seq) {
    if (LoopPart <= CalleePart) {
      Diag(CallLoc, diag::err_acc_routine_loop_par_level)
          << getOpenACCName(LoopDirKind)
          << ACCRoutineDeclAttr::ConvertPartitioningTyToStr(LoopPart)
          << Callee->getName()
          << ACCRoutineDeclAttr::ConvertPartitioningTyToStr(CalleePart);
      Diag(LoopLoc, diag::note_acc_enclosing_directive)
          << getOpenACCName(LoopDirKind);
      ImplicitRoutineDirInfo.emitNotesForRoutineDirChain(Callee);
    }
    return;
  }

  // Callee has a routine directive, and there's no enclosing loop construct.
  // If there's an enclosing compute construct, any level of parallelism for
  // Callee is compatible.
  if (isOpenACCComputeDirective(DirStack.isInComputeRegion()))
    return;

  // Callee has a routine directive, and there's no enclosing loop or compute
  // construct.  If Caller has no routine directive yet, complain if Callee has
  // a higher level of parallelism than seq.  Why?  First, if a routine
  // directive is implied for Caller later, then seq is the highest possible
  // level.  Second, if a routine directive is not implied for Caller later,
  // then Caller can execute only outside compute regions, but Callee's higher
  // level of parallelism requires execution modes (gang-redundant, etc.) that
  // are impossible outside compute regions.
  FunctionDecl *Caller = getCurFunctionDecl();
  assert(Caller && "expected function use to be in a function");
  ACCRoutineDeclAttr *CallerAttr = Caller->getAttr<ACCRoutineDeclAttr>();
  if (!CallerAttr) {
    if (CalleePart > ACCRoutineDeclAttr::Seq) {
      // We report that Caller has no explicit routine directive.  That's true,
      // and an explicit routine directive is required to enable the required
      // level of parallelism.  However, if an implicit routine seq has already
      // been implied for Caller, we report the mismatched level of parallelism
      // at the next diagnostic even though this diagnostic's wording would
      // cover that case too.
      Diag(CallLoc, diag::err_acc_routine_func_par_level_vs_no_explicit)
          << Caller->getName()
          << Callee->getName()
          << ACCRoutineDeclAttr::ConvertPartitioningTyToStr(CalleePart);
      ImplicitRoutineDirInfo.emitNotesForRoutineDirChain(Callee);
    }
    return;
  }

  // Callee and Caller have routine directives, and there's no enclosing loop or
  // compute construct.  Complain if Caller has a lower level of parallelism
  // than Callee.
  ACCRoutineDeclAttr::PartitioningTy CallerPart = CallerAttr->getPartitioning();
  if (CallerPart < CalleePart) {
    Diag(CallLoc, diag::err_acc_routine_func_par_level)
        << Caller->getName()
        << ACCRoutineDeclAttr::ConvertPartitioningTyToStr(CallerPart)
        << Callee->getName()
        << ACCRoutineDeclAttr::ConvertPartitioningTyToStr(CalleePart);
    ImplicitRoutineDirInfo.emitNotesForRoutineDirChain(Caller);
    ImplicitRoutineDirInfo.emitNotesForRoutineDirChain(Callee);
  }
}
void Sema::ActOnDeclStmtForOpenACC(DeclStmt *S) {
  if (OpenACCData->TransformingOpenACC)
    return;
  FunctionDecl *Fn = getCurFunctionDecl();
  StringRef FnName = getCurFunctionDecl()->getName();
  for (Decl *D : S->decls()) {
    if (VarDecl *VD = dyn_cast<VarDecl>(D)) {
      // OpenACC 3.1, sec. 2.15.1 "Routine Directive", L2794-2795:
      // "In C and C++, function static variables are not supported in functions
      // to which a routine directive applies."
      if (VD->isStaticLocal()) {
        DiagIfRoutineDir(OpenACCData->ImplicitRoutineDirInfo, Fn,
                         VD->getLocation(), diag::err_acc_routine_static_local)
            << VD->getName() << FnName;
      }
    }
  }
}

StmtResult Sema::ActOnOpenACCLoopDirective(ArrayRef<ACCClause *> Clauses,
                                           Stmt *AStmt,
                                           const ArrayRef<VarDecl *> &LCVs) {
  if (!AStmt)
    return StmtError();
  DirStackTy &DirStack = OpenACCData->DirStack;

  // OpenACC 2.7 sec. 2.9.1, lines 1441-1442:
  // "The collapse clause is used to specify how many tightly nested loops are
  // associated with the loop construct."
  // Complain if there aren't enough.
  Stmt *LoopStmt = AStmt;
  for (unsigned LoopI = 0, LoopCount = DirStack.getAssociatedLoops();
       LoopI < LoopCount; ++LoopI) {
    LoopStmt = LoopStmt->IgnoreContainers();
    auto *LoopFor = dyn_cast_or_null<ForStmt>(LoopStmt);
    if (!LoopFor) {
      Diag(LoopStmt->getBeginLoc(), diag::err_acc_not_for)
          << getOpenACCName(DirStack.getRealDirective());
      auto CollapseClauses =
          ACCDirectiveStmt::getClausesOfKind<ACCCollapseClause>(Clauses);
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

  ACCLoopDirective *Res = ACCLoopDirective::Create(
      Context, DirStack.getDirectiveStartLoc(), DirStack.getDirectiveEndLoc(),
      Clauses, AStmt, LCVs, DirStack.getLoopPartitioning(),
      DirStack.getNestedExplicitGangPartitioning());

  // If this is an outermost orphaned loop construct, TransformACCToOMP is about
  // to run on it, so run ImplicitGangAdder first.
  if (!isOpenACCDirectiveStmt(DirStack.getEffectiveParentDirective())) {
    // An orphaned loop construct must appear in a function with an explicit
    // routine directive, which must appear before the function definition.
    // Guard against violations of those rules just in case error recovery
    // permitted the analysis to reach this point anyway.
    FunctionDecl *CurFnDecl = getCurFunctionDecl();
    if (CurFnDecl) {
      ACCRoutineDeclAttr *FnAttr = CurFnDecl->getAttr<ACCRoutineDeclAttr>();
      // Implicit gang clauses are not permitted if the routine directive
      // doesn't specify gang.
      if (FnAttr && FnAttr->getPartitioning() == ACCRoutineDeclAttr::Gang)
        ImplicitGangAdder().Visit(Res);
    }
  }

  return Res;
}

StmtResult
Sema::ActOnOpenACCParallelLoopDirective(ArrayRef<ACCClause *> Clauses,
                                        Stmt *AStmt) {
  DirStackTy &DirStack = OpenACCData->DirStack;
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
  StmtResult Res = ActOnOpenACCDirectiveStmt(ACCD_loop, LoopClauses, AStmt);
  // The second DirStack.pop() happens in EndOpenACCDirectiveAndAssociate.
  DirStack.pop(ACCD_loop);
  if (Res.isInvalid())
    return StmtError();
  ACCLoopDirective *LoopDir = cast<ACCLoopDirective>(Res.get());

  // Build the effective parallel directive.
  Res = ActOnOpenACCDirectiveStmt(ACCD_parallel, ParallelClauses, LoopDir);
  if (Res.isInvalid())
    return StmtError();
  ACCParallelDirective *ParallelDir = cast<ACCParallelDirective>(Res.get());

  // Build the real directive.
  return ACCParallelLoopDirective::Create(
      Context, DirStack.getDirectiveStartLoc(), DirStack.getDirectiveEndLoc(),
      Clauses, AStmt, ParallelDir);
}

void Sema::ActOnOpenACCRoutineDirective(OpenACCDetermination Determination,
                                        DeclGroupRef TheDeclGroup) {
  DirStackTy &DirStack = OpenACCData->DirStack;
  ActOnOpenACCRoutineDirective(DirStack.getClauses(), Determination,
                               DirStack.getDirectiveStartLoc(),
                               DirStack.getDirectiveEndLoc(), TheDeclGroup);
}

void Sema::ActOnOpenACCRoutineDirective(ArrayRef<ACCClause *> Clauses,
                                        OpenACCDetermination Determination,
                                        SourceLocation StartLoc,
                                        SourceLocation EndLoc,
                                        DeclGroupRef TheDeclGroup) {
  // OpenACC 3.1, sec. 2.15.1 "Routine Directive", L2706-2708:
  // "In C and C++, the routine directive without a name may appear immediately
  // before a function definition, a C++ lambda, or just before a function
  // prototype and applies to that immediately following function or prototype."
  Decl *TheDecl = nullptr;
  for (Decl *D : TheDeclGroup) {
    // Permitting types in this DeclGroup enables the case that the routine
    // directive is followed by a function prototype containing new type
    // declarations, such as a struct return type.  We wouldn't bother to
    // support this case except that it's already supported in the case of a
    // function definition because the types are not in the same DeclGroup.  It
    // would be strange to support it for function definitions but not function
    // prototypes.
    if (isa<TypeDecl>(D))
      continue;
    if (TheDecl) {
      // This includes the case that the routine directive is followed by a
      // single declaration with multiple function declarators, such as
      // "void foo(), bar();".
      Diag(StartLoc, diag::err_acc_expected_function_after_directive)
          << getOpenACCName(ACCD_routine);
      return;
    }
    TheDecl = D;
  }
  if (!TheDecl || !TheDecl->isFunctionOrFunctionTemplate()) {
    // !TheDecl includes the case that the routine directive is at the end of
    // the enclosing context (e.g., the file, or a type definition).  It also
    // includes the case that the routine directive precedes a type declaration.
    Diag(StartLoc, diag::err_acc_expected_function_after_directive)
        << getOpenACCName(ACCD_routine);
    return;
  }
  FunctionDecl *FD = TheDecl->getAsFunction();
  ACCRoutineDeclAttr *ACCAttr = FD->getAttr<ACCRoutineDeclAttr>();

  // ActOnStartOfFunctionDefForOpenACC calls ActOnOpenACCRoutineDirective in the
  // case of a function definition, but ParseOpenACCDeclarativeDirective doesn't
  // know and calls it again, so skip if this is the second time.
  if (ACCAttr && !ACCAttr->isInherited())
    return;

  // OpenACC 3.2, sec. 2.15.1 "Routine Directive", L2927:
  // "At least one of the (gang, worker, vector, or seq) clauses must appear on
  // the construct."
  ACCClause *PartClause = nullptr;
  ACCRoutineDeclAttr::PartitioningTy Part;
  for (ACCClause *C : Clauses) {
    switch (C->getClauseKind()) {
    case ACCC_gang:
      PartClause = C;
      Part = ACCRoutineDeclAttr::Gang;
      break;
    case ACCC_worker:
      PartClause = C;
      Part = ACCRoutineDeclAttr::Worker;
      break;
    case ACCC_vector:
      PartClause = C;
      Part = ACCRoutineDeclAttr::Vector;
      break;
    case ACCC_seq:
      PartClause = C;
      Part = ACCRoutineDeclAttr::Seq;
      break;
    default:
      break;
    }
    if (PartClause)
      break;
  }
  if (!PartClause) {
    Diag(StartLoc, diag::err_acc_routine_no_gang_worker_vector_seq_clause);
    return;
  }

  // Proposed text for OpenACC after 3.2:
  // - "In C and C++, a routine directive's scope starts at the routine
  //   directive and ends at the end of the compilation unit."
  // - "In C and C++, a definition or use of a procedure must appear within the
  //   scope of at least one explicit and applying routine directive if any
  //   appears in the same compilation unit."
  // Uses include host uses and offload device uses but only if they're
  // evaluated (e.g., a reference in sizeof is not a use).
  bool AfterUseErrorReported = false;
  if (Determination == ACC_EXPLICIT && (!ACCAttr || ACCAttr->isImplicit())) {
    bool PreviousErrors = getDiagnostics().hasErrorOccurred();
    if (FunctionDecl *Def = FD->getDefinition()) {
      if (Def != FD) {
        Diag(StartLoc, diag::err_acc_routine_not_in_scope_at_function_def)
            << FD->getName();
        Diag(Def->getBeginLoc(), diag::note_acc_routine_definition)
            << FD->getName();
      }
    }
    if (FD->isUsed()) {
      // We are careful to emit this diagnostic before any diagnostics about
      // clauses that conflict with a previously implied routine directive
      // because this not that is the real problem.  That is, the explicit
      // routine directive should have appeared before the use that implied the
      // conflicting routine directive, which then wouldn't have been implied.
      // We emit conflicting clause diagnostics too to help explain the
      // compiler's current analysis of any code in between.
      AfterUseErrorReported = true;
      Diag(StartLoc, diag::err_acc_routine_not_in_scope_at_function_use)
          << FD->getName();
      RoutineUseReporter Reporter(*this, FD);
      Reporter.TraverseAST(Context);
      if (!Reporter.foundUse()) {
        assert(PreviousErrors &&
               "expected to find and report function use if no previous errors "
               "potentially discarded parts of the AST");
        SourceLocation UseLoc =
            OpenACCData->ImplicitRoutineDirInfo.getUseLoc(FD);
        if (UseLoc.isValid()) {
          // It's only one of potentially multiple uses, but that'll have to do.
          Diag(UseLoc, diag::note_acc_routine_use) << FD->getName();
        } else {
          // Must have been host uses, which don't imply routine directives.
          Diag(StartLoc, diag::note_acc_routine_use_lost) << FD->getName();
        }
      }
    }
  }

  // Create the new attributes or adjust the inherited ones.
  if (!ACCAttr) {
    switch (Determination) {
    case ACC_EXPLICIT:
      ACCAttr = ACCRoutineDeclAttr::Create(Context, Part,
                                           SourceRange(StartLoc, EndLoc));
      break;
    case ACC_IMPLICIT:
      ACCAttr = ACCRoutineDeclAttr::CreateImplicit(
          Context, Part, SourceRange(StartLoc, EndLoc));
      break;
    case ACC_PREDETERMINED:
    case ACC_UNDETERMINED:
      llvm_unreachable(
          "expected new routine directive to be explicit or implicit");
    }
    FD->addAttr(ACCAttr);
    transformACCToOMP(ACCAttr, FD);
  } else {
    assert(Determination == ACC_EXPLICIT &&
           "expected new routine directive to be explicit if already have one");
    ACCRoutineDeclAttr::PartitioningTy PartOld = ACCAttr->getPartitioning();
    // Proposed text for OpenACC after 3.2:
    //
    // "If multiple routine directives that apply to the same procedure have
    // overlapping scopes, then they must specify the same level of
    // parallelism."
    if (PartOld != Part) {
      Diag(PartClause->getBeginLoc(), diag::err_acc_routine_inconsistent)
          << FD->getName()
          << ACCRoutineDeclAttr::ConvertPartitioningTyToStr(Part)
          << ACCRoutineDeclAttr::ConvertPartitioningTyToStr(PartOld);
      OpenACCData->ImplicitRoutineDirInfo.emitNotesForRoutineDirChain(
          FD, /*PreviousDir=*/true);
      assert((ACCAttr->getLocation().isValid() || AfterUseErrorReported) &&
             "expected explicit routine directive conflicting with an implicit "
             "routine directive to produce a diagnostic about appearing after "
             "a use of the function");
      return;
    }
    // The routine directive is explicit, so clear the inherited bit that was
    // set automatically due to the previous routine directive.  This is
    // important for source-to-source mode and AST printing.
    //
    // The routine directive is explicit, so it should have come before any use,
    // so there shouldn't be an any existing implicit routine directive.
    // However, in case we're doing an AST dump in that erroneous case, clear
    // the implicit bit that is set automatically in that case.
    ACCAttr->setInherited(false);
    ACCAttr->setImplicit(false);
    ACCAttr->setRange(SourceRange(StartLoc, EndLoc));
    if (InheritableAttr *OMPAttr = ACCAttr->getOMPNode(FD)) {
      OMPAttr->setInherited(false);
      OMPAttr->setImplicit(false);
      OMPAttr->setRange(SourceRange(StartLoc, EndLoc));
    }
  }
}

ACCClause *Sema::ActOnOpenACCSingleExprClause(OpenACCClauseKind Kind,
                                              Expr *Expr,
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
    OpenACCClauseKind CKind, SourceLocation DirectiveLoc, SourceLocation ELoc)
{
  if (SemaRef.RequireCompleteType(ELoc, Type, diag::err_acc_incomplete_type,
                                  Determination, getOpenACCName(CKind)))
  {
    if (Determination != ACC_EXPLICIT)
      SemaRef.Diag(DirectiveLoc, diag::note_acc_clause_determination)
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

    if (!OpenACCData->DirStack.addDMA(VD, RefExpr->IgnoreParens(),
                                      ACC_DMA_nomap, ACC_IMPLICIT))
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
                               OpenACCData->DirStack.getDirectiveStartLoc(),
                               ELoc))
      continue;

    if (!OpenACCData->DirStack.addDMA(VD, RefExpr->IgnoreParens(),
                                      ACC_DMA_present, Determination))
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
                               OpenACCData->DirStack.getDirectiveStartLoc(),
                               ELoc))
      continue;

    if (!OpenACCData->DirStack.addDMA(VD, RefExpr->IgnoreParens(), ACC_DMA_copy,
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
                               OpenACCData->DirStack.getDirectiveStartLoc(),
                               ELoc))
      continue;

    if (!OpenACCData->DirStack.addDMA(VD, RefExpr->IgnoreParens(),
                                      ACC_DMA_copyin, ACC_EXPLICIT))
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
                               OpenACCData->DirStack.getDirectiveStartLoc(),
                               ELoc))
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

    if (!OpenACCData->DirStack.addDMA(VD, RefExpr->IgnoreParens(),
                                      ACC_DMA_copyout, ACC_EXPLICIT))
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
                               OpenACCData->DirStack.getDirectiveStartLoc(),
                               ELoc))
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

    if (!OpenACCData->DirStack.addDMA(VD, RefExpr->IgnoreParens(),
                                      ACC_DMA_create, ACC_EXPLICIT))
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
                               OpenACCData->DirStack.getDirectiveStartLoc(),
                               ELoc))
      continue;

    if (!OpenACCData->DirStack.addDMA(VD, RefExpr->IgnoreParens(),
                                      ACC_DMA_no_create, ACC_EXPLICIT))
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
                               OpenACCData->DirStack.getDirectiveStartLoc(),
                               ELoc))
      continue;

    if (!OpenACCData->DirStack.addDMA(VD, RefExpr->IgnoreParens(),
                                      ACC_DMA_delete, ACC_EXPLICIT))
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

    if (!OpenACCData->DirStack.addDSA(VD, RefExpr->IgnoreParens(),
                                      ACC_DSA_shared, ACC_IMPLICIT))
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
                               OpenACCData->DirStack.getDirectiveStartLoc(),
                               ELoc))
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

    if (!OpenACCData->DirStack.addDSA(VD, RefExpr->IgnoreParens(),
                                      ACC_DSA_private, Determination))
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
                               OpenACCData->DirStack.getDirectiveStartLoc(),
                               ELoc))
      continue;

    if (!OpenACCData->DirStack.addDSA(VD, RefExpr->IgnoreParens(),
                                      ACC_DSA_firstprivate, Determination))
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
  DirStackTy &DirStack = OpenACCData->DirStack;
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
                               DirStack.getDirectiveStartLoc(), ELoc))
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

    // OpenACC 3.2, sec. 2.9.11 "reduction clause", L2114:
    // "Every var in a reduction clause appearing on an orphaned loop construct
    // must be private."
    if (DirStack.getEffectiveDirective() == ACCD_loop &&
        !isOpenACCComputeDirective(DirStack.isInComputeRegion()) &&
        !DirStack.hasPrivatizingDSAOnAncestorOrphanedLoop(VD) &&
        !VD->hasLocalStorage())
      Diag(ELoc, diag::err_acc_orphaned_loop_reduction_var_not_gang_private)
          << VD->getName();

    // Record reduction item.
    if (!DirStack.addDSA(VD, RefExpr->IgnoreParens(), ACC_DSA_reduction,
                         Determination, ReductionId))
      Vars.push_back(RefExpr);
  }
  if (Vars.empty())
    return nullptr;
  return ACCReductionClause::Create(Context, Determination, StartLoc, LParenLoc,
                                    ColonLoc, EndLoc, Vars, ReductionId);
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
                               OpenACCData->DirStack.getDirectiveStartLoc(),
                               ELoc))
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

    if (!OpenACCData->DirStack.addUpdateVar(VD, RefExpr->IgnoreParens()))
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
                               OpenACCData->DirStack.getDirectiveStartLoc(),
                               ELoc))
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

    if (!OpenACCData->DirStack.addUpdateVar(VD, RefExpr->IgnoreParens()))
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
    Res = ActOnOpenACCSeqClause(ACC_EXPLICIT, StartLoc, EndLoc);
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

ACCClause *Sema::ActOnOpenACCSeqClause(OpenACCDetermination Determination,
                                       SourceLocation StartLoc,
                                       SourceLocation EndLoc) {
  return new (Context) ACCSeqClause(Determination, StartLoc, EndLoc);
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
  OpenACCData->DirStack.setAssociatedLoops(
      Collapse->EvaluateKnownConstInt(Context).getExtValue());
  return new (Context) ACCCollapseClause(Collapse, StartLoc, LParenLoc,
                                         EndLoc);
}

bool Sema::isInOpenACCDirectiveStmt() {
  return isOpenACCDirectiveStmt(OpenACCData->DirStack.getRealDirective());
}
