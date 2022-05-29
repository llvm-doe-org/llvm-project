// Check diagnostics for "acc atomic".

// The diagnostics should be the same regardless of the enclosing OpenACC
// construct.  We check all possible enclosing OpenACC constructs, and the test
// is still quite fast.
// RUN: %data {
// RUN:   (cflags=-DOUTER_DIR=OUTER_DIR_NONE         )
// RUN:   (cflags=-DOUTER_DIR=OUTER_DIR_DATA         )
// RUN:   (cflags=-DOUTER_DIR=OUTER_DIR_PARALLEL     )
// RUN:   (cflags=-DOUTER_DIR=OUTER_DIR_LOOP         )
// RUN:   (cflags=-DOUTER_DIR=OUTER_DIR_PARALLEL_LOOP)
// RUN:   (cflags=-DOUTER_DIR=OUTER_DIR_ROUTINE      )
// RUN: }
// RUN: %for {
//        # OpenACC disabled
// RUN:   %clang_cc1 -verify=noacc,expected-noacc %s %[cflags]
//
//        # OpenACC enabled
// RUN:   %clang_cc1 -verify=expected,expected-noacc -fopenacc %s %[cflags]
//
//        # OpenACC enabled, but just repeat update clause tests while leaving
//        # update implicit.
// RUN:   %clang_cc1 -verify=expected,expected-noacc -fopenacc \
// RUN:              -DCHECK_IMPLICIT_UPDATE %s %[cflags]
// RUN: }

// END.

#if CHECK_IMPLICIT_UPDATE
# define UPDATE
#else
# define UPDATE update
#endif

#define OUTER_DIR_NONE          0
#define OUTER_DIR_DATA          1
#define OUTER_DIR_PARALLEL      2
#define OUTER_DIR_LOOP          3
#define OUTER_DIR_PARALLEL_LOOP 4
#define OUTER_DIR_ROUTINE       5

