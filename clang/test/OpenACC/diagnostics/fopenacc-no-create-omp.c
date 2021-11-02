// Check bad -fopenacc-no-create-omp values.
//
// Diagnostics about ompx_no_alloc in the translation are checked in
// diagnostics/warn-acc-omp-map-ompx-no-alloc.c.  The behavior of no_create
// clauses for different values of -fopenacc-no-create-omp is checked in
// directives/Tests/no-create.c.

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
// RUN:     not %[cmd] -fopenacc-no-create-omp=%[val] %s 2>&1 \
// RUN:     | FileCheck -check-prefix=BAD-VAL -DVAL=%[val] %s
// RUN:   }
// RUN: }

// BAD-VAL: error: invalid value '[[VAL]]' in '-fopenacc-no-create-omp=[[VAL]]'
