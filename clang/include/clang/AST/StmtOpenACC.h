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

#include "clang/AST/Attr.h"
#include "clang/AST/DirectiveStmt.h"
#include "clang/AST/Expr.h"
#include "clang/AST/OpenACCClause.h"
#include "clang/AST/StmtOpenMP.h"
#include "clang/Basic/OpenACCKinds.h"
#include "clang/Basic/SourceLocation.h"

namespace clang {

//===----------------------------------------------------------------------===//
// AST classes for directives.
//===----------------------------------------------------------------------===//

/// This is a base class for representing an OpenACC executable directive (e.g.,
/// 'acc update') or construct (e.g., 'acc parallel').
class ACCDirectiveStmt : public DirectiveStmt {
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
  /// The outermost effective directive.  \c nullptr if not yet set or this is
  /// a non-combined directive.
  ACCDirectiveStmt *EffectiveDirective;
  /// The root node of the OpenMP translation.  \c nullptr if not yet set.
  Stmt *OMPNode;
  /// The number of OpenMP directives to which the OpenACC directive was
  /// translated.  0 if none or not yet set.
  unsigned OMPDirectiveCount;

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
  ACCDirectiveStmt(const T *, StmtClass SC, OpenACCDirectiveKind K,
                   SourceLocation StartLoc, SourceLocation EndLoc,
                   unsigned NumClauses, unsigned MaxAddClauses,
                   unsigned NumChildren)
      : DirectiveStmt(SC), Kind(K), StartLoc(std::move(StartLoc)),
        EndLoc(std::move(EndLoc)), NumClauses(NumClauses),
        MaxAddClauses(MaxAddClauses), NumChildren(NumChildren),
        ClausesOffset(llvm::alignTo(sizeof(T), alignof(ACCClause *))),
        EffectiveDirective(nullptr), OMPNode(nullptr), OMPDirectiveCount(0) {}

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
  void setEffectiveDirective(ACCDirectiveStmt *D) {
    assert(isOpenACCCombinedDirective(Kind) &&
           "expected combined OpenACC directive");
    EffectiveDirective = D;
  }

public:
  static bool classof(const Stmt *S) {
    return S->getStmtClass() >= firstACCDirectiveStmtConstant &&
           S->getStmtClass() <= lastACCDirectiveStmtConstant;
  }

  /// Set \p OMPNode, which must not be \c nullptr, as the root node of the
  /// OpenMP translation of this OpenACC directive.  In most cases, it's an
  /// OpenMP directive.  In some cases, it's a compound statement that might or
  /// might not contain an OpenMP directive.
  ///
  /// \p OMPDirectiveCount must be the number of OpenMP directives to which this
  /// OpenACC directive was translated, and each such OpenMP directive must
  /// appear in the subtree rooted at \p OMPNode.  \p OMPDirectiveCount must not
  /// include OpenMP directives appearing in the translation of this OpenACC
  /// directive's associated statement.  If this OpenACC directive was discarded
  /// rather than translated to any OpenMP directive, then \p OMPDirectiveCount
  /// must be zero.  If this OpenACC directive is a combined OpenACC directive,
  /// then \p OMPDirectiveCount must be the sum of the \p OMPDirectiveCount
  /// values for its effective OpenACC directives.
  ///
  /// Fails an assertion if \c setOMPNode has already been called for this
  /// OpenACC directive.
  void setOMPNode(Stmt *OMPNode, unsigned OMPDirectiveCount = 1) {
    assert(!hasOMPNode() && "expected not to have OpenMP translation already");
    assert(OMPNode != nullptr && "expected OMPNode");
    assert((OMPDirectiveCount > 0 || !isa<OMPExecutableDirective>(OMPNode)) &&
           "expected OMPNode not to be an OpenMP directive when translation "
           "discards directive");
    this->OMPNode = OMPNode;
    this->OMPDirectiveCount = OMPDirectiveCount;
  }

  /// Has this directive been translated to an OpenMP directive?  If false, then
  /// either the translation has not yet been performed, the translation failed,
  /// or \c setOMPNode hasn't been called yet.
  bool hasOMPNode() const { return OMPNode; }

