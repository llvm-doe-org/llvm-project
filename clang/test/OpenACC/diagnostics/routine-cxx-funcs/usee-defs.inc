// This file contains definitions and routine directives for the functions
// declared in usee-decls.h.

#include "usee-decls.h"

#ifndef USEE_ROUTINE_DIR
# error USEE_ROUTINE_DIR not defined
# define USEE_ROUTINE_DIR // to pacify editors
#endif
#ifndef USEE_BAD_CONTENT
# error USEE_BAD_CONTENT not defined
# define USEE_BAD_CONTENT 1 // to pacify editors
#endif
#if USEE_ADD_DEF_TO_DECL
# error USEE_ADD_DEF_TO_DECL expected to be 0
#endif

// earlyUses-error@#MemberUsees_ctor_routine {{first '#pragma acc routine' for function 'MemberUsees::MemberUsees' not in scope at some uses}}
// earlyUses-note@#Derived_ctorImpInit {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#Derived_ctorExpInit {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#Composite_ctorUsing {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#Composite_ctorImpInit {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#Composite_ctorExpInit {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#Diamond_A_ctor {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#Diamond_B_ctor {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#Diamond_C_ctor {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#MemberUsees_ctorDelegation {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#accRoutineUse_MemberUsees_var {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#accRoutineUse_MemberUsees_tmp {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#accRoutineUse_MemberUsees_new_call {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#accParallelUse_MemberUsees_var {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#accParallelUse_MemberUsees_tmp {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#accParallelUse_MemberUsees_new_call {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#accLoopUse_MemberUsees_var {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#accLoopUse_MemberUsees_tmp {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#accLoopUse_MemberUsees_new_call {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#hostUse_MemberUsees_var {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#hostUse_MemberUsees_tmp {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#hostUse_MemberUsees_new_call {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#fileScope_MemberUsees_var {{use of function 'MemberUsees::MemberUsees' appears here}}
USEE_ROUTINE_DIR // #MemberUsees_ctor_routine
MemberUsees::MemberUsees() {
#if USEE_BAD_CONTENT
  // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'MemberUsees::MemberUsees' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_ctor_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::MemberUsees' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  static int x;
  // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'MemberUsees::MemberUsees' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_ctor_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::MemberUsees' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  #pragma acc parallel
  ;
  // funcBadContentExp-error@+3 {{function 'MemberUsees::MemberUsees' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  // funcBadContentExp-note@#MemberUsees_ctor_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
  // funcBadContentImp-error@+1 {{function 'MemberUsees::MemberUsees' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
#endif
}

// earlyUses-error@#MemberUsees_ctorCopy_routine {{first '#pragma acc routine' for function 'MemberUsees::MemberUsees' not in scope at some uses}}
// earlyUses-note@#accRoutineUse_MemberUsees_throw {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#accRoutineUse_passByVal_call {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#accParallelUse_MemberUsees_throw {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#accParallelUse_passByVal_call {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#accLoopUse_MemberUsees_throw {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#accLoopUse_passByVal_call {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#hostUse_MemberUsees_throw {{use of function 'MemberUsees::MemberUsees' appears here}}
// earlyUses-note@#hostUse_passByVal_call {{use of function 'MemberUsees::MemberUsees' appears here}}
USEE_ROUTINE_DIR // #MemberUsees_ctorCopy_routine
MemberUsees::MemberUsees(MemberUsees &) {
#if USEE_BAD_CONTENT
  // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'MemberUsees::MemberUsees' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_ctorCopy_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::MemberUsees' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  static int x;
  // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'MemberUsees::MemberUsees' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_ctorCopy_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::MemberUsees' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  #pragma acc parallel
  ;
  // funcBadContentExp-error@+3 {{function 'MemberUsees::MemberUsees' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  // funcBadContentExp-note@#MemberUsees_ctorCopy_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
  // funcBadContentImp-error@+1 {{function 'MemberUsees::MemberUsees' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
#endif
}

// earlyUses-error@#MemberUsees_ctorInherited_routine {{first '#pragma acc routine' for function 'MemberUsees::MemberUsees' not in scope at some uses}}
// FIXME: See inherited ctor FIXME in routine-cxx-funcs/README.md.
// earlyUses-note@#Derived_ctorInherited_use {{use of function 'MemberUsees::MemberUsees' appears here}}
USEE_ROUTINE_DIR // #MemberUsees_ctorInherited_routine
MemberUsees::MemberUsees(CtorInherited) {
  // We don't check bad content for this constructor as users.inc happens not to
  // imply a routine directive for it.  Besides, we check bad content in
  // constructors pretty well elsewhere.
}

// earlyUses-error@#MemberUsees_dtor_routine {{first '#pragma acc routine' for function 'MemberUsees::~MemberUsees' not in scope at some uses}}
// earlyUses-note@#MemberUsees_ctorDelegation {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#Derived_ctorUsing {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#Derived_ctorImpInit {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#Derived_ctorExpInit {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#Derived_dtor {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#Composite_ctorUsing {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#Composite_ctorImpInit {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#Composite_ctorExpInit {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#Composite_dtor {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#Diamond_A_ctor {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#Diamond_A_dtor {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#Diamond_B_ctor {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#Diamond_B_dtor {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#Diamond_C_ctor {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#Diamond_C_dtor {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#accRoutineUse_MemberUsees_var {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#accRoutineUse_MemberUsees_tmp {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#accRoutineUse_MemberUsees_throw {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#accRoutineUse_passByVal_call {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#accRoutineUse_MemberUsees_delete_call {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#accRoutineUse_MemberUsees_dtor_call {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#accParallelUse_MemberUsees_var {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#accParallelUse_MemberUsees_tmp {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#accParallelUse_MemberUsees_throw {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#accParallelUse_passByVal_call {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#accParallelUse_MemberUsees_delete_call {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#accParallelUse_MemberUsees_dtor_call {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#accLoopUse_MemberUsees_var {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#accLoopUse_MemberUsees_tmp {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#accLoopUse_MemberUsees_throw {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#accLoopUse_passByVal_call {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#accLoopUse_MemberUsees_delete_call {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#accLoopUse_MemberUsees_dtor_call {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#hostUse_MemberUsees_var {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#hostUse_MemberUsees_tmp {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#hostUse_MemberUsees_throw {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#hostUse_passByVal_call {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#hostUse_MemberUsees_delete_call {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#hostUse_MemberUsees_dtor_call {{use of function 'MemberUsees::~MemberUsees' appears here}}
// earlyUses-note@#fileScope_MemberUsees_var {{use of function 'MemberUsees::~MemberUsees' appears here}}
USEE_ROUTINE_DIR // #MemberUsees_dtor_routine
MemberUsees::~MemberUsees() {
#if USEE_BAD_CONTENT
  // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'MemberUsees::~MemberUsees' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::~MemberUsees' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  static int x;
  // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'MemberUsees::~MemberUsees' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::~MemberUsees' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  #pragma acc parallel
  ;
  // funcBadContentExp-error@+3 {{function 'MemberUsees::~MemberUsees' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  // funcBadContentExp-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
  // funcBadContentImp-error@+1 {{function 'MemberUsees::~MemberUsees' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
#endif
}

