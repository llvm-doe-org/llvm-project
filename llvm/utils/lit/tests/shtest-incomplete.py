# RUN: not %{lit} -j 1 -vv %{inputs}/shtest-incomplete > %t.out
# RUN: FileCheck --input-file %t.out %s
#
# END.

# CHECK-LABEL: UNRESOLVED: shtest-incomplete :: empty.txt
# CHECK:       Test has no run line!

# CHECK-LABEL: UNRESOLVED: shtest-incomplete :: unterminated-run.txt
# CHECK:       Test has unterminated run lines (with '\')

# CHECK: Unresolved Tests (2)
