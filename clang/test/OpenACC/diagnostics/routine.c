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

// Generate unique function name based on the line number to make it easier to
// avoid diagnostics about multiple routine directives for the same function.
#define UNIQUE_NAME CONCAT2(unique_fn_, __LINE__)
#define CONCAT2(X, Y) CONCAT(X, Y)
#define CONCAT(X, Y) X##Y

//--------------------------------------------------
// Missing clauses
//--------------------------------------------------

// expected-error@+1 {{expected 'gang', 'worker', 'vector', or 'seq' clause for '#pragma acc routine'}}
#pragma acc routine
void UNIQUE_NAME();

// Any one of those clauses is sufficient to suppress that diagnostic.
#pragma acc routine gang
void UNIQUE_NAME();

// Any one of those clauses is sufficient to suppress that diagnostic.
#pragma acc routine worker
void UNIQUE_NAME();

// Any one of those clauses is sufficient to suppress that diagnostic.
#pragma acc routine vector
void UNIQUE_NAME();

// Any one of those clauses is sufficient to suppress that diagnostic.
#pragma acc routine seq
void UNIQUE_NAME();

//--------------------------------------------------
// Unrecognized clauses
//--------------------------------------------------

// Bogus clauses.

// expected-warning@+2 {{extra tokens at the end of '#pragma acc routine' are ignored}}
// expected-error@+1 {{expected 'gang', 'worker', 'vector', or 'seq' clause for '#pragma acc routine'}}
#pragma acc routine bogusClause0
void bogusClause0();
// expected-warning@+1 {{extra tokens at the end of '#pragma acc routine' are ignored}}
#pragma acc routine seq bogusClause1 bar
void bogusClause1();

// Well formed clauses not permitted here.  Make sure every one is an error.

// expected-error@+9 {{unexpected OpenACC clause 'nomap' in directive '#pragma acc routine'}}
// expected-error@+8 {{unexpected OpenACC clause 'present' in directive '#pragma acc routine'}}
// expected-error@+7 {{unexpected OpenACC clause 'copy' in directive '#pragma acc routine'}}
// expected-error@+6 {{unexpected OpenACC clause 'copyin' in directive '#pragma acc routine'}}
// expected-error@+5 {{unexpected OpenACC clause 'copyout' in directive '#pragma acc routine'}}
// expected-error@+4 {{unexpected OpenACC clause 'create' in directive '#pragma acc routine'}}
// expected-error@+3 {{unexpected OpenACC clause 'no_create' in directive '#pragma acc routine'}}
// expected-error@+2 {{unexpected OpenACC clause 'delete' in directive '#pragma acc routine'}}
// expected-error@+1 {{expected 'gang', 'worker', 'vector', or 'seq' clause for '#pragma acc routine'}}
#pragma acc routine nomap(i) present(i) copy(i) copyin(i) copyout(i) create(i) no_create(i) delete(i)
void UNIQUE_NAME();
// expected-error@+4 {{unexpected OpenACC clause 'pcopy' in directive '#pragma acc routine'}}
// expected-error@+3 {{unexpected OpenACC clause 'pcopyin' in directive '#pragma acc routine'}}
// expected-error@+2 {{unexpected OpenACC clause 'pcopyout' in directive '#pragma acc routine'}}
// expected-error@+1 {{unexpected OpenACC clause 'pcreate' in directive '#pragma acc routine'}}
#pragma acc routine seq pcopy(i) pcopyin(i) pcopyout(i) pcreate(i)
void UNIQUE_NAME();
// expected-error@+4 {{unexpected OpenACC clause 'present_or_copy' in directive '#pragma acc routine'}}
// expected-error@+3 {{unexpected OpenACC clause 'present_or_copyin' in directive '#pragma acc routine'}}
// expected-error@+2 {{unexpected OpenACC clause 'present_or_copyout' in directive '#pragma acc routine'}}
// expected-error@+1 {{unexpected OpenACC clause 'present_or_create' in directive '#pragma acc routine'}}
#pragma acc routine present_or_copy(i) present_or_copyin(i) present_or_copyout(i) present_or_create(i) seq
void UNIQUE_NAME();
// expected-error@+4 {{unexpected OpenACC clause 'shared' in directive '#pragma acc routine'}}
// expected-error@+3 {{unexpected OpenACC clause 'reduction' in directive '#pragma acc routine'}}
// expected-error@+2 {{unexpected OpenACC clause 'private' in directive '#pragma acc routine'}}
// expected-error@+1 {{unexpected OpenACC clause 'firstprivate' in directive '#pragma acc routine'}}
#pragma acc routine shared(i, jk) reduction(+:i,jk,f) seq private(i) firstprivate(i)
void UNIQUE_NAME();
// expected-error@+6 {{unexpected OpenACC clause 'if' in directive '#pragma acc routine'}}
// expected-error@+5 {{unexpected OpenACC clause 'if_present' in directive '#pragma acc routine'}}
// expected-error@+4 {{unexpected OpenACC clause 'self' in directive '#pragma acc routine'}}
// expected-error@+3 {{unexpected OpenACC clause 'host' in directive '#pragma acc routine'}}
// expected-error@+2 {{unexpected OpenACC clause 'device' in directive '#pragma acc routine'}}
// expected-error@+1 {{expected 'gang', 'worker', 'vector', or 'seq' clause for '#pragma acc routine'}}
#pragma acc routine if(1) if_present self(i) host(jk) device(f)
void UNIQUE_NAME();
// expected-error@+3 {{unexpected OpenACC clause 'num_gangs' in directive '#pragma acc routine'}}
// expected-error@+2 {{unexpected OpenACC clause 'num_workers' in directive '#pragma acc routine'}}
// expected-error@+1 {{unexpected OpenACC clause 'vector_length' in directive '#pragma acc routine'}}
#pragma acc routine seq num_gangs(1) num_workers(2) vector_length(3)
void UNIQUE_NAME();
// expected-error@+6 {{unexpected OpenACC clause 'independent' in directive '#pragma acc routine'}}
// expected-error@+5 {{unexpected OpenACC clause 'auto' in directive '#pragma acc routine'}}
// expected-error@+4 {{unexpected OpenACC clause 'auto', 'independent' is specified already}}
// expected-error@+3 {{unexpected OpenACC clause 'collapse' in directive '#pragma acc routine'}}
// expected-error@+2 {{unexpected OpenACC clause 'seq', 'independent' is specified already}}
// expected-error@+1 {{expected 'gang', 'worker', 'vector', or 'seq' clause for '#pragma acc routine'}}
#pragma acc routine independent auto collapse(1) seq
void UNIQUE_NAME();

