// Check 'static:' argument in 'gang' clause on "acc loop".

// REDEFINE: %{dmp:fc:args} = -strict-whitespace
// REDEFINE: %{exe:fc:args} = -strict-whitespace -match-full-lines \
// REDEFINE:                  -implicit-check-not='{{.}}'
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}

// END.

/* expected-no-diagnostics */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define NUM_GANGS 8
#define LOOP_NITERS 64
typedef unsigned long BitsetType;
#define BITSET_TYPE_CONV "0x%016lx"

int omp_get_team_num();
int omp_get_thread_num();
int two = 2;

//------------------------------------------------------------------------------
// 'static:*', orphaned loop.
//------------------------------------------------------------------------------

// DMP-LABEL: inOrphanedLoop
// PRT-LABEL: void inOrphanedLoop({{.*}})
#pragma acc routine gang
void inOrphanedLoop(BitsetType *scheds) {
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  // DMP-NEXT:     static: ACCStarExpr
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'scheds'
  //      DMP:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPDist_scheduleClause
  // DMP-NEXT:       <<<NULL>>>
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //
  //  PRT-A-NEXT: #pragma acc loop gang(static: *){{$}}
  // PRT-AO-NEXT: // #pragma omp distribute dist_schedule(static){{$}}
  //  PRT-O-NEXT: #pragma omp distribute dist_schedule(static){{$}}
  // PRT-OA-NEXT: // #pragma acc loop gang(static: *){{$}}
  #pragma acc loop gang(static: *)
  for (int i = 0; i < LOOP_NITERS; ++i)
    scheds[omp_get_team_num()] |= ((BitsetType)1) << i;
}

