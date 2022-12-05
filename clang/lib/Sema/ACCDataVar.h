//===------ ACCDataVar.h - OpenACC data variable ----------------*- C++ -*-===//
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

class ACCDataVar;

/// Convenient way to stream a variable or declaration name into a diagnostic.
/// For example, within Sema:
///
/// Diag(Loc, diag::err_acc_foobar)
///   << NameForDiag(*this, getCurFunctionDecl());
///
/// TODO: While convenient, it might be expensive, most notably if used before
/// diagnostics are actually known to be needed.
class NameForDiag {
  SmallString<128> Name;

public:
  NameForDiag(Sema &SemaRef, ValueDecl *D) {
    llvm::raw_svector_ostream OS(Name);
    D->getNameForDiagnostic(OS, SemaRef.getPrintingPolicy(),
                            /*Qualified=*/true);
  }
  NameForDiag(Sema &SemaRef, ACCDataVar Var);
  friend const StreamingDiagnostic &operator<<(const StreamingDiagnostic &SD,
                                               const NameForDiag &NFD) {
    return SD << NFD.Name;
  }
};

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
/// ultimately can be data attributes only for one or the other.  There is one
/// exception: an \c ACCDataVar for the \c this pointer (without a member) in
/// C++ always identifies \c this[0:1].  No data attributes are ever computed
/// for \c this as a scalar or for some other subarray of \c this.
///
/// An \c ACCDataVar for a class/struct/union member identifies both the
/// class/struct/union instance (possibly \c this) and the member.
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
  ///   - \c ReferencedType.isNull().
  /// - \c this[0:1] within a C++ class member function:
  ///   - \c CompositeDecl == \c nullptr.
  ///   - \c ReferencedDecl == \c nullptr.
  ///   - \c ReferencedType is the C++ class.
  /// - A data member of a class/struct/union instance:
  ///   - \c CompositeDecl is the class/struct/union instance's declaration that
  ///     was referenced by the original referencing expression, or it's
  ///     \c nullptr to indicate \c this within a C++ class member function.
  ///   - \c ReferencedDecl is the data member's \c FieldDecl that was
  ///     referenced by the original referencing expression.
  ///   - \c ReferencedType = \c ReferencedDecl->getType().
  /// - Some other kind of variable declaration:
  ///   - \c CompositeDecl == \c nullptr.
  ///   - \c ReferencedDecl is the variable's \c VarDecl that was referenced by
  ///     the original referencing expression.
  ///   - \c ReferencedType = \c ReferencedDecl->getType().
  ValueDecl *CompositeDecl;
  ValueDecl *ReferencedDecl;
  QualType ReferencedType;
  ///@}

