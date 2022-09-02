// Check when -Wopenacc-omp-map-ompx-no-alloc is enabled and when it's an error.
//
// Diagnostics about bad -fopenacc-no-create-omp arguments are checked in
// diagnostics/fopenacc-no-create-omp.c.  The behavior of no_create clauses for
// different values of -fopenacc-no-create-omp is checked in
// directives/Tests/no-create.c.

// DEFINE: %{check-no-create-opt}( CFLAGS %, VERIFY %) =                       \
// DEFINE:   : '------ VERIFY=%{VERIFY}; CFLAGS=%{CFLAGS} -------'          && \
// DEFINE:   %clang %{CFLAGS} -Xclang -verify=%{VERIFY} %s > /dev/null         \
// DEFINE:       -Wno-openacc-omp-map-ompx-hold

// DEFINE: %{check-no-create-opts}( CFLAGS %, NO_ALLOC %) =                                                  \
//                                   CFLAGS                                                VERIFY
// DEFINE:   %{check-no-create-opt}( %{CFLAGS}                                          %, %{NO_ALLOC} %) && \
// DEFINE:   %{check-no-create-opt}( %{CFLAGS} -fopenacc-no-create-omp=ompx-no-alloc    %, %{NO_ALLOC} %) && \
// DEFINE:   %{check-no-create-opt}( %{CFLAGS} -fopenacc-no-create-omp=no-ompx-no-alloc %, none        %)

// DEFINE: %{check-warn-opts}( CFLAGS %, NONE %, WARN %, NOERR %) =                                         \
//                                    CFLAGS                                                NO_ALLOC
// DEFINE:   %{check-no-create-opts}( %{CFLAGS}                                          %, %{NONE}   %) && \
// DEFINE:   %{check-no-create-opts}( %{CFLAGS} -Wopenacc-omp-map-ompx-no-alloc          %, %{WARN}   %) && \
// DEFINE:   %{check-no-create-opts}( %{CFLAGS} -Wopenacc-omp-ext                        %, %{WARN}   %) && \
// DEFINE:   %{check-no-create-opts}( %{CFLAGS} -Wno-error=openacc-omp-map-ompx-no-alloc %, %{NOERR}  %) && \
// DEFINE:   %{check-no-create-opts}( %{CFLAGS} -Wno-error=openacc-omp-ext               %, %{NOERR}  %) && \
// DEFINE:   %{check-no-create-opts}( %{CFLAGS} -Werror=openacc-omp-map-ompx-no-alloc    %, error     %) && \
// DEFINE:   %{check-no-create-opts}( %{CFLAGS} -Werror=openacc-omp-ext                  %, error     %) && \
// DEFINE:   %{check-no-create-opts}( %{CFLAGS} -Wno-openacc-omp-map-ompx-no-alloc       %, none      %) && \
// DEFINE:   %{check-no-create-opts}( %{CFLAGS} -Wno-openacc-omp-ext                     %, none      %)

//                          CFLAGS                         NONE     WARN     NOERR
// Default is error.
// RUN: %{check-warn-opts}( -fopenacc-ast-print=omp     %, error %, error %, warn %)
// RUN: %{check-warn-opts}( -fopenacc-ast-print=acc-omp %, error %, error %, warn %)
// RUN: %{check-warn-opts}( -fopenacc-ast-print=omp-acc %, error %, error %, warn %)
// RUN: %{check-warn-opts}( -fopenacc-print=omp         %, error %, error %, warn %)
// RUN: %{check-warn-opts}( -fopenacc-print=acc-omp     %, error %, error %, warn %)
// RUN: %{check-warn-opts}( -fopenacc-print=omp-acc     %, error %, error %, warn %)
//
// Default is no warning.
// RUN: %{check-warn-opts}( -fopenacc-ast-print=acc     %, none  %, warn  %, none %)
// RUN: %{check-warn-opts}( -fopenacc-print=acc         %, none  %, warn  %, none %)
// RUN: %{check-warn-opts}( -c -fopenacc                %, none  %, warn  %, none %)

// END.

// none-no-diagnostics

int main() {
  int x, y, z;
  // error-error@+8  2 {{the OpenACC 'no_create' clause translation uses the 'ompx_no_alloc' map type modifier}}
  // error-note@+7   2 {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-no-create-omp=no-ompx-no-alloc'}}
  // error-note@+6   2 {{you can disable this diagnostic with '-Wno-openacc-omp-map-ompx-no-alloc'}}
  // error-note@+5   2 {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  // warn-warning@+4 2 {{the OpenACC 'no_create' clause translation uses the 'ompx_no_alloc' map type modifier}}
  // warn-note@+3    2 {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-no-create-omp=no-ompx-no-alloc'}}
  // warn-note@+2    2 {{you can disable this diagnostic with '-Wno-openacc-omp-map-ompx-no-alloc'}}
  // warn-note@+1    2 {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc data no_create(x, y) no_create(z)
  // error-error@+2  {{the translation of a visible OpenACC 'nomap' clause produces a 'ompx_no_alloc' map type modifier here}}
  // warn-warning@+1 {{the translation of a visible OpenACC 'nomap' clause produces a 'ompx_no_alloc' map type modifier here}}
  #pragma acc parallel
  x = 5;
  // warn-warning@+4 {{the OpenACC 'no_create' clause translation uses the 'ompx_no_alloc' map type modifier}}
  // warn-note@+3    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-no-create-omp=no-ompx-no-alloc'}}
  // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-map-ompx-no-alloc'}}
  // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc parallel no_create(x)
  ;
  // warn-warning@+4 {{the OpenACC 'no_create' clause translation uses the 'ompx_no_alloc' map type modifier}}
  // warn-note@+3    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-no-create-omp=no-ompx-no-alloc'}}
  // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-map-ompx-no-alloc'}}
  // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc parallel loop no_create(x)
  for (int i = 0; i < 5; ++i)
    ;
  return 0;
}
