/*
 * backend-internal.cpp -- backend-internal.h implementation.
 */

//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "config.h"
#include "backend-internal.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>

acc2omp_msg_t acc2omp_msg(acc2omp_msgid_t MsgId) {
  acc2omp_msg_t Msg = {MsgId, nullptr};
  // Keep this in a switch instead of an array indexed on acc2omp_msg_id_t to
  // help avoid mismatching IDs and formats.
  //
  // Keep the messages in sync with openmp/runtime/src/i18n/en_US.txt.
  switch (MsgId) {
  case acc2omp_msg_env_acc_device_type_invalid:
    Msg.DefaultFmt = "ACC_DEVICE_TYPE is invalid: %s";
    break;
  case acc2omp_msg_env_acc_device_num_parse_error:
    Msg.DefaultFmt = "ACC_DEVICE_NUM is not a non-negative integer: %s";
    break;
  case acc2omp_msg_env_acc_device_num_default_invalid:
    Msg.DefaultFmt = "ACC_DEVICE_NUM=%d (default device number) is invalid for "
                     "ACC_DEVICE_TYPE=%s";
    break;
  case acc2omp_msg_env_acc_device_num_invalid:
    Msg.DefaultFmt = "ACC_DEVICE_NUM=%d is invalid for ACC_DEVICE_TYPE=%s";
    break;
  case acc2omp_msg_alloc_fail:
    Msg.DefaultFmt = "memory allocation failed";
    break;
  case acc2omp_msg_acc_proflib_fail:
    Msg.DefaultFmt = "failure using library from ACC_PROFLIB: %s";
    break;
  case acc2omp_msg_unsupported_event_register:
    Msg.DefaultFmt = "attempt to register event that is not yet supported: %s";
    break;
  case acc2omp_msg_unsupported_event_unregister:
    Msg.DefaultFmt =
        "attempt to unregister event that is not yet supported: %s";
    break;
  case acc2omp_msg_event_unsupported_toggle:
    Msg.DefaultFmt = "toggling events is not yet supported: %s";
    break;
  case acc2omp_msg_event_unsupported_multiple_register:
    Msg.DefaultFmt =
        "registering already registered events is not yet supported: %s";
    break;
  case acc2omp_msg_event_register_result:
    Msg.DefaultFmt = "%s result when registering event: %s";
    break;
  case acc2omp_msg_event_unregister_result:
    Msg.DefaultFmt = "%s result when unregistering event: %s";
    break;
  case acc2omp_msg_event_unregister_unregistered:
    Msg.DefaultFmt =
        "attempt to unregister event not previously registered: %s";
    break;
  case acc2omp_msg_callback_unregister_unregistered:
    Msg.DefaultFmt = "attempt to unregister wrong callback for event: %s";
    break;
  case acc2omp_msg_get_num_devices_invalid_type:
    Msg.DefaultFmt = "acc_get_num_devices called for invalid device type %d";
    break;
  case acc2omp_msg_set_device_type_invalid_type:
    Msg.DefaultFmt = "acc_set_device_type called for invalid device type %d";
    break;
  case acc2omp_msg_set_device_type_no_devices:
    Msg.DefaultFmt = "acc_set_device_type called for %s, which has no "
                     "available devices";
    break;
  case acc2omp_msg_set_device_num_invalid_type:
    Msg.DefaultFmt = "acc_set_device_num called for invalid device type %d";
    break;
  case acc2omp_msg_set_device_num_invalid_num:
    Msg.DefaultFmt = "acc_set_device_num called with invalid device number %d "
                     "for %s";
    break;
  case acc2omp_msg_get_device_num_invalid_type:
    Msg.DefaultFmt = "acc_get_device_num called for invalid device type %d";
    break;
  case acc2omp_msg_map_data_host_pointer_null:
    Msg.DefaultFmt = "acc_map_data called with null host pointer";
    break;
  case acc2omp_msg_map_data_device_pointer_null:
    Msg.DefaultFmt = "acc_map_data called with null device pointer";
    break;
  case acc2omp_msg_map_data_shared_memory:
    Msg.DefaultFmt = "acc_map_data called for shared memory";
    break;
  case acc2omp_msg_map_data_already_present:
    Msg.DefaultFmt = "acc_map_data called with host pointer that is already "
                     "mapped";
    break;
  case acc2omp_msg_map_data_fail:
    Msg.DefaultFmt = "acc_map_data failed";
    break;
  case acc2omp_msg_unmap_data_pointer_null:
    Msg.DefaultFmt = "acc_unmap_data called with null pointer";
    break;
  case acc2omp_msg_unmap_data_shared_memory:
    Msg.DefaultFmt = "acc_unmap_data called for shared memory";
    break;
  case acc2omp_msg_unmap_data_fail:
    Msg.DefaultFmt = "acc_unmap_data failed";
    break;
  case acc2omp_msg_memcpy_to_device_dest_pointer_null:
    Msg.DefaultFmt = "acc_memcpy_to_device called with null destination "
                     "pointer";
    break;
  case acc2omp_msg_memcpy_to_device_src_pointer_null:
    Msg.DefaultFmt = "acc_memcpy_to_device called with null source pointer";
    break;
  case acc2omp_msg_memcpy_to_device_fail:
    Msg.DefaultFmt = "acc_memcpy_to_device failed";
    break;
  case acc2omp_msg_memcpy_from_device_dest_pointer_null:
    Msg.DefaultFmt = "acc_memcpy_from_device called with null destination "
                     "pointer";
    break;
  case acc2omp_msg_memcpy_from_device_src_pointer_null:
    Msg.DefaultFmt = "acc_memcpy_from_device called with null source pointer";
    break;
  case acc2omp_msg_memcpy_from_device_fail:
    Msg.DefaultFmt = "acc_memcpy_from_device failed";
    break;
  case acc2omp_msg_memcpy_device_dest_pointer_null:
    Msg.DefaultFmt = "acc_memcpy_device called with null destination pointer";
    break;
  case acc2omp_msg_memcpy_device_src_pointer_null:
    Msg.DefaultFmt = "acc_memcpy_device called with null source pointer";
    break;
  case acc2omp_msg_memcpy_device_fail:
    Msg.DefaultFmt = "acc_memcpy_device failed";
    break;
  case acc2omp_msg_memcpy_d2d_dest_pointer_null:
    Msg.DefaultFmt = "acc_memcpy_d2d called with null destination pointer";
    break;
  case acc2omp_msg_memcpy_d2d_src_pointer_null:
    Msg.DefaultFmt = "acc_memcpy_d2d called with null source pointer";
    break;
  case acc2omp_msg_memcpy_d2d_dest_device_invalid:
    Msg.DefaultFmt = "acc_memcpy_d2d called with invalid destination device "
                     " number %d for the current device type %s";
    break;
  case acc2omp_msg_memcpy_d2d_src_device_invalid:
    Msg.DefaultFmt = "acc_memcpy_d2d called with invalid source device number "
                     "%d for the current device type %s";
    break;
  case acc2omp_msg_memcpy_d2d_dest_data_inaccessible:
    Msg.DefaultFmt = "acc_memcpy_d2d called with inaccessible destination data";
    break;
  case acc2omp_msg_memcpy_d2d_src_data_inaccessible:
    Msg.DefaultFmt = "acc_memcpy_d2d called with inaccessible source data";
    break;
  case acc2omp_msg_memcpy_d2d_fail:
    Msg.DefaultFmt = "acc_memcpy_d2d failed";
    break;
  }
  assert(Msg.DefaultFmt && "expected acc2omp_msg_t to be handled in switch");
  return Msg;
};