// earlyUses-error@#MemberUseesTrivial_ctor_routine {{first '#pragma acc routine' for function 'MemberUseesTrivial::MemberUseesTrivial' not in scope at some uses}}
// earlyUses-note@#accRoutineUse_MemberUseesTrivial_var {{use of function 'MemberUseesTrivial::MemberUseesTrivial' appears here}}
// earlyUses-note@#accParallelUse_MemberUseesTrivial_var {{use of function 'MemberUseesTrivial::MemberUseesTrivial' appears here}}
// earlyUses-note@#accLoopUse_MemberUseesTrivial_var {{use of function 'MemberUseesTrivial::MemberUseesTrivial' appears here}}
// earlyUses-note@#hostUse_MemberUseesTrivial_var {{use of function 'MemberUseesTrivial::MemberUseesTrivial' appears here}}
USEE_ROUTINE_DIR // #MemberUseesTrivial_ctor_routine
MemberUseesTrivial::MemberUseesTrivial() {
#if USEE_BAD_CONTENT
  // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'MemberUseesTrivial::MemberUseesTrivial' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUseesTrivial_ctor_routine {{'#pragma acc routine' for function 'MemberUseesTrivial::MemberUseesTrivial' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUseesTrivial::MemberUseesTrivial' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  static int x;
  // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'MemberUseesTrivial::MemberUseesTrivial' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUseesTrivial_ctor_routine {{'#pragma acc routine' for function 'MemberUseesTrivial::MemberUseesTrivial' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUseesTrivial::MemberUseesTrivial' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  #pragma acc parallel
  ;
  // funcBadContentExp-error@+3 {{function 'MemberUseesTrivial::MemberUseesTrivial' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  // funcBadContentExp-note@#MemberUseesTrivial_ctor_routine {{'#pragma acc routine' for function 'MemberUseesTrivial::MemberUseesTrivial' appears here}}
  // funcBadContentImp-error@+1 {{function 'MemberUseesTrivial::MemberUseesTrivial' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
#endif
}

// earlyUses-error@#MemberUseesTrivial_ctorCopy_routine {{first '#pragma acc routine' for function 'MemberUseesTrivial::MemberUseesTrivial' not in scope at some uses}}
// earlyUses-note@#accRoutineUse_passByVal_call {{use of function 'MemberUseesTrivial::MemberUseesTrivial' appears here}}
// earlyUses-note@#accParallelUse_passByVal_call {{use of function 'MemberUseesTrivial::MemberUseesTrivial' appears here}}
// earlyUses-note@#accLoopUse_passByVal_call {{use of function 'MemberUseesTrivial::MemberUseesTrivial' appears here}}
// earlyUses-note@#hostUse_passByVal_call {{use of function 'MemberUseesTrivial::MemberUseesTrivial' appears here}}
USEE_ROUTINE_DIR // #MemberUseesTrivial_ctorCopy_routine
MemberUseesTrivial::MemberUseesTrivial(MemberUseesTrivial &) {
#if USEE_BAD_CONTENT
  // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'MemberUseesTrivial::MemberUseesTrivial' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUseesTrivial_ctorCopy_routine {{'#pragma acc routine' for function 'MemberUseesTrivial::MemberUseesTrivial' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUseesTrivial::MemberUseesTrivial' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  static int x;
  // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'MemberUseesTrivial::MemberUseesTrivial' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUseesTrivial_ctorCopy_routine {{'#pragma acc routine' for function 'MemberUseesTrivial::MemberUseesTrivial' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUseesTrivial::MemberUseesTrivial' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  #pragma acc parallel
  ;
  // funcBadContentExp-error@+3 {{function 'MemberUseesTrivial::MemberUseesTrivial' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  // funcBadContentExp-note@#MemberUseesTrivial_ctorCopy_routine {{'#pragma acc routine' for function 'MemberUseesTrivial::MemberUseesTrivial' appears here}}
  // funcBadContentImp-error@+1 {{function 'MemberUseesTrivial::MemberUseesTrivial' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
#endif
}

// earlyUses-error@#MemberUseesTrivial_dtor_routine {{first '#pragma acc routine' for function 'MemberUseesTrivial::~MemberUseesTrivial' not in scope at some uses}}
// earlyUses-note@#passByVal_def {{use of function 'MemberUseesTrivial::~MemberUseesTrivial' appears here}}
// earlyUses-note@#accRoutineUse_MemberUseesTrivial_var {{use of function 'MemberUseesTrivial::~MemberUseesTrivial' appears here}}
// FIXME: See clang::trivial_abi FIXME in routine-cxx-funcs/README.md.
// earlyUses-note@#accRoutineUse_passByVal_call {{use of function 'MemberUseesTrivial::~MemberUseesTrivial' appears here}}
// earlyUses-note@#accParallelUse_MemberUseesTrivial_var {{use of function 'MemberUseesTrivial::~MemberUseesTrivial' appears here}}
// FIXME: See clang::trivial_abi FIXME in routine-cxx-funcs/README.md.
// earlyUses-note@#accParallelUse_passByVal_call {{use of function 'MemberUseesTrivial::~MemberUseesTrivial' appears here}}
// earlyUses-note@#accLoopUse_MemberUseesTrivial_var {{use of function 'MemberUseesTrivial::~MemberUseesTrivial' appears here}}
// FIXME: See clang::trivial_abi FIXME in routine-cxx-funcs/README.md.
// earlyUses-note@#accLoopUse_passByVal_call {{use of function 'MemberUseesTrivial::~MemberUseesTrivial' appears here}}
// earlyUses-note@#hostUse_MemberUseesTrivial_var {{use of function 'MemberUseesTrivial::~MemberUseesTrivial' appears here}}
// FIXME: See clang::trivial_abi FIXME in routine-cxx-funcs/README.md.
// earlyUses-note@#hostUse_passByVal_call {{use of function 'MemberUseesTrivial::~MemberUseesTrivial' appears here}}
USEE_ROUTINE_DIR // #MemberUseesTrivial_dtor_routine
MemberUseesTrivial::~MemberUseesTrivial() {
#if USEE_BAD_CONTENT
  // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'MemberUseesTrivial::~MemberUseesTrivial' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUseesTrivial_dtor_routine {{'#pragma acc routine' for function 'MemberUseesTrivial::~MemberUseesTrivial' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUseesTrivial::~MemberUseesTrivial' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  static int x;
  // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'MemberUseesTrivial::~MemberUseesTrivial' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUseesTrivial_dtor_routine {{'#pragma acc routine' for function 'MemberUseesTrivial::~MemberUseesTrivial' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUseesTrivial::~MemberUseesTrivial' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  #pragma acc parallel
  ;
  // funcBadContentExp-error@+3 {{function 'MemberUseesTrivial::~MemberUseesTrivial' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  // funcBadContentExp-note@#MemberUseesTrivial_dtor_routine {{'#pragma acc routine' for function 'MemberUseesTrivial::~MemberUseesTrivial' appears here}}
  // funcBadContentImp-error@+1 {{function 'MemberUseesTrivial::~MemberUseesTrivial' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
#endif
}

