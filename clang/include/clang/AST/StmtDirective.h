//===- StmtDirective.h - Base class for directives --------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// This file defines an AST base class for OpenMP and OpenACC executable
/// directives.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_STMTDIRECTIVE_H
#define LLVM_CLANG_AST_STMTDIRECTIVE_H

#include "clang/AST/Expr.h"
#include "clang/Basic/SourceLocation.h"

namespace clang {
/// This is a base class for representing a single OpenMP or OpenACC
/// executable directive.
///
/// TODO: Much more common functionality could be migrated into this class.
/// However, until OpenACC support goes upstream, to facilitate merging
/// upstream changes into downstream OpenACC support, this class mostly
/// contains common functionality that does not appear in upstream OpenMP
/// support.  (For common functionality that might be needed by that
/// functionality and that already appears in upstream OpenMP, CRTP could be
/// used to expose it here.  For example, we could have a getAssociatedStmt
/// here that calls static_cast<Derived&>(*this).getAssociatedStmt().)
class ExecutableDirective : public Stmt {
protected:
  /// Build instance of directive.
  ///
  /// \param SC Statement class.
  ExecutableDirective(StmtClass SC) : Stmt(SC) {}
public:
  /// Gets the source range covering the '#pragma' through the end of any
  /// associated statement but potentially without any final semicolon.
  ///
  /// If FinalSemicolonIsNext != nullptr, *FinalSemicolonIsNext is set to
  /// indicate whether the result does not include a final semicolon
  /// (currently, this happens when the associated statement ends with an
  /// expression statement, which is represented by Expr, which does include
  /// NullStmt). If true and the caller wants to find the final semicolon,
  /// the caller can re-lex the input starting at the result's end location,
  /// which points at the token before the final semicolon.  However, the
  /// caller must be sure to handle cases where either of these tokens
  /// appears in a macro expansion, which can affect re-lexing and the
  /// interpretation of a SourceLocation.  Because we haven't found a
  /// re-lexing technique that can always determine the final semicolon's
  /// location in such cases, and because some callers don't care if the result
  /// includes the final semicolon, we don't try to handle the re-lexing here.
  ///
  /// FIXME: Can we change the parser to store the semicolon location as a new
  /// field within an Expr when used as a statement?  Then we could return
  /// correct locations here.
  SourceRange getConstructRange(bool *FinalSemicolonIsNext = nullptr) const;
  /// Gets the source range covering the '#pragma' through the end of just
  /// the directive without any associated statement.
  SourceRange getDirectiveRange() const {
    return SourceRange(getBeginLoc(), getEndLoc());
  }

  static bool classof(const Stmt *S) {
    return S->getStmtClass() >= firstExecutableDirectiveConstant &&
           S->getStmtClass() <= lastExecutableDirectiveConstant;
  }
};
} // end namespace clang

#endif