public:
  /// Construct a placeholder for an invalid variable.
  ACCDataVar()
      : CompositeDecl(nullptr), ReferencedDecl(nullptr), ReferencedType() {}
  enum AllowMemberExprTy {
    /// Member expression not allowed.
    AllowMemberExprNone = 0,
    /// Member expression allowed only on implicit or explicit C++ 'this'.
    AllowMemberExprOnCXXThis = 1,
    /// Member expression allowed either on variable or on implicit or explicit
    /// C++ 'this'.
    AllowMemberExprOnAny = 2
  };
  /// Construct an \c ACCDataVar from an expression referencing a variable for
  /// which Clang analyzes OpenACC data attributes.
  ///
  /// - \p Ref is valid if it is either a:
  ///   - \c DeclRefExpr that references a variable declaration.
  ///   - \c MemberExpr that references a class/struct/union data member and
  ///     that is constrained by \p AllowMemberExpr.
  ///   - \c OMPArraySection whose base is a \c CXXThisExpr or one of the above
  ///     except that, if \p AllowSubarray is false, such a \p Ref is invalid.
  ///     If the base is a \c CXXThisExpr, the \c OMPArraySection must represent
  ///     \c this[0:1].
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
  ACCDataVar(Expr *Ref,
             AllowMemberExprTy AllowMemberExpr = AllowMemberExprOnAny,
             bool AllowSubarray = true, Sema *S = nullptr, bool Quiet = false,
             OpenACCDirectiveKind RealDKind = ACCD_unknown,
             OpenACCClauseKind CKind = ACCC_unknown)
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
    if (auto *OASE =
            dyn_cast_or_null<OMPArraySectionExpr>(RefWithoutSubarray)) {
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
      } else if (MemberExpr *NestedME = dyn_cast<MemberExpr>(BaseExpr)) {
        // A member expression on implicit 'this' looks like a variable name, so
        // "expected variable name" (the next diagnostic below) would be
        // confusing.  To be more helpful, we use the more specific diagnostic
        // for any nested member expression.
        assert(S && "unexpected nested member expression");
        bool NestedOnImplicitCXXThis = false;
        if (CXXThisExpr *TE = dyn_cast<CXXThisExpr>(NestedME->getBase()->
                                                    IgnoreParenImpCasts()))
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
  ACCDataVar(Expr *Ref, AllowMemberExprTy AllowMemberExpr, bool AllowSubarray,
             Sema *S, OpenACCDirectiveKind RealDKind, OpenACCClauseKind CKind)
      : ACCDataVar(Ref, AllowMemberExpr, AllowSubarray, S, /*Quiet=*/false,
                   RealDKind, CKind) {}
  /// If \c D is a variable declaration, construct a \c ACCDataVar for it.
  /// Otherwise, construct a placeholder for an invalid variable.  Because of
  /// the possibility of the latter case, it seems worthwhile to require this
  /// conversion to be explicit.
  explicit ACCDataVar(Decl *D)
      : CompositeDecl(nullptr), ReferencedDecl(dyn_cast<VarDecl>(D)) {
    if (!ReferencedDecl)
      return; /*invalid*/
    ReferencedType = ReferencedDecl->getType();
  }
  /// Is this a valid variable?  If not, other member functions shouldn't be
  /// called.
  bool isValid() const { return !ReferencedType.isNull(); }
  /// Is it \c this[0:1]?
  bool isCXXThis() const {
    assert(isValid() && "expected valid ACCDataVar");
    return !ReferencedDecl;
  }
  /// Does the variable have a declaration that was referenced by the original
  /// referencing expression?  That is, it's not \c this[0:1].
  bool hasReferencedDecl() const {
    assert(isValid() && "expected valid ACCDataVar");
    return ReferencedDecl;
  }
  /// Get variable's declaration that was referenced by the original referencing
  /// expression.  Fail an assertion if \c hasReferencedDecl would return false.
  ValueDecl *getReferencedDecl() const {
    assert(isValid() && "expected valid ACCDataVar");
    assert(ReferencedDecl && "expected hasReferencedDecl() to be checked");
    return ReferencedDecl;
  }
  /// Is the variable a class/struct/union data member?
  bool isMember() const {
    assert(isValid() && "expected valid ACCDataVar");
    return isa<FieldDecl>(ReferencedDecl);
  }
  /// Is the variable a class/struct data member on a C++ 'this' pointer?
  bool isMemberOnCXXThis() const { return isMember() && !CompositeDecl; }
  /// Get this variable's type.
  QualType getType() const {
    assert(isValid() && "expected valid ACCDataVar");
    return ReferencedType;
  }
  /// If the variable is const, issue diagnostic \p DiagID with the name of
  /// \p CKind as an argument and with location info from \p RefExpr, issue a
  /// note diagnostic explaining why it's const, and return true.  Otherwise,
  /// return false.
  bool diagnoseConst(Sema &SemaRef, unsigned int DiagID, Expr *RefExpr,
                     OpenACCClauseKind CKind) {
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
               SemaRef.getCurrentThisType()->getPointeeType()
                   .isConstant(Ctxt)) {
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
};

inline NameForDiag::NameForDiag(Sema &SemaRef, ACCDataVar Var) {
  llvm::raw_svector_ostream OS(Name);
  if (Var.hasReferencedDecl())
    Var.getReferencedDecl()->getNameForDiagnostic(
        OS, SemaRef.getPrintingPolicy(), /*Qualified=*/true);
  else {
    assert(Var.isCXXThis() &&
           "expected ACCDataVar without Decl* to be this[0:1]");
    OS << "this[0:1]";
  }
}
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
    Key.ReferencedType = DenseMapInfo<QualType>::getEmptyKey();
    return Key;
  }
  static inline ACCDataVar getTombstoneKey() {
    ACCDataVar Key;
    Key.CompositeDecl = DenseMapInfo<ValueDecl *>::getTombstoneKey();
    Key.ReferencedDecl = DenseMapInfo<ValueDecl *>::getTombstoneKey();
    Key.ReferencedType = DenseMapInfo<QualType>::getTombstoneKey();
    return Key;
  }
  static unsigned getHashValue(ACCDataVar Var) {
    return detail::combineHashValue(
        detail::combineHashValue(
            DenseMapInfo<Decl *>::getHashValue(canonicalize(Var.CompositeDecl)),
            DenseMapInfo<Decl *>::getHashValue(
                canonicalize(Var.ReferencedDecl))),
        DenseMapInfo<QualType>::getHashValue(Var.ReferencedType));
  }
  static bool isEqual(const ACCDataVar &LHS, const ACCDataVar &RHS) {
    return canonicalize(LHS.CompositeDecl) == canonicalize(RHS.CompositeDecl) &&
           canonicalize(LHS.ReferencedDecl) ==
               canonicalize(RHS.ReferencedDecl) &&
           LHS.ReferencedType == RHS.ReferencedType;
  }
};
} // end namespace llvm

#endif // LLVM_CLANG_LIB_SEMA_ACCDATAVAR_H
