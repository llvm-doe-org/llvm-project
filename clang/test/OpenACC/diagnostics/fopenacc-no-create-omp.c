// Check bad -fopenacc-no-create-omp values.
//
// Diagnostics about ompx_no_alloc in the translation are checked in
// diagnostics/warn-acc-omp-map-ompx-no-alloc.c.  The behavior of no_create
// clauses for different values of -fopenacc-no-create-omp is checked in
// directives/Tests/no-create.c.

// DEFINE: %{check-cmd}( CMD%, VAL %) =                                        \
// DEFINE:   not %{CMD} -fopenacc-no-create-omp=%{VAL} %s 2>&1 |               \
// DEFINE:   FileCheck -check-prefix=BAD-VAL -DVAL=%{VAL} %s

// DEFINE: %{check-cmds}( VAL %) =                                             \
// DEFINE:   %{check-cmd}( %clang -fopenacc     %, %{VAL} %)                && \
// DEFINE:   %{check-cmd}( %clang_cc1 -fopenacc %, %{VAL} %)                && \
// DEFINE:   %{check-cmd}( %clang               %, %{VAL} %)                && \
// DEFINE:   %{check-cmd}( %clang_cc1           %, %{VAL} %)

// RUN: %{check-cmds}( foo %)
// RUN: %{check-cmds}(     %)

// BAD-VAL: error: invalid value '[[VAL]]' in '-fopenacc-no-create-omp=[[VAL]]'
