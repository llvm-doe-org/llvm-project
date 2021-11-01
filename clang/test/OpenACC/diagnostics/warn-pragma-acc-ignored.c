// RUN: %data {
//        OpenACC disabled, no warn
// RUN:   (verify=nowarn   flags=                        )
// RUN:   (verify=nowarn   flags=-fno-openacc            )
// RUN:   (verify=nowarn   flags='-fopenacc -fno-openacc')
//
//        OpenACC disabled, warn
// RUN:   (verify=expected flags='-Wsource-uses-openacc'                       )
// RUN:   (verify=expected flags='-fno-openacc -Wsource-uses-openacc'          )
// RUN:   (verify=expected flags='-fopenacc -fno-openacc -Wsource-uses-openacc')
//
//        OpenACC enabled (never warns)
// RUN:   (verify=nowarn   flags='-fopenacc'                                   )
// RUN:   (verify=nowarn   flags='-fno-openacc -fopenacc'                      )
// RUN:   (verify=nowarn   flags='-fopenacc -Wsource-uses-openacc'             )
// RUN:   (verify=nowarn   flags='-fno-openacc -fopenacc -Wsource-uses-openacc')
// RUN: }
// RUN: %for {
// RUN:   %clang -o %t -Xclang -verify=%[verify] %[flags] %s
// RUN: }

// nowarn-no-diagnostics

int main() {
#pragma acc parallel // expected-warning {{unexpected '#pragma acc ...' in program}}
  ;
#pragma acc parallel // only warns once
  ;
  return 0;
}
