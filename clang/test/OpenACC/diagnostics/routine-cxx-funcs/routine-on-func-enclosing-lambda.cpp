// Check that diagnostics behave for directives within a lambda where an
// enclosing explicit routine directive applies to a function enclosing the
// lambda.
//
// This used to fail an incorrect assertion that any lexically enclosing routine
// directive applies to the nearest lexically enclosing function.  However, the
// assertion was skipped in the case of prior errors, so this test must stand
// alone from other diagnostics tests.
//
// We specifically check a diagnostic that would be skipped if the enclosing
// routine directive did apply to the lambda.
//
// RUN: %clang_cc1 -fopenacc -Wno-openacc-and-cxx -verify %s
//
// END.

#pragma acc routine seq
void fn() {
  auto lambda = []() {
    // expected-error@+3 {{function 'fn()::(anonymous class)::operator()' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
    // expected-note@+2 {{explicit '#pragma acc routine' is currently not supported for lambdas, but }}
    // expected-note@+1 {{you can disable this diagnostic with '-Wno-openacc-routine-cxx-lambda'}}
    #pragma acc loop
    for (int i = 0; i < 8; ++i)
    ;
    auto nestedLambda = []() {
      // expected-error@+3 {{function 'fn()::(anonymous class)::operator()()::(anonymous class)::operator()' has no explicit '#pragma acc routine' but contains orphaned '#pragma acc loop'}}
      // expected-note@+2 {{explicit '#pragma acc routine' is currently not supported for lambdas, but }}
      // expected-note@+1 {{you can disable this diagnostic with '-Wno-openacc-routine-cxx-lambda'}}
      #pragma acc loop
      for (int i = 0; i < 8; ++i)
      ;
    };
  };
}
