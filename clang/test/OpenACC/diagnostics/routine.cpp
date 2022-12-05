// Check some of the "acc routine" diagnostic cases from routine.c but in C++.
//
// This is intended to complement rather than repeat coverage already in
// routine-cxx-funcs.

// RUN: %clang_cc1 -verify=noacc -Wno-gnu-alignof-expression %s
// RUN: %clang_cc1 -verify=expected -fopenacc \
// RUN:     -Wno-openacc-and-cxx -Wno-gnu-alignof-expression %s

// END.

#include <stddef.h>

// Generate unique name based on the line number.
#define UNIQUE_NAME CONCAT2(unique_name_, __LINE__)
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

class ParLevelConflictForClass {
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
public:
  void afterClass();
};

// expected-error@+1 {{unexpected OpenACC clause 'worker', 'gang' is specified already}}
#pragma acc routine gang worker
void ParLevelConflictForClass::afterClass() {}

void UNIQUE_NAME() {
  class UNIQUE_NAME {
    // expected-error@+1 {{unexpected OpenACC clause 'gang', 'vector' is specified already}}
    #pragma acc routine vector gang
    void UNIQUE_NAME();
  public:
    // expected-error@+1 {{unexpected OpenACC clause 'worker', 'vector' is specified already}}
    #pragma acc routine vector worker
    void UNIQUE_NAME() {}
  protected:
    // expected-error@+1 {{directive '#pragma acc routine' cannot contain more than one 'vector' clause}}
    #pragma acc routine vector vector
    void UNIQUE_NAME();
  private:
    // expected-error@+1 {{unexpected OpenACC clause 'seq', 'vector' is specified already}}
    #pragma acc routine vector seq
    void UNIQUE_NAME() {}
  };
}

// Try one struct case.
struct UNIQUE_NAME {
  // expected-error@+1 {{directive '#pragma acc routine' cannot contain more than one 'gang' clause}}
  #pragma acc routine gang gang
  void UNIQUE_NAME();
};

namespace ParLevelConflictForNamespace {
  // expected-error@+1 {{unexpected OpenACC clause 'gang', 'seq' is specified already}}
  #pragma acc routine seq gang
  void UNIQUE_NAME();
  // expected-error@+1 {{unexpected OpenACC clause 'worker', 'seq' is specified already}}
  #pragma acc routine seq worker
  void UNIQUE_NAME() {}
  void afterNamespace();
}

// expected-error@+1 {{unexpected OpenACC clause 'vector', 'seq' is specified already}}
#pragma acc routine seq vector
void ParLevelConflictForNamespace::afterNamespace() {}

//..............................................................................
// Conflicting clauses on different explicit directives for the same function.

class ParLevelConflictForClassExpExp {
  // expected-note@+1 {{previous '#pragma acc routine' for function 'ParLevelConflictForClassExpExp::defaultAccess' appears here}}
  #pragma acc routine vector
  void defaultAccess();
public:
  // expected-note@+1 {{previous '#pragma acc routine' for function 'ParLevelConflictForClassExpExp::publicAccess' appears here}}
  #pragma acc routine vector
  void publicAccess();
protected:
  // expected-note@+1 {{previous '#pragma acc routine' for function 'ParLevelConflictForClassExpExp::protectedAccess' appears here}}
  #pragma acc routine vector
  void protectedAccess();
private:
  // expected-note@+1 {{previous '#pragma acc routine' for function 'ParLevelConflictForClassExpExp::privateAccess' appears here}}
  #pragma acc routine seq
  void privateAccess();
};

// expected-error@+1 {{for function 'ParLevelConflictForClassExpExp::defaultAccess', '#pragma acc routine gang' conflicts with previous '#pragma acc routine vector'}}
#pragma acc routine gang
void ParLevelConflictForClassExpExp::defaultAccess() {}

// expected-error@+1 {{for function 'ParLevelConflictForClassExpExp::publicAccess', '#pragma acc routine worker' conflicts with previous '#pragma acc routine vector'}}
#pragma acc routine worker
void ParLevelConflictForClassExpExp::publicAccess() {}

// expected-error@+1 {{for function 'ParLevelConflictForClassExpExp::protectedAccess', '#pragma acc routine seq' conflicts with previous '#pragma acc routine vector'}}
#pragma acc routine seq
void ParLevelConflictForClassExpExp::protectedAccess() {}

// expected-error@+1 {{for function 'ParLevelConflictForClassExpExp::privateAccess', '#pragma acc routine gang' conflicts with previous '#pragma acc routine seq'}}
#pragma acc routine gang
void ParLevelConflictForClassExpExp::privateAccess() {}

// Try one struct case.
struct ParLevelConflictForStructExpExp {
  // expected-note@+1 {{previous '#pragma acc routine' for function 'ParLevelConflictForStructExpExp::foo' appears here}}
  #pragma acc routine worker
  void foo();
};
// expected-error@+1 {{for function 'ParLevelConflictForStructExpExp::foo', '#pragma acc routine vector' conflicts with previous '#pragma acc routine worker'}}
#pragma acc routine vector
void ParLevelConflictForStructExpExp::foo() {}

// expected-note@+1 {{previous '#pragma acc routine' for function 'operator*' appears here}}
#pragma acc routine seq
ParLevelConflictForClassExpExp operator*(ParLevelConflictForClassExpExp x, ParLevelConflictForClassExpExp y);
// expected-error@+1 {{for function 'operator*', '#pragma acc routine gang' conflicts with previous '#pragma acc routine seq'}}
#pragma acc routine gang
ParLevelConflictForClassExpExp operator*(ParLevelConflictForClassExpExp x, ParLevelConflictForClassExpExp y);

