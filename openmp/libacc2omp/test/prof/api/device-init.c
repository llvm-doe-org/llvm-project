// Check that acc_ev_device_init_start and acc_ev_device_init_end callbacks
// specify the right OpenACC Runtime Library API routine.

// Redefine this to specify how %{check-cases} expands for each case.
//
// - CASE = the case name used in the enum and as a command line argument.
// - ARGS = the arguments to pass to the function identified by CASE.
//
// DEFINE: %{check-case}( CASE %, ARGS %) =

// Substitution to run %{check-case} for each case.
//
// If a case is listed here but is not covered in the code, that case will fail.
// If a case is covered in the code but not listed here, the code will not
// compile because this list produces the enum used by the code.
//
// DEFINE: %{check-cases} =                                                    \
// DEFINE:   %{check-case}( acc_malloc %, size      %)                      && \
// DEFINE:   %{check-case}( acc_copyin %, arr, size %)                      && \
// DEFINE:   %{check-case}( acc_create %, arr, size %)

// Generate the enum of cases.
//
//      RUN: echo '#define FOREACH_CASE(Macro) \' > %t-cases.h
// REDEFINE: %{check-case}( CASE %, ARGS %) =                                  \
// REDEFINE: echo '  Macro(%{CASE}, %{ARGS}) \' >> %t-cases.h
//      RUN: %{check-cases}
//      RUN: echo '  /*end of FOREACH_CASE*/' >> %t-cases.h

// Try all cases.
//
// REDEFINE: %{check-case}( CASE %, ARGS %) =                                  \
// REDEFINE:   : '---------- CASE: %{CASE} ----------' &&                      \
// REDEFINE:   %t.exe %{CASE} > %t.out 2> %t.err &&                            \
// REDEFINE:   FileCheck -input-file %t.err -allow-empty -check-prefixes=ERR   \
// REDEFINE:              %s &&                                                \
// REDEFINE:   FileCheck -input-file %t.out %s                                 \
// REDEFINE:     -check-prefixes=OUT,OUT-%if-host<host|OFF>                    \
// REDEFINE:     -match-full-lines -strict-whitespace                          \
// REDEFINE:     -DACC_DEVICE=acc_device_%dev-type-0-acc                       \
// REDEFINE:     -DVERSION=%acc-version -DHOST_DEV=%acc-host-dev -DOFF_DEV=0   \
// REDEFINE:     -DTHREAD_ID=0 -DASYNC_QUEUE=-1 -DROUTINE=%{CASE}
//
// RUN: %clang-acc -DCASES_HEADER='"%t-cases.h"' -o %t.exe %s
// RUN: %{check-cases}

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
