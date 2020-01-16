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
  ///
  /// We also store here whether the variable has a reduction on any effective
  /// directive that's a part of the same combined directive.
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
    /// Whether the effective or combined directive has a reduction for this
    /// variable.  That is, true if DSAKind = ACC_DSA_reduction or if this
    /// directive is part of a combined directive with a reduction for this
    /// variable.  In the latter case, the effective directive with the
    /// reduction might have been popped off the stack already.
    bool ReductionOnEffectiveOrCombined = false;
    DAVarData() {}
  };

private:
  struct DirStackEntryTy final {
    llvm::DenseMap<VarDecl *, DAVarData> DAMap;
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
                                   DAVarData &DVar,
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
                                   DAVarData &DVar,
                                   const DeclarationNameInfo &ReductionId) {
      if (K != ACC_DSA_reduction)
        return;
      if (AddHere)
        DVar.ReductionId = ReductionId;
      DVar.ReductionOnEffectiveOrCombined = true;
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
    if (isAllowedDAForDirective(Itr->EffectiveDKind, DAKind)) {
      DA::setReductionFields(!Added, DAKind, DVar, ReductionId);
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

void Sema::InitOpenACCDirectiveStack() {
  OpenACCDirectiveStack = new DirStackTy(*this);
}

#define DirStack static_cast<DirStackTy *>(OpenACCDirectiveStack)

void Sema::DestroyOpenACCDirectiveStack() { delete DirStack; }

void Sema::StartOpenACCDABlock(OpenACCDirectiveKind RealDKind,
                               SourceLocation Loc) {
  switch (RealDKind) {
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
    Res = ActOnOpenACCCopyoutClause(Kind, VarList, StartLoc, LParenLoc,
                                    EndLoc);
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
  case ACCC_nomap:
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
    llvm_unreachable("expected explicit variable list clause");
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
class ImplicitGangAdder : public StmtVisitor<ImplicitGangAdder> {
public:
  void VisitACCExecutableDirective(ACCExecutableDirective *D) {
    if (isOpenACCLoopDirective(D->getDirectiveKind())) {
      auto *LD = cast<ACCLoopDirective>(D);
      ACCPartitioningKind Part = LD->getPartitioning();
      // If there's nested gang partitioning or if the loop has not been
      // determined to be independent, continue on to descendants, some of
      // which that might not be true for.
      if (!LD->getNestedGangPartitioning() && Part.hasIndependent()) {
        // If there's already a gang, worker, or vector clause, don't mess
        // with the directive's partitioning specification.  Don't continue to
        // descendants because this means they can't accept a gang clause
        // either.
        if (Part.hasGangClause() || Part.hasWorkerClause() ||
            Part.hasVectorClause())
          return;
        // The first three conditions checked above plus the fact that we
        // haven't encountered a gang clause on enclosing loops mean this is a
        // gang clause candidate.  The last two conditions above plus the fact
        // that this is the outermost gang clause candidate we've encountered
        // means this is where we add the implicit gang clause.  Don't continue
        // to descendants as they then cannot have a gang clause.
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
    case ACC_DMA_copyin:
    case ACC_DMA_copyout:
    case ACC_DMA_unknown:
      llvm_unreachable("unexpected implicit DMA");
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
      llvm_unreachable("unexpected implicit DSA");
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

      // Below, we assume DSAKind == ACC_DSA_reduction implies
      // ReductionOnEffectiveOrCombined.
      assert((DVar.DSAKind != ACC_DSA_reduction ||
              DVar.ReductionOnEffectiveOrCombined) &&
             "expected reduction to be set consistently");

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
          //
          // We've already rejected reductions on loop control variables.
          assert((!DVar.ReductionOnEffectiveOrCombined ||
                  !Stack->hasLoopControlVariable(VD)) &&
                 "expected acc loop control var not to be reduction var");
          assert((!Stack->hasLoopControlVariable(VD) ||
                  Stack->getLoopPartitioning().hasSeqExplicit()) &&
                 "expected predetermined private for loop control variable "
                 "with explicit seq");
          // See the section "Basic Data Attributes" in the Clang OpenACC
          // design document for discussion of the shared data attribute.
          DSAKind = ACC_DSA_shared;
        }
      } else if (isOpenACCParallelDirective(Stack->getEffectiveDirective())) {
        if (DVar.DMAKind != ACC_DMA_unknown ||
            (DVar.DSAKind != ACC_DSA_unknown &&
             DVar.DSAKind != ACC_DSA_reduction)) {
          // There's a predetermined or explicit DA other than a reduction
          // (which would imply a copy clause), so there's no implicit DA other
          // than the defaults.
        } else if (!VD->getType()->isScalarType()) {
          // OpenACC 3.0 sec. 2.5.1 "Parallel Construct" L830-833:
          //   "If there is no default(present) clause on the construct, an
          //   array or composite variable referenced in the parallel construct
          //   that does not appear in a data clause for the construct or any
          //   enclosing data construct will be treated as if it appeared in a
          //   copy clause for the parallel construct."
          DMAKind = ACC_DMA_copy;
        } else if (DVar.ReductionOnEffectiveOrCombined) {
          // OpenACC 3.0 sec. 2.5.13 "reduction clause" L984-985:
          //   "It implies a copy data clause for each reduction var, unless a
          //   data clause for that variable appears on the compute construct."
          // OpenACC 3.0 sec. 2.11 "Combined Constructs" L1958-1959:
          //   "In addition, a reduction clause on a combined construct implies
          //   a copy data clause for each reduction variable, unless a data
          //   clause for that variable appears on the combined construct."
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
    // Push space for local definitions in this construct.
    LocalDefinitions.emplace_back(LocalDefinitions.back());

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

    // Push space for local definitions in this construct.
    LocalDefinitions.emplace_back(LocalDefinitions.back());

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
    // Push space for privates in this construct.
    Privates.emplace_back(Privates.back());

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

bool Sema::ActOnOpenACCRegionStart(
    OpenACCDirectiveKind DKind, ArrayRef<ACCClause *> Clauses, Scope *CurScope,
    SourceLocation StartLoc, SourceLocation EndLoc) {
  bool ErrorFound = false;
  // Check directive nesting.
  SourceLocation ParentLoc;
  OpenACCDirectiveKind ParentDKind =
      DirStack->getRealParentDirective(ParentLoc);
  if (!isAllowedParentForDirective(DKind, ParentDKind)) {
    // The OpenACC 2.6 spec doesn't say that an acc parallel or acc parallel
    // loop cannot be nested within another acc construct, but gcc 7.3.0 and
    // pgcc 18.4-0 don't permit that for simple cases I've tried.
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
    case ACCD_parallel:
    case ACCD_loop:
    case ACCD_unknown:
      llvm_unreachable("expected combined OpenACC directive");
    }
  }

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
  if (AStmt) {
    // Add implicit gang clauses.
    if (isOpenACCParallelDirective(DKind))
      ImplicitGangAdder().Visit(AStmt);

    // Iterate acc loop control control variables.
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

    // For referenced variables, add implicit DAs, possibly influenced by any
    // implicit gang clauses added above.
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

  switch (DKind) {
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
  if (CurScope->getBreakParent()->isOpenACCLoopScope())
    DirStack->addLoopBreakStatement(BreakLoc);
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
#define OPENACC_CLAUSE_ALIAS_copy(Name) \
  case ACCC_##Name:
#define OPENACC_CLAUSE_ALIAS_copyin(Name) \
  case ACCC_##Name:
#define OPENACC_CLAUSE_ALIAS_copyout(Name) \
  case ACCC_##Name:
#include "clang/Basic/OpenACCKinds.def"
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
      if (OASE->getColonLoc().isInvalid()) {
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
                                        ERange, /*AllowSubarray*/ true);
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
                                        ERange, /*AllowSubarray*/ true);
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
                                         ERange, /*AllowSubarray*/ true);
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

    if (!DirStack->addDMA(VD, RefExpr->IgnoreParens(), ACC_DMA_copyout,
                          ACC_EXPLICIT))
      Vars.push_back(RefExpr->IgnoreParens());
  }

  if (Vars.empty())
    return nullptr;

  return ACCCopyoutClause::Create(Context, Kind, StartLoc, LParenLoc, EndLoc,
                                  Vars);
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
    // TODO: Should this be isConstant?
    if (VD->getType().isConstQualified()) {
      Diag(ELoc, diag::err_acc_const_private);
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
          << getOpenACCName(CKind) << ValExpr->getSourceRange();
      return PosIntError;
    }
    return PosIntNonConst;
  } else if (!Result.isStrictlyPositive()) {
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
#define OPENACC_CLAUSE_ALIAS_copy(Name) \
  case ACCC_##Name:
#define OPENACC_CLAUSE_ALIAS_copyin(Name) \
  case ACCC_##Name:
#define OPENACC_CLAUSE_ALIAS_copyout(Name) \
  case ACCC_##Name:
#include "clang/Basic/OpenACCKinds.def"
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

    // Start OpenMP DA block.
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
    if (AddScopeWithAllPrivates || AddScopeWithLCVPrivate) {
      // Build set of loop control variables unless the only use of that set
      // below will be short-circuited.
      const ArrayRef<VarDecl *> &LCVArray = D->getLoopControlVariables();
      llvm::DenseSet<VarDecl *> LCVSet;
      if (!AddScopeWithAllPrivates) {
        LCVSet.reserve(LCVArray.size());
        LCVSet.insert(LCVArray.begin(), LCVArray.end());
      }
      for (ACCPrivateClause *C : D->getClausesOfKind<ACCPrivateClause>()) {
        for (Expr *VR : C->varlists()) {
          VarDecl *VD = cast<VarDecl>(cast<DeclRefExpr>(VR)->getDecl())
                        ->getCanonicalDecl();
          if (AddScopeWithAllPrivates || LCVSet.count(VD))
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

    // Start OpenMP DA block.
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
  #define OPENACC_CLAUSE(Name, Class)                                    \
    case ACCC_##Name:                                                    \
      return getDerived().Transform ## Class(D, TDKind, cast<Class>(C));
  #define OPENACC_CLAUSE_ALIAS(ClauseAlias, AliasedClause, Class) \
    case ACCC_##ClauseAlias:                                      \
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
    const SourceLocation LocEnd;
    ExplicitClauseLocs(ACCExecutableDirective *D, ACCClause *C,
                       SourceLocation LParenLoc)
      // So far, it appears that only LocStart is used to decide if the
      // directive is implicit.
      : LocStart(D && C->getBeginLoc().isInvalid() ? D->getEndLoc()
                                                   : C->getBeginLoc()),
        LParenLoc(LParenLoc), LocEnd(C->getEndLoc())
    {
      // So far, we have found that, if the clause's LocStart is invalid, all
      // the clause's locations are invalid.  Otherwise, setting LocStart to
      // the directive end might produce locations that are out of order.
      assert((!C->getBeginLoc().isInvalid() ||
              (LParenLoc.isInvalid() && C->getEndLoc().isInvalid())) &&
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
    for (Expr *RefExpr : C->varlists()) {
      Expr *Base = RefExpr;
      while (auto *OASE = dyn_cast<OMPArraySectionExpr>(Base))
        Base = OASE->getBase()->IgnoreParenImpCasts();
      while (auto *ASE = dyn_cast<ArraySubscriptExpr>(Base))
        Base = ASE->getBase()->IgnoreParenImpCasts();
      if (SkipVars.count(cast<VarDecl>(cast<DeclRefExpr>(Base)->getDecl())
                             ->getCanonicalDecl()))
        continue;
      ExprResult EVar = getDerived().TransformExpr(RefExpr);
      if (EVar.isInvalid())
        return true;
      Vars.push_back(EVar.get());
    }
    return false;
  }

  template<typename Derived, typename RebuilderType>
  OMPClauseResult transformACCVarListClause(
      ACCExecutableDirective *D, ACCVarListClause<Derived> *C,
      OpenMPClauseKind TCKind, const llvm::DenseSet<VarDecl *> &SkipVars,
      RebuilderType Rebuilder) {
    OpenMPStartEndClauseRAII ClauseRAII(getSema(), TCKind);
    llvm::SmallVector<Expr *, 16> Vars;
    if (transformACCVarList(D, C, SkipVars, Vars))
      return OMPClauseError();
    if (Vars.empty())
      return OMPClauseEmpty();
    ExplicitClauseLocs L(D, C, C->getLParenLoc());
    return Rebuilder(Vars, L);
  }

  template<typename Derived, typename RebuilderType>
  OMPClauseResult transformACCVarListClause(
      ACCExecutableDirective *D, ACCVarListClause<Derived> *C,
      OpenMPClauseKind TCKind, RebuilderType Rebuilder) {
    return transformACCVarListClause(D, C, TCKind, llvm::DenseSet<VarDecl *>(),
                                     Rebuilder);
  }

  class OMPVarListClauseRebuilder {
    TransformACCToOMP *ACCToOMP;
    OMPClause *(TransformACCToOMP::*Rebuilder)(
        ArrayRef<Expr *>, SourceLocation, SourceLocation, SourceLocation);
  public:
    OMPVarListClauseRebuilder(
        TransformACCToOMP *ACCToOMP,
        OMPClause *(TransformACCToOMP::*Rebuilder)(
            ArrayRef<Expr *>, SourceLocation, SourceLocation, SourceLocation))
      : ACCToOMP(ACCToOMP), Rebuilder(Rebuilder) {}
    OMPClause *operator()(ArrayRef<Expr *> Vars, const ExplicitClauseLocs &L) {
      return (ACCToOMP->getDerived().*Rebuilder)(Vars, L.LocStart, L.LParenLoc,
                                                 L.LocEnd);
    }
  };

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

  OMPClauseResult TransformACCNomapClause(ACCExecutableDirective *D,
                                          OpenMPDirectiveKind TDKind,
                                          ACCNomapClause *C) {
    return OMPClauseEmpty();
  }

  OMPClauseResult TransformACCCopyClause(ACCExecutableDirective *D,
                                         OpenMPDirectiveKind TDKind,
                                         ACCCopyClause *C) {
    return transformACCVarListClause<ACCCopyClause>(
        D, C, OMPC_map,
        [&](ArrayRef<Expr *> Vars, const ExplicitClauseLocs &L) {
          return getDerived().RebuildOMPMapClause(
            llvm::None, llvm::None, CXXScopeSpec(), DeclarationNameInfo(),
            OMPC_MAP_tofrom, /*IsMapTypeImplicit*/ false, L.LocStart,
            L.LParenLoc, Vars,
            OMPVarListLocTy(L.LocStart, L.LParenLoc, L.LocEnd), llvm::None);
        });
  }

  OMPClauseResult TransformACCCopyinClause(ACCExecutableDirective *D,
                                           OpenMPDirectiveKind TDKind,
                                           ACCCopyinClause *C) {
    return transformACCVarListClause<ACCCopyinClause>(
        D, C, OMPC_map,
        [&](ArrayRef<Expr *> Vars, const ExplicitClauseLocs &L) {
          return getDerived().RebuildOMPMapClause(
            llvm::None, llvm::None, CXXScopeSpec(), DeclarationNameInfo(),
            OMPC_MAP_to, /*IsMapTypeImplicit*/ false, L.LocStart, L.LParenLoc,
            Vars,
            OMPVarListLocTy(L.LocStart, L.LParenLoc, L.LocEnd), llvm::None);
        });
  }

  OMPClauseResult TransformACCCopyoutClause(ACCExecutableDirective *D,
                                            OpenMPDirectiveKind TDKind,
                                            ACCCopyoutClause *C) {
    return transformACCVarListClause<ACCCopyoutClause>(
        D, C, OMPC_map,
        [&](ArrayRef<Expr *> Vars, const ExplicitClauseLocs &L) {
          return getDerived().RebuildOMPMapClause(
            llvm::None, llvm::None, CXXScopeSpec(), DeclarationNameInfo(),
            OMPC_MAP_from, /*IsMapTypeImplicit*/ false, L.LocStart,
            L.LParenLoc, Vars,
            OMPVarListLocTy(L.LocStart, L.LParenLoc, L.LocEnd), llvm::None);
        });
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
        OMPVarListClauseRebuilder(
            this, &TransformACCToOMP::RebuildOMPSharedClause));
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
      const ArrayRef<VarDecl *> &LCVArray = LD->getLoopControlVariables();
      SkipVars.reserve(LCVArray.size());
      SkipVars.insert(LCVArray.begin(), LCVArray.end());
    }
    return transformACCVarListClause<ACCPrivateClause>(
        D, C, OMPC_private, SkipVars,
        OMPVarListClauseRebuilder(
            this, &TransformACCToOMP::RebuildOMPPrivateClause));
  }

  OMPClauseResult TransformACCFirstprivateClause(ACCExecutableDirective *D,
                                                 OpenMPDirectiveKind TDKind,
                                                 ACCFirstprivateClause *C) {
    return transformACCVarListClause<ACCFirstprivateClause>(
        D, C, OMPC_firstprivate,
        OMPVarListClauseRebuilder(
            this, &TransformACCToOMP::RebuildOMPFirstprivateClause));
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
    return transformACCVarListClause<ACCReductionClause>(
        D, C, OMPC_reduction,
        [&](ArrayRef<Expr *> Vars, const ExplicitClauseLocs &L) {
          CXXScopeSpec Spec;
          return getDerived().RebuildOMPReductionClause(
            Vars, L.LocStart, L.LParenLoc, C->getColonLoc(), L.LocEnd, Spec,
            C->getNameInfo(), ArrayRef<Expr*>());
        });
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
  if (DirStack->getRealDirective() != ACCD_unknown)
    return false;
  if (TransformACCToOMP(*this).TransformStmt(D).isInvalid())
    return true;
  return false;
}
