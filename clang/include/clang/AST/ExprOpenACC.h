//===------ ExprOpenACC.h - Classes for OpenACC expressions -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//  This file defines the OpenACC portion of the Expr interface.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_AST_EXPROPENACC_H
#define LLVM_CLANG_AST_EXPROPENACC_H

#include "clang/AST/ASTContext.h"
#include "clang/AST/Expr.h"

namespace clang {
/// A '*' appearing in various parts of the OpenACC directive syntax (e.g.,
/// 'gang' clause, 'tile' clause, 'device_type' clause) where it is distinct
/// from the various uses of '*' in the base language (C/C++).  The type is
/// always void, and there are no children.
class ACCStarExpr : public Expr {
  SourceLocation Loc;

public:
  ACCStarExpr(const ASTContext &Context, SourceLocation Loc)
      : Expr(ACCStarExprClass, Context.VoidTy, VK_PRValue, OK_Ordinary),
        Loc(Loc) {
    setDependence(ExprDependence::None);
  }
  explicit ACCStarExpr(EmptyShell Shell) : Expr(ACCStarExprClass, Shell) {}
  SourceLocation getLoc() const { return Loc; }
  void setLoc(SourceLocation L) { Loc = L; }

  SourceLocation getBeginLoc() const LLVM_READONLY { return Loc; }
  SourceLocation getEndLoc() const LLVM_READONLY { return Loc; }
  child_range children() {
    return child_range(child_iterator(), child_iterator());
  }
  const_child_range children() const {
    return const_child_range(const_child_iterator(), const_child_iterator());
  }
  static bool classof(const Stmt *T) {
    return T->getStmtClass() == ACCStarExprClass;
  }
};
} // end namespace clang

#endif
