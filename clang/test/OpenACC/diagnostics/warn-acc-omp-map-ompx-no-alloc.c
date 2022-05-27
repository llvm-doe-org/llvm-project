// Check when -Wopenacc-omp-map-ompx-no-alloc is enabled and when it's an error.
//
// Diagnostics about bad -fopenacc-no-create-omp arguments are checked in
// diagnostics/fopenacc-no-create-omp.c.  The behavior of no_create clauses for
// different values of -fopenacc-no-create-omp is checked in
// directives/Tests/no-create.c.

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
// RUN:   (warn-opt=                                         no-alloc=%[none] )
// RUN:   (warn-opt=-Wopenacc-omp-map-ompx-no-alloc          no-alloc=%[warn] )
// RUN:   (warn-opt=-Wopenacc-omp-ext                        no-alloc=%[warn] )
// RUN:   (warn-opt=-Wno-error=openacc-omp-map-ompx-no-alloc no-alloc=%[noerr])
// RUN:   (warn-opt=-Wno-error=openacc-omp-ext               no-alloc=%[noerr])
// RUN:   (warn-opt=-Werror=openacc-omp-map-ompx-no-alloc    no-alloc=error   )
// RUN:   (warn-opt=-Werror=openacc-omp-ext                  no-alloc=error   )
// RUN:   (warn-opt=-Wno-openacc-omp-map-ompx-no-alloc       no-alloc=none    )
// RUN:   (warn-opt=-Wno-openacc-omp-ext                     no-alloc=none    )
// RUN: }
// RUN: %data no-create-opts {
// RUN:   (no-create-opt=                                         verify=%[no-alloc])
// RUN:   (no-create-opt=-fopenacc-no-create-omp=ompx-no-alloc    verify=%[no-alloc])
// RUN:   (no-create-opt=-fopenacc-no-create-omp=no-ompx-no-alloc verify=none       )
// RUN: }
// RUN: %for prt-opts {
// RUN:   %for warn-opts {
// RUN:     %for no-create-opts {
// RUN:       %clang %[prt-opt] %[warn-opt] %[no-create-opt] %s \
// RUN:              -Xclang -verify=%[verify] -Wno-openacc-omp-map-ompx-hold
// RUN:     }
// RUN:   }
// RUN: }

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
