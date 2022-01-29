// Check diagnostics for "acc routine".

// OpenACC disabled
// RUN: %clang_cc1 -verify=noacc -Wno-gnu-alignof-expression %s

// OpenACC enabled
// RUN: %clang_cc1 -verify=expected -fopenacc -Wno-gnu-alignof-expression %s

// END.

// noacc-no-diagnostics

#include <stddef.h>

int i, jk;
float f;

//--------------------------------------------------
// Missing clauses
//
// TODO: So far, only seq is implemented.
//--------------------------------------------------

// expected-error@+1 {{expected 'seq' clause for '#pragma acc routine'}}
#pragma acc routine
void foo();

// Any one of those clauses is sufficient to suppress that diagnostic.
#pragma acc routine seq
void foo();

//--------------------------------------------------
// Unrecognized clauses
//--------------------------------------------------

// Bogus clauses.

// expected-warning@+2 {{extra tokens at the end of '#pragma acc routine' are ignored}}
// expected-error@+1 {{expected 'seq' clause for '#pragma acc routine'}}
#pragma acc routine foo
void foo();
// expected-warning@+1 {{extra tokens at the end of '#pragma acc routine' are ignored}}
#pragma acc routine seq foo bar
void foo();

// Well formed clauses not permitted here.  Make sure every one is an error.

// expected-error@+9 {{unexpected OpenACC clause 'nomap' in directive '#pragma acc routine'}}
// expected-error@+8 {{unexpected OpenACC clause 'present' in directive '#pragma acc routine'}}
// expected-error@+7 {{unexpected OpenACC clause 'copy' in directive '#pragma acc routine'}}
// expected-error@+6 {{unexpected OpenACC clause 'copyin' in directive '#pragma acc routine'}}
// expected-error@+5 {{unexpected OpenACC clause 'copyout' in directive '#pragma acc routine'}}
// expected-error@+4 {{unexpected OpenACC clause 'create' in directive '#pragma acc routine'}}
// expected-error@+3 {{unexpected OpenACC clause 'no_create' in directive '#pragma acc routine'}}
// expected-error@+2 {{unexpected OpenACC clause 'delete' in directive '#pragma acc routine'}}
// expected-error@+1 {{expected 'seq' clause for '#pragma acc routine'}}
#pragma acc routine nomap(i) present(i) copy(i) copyin(i) copyout(i) create(i) no_create(i) delete(i)
void foo();
// expected-error@+4 {{unexpected OpenACC clause 'pcopy' in directive '#pragma acc routine'}}
// expected-error@+3 {{unexpected OpenACC clause 'pcopyin' in directive '#pragma acc routine'}}
// expected-error@+2 {{unexpected OpenACC clause 'pcopyout' in directive '#pragma acc routine'}}
// expected-error@+1 {{unexpected OpenACC clause 'pcreate' in directive '#pragma acc routine'}}
#pragma acc routine seq pcopy(i) pcopyin(i) pcopyout(i) pcreate(i)
void foo();
// expected-error@+4 {{unexpected OpenACC clause 'present_or_copy' in directive '#pragma acc routine'}}
// expected-error@+3 {{unexpected OpenACC clause 'present_or_copyin' in directive '#pragma acc routine'}}
// expected-error@+2 {{unexpected OpenACC clause 'present_or_copyout' in directive '#pragma acc routine'}}
// expected-error@+1 {{unexpected OpenACC clause 'present_or_create' in directive '#pragma acc routine'}}
#pragma acc routine present_or_copy(i) present_or_copyin(i) present_or_copyout(i) present_or_create(i) seq
void foo();
// expected-error@+4 {{unexpected OpenACC clause 'shared' in directive '#pragma acc routine'}}
// expected-error@+3 {{unexpected OpenACC clause 'reduction' in directive '#pragma acc routine'}}
// expected-error@+2 {{unexpected OpenACC clause 'private' in directive '#pragma acc routine'}}
// expected-error@+1 {{unexpected OpenACC clause 'firstprivate' in directive '#pragma acc routine'}}
#pragma acc routine shared(i, jk) reduction(+:i,jk,f) seq private(i) firstprivate(i)
void foo();
// expected-error@+6 {{unexpected OpenACC clause 'if' in directive '#pragma acc routine'}}
// expected-error@+5 {{unexpected OpenACC clause 'if_present' in directive '#pragma acc routine'}}
// expected-error@+4 {{unexpected OpenACC clause 'self' in directive '#pragma acc routine'}}
// expected-error@+3 {{unexpected OpenACC clause 'host' in directive '#pragma acc routine'}}
// expected-error@+2 {{unexpected OpenACC clause 'device' in directive '#pragma acc routine'}}
// expected-error@+1 {{expected 'seq' clause for '#pragma acc routine'}}
#pragma acc routine if(1) if_present self(i) host(jk) device(f)
void foo();
// expected-error@+3 {{unexpected OpenACC clause 'num_gangs' in directive '#pragma acc routine'}}
// expected-error@+2 {{unexpected OpenACC clause 'num_workers' in directive '#pragma acc routine'}}
// expected-error@+1 {{unexpected OpenACC clause 'vector_length' in directive '#pragma acc routine'}}
#pragma acc routine seq num_gangs(1) num_workers(2) vector_length(3)
void foo();
// expected-error@+6 {{unexpected OpenACC clause 'independent' in directive '#pragma acc routine'}}
// expected-error@+5 {{unexpected OpenACC clause 'auto' in directive '#pragma acc routine'}}
// expected-error@+4 {{unexpected OpenACC clause 'auto', 'independent' is specified already}}
// expected-error@+3 {{unexpected OpenACC clause 'collapse' in directive '#pragma acc routine'}}
// expected-error@+2 {{unexpected OpenACC clause 'seq', 'independent' is specified already}}
// expected-error@+1 {{expected 'seq' clause for '#pragma acc routine'}}
#pragma acc routine independent auto collapse(1) seq
void foo();

