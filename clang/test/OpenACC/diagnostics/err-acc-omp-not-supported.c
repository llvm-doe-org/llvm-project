// DEFINE: %{check-good}(CLANG%) = %{CLANG} -o %t %s > %t.out 2>&1 &&          \
// DEFINE:                         FileCheck -allow-empty %s < %t.out
// DEFINE: %{check-bad}(CLANG%) = not %{CLANG} -o %t %s > %t.out 2>&1 &&       \
// DEFINE:                        FileCheck -check-prefix=ERROR %s < %t.out
// DEFINE: %{check-accs}(RUN%, CLANG%, CFLAGS%) =                              \
// DEFINE:   %{RUN}( %{CLANG} -fopenacc %{CFLAGS}                   %)      && \
// DEFINE:   %{RUN}( %{CLANG} -fopenacc %{CFLAGS}                   %)      && \
// DEFINE:   %{RUN}( %{CLANG} -fopenacc-ast-print=acc %{CFLAGS}     %)      && \
// DEFINE:   %{RUN}( %{CLANG} -fopenacc-ast-print=omp %{CFLAGS}     %)      && \
// DEFINE:   %{RUN}( %{CLANG} -fopenacc-ast-print=acc-omp %{CFLAGS} %)      && \
// DEFINE:   %{RUN}( %{CLANG} -fopenacc-ast-print=omp-acc %{CFLAGS} %)      && \
// DEFINE:   %{RUN}( %{CLANG} -fopenacc-print=acc %{CFLAGS}         %)      && \
// DEFINE:   %{RUN}( %{CLANG} -fopenacc-print=omp %{CFLAGS}         %)      && \
// DEFINE:   %{RUN}( %{CLANG} -fopenacc-print=acc-omp %{CFLAGS}     %)      && \
// DEFINE:   %{RUN}( %{CLANG} -fopenacc-print=omp-acc %{CFLAGS}     %)

// Make sure we're checking sane OpenMP options.
// RUN: %{check-good}( %clang -fopenmp        %)
// RUN: %{check-good}( %clang -fopenmp=libomp %)
// RUN: %{check-good}( %clang_cc1 -fopenmp    %)

// Make sure we're checking sane OpenACC options.
//                     RUN              CLANG         CFLAGS
// RUN: %{check-accs}( %{check-good} %, %clang     %,        %)
// RUN: %{check-accs}( %{check-good} %, %clang_cc1 %,        %)

// Make sure we cannot combine OpenACC and OpenMP options.
//                     RUN             CLANG                     CFLAGS
// RUN: %{check-accs}( %{check-bad} %, %clang -fopenmp        %,                 %)
// RUN: %{check-accs}( %{check-bad} %, %clang -fopenmp=libomp %,                 %)
// RUN: %{check-accs}( %{check-bad} %, %clang_cc1 -fopenmp    %,                 %)
// RUN: %{check-accs}( %{check-bad} %, %clang                 %, -fopenmp        %)
// RUN: %{check-accs}( %{check-bad} %, %clang                 %, -fopenmp=libomp %)
// RUN: %{check-accs}( %{check-bad} %, %clang_cc1             %, -fopenmp        %)

// END.

// Braces prevent these directives from matching themselves when printing.
// CHECK-NOT: {{error:}} -fopenacc combined with -fopenmp is not supported
// ERROR: {{error:}} -fopenacc combined with -fopenmp is not supported

int main() {
  return 0;
}
