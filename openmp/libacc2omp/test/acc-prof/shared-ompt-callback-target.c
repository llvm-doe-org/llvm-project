// Check registration/unregistration scenarios involving OpenACC events that
// share ompt_callback_target.

// RUN: %data events {
// RUN:   (event=NONE)
// RUN:   (event=ENTER_DATA_START)
// RUN:   (event=ENTER_DATA_END)
// RUN:   (event=EXIT_DATA_START)
// RUN:   (event=EXIT_DATA_END)
// RUN:   (event=COMPUTE_CONSTRUCT_START)
// RUN:   (event=COMPUTE_CONSTRUCT_END)
// RUN:   (event=ENQUEUE_LAUNCH_START)
// RUN:   (event=ENQUEUE_LAUNCH_END)
// RUN: }
//      # It's unlikely that no offloading or nvptx64 offloading covers any
//      # important logic here beyond what x86_64 or ppc64le offloading covers.
//      # However, nvptx64 offloading triples the test's run time for me, and no
//      # offloading has different output and doesn't exercise some callbacks,
//      # so just skip those cases.
// RUN: %data tgts {
// XUN:   (run-if=                tgt-cflags=                                    )
// RUN:   (run-if=%run-if-x86_64  tgt-cflags=-fopenmp-targets=%run-x86_64-triple )
// RUN:   (run-if=%run-if-ppc64le tgt-cflags=-fopenmp-targets=%run-ppc64le-triple)
// XUN:   (run-if=%run-if-nvptx64 tgt-cflags=-fopenmp-targets=%run-nvptx64-triple)
// RUN: }
// RUN: %for tgts {
// RUN:   %for events {
// RUN:     %[run-if] %clang -Xclang -verify -fopenacc %acc-includes %s -o %t \
// RUN:                      %[tgt-cflags] -DEVENT=EVENT_%[event]
// RUN:     %[run-if] %t > %t.out 2> %t.err
// RUN:     %[run-if] FileCheck -input-file %t.err %s \
// RUN:         -allow-empty -check-prefixes=ERR
// RUN:     %[run-if] FileCheck -input-file %t.out %s \
// RUN:         -match-full-lines -strict-whitespace \
// RUN:         -implicit-check-not=acc_ev_ -check-prefixes=%[event] \
// RUN:         -DVERSION=%acc-version -DOFF_DEV=0 -DTHREAD_ID=0 \
// RUN:         -DASYNC_QUEUE=-1 -DSRC_FILE=%s \
// RUN:         -DLINE_NO=20000 -DEND_LINE_NO=20002 \
// RUN:         -DFUNC_LINE_NO=10000 -DFUNC_END_LINE_NO=30000
// RUN:   }
// RUN: }
//
// END.

// expected-no-diagnostics

#include "callbacks.h"

enum {
  EVENT_NONE,
  EVENT_ENTER_DATA_START,
  EVENT_ENTER_DATA_END,
  EVENT_EXIT_DATA_START,
  EVENT_EXIT_DATA_END,
  EVENT_COMPUTE_CONSTRUCT_START,
  EVENT_COMPUTE_CONSTRUCT_END,
  EVENT_ENQUEUE_LAUNCH_START,
  EVENT_ENQUEUE_LAUNCH_END,
};