// Malformed clauses not permitted here.

// expected-error@+3 {{unexpected OpenACC clause 'shared' in directive '#pragma acc routine'}}
// expected-error@+2 {{expected '(' after 'shared'}}
// expected-error@+1 {{expected 'seq' clause for '#pragma acc routine'}}
#pragma acc routine shared
void foo();
// expected-error@+4 {{unexpected OpenACC clause 'shared' in directive '#pragma acc routine'}}
// expected-error@+3 {{expected expression}}
// expected-error@+2 {{expected ')'}}
// expected-note@+1 {{to match this '('}}
#pragma acc routine seq shared(
void foo();
// expected-error@+4 {{unexpected OpenACC clause 'shared' in directive '#pragma acc routine'}}
// expected-error@+3 {{expected ')'}}
// expected-note@+2 {{to match this '('}}
// expected-error@+1 {{expected 'seq' clause for '#pragma acc routine'}}
#pragma acc routine shared(i
void foo();

//--------------------------------------------------
// Partitioning clauses.
//
// TODO: So far, only seq is implemented, but we've already implemented the
// restriction that only one of gang, worker, vector, and seq is permitted.
//--------------------------------------------------

// expected-warning@+1 {{extra tokens at the end of '#pragma acc routine' are ignored}}
#pragma acc routine seq(
void foo();
// expected-warning@+1 {{extra tokens at the end of '#pragma acc routine' are ignored}}
#pragma acc routine seq()
void foo();
// expected-warning@+1 {{extra tokens at the end of '#pragma acc routine' are ignored}}
#pragma acc routine seq(i
void foo();
// expected-warning@+1 {{extra tokens at the end of '#pragma acc routine' are ignored}}
#pragma acc routine seq(i)
void foo();

// expected-error@+7 {{unexpected OpenACC clause 'gang' in directive '#pragma acc routine'}}
// expected-error@+6 {{unexpected OpenACC clause 'worker' in directive '#pragma acc routine'}}
// expected-error@+5 {{unexpected OpenACC clause 'worker', 'gang' is specified already}}
// expected-error@+4 {{unexpected OpenACC clause 'vector' in directive '#pragma acc routine'}}
// expected-error@+3 {{unexpected OpenACC clause 'vector', 'gang' is specified already}}
// expected-error@+2 {{unexpected OpenACC clause 'seq', 'gang' is specified already}}
// expected-error@+1 {{expected 'seq' clause for '#pragma acc routine'}}
#pragma acc routine gang worker vector seq
void foo();

// expected-error@+7 {{unexpected OpenACC clause 'worker' in directive '#pragma acc routine'}}
// expected-error@+6 {{unexpected OpenACC clause 'vector' in directive '#pragma acc routine'}}
// expected-error@+5 {{unexpected OpenACC clause 'vector', 'worker' is specified already}}
// expected-error@+4 {{unexpected OpenACC clause 'seq', 'worker' is specified already}}
// expected-error@+3 {{unexpected OpenACC clause 'gang' in directive '#pragma acc routine'}}
// expected-error@+2 {{unexpected OpenACC clause 'gang', 'worker' is specified already}}
// expected-error@+1 {{expected 'seq' clause for '#pragma acc routine'}}
#pragma acc routine worker vector seq gang
void foo();

