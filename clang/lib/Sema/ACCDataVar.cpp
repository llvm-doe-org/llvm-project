//===------ ACCDataVar.cpp - OpenACC data variable --------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
//
// This file implements ACCDataVar, which identifies a variable for which Clang
// analyzes OpenACC data attributes.
//
//===----------------------------------------------------------------------===//

#include "ACCDataVar.h"

using namespace clang;

ACCDataVar::ACCDataVar(Expr *Ref, AllowMemberExprTy AllowMemberExpr,
                       bool AllowSubarray, Sema *S, bool Quiet,
                       OpenACCDirectiveKind RealDKind, OpenACCClauseKind CKind)
    : CompositeDecl(nullptr), ReferencedDecl(nullptr), ReferencedType() {
  // After diagnosing an error in Ref, it's sometimes possible to recover and
  // diagnose additional errors in Ref, but make sure to construct an invalid
  // ACCDataVar in that case.
  class ErrorRAII {
  private:
    ACCDataVar &This;
    Sema *S;
    bool Quiet;
    bool FoundError;

  public:
    ErrorRAII(ACCDataVar &This, Sema *S, bool Quiet)
        : This(This), S(S), Quiet(Quiet), FoundError(false) {}
    Sema::SemaDiagnosticBuilder operator()(SourceLocation Loc,
                                           unsigned int DiagID) {
      FoundError = true;
      if (Quiet)
        return Sema::SemaDiagnosticBuilder(Sema::SemaDiagnosticBuilder::K_Nop,
                                           Loc, DiagID, nullptr, *S);
      return S->Diag(Loc, DiagID);
    }
    ~ErrorRAII() {
      if (FoundError)
        This = ACCDataVar();
    }
  } Diag(*this, S, Quiet);

  // Diagnostics shouldn't claim member expressions on C++ 'this' are
  // permitted if C++ isn't enabled.  If we're not emitting diagnostics (!S),
  // then this adjustment to AllowMemberExpr doesn't matter.  That is, it
  // doesn't affect what is permitted as it should be impossible to see a
  // CXXThisExpr here if C++ isn't enabled.
  if (AllowMemberExpr == AllowMemberExprOnCXXThis && S &&
      !S->getLangOpts().CPlusPlus)
    AllowMemberExpr = AllowMemberExprNone;

  // If it's a subarray, extract the base variable, and complain about any
  // subscript (masquerading as a subarray), for which OpenACC 2.7 doesn't
  // specify a behavior.
  Expr *RefWithoutSubarray = Ref->IgnoreParenImpCasts();
  bool HasSubarray = false;
  if (auto *OASE = dyn_cast_or_null<OMPArraySectionExpr>(RefWithoutSubarray)) {
    HasSubarray = true;
    do {
      if (OASE->getColonLocFirst().isInvalid()) {
        assert(S && "expected subarray with colon");
        Diag(OASE->getRBracketLoc(), diag::err_acc_subarray_without_colon);
      }
      if (!AllowSubarray) {
        assert(S && "unexpected subarray");
        Diag(OASE->getBeginLoc(), diag::err_acc_unsupported_subarray)
            << getOpenACCName(RealDKind) << getOpenACCName(CKind)
            << OASE->getSourceRange();
      }
      RefWithoutSubarray = OASE->getBase()->IgnoreParenImpCasts();
    } while ((OASE = dyn_cast<OMPArraySectionExpr>(RefWithoutSubarray)));
  }

  // Complain about any subscript, for which OpenACC 2.7 doesn't specify a
  // behavior.
  if (auto *ASE = dyn_cast<ArraySubscriptExpr>(RefWithoutSubarray)) {
    assert(S && "unexpected ArraySubscriptExpr");
    Diag(ASE->getRBracketLoc(), diag::err_acc_subarray_without_colon);
    if (!AllowSubarray) {
      Diag(ASE->getBeginLoc(), diag::err_acc_unsupported_subarray)
          << getOpenACCName(RealDKind) << getOpenACCName(CKind)
          << ASE->getSourceRange();
    }
    RefWithoutSubarray = ASE->getBase()->IgnoreParenImpCasts();
  }

  // If a CXXThisExpr was passed in (not as a base of a member expression), it
  // should be in \c this[0:1].  TODO: We don't actually try to verify the
  // subarray bounds here, but maybe we should.
  if (CXXThisExpr *TE = dyn_cast<CXXThisExpr>(RefWithoutSubarray)) {
    if (!HasSubarray) {
      assert(S && "expected CXXThisExpr within subarray");
      Diag(TE->getBeginLoc(), diag::err_acc_expected_data_var)
          << (AllowMemberExpr > AllowMemberExprNone) << AllowSubarray
          << TE->getSourceRange();
    }
    ReferencedType = TE->getType()->getPointeeType();
    return; /*this[0:1]*/
  }

  // Check any member expression.
  MemberExpr *ME = dyn_cast<MemberExpr>(RefWithoutSubarray);
  CXXMemberCallExpr *CME = dyn_cast<CXXMemberCallExpr>(RefWithoutSubarray);
  if (ME || CME) {
    SourceLocation BeginLoc = ME ? ME->getBeginLoc() : CME->getBeginLoc();
    SourceRange SrcRange = ME ? ME->getSourceRange() : CME->getSourceRange();
    Expr *BaseExpr = ME ? ME->getBase() : CME->getImplicitObjectArgument();
    BaseExpr = BaseExpr->IgnoreParenImpCasts();
    bool OnImplicitCXXThis = false;
    if (CXXThisExpr *TE = dyn_cast<CXXThisExpr>(BaseExpr))
      OnImplicitCXXThis = TE->isImplicit();
    if (AllowMemberExpr == AllowMemberExprNone) {
      assert(S && "expected no MemberExpr");
      Diag(BeginLoc, diag::err_acc_unsupported_member_expr)
          << getOpenACCName(RealDKind) << getOpenACCName(CKind)
          << (OnImplicitCXXThis ? 1 : 0) << SrcRange;
    }
    if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(BaseExpr)) {
      if (AllowMemberExpr == AllowMemberExprOnCXXThis) {
        assert(S && "expected MemberExpr only on CXXThisExpr");
        Diag(BeginLoc, diag::err_acc_unsupported_member_expr)
            << getOpenACCName(RealDKind) << getOpenACCName(CKind) << 2
            << SrcRange;
      }
      CompositeDecl = dyn_cast<VarDecl>(DRE->getDecl());
      if (!CompositeDecl) {
        assert(S && "expected VarDecl to be referenced by DeclRefExpr base of "
                    "member expression");
        // Base of member expr cannot be member expr or subarray.
        Diag(BaseExpr->getBeginLoc(), diag::err_acc_expected_data_var)
            << /*allowMemberExpr=*/false << /*allowSubarray=*/false
            << BaseExpr->getSourceRange();
        // TODO: We haven't found a way to reach this case.  If there is, is
        // the above diagnostic clear?
        llvm_unreachable("expected VarDecl to be referenced by DeclRefExpr "
                         "serving as base of MemberExpr");
      }
    } else if (MemberExpr *NestedME = dyn_cast<MemberExpr>(BaseExpr)) {
      // A member expression on implicit 'this' looks like a variable name, so
      // "expected variable name" (the next diagnostic below) would be
      // confusing.  To be more helpful, we use the more specific diagnostic
      // for any nested member expression.
      assert(S && "unexpected nested member expression");
      bool NestedOnImplicitCXXThis = false;
      if (CXXThisExpr *TE =
              dyn_cast<CXXThisExpr>(NestedME->getBase()->IgnoreParenImpCasts()))
        NestedOnImplicitCXXThis = TE->isImplicit();
      Diag(NestedME->getBeginLoc(),
           diag::err_acc_unsupported_member_expr_nested)
          << NestedOnImplicitCXXThis << NestedME->getSourceRange();
    } else if (!isa<CXXThisExpr>(BaseExpr)) {
      assert(S && "expected DeclRefExpr or CXXThisExpr as base of member "
                  "expression");
      // Base of member expr cannot be member expr or subarray.
      Diag(BaseExpr->getBeginLoc(), diag::err_acc_expected_data_var)
          << /*allowMemberExpr=*/false << /*allowSubarray=*/false
          << BaseExpr->getSourceRange();
    }
    ReferencedDecl = ME ? dyn_cast<FieldDecl>(ME->getMemberDecl()) : nullptr;
    if (!ReferencedDecl) {
      assert(S && "expected FieldDecl member of member expression");
      SourceLocation MemberLoc = ME ? ME->getMemberLoc() : CME->getExprLoc();
      if (OnImplicitCXXThis)
        Diag(BeginLoc, diag::err_acc_expected_data_var)
            << (AllowMemberExpr > AllowMemberExprNone) << AllowSubarray
            << SrcRange;
      else
        Diag(MemberLoc, diag::err_acc_expected_data_member) << SrcRange;
    } else {
      ReferencedType = ReferencedDecl->getType();
    }
    return; /*data member*/
  }

  // Check any plain variable name.
  if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(RefWithoutSubarray)) {
    ReferencedDecl = dyn_cast<VarDecl>(DRE->getDecl());
    if (!ReferencedDecl) {
      assert(S && "expected VarDecl to be referenced by DeclRefExpr");
      Diag(RefWithoutSubarray->getBeginLoc(), diag::err_acc_expected_data_var)
          << (AllowMemberExpr > AllowMemberExprNone) << AllowSubarray
          << RefWithoutSubarray->getSourceRange();
      // TODO: We haven't found a way to reach this case.  If there is, is
      // the above diagnostic clear?
      llvm_unreachable("expected VarDecl to be referenced by DeclRefExpr");
    } else {
      ReferencedType = ReferencedDecl->getType();
    }
    return; /*plain variable name*/
  }

  // We have an unexpected expression.  If it's a RecoveryExpr, then there was
  // an error reported somewhere else, so don't risk a spurious diagnostic
  // here.  Either way, do assert that an unexpected expression is encountered
  // only while errors are being diagnosed or ignored (i.e., S != nullptr).
  assert(S && "expected CXXThisExpr, MemberExpr, or DeclRefExpr");
  if (!isa<RecoveryExpr>(RefWithoutSubarray)) {
    Diag(RefWithoutSubarray->getBeginLoc(), diag::err_acc_expected_data_var)
        << (AllowMemberExpr > AllowMemberExprNone) << AllowSubarray
        << RefWithoutSubarray->getSourceRange();
  }
  /*invalid*/
}

