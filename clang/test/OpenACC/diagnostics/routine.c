// Check diagnostics for "acc routine".

// RUN: %data {
// RUN:   (cflags=-DCASE=CASE_WITHOUT_TRANSFORM)
// RUN:   (cflags=-DCASE=CASE_WITH_TRANSFORM_0)
// RUN:   (cflags=-DCASE=CASE_WITH_TRANSFORM_1)
// RUN:   (cflags=-DCASE=CASE_WITH_TRANSFORM_2)
// RUN:   (cflags=-DCASE=CASE_WITH_TRANSFORM_3)
// RUN: }

// OpenACC disabled
// RUN: %for {
// RUN:   %clang_cc1 -verify=noacc -Wno-gnu-alignof-expression %[cflags] %s
// RUN: }

// OpenACC enabled
// RUN: %for {
// RUN:   %clang_cc1 -verify=expected -fopenacc -Wno-gnu-alignof-expression \
// RUN:     %[cflags] %s
// RUN: }

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

#define CASE_WITHOUT_TRANSFORM 1
#define CASE_WITH_TRANSFORM_0  2
#define CASE_WITH_TRANSFORM_1  3
#define CASE_WITH_TRANSFORM_2  4
#define CASE_WITH_TRANSFORM_3  5

//##############################################################################
// Diagnostic checks where it's fine that other diagnostics suppress all
// transformation of OpenACC to OpenMP.
//##############################################################################

#if CASE == CASE_WITHOUT_TRANSFORM

//------------------------------------------------------------------------------
// Missing clauses
//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------
// Unrecognized clauses
//------------------------------------------------------------------------------

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
// Note that it does not complain that seq conflicts with independent (as on acc
// loop) because the latter isn't permitted here at all.
// expected-error@+3 {{unexpected OpenACC clause 'independent' in directive '#pragma acc routine'}}
// expected-error@+2 {{unexpected OpenACC clause 'auto' in directive '#pragma acc routine'}}
// expected-error@+1 {{unexpected OpenACC clause 'collapse' in directive '#pragma acc routine'}}
#pragma acc routine independent auto collapse(1) seq
void UNIQUE_NAME();
// expected-error@+5 {{unexpected OpenACC clause 'read' in directive '#pragma acc routine'}}
// expected-error@+4 {{unexpected OpenACC clause 'write' in directive '#pragma acc routine'}}
// expected-error@+3 {{unexpected OpenACC clause 'update' in directive '#pragma acc routine'}}
// expected-error@+2 {{unexpected OpenACC clause 'capture' in directive '#pragma acc routine'}}
// expected-error@+1 {{unexpected OpenACC clause 'compare' in directive '#pragma acc routine'}}
#pragma acc routine read write update capture compare seq
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

//------------------------------------------------------------------------------
// Level-of-parallelism clauses for a single function.
//
// Conflicts between uses of the function and enclosing loops or functions are
// checked later.
//------------------------------------------------------------------------------

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

// expected-error@+10 {{unexpected OpenACC clause 'worker', 'gang' is specified already}}
// expected-error@+9 {{unexpected OpenACC clause 'vector', 'gang' is specified already}}
// expected-error@+8 {{unexpected OpenACC clause 'vector', 'worker' is specified already}}
// expected-error@+7 {{unexpected OpenACC clause 'seq', 'gang' is specified already}}
// expected-error@+6 {{unexpected OpenACC clause 'seq', 'worker' is specified already}}
// expected-error@+5 {{unexpected OpenACC clause 'seq', 'vector' is specified already}}
// expected-error@+4 {{directive '#pragma acc routine' cannot contain more than one 'gang' clause}}
// expected-error@+3 {{directive '#pragma acc routine' cannot contain more than one 'worker' clause}}
// expected-error@+2 {{directive '#pragma acc routine' cannot contain more than one 'vector' clause}}
// expected-error@+1 {{directive '#pragma acc routine' cannot contain more than one 'seq' clause}}
#pragma acc routine gang worker vector seq gang worker vector seq
void UNIQUE_NAME();

// expected-error@+10 {{unexpected OpenACC clause 'vector', 'worker' is specified already}}
// expected-error@+9 {{unexpected OpenACC clause 'seq', 'worker' is specified already}}
// expected-error@+8 {{unexpected OpenACC clause 'seq', 'vector' is specified already}}
// expected-error@+7 {{unexpected OpenACC clause 'gang', 'worker' is specified already}}
// expected-error@+6 {{unexpected OpenACC clause 'gang', 'vector' is specified already}}
// expected-error@+5 {{unexpected OpenACC clause 'gang', 'seq' is specified already}}
// expected-error@+4 {{directive '#pragma acc routine' cannot contain more than one 'worker' clause}}
// expected-error@+3 {{directive '#pragma acc routine' cannot contain more than one 'vector' clause}}
// expected-error@+2 {{directive '#pragma acc routine' cannot contain more than one 'seq' clause}}
// expected-error@+1 {{directive '#pragma acc routine' cannot contain more than one 'gang' clause}}
#pragma acc routine worker vector seq gang worker vector seq gang
void UNIQUE_NAME();

// expected-error@+10 {{unexpected OpenACC clause 'seq', 'vector' is specified already}}
// expected-error@+9 {{unexpected OpenACC clause 'gang', 'vector' is specified already}}
// expected-error@+8 {{unexpected OpenACC clause 'gang', 'seq' is specified already}}
// expected-error@+7 {{unexpected OpenACC clause 'worker', 'vector' is specified already}}
// expected-error@+6 {{unexpected OpenACC clause 'worker', 'seq' is specified already}}
// expected-error@+5 {{unexpected OpenACC clause 'worker', 'gang' is specified already}}
// expected-error@+4 {{directive '#pragma acc routine' cannot contain more than one 'vector' clause}}
// expected-error@+3 {{directive '#pragma acc routine' cannot contain more than one 'seq' clause}}
// expected-error@+2 {{directive '#pragma acc routine' cannot contain more than one 'gang' clause}}
// expected-error@+1 {{directive '#pragma acc routine' cannot contain more than one 'worker' clause}}
#pragma acc routine vector seq gang worker vector seq gang worker
void UNIQUE_NAME();

// expected-error@+10 {{unexpected OpenACC clause 'gang', 'seq' is specified already}}
// expected-error@+9 {{unexpected OpenACC clause 'worker', 'seq' is specified already}}
// expected-error@+8 {{unexpected OpenACC clause 'worker', 'gang' is specified already}}
// expected-error@+7 {{unexpected OpenACC clause 'vector', 'seq' is specified already}}
// expected-error@+6 {{unexpected OpenACC clause 'vector', 'gang' is specified already}}
// expected-error@+5 {{unexpected OpenACC clause 'vector', 'worker' is specified already}}
// expected-error@+4 {{directive '#pragma acc routine' cannot contain more than one 'seq' clause}}
// expected-error@+3 {{directive '#pragma acc routine' cannot contain more than one 'gang' clause}}
// expected-error@+2 {{directive '#pragma acc routine' cannot contain more than one 'worker' clause}}
// expected-error@+1 {{directive '#pragma acc routine' cannot contain more than one 'vector' clause}}
#pragma acc routine seq gang worker vector seq gang worker vector
void UNIQUE_NAME();

// Conflicting clauses on different explicit directives for the same function.

#pragma acc routine gang
void gangGang();
#pragma acc routine gang
void gangGang();

// expected-note@+1 {{previous '#pragma acc routine' for function 'gangWorker' appears here}}
#pragma acc routine gang
void gangWorker();
// expected-error@+1 {{for function 'gangWorker', '#pragma acc routine worker' conflicts with previous '#pragma acc routine gang'}}
#pragma acc routine worker
void gangWorker();

// expected-note@+1 {{previous '#pragma acc routine' for function 'gangVector' appears here}}
#pragma acc routine gang
void gangVector();
// expected-error@+1 {{for function 'gangVector', '#pragma acc routine vector' conflicts with previous '#pragma acc routine gang'}}
#pragma acc routine vector
void gangVector();

// expected-note@+1 {{previous '#pragma acc routine' for function 'gangSeq' appears here}}
#pragma acc routine gang
void gangSeq();
// expected-error@+1 {{for function 'gangSeq', '#pragma acc routine seq' conflicts with previous '#pragma acc routine gang'}}
#pragma acc routine seq
void gangSeq();

// expected-note@+1 {{previous '#pragma acc routine' for function 'workerGang' appears here}}
#pragma acc routine worker
void workerGang();
// expected-error@+1 {{for function 'workerGang', '#pragma acc routine gang' conflicts with previous '#pragma acc routine worker'}}
#pragma acc routine gang
void workerGang();

#pragma acc routine worker
void workerWorker();
#pragma acc routine worker
void workerWorker();