  /// Get the statement to which this directive has been translated for OpenMP.
  /// Never returns nullptr.  Fails an assertion if \c hasOMPNode returns false.
  Stmt *getOMPNode() const {
    assert(hasOMPNode() && "expected to have OpenMP translation already");
    return OMPNode;
  }

  /// The number of OpenMP directives to which this directive was translated.
  /// Fails an assertion if \c hasOMPNode returns false.
  unsigned getOMPDirectiveCount() const {
    assert(hasOMPNode() && "expected to have OpenMP translation already");
    return OMPDirectiveCount;
  }

  /// Was the OpenACC directive discarded rather than translated to an OpenMP
  /// directive?  Fails an assertion if \c hasOMPNode returns false.
  bool directiveDiscardedForOMP() const {
    assert(hasOMPNode() && "expected to have OpenMP translation already");
    return OMPDirectiveCount == 0;
  }

  // Are the OpenACC and OpenMP versions of an OpenACC construct different
  // enough that, when printing both versions, it is not as simple as printing
  // the associated statement once (possibly containing multiple versions of
  // nested OpenACC constructs) preceded by both the OpenACC and OpenMP
  // directives (one within comments)?
  //
  // For example, StmtPrinter and RewriteOpenACC print both versions (one within
  // comments) when Policy.OpenACCPrint is OpenACCPrint_ACC_OMP or
  // OpenACCPrint_OMP_ACC.  When the result of ompStmtPrintsDifferently is true,
  // StmtPrinter/RewriteOpenACC must print the OpenACC directive plus its
  // associated statement completely separately (one within comments) from the
  // corresponding OpenMP directives and their associated statement.  When the
  // result is false, StmtPrinter/RewriteOpenACC can usually print the OpenACC
  // directive separately from the OpenMP directives (one within comments) and
  // print the associated statement once afterward.  RewriteOpenACC also calls
  // ompStmtPrintsDifferently when Policy.OpenACCPrint is OpenACCPrint_OMP.
  // When the result is true, RewriteOpenACC rewrites the OpenACC construct
  // entirely.  When the result is false, RewriteOpenACC rewrites only the
  // OpenACC directive and then any descendant OpenACC constructs as needed.
  //
  // ompStmtPrintsDifferently makes its determination by checking whether all
  // portions of the associated statements except nested OpenACC regions are
  // identical when printed.  The reason it doesn't check nested OpenACC regions
  // is that they can be split if necessary within this region (the need to
  // split them is checked by calling ompStmtPrintsDifferently when
  // printing/rewriting later reaches them).  A degenerate case is when
  // there is no associated statement at all (standalone directive), and then it
  // returns false because there's nothing to split.  Another special case is
  // when the corresponding OpenMP directives are not consecutive, and then it
  // returns true because there's interleaving code that must be printed only in
  // the OpenMP version.  Another special case is when the OpenMP version
  // contains no OpenMP directives, and then it compares the entire OpenMP
  // version with the OpenACC version's associated statement to see if the
  // translation modified the latter.
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
                llvm::ArrayRef(Clauses.end(), (size_t)0))};
  }

  template <typename SpecificClause>
  llvm::iterator_range<specific_clause_iterator<SpecificClause>>
  getClausesOfKind() const {
    return getClausesOfKind<SpecificClause>(clauses());
  }

  template <typename SpecificClause>
  SpecificClause *getTheClauseOfKind() const {
    auto Clauses = getClausesOfKind<SpecificClause>(clauses());
    if (Clauses.begin() == Clauses.end())
      return nullptr;
    assert(std::next(Clauses.begin()) == Clauses.end() &&
           "expected at most one clause of the specified kind");
    return *Clauses.begin();
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
  const Stmt *getAssociatedStmt() const {
    assert(hasAssociatedStmt() && "no associated statement.");
    return *child_begin();
  }
  Stmt *getAssociatedStmt() {
    assert(hasAssociatedStmt() && "no associated statement.");
    return *child_begin();
  }

  /// Returns the outermost effective directive (whose associated statement is
  /// the nested effective directive, etc.) if this is a combined directive,
  /// and returns nullptr otherwise.
  ACCDirectiveStmt *getEffectiveDirective() const { return EffectiveDirective; }

  OpenACCDirectiveKind getDirectiveKind() const { return Kind; }

  child_range children() {
    if (!hasAssociatedStmt())
      return child_range(child_iterator(), child_iterator());
    Stmt **ChildStorage = reinterpret_cast<Stmt **>(getClauseStorage().end());
    return child_range(ChildStorage, ChildStorage + NumChildren);
  }

  ArrayRef<ACCClause *> clauses() { return getClauses(); }

  ArrayRef<ACCClause *> clauses() const {
    return const_cast<ACCDirectiveStmt *>(this)->getClauses();
  }
};

