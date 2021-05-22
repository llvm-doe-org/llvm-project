// Check diagnostics for "acc data".

// OpenACC disabled
// RUN: %clang_cc1 -verify=noacc,expected-noacc -Wchar-subscripts %s

// OpenACC enabled
// RUN: %clang_cc1 -verify=expected,expected-noacc,char-subscripts -fopenacc \
// RUN:            -Wchar-subscripts %s

// OpenACC enabled but optional warnings disabled
// RUN: %clang_cc1 -verify=expected,expected-noacc -fopenacc %s

// END.

int incomplete[];

int main() {
  char c;
  enum { E1, E2 } e;
  int i, jk, a[2], m[6][2], *p;
  int (*fp)();
  float f;
  const int constI = 5;
  const extern int constIDecl;
  // expected-note@+2 {{variable 'constA' declared const here}}
  // expected-noacc-note@+1 {{variable 'constA' declared const here}}
  const int constA[3];
  // expected-noacc-note@+1 {{variable 'constADecl' declared const here}}
  const extern int constADecl[3];

  //--------------------------------------------------
  // No clauses
  //--------------------------------------------------

  // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
  #pragma acc data
    ;

  // Any data clause is sufficient to suppress that diagnostic.
  #pragma acc data present(i)
    ;
  #pragma acc data copy(i)
    ;
  #pragma acc data pcopy(i)
    ;
  #pragma acc data present_or_copy(i)
    ;
  #pragma acc data copyin(i)
    ;
  #pragma acc data pcopyin(i)
    ;
  #pragma acc data present_or_copyin(i)
    ;
  #pragma acc data copyout(i)
    ;
  #pragma acc data pcopyout(i)
    ;
  #pragma acc data present_or_copyout(i)
    ;
  #pragma acc data create(i)
    ;
  #pragma acc data no_create(i)
    ;

  //--------------------------------------------------
  // Unrecognized clauses
  //--------------------------------------------------

  // Bogus clauses.

  // expected-warning@+2 {{extra tokens at the end of '#pragma acc data' are ignored}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
  #pragma acc data foo
    ;
  // expected-warning@+2 {{extra tokens at the end of '#pragma acc data' are ignored}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
  #pragma acc data foo bar
    ;

  // Well formed clauses not permitted here.

  // expected-error@+7 {{unexpected OpenACC clause 'nomap' in directive '#pragma acc data'}}
  // expected-error@+6 {{unexpected OpenACC clause 'delete' in directive '#pragma acc data'}}
  // expected-error@+5 {{unexpected OpenACC clause 'shared' in directive '#pragma acc data'}}
  // expected-error@+4 {{unexpected OpenACC clause 'reduction' in directive '#pragma acc data'}}
  // expected-error@+3 {{unexpected OpenACC clause 'private' in directive '#pragma acc data'}}
  // expected-error@+2 {{unexpected OpenACC clause 'firstprivate' in directive '#pragma acc data'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
  #pragma acc data nomap(i) delete(i) shared(i, jk) reduction(+:i,jk,f) private(i) firstprivate(i)
    ;
  // expected-error@+6 {{unexpected OpenACC clause 'if' in directive '#pragma acc data'}}
  // expected-error@+5 {{unexpected OpenACC clause 'if_present' in directive '#pragma acc data'}}
  // expected-error@+4 {{unexpected OpenACC clause 'self' in directive '#pragma acc data'}}
  // expected-error@+3 {{unexpected OpenACC clause 'host' in directive '#pragma acc data'}}
  // expected-error@+2 {{unexpected OpenACC clause 'device' in directive '#pragma acc data'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
  #pragma acc data if(1) if_present self(i) host(i) device(i)
  // expected-error@+3 {{unexpected OpenACC clause 'num_gangs' in directive '#pragma acc data'}}
  // expected-error@+2 {{unexpected OpenACC clause 'num_workers' in directive '#pragma acc data'}}
  // expected-error@+1 {{unexpected OpenACC clause 'vector_length' in directive '#pragma acc data'}}
  #pragma acc data copy(i) num_gangs(1) num_workers(2) vector_length(3)
    ;
  // expected-error@+11 {{unexpected OpenACC clause 'independent' in directive '#pragma acc data'}}
  // expected-error@+10 {{unexpected OpenACC clause 'auto' in directive '#pragma acc data'}}
  // expected-error@+9 {{unexpected OpenACC clause 'seq' in directive '#pragma acc data'}}
  // expected-error@+8 {{unexpected OpenACC clause 'gang' in directive '#pragma acc data'}}
  // expected-error@+7 {{unexpected OpenACC clause 'worker' in directive '#pragma acc data'}}
  // expected-error@+6 {{unexpected OpenACC clause 'vector' in directive '#pragma acc data'}}
  // expected-error@+5 {{unexpected OpenACC clause 'auto', 'independent' is specified already}}
  // expected-error@+4 {{unexpected OpenACC clause 'seq', 'independent' is specified already}}
  // expected-error@+3 {{unexpected OpenACC clause 'gang', 'seq' is specified already}}
  // expected-error@+2 {{unexpected OpenACC clause 'worker', 'seq' is specified already}}
  // expected-error@+1 {{unexpected OpenACC clause 'vector', 'seq' is specified already}}
  #pragma acc data independent copy(i) auto seq gang worker vector
    ;
  // expected-error@+1 {{unexpected OpenACC clause 'collapse' in directive '#pragma acc data'}}
  #pragma acc data collapse(1) copy(i)
    ;

  // Malformed clauses not permitted here.

  // expected-error@+2 {{unexpected OpenACC clause 'shared' in directive '#pragma acc data'}}
  // expected-error@+1 {{expected '(' after 'shared'}}
  #pragma acc data copy(i) shared
    ;
  // expected-error@+4 {{unexpected OpenACC clause 'shared' in directive '#pragma acc data'}}
  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc data copy(i) shared(
    ;
  // expected-error@+4 {{unexpected OpenACC clause 'shared' in directive '#pragma acc data'}}
  // expected-error@+3 {{expected ')'}}
  // expected-note@+2 {{to match this '('}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
  #pragma acc data shared(i
    ;

  //--------------------------------------------------
  // Data clauses: syntax
  //
  // These clauses are checked thoroughly in parallel-messages.c, so just check
  // a cross section of cases here to confirm it works for the data directive.
  //--------------------------------------------------

  // expected-error@+2 {{expected '(' after 'present'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
  #pragma acc data present
    ;
  // expected-error@+4 {{expected expression}}
  // expected-error@+3 {{expected ')'}}
  // expected-note@+2 {{to match this '('}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
  #pragma acc data copy(
    ;
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc data pcopy(jk
    ;
  // expected-error@+2 {{expected expression}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
  #pragma acc data present_or_copy()
    ;
  // expected-error@+1 {{expected ',' or ')' in 'copyin' clause}}
  #pragma acc data copyin(i jk)
    ;
  // expected-error@+1 {{expected expression}}
  #pragma acc data pcopyin(jk ,)
    ;
  // expected-error@+2 {{expected variable name or subarray}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
  #pragma acc data present_or_copyin((int)i)
    ;
  // expected-error@+2 {{expected variable name as base of subarray}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
  #pragma acc data copyout((*(int(*)[3])a)[0:])
    ;
  // expected-error@+2 {{subscripted value is not an array or pointer}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
  #pragma acc data pcopyout(i[c:c])
    ;
  // expected-error@+2 {{subscripted value is not an array or pointer}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
  #pragma acc data present_or_copyout(i[0:2])
    ;
  // char-subscripts-warning@+1 {{subarray start is of type 'char'}}
  #pragma acc data create(a[c:2])
    ;
  // char-subscripts-warning@+1 {{subarray length is of type 'char'}}
  #pragma acc data pcreate(a[0:c])
    ;
  // expected-error@+2 {{subarray specified for pointer to function type 'int ()'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
  #pragma acc data present_or_create(fp[0:2])
    ;
  // expected-error@+2 {{subarray specified for pointer to incomplete type 'int []'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
  #pragma acc data no_create((&incomplete)[0:2])
    ;

  //--------------------------------------------------
  // Data clauses: arg semantics
  //
  // These clauses are checked thoroughly in parallel-messages.c, so just check
  // a cross section of cases here to confirm it works for the data directive.
  //--------------------------------------------------

  // expected-error@+2 {{use of undeclared identifier 'foo'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
  #pragma acc data copy(foo)
  // expected-error@+2 {{subarray syntax must include ':'}}
  // expected-error@+2 {{subarray syntax must include ':'}}
  #pragma acc data copyin(a[jk], \
                          m[2:][1], \
                          a[:], m[0:2][0:2])
    ;
  // expected-error@+2 {{variable in 'copyout' clause cannot have incomplete type 'int []'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
  #pragma acc data copyout(incomplete)
    ;
  #pragma acc data pcopy(constI, constIDecl)
    ;
  // expected-error@+1 {{const variable cannot be written by 'pcopyout'}}
  #pragma acc data pcopyin(constADecl) pcopyout(constA)
  {
    // expected-noacc-error@+2 {{cannot assign to variable 'constA' with const-qualified type 'const int [3]'}}
    // expected-noacc-error@+2 {{cannot assign to variable 'constADecl' with const-qualified type 'const int [3]'}}
    constA[0] = 5;
    constADecl[1] = 5;
  }
  // expected-error@+6 {{copy variable defined again as copy variable}}
  // expected-note@+5 {{previously defined as copy variable here}}
  // expected-error@+5 {{copy variable defined again as copy variable}}
  // expected-note@+3 {{previously defined as copy variable here}}
  // expected-error@+4 {{copy variable defined again as copy variable}}
  // expected-note@+1 {{previously defined as copy variable here}}
  #pragma acc data present_or_copy(e,e,i,jk) \
                   pcopy(i)                  \
                   copy(jk)
    ;
  // expected-error@+5 {{copy variable cannot be copyin variable}}
  // expected-note@+3 {{previously defined as copy variable here}}
  // expected-error@+4 {{copyin variable cannot be copyout variable}}
  // expected-note@+2 {{previously defined as copyin variable here}}
  #pragma acc data pcopy(a) \
                   present_or_copyin(a[0:2],p) \
                   copyout(p)
    ;

  //--------------------------------------------------
  // Nesting in other constructs
  //--------------------------------------------------

  // expected-note@+1 {{enclosing '#pragma acc parallel' here}}
  #pragma acc parallel
  // expected-error@+1 {{'#pragma acc data' cannot be nested within '#pragma acc parallel'}}
  #pragma acc data copy(i)
    ;

  // expected-note@+1 {{enclosing '#pragma acc parallel loop' here}}
  #pragma acc parallel loop
  for (int i = 0; i < 5; ++i)
    // expected-error@+1 {{'#pragma acc data' cannot be nested within '#pragma acc parallel loop'}}
    #pragma acc data copy(i)
      ;

  #pragma acc parallel
  // expected-note@+1 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop
  for (int i = 0; i < 5; ++i)
    // expected-error@+1 {{'#pragma acc data' cannot be nested within '#pragma acc loop'}}
    #pragma acc data copy(i)
      ;

  #pragma acc parallel
  // expected-note@+1 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i)
    // expected-error@+1 {{'#pragma acc data' cannot be nested within '#pragma acc loop'}}
    #pragma acc data copy(i)
      ;

  #pragma acc parallel loop
  for (int i = 0; i < 5; ++i)
  // expected-note@+1 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop
  for (int j = 0; j < 5; ++j)
    // expected-error@+1 {{'#pragma acc data' cannot be nested within '#pragma acc loop'}}
    #pragma acc data copy(i)
      ;

  return 0;
}

// Complete to suppress an additional warning, but it's too late for pragmas.
int incomplete[3];
