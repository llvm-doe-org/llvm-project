// This file contains various uses of the functions declared in usee-decls.h.

#include "usee-decls.h"

#ifndef USER_ROUTINE_DIR
# error USER_ROUTINE_DIR not defined
# define USER_ROUTINE_DIR // to pacify editors
#endif
#ifndef USER_USER_ROUTINE_DIR
# error USER_USER_ROUTINE_DIR not defined
# define USER_USER_ROUTINE_DIR // to pacify editors
#endif
#ifndef USER_LOOP_DIR
# error USER_LOOP_DIR not defined
# define USER_LOOP_DIR // to pacify editors
#endif
#ifndef USER_LOOP_ROUTINE_DIR
# error USER_LOOP_ROUTINE_DIR not defined
# define USER_LOOP_ROUTINE_DIR // to pacify editors
#endif
#ifndef INCLUDE_LAMBDA_USES
# error INCLUDE_LAMBDA_USES not defined
# define INCLUDE_LAMBDA_USES // to pacify editors
#endif

struct Derived : MemberUsees {
  using MemberUsees::MemberUsees; // #Derived_ctorUsing
  Derived();
  Derived(CtorExplicitInit);
  ~Derived();
};

struct Composite_Base {
  Composite_Base();
  Composite_Base(CtorInherited);
};
struct Composite : Composite_Base {
  using Composite_Base::Composite_Base; // #Composite_ctorUsing
  MemberUsees var;
  Composite();
  Composite(CtorExplicitInit);
  ~Composite();
};

struct Diamond_A : virtual MemberUsees {
  Diamond_A();
  ~Diamond_A();
};
struct Diamond_B : virtual MemberUsees {
  Diamond_B();
  ~Diamond_B();
};
struct Diamond_C : Diamond_A, Diamond_B {
  Diamond_C();
  ~Diamond_C();
};

// callParLevel-error@#MemberUsees_ctorDelegation {{function 'MemberUsees::MemberUsees' has '#pragma acc routine worker' but calls function 'MemberUsees::MemberUsees', which has '#pragma acc routine gang'}}
// callParLevel-note@#MemberUsees_ctorDelegator_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
// callParLevel-note@#MemberUsees_ctor_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
//
// callParLevel-error@#MemberUsees_ctorDelegation {{function 'MemberUsees::MemberUsees' has '#pragma acc routine worker' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
// callParLevel-note@#MemberUsees_ctorDelegator_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
// callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
USER_ROUTINE_DIR // #MemberUsees_ctorDelegator_routine
MemberUsees::MemberUsees(CtorDelegator) : MemberUsees() {} // #MemberUsees_ctorDelegation

// callParLevel-error@#Derived_ctorImpInit {{function 'Derived::Derived' has '#pragma acc routine worker' but calls function 'MemberUsees::MemberUsees', which has '#pragma acc routine gang'}}
// callParLevel-note@#Derived_ctorImpInit_routine {{'#pragma acc routine' for function 'Derived::Derived' appears here}}
// callParLevel-note@#MemberUsees_ctor_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
//
// callParLevel-error@#Derived_ctorImpInit {{function 'Derived::Derived' has '#pragma acc routine worker' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
// callParLevel-note@#Derived_ctorImpInit_routine {{'#pragma acc routine' for function 'Derived::Derived' appears here}}
// callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
USER_ROUTINE_DIR // #Derived_ctorImpInit_routine
Derived::Derived() {}  // #Derived_ctorImpInit
// callParLevel-error@#Derived_ctorExpInit {{function 'Derived::Derived' has '#pragma acc routine worker' but calls function 'MemberUsees::MemberUsees', which has '#pragma acc routine gang'}}
// callParLevel-note@#Derived_ctorExpInit_routine {{'#pragma acc routine' for function 'Derived::Derived' appears here}}
// callParLevel-note@#MemberUsees_ctor_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
//
// callParLevel-error@#Derived_ctorExpInit {{function 'Derived::Derived' has '#pragma acc routine worker' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
// callParLevel-note@#Derived_ctorExpInit_routine {{'#pragma acc routine' for function 'Derived::Derived' appears here}}
// callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
USER_ROUTINE_DIR // #Derived_ctorExpInit_routine
Derived::Derived(CtorExplicitInit) : MemberUsees() {} // #Derived_ctorExpInit
// callParLevel-error@#Derived_dtor {{function 'Derived::~Derived' has '#pragma acc routine worker' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
// callParLevel-note@#Derived_dtor_routine {{'#pragma acc routine' for function 'Derived::~Derived' appears here}}
// callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
USER_ROUTINE_DIR // #Derived_dtor_routine
Derived::~Derived() {} // #Derived_dtor

// callParLevel-error@#Composite_ctorImpInit {{function 'Composite::Composite' has '#pragma acc routine worker' but calls function 'MemberUsees::MemberUsees', which has '#pragma acc routine gang'}}
// callParLevel-note@#Composite_ctorImpInit_routine {{'#pragma acc routine' for function 'Composite::Composite' appears here}}
// callParLevel-note@#MemberUsees_ctor_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
//
// callParLevel-error@#Composite_ctorImpInit {{function 'Composite::Composite' has '#pragma acc routine worker' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
// callParLevel-note@#Composite_ctorImpInit_routine {{'#pragma acc routine' for function 'Composite::Composite' appears here}}
// callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
USER_ROUTINE_DIR// #Composite_ctorImpInit_routine
Composite::Composite() {} // #Composite_ctorImpInit
// callParLevel-error@#Composite_ctorExpInit {{function 'Composite::Composite' has '#pragma acc routine worker' but calls function 'MemberUsees::MemberUsees', which has '#pragma acc routine gang'}}
// callParLevel-note@#Composite_ctorExpInit_routine {{'#pragma acc routine' for function 'Composite::Composite' appears here}}
// callParLevel-note@#MemberUsees_ctor_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
//
// callParLevel-error@#Composite_ctorExpInit {{function 'Composite::Composite' has '#pragma acc routine worker' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
// callParLevel-note@#Composite_ctorExpInit_routine {{'#pragma acc routine' for function 'Composite::Composite' appears here}}
// callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
USER_ROUTINE_DIR // #Composite_ctorExpInit_routine
Composite::Composite(CtorExplicitInit) : var() {} // #Composite_ctorExpInit
// callParLevel-error@#Composite_dtor {{function 'Composite::~Composite' has '#pragma acc routine worker' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
// callParLevel-note@#Composite_dtor_routine {{'#pragma acc routine' for function 'Composite::~Composite' appears here}}
// callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
USER_ROUTINE_DIR // #Composite_dtor_routine
Composite::~Composite() {} // #Composite_dtor

