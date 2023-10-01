//===- OpenACCClause.h - Classes for OpenACC clauses ------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// This file defines OpenACC AST classes for clauses.
/// There are clauses for executable directives, clauses for declarative
/// directives and clauses which can be used in both kinds of directives.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_OPENACCCLAUSE_H
#define LLVM_CLANG_AST_OPENACCCLAUSE_H

#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/OpenACCKinds.h"
#include "clang/Basic/SourceLocation.h"

namespace clang {

//===----------------------------------------------------------------------===//
// AST classes for clauses.
//===----------------------------------------------------------------------===//

/// This is a basic class for representing single OpenACC clause.
class ACCClause {
  friend class ACCClauseReader;

  /// Kind of the clause.
  OpenACCClauseKind Kind;
  /// Dealiased kind of the clause.
  OpenACCClauseKind KindDealiased;

  /// How the clause was determined.
  OpenACCDetermination Determination;
  /// Starting location of the clause (the clause keyword).
  SourceLocation StartLoc;
  /// Ending location of the clause.
  SourceLocation EndLoc;

  /// Set how the clause was determined.
  void setDetermination(OpenACCDetermination D) { Determination = D; }
  /// Sets the starting location of the clause.
  void setLocStart(SourceLocation Loc) { StartLoc = Loc; }
  /// Sets the ending location of the clause.
  void setLocEnd(SourceLocation Loc) { EndLoc = Loc; }

protected:
  /// Build a clause.
  ACCClause(OpenACCClauseKind K, OpenACCDetermination Determination,
            SourceLocation StartLoc, SourceLocation EndLoc,
            OpenACCClauseKind KDealiased = ACCC_unknown)
      : Kind(K), KindDealiased(KDealiased == ACCC_unknown ? K : KDealiased),
        Determination(Determination), StartLoc(StartLoc), EndLoc(EndLoc)
  {
    // If we parsed it, we should have valid locations.  Sometimes parse error
    // recovery fails to set valid locations, but those cases should be fixed.
    assert((Determination == ACC_EXPLICIT) == (!StartLoc.isInvalid() &&
                                               !EndLoc.isInvalid()) &&
           "expected valid clause location for and only for explicit clause");
    // Make sure the right constructor was called.
    assert(Determination != ACC_UNDETERMINED &&
           "expected clause determination to be set to determined");
  }
  /// Build an empty clause, and \c ACCClauseReader will set the remaining
  /// fields afterward.
  ACCClause(OpenACCClauseKind K, OpenACCClauseKind KDealiased = ACCC_unknown)
      : Kind(K), KindDealiased(KDealiased == ACCC_unknown ? K : KDealiased),
        Determination(ACC_UNDETERMINED)
  {}

public:
  /// Returns kind of OpenACC clause (private, shared, reduction, etc.).
  OpenACCClauseKind getClauseKind() const { return Kind; }
  /// Returns dealiased kind of OpenACC clause (for example, copy when
  /// getClauseKind returns pcopy).
  OpenACCClauseKind getClauseKindDealiased() const { return KindDealiased; }

  /// How was the clause determined?
  OpenACCDetermination getDetermination() const { return Determination; }
  /// Returns the starting location of the clause.
  SourceLocation getBeginLoc() const { return StartLoc; }
  /// Returns the ending location of the clause.
  SourceLocation getEndLoc() const { return EndLoc; }
  /// Returns the source range of the clause.
  SourceRange getSourceRange() const { return SourceRange(StartLoc, EndLoc); }

  typedef StmtIterator child_iterator;
  typedef ConstStmtIterator const_child_iterator;
  typedef llvm::iterator_range<child_iterator> child_range;
  typedef llvm::iterator_range<const_child_iterator> const_child_range;

  child_range children();
  const_child_range children() const {
    auto Children = const_cast<ACCClause *>(this)->children();
    return const_child_range(Children.begin(), Children.end());
  }
  static bool classof(const ACCClause *) { return true; }
};

/// This represents clauses with a list of variables like 'private',
/// 'firstprivate', 'copyin', or 'reduction' clauses in the
/// '#pragma acc ...' directives.
template <class T> class ACCVarListClause : public ACCClause {
  friend class ACCClauseReader;

  /// Location of '('.
  SourceLocation LParenLoc;
  /// Number of variables in the list.
  unsigned NumVars;

  /// Sets the location of '('.
  void setLParenLoc(SourceLocation Loc) { LParenLoc = Loc; }

protected:
  /// Fetches list of variables associated with this clause.
  MutableArrayRef<Expr *> getVarRefs() {
    return MutableArrayRef<Expr *>(
        static_cast<T *>(this)->template getTrailingObjects<Expr *>(), NumVars);
  }

  /// Sets the list of variables for this clause.
  void setVarRefs(ArrayRef<Expr *> VL) {
    assert(VL.size() == NumVars &&
           "Number of variables is not the same as the preallocated buffer");
    std::copy(VL.begin(), VL.end(),
              static_cast<T *>(this)->template getTrailingObjects<Expr *>());
  }

  /// Build a clause that has \a N variables
  ///
  /// \param K Kind of the clause.
  /// \param Determination How the clause was determined.
  /// \param StartLoc Starting location of the clause (the clause keyword).
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param N Number of the variables in the clause.
  /// \param K Dealiased kind of the clause.
  ACCVarListClause(OpenACCClauseKind K, OpenACCDetermination Determination,
                   SourceLocation StartLoc, SourceLocation LParenLoc,
                   SourceLocation EndLoc, unsigned N,
                   OpenACCClauseKind KDealiased = ACCC_unknown)
      : ACCClause(K, Determination, StartLoc, EndLoc,
                  KDealiased == ACCC_unknown ? K : KDealiased),
        LParenLoc(LParenLoc), NumVars(N) {}
  /// Build an empty clause that has \a N variables, and \c ACCClauseReader
  /// will set the remaining fields afterward.
  ///
  /// \param K Kind of the clause.
  /// \param N Number of the variables in the clause.
  /// \param K Dealiased kind of the clause.
  ACCVarListClause(OpenACCClauseKind K, unsigned N,
                   OpenACCClauseKind KDealiased = ACCC_unknown)
      : ACCClause(K, KDealiased == ACCC_unknown ? K : KDealiased), NumVars(N)
  {}

public:
  typedef MutableArrayRef<Expr *>::iterator varlist_iterator;
  typedef ArrayRef<const Expr *>::iterator varlist_const_iterator;
  typedef llvm::iterator_range<varlist_iterator> varlist_range;
  typedef llvm::iterator_range<varlist_const_iterator> varlist_const_range;

  unsigned varlist_size() const { return NumVars; }
  bool varlist_empty() const { return NumVars == 0; }

  varlist_range varlists() {
    return varlist_range(varlist_begin(), varlist_end());
  }
  varlist_const_range varlists() const {
    return varlist_const_range(varlist_begin(), varlist_end());
  }

  varlist_iterator varlist_begin() { return getVarRefs().begin(); }
  varlist_iterator varlist_end() { return getVarRefs().end(); }
  varlist_const_iterator varlist_begin() const { return getVarRefs().begin(); }
  varlist_const_iterator varlist_end() const { return getVarRefs().end(); }

  /// Returns the location of '('.
  SourceLocation getLParenLoc() const { return LParenLoc; }

  /// Fetches list of all variables in the clause.
  ArrayRef<const Expr *> getVarRefs() const {
    return llvm::ArrayRef(
        static_cast<const T *>(this)->template getTrailingObjects<Expr *>(),
        NumVars);
  }
};

/// If a clause makes variables private to the construct on which it appears,
/// return the expressions referencing those variables.
llvm::iterator_range<ArrayRef<Expr *>::iterator>
getPrivateVarsFromClause(ACCClause *);

/// This represents the implicit clause 'nomap' for '#pragma acc ...'.
///
/// These clauses are computed implicitly by Clang.  Currently, OpenACC does
/// not define an explicit version, so Clang does not accept one.
class ACCNomapClause final
    : public ACCVarListClause<ACCNomapClause>,
      private llvm::TrailingObjects<ACCNomapClause, Expr *> {
  friend TrailingObjects;
  friend ACCVarListClause;
  friend class ACCClauseReader;

  /// Build clause with number of variables \a N.
  ///
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param N Number of the variables in the clause.
  explicit ACCNomapClause(SourceLocation StartLoc, SourceLocation LParenLoc,
                          SourceLocation EndLoc, unsigned N)
      : ACCVarListClause<ACCNomapClause>(ACCC_nomap, ACC_IMPLICIT,
                                         StartLoc, LParenLoc, EndLoc, N) {}

  /// Build an empty clause.
  ///
  /// \param N Number of the variables in the clause.
  explicit ACCNomapClause(unsigned N)
      : ACCVarListClause<ACCNomapClause>(ACCC_nomap, N) {}

public:
  /// Creates clause with a list of variables \a VL.
  ///
  /// \param C AST context.
  /// \param VL List of references to the variables.
  static ACCNomapClause *Create(const ASTContext &C, ArrayRef<Expr *> VL);
  /// Creates an empty clause with the place for \a N variables.
  ///
  /// \param C AST context.
  /// \param N The number of variables.
  static ACCNomapClause *CreateEmpty(const ASTContext &C, unsigned N);

  child_range children() {
    return child_range(reinterpret_cast<Stmt **>(varlist_begin()),
                       reinterpret_cast<Stmt **>(varlist_end()));
  }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_nomap;
  }
};

