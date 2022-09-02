// Check when -Wopenacc-omp-map-ompx-hold is enabled and when it's an error.
//
// Diagnostics about bad -fopenacc-structured-ref-count-omp arguments are
// checked in diagnostics/fopenacc-structured-ref-count-omp.c.  The behavior of
// data clauses for different values of -fopenacc-structured-ref-count-omp is
// checked in directives/Tests/fopenacc-structured-ref-count-omp.c.

// DEFINE: %{check-ref-count-opt}( CFLAGS %, VERIFY %) =                       \
// DEFINE:   : '------ VERIFY=%{VERIFY}; CFLAGS=%{CFLAGS} -------'          && \
// DEFINE:   %clang %{CFLAGS} -Xclang -verify=%{VERIFY} %s > /dev/null         \
// DEFINE:       -Wno-openacc-omp-map-present -Wno-openacc-omp-map-ompx-no-alloc

// DEFINE: %{check-ref-count-opts}( CFLAGS %, HOLD %) =                                                         \
//                                   CFLAGS                                                       VERIFY
// DEFINE:   %{check-ref-count-opt}( %{CFLAGS}                                                 %, %{HOLD} %) && \
// DEFINE:   %{check-ref-count-opt}( %{CFLAGS} -fopenacc-structured-ref-count-omp=ompx-hold    %, %{HOLD} %) && \
// DEFINE:   %{check-ref-count-opt}( %{CFLAGS} -fopenacc-structured-ref-count-omp=no-ompx-hold %, none    %)

// DEFINE: %{check-warn-opts}( CFLAGS %, NONE %, WARN %, NOERR %) =                                    \
//                                    CFLAGS                                            HOLD
// DEFINE:   %{check-ref-count-opts}( %{CFLAGS}                                      %, %{NONE}  %) && \
// DEFINE:   %{check-ref-count-opts}( %{CFLAGS} -Wopenacc-omp-map-ompx-hold          %, %{WARN}  %) && \
// DEFINE:   %{check-ref-count-opts}( %{CFLAGS} -Wopenacc-omp-ext                    %, %{WARN}  %) && \
// DEFINE:   %{check-ref-count-opts}( %{CFLAGS} -Wno-error=openacc-omp-map-ompx-hold %, %{NOERR} %) && \
// DEFINE:   %{check-ref-count-opts}( %{CFLAGS} -Wno-error=openacc-omp-ext           %, %{NOERR} %) && \
// DEFINE:   %{check-ref-count-opts}( %{CFLAGS} -Werror=openacc-omp-map-ompx-hold    %, error    %) && \
// DEFINE:   %{check-ref-count-opts}( %{CFLAGS} -Werror=openacc-omp-ext              %, error    %) && \
// DEFINE:   %{check-ref-count-opts}( %{CFLAGS} -Wno-openacc-omp-map-ompx-hold       %, none     %) && \
// DEFINE:   %{check-ref-count-opts}( %{CFLAGS} -Wno-openacc-omp-ext                 %, none     %)

//                          CFLAGS                          NONE      WARN      NOERR
// Default is error.
// RUN: %{check-warn-opts}( -fopenacc-ast-print=omp      %, error  %, error  %, warn %)
// RUN: %{check-warn-opts}( -fopenacc-ast-print=acc-omp  %, error  %, error  %, warn %)
// RUN: %{check-warn-opts}( -fopenacc-ast-print=omp-acc  %, error  %, error  %, warn %)
// RUN: %{check-warn-opts}( -fopenacc-print=omp          %, error  %, error  %, warn %)
// RUN: %{check-warn-opts}( -fopenacc-print=acc-omp      %, error  %, error  %, warn %)
// RUN: %{check-warn-opts}( -fopenacc-print=omp-acc      %, error  %, error  %, warn %)
//
// Default is no warning.
// RUN: %{check-warn-opts}( -fopenacc-ast-print=acc      %, none   %, warn   %, none %)
// RUN: %{check-warn-opts}( -fopenacc-print=acc          %, none   %, warn   %, none %)
// RUN: %{check-warn-opts}( -c -fopenacc                 %, none   %, warn   %, none %)

// END.

// none-no-diagnostics