// callParLevel-error@#Diamond_A_ctor {{function 'Diamond_A::Diamond_A' has '#pragma acc routine worker' but calls function 'MemberUsees::MemberUsees', which has '#pragma acc routine gang'}}
// callParLevel-note@#Diamond_A_ctor_routine {{'#pragma acc routine' for function 'Diamond_A::Diamond_A' appears here}}
// callParLevel-note@#MemberUsees_ctor_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
//
// callParLevel-error@#Diamond_A_ctor {{function 'Diamond_A::Diamond_A' has '#pragma acc routine worker' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
// callParLevel-note@#Diamond_A_ctor_routine {{'#pragma acc routine' for function 'Diamond_A::Diamond_A' appears here}}
// callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
USER_ROUTINE_DIR // #Diamond_A_ctor_routine
Diamond_A::Diamond_A() {} // #Diamond_A_ctor
// callParLevel-error@#Diamond_A_dtor {{function 'Diamond_A::~Diamond_A' has '#pragma acc routine worker' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
// callParLevel-note@#Diamond_A_dtor_routine {{'#pragma acc routine' for function 'Diamond_A::~Diamond_A' appears here}}
// callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
USER_ROUTINE_DIR // #Diamond_A_dtor_routine
Diamond_A::~Diamond_A() {} // #Diamond_A_dtor

// callParLevel-error@#Diamond_B_ctor {{function 'Diamond_B::Diamond_B' has '#pragma acc routine worker' but calls function 'MemberUsees::MemberUsees', which has '#pragma acc routine gang'}}
// callParLevel-note@#Diamond_B_ctor_routine {{'#pragma acc routine' for function 'Diamond_B::Diamond_B' appears here}}
// callParLevel-note@#MemberUsees_ctor_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
//
// callParLevel-error@#Diamond_B_ctor {{function 'Diamond_B::Diamond_B' has '#pragma acc routine worker' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
// callParLevel-note@#Diamond_B_ctor_routine {{'#pragma acc routine' for function 'Diamond_B::Diamond_B' appears here}}
// callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
USER_ROUTINE_DIR // #Diamond_B_ctor_routine
Diamond_B::Diamond_B() {} // #Diamond_B_ctor
// callParLevel-error@#Diamond_B_dtor {{function 'Diamond_B::~Diamond_B' has '#pragma acc routine worker' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
// callParLevel-note@#Diamond_B_dtor_routine {{'#pragma acc routine' for function 'Diamond_B::~Diamond_B' appears here}}
// callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
USER_ROUTINE_DIR // #Diamond_B_dtor_routine
Diamond_B::~Diamond_B() {} // #Diamond_B_dtor

// callParLevel-error@#Diamond_C_ctor {{function 'Diamond_C::Diamond_C' has '#pragma acc routine vector' but calls function 'MemberUsees::MemberUsees', which has '#pragma acc routine gang'}}
// callParLevel-note@#Diamond_C_ctor_routine {{'#pragma acc routine' for function 'Diamond_C::Diamond_C' appears here}}
// callParLevel-note@#MemberUsees_ctor_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
//
// callParLevel-error@#Diamond_C_ctor {{function 'Diamond_C::Diamond_C' has '#pragma acc routine vector' but calls function 'Diamond_A::Diamond_A', which has '#pragma acc routine worker'}}
// callParLevel-note@#Diamond_C_ctor_routine {{'#pragma acc routine' for function 'Diamond_C::Diamond_C' appears here}}
// callParLevel-note@#Diamond_A_ctor_routine {{'#pragma acc routine' for function 'Diamond_A::Diamond_A' appears here}}
//
// callParLevel-error@#Diamond_C_ctor {{function 'Diamond_C::Diamond_C' has '#pragma acc routine vector' but calls function 'Diamond_B::Diamond_B', which has '#pragma acc routine worker'}}
// callParLevel-note@#Diamond_C_ctor_routine {{'#pragma acc routine' for function 'Diamond_C::Diamond_C' appears here}}
// callParLevel-note@#Diamond_B_ctor_routine {{'#pragma acc routine' for function 'Diamond_B::Diamond_B' appears here}}
//
// callParLevel-error@#Diamond_C_ctor {{function 'Diamond_C::Diamond_C' has '#pragma acc routine vector' but calls function 'Diamond_A::~Diamond_A', which has '#pragma acc routine worker'}}
// callParLevel-note@#Diamond_C_ctor_routine {{'#pragma acc routine' for function 'Diamond_C::Diamond_C' appears here}}
// callParLevel-note@#Diamond_A_dtor_routine {{'#pragma acc routine' for function 'Diamond_A::~Diamond_A' appears here}}
//
// callParLevel-error@#Diamond_C_ctor {{function 'Diamond_C::Diamond_C' has '#pragma acc routine vector' but calls function 'Diamond_B::~Diamond_B', which has '#pragma acc routine worker'}}
// callParLevel-note@#Diamond_C_ctor_routine {{'#pragma acc routine' for function 'Diamond_C::Diamond_C' appears here}}
// callParLevel-note@#Diamond_B_dtor_routine {{'#pragma acc routine' for function 'Diamond_B::~Diamond_B' appears here}}
//
// callParLevel-error@#Diamond_C_ctor {{function 'Diamond_C::Diamond_C' has '#pragma acc routine vector' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
// callParLevel-note@#Diamond_C_ctor_routine {{'#pragma acc routine' for function 'Diamond_C::Diamond_C' appears here}}
// callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
USER_USER_ROUTINE_DIR // #Diamond_C_ctor_routine
Diamond_C::Diamond_C() {} // #Diamond_C_ctor
// callParLevel-error@#Diamond_C_dtor {{function 'Diamond_C::~Diamond_C' has '#pragma acc routine vector' but calls function 'Diamond_A::~Diamond_A', which has '#pragma acc routine worker'}}
// callParLevel-note@#Diamond_C_dtor_routine {{'#pragma acc routine' for function 'Diamond_C::~Diamond_C' appears here}}
// callParLevel-note@#Diamond_A_dtor_routine {{'#pragma acc routine' for function 'Diamond_A::~Diamond_A' appears here}}
//
// callParLevel-error@#Diamond_C_dtor {{function 'Diamond_C::~Diamond_C' has '#pragma acc routine vector' but calls function 'Diamond_B::~Diamond_B', which has '#pragma acc routine worker'}}
// callParLevel-note@#Diamond_C_dtor_routine {{'#pragma acc routine' for function 'Diamond_C::~Diamond_C' appears here}}
// callParLevel-note@#Diamond_B_dtor_routine {{'#pragma acc routine' for function 'Diamond_B::~Diamond_B' appears here}}
//
// callParLevel-error@#Diamond_C_dtor {{function 'Diamond_C::~Diamond_C' has '#pragma acc routine vector' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
// callParLevel-note@#Diamond_C_dtor_routine {{'#pragma acc routine' for function 'Diamond_C::~Diamond_C' appears here}}
// callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
USER_USER_ROUTINE_DIR // #Diamond_C_dtor_routine
Diamond_C::~Diamond_C() {} // #Diamond_C_dtor

