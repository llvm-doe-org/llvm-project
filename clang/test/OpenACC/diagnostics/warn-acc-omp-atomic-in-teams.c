// Check when -Wopenacc-omp-atomic-in-teams is enabled and when it's an error.

// DEFINE: %{check-warn-opt}( CFLAGS %, VERIFY %) =                            \
// DEFINE:   : '------ VERIFY=%{VERIFY}; CFLAGS=%{CFLAGS} -------'          && \
// DEFINE:   %clang %{CFLAGS} -Xclang -verify=%{VERIFY} %s > /dev/null

// DEFINE: %{check-warn-opts}( CFLAGS %, NONE %, WARN %, NOERR %) =            \
//                              CFLAGS                                              VERIFY
// DEFINE:   %{check-warn-opt}( %{CFLAGS}                                        %, %{NONE}  %) && \
// DEFINE:   %{check-warn-opt}( %{CFLAGS} -Wopenacc-omp-atomic-in-teams          %, %{WARN}  %) && \
// DEFINE:   %{check-warn-opt}( %{CFLAGS} -Wopenacc-omp-ext                      %, %{WARN}  %) && \
// DEFINE:   %{check-warn-opt}( %{CFLAGS} -Wno-error=openacc-omp-atomic-in-teams %, %{NOERR} %) && \
// DEFINE:   %{check-warn-opt}( %{CFLAGS} -Wno-error=openacc-omp-ext             %, %{NOERR} %) && \
// DEFINE:   %{check-warn-opt}( %{CFLAGS} -Werror=openacc-omp-atomic-in-teams    %, error    %) && \
// DEFINE:   %{check-warn-opt}( %{CFLAGS} -Werror=openacc-omp-ext                %, error    %) && \
// DEFINE:   %{check-warn-opt}( %{CFLAGS} -Wno-openacc-omp-atomic-in-teams       %, none     %) && \
// DEFINE:   %{check-warn-opt}( %{CFLAGS} -Wno-openacc-omp-ext                   %, none     %)    

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
  int v, x;

  // When the diagnostic is an error, translation of remaining OpenACC to OpenMP
  // is suppressed, so we don't see the error again.
  #pragma acc parallel
  // error-error@+6  {{the '#pragma acc atomic' translation produces a '#pragma omp atomic' strictly nested within a '#pragma omp target teams', but that nesting is an OpenMP extension that might not be supported yet by other compilers}}
  // error-note@+5   {{you can disable this diagnostic with '-Wno-openacc-omp-atomic-in-teams'}}
  // error-note@+4   {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  // warn-warning@+3 {{the '#pragma acc atomic' translation produces a '#pragma omp atomic' strictly nested within a '#pragma omp target teams', but that nesting is an OpenMP extension that might not be supported yet by other compilers}}
  // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-atomic-in-teams'}}
  // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc atomic read
  v = x;

  #pragma acc parallel
  {
    // warn-warning@+3 {{the '#pragma acc atomic' translation produces a '#pragma omp atomic' strictly nested within a '#pragma omp target teams', but that nesting is an OpenMP extension that might not be supported yet by other compilers}}
    // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-atomic-in-teams'}}
    // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
    #pragma acc atomic write
    x = 5;
  }

  #pragma acc parallel loop seq
  for (int i = 0; i < 5; ++i) {
    // warn-warning@+3 {{the '#pragma acc atomic' translation produces a '#pragma omp atomic' strictly nested within a '#pragma omp target teams', but that nesting is an OpenMP extension that might not be supported yet by other compilers}}
    // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-atomic-in-teams'}}
    // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
    #pragma acc atomic update
    ++x;
  }

  #pragma acc parallel
  {
    #pragma acc loop seq
    for (int i = 0; i < 5; ++i) {
      // warn-warning@+3 {{the '#pragma acc atomic' translation produces a '#pragma omp atomic' strictly nested within a '#pragma omp target teams', but that nesting is an OpenMP extension that might not be supported yet by other compilers}}
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