// earlyUses-error@#MemberUsees_new_routine {{first '#pragma acc routine' for function 'MemberUsees::operator new' not in scope at some uses}}
// earlyUses-note@#accRoutineUse_MemberUsees_new_call {{use of function 'MemberUsees::operator new' appears here}}
// earlyUses-note@#accParallelUse_MemberUsees_new_call {{use of function 'MemberUsees::operator new' appears here}}
// earlyUses-note@#accLoopUse_MemberUsees_new_call {{use of function 'MemberUsees::operator new' appears here}}
// earlyUses-note@#hostUse_MemberUsees_new_call {{use of function 'MemberUsees::operator new' appears here}}
USEE_ROUTINE_DIR // #MemberUsees_new_routine
void *MemberUsees::operator new(unsigned long s) {
#if USEE_BAD_CONTENT
  // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'MemberUsees::operator new' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_new_routine {{'#pragma acc routine' for function 'MemberUsees::operator new' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::operator new' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  static int x;
  // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'MemberUsees::operator new' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_new_routine {{'#pragma acc routine' for function 'MemberUsees::operator new' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::operator new' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  #pragma acc parallel
  ;
  // funcBadContentExp-error@+3 {{function 'MemberUsees::operator new' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  // funcBadContentExp-note@#MemberUsees_new_routine {{'#pragma acc routine' for function 'MemberUsees::operator new' appears here}}
  // funcBadContentImp-error@+1 {{function 'MemberUsees::operator new' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
#endif
  return (void*)1; // fake address that won't produce a warning
}

// earlyUses-error@#MemberUsees_delete_routine {{first '#pragma acc routine' for function 'MemberUsees::operator delete' not in scope at some uses}}
// earlyUses-note@#accRoutineUse_MemberUsees_new_call {{use of function 'MemberUsees::operator delete' appears here}}
// earlyUses-note@#accRoutineUse_MemberUsees_delete_call {{use of function 'MemberUsees::operator delete' appears here}}
// earlyUses-note@#accParallelUse_MemberUsees_new_call {{use of function 'MemberUsees::operator delete' appears here}}
// earlyUses-note@#accParallelUse_MemberUsees_delete_call {{use of function 'MemberUsees::operator delete' appears here}}
// earlyUses-note@#accLoopUse_MemberUsees_new_call {{use of function 'MemberUsees::operator delete' appears here}}
// earlyUses-note@#accLoopUse_MemberUsees_delete_call {{use of function 'MemberUsees::operator delete' appears here}}
// earlyUses-note@#hostUse_MemberUsees_new_call {{use of function 'MemberUsees::operator delete' appears here}}
// earlyUses-note@#hostUse_MemberUsees_delete_call {{use of function 'MemberUsees::operator delete' appears here}}
USEE_ROUTINE_DIR // #MemberUsees_delete_routine
void MemberUsees::operator delete(void *p) {
#if USEE_BAD_CONTENT
  // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'MemberUsees::operator delete' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_delete_routine {{'#pragma acc routine' for function 'MemberUsees::operator delete' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::operator delete' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  static int x;
  // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'MemberUsees::operator delete' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_delete_routine {{'#pragma acc routine' for function 'MemberUsees::operator delete' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::operator delete' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  #pragma acc parallel
  ;
  // funcBadContentExp-error@+3 {{function 'MemberUsees::operator delete' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  // funcBadContentExp-note@#MemberUsees_delete_routine {{'#pragma acc routine' for function 'MemberUsees::operator delete' appears here}}
  // funcBadContentImp-error@+1 {{function 'MemberUsees::operator delete' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
#endif
}

// earlyUses-error@#MemberUsees_opUnary_routine {{first '#pragma acc routine' for function 'MemberUsees::operator-' not in scope at some uses}}
// earlyUses-note@#accRoutineUse_MemberUsees_opUnary_call {{use of function 'MemberUsees::operator-' appears here}}
// earlyUses-note@#accParallelUse_MemberUsees_opUnary_call {{use of function 'MemberUsees::operator-' appears here}}
// earlyUses-note@#accLoopUse_MemberUsees_opUnary_call {{use of function 'MemberUsees::operator-' appears here}}
// earlyUses-note@#hostUse_MemberUsees_opUnary_call {{use of function 'MemberUsees::operator-' appears here}}
USEE_ROUTINE_DIR // #MemberUsees_opUnary_routine
int MemberUsees::operator-() const {
#if USEE_BAD_CONTENT
  // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'MemberUsees::operator-' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_opUnary_routine {{'#pragma acc routine' for function 'MemberUsees::operator-' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::operator-' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  static int x;
  // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'MemberUsees::operator-' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_opUnary_routine {{'#pragma acc routine' for function 'MemberUsees::operator-' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::operator-' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  #pragma acc parallel
  ;
  // funcBadContentExp-error@+3 {{function 'MemberUsees::operator-' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  // funcBadContentExp-note@#MemberUsees_opUnary_routine {{'#pragma acc routine' for function 'MemberUsees::operator-' appears here}}
  // funcBadContentImp-error@+1 {{function 'MemberUsees::operator-' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
#endif
  return -1;
}

// earlyUses-error@#MemberUsees_opBinary_routine {{first '#pragma acc routine' for function 'MemberUsees::operator+' not in scope at some uses}}
// earlyUses-note@#accRoutineUse_MemberUsees_opBinary_call {{use of function 'MemberUsees::operator+' appears here}}
// earlyUses-note@#accParallelUse_MemberUsees_opBinary_call {{use of function 'MemberUsees::operator+' appears here}}
// earlyUses-note@#accLoopUse_MemberUsees_opBinary_call {{use of function 'MemberUsees::operator+' appears here}}
// earlyUses-note@#hostUse_MemberUsees_opBinary_call {{use of function 'MemberUsees::operator+' appears here}}
USEE_ROUTINE_DIR // #MemberUsees_opBinary_routine
int MemberUsees::operator+(int) const {
#if USEE_BAD_CONTENT
  // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'MemberUsees::operator+' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_opBinary_routine {{'#pragma acc routine' for function 'MemberUsees::operator+' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::operator+' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  static int x;
  // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'MemberUsees::operator+' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_opBinary_routine {{'#pragma acc routine' for function 'MemberUsees::operator+' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::operator+' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  #pragma acc parallel
  ;
  // funcBadContentExp-error@+3 {{function 'MemberUsees::operator+' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  // funcBadContentExp-note@#MemberUsees_opBinary_routine {{'#pragma acc routine' for function 'MemberUsees::operator+' appears here}}
  // funcBadContentImp-error@+1 {{function 'MemberUsees::operator+' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
#endif
  return 0;
}