// expected-error@+7 {{unexpected OpenACC clause 'vector' in directive '#pragma acc routine'}}
// expected-error@+6 {{unexpected OpenACC clause 'seq', 'vector' is specified already}}
// expected-error@+5 {{unexpected OpenACC clause 'gang' in directive '#pragma acc routine'}}
// expected-error@+4 {{unexpected OpenACC clause 'gang', 'vector' is specified already}}
// expected-error@+3 {{unexpected OpenACC clause 'worker' in directive '#pragma acc routine'}}
// expected-error@+2 {{unexpected OpenACC clause 'worker', 'gang' is specified already}}
// expected-error@+1 {{expected 'seq' clause for '#pragma acc routine'}}
#pragma acc routine vector seq gang worker
void foo();

// expected-error@+6 {{unexpected OpenACC clause 'gang' in directive '#pragma acc routine'}}
// expected-error@+5 {{unexpected OpenACC clause 'gang', 'seq' is specified already}}
// expected-error@+4 {{unexpected OpenACC clause 'worker' in directive '#pragma acc routine'}}
// expected-error@+3 {{unexpected OpenACC clause 'worker', 'gang' is specified already}}
// expected-error@+2 {{unexpected OpenACC clause 'vector' in directive '#pragma acc routine'}}
// expected-error@+1 {{unexpected OpenACC clause 'vector', 'gang' is specified already}}
#pragma acc routine seq gang worker vector
void foo();

//--------------------------------------------------
// Context not permitted.
//--------------------------------------------------

void withinFunction() {
  // expected-error@+1 {{unexpected OpenACC directive '#pragma acc routine'}}
  #pragma acc routine seq
  void foo();
  struct WithinStruct {
    // expected-error@+1 {{unexpected OpenACC directive '#pragma acc routine'}}
    #pragma acc routine seq
    int i;
  };
  #pragma acc parallel
  {
    // expected-error@+1 {{unexpected OpenACC directive '#pragma acc routine'}}
    #pragma acc routine seq
    void foo();
  }
  #pragma acc parallel
  {
    #pragma acc loop
    for (int i = 0; i < 8; ++i) {
      // expected-error@+1 {{unexpected OpenACC directive '#pragma acc routine'}}
      #pragma acc routine seq
      void foo();
    }
  }
  #pragma acc parallel
  {
    #pragma acc loop
    for (int i = 0; i < 8; ++i) {
      #pragma acc loop
      for (int j = 0; j < 8; ++j) {
        // expected-error@+1 {{unexpected OpenACC directive '#pragma acc routine'}}
        #pragma acc routine seq
        void foo();
      }
    }
  }
  #pragma acc parallel loop
  for (int i = 0; i < 8; ++i) {
    // expected-error@+1 {{unexpected OpenACC directive '#pragma acc routine'}}
    #pragma acc routine seq
    void foo();
  }
  #pragma acc data copy(i)
  {
    // expected-error@+1 {{unexpected OpenACC directive '#pragma acc routine'}}
    #pragma acc routine seq
    void foo();
  }
}

#pragma acc routine seq
void withinOrphanedLoop() {
  // TODO: Eventually orphaned loops will be permitted but not the routine
  // directive within it.
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop seq
  for (int i = 0; i < 8; ++i) {
    // expected-error@+1 {{unexpected OpenACC directive '#pragma acc routine'}}
    #pragma acc routine seq
    void foo();
  }
}

struct WithinStruct {
  // expected-error@+1 {{unexpected OpenACC directive '#pragma acc routine'}}
  #pragma acc routine seq
  int i;
};

//--------------------------------------------------
// Bad associated declaration.
//--------------------------------------------------

// Multiple functions in one declaration.
// expected-error@+1 {{'#pragma acc routine' must be followed by a lone function prototype or definition}}
#pragma acc routine seq
void foo(), bar();

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

//--------------------------------------------------
// Restrictions on location of function definition and uses.
//
// Proposed text for OpenACC after 3.2:
// - "In C and C++, a routine directive's scope starts at the routine directive
//   and ends at the end of the compilation unit."
// - "In C and C++, a definition or use of a procedure must appear within the
//   scope of at least one explicit and applying routine directive if any
//   appears in the same compilation unit."
// Uses include host uses and accelerator uses but only if they're evaluated
// (e.g., a reference in sizeof is not a use).
//--------------------------------------------------