bool ACCDataVar::diagnoseConst(Sema &SemaRef, unsigned int DiagID,
                               Expr *RefExpr, OpenACCClauseKind CKind) {
  assert(isValid() && "expected valid ACCDataVar");
  ASTContext &Ctxt = SemaRef.Context;
  ValueDecl *ConstDecl;
  unsigned int NoteDiagID;
  if (ReferencedDecl && ReferencedDecl->getType().isConstant(Ctxt)) {
    ConstDecl = ReferencedDecl;
    NoteDiagID = diag::note_acc_const_var_decl;
  } else if (CompositeDecl && CompositeDecl->getType().isConstant(Ctxt)) {
    // TODO: Check for mutable fields.
    ConstDecl = CompositeDecl;
    NoteDiagID = diag::note_acc_const_var_decl;
  } else if ((isCXXThis() || isMemberOnCXXThis()) &&
             SemaRef.getCurrentThisType()->getPointeeType().isConstant(Ctxt)) {
    // TODO: Check for mutable fields.
    ConstDecl = SemaRef.getCurFunctionDecl();
    NoteDiagID = diag::note_acc_const_var_in_const_member_fn;
  } else {
    return false;
  }
  SemaRef.Diag(RefExpr->getExprLoc(), DiagID)
      << getOpenACCName(CKind) << RefExpr->getSourceRange();
  SemaRef.Diag(ConstDecl->getLocation(), NoteDiagID)
      << NameForDiag(SemaRef, ConstDecl) << ConstDecl->getSourceRange();
  return true;
}
