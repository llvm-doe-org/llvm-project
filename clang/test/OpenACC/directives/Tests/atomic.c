// Check "acc atomic".
//
// For the sake of execution tests, we avoid combining multiple atomic
// directives within a single parallel directive.  Otherwise, if one atomic
// directive works correctly but a latter atomic directive lacks atomicity, the
// earlier atomic directive typically (in our experiments) would cause the
// threads not to be in lockstep when reaching the later atomic directive, and
// so the lack of atomicity wouldn't result in concurrent reads/writes and so
// wouldn't produce the desired test failure.  Putting them in separate parallel
// directives helps resynchronize threads.

// REDEFINE: %{all:clang:args} = -lm
// RUN: %{acc-check-dmp}
// RUN: %{acc-check-prt}
// RUN: %{acc-check-exe}

// END.

/* expected-no-diagnostics */

#include <math.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// On the offload targets we've tried (x86_64, ppc64le, nvptx64, and amdgcn),
// uint64_t is not wide enough to require atomic reads and writes, so our
// execution tests below don't actually verify that the read and write clauses
// have any effect at run time.  The tests just verify that the read and write
// clauses don't somehow break the atomic behavior we expect.
//
// __uint128_t does require atomic reads and writes, but atomics aren't
// supported for it currently, so we cannot use it.
//
// In contrast, update, capture, and compare clauses each pair a read and a
// write, so any data type is sufficient to verify their effect.
typedef uint64_t WideInt;
#define OLD_VAL 0
#define NEW_VAL ((WideInt)0 - 1)

// For execution tests, we run many repetitions to increase the likelihood of
// seeing a failure if atomics aren't working.
#define NUM_REPS 100

// Error permitted in double precision computations.
#define ERROR 1E-5

// Increasing the number of gangs, workers, and vector lanes increases the
// likelihood of a execution test failure if atomics aren't working.  However,
// offload devices have limits on these.  So far, the following work in our
// testing.

#define WS_VS_NUM_GANGS 8

#define WP_VS_NUM_GANGS 4
#define WP_VS_NUM_WORKERS 2

#define WS_VP_NUM_GANGS 8
#define WS_VP_VECTOR_LENGTH 256

#define WP_VP_NUM_GANGS 4
#define WP_VP_NUM_WORKERS 2
#define WP_VP_VECTOR_LENGTH 256

//==============================================================================
// Check every clause within every parallel execution mode: gang-redundant, etc.
// Vary the associated statement form (for update, capture, and compare)
// arbitrarily for now, and we'll be sure to cover all forms later.
//==============================================================================

//------------------------------------------------------------------------------
// Orphaned loops for GR/WP/VS mode.
//------------------------------------------------------------------------------

// read/write: many readers try to catch many writers while incomplete.
//
// DMP-LABEL: FunctionDecl {{.*}} gr_wp_vs_read_write
//       DMP: ACCAtomicDirective
//  DMP-NEXT:   ACCWriteClause
//   DMP-NOT:     <implicit>
//  DMP-NEXT:   impl: OMPAtomicDirective
//  DMP-NEXT:     OMPWriteClause
//  DMP-NEXT:     BinaryOperator {{.*}} '='
//       DMP: ACCAtomicDirective
//  DMP-NEXT:   ACCReadClause
//   DMP-NOT:     <implicit>
//  DMP-NEXT:   impl: OMPAtomicDirective
//  DMP-NEXT:     OMPReadClause
//  DMP-NEXT:     BinaryOperator {{.*}} '='
//
//   PRT-LABEL: void gr_wp_vs_read_write({{.*}})
//         PRT: if (i == 0) {
//  PRT-A-NEXT:   #pragma acc atomic write
// PRT-AO-NEXT:   // #pragma omp atomic write
//  PRT-O-NEXT:   #pragma omp atomic write
// PRT-OA-NEXT:   // #pragma acc atomic write
//    PRT-NEXT:   *x = {{.*}};
//    PRT-NEXT: } else {
//    PRT-NEXT:   WideInt v;
//  PRT-A-NEXT:   #pragma acc atomic read
// PRT-AO-NEXT:   // #pragma omp atomic read
//  PRT-O-NEXT:   #pragma omp atomic read
// PRT-OA-NEXT:   // #pragma acc atomic read
//    PRT-NEXT:   v = *x;
//         PRT: }
#pragma acc routine worker
void gr_wp_vs_read_write(WideInt *x, bool *err) {
  #pragma acc loop worker
  for (int i = 0; i < WP_VS_NUM_WORKERS; ++i) {
    if (i == 0) {
      #pragma acc atomic write
      *x = NEW_VAL;
    } else {
      WideInt v;
      #pragma acc atomic read
      v = *x;
      if (v != OLD_VAL && v != NEW_VAL)
        *err = true;
    }
  }
}

// update: atomically adjust a counter.
//
// DMP-LABEL: FunctionDecl {{.*}} gr_wp_vs_update
//       DMP: ACCAtomicDirective
//  DMP-NEXT:   ACCUpdateClause
//   DMP-NOT:     <implicit>
//  DMP-NEXT:   impl: OMPAtomicDirective
//  DMP-NEXT:     OMPUpdateClause
//  DMP-NEXT:     UnaryOperator {{.*}} '++'
//
//   PRT-LABEL: void gr_wp_vs_update({{.*}})
//         PRT: for (int i = {{.*}}) {
//  PRT-A-NEXT:   #pragma acc atomic update
// PRT-AO-NEXT:   // #pragma omp atomic update
//  PRT-O-NEXT:   #pragma omp atomic update
// PRT-OA-NEXT:   // #pragma acc atomic update
//    PRT-NEXT:   ++*x;
//    PRT-NEXT: }
#pragma acc routine worker
void gr_wp_vs_update(int *x) {
  #pragma acc loop worker
  for (int i = 0; i < WP_VS_NUM_WORKERS; ++i) {
    #pragma acc atomic update
    ++*x;
  }
}

// capture: atomically adjust a counter and capture the old value.
//
// DMP-LABEL: FunctionDecl {{.*}} gr_wp_vs_capture
//       DMP: ACCAtomicDirective
//  DMP-NEXT:   ACCCaptureClause
//   DMP-NOT:     <implicit>
//  DMP-NEXT:   impl: OMPAtomicDirective
//  DMP-NEXT:     OMPCaptureClause
//  DMP-NEXT:     CompoundStmt
//
//   PRT-LABEL: void gr_wp_vs_capture({{.*}})
//         PRT: int v;
//  PRT-A-NEXT: #pragma acc atomic capture
// PRT-AO-NEXT: // #pragma omp atomic capture
//  PRT-O-NEXT: #pragma omp atomic capture
// PRT-OA-NEXT: // #pragma acc atomic capture
//    PRT-NEXT: {
//    PRT-NEXT:   ++*x;
//    PRT-NEXT:   v = *x;
//    PRT-NEXT: }
//    PRT-NEXT: arr[v - 1] =
#pragma acc routine worker
void gr_wp_vs_capture(int *x, bool *arr) {
  #pragma acc loop worker
  for (int i = 0; i < WP_VS_NUM_WORKERS; ++i) {
    int v;
    #pragma acc atomic capture
    {
      ++*x;
      v = *x;
    }
    arr[v - 1] = true;
  }
}

// compare with '<' or '>': search for min or max value.
//
// DMP-LABEL: FunctionDecl {{.*}} gr_wp_vs_compare
//       DMP: ACCAtomicDirective
//  DMP-NEXT:   ACCCompareClause
//   DMP-NOT:     <implicit>
//  DMP-NEXT:   impl: OMPAtomicDirective
//  DMP-NEXT:     OMPCompareClause
//  DMP-NEXT:     BinaryOperator {{.*}} '='
//
//   PRT-LABEL: void gr_wp_vs_compare({{.*}})
//         PRT: for (int i = {{.*}}) {
//  PRT-A-NEXT:   #pragma acc atomic compare
// PRT-AO-NEXT:   // #pragma omp atomic compare
//  PRT-O-NEXT:   #pragma omp atomic compare
// PRT-OA-NEXT:   // #pragma acc atomic compare
//    PRT-NEXT:   *x = Base + i > *x ? Base + i : *x;
//    PRT-NEXT: }
#pragma acc routine worker
void gr_wp_vs_compare(int *x, int gangIdx) {
  int Base = gangIdx * WP_VS_NUM_WORKERS;
  #pragma acc loop worker
  for (int i = 1; i <= WP_VS_NUM_WORKERS; ++i) {
    #pragma acc atomic compare
    *x = Base + i > *x ? Base + i : *x;
  }
}

//------------------------------------------------------------------------------
// Orphaned loops for GR/WS/VP mode.
//------------------------------------------------------------------------------

// read/write: many readers try to catch many writers while incomplete.
//
// DMP-LABEL: FunctionDecl {{.*}} gr_ws_vp_read_write
//       DMP: ACCAtomicDirective
//  DMP-NEXT:   ACCWriteClause
//   DMP-NOT:     <implicit>
//  DMP-NEXT:   impl: OMPAtomicDirective
//  DMP-NEXT:     OMPWriteClause
//  DMP-NEXT:     BinaryOperator {{.*}} '='
//       DMP: ACCAtomicDirective
//  DMP-NEXT:   ACCReadClause
//   DMP-NOT:     <implicit>
//  DMP-NEXT:   impl: OMPAtomicDirective
//  DMP-NEXT:     OMPReadClause
//  DMP-NEXT:     BinaryOperator {{.*}} '='
//
//   PRT-LABEL: void gr_ws_vp_read_write({{.*}})
//         PRT: if (i == 0) {
//  PRT-A-NEXT:   #pragma acc atomic write
// PRT-AO-NEXT:   // #pragma omp atomic write
//  PRT-O-NEXT:   #pragma omp atomic write
// PRT-OA-NEXT:   // #pragma acc atomic write
//    PRT-NEXT:   *x = {{.*}};
//    PRT-NEXT: } else {
//    PRT-NEXT:   WideInt v;
//  PRT-A-NEXT:   #pragma acc atomic read
// PRT-AO-NEXT:   // #pragma omp atomic read
//  PRT-O-NEXT:   #pragma omp atomic read
// PRT-OA-NEXT:   // #pragma acc atomic read
//    PRT-NEXT:   v = *x;
//         PRT: }
#pragma acc routine vector
void gr_ws_vp_read_write(WideInt *x, bool *err) {
  #pragma acc loop vector
  for (int i = 0; i < WS_VP_VECTOR_LENGTH; ++i) {
    if (i == 0) {
      #pragma acc atomic write
      *x = NEW_VAL;
    } else {
      WideInt v;
      #pragma acc atomic read
      v = *x;
      if (v != OLD_VAL && v != NEW_VAL)
        *err = true;
    }
  }
}

// update: atomically adjust a counter.
//
// DMP-LABEL: FunctionDecl {{.*}} gr_ws_vp_update
//       DMP: ACCAtomicDirective
//  DMP-NEXT:   ACCUpdateClause
//   DMP-NOT:     <implicit>
//  DMP-NEXT:   impl: OMPAtomicDirective
//  DMP-NEXT:     OMPUpdateClause
//  DMP-NEXT:     UnaryOperator {{.*}} '--'
//
//   PRT-LABEL: void gr_ws_vp_update({{.*}})
//         PRT: for (int i = {{.*}}) {
//  PRT-A-NEXT:   #pragma acc atomic update
// PRT-AO-NEXT:   // #pragma omp atomic update
//  PRT-O-NEXT:   #pragma omp atomic update
// PRT-OA-NEXT:   // #pragma acc atomic update
//    PRT-NEXT:   --*x;
//    PRT-NEXT: }
#pragma acc routine vector
void gr_ws_vp_update(int *x) {
  #pragma acc loop vector
  for (int i = 0; i < WS_VP_VECTOR_LENGTH; ++i) {
    #pragma acc atomic update
    --*x;
  }
}

// capture: atomically adjust a counter and capture the old value.
//
// DMP-LABEL: FunctionDecl {{.*}} gr_ws_vp_capture
//       DMP: ACCAtomicDirective
//  DMP-NEXT:   ACCCaptureClause
//   DMP-NOT:     <implicit>
//  DMP-NEXT:   impl: OMPAtomicDirective
//  DMP-NEXT:     OMPCaptureClause
//  DMP-NEXT:     CompoundStmt
//
//   PRT-LABEL: void gr_ws_vp_capture({{.*}})
//         PRT: int v;
//  PRT-A-NEXT: #pragma acc atomic capture
// PRT-AO-NEXT: // #pragma omp atomic capture
//  PRT-O-NEXT: #pragma omp atomic capture
// PRT-OA-NEXT: // #pragma acc atomic capture
//    PRT-NEXT: {
//    PRT-NEXT:   v = *x;
//    PRT-NEXT:   --*x;
//    PRT-NEXT: }
//    PRT-NEXT: arr[v - 1] =
#pragma acc routine vector
void gr_ws_vp_capture(int *x, bool *arr) {
  #pragma acc loop vector
  for (int i = 0; i < WS_VP_VECTOR_LENGTH; ++i) {
    int v;
    #pragma acc atomic capture
    {
      v = *x;
      --*x;
    }
    arr[v - 1] = true;
  }
}

