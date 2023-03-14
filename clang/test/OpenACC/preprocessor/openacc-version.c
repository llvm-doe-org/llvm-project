// Check _OPENACC definition.

// Check that #define _OPENACC is generated as expected.
//
// DEFINE: %{check-def}( FC_PRES %, CFLAGS %) =                                \
// DEFINE:   %clang -Xclang -verify=none -E -dM %{CFLAGS} %s |                 \
// DEFINE:     FileCheck -DACC_VERSION='%acc-version'                          \
// DEFINE:               -check-prefixes=CHECK,%{FC_PRES} %s
//
//                    FC_PRES     CFLAGS
// RUN: %{check-def}( NO-ACC  %,                                                    %)
// RUN: %{check-def}( ACC     %, -fopenacc                                          %)
// RUN: %{check-def}( ACC     %, -fopenacc -fopenmp-targets=%off-tgts -o - %libs-bc %)
//
// CHECK-NOT: #define _OPENACC
//       ACC: {{^}}#define _OPENACC [[ACC_VERSION]]{{$}}
//   ACC-NOT: #define _OPENACC

// Sanity-check one of those configurations to be sure we can actually use the
// _OPENACC macro as expected.
//
// RUN: %clang -Xclang -verify=none -E -fopenacc -DACC_VERSION=%acc-version %s \
// RUN: | FileCheck -check-prefix=CPP %s

// Check that -fopenacc-ast-print does not warn or add an _OPENACC definition as
// the output is fully expanded.
//
// DEFINE: %{check-ast-print}( PRT %) =                                        \
// DEFINE:   %clang -Xclang -verify=none -fopenacc-ast-print=%{PRT}            \
// DEFINE:          -DACC_VERSION=%acc-version %s |                            \
// DEFINE:     FileCheck -check-prefix=CPP %s
//
// RUN: %{check-ast-print}( acc     %)
// RUN: %{check-ast-print}( omp     %)
// RUN: %{check-ast-print}( acc-omp %)
// RUN: %{check-ast-print}( omp-acc %)

// Check that -fopenacc-print=acc|acc-omp does not warn or add an _OPENACC
// definition as the output is still OpenACC.
//
// DEFINE: %{check-print-acc}( PRT %) =                                        \
// DEFINE:   %clang -Xclang -verify=none -fopenacc-print=%{PRT} %s > %t-acc.c  \
// DEFINE:          -DACC_VERSION=%acc-version && \
// DEFINE:   diff -u %s %t-acc.c
//
// RUN: %{check-print-acc}( acc     %)
// RUN: %{check-print-acc}( acc-omp %)

// Check that -fopenacc-print=omp|omp-acc does warn and add an _OPENACC
// definition unless the compiler-generated _OPENACC definition isn't used.
//
// RUN: echo '#define _OPENACC %acc-version // inserted by Clang for OpenACC-to-OpenMP translation' \
// RUN:      > %t-omp-expected.c
// RUN: cat %s >> %t-omp-expected.c
//
// RUN: echo '// none-no-diagnostics' >  %t-no-def.c
// RUN: echo 'void foo();'            >> %t-no-def.c
//
// DEFINE: %{check-print-omp}( PRT %) =                                        \
//           # Compiler's _OPENACC def is used.
// DEFINE:   %clang -Xclang -verify=inserted -fopenacc-print=%{PRT} %s         \
// DEFINE:          > %t-omp.c -DACC_VERSION=%acc-version &&                   \
// DEFINE:   diff -u %t-omp-expected.c %t-omp.c &&                             \
//
//           # Repeat that but check that we can suppress the warning.
// DEFINE:   %clang -Xclang -verify=none -fopenacc-print=%{PRT} %s > %t-omp.c  \
// DEFINE:          -DACC_VERSION=%acc-version                                 \
// DEFINE:          -Wno-openacc-omp-macro-inserted &&                         \
// DEFINE:   diff -u %t-omp-expected.c %t-omp.c &&                             \
//
//           # Compiler's _OPENACC def isn't used because of inserted def.
// DEFINE:   %clang -Xclang -verify=none -fopenacc-print=%{PRT} %t-omp.c       \
// DEFINE:          -DACC_VERSION=%acc-version > %t-omp2.c &&                  \
// DEFINE:   diff -u %t-omp.c %t-omp2.c &&                                     \
//
//           # _OPENACC isn't used at all.
// DEFINE:   %clang -Xclang -verify=none -fopenacc-print=%{PRT} %t-no-def.c    \
// DEFINE:          > %t-no-def-omp.c &&                                       \
// DEFINE:   diff -u %t-no-def.c %t-no-def-omp.c
//
// RUN: %{check-print-omp}( omp     %)
// RUN: %{check-print-omp}( omp-acc %)

// END.

// none-no-diagnostics

// inserted-warning@1 {{inserted '_OPENACC' macro definition for uses that remain after translation to OpenMP}}
// inserted-note@1 {{use caution when modifying the OpenMP translation as this definition intentionally mispresents the compilation mode}}

// CPP-NOT: _OPENACC
// CPP-NOT: disabledOpenACC
//     CPP: enabledOpenACC
// CPP-NOT: _OPENACC
// CPP-NOT: disabledOpenACC

#ifdef _OPENACC
# if defined(_OPENACC) && _OPENACC == ACC_VERSION
void enabledOpenACC();
# else
void disabledOpenACC();
# endif
#else
void disabledOpenACC();
#endif
