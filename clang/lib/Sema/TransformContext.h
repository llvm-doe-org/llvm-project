//===------ TransformContext.h - DeclContext Transformation -----*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//===----------------------------------------------------------------------===//
//
//  This file implements TransformContext, which derives from TreeTransform to
//  rebuild an AST subtree while rebuilding every contained definition to update
//  its DeclContext for the new subtree.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_SEMA_TRANSFORMCONTEXT_H
#define LLVM_CLANG_LIB_SEMA_TRANSFORMCONTEXT_H

#include "TreeTransform.h"

namespace clang {
using namespace sema;

/// Like \c TreeTransform but rebuilds every contained definition to update its
/// \c DeclContext for the new AST subtree.
///
/// So far, \c TransformContext development is driven only by
/// \c TransformACCToOMP, which derives from it.  \c TransformContext is
/// ultimately intended to be more generally reusable, but it is likely still
/// far from a complete implementation in that respect, especially for C++.
template <typename Derived>
class TransformContext : public TreeTransform<Derived> {
  typedef TreeTransform<Derived> BaseTransform;

public:
  TransformContext(Sema &SemaRef) : BaseTransform(SemaRef) {}
  // FIXME: What does this really do?  Is TransformDefinition needed when we
  // have this?
  bool AlwaysRebuild() { return true; }
  // If there might be an existing mapping for D and you want to push a new
  // mapping, don't just call TransformDefinition, which will just reuse the
  // existing mapping.  Instead:
  // 1. Call TransformDecl to get the existing mapping, E, and store it
  //    somewhere.
  // 2. Call transformedLocalDecl(D, D).
  // 3. Call TransformDefinition, which will now create the new mapping.
  // 4. Call transformedLocalDecl(D, E) once the new mapping goes out of scope.
  // FIXME: Does this handle declarations that are not definitions?
  Decl *TransformDefinition(SourceLocation Loc, Decl *D,
                            bool DropInit = false) {
    // Skip previously transformed declarations.
    Decl *T = this->getDerived().TransformDecl(Loc, D);
    if (T != D)
      return T;

    // Would we have to rebuild the type or anything else in order to change
    // the ASTContext?
    assert(&D->getASTContext() == &this->getSema().getASTContext() &&
           "changing ASTContext not handled");

    // Handle FunctionDecl at file scope.
    if (isa<FunctionDecl>(D)) {
      assert(D->isDefinedOutsideFunctionOrMethod() &&
             "nested FunctionDecl not yet handled");
      return D;
    }

    // Handle variable declarations.
    DeclContext *DCNew = this->getSema().CurContext;
    if (VarDecl *VDOld = dyn_cast<VarDecl>(D)) {
      TypeSourceInfo *TypeNew =
          this->getDerived().TransformType(VDOld->getTypeSourceInfo());
      VarDecl *VDNew = VarDecl::Create(
          VDOld->getASTContext(), DCNew, VDOld->getBeginLoc(),
          VDOld->getLocation(), VDOld->getIdentifier(), TypeNew->getType(),
          TypeNew, VDOld->getStorageClass());
      if (!DropInit && VDOld->hasInit()) {
        // Sema::InstantiateVariableInitializer seems to be a good model for how
        // this code can expand to handle more cases.  We cannot use it directly
        // as is because its call to TransformInitializer (within
        // SubstInitializer) is on a local TemplateInstantiator instead of on
        // this->getDerived(), and so it doesn't perform all desired
        // transformations (specifically, we've noticed it using old references
        // to local variables instead of the transformed versions).
        ExprResult Init = this->getDerived().TransformInitializer(
            VDOld->getInit(), VDOld->getInitStyle() == VarDecl::CallInit);
        assert(!Init.isInvalid() && "Failed to transform VarDecl initializer");
        if (Init.get())
          this->getSema().AddInitializerToDecl(VDNew, Init.get(),
                                               VDOld->isDirectInit());
        else
          this->getSema().ActOnUninitializedDecl(VDNew);
      }
      this->getDerived().transformedLocalDecl(VDOld, VDNew);
      return VDNew;
    }

    // Handle type declarations.
    //
    // TODO: Unlike VarDecl above, we haven't found a case where it matters
    // whether we rebuild each Decl kind here with the new DeclContext.
    // Moreover, rebuilding proves to be tricky to get right.  Trying just
    // created bugs and didn't apparently fix any.
    if (isa<TypedefDecl>(D) || isa<EnumDecl>(D) || isa<RecordDecl>(D))
      return D;

    llvm_unreachable(
        "unhandled Decl type in TransformContext::TransformDefinition");
  }
};

} // end namespace clang

#endif // LLVM_CLANG_LIB_SEMA_TRANSFORMCONTEXT_H
