// Check callbacks for the case of a firstprivate non-const array.  This case is
// special because it involves unique code paths in Clang codegen and in the
// OpenMP runtime.  Moreover, due to privatization, the array's device address
// seen in callbacks (ARR_DEVICE_PTR_IN_CB) is different than the one seen
// within kernels (ARR_DEVICE_PTR_IN_KERNEL).  The former is the original device
// copy of the host's data, and the latter is the per-gang private copy.

// RUN: %data tgts {
// RUN:   (run-if=
// RUN:    tgt-cflags=
// RUN:    fc=HOST       )
// RUN:   (run-if=%run-if-x86_64
// RUN:    tgt-cflags=-fopenmp-targets=%run-x86_64-triple
// RUN:    fc=OFF,X86_64 )
// RUN:   (run-if=%run-if-ppc64le
// RUN:    tgt-cflags=-fopenmp-targets=%run-ppc64le-triple
// RUN:    fc=OFF,PPC64LE)
// RUN:   (run-if=%run-if-nvptx64
// RUN:    tgt-cflags='-fopenmp-targets=%run-nvptx64-triple -DNVPTX64'
// RUN:    fc=OFF,NVPTX64)
// RUN: }
// RUN: %for tgts {
// RUN:   %[run-if] %clang -Xclang -verify -fopenacc %acc-includes %s -o %t \
// RUN:                    %[tgt-cflags]
// RUN:   %[run-if] %t > %t.out 2> %t.err
// RUN:   %[run-if] FileCheck -input-file %t.err %s \
// RUN:       -allow-empty -check-prefixes=ERR
// RUN:   %[run-if] FileCheck -input-file %t.out %s \
// RUN:       -match-full-lines -strict-whitespace \
// RUN:       -implicit-check-not=acc_ev_ -check-prefixes=CHECK,%[fc] \
// RUN:       -DVERSION=%acc-version -DHOST_DEV=%acc-host-dev \
// RUN:       -DOFF_DEV=0 -DTHREAD_ID=0 -DASYNC_QUEUE=-1 -DSRC_FILE=%s \
// RUN:       -DLINE_NO0=20000 -DEND_LINE_NO0=20001 \
// RUN:       -DLINE_NO1=30000 -DEND_LINE_NO1=40000 \
// RUN:       -DFUNC_LINE_NO=10000 -DFUNC_END_LINE_NO=50000
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
  int arr[10] = {30, 31, 32, 33, 34, 35, 36, 37, 38, 39};

  // CHECK:arr host ptr = [[ARR_HOST_PTR_ON_HOST:0x[a-z0-9]+]]
  printf("arr host ptr = %p\n", arr);

  // CHECK:before kernel 0
  printf("before kernel 0\n");

  // OFF:acc_ev_device_init_start
  // OFF:acc_ev_device_init_end

  // CHECK:acc_ev_compute_construct_start
  //   OFF:acc_ev_enter_data_start
  //   OFF:acc_ev_enter_data_end
  // CHECK:acc_ev_enqueue_launch_start
  // CHECK:acc_ev_enqueue_launch_end
#line 20000
  #pragma acc parallel num_gangs(1)
    ;
  //   OFF:acc_ev_exit_data_start
  //   OFF:acc_ev_exit_data_end
  // CHECK:acc_ev_compute_construct_end

  // CHECK:before kernel 1
  printf("before kernel 1\n");

  // CHECK:acc_ev_compute_construct_start

  // Enter data for arr.
  //
  //      OFF:acc_ev_enter_data_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=10, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
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
  // OFF-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=8, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=arr, bytes=40,
  // OFF-NEXT:    host_ptr=[[ARR_HOST_PTR_ON_HOST]],
  // OFF-NEXT:    device_ptr=[[ARR_DEVICE_PTR_IN_CB:0x[a-z0-9]+]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_create
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=6, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=6, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=arr, bytes=40,
  // OFF-NEXT:    host_ptr=[[ARR_HOST_PTR_ON_HOST]],
  // OFF-NEXT:    device_ptr=[[ARR_DEVICE_PTR_IN_CB]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_enqueue_upload_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=20, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=20, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=arr, bytes=40,
  // OFF-NEXT:    host_ptr=[[ARR_HOST_PTR_ON_HOST]],
  // OFF-NEXT:    device_ptr=[[ARR_DEVICE_PTR_IN_CB]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_enqueue_upload_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=21, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=21, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=arr, bytes=40,
  // OFF-NEXT:    host_ptr=[[ARR_HOST_PTR_ON_HOST]],
  // OFF-NEXT:    device_ptr=[[ARR_DEVICE_PTR_IN_CB]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_enter_data_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=11, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=11, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host

  // CHECK:acc_ev_enqueue_launch_start
  // CHECK:acc_ev_enqueue_launch_end

