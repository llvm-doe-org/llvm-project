// OpenACC disabled
// RUN: %clang_cc1 -verify=noacc %s

// OpenACC enabled
// RUN: %clang_cc1 -verify -fopenacc %s

// noacc-no-diagnostics

#pragma acc // expected-error {{unknown or unsupported OpenACC directive}}
#pragma acc foobar // expected-error {{unknown or unsupported OpenACC directive}}
#pragma acc data // expected-error {{unexpected OpenACC directive '#pragma acc data'}}
#pragma acc parallel // expected-error {{unexpected OpenACC directive '#pragma acc parallel'}}
#pragma acc loop // expected-error {{unexpected OpenACC directive '#pragma acc loop'}}
#pragma acc parallel loop // expected-error {{unexpected OpenACC directive '#pragma acc parallel loop'}}

int i;
#pragma acc declare copy(i) // expected-error {{unknown or unsupported OpenACC directive}}

int main() {
#pragma acc // expected-error {{unknown or unsupported OpenACC directive}}
  ;
#pragma acc foobar // expected-error {{unknown or unsupported OpenACC directive}}
  ;
#pragma acc declare copy(i) // expected-error {{unknown or unsupported OpenACC directive}}
  ;
  return 0;
}

struct S {
  #pragma acc // expected-error {{unknown or unsupported OpenACC directive}}
  int i;
  #pragma acc foo // expected-error {{unknown or unsupported OpenACC directive}}
  int j;
  #pragma acc parallel // expected-error {{unexpected OpenACC directive '#pragma acc parallel'}}
  int k;
  #pragma acc routine seq // expected-error {{unexpected OpenACC directive '#pragma acc routine'}}
  int (*fp)();
};

union U {
  #pragma acc // expected-error {{unknown or unsupported OpenACC directive}}
  int i;
  #pragma acc bar // expected-error {{unknown or unsupported OpenACC directive}}
  int j;
  #pragma acc loop // expected-error {{unexpected OpenACC directive '#pragma acc loop'}}
  int k;
  #pragma acc routine seq // expected-error {{unexpected OpenACC directive '#pragma acc routine'}}
  int (*fp)();
};