/// This represents the clause 'present' for '#pragma acc ...' directives.
///
/// \code
/// #pragma acc parallel present(a,b)
/// \endcode
/// In this example directive '#pragma acc parallel' has clause 'present' with
/// the variables 'a' and 'b'.
class ACCPresentClause final
    : public ACCVarListClause<ACCPresentClause>,
      private llvm::TrailingObjects<ACCPresentClause, Expr *> {
  friend TrailingObjects;
  friend ACCVarListClause;
  friend class ACCClauseReader;

  /// Build clause with number of variables \a N.
  ///
  /// \param Determination How the clause was determined.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param N Number of the variables in the clause.
  ACCPresentClause(OpenACCDetermination Determination, SourceLocation StartLoc,
                   SourceLocation LParenLoc, SourceLocation EndLoc, unsigned N)
      : ACCVarListClause<ACCPresentClause>(ACCC_present, Determination,
                                           StartLoc, LParenLoc, EndLoc, N)
  {}

  /// Build an empty clause.
  ///
  /// \param N Number of variables.
  explicit ACCPresentClause(unsigned N)
      : ACCVarListClause<ACCPresentClause>(ACCC_present, N) {
  }

public:
  /// Creates clause with a list of variables \a VL.
  ///
  /// \param C AST context.
  /// \param Determination How the clause was determined.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param VL List of references to the variables.
  static ACCPresentClause *Create(
      const ASTContext &C, OpenACCDetermination Determination,
      SourceLocation StartLoc, SourceLocation LParenLoc, SourceLocation EndLoc,
      ArrayRef<Expr *> VL);
  /// Creates an empty clause with the place for \a N variables.
  ///
  /// \param C AST context.
  /// \param N The number of variables.
  static ACCPresentClause *CreateEmpty(const ASTContext &C, unsigned N);

  child_range children() {
    return child_range(reinterpret_cast<Stmt **>(varlist_begin()),
                       reinterpret_cast<Stmt **>(varlist_end()));
  }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_present;
  }
};

/// This represents the clause 'copy' (or any of its aliases) for
/// '#pragma acc ...' directives.
///
/// \code
/// #pragma acc parallel copy(a,b)
/// \endcode
/// In this example directive '#pragma acc parallel' has clause 'copy' with
/// the variables 'a' and 'b'.
class ACCCopyClause final
    : public ACCVarListClause<ACCCopyClause>,
      private llvm::TrailingObjects<ACCCopyClause, Expr *> {
  friend TrailingObjects;
  friend ACCVarListClause;
  friend class ACCClauseReader;

  /// Build clause with number of variables \a N.
  ///
  /// \param Kind Which alias of the copy clause.
  /// \param Determination How the clause was determined.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param N Number of the variables in the clause.
  ACCCopyClause(OpenACCClauseKind Kind, OpenACCDetermination Determination,
                SourceLocation StartLoc, SourceLocation LParenLoc,
                SourceLocation EndLoc, unsigned N)
      : ACCVarListClause<ACCCopyClause>(Kind, Determination, StartLoc,
                                        LParenLoc, EndLoc, N, ACCC_copy) {
    assert(isClauseKind(Kind) && "expected copy clause or alias");
  }

  /// Build an empty clause.
  ///
  /// \param Kind Which alias of the copy clause.
  /// \param N Number of variables.
  explicit ACCCopyClause(OpenACCClauseKind Kind, unsigned N)
      : ACCVarListClause<ACCCopyClause>(Kind, N, ACCC_copy) {
    assert(isClauseKind(Kind) && "expected copy clause or alias");
  }

public:
  /// Creates clause with a list of variables \a VL.
  ///
  /// \param C AST context.
  /// \param Kind Which alias of the copy clause.
  /// \param Determination How the clause was determined.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param VL List of references to the variables.
  static ACCCopyClause *Create(
      const ASTContext &C, OpenACCClauseKind Kind,
      OpenACCDetermination Determination, SourceLocation StartLoc,
      SourceLocation LParenLoc, SourceLocation EndLoc, ArrayRef<Expr *> VL);
  /// Creates an empty clause with the place for \a N variables.
  ///
  /// \param C AST context.
  /// \param Kind Which alias of the copy clause.
  /// \param N The number of variables.
  static ACCCopyClause *CreateEmpty(const ASTContext &C,
                                    OpenACCClauseKind Kind, unsigned N);

  child_range children() {
    return child_range(reinterpret_cast<Stmt **>(varlist_begin()),
                       reinterpret_cast<Stmt **>(varlist_end()));
  }

  static bool isClauseKind(OpenACCClauseKind Kind) {
    switch (Kind) {
#define OPENACC_CLAUSE_ALIAS_copy(Name) \
    case ACCC_##Name:                   \
      return true;
#include "clang/Basic/OpenACCKinds.def"
    default:
      return false;
    }
  }

  static bool classof(const ACCClause *T) {
    return isClauseKind(T->getClauseKind());
  }
};

/// This represents the clause 'copyin' (or any of its aliases) for
/// '#pragma acc ...' directives.
///
/// \code
/// #pragma acc parallel copyin(a,b)
/// \endcode
/// In this example directive '#pragma acc parallel' has clause 'copyin' with
/// the variables 'a' and 'b'.
class ACCCopyinClause final
    : public ACCVarListClause<ACCCopyinClause>,
      private llvm::TrailingObjects<ACCCopyinClause, Expr *> {
  friend TrailingObjects;
  friend ACCVarListClause;
  friend class ACCClauseReader;

  /// Build clause with number of variables \a N.
  ///
  /// \param Kind Which alias of the copyin clause.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param N Number of the variables in the clause.
  ACCCopyinClause(OpenACCClauseKind Kind, SourceLocation StartLoc,
                  SourceLocation LParenLoc, SourceLocation EndLoc, unsigned N)
      : ACCVarListClause<ACCCopyinClause>(Kind, ACC_EXPLICIT, StartLoc,
                                          LParenLoc, EndLoc, N, ACCC_copyin) {
    assert(isClauseKind(Kind) && "expected copyin clause or alias");
  }

  /// Build an empty clause.
  ///
  /// \param Kind Which alias of the copyin clause.
  /// \param N Number of variables.
  explicit ACCCopyinClause(OpenACCClauseKind Kind, unsigned N)
      : ACCVarListClause<ACCCopyinClause>(Kind, N, ACCC_copyin) {
    assert(isClauseKind(Kind) && "expected copyin clause or alias");
  }

public:
  /// Creates clause with a list of variables \a VL.
  ///
  /// \param C AST context.
  /// \param Kind Which alias of the copyin clause.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param VL List of references to the variables.
  static ACCCopyinClause *Create(const ASTContext &C, OpenACCClauseKind Kind,
                                 SourceLocation StartLoc,
                                 SourceLocation LParenLoc,
                                 SourceLocation EndLoc, ArrayRef<Expr *> VL);
  /// Creates an empty clause with the place for \a N variables.
  ///
  /// \param C AST context.
  /// \param Kind Which alias of the copyin clause.
  /// \param N The number of variables.
  static ACCCopyinClause *CreateEmpty(const ASTContext &C,
                                      OpenACCClauseKind Kind, unsigned N);

  child_range children() {
    return child_range(reinterpret_cast<Stmt **>(varlist_begin()),
                       reinterpret_cast<Stmt **>(varlist_end()));
  }

  static bool isClauseKind(OpenACCClauseKind Kind) {
    switch (Kind) {
#define OPENACC_CLAUSE_ALIAS_copyin(Name) \
    case ACCC_##Name:                   \
      return true;
#include "clang/Basic/OpenACCKinds.def"
    default:
      return false;
    }
  }

  static bool classof(const ACCClause *T) {
    return isClauseKind(T->getClauseKind());
  }
};

/// This represents the clause 'copyout' (or any of its aliases) for
/// '#pragma acc ...' directives.
///
/// \code
/// #pragma acc parallel copyout(a,b)
/// \endcode
/// In this example directive '#pragma acc parallel' has clause 'copyout' with
/// the variables 'a' and 'b'.
class ACCCopyoutClause final
    : public ACCVarListClause<ACCCopyoutClause>,
      private llvm::TrailingObjects<ACCCopyoutClause, Expr *> {
  friend TrailingObjects;
  friend ACCVarListClause;
  friend class ACCClauseReader;

  /// Build clause with number of variables \a N.
  ///
  /// \param Kind Which alias of the copyout clause.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param N Number of the variables in the clause.
  ACCCopyoutClause(OpenACCClauseKind Kind, SourceLocation StartLoc,
                   SourceLocation LParenLoc, SourceLocation EndLoc, unsigned N)
      : ACCVarListClause<ACCCopyoutClause>(Kind, ACC_EXPLICIT, StartLoc,
                                           LParenLoc, EndLoc, N, ACCC_copyout)
  {
    assert(isClauseKind(Kind) && "expected copyout clause or alias");
  }

  /// Build an empty clause.
  ///
  /// \param Kind Which alias of the copyout clause.
  /// \param N Number of variables.
  explicit ACCCopyoutClause(OpenACCClauseKind Kind, unsigned N)
      : ACCVarListClause<ACCCopyoutClause>(Kind, N, ACCC_copyout) {
    assert(isClauseKind(Kind) && "expected copyout clause or alias");
  }

public:
  /// Creates clause with a list of variables \a VL.
  ///
  /// \param C AST context.
  /// \param Kind Which alias of the copyout clause.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param VL List of references to the variables.
  static ACCCopyoutClause *Create(const ASTContext &C, OpenACCClauseKind Kind,
                                  SourceLocation StartLoc,
                                  SourceLocation LParenLoc,
                                  SourceLocation EndLoc, ArrayRef<Expr *> VL);
  /// Creates an empty clause with the place for \a N variables.
  ///
  /// \param C AST context.
  /// \param Kind Which alias of the copyout clause.
  /// \param N The number of variables.
  static ACCCopyoutClause *CreateEmpty(const ASTContext &C,
                                       OpenACCClauseKind Kind, unsigned N);

  child_range children() {
    return child_range(reinterpret_cast<Stmt **>(varlist_begin()),
                       reinterpret_cast<Stmt **>(varlist_end()));
  }

  static bool isClauseKind(OpenACCClauseKind Kind) {
    switch (Kind) {
#define OPENACC_CLAUSE_ALIAS_copyout(Name) \
    case ACCC_##Name:                   \
      return true;
#include "clang/Basic/OpenACCKinds.def"
    default:
      return false;
    }
  }

  static bool classof(const ACCClause *T) {
    return isClauseKind(T->getClauseKind());
  }
};

