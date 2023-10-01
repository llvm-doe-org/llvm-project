// Check diagnostics for "acc parallel".
//
// When ADD_LOOP_TO_PAR is not set, this file checks diagnostics for "acc
// parallel" without "loop".
//
// When ADD_LOOP_TO_PAR is set, it adds "loop" and a for loop to those "acc
// parallel" directives in order to check the same diagnostics but for combined
// "acc parallel loop" directives.

// DEFINE: %{check}( CFLAGS %) =                                               \

//           # OpenACC disabled
// DEFINE:   %clang_cc1 -verify=noacc,expected-noacc -Wchar-subscripts         \
// DEFINE:      %{CFLAGS} %s &&                                                \
// DEFINE:   %clang_cc1 -verify=noacc,expected-noacc -Wchar-subscripts         \
// DEFINE:      %{CFLAGS} -DADD_LOOP_TO_PAR %s &&                              \

//           # OpenACC enabled
// DEFINE:   %clang_cc1 \
// DEFINE:     -verify=par,acc-ignore,expected,expected-noacc,char-subscripts  \
// DEFINE:     -fopenacc -Wchar-subscripts %{CFLAGS} %s &&                     \
// DEFINE:   %clang_cc1 \
// DEFINE:     -verify=parloop,acc-ignore,expected,expected-noacc,char-subscripts \
// DEFINE:     -fopenacc -Wchar-subscripts %{CFLAGS} -DADD_LOOP_TO_PAR %s &&   \

//           # OpenACC enabled but optional warnings disabled
// DEFINE:   %clang_cc1 -verify=par,expected,expected-noacc -fopenacc          \
// DEFINE:      -Wno-openacc-ignored-clause %{CFLAGS} %s &&                    \
// DEFINE:   %clang_cc1 -verify=parloop,expected,expected-noacc -fopenacc      \
// DEFINE:      -Wno-openacc-ignored-clause %{CFLAGS} -DADD_LOOP_TO_PAR %s

// RUN: %{check}( -DERR=ERR_ACC                                 %)
// RUN: %{check}( -DERR=ERR_ACC2OMP_ASYNC2DEP_FOR_SYNC          %)
// RUN: %{check}( -DERR=ERR_ACC2OMP_ASYNC2DEP_MISSING_FOR_ASYNC %)
// RUN: %{check}( -DERR=ERR_ACC2OMP_ASYNC2DEP_NOT_FN_FOR_ASYNC  %)
// RUN: %{check}( -DERR=ERR_ACC2OMP_ASYNC2DEP_BAD_FN_FOR_ASYNC  %)
// RUN: %{check}( -DERR=ERR_ACC2OMP_ASYNC2DEP_MISSING_FOR_DEF   %)
// RUN: %{check}( -DERR=ERR_ACC2OMP_ASYNC2DEP_NOT_FN_FOR_DEF    %)
// RUN: %{check}( -DERR=ERR_ACC2OMP_ASYNC2DEP_BAD_FN_FOR_DEF    %)
// RUN: %{check}( -DERR=ERR_OMP_ARRAY_SECTION_NON_CONTIGUOUS_AP %)
// RUN: %{check}( -DERR=ERR_OMP_ARRAY_SECTION_NON_CONTIGUOUS_PP %)

// END.

