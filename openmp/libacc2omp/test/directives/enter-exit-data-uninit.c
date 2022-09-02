// Check the case where "acc enter data" or "acc exit data" is encountered
// before runtime initialization.

// DEFINE: %{check}( DIR %, CLAUSE %) =                                        \
// DEFINE:   %clang-acc -DDIR=%{DIR} -DCLAUSE=%{CLAUSE} -o %t.exe %s &&        \
// DEFINE:   %t.exe > %t.out 2>&1 &&                                           \
// DEFINE:   FileCheck -input-file %t.out -allow-empty -check-prefixes=EXE %s

// RUN: %{check}( enter %, copyin  %)
// RUN: %{check}( enter %, create  %)
// RUN: %{check}( exit  %, copyout %)
// RUN: %{check}( exit  %, delete  %)

// END.

// expected-no-diagnostics

#include <stdio.h>

int main() {
  int i;
  // EXE-NOT: {{.}}
  #pragma acc DIR data CLAUSE(i)
  return 0;
}
