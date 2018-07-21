// OpenACC disabled
// RUN: %clang_cc1 -verify=noacc %s

// OpenACC enabled
// RUN: %clang_cc1 -verify -fopenacc %s

// noacc-no-diagnostics

int main() {
#pragma acc // expected-error {{expected an OpenACC directive}}
  ;
#pragma acc foo // expected-error {{expected an OpenACC directive}}
  ;
  return 0;
}
