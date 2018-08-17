// OpenACC disabled
// RUN: %clang_cc1 -verify=expected-noacc %s

// OpenACC enabled
// RUN: %clang_cc1 -verify=expected,expected-noacc -fopenacc %s

// END.

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
#pragma acc parallel foo // expected-warning {{extra tokens at the end of '#pragma acc parallel' are ignored}}
  ;
#pragma acc parallel foo bar // expected-warning {{extra tokens at the end of '#pragma acc parallel' are ignored}}
  ;
#pragma acc parallel seq // expected-error {{unexpected OpenACC clause 'seq' in directive '#pragma acc parallel'}}
  ;
#pragma acc parallel shared // expected-error {{unexpected OpenACC clause 'shared' in directive '#pragma acc parallel'}}
                            // expected-error@-1 {{expected '(' after 'shared'}}
  ;
#pragma acc parallel shared( // expected-error {{unexpected OpenACC clause 'shared' in directive '#pragma acc parallel'}}
                             // expected-error@-1 {{expected expression}}
                             // expected-error@-2 {{expected ')'}}
                             // expected-note@-3 {{to match this '('}}
  ;
#pragma acc parallel shared(i // expected-error {{unexpected OpenACC clause 'shared' in directive '#pragma acc parallel'}}
                              // expected-error@-1 {{expected ')'}}
                              // expected-note@-2 {{to match this '('}}
  ;
#pragma acc parallel shared(i) // expected-error {{unexpected OpenACC clause 'shared' in directive '#pragma acc parallel'}}
  ;
#pragma acc parallel shared(i, jk) // expected-error {{unexpected OpenACC clause 'shared' in directive '#pragma acc parallel'}}
  ;
#pragma acc parallel
  ;

#pragma acc parallel firstprivate // expected-error {{expected '(' after 'firstprivate'}}
  ;
#pragma acc parallel private // expected-error {{expected '(' after 'private'}}
  ;
#pragma acc parallel reduction // expected-error {{expected '(' after 'reduction'}}
  ;

#pragma acc parallel firstprivate(
  // expected-error@-1 {{expected expression}}
  // expected-error@-2 {{expected ')'}}
  // expected-note@-3 {{to match this '('}}
  ;
#pragma acc parallel private(
  // expected-error@-1 {{expected expression}}
  // expected-error@-2 {{expected ')'}}
  // expected-note@-3 {{to match this '('}}
  ;
#pragma acc parallel reduction(
  // expected-error@-1 {{expected reduction operator}}
  // expected-warning@-2 {{missing ':' after reduction operator - ignoring}}
  // expected-error@-3 {{expected ')'}}
  // expected-note@-4 {{to match this '('}}
  ;

#pragma acc parallel firstprivate() // expected-error {{expected expression}}
  ;
#pragma acc parallel private( ) // expected-error {{expected expression}}
  ;
#pragma acc parallel reduction()
  // expected-error@-1 {{expected reduction operator}}
  // expected-warning@-2 {{missing ':' after reduction operator - ignoring}}
  ;

#pragma acc parallel reduction(:)
  // expected-error@-1 {{expected reduction operator}}
  ;
#pragma acc parallel reduction(:i)
  // expected-error@-1 {{expected reduction operator}}
  ;
#pragma acc parallel reduction(:foo)
  // expected-error@-1 {{expected reduction operator}}
  // expected-error@-2 {{use of undeclared identifier 'foo'}}
  ;

#pragma acc parallel reduction(-)
  // expected-error@-1 {{expected reduction operator}}
  // expected-warning@-2 {{missing ':' after reduction operator - ignoring}}
  ;
#pragma acc parallel reduction(i)
  // expected-warning@-1 {{missing ':' after reduction operator - ignoring}}
  // expected-error@-2 {{expected expression}}
  ;
#pragma acc parallel reduction(foo)
  // expected-warning@-1 {{missing ':' after reduction operator - ignoring}}
  // expected-error@-2 {{expected expression}}
  ;
#pragma acc parallel reduction(-:)
  // expected-error@-1 {{expected reduction operator}}
  ;
#pragma acc parallel reduction(foo:)
  // expected-error@-1 {{expected expression}}
  ;
#pragma acc parallel reduction(-:i)
  // expected-error@-1 {{expected reduction operator}}
  ;
