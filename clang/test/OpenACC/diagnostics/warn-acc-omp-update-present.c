// Check when -Wopenacc-omp-update-present is enabled and when it's an error.
//
// Diagnostics about bad -fopenacc-update-present-omp arguments are checked in
// diagnostics/fopenacc-update-present-omp.c.  The behavior of the update
// directive for different values of -fopenacc-update-present-omp is checked in
// directives/Tests/update.c.

// DEFINE: %{check-present-opt}( CFLAGS %, VERIFY %) =                         \
// DEFINE:   : '------ VERIFY=%{VERIFY}; CFLAGS=%{CFLAGS} -------'          && \
// DEFINE:   %clang %{CFLAGS} -Xclang -verify=%{VERIFY} %s > /dev/null

// DEFINE: %{check-present-opts}( CFLAGS %, PRESENT %) =                                                 \
//                                 CFLAGS                                               VERIFY
// DEFINE:   %{check-present-opt}( %{CFLAGS}                                         %, %{PRESENT} %) && \
// DEFINE:   %{check-present-opt}( %{CFLAGS} -fopenacc-update-present-omp=present    %, %{PRESENT} %) && \
// DEFINE:   %{check-present-opt}( %{CFLAGS} -fopenacc-update-present-omp=no-present %, none       %)

// DEFINE: %{check-warn-opts}( CFLAGS %, NONE %, WARN %, NOERR %) =                                   \
//                                  CFLAGS                                             PRESENT
// DEFINE:   %{check-present-opts}( %{CFLAGS}                                       %, %{NONE}  %) && \
// DEFINE:   %{check-present-opts}( %{CFLAGS} -Wopenacc-omp-update-present          %, %{WARN}  %) && \
// DEFINE:   %{check-present-opts}( %{CFLAGS} -Wopenacc-omp-ext                     %, %{WARN}  %) && \
// DEFINE:   %{check-present-opts}( %{CFLAGS} -Wno-error=openacc-omp-update-present %, %{NOERR} %) && \
// DEFINE:   %{check-present-opts}( %{CFLAGS} -Wno-error=openacc-omp-ext            %, %{NOERR} %) && \
// DEFINE:   %{check-present-opts}( %{CFLAGS} -Werror=openacc-omp-update-present    %, error    %) && \
// DEFINE:   %{check-present-opts}( %{CFLAGS} -Werror=openacc-omp-ext               %, error    %) && \
// DEFINE:   %{check-present-opts}( %{CFLAGS} -Wno-openacc-omp-update-present       %, none     %) && \
// DEFINE:   %{check-present-opts}( %{CFLAGS} -Wno-openacc-omp-ext                  %, none     %)

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
  int w, x, y, z;
  // error-error@+12 1 {{the OpenACC 'self' clause translation uses the 'present' motion modifier}}
  // error-error@+11 1 {{the OpenACC 'host' clause translation uses the 'present' motion modifier}}
  // error-error@+10 1 {{the OpenACC 'device' clause translation uses the 'present' motion modifier}}
  // error-note@+9   3 {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-update-present-omp=no-present'}}
  // error-note@+8   3 {{you can disable this diagnostic with '-Wno-openacc-omp-update-present'}}
  // error-note@+7   3 {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  // warn-warning@+6 1 {{the OpenACC 'self' clause translation uses the 'present' motion modifier}}
  // warn-warning@+5 1 {{the OpenACC 'host' clause translation uses the 'present' motion modifier}}
  // warn-warning@+4 1 {{the OpenACC 'device' clause translation uses the 'present' motion modifier}}
  // warn-note@+3    3 {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-update-present-omp=no-present'}}
  // warn-note@+2    3 {{you can disable this diagnostic with '-Wno-openacc-omp-update-present'}}
  // warn-note@+1    3 {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc update self(w) host(x, y) device(z)
  #pragma acc update self(w) host(x, y) device(z) if_present
  return 0;
}
