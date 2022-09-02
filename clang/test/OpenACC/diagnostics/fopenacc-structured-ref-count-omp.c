// Check bad -fopenacc-structured-ref-count-omp values.
//
// Diagnostics about 'ompx_hold' in the translation are checked in
// diagnostics/warn-acc-omp-map-ompx-hold.c.  The behavior of data clauses for
// different values of -fopenacc-structured-ref-count-omp is checked in
// directives/Tests/fopenacc-structured-ref-count-omp.c.

// DEFINE: %{check-cmd}( CMD %, VAL %) =                                       \
// DEFINE:   not %{CMD} -fopenacc-structured-ref-count-omp=%{VAL} %s           \
// DEFINE:              > %t.out 2>&1 &&                                       \
// DEFINE:   FileCheck -check-prefix=BAD-VAL -DVAL=%{VAL} -input-file=%t.out %s

// DEFINE: %{check-cmds}( VAL %) =                                             \
// DEFINE:   %{check-cmd}( %clang -fopenacc     %, %{VAL} %)                && \
// DEFINE:   %{check-cmd}( %clang_cc1 -fopenacc %, %{VAL} %)                && \
// DEFINE:   %{check-cmd}( %clang               %, %{VAL} %)                && \
// DEFINE:   %{check-cmd}( %clang_cc1           %, %{VAL} %)

// RUN: %{check-cmds}( foo %)
// RUN: %{check-cmds}(     %)

// BAD-VAL: error: invalid value '[[VAL]]' in '-fopenacc-structured-ref-count-omp=[[VAL]]'