int main() {
  assert(sizeof(BitsetType)*8 >= LOOP_NITERS &&
         "expected BitsetType to have a least one bit per loop iteration");
  BitsetType scheds[2][NUM_GANGS];

  // EXE-LABEL:orphaned acc loop gang(static:*)
  printf("orphaned acc loop gang(static:*)\n");

  // EXE-NEXT:team 0: 0x{{0*}}00000000000000ff
  // EXE-NEXT:team 1: 0x{{0*}}000000000000ff00
  // EXE-NEXT:team 2: 0x{{0*}}0000000000ff0000
  // EXE-NEXT:team 3: 0x{{0*}}00000000ff000000
  // EXE-NEXT:team 4: 0x{{0*}}000000ff00000000
  // EXE-NEXT:team 5: 0x{{0*}}0000ff0000000000
  // EXE-NEXT:team 6: 0x{{0*}}00ff000000000000
  // EXE-NEXT:team 7: 0x{{0*}}ff00000000000000
  memset(scheds, 0, sizeof scheds);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(1) vector_length(1)
  inOrphanedLoop(scheds[0]);
  for (int i = 0; i < NUM_GANGS; ++i)
    printf("team %d: " BITSET_TYPE_CONV "\n", i, scheds[0][i]);

  //----------------------------------------------------------------------------
  // 'static:' constant expr, value of 8, acc loop twice in acc parallel.
  //
  // OpenACC 3.3 sec. 2.9.2 "gang clause", L2085-2087:
  // "Two gang loops in the same parallel region with the same number of
  // iterations, and with static clauses with the same argument, will assign the
  // iterations to gangs in the same manner."
  //----------------------------------------------------------------------------

  // DMP-LABEL:acc loop gang(static:8) in acc parallel\n"
  // PRT-LABEL:"acc loop gang(static:8) in acc parallel\n"
  // EXE-LABEL:acc loop gang(static:8) in acc parallel
  printf("acc loop gang(static:8) in acc parallel\n");

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  // DMP-NEXT:     static: IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'scheds'
  //      DMP:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPDist_scheduleClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //
  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  // DMP-NEXT:     static: IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'scheds'
  //      DMP:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPDist_scheduleClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //
  //         PRT: memset
  //  PRT-A-NEXT: #pragma acc parallel
  // PRT-AO-NEXT: // #pragma omp target teams
  //  PRT-O-NEXT: #pragma omp target teams
  // PRT-OA-NEXT: // #pragma acc parallel
  //    PRT-NEXT: {
  //  PRT-A-NEXT:   #pragma acc loop gang(static: 8){{$}}
  // PRT-AO-NEXT:   // #pragma omp distribute dist_schedule(static, 8){{$}}
  //  PRT-O-NEXT:   #pragma omp distribute dist_schedule(static, 8){{$}}
  // PRT-OA-NEXT:   // #pragma acc loop gang(static: 8){{$}}
  //    PRT-NEXT:   for ({{.*}})
  //    PRT-NEXT:     scheds
  //  PRT-A-NEXT:   #pragma acc loop gang(static: 8){{$}}
  // PRT-AO-NEXT:   // #pragma omp distribute dist_schedule(static, 8){{$}}
  //  PRT-O-NEXT:   #pragma omp distribute dist_schedule(static, 8){{$}}
  // PRT-OA-NEXT:   // #pragma acc loop gang(static: 8){{$}}
  //    PRT-NEXT:   for ({{.*}})
  //    PRT-NEXT:     scheds
  //    PRT-NEXT: }
  //
  // EXE-NEXT:loop 0, team 0: 0x{{0*}}00000000000000ff
  // EXE-NEXT:loop 0, team 1: 0x{{0*}}000000000000ff00
  // EXE-NEXT:loop 0, team 2: 0x{{0*}}0000000000ff0000
  // EXE-NEXT:loop 0, team 3: 0x{{0*}}00000000ff000000
  // EXE-NEXT:loop 0, team 4: 0x{{0*}}000000ff00000000
  // EXE-NEXT:loop 0, team 5: 0x{{0*}}0000ff0000000000
  // EXE-NEXT:loop 0, team 6: 0x{{0*}}00ff000000000000
  // EXE-NEXT:loop 0, team 7: 0x{{0*}}ff00000000000000
  // EXE-NEXT:loop 1, team 0: 0x{{0*}}00000000000000ff
  // EXE-NEXT:loop 1, team 1: 0x{{0*}}000000000000ff00
  // EXE-NEXT:loop 1, team 2: 0x{{0*}}0000000000ff0000
  // EXE-NEXT:loop 1, team 3: 0x{{0*}}00000000ff000000
  // EXE-NEXT:loop 1, team 4: 0x{{0*}}000000ff00000000
  // EXE-NEXT:loop 1, team 5: 0x{{0*}}0000ff0000000000
  // EXE-NEXT:loop 1, team 6: 0x{{0*}}00ff000000000000
  // EXE-NEXT:loop 1, team 7: 0x{{0*}}ff00000000000000
  memset(scheds, 0, sizeof scheds);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(1) vector_length(1)
  {
    #pragma acc loop gang(static: 8)
    for (int i = 0; i < LOOP_NITERS; ++i)
      scheds[0][omp_get_team_num()] |= ((BitsetType)1) << i;
    #pragma acc loop gang(static: 8)
    for (int i = 0; i < LOOP_NITERS; ++i)
      scheds[1][omp_get_team_num()] |= ((BitsetType)1) << i;
  }
  for (int i = 0; i < 2; ++i)
    for (int j = 0; j < NUM_GANGS; ++j)
      printf("loop %d, team %d: " BITSET_TYPE_CONV "\n", i, j, scheds[i][j]);

  //----------------------------------------------------------------------------
  // 'static:' non-constant expr, value of 2, combined acc parallel loop.
  //----------------------------------------------------------------------------

  // DMP-LABEL:acc parallel loop gang(static:two)\n"
  // PRT-LABEL:"acc parallel loop gang(static:two)\n"
  // EXE-LABEL:acc parallel loop gang(static:two)
  printf("acc parallel loop gang(static:two)\n");

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  // DMP-NEXT:     static: ImplicitCastExpr
  // DMP-NEXT:       DeclRefExpr {{.*}} 'two'
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'scheds'
  //      DMP:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPDist_scheduleClause
  // DMP-NEXT:       ImplicitCastExpr
  // DMP-NEXT:         DeclRefExpr {{.*}} 'two'
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //
  //         PRT: memset
  //  PRT-A-NEXT: #pragma acc parallel loop {{.*}} gang(static: two){{$}}
  // PRT-AO-NEXT: // #pragma omp target teams
  // PRT-AO-NEXT: // #pragma omp distribute dist_schedule(static, two){{$}}
  //
  //  PRT-O-NEXT: #pragma omp target teams
  //  PRT-O-NEXT: #pragma omp distribute dist_schedule(static, two){{$}}
  // PRT-OA-NEXT: // #pragma acc parallel loop {{.*}} gang(static: two){{$}}
  //
  // EXE-NEXT:team 0: 0x{{0*}}0003000300030003
  // EXE-NEXT:team 1: 0x{{0*}}000c000c000c000c
  // EXE-NEXT:team 2: 0x{{0*}}0030003000300030
  // EXE-NEXT:team 3: 0x{{0*}}00c000c000c000c0
  // EXE-NEXT:team 4: 0x{{0*}}0300030003000300
  // EXE-NEXT:team 5: 0x{{0*}}0c000c000c000c00
  // EXE-NEXT:team 6: 0x{{0*}}3000300030003000
  // EXE-NEXT:team 7: 0x{{0*}}c000c000c000c000
  memset(scheds, 0, sizeof scheds);
  #pragma acc parallel loop num_gangs(NUM_GANGS) num_workers(1) vector_length(1) gang(static: two)
  for (int i = 0; i < LOOP_NITERS; ++i)
    scheds[0][omp_get_team_num()] |= ((BitsetType)1) << i;
  for (int i = 0; i < NUM_GANGS; ++i)
    printf("team %d: " BITSET_TYPE_CONV "\n", i, scheds[0][i]);

  //----------------------------------------------------------------------------
  // 'static:' non-constant arithmetic expression, value of 1.
  //----------------------------------------------------------------------------

  // DMP-LABEL:acc loop gang(static:two-1) in acc parallel\n"
  // PRT-LABEL:"acc loop gang(static:two-1) in acc parallel\n"
  // EXE-LABEL:acc loop gang(static:two-1) in acc parallel
  printf("acc loop gang(static:two-1) in acc parallel\n");

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  // DMP-NEXT:     static: BinaryOperator {{.*}} '-'
  // DMP-NEXT:       ImplicitCastExpr
  // DMP-NEXT:         DeclRefExpr {{.*}} 'two'
  // DMP-NEXT:       IntegerLiteral {{.*}} 1

  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'scheds'
  //      DMP:   impl: OMPDistributeDirective
  // DMP-NEXT:     OMPDist_scheduleClause
  // DMP-NEXT:       BinaryOperator {{.*}} '-'
  // DMP-NEXT:         ImplicitCastExpr
  // DMP-NEXT:           DeclRefExpr {{.*}} 'two'
  // DMP-NEXT:         IntegerLiteral {{.*}} 1
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //
  //         PRT: memset
  //  PRT-A-NEXT: #pragma acc parallel
  // PRT-AO-NEXT: // #pragma omp target teams
  //  PRT-O-NEXT: #pragma omp target teams
  // PRT-OA-NEXT: // #pragma acc parallel
  //
  //  PRT-A-NEXT: #pragma acc loop gang(static: two - 1){{$}}
  // PRT-AO-NEXT: // #pragma omp distribute dist_schedule(static, two - 1){{$}}
  //  PRT-O-NEXT: #pragma omp distribute dist_schedule(static, two - 1){{$}}
  // PRT-OA-NEXT: // #pragma acc loop gang(static: two - 1){{$}}
  //
  // EXE-NEXT:team 0: 0x{{0*}}0101010101010101
  // EXE-NEXT:team 1: 0x{{0*}}0202020202020202
  // EXE-NEXT:team 2: 0x{{0*}}0404040404040404
  // EXE-NEXT:team 3: 0x{{0*}}0808080808080808
  // EXE-NEXT:team 4: 0x{{0*}}1010101010101010
  // EXE-NEXT:team 5: 0x{{0*}}2020202020202020
  // EXE-NEXT:team 6: 0x{{0*}}4040404040404040
  // EXE-NEXT:team 7: 0x{{0*}}8080808080808080
  memset(scheds, 0, sizeof scheds);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(1) vector_length(1)
  #pragma acc loop gang(static: two - 1)
  for (int i = 0; i < LOOP_NITERS; ++i)
    scheds[0][omp_get_team_num()] |= ((BitsetType)1) << i;
  for (int i = 0; i < NUM_GANGS; ++i)
    printf("team %d: " BITSET_TYPE_CONV "\n", i, scheds[0][i]);

  //----------------------------------------------------------------------------
  // 'static:' plus worker clause.
  //----------------------------------------------------------------------------

  // DMP-LABEL:acc loop gang(static:8) worker\n"
  // PRT-LABEL:"acc loop gang(static:8) worker\n"
  // EXE-LABEL:acc loop gang(static:8) worker
  printf("acc loop gang(static:8) worker\n");

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  // DMP-NEXT:     static: IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'scheds'
  //      DMP:   impl: OMPDistributeParallelForDirective
  // DMP-NEXT:     OMPDist_scheduleClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //
  //         PRT: memset
  //  PRT-A-NEXT: #pragma acc parallel
  // PRT-AO-NEXT: // #pragma omp target teams
  //  PRT-O-NEXT: #pragma omp target teams
  // PRT-OA-NEXT: // #pragma acc parallel

  //  PRT-A-NEXT: #pragma acc loop gang(static: 8) worker{{$}}
  // PRT-AO-NEXT: // #pragma omp distribute parallel for dist_schedule(static, 8){{$}}
  //  PRT-O-NEXT: #pragma omp distribute parallel for dist_schedule(static, 8){{$}}
  // PRT-OA-NEXT: // #pragma acc loop gang(static: 8) worker{{$}}
  //    PRT-NEXT: for ({{.*}})
  //    PRT-NEXT:   scheds
  //
  // EXE-NEXT:team 0: 0x{{0*}}000000ff000000ff
  // EXE-NEXT:team 1: 0x{{0*}}0000ff000000ff00
  // EXE-NEXT:team 2: 0x{{0*}}00ff000000ff0000
  // EXE-NEXT:team 3: 0x{{0*}}ff000000ff000000
  memset(scheds, 0, sizeof scheds);
  #pragma acc parallel num_gangs(NUM_GANGS/2) num_workers(2) vector_length(1)
  #pragma acc loop gang(static: 8) worker
  for (int i = 0; i < LOOP_NITERS; ++i)
    scheds[omp_get_thread_num()][omp_get_team_num()] |= ((BitsetType)1) << i;
  for (int i = 0; i < NUM_GANGS/2; ++i)
    printf("team %d: " BITSET_TYPE_CONV "\n", i, scheds[0][i] | scheds[1][i]);

  //----------------------------------------------------------------------------
  // 'static:' plus vector clause.
  //----------------------------------------------------------------------------

  // DMP-LABEL:acc loop gang(static:8) vector\n"
  // PRT-LABEL:"acc loop gang(static:8) vector\n"
  // EXE-LABEL:acc loop gang(static:8) vector
  printf("acc loop gang(static:8) vector\n");

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  // DMP-NEXT:     static: IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'scheds'
  //      DMP:   impl: OMPDistributeSimdDirective
  // DMP-NEXT:     OMPSimdlenClause
  // DMP-NEXT:       ConstantExpr
  // DMP-NEXT:         value: Int 256
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 256
  // DMP-NEXT:     OMPDist_scheduleClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //
  //         PRT: memset
  //  PRT-A-NEXT: #pragma acc parallel
  // PRT-AO-NEXT: // #pragma omp target teams
  //  PRT-O-NEXT: #pragma omp target teams
  // PRT-OA-NEXT: // #pragma acc parallel
  //
  //  PRT-A-NEXT: #pragma acc loop gang(static: 8) vector{{$}}
  // PRT-AO-NEXT: // #pragma omp distribute simd simdlen(256) dist_schedule(static, 8){{$}}
  //  PRT-O-NEXT: #pragma omp distribute simd simdlen(256) dist_schedule(static, 8){{$}}
  // PRT-OA-NEXT: // #pragma acc loop gang(static: 8) vector{{$}}
  //    PRT-NEXT: for ({{.*}})
  //
  // EXE-NEXT:team 0: 0x{{0*}}00000000000000ff
  // EXE-NEXT:team 1: 0x{{0*}}000000000000ff00
  // EXE-NEXT:team 2: 0x{{0*}}0000000000ff0000
  // EXE-NEXT:team 3: 0x{{0*}}00000000ff000000
  // EXE-NEXT:team 4: 0x{{0*}}000000ff00000000
  // EXE-NEXT:team 5: 0x{{0*}}0000ff0000000000
  // EXE-NEXT:team 6: 0x{{0*}}00ff000000000000
  // EXE-NEXT:team 7: 0x{{0*}}ff00000000000000
  memset(scheds, 0, sizeof scheds);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(1) vector_length(256)
  #pragma acc loop gang(static: 8) vector
  for (int i = 0; i < LOOP_NITERS; ++i)
    #pragma acc atomic update
    scheds[0][omp_get_team_num()] |= ((BitsetType)1) << i;
  for (int i = 0; i < NUM_GANGS; ++i)
    printf("team %d: " BITSET_TYPE_CONV "\n", i, scheds[0][i]);

  //----------------------------------------------------------------------------
  // 'static:' plus worker and vector clause.
  //----------------------------------------------------------------------------

  // DMP-LABEL:acc loop gang(static:8) worker vector\n"
  // PRT-LABEL:"acc loop gang(static:8) worker vector\n"
  // EXE-LABEL:acc loop gang(static:8) worker vector
  printf("acc loop gang(static:8) worker vector\n");

  //      DMP: ACCLoopDirective
  // DMP-NEXT:   ACCGangClause
  // DMP-NEXT:     static: IntegerLiteral {{.*}} 'int' 8
  // DMP-NEXT:   ACCWorkerClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCVectorClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  // DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  // DMP-NEXT:     DeclRefExpr {{.*}} 'scheds'
  //      DMP:   impl: OMPDistributeParallelForSimdDirective
  // DMP-NEXT:     OMPSimdlenClause
  // DMP-NEXT:       ConstantExpr
  // DMP-NEXT:         value: Int 256
  // DMP-NEXT:         IntegerLiteral {{.*}} 'int' 256
  // DMP-NEXT:     OMPDist_scheduleClause
  // DMP-NEXT:       IntegerLiteral {{.*}} 'int' 8
  //  DMP-NOT:     OMP
  //      DMP:     ForStmt
  //
  //         PRT: memset
  //  PRT-A-NEXT: #pragma acc parallel
  // PRT-AO-NEXT: // #pragma omp target teams
  //  PRT-O-NEXT: #pragma omp target teams
  // PRT-OA-NEXT: // #pragma acc parallel

  //  PRT-A-NEXT: #pragma acc loop gang(static: 8) worker vector{{$}}
  // PRT-AO-NEXT: // #pragma omp distribute parallel for simd simdlen(256) dist_schedule(static, 8){{$}}
  //  PRT-O-NEXT: #pragma omp distribute parallel for simd simdlen(256) dist_schedule(static, 8){{$}}
  // PRT-OA-NEXT: // #pragma acc loop gang(static: 8) worker vector{{$}}
  //    PRT-NEXT: for ({{.*}})
  //
  // EXE-NEXT:team 0: 0x{{0*}}000000ff000000ff
  // EXE-NEXT:team 1: 0x{{0*}}0000ff000000ff00
  // EXE-NEXT:team 2: 0x{{0*}}00ff000000ff0000
  // EXE-NEXT:team 3: 0x{{0*}}ff000000ff000000
  memset(scheds, 0, sizeof scheds);
  #pragma acc parallel num_gangs(NUM_GANGS/2) num_workers(2) vector_length(256)
  #pragma acc loop gang(static: 8) worker vector
  for (int i = 0; i < LOOP_NITERS; ++i)
    #pragma acc atomic update
    scheds[omp_get_thread_num()][omp_get_team_num()] |= ((BitsetType)1) << i;
  for (int i = 0; i < NUM_GANGS/2; ++i)
    printf("team %d: " BITSET_TYPE_CONV "\n", i, scheds[0][i] | scheds[1][i]);

  return 0;
}
