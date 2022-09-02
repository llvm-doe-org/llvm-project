// Clang used to fail an assert when recursively adding implicit routine
// directives if the implicit routine directive info DenseMap happened to
// reallocate storage at that time.
//
// DenseMap.h's getMinBucketToReserveForEntries's current internal documentation
// says a reallocation happens at the 48th entry, so we test that for now.
//
// The main point of this test is to check that the assert fail doesn't happen
// at compile time, but we add dump, print, and execution checks to make sure
// the recursion to add an implicit routine seq to f actually behaved correctly.
//
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}
//
// END.

#include <openacc.h>
#include <stdio.h>

/* expected-no-diagnostics */

//      DMP: FunctionDecl {{.*}} f 'int ()'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Implicit Seq OMPNodeKind=unknown{{$}}
//  DMP-NOT:   OMPDeclareTargetDeclAttr
//      DMP: FunctionDecl {{.*}} g00 'void ()'
//
// PRT-NOT: #pragma acc routine
// PRT-NOT: #pragma omp
//     PRT: int f();
// PRT-NOT: #pragma acc routine
// PRT-NOT: #pragma omp
//     PRT: void g00()
int f();

// First, create 47 entries.
void g00() { f(); }
void g01() { f(); }
void g02() { f(); }
void g03() { f(); }
void g04() { f(); }
void g05() { f(); }
void g06() { f(); }
void g07() { f(); }
void g08() { f(); }
void g09() { f(); }
void g10() { f(); }
void g11() { f(); }
void g12() { f(); }
void g13() { f(); }
void g14() { f(); }
void g15() { f(); }
void g16() { f(); }
void g17() { f(); }
void g18() { f(); }
void g19() { f(); }
void g20() { f(); }
void g21() { f(); }
void g22() { f(); }
void g23() { f(); }
void g24() { f(); }
void g25() { f(); }
void g26() { f(); }
void g27() { f(); }
void g28() { f(); }
void g29() { f(); }
void g30() { f(); }
void g31() { f(); }
void g32() { f(); }
void g33() { f(); }
void g34() { f(); }
void g35() { f(); }
void g36() { f(); }
void g37() { f(); }
void g38() { f(); }
void g39() { f(); }
void g40() { f(); }
void g41() { f(); }
void g42() { f(); }
void g43() { f(); }
void g44() { f(); }
void g45() { f(); }
void g46() { f(); }

// This reuses the existing entry for g00 when adding its implicit routine seq,
// but the recursive call to add an implicit routine seq for f creates the 48th
// entry, which reallocates DenseMap storage, invalidating any locally stored
// reference for g00's entry.
#pragma acc routine seq
void h() { g00(); }

//      DMP: FunctionDecl {{.*}} f 'int ()'
//  DMP-NOT: FunctionDecl
//      DMP:   ACCRoutineDeclAttr {{.*}}> Inherited Implicit Seq OMPNodeKind=unknown{{$}}
//  DMP-NOT:   OMPDeclareTargetDeclAttr
//      DMP: FunctionDecl {{.*}} main 'int ()'
//
//         PRT: void h() {
//         PRT: }
//  PRT-O-NEXT: #pragma omp end declare target
// PRT-AO-NEXT: #pragma omp end declare target
//     PRT-NOT: #pragma acc routine
//     PRT-NOT: #pragma omp
//         PRT: int f() {
//    PRT-NEXT:   return {{.*}};
//    PRT-NEXT: }
//     PRT-NOT: #pragma acc routine
//     PRT-NOT: #pragma omp
//         PRT: int main()
int f() {
  return acc_on_device(acc_device_host);
}

int main() {
  int res = 99;
  #pragma acc parallel num_gangs(1) copyout(res)
  res = f();
  //  EXE-NOT: {{.}}
  // EXE-HOST: f() = 1
  //  EXE-OFF: f() = 0
  //  EXE-NOT: {{.}}
  printf("f() = %d\n", res);
  return 0;
}