#if OUTER_DIR == OUTER_DIR_NONE
# define OUTER_DIR_START
# define OUTER_DIR_END
# define ROUTINE_DIR
#elif OUTER_DIR == OUTER_DIR_DATA
# define OUTER_DIR_START _Pragma("acc data copy(i)") {
# define OUTER_DIR_END }
# define ROUTINE_DIR
#elif OUTER_DIR == OUTER_DIR_PARALLEL
# define OUTER_DIR_START _Pragma("acc parallel") {
# define OUTER_DIR_END }
# define ROUTINE_DIR
#elif OUTER_DIR == OUTER_DIR_LOOP
# define OUTER_DIR_START                                                       \
   _Pragma("acc loop")                                                         \
   for (int l = 0; l < 5; ++l) {
# define OUTER_DIR_END }
# define ROUTINE_DIR _Pragma("acc routine seq")
#elif OUTER_DIR == OUTER_DIR_PARALLEL_LOOP
# define OUTER_DIR_START                                                       \
   _Pragma("acc parallel loop")                                                \
   for (int l = 0; l < 5; ++l) {
# define OUTER_DIR_END }
# define ROUTINE_DIR
#elif OUTER_DIR == OUTER_DIR_ROUTINE
# define OUTER_DIR_START
# define OUTER_DIR_END
# define ROUTINE_DIR _Pragma("acc routine seq")
#else
# error "unrecognized OUTER_DIR"
#endif

int foo() { return 8; }

ROUTINE_DIR
int main() {
  int v, x;
  int i, jk;
  float f;
  struct S { int i; int j; } vs, xs;

  OUTER_DIR_START

#if !CHECK_IMPLICIT_UPDATE

  //============================================================================
  // Unrecognized clauses
  //============================================================================

  //----------------------------------------------------------------------------
  // Bogus clauses.
  //----------------------------------------------------------------------------

  // expected-warning@+1 {{extra tokens at the end of '#pragma acc atomic' are ignored}}
  #pragma acc atomic foo
  x++;
  // expected-warning@+1 {{extra tokens at the end of '#pragma acc atomic' are ignored}}
  #pragma acc atomic foo bar
  x++;

  //----------------------------------------------------------------------------
  // Well formed clauses not permitted here.  Make sure every one is an error.
  //----------------------------------------------------------------------------

  // expected-error@+8 {{unexpected OpenACC clause 'nomap' in directive '#pragma acc atomic'}}
  // expected-error@+7 {{unexpected OpenACC clause 'present' in directive '#pragma acc atomic'}}
  // expected-error@+6 {{unexpected OpenACC clause 'copy' in directive '#pragma acc atomic'}}
  // expected-error@+5 {{unexpected OpenACC clause 'copyin' in directive '#pragma acc atomic'}}
  // expected-error@+4 {{unexpected OpenACC clause 'copyout' in directive '#pragma acc atomic'}}
  // expected-error@+3 {{unexpected OpenACC clause 'create' in directive '#pragma acc atomic'}}
  // expected-error@+2 {{unexpected OpenACC clause 'no_create' in directive '#pragma acc atomic'}}
  // expected-error@+1 {{unexpected OpenACC clause 'delete' in directive '#pragma acc atomic'}}
  #pragma acc atomic nomap(i) present(i) copy(i) copyin(i) copyout(i) create(i) no_create(i) delete(i)
  x++;
  // expected-error@+4 {{unexpected OpenACC clause 'pcopy' in directive '#pragma acc atomic'}}
  // expected-error@+3 {{unexpected OpenACC clause 'pcopyin' in directive '#pragma acc atomic'}}
  // expected-error@+2 {{unexpected OpenACC clause 'pcopyout' in directive '#pragma acc atomic'}}
  // expected-error@+1 {{unexpected OpenACC clause 'pcreate' in directive '#pragma acc atomic'}}
  #pragma acc atomic pcopy(i) pcopyin(i) pcopyout(i) pcreate(i)
  x++;
  // expected-error@+4 {{unexpected OpenACC clause 'present_or_copy' in directive '#pragma acc atomic'}}
  // expected-error@+3 {{unexpected OpenACC clause 'present_or_copyin' in directive '#pragma acc atomic'}}
  // expected-error@+2 {{unexpected OpenACC clause 'present_or_copyout' in directive '#pragma acc atomic'}}
  // expected-error@+1 {{unexpected OpenACC clause 'present_or_create' in directive '#pragma acc atomic'}}
  #pragma acc atomic present_or_copy(i) present_or_copyin(i) present_or_copyout(i) present_or_create(i)
  x++;
  // expected-error@+4 {{unexpected OpenACC clause 'shared' in directive '#pragma acc atomic'}}
  // expected-error@+3 {{unexpected OpenACC clause 'reduction' in directive '#pragma acc atomic'}}
  // expected-error@+2 {{unexpected OpenACC clause 'private' in directive '#pragma acc atomic'}}
  // expected-error@+1 {{unexpected OpenACC clause 'firstprivate' in directive '#pragma acc atomic'}}
  #pragma acc atomic read shared(i, jk) reduction(+:i,jk,f) private(i) firstprivate(i)
  v = x;
  // expected-error@+5 {{unexpected OpenACC clause 'if' in directive '#pragma acc atomic'}}
  // expected-error@+4 {{unexpected OpenACC clause 'if_present' in directive '#pragma acc atomic'}}
  // expected-error@+3 {{unexpected OpenACC clause 'self' in directive '#pragma acc atomic'}}
  // expected-error@+2 {{unexpected OpenACC clause 'host' in directive '#pragma acc atomic'}}
  // expected-error@+1 {{unexpected OpenACC clause 'device' in directive '#pragma acc atomic'}}
  #pragma acc atomic if(1) write if_present self(i) host(jk) device(f)
  x = 10;
  // expected-error@+3 {{unexpected OpenACC clause 'num_gangs' in directive '#pragma acc atomic'}}
  // expected-error@+2 {{unexpected OpenACC clause 'num_workers' in directive '#pragma acc atomic'}}
  // expected-error@+1 {{unexpected OpenACC clause 'vector_length' in directive '#pragma acc atomic'}}
  #pragma acc atomic num_gangs(1) update num_workers(2) vector_length(3)
  x++;
  // expected-error@+7 {{unexpected OpenACC clause 'seq' in directive '#pragma acc atomic'}}
  // expected-error@+6 {{unexpected OpenACC clause 'independent' in directive '#pragma acc atomic'}}
  // expected-error@+5 {{unexpected OpenACC clause 'auto' in directive '#pragma acc atomic'}}
  // expected-error@+4 {{unexpected OpenACC clause 'gang' in directive '#pragma acc atomic'}}
  // expected-error@+3 {{unexpected OpenACC clause 'worker' in directive '#pragma acc atomic'}}
  // expected-error@+2 {{unexpected OpenACC clause 'vector' in directive '#pragma acc atomic'}}
  // expected-error@+1 {{unexpected OpenACC clause 'collapse' in directive '#pragma acc atomic'}}
  #pragma acc atomic seq independent auto capture gang worker vector collapse(1)
  v = x++;

  //----------------------------------------------------------------------------
  // Malformed clauses not permitted here.
  //----------------------------------------------------------------------------

  // expected-error@+2 {{unexpected OpenACC clause 'shared' in directive '#pragma acc atomic'}}
  // expected-error@+1 {{expected '(' after 'shared'}}
  #pragma acc atomic shared
  x++;
  // expected-error@+4 {{unexpected OpenACC clause 'shared' in directive '#pragma acc atomic'}}
  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc atomic shared(
  x++;
  // expected-error@+3 {{unexpected OpenACC clause 'shared' in directive '#pragma acc atomic'}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc atomic shared(i
  x++;

  //============================================================================
  // Conflicting clauses.
  //============================================================================

  // expected-error@+1 {{directive '#pragma acc atomic' cannot contain more than one 'read' clause}}
  #pragma acc atomic read read
  v = x;
  // expected-error@+1 {{unexpected OpenACC clause 'write', 'read' is specified already}}
  #pragma acc atomic read write
  v = x;
  // expected-error@+1 {{unexpected OpenACC clause 'update', 'read' is specified already}}
  #pragma acc atomic read update
  v = x;
  // expected-error@+1 {{unexpected OpenACC clause 'capture', 'read' is specified already}}
  #pragma acc atomic read capture
  v = x;

  // expected-error@+1 {{unexpected OpenACC clause 'read', 'write' is specified already}}
  #pragma acc atomic write read
  x = 10;
  // expected-error@+1 {{directive '#pragma acc atomic' cannot contain more than one 'write' clause}}
  #pragma acc atomic write write
  x = 10;
  // expected-error@+1 {{unexpected OpenACC clause 'update', 'write' is specified already}}
  #pragma acc atomic write update
  x = 10;
  // expected-error@+1 {{unexpected OpenACC clause 'capture', 'write' is specified already}}
  #pragma acc atomic write capture
  x = 10;

  // expected-error@+1 {{unexpected OpenACC clause 'read', 'update' is specified already}}
  #pragma acc atomic update read
  x++;
  // expected-error@+1 {{unexpected OpenACC clause 'write', 'update' is specified already}}
  #pragma acc atomic update write
  x++;
  // expected-error@+1 {{directive '#pragma acc atomic' cannot contain more than one 'update' clause}}
  #pragma acc atomic update update
  x++;
  // expected-error@+1 {{unexpected OpenACC clause 'capture', 'update' is specified already}}
  #pragma acc atomic update capture
  x++;

  // expected-error@+1 {{unexpected OpenACC clause 'read', 'capture' is specified already}}
  #pragma acc atomic capture read
  v = x++;
  // expected-error@+1 {{unexpected OpenACC clause 'write', 'capture' is specified already}}
  #pragma acc atomic capture write
  v = x++;
  // expected-error@+1 {{unexpected OpenACC clause 'update', 'capture' is specified already}}
  #pragma acc atomic capture update
  v = x++;
  // expected-error@+1 {{directive '#pragma acc atomic' cannot contain more than one 'capture' clause}}
  #pragma acc atomic capture capture
  v = x++;

  // expected-error@+10 {{unexpected OpenACC clause 'write', 'read' is specified already}}
  // expected-error@+9 {{unexpected OpenACC clause 'update', 'read' is specified already}}
  // expected-error@+8 {{unexpected OpenACC clause 'update', 'write' is specified already}}
  // expected-error@+7 {{unexpected OpenACC clause 'capture', 'read' is specified already}}
  // expected-error@+6 {{unexpected OpenACC clause 'capture', 'write' is specified already}}
  // expected-error@+5 {{unexpected OpenACC clause 'capture', 'update' is specified already}}
  // expected-error@+4 {{directive '#pragma acc atomic' cannot contain more than one 'read' clause}}
  // expected-error@+3 {{directive '#pragma acc atomic' cannot contain more than one 'write' clause}}
  // expected-error@+2 {{directive '#pragma acc atomic' cannot contain more than one 'update' clause}}
  // expected-error@+1 {{directive '#pragma acc atomic' cannot contain more than one 'capture' clause}}
  #pragma acc atomic read write update capture read write update capture
  v = x;
  // expected-error@+10 {{unexpected OpenACC clause 'update', 'write' is specified already}}
  // expected-error@+9 {{unexpected OpenACC clause 'capture', 'write' is specified already}}
  // expected-error@+8 {{unexpected OpenACC clause 'capture', 'update' is specified already}}
  // expected-error@+7 {{unexpected OpenACC clause 'read', 'write' is specified already}}
  // expected-error@+6 {{unexpected OpenACC clause 'read', 'update' is specified already}}
  // expected-error@+5 {{unexpected OpenACC clause 'read', 'capture' is specified already}}
  // expected-error@+4 {{directive '#pragma acc atomic' cannot contain more than one 'write' clause}}
  // expected-error@+3 {{directive '#pragma acc atomic' cannot contain more than one 'update' clause}}
  // expected-error@+2 {{directive '#pragma acc atomic' cannot contain more than one 'capture' clause}}
  // expected-error@+1 {{directive '#pragma acc atomic' cannot contain more than one 'read' clause}}
  #pragma acc atomic write update capture read write update capture read
  x = 10;
  // expected-error@+10 {{unexpected OpenACC clause 'capture', 'update' is specified already}}
  // expected-error@+9 {{unexpected OpenACC clause 'read', 'update' is specified already}}
  // expected-error@+8 {{unexpected OpenACC clause 'read', 'capture' is specified already}}
  // expected-error@+7 {{unexpected OpenACC clause 'write', 'update' is specified already}}
  // expected-error@+6 {{unexpected OpenACC clause 'write', 'capture' is specified already}}
  // expected-error@+5 {{unexpected OpenACC clause 'write', 'read' is specified already}}
  // expected-error@+4 {{directive '#pragma acc atomic' cannot contain more than one 'update' clause}}
  // expected-error@+3 {{directive '#pragma acc atomic' cannot contain more than one 'capture' clause}}
  // expected-error@+2 {{directive '#pragma acc atomic' cannot contain more than one 'read' clause}}
  // expected-error@+1 {{directive '#pragma acc atomic' cannot contain more than one 'write' clause}}
  #pragma acc atomic update capture read write update capture read write
  x++;
  // expected-error@+10 {{unexpected OpenACC clause 'read', 'capture' is specified already}}
  // expected-error@+9 {{unexpected OpenACC clause 'write', 'capture' is specified already}}
  // expected-error@+8 {{unexpected OpenACC clause 'write', 'read' is specified already}}
  // expected-error@+7 {{unexpected OpenACC clause 'update', 'capture' is specified already}}
  // expected-error@+6 {{unexpected OpenACC clause 'update', 'read' is specified already}}
  // expected-error@+5 {{unexpected OpenACC clause 'update', 'write' is specified already}}
  // expected-error@+4 {{directive '#pragma acc atomic' cannot contain more than one 'capture' clause}}
  // expected-error@+3 {{directive '#pragma acc atomic' cannot contain more than one 'read' clause}}
  // expected-error@+2 {{directive '#pragma acc atomic' cannot contain more than one 'write' clause}}
  // expected-error@+1 {{directive '#pragma acc atomic' cannot contain more than one 'update' clause}}
  #pragma acc atomic capture read write update capture read write update
  v = x++;

  //============================================================================
  // Associated statement.
  //
  // For each clause, as part of the effort to achieve reasonable coverage of
  // the associated statement validation, we check all associated statement
  // forms that are listed in OpenACC 3.2, sec. 2.12 "Atomic Construct".  The
  // forms listed for other clauses seem especially important because providing
  // the wrong clause for a valid form seems like an easy mistake to make.  One
  // exception is that, if a structured block isn't permitted, it seems
  // unnecessary to check all structured block forms listed in the spec.
  //============================================================================

  //----------------------------------------------------------------------------
  // acc atomic read
  //----------------------------------------------------------------------------

  //............................................................................
  // Check forms listed in the spec.
  //............................................................................

  #pragma acc atomic read
  v = x;
  #pragma acc atomic read
  x // expected-error {{invalid statement after '#pragma acc atomic read'}}
    =
      1; // expected-note {{expected expression here to be an lvalue}}
  #pragma acc atomic read
  x++; // expected-error {{invalid statement after '#pragma acc atomic read'}}
       // expected-note@-1 {{expected expression here to be a simple assignment}}
  #pragma acc atomic read
  x--; // expected-error {{invalid statement after '#pragma acc atomic read'}}
       // expected-note@-1 {{expected expression here to be a simple assignment}}
  #pragma acc atomic read
  ++x; // expected-error {{invalid statement after '#pragma acc atomic read'}}
       // expected-note@-1 {{expected expression here to be a simple assignment}}
  #pragma acc atomic read
  --x; // expected-error {{invalid statement after '#pragma acc atomic read'}}
       // expected-note@-1 {{expected expression here to be a simple assignment}}
  #pragma acc atomic read
  x // expected-error {{invalid statement after '#pragma acc atomic read'}}
    += // expected-note {{expected expression here to be a simple assignment}}
       3;
  #pragma acc atomic read
  x // expected-error {{invalid statement after '#pragma acc atomic read'}}
    =
      x - 9; // expected-note {{expected expression here to be an lvalue}}
  #pragma acc atomic read
  x // expected-error {{invalid statement after '#pragma acc atomic read'}}
    =
      10 * x; // expected-note {{expected expression here to be an lvalue}}
  #pragma acc atomic read
  v // expected-error {{invalid statement after '#pragma acc atomic read'}}
    =
      x++; // expected-note {{expected expression here to be an lvalue}}
  #pragma acc atomic read
  v // expected-error {{invalid statement after '#pragma acc atomic read'}}
    =
      x--; // expected-note {{expected expression here to be an lvalue}}
  #pragma acc atomic read
  v // expected-error {{invalid statement after '#pragma acc atomic read'}}
    =
      ++x; // expected-note {{expected expression here to be an lvalue}}
  #pragma acc atomic read
  v // expected-error {{invalid statement after '#pragma acc atomic read'}}
    =
      --x; // expected-note {{expected expression here to be an lvalue}}
  #pragma acc atomic read
  v // expected-error {{invalid statement after '#pragma acc atomic read'}}
    =
      x /= 89; // expected-note {{expected expression here to be an lvalue}}
  #pragma acc atomic read
  v // expected-error {{invalid statement after '#pragma acc atomic read'}}
    =
      x = x & 5; // expected-note {{expected expression here to be an lvalue}}
  #pragma acc atomic read
  v // expected-error {{invalid statement after '#pragma acc atomic read'}}
    =
      x = 384 ^ x; // expected-note {{expected expression here to be an lvalue}}
  #pragma acc atomic read
  { // expected-error {{invalid statement after '#pragma acc atomic read'}}
    // expected-note@-1 {{expected statement here to be an expression statement}}
     v = x;
     x |= 3;
  }

  //............................................................................
  // Try another non-expression statement.
  //............................................................................

  #pragma acc atomic read
  do { // expected-error {{invalid statement after '#pragma acc atomic read'}}
       // expected-note@-1 {{expected statement here to be an expression statement}}
    v = x;
  } while (0);

  //............................................................................
  // Check expression statement.
  //............................................................................

  // Make sure the valid form isn't accepted within a compound statement.
  #pragma acc atomic read
  { // expected-error {{invalid statement after '#pragma acc atomic read'}}
    // expected-note@-1 {{expected statement here to be an expression statement}}
    v = x;
  }

  // LHS non-lvalue is generally not permitted, so some OpenACC checks don't
  // run.
  #pragma acc atomic read
  10 = x; // expected-noacc-error {{expression is not assignable}}
          // expected-error@-1 {{invalid statement after '#pragma acc atomic read'}}
          // expected-note@-2 {{expected expression here to be a simple assignment}}

  // Check non-scalar operands.
  #pragma acc atomic read
  vs // expected-error {{invalid statement after '#pragma acc atomic read'}}
     // expected-note@-1 {{expected expression here to be of scalar type}}
     =
       xs; // expected-note {{expected expression here to be of scalar type}}

  //----------------------------------------------------------------------------
  // acc atomic write
  //----------------------------------------------------------------------------

  //............................................................................
  // Check forms listed in the spec.
  //............................................................................

  #pragma acc atomic write
  v = x;
  #pragma acc atomic write
  x = 1;
  #pragma acc atomic write
  x++; // expected-error {{invalid statement after '#pragma acc atomic write'}}
       // expected-note@-1 {{expected expression here to be a simple assignment}}
  #pragma acc atomic write
  x--; // expected-error {{invalid statement after '#pragma acc atomic write'}}
       // expected-note@-1 {{expected expression here to be a simple assignment}}
  #pragma acc atomic write
  ++x; // expected-error {{invalid statement after '#pragma acc atomic write'}}
       // expected-note@-1 {{expected expression here to be a simple assignment}}
  #pragma acc atomic write
  --x; // expected-error {{invalid statement after '#pragma acc atomic write'}}
       // expected-note@-1 {{expected expression here to be a simple assignment}}
  #pragma acc atomic write
  x // expected-error {{invalid statement after '#pragma acc atomic write'}}
    += // expected-note {{expected expression here to be a simple assignment}}
       3;
  // We don't currently have compile-time diagnostics for the next two forms,
  // but they are not permitted by OpenACC because the right-hand side accesses
  // x.
  #pragma acc atomic write
  x = x - 9;
  #pragma acc atomic write
  x = 10 * x;
  // The next few forms are fine.
  #pragma acc atomic write
  v = x++;
  #pragma acc atomic write
  v = x--;
  #pragma acc atomic write
  v = ++x;
  #pragma acc atomic write
  v = --x;
  #pragma acc atomic write
  v = x /= 89;
  #pragma acc atomic write
  v = x = x & 5;
  #pragma acc atomic write
  v = x = 384 ^ x;
  #pragma acc atomic write
  { // expected-error {{invalid statement after '#pragma acc atomic write'}}
    // expected-note@-1 {{expected statement here to be an expression statement}}
    v = x;
    x |= 3;
  }

  //............................................................................
  // Try another non-expression statement.
  //............................................................................

  #pragma acc atomic write
  do { // expected-error {{invalid statement after '#pragma acc atomic write'}}
       // expected-note@-1 {{expected statement here to be an expression statement}}
    x = 6;
  } while (0);

  //............................................................................
  // Check expression statement.
  //............................................................................

  // Make sure the valid form isn't accepted within a compound statement.
  #pragma acc atomic write
  { // expected-error {{invalid statement after '#pragma acc atomic write'}}
    // expected-note@-1 {{expected statement here to be an expression statement}}
    x = 75;
  }

  // LHS non-lvalue is generally not permitted, so some OpenACC checks don't
  // run.
  #pragma acc atomic write
  10 = x; // expected-noacc-error {{expression is not assignable}}
          // expected-error@-1 {{invalid statement after '#pragma acc atomic write'}}
          // expected-note@-2 {{expected expression here to be a simple assignment}}

  // Check non-scalar operands.
  #pragma acc atomic write
  vs // expected-error {{invalid statement after '#pragma acc atomic write'}}
     // expected-note@-1 {{expected expression here to be of scalar type}}
     =
       xs; // expected-note {{expected expression here to be of scalar type}}

#endif // !CHECK_IMPLICIT_UPDATE

  //----------------------------------------------------------------------------
  // acc atomic update
  //----------------------------------------------------------------------------

  //............................................................................
  // Check forms listed in the spec.
  //............................................................................

  #pragma acc atomic UPDATE
  v // expected-error {{invalid statement after '#pragma acc atomic update'}}
    =
      x; // expected-note {{expected operator expression here where the operator is one of '+', }}
  #pragma acc atomic UPDATE
  x // expected-error {{invalid statement after '#pragma acc atomic update'}}
    =
      1; // expected-note {{expected operator expression here where the operator is one of '+', }}
  #pragma acc atomic UPDATE
  x++;
  #pragma acc atomic UPDATE
  x--;
  #pragma acc atomic UPDATE
  ++x;
  #pragma acc atomic UPDATE
  --x;
  #pragma acc atomic UPDATE
  x += 3;
  #pragma acc atomic UPDATE
  x = x - 9;
  #pragma acc atomic UPDATE
  x = 10 * x;
  #pragma acc atomic UPDATE
  v // expected-error {{invalid statement after '#pragma acc atomic update'}}
    =
      x++; // expected-note {{expected operator expression here where the operator is one of '+', }}
  #pragma acc atomic UPDATE
  v // expected-error {{invalid statement after '#pragma acc atomic update'}}
    =
      x--; // expected-note {{expected operator expression here where the operator is one of '+', }}
  #pragma acc atomic UPDATE
  v // expected-error {{invalid statement after '#pragma acc atomic update'}}
    =
      ++x; // expected-note {{expected operator expression here where the operator is one of '+', }}
  #pragma acc atomic UPDATE
  v // expected-error {{invalid statement after '#pragma acc atomic update'}}
    =
      --x; // expected-note {{expected operator expression here where the operator is one of '+', }}
  #pragma acc atomic UPDATE
  v // expected-error {{invalid statement after '#pragma acc atomic update'}}
    = x
        /= // expected-note {{expected operator expression here where the operator is one of '+', }}
           89;
  #pragma acc atomic UPDATE
  v // expected-error {{invalid statement after '#pragma acc atomic update'}}
    = x
        = // expected-note {{expected operator expression here where the operator is one of '+', }}
          x & 5;
  #pragma acc atomic UPDATE
  v // expected-error {{invalid statement after '#pragma acc atomic update'}}
    = x
        = // expected-note {{expected operator expression here where the operator is one of '+', }}
          384 ^ x;
  // expected-error@+3 {{invalid statement after '#pragma acc atomic update'}}
  // expected-note@+2 {{expected statement here to be an expression statement}}
  #pragma acc atomic UPDATE
  { v = x; x |= 3; }

  //............................................................................
  // Try another non-expression statement.
  //............................................................................

  #pragma acc atomic UPDATE
  do { // expected-error {{invalid statement after '#pragma acc atomic update'}}
       // expected-note@-1 {{expected statement here to be an expression statement}}
    x++;
  } while (0);

  //............................................................................
  // Check expression statement.
  //............................................................................

  // Make sure a valid form isn't accepted within a compound statement.
  #pragma acc atomic UPDATE
  { // expected-error {{invalid statement after '#pragma acc atomic update'}}
    // expected-note@-1 {{expected statement here to be an expression statement}}
    x++;
  }

  // Check an expression statement that is not a unary/binary-operator
  // expression.
  #pragma acc atomic UPDATE
  foo(); // expected-error {{invalid statement after '#pragma acc atomic update'}}
         // expected-note@-1 {{expected operator expression here where the operator is one of '++', }}

  // Check a unary-operator expression that is not an increment or decrement.
  #pragma acc atomic UPDATE
  &foo; // expected-noacc-warning {{expression result unused}}
        // expected-error@-1 {{invalid statement after '#pragma acc atomic update'}}
        // expected-note@-2 {{expected operator expression here where the operator is one of '++', }}

  // Check a binary-operator expression that is not an assignment.
  #pragma acc atomic UPDATE
  x // expected-error {{invalid statement after '#pragma acc atomic update'}}
    + // expected-noacc-warning {{expression result unused}}
      // expected-note@-1 {{expected operator expression here where the operator is one of '++', }}
      v;

  // Check an unsupported assignment operator ('%=' as of OpenACC 3.2).
  #pragma acc atomic UPDATE
  x // expected-error {{invalid statement after '#pragma acc atomic update'}}
    %= // expected-note {{expected operator expression here where the operator is one of '++', }}
       34;

  // After assign, check an expression that is not a binary-operator expression.
  #pragma acc atomic UPDATE
  x // expected-error {{invalid statement after '#pragma acc atomic update'}}
    =
      ++v; // expected-note {{expected operator expression here where the operator is one of '+', }}

  // After assign, check an unsupported binary operator ('%' as of OpenACC 3.2).
  #pragma acc atomic UPDATE
  x // expected-error {{invalid statement after '#pragma acc atomic update'}}
    =
      x % 9; // expected-note {{expected operator expression here where the operator is one of '+', }}

  // Check the case where the assign's LHS is not an operand in the RHS.
  #pragma acc atomic UPDATE
  x = x + 8;
  #pragma acc atomic UPDATE
  x = v + x;
  #pragma acc atomic UPDATE
  x // expected-error {{invalid statement after '#pragma acc atomic update'}}
    // expected-note@-1 {{other expression appears here}}
    = i
        + // expected-note {{expected one operand of this expression to match other expression}}
          v;
  #pragma acc atomic UPDATE
  xs.i = xs.i + v;
  #pragma acc atomic UPDATE
  xs.i = v + xs.i;
  #pragma acc atomic UPDATE
  xs.i // expected-error {{invalid statement after '#pragma acc atomic update'}}
       // expected-note@-1 {{other expression appears here}}
       = xs.j
              + // expected-note {{expected one operand of this expression to match other expression}}
                 v;

  // Non-lvalue is generally not permitted for increment, decrement, or LHS of
  // assign, so some OpenACC checks don't run.
  #pragma acc atomic UPDATE
  10++; // expected-noacc-error {{expression is not assignable}}
        // expected-error@-1 {{invalid statement after '#pragma acc atomic update'}}
        // expected-note@-2 {{expected operator expression here where the operator is one of '++', }}
  #pragma acc atomic UPDATE
  10 // expected-error {{invalid statement after '#pragma acc atomic update'}}
     // expected-note@-1 {{expected operator expression here where the operator is one of '++', }}
     += // expected-noacc-error {{expression is not assignable}}
        10 + v;
  #pragma acc atomic UPDATE
  10 // expected-error {{invalid statement after '#pragma acc atomic update'}}
     // expected-note@-1 {{expected operator expression here where the operator is one of '++', }}
     = // expected-noacc-error {{expression is not assignable}}
       10 + v;

  // Non-scalar operands are generally not permitted in expressions accepted
  // here, so some OpenACC checks don't run.
  #pragma acc atomic UPDATE
  xs++; // expected-noacc-error {{cannot increment value of type 'struct S'}}
        // expected-error@-1 {{invalid statement after '#pragma acc atomic update'}}
        // expected-note@-2 {{expected operator expression here where the operator is one of '++', }}
  #pragma acc atomic UPDATE
  xs // expected-error {{invalid statement after '#pragma acc atomic update'}}
     // expected-note@-1 {{expected operator expression here where the operator is one of '++', }}
     *= // expected-noacc-error {{invalid operands to binary expression ('struct S' and 'struct S')}}
        vs;
  #pragma acc atomic UPDATE
  xs // expected-error {{invalid statement after '#pragma acc atomic update'}}
     // expected-note@-1 {{expected expression here to be of scalar type}}
     =
       xs // expected-note {{expected operator expression here where the operator is one of '+', }}
          - // expected-noacc-error {{invalid operands to binary expression ('struct S' and 'struct S')}}
            vs;
  #pragma acc atomic UPDATE
  xs // expected-error {{invalid statement after '#pragma acc atomic update'}}
     // expected-note@-1 {{expected expression here to be of scalar type}}
     =
       vs // expected-note {{expected operator expression here where the operator is one of '+', }}
          / // expected-noacc-error {{invalid operands to binary expression ('struct S' and 'struct S')}}
            xs;

#if !CHECK_IMPLICIT_UPDATE

  //----------------------------------------------------------------------------
  // acc atomic capture
  //----------------------------------------------------------------------------

  //............................................................................
  // Check forms listed in the spec.
  //............................................................................

  #pragma acc atomic capture
  v // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    =
      x; // expected-note {{expected operator expression here where the operator is one of '++', }}
  #pragma acc atomic capture
  x // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    =
      1; // expected-note {{expected operator expression here where the operator is one of '++', }}
  #pragma acc atomic capture
  x++; // expected-error {{invalid statement after '#pragma acc atomic capture'}}
       // expected-note@-1 {{expected expression here to be a simple assignment}}
  #pragma acc atomic capture
  x--; // expected-error {{invalid statement after '#pragma acc atomic capture'}}
       // expected-note@-1 {{expected expression here to be a simple assignment}}
  #pragma acc atomic capture
  ++x; // expected-error {{invalid statement after '#pragma acc atomic capture'}}
       // expected-note@-1 {{expected expression here to be a simple assignment}}
  #pragma acc atomic capture
  --x; // expected-error {{invalid statement after '#pragma acc atomic capture'}}
       // expected-note@-1 {{expected expression here to be a simple assignment}}
  #pragma acc atomic capture
  x // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    += // expected-note {{expected expression here to be a simple assignment}}
       3;
  #pragma acc atomic capture
  x // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    = x
        - // expected-note {{expected operator expression here where the operator is one of '++', }}
          9;
  #pragma acc atomic capture
  x // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    = 10
         * // expected-note {{expected operator expression here where the operator is one of '++', }}
           x;
  #pragma acc atomic capture
  v = x++;
  #pragma acc atomic capture
  v = x--;
  #pragma acc atomic capture
  v = ++x;
  #pragma acc atomic capture
  v = --x;
  #pragma acc atomic capture
  v = x /= 89;
  #pragma acc atomic capture
  v = x = x & 5;
  #pragma acc atomic capture
  v = x = 384 ^ x;
  #pragma acc atomic capture
  { v = x; x |= 3; }
  #pragma acc atomic capture
  { x <<= 3; v = x; }
  #pragma acc atomic capture
  { v = x; x = x >> 3; }
  #pragma acc atomic capture
  { v = x; x = 3 + x; }
  #pragma acc atomic capture
  { x = x * 3; v = x; }
  #pragma acc atomic capture
  { x = 3 - x; v = x; }
  #pragma acc atomic capture
  { v = x; x = 3; }
  #pragma acc atomic capture
  { v = x; x++; }
  #pragma acc atomic capture
  { v = x; ++x; }
  #pragma acc atomic capture
  { ++x; v = x; }
  #pragma acc atomic capture
  { x++; v = x; }
  #pragma acc atomic capture
  { v = x; x--; }
  #pragma acc atomic capture
  { v = x; --x; }
  #pragma acc atomic capture
  { --x; v = x; }
  #pragma acc atomic capture
  { x--; v = x; }

  //............................................................................
  // Try a statement that is not an expression statement or compound statement.
  //............................................................................

  #pragma acc atomic capture
  do { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
       // expected-note@-1 {{expected statement here to be an expression statement or compound statement}}
    x++;
  } while (0);
  #pragma acc atomic capture
  do { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
       // expected-note@-1 {{expected statement here to be an expression statement or compound statement}}
    {
      v = x;
      x += 3;
    }
  } while (0);

  //............................................................................
  // Check expression statement.
  //............................................................................

  // Make sure a valid form for an expression statement isn't accepted within
  // compound statement.
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    // expected-note@-1 {{expected compound statement here to contain exactly two statements}}
    v = x++;
  }

  // Check an expression statement that is not a binary-operator expression.
  #pragma acc atomic capture
  foo(); // expected-error {{invalid statement after '#pragma acc atomic capture'}}
         // expected-note@-1 {{expected expression here to be a simple assignment}}

  // Check a binary-operator expression that is not a simple assignment.
  #pragma acc atomic capture
  x // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    += // expected-note {{expected expression here to be a simple assignment}}
       v;

  // After assign, check an expression statement that is not a unary- or
  // binary-operator expression.
  #pragma acc atomic capture
  v // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    =
      foo(); // expected-note {{expected operator expression here where the operator is one of '++', }}

  // After assign, check a unary-operator expression that is not an increment or
  // decrement.
  #pragma acc atomic capture
  v // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    =
      -3; // expected-note {{expected operator expression here where the operator is one of '++', }}

  // After assign, check a binary-operator expression that is not an assignment.
  #pragma acc atomic capture
  v // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    = x
        + // expected-note {{expected operator expression here where the operator is one of '++', }}
          v;

  // After assign, check an unsupported assignment operator ('%=' as of OpenACC
  // 3.2).
  #pragma acc atomic capture
  v // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    = x
        %= // expected-note {{expected operator expression here where the operator is one of '++', }}
           34;

  // After two assigns, check an expression that is not a binary-operator
  // expression.
  #pragma acc atomic capture
  v // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    = x =
          ++i; // expected-note {{expected operator expression here where the operator is one of '+', }}

  // After two assigns, check an unsupported binary operator ('%' as of OpenACC
  // 3.2).
  #pragma acc atomic capture
  v // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    = x = x
            % // expected-note {{expected operator expression here where the operator is one of '+', }}
              9;

  // After assign, check the case where the next assign's LHS is not an operand
  // in the RHS.
  #pragma acc atomic capture
  v = x = x + 8;
  #pragma acc atomic capture
  v = x = v + x;
  #pragma acc atomic capture
  v // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    =
      x // expected-note {{other expression appears here}}
        = i
            + // expected-note {{expected one operand of this expression to match other expression}}
              v;
  #pragma acc atomic capture
  vs.i = xs.i = xs.i + v;
  #pragma acc atomic capture
  vs.i = xs.i = v + xs.i;
  #pragma acc atomic capture
  vs.i // expected-error {{invalid statement after '#pragma acc atomic capture'}}
       =
         xs.i // expected-note {{other expression appears here}}
              = xs.j
                     + // expected-note {{expected one operand of this expression to match other expression}}
                       v;

  // Non-lvalue is generally not permitted for LHS of assign, so some OpenACC
  // checks don't run.
  #pragma acc atomic capture
  10 // expected-error {{invalid statement after '#pragma acc atomic capture'}}
     // expected-note@-1 {{expected expression here to be a simple assignment}}
     = // expected-noacc-error {{expression is not assignable}}
       x++;

  // After assignment, non-lvalue is generally not permitted for increment,
  // decrement, or LHS of assign, so some OpenACC checks don't run.
  #pragma acc atomic capture
  v // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    =
      10++; // expected-noacc-error {{expression is not assignable}}
            // expected-note@-1 {{expected operator expression here where the operator is one of '++', }}
  #pragma acc atomic capture
  v // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    =
      10 // expected-note {{expected operator expression here where the operator is one of '++', }}
         += // expected-noacc-error {{expression is not assignable}}
            10 + v;
  #pragma acc atomic capture
  v // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    =
      10 // expected-note {{expected operator expression here where the operator is one of '++', }}
         = // expected-noacc-error {{expression is not assignable}}
           10 + v;

  // Non-scalar operands are generally not permitted in expressions accepted
  // here, so some OpenACC checks don't run.
  #pragma acc atomic capture
  vs // expected-error {{invalid statement after '#pragma acc atomic capture'}}
     // expected-note@-1 {{expected expression here to be of scalar type}}
     =
       xs++; // expected-noacc-error {{cannot increment value of type 'struct S'}}
             // expected-note@-1 {{expected operator expression here where the operator is one of '++', }}
  #pragma acc atomic capture
  vs // expected-error {{invalid statement after '#pragma acc atomic capture'}}
     // expected-note@-1 {{expected expression here to be of scalar type}}
     =
       xs // expected-note {{expected operator expression here where the operator is one of '++', }}
          *= // expected-noacc-error {{invalid operands to binary expression ('struct S' and 'struct S')}}
             vs;
  #pragma acc atomic capture
  vs // expected-error {{invalid statement after '#pragma acc atomic capture'}}
     // expected-note@-1 {{expected expression here to be of scalar type}}
     =
       xs // expected-note {{expected expression here to be of scalar type}}
          =
            xs // expected-note {{expected operator expression here where the operator is one of '+', }}
               - // expected-noacc-error {{invalid operands to binary expression ('struct S' and 'struct S')}}
                 vs;
  #pragma acc atomic capture
  vs // expected-error {{invalid statement after '#pragma acc atomic capture'}}
     // expected-note@-1 {{expected expression here to be of scalar type}}
     =
       xs // expected-note {{expected expression here to be of scalar type}}
          =
            vs // expected-note {{expected operator expression here where the operator is one of '+', }}
               / // expected-noacc-error {{invalid operands to binary expression ('struct S' and 'struct S')}}
                 xs;

  //............................................................................
  // Check compound statement.
  //............................................................................

  // Make sure a valid form for a compound statement isn't accepted within
  // another compound statement.
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    // expected-note@-1 {{expected compound statement here to contain exactly two statements}}
    { // expected-note {{expected statement here to be an expression statement}}
      v = x;
      x += 3;
    }
  }

  // Check wrong number of statements.
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    // expected-note@-1 {{expected compound statement here to contain exactly two statements}}
  }
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    // expected-note@-1 {{expected compound statement here to contain exactly two statements}}
    v = x;
  }
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    // expected-note@-1 {{expected compound statement here to contain exactly two statements}}
    x += 5;
  }
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    // expected-note@-1 {{expected compound statement here to contain exactly two statements}}
    v = x;
    x += 5;
    x -= 3;
  }

  // Check wrong kinds of statements.
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    { // expected-note {{expected statement here to be an expression statement}}
      v = x;
    }
    x += 5;
  }
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    x += 5;
    if (x) { // expected-note {{expected statement here to be an expression statement}}
      v = x;
    }
  }
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    ; // expected-note {{expected statement here to be an expression statement}}
    ; // expected-note {{expected statement here to be an expression statement}}
  }

  // Check wrong number and kinds of statements.
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    // expected-note@-1 {{expected compound statement here to contain exactly two statements}}
    ; // expected-note {{expected statement here to be an expression statement}}
  }
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    // expected-note@-1 {{expected compound statement here to contain exactly two statements}}
    ; // expected-note {{expected statement here to be an expression statement}}
    ; // expected-note {{expected statement here to be an expression statement}}
    ;
  }

  // Check missing read statement.
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    // expected-note@-1 {{expected compound statement here to contain one read statement (i.e., form 'v = x;' where v and x are lvalues)}}
    v += x;
    ++x;
  }
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    // expected-note@-1 {{expected compound statement here to contain one read statement (i.e., form 'v = x;' where v and x are lvalues)}}
    v = 10;
    x += 3;
  }
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    // expected-note@-1 {{expected compound statement here to contain one read statement (i.e., form 'v = x;' where v and x are lvalues)}}
    ++x;
    v = 10;
  }

  // Check read statement followed by invalid write/update statement form.
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    v = x;
    foo(); // expected-note {{expected operator expression here where the operator is one of '++', }}
  }
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    v = x;
    &foo; // expected-noacc-warning {{expression result unused}}
          // expected-note@-1 {{expected operator expression here where the operator is one of '++', }}
  }
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    v = x;
    x
      + // expected-noacc-warning {{expression result unused}}
        // expected-note@-1 {{expected operator expression here where the operator is one of '++', }}
        v;
  }
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    v = x;
    x
      %= // expected-note {{expected operator expression here where the operator is one of '++', }}
         34;
  }

  // Check read statement followed by invalid update statement form that is
  // accepted as a write statement.
  #pragma acc atomic capture
  {
    v = x;
    x = ++i;
  }
  #pragma acc atomic capture
  {
    v = x;
    x = x % 9; // Currently accepted but invalid because RHS contains x.
  }
  #pragma acc atomic capture
  {
    v = x;
    x = i + jk;
  }

  // Check read and write statements reversed.  The first statement is then
  // validated as an update statement and rejected.
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    x =
        10; // expected-note {{expected operator expression here where the operator is one of '+', }}
    v = x;
  }

  // Check invalid update statement followed by read statement.
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    foo(); // expected-note {{expected operator expression here where the operator is one of '++', }}
    v = x;
  }
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    &foo; // expected-noacc-warning {{expression result unused}}
          // expected-note@-1 {{expected operator expression here where the operator is one of '++', }}
    v = x;
  }
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    x
      + // expected-noacc-warning {{expression result unused}}
        // expected-note@-1 {{expected operator expression here where the operator is one of '++', }}
        v;
    v = x;
  }
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    x
      %= // expected-note {{expected operator expression here where the operator is one of '++', }}
         34;
    v = x;
  }
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    x =
        ++v; // expected-note {{expected operator expression here where the operator is one of '+', }}
    v = x;
  }
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    x =
        x % 9; // expected-note {{expected operator expression here where the operator is one of '+', }}
    v = x;
  }
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    x // expected-note {{other expression appears here}}
      = i
          + // expected-note {{expected one operand of this expression to match other expression}}
            v;
    v = x;
  }
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    xs.i // expected-note {{other expression appears here}}
         = xs.j
                + // expected-note {{expected one operand of this expression to match other expression}}
                  v;
    vs.i = xs.i;
  }

  // Check case where the x from the read statement does not match the x from
  // the write/update statement.
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    v =
        x // expected-note {{other expression appears here}}
          ;
    i // expected-note {{expected this expression to match other expression}}
      = 10;
  }
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    v =
        x // expected-note {{other expression appears here}}
          ;
    i // expected-note {{expected this expression to match other expression}}
      = i + 10;
  }
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    i // expected-note {{expected this expression to match other expression}}
     ++;
    v =
        x; // expected-note {{other expression appears here}}
  }

  // Non-lvalue is generally not permitted for increment, decrement, or LHS of
  // assign, so some OpenACC checks don't run.
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    // expected-note@-1 {{expected compound statement here to contain one read statement (i.e., form 'v = x;' where v and x are lvalues}}
    v = 10;
    10++; // expected-noacc-error {{expression is not assignable}}
  }
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    // expected-note@-1 {{expected compound statement here to contain one read statement (i.e., form 'v = x;' where v and x are lvalues}}
    10 = x; // expected-noacc-error {{expression is not assignable}}
    x++;
  }

  // Non-scalar operands are generally not permitted in expressions accepted
  // here, so some OpenACC checks don't run.
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    vs // expected-note {{expected expression here to be of scalar type}}
       =
         xs; // expected-note {{expected expression here to be of scalar type}}
    xs++; // expected-noacc-error {{cannot increment value of type 'struct S'}}
          // expected-note@-1 {{expected operator expression here where the operator is one of '++', }}
  }
  #pragma acc atomic capture
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    xs++; // expected-noacc-error {{cannot increment value of type 'struct S'}}
          // expected-note@-1 {{expected operator expression here where the operator is one of '++', }}
    vs // expected-note {{expected expression here to be of scalar type}}
       =
         xs; // expected-note {{expected expression here to be of scalar type}}
  }

  //----------------------------------------------------------------------------
  // Enclosed directive.
  //----------------------------------------------------------------------------

  #pragma acc atomic capture // expected-note {{enclosing '#pragma acc atomic' here}}
  {
    v = x;
    #pragma acc update host(x) // expected-error {{'#pragma acc update' cannot be nested within '#pragma acc atomic'}}
    x++;
  }

  #pragma acc atomic capture // expected-note {{enclosing '#pragma acc atomic' here}}
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    // expected-note@-1 {{expected compound statement here to contain exactly two statements}}
    v = x;
    #pragma acc enter data copyin(x) // expected-error {{'#pragma acc enter data' cannot be nested within '#pragma acc atomic'}}
  }

  #pragma acc atomic capture // expected-note {{enclosing '#pragma acc atomic' here}}
  { // expected-error {{invalid statement after '#pragma acc atomic capture'}}
    // expected-note@-1 {{expected compound statement here to contain exactly two statements}}
    #pragma acc exit data copyout(x) // expected-error {{'#pragma acc exit data' cannot be nested within '#pragma acc atomic'}}
    v = x;
  }

  #pragma acc atomic read // expected-note {{enclosing '#pragma acc atomic' here}}
  #pragma acc parallel // expected-error {{'#pragma acc parallel' cannot be nested within '#pragma acc atomic'}}
  v = x;

  #pragma acc atomic write // expected-note {{enclosing '#pragma acc atomic' here}}
  #pragma acc loop // expected-error {{'#pragma acc loop' cannot be nested within '#pragma acc atomic'}}
  for (int i = 0; i < 5; ++i)
    x++;

  #pragma acc atomic update // expected-note {{enclosing '#pragma acc atomic' here}}
  #pragma acc parallel loop // expected-error {{'#pragma acc parallel loop' cannot be nested within '#pragma acc atomic'}}
  for (int i = 0; i < 5; ++i)
    x++;

  #pragma acc atomic read // expected-note {{enclosing '#pragma acc atomic' here}}
  #pragma acc atomic read // expected-error {{'#pragma acc atomic' cannot be nested within '#pragma acc atomic'}}
  v = x;
  #pragma acc atomic write // expected-note {{enclosing '#pragma acc atomic' here}}
  #pragma acc atomic // expected-error {{'#pragma acc atomic' cannot be nested within '#pragma acc atomic'}}
  x++;

#endif // !CHECK_IMPLICIT_UPDATE

  OUTER_DIR_END
  return 0;
}