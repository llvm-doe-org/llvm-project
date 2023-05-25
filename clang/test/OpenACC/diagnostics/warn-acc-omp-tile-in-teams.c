// Check when -Wopenacc-omp-tile-in-teams is enabled and when it's an error.

// DEFINE: %{check-warn-opt}( CFLAGS %, VERIFY %) =                            \
// DEFINE:   : '------ VERIFY=%{VERIFY}; CFLAGS=%{CFLAGS} -------'          && \
// DEFINE:   %clang %{CFLAGS} -Xclang -verify=%{VERIFY} %s > /dev/null

// DEFINE: %{check-warn-opts}( CFLAGS %, NONE %, WARN %, NOERR %) =            \
//                              CFLAGS                                              VERIFY
// DEFINE:   %{check-warn-opt}( %{CFLAGS}                                        %, %{NONE}  %) && \
// DEFINE:   %{check-warn-opt}( %{CFLAGS} -Wopenacc-omp-tile-in-teams            %, %{WARN}  %) && \
// DEFINE:   %{check-warn-opt}( %{CFLAGS} -Wopenacc-omp-ext                      %, %{WARN}  %) && \
// DEFINE:   %{check-warn-opt}( %{CFLAGS} -Wno-error=openacc-omp-tile-in-teams   %, %{NOERR} %) && \
// DEFINE:   %{check-warn-opt}( %{CFLAGS} -Wno-error=openacc-omp-ext             %, %{NOERR} %) && \
// DEFINE:   %{check-warn-opt}( %{CFLAGS} -Werror=openacc-omp-tile-in-teams      %, error    %) && \
// DEFINE:   %{check-warn-opt}( %{CFLAGS} -Werror=openacc-omp-ext                %, error    %) && \
// DEFINE:   %{check-warn-opt}( %{CFLAGS} -Wno-openacc-omp-tile-in-teams         %, none     %) && \
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
  //----------------------------------------------------------------------------
  // seq
  //----------------------------------------------------------------------------

  // When the diagnostic is an error, translation of remaining OpenACC to OpenMP
  // is suppressed, so we don't see the error again.
  #pragma acc parallel
  // error-error@+6  {{the OpenACC 'tile' clause translation produces a '#pragma omp tile' strictly nested within a '#pragma omp target teams', but that nesting is an OpenMP extension that might not be supported yet by other compilers}}
  // error-note@+5   {{you can disable this diagnostic with '-Wno-openacc-omp-tile-in-teams'}}
  // error-note@+4   {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  // warn-warning@+3 {{the OpenACC 'tile' clause translation produces a '#pragma omp tile' strictly nested within a '#pragma omp target teams', but that nesting is an OpenMP extension that might not be supported yet by other compilers}}
  // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-tile-in-teams'}}
  // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc loop seq tile(2)
  for (int i = 0; i < 8; ++i)
    ;
  // warn-warning@+3 {{the OpenACC 'tile' clause translation produces a '#pragma omp tile' strictly nested within a '#pragma omp target teams', but that nesting is an OpenMP extension that might not be supported yet by other compilers}}
  // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-tile-in-teams'}}
  // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc parallel loop seq tile(2)
  for (int i = 0; i < 8; ++i)
    ;

  //----------------------------------------------------------------------------
  // auto
  //----------------------------------------------------------------------------

  #pragma acc parallel
  // warn-warning@+3 {{the OpenACC 'tile' clause translation produces a '#pragma omp tile' strictly nested within a '#pragma omp target teams', but that nesting is an OpenMP extension that might not be supported yet by other compilers}}
  // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-tile-in-teams'}}
  // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc loop auto tile(2)
  for (int i = 0; i < 8; ++i)
    ;
  // warn-warning@+3 {{the OpenACC 'tile' clause translation produces a '#pragma omp tile' strictly nested within a '#pragma omp target teams', but that nesting is an OpenMP extension that might not be supported yet by other compilers}}
  // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-tile-in-teams'}}
  // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc parallel loop auto tile(2)
  for (int i = 0; i < 8; ++i)
    ;

  //----------------------------------------------------------------------------
  // implicit or explicit gang
  //----------------------------------------------------------------------------

  #pragma acc parallel
  #pragma acc loop tile(2)
  for (int i = 0; i < 8; ++i)
    ;
  #pragma acc parallel loop tile(2)
  for (int i = 0; i < 8; ++i)
    ;
  #pragma acc parallel
  #pragma acc loop gang tile(2)
  for (int i = 0; i < 8; ++i)
    ;
  #pragma acc parallel loop gang tile(2)
  for (int i = 0; i < 8; ++i)
    ;

  //----------------------------------------------------------------------------
  // explicit worker or vector
  //----------------------------------------------------------------------------

  #pragma acc parallel
  #pragma acc loop worker tile(2)
  for (int i = 0; i < 8; ++i)
    ;
  #pragma acc parallel loop vector tile(2)
  for (int i = 0; i < 8; ++i)
    ;

  //----------------------------------------------------------------------------
  // seq or auto with enclosing seq loop
  //----------------------------------------------------------------------------

  #pragma acc parallel
  #pragma acc loop seq
  for (int i = 0; i < 8; ++i)
  // warn-warning@+3 {{the OpenACC 'tile' clause translation produces a '#pragma omp tile' strictly nested within a '#pragma omp target teams', but that nesting is an OpenMP extension that might not be supported yet by other compilers}}
  // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-tile-in-teams'}}
  // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc loop seq tile(2)
  for (int j = 0; j < 8; ++j)
    ;
  #pragma acc parallel loop seq
  for (int i = 0; i < 8; ++i)
  // warn-warning@+3 {{the OpenACC 'tile' clause translation produces a '#pragma omp tile' strictly nested within a '#pragma omp target teams', but that nesting is an OpenMP extension that might not be supported yet by other compilers}}
  // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-tile-in-teams'}}
  // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc loop auto tile(2)
  for (int j = 0; j < 8; ++j)
    ;

  //----------------------------------------------------------------------------
  // seq or auto with enclosing auto loop
  //----------------------------------------------------------------------------

  #pragma acc parallel
  #pragma acc loop auto
  for (int i = 0; i < 8; ++i)
  // warn-warning@+3 {{the OpenACC 'tile' clause translation produces a '#pragma omp tile' strictly nested within a '#pragma omp target teams', but that nesting is an OpenMP extension that might not be supported yet by other compilers}}
  // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-tile-in-teams'}}
  // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc loop auto tile(2)
  for (int j = 0; j < 8; ++j)
    ;
  #pragma acc parallel loop auto
  for (int i = 0; i < 8; ++i)
  // warn-warning@+3 {{the OpenACC 'tile' clause translation produces a '#pragma omp tile' strictly nested within a '#pragma omp target teams', but that nesting is an OpenMP extension that might not be supported yet by other compilers}}
  // warn-note@+2    {{you can disable this diagnostic with '-Wno-openacc-omp-tile-in-teams'}}
  // warn-note@+1    {{you can disable all diagnostics about OpenMP extensions appearing in the translation with '-Wno-openacc-omp-ext'}}
  #pragma acc loop seq tile(2)
  for (int j = 0; j < 8; ++j)
    ;

  //----------------------------------------------------------------------------
  // seq or auto with enclosing worker or vector loop
  //----------------------------------------------------------------------------

  #pragma acc parallel
  #pragma acc loop worker
  for (int i = 0; i < 8; ++i)
  #pragma acc loop seq tile(2)
  for (int j = 0; j < 8; ++j)
    ;
  #pragma acc parallel loop vector
  for (int i = 0; i < 8; ++i)
  #pragma acc loop auto tile(2)
  for (int j = 0; j < 8; ++j)
    ;

  return 0;
}
