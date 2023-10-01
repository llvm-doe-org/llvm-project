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

#include "ACCDataVar.h"
#include "UsedDeclVisitor.h"
#include "clang/AST/ASTLambda.h"
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
    /// Map from a variable to its DA data.
    llvm::DenseMap<ACCDataVar, DAVarData> DAMap;
    llvm::DenseMap<ACCDataVar, Expr *> UpdateVarSet;
    llvm::SmallVector<Expr *, 4> ReductionVarsOnEffectiveOrCombined;
    llvm::DenseSet<ACCDataVar> LCVSet;
    llvm::SmallVector<Expr *, 4> LCVExprs;
    /// The function immediately lexically enclosing this directive, or
    /// \c nullptr if none.
    FunctionDecl *EnclosingFn;
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
    /// Has the current (explicit) directive on the stack been acted on?
    /// Undefined if that's not a routine directive.
    bool ExplicitRoutineDirectiveIsActedOn = false;
    ACCPartitioningKind LoopDirectiveKind;
    SourceLocation LoopBreakLoc; // invalid if no break statement or not loop
    unsigned AssociatedLoops = 1; // from collapse or tile clause
    unsigned AssociatedLoopsParsed = 0; // how many have been parsed so far
    ///@{
    /// True if this is an effective compute or loop construct that encloses one
    /// of the following that is not in an enclosed function definition: either
    /// an effective loop directive with explicit gang/worker/vector
    /// partitioning (after auto clause analysis), or a function call with a
    /// routine gang/worker/vector directive.
    ///
    /// Implicit gang/worker/vector clauses are added by
    /// \c ImplicitGangWorkerVectorAdder after an entire parallel construct or
    /// outermost orphaned loop construct is parsed and thus after this stack is
    /// popped for all effective nested loop directives, so it's too late to
    /// update these fields then.  Thus, these fields only track explicit
    /// partitioning.
    bool NestedExplicitGangPartitioning = false;
    bool NestedExplicitWorkerPartitioning = false;
    bool NestedExplicitVectorPartitioning = false;
    ///@}
    DirStackEntryTy(FunctionDecl *EnclosingFn, OpenACCDirectiveKind RealDKind,
                    OpenACCDirectiveKind EffectiveDKind,
                    SourceLocation StartLoc)
        : EnclosingFn(EnclosingFn), RealDKind(RealDKind),
          EffectiveDKind(EffectiveDKind), DirectiveStartLoc(StartLoc) {}
  };

  /// The underlying directive stack.
  SmallVector<DirStackEntryTy, 4> Stack;