// Because this routine directive is attached to the definition (not an earlier
// prototype) of passByVal, Clang must be careful to attach the
// ACCRoutineDeclAttr to passByVal before checking destructor (or other
// function) calls caused by the parameters.
//
// callParLevel-error@#passByVal_def {{function 'passByVal' has '#pragma acc routine worker' but calls function 'MemberUseesTrivial::~MemberUseesTrivial', which has '#pragma acc routine gang'}}
// callParLevel-note@#passByVal_routine {{'#pragma acc routine' for function 'passByVal' appears here}}
// callParLevel-note@#MemberUseesTrivial_dtor_routine {{'#pragma acc routine' for function 'MemberUseesTrivial::~MemberUseesTrivial' appears here}}
USER_ROUTINE_DIR // #passByVal_routine
int passByVal(MemberUsees, MemberUseesTrivial) { return 0; } // #passByVal_def

USER_ROUTINE_DIR // #accRoutineUse_routine
void accRoutineUse(int cond) {
  // callParLevel-error@#accRoutineUse_MemberUsees_var {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUsees::MemberUsees', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUsees_ctor_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
  //
  // callParLevel-error@#accRoutineUse_MemberUsees_var {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
  MemberUsees var; // #accRoutineUse_MemberUsees_var
  // callParLevel-error@#accRoutineUse_MemberUsees_tmp {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUsees::MemberUsees', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUsees_ctor_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
  //
  // callParLevel-error@#accRoutineUse_MemberUsees_tmp {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
  MemberUsees(); // #accRoutineUse_MemberUsees_tmp
  decltype(MemberUsees()) *tmpUneval; // not a use
  // callParLevel-error@#accRoutineUse_MemberUsees_new_call {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUsees::MemberUsees', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUsees_ctor_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
  //
  // callParLevel-error@#accRoutineUse_MemberUsees_new_call {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUsees::operator new', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUsees_new_routine {{'#pragma acc routine' for function 'MemberUsees::operator new' appears here}}
  //
  // callParLevel-error@#accRoutineUse_MemberUsees_new_call {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUsees::operator delete', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUsees_delete_routine {{'#pragma acc routine' for function 'MemberUsees::operator delete' appears here}}
  MemberUsees *varPtr = new MemberUsees(); // #accRoutineUse_MemberUsees_new_call
  decltype(new MemberUsees()) varPtrUneval; // not a use
  // callParLevel-error@#accRoutineUse_MemberUsees_throw {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
  //
  // callParLevel-error@#accRoutineUse_MemberUsees_throw {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUsees::MemberUsees', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUsees_ctorCopy_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
  if (cond)
    throw var; // #accRoutineUse_MemberUsees_throw
  // callParLevel-error@#accRoutineUse_MemberUseesTrivial_var {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUseesTrivial::MemberUseesTrivial', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUseesTrivial_ctor_routine {{'#pragma acc routine' for function 'MemberUseesTrivial::MemberUseesTrivial' appears here}}
  //
  // callParLevel-error@#accRoutineUse_MemberUseesTrivial_var {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUseesTrivial::~MemberUseesTrivial', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUseesTrivial_dtor_routine {{'#pragma acc routine' for function 'MemberUseesTrivial::~MemberUseesTrivial' appears here}}
  MemberUseesTrivial varTrivial; // #accRoutineUse_MemberUseesTrivial_var
  // callParLevel-error@#accRoutineUse_passByVal_call {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUsees::MemberUsees', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUsees_ctorCopy_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
  //
  // callParLevel-error@#accRoutineUse_passByVal_call{{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
  //
  // callParLevel-error@#accRoutineUse_passByVal_call{{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUseesTrivial::MemberUseesTrivial', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUseesTrivial_ctorCopy_routine {{'#pragma acc routine' for function 'MemberUseesTrivial::MemberUseesTrivial' appears here}}
  //
  // FIXME: See clang::trivial_abi FIXME in routine-cxx-funcs/README.md.
  // callParLevel-error@#accRoutineUse_passByVal_call{{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUseesTrivial::~MemberUseesTrivial', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUseesTrivial_dtor_routine {{'#pragma acc routine' for function 'MemberUseesTrivial::~MemberUseesTrivial' appears here}}
  passByVal(var, varTrivial); // #accRoutineUse_passByVal_call
  decltype(passByVal(var, varTrivial)) varPassByValUneval;
  // callParLevel-error@#accRoutineUse_MemberUsees_delete_call {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
  //
  // callParLevel-error@#accRoutineUse_MemberUsees_delete_call {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUsees::operator delete', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUsees_delete_routine {{'#pragma acc routine' for function 'MemberUsees::operator delete' appears here}}
  delete varPtr; // #accRoutineUse_MemberUsees_delete_call
  // callParLevel-error@#accRoutineUse_MemberUsees_dtor_call {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
  varPtr->~MemberUsees(); // #accRoutineUse_MemberUsees_dtor_call
  // callParLevel-error@#accRoutineUse_MemberUsees_opUnary_call {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUsees::operator-', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUsees_opUnary_routine {{'#pragma acc routine' for function 'MemberUsees::operator-' appears here}}
  -var; // #accRoutineUse_MemberUsees_opUnary_call
  decltype(-var) varOpUnaryUneval;
  // callParLevel-error@#accRoutineUse_MemberUsees_opBinary_call {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUsees::operator+', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUsees_opBinary_routine {{'#pragma acc routine' for function 'MemberUsees::operator+' appears here}}
  var + 3; // #accRoutineUse_MemberUsees_opBinary_call
  decltype(var + 3) varOpBinaryUneval;
  // callParLevel-error@#accRoutineUse_MemberUsees_opSubscript_call {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUsees::operator[]', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUsees_opSubscript_routine {{'#pragma acc routine' for function 'MemberUsees::operator[]' appears here}}
  var[3]; // #accRoutineUse_MemberUsees_opSubscript_call
  decltype(var[3]) varOpSubscriptUneval;
  // callParLevel-error@#accRoutineUse_MemberUsees_opCall_call {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUsees::operator()', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUsees_opCall_routine {{'#pragma acc routine' for function 'MemberUsees::operator()' appears here}}
  var(); // #accRoutineUse_MemberUsees_opCall_call
  decltype(var()) varOpCallUneval;
  // callParLevel-error@#accRoutineUse_MemberUsees_opArrow_call {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUsees::operator->', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUsees_opArrow_routine {{'#pragma acc routine' for function 'MemberUsees::operator->' appears here}}
  //
  // callParLevel-error@#accRoutineUse_MemberUsees_opArrow_call {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUsees::ArrowResult::operator->', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUsees_ArrowResult_opArrow_routine {{'#pragma acc routine' for function 'MemberUsees::ArrowResult::operator->' appears here}}
  (void)var->x; // #accRoutineUse_MemberUsees_opArrow_call
  decltype(var->x) varOpArrowUneval;
  // callParLevel-error@#accRoutineUse_MemberUsees_opConvert_callImp {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUsees::operator int', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUsees_opConvert_routine {{'#pragma acc routine' for function 'MemberUsees::operator int' appears here}}
  int varConvImp = var; // #accRoutineUse_MemberUsees_opConvert_callImp
  // callParLevel-error@#accRoutineUse_MemberUsees_opConvert_callExp {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUsees::operator int', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUsees_opConvert_routine {{'#pragma acc routine' for function 'MemberUsees::operator int' appears here}}
  int varConvExp = (int)var; // #accRoutineUse_MemberUsees_opConvert_callExp
  decltype((int)var) varOpConvertUneval;
  // callParLevel-error@#accRoutineUse_MemberUsees_opConvertSurrogate_call {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUsees::operator int (*)(int)', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUsees_opConvertSurrogate_routine {{'#pragma acc routine' for function 'MemberUsees::operator int (*)(int)' appears here}}
  var(3); // #accRoutineUse_MemberUsees_opConvertSurrogate_call
  decltype(var(3)) varOpConvertSurrogateUneval;
  // callParLevel-error@#accRoutineUse_MemberUsees_rangeBasedLoop {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUsees::begin', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUsees_begin_routine {{'#pragma acc routine' for function 'MemberUsees::begin' appears here}}
  //
  // callParLevel-error@#accRoutineUse_MemberUsees_rangeBasedLoop {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function 'MemberUsees::end', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#MemberUsees_end_routine {{'#pragma acc routine' for function 'MemberUsees::end' appears here}}
  for (int i : var) // #accRoutineUse_MemberUsees_rangeBasedLoop
    ;
  // callParLevel-error@#accRoutineUse_lambdaUsee_call {{function 'accRoutineUse' has '#pragma acc routine worker' but calls function '(anonymous class)::operator()', which has '#pragma acc routine gang'}}
  // callParLevel-note@#accRoutineUse_routine {{'#pragma acc routine' for function 'accRoutineUse' appears here}}
  // callParLevel-note@#lambdaUsee_lambdaUseeUsee_call {{'#pragma acc routine gang' implied for function '(anonymous class)::operator()' by its use of function 'lambdaUseeUsee' here}}
  // callParLevel-note@#lambdaUseeUsee_routine {{'#pragma acc routine' for function 'lambdaUseeUsee' appears here}}
#if INCLUDE_LAMBDA_USES
  lambdaUsee(); // #accRoutineUse_lambdaUsee_call
#endif
}

