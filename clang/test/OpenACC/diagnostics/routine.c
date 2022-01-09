// Check diagnostics for "acc routine".

// OpenACC disabled
// RUN: %clang_cc1 -verify=noacc %s

// OpenACC enabled
// RUN: %clang_cc1 -verify=expected -fopenacc %s

// END.

// noacc-no-diagnostics

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
// Restrictions on the function definition.
//--------------------------------------------------

// Does the definition check see the attached routine directive?
// expected-note@+1 7 {{function 'onDef' attributed with '#pragma acc routine' here}}
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
  ;
  // expected-error@+1 {{'#pragma acc parallel' is not permitted within function 'onDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel
  ;
  // expected-error@+1 {{'#pragma acc parallel loop' is not permitted within function 'onDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel loop seq
  for (int i = 0; i < 5; ++i)
  ;
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
}

// Does the definition check see the routine directive on the preceding decl?
// expected-note@+1 7 {{function 'onDeclDef' attributed with '#pragma acc routine' here}}
#pragma acc routine seq
void onDeclDef();
void onDeclDef() {
  // expected-error@+1 {{static local variable 's' is not permitted within function 'onDeclDef' because the latter is attributed with '#pragma acc routine'}}
  static int s;
  int i;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'onDeclDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  // expected-error@+1 {{'#pragma acc enter data' is not permitted within function 'onDeclDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc enter data copyin(i)
  // expected-error@+1 {{'#pragma acc exit data' is not permitted within function 'onDeclDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc exit data copyout(i)
  // expected-error@+1 {{'#pragma acc data' is not permitted within function 'onDeclDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc data copy(i)
  ;
  // expected-error@+1 {{'#pragma acc parallel' is not permitted within function 'onDeclDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel
  ;
  // expected-error@+1 {{'#pragma acc parallel loop' is not permitted within function 'onDeclDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel loop seq
  for (int i = 0; i < 5; ++i)
  ;
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
  int i;
  #pragma acc update device(i)
  #pragma acc enter data copyin(i)
  #pragma acc exit data copyout(i)
  #pragma acc data copy(i)
  ;
  #pragma acc parallel
  ;
  #pragma acc parallel loop seq
  for (int i = 0; i < 5; ++i)
  ;
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
}
// expected-note@+1 {{function 'defOnDecl' attributed with '#pragma acc routine' here}}
#pragma acc routine seq
void defOnDecl();

// Do we avoid repeated diagnostics?
// expected-note@+1 7 {{function 'onDeclOnDef' attributed with '#pragma acc routine' here}}
#pragma acc routine seq
void onDeclOnDef();
#pragma acc routine seq
void onDeclOnDef() {
  // expected-error@+1 {{static local variable 's' is not permitted within function 'onDeclOnDef' because the latter is attributed with '#pragma acc routine'}}
  static int s;
  int i;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'onDeclOnDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  // expected-error@+1 {{'#pragma acc enter data' is not permitted within function 'onDeclOnDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc enter data copyin(i)
  // expected-error@+1 {{'#pragma acc exit data' is not permitted within function 'onDeclOnDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc exit data copyout(i)
  // expected-error@+1 {{'#pragma acc data' is not permitted within function 'onDeclOnDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc data copy(i)
  ;
  // expected-error@+1 {{'#pragma acc parallel' is not permitted within function 'onDeclOnDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel
  ;
  // expected-error@+1 {{'#pragma acc parallel loop' is not permitted within function 'onDeclOnDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel loop seq
  for (int i = 0; i < 5; ++i)
  ;
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
}

// Do we avoid repeated diagnostics?
// expected-note@+1 7 {{function 'onDefOnDecl' attributed with '#pragma acc routine' here}}
#pragma acc routine seq
void onDefOnDecl() {
  // expected-error@+1 {{static local variable 's' is not permitted within function 'onDefOnDecl' because the latter is attributed with '#pragma acc routine'}}
  static int s;
  int i;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'onDefOnDecl' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  // expected-error@+1 {{'#pragma acc enter data' is not permitted within function 'onDefOnDecl' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc enter data copyin(i)
  // expected-error@+1 {{'#pragma acc exit data' is not permitted within function 'onDefOnDecl' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc exit data copyout(i)
  // expected-error@+1 {{'#pragma acc data' is not permitted within function 'onDefOnDecl' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc data copy(i)
  ;
  // expected-error@+1 {{'#pragma acc parallel' is not permitted within function 'onDefOnDecl' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel
  ;
  // expected-error@+1 {{'#pragma acc parallel loop' is not permitted within function 'onDefOnDecl' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel loop seq
  for (int i = 0; i < 5; ++i)
  ;
  // expected-error@+1 {{orphaned '#pragma acc loop' is not supported}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
}
#pragma acc routine seq
void onDefOnDecl();

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