// compare with '<' or '>': search for min or max value.
//
// DMP-LABEL: FunctionDecl {{.*}} gr_ws_vp_compare
//       DMP: ACCAtomicDirective
//  DMP-NEXT:   ACCCompareClause
//   DMP-NOT:     <implicit>
//  DMP-NEXT:   impl: OMPAtomicDirective
//  DMP-NEXT:     OMPCompareClause
//  DMP-NEXT:     BinaryOperator {{.*}} '='
//
//   PRT-LABEL: void gr_ws_vp_compare({{.*}})
//         PRT: for (int i = {{.*}}) {
//  PRT-A-NEXT:   #pragma acc atomic compare
// PRT-AO-NEXT:   // #pragma omp atomic compare
//  PRT-O-NEXT:   #pragma omp atomic compare
// PRT-OA-NEXT:   // #pragma acc atomic compare
//    PRT-NEXT:   *x = *x < Base + i ? Base + i : *x;
//    PRT-NEXT: }
#pragma acc routine vector
void gr_ws_vp_compare(int *x, int gangIdx) {
  int Base = gangIdx * WS_VP_VECTOR_LENGTH;
  #pragma acc loop vector
  for (int i = 1; i <= WS_VP_VECTOR_LENGTH; ++i) {
    #pragma acc atomic compare
    *x = *x < Base + i ? Base + i : *x;
  }
}

//------------------------------------------------------------------------------
// Orphaned loops for GR/WP/VP mode.
//------------------------------------------------------------------------------

// read/write: many readers try to catch many writers while incomplete.
//
// DMP-LABEL: FunctionDecl {{.*}} gr_wp_vp_read_write
//       DMP: ACCAtomicDirective
//  DMP-NEXT:   ACCWriteClause
//   DMP-NOT:     <implicit>
//  DMP-NEXT:   impl: OMPAtomicDirective
//  DMP-NEXT:     OMPWriteClause
//  DMP-NEXT:     BinaryOperator {{.*}} '='
//       DMP: ACCAtomicDirective
//  DMP-NEXT:   ACCReadClause
//   DMP-NOT:     <implicit>
//  DMP-NEXT:   impl: OMPAtomicDirective
//  DMP-NEXT:     OMPReadClause
//  DMP-NEXT:     BinaryOperator {{.*}} '='
//
//   PRT-LABEL: void gr_wp_vp_read_write({{.*}})
//         PRT: if (i == 0) {
//  PRT-A-NEXT:   #pragma acc atomic write
// PRT-AO-NEXT:   // #pragma omp atomic write
//  PRT-O-NEXT:   #pragma omp atomic write
// PRT-OA-NEXT:   // #pragma acc atomic write
//    PRT-NEXT:   *x = {{.*}};
//    PRT-NEXT: } else {
//    PRT-NEXT:   WideInt v;
//  PRT-A-NEXT:   #pragma acc atomic read
// PRT-AO-NEXT:   // #pragma omp atomic read
//  PRT-O-NEXT:   #pragma omp atomic read
// PRT-OA-NEXT:   // #pragma acc atomic read
//    PRT-NEXT:   v = *x;
//         PRT: }
#pragma acc routine worker
void gr_wp_vp_read_write(WideInt *x, bool *err) {
  #pragma acc loop worker vector
  for (int i = 0; i < WP_VP_NUM_WORKERS * WP_VP_VECTOR_LENGTH; ++i) {
    if (i == 0) {
      #pragma acc atomic write
      *x = NEW_VAL;
    } else {
      WideInt v;
      #pragma acc atomic read
      v = *x;
      if (v != OLD_VAL && v != NEW_VAL)
        *err = true;
    }
  }
}

// update: atomically adjust a counter.
//
// DMP-LABEL: FunctionDecl {{.*}} gr_wp_vp_update
//       DMP: ACCAtomicDirective
//  DMP-NEXT:   ACCUpdateClause
//   DMP-NOT:     <implicit>
//  DMP-NEXT:   impl: OMPAtomicDirective
//  DMP-NEXT:     OMPUpdateClause
//  DMP-NEXT:     UnaryOperator {{.*}} '--'
//
//   PRT-LABEL: void gr_wp_vp_update({{.*}})
//         PRT: for (int i = {{.*}}) {
//  PRT-A-NEXT:   #pragma acc atomic update
// PRT-AO-NEXT:   // #pragma omp atomic update
//  PRT-O-NEXT:   #pragma omp atomic update
// PRT-OA-NEXT:   // #pragma acc atomic update
//    PRT-NEXT:   --*x;
//    PRT-NEXT: }
#pragma acc routine worker
void gr_wp_vp_update(int *x) {
  #pragma acc loop worker vector
  for (int i = 0; i < WP_VP_NUM_WORKERS * WP_VP_VECTOR_LENGTH; ++i) {
    #pragma acc atomic update
    --*x;
  }
}

// capture: atomically adjust a counter and capture the old value.
//
// DMP-LABEL: FunctionDecl {{.*}} gr_wp_vp_capture
//       DMP: ACCAtomicDirective
//  DMP-NEXT:   ACCCaptureClause
//   DMP-NOT:     <implicit>
//  DMP-NEXT:   impl: OMPAtomicDirective
//  DMP-NEXT:     OMPCaptureClause
//  DMP-NEXT:     CompoundStmt
//
//   PRT-LABEL: void gr_wp_vp_capture({{.*}})
//         PRT: int v;
//  PRT-A-NEXT: #pragma acc atomic capture
// PRT-AO-NEXT: // #pragma omp atomic capture
//  PRT-O-NEXT: #pragma omp atomic capture
// PRT-OA-NEXT: // #pragma acc atomic capture
//    PRT-NEXT: {
//    PRT-NEXT:   v = *x;
//    PRT-NEXT:   --*x;
//    PRT-NEXT: }
//    PRT-NEXT: arr[v - 1] =
#pragma acc routine worker
void gr_wp_vp_capture(int *x, bool *arr) {
  #pragma acc loop worker vector
  for (int i = 0; i < WP_VP_NUM_WORKERS * WP_VP_VECTOR_LENGTH; ++i) {
    int v;
    #pragma acc atomic capture
    {
      v = *x;
      --*x;
    }
    arr[v - 1] = true;
  }
}

// compare with '<' or '>': search for min or max value.
//
// DMP-LABEL: FunctionDecl {{.*}} gr_wp_vp_compare
//       DMP: ACCAtomicDirective
//  DMP-NEXT:   ACCCompareClause
//   DMP-NOT:     <implicit>
//  DMP-NEXT:   impl: OMPAtomicDirective
//  DMP-NEXT:     OMPCompareClause
//  DMP-NEXT:     BinaryOperator {{.*}} '='
//
//   PRT-LABEL: void gr_wp_vp_compare({{.*}})
//         PRT: for (int i = {{.*}}) {
//  PRT-A-NEXT:   #pragma acc atomic compare
// PRT-AO-NEXT:   // #pragma omp atomic compare
//  PRT-O-NEXT:   #pragma omp atomic compare
// PRT-OA-NEXT:   // #pragma acc atomic compare
//    PRT-NEXT:   *x = *x > Base + i ? Base + i : *x;
//    PRT-NEXT: }
#pragma acc routine worker
void gr_wp_vp_compare(int *x, int gangIdx) {
  int Base = gangIdx * WP_VP_NUM_WORKERS * WP_VP_VECTOR_LENGTH;
  #pragma acc loop worker vector
  for (int i = 1; i <= WP_VP_NUM_WORKERS * WP_VP_VECTOR_LENGTH; ++i) {
    #pragma acc atomic compare
    *x = *x > Base + i ? Base + i : *x;
  }
}