/// This represents the clause 'create' (or any of its aliases) for
/// '#pragma acc ...' directives.
///
/// \code
/// #pragma acc parallel create(a,b)
/// \endcode
/// In this example directive '#pragma acc parallel' has clause 'create' with
/// the variables 'a' and 'b'.
class ACCCreateClause final
    : public ACCVarListClause<ACCCreateClause>,
      private llvm::TrailingObjects<ACCCreateClause, Expr *> {
  friend TrailingObjects;
  friend ACCVarListClause;
  friend class ACCClauseReader;

  /// Build clause with number of variables \a N.
  ///
  /// \param Kind Which alias of the create clause.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param N Number of the variables in the clause.
  ACCCreateClause(OpenACCClauseKind Kind, SourceLocation StartLoc,
                  SourceLocation LParenLoc, SourceLocation EndLoc, unsigned N)
      : ACCVarListClause<ACCCreateClause>(Kind, ACC_EXPLICIT, StartLoc,
                                          LParenLoc, EndLoc, N, ACCC_create)
  {
    assert(isClauseKind(Kind) && "expected create clause or alias");
  }

  /// Build an empty clause.
  ///
  /// \param Kind Which alias of the create clause.
  /// \param N Number of variables.
  explicit ACCCreateClause(OpenACCClauseKind Kind, unsigned N)
      : ACCVarListClause<ACCCreateClause>(Kind, N, ACCC_create) {
    assert(isClauseKind(Kind) && "expected create clause or alias");
  }

public:
  /// Creates clause with a list of variables \a VL.
  ///
  /// \param C AST context.
  /// \param Kind Which alias of the create clause.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param VL List of references to the variables.
  static ACCCreateClause *Create(const ASTContext &C, OpenACCClauseKind Kind,
                                 SourceLocation StartLoc,
                                 SourceLocation LParenLoc,
                                 SourceLocation EndLoc, ArrayRef<Expr *> VL);
  /// Creates an empty clause with the place for \a N variables.
  ///
  /// \param C AST context.
  /// \param Kind Which alias of the create clause.
  /// \param N The number of variables.
  static ACCCreateClause *CreateEmpty(const ASTContext &C,
                                      OpenACCClauseKind Kind, unsigned N);

  child_range children() {
    return child_range(reinterpret_cast<Stmt **>(varlist_begin()),
                       reinterpret_cast<Stmt **>(varlist_end()));
  }

  static bool isClauseKind(OpenACCClauseKind Kind) {
    switch (Kind) {
#define OPENACC_CLAUSE_ALIAS_create(Name) \
    case ACCC_##Name:                   \
      return true;
#include "clang/Basic/OpenACCKinds.def"
    default:
      return false;
    }
  }

  static bool classof(const ACCClause *T) {
    return isClauseKind(T->getClauseKind());
  }
};

/// This represents the clause 'no_create' for '#pragma acc ...' directives.
///
/// \code
/// #pragma acc parallel no_create(a,b)
/// \endcode
/// In this example directive '#pragma acc parallel' has clause 'no_create' with
/// the variables 'a' and 'b'.
class ACCNoCreateClause final
    : public ACCVarListClause<ACCNoCreateClause>,
      private llvm::TrailingObjects<ACCNoCreateClause, Expr *> {
  friend TrailingObjects;
  friend ACCVarListClause;
  friend class ACCClauseReader;

  /// Build clause with number of variables \a N.
  ///
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param N Number of the variables in the clause.
  ACCNoCreateClause(SourceLocation StartLoc, SourceLocation LParenLoc,
                    SourceLocation EndLoc, unsigned N)
      : ACCVarListClause<ACCNoCreateClause>(ACCC_no_create, ACC_EXPLICIT,
                                            StartLoc, LParenLoc, EndLoc, N)
  {}

  /// Build an empty clause.
  ///
  /// \param N Number of variables.
  explicit ACCNoCreateClause(unsigned N)
      : ACCVarListClause<ACCNoCreateClause>(ACCC_no_create, N) {
  }

public:
  /// Creates clause with a list of variables \a VL.
  ///
  /// \param C AST context.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param VL List of references to the variables.
  static ACCNoCreateClause *Create(
      const ASTContext &C, SourceLocation StartLoc, SourceLocation LParenLoc,
      SourceLocation EndLoc, ArrayRef<Expr *> VL);
  /// Creates an empty clause with the place for \a N variables.
  ///
  /// \param C AST context.
  /// \param N The number of variables.
  static ACCNoCreateClause *CreateEmpty(const ASTContext &C, unsigned N);

  child_range children() {
    return child_range(reinterpret_cast<Stmt **>(varlist_begin()),
                       reinterpret_cast<Stmt **>(varlist_end()));
  }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_no_create;
  }
};

/// This represents the clause 'delete' for '#pragma acc ...' directives.
///
/// \code
/// #pragma acc exit data delete(a,b)
/// \endcode
/// In this example directive '#pragma acc exit data' has clause 'delete' with
/// the variables 'a' and 'b'.
class ACCDeleteClause final
    : public ACCVarListClause<ACCDeleteClause>,
      private llvm::TrailingObjects<ACCDeleteClause, Expr *> {
  friend TrailingObjects;
  friend ACCVarListClause;
  friend class ACCClauseReader;

  /// Build clause with number of variables \a N.
  ///
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param N Number of the variables in the clause.
  ACCDeleteClause(SourceLocation StartLoc, SourceLocation LParenLoc,
                  SourceLocation EndLoc, unsigned N)
      : ACCVarListClause<ACCDeleteClause>(ACCC_delete, ACC_EXPLICIT, StartLoc,
                                          LParenLoc, EndLoc, N) {}

  /// Build an empty clause.
  ///
  /// \param N Number of variables.
  explicit ACCDeleteClause(unsigned N)
      : ACCVarListClause<ACCDeleteClause>(ACCC_delete, N) {}

public:
  /// Creates clause with a list of variables \a VL.
  ///
  /// \param C AST context.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param VL List of references to the variables.
  static ACCDeleteClause *Create(const ASTContext &C, SourceLocation StartLoc,
                                 SourceLocation LParenLoc,
                                 SourceLocation EndLoc, ArrayRef<Expr *> VL);
  /// Creates an empty clause with the place for \a N variables.
  ///
  /// \param C AST context.
  /// \param N The number of variables.
  static ACCDeleteClause *CreateEmpty(const ASTContext &C, unsigned N);

  child_range children() {
    return child_range(reinterpret_cast<Stmt **>(varlist_begin()),
                       reinterpret_cast<Stmt **>(varlist_end()));
  }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_delete;
  }
};

/// This represents the implicit clause 'shared' for '#pragma acc ...'.
///
/// These clauses are computed implicitly by Clang.  Currently, OpenACC does
/// not define an explicit version, so Clang does not accept one.
class ACCSharedClause final
    : public ACCVarListClause<ACCSharedClause>,
      private llvm::TrailingObjects<ACCSharedClause, Expr *> {
  friend TrailingObjects;
  friend ACCVarListClause;
  friend class ACCClauseReader;

  /// Build clause with number of variables \a N.
  ///
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param N Number of the variables in the clause.
  explicit ACCSharedClause(SourceLocation StartLoc, SourceLocation LParenLoc,
                           SourceLocation EndLoc, unsigned N)
      : ACCVarListClause<ACCSharedClause>(ACCC_shared, ACC_IMPLICIT,
                                          StartLoc, LParenLoc, EndLoc, N) {}

  /// Build an empty clause.
  ///
  /// \param N Number of the variables in the clause.
  explicit ACCSharedClause(unsigned N)
      : ACCVarListClause<ACCSharedClause>(ACCC_shared, N) {}

public:
  /// Creates clause with a list of variables \a VL.
  ///
  /// \param C AST context.
  /// \param VL List of references to the variables.
  static ACCSharedClause *Create(const ASTContext &C, ArrayRef<Expr *> VL);
  /// Creates an empty clause with the place for \a N variables.
  ///
  /// \param C AST context.
  /// \param N The number of variables.
  static ACCSharedClause *CreateEmpty(const ASTContext &C, unsigned N);

  child_range children() {
    return child_range(reinterpret_cast<Stmt **>(varlist_begin()),
                       reinterpret_cast<Stmt **>(varlist_end()));
  }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_shared;
  }
};

