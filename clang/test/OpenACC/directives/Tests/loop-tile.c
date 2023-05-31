// Check tile clause on "acc parallel loop", on "acc loop" within
// "acc parallel", and on orphaned "acc loop".
//
// Interaction with implicit worker and vector clauses is checked in
// loop-implicit-gang-worker-vector.c.
//
// Keep these FileCheck variable definitions in sync with preprocessor macro
// definitions below.
// REDEFINE: %{all:fc:args} = \
// REDEFINE:   -D#NUM_GANGS=4 -D#NUM_WORKERS=2 -D#VECTOR_LENGTH=256 \
// REDEFINE:   -D#I_NTILES=8 -D#I_NELEMS=2 -D#J_NTILES=6 -D#J_NELEMS=3
//
// REDEFINE: %{dmp:fc:args} = -strict-whitespace
// REDEFINE: %{exe:fc:args} = -strict-whitespace -match-full-lines \
// REDEFINE:                  -implicit-check-not='{{.}}'
//
// FIXME: Without -O1, thread_limit(NUM_WORKERS) is sometimes treated as
// thread_limit(1) on at least nvptx64.
// REDEFINE: %{exe:clang:args} = -O1
//
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}
//
// END.

/* expected-no-diagnostics */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

// Keep these in sync with FileCheck variable definitions above.
#define NUM_GANGS 4
#define NUM_WORKERS 2
#define VECTOR_LENGTH 256
#define I_NTILES 8
#define I_NELEMS 2
#define J_NTILES 6
#define J_NELEMS 3

typedef struct Order {
  // indices[gangIdx][workerIdx][iTile][jTile][iElem][jElem] is the index of
  // element (iElem,jElem) of tile (iTile,jTile) when sorting all elements from
  // all tiles based on the order in which they're visited by worker workerIdx
  // of gang gangIdx during execution of the loop nest.  Thus, 0 <= that index
  // < I_NTILES * J_NTILES * I_NELEMS * J_NELEMS if the element is visited.
  // That index < 0 if the element is unvisited.
  short indices[NUM_GANGS][NUM_WORKERS][I_NTILES][J_NTILES][I_NELEMS][J_NELEMS];
  // nexts[gangIdx][workerIdx] is the next index to record in
  // indices[gangIdx][workerIdx] for any element visited by worker workerIdx of
  // gang gangIdx.
  short nexts[NUM_GANGS][NUM_WORKERS];
} Order;

// Initialize Order.
static void initOrder(Order *order) {
  for (int gangIdx = 0; gangIdx < NUM_GANGS; ++gangIdx) {
    for (int workerIdx = 0; workerIdx < NUM_WORKERS; ++workerIdx) {
      for (int iTile = 0; iTile < I_NTILES; ++iTile)
        for (int jTile = 0; jTile < J_NTILES; ++jTile)
          for (int iElem = 0; iElem < I_NELEMS; ++iElem)
            for (int jElem = 0; jElem < J_NELEMS; ++jElem)
              order->indices[gangIdx][workerIdx][iTile][jTile][iElem][jElem] =
                  -1;
      order->nexts[gangIdx][workerIdx] = 0;
    }
  }
}

// Record in an Order that (i,j) was visited.
//
// iNtiles and jNtiles are the numbers of tiles.  They must be less than
// I_NTILES and J_NTILES, respectively.
#pragma acc routine seq
static void setOrderFull(Order *order, int i, int j, int iNelems, int jNelems) {
  int omp_get_team_num();
  int omp_get_thread_num();
  int gangIdx = omp_get_team_num();
  int workerIdx = omp_get_thread_num();
  int iTile = i / iNelems;
  int jTile = j / jNelems;
  int iElem = i % iNelems;
  int jElem = j % jNelems;
  order->indices[gangIdx][workerIdx][iTile][jTile][iElem][jElem] =
      order->nexts[gangIdx][workerIdx]++;
}
#pragma acc routine seq
static void setOrder(Order *order, int i, int j) {
  setOrderFull(order, i, j, I_NELEMS, J_NELEMS);
}

// Check that an Order records a correct visitation order for 2-D tiling.
//
// gangPartitioned and workerPartitioned indicate whether the tile loops were
// gang partitioned and worker partitioned.  Currently in our implementation,
// the element loops are always executed sequentially, and neither tile nor
// elements loops are vector partitioned.
//
// numGangs is the number of gangs that executed the loops.  It must be less
// than NUM_GANGS.
//
// iNtiles, jNtiles, iNElems, and jNelems are the numbers of tiles and tile
// sizes.  They must be less than I_NTILES, J_NTILES, I_NELEMS, and J_NELEMS,
// respectively.
static bool checkOrder2DFull(Order *order, bool gangPartitioned,
                             bool workerPartitioned, int numGangs, int iNtiles,
                             int jNtiles, int iNelems, int jNelems) {
  assert(numGangs <= NUM_GANGS);
  assert(iNtiles <= I_NTILES);
  assert(jNtiles <= J_NTILES);
  assert(iNelems <= I_NELEMS);
  assert(jNelems <= J_NELEMS);
  // tileVisits[iTile][jTile] records how many times tile (iTile,jTile) was
  // visited.
  int tileVisits[I_NTILES][J_NTILES];
  for (int iTile = 0; iTile < iNtiles; ++iTile)
    for (int jTile = 0; jTile < jNtiles; ++jTile)
      tileVisits[iTile][jTile] = 0;
  // gangWorkerVisits[gangIdx][workerIdx] records how many tiles worker
  // workerIdx of gang gangIdx visited.
  int gangWorkerVisits[NUM_GANGS][NUM_WORKERS];
  for (int gangIdx = 0; gangIdx < numGangs; ++gangIdx)
    for (int workerIdx = 0; workerIdx < NUM_WORKERS; ++workerIdx)
      gangWorkerVisits[gangIdx][workerIdx] = 0;
  // Check that each worker in each gang iterated among tiles in row-major
  // order, possibly skipping some.  That's not required by OpenACC 3.3, but
  // it's how our implementation currently works.
  //
  // Within each tile, check that each worker in each gang iterated the elements
  // in row-major order.  That is required by OpenACC 3.3 when combined with the
  // fact that our implementation currently drops the parallelism OpenACC
  // specifies for the element loops.
  //
  // Record how many times each tile was visited.  Check that no tile was
  // only partially visited by any worker of any gang.
  for (int gangIdx = 0; gangIdx < numGangs; ++gangIdx) {
    for (int workerIdx = 0; workerIdx < NUM_WORKERS; ++workerIdx) {
      int nextExpected = 0;
      for (int iTile = 0; iTile < iNtiles; ++iTile) {
        for (int jTile = 0; jTile < jNtiles; ++jTile) {
          bool tileVisited = false;
          int actual0 =
              order->indices[gangIdx][workerIdx][iTile][jTile][0][0];
          if (actual0 >= 0) {
            ++tileVisits[iTile][jTile];
            ++gangWorkerVisits[gangIdx][workerIdx];
            tileVisited = true;
          }
          for (int iElem = 0; iElem < iNelems; ++iElem) {
            for (int jElem = 0; jElem < jNelems; ++jElem) {
              int actual =
                order->indices[gangIdx][workerIdx][iTile][jTile][iElem][jElem];
              if (tileVisited) {
                if (actual != nextExpected) {
                  printf("  error: indices[%d][%d][%d][%d][%d][%d] = %d, "
                         "but expected %d\n",
                         gangIdx, workerIdx, iTile, jTile, iElem, jElem, actual,
                         nextExpected);
                  return true;
                }
                ++nextExpected;
              } else if (actual >= 0) {
                printf("  error: indices[%d][%d][%d][%d][%d][%d] = %d, "
                       "but earlier elements of tile were unvisited\n",
                       gangIdx, workerIdx, iTile, jTile, iElem, jElem, actual);
                return true;
              }
            }
          }
        }
      }
    }
  }
  // Based on gangPartitioned and workerPartitioned, check that each tile was
  // visited (in its entirety, as verified above) the right number of times
  // (correct user-visible behavior), and check the number of tiles that each
  // worker of each gang visited (correct utilization of resources).
  bool err = false;
  for (int iTile = 0; iTile < iNtiles; ++iTile) {
    for (int jTile = 0; jTile < jNtiles; ++jTile) {
      int expected = gangPartitioned ? 1 : numGangs;
      int actual = tileVisits[iTile][jTile];
      if (actual != expected) {
        printf("  error: tileVisits[%d][%d] = %d, expected = %d\n",
               iTile, jTile, actual, expected);
        err =  true;
      }
    }
  }
  for (int gangIdx = 0; gangIdx < numGangs; ++gangIdx) {
    for (int workerIdx = 0; workerIdx < NUM_WORKERS; ++workerIdx) {
      int actual = gangWorkerVisits[gangIdx][workerIdx];
      int expected = iNtiles * jNtiles;
      if (gangPartitioned)
        expected /= numGangs;
      if (workerPartitioned)
        expected /= NUM_WORKERS;
      else if (workerIdx > 0)
        expected = 0;
      if (actual != expected) {
        printf("  error: gangWorkerVisits[%d][%d] = %d, expected = %d\n",
               gangIdx, workerIdx, actual, expected);
        err =  true;
      }
    }
  }
  return err;
}
static bool checkOrder2D(Order *order, bool gangPartitioned,
                                 bool workerPartitioned) {
  return checkOrder2DFull(order, gangPartitioned, workerPartitioned, NUM_GANGS,
                          I_NTILES, J_NTILES, I_NELEMS, J_NELEMS);
}

// Check that an Order records a correct visitation order for 1-D tiling.
static bool checkOrder1D(Order *order, bool gangPartitioned,
                         bool workerPartitioned, bool innerWorkerPartitioned) {
  assert((!workerPartitioned || !innerWorkerPartitioned) &&
         "expected either outer or inner worker partitioning but not both");
  int tileVisits[I_NTILES][I_NELEMS][J_NTILES];
  for (int iTile = 0; iTile < I_NTILES; ++iTile)
    for (int iElem = 0; iElem < I_NELEMS; ++iElem)
      for (int jTile = 0; jTile < J_NTILES; ++jTile)
        tileVisits[iTile][iElem][jTile] = 0;
  int gangWorkerVisitsOuter[NUM_GANGS][NUM_WORKERS];
  int gangWorkerVisitsInner[NUM_GANGS][NUM_WORKERS];
  for (int gangIdx = 0; gangIdx < NUM_GANGS; ++gangIdx) {
    for (int workerIdx = 0; workerIdx < NUM_WORKERS; ++workerIdx) {
      gangWorkerVisitsOuter[gangIdx][workerIdx] = 0;
      gangWorkerVisitsInner[gangIdx][workerIdx] = 0;
    }
  }
  for (int gangIdx = 0; gangIdx < NUM_GANGS; ++gangIdx) {
    for (int workerIdx = 0; workerIdx < NUM_WORKERS; ++workerIdx) {
      int nextExpected = 0;
      for (int iTile = 0; iTile < I_NTILES; ++iTile) {
        bool outerTileVisited = false;
        for (int iElem = 0; iElem < I_NELEMS; ++iElem) {
          for (int jTile = 0; jTile < J_NTILES; ++jTile) {
            bool innerTileVisited = false;
            int innerActual0 =
                order->indices[gangIdx][workerIdx][iTile][jTile][0][0];
            if (innerActual0 >= 0) {
              if (!innerWorkerPartitioned && jTile > 0 && !outerTileVisited) {
                printf("  error: indices[%d][%d][%d][0][0][0] = %d, "
                       "but earlier elements of outer tile were unvisited\n",
                       gangIdx, workerIdx, iTile, innerActual0);
                return true;
              }
              ++tileVisits[iTile][iElem][jTile];
              ++gangWorkerVisitsInner[gangIdx][workerIdx];
              innerTileVisited = true;
              outerTileVisited = true;
            } else if (!innerWorkerPartitioned && jTile > 0 &&
                       outerTileVisited) {
              printf("  error: indices[%d][%d][%d][0][0][0] = %d, "
                     "but earlier elements of outer tile were visited\n",
                     gangIdx, workerIdx, iTile, innerActual0);
              return true;
            }
            for (int jElem = 0; jElem < J_NELEMS; ++jElem) {
              int actual =
                order->indices[gangIdx][workerIdx][iTile][jTile][iElem][jElem];
              if (innerTileVisited) {
                if (actual != nextExpected) {
                  printf("  error: indices[%d][%d][%d][%d][%d][%d] = %d, "
                         "but expected %d\n",
                         gangIdx, workerIdx, iTile, jTile, iElem, jElem, actual,
                         nextExpected);
                  return true;
                }
                ++nextExpected;
              } else if (actual >= 0) {
                printf("  error: indices[%d][%d][%d][%d][%d][%d] = %d, "
                       "but earlier elements of inner tile were unvisited\n",
                       gangIdx, workerIdx, iTile, jTile, iElem, jElem, actual);
                return true;
              }
            }
          }
        }
        if (outerTileVisited)
          ++gangWorkerVisitsOuter[gangIdx][workerIdx];
      }
    }
  }
  bool err = false;
  for (int iTile = 0; iTile < I_NTILES; ++iTile) {
    for (int iElem = 0; iElem < I_NELEMS; ++iElem) {
      for (int jTile = 0; jTile < J_NTILES; ++jTile) {
        int expected = gangPartitioned ? 1 : NUM_GANGS;
        int actual = tileVisits[iTile][iElem][jTile];
        if (actual != expected) {
          printf("  error: tileVisits[%d][%d][%d] = %d, expected = %d\n",
                 iTile, iElem, jTile, actual, expected);
          err =  true;
        }
      }
    }
  }
  for (int gangIdx = 0; gangIdx < NUM_GANGS; ++gangIdx) {
    for (int workerIdx = 0; workerIdx < NUM_WORKERS; ++workerIdx) {
      int actualOuter = gangWorkerVisitsOuter[gangIdx][workerIdx];
      int actualInner = gangWorkerVisitsInner[gangIdx][workerIdx];
      int expectedOuter = I_NTILES;
      int expectedInner = I_NTILES * I_NELEMS * J_NTILES;
      if (gangPartitioned) {
        expectedOuter /= NUM_GANGS;
        expectedInner /= NUM_GANGS;
      }
      if (workerPartitioned) {
        expectedOuter /= NUM_WORKERS;
        expectedInner /= NUM_WORKERS;
      } else if (innerWorkerPartitioned) {
        expectedInner /= NUM_WORKERS;
      } else if (workerIdx > 0) {
        expectedOuter = 0;
        expectedInner = 0;
      }
      if (actualOuter != expectedOuter) {
        printf("  error: gangWorkerVisitsOuter[%d][%d] = %d, expected = %d\n",
               gangIdx, workerIdx, actualOuter, expectedOuter);
        err =  true;
      }
      if (actualInner != expectedInner) {
        printf("  error: gangWorkerVisitsInner[%d][%d] = %d, expected = %d\n",
               gangIdx, workerIdx, actualInner, expectedInner);
        err =  true;
      }
    }
  }
  return err;
}

