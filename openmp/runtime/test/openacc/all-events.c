// Check that all callbacks are dispatched in the correct order with the
// correct data for a simple program.

// REQUIRES: ompt
//
// RUN: %data tgts {
// RUN:   (run-if=                tgt-cflags=                                     tgt-fc=HOST       )
// RUN:   (run-if=%run-if-x86_64  tgt-cflags=-fopenmp-targets=%run-x86_64-triple  tgt-fc=OFF,X86_64 )
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags=-fopenmp-targets=%run-nvptx64-triple tgt-fc=OFF,NVPTX64)
// RUN: }
//      # Check offloading compilation both with and without offloading at run
//      # time.  This is important because some runtime calls that must be
//      # instrumented with some callback data are not exercised in both cases.
// RUN: %data run-envs {
// RUN:   (run-env=                                  fc=%[tgt-fc])
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled' fc=HOST     )
// RUN: }
// RUN: %for tgts {
// RUN:   %[run-if] %clang -Xclang -verify -fopenacc %flags %s -o %t \
// RUN:                    %[tgt-cflags]
// RUN:   %for run-envs {
// RUN:     %[run-if] %[run-env] %t > %t.out 2> %t.err
// RUN:     %[run-if] FileCheck -input-file %t.err %s \
// RUN:         -allow-empty -check-prefixes=ERR
// RUN:     %[run-if] FileCheck -input-file %t.out %s \
// RUN:         -match-full-lines -strict-whitespace \
// RUN:         -check-prefixes=CHECK,%[fc] \
// RUN:         -DVERSION=%acc-version -DHOST_DEV=%acc-host-dev \
// RUN:         -DOFF_DEV=0 -DTHREAD_ID=0 -DASYNC_QUEUE=-1 -DSRC_FILE=%s \
// RUN:         -DLINE_NO=20000 -DEND_LINE_NO=30000 \
// RUN:         -DFUNC_LINE_NO=10000 -DFUNC_END_LINE_NO=40000
// RUN:   }
// RUN: }
//
// END.

// expected-no-diagnostics

#include "callbacks.h"

// ERR-NOT:{{.}}
void acc_register_library(acc_prof_reg reg, acc_prof_reg unreg,
                          acc_prof_lookup lookup) {
  register_all_callbacks(reg);
}

