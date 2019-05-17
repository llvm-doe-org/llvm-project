//===- StmtOpenACC.h - Classes for OpenACC directives  ----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// This file defines OpenACC AST classes for executable directives and
/// clauses.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_STMTOPENACC_H
#define LLVM_CLANG_AST_STMTOPENACC_H

#include "clang/AST/Expr.h"
#include "clang/AST/OpenACCClause.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/StmtOpenMP.h"
#include "clang/Basic/OpenACCKinds.h"
#include "clang/Basic/SourceLocation.h"

namespace clang {

//===----------------------------------------------------------------------===//
// AST classes for directives.
//===----------------------------------------------------------------------===//

/// This is a basic class for representing single OpenACC executable
/// directive.
///
class ACCExecutableDirective : public Stmt {
  friend class ASTStmtReader;
  /// Kind of the directive.
  OpenACCDirectiveKind Kind;
  /// Starting location of the directive (directive keyword).
  SourceLocation StartLoc;
  /// Ending location of the directive.
  SourceLocation EndLoc;
  /// Number of clauses.
  const unsigned NumClauses;
  /// Number of child expressions/stmts.
  const unsigned NumChildren;
  /// Offset from this to the start of clauses.
  /// There are NumClauses pointers to clauses, they are followed by
  /// NumChildren pointers to child stmts/exprs (if the directive type
  /// requires an associated stmt, then it has to be the first of them).
  const unsigned ClausesOffset;
  ACCExecutableDirective *EffectiveDirective = nullptr;
  Stmt *OMPNode;
  bool DirectiveDiscardedForOMP;

  /// Get the clauses storage.
  MutableArrayRef<ACCClause *> getClauses() {
    ACCClause **ClauseStorage = reinterpret_cast<ACCClause **>(
        reinterpret_cast<char *>(this) + ClausesOffset);
    return MutableArrayRef<ACCClause *>(ClauseStorage, NumClauses);
  }

protected:
  /// Build instance of directive of class \a K.
  ///
  /// \param SC Statement class.
  /// \param K Kind of OpenACC directive.
  /// \param StartLoc Starting location of the directive (directive keyword).
  /// \param EndLoc Ending location of the directive.
  ///
  template <typename T>
  ACCExecutableDirective(const T *, StmtClass SC, OpenACCDirectiveKind K,
                         SourceLocation StartLoc, SourceLocation EndLoc,
                         unsigned NumClauses, unsigned NumChildren)
      : Stmt(SC), Kind(K), StartLoc(std::move(StartLoc)),
        EndLoc(std::move(EndLoc)), NumClauses(NumClauses),
        NumChildren(NumChildren),
        ClausesOffset(llvm::alignTo(sizeof(T), alignof(ACCClause *))),
        OMPNode(nullptr), DirectiveDiscardedForOMP(false) {}

  /// Sets the list of variables for this clause.
  ///
  /// \param Clauses The list of clauses for the directive.
  ///
  void setClauses(ArrayRef<ACCClause *> Clauses);

  /// Set the associated statement for the directive.
  ///
  /// \param S Associated statement.
  ///
  void setAssociatedStmt(Stmt *S) {
    assert(hasAssociatedStmt() && "no associated statement.");
    *child_begin() = S;
  }

  /// Set the outermost effective directive.  Non-combined directives must not
  /// call this.
  ///
  /// \param D Effective directive.
  ///
  void setEffectiveDirective(ACCExecutableDirective *D) {
    assert(isOpenACCCombinedDirective(Kind) &&
           "expected combined OpenACC directive");
    EffectiveDirective = D;
  }

public:
  static bool classof(const Stmt *S) {
    return S->getStmtClass() >= firstACCExecutableDirectiveConstant &&
           S->getStmtClass() <= lastACCExecutableDirectiveConstant;
  }