// OpenACC 2.6 sec. 2.5.12 line 774 mistypes "^" as "%", which is nonsense as a
// reduction operator.
#pragma acc parallel reduction(% :i)
  // expected-error@-1 {{expected reduction operator}}
  ;
#pragma acc parallel reduction(foo:i)
  // expected-error@-1 {{unknown reduction operator}}
  ;
#pragma acc parallel reduction(foo:bar)
  // expected-error@-1 {{use of undeclared identifier 'bar'}}
  ;
#pragma acc parallel reduction(foo:a[5])
  // expected-error@-1 {{unknown reduction operator}}
  ;

#pragma acc parallel reduction(+)
  // expected-warning@-1 {{missing ':' after reduction operator - ignoring}}
  // expected-error@-2 {{expected expression}}
  ;
#pragma acc parallel reduction(max)
  // expected-warning@-1 {{missing ':' after reduction operator - ignoring}}
  // expected-error@-2 {{expected expression}}
  ;
#pragma acc parallel reduction(+:) // expected-error {{expected expression}}
  ;
#pragma acc parallel reduction(max:) // expected-error {{expected expression}}
  ;

#pragma acc parallel firstprivate(i jk) // expected-error {{expected ',' or ')' in 'firstprivate' clause}}
  ;
#pragma acc parallel private(jk i) // expected-error {{expected ',' or ')' in 'private' clause}}
  ;
#pragma acc parallel reduction(+:i jk) // expected-error {{expected ',' or ')' in 'reduction' clause}}
  ;

#pragma acc parallel firstprivate(foo) // expected-error {{use of undeclared identifier 'foo'}}
  ;
#pragma acc parallel private( q) // expected-error {{use of undeclared identifier 'q'}}
  ;
#pragma acc parallel reduction(max:bar) // expected-error {{use of undeclared identifier 'bar'}}
  ;

#pragma acc parallel firstprivate(i,) // expected-error {{expected expression}}
  ;
#pragma acc parallel private(jk, ) // expected-error {{expected expression}}
  ;
#pragma acc parallel reduction(*:i ,) // expected-error {{expected expression}}
  ;

#pragma acc parallel firstprivate(a[0], a[0:1])
  // expected-error@-1 {{expected variable name}}
  // expected-error@-2 {{expected variable name}}
  ;
#pragma acc parallel private(a[3:5], a[10])
  // expected-error@-1 {{expected variable name}}
  // expected-error@-2 {{expected variable name}}
  ;
#pragma acc parallel reduction(min:a[23],a[0:10])
  // expected-error@-1 {{expected variable name}}
  // expected-error@-2 {{expected variable name}}
  ;

#pragma acc parallel firstprivate(incomplete) private(incomplete) reduction(&:incomplete)
  // expected-error@-1 {{firstprivate variable cannot have incomplete type 'int []'}}
  // expected-error@-2 {{private variable cannot have incomplete type 'int []'}}
  // expected-error@-3 {{reduction variable cannot have incomplete type 'int []'}}
  ;

#pragma acc parallel firstprivate(constI, constIDecl)
  ;
#pragma acc parallel private(constI, constIDecl) reduction(max: constI, constIDecl)
  // expected-error@-1 {{const variable cannot be private because initialization is impossible}}
  // expected-error@-2 {{const variable cannot be private because initialization is impossible}}
  // expected-error@-3 {{reduction variable cannot be const}}
  // expected-error@-4 {{reduction variable cannot be const}}
  ;

#pragma acc parallel reduction(max:b,e,i,jk,f,d,p)
  ;
#pragma acc parallel reduction(max:fc,dc,a,s,u,uDecl)
  // expected-error@-1 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
  // expected-error@-2 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
  // expected-error@-3 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
  // expected-error@-4 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
  // expected-error@-5 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
  // expected-error@-6 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
  ;
#pragma acc parallel reduction(min:b,e,i,jk,f,d,p)
  ;
#pragma acc parallel reduction(min:fc,dc,a,s,u,uDecl)
  // expected-error@-1 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
  // expected-error@-2 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
  // expected-error@-3 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
  // expected-error@-4 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
  // expected-error@-5 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
  // expected-error@-6 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
  ;
#pragma acc parallel reduction(+:b,e,i,jk,f,d,fc,dc)
  ;