/// This represents clause 'private' in the '#pragma acc ...' directives.
///
/// \code
/// #pragma acc parallel private(a,b)
/// \endcode
/// In this example directive '#pragma acc parallel' has clause 'private'
/// with the variables 'a' and 'b'.
class ACCPrivateClause final
    : public ACCVarListClause<ACCPrivateClause>,
      private llvm::TrailingObjects<ACCPrivateClause, Expr *> {
  friend TrailingObjects;
  friend ACCVarListClause;
  friend class ACCClauseReader;

  /// Build clause with number of variables \a N.
  ///
  /// \param Determination How the clause was determined.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param N Number of the variables in the clause.
  ACCPrivateClause(OpenACCDetermination Determination, SourceLocation StartLoc,
                   SourceLocation LParenLoc, SourceLocation EndLoc, unsigned N)
      : ACCVarListClause<ACCPrivateClause>(ACCC_private, Determination,
                                           StartLoc, LParenLoc, EndLoc, N) {}

  /// Build an empty clause.
  ///
  /// \param N Number of variables.
  explicit ACCPrivateClause(unsigned N)
      : ACCVarListClause<ACCPrivateClause>(ACCC_private, N) {}

public:
  /// Creates clause with a list of variables \a VL.
  ///
  /// \param C AST context.
  /// \param Determination How the clause was determined.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param VL List of references to the variables.
  static ACCPrivateClause *Create(
      const ASTContext &C, OpenACCDetermination Determination,
      SourceLocation StartLoc, SourceLocation LParenLoc, SourceLocation EndLoc,
      ArrayRef<Expr *> VL);
  /// Creates an empty clause with the place for \a N variables.
  ///
  /// \param C AST context.
  /// \param N The number of variables.
  static ACCPrivateClause *CreateEmpty(const ASTContext &C, unsigned N);

  child_range children() {
    return child_range(reinterpret_cast<Stmt **>(varlist_begin()),
                       reinterpret_cast<Stmt **>(varlist_end()));
  }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_private;
  }
};

/// This represents clause 'firstprivate' in the '#pragma acc ...'
/// directives.
///
/// \code
/// #pragma acc parallel firstprivate(a,b)
/// \endcode
/// In this example directive '#pragma acc parallel' has clause 'firstprivate'
/// with the variables 'a' and 'b'.
class ACCFirstprivateClause final
    : public ACCVarListClause<ACCFirstprivateClause>,
      private llvm::TrailingObjects<ACCFirstprivateClause, Expr *> {
  friend TrailingObjects;
  friend ACCVarListClause;
  friend class ACCClauseReader;

  /// Build clause with number of variables \a N.
  ///
  /// \param Determination How the clause was determined.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param N Number of the variables in the clause.
  ACCFirstprivateClause(OpenACCDetermination Determination,
                        SourceLocation StartLoc, SourceLocation LParenLoc,
                        SourceLocation EndLoc, unsigned N)
      : ACCVarListClause<ACCFirstprivateClause>(
          ACCC_firstprivate, Determination, StartLoc, LParenLoc, EndLoc, N) {}

  /// Build an empty clause.
  ///
  /// \param N Number of variables.
  explicit ACCFirstprivateClause(unsigned N)
      : ACCVarListClause<ACCFirstprivateClause>(ACCC_firstprivate, N) {}

public:
  /// Creates clause with a list of variables \a VL.
  ///
  /// \param C AST context.
  /// \param Determination How the clause was determined.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param VL List of references to the original variables.
  static ACCFirstprivateClause *
  Create(const ASTContext &C, OpenACCDetermination Determination,
         SourceLocation StartLoc, SourceLocation LParenLoc,
         SourceLocation EndLoc, ArrayRef<Expr *> VL);
  /// Creates an empty clause with the place for \a N variables.
  ///
  /// \param C AST context.
  /// \param N The number of variables.
  static ACCFirstprivateClause *CreateEmpty(const ASTContext &C, unsigned N);

  child_range children() {
    return child_range(reinterpret_cast<Stmt **>(varlist_begin()),
                       reinterpret_cast<Stmt **>(varlist_end()));
  }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_firstprivate;
  }
};

/// This represents 'reduction' clause in the '#pragma acc ...' directives.
///
/// \code
/// #pragma acc parallel reduction(+:a,b)
/// \endcode
/// In this example directive '#pragma acc parallel' has clause 'reduction'
/// with operator '+' and the variables 'a' and 'b'.
class ACCReductionClause final
    : public ACCVarListClause<ACCReductionClause>,
      private llvm::TrailingObjects<ACCReductionClause, Expr *> {
  friend class ACCClauseReader;
  friend ACCVarListClause;
  friend TrailingObjects;

  /// Location of ':'.
  SourceLocation ColonLoc;

  /// Name of custom operator.
  DeclarationNameInfo NameInfo;

  /// Build clause with number of variables \a N.
  ///
  /// \param Determination How the clause was determined.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param ColonLoc Location of ':'.
  /// \param N Number of the variables in the clause.
  /// \param NameInfo The full name info for reduction operator.
  ACCReductionClause(OpenACCDetermination Determination,
                     SourceLocation StartLoc, SourceLocation LParenLoc,
                     SourceLocation ColonLoc, SourceLocation EndLoc,
                     unsigned N, const DeclarationNameInfo &NameInfo)
      : ACCVarListClause<ACCReductionClause>(ACCC_reduction, Determination,
                                             StartLoc, LParenLoc, EndLoc, N),
        ColonLoc(ColonLoc), NameInfo(NameInfo) {}

  /// Build an empty clause.
  ///
  /// \param N Number of variables.
  explicit ACCReductionClause(unsigned N)
      : ACCVarListClause<ACCReductionClause>(ACCC_reduction, N) {}

  /// Sets location of ':' symbol in clause.
  void setColonLoc(SourceLocation CL) { ColonLoc = CL; }

  /// Sets the name info for specified reduction operator.
  void setNameInfo(DeclarationNameInfo DNI) { NameInfo = DNI; }

public:
  /// Creates clause with a list of variables \a VL.
  ///
  /// \param Determination How the clause was determined.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param ColonLoc Location of ':'.
  /// \param EndLoc Ending location of the clause.
  /// \param VL The variables in the clause.
  /// \param NameInfo The full name info for reduction operator.
  static ACCReductionClause *
  Create(const ASTContext &C, OpenACCDetermination Determination,
         SourceLocation StartLoc, SourceLocation LParenLoc,
         SourceLocation ColonLoc, SourceLocation EndLoc, ArrayRef<Expr *> VL,
         const DeclarationNameInfo &NameInfo);

  /// Creates an empty clause with the place for \a N variables.
  ///
  /// \param C AST context.
  /// \param N The number of variables.
  static ACCReductionClause *CreateEmpty(const ASTContext &C, unsigned N);

  /// Gets location of ':' symbol in clause.
  SourceLocation getColonLoc() const { return ColonLoc; }

  /// Gets the name info for specified reduction operator.
  const DeclarationNameInfo &getNameInfo() const { return NameInfo; }

  /// Print a reduction operator as it appears in source.
  static void printReductionOperator(raw_ostream &OS,
                                     DeclarationNameInfo NameInfo) {
    OverloadedOperatorKind OOK = NameInfo.getName().getCXXOverloadedOperator();
    if (OOK != OO_None) {
      // Print reduction operator in C format
      OS << getOperatorSpelling(OOK);
    } else {
      // Use C++ format
      OS << NameInfo;
    }
  }

  /// Print to a string a reduction operator as it appears in source.
  static std::string printReductionOperatorToString(
      DeclarationNameInfo NameInfo) {
    std::string Result;
    llvm::raw_string_ostream OS(Result);
    ACCReductionClause::printReductionOperator(OS, NameInfo);
    return Result;
  }

  /// Print the reduction operator as it appears in source.
  void printReductionOperator(raw_ostream &OS) const {
    printReductionOperator(OS, NameInfo);
  }

  child_range children() {
    return child_range(reinterpret_cast<Stmt **>(varlist_begin()),
                       reinterpret_cast<Stmt **>(varlist_end()));
  }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_reduction;
  }
};

/// This represents 'if' clause in the '#pragma acc update' directive.
///
/// \code
/// #pragma acc update if(c)
/// \endcode
/// In this example directive '#pragma acc update' has clause 'if' with single
/// expression 'c'.
class ACCIfClause : public ACCClause {
  friend class ACCClauseReader;

  /// Location of '('.
  SourceLocation LParenLoc;

  /// Original condition expression.
  Stmt *Condition = nullptr;

  /// Sets the location of '('.
  void setLParenLoc(SourceLocation Loc) { LParenLoc = Loc; }

  /// Set the original condition expression.
  ///
  /// \param C condition expression.
  void setCondition(Expr *C) { Condition = C; }

public:
  /// Build 'if' clause.
  ///
  /// \param C Original expression associated with this clause.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  ACCIfClause(Expr *C, SourceLocation StartLoc, SourceLocation LParenLoc,
              SourceLocation EndLoc)
      : ACCClause(ACCC_if, ACC_EXPLICIT, StartLoc, EndLoc),
        LParenLoc(LParenLoc), Condition(C) {}

  /// Build an empty clause.
  ACCIfClause() : ACCClause(ACCC_if) {}

  /// Returns the location of '('.
  SourceLocation getLParenLoc() const { return LParenLoc; }

  /// Return the original condition expression.
  Expr *getCondition() { return cast<Expr>(Condition); }

  /// Return the original condition expression.
  Expr *getCondition() const { return cast<Expr>(Condition); }

  child_range children() {
    return child_range(&Condition, &Condition + 1);
  }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_if;
  }
};

