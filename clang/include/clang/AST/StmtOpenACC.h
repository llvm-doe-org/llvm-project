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
  unsigned NumClauses;
  /// Maximum number of clauses that might be added later.
  unsigned MaxAddClauses;
  /// Number of child expressions/stmts.
  const unsigned NumChildren;
  /// Offset from this to the start of clauses.
  /// There are NumClauses + MaxAddClauses pointers to clauses.  They are
  /// followed by NumChildren pointers to child stmts/exprs (if the directive
  /// type requires an associated stmt, then it has to be the first of them).
  const unsigned ClausesOffset;
  ACCExecutableDirective *EffectiveDirective = nullptr;
  Stmt *OMPNode;
  bool DirectiveDiscardedForOMP;

  /// Get the full storage for the clauses.
  MutableArrayRef<ACCClause *> getClauseStorage() {
    ACCClause **ClauseStorage = reinterpret_cast<ACCClause **>(
        reinterpret_cast<char *>(this) + ClausesOffset);
    return MutableArrayRef<ACCClause *>(ClauseStorage,
                                        NumClauses + MaxAddClauses);
  }
  /// Get the clauses.
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
  /// \param NumClauses Number of actual clauses.
  /// \param MaxAddClauses Maximum number of clauses that might be added later.
  /// \param NumChildren Number of child nodes.
  ///
  template <typename T>
  ACCExecutableDirective(const T *, StmtClass SC, OpenACCDirectiveKind K,
                         SourceLocation StartLoc, SourceLocation EndLoc,
                         unsigned NumClauses, unsigned MaxAddClauses,
                         unsigned NumChildren)
      : Stmt(SC), Kind(K), StartLoc(std::move(StartLoc)),
        EndLoc(std::move(EndLoc)), NumClauses(NumClauses),
        MaxAddClauses(MaxAddClauses), NumChildren(NumChildren),
        ClausesOffset(llvm::alignTo(sizeof(T), alignof(ACCClause *))),
        OMPNode(nullptr), DirectiveDiscardedForOMP(false) {}

  /// Sets the list of clauses for this directive.
  ///
  /// \param Clauses The list of clauses for the directive.
  ///
  void setClauses(ArrayRef<ACCClause *> Clauses);

  /// Adds a clause to the directive (total must be no more than the
  /// NumClauses + MaxAddClauses specified to the constructor).
  ///
  /// \param Clause The clause to add.
  ///
  void addClause(ACCClause *Clause);

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
    Stmt **ChildStorage = reinterpret_cast<Stmt **>(getClauseStorage().end());
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
  /// \param NumClauses Number of clauses.
  ///
  ACCParallelDirective(SourceLocation StartLoc, SourceLocation EndLoc,
                       unsigned NumClauses)
      : ACCExecutableDirective(this, ACCParallelDirectiveClass, ACCD_parallel,
                               StartLoc, EndLoc, NumClauses, 0, 1)
        {}

  /// Build an empty directive.
  explicit ACCParallelDirective(unsigned NumClauses)
      : ACCExecutableDirective(this, ACCParallelDirectiveClass, ACCD_parallel,
                               SourceLocation(), SourceLocation(), NumClauses,
                               0, 1)
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

/// How a loop is partitioned.
class ACCPartitioningKind {
  friend class ASTStmtReader;
  enum PartitionabilitySource {
    PartSourceUnknown,
    PartImplicit,
    PartExplicit,
    PartComputed,
  } PartSource : 2;
  enum PartitionabilityKind {
    PartKindUnknown,
    PartSeq,
    PartIndependent,
    PartAuto
  } PartKind : 2;
  bool Gang : 1;
  bool Worker : 1;
  bool Vector : 1;

