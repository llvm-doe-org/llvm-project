// Check diagnostics for "acc parallel".
//
// When ADD_LOOP_TO_PAR is not set, this file checks diagnostics for "acc
// parallel" without "loop".
//
// When ADD_LOOP_TO_PAR is set, it adds "loop" and a for loop to those "acc
// parallel" directives in order to check the same diagnostics but for combined
// "acc parallel loop" directives.

// RUN: %data {
// RUN:   (cflags=-DERR=ERR_ACC)
// RUN:   (cflags=-DERR=ERR_OMP_ARRAY_SECTION_NON_CONTIGUOUS_AP)
// RUN:   (cflags=-DERR=ERR_OMP_ARRAY_SECTION_NON_CONTIGUOUS_PP)
// RUN: }

// OpenACC disabled
// RUN: %for {
// RUN:   %clang_cc1 -verify=noacc,expected-noacc -Wchar-subscripts %[cflags] \
// RUN:              %s
// RUN:   %clang_cc1 -verify=noacc,expected-noacc -Wchar-subscripts %[cflags] \
// RUN:              -DADD_LOOP_TO_PAR %s
// RUN: }

// OpenACC enabled
// RUN: %for {
// RUN:   %clang_cc1 \
// RUN:     -verify=par,acc-ignore,expected,expected-noacc,char-subscripts \
// RUN:     -fopenacc -Wchar-subscripts %[cflags] %s
// RUN:   %clang_cc1 \
// RUN:     -verify=parloop,acc-ignore,expected,expected-noacc,char-subscripts \
// RUN:     -fopenacc -Wchar-subscripts %[cflags] -DADD_LOOP_TO_PAR %s
// RUN: }

// OpenACC enabled but optional warnings disabled
// RUN: %for {
// RUN:   %clang_cc1 -verify=par,expected,expected-noacc -fopenacc \
// RUN:              -Wno-openacc-ignored-clause %[cflags] %s
// RUN:   %clang_cc1 -verify=parloop,expected,expected-noacc -fopenacc \
// RUN:              -Wno-openacc-ignored-clause %[cflags] -DADD_LOOP_TO_PAR %s
// RUN: }

// END.

#define ERR_ACC                                 1
#define ERR_OMP_ARRAY_SECTION_NON_CONTIGUOUS_AP 2
#define ERR_OMP_ARRAY_SECTION_NON_CONTIGUOUS_PP 3

#if !ADD_LOOP_TO_PAR
# define LOOP
# define FORLOOP ;
# define FORLOOP_HEAD
#else
# define LOOP loop
# define FORLOOP for (int i = 0; i < 2; ++i) ;
# define FORLOOP_HEAD for (int fli = 0; fli < 2; ++fli)
#endif

#if ERR == ERR_ACC

int incomplete[];

