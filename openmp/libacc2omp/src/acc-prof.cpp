/*
 * acc-prof.cpp -- OpenACC Profiling Interface implementation.
 */

//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

/*****************************************************************************
 * System include files.
 ****************************************************************************/

// FIXME: Any uses of stderr or stdout in this file should use an
// acc2omp-proxy.h function instead.
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <map>

/*****************************************************************************
 * OpenMP standard include files.
 ****************************************************************************/

#include "omp-tools.h"

/*****************************************************************************
 * LLVM OpenMP include files.
 *
 * One of our eventual goals is to be able to wrap any OpenMP runtime
 * implementing OMPT.  For that reason, it's expected that anything included
 * here doesn't actually require linking with LLVM's OpenMP runtime.  Only
 * definitions in the header are needed.  If you do need to link something from
 * the OpenMP runtime, see through acc2omp-proxy.
 ****************************************************************************/

// Currently needed for:
// - KMP_HAVE_WEAK_ATTRIBUTE
// - KMP_WEAK_ATTRIBUTE_INTERNAL
// - KMP_FALLTHROUGH
#include "kmp_os.h"

/*****************************************************************************
 * OpenACC standard include files.
 ****************************************************************************/

#include "acc_prof.h"

/*****************************************************************************
 * Internal includes.
 ****************************************************************************/

#include "acc2omp-proxy-internal.h"

/*****************************************************************************
 * Runtime state.
 *
 * The only other runtime state is the registration counts for OMPT callback
 * functions.  Those are defined next to their associated OMPT callback
 * functions.
 *
 * FIXME: Access is not thread-safe.  Does it need to be?
 ****************************************************************************/

// The initial_device_num passed to the OMPT initialize callback.
static int acc_ompt_initial_device_num;

// Map from target_id to device_num as passed to ompt_callback_target.  Entries
// are inserted/erased as target regions begin/end.
static std::map<ompt_id_t, int> acc_ompt_target_device_map;

// OMPT entry points previously looked up.
static ompt_set_callback_t acc_ompt_set_callback = NULL;
static ompt_get_directive_info_t acc_ompt_get_directive_info = NULL;
static ompt_get_data_expression_t acc_ompt_get_data_expression = NULL;

// Registered OpenACC callbacks.
static acc_prof_callback acc_ev_device_init_start_callback = NULL;
static acc_prof_callback acc_ev_device_init_end_callback = NULL;
static acc_prof_callback acc_ev_device_shutdown_start_callback = NULL;
static acc_prof_callback acc_ev_device_shutdown_end_callback = NULL;
static acc_prof_callback acc_ev_runtime_shutdown_callback = NULL;
static acc_prof_callback acc_ev_create_callback = NULL;
static acc_prof_callback acc_ev_delete_callback = NULL;
static acc_prof_callback acc_ev_alloc_callback = NULL;
static acc_prof_callback acc_ev_free_callback = NULL;
static acc_prof_callback acc_ev_enter_data_start_callback = NULL;
static acc_prof_callback acc_ev_enter_data_end_callback = NULL;
static acc_prof_callback acc_ev_exit_data_start_callback = NULL;
static acc_prof_callback acc_ev_exit_data_end_callback = NULL;
static acc_prof_callback acc_ev_update_start_callback = NULL;
static acc_prof_callback acc_ev_update_end_callback = NULL;
static acc_prof_callback acc_ev_compute_construct_start_callback = NULL;
static acc_prof_callback acc_ev_compute_construct_end_callback = NULL;
static acc_prof_callback acc_ev_enqueue_launch_start_callback = NULL;
static acc_prof_callback acc_ev_enqueue_launch_end_callback = NULL;
static acc_prof_callback acc_ev_enqueue_upload_start_callback = NULL;
static acc_prof_callback acc_ev_enqueue_upload_end_callback = NULL;
static acc_prof_callback acc_ev_enqueue_download_start_callback = NULL;
static acc_prof_callback acc_ev_enqueue_download_end_callback = NULL;

// OpenACC callback register/deregister action queue implementation.
struct acc_prof_action {
  bool reg;
  acc_event_t event;
  acc_prof_callback cb;
  acc_register_t info;
  acc_prof_action *next;
};
static acc_prof_action *acc_prof_action_head = nullptr;
static acc_prof_action *acc_prof_action_tail = nullptr;
static void acc_prof_enqueue(acc_prof_action *action) {
  action->next = nullptr;
  if (!acc_prof_action_head)
    acc_prof_action_head = acc_prof_action_tail = action;
  else {
    acc_prof_action_tail->next = action;
    acc_prof_action_tail = action;
  }
}
static acc_prof_action *acc_prof_dequeue() {
  acc_prof_action *action = acc_prof_action_head;
  if (action) {
    acc_prof_action_head = action->next;
    if (!acc_prof_action_head)
      acc_prof_action_tail = nullptr;
    action->next = nullptr;
  }
  return action;
}

