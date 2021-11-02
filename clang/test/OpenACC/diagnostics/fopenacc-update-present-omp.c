// Check bad -fopenacc-update-present-omp values.
//
// Diagnostics about the present motion modifier in the translation are checked
// in diagnostics/warn-acc-omp-update-present.c.  The behavior of the update
// directive for different values of -fopenacc-update-present-omp is checked in
// directives/Tests/update.c.

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
// RUN:     not %[cmd] -fopenacc-update-present-omp=%[val] %s 2>&1 \
// RUN:     | FileCheck -check-prefix=BAD-VAL -DVAL=%[val] %s
// RUN:   }
// RUN: }

// BAD-VAL: error: invalid value '[[VAL]]' in '-fopenacc-update-present-omp=[[VAL]]'