#pragma acc routine gang
static void expSeqWithinGangFn(Order *order);
#pragma acc routine gang
static void expAutoWithinGangFn(Order *order);
#pragma acc routine gang
static void expGangWithinGangFn(Order *order);
#pragma acc routine gang
static void impGangWithinGangFn(Order *order);
#pragma acc routine gang
static void expWorkerWithinGangFn(Order *order);
#pragma acc routine gang
static void expVectorWithinGangFn(Order *order);
#pragma acc routine gang
static void expWorkerVectorWithinGangFn(Order *order);
#pragma acc routine worker
static void expSeqWithinWorkerFn(Order *order);
#pragma acc routine worker
static void expAutoWithinWorkerFn(Order *order);
#pragma acc routine worker
static void expWorkerWithinWorkerFn(Order *order);
#pragma acc routine worker
static void expVectorWithinWorkerFn(Order *order);
#pragma acc routine worker
static void expWorkerVectorWithinWorkerFn(Order *order);
#pragma acc routine vector
static void expSeqWithinVectorFn(Order *order);
#pragma acc routine vector
static void expAutoWithinVectorFn(Order *order);
#pragma acc routine vector
static void expVectorWithinVectorFn(Order *order);
#pragma acc routine seq
static void expSeqWithinSeqFn(Order *order);
#pragma acc routine seq
static void expAutoWithinSeqFn(Order *order);

//------------------------------------------------------------------------------
// Check tile clauses within parallel constructs.
//------------------------------------------------------------------------------

