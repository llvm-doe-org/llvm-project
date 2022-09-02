// This test checks basic functionality of -fopenacc[-ast]-print.  Output is
// thoroughly checked for more complex situations in directive-specific tests
// and in fopenacc-print-torture.c.

// Define iterations over printing options and arguments.
//
// DEFINE: %{check-prt-bad-arg}( PRT_OPT %, PRT_ARG %) =
// DEFINE: %{check-prt-bad-args}( PRT_OPT %) =                                 \
// DEFINE:   %{check-prt-bad-arg}( %{PRT_OPT} %, foo %) &&                     \
// DEFINE:   %{check-prt-bad-arg}( %{PRT_OPT} %,     %)
//
// DEFINE: %{check-prt-arg}( PRT_OPT %, PRT_ARG %, PRT_CHK %) =
// DEFINE: %{check-prt-args}( PRT_OPT %) =                                     \
// DEFINE:   %{check-prt-arg}( %{PRT_OPT} %, acc     %, PRT,PRT-A        %) && \
// DEFINE:   %{check-prt-arg}( %{PRT_OPT} %, omp     %, PRT,PRT-O        %) && \
// DEFINE:   %{check-prt-arg}( %{PRT_OPT} %, acc-omp %, PRT,PRT-A,PRT-AO %) && \
// DEFINE:   %{check-prt-arg}( %{PRT_OPT} %, omp-acc %, PRT,PRT-O,PRT-OA %)
//
// DEFINE: %{check-prt-opt}( PRT_OPT %) =
// DEFINE: %{check-prt-opts} =                                                 \
// DEFINE:   %{check-prt-opt}( -fopenacc-ast-print %) &&                       \
// DEFINE:   %{check-prt-opt}( -fopenacc-print     %)

// Check bad -fopenacc[-ast]-print values.
//
// REDEFINE: %{check-prt-bad-arg}( PRT_OPT %, PRT_ARG %) =                     \
// REDEFINE:   : '---------- %{PRT_OPT}=%{PRT_ARG} ----------' &&              \
// REDEFINE:   not %clang_cc1 %{PRT_OPT}=%{PRT_ARG} %s > %t.out 2>&1 &&        \
// REDEFINE:   FileCheck -check-prefix=BAD-VALUE -input-file=%t.out %s         \
// REDEFINE:             -DOPT=%{PRT_OPT} -DVAL=%{PRT_ARG}
//
// REDEFINE: %{check-prt-opt}( PRT_OPT %) = %{check-prt-bad-args}( %{PRT_OPT} %)
//
// RUN: %{check-prt-opts}
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
// REDEFINE: %{check-prt-arg}( PRT_OPT %, PRT_ARG %, PRT_CHK %) =              \
// REDEFINE:   : '---------- %{PRT_OPT}=%{PRT_ARG} ----------' &&              \
// REDEFINE:   %clang_cc1 -verify %{PRT_OPT}=%{PRT_ARG} %t-acc.c |             \
// REDEFINE:     FileCheck -check-prefixes=%{PRT_CHK} %s &&                    \
// REDEFINE:   %clang -Xclang -verify %{PRT_OPT}=%{PRT_ARG} %t-acc.c |         \
// REDEFINE:     FileCheck -check-prefixes=%{PRT_CHK} %s
//
// REDEFINE: %{check-prt-opt}( PRT_OPT %) = %{check-prt-args}( %{PRT_OPT} %)
//
// RUN: %{check-prt-opts}

// Check that -fopenacc[-ast]-print works when reading .ast file.
//
// At one time, the -fopenacc-print value (default=acc) specified when parsing
// the source was stored as a language option, which was written to the .ast
// file and re-used when later reading and printing the .ast file, thus
// ignoring the new -fopenacc-print value.
//
// RUN: %clang -Xclang -verify -fopenacc -emit-ast -o %t.ast %t-acc.c
//
// REDEFINE: %{check-prt-arg}(PRT_OPT %, PRT_ARG %, PRT_CHK %) =               \
// REDEFINE:   : '---------- %{PRT_OPT}=%{PRT_ARG} ----------' &&              \
// REDEFINE:   %clang -Xclang -verify %{PRT_OPT}=%{PRT_ARG} %t.ast |           \
// REDEFINE:     FileCheck -check-prefixes=%{PRT_CHK} %s
//
// REDEFINE: %{check-prt-opt}( PRT_OPT %) = %{check-prt-args}( %{PRT_OPT} %)
//
// RUN: %{check-prt-opts}

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