// expected-note@+1 {{previous '#pragma acc routine' for function 'workerVector' appears here}}
#pragma acc routine worker
void workerVector();
// expected-error@+1 {{for function 'workerVector', '#pragma acc routine vector' conflicts with previous '#pragma acc routine worker'}}
#pragma acc routine vector
void workerVector();

// expected-note@+1 {{previous '#pragma acc routine' for function 'workerSeq' appears here}}
#pragma acc routine worker
void workerSeq();
// expected-error@+1 {{for function 'workerSeq', '#pragma acc routine seq' conflicts with previous '#pragma acc routine worker'}}
#pragma acc routine seq
void workerSeq();

// expected-note@+1 {{previous '#pragma acc routine' for function 'vectorGang' appears here}}
#pragma acc routine vector
void vectorGang();
// expected-error@+1 {{for function 'vectorGang', '#pragma acc routine gang' conflicts with previous '#pragma acc routine vector'}}
#pragma acc routine gang
void vectorGang();

// expected-note@+1 {{previous '#pragma acc routine' for function 'vectorWorker' appears here}}
#pragma acc routine vector
void vectorWorker();
// expected-error@+1 {{for function 'vectorWorker', '#pragma acc routine worker' conflicts with previous '#pragma acc routine vector'}}
#pragma acc routine worker
void vectorWorker();

#pragma acc routine vector
void vectorVector();
#pragma acc routine vector
void vectorVector();

// expected-note@+1 {{previous '#pragma acc routine' for function 'vectorSeq' appears here}}
#pragma acc routine vector
void vectorSeq();
// expected-error@+1 {{for function 'vectorSeq', '#pragma acc routine seq' conflicts with previous '#pragma acc routine vector'}}
#pragma acc routine seq
void vectorSeq();

// expected-note@+1 {{previous '#pragma acc routine' for function 'seqGang' appears here}}
#pragma acc routine seq
void seqGang();
// expected-error@+1 {{for function 'seqGang', '#pragma acc routine gang' conflicts with previous '#pragma acc routine seq'}}
#pragma acc routine gang
void seqGang();

// expected-note@+1 {{previous '#pragma acc routine' for function 'seqWorker' appears here}}
#pragma acc routine seq
void seqWorker();
// expected-error@+1 {{for function 'seqWorker', '#pragma acc routine worker' conflicts with previous '#pragma acc routine seq'}}
#pragma acc routine worker
void seqWorker();

// expected-note@+1 {{previous '#pragma acc routine' for function 'seqVector' appears here}}
#pragma acc routine seq
void seqVector();
// expected-error@+1 {{for function 'seqVector', '#pragma acc routine vector' conflicts with previous '#pragma acc routine seq'}}
#pragma acc routine vector
void seqVector();

#pragma acc routine seq
void seqSeq();
#pragma acc routine seq
void seqSeq();

// Conflicting clauses between explicit directive and previously implied
// directive for the same function.  These necessarily include errors that the
// explicit directives appear after uses.  Those errors are more thoroughly
// tested separately later in this file, but we do vary the way they happen some
// here to check for undesirable interactions between the errors.

void impSeqExpGang();
void impSeqExpGang_use() {
  // expected-note@+3 {{use of function 'impSeqExpGang' appears here}}
  // expected-note@+2 {{'#pragma acc routine seq' previously implied for function 'impSeqExpGang' by use in construct '#pragma acc parallel' here}}
  #pragma acc parallel
  impSeqExpGang();
}
// expected-error@+2 {{first '#pragma acc routine' for function 'impSeqExpGang' not in scope at some uses}}
// expected-error@+1 {{for function 'impSeqExpGang', '#pragma acc routine gang' conflicts with previous '#pragma acc routine seq'}}
#pragma acc routine gang
void impSeqExpGang();

void impSeqExpWorker();
void impSeqExpWorker_use() {
  #pragma acc parallel loop
  for (int i = 0; i < 5; ++i) {
    // expected-note@+2 {{use of function 'impSeqExpWorker' appears here}}
    // expected-note@+1 {{'#pragma acc routine seq' previously implied for function 'impSeqExpWorker' by use in construct '#pragma acc parallel loop' here}}
    impSeqExpWorker();
  }
}
// expected-error@+2 {{first '#pragma acc routine' for function 'impSeqExpWorker' not in scope at some uses}}
// expected-error@+1 {{for function 'impSeqExpWorker', '#pragma acc routine worker' conflicts with previous '#pragma acc routine seq'}}
#pragma acc routine worker
void impSeqExpWorker();

void impSeqExpVector();
// expected-note@+1 {{'#pragma acc routine' for function 'impSeqExpVector_use' appears here}}
#pragma acc routine seq
void impSeqExpVector_use() {
  // expected-note@+2 {{use of function 'impSeqExpVector' appears here}}
  // expected-note@+1 {{'#pragma acc routine seq' previously implied for function 'impSeqExpVector' by use in function 'impSeqExpVector_use' here}}
  impSeqExpVector();
}
// expected-error@+2 {{first '#pragma acc routine' for function 'impSeqExpVector' not in scope at some uses}}
// expected-error@+1 {{for function 'impSeqExpVector', '#pragma acc routine vector' conflicts with previous '#pragma acc routine seq'}}
#pragma acc routine vector
void impSeqExpVector();

void impSeqExpSeq();
void impSeqExpSeq_use() {
  // expected-note@+2 {{use of function 'impSeqExpSeq' appears here}}
  #pragma acc parallel
  impSeqExpSeq();
}
// expected-error@+1 {{first '#pragma acc routine' for function 'impSeqExpSeq' not in scope at some uses}}
#pragma acc routine seq
void impSeqExpSeq();

//------------------------------------------------------------------------------
// Context not permitted.
//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------
// Bad associated declaration.
//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------

// Evaluated host uses.
void hostUseBefore();
void hostUseBefore_hostUses() {
  // expected-note@+1 {{use of function 'hostUseBefore' appears here}}
  hostUseBefore();
  // expected-note@+1 {{use of function 'hostUseBefore' appears here}}
  void (*p)() = hostUseBefore;
  #pragma acc data copy(i)
  {
    // expected-note@+1 {{use of function 'hostUseBefore' appears here}}
    hostUseBefore();
    // expected-note@+1 {{use of function 'hostUseBefore' appears here}}
    void (*p)() = hostUseBefore;
  }
}
// expected-error@+1 {{first '#pragma acc routine' for function 'hostUseBefore' not in scope at some uses}}
#pragma acc routine seq
void hostUseBefore();
#pragma acc routine seq // Should note only the first routine directive.
void hostUseBefore();

// Evaluated accelerator uses.
void accUseBefore();
#pragma acc routine seq
void accUseBefore_accUses() {
  // expected-note@+1 {{use of function 'accUseBefore' appears here}}
  accUseBefore();
  // expected-note@+1 {{use of function 'accUseBefore' appears here}}
  void (*p)() = accUseBefore;
}
void accUseBefore_accUses2() {
  #pragma acc parallel
  {
    // expected-note@+1 {{use of function 'accUseBefore' appears here}}
    accUseBefore();
    // expected-note@+1 {{use of function 'accUseBefore' appears here}}
    void (*p)() = accUseBefore;
  }
}
void accUseBefore_accUses3() {
  // Make sure the search for uses traverses into the child construct.
  #pragma acc parallel loop
  for (int i = 0; i < 5; ++i) {
    // expected-note@+1 {{use of function 'accUseBefore' appears here}}
    accUseBefore();
    // expected-note@+1 {{use of function 'accUseBefore' appears here}}
    void (*p)() = accUseBefore;
  }
}
void accUseBefore_accUses4() {
  // Make sure the search for uses traverses into the child construct.
  #pragma acc parallel
  #pragma acc loop
  for (int i = 0; i < 5; ++i) {
    // expected-note@+1 {{use of function 'accUseBefore' appears here}}
    accUseBefore();
    // expected-note@+1 {{use of function 'accUseBefore' appears here}}
    void (*p)() = accUseBefore;
  }
}
// expected-error@+1 {{first '#pragma acc routine' for function 'accUseBefore' not in scope at some uses}}
#pragma acc routine seq
void accUseBefore();
#pragma acc routine seq // Should note only the first routine directive.
void accUseBefore();

