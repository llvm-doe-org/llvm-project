// Check diagnostics for "acc enter data" and "acc exit data".

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
  double d;
  // expected-note@+1 3 {{variable 'constI' declared const here}}
  const int constI = 5;
  // expected-note@+1 3 {{variable 'constIDecl' declared const here}}
  const extern int constIDecl;
  // expected-note@+1 3 {{variable 'constA' declared const here}}
  const int constA[3];
  // expected-note@+1 3 {{variable 'constADecl' declared const here}}
  const extern int constADecl[3];
  struct S { int i; };
  struct S *ps;
  struct SS { struct S s; } ss;

  //--------------------------------------------------
  // Directive name parsing errors
  //--------------------------------------------------

  // expected-error@+1 {{unknown or unsupported OpenACC directive}}
  #pragma acc enter
  ;
  // expected-error@+1 {{unknown or unsupported OpenACC directive}}
  #pragma acc exit
  ;
  // expected-error@+1 {{unknown or unsupported OpenACC directive}}
  #pragma acc enter copyin(i)
  ;
  // expected-error@+1 {{unknown or unsupported OpenACC directive}}
  #pragma acc exit copyout(i)
  ;
  // expected-warning@+2 {{extra tokens at the end of '#pragma acc data' are ignored}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
  #pragma acc data enter
  ;
  // expected-warning@+2 {{extra tokens at the end of '#pragma acc data' are ignored}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
  #pragma acc data exit
  ;
  // expected-warning@+1 {{extra tokens at the end of '#pragma acc parallel' are ignored}}
  #pragma acc parallel enter
  ;
  // expected-warning@+1 {{extra tokens at the end of '#pragma acc parallel' are ignored}}
  #pragma acc parallel exit
  ;
  // expected-warning@+2 {{extra tokens at the end of '#pragma acc loop' are ignored}}
  #pragma acc parallel
  #pragma acc loop enter
  for (int i = 0; i < 5; ++i)
    ;
  // expected-warning@+2 {{extra tokens at the end of '#pragma acc loop' are ignored}}
  #pragma acc parallel
  #pragma acc loop exit
  for (int i = 0; i < 5; ++i)
    ;

  //--------------------------------------------------
  // No clauses
  //--------------------------------------------------

  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data

  // Any data clause is sufficient to suppress that diagnostic.
  #pragma acc enter data copyin(i)
  #pragma acc enter data pcopyin(i)
  #pragma acc enter data present_or_copyin(i)
  #pragma acc enter data create(i)
  #pragma acc enter data pcreate(i)
  #pragma acc enter data present_or_create(i)
  #pragma acc exit data copyout(i)
  #pragma acc exit data pcopyout(i)
  #pragma acc exit data present_or_copyout(i)
  #pragma acc exit data delete(i)

  //--------------------------------------------------
  // Unrecognized clauses
  //--------------------------------------------------

  // Bogus clauses.

  // expected-warning@+2 {{extra tokens at the end of '#pragma acc enter data' are ignored}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data foo
  // expected-warning@+2 {{extra tokens at the end of '#pragma acc exit data' are ignored}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data foo bar

  // Well formed clauses not permitted here.

  // expected-error@+6 {{unexpected OpenACC clause 'nomap' in directive '#pragma acc enter data'}}
  // expected-error@+5 {{unexpected OpenACC clause 'present' in directive '#pragma acc enter data'}}
  // expected-error@+4 {{unexpected OpenACC clause 'copy' in directive '#pragma acc enter data'}}
  // expected-error@+3 {{unexpected OpenACC clause 'copyout' in directive '#pragma acc enter data'}}
  // expected-error@+2 {{unexpected OpenACC clause 'no_create' in directive '#pragma acc enter data'}}
  // expected-error@+1 {{unexpected OpenACC clause 'delete' in directive '#pragma acc enter data'}}
  #pragma acc enter data nomap(i) present(i) copy(i) copyin(i) copyout(i) create(jk) no_create(i) delete(i)
  // expected-error@+6 {{unexpected OpenACC clause 'nomap' in directive '#pragma acc exit data'}}
  // expected-error@+5 {{unexpected OpenACC clause 'present' in directive '#pragma acc exit data'}}
  // expected-error@+4 {{unexpected OpenACC clause 'copy' in directive '#pragma acc exit data'}}
  // expected-error@+3 {{unexpected OpenACC clause 'copyin' in directive '#pragma acc exit data'}}
  // expected-error@+2 {{unexpected OpenACC clause 'create' in directive '#pragma acc exit data'}}
  // expected-error@+1 {{unexpected OpenACC clause 'no_create' in directive '#pragma acc exit data'}}
  #pragma acc exit data nomap(i) present(i) copy(i) copyin(i) copyout(i) create(i) no_create(i) delete(jk)

  // expected-error@+5 {{unexpected OpenACC clause 'pcopy' in directive '#pragma acc enter data'}}
  // expected-error@+4 {{unexpected OpenACC clause 'pcopyout' in directive '#pragma acc enter data'}}
  // expected-error@+3 {{unexpected OpenACC clause 'present_or_copy' in directive '#pragma acc enter data'}}
  // expected-error@+2 {{unexpected OpenACC clause 'present_or_copyout' in directive '#pragma acc enter data'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data pcopy(i) pcopyout(i) present_or_copy(i) present_or_copyout(i)
  // expected-error@+7 {{unexpected OpenACC clause 'pcopy' in directive '#pragma acc exit data'}}
  // expected-error@+6 {{unexpected OpenACC clause 'pcopyin' in directive '#pragma acc exit data'}}
  // expected-error@+5 {{unexpected OpenACC clause 'pcreate' in directive '#pragma acc exit data'}}
  // expected-error@+4 {{unexpected OpenACC clause 'present_or_copy' in directive '#pragma acc exit data'}}
  // expected-error@+3 {{unexpected OpenACC clause 'present_or_copyin' in directive '#pragma acc exit data'}}
  // expected-error@+2 {{unexpected OpenACC clause 'present_or_create' in directive '#pragma acc exit data'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data pcopy(i) pcopyin(i) pcreate(i) present_or_copy(i) present_or_copyin(i) present_or_create(i)

  // expected-error@+5 {{unexpected OpenACC clause 'shared' in directive '#pragma acc enter data'}}
  // expected-error@+4 {{unexpected OpenACC clause 'reduction' in directive '#pragma acc enter data'}}
  // expected-error@+3 {{unexpected OpenACC clause 'private' in directive '#pragma acc enter data'}}
  // expected-error@+2 {{unexpected OpenACC clause 'firstprivate' in directive '#pragma acc enter data'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data shared(i, jk) reduction(+:i,jk,f) private(i) firstprivate(i)
  // expected-error@+5 {{unexpected OpenACC clause 'shared' in directive '#pragma acc exit data'}}
  // expected-error@+4 {{unexpected OpenACC clause 'reduction' in directive '#pragma acc exit data'}}
  // expected-error@+3 {{unexpected OpenACC clause 'private' in directive '#pragma acc exit data'}}
  // expected-error@+2 {{unexpected OpenACC clause 'firstprivate' in directive '#pragma acc exit data'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data shared(i, jk) reduction(+:i,jk,f) private(i) firstprivate(i)

  // expected-error@+6 {{unexpected OpenACC clause 'if' in directive '#pragma acc enter data'}}
  // expected-error@+5 {{unexpected OpenACC clause 'if_present' in directive '#pragma acc enter data'}}
  // expected-error@+4 {{unexpected OpenACC clause 'self' in directive '#pragma acc enter data'}}
  // expected-error@+3 {{unexpected OpenACC clause 'host' in directive '#pragma acc enter data'}}
  // expected-error@+2 {{unexpected OpenACC clause 'device' in directive '#pragma acc enter data'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data if(1) if_present self(i) host(i) device(i)
  // expected-error@+6 {{unexpected OpenACC clause 'if' in directive '#pragma acc exit data'}}
  // expected-error@+5 {{unexpected OpenACC clause 'if_present' in directive '#pragma acc exit data'}}
  // expected-error@+4 {{unexpected OpenACC clause 'self' in directive '#pragma acc exit data'}}
  // expected-error@+3 {{unexpected OpenACC clause 'host' in directive '#pragma acc exit data'}}
  // expected-error@+2 {{unexpected OpenACC clause 'device' in directive '#pragma acc exit data'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data if(1) if_present self(i) host(i) device(i)

  // expected-error@+4 {{unexpected OpenACC clause 'num_gangs' in directive '#pragma acc enter data'}}
  // expected-error@+3 {{unexpected OpenACC clause 'num_workers' in directive '#pragma acc enter data'}}
  // expected-error@+2 {{unexpected OpenACC clause 'vector_length' in directive '#pragma acc enter data'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data num_gangs(1) num_workers(2) vector_length(3)
  // expected-error@+4 {{unexpected OpenACC clause 'num_gangs' in directive '#pragma acc exit data'}}
  // expected-error@+3 {{unexpected OpenACC clause 'num_workers' in directive '#pragma acc exit data'}}
  // expected-error@+2 {{unexpected OpenACC clause 'vector_length' in directive '#pragma acc exit data'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data num_gangs(1) num_workers(2) vector_length(3)

  // expected-error@+9 {{unexpected OpenACC clause 'independent' in directive '#pragma acc enter data'}}
  // expected-error@+8 {{unexpected OpenACC clause 'auto' in directive '#pragma acc enter data'}}
  // expected-error@+7 {{unexpected OpenACC clause 'seq' in directive '#pragma acc enter data'}}
  // expected-error@+6 {{unexpected OpenACC clause 'gang' in directive '#pragma acc enter data'}}
  // expected-error@+5 {{unexpected OpenACC clause 'worker' in directive '#pragma acc enter data'}}
  // expected-error@+4 {{unexpected OpenACC clause 'vector' in directive '#pragma acc enter data'}}
  // expected-error@+3 {{unexpected OpenACC clause 'collapse' in directive '#pragma acc enter data'}}
  // expected-error@+2 {{unexpected OpenACC clause 'tile' in directive '#pragma acc enter data'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data seq independent auto gang worker vector collapse(1) tile(3)
  // expected-error@+9 {{unexpected OpenACC clause 'independent' in directive '#pragma acc exit data'}}
  // expected-error@+8 {{unexpected OpenACC clause 'auto' in directive '#pragma acc exit data'}}
  // expected-error@+7 {{unexpected OpenACC clause 'seq' in directive '#pragma acc exit data'}}
  // expected-error@+6 {{unexpected OpenACC clause 'gang' in directive '#pragma acc exit data'}}
  // expected-error@+5 {{unexpected OpenACC clause 'worker' in directive '#pragma acc exit data'}}
  // expected-error@+4 {{unexpected OpenACC clause 'vector' in directive '#pragma acc exit data'}}
  // expected-error@+3 {{unexpected OpenACC clause 'collapse' in directive '#pragma acc exit data'}}
  // expected-error@+2 {{unexpected OpenACC clause 'tile' in directive '#pragma acc exit data'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data seq independent auto gang worker vector collapse(1) tile(3)

  // expected-error@+2 {{unexpected OpenACC clause 'async' in directive '#pragma acc enter data'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data async
  // expected-error@+2 {{unexpected OpenACC clause 'async' in directive '#pragma acc exit data'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data async

  // expected-error@+5 {{unexpected OpenACC clause 'read' in directive '#pragma acc enter data'}}
  // expected-error@+4 {{unexpected OpenACC clause 'write' in directive '#pragma acc enter data'}}
  // expected-error@+3 {{unexpected OpenACC clause 'update' in directive '#pragma acc enter data'}}
  // expected-error@+2 {{unexpected OpenACC clause 'capture' in directive '#pragma acc enter data'}}
  // expected-error@+1 {{unexpected OpenACC clause 'compare' in directive '#pragma acc enter data'}}
  #pragma acc enter data read write update capture compare copyin(i)
  // expected-error@+5 {{unexpected OpenACC clause 'read' in directive '#pragma acc exit data'}}
  // expected-error@+4 {{unexpected OpenACC clause 'write' in directive '#pragma acc exit data'}}
  // expected-error@+3 {{unexpected OpenACC clause 'update' in directive '#pragma acc exit data'}}
  // expected-error@+2 {{unexpected OpenACC clause 'capture' in directive '#pragma acc exit data'}}
  // expected-error@+1 {{unexpected OpenACC clause 'compare' in directive '#pragma acc exit data'}}
  #pragma acc exit data copyout(i) read write update capture compare

  // Malformed clauses not permitted here.

  // expected-error@+2 {{unexpected OpenACC clause 'shared' in directive '#pragma acc enter data'}}
  // expected-error@+1 {{expected '(' after 'shared'}}
  #pragma acc enter data copyin(i) shared
  // expected-error@+4 {{unexpected OpenACC clause 'shared' in directive '#pragma acc enter data'}}
  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc enter data create(i) shared(
  // expected-error@+4 {{unexpected OpenACC clause 'shared' in directive '#pragma acc enter data'}}
  // expected-error@+3 {{expected ')'}}
  // expected-note@+2 {{to match this '('}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data shared(i

  // expected-error@+2 {{unexpected OpenACC clause 'create' in directive '#pragma acc exit data'}}
  // expected-error@+1 {{expected '(' after 'create'}}
  #pragma acc exit data copyout(i) create
  // expected-error@+4 {{unexpected OpenACC clause 'create' in directive '#pragma acc exit data'}}
  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc exit data delete(i) create(
  // expected-error@+4 {{unexpected OpenACC clause 'create' in directive '#pragma acc exit data'}}
  // expected-error@+3 {{expected ')'}}
  // expected-note@+2 {{to match this '('}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data create(i

  //--------------------------------------------------
  // Data clauses: syntax
  //
  // Parsing of data clauses is checked thoroughly in diagnostics/parallel.c, so
  // just check a cross section of cases here to confirm it works for the enter
  // data and exit data directives' data clauses.
  //--------------------------------------------------

  // expected-error@+2 {{expected '(' after 'copyin'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data copyin
  // expected-error@+4 {{expected expression}}
  // expected-error@+3 {{expected ')'}}
  // expected-note@+2 {{to match this '('}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data copyout(
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc enter data create(jk
  // expected-error@+2 {{expected expression}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data delete()
  // expected-error@+1 {{expected ',' or ')' in 'pcopyin' clause}}
  #pragma acc enter data pcopyin(i jk)
  // expected-error@+1 {{expected expression}}
  #pragma acc exit data pcopyout(jk ,)
  // expected-error@+2 {{expected variable name or data member expression or subarray}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data present_or_copyin((int)i)
  // expected-error@+2 {{expected variable name or data member expression or subarray}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data present_or_copyout((*(int(*)[3])a)[0:])
  // expected-error@+2 {{subscripted value is not an array or pointer}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data pcreate(i[c:c])
  // expected-error@+2 {{subscripted value is not an array or pointer}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data delete(i[0:2])
  // char-subscripts-warning@+1 {{subarray start is of type 'char'}}
  #pragma acc enter data present_or_create(a[c:2])
  // char-subscripts-warning@+1 {{subarray length is of type 'char'}}
  #pragma acc exit data copyout(a[0:c])
  // expected-error@+2 {{subarray specified for pointer to function type 'int ()'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data copyin(fp[0:2])
  // expected-error@+2 {{subarray specified for pointer to incomplete type 'int[]'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data delete((&incomplete)[0:2])

  //--------------------------------------------------
  // Data clauses: arg semantics
  //--------------------------------------------------

  // Unknown variable name.

  // expected-error@+2 {{use of undeclared identifier 'foo'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data copyin(foo)
  // expected-error@+2 {{use of undeclared identifier 'foo'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data pcopyin(foo)
  // expected-error@+2 {{use of undeclared identifier 'foo'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data present_or_copyin(foo)
  // expected-error@+2 {{use of undeclared identifier 'foo'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data create(foo)
  // expected-error@+2 {{use of undeclared identifier 'foo'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data pcreate(foo)
  // expected-error@+2 {{use of undeclared identifier 'foo'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data present_or_create(foo)

  // expected-error@+2 {{use of undeclared identifier 'foo'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data copyout(foo)
  // expected-error@+2 {{use of undeclared identifier 'foo'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data pcopyout(foo)
  // expected-error@+2 {{use of undeclared identifier 'foo'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data present_or_copyout(foo)
  // expected-error@+2 {{use of undeclared identifier 'foo'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data delete(foo)

  // Nested member expression not permitted.

  // expected-error-re@+7 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error-re@+7 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error-re@+7 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error-re@+7 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error-re@+7 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error-re@+7 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data copyin(ss.s.i)             \
                         pcopyin(ss.s.i)            \
                         present_or_copyin(ss.s.i)  \
                         create(ss.s.i)             \
                         pcreate(ss.s.i)            \
                         present_or_create(ss.s.i)

  // expected-error-re@+5 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error-re@+5 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error-re@+5 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error-re@+5 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data copyout(ss.s.i)            \
                        pcopyout(ss.s.i)           \
                        present_or_copyout(ss.s.i) \
                        delete(ss.s.i)

  // Array subscript not supported where subarray is permitted.

  // expected-error@+2 {{subarray syntax must include ':'}}
  // expected-error@+2 {{subarray syntax must include ':'}}
  #pragma acc enter data copyin(a[jk], \
                                m[2:][1], \
                                a[:], m[0:2][0:2])
  // expected-error@+2 {{subarray syntax must include ':'}}
  // expected-error@+2 {{subarray syntax must include ':'}}
  #pragma acc enter data pcopyin(a[jk], \
                                 m[2:][1], \
                                 a[:], m[0:2][0:2])
  // expected-error@+2 {{subarray syntax must include ':'}}
  // expected-error@+2 {{subarray syntax must include ':'}}
  #pragma acc enter data present_or_copyin(a[jk], \
                                           m[2:][1], \
                                           a[:], m[0:2][0:2])
  // expected-error@+2 {{subarray syntax must include ':'}}
  // expected-error@+2 {{subarray syntax must include ':'}}
  #pragma acc enter data create(a[jk], \
                                m[2:][1], \
                                a[:], m[0:2][0:2])
  // expected-error@+2 {{subarray syntax must include ':'}}
  // expected-error@+2 {{subarray syntax must include ':'}}
  #pragma acc enter data pcreate(a[jk], \
                                 m[2:][1], \
                                 a[:], m[0:2][0:2])
  // expected-error@+2 {{subarray syntax must include ':'}}
  // expected-error@+2 {{subarray syntax must include ':'}}
  #pragma acc enter data present_or_create(a[jk], \
                                           m[2:][1], \
                                           a[:], m[0:2][0:2])
  // expected-error@+2 {{subarray syntax must include ':'}}
  // expected-error@+2 {{subarray syntax must include ':'}}
  #pragma acc exit data copyout(a[jk], \
                                m[2:][1], \
                                a[:], m[0:2][0:2])
  // expected-error@+2 {{subarray syntax must include ':'}}
  // expected-error@+2 {{subarray syntax must include ':'}}
  #pragma acc exit data pcopyout(a[jk], \
                                 m[2:][1], \
                                 a[:], m[0:2][0:2])
  // expected-error@+2 {{subarray syntax must include ':'}}
  // expected-error@+2 {{subarray syntax must include ':'}}
  #pragma acc exit data present_or_copyout(a[jk], \
                                           m[2:][1], \
                                           a[:], m[0:2][0:2])
  // expected-error@+2 {{subarray syntax must include ':'}}
  // expected-error@+2 {{subarray syntax must include ':'}}
  #pragma acc exit data delete(a[jk], \
                               m[2:][1], \
                               a[:], m[0:2][0:2])

  // Member expression plus subarray not permitted.

  // expected-error@+7 {{OpenACC subarray is not allowed here}}
  // expected-error@+7 {{OpenACC subarray is not allowed here}}
  // expected-error@+7 {{OpenACC subarray is not allowed here}}
  // expected-error@+7 {{OpenACC subarray is not allowed here}}
  // expected-error@+7 {{OpenACC subarray is not allowed here}}
  // expected-error@+7 {{OpenACC subarray is not allowed here}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data copyin(ps[4:6].i)              \
                         pcopyin(ps[5:7].i)             \
                         present_or_copyin(ps[6:8].i)   \
                         create(ps[10:12].i)            \
                         pcreate(ps[11:13].i)           \
                         present_or_create(ps[12:14].i)

  // expected-error@+5 {{OpenACC subarray is not allowed here}}
  // expected-error@+5 {{OpenACC subarray is not allowed here}}
  // expected-error@+5 {{OpenACC subarray is not allowed here}}
  // expected-error@+5 {{OpenACC subarray is not allowed here}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data copyout(ps[4:6].i)             \
                        pcopyout(ps[5:7].i)            \
                        present_or_copyout(ps[6:8].i)  \
                        delete(ps[10:12].i)

  // Variables of incomplete type.

  // expected-error@+2 {{variable in 'copyin' clause cannot have incomplete type 'int[]'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data copyin(incomplete)
  // expected-error@+2 {{variable in 'pcopyin' clause cannot have incomplete type 'int[]'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data pcopyin(incomplete)
  // expected-error@+2 {{variable in 'present_or_copyin' clause cannot have incomplete type 'int[]'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data present_or_copyin(incomplete)
  // expected-error@+2 {{variable in 'create' clause cannot have incomplete type 'int[]'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data create(incomplete)
  // expected-error@+2 {{variable in 'pcreate' clause cannot have incomplete type 'int[]'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data pcreate(incomplete)
  // expected-error@+2 {{variable in 'present_or_create' clause cannot have incomplete type 'int[]'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data present_or_create(incomplete)

  // expected-error@+2 {{variable in 'copyout' clause cannot have incomplete type 'int[]'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data copyout(incomplete)
  // expected-error@+2 {{variable in 'pcopyout' clause cannot have incomplete type 'int[]'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data pcopyout(incomplete)
  // expected-error@+2 {{variable in 'present_or_copyout' clause cannot have incomplete type 'int[]'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data present_or_copyout(incomplete)
  // expected-error@+2 {{variable in 'delete' clause cannot have incomplete type 'int[]'}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data delete(incomplete)

  // Variables of const type.

  #pragma acc enter data copyin(constI, constIDecl)
  #pragma acc enter data pcopyin(constA, constADecl)
  #pragma acc enter data present_or_copyin(constI, constIDecl)
  // expected-error@+2 2 {{const variable cannot be initialized after 'create' clause}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data create(constA, constADecl)
  // expected-error@+2 2 {{const variable cannot be initialized after 'pcreate' clause}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data pcreate(constI, constIDecl)
  // expected-error@+2 2 {{const variable cannot be initialized after 'present_or_create' clause}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data present_or_create(constA, constADecl)

  // expected-error@+2 2 {{const variable cannot be written by 'copyout' clause}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data copyout(constI, constIDecl)
  // expected-error@+2 2 {{const variable cannot be written by 'pcopyout' clause}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data pcopyout(constA, constADecl)
  // expected-error@+2 2 {{const variable cannot be written by 'present_or_copyout' clause}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data present_or_copyout(constI, constIDecl)
  #pragma acc exit data delete(constI, constIDecl)

  // Redundant clauses.

  // expected-error@+6 {{copyin variable defined again as copyin variable}}
  // expected-note@+5 {{previously defined as copyin variable here}}
  // expected-error@+5 {{copyin variable defined again as copyin variable}}
  // expected-note@+3 {{previously defined as copyin variable here}}
  // expected-error@+4 {{copyin variable defined again as copyin variable}}
  // expected-note@+1 {{previously defined as copyin variable here}}
  #pragma acc enter data copyin(e,e,i,jk) \
                         copyin(i)        \
                         pcopyin(jk)
  // expected-error@+6 {{copyin variable defined again as copyin variable}}
  // expected-note@+5 {{previously defined as copyin variable here}}
  // expected-error@+5 {{copyin variable defined again as copyin variable}}
  // expected-note@+3 {{previously defined as copyin variable here}}
  // expected-error@+4 {{copyin variable defined again as copyin variable}}
  // expected-note@+1 {{previously defined as copyin variable here}}
  #pragma acc enter data pcopyin(e,e,i,jk) \
                         pcopyin(i)        \
                         present_or_copyin(jk)
  // expected-error@+6 {{copyin variable defined again as copyin variable}}
  // expected-note@+5 {{previously defined as copyin variable here}}
  // expected-error@+5 {{copyin variable defined again as copyin variable}}
  // expected-note@+3 {{previously defined as copyin variable here}}
  // expected-error@+4 {{copyin variable defined again as copyin variable}}
  // expected-note@+1 {{previously defined as copyin variable here}}
  #pragma acc enter data copyin(e,e,i,jk)      \
                         present_or_copyin(i)  \
                         present_or_copyin(jk)
  // expected-error@+6 {{copyin variable defined again as copyin variable}}
  // expected-note@+5 {{previously defined as copyin variable here}}
  // expected-error@+5 {{copyin variable defined again as copyin variable}}
  // expected-note@+3 {{previously defined as copyin variable here}}
  // expected-error@+4 {{copyin variable defined again as copyin variable}}
  // expected-note@+1 {{previously defined as copyin variable here}}
  #pragma acc enter data present_or_copyin(e,e,i,jk) \
                         pcopyin(i)                  \
                         copyin(jk)

  // expected-error@+6 {{create variable defined again as create variable}}
  // expected-note@+5 {{previously defined as create variable here}}
  // expected-error@+5 {{create variable defined again as create variable}}
  // expected-note@+3 {{previously defined as create variable here}}
  // expected-error@+4 {{create variable defined again as create variable}}
  // expected-note@+1 {{previously defined as create variable here}}
  #pragma acc enter data create(e,e,i,jk) \
                         create(i)        \
                         pcreate(jk)
  // expected-error@+6 {{create variable defined again as create variable}}
  // expected-note@+5 {{previously defined as create variable here}}
  // expected-error@+5 {{create variable defined again as create variable}}
  // expected-note@+3 {{previously defined as create variable here}}
  // expected-error@+4 {{create variable defined again as create variable}}
  // expected-note@+1 {{previously defined as create variable here}}
  #pragma acc enter data pcreate(e,e,i,jk) \
                         pcreate(i)        \
                         present_or_create(jk)
  // expected-error@+6 {{create variable defined again as create variable}}
  // expected-note@+5 {{previously defined as create variable here}}
  // expected-error@+5 {{create variable defined again as create variable}}
  // expected-note@+3 {{previously defined as create variable here}}
  // expected-error@+4 {{create variable defined again as create variable}}
  // expected-note@+1 {{previously defined as create variable here}}
  #pragma acc enter data create(e,e,i,jk)      \
                         present_or_create(i)  \
                         present_or_create(jk)
  // expected-error@+6 {{create variable defined again as create variable}}
  // expected-note@+5 {{previously defined as create variable here}}
  // expected-error@+5 {{create variable defined again as create variable}}
  // expected-note@+3 {{previously defined as create variable here}}
  // expected-error@+4 {{create variable defined again as create variable}}
  // expected-note@+1 {{previously defined as create variable here}}
  #pragma acc enter data present_or_create(e,e,i,jk) \
                         pcreate(i)                  \
                         create(jk)

  // expected-error@+6 {{copyout variable defined again as copyout variable}}
  // expected-note@+5 {{previously defined as copyout variable here}}
  // expected-error@+5 {{copyout variable defined again as copyout variable}}
  // expected-note@+3 {{previously defined as copyout variable here}}
  // expected-error@+4 {{copyout variable defined again as copyout variable}}
  // expected-note@+1 {{previously defined as copyout variable here}}
  #pragma acc exit data copyout(e,e,i,jk) \
                        copyout(i)        \
                        pcopyout(jk)
  // expected-error@+6 {{copyout variable defined again as copyout variable}}
  // expected-note@+5 {{previously defined as copyout variable here}}
  // expected-error@+5 {{copyout variable defined again as copyout variable}}
  // expected-note@+3 {{previously defined as copyout variable here}}
  // expected-error@+4 {{copyout variable defined again as copyout variable}}
  // expected-note@+1 {{previously defined as copyout variable here}}
  #pragma acc exit data pcopyout(e,e,i,jk) \
                        pcopyout(i)        \
                        present_or_copyout(jk)
  // expected-error@+6 {{copyout variable defined again as copyout variable}}
  // expected-note@+5 {{previously defined as copyout variable here}}
  // expected-error@+5 {{copyout variable defined again as copyout variable}}
  // expected-note@+3 {{previously defined as copyout variable here}}
  // expected-error@+4 {{copyout variable defined again as copyout variable}}
  // expected-note@+1 {{previously defined as copyout variable here}}
  #pragma acc exit data copyout(e,e,i,jk)      \
                        present_or_copyout(i)  \
                        present_or_copyout(jk)
  // expected-error@+6 {{copyout variable defined again as copyout variable}}
  // expected-note@+5 {{previously defined as copyout variable here}}
  // expected-error@+5 {{copyout variable defined again as copyout variable}}
  // expected-note@+3 {{previously defined as copyout variable here}}
  // expected-error@+4 {{copyout variable defined again as copyout variable}}
  // expected-note@+1 {{previously defined as copyout variable here}}
  #pragma acc exit data present_or_copyout(e,e,i,jk) \
                        pcopyout(i)                  \
                        copyout(jk)

  // expected-error@+4 {{delete variable defined again as delete variable}}
  // expected-note@+3 {{previously defined as delete variable here}}
  // expected-error@+3 {{delete variable defined again as delete variable}}
  // expected-note@+1 {{previously defined as delete variable here}}
  #pragma acc exit data delete(e,i,e,jk) \
                        delete(i)

  // expected-error@+2 {{copyin variable defined again as copyin variable}}
  // expected-note@+1 {{previously defined as copyin variable here}}
  #pragma acc enter data copyin(a[0:1], a[1:1])
  // expected-error@+2 {{copyout variable defined again as copyout variable}}
  // expected-note@+1 {{previously defined as copyout variable here}}
  #pragma acc exit data copyout(a[0:2]) copyout(a[1:1])
  // expected-error@+2 {{create variable defined again as create variable}}
  // expected-note@+1 {{previously defined as create variable here}}
  #pragma acc enter data create(a[0:1]) pcreate(a[0:1])
  // expected-error@+2 {{delete variable defined again as delete variable}}
  // expected-note@+1 {{previously defined as delete variable here}}
  #pragma acc exit data delete(a[0:1]) delete(a[0:2])

  // Conflicting clauses: different clause.

  // expected-error@+5 {{create variable cannot be copyin variable}}
  // expected-note@+3 {{previously defined as create variable here}}
  // expected-error@+4 {{copyin variable cannot be create variable}}
  // expected-note@+2 {{previously defined as copyin variable here}}
  #pragma acc enter data pcreate(a) \
                         present_or_copyin(a,p) \
                         create(p)
  // expected-error@+5 {{copyin variable cannot be create variable}}
  // expected-note@+3 {{previously defined as copyin variable here}}
  // expected-error@+4 {{create variable cannot be copyin variable}}
  // expected-note@+2 {{previously defined as create variable here}}
  #pragma acc enter data pcopyin(f) \
                         present_or_create(f,d) \
                         copyin(d)

  // expected-error@+5 {{delete variable cannot be copyout variable}}
  // expected-note@+3 {{previously defined as delete variable here}}
  // expected-error@+4 {{copyout variable cannot be delete variable}}
  // expected-note@+2 {{previously defined as copyout variable here}}
  #pragma acc exit data delete(a) \
                        present_or_copyout(a,p) \
                        delete(p)
  // expected-error@+5 {{copyout variable cannot be delete variable}}
  // expected-note@+3 {{previously defined as copyout variable here}}
  // expected-error@+4 {{delete variable cannot be copyout variable}}
  // expected-note@+2 {{previously defined as delete variable here}}
  #pragma acc exit data pcopyout(f) \
                        delete(f,d) \
                        copyout(d)

  //--------------------------------------------------
  // As immediate substatement
  //--------------------------------------------------

  // Only check the parse errors here when OpenACC directives are included
  // because there are different parse errors when they're ignored.
