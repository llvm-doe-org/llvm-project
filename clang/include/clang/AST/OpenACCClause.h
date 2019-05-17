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
///
class ACCClause {
  friend class ACCClauseReader;

  /// Starting location of the clause (the clause keyword).
  SourceLocation StartLoc;
  /// Ending location of the clause.
  SourceLocation EndLoc;
  /// Kind of the clause.
  OpenACCClauseKind Kind;

  /// Sets the starting location of the clause.
  void setLocStart(SourceLocation Loc) { StartLoc = Loc; }
  /// Sets the ending location of the clause.
  void setLocEnd(SourceLocation Loc) { EndLoc = Loc; }

protected:
  ACCClause(OpenACCClauseKind K, SourceLocation StartLoc, SourceLocation EndLoc)
      : StartLoc(StartLoc), EndLoc(EndLoc), Kind(K) {}

public:
  /// Returns the starting location of the clause.
  SourceLocation getBeginLoc() const { return StartLoc; }
  /// Returns the ending location of the clause.
  SourceLocation getEndLoc() const { return EndLoc; }

  /// Returns kind of OpenACC clause (private, shared, reduction, etc.).
  OpenACCClauseKind getClauseKind() const { return Kind; }

  bool isImplicit() const { return StartLoc.isInvalid(); }

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

/// This represents clauses with the list of variables like 'private',
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

  /// Build a clause with \a N variables
  ///
  /// \param K Kind of the clause.
  /// \param StartLoc Starting location of the clause (the clause keyword).
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param N Number of the variables in the clause.
  ///
  ACCVarListClause(OpenACCClauseKind K, SourceLocation StartLoc,
                   SourceLocation LParenLoc, SourceLocation EndLoc, unsigned N)
      : ACCClause(K, StartLoc, EndLoc), LParenLoc(LParenLoc), NumVars(N) {}

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
    return llvm::makeArrayRef(
        static_cast<const T *>(this)->template getTrailingObjects<Expr *>(),
        NumVars);
  }
};

