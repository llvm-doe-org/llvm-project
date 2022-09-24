// Check some of the "acc routine" diagnostic cases from routine.c but in C++.
//
// This is intended to complement rather than repeat coverage already in
// routine-cxx-funcs.

// OpenACC disabled
// RUN: %clang_cc1 -verify=noacc -Wno-gnu-alignof-expression %s
// RUN: %clang_cc1 -verify=expected -fopenacc \
// RUN:     -Wno-openacc-and-cxx -Wno-gnu-alignof-expression %s

// END.

#include <stddef.h>

// Generate unique function name based on the line number to make it easier to
// avoid diagnostics about multiple routine directives for the same function.
#define UNIQUE_NAME CONCAT2(unique_fn_, __LINE__)
#define CONCAT2(X, Y) CONCAT(X, Y)
#define CONCAT(X, Y) X##Y

// noacc-no-diagnostics

//------------------------------------------------------------------------------
// Level-of-parallelism clauses for a single function.
//
// Conflicts between uses of the function and enclosing loops or functions are
// checked later.
//------------------------------------------------------------------------------

//..............................................................................
// Conflicting clauses on the same directive.

class UNIQUE_NAME {
  // expected-error@+1 {{unexpected OpenACC clause 'gang', 'worker' is specified already}}
  #pragma acc routine worker gang
  void UNIQUE_NAME() {}
public:
  // expected-error@+1 {{directive '#pragma acc routine' cannot contain more than one 'worker' clause}}
  #pragma acc routine worker worker
  void UNIQUE_NAME();
protected:
  // expected-error@+1 {{unexpected OpenACC clause 'vector', 'worker' is specified already}}
  #pragma acc routine worker vector
  void UNIQUE_NAME() {}
private:
  // expected-error@+1 {{unexpected OpenACC clause 'seq', 'worker' is specified already}}
  #pragma acc routine worker seq
  void UNIQUE_NAME();
};

void UNIQUE_NAME() {
  class UNIQUE_NAME {
    // expected-error@+1 {{unexpected OpenACC clause 'gang', 'worker' is specified already}}
    #pragma acc routine worker gang
    void UNIQUE_NAME();
  public:
    // expected-error@+1 {{directive '#pragma acc routine' cannot contain more than one 'worker' clause}}
    #pragma acc routine worker worker
    void UNIQUE_NAME() {}
  protected:
    // expected-error@+1 {{unexpected OpenACC clause 'vector', 'worker' is specified already}}
    #pragma acc routine worker vector
    void UNIQUE_NAME();
  private:
    // expected-error@+1 {{unexpected OpenACC clause 'seq', 'worker' is specified already}}
    #pragma acc routine worker seq
    void UNIQUE_NAME() {}
  };
}

// Try one struct case.
struct UNIQUE_NAME {
  // expected-error@+1 {{directive '#pragma acc routine' cannot contain more than one 'gang' clause}}
  #pragma acc routine gang gang
  void UNIQUE_NAME();
};

//..............................................................................
// Conflicting clauses on different explicit directives for the same function.

class ParLevelConflictExpExp {
  // expected-note@+1 {{previous '#pragma acc routine' for function 'ParLevelConflictExpExp::defaultAccess' appears here}}
  #pragma acc routine vector
  void defaultAccess();
public:
  // expected-note@+1 {{previous '#pragma acc routine' for function 'ParLevelConflictExpExp::publicAccess' appears here}}
  #pragma acc routine vector
  void publicAccess();
protected:
  // expected-note@+1 {{previous '#pragma acc routine' for function 'ParLevelConflictExpExp::protectedAccess' appears here}}
  #pragma acc routine vector
  void protectedAccess();
private:
  // expected-note@+1 {{previous '#pragma acc routine' for function 'ParLevelConflictExpExp::privateAccess' appears here}}
  #pragma acc routine seq
  void privateAccess();
};

// expected-error@+1 {{for function 'ParLevelConflictExpExp::defaultAccess', '#pragma acc routine gang' conflicts with previous '#pragma acc routine vector'}}
#pragma acc routine gang
void ParLevelConflictExpExp::defaultAccess() {}

// expected-error@+1 {{for function 'ParLevelConflictExpExp::publicAccess', '#pragma acc routine worker' conflicts with previous '#pragma acc routine vector'}}
#pragma acc routine worker
void ParLevelConflictExpExp::publicAccess() {}