// Malformed clauses not permitted here.

// expected-error@+3 {{unexpected OpenACC clause 'shared' in directive '#pragma acc routine'}}
// expected-error@+2 {{expected '(' after 'shared'}}
// expected-error@+1 {{expected 'gang', 'worker', 'vector', or 'seq' clause for '#pragma acc routine'}}
#pragma acc routine shared
void UNIQUE_NAME();
// expected-error@+4 {{unexpected OpenACC clause 'shared' in directive '#pragma acc routine'}}
// expected-error@+3 {{expected expression}}
// expected-error@+2 {{expected ')'}}
// expected-note@+1 {{to match this '('}}
#pragma acc routine seq shared(
void UNIQUE_NAME();
// expected-error@+4 {{unexpected OpenACC clause 'shared' in directive '#pragma acc routine'}}
// expected-error@+3 {{expected ')'}}
// expected-note@+2 {{to match this '('}}
// expected-error@+1 {{expected 'gang', 'worker', 'vector', or 'seq' clause for '#pragma acc routine'}}
#pragma acc routine shared(i
void UNIQUE_NAME();

//--------------------------------------------------
// Partitioning clauses.
//--------------------------------------------------

// Parse errors.

// expected-warning@+1 {{extra tokens at the end of '#pragma acc routine' are ignored}}
#pragma acc routine gang(
void UNIQUE_NAME();
// expected-warning@+1 {{extra tokens at the end of '#pragma acc routine' are ignored}}
#pragma acc routine gang()
void UNIQUE_NAME();
// expected-warning@+1 {{extra tokens at the end of '#pragma acc routine' are ignored}}
#pragma acc routine gang(i
void UNIQUE_NAME();
// expected-warning@+1 {{extra tokens at the end of '#pragma acc routine' are ignored}}
#pragma acc routine gang(i)
void UNIQUE_NAME();