#pragma acc parallel reduction(+:p,a,s,u,uDecl)
  // expected-error@-1 {{OpenACC reduction operator '+' argument must be of arithmetic type}}
  // expected-error@-2 {{OpenACC reduction operator '+' argument must be of arithmetic type}}
  // expected-error@-3 {{OpenACC reduction operator '+' argument must be of arithmetic type}}
  // expected-error@-4 {{OpenACC reduction operator '+' argument must be of arithmetic type}}
  // expected-error@-5 {{OpenACC reduction operator '+' argument must be of arithmetic type}}
  ;
#pragma acc parallel reduction(*:b,e,i,jk,f,d,fc,dc)
  ;
#pragma acc parallel reduction(*:p,a,s,u,uDecl)
  // expected-error@-1 {{OpenACC reduction operator '*' argument must be of arithmetic type}}
  // expected-error@-2 {{OpenACC reduction operator '*' argument must be of arithmetic type}}
  // expected-error@-3 {{OpenACC reduction operator '*' argument must be of arithmetic type}}
  // expected-error@-4 {{OpenACC reduction operator '*' argument must be of arithmetic type}}
  // expected-error@-5 {{OpenACC reduction operator '*' argument must be of arithmetic type}}
  ;
#pragma acc parallel reduction(&&:b,e,i,jk,f,d,fc,dc)
  ;
#pragma acc parallel reduction(&&:p,a,s,u,uDecl)
  // expected-error@-1 {{OpenACC reduction operator '&&' argument must be of arithmetic type}}
  // expected-error@-2 {{OpenACC reduction operator '&&' argument must be of arithmetic type}}
  // expected-error@-3 {{OpenACC reduction operator '&&' argument must be of arithmetic type}}
  // expected-error@-4 {{OpenACC reduction operator '&&' argument must be of arithmetic type}}
  // expected-error@-5 {{OpenACC reduction operator '&&' argument must be of arithmetic type}}
  ;
#pragma acc parallel reduction(||:b,e,i,jk,f,d,fc,dc)
  ;
#pragma acc parallel reduction(||:p,a,s,u,uDecl)
  // expected-error@-1 {{OpenACC reduction operator '||' argument must be of arithmetic type}}
  // expected-error@-2 {{OpenACC reduction operator '||' argument must be of arithmetic type}}
  // expected-error@-3 {{OpenACC reduction operator '||' argument must be of arithmetic type}}
  // expected-error@-4 {{OpenACC reduction operator '||' argument must be of arithmetic type}}
  // expected-error@-5 {{OpenACC reduction operator '||' argument must be of arithmetic type}}
  ;
#pragma acc parallel reduction(&:b,e,i,jk)
  ;
#pragma acc parallel reduction(&:f,d,fc,dc,p,a,s,u,uDecl)
  // expected-error@-1 {{OpenACC reduction operator '&' argument must be of integer type}}
  // expected-error@-2 {{OpenACC reduction operator '&' argument must be of integer type}}
  // expected-error@-3 {{OpenACC reduction operator '&' argument must be of integer type}}
  // expected-error@-4 {{OpenACC reduction operator '&' argument must be of integer type}}
  // expected-error@-5 {{OpenACC reduction operator '&' argument must be of integer type}}
  // expected-error@-6 {{OpenACC reduction operator '&' argument must be of integer type}}
  // expected-error@-7 {{OpenACC reduction operator '&' argument must be of integer type}}
  // expected-error@-8 {{OpenACC reduction operator '&' argument must be of integer type}}
  // expected-error@-9 {{OpenACC reduction operator '&' argument must be of integer type}}
  ;
#pragma acc parallel reduction(|:b,e,i,jk)
  ;
#pragma acc parallel reduction(|:f,d,fc,dc,p,a,s,u,uDecl)
  // expected-error@-1 {{OpenACC reduction operator '|' argument must be of integer type}}
  // expected-error@-2 {{OpenACC reduction operator '|' argument must be of integer type}}
  // expected-error@-3 {{OpenACC reduction operator '|' argument must be of integer type}}
  // expected-error@-4 {{OpenACC reduction operator '|' argument must be of integer type}}
  // expected-error@-5 {{OpenACC reduction operator '|' argument must be of integer type}}
  // expected-error@-6 {{OpenACC reduction operator '|' argument must be of integer type}}
  // expected-error@-7 {{OpenACC reduction operator '|' argument must be of integer type}}
  // expected-error@-8 {{OpenACC reduction operator '|' argument must be of integer type}}
  // expected-error@-9 {{OpenACC reduction operator '|' argument must be of integer type}}
  ;