// expected-error@+1 {{for function 'ParLevelConflictExpExp::protectedAccess', '#pragma acc routine seq' conflicts with previous '#pragma acc routine vector'}}
#pragma acc routine seq
void ParLevelConflictExpExp::protectedAccess() {}

// expected-error@+1 {{for function 'ParLevelConflictExpExp::privateAccess', '#pragma acc routine gang' conflicts with previous '#pragma acc routine seq'}}
#pragma acc routine gang
void ParLevelConflictExpExp::privateAccess() {}

// Try one struct case.
struct ParLevelConflictExpExpInStruct {
  // expected-note@+1 {{previous '#pragma acc routine' for function 'ParLevelConflictExpExpInStruct::foo' appears here}}
  #pragma acc routine worker
  void foo();
};
// expected-error@+1 {{for function 'ParLevelConflictExpExpInStruct::foo', '#pragma acc routine vector' conflicts with previous '#pragma acc routine worker'}}
#pragma acc routine vector
void ParLevelConflictExpExpInStruct::foo() {}

// expected-note@+1 {{previous '#pragma acc routine' for function 'operator*' appears here}}
#pragma acc routine seq
ParLevelConflictExpExp operator*(ParLevelConflictExpExp x, ParLevelConflictExpExp y);
// expected-error@+1 {{for function 'operator*', '#pragma acc routine gang' conflicts with previous '#pragma acc routine seq'}}
#pragma acc routine gang
ParLevelConflictExpExp operator*(ParLevelConflictExpExp x, ParLevelConflictExpExp y);

//..............................................................................
// Conflicting clauses between explicit directive and previously implied
// directive for the same function.
//
// These necessarily include errors that the explicit directives appear after
// uses.  Those errors are more thoroughly tested separately later in this file,
// but we do vary the way they happen some here to check for undesirable
// interactions between the errors.

class ParLevelConflictImpExp {
  void declUseOnDef();
  void user() {
    #pragma acc parallel
    {
      // expected-note@+2 {{use of function 'ParLevelConflictImpExp::declUseOnDef' appears here}}
      // expected-note@+1 {{'#pragma acc routine seq' previously implied for function 'ParLevelConflictImpExp::declUseOnDef' by use in construct '#pragma acc parallel' here}}
      declUseOnDef();
      // expected-note@+2 {{use of function 'ParLevelConflictImpExp::useDeclOnDef' appears here}}
      // expected-note@+1 {{'#pragma acc routine seq' previously implied for function 'ParLevelConflictImpExp::useDeclOnDef' by use in construct '#pragma acc parallel' here}}
      useDeclOnDef();
    }
  }
  void useDeclOnDef();
};

// expected-error@+2 {{first '#pragma acc routine' for function 'ParLevelConflictImpExp::declUseOnDef' not in scope at some uses}}
// expected-error@+1 {{for function 'ParLevelConflictImpExp::declUseOnDef', '#pragma acc routine gang' conflicts with previous '#pragma acc routine seq'}}
#pragma acc routine gang
void ParLevelConflictImpExp::declUseOnDef() {}

// expected-error@+2 {{first '#pragma acc routine' for function 'ParLevelConflictImpExp::useDeclOnDef' not in scope at some uses}}
// expected-error@+1 {{for function 'ParLevelConflictImpExp::useDeclOnDef', '#pragma acc routine worker' conflicts with previous '#pragma acc routine seq'}}
#pragma acc routine worker
void ParLevelConflictImpExp::useDeclOnDef() {}

// Try one struct case.
struct ParLevelConflictImpExpInStruct {
  void declUseOnDef();
  void user() {
    // expected-note@+3 {{use of function 'ParLevelConflictImpExpInStruct::declUseOnDef' appears here}}
    // expected-note@+2 {{'#pragma acc routine seq' previously implied for function 'ParLevelConflictImpExpInStruct::declUseOnDef' by use in construct '#pragma acc parallel' here}}
    #pragma acc parallel
    declUseOnDef();
  }
};
// expected-error@+2 {{first '#pragma acc routine' for function 'ParLevelConflictImpExpInStruct::declUseOnDef' not in scope at some uses}}
// expected-error@+1 {{for function 'ParLevelConflictImpExpInStruct::declUseOnDef', '#pragma acc routine gang' conflicts with previous '#pragma acc routine seq'}}
#pragma acc routine gang
void ParLevelConflictImpExpInStruct::declUseOnDef() {}

//------------------------------------------------------------------------------
// Bad associated declaration.
//------------------------------------------------------------------------------

