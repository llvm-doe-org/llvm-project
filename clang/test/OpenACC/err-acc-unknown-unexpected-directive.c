// OpenACC disabled
// RUN: %clang_cc1 -verify=noacc %s

// OpenACC enabled
// RUN: %clang_cc1 -verify -fopenacc %s

// noacc-no-diagnostics

struct S {
  #pragma acc // expected-error {{unknown or unsupported OpenACC directive}}
  int i;
  #pragma acc foo // expected-error {{unknown or unsupported OpenACC directive}}
  int j;
  #pragma acc parallel // expected-error {{unexpected OpenACC directive '#pragma acc parallel'}}
  int k;
};

union U {
  #pragma acc // expected-error {{unknown or unsupported OpenACC directive}}
  int i;
  #pragma acc bar // expected-error {{unknown or unsupported OpenACC directive}}
  int j;
  #pragma acc loop // expected-error {{unexpected OpenACC directive '#pragma acc loop'}}
  int k;
};

#pragma acc // expected-error {{unknown or unsupported OpenACC directive}}
#pragma acc foobar // expected-error {{unknown or unsupported OpenACC directive}}
#pragma acc parallel loop // expected-error {{unexpected OpenACC directive '#pragma acc parallel loop'}}

#pragma acc routine seq // expected-error {{unknown or unsupported OpenACC directive}}
int main() {
#pragma acc // expected-error {{unknown or unsupported OpenACC directive}}
  ;
#pragma acc foobar // expected-error {{unknown or unsupported OpenACC directive}}
  ;
#pragma acc routine seq // expected-error {{unknown or unsupported OpenACC directive}}
  ;
  return 0;
}
