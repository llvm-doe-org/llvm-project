// Check that acc_ev_device_init_start and acc_ev_device_init_end callbacks
// specify the right OpenACC Runtime Library API routine.
//
// RUN: %data cases {
// RUN:   (case=acc_malloc args='size'     )
// RUN:   (case=acc_copyin args='arr, size')
// RUN:   (case=acc_create args='arr, size')
// RUN: }
// RUN: echo '#define FOREACH_CASE(Macro) \' > %t-cases.h
// RUN: %for cases {
// RUN:   echo '  Macro(%[case], %[args]) \' >> %t-cases.h
// RUN: }
// RUN: echo '  /*end of FOREACH_CASE*/' >> %t-cases.h
// RUN: %clang-acc -DCASES_HEADER='"%t-cases.h"' -o %t.exe %s
// RUN: %for cases {
// RUN:   %t.exe %[case] > %t.out 2> %t.err
// RUN:   FileCheck -input-file %t.err -allow-empty -check-prefixes=ERR %s
// RUN:   FileCheck -input-file %t.out %s \
// RUN:     -check-prefixes=OUT,OUT-%if-host(host,OFF) \
// RUN:     -match-full-lines -strict-whitespace \
// RUN:     -DACC_DEVICE=acc_device_%dev-type-0-acc -DVERSION=%acc-version \
// RUN:     -DHOST_DEV=%acc-host-dev -DOFF_DEV=0 -DTHREAD_ID=0 \
// RUN:     -DASYNC_QUEUE=-1 -DROUTINE=%[case]
// RUN: }
//
// END.

// expected-error 0 {{}}

// FIXME: Clang produces spurious warning diagnostics for nvptx64 offload.  This
// issue is not limited to Clacc and is present upstream:
// nvptx64-warning@*:* 0+ {{Linking two modules of different target triples}}

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

// ERR-NOT:{{.}}

int main(int argc, char *argv[]) {
  // OUT:start
  printf("start\n");

  //      OUT-OFF:acc_ev_device_init_start
  // OUT-OFF-NEXT:  acc_prof_info
  // OUT-OFF-NEXT:    event_type=1, valid_bytes=72, version=[[VERSION]],
  // OUT-OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OUT-OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OUT-OFF-NEXT:    src_file=(null), func_name=[[ROUTINE]],
  // OUT-OFF-NEXT:    line_no=0, end_line_no=0,
  // OUT-OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OUT-OFF-NEXT:  acc_other_event_info
  // OUT-OFF-NEXT:    event_type=1, valid_bytes=24,
  // OUT-OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OUT-OFF-NEXT:    implicit=0, tool_info=(nil)
  // OUT-OFF-NEXT:  acc_api_info
  // OUT-OFF-NEXT:    device_api=0, valid_bytes=12,
  // OUT-OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OUT-OFF-NEXT:acc_ev_device_init_end
  // OUT-OFF-NEXT:  acc_prof_info
  // OUT-OFF-NEXT:    event_type=2, valid_bytes=72, version=[[VERSION]],
  // OUT-OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OUT-OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OUT-OFF-NEXT:    src_file=(null), func_name=[[ROUTINE]],
  // OUT-OFF-NEXT:    line_no=0, end_line_no=0,
  // OUT-OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OUT-OFF-NEXT:  acc_other_event_info
  // OUT-OFF-NEXT:    event_type=2, valid_bytes=24,
  // OUT-OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OUT-OFF-NEXT:    implicit=0, tool_info=(nil)
  // OUT-OFF-NEXT:  acc_api_info
  // OUT-OFF-NEXT:    device_api=0, valid_bytes=12,
  // OUT-OFF-NEXT:    device_type=[[ACC_DEVICE]]
  int arr[5];
  int size = sizeof arr;
  if (argc != 2) {
    fprintf(stderr, "expected one argument\n");
    return 1;
  }
#define AddCase(Fn, ...)                                                       \
  else if (!strcmp(argv[1], #Fn))                                              \
    Fn(__VA_ARGS__);
  FOREACH_CASE(AddCase)
#undef AddCase
  else {
    fprintf(stderr, "unexpected case: %s\n", argv[1]);
    return 1;
  }
  return 0;
}