  void setPartImplicit(PartitionabilityKind K) {
    assert(PartSource == PartSourceUnknown && PartKind == PartKindUnknown &&
           "expected partitionability not to be set yet");
    PartKind = K;
    PartSource = PartImplicit;
  }
  void setPartExplicit(PartitionabilityKind K) {
    assert(PartSource == PartImplicit && PartKind != PartKindUnknown &&
           "expected implicit partitionability to be set already");
    PartKind = K;
    PartSource = PartExplicit;
  }
  void setPartComputed(PartitionabilityKind K) {
    assert((PartSource == PartImplicit || PartSource == PartExplicit) &&
           PartKind == PartAuto &&
           "expected implicit or explicit auto to be set");
    PartKind = K;
    PartSource = PartComputed;
  }
  void setPartLevel() {
    assert(PartSource != PartSourceUnknown &&
           (PartKind != PartSeq || PartSource == PartImplicit) &&
           "expected implicit partitionability or some partitionablity other"
           " than seq to be set");
  }
  bool hasPartitioning(bool HasPartLevel) const {
    assert(((PartSource == PartSourceUnknown && PartKind == PartKindUnknown) ||
            PartKind != PartAuto) &&
           "expected any auto clause to already be converted to seq or"
           " independent");
    return PartKind == PartIndependent && HasPartLevel;
  }

public:
  /// Construct a placeholder with nothing yet specified.  This can be left as
  /// is for constructs other than loops.
  ACCPartitioningKind()
      : PartSource(PartSourceUnknown), PartKind(PartKindUnknown),
        Gang(false), Worker(false), Vector(false) {}

  /// Set implicit independent clause.  Must not be called after other members.
  void setIndependentImplicit() { setPartImplicit(PartIndependent); }
  /// Set implicit auto clause.  Must not be called after other members.
  void setAutoImplicit() { setPartImplicit(PartAuto); }

  /// Set explicit seq clause.  Must not be called before setting the implicit
  /// partitionability or after setting computed partitionability.
  void setSeqExplicit() { setPartExplicit(PartSeq); }
  /// Set explicit independent clause.  Must not be called before setting the
  /// implicit partitionability or after setting computed partitionability.
  void setIndependentExplicit() { setPartExplicit(PartIndependent); }
  /// Set explicit auto clause.  Must not be called before setting the implicit
  /// partitionability or after setting computed partitionability.
  void setAutoExplicit() { setPartExplicit(PartAuto); }

  /// Set gang clause.  Must be called only while implicit partitionablity or
  /// some partitionability other than seq is set.
  void setGang() { setPartLevel(); Gang = true; }
  /// Set worker clause.  Must be called only while implicit partitionablity or
  /// some partitionability other than seq is set.
  void setWorker() { setPartLevel(); Worker = true; }
  /// Set vector clause.  Must be called only while implicit partitionablity or
  /// some partitionability other than seq is set.
  void setVector() { setPartLevel(); Vector = true; }

  /// Set computed seq clause.  Must be called only while implicit or explicit
  /// auto is set.
  void setSeqComputed() { setPartComputed(PartSeq); }
  /// Set computed independent clause.  Must be called only while implicit
  /// or explicit auto is set.
  void setIndependentComputed() { setPartComputed(PartIndependent); }

  /// Does the loop have a seq clause, perhaps explicitly or computed by auto?
  /// False does not necessarily indicate the loop won't execute sequentially,
  /// depending on specified partitioning levels and any implicit gang clause
  /// possibly added later.
  bool hasSeq() const { return PartKind == PartSeq; }
  /// Does the loop have an independent clause, perhaps implicitly, explicitly,
  /// or computed by auto?  True does not necessarily indicate the loop won't
  /// execute sequentially, depending on specified partitioning levels and any
  /// implicit gang clause possibly added later.  In the case of implicit
  /// independent, returns true before any contradictory explicit clause is set
  /// and false afterward.
  bool hasIndependent() const { return PartKind == PartIndependent; }
  /// Does the loop have an auto clause, perhaps implicitly or explicitly?  If
  /// implicit, returns true before any contradictory explicit clause is set
  /// and false afterward.  Returns true before conversion of an auto clause
  /// to seq or independent and false afterward.
  bool hasAuto() const { return PartKind == PartAuto; }

  /// Does the loop have an explicit seq clause?
  bool hasSeqExplicit() const {
    return PartKind == PartSeq && PartSource == PartExplicit;
  }
  /// Does the loop have an implicit independent clause?  True does not
  /// necessarily indicate the loop won't execute sequentially, depending on
  /// specified partitioning levels and any implicit gang clause possibly added
  /// later.  If returns true, returns false after any contradictory explicit
  /// clause is set.
  bool hasIndependentImplicit() const {
    return PartKind == PartIndependent && PartSource == PartImplicit;
  }