/*****************************************************************************
 * Functions that build OpenACC callback arguments.
 ****************************************************************************/

static const char *acc_get_event_name(acc_event_t event) {
  switch (event) {
  case acc_ev_none:
    return "acc_ev_none";
#define EVENT(Event)       \
  case acc_ev_##Event:     \
    return "acc_ev_"#Event;
  ACC_FOREACH_EVENT(EVENT)
#undef EVENT
  case acc_ev_last:
    return "acc_ev_last";
  }
  ACC2OMP_UNREACHABLE("unexpected acc_event_t");
  return NULL;
}

#define valid_bytes(Type, Field) \
  (offsetof(Type, Field) + sizeof ((Type*)0)->Field)

static acc_prof_info acc_get_prof_info(acc_event_t event_type,
                                       int device_number) {
  acc_prof_info ret;
  ret.event_type = event_type;
  // FIXME: Clang should set _OPENACC to the same value, and we should
  // configure that value in one place in the code base.
  ret.version = 201811;
  ret.device_type = device_number == acc_ompt_initial_device_num
                    ? acc_device_host : acc_device_not_host;
  ret.device_number = device_number;
  // FIXME: We need to find the right way to compute this.
  ret.thread_id = 0;
  // FIXME: We currently don't support the async clause, so this is currently
  // always right.
  ret.async = acc_async_sync;
  // FIXME: OpenACC 2.7 sec. 5.2.1 L2912-2913 says, "If the runtime uses a
  // limited number of asynchronous queues, this field contains the internal
  // asynchronous queue number used for the event."  But it doesn't say what
  // the field contains if the runtime doesn't, so we guess -1.
  ret.async_queue = -1;
  // The remaining fields of acc_prof_info are source location information.
  // If the OpenMP runtime doesn't support the ompt_get_directive_info entry
  // point, just nullify the fields.  FIXME: That will make sense when we
  // separate the OpenACC runtime from LLVM's OpenMP runtime, thus creating the
  // possibility of linking the OpenACC runtime with alternate OpenMP runtimes.
  if (acc_ompt_get_directive_info) {
    ompt_directive_info_t *directive_info = acc_ompt_get_directive_info();
    ret.src_file = directive_info->src_file;
    ret.func_name = directive_info->func_name;
    ret.line_no = directive_info->line_no;
    ret.end_line_no = directive_info->end_line_no;
    ret.func_line_no = directive_info->func_line_no;
    ret.func_end_line_no = directive_info->func_end_line_no;
  } else {
    ret.src_file = NULL;
    ret.func_name = NULL;
    ret.line_no = 0;
    ret.end_line_no = 0;
    ret.func_line_no = 0;
    ret.func_end_line_no = 0;
  }
  ret.valid_bytes = valid_bytes(acc_prof_info, func_end_line_no);
  return ret;
}

static acc_event_info acc_get_other_event_info(acc_event_t event_type) {
  acc_event_info ret;
  ret.other_event.event_type = event_type;
  // When ompt_get_directive_info returns kind=ompt_directive_unknown, we
  // assume the event was triggered internally and thus has no connection back
  // to a specific directive or runtime call.  There's no acc_construct_t
  // member in OpenACC 3.0 that makes sense for this case, so we pick
  // parent_construct=acc_construct_runtime_api and implicit=true to try to
  // indicate an internal call.  FIXME: OpenACC should be extended with
  // something like acc_construct_internal.
  //
  // This case occurs when the event is acc_ev_runtime_shutdown,
  // acc_ev_device_shutdown_start, or acc_ev_device_shutdown_end without a
  // triggering directive or runtime API call.  This case also currently also
  // occurs if the compiler or OpenMP runtime isn't able to provide the
  // required directive info.  Thus, for this case, ompt_get_directive_info,
  // if available as an entry point, should always return a default-initialized
  // ompt_directive_info_t, which has is_explicit_event=false.
  if (!acc_ompt_get_directive_info) {
    ret.other_event.parent_construct = acc_construct_runtime_api;
    ret.other_event.implicit = true;
  } else {
    ompt_directive_info_t *directive_info = acc_ompt_get_directive_info();
    switch (directive_info->kind) {
    case ompt_directive_unknown:
      // TODO: Once we provide directive info for runtime calls, for which
      // is_explicit_event should be true, create an ompt_directive_runtime_api
      // instead of reusing ompt_directive_unknown.
      ACC2OMP_ASSERT(!directive_info->is_explicit_event,
                     "expected is_explicit_event=false for "
                     "kind=ompt_directve_unknown");
      ret.other_event.parent_construct = acc_construct_runtime_api;
      break;
    case ompt_directive_target_update:
      ret.other_event.parent_construct = acc_construct_update;
      break;
    case ompt_directive_target_enter_data:
      ret.other_event.parent_construct = acc_construct_enter_data;
      break;
    case ompt_directive_target_exit_data:
      ret.other_event.parent_construct = acc_construct_exit_data;
      break;
    case ompt_directive_target_data:
      ret.other_event.parent_construct = acc_construct_data;
      break;
    case ompt_directive_target_teams:
      ret.other_event.parent_construct = acc_construct_parallel;
      break;
    }
    ret.other_event.implicit = !directive_info->is_explicit_event;
  }
  ret.other_event.tool_info = NULL;
  ret.other_event.valid_bytes = valid_bytes(acc_other_event_info, tool_info);
  return ret;
}