/// This represents '#pragma acc update' directive.
///
class ACCUpdateDirective : public ACCDirectiveStmt {
  friend class ASTStmtReader;

  /// Build directive with the given start and end location.
  ///
  /// \param StartLoc Starting location of the directive (directive keyword).
  /// \param EndLoc Ending Location of the directive.
  /// \param NumClauses Number of clauses.
  ///
  ACCUpdateDirective(SourceLocation StartLoc, SourceLocation EndLoc,
                     unsigned NumClauses)
      : ACCDirectiveStmt(this, ACCUpdateDirectiveClass, ACCD_update, StartLoc,
                         EndLoc, NumClauses, 0, 0) {}

  /// Build an empty directive.
  explicit ACCUpdateDirective(unsigned NumClauses)
      : ACCDirectiveStmt(this, ACCUpdateDirectiveClass, ACCD_update,
                         SourceLocation(), SourceLocation(), NumClauses, 0, 0) {
  }

public:
  /// Creates directive.
  ///
  /// \param C AST context.
  /// \param StartLoc Starting location of the directive kind.
  /// \param EndLoc Ending Location of the directive.
  /// \param Clauses List of clauses.
  static ACCUpdateDirective *Create(const ASTContext &C,
                                    SourceLocation StartLoc,
                                    SourceLocation EndLoc,
                                    ArrayRef<ACCClause *> Clauses);

  /// Creates an empty directive.
  ///
  /// \param C AST context.
  /// \param NumClauses Number of clauses.
  ///
  static ACCUpdateDirective *CreateEmpty(const ASTContext &C,
                                         unsigned NumClauses, EmptyShell);

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == ACCUpdateDirectiveClass;
  }
};

/// This represents '#pragma acc enter data' directive.
///
class ACCEnterDataDirective : public ACCDirectiveStmt {
  friend class ASTStmtReader;

  /// Build directive with the given start and end location.
  ///
  /// \param StartLoc Starting location of the directive (directive keyword).
  /// \param EndLoc Ending Location of the directive.
  /// \param NumClauses Number of clauses.
  ///
  ACCEnterDataDirective(SourceLocation StartLoc, SourceLocation EndLoc,
                        unsigned NumClauses)
      : ACCDirectiveStmt(this, ACCEnterDataDirectiveClass, ACCD_enter_data,
                         StartLoc, EndLoc, NumClauses, 0, 0) {}

  /// Build an empty directive.
  explicit ACCEnterDataDirective(unsigned NumClauses)
      : ACCDirectiveStmt(this, ACCEnterDataDirectiveClass, ACCD_enter_data,
                         SourceLocation(), SourceLocation(), NumClauses, 0, 0) {
  }

public:
  /// Creates directive.
  ///
  /// \param C AST context.
  /// \param StartLoc Starting location of the directive kind.
  /// \param EndLoc Ending Location of the directive.
  /// \param Clauses List of clauses.
  static ACCEnterDataDirective *Create(const ASTContext &C,
                                       SourceLocation StartLoc,
                                       SourceLocation EndLoc,
                                       ArrayRef<ACCClause *> Clauses);