// expected-warning@+1 {{extra tokens at the end of '#pragma acc routine' are ignored}}
#pragma acc routine worker(
void UNIQUE_NAME();
// expected-warning@+1 {{extra tokens at the end of '#pragma acc routine' are ignored}}
#pragma acc routine worker()
void UNIQUE_NAME();
// expected-warning@+1 {{extra tokens at the end of '#pragma acc routine' are ignored}}
#pragma acc routine worker(i
void UNIQUE_NAME();
// expected-warning@+1 {{extra tokens at the end of '#pragma acc routine' are ignored}}
#pragma acc routine worker(i)
void UNIQUE_NAME();

// expected-warning@+1 {{extra tokens at the end of '#pragma acc routine' are ignored}}
#pragma acc routine vector(
void UNIQUE_NAME();
// expected-warning@+1 {{extra tokens at the end of '#pragma acc routine' are ignored}}
#pragma acc routine vector()
void UNIQUE_NAME();
// expected-warning@+1 {{extra tokens at the end of '#pragma acc routine' are ignored}}
#pragma acc routine vector(i
void UNIQUE_NAME();
// expected-warning@+1 {{extra tokens at the end of '#pragma acc routine' are ignored}}
#pragma acc routine vector(i)
void UNIQUE_NAME();

// expected-warning@+1 {{extra tokens at the end of '#pragma acc routine' are ignored}}
#pragma acc routine seq(
void UNIQUE_NAME();
// expected-warning@+1 {{extra tokens at the end of '#pragma acc routine' are ignored}}
#pragma acc routine seq()
void UNIQUE_NAME();
// expected-warning@+1 {{extra tokens at the end of '#pragma acc routine' are ignored}}
#pragma acc routine seq(i
void UNIQUE_NAME();
// expected-warning@+1 {{extra tokens at the end of '#pragma acc routine' are ignored}}
#pragma acc routine seq(i)
void UNIQUE_NAME();

// Conflicting clauses on the same directive.

// expected-error@+1 {{directive '#pragma acc routine' cannot contain more than one 'gang' clause}}
#pragma acc routine gang gang
void UNIQUE_NAME();

// expected-error@+1 {{directive '#pragma acc routine' cannot contain more than one 'worker' clause}}
#pragma acc routine worker worker
void UNIQUE_NAME();

// expected-error@+1 {{directive '#pragma acc routine' cannot contain more than one 'vector' clause}}
#pragma acc routine vector vector
void UNIQUE_NAME();

// expected-error@+1 {{directive '#pragma acc routine' cannot contain more than one 'seq' clause}}
#pragma acc routine seq seq
void UNIQUE_NAME();

// expected-error@+7 {{unexpected OpenACC clause 'worker', 'gang' is specified already}}
// expected-error@+6 {{unexpected OpenACC clause 'vector', 'gang' is specified already}}
// expected-error@+5 {{unexpected OpenACC clause 'seq', 'gang' is specified already}}
// expected-error@+4 {{directive '#pragma acc routine' cannot contain more than one 'gang' clause}}
// expected-error@+3 {{directive '#pragma acc routine' cannot contain more than one 'worker' clause}}
// expected-error@+2 {{directive '#pragma acc routine' cannot contain more than one 'vector' clause}}
// expected-error@+1 {{directive '#pragma acc routine' cannot contain more than one 'seq' clause}}
#pragma acc routine gang worker vector seq gang worker vector seq
void UNIQUE_NAME();

// expected-error@+7 {{unexpected OpenACC clause 'vector', 'worker' is specified already}}
// expected-error@+6 {{unexpected OpenACC clause 'seq', 'worker' is specified already}}
// expected-error@+5 {{unexpected OpenACC clause 'gang', 'worker' is specified already}}
// expected-error@+4 {{directive '#pragma acc routine' cannot contain more than one 'worker' clause}}
// expected-error@+3 {{directive '#pragma acc routine' cannot contain more than one 'vector' clause}}
// expected-error@+2 {{directive '#pragma acc routine' cannot contain more than one 'seq' clause}}
// expected-error@+1 {{directive '#pragma acc routine' cannot contain more than one 'gang' clause}}
#pragma acc routine worker vector seq gang worker vector seq gang
void UNIQUE_NAME();

