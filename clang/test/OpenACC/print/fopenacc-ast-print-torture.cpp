// This test checks a few -fopenacc-ast-print output formatting cases that
// fopenacc-ast-print-torture.c could not because they occur only in C++.
//
// Strip comments and blank lines so checking -fopenacc-ast-print output is
// easier.
// RUN: grep -v '^ *\(//.*\)\?$' %s | sed 's,//.*,,' > %t-acc.cpp

// DEFINE: %{check-prt-arg}( PRT_ARG %, PRT_CHK %) =                           \
// DEFINE:   : '----------- -fopenacc-ast-print=%{PRT_ARG} -----------'  &&    \
// DEFINE:   %clang -Xclang -verify -fopenacc-ast-print=%{PRT_ARG}             \
// DEFINE:          -Wno-openacc-and-cxx -Wno-openacc-omp-ext                  \
// DEFINE:          %t-acc.cpp > %t.out                                  &&    \
// DEFINE:   FileCheck -check-prefixes=%{PRT_CHK} -match-full-lines            \
// DEFINE:             -strict-whitespace -input-file=%t.out %s

//                        PRT_ARG    PRT_CHK
// RUN: %{check-prt-arg}( acc     %, PRT,PRT-A,PRT-AA %)
// RUN: %{check-prt-arg}( omp     %, PRT,PRT-O,PRT-OO %)
// RUN: %{check-prt-arg}( acc-omp %, PRT,PRT-A,PRT-AO %)
// RUN: %{check-prt-arg}( omp-acc %, PRT,PRT-O,PRT-OA %)

// END.

/* expected-no-diagnostics */

// Generate unique name based on the line number.
#define UNIQUE_NAME CONCAT2(unique_name_, __LINE__)
#define CONCAT2(X, Y) CONCAT(X, Y)
#define CONCAT(X, Y) X##Y

// PRT:int start;
int start;

// Check on function when routine directive has OpenMP translation and non-empty
// indentation.

//    PRT-NEXT:class unique_name_{{[0-9]+}} {
//  PRT-A-NEXT:    #pragma acc routine seq
// PRT-AO-NEXT:    // #pragma omp declare target
//  PRT-O-NEXT:    #pragma omp declare target
// PRT-OA-NEXT:    // #pragma acc routine seq
//    PRT-NEXT:    void unique_name_{{[0-9]+}}();
//  PRT-O-NEXT:    #pragma omp end declare target
// PRT-AO-NEXT:    // #pragma omp end declare target
//  PRT-A-NEXT:    #pragma acc routine seq
// PRT-AO-NEXT:    // #pragma omp declare target
//  PRT-O-NEXT:    #pragma omp declare target
// PRT-OA-NEXT:    // #pragma acc routine seq
//    PRT-NEXT:    void unique_name_{{[0-9]+}}() {
//    PRT-NEXT:    }
//  PRT-O-NEXT:    #pragma omp end declare target
// PRT-AO-NEXT:    // #pragma omp end declare target
//    PRT-NEXT:};
class UNIQUE_NAME {
  #pragma acc routine seq
  void UNIQUE_NAME();
  #pragma acc routine seq
  void UNIQUE_NAME() {}
};

//    PRT-NEXT:namespace unique_name_{{[0-9]+}} {
//  PRT-A-NEXT:    #pragma acc routine seq
// PRT-AO-NEXT:    // #pragma omp declare target
//  PRT-O-NEXT:    #pragma omp declare target
// PRT-OA-NEXT:    // #pragma acc routine seq
//    PRT-NEXT:    void unique_name_{{[0-9]+}}();
//  PRT-O-NEXT:    #pragma omp end declare target
// PRT-AO-NEXT:    // #pragma omp end declare target
//  PRT-A-NEXT:    #pragma acc routine seq
// PRT-AO-NEXT:    // #pragma omp declare target
//  PRT-O-NEXT:    #pragma omp declare target
// PRT-OA-NEXT:    // #pragma acc routine seq
//    PRT-NEXT:    void unique_name_{{[0-9]+}}() {
//    PRT-NEXT:    }
//  PRT-O-NEXT:    #pragma omp end declare target
// PRT-AO-NEXT:    // #pragma omp end declare target
//    PRT-NEXT:}
namespace UNIQUE_NAME {
  #pragma acc routine seq
  void UNIQUE_NAME();
  #pragma acc routine seq
  void UNIQUE_NAME() {}
}

// PRT-NOT: {{.}}