int main() {
  _Bool b;
  char c;
  enum { E1, E2 } e;
  int i, jk, a[2], m[6][2], *p; // expected-note 9 {{'a' defined here}}
                                // expected-note@-1 7 {{'p' defined here}}
  int (*fp)();
  int (*ap)[];
  float f; // expected-note 3 {{'f' defined here}}
  double d; // expected-note 3 {{'d' defined here}}
  float _Complex fc; // expected-note 5 {{'fc' defined here}}
  double _Complex dc; // expected-note 5 {{'dc' defined here}}
  // expected-note@+3 {{variable 'constI' declared const here}}
  // expected-noacc-note@+2 {{variable 'constI' declared const here}}
  // expected-note@+1 {{'constI' defined here}}
  const int constI = 5;
  // expected-note@+3 {{variable 'constIDecl' declared const here}}
  // expected-noacc-note@+2 {{variable 'constIDecl' declared const here}}
  // expected-note@+1 {{'constIDecl' declared here}}
  const extern int constIDecl;
  // expected-noacc-note@+1 6 {{variable 'constA' declared const here}}
  const int constA[3];
  // expected-noacc-note@+1 6 {{variable 'constADecl' declared const here}}
  const extern int constADecl[3];
  struct S { int i; } s; // expected-note 9 {{'s' defined here}}
  union U { int i; } u; // expected-note 9 {{'u' defined here}}
  extern union U uDecl; // expected-note 9 {{'uDecl' declared here}}

  //--------------------------------------------------
  // Basic clause syntax
  //--------------------------------------------------

  #pragma acc parallel LOOP
    FORLOOP

  // par-warning@+2 {{extra tokens at the end of '#pragma acc parallel' are ignored}}
  // parloop-warning@+1 {{extra tokens at the end of '#pragma acc parallel loop' are ignored}}
  #pragma acc parallel LOOP foo
    FORLOOP
  // par-warning@+2 {{extra tokens at the end of '#pragma acc parallel' are ignored}}
  // parloop-warning@+1 {{extra tokens at the end of '#pragma acc parallel loop' are ignored}}
  #pragma acc parallel LOOP foo bar
    FORLOOP
  // par-warning@+2 {{extra tokens at the end of '#pragma acc parallel' are ignored}}
  // parloop-warning@+1 {{extra tokens at the end of '#pragma acc parallel loop' are ignored}}
  #pragma acc parallel LOOP foo bar
    FORLOOP
  // par-error@+1 {{unexpected OpenACC clause 'seq' in directive '#pragma acc parallel'}}
  #pragma acc parallel LOOP seq
    FORLOOP
  // par-error@+1 {{unexpected OpenACC clause 'independent' in directive '#pragma acc parallel'}}
  #pragma acc parallel LOOP independent
    FORLOOP
  // par-error@+3 {{unexpected OpenACC clause 'gang' in directive '#pragma acc parallel'}}
  // par-error@+2 {{unexpected OpenACC clause 'worker' in directive '#pragma acc parallel'}}
  // par-error@+1 {{unexpected OpenACC clause 'vector' in directive '#pragma acc parallel'}}
  #pragma acc parallel LOOP gang worker vector
    FORLOOP
  // par-error@+1 {{unexpected OpenACC clause 'auto' in directive '#pragma acc parallel'}}
  #pragma acc parallel LOOP auto
    FORLOOP
  // par-error@+3 {{unexpected OpenACC clause 'shared' in directive '#pragma acc parallel'}}
  // parloop-error@+2 {{unexpected OpenACC clause 'shared' in directive '#pragma acc parallel loop'}}
  // expected-error@+1 {{expected '(' after 'shared'}}
  #pragma acc parallel LOOP shared
    FORLOOP
  // par-error@+5 {{unexpected OpenACC clause 'shared' in directive '#pragma acc parallel'}}
  // parloop-error@+4 {{unexpected OpenACC clause 'shared' in directive '#pragma acc parallel loop'}}
  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP shared(
    FORLOOP
  // par-error@+4 {{unexpected OpenACC clause 'shared' in directive '#pragma acc parallel'}}
  // parloop-error@+3 {{unexpected OpenACC clause 'shared' in directive '#pragma acc parallel loop'}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP shared(i
    FORLOOP
  // par-error@+2 {{unexpected OpenACC clause 'shared' in directive '#pragma acc parallel'}}
  // parloop-error@+1 {{unexpected OpenACC clause 'shared' in directive '#pragma acc parallel loop'}}
  #pragma acc parallel LOOP shared(i)
    FORLOOP
  // par-error@+2 {{unexpected OpenACC clause 'shared' in directive '#pragma acc parallel'}}
  // parloop-error@+1 {{unexpected OpenACC clause 'shared' in directive '#pragma acc parallel loop'}}
  #pragma acc parallel LOOP shared(i, jk)
    FORLOOP
  // par-error@+1 {{unexpected OpenACC clause 'collapse' in directive '#pragma acc parallel'}}
  #pragma acc parallel LOOP collapse(1)
    FORLOOP

  //--------------------------------------------------
  // Data clauses: syntax
  //--------------------------------------------------

  #pragma acc parallel LOOP copy // expected-error {{expected '(' after 'copy'}}
    FORLOOP
  #pragma acc parallel LOOP pcopy // expected-error {{expected '(' after 'pcopy'}}
    FORLOOP
  #pragma acc parallel LOOP present_or_copy // expected-error {{expected '(' after 'present_or_copy'}}
    FORLOOP
  #pragma acc parallel LOOP copyin // expected-error {{expected '(' after 'copyin'}}
    FORLOOP
  #pragma acc parallel LOOP pcopyin // expected-error {{expected '(' after 'pcopyin'}}
    FORLOOP
  #pragma acc parallel LOOP present_or_copyin // expected-error {{expected '(' after 'present_or_copyin'}}
    FORLOOP
  #pragma acc parallel LOOP copyout // expected-error {{expected '(' after 'copyout'}}
    FORLOOP
  #pragma acc parallel LOOP pcopyout // expected-error {{expected '(' after 'pcopyout'}}
    FORLOOP
  #pragma acc parallel LOOP present_or_copyout // expected-error {{expected '(' after 'present_or_copyout'}}
    FORLOOP
  #pragma acc parallel LOOP firstprivate // expected-error {{expected '(' after 'firstprivate'}}
    FORLOOP
  #pragma acc parallel LOOP private // expected-error {{expected '(' after 'private'}}
    FORLOOP
  #pragma acc parallel LOOP reduction // expected-error {{expected '(' after 'reduction'}}
    FORLOOP

  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP copy(
    FORLOOP
  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP pcopy(
    FORLOOP
  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP present_or_copy(
    FORLOOP
  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP copyin(
    FORLOOP
  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP pcopyin(
    FORLOOP
  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP present_or_copyin(
    FORLOOP
  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP copyout(
    FORLOOP
  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP pcopyout(
    FORLOOP
  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP present_or_copyout(
    FORLOOP
  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP firstprivate(
    FORLOOP
  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP private(
    FORLOOP
  // expected-error@+4 {{expected reduction operator}}
  // expected-warning@+3 {{missing ':' after reduction operator - ignoring}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP reduction(
    FORLOOP

  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP copy()
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP pcopy()
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP present_or_copy()
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP copyin()
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP pcopyin()
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP present_or_copyin()
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP copyout()
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP pcopyout()
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP present_or_copyout()
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP firstprivate()
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP private( )
    FORLOOP
  // expected-error@+2 {{expected reduction operator}}
  // expected-warning@+1 {{missing ':' after reduction operator - ignoring}}
  #pragma acc parallel LOOP reduction()
    FORLOOP

  // expected-error@+1 {{expected reduction operator}}
  #pragma acc parallel LOOP reduction(:)
    FORLOOP
  // expected-error@+1 {{expected reduction operator}}
  #pragma acc parallel LOOP reduction(:i)
    FORLOOP
  // expected-error@+2 {{expected reduction operator}}
  // expected-error@+1 {{use of undeclared identifier 'foo'}}
  #pragma acc parallel LOOP reduction(:foo)
    FORLOOP

  // expected-error@+2 {{expected reduction operator}}
  // expected-warning@+1 {{missing ':' after reduction operator - ignoring}}
  #pragma acc parallel LOOP reduction(-)
    FORLOOP
  // expected-warning@+2 {{missing ':' after reduction operator - ignoring}}
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP reduction(i)
    FORLOOP
  // expected-warning@+2 {{missing ':' after reduction operator - ignoring}}
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP reduction(foo)
    FORLOOP
  // expected-error@+1 {{expected reduction operator}}
  #pragma acc parallel LOOP reduction(-:)
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP reduction(foo:)
    FORLOOP

  // expected-error@+1 {{expected reduction operator}}
  #pragma acc parallel LOOP reduction(-:i)
    FORLOOP
  // OpenACC 2.6 sec. 2.5.12 line 774 mistypes "^" as "%", which is nonsense as a
  // reduction operator.
  // expected-error@+1 {{expected reduction operator}}
  #pragma acc parallel LOOP reduction(% :i)
    FORLOOP

  // expected-warning@+2 {{missing ':' after reduction operator - ignoring}}
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP reduction(+)
    FORLOOP
  // expected-warning@+2 {{missing ':' after reduction operator - ignoring}}
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP reduction(max)
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP reduction(+:)
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP reduction(max:)
    FORLOOP

  #pragma acc parallel LOOP copy(jk i) // expected-error {{expected ',' or ')' in 'copy' clause}}
    FORLOOP
  #pragma acc parallel LOOP pcopy(jk i) // expected-error {{expected ',' or ')' in 'pcopy' clause}}
    FORLOOP
  #pragma acc parallel LOOP present_or_copy(jk i) // expected-error {{expected ',' or ')' in 'present_or_copy' clause}}
    FORLOOP
  #pragma acc parallel LOOP copyin(jk i) // expected-error {{expected ',' or ')' in 'copyin' clause}}
    FORLOOP
  #pragma acc parallel LOOP pcopyin(jk i) // expected-error {{expected ',' or ')' in 'pcopyin' clause}}
    FORLOOP
  #pragma acc parallel LOOP present_or_copyin(jk i) // expected-error {{expected ',' or ')' in 'present_or_copyin' clause}}
    FORLOOP
  #pragma acc parallel LOOP copyout(jk i) // expected-error {{expected ',' or ')' in 'copyout' clause}}
    FORLOOP
  #pragma acc parallel LOOP pcopyout(jk i) // expected-error {{expected ',' or ')' in 'pcopyout' clause}}
    FORLOOP
  #pragma acc parallel LOOP present_or_copyout(jk i) // expected-error {{expected ',' or ')' in 'present_or_copyout' clause}}
    FORLOOP
  #pragma acc parallel LOOP firstprivate(i jk) // expected-error {{expected ',' or ')' in 'firstprivate' clause}}
    FORLOOP
  #pragma acc parallel LOOP private(jk i) // expected-error {{expected ',' or ')' in 'private' clause}}
    FORLOOP
  #pragma acc parallel LOOP reduction(+:i jk) // expected-error {{expected ',' or ')' in 'reduction' clause}}
    FORLOOP

  #pragma acc parallel LOOP copy(jk ,) // expected-error {{expected expression}}
    FORLOOP
  #pragma acc parallel LOOP pcopy(jk ,) // expected-error {{expected expression}}
    FORLOOP
  #pragma acc parallel LOOP present_or_copy(jk ,) // expected-error {{expected expression}}
    FORLOOP
  #pragma acc parallel LOOP copyin(jk ,) // expected-error {{expected expression}}
    FORLOOP
  #pragma acc parallel LOOP pcopyin(jk ,) // expected-error {{expected expression}}
    FORLOOP
  #pragma acc parallel LOOP present_or_copyin(jk ,) // expected-error {{expected expression}}
    FORLOOP
  #pragma acc parallel LOOP copyout(jk ,) // expected-error {{expected expression}}
    FORLOOP
  #pragma acc parallel LOOP pcopyout(jk ,) // expected-error {{expected expression}}
    FORLOOP
  #pragma acc parallel LOOP present_or_copyout(jk ,) // expected-error {{expected expression}}
    FORLOOP
  #pragma acc parallel LOOP firstprivate(i,) // expected-error {{expected expression}}
    FORLOOP
  #pragma acc parallel LOOP private(jk, ) // expected-error {{expected expression}}
    FORLOOP
  #pragma acc parallel LOOP reduction(*:i ,) // expected-error {{expected expression}}
    FORLOOP

  // expected-error@+1 {{expected variable name or subarray}}
  #pragma acc parallel LOOP copy((int)i)
    FORLOOP
  // expected-error@+1 {{expected variable name or subarray}}
  #pragma acc parallel LOOP pcopy((int)i)
    FORLOOP
  // expected-error@+1 {{expected variable name or subarray}}
  #pragma acc parallel LOOP present_or_copy((int)i)
    FORLOOP
  // expected-error@+1 {{expected variable name or subarray}}
  #pragma acc parallel LOOP copyin((int)i)
    FORLOOP
  // expected-error@+1 {{expected variable name or subarray}}
  #pragma acc parallel LOOP pcopyin((int)i)
    FORLOOP
  // expected-error@+1 {{expected variable name or subarray}}
  #pragma acc parallel LOOP present_or_copyin((int)i)
    FORLOOP
  // expected-error@+1 {{expected variable name or subarray}}
  #pragma acc parallel LOOP copyout((int)i)
    FORLOOP
  // expected-error@+1 {{expected variable name or subarray}}
  #pragma acc parallel LOOP pcopyout((int)i)
    FORLOOP
  // expected-error@+1 {{expected variable name or subarray}}
  #pragma acc parallel LOOP present_or_copyout((int)i)
    FORLOOP
  // expected-error@+1 {{expected variable name}}
  #pragma acc parallel LOOP firstprivate((int)i)
    FORLOOP
  // expected-error@+1 {{expected variable name}}
  #pragma acc parallel LOOP private((int)i)
    FORLOOP
  // expected-error@+1 {{expected variable name}}
  #pragma acc parallel LOOP reduction(*:(int)i)
    FORLOOP

  // expected-error@+1 {{expected variable name as base of subarray}}
  #pragma acc parallel LOOP copy(((int*)a)[0:2])
    FORLOOP
  // expected-error@+1 {{expected variable name as base of subarray}}
  #pragma acc parallel LOOP pcopy(((int*)a)[:2])
    FORLOOP
  // expected-error@+1 {{expected variable name as base of subarray}}
  #pragma acc parallel LOOP present_or_copy((*(int(*)[3])a)[0:])
    FORLOOP
  // expected-error@+1 {{expected variable name as base of subarray}}
  #pragma acc parallel LOOP copyin((*((int(*)[3])a))[:])
    FORLOOP
  // expected-error@+1 {{expected variable name as base of subarray}}
  #pragma acc parallel LOOP pcopyin(((int*)a)[0:2])
    FORLOOP
  // expected-error@+1 {{expected variable name as base of subarray}}
  #pragma acc parallel LOOP present_or_copyin(((int*)a)[:2])
    FORLOOP
  // expected-error@+1 {{expected variable name as base of subarray}}
  #pragma acc parallel LOOP copyout(((int*)a)[0:2])
    FORLOOP
  // expected-error@+1 {{expected variable name as base of subarray}}
  #pragma acc parallel LOOP pcopyout(((int*)a)[:2])
    FORLOOP
  // expected-error@+1 {{expected variable name as base of subarray}}
  #pragma acc parallel LOOP present_or_copyout(((int*)a)[0:2])
    FORLOOP
  // expected-error@+2 {{expected variable name as base of subarray}}
  // expected-error@+1 {{subarray is not supported in 'firstprivate' clause}}
  #pragma acc parallel LOOP firstprivate(((int*)a)[:2])
    FORLOOP
  // expected-error@+2 {{expected variable name as base of subarray}}
  // expected-error@+1 {{subarray is not supported in 'private' clause}}
  #pragma acc parallel LOOP private(((int*)a)[0:2])
    FORLOOP
  // expected-error@+2 {{expected variable name as base of subarray}}
  // expected-error@+1 {{subarray is not supported in 'reduction' clause}}
  #pragma acc parallel LOOP reduction(*:((int*)a)[:2])
    FORLOOP

  // The following subarray diagnostics are built on the OpenMP array section
  // implementation, which we assume is tested thoroughly.  Thus, we don't have
  // exhaustive test coverage here.  Instead, we check at least one case for
  // each diagnostic kind to give us some confidence that the OpenACC wordings
  // for those are being used instead of the OpenMP wordings.
  //
  // noacc-error@+3 {{expected ']'}}
  // noacc-note@+2 {{to match this '['}}
  // expected-error@+1 {{OpenACC subarray is not allowed here}}
  a[0:1] = 5;
  // expected-error@+1 {{subscripted value is not an array or pointer}}
  #pragma acc parallel LOOP copy(i[0:2])
    FORLOOP
  // char-subscripts-warning@+1 {{subarray start is of type 'char'}}
  #pragma acc parallel LOOP pcopy(a[c:2])
    FORLOOP
  // char-subscripts-warning@+1 {{subarray length is of type 'char'}}
  #pragma acc parallel LOOP present_or_copy(a[0:c])
    FORLOOP
  // expected-error@+1 {{subarray specified for pointer to function type 'int ()'}}
  #pragma acc parallel LOOP copyin(fp[0:2])
    FORLOOP
  // expected-error@+1 {{subarray specified for pointer to incomplete type 'int []'}}
  #pragma acc parallel LOOP pcopyin((&incomplete)[0:2])
    FORLOOP
  // expected-error@+1 {{subarray must be a subset of the original array}}
  #pragma acc parallel LOOP present_or_copyin(a[-1:2])
    FORLOOP
  // expected-error@+1 {{subarray length evaluates to a negative value -5}}
  #pragma acc parallel LOOP copyout(a[0:-5])
    FORLOOP
  #pragma acc parallel LOOP pcopyout(a[0:])
    FORLOOP
  // expected-error@+1 {{subarray length is unspecified and cannot be inferred because subscripted value is not an array}}
  #pragma acc parallel LOOP present_or_copyout(p[0:])
    FORLOOP
  // expected-error@+1 2 {{subarray length is unspecified and cannot be inferred because subscripted value is an array of unknown bound}}
  #pragma acc parallel LOOP copy(incomplete[0:], (*ap)[0:])
    FORLOOP

  //--------------------------------------------------
  // Data clauses: arg semantics
  //--------------------------------------------------

  // Unknown reduction operator.
  #pragma acc parallel LOOP reduction(foo:i) // expected-error {{unknown reduction operator}}
    FORLOOP
  #pragma acc parallel LOOP reduction(foo:bar) // expected-error {{use of undeclared identifier 'bar'}}
    FORLOOP
  #pragma acc parallel LOOP reduction(foo:a[5]) // expected-error {{unknown reduction operator}}
    FORLOOP

  // Unknown variable name.
  #pragma acc parallel LOOP copy(foo) // expected-error {{use of undeclared identifier 'foo'}}
    FORLOOP
  #pragma acc parallel LOOP pcopy(foo) // expected-error {{use of undeclared identifier 'foo'}}
    FORLOOP
  #pragma acc parallel LOOP present_or_copy(foo) // expected-error {{use of undeclared identifier 'foo'}}
    FORLOOP
  #pragma acc parallel LOOP copyin(foo) // expected-error {{use of undeclared identifier 'foo'}}
    FORLOOP
  #pragma acc parallel LOOP pcopyin(foo) // expected-error {{use of undeclared identifier 'foo'}}
    FORLOOP
  #pragma acc parallel LOOP present_or_copyin(foo) // expected-error {{use of undeclared identifier 'foo'}}
    FORLOOP
  #pragma acc parallel LOOP copyout(foo) // expected-error {{use of undeclared identifier 'foo'}}
    FORLOOP
  #pragma acc parallel LOOP pcopyout(foo) // expected-error {{use of undeclared identifier 'foo'}}
    FORLOOP
  #pragma acc parallel LOOP present_or_copyout(foo) // expected-error {{use of undeclared identifier 'foo'}}
    FORLOOP
  #pragma acc parallel LOOP firstprivate(foo) // expected-error {{use of undeclared identifier 'foo'}}
    FORLOOP
  #pragma acc parallel LOOP private( q) // expected-error {{use of undeclared identifier 'q'}}
    FORLOOP
  #pragma acc parallel LOOP reduction(max:bar) // expected-error {{use of undeclared identifier 'bar'}}
    FORLOOP

  // Array subscript not supported where subarray is permitted.
  // expected-error@+2 {{subarray syntax must include ':'}}
  // expected-error@+2 {{subarray syntax must include ':'}}
  #pragma acc parallel LOOP copy(a[0], \
                                 m[0][0:2], \
                                 a[0:1], m[0:2][0:2])
    FORLOOP
  // expected-error@+2 {{subarray syntax must include ':'}}
  // expected-error@+2 {{subarray syntax must include ':'}}
  #pragma acc parallel LOOP pcopy(a[1], \
                                  m[0:2][1], \
                                  a[0:], m[0:2][0:2])
    FORLOOP
  // expected-error@+2 {{subarray syntax must include ':'}}
  // expected-error@+2 {{subarray syntax must include ':'}}
  #pragma acc parallel LOOP present_or_copy(a[i], \
                                            m[0][1], \
                                            a[:1], m[0:2][0:2])
    FORLOOP
  // expected-error@+2 {{subarray syntax must include ':'}}
  // expected-error@+2 {{subarray syntax must include ':'}}
  #pragma acc parallel LOOP copyin(a[jk], \
                                   m[2:][1], \
                                   a[:], m[0:2][0:2])
    FORLOOP
  // expected-error@+2 {{subarray syntax must include ':'}}
  // expected-error@+2 {{subarray syntax must include ':'}}
  #pragma acc parallel LOOP pcopyin(a[b], \
                                    m[i][3], \
                                    a[0:2], m[0:2][0:2])
    FORLOOP
  // expected-error@+2 {{subarray syntax must include ':'}}
  // expected-error@+2 {{subarray syntax must include ':'}}
  #pragma acc parallel LOOP present_or_copyin(a[e], \
                                              m[jk][:i], \
                                              a[1:], m[0:2][0:2])
    FORLOOP
  // expected-error@+2 {{subarray syntax must include ':'}}
  // expected-error@+2 {{subarray syntax must include ':'}}
  #pragma acc parallel LOOP copyout(a[0], \
                                    m[i][e], \
                                    a[0:2], m[0:2][0:2])
    FORLOOP
  // expected-error@+2 {{subarray syntax must include ':'}}
  // expected-error@+2 {{subarray syntax must include ':'}}
  #pragma acc parallel LOOP pcopyout(a[1], \
                                     m[i:jk][b], \
                                     a[:], m[0:2][0:2])
    FORLOOP
  // expected-error@+2 {{subarray syntax must include ':'}}
  // expected-error@+2 {{subarray syntax must include ':'}}
  #pragma acc parallel LOOP present_or_copyout(a[i], \
                                               m[3][:], \
                                               a[1:1], m[0:2][0:2])
    FORLOOP

  // Subarrays not permitted.
  // expected-error@+6 {{subarray syntax must include ':'}}
  // expected-error@+5 {{subarray is not supported in 'firstprivate' clause}}
  // expected-error@+5 {{subarray is not supported in 'firstprivate' clause}}
  // expected-error@+5 {{subarray is not supported in 'firstprivate' clause}}
  // expected-error@+5 {{subarray is not supported in 'firstprivate' clause}}
  // expected-error@+5 {{subarray is not supported in 'firstprivate' clause}}
  #pragma acc parallel LOOP firstprivate(a[0],   \
                                         a[0:1], \
                                         a[0:],  \
                                         a[:1],  \
                                         a[:])
    FORLOOP
  // expected-error@+6 {{subarray is not supported in 'private' clause}}
  // expected-error@+6 {{subarray syntax must include ':'}}
  // expected-error@+5 {{subarray is not supported in 'private' clause}}
  // expected-error@+5 {{subarray is not supported in 'private' clause}}
  // expected-error@+5 {{subarray is not supported in 'private' clause}}
  // expected-error@+5 {{subarray is not supported in 'private' clause}}
  #pragma acc parallel LOOP private(a[3:5], \
                                    a[10],  \
                                    a[0:],  \
                                    a[:1],  \
                                    a[:])
    FORLOOP
  // expected-error@+6 {{subarray syntax must include ':'}}
  // expected-error@+5 {{subarray is not supported in 'reduction' clause}}
  // expected-error@+5 {{subarray is not supported in 'reduction' clause}}
  // expected-error@+5 {{subarray is not supported in 'reduction' clause}}
  // expected-error@+5 {{subarray is not supported in 'reduction' clause}}
  // expected-error@+5 {{subarray is not supported in 'reduction' clause}}
  #pragma acc parallel LOOP reduction(min:a[23],   \
                                          a[0:10], \
                                          a[0:],   \
                                          a[:1],   \
                                          a[:])
    FORLOOP

  // Variables of incomplete type.
  // expected-error@+13 {{variable in 'copy' clause cannot have incomplete type 'int []'}}
  // expected-error@+12 {{variable in 'pcopy' clause cannot have incomplete type 'int []'}}
  // expected-error@+11 {{variable in 'present_or_copy' clause cannot have incomplete type 'int []'}}
  // expected-error@+11 {{variable in 'copyin' clause cannot have incomplete type 'int []'}}
  // expected-error@+10 {{variable in 'pcopyin' clause cannot have incomplete type 'int []'}}
  // expected-error@+9 {{variable in 'present_or_copyin' clause cannot have incomplete type 'int []'}}
  // expected-error@+9 {{variable in 'copyout' clause cannot have incomplete type 'int []'}}
  // expected-error@+8 {{variable in 'pcopyout' clause cannot have incomplete type 'int []'}}
  // expected-error@+7 {{variable in 'present_or_copyout' clause cannot have incomplete type 'int []'}}
  // expected-error@+7 {{variable in 'firstprivate' clause cannot have incomplete type 'int []'}}
  // expected-error@+6 {{variable in 'private' clause cannot have incomplete type 'int []'}}
  // expected-error@+5 {{variable in 'reduction' clause cannot have incomplete type 'int []'}}
  #pragma acc parallel LOOP \
      copy(incomplete) pcopy(incomplete) present_or_copy(incomplete) \
      copyin(incomplete) pcopyin(incomplete) present_or_copyin(incomplete) \
      copyout(incomplete) pcopyout(incomplete) present_or_copyout(incomplete) \
      firstprivate(incomplete) private(incomplete) reduction(&:incomplete)
    FORLOOP
  // expected-error@+4 {{variable in implied 'copy' clause cannot have incomplete type 'int []'}}
  // expected-note@+1 {{'copy' clause implied here}}
  #pragma acc parallel LOOP
    FORLOOP_HEAD {
      i = *incomplete;
    }
  // expected-error@+3 {{variable in 'copy' clause cannot have incomplete type 'int []'}}
  // expected-error@+4 {{variable in implied 'copy' clause cannot have incomplete type 'int []'}}
  // expected-note@+1 {{'copy' clause implied here}}
  #pragma acc parallel LOOP copy(incomplete)
    FORLOOP_HEAD {
      i = *incomplete;
    }

  // Variables of const type.
  #pragma acc parallel LOOP copy(constI, constIDecl)
    FORLOOP
  #pragma acc parallel LOOP pcopy(constI, constIDecl)
    FORLOOP
  #pragma acc parallel LOOP present_or_copy(constI, constIDecl)
    FORLOOP
  #pragma acc parallel LOOP copyin(constI, constIDecl)
    FORLOOP
  #pragma acc parallel LOOP pcopyin(constI, constIDecl)
    FORLOOP
  #pragma acc parallel LOOP present_or_copyin(constI, constIDecl)
    FORLOOP
  #pragma acc parallel LOOP copyout(constI, constIDecl)
    FORLOOP
  #pragma acc parallel LOOP pcopyout(constI, constIDecl)
    FORLOOP
  #pragma acc parallel LOOP present_or_copyout(constI, constIDecl)
    FORLOOP
  #pragma acc parallel LOOP firstprivate(constI, constIDecl)
    FORLOOP
  // expected-error@+2 2 {{const variable cannot be private because initialization is impossible}}
  // expected-error@+2 2 {{reduction variable cannot be const}}
  #pragma acc parallel LOOP private(constI, constIDecl) \
                            reduction(max: constI, constIDecl)
    FORLOOP

  // Make sure const qualifier isn't lost in the case of implicit/explicit
  // clauses that permit const variables.
  #pragma acc parallel LOOP firstprivate(constI)
    FORLOOP_HEAD {
      // expected-noacc-error@+2 {{cannot assign to variable 'constI' with const-qualified type 'const int'}}
      // expected-noacc-error@+2 {{cannot assign to variable 'constIDecl' with const-qualified type 'const int'}}
      constI = 5;
      constIDecl = 5;
    }
  #pragma acc parallel LOOP copy(constADecl)
    FORLOOP_HEAD {
      // expected-noacc-error@+2 {{cannot assign to variable 'constA' with const-qualified type 'const int [3]'}}
      // expected-noacc-error@+2 {{cannot assign to variable 'constADecl' with const-qualified type 'const int [3]'}}
      constA[0] = 5;
      constADecl[1] = 5;
    }
  #pragma acc parallel LOOP pcopy(constA)
    FORLOOP_HEAD {
      // expected-noacc-error@+2 {{cannot assign to variable 'constA' with const-qualified type 'const int [3]'}}
      // expected-noacc-error@+2 {{cannot assign to variable 'constADecl' with const-qualified type 'const int [3]'}}
      constA[0] = 5;
      constADecl[1] = 5;
    }
  #pragma acc parallel LOOP present_or_copy(constADecl)
    FORLOOP_HEAD {
      // expected-noacc-error@+2 {{cannot assign to variable 'constADecl' with const-qualified type 'const int [3]'}}
      // expected-noacc-error@+2 {{cannot assign to variable 'constA' with const-qualified type 'const int [3]'}}
      constADecl[1] = 5;
      constA[0] = 5;
    }
  #pragma acc parallel LOOP copyin(constA) copyout(constADecl)
    FORLOOP_HEAD {
      // expected-noacc-error@+2 {{cannot assign to variable 'constA' with const-qualified type 'const int [3]'}}
      // expected-noacc-error@+2 {{cannot assign to variable 'constADecl' with const-qualified type 'const int [3]'}}
      constA[0] = 5;
      constADecl[1] = 5;
    }
  #pragma acc parallel LOOP pcopyin(constADecl) pcopyout(constA)
    FORLOOP_HEAD {
      // expected-noacc-error@+2 {{cannot assign to variable 'constA' with const-qualified type 'const int [3]'}}
      // expected-noacc-error@+2 {{cannot assign to variable 'constADecl' with const-qualified type 'const int [3]'}}
      constA[0] = 5;
      constADecl[1] = 5;
    }
  #pragma acc parallel LOOP present_or_copyin(constA) present_or_copyout(constADecl)
    FORLOOP_HEAD {
      // expected-noacc-error@+2 {{cannot assign to variable 'constADecl' with const-qualified type 'const int [3]'}}
      // expected-noacc-error@+2 {{cannot assign to variable 'constA' with const-qualified type 'const int [3]'}}
      constADecl[1] = 5;
      constA[0] = 5;
    }

  // Reduction operator doesn't permit variable type.
  #pragma acc parallel LOOP reduction(max:b,e,i,jk,f,d,p)
    FORLOOP
  // expected-error@+6 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
  // expected-error@+5 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
  // expected-error@+4 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
  // expected-error@+3 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
  // expected-error@+2 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
  // expected-error@+1 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
  #pragma acc parallel LOOP reduction(max:fc,dc,a,s,u,uDecl)
    FORLOOP
  #pragma acc parallel LOOP reduction(min:b,e,i,jk,f,d,p)
    FORLOOP
  // expected-error@+6 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
  // expected-error@+5 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
  // expected-error@+4 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
  // expected-error@+3 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
  // expected-error@+2 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
  // expected-error@+1 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
  #pragma acc parallel LOOP reduction(min:fc,dc,a,s,u,uDecl)
    FORLOOP
  #pragma acc parallel LOOP reduction(+:b,e,i,jk,f,d,fc,dc)
    FORLOOP
  // expected-error@+5 {{OpenACC reduction operator '+' argument must be of arithmetic type}}
  // expected-error@+4 {{OpenACC reduction operator '+' argument must be of arithmetic type}}
  // expected-error@+3 {{OpenACC reduction operator '+' argument must be of arithmetic type}}
  // expected-error@+2 {{OpenACC reduction operator '+' argument must be of arithmetic type}}
  // expected-error@+1 {{OpenACC reduction operator '+' argument must be of arithmetic type}}
  #pragma acc parallel LOOP reduction(+:p,a,s,u,uDecl)
    FORLOOP
  #pragma acc parallel LOOP reduction(*:b,e,i,jk,f,d,fc,dc)
    FORLOOP
  // expected-error@+5 {{OpenACC reduction operator '*' argument must be of arithmetic type}}
  // expected-error@+4 {{OpenACC reduction operator '*' argument must be of arithmetic type}}
  // expected-error@+3 {{OpenACC reduction operator '*' argument must be of arithmetic type}}
  // expected-error@+2 {{OpenACC reduction operator '*' argument must be of arithmetic type}}
  // expected-error@+1 {{OpenACC reduction operator '*' argument must be of arithmetic type}}
  #pragma acc parallel LOOP reduction(*:p,a,s,u,uDecl)
    FORLOOP
  #pragma acc parallel LOOP reduction(&&:b,e,i,jk,f,d,fc,dc)
    FORLOOP
  // expected-error@+5 {{OpenACC reduction operator '&&' argument must be of arithmetic type}}
  // expected-error@+4 {{OpenACC reduction operator '&&' argument must be of arithmetic type}}
  // expected-error@+3 {{OpenACC reduction operator '&&' argument must be of arithmetic type}}
  // expected-error@+2 {{OpenACC reduction operator '&&' argument must be of arithmetic type}}
  // expected-error@+1 {{OpenACC reduction operator '&&' argument must be of arithmetic type}}
  #pragma acc parallel LOOP reduction(&&:p,a,s,u,uDecl)
    FORLOOP
  #pragma acc parallel LOOP reduction(||:b,e,i,jk,f,d,fc,dc)
    FORLOOP
  // expected-error@+5 {{OpenACC reduction operator '||' argument must be of arithmetic type}}
  // expected-error@+4 {{OpenACC reduction operator '||' argument must be of arithmetic type}}
  // expected-error@+3 {{OpenACC reduction operator '||' argument must be of arithmetic type}}
  // expected-error@+2 {{OpenACC reduction operator '||' argument must be of arithmetic type}}
  // expected-error@+1 {{OpenACC reduction operator '||' argument must be of arithmetic type}}
  #pragma acc parallel LOOP reduction(||:p,a,s,u,uDecl)
    FORLOOP
  #pragma acc parallel LOOP reduction(&:b,e,i,jk)
    FORLOOP
  // expected-error@+9 {{OpenACC reduction operator '&' argument must be of integer type}}
  // expected-error@+8 {{OpenACC reduction operator '&' argument must be of integer type}}
  // expected-error@+7 {{OpenACC reduction operator '&' argument must be of integer type}}
  // expected-error@+6 {{OpenACC reduction operator '&' argument must be of integer type}}
  // expected-error@+5 {{OpenACC reduction operator '&' argument must be of integer type}}
  // expected-error@+4 {{OpenACC reduction operator '&' argument must be of integer type}}
  // expected-error@+3 {{OpenACC reduction operator '&' argument must be of integer type}}
  // expected-error@+2 {{OpenACC reduction operator '&' argument must be of integer type}}
  // expected-error@+1 {{OpenACC reduction operator '&' argument must be of integer type}}
  #pragma acc parallel LOOP reduction(&:f,d,fc,dc,p,a,s,u,uDecl)
    FORLOOP
  #pragma acc parallel LOOP reduction(|:b,e,i,jk)
    FORLOOP
  // expected-error@+9 {{OpenACC reduction operator '|' argument must be of integer type}}
  // expected-error@+8 {{OpenACC reduction operator '|' argument must be of integer type}}
  // expected-error@+7 {{OpenACC reduction operator '|' argument must be of integer type}}
  // expected-error@+6 {{OpenACC reduction operator '|' argument must be of integer type}}
  // expected-error@+5 {{OpenACC reduction operator '|' argument must be of integer type}}
  // expected-error@+4 {{OpenACC reduction operator '|' argument must be of integer type}}
  // expected-error@+3 {{OpenACC reduction operator '|' argument must be of integer type}}
  // expected-error@+2 {{OpenACC reduction operator '|' argument must be of integer type}}
  // expected-error@+1 {{OpenACC reduction operator '|' argument must be of integer type}}
  #pragma acc parallel LOOP reduction(|:f,d,fc,dc,p,a,s,u,uDecl)
    FORLOOP
  #pragma acc parallel LOOP reduction(^:b,e,i,jk)
    FORLOOP
  // expected-error@+9 {{OpenACC reduction operator '^' argument must be of integer type}}
  // expected-error@+8 {{OpenACC reduction operator '^' argument must be of integer type}}
  // expected-error@+7 {{OpenACC reduction operator '^' argument must be of integer type}}
  // expected-error@+6 {{OpenACC reduction operator '^' argument must be of integer type}}
  // expected-error@+5 {{OpenACC reduction operator '^' argument must be of integer type}}
  // expected-error@+4 {{OpenACC reduction operator '^' argument must be of integer type}}
  // expected-error@+3 {{OpenACC reduction operator '^' argument must be of integer type}}
  // expected-error@+2 {{OpenACC reduction operator '^' argument must be of integer type}}
  // expected-error@+1 {{OpenACC reduction operator '^' argument must be of integer type}}
  #pragma acc parallel LOOP reduction(^:f,d,fc,dc,p,a,s,u,uDecl)
    FORLOOP

  // Redundant clauses.
  // expected-error@+6 {{copy variable defined again as copy variable}}
  // expected-note@+5 {{previously defined as copy variable here}}
  // expected-error@+5 {{copy variable defined again as copy variable}}
  // expected-note@+3 {{previously defined as copy variable here}}
  // expected-error@+4 {{copy variable defined again as copy variable}}
  // expected-note@+1 {{previously defined as copy variable here}}
  #pragma acc parallel LOOP copy(e,e,i,jk) \
                            copy(i)        \
                            pcopy(jk)
    FORLOOP
  // expected-error@+6 {{copy variable defined again as copy variable}}
  // expected-note@+5 {{previously defined as copy variable here}}
  // expected-error@+5 {{copy variable defined again as copy variable}}
  // expected-note@+3 {{previously defined as copy variable here}}
  // expected-error@+4 {{copy variable defined again as copy variable}}
  // expected-note@+1 {{previously defined as copy variable here}}
  #pragma acc parallel LOOP pcopy(e,e,i,jk) \
                            pcopy(i)        \
                            present_or_copy(jk)
    FORLOOP
  // expected-error@+6 {{copy variable defined again as copy variable}}
  // expected-note@+5 {{previously defined as copy variable here}}
  // expected-error@+5 {{copy variable defined again as copy variable}}
  // expected-note@+3 {{previously defined as copy variable here}}
  // expected-error@+4 {{copy variable defined again as copy variable}}
  // expected-note@+1 {{previously defined as copy variable here}}
  #pragma acc parallel LOOP copy(e,e,i,jk)      \
                            present_or_copy(i)  \
                            present_or_copy(jk)
    FORLOOP
  // expected-error@+6 {{copy variable defined again as copy variable}}
  // expected-note@+5 {{previously defined as copy variable here}}
  // expected-error@+5 {{copy variable defined again as copy variable}}
  // expected-note@+3 {{previously defined as copy variable here}}
  // expected-error@+4 {{copy variable defined again as copy variable}}
  // expected-note@+1 {{previously defined as copy variable here}}
  #pragma acc parallel LOOP present_or_copy(e,e,i,jk) \
                            pcopy(i)                  \
                            copy(jk)
    FORLOOP
  // expected-error@+6 {{copyin variable defined again as copyin variable}}
  // expected-note@+5 {{previously defined as copyin variable here}}
  // expected-error@+5 {{copyin variable defined again as copyin variable}}
  // expected-note@+3 {{previously defined as copyin variable here}}
  // expected-error@+4 {{copyin variable defined again as copyin variable}}
  // expected-note@+1 {{previously defined as copyin variable here}}
  #pragma acc parallel LOOP copyin(e,e,i,jk) \
                            copyin(i)        \
                            pcopyin(jk)
    FORLOOP
  // expected-error@+6 {{copyin variable defined again as copyin variable}}
  // expected-note@+5 {{previously defined as copyin variable here}}
  // expected-error@+5 {{copyin variable defined again as copyin variable}}
  // expected-note@+3 {{previously defined as copyin variable here}}
  // expected-error@+4 {{copyin variable defined again as copyin variable}}
  // expected-note@+1 {{previously defined as copyin variable here}}
  #pragma acc parallel LOOP pcopyin(e,e,i,jk) \
                            pcopyin(i)        \
                            present_or_copyin(jk)
    FORLOOP
  // expected-error@+6 {{copyin variable defined again as copyin variable}}
  // expected-note@+5 {{previously defined as copyin variable here}}
  // expected-error@+5 {{copyin variable defined again as copyin variable}}
  // expected-note@+3 {{previously defined as copyin variable here}}
  // expected-error@+4 {{copyin variable defined again as copyin variable}}
  // expected-note@+1 {{previously defined as copyin variable here}}
  #pragma acc parallel LOOP copyin(e,e,i,jk)      \
                            present_or_copyin(i)  \
                            present_or_copyin(jk)
    FORLOOP
  // expected-error@+6 {{copyin variable defined again as copyin variable}}
  // expected-note@+5 {{previously defined as copyin variable here}}
  // expected-error@+5 {{copyin variable defined again as copyin variable}}
  // expected-note@+3 {{previously defined as copyin variable here}}
  // expected-error@+4 {{copyin variable defined again as copyin variable}}
  // expected-note@+1 {{previously defined as copyin variable here}}
  #pragma acc parallel LOOP present_or_copyin(e,e,i,jk) \
                            pcopyin(i)                  \
                            copyin(jk)
    FORLOOP
  // expected-error@+6 {{copyout variable defined again as copyout variable}}
  // expected-note@+5 {{previously defined as copyout variable here}}
  // expected-error@+5 {{copyout variable defined again as copyout variable}}
  // expected-note@+3 {{previously defined as copyout variable here}}
  // expected-error@+4 {{copyout variable defined again as copyout variable}}
  // expected-note@+1 {{previously defined as copyout variable here}}
  #pragma acc parallel LOOP copyout(e,e,i,jk) \
                            copyout(i)        \
                            pcopyout(jk)
    FORLOOP
  // expected-error@+6 {{copyout variable defined again as copyout variable}}
  // expected-note@+5 {{previously defined as copyout variable here}}
  // expected-error@+5 {{copyout variable defined again as copyout variable}}
  // expected-note@+3 {{previously defined as copyout variable here}}
  // expected-error@+4 {{copyout variable defined again as copyout variable}}
  // expected-note@+1 {{previously defined as copyout variable here}}
  #pragma acc parallel LOOP pcopyout(e,e,i,jk) \
                            pcopyout(i)        \
                            present_or_copyout(jk)
    FORLOOP
  // expected-error@+6 {{copyout variable defined again as copyout variable}}
  // expected-note@+5 {{previously defined as copyout variable here}}
  // expected-error@+5 {{copyout variable defined again as copyout variable}}
  // expected-note@+3 {{previously defined as copyout variable here}}
  // expected-error@+4 {{copyout variable defined again as copyout variable}}
  // expected-note@+1 {{previously defined as copyout variable here}}
  #pragma acc parallel LOOP copyout(e,e,i,jk)      \
                            present_or_copyout(i)  \
                            present_or_copyout(jk)
    FORLOOP
  // expected-error@+6 {{copyout variable defined again as copyout variable}}
  // expected-note@+5 {{previously defined as copyout variable here}}
  // expected-error@+5 {{copyout variable defined again as copyout variable}}
  // expected-note@+3 {{previously defined as copyout variable here}}
  // expected-error@+4 {{copyout variable defined again as copyout variable}}
  // expected-note@+1 {{previously defined as copyout variable here}}
  #pragma acc parallel LOOP present_or_copyout(e,e,i,jk) \
                            pcopyout(i)                  \
                            copyout(jk)
    FORLOOP
  // expected-error@+2 {{copy variable defined again as copy variable}}
  // expected-note@+1 {{previously defined as copy variable here}}
  #pragma acc parallel LOOP copy(a[0:2], a[0:2])
    FORLOOP
  // expected-error@+2 {{copyin variable defined again as copyin variable}}
  // expected-note@+1 {{previously defined as copyin variable here}}
  #pragma acc parallel LOOP copyin(a[0:1]) copyin(a[1:1])
    FORLOOP
  // expected-error@+2 {{copyout variable defined again as copyout variable}}
  // expected-note@+1 {{previously defined as copyout variable here}}
  #pragma acc parallel LOOP copyout(a[0:2]) pcopyout(a[1:1])
    FORLOOP
  // expected-error@+6 {{firstprivate variable defined again as firstprivate variable}}
  // expected-note@+5 {{previously defined as firstprivate variable here}}
  // expected-error@+6 {{firstprivate variable defined again as firstprivate variable}}
  // expected-note@+4 {{previously defined as firstprivate variable here}}
  // expected-error@+4 {{firstprivate variable defined again as firstprivate variable}}
  // expected-note@+1 {{previously defined as firstprivate variable here}}
  #pragma acc parallel LOOP firstprivate(a,p,a) \
                            firstprivate(f) \
                            firstprivate(f,p)
    FORLOOP
  // expected-error@+5 3 {{private variable defined again as private variable}}
  // expected-note@+3 3 {{previously defined as private variable here}}
  // expected-error@+4 3 {{private variable defined again as private variable}}
  // expected-note@+1 3 {{previously defined as private variable here}}
  #pragma acc parallel LOOP private(d,fc,dc) \
                            private(fc,dc,d) \
                            private(dc,d,fc)
    FORLOOP
  // expected-error@+6 {{redundant '&&' reduction for variable 'i'}}
  // expected-note@+5 {{previous '&&' reduction here}}
  // expected-error@+5 {{redundant '&&' reduction for variable 'jk'}}
  // expected-note@+3 {{previous '&&' reduction here}}
  // expected-error@+4 {{conflicting 'max' reduction for variable 'd'}}
  // expected-note@+1 {{previous '&&' reduction here}}
  #pragma acc parallel LOOP reduction(&&:i,i,jk,d) \
                            reduction(&&:jk) \
                            reduction(max:d)
    FORLOOP

  // Conflicting clauses: copy and aliases vs. other clauses.
  // expected-error@+5 2 {{copy variable cannot be copyin variable}}
  // expected-note@+3 2 {{previously defined as copy variable here}}
  // expected-error@+4 {{copyin variable cannot be copy variable}}
  // expected-note@+2 {{previously defined as copyin variable here}}
  #pragma acc parallel LOOP copy(i,a[0:1]) \
                            copyin(i,jk,a[0:1]) \
                            pcopy(jk)
    FORLOOP
  // expected-error@+5 {{copyin variable cannot be copy variable}}
  // expected-note@+3 {{previously defined as copyin variable here}}
  // expected-error@+4 {{copy variable cannot be copyin variable}}
  // expected-note@+2 {{previously defined as copy variable here}}
  #pragma acc parallel LOOP pcopyin(a) \
                            present_or_copy(a[0:2],p) \
                            present_or_copyin(p)
    FORLOOP
  // expected-error@+5 {{copyout variable cannot be copy variable}}
  // expected-note@+3 {{previously defined as copyout variable here}}
  // expected-error@+4 {{copy variable cannot be copyout variable}}
  // expected-note@+2 {{previously defined as copy variable here}}
  #pragma acc parallel LOOP present_or_copyout(a[0:1]) \
                            pcopy(a,p) \
                            pcopyout(p)
    FORLOOP
  // expected-error@+5 {{copy variable cannot be copyout variable}}
  // expected-note@+3 {{previously defined as copy variable here}}
  // expected-error@+4 2 {{copyout variable cannot be copy variable}}
  // expected-note@+2 2 {{previously defined as copyout variable here}}
  #pragma acc parallel LOOP present_or_copy(f) \
                            copyout(f,d,a[0:1]) \
                            copy(d,a[1:1])
    FORLOOP
  // expected-error@+5 2 {{copy variable cannot be firstprivate variable}}
  // expected-note@+3 2 {{previously defined as copy variable here}}
  // expected-error@+4 {{firstprivate variable cannot be copy variable}}
  // expected-note@+2 {{previously defined as firstprivate variable here}}
  #pragma acc parallel LOOP copy(i,a[0:2]) \
                            firstprivate(i,jk,a) \
                            pcopy(jk)
    FORLOOP
  // expected-error@+5 {{firstprivate variable cannot be copy variable}}
  // expected-note@+3 {{previously defined as firstprivate variable here}}
  // expected-error@+4 {{copy variable cannot be firstprivate variable}}
  // expected-note@+2 {{previously defined as copy variable here}}
  #pragma acc parallel LOOP firstprivate(a) \
                            present_or_copy(a[0:1],p) \
                            firstprivate(p)
    FORLOOP
  // expected-error@+5 {{private variable cannot be copy variable}}
  // expected-note@+3 {{previously defined as private variable here}}
  // expected-error@+4 {{copy variable cannot be private variable}}
  // expected-note@+2 {{previously defined as copy variable here}}
  #pragma acc parallel LOOP private(a) \
                            pcopy(a[1:1],p) \
                            private(p)
    FORLOOP
  // expected-error@+5 {{copy variable cannot be private variable}}
  // expected-note@+3 {{previously defined as copy variable here}}
  // expected-error@+4 {{private variable cannot be copy variable}}
  // expected-note@+2 {{previously defined as private variable here}}
  #pragma acc parallel LOOP present_or_copy(f) \
                            private(f,d) \
                            copy(d)
    FORLOOP
  #pragma acc parallel LOOP present_or_copy(f) reduction(+:f,d) pcopy(d)
    FORLOOP
  #pragma acc parallel LOOP reduction(*:i) copy(i,jk) reduction(*:jk)
    FORLOOP

  // Conflicting clauses: copyin and aliases vs. other clauses (except copy,
  // which is tested above).
  // expected-error@+5 {{copyout variable cannot be copyin variable}}
  // expected-note@+3 {{previously defined as copyout variable here}}
  // expected-error@+4 {{copyin variable cannot be copyout variable}}
  // expected-note@+2 {{previously defined as copyin variable here}}
  #pragma acc parallel LOOP pcopyout(a) \
                            present_or_copyin(a,p) \
                            copyout(p)
    FORLOOP
  // expected-error@+5 {{copyin variable cannot be copyout variable}}
  // expected-note@+3 {{previously defined as copyin variable here}}
  // expected-error@+4 {{copyout variable cannot be copyin variable}}
  // expected-note@+2 {{previously defined as copyout variable here}}
  #pragma acc parallel LOOP pcopyin(f) \
                            present_or_copyout(f,d) \
                            copyin(d)
    FORLOOP
  // expected-error@+5 {{copyin variable cannot be firstprivate variable}}
  // expected-note@+3 {{previously defined as copyin variable here}}
  // expected-error@+4 {{firstprivate variable cannot be copyin variable}}
  // expected-note@+2 {{previously defined as firstprivate variable here}}
  #pragma acc parallel LOOP pcopyin(i) \
                            firstprivate(i,jk) \
                            present_or_copyin(jk)
    FORLOOP
  // expected-error@+5 {{firstprivate variable cannot be copyin variable}}
  // expected-note@+3 {{previously defined as firstprivate variable here}}
  // expected-error@+4 {{copyin variable cannot be firstprivate variable}}
  // expected-note@+2 {{previously defined as copyin variable here}}
  #pragma acc parallel LOOP firstprivate(a) \
                            copyin(a,p) \
                            firstprivate(p)
    FORLOOP
  // expected-error@+5 {{private variable cannot be copyin variable}}
  // expected-note@+3 {{previously defined as private variable here}}
  // expected-error@+4 {{copyin variable cannot be private variable}}
  // expected-note@+2 {{previously defined as copyin variable here}}
  #pragma acc parallel LOOP private(a) \
                            copyin(a,p) \
                            private(p)
    FORLOOP
  // expected-error@+5 {{copyin variable cannot be private variable}}
  // expected-note@+3 {{previously defined as copyin variable here}}
  // expected-error@+4 {{private variable cannot be copyin variable}}
  // expected-note@+2 {{previously defined as private variable here}}
  #pragma acc parallel LOOP pcopyin(f) \
                            private(f,d) \
                            present_or_copyin(d)
    FORLOOP
  #pragma acc parallel LOOP pcopyin(f) reduction(+:f,d) present_or_copyin(d)
    FORLOOP
  #pragma acc parallel LOOP reduction(*:i) pcopyin(i,jk) reduction(*:jk)
    FORLOOP

  // Conflicting clauses: copyout and aliases vs. other clauses (except copy
  // and copyin, which are tested above).
  // expected-error@+5 {{copyout variable cannot be firstprivate variable}}
  // expected-note@+3 {{previously defined as copyout variable here}}
  // expected-error@+4 {{firstprivate variable cannot be copyout variable}}
  // expected-note@+2 {{previously defined as firstprivate variable here}}
  #pragma acc parallel LOOP copyout(i) \
                            firstprivate(i,jk) \
                            pcopyout(jk)
    FORLOOP
  // expected-error@+5 {{firstprivate variable cannot be copyout variable}}
  // expected-note@+3 {{previously defined as firstprivate variable here}}
  // expected-error@+4 {{copyout variable cannot be firstprivate variable}}
  // expected-note@+2 {{previously defined as copyout variable here}}
  #pragma acc parallel LOOP firstprivate(a) \
                            present_or_copyout(a,p) \
                            firstprivate(p)
    FORLOOP
  // expected-error@+5 {{private variable cannot be copyout variable}}
  // expected-note@+3 {{previously defined as private variable here}}
  // expected-error@+4 {{copyout variable cannot be private variable}}
  // expected-note@+2 {{previously defined as copyout variable here}}
  #pragma acc parallel LOOP private(a) \
                            present_or_copyout(a,p) \
                            private(p)
    FORLOOP
  // expected-error@+5 {{copyout variable cannot be private variable}}
  // expected-note@+3 {{previously defined as copyout variable here}}
  // expected-error@+4 {{private variable cannot be copyout variable}}
  // expected-note@+2 {{previously defined as private variable here}}
  #pragma acc parallel LOOP pcopyout(f) \
                            private(f,d) \
                            copyout(d)
    FORLOOP
  #pragma acc parallel LOOP present_or_copyout(f) reduction(+:f,d) pcopyout(d)
    FORLOOP
  #pragma acc parallel LOOP reduction(*:i) copyout(i,jk) reduction(*:jk)
    FORLOOP

  // Conflicting clauses: firstprivate vs. private vs. reduction (copy, copyin,
  // and copyout are tested above).
  // expected-error@+5 {{firstprivate variable cannot be private variable}}
  // expected-note@+3 {{previously defined as firstprivate variable here}}
  // expected-error@+4 {{private variable cannot be firstprivate variable}}
  // expected-note@+2 {{previously defined as private variable here}}
  #pragma acc parallel LOOP firstprivate(i) \
                            private(i,jk) \
                            firstprivate(jk)
    FORLOOP
  // expected-error@+5 {{private variable cannot be reduction variable}}
  // expected-note@+3 {{previously defined as private variable here}}
  // expected-error@+5 {{firstprivate variable cannot be reduction variable}}
  // expected-note@+3 {{previously defined as firstprivate variable here}}
  #pragma acc parallel LOOP private(i) \
                            reduction(min:i) \
                            firstprivate(d) \
                            reduction(+:d)
    FORLOOP
  // expected-error@+5 {{reduction variable cannot be private variable}}
  // expected-note@+3 {{previously defined as reduction}}
  // expected-error@+5 {{reduction variable cannot be firstprivate variable}}
  // expected-note@+3 {{previously defined as reduction}}
  #pragma acc parallel LOOP reduction(max:i) \
                            private(i) \
                            reduction(*:d) \
                            firstprivate(d)
    FORLOOP

  //--------------------------------------------------
  // Partition count clauses
  //--------------------------------------------------

  // expected-error@+3 {{expected '(' after 'num_gangs'}}
  // expected-error@+2 {{expected '(' after 'num_workers'}}
  // expected-error@+1 {{expected '(' after 'vector_length'}}
  #pragma acc parallel LOOP num_gangs num_workers vector_length
    FORLOOP
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP num_gangs(1
    FORLOOP
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP num_workers( i
    FORLOOP
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP vector_length (3 + 5
    FORLOOP
  // par-error@+2 {{directive '#pragma acc parallel' cannot contain more than one 'num_gangs' clause}}
  // parloop-error@+1 {{directive '#pragma acc parallel loop' cannot contain more than one 'num_gangs' clause}}
  #pragma acc parallel LOOP num_gangs(i) num_gangs( i)
    FORLOOP
  // par-error@+4 {{directive '#pragma acc parallel' cannot contain more than one 'num_workers' clause}}
  // par-error@+3 {{directive '#pragma acc parallel' cannot contain more than one 'num_workers' clause}}
  // parloop-error@+2 {{directive '#pragma acc parallel loop' cannot contain more than one 'num_workers' clause}}
  // parloop-error@+1 {{directive '#pragma acc parallel loop' cannot contain more than one 'num_workers' clause}}
  #pragma acc parallel LOOP num_workers(1) num_workers(1 ) num_workers(i+3)
    FORLOOP
  // par-error@+4 {{directive '#pragma acc parallel' cannot contain more than one 'vector_length' clause}}
  // par-error@+3 {{directive '#pragma acc parallel' cannot contain more than one 'vector_length' clause}}
  // parloop-error@+2 {{directive '#pragma acc parallel loop' cannot contain more than one 'vector_length' clause}}
  // parloop-error@+1 {{directive '#pragma acc parallel loop' cannot contain more than one 'vector_length' clause}}
  #pragma acc parallel LOOP vector_length(1) vector_length(2 ) vector_length(3)
    FORLOOP
  // expected-error@+3 {{use of undeclared identifier 'bogusg'}}
  // expected-error@+2 {{use of undeclared identifier 'bogusw'}}
  // expected-error@+1 {{use of undeclared identifier 'bogusv'}}
  #pragma acc parallel LOOP num_gangs(bogusg) num_workers(bogusw) vector_length(bogusv)
    FORLOOP
  // acc-ignore-warning@+1 {{'vector_length' ignored because argument is not an integer constant expression}}
  #pragma acc parallel LOOP num_gangs(i*1) num_workers(i+3) vector_length(i)
    FORLOOP
  #pragma acc parallel LOOP num_gangs(31 * 3 + 1) num_workers(10 + 5) vector_length(5 * 2)
    FORLOOP
  #pragma acc parallel LOOP num_gangs(1) num_workers(1) vector_length(1)
    FORLOOP
  // expected-error@+3 {{argument to 'num_gangs' clause must be a strictly positive integer value}}
  // expected-error@+2 {{argument to 'num_workers' clause must be a strictly positive integer value}}
  // expected-error@+1 {{argument to 'vector_length' clause must be a strictly positive integer value}}
  #pragma acc parallel LOOP num_gangs(0) num_workers(0) vector_length(0)
    FORLOOP
  // expected-error@+3 {{argument to 'num_gangs' clause must be a strictly positive integer value}}
  // expected-error@+2 {{argument to 'num_workers' clause must be a strictly positive integer value}}
  // expected-error@+1 {{argument to 'vector_length' clause must be a strictly positive integer value}}
  #pragma acc parallel LOOP num_gangs(0u) num_workers(0u) vector_length(0u)
    FORLOOP
  // expected-error@+3 {{argument to 'num_gangs' clause must be a strictly positive integer value}}
  // expected-error@+2 {{argument to 'num_workers' clause must be a strictly positive integer value}}
  // expected-error@+1 {{argument to 'vector_length' clause must be a strictly positive integer value}}
  #pragma acc parallel LOOP num_gangs(-1) num_workers(-1) vector_length(-1)
    FORLOOP
  // expected-error@+3 {{argument to 'num_gangs' clause must be a strictly positive integer value}}
  // expected-error@+2 {{argument to 'num_workers' clause must be a strictly positive integer value}}
  // expected-error@+1 {{argument to 'vector_length' clause must be a strictly positive integer value}}
  #pragma acc parallel LOOP num_gangs(-5) num_workers(-5) vector_length(-5)
    FORLOOP
  // expected-error@+4 {{expression must have integral or unscoped enumeration type, not 'double'}}
  // expected-error@+3 {{expression must have integral or unscoped enumeration type, not 'float'}}
  // expected-error@+2 {{expression must have integral or unscoped enumeration type, not '_Complex float'}}
  // acc-ignore-warning@+1 {{'vector_length' ignored because argument is not an integer constant expression}}
  #pragma acc parallel LOOP num_gangs(d) num_workers(f) vector_length(fc)
    FORLOOP
  // expected-error@+4 {{expression must have integral or unscoped enumeration type, not 'float'}}
  // expected-error@+3 {{expression must have integral or unscoped enumeration type, not 'double'}}
  // expected-error@+2 {{expression must have integral or unscoped enumeration type, not 'long double'}}
  // acc-ignore-warning@+1 {{'vector_length' ignored because argument is not an integer constant expression}}
  #pragma acc parallel LOOP num_gangs(1.0f) num_workers(2e3) vector_length(1.3l)
    FORLOOP

  //--------------------------------------------------
  // Nesting
  //--------------------------------------------------

  #pragma acc parallel // expected-note {{enclosing '#pragma acc parallel' here}}
  // par-error@+2 {{'#pragma acc parallel' cannot be nested within '#pragma acc parallel'}}
  // parloop-error@+1 {{'#pragma acc parallel loop' cannot be nested within '#pragma acc parallel'}}
  #pragma acc parallel LOOP
    FORLOOP

  #pragma acc parallel
  {
    #pragma acc loop seq // expected-note {{enclosing '#pragma acc loop' here}}
    for (int i = 0; i < 2; ++i) {
      // par-error@+2 {{'#pragma acc parallel' cannot be nested within '#pragma acc loop'}}
      // parloop-error@+1 {{'#pragma acc parallel loop' cannot be nested within '#pragma acc loop'}}
      #pragma acc parallel LOOP
        FORLOOP
    }
  }

  #pragma acc parallel loop // expected-note {{enclosing '#pragma acc parallel loop' here}}
  for (int i = 0; i < 2; ++i) {
    // par-error@+2 {{'#pragma acc parallel' cannot be nested within '#pragma acc parallel loop'}}
    // parloop-error@+1 {{'#pragma acc parallel loop' cannot be nested within '#pragma acc parallel loop'}}
    #pragma acc parallel LOOP
      FORLOOP
  }

  //--------------------------------------------------
  // Miscellaneous
  //--------------------------------------------------

  // At one time, an assert failed due to the use of arrNoSize.
  int arrNoSize[]; // expected-noacc-error {{definition of variable with array type needs an explicit size or an initializer}}
  #pragma acc parallel LOOP
  #if ADD_LOOP_TO_PAR
  for (int i = 0; i < 2; ++i)
  #endif
    arrNoSize;

  return 0;
}

// Complete to suppress an additional warning, but it's too late for pragmas.
int incomplete[3];

// The remaining diagnostics are currently produced by OpenMP sema during the
// transform from OpenACC to OpenMP, which is skipped if there have been any
// previous diagnostics.  Thus, each must be tested in a separate compilation.
//
// We don't check every OpenMP diagnostic case as the OpenMP tests should do
// that.  We just check some of them (roughly one of each diagnostic kind) to
// give us some confidence that they're handled properly by the OpenACC
// implementation.
//
// TODO: We currently don't even check one of each diagnostic kind.  Look in
// ActOnOpenMPMapClause and its callees, such as checkMappableExpressionList
// and checkMapClauseExpressionBase in SemaOpenMP.cpp.

#elif ERR == ERR_OMP_ARRAY_SECTION_NON_CONTIGUOUS_AP

// noacc-no-diagnostics

int main() {
  int *ap[3];
  // expected-error@+1 {{array section does not specify contiguous storage}}
  #pragma acc parallel LOOP copy(ap[0:2][0:3])
    FORLOOP
  return 0;
}

#elif ERR == ERR_OMP_ARRAY_SECTION_NON_CONTIGUOUS_PP

// noacc-no-diagnostics

int main() {
  int **pp;
  // expected-error@+1 {{array section does not specify contiguous storage}}
  #pragma acc parallel LOOP copy(pp[0:2][0:3])
    FORLOOP
  return 0;
}

#else
# error undefined ERR
#endif