// Make sure use checks aren't skipped at a routine directive on a definition as
// opposed to a prototype.
void useBeforeOnDef();
// expected-note@+1 {{use of function 'useBeforeOnDef' appears here}}
void useBeforeOnDef_hostUses() { useBeforeOnDef(); }
// expected-note@+2 {{use of function 'useBeforeOnDef' appears here}}
#pragma acc routine seq
void useBeforeOnDef_accUses() { useBeforeOnDef(); }
// expected-error@+1 {{first '#pragma acc routine' for function 'useBeforeOnDef' not in scope at some uses}}
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
void notAllUsesBefore();
void notAllUsesBefore_hostUses() {
  size_t s = sizeof &notAllUsesBefore;
  // expected-note@+1 {{use of function 'notAllUsesBefore' appears here}}
  notAllUsesBefore();
  size_t a = _Alignof(&notAllUsesBefore);
}
#pragma acc routine seq
void notAllUsesBefore_accUses() {
  // expected-note@+1 {{use of function 'notAllUsesBefore' appears here}}
  notAllUsesBefore();
  size_t a = _Alignof(&notAllUsesBefore);
  // expected-note@+1 {{use of function 'notAllUsesBefore' appears here}}
  void (*p)() = notAllUsesBefore;
}
// expected-error@+1 {{first '#pragma acc routine' for function 'notAllUsesBefore' not in scope at some uses}}
#pragma acc routine seq
void notAllUsesBefore();

// Definition.
// expected-note@+1 {{definition of function 'defBefore' appears here}}
void defBefore() {}
// expected-error@+1 {{first '#pragma acc routine' for function 'defBefore' not in scope at definition}}
#pragma acc routine seq
void defBefore();
#pragma acc routine seq // Should note only the first routine directive.
void defBefore();

// Use errors shouldn't suppress/break later definition errors.
void useDefBefore();
// expected-note@+1 {{use of function 'useDefBefore' appears here}}
void useDefBefore_uses() { useDefBefore(); }
// expected-note@+1 {{definition of function 'useDefBefore' appears here}}
void useDefBefore() {} // def
// expected-error@+2 {{first '#pragma acc routine' for function 'useDefBefore' not in scope at definition}}
// expected-error@+1 {{first '#pragma acc routine' for function 'useDefBefore' not in scope at some uses}}
#pragma acc routine seq
void useDefBefore();
#pragma acc routine seq // Should note only the first routine directive.
void useDefBefore();

// Definition errors shouldn't suppress/break later use errors.
// expected-note@+1 {{definition of function 'defUseBefore' appears here}}
void defUseBefore() {} // def
// expected-note@+1 {{use of function 'defUseBefore' appears here}}
void defUseBefore_uses() { defUseBefore(); }
// expected-error@+2 {{first '#pragma acc routine' for function 'defUseBefore' not in scope at definition}}
// expected-error@+1 {{first '#pragma acc routine' for function 'defUseBefore' not in scope at some uses}}
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
  // expected-note@+1 {{use of function 'someUsesInErrorBefore' appears here}}
  someUsesInErrorBefore();
  // This use isn't reported.
  // expected-error@+2 {{statement after '#pragma acc parallel loop' must be a for loop}}
  #pragma acc parallel loop
  someUsesInErrorBefore();
}
// expected-error@+1 {{first '#pragma acc routine' for function 'someUsesInErrorBefore' not in scope at some uses}}
#pragma acc routine seq
void someUsesInErrorBefore();

// If all uses appear within broken and discarded sections of the AST but at
// least one is an offload use, then the first offload use is reported because
// Clang saved its location for the routine directive it implies.
void accUseInErrorBefore();
void accUseInErrorBefore_use() {
  // expected-error@+3 {{statement after '#pragma acc parallel loop' must be a for loop}}
  // expected-note@+2 {{use of function 'accUseInErrorBefore' appears here}}
  #pragma acc parallel loop
  accUseInErrorBefore();
  // expected-error@+2 {{statement after '#pragma acc parallel loop' must be a for loop}}
  #pragma acc parallel loop
  accUseInErrorBefore();
}
// expected-error@+1 {{first '#pragma acc routine' for function 'accUseInErrorBefore' not in scope at some uses}}
#pragma acc routine seq
void accUseInErrorBefore();

// If all uses appear within broken and discarded sections of the AST and are
// host-only uses, then Clang can report that there are uses but cannot report
// any of their locations.
void hostUseInErrorBefore();
void hostUseInErrorBefore_use() {
  // expected-error@+2 {{function 'hostUseInErrorBefore_use' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
  // expected-error@+2 {{statement after '#pragma acc loop' must be a for loop}}
  #pragma acc loop
  hostUseInErrorBefore();
  // expected-error@+2 {{function 'hostUseInErrorBefore_use' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
  // expected-error@+2 {{statement after '#pragma acc loop' must be a for loop}}
  #pragma acc loop
  hostUseInErrorBefore();
}
// expected-error@+2 {{first '#pragma acc routine' for function 'hostUseInErrorBefore' not in scope at some uses}}
// expected-note@+1 {{cannot report locations of uses of function 'hostUseInErrorBefore' due to previous errors}}
#pragma acc routine seq
void hostUseInErrorBefore();

//------------------------------------------------------------------------------
// Restrictions on the function definition body for explicit routine directives.
//------------------------------------------------------------------------------

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
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
}

// Does the definition check see the routine directive on the following decl?
// No, that's an error.
// expected-note@+1 {{definition of function 'defOnDecl' appears here}}
void defOnDecl() {
  static int s;
  #pragma acc update device(i)
  // expected-error@+1 {{function 'defOnDecl' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
}
// expected-error@+1 {{first '#pragma acc routine' for function 'defOnDecl' not in scope at definition}}
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
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
}
#pragma acc routine seq
void onDefOnDecl();

//------------------------------------------------------------------------------
// Restrictions on the function definition body for implicit routine directives.
//------------------------------------------------------------------------------

// Does the definition check see the routine directive previously implied by a
// use within a parallel construct?
//
// Exhaustively test diagnostic kinds in this and the next case, but choose only
// a single representative of each diagnostic kind afterward.
void parUseBeforeDef();
void parUseBeforeDef_use() {
  #pragma acc parallel
  // expected-note@+1 11 {{'#pragma acc routine seq' implied for function 'parUseBeforeDef' by use in construct '#pragma acc parallel' here}}
  parUseBeforeDef();
}
void parUseBeforeDef() {
  // expected-error@+1 {{static local variable 's' is not permitted within function 'parUseBeforeDef' because the latter is attributed with '#pragma acc routine'}}
  static int s;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'parUseBeforeDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  // expected-error@+1 {{'#pragma acc enter data' is not permitted within function 'parUseBeforeDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc enter data copyin(i)
  // expected-error@+1 {{'#pragma acc exit data' is not permitted within function 'parUseBeforeDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc exit data copyout(i)
  // expected-error@+1 {{'#pragma acc data' is not permitted within function 'parUseBeforeDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc data copy(i)
  {
    // expected-error@+1 {{static local variable 's' is not permitted within function 'parUseBeforeDef' because the latter is attributed with '#pragma acc routine'}}
    static int s;
  }
  // expected-error@+1 {{'#pragma acc parallel' is not permitted within function 'parUseBeforeDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel
  {
    // expected-error@+1 {{static local variable 's' is not permitted within function 'parUseBeforeDef' because the latter is attributed with '#pragma acc routine'}}
    static int s;
  }
  // expected-error@+1 {{'#pragma acc parallel loop' is not permitted within function 'parUseBeforeDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel loop seq
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{static local variable 's' is not permitted within function 'parUseBeforeDef' because the latter is attributed with '#pragma acc routine'}}
    static int s;
  }
  // expected-error@+1 {{function 'parUseBeforeDef' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{static local variable 's' is not permitted within function 'parUseBeforeDef' because the latter is attributed with '#pragma acc routine'}}
    static int s;
  }
  // expected-error@+1 {{function 'parUseBeforeDef' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
  parUseBeforeDef();
}

// Does the definition check see the routine directive later implied by a use
// within a parallel construct?
//
// Exhaustively test diagnostic kinds in this and the previous case, but choose
// only a single representative of each diagnostic kind afterward.
void parUseAfterDef() {
  // expected-error@+1 {{static local variable 's' is not permitted within function 'parUseAfterDef' because the latter is attributed with '#pragma acc routine'}}
  static int s;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'parUseAfterDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  // expected-error@+1 {{'#pragma acc enter data' is not permitted within function 'parUseAfterDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc enter data copyin(i)
  // expected-error@+1 {{'#pragma acc exit data' is not permitted within function 'parUseAfterDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc exit data copyout(i)
  // expected-error@+1 {{'#pragma acc data' is not permitted within function 'parUseAfterDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc data copy(i)
  {
    // expected-error@+1 {{static local variable 's' is not permitted within function 'parUseAfterDef' because the latter is attributed with '#pragma acc routine'}}
    static int s;
  }
  // expected-error@+1 {{'#pragma acc parallel' is not permitted within function 'parUseAfterDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel
  {
    // expected-error@+1 {{static local variable 's' is not permitted within function 'parUseAfterDef' because the latter is attributed with '#pragma acc routine'}}
    static int s;
  }
  // expected-error@+1 {{'#pragma acc parallel loop' is not permitted within function 'parUseAfterDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel loop seq
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{static local variable 's' is not permitted within function 'parUseAfterDef' because the latter is attributed with '#pragma acc routine'}}
    static int s;
  }
  // expected-error@+1 {{function 'parUseAfterDef' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{static local variable 's' is not permitted within function 'parUseAfterDef' because the latter is attributed with '#pragma acc routine'}}
    static int s;
  }
  // expected-error@+1 {{function 'parUseAfterDef' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
  parUseAfterDef();
}
void parUseAfterDef_use() {
  #pragma acc parallel
  // expected-note@+1 11 {{'#pragma acc routine seq' implied for function 'parUseAfterDef' by use in construct '#pragma acc parallel' here}}
  parUseAfterDef();
}

