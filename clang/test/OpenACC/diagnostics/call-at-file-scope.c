// For a function call at file scope, Clang used to fail an assertion because
// the OpenACC routine directive analysis incorrectly assumed the call would
// already be discarded because it's not permitted in C.
//
// RUN: %clang_cc1 -fopenacc -verify %s
//
// END.

int f();
int i = f(); // expected-error {{initializer element is not a compile-time constant}}