// expected-error@+7 {{unexpected OpenACC clause 'seq', 'vector' is specified already}}
// expected-error@+6 {{unexpected OpenACC clause 'gang', 'vector' is specified already}}
// expected-error@+5 {{unexpected OpenACC clause 'worker', 'gang' is specified already}}
// expected-error@+4 {{directive '#pragma acc routine' cannot contain more than one 'vector' clause}}
// expected-error@+3 {{directive '#pragma acc routine' cannot contain more than one 'seq' clause}}
// expected-error@+2 {{directive '#pragma acc routine' cannot contain more than one 'gang' clause}}
// expected-error@+1 {{directive '#pragma acc routine' cannot contain more than one 'worker' clause}}
#pragma acc routine vector seq gang worker vector seq gang worker
void UNIQUE_NAME();

// expected-error@+7 {{unexpected OpenACC clause 'gang', 'seq' is specified already}}
// expected-error@+6 {{unexpected OpenACC clause 'worker', 'gang' is specified already}}
// expected-error@+5 {{unexpected OpenACC clause 'vector', 'gang' is specified already}}
// expected-error@+4 {{directive '#pragma acc routine' cannot contain more than one 'seq' clause}}
// expected-error@+3 {{directive '#pragma acc routine' cannot contain more than one 'gang' clause}}
// expected-error@+2 {{directive '#pragma acc routine' cannot contain more than one 'worker' clause}}
// expected-error@+1 {{directive '#pragma acc routine' cannot contain more than one 'vector' clause}}
#pragma acc routine seq gang worker vector seq gang worker vector
void UNIQUE_NAME();

// Conflicting clauses on different explicit directives.

#pragma acc routine gang
void gangGang();
#pragma acc routine gang
void gangGang();

// expected-note@+1 {{previous '#pragma acc routine' for function 'gangWorker' appears here}}
#pragma acc routine gang
void gangWorker();
// expected-error@+1 {{for function 'gangWorker', 'worker' clause on '#pragma acc routine' conflicts with 'gang' clause on previous '#pragma acc routine'}}
#pragma acc routine worker
void gangWorker();

// expected-note@+1 {{previous '#pragma acc routine' for function 'gangVector' appears here}}
#pragma acc routine gang
void gangVector();
// expected-error@+1 {{for function 'gangVector', 'vector' clause on '#pragma acc routine' conflicts with 'gang' clause on previous '#pragma acc routine'}}
#pragma acc routine vector
void gangVector();

// expected-note@+1 {{previous '#pragma acc routine' for function 'gangSeq' appears here}}
#pragma acc routine gang
void gangSeq();
// expected-error@+1 {{for function 'gangSeq', 'seq' clause on '#pragma acc routine' conflicts with 'gang' clause on previous '#pragma acc routine'}}
#pragma acc routine seq
void gangSeq();

// expected-note@+1 {{previous '#pragma acc routine' for function 'workerGang' appears here}}
#pragma acc routine worker
void workerGang();
// expected-error@+1 {{for function 'workerGang', 'gang' clause on '#pragma acc routine' conflicts with 'worker' clause on previous '#pragma acc routine'}}
#pragma acc routine gang
void workerGang();

#pragma acc routine worker
void workerWorker();
#pragma acc routine worker
void workerWorker();

// expected-note@+1 {{previous '#pragma acc routine' for function 'workerVector' appears here}}
#pragma acc routine worker
void workerVector();
// expected-error@+1 {{for function 'workerVector', 'vector' clause on '#pragma acc routine' conflicts with 'worker' clause on previous '#pragma acc routine'}}
#pragma acc routine vector
void workerVector();

// expected-note@+1 {{previous '#pragma acc routine' for function 'workerSeq' appears here}}
#pragma acc routine worker
void workerSeq();
// expected-error@+1 {{for function 'workerSeq', 'seq' clause on '#pragma acc routine' conflicts with 'worker' clause on previous '#pragma acc routine'}}
#pragma acc routine seq
void workerSeq();

// expected-note@+1 {{previous '#pragma acc routine' for function 'vectorGang' appears here}}
#pragma acc routine vector
void vectorGang();
// expected-error@+1 {{for function 'vectorGang', 'gang' clause on '#pragma acc routine' conflicts with 'vector' clause on previous '#pragma acc routine'}}
#pragma acc routine gang
void vectorGang();

