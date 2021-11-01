// Check when -Wopenacc-omp-update-present is enabled and when it's an error.

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
// RUN:   (warn-opt=                                      present=%[none] )
// RUN:   (warn-opt=-Wopenacc-omp-update-present          present=%[warn] )
// RUN:   (warn-opt=-Wno-error=openacc-omp-update-present present=%[noerr])
// RUN:   (warn-opt=-Werror=openacc-omp-update-present    present=error   )
// RUN:   (warn-opt=-Wno-openacc-omp-update-present       present=none    )
// RUN: }
// RUN: %data present-opts {
// RUN:   (present-opt=                                        verify=%[present])
// RUN:   (present-opt=-fopenacc-update-present-omp=present    verify=%[present])
// RUN:   (present-opt=-fopenacc-update-present-omp=no-present verify=none      )
// RUN: }
// RUN: %for prt-opts {
// RUN:   %for warn-opts {
// RUN:     %for present-opts {
// RUN:       %clang %[prt-opt] %[warn-opt] %[present-opt] %s \
// RUN:              -Xclang -verify=%[verify]
// RUN:     }
// RUN:   }
// RUN: }

// END.

// none-no-diagnostics

int main() {
  int w, x, y, z;
  // error-error@+10 1 {{the OpenACC 'self' clause translation uses the 'present' motion modifier}}
  // error-error@+9  1 {{the OpenACC 'host' clause translation uses the 'present' motion modifier}}
  // error-error@+8  1 {{the OpenACC 'device' clause translation uses the 'present' motion modifier}}
  // error-note@+7   3 {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-update-present-omp=no-present'}}
  // error-note@+6   3 {{or you can just disable this diagnostic with '-Wno-openacc-omp-update-present'}}
  // warn-warning@+5 1 {{the OpenACC 'self' clause translation uses the 'present' motion modifier}}
  // warn-warning@+4 1 {{the OpenACC 'host' clause translation uses the 'present' motion modifier}}
  // warn-warning@+3 1 {{the OpenACC 'device' clause translation uses the 'present' motion modifier}}
  // warn-note@+2    3 {{an alternative OpenMP translation can be specified with, for example, '-fopenacc-update-present-omp=no-present'}}
  // warn-note@+1    3 {{or you can just disable this diagnostic with '-Wno-openacc-omp-update-present'}}
  #pragma acc update self(w) host(x, y) device(z)
  #pragma acc update self(w) host(x, y) device(z) if_present
  return 0;
}
