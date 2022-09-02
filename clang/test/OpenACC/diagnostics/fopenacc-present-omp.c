// Check bad -fopenacc-present-omp values.
//
// Diagnostics about the present map type modifier in the translation are
// checked in diagnostics/warn-acc-omp-map-present.c.  The behavior of present
// clauses for different values of -fopenacc-present-omp is checked in
// directives/Tests/present.c.

// DEFINE: %{check-cmd}( CMD %, VAL %) =                                       \
// DEFINE:   not %{CMD} -fopenacc-present-omp=%{VAL} %s 2>&1 |                 \
// DEFINE:   FileCheck -check-prefix=BAD-VAL -DVAL=%{VAL} %s

// DEFINE: %{check-cmds}( VAL %) =                                             \
// DEFINE:   %{check-cmd}( %clang -fopenacc     %, %{VAL} %)                && \
// DEFINE:   %{check-cmd}( %clang_cc1 -fopenacc %, %{VAL} %)                && \
// DEFINE:   %{check-cmd}( %clang               %, %{VAL} %)                && \
// DEFINE:   %{check-cmd}( %clang_cc1           %, %{VAL} %)

// RUN: %{check-cmds}( foo %)
// RUN: %{check-cmds}(     %)

// BAD-VAL: error: invalid value '[[VAL]]' in '-fopenacc-present-omp=[[VAL]]'
