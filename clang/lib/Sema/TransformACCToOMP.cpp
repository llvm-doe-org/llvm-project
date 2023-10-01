//===--- TransformACCToOMP.cpp - Transformation of OpenACC to OpenMP ------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// This file implements the transformation of OpenACC directives and clauses to
/// OpenMP.
///
//===----------------------------------------------------------------------===//

#include "ACCDataVar.h"
#include "TransformContext.h"
#include "clang/Basic/DiagnosticCategories.h"
using namespace clang;

namespace {
class TransformACCToOMP : public TransformContext<TransformACCToOMP> {
  typedef TransformContext<TransformACCToOMP> BaseTransform;
  /// A variable's DA data.
  struct DAVarData {
    /// If the DMA is undetermined, then ACC_DMA_unknown.  Otherwise, not
    /// ACC_DMA_unknown.
    OpenACCDMAKind DMAKind = ACC_DMA_unknown;
    /// If the DSA is undetermined, then ACC_DSA_unknown.  Otherwise, not
    /// ACC_DSA_unknown.
    OpenACCDSAKind DSAKind = ACC_DSA_unknown;
    /// If no DMA is visible, then ACC_DMA_unknown.  Otherwise, not
    /// ACC_DMA_unknown.
    OpenACCDMAKind VisibleDMAKind = ACC_DMA_unknown;
    /// If no DMA is visible, then \c nullptr.  Otherwise, the referencing
    // expression appearing in the associated clause.
    Expr *VisibleDMARefExpr = nullptr;
  };

  /// Data we store per effective OpenACC directive while transforming them.
  struct DirStackEntry {
    /// Map from all variables with DAs on this OpenACC directive to those DAs.
    /// This is set before OpenACC clauses are translated.
    llvm::DenseMap<ACCDataVar, DAVarData> DAMap;
    ///@{
    /// Before translating the associated statement of an acc parallel
    /// directive, TransformACCParallelDirective sets this as follows, and it
    /// is copied to descendant stack entries as those entries are created.
    ///
    /// If the acc parallel directive has a constant-expression vector_length
    /// clause, then VectorLengthExpr is set to its value.
    Expr *VectorLengthExpr = nullptr;
    ///@}
  };
  /// This stack is pushed and popped at each effective OpenACC directive.
  SmallVector<DirStackEntry, 4> DirStack;
  /// An RAII object to push and pop entries in DirStack.
  class DirStackEntryRAII {
  private:
    TransformACCToOMP &Transform;

  public:
    /// Push D's data onto Transform.DirStack.
    DirStackEntryRAII(TransformACCToOMP &Transform, ACCDirectiveStmt *D)
        : Transform(Transform) {
      DirStackEntry &DirEntry = Transform.DirStack.emplace_back();

      // Collect DAs from clauses on D.
      DAVarData ClauseDAs;
      auto RecordClauseDAs = [&](Expr *RefExpr) {
        DAVarData &VarDAs = DirEntry.DAMap[ACCDataVar(RefExpr)];
        if (ClauseDAs.DMAKind != ACC_DMA_unknown) {
          assert(VarDAs.DMAKind == ACC_DMA_unknown &&
                 "expected at most one DMA per variable");
          VarDAs.DMAKind = VarDAs.VisibleDMAKind = ClauseDAs.DMAKind;
          VarDAs.VisibleDMARefExpr = RefExpr;
        }
        if (ClauseDAs.DSAKind != ACC_DSA_unknown) {
          assert(VarDAs.DSAKind == ACC_DSA_unknown &&
                 "expected at most one DSA per variable");
          VarDAs.DSAKind = ClauseDAs.DSAKind;
        }
        return false;
      };
      for (ACCClause *C : D->clauses()) {
        switch (C->getClauseKindDealiased()) {
#define OPENACC_DMA(Name, Class)                                        \
        case ACCC_##Name:                                               \
          ClauseDAs.DMAKind = ACC_DMA_##Name;                           \
          ClauseDAs.DSAKind = ACC_DSA_unknown;                          \
          Transform.iterateACCVarList(cast<Class>(C), RecordClauseDAs); \
          break;
#define OPENACC_DSA_UNMAPPABLE(Name, Class)                             \
        case ACCC_##Name:                                               \
          ClauseDAs.DMAKind = ACC_DMA_unknown;                          \
          ClauseDAs.DSAKind = ACC_DSA_##Name;                           \
          Transform.iterateACCVarList(cast<Class>(C), RecordClauseDAs); \
          break;
#define OPENACC_DSA_MAPPABLE(Name, Class)                               \
        case ACCC_##Name:                                               \
          ClauseDAs.DMAKind = ACC_DMA_unknown;                          \
          ClauseDAs.DSAKind = ACC_DSA_##Name;                           \
          Transform.iterateACCVarList(cast<Class>(C), RecordClauseDAs); \
          break;
#include "clang/Basic/OpenACCKinds.def"
        default:
          break;
        }
      }

      // If this is the outermost directive entry, we're done.
      if (Transform.DirStack.size() == 1)
        return;
      DirStackEntry &ParentDirEntry = *(Transform.DirStack.rbegin() + 1);

      // Collect visible DMAs from parent directive.
      for (auto ParentEntry : ParentDirEntry.DAMap) {
        const ACCDataVar &Var = ParentEntry.first;
        const DAVarData &ParentDAs = ParentEntry.second;
        DAVarData &DAs = DirEntry.DAMap[Var];
        if (DAs.VisibleDMAKind == ACC_DMA_unknown) {
          DAs.VisibleDMAKind = ParentDAs.VisibleDMAKind;
          DAs.VisibleDMARefExpr = ParentDAs.VisibleDMARefExpr;
        }
      }

      // Copy data from parent directive.
      DirEntry.VectorLengthExpr = ParentDirEntry.VectorLengthExpr;
    }
    /// Pop top directive's data from Transform.DirStack.
    ~DirStackEntryRAII() { Transform.DirStack.pop_back(); }
  };

