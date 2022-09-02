// DEFINE: %{check}( NOT %, CFLAGS %, FC_OPTS %, EXTRA_CFLAGS %) =             \
// DEFINE:   : '----------- CFLAGS: %{CFLAGS} ------------'                 && \
// DEFINE:   %{NOT} %clang %{CFLAGS} %{EXTRA_CFLAGS} %s > %t.out 2>&1       && \
// DEFINE:   FileCheck %{FC_OPTS} %s -input-file=%t.out

// DEFINE: %{check-warn-opts}( CFLAGS %) =                                                         \
//                     NOT    CFLAGS                                   FC_OPTS
// DEFINE:   %{check}( not %, %{CFLAGS}                            %, -check-prefix=ERROR %, %) && \
// DEFINE:   %{check}(     %, %{CFLAGS} -Wno-error=openacc-and-cxx %, -check-prefix=WARN  %, %) && \
// DEFINE:   %{check}(     %, %{CFLAGS} -Wno-openacc-and-cxx       %, -allow-empty        %, %)

// DEFINE: %{check-acc-opts}( CFLAGS %) =                                      \
// DEFINE:   %{check-warn-opts}( %{CFLAGS} -fopenacc                   %)   && \
// DEFINE:   %{check-warn-opts}( %{CFLAGS} -fopenacc-ast-print=acc     %)   && \
// DEFINE:   %{check-warn-opts}( %{CFLAGS} -fopenacc-ast-print=omp     %)   && \
// DEFINE:   %{check-warn-opts}( %{CFLAGS} -fopenacc-ast-print=acc-omp %)   && \
// DEFINE:   %{check-warn-opts}( %{CFLAGS} -fopenacc-ast-print=omp-acc %)   && \
// DEFINE:   %{check-warn-opts}( %{CFLAGS} -fopenacc-print=acc         %)   && \
// DEFINE:   %{check-warn-opts}( %{CFLAGS} -fopenacc-print=omp         %)   && \
// DEFINE:   %{check-warn-opts}( %{CFLAGS} -fopenacc-print=acc-omp     %)   && \
// DEFINE:   %{check-warn-opts}( %{CFLAGS} -fopenacc-print=omp-acc     %)

// DEFINE: %{check-std}( CFLAGS %) =                                           \
// DEFINE:   %{check}( %, %{CFLAGS} %, -allow-empty %, -o %t.exe %)         && \
// DEFINE:   %{check-acc-opts}( %{CFLAGS} %)

// RUN: %{check-std}(            %)
// RUN: %{check-std}( -std=c++98 %)
// RUN: %{check-std}( -std=c++20 %)

// END.

// Braces prevent these directives from matching themselves when printing.
//  CHECK-NOT: {{error:}} OpenACC support for C++ is highly experimental
//      ERROR: {{error:}} OpenACC support for C++ is highly experimental
//       WARN: {{warning:}} OpenACC support for C++ is highly experimental
// ERROR-NEXT: {{note:}} you can disable this diagnostic with '-Wno-openacc-and-cxx'
//  WARN-NEXT: {{note:}} you can disable this diagnostic with '-Wno-openacc-and-cxx'

int main() {
  return 0;
}