/// This represents 'if_present' clause in the '#pragma acc ...'
/// directive.
///
/// \code
/// #pragma acc update if_present
/// \endcode
/// In this example directive '#pragma acc update' has clause 'if_present'.
class ACCIfPresentClause : public ACCClause {
public:
  /// Build 'if_present' clause.
  ///
  /// \param StartLoc Starting location of the clause.
  /// \param EndLoc Ending location of the clause.
  ACCIfPresentClause(SourceLocation StartLoc, SourceLocation EndLoc)
      : ACCClause(ACCC_if_present, ACC_EXPLICIT, StartLoc, EndLoc) {}

  /// Build an empty clause.
  ACCIfPresentClause() : ACCClause(ACCC_if_present) {}

  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_if_present;
  }
};

/// This represents the clause 'self' (or any of its aliases) for
/// '#pragma acc ...' directives.
///
/// \code
/// #pragma acc update self(a,b)
/// \endcode
/// In this example directive '#pragma acc update' has clause 'self' with
/// the variables 'a' and 'b'.
class ACCSelfClause final
    : public ACCVarListClause<ACCSelfClause>,
      private llvm::TrailingObjects<ACCSelfClause, Expr *> {
  friend TrailingObjects;
  friend ACCVarListClause;
  friend class ACCClauseReader;

  /// Build clause with number of variables \a N.
  ///
  /// \param Kind Which alias of the self clause.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param N Number of the variables in the clause.
  ACCSelfClause(OpenACCClauseKind Kind, SourceLocation StartLoc,
                SourceLocation LParenLoc, SourceLocation EndLoc, unsigned N)
      : ACCVarListClause<ACCSelfClause>(Kind, ACC_EXPLICIT, StartLoc, LParenLoc,
                                        EndLoc, N, ACCC_self) {
    assert(isClauseKind(Kind) && "expected self clause or alias");
  }

  /// Build an empty clause.
  ///
  /// \param Kind Which alias of the self clause.
  /// \param N Number of variables.
  explicit ACCSelfClause(OpenACCClauseKind Kind, unsigned N)
      : ACCVarListClause<ACCSelfClause>(Kind, N, ACCC_self) {
    assert(isClauseKind(Kind) && "expected self clause or alias");
  }

public:
  /// Creates clause with a list of variables \a VL.
  ///
  /// \param C AST context.
  /// \param Kind Which alias of the self clause.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param VL List of references to the variables.
  static ACCSelfClause *Create(const ASTContext &C, OpenACCClauseKind Kind,
                               SourceLocation StartLoc,
                               SourceLocation LParenLoc, SourceLocation EndLoc,
                               ArrayRef<Expr *> VL);
  /// Creates an empty clause with the place for \a N variables.
  ///
  /// \param C AST context.
  /// \param Kind Which alias of the self clause.
  /// \param N The number of variables.
  static ACCSelfClause *CreateEmpty(const ASTContext &C, OpenACCClauseKind Kind,
                                    unsigned N);

  child_range children() {
    return child_range(reinterpret_cast<Stmt **>(varlist_begin()),
                       reinterpret_cast<Stmt **>(varlist_end()));
  }

  static bool isClauseKind(OpenACCClauseKind Kind) {
    switch (Kind) {
#define OPENACC_CLAUSE_ALIAS_self(Name) \
    case ACCC_##Name:                   \
      return true;
#include "clang/Basic/OpenACCKinds.def"
    default:
      return false;
    }
  }

  static bool classof(const ACCClause *T) {
    return isClauseKind(T->getClauseKind());
  }
};

/// This represents the clause 'device' (or any of its aliases) for
/// '#pragma acc ...' directives.
///
/// \code
/// #pragma acc update device(a,b)
/// \endcode
/// In this example directive '#pragma acc update' has clause 'device' with
/// the variables 'a' and 'b'.
class ACCDeviceClause final
    : public ACCVarListClause<ACCDeviceClause>,
      private llvm::TrailingObjects<ACCDeviceClause, Expr *> {
  friend TrailingObjects;
  friend ACCVarListClause;
  friend class ACCClauseReader;

  /// Build clause with number of variables \a N.
  ///
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param N Number of the variables in the clause.
  ACCDeviceClause(SourceLocation StartLoc, SourceLocation LParenLoc,
                  SourceLocation EndLoc, unsigned N)
      : ACCVarListClause<ACCDeviceClause>(ACCC_device, ACC_EXPLICIT, StartLoc,
                                          LParenLoc, EndLoc, N) {}

  /// Build an empty clause.
  ///
  /// \param N Number of variables.
  explicit ACCDeviceClause(unsigned N)
      : ACCVarListClause<ACCDeviceClause>(ACCC_device, N) {}

public:
  /// Creates clause with a list of variables \a VL.
  ///
  /// \param C AST context.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param VL List of references to the variables.
  static ACCDeviceClause *Create(const ASTContext &C, SourceLocation StartLoc,
                                 SourceLocation LParenLoc,
                                 SourceLocation EndLoc, ArrayRef<Expr *> VL);
  /// Creates an empty clause with the place for \a N variables.
  ///
  /// \param C AST context.
  /// \param N The number of variables.
  static ACCDeviceClause *CreateEmpty(const ASTContext &C, unsigned N);

  child_range children() {
    return child_range(reinterpret_cast<Stmt **>(varlist_begin()),
                       reinterpret_cast<Stmt **>(varlist_end()));
  }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_device;
  }
};

/// This represents 'num_gangs' clause in the '#pragma acc ...'
/// directive.
///
/// \code
/// #pragma acc parallel num_gangs(n)
/// \endcode
/// In this example directive '#pragma acc parallel' has clause 'num_gangs'
/// with single expression 'n'.
class ACCNumGangsClause : public ACCClause {
  friend class ACCClauseReader;

  /// Location of '('.
  SourceLocation LParenLoc;

  /// Original NumGangs expression.
  Stmt *NumGangs = nullptr;

  /// Set the original NumGangs expression.
  ///
  /// \param E NumGangs expression.
  void setNumGangs(Expr *E) { NumGangs = E; }

public:
  /// Build 'num_gangs' clause.
  ///
  /// \param E Original expression associated with this clause.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  ACCNumGangsClause(Expr *E, SourceLocation StartLoc, SourceLocation LParenLoc,
                    SourceLocation EndLoc)
      : ACCClause(ACCC_num_gangs, ACC_EXPLICIT, StartLoc, EndLoc),
        LParenLoc(LParenLoc), NumGangs(E) {
  }

  /// Build an empty clause.
  ACCNumGangsClause() : ACCClause(ACCC_num_gangs) {}

  /// Sets the location of '('.
  void setLParenLoc(SourceLocation Loc) { LParenLoc = Loc; }

  /// Returns the location of '('.
  SourceLocation getLParenLoc() const { return LParenLoc; }

  /// Return the original NumGangs expression.
  Expr *getNumGangs() { return cast<Expr>(NumGangs); }

  /// Return the original NumGangs expression.
  Expr *getNumGangs() const { return cast<Expr>(NumGangs); }

  child_range children() { return child_range(&NumGangs, &NumGangs + 1); }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_num_gangs;
  }
};

/// This represents 'num_workers' clause in the '#pragma acc ...'
/// directive.
///
/// \code
/// #pragma acc parallel num_workers(n)
/// \endcode
/// In this example directive '#pragma acc parallel' has clause 'num_workers'
/// with single expression 'n'.
class ACCNumWorkersClause : public ACCClause {
  friend class ACCClauseReader;

  /// Location of '('.
  SourceLocation LParenLoc;

  /// Original num_workers expression.
  Stmt *NumWorkers = nullptr;

  /// Sets the location of '('.
  void setLParenLoc(SourceLocation Loc) { LParenLoc = Loc; }

  /// Set the original num_workers expression.
  ///
  /// \param E num_workers expression.
  void setNumWorkers(Expr *E) { NumWorkers = E; }

public:
  /// Build 'num_workers' clause.
  ///
  /// \param E Original expression associated with this clause.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  ACCNumWorkersClause(Expr *E, SourceLocation StartLoc, SourceLocation LParenLoc,
                      SourceLocation EndLoc)
      : ACCClause(ACCC_num_workers, ACC_EXPLICIT, StartLoc, EndLoc),
        LParenLoc(LParenLoc), NumWorkers(E) {
  }

  /// Build an empty clause.
  ACCNumWorkersClause() : ACCClause(ACCC_num_workers) {}

  /// Returns the location of '('.
  SourceLocation getLParenLoc() const { return LParenLoc; }

  /// Return the original num_workers expression.
  Expr *getNumWorkers() { return cast<Expr>(NumWorkers); }

  /// Return the original num_workers expression.
  Expr *getNumWorkers() const { return cast<Expr>(NumWorkers); }

  child_range children() { return child_range(&NumWorkers, &NumWorkers + 1); }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_num_workers;
  }
};

/// This represents 'vector_length' clause in the '#pragma acc ...'
/// directive.
///
/// \code
/// #pragma acc parallel vector_length(n)
/// \endcode
/// In this example directive '#pragma acc parallel' has clause 'vector_length'
/// with single expression 'n'.
class ACCVectorLengthClause : public ACCClause {
  friend class ACCClauseReader;

  /// Location of '('.
  SourceLocation LParenLoc;

  /// Original vector_length expression.
  Stmt *VectorLength = nullptr;