  /// Creates an empty directive.
  ///
  /// \param C AST context.
  /// \param NumClauses Number of clauses.
  ///
  static ACCEnterDataDirective *CreateEmpty(const ASTContext &C,
                                            unsigned NumClauses, EmptyShell);

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == ACCEnterDataDirectiveClass;
  }
};

/// This represents '#pragma acc exit data' directive.
///
class ACCExitDataDirective : public ACCDirectiveStmt {
  friend class ASTStmtReader;

  /// Build directive with the given start and end location.
  ///
  /// \param StartLoc Starting location of the directive (directive keyword).
  /// \param EndLoc Ending Location of the directive.
  /// \param NumClauses Number of clauses.
  ///
  ACCExitDataDirective(SourceLocation StartLoc, SourceLocation EndLoc,
                       unsigned NumClauses)
      : ACCDirectiveStmt(this, ACCExitDataDirectiveClass, ACCD_exit_data,
                         StartLoc, EndLoc, NumClauses, 0, 0) {}

  /// Build an empty directive.
  explicit ACCExitDataDirective(unsigned NumClauses)
      : ACCDirectiveStmt(this, ACCExitDataDirectiveClass, ACCD_exit_data,
                         SourceLocation(), SourceLocation(), NumClauses, 0, 0) {
  }

public:
  /// Creates directive.
  ///
  /// \param C AST context.
  /// \param StartLoc Starting location of the directive kind.
  /// \param EndLoc Ending Location of the directive.
  /// \param Clauses List of clauses.
  static ACCExitDataDirective *Create(const ASTContext &C,
                                      SourceLocation StartLoc,
                                      SourceLocation EndLoc,
                                      ArrayRef<ACCClause *> Clauses);

  /// Creates an empty directive.
  ///
  /// \param C AST context.
  /// \param NumClauses Number of clauses.
  ///
  static ACCExitDataDirective *CreateEmpty(const ASTContext &C,
                                           unsigned NumClauses, EmptyShell);

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == ACCExitDataDirectiveClass;
  }
};

/// This represents '#pragma acc data' directive.
///
class ACCDataDirective : public ACCDirectiveStmt {
  friend class ASTStmtReader;

  /// Build directive with the given start and end location.
  ///
  /// \param StartLoc Starting location of the directive (directive keyword).
  /// \param EndLoc Ending Location of the directive.
  /// \param NumClauses Number of clauses.
  ///
  ACCDataDirective(SourceLocation StartLoc, SourceLocation EndLoc,
                   unsigned NumClauses)
      : ACCDirectiveStmt(this, ACCDataDirectiveClass, ACCD_data, StartLoc,
                         EndLoc, NumClauses, 0, 1) {}

  /// Build an empty directive.
  explicit ACCDataDirective(unsigned NumClauses)
      : ACCDirectiveStmt(this, ACCDataDirectiveClass, ACCD_data,
                         SourceLocation(), SourceLocation(), NumClauses, 0, 1) {
  }

public:
  /// Creates directive.
  ///
  /// \param C AST context.
  /// \param StartLoc Starting location of the directive kind.
  /// \param EndLoc Ending Location of the directive.
  /// \param Clauses List of clauses.
  /// \param AssociatedStmt Statement associated with the directive.
  static ACCDataDirective *
  Create(const ASTContext &C, SourceLocation StartLoc, SourceLocation EndLoc,
         ArrayRef<ACCClause *> Clauses, Stmt *AssociatedStmt);

  /// Creates an empty directive.
  ///
  /// \param C AST context.
  /// \param NumClauses Number of clauses.
  ///
  static ACCDataDirective *CreateEmpty(const ASTContext &C,
                                       unsigned NumClauses, EmptyShell);

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == ACCDataDirectiveClass;
  }
};

/// This represents '#pragma acc parallel' directive.
///
class ACCParallelDirective : public ACCDirectiveStmt {
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
      : ACCDirectiveStmt(this, ACCParallelDirectiveClass, ACCD_parallel,
                         StartLoc, EndLoc, NumClauses, 0, 1) {}

  /// Build an empty directive.
  explicit ACCParallelDirective(unsigned NumClauses)
      : ACCDirectiveStmt(this, ACCParallelDirectiveClass, ACCD_parallel,
                         SourceLocation(), SourceLocation(), NumClauses, 0, 1) {
  }