#pragma acc parallel reduction(^:b,e,i,jk)
  ;
#pragma acc parallel reduction(^:f,d,fc,dc,p,a,s,u,uDecl)
  // expected-error@-1 {{OpenACC reduction operator '^' argument must be of integer type}}
  // expected-error@-2 {{OpenACC reduction operator '^' argument must be of integer type}}
  // expected-error@-3 {{OpenACC reduction operator '^' argument must be of integer type}}
  // expected-error@-4 {{OpenACC reduction operator '^' argument must be of integer type}}
  // expected-error@-5 {{OpenACC reduction operator '^' argument must be of integer type}}
  // expected-error@-6 {{OpenACC reduction operator '^' argument must be of integer type}}
  // expected-error@-7 {{OpenACC reduction operator '^' argument must be of integer type}}
  // expected-error@-8 {{OpenACC reduction operator '^' argument must be of integer type}}
  // expected-error@-9 {{OpenACC reduction operator '^' argument must be of integer type}}
  ;

#pragma acc parallel reduction(&&:i,i,jk,d) reduction(&&:jk) reduction(max:d)
  // expected-error@-1 {{redundant '&&' reduction for variable 'i'}}
  // expected-note@-2 {{previous '&&' reduction here}}
  // expected-error@-3 {{redundant '&&' reduction for variable 'jk'}}
  // expected-note@-4 {{previous '&&' reduction here}}
  // expected-error@-5 {{conflicting 'max' reduction for variable 'd'}}
  // expected-note@-6 {{previous '&&' reduction here}}
  ;
#pragma acc parallel private(i) firstprivate(i,jk) private(jk)
  // expected-error@-1 {{private variable cannot be firstprivate}}
  // expected-note@-2 {{defined as private}}
  // expected-error@-3 {{firstprivate variable cannot be private}}
  // expected-note@-4 {{defined as firstprivate}}
  ;
#pragma acc parallel private(i) reduction(min:i) firstprivate(d) reduction(+:d)
  // expected-error@-1 {{private variable cannot be reduction}}
  // expected-note@-2 {{defined as private}}
  // expected-error@-3 {{firstprivate variable cannot be reduction}}
  // expected-note@-4 {{defined as firstprivate}}
  ;
#pragma acc parallel reduction(max:i) private(i) reduction(*:d) firstprivate(d)
  // expected-error@-1 {{reduction variable cannot be private}}
  // expected-note@-2 {{defined as reduction}}
  // expected-error@-3 {{reduction variable cannot be firstprivate}}
  // expected-note@-4 {{defined as reduction}}
  ;

#pragma acc parallel num_gangs num_workers vector_length
  // expected-error@-1 {{expected '(' after 'num_gangs'}}
  // expected-error@-2 {{expected '(' after 'num_workers'}}
  // expected-error@-3 {{expected '(' after 'vector_length'}}
  ;
#pragma acc parallel num_gangs(1
  // expected-error@-1 {{expected ')'}}
  // expected-note@-2 {{to match this '('}}
  ;
#pragma acc parallel num_workers( i
  // expected-error@-1 {{expected ')'}}
  // expected-note@-2 {{to match this '('}}
  ;
#pragma acc parallel vector_length (3 + 5
  // expected-error@-1 {{expected ')'}}
  // expected-note@-2 {{to match this '('}}
  ;
#pragma acc parallel num_gangs(i) num_gangs( i)
  // expected-error@-1 {{directive '#pragma acc parallel' cannot contain more than one 'num_gangs' clause}}
  ;
#pragma acc parallel num_workers(1) num_workers(1 ) num_workers(i+3)
  // expected-error@-1 {{directive '#pragma acc parallel' cannot contain more than one 'num_workers' clause}}
  // expected-error@-2 {{directive '#pragma acc parallel' cannot contain more than one 'num_workers' clause}}
  ;
#pragma acc parallel vector_length(1) vector_length(2 ) vector_length(3)
  // expected-error@-1 {{directive '#pragma acc parallel' cannot contain more than one 'vector_length' clause}}
  // expected-error@-2 {{directive '#pragma acc parallel' cannot contain more than one 'vector_length' clause}}
  ;