int main() {
  bool err;

  // EXE-NOT: {{.}}
  // EXE: start
  printf("start\n");

  //----------------------------------------------------------------------------
  // GR/WS/VS mode.
  //----------------------------------------------------------------------------

  // read/write: many readers try to catch one writer while it's incomplete.
  //
  // DMP-LABEL: "GR/WS/VS read/write: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCWriteClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPWriteClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCReadClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPReadClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("GR/WS/VS read/write: ");
  //         PRT: if (gangIdx == 0) {
  //  PRT-A-NEXT:   #pragma acc atomic write
  // PRT-AO-NEXT:   // #pragma omp atomic write
  //  PRT-O-NEXT:   #pragma omp atomic write
  // PRT-OA-NEXT:   // #pragma acc atomic write
  //    PRT-NEXT:   x = {{.*}};
  //    PRT-NEXT: } else {
  //    PRT-NEXT:   WideInt v;
  //  PRT-A-NEXT:   #pragma acc atomic read
  // PRT-AO-NEXT:   // #pragma omp atomic read
  //  PRT-O-NEXT:   #pragma omp atomic read
  // PRT-OA-NEXT:   // #pragma acc atomic read
  //    PRT-NEXT:   v = x;
  //         PRT: }
  //
  // EXE-NEXT: GR/WS/VS read/write: err=0
  printf("GR/WS/VS read/write: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    WideInt x = OLD_VAL;
    #pragma acc parallel num_gangs(WS_VS_NUM_GANGS) copy(x, err)
    {
      int gangIdx;
      #pragma acc loop gang
      for (int i = 0; i < WS_VS_NUM_GANGS; ++i)
        gangIdx = i;
      if (gangIdx == 0) {
        #pragma acc atomic write
        x = NEW_VAL;
      } else {
        WideInt v;
        #pragma acc atomic read
        v = x;
        if (v != OLD_VAL && v != NEW_VAL)
          err = true;
      }
    }
  }
  printf("err=%d\n", err);

  // update: atomically adjust a counter.
  //
  // DMP-LABEL: "GR/WS/VS update: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     UnaryOperator {{.*}} '++'
  //
  //   PRT-LABEL: printf("GR/WS/VS update: ");
  //         PRT: {
  //         PRT:   {
  //  PRT-A-NEXT:     #pragma acc atomic update
  // PRT-AO-NEXT:     // #pragma omp atomic update
  //  PRT-O-NEXT:     #pragma omp atomic update
  // PRT-OA-NEXT:     // #pragma acc atomic update
  //    PRT-NEXT:     x++;
  //    PRT-NEXT:   }
  //         PRT: }
  //
  // EXE-NEXT: GR/WS/VS update: err=0
  printf("GR/WS/VS update: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    #pragma acc parallel num_gangs(WS_VS_NUM_GANGS) copy(x)
    {
      #pragma acc atomic update
      x++;
    }
    if (x != WS_VS_NUM_GANGS)
      err = true;
  }
  printf("err=%d\n", err);

  // capture: atomically adjust a counter and capture the old value.
  //
  // DMP-LABEL: "GR/WS/VS capture: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("GR/WS/VS capture: ");
  //         PRT: int v;
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: v = x++;
  //    PRT-NEXT: arr[v] =
  //
  // EXE-NEXT: GR/WS/VS capture: err=0
  printf("GR/WS/VS capture: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    bool arr[WS_VS_NUM_GANGS];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(WS_VS_NUM_GANGS) copy(x, arr)
    {
      int v;
      #pragma acc atomic capture
      v = x++;
      arr[v] = true;
    }
    for (int i = 0; i < WS_VS_NUM_GANGS; ++i)
      if (!arr[i])
        err = true;
    if (x != WS_VS_NUM_GANGS)
      err = true;
  }
  printf("err=%d\n", err);

  // compare with '<' or '>': search for min or max value.
  //
  // DMP-LABEL: "GR/WS/VS compare: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCompareClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCompareClause
  //  DMP-NEXT:     IfStmt
  //
  //   PRT-LABEL: printf("GR/WS/VS compare: ");
  //         PRT:   gangIdx = i;
  //  PRT-A-NEXT: #pragma acc atomic compare
  // PRT-AO-NEXT: // #pragma omp atomic compare
  //  PRT-O-NEXT: #pragma omp atomic compare
  // PRT-OA-NEXT: // #pragma acc atomic compare
  //    PRT-NEXT: if (gangIdx + 1 < x) {
  //    PRT-NEXT:   x = gangIdx + 1;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: GR/WS/VS compare: err=0
  printf("GR/WS/VS compare: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = INT_MAX;
    #pragma acc parallel num_gangs(WS_VS_NUM_GANGS) copy(x)
    {
      int gangIdx;
      #pragma acc loop gang
      for (int i = 0; i < WS_VS_NUM_GANGS; ++i)
        gangIdx = i;
      #pragma acc atomic compare
      if (gangIdx + 1 < x) {
        x = gangIdx + 1;
      }
    }
    if (x != 1)
      err = true;
  }
  printf("err=%d\n", err);

  //----------------------------------------------------------------------------
  // GP/WS/VS mode.
  //----------------------------------------------------------------------------

  // read/write: many readers try to catch one writer while it's incomplete.
  //
  // DMP-LABEL: "GP/WS/VS read/write: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCWriteClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPWriteClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCReadClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPReadClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("GP/WS/VS read/write: ");
  //         PRT: if (gangIdx == 0) {
  //  PRT-A-NEXT:   #pragma acc atomic write
  // PRT-AO-NEXT:   // #pragma omp atomic write
  //  PRT-O-NEXT:   #pragma omp atomic write
  // PRT-OA-NEXT:   // #pragma acc atomic write
  //    PRT-NEXT:   x = {{.*}};
  //    PRT-NEXT: } else {
  //    PRT-NEXT:   WideInt v;
  //  PRT-A-NEXT:   #pragma acc atomic read
  // PRT-AO-NEXT:   // #pragma omp atomic read
  //  PRT-O-NEXT:   #pragma omp atomic read
  // PRT-OA-NEXT:   // #pragma acc atomic read
  //    PRT-NEXT:   v = x;
  //         PRT: }
  //
  // EXE-NEXT: GP/WS/VS read/write: err=0
  printf("GP/WS/VS read/write: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    WideInt x = OLD_VAL;
    #pragma acc parallel num_gangs(WS_VS_NUM_GANGS) copy(x, err)
    {
      #pragma acc loop gang
      for (int gangIdx = 0; gangIdx < WS_VS_NUM_GANGS; ++gangIdx) {
        if (gangIdx == 0) {
          #pragma acc atomic write
          x = NEW_VAL;
        } else {
          WideInt v;
          #pragma acc atomic read
          v = x;
          if (v != OLD_VAL && v != NEW_VAL)
            err = true;
        }
      }
    }
  }
  printf("err=%d\n", err);

  // update: atomically adjust a counter.
  //
  // DMP-LABEL: "GP/WS/VS update: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     UnaryOperator {{.*}} '--'
  //
  //   PRT-LABEL: printf("GP/WS/VS update: ");
  //         PRT: for (int i = 0; i < {{.*}}; ++i) {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   --x;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: GP/WS/VS update: err=0
  printf("GP/WS/VS update: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = WS_VS_NUM_GANGS;
    #pragma acc parallel num_gangs(WS_VS_NUM_GANGS) copy(x)
    {
      #pragma acc loop gang
      for (int i = 0; i < WS_VS_NUM_GANGS; ++i) {
        #pragma acc atomic update
        --x;
      }
    }
    if (x != 0)
      err = true;
  }
  printf("err=%d\n", err);

  // capture: atomically adjust a counter and capture the old value.
  //
  // DMP-LABEL: "GP/WS/VS capture: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("GP/WS/VS capture: ");
  //         PRT: int v;
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: v = --x;
  //    PRT-NEXT: arr[v] =
  //
  // EXE-NEXT: GP/WS/VS capture: err=0
  printf("GP/WS/VS capture: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = WS_VS_NUM_GANGS;
    bool arr[WS_VS_NUM_GANGS];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(WS_VS_NUM_GANGS) copy(x, arr)
    {
      #pragma acc loop gang
      for (int i = 0; i < WS_VS_NUM_GANGS; ++i) {
        int v;
        #pragma acc atomic capture
        v = --x;
        arr[v] = true;
      }
    }
    for (int i = 0; i < WS_VS_NUM_GANGS; ++i)
      if (!arr[i])
        err = true;
    if (x != 0)
      err = true;
  }
  printf("err=%d\n", err);

  // compare with '<' or '>': search for min or max value.
  //
  // DMP-LABEL: "GP/WS/VS compare: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCompareClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCompareClause
  //  DMP-NEXT:     IfStmt
  //
  //   PRT-LABEL: printf("GP/WS/VS compare: ");
  //         PRT: for (int i = {{.*}}) {
  //  PRT-A-NEXT:   #pragma acc atomic compare
  // PRT-AO-NEXT:   // #pragma omp atomic compare
  //  PRT-O-NEXT:   #pragma omp atomic compare
  // PRT-OA-NEXT:   // #pragma acc atomic compare
  //    PRT-NEXT:   if (i > x) {
  //    PRT-NEXT:     x = i;
  //    PRT-NEXT:   }
  //    PRT-NEXT: }
  //
  // EXE-NEXT: GP/WS/VS compare: err=0
  printf("GP/WS/VS compare: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    #pragma acc parallel num_gangs(WS_VS_NUM_GANGS) copy(x)
    {
      #pragma acc loop gang
      for (int i = 1; i <= WS_VS_NUM_GANGS; ++i) {
        #pragma acc atomic compare
        if (i > x) {
          x = i;
        }
      }
    }
    if (x != WS_VS_NUM_GANGS)
      err = true;
  }
  printf("err=%d\n", err);

  //----------------------------------------------------------------------------
  // GP/WP/VS mode.
  //----------------------------------------------------------------------------

  // read/write: many readers try to catch one writer while it's incomplete.
  //
  // DMP-LABEL: "GP/WP/VS read/write: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCWriteClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPWriteClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCReadClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPReadClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("GP/WP/VS read/write: ");
  //         PRT: if (i == 0) {
  //  PRT-A-NEXT:   #pragma acc atomic write
  // PRT-AO-NEXT:   // #pragma omp atomic write
  //  PRT-O-NEXT:   #pragma omp atomic write
  // PRT-OA-NEXT:   // #pragma acc atomic write
  //    PRT-NEXT:   x = {{.*}};
  //    PRT-NEXT: } else {
  //    PRT-NEXT:   WideInt v;
  //  PRT-A-NEXT:   #pragma acc atomic read
  // PRT-AO-NEXT:   // #pragma omp atomic read
  //  PRT-O-NEXT:   #pragma omp atomic read
  // PRT-OA-NEXT:   // #pragma acc atomic read
  //    PRT-NEXT:   v = x;
  //         PRT: }
  //
  // EXE-NEXT: GP/WP/VS read/write: err=0
  printf("GP/WP/VS read/write: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    WideInt x = OLD_VAL;
    #pragma acc parallel num_gangs(WP_VS_NUM_GANGS)                            \
                         num_workers(WP_VS_NUM_WORKERS)                        \
                         copy(x, err)
    {
      #pragma acc loop gang worker
      for (int i = 0; i < WP_VS_NUM_GANGS * WP_VS_NUM_WORKERS; ++i) {
        if (i == 0) {
          #pragma acc atomic write
          x = NEW_VAL;
        } else {
          WideInt v;
          #pragma acc atomic read
          v = x;
          if (v != OLD_VAL && v != NEW_VAL)
            err = true;
        }
      }
    }
  }
  printf("err=%d\n", err);

  // update: atomically adjust a counter.
  //
  // DMP-LABEL: "GP/WP/VS update: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     CompoundAssignOperator {{.*}} '+='
  //
  //   PRT-LABEL: printf("GP/WP/VS update: ");
  //         PRT: for (int i = 0; i < {{.*}}; ++i) {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x += 2;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: GP/WP/VS update: err=0
  printf("GP/WP/VS update: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    #pragma acc parallel num_gangs(WP_VS_NUM_GANGS)                            \
                         num_workers(WP_VS_NUM_WORKERS)                        \
                         copy(x)
    {
      #pragma acc loop gang worker
      for (int i = 0; i < WP_VS_NUM_GANGS * WP_VS_NUM_WORKERS; ++i) {
        #pragma acc atomic update
        x += 2;
      }
    }
    if (x != 2 * WP_VS_NUM_GANGS * WP_VS_NUM_WORKERS)
      err = true;
  }
  printf("err=%d\n", err);

  // capture: atomically adjust a counter and capture the old value.
  //
  // DMP-LABEL: "GP/WP/VS capture: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("GP/WP/VS capture: ");
  //         PRT: int v;
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: v = x += 1;
  //    PRT-NEXT: arr[v - 1] =
  //
  // EXE-NEXT: GP/WP/VS capture: err=0
  printf("GP/WP/VS capture: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    bool arr[WP_VS_NUM_GANGS * WP_VS_NUM_WORKERS];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(WP_VS_NUM_GANGS)                            \
                         num_workers(WP_VS_NUM_WORKERS)                        \
                         copy(x, arr)
    {
      #pragma acc loop gang worker
      for (int i = 0; i < WP_VS_NUM_GANGS * WP_VS_NUM_WORKERS; ++i) {
        int v;
        #pragma acc atomic capture
        v = x += 1;
        arr[v - 1] = true;
      }
    }
    for (int i = 0; i < WP_VS_NUM_GANGS * WP_VS_NUM_WORKERS; ++i)
      if (!arr[i])
        err = true;
    if (x != WP_VS_NUM_GANGS * WP_VS_NUM_WORKERS)
      err = true;
  }
  printf("err=%d\n", err);

  // compare with '<' or '>': search for min or max value.
  //
  // DMP-LABEL: "GP/WP/VS compare: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCompareClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCompareClause
  //  DMP-NEXT:     IfStmt
  //
  //   PRT-LABEL: printf("GP/WP/VS compare: ");
  //         PRT: for (int i = {{.*}}) {
  //  PRT-A-NEXT:   #pragma acc atomic compare
  // PRT-AO-NEXT:   // #pragma omp atomic compare
  //  PRT-O-NEXT:   #pragma omp atomic compare
  // PRT-OA-NEXT:   // #pragma acc atomic compare
  //    PRT-NEXT:   if (x < i) {
  //    PRT-NEXT:     x = i;
  //    PRT-NEXT:   }
  //    PRT-NEXT: }
  //
  // EXE-NEXT: GP/WP/VS compare: err=0
  printf("GP/WP/VS compare: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    #pragma acc parallel num_gangs(WP_VS_NUM_GANGS)                            \
                         num_workers(WP_VS_NUM_WORKERS)                        \
                         copy(x)
    {
      #pragma acc loop gang worker
      for (int i = 1; i <= WP_VS_NUM_GANGS * WP_VS_NUM_WORKERS; ++i) {
        #pragma acc atomic compare
        if (x < i) {
          x = i;
        }
      }
    }
    if (x != WP_VS_NUM_GANGS * WP_VS_NUM_WORKERS)
      err = true;
  }
  printf("err=%d\n", err);

  //----------------------------------------------------------------------------
  // GP/WS/VP mode.
  //----------------------------------------------------------------------------

  // read/write: many readers try to catch one writer while it's incomplete.
  //
  // DMP-LABEL: "GP/WS/VP read/write: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCWriteClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPWriteClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //      DMP: ACCAtomicDirective
  // DMP-NEXT:   ACCReadClause
  //  DMP-NOT:     <implicit>
  // DMP-NEXT:   impl: OMPAtomicDirective
  // DMP-NEXT:     OMPReadClause
  // DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("GP/WS/VP read/write: ");
  //         PRT: if (i == 0) {
  //  PRT-A-NEXT:   #pragma acc atomic write
  // PRT-AO-NEXT:   // #pragma omp atomic write
  //  PRT-O-NEXT:   #pragma omp atomic write
  // PRT-OA-NEXT:   // #pragma acc atomic write
  //    PRT-NEXT:   x = {{.*}};
  //    PRT-NEXT: } else {
  //    PRT-NEXT:   WideInt v;
  //  PRT-A-NEXT:   #pragma acc atomic read
  // PRT-AO-NEXT:   // #pragma omp atomic read
  //  PRT-O-NEXT:   #pragma omp atomic read
  // PRT-OA-NEXT:   // #pragma acc atomic read
  //    PRT-NEXT:   v = x;
  //         PRT: }
  //
  // EXE-NEXT: GP/WS/VP read/write: err=0
  printf("GP/WS/VP read/write: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    WideInt x = OLD_VAL;
    #pragma acc parallel num_gangs(WS_VP_NUM_GANGS)                            \
                         vector_length(WS_VP_VECTOR_LENGTH)                    \
                         copy(x, err)
    {
      #pragma acc loop gang vector
      for (int i = 0; i < WS_VP_NUM_GANGS * WS_VP_VECTOR_LENGTH; ++i) {
        if (i == 0) {
          #pragma acc atomic write
          x = NEW_VAL;
        } else {
          WideInt v;
          #pragma acc atomic read
          v = x;
          if (v != OLD_VAL && v != NEW_VAL)
            err = true;
        }
      }
    }
  }
  printf("err=%d\n", err);

  // update: atomically adjust a counter.
  //
  // DMP-LABEL: "GP/WS/VP update: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("GP/WS/VP update: ");
  //         PRT: for (int i = 0; i < {{.*}}; ++i) {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x = x + 3;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: GP/WS/VP update: err=0
  printf("GP/WS/VP update: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    #pragma acc parallel num_gangs(WS_VP_NUM_GANGS)                            \
                         vector_length(WS_VP_VECTOR_LENGTH)                    \
                         copy(x)
    {
      #pragma acc loop gang vector
      for (int i = 0; i < WS_VP_NUM_GANGS * WS_VP_VECTOR_LENGTH; ++i) {
        #pragma acc atomic update
        x = x + 3;
      }
    }
    if (x != 3 * WS_VP_NUM_GANGS * WS_VP_VECTOR_LENGTH)
      err = true;
  }
  printf("err=%d\n", err);

  // capture: atomically adjust a counter and capture the old value.
  //
  // DMP-LABEL: "GP/WS/VP capture: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("GP/WS/VP capture: ");
  //         PRT: for (int i = 0; i < {{.*}}; ++i) {
  //    PRT-NEXT:   int v;
  //  PRT-A-NEXT:   #pragma acc atomic capture
  // PRT-AO-NEXT:   // #pragma omp atomic capture
  //  PRT-O-NEXT:   #pragma omp atomic capture
  // PRT-OA-NEXT:   // #pragma acc atomic capture
  //    PRT-NEXT:   v = x = x + 1;
  //    PRT-NEXT:   arr[v - 1] =
  //    PRT-NEXT: }
  //
  // EXE-NEXT: GP/WS/VP capture: err=0
  printf("GP/WS/VP capture: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    bool arr[WS_VP_NUM_GANGS * WS_VP_VECTOR_LENGTH];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(WS_VP_NUM_GANGS)                            \
                         vector_length(WS_VP_VECTOR_LENGTH)                    \
                         copy(x, arr)
    {
      #pragma acc loop gang vector
      for (int i = 0; i < WS_VP_NUM_GANGS * WS_VP_VECTOR_LENGTH; ++i) {
        int v;
        #pragma acc atomic capture
        v = x = x + 1;
        arr[v - 1] = true;
      }
    }
    for (int i = 0; i < WS_VP_NUM_GANGS * WS_VP_VECTOR_LENGTH; ++i)
      if (!arr[i])
        err = true;
    if (x != WS_VP_NUM_GANGS * WS_VP_VECTOR_LENGTH)
      err = true;
  }
  printf("err=%d\n", err);

  // compare with '<' or '>': search for min or max value.
  //
  // DMP-LABEL: "GP/WS/VP compare: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCompareClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCompareClause
  //  DMP-NEXT:     IfStmt
  //
  //   PRT-LABEL: printf("GP/WS/VP compare: ");
  //         PRT: for (int i = {{.*}}) {
  //  PRT-A-NEXT:   #pragma acc atomic compare
  // PRT-AO-NEXT:   // #pragma omp atomic compare
  //  PRT-O-NEXT:   #pragma omp atomic compare
  // PRT-OA-NEXT:   // #pragma acc atomic compare
  //    PRT-NEXT:   if (x > i + 1) {
  //    PRT-NEXT:     x = i + 1;
  //    PRT-NEXT:   }
  //    PRT-NEXT: }
  //
  // EXE-NEXT: GP/WS/VP compare: err=0
  printf("GP/WS/VP compare: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = INT_MAX;
    #pragma acc parallel num_gangs(WS_VP_NUM_GANGS)                            \
                         vector_length(WS_VP_VECTOR_LENGTH)                    \
                         copy(x)
    {
      #pragma acc loop gang worker
      for (int i = 0; i < WS_VP_NUM_GANGS * WS_VP_VECTOR_LENGTH; ++i) {
        #pragma acc atomic compare
        if (x > i + 1) {
          x = i + 1;
        }
      }
    }
    if (x != 1)
      err = true;
  }
  printf("err=%d\n", err);

  //----------------------------------------------------------------------------
  // GP/WP/VP mode.
  //----------------------------------------------------------------------------

  // read/write: many readers try to catch one writer while it's incomplete.
  //
  // DMP-LABEL: "GP/WP/VP read/write: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCWriteClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPWriteClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCReadClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPReadClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("GP/WP/VP read/write: ");
  //         PRT: if (i == 0) {
  //  PRT-A-NEXT:   #pragma acc atomic write
  // PRT-AO-NEXT:   // #pragma omp atomic write
  //  PRT-O-NEXT:   #pragma omp atomic write
  // PRT-OA-NEXT:   // #pragma acc atomic write
  //    PRT-NEXT:   x = {{.*}};
  //    PRT-NEXT: } else {
  //    PRT-NEXT:   WideInt v;
  //  PRT-A-NEXT:   #pragma acc atomic read
  // PRT-AO-NEXT:   // #pragma omp atomic read
  //  PRT-O-NEXT:   #pragma omp atomic read
  // PRT-OA-NEXT:   // #pragma acc atomic read
  //    PRT-NEXT:   v = x;
  //         PRT: }
  //
  // EXE-NEXT: GP/WP/VP read/write: err=0
  printf("GP/WP/VP read/write: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    WideInt x = OLD_VAL;
    #pragma acc parallel num_gangs(WP_VP_NUM_GANGS)                            \
                         num_workers(WP_VP_NUM_WORKERS)                        \
                         vector_length(WP_VP_VECTOR_LENGTH)                    \
                         copy(x, err)
    {
      #pragma acc loop gang worker vector
      for (int i = 0;
           i < WP_VP_NUM_GANGS * WP_VP_NUM_WORKERS * WP_VP_VECTOR_LENGTH;
           ++i) {
        if (i == 0) {
          #pragma acc atomic write
          x = NEW_VAL;
        } else {
          WideInt v;
          #pragma acc atomic read
          v = x;
          if (v != OLD_VAL && v != NEW_VAL)
            err = true;
        }
      }
    }
  }
  printf("err=%d\n", err);

  // update: atomically adjust a counter.
  //
  // DMP-LABEL: "GP/WP/VP update: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("GP/WP/VP update: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x = 5 + x;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: GP/WP/VP update: err=0
  printf("GP/WP/VP update: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    #pragma acc parallel num_gangs(WP_VP_NUM_GANGS)                            \
                         num_workers(WP_VP_NUM_WORKERS)                        \
                         vector_length(WP_VP_VECTOR_LENGTH)                    \
                         copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0;
           i < WP_VP_NUM_GANGS * WP_VP_NUM_WORKERS * WP_VP_VECTOR_LENGTH;
           ++i) {
        #pragma acc atomic update
        x = 5 + x;
      }
    }
    if (x != 5 * WP_VP_NUM_GANGS * WP_VP_NUM_WORKERS * WP_VP_VECTOR_LENGTH)
      err = true;
  }
  printf("err=%d\n", err);

  // capture: atomically adjust a counter and capture the old value.
  //
  // DMP-LABEL: "GP/WP/VP capture: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("GP/WP/VP capture: ");
  //         PRT: for (int i =
  //         PRT: {
  //    PRT-NEXT:   int v;
  //  PRT-A-NEXT:   #pragma acc atomic capture
  // PRT-AO-NEXT:   // #pragma omp atomic capture
  //  PRT-O-NEXT:   #pragma omp atomic capture
  // PRT-OA-NEXT:   // #pragma acc atomic capture
  //    PRT-NEXT:   v = x = 1 + x;
  //    PRT-NEXT:   arr[v - 1] =
  //    PRT-NEXT: }
  //
  // EXE-NEXT: GP/WP/VP capture: err=0
  printf("GP/WP/VP capture: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    bool arr[WP_VP_NUM_GANGS * WP_VP_NUM_WORKERS * WP_VP_VECTOR_LENGTH];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(WP_VP_NUM_GANGS)                            \
                         num_workers(WP_VP_NUM_WORKERS)                        \
                         vector_length(WP_VP_VECTOR_LENGTH)                    \
                         copy(x, arr)
    {
      #pragma acc loop gang worker vector
      for (int i = 0;
           i < WP_VP_NUM_GANGS * WP_VP_NUM_WORKERS * WP_VP_VECTOR_LENGTH;
           ++i) {
        int v;
        #pragma acc atomic capture
        v = x = 1 + x;
        arr[v - 1] = true;
      }
    }
    for (int i = 0;
         i < WP_VP_NUM_GANGS * WP_VP_NUM_WORKERS * WP_VP_VECTOR_LENGTH;
         ++i)
      if (!arr[i])
        err = true;
    if (x != WP_VP_NUM_GANGS * WP_VP_NUM_WORKERS *WP_VP_VECTOR_LENGTH)
      err = true;
  }
  printf("err=%d\n", err);

  // compare with '<' or '>': search for min or max value.
  //
  // DMP-LABEL: "GP/WP/VP compare: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCompareClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCompareClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("GP/WP/VP compare: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic compare
  // PRT-AO-NEXT:   // #pragma omp atomic compare
  //  PRT-O-NEXT:   #pragma omp atomic compare
  // PRT-OA-NEXT:   // #pragma acc atomic compare
  //    PRT-NEXT:   x = i + 1 < x ? i + 1 : x;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: GP/WP/VP compare: err=0
  printf("GP/WP/VP compare: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = INT_MAX;
    #pragma acc parallel num_gangs(WP_VP_NUM_GANGS)                            \
                         num_workers(WP_VP_NUM_WORKERS)                        \
                         vector_length(WP_VP_VECTOR_LENGTH)                    \
                         copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0;
           i < WP_VP_NUM_GANGS * WP_VP_NUM_WORKERS * WP_VP_VECTOR_LENGTH;
           ++i) {
        #pragma acc atomic compare
        x = i + 1 < x ? i + 1 : x;
      }
    }
    if (x != 1)
      err = true;
  }
  printf("err=%d\n", err);

  //----------------------------------------------------------------------------
  // GR/WP/VS mode.
  //----------------------------------------------------------------------------

  // EXE-NEXT: GR/WP/VS read/write: err=0
  printf("GR/WP/VS read/write: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    WideInt x = OLD_VAL;
    #pragma acc parallel num_gangs(WP_VS_NUM_GANGS)                            \
                         num_workers(WP_VS_NUM_WORKERS)                        \
                         copy(x, err)
    gr_wp_vs_read_write(&x, &err);
  }
  printf("err=%d\n", err);

  // EXE-NEXT: GR/WP/VS update: err=0
  printf("GR/WP/VS update: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    #pragma acc parallel num_gangs(WP_VS_NUM_GANGS)                            \
                         num_workers(WP_VS_NUM_WORKERS)                        \
                         copy(x, err)
    gr_wp_vs_update(&x);
    if (x != WP_VS_NUM_GANGS * WP_VS_NUM_WORKERS)
      err = true;
  }
  printf("err=%d\n", err);

  // EXE-NEXT: GR/WP/VS capture: err=0
  printf("GR/WP/VS capture: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    bool arr[WP_VS_NUM_GANGS * WP_VS_NUM_WORKERS];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(WP_VS_NUM_GANGS)                            \
                         num_workers(WP_VS_NUM_WORKERS)                        \
                         copy(x, err)
    gr_wp_vs_capture(&x, arr);
    for (int i = 0; i < WP_VS_NUM_GANGS * WP_VS_NUM_WORKERS; ++i)
      if (!arr[i])
        err = true;
    if (x != WP_VS_NUM_GANGS * WP_VS_NUM_WORKERS)
      err = true;
  }
  printf("err=%d\n", err);

  // EXE-NEXT: GR/WP/VS compare: err=0
  printf("GR/WP/VS compare: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    #pragma acc parallel num_gangs(WP_VS_NUM_GANGS)                            \
                         num_workers(WP_VS_NUM_WORKERS)                        \
                         copy(x, err)
    {
      int gangIdx;
      #pragma acc loop gang
      for (int i = 0; i < WP_VS_NUM_GANGS; ++i)
        gangIdx = i;
      gr_wp_vs_compare(&x, gangIdx);
    }
    if (x != WP_VS_NUM_GANGS * WP_VS_NUM_WORKERS)
      err = true;
  }
  printf("err=%d\n", err);

  //----------------------------------------------------------------------------
  // GR/WS/VP mode.
  //----------------------------------------------------------------------------

  // EXE-NEXT: GR/WS/VP read/write: err=0
  printf("GR/WS/VP read/write: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    WideInt x = OLD_VAL;
    #pragma acc parallel num_gangs(WS_VP_NUM_GANGS)                            \
                         vector_length(WS_VP_VECTOR_LENGTH)                    \
                         copy(x, err)
    gr_ws_vp_read_write(&x, &err);
  }
  printf("err=%d\n", err);

  // EXE-NEXT: GR/WS/VP update: err=0
  printf("GR/WS/VP update: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x =  WS_VP_NUM_GANGS * WS_VP_VECTOR_LENGTH;
    #pragma acc parallel num_gangs(WS_VP_NUM_GANGS)                            \
                         vector_length(WS_VP_VECTOR_LENGTH)                    \
                         copy(x, err)
    gr_ws_vp_update(&x);
    if (x != 0)
      err = true;
  }
  printf("err=%d\n", err);

  // EXE-NEXT: GR/WS/VP capture: err=0
  printf("GR/WS/VP capture: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = WS_VP_NUM_GANGS * WS_VP_VECTOR_LENGTH;
    bool arr[WS_VP_NUM_GANGS * WS_VP_VECTOR_LENGTH];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(WS_VP_NUM_GANGS)                            \
                         vector_length(WS_VP_VECTOR_LENGTH)                    \
                         copy(x, err)
    gr_ws_vp_capture(&x, arr);
    for (int i = 0; i < WS_VP_NUM_GANGS * WS_VP_VECTOR_LENGTH; ++i)
      if (!arr[i])
        err = true;
    if (x != 0)
      err = true;
  }
  printf("err=%d\n", err);

  // EXE-NEXT: GR/WS/VP compare: err=0
  printf("GR/WS/VP compare: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    #pragma acc parallel num_gangs(WS_VP_NUM_GANGS)                            \
                         vector_length(WS_VP_VECTOR_LENGTH)                    \
                         copy(x, err)
    {
      int gangIdx;
      #pragma acc loop gang
      for (int i = 0; i < WS_VP_NUM_GANGS; ++i)
        gangIdx = i;
      gr_ws_vp_compare(&x, gangIdx);
    }
    if (x != WS_VP_NUM_GANGS * WS_VP_VECTOR_LENGTH)
      err = true;
  }
  printf("err=%d\n", err);

  //----------------------------------------------------------------------------
  // GR/WP/VP mode.
  //----------------------------------------------------------------------------

  // EXE-NEXT: GR/WP/VP read/write: err=0
  printf("GR/WP/VP read/write: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    WideInt x = OLD_VAL;
    #pragma acc parallel num_gangs(WP_VP_NUM_GANGS)                            \
                         num_workers(WP_VP_NUM_WORKERS)                        \
                         vector_length(WP_VP_VECTOR_LENGTH)                    \
                         copy(x, err)
    gr_wp_vp_read_write(&x, &err);
  }
  printf("err=%d\n", err);

  // EXE-NEXT: GR/WP/VP update: err=0
  printf("GR/WP/VP update: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x =  WP_VP_NUM_GANGS * WP_VP_NUM_WORKERS * WP_VP_VECTOR_LENGTH;
    #pragma acc parallel num_gangs(WP_VP_NUM_GANGS)                            \
                         num_workers(WP_VP_NUM_WORKERS)                        \
                         vector_length(WP_VP_VECTOR_LENGTH)                    \
                         copy(x, err)
    gr_wp_vp_update(&x);
    if (x != 0)
      err = true;
  }
  printf("err=%d\n", err);

  // EXE-NEXT: GR/WP/VP capture: err=0
  printf("GR/WP/VP capture: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = WP_VP_NUM_GANGS * WP_VP_NUM_WORKERS * WP_VP_VECTOR_LENGTH;
    bool arr[WP_VP_NUM_GANGS * WP_VP_NUM_WORKERS * WP_VP_VECTOR_LENGTH];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(WP_VP_NUM_GANGS)                            \
                         num_workers(WP_VP_NUM_WORKERS)                        \
                         vector_length(WP_VP_VECTOR_LENGTH)                    \
                         copy(x, err)
    gr_wp_vp_capture(&x, arr);
    for (int i = 0;
         i < WP_VP_NUM_GANGS * WP_VP_NUM_WORKERS * WP_VP_VECTOR_LENGTH; ++i)
      if (!arr[i])
        err = true;
    if (x != 0)
      err = true;
  }
  printf("err=%d\n", err);

  // EXE-NEXT: GR/WP/VP compare: err=0
  printf("GR/WP/VP compare: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = INT_MAX;
    #pragma acc parallel num_gangs(WP_VP_NUM_GANGS)                            \
                         num_workers(WP_VP_NUM_WORKERS)                        \
                         vector_length(WP_VP_VECTOR_LENGTH)                    \
                         copy(x, err)
    {
      int gangIdx;
      #pragma acc loop gang
      for (int i = 0; i < WP_VP_NUM_GANGS; ++i)
        gangIdx = i;
      gr_wp_vp_compare(&x, gangIdx);
    }
    if (x != 1)
      err = true;
  }
  printf("err=%d\n", err);

  //============================================================================
  // Check every associated statement form for update, capture, and compare, but
  // just for GP/WP/VP mode.  Checking every combination of form and mode would
  // seem like overkill.
  //============================================================================

  //----------------------------------------------------------------------------
  // update: atomically adjust a counter.
  //
  // Here, we also check every binop for every form.
  //----------------------------------------------------------------------------

  // DMP-LABEL: "update x++: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     UnaryOperator {{.*}} '++'
  //
  //   PRT-LABEL: printf("update x++: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x++;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x++: err=0
  printf("update x++: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        x++;
      }
    }
    if (x != 8)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "update x--: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     UnaryOperator {{.*}} '--'
  //
  //   PRT-LABEL: printf("update x--: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x--;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x--: err=0
  printf("update x--: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 8;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        x--;
      }
    }
    if (x != 0)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "update ++x: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     UnaryOperator {{.*}} '++'
  //
  //   PRT-LABEL: printf("update ++x: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   ++x;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update ++x: err=0
  printf("update ++x: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        ++x;
      }
    }
    if (x != 8)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "update --x: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     UnaryOperator {{.*}} '--'
  //
  //   PRT-LABEL: printf("update --x: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   --x;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update --x: err=0
  printf("update --x: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 8;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        --x;
      }
    }
    if (x != 0)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "update x += expr: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     CompoundAssignOperator {{.*}} '+='
  //
  //   PRT-LABEL: printf("update x += expr: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x += 2;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x += expr: err=0
  printf("update x += expr: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        x += 2;
      }
    }
    if (x != 2 * 8)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "update x *= expr: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     CompoundAssignOperator {{.*}} '*='
  //
  //   PRT-LABEL: printf("update x *= expr: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x *= 2;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x *= expr: err=0
  printf("update x *= expr: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 1;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        x *= 2;
      }
    }
    if (x != 256)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "update x -= expr: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     CompoundAssignOperator {{.*}} '-='
  //
  //   PRT-LABEL: printf("update x -= expr: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x -= 1;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x -= expr: err=0
  printf("update x -= expr: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 8;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        x -= 1;
      }
    }
    if (x != 0)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "update x /= expr: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     CompoundAssignOperator {{.*}} '/='
  //
  //   PRT-LABEL: printf("update x /= expr: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x /= 2;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x /= expr: err=0
  printf("update x /= expr: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    double x = 1;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        x /= 2;
      }
    }
    double xExp = pow(2, -8);
    if ((x - xExp) / xExp > ERROR)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "update x &= expr: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     CompoundAssignOperator {{.*}} '&='
  //
  //   PRT-LABEL: printf("update x &= expr: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x &= {{.*}};
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x &= expr: err=0
  printf("update x &= expr: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    unsigned x = 0xffff;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        x &= UINT_MAX - (1 << (2 * i));
      }
    }
    if (x != 0xaaaa)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "update x ^= expr: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     CompoundAssignOperator {{.*}} '^='
  //
  //   PRT-LABEL: printf("update x ^= expr: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x ^= {{.*}};
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x ^= expr: err=0
  printf("update x ^= expr: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    unsigned x = 0xffff;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        x ^= UINT_MAX - (1 << (2 * i));
      }
    }
    if (x != 0xaaaa)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "update x |= expr: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     CompoundAssignOperator {{.*}} '|='
  //
  //   PRT-LABEL: printf("update x |= expr: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x |= {{.*}};
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x |= expr: err=0
  printf("update x |= expr: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    unsigned x = 0;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        x |= 1 << (2 * i);
      }
    }
    if (x != 0x5555)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "update x <<= expr: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     CompoundAssignOperator {{.*}} '<<='
  //
  //   PRT-LABEL: printf("update x <<= expr: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x <<= {{.*}};
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x <<= expr: err=0
  printf("update x <<= expr: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    unsigned x = 1;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        x <<= 1;
      }
    }
    if (x != 0x100)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "update x >>= expr: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     CompoundAssignOperator {{.*}} '>>='
  //
  //   PRT-LABEL: printf("update x >>= expr: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x >>= {{.*}};
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x >>= expr: err=0
  printf("update x >>= expr: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    unsigned x = 0x100;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        x >>= 1;
      }
    }
    if (x != 1)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "update x = x + expr: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("update x = x + expr: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x = x + 2;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x = x + expr: err=0
  printf("update x = x + expr: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        x = x + 2;
      }
    }
    if (x != 2 * 8)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "update x = x * expr: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("update x = x * expr: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x = x * 2;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x = x * expr: err=0
  printf("update x = x * expr: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 1;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        x = x * 2;
      }
    }
    if (x != 256)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "update x = x - expr: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("update x = x - expr: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x = x - 1;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x = x - expr: err=0
  printf("update x = x - expr: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 8;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        x = x - 1;
      }
    }
    if (x != 0)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "update x = x / expr: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("update x = x / expr: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x = x / 2;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x = x / expr: err=0
  printf("update x = x / expr: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    double x = 1;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        x = x / 2;
      }
    }
    double xExp = pow(2, -8);
    if ((x - xExp) / xExp > ERROR)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "update x = x & expr: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("update x = x & expr: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x = x & {{.*}};
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x = x & expr: err=0
  printf("update x = x & expr: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    unsigned x = 0xffff;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        x = x & (UINT_MAX - (1 << (2 * i)));
      }
    }
    if (x != 0xaaaa)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "update x = x ^ expr: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("update x = x ^ expr: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x = x ^ {{.*}};
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x = x ^ expr: err=0
  printf("update x = x ^ expr: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    unsigned x = 0xffff;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        x = x ^ (UINT_MAX - (1 << (2 * i)));
      }
    }
    if (x != 0xaaaa)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "update x = x | expr: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("update x = x | expr: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x = x | {{.*}};
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x = x | expr: err=0
  printf("update x = x | expr: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    unsigned x = 0;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        x = x | (1 << (2 * i));
      }
    }
    if (x != 0x5555)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "update x = x << expr: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("update x = x << expr: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x = x << {{.*}};
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x = x << expr: err=0
  printf("update x = x << expr: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    unsigned x = 1;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        x = x << 1;
      }
    }
    if (x != 0x100)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "update x = x >> expr: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("update x = x >> expr: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x = x >> {{.*}};
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x = x >> expr: err=0
  printf("update x = x >> expr: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    unsigned x = 0x100;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        x = x >> 1;
      }
    }
    if (x != 1)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "update x = expr + x: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("update x = expr + x: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x = 2 + x;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x = expr + x: err=0
  printf("update x = expr + x: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        x = 2 + x;
      }
    }
    if (x != 2 * 8)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "update x = expr * x: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("update x = expr * x: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x = 2 * x;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x = expr * x: err=0
  printf("update x = expr * x: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 1;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        x = 2 * x;
      }
    }
    if (x != 256)
      err = true;
  }
  printf("err=%d\n", err);

  // It's easy to produce a race with this form: each iteration of the loop will
  // add a value from expr that will be negated in each of the subsequent
  // iterations, but the order of iterations and thus the number of negations
  // for each iteration's value is unpredictable.  Fortunately, when expr has a
  // fixed value, the order doesn't matter, so the final value is predictable.
  //
  // DMP-LABEL: "update x = expr - x: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("update x = expr - x: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x = 1 - x;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x = expr - x: err=0
  printf("update x = expr - x: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 5;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 9; ++i) {
        #pragma acc atomic update
        x = 1 - x;
      }
    }
    if (x != -4)
      err = true;
  }
  printf("err=%d\n", err);

  // As in the previous case (except it's now a product instead of a sum), it's
  // easy to produce a race, and we make expr a fixed value to avoid that.
  //
  // DMP-LABEL: "update x = expr / x: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("update x = expr / x: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x = 2 / x;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x = expr / x: err=0
  printf("update x = expr / x: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    double x = 1;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 9; ++i) {
        #pragma acc atomic update
        x = 2 / x;
      }
    }
    double xExp = 2;
    if ((x - xExp) / xExp > ERROR)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "update x = expr & x: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("update x = expr & x: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x = {{.*}} & x;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x = expr & x: err=0
  printf("update x = expr & x: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    unsigned x = 0xffff;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        x = (UINT_MAX - (1 << (2 * i))) & x;
      }
    }
    if (x != 0xaaaa)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "update x = expr ^ x: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("update x = expr ^ x: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x = {{.*}} ^ x;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x = expr ^ x: err=0
  printf("update x = expr ^ x: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    unsigned x = 0xffff;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        x = (UINT_MAX - (1 << (2 * i))) ^ x;
      }
    }
    if (x != 0xaaaa)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "update x = expr | x: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("update x = expr | x: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x = {{.*}} | x;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x = expr | x: err=0
  printf("update x = expr | x: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    unsigned x = 0;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic update
        x = (1 << (2 * i)) | x;
      }
    }
    if (x != 0x5555)
      err = true;
  }
  printf("err=%d\n", err);

  // This is another form that's easily racy if expr doesn't have a fixed value.
  //
  // DMP-LABEL: "update x = expr << x: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("update x = expr << x: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x = 1 << x;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x = expr << x: err=0
  printf("update x = expr << x: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    unsigned x = 1;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 4; ++i) {
        #pragma acc atomic update
        x = 1 << x;
      }
    }
    if (x != 0x10000)
      err = true;
  }
  printf("err=%d\n", err);

  // This is another form that's easily racy if expr doesn't have a fixed value.
  //
  // DMP-LABEL: "update x = expr >> x: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("update x = expr >> x: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x = {{.*}} >> x;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: update x = expr >> x: err=0
  printf("update x = expr >> x: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    unsigned x = 3;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 4; ++i) {
        #pragma acc atomic update
        x = 0x10 >> x;
      }
    }
    if (x != 0x8)
      err = true;
  }
  printf("err=%d\n", err);

  //----------------------------------------------------------------------------
  // capture: atomically adjust a variable and capture the old value.
  //
  // We already checked every binop for every form of update.  For capture, we
  // just check that every form works for one binop and assume that every
  // remaining binop is handled in the same manner as for update.  There are so
  // many capture forms that checking every binop for every form would seem like
  // overkill.
  //----------------------------------------------------------------------------

  // DMP-LABEL: "capture v = x++: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("capture v = x++: ");
  //         PRT: int v;
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: v = x++;
  //    PRT-NEXT: arr[v] =
  //
  // EXE-NEXT: capture v = x++: err=0
  printf("capture v = x++: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    bool arr[8];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) \
                copy(x, arr)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        int v;
        #pragma acc atomic capture
        v = x++;
        arr[v] = true;
      }
    }
    for (int i = 0; i < 8; ++i)
      if (!arr[i])
        err = true;
    if (x != 8)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "capture v = x--: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("capture v = x--: ");
  //         PRT: int v;
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: v = x--;
  //    PRT-NEXT: arr[v - 1] = 
  //
  // EXE-NEXT: capture v = x--: err=0
  printf("capture v = x--: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 8;
    bool arr[8];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) \
                copy(x, arr)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        int v;
        #pragma acc atomic capture
        v = x--;
        arr[v - 1] = true;
      }
    }
    for (int i = 0; i < 8; ++i)
      if (!arr[i])
        err = true;
    if (x != 0)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "capture v = ++x: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("capture v = ++x: ");
  //         PRT: int v;
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: v = ++x;
  //    PRT-NEXT: arr[v - 1] =
  //
  // EXE-NEXT: capture v = ++x: err=0
  printf("capture v = ++x: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    bool arr[8];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) \
                copy(x, arr)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        int v;
        #pragma acc atomic capture
        v = ++x;
        arr[v - 1] = true;
      }
    }
    for (int i = 0; i < 8; ++i)
      if (!arr[i])
        err = true;
    if (x != 8)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "capture v = --x: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("capture v = --x: ");
  //         PRT: int v;
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: v = --x;
  //    PRT-NEXT: arr[v] = 
  //
  // EXE-NEXT: capture v = --x: err=0
  printf("capture v = --x: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 8;
    bool arr[8];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) \
                copy(x, arr)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        int v;
        #pragma acc atomic capture
        v = --x;
        arr[v] = true;
      }
    }
    for (int i = 0; i < 8; ++i)
      if (!arr[i])
        err = true;
    if (x != 0)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "capture v = x binop= expr: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("capture v = x binop= expr: ");
  //         PRT: int v
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: v = x += 1;
  //    PRT-NEXT: arr[v - 1] =
  //
  // EXE-NEXT: capture v = x binop= expr: err=0
  printf("capture v = x binop= expr: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    bool arr[8];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) \
                copy(x, arr)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        int v;
        #pragma acc atomic capture
        v = x += 1;
        arr[v - 1] = true;
      }
    }
    for (int i = 0; i < 8; ++i)
      if (!arr[i])
        err = true;
    if (x != 8)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "capture v = x = x binop expr: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("capture v = x = x binop expr: ");
  //         PRT: int v
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: v = x = x - 1;
  //    PRT-NEXT: arr[v] =
  //
  // EXE-NEXT: capture v = x = x binop expr: err=0
  printf("capture v = x = x binop expr: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 8;
    bool arr[8];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) \
                copy(x, arr)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        int v;
        #pragma acc atomic capture
        v = x = x - 1;
        arr[v] = true;
      }
    }
    for (int i = 0; i < 8; ++i)
      if (!arr[i])
        err = true;
    if (x != 0)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "capture v = x = expr binop x: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("capture v = x = expr binop x: ");
  //         PRT: int v
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: v = x = 2 * x;
  //    PRT-NEXT: arr[v] =
  //
  // EXE-NEXT: capture v = x = expr binop x: err=0
  printf("capture v = x = expr binop x: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 1;
    bool arr[257];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) \
                copy(x, arr)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        int v;
        #pragma acc atomic capture
        v = x = 2 * x;
        arr[v] = true;
      }
    }
    for (int i = 0; i <= 256; ++i) {
      switch (i) {
      case 2: case 4: case 8: case 16: case 32: case 64: case 128: case 256:
        if (!arr[i])
          err = true;
        break;
      default:
        if (arr[i])
          err = true;
      }
    }
    if (x != 256)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "capture {v = x; x = expr;}: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     CompoundStmt
  //
  //   PRT-LABEL: printf("capture {v = x; x = expr;}: ");
  //         PRT: int v
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: {
  //    PRT-NEXT:   v = x;
  //    PRT-NEXT:   x = i + 1;
  //    PRT-NEXT: }
  //    PRT-NEXT: arr[v] =
  //
  // EXE-NEXT: capture {v = x; x = expr;}: err=0
  printf("capture {v = x; x = expr;}: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    bool arr[9];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) \
                copy(x, arr)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        int v;
        #pragma acc atomic capture
        {
          v = x;
          x = i + 1;
        }
        arr[v] = true;
      }
    }
    if (!arr[0])
      err = true;
    int count = 0;
    for (int i = 1; i < 9; ++i) {
      if (arr[i])
        ++count;
    }
    if (count != 7)
      err = true;
    if (x < 1 || 8 < x)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "capture {v = x; x++;}: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     CompoundStmt
  //
  //   PRT-LABEL: printf("capture {v = x; x++;}: ");
  //         PRT: int v
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: {
  //    PRT-NEXT:   v = x;
  //    PRT-NEXT:   x++;
  //    PRT-NEXT: }
  //    PRT-NEXT: arr[v] =
  //
  // EXE-NEXT: capture {v = x; x++;}: err=0
  printf("capture {v = x; x++;}: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    bool arr[8];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) \
                copy(x, arr)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        int v;
        #pragma acc atomic capture
        {
          v = x;
          x++;
        }
        arr[v] = true;
      }
    }
    for (int i = 0; i < 8; ++i)
      if (!arr[i])
        err = true;
    if (x != 8)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "capture {v = x; x--;}: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     CompoundStmt
  //
  //   PRT-LABEL: printf("capture {v = x; x--;}: ");
  //         PRT: int v
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: {
  //    PRT-NEXT:   v = x;
  //    PRT-NEXT:   x--;
  //    PRT-NEXT: }
  //    PRT-NEXT: arr[v - 1] =
  //
  // EXE-NEXT: capture {v = x; x--;}: err=0
  printf("capture {v = x; x--;}: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 8;
    bool arr[8];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) \
                copy(x, arr)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        int v;
        #pragma acc atomic capture
        {
          v = x;
          x--;
        }
        arr[v - 1] = true;
      }
    }
    for (int i = 0; i < 8; ++i)
      if (!arr[i])
        err = true;
    if (x != 0)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "capture {v = x; ++x;}: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     CompoundStmt
  //
  //   PRT-LABEL: printf("capture {v = x; ++x;}: ");
  //         PRT: int v
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: {
  //    PRT-NEXT:   v = x;
  //    PRT-NEXT:   ++x;
  //    PRT-NEXT: }
  //    PRT-NEXT: arr[v] =
  //
  // EXE-NEXT: capture {v = x; ++x;}: err=0
  printf("capture {v = x; ++x;}: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    bool arr[8];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) \
                copy(x, arr)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        int v;
        #pragma acc atomic capture
        {
          v = x;
          ++x;
        }
        arr[v] = true;
      }
    }
    for (int i = 0; i < 8; ++i)
      if (!arr[i])
        err = true;
    if (x != 8)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "capture {v = x; --x;}: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     CompoundStmt
  //
  //   PRT-LABEL: printf("capture {v = x; --x;}: ");
  //         PRT: int v
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: {
  //    PRT-NEXT:   v = x;
  //    PRT-NEXT:   --x;
  //    PRT-NEXT: }
  //    PRT-NEXT: arr[v - 1] =
  //
  // EXE-NEXT: capture {v = x; --x;}: err=0
  printf("capture {v = x; --x;}: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 8;
    bool arr[8];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) \
                copy(x, arr)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        int v;
        #pragma acc atomic capture
        {
          v = x;
          --x;
        }
        arr[v - 1] = true;
      }
    }
    for (int i = 0; i < 8; ++i)
      if (!arr[i])
        err = true;
    if (x != 0)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "capture {v = x; x binop= expr;}: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     CompoundStmt
  //
  //   PRT-LABEL: printf("capture {v = x; x binop= expr;}: ");
  //         PRT: int v
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: {
  //    PRT-NEXT:   v = x;
  //    PRT-NEXT:   x /= 2;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: capture {v = x; x binop= expr;}: err=0
  printf("capture {v = x; x binop= expr;}: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 256;
    bool arr[257];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) \
                copy(x, arr)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        int v;
        #pragma acc atomic capture
        {
          v = x;
          x /= 2;
        }
        arr[v] = true;
      }
    }
    for (int i = 0; i <= 256; ++i) {
      switch (i) {
      case 2: case 4: case 8: case 16: case 32: case 64: case 128: case 256:
        if (!arr[i])
          err = true;
        break;
      default:
        if (arr[i])
          err = true;
      }
    }
    if (x != 1)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "capture {v = x; x = x binop expr;}: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     CompoundStmt
  //
  //   PRT-LABEL: printf("capture {v = x; x = x binop expr;}: ");
  //         PRT: int v
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: {
  //    PRT-NEXT:   v = x;
  //    PRT-NEXT:   x = x + 1;
  //    PRT-NEXT: }
  //    PRT-NEXT: arr[v] =
  //
  // EXE-NEXT: capture {v = x; x = x binop expr;}: err=0
  printf("capture {v = x; x = x binop expr;}: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    bool arr[8];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) \
                copy(x, arr)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        int v;
        #pragma acc atomic capture
        {
          v = x;
          x = x + 1;
        }
        arr[v] = true;
      }
    }
    for (int i = 0; i < 8; ++i)
      if (!arr[i])
        err = true;
    if (x != 8)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "capture {v = x; x = expr binop x;}: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     CompoundStmt
  //
  //   PRT-LABEL: printf("capture {v = x; x = expr binop x;}: ");
  //         PRT: int v
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: {
  //    PRT-NEXT:   v = x;
  //    PRT-NEXT:   x = 1 + x;
  //    PRT-NEXT: }
  //    PRT-NEXT: arr[v] =
  //
  // EXE-NEXT: capture {v = x; x = expr binop x;}: err=0
  printf("capture {v = x; x = expr binop x;}: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    bool arr[8];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) \
                copy(x, arr)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        int v;
        #pragma acc atomic capture
        {
          v = x;
          x = 1 + x;
        }
        arr[v] = true;
      }
    }
    for (int i = 0; i < 8; ++i)
      if (!arr[i])
        err = true;
    if (x != 8)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "capture {x++; v = x;}: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     CompoundStmt
  //
  //   PRT-LABEL: printf("capture {x++; v = x;}: ");
  //         PRT: int v
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: {
  //    PRT-NEXT:   x++;
  //    PRT-NEXT:   v = x;
  //    PRT-NEXT: }
  //    PRT-NEXT: arr[v - 1] =
  //
  // EXE-NEXT: capture {x++; v = x;}: err=0
  printf("capture {x++; v = x;}: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    bool arr[8];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) \
                copy(x, arr)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        int v;
        #pragma acc atomic capture
        {
          x++;
          v = x;
        }
        arr[v - 1] = true;
      }
    }
    for (int i = 0; i < 8; ++i)
      if (!arr[i])
        err = true;
    if (x != 8)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "capture {x--; v = x;}: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     CompoundStmt
  //
  //   PRT-LABEL: printf("capture {x--; v = x;}: ");
  //         PRT: int v
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: {
  //    PRT-NEXT:   x--;
  //    PRT-NEXT:   v = x;
  //    PRT-NEXT: }
  //    PRT-NEXT: arr[v] =
  //
  // EXE-NEXT: capture {x--; v = x;}: err=0
  printf("capture {x--; v = x;}: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 8;
    bool arr[8];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) \
                copy(x, arr)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        int v;
        #pragma acc atomic capture
        {
          x--;
          v = x;
        }
        arr[v] = true;
      }
    }
    for (int i = 0; i < 8; ++i)
      if (!arr[i])
        err = true;
    if (x != 0)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "capture {++x; v = x;}: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     CompoundStmt
  //
  //   PRT-LABEL: printf("capture {++x; v = x;}: ");
  //         PRT: int v
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: {
  //    PRT-NEXT:   ++x;
  //    PRT-NEXT:   v = x;
  //    PRT-NEXT: }
  //    PRT-NEXT: arr[v - 1] =
  //
  // EXE-NEXT: capture {++x; v = x;}: err=0
  printf("capture {++x; v = x;}: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    bool arr[8];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) \
                copy(x, arr)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        int v;
        #pragma acc atomic capture
        {
          ++x;
          v = x;
        }
        arr[v - 1] = true;
      }
    }
    for (int i = 0; i < 8; ++i)
      if (!arr[i])
        err = true;
    if (x != 8)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "capture {--x; v = x;}: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     CompoundStmt
  //
  //   PRT-LABEL: printf("capture {--x; v = x;}: ");
  //         PRT: int v
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: {
  //    PRT-NEXT:   --x;
  //    PRT-NEXT:   v = x;
  //    PRT-NEXT: }
  //    PRT-NEXT: arr[v] =
  //
  // EXE-NEXT: capture {--x; v = x;}: err=0
  printf("capture {--x; v = x;}: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 8;
    bool arr[8];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) \
                copy(x, arr)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        int v;
        #pragma acc atomic capture
        {
          --x;
          v = x;
        }
        arr[v] = true;
      }
    }
    for (int i = 0; i < 8; ++i)
      if (!arr[i])
        err = true;
    if (x != 0)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "capture {x binop= expr; v = x;}: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     CompoundStmt
  //
  //   PRT-LABEL: printf("capture {x binop= expr; v = x;}: ");
  //         PRT: int v
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: {
  //    PRT-NEXT:   x *= 2;
  //    PRT-NEXT:   v = x;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: capture {x binop= expr; v = x;}: err=0
  printf("capture {x binop= expr; v = x;}: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 1;
    bool arr[257];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) \
                copy(x, arr)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        int v;
        #pragma acc atomic capture
        {
          x *= 2;
          v = x;
        }
        arr[v] = true;
      }
    }
    for (int i = 0; i <= 256; ++i) {
      switch (i) {
      case 2: case 4: case 8: case 16: case 32: case 64: case 128: case 256:
        if (!arr[i])
          err = true;
        break;
      default:
        if (arr[i])
          err = true;
      }
    }
    if (x != 256)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "capture {x = x binop expr; v = x;}: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     CompoundStmt
  //
  //   PRT-LABEL: printf("capture {x = x binop expr; v = x;}: ");
  //         PRT: int v
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: {
  //    PRT-NEXT:   x = x + 1;
  //    PRT-NEXT:   v = x;
  //    PRT-NEXT: }
  //    PRT-NEXT: arr[v - 1] =
  //
  // EXE-NEXT: capture {x = x binop expr; v = x;}: err=0
  printf("capture {x = x binop expr; v = x;}: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    bool arr[8];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) \
                copy(x, arr)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        int v;
        #pragma acc atomic capture
        {
          x = x + 1;
          v = x;
        }
        arr[v - 1] = true;
      }
    }
    for (int i = 0; i < 8; ++i)
      if (!arr[i])
        err = true;
    if (x != 8)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "capture {x = expr binop x; v = x;}: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     CompoundStmt
  //
  //   PRT-LABEL: printf("capture {x = expr binop x; v = x;}: ");
  //         PRT: int v
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: {
  //    PRT-NEXT:   x = 1 + x;
  //    PRT-NEXT:   v = x;
  //    PRT-NEXT: }
  //    PRT-NEXT: arr[v - 1] =
  //
  // EXE-NEXT: capture {x = expr binop x; v = x;}: err=0
  printf("capture {x = expr binop x; v = x;}: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    bool arr[8];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) \
                copy(x, arr)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        int v;
        #pragma acc atomic capture
        {
          x = 1 + x;
          v = x;
        }
        arr[v - 1] = true;
      }
    }
    for (int i = 0; i < 8; ++i)
      if (!arr[i])
        err = true;
    if (x != 8)
      err = true;
  }
  printf("err=%d\n", err);

  //----------------------------------------------------------------------------
  // compare with '<' or '>': search for min or max value.
  //
  // While iterating execution modes above, we already managed to check every
  // form for each of '<' and '>' except the 'if'-statement forms without a
  // compound statement, so we check only those remaining forms here.
  //----------------------------------------------------------------------------

  // DMP-LABEL: "compare if (expr < x) x = expr: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCompareClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCompareClause
  //  DMP-NEXT:     IfStmt
  //
  //   PRT-LABEL: printf("compare if (expr < x) x = expr: ");
  //         PRT: for (int i = {{.*}}) {
  //  PRT-A-NEXT:   #pragma acc atomic compare
  // PRT-AO-NEXT:   // #pragma omp atomic compare
  //  PRT-O-NEXT:   #pragma omp atomic compare
  // PRT-OA-NEXT:   // #pragma acc atomic compare
  //    PRT-NEXT:   if (i < x)
  //    PRT-NEXT:     x = i;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: compare if (expr < x) x = expr: err=0
  printf("compare if (expr < x) x = expr: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = INT_MAX;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 1; i <= 8; ++i) {
        #pragma acc atomic compare
        if (i < x)
          x = i;
      }
    }
    if (x != 1)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "compare if (expr > x) x = expr: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCompareClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCompareClause
  //  DMP-NEXT:     IfStmt
  //
  //   PRT-LABEL: printf("compare if (expr > x) x = expr: ");
  //         PRT: for (int i = {{.*}}) {
  //  PRT-A-NEXT:   #pragma acc atomic compare
  // PRT-AO-NEXT:   // #pragma omp atomic compare
  //  PRT-O-NEXT:   #pragma omp atomic compare
  // PRT-OA-NEXT:   // #pragma acc atomic compare
  //    PRT-NEXT:   if (i + 1 > x)
  //    PRT-NEXT:     x = i + 1;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: compare if (expr > x) x = expr: err=0
  printf("compare if (expr > x) x = expr: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic compare
        if (i + 1 > x)
          x = i + 1;
      }
    }
    if (x != 8)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "compare if (x < expr) x = expr: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCompareClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCompareClause
  //  DMP-NEXT:     IfStmt
  //
  //   PRT-LABEL: printf("compare if (x < expr) x = expr: ");
  //         PRT: for (int i = {{.*}}) {
  //  PRT-A-NEXT:   #pragma acc atomic compare
  // PRT-AO-NEXT:   // #pragma omp atomic compare
  //  PRT-O-NEXT:   #pragma omp atomic compare
  // PRT-OA-NEXT:   // #pragma acc atomic compare
  //    PRT-NEXT:   if (x < i + 1)
  //    PRT-NEXT:     x = i + 1;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: compare if (x < expr) x = expr: err=0
  printf("compare if (x < expr) x = expr: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic compare
        if (x < i + 1)
          x = i + 1;
      }
    }
    if (x != 8)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "compare if (x > expr) x = expr: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCompareClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCompareClause
  //  DMP-NEXT:     IfStmt
  //
  //   PRT-LABEL: printf("compare if (x > expr) x = expr: ");
  //         PRT: for (int i = {{.*}}) {
  //  PRT-A-NEXT:   #pragma acc atomic compare
  // PRT-AO-NEXT:   // #pragma omp atomic compare
  //  PRT-O-NEXT:   #pragma omp atomic compare
  // PRT-OA-NEXT:   // #pragma acc atomic compare
  //    PRT-NEXT:   if (x > i)
  //    PRT-NEXT:     x = i;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: compare if (x > expr) x = expr: err=0
  printf("compare if (x > expr) x = expr: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = INT_MAX;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 1; i <= 8; ++i) {
        #pragma acc atomic compare
        if (x > i)
          x = i;
      }
    }
    if (x != 1)
      err = true;
  }
  printf("err=%d\n", err);

  //----------------------------------------------------------------------------
  // compare with '==': many try to interfere with one changing the value.
  //
  // That is, if x still has the old value, one thread changes it, and many
  // other threads just write the old value back again.  With atomicity in both
  // cases, the many threads should never have any effect.  Otherwise, they are
  // likely to write the old value after the one thread writes the new value.
  //----------------------------------------------------------------------------

  // DMP-LABEL: "compare if (x == e) { x = d; }: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCompareClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCompareClause
  //  DMP-NEXT:     IfStmt
  //
  //   PRT-LABEL: printf("compare if (x == e) { x = d; }: ");
  //         PRT: for (int i = {{.*}}) {
  //  PRT-A-NEXT:   #pragma acc atomic compare
  // PRT-AO-NEXT:   // #pragma omp atomic compare
  //  PRT-O-NEXT:   #pragma omp atomic compare
  // PRT-OA-NEXT:   // #pragma acc atomic compare
  //    PRT-NEXT:   if (x == 77) {
  //    PRT-NEXT:     x = i == 0 ? 88 : 77;
  //    PRT-NEXT:   }
  //    PRT-NEXT: }
  //
  // EXE-NEXT: compare if (x == e) { x = d; }: err=0
  printf("compare if (x == e) { x = d; }: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 77;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic compare
        if (x == 77) {
          x = i == 0 ? 88 : 77;
        }
      }
    }
    if (x != 88)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "compare if (x == e) x = d: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCompareClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCompareClause
  //  DMP-NEXT:     IfStmt
  //
  //   PRT-LABEL: printf("compare if (x == e) x = d: ");
  //         PRT: for (int i = {{.*}}) {
  //  PRT-A-NEXT:   #pragma acc atomic compare
  // PRT-AO-NEXT:   // #pragma omp atomic compare
  //  PRT-O-NEXT:   #pragma omp atomic compare
  // PRT-OA-NEXT:   // #pragma acc atomic compare
  //    PRT-NEXT:   if (x == 77)
  //    PRT-NEXT:     x = i == 0 ? 88 : 77;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: compare if (x == e) x = d: err=0
  printf("compare if (x == e) x = d: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 77;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic compare
        if (x == 77)
          x = i == 0 ? 88 : 77;
      }
    }
    if (x != 88)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "compare x = x == e ? d : x: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCompareClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCompareClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("compare x = x == e ? d : x: ");
  //         PRT: for (int i = {{.*}}) {
  //  PRT-A-NEXT:   #pragma acc atomic compare
  // PRT-AO-NEXT:   // #pragma omp atomic compare
  //  PRT-O-NEXT:   #pragma omp atomic compare
  // PRT-OA-NEXT:   // #pragma acc atomic compare
  //    PRT-NEXT:   x = x == 77 ? (i == 0 ? 88 : 77) : x;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: compare x = x == e ? d : x: err=0
  printf("compare x = x == e ? d : x: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 77;
    #pragma acc parallel num_gangs(2) num_workers(2) vector_length(2) copy(x)
    {
      #pragma acc loop gang worker vector
      for (int i = 0; i < 8; ++i) {
        #pragma acc atomic compare
        x = x == 77 ? (i == 0 ? 88 : 77) : x;
      }
    }
    if (x != 88)
      err = true;
  }
  printf("err=%d\n", err);

  //============================================================================
  // Check that atomic works in acc parallel loop.  Checking a few basic forms
  // should be sufficient.
  //============================================================================

  // DMP-LABEL: "acc parallel loop with acc atomic read/write: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCWriteClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPWriteClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCReadClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPReadClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("acc parallel loop with acc atomic read/write: ");
  //         PRT: if (i == 0) {
  //  PRT-A-NEXT:   #pragma acc atomic write
  // PRT-AO-NEXT:   // #pragma omp atomic write
  //  PRT-O-NEXT:   #pragma omp atomic write
  // PRT-OA-NEXT:   // #pragma acc atomic write
  //    PRT-NEXT:   x = {{.*}};
  //    PRT-NEXT: } else {
  //    PRT-NEXT:   WideInt v;
  //  PRT-A-NEXT:   #pragma acc atomic read
  // PRT-AO-NEXT:   // #pragma omp atomic read
  //  PRT-O-NEXT:   #pragma omp atomic read
  // PRT-OA-NEXT:   // #pragma acc atomic read
  //    PRT-NEXT:   v = x;
  //         PRT: }
  //
  // EXE-NEXT: acc parallel loop with acc atomic read/write: err=0
  printf("acc parallel loop with acc atomic read/write: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    WideInt x = OLD_VAL;
    #pragma acc parallel loop                                                  \
        num_gangs(WP_VP_NUM_GANGS) num_workers(WP_VP_NUM_WORKERS)              \
        vector_length(WP_VP_VECTOR_LENGTH)                                     \
        gang worker vector copy(x, err)
    for (int i = 0;
         i < WP_VP_NUM_GANGS * WP_VP_NUM_WORKERS * WP_VP_VECTOR_LENGTH;
         ++i) {
      if (i == 0) {
        #pragma acc atomic write
        x = NEW_VAL;
      } else {
        WideInt v;
        #pragma acc atomic read
        v = x;
        if (v != OLD_VAL && v != NEW_VAL)
          err = true;
      }
    }
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "acc parallel loop with acc atomic update: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     UnaryOperator {{.*}} '++'
  //
  //   PRT-LABEL: printf("acc parallel loop with acc atomic update: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic update
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic update
  //    PRT-NEXT:   x++;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: acc parallel loop with acc atomic update: err=0
  printf("acc parallel loop with acc atomic update: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    #pragma acc parallel loop num_gangs(2) num_workers(2) vector_length(2)     \
        gang worker vector copy(x)
    for (int i = 0; i < 8; ++i) {
      #pragma acc atomic update
      x++;
    }
    if (x != 8)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "acc parallel loop with acc atomic: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause {{.*}} <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     UnaryOperator {{.*}} '--'
  //
  //   PRT-LABEL: printf("acc parallel loop with acc atomic: ");
  //         PRT: for (int i =
  //         PRT: {
  //  PRT-A-NEXT:   #pragma acc atomic
  // PRT-AO-NEXT:   // #pragma omp atomic update
  //  PRT-O-NEXT:   #pragma omp atomic update
  // PRT-OA-NEXT:   // #pragma acc atomic
  //    PRT-NEXT:   x--;
  //    PRT-NEXT: }
  //
  // EXE-NEXT: acc parallel loop with acc atomic: err=0
  printf("acc parallel loop with acc atomic: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 8;
    #pragma acc parallel loop num_gangs(2) num_workers(2) vector_length(2) \
        gang worker vector copy(x)
    for (int i = 0; i < 8; ++i) {
      #pragma acc atomic
      x--;
    }
    if (x != 0)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "acc parallel loop with acc atomic capture: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("acc parallel loop with acc atomic capture: ");
  //         PRT: int v
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: v = x += 1;
  //    PRT-NEXT: arr[v - 1] =
  //
  // EXE-NEXT: acc parallel loop with acc atomic capture: err=0
  printf("acc parallel loop with acc atomic capture: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    bool arr[8];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel loop num_gangs(2) num_workers(2) vector_length(2) \
        gang worker vector copy(x, arr)
    for (int i = 0; i < 8; ++i) {
      int v;
      #pragma acc atomic capture
      v = x += 1;
      arr[v - 1] = true;
    }
    for (int i = 0; i < 8; ++i)
      if (!arr[i])
        err = true;
    if (x != 8)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "acc parallel loop with acc atomic capture compound statement: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     CompoundStmt
  //
  //   PRT-LABEL: printf("acc parallel loop with acc atomic capture compound statement: ");
  //         PRT: int v
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: {
  //    PRT-NEXT:   v = x;
  //    PRT-NEXT:   x++;
  //    PRT-NEXT: }
  //    PRT-NEXT: arr[v] =
  //
  // EXE-NEXT: acc parallel loop with acc atomic capture compound statement: err=0
  printf("acc parallel loop with acc atomic capture compound statement: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = 0;
    bool arr[8];
    memset(arr, 0, sizeof arr);
    #pragma acc parallel loop num_gangs(2) num_workers(2) vector_length(2) \
        gang worker vector copy(x, arr)
    for (int i = 0; i < 8; ++i) {
      int v;
      #pragma acc atomic capture
      {
        v = x;
        x++;
      }
      arr[v] = true;
    }
    for (int i = 0; i < 8; ++i)
      if (!arr[i])
        err = true;
    if (x != 8)
      err = true;
  }
  printf("err=%d\n", err);

  // DMP-LABEL: "acc parallel loop with acc atomic compare: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCompareClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCompareClause
  //  DMP-NEXT:     IfStmt
  //
  //   PRT-LABEL: printf("acc parallel loop with acc atomic compare: ");
  //         PRT: for (int i = {{.*}}) {
  //  PRT-A-NEXT:   #pragma acc atomic compare
  // PRT-AO-NEXT:   // #pragma omp atomic compare
  //  PRT-O-NEXT:   #pragma omp atomic compare
  // PRT-OA-NEXT:   // #pragma acc atomic compare
  //    PRT-NEXT:   if (i < x) {
  //    PRT-NEXT:     x = i;
  //    PRT-NEXT:   }
  //    PRT-NEXT: }
  //
  // EXE-NEXT: acc parallel loop with acc atomic compare: err=0
  printf("acc parallel loop with acc atomic compare: ");
  err = false;
  for (int rep = 0; rep < NUM_REPS; ++rep) {
    int x = INT_MAX;
    #pragma acc parallel loop num_gangs(2) num_workers(2) vector_length(2)     \
        gang worker vector copy(x)
    for (int i = 1; i <= 8; ++i) {
      #pragma acc atomic compare
      if (i < x) {
        x = i;
      }
    }
    if (x != 1)
      err = true;
  }
  printf("err=%d\n", err);

  //============================================================================
  // Check host-only compilation, where the atomic directive has no effect in
  // OpenACC.  For a few cases, try it within acc data to make sure that nesting
  // is permitted.
  //
  // It's questionable whether this use case actually needs to be supported.
  //============================================================================

  // DMP-LABEL: "host-only: acc atomic read: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCReadClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPReadClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("host-only: acc atomic read: ");
  //         PRT: v =
  //    PRT-NEXT: x =
  //  PRT-A-NEXT: #pragma acc atomic read
  // PRT-AO-NEXT: // #pragma omp atomic read
  //  PRT-O-NEXT: #pragma omp atomic read
  // PRT-OA-NEXT: // #pragma acc atomic read
  //    PRT-NEXT: v = x;
  //
  // EXE-NEXT: host-only: acc atomic read: v=99, x=99
  printf("host-only: acc atomic read: ");
  {
    int v, x;
    v = 88;
    x = 99;
    #pragma acc atomic read
    v = x;
    printf("v=%d, x=%d\n", v, x);
  }

  // DMP-LABEL: "host-only: acc atomic write: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCWriteClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPWriteClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("host-only: acc atomic write: ");
  //         PRT: {{^ *}}x =
  //  PRT-A-NEXT: #pragma acc atomic write
  // PRT-AO-NEXT: // #pragma omp atomic write
  //  PRT-O-NEXT: #pragma omp atomic write
  // PRT-OA-NEXT: // #pragma acc atomic write
  //    PRT-NEXT: x = 99;
  //
  // EXE-HOST-NEXT: host-only: acc atomic write: x=99, x=99
  //  EXE-OFF-NEXT: host-only: acc atomic write: x=99, x=77
  printf("host-only: acc atomic write: ");
  {
    int x = 77;
    #pragma acc data copy(x)
    {
      x = 88;
      #pragma acc atomic write
      x = 99;
      printf("x=%d, ", x);
    }
    printf("x=%d\n", x);
  }

  // DMP-LABEL: "host-only: acc atomic update: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     UnaryOperator {{.*}} '++'
  //
  //   PRT-LABEL: printf("host-only: acc atomic update: ");
  //         PRT: x =
  //  PRT-A-NEXT: #pragma acc atomic update
  // PRT-AO-NEXT: // #pragma omp atomic update
  //  PRT-O-NEXT: #pragma omp atomic update
  // PRT-OA-NEXT: // #pragma acc atomic update
  //    PRT-NEXT: x++;
  //
  // EXE-NEXT: host-only: acc atomic update: x=89
  printf("host-only: acc atomic update: ");
  {
    int x;
    x = 88;
    #pragma acc atomic update
    x++;
    printf("x=%d\n", x);
  }

  // DMP-LABEL: "host-only: acc atomic: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCUpdateClause {{.*}} <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPUpdateClause
  //  DMP-NEXT:     CompoundAssignOperator {{.*}} '-='
  //
  //   PRT-LABEL: printf("host-only: acc atomic: ");
  //         PRT: {{^ *}}x =
  //  PRT-A-NEXT: #pragma acc atomic
  // PRT-AO-NEXT: // #pragma omp atomic update
  //  PRT-O-NEXT: #pragma omp atomic update
  // PRT-OA-NEXT: // #pragma acc atomic
  //    PRT-NEXT: x -= 5;
  //
  // EXE-HOST-NEXT: host-only: acc atomic: x=83, x=83
  //  EXE-OFF-NEXT: host-only: acc atomic: x=83, x=77
  printf("host-only: acc atomic: ");
  {
    int x = 77;
    #pragma acc data copy(x)
    {
      x = 88;
      #pragma acc atomic
      x -= 5;
      printf("x=%d, ", x);
    }
    printf("x=%d\n", x);
  }

  // DMP-LABEL: "host-only: acc atomic capture: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     BinaryOperator {{.*}} '='
  //
  //   PRT-LABEL: printf("host-only: acc atomic capture: ");
  //         PRT: v =
  //    PRT-NEXT: x =
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: v = x++;
  //
  // EXE-NEXT: host-only: acc atomic capture: v=99, x=100
  printf("host-only: acc atomic capture: ");
  {
    int v, x;
    v = 88;
    x = 99;
    #pragma acc atomic capture
    v = x++;
    printf("v=%d, x=%d\n", v, x);
  }

  // DMP-LABEL: "host-only: acc atomic capture compound statement: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCaptureClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCaptureClause
  //  DMP-NEXT:     CompoundStmt
  //
  //   PRT-LABEL: printf("host-only: acc atomic capture compound statement: ");
  //         PRT: {{^ *}}v =
  //    PRT-NEXT: {{^ *}}x =
  //  PRT-A-NEXT: #pragma acc atomic capture
  // PRT-AO-NEXT: // #pragma omp atomic capture
  //  PRT-O-NEXT: #pragma omp atomic capture
  // PRT-OA-NEXT: // #pragma acc atomic capture
  //    PRT-NEXT: {
  //    PRT-NEXT:   v = x;
  //    PRT-NEXT:   x += 3;
  //    PRT-NEXT: }
  //
  // EXE-HOST-NEXT: host-only: acc atomic capture compound statement: v=99, x=102; v=99, x=102
  //  EXE-OFF-NEXT: host-only: acc atomic capture compound statement: v=99, x=102; v=66, x=77
  printf("host-only: acc atomic capture compound statement: ");
  {
    int v = 66, x = 77;
    #pragma acc data copy(v, x)
    {
      v = 88;
      x = 99;
      #pragma acc atomic capture
      {
        v = x;
        x += 3;
      }
      printf("v=%d, x=%d; ", v, x);
    }
    printf("v=%d, x=%d\n", v, x);
  }

  // DMP-LABEL: "host-only: acc atomic compare: "
  //       DMP: ACCAtomicDirective
  //  DMP-NEXT:   ACCCompareClause
  //   DMP-NOT:     <implicit>
  //  DMP-NEXT:   impl: OMPAtomicDirective
  //  DMP-NEXT:     OMPCompareClause
  //  DMP-NEXT:     IfStmt
  //
  //   PRT-LABEL: printf("host-only: acc atomic compare: ");
  //         PRT: for (int i = {{.*}}) {
  //  PRT-A-NEXT:   #pragma acc atomic compare
  // PRT-AO-NEXT:   // #pragma omp atomic compare
  //  PRT-O-NEXT:   #pragma omp atomic compare
  // PRT-OA-NEXT:   // #pragma acc atomic compare
  //    PRT-NEXT:   if (i > x) {
  //    PRT-NEXT:     x = i;
  //    PRT-NEXT:   }
  //    PRT-NEXT: }
  //
  // EXE-NEXT: host-only: acc atomic compare: x=8
  printf("host-only: acc atomic compare: ");
  err = false;
  int x = 0;
  for (int i = 1; i <= 8; ++i) {
    #pragma acc atomic compare
    if (i > x) {
      x = i;
    }
  }
  printf("x=%d\n", x);

  // EXE-NOT: {{.}}
  return 0;
}
