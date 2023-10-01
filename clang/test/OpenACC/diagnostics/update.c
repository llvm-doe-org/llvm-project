// Check diagnostics for "acc update".
//
// -Wopenacc-omp-update-present is tested in warn-acc-omp-update-present.c

// OpenACC disabled
// RUN: %clang_cc1 -verify=noacc,expected-noacc -Wchar-subscripts %s

// OpenACC enabled
// RUN: %clang_cc1 -DACC -verify=expected,expected-noacc,char-subscripts \
// RUN:            -fopenacc -Wchar-subscripts %s

// OpenACC enabled but optional warnings disabled
// RUN: %clang_cc1 -DACC -verify=expected,expected-noacc -fopenacc %s

// END.

// noacc-no-diagnostics

int incomplete[];

int main() {
  char c;
  enum { E1, E2 } e;
  int i, jk, a[2], m[6][2], *p;
  int (*fp)();
  float f;
  // expected-note@+1 2 {{variable 'constI' declared const here}}
  const int constI = 5;
  // expected-note@+1 {{variable 'constIDecl' declared const here}}
  const extern int constIDecl;
  // expected-note@+1 {{variable 'constA' declared const here}}
  const int constA[3];
  // expected-note@+1 2 {{variable 'constADecl' declared const here}}
  const extern int constADecl[3];
  struct S { int i; } s;
  union U { int i; } u;
  struct S *ps;
  struct SS { struct S s; } ss;

  //--------------------------------------------------
  // Missing clauses
  //--------------------------------------------------

  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update if(0)
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update if(1)

  // Any one of those clauses is sufficient to suppress that diagnostic.
  #pragma acc update self(i)
  #pragma acc update host(i)
  #pragma acc update device(i)

  //--------------------------------------------------
  // Unrecognized clauses
  //--------------------------------------------------

  // Bogus clauses.

  // expected-warning@+2 {{extra tokens at the end of '#pragma acc update' are ignored}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update foo
  // expected-warning@+2 {{extra tokens at the end of '#pragma acc update' are ignored}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update foo bar

  // Well formed clauses not permitted here.

  // expected-error@+9 {{unexpected OpenACC clause 'nomap' in directive '#pragma acc update'}}
  // expected-error@+8 {{unexpected OpenACC clause 'present' in directive '#pragma acc update'}}
  // expected-error@+7 {{unexpected OpenACC clause 'copy' in directive '#pragma acc update'}}
  // expected-error@+6 {{unexpected OpenACC clause 'copyin' in directive '#pragma acc update'}}
  // expected-error@+5 {{unexpected OpenACC clause 'copyout' in directive '#pragma acc update'}}
  // expected-error@+4 {{unexpected OpenACC clause 'create' in directive '#pragma acc update'}}
  // expected-error@+3 {{unexpected OpenACC clause 'no_create' in directive '#pragma acc update'}}
  // expected-error@+2 {{unexpected OpenACC clause 'delete' in directive '#pragma acc update'}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update nomap(i) present(i) copy(i) copyin(i) copyout(i) create(i) no_create(i) delete(i)
  // expected-error@+5 {{unexpected OpenACC clause 'pcopy' in directive '#pragma acc update'}}
  // expected-error@+4 {{unexpected OpenACC clause 'pcopyin' in directive '#pragma acc update'}}
  // expected-error@+3 {{unexpected OpenACC clause 'pcopyout' in directive '#pragma acc update'}}
  // expected-error@+2 {{unexpected OpenACC clause 'pcreate' in directive '#pragma acc update'}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update pcopy(i) pcopyin(i) pcopyout(i) pcreate(i)
  // expected-error@+5 {{unexpected OpenACC clause 'present_or_copy' in directive '#pragma acc update'}}
  // expected-error@+4 {{unexpected OpenACC clause 'present_or_copyin' in directive '#pragma acc update'}}
  // expected-error@+3 {{unexpected OpenACC clause 'present_or_copyout' in directive '#pragma acc update'}}
  // expected-error@+2 {{unexpected OpenACC clause 'present_or_create' in directive '#pragma acc update'}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update present_or_copy(i) present_or_copyin(i) present_or_copyout(i) present_or_create(i)
  // expected-error@+5 {{unexpected OpenACC clause 'shared' in directive '#pragma acc update'}}
  // expected-error@+4 {{unexpected OpenACC clause 'reduction' in directive '#pragma acc update'}}
  // expected-error@+3 {{unexpected OpenACC clause 'private' in directive '#pragma acc update'}}
  // expected-error@+2 {{unexpected OpenACC clause 'firstprivate' in directive '#pragma acc update'}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update shared(i, jk) reduction(+:i,jk,f) private(i) firstprivate(i)
  // expected-error@+3 {{unexpected OpenACC clause 'num_gangs' in directive '#pragma acc update'}}
  // expected-error@+2 {{unexpected OpenACC clause 'num_workers' in directive '#pragma acc update'}}
  // expected-error@+1 {{unexpected OpenACC clause 'vector_length' in directive '#pragma acc update'}}
  #pragma acc update self(i) num_gangs(1) num_workers(2) vector_length(3)
  // expected-error@+6 {{unexpected OpenACC clause 'independent' in directive '#pragma acc update'}}
  // expected-error@+5 {{unexpected OpenACC clause 'auto' in directive '#pragma acc update'}}
  // expected-error@+4 {{unexpected OpenACC clause 'seq' in directive '#pragma acc update'}}
  // expected-error@+3 {{unexpected OpenACC clause 'gang' in directive '#pragma acc update'}}
  // expected-error@+2 {{unexpected OpenACC clause 'worker' in directive '#pragma acc update'}}
  // expected-error@+1 {{unexpected OpenACC clause 'vector' in directive '#pragma acc update'}}
  #pragma acc update independent host(i) auto seq gang worker vector
  // expected-error@+2 {{unexpected OpenACC clause 'collapse' in directive '#pragma acc update'}}
  // expected-error@+1 {{unexpected OpenACC clause 'tile' in directive '#pragma acc update'}}
  #pragma acc update collapse(1) device(i) tile(6)
  // expected-error@+1 {{unexpected OpenACC clause 'async' in directive '#pragma acc update'}}
  #pragma acc update device(i) async
  // expected-error@+5 {{unexpected OpenACC clause 'read' in directive '#pragma acc update'}}
  // expected-error@+4 {{unexpected OpenACC clause 'write' in directive '#pragma acc update'}}
  // expected-error@+3 {{unexpected OpenACC clause 'update' in directive '#pragma acc update'}}
  // expected-error@+2 {{unexpected OpenACC clause 'capture' in directive '#pragma acc update'}}
  // expected-error@+1 {{unexpected OpenACC clause 'compare' in directive '#pragma acc update'}}
  #pragma acc update read write self(i) update capture compare

  // Malformed clauses not permitted here.

  // expected-error@+2 {{unexpected OpenACC clause 'shared' in directive '#pragma acc update'}}
  // expected-error@+1 {{expected '(' after 'shared'}}
  #pragma acc update self(i) shared
  // expected-error@+4 {{unexpected OpenACC clause 'shared' in directive '#pragma acc update'}}
  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc update device(i) shared(
  // expected-error@+4 {{unexpected OpenACC clause 'shared' in directive '#pragma acc update'}}
  // expected-error@+3 {{expected ')'}}
  // expected-note@+2 {{to match this '('}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update shared(i

  //--------------------------------------------------
  // if clause.
  //--------------------------------------------------

  // expected-error@+1 {{expected '(' after 'if'}}
  #pragma acc update self(i) if
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc update self(i) if(1
  // expected-error@+1 {{use of undeclared identifier 'bogus'}}
  #pragma acc update self(i) if(bogus)
  // expected-error@+1 {{statement requires expression of scalar type ('struct S' invalid)}}
  #pragma acc update self(i) if(s)
  // expected-error@+1 {{statement requires expression of scalar type ('union U' invalid)}}
  #pragma acc update self(i) if(u)
  // expected-warning@+1 {{implicit conversion from 'double' to '_Bool' changes value from 0.1 to true}}
  #pragma acc update self(i) if(0.1)
  // expected-error@+1 {{directive '#pragma acc update' cannot contain more than one 'if' clause}}
  #pragma acc update self(i) if(1) if(1)
  // expected-error@+2 {{directive '#pragma acc update' cannot contain more than one 'if' clause}}
  // expected-error@+1 {{directive '#pragma acc update' cannot contain more than one 'if' clause}}
  #pragma acc update self(i) if(i) if( 1) if(0)

  //--------------------------------------------------
  // if_present clause.
  //--------------------------------------------------

  // expected-warning@+1 {{extra tokens at the end of '#pragma acc update' are ignored}}
  #pragma acc update self(i) if_present(
  // expected-warning@+1 {{extra tokens at the end of '#pragma acc update' are ignored}}
  #pragma acc update self(i) if_present()
  // expected-warning@+1 {{extra tokens at the end of '#pragma acc update' are ignored}}
  #pragma acc update self(i) if_present(i
  // expected-warning@+1 {{extra tokens at the end of '#pragma acc update' are ignored}}
  #pragma acc update self(i) if_present(i)

  //--------------------------------------------------
  // Variable clauses: syntax
  //
  // Parsing of variable clauses is checked thoroughly in diagnostics/parallel.c
  // for data clauses, so just check a cross section of cases here to confirm it
  // works for the update directive's variable clauses.
  //--------------------------------------------------

  // expected-error@+2 {{expected '(' after 'self'}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update self
  // expected-error@+4 {{expected expression}}
  // expected-error@+3 {{expected ')'}}
  // expected-note@+2 {{to match this '('}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update host(
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc update device(jk
  // expected-error@+2 {{expected expression}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update self()
  // expected-error@+1 {{expected ',' or ')' in 'host' clause}}
  #pragma acc update host(i jk)
  // expected-error@+1 {{expected expression}}
  #pragma acc update device(jk ,)
  // expected-error@+2 {{expected variable name or data member expression or subarray}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update self((int)i)
  // expected-error@+2 {{expected variable name or data member expression or subarray}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update host((*(int(*)[3])a)[0:])
  // expected-error@+2 {{subscripted value is not an array or pointer}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update device(i[c:c])
  // expected-error@+2 {{subscripted value is not an array or pointer}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update self(i[0:2])
  // char-subscripts-warning@+1 {{subarray start is of type 'char'}}
  #pragma acc update host(a[c:2])
  // char-subscripts-warning@+1 {{subarray length is of type 'char'}}
  #pragma acc update device(a[0:c])
  // expected-error@+2 {{subarray specified for pointer to function type 'int ()'}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update self(fp[0:2])
  // expected-error@+2 {{subarray specified for pointer to incomplete type 'int[]'}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update host((&incomplete)[0:2])

  //--------------------------------------------------
  // Variable clauses: arg semantics
  //--------------------------------------------------

  // Unknown variable name.

  // expected-error@+2 {{use of undeclared identifier 'foo'}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update self(foo)
  // expected-error@+2 {{use of undeclared identifier 'foo'}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update host(foo)
  // expected-error@+2 {{use of undeclared identifier 'foo'}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update device(foo)

  // Nested member expression not permitted.

  // expected-error-re@+2 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update self(ss.s.i)
  // expected-error-re@+2 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update host(ss.s.i)
  // expected-error-re@+2 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update device(ss.s.i)

  // Array subscript not supported where subarray is permitted.

  // expected-error@+2 {{subarray syntax must include ':'}}
  // expected-error@+2 {{subarray syntax must include ':'}}
  #pragma acc update self(a[jk], \
                          m[2:][1], \
                          a[:], m[0:2][0:2])
  // expected-error@+2 {{subarray syntax must include ':'}}
  // expected-error@+2 {{subarray syntax must include ':'}}
  #pragma acc update host(a[jk], \
                          m[2:][1], \
                          a[:], m[0:2][0:2])
  // expected-error@+2 {{subarray syntax must include ':'}}
  // expected-error@+2 {{subarray syntax must include ':'}}
  #pragma acc update device(a[jk], \
                            m[2:][1], \
                            a[:], m[0:2][0:2])

  // Member expression plus subarray not permitted.

  // expected-error@+2 {{OpenACC subarray is not allowed here}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update self(ps[4:6].i)

  // expected-error@+2 {{OpenACC subarray is not allowed here}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update host(ps[4:6].i)

  // expected-error@+2 {{OpenACC subarray is not allowed here}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update device(ps[4:6].i)

  // Variables of incomplete type.

  // expected-error@+2 {{variable in 'self' clause cannot have incomplete type 'int[]'}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update self(incomplete)
  // expected-error@+2 {{variable in 'host' clause cannot have incomplete type 'int[]'}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update host(incomplete)
  // expected-error@+2 {{variable in 'device' clause cannot have incomplete type 'int[]'}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update device(incomplete)

  // Variables of const type.

  // expected-error@+2 2 {{const variable cannot be written by 'self' clause}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update self(constI, constIDecl)
  // expected-error@+2 2 {{const variable cannot be written by 'host' clause}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update host(constADecl) host(constA)
  // expected-error@+2 2 {{const variable cannot be written by 'device' clause}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update device(constI) device(constADecl)

  // Redundant clauses.

  // expected-error@+6 {{variable 'e' appears in multiple self, host, or device clauses of the same '#pragma acc update' directive}}
  // expected-note@+5 {{previously appeared here}}
  // expected-error@+5 {{variable 'i' appears in multiple self, host, or device clauses of the same '#pragma acc update' directive}}
  // expected-note@+3 {{previously appeared here}}
  // expected-error@+4 {{variable 'jk' appears in multiple self, host, or device clauses of the same '#pragma acc update' directive}}
  // expected-note@+1 {{previously appeared here}}
  #pragma acc update self(e,e,i,jk) \
                     self(i)        \
                     host(jk)
  // expected-error@+4 {{variable 'e' appears in multiple self, host, or device clauses of the same '#pragma acc update' directive}}
  // expected-note@+3 {{previously appeared here}}
  // expected-error@+3 {{variable 'i' appears in multiple self, host, or device clauses of the same '#pragma acc update' directive}}
  // expected-note@+1 {{previously appeared here}}
  #pragma acc update device(e,i,e,jk) \
                     device(i)

  // Conflicting clauses: different clause.

  // expected-error@+7 {{variable 'i' appears in multiple self, host, or device clauses of the same '#pragma acc update' directive}}
  // expected-note@+5 {{previously appeared here}}
  // expected-error@+5 {{variable 'a' appears in multiple self, host, or device clauses of the same '#pragma acc update' directive}}
  // expected-note@+3 {{previously appeared here}}
  // expected-error@+4 {{variable 'jk' appears in multiple self, host, or device clauses of the same '#pragma acc update' directive}}
  // expected-note@+2 {{previously appeared here}}
  #pragma acc update device(i,a[0:1])  \
                     self(i,jk,a[0:1]) \
                     device(jk)
  // expected-error@+5 {{variable 'a' appears in multiple self, host, or device clauses of the same '#pragma acc update' directive}}
  // expected-note@+3 {{previously appeared here}}
  // expected-error@+4 {{variable 'p' appears in multiple self, host, or device clauses of the same '#pragma acc update' directive}}
  // expected-note@+2 {{previously appeared here}}
  #pragma acc update self(a)          \
                     device(a[0:2],p) \
                     host(p)

  //--------------------------------------------------
  // Variable clauses: arg semantics not yet supported
  //--------------------------------------------------

  // Conflicting clauses: different subarray.

  // expected-error@+3 {{variable 'a' appears in multiple self, host, or device clauses of the same '#pragma acc update' directive}}
  // expected-note@+1 {{previously appeared here}}
  #pragma acc update self(a[0:2])  \
                     host(a[0:1])

  // expected-error@+3 {{variable 'a' appears in multiple self, host, or device clauses of the same '#pragma acc update' directive}}
  // expected-note@+1 {{previously appeared here}}
  #pragma acc update self(a[0:2])  \
                     device(a[0:1])

  //--------------------------------------------------
  // As immediate substatement
  //--------------------------------------------------

  // Only check the parse errors here when OpenACC directives are included
  // because there are different parse errors when they're ignored.
#ifdef ACC
  if (i)
    // expected-error@+1 {{'#pragma acc update' cannot be an immediate substatement}}
    #pragma acc update self(i)

  if (i)
    ;
  else
    // expected-error@+1 {{'#pragma acc update' cannot be an immediate substatement}}
    #pragma acc update self(i)

  while (i)
    // expected-error@+1 {{'#pragma acc update' cannot be an immediate substatement}}
    #pragma acc update self(i)

  do
    // expected-error@+1 {{'#pragma acc update' cannot be an immediate substatement}}
    #pragma acc update self(i)
  while (i);

  for (;;)
    // expected-error@+1 {{'#pragma acc update' cannot be an immediate substatement}}
    #pragma acc update self(i)

  switch (i)
    // expected-error@+1 {{'#pragma acc update' cannot be an immediate substatement}}
    #pragma acc update self(i)

  // Both the OpenACC 3.0 and OpenMP TR8 say the following cases (labeled
  // statements) are not permitted, but Clang permits them for both OpenACC and
  // OpenMP.
  switch (i) {
  case 0:
    #pragma acc update self(i)
  }
  switch (i) {
  default:
    #pragma acc update self(i)
  }
#endif

  lab:
    #pragma acc update self(i)

  #pragma acc data copy(i)
  // expected-error@+1 {{'#pragma acc update' cannot be an immediate substatement}}
  #pragma acc update self(i)

  // expected-note@+1 {{enclosing '#pragma acc parallel' here}}
  #pragma acc parallel
  // expected-error@+2 {{'#pragma acc update' cannot be an immediate substatement}}
  // expected-error@+1 {{'#pragma acc update' cannot be nested within '#pragma acc parallel'}}
  #pragma acc update self(i)

  //--------------------------------------------------
  // Nesting in other constructs (not as immediate substatement)
  //--------------------------------------------------

  #pragma acc data copy(i)
  {
    #pragma acc update self(i)
  }

  // expected-note@+1 {{enclosing '#pragma acc parallel' here}}
  #pragma acc parallel
  {
    // expected-error@+1 {{'#pragma acc update' cannot be nested within '#pragma acc parallel'}}
    #pragma acc update self(i)
  }

  // expected-note@+1 {{enclosing '#pragma acc parallel loop' here}}
  #pragma acc parallel loop
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc update' cannot be nested within '#pragma acc parallel loop'}}
    #pragma acc update self(i)
  }

  #pragma acc parallel
  // expected-note@+1 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc update' cannot be nested within '#pragma acc loop'}}
    #pragma acc update self(i)
  }

  #pragma acc parallel
  // expected-note@+1 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc update' cannot be nested within '#pragma acc loop'}}
    #pragma acc update self(i)
  }

  #pragma acc parallel loop
  for (int i = 0; i < 5; ++i)
  // expected-note@+1 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop
  for (int j = 0; j < 5; ++j) {
    // expected-error@+1 {{'#pragma acc update' cannot be nested within '#pragma acc loop'}}
    #pragma acc update self(i)
  }

  return 0;
}

// Complete to suppress an additional warning, but it's too late for pragmas.
int incomplete[3];