static acc_event_info acc_get_data_event_info(
    acc_event_t event_type, size_t bytes, const void *host_ptr,
    const void *device_ptr) {
  acc_event_info ret = acc_get_other_event_info(event_type);
  // If the OpenMP runtime doesn't support the ompt_get_data_expression entry
  // point, just use NULL.  FIXME: That will make sense when we separate the
  // OpenACC runtime from LLVM's OpenMP runtime, thus creating the possibility
  // of linking the OpenACC runtime with alternate OpenMP runtimes.
  if (acc_ompt_get_data_expression)
    ret.data_event.var_name = acc_ompt_get_data_expression();
  else
    ret.data_event.var_name = NULL;
  ret.data_event.bytes = bytes;
  ret.data_event.host_ptr = host_ptr;
  ret.data_event.device_ptr = device_ptr;
  ret.data_event.valid_bytes = valid_bytes(acc_data_event_info, device_ptr);
  return ret;
}

static acc_event_info acc_get_launch_event_info(acc_event_t event_type) {
  acc_event_info ret = acc_get_other_event_info(event_type);
  ret.launch_event.kernel_name = NULL;
  ret.launch_event.valid_bytes = valid_bytes(acc_launch_event_info,
                                             kernel_name);
  // FIXME: The remaining fields of acc_launch_event_info are num_gangs,
  // num_workers, and vector_length, but I don't know how to access that here.
  // ompt_callback_target_submit{,_start} does provide requested_num_teams, but
  // OpenACC apparently wants the number of gangs created.
  return ret;
}

static acc_api_info acc_get_api_info(int device_number) {
  acc_api_info ret;
  // FIXME: We don't support any device-specific APIs yet in remaining fields,
  // so acc_device_api_none seems like the right thing for now.
  ret.device_api = acc_device_api_none;
  ret.device_type = device_number == acc_ompt_initial_device_num
                    ? acc_device_host : acc_device_not_host;
  // FIXME: How do we choose our vendor identifier?  Does Clang's OpenMP
  // implementation have something?
  //ret.vendor = 0;
  // FIXME: OpenACC 2.7 doesn't seems to say what goes here, so it must be
  // implementation-defined.
  //ret.device_handle = NULL;
  //ret.context_handle = NULL;
  //ret.async_handle = NULL;
  ret.valid_bytes = valid_bytes(acc_api_info, device_type);
  return ret;
}

static void acc_get_target_data_op_info(
    acc_event_t event_type, ompt_id_t target_id, int device_num, size_t bytes,
    const void *host_ptr, const void *device_ptr, acc_prof_info *pi,
    acc_event_info *ei, acc_api_info *ai) {
  *pi = acc_get_prof_info(event_type, device_num);
  *ei = acc_get_data_event_info(event_type, bytes, host_ptr, device_ptr);
  *ai = acc_get_api_info(device_num);
}

/*****************************************************************************
 * OMPT callback functions (except initialize and finalize) and their
 * registration counts.
 ****************************************************************************/

static unsigned acc_ompt_callback_device_initialize_start_reg_counter = 0;
static void acc_ompt_callback_device_initialize_start(
    int device_num, const char *type, ompt_device_t *device,
    ompt_function_lookup_t lookup, const char *documentation) {
  acc_event_t event_type = acc_ev_device_init_start;
  acc_prof_info pi = acc_get_prof_info(event_type, device_num);
  acc_event_info ei = acc_get_other_event_info(event_type);
  acc_api_info ai = acc_get_api_info(device_num);
  acc_ev_device_init_start_callback(&pi, &ei, &ai);
}

static unsigned acc_ompt_callback_device_initialize_reg_counter = 0;
static void acc_ompt_callback_device_initialize(
    int device_num, const char *type, ompt_device_t *device,
    ompt_function_lookup_t lookup, const char *documentation) {
  acc_event_t event_type = acc_ev_device_init_end;
  acc_prof_info pi = acc_get_prof_info(event_type, device_num);
  acc_event_info ei = acc_get_other_event_info(event_type);
  acc_api_info ai = acc_get_api_info(device_num);
  acc_ev_device_init_end_callback(&pi, &ei, &ai);
}