// earlyUses-error@#MemberUsees_opSubscript_routine {{first '#pragma acc routine' for function 'MemberUsees::operator[]' not in scope at some uses}}
// earlyUses-note@#accRoutineUse_MemberUsees_opSubscript_call {{use of function 'MemberUsees::operator[]' appears here}}
// earlyUses-note@#accParallelUse_MemberUsees_opSubscript_call {{use of function 'MemberUsees::operator[]' appears here}}
// earlyUses-note@#accLoopUse_MemberUsees_opSubscript_call {{use of function 'MemberUsees::operator[]' appears here}}
// earlyUses-note@#hostUse_MemberUsees_opSubscript_call {{use of function 'MemberUsees::operator[]' appears here}}
USEE_ROUTINE_DIR // #MemberUsees_opSubscript_routine
int MemberUsees::operator[](int) const {
#if USEE_BAD_CONTENT
  // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'MemberUsees::operator[]' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_opSubscript_routine {{'#pragma acc routine' for function 'MemberUsees::operator[]' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::operator[]' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  static int x;
  // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'MemberUsees::operator[]' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_opSubscript_routine {{'#pragma acc routine' for function 'MemberUsees::operator[]' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::operator[]' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  #pragma acc parallel
  ;
  // funcBadContentExp-error@+3 {{function 'MemberUsees::operator[]' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  // funcBadContentExp-note@#MemberUsees_opSubscript_routine {{'#pragma acc routine' for function 'MemberUsees::operator[]' appears here}}
  // funcBadContentImp-error@+1 {{function 'MemberUsees::operator[]' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
#endif
  return 0;
}

// earlyUses-error@#MemberUsees_opCall_routine {{first '#pragma acc routine' for function 'MemberUsees::operator()' not in scope at some uses}}
// earlyUses-note@#accRoutineUse_MemberUsees_opCall_call {{use of function 'MemberUsees::operator()' appears here}}
// earlyUses-note@#accParallelUse_MemberUsees_opCall_call {{use of function 'MemberUsees::operator()' appears here}}
// earlyUses-note@#accLoopUse_MemberUsees_opCall_call {{use of function 'MemberUsees::operator()' appears here}}
// earlyUses-note@#hostUse_MemberUsees_opCall_call {{use of function 'MemberUsees::operator()' appears here}}
USEE_ROUTINE_DIR // #MemberUsees_opCall_routine
int MemberUsees::operator()() const {
#if USEE_BAD_CONTENT
  // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'MemberUsees::operator()' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_opCall_routine {{'#pragma acc routine' for function 'MemberUsees::operator()' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::operator()' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  static int x;
  // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'MemberUsees::operator()' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_opCall_routine {{'#pragma acc routine' for function 'MemberUsees::operator()' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::operator()' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  #pragma acc parallel
  ;
  // funcBadContentExp-error@+3 {{function 'MemberUsees::operator()' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  // funcBadContentExp-note@#MemberUsees_opCall_routine {{'#pragma acc routine' for function 'MemberUsees::operator()' appears here}}
  // funcBadContentImp-error@+1 {{function 'MemberUsees::operator()' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
#endif
  return 0;
}

// earlyUses-error@#MemberUsees_opArrow_routine {{first '#pragma acc routine' for function 'MemberUsees::operator->' not in scope at some uses}}
// earlyUses-note@#accRoutineUse_MemberUsees_opArrow_call {{use of function 'MemberUsees::operator->' appears here}}
// earlyUses-note@#accParallelUse_MemberUsees_opArrow_call {{use of function 'MemberUsees::operator->' appears here}}
// earlyUses-note@#accLoopUse_MemberUsees_opArrow_call {{use of function 'MemberUsees::operator->' appears here}}
// earlyUses-note@#hostUse_MemberUsees_opArrow_call {{use of function 'MemberUsees::operator->' appears here}}
USEE_ROUTINE_DIR // #MemberUsees_opArrow_routine
MemberUsees::ArrowResult MemberUsees::operator->() {
#if USEE_BAD_CONTENT
  // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'MemberUsees::operator->' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_opArrow_routine {{'#pragma acc routine' for function 'MemberUsees::operator->' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::operator->' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  static int x;
  // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'MemberUsees::operator->' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_opArrow_routine {{'#pragma acc routine' for function 'MemberUsees::operator->' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::operator->' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  #pragma acc parallel
  ;
  // funcBadContentExp-error@+3 {{function 'MemberUsees::operator->' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  // funcBadContentExp-note@#MemberUsees_opArrow_routine {{'#pragma acc routine' for function 'MemberUsees::operator->' appears here}}
  // funcBadContentImp-error@+1 {{function 'MemberUsees::operator->' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
#endif
  return arrowResult;
}

// earlyUses-error@#MemberUsees_ArrowResult_opArrow_routine {{first '#pragma acc routine' for function 'MemberUsees::ArrowResult::operator->' not in scope at some uses}}
// earlyUses-note@#accRoutineUse_MemberUsees_opArrow_call {{use of function 'MemberUsees::ArrowResult::operator->' appears here}}
// earlyUses-note@#accParallelUse_MemberUsees_opArrow_call {{use of function 'MemberUsees::ArrowResult::operator->' appears here}}
// earlyUses-note@#accLoopUse_MemberUsees_opArrow_call {{use of function 'MemberUsees::ArrowResult::operator->' appears here}}
// earlyUses-note@#hostUse_MemberUsees_opArrow_call {{use of function 'MemberUsees::ArrowResult::operator->' appears here}}
USEE_ROUTINE_DIR // #MemberUsees_ArrowResult_opArrow_routine
MemberUsees::ArrowResult::ArrowResultInner *
MemberUsees::ArrowResult::operator->() {
#if USEE_BAD_CONTENT
  // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'MemberUsees::ArrowResult::operator->' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_ArrowResult_opArrow_routine {{'#pragma acc routine' for function 'MemberUsees::ArrowResult::operator->' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::ArrowResult::operator->' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  static int x;
  // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'MemberUsees::ArrowResult::operator->' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_ArrowResult_opArrow_routine {{'#pragma acc routine' for function 'MemberUsees::ArrowResult::operator->' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::ArrowResult::operator->' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  #pragma acc parallel
  ;
  // funcBadContentExp-error@+3 {{function 'MemberUsees::ArrowResult::operator->' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  // funcBadContentExp-note@#MemberUsees_ArrowResult_opArrow_routine {{'#pragma acc routine' for function 'MemberUsees::ArrowResult::operator->' appears here}}
  // funcBadContentImp-error@+1 {{function 'MemberUsees::ArrowResult::operator->' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
#endif
  return &arrowResultInner;
}

// earlyUses-error@#MemberUsees_opConvert_routine {{first '#pragma acc routine' for function 'MemberUsees::operator int' not in scope at some uses}}
// earlyUses-note@#accRoutineUse_MemberUsees_opConvert_callImp {{use of function 'MemberUsees::operator int' appears here}}
// earlyUses-note@#accRoutineUse_MemberUsees_opConvert_callExp {{use of function 'MemberUsees::operator int' appears here}}
// earlyUses-note@#accParallelUse_MemberUsees_opConvert_callImp {{use of function 'MemberUsees::operator int' appears here}}
// earlyUses-note@#accParallelUse_MemberUsees_opConvert_callExp {{use of function 'MemberUsees::operator int' appears here}}
// earlyUses-note@#accLoopUse_MemberUsees_opConvert_callImp {{use of function 'MemberUsees::operator int' appears here}}
// earlyUses-note@#accLoopUse_MemberUsees_opConvert_callExp {{use of function 'MemberUsees::operator int' appears here}}
// earlyUses-note@#hostUse_MemberUsees_opConvert_callImp {{use of function 'MemberUsees::operator int' appears here}}
// earlyUses-note@#hostUse_MemberUsees_opConvert_callExp {{use of function 'MemberUsees::operator int' appears here}}
USEE_ROUTINE_DIR // #MemberUsees_opConvert_routine
MemberUsees::operator int() const {
#if USEE_BAD_CONTENT
  // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'MemberUsees::operator int' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_opConvert_routine {{'#pragma acc routine' for function 'MemberUsees::operator int' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::operator int' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  static int x;
  // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'MemberUsees::operator int' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_opConvert_routine {{'#pragma acc routine' for function 'MemberUsees::operator int' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::operator int' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  #pragma acc parallel
  ;
  // funcBadContentExp-error@+3 {{function 'MemberUsees::operator int' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  // funcBadContentExp-note@#MemberUsees_opConvert_routine {{'#pragma acc routine' for function 'MemberUsees::operator int' appears here}}
  // funcBadContentImp-error@+1 {{function 'MemberUsees::operator int' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
#endif
  return 0;
}