class UNIQUE_NAME {
  // Multiple functions in one declaration.
  // expected-error@+1 {{'#pragma acc routine' must be followed by a lone function prototype or definition}}
  #pragma acc routine seq
  void UNIQUE_NAME(),
       UNIQUE_NAME();

  // Function pointer.
  // expected-error@+1 {{'#pragma acc routine' must be followed by a lone function prototype or definition}}
  #pragma acc routine seq
  void (*fp)();

  // Scalar.
  // expected-error@+1 {{'#pragma acc routine' must be followed by a lone function prototype or definition}}
  #pragma acc routine seq
  int i;

  // Type.
  // expected-error@+1 {{'#pragma acc routine' must be followed by a lone function prototype or definition}}
  #pragma acc routine seq
  struct AssociatedDeclIsType {
    int i;
  };

  // Null statement.
  // expected-error@+1 {{'#pragma acc routine' must be followed by a lone function prototype or definition}}
  #pragma acc routine seq
  ;
};

// Try one struct case.
struct UNIQUE_NAME {
  // Multiple functions in one declaration.
  // expected-error@+1 {{'#pragma acc routine' must be followed by a lone function prototype or definition}}
  #pragma acc routine seq
  void UNIQUE_NAME(),
       UNIQUE_NAME();
};

//------------------------------------------------------------------------------
// Restrictions on location of function definition and uses.
//------------------------------------------------------------------------------

//..............................................................................
// Early definitions.
//
// There seems to be no way to make that happen for class member functions until
// we support routine directives with names.

struct EarlyDef {};
// expected-note@+1 {{definition of function 'operator+' appears here}}
EarlyDef operator+(EarlyDef x, EarlyDef y) { return x; }
// expected-error@+1 {{first '#pragma acc routine' for function 'operator+' not in scope at definition}}
#pragma acc routine seq
EarlyDef operator+(EarlyDef x, EarlyDef y);

//..............................................................................
// Early uses.
//
// routine-cxx-funcs/early-uses.cpp focuses on various kinds of C++ functions
// while member function definitions are always after their classes.  Here, we
// focus on various possible placements of member function prototypes,
// definitions, and routine directives.

class EarlyUse {
public:
  #pragma acc routine seq
  void onDeclUse();
  #pragma acc routine seq
  void onDefUse() {}
  #pragma acc routine seq
  void onDeclUseDef();
  void declUseOnDef();
  #pragma acc routine seq
  void onDeclDefUse();
  void declOnDefUse();
  void inClassUser() {
    // Host uses.
    onDeclUse();
    onDefUse();
    onDeclUseDef();
    // expected-note@+1 {{use of function 'EarlyUse::declUseOnDef' appears here}}
    declUseOnDef();
    useOnDecl();
    useOnDef();
    useOnDeclDef();
    // expected-note@+1 {{use of function 'EarlyUse::useDeclOnDef' appears here}}
    useDeclOnDef();

    // Accelerator uses.
    #pragma acc parallel
    {
      onDeclUse();
      onDefUse();
      onDeclUseDef();
      // expected-note@+1 {{use of function 'EarlyUse::declUseOnDef' appears here}}
      declUseOnDef();
      useOnDecl();
      useOnDef();
      useOnDeclDef();
      // expected-note@+1 {{use of function 'EarlyUse::useDeclOnDef' appears here}}
      useDeclOnDef();
    }
  }
  #pragma acc routine seq
  void useOnDecl() {}
  #pragma acc routine seq
  void useOnDef() {}
  // The following routine directives are visible in inClassUser even though
  // they appear afterward.
  #pragma acc routine seq
  void useOnDeclDef();
  void useDeclOnDef();
};

void EarlyUses_afterClassUser() {
  EarlyUse o;

  // Host uses.
  o.onDeclUse();
  o.onDefUse();
  o.onDeclUseDef();
  // expected-note@+1 {{use of function 'EarlyUse::declUseOnDef' appears here}}
  o.declUseOnDef();

  // Accelerator uses.
  #pragma acc parallel
  {
    o.onDeclUse();
    o.onDefUse();
    o.onDeclUseDef();
    // expected-note@+1 {{use of function 'EarlyUse::declUseOnDef' appears here}}
    o.declUseOnDef();
  }
}

void EarlyUse::onDeclUseDef() {}

// expected-error@+1 {{first '#pragma acc routine' for function 'EarlyUse::declUseOnDef' not in scope at some uses}}
#pragma acc routine seq
void EarlyUse::declUseOnDef() {}