  /// Record whether this parallel construct encloses one of the following that
  /// is not in an enclosed function definition: either an effective loop
  /// directive with worker partitioning, or a function call with a routine
  /// worker directive.
  void setNestedWorkerPartitioning(bool V) { NestedWorkerPartitioning = V; }

public:
  /// Creates directive.
  ///
  /// \param C AST context.
  /// \param StartLoc Starting location of the directive kind.
  /// \param EndLoc Ending Location of the directive.
  /// \param Clauses List of clauses.
  /// \param AssociatedStmt Statement associated with the directive.
  /// \param NestedWorkerPartitioning Whether this parallel construct encloses
  ///        one of the following that is not in an enclosed function
  ///        definition: either an effective loop directive with worker
  ///        partitioning, or a function call with a routine worker directive.
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

  /// Return true if this parallel construct encloses one of the following that
  /// is not in an enclosed function definition: either an effective loop
  /// directive with worker partitioning, or a function call with a routine
  /// worker directive.
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
  /// Return either gang, worker, or vector (in that order of preference) if the
  /// loop has that clause.  Otherwise, return seq (which does not indicate the
  /// loop necessarily has a seq clause).
  ACCRoutineDeclAttr::PartitioningTy getMaxParLevelClause() const {
    if (hasGangClause())
      return ACCRoutineDeclAttr::Gang;
    if (hasWorkerClause())
      return ACCRoutineDeclAttr::Worker;
    if (hasVectorClause())
      return ACCRoutineDeclAttr::Vector;
    return ACCRoutineDeclAttr::Seq;
  }
  /// Return either vector, worker, or gang (in that order of preference) if the
  /// loop has that clause.  Otherwise, return seq (which does not indicate the
  /// loop necessarily has a seq clause).
  ACCRoutineDeclAttr::PartitioningTy getMinParLevelClause() const {
    if (hasVectorClause())
      return ACCRoutineDeclAttr::Vector;
    if (hasWorkerClause())
      return ACCRoutineDeclAttr::Worker;
    if (hasGangClause())
      return ACCRoutineDeclAttr::Gang;
    return ACCRoutineDeclAttr::Seq;
  }

  /// Does the loop have gang partitioning?  Must not be called before
  /// converting any auto clause to seq or independent.
  bool hasGangPartitioning() const { return hasPartitioning(Gang); }
  /// Does the loop have worker partitioning?  Must not be called before
  /// converting any auto clause to seq or independent.
  bool hasWorkerPartitioning() const { return hasPartitioning(Worker); }
  /// Does the loop have vector partitioning?  Must not be called before
  /// converting any auto clause to seq or independent.
  bool hasVectorPartitioning() const { return hasPartitioning(Vector); }
  /// Return either gang, worker, or vector (in that order of preference) if the
  /// loop has that level of partitioning.  Otherwise, return seq.  Must not be
  /// called before converting any auto clause to seq or independent.
  ACCRoutineDeclAttr::PartitioningTy getMaxPartitioningLevel() const {
    if (hasGangPartitioning())
      return ACCRoutineDeclAttr::Gang;
    if (hasWorkerPartitioning())
      return ACCRoutineDeclAttr::Worker;
    if (hasVectorPartitioning())
      return ACCRoutineDeclAttr::Vector;
    return ACCRoutineDeclAttr::Seq;
  }

  // Remove any worker clause and partitioning.
  void removeWorker() { Worker = false; }
  // Remove any vector clause and partitioning.
  void removeVector() { Vector = false; }
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
class ACCLoopDirective : public ACCDirectiveStmt {
  friend class ASTStmtReader;
  MutableArrayRef<VarDecl *> LCVs;
  ACCPartitioningKind Partitioning;
  bool NestedGangPartitioning = false;

