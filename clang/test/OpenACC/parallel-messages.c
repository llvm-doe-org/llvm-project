// Check diagnostics for "acc parallel".
//
// When ADD_LOOP_TO_PAR is not set, this file checks diagnostics for "acc
// parallel" without "loop".
//
// When ADD_LOOP_TO_PAR is set, it adds "loop" and a for loop to those "acc
// parallel" directives in order to check the same diagnostics but for combined
// "acc parallel loop" directives.

// OpenACC disabled
// RUN: %clang_cc1 -verify=expected-noacc %s
// RUN: %clang_cc1 -verify=expected-noacc %s -DADD_LOOP_TO_PAR

// OpenACC enabled
// RUN: %clang_cc1 -verify=par,acc-discard,expected,expected-noacc -fopenacc %s
// RUN: %clang_cc1 -verify=parloop,acc-discard,expected,expected-noacc \
// RUN:            -DADD_LOOP_TO_PAR -fopenacc %s

// OpenACC enabled but optional warnings disabled
// RUN: %clang_cc1 -verify=par,expected,expected-noacc -fopenacc %s \
// RUN:            -Wno-openacc-discarded-clause
// RUN: %clang_cc1 -verify=parloop,expected,expected-noacc -fopenacc %s \
// RUN:            -DADD_LOOP_TO_PAR -Wno-openacc-discarded-clause

// END.

#if !ADD_LOOP_TO_PAR
# define LOOP
# define FORLOOP ;
#else
# define LOOP loop
# define FORLOOP for (int i = 0; i < 2; ++i) ;
#endif

int incomplete[];

