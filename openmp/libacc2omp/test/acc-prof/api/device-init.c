// Check that acc_ev_device_init_start and acc_ev_device_init_end callbacks
// specify the right OpenACC Runtime Library API routine.
//
// RUN: %data tgts {
// RUN:   (run-if=%run-if-x86_64  cflags=-fopenmp-targets=%run-x86_64-triple )
// RUN:   (run-if=%run-if-ppc64le cflags=-fopenmp-targets=%run-ppc64le-triple)
// RUN:   (run-if=%run-if-nvptx64 cflags=-fopenmp-targets=%run-nvptx64-triple)
// RUN: }
// RUN: %data cases {
// RUN:   (case=acc_malloc)
// RUN:   (case=acc_copyin)
// RUN:   (case=acc_create)
// RUN: }
// RUN: rm -f %t.h
// RUN: echo '#define FOREACH_CASE(Macro) \' > %t.h
// RUN: %for cases {
// RUN:   echo '  Macro(%[case]) \' >> %t.h
// RUN: }
// RUN: echo '  /*end of FOREACH_CASE*/' >> %t.h
// RUN: %for tgts {
// RUN:   %[run-if] %clang -Xclang -verify -fopenacc %acc-includes %[cflags] \
// RUN:                    -DCASES_HEADER='"%t.h"' -o %t.exe %s
// RUN:   %for cases {
// RUN:     %[run-if] %t.exe %[case] > %t.out 2> %t.err
// RUN:     %[run-if] FileCheck -input-file %t.err %s \
// RUN:         -allow-empty -check-prefixes=ERR
// RUN:     %[run-if] FileCheck -input-file %t.out %s \
// RUN:         -match-full-lines -strict-whitespace \
// RUN:         -DVERSION=%acc-version -DHOST_DEV=%acc-host-dev \
// RUN:         -DOFF_DEV=0 -DTHREAD_ID=0 -DASYNC_QUEUE=-1 \
// RUN:         -DROUTINE=%[case]
// RUN:   }
// RUN: }
//
// END.

// expected-no-diagnostics

#include "../callbacks.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

void acc_register_library(acc_prof_reg reg, acc_prof_reg unreg,
                          acc_prof_lookup lookup) {
  // It's important to register all callbacks so we don't miss events, such as
  // data movements, that shouldn't occur.
  register_all_callbacks(reg);
}

#include CASES_HEADER

enum Case {
#define AddCase(CaseName) \
  CASE_##CaseName,
FOREACH_CASE(AddCase)
#undef AddCase
  CASE_END
};

const char *CaseNames[] = {
#define AddCase(CaseName) \
  #CaseName,
FOREACH_CASE(AddCase)
#undef AddCase
};

// ERR-NOT:{{.}}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "expected one argument\n");
    return 1;
  }
  enum Case selectedCase;
  for (selectedCase = 0; selectedCase < CASE_END; ++selectedCase) {
    if (!strcmp(argv[1], CaseNames[selectedCase]))
      break;
  }
  if (selectedCase == CASE_END) {
    fprintf(stderr, "unexpected case: %s\n", argv[1]);
    return 1;
  }

  // CHECK-NOT:{{.}}
  //      CHECK:acc_ev_device_init_start
  // CHECK-NEXT:  acc_prof_info
  // CHECK-NEXT:    event_type=1, valid_bytes=72, version=[[VERSION]],
  // CHECK-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // CHECK-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // CHECK-NEXT:    src_file=(null), func_name=[[ROUTINE]],
  // CHECK-NEXT:    line_no=0, end_line_no=0,
  // CHECK-NEXT:    func_line_no=0, func_end_line_no=0
  // CHECK-NEXT:  acc_other_event_info
  // CHECK-NEXT:    event_type=1, valid_bytes=24,
  // CHECK-NEXT:    parent_construct=acc_construct_runtime_api,
  // CHECK-NEXT:    implicit=0, tool_info=(nil)
  // CHECK-NEXT:  acc_api_info
  // CHECK-NEXT:    device_api=0, valid_bytes=12,
  // CHECK-NEXT:    device_type=acc_device_not_host
  // CHECK-NEXT:acc_ev_device_init_end
  // CHECK-NEXT:  acc_prof_info
  // CHECK-NEXT:    event_type=2, valid_bytes=72, version=[[VERSION]],
  // CHECK-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // CHECK-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // CHECK-NEXT:    src_file=(null), func_name=[[ROUTINE]],
  // CHECK-NEXT:    line_no=0, end_line_no=0,
  // CHECK-NEXT:    func_line_no=0, func_end_line_no=0
  // CHECK-NEXT:  acc_other_event_info
  // CHECK-NEXT:    event_type=2, valid_bytes=24,
  // CHECK-NEXT:    parent_construct=acc_construct_runtime_api,
  // CHECK-NEXT:    implicit=0, tool_info=(nil)
  // CHECK-NEXT:  acc_api_info
  // CHECK-NEXT:    device_api=0, valid_bytes=12,
  // CHECK-NEXT:    device_type=acc_device_not_host
  int arr[5];
  int size = sizeof arr;
  switch (selectedCase) {
#define CASE(Fn, ...)                                                          \
  case CASE_##Fn:                                                              \
    Fn(__VA_ARGS__);                                                           \
    break;
  CASE(acc_malloc, size);
  CASE(acc_copyin, arr, size);
  CASE(acc_create, arr, size);
  case CASE_END:
    fprintf(stderr, "unexpected CASE_END\n");
    break;
  }
  return 0;
}