/// This represents the implicit clause 'shared' for '#pragma acc ...'.
///
/// These clauses are computed implicitly by clang.  Currently, OpenACC does
/// not define an explicit version, so clang does not accept one.
///
class ACCSharedClause final
    : public ACCVarListClause<ACCSharedClause>,
      private llvm::TrailingObjects<ACCSharedClause, Expr *> {
  friend TrailingObjects;
  friend ACCVarListClause;
  friend class ACCClauseReader;

  /// Build clause with number of variables \a N.
  ///
  /// \param N Number of the variables in the clause.
  explicit ACCSharedClause(unsigned N)
      : ACCVarListClause<ACCSharedClause>(ACCC_shared, SourceLocation(), SourceLocation(),
                                          SourceLocation(), N) {}

public:
  /// Creates clause with a list of variables \a VL.
  ///
  /// \param C AST context.
  /// \param VL List of references to the variables.
  ///
  static ACCSharedClause *Create(const ASTContext &C, ArrayRef<Expr *> VL);
  /// Creates an empty clause with the place for \a N variables.
  ///
  /// \param C AST context.
  /// \param N The number of variables.
  ///
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
///
class ACCPrivateClause final
    : public ACCVarListClause<ACCPrivateClause>,
      private llvm::TrailingObjects<ACCPrivateClause, Expr *> {
  friend TrailingObjects;
  friend ACCVarListClause;
  friend class ACCClauseReader;

  /// Build clause with number of variables \a N.
  ///
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param N Number of the variables in the clause.
  ///
  ACCPrivateClause(SourceLocation StartLoc, SourceLocation LParenLoc,
                   SourceLocation EndLoc, unsigned N)
      : ACCVarListClause<ACCPrivateClause>(ACCC_private, StartLoc, LParenLoc,
                                           EndLoc, N) {}

  /// Build an empty clause.
  ///
  /// \param N Number of variables.
  ///
  explicit ACCPrivateClause(unsigned N)
      : ACCVarListClause<ACCPrivateClause>(ACCC_private, SourceLocation(),
                                           SourceLocation(), SourceLocation(),
                                           N) {}

public:
  /// Creates clause with a list of variables \a VL.
  ///
  /// \param C AST context.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param VL List of references to the variables.
  ///
  static ACCPrivateClause *Create(const ASTContext &C, SourceLocation StartLoc,
                                  SourceLocation LParenLoc,
                                  SourceLocation EndLoc, ArrayRef<Expr *> VL);
  /// Creates an empty clause with the place for \a N variables.
  ///
  /// \param C AST context.
  /// \param N The number of variables.
  ///
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
///
class ACCFirstprivateClause final
    : public ACCVarListClause<ACCFirstprivateClause>,
      private llvm::TrailingObjects<ACCFirstprivateClause, Expr *> {
  friend TrailingObjects;
  friend ACCVarListClause;
  friend class ACCClauseReader;

  /// Build clause with number of variables \a N.
  ///
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param N Number of the variables in the clause.
  ///
  ACCFirstprivateClause(SourceLocation StartLoc, SourceLocation LParenLoc,
                        SourceLocation EndLoc, unsigned N)
      : ACCVarListClause<ACCFirstprivateClause>(ACCC_firstprivate, StartLoc,
                                                LParenLoc, EndLoc, N) {}

  /// Build an empty clause.
  ///
  /// \param N Number of variables.
  ///
  explicit ACCFirstprivateClause(unsigned N)
      : ACCVarListClause<ACCFirstprivateClause>(
            ACCC_firstprivate, SourceLocation(), SourceLocation(),
            SourceLocation(), N) {}

public:
  /// Creates clause with a list of variables \a VL.
  ///
  /// \param C AST context.
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param VL List of references to the original variables.
  ///
  static ACCFirstprivateClause *
  Create(const ASTContext &C, SourceLocation StartLoc, SourceLocation LParenLoc,
         SourceLocation EndLoc, ArrayRef<Expr *> VL);
  /// Creates an empty clause with the place for \a N variables.
  ///
  /// \param C AST context.
  /// \param N The number of variables.
  ///
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
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param EndLoc Ending location of the clause.
  /// \param ColonLoc Location of ':'.
  /// \param N Number of the variables in the clause.
  /// \param NameInfo The full name info for reduction operator.
  ACCReductionClause(SourceLocation StartLoc, SourceLocation LParenLoc,
                     SourceLocation ColonLoc, SourceLocation EndLoc, unsigned N,
                     const DeclarationNameInfo &NameInfo)
      : ACCVarListClause<ACCReductionClause>(ACCC_reduction, StartLoc,
                                             LParenLoc, EndLoc, N),
        ColonLoc(ColonLoc), NameInfo(NameInfo) {}

  /// Build an empty clause.
  ///
  /// \param N Number of variables.
  explicit ACCReductionClause(unsigned N)
      : ACCVarListClause<ACCReductionClause>(ACCC_reduction, SourceLocation(),
                                             SourceLocation(), SourceLocation(),
                                             N) {}

  /// Sets location of ':' symbol in clause.
  void setColonLoc(SourceLocation CL) { ColonLoc = CL; }

  /// Sets the name info for specified reduction operator.
  void setNameInfo(DeclarationNameInfo DNI) { NameInfo = DNI; }

public:
  /// Creates clause with a list of variables \a VL.
  ///
  /// \param StartLoc Starting location of the clause.
  /// \param LParenLoc Location of '('.
  /// \param ColonLoc Location of ':'.
  /// \param EndLoc Ending location of the clause.
  /// \param VL The variables in the clause.
  /// \param NameInfo The full name info for reduction operator.
  static ACCReductionClause *
  Create(const ASTContext &C, SourceLocation StartLoc,
         SourceLocation LParenLoc, SourceLocation ColonLoc,
         SourceLocation EndLoc, ArrayRef<Expr *> VL,
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
      : ACCClause(ACCC_num_gangs, StartLoc, EndLoc), LParenLoc(LParenLoc),
        NumGangs(E) {
  }

  /// Build an empty clause.
  ACCNumGangsClause()
      : ACCClause(ACCC_num_gangs, SourceLocation(), SourceLocation()) {}

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
      : ACCClause(ACCC_num_workers, StartLoc, EndLoc), LParenLoc(LParenLoc),
        NumWorkers(E) {
  }

  /// Build an empty clause.
  ACCNumWorkersClause()
      : ACCClause(ACCC_num_workers, SourceLocation(), SourceLocation()) {}

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
      : ACCClause(ACCC_vector_length, StartLoc, EndLoc), LParenLoc(LParenLoc),
        VectorLength(E) {
  }

  /// Build an empty clause.
  ACCVectorLengthClause()
      : ACCClause(ACCC_vector_length, SourceLocation(), SourceLocation()) {}

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
  ACCSeqClause(SourceLocation StartLoc, SourceLocation EndLoc)
      : ACCClause(ACCC_seq, StartLoc, EndLoc) {}

  /// Build an empty clause.
  ACCSeqClause()
      : ACCClause(ACCC_seq, SourceLocation(), SourceLocation()) {}

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
  /// \param StartLoc Starting location of the clause.
  /// \param EndLoc Ending location of the clause.
  ACCIndependentClause(SourceLocation StartLoc, SourceLocation EndLoc)
      : ACCClause(ACCC_independent, StartLoc, EndLoc) {}

  /// Build an empty clause.
  ACCIndependentClause()
      : ACCClause(ACCC_independent, SourceLocation(), SourceLocation()) {}

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
      : ACCClause(ACCC_auto, StartLoc, EndLoc) {}

  /// Build an empty clause.
  ACCAutoClause()
      : ACCClause(ACCC_auto, SourceLocation(), SourceLocation()) {}

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
public:
  /// Build 'gang' clause.
  ///
  /// \param StartLoc Starting location of the clause.
  /// \param EndLoc Ending location of the clause.
  ACCGangClause(SourceLocation StartLoc, SourceLocation EndLoc)
      : ACCClause(ACCC_gang, StartLoc, EndLoc) {}

  /// Build an empty clause.
  ACCGangClause() : ACCClause(ACCC_gang, SourceLocation(), SourceLocation()) {}

  child_range children() {
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
  /// \param StartLoc Starting location of the clause.
  /// \param EndLoc Ending location of the clause.
  ACCWorkerClause(SourceLocation StartLoc, SourceLocation EndLoc)
      : ACCClause(ACCC_worker, StartLoc, EndLoc) {}

  /// Build an empty clause.
  ACCWorkerClause()
      : ACCClause(ACCC_worker, SourceLocation(), SourceLocation()) {}

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
  /// \param StartLoc Starting location of the clause.
  /// \param EndLoc Ending location of the clause.
  ACCVectorClause(SourceLocation StartLoc, SourceLocation EndLoc)
      : ACCClause(ACCC_vector, StartLoc, EndLoc) {}

  /// Build an empty clause.
  ACCVectorClause()
      : ACCClause(ACCC_vector, SourceLocation(), SourceLocation()) {}

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
      : ACCClause(ACCC_collapse, StartLoc, EndLoc), LParenLoc(LParenLoc),
        Collapse(E) {
  }

  /// Build an empty clause.
  ACCCollapseClause()
      : ACCClause(ACCC_collapse, SourceLocation(), SourceLocation()) {}

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

/// This class implements a simple visitor for ACCClause
/// subclasses.
template<class ImplClass, template <typename> class Ptr, typename RetTy>
class ACCClauseVisitorBase {
public:
#define PTR(CLASS) typename Ptr<CLASS>::type
#define DISPATCH(CLASS) \
  return static_cast<ImplClass*>(this)->Visit##CLASS(static_cast<PTR(CLASS)>(S))

#define OPENACC_CLAUSE(Name, Class)                              \
  RetTy Visit ## Class (PTR(Class) S) { DISPATCH(Class); }
#include "clang/Basic/OpenACCKinds.def"

  RetTy Visit(PTR(ACCClause) S) {
    // Top switch clause: visit each ACCClause.
    switch (S->getClauseKind()) {
    default: llvm_unreachable("Unknown clause kind!");
#define OPENACC_CLAUSE(Name, Class)                              \
    case ACCC_ ## Name : return Visit ## Class(static_cast<PTR(Class)>(S));
#include "clang/Basic/OpenACCKinds.def"
    }
  }
  // Base case, ignore it. :)
  RetTy VisitACCClause(PTR(ACCClause) Node) { return RetTy(); }
#undef PTR
#undef DISPATCH
};

template <typename T>
using const_ptr = typename std::add_pointer<typename std::add_const<T>::type>;

template<class ImplClass, typename RetTy = void>
class ACCClauseVisitor :
      public ACCClauseVisitorBase <ImplClass, std::add_pointer, RetTy> {};
template<class ImplClass, typename RetTy = void>
class ConstACCClauseVisitor :
      public ACCClauseVisitorBase <ImplClass, const_ptr, RetTy> {};
} // end namespace clang

#endif // LLVM_CLANG_AST_OPENACCCLAUSE_H