  /// Sets the location of '('.
  void setLParenLoc(SourceLocation Loc) { LParenLoc = Loc; }

  /// Set the original vector_length expression.
  ///
  /// \param E vector_length expression.
  void setVectorLength(Expr *E) { VectorLength = E; }

public:
  /// Build 'vector_length' clause.
  ///
  /// \param E Original expression associated with this clause.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  ACCVectorLengthClause(Expr *E, SourceLocation StartLoc, SourceLocation LParenLoc,
                        SourceLocation EndLoc)
      : ACCClause(ACCC_vector_length, ACC_EXPLICIT, StartLoc, EndLoc),
        LParenLoc(LParenLoc), VectorLength(E) {
  }

  /// Build an empty clause.
  ACCVectorLengthClause() : ACCClause(ACCC_vector_length) {}

  /// Returns the location of '('.
  SourceLocation getLParenLoc() const { return LParenLoc; }

  /// Return the original vector_length expression.
  Expr *getVectorLength() { return cast<Expr>(VectorLength); }

  /// Return the original vector_length expression.
  Expr *getVectorLength() const { return cast<Expr>(VectorLength); }

  child_range children() {
    return child_range(&VectorLength, &VectorLength + 1);
  }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_vector_length;
  }
};

/// This represents 'seq' clause in the '#pragma acc ...'
/// directive.
///
/// \code
/// #pragma acc loop seq
/// \endcode
/// In this example directive '#pragma acc loop' has clause 'seq'.
class ACCSeqClause : public ACCClause {
public:
  /// Build 'seq' clause.
  ///
  /// \param StartLoc Starting location of the clause.
  /// \param EndLoc Ending location of the clause.
  ACCSeqClause(OpenACCDetermination Determination, SourceLocation StartLoc,
               SourceLocation EndLoc)
      : ACCClause(ACCC_seq, Determination, StartLoc, EndLoc) {}

  /// Build an empty clause.
  ACCSeqClause() : ACCClause(ACCC_seq) {}

  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_seq;
  }
};

/// This represents 'independent' clause in the '#pragma acc ...'
/// directive.
///
/// \code
/// #pragma acc loop independent
/// \endcode
/// In this example directive '#pragma acc loop' has clause 'independent'.
class ACCIndependentClause : public ACCClause {
public:
  /// Build 'independent' clause.
  ///
  /// \param Determination How the clause was determined.
  /// \param StartLoc Starting location of the clause.
  /// \param EndLoc Ending location of the clause.
  ACCIndependentClause(OpenACCDetermination Determination,
                       SourceLocation StartLoc, SourceLocation EndLoc)
      : ACCClause(ACCC_independent, Determination, StartLoc, EndLoc) {}

  /// Build an empty clause.
  ACCIndependentClause() : ACCClause(ACCC_independent) {}

  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_independent;
  }
};

/// This represents 'auto' clause in the '#pragma acc ...'
/// directive.
///
/// \code
/// #pragma acc loop auto
/// \endcode
/// In this example directive '#pragma acc loop' has clause 'auto'.
class ACCAutoClause : public ACCClause {
public:
  /// Build 'auto' clause.
  ///
  /// \param StartLoc Starting location of the clause.
  /// \param EndLoc Ending location of the clause.
  ACCAutoClause(SourceLocation StartLoc, SourceLocation EndLoc)
      : ACCClause(ACCC_auto, ACC_EXPLICIT, StartLoc, EndLoc) {}

  /// Build an empty clause.
  ACCAutoClause() : ACCClause(ACCC_auto) {}

  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_auto;
  }
};

/// This represents 'gang' clause in the '#pragma acc ...' directive.
///
/// \code
/// #pragma acc loop gang
/// \endcode
/// In this example directive '#pragma acc loop' has clause 'gang'.
class ACCGangClause : public ACCClause {
  friend class ACCClauseReader;

  /// Location of '(', or invalid if none.
  SourceLocation LParenLoc;
  /// Location of 'static', or invalid if none.
  SourceLocation StaticKwLoc;
  /// Location of ':' after 'static', or invalid if none.
  SourceLocation StaticColonLoc;
  /// Arg for 'static:' (always an \c Expr, possibly an \c ACCStarExpr), or
  /// \c nullptr if none.
  Stmt *StaticArg = nullptr;

  void setLParenLoc(SourceLocation Loc) { LParenLoc = Loc; }
  void setStaticKwLoc(SourceLocation Loc) { StaticKwLoc = Loc; }
  void setStaticColonLoc(SourceLocation Loc) { StaticColonLoc = Loc; }
  void setStaticArg(Expr *E) { StaticArg = E; }

public:
  /// Build 'gang' clause without an argument.
  ///
  /// \param Determination How the clause was determined.
  /// \param StartLoc Starting location of the clause.
  /// \param EndLoc Ending location of the clause.
  ACCGangClause(OpenACCDetermination Determination, SourceLocation StartLoc,
                SourceLocation EndLoc)
      : ACCClause(ACCC_gang, Determination, StartLoc, EndLoc) {}

  /// Build 'gang' clause with an argument.
  ///
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param StaticKwLoc Location of 'static'.
  /// \param StaticColonLoc Location of ':' after 'static'.
  /// \param StaticArg Arg for 'static:' (possibly an \c ACCStarExpr).
  /// \param EndLoc Location of ')'.
  ///
  /// Called on well-formed 'gang' clause with an argument.
  ACCGangClause(SourceLocation StartLoc, SourceLocation LParenLoc,
                SourceLocation StaticKwLoc, SourceLocation StaticColonLoc,
                Expr *StaticArg, SourceLocation EndLoc)
      : ACCClause(ACCC_gang, ACC_EXPLICIT, StartLoc, EndLoc),
        LParenLoc(LParenLoc), StaticKwLoc(StaticKwLoc),
        StaticColonLoc(StaticColonLoc), StaticArg(StaticArg) {}

  /// Build an empty clause.
  ACCGangClause() : ACCClause(ACCC_gang) {}

  /// Does it have any arguments?
  bool hasArg() const { return StaticArg; }
  /// Return location of '(', or invalid if \c !hasArg().
  SourceLocation getLParenLoc() const { return LParenLoc; }
  /// Return location of 'static', or invalid if none.
  SourceLocation getStaticKwLoc() const { return StaticKwLoc; }
  /// Return location of ':' after 'static', or invalid if none.
  SourceLocation getStaticColonLoc() const { return StaticColonLoc; }
  /// Return arg for 'static:' (possibly an \c ACCStarExpr), or \c nullptr if
  /// none.
  Expr *getStaticArg() const { return cast_or_null<Expr>(StaticArg); }

  child_range children() {
    if (StaticArg)
      return child_range(&StaticArg, &StaticArg + 1);
    return child_range(child_iterator(), child_iterator());
  }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_gang;
  }
};

/// This represents 'worker' clause in the '#pragma acc ...' directive.
///
/// \code
/// #pragma acc loop worker
/// \endcode
/// In this example directive '#pragma acc loop' has clause 'worker'.
class ACCWorkerClause : public ACCClause {
public:
  /// Build 'worker' clause.
  ///
  /// \param Determination How the clause was determined.
  /// \param StartLoc Starting location of the clause.
  /// \param EndLoc Ending location of the clause.
  ACCWorkerClause(OpenACCDetermination Determination, SourceLocation StartLoc,
                  SourceLocation EndLoc)
      : ACCClause(ACCC_worker, Determination, StartLoc, EndLoc) {}

  /// Build an empty clause.
  ACCWorkerClause() : ACCClause(ACCC_worker) {}

  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_worker;
  }
};

/// This represents 'vector' clause in the '#pragma acc ...' directive.
///
/// \code
/// #pragma acc loop vector
/// \endcode
/// In this example directive '#pragma acc loop' has clause 'vector'.
class ACCVectorClause : public ACCClause {
public:
  /// Build 'vector' clause.
  ///
  /// \param Determination How the clause was determined.
  /// \param StartLoc Starting location of the clause.
  /// \param EndLoc Ending location of the clause.
  ACCVectorClause(OpenACCDetermination Determination, SourceLocation StartLoc,
                  SourceLocation EndLoc)
      : ACCClause(ACCC_vector, Determination, StartLoc, EndLoc) {}

  /// Build an empty clause.
  ACCVectorClause() : ACCClause(ACCC_vector) {}

  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_vector;
  }
};

/// This represents 'collapse' clause in the '#pragma acc ...'
/// directive.
///
/// \code
/// #pragma acc loop collapse(n)
/// \endcode
/// In this example directive '#pragma acc loop' has clause 'collapse'
/// with single expression 'n'.
class ACCCollapseClause : public ACCClause {
  friend class ACCClauseReader;

  /// Location of '('.
  SourceLocation LParenLoc;

  /// Original collapse expression.
  Stmt *Collapse = nullptr;

  /// Sets the location of '('.
  void setLParenLoc(SourceLocation Loc) { LParenLoc = Loc; }

  /// Set the original collapse expression.
  ///
  /// \param E collapse expression.
  void setCollapse(Expr *E) { Collapse = E; }

public:
  /// Build 'collapse' clause.
  ///
  /// \param E Original expression associated with this clause.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  ACCCollapseClause(Expr *E, SourceLocation StartLoc, SourceLocation LParenLoc,
                    SourceLocation EndLoc)
      : ACCClause(ACCC_collapse, ACC_EXPLICIT, StartLoc, EndLoc),
        LParenLoc(LParenLoc), Collapse(E) {
  }

  /// Build an empty clause.
  ACCCollapseClause() : ACCClause(ACCC_collapse) {}

  /// Returns the location of '('.
  SourceLocation getLParenLoc() const { return LParenLoc; }

  /// Return the original collapse expression.
  Expr *getCollapse() { return cast<Expr>(Collapse); }

  /// Return the original collapse expression.
  Expr *getCollapse() const { return cast<Expr>(Collapse); }

  child_range children() {
    return child_range(&Collapse, &Collapse + 1);
  }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_collapse;
  }
};