// Evaluated host uses.
void hostUseBefore();
void hostUseBefore_hostUses() {
  // expected-error@+1 {{'#pragma acc routine' is not in scope at use of associated function 'hostUseBefore'}}
  hostUseBefore();
  // expected-error@+1 {{'#pragma acc routine' is not in scope at use of associated function 'hostUseBefore'}}
  void (*p)() = hostUseBefore;
}
// expected-note@+1 2 {{'#pragma acc routine' for function 'hostUseBefore' appears here}}
#pragma acc routine seq
void hostUseBefore();
#pragma acc routine seq // Should note only the first routine directive.
void hostUseBefore();

// Evaluated accelerator uses.
void accUseBefore();
#pragma acc routine seq
void accUseBefore_accUses() {
  // expected-error@+1 {{'#pragma acc routine' is not in scope at use of associated function 'accUseBefore'}}
  accUseBefore();
  // expected-error@+1 {{'#pragma acc routine' is not in scope at use of associated function 'accUseBefore'}}
  void (*p)() = accUseBefore;
}
void accUseBefore_accUses2() {
  #pragma acc parallel
  {
    // expected-error@+1 {{'#pragma acc routine' is not in scope at use of associated function 'accUseBefore'}}
    accUseBefore();
    // expected-error@+1 {{'#pragma acc routine' is not in scope at use of associated function 'accUseBefore'}}
    void (*p)() = accUseBefore;
  }
}
// expected-note@+1 4 {{'#pragma acc routine' for function 'accUseBefore' appears here}}
#pragma acc routine seq
void accUseBefore();
#pragma acc routine seq // Should note only the first routine directive.
void accUseBefore();

// Make sure use checks aren't skipped at a routine directive on a definition as
// opposed to a prototype.
void useBeforeOnDef();
// expected-error@+1 {{'#pragma acc routine' is not in scope at use of associated function 'useBeforeOnDef'}}
void useBeforeOnDef_hostUses() { useBeforeOnDef(); }
// expected-error@+2 {{'#pragma acc routine' is not in scope at use of associated function 'useBeforeOnDef'}}
#pragma acc routine seq
void useBeforeOnDef_accUses() { useBeforeOnDef(); }
// expected-note@+1 2 {{'#pragma acc routine' for function 'useBeforeOnDef' appears here}}
#pragma acc routine seq
void useBeforeOnDef() {}

// Unevaluated uses should not trigger an error.
void unevaluatedUseBefore();
void unevaluatedUseBefore_hostUses() {
  size_t s = sizeof &unevaluatedUseBefore;
  size_t a = _Alignof(&unevaluatedUseBefore);
}
#pragma acc routine seq
void unevaluatedUseBefore_accUses() {
  size_t s = sizeof &unevaluatedUseBefore;
  size_t a = _Alignof(&unevaluatedUseBefore);
}
#pragma acc routine seq
void unevaluatedUseBefore();

// Unevaluated uses shouldn't be reported with evaluated uses.
void notAllUseBefore();
void notAllUseBefore_hostUses() {
  size_t s = sizeof &notAllUseBefore;
  // expected-error@+1 {{'#pragma acc routine' is not in scope at use of associated function 'notAllUseBefore'}}
  notAllUseBefore();
  size_t a = _Alignof(&notAllUseBefore);
}
#pragma acc routine seq
void notAllUseBefore_accUses() {
  // expected-error@+1 {{'#pragma acc routine' is not in scope at use of associated function 'notAllUseBefore'}}
  notAllUseBefore();
  size_t a = _Alignof(&notAllUseBefore);
  // expected-error@+1 {{'#pragma acc routine' is not in scope at use of associated function 'notAllUseBefore'}}
  void (*p)() = notAllUseBefore;
}
// expected-note@+1 3 {{'#pragma acc routine' for function 'notAllUseBefore' appears here}}
#pragma acc routine seq
void notAllUseBefore();

// Definition.
// expected-error@+1 {{'#pragma acc routine' is not in scope at definition of associated function 'defBefore'}}
void defBefore() {}
// expected-note@+1 {{'#pragma acc routine' for function 'defBefore' appears here}}
#pragma acc routine seq
void defBefore();
#pragma acc routine seq // Should note only the first routine directive.
void defBefore();