  /// Set the statement to which this directive has been translated for OpenMP.
  /// In most cases, it's an OpenMP directive.  In some cases, it's a compound
  /// statement that contains an OpenMP directive.  If the OpenACC directive
  /// was discarded rather than translated to an OpenMP directive, set
  /// DirectiveDiscardedForOMP to true.
  ///
  /// \param OMPNode The statement.
  /// \param DirectiveDiscardedForOMP Whether the OpenACC directive was simply
  ///        discarded for translation to OpenMP.
  void setOMPNode(Stmt *OMPNode, bool DirectiveDiscardedForOMP = false) {
    assert(this->OMPNode == nullptr && !this->DirectiveDiscardedForOMP &&
           "expected OMPNode not to be set already");
    assert(OMPNode != nullptr && "expected OMPNode");
    assert((!DirectiveDiscardedForOMP || !isa<OMPExecutableDirective>(OMPNode)) &&
           "expected OMPNode not to be an OpenMP directive when OpenMP"
           " directive discarded");
    this->OMPNode = OMPNode;
    this->DirectiveDiscardedForOMP = DirectiveDiscardedForOMP;
  }

  /// Has this directive been translated to OpenMP?
  bool hasOMPNode() const { return OMPNode; }

  /// Get the statement to which this directive has been translated for OpenMP.
  Stmt *getOMPNode() const {
    assert(OMPNode && "expected to have OMPNode already");
    return OMPNode;
  }

  /// Was the OpenACC directive discarded rather than translated to an OpenMP
  /// directive?
  bool directiveDiscardedForOMP() const {
    assert(OMPNode && "expected to have OMPNode already");
    return DirectiveDiscardedForOMP;
  }

  // Are the OpenACC and OpenMP versions of an OpenACC construct different
  // enough that, when printing both versions, the associated statements must
  // be printed separately?
  //
  // For example, StmtPrinter prints both versions (one within comments) when
  // Policy.OpenACCPrint is OpenACCPrint_ACC_OMP or OpenACCPrint_OMP_ACC.  When
  // the result of ompStmtPrintsDifferently is true, StmtPrinter must print the
  // OpenACC directive plus its associated statement completely separately (one
  // within comments) from the OpenMP directive plus its associated statement.
  // When the result is false, StmtPrinter prints the OpenACC directive
  // separately (one within comments) from the OpenMP directive but prints the
  // associated statement once afterward.
  //
  // ompStmtPrintsDifferently makes its determination by checking whether all
  // portions of the associated statements except nested OpenACC regions are
  // identical when printed.  The reason it doesn't check nested OpenACC
  // regions is that they can be split if necessary within this region (it
  // checks them for the need to split when its reaches them while printing
  // this region).  A degenerate case is when there is no associated statement
  // at all (standalone directive), and then it returns false because there's
  // nothing to split. Another special case is when the OpenMP version is not
  // an OpenMP directive (OpenACC directive was dropped but perhaps some new
  // code was inserted into a new compound statement enclosing the possibly
  // transformed associated statement), and then it just compares the entire
  // OpenMP version with the OpenACC version's associated statement.
  //
  // \param Policy The base printing policy to use for comparisons.
  // \param Context AST context.
  bool ompStmtPrintsDifferently(const PrintingPolicy &Policy,
                                const ASTContext *Context);

  /// Iterates over a filtered subrange of clauses applied to a
  /// directive.
  ///
  /// This iterator visits only clauses of type SpecificClause.
  template <typename SpecificClause>
  class specific_clause_iterator
      : public llvm::iterator_adaptor_base<
            specific_clause_iterator<SpecificClause>,
            ArrayRef<ACCClause *>::const_iterator, std::forward_iterator_tag,
            SpecificClause *, ptrdiff_t, SpecificClause *,
            SpecificClause *> {
    ArrayRef<ACCClause *>::const_iterator End;

    void SkipToNextClause() {
      while (this->I != End && !isa<SpecificClause>(*this->I))
        ++this->I;
    }

  public:
    explicit specific_clause_iterator(ArrayRef<ACCClause *> Clauses)
        : specific_clause_iterator::iterator_adaptor_base(Clauses.begin()),
          End(Clauses.end()) {
      SkipToNextClause();
    }

    SpecificClause *operator*() const {
      return cast<SpecificClause>(*this->I);
    }
    SpecificClause *operator->() const { return **this; }

    specific_clause_iterator &operator++() {
      ++this->I;
      SkipToNextClause();
      return *this;
    }
  };

