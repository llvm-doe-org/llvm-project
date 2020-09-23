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
  fprintf(stderr, "check: dyn>0, hold=0, dec/reset dyn=0\n");

  // CHECK-NEXT: structured{{.*}}
  fprintf(stderr, "  structured dec of dyn\n");
  #pragma omp target data map(tofrom: m) map(alloc: r, d)
  {
    // CHECK-NEXT: presence of m, r, d: 1, 1, 1
    CHECK_PRESENCE(m, r, d);
    #pragma omp target data map(tofrom: m) map(alloc: r, d)
    {
      // CHECK-NEXT: presence of m, r, d: 1, 1, 1
      CHECK_PRESENCE(m, r, d);
    }
    // CHECK-NEXT: presence of m, r, d: 1, 1, 1
    CHECK_PRESENCE(m, r, d);
  }
  // CHECK-NEXT: presence of m, r, d: 0, 0, 0
  CHECK_PRESENCE(m, r, d);

  // CHECK-NEXT: dynamic{{.*}}
  fprintf(stderr, "  dynamic dec/reset of dyn\n");
  #pragma omp target data map(tofrom: m) map(alloc: r, d)
  {
    // CHECK-NEXT: presence of m, r, d: 1, 1, 1
    CHECK_PRESENCE(m, r, d);
    #pragma omp target data map(tofrom: m) map(alloc: r, d)
    {
      // CHECK-NEXT: presence of m, r, d: 1, 1, 1
      CHECK_PRESENCE(m, r, d);
      #pragma omp target exit data map(from: m) map(release: r)
      // CHECK-NEXT: presence of m, r, d: 1, 1, 1
      CHECK_PRESENCE(m, r, d);
      #pragma omp target exit data map(from: m) map(release: r) map(delete: d)
      // CHECK-NEXT: presence of m, r, d: 0, 0, 0
      CHECK_PRESENCE(m, r, d);
    }
    // CHECK-NEXT: presence of m, r, d: 0, 0, 0
    CHECK_PRESENCE(m, r, d);
    // FIXME: 'delete' when not present should be fine, but currently it
    // produces a runtime error.
    #pragma omp target exit data map(from: m) map(release: r) //map(delete: d)
    // CHECK-NEXT: presence of m, r, d: 0, 0, 0
    CHECK_PRESENCE(m, r, d);
  }
  // CHECK-NEXT: presence of m, r, d: 0, 0, 0
  CHECK_PRESENCE(m, r, d);

  // -----------------------------------------------------------------------
  // CHECK: check:{{.*}}
  fprintf(stderr, "check: dyn=0, hold>0, dec/reset dyn=0, dec hold=0\n");

  // Structured dec of dyn would require dyn>0.

  // CHECK-NEXT: dynamic{{.*}}
  fprintf(stderr, "  dynamic dec/reset of dyn\n");
  #pragma omp target data map(hold, tofrom: m) map(hold, alloc: r, d)
  {
    // CHECK-NEXT: presence of m, r, d: 1, 1, 1
    CHECK_PRESENCE(m, r, d);
    #pragma omp target data map(hold, tofrom: m) map(hold, alloc: r, d)
    {
      // CHECK-NEXT: presence of m, r, d: 1, 1, 1
      CHECK_PRESENCE(m, r, d);
      #pragma omp target exit data map(from: m) map(release: r)
      // CHECK-NEXT: presence of m, r, d: 1, 1, 1
      CHECK_PRESENCE(m, r, d);
      #pragma omp target exit data map(from: m) map(release: r) map(delete: d)
      // CHECK-NEXT: presence of m, r, d: 1, 1, 1
      CHECK_PRESENCE(m, r, d);
    }
    // CHECK-NEXT: presence of m, r, d: 1, 1, 1
    CHECK_PRESENCE(m, r, d);
    #pragma omp target exit data map(from: m) map(release: r) map(delete: d)
    // CHECK-NEXT: presence of m, r, d: 1, 1, 1
    CHECK_PRESENCE(m, r, d);
  }
  // CHECK-NEXT: presence of m, r, d: 0, 0, 0
  CHECK_PRESENCE(m, r, d);

  // -----------------------------------------------------------------------
  // CHECK: check:{{.*}}
  fprintf(stderr, "check: dyn>0, hold>0, dec/reset dyn=0, dec hold=0\n");

  // CHECK-NEXT: structured{{.*}}
  fprintf(stderr, "  structured dec of dyn\n");
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
        #pragma omp target data map(tofrom: m) map(alloc: r, d)
        {
          // CHECK-NEXT: presence of m, r, d: 1, 1, 1
          CHECK_PRESENCE(m, r, d);
        }
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

  // CHECK-NEXT: dynamic{{.*}}
  fprintf(stderr, "  dynamic dec/reset of dyn\n");
  #pragma omp target enter data map(to: m) map(alloc: r, d)
  // CHECK-NEXT: presence of m, r, d: 1, 1, 1
  CHECK_PRESENCE(m, r, d);
  #pragma omp target enter data map(to: m) map(alloc: r, d)
  // CHECK-NEXT: presence of m, r, d: 1, 1, 1
  CHECK_PRESENCE(m, r, d);
  #pragma omp target data map(hold, tofrom: m) map(hold, alloc: r, d)
  {
    // CHECK-NEXT: presence of m, r, d: 1, 1, 1
    CHECK_PRESENCE(m, r, d);
    #pragma omp target data map(hold, tofrom: m) map(hold, alloc: r, d)
    {
      // CHECK-NEXT: presence of m, r, d: 1, 1, 1
      CHECK_PRESENCE(m, r, d);
      #pragma omp target exit data map(from: m) map(release: r)
      // CHECK-NEXT: presence of m, r, d: 1, 1, 1
      CHECK_PRESENCE(m, r, d);
      #pragma omp target exit data map(from: m) map(release: r) map(delete: d)
      // CHECK-NEXT: presence of m, r, d: 1, 1, 1
      CHECK_PRESENCE(m, r, d);
    }
    // CHECK-NEXT: presence of m, r, d: 1, 1, 1
    CHECK_PRESENCE(m, r, d);
    #pragma omp target exit data map(from: m) map(release: r) map(delete: d)
    // CHECK-NEXT: presence of m, r, d: 1, 1, 1
    CHECK_PRESENCE(m, r, d);
  }
  // CHECK-NEXT: presence of m, r, d: 0, 0, 0
  CHECK_PRESENCE(m, r, d);

  // -----------------------------------------------------------------------
  // CHECK: check:{{.*}}
  fprintf(stderr, "check: dyn>0, hold>0, dec hold=0, dec/reset dyn=0\n");

  // CHECK-NEXT: structured{{.*}}
  fprintf(stderr, "  structured dec of dyn\n");
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
        #pragma omp target data map(hold, tofrom: m) map(hold, alloc: r, d)
        {
          // CHECK-NEXT: presence of m, r, d: 1, 1, 1
          CHECK_PRESENCE(m, r, d);
        }
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

  // CHECK-NEXT: dynamic{{.*}}
  fprintf(stderr, "  dynamic dec/reset of dyn\n");
  #pragma omp target enter data map(to: m) map(alloc: r, d)
  // CHECK-NEXT: presence of m, r, d: 1, 1, 1
  CHECK_PRESENCE(m, r, d);
  #pragma omp target enter data map(to: m) map(alloc: r, d)
  // CHECK-NEXT: presence of m, r, d: 1, 1, 1
  CHECK_PRESENCE(m, r, d);
  #pragma omp target data map(hold, tofrom: m) map(hold, alloc: r, d)
  {
    // CHECK-NEXT: presence of m, r, d: 1, 1, 1
    CHECK_PRESENCE(m, r, d);
    #pragma omp target data map(hold, tofrom: m) map(hold, alloc: r, d)
    {
      // CHECK-NEXT: presence of m, r, d: 1, 1, 1
      CHECK_PRESENCE(m, r, d);
    }
    // CHECK-NEXT: presence of m, r, d: 1, 1, 1
    CHECK_PRESENCE(m, r, d);
  }
  // CHECK-NEXT: presence of m, r, d: 1, 1, 1
  CHECK_PRESENCE(m, r, d);
  #pragma omp target exit data map(from: m) map(release: r)
  // CHECK-NEXT: presence of m, r, d: 1, 1, 1
  CHECK_PRESENCE(m, r, d);
  #pragma omp target exit data map(from: m) map(release: r) map(delete: d)
  // CHECK-NEXT: presence of m, r, d: 0, 0, 0
  CHECK_PRESENCE(m, r, d);

  return 0;
}