#line 10000
int main() {
  // CHECK-NOT:{{.}}

  int arr0[2] = {10, 11};
  int arr1[5] = {20, 21, 22, 23, 24};

  //      CHECK:arr0 host ptr = [[ARR0_HOST_PTR:0x[a-z0-9]+]]
  // CHECK-NEXT:arr1 host ptr = [[ARR1_HOST_PTR:0x[a-z0-9]+]]
  printf("arr0 host ptr = %p\n", arr0);
  printf("arr1 host ptr = %p\n", arr1);

  // CHECK-NEXT:before kernel
  printf("before kernel\n");

  // Device initialization.
  //
  // OFF-NEXT:acc_ev_device_init_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=1, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=1, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_device_init_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=2, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=2, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host

  // Parallel construct start.
  //
  // CHECK-NEXT:acc_ev_compute_construct_start
  // CHECK-NEXT:  acc_prof_info
  // CHECK-NEXT:    event_type=16, valid_bytes=72, version=[[VERSION]],
  //  HOST-NEXT:    device_type=acc_device_host, device_number=[[HOST_DEV]],
  //   OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // CHECK-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // CHECK-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // CHECK-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // CHECK-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // CHECK-NEXT:  acc_other_event_info
  // CHECK-NEXT:    event_type=16, valid_bytes=24,
  // CHECK-NEXT:    parent_construct=acc_construct_parallel,
  // CHECK-NEXT:    implicit=0, tool_info=(nil)
  // CHECK-NEXT:  acc_api_info
  // CHECK-NEXT:    device_api=0, valid_bytes=12,
  //  HOST-NEXT:    device_type=acc_device_host
  //   OFF-NEXT:    device_type=acc_device_not_host

  // Enter data for arr0 and arr1.
  //
  // OFF-NEXT:acc_ev_enter_data_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=10, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=10, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_alloc
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=8, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=8, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-NEXT:    device_ptr=[[ARR0_DEVICE_PTR:0x[a-z0-9]+]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_create
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=6, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=6, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_enqueue_upload_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=20, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=20, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_enqueue_upload_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=21, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=21, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_alloc
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=8, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=8, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=20,
  // OFF-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-NEXT:    device_ptr=[[ARR1_DEVICE_PTR:0x[a-z0-9]+]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_create
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=6, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=6, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=20,
  // OFF-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_enqueue_upload_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=20, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=20, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=20,
  // OFF-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_enqueue_upload_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=21, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=21, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=20,
  // OFF-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_enter_data_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=11, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=11, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host

  // Enqueue launch.
  //
  // CHECK-NEXT:acc_ev_enqueue_launch_start
  // CHECK-NEXT:  acc_prof_info
  // CHECK-NEXT:    event_type=18, valid_bytes=72, version=[[VERSION]],
  //  HOST-NEXT:    device_type=acc_device_host, device_number=[[HOST_DEV]],
  //   OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // CHECK-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // CHECK-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // CHECK-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // CHECK-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // CHECK-NEXT:  acc_launch_event_info
  // CHECK-NEXT:    event_type=18, valid_bytes=32,
  // CHECK-NEXT:    parent_construct=acc_construct_parallel,
  // CHECK-NEXT:    implicit=0, tool_info=(nil),
  // CHECK-NEXT:    kernel_name=(nil)
  // CHECK-NEXT:  acc_api_info
  // CHECK-NEXT:    device_api=0, valid_bytes=12,
  //  HOST-NEXT:    device_type=acc_device_host
  //   OFF-NEXT:    device_type=acc_device_not_host
  // CHECK-NEXT:acc_ev_enqueue_launch_end
  // CHECK-NEXT:  acc_prof_info
  // CHECK-NEXT:    event_type=19, valid_bytes=72, version=[[VERSION]],
  //  HOST-NEXT:    device_type=acc_device_host, device_number=[[HOST_DEV]],
  //   OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // CHECK-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // CHECK-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // CHECK-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // CHECK-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // CHECK-NEXT:  acc_launch_event_info
  // CHECK-NEXT:    event_type=19, valid_bytes=32,
  // CHECK-NEXT:    parent_construct=acc_construct_parallel,
  // CHECK-NEXT:    implicit=0, tool_info=(nil),
  // CHECK-NEXT:    kernel_name=(nil)
  // CHECK-NEXT:  acc_api_info
  // CHECK-NEXT:    device_api=0, valid_bytes=12,
  //  HOST-NEXT:    device_type=acc_device_host
  //   OFF-NEXT:    device_type=acc_device_not_host

#line 20000
  #pragma acc parallel copy(arr0, arr1) num_gangs(1)
  for (int j = 0; j < 2; ++j) {
    // HOST-NEXT:inside: arr0=[[ARR0_HOST_PTR]], arr0[0]=10
    // HOST-NEXT:inside: arr1=[[ARR1_HOST_PTR]], arr1[0]=20
    // HOST-NEXT:inside: arr0=[[ARR0_HOST_PTR]], arr0[1]=11
    // HOST-NEXT:inside: arr1=[[ARR1_HOST_PTR]], arr1[1]=21
    //  OFF-NEXT:inside: arr0=[[ARR0_DEVICE_PTR]], arr0[0]=10
    //  OFF-NEXT:inside: arr1=[[ARR1_DEVICE_PTR]], arr1[0]=20
    //  OFF-NEXT:inside: arr0=[[ARR0_DEVICE_PTR]], arr0[1]=11
    //  OFF-NEXT:inside: arr1=[[ARR1_DEVICE_PTR]], arr1[1]=21
    printf("inside: arr0=%p, arr0[%d]=%d\n", arr0, j, arr0[j]);
    printf("inside: arr1=%p, arr1[%d]=%d\n", arr1, j, arr1[j]);
#line 30000
  }

  // Exit data for arr0 and arr1.
  //
  // OFF-NEXT:acc_ev_exit_data_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=12, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=12, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_enqueue_download_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=22, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=22, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=20,
  // OFF-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_enqueue_download_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=23, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=23, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=20,
  // OFF-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_delete
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=7, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=7, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=20,
  // OFF-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_free
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=9, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=9, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=20,
  // OFF-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_enqueue_download_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=22, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=22, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_enqueue_download_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=23, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=23, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_delete
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=7, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=7, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_free
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=9, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=9, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_exit_data_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=13, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=13, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host

  // Parallel construct end.
  //
  // CHECK-NEXT:acc_ev_compute_construct_end
  // CHECK-NEXT:  acc_prof_info
  // CHECK-NEXT:    event_type=17, valid_bytes=72, version=[[VERSION]],
  //  HOST-NEXT:    device_type=acc_device_host, device_number=[[HOST_DEV]],
  //   OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // CHECK-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // CHECK-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // CHECK-NEXT:    line_no=[[LINE_NO]], end_line_no=[[END_LINE_NO]],
  // CHECK-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // CHECK-NEXT:  acc_other_event_info
  // CHECK-NEXT:    event_type=17, valid_bytes=24,
  // CHECK-NEXT:    parent_construct=acc_construct_parallel,
  // CHECK-NEXT:    implicit=0, tool_info=(nil)
  // CHECK-NEXT:  acc_api_info
  // CHECK-NEXT:    device_api=0, valid_bytes=12,
  //  HOST-NEXT:    device_type=acc_device_host
  //   OFF-NEXT:    device_type=acc_device_not_host

  // CHECK-NEXT:after kernel
  printf("after kernel\n");

  return 0;
#line 40000
}

// Device shutdown.
//
// OFF-NEXT:acc_ev_device_shutdown_start
// OFF-NEXT:  acc_prof_info
// OFF-NEXT:    event_type=3, valid_bytes=72, version=[[VERSION]],
// OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
// OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
// OFF-NEXT:    src_file=(null), func_name=(null),
// OFF-NEXT:    line_no=0, end_line_no=0,
// OFF-NEXT:    func_line_no=0, func_end_line_no=0
// OFF-NEXT:  acc_other_event_info
// OFF-NEXT:    event_type=3, valid_bytes=24,
// OFF-NEXT:    parent_construct=acc_construct_runtime_api,
// OFF-NEXT:    implicit=1, tool_info=(nil)
// OFF-NEXT:  acc_api_info
// OFF-NEXT:    device_api=0, valid_bytes=12,
// OFF-NEXT:    device_type=acc_device_not_host
// OFF-NEXT:acc_ev_device_shutdown_end
// OFF-NEXT:  acc_prof_info
// OFF-NEXT:    event_type=4, valid_bytes=72, version=[[VERSION]],
// OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
// OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
// OFF-NEXT:    src_file=(null), func_name=(null),
// OFF-NEXT:    line_no=0, end_line_no=0,
// OFF-NEXT:    func_line_no=0, func_end_line_no=0
// OFF-NEXT:  acc_other_event_info
// OFF-NEXT:    event_type=4, valid_bytes=24,
// OFF-NEXT:    parent_construct=acc_construct_runtime_api,
// OFF-NEXT:    implicit=1, tool_info=(nil)
// OFF-NEXT:  acc_api_info
// OFF-NEXT:    device_api=0, valid_bytes=12,
// OFF-NEXT:    device_type=acc_device_not_host

// Runtime shutdown.
//
// CHECK-NEXT:acc_ev_runtime_shutdown
// CHECK-NEXT:  acc_prof_info
// CHECK-NEXT:    event_type=5, valid_bytes=72, version=[[VERSION]],
// CHECK-NEXT:    device_type=acc_device_host, device_number=[[HOST_DEV]],
// CHECK-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
// CHECK-NEXT:    src_file=(null), func_name=(null),
// CHECK-NEXT:    line_no=0, end_line_no=0,
// CHECK-NEXT:    func_line_no=0, func_end_line_no=0
// CHECK-NEXT:  acc_other_event_info
// CHECK-NEXT:    event_type=5, valid_bytes=24,
// CHECK-NEXT:    parent_construct=acc_construct_runtime_api,
// CHECK-NEXT:    implicit=1, tool_info=(nil)
// CHECK-NEXT:  acc_api_info
// CHECK-NEXT:    device_api=0, valid_bytes=12,
// CHECK-NEXT:    device_type=acc_device_host
