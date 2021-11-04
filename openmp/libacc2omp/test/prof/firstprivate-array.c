// Check callbacks for the case of a firstprivate non-const array.
//
// This case is special because it involves unique code paths in Clang codegen
// and in the OpenMP runtime.  Moreover, the addresses seen in callbacks
// (PACK0_HOST_PTR and PACK0_DEVICE_PTR) can be different than the mapped
// variable's address seen on the host (ARR0_HOST_PTR) or within kernels
// (ARR0_DEVICE_PTR).  PACK0_HOST_PTR is the address of a buffer allocated for
// packing small firstprivates even when there's only one firstprivate.
// PACK0_DEVICE_PTR is the original device copy of the host's data within that
// buffer, and ARR0_DEVICE_PTR is the per-gang private copy of the specific
// variable.
//
// It used to be the case that, if the firstprivate array was const,
// acc_ev_create would be triggered immediately before the first kernel even if
// that kernel didn't access the array.  Moreover, var_name wouldn't be set, and
// acc_ev_alloc, acc_ev_delete, and acc_ev_free wouldn't be triggered.  Thus,
// we continue to test with a preceding kernel, and we test both const and
// non-const cases.

//      # Large firstprivates are not packed, so they exercise a different OMPT
//      # code path.  Packing multiple (small) firstprivates means we cannot
//      # provide a single var_name, so currently it's set to a null pointer.
// RUN: %data packing-cases {
// RUN:   (arr0-size=64   pack0-name=arr0     pack0-size=64   pack0=PACK0
// RUN:    arr1-size=0    pack1-name=         pack1-size=0    pack1=NO_PACK1)
// RUN:   (arr0-size=2048 pack0-name=arr0     pack0-size=2048 pack0=ARR0
// RUN:    arr1-size=0    pack1-name=         pack1-size=0    pack1=NO_PACK1)
// RUN:   (arr0-size=64   pack0-name='(null)' pack0-size=128  pack0=PACK0
// RUN:    arr1-size=64   pack1-name=         pack1-size=0    pack1=NO_PACK1)
// RUN:   (arr0-size=2048 pack0-name=arr0     pack0-size=2048 pack0=ARR0
// RUN:    arr1-size=64   pack1-name=arr1     pack1-size=64   pack1=PACK1   )
// RUN: }
// RUN: %data const-cases {
// RUN:   (const=     )
// RUN:   (const=const)
// RUN: }
// RUN: %for packing-cases {
// RUN:   %for const-cases {
// RUN:     %clang-acc %s -o %t.exe -DCONST=%[const] \
// RUN:       -DARR0_SIZE=%[arr0-size] -DARR1_SIZE=%[arr1-size] \
// RUN:       -DTGT_%dev-type-0-omp
// RUN:     %t.exe > %t.out 2> %t.err
// RUN:     FileCheck -input-file %t.err %s \
// RUN:       -allow-empty -check-prefixes=ERR
// RUN:     FileCheck -input-file %t.out %s \
// RUN:       -match-full-lines -strict-whitespace \
// RUN:       -implicit-check-not=acc_ev_ \
// RUN:       -check-prefixes=CHECK,TGT-%dev-type-0-omp,%if-host(HOST,OFF) \
// RUN:       -check-prefixes=CHECK-%[pack0],%if-host(HOST,OFF)-%[pack0] \
// RUN:       -check-prefixes=CHECK-%[pack1],%if-host(HOST,OFF)-%[pack1] \
// RUN:       -DACC_DEVICE=acc_device_%dev-type-0-acc -DVERSION=%acc-version \
// RUN:       -DHOST_DEV=%acc-host-dev -DOFF_DEV=0 -DTHREAD_ID=0 \
// RUN:       -DASYNC_QUEUE=-1 -DSRC_FILE=%s \
// RUN:       -DLINE_NO0=20000 -DEND_LINE_NO0=20001 \
// RUN:       -DLINE_NO1=30000 -DEND_LINE_NO1=40000 \
// RUN:       -DFUNC_LINE_NO=10000 -DFUNC_END_LINE_NO=50000 \
// RUN:       -DPACK0_NAME=%[pack0-name] -D#PACK0_SIZE=%[pack0-size] \
// RUN:       -DPACK1_NAME=%[pack1-name] -D#PACK1_SIZE=%[pack1-size]
// RUN:   }
// RUN: }
//
// END.