  using OMPClauseResult = ActionResult<OMPClause *>;
  inline OMPClauseResult OMPClauseError() { return OMPClauseResult(true); }
  inline OMPClauseResult OMPClauseEmpty() { return OMPClauseResult(false); }

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
    ExplicitClauseLocs(ACCDirectiveStmt *D, ACCClause *C,
                       SourceLocation LParenLoc = SourceLocation())
        // So far, it appears that only LocStart is used to decide if the
        // directive is implicit.
        : LocStart(D && C->getBeginLoc().isInvalid() ? D->getEndLoc()
                                                     : C->getBeginLoc()),
          LParenLoc(LParenLoc), LocEnd(C->getEndLoc()) {
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

  void transformACCClauses(ACCDirectiveStmt *D, OpenMPDirectiveKind TDKind,
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

  StmtResult transformACCAssociatedStmt(ACCDirectiveStmt *D,
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

  template <typename Derived, typename OperationType>
  bool iterateACCVarList(ACCVarListClause<Derived> *C,
                         OperationType Operation) {
    for (Expr *RefExpr : C->varlists()) {
      if (Operation(RefExpr))
        return true;
    }
    return false;
  }

  enum VarAction {
    VAR_DISCARD,
    VAR_PRESERVE,
    VAR_ADD_ZERO_LENGTH_ARRAY_SECTION
  };

  template <typename Derived, typename VarFilterType>
  bool transformACCVarList(ACCDirectiveStmt *D, ACCVarListClause<Derived> *C,
                           VarFilterType VarFilter,
                           llvm::SmallVector<Expr *, 16> &Vars) {
    Vars.reserve(C->varlist_size());
    return iterateACCVarList(C, [&VarFilter, &Vars, this](Expr *RefExpr) {
      VarAction Action = VarFilter(RefExpr);
      if (Action == VAR_DISCARD)
        return false;
      ExprResult EVar = getDerived().TransformExpr(RefExpr);
      if (EVar.isInvalid())
        return true;
      if (Action == VAR_ADD_ZERO_LENGTH_ARRAY_SECTION) {
        SourceLocation Loc = RefExpr->getEndLoc();
        ExprResult Zero = getSema().ActOnIntegerConstant(Loc, 0);
        if (Zero.isInvalid())
          return true;
        EVar = getDerived().RebuildOMPArraySectionExpr(
            EVar.get(), Loc, Zero.get(), Loc, SourceLocation(), Zero.get(),
            nullptr, Loc);
        if (EVar.isInvalid())
          return true;
      }
      Vars.push_back(EVar.get());
      return false;
    });
  }

  template <typename Derived, typename VarFilterType, typename RebuilderType>
  OMPClauseResult
  transformACCVarListClause(ACCDirectiveStmt *D, ACCVarListClause<Derived> *C,
                            OpenMPClauseKind TCKind, VarFilterType VarFilter,
                            RebuilderType Rebuilder) {
    OpenMPStartEndClauseRAII ClauseRAII(getSema(), TCKind);
    llvm::SmallVector<Expr *, 16> Vars;
    if (transformACCVarList(D, C, VarFilter, Vars))
      return OMPClauseError();
    if (Vars.empty())
      return OMPClauseEmpty();
    ExplicitClauseLocs L(D, C, C->getLParenLoc());
    return Rebuilder(Vars, L);
  }

  template <typename Derived, typename RebuilderType>
  OMPClauseResult
  transformACCVarListClause(ACCDirectiveStmt *D, ACCVarListClause<Derived> *C,
                            OpenMPClauseKind TCKind, RebuilderType Rebuilder) {
    return transformACCVarListClause(
        D, C, TCKind, [](Expr *) { return VAR_PRESERVE; }, Rebuilder);
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

  void addHoldMapTypeModifier(ACCDirectiveStmt *D, ACCClause *C,
                              SmallVectorImpl<OpenMPMapModifierKind> &MapMods) {
    switch (D->getDirectiveKind()) {
    case ACCD_enter_data:
    case ACCD_exit_data:
      return;
    case ACCD_data:
    case ACCD_parallel:
    case ACCD_parallel_loop:
      break;
    case ACCD_update:
    case ACCD_wait:
    case ACCD_loop:
    case ACCD_routine:
    case ACCD_atomic:
    case ACCD_unknown:
      llvm_unreachable("expected directive that takes data clauses");
    }
    switch (getSema().LangOpts.getOpenACCStructuredRefCountOMP()) {
    case LangOptions::OpenACCStructuredRefCountOMP_OmpxHold:
      MapMods.push_back(OMPC_MAP_MODIFIER_ompx_hold);
      getSema().Diag(C->getBeginLoc(), diag::warn_acc_omp_map_ompx_hold)
          << getOpenACCName(C->getClauseKind()) << C->getSourceRange();
      getSema().Diag(C->getBeginLoc(), diag::note_acc_alternate_omp)
          << "structured-ref-count"
          << LangOptions::getOpenACCStructuredRefCountOMPValue(
                 LangOptions::OpenACCStructuredRefCountOMP_NoOmpxHold);
      getSema().Diag(C->getBeginLoc(), diag::note_acc_disable_diag)
          << DiagnosticIDs::getWarningOptionForDiag(
                 diag::warn_acc_omp_map_ompx_hold);
      getSema().Diag(C->getBeginLoc(), diag::note_acc_disable_all_omp_ext_diags)
          << DiagnosticIDs::getWarningOptionForGroup(
                 diag::Group::OpenACCOMPExt);
      break;
    case LangOptions::OpenACCStructuredRefCountOMP_NoOmpxHold:
      break;
    }
  }

  /// A RAII object to conditionally build a compound scope and statement.
  class ConditionalCompoundStmtRAII {
    TransformACCToOMP &Tx;
    bool Started : 1;
    bool Finalized : 1;
    bool Err : 1;
    SmallVector<Stmt*, 8> Stmts;
    llvm::DenseMap<Decl *, Decl *> OldDeclTxs;
    void prepForAdd() {
      assert(!Finalized &&
             "expected compound statement not to be finalized yet");
      if (!Started) {
        Tx.getSema().ActOnStartOfCompoundStmt(/*IsStmtExpr=*/false);
        Started = true;
      }
    }
    template <typename T>
    void add(const T &Res) {
      if (Res.isInvalid()) {
        Err = true;
        return;
      }
      Stmts.push_back(Res.get());
    }
  public:
    ConditionalCompoundStmtRAII(TransformACCToOMP &Tx)
        : Tx(Tx), Started(false), Finalized(false), Err(false) {}
    /// Add a variable declaration that privatizes an enclosing declaration.
    /// (For an entirely new variable, we once had an \c addNewPrivateDecl, but
    /// there are currently no uses.)
    void addPrivatizingDecl(SourceLocation RefStartLoc, SourceLocation RefEndLoc,
                            VarDecl *VD) {
      prepForAdd();
      VD = VD->getCanonicalDecl();
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
                                                           /*DropInit=*/true);
      add(Tx.getSema().ActOnDeclStmt(
              Tx.getSema().ConvertDeclToDeclGroup(DPrivate), RefStartLoc,
              RefEndLoc));
    }
    /// Evaluate an expression that has side effects but whose original use has
    /// been removed.
    void addUnusedExpr(Expr *E) {
      prepForAdd();
      ExprResult Res = Tx.getDerived().TransformExpr(E);
      if (Res.isInvalid()) {
        Err = true;
        return;
      }
      ASTContext &Context = Tx.getSema().getASTContext();
      SourceLocation Loc = E->getExprLoc();
      TypeSourceInfo *TInfo = Context.getTrivialTypeSourceInfo(Context.VoidTy,
                                                               Loc);
      add(Tx.getSema().BuildCStyleCastExpr(Loc, TInfo, Loc, Res.get()));
    }
    /// Add a statement that does not require any transformation and that does
    /// not terminate the compound statement.
    void addNewStmt(StmtResult &S) {
      prepForAdd();
      add(S);
    }
    /// Add the statement that terminates the compound statement.
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

  // We map locations as follows:
  //
  // 1. If C has an async_arg and Async2DepArg is its OpenMP transformation:
  //
  //      L.LocStart  L.LParenLoc                            L.LocEnd
  //      async       (                           async_arg  )
  //      depend      (inout:*acc2omp_async2dep(  async_arg  ))
  //
  //    It's up to the caller to ensure that L correctly represents C's location
  //    info and thus that C's async_arg's location info is between L.LParenLoc
  //    and L.LocEnd.
  //
  // 2. If C has no async_arg or Async2DepArg is not its OpenMP transformation:
  //
  //      L.LocStart  L.LParenLoc = L.LocEnd
  //      async
  //      depend      (inout:*acc2omp_async2dep(async_arg_gen))
  //
  //      L.LocStart              L.LParenLoc = L.LocEnd
  //      async       (async_arg  )
  //      depend                  (inout:*acc2omp_async2dep(async_arg_gen))
  //
  //    It's up to the caller to ensure that L.LocStart and L.LocEnd correctly
  //    represent C's location info and that L.LParenLoc == L.LocEnd.
  OMPClauseResult buildOMPDependClause(ACCAsyncClause *C, ExplicitClauseLocs L,
                                       Expr *Async2DepArg) {
    ASTContext &Ctxt = getSema().getASTContext();

    // Get acc2omp_async2dep.
    NamedDecl *Async2DepND = C->getAsync2Dep();
    if (!Async2DepND) {
      // If we just reported this error for C, don't repeat it.
      static ACCAsyncClause *CErr = nullptr;
      if (C != CErr) {
        CErr = C;
        // TODO: Eventually, we want to insert the prototype if missing.  Be
        // sure to make it 'extern "C"' so it resolves to libacc2omp's
        // implementation even if we're in, for example, a C++ namespace.
        getSema().Diag(C->getBeginLoc(), diag::err_acc_missing_function)
            << ACCAsyncClause::Async2DepName;
        getSema().Diag(C->getBeginLoc(), diag::note_acc_required_by_clause)
            << getOpenACCName(C->getClauseKind()) << C->getSourceRange();
        getSema().Diag(C->getBeginLoc(), diag::note_acc_include_openacc_h);
      }
      return OMPClauseError();
    }
    FunctionDecl *Async2DepFD = dyn_cast<FunctionDecl>(Async2DepND);
    QualType Async2DepExpectedType =
        Ctxt.getFunctionType(/*ResultTy=*/Ctxt.getPointerType(Ctxt.CharTy),
                             /*Args=*/{Ctxt.IntTy},
                             FunctionProtoType::ExtProtoInfo());
    if (!Async2DepFD || Async2DepFD->getType() != Async2DepExpectedType) {
      // If we just reported this error for C, don't repeat it.
      static ACCAsyncClause *CErr = nullptr;
      if (C != CErr) {
        CErr = C;
        getSema().Diag(Async2DepND->getBeginLoc(), diag::err_acc_not_function)
            << ACCAsyncClause::Async2DepName << Async2DepExpectedType
            << Async2DepND->getSourceRange();
        getSema().Diag(C->getBeginLoc(), diag::note_acc_required_by_clause)
            << getOpenACCName(C->getClauseKind()) << C->getSourceRange();
        getSema().Diag(C->getBeginLoc(), diag::note_acc_include_openacc_h);
      }
      return OMPClauseError();
    }

    // Build *acc2omp_async2dep(async_arg).
    DeclRefExpr *Async2DepDRE = getSema().BuildDeclRefExpr(
        Async2DepFD, Async2DepFD->getType(), VK_LValue, L.LParenLoc);
    if (!Async2DepDRE)
      return OMPClauseError();
    Expr *Async2DepArgs[] = {Async2DepArg};
    ExprResult Async2DepCall = getSema().BuildCallExpr(
        getSema().getCurScope(), Async2DepDRE, L.LParenLoc, Async2DepArgs,
        L.LocEnd);
    if (Async2DepCall.isInvalid())
      return OMPClauseError();
    ExprResult Async2DepCallDeref = getSema().BuildUnaryOp(
        getSema().getCurScope(), L.LParenLoc, UO_Deref, Async2DepCall.get());
    if (Async2DepCallDeref.isInvalid())
      return OMPClauseError();

    // Build depend clause.
    Expr *DependClauseArgs[] = {Async2DepCallDeref.get()};
    return getDerived().RebuildOMPDependClause(
      {OMPC_DEPEND_inout, /*DepLoc=*/L.LParenLoc, /*ColonLoc=*/L.LParenLoc,
       /*OmpAllMemoryLoc=*/SourceLocation()},
      /*DepModifier=*/nullptr, DependClauseArgs, L.LocStart, L.LParenLoc,
      L.LocEnd);
  }

  void transformACCAsyncClauseToOMPClauses(
      ACCDirectiveStmt *D, ACCAsyncClause *C,
      llvm::SmallVectorImpl<OMPClause *> &TClauses,
      size_t &TClausesAddedCount) {
    // We cannot have asynchronous behavior while some wait features are faked.
    if (getSema().getLangOpts().OpenACCFakeAsyncWait)
      return;

    // We can simply discard the async clause if it's definitely acc_async_sync.
    if (C->getAsyncArgStatus() == ACCAsyncClause::AsyncArgIsSync)
      return;

    // If we return before adding two clauses, there was an error.
    TClausesAddedCount += 2;
    ExplicitClauseLocs L(D, C,
                         C->getLParenLoc().isInvalid() ? C->getEndLoc()
                                                       : C->getLParenLoc());

    // Add nowait clause.
    OMPClause *NowaitClause = getSema().ActOnOpenMPNowaitClause(L.LocStart,
                                                                L.LocEnd);
    if (!NowaitClause)
      return;
    TClauses.push_back(NowaitClause);

    // Transform the async_arg.
    OpenMPStartEndClauseRAII ClauseRAII(getSema(), OMPC_depend);
    ExprResult TAsyncArg;
    if (Expr *AsyncArg = C->getAsyncArg())
      TAsyncArg = getDerived().TransformExpr(C->getAsyncArg());
    else
      TAsyncArg = getSema().ActOnIntegerConstant(
          L.LocEnd, ACCAsyncClause::Acc2ompAccAsyncNoval);
    if (TAsyncArg.isInvalid())
      return;

    // Build and add the depend clause.
    OMPClauseResult DependClause = buildOMPDependClause(C, L,
                                                        TAsyncArg.get());
    if (!DependClause.isInvalid())
      TClauses.push_back(DependClause.get());
  }

  // To handle acc_async_sync, we need a taskwait for the sync queue after the
  // construct.  The simplest way to implement that (given that the parent might
  // not be a compound statement), is to add a compound statement enclosing
  // both.
  void transformACCAsyncClauseToTrailingOMPTaskwait(
      ACCDirectiveStmt *D, ConditionalCompoundStmtRAII &EnclosingCompoundStmt,
      StmtResult &Res) {
    // If we don't have asynchronous behavior because there's no async clause or
    // because we're faking (see transformACCAsyncClauseToOMPClauses), then this
    // taskwait is useless.
    ACCAsyncClause *C = D->getTheClauseOfKind<ACCAsyncClause>();
    if (!C || getSema().getLangOpts().OpenACCFakeAsyncWait) {
      EnclosingCompoundStmt.finalize(Res);
      return;
    }
    // We only need a taskwait for the synchronous queue if we cannot determine
    // at compile time whether we have synchronous or asynchronous behavior
    // (whether the async_arg is acc_async_sync).  That is, we need nowait if it
    // might be asynchronous, and in that case we need the taskwait if it might
    // be synchronous.
    switch (C->getAsyncArgStatus()) {
      case ACCAsyncClause::AsyncArgIsSync:
      case ACCAsyncClause::AsyncArgIsAsync:
        EnclosingCompoundStmt.finalize(Res);
        return;
      case ACCAsyncClause::AsyncArgIsUnknown:
        break;
      case ACCAsyncClause::AsyncArgIsError:
        llvm_unreachable("expected valid async-arg");
    }
    EnclosingCompoundStmt.addNewStmt(Res);
    ExplicitClauseLocs L(D, C, C->getEndLoc());

    // Build the async_arg.
    ExprResult Async2DepArg =
        getSema().ActOnIntegerConstant(L.LocEnd,
                                       ACCAsyncClause::Acc2ompAccAsyncSync);
    if (Async2DepArg.isInvalid()) {
      Res = StmtError();
      return;
    }

    // Build the depend clause.
    OMPClauseResult DependClause = buildOMPDependClause(C, L,
                                                        Async2DepArg.get());
    if (DependClause.isInvalid()) {
      Res = StmtError();
      return;
    }

    // Build OpenMP directive and finalize enclosing compound statement.
    DirStackEntryRAII TheDirStackEntryRAII(*this, D);
    getSema().StartOpenMPDSABlock(OMPD_taskwait, DeclarationNameInfo(),
                                  /*CurScope=*/nullptr, L.LocStart);
    OMPClause *TaskwaitClauses[] = {DependClause.get()};
    Res = getDerived().RebuildOMPExecutableDirective(
        OMPD_taskwait, DeclarationNameInfo(), /*CancelRegion=*/OMPD_unknown,
        TaskwaitClauses, /*AStmt=*/nullptr, L.LocStart, L.LocEnd);
    getSema().EndOpenMPDSABlock(Res.get());
    EnclosingCompoundStmt.finalize(Res);
  }

public:
  TransformACCToOMP(Sema &SemaRef) : BaseTransform(SemaRef) {}

  // TODO: The common parts of the next several functions could probably be
  // captured in a template function.
  StmtResult TransformACCUpdateDirective(ACCUpdateDirective *D) {
    DirStackEntryRAII TheDirStackEntryRAII(*this, D);

    // Start OpenMP DA block.
    getSema().StartOpenMPDSABlock(OMPD_target_update, DeclarationNameInfo(),
                                  /*CurScope=*/nullptr, D->getBeginLoc());

    // Transform OpenACC clauses.
    llvm::SmallVector<OMPClause *, 16> TClauses;
    size_t TClausesEmptyCount;
    transformACCClauses(D, OMPD_target_update, TClauses, TClausesEmptyCount);

    // Neither acc update nor omp target update have associated statements, but
    // for some reason OMPTargetUpdateDirective expects one.
    assert(!D->hasAssociatedStmt() &&
           "expected no associated statement on 'acc update' directive");
    getSema().ActOnOpenMPRegionStart(OMPD_target_update, nullptr);
    StmtResult AssociatedStmt =
        (Sema::CompoundScopeRAII(getSema()),
         getSema().ActOnCompoundStmt(D->getEndLoc(), D->getEndLoc(),
                                     std::nullopt, /*isStmtExpr=*/false));
    AssociatedStmt = getSema().ActOnOpenMPRegionEnd(AssociatedStmt, TClauses);

    // Build OpenMP directive and finalize enclosing compound statement, if
    // any.
    StmtResult Res;
    if (TClauses.size() != D->clauses().size() - TClausesEmptyCount)
      Res = StmtError();
    else
      Res = getDerived().RebuildOMPExecutableDirective(
          OMPD_target_update, DeclarationNameInfo(), OMPD_unknown, TClauses,
          AssociatedStmt.get(), D->getBeginLoc(), D->getEndLoc());
    getSema().EndOpenMPDSABlock(Res.get());
    if (!Res.isInvalid())
      D->setOMPNode(Res.get());
    return Res;
  }

  StmtResult TransformACCEnterDataDirective(ACCEnterDataDirective *D) {
    DirStackEntryRAII TheDirStackEntryRAII(*this, D);

    // Start OpenMP DA block.
    getSema().StartOpenMPDSABlock(OMPD_target_enter_data, DeclarationNameInfo(),
                                  /*CurScope=*/nullptr, D->getBeginLoc());

    // Transform OpenACC clauses.
    llvm::SmallVector<OMPClause *, 16> TClauses;
    size_t TClausesEmptyCount;
    transformACCClauses(D, OMPD_target_enter_data, TClauses,
                        TClausesEmptyCount);

    // Neither acc enter data nor omp target enter data have associated
    // statements, but for some reason OMPTargetEnterDataDirective expects one.
    assert(!D->hasAssociatedStmt() &&
           "expected no associated statement on 'acc enter data' directive");
    getSema().ActOnOpenMPRegionStart(OMPD_target_enter_data, nullptr);
    StmtResult AssociatedStmt =
        (Sema::CompoundScopeRAII(getSema()),
         getSema().ActOnCompoundStmt(D->getEndLoc(), D->getEndLoc(),
                                     std::nullopt, /*isStmtExpr=*/false));
    AssociatedStmt = getSema().ActOnOpenMPRegionEnd(AssociatedStmt, TClauses);

    // Build OpenMP directive and finalize enclosing compound statement, if any.
    StmtResult Res;
    if (TClauses.size() != D->clauses().size() - TClausesEmptyCount)
      Res = StmtError();
    else
      Res = getDerived().RebuildOMPExecutableDirective(
          OMPD_target_enter_data, DeclarationNameInfo(), OMPD_unknown, TClauses,
          AssociatedStmt.get(), D->getBeginLoc(), D->getEndLoc());
    getSema().EndOpenMPDSABlock(Res.get());
    if (!Res.isInvalid())
      D->setOMPNode(Res.get());
    return Res;
  }

  StmtResult TransformACCExitDataDirective(ACCExitDataDirective *D) {
    DirStackEntryRAII TheDirStackEntryRAII(*this, D);

    // Start OpenMP DA block.
    getSema().StartOpenMPDSABlock(OMPD_target_exit_data, DeclarationNameInfo(),
                                  /*CurScope=*/nullptr, D->getBeginLoc());

    // Transform OpenACC clauses.
    llvm::SmallVector<OMPClause *, 16> TClauses;
    size_t TClausesEmptyCount;
    transformACCClauses(D, OMPD_target_exit_data, TClauses, TClausesEmptyCount);

    // Neither acc exit data nor omp target exit data have associated
    // statements, but for some reason OMPTargetExitDataDirective expects one.
    assert(!D->hasAssociatedStmt() &&
           "expected no associated statement on 'acc exit data' directive");
    getSema().ActOnOpenMPRegionStart(OMPD_target_exit_data, nullptr);
    StmtResult AssociatedStmt =
        (Sema::CompoundScopeRAII(getSema()),
         getSema().ActOnCompoundStmt(D->getEndLoc(), D->getEndLoc(),
                                     std::nullopt, /*isStmtExpr=*/false));
    AssociatedStmt = getSema().ActOnOpenMPRegionEnd(AssociatedStmt, TClauses);

    // Build OpenMP directive and finalize enclosing compound statement, if any.
    StmtResult Res;
    if (TClauses.size() != D->clauses().size() - TClausesEmptyCount)
      Res = StmtError();
    else
      Res = getDerived().RebuildOMPExecutableDirective(
          OMPD_target_exit_data, DeclarationNameInfo(), OMPD_unknown, TClauses,
          AssociatedStmt.get(), D->getBeginLoc(), D->getEndLoc());
    getSema().EndOpenMPDSABlock(Res.get());
    if (!Res.isInvalid())
      D->setOMPNode(Res.get());
    return Res;
  }

  StmtResult TransformACCWaitDirective(ACCWaitDirective *D) {
    DirStackEntryRAII TheDirStackEntryRAII(*this, D);

    // Start OpenMP DA block.
    getSema().StartOpenMPDSABlock(OMPD_taskwait, DeclarationNameInfo(),
                                  /*CurScope=*/nullptr, D->getBeginLoc());

    // Transform OpenACC clauses.
    llvm::SmallVector<OMPClause *, 16> TClauses;
    size_t TClausesEmptyCount;
    transformACCClauses(D, OMPD_taskwait, TClauses, TClausesEmptyCount);

    // Build OpenMP directive and finalize enclosing compound statement, if any.
    StmtResult Res;
    if (TClauses.size() != D->clauses().size() - TClausesEmptyCount)
      Res = StmtError();
    else
      Res = getDerived().RebuildOMPExecutableDirective(
          OMPD_taskwait, DeclarationNameInfo(), /*CancelRegion=*/OMPD_unknown,
          TClauses, /*AStmt=*/nullptr, D->getBeginLoc(), D->getEndLoc());
    getSema().EndOpenMPDSABlock(Res.get());
    if (!Res.isInvalid())
      D->setOMPNode(Res.get());
    return Res;
  }

  StmtResult TransformACCDataDirective(ACCDataDirective *D) {
    DirStackEntryRAII TheDirStackEntryRAII(*this, D);

    // Start OpenMP DA block.
    getSema().StartOpenMPDSABlock(OMPD_target_data, DeclarationNameInfo(),
                                  /*CurScope=*/nullptr, D->getBeginLoc());

    // Transform OpenACC clauses.
    llvm::SmallVector<OMPClause *, 16> TClauses;
    size_t TClausesEmptyCount;
    transformACCClauses(D, OMPD_target_data, TClauses, TClausesEmptyCount);

    // Transform associated statement.
    StmtResult AssociatedStmt = transformACCAssociatedStmt(D, OMPD_target_data,
                                                           TClauses);

    // Build OpenMP directive and finalize enclosing compound statement, if
    // any.
    StmtResult Res;
    if (AssociatedStmt.isInvalid() ||
        TClauses.size() != D->clauses().size() - TClausesEmptyCount)
      Res = StmtError();
    else
      Res = getDerived().RebuildOMPExecutableDirective(
          OMPD_target_data, DeclarationNameInfo(), OMPD_unknown, TClauses,
          AssociatedStmt.get(), D->getBeginLoc(), D->getEndLoc());
    getSema().EndOpenMPDSABlock(Res.get());
    if (!Res.isInvalid())
      D->setOMPNode(Res.get());
    return Res;
  }

  StmtResult TransformACCParallelDirective(ACCParallelDirective *D) {
    DirStackEntryRAII TheDirStackEntryRAII(*this, D);
    DirStackEntry &DirEntry = DirStack.back();
    ConditionalCompoundStmtRAII EnclosingCompoundStmt(*this);
    ASTContext &Context = getSema().getASTContext();

    auto VectorLengthClauses = D->getClausesOfKind<ACCVectorLengthClause>();
    if (VectorLengthClauses.begin() != VectorLengthClauses.end()) {
      Expr *E = VectorLengthClauses.begin()->getVectorLength();
      if (E->isIntegerConstantExpr(Context))
        DirEntry.VectorLengthExpr = E;
      else if (E->HasSideEffects(Context))
        EnclosingCompoundStmt.addUnusedExpr(E);
    }

    // Start OpenMP DA block.
    getSema().StartOpenMPDSABlock(OMPD_target_teams, DeclarationNameInfo(),
                                  /*CurScope=*/nullptr, D->getBeginLoc());

    // Transform OpenACC clauses.
    llvm::SmallVector<OMPClause *, 16> TClauses;
    size_t TClausesAddedCount = 0;
    size_t TClausesEmptyCount;
    transformACCClauses(D, OMPD_target_teams, TClauses, TClausesEmptyCount);
    if (ACCAsyncClause *C = D->getTheClauseOfKind<ACCAsyncClause>())
      transformACCAsyncClauseToOMPClauses(D, C, TClauses, TClausesAddedCount);

    // Transform associated statement.
    StmtResult AssociatedStmt = transformACCAssociatedStmt(D, OMPD_target_teams,
                                                           TClauses);

    // Build OpenMP directive and finalize enclosing compound statement, if
    // any.
    StmtResult Res;
    if (AssociatedStmt.isInvalid() ||
        TClauses.size() != D->clauses().size() - TClausesEmptyCount +
                           TClausesAddedCount)
      Res = StmtError();
    else
      Res = getDerived().RebuildOMPExecutableDirective(
          OMPD_target_teams, DeclarationNameInfo(), OMPD_unknown, TClauses,
          AssociatedStmt.get(), D->getBeginLoc(), D->getEndLoc());
    getSema().EndOpenMPDSABlock(Res.get());
    transformACCAsyncClauseToTrailingOMPTaskwait(D, EnclosingCompoundStmt, Res);
    if (!Res.isInvalid())
      D->setOMPNode(Res.get());
    return Res;
  }

  StmtResult transformACCTileToOMPTile(ACCLoopDirective *D, ACCTileClause *C) {
    if (getSema().isStrictlyNestedInOpenMPTeamsRegion()) {
      getSema().Diag(D->getBeginLoc(), diag::warn_acc_omp_tile_in_teams)
          << D->getSourceRange();
      getSema().Diag(D->getBeginLoc(), diag::note_acc_disable_diag)
          << DiagnosticIDs::getWarningOptionForDiag(
                 diag::warn_acc_omp_tile_in_teams);
      getSema().Diag(D->getBeginLoc(), diag::note_acc_disable_all_omp_ext_diags)
          << DiagnosticIDs::getWarningOptionForGroup(
                 diag::Group::OpenACCOMPExt);
    }
    getSema().StartOpenMPDSABlock(OMPD_tile, DeclarationNameInfo(),
                                  /*CurScope=*/nullptr, D->getBeginLoc());
    bool Err = false;
    llvm::SmallVector<OMPClause *, 1> TClauses;
    {
      OpenMPStartEndClauseRAII ClauseRAII(getSema(), OMPC_sizes);
      llvm::SmallVector<Expr *, 16> TSizeExprs;
      for (Expr *SizeExpr : C->sizelist()) {
        ExprResult TSizeExpr;
        if (isa<ACCStarExpr>(SizeExpr) ||
            !SizeExpr->isIntegerConstantExpr(getSema().getASTContext()))
          TSizeExpr = getSema().ActOnIntegerConstant(SizeExpr->getExprLoc(), 1);
        else
          TSizeExpr = getDerived().TransformExpr(SizeExpr);
        if (TSizeExpr.isInvalid())
          Err = true;
        else
          TSizeExprs.push_back(TSizeExpr.get());
      }
      ExplicitClauseLocs L(D, C, C->getLParenLoc());
      OMPClauseResult TSizesClause = getDerived().RebuildOMPSizesClause(
          TSizeExprs, L.LParenLoc, L.LParenLoc, L.LocEnd);
      if (TSizesClause.isInvalid())
        Err = true;
      else
        TClauses.push_back(TSizesClause.get());
    }
    StmtResult AssociatedStmt =
        transformACCAssociatedStmt(D, OMPD_tile, TClauses);
    if (AssociatedStmt.isInvalid())
      Err = true;
    StmtResult Res;
    if (Err)
      Res = StmtError();
    else
      Res = getDerived().RebuildOMPExecutableDirective(
          OMPD_tile, DeclarationNameInfo(), OMPD_unknown, TClauses,
          AssociatedStmt.get(), C->getBeginLoc(), C->getEndLoc());
    getSema().EndOpenMPDSABlock(Res.get());
    return Res;
  }

  StmtResult TransformACCLoopDirective(ACCLoopDirective *D) {
    DirStackEntryRAII TheDirStackEntryRAII(*this, D);
    DirStackEntry &DirEntry = DirStack.back();

    // What OpenACC level-of-parallelism clauses do we have?
    ACCPartitioningKind Partitioning = D->getPartitioning();

    // OpenACC 3.3, sec. 2.9.8 "tile clause", L2189-2192:
    // "If the vector clause appears on the loop construct, the vector clause is
    // applied to the element loops. If the gang clause appears on the loop
    // construct, the gang clause is applied to the tile loops. If the worker
    // clause appears on the loop construct, the worker clause is applied to the
    // element loops if no vector clause appears, and to the tile loops
    // otherwise."
    //
    // If there's a tile clause, Partitioning is for the tile loops.  Remove the
    // levels of parallelism that apply for the element loops.  TODO: Currently,
    // OpenMP's tile construct gives us no way to apply those to the element
    // loops, so we just discard them entirely.
    ACCTileClause *TileClause = D->getTheClauseOfKind<ACCTileClause>();
    if (TileClause) {
      if (!Partitioning.hasVectorPartitioning())
        Partitioning.removeWorker();
      Partitioning.removeVector();
    }

    // What kind of OpenMP directive should we build?
    // OMPD_unknown means none (so sequential).
    OpenMPDirectiveKind TDKind;
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
      if (Partitioning.hasVectorPartitioning()) {
        AddSimdlenExpr = DirEntry.VectorLengthExpr;
        AddScopeWithLCVPrivate = true;
      }
      if (!Partitioning.hasGangPartitioning()) {
        if (!Partitioning.hasWorkerPartitioning()) {
          if (!Partitioning.hasVectorPartitioning()) {
            // sequential loop
            TDKind = OMPD_unknown;
            AddScopeWithAllPrivates = true;
          } else { // hasVectorPartitioning
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
          // SemaOpenACC.cpp reports an error for any member expression or
          // subarray in a private clause on a loop, and the ACCDataVar
          // constructor call below just asserts that VR is not one of those.
          //
          // See the section "Basic Data Attributes" in the Clang OpenACC design
          // document for a discussion of how the following implementation is a
          // reason we don't permit member expressions in private clauses.
          ACCDataVar Var(VR, ACCDataVar::AllowMemberExprNone,
                         /*AllowSubarray=*/false);
          VarDecl *VD =
              cast<VarDecl>(Var.getReferencedDecl())->getCanonicalDecl();
          if (AddScopeWithAllPrivates || LCVSet.count(VD))
            EnclosingCompoundStmt.addPrivatizingDecl(VR->getBeginLoc(),
                                                     VR->getEndLoc(), VD);
        }
      }
    }

    // Handle case of no OpenMP loop-partitioning directive.
    if (TDKind == OMPD_unknown) {
      Sema::CompoundScopeRAII CompoundScope(getSema());
      StmtResult Res;
      if (TileClause)
        Res = transformACCTileToOMPTile(D, TileClause);
      else
        Res = getDerived().TransformStmt(D->getAssociatedStmt());
      EnclosingCompoundStmt.finalize(Res);
      if (!Res.isInvalid())
        D->setOMPNode(Res.get(), /*OMPDirectiveCount=*/TileClause ? 1 : 0);
      return Res;
    }

    // Start OpenMP DA block.
    getSema().StartOpenMPDSABlock(TDKind, DeclarationNameInfo(),
                                  /*CurScope=*/nullptr, D->getBeginLoc());

    // Add simdlen clause if needed.
    llvm::SmallVector<OMPClause *, 16> TClauses;
    size_t TClausesAddedCount = 0;
    if (AddSimdlenExpr) {
      OpenMPStartEndClauseRAII ClauseRAII(getSema(), OMPC_simdlen);
      TClauses.push_back(getDerived().RebuildOMPSimdlenClause(
          AddSimdlenExpr, AddSimdlenExpr->getBeginLoc(),
          AddSimdlenExpr->getBeginLoc(), AddSimdlenExpr->getEndLoc()));
      ++TClausesAddedCount;
    }

    // Transform OpenACC clauses.
    size_t TClausesEmptyCount;
    transformACCClauses(D, TDKind, TClauses, TClausesEmptyCount);

    // Transform associated statement.
    StmtResult AssociatedStmt;
    if (TileClause) {
      getSema().ActOnOpenMPRegionStart(TDKind, /*CurScope=*/nullptr);
      AssociatedStmt = transformACCTileToOMPTile(D, TileClause);
      AssociatedStmt = getSema().ActOnOpenMPRegionEnd(AssociatedStmt, TClauses);
    } else {
      AssociatedStmt = transformACCAssociatedStmt(D, TDKind, TClauses);
    }

    // Build OpenMP directive and finalize enclosing compound statement, if
    // any.
    StmtResult Res;
    if (AssociatedStmt.isInvalid() ||
        TClauses.size() != D->clauses().size() - TClausesEmptyCount +
                           TClausesAddedCount)
      Res = StmtError();
    else
      Res = getDerived().RebuildOMPExecutableDirective(
          TDKind, DeclarationNameInfo(), OMPD_unknown, TClauses,
          AssociatedStmt.get(), D->getBeginLoc(), D->getEndLoc());
    getSema().EndOpenMPDSABlock(Res.get());
    EnclosingCompoundStmt.finalize(Res);
    if (!Res.isInvalid())
      D->setOMPNode(Res.get(), /*OMPDirectiveCount=*/TileClause ? 2 : 1);
    return Res;
  }

  StmtResult TransformACCParallelLoopDirective(ACCParallelLoopDirective *D) {
    Stmt *Effective = D->getEffectiveDirective();
    StmtResult Res =
        TransformACCParallelDirective(cast<ACCParallelDirective>(Effective));
    if (Res.isInvalid())
      return Res;
    int OMPDirectiveCount = 0;
    for (int I = 0, E = getOpenACCEffectiveDirectives(D->getDirectiveKind());
         I < E; ++I) {
      ACCDirectiveStmt *ACCEffective = cast<ACCDirectiveStmt>(Effective);
      OMPDirectiveCount += ACCEffective->getOMPDirectiveCount();
      Effective = ACCEffective->getAssociatedStmt();
      assert(Effective && "expected more effective directives");
    }
    D->setOMPNode(Res.get(), OMPDirectiveCount);
    return Res;
  }

  StmtResult TransformACCAtomicDirective(ACCAtomicDirective *D) {
    if (getSema().isStrictlyNestedInOpenMPTeamsRegion()) {
      getSema().Diag(D->getBeginLoc(), diag::warn_acc_omp_atomic_in_teams)
          << D->getSourceRange();
      getSema().Diag(D->getBeginLoc(), diag::note_acc_disable_diag)
          << DiagnosticIDs::getWarningOptionForDiag(
                 diag::warn_acc_omp_atomic_in_teams);
      getSema().Diag(D->getBeginLoc(), diag::note_acc_disable_all_omp_ext_diags)
          << DiagnosticIDs::getWarningOptionForGroup(
                 diag::Group::OpenACCOMPExt);
    }

    DirStackEntryRAII TheDirStackEntryRAII(*this, D);

    // Start OpenMP DA block.
    getSema().StartOpenMPDSABlock(OMPD_atomic, DeclarationNameInfo(),
                                  /*CurScope=*/nullptr, D->getBeginLoc());

    // Transform OpenACC clauses.
    llvm::SmallVector<OMPClause *, 16> TClauses;
    size_t TClausesEmptyCount;
    transformACCClauses(D, OMPD_atomic, TClauses, TClausesEmptyCount);

    // Transform associated statement.
    StmtResult AssociatedStmt =
        transformACCAssociatedStmt(D, OMPD_atomic, TClauses);

    // Build OpenMP directive and finalize enclosing compound statement, if
    // any.
    StmtResult Res;
    if (AssociatedStmt.isInvalid() ||
        TClauses.size() != D->clauses().size() - TClausesEmptyCount)
      Res = StmtError();
    else
      Res = getDerived().RebuildOMPExecutableDirective(
          OMPD_atomic, DeclarationNameInfo(), OMPD_unknown, TClauses,
          AssociatedStmt.get(), D->getBeginLoc(), D->getEndLoc());
    getSema().EndOpenMPDSABlock(Res.get());
    if (!Res.isInvalid())
      D->setOMPNode(Res.get());
    return Res;
  }

  OMPClauseResult TransformACCClause(ACCDirectiveStmt *D,
                                     OpenMPDirectiveKind TDKind, ACCClause *C) {
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

  OMPClauseResult TransformACCNumGangsClause(ACCDirectiveStmt *D,
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

  OMPClauseResult TransformACCNumWorkersClause(ACCDirectiveStmt *D,
                                               OpenMPDirectiveKind TDKind,
                                               ACCNumWorkersClause *C) {
    ExplicitClauseLocs L(D, C, C->getLParenLoc());
    return getDerived().RebuildOMPThreadLimitClause(
        C->getNumWorkers(), L.LocStart, L.LParenLoc, L.LocEnd);
  }

  OMPClauseResult TransformACCVectorLengthClause(ACCDirectiveStmt *D,
                                                 OpenMPDirectiveKind TDKind,
                                                 ACCVectorLengthClause *C) {
    return OMPClauseEmpty();
  }

  OMPClauseResult TransformACCCollapseClause(ACCDirectiveStmt *D,
                                             OpenMPDirectiveKind TDKind,
                                             ACCCollapseClause *C) {
    ExplicitClauseLocs L(D, C, C->getLParenLoc());
    return getDerived().RebuildOMPCollapseClause(
        C->getCollapse(), L.LocStart, L.LParenLoc, L.LocEnd);
  }

  OMPClauseResult TransformACCTileClause(ACCDirectiveStmt *D,
                                         OpenMPDirectiveKind TDKind,
                                         ACCTileClause *C) {
    if (C->sizelist_size() == 1)
      return OMPClauseEmpty();
    ExplicitClauseLocs L(D, C, C->getLParenLoc());
    ExprResult Num =
        getSema().ActOnIntegerConstant(L.LocStart, C->sizelist_size());
    if (Num.isInvalid())
      return OMPClauseError();
    return getDerived().RebuildOMPCollapseClause(Num.get(), L.LocStart,
                                                 L.LParenLoc, L.LocEnd);
  }

  OMPClauseResult TransformACCAsyncClause(ACCDirectiveStmt *D,
                                          OpenMPDirectiveKind TDKind,
                                          ACCAsyncClause *C) {
    return OMPClauseEmpty();
  }

  OMPClauseResult TransformACCWaitClause(ACCDirectiveStmt *D,
                                         OpenMPDirectiveKind TDKind,
                                         ACCWaitClause *C) {
    // TODO: Actually translate it.  Currently, this should be reachable only
    // if -fopenacc-fake-async-wait.
    return OMPClauseEmpty();
  }

  OMPClauseResult TransformACCNomapClause(ACCDirectiveStmt *D,
                                          OpenMPDirectiveKind TDKind,
                                          ACCNomapClause *C) {
    bool NoCreateOMPNoAlloc;
    switch (getSema().LangOpts.getOpenACCNoCreateOMP()) {
    case LangOptions::OpenACCNoCreateOMP_OmpxNoAlloc:
      NoCreateOMPNoAlloc = true;
      break;
    case LangOptions::OpenACCNoCreateOMP_NoOmpxNoAlloc:
      NoCreateOMPNoAlloc = false;
      break;
    }
    SmallVector<OpenMPMapModifierKind, 1> MapMods;
    return transformACCVarListClause<ACCNomapClause>(
        D, C, OMPC_map,
        [&](Expr *RefExpr) {
          ACCDataVar Var(RefExpr);
          const DAVarData &VarDAs = DirStack.back().DAMap[Var];
          assert(VarDAs.DMAKind == ACC_DMA_nomap &&
                 "expected nomap clauses to record ACC_DMA_nomap");
          if (VarDAs.DSAKind != ACC_DSA_shared)
            return VAR_DISCARD;
          assert(DirStack.size() > 1 &&
                 "expected visible DMA at parent for nomap/shared var");
          const DAVarData &VarParentDAs = (DirStack.rbegin() + 1)->DAMap[Var];
          assert(VarParentDAs.VisibleDMAKind != ACC_DMA_unknown &&
                 VarParentDAs.VisibleDMARefExpr &&
                 "expected visible DMA at parent for nomap/shared var");
          if (VarParentDAs.VisibleDMAKind == ACC_DMA_no_create &&
              NoCreateOMPNoAlloc) {
            if (MapMods.empty())
              MapMods.push_back(OMPC_MAP_MODIFIER_ompx_no_alloc);
            getSema().Diag(D->getEndLoc(),
                           diag::warn_acc_omp_map_ompx_no_alloc_from_visible)
                << getOpenACCName(C->getClauseKind());
          }
          if (isa<OMPArraySectionExpr>(
                  VarParentDAs.VisibleDMARefExpr->IgnoreParenImpCasts()) &&
              !Var.isCXXThis())
            return VAR_ADD_ZERO_LENGTH_ARRAY_SECTION;
          return VAR_PRESERVE;
        },
        [&](ArrayRef<Expr *> Vars, const ExplicitClauseLocs &L) {
          SmallVector<SourceLocation, 1> MapModLocs;
          return getDerived().RebuildOMPMapClause(
              /*IteratorModifier=*/nullptr, MapMods, {L.LParenLoc},
              CXXScopeSpec(), DeclarationNameInfo(), OMPC_MAP_alloc,
              /*IsMapTypeImplicit=*/false, L.LocStart, L.LParenLoc, Vars,
              OMPVarListLocTy(L.LocStart, L.LParenLoc, L.LocEnd), std::nullopt);
        });
  }

  OMPClauseResult TransformACCPresentClause(ACCDirectiveStmt *D,
                                            OpenMPDirectiveKind TDKind,
                                            ACCPresentClause *C) {
    SmallVector<OpenMPMapModifierKind, 2> MapMods;
    switch (getSema().LangOpts.getOpenACCPresentOMP()) {
    case LangOptions::OpenACCPresentOMP_Present:
      MapMods.push_back(OMPC_MAP_MODIFIER_present);
      getSema().Diag(C->getBeginLoc(), diag::warn_acc_omp_map_present)
          << getOpenACCName(C->getClauseKind())
          << C->getSourceRange();
      getSema().Diag(C->getBeginLoc(), diag::note_acc_alternate_omp)
          << getOpenACCName(C->getClauseKind())
          << LangOptions::getOpenACCPresentOMPValue(
              LangOptions::OpenACCPresentOMP_NoPresent);
      getSema().Diag(C->getBeginLoc(), diag::note_acc_disable_diag)
          << DiagnosticIDs::getWarningOptionForDiag(
              diag::warn_acc_omp_map_present);
      getSema().Diag(C->getBeginLoc(), diag::note_acc_disable_all_omp_ext_diags)
          << DiagnosticIDs::getWarningOptionForGroup(
                 diag::Group::OpenACCOMPExt);
      break;
    case LangOptions::OpenACCPresentOMP_NoPresent:
      break;
    }
    addHoldMapTypeModifier(D, C, MapMods);
    return transformACCVarListClause<ACCPresentClause>(
        D, C, OMPC_map,
        [&](ArrayRef<Expr *> Vars, const ExplicitClauseLocs &L) {
          SmallVector<SourceLocation, 2> MapModLocs;
          for (int i = 0, e = MapMods.size(); i < e; ++i)
            MapModLocs.push_back(L.LParenLoc);
          return getDerived().RebuildOMPMapClause(
              /*IteratorModifier=*/nullptr, MapMods, MapModLocs, CXXScopeSpec(),
              DeclarationNameInfo(), OMPC_MAP_alloc,
              /*IsMapTypeImplicit=*/false, L.LocStart, L.LParenLoc, Vars,
              OMPVarListLocTy(L.LocStart, L.LParenLoc, L.LocEnd), std::nullopt);
        });
  }

  OMPClauseResult TransformACCCopyClause(ACCDirectiveStmt *D,
                                         OpenMPDirectiveKind TDKind,
                                         ACCCopyClause *C) {
    SmallVector<OpenMPMapModifierKind, 1> MapMods;
    addHoldMapTypeModifier(D, C, MapMods);
    return transformACCVarListClause<ACCCopyClause>(
        D, C, OMPC_map,
        [&](ArrayRef<Expr *> Vars, const ExplicitClauseLocs &L) {
          SmallVector<SourceLocation, 1> MapModLocs;
          for (int i = 0, e = MapMods.size(); i < e; ++i)
            MapModLocs.push_back(L.LParenLoc);
          return getDerived().RebuildOMPMapClause(
              /*IteratorModifier=*/nullptr, MapMods, MapModLocs, CXXScopeSpec(),
              DeclarationNameInfo(), OMPC_MAP_tofrom,
              /*IsMapTypeImplicit=*/false, L.LocStart, L.LParenLoc, Vars,
              OMPVarListLocTy(L.LocStart, L.LParenLoc, L.LocEnd), std::nullopt);
        });
  }

  OMPClauseResult TransformACCCopyinClause(ACCDirectiveStmt *D,
                                           OpenMPDirectiveKind TDKind,
                                           ACCCopyinClause *C) {
    SmallVector<OpenMPMapModifierKind, 1> MapMods;
    addHoldMapTypeModifier(D, C, MapMods);
    return transformACCVarListClause<ACCCopyinClause>(
        D, C, OMPC_map,
        [&](ArrayRef<Expr *> Vars, const ExplicitClauseLocs &L) {
          SmallVector<SourceLocation, 1> MapModLocs;
          for (int i = 0, e = MapMods.size(); i < e; ++i)
            MapModLocs.push_back(L.LParenLoc);
          return getDerived().RebuildOMPMapClause(
              /*IteratorModifier=*/nullptr, MapMods, MapModLocs, CXXScopeSpec(),
              DeclarationNameInfo(), OMPC_MAP_to, /*IsMapTypeImplicit=*/false,
              L.LocStart, L.LParenLoc, Vars,
              OMPVarListLocTy(L.LocStart, L.LParenLoc, L.LocEnd), std::nullopt);
        });
  }

  OMPClauseResult TransformACCCopyoutClause(ACCDirectiveStmt *D,
                                            OpenMPDirectiveKind TDKind,
                                            ACCCopyoutClause *C) {
    SmallVector<OpenMPMapModifierKind, 1> MapMods;
    addHoldMapTypeModifier(D, C, MapMods);
    return transformACCVarListClause<ACCCopyoutClause>(
        D, C, OMPC_map,
        [&](ArrayRef<Expr *> Vars, const ExplicitClauseLocs &L) {
          SmallVector<SourceLocation, 1> MapModLocs;
          for (int i = 0, e = MapMods.size(); i < e; ++i)
            MapModLocs.push_back(L.LParenLoc);
          return getDerived().RebuildOMPMapClause(
              /*IteratorModifier=*/nullptr, MapMods, MapModLocs, CXXScopeSpec(),
              DeclarationNameInfo(), OMPC_MAP_from, /*IsMapTypeImplicit=*/false,
              L.LocStart, L.LParenLoc, Vars,
              OMPVarListLocTy(L.LocStart, L.LParenLoc, L.LocEnd), std::nullopt);
        });
  }

  OMPClauseResult TransformACCCreateClause(ACCDirectiveStmt *D,
                                           OpenMPDirectiveKind TDKind,
                                           ACCCreateClause *C) {
    SmallVector<OpenMPMapModifierKind, 1> MapMods;
    addHoldMapTypeModifier(D, C, MapMods);
    return transformACCVarListClause<ACCCreateClause>(
        D, C, OMPC_map,
        [&](ArrayRef<Expr *> Vars, const ExplicitClauseLocs &L) {
          SmallVector<SourceLocation, 1> MapModLocs;
          for (int i = 0, e = MapMods.size(); i < e; ++i)
            MapModLocs.push_back(L.LParenLoc);
          return getDerived().RebuildOMPMapClause(
              /*IteratorModifier=*/nullptr, MapMods, MapModLocs, CXXScopeSpec(),
              DeclarationNameInfo(), OMPC_MAP_alloc,
              /*IsMapTypeImplicit=*/false, L.LocStart, L.LParenLoc, Vars,
              OMPVarListLocTy(L.LocStart, L.LParenLoc, L.LocEnd), std::nullopt);
        });
  }

  OMPClauseResult TransformACCNoCreateClause(ACCDirectiveStmt *D,
                                             OpenMPDirectiveKind TDKind,
                                             ACCNoCreateClause *C) {
    SmallVector<OpenMPMapModifierKind, 2> MapMods;
    switch (getSema().LangOpts.getOpenACCNoCreateOMP()) {
    case LangOptions::OpenACCNoCreateOMP_OmpxNoAlloc:
      MapMods.push_back(OMPC_MAP_MODIFIER_ompx_no_alloc);
      getSema().Diag(C->getBeginLoc(), diag::warn_acc_omp_map_ompx_no_alloc)
          << getOpenACCName(C->getClauseKind())
          << C->getSourceRange();
      getSema().Diag(C->getBeginLoc(), diag::note_acc_alternate_omp)
          << "no-create"
          << LangOptions::getOpenACCNoCreateOMPValue(
              LangOptions::OpenACCNoCreateOMP_NoOmpxNoAlloc);
      getSema().Diag(C->getBeginLoc(), diag::note_acc_disable_diag)
          << DiagnosticIDs::getWarningOptionForDiag(
              diag::warn_acc_omp_map_ompx_no_alloc);
      getSema().Diag(C->getBeginLoc(), diag::note_acc_disable_all_omp_ext_diags)
          << DiagnosticIDs::getWarningOptionForGroup(
                 diag::Group::OpenACCOMPExt);
      break;
    case LangOptions::OpenACCNoCreateOMP_NoOmpxNoAlloc:
      break;
    }
    addHoldMapTypeModifier(D, C, MapMods);
    return transformACCVarListClause<ACCNoCreateClause>(
        D, C, OMPC_map,
        [&](ArrayRef<Expr *> Vars, const ExplicitClauseLocs &L) {
          SmallVector<SourceLocation, 1> MapModLocs;
          for (int i = 0, e = MapMods.size(); i < e; ++i)
            MapModLocs.push_back(L.LParenLoc);
          return getDerived().RebuildOMPMapClause(
              /*IteratorModifier=*/nullptr, MapMods, MapModLocs, CXXScopeSpec(),
              DeclarationNameInfo(), OMPC_MAP_alloc,
              /*IsMapTypeImplicit=*/false, L.LocStart, L.LParenLoc, Vars,
              OMPVarListLocTy(L.LocStart, L.LParenLoc, L.LocEnd), std::nullopt);
        });
  }

  OMPClauseResult TransformACCDeleteClause(ACCDirectiveStmt *D,
                                           OpenMPDirectiveKind TDKind,
                                           ACCDeleteClause *C) {
    return transformACCVarListClause<ACCDeleteClause>(
        D, C, OMPC_map,
        [&](ArrayRef<Expr *> Vars, const ExplicitClauseLocs &L) {
          return getDerived().RebuildOMPMapClause(
              /*IteratorModifier=*/nullptr, std::nullopt, std::nullopt,
              CXXScopeSpec(), DeclarationNameInfo(), OMPC_MAP_release,
              /*IsMapTypeImplicit=*/false, L.LocStart, L.LParenLoc, Vars,
              OMPVarListLocTy(L.LocStart, L.LParenLoc, L.LocEnd), std::nullopt);
        });
  }

  OMPClauseResult TransformACCSharedClause(ACCDirectiveStmt *D,
                                           OpenMPDirectiveKind TDKind,
                                           ACCSharedClause *C) {
    return OMPClauseEmpty();
  }

  OMPClauseResult TransformACCPrivateClause(ACCDirectiveStmt *D,
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
    llvm::DenseSet<ACCDataVar> SkipVars;
    if (isOpenMPSimdDirective(TDKind)) {
      ACCLoopDirective *LD = dyn_cast_or_null<ACCLoopDirective>(D);
      assert(LD && "expected omp simd directive to translate from acc loop"
                   " directive");
      const ArrayRef<VarDecl *> &LCVArray = LD->getLoopControlVariables();
      SkipVars.reserve(LCVArray.size());
      for (VarDecl *VD : LCVArray)
        SkipVars.insert(ACCDataVar(VD));
    }
    return transformACCVarListClause<ACCPrivateClause>(
        D, C, OMPC_private,
        [&SkipVars](Expr *RefExpr) {
          return SkipVars.count(ACCDataVar(RefExpr)) ? VAR_DISCARD
                                                     : VAR_PRESERVE;
        },
        OMPVarListClauseRebuilder(this,
                                  &TransformACCToOMP::RebuildOMPPrivateClause));
  }

  OMPClauseResult TransformACCFirstprivateClause(ACCDirectiveStmt *D,
                                                 OpenMPDirectiveKind TDKind,
                                                 ACCFirstprivateClause *C) {
    return transformACCVarListClause<ACCFirstprivateClause>(
        D, C, OMPC_firstprivate,
        OMPVarListClauseRebuilder(
            this, &TransformACCToOMP::RebuildOMPFirstprivateClause));
  }

  OMPClauseResult TransformACCReductionClause(ACCDirectiveStmt *D,
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
              Vars, OMPC_REDUCTION_unknown, L.LocStart, L.LParenLoc,
              /*ModifierLoc=*/SourceLocation(), C->getColonLoc(), L.LocEnd,
              Spec, C->getNameInfo(), ArrayRef<Expr *>());
        });
  }

  OMPClauseResult TransformACCIfClause(ACCDirectiveStmt *D,
                                       OpenMPDirectiveKind TDKind,
                                       ACCIfClause *C) {
    OpenMPStartEndClauseRAII ClauseRAII(getSema(), OMPC_if);
    ExprResult E = getDerived().TransformExpr(C->getCondition());
    if (E.isInvalid())
      return OMPClauseError();
    ExplicitClauseLocs L(D, C, C->getLParenLoc());
    return getDerived().RebuildOMPIfClause(OMPD_unknown, E.get(), L.LocStart,
                                           L.LParenLoc, L.LParenLoc,
                                           L.LParenLoc, L.LocEnd);
  }

  OMPClauseResult TransformACCIfPresentClause(ACCDirectiveStmt *D,
                                              OpenMPDirectiveKind TDKind,
                                              ACCIfPresentClause *C) {
    return OMPClauseEmpty();
  }

private:
  template <typename ClauseType>
  OMPClauseResult TransformACCSelfOrDeviceClause(
      ACCDirectiveStmt *D, OpenMPDirectiveKind TDKind, ClauseType *C,
      OpenMPClauseKind TCKind,
      OMPClause *(TransformACCToOMP::*Rebuilder)(
          ArrayRef<OpenMPMotionModifierKind> MotionMods,
          ArrayRef<SourceLocation> MotionModLocs,
          CXXScopeSpec &MapperIdScopeSpec, DeclarationNameInfo &MapperId,
          SourceLocation ColonLoc, ArrayRef<Expr *> VarList,
          const OMPVarListLocTy &Locs, ArrayRef<Expr *> UnresolvedMappers)) {
    SmallVector<OpenMPMotionModifierKind, 1> MotionMods;
    switch (getSema().LangOpts.getOpenACCUpdatePresentOMP()) {
    case LangOptions::OpenACCUpdatePresentOMP_Present:
      if (!D->hasClausesOfKind<ACCIfPresentClause>()) {
        MotionMods.push_back(OMPC_MOTION_MODIFIER_present);
        getSema().Diag(C->getBeginLoc(), diag::warn_acc_omp_update_present)
            << getOpenACCName(C->getClauseKind()) << C->getSourceRange();
        getSema().Diag(C->getBeginLoc(), diag::note_acc_alternate_omp)
            << (std::string(getOpenACCName(D->getDirectiveKind())) + "-present")
            << LangOptions::getOpenACCUpdatePresentOMPValue(
                   LangOptions::OpenACCUpdatePresentOMP_NoPresent);
        getSema().Diag(C->getBeginLoc(), diag::note_acc_disable_diag)
            << DiagnosticIDs::getWarningOptionForDiag(
                   diag::warn_acc_omp_update_present);
        getSema().Diag(C->getBeginLoc(),
                       diag::note_acc_disable_all_omp_ext_diags)
            << DiagnosticIDs::getWarningOptionForGroup(
                   diag::Group::OpenACCOMPExt);
      }
      break;
    case LangOptions::OpenACCUpdatePresentOMP_NoPresent:
      break;
    }
    return transformACCVarListClause<ClauseType>(
        D, C, TCKind, [&](ArrayRef<Expr *> Vars, const ExplicitClauseLocs &L) {
          SmallVector<SourceLocation, 1> MotionModLocs;
          for (int i = 0, e = MotionMods.size(); i < e; ++i)
            MotionModLocs.push_back(L.LParenLoc);
          CXXScopeSpec MapperIdScopeSpec;
          DeclarationNameInfo MapperIdInfo;
          return (getDerived().*Rebuilder)(
              MotionMods, MotionModLocs, MapperIdScopeSpec, MapperIdInfo,
              L.LParenLoc, Vars,
              OMPVarListLocTy(L.LocStart, L.LParenLoc, L.LocEnd), std::nullopt);
        });
  }

public:
  OMPClauseResult TransformACCSelfClause(ACCDirectiveStmt *D,
                                         OpenMPDirectiveKind TDKind,
                                         ACCSelfClause *C) {
    return TransformACCSelfOrDeviceClause(
        D, TDKind, C, OMPC_from, &TransformACCToOMP::RebuildOMPFromClause);
  }

  OMPClauseResult TransformACCDeviceClause(ACCDirectiveStmt *D,
                                           OpenMPDirectiveKind TDKind,
                                           ACCDeviceClause *C) {
    return TransformACCSelfOrDeviceClause(
        D, TDKind, C, OMPC_to, &TransformACCToOMP::RebuildOMPToClause);
  }

  OMPClauseResult TransformACCSeqClause(ACCDirectiveStmt *D,
                                        OpenMPDirectiveKind TDKind,
                                        ACCSeqClause *C) {
    return OMPClauseEmpty();
  }

  OMPClauseResult TransformACCIndependentClause(ACCDirectiveStmt *D,
                                                OpenMPDirectiveKind TDKind,
                                                ACCIndependentClause *C) {
    return OMPClauseEmpty();
  }

  OMPClauseResult TransformACCAutoClause(ACCDirectiveStmt *D,
                                         OpenMPDirectiveKind TDKind,
                                         ACCAutoClause *C) {
    return OMPClauseEmpty();
  }

  OMPClauseResult TransformACCGangClause(ACCDirectiveStmt *D,
                                         OpenMPDirectiveKind TDKind,
                                         ACCGangClause *C) {
    Expr *StaticArg = C->getStaticArg();
    if (!StaticArg)
      return OMPClauseEmpty();
    OpenMPStartEndClauseRAII ClauseRAII(getSema(), OMPC_dist_schedule);
    ExplicitClauseLocs L(D, C, C->getLParenLoc());
    Expr *OMPChunkSize;
    if (isa<ACCStarExpr>(StaticArg))
      OMPChunkSize = nullptr;
    else {
      ExprResult E = getDerived().TransformExpr(StaticArg);
      if (E.isInvalid())
        return OMPClauseError();
      OMPChunkSize = E.get();
    }
    return getDerived().RebuildOMPDistScheduleClause(
        OMPC_DIST_SCHEDULE_static, OMPChunkSize, L.LocStart, L.LParenLoc,
        C->getStaticKwLoc(), C->getStaticColonLoc(), L.LocEnd);
  }

  OMPClauseResult TransformACCWorkerClause(ACCDirectiveStmt *D,
                                           OpenMPDirectiveKind TDKind,
                                           ACCWorkerClause *C) {
    return OMPClauseEmpty();
  }

  OMPClauseResult TransformACCVectorClause(ACCDirectiveStmt *D,
                                           OpenMPDirectiveKind TDKind,
                                           ACCVectorClause *C) {
    return OMPClauseEmpty();
  }

  OMPClauseResult TransformACCReadClause(ACCDirectiveStmt *D,
                                         OpenMPDirectiveKind TDKind,
                                         ACCReadClause *C) {
    ExplicitClauseLocs L(D, C);
    return getSema().ActOnOpenMPReadClause(L.LocStart, L.LocEnd);
  }

  OMPClauseResult TransformACCWriteClause(ACCDirectiveStmt *D,
                                          OpenMPDirectiveKind TDKind,
                                          ACCWriteClause *C) {
    ExplicitClauseLocs L(D, C);
    return getSema().ActOnOpenMPWriteClause(L.LocStart, L.LocEnd);
  }

  OMPClauseResult TransformACCUpdateClause(ACCDirectiveStmt *D,
                                           OpenMPDirectiveKind TDKind,
                                           ACCUpdateClause *C) {
    ExplicitClauseLocs L(D, C);
    return getSema().ActOnOpenMPUpdateClause(L.LocStart, L.LocEnd);
  }

  OMPClauseResult TransformACCCaptureClause(ACCDirectiveStmt *D,
                                            OpenMPDirectiveKind TDKind,
                                            ACCCaptureClause *C) {
    ExplicitClauseLocs L(D, C);
    return getSema().ActOnOpenMPCaptureClause(L.LocStart, L.LocEnd);
  }

  OMPClauseResult TransformACCCompareClause(ACCDirectiveStmt *D,
                                            OpenMPDirectiveKind TDKind,
                                            ACCCompareClause *C) {
    ExplicitClauseLocs L(D, C);
    return getSema().ActOnOpenMPCompareClause(L.LocStart, L.LocEnd);
  }

  using InheritableAttrResult = ActionResult<InheritableAttr *>;
  inline InheritableAttrResult InheritableAttrEmpty() {
    return InheritableAttrResult(false);
  }
  inline InheritableAttrResult InheritableAttrError() {
    return InheritableAttrResult(true);
  }

  InheritableAttrResult TransformACCAttrToOMP(ACCDeclAttr *ACCAttr,
                                              FunctionDecl *FD) {
    if (!ACCAttr)
      return InheritableAttrError();
    switch (ACCAttr->getKind()) {
    case attr::ACCRoutineDecl:
      return getDerived().TransformACCRoutineDeclAttrToOMP(
          cast<ACCRoutineDeclAttr>(ACCAttr), FD);
    default:
      llvm_unreachable("expected OpenACC attribute");
      return InheritableAttrError();
    }
  }

  InheritableAttrResult
  TransformACCRoutineDeclAttrToOMP(ACCRoutineDeclAttr *ACCAttr,
                                   FunctionDecl *FD) {
    assert(FD->getAttr<ACCRoutineDeclAttr>() == ACCAttr &&
           "expected Attr to be attached to FunctionDecl");
    if (ACCAttr->isImplicit())
      return InheritableAttrEmpty();
    // Clang currently doesn't support 'omp declare target' at the statement
    // level of a function.  Where FD is used in a compute region, 'omp declare
    // target' is implied anyway.
    // TODO: If FD is defined in this compilation unit within the scope of the
    // 'acc routine' directive, that definition might not be compiled for the
    // target device as it should, but the OpenACC programmer would likely have
    // placed the ACCAttr on FD's definition rather than bury it in a different
    // function's definition anyway.  See the Clang OpenACC design document for
    // a more detailed discussion.
    if (FD->getLexicalDeclContext()->isFunctionOrMethod())
      return InheritableAttrEmpty();
    ASTContext &Context = getSema().getASTContext();
    OMPDeclareTargetDeclAttr *OMPAttr = OMPDeclareTargetDeclAttr::Create(
        Context, OMPDeclareTargetDeclAttr::MT_To,
        OMPDeclareTargetDeclAttr::DT_Any, /*IndirectExpr=*/nullptr,
        /*Indirect=*/false, -1, /*IsOpenACCTranslation=*/true,
        ACCAttr->getRange());
    return OMPAttr;
  }
};
} // namespace

Stmt *Sema::transformACCToOMP(FunctionDecl *FD, Stmt *Body,
                              bool HasOrphanedLoopConstructImplier) {
  if (!HasOrphanedLoopConstructImplier)
    return Body;
  if (getDiagnostics().hasErrorOccurred())
    return Body;
  // Checking isInOpenACCDirectiveStmt isn't strictly necessary, but there's no
  // point in running this if transformACCToOMP on every ACCDirectiveStmt will
  // skip transforming to OpenMP.
  if (isInOpenACCDirectiveStmt())
    return Body;
  class TransformFnBody : public TreeTransform<TransformFnBody> {
    typedef TreeTransform<TransformFnBody> BaseTransform;

  public:
    TransformFnBody(Sema &SemaRef) : BaseTransform(SemaRef) {}
    StmtResult TransformACCDirectiveStmt(ACCDirectiveStmt *D) {
      if (SemaRef.transformACCToOMP(cast<ACCDirectiveStmt>(D)))
        return StmtError();
      return D;
    }
  };
  StmtResult Res = TransformFnBody(*this).TransformStmt(Body);
  if (!Res.isInvalid())
    return Res.get();
  return Body;
}

bool Sema::transformACCToOMP(ACCDirectiveStmt *D) {
  if (getDiagnostics().hasErrorOccurred())
    return false;
  if (isInOpenACCDirectiveStmt())
    return false;
  FunctionDecl *CurFn = getCurFunctionDecl(/*AllowLambda=*/true);
  if (isa<ACCLoopDirective>(D) && !CurFn->hasAttr<ACCRoutineDeclAttr>() &&
      !CurFn->hasBody())
    return false;
  StartOpenACCTransform();
  bool Res = TransformACCToOMP(*this).TransformStmt(D).isInvalid();
  EndOpenACCTransform();
  return Res;
}

void Sema::transformACCToOMP(ACCDeclAttr *ACCAttr, FunctionDecl *FD) {
  TransformACCToOMP::InheritableAttrResult OMPAttrResult =
      TransformACCToOMP(*this).TransformACCAttrToOMP(ACCAttr, FD);
  if (OMPAttrResult.isInvalid())
    return;
  if (OMPAttrResult.isUnset()) {
    ACCAttr->setOMPNode(FD, /*OMPNode=*/nullptr,
                        /*DirectiveDiscardedForOMP=*/true);
    return;
  }
  FD->addAttr(OMPAttrResult.get());
  ACCAttr->setOMPNode(FD, OMPAttrResult.get(),
                      /*DirectiveDiscardedForOMP=*/false);
}
