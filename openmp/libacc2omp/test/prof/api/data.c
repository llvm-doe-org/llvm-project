// Check that all callbacks are dispatched in the correct order with the
// correct data for the OpenACC Runtime Library API's data and memory management
// routines.

// RUN: %clang-acc -o %t.exe %s

// DEFINE: %{check}( RUN_ENV %, HOST_OR_OFF %, COPY %) =                       \
// DEFINE:   %{RUN_ENV} %t.exe %{HOST_OR_OFF} > %t.out 2> %t.err &&            \
// DEFINE:   FileCheck -input-file %t.err -allow-empty -check-prefixes=ERR     \
// DEFINE:             %s &&                                                   \
// DEFINE:   FileCheck -input-file %t.out %s                                   \
// DEFINE:     -match-full-lines -strict-whitespace                            \
// DEFINE:     -implicit-check-not=acc_ev_                                     \
// DEFINE:     -check-prefixes=CHECK,%{HOST_OR_OFF},%{COPY}                    \
// DEFINE:     -DACC_DEVICE=acc_device_%dev-type-0-acc -DVERSION=%acc-version  \
// DEFINE:     -DTHREAD_ID=0 -DASYNC_QUEUE=-1

// RUN: %{check}(                                 %, %if-host<HOST|OFF> %, COPY-%dev-type-0-copy %)
// RUN: %{check}( env OMP_TARGET_OFFLOAD=disabled %, HOST               %, COPY-direct           %)
// RUN: %{check}( env ACC_DEVICE_TYPE=host        %, HOST               %, COPY-direct           %)
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