void accParallelUse(int cond) {
  #pragma acc parallel // #accParallelUse_accParallel
  for (int i = 0; i < 5; ++i) {
    MemberUsees var; // #accParallelUse_MemberUsees_var
    MemberUsees(); // #accParallelUse_MemberUsees_tmp
    decltype(MemberUsees()) *tmpUneval; // not a use
    MemberUsees *varPtr = new MemberUsees(); // #accParallelUse_MemberUsees_new_call
    decltype(new MemberUsees()) varPtrUneval; // not a use
    if (cond)
      throw var; // #accParallelUse_MemberUsees_throw
    MemberUseesTrivial varTrivial; // #accParallelUse_MemberUseesTrivial_var
    passByVal(var, varTrivial); // #accParallelUse_passByVal_call
    decltype(passByVal(var, varTrivial)) varPassByValUneval;
    delete varPtr; // #accParallelUse_MemberUsees_delete_call
    varPtr->~MemberUsees(); // #accParallelUse_MemberUsees_dtor_call
    -var; // #accParallelUse_MemberUsees_opUnary_call
    decltype(-var) varOpUnaryUneval;
    var + 3; // #accParallelUse_MemberUsees_opBinary_call
    decltype(var + 3) varOpBinaryUneval;
    var[3]; // #accParallelUse_MemberUsees_opSubscript_call
    decltype(var[3]) varOpSubscriptUneval;
    var(); // #accParallelUse_MemberUsees_opCall_call
    decltype(var()) varOpCallUneval;
    (void)var->x; // #accParallelUse_MemberUsees_opArrow_call
    decltype(var->x) varOpArrowUneval;
    int varConvImp = var; // #accParallelUse_MemberUsees_opConvert_callImp
    int varConvExp = (int)var; // #accParallelUse_MemberUsees_opConvert_callExp
    decltype((int)var) varOpConvertUneval;
    var(3); // #accParallelUse_MemberUsees_opConvertSurrogate_call
    decltype(var(3)) varOpConvertSurrogateUneval;
    for (int i : var) // #accParallelUse_MemberUsees_rangeBasedLoop
      ;
#if INCLUDE_LAMBDA_USES
    lambdaUsee();
#endif
  }
}