// Does the definition check see the routine directive implied by a use within a
// parallel construct within the same definition?
void parUseInDef() {
  // expected-error@+1 {{static local variable 'sBefore' is not permitted within function 'parUseInDef' because the latter is attributed with '#pragma acc routine'}}
  static int sBefore;
  // expected-error@+1 {{'#pragma acc update' is not permitted within function 'parUseInDef' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc update device(i)
  // expected-error@+1 {{function 'parUseInDef' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
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
  // expected-error@+1 {{function 'parUseInDef' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
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
  // expected-error@+1 {{function 'parLoopUseBeforeDef' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
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
  // expected-error@+1 {{function 'parLoopUseAfterDef' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
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
  // expected-error@+1 {{function 'parLoopUseInDef' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
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
  // expected-error@+1 {{function 'parLoopUseInDef' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
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
  // expected-error@+1 {{function 'expOffFnUseBeforeDef' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
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
  // expected-error@+1 {{function 'expOffFnUseAfterDef' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
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
  // expected-error@+1 {{function 'impOffFnUseBeforeDef' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
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
  // expected-error@+1 {{function 'chainedImpOffFnUseBeforeDef' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
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
  // expected-error@+1 {{function 'impOffFnUseAfterDef' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
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
  // expected-error@+1 {{function 'chainedImpOffFnUseAfterDef' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
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
  // expected-error@+1 {{function 'indirectParUseInDef' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
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
  // expected-error@+1 {{function 'indirectParUseInDef' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
}

//------------------------------------------------------------------------------
// Compatible levels of parallelism.
//------------------------------------------------------------------------------

//..............................................................................
// Usee never has a routine directive, so user cannot either, so their levels of
// parallelism are compatible.

void useeNoDir();
void useeNoDir_user() {
  useeNoDir();
  #pragma acc data copy(i)
  useeNoDir();
}

//..............................................................................
// Usee has no routine directive yet, so its level of parallelism is compatible
// with any use site.

void useeInGangFn();
void useeInWorkerFn();
void useeInVectorFn();
void useeInSeqFn();
void useeInImpLaterFn();
void useeInPar();
void useeInParGangLoop();
void useeInParWorkerLoop();
void useeInParVectorLoop();
void useeInParGangWorkerLoop();
void useeInParGangVectorLoop();
void useeInParWorkerVectorLoop();
void useeInParGangWorkerVectorLoop();
void useeInParSeqLoop();
void useeInParNakedLoop();
void useeInParAutoLoop();
void useeInGangLoop();
void useeInWorkerLoop();
void useeInVectorLoop();
void useeInGangWorkerLoop();
void useeInGangVectorLoop();
void useeInWorkerVectorLoop();
void useeInGangWorkerVectorLoop();
void useeInSeqLoop();
void useeInNakedLoop();
void useeInAutoLoop();

void useeInAny_hostUser() {
  useeInGangFn();
  useeInWorkerFn();
  useeInVectorFn();
  useeInSeqFn();
  useeInImpLaterFn();
  useeInPar();
  useeInParGangLoop();
  useeInParWorkerLoop();
  useeInParVectorLoop();
  useeInParGangWorkerLoop();
  useeInParGangVectorLoop();
  useeInParWorkerVectorLoop();
  useeInParGangWorkerVectorLoop();
  useeInParSeqLoop();
  useeInParNakedLoop();
  useeInParAutoLoop();
  useeInGangLoop();
  useeInWorkerLoop();
  useeInVectorLoop();
  useeInGangWorkerLoop();
  useeInGangVectorLoop();
  useeInWorkerVectorLoop();
  useeInGangWorkerVectorLoop();
  useeInSeqLoop();
  useeInNakedLoop();
  useeInAutoLoop();
}

#pragma acc routine gang
void useeInGangFn_user() { useeInGangFn(); }
#pragma acc routine worker
void useeInWorkerFn_user() { useeInWorkerFn(); }
#pragma acc routine vector
void useeInVectorFn_user() { useeInVectorFn(); }
#pragma acc routine seq
void useeInSeqFn_user() { useeInSeqFn(); }
void useeInImpLaterFn_user() { useeInImpLaterFn(); }
#pragma acc routine seq
void useeInImpLaterFn_user_user() { useeInImpLaterFn_user(); }

void useeInAnyConstruct_parUser() {
  #pragma acc parallel
  {
    useeInPar();
    #pragma acc loop gang
    for (int i = 0; i < 5; ++i)
      useeInParGangLoop();
    #pragma acc loop worker
    for (int i = 0; i < 5; ++i)
      useeInParWorkerLoop();
    #pragma acc loop vector
    for (int i = 0; i < 5; ++i)
      useeInParVectorLoop();
    #pragma acc loop gang worker
    for (int i = 0; i < 5; ++i)
      useeInParGangWorkerLoop();
    #pragma acc loop gang vector
    for (int i = 0; i < 5; ++i)
      useeInParGangVectorLoop();
    #pragma acc loop worker vector
    for (int i = 0; i < 5; ++i)
      useeInParWorkerVectorLoop();
    #pragma acc loop gang worker vector
    for (int i = 0; i < 5; ++i)
      useeInParGangWorkerVectorLoop();
    #pragma acc loop seq
    for (int i = 0; i < 5; ++i)
      useeInParSeqLoop();
    #pragma acc loop
    for (int i = 0; i < 5; ++i)
      useeInParNakedLoop();
    #pragma acc loop auto
    for (int i = 0; i < 5; ++i)
      useeInParAutoLoop();
    #pragma acc loop auto gang
    for (int i = 0; i < 5; ++i)
      useeInParAutoLoop();
    #pragma acc loop auto worker
    for (int i = 0; i < 5; ++i)
      useeInParAutoLoop();
    #pragma acc loop auto vector
    for (int i = 0; i < 5; ++i)
      useeInParAutoLoop();
  }
}

#pragma acc routine gang
void useeInAnyConstruct_orphanedPartLoopUser() {
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    useeInGangLoop();
  #pragma acc loop worker
  for (int i = 0; i < 5; ++i)
    useeInWorkerLoop();
  #pragma acc loop vector
  for (int i = 0; i < 5; ++i)
    useeInVectorLoop();
  #pragma acc loop gang worker
  for (int i = 0; i < 5; ++i)
    useeInGangWorkerLoop();
  #pragma acc loop gang vector
  for (int i = 0; i < 5; ++i)
    useeInGangVectorLoop();
  #pragma acc loop worker vector
  for (int i = 0; i < 5; ++i)
    useeInWorkerVectorLoop();
  #pragma acc loop gang worker vector
  for (int i = 0; i < 5; ++i)
    useeInGangWorkerVectorLoop();
  #pragma acc loop auto gang
  for (int i = 0; i < 5; ++i)
    useeInAutoLoop();
  #pragma acc loop auto worker
  for (int i = 0; i < 5; ++i)
    useeInAutoLoop();
  #pragma acc loop auto vector
  for (int i = 0; i < 5; ++i)
    useeInAutoLoop();
}

#pragma acc routine seq
void useeInAnyConstruct_orphanedUnpartLoopUser() {
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    useeInSeqLoop();
  #pragma acc loop
  for (int i = 0; i < 5; ++i)
    useeInNakedLoop();
  #pragma acc loop auto
  for (int i = 0; i < 5; ++i)
    useeInAutoLoop();
}

//..............................................................................
// Usee has routine directive already.

// expected-note@+1 48 {{'#pragma acc routine' for function 'useeHasGang' appears here}}
#pragma acc routine gang
void useeHasGang();
// expected-note@+1 40 {{'#pragma acc routine' for function 'useeHasWorker' appears here}}
#pragma acc routine worker
void useeHasWorker();
// expected-note@+1 28 {{'#pragma acc routine' for function 'useeHasVector' appears here}}
#pragma acc routine vector
void useeHasVector();
#pragma acc routine seq
void useeHasSeq();