  template <typename SpecificClause>
  static llvm::iterator_range<specific_clause_iterator<SpecificClause>>
  getClausesOfKind(ArrayRef<ACCClause *> Clauses) {
    return {specific_clause_iterator<SpecificClause>(Clauses),
            specific_clause_iterator<SpecificClause>(
                llvm::makeArrayRef(Clauses.end(), 0))};
  }

  template <typename SpecificClause>
  llvm::iterator_range<specific_clause_iterator<SpecificClause>>
  getClausesOfKind() const {
    return getClausesOfKind<SpecificClause>(clauses());
  }

  /// Returns true if the current directive has one or more clauses of a
  /// specific kind.
  template <typename SpecificClause>
  bool hasClausesOfKind() {
    auto Clauses = getClausesOfKind<SpecificClause>();
    return Clauses.begin() != Clauses.end();
  }

  /// Returns starting location of directive kind.
  SourceLocation getBeginLoc() const { return StartLoc; }
  /// Returns ending location of directive.
  SourceLocation getEndLoc() const { return EndLoc; }
  /// Gets the source range covering the '#pragma' through the end of any
  /// associated statement.
  SourceRange getConstructRange() const;
  /// Gets the source range covering the '#pragma' through the end of just
  /// the directive without any associated statement.
  SourceRange getDirectiveRange() const {
    return SourceRange(StartLoc, EndLoc);
  }

  /// Set starting location of directive kind.
  ///
  /// \param Loc New starting location of directive.
  ///
  void setLocStart(SourceLocation Loc) { StartLoc = Loc; }
  /// Set ending location of directive.
  ///
  /// \param Loc New ending location of directive.
  ///
  void setLocEnd(SourceLocation Loc) { EndLoc = Loc; }

  /// Get number of clauses.
  unsigned getNumClauses() const { return NumClauses; }

  /// Returns specified clause.
  ///
  /// \param i Number of clause.
  ///
  ACCClause *getClause(unsigned i) const { return clauses()[i]; }

  /// Returns true if directive has associated statement.
  bool hasAssociatedStmt() const { return NumChildren > 0; }

  /// Returns statement associated with the directive.
  Stmt *getAssociatedStmt() const {
    assert(hasAssociatedStmt() && "no associated statement.");
    return const_cast<Stmt *>(*child_begin());
  }

  /// Returns the outermost effective directive (whose associated statement is
  /// the nested effective directive, etc.) if this is a combined directive,
  /// and returns nullptr otherwise.
  ACCExecutableDirective *getEffectiveDirective() const {
    return EffectiveDirective;
  }

  OpenACCDirectiveKind getDirectiveKind() const { return Kind; }

  child_range children() {
    if (!hasAssociatedStmt())
      return child_range(child_iterator(), child_iterator());
    Stmt **ChildStorage = reinterpret_cast<Stmt **>(getClauses().end());
    return child_range(ChildStorage, ChildStorage + NumChildren);
  }

  ArrayRef<ACCClause *> clauses() { return getClauses(); }

  ArrayRef<ACCClause *> clauses() const {
    return const_cast<ACCExecutableDirective *>(this)->getClauses();
  }
};

/// This represents '#pragma acc parallel' directive.
///
class ACCParallelDirective : public ACCExecutableDirective {
  friend class ASTStmtReader;
  bool NestedWorkerPartitioning = false;