namespace ParLevelConflictForNamespaceExpExp {
  // expected-note@+1 {{previous '#pragma acc routine' for function 'ParLevelConflictForNamespaceExpExp::innerDeclInnerDecl' appears here}}
  #pragma acc routine gang
  void innerDeclInnerDecl();
  // expected-error@+1 {{for function 'ParLevelConflictForNamespaceExpExp::innerDeclInnerDecl', '#pragma acc routine worker' conflicts with previous '#pragma acc routine gang'}}
  #pragma acc routine worker
  void innerDeclInnerDecl();
  // expected-note@+1 {{previous '#pragma acc routine' for function 'ParLevelConflictForNamespaceExpExp::innerDeclOuterDecl' appears here}}
  #pragma acc routine gang
  void innerDeclOuterDecl();
  // expected-note@+1 {{previous '#pragma acc routine' for function 'ParLevelConflictForNamespaceExpExp::innerDeclInnerDef' appears here}}
  #pragma acc routine gang
  void innerDeclInnerDef();
  // expected-error@+1 {{for function 'ParLevelConflictForNamespaceExpExp::innerDeclInnerDef', '#pragma acc routine seq' conflicts with previous '#pragma acc routine gang'}}
  #pragma acc routine seq
  void innerDeclInnerDef() {}
  // expected-note@+1 {{previous '#pragma acc routine' for function 'ParLevelConflictForNamespaceExpExp::innerDeclOuterDef' appears here}}
  #pragma acc routine vector
  void innerDeclOuterDef();
}

// expected-error@+1 {{for function 'ParLevelConflictForNamespaceExpExp::innerDeclOuterDecl', '#pragma acc routine vector' conflicts with previous '#pragma acc routine gang'}}
#pragma acc routine vector
void ParLevelConflictForNamespaceExpExp::innerDeclOuterDecl();

// expected-error@+1 {{for function 'ParLevelConflictForNamespaceExpExp::innerDeclOuterDef', '#pragma acc routine worker' conflicts with previous '#pragma acc routine vector'}}
#pragma acc routine worker
void ParLevelConflictForNamespaceExpExp::innerDeclOuterDef() {}

//..............................................................................
// Conflicting clauses between explicit directive and previously implied
// directive for the same function.
//
// These necessarily include errors that the explicit directives appear after
// uses.  Those errors are more thoroughly tested separately later in this file,
// but we do vary the way they happen some here to check for undesirable
// interactions between the errors.

class ParLevelConflictForClassImpExp {
  void declUseOnDef();
  void user() {
    #pragma acc parallel
    {
      // expected-note@+2 {{use of function 'ParLevelConflictForClassImpExp::declUseOnDef' appears here}}
      // expected-note@+1 {{'#pragma acc routine seq' previously implied for function 'ParLevelConflictForClassImpExp::declUseOnDef' by use in construct '#pragma acc parallel' here}}
      declUseOnDef();
      // expected-note@+2 {{use of function 'ParLevelConflictForClassImpExp::useDeclOnDef' appears here}}
      // expected-note@+1 {{'#pragma acc routine seq' previously implied for function 'ParLevelConflictForClassImpExp::useDeclOnDef' by use in construct '#pragma acc parallel' here}}
      useDeclOnDef();
    }
  }
  void useDeclOnDef();
};

// expected-error@+2 {{first '#pragma acc routine' for function 'ParLevelConflictForClassImpExp::declUseOnDef' not in scope at some uses}}
// expected-error@+1 {{for function 'ParLevelConflictForClassImpExp::declUseOnDef', '#pragma acc routine gang' conflicts with previous '#pragma acc routine seq'}}
#pragma acc routine gang
void ParLevelConflictForClassImpExp::declUseOnDef() {}

// expected-error@+2 {{first '#pragma acc routine' for function 'ParLevelConflictForClassImpExp::useDeclOnDef' not in scope at some uses}}
// expected-error@+1 {{for function 'ParLevelConflictForClassImpExp::useDeclOnDef', '#pragma acc routine worker' conflicts with previous '#pragma acc routine seq'}}
#pragma acc routine worker
void ParLevelConflictForClassImpExp::useDeclOnDef() {}

// Try one struct case.
struct ParLevelConflictForStructImpExp {
  void declUseOnDef();
  void user() {
    // expected-note@+3 {{use of function 'ParLevelConflictForStructImpExp::declUseOnDef' appears here}}
    // expected-note@+2 {{'#pragma acc routine seq' previously implied for function 'ParLevelConflictForStructImpExp::declUseOnDef' by use in construct '#pragma acc parallel' here}}
    #pragma acc parallel
    declUseOnDef();
  }
};
// expected-error@+2 {{first '#pragma acc routine' for function 'ParLevelConflictForStructImpExp::declUseOnDef' not in scope at some uses}}
// expected-error@+1 {{for function 'ParLevelConflictForStructImpExp::declUseOnDef', '#pragma acc routine gang' conflicts with previous '#pragma acc routine seq'}}
#pragma acc routine gang
void ParLevelConflictForStructImpExp::declUseOnDef() {}