void acc_register_library(acc_prof_reg reg, acc_prof_reg unreg,
                          acc_prof_lookup lookup) {
  // These events share ompt_callback_target as an auxiliary callback except
  // that, for each of acc_ev_compute_construct_{start,end}, it's the
  // triggering OMPT callback.
  //
  // Check that registering callbacks for all of these events and then
  // unregistering callbacks for all but one of them doesn't cause the
  // remaining one to malfunction or fail to dispatch.  Check the data received
  // as the purpose of ompt_callback_target when it's an auxiliary callback is
  // to provide additional data.  Check that every event can be the only event
  // with a remaining callback in order to cover cases like the exceptions
  // mentioned above.  Also check the case of unregistering all callbacks.
  reg(acc_ev_enter_data_start, on_enter_data_start, acc_reg);
  reg(acc_ev_enter_data_end, on_enter_data_end, acc_reg);
  reg(acc_ev_exit_data_start, on_exit_data_start, acc_reg);
  reg(acc_ev_exit_data_end, on_exit_data_end, acc_reg);
  reg(acc_ev_compute_construct_start, on_compute_construct_start, acc_reg);
  reg(acc_ev_compute_construct_end, on_compute_construct_end, acc_reg);
  reg(acc_ev_enqueue_launch_start, on_enqueue_launch_start, acc_reg);
  reg(acc_ev_enqueue_launch_end, on_enqueue_launch_end, acc_reg);

  int event = EVENT;
  if (event != EVENT_ENTER_DATA_START)
    unreg(acc_ev_enter_data_start, on_enter_data_start, acc_reg);
  if (event != EVENT_ENTER_DATA_END)
    unreg(acc_ev_enter_data_end, on_enter_data_end, acc_reg);
  if (event != EVENT_EXIT_DATA_START)
    unreg(acc_ev_exit_data_start, on_exit_data_start, acc_reg);
  if (event != EVENT_EXIT_DATA_END)
    unreg(acc_ev_exit_data_end, on_exit_data_end, acc_reg);
  if (event != EVENT_COMPUTE_CONSTRUCT_START)
    unreg(acc_ev_compute_construct_start, on_compute_construct_start, acc_reg);
  if (event != EVENT_COMPUTE_CONSTRUCT_END)
    unreg(acc_ev_compute_construct_end, on_compute_construct_end, acc_reg);
  if (event != EVENT_ENQUEUE_LAUNCH_START)
    unreg(acc_ev_enqueue_launch_start, on_enqueue_launch_start, acc_reg);
  if (event != EVENT_ENQUEUE_LAUNCH_END)
    unreg(acc_ev_enqueue_launch_end, on_enqueue_launch_end, acc_reg);
}