int main() {
  int x, y, z;

  //--------------------------------------------------
  // present
  //--------------------------------------------------

  // error-error@+8  2 {{the OpenACC 'present' clause translation uses the 'ompx_hold' map type modifier}}
  // error-note@+7   2 {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-ompx-hold'}}
  // error-note@+6   2 {{you can disable this diagnostic with '-Wno-openacc-omp-map-ompx-hold'}}
  // error-note@+5   2 {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  // warn-warning@+4 2 {{the OpenACC 'present' clause translation uses the 'ompx_hold' map type modifier}}
  // warn-note@+3    2 {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-ompx-hold'}}
  // warn-note@+2    2 {{you can disable this diagnostic with '-Wno-openacc-omp-map-ompx-hold'}}
  // warn-note@+1    2 {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc data present(x, y) present(z)
  // nomap translation here should not use ompx_hold and so should not warn.
  #pragma acc parallel
  x = 5;
  // warn-warning@+4 {{the OpenACC 'present' clause translation uses the 'ompx_hold' map type modifier}}
  // warn-note@+3    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-ompx-hold'}}
  // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-map-ompx-hold'}}
  // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc parallel present(x)
  ;
  // warn-warning@+4 {{the OpenACC 'present' clause translation uses the 'ompx_hold' map type modifier}}
  // warn-note@+3    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-ompx-hold'}}
  // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-map-ompx-hold'}}
  // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc parallel loop present(x)
  for (int i = 0; i < 5; ++i)
    ;

  //--------------------------------------------------
  // copy
  //--------------------------------------------------

  // warn-warning@+4 2 {{the OpenACC 'copy' clause translation uses the 'ompx_hold' map type modifier}}
  // warn-note@+3    2 {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-ompx-hold'}}
  // warn-note@+2    2 {{you can disable this diagnostic with '-Wno-openacc-omp-map-ompx-hold'}}
  // warn-note@+1    2 {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc data copy(x, y) copy(z)
  // nomap translation here should not use ompx_hold and so should not warn.
  #pragma acc parallel
  x = 5;
  // warn-warning@+4 {{the OpenACC 'copy' clause translation uses the 'ompx_hold' map type modifier}}
  // warn-note@+3    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-ompx-hold'}}
  // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-map-ompx-hold'}}
  // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc parallel copy(x)
  ;
  // warn-warning@+4 {{the OpenACC 'copy' clause translation uses the 'ompx_hold' map type modifier}}
  // warn-note@+3    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-ompx-hold'}}
  // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-map-ompx-hold'}}
  // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc parallel loop copy(x)
  for (int i = 0; i < 5; ++i)
    ;

  //--------------------------------------------------
  // copyin
  //--------------------------------------------------

  // warn-warning@+4 2 {{the OpenACC 'copyin' clause translation uses the 'ompx_hold' map type modifier}}
  // warn-note@+3    2 {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-ompx-hold'}}
  // warn-note@+2    2 {{you can disable this diagnostic with '-Wno-openacc-omp-map-ompx-hold'}}
  // warn-note@+1    2 {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc data copyin(x, y) copyin(z)
  // nomap translation here should not use ompx_hold and so should not warn.
  #pragma acc parallel
  x = 5;
  // warn-warning@+4 {{the OpenACC 'copyin' clause translation uses the 'ompx_hold' map type modifier}}
  // warn-note@+3    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-ompx-hold'}}
  // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-map-ompx-hold'}}
  // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc parallel copyin(x)
  ;
  // warn-warning@+4 {{the OpenACC 'copyin' clause translation uses the 'ompx_hold' map type modifier}}
  // warn-note@+3    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-ompx-hold'}}
  // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-map-ompx-hold'}}
  // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc parallel loop copyin(x)
  for (int i = 0; i < 5; ++i)
    ;

  //--------------------------------------------------
  // copyout
  //--------------------------------------------------

  // warn-warning@+4 2 {{the OpenACC 'copyout' clause translation uses the 'ompx_hold' map type modifier}}
  // warn-note@+3    2 {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-ompx-hold'}}
  // warn-note@+2    2 {{you can disable this diagnostic with '-Wno-openacc-omp-map-ompx-hold'}}
  // warn-note@+1    2 {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc data copyout(x, y) copyout(z)
  // nomap translation here should not use ompx_hold and so should not warn.
  #pragma acc parallel
  x = 5;
  // warn-warning@+4 {{the OpenACC 'copyout' clause translation uses the 'ompx_hold' map type modifier}}
  // warn-note@+3    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-ompx-hold'}}
  // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-map-ompx-hold'}}
  // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc parallel copyout(x)
  ;
  // warn-warning@+4 {{the OpenACC 'copyout' clause translation uses the 'ompx_hold' map type modifier}}
  // warn-note@+3    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-ompx-hold'}}
  // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-map-ompx-hold'}}
  // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc parallel loop copyout(x)
  for (int i = 0; i < 5; ++i)
    ;

  //--------------------------------------------------
  // create
  //--------------------------------------------------

  // warn-warning@+4 2 {{the OpenACC 'create' clause translation uses the 'ompx_hold' map type modifier}}
  // warn-note@+3    2 {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-ompx-hold'}}
  // warn-note@+2    2 {{you can disable this diagnostic with '-Wno-openacc-omp-map-ompx-hold'}}
  // warn-note@+1    2 {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc data create(x, y) create(z)
  // nomap translation here should not use ompx_hold and so should not warn.
  #pragma acc parallel
  x = 5;
  // warn-warning@+4 {{the OpenACC 'create' clause translation uses the 'ompx_hold' map type modifier}}
  // warn-note@+3    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-ompx-hold'}}
  // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-map-ompx-hold'}}
  // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc parallel create(x)
  ;
  // warn-warning@+4 {{the OpenACC 'create' clause translation uses the 'ompx_hold' map type modifier}}
  // warn-note@+3    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-ompx-hold'}}
  // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-map-ompx-hold'}}
  // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc parallel loop create(x)
  for (int i = 0; i < 5; ++i)
    ;

  //--------------------------------------------------
  // no_create
  //--------------------------------------------------

  // warn-warning@+4 2 {{the OpenACC 'no_create' clause translation uses the 'ompx_hold' map type modifier}}
  // warn-note@+3    2 {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-ompx-hold'}}
  // warn-note@+2    2 {{you can disable this diagnostic with '-Wno-openacc-omp-map-ompx-hold'}}
  // warn-note@+1    2 {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc data no_create(x, y) no_create(z)
  // nomap translation here should not use 'ompx_hold' and so should not warn.
  #pragma acc parallel
  x = 5;
  // warn-warning@+4 {{the OpenACC 'no_create' clause translation uses the 'ompx_hold' map type modifier}}
  // warn-note@+3    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-ompx-hold'}}
  // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-map-ompx-hold'}}
  // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc parallel no_create(x)
  ;
  // warn-warning@+4 {{the OpenACC 'no_create' clause translation uses the 'ompx_hold' map type modifier}}
  // warn-note@+3    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-ompx-hold'}}
  // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-map-ompx-hold'}}
  // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc parallel loop no_create(x)
  for (int i = 0; i < 5; ++i)
    ;

  return 0;
}
