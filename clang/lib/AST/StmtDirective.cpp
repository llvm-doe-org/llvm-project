//===--- StmtDirective.cpp - Base class for directives --------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the AST base class for OpenMP and OpenACC executable
// directives declared in StmtDirective.h.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/StmtDirective.h"
#include "clang/AST/StmtOpenMP.h"
#include "clang/Basic/SourceManager.h"

using namespace clang;

SourceRange ExecutableDirective::getConstructRange(
    bool *FinalSemicolonIsNext) const {
  // TODO: We haven't found a clean way to determine the location of the last
  // token from an AST node, so we implemented the following.  The switch below
  // has cases for AST node types that are known to require special handling,
  // and the default case employs a general approach that so far works for
  // other AST node types.  It might need new cases if we discover bugs.  When
  // OpenACC support is upstreamed, this switch statement should probably
  // migrate into a Stmt member function, perhaps named getFullSourceRange,
  // such that the Stmt implementation comes from the default case and other
  // AST node types' overriding implementations come from the other cases.
  //
  // Instead of all this, we previously tried to use getSourceRange (after
  // fixing it for ExecutableDirective), but it doesn't work when a ForStmt's
  // body is an ExecutableDirective because ForStmt's getSourceRange then calls
  // the ExecutableDirective's getEndLoc instead of its getSourceRange.
  //
  // We also tried to implement one general approach that works for all AST
  // node types by descending through the AST looking for the node with the
  // last location.  However, do you use spelling, expansion, or presumed
  // locations (as documented in SourceManager.h)?  Spelling locations don't
  // work because, if the last token is expanded from a macro, its spelling
  // location might be before any other token.  Expansion locations don't work
  // because tokens expanded from the same macro invocation have the same
  // location.  Presumed locations don't work because #line can make tokens
  // appear to be in a different order.  Using operator< for original
  // SourceLocation objects before conversion to any of those forms doesn't
  // work because tokens expanded from macros then appear to occur after other
  // tokens.  Instead, we just use AST node types to determine where to find
  // the last token's location, and we return the original SourceLocation
  // objects there to the caller to convert to any location form the caller
  // desires.
  const Stmt *S = this;
  for (const Stmt *SPrev = nullptr; S != SPrev;) {
    SPrev = S;
    switch (S->getStmtClass()) {
    default:
      // For most AST node types, the last child has the last token.  In at
      // least some cases, calling getEndLoc on the node itself doesn't return
      // that token's location.  For example, when ExecutableDirective has an
      // associated statement (which is then its child), getEndLoc on the
      // ExecutableDirective points to the end of the directive only, so the
      // child must be examined.
      //
      // TODO: It should be more efficient to add a special case for every AST
      // node type that has multiple children such that the child with the last
      // token can be retrieved by a fixed member function or such that
      // getEndLoc on the node itself suffices, but we haven't yet identified
      // all those AST node types.
      for (const Stmt *Child : S->children()) {
        // Some nodes, such as DeclStmt, have nullptr for some children, so
        // skip those.
        if (Child)
          S = Child;
      }
      break;
    case CapturedStmtClass:
      // The last child might not have the last token.
      S = cast<CapturedStmt>(S)->getCapturedStmt();
      break;
    case ArraySubscriptExprClass:
    case CallExprClass:
    case CompoundStmtClass:
    case GenericSelectionExprClass:
    case InitListExprClass:
    case ParenExprClass:
    case UnaryExprOrTypeTraitExprClass:
      // In most cases, getEndLoc refers to the closing bracket, brace, or
      // parenthesis, which is not contained in any of the children.
      //
      // UnaryExprOrTypeTraitExpr, which represents sizeof or _Alignof, has
      // many special cases: no child in the case of most type arguments, child
      // whose getEndLoc doesn't refer to the closing parenthesis in the case
      // of a variable array type argument, and either no closing parenthesis
      // or a closing parenthesis from the child in the case of an expression
      // argument.  In all cases, it seems getEndLoc on
      // UnaryExprOrTypeTraitExpr works fine.
      break;
    }
  }
  if (FinalSemicolonIsNext)
    *FinalSemicolonIsNext = dyn_cast_or_null<Expr>(S);
  return SourceRange(getBeginLoc(), S->getEndLoc());
}