#pragma acc parallel num_gangs(bogusg) num_workers(bogusw) vector_length(bogusv)
  // expected-error@-1 {{use of undeclared identifier 'bogusg'}}
  // expected-error@-2 {{use of undeclared identifier 'bogusw'}}
  // expected-error@-3 {{use of undeclared identifier 'bogusv'}}
  ;
#pragma acc parallel num_gangs(i*1) num_workers(i+3) vector_length(i)
  // expected-error@-1 {{argument to 'vector_length' clause must be an integer constant expression}}
  ;
#pragma acc parallel num_gangs(31 * 3 + 1) num_workers(10 + 5) vector_length(5 * 2)
  ;
#pragma acc parallel num_gangs(1) num_workers(1) vector_length(1)
  ;
#pragma acc parallel num_gangs(0) num_workers(0) vector_length(0)
  // expected-error@-1 {{argument to 'num_gangs' clause must be a strictly positive integer value}}
  // expected-error@-2 {{argument to 'num_workers' clause must be a strictly positive integer value}}
  // expected-error@-3 {{argument to 'vector_length' clause must be a strictly positive integer value}}
  ;
#pragma acc parallel num_gangs(0u) num_workers(0u) vector_length(0u)
  // expected-error@-1 {{argument to 'num_gangs' clause must be a strictly positive integer value}}
  // expected-error@-2 {{argument to 'num_workers' clause must be a strictly positive integer value}}
  // expected-error@-3 {{argument to 'vector_length' clause must be a strictly positive integer value}}
  ;
#pragma acc parallel num_gangs(-1) num_workers(-1) vector_length(-1)
  // expected-error@-1 {{argument to 'num_gangs' clause must be a strictly positive integer value}}
  // expected-error@-2 {{argument to 'num_workers' clause must be a strictly positive integer value}}
  // expected-error@-3 {{argument to 'vector_length' clause must be a strictly positive integer value}}
  ;
#pragma acc parallel num_gangs(-5) num_workers(-5) vector_length(-5)
  // expected-error@-1 {{argument to 'num_gangs' clause must be a strictly positive integer value}}
  // expected-error@-2 {{argument to 'num_workers' clause must be a strictly positive integer value}}
  // expected-error@-3 {{argument to 'vector_length' clause must be a strictly positive integer value}}
  ;
#pragma acc parallel num_gangs(d) num_workers(f) vector_length(fc)
  // expected-error@-1 {{expression must have integral or unscoped enumeration type, not 'double'}}
  // expected-error@-2 {{expression must have integral or unscoped enumeration type, not 'float'}}
  // expected-error@-3 {{expression must have integral or unscoped enumeration type, not '_Complex float'}}
  // expected-error@-4 {{argument to 'vector_length' clause must be an integer constant expression}}
  ;
#pragma acc parallel num_gangs(1.0f) num_workers(2e3) vector_length(1.3l)
  // expected-error@-1 {{expression must have integral or unscoped enumeration type, not 'float'}}
  // expected-error@-2 {{expression must have integral or unscoped enumeration type, not 'double'}}
  // expected-error@-3 {{expression must have integral or unscoped enumeration type, not 'long double'}}
  // expected-error@-4 {{argument to 'vector_length' clause must be an integer constant expression}}
  ;

  // At one time, an assert failed due to the use of arrNoSize.
  int arrNoSize[]; // expected-noacc-error {{definition of variable with array type needs an explicit size or an initializer}}
#pragma acc parallel
  arrNoSize;

  // Bad nesting.
  #pragma acc parallel // expected-note {{enclosing '#pragma acc parallel' here}}
  #pragma acc parallel // expected-error {{'#pragma acc parallel' cannot be nested within '#pragma acc parallel'}}
    ;
  #pragma acc parallel
  {
    #pragma acc loop seq // expected-note {{enclosing '#pragma acc loop' here}}
    for (int i = 0; i < 2; ++i) {
      #pragma acc parallel // expected-error {{'#pragma acc parallel' cannot be nested within '#pragma acc loop'}}
        ;
    }
  }

  return 0;
}

// Complete to suppress an additional warning, but it's too late for pragmas.
int incomplete[3];
