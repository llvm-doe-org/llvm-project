// Check when -Wopenacc-omp-map-hold is enabled and when it's an error.

// RUN: %data prt-opts {
//        # Default is error.
// RUN:   (prt-opt=-fopenacc-ast-print=omp     none=error warn=error noerr=warn)
// RUN:   (prt-opt=-fopenacc-ast-print=acc-omp none=error warn=error noerr=warn)
// RUN:   (prt-opt=-fopenacc-ast-print=omp-acc none=error warn=error noerr=warn)
// RUN:   (prt-opt=-fopenacc-print=omp         none=error warn=error noerr=warn)
// RUN:   (prt-opt=-fopenacc-print=acc-omp     none=error warn=error noerr=warn)
// RUN:   (prt-opt=-fopenacc-print=omp-acc     none=error warn=error noerr=warn)
//        # Default is no warning.
// RUN:   (prt-opt=-fopenacc-ast-print=acc     none=none  warn=warn  noerr=none)
// RUN:   (prt-opt=-fopenacc-print=acc         none=none  warn=warn  noerr=none)
// RUN:   (prt-opt='-c -fopenacc'              none=none  warn=warn  noerr=none)
// RUN: }
// RUN: %data warn-opts {
// RUN:   (warn-opt=                                hold=%[none] )
// RUN:   (warn-opt=-Wopenacc-omp-map-hold          hold=%[warn] )
// RUN:   (warn-opt=-Wno-error=openacc-omp-map-hold hold=%[noerr])
// RUN:   (warn-opt=-Werror=openacc-omp-map-hold    hold=error   )
// RUN:   (warn-opt=-Wno-openacc-omp-map-hold       hold=none    )
// RUN: }
// RUN: %data ref-count-opts {
// RUN:   (ref-count-opt=                                           verify=%[hold])
// RUN:   (ref-count-opt=-fopenacc-structured-ref-count-omp=hold    verify=%[hold])
// RUN:   (ref-count-opt=-fopenacc-structured-ref-count-omp=no-hold verify=none   )
// RUN: }
// RUN: %for prt-opts {
// RUN:   %for warn-opts {
// RUN:     %for ref-count-opts {
// RUN:       %clang %[prt-opt] %[warn-opt] %[ref-count-opt] %s \
// RUN:              -Xclang -verify=%[verify] -Wno-openacc-omp-map-present \
// RUN:              -Wno-openacc-omp-map-no-alloc
// RUN:     }
// RUN:   }
// RUN: }

// END.

// none-no-diagnostics