USER_LOOP_ROUTINE_DIR
void accLoopUse(int cond) {
  USER_LOOP_DIR // #accLoopUse_accLoop
  for (int i = 0; i < 5; ++i) {
    // callParLevel-error@#accLoopUse_MemberUsees_var {{'#pragma acc loop worker' construct calls function 'MemberUsees::MemberUsees', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUsees_ctor_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
    //
    // callParLevel-error@#accLoopUse_MemberUsees_var {{'#pragma acc loop worker' construct calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
    MemberUsees var; // #accLoopUse_MemberUsees_var
    // callParLevel-error@#accLoopUse_MemberUsees_tmp {{'#pragma acc loop worker' construct calls function 'MemberUsees::MemberUsees', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUsees_ctor_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
    //
    // callParLevel-error@#accLoopUse_MemberUsees_tmp {{'#pragma acc loop worker' construct calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
    MemberUsees(); // #accLoopUse_MemberUsees_tmp
    decltype(MemberUsees()) *tmpUneval; // not a use
    // callParLevel-error@#accLoopUse_MemberUsees_new_call {{'#pragma acc loop worker' construct calls function 'MemberUsees::MemberUsees', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUsees_ctor_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
    //
    // callParLevel-error@#accLoopUse_MemberUsees_new_call {{'#pragma acc loop worker' construct calls function 'MemberUsees::operator new', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUsees_new_routine {{'#pragma acc routine' for function 'MemberUsees::operator new' appears here}}
    //
    // callParLevel-error@#accLoopUse_MemberUsees_new_call {{'#pragma acc loop worker' construct calls function 'MemberUsees::operator delete', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUsees_delete_routine {{'#pragma acc routine' for function 'MemberUsees::operator delete' appears here}}
    MemberUsees *varPtr = new MemberUsees(); // #accLoopUse_MemberUsees_new_call
    decltype(new MemberUsees()) varPtrUneval; // not a use
    // callParLevel-error@#accLoopUse_MemberUsees_throw {{'#pragma acc loop worker' construct calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
    //
    // callParLevel-error@#accLoopUse_MemberUsees_throw {{'#pragma acc loop worker' construct calls function 'MemberUsees::MemberUsees', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUsees_ctorCopy_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
    if (cond)
      throw var; // #accLoopUse_MemberUsees_throw
    // callParLevel-error@#accLoopUse_MemberUseesTrivial_var {{'#pragma acc loop worker' construct calls function 'MemberUseesTrivial::MemberUseesTrivial', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUseesTrivial_ctor_routine {{'#pragma acc routine' for function 'MemberUseesTrivial::MemberUseesTrivial' appears here}}
    //
    // callParLevel-error@#accLoopUse_MemberUseesTrivial_var {{'#pragma acc loop worker' construct calls function 'MemberUseesTrivial::~MemberUseesTrivial', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUseesTrivial_dtor_routine {{'#pragma acc routine' for function 'MemberUseesTrivial::~MemberUseesTrivial' appears here}}
    MemberUseesTrivial varTrivial; // #accLoopUse_MemberUseesTrivial_var
    // callParLevel-error@#accLoopUse_passByVal_call {{'#pragma acc loop worker' construct calls function 'MemberUsees::MemberUsees', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUsees_ctorCopy_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
    //
    // callParLevel-error@#accLoopUse_passByVal_call{{'#pragma acc loop worker' construct calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
    //
    // callParLevel-error@#accLoopUse_passByVal_call{{'#pragma acc loop worker' construct calls function 'MemberUseesTrivial::MemberUseesTrivial', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUseesTrivial_ctorCopy_routine {{'#pragma acc routine' for function 'MemberUseesTrivial::MemberUseesTrivial' appears here}}
    //
    // FIXME: See clang::trivial_abi FIXME in routine-cxx-funcs/README.md.
    // callParLevel-error@#accLoopUse_passByVal_call{{'#pragma acc loop worker' construct calls function 'MemberUseesTrivial::~MemberUseesTrivial', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUseesTrivial_dtor_routine {{'#pragma acc routine' for function 'MemberUseesTrivial::~MemberUseesTrivial' appears here}}
    // 
    // callParLevel-error@#accLoopUse_passByVal_call {{'#pragma acc loop worker' construct calls function 'passByVal', which has '#pragma acc routine worker}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#passByVal_routine {{'#pragma acc routine' for function 'passByVal' appears here}}
    passByVal(var, varTrivial); // #accLoopUse_passByVal_call
    decltype(passByVal(var, varTrivial)) varPassByValUneval;
    // callParLevel-error@#accLoopUse_MemberUsees_delete_call {{'#pragma acc loop worker' construct calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
    //
    // callParLevel-error@#accLoopUse_MemberUsees_delete_call {{'#pragma acc loop worker' construct calls function 'MemberUsees::operator delete', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUsees_delete_routine {{'#pragma acc routine' for function 'MemberUsees::operator delete' appears here}}
    delete varPtr; // #accLoopUse_MemberUsees_delete_call
    // callParLevel-error@#accLoopUse_MemberUsees_dtor_call {{'#pragma acc loop worker' construct calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
    varPtr->~MemberUsees(); // #accLoopUse_MemberUsees_dtor_call
    // callParLevel-error@#accLoopUse_MemberUsees_opUnary_call {{'#pragma acc loop worker' construct calls function 'MemberUsees::operator-', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUsees_opUnary_routine {{'#pragma acc routine' for function 'MemberUsees::operator-' appears here}}
    -var; // #accLoopUse_MemberUsees_opUnary_call
    decltype(-var) varOpUnaryUneval;
    // callParLevel-error@#accLoopUse_MemberUsees_opBinary_call {{'#pragma acc loop worker' construct calls function 'MemberUsees::operator+', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUsees_opBinary_routine {{'#pragma acc routine' for function 'MemberUsees::operator+' appears here}}
    var + 3; // #accLoopUse_MemberUsees_opBinary_call
    decltype(var + 3) varOpBinaryUneval;
    // callParLevel-error@#accLoopUse_MemberUsees_opSubscript_call {{'#pragma acc loop worker' construct calls function 'MemberUsees::operator[]', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUsees_opSubscript_routine {{'#pragma acc routine' for function 'MemberUsees::operator[]' appears here}}
    var[3]; // #accLoopUse_MemberUsees_opSubscript_call
    decltype(var[3]) varOpSubscriptUneval;
    // callParLevel-error@#accLoopUse_MemberUsees_opCall_call {{'#pragma acc loop worker' construct calls function 'MemberUsees::operator()', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUsees_opCall_routine {{'#pragma acc routine' for function 'MemberUsees::operator()' appears here}}
    var(); // #accLoopUse_MemberUsees_opCall_call
    decltype(var()) varOpCallUneval;
    // callParLevel-error@#accLoopUse_MemberUsees_opArrow_call {{'#pragma acc loop worker' construct calls function 'MemberUsees::operator->', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUsees_opArrow_routine {{'#pragma acc routine' for function 'MemberUsees::operator->' appears here}}
    //
    // callParLevel-error@#accLoopUse_MemberUsees_opArrow_call {{'#pragma acc loop worker' construct calls function 'MemberUsees::ArrowResult::operator->', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUsees_ArrowResult_opArrow_routine {{'#pragma acc routine' for function 'MemberUsees::ArrowResult::operator->' appears here}}
    (void)var->x; // #accLoopUse_MemberUsees_opArrow_call
    decltype(var->x) varOpArrowUneval;
    // callParLevel-error@#accLoopUse_MemberUsees_opConvert_callImp {{'#pragma acc loop worker' construct calls function 'MemberUsees::operator int', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUsees_opConvert_routine {{'#pragma acc routine' for function 'MemberUsees::operator int' appears here}}
    int varConvImp = var; // #accLoopUse_MemberUsees_opConvert_callImp
    // callParLevel-error@#accLoopUse_MemberUsees_opConvert_callExp {{'#pragma acc loop worker' construct calls function 'MemberUsees::operator int', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUsees_opConvert_routine {{'#pragma acc routine' for function 'MemberUsees::operator int' appears here}}
    int varConvExp = (int)var; // #accLoopUse_MemberUsees_opConvert_callExp
    decltype((int)var) varOpConvertUneval;
    // callParLevel-error@#accLoopUse_MemberUsees_opConvertSurrogate_call {{'#pragma acc loop worker' construct calls function 'MemberUsees::operator int (*)(int)', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUsees_opConvertSurrogate_routine {{'#pragma acc routine' for function 'MemberUsees::operator int (*)(int)' appears here}}
    var(3); // #accLoopUse_MemberUsees_opConvertSurrogate_call
    decltype(var(3)) varOpConvertSurrogateUneval;
    // callParLevel-error@#accLoopUse_MemberUsees_rangeBasedLoop {{'#pragma acc loop worker' construct calls function 'MemberUsees::begin', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUsees_begin_routine {{'#pragma acc routine' for function 'MemberUsees::begin' appears here}}
    //
    // callParLevel-error@#accLoopUse_MemberUsees_rangeBasedLoop {{'#pragma acc loop worker' construct calls function 'MemberUsees::end', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#MemberUsees_end_routine {{'#pragma acc routine' for function 'MemberUsees::end' appears here}}
    for (int i : var) // #accLoopUse_MemberUsees_rangeBasedLoop
      ;
    // callParLevel-error@#accLoopUse_lambdaUsee_call {{'#pragma acc loop worker' construct calls function '(anonymous class)::operator()', which has '#pragma acc routine gang'}}
    // callParLevel-note@#accLoopUse_accLoop {{enclosing '#pragma acc loop' here}}
    // callParLevel-note@#lambdaUsee_lambdaUseeUsee_call {{'#pragma acc routine gang' implied for function '(anonymous class)::operator()' by its use of function 'lambdaUseeUsee' here}}
    // callParLevel-note@#lambdaUseeUsee_routine {{'#pragma acc routine' for function 'lambdaUseeUsee' appears here}}
#if INCLUDE_LAMBDA_USES
    lambdaUsee(); // #accLoopUse_lambdaUsee_call
#endif
  }
}