void EarlyUse::onDeclDefUse() {}

#pragma acc routine seq
void EarlyUse::declOnDefUse() {}

void EarlyUse::useOnDeclDef() {}

// expected-error@+1 {{first '#pragma acc routine' for function 'EarlyUse::useDeclOnDef' not in scope at some uses}}
#pragma acc routine seq
void EarlyUse::useDeclOnDef() {}

void EarlyUses_afterDefAfterClassUser() {
  EarlyUse o;

  // Host uses.
  o.onDeclDefUse();
  o.declOnDefUse();

  // Accelerator uses.
  #pragma acc parallel
  {
    o.onDeclDefUse();
    o.declOnDefUse();
  }
}

// Try one struct case.
struct EarlyUseInStruct {
  void declUseOnDef();
  void user() {
    // expected-note@+2 {{use of function 'EarlyUseInStruct::declUseOnDef' appears here}}
    #pragma acc parallel
    declUseOnDef();
  }
};
// expected-error@+1 {{first '#pragma acc routine' for function 'EarlyUseInStruct::declUseOnDef' not in scope at some uses}}
#pragma acc routine seq
void EarlyUseInStruct::declUseOnDef() {}

//------------------------------------------------------------------------------
// Restrictions on the function definition body for explicit routine directives.
//
// routine-cxx-funcs/func-bad-content-exp.cpp focuses on various kinds of C++
// functions while routine directives for class member functions are always in
// the class and member function definitions are always after their classes.
// Here, we focus on various possible placements of member function prototypes,
// definitions, and routine directives.
//------------------------------------------------------------------------------

struct BadContent {
  // expected-note@+1 3 {{'#pragma acc routine' for function 'BadContent::onDef' appears here}}
  #pragma acc routine vector
  void onDef() {
    // expected-error@+1 {{static local variable 'x' is not permitted within function 'BadContent::onDef' because the latter is attributed with '#pragma acc routine'}}
    static int x;
    // expected-error@+1 {{'#pragma acc parallel' is not permitted within function 'BadContent::onDef' because the latter is attributed with '#pragma acc routine'}}
    #pragma acc parallel
    ;
    // expected-error@+1 {{function 'BadContent::onDef' has '#pragma acc routine vector' but contains '#pragma acc loop worker'}}
    #pragma acc loop worker
    for (int i = 0; i < 5; ++i)
      ;
  }
  // expected-note@+1 3 {{'#pragma acc routine' for function 'BadContent::onDeclDef' appears here}}
  #pragma acc routine vector
  void onDeclDef();
  void declOnDef();
  #pragma acc routine worker
  void onDeclOnDef();
};

void BadContent::onDeclDef() {
  // expected-error@+1 {{static local variable 'x' is not permitted within function 'BadContent::onDeclDef' because the latter is attributed with '#pragma acc routine'}}
  static int x;
  // expected-error@+1 {{'#pragma acc parallel loop' is not permitted within function 'BadContent::onDeclDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel loop
  for (int i = 0; i < 5; ++i)
  ;
  // expected-error@+1 {{function 'BadContent::onDeclDef' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
}

// expected-note@+1 3 {{'#pragma acc routine' for function 'BadContent::declOnDef' appears here}}
#pragma acc routine seq
void BadContent::declOnDef() {
  // expected-error@+1 {{static local variable 'x' is not permitted within function 'BadContent::declOnDef' because the latter is attributed with '#pragma acc routine'}}
  static int x;
  int i;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'BadContent::declOnDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  ;
  // expected-error@+1 {{function 'BadContent::declOnDef' has '#pragma acc routine seq' but contains '#pragma acc loop vector'}}
  #pragma acc loop vector
  for (int i = 0; i < 5; ++i)
    ;
}

// expected-note@+1 3 {{'#pragma acc routine' for function 'BadContent::onDeclOnDef' appears here}}
#pragma acc routine worker
void BadContent::onDeclOnDef() {
  // expected-error@+1 {{static local variable 'x' is not permitted within function 'BadContent::onDeclOnDef' because the latter is attributed with '#pragma acc routine'}}
  static int x;
  int i;
  // expected-error@+1 {{'#pragma acc enter data' is not permitted within function 'BadContent::onDeclOnDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc enter data copyin(i)
  ;
  // expected-error@+1 {{function 'BadContent::onDeclOnDef' has '#pragma acc routine worker' but contains '#pragma acc loop gang'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
}