int main() {
  _Bool b;
  enum { E1, E2 } e;
  int i, jk, a[2], *p; // expected-note 9 {{'a' defined here}}
                       // expected-note@-1 7 {{'p' defined here}}
  float f; // expected-note 3 {{'f' defined here}}
  double d; // expected-note 3 {{'d' defined here}}
  float _Complex fc; // expected-note 5 {{'fc' defined here}}
  double _Complex dc; // expected-note 5 {{'dc' defined here}}
  const int constI = 5; // expected-note {{variable 'constI' declared const here}}
                        // expected-note@-1 {{'constI' defined here}}
  const extern int constIDecl; // expected-note {{variable 'constIDecl' declared const here}}
                               // expected-note@-1 {{'constIDecl' declared here}}
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
  // Data sharing attribute clauses: syntax
  //--------------------------------------------------

  #pragma acc parallel LOOP firstprivate // expected-error {{expected '(' after 'firstprivate'}}
    FORLOOP
  #pragma acc parallel LOOP private // expected-error {{expected '(' after 'private'}}
    FORLOOP
  #pragma acc parallel LOOP reduction // expected-error {{expected '(' after 'reduction'}}
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

  #pragma acc parallel LOOP firstprivate() // expected-error {{expected expression}}
    FORLOOP
  #pragma acc parallel LOOP private( ) // expected-error {{expected expression}}
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

  #pragma acc parallel LOOP firstprivate(i jk) // expected-error {{expected ',' or ')' in 'firstprivate' clause}}
    FORLOOP
  #pragma acc parallel LOOP private(jk i) // expected-error {{expected ',' or ')' in 'private' clause}}
    FORLOOP
  #pragma acc parallel LOOP reduction(+:i jk) // expected-error {{expected ',' or ')' in 'reduction' clause}}
    FORLOOP

  #pragma acc parallel LOOP firstprivate(i,) // expected-error {{expected expression}}
    FORLOOP
  #pragma acc parallel LOOP private(jk, ) // expected-error {{expected expression}}
    FORLOOP
  #pragma acc parallel LOOP reduction(*:i ,) // expected-error {{expected expression}}
    FORLOOP

  //--------------------------------------------------
  // Data sharing attribute clauses: arg semantics
  //--------------------------------------------------

  #pragma acc parallel LOOP reduction(foo:i) // expected-error {{unknown reduction operator}}
    FORLOOP
  #pragma acc parallel LOOP reduction(foo:bar) // expected-error {{use of undeclared identifier 'bar'}}
    FORLOOP
  #pragma acc parallel LOOP reduction(foo:a[5]) // expected-error {{unknown reduction operator}}
    FORLOOP

  #pragma acc parallel LOOP firstprivate(foo) // expected-error {{use of undeclared identifier 'foo'}}
    FORLOOP
  #pragma acc parallel LOOP private( q) // expected-error {{use of undeclared identifier 'q'}}
    FORLOOP
  #pragma acc parallel LOOP reduction(max:bar) // expected-error {{use of undeclared identifier 'bar'}}
    FORLOOP

  // expected-error@+2 {{expected variable name}}
  // expected-error@+1 {{expected variable name}}
  #pragma acc parallel LOOP firstprivate(a[0], a[0:1])
    FORLOOP
  // expected-error@+2 {{expected variable name}}
  // expected-error@+1 {{expected variable name}}
  #pragma acc parallel LOOP private(a[3:5], a[10])
    FORLOOP
  // expected-error@+2 {{expected variable name}}
  // expected-error@+1 {{expected variable name}}
  #pragma acc parallel LOOP reduction(min:a[23],a[0:10])
    FORLOOP

  // expected-error@+3 {{firstprivate variable cannot have incomplete type 'int []'}}
  // expected-error@+2 {{private variable cannot have incomplete type 'int []'}}
  // expected-error@+1 {{reduction variable cannot have incomplete type 'int []'}}
  #pragma acc parallel LOOP firstprivate(incomplete) private(incomplete) reduction(&:incomplete)
    FORLOOP

  #pragma acc parallel LOOP firstprivate(constI, constIDecl)
    FORLOOP
  // expected-error@+4 {{const variable cannot be private because initialization is impossible}}
  // expected-error@+3 {{const variable cannot be private because initialization is impossible}}
  // expected-error@+2 {{reduction variable cannot be const}}
  // expected-error@+1 {{reduction variable cannot be const}}
  #pragma acc parallel LOOP private(constI, constIDecl) reduction(max: constI, constIDecl)
    FORLOOP

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

  // expected-error@+6 {{redundant '&&' reduction for variable 'i'}}
  // expected-note@+5 {{previous '&&' reduction here}}
  // expected-error@+4 {{redundant '&&' reduction for variable 'jk'}}
  // expected-note@+3 {{previous '&&' reduction here}}
  // expected-error@+2 {{conflicting 'max' reduction for variable 'd'}}
  // expected-note@+1 {{previous '&&' reduction here}}
  #pragma acc parallel LOOP reduction(&&:i,i,jk,d) reduction(&&:jk) reduction(max:d)
    FORLOOP
  // par-error@+4 {{private variable cannot be firstprivate}}
  // par-note@+3 {{defined as private}}
  // par-error@+2 {{firstprivate variable cannot be private}}
  // par-note@+1 {{defined as firstprivate}}
  #pragma acc parallel LOOP private(i) firstprivate(i,jk) private(jk)
    FORLOOP
  // expected-error@+4 {{private variable cannot be reduction}}
  // expected-note@+3 {{defined as private}}
  // expected-error@+2 {{firstprivate variable cannot be reduction}}
  // expected-note@+1 {{defined as firstprivate}}
  #pragma acc parallel LOOP private(i) reduction(min:i) firstprivate(d) reduction(+:d)
    FORLOOP
  // expected-error@+4 {{reduction variable cannot be private}}
  // expected-note@+3 {{defined as reduction}}
  // expected-error@+2 {{reduction variable cannot be firstprivate}}
  // expected-note@+1 {{defined as reduction}}
  #pragma acc parallel LOOP reduction(max:i) private(i) reduction(*:d) firstprivate(d)
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
  // acc-discard-warning@+1 {{'vector_length' discarded because argument is not an integer constant expression}}
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
  // acc-discard-warning@+1 {{'vector_length' discarded because argument is not an integer constant expression}}
  #pragma acc parallel LOOP num_gangs(d) num_workers(f) vector_length(fc)
    FORLOOP
  // expected-error@+4 {{expression must have integral or unscoped enumeration type, not 'float'}}
  // expected-error@+3 {{expression must have integral or unscoped enumeration type, not 'double'}}
  // expected-error@+2 {{expression must have integral or unscoped enumeration type, not 'long double'}}
  // acc-discard-warning@+1 {{'vector_length' discarded because argument is not an integer constant expression}}
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
