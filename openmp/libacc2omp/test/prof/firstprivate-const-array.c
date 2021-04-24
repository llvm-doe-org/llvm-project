// Check callbacks for the unusual case of a firstprivate const array:
//
// 1. acc_ev_create triggers before the first kernel even if that kernel
//    doesn't access the array.  However, acc_ev_enqueue_upload_{start,end}
//    doesn't trigger until the kernel that accesses the array.  (This
//    acc_ev_create is triggered by the ompt_callback_target_data_op_emi
//    callback with optype=ompt_target_data_associate that is dispatched within
//    omptarget.cpp's InitLibrary.  This test case is important for covering
//    that dispatch.)
// 2. When offloading, the array's host address as seen on the host
//    (CARR_HOST_PTR_ON_HOST) isn't the array's host address provided to the
//    callbacks (CARR_HOST_PTR_IN_CB).  FIXME: Why is this true?  Note that
//    it's consistent across callbacks, so it's not a result of one incorrectly
//    coded callback.

// RUN: %data tgts {
// RUN:   (run-if=
// RUN:    tgt-cflags=
// RUN:    tgt-acc-device=acc_device_host
// RUN:    fc=HOST)
// RUN:   (run-if=%run-if-x86_64
// RUN:    tgt-cflags=-fopenmp-targets=%run-x86_64-triple
// RUN:    tgt-acc-device=acc_device_x86_64
// RUN:    fc=OFF,X86_64 )
// RUN:   (run-if=%run-if-ppc64le
// RUN:    tgt-cflags=-fopenmp-targets=%run-ppc64le-triple
// RUN:    tgt-acc-device=acc_device_ppc64le
// RUN:    fc=OFF,PPC64LE)
// RUN:   (run-if=%run-if-nvptx64
// RUN:    tgt-cflags='-fopenmp-targets=%run-nvptx64-triple -DNVPTX64'
// RUN:    tgt-acc-device=acc_device_nvidia
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
// RUN:       -DACC_DEVICE=%[tgt-acc-device] -DVERSION=%acc-version \
// RUN:       -DHOST_DEV=%acc-host-dev -DOFF_DEV=0 -DTHREAD_ID=0 \
// RUN:       -DASYNC_QUEUE=-1 -DSRC_FILE=%s \
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
  const int carr[10] = {30, 31, 32, 33, 34, 35, 36, 37, 38, 39};

  // CHECK:carr host ptr = [[CARR_HOST_PTR_ON_HOST:0x[a-z0-9]+]]
  printf("carr host ptr = %p\n", carr);

  // CHECK:before kernel 0
  printf("before kernel 0\n");

  // OFF:acc_ev_device_init_start
  // OFF:acc_ev_device_init_end

  // Creation of const array before first kernel even though it's not used
  // until a later kernel.
  //
  // FIXME: var_name isn't yet supported in this case.
  //
  //      OFF:acc_ev_create
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=6, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO0]], end_line_no=[[END_LINE_NO0]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=6, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=40,
  // OFF-NEXT:    host_ptr=[[CARR_HOST_PTR_IN_CB:0x[a-z0-9]+]],
  // OFF-NEXT:    device_ptr=[[CARR_DEVICE_PTR:0x[a-z0-9]+]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]

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

  // Enter data for carr.
  //
  //      OFF:acc_ev_enter_data_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=10, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
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
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_enqueue_upload_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=20, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=20, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=carr, bytes=40,
  // OFF-NEXT:    host_ptr=[[CARR_HOST_PTR_IN_CB]],
  // OFF-NEXT:    device_ptr=[[CARR_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_enqueue_upload_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=21, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=21, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=carr, bytes=40,
  // OFF-NEXT:    host_ptr=[[CARR_HOST_PTR_IN_CB]],
  // OFF-NEXT:    device_ptr=[[CARR_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_enter_data_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=11, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
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
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]

  // CHECK:acc_ev_enqueue_launch_start
  // CHECK:acc_ev_enqueue_launch_end