int main() {
  int x, y, z;

  //--------------------------------------------------
  // present
  //--------------------------------------------------

  // error-error@+6  2 {{the OpenACC 'present' clause translation uses the 'hold' map type modifier}}
  // error-note@+5   2 {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-hold'}}
  // error-note@+4   2 {{or you can just disable this diagnostic with '-Wno-openacc-omp-map-hold'}}
  // warn-warning@+3 2 {{the OpenACC 'present' clause translation uses the 'hold' map type modifier}}
  // warn-note@+2    2 {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-hold'}}
  // warn-note@+1    2 {{or you can just disable this diagnostic with '-Wno-openacc-omp-map-hold'}}
  #pragma acc data present(x, y) present(z)
  // nomap translation here should not use hold and so should not warn.
  #pragma acc parallel
  x = 5;
  // warn-warning@+3 {{the OpenACC 'present' clause translation uses the 'hold' map type modifier}}
  // warn-note@+2    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-hold'}}
  // warn-note@+1    {{or you can just disable this diagnostic with '-Wno-openacc-omp-map-hold'}}
  #pragma acc parallel present(x)
  ;
  // warn-warning@+3 {{the OpenACC 'present' clause translation uses the 'hold' map type modifier}}
  // warn-note@+2    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-hold'}}
  // warn-note@+1    {{or you can just disable this diagnostic with '-Wno-openacc-omp-map-hold'}}
  #pragma acc parallel loop present(x)
  for (int i = 0; i < 5; ++i)
    ;

  //--------------------------------------------------
  // copy
  //--------------------------------------------------

  // warn-warning@+3 2 {{the OpenACC 'copy' clause translation uses the 'hold' map type modifier}}
  // warn-note@+2    2 {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-hold'}}
  // warn-note@+1    2 {{or you can just disable this diagnostic with '-Wno-openacc-omp-map-hold'}}
  #pragma acc data copy(x, y) copy(z)
  // nomap translation here should not use hold and so should not warn.
  #pragma acc parallel
  x = 5;
  // warn-warning@+3 {{the OpenACC 'copy' clause translation uses the 'hold' map type modifier}}
  // warn-note@+2    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-hold'}}
  // warn-note@+1    {{or you can just disable this diagnostic with '-Wno-openacc-omp-map-hold'}}
  #pragma acc parallel copy(x)
  ;
  // warn-warning@+3 {{the OpenACC 'copy' clause translation uses the 'hold' map type modifier}}
  // warn-note@+2    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-hold'}}
  // warn-note@+1    {{or you can just disable this diagnostic with '-Wno-openacc-omp-map-hold'}}
  #pragma acc parallel loop copy(x)
  for (int i = 0; i < 5; ++i)
    ;

  //--------------------------------------------------
  // copyin
  //--------------------------------------------------

  // warn-warning@+3 2 {{the OpenACC 'copyin' clause translation uses the 'hold' map type modifier}}
  // warn-note@+2    2 {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-hold'}}
  // warn-note@+1    2 {{or you can just disable this diagnostic with '-Wno-openacc-omp-map-hold'}}
  #pragma acc data copyin(x, y) copyin(z)
  // nomap translation here should not use hold and so should not warn.
  #pragma acc parallel
  x = 5;
  // warn-warning@+3 {{the OpenACC 'copyin' clause translation uses the 'hold' map type modifier}}
  // warn-note@+2    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-hold'}}
  // warn-note@+1    {{or you can just disable this diagnostic with '-Wno-openacc-omp-map-hold'}}
  #pragma acc parallel copyin(x)
  ;
  // warn-warning@+3 {{the OpenACC 'copyin' clause translation uses the 'hold' map type modifier}}
  // warn-note@+2    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-hold'}}
  // warn-note@+1    {{or you can just disable this diagnostic with '-Wno-openacc-omp-map-hold'}}
  #pragma acc parallel loop copyin(x)
  for (int i = 0; i < 5; ++i)
    ;

  //--------------------------------------------------
  // copyout
  //--------------------------------------------------

  // warn-warning@+3 2 {{the OpenACC 'copyout' clause translation uses the 'hold' map type modifier}}
  // warn-note@+2    2 {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-hold'}}
  // warn-note@+1    2 {{or you can just disable this diagnostic with '-Wno-openacc-omp-map-hold'}}
  #pragma acc data copyout(x, y) copyout(z)
  // nomap translation here should not use hold and so should not warn.
  #pragma acc parallel
  x = 5;
  // warn-warning@+3 {{the OpenACC 'copyout' clause translation uses the 'hold' map type modifier}}
  // warn-note@+2    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-hold'}}
  // warn-note@+1    {{or you can just disable this diagnostic with '-Wno-openacc-omp-map-hold'}}
  #pragma acc parallel copyout(x)
  ;
  // warn-warning@+3 {{the OpenACC 'copyout' clause translation uses the 'hold' map type modifier}}
  // warn-note@+2    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-hold'}}
  // warn-note@+1    {{or you can just disable this diagnostic with '-Wno-openacc-omp-map-hold'}}
  #pragma acc parallel loop copyout(x)
  for (int i = 0; i < 5; ++i)
    ;

  //--------------------------------------------------
  // create
  //--------------------------------------------------

  // warn-warning@+3 2 {{the OpenACC 'create' clause translation uses the 'hold' map type modifier}}
  // warn-note@+2    2 {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-hold'}}
  // warn-note@+1    2 {{or you can just disable this diagnostic with '-Wno-openacc-omp-map-hold'}}
  #pragma acc data create(x, y) create(z)
  // nomap translation here should not use hold and so should not warn.
  #pragma acc parallel
  x = 5;
  // warn-warning@+3 {{the OpenACC 'create' clause translation uses the 'hold' map type modifier}}
  // warn-note@+2    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-hold'}}
  // warn-note@+1    {{or you can just disable this diagnostic with '-Wno-openacc-omp-map-hold'}}
  #pragma acc parallel create(x)
  ;
  // warn-warning@+3 {{the OpenACC 'create' clause translation uses the 'hold' map type modifier}}
  // warn-note@+2    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-hold'}}
  // warn-note@+1    {{or you can just disable this diagnostic with '-Wno-openacc-omp-map-hold'}}
  #pragma acc parallel loop create(x)
  for (int i = 0; i < 5; ++i)
    ;

  //--------------------------------------------------
  // no_create
  //--------------------------------------------------

  // warn-warning@+3 2 {{the OpenACC 'no_create' clause translation uses the 'hold' map type modifier}}
  // warn-note@+2    2 {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-hold'}}
  // warn-note@+1    2 {{or you can just disable this diagnostic with '-Wno-openacc-omp-map-hold'}}
  #pragma acc data no_create(x, y) no_create(z)
  // nomap translation here should not use 'hold' and so should not warn.
  #pragma acc parallel
  x = 5;
  // warn-warning@+3 {{the OpenACC 'no_create' clause translation uses the 'hold' map type modifier}}
  // warn-note@+2    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-hold'}}
  // warn-note@+1    {{or you can just disable this diagnostic with '-Wno-openacc-omp-map-hold'}}
  #pragma acc parallel no_create(x)
  ;
  // warn-warning@+3 {{the OpenACC 'no_create' clause translation uses the 'hold' map type modifier}}
  // warn-note@+2    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-structured-ref-count-omp=no-hold'}}
  // warn-note@+1    {{or you can just disable this diagnostic with '-Wno-openacc-omp-map-hold'}}
  #pragma acc parallel loop no_create(x)
  for (int i = 0; i < 5; ++i)
    ;

  return 0;
}