namespace ParLevelConflictForNamespaceImpExp {
  void declUseOnInnerDecl();
  void declUseOnOuterDecl();
  void declUseOnInnerDef();
  void declUseOnOuterDef();
  void user() {
    #pragma acc parallel
    {
      // expected-note@+2 {{use of function 'ParLevelConflictForNamespaceImpExp::declUseOnInnerDecl' appears here}}
      // expected-note@+1 {{'#pragma acc routine seq' previously implied for function 'ParLevelConflictForNamespaceImpExp::declUseOnInnerDecl' by use in construct '#pragma acc parallel' here}}
      declUseOnInnerDecl();
      // expected-note@+2 {{use of function 'ParLevelConflictForNamespaceImpExp::declUseOnOuterDecl' appears here}}
      // expected-note@+1 {{'#pragma acc routine seq' previously implied for function 'ParLevelConflictForNamespaceImpExp::declUseOnOuterDecl' by use in construct '#pragma acc parallel' here}}
      declUseOnOuterDecl();
      // expected-note@+2 {{use of function 'ParLevelConflictForNamespaceImpExp::declUseOnInnerDef' appears here}}
      // expected-note@+1 {{'#pragma acc routine seq' previously implied for function 'ParLevelConflictForNamespaceImpExp::declUseOnInnerDef' by use in construct '#pragma acc parallel' here}}
      declUseOnInnerDef();
      // expected-note@+2 {{use of function 'ParLevelConflictForNamespaceImpExp::declUseOnOuterDef' appears here}}
      // expected-note@+1 {{'#pragma acc routine seq' previously implied for function 'ParLevelConflictForNamespaceImpExp::declUseOnOuterDef' by use in construct '#pragma acc parallel' here}}
      declUseOnOuterDef();
    }
  }
  // expected-error@+2 {{first '#pragma acc routine' for function 'ParLevelConflictForNamespaceImpExp::declUseOnInnerDecl' not in scope at some uses}}
  // expected-error@+1 {{for function 'ParLevelConflictForNamespaceImpExp::declUseOnInnerDecl', '#pragma acc routine gang' conflicts with previous '#pragma acc routine seq'}}
  #pragma acc routine gang
  void declUseOnInnerDecl();
  // expected-error@+2 {{first '#pragma acc routine' for function 'ParLevelConflictForNamespaceImpExp::declUseOnInnerDef' not in scope at some uses}}
  // expected-error@+1 {{for function 'ParLevelConflictForNamespaceImpExp::declUseOnInnerDef', '#pragma acc routine worker' conflicts with previous '#pragma acc routine seq'}}
  #pragma acc routine worker
  void declUseOnInnerDef() {}
}

// expected-error@+2 {{first '#pragma acc routine' for function 'ParLevelConflictForNamespaceImpExp::declUseOnOuterDecl' not in scope at some uses}}
// expected-error@+1 {{for function 'ParLevelConflictForNamespaceImpExp::declUseOnOuterDecl', '#pragma acc routine vector' conflicts with previous '#pragma acc routine seq'}}
#pragma acc routine vector
void ParLevelConflictForNamespaceImpExp::declUseOnOuterDecl();

// expected-error@+2 {{first '#pragma acc routine' for function 'ParLevelConflictForNamespaceImpExp::declUseOnOuterDef' not in scope at some uses}}
// expected-error@+1 {{for function 'ParLevelConflictForNamespaceImpExp::declUseOnOuterDef', '#pragma acc routine gang' conflicts with previous '#pragma acc routine seq'}}
#pragma acc routine gang
void ParLevelConflictForNamespaceImpExp::declUseOnOuterDef() {}

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

namespace UNIQUE_NAME {
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
}

//------------------------------------------------------------------------------
// Restrictions on location of function definition and uses.
//------------------------------------------------------------------------------

//..............................................................................
// Early uses.
//
// routine-cxx-funcs/early-uses.cpp focuses on various kinds of C++ functions
// while member function definitions are always after their classes/namespaces.
// Here, we focus on various possible placements of member function prototypes,
// definitions, and routine directives.

class EarlyUseForClass {
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
    // expected-note@+1 {{use of function 'EarlyUseForClass::declUseOnDef' appears here}}
    declUseOnDef();
    useOnDecl();
    useOnDef();
    useOnDeclDef();
    // expected-note@+1 {{use of function 'EarlyUseForClass::useDeclOnDef' appears here}}
    useDeclOnDef();

    // Accelerator uses.
    #pragma acc parallel
    {
      onDeclUse();
      onDefUse();
      onDeclUseDef();
      // expected-note@+1 {{use of function 'EarlyUseForClass::declUseOnDef' appears here}}
      declUseOnDef();
      useOnDecl();
      useOnDef();
      useOnDeclDef();
      // expected-note@+1 {{use of function 'EarlyUseForClass::useDeclOnDef' appears here}}
      useDeclOnDef();
    }
  }
  #pragma acc routine seq
  void inClassAccUser() {
    onDeclUse();
    onDefUse();
    onDeclUseDef();
    // expected-note@+1 {{use of function 'EarlyUseForClass::declUseOnDef' appears here}}
    declUseOnDef();
    useOnDecl();
    useOnDef();
    useOnDeclDef();
    // expected-note@+1 {{use of function 'EarlyUseForClass::useDeclOnDef' appears here}}
    useDeclOnDef();
  }
  // The following routine directives are visible in inClass*User even though
  // they appear afterward.
  #pragma acc routine seq
  void useOnDecl() {}
  #pragma acc routine seq
  void useOnDef() {}
  #pragma acc routine seq
  void useOnDeclDef();
  void useDeclOnDef();
};

void EarlyUseForClass_afterClassUser() {
  EarlyUseForClass o;

  // Host uses.
  o.onDeclUse();
  o.onDefUse();
  o.onDeclUseDef();
  // expected-note@+1 {{use of function 'EarlyUseForClass::declUseOnDef' appears here}}
  o.declUseOnDef();

  // Accelerator uses.
  #pragma acc parallel
  {
    o.onDeclUse();
    o.onDefUse();
    o.onDeclUseDef();
    // expected-note@+1 {{use of function 'EarlyUseForClass::declUseOnDef' appears here}}
    o.declUseOnDef();
  }
}

void EarlyUseForClass::onDeclUseDef() {}