#line 30000
  #pragma acc parallel firstprivate(carr) num_gangs(1)
  for (int j = 0; j < 10; ++j) {
    // Because of firstprivate, carr's address is different here even when not
    // offloading.
    //         HOST:inside: carr=[[CARR_HOST_PTR_IN_KERNEL:0x[a-z0-9]+]], carr[0]=30
    //    HOST-NEXT:inside: carr=[[CARR_HOST_PTR_IN_KERNEL]], carr[1]=31
    //    HOST-NEXT:inside: carr=[[CARR_HOST_PTR_IN_KERNEL]], carr[2]=32
    //    HOST-NEXT:inside: carr=[[CARR_HOST_PTR_IN_KERNEL]], carr[3]=33
    //    HOST-NEXT:inside: carr=[[CARR_HOST_PTR_IN_KERNEL]], carr[4]=34
    //    HOST-NEXT:inside: carr=[[CARR_HOST_PTR_IN_KERNEL]], carr[5]=35
    //    HOST-NEXT:inside: carr=[[CARR_HOST_PTR_IN_KERNEL]], carr[6]=36
    //    HOST-NEXT:inside: carr=[[CARR_HOST_PTR_IN_KERNEL]], carr[7]=37
    //    HOST-NEXT:inside: carr=[[CARR_HOST_PTR_IN_KERNEL]], carr[8]=38
    //    HOST-NEXT:inside: carr=[[CARR_HOST_PTR_IN_KERNEL]], carr[9]=39
    //       X86_64:inside: carr=[[CARR_DEVICE_PTR]], carr[0]=30
    //  X86_64-NEXT:inside: carr=[[CARR_DEVICE_PTR]], carr[1]=31
    //  X86_64-NEXT:inside: carr=[[CARR_DEVICE_PTR]], carr[2]=32
    //  X86_64-NEXT:inside: carr=[[CARR_DEVICE_PTR]], carr[3]=33
    //  X86_64-NEXT:inside: carr=[[CARR_DEVICE_PTR]], carr[4]=34
    //  X86_64-NEXT:inside: carr=[[CARR_DEVICE_PTR]], carr[5]=35
    //  X86_64-NEXT:inside: carr=[[CARR_DEVICE_PTR]], carr[6]=36
    //  X86_64-NEXT:inside: carr=[[CARR_DEVICE_PTR]], carr[7]=37
    //  X86_64-NEXT:inside: carr=[[CARR_DEVICE_PTR]], carr[8]=38
    //  X86_64-NEXT:inside: carr=[[CARR_DEVICE_PTR]], carr[9]=39
    //      PPC64LE:inside: carr=[[CARR_DEVICE_PTR]], carr[0]=30
    // PPC64LE-NEXT:inside: carr=[[CARR_DEVICE_PTR]], carr[1]=31
    // PPC64LE-NEXT:inside: carr=[[CARR_DEVICE_PTR]], carr[2]=32
    // PPC64LE-NEXT:inside: carr=[[CARR_DEVICE_PTR]], carr[3]=33
    // PPC64LE-NEXT:inside: carr=[[CARR_DEVICE_PTR]], carr[4]=34
    // PPC64LE-NEXT:inside: carr=[[CARR_DEVICE_PTR]], carr[5]=35
    // PPC64LE-NEXT:inside: carr=[[CARR_DEVICE_PTR]], carr[6]=36
    // PPC64LE-NEXT:inside: carr=[[CARR_DEVICE_PTR]], carr[7]=37
    // PPC64LE-NEXT:inside: carr=[[CARR_DEVICE_PTR]], carr[8]=38
    // PPC64LE-NEXT:inside: carr=[[CARR_DEVICE_PTR]], carr[9]=39
    // We omit NVPTX64 here because exit events might trigger before kernel
    // execution due to the use of CUDA streams.
#ifndef NVPTX64
    printf("inside: carr=%p, carr[%d]=%d\n", carr, j, carr[j]);
#endif
#line 40000
  }

  // Exit data for carr.
  //
  //      OFF:acc_ev_exit_data_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=12, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
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
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_exit_data_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=13, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
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
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]

  // CHECK:acc_ev_compute_construct_end

  // CHECK:after kernels
  printf("after kernels\n");

  return 0;
#line 50000
}

//   OFF:acc_ev_device_shutdown_start
//   OFF:acc_ev_device_shutdown_end
// CHECK:acc_ev_runtime_shutdown