// Use errors shouldn't suppress/break later definition errors.
void useDefBefore();
// expected-error@+1 {{'#pragma acc routine' is not in scope at use of associated function 'useDefBefore'}}
void useDefBefore_uses() { useDefBefore(); }
// expected-error@+1 {{'#pragma acc routine' is not in scope at definition of associated function 'useDefBefore'}}
void useDefBefore() {} // def
// expected-note@+1 2 {{'#pragma acc routine' for function 'useDefBefore' appears here}}
#pragma acc routine seq
void useDefBefore();
#pragma acc routine seq // Should note only the first routine directive.
void useDefBefore();

// Definition errors shouldn't suppress/break later use errors.
// expected-error@+1 {{'#pragma acc routine' is not in scope at definition of associated function 'defUseBefore'}}
void defUseBefore() {} // def
// expected-error@+1 {{'#pragma acc routine' is not in scope at use of associated function 'defUseBefore'}}
void defUseBefore_uses() { defUseBefore(); }
// expected-note@+1 2 {{'#pragma acc routine' for function 'defUseBefore' appears here}}
#pragma acc routine seq
void defUseBefore();
#pragma acc routine seq // Should note only the first routine directive.
void defUseBefore();

// Only one routine directive has to be in scope, so additional routine
// directives that are out of scope are fine.
#pragma acc routine seq
void useDefBetween();
void useDefBetween_hostUses() { useDefBetween(); }
#pragma acc routine seq
void useDefBetween_accUses() { useDefBetween(); }
void useDefBetween() {} // def
#pragma acc routine seq
void useDefBetween();
#pragma acc routine seq
void useDefBetween();

//--------------------------------------------------
// Restrictions on the function definition body for explicit routine directives.
//--------------------------------------------------

// Does the definition check see the attached routine directive?
//
// Exhaustively test diagnostic kinds this time, but choose only a single
// representative of each diagnostic kind after this.
//
// expected-note@+1 11 {{'#pragma acc routine' for function 'onDef' appears here}}
#pragma acc routine seq
void onDef() {
  // expected-error@+1 {{static local variable 's' is not permitted within function 'onDef' because the latter is attributed with '#pragma acc routine'}}
  static int s;
  int i;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'onDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  // expected-error@+1 {{'#pragma acc enter data' is not permitted within function 'onDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc enter data copyin(i)
  // expected-error@+1 {{'#pragma acc exit data' is not permitted within function 'onDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc exit data copyout(i)
  // expected-error@+1 {{'#pragma acc data' is not permitted within function 'onDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc data copy(i)
  {
    // expected-error@+1 {{static local variable 's' is not permitted within function 'onDef' because the latter is attributed with '#pragma acc routine'}}
    static int s;
  }
  // expected-error@+1 {{'#pragma acc parallel' is not permitted within function 'onDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel
  {
    // expected-error@+1 {{static local variable 's' is not permitted within function 'onDef' because the latter is attributed with '#pragma acc routine'}}
    static int s;
  }
  // expected-error@+1 {{'#pragma acc parallel loop' is not permitted within function 'onDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel loop seq
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{static local variable 's' is not permitted within function 'onDef' because the latter is attributed with '#pragma acc routine'}}
    static int s;
  }
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{static local variable 's' is not permitted within function 'onDef' because the latter is attributed with '#pragma acc routine'}}
    static int s;
  }
}

// Does the definition check see the routine directive on the preceding decl?
// expected-note@+1 2 {{'#pragma acc routine' for function 'onDeclDef' appears here}}
#pragma acc routine seq
void onDeclDef();
void onDeclDef() {
  // expected-error@+1 {{static local variable 's' is not permitted within function 'onDeclDef' because the latter is attributed with '#pragma acc routine'}}
  static int s;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'onDeclDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
}

// Does the definition check see the routine directive on the following decl?
// No, that's an error.
// expected-error@+1 {{'#pragma acc routine' is not in scope at definition of associated function 'defOnDecl'}}
void defOnDecl() {
  static int s;
  #pragma acc update device(i)
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
}
// expected-note@+1 {{'#pragma acc routine' for function 'defOnDecl' appears here}}
#pragma acc routine seq
void defOnDecl();

// Do we avoid repeated diagnostics and note the most recent routine directive?
#pragma acc routine seq
void onDeclOnDef();
// expected-note@+1 2 {{'#pragma acc routine' for function 'onDeclOnDef' appears here}}
#pragma acc routine seq
void onDeclOnDef() {
  // expected-error@+1 {{static local variable 's' is not permitted within function 'onDeclOnDef' because the latter is attributed with '#pragma acc routine'}}
  static int s;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'onDeclOnDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
}