//------------------------------------------------------------------------------
// Restrictions on the function definition body for implicit routine directives.
//
// routine-cxx-funcs/func-bad-content-imp.cpp checks this for one context
// (currently that's the body of another accelerator routine) that can imply a
// routine directive, and it does so for many kinds of C++ functions.  Here, we
// focus on the various contexts in which it can be implied (which exercise
// different parts of the implementation in some cases), and we do not try to
// cover all kinds of C++ functions (but we vary that some).
//------------------------------------------------------------------------------

struct ImpAccRoutineNote {
  //............................................................................
  // Usees with bad content due to implicit routine directives.
  //............................................................................

  int operator[](int) const {
    // expected-error@+3 {{static local variable 'x' is not permitted within function 'ImpAccRoutineNote::operator[]' because the latter is attributed with '#pragma acc routine'}}
    // expected-note@#ImpAccRoutineNote_opSubscript_use {{'#pragma acc routine seq' implied for function 'ImpAccRoutineNote::operator[]' by use in function 'ImpAccRoutineNote::operator()' here}}
    // expected-note@#ImpAccRoutineNote_opCall_routine {{'#pragma acc routine' for function 'ImpAccRoutineNote::operator()' appears here}}
    static int x;
    return 0;
  }
  int *begin() const {
    // expected-error@+3 {{static local variable 'x' is not permitted within function 'ImpAccRoutineNote::begin' because the latter is attributed with '#pragma acc routine'}}
    // expected-note@#ImpAccRoutineNote_rangeBasedLoop_use {{'#pragma acc routine seq' implied for function 'ImpAccRoutineNote::begin' by use in function 'ImpAccRoutineNote::operator()' here}}
    // expected-note@#ImpAccRoutineNote_opCall_routine {{'#pragma acc routine' for function 'ImpAccRoutineNote::operator()' appears here}}
    static int x;
    return nullptr;
  }
  int *end() const {
    // expected-error@+3 {{static local variable 'x' is not permitted within function 'ImpAccRoutineNote::end' because the latter is attributed with '#pragma acc routine'}}
    // expected-note@#ImpAccRoutineNote_rangeBasedLoop_use {{'#pragma acc routine seq' implied for function 'ImpAccRoutineNote::end' by use in function 'ImpAccRoutineNote::operator()' here}}
    // expected-note@#ImpAccRoutineNote_opCall_routine {{'#pragma acc routine' for function 'ImpAccRoutineNote::operator()' appears here}}
    static int x;
    return nullptr;
  }
  ImpAccRoutineNote() {
    // expected-error@+2 {{static local variable 'x' is not permitted within function 'ImpAccRoutineNote::ImpAccRoutineNote' because the latter is attributed with '#pragma acc routine'}}
    // expected-note@#ImpAccRoutineNote_ctor_use {{'#pragma acc routine seq' implied for function 'ImpAccRoutineNote::ImpAccRoutineNote' by use in construct '#pragma acc parallel' here}}
    static int x;
  }
  int operator-() {
    // expected-error@+2 {{static local variable 'x' is not permitted within function 'ImpAccRoutineNote::operator-' because the latter is attributed with '#pragma acc routine'}}
    // expected-note@#ImpAccRoutineNote_opUnary_use {{'#pragma acc routine seq' implied for function 'ImpAccRoutineNote::operator-' by use in construct '#pragma acc parallel' here}}
    static int x;
    return 0;
  }
  int operator+(int) {
    // expected-error@+3 {{static local variable 'x' is not permitted within function 'ImpAccRoutineNote::operator+' because the latter is attributed with '#pragma acc routine'}}
    // expected-note@#ImpAccRoutineNote_opBinary_use {{'#pragma acc routine seq' implied for function 'ImpAccRoutineNote::operator+' by use in function 'ImpAccRoutineNote::operator delete' here}}
    // expected-note@#ImpAccRoutineNote_delete_routine {{'#pragma acc routine' for function 'ImpAccRoutineNote::operator delete' appears here}}
    static int x;
    return 0;
  }
  operator int() {
    // expected-error@+3 {{static local variable 'x' is not permitted within function 'ImpAccRoutineNote::operator int' because the latter is attributed with '#pragma acc routine'}}
    // expected-note@#ImpAccRoutineNote_opConvert_use {{'#pragma acc routine seq' implied for function 'ImpAccRoutineNote::operator int' by use in function 'ImpAccRoutineNote::operator delete' here}}
    // expected-note@#ImpAccRoutineNote_delete_routine {{'#pragma acc routine' for function 'ImpAccRoutineNote::operator delete' appears here}}
    static int x;
    return 0;
  }

