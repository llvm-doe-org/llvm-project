// Check the case where "acc enter data" or "acc exit data" is encountered
// before runtime initialization.

// RUN: %data dirs {
// RUN:   (dir=enter clause=copyin )
// RUN:   (dir=enter clause=create )
// RUN:   (dir=exit  clause=copyout)
// RUN:   (dir=exit  clause=delete )
// RUN: }
// RUN: %for dirs {
// RUN:   %clang-acc -DDIR=%[dir] -DCLAUSE=%[clause] -o %t.exe %s
// RUN:   %t.exe > %t.out 2>&1
// RUN:   FileCheck -input-file %t.out -allow-empty -check-prefixes=EXE %s
// RUN: }

// END.

// expected-no-diagnostics

#include <stdio.h>

int main() {
  int i;
  // EXE-NOT: {{.}}
  #pragma acc DIR data CLAUSE(i)
  return 0;
}