static unsigned acc_ompt_callback_device_finalize_start_reg_counter = 0;
static void acc_ompt_callback_device_finalize_start(int device_num) {
  acc_event_t event_type = acc_ev_device_shutdown_start;
  acc_prof_info pi = acc_get_prof_info(event_type, device_num);
  acc_event_info ei = acc_get_other_event_info(event_type);
  acc_api_info ai = acc_get_api_info(device_num);
  acc_ev_device_shutdown_start_callback(&pi, &ei, &ai);
}

static unsigned acc_ompt_callback_device_finalize_reg_counter = 0;
static void acc_ompt_callback_device_finalize(int device_num) {
  acc_event_t event_type = acc_ev_device_shutdown_end;
  acc_prof_info pi = acc_get_prof_info(event_type, device_num);
  acc_event_info ei = acc_get_other_event_info(event_type);
  acc_api_info ai = acc_get_api_info(device_num);
  acc_ev_device_shutdown_end_callback(&pi, &ei, &ai);
}

static unsigned acc_ompt_callback_target_reg_counter = 0 ;
static void acc_ompt_callback_target(
    ompt_target_t kind, ompt_scope_endpoint_t endpoint, int device_num,
    ompt_data_t *task_data, ompt_id_t target_id, const void *codeptr_ra) {
  acc_event_t event_type;
  acc_prof_callback acc_cb = nullptr;
  bool sub_region = false;
  switch (kind) {
  case ompt_target:
    switch (endpoint) {
    case ompt_scope_begin:
      event_type = acc_ev_compute_construct_start;
      acc_cb = acc_ev_compute_construct_start_callback;
      break;
    case ompt_scope_end:
      event_type = acc_ev_compute_construct_end;
      acc_cb = acc_ev_compute_construct_end_callback;
      break;
    }
    break;
  case ompt_target_region_enter_data:
    sub_region = true;
    KMP_FALLTHROUGH();
  case ompt_target_enter_data:
    switch (endpoint) {
    case ompt_scope_begin:
      event_type = acc_ev_enter_data_start;
      acc_cb = acc_ev_enter_data_start_callback;
      break;
    case ompt_scope_end:
      event_type = acc_ev_enter_data_end;
      acc_cb = acc_ev_enter_data_end_callback;
      break;
    }
    break;
  case ompt_target_region_exit_data:
    sub_region = true;
    KMP_FALLTHROUGH();
  case ompt_target_exit_data:
    switch (endpoint) {
    case ompt_scope_begin:
      event_type = acc_ev_exit_data_start;
      acc_cb = acc_ev_exit_data_start_callback;
      break;
    case ompt_scope_end:
      event_type = acc_ev_exit_data_end;
      acc_cb = acc_ev_exit_data_end_callback;
      break;
    }
    break;
  case ompt_target_update:
    switch (endpoint) {
    case ompt_scope_begin:
      event_type = acc_ev_update_start;
      acc_cb = acc_ev_update_start_callback;
      break;
    case ompt_scope_end:
      event_type = acc_ev_update_end;
      acc_cb = acc_ev_update_end_callback;
      break;
    }
    break;
  }
  if (!sub_region && endpoint == ompt_scope_begin)
    acc_ompt_target_device_map[target_id] = device_num;
  if (acc_cb) {
    acc_prof_info pi = acc_get_prof_info(event_type, device_num);
    acc_event_info ei = acc_get_other_event_info(event_type);
    acc_api_info ai = acc_get_api_info(device_num);
    acc_cb(&pi, &ei, &ai);
  }
  if (!sub_region && endpoint == ompt_scope_end)
    acc_ompt_target_device_map.erase(target_id);
}

static unsigned acc_ompt_callback_target_submit_reg_counter = 0 ;
static void acc_ompt_callback_target_submit(
    ompt_id_t target_id, ompt_id_t host_op_id,
    unsigned int requested_num_teams) {
  acc_event_t event_type = acc_ev_enqueue_launch_start;
  auto itr = acc_ompt_target_device_map.find(target_id);
  ACC2OMP_ASSERT(itr != acc_ompt_target_device_map.end(),
                 "unexpected target_id");
  int device_num = itr->second;
  acc_prof_info pi = acc_get_prof_info(event_type, device_num);
  acc_event_info ei = acc_get_launch_event_info(event_type);
  acc_api_info ai = acc_get_api_info(device_num);
  acc_ev_enqueue_launch_start_callback(&pi, &ei, &ai);
}