// Do we avoid repeated diagnostics and note the most recent routine directive?
// expected-note@+1 2 {{'#pragma acc routine' for function 'onDefOnDecl' appears here}}
#pragma acc routine seq
void onDefOnDecl() {
  // expected-error@+1 {{static local variable 's' is not permitted within function 'onDefOnDecl' because the latter is attributed with '#pragma acc routine'}}
  static int s;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'onDefOnDecl' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
}
#pragma acc routine seq
void onDefOnDecl();

//--------------------------------------------------
// Restrictions on the function definition body for implicit routine directives.
//--------------------------------------------------

// Does the definition check see the routine directive previously implied by a
// use within a parallel construct?
void parUseBeforeDef();
void parUseBeforeDef_use() {
  #pragma acc parallel
  // expected-note@+1 2 {{'#pragma acc routine seq' implied for function 'parUseBeforeDef' by use in construct '#pragma acc parallel' here}}
  parUseBeforeDef();
}
void parUseBeforeDef() {
  // expected-error@+1 {{static local variable 's' is not permitted within function 'parUseBeforeDef' because the latter is attributed with '#pragma acc routine'}}
  static int s;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'parUseBeforeDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
  parUseBeforeDef();
}

// Does the definition check see the routine directive later implied by a use
// within a parallel construct?
void parUseAfterDef() {
  // expected-error@+1 {{static local variable 's' is not permitted within function 'parUseAfterDef' because the latter is attributed with '#pragma acc routine'}}
  static int s;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'parUseAfterDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
  parUseAfterDef();
}
void parUseAfterDef_use() {
  #pragma acc parallel
  // expected-note@+1 2 {{'#pragma acc routine seq' implied for function 'parUseAfterDef' by use in construct '#pragma acc parallel' here}}
  parUseAfterDef();
}

// Does the definition check see the routine directive implied by a use within a
// parallel construct within the same definition?
void parUseInDef() {
  // expected-error@+1 {{static local variable 'sBefore' is not permitted within function 'parUseInDef' because the latter is attributed with '#pragma acc routine'}}
  static int sBefore;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'parUseInDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{'#pragma acc parallel' is not permitted within function 'parUseInDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel
  // expected-note@+1 5 {{'#pragma acc routine seq' implied for function 'parUseInDef' by use in construct '#pragma acc parallel' here}}
  parUseInDef();
  // expected-error@+1 {{static local variable 'sAfter' is not permitted within function 'parUseInDef' because the latter is attributed with '#pragma acc routine'}}
  static int sAfter;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'parUseInDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
}

// Does the definition check see the routine directive previously implied by a
// use within a parallel loop construct?
void parLoopUseBeforeDef();
void parLoopUseBeforeDef_use() {
  #pragma acc parallel loop
  for (int i = 0; i < 5; ++i) {
    // expected-note@+1 2 {{'#pragma acc routine seq' implied for function 'parLoopUseBeforeDef' by use in construct '#pragma acc parallel loop' here}}
    parLoopUseBeforeDef();
  }
}
void parLoopUseBeforeDef() {
  // expected-error@+1 {{static local variable 's' is not permitted within function 'parLoopUseBeforeDef' because the latter is attributed with '#pragma acc routine'}}
  static int s;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'parLoopUseBeforeDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
}

// Does the definition check see the routine directive later implied by a use
// within a parallel loop construct?
void parLoopUseAfterDef() {
  // expected-error@+1 {{static local variable 's' is not permitted within function 'parLoopUseAfterDef' because the latter is attributed with '#pragma acc routine'}}
  static int s;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'parLoopUseAfterDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
}
void parLoopUseAfterDef_use() {
  #pragma acc parallel loop
  for (int i = 0; i < 5; ++i) {
    // expected-note@+1 2 {{'#pragma acc routine seq' implied for function 'parLoopUseAfterDef' by use in construct '#pragma acc parallel loop' here}}
    parLoopUseAfterDef();
  }
}

// Does the definition check see the routine directive implied by a use within a
// parallel loop construct within the same definition?
void parLoopUseInDef() {
  // expected-error@+1 {{static local variable 'sBefore' is not permitted within function 'parLoopUseInDef' because the latter is attributed with '#pragma acc routine'}}
  static int sBefore;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'parLoopUseInDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{'#pragma acc parallel' is not permitted within function 'parLoopUseInDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel
  // expected-note@+1 5 {{'#pragma acc routine seq' implied for function 'parLoopUseInDef' by use in construct '#pragma acc parallel' here}}
  parLoopUseInDef();
  // expected-error@+1 {{static local variable 'sAfter' is not permitted within function 'parLoopUseInDef' because the latter is attributed with '#pragma acc routine'}}
  static int sAfter;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'parLoopUseInDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
}