  //............................................................................
  // In-class users implying routine directives for usees.
  //............................................................................

  #pragma acc routine worker // #ImpAccRoutineNote_opCall_routine
  int operator()() const {
    (*this)[3]; // #ImpAccRoutineNote_opSubscript_use
    #pragma acc loop vector
    for (int i = 0; i < 5; ++i)
      for (int Itr : *this) // #ImpAccRoutineNote_rangeBasedLoop_use
        ;
    return 1;
  }
  void *operator new(unsigned long);
  void operator delete(void *);
};

//............................................................................
// After-class users implying routine directives for usees.
//............................................................................

void *ImpAccRoutineNote::operator new(unsigned long) {
  #pragma acc parallel
  {
    ImpAccRoutineNote obj; // #ImpAccRoutineNote_ctor_use
    #pragma acc loop gang
    for (int i = 0; i < 5; ++i)
      -obj; // #ImpAccRoutineNote_opUnary_use
  }
  return (void*)1;
}

#pragma acc routine gang // #ImpAccRoutineNote_delete_routine
void ImpAccRoutineNote::operator delete(void *ptr) {
  ImpAccRoutineNote *objPtr = (ImpAccRoutineNote*)ptr;
  *objPtr + 3; // #ImpAccRoutineNote_opBinary_use
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    int j = (int)*objPtr; // #ImpAccRoutineNote_opConvert_use
}

//------------------------------------------------------------------------------
// Compatible levels of parallelism.
//
// routine-cxx-funcs/call-par-level-*.cpp cover this pretty thoroughly, but the
// users are always outside the usee's class.  Here we check the case where the
// user and usee are in the same class.
//-----------------------------------------------------------------------------

class ParLevelCompatible {
  // expected-note@+1 {{'#pragma acc routine' for function 'ParLevelCompatible::forwardRefUser' appears here}}
  #pragma acc routine seq
  // expected-error@+1 {{function 'ParLevelCompatible::forwardRefUser' has '#pragma acc routine seq' but calls function 'ParLevelCompatible::forwardRefUsee', which has '#pragma acc routine gang'}}
  void forwardRefUser() { forwardRefUsee(); }
  // expected-note@+1 {{'#pragma acc routine' for function 'ParLevelCompatible::forwardRefUsee' appears here}}
  #pragma acc routine gang
  void forwardRefUsee() {}

  // expected-note@+1 {{'#pragma acc routine' for function 'ParLevelCompatible::backwardRefUsee' appears here}}
  #pragma acc routine worker
  void backwardRefUsee() {}
  // expected-note@+1 {{'#pragma acc routine' for function 'ParLevelCompatible::backwardRefUser' appears here}}
  #pragma acc routine vector
  // expected-error@+1 {{function 'ParLevelCompatible::backwardRefUser' has '#pragma acc routine vector' but calls function 'ParLevelCompatible::backwardRefUsee', which has '#pragma acc routine worker'}}
  void backwardRefUser() { backwardRefUsee(); }
};

//------------------------------------------------------------------------------
// Missing declaration possibly within bad context.
//------------------------------------------------------------------------------

class UNIQUE_NAME {
  // expected-error@+1 {{'#pragma acc routine' must be followed by a lone function prototype or definition}}
  #pragma acc routine seq
};

struct UNIQUE_NAME {
  // expected-error@+1 {{'#pragma acc routine' must be followed by a lone function prototype or definition}}
  #pragma acc routine seq
};

class UNIQUE_NAME {
  // expected-error@+1 {{'#pragma acc routine' must be followed by a lone function prototype or definition}}
  #pragma acc routine seq
  // expected-error@+1 {{'#pragma acc routine' must be followed by a lone function prototype or definition}}
  #pragma acc routine seq
};

class MissingDeclaration {
  // expected-error@+1 {{'#pragma acc routine' must be followed by a lone function prototype or definition}}
  #pragma acc routine seq
  #pragma acc routine seq
  void doubledInClass();
  void doubledAfterClass();
};

// expected-error@+1 {{'#pragma acc routine' must be followed by a lone function prototype or definition}}
#pragma acc routine seq
#pragma acc routine seq
void MissingDeclaration::doubledAfterClass() {}