#line 10000
int main() {
  int arr[2] = {10, 11};
  //      COMPUTE_CONSTRUCT_START:acc_ev_compute_construct_start
  // COMPUTE_CONSTRUCT_START-NEXT:  acc_prof_info
  // COMPUTE_CONSTRUCT_START-NEXT:    event_type=16, valid_bytes=72, version=[[VERSION]],
  // COMPUTE_CONSTRUCT_START-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // COMPUTE_CONSTRUCT_START-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // COMPUTE_CONSTRUCT_START-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // COMPUTE_CONSTRUCT_START-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // COMPUTE_CONSTRUCT_START-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // COMPUTE_CONSTRUCT_START-NEXT:  acc_other_event_info
  // COMPUTE_CONSTRUCT_START-NEXT:    event_type=16, valid_bytes=24,
  // COMPUTE_CONSTRUCT_START-NEXT:    parent_construct=acc_construct_parallel,
  // COMPUTE_CONSTRUCT_START-NEXT:    implicit=0, tool_info=(nil)
  // COMPUTE_CONSTRUCT_START-NEXT:  acc_api_info
  // COMPUTE_CONSTRUCT_START-NEXT:    device_api=0, valid_bytes=12,
  // COMPUTE_CONSTRUCT_START-NEXT:    device_type=acc_device_not_host
  //             ENTER_DATA_START:acc_ev_enter_data_start
  //        ENTER_DATA_START-NEXT:  acc_prof_info
  //        ENTER_DATA_START-NEXT:    event_type=10, valid_bytes=72, version=[[VERSION]],
  //        ENTER_DATA_START-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  //        ENTER_DATA_START-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //        ENTER_DATA_START-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //        ENTER_DATA_START-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  //        ENTER_DATA_START-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //        ENTER_DATA_START-NEXT:  acc_other_event_info
  //        ENTER_DATA_START-NEXT:    event_type=10, valid_bytes=24,
  //        ENTER_DATA_START-NEXT:    parent_construct=acc_construct_parallel,
  //        ENTER_DATA_START-NEXT:    implicit=0, tool_info=(nil)
  //        ENTER_DATA_START-NEXT:  acc_api_info
  //        ENTER_DATA_START-NEXT:    device_api=0, valid_bytes=12,
  //        ENTER_DATA_START-NEXT:    device_type=acc_device_not_host
  //               ENTER_DATA_END:acc_ev_enter_data_end
  //          ENTER_DATA_END-NEXT:  acc_prof_info
  //          ENTER_DATA_END-NEXT:    event_type=11, valid_bytes=72, version=[[VERSION]],
  //          ENTER_DATA_END-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  //          ENTER_DATA_END-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //          ENTER_DATA_END-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //          ENTER_DATA_END-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  //          ENTER_DATA_END-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //          ENTER_DATA_END-NEXT:  acc_other_event_info
  //          ENTER_DATA_END-NEXT:    event_type=11, valid_bytes=24,
  //          ENTER_DATA_END-NEXT:    parent_construct=acc_construct_parallel,
  //          ENTER_DATA_END-NEXT:    implicit=0, tool_info=(nil)
  //          ENTER_DATA_END-NEXT:  acc_api_info
  //          ENTER_DATA_END-NEXT:    device_api=0, valid_bytes=12,
  //          ENTER_DATA_END-NEXT:    device_type=acc_device_not_host
  //         ENQUEUE_LAUNCH_START:acc_ev_enqueue_launch_start
  //    ENQUEUE_LAUNCH_START-NEXT:  acc_prof_info
  //    ENQUEUE_LAUNCH_START-NEXT:    event_type=18, valid_bytes=72, version=[[VERSION]],
  //    ENQUEUE_LAUNCH_START-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  //    ENQUEUE_LAUNCH_START-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //    ENQUEUE_LAUNCH_START-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //    ENQUEUE_LAUNCH_START-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  //    ENQUEUE_LAUNCH_START-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //    ENQUEUE_LAUNCH_START-NEXT:  acc_launch_event_info
  //    ENQUEUE_LAUNCH_START-NEXT:    event_type=18, valid_bytes=32,
  //    ENQUEUE_LAUNCH_START-NEXT:    parent_construct=acc_construct_parallel,
  //    ENQUEUE_LAUNCH_START-NEXT:    implicit=0, tool_info=(nil),
  //    ENQUEUE_LAUNCH_START-NEXT:    kernel_name=(nil)
  //    ENQUEUE_LAUNCH_START-NEXT:  acc_api_info
  //    ENQUEUE_LAUNCH_START-NEXT:    device_api=0, valid_bytes=12,
  //    ENQUEUE_LAUNCH_START-NEXT:    device_type=acc_device_not_host
  //           ENQUEUE_LAUNCH_END:acc_ev_enqueue_launch_end
  //      ENQUEUE_LAUNCH_END-NEXT:  acc_prof_info
  //      ENQUEUE_LAUNCH_END-NEXT:    event_type=19, valid_bytes=72, version=[[VERSION]],
  //      ENQUEUE_LAUNCH_END-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  //      ENQUEUE_LAUNCH_END-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //      ENQUEUE_LAUNCH_END-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //      ENQUEUE_LAUNCH_END-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  //      ENQUEUE_LAUNCH_END-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //      ENQUEUE_LAUNCH_END-NEXT:  acc_launch_event_info
  //      ENQUEUE_LAUNCH_END-NEXT:    event_type=19, valid_bytes=32,
  //      ENQUEUE_LAUNCH_END-NEXT:    parent_construct=acc_construct_parallel,
  //      ENQUEUE_LAUNCH_END-NEXT:    implicit=0, tool_info=(nil),
  //      ENQUEUE_LAUNCH_END-NEXT:    kernel_name=(nil)
  //      ENQUEUE_LAUNCH_END-NEXT:  acc_api_info
  //      ENQUEUE_LAUNCH_END-NEXT:    device_api=0, valid_bytes=12,
  //      ENQUEUE_LAUNCH_END-NEXT:    device_type=acc_device_not_host
#line 20000
  #pragma acc parallel copy(arr) num_gangs(1)
  for (int j = 0; j < 2; ++j)
    printf("arr[%d]=%d\n", j, arr[j]);
  //            EXIT_DATA_START:acc_ev_exit_data_start
  //       EXIT_DATA_START-NEXT:  acc_prof_info
  //       EXIT_DATA_START-NEXT:    event_type=12, valid_bytes=72, version=[[VERSION]],
  //       EXIT_DATA_START-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  //       EXIT_DATA_START-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //       EXIT_DATA_START-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //       EXIT_DATA_START-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  //       EXIT_DATA_START-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //       EXIT_DATA_START-NEXT:  acc_other_event_info
  //       EXIT_DATA_START-NEXT:    event_type=12, valid_bytes=24,
  //       EXIT_DATA_START-NEXT:    parent_construct=acc_construct_parallel,
  //       EXIT_DATA_START-NEXT:    implicit=0, tool_info=(nil)
  //       EXIT_DATA_START-NEXT:  acc_api_info
  //       EXIT_DATA_START-NEXT:    device_api=0, valid_bytes=12,
  //       EXIT_DATA_START-NEXT:    device_type=acc_device_not_host
  //              EXIT_DATA_END:acc_ev_exit_data_end
  //         EXIT_DATA_END-NEXT:  acc_prof_info
  //         EXIT_DATA_END-NEXT:    event_type=13, valid_bytes=72, version=[[VERSION]],
  //         EXIT_DATA_END-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  //         EXIT_DATA_END-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //         EXIT_DATA_END-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //         EXIT_DATA_END-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  //         EXIT_DATA_END-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //         EXIT_DATA_END-NEXT:  acc_other_event_info
  //         EXIT_DATA_END-NEXT:    event_type=13, valid_bytes=24,
  //         EXIT_DATA_END-NEXT:    parent_construct=acc_construct_parallel,
  //         EXIT_DATA_END-NEXT:    implicit=0, tool_info=(nil)
  //         EXIT_DATA_END-NEXT:  acc_api_info
  //         EXIT_DATA_END-NEXT:    device_api=0, valid_bytes=12,
  //         EXIT_DATA_END-NEXT:    device_type=acc_device_not_host
  //      COMPUTE_CONSTRUCT_END:acc_ev_compute_construct_end
  // COMPUTE_CONSTRUCT_END-NEXT:  acc_prof_info
  // COMPUTE_CONSTRUCT_END-NEXT:    event_type=17, valid_bytes=72, version=[[VERSION]],
  // COMPUTE_CONSTRUCT_END-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // COMPUTE_CONSTRUCT_END-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // COMPUTE_CONSTRUCT_END-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // COMPUTE_CONSTRUCT_END-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // COMPUTE_CONSTRUCT_END-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // COMPUTE_CONSTRUCT_END-NEXT:  acc_other_event_info
  // COMPUTE_CONSTRUCT_END-NEXT:    event_type=17, valid_bytes=24,
  // COMPUTE_CONSTRUCT_END-NEXT:    parent_construct=acc_construct_parallel,
  // COMPUTE_CONSTRUCT_END-NEXT:    implicit=0, tool_info=(nil)
  // COMPUTE_CONSTRUCT_END-NEXT:  acc_api_info
  // COMPUTE_CONSTRUCT_END-NEXT:    device_api=0, valid_bytes=12,
  // COMPUTE_CONSTRUCT_END-NEXT:    device_type=acc_device_not_host
  return 0;
#line 30000
}

// NONE-NOT: {{.}}
// ERR-NOT:{{.}}