/// This represents the clause 'tile' for '#pragma acc loop' directives.
///
/// \code
/// #pragma acc loop tile(1,2)
/// \endcode
/// In this example directive '#pragma acc loop' has clause 'tile' with the tile
/// sizes '1' and '2'.
class ACCTileClause final
    : public ACCClause,
      private llvm::TrailingObjects<ACCTileClause, Expr *> {
  friend TrailingObjects;
  friend class ACCClauseReader;

  /// Location of '('.
  SourceLocation LParenLoc;
  /// Number of size expressions in the list.
  unsigned NumSizeExprs;

  /// Sets the location of '('.
  void setLParenLoc(SourceLocation Loc) { LParenLoc = Loc; }

  /// Fetches list of size expressions associated with this clause.
  MutableArrayRef<Expr *> getSizeExprs() {
    return MutableArrayRef<Expr *>(getTrailingObjects<Expr *>(), NumSizeExprs);
  }

  /// Sets the list of size expressions for this clause.
  void setSizeExprs(ArrayRef<Expr *> SL) {
    assert(SL.size() == NumSizeExprs &&
           "expected number of size expressions to be the same as in the "
           "preallocated buffer");
    std::copy(SL.begin(), SL.end(), getTrailingObjects<Expr *>());
  }

  /// Build clause with number of size expressions \p N.
  ///
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param NumSizeExprs Number of the size expressions in the clause.
  ACCTileClause(SourceLocation StartLoc, SourceLocation LParenLoc,
                SourceLocation EndLoc, unsigned NumSizeExprs)
      : ACCClause(ACCC_tile, ACC_EXPLICIT, StartLoc, EndLoc, ACCC_tile),
        LParenLoc(LParenLoc), NumSizeExprs(NumSizeExprs) {}

  /// Build an empty clause.
  ///
  /// \param NumSizeExprs Number of the size expressions in the clause.
  explicit ACCTileClause(unsigned NumSizeExprs)
      : ACCClause(ACCC_tile, ACCC_tile), NumSizeExprs(NumSizeExprs) {}

public:
  /// Creates clause with a list of size expressions \p SL.
  ///
  /// \param C AST context.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param SL List of references to the size expressions.
  static ACCTileClause *Create(const ASTContext &C, SourceLocation StartLoc,
                               SourceLocation LParenLoc, SourceLocation EndLoc,
                               ArrayRef<Expr *> SL);
  /// Creates an empty clause with the place for \p N size expressions.
  ///
  /// \param C AST context.
  /// \param N The number of size expressions.
  static ACCTileClause *CreateEmpty(const ASTContext &C, unsigned N);

  typedef MutableArrayRef<Expr *>::iterator sizelist_iterator;
  typedef ArrayRef<const Expr *>::iterator sizelist_const_iterator;
  typedef llvm::iterator_range<sizelist_iterator> sizelist_range;
  typedef llvm::iterator_range<sizelist_const_iterator> sizelist_const_range;

  unsigned sizelist_size() const { return NumSizeExprs; }
  bool sizelist_empty() const { return NumSizeExprs == 0; }

  sizelist_range sizelist() {
    return sizelist_range(sizelist_begin(), sizelist_end());
  }
  sizelist_const_range sizelist() const {
    return sizelist_const_range(sizelist_begin(), sizelist_end());
  }

  sizelist_iterator sizelist_begin() { return getSizeExprs().begin(); }
  sizelist_iterator sizelist_end() { return getSizeExprs().end(); }
  sizelist_const_iterator sizelist_begin() const {
    return getSizeExprs().begin();
  }
  sizelist_const_iterator sizelist_end() const { return getSizeExprs().end(); }

  /// Returns the location of '('.
  SourceLocation getLParenLoc() const { return LParenLoc; }

  /// Fetches list of size expressions associated with this clause.
  ArrayRef<const Expr *> getSizeExprs() const {
    return llvm::ArrayRef(getTrailingObjects<Expr *>(), NumSizeExprs);
  }

  child_range children() {
    return child_range(reinterpret_cast<Stmt **>(sizelist_begin()),
                       reinterpret_cast<Stmt **>(sizelist_end()));
  }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_tile;
  }
};

/// This represents 'async' clause in the '#pragma acc ...' directive.
///
/// \code
/// #pragma acc parallel async(q)
/// \endcode
/// In this example directive '#pragma acc parallel' has clause 'async' with
/// single expression 'q'.
class ACCAsyncClause : public ACCClause {
  friend class ACCClauseReader;

public:
  enum {
    // These enumerator values must be kept in sync with
    // openmp/libacc2omp/src/include/openacc.h.var.
    Acc2ompAccAsyncSync = -1,
    Acc2ompAccAsyncNoval = -2,
    Acc2ompAccAsyncDefault = -3,
  };

  enum AsyncArgStatus {
    AsyncArgIsSync, ///< definitely the synchronous activity queue
    AsyncArgIsAsync, ///< definitely an asynchronous activity queue
    AsyncArgIsUnknown, ///< might be synchronous or asynchronous activity queue
    AsyncArgIsError, ///< invalid async-arg
  };

  static constexpr StringRef Async2DepName = "acc2omp_async2dep";

private:
  /// Location of '('.
  SourceLocation LParenLoc;

  /// Original AsyncArg expression.
  Stmt *AsyncArg = nullptr;

  /// Status of AsyncArg.
  AsyncArgStatus TheAsyncArgStatus = AsyncArgIsError;

  /// The \c acc2omp_async2dep symbol found in scope at this clause, or
  /// \c nullptr if none.
  NamedDecl *Async2Dep = nullptr;

  /// Set the original AsyncArg expression.
  ///
  /// \param E AsyncArg expression.
  void setAsyncArg(Expr *E) { AsyncArg = E; }

  /// Set AsyncArg status, which must not be \c AsyncArgIsError.
  void setAsyncArgStatus(AsyncArgStatus TheAsyncArgStatus) {
    assert(TheAsyncArgStatus != AsyncArgIsError &&
           "expected valid async-arg status");
    this->TheAsyncArgStatus = TheAsyncArgStatus;
  }

  /// Set the \c acc2omp_async2dep symbol found in scope at this clause, or
  /// \c nullptr if none.
  void setAsync2Dep(NamedDecl *ND) { Async2Dep = ND; }

public:
  /// Build 'async' clause.
  ///
  /// \param AsyncArg Original expression associated with this clause, or
  ///        \c nullptr if omitted.
  /// \param TheAsyncArgStatus Status of \c AsyncArg.  Must not be
  ///        \c AsyncArgIsError.
  /// \param Async2Dep The \c acc2omp_async2dep symbol found in scope at this
  ///        clause, or \c nullptr if none.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '(', or an invalid location if argument
  ///        omitted.
  /// \param EndLoc Ending location of the clause.
  ACCAsyncClause(Expr *AsyncArg, AsyncArgStatus TheAsyncArgStatus,
                 NamedDecl *Async2Dep, SourceLocation StartLoc,
                 SourceLocation LParenLoc, SourceLocation EndLoc)
      : ACCClause(ACCC_async, ACC_EXPLICIT, StartLoc, EndLoc),
        LParenLoc(LParenLoc), AsyncArg(AsyncArg),
        TheAsyncArgStatus(TheAsyncArgStatus), Async2Dep(Async2Dep) {
    assert(TheAsyncArgStatus != AsyncArgIsError &&
           "expected valid async-arg status");
  }

  /// Build an empty clause.
  ACCAsyncClause() : ACCClause(ACCC_async) {}

  /// Sets the location of '('.
  void setLParenLoc(SourceLocation Loc) { LParenLoc = Loc; }

  /// Returns the location of '(', invalid if argument omitted.
  SourceLocation getLParenLoc() const { return LParenLoc; }

  /// Return the original AsyncArg expression or \c nullptr if omitted.
  Expr *getAsyncArg() { return cast_or_null<Expr>(AsyncArg); }

  /// Return the original AsyncArg expression or \c nullptr if omitted.
  Expr *getAsyncArg() const { return cast_or_null<Expr>(AsyncArg); }

  /// Return AsyncArg status, which is never \c AsyncArgIsError.
  AsyncArgStatus getAsyncArgStatus() const { return TheAsyncArgStatus; }

  /// Get the \c acc2omp_async2dep symbol found in scope at this clause, or
  /// \c nullptr if none.
  NamedDecl *getAsync2Dep() const { return Async2Dep; }

  child_range children() { return child_range(&AsyncArg, &AsyncArg + 1); }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_async;
  }
};