static unsigned acc_ompt_callback_target_submit_end_reg_counter = 0 ;
static void acc_ompt_callback_target_submit_end(
    ompt_id_t target_id, ompt_id_t host_op_id,
    unsigned int requested_num_teams) {
  acc_event_t event_type = acc_ev_enqueue_launch_end;
  auto itr = acc_ompt_target_device_map.find(target_id);
  ACC2OMP_ASSERT(itr != acc_ompt_target_device_map.end(),
                 "unexpected target_id");
  int device_num = itr->second;
  acc_prof_info pi = acc_get_prof_info(event_type, device_num);
  acc_event_info ei = acc_get_launch_event_info(event_type);
  acc_api_info ai = acc_get_api_info(device_num);
  acc_ev_enqueue_launch_end_callback(&pi, &ei, &ai);
}

static unsigned acc_ompt_callback_target_data_op_reg_counter = 0 ;
static void acc_ompt_callback_target_data_op(
    ompt_id_t target_id, ompt_id_t host_op_id, ompt_target_data_op_t optype,
    void *src_addr, int src_device_num, void *dest_addr, int dest_device_num,
    size_t bytes, const void *codeptr_ra) {
  switch (optype) {
  case ompt_target_data_associate:
    if (acc_ev_create_callback) {
      acc_prof_info pi;
      acc_event_info ei;
      acc_api_info ai;
      acc_get_target_data_op_info(acc_ev_create, target_id, dest_device_num,
                                  bytes, src_addr, dest_addr, &pi, &ei, &ai);
      acc_ev_create_callback(&pi, &ei, &ai);
    }
    break;
  case ompt_target_data_disassociate:
    if (acc_ev_delete_callback) {
      acc_prof_info pi;
      acc_event_info ei;
      acc_api_info ai;
      acc_get_target_data_op_info(acc_ev_delete, target_id, dest_device_num,
                                  bytes, src_addr, dest_addr, &pi, &ei, &ai);
      acc_ev_delete_callback(&pi, &ei, &ai);
    }
    break;
  case ompt_target_data_alloc:
    if (acc_ev_alloc_callback) {
      acc_prof_info pi;
      acc_event_info ei;
      acc_api_info ai;
      acc_get_target_data_op_info(acc_ev_alloc, target_id, dest_device_num,
                                  bytes, src_addr, dest_addr, &pi, &ei, &ai);
      acc_ev_alloc_callback(&pi, &ei, &ai);
    }
    break;
  case ompt_target_data_delete:
    if (acc_ev_free_callback) {
      acc_prof_info pi;
      acc_event_info ei;
      acc_api_info ai;
      acc_get_target_data_op_info(acc_ev_free, target_id, dest_device_num,
                                  bytes, src_addr, dest_addr, &pi, &ei, &ai);
      acc_ev_free_callback(&pi, &ei, &ai);
    }
    break;
  case ompt_target_data_transfer_to_device:
    if (acc_ev_enqueue_upload_start_callback) {
      acc_prof_info pi;
      acc_event_info ei;
      acc_api_info ai;
      acc_get_target_data_op_info(acc_ev_enqueue_upload_start, target_id,
                                  dest_device_num, bytes, src_addr, dest_addr,
                                  &pi, &ei, &ai);
      acc_ev_enqueue_upload_start_callback(&pi, &ei, &ai);
    }
    break;
  case ompt_target_data_transfer_to_device_end:
    if (acc_ev_enqueue_upload_end_callback) {
      acc_prof_info pi;
      acc_event_info ei;
      acc_api_info ai;
      acc_get_target_data_op_info(acc_ev_enqueue_upload_end, target_id,
                                  dest_device_num, bytes, src_addr, dest_addr,
                                  &pi, &ei, &ai);
      acc_ev_enqueue_upload_end_callback(&pi, &ei, &ai);
    }
    break;
  case ompt_target_data_transfer_from_device:
    if (acc_ev_enqueue_download_start_callback) {
      acc_prof_info pi;
      acc_event_info ei;
      acc_api_info ai;
      acc_get_target_data_op_info(acc_ev_enqueue_download_start, target_id,
                                  src_device_num, bytes, dest_addr, src_addr,
                                  &pi, &ei, &ai);
      acc_ev_enqueue_download_start_callback(&pi, &ei, &ai);
    }
    break;
  case ompt_target_data_transfer_from_device_end:
    if (acc_ev_enqueue_download_end_callback) {
      acc_prof_info pi;
      acc_event_info ei;
      acc_api_info ai;
      acc_get_target_data_op_info(acc_ev_enqueue_download_end, target_id,
                                  src_device_num, bytes, dest_addr, src_addr,
                                  &pi, &ei, &ai);
      acc_ev_enqueue_download_end_callback(&pi, &ei, &ai);
    }
    break;
  }
}

/*****************************************************************************
 * OpenACC callback registration performed during OMPT initialize callback
 * (acc_ompt_initialize).
 ****************************************************************************/