  /// Build directive with the given start and end location.
  ///
  /// \param StartLoc Starting location of the directive (directive keyword).
  /// \param EndLoc Ending Location of the directive.
  ///
  ACCParallelDirective(SourceLocation StartLoc, SourceLocation EndLoc,
                       unsigned NumClauses)
      : ACCExecutableDirective(this, ACCParallelDirectiveClass, ACCD_parallel,
                               StartLoc, EndLoc, NumClauses, 1)
        {}

  /// Build an empty directive.
  explicit ACCParallelDirective(unsigned NumClauses)
      : ACCExecutableDirective(this, ACCParallelDirectiveClass, ACCD_parallel,
                               SourceLocation(), SourceLocation(), NumClauses,
                               1)
        {}

public:
  /// Creates directive.
  ///
  /// \param C AST context.
  /// \param StartLoc Starting location of the directive kind.
  /// \param EndLoc Ending Location of the directive.
  /// \param Clauses List of clauses.
  /// \param AssociatedStmt Statement associated with the directive.
  /// \param NestedWorkerPartitioning Whether a (separate or combined with this
  ///        directive) acc loop directive with worker partitioning is nested
  ///        here.
  static ACCParallelDirective *
  Create(const ASTContext &C, SourceLocation StartLoc, SourceLocation EndLoc,
         ArrayRef<ACCClause *> Clauses, Stmt *AssociatedStmt,
         bool NestedWorkerPartitioning);

  /// Creates an empty directive.
  ///
  /// \param C AST context.
  /// \param NumClauses Number of clauses.
  ///
  static ACCParallelDirective *CreateEmpty(const ASTContext &C,
                                           unsigned NumClauses, EmptyShell);

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == ACCParallelDirectiveClass;
  }

  /// Record whether a (separate or combined with this directive) acc loop
  /// directive with worker partitioning is nested here.
  void setNestedWorkerPartitioning(bool V) { NestedWorkerPartitioning = V; }
  /// Return true if a (separate or combined with this directive) acc loop
  /// directive with worker partitioning is nested here.
  bool getNestedWorkerPartitioning() const { return NestedWorkerPartitioning; }
};

/// This represents '#pragma acc loop' directive.
///
/// \code
/// #pragma acc loop private(a,b) reduction(+: c,d)
/// \endcode
/// In this example directive '#pragma acc loop' has clauses 'private'
/// with the variables 'a' and 'b' and 'reduction' with operator '+' and
/// variables 'c' and 'd'.
///
class ACCLoopDirective : public ACCExecutableDirective {
  friend class ASTStmtReader;
  llvm::DenseSet<VarDecl *> LCVars;
  OpenACCClauseKind ParentLoopPartitioning = ACCC_unknown;

  /// Build directive with the given start and end location.
  ///
  /// \param StartLoc Starting location of the directive (directive keyword).
  /// \param EndLoc Ending Location of the directive.
  ///
  ACCLoopDirective(SourceLocation StartLoc, SourceLocation EndLoc,
                   unsigned NumClauses)
      : ACCExecutableDirective(this, ACCLoopDirectiveClass, ACCD_loop, StartLoc,
                               EndLoc, NumClauses, 1) {}

  /// Build an empty directive.
  explicit ACCLoopDirective(unsigned NumClauses)
      : ACCExecutableDirective(this, ACCLoopDirectiveClass, ACCD_loop,
                               SourceLocation(), SourceLocation(), NumClauses,
                               1) {}

public:
  /// Creates directive.
  ///
  /// \param C AST context.
  /// \param StartLoc Starting location of the directive kind.
  /// \param EndLoc Ending Location of the directive.
  /// \param Clauses List of clauses.
  /// \param AssociatedStmt Statement associated with the directive.
  /// \param LCVars Loop control variables that are assigned but not declared
  ///        in the inits of the for loops associated with the directive.
  /// \param ParentLoopPartitioning The loop partitioning that immediately
  ///        parents this directive.
  static ACCLoopDirective *Create(
      const ASTContext &C, SourceLocation StartLoc, SourceLocation EndLoc,
      ArrayRef<ACCClause *> Clauses, Stmt *AssociatedStmt,
      const llvm::DenseSet<VarDecl *> &LCVars,
      OpenACCClauseKind ParentLoopPartitioning);