/// This represents 'wait' clause in the '#pragma acc ...' directive.
///
/// \code
/// #pragma acc parallel wait(q)
/// \endcode
/// In this example directive '#pragma acc parallel' has clause 'wait' with
/// single expression 'q'.
class ACCWaitClause final
    : public ACCClause,
      private llvm::TrailingObjects<ACCWaitClause, Expr *> {
  friend TrailingObjects;
  friend class ACCClauseReader;

  /// Location of '('.
  SourceLocation LParenLoc;
  /// Number of queue expressions in the list.
  unsigned NumQueueExprs;

  /// Sets the location of '('.
  void setLParenLoc(SourceLocation Loc) { LParenLoc = Loc; }

  /// Fetches list of queue expressions associated with this clause.
  MutableArrayRef<Expr *> getQueueExprs() {
    return MutableArrayRef<Expr *>(getTrailingObjects<Expr *>(), NumQueueExprs);
  }

  /// Sets the list of queue expressions for this clause.
  void setQueueExprs(ArrayRef<Expr *> SL) {
    assert(SL.size() == NumQueueExprs &&
           "expected number of queue expressions to be the same as in the "
           "preallocated buffer");
    std::copy(SL.begin(), SL.end(), getTrailingObjects<Expr *>());
  }

  /// Build clause with number of queue expressions \p N.
  ///
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param NumSizeExprs Number of the queue expressions in the clause.
  ACCWaitClause(SourceLocation StartLoc, SourceLocation LParenLoc,
                SourceLocation EndLoc, unsigned NumQueueExprs)
      : ACCClause(ACCC_wait, ACC_EXPLICIT, StartLoc, EndLoc, ACCC_wait),
        LParenLoc(LParenLoc), NumQueueExprs(NumQueueExprs) {}

  /// Build an empty clause.
  ///
  /// \param NumQueueExprs Number of the size expressions in the clause.
  explicit ACCWaitClause(unsigned NumQueueExprs)
      : ACCClause(ACCC_wait, ACCC_wait), NumQueueExprs(NumQueueExprs) {}

public:
  /// Creates clause with a list of queue expressions \p QL.
  ///
  /// \param C AST context.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '(', or invalid if argument omitted.
  /// \param EndLoc Ending location of the clause.
  /// \param QL List of references to the queue expressions.
  static ACCWaitClause *Create(const ASTContext &C, SourceLocation StartLoc,
                               SourceLocation LParenLoc, SourceLocation EndLoc,
                               ArrayRef<Expr *> QL);
  /// Creates an empty clause with the place for \p N queue expressions.
  ///
  /// \param C AST context.
  /// \param N The number of queue expressions.
  static ACCWaitClause *CreateEmpty(const ASTContext &C, unsigned N);

  typedef MutableArrayRef<Expr *>::iterator queuelist_iterator;
  typedef ArrayRef<const Expr *>::iterator queuelist_const_iterator;
  typedef llvm::iterator_range<queuelist_iterator> queuelist_range;
  typedef llvm::iterator_range<queuelist_const_iterator> queuelist_const_range;

  unsigned queuelist_size() const { return NumQueueExprs; }
  bool queuelist_empty() const { return NumQueueExprs == 0; }

  queuelist_range queuelists() {
    return queuelist_range(queuelist_begin(), queuelist_end());
  }
  queuelist_const_range queuelists() const {
    return queuelist_const_range(queuelist_begin(), queuelist_end());
  }

  queuelist_iterator queuelist_begin() { return getQueueExprs().begin(); }
  queuelist_iterator queuelist_end() { return getQueueExprs().end(); }
  queuelist_const_iterator queuelist_begin() const {
    return getQueueExprs().begin();
  }
  queuelist_const_iterator queuelist_end() const {
    return getQueueExprs().end();
  }

  /// Returns the location of '(', invalid if argument omitted.
  SourceLocation getLParenLoc() const { return LParenLoc; }

  /// Fetches list of queue expressions associated with this clause.  It's
  /// empty if none.
  ArrayRef<const Expr *> getQueueExprs() const {
    return llvm::ArrayRef(getTrailingObjects<Expr *>(), NumQueueExprs);
  }

  child_range children() {
    return child_range(reinterpret_cast<Stmt **>(queuelist_begin()),
                       reinterpret_cast<Stmt **>(queuelist_end()));
  }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_wait;
  }
};

/// This represents 'read' clause in the '#pragma acc atomic' directive.
///
/// \code
/// #pragma acc atomic read
/// \endcode
/// In this example directive '#pragma acc atomic' has clause 'read'.
class ACCReadClause : public ACCClause {
public:
  /// Build 'read' clause.
  ///
  /// \param StartLoc Starting location of the clause.
  /// \param EndLoc Ending location of the clause.
  ACCReadClause(SourceLocation StartLoc, SourceLocation EndLoc)
      : ACCClause(ACCC_read, ACC_EXPLICIT, StartLoc, EndLoc) {}

  /// Build an empty clause.
  ACCReadClause() : ACCClause(ACCC_read) {}

  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_read;
  }
};

/// This represents 'write' clause in the '#pragma acc atomic' directive.
///
/// \code
/// #pragma acc atomic write
/// \endcode
/// In this example directive '#pragma acc atomic' has clause 'write'.
class ACCWriteClause : public ACCClause {
public:
  /// Build 'write' clause.
  ///
  /// \param StartLoc Starting location of the clause.
  /// \param EndLoc Ending location of the clause.
  ACCWriteClause(SourceLocation StartLoc, SourceLocation EndLoc)
      : ACCClause(ACCC_write, ACC_EXPLICIT, StartLoc, EndLoc) {}

  /// Build an empty clause.
  ACCWriteClause() : ACCClause(ACCC_write) {}

  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_write;
  }
};

/// This represents 'update' clause in the '#pragma acc atomic' directive.
///
/// \code
/// #pragma acc atomic update
/// \endcode
/// In this example directive '#pragma acc atomic' has clause 'update'.
class ACCUpdateClause : public ACCClause {
public:
  /// Build 'update' clause.
  ///
  /// \param Determination How the clause was determined.
  /// \param StartLoc Starting location of the clause.
  /// \param EndLoc Ending location of the clause.
  ACCUpdateClause(OpenACCDetermination Determination, SourceLocation StartLoc,
                  SourceLocation EndLoc)
      : ACCClause(ACCC_update, Determination, StartLoc, EndLoc) {}

  /// Build an empty clause.
  ACCUpdateClause() : ACCClause(ACCC_update) {}

  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_update;
  }
};

/// This represents 'capture' clause in the '#pragma acc atomic' directive.
///
/// \code
/// #pragma acc atomic capture
/// \endcode
/// In this example directive '#pragma acc atomic' has clause 'capture'.
class ACCCaptureClause : public ACCClause {
public:
  /// Build 'capture' clause.
  ///
  /// \param StartLoc Starting location of the clause.
  /// \param EndLoc Ending location of the clause.
  ACCCaptureClause(SourceLocation StartLoc, SourceLocation EndLoc)
      : ACCClause(ACCC_capture, ACC_EXPLICIT, StartLoc, EndLoc) {}

  /// Build an empty clause.
  ACCCaptureClause() : ACCClause(ACCC_capture) {}

  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_capture;
  }
};

/// This represents 'compare' clause in the '#pragma acc atomic' directive.
///
/// \code
/// #pragma acc atomic compare
/// \endcode
/// In this example directive '#pragma acc atomic' has clause 'compare'.
class ACCCompareClause : public ACCClause {
public:
  /// Build 'compare' clause.
  ///
  /// \param StartLoc Starting location of the clause.
  /// \param EndLoc Ending location of the clause.
  ACCCompareClause(SourceLocation StartLoc, SourceLocation EndLoc)
      : ACCClause(ACCC_compare, ACC_EXPLICIT, StartLoc, EndLoc) {}

  /// Build an empty clause.
  ACCCompareClause() : ACCClause(ACCC_compare) {}

  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }

  static bool classof(const ACCClause *T) {
    return T->getClauseKind() == ACCC_compare;
  }
};

/// This class implements a simple visitor for ACCClause
/// subclasses.
template<class ImplClass, template <typename> class Ptr, typename RetTy>
class ACCClauseVisitorBase {
public:
#define PTR(CLASS) Ptr<CLASS>
#define DISPATCH(CLASS) \
  return static_cast<ImplClass*>(this)->Visit##CLASS(static_cast<PTR(CLASS)>(S))

#define OPENACC_CLAUSE(Name, Class)                              \
  RetTy Visit ## Class (PTR(Class) S) { DISPATCH(Class); }
#include "clang/Basic/OpenACCKinds.def"

  RetTy Visit(PTR(ACCClause) S) {
    // Top switch clause: visit each ACCClause.
    switch (S->getClauseKind()) {
    default: llvm_unreachable("Unknown clause kind!");
#define OPENACC_CLAUSE(Name, Class) \
    case ACCC_##Name:               \
      return Visit ## Class(static_cast<PTR(Class)>(S));
#define OPENACC_CLAUSE_ALIAS(ClauseAlias, AliasedClause, Class) \
    case ACCC_##ClauseAlias:                                    \
      return Visit ## Class(static_cast<PTR(Class)>(S));
#include "clang/Basic/OpenACCKinds.def"
    }
  }
  // Base case, ignore it. :)
  RetTy VisitACCClause(PTR(ACCClause) Node) { return RetTy(); }
#undef PTR
#undef DISPATCH
};

template <typename T> using const_ptr = std::add_pointer_t<std::add_const_t<T>>;

template <class ImplClass, typename RetTy = void>
class ACCClauseVisitor
    : public ACCClauseVisitorBase<ImplClass, std::add_pointer_t, RetTy> {};
template <class ImplClass, typename RetTy = void>
class ConstACCClauseVisitor
    : public ACCClauseVisitorBase<ImplClass, const_ptr, RetTy> {};
} // end namespace clang

#endif // LLVM_CLANG_AST_OPENACCCLAUSE_H