// For the specified OpenACC event, set the specified OpenACC callback function
// and get the required OMPT callback functions.
static bool acc_event_callback(
    acc_event_t acc_event, acc_prof_callback **acc_cb_ptr,
    unsigned *ompt_event_count, ompt_callbacks_t **ompt_events) {
  switch (acc_event) {
#define ACC_EVENT_CASE(AccEvent, ...)                                     \
  case AccEvent: {                                                        \
    *acc_cb_ptr = &AccEvent##_callback;                                   \
    static ompt_callbacks_t ompt_event_arr[] = {__VA_ARGS__};             \
    *ompt_event_count = sizeof ompt_event_arr / sizeof ompt_event_arr[0]; \
    *ompt_events = ompt_event_arr;                                        \
    return 0;                                                             \
  }
  // ompt_callback_target is needed for many events in order to associate the
  // target_id with the device_num.
  ACC_EVENT_CASE(acc_ev_device_init_start,       ompt_callback_device_initialize_start)
  ACC_EVENT_CASE(acc_ev_device_init_end,         ompt_callback_device_initialize)
  ACC_EVENT_CASE(acc_ev_device_shutdown_start,   ompt_callback_device_finalize_start)
  ACC_EVENT_CASE(acc_ev_device_shutdown_end,     ompt_callback_device_finalize)
  ACC_EVENT_CASE(acc_ev_create,                  ompt_callback_target_data_op)
  ACC_EVENT_CASE(acc_ev_delete,                  ompt_callback_target_data_op)
  ACC_EVENT_CASE(acc_ev_alloc,                   ompt_callback_target_data_op)
  ACC_EVENT_CASE(acc_ev_free,                    ompt_callback_target_data_op)
  ACC_EVENT_CASE(acc_ev_enter_data_start,        ompt_callback_target)
  ACC_EVENT_CASE(acc_ev_enter_data_end,          ompt_callback_target)
  ACC_EVENT_CASE(acc_ev_exit_data_start,         ompt_callback_target)
  ACC_EVENT_CASE(acc_ev_exit_data_end,           ompt_callback_target)
  ACC_EVENT_CASE(acc_ev_update_start,            ompt_callback_target)
  ACC_EVENT_CASE(acc_ev_update_end,              ompt_callback_target)
  ACC_EVENT_CASE(acc_ev_compute_construct_start, ompt_callback_target)
  ACC_EVENT_CASE(acc_ev_compute_construct_end,   ompt_callback_target)
  ACC_EVENT_CASE(acc_ev_enqueue_launch_start,    ompt_callback_target, ompt_callback_target_submit)
  ACC_EVENT_CASE(acc_ev_enqueue_launch_end,      ompt_callback_target, ompt_callback_target_submit_end)
  ACC_EVENT_CASE(acc_ev_enqueue_upload_start,    ompt_callback_target_data_op)
  ACC_EVENT_CASE(acc_ev_enqueue_upload_end,      ompt_callback_target_data_op)
  ACC_EVENT_CASE(acc_ev_enqueue_download_start,  ompt_callback_target_data_op)
  ACC_EVENT_CASE(acc_ev_enqueue_download_end,    ompt_callback_target_data_op)
#undef ACC_EVENT_CASE
  case acc_ev_runtime_shutdown:
    *acc_cb_ptr = &acc_ev_runtime_shutdown_callback;
    *ompt_event_count = 0;
    return 0;
  case acc_ev_none:
  case acc_ev_wait_start:
  case acc_ev_wait_end:
  case acc_ev_last:
    return 1;
  }
}

// Get the callback function and registration counter for an OMPT event.
static void acc_ompt_event_callback(ompt_callbacks_t ompt_event,
                                    ompt_callback_t *ompt_cb,
                                    unsigned **ompt_reg_counter) {
  switch (ompt_event) {
#define OMPT_EVENT_CASE(OmptEvent)                      \
  case OmptEvent:                                       \
    *ompt_cb = (ompt_callback_t)acc_##OmptEvent;        \
    *ompt_reg_counter = &acc_##OmptEvent##_reg_counter; \
    return;
  OMPT_EVENT_CASE(ompt_callback_device_initialize_start)
  OMPT_EVENT_CASE(ompt_callback_device_initialize)
  OMPT_EVENT_CASE(ompt_callback_device_finalize_start)
  OMPT_EVENT_CASE(ompt_callback_device_finalize)
  OMPT_EVENT_CASE(ompt_callback_target)
  OMPT_EVENT_CASE(ompt_callback_target_submit)
  OMPT_EVENT_CASE(ompt_callback_target_submit_end)
  OMPT_EVENT_CASE(ompt_callback_target_data_op)
#undef OMPT_EVENT_CASE
  default:
    ACC2OMP_UNREACHABLE("unexpected ompt_callbacks_t");
    break;
  }
}