void hostUse(int cond) {
  // callParLevel-error@#hostUse_MemberUsees_var {{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUsees::MemberUsees', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUsees_ctor_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
  //
  // callParLevel-error@#hostUse_MemberUsees_var {{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
  MemberUsees var; // #hostUse_MemberUsees_var
  // callParLevel-error@#hostUse_MemberUsees_tmp {{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUsees::MemberUsees', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUsees_ctor_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
  //
  // callParLevel-error@#hostUse_MemberUsees_tmp {{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
  MemberUsees(); // #hostUse_MemberUsees_tmp
  decltype(MemberUsees()) *tmpUneval; // not a use
  // callParLevel-error@#hostUse_MemberUsees_new_call {{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUsees::MemberUsees', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUsees_ctor_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
  //
  // callParLevel-error@#hostUse_MemberUsees_new_call {{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUsees::operator new', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUsees_new_routine {{'#pragma acc routine' for function 'MemberUsees::operator new' appears here}}
  //
  // callParLevel-error@#hostUse_MemberUsees_new_call {{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUsees::operator delete', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUsees_delete_routine {{'#pragma acc routine' for function 'MemberUsees::operator delete' appears here}}
  MemberUsees *varPtr = new MemberUsees(); // #hostUse_MemberUsees_new_call
  decltype(new MemberUsees()) varPtrUneval; // not a use
  // callParLevel-error@#hostUse_MemberUsees_throw {{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
  //
  // callParLevel-error@#hostUse_MemberUsees_throw {{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUsees::MemberUsees', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUsees_ctorCopy_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
  if (cond)
    throw var; // #hostUse_MemberUsees_throw
  // callParLevel-error@#hostUse_MemberUseesTrivial_var {{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUseesTrivial::MemberUseesTrivial', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUseesTrivial_ctor_routine {{'#pragma acc routine' for function 'MemberUseesTrivial::MemberUseesTrivial' appears here}}
  //
  // callParLevel-error@#hostUse_MemberUseesTrivial_var {{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUseesTrivial::~MemberUseesTrivial', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUseesTrivial_dtor_routine {{'#pragma acc routine' for function 'MemberUseesTrivial::~MemberUseesTrivial' appears here}}
  MemberUseesTrivial varTrivial; // #hostUse_MemberUseesTrivial_var
  // callParLevel-error@#hostUse_passByVal_call {{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUsees::MemberUsees', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUsees_ctorCopy_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
  //
  // callParLevel-error@#hostUse_passByVal_call{{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
  //
  // callParLevel-error@#hostUse_passByVal_call{{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUseesTrivial::MemberUseesTrivial', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUseesTrivial_ctorCopy_routine {{'#pragma acc routine' for function 'MemberUseesTrivial::MemberUseesTrivial' appears here}}
  //
  // FIXME: See clang::trivial_abi FIXME in routine-cxx-funcs/README.md.
  // callParLevel-error@#hostUse_passByVal_call{{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUseesTrivial::~MemberUseesTrivial', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUseesTrivial_dtor_routine {{'#pragma acc routine' for function 'MemberUseesTrivial::~MemberUseesTrivial' appears here}}
  // 
  // callParLevel-error@#hostUse_passByVal_call{{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'passByVal', which has '#pragma acc routine worker'}}
  // callParLevel-note@#passByVal_routine {{'#pragma acc routine' for function 'passByVal' appears here}}
  passByVal(var, varTrivial); // #hostUse_passByVal_call
  decltype(passByVal(var, varTrivial)) varPassByValUneval;
  // callParLevel-error@#hostUse_MemberUsees_delete_call {{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
  //
  // callParLevel-error@#hostUse_MemberUsees_delete_call {{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUsees::operator delete', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUsees_delete_routine {{'#pragma acc routine' for function 'MemberUsees::operator delete' appears here}}
  delete varPtr; // #hostUse_MemberUsees_delete_call
  // callParLevel-error@#hostUse_MemberUsees_dtor_call {{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
  varPtr->~MemberUsees(); // #hostUse_MemberUsees_dtor_call
  // callParLevel-error@#hostUse_MemberUsees_opUnary_call {{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUsees::operator-', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUsees_opUnary_routine {{'#pragma acc routine' for function 'MemberUsees::operator-' appears here}}
  -var; // #hostUse_MemberUsees_opUnary_call
  decltype(-var) varOpUnaryUneval;
  // callParLevel-error@#hostUse_MemberUsees_opBinary_call {{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUsees::operator+', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUsees_opBinary_routine {{'#pragma acc routine' for function 'MemberUsees::operator+' appears here}}
  var + 3; // #hostUse_MemberUsees_opBinary_call
  decltype(var + 3) varOpBinaryUneval;
  // callParLevel-error@#hostUse_MemberUsees_opSubscript_call {{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUsees::operator[]', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUsees_opSubscript_routine {{'#pragma acc routine' for function 'MemberUsees::operator[]' appears here}}
  var[3]; // #hostUse_MemberUsees_opSubscript_call
  decltype(var[3]) varOpSubscriptUneval;
  // callParLevel-error@#hostUse_MemberUsees_opCall_call {{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUsees::operator()', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUsees_opCall_routine {{'#pragma acc routine' for function 'MemberUsees::operator()' appears here}}
  var(); // #hostUse_MemberUsees_opCall_call
  decltype(var()) varOpCallUneval;
  // callParLevel-error@#hostUse_MemberUsees_opArrow_call {{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUsees::operator->', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUsees_opArrow_routine {{'#pragma acc routine' for function 'MemberUsees::operator->' appears here}}
  //
  // callParLevel-error@#hostUse_MemberUsees_opArrow_call {{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUsees::ArrowResult::operator->', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUsees_ArrowResult_opArrow_routine {{'#pragma acc routine' for function 'MemberUsees::ArrowResult::operator->' appears here}}
  (void)var->x; // #hostUse_MemberUsees_opArrow_call
  decltype(var->x) varOpArrowUneval;
  // callParLevel-error@#hostUse_MemberUsees_opConvert_callImp {{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUsees::operator int', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUsees_opConvert_routine {{'#pragma acc routine' for function 'MemberUsees::operator int' appears here}}
  int varConvImp = var; // #hostUse_MemberUsees_opConvert_callImp
  // callParLevel-error@#hostUse_MemberUsees_opConvert_callExp {{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUsees::operator int', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUsees_opConvert_routine {{'#pragma acc routine' for function 'MemberUsees::operator int' appears here}}
  int varConvExp = (int)var; // #hostUse_MemberUsees_opConvert_callExp
  decltype((int)var) varOpConvertUneval;
  // callParLevel-error@#hostUse_MemberUsees_opConvertSurrogate_call {{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUsees::operator int (*)(int)', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUsees_opConvertSurrogate_routine {{'#pragma acc routine' for function 'MemberUsees::operator int (*)(int)' appears here}}
  var(3); // #hostUse_MemberUsees_opConvertSurrogate_call
  decltype(var(3)) varOpConvertSurrogateUneval;
  // callParLevel-error@#hostUse_MemberUsees_rangeBasedLoop {{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUsees::begin', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUsees_begin_routine {{'#pragma acc routine' for function 'MemberUsees::begin' appears here}}
  //
  // callParLevel-error@#hostUse_MemberUsees_rangeBasedLoop {{function 'hostUse' has no explicit '#pragma acc routine' but calls function 'MemberUsees::end', which has '#pragma acc routine gang'}}
  // callParLevel-note@#MemberUsees_end_routine {{'#pragma acc routine' for function 'MemberUsees::end' appears here}}
  for (int i : var) // #hostUse_MemberUsees_rangeBasedLoop
    ;
  // callParLevel-error@#hostUse_lambdaUsee_call {{function 'hostUse' has no explicit '#pragma acc routine' but calls function '(anonymous class)::operator()', which has '#pragma acc routine gang'}}
  // callParLevel-note@#lambdaUsee_lambdaUseeUsee_call {{'#pragma acc routine gang' implied for function '(anonymous class)::operator()' by its use of function 'lambdaUseeUsee' here}}
  // callParLevel-note@#lambdaUseeUsee_routine {{'#pragma acc routine' for function 'lambdaUseeUsee' appears here}}
#if INCLUDE_LAMBDA_USES
  lambdaUsee(); // #hostUse_lambdaUsee_call
#endif
}