int main() {
  Order order;

  //............................................................................
  // Explicit seq.
  //............................................................................

  // DMP-LABEL:"expSeqWithinSepPar\n"
  // PRT-LABEL:"expSeqWithinSepPar\n"
  // EXE-LABEL:expSeqWithinSepPar
  printf("expSeqWithinSepPar\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCSeqClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   impl: OMPTileDirective
  //  DMP-NEXT:     OMPSizesClause
  //  DMP-NEXT:       IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:       IntegerLiteral {{.*}} [[#J_NELEMS]]
  //       DMP:     ForStmt
  //       DMP:       ForStmt
  //
  //    PRT-NEXT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}){{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  //
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     setOrder(&order, i, j);
  //
  // EXE-NEXT:  verified
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS)
  #pragma acc loop seq tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(&order, i, j);
  if (!checkOrder2D(&order, /*gangPartitioned=*/false,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  // DMP-LABEL:"expSeqWithinCmbPar\n"
  // PRT-LABEL:"expSeqWithinCmbPar\n"
  // EXE-LABEL:expSeqWithinCmbPar
  printf("expSeqWithinCmbPar\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCSeqClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   impl: OMPTileDirective
  //  DMP-NEXT:     OMPSizesClause
  //  DMP-NEXT:       IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:       IntegerLiteral {{.*}} [[#J_NELEMS]]
  //       DMP:     ForStmt
  //       DMP:       ForStmt
  //
  //    PRT-NEXT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs({{NUM_GANGS|[0-9]+}}) seq tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs({{NUM_GANGS|[0-9]+}}) seq tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  //
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     setOrder(&order, i, j);
  //
  // EXE-NEXT:  verified
  initOrder(&order);
  #pragma acc parallel loop num_gangs(NUM_GANGS) seq tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(&order, i, j);
  if (!checkOrder2D(&order, /*gangPartitioned=*/false,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  //............................................................................
  // Explicit auto.
  //............................................................................

  // DMP-LABEL:"expAutoWithinSepPar\n"
  // PRT-LABEL:"expAutoWithinSepPar\n"
  // EXE-LABEL:expAutoWithinSepPar
  printf("expAutoWithinSepPar\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCAutoClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   impl: OMPTileDirective
  //  DMP-NEXT:     OMPSizesClause
  //  DMP-NEXT:       IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:       IntegerLiteral {{.*}} [[#J_NELEMS]]
  //       DMP:     ForStmt
  //       DMP:       ForStmt
  //
  //    PRT-NEXT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}){{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop auto tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop auto tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  //
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     setOrder(&order, i, j);
  //
  // EXE-NEXT:  verified
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS)
  #pragma acc loop auto tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(&order, i, j);
  if (!checkOrder2D(&order, /*gangPartitioned=*/false,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  // DMP-LABEL:"expAutoWithinCmbPar\n"
  // PRT-LABEL:"expAutoWithinCmbPar\n"
  // EXE-LABEL:expAutoWithinCmbPar
  printf("expAutoWithinCmbPar\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCAutoClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   impl: OMPTileDirective
  //  DMP-NEXT:     OMPSizesClause
  //  DMP-NEXT:       IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:       IntegerLiteral {{.*}} [[#J_NELEMS]]
  //       DMP:     ForStmt
  //       DMP:       ForStmt
  //
  //    PRT-NEXT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs({{NUM_GANGS|[0-9]+}}) auto tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs({{NUM_GANGS|[0-9]+}}) auto tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  //
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     setOrder(&order, i, j);
  //
  // EXE-NEXT:  verified
  initOrder(&order);
  #pragma acc parallel loop num_gangs(NUM_GANGS) auto tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(&order, i, j);
  if (!checkOrder2D(&order, /*gangPartitioned=*/false,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  //............................................................................
  // Explicit gang partitioning.
  //............................................................................

  // DMP-LABEL:"expGangWithinSepPar\n"
  // PRT-LABEL:"expGangWithinSepPar\n"
  // EXE-LABEL:expGangWithinSepPar
  printf("expGangWithinSepPar\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCGangClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
  //  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   impl: OMPDistributeDirective
  //  DMP-NEXT:     OMPCollapseClause
  //  DMP-NEXT:       ConstantExpr {{.*}} 'int'
  //  DMP-NEXT:         value: Int 2
  //  DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //       DMP:     OMPTileDirective
  //  DMP-NEXT:       OMPSizesClause
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#J_NELEMS]]
  //       DMP:       ForStmt
  //       DMP:         ForStmt
  //
  //    PRT-NEXT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}){{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop gang tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute collapse(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop gang tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  //
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     setOrder(&order, i, j);
  //
  // EXE-NEXT:  verified
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS)
  #pragma acc loop gang tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(&order, i, j);
  if (!checkOrder2D(&order, /*gangPartitioned=*/true,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  // DMP-LABEL:"expGangWithinCmbPar\n"
  // PRT-LABEL:"expGangWithinCmbPar\n"
  // EXE-LABEL:expGangWithinCmbPar
  printf("expGangWithinCmbPar\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCGangClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
  //  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   impl: OMPDistributeDirective
  //  DMP-NEXT:     OMPCollapseClause
  //  DMP-NEXT:       ConstantExpr {{.*}} 'int'
  //  DMP-NEXT:         value: Int 2
  //  DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //       DMP:     OMPTileDirective
  //  DMP-NEXT:       OMPSizesClause
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#J_NELEMS]]
  //       DMP:       ForStmt
  //       DMP:         ForStmt
  //
  //    PRT-NEXT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs({{NUM_GANGS|[0-9]+}}) gang tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute collapse(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs({{NUM_GANGS|[0-9]+}}) gang tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  //
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     setOrder(&order, i, j);
  //
  // EXE-NEXT:  verified
  initOrder(&order);
  #pragma acc parallel loop num_gangs(NUM_GANGS) gang tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(&order, i, j);
  if (!checkOrder2D(&order, /*gangPartitioned=*/true,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  //............................................................................
  // Implicit gang partitioning.
  //............................................................................

  // DMP-LABEL:"impGangWithinSepPar\n"
  // PRT-LABEL:"impGangWithinSepPar\n"
  // EXE-LABEL:impGangWithinSepPar
  printf("impGangWithinSepPar\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
  //  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  //  DMP-NEXT:   impl: OMPDistributeDirective
  //  DMP-NEXT:     OMPCollapseClause
  //  DMP-NEXT:       ConstantExpr {{.*}} 'int'
  //  DMP-NEXT:         value: Int 2
  //  DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //       DMP:     OMPTileDirective
  //  DMP-NEXT:       OMPSizesClause
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#J_NELEMS]]
  //       DMP:       ForStmt
  //       DMP:         ForStmt
  //
  //    PRT-NEXT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}){{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute collapse(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  //
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     setOrder(&order, i, j);
  //
  // EXE-NEXT:  verified
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS)
  #pragma acc loop tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(&order, i, j);
  if (!checkOrder2D(&order, /*gangPartitioned=*/true,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  // DMP-LABEL:"impGangWithinCmbPar\n"
  // PRT-LABEL:"impGangWithinCmbPar\n"
  // EXE-LABEL:impGangWithinCmbPar
  printf("impGangWithinCmbPar\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
  //  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  //  DMP-NEXT:   impl: OMPDistributeDirective
  //  DMP-NEXT:     OMPCollapseClause
  //  DMP-NEXT:       ConstantExpr {{.*}} 'int'
  //  DMP-NEXT:         value: Int 2
  //  DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //       DMP:     OMPTileDirective
  //  DMP-NEXT:       OMPSizesClause
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#J_NELEMS]]
  //       DMP:       ForStmt
  //       DMP:         ForStmt
  //
  //    PRT-NEXT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs({{NUM_GANGS|[0-9]+}}) tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute collapse(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs({{NUM_GANGS|[0-9]+}}) tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  //
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     setOrder(&order, i, j);
  //
  // EXE-NEXT:  verified
  initOrder(&order);
  #pragma acc parallel loop num_gangs(NUM_GANGS) tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(&order, i, j);
  if (!checkOrder2D(&order, /*gangPartitioned=*/true,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  //............................................................................
  // Worker partitioning.
  //............................................................................

  // DMP-LABEL:"expWorkerWithinSepPar\n"
  // PRT-LABEL:"expWorkerWithinSepPar\n"
  // EXE-LABEL:expWorkerWithinSepPar
  printf("expWorkerWithinSepPar\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCWorkerClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
  //  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  //  DMP-NEXT:   impl: OMPDistributeDirective
  //  DMP-NEXT:     OMPCollapseClause
  //  DMP-NEXT:       ConstantExpr {{.*}} 'int'
  //  DMP-NEXT:         value: Int 2
  //  DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //       DMP:     OMPTileDirective
  //  DMP-NEXT:       OMPSizesClause
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#J_NELEMS]]
  //       DMP:       ForStmt
  //       DMP:         ForStmt
  //
  //    PRT-NEXT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}) num_workers({{NUM_WORKERS|[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) thread_limit({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) thread_limit({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}) num_workers({{NUM_WORKERS|[0-9]+}}){{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute collapse(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  //
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     setOrder(&order, i, j);
  //
  // EXE-NEXT:  verified
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS)
  #pragma acc loop worker tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(&order, i, j);
  if (!checkOrder2D(&order, /*gangPartitioned=*/true,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  // DMP-LABEL:"expWorkerWithinCmbPar\n"
  // PRT-LABEL:"expWorkerWithinCmbPar\n"
  // EXE-LABEL:expWorkerWithinCmbPar
  printf("expWorkerWithinCmbPar\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCWorkerClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
  //  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  //  DMP-NEXT:   impl: OMPDistributeDirective
  //  DMP-NEXT:     OMPCollapseClause
  //  DMP-NEXT:       ConstantExpr {{.*}} 'int'
  //  DMP-NEXT:         value: Int 2
  //  DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //       DMP:     OMPTileDirective
  //  DMP-NEXT:       OMPSizesClause
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#J_NELEMS]]
  //       DMP:       ForStmt
  //       DMP:         ForStmt
  //
  //    PRT-NEXT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs({{NUM_GANGS|[0-9]+}}) num_workers({{NUM_WORKERS|[0-9]+}}) worker tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) thread_limit({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) thread_limit({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute collapse(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs({{NUM_GANGS|[0-9]+}}) num_workers({{NUM_WORKERS|[0-9]+}}) worker tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  //
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     setOrder(&order, i, j);
  //
  // EXE-NEXT:  verified
  initOrder(&order);
  #pragma acc parallel loop num_gangs(NUM_GANGS) num_workers(NUM_WORKERS) worker tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(&order, i, j);
  if (!checkOrder2D(&order, /*gangPartitioned=*/true,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  //............................................................................
  // Vector partitioning.
  //............................................................................

  // DMP-LABEL:"expVectorWithinSepPar\n"
  // PRT-LABEL:"expVectorWithinSepPar\n"
  // EXE-LABEL:expVectorWithinSepPar
  printf("expVectorWithinSepPar\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCVectorClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
  //  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  //  DMP-NEXT:   impl: OMPDistributeDirective
  //  DMP-NEXT:     OMPCollapseClause
  //  DMP-NEXT:       ConstantExpr {{.*}} 'int'
  //  DMP-NEXT:         value: Int 2
  //  DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //       DMP:     OMPTileDirective
  //  DMP-NEXT:       OMPSizesClause
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#J_NELEMS]]
  //       DMP:       ForStmt
  //       DMP:         ForStmt
  //
  //    PRT-NEXT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}) vector_length({{VECTOR_LENGTH|[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}) vector_length({{VECTOR_LENGTH|[0-9]+}}){{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop vector tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute collapse(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop vector tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  //
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     setOrder(&order, i, j);
  //
  // EXE-NEXT:  verified
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS) vector_length(VECTOR_LENGTH)
  #pragma acc loop vector tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(&order, i, j);
  if (!checkOrder2D(&order, /*gangPartitioned=*/true,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  // DMP-LABEL:"expVectorWithinCmbPar\n"
  // PRT-LABEL:"expVectorWithinCmbPar\n"
  // EXE-LABEL:expVectorWithinCmbPar
  printf("expVectorWithinCmbPar\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCVectorClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
  //  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  //  DMP-NEXT:   impl: OMPDistributeDirective
  //  DMP-NEXT:     OMPCollapseClause
  //  DMP-NEXT:       ConstantExpr {{.*}} 'int'
  //  DMP-NEXT:         value: Int 2
  //  DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //       DMP:     OMPTileDirective
  //  DMP-NEXT:       OMPSizesClause
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#J_NELEMS]]
  //       DMP:       ForStmt
  //       DMP:         ForStmt
  //
  //    PRT-NEXT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs({{NUM_GANGS|[0-9]+}}) vector_length({{VECTOR_LENGTH|[0-9]+}}) vector tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute collapse(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs({{NUM_GANGS|[0-9]+}}) vector_length({{VECTOR_LENGTH|[0-9]+}}) vector tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  //
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     setOrder(&order, i, j);
  //
  // EXE-NEXT:  verified
  initOrder(&order);
  #pragma acc parallel loop num_gangs(NUM_GANGS) vector_length(VECTOR_LENGTH) vector tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(&order, i, j);
  if (!checkOrder2D(&order, /*gangPartitioned=*/true,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  //............................................................................
  // Worker and vector partitioning.
  //............................................................................

  // DMP-LABEL:"expWorkerVectorWithinSepPar\n"
  // PRT-LABEL:"expWorkerVectorWithinSepPar\n"
  // EXE-LABEL:expWorkerVectorWithinSepPar
  printf("expWorkerVectorWithinSepPar\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCWorkerClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCVectorClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
  //  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  //  DMP-NEXT:   impl: OMPDistributeParallelForDirective
  //  DMP-NEXT:     OMPCollapseClause
  //  DMP-NEXT:       ConstantExpr {{.*}} 'int'
  //  DMP-NEXT:         value: Int 2
  //  DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //       DMP:     OMPTileDirective
  //  DMP-NEXT:       OMPSizesClause
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#J_NELEMS]]
  //       DMP:       ForStmt
  //       DMP:         ForStmt
  //
  //    PRT-NEXT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}) num_workers({{NUM_WORKERS|[0-9]+}}) vector_length({{VECTOR_LENGTH|[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) thread_limit({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) thread_limit({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}) num_workers({{NUM_WORKERS|[0-9]+}}) vector_length({{VECTOR_LENGTH|[0-9]+}}){{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for collapse(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  //
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     setOrder(&order, i, j);
  //
  // EXE-NEXT:  verified
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS) vector_length(VECTOR_LENGTH)
  #pragma acc loop worker vector tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(&order, i, j);
  if (!checkOrder2D(&order, /*gangPartitioned=*/true,
                    /*workerPartitioned=*/true))
    printf("  verified\n");

  // DMP-LABEL:"expWorkerVectorWithinCmbPar\n"
  // PRT-LABEL:"expWorkerVectorWithinCmbPar\n"
  // EXE-LABEL:expWorkerVectorWithinCmbPar
  printf("expWorkerVectorWithinCmbPar\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCWorkerClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCVectorClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
  //  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  //  DMP-NEXT:   impl: OMPDistributeParallelForDirective
  //  DMP-NEXT:     OMPCollapseClause
  //  DMP-NEXT:       ConstantExpr {{.*}} 'int'
  //  DMP-NEXT:         value: Int 2
  //  DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //       DMP:     OMPTileDirective
  //  DMP-NEXT:       OMPSizesClause
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#J_NELEMS]]
  //       DMP:       ForStmt
  //       DMP:         ForStmt
  //
  //    PRT-NEXT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs({{NUM_GANGS|[0-9]+}}) num_workers({{NUM_WORKERS|[0-9]+}}) vector_length({{VECTOR_LENGTH|[0-9]+}}) worker vector tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) thread_limit({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) thread_limit({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for collapse(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs({{NUM_GANGS|[0-9]+}}) num_workers({{NUM_WORKERS|[0-9]+}}) vector_length({{VECTOR_LENGTH|[0-9]+}}) worker vector tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  //
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     setOrder(&order, i, j);
  //
  // EXE-NEXT:  verified
  initOrder(&order);
  #pragma acc parallel loop num_gangs(NUM_GANGS) num_workers(NUM_WORKERS) vector_length(VECTOR_LENGTH) worker vector tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(&order, i, j);
  if (!checkOrder2D(&order, /*gangPartitioned=*/true,
                    /*workerPartitioned=*/true))
    printf("  verified\n");

  //............................................................................
  // 1-D tiling with one more loop than required by tile clause.
  //
  // No redundant collapse(1) clause is generated for 1-D tiling.
  //............................................................................

  // DMP-LABEL:"seq1D\n"
  // PRT-LABEL:"seq1D\n"
  // EXE-LABEL:seq1D
  printf("seq1D\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCSeqClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   impl: OMPTileDirective
  //  DMP-NEXT:     OMPSizesClause
  //  DMP-NEXT:       IntegerLiteral {{.*}} [[#I_NELEMS]]
  //       DMP:     ForStmt
  //       DMP:       ForStmt
  //
  //    PRT-NEXT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}){{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq tile({{I_NELEMS|[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]]){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp tile sizes([[#I_NELEMS]]){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq tile({{I_NELEMS|[0-9]+}}){{$}}
  //
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     setOrder(&order, i, j);
  //
  // EXE-NEXT:  verified
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS)
  #pragma acc loop seq tile(I_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(&order, i, j);
  if (!checkOrder1D(&order, /*gangPartitioned=*/false,
                    /*workerPartitioned=*/false,
                    /*innerWorkerPartitioned=*/false))
    printf("  verified\n");

  // DMP-LABEL:"gang1D\n"
  // PRT-LABEL:"gang1D\n"
  // EXE-LABEL:gang1D
  printf("gang1D\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  //  DMP-NEXT:   impl: OMPDistributeDirective
  //   DMP-NOT:     OMPCollapseClause
  //       DMP:     OMPTileDirective
  //  DMP-NEXT:       OMPSizesClause
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#I_NELEMS]]
  //       DMP:       ForStmt
  //       DMP:         ForStmt
  //
  //    PRT-NEXT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}){{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop tile({{I_NELEMS|[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]]){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp tile sizes([[#I_NELEMS]]){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop tile({{I_NELEMS|[0-9]+}}){{$}}
  //
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     setOrder(&order, i, j);
  //
  // EXE-NEXT:  verified
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS)
  #pragma acc loop tile(I_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(&order, i, j);
  if (!checkOrder1D(&order, /*gangPartitioned=*/true,
                    /*workerPartitioned=*/false,
                    /*innerWorkerPartitioned=*/false))
    printf("  verified\n");

  // DMP-LABEL:"gangWorkerVector1D\n"
  // PRT-LABEL:"gangWorkerVector1D\n"
  // EXE-LABEL:gangWorkerVector1D
  printf("gangWorkerVector1D\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCWorkerClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCVectorClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  //  DMP-NEXT:   impl: OMPDistributeParallelForDirective
  //   DMP-NOT:     OMPCollapseClause
  //       DMP:     OMPTileDirective
  //  DMP-NEXT:       OMPSizesClause
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#I_NELEMS]]
  //       DMP:       ForStmt
  //       DMP:         ForStmt
  //
  //    PRT-NEXT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}) num_workers({{NUM_WORKERS|[0-9]+}}) vector_length({{VECTOR_LENGTH|[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) thread_limit({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) thread_limit({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}) num_workers({{NUM_WORKERS|[0-9]+}}) vector_length({{VECTOR_LENGTH|[0-9]+}}){{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector tile({{I_NELEMS|[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]]){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp tile sizes([[#I_NELEMS]]){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector tile({{I_NELEMS|[0-9]+}}){{$}}
  //
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     setOrder(&order, i, j);
  //
  // EXE-NEXT:  verified
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS) vector_length(VECTOR_LENGTH)
  #pragma acc loop worker vector tile(I_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(&order, i, j);
  if (!checkOrder1D(&order, /*gangPartitioned=*/true,
                    /*workerPartitioned=*/true,
                    /*innerWorkerPartitioned=*/false))
    printf("  verified\n");

  //............................................................................
  // 1-D tiling containing 1-D tiling.
  //
  // No redundant collapse(1) clause is generated for 1-D tiling.
  //............................................................................

  // DMP-LABEL:"seq1Dseq1D\n"
  // PRT-LABEL:"seq1Dseq1D\n"
  // EXE-LABEL:seq1Dseq1D
  printf("seq1Dseq1D\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCSeqClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   impl: OMPTileDirective
  //  DMP-NEXT:     OMPSizesClause
  //  DMP-NEXT:       IntegerLiteral {{.*}} [[#I_NELEMS]]
  //       DMP:     ForStmt
  //       DMP:   ForStmt
  //       DMP:     ACCLoopDirective
  //  DMP-NEXT:       ACCSeqClause
  //   DMP-NOT:         <implicit>
  //  DMP-NEXT:       ACCTileClause
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#J_NELEMS]]
  //  DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:         DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:         DeclRefExpr {{.*}} 'i'
  //  DMP-NEXT:       impl: OMPTileDirective
  //  DMP-NEXT:         OMPSizesClause
  //  DMP-NEXT:           IntegerLiteral {{.*}} [[#J_NELEMS]]
  //       DMP:         ForStmt
  //       DMP:       ForStmt
  //
  //    PRT-NEXT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}){{$}}
  //
  //  PRT-A-NEXT:   {{^ *}}#pragma acc loop seq tile({{I_NELEMS|[0-9]+}}){{$}}
  // PRT-AO-NEXT:   {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]]){{$}}
  //  PRT-O-NEXT:   {{^ *}}#pragma omp tile sizes([[#I_NELEMS]]){{$}}
  // PRT-OA-NEXT:   {{^ *}}// #pragma acc loop seq tile({{I_NELEMS|[0-9]+}}){{$}}
  //
  //    PRT-NEXT: for (int i ={{.*}})
  //
  //  PRT-A-NEXT:   {{^ *}}#pragma acc loop seq tile({{J_NELEMS|[0-9]+}}){{$}}
  // PRT-AO-NEXT:   {{^ *}}// #pragma omp tile sizes([[#J_NELEMS]]){{$}}
  //  PRT-O-NEXT:   {{^ *}}#pragma omp tile sizes([[#J_NELEMS]]){{$}}
  // PRT-OA-NEXT:   {{^ *}}// #pragma acc loop seq tile({{J_NELEMS|[0-9]+}}){{$}}
  //
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     setOrder(&order, i, j);
  //
  // EXE-NEXT:  verified
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS)
  #pragma acc loop seq tile(I_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    #pragma acc loop seq tile(J_NELEMS)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(&order, i, j);
  if (!checkOrder1D(&order, /*gangPartitioned=*/false,
                    /*workerPartitioned=*/false,
                    /*innerWorkerPartitioned=*/false))
    printf("  verified\n");

  // DMP-LABEL:"gang1Dseq1D\n"
  // PRT-LABEL:"gang1Dseq1D\n"
  // EXE-LABEL:gang1Dseq1D
  printf("gang1Dseq1D\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  //  DMP-NEXT:   impl: OMPDistributeDirective
  //   DMP-NOT:     OMPCollapseClause
  //       DMP:     OMPTileDirective
  //  DMP-NEXT:       OMPSizesClause
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#I_NELEMS]]
  //       DMP:       ForStmt
  //       DMP:   ForStmt
  //       DMP:     ACCLoopDirective
  //  DMP-NEXT:       ACCSeqClause
  //   DMP-NOT:         <implicit>
  //  DMP-NEXT:       ACCTileClause
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#J_NELEMS]]
  //  DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:         DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:         DeclRefExpr {{.*}} 'i'
  //  DMP-NEXT:       impl: OMPTileDirective
  //  DMP-NEXT:         OMPSizesClause
  //  DMP-NEXT:           IntegerLiteral {{.*}} [[#J_NELEMS]]
  //       DMP:         ForStmt
  //       DMP:       ForStmt
  //
  //    PRT-NEXT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}){{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop tile({{I_NELEMS|[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]]){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp tile sizes([[#I_NELEMS]]){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop tile({{I_NELEMS|[0-9]+}}){{$}}
  //
  //    PRT-NEXT: for (int i ={{.*}})
  //
  //  PRT-A-NEXT:   {{^ *}}#pragma acc loop seq tile({{J_NELEMS|[0-9]+}}){{$}}
  // PRT-AO-NEXT:   {{^ *}}// #pragma omp tile sizes([[#J_NELEMS]]){{$}}
  //  PRT-O-NEXT:   {{^ *}}#pragma omp tile sizes([[#J_NELEMS]]){{$}}
  // PRT-OA-NEXT:   {{^ *}}// #pragma acc loop seq tile({{J_NELEMS|[0-9]+}}){{$}}
  //
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     setOrder(&order, i, j);
  //
  // EXE-NEXT:  verified
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS)
  #pragma acc loop tile(I_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    #pragma acc loop seq tile(J_NELEMS)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(&order, i, j);
  if (!checkOrder1D(&order, /*gangPartitioned=*/true,
                    /*workerPartitioned=*/false,
                    /*innerWorkerPartitioned=*/false))
    printf("  verified\n");

  // DMP-LABEL:"gang1DworkerVector1D\n"
  // PRT-LABEL:"gang1DworkerVector1D\n"
  // EXE-LABEL:gang1DworkerVector1D
  printf("gang1DworkerVector1D\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  //  DMP-NEXT:   impl: OMPDistributeDirective
  //   DMP-NOT:     OMPCollapseClause
  //       DMP:     OMPTileDirective
  //  DMP-NEXT:       OMPSizesClause
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#I_NELEMS]]
  //       DMP:       ForStmt
  //       DMP:   ForStmt
  //       DMP:     ACCLoopDirective
  //  DMP-NEXT:       ACCWorkerClause
  //   DMP-NOT:         <implicit>
  //  DMP-NEXT:       ACCVectorClause
  //   DMP-NOT:         <implicit>
  //  DMP-NEXT:       ACCTileClause
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#J_NELEMS]]
  //  DMP-NEXT:       ACCIndependentClause {{.*}} <implicit>
  //  DMP-NEXT:       ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:         DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:         DeclRefExpr {{.*}} 'i'
  //  DMP-NEXT:       impl: OMPParallelForDirective
  //       DMP:         OMPTileDirective
  //  DMP-NEXT:           OMPSizesClause
  //  DMP-NEXT:             IntegerLiteral {{.*}} [[#J_NELEMS]]
  //       DMP:           ForStmt
  //       DMP:             ForStmt
  //
  //    PRT-NEXT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}) num_workers({{NUM_WORKERS|[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) thread_limit({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) thread_limit({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}) num_workers({{NUM_WORKERS|[0-9]+}}){{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop tile({{I_NELEMS|[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute{{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]]){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute{{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp tile sizes([[#I_NELEMS]]){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop tile({{I_NELEMS|[0-9]+}}){{$}}
  //
  //    PRT-NEXT: for (int i ={{.*}})
  //
  //  PRT-A-NEXT:   {{^ *}}#pragma acc loop worker vector tile({{J_NELEMS|[0-9]+}}){{$}}
  // PRT-AO-NEXT:   {{^ *}}// #pragma omp parallel for{{$}}
  // PRT-AO-NEXT:   {{^ *}}// #pragma omp tile sizes([[#J_NELEMS]]){{$}}
  //  PRT-O-NEXT:   {{^ *}}#pragma omp parallel for{{$}}
  //  PRT-O-NEXT:   {{^ *}}#pragma omp tile sizes([[#J_NELEMS]]){{$}}
  // PRT-OA-NEXT:   {{^ *}}// #pragma acc loop worker vector tile({{J_NELEMS|[0-9]+}}){{$}}
  //
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     setOrder(&order, i, j);
  //
  // EXE-NEXT:  verified
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS)
  #pragma acc loop tile(I_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    #pragma acc loop worker vector tile(J_NELEMS)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(&order, i, j);
  if (!checkOrder1D(&order, /*gangPartitioned=*/true,
                    /*workerPartitioned=*/false,
                    /*innerWorkerPartitioned=*/true))
    printf("  verified\n");

  //............................................................................
  // Visibility of loop control variables that are assigned not declared in init
  // of for loop that is attached via tile.
  //
  // On explicit seq loops, it's shared.  On partitioned loops, it's
  // predetermined private.
  //............................................................................

  // DMP-LABEL:"expSeqMappedLoopVar\n"
  // PRT-LABEL:"expSeqMappedLoopVar\n"
  // EXE-LABEL:expSeqMappedLoopVar
  printf("expSeqMappedLoopVar\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCSeqClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'i'
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'j'
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   impl: OMPTileDirective
  //  DMP-NEXT:     OMPSizesClause
  //  DMP-NEXT:       IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:       IntegerLiteral {{.*}} [[#J_NELEMS]]
  //       DMP:     ForStmt
  //       DMP:       ForStmt
  //
  //         PRT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs(1) copy(i,j){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(1) map(ompx_hold,tofrom: i,j) map(ompx_hold,tofrom: order){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams(1) map(ompx_hold,tofrom: i,j) map(ompx_hold,tofrom: order){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(1) copy(i,j){{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  //
  //    PRT-NEXT: for (i ={{.*}})
  //    PRT-NEXT:   for (j ={{.*}})
  //    PRT-NEXT:     setOrder(&order, i, j);
  //
  // EXE-NEXT:  order verified
  // EXE-NEXT:  i verified
  // EXE-NEXT:  j verified
  {
    int i = 98;
    int j = 99;
    initOrder(&order);
    #pragma acc parallel num_gangs(1) copy(i,j)
    #pragma acc loop seq tile(I_NELEMS,J_NELEMS)
    for (i = 0; i < I_NTILES * I_NELEMS; ++i)
      for (j = 0; j < J_NTILES * J_NELEMS; ++j)
        setOrder(&order, i, j);
    if (!checkOrder2DFull(&order, /*gangPartitioned=*/false,
                          /*workerPartitioned=*/false, /*numGangs=*/1, I_NTILES,
                          J_NTILES, I_NELEMS, J_NELEMS))
      printf("  order verified\n");
    // FIXME: The loop variables end up with the final iteration's value rather
    // than +1 as they would without tiling.
    int expected = I_NTILES * I_NELEMS - 1;
    if (i != expected)
      printf("  error: i = %d, expected %d\n", i, expected);
    else
      printf("  i verified\n");
    expected = J_NTILES * J_NELEMS - 1;
    if (j != expected)
      printf("  error: j = %d, expected %d\n", j, expected);
    else
      printf("  j verified\n");
  }

  // DMP-LABEL:"expSeqUnmappedLoopVar\n"
  // PRT-LABEL:"expSeqUnmappedLoopVar\n"
  // EXE-LABEL:expSeqUnmappedLoopVar
  printf("expSeqUnmappedLoopVar\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCSeqClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'i'
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'j'
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   impl: OMPTileDirective
  //  DMP-NEXT:     OMPSizesClause
  //  DMP-NEXT:       IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:       IntegerLiteral {{.*}} [[#J_NELEMS]]
  //       DMP:     ForStmt
  //       DMP:       ForStmt
  //
  //         PRT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order,iPerGang,jPerGang) firstprivate(i,j){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order,iPerGang,jPerGang) firstprivate(i,j){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}){{$}}
  //    PRT-NEXT: {
  //  PRT-A-NEXT:   {{^ *}}#pragma acc loop seq tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  // PRT-AO-NEXT:   {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  //  PRT-O-NEXT:   {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  // PRT-OA-NEXT:   {{^ *}}// #pragma acc loop seq tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  //    PRT-NEXT:   for (i ={{.*}})
  //    PRT-NEXT:     for (j ={{.*}})
  //    PRT-NEXT:       setOrder(&order, i, j);
  //         PRT: }
  //
  // EXE-NEXT:  order verified
  // EXE-NEXT:  i = 98
  // EXE-NEXT:  j = 99
  {
    int i = 98;
    int j = 99;
    int iPerGang[NUM_GANGS];
    int jPerGang[NUM_GANGS];
    initOrder(&order);
    #pragma acc parallel num_gangs(NUM_GANGS)
    {
      #pragma acc loop seq tile(I_NELEMS,J_NELEMS)
      for (i = 0; i < I_NTILES * I_NELEMS; ++i)
        for (j = 0; j < J_NTILES * J_NELEMS; ++j)
          setOrder(&order, i, j);
      int omp_get_team_num();
      int gangIdx = omp_get_team_num();
      iPerGang[gangIdx] = i;
      jPerGang[gangIdx] = j;
    }
    if (!checkOrder2D(&order, /*gangPartitioned=*/false,
                      /*workerPartitioned=*/false))
      printf("  order verified\n");
    printf("  i = %d\n", i);
    printf("  j = %d\n", j);
    // FIXME: The loop variables end up with the final iteration's value rather
    // than +1 as they would without tiling.
    int iExpected = I_NTILES * I_NELEMS - 1;
    int jExpected = J_NTILES * J_NELEMS - 1;
    for (int gangIdx = 0; gangIdx < NUM_GANGS; ++gangIdx) {
      if (iPerGang[gangIdx] != iExpected)
        printf("  error: iPerGang[%d] = %d, expected %d\n",
               gangIdx, iPerGang[gangIdx], iExpected);
      if (jPerGang[gangIdx] != jExpected)
        printf("  error: jPerGang[%d] = %d, expected %d\n",
               gangIdx, jPerGang[gangIdx], jExpected);
    }
  }

  // DMP-LABEL:"expAutoMappedLoopVar\n"
  // PRT-LABEL:"expAutoMappedLoopVar\n"
  // EXE-LABEL:expAutoMappedLoopVar
  printf("expAutoMappedLoopVar\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCAutoClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
  //  DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'i'
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'j'
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   impl: CompoundStmt
  //  DMP-NEXT:     DeclStmt
  //  DMP-NEXT:       VarDecl {{.*}} i 'int'
  //  DMP-NEXT:     DeclStmt
  //  DMP-NEXT:       VarDecl {{.*}} j 'int'
  //  DMP-NEXT:     OMPTileDirective
  //  DMP-NEXT:       OMPSizesClause
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#J_NELEMS]]
  //       DMP:       ForStmt
  //       DMP:         ForStmt
  //
  //         PRT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}) copy(i,j){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: i,j) map(ompx_hold,tofrom: order,iPerGang,jPerGang){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: i,j) map(ompx_hold,tofrom: order,iPerGang,jPerGang){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}) copy(i,j){{$}}
  //    PRT-NEXT: {
  //
  // PRT-AO-NEXT:   {{^ *}}// v----------ACC----------v{{$}}
  //  PRT-A-NEXT:   {{^ *}}#pragma acc loop auto tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  //  PRT-A-NEXT:   {{^ *}}for (i ={{.*}})
  //  PRT-A-NEXT:   {{^ *}}  for (j ={{.*}})
  //  PRT-A-NEXT:   {{^ *}}    setOrder(&order, i, j);
  // PRT-AO-NEXT:   {{^ *}}// ---------ACC->OMP--------{{$}}
  // PRT-AO-NEXT:   {{^ *}}// {
  // PRT-AO-NEXT:   {{^ *}}//   int i;
  // PRT-AO-NEXT:   {{^ *}}//   int j;
  // PRT-AO-NEXT:   {{^ *}}//   #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  // PRT-AO-NEXT:   {{^ *}}//   for (i ={{.*}})
  // PRT-AO-NEXT:   {{^ *}}//     for (j ={{.*}})
  // PRT-AO-NEXT:   {{^ *}}//       setOrder(&order, i, j);
  // PRT-AO-NEXT:   {{^ *}}// }
  // PRT-AO-NEXT:   {{^ *}}// ^----------OMP----------^{{$}}
  //
  // PRT-OA-NEXT:   {{^ *}}// v----------OMP----------v{{$}}
  //  PRT-O-NEXT:   {{^ *}}{
  //  PRT-O-NEXT:   {{^ *}}  int i;
  //  PRT-O-NEXT:   {{^ *}}  int j;
  //  PRT-O-NEXT:   {{^ *}}  #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  //  PRT-O-NEXT:   {{^ *}}  for (i ={{.*}})
  //  PRT-O-NEXT:   {{^ *}}    for (j ={{.*}})
  //  PRT-O-NEXT:   {{^ *}}      setOrder(&order, i, j);
  //  PRT-O-NEXT:   {{^ *}}}
  // PRT-OA-NEXT:   {{^ *}}// ---------OMP<-ACC--------{{$}}
  // PRT-OA-NEXT:   {{^ *}}// #pragma acc loop auto tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  // PRT-OA-NEXT:   {{^ *}}// for (i ={{.*}})
  // PRT-OA-NEXT:   {{^ *}}//   for (j ={{.*}})
  // PRT-OA-NEXT:   {{^ *}}//     setOrder(&order, i, j);
  // PRT-OA-NEXT:   {{^ *}}// ^----------ACC----------^{{$}}
  //
  //         PRT: }
  //
  // EXE-NEXT:  order verified
  // EXE-NEXT:  i = 98
  // EXE-NEXT:  j = 99
  {
    int i = 98;
    int j = 99;
    int iPerGang[NUM_GANGS];
    int jPerGang[NUM_GANGS];
    initOrder(&order);
    #pragma acc parallel num_gangs(NUM_GANGS) copy(i,j)
    {
      #pragma acc loop auto tile(I_NELEMS,J_NELEMS)
      for (i = 0; i < I_NTILES * I_NELEMS; ++i)
        for (j = 0; j < J_NTILES * J_NELEMS; ++j)
          setOrder(&order, i, j);
      int omp_get_team_num();
      int gangIdx = omp_get_team_num();
      iPerGang[gangIdx] = i;
      jPerGang[gangIdx] = j;
    }
    if (!checkOrder2D(&order, /*gangPartitioned=*/false,
                      /*workerPartitioned=*/false))
      printf("  order verified\n");
    printf("  i = %d\n", i);
    printf("  j = %d\n", j);
    for (int gangIdx = 0; gangIdx < NUM_GANGS; ++gangIdx) {
      if (iPerGang[gangIdx] != i)
        printf("  error: iPerGang[%d] = %d, expected %d\n",
               gangIdx, iPerGang[gangIdx], i);
      if (jPerGang[gangIdx] != j)
        printf("  error: jPerGang[%d] = %d, expected %d\n",
               gangIdx, jPerGang[gangIdx], j);
    }
  }

  // DMP-LABEL:"impGangMappedLoopVar\n"
  // PRT-LABEL:"impGangMappedLoopVar\n"
  // EXE-LABEL:impGangMappedLoopVar
  printf("impGangMappedLoopVar\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
  //  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //  DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'i'
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'j'
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  //  DMP-NEXT:   impl: OMPDistributeDirective
  //  DMP-NEXT:     OMPCollapseClause
  //  DMP-NEXT:       ConstantExpr {{.*}} 'int'
  //  DMP-NEXT:         value: Int 2
  //  DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //  DMP-NEXT:     OMPPrivateClause
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'i'
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'j'
  //       DMP:     OMPTileDirective
  //  DMP-NEXT:       OMPSizesClause
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#J_NELEMS]]
  //       DMP:       ForStmt
  //       DMP:         ForStmt
  //
  //         PRT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}) copy(i,j){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: i,j) map(ompx_hold,tofrom: order,iPerGang,jPerGang){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: i,j) map(ompx_hold,tofrom: order,iPerGang,jPerGang){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}) copy(i,j){{$}}
  //    PRT-NEXT: {
  //  PRT-A-NEXT:   {{^ *}}#pragma acc loop tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  // PRT-AO-NEXT:   {{^ *}}// #pragma omp distribute collapse(2) private(i,j){{$}}
  // PRT-AO-NEXT:   {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  //  PRT-O-NEXT:   {{^ *}}#pragma omp distribute collapse(2) private(i,j){{$}}
  //  PRT-O-NEXT:   {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  // PRT-OA-NEXT:   {{^ *}}// #pragma acc loop tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  //    PRT-NEXT:   for (i ={{.*}})
  //    PRT-NEXT:     for (j ={{.*}})
  //    PRT-NEXT:       setOrder(&order, i, j);
  //         PRT: }
  //
  // EXE-NEXT:  order verified
  // EXE-NEXT:  i = 98
  // EXE-NEXT:  j = 99
  {
    int i = 98;
    int j = 99;
    int iPerGang[NUM_GANGS];
    int jPerGang[NUM_GANGS];
    initOrder(&order);
    #pragma acc parallel num_gangs(NUM_GANGS) copy(i,j)
    {
      #pragma acc loop tile(I_NELEMS,J_NELEMS)
      for (i = 0; i < I_NTILES * I_NELEMS; ++i)
        for (j = 0; j < J_NTILES * J_NELEMS; ++j)
          setOrder(&order, i, j);
      int omp_get_team_num();
      int gangIdx = omp_get_team_num();
      iPerGang[gangIdx] = i;
      jPerGang[gangIdx] = j;
    }
    if (!checkOrder2D(&order, /*gangPartitioned=*/true,
                      /*workerPartitioned=*/false))
      printf("  order verified\n");
    printf("  i = %d\n", i);
    printf("  j = %d\n", j);
    for (int gangIdx = 0; gangIdx < NUM_GANGS; ++gangIdx) {
      if (iPerGang[gangIdx] != i)
        printf("  error: iPerGang[%d] = %d, expected %d\n",
               gangIdx, iPerGang[gangIdx], i);
      if (jPerGang[gangIdx] != j)
        printf("  error: jPerGang[%d] = %d, expected %d\n",
               gangIdx, jPerGang[gangIdx], j);
    }
  }

  // DMP-LABEL:"expWorkerVectorMappedLoopVar\n"
  // PRT-LABEL:"expWorkerVectorMappedLoopVar\n"
  // EXE-LABEL:expWorkerVectorMappedLoopVar
  printf("expWorkerVectorMappedLoopVar\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCWorkerClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCVectorClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
  //  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //  DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'i'
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'j'
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  //  DMP-NEXT:   impl: OMPDistributeParallelForDirective
  //  DMP-NEXT:     OMPCollapseClause
  //  DMP-NEXT:       ConstantExpr {{.*}} 'int'
  //  DMP-NEXT:         value: Int 2
  //  DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //  DMP-NEXT:     OMPPrivateClause
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'i'
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'j'
  //       DMP:     OMPTileDirective
  //  DMP-NEXT:       OMPSizesClause
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#J_NELEMS]]
  //       DMP:       ForStmt
  //       DMP:         ForStmt
  //
  //         PRT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}) num_workers({{NUM_WORKERS|[0-9]+}}) vector_length({{VECTOR_LENGTH|[0-9]+}}) copy(i,j){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) thread_limit({{[0-9]+}}) map(ompx_hold,tofrom: i,j) map(ompx_hold,tofrom: order,iPerGang,jPerGang){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) thread_limit({{[0-9]+}}) map(ompx_hold,tofrom: i,j) map(ompx_hold,tofrom: order,iPerGang,jPerGang){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}) num_workers({{NUM_WORKERS|[0-9]+}}) vector_length({{VECTOR_LENGTH|[0-9]+}}) copy(i,j){{$}}
  //    PRT-NEXT: {
  //  PRT-A-NEXT:   {{^ *}}#pragma acc loop worker vector tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  // PRT-AO-NEXT:   {{^ *}}// #pragma omp distribute parallel for collapse(2) private(i,j){{$}}
  // PRT-AO-NEXT:   {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  //  PRT-O-NEXT:   {{^ *}}#pragma omp distribute parallel for collapse(2) private(i,j){{$}}
  //  PRT-O-NEXT:   {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  // PRT-OA-NEXT:   {{^ *}}// #pragma acc loop worker vector tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  //    PRT-NEXT:   for (i ={{.*}})
  //    PRT-NEXT:     for (j ={{.*}})
  //    PRT-NEXT:       setOrder(&order, i, j);
  //         PRT: }
  //
  // EXE-NEXT:  order verified
  // EXE-NEXT:  i = 98
  // EXE-NEXT:  j = 99
  {
    int i = 98;
    int j = 99;
    int iPerGang[NUM_GANGS];
    int jPerGang[NUM_GANGS];
    initOrder(&order);
    #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS) vector_length(VECTOR_LENGTH) copy(i,j)
    {
      #pragma acc loop worker vector tile(I_NELEMS,J_NELEMS)
      for (i = 0; i < I_NTILES * I_NELEMS; ++i)
        for (j = 0; j < J_NTILES * J_NELEMS; ++j)
          setOrder(&order, i, j);
      int omp_get_team_num();
      int gangIdx = omp_get_team_num();
      iPerGang[gangIdx] = i;
      jPerGang[gangIdx] = j;
    }
    if (!checkOrder2D(&order, /*gangPartitioned=*/true,
                      /*workerPartitioned=*/true))
      printf("  order verified\n");
    printf("  i = %d\n", i);
    printf("  j = %d\n", j);
    for (int gangIdx = 0; gangIdx < NUM_GANGS; ++gangIdx) {
      if (iPerGang[gangIdx] != i)
        printf("  error: iPerGang[%d] = %d, expected %d\n",
               gangIdx, iPerGang[gangIdx], i);
      if (jPerGang[gangIdx] != j)
        printf("  error: jPerGang[%d] = %d, expected %d\n",
               gangIdx, jPerGang[gangIdx], j);
    }
  }

  // DMP-LABEL:"expWorkerVectorCmbUnmappedLoopVar\n"
  // PRT-LABEL:"expWorkerVectorCmbUnmappedLoopVar\n"
  // EXE-LABEL:expWorkerVectorCmbUnmappedLoopVar
  printf("expWorkerVectorCmbUnmappedLoopVar\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCWorkerClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCVectorClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
  //  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //  DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'i'
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'j'
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  //  DMP-NEXT:   impl: OMPDistributeParallelForDirective
  //  DMP-NEXT:     OMPCollapseClause
  //  DMP-NEXT:       ConstantExpr {{.*}} 'int'
  //  DMP-NEXT:         value: Int 2
  //  DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //  DMP-NEXT:     OMPPrivateClause
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'i'
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'j'
  //       DMP:     OMPTileDirective
  //  DMP-NEXT:       OMPSizesClause
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#J_NELEMS]]
  //       DMP:       ForStmt
  //       DMP:         ForStmt
  //
  //         PRT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel loop num_gangs({{NUM_GANGS|[0-9]+}}) num_workers({{NUM_WORKERS|[0-9]+}}) vector_length({{VECTOR_LENGTH|[0-9]+}}) worker vector tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) thread_limit({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for collapse(2) private(i,j){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) thread_limit({{[0-9]+}}) map(ompx_hold,tofrom: order){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for collapse(2) private(i,j){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel loop num_gangs({{NUM_GANGS|[0-9]+}}) num_workers({{NUM_WORKERS|[0-9]+}}) vector_length({{VECTOR_LENGTH|[0-9]+}}) worker vector tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  //    PRT-NEXT: for (i ={{.*}})
  //    PRT-NEXT:   for (j ={{.*}})
  //    PRT-NEXT:     setOrder(&order, i, j);
  //
  // EXE-NEXT:  order verified
  // EXE-NEXT:  i = 98
  // EXE-NEXT:  j = 99
  {
    int i = 98;
    int j = 99;
    initOrder(&order);
    #pragma acc parallel loop num_gangs(NUM_GANGS) num_workers(NUM_WORKERS) vector_length(VECTOR_LENGTH) worker vector tile(I_NELEMS,J_NELEMS)
    for (i = 0; i < I_NTILES * I_NELEMS; ++i)
      for (j = 0; j < J_NTILES * J_NELEMS; ++j)
        setOrder(&order, i, j);
    if (!checkOrder2D(&order, /*gangPartitioned=*/true,
                      /*workerPartitioned=*/true))
      printf("  order verified\n");
    printf("  i = %d\n", i);
    printf("  j = %d\n", j);
  }

  // DMP-LABEL:"extraLoopVarUnaffected\n"
  // PRT-LABEL:"extraLoopVarUnaffected\n"
  // EXE-LABEL:extraLoopVarUnaffected
  printf("extraLoopVarUnaffected\n");

  // That i is declared in for loop init must not throw off the loop count and
  // thus affect k's privacy.

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
  //  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //  DMP-NEXT:   ACCPrivateClause {{.*}} <predetermined>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'j'
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'k'
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  //  DMP-NEXT:   impl: OMPDistributeDirective
  //  DMP-NEXT:     OMPCollapseClause
  //  DMP-NEXT:       ConstantExpr {{.*}} 'int'
  //  DMP-NEXT:         value: Int 2
  //  DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //  DMP-NEXT:     OMPPrivateClause
  //  DMP-NEXT:       DeclRefExpr {{.*}} 'j'
  //       DMP:     OMPTileDirective
  //  DMP-NEXT:       OMPSizesClause
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#I_NELEMS]]
  //  DMP-NEXT:         IntegerLiteral {{.*}} [[#J_NELEMS]]
  //       DMP:       ForStmt
  //       DMP:         ForStmt
  //
  //         PRT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}) copy(i,j,k){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: i,j,k) map(ompx_hold,tofrom: order){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: i,j,k) map(ompx_hold,tofrom: order){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}) copy(i,j,k){{$}}
  //    PRT-NEXT: {
  //  PRT-A-NEXT:   {{^ *}}#pragma acc loop tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  // PRT-AO-NEXT:   {{^ *}}// #pragma omp distribute collapse(2) private(j){{$}}
  // PRT-AO-NEXT:   {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  //  PRT-O-NEXT:   {{^ *}}#pragma omp distribute collapse(2) private(j){{$}}
  //  PRT-O-NEXT:   {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
  // PRT-OA-NEXT:   {{^ *}}// #pragma acc loop tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
  //    PRT-NEXT:   for (int i ={{.*}})
  //    PRT-NEXT:     for (j ={{.*}})
  //    PRT-NEXT:       for (k ={{.*}})
  //    PRT-NEXT:         setOrder(&order, i, j);
  //         PRT: }
  //
  // EXE-NEXT:  order verified
  // EXE-NEXT:  i = 99
  // EXE-NEXT:  j = 98
  // EXE-NEXT:  k = 1
  {
    int i = 99;
    int j = 98;
    int k = 97;
    initOrder(&order);
    #pragma acc parallel num_gangs(1) copy(i,j,k)
    {
      #pragma acc loop tile(I_NELEMS,J_NELEMS)
      for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
        for (j = 0; j < J_NTILES * J_NELEMS; ++j)
          for (k = 0; k < 1; ++k)
            setOrder(&order, i, j);
    }
    if (!checkOrder2DFull(&order, /*gangPartitioned=*/true,
                          /*workerPartitioned=*/false, /*numGangs=*/1, I_NTILES,
                          J_NTILES, I_NELEMS, J_NELEMS))
      printf("  order verified\n");
    printf("  i = %d\n", i);
    printf("  j = %d\n", j);
    printf("  k = %d\n", k);
  }

  //............................................................................
  // Tile size '*' or non-constant expression.
  //
  // For now, these become a tile size of 1.
  //............................................................................

  // DMP-LABEL:"expSeqTileSizeBecomes1\n"
  // PRT-LABEL:"expSeqTileSizeBecomes1\n"
  // EXE-LABEL:expSeqTileSizeBecomes1
  printf("expSeqTileSizeBecomes1\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCSeqClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     ACCStarExpr
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'nonConstTileSize'
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   impl: OMPTileDirective
  //  DMP-NEXT:     OMPSizesClause
  //  DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  //  DMP-NEXT:       IntegerLiteral {{.*}} 'int' 1
  //       DMP:     ForStmt
  //       DMP:       ForStmt
  //
  //         PRT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order) firstprivate(nonConstTileSize){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) map(ompx_hold,tofrom: order) firstprivate(nonConstTileSize){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}){{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop seq tile(*,nonConstTileSize){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp tile sizes(1, 1){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp tile sizes(1, 1){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop seq tile(*,nonConstTileSize){{$}}
  //
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     setOrderFull(&order, i, j, {{.*}});
  //
  // EXE-NEXT:  verified
  {
    int nonConstTileSize = 8;
    initOrder(&order);
    #pragma acc parallel num_gangs(NUM_GANGS)
    #pragma acc loop seq tile(*,nonConstTileSize)
    for (int i = 0; i < I_NTILES; ++i)
      for (int j = 0; j < J_NTILES; ++j)
        setOrderFull(&order, i, j, /*iNelems=*/1, /*jNelems=*/1);
    if (!checkOrder2DFull(&order, /*gangPartitioned=*/false,
                          /*workerPartitioned=*/false, NUM_GANGS,
                          /*iNtiles=*/I_NTILES, /*jNtiles=*/J_NTILES,
                          /*iNelems=*/1, /*jNelems=*/1))
      printf("  verified\n");
  }

  // DMP-LABEL:"expWorkerVectorTileSizeBecomes1\n"
  // PRT-LABEL:"expWorkerVectorTileSizeBecomes1\n"
  // EXE-LABEL:expWorkerVectorTileSizeBecomes1
  printf("expWorkerVectorTileSizeBecomes1\n");

  //       DMP: ACCLoopDirective
  //  DMP-NEXT:   ACCWorkerClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCVectorClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   ACCTileClause
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'nonConstTileSize'
  //  DMP-NEXT:     ACCStarExpr
  //  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
  //  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
  //  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
  //  DMP-NEXT:   ACCGangClause {{.*}} <implicit>
  //  DMP-NEXT:   impl: OMPDistributeParallelForDirective
  //  DMP-NEXT:     OMPCollapseClause
  //  DMP-NEXT:       ConstantExpr {{.*}} 'int'
  //  DMP-NEXT:         value: Int 2
  //  DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  //       DMP:     OMPTileDirective
  //  DMP-NEXT:       OMPSizesClause
  //  DMP-NEXT:         IntegerLiteral {{.*}} 1
  //  DMP-NEXT:         IntegerLiteral {{.*}} 1
  //       DMP:       ForStmt
  //       DMP:         ForStmt
  //
  //         PRT: initOrder(&order);
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}) num_workers({{NUM_WORKERS|[0-9]+}}) vector_length({{VECTOR_LENGTH|[0-9]+}}){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams({{[0-9]+}}) thread_limit({{[0-9]+}}) map(ompx_hold,tofrom: order) firstprivate(nonConstTileSize){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp target teams num_teams({{[0-9]+}}) thread_limit({{[0-9]+}}) map(ompx_hold,tofrom: order) firstprivate(nonConstTileSize){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs({{NUM_GANGS|[0-9]+}}) num_workers({{NUM_WORKERS|[0-9]+}}) vector_length({{VECTOR_LENGTH|[0-9]+}}){{$}}
  //
  //  PRT-A-NEXT: {{^ *}}#pragma acc loop worker vector tile(nonConstTileSize,*){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp distribute parallel for collapse(2){{$}}
  // PRT-AO-NEXT: {{^ *}}// #pragma omp tile sizes(1, 1){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp distribute parallel for collapse(2){{$}}
  //  PRT-O-NEXT: {{^ *}}#pragma omp tile sizes(1, 1){{$}}
  // PRT-OA-NEXT: {{^ *}}// #pragma acc loop worker vector tile(nonConstTileSize,*){{$}}
  //
  //    PRT-NEXT: for (int i ={{.*}})
  //    PRT-NEXT:   for (int j ={{.*}})
  //    PRT-NEXT:     setOrderFull(&order, i, j, {{.*}});
  //
  // EXE-NEXT:  verified
  {
    int nonConstTileSize = 2;
    initOrder(&order);
    #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS) vector_length(VECTOR_LENGTH)
    #pragma acc loop worker vector tile(nonConstTileSize,*)
    for (int i = 0; i < I_NTILES; ++i)
      for (int j = 0; j < J_NTILES; ++j)
        setOrderFull(&order, i, j, /*iNelems=*/1, /*jNelems=*/1);
    if (!checkOrder2DFull(&order, /*gangPartitioned=*/true,
                          /*workerPartitioned=*/true, NUM_GANGS,
                          /*iNtiles=*/I_NTILES, /*jNtiles=*/J_NTILES,
                          /*iNelems=*/1, /*jNelems=*/1))
      printf("  verified\n");
  }

  //............................................................................
  // Tile clauses within accelerator routines.
  //............................................................................

  // EXE-LABEL:expSeqWithinGangFn
  //  EXE-NEXT:  verified
  printf("expSeqWithinGangFn\n");
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS) vector_length(VECTOR_LENGTH)
  expSeqWithinGangFn(&order);
  if (!checkOrder2D(&order, /*gangPartitioned=*/false,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  // EXE-LABEL:expAutoWithinGangFn
  //  EXE-NEXT:  verified
  printf("expAutoWithinGangFn\n");
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS) vector_length(VECTOR_LENGTH)
  expAutoWithinGangFn(&order);
  if (!checkOrder2D(&order, /*gangPartitioned=*/false,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  // EXE-LABEL:expGangWithinGangFn
  //  EXE-NEXT:  verified
  printf("expGangWithinGangFn\n");
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS) vector_length(VECTOR_LENGTH)
  expGangWithinGangFn(&order);
  if (!checkOrder2D(&order, /*gangPartitioned=*/true,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  // EXE-LABEL:impGangWithinGangFn
  //  EXE-NEXT:  verified
  printf("impGangWithinGangFn\n");
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS) vector_length(VECTOR_LENGTH)
  impGangWithinGangFn(&order);
  if (!checkOrder2D(&order, /*gangPartitioned=*/true,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  // EXE-LABEL:expWorkerWithinGangFn
  //  EXE-NEXT:  verified
  printf("expWorkerWithinGangFn\n");
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS) vector_length(VECTOR_LENGTH)
  expWorkerWithinGangFn(&order);
  if (!checkOrder2D(&order, /*gangPartitioned=*/true,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  // EXE-LABEL:expVectorWithinGangFn
  //  EXE-NEXT:  verified
  printf("expVectorWithinGangFn\n");
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS) vector_length(VECTOR_LENGTH)
  expVectorWithinGangFn(&order);
  if (!checkOrder2D(&order, /*gangPartitioned=*/true,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  // EXE-LABEL:expWorkerVectorWithinGangFn
  //  EXE-NEXT:  verified
  printf("expWorkerVectorWithinGangFn\n");
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS) vector_length(VECTOR_LENGTH)
  expWorkerVectorWithinGangFn(&order);
  if (!checkOrder2D(&order, /*gangPartitioned=*/true,
                    /*workerPartitioned=*/true))
    printf("  verified\n");

  // EXE-LABEL:expSeqWithinWorkerFn
  //  EXE-NEXT:  verified
  printf("expSeqWithinWorkerFn\n");
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS) vector_length(VECTOR_LENGTH)
  expSeqWithinWorkerFn(&order);
  if (!checkOrder2D(&order, /*gangPartitioned=*/false,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  // EXE-LABEL:expAutoWithinWorkerFn
  //  EXE-NEXT:  verified
  printf("expAutoWithinWorkerFn\n");
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS) vector_length(VECTOR_LENGTH)
  expAutoWithinWorkerFn(&order);
  if (!checkOrder2D(&order, /*gangPartitioned=*/false,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  // EXE-LABEL:expWorkerWithinWorkerFn
  //  EXE-NEXT:  verified
  printf("expWorkerWithinWorkerFn\n");
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS) vector_length(VECTOR_LENGTH)
  expWorkerWithinWorkerFn(&order);
  if (!checkOrder2D(&order, /*gangPartitioned=*/false,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  // EXE-LABEL:expVectorWithinWorkerFn
  //  EXE-NEXT:  verified
  printf("expVectorWithinWorkerFn\n");
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS) vector_length(VECTOR_LENGTH)
  expVectorWithinWorkerFn(&order);
  if (!checkOrder2D(&order, /*gangPartitioned=*/false,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  // EXE-LABEL:expWorkerVectorWithinWorkerFn
  //  EXE-NEXT:  verified
  printf("expWorkerVectorWithinWorkerFn\n");
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS) vector_length(VECTOR_LENGTH)
  expWorkerVectorWithinWorkerFn(&order);
  if (!checkOrder2D(&order, /*gangPartitioned=*/false,
                    /*workerPartitioned=*/true))
    printf("  verified\n");

  // EXE-LABEL:expSeqWithinVectorFn
  //  EXE-NEXT:  verified
  printf("expSeqWithinVectorFn\n");
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS) vector_length(VECTOR_LENGTH)
  expSeqWithinVectorFn(&order);
  if (!checkOrder2D(&order, /*gangPartitioned=*/false,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  // EXE-LABEL:expAutoWithinVectorFn
  //  EXE-NEXT:  verified
  printf("expAutoWithinVectorFn\n");
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS) vector_length(VECTOR_LENGTH)
  expAutoWithinVectorFn(&order);
  if (!checkOrder2D(&order, /*gangPartitioned=*/false,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  // EXE-LABEL:expVectorWithinVectorFn
  //  EXE-NEXT:  verified
  printf("expVectorWithinVectorFn\n");
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS) vector_length(VECTOR_LENGTH)
  expVectorWithinVectorFn(&order);
  if (!checkOrder2D(&order, /*gangPartitioned=*/false,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  // EXE-LABEL:expSeqWithinSeqFn
  //  EXE-NEXT:  verified
  printf("expSeqWithinSeqFn\n");
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS) vector_length(VECTOR_LENGTH)
  expSeqWithinSeqFn(&order);
  if (!checkOrder2D(&order, /*gangPartitioned=*/false,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  // EXE-LABEL:expAutoWithinSeqFn
  //  EXE-NEXT:  verified
  printf("expAutoWithinSeqFn\n");
  initOrder(&order);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS) vector_length(VECTOR_LENGTH)
  expAutoWithinSeqFn(&order);
  if (!checkOrder2D(&order, /*gangPartitioned=*/false,
                    /*workerPartitioned=*/false))
    printf("  verified\n");

  return 0;
}

//------------------------------------------------------------------------------
// Check tile clauses within a gang function.
//------------------------------------------------------------------------------

//..............................................................................
// Explicit seq.
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} expSeqWithinGangFn
//       DMP: ACCLoopDirective
//  DMP-NEXT:   ACCSeqClause
//  DMP-NEXT:   ACCTileClause
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
//  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
//  DMP-NEXT:   impl: OMPTileDirective
//  DMP-NEXT:     OMPSizesClause
//  DMP-NEXT:       IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:       IntegerLiteral {{.*}} [[#J_NELEMS]]
//       DMP:     ForStmt
//       DMP:       ForStmt
//
//   PRT-LABEL: void expSeqWithinGangFn({{.*}}) {
//  PRT-A-NEXT:   {{^ *}}#pragma acc loop seq tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc loop seq tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
//    PRT-NEXT:   for (int i ={{.*}})
//    PRT-NEXT:     for (int j ={{.*}})
//    PRT-NEXT:       setOrder(order, i, j);
//    PRT-NEXT: }
static void expSeqWithinGangFn(Order *order) {
  #pragma acc loop seq tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(order, i, j);
}

//..............................................................................
// Explicit auto.
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} expAutoWithinGangFn
//       DMP: ACCLoopDirective
//  DMP-NEXT:   ACCAutoClause
//  DMP-NEXT:   ACCTileClause
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
//  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
//  DMP-NEXT:   impl: OMPTileDirective
//  DMP-NEXT:     OMPSizesClause
//  DMP-NEXT:       IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:       IntegerLiteral {{.*}} [[#J_NELEMS]]
//       DMP:     ForStmt
//       DMP:       ForStmt
//
//   PRT-LABEL: void expAutoWithinGangFn({{.*}}) {
//  PRT-A-NEXT:   {{^ *}}#pragma acc loop auto tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc loop auto tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
//    PRT-NEXT:   for (int i ={{.*}})
//    PRT-NEXT:     for (int j ={{.*}})
//    PRT-NEXT:       setOrder(order, i, j);
//    PRT-NEXT: }
static void expAutoWithinGangFn(Order *order) {
  #pragma acc loop auto tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(order, i, j);
}

//............................................................................
// Explicit gang partitioning.
//............................................................................

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} expGangWithinGangFn
//       DMP: ACCLoopDirective
//  DMP-NEXT:   ACCGangClause
//   DMP-NOT:     <implicit>
//  DMP-NEXT:   ACCTileClause
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
//  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
//  DMP-NEXT:   impl: OMPDistributeDirective
//  DMP-NEXT:     OMPCollapseClause
//  DMP-NEXT:       ConstantExpr {{.*}} 'int'
//  DMP-NEXT:         value: Int 2
//  DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
//       DMP:     OMPTileDirective
//  DMP-NEXT:       OMPSizesClause
//  DMP-NEXT:         IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:         IntegerLiteral {{.*}} [[#J_NELEMS]]
//       DMP:       ForStmt
//       DMP:         ForStmt
//
//   PRT-LABEL: void expGangWithinGangFn({{.*}}) {
//  PRT-A-NEXT:   {{^ *}}#pragma acc loop gang tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp distribute collapse(2){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp distribute collapse(2){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc loop gang tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
//    PRT-NEXT:   for (int i ={{.*}})
//    PRT-NEXT:     for (int j ={{.*}})
//    PRT-NEXT:       setOrder(order, i, j);
//    PRT-NEXT: }
static void expGangWithinGangFn(Order *order) {
  #pragma acc loop gang tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(order, i, j);
}

//..............................................................................
// Implicit gang partitioning.
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} impGangWithinGangFn
//       DMP: ACCLoopDirective
//  DMP-NEXT:   ACCTileClause
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
//  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
//  DMP-NEXT:   ACCGangClause {{.*}} <implicit>
//  DMP-NEXT:   impl: OMPDistributeDirective
//  DMP-NEXT:     OMPCollapseClause
//  DMP-NEXT:       ConstantExpr {{.*}} 'int'
//  DMP-NEXT:         value: Int 2
//  DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
//       DMP:     OMPTileDirective
//  DMP-NEXT:       OMPSizesClause
//  DMP-NEXT:         IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:         IntegerLiteral {{.*}} [[#J_NELEMS]]
//       DMP:       ForStmt
//       DMP:         ForStmt
//
//   PRT-LABEL: void impGangWithinGangFn({{.*}}) {
//  PRT-A-NEXT:   {{^ *}}#pragma acc loop tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp distribute collapse(2){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp distribute collapse(2){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc loop tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
//    PRT-NEXT:   for (int i ={{.*}})
//    PRT-NEXT:     for (int j ={{.*}})
//    PRT-NEXT:       setOrder(order, i, j);
//    PRT-NEXT: }
static void impGangWithinGangFn(Order *order) {
  #pragma acc loop tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(order, i, j);
}

//............................................................................
// Worker partitioning.
//............................................................................

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} expWorkerWithinGangFn
//       DMP: ACCLoopDirective
//  DMP-NEXT:   ACCWorkerClause
//   DMP-NOT:     <implicit>
//  DMP-NEXT:   ACCTileClause
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
//  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
//  DMP-NEXT:   ACCGangClause {{.*}} <implicit>
//  DMP-NEXT:   impl: OMPDistributeDirective
//  DMP-NEXT:     OMPCollapseClause
//  DMP-NEXT:       ConstantExpr {{.*}} 'int'
//  DMP-NEXT:         value: Int 2
//  DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
//       DMP:     OMPTileDirective
//  DMP-NEXT:       OMPSizesClause
//  DMP-NEXT:         IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:         IntegerLiteral {{.*}} [[#J_NELEMS]]
//       DMP:       ForStmt
//       DMP:         ForStmt
//
//   PRT-LABEL: void expWorkerWithinGangFn({{.*}}) {
//  PRT-A-NEXT:   {{^ *}}#pragma acc loop worker tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp distribute collapse(2){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp distribute collapse(2){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc loop worker tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
//    PRT-NEXT:   for (int i ={{.*}})
//    PRT-NEXT:     for (int j ={{.*}})
//    PRT-NEXT:       setOrder(order, i, j);
//    PRT-NEXT: }
static void expWorkerWithinGangFn(Order *order) {
  #pragma acc loop worker tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(order, i, j);
}

//............................................................................
// Vector partitioning.
//............................................................................

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} expVectorWithinGangFn
//       DMP: ACCLoopDirective
//  DMP-NEXT:   ACCVectorClause
//   DMP-NOT:     <implicit>
//  DMP-NEXT:   ACCTileClause
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
//  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
//  DMP-NEXT:   ACCGangClause {{.*}} <implicit>
//  DMP-NEXT:   impl: OMPDistributeDirective
//  DMP-NEXT:     OMPCollapseClause
//  DMP-NEXT:       ConstantExpr {{.*}} 'int'
//  DMP-NEXT:         value: Int 2
//  DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
//       DMP:     OMPTileDirective
//  DMP-NEXT:       OMPSizesClause
//  DMP-NEXT:         IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:         IntegerLiteral {{.*}} [[#J_NELEMS]]
//       DMP:       ForStmt
//       DMP:         ForStmt
//
//   PRT-LABEL: void expVectorWithinGangFn({{.*}}) {
//  PRT-A-NEXT:   {{^ *}}#pragma acc loop vector tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp distribute collapse(2){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp distribute collapse(2){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc loop vector tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
//    PRT-NEXT:   for (int i ={{.*}})
//    PRT-NEXT:     for (int j ={{.*}})
//    PRT-NEXT:       setOrder(order, i, j);
//    PRT-NEXT: }
static void expVectorWithinGangFn(Order *order) {
  #pragma acc loop vector tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(order, i, j);
}

//............................................................................
// Worker and vector partitioning.
//............................................................................

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} expWorkerVectorWithinGangFn
//       DMP: ACCLoopDirective
//  DMP-NEXT:   ACCWorkerClause
//   DMP-NOT:     <implicit>
//  DMP-NEXT:   ACCVectorClause
//   DMP-NOT:     <implicit>
//  DMP-NEXT:   ACCTileClause
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
//  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
//  DMP-NEXT:   ACCGangClause {{.*}} <implicit>
//  DMP-NEXT:   impl: OMPDistributeParallelForDirective
//  DMP-NEXT:     OMPCollapseClause
//  DMP-NEXT:       ConstantExpr {{.*}} 'int'
//  DMP-NEXT:         value: Int 2
//  DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
//       DMP:     OMPTileDirective
//  DMP-NEXT:       OMPSizesClause
//  DMP-NEXT:         IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:         IntegerLiteral {{.*}} [[#J_NELEMS]]
//       DMP:       ForStmt
//       DMP:         ForStmt
//
//   PRT-LABEL: void expWorkerVectorWithinGangFn({{.*}}) {
//  PRT-A-NEXT:   {{^ *}}#pragma acc loop worker vector tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp distribute parallel for collapse(2){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp distribute parallel for collapse(2){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc loop worker vector tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
//    PRT-NEXT:   for (int i ={{.*}})
//    PRT-NEXT:     for (int j ={{.*}})
//    PRT-NEXT:       setOrder(order, i, j);
//    PRT-NEXT: }
static void expWorkerVectorWithinGangFn(Order *order) {
  #pragma acc loop worker vector tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(order, i, j);
}

//------------------------------------------------------------------------------
// Check tile clauses within a worker function.
//------------------------------------------------------------------------------

//..............................................................................
// Explicit seq.
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} expSeqWithinWorkerFn
//       DMP: ACCLoopDirective
//  DMP-NEXT:   ACCSeqClause
//  DMP-NEXT:   ACCTileClause
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
//  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
//  DMP-NEXT:   impl: OMPTileDirective
//  DMP-NEXT:     OMPSizesClause
//  DMP-NEXT:       IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:       IntegerLiteral {{.*}} [[#J_NELEMS]]
//       DMP:     ForStmt
//       DMP:       ForStmt
//
//   PRT-LABEL: void expSeqWithinWorkerFn({{.*}}) {
//  PRT-A-NEXT:   {{^ *}}#pragma acc loop seq tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc loop seq tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
//    PRT-NEXT:   for (int i ={{.*}})
//    PRT-NEXT:     for (int j ={{.*}})
//    PRT-NEXT:       setOrder(order, i, j);
//    PRT-NEXT: }
static void expSeqWithinWorkerFn(Order *order) {
  #pragma acc loop seq tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(order, i, j);
}

//..............................................................................
// Explicit auto.
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} expAutoWithinWorkerFn
//       DMP: ACCLoopDirective
//  DMP-NEXT:   ACCAutoClause
//  DMP-NEXT:   ACCTileClause
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
//  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
//  DMP-NEXT:   impl: OMPTileDirective
//  DMP-NEXT:     OMPSizesClause
//  DMP-NEXT:       IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:       IntegerLiteral {{.*}} [[#J_NELEMS]]
//       DMP:     ForStmt
//       DMP:       ForStmt
//
//   PRT-LABEL: void expAutoWithinWorkerFn({{.*}}) {
//  PRT-A-NEXT:   {{^ *}}#pragma acc loop auto tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc loop auto tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
//    PRT-NEXT:   for (int i ={{.*}})
//    PRT-NEXT:     for (int j ={{.*}})
//    PRT-NEXT:       setOrder(order, i, j);
//    PRT-NEXT: }
static void expAutoWithinWorkerFn(Order *order) {
  #pragma acc loop auto tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(order, i, j);
}

//..............................................................................
// Worker partitioning.
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} expWorkerWithinWorkerFn
//       DMP: ACCLoopDirective
//  DMP-NEXT:   ACCWorkerClause
//   DMP-NOT:     <implicit>
//  DMP-NEXT:   ACCTileClause
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
//  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
//  DMP-NEXT:   impl: OMPTileDirective
//  DMP-NEXT:     OMPSizesClause
//  DMP-NEXT:       IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:       IntegerLiteral {{.*}} [[#J_NELEMS]]
//       DMP:     ForStmt
//       DMP:       ForStmt
//
//   PRT-LABEL: void expWorkerWithinWorkerFn({{.*}}) {
//  PRT-A-NEXT:   {{^ *}}#pragma acc loop worker tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc loop worker tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
//    PRT-NEXT:   for (int i ={{.*}})
//    PRT-NEXT:     for (int j ={{.*}})
//    PRT-NEXT:       setOrder(order, i, j);
//    PRT-NEXT: }
static void expWorkerWithinWorkerFn(Order *order) {
  #pragma acc loop worker tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(order, i, j);
}

//..............................................................................
// Vector partitioning.
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} expVectorWithinWorkerFn
//       DMP: ACCLoopDirective
//  DMP-NEXT:   ACCVectorClause
//   DMP-NOT:     <implicit>
//  DMP-NEXT:   ACCTileClause
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
//  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
//  DMP-NEXT:   impl: OMPTileDirective
//  DMP-NEXT:     OMPSizesClause
//  DMP-NEXT:       IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:       IntegerLiteral {{.*}} [[#J_NELEMS]]
//       DMP:     ForStmt
//       DMP:       ForStmt
//
//   PRT-LABEL: void expVectorWithinWorkerFn({{.*}}) {
//  PRT-A-NEXT:   {{^ *}}#pragma acc loop vector tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc loop vector tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
//    PRT-NEXT:   for (int i ={{.*}})
//    PRT-NEXT:     for (int j ={{.*}})
//    PRT-NEXT:       setOrder(order, i, j);
//    PRT-NEXT: }
static void expVectorWithinWorkerFn(Order *order) {
  #pragma acc loop vector tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(order, i, j);
}

//..............................................................................
// Worker and vector partitioning.
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} expWorkerVectorWithinWorkerFn
//       DMP: ACCLoopDirective
//  DMP-NEXT:   ACCWorkerClause
//   DMP-NOT:     <implicit>
//  DMP-NEXT:   ACCVectorClause
//   DMP-NOT:     <implicit>
//  DMP-NEXT:   ACCTileClause
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
//  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
//  DMP-NEXT:   impl: OMPParallelForDirective
//  DMP-NEXT:     OMPCollapseClause
//  DMP-NEXT:       ConstantExpr {{.*}} 'int'
//  DMP-NEXT:         value: Int 2
//  DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2
//       DMP:     OMPTileDirective
//  DMP-NEXT:       OMPSizesClause
//  DMP-NEXT:         IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:         IntegerLiteral {{.*}} [[#J_NELEMS]]
//       DMP:       ForStmt
//       DMP:         ForStmt
//
//   PRT-LABEL: void expWorkerVectorWithinWorkerFn({{.*}}) {
//  PRT-A-NEXT:   {{^ *}}#pragma acc loop worker vector tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp parallel for collapse(2){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp parallel for collapse(2){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc loop worker vector tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
//    PRT-NEXT:   for (int i ={{.*}})
//    PRT-NEXT:     for (int j ={{.*}})
//    PRT-NEXT:       setOrder(order, i, j);
//    PRT-NEXT: }
static void expWorkerVectorWithinWorkerFn(Order *order) {
  #pragma acc loop worker vector tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(order, i, j);
}

//------------------------------------------------------------------------------
// Check tile clauses within a vector function.
//------------------------------------------------------------------------------

//..............................................................................
// Explicit seq.
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} expSeqWithinVectorFn
//       DMP: ACCLoopDirective
//  DMP-NEXT:   ACCSeqClause
//  DMP-NEXT:   ACCTileClause
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
//  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
//  DMP-NEXT:   impl: OMPTileDirective
//  DMP-NEXT:     OMPSizesClause
//  DMP-NEXT:       IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:       IntegerLiteral {{.*}} [[#J_NELEMS]]
//       DMP:     ForStmt
//       DMP:       ForStmt
//
//   PRT-LABEL: void expSeqWithinVectorFn({{.*}}) {
//  PRT-A-NEXT:   {{^ *}}#pragma acc loop seq tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc loop seq tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
//    PRT-NEXT:   for (int i ={{.*}})
//    PRT-NEXT:     for (int j ={{.*}})
//    PRT-NEXT:       setOrder(order, i, j);
//    PRT-NEXT: }
static void expSeqWithinVectorFn(Order *order) {
  #pragma acc loop seq tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(order, i, j);
}

//..............................................................................
// Explicit auto.
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} expAutoWithinVectorFn
//       DMP: ACCLoopDirective
//  DMP-NEXT:   ACCAutoClause
//  DMP-NEXT:   ACCTileClause
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
//  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
//  DMP-NEXT:   impl: OMPTileDirective
//  DMP-NEXT:     OMPSizesClause
//  DMP-NEXT:       IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:       IntegerLiteral {{.*}} [[#J_NELEMS]]
//       DMP:     ForStmt
//       DMP:       ForStmt
//
//   PRT-LABEL: void expAutoWithinVectorFn({{.*}}) {
//  PRT-A-NEXT:   {{^ *}}#pragma acc loop auto tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc loop auto tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
//    PRT-NEXT:   for (int i ={{.*}})
//    PRT-NEXT:     for (int j ={{.*}})
//    PRT-NEXT:       setOrder(order, i, j);
//    PRT-NEXT: }
static void expAutoWithinVectorFn(Order *order) {
  #pragma acc loop auto tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(order, i, j);
}

//..............................................................................
// Vector partitioning.
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} expVectorWithinVectorFn
//       DMP: ACCLoopDirective
//  DMP-NEXT:   ACCVectorClause
//   DMP-NOT:     <implicit>
//  DMP-NEXT:   ACCTileClause
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
//  DMP-NEXT:   ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
//  DMP-NEXT:   impl: OMPTileDirective
//  DMP-NEXT:     OMPSizesClause
//  DMP-NEXT:       IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:       IntegerLiteral {{.*}} [[#J_NELEMS]]
//       DMP:     ForStmt
//       DMP:       ForStmt
//
//   PRT-LABEL: void expVectorWithinVectorFn({{.*}}) {
//  PRT-A-NEXT:   {{^ *}}#pragma acc loop vector tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc loop vector tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
//    PRT-NEXT:   for (int i ={{.*}})
//    PRT-NEXT:     for (int j ={{.*}})
//    PRT-NEXT:       setOrder(order, i, j);
//    PRT-NEXT: }
static void expVectorWithinVectorFn(Order *order) {
  #pragma acc loop vector tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(order, i, j);
}

//------------------------------------------------------------------------------
// Check tile clauses within a seq function.
//------------------------------------------------------------------------------

//..............................................................................
// Explicit seq.
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} expSeqWithinSeqFn
//       DMP: ACCLoopDirective
//  DMP-NEXT:   ACCSeqClause
//  DMP-NEXT:   ACCTileClause
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
//  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
//  DMP-NEXT:   impl: OMPTileDirective
//  DMP-NEXT:     OMPSizesClause
//  DMP-NEXT:       IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:       IntegerLiteral {{.*}} [[#J_NELEMS]]
//       DMP:     ForStmt
//       DMP:       ForStmt
//
//   PRT-LABEL: void expSeqWithinSeqFn({{.*}}) {
//  PRT-A-NEXT:   {{^ *}}#pragma acc loop seq tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc loop seq tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
//    PRT-NEXT:   for (int i ={{.*}})
//    PRT-NEXT:     for (int j ={{.*}})
//    PRT-NEXT:       setOrder(order, i, j);
//    PRT-NEXT: }
static void expSeqWithinSeqFn(Order *order) {
  #pragma acc loop seq tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(order, i, j);
}

//..............................................................................
// Explicit auto.
//..............................................................................

// DMP-LABEL: FunctionDecl {{.*}} prev {{.*}} expAutoWithinSeqFn
//       DMP: ACCLoopDirective
//  DMP-NEXT:   ACCAutoClause
//  DMP-NEXT:   ACCTileClause
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:     IntegerLiteral {{.*}} [[#J_NELEMS]]
//  DMP-NEXT:   ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:     DeclRefExpr {{.*}} 'order'
//  DMP-NEXT:   impl: OMPTileDirective
//  DMP-NEXT:     OMPSizesClause
//  DMP-NEXT:       IntegerLiteral {{.*}} [[#I_NELEMS]]
//  DMP-NEXT:       IntegerLiteral {{.*}} [[#J_NELEMS]]
//       DMP:     ForStmt
//       DMP:       ForStmt
//
//   PRT-LABEL: void expAutoWithinSeqFn({{.*}}) {
//  PRT-A-NEXT:   {{^ *}}#pragma acc loop auto tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp tile sizes([[#I_NELEMS]], [[#J_NELEMS]]){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc loop auto tile({{I_NELEMS,J_NELEMS|[0-9]+,[0-9]+}}){{$}}
//    PRT-NEXT:   for (int i ={{.*}})
//    PRT-NEXT:     for (int j ={{.*}})
//    PRT-NEXT:       setOrder(order, i, j);
//    PRT-NEXT: }
static void expAutoWithinSeqFn(Order *order) {
  #pragma acc loop auto tile(I_NELEMS,J_NELEMS)
  for (int i = 0; i < I_NTILES * I_NELEMS; ++i)
    for (int j = 0; j < J_NTILES * J_NELEMS; ++j)
      setOrder(order, i, j);
}
