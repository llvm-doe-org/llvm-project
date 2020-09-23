// RUN: %libomptarget-compile-aarch64-unknown-linux-gnu -fopenmp-version=51
// RUN: %libomptarget-run-aarch64-unknown-linux-gnu 2>&1 \
// RUN: | %fcheck-aarch64-unknown-linux-gnu -strict-whitespace

// RUN: %libomptarget-compile-powerpc64-ibm-linux-gnu -fopenmp-version=51
// RUN: %libomptarget-run-powerpc64-ibm-linux-gnu 2>&1 \
// RUN: | %fcheck-powerpc64-ibm-linux-gnu -strict-whitespace

// RUN: %libomptarget-compile-powerpc64le-ibm-linux-gnu -fopenmp-version=51
// RUN: %libomptarget-run-powerpc64le-ibm-linux-gnu 2>&1 \
// RUN: | %fcheck-powerpc64le-ibm-linux-gnu -strict-whitespace

// RUN: %libomptarget-compile-x86_64-pc-linux-gnu -fopenmp-version=51
// RUN: %libomptarget-run-x86_64-pc-linux-gnu 2>&1 \
// RUN: | %fcheck-x86_64-pc-linux-gnu -strict-whitespace

#include <omp.h>
#include <stdio.h>

#define CHECK_PRESENCE(Var1, Var2, Var3)                          \
  fprintf(stderr, "    presence of %s, %s, %s: %d, %d, %d\n",     \
          #Var1, #Var2, #Var3,                                    \
          omp_target_is_present(&Var1, omp_get_default_device()), \
          omp_target_is_present(&Var2, omp_get_default_device()), \
          omp_target_is_present(&Var3, omp_get_default_device()))