#pragma acc routine gang
void useeHasAny_gangFnUser() {
  useeHasGang();
  useeHasWorker();
  useeHasVector();
  useeHasSeq();
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i) {
    useeHasGang();
    useeHasWorker();
    useeHasVector();
    useeHasSeq();
  }
  #pragma acc loop
  for (int i = 0; i < 5; ++i) {
    useeHasGang();
    useeHasWorker();
    useeHasVector();
    useeHasSeq();
  }
  #pragma acc loop auto
  for (int i = 0; i < 5; ++i) {
    useeHasGang();
    useeHasWorker();
    useeHasVector();
    useeHasSeq();
  }
}
// expected-note@+1 4 {{'#pragma acc routine' for function 'useeHasAny_workerFnUser' appears here}}
#pragma acc routine worker
void useeHasAny_workerFnUser() {
  // expected-error@+1 {{function 'useeHasAny_workerFnUser' has '#pragma acc routine worker' but calls function 'useeHasGang', which has '#pragma acc routine gang'}}
  useeHasGang();
  useeHasWorker();
  useeHasVector();
  useeHasSeq();
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{function 'useeHasAny_workerFnUser' has '#pragma acc routine worker' but calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    useeHasWorker();
    useeHasVector();
    useeHasSeq();
  }
  #pragma acc loop
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{function 'useeHasAny_workerFnUser' has '#pragma acc routine worker' but calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    useeHasWorker();
    useeHasVector();
    useeHasSeq();
  }
  #pragma acc loop auto
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{function 'useeHasAny_workerFnUser' has '#pragma acc routine worker' but calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    useeHasWorker();
    useeHasVector();
    useeHasSeq();
  }
}
// expected-note@+1 8 {{'#pragma acc routine' for function 'useeHasAny_vectorFnUser' appears here}}
#pragma acc routine vector
void useeHasAny_vectorFnUser() {
  // expected-error@+1 {{function 'useeHasAny_vectorFnUser' has '#pragma acc routine vector' but calls function 'useeHasGang', which has '#pragma acc routine gang'}}
  useeHasGang();
  // expected-error@+1 {{function 'useeHasAny_vectorFnUser' has '#pragma acc routine vector' but calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
  useeHasWorker();
  useeHasVector();
  useeHasSeq();
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{function 'useeHasAny_vectorFnUser' has '#pragma acc routine vector' but calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    // expected-error@+1 {{function 'useeHasAny_vectorFnUser' has '#pragma acc routine vector' but calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
    useeHasWorker();
    useeHasVector();
    useeHasSeq();
  }
  #pragma acc loop
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{function 'useeHasAny_vectorFnUser' has '#pragma acc routine vector' but calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    // expected-error@+1 {{function 'useeHasAny_vectorFnUser' has '#pragma acc routine vector' but calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
    useeHasWorker();
    useeHasVector();
    useeHasSeq();
  }
  #pragma acc loop auto
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{function 'useeHasAny_vectorFnUser' has '#pragma acc routine vector' but calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    // expected-error@+1 {{function 'useeHasAny_vectorFnUser' has '#pragma acc routine vector' but calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
    useeHasWorker();
    useeHasVector();
    useeHasSeq();
  }
}
// expected-note@+1 12 {{'#pragma acc routine' for function 'useeHasAny_seqFnUser' appears here}}
#pragma acc routine seq
void useeHasAny_seqFnUser() {
  // expected-error@+1 {{function 'useeHasAny_seqFnUser' has '#pragma acc routine seq' but calls function 'useeHasGang', which has '#pragma acc routine gang'}}
  useeHasGang();
  // expected-error@+1 {{function 'useeHasAny_seqFnUser' has '#pragma acc routine seq' but calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
  useeHasWorker();
  // expected-error@+1 {{function 'useeHasAny_seqFnUser' has '#pragma acc routine seq' but calls function 'useeHasVector', which has '#pragma acc routine vector'}}
  useeHasVector();
  useeHasSeq();
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{function 'useeHasAny_seqFnUser' has '#pragma acc routine seq' but calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    // expected-error@+1 {{function 'useeHasAny_seqFnUser' has '#pragma acc routine seq' but calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
    useeHasWorker();
    // expected-error@+1 {{function 'useeHasAny_seqFnUser' has '#pragma acc routine seq' but calls function 'useeHasVector', which has '#pragma acc routine vector'}}
    useeHasVector();
    useeHasSeq();
  }
  #pragma acc loop
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{function 'useeHasAny_seqFnUser' has '#pragma acc routine seq' but calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    // expected-error@+1 {{function 'useeHasAny_seqFnUser' has '#pragma acc routine seq' but calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
    useeHasWorker();
    // expected-error@+1 {{function 'useeHasAny_seqFnUser' has '#pragma acc routine seq' but calls function 'useeHasVector', which has '#pragma acc routine vector'}}
    useeHasVector();
    useeHasSeq();
  }
  #pragma acc loop auto
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{function 'useeHasAny_seqFnUser' has '#pragma acc routine seq' but calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    // expected-error@+1 {{function 'useeHasAny_seqFnUser' has '#pragma acc routine seq' but calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
    useeHasWorker();
    // expected-error@+1 {{function 'useeHasAny_seqFnUser' has '#pragma acc routine seq' but calls function 'useeHasVector', which has '#pragma acc routine vector'}}
    useeHasVector();
    useeHasSeq();
  }
}
void useeHasAny_impLaterFnUser() {
  // expected-error@+1 {{function 'useeHasAny_impLaterFnUser' has no explicit '#pragma acc routine' but calls function 'useeHasGang', which has '#pragma acc routine gang'}}
  useeHasGang();
  // expected-error@+1 {{function 'useeHasAny_impLaterFnUser' has no explicit '#pragma acc routine' but calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
  useeHasWorker();
  // expected-error@+1 {{function 'useeHasAny_impLaterFnUser' has no explicit '#pragma acc routine' but calls function 'useeHasVector', which has '#pragma acc routine vector'}}
  useeHasVector();
  useeHasSeq();
  // expected-error@+1 {{function 'useeHasAny_impLaterFnUser' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{function 'useeHasAny_impLaterFnUser' has no explicit '#pragma acc routine' but calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    // expected-error@+1 {{function 'useeHasAny_impLaterFnUser' has no explicit '#pragma acc routine' but calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
    useeHasWorker();
    // expected-error@+1 {{function 'useeHasAny_impLaterFnUser' has no explicit '#pragma acc routine' but calls function 'useeHasVector', which has '#pragma acc routine vector'}}
    useeHasVector();
    useeHasSeq();
  }
  // expected-error@+1 {{function 'useeHasAny_impLaterFnUser' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
  #pragma acc loop
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{function 'useeHasAny_impLaterFnUser' has no explicit '#pragma acc routine' but calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    // expected-error@+1 {{function 'useeHasAny_impLaterFnUser' has no explicit '#pragma acc routine' but calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
    useeHasWorker();
    // expected-error@+1 {{function 'useeHasAny_impLaterFnUser' has no explicit '#pragma acc routine' but calls function 'useeHasVector', which has '#pragma acc routine vector'}}
    useeHasVector();
    useeHasSeq();
  }
  // expected-error@+1 {{function 'useeHasAny_impLaterFnUser' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
  #pragma acc loop auto
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{function 'useeHasAny_impLaterFnUser' has no explicit '#pragma acc routine' but calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    // expected-error@+1 {{function 'useeHasAny_impLaterFnUser' has no explicit '#pragma acc routine' but calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
    useeHasWorker();
    // expected-error@+1 {{function 'useeHasAny_impLaterFnUser' has no explicit '#pragma acc routine' but calls function 'useeHasVector', which has '#pragma acc routine vector'}}
    useeHasVector();
    useeHasSeq();
  }
}
#pragma acc routine seq
void useeHasAny_impLaterFnUser_user() {
  useeHasAny_impLaterFnUser();
}
void useeHasAny_hostFnUser() {
  // expected-error@+1 {{function 'useeHasAny_hostFnUser' has no explicit '#pragma acc routine' but calls function 'useeHasGang', which has '#pragma acc routine gang'}}
  useeHasGang();
  // expected-error@+1 {{function 'useeHasAny_hostFnUser' has no explicit '#pragma acc routine' but calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
  useeHasWorker();
  // expected-error@+1 {{function 'useeHasAny_hostFnUser' has no explicit '#pragma acc routine' but calls function 'useeHasVector', which has '#pragma acc routine vector'}}
  useeHasVector();
  useeHasSeq();
  // expected-error@+1 {{function 'useeHasAny_hostFnUser' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{function 'useeHasAny_hostFnUser' has no explicit '#pragma acc routine' but calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    // expected-error@+1 {{function 'useeHasAny_hostFnUser' has no explicit '#pragma acc routine' but calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
    useeHasWorker();
    // expected-error@+1 {{function 'useeHasAny_hostFnUser' has no explicit '#pragma acc routine' but calls function 'useeHasVector', which has '#pragma acc routine vector'}}
    useeHasVector();
    useeHasSeq();
  }
  // expected-error@+1 {{function 'useeHasAny_hostFnUser' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
  #pragma acc loop
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{function 'useeHasAny_hostFnUser' has no explicit '#pragma acc routine' but calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    // expected-error@+1 {{function 'useeHasAny_hostFnUser' has no explicit '#pragma acc routine' but calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
    useeHasWorker();
    // expected-error@+1 {{function 'useeHasAny_hostFnUser' has no explicit '#pragma acc routine' but calls function 'useeHasVector', which has '#pragma acc routine vector'}}
    useeHasVector();
    useeHasSeq();
  }
  // expected-error@+1 {{function 'useeHasAny_hostFnUser' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
  #pragma acc loop auto
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{function 'useeHasAny_hostFnUser' has no explicit '#pragma acc routine' but calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    // expected-error@+1 {{function 'useeHasAny_hostFnUser' has no explicit '#pragma acc routine' but calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
    useeHasWorker();
    // expected-error@+1 {{function 'useeHasAny_hostFnUser' has no explicit '#pragma acc routine' but calls function 'useeHasVector', which has '#pragma acc routine vector'}}
    useeHasVector();
    useeHasSeq();
  }
}

