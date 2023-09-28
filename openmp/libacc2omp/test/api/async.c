// Check acc_get_default_async, acc_set_default_async, and acc2omp_async2dep.

// RUN: %clang-acc %s -o %t.exe

// DEFINE: %{run}( CASE %, NOT %, FC_ARGS %) = \
// DEFINE:   %{NOT} %t.exe %{CASE} > %t.out 2>&1 && \
// DEFINE:   FileCheck -match-full-lines -input-file %t.out \
// DEFINE:             -DACC_ASYNC_SYNC=-1 %s %{FC_ARGS}

//              CASE              NOT             FC_ARGS
// RUN: %{run}( good           %,              %,                                                 %)
// RUN: %{run}( set-fail       %, %not --crash %, -check-prefix=FAIL -DFUNC=acc_set_default_async %)
// RUN: %{run}( async2dep-fail %, %not --crash %, -check-prefix=FAIL -DFUNC=acc2omp_async2dep     %)

// END.

// expected-error 0 {{}}

#include <openacc.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

int main(int argc, const char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s CASE\n", argv[0]);
    return 1;
  }

  // FAIL-NOT: {{.}}
  // FAIL: OMP: Error #[[#]]: [[FUNC]] called with invalid async-arg -99
  // An abort messages usually follows any error.
  // FAIL-NOT: OMP:
  // FAIL-NOT: Libomptarget
  // FAIL-NOT: start
  if (0 == strcmp(argv[1], "set-fail"))
    acc_set_default_async(-99); // should terminate immediately
  if (0 == strcmp(argv[1], "async2dep-fail"))
    acc2omp_async2dep(-99); // should terminate immediately

  if (0 != strcmp(argv[1], "good")) {
    fprintf(stderr, "invalid CASE\n");
    return 1;
  }

  // CHECK-NOT: {{.}}
  // CHECK: start
  printf("start\n");

  //----------------------------------------------------------------------------
  // Call acc_get_default_async to check the initial default activity queue and
  // to check the effects of different arguments to acc_set_default_async.
  //----------------------------------------------------------------------------

  // CHECK-NEXT: initial default: 0
  // CHECK-NEXT: acc_set_default_async(acc_async_noval) has no effect: 0
  printf("initial default: %d\n", acc_get_default_async());
  acc_set_default_async(acc_async_noval);
  printf("acc_set_default_async(acc_async_noval) has no effect: %d\n",
         acc_get_default_async());

  // CHECK-NEXT: acc_set_default_async(acc_async_sync): [[ACC_ASYNC_SYNC]]
  // CHECK-NEXT: acc_set_default_async(acc_async_noval) has no effect: [[ACC_ASYNC_SYNC]]
  acc_set_default_async(acc_async_sync);
  printf("acc_set_default_async(acc_async_sync): %d\n",
         acc_get_default_async());
  acc_set_default_async(acc_async_noval);
  printf("acc_set_default_async(acc_async_noval) has no effect: %d\n",
         acc_get_default_async());

  // CHECK-NEXT: acc_set_default_async(0): 0
  // CHECK-NEXT: acc_set_default_async(acc_async_noval) has no effect: 0
  acc_set_default_async(0);
  printf("acc_set_default_async(0): %d\n", acc_get_default_async());
  acc_set_default_async(acc_async_noval);
  printf("acc_set_default_async(acc_async_noval) has no effect: %d\n",
         acc_get_default_async());

  // CHECK-NEXT: acc_set_default_async(1): 1
  // CHECK-NEXT: acc_set_default_async(acc_async_noval) has no effect: 1
  acc_set_default_async(1);
  printf("acc_set_default_async(1): %d\n", acc_get_default_async());
  acc_set_default_async(acc_async_noval);
  printf("acc_set_default_async(acc_async_noval) has no effect: %d\n",
         acc_get_default_async());

  // CHECK-NEXT: acc_set_default_async(>1): 17
  // CHECK-NEXT: acc_set_default_async(acc_async_noval) has no effect: 17
  acc_set_default_async(17);
  printf("acc_set_default_async(>1): %d\n", acc_get_default_async());
  acc_set_default_async(acc_async_noval);
  printf("acc_set_default_async(acc_async_noval) has no effect: %d\n",
         acc_get_default_async());

  // CHECK-NEXT: acc_set_default_async(29): 29
  // CHECK-NEXT: acc_set_default_async(acc_async_noval) has no effect: 29
  acc_set_default_async(29);
  printf("acc_set_default_async(29): %d\n", acc_get_default_async());
  acc_set_default_async(acc_async_noval);
  printf("acc_set_default_async(acc_async_noval) has no effect: %d\n",
         acc_get_default_async());

  // CHECK-NEXT: acc_set_default_async(acc_async_default): 0
  // CHECK-NEXT: acc_set_default_async(acc_async_noval) has no effect: 0
  acc_set_default_async(acc_async_default);
  printf("acc_set_default_async(acc_async_default): %d\n", acc_get_default_async());
  acc_set_default_async(acc_async_noval);
  printf("acc_set_default_async(acc_async_noval) has no effect: %d\n",
         acc_get_default_async());

  //----------------------------------------------------------------------------
  // Check that acc2omp_async2dep maps each activity queue (whether synchronous
  // or asynchronous) to a unique depends value.
  //----------------------------------------------------------------------------

  // CHECK-NEXT: sync has a unique depends value: [[#%#x, DEP_SYNC:]]
  // CHECK-NEXT: async0 has a unique depends value: [[#%#x, DEP_ASYNC0:]]
  // CHECK-NEXT: async1 has a unique depends value: [[#%#x, DEP_ASYNC1:]]
  // CHECK-NEXT: async12 has a unique depends value: [[#%#x, DEP_ASYNC12:]]
  // CHECK-NEXT: async29 has a unique depends value: [[#%#x, DEP_ASYNC29:]]
  char *depSync = acc2omp_async2dep(acc_async_sync);
  char *depAsync0 = acc2omp_async2dep(0);
  char *depAsync1 = acc2omp_async2dep(1);
  char *depAsync12 = acc2omp_async2dep(12);
  char *depAsync29 = acc2omp_async2dep(29);
  char *deps[] = { depSync, depAsync0, depAsync1, depAsync12, depAsync29 };
  char *depNames[] = {"sync", "async0", "async1", "async12", "async29"};
  for (int i = 0; i < sizeof deps / sizeof *deps; ++i) {
    bool unique = true;
    for (int j = i + 1; j < sizeof deps / sizeof *deps; ++j) {
      if (deps[i] == deps[j]) {
        printf("%s and %s have identical depends values: %p\n",
               depNames[i], depNames[j], deps[i]);
        unique = false;
      }
    }
    if (unique)
      printf("%s has a unique depends value: %p\n", depNames[i], deps[i]);
  }

  //----------------------------------------------------------------------------
  // Check acc2omp_async2dep's handling of acc_async_noval.
  //----------------------------------------------------------------------------

  // CHECK-NEXT: acc2omp_async2dep(acc_async_noval) when acc_async_sync: [[#DEP_SYNC]]
  // CHECK-NEXT: acc2omp_async2dep(acc_async_noval) when 0: [[#DEP_ASYNC0]]
  // CHECK-NEXT: acc2omp_async2dep(acc_async_noval) when 29: [[#DEP_ASYNC29]]
  acc_set_default_async(acc_async_sync);
  printf("acc2omp_async2dep(acc_async_noval) when acc_async_sync: %p\n",
         acc2omp_async2dep(acc_async_noval));
  acc_set_default_async(0);
  printf("acc2omp_async2dep(acc_async_noval) when 0: %p\n",
         acc2omp_async2dep(acc_async_noval));
  acc_set_default_async(29);
  printf("acc2omp_async2dep(acc_async_noval) when 29: %p\n",
         acc2omp_async2dep(acc_async_noval));

  //----------------------------------------------------------------------------
  // Check acc2omp_async2dep's handling of acc_async_default.
  //----------------------------------------------------------------------------

  // CHECK-NEXT: acc2omp_async2dep(acc_async_default) when current default == initial default: [[#DEP_ASYNC0]]
  // CHECK-NEXT: acc2omp_async2dep(acc_async_default) when current default != initial default: [[#DEP_ASYNC0]]
  acc_set_default_async(acc_async_default);
  printf("acc2omp_async2dep(acc_async_default) when current default == initial "
         "default: %p\n",
         acc2omp_async2dep(acc_async_default));
  acc_set_default_async(acc_get_default_async() + 1);
  printf("acc2omp_async2dep(acc_async_default) when current default != initial "
         "default: %p\n",
         acc2omp_async2dep(acc_async_default));

  // CHECK-NOT: {{.}}
  return 0;
}
