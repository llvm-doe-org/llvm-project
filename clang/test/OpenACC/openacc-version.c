// Check _OPENACC definition.

// Check that #define _OPENACC is generated as expected for various offload
// configurations.
//
// RUN: %data tgts {
// RUN:   (run-if=                cflags=                                                      fc=DEFS                   )
// RUN:   (run-if=                cflags=-fopenacc                                             fc=DEFS,DEFS-ACC          )
// RUN:   (run-if=%run-if-x86_64  cflags='-fopenacc -fopenmp-targets=%run-x86_64-triple -o -'  fc=DEFS,DEFS-ACC,DEFS-ACC2)
// RUN:   (run-if=%run-if-ppc64le cflags='-fopenacc -fopenmp-targets=%run-ppc64le-triple -o -' fc=DEFS,DEFS-ACC,DEFS-ACC2)
// RUN:   (run-if=%run-if-nvptx64 cflags='-fopenacc -fopenmp-targets=%run-nvptx64-triple -o -' fc=DEFS,DEFS-ACC,DEFS-ACC2)
// RUN: }
// RUN: %for tgts {
// RUN:   %[run-if] %clang -Xclang -verify=none -E -dM %[cflags] %s > %t.out
// RUN:   %[run-if] FileCheck -check-prefixes=%[fc] -DACC_VERSION=%acc-version \
// RUN:                       -input-file=%t.out %s
// RUN: }
//
//  DEFS-NOT: #define _OPENACC
//  DEFS-ACC: #define _OPENACC [[ACC_VERSION]]
// DEFS-ACC2: #define _OPENACC [[ACC_VERSION]]
//  DEFS-NOT: #define _OPENACC

// Sanity-check one of those configurations to be sure we can actually use the
// _OPENACC macro as expected.
//
// RUN: %clang -Xclang -verify=none -E -fopenacc -DACC_VERSION=%acc-version %s \
// RUN: | FileCheck -check-prefix=CPP %s

// Check that -fopenacc-ast-print does not warn or add an _OPENACC definition as
// the output is fully expanded.
//
// RUN: %data prts {
// RUN:   (prt=acc)
// RUN:   (prt=omp)
// RUN:   (prt=acc-omp)
// RUN:   (prt=omp-acc)
// RUN: }
// RUN: %for prts {
// RUN:   %clang -Xclang -verify=none -fopenacc-ast-print=%[prt] \
// RUN:          -DACC_VERSION=%acc-version %s \
// RUN:   | FileCheck -check-prefix=CPP %s
// RUN: }

// Check that -fopenacc-print=acc|acc-omp does not warn or add an _OPENACC
// definition as the output is still OpenACC.
//
// RUN: %data acc-prts {
// RUN:   (prt=acc)
// RUN:   (prt=acc-omp)
// RUN: }
// RUN: %for acc-prts {
// RUN:   %clang -Xclang -verify=none -fopenacc-print=%[prt] %s > %t-acc.c \
// RUN:          -DACC_VERSION=%acc-version
// RUN:   diff -u %s %t-acc.c
// RUN: }

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
// RUN: %data omp-prts {
// RUN:   (prt=omp)
// RUN:   (prt=omp-acc)
// RUN: }
// RUN: %for omp-prts {
//        # Compiler's _OPENACC def is used.
// RUN:   %clang -Xclang -verify=inserted -fopenacc-print=%[prt] %s > %t-omp.c \
// RUN:          -DACC_VERSION=%acc-version
// RUN:   diff -u %t-omp-expected.c %t-omp.c
//
//        # Repeat that but check that we can suppress the warning.
// RUN:   %clang -Xclang -verify=none -fopenacc-print=%[prt] %s > %t-omp.c \
// RUN:          -DACC_VERSION=%acc-version -Wno-openacc-omp-macro-inserted
// RUN:   diff -u %t-omp-expected.c %t-omp.c
//
//        # Compiler's _OPENACC def isn't used because of inserted def.
// RUN:   %clang -Xclang -verify=none -fopenacc-print=%[prt] %t-omp.c \
// RUN:          -DACC_VERSION=%acc-version > %t-omp2.c
// RUN:   diff -u %t-omp.c %t-omp2.c
//
//        # _OPENACC isn't used at all.
// RUN:   %clang -Xclang -verify=none -fopenacc-print=%[prt] %t-no-def.c \
// RUN:          > %t-no-def-omp.c
// RUN:   diff -u %t-no-def.c %t-no-def-omp.c
// RUN: }

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