void useeHasAny_parUser() {
  #pragma acc parallel
  {
    useeHasGang();
    useeHasWorker();
    useeHasVector();
    useeHasSeq();
    // expected-note@+1 {{enclosing '#pragma acc loop' here}}
    #pragma acc loop gang
    for (int i = 0; i < 5; ++i) {
      // expected-error@+1 {{'#pragma acc loop gang' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
      useeHasGang();
      useeHasWorker();
      useeHasVector();
      useeHasSeq();
    }
    // expected-note@+1 2 {{enclosing '#pragma acc loop' here}}
    #pragma acc loop worker
    for (int i = 0; i < 5; ++i) {
      // expected-error@+1 {{'#pragma acc loop worker' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
      useeHasGang();
      // expected-error@+1 {{'#pragma acc loop worker' construct calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
      useeHasWorker();
      useeHasVector();
      useeHasSeq();
    }
    // expected-note@+1 3 {{enclosing '#pragma acc loop' here}}
    #pragma acc loop vector
    for (int i = 0; i < 5; ++i) {
      // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
      useeHasGang();
      // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
      useeHasWorker();
      // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasVector', which has '#pragma acc routine vector'}}
      useeHasVector();
      useeHasSeq();
    }
    // expected-note@+1 2 {{enclosing '#pragma acc loop' here}}
    #pragma acc loop gang worker
    for (int i = 0; i < 5; ++i) {
      // expected-error@+1 {{'#pragma acc loop worker' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
      useeHasGang();
      // expected-error@+1 {{'#pragma acc loop worker' construct calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
      useeHasWorker();
      useeHasVector();
      useeHasSeq();
    }
    // expected-note@+1 3 {{enclosing '#pragma acc loop' here}}
    #pragma acc loop gang vector
    for (int i = 0; i < 5; ++i) {
      // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
      useeHasGang();
      // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
      useeHasWorker();
      // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasVector', which has '#pragma acc routine vector'}}
      useeHasVector();
      useeHasSeq();
    }
    // expected-note@+1 3 {{enclosing '#pragma acc loop' here}}
    #pragma acc loop worker vector
    for (int i = 0; i < 5; ++i) {
      // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
      useeHasGang();
      // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
      useeHasWorker();
      // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasVector', which has '#pragma acc routine vector'}}
      useeHasVector();
      useeHasSeq();
    }
    // expected-note@+1 3 {{enclosing '#pragma acc loop' here}}
    #pragma acc loop gang worker vector
    for (int i = 0; i < 5; ++i) {
      // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
      useeHasGang();
      // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
      useeHasWorker();
      // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasVector', which has '#pragma acc routine vector'}}
      useeHasVector();
      useeHasSeq();
    }
    #pragma acc loop seq
    for (int i = 0; i < 5; ++i) {
      useeHasGang();
      useeHasWorker();
      useeHasVector();
      useeHasSeq();
    }
    #pragma acc loop
    for (int i = 0; i < 5; ++i) {
      useeHasGang();
      useeHasWorker();
      useeHasVector();
      useeHasSeq();
    }
    #pragma acc loop auto
    for (int i = 0; i < 5; ++i) {
      useeHasGang();
      useeHasWorker();
      useeHasVector();
      useeHasSeq();
    }
    // expected-note@+1 {{enclosing '#pragma acc loop' here}}
    #pragma acc loop auto gang
    for (int i = 0; i < 5; ++i) {
      // expected-error@+1 {{'#pragma acc loop gang' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
      useeHasGang();
      useeHasWorker();
      useeHasVector();
      useeHasSeq();
    }
    // expected-note@+1 2 {{enclosing '#pragma acc loop' here}}
    #pragma acc loop auto worker
    for (int i = 0; i < 5; ++i) {
      // expected-error@+1 {{'#pragma acc loop worker' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
      useeHasGang();
      // expected-error@+1 {{'#pragma acc loop worker' construct calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
      useeHasWorker();
      useeHasVector();
      useeHasSeq();
    }
    // expected-note@+1 3 {{enclosing '#pragma acc loop' here}}
    #pragma acc loop auto vector
    for (int i = 0; i < 5; ++i) {
      // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
      useeHasGang();
      // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
      useeHasWorker();
      // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasVector', which has '#pragma acc routine vector'}}
      useeHasVector();
      useeHasSeq();
    }
    // expected-note@+1 2 {{enclosing '#pragma acc loop' here}}
    #pragma acc loop auto gang worker
    for (int i = 0; i < 5; ++i) {
      // expected-error@+1 {{'#pragma acc loop worker' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
      useeHasGang();
      // expected-error@+1 {{'#pragma acc loop worker' construct calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
      useeHasWorker();
      useeHasVector();
      useeHasSeq();
    }
    // expected-note@+1 3 {{enclosing '#pragma acc loop' here}}
    #pragma acc loop auto gang vector
    for (int i = 0; i < 5; ++i) {
      // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
      useeHasGang();
      // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
      useeHasWorker();
      // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasVector', which has '#pragma acc routine vector'}}
      useeHasVector();
      useeHasSeq();
    }
    // expected-note@+1 3 {{enclosing '#pragma acc loop' here}}
    #pragma acc loop auto worker vector
    for (int i = 0; i < 5; ++i) {
      // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
      useeHasGang();
      // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
      useeHasWorker();
      // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasVector', which has '#pragma acc routine vector'}}
      useeHasVector();
      useeHasSeq();
    }
    // expected-note@+1 3 {{enclosing '#pragma acc loop' here}}
    #pragma acc loop auto gang worker vector
    for (int i = 0; i < 5; ++i) {
      // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
      useeHasGang();
      // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
      useeHasWorker();
      // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasVector', which has '#pragma acc routine vector'}}
      useeHasVector();
      useeHasSeq();
    }
  }
}

// Unpartitioned orphaned loops are checked in useeHasAny_*FnUser above.
#pragma acc routine gang
void useeHasAny_orphanedPartLoopUser() {
  // expected-note@+1 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc loop gang' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    useeHasWorker();
    useeHasVector();
    useeHasSeq();
  }
  // expected-note@+1 2 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop worker
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc loop worker' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    // expected-error@+1 {{'#pragma acc loop worker' construct calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
    useeHasWorker();
    useeHasVector();
    useeHasSeq();
  }
  // expected-note@+1 3 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop vector
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
    useeHasWorker();
    // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasVector', which has '#pragma acc routine vector'}}
    useeHasVector();
    useeHasSeq();
  }
  // expected-note@+1 2 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop gang worker
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc loop worker' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    // expected-error@+1 {{'#pragma acc loop worker' construct calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
    useeHasWorker();
    useeHasVector();
    useeHasSeq();
  }
  // expected-note@+1 3 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop gang vector
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
    useeHasWorker();
    // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasVector', which has '#pragma acc routine vector'}}
    useeHasVector();
    useeHasSeq();
  }
  // expected-note@+1 3 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop worker vector
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
    useeHasWorker();
    // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasVector', which has '#pragma acc routine vector'}}
    useeHasVector();
    useeHasSeq();
  }
  // expected-note@+1 3 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop gang worker vector
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
    useeHasWorker();
    // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasVector', which has '#pragma acc routine vector'}}
    useeHasVector();
    useeHasSeq();
  }
  // expected-note@+1 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop auto gang
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc loop gang' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    useeHasWorker();
    useeHasVector();
    useeHasSeq();
  }
  // expected-note@+1 2 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop auto worker
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc loop worker' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    // expected-error@+1 {{'#pragma acc loop worker' construct calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
    useeHasWorker();
    useeHasVector();
    useeHasSeq();
  }
  // expected-note@+1 3 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop auto vector
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
    useeHasWorker();
    // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasVector', which has '#pragma acc routine vector'}}
    useeHasVector();
    useeHasSeq();
  }
  // expected-note@+1 2 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop auto gang worker
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc loop worker' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    // expected-error@+1 {{'#pragma acc loop worker' construct calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
    useeHasWorker();
    useeHasVector();
    useeHasSeq();
  }
  // expected-note@+1 3 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop auto gang vector
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
    useeHasWorker();
    // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasVector', which has '#pragma acc routine vector'}}
    useeHasVector();
    useeHasSeq();
  }
  // expected-note@+1 3 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop auto worker vector
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
    useeHasWorker();
    // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasVector', which has '#pragma acc routine vector'}}
    useeHasVector();
    useeHasSeq();
  }
  // expected-note@+1 3 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop auto gang worker vector
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasGang', which has '#pragma acc routine gang'}}
    useeHasGang();
    // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasWorker', which has '#pragma acc routine worker'}}
    useeHasWorker();
    // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'useeHasVector', which has '#pragma acc routine vector'}}
    useeHasVector();
    useeHasSeq();
  }
}

