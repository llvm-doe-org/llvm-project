// Check that all callbacks are dispatched in the correct order with the
// correct data for the OpenACC Runtime Library API's data and memory management
// routines.
//
// RUN: %data tgts {
// RUN:   (run-if=                cflags=                                     host-or-off=HOST)
// RUN:   (run-if=%run-if-x86_64  cflags=-fopenmp-targets=%run-x86_64-triple  host-or-off=OFF )
// RUN:   (run-if=%run-if-ppc64le cflags=-fopenmp-targets=%run-ppc64le-triple host-or-off=OFF )
// RUN:   (run-if=%run-if-nvptx64 cflags=-fopenmp-targets=%run-nvptx64-triple host-or-off=OFF )
// RUN: }
// RUN: %data run-envs {
// RUN:   (run-env=                                  env-fc=%[host-or-off],%[host-or-off]-BEFORE-ENV)
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled' env-fc=HOST                                    )
// RUN: }
// RUN: %for tgts {
// RUN:   %[run-if] %clang -Xclang -verify -fopenacc %acc-includes %s \
// RUN:                    %[cflags] -o %t
// RUN:   %for run-envs {
// RUN:     %[run-if] %[run-env] %t > %t.out 2> %t.err
// RUN:     %[run-if] FileCheck -input-file %t.err %s \
// RUN:         -allow-empty -check-prefixes=ERR
// RUN:     %[run-if] FileCheck -input-file %t.out %s \
// RUN:         -match-full-lines -strict-whitespace \
// RUN:         -check-prefixes=CHECK,%[env-fc] \
// RUN:         -DVERSION=%acc-version -DHOST_DEV=%acc-host-dev \
// RUN:         -DOFF_DEV=0 -DTHREAD_ID=0 -DASYNC_QUEUE=-1
// RUN:   }
// RUN: }
//
// END.

// expected-no-diagnostics

#include "callbacks.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

void acc_register_library(acc_prof_reg reg, acc_prof_reg unreg,
                          acc_prof_lookup lookup) {
  // It's important to register all callbacks so we don't miss events, such as
  // data movements, that shouldn't occur.
  register_all_callbacks(reg);
}

// CHECK-NOT:{{.}}
// ERR-NOT:{{.}}

int main() {
  int arr[2] = {10, 11};

  // CHECK:arr host ptr = [[ARR_HOST_PTR:0x[a-z0-9]+]]
  printf("arr host ptr = %p\n", arr);

  // OFF-NEXT:acc_ev_device_init_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=1, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_malloc,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=1, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  // OFF-NEXT:acc_ev_device_init_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=2, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_malloc,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=2, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host

  // CHECK-NEXT:acc_ev_alloc
  // CHECK-NEXT:  acc_prof_info
  // CHECK-NEXT:    event_type=8, valid_bytes=72, version=[[VERSION]],
  //  HOST-NEXT:    device_type=acc_device_host, device_number=[[HOST_DEV]],
  //   OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // CHECK-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // CHECK-NEXT:    src_file=(null), func_name=acc_malloc,
  // CHECK-NEXT:    line_no=0, end_line_no=0,
  // CHECK-NEXT:    func_line_no=0, func_end_line_no=0
  // CHECK-NEXT:  acc_data_event_info
  // CHECK-NEXT:    event_type=8, valid_bytes=56,
  // CHECK-NEXT:    parent_construct=acc_construct_runtime_api,
  // CHECK-NEXT:    implicit=0, tool_info=(nil),
  // CHECK-NEXT:    var_name=(null), bytes=8,
  // CHECK-NEXT:    host_ptr=(null),
  // CHECK-NEXT:    device_ptr=[[ARR_DEVICE_PTR:0x[a-z0-9]+]]
  // CHECK-NEXT:  acc_api_info
  // CHECK-NEXT:    device_api=0, valid_bytes=12,
  //  HOST-NEXT:    device_type=acc_device_host
  //   OFF-NEXT:    device_type=acc_device_not_host
  int *arr_dev = acc_malloc(sizeof arr);
  if (!arr_dev) {
    fprintf(stderr, "acc_malloc failed\n");
    return 1;
  }
  // CHECK-NEXT:arr device ptr = [[ARR_DEVICE_PTR]]
  printf("arr device ptr = %p\n", arr_dev);

  // OFF-NEXT:acc_ev_create
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=6, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_map_data,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=6, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=[[ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=[[ARR_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  acc_map_data(arr, arr_dev, sizeof arr);

  // OFF-NEXT:acc_ev_delete
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=7, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_unmap_data,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=7, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=[[ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=[[ARR_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=acc_device_not_host
  acc_unmap_data(arr);

  // CHECK-NEXT:acc_ev_free
  // CHECK-NEXT:  acc_prof_info
  // CHECK-NEXT:    event_type=9, valid_bytes=72, version=[[VERSION]],
  //  HOST-NEXT:    device_type=acc_device_host, device_number=[[HOST_DEV]],
  //   OFF-NEXT:    device_type=acc_device_not_host, device_number=[[OFF_DEV]],
  // CHECK-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // CHECK-NEXT:    src_file=(null), func_name=acc_free,
  // CHECK-NEXT:    line_no=0, end_line_no=0,
  // CHECK-NEXT:    func_line_no=0, func_end_line_no=0
  // CHECK-NEXT:  acc_data_event_info
  // CHECK-NEXT:    event_type=9, valid_bytes=56,
  // CHECK-NEXT:    parent_construct=acc_construct_runtime_api,
  // CHECK-NEXT:    implicit=0, tool_info=(nil),
  // CHECK-NEXT:    var_name=(null), bytes=8,
  // CHECK-NEXT:    host_ptr=(null),
  // CHECK-NEXT:    device_ptr=[[ARR_DEVICE_PTR]]
  // CHECK-NEXT:  acc_api_info
  // CHECK-NEXT:    device_api=0, valid_bytes=12,
  //  HOST-NEXT:    device_type=acc_device_host
  //   OFF-NEXT:    device_type=acc_device_not_host
  acc_free(arr_dev);

  return 0;
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

// CHECK-NOT: {{.}}
// ERR-NOT: {{.}}