public:
  explicit DirStackTy(Sema &S) : SemaRef(S) {}

  void push(OpenACCDirectiveKind RealDKind, OpenACCDirectiveKind EffectiveDKind,
            SourceLocation Loc) {
    Stack.push_back(
        DirStackEntryTy(SemaRef.getCurFunctionDecl(/*AllowLambda=*/true),
                        RealDKind, EffectiveDKind, Loc));
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

  /// Mark the current (explicit) directive on the stack as having been acted
  /// on.  Fails an assert if that's not a routine directive.
  void setExplicitRoutineDirectiveActedOn() {
    assert(!Stack.empty() && "expected non-empty directive stack");
    assert(Stack.rbegin()->EffectiveDKind == ACCD_routine &&
           "expected routine directive at top of directive stack");
    Stack.rbegin()->ExplicitRoutineDirectiveIsActedOn = true;
  }

  /// Has the current (explicit) directive on the stack been acted on?  Fails an
  /// assert if that's not a routine directive.
  bool isExplicitRoutineDirectiveActedOn() {
    assert(!Stack.empty() && "expected non-empty directive stack");
    assert(Stack.rbegin()->EffectiveDKind == ACCD_routine &&
           "expected routine directive at top of directive stack");
    return Stack.rbegin()->ExplicitRoutineDirectiveIsActedOn;
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
  void addLoopControlVariable(Expr *E) {
    assert(!Stack.empty() && "expected non-empty directive stack");
    assert(isOpenACCLoopDirective(getEffectiveDirective()) &&
           "expected loop control variable to be added to loop directive");
    // It should not be possible to receive a subarray here.  Member expressions
    // will be diagnosed later, so accept them here.
    ACCDataVar Var(E, ACCDataVar::AllowMemberExprOnAny, /*AllowSubarray=*/false,
                   &SemaRef, /*Quiet=*/true);
    // Drop invalid loop control variables for now.  Loop form validation will
    // catch them later.
    if (Var.isValid()) {
      Stack.back().LCVSet.insert(Var);
      Stack.back().LCVExprs.emplace_back(E);
    }
  }
  /// Is the specified variable a loop control variable that is assigned but
  /// not declared in the init of a for loop associated with the current
  /// directive?
  bool hasLoopControlVariable(ACCDataVar Var) const {
    assert(!Stack.empty() && "expected non-empty directive stack");
    return Stack.back().LCVSet.count(Var);
  }
  /// Get the loop control variables that are assigned but not declared in the
  /// inits of the for loops associated with the current directive, or return
  /// an empty list if none.
  ///
  /// Each item in the list is the expression that references the variable in
  /// the assignment.
  ArrayRef<Expr *> getLoopControlVariables() const {
    assert(!Stack.empty() && "expected non-empty directive stack");
    return Stack.back().LCVExprs;
  }
  /// Register loop partitioning for the current loop directive if
  /// \p ForCurrentDir or for a function call otherwise.
  ///
  /// As part of that, mark all effective ancestor compute or loop directives
  /// within this function as containing any explicit gang or worker
  /// partitioning.
  void setLoopPartitioning(ACCPartitioningKind Kind, bool ForCurrentDir) {
    auto I = Stack.rbegin();
    if (ForCurrentDir) {
      assert(isOpenACCLoopDirective(getEffectiveDirective()) &&
             "expected to set partitioning for a loop directive");
      I->LoopDirectiveKind = Kind;
      ++I;
    }
    if (Kind.hasGangPartitioning() || Kind.hasWorkerPartitioning() ||
        Kind.hasVectorPartitioning()) {
      FunctionDecl *CurFn = SemaRef.getCurFunctionDecl(/*AllowLambda=*/true);
      for (auto E = Stack.rend(); I != E && I->EnclosingFn == CurFn; ++I) {
        bool IsComputeConstruct = isOpenACCComputeDirective(I->EffectiveDKind);
        if (!isOpenACCLoopDirective(I->EffectiveDKind) && !IsComputeConstruct)
          break;
        if (Kind.hasGangPartitioning())
          I->NestedExplicitGangPartitioning = true;
        if (Kind.hasWorkerPartitioning())
          I->NestedExplicitWorkerPartitioning = true;
        if (Kind.hasVectorPartitioning())
          I->NestedExplicitVectorPartitioning = true;
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
  /// the stack or a directive outside the current function.  If case 1, return
  /// that directive's loop partitioning kind, and record its real directive
  /// kind and location.  Else if case 2 or 3, return no partitioning, and don't
  // record a directive kind or location.
  ACCPartitioningKind
  getAncestorLoopPartitioning(OpenACCDirectiveKind &ParentDir,
                              SourceLocation &ParentLoc,
                              bool SkipCurrentDir) const {
    auto I = Stack.rbegin();
    if (SkipCurrentDir)
      ++I;
    FunctionDecl *CurFn = SemaRef.getCurFunctionDecl(/*AllowLambda=*/true);
    for (auto E = Stack.rend(); I != E && I->EnclosingFn == CurFn; ++I) {
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
  ///@{
  /// Is this an effective compute or loop construct that encloses one of the
  /// following that is not in an enclosed function definition: either an
  /// effective loop directive with explicit gang/worker/vector partitioning
  /// (after auto clause analysis), or a function call with a routine
  /// gang/worker/vector directive?
  ///
  /// Implicit gang/worker/vector clauses are added by
  /// \c ImplicitGangWorkerVectorAdder after an entire parallel construct or
  /// outermost orphaned loop construct is parsed and thus after this stack is
  /// popped for all effective nested loop directives, so it's too late to
  /// update these fields then.  Thus, these fields only track explicit
  /// partitioning.
  bool getNestedExplicitGangPartitioning() const {
    assert(!Stack.empty() && "expected non-empty directive stack");
    return Stack.back().NestedExplicitGangPartitioning;
  }
  bool getNestedExplicitWorkerPartitioning() const {
    assert(!Stack.empty() && "expected non-empty directive stack");
    return Stack.back().NestedExplicitWorkerPartitioning;
  }
  bool getNestedExplicitVectorPartitioning() const {
    assert(!Stack.empty() && "expected non-empty directive stack");
    return Stack.back().NestedExplicitVectorPartitioning;
  }
  ///@}

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
    static bool checkReductionConflict(Sema &SemaRef, ACCDataVar Var, Expr *E,
                                       OpenACCDMAKind K, DAVarData VarData,
                                       const DeclarationNameInfo &ReductionId) {
      return false;
    }
    static void setReductionFields(bool AddHere, OpenACCDMAKind K,
                                   DirStackEntryTy &StackEntry,
                                   DAVarData &VarData, Expr *RefExpr,
                                   const DeclarationNameInfo &ReductionId) {}
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
    static bool checkReductionConflict(Sema &SemaRef, ACCDataVar Var, Expr *E,
                                       OpenACCDSAKind K, DAVarData VarData,
                                       const DeclarationNameInfo &ReductionId) {
      if (K == ACC_DSA_reduction && Kind(VarData) == ACC_DSA_reduction) {
        SemaRef.Diag(E->getExprLoc(), diag::err_acc_conflicting_reduction)
            << (VarData.ReductionId.getName() == ReductionId.getName())
            << ACCReductionClause::printReductionOperatorToString(ReductionId)
            << NameForDiag(SemaRef, Var);
        SemaRef.Diag(VarData.DSARefExpr->getExprLoc(),
                     diag::note_acc_previous_reduction)
            << ACCReductionClause::printReductionOperatorToString(
                   VarData.ReductionId);
        return true;
      }
      return false;
    }
    static void setReductionFields(bool AddHere, OpenACCDSAKind K,
                                   DirStackEntryTy &StackEntry,
                                   DAVarData &VarData, Expr *RefExpr,
                                   const DeclarationNameInfo &ReductionId) {
      if (K != ACC_DSA_reduction)
        return;
      if (AddHere)
        VarData.ReductionId = ReductionId;
      StackEntry.ReductionVarsOnEffectiveOrCombined.push_back(RefExpr);
    }
  };
  /// Encapsulates commonalities between implement addDMA and addDSA.  Each
  /// of DA and OtherDA is one of DMATraits or DSATraits.
  template <typename DA, typename OtherDA>
  bool addDA(ACCDataVar Var, Expr *E, typename DA::KindTy DAKind,
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
  bool addDMA(ACCDataVar Var, Expr *E, OpenACCDMAKind DMAKind,
              OpenACCDetermination Determination) {
    return addDA<DMATraits, DSATraits>(Var, E, DMAKind, Determination);
  }
  bool addDSA(ACCDataVar Var, Expr *E, OpenACCDSAKind DSAKind,
              OpenACCDetermination Determination,
              const DeclarationNameInfo &ReductionId = DeclarationNameInfo()) {
    return addDA<DSATraits, DMATraits>(Var, E, DSAKind, Determination,
                                       ReductionId);
  }
  ///@}

  /// Returns data attributes from top of the stack for the specified
  /// declaration.  (Predetermined and implicitly determined data attributes
  /// usually have not been computed yet.)
  DAVarData getTopDA(ACCDataVar Var);

  /// Returns true if the declaration has a DMA on the current directive or on
  /// any lexically enclosing directive within the current function.
  ///
  /// (Predetermined and implicitly determined data attributes usually have not
  /// been computed yet.)
  bool hasVisibleDMA(ACCDataVar Var) {
    assert(!Stack.empty() && "expected non-empty directive stack");
    FunctionDecl *CurFD = SemaRef.getCurFunctionDecl(/*AllowLambda=*/true);
    for (auto I = Stack.rbegin(), E = Stack.rend();
         I != E && I->EnclosingFn == CurFD; ++I) {
      auto DAItr = I->DAMap.find(Var);
      if (DAItr != I->DAMap.end() && DAItr->second.DMAKind != ACC_DMA_unknown)
        return true;
    }
    return false;
  }

  /// Returns true if the declaration has a privatizing DSA on any loop
  /// construct within the current function beyond the current directive.
  ///
  /// Fails an assertion if encounters an enclosing construct that is not a
  /// loop construct and that is within the current function.
  ///
  /// Checks for explicit DSAs and predetermined DSAs except in the case of
  /// local variable declarations, which must be checked separately.  Loop
  /// constructs have no implicitly determined DSAs.
  bool hasPrivatizingDSAOnAncestorOrphanedLoop(ACCDataVar Var) {
    assert(!Stack.empty() && "expected non-empty directive stack");
    FunctionDecl *CurFD = SemaRef.getCurFunctionDecl(/*AllowLambda=*/true);
    for (auto I = std::next(Stack.rbegin()), E = Stack.rend();
         I != E && I->EnclosingFn == CurFD; ++I) {
      assert((I->EffectiveDKind == ACCD_loop ||
              I->EffectiveDKind == ACCD_routine) &&
             "expected all ancestor constructs to be orphaned loop constructs");
      auto DAItr = I->DAMap.find(Var);
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
      if (I->LCVSet.count(Var))
        return true;
    }
    return false;
  }

  /// Records a variable appearing in an update directive clause and returns
  /// false, or complains and returns true if it already appeared in one for
  /// the current directive.
  bool addUpdateVar(ACCDataVar Var, Expr *E) {
    assert(!Stack.empty() && "expected non-empty directive stack");
    auto Itr = Stack.rbegin();
    if (Expr *OldExpr = Itr->UpdateVarSet[Var]) {
      SemaRef.Diag(E->getExprLoc(), diag::err_acc_update_same_var)
          << NameForDiag(SemaRef, Var) << E->getSourceRange();
      SemaRef.Diag(OldExpr->getExprLoc(), diag::note_acc_update_var)
          << OldExpr->getSourceRange();
      return true;
    }
    Itr->UpdateVarSet[Var] = E;
    return false;
  }

  /// If the parser is currently somewhere in a compute construct, returns the
  /// real kind of the compute construct.  If the parser is currently somewhere
  /// in a function for which an applying routine directive is already known,
  /// returns \c ACCD_routine.  Otherwise, returns \c ACCD_unknown even if a
  /// routine directive will be implied later.  Does not otherwise return
  /// directives outside the current function.
  OpenACCDirectiveKind isInComputeRegion() const {
    FunctionDecl *CurFD = SemaRef.getCurFunctionDecl(/*AllowLambda=*/true);
    for (auto I = Stack.rbegin(), E = Stack.rend();
         I != E && I->EnclosingFn == CurFD; ++I) {
      OpenACCDirectiveKind DKind = I->RealDKind;
      if (isOpenACCComputeDirective(DKind))
        return DKind;
    }
    if (CurFD && CurFD->hasAttr<ACCRoutineDeclAttr>())
      return ACCD_routine;
    return ACCD_unknown;
  }

  /// Returns the real directive currently being analyzed, or returns
  /// ACCD_unknown if none.
  ///
  /// Except for any routine directive lexically attached to the current
  /// function, directives outside the current function are not returned unless
  /// \p LookOutsideCurFunction.
  OpenACCDirectiveKind
  getRealDirective(bool LookOutsideCurFunction = false) const {
    auto I = Stack.rbegin();
    if (I == Stack.rend())
      return ACCD_unknown;
    if (!LookOutsideCurFunction) {
      FunctionDecl *CurFn = SemaRef.getCurFunctionDecl(/*AllowLambda=*/true);
      if (I->EffectiveDKind != ACCD_routine && I->EnclosingFn != CurFn)
        return ACCD_unknown;
    }
    while (I->RealDKind == ACCD_unknown)
      I = std::next(I);
    return I->RealDKind;
  }

  /// Returns the effective directive currently being analyzed, or returns
  /// ACCD_unknown if none.  This is always the same as \c getRealDirective()
  /// unless the latter is a combined directive.
  ///
  /// Except for any routine directive lexically attached to the current
  /// function, directives outside the current function are not returned.
  OpenACCDirectiveKind getEffectiveDirective() const {
    auto I = Stack.rbegin();
    if (I == Stack.rend())
      return ACCD_unknown;
    FunctionDecl *CurFn = SemaRef.getCurFunctionDecl(/*AllowLambda=*/true);
    if (I->EffectiveDKind != ACCD_routine && I->EnclosingFn != CurFn)
      return ACCD_unknown;
    return I->EffectiveDKind;
  }

  /// Returns the real directive for the construct immediately enclosing the
  /// currently analyzed real directive, or returns ACCD_unknown if none.  In
  /// the former case only, set ParentLoc as the returned directive's location.
  ///
  /// Routine directives that are lexically attached to the current function are
  /// modeled as enclosing constructs.  Other directives outside the current
  /// function are not returned.
  OpenACCDirectiveKind getRealParentDirective(SourceLocation &ParentLoc) const {
    // Find real directive.
    auto I = Stack.rbegin();
    if (I == Stack.rend())
      return ACCD_unknown;
    FunctionDecl *CurFn = SemaRef.getCurFunctionDecl(/*AllowLambda=*/true);
    if (I->EffectiveDKind != ACCD_routine && I->EnclosingFn != CurFn)
      return ACCD_unknown;
    while (I->RealDKind == ACCD_unknown)
      I = std::next(I);
    // Find real parent directive.
    I = std::next(I);
    if (I == Stack.rend())
      return ACCD_unknown;
    if (I->EffectiveDKind != ACCD_routine && I->EnclosingFn != CurFn)
      return ACCD_unknown;
    while (I->RealDKind == ACCD_unknown)
      I = std::next(I);
    ParentLoc = I->DirectiveStartLoc;
    return I->RealDKind;
  }

  /// Returns the effective directive for the construct immediately enclosing
  /// the currently analyzed effective directive, or returns ACCD_unknown if
  /// none.
  ///
  /// Routine directives that are lexically attached to the current function are
  /// modeled as enclosing constructs.  Other directives outside the current
  /// function are not returned.
  OpenACCDirectiveKind getEffectiveParentDirective() const {
    auto I = Stack.rbegin();
    if (I == Stack.rend())
      return ACCD_unknown;
    FunctionDecl *CurFn = SemaRef.getCurFunctionDecl(/*AllowLambda=*/true);
    if (I->EffectiveDKind != ACCD_routine && I->EnclosingFn != CurFn)
      return ACCD_unknown;
    I = std::next(I);
    if (I == Stack.rend())
      return ACCD_unknown;
    if (I->EffectiveDKind != ACCD_routine && I->EnclosingFn != CurFn)
      return ACCD_unknown;
    return I->EffectiveDKind;
  }

  /// Set associated loop count (from collapse or tile clause) for the region.
  void setAssociatedLoops(unsigned Val) {
    assert(!Stack.empty() && "expected non-empty directive stack");
    assert(Stack.back().AssociatedLoops == 1);
    assert(Stack.back().AssociatedLoopsParsed == 0);
    Stack.back().AssociatedLoops = Val;
  }
  /// Return associated loop count (from collapse or tile clause) for the
  /// region.
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
bool DirStackTy::addDA(ACCDataVar Var, Expr *E, typename DA::KindTy DAKind,
                       OpenACCDetermination Determination,
                       const DeclarationNameInfo &ReductionId) {
  assert(!Stack.empty() && "expected non-empty directive stack");
  assert(DAKind != DA::KindUnknown && "expected known DA");
  DA::checkReductionIdArg(DAKind, ReductionId);

  // If this is not a combined directive, or if this is an implicit clause,
  // visit just this directive.  Otherwise, climb through all effective
  // directives for the current combined directive.
  bool Added = false;
  for (auto Itr = Stack.rbegin(), End = Stack.rend(); Itr != End; ++Itr) {
    DAVarData &VarData = Itr->DAMap[Var];
    // Complain for conflict with existing DA.
    //
    // See the section "Basic Data Attributes" in the Clang OpenACC design
    // document.
    //
    // Some DAs should be suppressed by rather than conflict with existing DAs,
    // but addDMA and addDSA have preconditions to avoid those scenarios.
    if (DA::Kind(VarData) != DA::KindUnknown) {
      if (DA::checkReductionConflict(SemaRef, Var, E, DAKind, VarData,
                                     ReductionId))
        return true;
      SemaRef.Diag(E->getExprLoc(), diag::err_acc_conflicting_da)
          << (DA::Kind(VarData) == DAKind) << getOpenACCName(DA::Kind(VarData))
          << Determination << getOpenACCName(DAKind);
      SemaRef.Diag(DA::RefExpr(VarData)->getExprLoc(),
                   diag::note_acc_explicit_da)
          << getOpenACCName(DA::Kind(VarData));
      return true;
    }
    if (!isAllowedDSAForDMA(DAKind, OtherDA::Kind(VarData))) {
      SemaRef.Diag(E->getExprLoc(), diag::err_acc_conflicting_da)
          << false << getOpenACCName(OtherDA::Kind(VarData)) << Determination
          << getOpenACCName(DAKind);
      SemaRef.Diag(OtherDA::RefExpr(VarData)->getExprLoc(),
                   diag::note_acc_explicit_da)
          << getOpenACCName(OtherDA::Kind(VarData));
      return true;
    }
    // Complain for conflict with directive.
    if (isAllowedDAForDirective(Itr->EffectiveDKind, DAKind)) {
      DA::setReductionFields(!Added, DAKind, *Itr, VarData, E, ReductionId);
      if (!Added) {
        DA::Kind(VarData) = DAKind;
        DA::RefExpr(VarData) = E;
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

DirStackTy::DAVarData DirStackTy::getTopDA(ACCDataVar Var) {
  assert(!Stack.empty() && "expected non-empty directive stack");
  auto I = Stack.rbegin();
  auto DAItr = I->DAMap.find(Var);
  if (DAItr != I->DAMap.end())
    return DAItr->second;
  return DAVarData();
}

//===----------------------------------------------------------------------===//
// OpenACC implicit routine directive info.
//===----------------------------------------------------------------------===//

namespace {
/// Info recorded about functions that at first do not have fully determined
/// routine directives.
///
/// An \c ACCRoutineDeclAttr is attached to a function at the point when an
/// implicit routine directive is fully determined.  If an implier is a use of
/// the function, the implicit routine directive is fully determined immediately
/// upon the use.  If an implier is within the definition of the function, the
/// implicit routine directive (and specifically its level of parallelism) is
/// not fully determined until the end of the function.
class ImplicitRoutineDirInfoTy {
private:
  Sema &SemaRef;
  struct FunctionEntryTy {
    typedef std::pair<SourceLocation, PartialDiagnostic> DiagTy;
    /// Diagnostics to report if and when an implicit routine directive is fully
    /// determined for this function later.  Diagnostics are stored here by
    /// \c DiagIfRoutineDir.
    std::list<DiagTy> Diags;
    /// This function's uses of other functions such that, in each case, the use
    /// does not appear in a compute construct and neither function had a fully
    /// determined routine directive when the use was discovered.  For each such
    /// other function \c FD, this table is indexed by
    /// \c FD->getCanonicalDecl().
    llvm::DenseMap<FunctionDecl *, SourceLocation> HostFunctionUses;
    /// The location of the original implier of a routine directive with level
    /// of parallelism \c ParLevel for this function, or invalid if no routine
    /// directive implier has been encountered yet for this function.  That is,
    /// this location is valid starting at the first routine directive implier
    /// for this function even if an implicit routine directive has not yet been
    /// fully determined for this function.
    SourceLocation ImplierLoc;
    /// Either:
    /// - The function in which \c ImplierLoc appeared if \c ImplierLoc is for a
    ///   use of this function.
    /// - The callee if \c ImplierLoc is for a function call within this
    ///   function.
    /// - \c nullptr otherwise.
    FunctionDecl *ImplierFn = nullptr;
    /// Either:
    /// - The real kind of the compute construct in which \c ImplierLoc appeared
    ///   if \c ImplierLoc is for a use of this function.
    /// - \c ACCD_routine if \c ImplierLoc is for a use of this function not
    ///   within a compute construct.
    /// - \c ACCD_loop if \c ImplierLoc is for an orphaned loop construct within
    ///   this function.
    /// - \c ACCD_unknown if \c ImplierLoc is for a function call within this
    ///   function or is invalid.
    OpenACCDirectiveKind ImplierConstruct = ACCD_unknown;
    /// The current level of parallelism implied for this function.  Undefined
    /// if \c ImplierLoc is invalid.
    ACCRoutineDeclAttr::PartitioningTy ParLevel;
    /// Whether at least one implier was an orphaned loop construct within this
    /// function.
    bool HasOrphanedLoopConstructImplier = false;
  };
  /// For each function FD, this table is indexed by FD->getCanonicalDecl().
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

  /// Record use of \p Usee at \p UseLoc within \p User such that the use does
  /// not appear in a compute construct and neither function yet has a fully
  /// determined routine directive.
  void addHostFunctionUse(FunctionDecl *User, FunctionDecl *Usee,
                          SourceLocation UseLoc) {
    assert(!User->getMostRecentDecl()->hasAttr<ACCRoutineDeclAttr>() &&
           "unexpected host function use after routine directive for user");
    assert(!Usee->getMostRecentDecl()->hasAttr<ACCRoutineDeclAttr>() &&
           "unexpected host function use after routine directive for usee");
    FunctionEntryTy &Entry = Map[User->getCanonicalDecl()];
    Entry.HostFunctionUses.try_emplace(Usee->getCanonicalDecl(), UseLoc);
  }

  /// Add routine directive implier with level of parallelism \p ParLevel for
  /// \p FD at \p ImplierLoc.  An implicit routine directive isn't fully
  /// determined until a later call to \c fullyDetermineImplicitRoutineDir,
  /// which uses the information added here.
  ///
  /// \p ImplierConstruct must be either:
  /// - The real kind of the compute construct in which \p ImplierLoc appeared
  ///   if \p ImplierLoc is for a use of \p FD.  The use must have appeared in
  ///   \p ImplierFn.
  /// - \c ACCD_routine if \p ImplierLoc is for a use of \p FD not within a
  ///   compute construct.  The use must have appeared in \p ImplierFn.
  /// - \c ACCD_loop if \p ImplierLoc is for an orphaned loop construct within
  ///   \p FD.  \p ImplierFn must then be \c nullptr.
  /// - \c ACCD_unknown if \p ImplierLoc is for a function call within \p FD.
  ///   \c ImplierFn must then be the callee.
  ///
  /// \c addRoutineDirImplier must not be called after
  /// \c fullyDetermineImplicitRoutineDir for the same \p FD.
  void addRoutineDirImplier(FunctionDecl *FD,
                            ACCRoutineDeclAttr::PartitioningTy ParLevel,
                            SourceLocation ImplierLoc, FunctionDecl *ImplierFn,
                            OpenACCDirectiveKind ImplierConstruct) {
    assert(!FD->getMostRecentDecl()->hasAttr<ACCRoutineDeclAttr>() &&
           "expected function not to have routine directive already");
    assert((isOpenACCComputeDirective(ImplierConstruct) ||
            ImplierConstruct == ACCD_routine || ImplierConstruct == ACCD_loop ||
            ImplierConstruct == ACCD_unknown) &&
           "unexpected routine directive implier kind");
    assert((ImplierFn || ImplierConstruct == ACCD_loop) &&
           "expected function to be specified for specified implier kind");
    FunctionEntryTy &Entry = Map[FD->getCanonicalDecl()];
    if (Entry.ImplierLoc.isInvalid() || Entry.ParLevel < ParLevel) {
      Entry.ImplierLoc = ImplierLoc;
      Entry.ImplierFn = ImplierFn ? ImplierFn->getCanonicalDecl() : nullptr;
      Entry.ImplierConstruct = ImplierConstruct;
      Entry.ParLevel = ParLevel;
    }
    if (ImplierConstruct == ACCD_loop)
      Entry.HasOrphanedLoopConstructImplier = true;
  }

  /// Has an orphaned loop construct within \p FD been added as an implier of a
  /// routine directive for \p FD?
  bool hasOrphanedLoopConstructImplier(FunctionDecl *FD) {
    FunctionEntryTy &Entry = Map[FD->getCanonicalDecl()];
    return Entry.HasOrphanedLoopConstructImplier;
  }

  /// Fully determine routine directive for \p FD based on impliers previously
  /// added by \c addRoutineDirImplier, or do nothing if none.
  ///
  /// In the former case:
  /// - Emit diagnostics that were previously recorded for \p FD by
  ///   \c DiagIfRoutineDir.  \c emitNotesForRoutineDirChain is called for
  ///   \p FD, so its preconditions must be met.
  /// - Recursively add routine seq directives for all functions recorded by
  ///   \c addHostFunctionUse as used by \p FD.
  ///
  /// \c fullyDetermineImplicitRoutineDir must be called at most once per \p FD.
  void fullyDetermineImplicitRoutineDir(FunctionDecl *FD) {
    assert(!FD->getMostRecentDecl()->hasAttr<ACCRoutineDeclAttr>() &&
           "expected function not to have routine directive already");
    FunctionEntryTy &Entry = Map[FD->getCanonicalDecl()];
    if (Entry.ImplierLoc.isInvalid())
      return;
    ACCClause *ParLevelClause;
    switch (Entry.ParLevel) {
    case ACCRoutineDeclAttr::Seq:
      ParLevelClause = SemaRef.ActOnOpenACCSeqClause(
          ACC_IMPLICIT, SourceLocation(), SourceLocation());
      break;
    case ACCRoutineDeclAttr::Vector:
      ParLevelClause = SemaRef.ActOnOpenACCVectorClause(
          ACC_IMPLICIT, SourceLocation(), SourceLocation());
      break;
    case ACCRoutineDeclAttr::Worker:
      ParLevelClause = SemaRef.ActOnOpenACCWorkerClause(
          ACC_IMPLICIT, SourceLocation(), SourceLocation());
      break;
    case ACCRoutineDeclAttr::Gang:
      ParLevelClause = SemaRef.ActOnOpenACCGangClause(
          ACC_IMPLICIT, SourceLocation(), SourceLocation());
      break;
    }
    // Add the ACCRoutineDeclAttr to the most recent FunctionDecl for FD so that
    // it's inherited by any later FunctionDecl for FD and so that we see it in
    // the recursion check below, which examines the most recent FunctionDecl.
    SemaRef.ActOnOpenACCRoutineDirective({ParLevelClause}, ACC_IMPLICIT,
                                         SourceLocation(), SourceLocation(),
                                         DeclGroupRef(FD->getMostRecentDecl()));
    for (auto Diag : Entry.Diags)
      emitRoutineDirDiag(FD, Diag.first, Diag.second);
    Entry.Diags.clear(); // don't need them anymore
    SmallVector<std::pair<FunctionDecl *, SourceLocation>> Uses;
    // fullyDetermineImplicitRoutineDir sometimes inserts a new entry into Map
    // (above), so calling it could relocate the current Entry in memory and
    // break any ongoing iteration over Entry.HostFunctionUses.  Thus, we move
    // Entry.HostFunctionUses to local memory before iterating it below.  We
    // won't need Entry.HostFunctionUses again, so it's fine that it's destroyed
    // afterward.
    auto HostFunctionUses = std::move(Entry.HostFunctionUses);
    for (auto Use : HostFunctionUses) {
      FunctionDecl *Usee = Use.first;
      SourceLocation UseeUseLoc = Use.second;
      Usee = Usee->getMostRecentDecl();
      if (Usee->hasAttr<ACCRoutineDeclAttr>())
        continue; // avoid infinite recursion
      addRoutineDirImplier(Usee, ACCRoutineDeclAttr::Seq, UseeUseLoc, FD,
                           ACCD_routine);
      fullyDetermineImplicitRoutineDir(Usee);
    }
  }

  /// Emit notes identifying the origin of the routine directive for \p FD.
  /// If \p PreviousDir, then point out that it was a previous routine directive
  /// to contrast with a new routine directive.
  ///
  /// If \p FD has an explicit routine directive, report it.  Otherwise, report
  /// the original implier of the routine directive for \p FD with its current
  /// level of parallelism.  If that implier is a use of \p FD in a function,
  /// recursively emit notes identifying the origin of the routine directive for
  /// the function in which the use appears.  If that implier is a function call
  /// within the body of \p FD, recursively emit notes identifying the origin of
  /// the routine directive for the function called.
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
          << PreviousDir << NameForDiag(SemaRef, FD);
      return;
    }

    // Handle implicit routine directive.
    const FunctionEntryTy &Entry = Map.lookup(FD->getCanonicalDecl());
    assert(Entry.ImplierLoc.isValid() &&
           "expected function to have implicit routine directive");
    StringRef ParLevel =
        ACCRoutineDeclAttr::ConvertPartitioningTyToStr(Attr->getPartitioning());
    bool DefNotUse;
    bool FnNotConstruct;
    if (isOpenACCComputeDirective(Entry.ImplierConstruct)) {
      DefNotUse = 0;
      FnNotConstruct = 0;
    } else if (Entry.ImplierConstruct == ACCD_routine) {
      DefNotUse = 0;
      FnNotConstruct = 1;
    } else if (Entry.ImplierConstruct == ACCD_loop) {
      DefNotUse = 1;
      FnNotConstruct = 0;
    } else if (Entry.ImplierConstruct == ACCD_unknown) {
      DefNotUse = 1;
      FnNotConstruct = 1;
    } else {
      llvm_unreachable("unexpected routine directive implier");
    }
    {
      Sema::SemaDiagnosticBuilder Diag =
          SemaRef.Diag(Entry.ImplierLoc, diag::note_acc_routine_implicit)
          << ParLevel << PreviousDir << NameForDiag(SemaRef, FD) << DefNotUse
          << FnNotConstruct;
      if (FnNotConstruct)
        Diag << NameForDiag(SemaRef, Entry.ImplierFn);
      else
        Diag << getOpenACCName(Entry.ImplierConstruct);
    }
    if (FnNotConstruct)
      emitNotesForRoutineDirChain(Entry.ImplierFn, /*PreviousDir=*/false);
  }

  /// A diagnostic for a function that is emitted either immediately if the
  /// function is already known to have a routine directive or later when a
  /// routine directive is implied for it, if ever.  In either case, notes
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
    /// The diagnostic to be emitted is \p DiagID at \p DiagLoc for function
    /// \p FD.
    ///
    /// If \p Emitted is not \c nullptr, \p *Emitted is set to true or false
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
      if (FD->getMostRecentDecl()->hasAttr<ACCRoutineDeclAttr>()) {
        ImplicitRoutineDirInfo.emitRoutineDirDiag(FD, DiagLoc, *this);
        if (Emitted)
          *Emitted = true;
        return;
      }
      if (Emitted)
        *Emitted = false;
      FunctionEntryTy &Entry =
          ImplicitRoutineDirInfo.Map[FD->getCanonicalDecl()];
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
  /// This table records locations of all uses discovered so far for all
  /// functions.  For each function FD, it is indexed by FD->getCanonicalDecl().
  ///
  /// This table is maintained in order to produce notes upon an error.  If we
  /// find it's too big, we might decide it's sufficient to record and report
  /// just a single use.
  llvm::DenseMap<FunctionDecl *, llvm::SmallVector<SourceLocation>>
      FunctionUses;
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
  case ACCD_wait:
  case ACCD_data:
  case ACCD_parallel:
  case ACCD_loop:
  case ACCD_atomic:
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
  FunctionDecl *CurFnDecl = getCurFunctionDecl(/*AllowLambda=*/true);
  if (CurFnDecl && !isOpenACCDirectiveStmt(ParentDKind)) {
    assert((ParentDKind == ACCD_unknown || ParentDKind == ACCD_routine) &&
           "expected the only non-ACCDirectiveStmt OpenACC directive to be the "
           "routine directive");
    // Assert that any lexically enclosing routine directive (that is,
    // ParentDKind == ACCD_routine) is already attached as an ACCRoutineDeclAttr
    // to a lexically enclosing function.  That function might not be the
    // nearest lexically enclosing function (CurFnDecl) if the nearest is a
    // lambda without a routine directive.  However, if it is the nearest, it's
    // important that the ACCRoutineDeclAttr has already been attached to it or
    // we'll overlook the routine directive for the diagnostics below.
    //
    // The only exception is that an erroneous routine directive might have been
    // discarded, leaving no ACCRoutineDeclAttr or a previous routine
    // directive's ACCRoutineDeclAttr.
#ifndef NDEBUG
    if (ParentDKind == ACCD_routine && !getDiagnostics().hasErrorOccurred()) {
      DeclContext *DCFind = CurFnDecl;
      ACCRoutineDeclAttr *AttrFind = nullptr;
      do {
        if (FunctionDecl *FDFind = dyn_cast_or_null<FunctionDecl>(DCFind)) {
          AttrFind = FDFind->getAttr<ACCRoutineDeclAttr>();
          if (AttrFind && ParentLoc == AttrFind->getLoc())
            break;
          AttrFind = nullptr;
        }
        DCFind = DCFind->getLexicalParent();
      } while (DCFind);
      assert(AttrFind &&
             "expected enclosing routine directive to already be attached as "
             "ACCRoutineDeclAttr to an enclosing FunctionDecl");
    }
#endif
    assert(isAllowedParentForDirective(RealDKind, ACCD_unknown) &&
           "expected every OpenACC directive to be permitted without a "
           "lexically enclosing OpenACC directive");
    if (!isAllowedParentForDirective(RealDKind, ACCD_routine)) {
      bool Err;
      DiagIfRoutineDir(OpenACCData->ImplicitRoutineDirInfo, CurFnDecl, StartLoc,
                       diag::err_acc_routine_unexpected_directive, &Err)
          << getOpenACCName(RealDKind) << NameForDiag(*this, CurFnDecl);
      return Err;
    }
    assert((RealDKind == ACCD_loop || RealDKind == ACCD_atomic ||
            RealDKind == ACCD_routine) &&
           "expected orphaned loop construct, atomic construct, and routine "
           "directive to be the only permitted OpenACC directives in function "
           "with routine directive");
    // Proposed text for OpenACC after 3.3:
    // "A procedure definition containing an orphaned loop construct must be in
    // the scope of an explicit and applying routine directive if the procedure
    // is not a C++ lambda."
    ACCRoutineDeclAttr *Attr = CurFnDecl->getAttr<ACCRoutineDeclAttr>();
    if (RealDKind == ACCD_loop && (!Attr || !Attr->getLoc().isValid()) &&
        !isLambdaCallOperator(CurFnDecl)) {
      Diag(StartLoc, diag::err_acc_routine_for_orphaned_loop)
          << NameForDiag(*this, CurFnDecl);
      return true;
    }
    return false;
  }
  if (!isAllowedParentForDirective(RealDKind, ParentDKind)) {
    // The following diagnostic does not make sense for the case that the
    // current OpenACC directive isn't permitted without a parent directive.
    assert(ParentDKind != ACCD_unknown &&
           "expected every OpenACC directive to be permitted without a "
           "lexically enclosing OpenACC directive");
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
  case ACCC_tile:
  case ACCC_async:
  case ACCC_wait:
  case ACCC_read:
  case ACCC_write:
  case ACCC_update:
  case ACCC_capture:
  case ACCC_compare:
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
/// Computes implicit gang, worker, and vector clauses on loop constructs.
///
/// For implicit gang clause, this follows the OpenACC spec.  Implicit worker
/// and vector clauses are not specified by OpenACC, but this follows a similar
/// approach as for implicit gang clauses.  See the Clang OpenACC status
/// document for details.
class ImplicitGangWorkerVectorAdder
    : public StmtVisitor<ImplicitGangWorkerVectorAdder> {
private:
  Sema &SemaRef;
  llvm::SmallVector<ACCRoutineDeclAttr::PartitioningTy> MaxPartStack;
  /// \p MaxPart must be the maximum partitioning level permitted for the
  /// current context.  If \p LD is not \c nullptr, then it must be within the
  /// current context.  In that case, add implicit gang, worker, and vector
  /// clauses to \p LD if possible, and consider the body of \p LD to be the
  /// current context now.  In either case, compute and return a new \p MaxPart
  /// appropriate for the current context while excluding any partitioning
  /// levels for which implicit clauses are disabled.
  ACCRoutineDeclAttr::PartitioningTy
  updateMaxPartAndLoop(ACCRoutineDeclAttr::PartitioningTy MaxPart,
                       ACCLoopDirective *LD = nullptr) {
    bool ImpWorkerOnVector = false;
    bool ImpWorkerOnOuter = false;
    switch (SemaRef.getLangOpts().getOpenACCImplicitWorker()) {
    case LangOptions::OpenACCImplicitWorker_None:
      break;
    case LangOptions::OpenACCImplicitWorker_Vector:
      ImpWorkerOnVector = true;
      break;
    case LangOptions::OpenACCImplicitWorker_Outer:
      ImpWorkerOnOuter = true;
      break;
    case LangOptions::OpenACCImplicitWorker_VectorOuter:
      ImpWorkerOnVector = true;
      ImpWorkerOnOuter = true;
      break;
    }
    bool ImpVectorOnOuter = false;
    switch (SemaRef.getLangOpts().getOpenACCImplicitVector()) {
    case LangOptions::OpenACCImplicitVector_None:
      break;
    case LangOptions::OpenACCImplicitVector_Outer:
      ImpVectorOnOuter = true;
      break;
    }
    // If we have an independent loop, try to add partitioning levels.
    if (LD && LD->getPartitioning().hasIndependent()) {
      ACCRoutineDeclAttr::PartitioningTy MinPartExcl = ACCRoutineDeclAttr::Seq;
      if (LD->getNestedExplicitGangPartitioning())
        MinPartExcl = ACCRoutineDeclAttr::Gang;
      else if (LD->getNestedExplicitWorkerPartitioning())
        MinPartExcl = ACCRoutineDeclAttr::Worker;
      else if (LD->getNestedExplicitVectorPartitioning())
        MinPartExcl = ImpWorkerOnVector ? ACCRoutineDeclAttr::Worker
                                        : ACCRoutineDeclAttr::Vector;
      // (MinPartExcl,MaxPart] is now the partitioning range permitted on LD.
      for (int Part = MaxPart; Part > MinPartExcl; --Part) {
        switch ((ACCRoutineDeclAttr::PartitioningTy)Part) {
        case ACCRoutineDeclAttr::Gang:
          if (!LD->getPartitioning().hasGangPartitioning())
            LD->addImplicitGangClause();
          break;
        case ACCRoutineDeclAttr::Worker:
          if (LD->getPartitioning().hasWorkerPartitioning())
            break;
          if (ImpWorkerOnOuter ||
              (ImpWorkerOnVector &&
               LD->getPartitioning().hasVectorPartitioning()))
            LD->addImplicitWorkerClause();
          break;
        case ACCRoutineDeclAttr::Vector:
          if (ImpVectorOnOuter &&
              !LD->getPartitioning().hasVectorPartitioning())
            LD->addImplicitVectorClause();
          break;
        case ACCRoutineDeclAttr::Seq:
          llvm_unreachable("expected seq to be excluded from iteration");
        }
      }
      // Every partitioning above MinPartExcl cannot be added within LD for one
      // of the following reasons: it's above the original MaxPart, it's already
      // on LD, it was just added to LD, or adding it implicitly anywhere is
      // disabled.  Thus, MinPartExcl is the new MaxPart.
      MaxPart = MinPartExcl;
    }
    // Look for potential remaining partitioning in this context.
    int Part;
    for (Part = MaxPart; Part > ACCRoutineDeclAttr::Seq; --Part) {
      bool Found = false;
      switch ((ACCRoutineDeclAttr::PartitioningTy)Part) {
      case ACCRoutineDeclAttr::Gang:
        Found = true;
        break;
      case ACCRoutineDeclAttr::Worker:
        if (ImpWorkerOnOuter)
          Found = true;
        else if (ImpWorkerOnVector &&
                 (!LD || LD->getNestedExplicitVectorPartitioning()))
          Found = true;
        break;
      case ACCRoutineDeclAttr::Vector:
        if (ImpVectorOnOuter)
          Found = true;
        break;
      case ACCRoutineDeclAttr::Seq:
        llvm_unreachable("expected seq to be excluded from iteration");
      }
      if (Found)
        break;
    }
    return (ACCRoutineDeclAttr::PartitioningTy)Part;
  }

public:
  void VisitACCDirectiveStmt(ACCDirectiveStmt *D) {
    ACCLoopDirective *LD = dyn_cast<ACCLoopDirective>(D);
    // It shouldn't be possible to encounter a combined loop construct because
    // ImplicitGangWorkerVectorAdder is always called on a compute construct
    // body, an accelerator function's body, or an orphaned loop.
    assert(isOpenACCLoopDirective(D->getDirectiveKind()) == (LD != nullptr) &&
           "expected non-combined acc loop directive");
    ACCRoutineDeclAttr::PartitioningTy MaxPart =
        updateMaxPartAndLoop(MaxPartStack.back(), LD);
    if (MaxPart == ACCRoutineDeclAttr::PartitioningTy::Seq)
      return;
    if (LD)
      MaxPartStack.push_back(MaxPart);
    for (auto *C : D->children()) {
      if (C)
        Visit(C);
    }
    if (LD)
      MaxPartStack.pop_back();
  }
  void VisitStmt(Stmt *S) {
    // Running updateMaxPartAndLoop is useless here because nothing can have
    // changed the maximum partitioning level since the last time it was run.
    // However, the constructor cannot stop the AST descent, so check if it
    // discovered that implicit partitioning is already exhausted.
    if (MaxPartStack.back() == ACCRoutineDeclAttr::Seq)
      return;
    for (Stmt *C : S->children()) {
      if (C)
        Visit(C);
    }
  }
  /// \p MaxPart must be the maximum partitioning level permitted in the current
  /// context (e.g., by a routine directive).
  ImplicitGangWorkerVectorAdder(Sema &SemaRef,
                                ACCRoutineDeclAttr::PartitioningTy MaxPart)
      : SemaRef(SemaRef) {
    // Running updateMaxPartAndLoop here tries to eliminate partitioning levels
    // for which implicit clauses are disabled.
    MaxPartStack.push_back(updateMaxPartAndLoop(MaxPart));
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
    Expr *E;
    /// The reduction clause that produced the implicit DA, or nullptr if the
    /// implicit DA is not a reduction.
    ACCReductionClause *ReductionClause;
    Implier(Expr *E, ACCReductionClause *C = nullptr)
        : E(E), ReductionClause(C) {}
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
    void setDMA(OpenACCDMAKind Kind, Expr *E) {
      assert(DMAKind == ACC_DMA_unknown && "expected unset DMA");
      DMAKind = Kind;
      ImpliersTy &Impliers = Table->getImpliers(DMAKind);
      Impliers.emplace_back(E);
      DMAImplier = std::prev(Impliers.end());
    }
    void setDSA(OpenACCDSAKind Kind, Expr *E,
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
  llvm::DenseMap<ACCDataVar, Entry> Map;

public:
  Entry &lookup(ACCDataVar Var) {
    return Map.try_emplace(Var, Entry(this)).first->second;
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
      ExprVec.push_back(I.E);
    return !ExprVec.empty();
  }
};

/// Computes all implicit DAs except implicit reductions, which are computed
/// by \c ImplicitGangReductionAdder.
class ImplicitDAAdder : public StmtVisitor<ImplicitDAAdder> {
  typedef StmtVisitor<ImplicitDAAdder> BaseVisitor;
  DirStackTy &DirStack;
  ImplicitDATable &ImplicitDAs;
  llvm::SmallVector<llvm::DenseSet<ACCDataVar>, 8> LocalDefinitions;
  size_t ACCDirectiveStmtCount;
  std::list<Expr *> ScalarReductionVarDiags;
  llvm::DenseMap<ACCDataVar, SourceLocation> ScalarGangReductionVarDiags;
  /// Add any required implicit attributes for the variable referenced by the
  /// expression \p E unless \p FinishOnly and the variable has no attributes
  /// yet visible at the directive.
  ///
  /// Returns true if the variable now has all implicit attributes on the
  /// directive as required by \p E.  Returns false otherwise (the only case is
  /// that \p FinishOnly == \c true prevented adding attributes).
  bool VisitVar(Expr *E, bool FinishOnly = false) {
    ACCDataVar Var(E);

    // Skip variable if it is locally declared or privatized by a clause.
    if (LocalDefinitions.back().count(Var))
      return true;

    // Skip variable if an implicit DA has already been computed.
    ImplicitDATable::Entry &ImplicitDA = ImplicitDAs.lookup(Var);
    if (ImplicitDA.getDMAKind() != ACC_DMA_unknown ||
        ImplicitDA.getDSAKind() != ACC_DSA_unknown)
      return true;

    // Get predetermined and explicit DAs.
    const DirStackTy::DAVarData &VarData = DirStack.getTopDA(Var);

    // Compute implicit DAs.
    OpenACCDMAKind DMAKind = ACC_DMA_unknown;
    OpenACCDSAKind DSAKind = ACC_DSA_unknown;
    if (isOpenACCLoopDirective(DirStack.getEffectiveDirective())) {
      if (VarData.DSAKind != ACC_DSA_unknown) {
        // There's a predetermined or explicit DSA, so there's no implicit DA.
      } else if (FinishOnly) {
        assert(VarData.DMAKind == ACC_DMA_unknown &&
               "expected no DMA on loop directive");
        return false;
      } else {
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
        assert((!DirStack.hasLoopControlVariable(Var) ||
                DirStack.getLoopPartitioning().hasSeqExplicit()) &&
               "expected predetermined private for loop control variable "
               "with explicit seq");
        // See the section "Basic Data Attributes" in the Clang OpenACC
        // design document for discussion of the shared data attribute.
        DSAKind = ACC_DSA_shared;
      }
    } else if (isOpenACCParallelDirective(DirStack.getEffectiveDirective())) {
      if (DirStack.hasVisibleDMA(Var) || VarData.DSAKind != ACC_DSA_unknown) {
        // There's a visible explicit DMA, or there's a predetermined or
        // explicit DSA, so there's no implicit DA other than the defaults.
      } else if (FinishOnly) {
        return false;
      } else if (!Var.getType()->isScalarType()) {
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
      if (DMAKind == ACC_DMA_unknown && VarData.DMAKind == ACC_DMA_unknown)
        DMAKind = ACC_DMA_nomap;
      if (DSAKind == ACC_DSA_unknown && VarData.DSAKind == ACC_DSA_unknown)
        DSAKind = ACC_DSA_shared;
    } else {
      llvm_unreachable("unexpected effective directive");
    }

    // Store implicit DAs for later conversion to clause objects.
    if (DMAKind != ACC_DMA_unknown)
      ImplicitDA.setDMA(DMAKind, E);
    if (DSAKind != ACC_DSA_unknown)
      ImplicitDA.setDSA(DSAKind, E);
    return true;
  }

public:
  void VisitDeclStmt(DeclStmt *S) {
    for (auto I = S->decl_begin(), E = S->decl_end(); I != E; ++I)
      LocalDefinitions.back().insert(ACCDataVar(*I));
    BaseVisitor::VisitDeclStmt(S);
  }
  void VisitDeclRefExpr(DeclRefExpr *DRE) {
    if (isa<VarDecl>(DRE->getDecl()))
      VisitVar(DRE);
  }
  void VisitCXXThisExpr(CXXThisExpr *TE) {
    SourceLocation Loc = TE->getEndLoc();
    ExprResult TENew = DirStack.SemaRef.BuildCXXThisExpr(
        TE->getLocation(), TE->getType(), /*IsImplicit=*/true);
    if (TENew.isInvalid())
      return;
    ExprResult Zero = DirStack.SemaRef.ActOnIntegerConstant(Loc, 0).get();
    if (Zero.isInvalid())
      return;
    ExprResult One = DirStack.SemaRef.ActOnIntegerConstant(Loc, 1).get();
    if (One.isInvalid())
      return;
    ExprResult Subarray = DirStack.SemaRef.ActOnOMPArraySectionExpr(
        /*Base=*/TE, /*LBLoc=*/Loc, /*LowerBound=*/Zero.get(),
        /*ColonLocFirst=*/Loc, /*ColonLocSecond=*/SourceLocation(),
        /*Length=*/One.get(), /*Stride=*/nullptr, /*RBLoc=*/Loc);
    if (Subarray.isInvalid())
      return;
    VisitVar(Subarray.get());
  }
  // So far in our experiments, a CXXMemberCallExpr has a MemberExpr child,
  // which is handled properly here.
  void VisitMemberExpr(MemberExpr *ME) {
    // If the member has a visible data clause, finish its implicit data
    // attributes.  Otherwise, compute implicit data attributes for the base.
    //
    // There's one exception: if the base has an explicit data clause on this
    // directive, then that covers the member too.  Don't confuse things (like
    // OpenMP codegen) by adding potentially contradictory implicit data
    // attributes for the member.  Just compute implicit data attributes for the
    // base instead.
    //
    // If an ACCDataVar ctor call below can handle an expression, then that
    // expression might appear in a data clause.  If not (it creates an invalid
    // ACCDataVar), then we can safely assume it doesn't appear in one.
    ACCDataVar MEVar(ME, ACCDataVar::AllowMemberExprOnAny,
                     /*AllowSubarray=*/true, &DirStack.SemaRef, /*Quiet=*/true);
    if (MEVar.isValid()) {
      ACCDataVar MEBaseVar(ME->getBase(), ACCDataVar::AllowMemberExprOnAny,
                           /*AllowSubarray=*/true, &DirStack.SemaRef,
                           /*Quiet=*/true);
      DirStackTy::DAVarData MEBaseVarData;
      if (MEBaseVar.isValid())
        MEBaseVarData = DirStack.getTopDA(MEBaseVar);
      if (MEBaseVarData.DMAKind == ACC_DMA_unknown &&
          MEBaseVarData.DSAKind == ACC_DSA_unknown &&
          VisitVar(ME, /*FinishOnly=*/true))
        return;
    }
    // In the case of this->x or s.x, the following produces copy(this[0:1]) or
    // copy(s).  In the case of p->x, it produces firstprivate(p) because p is a
    // scalar.  It's up to the programmer to get that right.
    Visit(ME->getBase());
  }
  void VisitACCDirectiveStmt(ACCDirectiveStmt *D) {
    ++ACCDirectiveStmtCount;

    // Push space for local definitions in this construct.  It's important to
    // force a copy construct call here so that, if the vector is resized, the
    // pass-by-reference parameter isn't invalidated before it's copied.
    LocalDefinitions.emplace_back(
        llvm::DenseSet<ACCDataVar>(LocalDefinitions.back()));

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
    ACCLoopDirective *LD = dyn_cast<ACCLoopDirective>(D);
    bool IsGangReduction = LD && LD->getPartitioning().hasGangPartitioning();

    // Search this directive's clauses for reductions requiring data clauses and
    // for privatizations.
    for (ACCClause *C : D->clauses()) {
      if (!C)
        continue;

      // Record diagnostics for missing data clauses for scalar loop reductions.
      if (ScalarReductionRequiresDataClause &&
          C->getClauseKind() == ACCC_reduction) {
        for (Expr *VR : cast<ACCReductionClause>(C)->varlists()) {
          ACCDataVar Var(VR);
          // Skip variable if it is locally declared or privatized by a clause.
          if (LocalDefinitions.back().count(Var))
            continue;
          // Skip variable if its type is dependent.  The diagnostic won't make
          // sense if the variable turns out never to be a scalar.
          if (Var.getType()->isDependentType())
            continue;
          // Currently, Clang supports only scalar variables in OpenACC
          // reductions.
          assert(Var.getType()->isScalarType() &&
                 "expected reduction variable to be scalar");
          // If there's a visible explicit DMA or if there's an explicit DSA on
          // the compute construct, the restriction is satisfied.
          DirStackTy::DAVarData VarData = DirStack.getTopDA(Var);
          if (DirStack.hasVisibleDMA(Var) || VarData.DSAKind != ACC_DSA_unknown)
            continue;
          // Record diagnostic.
          ScalarReductionVarDiags.push_back(VR);
          if (IsGangReduction)
            ScalarGangReductionVarDiags.try_emplace(Var, VR->getExprLoc());
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
      for (Expr *E : getPrivateVarsFromClause(C))
        LocalDefinitions.back().insert(ACCDataVar(E));
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
        ACCDataVar Var(VR);
        const DirStackTy::DAVarData &VarData = DirStack.getTopDA(Var);
        if (VarData.DMAKind != ACC_DMA_unknown)
          continue;
        ImplicitDATable::Entry &ImplicitDA = ImplicitDAs.lookup(Var);
        assert(ImplicitDA.getDMAKind() == ACC_DMA_unknown &&
               ImplicitDA.getDSAKind() == ACC_DSA_unknown &&
               "expected no implicit DA to be set yet");
        ImplicitDA.setDMA(ACC_DMA_copy, VR);
        if (VarData.DSAKind == ACC_DSA_unknown)
          ImplicitDA.setDSA(ACC_DSA_shared, VR);
        else
          assert(VarData.DSAKind == ACC_DSA_reduction &&
                 "expected any explicit DSA for reduction var to be "
                 "reduction");
      }
    }
    LocalDefinitions.emplace_back();
  }
  void reportDiags() const {
    for (Expr *E : ScalarReductionVarDiags) {
      ACCDataVar Var(E);
      DirStack.SemaRef.Diag(E->getExprLoc(),
                            diag::err_acc_loop_reduction_needs_data_clause)
          << NameForDiag(DirStack.SemaRef, Var)
          << getOpenACCName(DirStack.getRealDirective());
      DirStack.SemaRef.Diag(DirStack.getDirectiveStartLoc(),
                            diag::note_acc_parent_compute_construct)
          << getOpenACCName(DirStack.getRealDirective());
      SourceLocation GangRedLoc = ScalarGangReductionVarDiags.lookup(Var);
      // OpenACC compilers typically implicitly determine copy clauses for gang
      // reductions, which are not so useful otherwise, so suggest that if there
      // is a gang reduction for this variable.  Otherwise, suggest
      // firstprivate, which makes sense when it's not reduced across gangs.
      if (GangRedLoc.isInvalid()) {
        DirStack.SemaRef.Diag(
            DirStack.getDirectiveStartLoc(),
            diag::note_acc_loop_reduction_suggest_firstprivate)
            << NameForDiag(DirStack.SemaRef, Var)
            << getOpenACCName(DirStack.getRealDirective());
        continue;
      }
      DirStack.SemaRef.Diag(GangRedLoc,
                            diag::note_acc_loop_reduction_suggest_copy)
          << NameForDiag(DirStack.SemaRef, Var);
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
  llvm::SmallVector<llvm::DenseSet<ACCDataVar>, 8> LocalDefinitions;

public:
  void VisitDeclStmt(DeclStmt *S) {
    for (auto I = S->decl_begin(), E = S->decl_end(); I != E; ++I)
      LocalDefinitions.back().insert(ACCDataVar(*I));
    BaseVisitor::VisitDeclStmt(S);
  }
  void VisitACCDirectiveStmt(ACCDirectiveStmt *D) {
    // If this is an acc loop, compute gang reductions implied here.
    if (isOpenACCLoopDirective(D->getDirectiveKind())) {
      for (ACCReductionClause *C : D->getClausesOfKind<ACCReductionClause>()) {
        for (Expr *VR : C->varlists()) {
          ACCDataVar Var(VR);

          // Skip variable if it is declared locally in the acc parallel or if
          // it is privatized by an enclosing acc loop.
          if (LocalDefinitions.back().count(Var))
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
          DirStackTy::DAVarData VarData = DirStack.getTopDA(Var);
          ImplicitDATable::Entry &ImplicitDA = ImplicitDAs.lookup(Var);
          if (VarData.DSAKind == ACC_DSA_reduction ||
              ImplicitDA.getDSAKind() == ACC_DSA_reduction)
            continue;

          // The reduction can be added regardless of the DMA.
          assert(
              isAllowedDSAForDMA(ACC_DSA_reduction, VarData.DMAKind) &&
              isAllowedDSAForDMA(ACC_DSA_reduction, ImplicitDA.getDMAKind()) &&
              "expected reduction not to conflict with DMA");

          // If it's not privatized, then record the reduction.
          if (ImplicitDA.getDSAKind() == ACC_DSA_shared) {
            assert(VarData.DSAKind == ACC_DSA_unknown &&
                   "expected no implicit DSA with explicit/predetermined DSA");
            ImplicitDA.unsetDSA();
            ImplicitDA.setDSA(ACC_DSA_reduction, VR, C);
          }
        }
      }
    }

    // Push space for local definitions in this construct.  It's important to
    // force a copy construct call here so that, if the vector is resized, the
    // pass-by-reference parameter isn't invalidated before it's copied.
    LocalDefinitions.emplace_back(
        llvm::DenseSet<ACCDataVar>(LocalDefinitions.back()));

    // Record variables privatized by this directive as local definitions so
    // that they are skipped while computing gang reductions implied by
    // nested directives.
    for (ACCClause *C : D->clauses()) {
      if (!C)
        continue;
      for (Expr *E : getPrivateVarsFromClause(C))
        LocalDefinitions.back().insert(ACCDataVar(E));
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
    Expr *RE;
    ReductionVar() : ReductionClause(nullptr), RE(nullptr) {}
    ReductionVar(ACCReductionClause *C, Expr *RE)
        : ReductionClause(C), RE(RE) {}
  };
  typedef StmtVisitor<NestedReductionChecker> BaseVisitor;
  DirStackTy &DirStack;
  llvm::SmallVector<llvm::DenseMap<ACCDataVar, ReductionVar>, 8> Privates;
  bool Error = false;

public:
  void VisitACCDirectiveStmt(ACCDirectiveStmt *D) {
    // Push space for privates in this construct.  It's important to force a
    // copy construct call here so that, if the vector is resized, the
    // pass-by-reference parameter isn't invalidated before it's copied.
    Privates.emplace_back(
        llvm::DenseMap<ACCDataVar, ReductionVar>(Privates.back()));

    // Record variables privatized by this directive, and complain for any
    // reduction conflicting with its immediately enclosing reduction.
    for (ACCClause *C : D->clauses()) {
      if (!C)
        continue;
      if (C->getClauseKind() == ACCC_reduction) {
        ACCReductionClause *ReductionClause = cast<ACCReductionClause>(C);
        for (Expr *VR : ReductionClause->varlists()) {
          // Try to record this variable's reduction.
          ACCDataVar Var(VR);
          auto Insert = Privates.back().try_emplace(Var, ReductionClause, VR);

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
            DirStack.SemaRef.Diag(VR->getExprLoc(),
                                  diag::err_acc_conflicting_reduction)
                << false
                << ACCReductionClause::printReductionOperatorToString(
                       ReductionClause->getNameInfo())
                << NameForDiag(DirStack.SemaRef, Var);
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
          EnclosingPrivate = ReductionVar(ReductionClause, VR);
        }
      } else {
        // Record variables privatized by clauses other than reductions.
        for (Expr *E : getPrivateVarsFromClause(C)) {
          ACCDataVar Var(E);
          Privates.back()[Var] = ReductionVar();
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
        LoopKind.getMaxParLevelClause();
    OpenACCDirectiveKind ParentDKind;
    SourceLocation ParentLoopLoc;
    ACCRoutineDeclAttr::PartitioningTy ParentLoopLevel =
        OpenACCData->DirStack
            .getAncestorLoopPartitioning(ParentDKind, ParentLoopLoc,
                                         /*SkipCurrentDir=*/true)
            .getMinParLevelClause();
    bool RoutineDirImplier = false;
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
      // for the enclosing function, we already complained about that, or we
      // will record the loop construct as a routine directive implier if the
      // enclosing function is a lambda.  If there is a routine directive,
      // complain if the level of parallelism is incompatible.
      FunctionDecl *Fn = getCurFunctionDecl(/*AllowLambda=*/true);
      assert(Fn && "expected acc loop to be in a function");
      ACCRoutineDeclAttr *FnAttr = Fn->getAttr<ACCRoutineDeclAttr>();
      if (FnAttr) {
        ACCRoutineDeclAttr::PartitioningTy FnLevel = FnAttr->getPartitioning();
        if (FnLevel < LoopLevel) {
          Diag(StartLoc, diag::err_acc_loop_func_par_level)
              << NameForDiag(*this, Fn)
              << ACCRoutineDeclAttr::ConvertPartitioningTyToStr(FnLevel)
              << getOpenACCName(DKind)
              << ACCRoutineDeclAttr::ConvertPartitioningTyToStr(LoopLevel);
          OpenACCData->ImplicitRoutineDirInfo.emitNotesForRoutineDirChain(Fn);
        }
      } else if (isLambdaCallOperator(Fn)) {
        RoutineDirImplier = true;
      }
    }

    // TODO: For now, we prescriptively map auto to sequential execution, but
    // obviously an OpenACC compiler is meant to sometimes determine that
    // independent is possible.
    if (LoopKind.hasAuto())
      LoopKind.setSeqComputed();

    // Now that auto clause analysis is done, record any routine directive
    // implier with the resulting level of parallelism.
    if (RoutineDirImplier) {
      FunctionDecl *Fn = getCurFunctionDecl(/*AllowLambda=*/true);
      OpenACCData->ImplicitRoutineDirInfo.addRoutineDirImplier(
          Fn, LoopKind.getMaxPartitioningLevel(), StartLoc,
          /*ImplierFn=*/nullptr, ACCD_loop);
    }

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
  switch (DKind) {
  case ACCD_parallel_loop:
    assert(isOpenACCCombinedDirective(DKind) &&
           "expected combined OpenACC directive");
    return ActOnOpenACCParallelLoopDirective(Clauses, AStmt);
  case ACCD_update:
  case ACCD_enter_data:
  case ACCD_exit_data:
  case ACCD_wait:
  case ACCD_data:
  case ACCD_parallel:
  case ACCD_loop:
  case ACCD_atomic:
  case ACCD_routine:
  case ACCD_unknown:
    assert(!isOpenACCCombinedDirective(DKind) &&
           "expected non-combined OpenACC directive");
    break;
  }

  // Compute implicit independent clause for loop construct.
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

  // Compute implicit update clause for atomic construct.
  if (DKind == ACCD_atomic && ComputedClauses.empty()) {
    ACCClause *Implicit = ActOnOpenACCUpdateClause(
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

  // Compute implicit gang/worker/vector clauses within compute constructs,
  // compute predetermined DAs, and compute implicit DAs.  Complain about
  // reductions for loop control variables.
  if (AStmt &&
      (isOpenACCParallelDirective(DKind) || isOpenACCLoopDirective(DKind))) {
    // Add implicit gang/worker/vector clauses to acc loops nested in the acc
    // parallel.
    if (isOpenACCParallelDirective(DKind))
      ImplicitGangWorkerVectorAdder(*this, ACCRoutineDeclAttr::Gang)
          .Visit(AStmt);

    // Iterate acc loop control variables.
    llvm::SmallVector<Expr *, 8> PrePrivate;
    for (Expr *RefExpr : DirStack.getLoopControlVariables()) {
      ACCDataVar Var(RefExpr);
      const DirStackTy::DAVarData &VarData = DirStack.getTopDA(Var);

      // Complain for any reduction.
      //
      // This restriction is not clearly specified in OpenACC 3.0.  See the
      // section "Loop Control Variables" in the Clang OpenACC design document.
      if (VarData.DSAKind == ACC_DSA_reduction) {
        Diag(VarData.DSARefExpr->getEndLoc(),
             diag::err_acc_reduction_on_loop_control_var)
            << NameForDiag(*this, Var) << VarData.DSARefExpr->getSourceRange();
        Diag(RefExpr->getExprLoc(), diag::note_acc_loop_control_var)
            << NameForDiag(*this, Var) << RefExpr->getSourceRange();
        ErrorFound = true;
        continue;
      }

      // Complain for member expression.
      //
      // ActOnOpenACCPrivateClause below would do this, but its diagnostic would
      // talk about what's permitted in a private clause, which would be
      // confusing because the private clause in this case is implicit.
      if (Var.isMember()) {
        Expr *BaseExpr =
            cast<MemberExpr>(RefExpr)->getBase()->IgnoreParenImpCasts();
        bool OnImplicitCXXThis = false;
        if (CXXThisExpr *TE = dyn_cast<CXXThisExpr>(BaseExpr))
          OnImplicitCXXThis = TE->isImplicit();
        Diag(RefExpr->getBeginLoc(),
             diag::err_acc_unsupported_member_expr_loop_control_variable)
            << (OnImplicitCXXThis ? 1 : 0) << RefExpr->getSourceRange();
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
          VarData.DSAKind != ACC_DSA_private) {
        // OpenACC 3.0 sec. 2.6 "Data Environment" L1023-1024:
        //   "Variables with predetermined data attributes may not appear in a
        //   data clause that conflicts with that data attribute."
        // As far as we know, it's not actually possible to create such a
        // conflict except with a DA on the parent effective directive in a
        // combined directive.  See the section "Basic Data Attributes" in the
        // Clang OpenACC design document for further discussion.
        assert(VarData.DSAKind == ACC_DSA_unknown &&
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
          I.E, ACC_IMPLICIT, SourceLocation(), SourceLocation(),
          SourceLocation(), SourceLocation(), I.ReductionClause->getNameInfo());
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
  case ACCD_wait:
    assert(!AStmt &&
           "expected no associated statement for 'acc wait' directive");
    Res = ActOnOpenACCWaitDirective(ComputedClauses);
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
    for (Expr *RefExpr : DirStack.getLoopControlVariables()) {
      ACCDataVar Var(RefExpr);
      assert(Var.hasReferencedDecl() &&
             "expected loop control variable to have a declaration");
      VarDecl *VD = cast<VarDecl>(Var.getReferencedDecl());
      LCVVars.push_back(VD);
    }
    Res = ActOnOpenACCLoopDirective(ComputedClauses, AStmt, LCVVars);
    break;
  }
  case ACCD_atomic:
    Res = ActOnOpenACCAtomicDirective(ComputedClauses, AStmt);
    break;
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

StmtResult Sema::ActOnOpenACCWaitDirective(ArrayRef<ACCClause *> Clauses) {
  DirStackTy &DirStack = OpenACCData->DirStack;
  SourceLocation StartLoc = DirStack.getDirectiveStartLoc();
  SourceLocation EndLoc = DirStack.getDirectiveEndLoc();
  return ACCWaitDirective::Create(Context, StartLoc, EndLoc, Clauses);
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
      Clauses, AStmt, DirStack.getNestedExplicitWorkerPartitioning());
}

void Sema::ActOnOpenACCLoopInitialization(SourceLocation ForLoc, Stmt *Init) {
  DirStackTy &DirStack = OpenACCData->DirStack;
  assert(getLangOpts().OpenACC && "OpenACC is not active.");
  assert(Init && "Expected loop in canonical form.");
  if (DirStack.getAssociatedLoopsParsed() < DirStack.getAssociatedLoops() &&
      isOpenACCLoopDirective(DirStack.getEffectiveDirective())) {
    if (Expr *E = dyn_cast<Expr>(Init))
      Init = E->IgnoreParens();
    if (BinaryOperator *BO = dyn_cast<BinaryOperator>(Init)) {
      if (BO->getOpcode() == BO_Assign)
        DirStack.addLoopControlVariable(BO->getLHS()->IgnoreParens());
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

Stmt *Sema::ActOnFinishFunctionBodyForOpenACC(FunctionDecl *FD, Stmt *Body) {
  if (OpenACCData->TransformingOpenACC)
    return Body;
  ImplicitRoutineDirInfoTy &ImplicitRoutineDirInfo =
      OpenACCData->ImplicitRoutineDirInfo;
  if (isLambdaCallOperator(FD)) {
    ImplicitRoutineDirInfo.fullyDetermineImplicitRoutineDir(FD);
    ACCRoutineDeclAttr *FnAttr = FD->getAttr<ACCRoutineDeclAttr>();
    ACCRoutineDeclAttr::PartitioningTy Part =
        FnAttr ? FnAttr->getPartitioning() : ACCRoutineDeclAttr::Seq;
    ImplicitGangWorkerVectorAdder(*this, Part).Visit(Body);
  }
  return transformACCToOMP(
      FD, Body, ImplicitRoutineDirInfo.hasOrphanedLoopConstructImplier(FD));
}

void Sema::ActOnFunctionUseForOpenACC(FunctionDecl *Usee,
                                      SourceLocation UseLoc) {
  if (OpenACCData->TransformingOpenACC || isa<CXXDeductionGuideDecl>(Usee))
    return;
  OpenACCData->FunctionUses[Usee->getCanonicalDecl()].push_back(UseLoc);
  DirStackTy &DirStack = OpenACCData->DirStack;
  ImplicitRoutineDirInfoTy &ImplicitRoutineDirInfo =
      OpenACCData->ImplicitRoutineDirInfo;

  // If Usee has no routine directive yet, then add an implicit routine seq
  // directive if it's an accelerator use, and record as host use otherwise.
  // Either way, seq is the highest level of parallelism Usee might eventually
  // have, so any level of parallelism at the use site is compatible.
  ACCRoutineDeclAttr *UseeAttr =
      Usee->getMostRecentDecl()->getAttr<ACCRoutineDeclAttr>();
  if (!UseeAttr) {
    OpenACCDirectiveKind ComputeDKind = DirStack.isInComputeRegion();
    FunctionDecl *CurFD = getCurFunctionDecl(/*AllowLambda=*/true);
    if (ComputeDKind != ACCD_unknown) {
      ImplicitRoutineDirInfo.addRoutineDirImplier(Usee, ACCRoutineDeclAttr::Seq,
                                                  UseLoc, CurFD, ComputeDKind);
      ImplicitRoutineDirInfo.fullyDetermineImplicitRoutineDir(Usee);
      return;
    }
    if (CurFD)
      ImplicitRoutineDirInfo.addHostFunctionUse(CurFD, Usee, UseLoc);
  }
}
void Sema::ActOnFunctionCallForOpenACC(CallExpr *CE) {
  if (FunctionDecl *Callee = CE->getDirectCallee())
    ActOnFunctionCallForOpenACC(Callee, CE->getExprLoc());
}
void Sema::ActOnFunctionCallForOpenACC(FunctionDecl *Callee,
                                       SourceLocation CallLoc) {
  assert(Callee && "expected Callee != nullptr");
  assert(!isa<CXXDeductionGuideDecl>(Callee) &&
         "unexpected call to C++ deduction guide");
  if (OpenACCData->TransformingOpenACC || isUnevaluatedContext())
    return;
  DirStackTy &DirStack = OpenACCData->DirStack;
  ImplicitRoutineDirInfoTy &ImplicitRoutineDirInfo =
      OpenACCData->ImplicitRoutineDirInfo;
  ACCRoutineDeclAttr *CalleeAttr =
      Callee->getMostRecentDecl()->getAttr<ACCRoutineDeclAttr>();
  if (!CalleeAttr)
    return;

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
          .getMinParLevelClause();
  if (LoopPart != ACCRoutineDeclAttr::Seq) {
    if (LoopPart <= CalleePart) {
      Diag(CallLoc, diag::err_acc_routine_loop_par_level)
          << getOpenACCName(LoopDirKind)
          << ACCRoutineDeclAttr::ConvertPartitioningTyToStr(LoopPart)
          << NameForDiag(*this, Callee)
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
  // construct.  If there is no Caller (because call is at file scope) or Caller
  // has no routine directive yet, complain if Callee has a higher level of
  // parallelism than seq.  Why?  First, if a routine directive is implied for
  // Caller later, then seq is the highest possible level.  Second, if a routine
  // directive is not implied for Caller later or there is no Caller, then
  // Caller can execute only outside compute regions, but Callee's higher level
  // of parallelism requires execution modes (gang-redundant, etc.) that are
  // impossible outside compute regions.
  FunctionDecl *Caller = getCurFunctionDecl(/*AllowLambda=*/true);
  if (!Caller) {
    if (CalleePart > ACCRoutineDeclAttr::Seq) {
      Diag(CallLoc, diag::err_acc_routine_func_par_level_at_file_scope)
          << NameForDiag(*this, Callee)
          << ACCRoutineDeclAttr::ConvertPartitioningTyToStr(CalleePart);
      ImplicitRoutineDirInfo.emitNotesForRoutineDirChain(Callee);
    }
    return;
  }
  ACCRoutineDeclAttr *CallerAttr = Caller->getAttr<ACCRoutineDeclAttr>();
  if (!CallerAttr) {
    if (CalleePart > ACCRoutineDeclAttr::Seq) {
      // If Caller is a lambda, we record the call as a routine directive
      // implier.  Otherwise, we report that Caller has no explicit routine
      // directive.  That's true, and an explicit routine directive is required
      // to enable the required level of parallelism.  However, if an implicit
      // routine seq has already been implied for Caller, we report the
      // mismatched level of parallelism at the next diagnostic even though this
      // diagnostic's wording would cover that case too.
      if (isLambdaCallOperator(Caller)) {
        ImplicitRoutineDirInfo.addRoutineDirImplier(
            Caller, CalleePart, CallLoc, Callee,
            /*ImplierConstruct=*/ACCD_unknown);
      } else {
        Diag(CallLoc, diag::err_acc_routine_func_par_level_vs_no_explicit)
            << NameForDiag(*this, Caller) << NameForDiag(*this, Callee)
            << ACCRoutineDeclAttr::ConvertPartitioningTyToStr(CalleePart);
        ImplicitRoutineDirInfo.emitNotesForRoutineDirChain(Callee);
      }
    }
    return;
  }

  // Callee and Caller have routine directives, and there's no enclosing loop or
  // compute construct.  Complain if Caller has a lower level of parallelism
  // than Callee.
  ACCRoutineDeclAttr::PartitioningTy CallerPart = CallerAttr->getPartitioning();
  if (CallerPart < CalleePart) {
    Diag(CallLoc, diag::err_acc_routine_func_par_level)
        << NameForDiag(*this, Caller)
        << ACCRoutineDeclAttr::ConvertPartitioningTyToStr(CallerPart)
        << NameForDiag(*this, Callee)
        << ACCRoutineDeclAttr::ConvertPartitioningTyToStr(CalleePart);
    ImplicitRoutineDirInfo.emitNotesForRoutineDirChain(Caller);
    ImplicitRoutineDirInfo.emitNotesForRoutineDirChain(Callee);
  }
}

void Sema::ActOnDeclStmtForOpenACC(DeclStmt *S) {
  if (OpenACCData->TransformingOpenACC)
    return;
  FunctionDecl *CurFn = getCurFunctionDecl(/*AllowLambda=*/true);
  for (Decl *D : S->decls()) {
    if (VarDecl *VD = dyn_cast<VarDecl>(D)) {
      // OpenACC 3.1, sec. 2.15.1 "Routine Directive", L2794-2795:
      // "In C and C++, function static variables are not supported in functions
      // to which a routine directive applies."
      if (VD->isStaticLocal()) {
        DiagIfRoutineDir(OpenACCData->ImplicitRoutineDirInfo, CurFn,
                         VD->getLocation(), diag::err_acc_routine_static_local)
            << NameForDiag(*this, ACCDataVar(VD)) << NameForDiag(*this, CurFn);
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

  // OpenACC 3.3 sec. 2.9.1 "collapse clause", L2025-2026:
  // "The collapse clause is used to specify how many nested loops are
  // associated with the loop construct."
  //
  // OpenACC 3.3 sec. 2.9.8 "tile clause", L2181-2182:
  // "If there are n tile sizes in the list, the loop construct must be
  // immediately followed by n tightly-nested loops."
  //
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
      auto TileClauses =
          ACCDirectiveStmt::getClausesOfKind<ACCTileClause>(Clauses);
      if (CollapseClauses.begin() != CollapseClauses.end()) {
        ACCCollapseClause *C = *CollapseClauses.begin();
        Expr *E = C->getCollapse();
        Diag(E->getExprLoc(), diag::note_acc_specified_in_clause)
            << getOpenACCName(C->getClauseKind()) << E->getSourceRange();
      } else if (TileClauses.begin() != TileClauses.end()) {
        ACCTileClause *C = *TileClauses.begin();
        Diag(C->getBeginLoc(), diag::note_acc_specified_in_clause)
            << getOpenACCName(C->getClauseKind()) << C->getSourceRange();
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
      DirStack.getNestedExplicitGangPartitioning(),
      DirStack.getNestedExplicitWorkerPartitioning(),
      DirStack.getNestedExplicitVectorPartitioning());

  // If this is an outermost orphaned loop construct and there's an explicit
  // routine directive, TransformACCToOMP is about to run on it, so run
  // ImplicitGangWorkerVectorAdder first.
  //
  // Normally, an orphaned loop construct must appear in a function with an
  // explicit routine directive, which must appear before the function
  // definition.  However, in the case of a lambda, the routine directive can be
  // implied within the lambda's body and isn't fully determined until the end
  // of the body.  Moreover, rules might be violated during error recovery.  For
  // the sake of the last two cases, skip ImplicitGangWorkerVectorAdder here if
  // there's no routine direcive.
  if (!isOpenACCDirectiveStmt(DirStack.getEffectiveParentDirective())) {
    FunctionDecl *CurFnDecl = getCurFunctionDecl(/*AllowLambda=*/true);
    if (CurFnDecl) {
      if (ACCRoutineDeclAttr *FnAttr = CurFnDecl->getAttr<ACCRoutineDeclAttr>())
        ImplicitGangWorkerVectorAdder(*this, FnAttr->getPartitioning())
            .Visit(Res);
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

struct AtomicDirNoteTy {
  unsigned DiagID;
  SourceLocation Loc;
  SourceRange Range0;
  SourceRange Range1;
  AtomicDirNoteTy(unsigned DiagID, SourceLocation Loc)
      : DiagID(DiagID), Loc(Loc) {}
  AtomicDirNoteTy(unsigned DiagID, const Stmt *S)
      : DiagID(DiagID), Loc(S->getBeginLoc()), Range0(S->getSourceRange()) {}
  AtomicDirNoteTy(unsigned DiagID, const BinaryOperator *Op)
      : DiagID(DiagID), Loc(Op->getOperatorLoc()),
        Range0(Op->getLHS()->getSourceRange()),
        Range1(Op->getRHS()->getSourceRange()) {}
};

// Check associated statement forms for atomic constructs.
//
// These functions don't check that Op_* are lvalues and scalar types as that is
// handled later.  These functions sometimes set Op_* arguments even if
// validation of the form fails.  That way, additional complaints about
// whether they're lvalues and scalar types can be produced later.
//
// Some of these functions are reused by the other functions, so the diagnostic
// wording must not be specific to a particular atomic construct clause.
static bool
isAtomicDirReadOrWriteForm(Sema &SemaRef, const Stmt *AStmt,
                           const Expr *&Op_lhs, const Expr *&Op_rhs,
                           SmallVectorImpl<AtomicDirNoteTy> *Notes = nullptr) {
  const Expr *AExpr = dyn_cast<Expr>(AStmt);
  if (!AExpr) {
    if (Notes)
      Notes->emplace_back(diag::note_acc_atomic_not_expr_stmt, AStmt);
    return false;
  }
  const BinaryOperator *BinOp =
      dyn_cast<BinaryOperator>(AExpr->IgnoreParenImpCasts());
  if (!BinOp) {
    if (Notes)
      Notes->emplace_back(diag::note_acc_atomic_not_simple_assign, AExpr);
    return false;
  }
  if (BinOp->getOpcode() != BO_Assign) {
    if (Notes)
      Notes->emplace_back(diag::note_acc_atomic_not_simple_assign, BinOp);
    return false;
  }
  Op_lhs = BinOp->getLHS()->IgnoreParenImpCasts();
  Op_rhs = BinOp->getRHS()->IgnoreParenImpCasts();
  return true;
}
static void checkAtomicDirUpdateForm(Sema &SemaRef, const Stmt *AStmt,
                                     const Expr *&Op_x, const Expr *&Op_expr,
                                     SmallVectorImpl<AtomicDirNoteTy> &Notes) {
  const Expr *AExpr = dyn_cast<Expr>(AStmt);
  if (!AExpr) {
    Notes.emplace_back(diag::note_acc_atomic_not_expr_stmt, AStmt);
    return;
  }

  // Check unary-operator form.
  if (const UnaryOperator *IncDecOp =
          dyn_cast<UnaryOperator>(AExpr->IgnoreParenImpCasts())) {
    if (!IncDecOp->isIncrementDecrementOp()) {
      Notes.emplace_back(diag::note_acc_atomic_not_supported_inc_dec_assign,
                         AExpr);
      return;
    }
    Op_x = IncDecOp->getSubExpr()->IgnoreParenImpCasts();
    return;
  }

  // Complain if not binary-operator form.
  const BinaryOperator *Assign =
      dyn_cast<BinaryOperator>(AExpr->IgnoreParenImpCasts());
  if (!Assign) {
    Notes.emplace_back(diag::note_acc_atomic_not_supported_inc_dec_assign,
                       AExpr);
    return;
  }

  // Check binary-operator form.
  switch (Assign->getOpcode()) {
  case BO_AddAssign:
  case BO_MulAssign:
  case BO_SubAssign:
  case BO_DivAssign:
  case BO_AndAssign:
  case BO_XorAssign:
  case BO_OrAssign:
  case BO_ShlAssign:
  case BO_ShrAssign:
    Op_x = Assign->getLHS()->IgnoreParenImpCasts();
    Op_expr = Assign->getRHS()->IgnoreParenImpCasts();
    return;
  case BO_Assign: {
    Op_x = Assign->getLHS()->IgnoreParenImpCasts();
    const BinaryOperator *InOp =
        dyn_cast<BinaryOperator>(Assign->getRHS()->IgnoreParenImpCasts());
    if (!InOp) {
      Notes.emplace_back(diag::note_acc_atomic_not_supported_binop_after_assign,
                         Assign->getRHS());
      return;
    }
    switch (InOp->getOpcode()) {
    case BO_Add:
    case BO_Mul:
    case BO_Sub:
    case BO_Div:
    case BO_And:
    case BO_Xor:
    case BO_Or:
    case BO_Shl:
    case BO_Shr: {
      const Expr *LHS = InOp->getLHS()->IgnoreParenImpCasts();
      const Expr *RHS = InOp->getRHS()->IgnoreParenImpCasts();
      llvm::FoldingSetNodeID ID_x, ID_LHS, ID_RHS;
      Op_x->Profile(ID_x, SemaRef.getASTContext(), /*Canonical=*/true);
      LHS->Profile(ID_LHS, SemaRef.getASTContext(), /*Canonical=*/true);
      RHS->Profile(ID_RHS, SemaRef.getASTContext(), /*Canonical=*/true);
      if (ID_x == ID_LHS)
        Op_expr = RHS;
      else if (ID_x == ID_RHS)
        Op_expr = LHS;
      else {
        Notes.emplace_back(diag::note_acc_atomic_unmatched_operand, InOp);
        Notes.emplace_back(diag::note_acc_atomic_other_expr, Op_x);
      }
      return;
    }
    default:
      Notes.emplace_back(diag::note_acc_atomic_not_supported_binop_after_assign,
                         InOp);
      return;
    }
  }
  default:
    Notes.emplace_back(diag::note_acc_atomic_not_supported_inc_dec_assign,
                       Assign);
    return;
  }
}
static void checkAtomicDirCaptureForm(Sema &SemaRef, const Stmt *AStmt,
                                      const Expr *&Op_v, const Expr *&Op_x,
                                      const Expr *&Op_expr,
                                      SmallVectorImpl<AtomicDirNoteTy> &Notes) {
  // Check expression-statement form.
  if (const Expr *AExpr = dyn_cast<Expr>(AStmt)) {
    const BinaryOperator *Assign =
        dyn_cast<BinaryOperator>(AExpr->IgnoreParenImpCasts());
    if (!Assign) {
      Notes.emplace_back(diag::note_acc_atomic_not_simple_assign, AExpr);
      return;
    }
    if (Assign->getOpcode() != BO_Assign) {
      Notes.emplace_back(diag::note_acc_atomic_not_simple_assign, Assign);
      return;
    }
    Op_v = Assign->getLHS()->IgnoreParenImpCasts();
    checkAtomicDirUpdateForm(SemaRef, Assign->getRHS(), Op_x, Op_expr, Notes);
    return;
  }

  // Complain if not compound-statement form.
  const CompoundStmt *CS = dyn_cast<CompoundStmt>(AStmt);
  if (!CS) {
    Notes.emplace_back(diag::note_acc_atomic_not_expr_stmt_or_compound_stmt,
                       AStmt);
    return;
  }

  // Complain if the compound statement's children aren't exactly two expression
  // statements.
  //
  // The later checks for read and write/update statements would also ensure
  // the children are expression statements.  However, the diagnostic here is
  // clearer in some cases.  For example, if a read statement is mistakenly
  // within a compound/if/etc. statement, complaining that the child is not an
  // expression statement is clearer than complaining that the read statement
  // is missing.
  for (int I = 0, E = CS->size(); I < E && I < 2; ++I) {
    const Stmt *Child = CS->body_begin()[I];
    if (!isa<Expr>(Child))
      Notes.emplace_back(diag::note_acc_atomic_not_expr_stmt, Child);
  }
  if (CS->size() != 2)
    Notes.emplace_back(diag::note_acc_atomic_not_two_stmts, CS);
  if (!Notes.empty())
    return;

  // Find the read statement.
  int ReadStmtIdx;
  for (ReadStmtIdx = 0; ReadStmtIdx < 2; ++ReadStmtIdx) {
    if (isAtomicDirReadOrWriteForm(SemaRef, CS->body_begin()[ReadStmtIdx], Op_v,
                                   Op_x) &&
        Op_v->isLValue() && Op_x->isLValue())
      break;
  }
  if (ReadStmtIdx == 2) {
    Op_v = Op_x = nullptr;
    Notes.emplace_back(diag::note_acc_atomic_no_read_stmt, AStmt);
    return;
  }

  // Check the write or update statement.
  const Expr *Op_x2 = nullptr;
  if (ReadStmtIdx == 0) {
    Stmt *WriteOrUpdateStmt = CS->body_begin()[1];
    if (!isAtomicDirReadOrWriteForm(SemaRef, WriteOrUpdateStmt, Op_x2, Op_expr))
      checkAtomicDirUpdateForm(SemaRef, WriteOrUpdateStmt, Op_x2, Op_expr,
                               Notes);
  } else {
    checkAtomicDirUpdateForm(SemaRef, CS->body_begin()[0], Op_x2, Op_expr,
                             Notes);
  }

  // Check that the x from the read statement matches the x from the
  // write/update statement.
  if (Op_x && Op_x2) {
    llvm::FoldingSetNodeID ID_x, ID_x2;
    Op_x->Profile(ID_x, SemaRef.getASTContext(), /*Canonical=*/true);
    Op_x2->Profile(ID_x2, SemaRef.getASTContext(), /*Canonical=*/true);
    if (ID_x != ID_x2) {
      Notes.emplace_back(diag::note_acc_atomic_unmatched_expr, Op_x2);
      Notes.emplace_back(diag::note_acc_atomic_other_expr, Op_x);
    }
  }
}
static void checkAtomicDirCompareForm(Sema &SemaRef, const Stmt *AStmt,
                                      const Expr *&Op_x, const Expr *&Op_expr,
                                      const Expr *&Op_e, const Expr *&Op_d,
                                      SmallVectorImpl<AtomicDirNoteTy> &Notes) {
  // Check expression-statement form.
  if (const Expr *AExpr = dyn_cast<Expr>(AStmt)) {
    // Check that it's an assign-operator expression.  If so, record the LHS as
    // 'x'.
    const BinaryOperator *Assign =
        dyn_cast<BinaryOperator>(AExpr->IgnoreParenImpCasts());
    if (!Assign) {
      Notes.emplace_back(diag::note_acc_atomic_not_simple_assign, AExpr);
      return;
    }
    if (Assign->getOpcode() != BO_Assign) {
      Notes.emplace_back(diag::note_acc_atomic_not_simple_assign, Assign);
      return;
    }
    Op_x = Assign->getLHS()->IgnoreParenImpCasts();

    // Check that assign operator RHS is conditional-operator expression.  Give
    // up if not.
    const ConditionalOperator *CondOp =
        dyn_cast<ConditionalOperator>(Assign->getRHS()->IgnoreParenImpCasts());
    if (!CondOp) {
      Notes.emplace_back(diag::note_acc_atomic_not_condop_after_assign,
                         Assign->getRHS());
      return;
    }

    // Check that x in conditional-operator expression's false expression is x
    // from assign operator's LHS.
    const Expr *CondFalse = CondOp->getFalseExpr()->IgnoreParenImpCasts();
    llvm::FoldingSetNodeID ID_x, ID_CondFalse;
    Op_x->Profile(ID_x, SemaRef.getASTContext(), /*Canonical=*/true);
    CondFalse->Profile(ID_CondFalse, SemaRef.getASTContext(),
                       /*Canonical=*/true);
    if (ID_x != ID_CondFalse) {
      Notes.emplace_back(diag::note_acc_atomic_unmatched_expr, CondFalse);
      Notes.emplace_back(diag::note_acc_atomic_other_expr, Op_x);
    }

    // Check that condition expression is a binary-operator expression.  Give
    // up if not.
    const BinaryOperator *CmpOp =
        dyn_cast<BinaryOperator>(CondOp->getCond()->IgnoreParenImpCasts());
    if (!CmpOp) {
      Notes.emplace_back(diag::note_acc_atomic_not_supported_cmpop,
                         CondOp->getCond());
      return;
    }

    // Check that condition expression's binary operator is a supported
    // comparison operator.  If so, gather operands ('x', 'expr', 'e', and/or
    // 'd') depending on the operator, and check that they match among the
    // assignment's LHS, the condition, and the true/false statements.
    switch (CmpOp->getOpcode()) {
    // Where ordop is '<' or '>', check these forms:
    // - x = expr ordop x ? expr : x;
    // - x = x ordop expr ? expr : x;
    case BO_LT:
    case BO_GT: {
      const Expr *CmpLHS = CmpOp->getLHS()->IgnoreParenImpCasts();
      const Expr *CmpRHS = CmpOp->getRHS()->IgnoreParenImpCasts();
      const Expr *CondTrue = CondOp->getTrueExpr()->IgnoreParenImpCasts();
      llvm::FoldingSetNodeID ID_CmpLHS, ID_CmpRHS;
      llvm::FoldingSetNodeID ID_CondTrue;
      CmpLHS->Profile(ID_CmpLHS, SemaRef.getASTContext(), /*Canonical=*/true);
      CmpRHS->Profile(ID_CmpRHS, SemaRef.getASTContext(), /*Canonical=*/true);
      CondTrue->Profile(ID_CondTrue, SemaRef.getASTContext(),
                        /*Canonical=*/true);
      if (ID_x == ID_CmpRHS)
        Op_expr = CmpLHS;
      else if (ID_x == ID_CmpLHS)
        Op_expr = CmpRHS;
      else {
        Notes.emplace_back(diag::note_acc_atomic_unmatched_operand, CmpOp);
        Notes.emplace_back(diag::note_acc_atomic_other_expr, Op_x);
      }
      if (Op_expr) {
        llvm::FoldingSetNodeID ID_expr;
        Op_expr->Profile(ID_expr, SemaRef.getASTContext(), /*Canonical=*/true);
        if (ID_expr != ID_CondTrue) {
          Notes.emplace_back(diag::note_acc_atomic_unmatched_expr, CondTrue);
          Notes.emplace_back(diag::note_acc_atomic_other_expr, Op_expr);
        }
      }
      break;
    }
    // Check this form:
    // - x = x == e ? d : x;
    case BO_EQ: {
      const Expr *CmpLHS = CmpOp->getLHS()->IgnoreParenImpCasts();
      Op_e = CmpOp->getRHS()->IgnoreParenImpCasts();
      Op_d = CondOp->getTrueExpr()->IgnoreParenImpCasts();
      llvm::FoldingSetNodeID ID_CmpLHS;
      CmpLHS->Profile(ID_CmpLHS, SemaRef.getASTContext(), /*Canonical=*/true);
      if (ID_x != ID_CmpLHS) {
        Notes.emplace_back(diag::note_acc_atomic_unmatched_expr, CmpLHS);
        Notes.emplace_back(diag::note_acc_atomic_other_expr, Op_x);
      }
      break;
    }
    default:
      Notes.emplace_back(diag::note_acc_atomic_not_supported_cmpop, CmpOp);
      break;
    }
    return;
  }

  // Check that associated statement is an 'if' statement.  Give up if not.
  const IfStmt *If = dyn_cast<IfStmt>(AStmt);
  if (!If) {
    Notes.emplace_back(diag::note_acc_atomic_not_expr_stmt_or_if_stmt, AStmt);
    return;
  }

  // Check that 'then' statement is a single simple assignment statement or a
  // compound statement enclosing one.  If so, record the 'then' statement's
  // 'x'.
  const Expr *ThenExpr = nullptr;
  if (const CompoundStmt *CS = dyn_cast<CompoundStmt>(If->getThen())) {
    if (CS->size() != 1)
      Notes.emplace_back(diag::note_acc_atomic_not_one_stmt, CS);
    else if (!(ThenExpr = dyn_cast<Expr>(CS->body_begin()[0])))
      Notes.emplace_back(diag::note_acc_atomic_not_expr_stmt,
                         CS->body_begin()[0]);
  } else if (!(ThenExpr = dyn_cast<Expr>(If->getThen()))) {
    Notes.emplace_back(diag::note_acc_atomic_not_expr_stmt_or_compound_stmt,
                       If->getThen());
  }
  const BinaryOperator *Assign = nullptr;
  if (ThenExpr) {
    Assign = dyn_cast<BinaryOperator>(ThenExpr->IgnoreParenImpCasts());
    if (!Assign)
      Notes.emplace_back(diag::note_acc_atomic_not_simple_assign, ThenExpr);
    else if (Assign->getOpcode() != BO_Assign)
      Notes.emplace_back(diag::note_acc_atomic_not_simple_assign, Assign);
    else
      Op_x = Assign->getLHS()->IgnoreParenImpCasts();
  }

  // Check that the condition is a binary-operator expression with a supported
  // comparison operator.  If so, gather operands ('x', 'expr', 'e', and/or 'd')
  // depending on the operator, and check that they match between the condition
  // and the 'then' statement.
  const BinaryOperator *CmpOp =
      dyn_cast<BinaryOperator>(If->getCond()->IgnoreParenImpCasts());
  if (!CmpOp) {
    Notes.emplace_back(diag::note_acc_atomic_not_supported_cmpop,
                       If->getCond());
  } else {
    switch (CmpOp->getOpcode()) {
    // Where ordop is '<' or '>', check these forms:
    // - if (expr ordop x) { x = expr; }
    // - if (expr ordop x) x = expr;
    // - if (x ordop expr) { x = expr; }
    // - if (x ordop expr) x = expr;
    case BO_LT:
    case BO_GT:
      if (Op_x) {
        Op_expr = Assign->getRHS()->IgnoreParenImpCasts();
        const Expr *CmpLHS = CmpOp->getLHS()->IgnoreParenImpCasts();
        const Expr *CmpRHS = CmpOp->getRHS()->IgnoreParenImpCasts();
        llvm::FoldingSetNodeID ID_x, ID_expr, ID_CmpLHS, ID_CmpRHS;
        Op_x->Profile(ID_x, SemaRef.getASTContext(), /*Canonical=*/true);
        Op_expr->Profile(ID_expr, SemaRef.getASTContext(), /*Canonical=*/true);
        CmpLHS->Profile(ID_CmpLHS, SemaRef.getASTContext(), /*Canonical=*/true);
        CmpRHS->Profile(ID_CmpRHS, SemaRef.getASTContext(), /*Canonical=*/true);
        if (ID_x == ID_CmpRHS) {
          if (ID_expr != ID_CmpLHS) {
            Notes.emplace_back(diag::note_acc_atomic_unmatched_expr, CmpLHS);
            Notes.emplace_back(diag::note_acc_atomic_other_expr, Op_expr);
          }
        } else if (ID_x == ID_CmpLHS) {
          if (ID_expr != ID_CmpRHS) {
            Notes.emplace_back(diag::note_acc_atomic_unmatched_expr, CmpRHS);
            Notes.emplace_back(diag::note_acc_atomic_other_expr, Op_expr);
          }
        } else {
          Notes.emplace_back(diag::note_acc_atomic_unmatched_operand, CmpOp);
          Notes.emplace_back(diag::note_acc_atomic_other_expr, Op_x);
        }
      }
      break;
    // Check these forms:
    // - if (x == e) { x = d; }
    // - if (x == e) x = d;
    case BO_EQ:
      Op_e = CmpOp->getRHS()->IgnoreParenImpCasts();
      if (Op_x) {
        Op_d = Assign->getRHS()->IgnoreParenImpCasts();
        const Expr *CmpLHS = CmpOp->getLHS()->IgnoreParenImpCasts();
        llvm::FoldingSetNodeID ID_x, ID_CmpLHS;
        Op_x->Profile(ID_x, SemaRef.getASTContext(), /*Canonical=*/true);
        CmpLHS->Profile(ID_CmpLHS, SemaRef.getASTContext(), /*Canonical=*/true);
        if (ID_x != ID_CmpLHS) {
          Notes.emplace_back(diag::note_acc_atomic_unmatched_expr, CmpLHS);
          Notes.emplace_back(diag::note_acc_atomic_other_expr, Op_x);
        }
      }
      break;
    default:
      Notes.emplace_back(diag::note_acc_atomic_not_supported_cmpop, CmpOp);
      break;
    }
  }

  // Complain about any 'else' statement.
  if (If->getElse())
    Notes.emplace_back(diag::note_acc_atomic_unexpected_else, If->getElseLoc());
}

StmtResult Sema::ActOnOpenACCAtomicDirective(ArrayRef<ACCClause *> Clauses,
                                             Stmt *AStmt) {
  if (!AStmt)
    return StmtError();
  DirStackTy &DirStack = OpenACCData->DirStack;
  assert(Clauses.size() == 1 && "expected one clause for acc atomic");
  OpenACCClauseKind ClauseKind = Clauses.back()->getClauseKind();

  // Validate associated statement's form and gather operands.
  SmallVector<AtomicDirNoteTy, 4> Notes;
  const Expr *Op_v = nullptr;
  const Expr *Op_x = nullptr;
  const Expr *Op_expr = nullptr;
  const Expr *Op_e = nullptr;
  const Expr *Op_d = nullptr;
  switch (ClauseKind) {
  case ACCC_read:
    isAtomicDirReadOrWriteForm(*this, AStmt, Op_v, Op_x, &Notes);
    break;
  case ACCC_write:
    isAtomicDirReadOrWriteForm(*this, AStmt, Op_x, Op_expr, &Notes);
    break;
  case ACCC_update:
    checkAtomicDirUpdateForm(*this, AStmt, Op_x, Op_expr, Notes);
    break;
  case ACCC_capture:
    checkAtomicDirCaptureForm(*this, AStmt, Op_v, Op_x, Op_expr, Notes);
    break;
  case ACCC_compare:
    checkAtomicDirCompareForm(*this, AStmt, Op_x, Op_expr, Op_e, Op_d, Notes);
    break;
  default:
    llvm_unreachable("unexpected acc atomic clause");
  }

  // Validate associated statement's operands.
  if (Op_v) {
    if (!Op_v->isLValue())
      Notes.emplace_back(diag::note_acc_atomic_not_lvalue, Op_v);
    if (!Op_v->getType()->isScalarType())
      Notes.emplace_back(diag::note_acc_atomic_not_scalar, Op_v);
  }
  if (Op_x) {
    if (!Op_x->isLValue())
      Notes.emplace_back(diag::note_acc_atomic_not_lvalue, Op_x);
    if (!Op_x->getType()->isScalarType())
      Notes.emplace_back(diag::note_acc_atomic_not_scalar, Op_x);
  }
  if (Op_expr && !Op_expr->getType()->isScalarType())
    Notes.emplace_back(diag::note_acc_atomic_not_scalar, Op_expr);
  if (Op_e && !Op_e->getType()->isScalarType())
    Notes.emplace_back(diag::note_acc_atomic_not_scalar, Op_e);
  if (Op_d && !Op_d->getType()->isScalarType())
    Notes.emplace_back(diag::note_acc_atomic_not_scalar, Op_d);

  // Emit any diagnostics about the associated statement.
  if (!Notes.empty()) {
    Diag(AStmt->getBeginLoc(), diag::err_acc_atomic_invalid_stmt)
        << getOpenACCName(ClauseKind) << AStmt->getSourceRange();
    for (const AtomicDirNoteTy &Note : Notes) {
      SemaDiagnosticBuilder DiagBuilder = Diag(Note.Loc, Note.DiagID);
      if (Note.Range0.isValid())
        DiagBuilder << Note.Range0;
      if (Note.Range1.isValid())
        DiagBuilder << Note.Range1;
    }
    return StmtError();
  }

  return ACCAtomicDirective::Create(Context, DirStack.getDirectiveStartLoc(),
                                    DirStack.getDirectiveEndLoc(), Clauses,
                                    AStmt);
}

void Sema::ActOnOpenACCRoutineDirective(OpenACCDetermination Determination,
                                        DeclGroupRef TheDeclGroup) {
  DirStackTy &DirStack = OpenACCData->DirStack;
  // ActOnStartOfFunctionDefForOpenACC calls ActOnOpenACCRoutineDirective in the
  // case of a function definition, but ParseOpenACCDeclarativeDirective doesn't
  // know and calls it again, so skip if this is the second time.
  if (DirStack.isExplicitRoutineDirectiveActedOn())
    return;
  ActOnOpenACCRoutineDirective(DirStack.getClauses(), Determination,
                               DirStack.getDirectiveStartLoc(),
                               DirStack.getDirectiveEndLoc(), TheDeclGroup);
  DirStack.setExplicitRoutineDirectiveActedOn();
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
  if (!TheDecl || !TheDecl->isFunctionOrFunctionTemplate() ||
      isa<CXXDeductionGuideDecl>(TheDecl->getAsFunction())) {
    // !TheDecl includes the case that the routine directive is at the end of
    // the enclosing context (e.g., the file, or a type definition).  It also
    // includes the case that the routine directive precedes a type declaration.
    Diag(StartLoc, diag::err_acc_expected_function_after_directive)
        << getOpenACCName(ACCD_routine);
    return;
  }
  FunctionDecl *FD = TheDecl->getAsFunction();
  ACCRoutineDeclAttr *ACCAttr =
      FD->getMostRecentDecl()->getAttr<ACCRoutineDeclAttr>();

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
    if (FunctionDecl *Def = FD->getDefinition()) {
      if (Def != FD) {
        Diag(StartLoc, diag::err_acc_routine_not_in_scope_at_function_def)
            << NameForDiag(*this, FD);
        Diag(Def->getBeginLoc(), diag::note_acc_routine_definition)
            << NameForDiag(*this, FD);
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
          << NameForDiag(*this, FD);
      const SmallVector<SourceLocation> &Uses =
          OpenACCData->FunctionUses[FD->getCanonicalDecl()];
      assert(!Uses.empty() &&
             "expected a use of FD to be recorded if FD->isUsed()");
      for (SourceLocation UseLoc : Uses)
        Diag(UseLoc, diag::note_acc_routine_use) << NameForDiag(*this, FD);
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
          << NameForDiag(*this, FD)
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
    // Retranslate because, in different contexts, the translation might differ
    // (e.g., the directive might simply be dropped).
    if (InheritableAttr *OMPNode = ACCAttr->unsetOMPNode(FD)) {
      AttrVec &Attrs = FD->getAttrs();
      for (auto I = Attrs.begin(), E = Attrs.end(); I != E; ++I) {
        if (*I == OMPNode) {
          FD->getAttrs().erase(I);
          break;
        }
      }
    }
    transformACCToOMP(ACCAttr, FD);
  }
}

ACCClause *Sema::ActOnOpenACCSingleExprClause(OpenACCClauseKind Kind,
                                              Expr *Expr,
                                              SourceLocation StartLoc,
                                              SourceLocation LParenLoc,
                                              SourceLocation EndLoc,
                                              NamedDecl *Async2Dep) {
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
  case ACCC_async:
    Res = ActOnOpenACCAsyncClause(Expr, Async2Dep, StartLoc, LParenLoc, EndLoc);
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
  case ACCC_tile:
  case ACCC_wait:
  case ACCC_read:
  case ACCC_write:
  case ACCC_update:
  case ACCC_capture:
  case ACCC_compare:
  case ACCC_unknown:
    llvm_unreachable("expected clause that takes a single expression");
  }
  return Res;
}

static ACCDataVar getVarFromDMAVarList(Sema &S, DirStackTy &DirStack,
                                       OpenACCClauseKind CKind, Expr *RefExpr) {
  return ACCDataVar(RefExpr, ACCDataVar::AllowMemberExprOnAny,
                    /*AllowSubarray=*/true, &S, /*Quiet=*/false,
                    DirStack.getRealDirective(), CKind);
}

static ACCDataVar getVarFromDSAVarList(Sema &S, DirStackTy &DirStack,
                                       OpenACCClauseKind CKind, Expr *RefExpr) {
  // See the section "Basic Data Attributes" in the Clang OpenACC design
  // document for a discussion of why we restrict member expressions and
  // subarrays for explicit DSAs.
  //
  // Except for some cases of implicit 'shared', which uses
  // 'getVarFromDMAVarList' instead, an implicit DSA is never computed for a
  // member expression or subarray.  (If such an expression is used in a
  // construct, an implicit DSA might be computed for the base of the expression
  // instead.)
  ACCDataVar::AllowMemberExprTy AllowMemberExpr =
      isOpenACCLoopDirective(DirStack.getEffectiveDirective()) &&
              CKind == ACCC_private
          ? ACCDataVar::AllowMemberExprNone
          : ACCDataVar::AllowMemberExprOnCXXThis;
  return ACCDataVar(RefExpr, AllowMemberExpr, /*AllowSubarray=*/false, &S,
                    /*Quiet=*/false, DirStack.getRealDirective(), CKind);
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
  for (Expr *RefExpr : VarList) {
    assert(RefExpr && "unexpected nullptr in OpenACC implicit nomap clause");
    ACCDataVar Var(RefExpr);
    if (!OpenACCData->DirStack.addDMA(Var, RefExpr->IgnoreParens(),
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
  DirStackTy &DirStack = OpenACCData->DirStack;
  SmallVector<Expr *, 8> Vars;
  for (Expr *RefExpr : VarList) {
    assert(RefExpr && "unexpected nullptr in OpenACC present clause");
    ACCDataVar Var =
        getVarFromDMAVarList(*this, DirStack, ACCC_present, RefExpr);
    if (!Var.isValid())
      continue;

    QualType Type = Var.getType();

    // The OpenACC 3.0 spec doesn't say, as far as I know, that a variable
    // in a present clause must have a complete type.  However, without its
    // size, we cannot know if it's fully present.  Besides, it translates to
    // an OpenMP map clause, which does require a complete type.
    if (RequireCompleteTypeACC(*this, Type, Determination, ACCC_present,
                               DirStack.getDirectiveStartLoc(),
                               RefExpr->getExprLoc()))
      continue;

    if (!DirStack.addDMA(Var, RefExpr->IgnoreParens(), ACC_DMA_present,
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
  assert(ACCCopyClause::isClauseKind(Kind) && "expected copy clause or alias");
  DirStackTy &DirStack = OpenACCData->DirStack;
  SmallVector<Expr *, 8> Vars;
  for (Expr *RefExpr : VarList) {
    assert(RefExpr && "unexpected nullptr in OpenACC copy clause");
    ACCDataVar Var = getVarFromDMAVarList(*this, DirStack, Kind, RefExpr);
    if (!Var.isValid())
      continue;

    QualType Type = Var.getType();

    // The OpenACC 2.7 spec doesn't say, as far as I know, that a variable
    // in a copy clause must have a complete type.  However, you cannot copy
    // data if it doesn't have a size, and the OpenMP implementation does have
    // this restriction for map clauses.
    if (RequireCompleteTypeACC(*this, Type, Determination, Kind,
                               DirStack.getDirectiveStartLoc(),
                               RefExpr->getExprLoc()))
      continue;

    if (!DirStack.addDMA(Var, RefExpr->IgnoreParens(), ACC_DMA_copy,
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
  DirStackTy &DirStack = OpenACCData->DirStack;
  SmallVector<Expr *, 8> Vars;
  for (Expr *RefExpr : VarList) {
    assert(RefExpr && "unexpected nullptr in OpenACC copyin clause");
    ACCDataVar Var = getVarFromDMAVarList(*this, DirStack, Kind, RefExpr);
    if (!Var.isValid())
      continue;

    QualType Type = Var.getType();

    // The OpenACC 2.7 spec doesn't say, as far as I know, that a variable
    // in a copyin clause must have a complete type.  However, you cannot copy
    // data if it doesn't have a size, and the OpenMP implementation does have
    // this restriction for map clauses.
    if (RequireCompleteTypeACC(*this, Type, ACC_EXPLICIT, Kind,
                               DirStack.getDirectiveStartLoc(),
                               RefExpr->getExprLoc()))
      continue;

    if (!DirStack.addDMA(Var, RefExpr->IgnoreParens(), ACC_DMA_copyin,
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
  DirStackTy &DirStack = OpenACCData->DirStack;
  SmallVector<Expr *, 8> Vars;
  for (Expr *RefExpr : VarList) {
    assert(RefExpr && "unexpected nullptr in OpenACC copyout clause");
    ACCDataVar Var = getVarFromDMAVarList(*this, DirStack, Kind, RefExpr);
    if (!Var.isValid())
      continue;

    // The OpenACC 2.7 spec doesn't say, as far as I know, that a variable
    // in a copyout clause must have a complete type.  However, you cannot copy
    // data if it doesn't have a size, and the OpenMP implementation does have
    // this restriction for map clauses.
    if (RequireCompleteTypeACC(*this, Var.getType(), ACC_EXPLICIT, Kind,
                               OpenACCData->DirStack.getDirectiveStartLoc(),
                               RefExpr->getExprLoc()))
      continue;

    // The OpenACC 3.0 spec doesn't say, as far as I know, that a const
    // variable cannot be copyout.  However, you can never initialize the
    // device copy of such a variable (except that, in the case of acc exit
    // data, another directive might have initialized it), and the value has to
    // be written back to the host.
    if (Var.diagnoseConst(*this, diag::err_acc_const_var_write, RefExpr, Kind))
      continue;

    if (!DirStack.addDMA(Var, RefExpr->IgnoreParens(), ACC_DMA_copyout,
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
  DirStackTy &DirStack = OpenACCData->DirStack;
  SmallVector<Expr *, 8> Vars;
  for (Expr *RefExpr : VarList) {
    assert(RefExpr && "unexpected nullptr in OpenACC create clause");
    ACCDataVar Var = getVarFromDMAVarList(*this, DirStack, Kind, RefExpr);
    if (!Var.isValid())
      continue;

    QualType Type = Var.getType();

    // The OpenACC 2.7 spec doesn't say, as far as I know, that a variable in a
    // create clause must have a complete type.  However, you cannot allocate
    // data if it doesn't have a size, and the OpenMP implementation does have
    // this restriction for map clauses.
    if (RequireCompleteTypeACC(*this, Type, ACC_EXPLICIT, Kind,
                               DirStack.getDirectiveStartLoc(),
                               RefExpr->getExprLoc()))
      continue;

    // The OpenACC 3.0 spec doesn't say, as far as I know, that a const variable
    // cannot be create.  However, you can never initialize the device copy of
    // such a variable.
    if (Var.diagnoseConst(*this, diag::err_acc_const_var_init, RefExpr, Kind))
      continue;

    if (!DirStack.addDMA(Var, RefExpr->IgnoreParens(), ACC_DMA_create,
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
  DirStackTy &DirStack = OpenACCData->DirStack;
  SmallVector<Expr *, 8> Vars;
  for (Expr *RefExpr : VarList) {
    assert(RefExpr && "unexpected nullptr in OpenACC no_create clause");
    ACCDataVar Var =
        getVarFromDMAVarList(*this, DirStack, ACCC_no_create, RefExpr);
    if (!Var.isValid())
      continue;

    QualType Type = Var.getType();

    // The OpenACC 3.0 spec doesn't say, as far as I know, that a variable
    // in a no_create clause must have a complete type.  However, without its
    // size, we cannot know if it's fully present.  Besides, it translates to
    // an OpenMP map clause, which does require a complete type.
    if (RequireCompleteTypeACC(*this, Type, ACC_EXPLICIT, ACCC_no_create,
                               OpenACCData->DirStack.getDirectiveStartLoc(),
                               RefExpr->getExprLoc()))
      continue;

    if (!DirStack.addDMA(Var, RefExpr->IgnoreParens(), ACC_DMA_no_create,
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
  DirStackTy &DirStack = OpenACCData->DirStack;
  SmallVector<Expr *, 8> Vars;
  for (Expr *RefExpr : VarList) {
    assert(RefExpr && "unexpected nullptr in OpenACC delete clause");
    ACCDataVar Var =
        getVarFromDMAVarList(*this, DirStack, ACCC_delete, RefExpr);
    if (!Var.isValid())
      continue;

    QualType Type = Var.getType();

    // The OpenACC 3.0 spec doesn't say, as far as I know, that a variable
    // in a delete clause must have a complete type.  However, without its
    // size, we cannot know if it's fully present.  Besides, it translates to
    // an OpenMP map clause, which does require a complete type.
    if (RequireCompleteTypeACC(*this, Type, ACC_EXPLICIT, ACCC_delete,
                               DirStack.getDirectiveStartLoc(),
                               RefExpr->getExprLoc()))
      continue;

    if (!DirStack.addDMA(Var, RefExpr->IgnoreParens(), ACC_DMA_delete,
                         ACC_EXPLICIT))
      Vars.push_back(RefExpr->IgnoreParens());
  }

  if (Vars.empty())
    return nullptr;

  return ACCDeleteClause::Create(Context, StartLoc, LParenLoc, EndLoc, Vars);
}

ACCClause *Sema::ActOnOpenACCSharedClause(ArrayRef<Expr *> VarList) {
  DirStackTy &DirStack = OpenACCData->DirStack;
  SmallVector<Expr *, 8> Vars;
  for (Expr *RefExpr : VarList) {
    assert(RefExpr && "unexpected nullptr in OpenACC implicit shared clause");
    // On compute constructs, the implicit shared clause is usually paired with
    // DMAs, which permit member expressions and subarrays.  On compute and loop
    // constructs, it can be implicit for 'this[0:1]'.  Thus, we make it as
    // flexible as a DMA.
    ACCDataVar Var =
        getVarFromDMAVarList(*this, DirStack, ACCC_shared, RefExpr);
    assert(Var.isValid() && "expected valid Expr for implicit shared clause");
    if (!DirStack.addDSA(Var, RefExpr->IgnoreParens(), ACC_DSA_shared,
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
  DirStackTy &DirStack = OpenACCData->DirStack;
  SmallVector<Expr *, 8> Vars;
  for (Expr *RefExpr : VarList) {
    assert(RefExpr && "unexpected nullptr in OpenACC private clause");
    ACCDataVar Var =
        getVarFromDSAVarList(*this, DirStack, ACCC_private, RefExpr);
    if (!Var.isValid())
      continue;

    QualType Type = Var.getType();

    // The OpenACC 2.5 spec doesn't say, as far as I know, that a private
    // variable must have a complete type.  However, you cannot copy data if it
    // doesn't have a size, and OpenMP does have this restriction.
    if (RequireCompleteTypeACC(*this, Type, Determination, ACCC_private,
                               DirStack.getDirectiveStartLoc(),
                               RefExpr->getExprLoc()))
      continue;

    // The OpenACC 2.5 spec doesn't say, as far as I know, that a const
    // variable cannot be private.  However, you can never initialize the
    // private version of such a variable, and OpenMP does have this
    // restriction.
    if (Var.diagnoseConst(*this, diag::err_acc_const_var_init, RefExpr,
                          ACCC_private)) {
      // Implicit private is for loop control variables, which cannot be const.
      assert(Determination == ACC_EXPLICIT &&
             "unexpected const type for computed private");
      continue;
    }

    if (!DirStack.addDSA(Var, RefExpr->IgnoreParens(), ACC_DSA_private,
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
  DirStackTy &DirStack = OpenACCData->DirStack;
  SmallVector<Expr *, 8> Vars;
  for (Expr *RefExpr : VarList) {
    assert(RefExpr && "unexpected nullptr in OpenACC firstprivate clause");
    ACCDataVar Var =
        getVarFromDSAVarList(*this, DirStack, ACCC_firstprivate, RefExpr);
    if (!Var.isValid())
      continue;

    QualType Type = Var.getType();

    // The OpenACC 2.5 spec doesn't say, as far as I know, that a private
    // variable must have a complete type.  However, you cannot copy data if it
    // doesn't have a size, and OpenMP does have this restriction.
    if (RequireCompleteTypeACC(*this, Type, Determination, ACCC_firstprivate,
                               DirStack.getDirectiveStartLoc(),
                               RefExpr->getExprLoc()))
      continue;

    if (!DirStack.addDSA(Var, RefExpr->IgnoreParens(), ACC_DSA_firstprivate,
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

  for (Expr *RefExpr : VarList) {
    assert(RefExpr && "unexpected nullptr in OpenACC reduction clause");
    SourceLocation ELoc = RefExpr->getExprLoc();
    ACCDataVar Var =
        getVarFromDSAVarList(*this, DirStack, ACCC_reduction, RefExpr);
    if (!Var.isValid())
      continue;

    QualType Type = Var.getType().getNonReferenceType();

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
    if (Var.diagnoseConst(*this, diag::err_acc_const_var_write, RefExpr,
                          ACCC_reduction)) {
      // Implicit reductions are copied from explicit reductions, which are
      // validated already.
      assert(Determination == ACC_EXPLICIT &&
             "unexpected const type for computed reduction");
      continue;
    }

    // Is the reduction variable type reasonable for the reduction operator?
    //
    // Skip this check if the variable's type is dependent.  The type might
    // always turn out to be correct.
    //
    // TODO: When adding support for arrays, this should be on the base element
    // type of the array.  See the OpenMP implementation for an example.
    if (!Type->isDependentType()) {
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
        if (Var.hasReferencedDecl())
          Diag(Var.getReferencedDecl()->getLocation(),
               diag::note_acc_var_declared)
              << NameForDiag(*this, Var);
        continue;
      }
    }

    // OpenACC 3.2, sec. 2.9.11 "reduction clause", L2114:
    // "Every var in a reduction clause appearing on an orphaned loop construct
    // must be private."
    // The getVarFromDSAVarList call above rejects member expressions in
    // reduction clauses except on C++ 'this', but we cannot generally determine
    // whether 'this' points to local storage, so we assume it doesn't to be
    // safe.
    assert((!Var.isMember() || Var.isMemberOnCXXThis()) &&
           "expected reduction clause not to accept non-this member "
           "expression");
    bool HasLocalStorage =
        !Var.isMemberOnCXXThis() && Var.hasReferencedDecl() &&
        cast<VarDecl>(Var.getReferencedDecl())->hasLocalStorage();
    if (DirStack.getEffectiveDirective() == ACCD_loop &&
        !isOpenACCComputeDirective(DirStack.isInComputeRegion()) &&
        !DirStack.hasPrivatizingDSAOnAncestorOrphanedLoop(Var) &&
        !HasLocalStorage)
      Diag(ELoc, diag::err_acc_orphaned_loop_reduction_var_not_gang_private)
          << NameForDiag(*this, Var);

    // Record reduction item.
    if (!DirStack.addDSA(Var, RefExpr->IgnoreParens(), ACC_DSA_reduction,
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
  DirStackTy &DirStack = OpenACCData->DirStack;
  SmallVector<Expr *, 8> Vars;
  for (Expr *RefExpr : VarList) {
    assert(RefExpr && "unexpected nullptr in OpenACC self clause");
    ACCDataVar Var = getVarFromDMAVarList(*this, DirStack, Kind, RefExpr);
    if (!Var.isValid())
      continue;

    QualType Type = Var.getType();

    // The OpenACC 2.7 spec doesn't say, as far as I know, that a variable
    // in a self clause must have a complete type.  However, it could not
    // have been allocated on the device copy if it didn't have a size.
    if (RequireCompleteTypeACC(*this, Type, ACC_EXPLICIT, Kind,
                               DirStack.getDirectiveStartLoc(),
                               RefExpr->getExprLoc()))
      continue;

    // The OpenACC 3.0 spec doesn't say, as far as I know, that a const
    // variable cannot be in a self clause, but updating it seems to be a
    // violation of const.
    if (Var.diagnoseConst(*this, diag::err_acc_const_var_write, RefExpr, Kind))
      continue;

    if (!DirStack.addUpdateVar(Var, RefExpr->IgnoreParens()))
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
  DirStackTy &DirStack = OpenACCData->DirStack;
  SmallVector<Expr *, 8> Vars;
  for (Expr *RefExpr : VarList) {
    assert(RefExpr && "unexpected nullptr in OpenACC device clause");
    ACCDataVar Var =
        getVarFromDMAVarList(*this, DirStack, ACCC_device, RefExpr);
    if (!Var.isValid())
      continue;

    QualType Type = Var.getType();

    // The OpenACC 2.7 spec doesn't say, as far as I know, that a variable
    // in a device clause must have a complete type.  However, it could not
    // have been allocated on the device copy if it didn't have a size.
    if (RequireCompleteTypeACC(*this, Type, ACC_EXPLICIT, ACCC_device,
                               DirStack.getDirectiveStartLoc(),
                               RefExpr->getExprLoc()))
      continue;

    // The OpenACC 3.0 spec doesn't say, as far as I know, that a const
    // variable cannot be in a device clause, but updating it seems to be a
    // violation of const.
    if (Var.diagnoseConst(*this, diag::err_acc_const_var_write, RefExpr,
                          ACCC_device))
      continue;

    if (!DirStack.addUpdateVar(Var, RefExpr->IgnoreParens()))
      Vars.push_back(RefExpr->IgnoreParens());
  }

  if (Vars.empty())
    return nullptr;

  return ACCDeviceClause::Create(Context, StartLoc, LParenLoc, EndLoc, Vars);
}

enum PosIntResult {
  PosIntConst,
  PosIntNonConst,
  PosIntError,
};

static PosIntResult IsPositiveIntegerValue(Expr *&ValExpr, Sema &SemaRef,
                                           OpenACCClauseKind CKind,
                                           std::optional<StringRef> ArgName,
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
  std::optional<llvm::APSInt> Result =
      ValExpr->getIntegerConstantExpr(SemaRef.Context);
  if (!Result) {
    if (ErrorIfNotConst) {
      SemaRef.Diag(Loc, diag::err_acc_clause_not_ice)
          << getOpenACCName(CKind) << ArgName.has_value()
          << ArgName.value_or("") << ValExpr->getSourceRange();
      return PosIntError;
    }
    return PosIntNonConst;
  }
  if (!Result->isStrictlyPositive()) {
    SemaRef.Diag(Loc, diag::err_acc_clause_not_positive_ice)
        << getOpenACCName(CKind) << ArgName.has_value() << ArgName.value_or("")
        << ValExpr->getSourceRange();
    return PosIntError;
  }

  return PosIntConst;
}

static ACCAsyncClause::AsyncArgStatus checkAsyncArg(
    Expr *&AsyncArg, Sema &SemaRef, OpenACCClauseKind CKind) {
  // If there's no async-arg, that's the same as acc_async_noval below.
  if (!AsyncArg)
    return ACCAsyncClause::AsyncArgIsUnknown;
  SourceLocation Loc = AsyncArg->getExprLoc();

  // This uses err_omp_* diagnostics, but none currently mention OpenMP or
  // OpenMP-specific constructs, so they should work fine for OpenACC.
  ExprResult Value =
      SemaRef.PerformOpenMPImplicitIntegerConversion(Loc, AsyncArg);
  if (Value.isInvalid())
    return ACCAsyncClause::AsyncArgIsError;

  // AsyncArg->getIntegerConstantExpr() fails if AsyncArg->isValueDependent(),
  // which might be true after an error with typo correction.
  if (AsyncArg->isValueDependent())
    return ACCAsyncClause::AsyncArgIsError;

  // If it's non-negative, it's for an asynchronous activity queue.
  if (AsyncArg->getType()->isUnsignedIntegerType())
    return ACCAsyncClause::AsyncArgIsAsync;

  // It's signed.  If it's not a constant expression or we cannot represent the
  // value, then we don't know whether it's for a synchronous or asynchronous
  // activity queue.
  AsyncArg = Value.get();
  std::optional<llvm::APSInt> ICE =
      AsyncArg->getIntegerConstantExpr(SemaRef.Context);
  if (!ICE)
    return ACCAsyncClause::AsyncArgIsUnknown;
  std::optional<int64_t> SignedVal = ICE.value().tryExtValue();
  if (!SignedVal)
    return ACCAsyncClause::AsyncArgIsUnknown;

  // If it's non-negative, it's for an asynchronous activity queue.
  if (0 <= SignedVal.value())
    return ACCAsyncClause::AsyncArgIsAsync;

  // Check known negative values.
  switch (SignedVal.value()) {
    case ACCAsyncClause::Acc2ompAccAsyncSync:
      return ACCAsyncClause::AsyncArgIsSync;
    case ACCAsyncClause::Acc2ompAccAsyncNoval:
      // Could be changed by acc_set_default_async.
      return ACCAsyncClause::AsyncArgIsUnknown;
    case ACCAsyncClause::Acc2ompAccAsyncDefault:
      // libacc2omp uses 0 for this case.
      return ACCAsyncClause::AsyncArgIsAsync;
  }

  // Complain for unknown negative values.
  SemaRef.Diag(Loc, diag::err_acc_clause_not_async_arg)
      << getOpenACCName(CKind) << AsyncArg->getSourceRange();
  return ACCAsyncClause::AsyncArgIsError;
}

ACCClause *Sema::ActOnOpenACCClause(OpenACCClauseKind Kind,
                                    SourceLocation StartLoc,
                                    SourceLocation EndLoc,
                                    NamedDecl *Async2Dep) {
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
    Res = ActOnOpenACCWorkerClause(ACC_EXPLICIT, StartLoc, EndLoc);
    break;
  case ACCC_vector:
    Res = ActOnOpenACCVectorClause(ACC_EXPLICIT, StartLoc, EndLoc);
    break;
  case ACCC_async:
    Res = ActOnOpenACCAsyncClause(/*AsyncArg=*/nullptr, Async2Dep, StartLoc,
                                  /*LParenLoc=*/SourceLocation(), EndLoc);
    break;
  case ACCC_wait:
    Res = ActOnOpenACCWaitClause(std::nullopt, StartLoc,
                                 /*LParenLoc=*/SourceLocation(), EndLoc);
    break;
  case ACCC_read:
    Res = ActOnOpenACCReadClause(StartLoc, EndLoc);
    break;
  case ACCC_write:
    Res = ActOnOpenACCWriteClause(StartLoc, EndLoc);
    break;
  case ACCC_update:
    Res = ActOnOpenACCUpdateClause(ACC_EXPLICIT, StartLoc, EndLoc);
    break;
  case ACCC_capture:
    Res = ActOnOpenACCCaptureClause(StartLoc, EndLoc);
    break;
  case ACCC_compare:
    Res = ActOnOpenACCCompareClause(StartLoc, EndLoc);
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
  case ACCC_tile:
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

ACCClause *Sema::ActOnOpenACCGangClause(SourceLocation StartLoc,
                                        SourceLocation LParenLoc,
                                        SourceLocation StaticKwLoc,
                                        SourceLocation StaticColonLoc,
                                        Expr *StaticArg,
                                        SourceLocation EndLoc) {
  if (!isa<ACCStarExpr>(StaticArg) &&
      PosIntError == IsPositiveIntegerValue(StaticArg, *this, ACCC_gang,
                                            StringRef("static:"),
                                            /*ErrorIfNotConst=*/false))
    return nullptr;
  return new (Context) ACCGangClause(StartLoc, LParenLoc, StaticKwLoc,
                                     StaticColonLoc, StaticArg, EndLoc);
}

ACCClause *Sema::ActOnOpenACCWorkerClause(OpenACCDetermination Determination,
                                          SourceLocation StartLoc,
                                          SourceLocation EndLoc) {
  return new (Context) ACCWorkerClause(Determination, StartLoc, EndLoc);
}

ACCClause *Sema::ActOnOpenACCVectorClause(OpenACCDetermination Determination,
                                          SourceLocation StartLoc,
                                          SourceLocation EndLoc) {
  return new (Context) ACCVectorClause(Determination, StartLoc, EndLoc);
}

ACCClause *Sema::ActOnOpenACCNumGangsClause(Expr *NumGangs,
                                            SourceLocation StartLoc,
                                            SourceLocation LParenLoc,
                                            SourceLocation EndLoc) {
  // OpenMP says num_teams must evaluate to a positive integer value.
  // OpenACC doesn't specify such a restriction that I see for num_gangs, but
  // it seems reasonable.
  if (PosIntError == IsPositiveIntegerValue(NumGangs, *this, ACCC_num_gangs,
                                            /*ArgName=*/std::nullopt,
                                            /*ErrorIfNotConst=*/false))
    return nullptr;
  return new (Context) ACCNumGangsClause(NumGangs, StartLoc, LParenLoc,
                                         EndLoc);
}

ACCClause *Sema::ActOnOpenACCNumWorkersClause(Expr *NumWorkers,
                                              SourceLocation StartLoc,
                                              SourceLocation LParenLoc,
                                              SourceLocation EndLoc) {
  // OpenMP says thread_limit must evaluate to a positive integer value.
  // OpenACC doesn't specify such a restriction that I see for num_workers, but
  // it seems reasonable.
  if (PosIntError == IsPositiveIntegerValue(NumWorkers, *this, ACCC_num_workers,
                                            /*ArgName=*/std::nullopt,
                                            /*ErrorIfNotConst=*/false))
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
  PosIntResult Res =
      IsPositiveIntegerValue(VectorLength, *this, ACCC_vector_length,
                             /*ArgName=*/std::nullopt,
                             /*ErrorIfNotConst=*/false);
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
                                            /*ArgName=*/std::nullopt,
                                            /*ErrorIfNotConst=*/true))
    return nullptr;
  OpenACCData->DirStack.setAssociatedLoops(
      Collapse->EvaluateKnownConstInt(Context).getExtValue());
  return new (Context) ACCCollapseClause(Collapse, StartLoc, LParenLoc,
                                         EndLoc);
}

ACCClause *Sema::ActOnOpenACCTileClause(ArrayRef<Expr *> SizeExprList,
                                        SourceLocation StartLoc,
                                        SourceLocation LParenLoc,
                                        SourceLocation EndLoc) {
  for (Expr *SizeExpr : SizeExprList) {
    // TODO: OpenACC says it must be a constant positive integer expression, but
    // we accept non-constant expressions for now.  See the Clang OpenACC status
    // document for further discussion.  In short, this will eventually change.
    if (SizeExpr && !isa<ACCStarExpr>(SizeExpr) &&
        PosIntError == IsPositiveIntegerValue(SizeExpr, *this, ACCC_tile,
                                              /*ArgName=*/std::nullopt,
                                              /*ErrorIfNotConst=*/false))
      return nullptr;
  }
  OpenACCData->DirStack.setAssociatedLoops(SizeExprList.size());
  return ACCTileClause::Create(Context, StartLoc, LParenLoc, EndLoc,
                               SizeExprList);
}

ACCClause *Sema::ActOnOpenACCAsyncClause(Expr *AsyncArg, NamedDecl *Async2Dep,
                                         SourceLocation StartLoc,
                                         SourceLocation LParenLoc,
                                         SourceLocation EndLoc) {
  ACCAsyncClause::AsyncArgStatus TheAsyncArgStatus =
      checkAsyncArg(AsyncArg, *this, ACCC_async);
  if (TheAsyncArgStatus == ACCAsyncClause::AsyncArgIsError)
    return nullptr;
  return new (Context) ACCAsyncClause(AsyncArg, TheAsyncArgStatus, Async2Dep,
                                      StartLoc, LParenLoc, EndLoc);
}

ACCClause *Sema::ActOnOpenACCWaitClause(ArrayRef<Expr *> QueueExprList,
                                        SourceLocation StartLoc,
                                        SourceLocation LParenLoc,
                                        SourceLocation EndLoc) {
  // TODO: Check that has integer expressions.  Currently, this should only be
  // reachable if -fopenacc-fake-async-wait.
  return ACCWaitClause::Create(Context, StartLoc, LParenLoc, EndLoc,
                               QueueExprList);
}

ACCClause *Sema::ActOnOpenACCReadClause(SourceLocation StartLoc,
                                        SourceLocation EndLoc) {
  return new (Context) ACCReadClause(StartLoc, EndLoc);
}

ACCClause *Sema::ActOnOpenACCWriteClause(SourceLocation StartLoc,
                                         SourceLocation EndLoc) {
  return new (Context) ACCWriteClause(StartLoc, EndLoc);
}

ACCClause *Sema::ActOnOpenACCUpdateClause(OpenACCDetermination Determination,
                                          SourceLocation StartLoc,
                                          SourceLocation EndLoc) {
  return new (Context) ACCUpdateClause(Determination, StartLoc, EndLoc);
}

ACCClause *Sema::ActOnOpenACCCaptureClause(SourceLocation StartLoc,
                                           SourceLocation EndLoc) {
  return new (Context) ACCCaptureClause(StartLoc, EndLoc);
}

ACCClause *Sema::ActOnOpenACCCompareClause(SourceLocation StartLoc,
                                           SourceLocation EndLoc) {
  return new (Context) ACCCompareClause(StartLoc, EndLoc);
}

ExprResult Sema::ActOnOpenACCStarExpr(SourceLocation Loc) {
  return new (Context) ACCStarExpr(Context, Loc);
}

bool Sema::isInOpenACCDirectiveStmt() {
  return isOpenACCDirectiveStmt(
      OpenACCData->DirStack.getRealDirective(/*LookOutsideCurFunction=*/true));
}
