// Check diagnostics for "acc wait".
//
// OpenACC disabled
// RUN: %clang_cc1 -verify=noacc,expected-noacc %s

// OpenACC enabled
// RUN: %clang_cc1 -DACC -verify=expected,expected-noacc -fopenacc %s

// END.

// noacc-no-diagnostics

int main() {
  int i, jk;
  float f;

  //============================================================================
  // No clauses.
  //============================================================================

  #pragma acc wait

  //============================================================================
  // Unrecognized clauses.
  //============================================================================

  //----------------------------------------------------------------------------
  // Bogus clauses.
  //----------------------------------------------------------------------------

  // expected-warning@+1 {{extra tokens at the end of '#pragma acc wait' are ignored}}
  #pragma acc wait foo
  // expected-warning@+1 {{extra tokens at the end of '#pragma acc wait' are ignored}}
  #pragma acc wait foo bar

  //----------------------------------------------------------------------------
  // Well formed clauses not permitted here.  Make sure every one is an error.
  //----------------------------------------------------------------------------

  // expected-error@+8 {{unexpected OpenACC clause 'nomap' in directive '#pragma acc wait'}}
  // expected-error@+7 {{unexpected OpenACC clause 'present' in directive '#pragma acc wait'}}
  // expected-error@+6 {{unexpected OpenACC clause 'copy' in directive '#pragma acc wait'}}
  // expected-error@+5 {{unexpected OpenACC clause 'copyin' in directive '#pragma acc wait'}}
  // expected-error@+4 {{unexpected OpenACC clause 'copyout' in directive '#pragma acc wait'}}
  // expected-error@+3 {{unexpected OpenACC clause 'create' in directive '#pragma acc wait'}}
  // expected-error@+2 {{unexpected OpenACC clause 'no_create' in directive '#pragma acc wait'}}
  // expected-error@+1 {{unexpected OpenACC clause 'delete' in directive '#pragma acc wait'}}
  #pragma acc wait nomap(i) present(i) copy(i) copyin(i) copyout(i) create(i) no_create(i) delete(i)
  // expected-error@+4 {{unexpected OpenACC clause 'pcopy' in directive '#pragma acc wait'}}
  // expected-error@+3 {{unexpected OpenACC clause 'pcopyin' in directive '#pragma acc wait'}}
  // expected-error@+2 {{unexpected OpenACC clause 'pcopyout' in directive '#pragma acc wait'}}
  // expected-error@+1 {{unexpected OpenACC clause 'pcreate' in directive '#pragma acc wait'}}
  #pragma acc wait pcopy(i) pcopyin(i) pcopyout(i) pcreate(i)
  // expected-error@+4 {{unexpected OpenACC clause 'present_or_copy' in directive '#pragma acc wait'}}
  // expected-error@+3 {{unexpected OpenACC clause 'present_or_copyin' in directive '#pragma acc wait'}}
  // expected-error@+2 {{unexpected OpenACC clause 'present_or_copyout' in directive '#pragma acc wait'}}
  // expected-error@+1 {{unexpected OpenACC clause 'present_or_create' in directive '#pragma acc wait'}}
  #pragma acc wait present_or_copy(i) present_or_copyin(i) present_or_copyout(i) present_or_create(i)
  // expected-error@+4 {{unexpected OpenACC clause 'shared' in directive '#pragma acc wait'}}
  // expected-error@+3 {{unexpected OpenACC clause 'reduction' in directive '#pragma acc wait'}}
  // expected-error@+2 {{unexpected OpenACC clause 'private' in directive '#pragma acc wait'}}
  // expected-error@+1 {{unexpected OpenACC clause 'firstprivate' in directive '#pragma acc wait'}}
  #pragma acc wait shared(i, jk) reduction(+:i,jk,f) private(i) firstprivate(i)
  // expected-error@+5 {{unexpected OpenACC clause 'if' in directive '#pragma acc wait'}}
  // expected-error@+4 {{unexpected OpenACC clause 'if_present' in directive '#pragma acc wait'}}
  // expected-error@+3 {{unexpected OpenACC clause 'self' in directive '#pragma acc wait'}}
  // expected-error@+2 {{unexpected OpenACC clause 'host' in directive '#pragma acc wait'}}
  // expected-error@+1 {{unexpected OpenACC clause 'device' in directive '#pragma acc wait'}}
  #pragma acc wait if(1) if_present self(i) host(jk) device(f)
  // expected-error@+3 {{unexpected OpenACC clause 'num_gangs' in directive '#pragma acc wait'}}
  // expected-error@+2 {{unexpected OpenACC clause 'num_workers' in directive '#pragma acc wait'}}
  // expected-error@+1 {{unexpected OpenACC clause 'vector_length' in directive '#pragma acc wait'}}
  #pragma acc wait num_gangs(1) num_workers(2) vector_length(3)
  // expected-error@+6 {{unexpected OpenACC clause 'seq' in directive '#pragma acc wait'}}
  // expected-error@+5 {{unexpected OpenACC clause 'auto' in directive '#pragma acc wait'}}
  // expected-error@+4 {{unexpected OpenACC clause 'independent' in directive '#pragma acc wait'}}
  // expected-error@+3 {{unexpected OpenACC clause 'gang' in directive '#pragma acc wait'}}
  // expected-error@+2 {{unexpected OpenACC clause 'worker' in directive '#pragma acc wait'}}
  // expected-error@+1 {{unexpected OpenACC clause 'vector' in directive '#pragma acc wait'}}
  #pragma acc wait seq independent auto gang worker vector
  // expected-error@+2 {{unexpected OpenACC clause 'collapse' in directive '#pragma acc wait'}}
  // expected-error@+1 {{unexpected OpenACC clause 'tile' in directive '#pragma acc wait'}}
  #pragma acc wait collapse(1) tile(6)
  // expected-error@+2 {{unexpected OpenACC clause 'async' in directive '#pragma acc wait'}}
  // expected-error@+1 {{unexpected OpenACC clause 'wait' in directive '#pragma acc wait'}}
  #pragma acc wait async wait
  // expected-error@+5 {{unexpected OpenACC clause 'read' in directive '#pragma acc wait'}}
  // expected-error@+4 {{unexpected OpenACC clause 'write' in directive '#pragma acc wait'}}
  // expected-error@+3 {{unexpected OpenACC clause 'update' in directive '#pragma acc wait'}}
  // expected-error@+2 {{unexpected OpenACC clause 'capture' in directive '#pragma acc wait'}}
  // expected-error@+1 {{unexpected OpenACC clause 'compare' in directive '#pragma acc wait'}}
  #pragma acc wait read write update capture compare

  //----------------------------------------------------------------------------
  // Malformed clauses not permitted here.
  //----------------------------------------------------------------------------

  // expected-error@+2 {{unexpected OpenACC clause 'shared' in directive '#pragma acc wait'}}
  // expected-error@+1 {{expected '(' after 'shared'}}
  #pragma acc wait shared
  // expected-error@+4 {{unexpected OpenACC clause 'shared' in directive '#pragma acc wait'}}
  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc wait shared(
  // expected-error@+3 {{unexpected OpenACC clause 'shared' in directive '#pragma acc wait'}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc wait shared(i

  //============================================================================
  // Nesting.
  //============================================================================

  //----------------------------------------------------------------------------
  // As immediate substatement.
  //----------------------------------------------------------------------------

  // Only check the parse errors here when OpenACC directives are included
  // because there are different parse errors when they're ignored.
#ifdef ACC
  if (i)
    // expected-error@+1 {{'#pragma acc wait' cannot be an immediate substatement}}
    #pragma acc wait

  if (i)
    ;
  else
    // expected-error@+1 {{'#pragma acc wait' cannot be an immediate substatement}}
    #pragma acc wait

  while (i)
    // expected-error@+1 {{'#pragma acc wait' cannot be an immediate substatement}}
    #pragma acc wait

  do
    // expected-error@+1 {{'#pragma acc wait' cannot be an immediate substatement}}
    #pragma acc wait
  while (i);

  for (;;)
    // expected-error@+1 {{'#pragma acc wait' cannot be an immediate substatement}}
    #pragma acc wait

  switch (i)
    // expected-error@+1 {{'#pragma acc wait' cannot be an immediate substatement}}
    #pragma acc wait

  // Both the OpenACC 3.0 and OpenMP TR8 say the following cases (labeled
  // statements) are not permitted, but Clang permits them for both OpenACC and
  // OpenMP.
  switch (i) {
  case 0:
    #pragma acc wait
  }
  switch (i) {
  default:
    #pragma acc wait
  }
#endif

  lab:
    #pragma acc wait

  #pragma acc data copy(i)
  // expected-error@+1 {{'#pragma acc wait' cannot be an immediate substatement}}
  #pragma acc wait

  // expected-note@+1 {{enclosing '#pragma acc parallel' here}}
  #pragma acc parallel
  // expected-error@+2 {{'#pragma acc wait' cannot be an immediate substatement}}
  // expected-error@+1 {{'#pragma acc wait' cannot be nested within '#pragma acc parallel'}}
  #pragma acc wait

  //----------------------------------------------------------------------------
  // In other constructs not as immediate substatement.
  //----------------------------------------------------------------------------

  #pragma acc data copy(i)
  {
    #pragma acc wait
  }

  // expected-note@+1 {{enclosing '#pragma acc parallel' here}}
  #pragma acc parallel
  {
    // expected-error@+1 {{'#pragma acc wait' cannot be nested within '#pragma acc parallel'}}
    #pragma acc wait
  }

  // expected-note@+1 {{enclosing '#pragma acc parallel loop' here}}
  #pragma acc parallel loop
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc wait' cannot be nested within '#pragma acc parallel loop'}}
    #pragma acc wait
  }

  #pragma acc parallel
  // expected-note@+1 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc wait' cannot be nested within '#pragma acc loop'}}
    #pragma acc wait
  }

  #pragma acc parallel
  // expected-note@+1 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc wait' cannot be nested within '#pragma acc loop'}}
    #pragma acc wait
  }

  #pragma acc parallel loop
  for (int i = 0; i < 5; ++i)
  // expected-note@+1 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop
  for (int j = 0; j < 5; ++j) {
    // expected-error@+1 {{'#pragma acc wait' cannot be nested within '#pragma acc loop'}}
    #pragma acc wait
  }

  return 0;
}

// expected-error@+1 {{unexpected OpenACC directive '#pragma acc wait'}}
#pragma acc wait

void impSeqRoutine() {
  // expected-error@+3 {{'#pragma acc wait' is not permitted within function 'impSeqRoutine' because the latter is attributed with '#pragma acc routine'}}
  // expected-note@#impSeqRoutine_call {{'#pragma acc routine seq' implied for function 'impSeqRoutine' by its use in function 'expSeqRoutine' here}}
  // expected-note@#expSeqRoutine_routine {{'#pragma acc routine' for function 'expSeqRoutine' appears here}}
  #pragma acc wait
}

#pragma acc routine seq // #expSeqRoutine_routine
void expSeqRoutine() {
  // expected-error@+2 {{'#pragma acc wait' is not permitted within function 'expSeqRoutine' because the latter is attributed with '#pragma acc routine'}}
  // expected-note@#expSeqRoutine_routine {{'#pragma acc routine' for function 'expSeqRoutine' appears here}}
  #pragma acc wait
  impSeqRoutine(); // #impSeqRoutine_call
}
