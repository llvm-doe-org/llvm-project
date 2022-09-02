// DEFINE: %{check}( VERIFY %, CFLAGS %) =                                     \
// DEFINE:   %clang -o %t -Xclang -verify=%{VERIFY} %{CFLAGS} %s

// OpenACC disabled, no warn
// RUN: %{check}( nowarn %,                        %)
// RUN: %{check}( nowarn %, -fno-openacc           %)
// RUN: %{check}( nowarn %, -fopenacc -fno-openacc %)
//
// OpenACC disabled, warn
// RUN: %{check}( expected %, -Wsource-uses-openacc                        %)
// RUN: %{check}( expected %, -fno-openacc -Wsource-uses-openacc           %)
// RUN: %{check}( expected %, -fopenacc -fno-openacc -Wsource-uses-openacc %)
//
// OpenACC enabled (never warns)
// RUN: %{check}( nowarn %, -fopenacc                                    %)
// RUN: %{check}( nowarn %, -fno-openacc -fopenacc                       %)
// RUN: %{check}( nowarn %, -fopenacc -Wsource-uses-openacc              %)
// RUN: %{check}( nowarn %, -fno-openacc -fopenacc -Wsource-uses-openacc %)

// nowarn-no-diagnostics

int main() {
#pragma acc parallel // expected-warning {{unexpected '#pragma acc ...' in program}}
  ;
#pragma acc parallel // only warns once
  ;
  return 0;
}