// earlyUses-error@#MemberUsees_opConvertSurrogate_routine {{first '#pragma acc routine' for function 'MemberUsees::operator int (*)(int)' not in scope at some uses}}
// earlyUses-note@#accRoutineUse_MemberUsees_opConvertSurrogate_call {{use of function 'MemberUsees::operator int (*)(int)' appears here}}
// earlyUses-note@#accParallelUse_MemberUsees_opConvertSurrogate_call {{use of function 'MemberUsees::operator int (*)(int)' appears here}}
// earlyUses-note@#accLoopUse_MemberUsees_opConvertSurrogate_call {{use of function 'MemberUsees::operator int (*)(int)' appears here}}
// earlyUses-note@#hostUse_MemberUsees_opConvertSurrogate_call {{use of function 'MemberUsees::operator int (*)(int)' appears here}}
USEE_ROUTINE_DIR // #MemberUsees_opConvertSurrogate_routine
MemberUsees::operator FnPtr() const {
#if USEE_BAD_CONTENT
  // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'MemberUsees::operator int (*)(int)' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_opConvertSurrogate_routine {{'#pragma acc routine' for function 'MemberUsees::operator int (*)(int)' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::operator int (*)(int)' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  static int x;
  // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'MemberUsees::operator int (*)(int)' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_opConvertSurrogate_routine {{'#pragma acc routine' for function 'MemberUsees::operator int (*)(int)' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::operator int (*)(int)' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  #pragma acc parallel
  ;
  // funcBadContentExp-error@+3 {{function 'MemberUsees::operator int (*)(int)' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  // funcBadContentExp-note@#MemberUsees_opConvertSurrogate_routine {{'#pragma acc routine' for function 'MemberUsees::operator int (*)(int)' appears here}}
  // funcBadContentImp-error@+1 {{function 'MemberUsees::operator int (*)(int)' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
#endif
  return 0;
}

// earlyUses-error@#MemberUsees_begin_routine {{first '#pragma acc routine' for function 'MemberUsees::begin' not in scope at some uses}}
// earlyUses-note@#accRoutineUse_MemberUsees_rangeBasedLoop {{use of function 'MemberUsees::begin' appears here}}
// earlyUses-note@#accParallelUse_MemberUsees_rangeBasedLoop {{use of function 'MemberUsees::begin' appears here}}
// earlyUses-note@#accLoopUse_MemberUsees_rangeBasedLoop {{use of function 'MemberUsees::begin' appears here}}
// earlyUses-note@#hostUse_MemberUsees_rangeBasedLoop {{use of function 'MemberUsees::begin' appears here}}
USEE_ROUTINE_DIR // #MemberUsees_begin_routine
int *MemberUsees::begin() {
#if USEE_BAD_CONTENT
  // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'MemberUsees::begin' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_begin_routine {{'#pragma acc routine' for function 'MemberUsees::begin' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::begin' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  static int x;
  // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'MemberUsees::begin' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_begin_routine {{'#pragma acc routine' for function 'MemberUsees::begin' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::begin' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  #pragma acc parallel
  ;
  // funcBadContentExp-error@+3 {{function 'MemberUsees::begin' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  // funcBadContentExp-note@#MemberUsees_begin_routine {{'#pragma acc routine' for function 'MemberUsees::begin' appears here}}
  // funcBadContentImp-error@+1 {{function 'MemberUsees::begin' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
#endif
  return nullptr;
}

// earlyUses-error@#MemberUsees_end_routine {{first '#pragma acc routine' for function 'MemberUsees::end' not in scope at some uses}}
// earlyUses-note@#accRoutineUse_MemberUsees_rangeBasedLoop {{use of function 'MemberUsees::end' appears here}}
// earlyUses-note@#accParallelUse_MemberUsees_rangeBasedLoop {{use of function 'MemberUsees::end' appears here}}
// earlyUses-note@#accLoopUse_MemberUsees_rangeBasedLoop {{use of function 'MemberUsees::end' appears here}}
// earlyUses-note@#hostUse_MemberUsees_rangeBasedLoop {{use of function 'MemberUsees::end' appears here}}
USEE_ROUTINE_DIR // #MemberUsees_end_routine
int *MemberUsees::end() {
#if USEE_BAD_CONTENT
  // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'MemberUsees::end' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_end_routine {{'#pragma acc routine' for function 'MemberUsees::end' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::end' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  static int x;
  // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'MemberUsees::end' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#MemberUsees_end_routine {{'#pragma acc routine' for function 'MemberUsees::end' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'MemberUsees::end' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  #pragma acc parallel
  ;
  // funcBadContentExp-error@+3 {{function 'MemberUsees::end' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  // funcBadContentExp-note@#MemberUsees_end_routine {{'#pragma acc routine' for function 'MemberUsees::end' appears here}}
  // funcBadContentImp-error@+1 {{function 'MemberUsees::end' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
#endif
  return nullptr;
}

// earlyUses-error@#fnUsee_routine {{first '#pragma acc routine' for function 'fnUsee' not in scope at some uses}}
// earlyUses-note@#templateFunc_call {{use of function 'fnUsee' appears here}}
USEE_ROUTINE_DIR // #fnUsee_routine
void fnUsee() {
#if USEE_BAD_CONTENT
  // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'fnUsee' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#fnUsee_routine  {{'#pragma acc routine' for function 'fnUsee' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'fnUsee' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  static int x;
  // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'fnUsee' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#fnUsee_routine  {{'#pragma acc routine' for function 'fnUsee' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'fnUsee' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  #pragma acc parallel
  ;
  // funcBadContentExp-error@+3 {{function 'fnUsee' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  // funcBadContentExp-note@#fnUsee_routine  {{'#pragma acc routine' for function 'fnUsee' appears here}}
  // funcBadContentImp-error@+1 {{function 'fnUsee' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
#endif
}