//..............................................................................
// What is considered a use when checking levels of parallelism?

// expected-note@+1 2 {{'#pragma acc routine' for function 'useKindParLevel' appears here}}
#pragma acc routine gang
void useKindParLevel();

// expected-note@+1 {{'#pragma acc routine' for function 'useKindParLevel_use' appears here}}
#pragma acc routine seq
void useKindParLevel_use() {
  // Unevaluated uses should not trigger diagnostics.
  size_t s = sizeof &useKindParLevel;
  size_t a = _Alignof(&useKindParLevel);
  // expected-error@+1 {{function 'useKindParLevel_use' has '#pragma acc routine seq' but calls function 'useKindParLevel', which has '#pragma acc routine gang'}}
  useKindParLevel();
  // Taking the address of a function doesn't place any requirement on its level
  // of parallelism.  It just means we have no way to check its level of
  // parallelism when it's actually called.
  void (*p)() = useKindParLevel;
}

// Repeat when user is loop construct.
void useKindParLevel_loopUse() {
  // expected-note@+1 {{enclosing '#pragma acc parallel loop' here}}
  #pragma acc parallel loop vector
  for (int i = 0; i < 5; ++i) {
    size_t s = sizeof &useKindParLevel;
    size_t a = _Alignof(&useKindParLevel);
    // expected-error@+1 {{'#pragma acc parallel loop vector' construct calls function 'useKindParLevel', which has '#pragma acc routine gang'}}
    useKindParLevel();
    void (*p)() = useKindParLevel;
  }
}

//..............................................................................
// Usee is orphaned loop construct.

#pragma acc routine gang
void useeIsOrphanedLoop_gangUser() {
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop worker
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop vector
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop gang worker
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop gang vector
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop worker vector
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop gang worker vector
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop auto
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop auto gang
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop auto worker
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop auto vector
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop auto gang worker
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop auto gang vector
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop auto worker vector
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop auto gang worker vector
  for (int i = 0; i < 5; ++i)
    ;
}

// expected-note@+1 8 {{'#pragma acc routine' for function 'useeIsOrphanedLoop_workerUser' appears here}}
#pragma acc routine worker
void useeIsOrphanedLoop_workerUser() {
  // expected-error@+1 {{function 'useeIsOrphanedLoop_workerUser' has '#pragma acc routine worker' but contains '#pragma acc loop gang'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop worker
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop vector
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_workerUser' has '#pragma acc routine worker' but contains '#pragma acc loop gang'}}
  #pragma acc loop gang worker
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_workerUser' has '#pragma acc routine worker' but contains '#pragma acc loop gang'}}
  #pragma acc loop gang vector
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop worker vector
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_workerUser' has '#pragma acc routine worker' but contains '#pragma acc loop gang'}}
  #pragma acc loop gang worker vector
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop auto
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_workerUser' has '#pragma acc routine worker' but contains '#pragma acc loop gang'}}
  #pragma acc loop auto gang
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop auto worker
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop auto vector
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_workerUser' has '#pragma acc routine worker' but contains '#pragma acc loop gang'}}
  #pragma acc loop auto gang worker
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_workerUser' has '#pragma acc routine worker' but contains '#pragma acc loop gang'}}
  #pragma acc loop auto gang vector
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop auto worker vector
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_workerUser' has '#pragma acc routine worker' but contains '#pragma acc loop gang'}}
  #pragma acc loop auto gang worker vector
  for (int i = 0; i < 5; ++i)
    ;
}

// expected-note@+1 12 {{'#pragma acc routine' for function 'useeIsOrphanedLoop_vectorUser' appears here}}
#pragma acc routine vector
void useeIsOrphanedLoop_vectorUser() {
  // expected-error@+1 {{function 'useeIsOrphanedLoop_vectorUser' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_vectorUser' has '#pragma acc routine vector' but contains '#pragma acc loop worker'}}
  #pragma acc loop worker
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop vector
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_vectorUser' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  #pragma acc loop gang worker
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_vectorUser' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  #pragma acc loop gang vector
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_vectorUser' has '#pragma acc routine vector' but contains '#pragma acc loop worker'}}
  #pragma acc loop worker vector
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_vectorUser' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  #pragma acc loop gang worker vector
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop auto
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_vectorUser' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  #pragma acc loop auto gang
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_vectorUser' has '#pragma acc routine vector' but contains '#pragma acc loop worker'}}
  #pragma acc loop auto worker
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop auto vector
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_vectorUser' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  #pragma acc loop auto gang worker
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_vectorUser' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  #pragma acc loop auto gang vector
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_vectorUser' has '#pragma acc routine vector' but contains '#pragma acc loop worker'}}
  #pragma acc loop auto worker vector
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_vectorUser' has '#pragma acc routine vector' but contains '#pragma acc loop gang'}}
  #pragma acc loop auto gang worker vector
  for (int i = 0; i < 5; ++i)
    ;
}

// expected-note@+1 14 {{'#pragma acc routine' for function 'useeIsOrphanedLoop_seqUser' appears here}}
#pragma acc routine seq
void useeIsOrphanedLoop_seqUser() {
  // expected-error@+1 {{function 'useeIsOrphanedLoop_seqUser' has '#pragma acc routine seq' but contains '#pragma acc loop gang'}}
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_seqUser' has '#pragma acc routine seq' but contains '#pragma acc loop worker'}}
  #pragma acc loop worker
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_seqUser' has '#pragma acc routine seq' but contains '#pragma acc loop vector'}}
  #pragma acc loop vector
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_seqUser' has '#pragma acc routine seq' but contains '#pragma acc loop gang'}}
  #pragma acc loop gang worker
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_seqUser' has '#pragma acc routine seq' but contains '#pragma acc loop gang'}}
  #pragma acc loop gang vector
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_seqUser' has '#pragma acc routine seq' but contains '#pragma acc loop worker'}}
  #pragma acc loop worker vector
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_seqUser' has '#pragma acc routine seq' but contains '#pragma acc loop gang'}}
  #pragma acc loop gang worker vector
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop
  for (int i = 0; i < 5; ++i)
    ;
  #pragma acc loop auto
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_seqUser' has '#pragma acc routine seq' but contains '#pragma acc loop gang'}}
  #pragma acc loop auto gang
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_seqUser' has '#pragma acc routine seq' but contains '#pragma acc loop worker'}}
  #pragma acc loop auto worker
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_seqUser' has '#pragma acc routine seq' but contains '#pragma acc loop vector'}}
  #pragma acc loop auto vector
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_seqUser' has '#pragma acc routine seq' but contains '#pragma acc loop gang'}}
  #pragma acc loop auto gang worker
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_seqUser' has '#pragma acc routine seq' but contains '#pragma acc loop gang'}}
  #pragma acc loop auto gang vector
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_seqUser' has '#pragma acc routine seq' but contains '#pragma acc loop worker'}}
  #pragma acc loop auto worker vector
  for (int i = 0; i < 5; ++i)
    ;
  // expected-error@+1 {{function 'useeIsOrphanedLoop_seqUser' has '#pragma acc routine seq' but contains '#pragma acc loop gang'}}
  #pragma acc loop auto gang worker vector
  for (int i = 0; i < 5; ++i)
    ;
}

//------------------------------------------------------------------------------
// Missing declaration possibly within bad context.
//------------------------------------------------------------------------------

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

// Check at end of file (after preprocessing).
// expected-error@+4 {{'#pragma acc routine' cannot be nested within '#pragma acc routine'}}
// expected-note@+2 {{enclosing '#pragma acc routine' here}}
// expected-error@+1 {{'#pragma acc routine' must be followed by a lone function prototype or definition}}
#pragma acc routine seq
#pragma acc routine seq

