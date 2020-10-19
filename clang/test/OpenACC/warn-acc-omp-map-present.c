// Check when -Wopenacc-omp-map-present is enabled and when it's an error.

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
// RUN:   (warn-opt=                                   present=%[none] )
// RUN:   (warn-opt=-Wopenacc-omp-map-present          present=%[warn] )
// RUN:   (warn-opt=-Wno-error=openacc-omp-map-present present=%[noerr])
// RUN:   (warn-opt=-Werror=openacc-omp-map-present    present=error   )
// RUN:   (warn-opt=-Wno-openacc-omp-map-present       present=none    )
// RUN: }
// RUN: %data present-opts {
// RUN:   (present-opt=                                 verify=%[present])
// RUN:   (present-opt=-fopenacc-present-omp=present    verify=%[present])
// RUN:   (present-opt=-fopenacc-present-omp=no-present verify=none      )
// RUN: }
// RUN: %for prt-opts {
// RUN:   %for warn-opts {
// RUN:     %for present-opts {
// RUN:       %clang %[prt-opt] %[warn-opt] %[present-opt] %s \
// RUN:              -Xclang -verify=%[verify] -Wno-openacc-omp-map-hold
// RUN:     }
// RUN:   }
// RUN: }

// END.

// none-no-diagnostics

int main() {
  int x, y, z;
  // error-error@+6  2 {{the OpenACC 'present' clause translation uses the 'present' map type modifier}}
  // error-note@+5   2 {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-present-omp=no-present'}}
  // error-note@+4   2 {{or you can just disable this diagnostic with '-Wno-openacc-omp-map-present'}}
  // warn-warning@+3 2 {{the OpenACC 'present' clause translation uses the 'present' map type modifier}}
  // warn-note@+2    2 {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-present-omp=no-present'}}
  // warn-note@+1    2 {{or you can just disable this diagnostic with '-Wno-openacc-omp-map-present'}}
  #pragma acc data present(x, y) present(z)
    ;
  // warn-warning@+3 {{the OpenACC 'present' clause translation uses the 'present' map type modifier}}
  // warn-note@+2    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-present-omp=no-present'}}
  // warn-note@+1    {{or you can just disable this diagnostic with '-Wno-openacc-omp-map-present'}}
  #pragma acc parallel present(x)
    ;
  // warn-warning@+3 {{the OpenACC 'present' clause translation uses the 'present' map type modifier}}
  // warn-note@+2    {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-present-omp=no-present'}}
  // warn-note@+1    {{or you can just disable this diagnostic with '-Wno-openacc-omp-map-present'}}
  #pragma acc parallel loop present(x)
  for (int i = 0; i < 5; ++i)
    ;
  return 0;
}