// earlyUses-error@#fnUseeInNestedHostLambda_routine {{first '#pragma acc routine' for function 'fnUseeInNestedHostLambda' not in scope at some uses}}
// earlyUses-note@#fnUseeInNestedHostLambda_call {{use of function 'fnUseeInNestedHostLambda' appears here}}
USEE_ROUTINE_DIR // #fnUseeInNestedHostLambda_routine
void fnUseeInNestedHostLambda() {
#if USEE_BAD_CONTENT
  // funcBadContentExp-error@+2 {{static local variable 'x' is not permitted within function 'fnUseeInNestedHostLambda' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#fnUseeInNestedHostLambda_routine  {{'#pragma acc routine' for function 'fnUseeInNestedHostLambda' appears here}}
  static int x;
  // funcBadContentExp-error@+2 {{'#pragma acc parallel' is not permitted within function 'fnUseeInNestedHostLambda' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#fnUseeInNestedHostLambda_routine  {{'#pragma acc routine' for function 'fnUseeInNestedHostLambda' appears here}}
  #pragma acc parallel
  ;
  // funcBadContentExp-error@+3 {{function 'fnUseeInNestedHostLambda' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  // funcBadContentExp-note@#fnUseeInNestedHostLambda_routine  {{'#pragma acc routine' for function 'fnUseeInNestedHostLambda' appears here}}
  // funcBadContentImp-error@+1 {{function 'fnUseeInNestedHostLambda' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
#endif
}

// earlyUses-error@#fnUseeInHostLambda_routine {{first '#pragma acc routine' for function 'fnUseeInHostLambda' not in scope at some uses}}
// earlyUses-note@#fnUseeInHostLambda_call {{use of function 'fnUseeInHostLambda' appears here}}
USEE_ROUTINE_DIR // #fnUseeInHostLambda_routine
void fnUseeInHostLambda() {
#if USEE_BAD_CONTENT
  // funcBadContentExp-error@+2 {{static local variable 'x' is not permitted within function 'fnUseeInHostLambda' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#fnUseeInHostLambda_routine  {{'#pragma acc routine' for function 'fnUseeInHostLambda' appears here}}
  static int x;
  // funcBadContentExp-error@+2 {{'#pragma acc parallel' is not permitted within function 'fnUseeInHostLambda' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#fnUseeInHostLambda_routine  {{'#pragma acc routine' for function 'fnUseeInHostLambda' appears here}}
  #pragma acc parallel
  ;
  // funcBadContentExp-error@+3 {{function 'fnUseeInHostLambda' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  // funcBadContentExp-note@#fnUseeInHostLambda_routine  {{'#pragma acc routine' for function 'fnUseeInHostLambda' appears here}}
  // funcBadContentImp-error@+1 {{function 'fnUseeInHostLambda' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
#endif
}

// earlyUses-error@#fnUseeInNestedAccRoutineLambda_routine {{first '#pragma acc routine' for function 'fnUseeInNestedAccRoutineLambda' not in scope at some uses}}
// earlyUses-note@#fnUseeInNestedAccRoutineLambda_call {{use of function 'fnUseeInNestedAccRoutineLambda' appears here}}
USEE_ROUTINE_DIR // #fnUseeInNestedAccRoutineLambda_routine
void fnUseeInNestedAccRoutineLambda() {
#if USEE_BAD_CONTENT
  // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'fnUseeInNestedAccRoutineLambda' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#fnUseeInNestedAccRoutineLambda_routine  {{'#pragma acc routine' for function 'fnUseeInNestedAccRoutineLambda' appears here}}
  // funcBadContentImp-note@#fnUseeInNestedAccRoutineLambda_call {{'#pragma acc routine seq' implied for function 'fnUseeInNestedAccRoutineLambda' by its use in function 'lambdaDefiner()::(anonymous class)::operator()' here}}
  // funcBadContentImp-note@#lambdaDefiner_accLambda_call {{'#pragma acc routine seq' implied for function 'lambdaDefiner()::(anonymous class)::operator()' by its use in construct '#pragma acc parallel' here}}
  static int x;
  // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'fnUseeInNestedAccRoutineLambda' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#fnUseeInNestedAccRoutineLambda_routine  {{'#pragma acc routine' for function 'fnUseeInNestedAccRoutineLambda' appears here}}
  // funcBadContentImp-note@#fnUseeInNestedAccRoutineLambda_call {{'#pragma acc routine seq' implied for function 'fnUseeInNestedAccRoutineLambda' by its use in function 'lambdaDefiner()::(anonymous class)::operator()' here}}
  // funcBadContentImp-note@#lambdaDefiner_accLambda_call {{'#pragma acc routine seq' implied for function 'lambdaDefiner()::(anonymous class)::operator()' by its use in construct '#pragma acc parallel' here}}
  #pragma acc parallel
  ;
  // funcBadContentExp-error@+3 {{function 'fnUseeInNestedAccRoutineLambda' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  // funcBadContentExp-note@#fnUseeInNestedAccRoutineLambda_routine  {{'#pragma acc routine' for function 'fnUseeInNestedAccRoutineLambda' appears here}}
  // funcBadContentImp-error@+1 {{function 'fnUseeInNestedAccRoutineLambda' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
#endif
}

// earlyUses-error@#fnUseeInAccRoutineLambda_routine {{first '#pragma acc routine' for function 'fnUseeInAccRoutineLambda' not in scope at some uses}}
// earlyUses-note@#fnUseeInAccRoutineLambda_call {{use of function 'fnUseeInAccRoutineLambda' appears here}}
USEE_ROUTINE_DIR // #fnUseeInAccRoutineLambda_routine
void fnUseeInAccRoutineLambda() {
#if USEE_BAD_CONTENT
  // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'fnUseeInAccRoutineLambda' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#fnUseeInAccRoutineLambda_routine  {{'#pragma acc routine' for function 'fnUseeInAccRoutineLambda' appears here}}
  // funcBadContentImp-note@#fnUseeInAccRoutineLambda_call {{'#pragma acc routine seq' implied for function 'fnUseeInAccRoutineLambda' by its use in function '(anonymous class)::operator()' here}}
  // funcBadContentImp-note@#accLambda_call {{'#pragma acc routine seq' implied for function '(anonymous class)::operator()' by its use in construct '#pragma acc parallel' here}}
  static int x;
  // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'fnUseeInAccRoutineLambda' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#fnUseeInAccRoutineLambda_routine  {{'#pragma acc routine' for function 'fnUseeInAccRoutineLambda' appears here}}
  // funcBadContentImp-note@#fnUseeInAccRoutineLambda_call {{'#pragma acc routine seq' implied for function 'fnUseeInAccRoutineLambda' by its use in function '(anonymous class)::operator()' here}}
  // funcBadContentImp-note@#accLambda_call {{'#pragma acc routine seq' implied for function '(anonymous class)::operator()' by its use in construct '#pragma acc parallel' here}}
  #pragma acc parallel
  ;
  // funcBadContentExp-error@+3 {{function 'fnUseeInAccRoutineLambda' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  // funcBadContentExp-note@#fnUseeInAccRoutineLambda_routine  {{'#pragma acc routine' for function 'fnUseeInAccRoutineLambda' appears here}}
  // funcBadContentImp-error@+1 {{function 'fnUseeInAccRoutineLambda' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
#endif
}