//##############################################################################
// Diagnostic checks that require some OpenACC to be transformed to OpenMP.
//##############################################################################

#elif CASE == CASE_WITH_TRANSFORM_0

// Within each OpenACC construct below, diagnostics are recorded, and then the
// construct is transformed to OpenMP.  During the transformation, SemaOpenACC
// is careful to skip recording the diagnostics again.  Later, the routine
// directive is implied, and all recorded diagnostics are then emitted.

void dupStaticDiagBeforeRoutineDir() {
  int i;
  // expected-error@+1 {{'#pragma acc data' is not permitted within function 'dupStaticDiagBeforeRoutineDir' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc data copy(i)
  {
    // expected-error@+1 {{static local variable 'j' is not permitted within function 'dupStaticDiagBeforeRoutineDir' because the latter is attributed with '#pragma acc routine'}}
    static int j;
  }
  // expected-error@+1 {{'#pragma acc parallel' is not permitted within function 'dupStaticDiagBeforeRoutineDir' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel
  {
    // expected-error@+1 {{static local variable 'j' is not permitted within function 'dupStaticDiagBeforeRoutineDir' because the latter is attributed with '#pragma acc routine'}}
    static int j;
  }
  // expected-error@+1 {{'#pragma acc parallel loop' is not permitted within function 'dupStaticDiagBeforeRoutineDir' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel loop
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{static local variable 'j' is not permitted within function 'dupStaticDiagBeforeRoutineDir' because the latter is attributed with '#pragma acc routine'}}
    static int j;
  }
  // This diagnostic is emitted immediately, so keep it last so it doesn't
  // suppress transformation of the other OpenACC constructs.
  // expected-error@+1 {{function 'dupStaticDiagBeforeRoutineDir' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
  #pragma acc loop
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{static local variable 'j' is not permitted within function 'dupStaticDiagBeforeRoutineDir' because the latter is attributed with '#pragma acc routine'}}
    static int j;
  }
}
void dupStaticDiagBeforeRoutineDir_use() {
  // expected-note@+2 7 {{'#pragma acc routine seq' implied for function 'dupStaticDiagBeforeRoutineDir' by use in construct '#pragma acc parallel' here}}
  #pragma acc parallel
  dupStaticDiagBeforeRoutineDir();
}

#elif CASE == CASE_WITH_TRANSFORM_1

// Same as previous case except diagnostics are emitted immediately, so
// transforming to OpenMP should never happen, and neither should duplicate
// diagnostics.

// expected-note@+1 7 {{'#pragma acc routine' for function 'dupStaticDiagAfterRoutineDir' appears here}}
#pragma acc routine seq
void dupStaticDiagAfterRoutineDir() {
  int i;
  // expected-error@+1 {{'#pragma acc data' is not permitted within function 'dupStaticDiagAfterRoutineDir' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc data copy(i)
  {
    // expected-error@+1 {{static local variable 'j' is not permitted within function 'dupStaticDiagAfterRoutineDir' because the latter is attributed with '#pragma acc routine'}}
    static int j;
  }
  // expected-error@+1 {{'#pragma acc parallel' is not permitted within function 'dupStaticDiagAfterRoutineDir' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel
  {
    // expected-error@+1 {{static local variable 'j' is not permitted within function 'dupStaticDiagAfterRoutineDir' because the latter is attributed with '#pragma acc routine'}}
    static int j;
  }
  // expected-error@+1 {{'#pragma acc parallel loop' is not permitted within function 'dupStaticDiagAfterRoutineDir' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel loop
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{static local variable 'j' is not permitted within function 'dupStaticDiagAfterRoutineDir' because the latter is attributed with '#pragma acc routine'}}
    static int j;
  }
  #pragma acc loop
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{static local variable 'j' is not permitted within function 'dupStaticDiagAfterRoutineDir' because the latter is attributed with '#pragma acc routine'}}
    static int j;
  }
}

#elif CASE == CASE_WITH_TRANSFORM_2

// Within each OpenACC construct below, any diagnostic is recorded, and then the
// construct is transformed to OpenMP.  During the transformation, SemaOpenACC
// is careful to skip recording the diagnostics again, and it's careful to skip
// emitting new diagnostics in some cases now that the enclosing OpenACC
// construct isn't seen.  Later, the routine directive is implied, and all
// recorded diagnostics are then emitted.

// expected-note@+1 2 {{'#pragma acc routine' for function 'dupParLevelDiagBeforeRoutineDir_usee' appears here}}
#pragma acc routine gang
void dupParLevelDiagBeforeRoutineDir_usee();
void dupParLevelDiagBeforeRoutineDir_user() {
  int i;
  // expected-error@+1 {{'#pragma acc parallel' is not permitted within function 'dupParLevelDiagBeforeRoutineDir_user' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel
  {
    // New diagnostic about _user/_usee mismatch would be emitted if not
    // suppressed in OpenMP translation.
    dupParLevelDiagBeforeRoutineDir_usee();
  }
  // expected-error@+1 {{'#pragma acc parallel loop' is not permitted within function 'dupParLevelDiagBeforeRoutineDir_user' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel loop
  for (int i = 0; i < 5; ++i) {
    // New diagnostic about _user/_usee mismatch would be emitted if not
    // suppressed in OpenMP translation.
    dupParLevelDiagBeforeRoutineDir_usee();
  }
  // expected-error@+2 {{'#pragma acc parallel loop' is not permitted within function 'dupParLevelDiagBeforeRoutineDir_user' because the latter is attributed with '#pragma acc routine'}}
  // expected-note@+1 {{enclosing '#pragma acc parallel loop' here}}
  #pragma acc parallel loop vector
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc parallel loop vector' construct calls function 'dupParLevelDiagBeforeRoutineDir_usee', which has '#pragma acc routine gang'}}
    dupParLevelDiagBeforeRoutineDir_usee();
  }
  // This diagnostic is emitted immediately, so keep it last so it doesn't
  // suppress transformation of the other OpenACC constructs.
  // expected-error@+2 {{function 'dupParLevelDiagBeforeRoutineDir_user' contains orphaned '#pragma acc loop' but has no explicit '#pragma acc routine'}}
  // expected-note@+1 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop vector
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'dupParLevelDiagBeforeRoutineDir_usee', which has '#pragma acc routine gang'}}
    dupParLevelDiagBeforeRoutineDir_usee();
  }
}
void dupParLevelDiagBeforeRoutineDir_user_user() {
  // expected-note@+2 3 {{'#pragma acc routine seq' implied for function 'dupParLevelDiagBeforeRoutineDir_user' by use in construct '#pragma acc parallel' here}}
  #pragma acc parallel
  dupParLevelDiagBeforeRoutineDir_user();
}

#elif CASE == CASE_WITH_TRANSFORM_3

// Same as previous case except diagnostics are emitted immediately, so
// transforming to OpenMP should never happen, and neither should duplicate or
// new diagnostics.

// expected-note@+1 2 {{'#pragma acc routine' for function 'dupParLevelDiagBeforeRoutineDir_usee' appears here}}
#pragma acc routine gang
void dupParLevelDiagBeforeRoutineDir_usee();
// expected-note@+1 4 {{'#pragma acc routine' for function 'dupParLevelDiagBeforeRoutineDir_user' appears here}}
#pragma acc routine seq
void dupParLevelDiagBeforeRoutineDir_user() {
  int i;
  // expected-error@+1 {{'#pragma acc parallel' is not permitted within function 'dupParLevelDiagBeforeRoutineDir_user' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel
  {
    dupParLevelDiagBeforeRoutineDir_usee();
  }
  // expected-error@+1 {{'#pragma acc parallel loop' is not permitted within function 'dupParLevelDiagBeforeRoutineDir_user' because the latter is attributed with '#pragma acc routine'}}
  #pragma acc parallel loop
  for (int i = 0; i < 5; ++i) {
    dupParLevelDiagBeforeRoutineDir_usee();
  }
  // expected-error@+2 {{'#pragma acc parallel loop' is not permitted within function 'dupParLevelDiagBeforeRoutineDir_user' because the latter is attributed with '#pragma acc routine'}}
  // expected-note@+1 {{enclosing '#pragma acc parallel loop' here}}
  #pragma acc parallel loop vector
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc parallel loop vector' construct calls function 'dupParLevelDiagBeforeRoutineDir_usee', which has '#pragma acc routine gang'}}
    dupParLevelDiagBeforeRoutineDir_usee();
  }
  // expected-error@+2 {{function 'dupParLevelDiagBeforeRoutineDir_user' has '#pragma acc routine seq' but contains '#pragma acc loop vector'}}
  // expected-note@+1 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop vector
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc loop vector' construct calls function 'dupParLevelDiagBeforeRoutineDir_usee', which has '#pragma acc routine gang'}}
    dupParLevelDiagBeforeRoutineDir_usee();
  }
}

#else
# error undefined CASE
#endif