// Check bad -fopenacc-structured-ref-count-omp values.
//
// Diagnostics about 'ompx_hold' in the translation are checked in
// diagnostics/warn-acc-omp-map-ompx-hold.c.  The behavior of data clauses for
// different values of -fopenacc-structured-ref-count-omp is checked in
// directives/Tests/fopenacc-structured-ref-count-omp.c.

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
// RUN:     not %[cmd] -fopenacc-structured-ref-count-omp=%[val] %s 2>&1 \
// RUN:     | FileCheck -check-prefix=BAD-VAL -DVAL=%[val] %s
// RUN:   }
// RUN: }

// BAD-VAL: error: invalid value '[[VAL]]' in '-fopenacc-structured-ref-count-omp=[[VAL]]'