  /// Does the loop have a gang clause, perhaps implicitly or explicitly?
  /// True does not necessarily indicate gang-partitioning, depending on the
  /// determination of any auto clause.
  bool hasGangClause() const { return Gang; }
  /// Does the loop have a worker clause?  True does not necessarily indicate
  /// worker-partitioning, depending on the determination of any auto clause.
  bool hasWorkerClause() const { return Worker; }
  /// Does the loop have a vector clause?  True does not necessarily indicate
  /// vector-partitioning, depending on the determination of any auto clause.
  bool hasVectorClause() const { return Vector; }

  /// Does the loop have gang partitioning?  Must not be called before
  /// converting any auto clause to seq or independent.
  bool hasGangPartitioning() const { return hasPartitioning(Gang); }
  /// Does the loop have worker partitioning?  Must not be called before
  /// converting any auto clause to seq or independent.
  bool hasWorkerPartitioning() const { return hasPartitioning(Worker); }
  /// Does the loop have vector partitioning?  Must not be called before
  /// converting any auto clause to seq or independent.
  bool hasVectorPartitioning() const { return hasPartitioning(Vector); }
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
  ACCPartitioningKind Partitioning;
  bool NestedGangPartitioning = false;

  /// Build directive with the given start and end location.
  ///
  /// \param StartLoc Starting location of the directive (directive keyword).
  /// \param EndLoc Ending Location of the directive.
  /// \param NumClauses Starting number of clauses (implicit gang clause can be
  ///        added later).
  ///
  ACCLoopDirective(SourceLocation StartLoc, SourceLocation EndLoc,
                   unsigned NumClauses)
      : ACCExecutableDirective(this, ACCLoopDirectiveClass, ACCD_loop,
                               StartLoc, EndLoc, NumClauses, 1, 1) {}

  /// Build an empty directive.
  ///
  /// \param NumClauses Permanent number of clauses (implicit gang clause
  ///        cannot be added later).
  ///
  explicit ACCLoopDirective(unsigned NumClauses)
      : ACCExecutableDirective(this, ACCLoopDirectiveClass, ACCD_loop,
                               SourceLocation(), SourceLocation(), NumClauses,
                               0, 1) {}

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
  /// \param Partitioning How this loop is partitioned.
  /// \param NestedGangPartitioning Whether an acc loop directive with gang
  ///        partitioning is nested here.
  static ACCLoopDirective *Create(
      const ASTContext &C, SourceLocation StartLoc, SourceLocation EndLoc,
      ArrayRef<ACCClause *> Clauses, Stmt *AssociatedStmt,
      const llvm::DenseSet<VarDecl *> &LCVars,
      ACCPartitioningKind Partitioning, bool NestedGangPartitioning);

  /// Creates an empty directive.
  ///
  /// \param C AST context.
  /// \param NumClauses Number of clauses.
  ///
  static ACCLoopDirective *CreateEmpty(const ASTContext &C,
                                       unsigned NumClauses, EmptyShell);

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

  /// Record whether an acc loop directive with gang partitioning is nested
  /// here.
  void setNestedGangPartitioning(bool V) { NestedGangPartitioning = V; }
  /// Return true if an acc loop directive with gang partitioning is nested
  /// here.
  bool getNestedGangPartitioning() const { return NestedGangPartitioning; }

  /// Set how the loop is partitioned.
  void setPartitioning(ACCPartitioningKind V) { Partitioning = V; }
  /// Get how the loop is partitioned.
  ACCPartitioningKind getPartitioning() const { return Partitioning; }

  /// Add an implicit gang clause.
  ///
  /// Must not be called if there's already a gang clause or if this directive
  /// was constructed using CreateEmpty.
  ///
  void addImplicitGangClause() {
    assert(!hasClausesOfKind<ACCGangClause>() &&
           "expected loop directive not to already have a gang clause");
    addClause(new ACCGangClause());
    Partitioning.setGang();
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
  /// \param NumClauses Number of clauses.
  ///
  ACCParallelLoopDirective(SourceLocation StartLoc, SourceLocation EndLoc,
                           unsigned NumClauses)
      : ACCExecutableDirective(this, ACCParallelLoopDirectiveClass,
                               ACCD_parallel_loop, StartLoc, EndLoc,
                               NumClauses, 0, 1) {}

  /// Build an empty directive.
  explicit ACCParallelLoopDirective(unsigned NumClauses)
      : ACCExecutableDirective(this, ACCParallelLoopDirectiveClass,
                               ACCD_parallel_loop, SourceLocation(),
                               SourceLocation(), NumClauses, 0, 1) {}

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
