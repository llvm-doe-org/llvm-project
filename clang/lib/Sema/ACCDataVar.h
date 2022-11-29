//===------ ACCDataVar.h - DeclContext Transformation -----------*- C++ -*-===//
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

#ifndef LLVM_CLANG_LIB_SEMA_ACCDATAVAR_H
#define LLVM_CLANG_LIB_SEMA_ACCDATAVAR_H

#include "clang/Sema/SemaInternal.h"
#include "llvm/ADT/DenseMapInfo.h"

namespace clang {
using namespace sema;
/// Identifies a variable for which Clang analyzes OpenACC data attributes.
///
/// The information recorded in an \c ACCDataVar is not sufficient to determine
/// the data attribute for a variable.  However, given an \c ACCDataVar for each
/// of two different data attributes, that information is sufficient to
/// determine whether the data attributes need to be merged (possibly resulting
/// in a conflict).  In that case, \c DenseMapInfo<ACCDataVar> will compare
/// those \c ACCDataVar objects as equal, and so only one will be stored in a
/// \c DenseMap<ACCDataVar,T> or \c DenseSet<ACCDataVar>.
///
/// An \c ACCDataVar for a subarray expression identifies only the base array
/// or pointer variable and omits the subarray bounds.  Thus, an \c ACCDataVar
/// is not sufficient to determine whether data attributes for a pointer will
/// apply to the pointer as a scalar address or as a subarray, but there
/// ultimately can be data attributes only for one or the other.
///
/// An \c ACCDataVar for a class/struct/union member identifies both the
/// class/struct/union instance and the member.
///
/// Some variables have multiple declarations.  The information stored here
/// includes the declarations that were originally referenced when analyzing
/// some data attribute for the variable.  However, \c DenseMapInfo<ACCDataVar>
/// will treat two \c ACCDataVar objects as equal based on the variables not the
/// specific declarations that happen to be recorded within the \c ACCDataVar
/// objects.
class ACCDataVar final {
  /// Be sure to keep \c DenseMapInfo<ACCDataVar> up to do date with any changes
  /// to the implementation here.
  friend struct llvm::DenseMapInfo<ACCDataVar>;
  ///@{
  /// This \c ACCDataVar identifies either:
  /// - Invalid variable:
  ///   - \c CompositeDecl == \c nullptr.
  ///   - \c ReferencedDecl == \c nullptr.
  /// - A data member of a class/struct/union instance:
  ///   - \c CompositeDecl is the class/struct/union instance's declaration that
  ///     was referenced by the original referencing expression.
  ///   - \c ReferencedDecl is the data member's \c FieldDecl that was
  ///     referenced by the original referencing expression.
  /// - Some other kind of variable declaration:
  ///   - \c CompositeDecl == \c nullptr.
  ///   - \c ReferencedDecl is the variable's \c VarDecl that was referenced by
  ///     the original referencing expression.
  ValueDecl *CompositeDecl;
  ValueDecl *ReferencedDecl;
  ///@}

public:
  /// Construct a placeholder for an invalid variable.
  ACCDataVar() : CompositeDecl(nullptr), ReferencedDecl(nullptr) {}
  /// Construct an \c ACCDataVar from an expression referencing a variable for
  /// which Clang analyzes OpenACC data attributes.
  ///
  /// - \p Ref is valid if it is either a:
  ///   - \c DeclRefExpr that references a variable declaration.
  ///   - \c MemberExpr that references a class/struct/union data member except
  ///     that, if \p AllowMemberExpr is false, such a \p Ref is invalid.
  ///   - \c OMPArraySection whose base is one of the above except that, if
  ///     \p AllowSubarray is false, such a \p Ref is invalid.
  /// - If \p Ref is valid, then \p S, \p Quiet, \p RealDKind, and \p CKind are
  ///   ignored.  Thus, these can be omitted for a \p Ref that was previously
  ///   validated.
  /// - If \p Ref is invalid and \p S is \c nullptr, then assertions fail in
  ///   order to indicate the invalid \p Ref.
  /// - If \p Ref is invalid and \p S is not \c nullptr, then:
  ///   - The resulting \c ACCDataVar is invalid as if constructed with the
  ///     default constructor.
  ///   - If \p Quiet is false, \p S is used to issue one or more error
  ///     diagnostics.  For the sake of those diagnostics, \p RealDKind and
  ///     \p CKind must be the real directive (not an effective directive within
  //      a combined directive) and clause in which \p Ref appeared.
  ///   - If \p Quiet is true, \p S is used just to build and discard the
  ///     diagnostics that would have been issued.
  /// - If \p Ref is already known to be valid, it is harmless to set
  ///   \p AllowMemberExpr and \p AllowSubarray more permissively than
  ///   necessary, but it might be worthwhile to set them strictly and set \p S
  ///   to \c nullptr to assert that \p Ref really is valid.
  ACCDataVar(Expr *Ref, bool AllowMemberExpr = true, bool AllowSubarray = true,
             Sema *S = nullptr, bool Quiet = false,
             OpenACCDirectiveKind RealDKind = ACCD_unknown,
             OpenACCClauseKind CKind = ACCC_unknown)
      : CompositeDecl(nullptr), ReferencedDecl(nullptr) {
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

    // If it's a subarray, extract the base variable, and complain about any
    // subscript (masquerading as a subarray), for which OpenACC 2.7 doesn't
    // specify a behavior.
    Expr *RefWithoutSubarray = Ref->IgnoreParenImpCasts();
    if (auto *OASE =
            dyn_cast_or_null<OMPArraySectionExpr>(RefWithoutSubarray)) {
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

    // Check any member expression.
    if (MemberExpr *ME = dyn_cast<MemberExpr>(RefWithoutSubarray)) {
      if (!AllowMemberExpr) {
        assert(S && "expected no MemberExpr");
        Diag(ME->getBeginLoc(), diag::err_acc_unsupported_member_expr)
            << getOpenACCName(RealDKind) << getOpenACCName(CKind)
            << ME->getSourceRange();
      }
      Expr *BaseExpr = ME->getBase()->IgnoreParenImpCasts();
      if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(BaseExpr)) {
        CompositeDecl = dyn_cast<VarDecl>(DRE->getDecl());
        if (!CompositeDecl) {
          assert(S &&
                 "expected VarDecl to be referenced by DeclRefExpr base of "
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
      } else {
        assert(S && "expected DeclRefExpr as base of member expression");
        // Base of member expr cannot be member expr or subarray.
        Diag(BaseExpr->getBeginLoc(), diag::err_acc_expected_data_var)
            << /*allowMemberExpr=*/false << /*allowSubarray=*/false
            << BaseExpr->getSourceRange();
      }
      ReferencedDecl = dyn_cast<FieldDecl>(ME->getMemberDecl());
      if (!ReferencedDecl) {
        assert(S && "expected FieldDecl member of member expression");
        Diag(ME->getMemberLoc(), diag::err_acc_expected_data_member)
            << ME->getSourceRange();
      }
      return; /*data member*/
    }
    if (CXXMemberCallExpr *ME =
            dyn_cast<CXXMemberCallExpr>(RefWithoutSubarray)) {
      assert(S && "expected no CXXMemberCallExpr");
      if (!AllowMemberExpr) {
        Diag(ME->getBeginLoc(), diag::err_acc_unsupported_member_expr)
            << getOpenACCName(RealDKind) << getOpenACCName(CKind)
            << ME->getSourceRange();
      }
      Diag(ME->getExprLoc(), diag::err_acc_expected_data_member)
          << ME->getSourceRange();
      return; /*invalid*/
    }

    // Check any plain variable name.
    if (DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(RefWithoutSubarray)) {
      ReferencedDecl = dyn_cast<VarDecl>(DRE->getDecl());
      if (!ReferencedDecl) {
        assert(S && "expected VarDecl to be referenced by DeclRefExpr");
        Diag(RefWithoutSubarray->getBeginLoc(), diag::err_acc_expected_data_var)
            << AllowMemberExpr << AllowSubarray
            << RefWithoutSubarray->getSourceRange();
      }
      return; /*plain variable name*/
    }

    // We have an unexpected expression.  If it's a RecoveryExpr, then there was
    // an error reported somewhere else, so don't risk a spurious diagnostic
    // here.  Either way, do assert that an unexpected expression is encountered
    // only while errors are being diagnosed or ignored (i.e., S != nullptr).
    assert(S && "expected MemberExpr or DeclRefExpr");
    if (!isa<RecoveryExpr>(RefWithoutSubarray)) {
      Diag(RefWithoutSubarray->getBeginLoc(), diag::err_acc_expected_data_var)
          << AllowMemberExpr << AllowSubarray
          << RefWithoutSubarray->getSourceRange();
    }
    /*invalid*/
  }
  ACCDataVar(Expr *Ref, bool AllowMemberExpr, bool AllowSubarray, Sema *S,
             OpenACCDirectiveKind RealDKind, OpenACCClauseKind CKind)
      : ACCDataVar(Ref, AllowMemberExpr, AllowSubarray, S, /*Quiet=*/false,
                   RealDKind, CKind) {}
  /// If \c D is a variable declaration, construct a \c ACCDataVar for it.
  /// Otherwise, construct a placeholder for an invalid variable.  Because of
  /// the possibility of the latter case, it seems worthwhile to require this
  /// conversion to be explicit.
  explicit ACCDataVar(Decl *D)
      : CompositeDecl(nullptr), ReferencedDecl(dyn_cast<VarDecl>(D)) {}
  /// Is this a valid variable?  If not, other member functions shouldn't be
  /// called.
  bool isValid() const { return ReferencedDecl; }
  /// Get variable's declaration that was referenced by the original referencing
  /// expression.  Fail an assertion if \c isValid would return false.
  ValueDecl *getReferencedDecl() const {
    assert(isValid() && "expected valid ACCDataVar");
    return ReferencedDecl;
  }
  /// Is the variable a class/struct/union data member?
  bool isMember() const {
    assert(isValid() && "expected valid ACCDataVar");
    return isa<FieldDecl>(ReferencedDecl);
  }
  /// Get this variable's type.  Fail an assertion if \c isValid would return
  /// false.
  QualType getType() const {
    assert(isValid() && "expected valid ACCDataVar");
    return ReferencedDecl->getType();
  }
};
} // end namespace clang

namespace llvm {
using namespace clang;
/// See \c ACCDataVar documentation.
template <> struct DenseMapInfo<ACCDataVar> {
  static Decl *canonicalize(Decl *D) {
    if (D && D != DenseMapInfo<VarDecl *>::getEmptyKey() &&
        D != DenseMapInfo<VarDecl *>::getTombstoneKey())
      return D->getCanonicalDecl();
    return D;
  }
  static inline ACCDataVar getEmptyKey() {
    ACCDataVar Key;
    Key.CompositeDecl = DenseMapInfo<ValueDecl *>::getEmptyKey();
    Key.ReferencedDecl = DenseMapInfo<ValueDecl *>::getEmptyKey();
    return Key;
  }
  static inline ACCDataVar getTombstoneKey() {
    ACCDataVar Key;
    Key.CompositeDecl = DenseMapInfo<ValueDecl *>::getTombstoneKey();
    Key.ReferencedDecl = DenseMapInfo<ValueDecl *>::getTombstoneKey();
    return Key;
  }
  static unsigned getHashValue(ACCDataVar Var) {
    return DenseMapInfo<Decl *>::getHashValue(canonicalize(Var.ReferencedDecl));
    return detail::combineHashValue(
        DenseMapInfo<Decl *>::getHashValue(canonicalize(Var.CompositeDecl)),
        DenseMapInfo<Decl *>::getHashValue(canonicalize(Var.ReferencedDecl)));
  }
  static bool isEqual(const ACCDataVar &LHS, const ACCDataVar &RHS) {
    return canonicalize(LHS.CompositeDecl) == canonicalize(RHS.CompositeDecl) &&
           canonicalize(LHS.ReferencedDecl) == canonicalize(RHS.ReferencedDecl);
  }
};
} // end namespace llvm

#endif // LLVM_CLANG_LIB_SEMA_ACCDATAVAR_H
