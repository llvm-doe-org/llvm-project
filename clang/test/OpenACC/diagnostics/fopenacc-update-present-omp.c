// Check bad -fopenacc-update-present-omp values.
//
// Diagnostics about the present motion modifier in the translation are checked
// in diagnostics/warn-acc-omp-update-present.c.  The behavior of the update
// directive for different values of -fopenacc-update-present-omp is checked in
// directives/Tests/update.c.

// DEFINE: %{check-cmd}( CMD %, VAL %) =                                       \
// DEFINE:   not %{CMD} -fopenacc-update-present-omp=%{VAL} %s                 \
// DEFINE:              > %t.out 2>&1 &&                                       \
// DEFINE:   FileCheck -check-prefix=BAD-VAL -DVAL=%{VAL} -input-file=%t.out %s

// DEFINE: %{check-cmds}( VAL %) =                                             \
// DEFINE:   %{check-cmd}( %clang -fopenacc     %, %{VAL} %)                && \
// DEFINE:   %{check-cmd}( %clang_cc1 -fopenacc %, %{VAL} %)                && \
// DEFINE:   %{check-cmd}( %clang               %, %{VAL} %)                && \
// DEFINE:   %{check-cmd}( %clang_cc1           %, %{VAL} %)

// RUN: %{check-cmds}( foo %)
// RUN: %{check-cmds}(     %)

// BAD-VAL: error: invalid value '[[VAL]]' in '-fopenacc-update-present-omp=[[VAL]]'