// callParLevel-error@#fileScope_MemberUsees_var {{function 'MemberUsees::MemberUsees' has '#pragma acc routine gang' but is called at file scope}}
// callParLevel-note@#MemberUsees_ctor_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
//
// callParLevel-error@#fileScope_MemberUsees_var {{function 'MemberUsees::~MemberUsees' has '#pragma acc routine gang' but is called at file scope}}
// callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
MemberUsees varFileScope; // #fileScope_MemberUsees_var

// Trigger Clang to instantiate inheriting constructors, for which we have no
// way to provide explicit routine directives (declaring them explicitly would
// mean they would not be inheriting constructors, as declared by the using
// declaration).
//
// FIXME: See inherited ctor FIXME in routine-cxx-funcs/README.md.
//
// FIXME: If we move this after the routine directive, early-uses.cpp loses
// diagnostics because Clang thinks the use came after routine directive.
// Should we test that?
//
// callParLevel-error@#Derived_ctorUsing {{function 'Derived::MemberUsees' has no explicit '#pragma acc routine' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
// callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
//
// callParLevel-error@#Derived_ctorInherited_use {{function 'Derived::MemberUsees' has no explicit '#pragma acc routine' but calls function 'MemberUsees::MemberUsees', which has '#pragma acc routine gang'}}
// callParLevel-note@#MemberUsees_ctorInherited_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
//
// callParLevel-error@#Derived_ctorInherited_use {{function 'Derived::~Derived' has '#pragma acc routine worker' but is called at file scope}}
// callParLevel-note@#Derived_dtor_routine {{'#pragma acc routine' for function 'Derived::~Derived' appears here}}
Derived Derived_var(CtorInherited{}); // #Derived_ctorInherited_use
// callParLevel-error@#Composite_ctorUsing {{function 'Composite::Composite_Base' has no explicit '#pragma acc routine' but calls function 'MemberUsees::MemberUsees', which has '#pragma acc routine gang'}}
// callParLevel-note@#MemberUsees_ctor_routine {{'#pragma acc routine' for function 'MemberUsees::MemberUsees' appears here}}
//
// callParLevel-error@#Composite_ctorUsing {{function 'Composite::Composite_Base' has no explicit '#pragma acc routine' but calls function 'MemberUsees::~MemberUsees', which has '#pragma acc routine gang'}}
// callParLevel-note@#MemberUsees_dtor_routine {{'#pragma acc routine' for function 'MemberUsees::~MemberUsees' appears here}}
//
// callParLevel-error@#Composite_ctorInherited_use {{function 'Composite::~Composite' has '#pragma acc routine worker' but is called at file scope}}
// callParLevel-note@#Composite_dtor_routine {{'#pragma acc routine' for function 'Composite::~Composite' appears here}}
Composite Composite_var(CtorInherited{}); // #Composite_ctorInherited_use

