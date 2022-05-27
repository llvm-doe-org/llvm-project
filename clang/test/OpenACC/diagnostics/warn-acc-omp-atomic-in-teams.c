// Check when -Wopenacc-omp-atomic-in-teams is enabled and when it's an error.
//
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
// RUN:   (warn-opt=                                       verify=%[none] )
// RUN:   (warn-opt=-Wopenacc-omp-atomic-in-teams          verify=%[warn] )
// RUN:   (warn-opt=-Wopenacc-omp-ext                      verify=%[warn] )
// RUN:   (warn-opt=-Wno-error=openacc-omp-atomic-in-teams verify=%[noerr])
// RUN:   (warn-opt=-Wno-error=openacc-omp-ext             verify=%[noerr])
// RUN:   (warn-opt=-Werror=openacc-omp-atomic-in-teams    verify=error   )
// RUN:   (warn-opt=-Werror=openacc-omp-ext                verify=error   )
// RUN:   (warn-opt=-Wno-openacc-omp-atomic-in-teams       verify=none    )
// RUN:   (warn-opt=-Wno-openacc-omp-ext                   verify=none    )
// RUN: }
// RUN: %for prt-opts {
// RUN:   %for warn-opts {
// RUN:     %clang %[prt-opt] %[warn-opt] -Xclang -verify=%[verify] %s
// RUN:   }
// RUN: }

// END.

// none-no-diagnostics

int main() {
  int v, x;

  #pragma acc parallel
  // error-error@+6  {{the '#pragma acc atomic' translation produces a '#pragma omp atomic' strictly nested within an '#pragma omp target teams', but that nesting is an OpenMP extension that might not be supported yet by other compilers}}
  // error-note@+5   {{you can disable this diagnostic with '-Wno-openacc-omp-atomic-in-teams'}}
  // error-note@+4   {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  // warn-warning@+3 {{the '#pragma acc atomic' translation produces a '#pragma omp atomic' strictly nested within an '#pragma omp target teams', but that nesting is an OpenMP extension that might not be supported yet by other compilers}}
  // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-atomic-in-teams'}}
  // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc atomic read
  v = x;

  #pragma acc parallel
  {
    // warn-warning@+3 {{the '#pragma acc atomic' translation produces a '#pragma omp atomic' strictly nested within an '#pragma omp target teams', but that nesting is an OpenMP extension that might not be supported yet by other compilers}}
    // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-atomic-in-teams'}}
    // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
    #pragma acc atomic write
    x = 5;
  }

  #pragma acc parallel loop seq
  for (int i = 0; i < 5; ++i) {
    // warn-warning@+3 {{the '#pragma acc atomic' translation produces a '#pragma omp atomic' strictly nested within an '#pragma omp target teams', but that nesting is an OpenMP extension that might not be supported yet by other compilers}}
    // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-atomic-in-teams'}}
    // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
    #pragma acc atomic update
    ++x;
  }

  #pragma acc parallel
  {
    #pragma acc loop seq
    for (int i = 0; i < 5; ++i) {
      // warn-warning@+3 {{the '#pragma acc atomic' translation produces a '#pragma omp atomic' strictly nested within an '#pragma omp target teams', but that nesting is an OpenMP extension that might not be supported yet by other compilers}}
      // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-atomic-in-teams'}}
      // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
      #pragma acc atomic capture
      v = ++x;
    }
  }

  #pragma acc parallel
  {
    #pragma acc loop
    for (int i = 0; i < 5; ++i) {
      #pragma acc atomic capture
      { v = x; ++x; }
    }
  }

  return 0;
}