// expected-error 0 {{}}

// FIXME: Clang produces spurious warning diagnostics for nvptx64 offload.  This
// issue is not limited to Clacc and is present upstream:
// nvptx64-warning@*:* 0+ {{Linking two modules of different target triples}}

#include "callbacks.h"

// ERR-NOT:{{.}}
void acc_register_library(acc_prof_reg reg, acc_prof_reg unreg,
                          acc_prof_lookup lookup) {
  register_all_callbacks(reg);
}

#ifndef ARR0_SIZE
# define ARR0_SIZE 1
# error "expected ARR0_SIZE definition"
#endif

#ifndef ARR1_SIZE
# define ARR1_SIZE 1
# error "expected ARR1_SIZE definition"
#endif

#line 10000
int main() {
  CONST char arr0[ARR0_SIZE] = {30, 31, 32, 33, 34, 35, 36, 37, 38, 39};
  // CHECK:arr0 host ptr = 0x[[#%x,ARR0_HOST_PTR:]]
  printf("arr0 host ptr = %p\n", arr0);

#if ARR1_SIZE
  char arr1[ARR1_SIZE];
  // CHECK-PACK1:arr1 host ptr = 0x[[#%x,ARR1_HOST_PTR:]]
  printf("arr1 host ptr = %p\n", arr1);
#endif

  // CHECK:before kernel 0
  printf("before kernel 0\n");

  // OFF:acc_ev_device_init_start
  // OFF:acc_ev_device_init_end

  // CHECK:acc_ev_compute_construct_start
  // CHECK:acc_ev_enqueue_launch_start
  // CHECK:acc_ev_enqueue_launch_end
#line 20000
  #pragma acc parallel num_gangs(1)
    ;
  // CHECK:acc_ev_compute_construct_end

  // CHECK:before kernel 1
  printf("before kernel 1\n");

  // CHECK:acc_ev_compute_construct_start

  //            OFF:acc_ev_enter_data_start
  //       OFF-NEXT:  acc_prof_info
  //       OFF-NEXT:    event_type=10, valid_bytes=72, version=[[VERSION]],
  //       OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  //       OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //       OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //       OFF-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  //       OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //       OFF-NEXT:  acc_other_event_info
  //       OFF-NEXT:    event_type=10, valid_bytes=24,
  //       OFF-NEXT:    parent_construct=acc_construct_parallel,
  //       OFF-NEXT:    implicit=0, tool_info=(nil)
  //       OFF-NEXT:  acc_api_info
  //       OFF-NEXT:    device_api=0, valid_bytes=12,
  //       OFF-NEXT:    device_type=[[ACC_DEVICE]]
  //       OFF-NEXT:acc_ev_alloc
  //       OFF-NEXT:  acc_prof_info
  //       OFF-NEXT:    event_type=8, valid_bytes=72, version=[[VERSION]],
  //       OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  //       OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //       OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //       OFF-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  //       OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //       OFF-NEXT:  acc_data_event_info
  //       OFF-NEXT:    event_type=8, valid_bytes=56,
  //       OFF-NEXT:    parent_construct=acc_construct_parallel,
  //       OFF-NEXT:    implicit=0, tool_info=(nil),
  //       OFF-NEXT:    var_name=[[PACK0_NAME]], bytes=[[#PACK0_SIZE]],
  //  OFF-ARR0-NEXT:    host_ptr=0x[[#%x,PACK0_HOST_PTR:ARR0_HOST_PTR]],
  // OFF-PACK0-NEXT:    host_ptr=0x[[#%x,PACK0_HOST_PTR:]],
  //       OFF-NEXT:    device_ptr=0x[[#%x,PACK0_DEVICE_PTR:]]
  //       OFF-NEXT:  acc_api_info
  //       OFF-NEXT:    device_api=0, valid_bytes=12,
  //       OFF-NEXT:    device_type=[[ACC_DEVICE]]
  //       OFF-NEXT:acc_ev_create
  //       OFF-NEXT:  acc_prof_info
  //       OFF-NEXT:    event_type=6, valid_bytes=72, version=[[VERSION]],
  //       OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  //       OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //       OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //       OFF-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  //       OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //       OFF-NEXT:  acc_data_event_info
  //       OFF-NEXT:    event_type=6, valid_bytes=56,
  //       OFF-NEXT:    parent_construct=acc_construct_parallel,
  //       OFF-NEXT:    implicit=0, tool_info=(nil),
  //       OFF-NEXT:    var_name=[[PACK0_NAME]], bytes=[[#PACK0_SIZE]],
  //       OFF-NEXT:    host_ptr=0x[[#PACK0_HOST_PTR]],
  //       OFF-NEXT:    device_ptr=0x[[#PACK0_DEVICE_PTR]]
  //       OFF-NEXT:  acc_api_info
  //       OFF-NEXT:    device_api=0, valid_bytes=12,
  //       OFF-NEXT:    device_type=[[ACC_DEVICE]]
  //       OFF-NEXT:acc_ev_enqueue_upload_start
  //       OFF-NEXT:  acc_prof_info
  //       OFF-NEXT:    event_type=20, valid_bytes=72, version=[[VERSION]],
  //       OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  //       OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //       OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //       OFF-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  //       OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //       OFF-NEXT:  acc_data_event_info
  //       OFF-NEXT:    event_type=20, valid_bytes=56,
  //       OFF-NEXT:    parent_construct=acc_construct_parallel,
  //       OFF-NEXT:    implicit=0, tool_info=(nil),
  //       OFF-NEXT:    var_name=[[PACK0_NAME]], bytes=[[#PACK0_SIZE]],
  //       OFF-NEXT:    host_ptr=0x[[#PACK0_HOST_PTR]],
  //       OFF-NEXT:    device_ptr=0x[[#PACK0_DEVICE_PTR]]
  //       OFF-NEXT:  acc_api_info
  //       OFF-NEXT:    device_api=0, valid_bytes=12,
  //       OFF-NEXT:    device_type=[[ACC_DEVICE]]
  //       OFF-NEXT:acc_ev_enqueue_upload_end
  //       OFF-NEXT:  acc_prof_info
  //       OFF-NEXT:    event_type=21, valid_bytes=72, version=[[VERSION]],
  //       OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  //       OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //       OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //       OFF-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  //       OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //       OFF-NEXT:  acc_data_event_info
  //       OFF-NEXT:    event_type=21, valid_bytes=56,
  //       OFF-NEXT:    parent_construct=acc_construct_parallel,
  //       OFF-NEXT:    implicit=0, tool_info=(nil),
  //       OFF-NEXT:    var_name=[[PACK0_NAME]], bytes=[[#PACK0_SIZE]],
  //       OFF-NEXT:    host_ptr=0x[[#PACK0_HOST_PTR]],
  //       OFF-NEXT:    device_ptr=0x[[#PACK0_DEVICE_PTR]]
  //       OFF-NEXT:  acc_api_info
  //       OFF-NEXT:    device_api=0, valid_bytes=12,
  //       OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-PACK1-NEXT:acc_ev_alloc
  // OFF-PACK1-NEXT:  acc_prof_info
  // OFF-PACK1-NEXT:    event_type=8, valid_bytes=72, version=[[VERSION]],
  // OFF-PACK1-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-PACK1-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-PACK1-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-PACK1-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  // OFF-PACK1-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-PACK1-NEXT:  acc_data_event_info
  // OFF-PACK1-NEXT:    event_type=8, valid_bytes=56,
  // OFF-PACK1-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-PACK1-NEXT:    implicit=0, tool_info=(nil),
  // OFF-PACK1-NEXT:    var_name=[[PACK1_NAME]], bytes=[[#PACK1_SIZE]],
  // OFF-PACK1-NEXT:    host_ptr=0x[[#%x,PACK1_HOST_PTR:]],
  // OFF-PACK1-NEXT:    device_ptr=0x[[#%x,ARR1_DEVICE_PTR:]]
  // OFF-PACK1-NEXT:  acc_api_info
  // OFF-PACK1-NEXT:    device_api=0, valid_bytes=12,
  // OFF-PACK1-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-PACK1-NEXT:acc_ev_create
  // OFF-PACK1-NEXT:  acc_prof_info
  // OFF-PACK1-NEXT:    event_type=6, valid_bytes=72, version=[[VERSION]],
  // OFF-PACK1-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-PACK1-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-PACK1-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-PACK1-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  // OFF-PACK1-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-PACK1-NEXT:  acc_data_event_info
  // OFF-PACK1-NEXT:    event_type=6, valid_bytes=56,
  // OFF-PACK1-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-PACK1-NEXT:    implicit=0, tool_info=(nil),
  // OFF-PACK1-NEXT:    var_name=[[PACK1_NAME]], bytes=[[#PACK1_SIZE]],
  // OFF-PACK1-NEXT:    host_ptr=0x[[#PACK1_HOST_PTR]],
  // OFF-PACK1-NEXT:    device_ptr=0x[[#ARR1_DEVICE_PTR]]
  // OFF-PACK1-NEXT:  acc_api_info
  // OFF-PACK1-NEXT:    device_api=0, valid_bytes=12,
  // OFF-PACK1-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-PACK1-NEXT:acc_ev_enqueue_upload_start
  // OFF-PACK1-NEXT:  acc_prof_info
  // OFF-PACK1-NEXT:    event_type=20, valid_bytes=72, version=[[VERSION]],
  // OFF-PACK1-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-PACK1-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-PACK1-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-PACK1-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  // OFF-PACK1-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-PACK1-NEXT:  acc_data_event_info
  // OFF-PACK1-NEXT:    event_type=20, valid_bytes=56,
  // OFF-PACK1-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-PACK1-NEXT:    implicit=0, tool_info=(nil),
  // OFF-PACK1-NEXT:    var_name=[[PACK1_NAME]], bytes=[[#PACK1_SIZE]],
  // OFF-PACK1-NEXT:    host_ptr=0x[[#PACK1_HOST_PTR]],
  // OFF-PACK1-NEXT:    device_ptr=0x[[#ARR1_DEVICE_PTR]]
  // OFF-PACK1-NEXT:  acc_api_info
  // OFF-PACK1-NEXT:    device_api=0, valid_bytes=12,
  // OFF-PACK1-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-PACK1-NEXT:acc_ev_enqueue_upload_end
  // OFF-PACK1-NEXT:  acc_prof_info
  // OFF-PACK1-NEXT:    event_type=21, valid_bytes=72, version=[[VERSION]],
  // OFF-PACK1-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-PACK1-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-PACK1-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-PACK1-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  // OFF-PACK1-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-PACK1-NEXT:  acc_data_event_info
  // OFF-PACK1-NEXT:    event_type=21, valid_bytes=56,
  // OFF-PACK1-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-PACK1-NEXT:    implicit=0, tool_info=(nil),
  // OFF-PACK1-NEXT:    var_name=[[PACK1_NAME]], bytes=[[#PACK1_SIZE]],
  // OFF-PACK1-NEXT:    host_ptr=0x[[#PACK1_HOST_PTR]],
  // OFF-PACK1-NEXT:    device_ptr=0x[[#ARR1_DEVICE_PTR]]
  // OFF-PACK1-NEXT:  acc_api_info
  // OFF-PACK1-NEXT:    device_api=0, valid_bytes=12,
  // OFF-PACK1-NEXT:    device_type=[[ACC_DEVICE]]
  //       OFF-NEXT:acc_ev_enter_data_end
  //       OFF-NEXT:  acc_prof_info
  //       OFF-NEXT:    event_type=11, valid_bytes=72, version=[[VERSION]],
  //       OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  //       OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //       OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //       OFF-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  //       OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //       OFF-NEXT:  acc_other_event_info
  //       OFF-NEXT:    event_type=11, valid_bytes=24,
  //       OFF-NEXT:    parent_construct=acc_construct_parallel,
  //       OFF-NEXT:    implicit=0, tool_info=(nil)
  //       OFF-NEXT:  acc_api_info
  //       OFF-NEXT:    device_api=0, valid_bytes=12,
  //       OFF-NEXT:    device_type=[[ACC_DEVICE]]

  // CHECK:acc_ev_enqueue_launch_start
  // CHECK:acc_ev_enqueue_launch_end

#if ARR1_SIZE == 0
# line 30000
  #pragma acc parallel firstprivate(arr0) num_gangs(1)
#else
# line 30000
  #pragma acc parallel firstprivate(arr0, arr1) num_gangs(1)
#endif
  for (int j = 0; j < 10; ++j) {
    // Because of firstprivate, arr0's address is different here even when not
    // offloading.
    //             HOST:inside: arr0=0x[[#%x,ARR0_HOST_PTR_IN_KERNEL:]], arr0[0]=30
    //        HOST-NEXT:inside: arr0=0x[[#ARR0_HOST_PTR_IN_KERNEL]], arr0[1]=31
    //        HOST-NEXT:inside: arr0=0x[[#ARR0_HOST_PTR_IN_KERNEL]], arr0[2]=32
    //        HOST-NEXT:inside: arr0=0x[[#ARR0_HOST_PTR_IN_KERNEL]], arr0[3]=33
    //        HOST-NEXT:inside: arr0=0x[[#ARR0_HOST_PTR_IN_KERNEL]], arr0[4]=34
    //        HOST-NEXT:inside: arr0=0x[[#ARR0_HOST_PTR_IN_KERNEL]], arr0[5]=35
    //        HOST-NEXT:inside: arr0=0x[[#ARR0_HOST_PTR_IN_KERNEL]], arr0[6]=36
    //        HOST-NEXT:inside: arr0=0x[[#ARR0_HOST_PTR_IN_KERNEL]], arr0[7]=37
    //        HOST-NEXT:inside: arr0=0x[[#ARR0_HOST_PTR_IN_KERNEL]], arr0[8]=38
    //        HOST-NEXT:inside: arr0=0x[[#ARR0_HOST_PTR_IN_KERNEL]], arr0[9]=39
    //       TGT-x86_64:inside: arr0=0x[[#%x,ARR0_DEVICE_PTR_IN_KERNEL:]], arr0[0]=30
    //  TGT-x86_64-NEXT:inside: arr0=0x[[#ARR0_DEVICE_PTR_IN_KERNEL]], arr0[1]=31
    //  TGT-x86_64-NEXT:inside: arr0=0x[[#ARR0_DEVICE_PTR_IN_KERNEL]], arr0[2]=32
    //  TGT-x86_64-NEXT:inside: arr0=0x[[#ARR0_DEVICE_PTR_IN_KERNEL]], arr0[3]=33
    //  TGT-x86_64-NEXT:inside: arr0=0x[[#ARR0_DEVICE_PTR_IN_KERNEL]], arr0[4]=34
    //  TGT-x86_64-NEXT:inside: arr0=0x[[#ARR0_DEVICE_PTR_IN_KERNEL]], arr0[5]=35
    //  TGT-x86_64-NEXT:inside: arr0=0x[[#ARR0_DEVICE_PTR_IN_KERNEL]], arr0[6]=36
    //  TGT-x86_64-NEXT:inside: arr0=0x[[#ARR0_DEVICE_PTR_IN_KERNEL]], arr0[7]=37
    //  TGT-x86_64-NEXT:inside: arr0=0x[[#ARR0_DEVICE_PTR_IN_KERNEL]], arr0[8]=38
    //  TGT-x86_64-NEXT:inside: arr0=0x[[#ARR0_DEVICE_PTR_IN_KERNEL]], arr0[9]=39
    //      TGT-ppc64le:inside: arr0=0x[[#%x,ARR0_DEVICE_PTR_IN_KERNEL:]], arr0[0]=30
    // TGT-ppc64le-NEXT:inside: arr0=0x[[#ARR0_DEVICE_PTR_IN_KERNEL]], arr0[1]=31
    // TGT-ppc64le-NEXT:inside: arr0=0x[[#ARR0_DEVICE_PTR_IN_KERNEL]], arr0[2]=32
    // TGT-ppc64le-NEXT:inside: arr0=0x[[#ARR0_DEVICE_PTR_IN_KERNEL]], arr0[3]=33
    // TGT-ppc64le-NEXT:inside: arr0=0x[[#ARR0_DEVICE_PTR_IN_KERNEL]], arr0[4]=34
    // TGT-ppc64le-NEXT:inside: arr0=0x[[#ARR0_DEVICE_PTR_IN_KERNEL]], arr0[5]=35
    // TGT-ppc64le-NEXT:inside: arr0=0x[[#ARR0_DEVICE_PTR_IN_KERNEL]], arr0[6]=36
    // TGT-ppc64le-NEXT:inside: arr0=0x[[#ARR0_DEVICE_PTR_IN_KERNEL]], arr0[7]=37
    // TGT-ppc64le-NEXT:inside: arr0=0x[[#ARR0_DEVICE_PTR_IN_KERNEL]], arr0[8]=38
    // TGT-ppc64le-NEXT:inside: arr0=0x[[#ARR0_DEVICE_PTR_IN_KERNEL]], arr0[9]=39
    //
    // We omit nvptx64 here because exit events might trigger before kernel
    // execution due to the use of CUDA streams.
    //
    // FIXME: We omit amdgcn here because it doesn't support target printf yet.
#if !TGT_nvptx64 && !TGT_amdgcn
    printf("inside: arr0=%p, arr0[%d]=%d\n", arr0, j, arr0[j]);
#endif
#line 40000
  }

  //            OFF:acc_ev_exit_data_start
  //       OFF-NEXT:  acc_prof_info
  //       OFF-NEXT:    event_type=12, valid_bytes=72, version=[[VERSION]],
  //       OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  //       OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //       OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //       OFF-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  //       OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //       OFF-NEXT:  acc_other_event_info
  //       OFF-NEXT:    event_type=12, valid_bytes=24,
  //       OFF-NEXT:    parent_construct=acc_construct_parallel,
  //       OFF-NEXT:    implicit=0, tool_info=(nil)
  //       OFF-NEXT:  acc_api_info
  //       OFF-NEXT:    device_api=0, valid_bytes=12,
  //       OFF-NEXT:    device_type=[[ACC_DEVICE]]
  //       OFF-NEXT:acc_ev_delete
  //       OFF-NEXT:  acc_prof_info
  //       OFF-NEXT:    event_type=7, valid_bytes=72, version=[[VERSION]],
  //       OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  //       OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //       OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //       OFF-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  //       OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //       OFF-NEXT:  acc_data_event_info
  //       OFF-NEXT:    event_type=7, valid_bytes=56,
  //       OFF-NEXT:    parent_construct=acc_construct_parallel,
  //       OFF-NEXT:    implicit=0, tool_info=(nil),
  //       OFF-NEXT:    var_name=[[PACK0_NAME]], bytes=[[#PACK0_SIZE]],
  //       OFF-NEXT:    host_ptr=0x[[#PACK0_HOST_PTR]],
  //       OFF-NEXT:    device_ptr=0x[[#PACK0_DEVICE_PTR]]
  //       OFF-NEXT:  acc_api_info
  //       OFF-NEXT:    device_api=0, valid_bytes=12,
  //       OFF-NEXT:    device_type=[[ACC_DEVICE]]
  //       OFF-NEXT:acc_ev_free
  //       OFF-NEXT:  acc_prof_info
  //       OFF-NEXT:    event_type=9, valid_bytes=72, version=[[VERSION]],
  //       OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  //       OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //       OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //       OFF-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  //       OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //       OFF-NEXT:  acc_data_event_info
  //       OFF-NEXT:    event_type=9, valid_bytes=56,
  //       OFF-NEXT:    parent_construct=acc_construct_parallel,
  //       OFF-NEXT:    implicit=0, tool_info=(nil),
  //       OFF-NEXT:    var_name=[[PACK0_NAME]], bytes=[[#PACK0_SIZE]],
  //       OFF-NEXT:    host_ptr=0x[[#PACK0_HOST_PTR]],
  //       OFF-NEXT:    device_ptr=0x[[#PACK0_DEVICE_PTR]]
  //       OFF-NEXT:  acc_api_info
  //       OFF-NEXT:    device_api=0, valid_bytes=12,
  //       OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-PACK1-NEXT:acc_ev_delete
  // OFF-PACK1-NEXT:  acc_prof_info
  // OFF-PACK1-NEXT:    event_type=7, valid_bytes=72, version=[[VERSION]],
  // OFF-PACK1-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-PACK1-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-PACK1-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-PACK1-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  // OFF-PACK1-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-PACK1-NEXT:  acc_data_event_info
  // OFF-PACK1-NEXT:    event_type=7, valid_bytes=56,
  // OFF-PACK1-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-PACK1-NEXT:    implicit=0, tool_info=(nil),
  // OFF-PACK1-NEXT:    var_name=[[PACK1_NAME]], bytes=[[#PACK1_SIZE]],
  // OFF-PACK1-NEXT:    host_ptr=0x[[#PACK1_HOST_PTR]],
  // OFF-PACK1-NEXT:    device_ptr=0x[[#ARR1_DEVICE_PTR]]
  // OFF-PACK1-NEXT:  acc_api_info
  // OFF-PACK1-NEXT:    device_api=0, valid_bytes=12,
  // OFF-PACK1-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-PACK1-NEXT:acc_ev_free
  // OFF-PACK1-NEXT:  acc_prof_info
  // OFF-PACK1-NEXT:    event_type=9, valid_bytes=72, version=[[VERSION]],
  // OFF-PACK1-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-PACK1-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-PACK1-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  // OFF-PACK1-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  // OFF-PACK1-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-PACK1-NEXT:  acc_data_event_info
  // OFF-PACK1-NEXT:    event_type=9, valid_bytes=56,
  // OFF-PACK1-NEXT:    parent_construct=acc_construct_parallel,
  // OFF-PACK1-NEXT:    implicit=0, tool_info=(nil),
  // OFF-PACK1-NEXT:    var_name=[[PACK1_NAME]], bytes=[[#PACK1_SIZE]],
  // OFF-PACK1-NEXT:    host_ptr=0x[[#PACK1_HOST_PTR]],
  // OFF-PACK1-NEXT:    device_ptr=0x[[#ARR1_DEVICE_PTR]]
  // OFF-PACK1-NEXT:  acc_api_info
  // OFF-PACK1-NEXT:    device_api=0, valid_bytes=12,
  // OFF-PACK1-NEXT:    device_type=[[ACC_DEVICE]]
  //       OFF-NEXT:acc_ev_exit_data_end
  //       OFF-NEXT:  acc_prof_info
  //       OFF-NEXT:    event_type=13, valid_bytes=72, version=[[VERSION]],
  //       OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  //       OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //       OFF-NEXT:    src_file=[[SRC_FILE]], func_name=main,
  //       OFF-NEXT:    line_no=[[LINE_NO1]], end_line_no=[[END_LINE_NO1]],
  //       OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //       OFF-NEXT:  acc_other_event_info
  //       OFF-NEXT:    event_type=13, valid_bytes=24,
  //       OFF-NEXT:    parent_construct=acc_construct_parallel,
  //       OFF-NEXT:    implicit=0, tool_info=(nil)
  //       OFF-NEXT:  acc_api_info
  //       OFF-NEXT:    device_api=0, valid_bytes=12,
  //       OFF-NEXT:    device_type=[[ACC_DEVICE]]

  // CHECK:acc_ev_compute_construct_end

  // CHECK:after kernels
  printf("after kernels\n");

  return 0;
#line 50000
}

//   OFF:acc_ev_device_shutdown_start
//   OFF:acc_ev_device_shutdown_end
// CHECK:acc_ev_runtime_shutdown