#ifdef ACC
  if (i)
    // expected-error@+1 {{'#pragma acc enter data' cannot be an immediate substatement}}
    #pragma acc enter data copyin(i)
  if (i)
    // expected-error@+1 {{'#pragma acc exit data' cannot be an immediate substatement}}
    #pragma acc exit data copyout(i)

  if (i)
    ;
  else
    // expected-error@+1 {{'#pragma acc enter data' cannot be an immediate substatement}}
    #pragma acc enter data copyin(i)
  if (i)
    ;
  else
    // expected-error@+1 {{'#pragma acc exit data' cannot be an immediate substatement}}
    #pragma acc exit data copyout(i)

  while (i)
    // expected-error@+1 {{'#pragma acc enter data' cannot be an immediate substatement}}
    #pragma acc enter data copyin(i)
  while (i)
    // expected-error@+1 {{'#pragma acc exit data' cannot be an immediate substatement}}
    #pragma acc exit data copyout(i)

  do
    // expected-error@+1 {{'#pragma acc enter data' cannot be an immediate substatement}}
    #pragma acc enter data copyin(i)
  while (i);
  do
    // expected-error@+1 {{'#pragma acc exit data' cannot be an immediate substatement}}
    #pragma acc exit data copyout(i)
  while (i);

  for (;;)
    // expected-error@+1 {{'#pragma acc enter data' cannot be an immediate substatement}}
    #pragma acc enter data copyin(i)
  for (;;)
    // expected-error@+1 {{'#pragma acc exit data' cannot be an immediate substatement}}
    #pragma acc exit data copyout(i)

  switch (i)
    // expected-error@+1 {{'#pragma acc enter data' cannot be an immediate substatement}}
    #pragma acc enter data copyin(i)
  switch (i)
    // expected-error@+1 {{'#pragma acc exit data' cannot be an immediate substatement}}
    #pragma acc exit data copyout(i)

  // Both the OpenACC 3.0 and OpenMP TR8 say the following cases (labeled
  // statements) are not permitted, but Clang permits them for both OpenACC and
  // OpenMP.
  switch (i) {
  case 0:
    #pragma acc enter data copyin(i)
  }
  switch (i) {
  case 0:
    #pragma acc exit data copyout(i)
  }
  switch (i) {
  default:
    #pragma acc enter data copyin(i)
  }
  switch (i) {
  default:
    #pragma acc exit data copyout(i)
  }