// Register an OpenACC callback and any required OMPT callbacks.
static void acc_prof_register_ompt(
    acc_event_t acc_event, acc_prof_callback acc_cb, acc_register_t acc_info) {
  // FIXME: Support other values of acc_info.  Probably should coordinate with
  // fixme below about registering multiple callbacks per event.
  if (acc_info != acc_reg) {
    fprintf(stderr, "Warning: toggling events is not supported: %s\n",
            acc_get_event_name(acc_event));
    return;
  }
  acc_prof_callback *acc_cb_ptr;
  unsigned ompt_event_count;
  ompt_callbacks_t *ompt_events;
  if (acc_event_callback(acc_event, &acc_cb_ptr, &ompt_event_count,
                         &ompt_events)) {
    fprintf(stderr, "Warning: attempt to register unhandled event: %s\n",
            acc_get_event_name(acc_event));
    return;
  }
  // FIXME: According to the spec, we need to store a list of callbacks per
  // event, and we need a reference counter per callback per event so a
  // callback can be registered multiple times for the same event without
  // causing it to be called multiple times.  Perhaps we should then also have
  // a hash for fast lookup.
  if (*acc_cb_ptr) {
    fprintf(stderr, "Warning: registering already registered events is not "
                    "supported: %s\n",
            acc_get_event_name(acc_event));
    return;
  }
  *acc_cb_ptr = acc_cb;
  for (unsigned i = 0; i < ompt_event_count; ++i) {
    ompt_callback_t ompt_cb;
    unsigned *ompt_reg_counter;
    acc_ompt_event_callback(ompt_events[i], &ompt_cb, &ompt_reg_counter);
    if ((*ompt_reg_counter)++)
      continue;
    switch (acc_ompt_set_callback(ompt_events[i], ompt_cb)) {
    case ompt_set_error:
      fprintf(stderr, "Warning: ompt_set_error result when registering event: "
                      "%s\n",
              acc_get_event_name(acc_event));
      break;
    case ompt_set_never:
      fprintf(stderr, "Warning: ompt_set_never result when registering event: "
                      "%s\n",
              acc_get_event_name(acc_event));
      break;
    case ompt_set_impossible:
      fprintf(stderr, "Warning: ompt_set_impossible result when registering "
                      "event: %s\n",
              acc_get_event_name(acc_event));
      break;
    case ompt_set_sometimes:
    case ompt_set_sometimes_paired:
    case ompt_set_always:
      break;
    }
  }
}

// Unregister an OpenACC callback and any OMPT callbacks that are no longer
// required.
//
// FIXME: See fixmes in acc_prof_register_ompt.
static void acc_prof_unregister_ompt(
    acc_event_t acc_event, acc_prof_callback acc_cb, acc_register_t acc_info) {
  if (acc_info != acc_reg) {
    fprintf(stderr, "Warning: toggling events is not supported: %s\n",
            acc_get_event_name(acc_event));
    return;
  }
  acc_prof_callback *acc_cb_ptr;
  unsigned ompt_event_count;
  ompt_callbacks_t *ompt_events;
  if (acc_event_callback(acc_event, &acc_cb_ptr, &ompt_event_count,
                         &ompt_events)) {
    fprintf(stderr, "Warning: attempt to unregister unhandled event: %s\n",
            acc_get_event_name(acc_event));
    return;
  }
  if (!*acc_cb_ptr) {
    fprintf(stderr, "Warning: attempt to unregister event not previously "
                    "registered: %s\n",
            acc_get_event_name(acc_event));
    return;
  }
  if (*acc_cb_ptr != acc_cb) {
    fprintf(stderr, "Warning: attempt to unregister wrong callback for event: "
                    "%s\n",
            acc_get_event_name(acc_event));
    return;
  }
  *acc_cb_ptr = NULL;
  for (unsigned i = 0; i < ompt_event_count; ++i) {
    ompt_callback_t ompt_cb;
    unsigned *ompt_reg_counter;
    acc_ompt_event_callback(ompt_events[i], &ompt_cb, &ompt_reg_counter);
    ACC2OMP_ASSERT(*ompt_reg_counter,
                   "expected OMPT callback registration count to be non-zero");
    if (--*ompt_reg_counter)
      continue;
    if (acc_ompt_set_callback(ompt_events[i], NULL) == ompt_set_error)
      fprintf(stderr, "Warning: ompt_set_error result when unregistering "
                      "event: %s\n",
              acc_get_event_name(acc_event));
  }
}

/*****************************************************************************
 * Implementations for acc_prof_register and acc_prof_unregister during
 * acc_register_library.
 *
 * These just enqueue register/unregister actions that will be dequeued and
 * executed during the OMPT initialize callback (acc_ompt_initialize) later.
 ****************************************************************************/