// expected-error@+1 {{first '#pragma acc routine' for function 'EarlyUseForClass::declUseOnDef' not in scope at some uses}}
#pragma acc routine seq
void EarlyUseForClass::declUseOnDef() {}

void EarlyUseForClass::onDeclDefUse() {}

#pragma acc routine seq
void EarlyUseForClass::declOnDefUse() {}

void EarlyUseForClass::useOnDeclDef() {}

// expected-error@+1 {{first '#pragma acc routine' for function 'EarlyUseForClass::useDeclOnDef' not in scope at some uses}}
#pragma acc routine seq
void EarlyUseForClass::useDeclOnDef() {}

void EarlyUseForClass_afterDefAfterClassUser() {
  EarlyUseForClass o;

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
struct EarlyUseForStruct {
  void declUseOnDef();
  void user() {
    // expected-note@+2 {{use of function 'EarlyUseForStruct::declUseOnDef' appears here}}
    #pragma acc parallel
    declUseOnDef();
  }
};
// expected-error@+1 {{first '#pragma acc routine' for function 'EarlyUseForStruct::declUseOnDef' not in scope at some uses}}
#pragma acc routine seq
void EarlyUseForStruct::declUseOnDef() {}

namespace EarlyUseForNamespace {
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
  void inNamespaceUser() {
    // Host uses.
    onDeclUse();
    onDefUse();
    onDeclUseDef();
    // expected-note@+1 {{use of function 'EarlyUseForNamespace::declUseOnDef' appears here}}
    declUseOnDef();

    // Accelerator uses.
    #pragma acc parallel
    {
      onDeclUse();
      onDefUse();
      onDeclUseDef();
      // expected-note@+1 {{use of function 'EarlyUseForNamespace::declUseOnDef' appears here}}
      declUseOnDef();
    }
  }
  #pragma acc routine seq
  void inNamespaceAccUser() {
    onDeclUse();
    onDefUse();
    onDeclUseDef();
    // expected-note@+1 {{use of function 'EarlyUseForNamespace::declUseOnDef' appears here}}
    declUseOnDef();
  }
}

void EarlyUseForNamespace_afterNamespaceUser() {
  // Host uses.
  EarlyUseForNamespace::onDeclUse();
  EarlyUseForNamespace::onDefUse();
  EarlyUseForNamespace::onDeclUseDef();
  // expected-note@+1 {{use of function 'EarlyUseForNamespace::declUseOnDef' appears here}}
  EarlyUseForNamespace::declUseOnDef();

  // Accelerator uses.
  #pragma acc parallel
  {
    EarlyUseForNamespace::onDeclUse();
    EarlyUseForNamespace::onDefUse();
    EarlyUseForNamespace::onDeclUseDef();
    // expected-note@+1 {{use of function 'EarlyUseForNamespace::declUseOnDef' appears here}}
    EarlyUseForNamespace::declUseOnDef();
  }
}

void EarlyUseForNamespace::onDeclUseDef() {}

// expected-error@+1 {{first '#pragma acc routine' for function 'EarlyUseForNamespace::declUseOnDef' not in scope at some uses}}
#pragma acc routine seq
void EarlyUseForNamespace::declUseOnDef() {}

void EarlyUseForNamespace::onDeclDefUse() {}

#pragma acc routine seq
void EarlyUseForNamespace::declOnDefUse() {}

void EarlyUseForNamespace_afterDefAfterNamespaceUser() {
  // Host uses.
  EarlyUseForNamespace::onDeclDefUse();
  EarlyUseForNamespace::declOnDefUse();

  // Accelerator uses.
  #pragma acc parallel
  {
    EarlyUseForNamespace::onDeclDefUse();
    EarlyUseForNamespace::declOnDefUse();
  }
}

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

namespace EarlyDefForNamespace {
  // expected-note@+1 {{definition of function 'EarlyDefForNamespace::innerDefInnerDecl' appears here}}
  void innerDefInnerDecl() {}
  // expected-error@+1 {{first '#pragma acc routine' for function 'EarlyDefForNamespace::innerDefInnerDecl' not in scope at definition}}
  #pragma acc routine seq
  void innerDefInnerDecl();
  #pragma acc routine seq // Should note only the first routine directive.
  void innerDefInnerDecl();
  // expected-note@+1 {{definition of function 'EarlyDefForNamespace::innerDefOuterDecl' appears here}}
  void innerDefOuterDecl() {}
}

// expected-error@+1 {{first '#pragma acc routine' for function 'EarlyDefForNamespace::innerDefOuterDecl' not in scope at definition}}
#pragma acc routine seq
void EarlyDefForNamespace::innerDefOuterDecl();
#pragma acc routine seq // Should note only the first routine directive.
void EarlyDefForNamespace::innerDefOuterDecl();

//------------------------------------------------------------------------------
// Restrictions on the function definition body for explicit routine directives.
//
// routine-cxx-funcs/func-bad-content-exp.cpp focuses on various kinds of C++
// functions while routine directives for class/namespace member functions are
// always in the class/namespace and while member function definitions are
// always after their classes/namespaces.  Here, we focus on various possible
// placements of member function prototypes, definitions, and routine
// directives.
//------------------------------------------------------------------------------

struct BadContentForClass {
  // expected-note@+1 3 {{'#pragma acc routine' for function 'BadContentForClass::onDef' appears here}}
  #pragma acc routine vector
  void onDef() {
    // expected-error@+1 {{static local variable 'x' is not permitted within function 'BadContentForClass::onDef' because the latter is attributed with '#pragma acc routine'}}
    static int x;
    // expected-error@+1 {{'#pragma acc parallel' is not permitted within function 'BadContentForClass::onDef' because the latter is attributed with '#pragma acc routine'}}
    #pragma acc parallel
    ;
    // expected-error@+1 {{function 'BadContentForClass::onDef' has '#pragma acc routine vector' but contains '#pragma acc loop worker'}}
    #pragma acc loop worker
    for (int i = 0; i < 5; ++i)
      ;
  }
  // expected-note@+1 3 {{'#pragma acc routine' for function 'BadContentForClass::onDeclDef' appears here}}
  #pragma acc routine vector
  void onDeclDef();
  void declOnDef();
  #pragma acc routine worker
  void onDeclOnDef();
};

void BadContentForClass::onDeclDef() {
  // expected-error@+1 {{static local variable 'x' is not permitted within function 'BadContentForClass::onDeclDef' because the latter is attributed with '#pragma acc routine'}}
  static int x;
  // expected-error@+1 {{'#pragma acc parallel loop' is not permitted within function 'BadContentForClass::onDeclDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel loop
  for (int i = 0; i < 5; ++i)
  ;
  // expected-error@+1 {{function 'BadContentForClass::onDeclDef' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
}

// expected-note@+1 3 {{'#pragma acc routine' for function 'BadContentForClass::declOnDef' appears here}}
#pragma acc routine seq
void BadContentForClass::declOnDef() {
  // expected-error@+1 {{static local variable 'x' is not permitted within function 'BadContentForClass::declOnDef' because the latter is attributed with '#pragma acc routine'}}
  static int x;
  int i;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'BadContentForClass::declOnDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  ;
  // expected-error@+1 {{function 'BadContentForClass::declOnDef' has '#pragma acc routine seq' but contains '#pragma acc loop vector'}}
  #pragma acc loop vector
  for (int i = 0; i < 5; ++i)
    ;
}

// expected-note@+1 3 {{'#pragma acc routine' for function 'BadContentForClass::onDeclOnDef' appears here}}
#pragma acc routine worker
void BadContentForClass::onDeclOnDef() {
  // expected-error@+1 {{static local variable 'x' is not permitted within function 'BadContentForClass::onDeclOnDef' because the latter is attributed with '#pragma acc routine'}}
  static int x;
  int i;
  // expected-error@+1 {{'#pragma acc enter data' is not permitted within function 'BadContentForClass::onDeclOnDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc enter data copyin(i)
  ;
  // expected-error@+1 {{function 'BadContentForClass::onDeclOnDef' has '#pragma acc routine worker' but contains '#pragma acc loop gang'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
}

namespace BadContentForNamespace {
  // expected-note@+1 3 {{'#pragma acc routine' for function 'BadContentForNamespace::onDef' appears here}}
  #pragma acc routine vector
  void onDef() {
    // expected-error@+1 {{static local variable 'x' is not permitted within function 'BadContentForNamespace::onDef' because the latter is attributed with '#pragma acc routine'}}
    static int x;
    // expected-error@+1 {{'#pragma acc parallel' is not permitted within function 'BadContentForNamespace::onDef' because the latter is attributed with '#pragma acc routine'}}
    #pragma acc parallel
    ;
    // expected-error@+1 {{function 'BadContentForNamespace::onDef' has '#pragma acc routine vector' but contains '#pragma acc loop worker'}}
    #pragma acc loop worker
    for (int i = 0; i < 5; ++i)
      ;
  }
  // expected-note@+1 3 {{'#pragma acc routine' for function 'BadContentForNamespace::onDeclInnerDef' appears here}}
  #pragma acc routine vector
  void onDeclInnerDef();
  void onDeclInnerDef() {
    // expected-error@+1 {{static local variable 'x' is not permitted within function 'BadContentForNamespace::onDeclInnerDef' because the latter is attributed with '#pragma acc routine'}}
    static int x;
    // expected-error@+1 {{'#pragma acc parallel loop' is not permitted within function 'BadContentForNamespace::onDeclInnerDef' because the latter is attributed with '#pragma acc routine'}}
    #pragma acc parallel loop
    for (int i = 0; i < 5; ++i)
    ;
    // expected-error@+1 {{function 'BadContentForNamespace::onDeclInnerDef' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
    #pragma acc loop gang
    for (int i = 0; i < 5; ++i)
      ;
  }
  // expected-note@+1 3 {{'#pragma acc routine' for function 'BadContentForNamespace::onDeclOuterDef' appears here}}
  #pragma acc routine vector
  void onDeclOuterDef();
  void declOnInnerDef();
  // expected-note@+1 3 {{'#pragma acc routine' for function 'BadContentForNamespace::declOnInnerDef' appears here}}
  #pragma acc routine seq
  void declOnInnerDef() {
    // expected-error@+1 {{static local variable 'x' is not permitted within function 'BadContentForNamespace::declOnInnerDef' because the latter is attributed with '#pragma acc routine'}}
    static int x;
    int i;
    // expected-error@+1 {{'#pragma acc update' is not permitted within function 'BadContentForNamespace::declOnInnerDef' because the latter is attributed with '#pragma acc routine'}}
    #pragma acc update device(i)
    ;
    // expected-error@+1 {{function 'BadContentForNamespace::declOnInnerDef' has '#pragma acc routine seq' but contains '#pragma acc loop vector'}}
    #pragma acc loop vector
    for (int i = 0; i < 5; ++i)
      ;
  }
  void declOnOuterDef();
  #pragma acc routine worker
  void onDeclOnInnerDef();
  // expected-note@+1 3 {{'#pragma acc routine' for function 'BadContentForNamespace::onDeclOnInnerDef' appears here}}
  #pragma acc routine worker
  void onDeclOnInnerDef() {
    // expected-error@+1 {{static local variable 'x' is not permitted within function 'BadContentForNamespace::onDeclOnInnerDef' because the latter is attributed with '#pragma acc routine'}}
    static int x;
    int i;
    // expected-error@+1 {{'#pragma acc enter data' is not permitted within function 'BadContentForNamespace::onDeclOnInnerDef' because the latter is attributed with '#pragma acc routine'}}
    #pragma acc enter data copyin(i)
    ;
    // expected-error@+1 {{function 'BadContentForNamespace::onDeclOnInnerDef' has '#pragma acc routine worker' but contains '#pragma acc loop gang'}}
    #pragma acc loop gang
    for (int i = 0; i < 5; ++i)
      ;
  }
  #pragma acc routine worker
  void onDeclOnOuterDef();
}

void BadContentForNamespace::onDeclOuterDef() {
  // expected-error@+1 {{static local variable 'x' is not permitted within function 'BadContentForNamespace::onDeclOuterDef' because the latter is attributed with '#pragma acc routine'}}
  static int x;
  // expected-error@+1 {{'#pragma acc parallel loop' is not permitted within function 'BadContentForNamespace::onDeclOuterDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel loop
  for (int i = 0; i < 5; ++i)
  ;
  // expected-error@+1 {{function 'BadContentForNamespace::onDeclOuterDef' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
}

// expected-note@+1 3 {{'#pragma acc routine' for function 'BadContentForNamespace::declOnOuterDef' appears here}}
#pragma acc routine seq
void BadContentForNamespace::declOnOuterDef() {
  // expected-error@+1 {{static local variable 'x' is not permitted within function 'BadContentForNamespace::declOnOuterDef' because the latter is attributed with '#pragma acc routine'}}
  static int x;
  int i;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'BadContentForNamespace::declOnOuterDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  ;
  // expected-error@+1 {{function 'BadContentForNamespace::declOnOuterDef' has '#pragma acc routine seq' but contains '#pragma acc loop vector'}}
  #pragma acc loop vector
  for (int i = 0; i < 5; ++i)
    ;
}

// expected-note@+1 3 {{'#pragma acc routine' for function 'BadContentForNamespace::onDeclOnOuterDef' appears here}}
#pragma acc routine worker
void BadContentForNamespace::onDeclOnOuterDef() {
  // expected-error@+1 {{static local variable 'x' is not permitted within function 'BadContentForNamespace::onDeclOnOuterDef' because the latter is attributed with '#pragma acc routine'}}
  static int x;
  int i;
  // expected-error@+1 {{'#pragma acc enter data' is not permitted within function 'BadContentForNamespace::onDeclOnOuterDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc enter data copyin(i)
  ;
  // expected-error@+1 {{function 'BadContentForNamespace::onDeclOnOuterDef' has '#pragma acc routine worker' but contains '#pragma acc loop gang'}}
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

struct ImpAccRoutineNoteForClass {
  //............................................................................
  // Usees with bad content due to implicit routine directives.
  //............................................................................

  int operator[](int) const {
    // expected-error@+3 {{static local variable 'x' is not permitted within function 'ImpAccRoutineNoteForClass::operator[]' because the latter is attributed with '#pragma acc routine'}}
    // expected-note@#ImpAccRoutineNoteForClass_opSubscript_use {{'#pragma acc routine seq' implied for function 'ImpAccRoutineNoteForClass::operator[]' by use in function 'ImpAccRoutineNoteForClass::operator()' here}}
    // expected-note@#ImpAccRoutineNoteForClass_opCall_routine {{'#pragma acc routine' for function 'ImpAccRoutineNoteForClass::operator()' appears here}}
    static int x;
    return 0;
  }
  int *begin() const {
    // expected-error@+3 {{static local variable 'x' is not permitted within function 'ImpAccRoutineNoteForClass::begin' because the latter is attributed with '#pragma acc routine'}}
    // expected-note@#ImpAccRoutineNoteForClass_rangeBasedLoop_use {{'#pragma acc routine seq' implied for function 'ImpAccRoutineNoteForClass::begin' by use in function 'ImpAccRoutineNoteForClass::operator()' here}}
    // expected-note@#ImpAccRoutineNoteForClass_opCall_routine {{'#pragma acc routine' for function 'ImpAccRoutineNoteForClass::operator()' appears here}}
    static int x;
    return nullptr;
  }
  int *end() const {
    // expected-error@+3 {{static local variable 'x' is not permitted within function 'ImpAccRoutineNoteForClass::end' because the latter is attributed with '#pragma acc routine'}}
    // expected-note@#ImpAccRoutineNoteForClass_rangeBasedLoop_use {{'#pragma acc routine seq' implied for function 'ImpAccRoutineNoteForClass::end' by use in function 'ImpAccRoutineNoteForClass::operator()' here}}
    // expected-note@#ImpAccRoutineNoteForClass_opCall_routine {{'#pragma acc routine' for function 'ImpAccRoutineNoteForClass::operator()' appears here}}
    static int x;
    return nullptr;
  }
  ImpAccRoutineNoteForClass() {
    // expected-error@+2 {{static local variable 'x' is not permitted within function 'ImpAccRoutineNoteForClass::ImpAccRoutineNoteForClass' because the latter is attributed with '#pragma acc routine'}}
    // expected-note@#ImpAccRoutineNoteForClass_ctor_use {{'#pragma acc routine seq' implied for function 'ImpAccRoutineNoteForClass::ImpAccRoutineNoteForClass' by use in construct '#pragma acc parallel' here}}
    static int x;
  }
  int operator-() {
    // expected-error@+2 {{static local variable 'x' is not permitted within function 'ImpAccRoutineNoteForClass::operator-' because the latter is attributed with '#pragma acc routine'}}
    // expected-note@#ImpAccRoutineNoteForClass_opUnary_use {{'#pragma acc routine seq' implied for function 'ImpAccRoutineNoteForClass::operator-' by use in construct '#pragma acc parallel' here}}
    static int x;
    return 0;
  }
  int operator+(int) {
    // expected-error@+3 {{static local variable 'x' is not permitted within function 'ImpAccRoutineNoteForClass::operator+' because the latter is attributed with '#pragma acc routine'}}
    // expected-note@#ImpAccRoutineNoteForClass_opBinary_use {{'#pragma acc routine seq' implied for function 'ImpAccRoutineNoteForClass::operator+' by use in function 'ImpAccRoutineNoteForClass::operator delete' here}}
    // expected-note@#ImpAccRoutineNoteForClass_delete_routine {{'#pragma acc routine' for function 'ImpAccRoutineNoteForClass::operator delete' appears here}}
    static int x;
    return 0;
  }
  operator int() {
    // expected-error@+3 {{static local variable 'x' is not permitted within function 'ImpAccRoutineNoteForClass::operator int' because the latter is attributed with '#pragma acc routine'}}
    // expected-note@#ImpAccRoutineNoteForClass_opConvert_use {{'#pragma acc routine seq' implied for function 'ImpAccRoutineNoteForClass::operator int' by use in function 'ImpAccRoutineNoteForClass::operator delete' here}}
    // expected-note@#ImpAccRoutineNoteForClass_delete_routine {{'#pragma acc routine' for function 'ImpAccRoutineNoteForClass::operator delete' appears here}}
    static int x;
    return 0;
  }

  //............................................................................
  // In-class users implying routine directives for usees.
  //............................................................................

  #pragma acc routine worker // #ImpAccRoutineNoteForClass_opCall_routine
  int operator()() const {
    (*this)[3]; // #ImpAccRoutineNoteForClass_opSubscript_use
    #pragma acc loop vector
    for (int i = 0; i < 5; ++i)
      for (int Itr : *this) // #ImpAccRoutineNoteForClass_rangeBasedLoop_use
        ;
    return 1;
  }
  void *operator new(unsigned long);
  void operator delete(void *);
};

//............................................................................
// After-class users implying routine directives for usees.
//............................................................................

void *ImpAccRoutineNoteForClass::operator new(unsigned long) {
  #pragma acc parallel
  {
    ImpAccRoutineNoteForClass obj; // #ImpAccRoutineNoteForClass_ctor_use
    #pragma acc loop gang
    for (int i = 0; i < 5; ++i)
      -obj; // #ImpAccRoutineNoteForClass_opUnary_use
  }
  return (void*)1;
}

#pragma acc routine gang // #ImpAccRoutineNoteForClass_delete_routine
void ImpAccRoutineNoteForClass::operator delete(void *ptr) {
  ImpAccRoutineNoteForClass *objPtr = (ImpAccRoutineNoteForClass*)ptr;
  *objPtr + 3; // #ImpAccRoutineNoteForClass_opBinary_use
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    int j = (int)*objPtr; // #ImpAccRoutineNoteForClass_opConvert_use
}

namespace ImpAccRoutineNoteForNamespace {
  //............................................................................
  // Usees with bad content due to implicit routine directives.
  //............................................................................

  void inAccUserInnerDef() {
    // expected-error@+3 {{static local variable 'x' is not permitted within function 'ImpAccRoutineNoteForNamespace::inAccUserInnerDef' because the latter is attributed with '#pragma acc routine'}}
    // expected-note@#ImpAccRoutineNoteForNamespace_inAccUser_use {{'#pragma acc routine seq' implied for function 'ImpAccRoutineNoteForNamespace::inAccUserInnerDef' by use in function 'ImpAccRoutineNoteForNamespace::accUserInnerDef' here}}
    // expected-note@#ImpAccRoutineNoteForNamespace_accUserInnerDef_routine {{'#pragma acc routine' for function 'ImpAccRoutineNoteForNamespace::accUserInnerDef' appears here}}
    static int x;
  }
  void inOrphanedLoop() {
    // expected-error@+3 {{static local variable 'x' is not permitted within function 'ImpAccRoutineNoteForNamespace::inOrphanedLoop' because the latter is attributed with '#pragma acc routine'}}
    // expected-note@#ImpAccRoutineNoteForNamespace_rangeBasedLoop_use {{'#pragma acc routine seq' implied for function 'ImpAccRoutineNoteForNamespace::inOrphanedLoop' by use in function 'ImpAccRoutineNoteForNamespace::accUserInnerDef' here}}
    // expected-note@#ImpAccRoutineNoteForNamespace_accUserInnerDef_routine {{'#pragma acc routine' for function 'ImpAccRoutineNoteForNamespace::accUserInnerDef' appears here}}
    static int x;
  }
  void inParallel() {
    // expected-error@+2 {{static local variable 'x' is not permitted within function 'ImpAccRoutineNoteForNamespace::inParallel' because the latter is attributed with '#pragma acc routine'}}
    // expected-note@#ImpAccRoutineNoteForNamespace_inParallel_use {{'#pragma acc routine seq' implied for function 'ImpAccRoutineNoteForNamespace::inParallel' by use in construct '#pragma acc parallel' here}}
    static int x;
  }
  void inLoop() {
    // expected-error@+2 {{static local variable 'x' is not permitted within function 'ImpAccRoutineNoteForNamespace::inLoop' because the latter is attributed with '#pragma acc routine'}}
    // expected-note@#ImpAccRoutineNoteForNamespace_inLoop_use {{'#pragma acc routine seq' implied for function 'ImpAccRoutineNoteForNamespace::inLoop' by use in construct '#pragma acc parallel' here}}
    static int x;
  }
  void inAccUserOuterDef() {
    // expected-error@+3 {{static local variable 'x' is not permitted within function 'ImpAccRoutineNoteForNamespace::inAccUserOuterDef' because the latter is attributed with '#pragma acc routine'}}
    // expected-note@#ImpAccRoutineNoteForNamespace_inAccUserOuterDef_use {{'#pragma acc routine seq' implied for function 'ImpAccRoutineNoteForNamespace::inAccUserOuterDef' by use in function 'ImpAccRoutineNoteForNamespace::accUserOuterDef' here}}
    // expected-note@#ImpAccRoutineNoteForNamespace_accUserOuterDef_routine {{'#pragma acc routine' for function 'ImpAccRoutineNoteForNamespace::accUserOuterDef' appears here}}
    static int x;
  }

  //............................................................................
  // In-namespace users implying routine directives for usees.
  //............................................................................

  #pragma acc routine worker // #ImpAccRoutineNoteForNamespace_accUserInnerDef_routine
  void accUserInnerDef() {
    inAccUserInnerDef(); // #ImpAccRoutineNoteForNamespace_inAccUser_use
    #pragma acc loop vector
    for (int i = 0; i < 5; ++i)
      inOrphanedLoop(); // #ImpAccRoutineNoteForNamespace_rangeBasedLoop_use
  }
  void computeUser();
  void accUserOuterDef();
}

//............................................................................
// After-namespace users implying routine directives for usees.
//............................................................................

void ImpAccRoutineNoteForNamespace::computeUser() {
  #pragma acc parallel
  {
    inParallel(); // #ImpAccRoutineNoteForNamespace_inParallel_use
    #pragma acc loop gang
    for (int i = 0; i < 5; ++i)
      inLoop(); // #ImpAccRoutineNoteForNamespace_inLoop_use
  }
}

#pragma acc routine gang // #ImpAccRoutineNoteForNamespace_accUserOuterDef_routine
void ImpAccRoutineNoteForNamespace::accUserOuterDef() {
  inAccUserOuterDef(); // #ImpAccRoutineNoteForNamespace_inAccUserOuterDef_use
}

//------------------------------------------------------------------------------
// Compatible levels of parallelism.
//
// routine-cxx-funcs/call-par-level-*.cpp cover this pretty thoroughly, but the
// users are always outside the usee's class/namespace.  Here we check the case
// where the user and usee are in the same class/namespace.
//-----------------------------------------------------------------------------

class ParLevelCompatibleForClass {
  // expected-note@+1 {{'#pragma acc routine' for function 'ParLevelCompatibleForClass::forwardRefUser' appears here}}
  #pragma acc routine seq
  // expected-error@+1 {{function 'ParLevelCompatibleForClass::forwardRefUser' has '#pragma acc routine seq' but calls function 'ParLevelCompatibleForClass::forwardRefUsee', which has '#pragma acc routine gang'}}
  void forwardRefUser() { forwardRefUsee(); }
  // expected-note@+1 {{'#pragma acc routine' for function 'ParLevelCompatibleForClass::forwardRefUsee' appears here}}
  #pragma acc routine gang
  void forwardRefUsee() {}

  // expected-note@+1 {{'#pragma acc routine' for function 'ParLevelCompatibleForClass::backwardRefUsee' appears here}}
  #pragma acc routine worker
  void backwardRefUsee() {}
  // expected-note@+1 {{'#pragma acc routine' for function 'ParLevelCompatibleForClass::backwardRefUser' appears here}}
  #pragma acc routine vector
  // expected-error@+1 {{function 'ParLevelCompatibleForClass::backwardRefUser' has '#pragma acc routine vector' but calls function 'ParLevelCompatibleForClass::backwardRefUsee', which has '#pragma acc routine worker'}}
  void backwardRefUser() { backwardRefUsee(); }
};

namespace ParLevelCompatibleForNamespace {
  // expected-note@+1 {{'#pragma acc routine' for function 'ParLevelCompatibleForNamespace::usee' appears here}}
  #pragma acc routine vector
  void usee() {}
  // expected-note@+1 {{'#pragma acc routine' for function 'ParLevelCompatibleForNamespace::user' appears here}}
  #pragma acc routine seq
  // expected-error@+1 {{function 'ParLevelCompatibleForNamespace::user' has '#pragma acc routine seq' but calls function 'ParLevelCompatibleForNamespace::usee', which has '#pragma acc routine vector'}}
  void user() { usee(); }
}

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

class MissingDeclarationForClass {
  // expected-error@+1 {{'#pragma acc routine' must be followed by a lone function prototype or definition}}
  #pragma acc routine seq
  #pragma acc routine seq
  void doubledInClass();
  void doubledAfterClass();
};

// expected-error@+1 {{'#pragma acc routine' must be followed by a lone function prototype or definition}}
#pragma acc routine seq
#pragma acc routine seq
void MissingDeclarationForClass::doubledAfterClass() {}

namespace UNIQUE_NAME {
  // expected-error@+1 {{'#pragma acc routine' must be followed by a lone function prototype or definition}}
  #pragma acc routine seq
}

namespace UNIQUE_NAME {
  // expected-error@+1 {{'#pragma acc routine' must be followed by a lone function prototype or definition}}
  #pragma acc routine seq
  // expected-error@+1 {{'#pragma acc routine' must be followed by a lone function prototype or definition}}
  #pragma acc routine seq
}

namespace MissingDeclarationForNamespace {
  // expected-error@+1 {{'#pragma acc routine' must be followed by a lone function prototype or definition}}
  #pragma acc routine seq
  #pragma acc routine seq
  void doubledInClass();
  void doubledAfterClass();
}

// expected-error@+1 {{'#pragma acc routine' must be followed by a lone function prototype or definition}}
#pragma acc routine seq
#pragma acc routine seq
void MissingDeclarationForNamespace::doubledAfterClass() {}