// Does the definition check see the routine directive previously implied by a
// use within a function with an explicit routine directive?
void expOffFnUseBeforeDef();
// expected-note@+1 2 {{'#pragma acc routine' for function 'expOffFnUseBeforeDef_use' appears here}}
#pragma acc routine seq
void expOffFnUseBeforeDef_use() {
  // expected-note@+1 2 {{'#pragma acc routine seq' implied for function 'expOffFnUseBeforeDef' by use in function 'expOffFnUseBeforeDef_use' here}}
  expOffFnUseBeforeDef();
}
void expOffFnUseBeforeDef() {
  // expected-error@+1 {{static local variable 's' is not permitted within function 'expOffFnUseBeforeDef' because the latter is attributed with '#pragma acc routine'}}
  static int s;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'expOffFnUseBeforeDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
}

// Does the definition check see the routine directive later implied by a use
// within a function with an explicit routine directive?
void expOffFnUseAfterDef() {
  // expected-error@+1 {{static local variable 's' is not permitted within function 'expOffFnUseAfterDef' because the latter is attributed with '#pragma acc routine'}}
  static int s;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'expOffFnUseAfterDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
}
// expected-note@+1 2 {{'#pragma acc routine' for function 'expOffFnUseAfterDef_use' appears here}}
#pragma acc routine seq
void expOffFnUseAfterDef_use() {
  // expected-note@+1 2 {{'#pragma acc routine seq' implied for function 'expOffFnUseAfterDef' by use in function 'expOffFnUseAfterDef_use' here}}
  expOffFnUseAfterDef();
}

// Does the definition check see the routine directive previously implied by a
// use within a function with an implicit routine directive?
void impOffFnUseBeforeDef();
void impOffFnUseBeforeDef_use();
// expected-note@+1 2 {{'#pragma acc routine' for function 'impOffFnUseBeforeDef_use_use' appears here}}
#pragma acc routine seq
void impOffFnUseBeforeDef_use_use() {
  // expected-note@+1 2 {{'#pragma acc routine seq' implied for function 'impOffFnUseBeforeDef_use' by use in function 'impOffFnUseBeforeDef_use_use' here}}
  impOffFnUseBeforeDef_use();
}
void impOffFnUseBeforeDef_use() {
  // expected-note@+1 2 {{'#pragma acc routine seq' implied for function 'impOffFnUseBeforeDef' by use in function 'impOffFnUseBeforeDef_use' here}}
  impOffFnUseBeforeDef();
}
void impOffFnUseBeforeDef() {
  // expected-error@+1 {{static local variable 's' is not permitted within function 'impOffFnUseBeforeDef' because the latter is attributed with '#pragma acc routine'}}
  static int s;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'impOffFnUseBeforeDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
}

// Repeat that but discover multiple implicit routine directives at once in a
// chain.
void chainedImpOffFnUseBeforeDef();
void chainedImpOffFnUseBeforeDef_use() {
  // expected-note@+1 2 {{'#pragma acc routine seq' implied for function 'chainedImpOffFnUseBeforeDef' by use in function 'chainedImpOffFnUseBeforeDef_use' here}}
  chainedImpOffFnUseBeforeDef();
}
// expected-note@+1 2 {{'#pragma acc routine' for function 'chainedImpOffFnUseBeforeDef_use_use' appears here}}
#pragma acc routine seq
void chainedImpOffFnUseBeforeDef_use_use() {
  // expected-note@+1 2 {{'#pragma acc routine seq' implied for function 'chainedImpOffFnUseBeforeDef_use' by use in function 'chainedImpOffFnUseBeforeDef_use_use' here}}
  chainedImpOffFnUseBeforeDef_use();
}
void chainedImpOffFnUseBeforeDef() {
  // expected-error@+1 {{static local variable 's' is not permitted within function 'chainedImpOffFnUseBeforeDef' because the latter is attributed with '#pragma acc routine'}}
  static int s;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'chainedImpOffFnUseBeforeDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
}