int main() {
  int m, r, d;
  // CHECK: presence of m, r, d: 0, 0, 0
  CHECK_PRESENCE(m, r, d);

  // -----------------------------------------------------------------------
  // CHECK-NEXT: check:{{.*}}
  fprintf(stderr, "check: dyn>0, hold=0, dec dyn=0\n");

  // CHECK-NEXT: once
  fprintf(stderr, "  once\n");
  #pragma omp target map(tofrom: m) map(alloc: r, d)
  ;
  // CHECK-NEXT: presence of m, r, d: 0, 0, 0
  CHECK_PRESENCE(m, r, d);

  // CHECK-NEXT: twice
  fprintf(stderr, "  twice\n");
  #pragma omp target data map(tofrom: m) map(alloc: r, d)
  {
    // CHECK-NEXT: presence of m, r, d: 1, 1, 1
    CHECK_PRESENCE(m, r, d);
    #pragma omp target map(tofrom: m) map(alloc: r, d)
    ;
    // CHECK-NEXT: presence of m, r, d: 1, 1, 1
    CHECK_PRESENCE(m, r, d);
  }
  // CHECK-NEXT: presence of m, r, d: 0, 0, 0
  CHECK_PRESENCE(m, r, d);

  // -----------------------------------------------------------------------
  // CHECK: check:{{.*}}
  fprintf(stderr, "check: dyn=0, hold>0, dec hold=0\n");

  // CHECK-NEXT: once
  fprintf(stderr, "  once\n");
  #pragma omp target map(hold, tofrom: m) map(hold, alloc: r, d)
  ;
  // CHECK-NEXT: presence of m, r, d: 0, 0, 0
  CHECK_PRESENCE(m, r, d);

  // CHECK-NEXT: twice
  fprintf(stderr, "  twice\n");
  #pragma omp target data map(hold, tofrom: m) map(hold, alloc: r, d)
  {
    // CHECK-NEXT: presence of m, r, d: 1, 1, 1
    CHECK_PRESENCE(m, r, d);
    #pragma omp target map(hold, tofrom: m) map(hold, alloc: r, d)
    ;
    // CHECK-NEXT: presence of m, r, d: 1, 1, 1
    CHECK_PRESENCE(m, r, d);
  }
  // CHECK-NEXT: presence of m, r, d: 0, 0, 0
  CHECK_PRESENCE(m, r, d);

  // -----------------------------------------------------------------------
  // CHECK: check:{{.*}}
  fprintf(stderr, "check: dyn>0, hold>0, dec dyn=0, dec hold=0\n");

  // CHECK-NEXT: once each
  fprintf(stderr, "  once each\n");
  #pragma omp target data map(hold, tofrom: m) map(hold, alloc: r, d)
  {
    // CHECK-NEXT: presence of m, r, d: 1, 1, 1
    CHECK_PRESENCE(m, r, d);
    #pragma omp target map(tofrom: m) map(alloc: r, d)
    ;
    // CHECK-NEXT: presence of m, r, d: 1, 1, 1
    CHECK_PRESENCE(m, r, d);
  }
  // CHECK-NEXT: presence of m, r, d: 0, 0, 0
  CHECK_PRESENCE(m, r, d);

  // CHECK-NEXT: twice each
  fprintf(stderr, "  twice each\n");
  #pragma omp target data map(hold, tofrom: m) map(hold, alloc: r, d)
  {
    // CHECK-NEXT: presence of m, r, d: 1, 1, 1
    CHECK_PRESENCE(m, r, d);
    #pragma omp target data map(hold, tofrom: m) map(hold, alloc: r, d)
    {
      // CHECK-NEXT: presence of m, r, d: 1, 1, 1
      CHECK_PRESENCE(m, r, d);
      #pragma omp target data map(tofrom: m) map(alloc: r, d)
      {
        // CHECK-NEXT: presence of m, r, d: 1, 1, 1
        CHECK_PRESENCE(m, r, d);
        #pragma omp target map(tofrom: m) map(alloc: r, d)
        ;
        // CHECK-NEXT: presence of m, r, d: 1, 1, 1
        CHECK_PRESENCE(m, r, d);
      }
      // CHECK-NEXT: presence of m, r, d: 1, 1, 1
      CHECK_PRESENCE(m, r, d);
    }
    // CHECK-NEXT: presence of m, r, d: 1, 1, 1
    CHECK_PRESENCE(m, r, d);
  }
  // CHECK-NEXT: presence of m, r, d: 0, 0, 0
  CHECK_PRESENCE(m, r, d);

  // -----------------------------------------------------------------------
  // CHECK: check:{{.*}}
  fprintf(stderr, "check: dyn>0, hold>0, dec hold=0, dec dyn=0\n");

  // CHECK-NEXT: once each
  fprintf(stderr, "  once each\n");
  #pragma omp target data map(tofrom: m) map(alloc: r, d)
  {
    // CHECK-NEXT: presence of m, r, d: 1, 1, 1
    CHECK_PRESENCE(m, r, d);
    #pragma omp target map(hold, tofrom: m) map(hold, alloc: r, d)
    ;
    // CHECK-NEXT: presence of m, r, d: 1, 1, 1
    CHECK_PRESENCE(m, r, d);
  }
  // CHECK-NEXT: presence of m, r, d: 0, 0, 0
  CHECK_PRESENCE(m, r, d);

  // CHECK-NEXT: twice each
  fprintf(stderr, "  twice each\n");
  #pragma omp target data map(tofrom: m) map(alloc: r, d)
  {
    // CHECK-NEXT: presence of m, r, d: 1, 1, 1
    CHECK_PRESENCE(m, r, d);
    #pragma omp target data map(tofrom: m) map(alloc: r, d)
    {
      // CHECK-NEXT: presence of m, r, d: 1, 1, 1
      CHECK_PRESENCE(m, r, d);
      #pragma omp target data map(hold, tofrom: m) map(hold, alloc: r, d)
      {
        // CHECK-NEXT: presence of m, r, d: 1, 1, 1
        CHECK_PRESENCE(m, r, d);
        #pragma omp target map(hold, tofrom: m) map(hold, alloc: r, d)
        ;
        // CHECK-NEXT: presence of m, r, d: 1, 1, 1
        CHECK_PRESENCE(m, r, d);
      }
      // CHECK-NEXT: presence of m, r, d: 1, 1, 1
      CHECK_PRESENCE(m, r, d);
    }
    // CHECK-NEXT: presence of m, r, d: 1, 1, 1
    CHECK_PRESENCE(m, r, d);
  }
  // CHECK-NEXT: presence of m, r, d: 0, 0, 0
  CHECK_PRESENCE(m, r, d);

  return 0;
}
