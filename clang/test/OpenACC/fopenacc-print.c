// Check bad -fopenacc-print values.
//
// RUN: not %clang_cc1 -fopenacc-print=foo %s 2>&1 \
// RUN: | FileCheck -check-prefix=VALUE-FOO %s
// RUN: not %clang -fopenacc-print= %s 2>&1 \
// RUN: | FileCheck -check-prefix=VALUE-EMPTY %s
//
// VALUE-FOO: invalid value 'foo' in '-fopenacc-print=foo'
// VALUE-EMPTY: invalid value '' in '-fopenacc-print='

// Check basic functionality.
//
// RUN: %clang_cc1 -fopenacc -verify -ast-print %s \
// RUN: | FileCheck -check-prefixes=PRT,PRT-A %s
// RUN: %clang -fopenacc -Xclang -verify -Xclang -ast-print -fsyntax-only %s \
// RUN: | FileCheck -check-prefixes=PRT,PRT-A %s
//
// RUN: %data prints {
// RUN:   (prt=acc     pre=PRT,PRT-A       )
// RUN:   (prt=omp     pre=PRT,PRT-O       )
// RUN:   (prt=acc-omp pre=PRT,PRT-A,PRT-AO)
// RUN:   (prt=omp-acc pre=PRT,PRT-O,PRT-OA)
// RUN: }
//
// RUN: %for prints {
// RUN:   %clang_cc1 -verify -fopenacc-print=%[prt] %s \
// RUN:   | FileCheck -check-prefixes=%[pre] %s
// RUN:   %clang -Xclang -verify -fopenacc-print=%[prt] %s \
// RUN:   | FileCheck -check-prefixes=%[pre] %s
// RUN: }

// Check that -fopenacc-print works when reading .ast file.
//
// At one time, the -fopenacc-print value (default=acc) specified when parsing
// the source was stored as a language option, which was written to the .ast
// file and re-used when later reading and printing the .ast file, thus
// ignoring the new -fopenacc-print value.
//
// RUN: %clang -Xclang -verify -fopenacc -emit-ast -o %t.ast %s
// RUN: %for prints {
// RUN:   %clang -Xclang -verify -fopenacc-print=%[prt] %t.ast \
// RUN:   | FileCheck -check-prefixes=%[pre] %s
// RUN: }

// expected-no-diagnostics

// -fopenacc-print output is thoroughly checked for more complex directives in
// directive-specific tests.

// PRT: {{^}}int main() {{[{]$}}
int main() {
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams{{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel{{$}}
  #pragma acc parallel
  // PRT-NEXT: {{^ *;$}}
    ;
// PRT-NEXT: {{^[}]$}}
}
// PRT-NOT: {{.}}