static void acc_prof_register_enqueue(
    acc_event_t acc_event, acc_prof_callback acc_cb, acc_register_t acc_info) {
  acc_prof_action *action = (acc_prof_action *)malloc(sizeof *action);
  action->reg = true;
  action->event = acc_event;
  action->cb = acc_cb;
  action->info = acc_info;
  acc_prof_enqueue(action);
}

static void acc_prof_unregister_enqueue(
    acc_event_t acc_event, acc_prof_callback acc_cb, acc_register_t acc_info) {
  acc_prof_action *action = (acc_prof_action *)malloc(sizeof *action);
  action->reg = false;
  action->event = acc_event;
  action->cb = acc_cb;
  action->info = acc_info;
  acc_prof_enqueue(action);
}

/*****************************************************************************
 * OMPT initialize and finalize callback functions.
 ****************************************************************************/

static int acc_ompt_initialize(ompt_function_lookup_t lookup,
                               int initial_device_num,
                               ompt_data_t *tool_data) {
  acc_ompt_set_callback = (ompt_set_callback_t)lookup("ompt_set_callback");
  acc_ompt_get_directive_info =
      (ompt_get_directive_info_t)lookup("ompt_get_directive_info");
  acc_ompt_get_data_expression =
      (ompt_get_data_expression_t)lookup("ompt_get_data_expression");
  while (acc_prof_action *action = acc_prof_dequeue()) {
    if (action->reg)
      acc_prof_register_ompt(action->event, action->cb, action->info);
    else
      acc_prof_unregister_ompt(action->event, action->cb, action->info);
    free(action);
  }
  acc_ompt_initial_device_num = initial_device_num;
  return 1;
}

static void acc_ompt_finalize(ompt_data_t *tool_data) {
  if (!acc_ev_runtime_shutdown_callback)
    return;
  acc_event_t event_type = acc_ev_runtime_shutdown;
  acc_prof_info pi = acc_get_prof_info(event_type,
                                       acc_ompt_initial_device_num);
  acc_event_info ei = acc_get_other_event_info(event_type);
  acc_api_info ai = acc_get_api_info(acc_ompt_initial_device_num);
  acc_ev_runtime_shutdown_callback(&pi, &ei, &ai);
}

/*****************************************************************************
 * The default acc_register_library called by acc2omp_ompt_start_tool.
 ****************************************************************************/

#if KMP_HAVE_WEAK_ATTRIBUTE

// FIXME: This needs implementations for other platforms.  See the approach
// for ompt_start_tool in ompt-general.cpp.
extern "C" KMP_WEAK_ATTRIBUTE_INTERNAL void
acc_register_library(acc_prof_reg reg, acc_prof_reg unref,
                     acc_prof_lookup_func lookup) {}
typedef void (*acc_register_library_t)(acc_prof_reg reg, acc_prof_reg unref,
                                       acc_prof_lookup_func lookup);

#else

#error "Activation of the OpenACC Profiling Interface is not supported on this platform."

#endif

/*****************************************************************************
 * acc2omp_ompt_start_tool, which is meant to be called by ompt_start_tool to
 * enable OpenACC Profiling Interface support.
 *
 * See header comments in acc2omp-proxy.h for an explanation of how
 * acc2omp_ompt_start_tool is intended to be used by OMPT support in an OpenMP
 * runtime, LLVM's in particular.
 ****************************************************************************/

extern "C" ompt_start_tool_result_t *
acc2omp_ompt_start_tool(unsigned int omp_version, const char *runtime_version) {
  // Call all acc_register_library functions.
  acc_register_library(acc_prof_register_enqueue, acc_prof_unregister_enqueue,
                       /*acc_query_fn_name=*/NULL);
  if (const char *env = getenv("ACC_PROFLIB")) {
    char *proflib = strdup(env);
    if (!proflib)
      acc2omp_fatal(ACC2OMP_MSG(alloc_fail));
    char *buf;
    char *fname = strtok_r(proflib, ";", &buf);
    while (fname) {
      void *h = dlopen(fname, RTLD_LAZY);
      if (!h)
        acc2omp_fatal(ACC2OMP_MSG(acc_proflib_fail), dlerror());
      acc_register_library_t register_library =
          (acc_register_library_t)dlsym(h, "acc_register_library");
      if (!register_library)
        acc2omp_fatal(ACC2OMP_MSG(acc_proflib_fail), dlerror());
      register_library(acc_prof_register_enqueue, acc_prof_unregister_enqueue,
                       /*acc_query_fn_name=*/NULL);
      fname = strtok_r(NULL, ";", &buf);
    }
    free(proflib);
  }

  // If an OpenACC callback has been registered, set up OMPT initialize/finalize
  // callbacks.
  if (acc_prof_action_head) {
    static ompt_start_tool_result_t res;
    res.initialize = acc_ompt_initialize;
    res.finalize = acc_ompt_finalize;
    return &res;
  }
  return NULL;
}
