// Check when -Wopenacc-omp-map-present is enabled and when it's an error.
//
// Diagnostics about bad -fopenacc-present-omp arguments are checked in
// diagnostics/fopenacc-present-omp.c.  The behavior of present clauses for
// different values of -fopenacc-present-omp is checked in
// directives/Tests/present.c.

// DEFINE: %{check-present-opt}( CFLAGS %, VERIFY %) =                         \
// DEFINE:   : '------ VERIFY=%{VERIFY}; CFLAGS=%{CFLAGS} -------'          && \
// DEFINE:   %clang %{CFLAGS} -Xclang -verify=%{VERIFY} %s > /dev/null         \
// DEFINE:       -Wno-openacc-omp-map-ompx-hold

// DEFINE: %{check-present-opts}(CFLAGS %, PRESENT %) =                                          \
//                                CFLAGS                                        VERIFY
// DEFINE:   %{check-present-opt}(%{CFLAGS}                                  %, %{PRESENT} %) && \
// DEFINE:   %{check-present-opt}(%{CFLAGS} -fopenacc-present-omp=present    %, %{PRESENT} %) && \
// DEFINE:   %{check-present-opt}(%{CFLAGS} -fopenacc-present-omp=no-present %, none       %)

// DEFINE: %{check-warn-opts}( CFLAGS %, NONE %, WARN %, NOERR %) =                                \
//                                  CFLAGS                                          PRESENT
// DEFINE:   %{check-present-opts}( %{CFLAGS}                                    %, %{NONE}  %) && \
// DEFINE:   %{check-present-opts}( %{CFLAGS} -Wopenacc-omp-map-present          %, %{WARN}  %) && \
// DEFINE:   %{check-present-opts}( %{CFLAGS} -Wopenacc-omp-ext                  %, %{WARN}  %) && \
// DEFINE:   %{check-present-opts}( %{CFLAGS} -Wno-error=openacc-omp-map-present %, %{NOERR} %) && \
// DEFINE:   %{check-present-opts}( %{CFLAGS} -Wno-error=openacc-omp-ext         %, %{NOERR} %) && \
// DEFINE:   %{check-present-opts}( %{CFLAGS} -Werror=openacc-omp-map-present    %, error    %) && \
// DEFINE:   %{check-present-opts}( %{CFLAGS} -Werror=openacc-omp-ext            %, error    %) && \
// DEFINE:   %{check-present-opts}( %{CFLAGS} -Wno-openacc-omp-map-present       %, none     %) && \
// DEFINE:   %{check-present-opts}( %{CFLAGS} -Wno-openacc-omp-ext               %, none     %)

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
  // error-error@+8  2 {{the OpenACC 'present' clause translation uses the 'present' map type modifier}}
  // error-note@+7   2 {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-present-omp=no-present'}}
  // error-note@+6   2 {{you can disable this diagnostic with '-Wno-openacc-omp-map-present'}}
  // error-note@+5   2 {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  // warn-warning@+4 2 {{the OpenACC 'present' clause translation uses the 'present' map type modifier}}
  // warn-note@+3    2 {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-present-omp=no-present'}}
  // warn-note@+2    2 {{you can disable this diagnostic with '-Wno-openacc-omp-map-present'}}
  // warn-note@+1    2 {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc data present(x, y) present(z)
    ;
  // warn-warning@+4 {{the OpenACC 'present' clause translation uses the 'present' map type modifier}}
  // warn-note@+3    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-present-omp=no-present'}}
  // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-map-present'}}
  // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc parallel present(x)
    ;
  // warn-warning@+4 {{the OpenACC 'present' clause translation uses the 'present' map type modifier}}
  // warn-note@+3    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-present-omp=no-present'}}
  // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-map-present'}}
  // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc parallel loop present(x)
  for (int i = 0; i < 5; ++i)
    ;
  return 0;
}