// expected-note@+1 {{previous '#pragma acc routine' for function 'vectorWorker' appears here}}
#pragma acc routine vector
void vectorWorker();
// expected-error@+1 {{for function 'vectorWorker', 'worker' clause on '#pragma acc routine' conflicts with 'vector' clause on previous '#pragma acc routine'}}
#pragma acc routine worker
void vectorWorker();

#pragma acc routine vector
void vectorVector();
#pragma acc routine vector
void vectorVector();

// expected-note@+1 {{previous '#pragma acc routine' for function 'vectorSeq' appears here}}
#pragma acc routine vector
void vectorSeq();
// expected-error@+1 {{for function 'vectorSeq', 'seq' clause on '#pragma acc routine' conflicts with 'vector' clause on previous '#pragma acc routine'}}
#pragma acc routine seq
void vectorSeq();

// expected-note@+1 {{previous '#pragma acc routine' for function 'seqGang' appears here}}
#pragma acc routine seq
void seqGang();
// expected-error@+1 {{for function 'seqGang', 'gang' clause on '#pragma acc routine' conflicts with 'seq' clause on previous '#pragma acc routine'}}
#pragma acc routine gang
void seqGang();

// expected-note@+1 {{previous '#pragma acc routine' for function 'seqWorker' appears here}}
#pragma acc routine seq
void seqWorker();
// expected-error@+1 {{for function 'seqWorker', 'worker' clause on '#pragma acc routine' conflicts with 'seq' clause on previous '#pragma acc routine'}}
#pragma acc routine worker
void seqWorker();

// expected-note@+1 {{previous '#pragma acc routine' for function 'seqVector' appears here}}
#pragma acc routine seq
void seqVector();
// expected-error@+1 {{for function 'seqVector', 'vector' clause on '#pragma acc routine' conflicts with 'seq' clause on previous '#pragma acc routine'}}
#pragma acc routine vector
void seqVector();

#pragma acc routine seq
void seqSeq();
#pragma acc routine seq
void seqSeq();

// Conflicting clauses between explicit directive and previously implied
// directive.  These necessarily include errors that the explicit directives
// appear after uses.  Those errors are more thoroughly tested separately later
// in this file, but we do vary the way they happen some here to check for
// undesirable interactions between the errors.

void impSeqExpGang();
void impSeqExpGang_use() {
  #pragma acc parallel
  // expected-error@+2 {{'#pragma acc routine' is not in scope at use of associated function 'impSeqExpGang'}}
  // expected-note@+1 {{'#pragma acc routine seq' previously implied for function 'impSeqExpGang' by use in construct '#pragma acc parallel' here}}
  impSeqExpGang();
}
// expected-note@+2 {{'#pragma acc routine' for function 'impSeqExpGang' appears here}}
// expected-error@+1 {{for function 'impSeqExpGang', 'gang' clause on '#pragma acc routine' conflicts with 'seq' clause on previous '#pragma acc routine'}}
#pragma acc routine gang
void impSeqExpGang();

void impSeqExpWorker();
void impSeqExpWorker_use() {
  #pragma acc parallel loop
  for (int i = 0; i < 5; ++i) {
    // expected-error@+2 {{'#pragma acc routine' is not in scope at use of associated function 'impSeqExpWorker'}}
    // expected-note@+1 {{'#pragma acc routine seq' previously implied for function 'impSeqExpWorker' by use in construct '#pragma acc parallel loop' here}}
    impSeqExpWorker();
  }
}
// expected-note@+2 {{'#pragma acc routine' for function 'impSeqExpWorker' appears here}}
// expected-error@+1 {{for function 'impSeqExpWorker', 'worker' clause on '#pragma acc routine' conflicts with 'seq' clause on previous '#pragma acc routine'}}
#pragma acc routine worker
void impSeqExpWorker();

void impSeqExpVector();
// expected-note@+1 {{'#pragma acc routine' for function 'impSeqExpVector_use' appears here}}
#pragma acc routine seq
void impSeqExpVector_use() {
  // expected-error@+2 {{'#pragma acc routine' is not in scope at use of associated function 'impSeqExpVector'}}
  // expected-note@+1 {{'#pragma acc routine seq' previously implied for function 'impSeqExpVector' by use in function 'impSeqExpVector_use' here}}
  impSeqExpVector();
}
// expected-note@+2 {{'#pragma acc routine' for function 'impSeqExpVector' appears here}}
// expected-error@+1 {{for function 'impSeqExpVector', 'vector' clause on '#pragma acc routine' conflicts with 'seq' clause on previous '#pragma acc routine'}}
#pragma acc routine vector
void impSeqExpVector();

