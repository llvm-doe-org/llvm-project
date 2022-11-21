// Check implicit data attributes for member expressions on "acc loop".

// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}

// END.

/* expected-no-diagnostics */

#include <stdio.h>

struct S {
  int x;
  int y;
  int z;
};

struct SS {
  struct S s;
};

// DMP-LABEL: inOrphanedLoop
// PRT-LABEL: void inOrphanedLoop(struct S *ps)
#pragma acc routine gang
void inOrphanedLoop(struct S *ps) {
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'ps'
  // DMP-NEXT:   ACCGangClause
  //      DMP:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 'ps'
  //
  //  PRT-A-NEXT: #pragma acc loop{{$}}
  // PRT-AO-NEXT: // #pragma omp distribute{{$}}
  //  PRT-O-NEXT: #pragma omp distribute{{$}}
  // PRT-OA-NEXT: // #pragma acc loop{{$}}
  #pragma acc loop
  for (int i = 0; i < 1; ++i)
    ps->x += 10;
}

int main() {
  struct S s;
  struct SS ss;

  //============================================================================
  // Check that using a member of a struct produces an implicit data attribute
  // for the struct.
  //============================================================================

  //----------------------------------------------------------------------------
  // Check when loop construct is orphaned.
  //----------------------------------------------------------------------------

  // EXE-LABEL: implicit da for member use in orphaned acc loop
  printf("implicit da for member use in orphaned acc loop\n");

  // EXE-NEXT: s.x = 15
  // EXE-NEXT: s.y = 6
  // EXE-NEXT: s.z = 7
  s.x = 5;
  s.y = 6;
  s.z = 7;
  #pragma acc parallel
  inOrphanedLoop(&s);
  printf("s.x = %d\n", s.x);
  printf("s.y = %d\n", s.y);
  printf("s.z = %d\n", s.z);

  //----------------------------------------------------------------------------
  // Check within parallel construct.
  //----------------------------------------------------------------------------

  // DMP-LABEL: "implicit da for member use in acc loop in acc parallel\n"
  // PRT-LABEL: "implicit da for member use in acc loop in acc parallel\n"
  // EXE-LABEL: implicit da for member use in acc loop in acc parallel
  printf("implicit da for member use in acc loop in acc parallel\n");

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 's'
  // DMP-NEXT:   ACCGangClause
  //      DMP:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 's'
  //
  //         PRT: s.z = {{.}};
  //  PRT-A-NEXT: #pragma acc parallel{{$}}
  // PRT-AO-NEXT: // #pragma omp target teams
  //  PRT-A-NEXT: #pragma acc loop{{$}}
  // PRT-AO-NEXT: // #pragma omp distribute{{$}}
  //  PRT-O-NEXT: #pragma omp target teams
  // PRT-OA-NEXT: // #pragma acc parallel{{$}}
  //  PRT-O-NEXT: #pragma omp distribute{{$}}
  // PRT-OA-NEXT: // #pragma acc loop{{$}}
  //
  // EXE-NEXT: s.x = 15
  // EXE-NEXT: s.y = 6
  // EXE-NEXT: s.z = 7
  s.x = 5;
  s.y = 6;
  s.z = 7;
  #pragma acc parallel
  #pragma acc loop
  for (int i = 0; i < 1; ++i)
    s.x += 10;
  printf("s.x = %d\n", s.x);
  printf("s.y = %d\n", s.y);
  printf("s.z = %d\n", s.z);

  //----------------------------------------------------------------------------
  // Check as part of combined construct.
  //----------------------------------------------------------------------------

  // DMP-LABEL: "implicit da for member use in combined construct\n"
  // PRT-LABEL: "implicit da for member use in combined construct\n"
  // EXE-LABEL: implicit da for member use in combined construct
  printf("implicit da for member use in combined construct\n");

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 's'
  // DMP-NEXT:   ACCGangClause
  //      DMP:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 's'
  //
  //         PRT: s.z = {{.}};
  //  PRT-A-NEXT: #pragma acc parallel loop{{$}}
  // PRT-AO-NEXT: // #pragma omp target teams
  // PRT-AO-NEXT: // #pragma omp distribute{{$}}
  //  PRT-O-NEXT: #pragma omp target teams
  //  PRT-O-NEXT: #pragma omp distribute{{$}}
  // PRT-OA-NEXT: // #pragma acc parallel loop{{$}}
  //
  // EXE-NEXT: s.x = 15
  // EXE-NEXT: s.y = 6
  // EXE-NEXT: s.z = 7
  s.x = 5;
  s.y = 6;
  s.z = 7;
  #pragma acc parallel loop
  for (int i = 0; i < 1; ++i)
    s.x += 10;
  printf("s.x = %d\n", s.x);
  printf("s.y = %d\n", s.y);
  printf("s.z = %d\n", s.z);

  //----------------------------------------------------------------------------
  // Check within nested loop construct.
  //
  // This also checks gang, worker, and vector on separate loop constructs.
  //----------------------------------------------------------------------------

  // DMP-LABEL: "implicit da for member use in nested acc loop\n"
  // PRT-LABEL: "implicit da for member use in nested acc loop\n"
  // EXE-LABEL: implicit da for member use in nested acc loop
  printf("implicit da for member use in nested acc loop\n");

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  // DMP-NEXT:   ACCIndependentClause
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 's'
  //      DMP:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 's'
  //      DMP:   ACCLoopDirective
  // DMP-NEXT:     ACCWorkerClause
  // DMP-NEXT:     ACCIndependentClause
  // DMP-NEXT:     ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 's'
  //      DMP:     impl: OMPParallelForDirective
  // DMP-NEXT:       OMPSharedClause
  //  DMP-NOT:         <implicit>
  // DMP-NEXT:         DeclRefExpr {{.*}} 's'
  //      DMP:     ACCLoopDirective
  // DMP-NEXT:       ACCVectorClause
  // DMP-NEXT:       ACCIndependentClause
  // DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:         DeclRefExpr {{.*}} 's'
  //      DMP:       impl: OMPSimdDirective
  // DMP-NEXT:         OMPSharedClause {{.*}} <implicit>
  // DMP-NEXT:           DeclRefExpr {{.*}} 's'
  //
  //         PRT: s.z = {{.}};
  //  PRT-A-NEXT: #pragma acc parallel loop gang{{$}}
  // PRT-AO-NEXT: // #pragma omp target teams
  // PRT-AO-NEXT: // #pragma omp distribute{{$}}
  //  PRT-O-NEXT: #pragma omp target teams
  //  PRT-O-NEXT: #pragma omp distribute{{$}}
  // PRT-OA-NEXT: // #pragma acc parallel loop gang{{$}}
  //    PRT-NEXT: for (int i = 0; i < 1; ++i) {
  //  PRT-A-NEXT:   #pragma acc loop worker{{$}}
  // PRT-AO-NEXT:   // #pragma omp parallel for shared(s){{$}}
  //  PRT-O-NEXT:   #pragma omp parallel for shared(s){{$}}
  // PRT-OA-NEXT:   // #pragma acc loop worker{{$}}
  //    PRT-NEXT:   for (int j = 0; j < 1; ++j) {
  //  PRT-A-NEXT:     #pragma acc loop vector{{$}}
  // PRT-AO-NEXT:     // #pragma omp simd{{$}}
  //  PRT-O-NEXT:     #pragma omp simd{{$}}
  // PRT-OA-NEXT:     // #pragma acc loop vector{{$}}
  //    PRT-NEXT:     for (int k = 0; k < 1; ++k)
  //    PRT-NEXT:       s.x += 10;
  //    PRT-NEXT:   }
  //    PRT-NEXT: }
  //
  // EXE-NEXT: s.x = 15
  // EXE-NEXT: s.y = 6
  // EXE-NEXT: s.z = 7
  s.x = 5;
  s.y = 6;
  s.z = 7;
  #pragma acc parallel loop gang
  for (int i = 0; i < 1; ++i) {
    #pragma acc loop worker
    for (int j = 0; j < 1; ++j) {
      #pragma acc loop vector
      for (int k = 0; k < 1; ++k)
        s.x += 10;
    }
  }
  printf("s.x = %d\n", s.x);
  printf("s.y = %d\n", s.y);
  printf("s.z = %d\n", s.z);

  //----------------------------------------------------------------------------
  // Check acc loop gang worker.
  //----------------------------------------------------------------------------

  // DMP-LABEL: "implicit da for member use in acc loop gang worker\n"
  // PRT-LABEL: "implicit da for member use in acc loop gang worker\n"
  // EXE-LABEL: implicit da for member use in acc loop gang worker
  printf("implicit da for member use in acc loop gang worker\n");

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  // DMP-NEXT:   ACCWorkerClause
  // DMP-NEXT:   ACCIndependentClause
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 's'
  //      DMP:   impl: OMPDistributeParallelForDirective
  // DMP-NEXT:     OMPSharedClause
  //  DMP-NOT:       <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 's'
  //
  //         PRT: s.z = {{.}};
  //  PRT-A-NEXT: #pragma acc parallel loop gang worker{{$}}
  // PRT-AO-NEXT: // #pragma omp target teams
  // PRT-AO-NEXT: // #pragma omp distribute parallel for shared(s){{$}}
  //  PRT-O-NEXT: #pragma omp target teams
  //  PRT-O-NEXT: #pragma omp distribute parallel for shared(s){{$}}
  // PRT-OA-NEXT: // #pragma acc parallel loop gang worker{{$}}
  //
  // EXE-NEXT: s.x = 5
  // EXE-NEXT: s.y = 26
  // EXE-NEXT: s.z = 7
  s.x = 5;
  s.y = 6;
  s.z = 7;
  #pragma acc parallel loop gang worker
  for (int i = 0; i < 1; ++i)
    s.y += 20;
  printf("s.x = %d\n", s.x);
  printf("s.y = %d\n", s.y);
  printf("s.z = %d\n", s.z);

  //----------------------------------------------------------------------------
  // Check acc loop gang vector.
  //----------------------------------------------------------------------------

  // DMP-LABEL: "implicit da for member use in acc loop gang vector\n"
  // PRT-LABEL: "implicit da for member use in acc loop gang vector\n"
  // EXE-LABEL: implicit da for member use in acc loop gang vector
  printf("implicit da for member use in acc loop gang vector\n");

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  // DMP-NEXT:   ACCVectorClause
  // DMP-NEXT:   ACCIndependentClause
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 's'
  //      DMP:   impl: OMPDistributeSimdDirective
  // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 's'
  //
  //         PRT: s.z = {{.}};
  //  PRT-A-NEXT: #pragma acc parallel loop gang vector{{$}}
  // PRT-AO-NEXT: // #pragma omp target teams
  // PRT-AO-NEXT: // #pragma omp distribute simd{{$}}
  //  PRT-O-NEXT: #pragma omp target teams
  //  PRT-O-NEXT: #pragma omp distribute simd{{$}}
  // PRT-OA-NEXT: // #pragma acc parallel loop gang vector{{$}}
  //
  // EXE-NEXT: s.x = 5
  // EXE-NEXT: s.y = 6
  // EXE-NEXT: s.z = 37
  s.x = 5;
  s.y = 6;
  s.z = 7;
  #pragma acc parallel loop gang vector
  for (int i = 0; i < 1; ++i)
    s.z += 30;
  printf("s.x = %d\n", s.x);
  printf("s.y = %d\n", s.y);
  printf("s.z = %d\n", s.z);

  //----------------------------------------------------------------------------
  // Check acc loop gang worker vector.
  //----------------------------------------------------------------------------

  // DMP-LABEL: "implicit da for member use in acc loop gang worker vector\n"
  // PRT-LABEL: "implicit da for member use in acc loop gang worker vector\n"
  // EXE-LABEL: implicit da for member use in acc loop gang worker vector
  printf("implicit da for member use in acc loop gang worker vector\n");

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  // DMP-NEXT:   ACCWorkerClause
  // DMP-NEXT:   ACCVectorClause
  // DMP-NEXT:   ACCIndependentClause
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 's'
  //      DMP:   impl: OMPDistributeParallelForSimdDirective
  // DMP-NEXT:     OMPSharedClause
  //  DMP-NOT:       <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 's'
  //
  //         PRT: s.z = {{.}};
  //  PRT-A-NEXT: #pragma acc parallel loop gang worker vector{{$}}
  // PRT-AO-NEXT: // #pragma omp target teams
  // PRT-AO-NEXT: // #pragma omp distribute parallel for simd shared(s){{$}}
  //  PRT-O-NEXT: #pragma omp target teams
  //  PRT-O-NEXT: #pragma omp distribute parallel for simd shared(s){{$}}
  // PRT-OA-NEXT: // #pragma acc parallel loop gang worker vector{{$}}
  //
  // EXE-NEXT: s.x = 15
  // EXE-NEXT: s.y = 6
  // EXE-NEXT: s.z = 7
  s.x = 5;
  s.y = 6;
  s.z = 7;
  #pragma acc parallel loop gang worker vector
  for (int i = 0; i < 1; ++i)
    s.x += 10;
  printf("s.x = %d\n", s.x);
  printf("s.y = %d\n", s.y);
  printf("s.z = %d\n", s.z);

  //----------------------------------------------------------------------------
  // Check acc loop seq.
  //----------------------------------------------------------------------------

  // DMP-LABEL: "implicit da for member use in acc loop seq\n"
  // PRT-LABEL: "implicit da for member use in acc loop seq\n"
  // EXE-LABEL: implicit da for member use in acc loop seq
  printf("implicit da for member use in acc loop seq\n");

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCSeqClause
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 's'
  //      DMP:   impl: ForStmt
  //
  //         PRT: s.z = {{.}};
  //  PRT-A-NEXT: #pragma acc parallel num_gangs(1){{$}}
  // PRT-AO-NEXT: // #pragma omp target teams
  //  PRT-A-NEXT: #pragma acc loop seq
  // PRT-AO-SAME: {{^}} // discarded in OpenMP translation
  //  PRT-A-SAME: {{^$}}
  //  PRT-O-NEXT: #pragma omp target teams
  // PRT-OA-NEXT: // #pragma acc parallel num_gangs(1){{$}}
  // PRT-OA-NEXT: // #pragma acc loop seq // discarded in OpenMP translation{{$}}
  //    PRT-NEXT: for (int i = 0; i < 1; ++i)
  //
  // EXE-NEXT: s.x = 15
  // EXE-NEXT: s.y = 6
  // EXE-NEXT: s.z = 7
  s.x = 5;
  s.y = 6;
  s.z = 7;
  #pragma acc parallel num_gangs(1)
  #pragma acc loop seq
  for (int i = 0; i < 1; ++i)
    s.x += 10;
  printf("s.x = %d\n", s.x);
  printf("s.y = %d\n", s.y);
  printf("s.z = %d\n", s.z);

  //----------------------------------------------------------------------------
  // Check with uses of multiple members for the same struct.
  //----------------------------------------------------------------------------

  // DMP-LABEL: "implicit da for use of multiple members\n"
  // PRT-LABEL: "implicit da for use of multiple members\n"
  // EXE-LABEL: implicit da for use of multiple members
  printf("implicit da for use of multiple members\n");

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 's'
  // DMP-NEXT:   ACCGangClause
  //      DMP:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 's'
  //
  //         PRT: s.z = {{.}};
  //  PRT-A-NEXT: #pragma acc parallel{{$}}
  // PRT-AO-NEXT: // #pragma omp target teams
  //  PRT-A-NEXT: #pragma acc loop{{$}}
  // PRT-AO-NEXT: // #pragma omp distribute{{$}}
  //  PRT-O-NEXT: #pragma omp target teams
  // PRT-OA-NEXT: // #pragma acc parallel{{$}}
  //  PRT-O-NEXT: #pragma omp distribute{{$}}
  // PRT-OA-NEXT: // #pragma acc loop{{$}}
  //
  // EXE-NEXT: s.x = 5
  // EXE-NEXT: s.y = 26
  // EXE-NEXT: s.z = 37
  s.x = 5;
  s.y = 6;
  s.z = 7;
  #pragma acc parallel
  #pragma acc loop
  for (int i = 0; i < 1; ++i) {
    s.y += 20;
    s.z += 30;
  }
  printf("s.x = %d\n", s.x);
  printf("s.y = %d\n", s.y);
  printf("s.z = %d\n", s.z);

  //----------------------------------------------------------------------------
  // Check that using a member of a member of a struct produces an implicit data
  // attribute for the outermost struct.
  //----------------------------------------------------------------------------

  // DMP-LABEL: "implicit da for member of member use\n"
  // PRT-LABEL: "implicit da for member of member use\n"
  // EXE-LABEL: implicit da for member of member use
  printf("implicit da for member of member use\n");

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'ss'
  // DMP-NEXT:   ACCGangClause
  //      DMP:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 'ss'
  //
  //         PRT: ss.s.z = {{.}};
  //  PRT-A-NEXT: #pragma acc parallel{{$}}
  // PRT-AO-NEXT: // #pragma omp target teams
  //  PRT-A-NEXT: #pragma acc loop{{$}}
  // PRT-AO-NEXT: // #pragma omp distribute{{$}}
  //  PRT-O-NEXT: #pragma omp target teams
  // PRT-OA-NEXT: // #pragma acc parallel{{$}}
  //  PRT-O-NEXT: #pragma omp distribute{{$}}
  // PRT-OA-NEXT: // #pragma acc loop{{$}}
  //
  // EXE-NEXT: ss.s.x = 5
  // EXE-NEXT: ss.s.y = 6
  // EXE-NEXT: ss.s.z = 37
  ss.s.x = 5;
  ss.s.y = 6;
  ss.s.z = 7;
  #pragma acc parallel
  #pragma acc loop
  for (int i = 0; i < 1; ++i)
    ss.s.z += 30;
  printf("ss.s.x = %d\n", ss.s.x);
  printf("ss.s.y = %d\n", ss.s.y);
  printf("ss.s.z = %d\n", ss.s.z);

  //----------------------------------------------------------------------------
  // Check with -> operator.
  //----------------------------------------------------------------------------

  // DMP-LABEL: "implicit da for use of member via ->\n"
  // PRT-LABEL: "implicit da for use of member via ->\n"
  // EXE-LABEL: implicit da for use of member via ->
  printf("implicit da for use of member via ->\n");

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCIndependentClause
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'py'
  // DMP-NEXT:     DeclRefExpr {{.*}} 'ps'
  // DMP-NEXT:   ACCGangClause
  // DMP-NEXT:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPSharedClause {{.*}} <implicit>
  // DMP-NEXT:       DeclRefExpr {{.*}} 'py'
  // DMP-NEXT:       DeclRefExpr {{.*}} 'ps'
  //
  //         PRT: int *py;
  //  PRT-A-NEXT: #pragma acc parallel copy(py,ps){{$}}
  // PRT-AO-NEXT: // #pragma omp target teams
  //  PRT-A-NEXT: #pragma acc loop{{$}}
  // PRT-AO-NEXT: // #pragma omp distribute{{$}}
  //  PRT-O-NEXT: #pragma omp target teams
  // PRT-OA-NEXT: // #pragma acc parallel copy(py,ps){{$}}
  //  PRT-O-NEXT: #pragma omp distribute{{$}}
  // PRT-OA-NEXT: // #pragma acc loop{{$}}
  //
  // EXE-NEXT: &s.y = 0x[[#%x,S_Y_ADDR:]]
  // EXE-NEXT: py = 0x[[#S_Y_ADDR]]
  printf("&s.y = %p\n", &s.y);
  {
    struct S *ps = &s;
    int *py;
    #pragma acc parallel copy(py,ps)
    #pragma acc loop
    for (int i = 0; i < 1; ++i)
      py = &ps->y;
    printf("py = %p\n", py);
  }

  return 0;
}