// earlyUses-error@#fnUseeInAccParallelLambda_routine {{first '#pragma acc routine' for function 'fnUseeInAccParallelLambda' not in scope at some uses}}
// earlyUses-note@#fnUseeInAccParallelLambda_call {{use of function 'fnUseeInAccParallelLambda' appears here}}
USEE_ROUTINE_DIR // #fnUseeInAccParallelLambda_routine
void fnUseeInAccParallelLambda() {
#if USEE_BAD_CONTENT
  // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'fnUseeInAccParallelLambda' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#fnUseeInAccParallelLambda_routine  {{'#pragma acc routine' for function 'fnUseeInAccParallelLambda' appears here}}
  // funcBadContentImp-note@#fnUseeInAccParallelLambda_call {{'#pragma acc routine seq' implied for function 'fnUseeInAccParallelLambda' by its use in function 'lambdaDefiner()::(anonymous class)::operator()' here}}
  // funcBadContentImp-note@#lambdaDefiner_accParallelLambda_call {{'#pragma acc routine seq' implied for function 'lambdaDefiner()::(anonymous class)::operator()' by its use in construct '#pragma acc parallel' here}}
  static int x;
  // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'fnUseeInAccParallelLambda' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#fnUseeInAccParallelLambda_routine  {{'#pragma acc routine' for function 'fnUseeInAccParallelLambda' appears here}}
  // funcBadContentImp-note@#fnUseeInAccParallelLambda_call {{'#pragma acc routine seq' implied for function 'fnUseeInAccParallelLambda' by its use in function 'lambdaDefiner()::(anonymous class)::operator()' here}}
  // funcBadContentImp-note@#lambdaDefiner_accParallelLambda_call {{'#pragma acc routine seq' implied for function 'lambdaDefiner()::(anonymous class)::operator()' by its use in construct '#pragma acc parallel' here}}
  #pragma acc parallel
  ;
  // funcBadContentExp-error@+3 {{function 'fnUseeInAccParallelLambda' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  // funcBadContentExp-note@#fnUseeInAccParallelLambda_routine  {{'#pragma acc routine' for function 'fnUseeInAccParallelLambda' appears here}}
  // funcBadContentImp-error@+1 {{function 'fnUseeInAccParallelLambda' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
#endif
}

// earlyUses-error@#fnUseeInAccLoopLambda_routine {{first '#pragma acc routine' for function 'fnUseeInAccLoopLambda' not in scope at some uses}}
// earlyUses-note@#fnUseeInAccLoopLambda_call {{use of function 'fnUseeInAccLoopLambda' appears here}}
USEE_ROUTINE_DIR // #fnUseeInAccLoopLambda_routine
void fnUseeInAccLoopLambda() {
#if USEE_BAD_CONTENT
  // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'fnUseeInAccLoopLambda' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#fnUseeInAccLoopLambda_routine  {{'#pragma acc routine' for function 'fnUseeInAccLoopLambda' appears here}}
  // funcBadContentImp-note@#fnUseeInAccLoopLambda_call {{'#pragma acc routine seq' implied for function 'fnUseeInAccLoopLambda' by its use in function 'lambdaDefiner()::(anonymous class)::operator()' here}}
  // funcBadContentImp-note@#lambdaDefiner_accLoopLambda_loop {{'#pragma acc routine seq' implied for function 'lambdaDefiner()::(anonymous class)::operator()' by its use of construct '#pragma acc loop' here}}
  static int x;
  // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'fnUseeInAccLoopLambda' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#fnUseeInAccLoopLambda_routine  {{'#pragma acc routine' for function 'fnUseeInAccLoopLambda' appears here}}
  // funcBadContentImp-note@#fnUseeInAccLoopLambda_call {{'#pragma acc routine seq' implied for function 'fnUseeInAccLoopLambda' by its use in function 'lambdaDefiner()::(anonymous class)::operator()' here}}
  // funcBadContentImp-note@#lambdaDefiner_accLoopLambda_loop {{'#pragma acc routine seq' implied for function 'lambdaDefiner()::(anonymous class)::operator()' by its use of construct '#pragma acc loop' here}}
  #pragma acc parallel
  ;
  // funcBadContentExp-error@+3 {{function 'fnUseeInAccLoopLambda' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  // funcBadContentExp-note@#fnUseeInAccLoopLambda_routine  {{'#pragma acc routine' for function 'fnUseeInAccLoopLambda' appears here}}
  // funcBadContentImp-error@+1 {{function 'fnUseeInAccLoopLambda' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
#endif
}

// earlyUses-error@#NamespaceUsees_fn_routine {{first '#pragma acc routine' for function 'NamespaceUsees::fn' not in scope at some uses}}
// earlyUses-note@#NamespaceUsees_outsideUser_call {{use of function 'NamespaceUsees::fn' appears here}}
// earlyUses-note@#NamespaceUsees_user_call {{use of function 'NamespaceUsees::fn' appears here}}
USEE_ROUTINE_DIR // #NamespaceUsees_fn_routine
void NamespaceUsees::fn() {
#if USEE_BAD_CONTENT
  // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'NamespaceUsees::fn' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#NamespaceUsees_fn_routine  {{'#pragma acc routine' for function 'NamespaceUsees::fn' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'NamespaceUsees::fn' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  static int x;
  // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'NamespaceUsees::fn' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#NamespaceUsees_fn_routine  {{'#pragma acc routine' for function 'NamespaceUsees::fn' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function 'NamespaceUsees::fn' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  #pragma acc parallel
  ;
  // funcBadContentExp-error@+3 {{function 'NamespaceUsees::fn' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  // funcBadContentExp-note@#NamespaceUsees_fn_routine  {{'#pragma acc routine' for function 'NamespaceUsees::fn' appears here}}
  // funcBadContentImp-error@+1 {{function 'NamespaceUsees::fn' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
#endif
}

// Because lambdas currently cannot have explicit routine directives, this
// function is called within lambdas to imply their routine directives instead.
USEE_ROUTINE_DIR // #lambdaUseeUsee_routine
void lambdaUseeUsee() {}

