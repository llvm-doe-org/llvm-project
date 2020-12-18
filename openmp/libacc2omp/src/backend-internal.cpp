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
#include <dlfcn.h>

acc2omp_msg_t acc2omp_msg(acc2omp_msgid_t MsgId) {
  acc2omp_msg_t Msg = {MsgId, nullptr};
  // Keep this in a switch instead of an array indexed on acc2omp_msg_id_t to
  // help avoid mismatching IDs and formats.
  switch (MsgId) {
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
  case acc2omp_msg_map_data_host_pointer_null:
    Msg.DefaultFmt = "acc_map_data called with null host pointer";
    break;
  case acc2omp_msg_map_data_device_pointer_null:
    Msg.DefaultFmt = "acc_map_data called with null device pointer";
    break;
  case acc2omp_msg_map_data_bytes_zero:
    Msg.DefaultFmt = "acc_map_data called with zero bytes";
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
    Msg.DefaultFmt = "acc_unmap_data call with null pointer";
    break;
  case acc2omp_msg_unmap_data_shared_memory:
    Msg.DefaultFmt = "acc_unmap_data called for shared memory";
    break;
  case acc2omp_msg_unmap_data_fail:
    Msg.DefaultFmt = "acc_unmap_data failed";
    break;
  }
  assert(Msg.DefaultFmt && "expected acc2omp_msg_t to be handled in switch");
  return Msg;
};
