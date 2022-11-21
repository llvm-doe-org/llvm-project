// Check implicit and explicit data attributes for member expressions on
// "acc parallel", but only check cases that are not covered by
// parallel-da-member-expr.c because they are specific to C++.

// RUN: %{acc-check-dmp-cxx}
// RUN: %{acc-check-prt-cxx}
// RUN: %{acc-check-exe-cxx}

// END.

/* expected-no-diagnostics */

#include <stdio.h>

class C {
public:
  int x;
  int y;
  int z;
  void fn() { x += 10; y += 20; z += 30; }
};

int main() {
  class C obj;

  //----------------------------------------------------------------------------
  // Check that calling a member function of a class produces an implicit data
  // attribute for the struct.
  //----------------------------------------------------------------------------

  // DMP-LABEL: "implicit da for member function call\n"
  // PRT-LABEL: "implicit da for member function call\n"
  // EXE-LABEL: implicit da for member function call
  printf("implicit da for member function call\n");

  //      DMP: ACCParallelDirective
  // DMP-NEXT:   ACCNumGangsClause
  // DMP-NEXT:     IntegerLiteral {{.*}} 1
  // DMP-NEXT:   ACCCopyClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'obj'
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'obj'
  // DMP-NEXT:   impl: OMPTargetTeamsDirective
  // DMP-NEXT:     OMPNum_teamsClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 1
  // DMP-NEXT:     OMPMapClause
  //  DMP-NOT:       <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 'obj'
  //
  //         PRT: obj.z = {{.}};
  //  PRT-A-NEXT: #pragma acc parallel num_gangs(1){{$}}
  // PRT-AO-NEXT: // #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: obj) shared(obj){{$}}
  //  PRT-O-NEXT: #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: obj) shared(obj){{$}}
  // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1){{$}}
  //
  // EXE-NEXT: obj.x = 15
  // EXE-NEXT: obj.y = 26
  // EXE-NEXT: obj.z = 37
  obj.x = 5;
  obj.y = 6;
  obj.z = 7;
  #pragma acc parallel num_gangs(1)
  obj.fn();
  printf("obj.x = %d\n", obj.x);
  printf("obj.y = %d\n", obj.y);
  printf("obj.z = %d\n", obj.z);

  return 0;
}
