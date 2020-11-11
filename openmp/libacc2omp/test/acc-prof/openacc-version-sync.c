// Check that the OpenACC Profiling Interface support reports the same OpenACC
// version as Clang.
//
// Various other tests check that each of Clang and the OpenACC Profiling
// Interface support reports the same OpenACC version (LIT substitution
// %acc-version) across offload configurations.  This test connects the two.

// RUN: %clang -Xclang -verify -fopenacc %acc-includes -o %t %s
// RUN: %t 2>&1 | FileCheck -DACC_VERSION=%acc-version %s
//
// END.

// expected-no-diagnostics

#include <acc_prof.h>
#include <stdio.h>

//  CHECK-NOT: {{.}}
//      CHECK: _OPENACC=[[ACC_VERSION]]
// CHECK-NEXT: pi->version=[[ACC_VERSION]]
// CHECK-NEXT: equal
//  CHECK-NOT: {{.}}

static void cb(acc_prof_info *pi, acc_event_info *ei, acc_api_info *ai) {
  printf("_OPENACC=%d\n", _OPENACC);
  printf("pi->version=%d\n", pi->version);
  printf("%s\n", pi->version == _OPENACC ? "equal" : "not equal");
}

void acc_register_library(acc_prof_reg reg, acc_prof_reg unreg,
                          acc_prof_lookup lookup) {
  reg(acc_ev_compute_construct_start, cb, acc_reg);
}

int main() {
  #pragma acc parallel
  ;
  return 0;
}