  static MutableArrayRef<VarDecl *> reserveLCVs(
      ACCLoopDirective *D, unsigned NumClauses, unsigned MaxAddClauses,
      unsigned NumLCVs) {
    unsigned LCVsOffset =
        llvm::alignTo(sizeof(ACCLoopDirective), alignof(ACCClause *)) +
        sizeof(ACCClause *) * (NumClauses + MaxAddClauses) + sizeof(Stmt *);
    VarDecl **LCVStorage = reinterpret_cast<VarDecl **>(
        reinterpret_cast<char *>(D) + LCVsOffset);
    return MutableArrayRef<VarDecl *>(LCVStorage, NumLCVs);
  }

  /// Build directive with the given start and end location.
  ///
  /// \param StartLoc Starting location of the directive (directive keyword).
  /// \param EndLoc Ending Location of the directive.
  /// \param NumClauses Starting number of clauses (implicit gang clause can be
  ///        added later).
  /// \param NumLCVs The number of loop control variables that are assigned but
  ///        not declared in the inits of the for loops associated with the
  ///        directive.
  ///
  ACCLoopDirective(SourceLocation StartLoc, SourceLocation EndLoc,
                   unsigned NumClauses, unsigned NumLCVs)
      : ACCDirectiveStmt(this, ACCLoopDirectiveClass, ACCD_loop, StartLoc,
                         EndLoc, NumClauses, /*MaxAddClauses=*/1,
                         /*NumChildren=*/1),
        LCVs(reserveLCVs(this, NumClauses, /*MaxAddClauses=*/1, NumLCVs)) {}

  /// Build an empty directive.
  ///
  /// \param NumClauses Permanent number of clauses (implicit gang clause
  ///        cannot be added later).
  /// \param NumLCVs The number of loop control variables that are assigned but
  ///        not declared in the inits of the for loops associated with the
  ///        directive.
  ///
  explicit ACCLoopDirective(unsigned NumClauses, unsigned NumLCVs)
      : ACCDirectiveStmt(this, ACCLoopDirectiveClass, ACCD_loop,
                         SourceLocation(), SourceLocation(), NumClauses,
                         /*MaxAddClauses=*/0, /*NumChildren=*/1),
        LCVs(reserveLCVs(this, NumClauses, /*MaxAddClauses=*/0, NumLCVs)) {}

  /// Set the loop control variables that are assigned but not declared in the
  /// inits of the for loops associated with the directive.
  void setLoopControlVariables(const ArrayRef<VarDecl *> &LCVs) {
    assert(LCVs.size() == this->LCVs.size() &&
           "unexpected number of loop control variables");
    unsigned I = 0;
    for (VarDecl *LCV : LCVs)
      this->LCVs[I++] = LCV;
  }

  /// Record whether this loop construct encloses one of the following that is
  /// not in an enclosed function definition: either a loop directive with
  /// explicit gang partitioning, or a function call with a routine gang
  /// directive.
  void setNestedGangPartitioning(bool V) { NestedGangPartitioning = V; }

  /// Set how the loop is partitioned.
  void setPartitioning(ACCPartitioningKind V) { Partitioning = V; }

public:
  /// Creates directive.
  ///
  /// \param C AST context.
  /// \param StartLoc Starting location of the directive kind.
  /// \param EndLoc Ending Location of the directive.
  /// \param Clauses List of clauses.
  /// \param AssociatedStmt Statement associated with the directive.
  /// \param LCVs Loop control variables that are assigned but not declared in
  ///        the inits of the for loops associated with the directive.
  /// \param Partitioning How this loop is partitioned.
  /// \param NestedGangPartitioning Whether this loop construct encloses one of
  ///        the following that is not in an enclosed function definition:
  ///        either a loop directive with explicit gang partitioning, or a
  ///        function call with a routine gang directive.
  static ACCLoopDirective *Create(
      const ASTContext &C, SourceLocation StartLoc, SourceLocation EndLoc,
      ArrayRef<ACCClause *> Clauses, Stmt *AssociatedStmt,
      const ArrayRef<VarDecl *> &LCVs, ACCPartitioningKind Partitioning,
      bool NestedGangPartitioning);