  /// Creates an empty directive.
  ///
  /// \param C AST context.
  /// \param NumClauses Number of clauses.
  ///
  static ACCLoopDirective *CreateEmpty(const ASTContext &C, unsigned NumClauses,
                                       EmptyShell);

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == ACCLoopDirectiveClass;
  }

  /// Set the loop control variables that are assigned but not declared in the
  /// inits of the for loops associated with the directive.
  void setLoopControlVariables(const llvm::DenseSet<VarDecl *> &LCVars) {
    this->LCVars = LCVars;
  }
  /// Get the loop control variables that are assigned but not declared in the
  /// init of the for loop associated with the directive, or return an empty
  /// set if none.
  const llvm::DenseSet<VarDecl *> &getLoopControlVariables() const {
    return LCVars;
  }

  /// Set the loop partitioning that immediately parents this directive.
  void setParentLoopPartitioning(OpenACCClauseKind V) {
    ParentLoopPartitioning = V;
  }
  /// Get the loop partitioning that immediately parents this directive.  That
  /// is, search upward, skip sequential loops, and stop at the first compute
  /// directive or partitioned loop directive.  If stopping on a partitioned
  /// loop directive (including a combined compute and partitioned loop
  /// directive), return ACCC_vector, ACCC_worker, or ACCC_gang, in that order
  /// of preference where the loop is partitioned by more than one of these.
  /// Else, if stopping on a compute directive, return ACCC_unknown.
  OpenACCClauseKind getParentLoopPartitioning() const {
    return ParentLoopPartitioning;
  }
};

/// This represents '#pragma acc parallel loop' directive.
///
/// \code
/// #pragma acc parallel loop private(a,b) reduction(+: c,d)
/// \endcode
/// In this example directive '#pragma acc parallel loop' has clauses 'private'
/// with the variables 'a' and 'b' and 'reduction' with operator '+' and
/// variables 'c' and 'd'.
///
class ACCParallelLoopDirective : public ACCExecutableDirective {
  /// Build directive with the given start and end location.
  ///
  /// \param StartLoc Starting location of the directive (directive keyword).
  /// \param EndLoc Ending Location of the directive.
  ///
  ACCParallelLoopDirective(SourceLocation StartLoc, SourceLocation EndLoc,
                           unsigned NumClauses)
      : ACCExecutableDirective(this, ACCParallelLoopDirectiveClass,
                               ACCD_parallel_loop, StartLoc, EndLoc,
                               NumClauses, 1) {}

  /// Build an empty directive.
  explicit ACCParallelLoopDirective(unsigned NumClauses)
      : ACCExecutableDirective(this, ACCParallelLoopDirectiveClass,
                               ACCD_parallel_loop, SourceLocation(),
                               SourceLocation(), NumClauses, 1) {}

public:
  /// Creates directive.
  ///
  /// \param C AST context.
  /// \param StartLoc Starting location of the directive kind.
  /// \param EndLoc Ending Location of the directive.
  /// \param Clauses List of clauses.
  /// \param AssociatedStmt Statement associated with the directive.
  /// \param ParallelDir The effective acc parallel directive.
  static ACCParallelLoopDirective *Create(
      const ASTContext &C, SourceLocation StartLoc, SourceLocation EndLoc,
      ArrayRef<ACCClause *> Clauses, Stmt *AssociatedStmt,
      ACCParallelDirective *ParallelDir);

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == ACCParallelLoopDirectiveClass;
  }

  /// Creates an empty directive.
  ///
  /// \param C AST context.
  /// \param NumClauses Number of clauses.
  ///
  static ACCParallelLoopDirective *CreateEmpty(
      const ASTContext &C, unsigned NumClauses, EmptyShell);
};

} // end namespace clang

#endif