#define ERR_ACC                                 1
#define ERR_ACC2OMP_ASYNC2DEP_FOR_SYNC          2
#define ERR_ACC2OMP_ASYNC2DEP_MISSING_FOR_ASYNC 3
#define ERR_ACC2OMP_ASYNC2DEP_NOT_FN_FOR_ASYNC  4
#define ERR_ACC2OMP_ASYNC2DEP_BAD_FN_FOR_ASYNC  5
#define ERR_ACC2OMP_ASYNC2DEP_MISSING_FOR_DEF   6
#define ERR_ACC2OMP_ASYNC2DEP_NOT_FN_FOR_DEF    7
#define ERR_ACC2OMP_ASYNC2DEP_BAD_FN_FOR_DEF    8
#define ERR_OMP_ARRAY_SECTION_NON_CONTIGUOUS_AP 9
#define ERR_OMP_ARRAY_SECTION_NON_CONTIGUOUS_PP 10

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
  int i, jk, a[2], m[6][2], *p; // expected-note 9 {{variable 'a' declared here}}
                                // expected-note@-1 7 {{variable 'p' declared here}}
  int (*fp)();
  int (*ap)[];
  float f; // expected-note 3 {{variable 'f' declared here}}
  double d; // expected-note 3 {{variable 'd' declared here}}
  float _Complex fc; // expected-note 5 {{variable 'fc' declared here}}
  double _Complex dc; // expected-note 5 {{variable 'dc' declared here}}
  // expected-note@+2 8 {{variable 'constI' declared const here}}
  // expected-noacc-note@+1 3 {{variable 'constI' declared const here}}
  const int constI = 5;
  // expected-note@+2 8 {{variable 'constIDecl' declared const here}}
  // expected-noacc-note@+1 3 {{variable 'constIDecl' declared const here}}
  const extern int constIDecl;
  // expected-noacc-note@+1 5 {{variable 'constA' declared const here}}
  const int constA[3];
  // expected-noacc-note@+1 4 {{variable 'constADecl' declared const here}}
  const extern int constADecl[3];
  struct S { int i; } s; // expected-note 9 {{variable 's' declared here}}
  union U { int i; } u; // expected-note 9 {{variable 'u' declared here}}
  extern union U uDecl; // expected-note 9 {{variable 'uDecl' declared here}}
  struct SS { struct S s; struct S *ps; } ss;
  struct S *ps;
  union U *pu;
  struct SS *pss;

  //----------------------------------------------------------------------------
  // No clauses
  //----------------------------------------------------------------------------

  #pragma acc parallel LOOP
    FORLOOP

  //----------------------------------------------------------------------------
  // Unrecognized clauses
  //----------------------------------------------------------------------------

  // Bogus clauses.

  // par-warning@+2 {{extra tokens at the end of '#pragma acc parallel' are ignored}}
  // parloop-warning@+1 {{extra tokens at the end of '#pragma acc parallel loop' are ignored}}
  #pragma acc parallel LOOP foo
    FORLOOP
  // par-warning@+2 {{extra tokens at the end of '#pragma acc parallel' are ignored}}
  // parloop-warning@+1 {{extra tokens at the end of '#pragma acc parallel loop' are ignored}}
  #pragma acc parallel LOOP foo bar
    FORLOOP

  // Well formed clauses not permitted here.

  // par-error@+6 {{unexpected OpenACC clause 'nomap' in directive '#pragma acc parallel'}}
  // parloop-error@+5 {{unexpected OpenACC clause 'nomap' in directive '#pragma acc parallel loop'}}
  // par-error@+4 {{unexpected OpenACC clause 'delete' in directive '#pragma acc parallel'}}
  // parloop-error@+3 {{unexpected OpenACC clause 'delete' in directive '#pragma acc parallel loop'}}
  // par-error@+2 {{unexpected OpenACC clause 'shared' in directive '#pragma acc parallel'}}
  // parloop-error@+1 {{unexpected OpenACC clause 'shared' in directive '#pragma acc parallel loop'}}
  #pragma acc parallel LOOP nomap(i) delete(i) shared(i, jk)
    FORLOOP
  // par-error@+10 {{unexpected OpenACC clause 'if' in directive '#pragma acc parallel'}}
  // parloop-error@+9 {{unexpected OpenACC clause 'if' in directive '#pragma acc parallel loop'}}
  // par-error@+8 {{unexpected OpenACC clause 'if_present' in directive '#pragma acc parallel'}}
  // parloop-error@+7 {{unexpected OpenACC clause 'if_present' in directive '#pragma acc parallel loop'}}
  // par-error@+6 {{unexpected OpenACC clause 'self' in directive '#pragma acc parallel'}}
  // parloop-error@+5 {{unexpected OpenACC clause 'self' in directive '#pragma acc parallel loop'}}
  // par-error@+4 {{unexpected OpenACC clause 'host' in directive '#pragma acc parallel'}}
  // parloop-error@+3 {{unexpected OpenACC clause 'host' in directive '#pragma acc parallel loop'}}
  // par-error@+2 {{unexpected OpenACC clause 'device' in directive '#pragma acc parallel'}}
  // parloop-error@+1 {{unexpected OpenACC clause 'device' in directive '#pragma acc parallel loop'}}
  #pragma acc parallel LOOP if(1) if_present self(i) host(i) device(i)
    FORLOOP
  // par-error@+12 {{unexpected OpenACC clause 'seq' in directive '#pragma acc parallel'}}
  // par-error@+11 {{unexpected OpenACC clause 'independent' in directive '#pragma acc parallel'}}
  // par-error@+10 {{unexpected OpenACC clause 'auto' in directive '#pragma acc parallel'}}
  // par-error@+9 {{unexpected OpenACC clause 'gang' in directive '#pragma acc parallel'}}
  // par-error@+8 {{unexpected OpenACC clause 'worker' in directive '#pragma acc parallel'}}
  // par-error@+7 {{unexpected OpenACC clause 'vector' in directive '#pragma acc parallel'}}
  // parloop-error@+6 {{unexpected OpenACC clause 'independent', 'seq' is specified already}}
  // parloop-error@+5 {{unexpected OpenACC clause 'auto', 'seq' is specified already}}
  // parloop-error@+4 {{unexpected OpenACC clause 'auto', 'independent' is specified already}}
  // parloop-error@+3 {{unexpected OpenACC clause 'gang', 'seq' is specified already}}
  // parloop-error@+2 {{unexpected OpenACC clause 'worker', 'seq' is specified already}}
  // parloop-error@+1 {{unexpected OpenACC clause 'vector', 'seq' is specified already}}
  #pragma acc parallel LOOP seq independent auto gang worker vector
    FORLOOP
  // par-error@+1 {{unexpected OpenACC clause 'collapse' in directive '#pragma acc parallel'}}
  #pragma acc parallel LOOP collapse(1)
    FORLOOP
  // par-error@+1 {{unexpected OpenACC clause 'tile' in directive '#pragma acc parallel'}}
  #pragma acc parallel LOOP tile(4)
    FORLOOP
  // par-error@+10    {{unexpected OpenACC clause 'read' in directive '#pragma acc parallel'}}
  // parloop-error@+9 {{unexpected OpenACC clause 'read' in directive '#pragma acc parallel loop'}}
  // par-error@+8     {{unexpected OpenACC clause 'write' in directive '#pragma acc parallel'}}
  // parloop-error@+7 {{unexpected OpenACC clause 'write' in directive '#pragma acc parallel loop'}}
  // par-error@+6     {{unexpected OpenACC clause 'update' in directive '#pragma acc parallel'}}
  // parloop-error@+5 {{unexpected OpenACC clause 'update' in directive '#pragma acc parallel loop'}}
  // par-error@+4     {{unexpected OpenACC clause 'capture' in directive '#pragma acc parallel'}}
  // parloop-error@+3 {{unexpected OpenACC clause 'capture' in directive '#pragma acc parallel loop'}}
  // par-error@+2     {{unexpected OpenACC clause 'compare' in directive '#pragma acc parallel'}}
  // parloop-error@+1 {{unexpected OpenACC clause 'compare' in directive '#pragma acc parallel loop'}}
  #pragma acc parallel LOOP read write update capture compare
    FORLOOP

  // Malformed clauses not permitted here.

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

  //----------------------------------------------------------------------------
  // Data clauses: syntax
  //----------------------------------------------------------------------------

  // expected-error@+1 {{expected '(' after 'present'}}
  #pragma acc parallel LOOP present
    FORLOOP
  // expected-error@+1 {{expected '(' after 'copy'}}
  #pragma acc parallel LOOP copy
    FORLOOP
  // expected-error@+1 {{expected '(' after 'pcopy'}}
  #pragma acc parallel LOOP pcopy
    FORLOOP
  // expected-error@+1 {{expected '(' after 'present_or_copy'}}
  #pragma acc parallel LOOP present_or_copy
    FORLOOP
  // expected-error@+1 {{expected '(' after 'copyin'}}
  #pragma acc parallel LOOP copyin
    FORLOOP
  // expected-error@+1 {{expected '(' after 'pcopyin'}}
  #pragma acc parallel LOOP pcopyin
    FORLOOP
  // expected-error@+1 {{expected '(' after 'present_or_copyin'}}
  #pragma acc parallel LOOP present_or_copyin
    FORLOOP
  // expected-error@+1 {{expected '(' after 'copyout'}}
  #pragma acc parallel LOOP copyout
    FORLOOP
  // expected-error@+1 {{expected '(' after 'pcopyout'}}
  #pragma acc parallel LOOP pcopyout
    FORLOOP
  // expected-error@+1 {{expected '(' after 'present_or_copyout'}}
  #pragma acc parallel LOOP present_or_copyout
    FORLOOP
  // expected-error@+1 {{expected '(' after 'create'}}
  #pragma acc parallel LOOP create
    FORLOOP
  // expected-error@+1 {{expected '(' after 'pcreate'}}
  #pragma acc parallel LOOP pcreate
    FORLOOP
  // expected-error@+1 {{expected '(' after 'present_or_create'}}
  #pragma acc parallel LOOP present_or_create
    FORLOOP
  // expected-error@+1 {{expected '(' after 'no_create'}}
  #pragma acc parallel LOOP no_create
    FORLOOP
  // expected-error@+1 {{expected '(' after 'firstprivate'}}
  #pragma acc parallel LOOP firstprivate
    FORLOOP
  // expected-error@+1 {{expected '(' after 'private'}}
  #pragma acc parallel LOOP private
    FORLOOP
  // expected-error@+1 {{expected '(' after 'reduction'}}
  #pragma acc parallel LOOP reduction
    FORLOOP

  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP present(
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
  #pragma acc parallel LOOP create(
    FORLOOP
  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP pcreate(
    FORLOOP
  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP present_or_create(
    FORLOOP
  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP no_create(
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

  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP present(i
    FORLOOP
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP copy( i
    FORLOOP
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP pcopy(jk
    FORLOOP
  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP present_or_copy(i,
    FORLOOP
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP copyin( i, jk
    FORLOOP
  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP pcopyin(i,jk,
    FORLOOP
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP present_or_copyin(i,jk,p
    FORLOOP
  // expected-error@+3 {{use of undeclared identifier 'foobar'}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP copyout(foobar
    FORLOOP
  // expected-error@+4 {{use of undeclared identifier 'foobar'}}
  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP pcopyout(foobar,
    FORLOOP
  // expected-error@+3 {{use of undeclared identifier 'foobar'}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP present_or_copyout(i, foobar, jk
    FORLOOP
  // expected-error@+3 {{use of undeclared identifier 'foobar'}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP create(foobar
    FORLOOP
  // expected-error@+4 {{use of undeclared identifier 'foobar'}}
  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP pcreate(foobar,
    FORLOOP
  // expected-error@+3 {{use of undeclared identifier 'foobar'}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP present_or_create(i, foobar, jk
    FORLOOP
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP no_create(i
    FORLOOP
  // expected-error@+4 {{use of undeclared identifier 'foobar'}}
  // expected-error@+3 {{expected expression}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP firstprivate(foobar, i,
    FORLOOP
  // expected-error@+4 {{use of undeclared identifier 'foobar'}}
  // expected-error@+3 {{use of undeclared identifier 'foobar'}}
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP private(foobar, foobar
    FORLOOP
  // expected-warning@+7 {{missing ':' after reduction operator - ignoring}}
  // expected-error@+6 {{expected expression}}
  // expected-error@+5 {{use of undeclared identifier 'foobar'}}
  // expected-error@+4 {{expected expression}}
  // expected-error@+3 {{expected ')'}}
  // expected-note@+2 {{to match this '('}}
  // expected-error@+1 {{unknown reduction operator}}
  #pragma acc parallel LOOP reduction(i, jk, foobar,
    FORLOOP
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP reduction(+:i, jk
    FORLOOP

  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP present()
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP copy( )
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP pcopy ()
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP present_or_copy ( )
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
  #pragma acc parallel LOOP create()
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP pcreate()
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP present_or_create()
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP no_create()
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

  // expected-error@+1 {{expected ',' or ')' in 'present' clause}}
  #pragma acc parallel LOOP present(i jk)
    FORLOOP
  // expected-error@+1 {{expected ',' or ')' in 'copy' clause}}
  #pragma acc parallel LOOP copy(jk i)
    FORLOOP
  // expected-error@+1 {{expected ',' or ')' in 'pcopy' clause}}
  #pragma acc parallel LOOP pcopy(jk i)
    FORLOOP
  // expected-error@+1 {{expected ',' or ')' in 'present_or_copy' clause}}
  #pragma acc parallel LOOP present_or_copy(jk i)
    FORLOOP
  // expected-error@+1 {{expected ',' or ')' in 'copyin' clause}}
  #pragma acc parallel LOOP copyin(jk i)
    FORLOOP
  // expected-error@+1 {{expected ',' or ')' in 'pcopyin' clause}}
  #pragma acc parallel LOOP pcopyin(jk i)
    FORLOOP
  // expected-error@+1 {{expected ',' or ')' in 'present_or_copyin' clause}}
  #pragma acc parallel LOOP present_or_copyin(jk i)
    FORLOOP
  // expected-error@+1 {{expected ',' or ')' in 'copyout' clause}}
  #pragma acc parallel LOOP copyout(jk i)
    FORLOOP
  // expected-error@+1 {{expected ',' or ')' in 'pcopyout' clause}}
  #pragma acc parallel LOOP pcopyout(jk i)
    FORLOOP
  // expected-error@+1 {{expected ',' or ')' in 'present_or_copyout' clause}}
  #pragma acc parallel LOOP present_or_copyout(jk i)
    FORLOOP
  // expected-error@+1 {{expected ',' or ')' in 'create' clause}}
  #pragma acc parallel LOOP create(jk i)
    FORLOOP
  // expected-error@+1 {{expected ',' or ')' in 'pcreate' clause}}
  #pragma acc parallel LOOP pcreate(jk i)
    FORLOOP
  // expected-error@+1 {{expected ',' or ')' in 'present_or_create' clause}}
  #pragma acc parallel LOOP present_or_create(jk i)
    FORLOOP
  // expected-error@+1 {{expected ',' or ')' in 'no_create' clause}}
  #pragma acc parallel LOOP no_create(i jk)
    FORLOOP
  // expected-error@+1 {{expected ',' or ')' in 'firstprivate' clause}}
  #pragma acc parallel LOOP firstprivate(i jk)
    FORLOOP
  // expected-error@+1 {{expected ',' or ')' in 'private' clause}}
  #pragma acc parallel LOOP private(jk i)
    FORLOOP
  // expected-error@+1 {{expected ',' or ')' in 'reduction' clause}}
  #pragma acc parallel LOOP reduction(+:i jk)
    FORLOOP

  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP present(i,)
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP copy(jk ,)
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP pcopy(jk ,)
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP present_or_copy(jk ,)
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP copyin(jk ,)
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP pcopyin(jk ,)
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP present_or_copyin(jk ,)
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP copyout(jk ,)
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP pcopyout(jk ,)
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP present_or_copyout(jk ,)
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP create(jk ,)
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP pcreate(jk ,)
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP present_or_create(jk ,)
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP no_create(i,)
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP firstprivate(i,)
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP private(jk, )
    FORLOOP
  // expected-error@+1 {{expected expression}}
  #pragma acc parallel LOOP reduction(*:i ,)
    FORLOOP

  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP present((int)jk)
    FORLOOP
  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP copy((int)i)
    FORLOOP
  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP pcopy((int)i)
    FORLOOP
  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP present_or_copy((int)i)
    FORLOOP
  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP copyin((int)i)
    FORLOOP
  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP pcopyin((int)i)
    FORLOOP
  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP present_or_copyin((int)i)
    FORLOOP
  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP copyout((int)i)
    FORLOOP
  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP pcopyout((int)i)
    FORLOOP
  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP present_or_copyout((int)i)
    FORLOOP
  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP create((int)i)
    FORLOOP
  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP pcreate((int)i)
    FORLOOP
  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP present_or_create((int)i)
    FORLOOP
  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP no_create((int)jk)
    FORLOOP
  // expected-error-re@+1 {{expected variable name{{$}}}}
  #pragma acc parallel LOOP firstprivate((int)i)
    FORLOOP
  // expected-error-re@+1 {{expected variable name{{$}}}}
  #pragma acc parallel LOOP private((int)i)
    FORLOOP
  // expected-error-re@+1 {{expected variable name{{$}}}}
  #pragma acc parallel LOOP reduction(*:(int)i)
    FORLOOP

  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP present(((int*)a)[1:1])
    FORLOOP
  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP copy(((int*)a)[0:2])
    FORLOOP
  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP pcopy(((int*)a)[:2])
    FORLOOP
  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP present_or_copy((*(int(*)[3])a)[0:])
    FORLOOP
  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP copyin((*((int(*)[3])a))[:])
    FORLOOP
  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP pcopyin(((int*)a)[0:2])
    FORLOOP
  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP present_or_copyin(((int*)a)[:2])
    FORLOOP
  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP copyout(((int*)a)[0:2])
    FORLOOP
  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP pcopyout(((int*)a)[:2])
    FORLOOP
  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP present_or_copyout(((int*)a)[0:2])
    FORLOOP
  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP create(((int*)a)[0:2])
    FORLOOP
  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP pcreate(((int*)a)[:2])
    FORLOOP
  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP present_or_create(((int*)a)[0:2])
    FORLOOP
  // expected-error@+1 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP no_create(((int*)a)[1:1])
    FORLOOP
  // expected-error-re@+3 {{expected variable name{{$}}}}
  // par-error@+2 {{in 'firstprivate' clause on '#pragma acc parallel', subarray is not supported}}
  // parloop-error@+1 {{in 'firstprivate' clause on '#pragma acc parallel loop', subarray is not supported}}
  #pragma acc parallel LOOP firstprivate(((int*)a)[:2])
    FORLOOP
  // expected-error-re@+3 {{expected variable name{{$}}}}
  // par-error@+2 {{in 'private' clause on '#pragma acc parallel', subarray is not supported}}
  // parloop-error@+1 {{in 'private' clause on '#pragma acc parallel loop', subarray is not supported}}
  #pragma acc parallel LOOP private(((int*)a)[0:2])
    FORLOOP
  // expected-error-re@+3 {{expected variable name{{$}}}}
  // par-error@+2 {{in 'reduction' clause on '#pragma acc parallel', subarray is not supported}}
  // parloop-error@+1 {{in 'reduction' clause on '#pragma acc parallel loop', subarray is not supported}}
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
  #pragma acc parallel LOOP present(i[c:c])
    FORLOOP
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
  // expected-error@+1 {{subarray specified for pointer to incomplete type 'int[]'}}
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
  #pragma acc parallel LOOP create(incomplete[0:]) no_create((*ap)[0:])
    FORLOOP

  //----------------------------------------------------------------------------
  // Data clauses: arg semantics
  //----------------------------------------------------------------------------

  // Unknown reduction operator.

  // expected-error@+1 {{unknown reduction operator}}
  #pragma acc parallel LOOP reduction(foo:i)
    FORLOOP
  // expected-error@+1 {{use of undeclared identifier 'bar'}}
  #pragma acc parallel LOOP reduction(foo:bar)
    FORLOOP
  // expected-error@+1 {{unknown reduction operator}}
  #pragma acc parallel LOOP reduction(foo:a[5])
    FORLOOP

  // Unknown variable name.

  // expected-error@+1 {{use of undeclared identifier 'foo'}}
  #pragma acc parallel LOOP present(foo)
    FORLOOP
  // expected-error@+1 {{use of undeclared identifier 'foo'}}
  #pragma acc parallel LOOP copy(foo)
    FORLOOP
  // expected-error@+1 {{use of undeclared identifier 'foo'}}
  #pragma acc parallel LOOP pcopy(foo)
    FORLOOP
  // expected-error@+1 {{use of undeclared identifier 'foo'}}
  #pragma acc parallel LOOP present_or_copy(foo)
    FORLOOP
  // expected-error@+1 {{use of undeclared identifier 'foo'}}
  #pragma acc parallel LOOP copyin(foo)
    FORLOOP
  // expected-error@+1 {{use of undeclared identifier 'foo'}}
  #pragma acc parallel LOOP pcopyin(foo)
    FORLOOP
  // expected-error@+1 {{use of undeclared identifier 'foo'}}
  #pragma acc parallel LOOP present_or_copyin(foo)
    FORLOOP
  // expected-error@+1 {{use of undeclared identifier 'foo'}}
  #pragma acc parallel LOOP copyout(foo)
    FORLOOP
  // expected-error@+1 {{use of undeclared identifier 'foo'}}
  #pragma acc parallel LOOP pcopyout(foo)
    FORLOOP
  // expected-error@+1 {{use of undeclared identifier 'foo'}}
  #pragma acc parallel LOOP present_or_copyout(foo)
    FORLOOP
  // expected-error@+1 {{use of undeclared identifier 'foo'}}
  #pragma acc parallel LOOP create(foo)
    FORLOOP
  // expected-error@+1 {{use of undeclared identifier 'foo'}}
  #pragma acc parallel LOOP pcreate(foo)
    FORLOOP
  // expected-error@+1 {{use of undeclared identifier 'foo'}}
  #pragma acc parallel LOOP present_or_create(foo)
    FORLOOP
  // expected-error@+1 {{use of undeclared identifier 'foo'}}
  #pragma acc parallel LOOP no_create(foo)
    FORLOOP
  // expected-error@+1 {{use of undeclared identifier 'foo'}}
  #pragma acc parallel LOOP firstprivate(foo)
    FORLOOP
  // expected-error@+1 {{use of undeclared identifier 'q'}}
  #pragma acc parallel LOOP private( q)
    FORLOOP
  // expected-error@+1 {{use of undeclared identifier 'bar'}}
  #pragma acc parallel LOOP reduction(max:bar)
    FORLOOP

  // Member expression not permitted.

  // par-error-re@+8 {{in 'firstprivate' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // par-error-re@+8 {{in 'firstprivate' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // par-error-re@+8 {{in 'firstprivate' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // par-error-re@+8 {{in 'firstprivate' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // parloop-error-re@+4 {{in 'firstprivate' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  // parloop-error-re@+4 {{in 'firstprivate' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  // parloop-error-re@+4 {{in 'firstprivate' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  // parloop-error-re@+4 {{in 'firstprivate' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  #pragma acc parallel LOOP firstprivate(s.i,   \
                                         u.i,   \
                                         ps->i, \
                                         pu->i)
    FORLOOP
  // par-error-re@+8 {{in 'private' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // par-error-re@+8 {{in 'private' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // par-error-re@+8 {{in 'private' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // par-error-re@+8 {{in 'private' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // parloop-error-re@+4 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  // parloop-error-re@+4 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  // parloop-error-re@+4 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  // parloop-error-re@+4 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  #pragma acc parallel LOOP private(s.i,   \
                                    u.i,   \
                                    ps->i, \
                                    pu->i)
    FORLOOP
  // par-error-re@+8 {{in 'reduction' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // par-error-re@+8 {{in 'reduction' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // par-error-re@+8 {{in 'reduction' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // par-error-re@+8 {{in 'reduction' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // parloop-error-re@+4 {{in 'reduction' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  // parloop-error-re@+4 {{in 'reduction' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  // parloop-error-re@+4 {{in 'reduction' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  // parloop-error-re@+4 {{in 'reduction' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  #pragma acc parallel LOOP reduction(min:s.i,   \
                                      u.i,   \
                                      ps->i, \
                                      pu->i)
    FORLOOP

  // Nested member expression not permitted.

  // par-error-re@+12 {{in 'firstprivate' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // par-error-re@+12 {{in 'firstprivate' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // par-error-re@+12 {{in 'firstprivate' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // par-error-re@+12 {{in 'firstprivate' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // parloop-error-re@+8 {{in 'firstprivate' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  // parloop-error-re@+8 {{in 'firstprivate' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  // parloop-error-re@+8 {{in 'firstprivate' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  // parloop-error-re@+8 {{in 'firstprivate' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  // expected-error-re@+4 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error-re@+4 {{nested member expression is not supported{{$}}}} // range is for ss.ps
  // expected-error-re@+4 {{nested member expression is not supported{{$}}}} // range is for pss->s
  // expected-error-re@+4 {{nested member expression is not supported{{$}}}} // range is for pss->ps
  #pragma acc parallel LOOP firstprivate(ss.s.i,   \
                                         ss.ps->i,   \
                                         pss->s.i, \
                                         pss->ps->i)
    FORLOOP
  // par-error-re@+12 {{in 'private' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // par-error-re@+12 {{in 'private' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // par-error-re@+12 {{in 'private' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // par-error-re@+12 {{in 'private' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // parloop-error-re@+8 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  // parloop-error-re@+8 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  // parloop-error-re@+8 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  // parloop-error-re@+8 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  // expected-error-re@+4 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error-re@+4 {{nested member expression is not supported{{$}}}} // range is for ss.ps
  // expected-error-re@+4 {{nested member expression is not supported{{$}}}} // range is for pss->s
  // expected-error-re@+4 {{nested member expression is not supported{{$}}}} // range is for pss->ps
  #pragma acc parallel LOOP private(ss.s.i,   \
                                    ss.ps->i,   \
                                    pss->s.i, \
                                    pss->ps->i)
    FORLOOP
  // par-error-re@+12 {{in 'reduction' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // par-error-re@+12 {{in 'reduction' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // par-error-re@+12 {{in 'reduction' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // par-error-re@+12 {{in 'reduction' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // parloop-error-re@+8 {{in 'reduction' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  // parloop-error-re@+8 {{in 'reduction' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  // parloop-error-re@+8 {{in 'reduction' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  // parloop-error-re@+8 {{in 'reduction' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  // expected-error-re@+4 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error-re@+4 {{nested member expression is not supported{{$}}}} // range is for ss.ps
  // expected-error-re@+4 {{nested member expression is not supported{{$}}}} // range is for pss->s
  // expected-error-re@+4 {{nested member expression is not supported{{$}}}} // range is for pss->ps
  #pragma acc parallel LOOP reduction(+:ss.s.i,   \
                                      ss.ps->i,   \
                                      pss->s.i, \
                                      pss->ps->i)
    FORLOOP
  // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for ss.s
  // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for ss.s
  #pragma acc parallel LOOP present(ss.s.i)            \
                            copy(ss.s.i)               \
                            pcopy(ss.s.i)              \
                            present_or_copy(ss.s.i)    \
                            copyin(ss.s.i)             \
                            pcopyin(ss.s.i)            \
                            present_or_copyin(ss.s.i)  \
                            copyout(ss.s.i)            \
                            pcopyout(ss.s.i)           \
                            present_or_copyout(ss.s.i) \
                            create(ss.s.i)             \
                            pcreate(ss.s.i)            \
                            present_or_create(ss.s.i)  \
                            no_create(ss.s.i)
    FORLOOP

  // Array subscript not supported where subarray is permitted.

  // expected-error@+4 {{subarray syntax must include ':'}}
  // expected-error@+6 {{subarray syntax must include ':'}}
  // expected-error@+8 {{subarray syntax must include ':'}}
  #pragma acc parallel LOOP present(a[0        \
                                       ]       \
                                       ,       \
                                    m[0        \
                                       ]       \
                                       [0:2],  \
                                    m[0:2][0   \
                                            ]  \
                                             , \
                                    a[0:1], m[0:2][0:2])
    FORLOOP
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
  // expected-error@+3 {{subarray syntax must include ':'}}
  // expected-error@+3 {{subarray syntax must include ':'}}
  // expected-error@+2 {{expected variable name or data member expression or subarray}}
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
  // expected-error@+3 {{subarray syntax must include ':'}}
  // expected-error@+3 {{subarray syntax must include ':'}}
  // expected-error@+2 {{expected variable name or data member expression or subarray}}
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
  // expected-error@+3 {{subarray syntax must include ':'}}
  // expected-error@+3 {{subarray syntax must include ':'}}
  // expected-error@+2 {{expected variable name or data member expression or subarray}}
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
  // expected-error@+3 {{subarray syntax must include ':'}}
  // expected-error@+3 {{subarray syntax must include ':'}}
  // expected-error@+2 {{expected variable name or data member expression or subarray}}
  #pragma acc parallel LOOP create(a[0], \
                                   m[i][e], \
                                   a[0:2], m[0:2][0:2])
    FORLOOP
  // expected-error@+2 {{subarray syntax must include ':'}}
  // expected-error@+2 {{subarray syntax must include ':'}}
  #pragma acc parallel LOOP pcreate(a[1], \
                                    m[i:jk][b], \
                                    a[:], m[0:2][0:2])
    FORLOOP
  // expected-error@+2 {{subarray syntax must include ':'}}
  // expected-error@+2 {{subarray syntax must include ':'}}
  #pragma acc parallel LOOP present_or_create(a[i], \
                                              m[3][:], \
                                              a[1:1], m[0:2][0:2])
    FORLOOP
  // expected-error@+2 {{subarray syntax must include ':'}}
  // expected-error@+2 {{subarray syntax must include ':'}}
  #pragma acc parallel LOOP no_create(a[0], \
                                      m[0][0:2], \
                                      a[0:1], m[0:2][0:2])
    FORLOOP

  // Subarray not permitted.

  // expected-error@+21 {{subarray syntax must include ':'}}
  // expected-error@+21 {{subarray syntax must include ':'}}
  // expected-error@+21 {{subarray syntax must include ':'}}
  // par-error@+14 {{in 'firstprivate' clause on '#pragma acc parallel', subarray is not supported}}
  // par-error@+14 {{in 'firstprivate' clause on '#pragma acc parallel', subarray is not supported}}
  // par-error@+14 {{in 'firstprivate' clause on '#pragma acc parallel', subarray is not supported}}
  // par-error@+14 {{in 'firstprivate' clause on '#pragma acc parallel', subarray is not supported}}
  // par-error@+14 {{in 'firstprivate' clause on '#pragma acc parallel', subarray is not supported}}
  // par-error@+14 2 {{in 'firstprivate' clause on '#pragma acc parallel', subarray is not supported}}
  // par-error@+14 2 {{in 'firstprivate' clause on '#pragma acc parallel', subarray is not supported}}
  // parloop-error@+7 {{in 'firstprivate' clause on '#pragma acc parallel loop', subarray is not supported}}
  // parloop-error@+7 {{in 'firstprivate' clause on '#pragma acc parallel loop', subarray is not supported}}
  // parloop-error@+7 {{in 'firstprivate' clause on '#pragma acc parallel loop', subarray is not supported}}
  // parloop-error@+7 {{in 'firstprivate' clause on '#pragma acc parallel loop', subarray is not supported}}
  // parloop-error@+7 {{in 'firstprivate' clause on '#pragma acc parallel loop', subarray is not supported}}
  // parloop-error@+7 2 {{in 'firstprivate' clause on '#pragma acc parallel loop', subarray is not supported}}
  // parloop-error@+7 2 {{in 'firstprivate' clause on '#pragma acc parallel loop', subarray is not supported}}
  #pragma acc parallel LOOP firstprivate(a[0:1],    \
                                         a[0:],     \
                                         a[:1],     \
                                         a[:],      \
                                         a[0],      \
                                         m[0:1][0], \
                                         m[0][0:1])
    FORLOOP
  // expected-error@+21 {{subarray syntax must include ':'}}
  // expected-error@+21 {{subarray syntax must include ':'}}
  // expected-error@+21 {{subarray syntax must include ':'}}
  // par-error@+14 {{in 'private' clause on '#pragma acc parallel', subarray is not supported}}
  // par-error@+14 {{in 'private' clause on '#pragma acc parallel', subarray is not supported}}
  // par-error@+14 {{in 'private' clause on '#pragma acc parallel', subarray is not supported}}
  // par-error@+14 {{in 'private' clause on '#pragma acc parallel', subarray is not supported}}
  // par-error@+14 {{in 'private' clause on '#pragma acc parallel', subarray is not supported}}
  // par-error@+14 2 {{in 'private' clause on '#pragma acc parallel', subarray is not supported}}
  // par-error@+14 2 {{in 'private' clause on '#pragma acc parallel', subarray is not supported}}
  // parloop-error@+7 {{in 'private' clause on '#pragma acc parallel loop', subarray is not supported}}
  // parloop-error@+7 {{in 'private' clause on '#pragma acc parallel loop', subarray is not supported}}
  // parloop-error@+7 {{in 'private' clause on '#pragma acc parallel loop', subarray is not supported}}
  // parloop-error@+7 {{in 'private' clause on '#pragma acc parallel loop', subarray is not supported}}
  // parloop-error@+7 {{in 'private' clause on '#pragma acc parallel loop', subarray is not supported}}
  // parloop-error@+7 2 {{in 'private' clause on '#pragma acc parallel loop', subarray is not supported}}
  // parloop-error@+7 2 {{in 'private' clause on '#pragma acc parallel loop', subarray is not supported}}
  #pragma acc parallel LOOP private(a[3:5],    \
                                    a[0:],     \
                                    a[:1],     \
                                    a[:],      \
                                    a[10],     \
                                    m[0:1][0], \
                                    m[0][0:1])
    FORLOOP
  // expected-error@+21 {{subarray syntax must include ':'}}
  // expected-error@+21 {{subarray syntax must include ':'}}
  // expected-error@+21 {{subarray syntax must include ':'}}
  // par-error@+14 {{in 'reduction' clause on '#pragma acc parallel', subarray is not supported}}
  // par-error@+14 {{in 'reduction' clause on '#pragma acc parallel', subarray is not supported}}
  // par-error@+14 {{in 'reduction' clause on '#pragma acc parallel', subarray is not supported}}
  // par-error@+14 {{in 'reduction' clause on '#pragma acc parallel', subarray is not supported}}
  // par-error@+14 {{in 'reduction' clause on '#pragma acc parallel', subarray is not supported}}
  // par-error@+14 2 {{in 'reduction' clause on '#pragma acc parallel', subarray is not supported}}
  // par-error@+14 2 {{in 'reduction' clause on '#pragma acc parallel', subarray is not supported}}
  // parloop-error@+7 {{in 'reduction' clause on '#pragma acc parallel loop', subarray is not supported}}
  // parloop-error@+7 {{in 'reduction' clause on '#pragma acc parallel loop', subarray is not supported}}
  // parloop-error@+7 {{in 'reduction' clause on '#pragma acc parallel loop', subarray is not supported}}
  // parloop-error@+7 {{in 'reduction' clause on '#pragma acc parallel loop', subarray is not supported}}
  // parloop-error@+7 {{in 'reduction' clause on '#pragma acc parallel loop', subarray is not supported}}
  // parloop-error@+7 2 {{in 'reduction' clause on '#pragma acc parallel loop', subarray is not supported}}
  // parloop-error@+7 2 {{in 'reduction' clause on '#pragma acc parallel loop', subarray is not supported}}
  #pragma acc parallel LOOP reduction(min:a[0:10],   \
                                          a[0:],     \
                                          a[:1],     \
                                          a[:],      \
                                          a[23],     \
                                          m[0:1][0], \
                                          m[0][0:1])
    FORLOOP

  // Member expression plus subarray not permitted.

  // expected-error@+17 {{OpenACC subarray is not allowed here}}
  // expected-error@+17 {{OpenACC subarray is not allowed here}}
  // expected-error@+17 {{OpenACC subarray is not allowed here}}
  // expected-error@+17 {{OpenACC subarray is not allowed here}}
  // expected-error@+17 {{OpenACC subarray is not allowed here}}
  // expected-error@+17 {{OpenACC subarray is not allowed here}}
  // expected-error@+17 {{OpenACC subarray is not allowed here}}
  // expected-error@+17 {{OpenACC subarray is not allowed here}}
  // expected-error@+17 {{OpenACC subarray is not allowed here}}
  // expected-error@+17 {{OpenACC subarray is not allowed here}}
  // expected-error@+17 {{OpenACC subarray is not allowed here}}
  // expected-error@+17 {{OpenACC subarray is not allowed here}}
  // expected-error@+17 {{OpenACC subarray is not allowed here}}
  // expected-error@+17 {{OpenACC subarray is not allowed here}}
  // expected-error@+17 {{OpenACC subarray is not allowed here}}
  // expected-error@+17 {{OpenACC subarray is not allowed here}}
  // expected-error@+17 {{OpenACC subarray is not allowed here}}
  #pragma acc parallel LOOP present(ps[0:1].i)             \
                            copy(ps[1:2].i)                \
                            pcopy(ps[2:3].i)               \
                            present_or_copy(ps[3:5].i)     \
                            copyin(ps[4:6].i)              \
                            pcopyin(ps[5:7].i)             \
                            present_or_copyin(ps[6:8].i)   \
                            copyout(ps[7:9].i)             \
                            pcopyout(ps[8:10].i)           \
                            present_or_copyout(ps[9:11].i) \
                            create(ps[10:12].i)            \
                            pcreate(ps[11:13].i)           \
                            present_or_create(ps[12:14].i) \
                            no_create(ps[13:15].i)          \
                            firstprivate(ps[0:3].i)        \
                            private(ps[1:2].i)             \
                            reduction(+:ps[2:1].i)
    FORLOOP
  // par-error@+12 {{in 'firstprivate' clause on '#pragma acc parallel', subarray is not supported}}
  // par-error@+12 {{in 'private' clause on '#pragma acc parallel', subarray is not supported}}
  // par-error@+12 {{in 'reduction' clause on '#pragma acc parallel', subarray is not supported}}
  // parloop-error@+9 {{in 'firstprivate' clause on '#pragma acc parallel loop', subarray is not supported}}
  // parloop-error@+9 {{in 'private' clause on '#pragma acc parallel loop', subarray is not supported}}
  // parloop-error@+9 {{in 'reduction' clause on '#pragma acc parallel loop', subarray is not supported}}
  // par-error-re@+6 {{in 'firstprivate' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // par-error-re@+6 {{in 'private' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // par-error-re@+6 {{in 'reduction' clause on '#pragma acc parallel', member expression is not supported{{$}}}}
  // parloop-error-re@+3 {{in 'firstprivate' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  // parloop-error-re@+3 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  // parloop-error-re@+3 {{in 'reduction' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
  #pragma acc parallel LOOP firstprivate(ss.ps[3:1]) \
                            private(ss.ps[2:2])      \
                            reduction(+:ss.ps[1:3])
    FORLOOP

  // Variables of incomplete type.

  // expected-error@+18 {{variable in 'present' clause cannot have incomplete type 'int[]'}}
  // expected-error@+18 {{variable in 'copy' clause cannot have incomplete type 'int[]'}}
  // expected-error@+17 {{variable in 'pcopy' clause cannot have incomplete type 'int[]'}}
  // expected-error@+16 {{variable in 'present_or_copy' clause cannot have incomplete type 'int[]'}}
  // expected-error@+16 {{variable in 'copyin' clause cannot have incomplete type 'int[]'}}
  // expected-error@+15 {{variable in 'pcopyin' clause cannot have incomplete type 'int[]'}}
  // expected-error@+14 {{variable in 'present_or_copyin' clause cannot have incomplete type 'int[]'}}
  // expected-error@+14 {{variable in 'copyout' clause cannot have incomplete type 'int[]'}}
  // expected-error@+13 {{variable in 'pcopyout' clause cannot have incomplete type 'int[]'}}
  // expected-error@+12 {{variable in 'present_or_copyout' clause cannot have incomplete type 'int[]'}}
  // expected-error@+12 {{variable in 'create' clause cannot have incomplete type 'int[]'}}
  // expected-error@+11 {{variable in 'pcreate' clause cannot have incomplete type 'int[]'}}
  // expected-error@+10 {{variable in 'present_or_create' clause cannot have incomplete type 'int[]'}}
  // expected-error@+10 {{variable in 'no_create' clause cannot have incomplete type 'int[]'}}
  // expected-error@+10 {{variable in 'firstprivate' clause cannot have incomplete type 'int[]'}}
  // expected-error@+9  {{variable in 'private' clause cannot have incomplete type 'int[]'}}
  // expected-error@+8  {{variable in 'reduction' clause cannot have incomplete type 'int[]'}}
  #pragma acc parallel LOOP \
      present(incomplete) \
      copy(incomplete) pcopy(incomplete) present_or_copy(incomplete) \
      copyin(incomplete) pcopyin(incomplete) present_or_copyin(incomplete) \
      copyout(incomplete) pcopyout(incomplete) present_or_copyout(incomplete) \
      create(incomplete) pcreate(incomplete) present_or_create(incomplete) \
      no_create(incomplete) \
      firstprivate(incomplete) private(incomplete) reduction(&:incomplete)
    FORLOOP
  // expected-error@+4 {{variable in implied 'copy' clause cannot have incomplete type 'int[]'}}
  // expected-note@+1 {{'copy' clause implied here}}
  #pragma acc parallel LOOP
    FORLOOP_HEAD {
      i = *incomplete;
    }
  // expected-error@+3 {{variable in 'copy' clause cannot have incomplete type 'int[]'}}
  // expected-error@+4 {{variable in implied 'copy' clause cannot have incomplete type 'int[]'}}
  // expected-note@+1 {{'copy' clause implied here}}
  #pragma acc parallel LOOP copy(incomplete)
    FORLOOP_HEAD {
      i = *incomplete;
    }

  // Variables of const type.

  #pragma acc parallel LOOP present(constI, constIDecl)
    FORLOOP
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
  // expected-error@+1 2 {{const variable cannot be written by 'copyout' clause}}
  #pragma acc parallel LOOP copyout(constI, constIDecl)
    FORLOOP
  // expected-error@+1 2 {{const variable cannot be written by 'pcopyout' clause}}
  #pragma acc parallel LOOP pcopyout(constI, constIDecl)
    FORLOOP
  // expected-error@+1 2 {{const variable cannot be written by 'present_or_copyout' clause}}
  #pragma acc parallel LOOP present_or_copyout(constI, constIDecl)
    FORLOOP
  // expected-error@+1 2 {{const variable cannot be initialized after 'create' clause}}
  #pragma acc parallel LOOP create(constI, constIDecl)
    FORLOOP
  // expected-error@+1 2 {{const variable cannot be initialized after 'pcreate' clause}}
  #pragma acc parallel LOOP pcreate(constI, constIDecl)
    FORLOOP
  // expected-error@+1 2 {{const variable cannot be initialized after 'present_or_create' clause}}
  #pragma acc parallel LOOP present_or_create(constI, constIDecl)
    FORLOOP
  #pragma acc parallel LOOP no_create(constI, constIDecl)
    FORLOOP
  #pragma acc parallel LOOP firstprivate(constI, constIDecl)
    FORLOOP
  // expected-error@+2 2 {{const variable cannot be initialized after 'private' clause}}
  // expected-error@+2 2 {{const variable cannot be written by 'reduction' clause}}
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
  #pragma acc parallel LOOP present(constIDecl)
    FORLOOP_HEAD {
      // expected-noacc-error@+2 {{cannot assign to variable 'constI' with const-qualified type 'const int'}}
      // expected-noacc-error@+2 {{cannot assign to variable 'constIDecl' with const-qualified type 'const int'}}
      constI = 5;
      constIDecl = 5;
    }
  #pragma acc parallel LOOP copy(constADecl)
    FORLOOP_HEAD {
      // expected-noacc-error@+2 {{cannot assign to variable 'constA' with const-qualified type 'const int[3]'}}
      // expected-noacc-error@+2 {{cannot assign to variable 'constADecl' with const-qualified type 'const int[3]'}}
      constA[0] = 5;
      constADecl[1] = 5;
    }
  #pragma acc parallel LOOP pcopy(constA)
    FORLOOP_HEAD {
      // expected-noacc-error@+2 {{cannot assign to variable 'constA' with const-qualified type 'const int[3]'}}
      // expected-noacc-error@+2 {{cannot assign to variable 'constADecl' with const-qualified type 'const int[3]'}}
      constA[0] = 5;
      constADecl[1] = 5;
    }
  #pragma acc parallel LOOP present_or_copy(constADecl)
    FORLOOP_HEAD {
      // expected-noacc-error@+2 {{cannot assign to variable 'constADecl' with const-qualified type 'const int[3]'}}
      // expected-noacc-error@+2 {{cannot assign to variable 'constA' with const-qualified type 'const int[3]'}}
      constADecl[1] = 5;
      constA[0] = 5;
    }
  #pragma acc parallel LOOP copyin(constA) pcopyin(constADecl)
    FORLOOP_HEAD {
      // expected-noacc-error@+2 {{cannot assign to variable 'constA' with const-qualified type 'const int[3]'}}
      // expected-noacc-error@+2 {{cannot assign to variable 'constADecl' with const-qualified type 'const int[3]'}}
      constA[0] = 5;
      constADecl[1] = 5;
    }
  #pragma acc parallel LOOP present_or_copyin(constA)
    FORLOOP_HEAD {
      // expected-noacc-error@+1 {{cannot assign to variable 'constA' with const-qualified type 'const int[3]'}}
      constA[0] = 5;
    }
  #pragma acc parallel LOOP no_create(constI)
    FORLOOP_HEAD {
      // expected-noacc-error@+2 {{cannot assign to variable 'constI' with const-qualified type 'const int'}}
      // expected-noacc-error@+2 {{cannot assign to variable 'constIDecl' with const-qualified type 'const int'}}
      constI = 5;
      constIDecl = 5;
    }

  // Reduction operator doesn't permit variable type.

  #pragma acc parallel LOOP reduction(max:b,e,i,jk,f,d,p)
    FORLOOP
  // expected-error@+1 6 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
  #pragma acc parallel LOOP reduction(max:fc,dc,a,s,u,uDecl)
    FORLOOP
  #pragma acc parallel LOOP reduction(min:b,e,i,jk,f,d,p)
    FORLOOP
  // expected-error@+1 6 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
  #pragma acc parallel LOOP reduction(min:fc,dc,a,s,u,uDecl)
    FORLOOP
  #pragma acc parallel LOOP reduction(+:b,e,i,jk,f,d,fc,dc)
    FORLOOP
  // expected-error@+1 5 {{OpenACC reduction operator '+' argument must be of arithmetic type}}
  #pragma acc parallel LOOP reduction(+:p,a,s,u,uDecl)
    FORLOOP
  #pragma acc parallel LOOP reduction(*:b,e,i,jk,f,d,fc,dc)
    FORLOOP
  // expected-error@+1 5 {{OpenACC reduction operator '*' argument must be of arithmetic type}}
  #pragma acc parallel LOOP reduction(*:p,a,s,u,uDecl)
    FORLOOP
  #pragma acc parallel LOOP reduction(&&:b,e,i,jk,f,d,fc,dc)
    FORLOOP
  // expected-error@+1 5 {{OpenACC reduction operator '&&' argument must be of arithmetic type}}
  #pragma acc parallel LOOP reduction(&&:p,a,s,u,uDecl)
    FORLOOP
  #pragma acc parallel LOOP reduction(||:b,e,i,jk,f,d,fc,dc)
    FORLOOP
  // expected-error@+1 5 {{OpenACC reduction operator '||' argument must be of arithmetic type}}
  #pragma acc parallel LOOP reduction(||:p,a,s,u,uDecl)
    FORLOOP
  #pragma acc parallel LOOP reduction(&:b,e,i,jk)
    FORLOOP
  // expected-error@+1 9 {{OpenACC reduction operator '&' argument must be of integer type}}
  #pragma acc parallel LOOP reduction(&:f,d,fc,dc,p,a,s,u,uDecl)
    FORLOOP
  #pragma acc parallel LOOP reduction(|:b,e,i,jk)
    FORLOOP
  // expected-error@+1 9 {{OpenACC reduction operator '|' argument must be of integer type}}
  #pragma acc parallel LOOP reduction(|:f,d,fc,dc,p,a,s,u,uDecl)
    FORLOOP
  #pragma acc parallel LOOP reduction(^:b,e,i,jk)
    FORLOOP
  // expected-error@+1 9 {{OpenACC reduction operator '^' argument must be of integer type}}
  #pragma acc parallel LOOP reduction(^:f,d,fc,dc,p,a,s,u,uDecl)
    FORLOOP

  // Redundant clauses.

  // expected-error@+4 {{present variable defined again as present variable}}
  // expected-note@+3 {{previously defined as present variable here}}
  // expected-error@+3 {{present variable defined again as present variable}}
  // expected-note@+1 {{previously defined as present variable here}}
  #pragma acc parallel LOOP present(e,i,e,jk) \
                            present(i)
    FORLOOP
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
  // expected-error@+6 {{create variable defined again as create variable}}
  // expected-note@+5 {{previously defined as create variable here}}
  // expected-error@+5 {{create variable defined again as create variable}}
  // expected-note@+3 {{previously defined as create variable here}}
  // expected-error@+4 {{create variable defined again as create variable}}
  // expected-note@+1 {{previously defined as create variable here}}
  #pragma acc parallel LOOP create(e,e,i,jk) \
                            create(i)        \
                            pcreate(jk)
    FORLOOP
  // expected-error@+6 {{create variable defined again as create variable}}
  // expected-note@+5 {{previously defined as create variable here}}
  // expected-error@+5 {{create variable defined again as create variable}}
  // expected-note@+3 {{previously defined as create variable here}}
  // expected-error@+4 {{create variable defined again as create variable}}
  // expected-note@+1 {{previously defined as create variable here}}
  #pragma acc parallel LOOP pcreate(e,e,i,jk) \
                            pcreate(i)        \
                            present_or_create(jk)
    FORLOOP
  // expected-error@+6 {{create variable defined again as create variable}}
  // expected-note@+5 {{previously defined as create variable here}}
  // expected-error@+5 {{create variable defined again as create variable}}
  // expected-note@+3 {{previously defined as create variable here}}
  // expected-error@+4 {{create variable defined again as create variable}}
  // expected-note@+1 {{previously defined as create variable here}}
  #pragma acc parallel LOOP create(e,e,i,jk)      \
                            present_or_create(i)  \
                            present_or_create(jk)
    FORLOOP
  // expected-error@+6 {{create variable defined again as create variable}}
  // expected-note@+5 {{previously defined as create variable here}}
  // expected-error@+5 {{create variable defined again as create variable}}
  // expected-note@+3 {{previously defined as create variable here}}
  // expected-error@+4 {{create variable defined again as create variable}}
  // expected-note@+1 {{previously defined as create variable here}}
  #pragma acc parallel LOOP present_or_create(e,e,i,jk) \
                            pcreate(i)                  \
                            create(jk)
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
  // expected-error@+2 {{create variable defined again as create variable}}
  // expected-note@+1 {{previously defined as create variable here}}
  #pragma acc parallel LOOP create(a[1:1]) pcreate(a[0:1])
    FORLOOP
  // expected-error@+4 {{no_create variable defined again as no_create variable}}
  // expected-note@+3 {{previously defined as no_create variable here}}
  // expected-error@+3 {{no_create variable defined again as no_create variable}}
  // expected-note@+1 {{previously defined as no_create variable here}}
  #pragma acc parallel LOOP no_create(e,i,e,jk) \
                            no_create(i)
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

  // Conflicting clauses: present vs. other clauses.

  // expected-error@+5 2 {{present variable cannot be copy variable}}
  // expected-note@+3 2 {{previously defined as present variable here}}
  // expected-error@+4 {{copy variable cannot be present variable}}
  // expected-note@+2 {{previously defined as copy variable here}}
  #pragma acc parallel LOOP present(i,a[0:1]) \
                            copy(i,jk,a[0:1]) \
                            present(jk)
    FORLOOP
  // expected-error@+5 {{copy variable cannot be present variable}}
  // expected-note@+3 {{previously defined as copy variable here}}
  // expected-error@+4 {{present variable cannot be copy variable}}
  // expected-note@+2 {{previously defined as present variable here}}
  #pragma acc parallel LOOP pcopy(a) \
                            present(a[0:2],p) \
                            present_or_copy(p)
    FORLOOP
  // expected-error@+5 2 {{present variable cannot be copyin variable}}
  // expected-note@+3 2 {{previously defined as present variable here}}
  // expected-error@+4 {{copyin variable cannot be present variable}}
  // expected-note@+2 {{previously defined as copyin variable here}}
  #pragma acc parallel LOOP present(i,a[0:1]) \
                            copyin(i,jk,a[0:1]) \
                            present(jk)
    FORLOOP
  // expected-error@+5 {{copyin variable cannot be present variable}}
  // expected-note@+3 {{previously defined as copyin variable here}}
  // expected-error@+4 {{present variable cannot be copyin variable}}
  // expected-note@+2 {{previously defined as present variable here}}
  #pragma acc parallel LOOP pcopyin(a) \
                            present(a[0:2],p) \
                            present_or_copyin(p)
    FORLOOP
  // expected-error@+5 {{copyout variable cannot be present variable}}
  // expected-note@+3 {{previously defined as copyout variable here}}
  // expected-error@+4 {{present variable cannot be copyout variable}}
  // expected-note@+2 {{previously defined as present variable here}}
  #pragma acc parallel LOOP present_or_copyout(a[0:1]) \
                            present(a,p) \
                            pcopyout(p)
    FORLOOP
  // expected-error@+5 {{present variable cannot be copyout variable}}
  // expected-note@+3 {{previously defined as present variable here}}
  // expected-error@+4 2 {{copyout variable cannot be present variable}}
  // expected-note@+2 2 {{previously defined as copyout variable here}}
  #pragma acc parallel LOOP present(f) \
                            copyout(f,d,a[0:1]) \
                            present(d,a[1:1])
    FORLOOP
  // expected-error@+5 {{create variable cannot be present variable}}
  // expected-note@+3 {{previously defined as create variable here}}
  // expected-error@+4 {{present variable cannot be create variable}}
  // expected-note@+2 {{previously defined as present variable here}}
  #pragma acc parallel LOOP present_or_create(a[0:1]) \
                            present(a,p) \
                            pcreate(p)
    FORLOOP
  // expected-error@+5 {{present variable cannot be create variable}}
  // expected-note@+3 {{previously defined as present variable here}}
  // expected-error@+4 2 {{create variable cannot be present variable}}
  // expected-note@+2 2 {{previously defined as create variable here}}
  #pragma acc parallel LOOP present(f) \
                            create(f,d,a[0:1]) \
                            present(d,a[1:1])
    FORLOOP
  // expected-error@+5 {{present variable cannot be no_create variable}}
  // expected-note@+3 {{previously defined as present variable here}}
  // expected-error@+4 2 {{no_create variable cannot be present variable}}
  // expected-note@+2 2 {{previously defined as no_create variable here}}
  #pragma acc parallel LOOP present(f) \
                            no_create(f,d,a[0:1]) \
                            present(d,a[1:1])
    FORLOOP
  // expected-error@+5 {{no_create variable cannot be present variable}}
  // expected-note@+3 {{previously defined as no_create variable here}}
  // expected-error@+4 {{present variable cannot be no_create variable}}
  // expected-note@+2 {{previously defined as present variable here}}
  #pragma acc parallel LOOP no_create(a) \
                            present(a[0:1],p) \
                            no_create(p)
    FORLOOP
  // expected-error@+5 2 {{present variable cannot be firstprivate variable}}
  // expected-note@+3 2 {{previously defined as present variable here}}
  // expected-error@+4 {{firstprivate variable cannot be present variable}}
  // expected-note@+2 {{previously defined as firstprivate variable here}}
  #pragma acc parallel LOOP present(i,a[0:2]) \
                            firstprivate(i,jk,a) \
                            present(jk)
    FORLOOP
  // expected-error@+5 {{firstprivate variable cannot be present variable}}
  // expected-note@+3 {{previously defined as firstprivate variable here}}
  // expected-error@+4 {{present variable cannot be firstprivate variable}}
  // expected-note@+2 {{previously defined as present variable here}}
  #pragma acc parallel LOOP firstprivate(a) \
                            present(a[0:1],p) \
                            firstprivate(p)
    FORLOOP
  // expected-error@+5 {{private variable cannot be present variable}}
  // expected-note@+3 {{previously defined as private variable here}}
  // expected-error@+4 {{present variable cannot be private variable}}
  // expected-note@+2 {{previously defined as present variable here}}
  #pragma acc parallel LOOP private(a) \
                            present(a[1:1],p) \
                            private(p)
    FORLOOP
  // expected-error@+5 {{present variable cannot be private variable}}
  // expected-note@+3 {{previously defined as present variable here}}
  // expected-error@+4 {{private variable cannot be present variable}}
  // expected-note@+2 {{previously defined as private variable here}}
  #pragma acc parallel LOOP present(f) \
                            private(f,d) \
                            present(d)
    FORLOOP
  #pragma acc parallel LOOP present(f) reduction(+:f,d) present(d)
    FORLOOP
  #pragma acc parallel LOOP reduction(*:i) present(i,jk) reduction(*:jk)
    FORLOOP

  // Conflicting clauses: copy and aliases vs. other clauses (except present,
  // which is tested above).

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
  // expected-error@+5 {{create variable cannot be copy variable}}
  // expected-note@+3 {{previously defined as create variable here}}
  // expected-error@+4 {{copy variable cannot be create variable}}
  // expected-note@+2 {{previously defined as copy variable here}}
  #pragma acc parallel LOOP present_or_create(a[0:1]) \
                            pcopy(a,p) \
                            pcreate(p)
    FORLOOP
  // expected-error@+5 {{copy variable cannot be create variable}}
  // expected-note@+3 {{previously defined as copy variable here}}
  // expected-error@+4 2 {{create variable cannot be copy variable}}
  // expected-note@+2 2 {{previously defined as create variable here}}
  #pragma acc parallel LOOP present_or_copy(f) \
                            create(f,d,a[0:1]) \
                            copy(d,a[1:1])
    FORLOOP
  // expected-error@+5 {{no_create variable cannot be copy variable}}
  // expected-note@+3 {{previously defined as no_create variable here}}
  // expected-error@+4 {{copy variable cannot be no_create variable}}
  // expected-note@+2 {{previously defined as copy variable here}}
  #pragma acc parallel LOOP no_create(a[0:1]) \
                            copy(a,p) \
                            no_create(p)
    FORLOOP
  // expected-error@+5 {{copy variable cannot be no_create variable}}
  // expected-note@+3 {{previously defined as copy variable here}}
  // expected-error@+4 2 {{no_create variable cannot be copy variable}}
  // expected-note@+2 2 {{previously defined as no_create variable here}}
  #pragma acc parallel LOOP pcopy(f) \
                            no_create(f,d,a[0:1]) \
                            present_or_copy(d,a[1:1])
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

  // Conflicting clauses: copyin and aliases vs. other clauses (except present
  // and copy, which are tested above).

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
  // expected-error@+5 {{create variable cannot be copyin variable}}
  // expected-note@+3 {{previously defined as create variable here}}
  // expected-error@+4 {{copyin variable cannot be create variable}}
  // expected-note@+2 {{previously defined as copyin variable here}}
  #pragma acc parallel LOOP pcreate(a) \
                            present_or_copyin(a,p) \
                            create(p)
    FORLOOP
  // expected-error@+5 {{copyin variable cannot be create variable}}
  // expected-note@+3 {{previously defined as copyin variable here}}
  // expected-error@+4 {{create variable cannot be copyin variable}}
  // expected-note@+2 {{previously defined as create variable here}}
  #pragma acc parallel LOOP pcopyin(f) \
                            present_or_create(f,d) \
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
  // expected-error@+5 {{no_create variable cannot be copyin variable}}
  // expected-note@+3 {{previously defined as no_create variable here}}
  // expected-error@+4 {{copyin variable cannot be no_create variable}}
  // expected-note@+2 {{previously defined as copyin variable here}}
  #pragma acc parallel LOOP no_create(a) \
                            present_or_copyin(a,p) \
                            no_create(p)
    FORLOOP
  // expected-error@+5 {{copyin variable cannot be no_create variable}}
  // expected-note@+3 {{previously defined as copyin variable here}}
  // expected-error@+4 {{no_create variable cannot be copyin variable}}
  // expected-note@+2 {{previously defined as no_create variable here}}
  #pragma acc parallel LOOP pcopyin(f) \
                            no_create(f,d) \
                            copyin(d)
    FORLOOP
  #pragma acc parallel LOOP pcopyin(f) reduction(+:f,d) present_or_copyin(d)
    FORLOOP
  #pragma acc parallel LOOP reduction(*:i) pcopyin(i,jk) reduction(*:jk)
    FORLOOP

  // Conflicting clauses: copyout and aliases vs. other clauses (except
  // present, copy, and copyin, which are tested above).

  // expected-error@+5 {{create variable cannot be copyout variable}}
  // expected-note@+3 {{previously defined as create variable here}}
  // expected-error@+4 {{copyout variable cannot be create variable}}
  // expected-note@+2 {{previously defined as copyout variable here}}
  #pragma acc parallel LOOP create(a) \
                            present_or_copyout(a,p) \
                            create(p)
    FORLOOP
  // expected-error@+5 {{copyout variable cannot be create variable}}
  // expected-note@+3 {{previously defined as copyout variable here}}
  // expected-error@+4 {{create variable cannot be copyout variable}}
  // expected-note@+2 {{previously defined as create variable here}}
  #pragma acc parallel LOOP pcopyout(f) \
                            create(f,d) \
                            copyout(d)
    FORLOOP
  // expected-error@+5 {{no_create variable cannot be copyout variable}}
  // expected-note@+3 {{previously defined as no_create variable here}}
  // expected-error@+4 {{copyout variable cannot be no_create variable}}
  // expected-note@+2 {{previously defined as copyout variable here}}
  #pragma acc parallel LOOP no_create(a) \
                            present_or_copyout(a,p) \
                            no_create(p)
    FORLOOP
  // expected-error@+5 {{copyout variable cannot be no_create variable}}
  // expected-note@+3 {{previously defined as copyout variable here}}
  // expected-error@+4 {{no_create variable cannot be copyout variable}}
  // expected-note@+2 {{previously defined as no_create variable here}}
  #pragma acc parallel LOOP pcopyout(f) \
                            no_create(f,d) \
                            copyout(d)
    FORLOOP
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

  // Conflicting clauses: create and aliases vs. other clauses (except
  // present, copy, copyin, and copyout which are tested above).

  // expected-error@+5 {{no_create variable cannot be create variable}}
  // expected-note@+3 {{previously defined as no_create variable here}}
  // expected-error@+4 {{create variable cannot be no_create variable}}
  // expected-note@+2 {{previously defined as create variable here}}
  #pragma acc parallel LOOP no_create(a) \
                            present_or_create(a,p) \
                            no_create(p)
    FORLOOP
  // expected-error@+5 {{create variable cannot be no_create variable}}
  // expected-note@+3 {{previously defined as create variable here}}
  // expected-error@+4 {{no_create variable cannot be create variable}}
  // expected-note@+2 {{previously defined as no_create variable here}}
  #pragma acc parallel LOOP pcreate(f) \
                            no_create(f,d) \
                            create(d)
    FORLOOP
  // expected-error@+5 {{create variable cannot be firstprivate variable}}
  // expected-note@+3 {{previously defined as create variable here}}
  // expected-error@+4 {{firstprivate variable cannot be create variable}}
  // expected-note@+2 {{previously defined as firstprivate variable here}}
  #pragma acc parallel LOOP create(i) \
                            firstprivate(i,jk) \
                            pcreate(jk)
    FORLOOP
  // expected-error@+5 {{firstprivate variable cannot be create variable}}
  // expected-note@+3 {{previously defined as firstprivate variable here}}
  // expected-error@+4 {{create variable cannot be firstprivate variable}}
  // expected-note@+2 {{previously defined as create variable here}}
  #pragma acc parallel LOOP firstprivate(a) \
                            present_or_create(a,p) \
                            firstprivate(p)
    FORLOOP
  // expected-error@+5 {{private variable cannot be create variable}}
  // expected-note@+3 {{previously defined as private variable here}}
  // expected-error@+4 {{create variable cannot be private variable}}
  // expected-note@+2 {{previously defined as create variable here}}
  #pragma acc parallel LOOP private(a) \
                            present_or_create(a,p) \
                            private(p)
    FORLOOP
  // expected-error@+5 {{create variable cannot be private variable}}
  // expected-note@+3 {{previously defined as create variable here}}
  // expected-error@+4 {{private variable cannot be create variable}}
  // expected-note@+2 {{previously defined as private variable here}}
  #pragma acc parallel LOOP pcreate(f) \
                            private(f,d) \
                            create(d)
    FORLOOP
  #pragma acc parallel LOOP present_or_create(f) reduction(+:f,d) pcreate(d)
    FORLOOP
  #pragma acc parallel LOOP reduction(*:i) create(i,jk) reduction(*:jk)
    FORLOOP

  // Conflicting clauses: no_create vs. other clauses (except present, copy,
  // copyin, copyout, and create, which are tested above).

  // expected-error@+5 {{no_create variable cannot be firstprivate variable}}
  // expected-note@+3 {{previously defined as no_create variable here}}
  // expected-error@+4 {{firstprivate variable cannot be no_create variable}}
  // expected-note@+2 {{previously defined as firstprivate variable here}}
  #pragma acc parallel LOOP no_create(i) \
                            firstprivate(i,jk) \
                            no_create(jk)
    FORLOOP
  // expected-error@+5 {{firstprivate variable cannot be no_create variable}}
  // expected-note@+3 {{previously defined as firstprivate variable here}}
  // expected-error@+4 {{no_create variable cannot be firstprivate variable}}
  // expected-note@+2 {{previously defined as no_create variable here}}
  #pragma acc parallel LOOP firstprivate(a) \
                            no_create(a,p) \
                            firstprivate(p)
    FORLOOP
  // expected-error@+5 {{private variable cannot be no_create variable}}
  // expected-note@+3 {{previously defined as private variable here}}
  // expected-error@+4 {{no_create variable cannot be private variable}}
  // expected-note@+2 {{previously defined as no_create variable here}}
  #pragma acc parallel LOOP private(a) \
                            no_create(a,p) \
                            private(p)
    FORLOOP
  // expected-error@+5 {{no_create variable cannot be private variable}}
  // expected-note@+3 {{previously defined as no_create variable here}}
  // expected-error@+4 {{private variable cannot be no_create variable}}
  // expected-note@+2 {{previously defined as private variable here}}
  #pragma acc parallel LOOP no_create(f) \
                            private(f,d) \
                            no_create(d)
    FORLOOP
  #pragma acc parallel LOOP no_create(f) reduction(+:f,d) no_create(d)
    FORLOOP
  #pragma acc parallel LOOP reduction(*:i) no_create(i,jk) reduction(*:jk)
    FORLOOP

  // Conflicting clauses: firstprivate vs. private vs. reduction (present, copy,
  // copyin, copyout, create, and no_create are tested above).

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

  //----------------------------------------------------------------------------
  // Partition count clauses
  //----------------------------------------------------------------------------

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

  //----------------------------------------------------------------------------
  // async clause
  //----------------------------------------------------------------------------

  #pragma acc parallel LOOP async
    FORLOOP
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP async(1
    FORLOOP
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP async( i
    FORLOOP
  // expected-error@+2 {{expected ')'}}
  // expected-note@+1 {{to match this '('}}
  #pragma acc parallel LOOP async (3 + 5
    FORLOOP
  // par-error@+2 {{directive '#pragma acc parallel' cannot contain more than one 'async' clause}}
  // parloop-error@+1 {{directive '#pragma acc parallel loop' cannot contain more than one 'async' clause}}
  #pragma acc parallel LOOP async(i) async( i)
    FORLOOP
  // expected-error@+1 {{use of undeclared identifier 'bogus'}}
  #pragma acc parallel LOOP async(bogus)
    FORLOOP
  // expected-error@+1 {{argument to 'async' clause must be a non-negative integer, acc_async_sync, acc_async_noval, or acc_async_default}}
  #pragma acc parallel LOOP async(-10)
    FORLOOP
  // expected-error@+1 {{argument to 'async' clause must be a non-negative integer, acc_async_sync, acc_async_noval, or acc_async_default}}
  #pragma acc parallel LOOP async(-5)
    FORLOOP
  #pragma acc parallel LOOP async(0)
    FORLOOP
  #pragma acc parallel LOOP async(1)
    FORLOOP
  #pragma acc parallel LOOP async(15)
    FORLOOP
  #pragma acc parallel LOOP async(29)
    FORLOOP
  #pragma acc parallel LOOP async(3 * 10 - 1)
    FORLOOP
  // expected-error@+1 {{argument to 'async' clause must be a non-negative integer, acc_async_sync, acc_async_noval, or acc_async_default}}
  #pragma acc parallel LOOP async(10 - 20)
    FORLOOP
  #pragma acc parallel LOOP async(i*1)
    FORLOOP
  // expected-error@+1 {{expression must have integral or unscoped enumeration type, not 'float'}}
  #pragma acc parallel LOOP async(1.0f)
    FORLOOP
  // expected-error@+1 {{expression must have integral or unscoped enumeration type, not 'double'}}
  #pragma acc parallel LOOP async(d)
    FORLOOP
  // expected-error@+1 {{expression must have integral or unscoped enumeration type, not 'float'}}
  #pragma acc parallel LOOP async(f)
    FORLOOP
  // expected-error@+1 {{expression must have integral or unscoped enumeration type, not '_Complex float'}}
  #pragma acc parallel LOOP async(fc)
    FORLOOP
  // expected-error@+1 {{expression must have integral or unscoped enumeration type, not 'int[2]'}}
  #pragma acc parallel LOOP async(a)
    FORLOOP

  //----------------------------------------------------------------------------
  // Nesting
  //----------------------------------------------------------------------------

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

  //----------------------------------------------------------------------------
  // Miscellaneous
  //----------------------------------------------------------------------------

  // At one time, an assert failed due to the use of arrNoSize.
  int arrNoSize[]; // expected-noacc-error {{definition of variable with array type needs an explicit size or an initializer}}
  #pragma acc parallel LOOP // expected-note {{'copy' clause implied here}}
  #if ADD_LOOP_TO_PAR
  for (int i = 0; i < 2; ++i)
  #endif
    arrNoSize; // expected-error {{variable in implied 'copy' clause cannot have incomplete type 'int[]'}}

  return 0;
}

// Complete to suppress an additional warning, but it's too late for pragmas.
int incomplete[3];

//------------------------------------------------------------------------------
// The following diagnostics are produced by the transform from OpenACC to
// OpenMP, which is skipped if there have been any previous diagnostics.  Thus,
// each must be tested in a separate compilation.
//------------------------------------------------------------------------------

//..............................................................................
// acc_async_sync (-1) doesn't require acc2omp_async2dep, so these shouldn't
// trigger diagnostics that it's missing or defined incorrectly.
//..............................................................................

#elif ERR == ERR_ACC2OMP_ASYNC2DEP_FOR_SYNC

int main() {
  // expected-noacc-no-diagnostics
  #pragma acc parallel async(-1)
  ;
  int acc2omp_async2dep;
  // expected-noacc-no-diagnostics
  #pragma acc parallel async(-1)
  ;
  return 0;
}

//..............................................................................
// Asynchronous queues (like 0) do require acc2omp_async2dep, just once for the
// depends clause on the target teams directive.
//..............................................................................

#elif ERR == ERR_ACC2OMP_ASYNC2DEP_MISSING_FOR_ASYNC

int main() {
  // noacc-no-diagnostics
  // expected-error@+3 {{function 'acc2omp_async2dep' prototype is not in scope}}
  // expected-note@+2 {{required by 'async' clause here}}
  // expected-note@+1 {{try including openacc.h}}
  #pragma acc parallel async(0)
  ;
  return 0;
}

#elif ERR == ERR_ACC2OMP_ASYNC2DEP_NOT_FN_FOR_ASYNC

int main() {
  // expected-error@+1 {{'acc2omp_async2dep' is not a function of type 'char *(int)'}}
  int acc2omp_async2dep;
  // noacc-no-diagnostics
  // expected-note@+2 {{required by 'async' clause here}}
  // expected-note@+1 {{try including openacc.h}}
  #pragma acc parallel async(0)
  ;
  return 0;
}

#elif ERR == ERR_ACC2OMP_ASYNC2DEP_BAD_FN_FOR_ASYNC

int main() {
  // expected-error@+1 {{'acc2omp_async2dep' is not a function of type 'char *(int)'}}
  void acc2omp_async2dep(int);
  // noacc-no-diagnostics
  // expected-note@+2 {{required by 'async' clause here}}
  // expected-note@+1 {{try including openacc.h}}
  #pragma acc parallel async(0)
  ;
  return 0;
}

//..............................................................................
// The default queue might be synchronous or asynchronous, so acc2omp_async2dep
// is required for the depends clauses on both the target teams directive and on
// a following taskwait directive.  Check that we avoid duplicating the
// diagnostic across them.
//..............................................................................

#elif ERR == ERR_ACC2OMP_ASYNC2DEP_MISSING_FOR_DEF

int main() {
  // noacc-no-diagnostics
  // expected-error@+3 {{function 'acc2omp_async2dep' prototype is not in scope}}
  // expected-note@+2 {{required by 'async' clause here}}
  // expected-note@+1 {{try including openacc.h}}
  #pragma acc parallel async
  ;
  return 0;
}

#elif ERR == ERR_ACC2OMP_ASYNC2DEP_NOT_FN_FOR_DEF

// noacc-no-diagnostics
// expected-error@+1 {{'acc2omp_async2dep' is not a function of type 'char *(int)'}}
int acc2omp_async2dep;

int main() {
  // expected-note@+2 {{required by 'async' clause here}}
  // expected-note@+1 {{try including openacc.h}}
  #pragma acc parallel async
  ;
  return 0;
}

#elif ERR == ERR_ACC2OMP_ASYNC2DEP_BAD_FN_FOR_DEF

// noacc-no-diagnostics
// expected-error@+1 {{'acc2omp_async2dep' is not a function of type 'char *(int)'}}
char *acc2omp_async2dep();

int main() {
  // expected-note@+2 {{required by 'async' clause here}}
  // expected-note@+1 {{try including openacc.h}}
  #pragma acc parallel async
  ;
  return 0;
}

//------------------------------------------------------------------------------
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
//------------------------------------------------------------------------------

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