void impSeqExpSeq();
void impSeqExpSeq_use() {
  #pragma acc parallel
  // expected-error@+1 {{'#pragma acc routine' is not in scope at use of associated function 'impSeqExpSeq'}}
  impSeqExpSeq();
}
// expected-note@+1 {{'#pragma acc routine' for function 'impSeqExpSeq' appears here}}
#pragma acc routine seq
void impSeqExpSeq();

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
void accUseBefore_accUses3() {
  // Make sure the search for uses traverses into the child construct.
  #pragma acc parallel loop
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc routine' is not in scope at use of associated function 'accUseBefore'}}
    accUseBefore();
    // expected-error@+1 {{'#pragma acc routine' is not in scope at use of associated function 'accUseBefore'}}
    void (*p)() = accUseBefore;
  }
}
void accUseBefore_accUses4() {
  // Make sure the search for uses traverses into the child construct.
  #pragma acc parallel
  #pragma acc loop
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc routine' is not in scope at use of associated function 'accUseBefore'}}
    accUseBefore();
    // expected-error@+1 {{'#pragma acc routine' is not in scope at use of associated function 'accUseBefore'}}
    void (*p)() = accUseBefore;
  }
}
// expected-note@+1 8 {{'#pragma acc routine' for function 'accUseBefore' appears here}}
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

// If some uses appear within broken and discarded sections of the AST and some
// don't, only the latter uses are reported even if one of the former is an
// offload use for which Clang saved its location for the routine directive it
// implies (see next example for how that might be reported instead).
void someUsesInErrorBefore();
void someUsesInErrorBefore_use() {
  // This use isn't reported even though it implied a routine directive.
  // expected-error@+2 {{statement after '#pragma acc parallel loop' must be a for loop}}
  #pragma acc parallel loop
  someUsesInErrorBefore();
  // This use is reported.
  // expected-error@+1 {{'#pragma acc routine' is not in scope at use of associated function 'someUsesInErrorBefore'}}
  someUsesInErrorBefore();
  // This use isn't reported.
  // expected-error@+2 {{statement after '#pragma acc parallel loop' must be a for loop}}
  #pragma acc parallel loop
  someUsesInErrorBefore();
}
// expected-note@+1 {{'#pragma acc routine' for function 'someUsesInErrorBefore' appears here}}
#pragma acc routine seq
void someUsesInErrorBefore();

// If all uses appear within broken and discarded sections of the AST but at
// least one is an offload use, then the first offload use is reported because
// Clang saved its location for the routine directive it implies.
void accUseInErrorBefore();
void accUseInErrorBefore_use() {
  // expected-error@+3 {{statement after '#pragma acc parallel loop' must be a for loop}}
  // expected-error@+2 {{'#pragma acc routine' is not in scope at use of associated function 'accUseInErrorBefore'}}
  #pragma acc parallel loop
  accUseInErrorBefore();
  // expected-error@+2 {{statement after '#pragma acc parallel loop' must be a for loop}}
  #pragma acc parallel loop
  accUseInErrorBefore();
}
// expected-note@+1 {{'#pragma acc routine' for function 'accUseInErrorBefore' appears here}}
#pragma acc routine seq
void accUseInErrorBefore();

// If all uses appear within broken and discarded sections of the AST and are
// host-only uses, then Clang can report that there are uses but cannot report
// any of their locations.
void hostUseInErrorBefore();
void hostUseInErrorBefore_use() {
  // expected-error@+2 {{orphaned '#pragma acc loop' is not supported}}
  // expected-error@+2 {{statement after '#pragma acc loop' must be a for loop}}
  #pragma acc loop
  hostUseInErrorBefore();
  // expected-error@+2 {{orphaned '#pragma acc loop' is not supported}}
  // expected-error@+2 {{statement after '#pragma acc loop' must be a for loop}}
  #pragma acc loop
  hostUseInErrorBefore();
}
// expected-error@+2 {{'#pragma acc routine' is not in scope at use of associated function 'hostUseInErrorBefore'}}
// expected-note@+1 {{cannot report locations of uses of function 'hostUseInErrorBefore' due to previous errors}}
#pragma acc routine seq
void hostUseInErrorBefore();

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
