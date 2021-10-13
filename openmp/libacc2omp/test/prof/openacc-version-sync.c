// Check that the OpenACC Profiling Interface support reports the same OpenACC
// version as Clang.
//
// Various other tests check that each of Clang and the OpenACC Profiling
// Interface support reports the same OpenACC version (LIT substitution
// %acc-version) across offload configurations.  This test makes sure the two
// report the same OpenACC version as each other.

// RUN: %clang-acc -o %t.exe %s
// RUN: %t.exe 2>&1 | FileCheck -DACC_VERSION=%acc-version %s
//
// END.

// expected-error 0 {{}}

// FIXME: Clang produces spurious warning diagnostics for nvptx64 offload.  This
// issue is not limited to Clacc and is present upstream:
// nvptx64-warning@*:* 0+ {{Linking two modules of different data layouts}}
// nvptx64-warning@*:* 0+ {{Linking two modules of different target triples}}

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