// CHECK-NOT:{{.}}
// ERR-NOT:{{.}}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "expected one argument\n");
    return 1;
  }
  bool Offloading;
  if (!strcmp(argv[1], "HOST"))
    Offloading = false;
  else if (!strcmp(argv[1], "OFF"))
    Offloading = true;
  else {
    fprintf(stderr, "invalid argument: %s\n", argv[1]);
    return 1;
  }

  // Put some info about the devices in the output to help with debugging.
  //
  //      CHECK:devTypeInit = acc_device_{{.*}}
  // CHECK-NEXT:numDevsOfTypeInit = [[#]]
  // CHECK-NEXT:devNumInit = [[OFF_DEV:[0-9]+]]
  // CHECK-NEXT:devNumOther = [[OFF_DEV_OTHER:[0-9]+]]
  //
  // The purpose of devNumOther is to enable testing acc_memcpy_d2d with
  // multiple devices (necessarily of the same type) when we have them.
  // When devNumInit = devNumOther (always true when not offloading), we
  // don't, so we just fake the expected results to make FileCheck happy.
  // TODO: Testing would be cleaner and more careful if we knew how many
  // devices we have before starting the test.
  acc_device_t devTypeInit = acc_get_device_type();
  int numDevsOfTypeInit = acc_get_num_devices(devTypeInit);
  int devNumInit = acc_get_device_num(devTypeInit);
  int devNumOther = (devNumInit + 1) % numDevsOfTypeInit;
  printf("devTypeInit = %s\n", deviceTypeToStr(devTypeInit));
  printf("numDevsOfTypeInit = %d\n", numDevsOfTypeInit);
  printf("devNumInit = %d\n", devNumInit);
  printf("devNumOther = %d\n", devNumOther);
  fflush(stdout);

  // CHECK-NEXT:arr host ptr = 0x[[#%x,ARR_HOST_PTR:]]
  // CHECK-NEXT:arr element size = [[#%u,ARR_ELE_SIZE:]]
  int arr[2] = {10, 11};
  printf("arr host ptr = %p\n", arr);
  printf("arr element size = %zu\n", sizeof *arr);

  //--------------------------------------------------
  // Check acc_malloc, acc_map_data, acc_unmap_data, and acc_free.  acc_map_data
  // and acc_unmap_data fail for shared memory, so skip them when offloading is
  // disabled.
  //
  // Check enter/exit-data-like routines (acc_copyin, acc_copyout, etc.) when
  // data is present and not ready to be deleted (a case we exercise here by
  // calling them between acc_map_data and acc_unmap_data).
  //--------------------------------------------------

  // OFF-NEXT:acc_ev_device_init_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=1, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
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
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_device_init_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=2, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
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
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]

  // CHECK-NEXT:acc_ev_alloc
  // CHECK-NEXT:  acc_prof_info
  // CHECK-NEXT:    event_type=8, valid_bytes=72, version=[[VERSION]],
  //  HOST-NEXT:    device_type=acc_device_host, device_number=0,
  //   OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
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
  // CHECK-NEXT:    device_ptr=0x[[#%x,ARR_DEV:]]
  // CHECK-NEXT:  acc_api_info
  // CHECK-NEXT:    device_api=0, valid_bytes=12,
  //  HOST-NEXT:    device_type=acc_device_host
  //   OFF-NEXT:    device_type=[[ACC_DEVICE]]
  int *arr_dev = acc_malloc(sizeof arr);
  if (!arr_dev) {
    fprintf(stderr, "acc_malloc failed\n");
    return 1;
  }
  // CHECK-NEXT:arr device ptr = 0x[[#ARR_DEV]]
  printf("arr device ptr = %p\n", arr_dev);

  // OFF-NEXT:acc_ev_create
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=6, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_map_data,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=6, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#ARR_DEV]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  if (Offloading)
    acc_map_data(arr, arr_dev, sizeof arr);

  // OFF-NEXT:acc_ev_enter_data_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=10, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyin,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=10, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_enter_data_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=11, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyin,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=11, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  acc_copyin(arr, sizeof arr);

  // OFF-NEXT:acc_ev_enter_data_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=10, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_create,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=10, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_enter_data_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=11, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_create,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=11, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  acc_create(arr, sizeof arr);

  // OFF-NEXT:acc_ev_exit_data_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=12, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyout,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=12, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_exit_data_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=13, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyout,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=13, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  acc_copyout(arr, sizeof arr);

  // OFF-NEXT:acc_ev_exit_data_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=12, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyout_finalize,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=12, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_exit_data_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=13, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyout_finalize,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=13, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  acc_copyout_finalize(arr, sizeof arr);

  // OFF-NEXT:acc_ev_exit_data_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=12, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_delete,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=12, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_exit_data_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=13, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_delete,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=13, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  acc_delete(arr, sizeof arr);

  // OFF-NEXT:acc_ev_exit_data_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=12, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_delete_finalize,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=12, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_exit_data_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=13, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_delete_finalize,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=13, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  acc_delete_finalize(arr, sizeof arr);

  // OFF-NEXT:acc_ev_delete
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=7, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_unmap_data,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=7, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#ARR_DEV]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  if (Offloading)
    acc_unmap_data(arr);

  // CHECK-NEXT:acc_ev_free
  // CHECK-NEXT:  acc_prof_info
  // CHECK-NEXT:    event_type=9, valid_bytes=72, version=[[VERSION]],
  //  HOST-NEXT:    device_type=acc_device_host, device_number=0,
  //   OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
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
  // CHECK-NEXT:    device_ptr=0x[[#ARR_DEV]]
  // CHECK-NEXT:  acc_api_info
  // CHECK-NEXT:    device_api=0, valid_bytes=12,
  //  HOST-NEXT:    device_type=acc_device_host
  //   OFF-NEXT:    device_type=[[ACC_DEVICE]]
  acc_free(arr_dev);

  //--------------------------------------------------
  // Check exit-data-like routines (acc_copyout, etc.) when data is not present
  // and so there's nothing to delete.
  //--------------------------------------------------

  // OFF-NEXT:acc_ev_exit_data_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=12, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyout,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=12, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_exit_data_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=13, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyout,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=13, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  acc_copyout(arr, sizeof arr);

  // OFF-NEXT:acc_ev_exit_data_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=12, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyout_finalize,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=12, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_exit_data_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=13, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyout_finalize,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=13, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  acc_copyout_finalize(arr, sizeof arr);

  // OFF-NEXT:acc_ev_exit_data_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=12, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_delete,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=12, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_exit_data_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=13, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_delete,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=13, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  acc_delete(arr, sizeof arr);

  // OFF-NEXT:acc_ev_exit_data_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=12, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_delete_finalize,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=12, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_exit_data_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=13, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_delete_finalize,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=13, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  acc_delete_finalize(arr, sizeof arr);

  //--------------------------------------------------
  // Check enter-data-like routines (acc_copyin, etc.) when data is not present
  // and so data is ready to be created, and check exit-data-like routines
  // (acc_copyout, etc.) when data is present and ready to be deleted.
  //--------------------------------------------------

  // OFF-NEXT:acc_ev_enter_data_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=10, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyin,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=10, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_alloc
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=8, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyin,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=8, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#%x,ARR_DEVICE_PTR:]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_create
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=6, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyin,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=6, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#ARR_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_enqueue_upload_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=20, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyin,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=20, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#ARR_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_enqueue_upload_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=21, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyin,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=21, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#ARR_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_enter_data_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=11, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyin,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=11, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  acc_copyin(arr, sizeof arr);

  // OFF-NEXT:acc_ev_exit_data_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=12, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyout,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=12, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_enqueue_download_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=22, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyout,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=22, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#ARR_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_enqueue_download_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=23, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyout,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=23, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#ARR_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_delete
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=7, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyout,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=7, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#ARR_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_free
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=9, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyout,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=9, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#ARR_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_exit_data_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=13, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyout,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=13, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  acc_copyout(arr, sizeof arr);

  // OFF-NEXT:acc_ev_enter_data_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=10, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_create,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=10, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_alloc
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=8, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_create,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=8, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#%x,ARR_DEVICE_PTR:]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_create
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=6, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_create,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=6, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#ARR_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_enter_data_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=11, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_create,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=11, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  acc_create(arr, sizeof arr);

  // OFF-NEXT:acc_ev_exit_data_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=12, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_delete,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=12, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_delete
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=7, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_delete,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=7, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#ARR_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_free
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=9, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_delete,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=9, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#ARR_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_exit_data_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=13, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_delete,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=13, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  acc_delete(arr, sizeof arr);

  // We've already checked acc_create when data is not present, but we need to
  // make it present again.
  //
  // OFF:acc_ev_enter_data_start
  // OFF:acc_ev_alloc
  // OFF:  acc_data_event_info
  // OFF:    device_ptr=0x[[#%x,ARR_DEVICE_PTR:]]
  // OFF:acc_ev_create
  // OFF:acc_ev_enter_data_end
  acc_create(arr, sizeof arr);

  //      OFF:acc_ev_exit_data_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=12, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyout_finalize,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=12, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_enqueue_download_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=22, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyout_finalize,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=22, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#ARR_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_enqueue_download_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=23, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyout_finalize,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=23, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#ARR_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_delete
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=7, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyout_finalize,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=7, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#ARR_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_free
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=9, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyout_finalize,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=9, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#ARR_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_exit_data_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=13, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_copyout_finalize,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=13, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  acc_copyout_finalize(arr, sizeof arr);

  // We've already checked acc_create when data is not present, but we need to
  // make it present again.
  //
  // OFF:acc_ev_enter_data_start
  // OFF:acc_ev_alloc
  // OFF:  acc_data_event_info
  // OFF:    device_ptr=0x[[#%x,ARR_DEVICE_PTR:]]
  // OFF:acc_ev_create
  // OFF:acc_ev_enter_data_end
  acc_create(arr, sizeof arr);

  //      OFF:acc_ev_exit_data_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=12, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_delete_finalize,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=12, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_delete
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=7, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_delete_finalize,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=7, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#ARR_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_free
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=9, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_delete_finalize,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=9, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#ARR_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_exit_data_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=13, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_delete_finalize,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=13, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  acc_delete_finalize(arr, sizeof arr);

  //--------------------------------------------------
  // Check update routines when data is present.
  //--------------------------------------------------

  // We've already checked acc_create when data is not present, but we need to
  // make it present again.
  //
  // OFF:acc_ev_enter_data_start
  // OFF:acc_ev_alloc
  // OFF:  acc_data_event_info
  // OFF:    device_ptr=0x[[#%x,ARR_DEVICE_PTR:]]
  // OFF:acc_ev_create
  // OFF:acc_ev_enter_data_end
  acc_create(arr, sizeof arr);

  //      OFF:acc_ev_update_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=14, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_update_device,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=14, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_enqueue_upload_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=20, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_update_device,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=20, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#ARR_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_enqueue_upload_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=21, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_update_device,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=21, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#ARR_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_update_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=15, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_update_device,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=15, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  acc_update_device(arr, sizeof arr);

  //      OFF:acc_ev_update_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=14, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_update_self,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=14, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_enqueue_download_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=22, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_update_self,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=22, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#ARR_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_enqueue_download_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=23, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_update_self,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=23, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#ARR_DEVICE_PTR]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_update_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=15, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_update_self,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=15, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  acc_update_self(arr, sizeof arr);

  // We've already checked acc_delete when data is present, but we want to make
  // it absent again.
  //
  // OFF:acc_ev_exit_data_start
  // OFF:acc_ev_delete
  // OFF:acc_ev_free
  // OFF:acc_ev_exit_data_end
  acc_delete(arr, sizeof arr);

  //--------------------------------------------------
  // Check memcpy routines.
  //--------------------------------------------------

  // We've already checked acc_malloc, but we need to allocate again.
  //
  // CHECK:acc_ev_alloc
  // CHECK:  acc_data_event_info
  // CHECK:    device_ptr=0x[[#%x,ARR_DEV:]]
  arr_dev = acc_malloc(sizeof arr);
  if (!arr_dev) {
    fprintf(stderr, "acc_malloc failed\n");
    return 1;
  }

  //      OFF:acc_ev_enqueue_upload_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=20, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_memcpy_to_device,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=20, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#ARR_DEV]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_enqueue_upload_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=21, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_memcpy_to_device,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=21, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#ARR_DEV]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  acc_memcpy_to_device(arr_dev, arr, sizeof arr);

  // OFF-NEXT:acc_ev_enqueue_download_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=22, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_memcpy_from_device,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=22, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#ARR_DEV]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_enqueue_download_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=23, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=(null), func_name=acc_memcpy_from_device,
  // OFF-NEXT:    line_no=0, end_line_no=0,
  // OFF-NEXT:    func_line_no=0, func_end_line_no=0
  // OFF-NEXT:  acc_data_event_info
  // OFF-NEXT:    event_type=23, valid_bytes=56,
  // OFF-NEXT:    parent_construct=acc_construct_runtime_api,
  // OFF-NEXT:    implicit=0, tool_info=(nil),
  // OFF-NEXT:    var_name=(null), bytes=8,
  // OFF-NEXT:    host_ptr=0x[[#ARR_HOST_PTR]],
  // OFF-NEXT:    device_ptr=0x[[#ARR_DEV]]
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  acc_memcpy_from_device(arr, arr_dev, sizeof arr);

  // COPY-by-host-NEXT:acc_ev_enqueue_download_start
  // COPY-by-host-NEXT:  acc_prof_info
  // COPY-by-host-NEXT:    event_type=22, valid_bytes=72, version=[[VERSION]],
  // COPY-by-host-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // COPY-by-host-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // COPY-by-host-NEXT:    src_file=(null), func_name=acc_memcpy_device,
  // COPY-by-host-NEXT:    line_no=0, end_line_no=0,
  // COPY-by-host-NEXT:    func_line_no=0, func_end_line_no=0
  // COPY-by-host-NEXT:  acc_data_event_info
  // COPY-by-host-NEXT:    event_type=22, valid_bytes=56,
  // COPY-by-host-NEXT:    parent_construct=acc_construct_runtime_api,
  // COPY-by-host-NEXT:    implicit=0, tool_info=(nil),
  // COPY-by-host-NEXT:    var_name=(null), bytes=4,
  // COPY-by-host-NEXT:    host_ptr=0x[[#%x,TMP_HOST_PTR:]],
  // COPY-by-host-NEXT:    device_ptr=0x[[#%x,ARR_DEV + ARR_ELE_SIZE]]
  // COPY-by-host-NEXT:  acc_api_info
  // COPY-by-host-NEXT:    device_api=0, valid_bytes=12,
  // COPY-by-host-NEXT:    device_type=[[ACC_DEVICE]]
  // COPY-by-host-NEXT:acc_ev_enqueue_download_end
  // COPY-by-host-NEXT:  acc_prof_info
  // COPY-by-host-NEXT:    event_type=23, valid_bytes=72, version=[[VERSION]],
  // COPY-by-host-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // COPY-by-host-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // COPY-by-host-NEXT:    src_file=(null), func_name=acc_memcpy_device,
  // COPY-by-host-NEXT:    line_no=0, end_line_no=0,
  // COPY-by-host-NEXT:    func_line_no=0, func_end_line_no=0
  // COPY-by-host-NEXT:  acc_data_event_info
  // COPY-by-host-NEXT:    event_type=23, valid_bytes=56,
  // COPY-by-host-NEXT:    parent_construct=acc_construct_runtime_api,
  // COPY-by-host-NEXT:    implicit=0, tool_info=(nil),
  // COPY-by-host-NEXT:    var_name=(null), bytes=4,
  // COPY-by-host-NEXT:    host_ptr=0x[[#TMP_HOST_PTR]],
  // COPY-by-host-NEXT:    device_ptr=0x[[#%x,ARR_DEV + ARR_ELE_SIZE]]
  // COPY-by-host-NEXT:  acc_api_info
  // COPY-by-host-NEXT:    device_api=0, valid_bytes=12,
  // COPY-by-host-NEXT:    device_type=[[ACC_DEVICE]]
  // COPY-by-host-NEXT:acc_ev_enqueue_upload_start
  // COPY-by-host-NEXT:  acc_prof_info
  // COPY-by-host-NEXT:    event_type=20, valid_bytes=72, version=[[VERSION]],
  // COPY-by-host-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // COPY-by-host-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // COPY-by-host-NEXT:    src_file=(null), func_name=acc_memcpy_device,
  // COPY-by-host-NEXT:    line_no=0, end_line_no=0,
  // COPY-by-host-NEXT:    func_line_no=0, func_end_line_no=0
  // COPY-by-host-NEXT:  acc_data_event_info
  // COPY-by-host-NEXT:    event_type=20, valid_bytes=56,
  // COPY-by-host-NEXT:    parent_construct=acc_construct_runtime_api,
  // COPY-by-host-NEXT:    implicit=0, tool_info=(nil),
  // COPY-by-host-NEXT:    var_name=(null), bytes=4,
  // COPY-by-host-NEXT:    host_ptr=0x[[#TMP_HOST_PTR]],
  // COPY-by-host-NEXT:    device_ptr=0x[[#%x,ARR_DEV]]
  // COPY-by-host-NEXT:  acc_api_info
  // COPY-by-host-NEXT:    device_api=0, valid_bytes=12,
  // COPY-by-host-NEXT:    device_type=[[ACC_DEVICE]]
  // COPY-by-host-NEXT:acc_ev_enqueue_upload_end
  // COPY-by-host-NEXT:  acc_prof_info
  // COPY-by-host-NEXT:    event_type=21, valid_bytes=72, version=[[VERSION]],
  // COPY-by-host-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // COPY-by-host-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // COPY-by-host-NEXT:    src_file=(null), func_name=acc_memcpy_device,
  // COPY-by-host-NEXT:    line_no=0, end_line_no=0,
  // COPY-by-host-NEXT:    func_line_no=0, func_end_line_no=0
  // COPY-by-host-NEXT:  acc_data_event_info
  // COPY-by-host-NEXT:    event_type=21, valid_bytes=56,
  // COPY-by-host-NEXT:    parent_construct=acc_construct_runtime_api,
  // COPY-by-host-NEXT:    implicit=0, tool_info=(nil),
  // COPY-by-host-NEXT:    var_name=(null), bytes=4,
  // COPY-by-host-NEXT:    host_ptr=0x[[#TMP_HOST_PTR]],
  // COPY-by-host-NEXT:    device_ptr=0x[[#%x,ARR_DEV]]
  // COPY-by-host-NEXT:  acc_api_info
  // COPY-by-host-NEXT:    device_api=0, valid_bytes=12,
  // COPY-by-host-NEXT:    device_type=[[ACC_DEVICE]]
  acc_memcpy_device(&arr_dev[0], &arr_dev[1], sizeof *arr_dev);

  // No-ops.  There's not much point in trying no-op cases that are specific to
  // shared memory, which generally has no events, as checked above.
  acc_memcpy_to_device(arr_dev, arr, 0);
  acc_memcpy_from_device(arr_dev, arr, 0);
  acc_memcpy_device(&arr_dev[0], &arr_dev[1], 0);
  acc_memcpy_d2d(&arr[0], &arr[1], 0, devNumInit, devNumInit);
  acc_memcpy_device(&arr_dev[0], &arr_dev[0], sizeof *arr_dev);
  acc_memcpy_d2d(&arr[0], &arr[0], sizeof *arr, devNumInit, devNumInit);
  acc_set_device_type(acc_device_host);
  acc_memcpy_d2d(&arr[0], &arr[0], sizeof *arr, 0, 0);
  acc_set_device_num(devNumInit, devTypeInit);

  // We've already checked acc_free, but we want to free again.
  //
  // CHECK:acc_ev_free
  acc_free(arr_dev);

  // We've already checked acc_create when data is not present, but we need to
  // make it present again.
  //
  // OFF:acc_ev_enter_data_start
  // OFF:acc_ev_alloc
  // OFF:  acc_data_event_info
  // OFF:    device_ptr=0x[[#%x,ARR_DEVICE_PTR:]]
  // OFF:acc_ev_create
  // OFF:acc_ev_enter_data_end
  // OFF:acc_ev_device_init_start
  // OFF:  acc_prof_info
  // OFF:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV_OTHER]],
  // OFF:    src_file=(null), func_name=acc_create,
  // OFF:acc_ev_device_init_end
  // OFF:  acc_prof_info
  // OFF:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV_OTHER]],
  // OFF:    src_file=(null), func_name=acc_create,
  // OFF:acc_ev_enter_data_start
  // OFF:acc_ev_alloc
  // OFF:  acc_data_event_info
  // OFF:    device_ptr=0x[[#%x,ARR_DEVICE_OTHER_PTR:]]
  // OFF:acc_ev_create
  // OFF:acc_ev_enter_data_end
  acc_create(arr, sizeof arr);
  if (devNumInit != devNumOther) {
    acc_set_device_num(devNumOther, devTypeInit);
    acc_create(arr, sizeof arr);
    acc_set_device_num(devNumInit, devTypeInit);
  } else if (Offloading) {
    printf("acc_ev_device_init_start\n"
           "  acc_prof_info\n"
           "    device_type=%s, device_number=%d,\n"
           "    src_file=(null), func_name=acc_create,\n"
           "acc_ev_device_init_end\n"
           "  acc_prof_info\n"
           "    device_type=%s, device_number=%d,\n"
           "    src_file=(null), func_name=acc_create,\n"
           "acc_ev_enter_data_start\n"
           "acc_ev_alloc\n"
           "  acc_data_event_info\n"
           "    device_ptr=%p\n"
           "acc_ev_create\n"
           "acc_ev_enter_data_end\n",
           deviceTypeToStr(devTypeInit), devNumOther,
           deviceTypeToStr(devTypeInit), devNumOther, acc_deviceptr(arr));
  }

  //      COPY-by-host:acc_ev_enqueue_download_start
  // COPY-by-host-NEXT:  acc_prof_info
  // COPY-by-host-NEXT:    event_type=22, valid_bytes=72, version=[[VERSION]],
  // COPY-by-host-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // COPY-by-host-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // COPY-by-host-NEXT:    src_file=(null), func_name=acc_memcpy_d2d,
  // COPY-by-host-NEXT:    line_no=0, end_line_no=0,
  // COPY-by-host-NEXT:    func_line_no=0, func_end_line_no=0
  // COPY-by-host-NEXT:  acc_data_event_info
  // COPY-by-host-NEXT:    event_type=22, valid_bytes=56,
  // COPY-by-host-NEXT:    parent_construct=acc_construct_runtime_api,
  // COPY-by-host-NEXT:    implicit=0, tool_info=(nil),
  // COPY-by-host-NEXT:    var_name=(null), bytes=4,
  // COPY-by-host-NEXT:    host_ptr=0x[[#%x,TMP_HOST_PTR:]],
  // COPY-by-host-NEXT:    device_ptr=0x[[#%x,ARR_DEVICE_PTR + ARR_ELE_SIZE]]
  // COPY-by-host-NEXT:  acc_api_info
  // COPY-by-host-NEXT:    device_api=0, valid_bytes=12,
  // COPY-by-host-NEXT:    device_type=[[ACC_DEVICE]]
  // COPY-by-host-NEXT:acc_ev_enqueue_download_end
  // COPY-by-host-NEXT:  acc_prof_info
  // COPY-by-host-NEXT:    event_type=23, valid_bytes=72, version=[[VERSION]],
  // COPY-by-host-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // COPY-by-host-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // COPY-by-host-NEXT:    src_file=(null), func_name=acc_memcpy_d2d,
  // COPY-by-host-NEXT:    line_no=0, end_line_no=0,
  // COPY-by-host-NEXT:    func_line_no=0, func_end_line_no=0
  // COPY-by-host-NEXT:  acc_data_event_info
  // COPY-by-host-NEXT:    event_type=23, valid_bytes=56,
  // COPY-by-host-NEXT:    parent_construct=acc_construct_runtime_api,
  // COPY-by-host-NEXT:    implicit=0, tool_info=(nil),
  // COPY-by-host-NEXT:    var_name=(null), bytes=4,
  // COPY-by-host-NEXT:    host_ptr=0x[[#TMP_HOST_PTR]],
  // COPY-by-host-NEXT:    device_ptr=0x[[#%x,ARR_DEVICE_PTR + ARR_ELE_SIZE]]
  // COPY-by-host-NEXT:  acc_api_info
  // COPY-by-host-NEXT:    device_api=0, valid_bytes=12,
  // COPY-by-host-NEXT:    device_type=[[ACC_DEVICE]]
  // COPY-by-host-NEXT:acc_ev_enqueue_upload_start
  // COPY-by-host-NEXT:  acc_prof_info
  // COPY-by-host-NEXT:    event_type=20, valid_bytes=72, version=[[VERSION]],
  // COPY-by-host-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // COPY-by-host-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // COPY-by-host-NEXT:    src_file=(null), func_name=acc_memcpy_d2d,
  // COPY-by-host-NEXT:    line_no=0, end_line_no=0,
  // COPY-by-host-NEXT:    func_line_no=0, func_end_line_no=0
  // COPY-by-host-NEXT:  acc_data_event_info
  // COPY-by-host-NEXT:    event_type=20, valid_bytes=56,
  // COPY-by-host-NEXT:    parent_construct=acc_construct_runtime_api,
  // COPY-by-host-NEXT:    implicit=0, tool_info=(nil),
  // COPY-by-host-NEXT:    var_name=(null), bytes=4,
  // COPY-by-host-NEXT:    host_ptr=0x[[#TMP_HOST_PTR]],
  // COPY-by-host-NEXT:    device_ptr=0x[[#%x,ARR_DEVICE_PTR]]
  // COPY-by-host-NEXT:  acc_api_info
  // COPY-by-host-NEXT:    device_api=0, valid_bytes=12,
  // COPY-by-host-NEXT:    device_type=[[ACC_DEVICE]]
  // COPY-by-host-NEXT:acc_ev_enqueue_upload_end
  // COPY-by-host-NEXT:  acc_prof_info
  // COPY-by-host-NEXT:    event_type=21, valid_bytes=72, version=[[VERSION]],
  // COPY-by-host-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // COPY-by-host-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // COPY-by-host-NEXT:    src_file=(null), func_name=acc_memcpy_d2d,
  // COPY-by-host-NEXT:    line_no=0, end_line_no=0,
  // COPY-by-host-NEXT:    func_line_no=0, func_end_line_no=0
  // COPY-by-host-NEXT:  acc_data_event_info
  // COPY-by-host-NEXT:    event_type=21, valid_bytes=56,
  // COPY-by-host-NEXT:    parent_construct=acc_construct_runtime_api,
  // COPY-by-host-NEXT:    implicit=0, tool_info=(nil),
  // COPY-by-host-NEXT:    var_name=(null), bytes=4,
  // COPY-by-host-NEXT:    host_ptr=0x[[#TMP_HOST_PTR]],
  // COPY-by-host-NEXT:    device_ptr=0x[[#%x,ARR_DEVICE_PTR]]
  // COPY-by-host-NEXT:  acc_api_info
  // COPY-by-host-NEXT:    device_api=0, valid_bytes=12,
  // COPY-by-host-NEXT:    device_type=[[ACC_DEVICE]]
  acc_memcpy_d2d(&arr[0], &arr[1], sizeof *arr, devNumInit, devNumInit);

  // There are no device upload/download events for host-to-host copy.
  acc_set_device_type(acc_device_host);
  acc_memcpy_d2d(&arr[0], &arr[1], sizeof *arr, 0, 0);
  acc_set_device_num(devNumInit, devTypeInit);

  //      COPY-by-host:acc_ev_enqueue_download_start
  // COPY-by-host-NEXT:  acc_prof_info
  // COPY-by-host-NEXT:    event_type=22, valid_bytes=72, version=[[VERSION]],
  // COPY-by-host-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV_OTHER]],
  // COPY-by-host-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // COPY-by-host-NEXT:    src_file=(null), func_name=acc_memcpy_d2d,
  // COPY-by-host-NEXT:    line_no=0, end_line_no=0,
  // COPY-by-host-NEXT:    func_line_no=0, func_end_line_no=0
  // COPY-by-host-NEXT:  acc_data_event_info
  // COPY-by-host-NEXT:    event_type=22, valid_bytes=56,
  // COPY-by-host-NEXT:    parent_construct=acc_construct_runtime_api,
  // COPY-by-host-NEXT:    implicit=0, tool_info=(nil),
  // COPY-by-host-NEXT:    var_name=(null), bytes=4,
  // COPY-by-host-NEXT:    host_ptr=0x[[#%x,TMP_HOST_PTR:]],
  // COPY-by-host-NEXT:    device_ptr=0x[[#%x,ARR_DEVICE_OTHER_PTR + ARR_ELE_SIZE]]
  // COPY-by-host-NEXT:  acc_api_info
  // COPY-by-host-NEXT:    device_api=0, valid_bytes=12,
  // COPY-by-host-NEXT:    device_type=[[ACC_DEVICE]]
  // COPY-by-host-NEXT:acc_ev_enqueue_download_end
  // COPY-by-host-NEXT:  acc_prof_info
  // COPY-by-host-NEXT:    event_type=23, valid_bytes=72, version=[[VERSION]],
  // COPY-by-host-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV_OTHER]],
  // COPY-by-host-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // COPY-by-host-NEXT:    src_file=(null), func_name=acc_memcpy_d2d,
  // COPY-by-host-NEXT:    line_no=0, end_line_no=0,
  // COPY-by-host-NEXT:    func_line_no=0, func_end_line_no=0
  // COPY-by-host-NEXT:  acc_data_event_info
  // COPY-by-host-NEXT:    event_type=23, valid_bytes=56,
  // COPY-by-host-NEXT:    parent_construct=acc_construct_runtime_api,
  // COPY-by-host-NEXT:    implicit=0, tool_info=(nil),
  // COPY-by-host-NEXT:    var_name=(null), bytes=4,
  // COPY-by-host-NEXT:    host_ptr=0x[[#TMP_HOST_PTR]],
  // COPY-by-host-NEXT:    device_ptr=0x[[#%x,ARR_DEVICE_OTHER_PTR + ARR_ELE_SIZE]]
  // COPY-by-host-NEXT:  acc_api_info
  // COPY-by-host-NEXT:    device_api=0, valid_bytes=12,
  // COPY-by-host-NEXT:    device_type=[[ACC_DEVICE]]
  // COPY-by-host-NEXT:acc_ev_enqueue_upload_start
  // COPY-by-host-NEXT:  acc_prof_info
  // COPY-by-host-NEXT:    event_type=20, valid_bytes=72, version=[[VERSION]],
  // COPY-by-host-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // COPY-by-host-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // COPY-by-host-NEXT:    src_file=(null), func_name=acc_memcpy_d2d,
  // COPY-by-host-NEXT:    line_no=0, end_line_no=0,
  // COPY-by-host-NEXT:    func_line_no=0, func_end_line_no=0
  // COPY-by-host-NEXT:  acc_data_event_info
  // COPY-by-host-NEXT:    event_type=20, valid_bytes=56,
  // COPY-by-host-NEXT:    parent_construct=acc_construct_runtime_api,
  // COPY-by-host-NEXT:    implicit=0, tool_info=(nil),
  // COPY-by-host-NEXT:    var_name=(null), bytes=4,
  // COPY-by-host-NEXT:    host_ptr=0x[[#TMP_HOST_PTR]],
  // COPY-by-host-NEXT:    device_ptr=0x[[#%x,ARR_DEVICE_PTR]]
  // COPY-by-host-NEXT:  acc_api_info
  // COPY-by-host-NEXT:    device_api=0, valid_bytes=12,
  // COPY-by-host-NEXT:    device_type=[[ACC_DEVICE]]
  // COPY-by-host-NEXT:acc_ev_enqueue_upload_end
  // COPY-by-host-NEXT:  acc_prof_info
  // COPY-by-host-NEXT:    event_type=21, valid_bytes=72, version=[[VERSION]],
  // COPY-by-host-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // COPY-by-host-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // COPY-by-host-NEXT:    src_file=(null), func_name=acc_memcpy_d2d,
  // COPY-by-host-NEXT:    line_no=0, end_line_no=0,
  // COPY-by-host-NEXT:    func_line_no=0, func_end_line_no=0
  // COPY-by-host-NEXT:  acc_data_event_info
  // COPY-by-host-NEXT:    event_type=21, valid_bytes=56,
  // COPY-by-host-NEXT:    parent_construct=acc_construct_runtime_api,
  // COPY-by-host-NEXT:    implicit=0, tool_info=(nil),
  // COPY-by-host-NEXT:    var_name=(null), bytes=4,
  // COPY-by-host-NEXT:    host_ptr=0x[[#TMP_HOST_PTR]],
  // COPY-by-host-NEXT:    device_ptr=0x[[#%x,ARR_DEVICE_PTR]]
  // COPY-by-host-NEXT:  acc_api_info
  // COPY-by-host-NEXT:    device_api=0, valid_bytes=12,
  // COPY-by-host-NEXT:    device_type=[[ACC_DEVICE]]
  acc_memcpy_d2d(&arr[0], &arr[1], sizeof *arr, devNumInit, devNumOther);

  // We've already checked acc_delete when data is present, but we want to make
  // it absent again.
  //
  // OFF:acc_ev_exit_data_start
  // OFF:acc_ev_delete
  // OFF:acc_ev_free
  // OFF:acc_ev_exit_data_end
  acc_delete(arr, sizeof arr);

  return 0;
}

// Only shutdown events should follow, which we check more thoroughly in other
// tests.  It's hard to check them here because there might be one or two device
// shutdown events.
//
// CHECK:{{($[[:space:]]  .+)+($[[:space:]]acc_ev_(device_shutdown_start|device_shutdown_end|runtime_shutdown)($[[:space:]]  .+)+)+}}

// CHECK-NOT: {{.}}
// ERR-NOT: {{.}}
