// Check that Parser::SkipUntil knows about OpenACC.

// RUN: %clang_cc1 -verify -fopenacc %s

void fun0();

void fun1() {

  fun0(
#pragma acc parallel // expected-error {{expected expression}}
  ;
}

void fun2() {
#pragma acc parallel
  {
  fun0(
#pragma acc loop // expected-error {{expected expression}}
  ;              // expected-error {{statement after '#pragma acc loop' must be a for loop}}
  }
}