// This is a caller and a callee.
USER_ROUTINE_DIR // #templateFunc_routine
template <void Func()> void templateFunc() {
  Func(); // #templateFunc_callFunc
}

// callParLevel-error@#templateFunc_call {{function 'templateFunc_user' has '#pragma acc routine vector' but calls function 'templateFunc<&fnUsee>', which has '#pragma acc routine worker'}}
// callParLevel-note@#templateFunc_user_routine {{'#pragma acc routine' for function 'templateFunc_user' appears here}}
// callParLevel-note@#templateFunc_routine {{'#pragma acc routine' for function 'templateFunc<&fnUsee>' appears here}}
//
// callParLevel-error@#templateFunc_callFunc {{function 'templateFunc<&fnUsee>' has '#pragma acc routine worker' but calls function 'fnUsee', which has '#pragma acc routine gang'}}
// callParLevel-note@#templateFunc_call{{in instantiation of function template specialization 'templateFunc<&fnUsee>' requested here}}
// callParLevel-note@#templateFunc_routine {{'#pragma acc routine' for function 'templateFunc<&fnUsee>' appears here}}
// callParLevel-note@#fnUsee_routine {{'#pragma acc routine' for function 'fnUsee' appears here}}
USER_USER_ROUTINE_DIR // #templateFunc_user_routine
void templateFunc_user() {
  templateFunc<fnUsee>(); // #templateFunc_call
}

// callParLevel-error@#NamespaceUsees_outsideUser_call {{function 'NamespaceUsees_outsideUser' has '#pragma acc routine worker' but calls function 'NamespaceUsees::fn', which has '#pragma acc routine gang'}}
// callParLevel-note@#NamespaceUsees_outsideUser_routine {{'#pragma acc routine' for function 'NamespaceUsees_outsideUser' appears here}}
// callParLevel-note@#NamespaceUsees_fn_routine {{'#pragma acc routine' for function 'NamespaceUsees::fn' appears here}}
USER_ROUTINE_DIR // #NamespaceUsees_outsideUser_routine
void NamespaceUsees_outsideUser() {
  NamespaceUsees::fn(); // #NamespaceUsees_outsideUser_call
}

// callParLevel-error@#NamespaceUsees_user_call {{function 'NamespaceUsees::user' has '#pragma acc routine worker' but calls function 'NamespaceUsees::fn', which has '#pragma acc routine gang'}}
// callParLevel-note@#NamespaceUsees_user_routine {{'#pragma acc routine' for function 'NamespaceUsees::user' appears here}}
// callParLevel-note@#NamespaceUsees_fn_routine {{'#pragma acc routine' for function 'NamespaceUsees::fn' appears here}}
USER_ROUTINE_DIR // #NamespaceUsees_user_routine
namespace NamespaceUsees {
  void user() {
    NamespaceUsees::fn(); // #NamespaceUsees_user_call
  }
}

// The cases below check a lambda as the user.  Because explicit routine
// directives are not yet supported for lambdas, and because an implicit routine
// directive for a lambda is determined by its usees, its level of parallelism
// cannot be incompatible with its usees.
//
// At one time, Clang's OpenACC analysis overlooked enclosing lambdas as if
// their contents appeared at file scope, in an enclosing function, or in an
// enclosing construct.  In most cases below, if that were to happen, there
// would be a diagnostic about an incompatible level of parallelism between a
// lambda's usees and whatever encloses the lambda.
//
// For each accLambda below, we imply rather than explicitly specify (using
// USER_ROUTINE_DIR) a routine directive for two reasons.  First, explicit
// routine directives on lambdas aren't supported yet.  Second, we want to check
// that the call to fnUseeIn*AccRoutineLambda is correctly recorded as occurring
// in accLambda (not in lambdaDefiner) specifically in the case that there's no
// routine directive yet on accLambda as that case used to misbehave.
void lambdaDefiner() {
  // It's a "host" lambda in that it's never actually used in a compute region,
  // but it has an implicit routine directive based on its definition.
  auto hostLambda = []() {
    fnUseeInNestedHostLambda(); // #fnUseeInNestedHostLambda_call
  };
  auto accLambda = []() {
    fnUseeInNestedAccRoutineLambda(); // #fnUseeInNestedAccRoutineLambda_call
  };
  #pragma acc parallel
  accLambda(); // #lambdaDefiner_accLambda_call
  #pragma acc parallel
  {
    []() {
      fnUseeInAccParallelLambda(); // #fnUseeInAccParallelLambda_call
    }(); // #lambdaDefiner_accParallelLambda_call
    USER_LOOP_DIR // #lambdaDefiner_loop
    for (int i = 0; i < 8; ++i) {
      auto f = []() {
        fnUseeInAccLoopLambda(); // #fnUseeInAccLoopLambda_call
        // The following makes sure f has a routine directive so that
        // fnUseeInAccLoopLambda always has one due to its use in f.
        #pragma acc loop seq // #lambdaDefiner_accLoopLambda_loop
        for (int i = 0; i < 8; ++i)
          ;
      };
    }
  }
}
// It's a "host" lambda in that it's never actually used in a compute region,
// but it has an implicit routine directive based on its definition.
auto hostLambda = []() {
  fnUseeInHostLambda(); // #fnUseeInHostLambda_call
};
auto accLambda = []() {
  fnUseeInAccRoutineLambda(); // #fnUseeInAccRoutineLambda_call
};
void accLambdaUser() {
  #pragma acc parallel
  accLambda(); // #accLambda_call
}