#line 30000
  #pragma acc parallel firstprivate(arr) num_gangs(1)
  for (int j = 0; j < 10; ++j) {
    // Because of firstprivate, arr's address is different here even when not
    // offloading.
    //         HOST:inside: arr=[[ARR_HOST_PTR_IN_KERNEL:0x[a-z0-9]+]], arr[0]=30
    //    HOST-NEXT:inside: arr=[[ARR_HOST_PTR_IN_KERNEL]], arr[1]=31
    //    HOST-NEXT:inside: arr=[[ARR_HOST_PTR_IN_KERNEL]], arr[2]=32
    //    HOST-NEXT:inside: arr=[[ARR_HOST_PTR_IN_KERNEL]], arr[3]=33
    //    HOST-NEXT:inside: arr=[[ARR_HOST_PTR_IN_KERNEL]], arr[4]=34
    //    HOST-NEXT:inside: arr=[[ARR_HOST_PTR_IN_KERNEL]], arr[5]=35
    //    HOST-NEXT:inside: arr=[[ARR_HOST_PTR_IN_KERNEL]], arr[6]=36
    //    HOST-NEXT:inside: arr=[[ARR_HOST_PTR_IN_KERNEL]], arr[7]=37
    //    HOST-NEXT:inside: arr=[[ARR_HOST_PTR_IN_KERNEL]], arr[8]=38
    //    HOST-NEXT:inside: arr=[[ARR_HOST_PTR_IN_KERNEL]], arr[9]=39
    //       X86_64:inside: arr=[[ARR_DEVICE_PTR_IN_KERNEL:0x[a-z0-9]+]], arr[0]=30
    //  X86_64-NEXT:inside: arr=[[ARR_DEVICE_PTR_IN_KERNEL]], arr[1]=31
    //  X86_64-NEXT:inside: arr=[[ARR_DEVICE_PTR_IN_KERNEL]], arr[2]=32
    //  X86_64-NEXT:inside: arr=[[ARR_DEVICE_PTR_IN_KERNEL]], arr[3]=33
    //  X86_64-NEXT:inside: arr=[[ARR_DEVICE_PTR_IN_KERNEL]], arr[4]=34
    //  X86_64-NEXT:inside: arr=[[ARR_DEVICE_PTR_IN_KERNEL]], arr[5]=35
    //  X86_64-NEXT:inside: arr=[[ARR_DEVICE_PTR_IN_KERNEL]], arr[6]=36
    //  X86_64-NEXT:inside: arr=[[ARR_DEVICE_PTR_IN_KERNEL]], arr[7]=37
    //  X86_64-NEXT:inside: arr=[[ARR_DEVICE_PTR_IN_KERNEL]], arr[8]=38
    //  X86_64-NEXT:inside: arr=[[ARR_DEVICE_PTR_IN_KERNEL]], arr[9]=39
    //      PPC64LE:inside: arr=[[ARR_DEVICE_PTR_IN_KERNEL:0x[a-z0-9]+]], arr[0]=30
    // PPC64LE-NEXT:inside: arr=[[ARR_DEVICE_PTR_IN_KERNEL]], arr[1]=31
    // PPC64LE-NEXT:inside: arr=[[ARR_DEVICE_PTR_IN_KERNEL]], arr[2]=32
    // PPC64LE-NEXT:inside: arr=[[ARR_DEVICE_PTR_IN_KERNEL]], arr[3]=33
    // PPC64LE-NEXT:inside: arr=[[ARR_DEVICE_PTR_IN_KERNEL]], arr[4]=34
    // PPC64LE-NEXT:inside: arr=[[ARR_DEVICE_PTR_IN_KERNEL]], arr[5]=35
    // PPC64LE-NEXT:inside: arr=[[ARR_DEVICE_PTR_IN_KERNEL]], arr[6]=36
    // PPC64LE-NEXT:inside: arr=[[ARR_DEVICE_PTR_IN_KERNEL]], arr[7]=37
    // PPC64LE-NEXT:inside: arr=[[ARR_DEVICE_PTR_IN_KERNEL]], arr[8]=38
    // PPC64LE-NEXT:inside: arr=[[ARR_DEVICE_PTR_IN_KERNEL]], arr[9]=39
    // We omit NVPTX64 here because exit events might trigger before kernel
    // execution due to the use of CUDA streams.
#ifndef NVPTX64
    printf("inside: arr=%p, arr[%d]=%d\n", arr, j, arr[j]);
#endif
#line 40000
  }

  // Exit data for arr.
  //
  //      OFF:acc_ev_exit_data_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=12, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=12, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_delete
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=7, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=7, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=arr, bytes=40,
  // OFF-NEXT:    host_ptr=[[ARR_HOST_PTR_ON_HOST]],
  // OFF-NEXT:    device_ptr=[[ARR_DEVICE_PTR_IN_CB]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_free
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=9, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=9, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=arr, bytes=40,
  // OFF-NEXT:    host_ptr=[[ARR_HOST_PTR_ON_HOST]],
  // OFF-NEXT:    device_ptr=[[ARR_DEVICE_PTR_IN_CB:0x[a-z0-9]+]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_exit_data_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=13, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=13, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host

  // CHECK:acc_ev_compute_construct_end

  // CHECK:after kernels
  printf("after kernels\n");

  return 0;
#line 50000
}

//   OFF:acc_ev_device_shutdown_start
//   OFF:acc_ev_device_shutdown_end
// CHECK:acc_ev_runtime_shutdown
