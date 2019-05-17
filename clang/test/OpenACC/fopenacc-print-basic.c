// This test checks basic functionality of -fopenacc[-ast]-print.  Output is
// thoroughly checked for more complex situations in directive-specific tests
// and in fopenacc-print-torture.c.

// RUN: %data prt-opts {
// RUN:   (prt-opt=-fopenacc-ast-print)
// RUN:   (prt-opt=-fopenacc-print    )
// RUN: }

// Check bad -fopenacc[-ast]-print values.
//
// RUN: %data bad-values {
// RUN:   (val=foo)
// RUN:   (val=   )
// RUN: }
// RUN: %for bad-values {
// RUN:   %for prt-opts {
// RUN:     not %clang_cc1 %[prt-opt]=%[val] %s 2>&1 \
// RUN:     | FileCheck -check-prefix=BAD-VALUE %s \
// RUN:                 -DOPT=%[prt-opt] -DVAL=%[val]
// RUN:   }
// RUN: }
//
// BAD-VALUE: invalid value '[[VAL]]' in '[[OPT]]=[[VAL]]'

// Check basic functionality.
//
// RUN: %clang_cc1 -fopenacc -verify -ast-print %s \
// RUN: | FileCheck -check-prefixes=PRT,PRT-A %s
// RUN: %clang -fopenacc -Xclang -verify -Xclang -ast-print -fsyntax-only %s \
// RUN: | FileCheck -check-prefixes=PRT,PRT-A %s
//
// Strip comments and blank lines so checking -fopenacc-print output is easier.
// RUN: echo "// expected""-no-diagnostics" > %t-acc.c
// RUN: grep -v '^ *\(//.*\)\?$' %s | sed 's,//.*,,' >> %t-acc.c
//
// RUN: %data prt-args {
// RUN:   (prt-arg=acc     prt-chk=PRT,PRT-A       )
// RUN:   (prt-arg=omp     prt-chk=PRT,PRT-O       )
// RUN:   (prt-arg=acc-omp prt-chk=PRT,PRT-A,PRT-AO)
// RUN:   (prt-arg=omp-acc prt-chk=PRT,PRT-O,PRT-OA)
// RUN: }
// RUN: %for prt-opts {
// RUN:   %for prt-args {
// RUN:     %clang_cc1 -verify %[prt-opt]=%[prt-arg] %t-acc.c \
// RUN:     | FileCheck -check-prefixes=%[prt-chk] %s
// RUN:     %clang -Xclang -verify %[prt-opt]=%[prt-arg] %t-acc.c \
// RUN:     | FileCheck -check-prefixes=%[prt-chk] %s
// RUN:   }
// RUN: }

// Check that -fopenacc[-ast]-print works when reading .ast file.
//
// At one time, the -fopenacc-print value (default=acc) specified when parsing
// the source was stored as a language option, which was written to the .ast
// file and re-used when later reading and printing the .ast file, thus
// ignoring the new -fopenacc-print value.
//
// RUN: %clang -Xclang -verify -fopenacc -emit-ast -o %t.ast %t-acc.c
// RUN: %for prt-opts {
// RUN:   %for prt-args {
// RUN:     %clang -Xclang -verify %[prt-opt]=%[prt-arg] %t.ast \
// RUN:     | FileCheck -check-prefixes=%[prt-chk] %s
// RUN:   }
// RUN: }

// END.

// expected-no-diagnostics

// PRT: int main() {
int main() {
  // PRT-A-NEXT:  {{^ *}}#pragma acc parallel{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams{{$}}
  // PRT-O-NEXT:  {{^ *}}#pragma omp target teams{{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel{{$}}
  #pragma acc parallel
    // PRT-NEXT: ;
    ;
}// PRT-NEXT: }
// PRT-NOT: {{.}}