// At one time, Clang's OpenACC analysis overlooked enclosing lambdas as if
// their contents appeared at file scope, in an enclosing function, or in an
// enclosing construct.
//
// Because explicit routine directives are not yet supported for lambdas, and
// because an implicit routine directive for a lambda is determined by its
// usees, an orphaned loop construct in a lambda cannot be bad content, so that
// case is skipped below.
//
// Like other usees, the file-scope lambdaUsee below is used in users.inc.  Each
// remaining lambdaUsee cannot be used elsewhere, so it's used immediately
// below.
auto lambdaUsee = []() {
#if USEE_BAD_CONTENT
  // funcBadContent-error@+5 {{static local variable 'x' is not permitted within function '(anonymous class)::operator()' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#lambdaUsee_lambdaUseeUsee_call{{'#pragma acc routine vector' implied for function '(anonymous class)::operator()' by its use of function 'lambdaUseeUsee' here}}
  // funcBadContentExp-note@#lambdaUseeUsee_routine {{'#pragma acc routine' for function 'lambdaUseeUsee' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function '(anonymous class)::operator()' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  static int x;
  // funcBadContent-error@+5 {{'#pragma acc parallel' is not permitted within function '(anonymous class)::operator()' because the latter is attributed with '#pragma acc routine'}}
  // funcBadContentExp-note@#lambdaUsee_lambdaUseeUsee_call{{'#pragma acc routine vector' implied for function '(anonymous class)::operator()' by its use of function 'lambdaUseeUsee' here}}
  // funcBadContentExp-note@#lambdaUseeUsee_routine {{'#pragma acc routine' for function 'lambdaUseeUsee' appears here}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine seq' implied for function '(anonymous class)::operator()' by its use in}}
  // funcBadContentImp-note@*:* {{'#pragma acc routine' for function}}
  #pragma acc parallel
  ;
#endif
  lambdaUseeUsee(); // #lambdaUsee_lambdaUseeUsee_call
};
void lambdaUseeDefinerAndUser() {
  auto lambdaUsee = []() {
#if USEE_BAD_CONTENT
    // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'lambdaUseeDefinerAndUser()::(anonymous class)::operator()' because the latter is attributed with '#pragma acc routine'}}
    // funcBadContentExp-note@#lambdaUseeDefinerAndUser_lambdaUsee_lambdaUseeUsee_call {{'#pragma acc routine vector' implied for function 'lambdaUseeDefinerAndUser()::(anonymous class)::operator()' by its use of function 'lambdaUseeUsee' here}}
    // funcBadContentExp-note@#lambdaUseeUsee_routine {{'#pragma acc routine' for function 'lambdaUseeUsee' appears here}}
    // funcBadContentImp-note@#lambdaUseeDefinerAndUser_lambdaUsee_call {{'#pragma acc routine seq' implied for function 'lambdaUseeDefinerAndUser()::(anonymous class)::operator()' by its use in construct '#pragma acc parallel' here}}
    static int x;
    // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'lambdaUseeDefinerAndUser()::(anonymous class)::operator()' because the latter is attributed with '#pragma acc routine'}}
    // funcBadContentExp-note@#lambdaUseeDefinerAndUser_lambdaUsee_lambdaUseeUsee_call {{'#pragma acc routine vector' implied for function 'lambdaUseeDefinerAndUser()::(anonymous class)::operator()' by its use of function 'lambdaUseeUsee' here}}
    // funcBadContentExp-note@#lambdaUseeUsee_routine {{'#pragma acc routine' for function 'lambdaUseeUsee' appears here}}
    // funcBadContentImp-note@#lambdaUseeDefinerAndUser_lambdaUsee_call {{'#pragma acc routine seq' implied for function 'lambdaUseeDefinerAndUser()::(anonymous class)::operator()' by its use in construct '#pragma acc parallel' here}}
    #pragma acc parallel
    ;
#endif
    lambdaUseeUsee(); // #lambdaUseeDefinerAndUser_lambdaUsee_lambdaUseeUsee_call
  };
  #pragma acc parallel
  lambdaUsee(); // #lambdaUseeDefinerAndUser_lambdaUsee_call
}
void accParallelLambdaUseeDefinerAndUser() {
  #pragma acc parallel
  {
    []() {
#if USEE_BAD_CONTENT
      // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'accParallelLambdaUseeDefinerAndUser()::(anonymous class)::operator()' because the latter is attributed with '#pragma acc routine'}}
      // funcBadContentExp-note@#accParallelLambdaUseeDefinerAndUser_lambdaUsee_lambdaUseeUsee_call {{'#pragma acc routine vector' implied for function 'accParallelLambdaUseeDefinerAndUser()::(anonymous class)::operator()' by its use of function 'lambdaUseeUsee' here}}
      // funcBadContentExp-note@#lambdaUseeUsee_routine {{'#pragma acc routine' for function 'lambdaUseeUsee' appears here}}
      // funcBadContentImp-note@#accParallelLambdaUseeDefinerAndUser_lambdaUsee_call {{'#pragma acc routine seq' implied for function 'accParallelLambdaUseeDefinerAndUser()::(anonymous class)::operator()' by its use in construct '#pragma acc parallel' here}}
      static int x;
      // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'accParallelLambdaUseeDefinerAndUser()::(anonymous class)::operator()' because the latter is attributed with '#pragma acc routine'}}
      // funcBadContentExp-note@#accParallelLambdaUseeDefinerAndUser_lambdaUsee_lambdaUseeUsee_call {{'#pragma acc routine vector' implied for function 'accParallelLambdaUseeDefinerAndUser()::(anonymous class)::operator()' by its use of function 'lambdaUseeUsee' here}}
      // funcBadContentExp-note@#lambdaUseeUsee_routine {{'#pragma acc routine' for function 'lambdaUseeUsee' appears here}}
      // funcBadContentImp-note@#accParallelLambdaUseeDefinerAndUser_lambdaUsee_call {{'#pragma acc routine seq' implied for function 'accParallelLambdaUseeDefinerAndUser()::(anonymous class)::operator()' by its use in construct '#pragma acc parallel' here}}
      #pragma acc parallel
      ;
#endif
      lambdaUseeUsee(); // #accParallelLambdaUseeDefinerAndUser_lambdaUsee_lambdaUseeUsee_call
    }(); // #accParallelLambdaUseeDefinerAndUser_lambdaUsee_call
  }
}
void accLoopLambdaUseeDefinerAndUser() {
  #pragma acc parallel loop
  for (int i = 0; i < 5; ++i) {
    []() {
#if USEE_BAD_CONTENT
      // funcBadContent-error@+4 {{static local variable 'x' is not permitted within function 'accLoopLambdaUseeDefinerAndUser()::(anonymous class)::operator()' because the latter is attributed with '#pragma acc routine'}}
      // funcBadContentExp-note@#accLoopLambdaUseeDefinerAndUser_lambdaUsee_lambdaUseeUsee_call {{'#pragma acc routine vector' implied for function 'accLoopLambdaUseeDefinerAndUser()::(anonymous class)::operator()' by its use of function 'lambdaUseeUsee' here}}
      // funcBadContentExp-note@#lambdaUseeUsee_routine {{'#pragma acc routine' for function 'lambdaUseeUsee' appears here}}
      // funcBadContentImp-note@#accLoopLambdaUseeDefinerAndUser_lambdaUsee_call {{'#pragma acc routine seq' implied for function 'accLoopLambdaUseeDefinerAndUser()::(anonymous class)::operator()' by its use in construct '#pragma acc parallel loop' here}}
      static int x;
      // funcBadContent-error@+4 {{'#pragma acc parallel' is not permitted within function 'accLoopLambdaUseeDefinerAndUser()::(anonymous class)::operator()' because the latter is attributed with '#pragma acc routine'}}
      // funcBadContentExp-note@#accLoopLambdaUseeDefinerAndUser_lambdaUsee_lambdaUseeUsee_call {{'#pragma acc routine vector' implied for function 'accLoopLambdaUseeDefinerAndUser()::(anonymous class)::operator()' by its use of function 'lambdaUseeUsee' here}}
      // funcBadContentExp-note@#lambdaUseeUsee_routine {{'#pragma acc routine' for function 'lambdaUseeUsee' appears here}}
      // funcBadContentImp-note@#accLoopLambdaUseeDefinerAndUser_lambdaUsee_call {{'#pragma acc routine seq' implied for function 'accLoopLambdaUseeDefinerAndUser()::(anonymous class)::operator()' by its use in construct '#pragma acc parallel loop' here}}
      #pragma acc parallel
      ;
#endif
      lambdaUseeUsee(); // #accLoopLambdaUseeDefinerAndUser_lambdaUsee_lambdaUseeUsee_call
    }(); // #accLoopLambdaUseeDefinerAndUser_lambdaUsee_call
  }
}