// Does the definition check see the routine directive later implied by a use
// within a function with an implicit routine directive?
void impOffFnUseAfterDef() {
  // expected-error@+1 {{static local variable 's' is not permitted within function 'impOffFnUseAfterDef' because the latter is attributed with '#pragma acc routine'}}
  static int s;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'impOffFnUseAfterDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
}
void impOffFnUseAfterDef_use();
// expected-note@+1 2 {{'#pragma acc routine' for function 'impOffFnUseAfterDef_use_use' appears here}}
#pragma acc routine seq
void impOffFnUseAfterDef_use_use() {
  // expected-note@+1 2 {{'#pragma acc routine seq' implied for function 'impOffFnUseAfterDef_use' by use in function 'impOffFnUseAfterDef_use_use' here}}
  impOffFnUseAfterDef_use();
}
void impOffFnUseAfterDef_use() {
  // expected-note@+1 2 {{'#pragma acc routine seq' implied for function 'impOffFnUseAfterDef' by use in function 'impOffFnUseAfterDef_use' here}}
  impOffFnUseAfterDef();
}

//// Repeat that but discover multiple implicit routine directives at once in a
//// chain.
void chainedImpOffFnUseAfterDef() {
  // expected-error@+1 {{static local variable 's' is not permitted within function 'chainedImpOffFnUseAfterDef' because the latter is attributed with '#pragma acc routine'}}
  static int s;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'chainedImpOffFnUseAfterDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
}
void chainedImpOffFnUseAfterDef_use() {
  // expected-note@+1 2 {{'#pragma acc routine seq' implied for function 'chainedImpOffFnUseAfterDef' by use in function 'chainedImpOffFnUseAfterDef_use' here}}
  chainedImpOffFnUseAfterDef();
}
// expected-note@+1 2 {{'#pragma acc routine' for function 'chainedImpOffFnUseAfterDef_use_use' appears here}}
#pragma acc routine seq
void chainedImpOffFnUseAfterDef_use_use() {
  // expected-note@+1 2 {{'#pragma acc routine seq' implied for function 'chainedImpOffFnUseAfterDef_use' by use in function 'chainedImpOffFnUseAfterDef_use_use' here}}
  chainedImpOffFnUseAfterDef_use();
}

// Does the definition check see the routine directive indirectly implied by a
// use within the same definition?  Check errors before and after that use to be
// sure the mid-stream addition of the routine directive doesn't break either.
void indirectParUseInDef();
void indirectParUseInDef_use() {
  // expected-note@+1 5 {{'#pragma acc routine seq' implied for function 'indirectParUseInDef' by use in function 'indirectParUseInDef_use' here}}
  indirectParUseInDef();
}
void indirectParUseInDef() {
  // expected-error@+1 {{static local variable 'sBefore' is not permitted within function 'indirectParUseInDef' because the latter is attributed with '#pragma acc routine'}}
  static int sBefore;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'indirectParUseInDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{'#pragma acc parallel' is not permitted within function 'indirectParUseInDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel
  // expected-note@+1 5 {{'#pragma acc routine seq' implied for function 'indirectParUseInDef_use' by use in construct '#pragma acc parallel' here}}
  indirectParUseInDef_use(); // routine directive added to current function here
  // expected-error@+1 {{static local variable 'sAfter' is not permitted within function 'indirectParUseInDef' because the latter is attributed with '#pragma acc routine'}}
  static int sAfter;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'indirectParUseInDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
}

//--------------------------------------------------
// Missing declaration possibly within bad context.
//--------------------------------------------------

void missingDeclarationWithinFunctionOnce() {
  // expected-error@+1 {{unexpected OpenACC directive '#pragma acc routine'}}
  #pragma acc routine seq
}

void missingDeclarationWithinFunctionTwice() {
  // expected-error@+1 {{unexpected OpenACC directive '#pragma acc routine'}}
  #pragma acc routine seq
  // expected-error@+1 {{unexpected OpenACC directive '#pragma acc routine'}}
  #pragma acc routine seq
}

struct MissingDeclarationWithinStructOnce {
  int i;
  // expected-error@+1 {{unexpected OpenACC directive '#pragma acc routine'}}
  #pragma acc routine seq
};

struct MissingDeclarationWithinStructTwice {
  int i;
  // expected-error@+1 {{unexpected OpenACC directive '#pragma acc routine'}}
  #pragma acc routine seq
  // expected-error@+1 {{unexpected OpenACC directive '#pragma acc routine'}}
  #pragma acc routine seq
};

// At end of file.
// expected-error@+4 {{'#pragma acc routine' cannot be nested within '#pragma acc routine'}}
// expected-note@+2 {{enclosing '#pragma acc routine' here}}
// expected-error@+1 {{'#pragma acc routine' must be followed by a lone function prototype or definition}}
#pragma acc routine seq
#pragma acc routine seq
