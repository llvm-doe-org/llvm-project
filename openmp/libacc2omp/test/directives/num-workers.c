// Check that num_workers controls the number of workers as desired.
//
// The point here isn't to check what the OpenMP translation is, which is
// checked in many Clang OpenACC tests.  The point is to check that, regardless
// of what the OpenMP translation is, it actually produces the desired effect.
//
// FIXME: Without -O1, num_workers(NUM_WORKERS) is sometimes treated as
// num_workers(1) on at least nvptx64.
//
// DEFINE: %{check}( NUM_GANGS %, NUM_WORKERS %) =                             \
// DEFINE:   %clang-acc %s -o %t.exe -DTGT_USE_STDIO=%if-tgt-stdio<1|0>        \
// DEFINE:     -DNUM_GANGS=%{NUM_GANGS} -DNUM_WORKERS=%{NUM_WORKERS} -O1 &&    \
// DEFINE:   %t.exe > %t.out 2>&1 &&                                           \
// DEFINE:   FileCheck %s -input-file %t.out -check-prefixes=EXE               \
// DEFINE:                -strict-whitespace -match-full-lines                 \
// DEFINE:                -implicit-check-not='{{.}}'
//
// NUM_GANGS and NUM_WORKERS are the number of gangs and workers requested.
// There are some restrictions:
// - The product must not be higher than the total number of threads typically
//   available on the machines it might be tested on.  Otherwise, the values
//   won't be honored, and the test will fail.  For example, for x86_64 testing,
//   we consider that older laptops might have as few as 8 threads, so the test
//   will always fail there if the product is 9.
// - For the given num_gangs (e.g., 3), if num_workers is the value that would
//   be picked anyway for the given hardware (e.g., 2 when there are only 8
//   hardware threads), then the test will pass even if the num_workers clause
//   has no effect, defeating the purpose of the test.
// - NUM_GANGS and NUM_WORKERS must be less than NUM_GANGS_MAX and
//   NUM_WORKERS_MAX (below) so we can record any additional active gangs or
//   workers.  Otherwise, the test will fail.
// - The product must be less than the NITERS (below) or we might not observe
//   some active gangs or workers.
//
//                                          NUM_GANGS    NUM_WORKERS
// RUN:                            %{check}(4         %, 1           %)
// RUN:                            %{check}(2         %, 3           %)
// RUN: %if nvptx64-nvidia-cuda %{ %{check}(4         %, 32          %) %}
// RUN: %if amdgcn-amd-amdhsa   %{ %{check}(4         %, 32          %) %}
//
// END.

/* expected-no-diagnostics */

#include <stdio.h>

#if TGT_USE_STDIO
# define TGT_PRINTF(...) printf(__VA_ARGS__)
#else
# define TGT_PRINTF(...)
#endif

#include <stdbool.h>
#include <stdio.h>

// The maximum number of gangs and workers we can record.
#define NUM_GANGS_MAX 64
#define NUM_WORKERS_MAX 64

// How many iterations to use in loops.  If NITERS is too low, some of the
// actual threads won't be observed.  If NITERS * NITERS_OUTER is too high, the
// test will take a long time.
#define NITERS 256
#define NITERS_OUTER 8

typedef struct {
  bool workers[NUM_GANGS_MAX][NUM_WORKERS_MAX];
  bool err;
} Record;

static void initRecord(Record *record) {
  for (int gang = 0; gang < NUM_GANGS_MAX; ++gang)
    for (int worker = 0; worker < NUM_WORKERS_MAX; ++worker)
      record->workers[gang][worker] = false;
  record->err = false;
}

#pragma acc routine seq
static void saveRecord(Record *record) {
  int omp_get_team_num();
  int omp_get_thread_num();
  int gang = omp_get_team_num();
  if (gang >= NUM_GANGS_MAX) {
    TGT_PRINTF("  error: insufficient alloc: gang=%d >= NUM_GANGS_MAX=%d\n",
               gang, NUM_GANGS_MAX);
    record->err = true;
    return;
  }
  int worker = omp_get_thread_num();
  if (worker >= NUM_WORKERS_MAX) {
    TGT_PRINTF("  error: insufficient alloc: worker=%d >= NUM_WORKERS_MAX=%d\n",
               worker, NUM_WORKERS_MAX);
    record->err = true;
    return;
  }
  record->workers[gang][worker] = true;
  return;
}

static bool checkRecord(Record *record) {
  for (int gang = 0; gang < NUM_GANGS_MAX; ++gang) {
    int workerMax = -1;
    for (int worker = 0; worker < NUM_WORKERS_MAX; ++worker) {
      if (record->workers[gang][worker]) {
        if (worker > 0 && workerMax != worker - 1) {
          printf("  error: worker discontinuity: workers[%d][%d]=true but "
                 "workers[%d][%d]=false\n",
                 gang, worker, gang, worker - 1);
          record->err = true;
        }
        workerMax = worker;
      }
    }
    int workerCount = workerMax + 1;
    int expected = gang < NUM_GANGS ? NUM_WORKERS : 0;
    if (workerCount != expected) {
      printf("  error: for gang=%d, workerCount=%d, but expected %d\n",
             gang, workerCount, expected);
      record->err = true;
    }
  }
  if (!record->err)
    printf("  success\n");
  return record->err;
}

#pragma acc routine gang
static void gangFn(Record *record) {
  #pragma acc loop gang worker
  for (int i = 0; i < NITERS; ++i)
    saveRecord(record);
}

#pragma acc routine worker
static void workerFn(Record *record) {
  #pragma acc loop worker
  for (int i = 0; i < NITERS; ++i)
    saveRecord(record);
}

int main() {
  // EXE:start
  printf("start\n");
  Record record;
  bool err = false;

  // EXE-NEXT:separate:
  // EXE-NEXT:  success
  printf("separate:\n");
  initRecord(&record);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS)
  #pragma acc loop gang worker
  for (int i = 0; i < NITERS; ++i)
    saveRecord(&record);
  err |= checkRecord(&record);

  // EXE-NEXT:combined:
  // EXE-NEXT:  success
  printf("combined:\n");
  initRecord(&record);
  #pragma acc parallel loop num_gangs(NUM_GANGS) num_workers(NUM_WORKERS) \
                       gang worker
  for (int i = 0; i < NITERS; ++i)
    saveRecord(&record);
  err |= checkRecord(&record);

  // EXE-NEXT:gangFn:
  // EXE-NEXT:  success
  printf("gangFn:\n");
  initRecord(&record);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS)
  gangFn(&record);
  err |= checkRecord(&record);

  // EXE-NEXT:workerFn, gang-partitioned:
  // EXE-NEXT:  success
  printf("workerFn, gang-partitioned:\n");
  initRecord(&record);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS)
  #pragma acc loop gang
  for (int i = 0; i < NITERS_OUTER; ++i)
    workerFn(&record);
  err |= checkRecord(&record);

  // EXE-NEXT:workerFn, gang-redundant:
  // EXE-NEXT:  success
  printf("workerFn, gang-redundant:\n");
  initRecord(&record);
  #pragma acc parallel num_gangs(NUM_GANGS) num_workers(NUM_WORKERS)
  workerFn(&record);
  err |= checkRecord(&record);

  return err;
}
