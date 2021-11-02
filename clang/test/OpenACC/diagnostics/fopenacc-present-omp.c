// Check bad -fopenacc-present-omp values.
//
// Diagnostics about the present map type modifier in the translation are
// checked in diagnostics/warn-acc-omp-map-present.c.  The behavior of present
// clauses for different values of -fopenacc-present-omp is checked in
// directives/Tests/present.c.

// RUN: %data bad-vals {
// RUN:   (val=foo)
// RUN:   (val=   )
// RUN: }
// RUN: %data bad-vals-cmds {
// RUN:   (cmd='%clang -fopenacc'    )
// RUN:   (cmd='%clang_cc1 -fopenacc')
// RUN:   (cmd='%clang'              )
// RUN:   (cmd='%clang_cc1'          )
// RUN: }
// RUN: %for bad-vals {
// RUN:   %for bad-vals-cmds {
// RUN:     not %[cmd] -fopenacc-present-omp=%[val] %s 2>&1 \
// RUN:     | FileCheck -check-prefix=BAD-VAL -DVAL=%[val] %s
// RUN:   }
// RUN: }

// BAD-VAL: error: invalid value '[[VAL]]' in '-fopenacc-present-omp=[[VAL]]'