#endif

  lab1:
    #pragma acc enter data copyin(i)
  lab2:
    #pragma acc exit data copyout(i)

  #pragma acc data copy(i)
  // expected-error@+1 {{'#pragma acc enter data' cannot be an immediate substatement}}
  #pragma acc enter data copyin(i)

  #pragma acc data copy(i)
  // expected-error@+1 {{'#pragma acc exit data' cannot be an immediate substatement}}
  #pragma acc exit data copyout(i)

  // expected-note@+1 {{enclosing '#pragma acc parallel' here}}
  #pragma acc parallel
  // expected-error@+2 {{'#pragma acc enter data' cannot be an immediate substatement}}
  // expected-error@+1 {{'#pragma acc enter data' cannot be nested within '#pragma acc parallel'}}
  #pragma acc enter data copyin(i)

  // expected-note@+1 {{enclosing '#pragma acc parallel' here}}
  #pragma acc parallel
  // expected-error@+2 {{'#pragma acc exit data' cannot be an immediate substatement}}
  // expected-error@+1 {{'#pragma acc exit data' cannot be nested within '#pragma acc parallel'}}
  #pragma acc exit data copyout(i)

  //--------------------------------------------------
  // Nesting in other constructs (not as immediate substatement)
  //--------------------------------------------------

  #pragma acc data copy(i)
  {
    #pragma acc enter data copyin(i)
    #pragma acc exit data copyout(i)
  }

  // expected-note@+1 {{enclosing '#pragma acc parallel' here}}
  #pragma acc parallel
  {
    // expected-error@+1 {{'#pragma acc enter data' cannot be nested within '#pragma acc parallel'}}
    #pragma acc enter data copyin(i)
  }

  // expected-note@+1 {{enclosing '#pragma acc parallel' here}}
  #pragma acc parallel
  {
    // expected-error@+1 {{'#pragma acc exit data' cannot be nested within '#pragma acc parallel'}}
    #pragma acc exit data copyout(i)
  }

  // expected-note@+1 {{enclosing '#pragma acc parallel loop' here}}
  #pragma acc parallel loop
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc enter data' cannot be nested within '#pragma acc parallel loop'}}
    #pragma acc enter data copyin(i)
  }

  // expected-note@+1 {{enclosing '#pragma acc parallel loop' here}}
  #pragma acc parallel loop
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc exit data' cannot be nested within '#pragma acc parallel loop'}}
    #pragma acc exit data copyout(i)
  }

  #pragma acc parallel
  // expected-note@+1 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc enter data' cannot be nested within '#pragma acc loop'}}
    #pragma acc enter data copyin(i)
  }

  #pragma acc parallel
  // expected-note@+1 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc exit data' cannot be nested within '#pragma acc loop'}}
    #pragma acc exit data copyout(i)
  }

  #pragma acc parallel
  // expected-note@+1 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc enter data' cannot be nested within '#pragma acc loop'}}
    #pragma acc enter data copyin(i)
  }

  #pragma acc parallel
  // expected-note@+1 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop seq
  for (int i = 0; i < 5; ++i) {
    // expected-error@+1 {{'#pragma acc exit data' cannot be nested within '#pragma acc loop'}}
    #pragma acc exit data copyout(i)
  }

  #pragma acc parallel loop
  for (int i = 0; i < 5; ++i)
  // expected-note@+1 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop
  for (int j = 0; j < 5; ++j) {
    // expected-error@+1 {{'#pragma acc enter data' cannot be nested within '#pragma acc loop'}}
    #pragma acc enter data copyin(i)
  }

  #pragma acc parallel loop
  for (int i = 0; i < 5; ++i)
  // expected-note@+1 {{enclosing '#pragma acc loop' here}}
  #pragma acc loop
  for (int j = 0; j < 5; ++j) {
    // expected-error@+1 {{'#pragma acc exit data' cannot be nested within '#pragma acc loop'}}
    #pragma acc exit data copyout(i)
  }

  return 0;
}

// Complete to suppress an additional warning, but it's too late for pragmas.
int incomplete[3];