  /// Creates an empty directive.
  ///
  /// \param C AST context.
  /// \param NumClauses Number of clauses.
  /// \param NumLCVs The number of loop control variables that are assigned
  ///        but not declared in the inits of the for loops associated with the
  ///        directive.
  ///
  static ACCLoopDirective *CreateEmpty(const ASTContext &C,
                                       unsigned NumClauses, unsigned NumLCVs,
                                       EmptyShell);

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == ACCLoopDirectiveClass;
  }

  /// Get the loop control variables that are assigned but not declared in the
  /// init of the for loop associated with the directive, or return an empty
  /// set if none.
  const ArrayRef<VarDecl *> &getLoopControlVariables() const { return LCVs; }

  /// Return true if this loop construct encloses one of the following that is
  /// not in an enclosed function definition: either a loop directive with
  /// explicit gang partitioning, or a function call with a routine gang
  /// directive.
  bool getNestedGangPartitioning() const { return NestedGangPartitioning; }

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
    addClause(new ACCGangClause(ACC_IMPLICIT, SourceLocation(),
                                SourceLocation()));
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
class ACCParallelLoopDirective : public ACCDirectiveStmt {
  /// Build directive with the given start and end location.
  ///
  /// \param StartLoc Starting location of the directive (directive keyword).
  /// \param EndLoc Ending Location of the directive.
  /// \param NumClauses Number of clauses.
  ///
  ACCParallelLoopDirective(SourceLocation StartLoc, SourceLocation EndLoc,
                           unsigned NumClauses)
      : ACCDirectiveStmt(this, ACCParallelLoopDirectiveClass,
                         ACCD_parallel_loop, StartLoc, EndLoc, NumClauses, 0,
                         1) {}

  /// Build an empty directive.
  explicit ACCParallelLoopDirective(unsigned NumClauses)
      : ACCDirectiveStmt(this, ACCParallelLoopDirectiveClass,
                         ACCD_parallel_loop, SourceLocation(), SourceLocation(),
                         NumClauses, 0, 1) {}

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

/// This represents '#pragma acc atomic' directive.
///
/// \code
/// #pragma acc atomic read
/// \endcode
/// In this example directive '#pragma acc atomic' has clause 'read'.
class ACCAtomicDirective : public ACCDirectiveStmt {
  friend class ASTStmtReader;

  /// Build directive with the given start and end location.
  ///
  /// \param StartLoc Starting location of the directive (directive keyword).
  /// \param EndLoc Ending Location of the directive.
  /// \param NumClauses Number of clauses.
  ACCAtomicDirective(SourceLocation StartLoc, SourceLocation EndLoc,
                     unsigned NumClauses)
      : ACCDirectiveStmt(this, ACCAtomicDirectiveClass, ACCD_atomic, StartLoc,
                         EndLoc, NumClauses, /*MaxAddClauses=*/0,
                         /*NumChildren=*/1) {}

  /// Build an empty directive.
  explicit ACCAtomicDirective(unsigned NumClauses)
      : ACCDirectiveStmt(this, ACCAtomicDirectiveClass, ACCD_atomic,
                         SourceLocation(), SourceLocation(), NumClauses,
                         /*MaxAddClauses=*/0, /*NumChildren=*/1) {}

public:
  /// Creates directive.
  ///
  /// \param C AST context.
  /// \param StartLoc Starting location of the directive kind.
  /// \param EndLoc Ending Location of the directive.
  /// \param Clauses List of clauses.
  /// \param AssociatedStmt Statement associated with the directive.
  static ACCAtomicDirective *
  Create(const ASTContext &C, SourceLocation StartLoc, SourceLocation EndLoc,
         ArrayRef<ACCClause *> Clauses, Stmt *AssociatedStmt);

  /// Creates an empty directive.
  ///
  /// \param C AST context.
  /// \param NumClauses Number of clauses.
  static ACCAtomicDirective *CreateEmpty(const ASTContext &C,
                                         unsigned NumClauses, EmptyShell);

  static bool classof(const Stmt *T) {
    return T->getStmtClass() == ACCAtomicDirectiveClass;
  }
};

} // end namespace clang

#endif
