/*
 * ompt-general.cpp -- OMPT implementation of interface functions
 */

//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

/*****************************************************************************
 * system include files
 ****************************************************************************/

#include <assert.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if KMP_OS_UNIX
#include <dlfcn.h>
#endif

// FIXME: This is for OpenACC support only.
#include <map>

/*****************************************************************************
 * ompt include files
 ****************************************************************************/

#include "ompt-specific.cpp"
#include "acc_prof.h"

/*****************************************************************************
 * macros
 ****************************************************************************/

#define ompt_get_callback_success 1
#define ompt_get_callback_failure 0

#define no_tool_present 0

#define OMPT_API_ROUTINE static

#ifndef OMPT_STR_MATCH
#define OMPT_STR_MATCH(haystack, needle) (!strcasecmp(haystack, needle))
#endif

/*****************************************************************************
 * types
 ****************************************************************************/

typedef struct {
  const char *state_name;
  ompt_state_t state_id;
} ompt_state_info_t;

typedef struct {
  const char *name;
  kmp_mutex_impl_t id;
} kmp_mutex_impl_info_t;

enum tool_setting_e {
  omp_tool_error,
  omp_tool_unset,
  omp_tool_disabled,
  omp_tool_enabled
};

/*****************************************************************************
 * global variables
 ****************************************************************************/

ompt_callbacks_active_t ompt_enabled;

ompt_state_info_t ompt_state_info[] = {
#define ompt_state_macro(state, code) {#state, state},
    FOREACH_OMPT_STATE(ompt_state_macro)
#undef ompt_state_macro
};

kmp_mutex_impl_info_t kmp_mutex_impl_info[] = {
#define kmp_mutex_impl_macro(name, id) {#name, name},
    FOREACH_KMP_MUTEX_IMPL(kmp_mutex_impl_macro)
#undef kmp_mutex_impl_macro
};

ompt_callbacks_internal_t ompt_callbacks;

static ompt_start_tool_result_t *ompt_start_tool_result = NULL;

// FIXME: Access to these is not thread-safe.  Does it need to be?
ompt_directive_info_t ompt_directive_info = {ompt_directive_unknown, 0, NULL,
                                             NULL, 0, 0, 0, 0};
const char * const *ompt_data_expressions = NULL;
const char *ompt_data_expression = NULL;
static unsigned ompt_device_inits_capacity = 0;
static unsigned ompt_device_inits_size = 0;
static int32_t *ompt_device_inits = NULL;
bool ompt_in_device_target_region = false;

/*****************************************************************************
 * forward declarations
 ****************************************************************************/

static ompt_interface_fn_t ompt_fn_lookup(const char *s);

OMPT_API_ROUTINE ompt_data_t *ompt_get_thread_data(void);

/*****************************************************************************
 * OpenACC profiling interface.
 *
 * FIXME: Move this to an OpenACC library.
 ****************************************************************************/

// FIXME: Access to this is not thread-safe.  Does it need to be?
static std::map<ompt_id_t, int> acc_ompt_target_device_map;

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
  KMP_ASSERT2(0, "unexpected acc_event_t");
  return NULL;
}

static ompt_set_callback_t acc_ompt_set_callback = NULL;
static ompt_get_directive_info_t acc_ompt_get_directive_info = NULL;
static ompt_get_data_expression_t acc_ompt_get_data_expression = NULL;
static int acc_ompt_initial_device_num;

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
  // FIXME: Is this the right thread ID?  Sometimes this returns
  // KMP_GTID_DNE (-2 in kmp.h) for device and runtime finalization events, but
  // that doesn't seem meaningful to the OpenACC user.  Setting to 0 in that
  // case seems right given that 0 is what __kmp_get_gtid() normally returns
  // and given the following text:
  // OpenMP 5.0 sec. 3.2.4 "omp_get_thread_num" p. 338 L8-9:
  // "The routine returns 0 if it is called from the sequential part of a
  // program."
  ret.thread_id = __kmp_get_gtid();
  // FIXME: So far, it always gives me one of these, so I'd like to notice if
  // it ever returns anything else.
  KMP_ASSERT2(ret.thread_id == 0 || ret.thread_id == KMP_GTID_DNE,
              "__kmp_get_gtid is useful in acc_get_prof_info");
  if (ret.thread_id < 0)
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
      assert(!directive_info->is_explicit_event &&
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
  KMP_ASSERT2(itr != acc_ompt_target_device_map.end(), "unexpected target_id");
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
  KMP_ASSERT2(itr != acc_ompt_target_device_map.end(), "unexpected target_id");
  int device_num = itr->second;
  acc_prof_info pi = acc_get_prof_info(event_type, device_num);
  acc_event_info ei = acc_get_launch_event_info(event_type);
  acc_api_info ai = acc_get_api_info(device_num);
  acc_ev_enqueue_launch_end_callback(&pi, &ei, &ai);
}

static void acc_get_target_data_op_info(
    acc_event_t event_type, ompt_id_t target_id, int device_num, size_t bytes,
    const void *host_ptr, const void *device_ptr, acc_prof_info *pi,
    acc_event_info *ei, acc_api_info *ai) {
  *pi = acc_get_prof_info(event_type, device_num);
  *ei = acc_get_data_event_info(event_type, bytes, host_ptr, device_ptr);
  *ai = acc_get_api_info(device_num);
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
  }
  return 1;
}

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
  }
  KMP_ASSERT2(0, "unexpected ompt_callbacks_t");
}

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
    KMP_ASSERT2(*ompt_reg_counter,
                "expected OMPT callback registration count to be non-zero");
    if (--*ompt_reg_counter)
      continue;
    if (acc_ompt_set_callback(ompt_events[i], NULL) == ompt_set_error)
      fprintf(stderr, "Warning: ompt_set_error result when unregistering "
                      "event: %s\n",
              acc_get_event_name(acc_event));
  }
}

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
 * initialization and finalization (private operations)
 ****************************************************************************/

typedef ompt_start_tool_result_t *(*ompt_start_tool_t)(unsigned int,
                                                       const char *);

#if KMP_OS_DARWIN

// While Darwin supports weak symbols, the library that wishes to provide a new
// implementation has to link against this runtime which defeats the purpose
// of having tools that are agnostic of the underlying runtime implementation.
//
// Fortunately, the linker includes all symbols of an executable in the global
// symbol table by default so dlsym() even finds static implementations of
// ompt_start_tool. For this to work on Linux, -Wl,--export-dynamic needs to be
// passed when building the application which we don't want to rely on.

static ompt_start_tool_result_t *ompt_tool_darwin(unsigned int omp_version,
                                                  const char *runtime_version) {
  ompt_start_tool_result_t *ret = NULL;
  // Search symbol in the current address space.
  ompt_start_tool_t start_tool =
      (ompt_start_tool_t)dlsym(RTLD_DEFAULT, "ompt_start_tool");
  if (start_tool) {
    ret = start_tool(omp_version, runtime_version);
  }
  return ret;
}

#elif OMPT_HAVE_WEAK_ATTRIBUTE

// On Unix-like systems that support weak symbols the following implementation
// of ompt_start_tool() will be used in case no tool-supplied implementation of
// this function is present in the address space of a process.

// FIXME: Move this to an OpenACC library.
// FIXME: This needs implementations in the other branches of this #if.
_OMP_EXTERN OMPT_WEAK_ATTRIBUTE void
acc_register_library(acc_prof_reg reg, acc_prof_reg unref,
                     acc_prof_lookup_func lookup) {}
typedef void (*acc_register_library_t)(acc_prof_reg reg, acc_prof_reg unref,
                                       acc_prof_lookup_func lookup);

_OMP_EXTERN OMPT_WEAK_ATTRIBUTE ompt_start_tool_result_t *
ompt_start_tool(unsigned int omp_version, const char *runtime_version) {
  acc_register_library(acc_prof_register_enqueue,
                       acc_prof_unregister_enqueue,
                       /*acc_query_fn_name=*/NULL);
  const char *proflibs = getenv("ACC_PROFLIB");
  if (proflibs) {
    char *libs = __kmp_str_format("%s", proflibs);
    char *buf;
    char *fname = __kmp_str_token(libs, ";", &buf);
    while (fname) {
      void *h = dlopen(fname, RTLD_LAZY);
      if (!h)
        KMP_FATAL(AccProflibFail, dlerror());
      acc_register_library_t register_library =
          (acc_register_library_t)dlsym(h, "acc_register_library");
      if (!register_library)
        KMP_FATAL(AccProflibFail, dlerror());
      register_library(acc_prof_register_enqueue, acc_prof_unregister_enqueue,
                       /*acc_query_fn_name=*/NULL);
      fname = __kmp_str_token(NULL, ";", &buf);
    }
    __kmp_str_free(&libs);
  }
  if (acc_prof_action_head) {
    static ompt_start_tool_result_t res;
    res.initialize = acc_ompt_initialize;
    res.finalize = acc_ompt_finalize;
    return &res;
  }

  ompt_start_tool_result_t *ret = NULL;
  // Search next symbol in the current address space. This can happen if the
  // runtime library is linked before the tool. Since glibc 2.2 strong symbols
  // don't override weak symbols that have been found before unless the user
  // sets the environment variable LD_DYNAMIC_WEAK.
  ompt_start_tool_t next_tool =
      (ompt_start_tool_t)dlsym(RTLD_NEXT, "ompt_start_tool");
  if (next_tool) {
    ret = next_tool(omp_version, runtime_version);
  }
  return ret;
}

#elif OMPT_HAVE_PSAPI

// On Windows, the ompt_tool_windows function is used to find the
// ompt_start_tool symbol across all modules loaded by a process. If
// ompt_start_tool is found, ompt_start_tool's return value is used to
// initialize the tool. Otherwise, NULL is returned and OMPT won't be enabled.

#include <psapi.h>
#pragma comment(lib, "psapi.lib")

// The number of loaded modules to start enumeration with EnumProcessModules()
#define NUM_MODULES 128

static ompt_start_tool_result_t *
ompt_tool_windows(unsigned int omp_version, const char *runtime_version) {
  int i;
  DWORD needed, new_size;
  HMODULE *modules;
  HANDLE process = GetCurrentProcess();
  modules = (HMODULE *)malloc(NUM_MODULES * sizeof(HMODULE));
  ompt_start_tool_t ompt_tool_p = NULL;

#if OMPT_DEBUG
  printf("ompt_tool_windows(): looking for ompt_start_tool\n");
#endif
  if (!EnumProcessModules(process, modules, NUM_MODULES * sizeof(HMODULE),
                          &needed)) {
    // Regardless of the error reason use the stub initialization function
    free(modules);
    return NULL;
  }
  // Check if NUM_MODULES is enough to list all modules
  new_size = needed / sizeof(HMODULE);
  if (new_size > NUM_MODULES) {
#if OMPT_DEBUG
    printf("ompt_tool_windows(): resize buffer to %d bytes\n", needed);
#endif
    modules = (HMODULE *)realloc(modules, needed);
    // If resizing failed use the stub function.
    if (!EnumProcessModules(process, modules, needed, &needed)) {
      free(modules);
      return NULL;
    }
  }
  for (i = 0; i < new_size; ++i) {
    (FARPROC &)ompt_tool_p = GetProcAddress(modules[i], "ompt_start_tool");
    if (ompt_tool_p) {
#if OMPT_DEBUG
      TCHAR modName[MAX_PATH];
      if (GetModuleFileName(modules[i], modName, MAX_PATH))
        printf("ompt_tool_windows(): ompt_start_tool found in module %s\n",
               modName);
#endif
      free(modules);
      return (*ompt_tool_p)(omp_version, runtime_version);
    }
#if OMPT_DEBUG
    else {
      TCHAR modName[MAX_PATH];
      if (GetModuleFileName(modules[i], modName, MAX_PATH))
        printf("ompt_tool_windows(): ompt_start_tool not found in module %s\n",
               modName);
    }
#endif
  }
  free(modules);
  return NULL;
}
#else
#error Activation of OMPT is not supported on this platform.
#endif

static ompt_start_tool_result_t *
ompt_try_start_tool(unsigned int omp_version, const char *runtime_version) {
  ompt_start_tool_result_t *ret = NULL;
  ompt_start_tool_t start_tool = NULL;
#if KMP_OS_WINDOWS
  // Cannot use colon to describe a list of absolute paths on Windows
  const char *sep = ";";
#else
  const char *sep = ":";
#endif

#if KMP_OS_DARWIN
  // Try in the current address space
  ret = ompt_tool_darwin(omp_version, runtime_version);
#elif OMPT_HAVE_WEAK_ATTRIBUTE
  ret = ompt_start_tool(omp_version, runtime_version);
#elif OMPT_HAVE_PSAPI
  ret = ompt_tool_windows(omp_version, runtime_version);
#else
#error Activation of OMPT is not supported on this platform.
#endif
  if (ret)
    return ret;

  // Try tool-libraries-var ICV
  const char *tool_libs = getenv("OMP_TOOL_LIBRARIES");
  if (tool_libs) {
    char *libs = __kmp_str_format("%s", tool_libs);
    char *buf;
    char *fname = __kmp_str_token(libs, sep, &buf);
    while (fname) {
#if KMP_OS_UNIX
      void *h = dlopen(fname, RTLD_LAZY);
      if (h) {
        start_tool = (ompt_start_tool_t)dlsym(h, "ompt_start_tool");
#elif KMP_OS_WINDOWS
      HMODULE h = LoadLibrary(fname);
      if (h) {
        start_tool = (ompt_start_tool_t)GetProcAddress(h, "ompt_start_tool");
#else
#error Activation of OMPT is not supported on this platform.
#endif
        if (start_tool && (ret = (*start_tool)(omp_version, runtime_version)))
          break;
      }
      fname = __kmp_str_token(NULL, sep, &buf);
    }
    __kmp_str_free(&libs);
  }
  if (ret)
    return ret;

#if KMP_OS_UNIX
  { // Non-standard: load archer tool if application is built with TSan
    const char *fname = "libarcher.so";
    void *h = dlopen(fname, RTLD_LAZY);
    if (h) {
      start_tool = (ompt_start_tool_t)dlsym(h, "ompt_start_tool");
      if (start_tool)
        ret = (*start_tool)(omp_version, runtime_version);
      if (ret)
        return ret;
    }
  }
#endif
  return ret;
}

void ompt_pre_init() {
  //--------------------------------------------------
  // Execute the pre-initialization logic only once.
  //--------------------------------------------------
  static int ompt_pre_initialized = 0;

  if (ompt_pre_initialized)
    return;

  ompt_pre_initialized = 1;

  //--------------------------------------------------
  // Use a tool iff a tool is enabled and available.
  //--------------------------------------------------
  const char *ompt_env_var = getenv("OMP_TOOL");
  tool_setting_e tool_setting = omp_tool_error;

  if (!ompt_env_var || !strcmp(ompt_env_var, ""))
    tool_setting = omp_tool_unset;
  else if (OMPT_STR_MATCH(ompt_env_var, "disabled"))
    tool_setting = omp_tool_disabled;
  else if (OMPT_STR_MATCH(ompt_env_var, "enabled"))
    tool_setting = omp_tool_enabled;

#if OMPT_DEBUG
  printf("ompt_pre_init(): tool_setting = %d\n", tool_setting);
#endif
  switch (tool_setting) {
  case omp_tool_disabled:
    break;

  case omp_tool_unset:
  case omp_tool_enabled:

    //--------------------------------------------------
    // Load tool iff specified in environment variable
    //--------------------------------------------------
    ompt_start_tool_result =
        ompt_try_start_tool(__kmp_openmp_version, ompt_get_runtime_version());

    memset(&ompt_enabled, 0, sizeof(ompt_enabled));
    break;

  case omp_tool_error:
    fprintf(stderr, "Warning: OMP_TOOL has invalid value \"%s\".\n"
                    "  legal values are (NULL,\"\",\"disabled\","
                    "\"enabled\").\n",
            ompt_env_var);
    break;
  }
#if OMPT_DEBUG
  printf("ompt_pre_init(): ompt_enabled = %d\n", ompt_enabled);
#endif
}

extern "C" int omp_get_initial_device(void);

void ompt_post_init() {
  //--------------------------------------------------
  // Execute the post-initialization logic only once.
  //--------------------------------------------------
  static int ompt_post_initialized = 0;

  if (ompt_post_initialized)
    return;

  ompt_post_initialized = 1;

  //--------------------------------------------------
  // Initialize the tool if so indicated.
  //--------------------------------------------------
  if (ompt_start_tool_result) {
    ompt_enabled.enabled = !!ompt_start_tool_result->initialize(
        ompt_fn_lookup, omp_get_initial_device(), &(ompt_start_tool_result->tool_data));

    if (!ompt_enabled.enabled) {
      // tool not enabled, zero out the bitmap, and done
      memset(&ompt_enabled, 0, sizeof(ompt_enabled));
      return;
    }

    kmp_info_t *root_thread = ompt_get_thread();

    ompt_set_thread_state(root_thread, ompt_state_overhead);

    if (ompt_enabled.ompt_callback_thread_begin) {
      ompt_callbacks.ompt_callback(ompt_callback_thread_begin)(
          ompt_thread_initial, __ompt_get_thread_data_internal());
    }
    ompt_data_t *task_data;
    ompt_data_t *parallel_data;
    __ompt_get_task_info_internal(0, NULL, &task_data, NULL, &parallel_data, NULL);
    if (ompt_enabled.ompt_callback_implicit_task) {
      ompt_callbacks.ompt_callback(ompt_callback_implicit_task)(
          ompt_scope_begin, parallel_data, task_data, 1, 1, ompt_task_initial);
    }

    ompt_set_thread_state(root_thread, ompt_state_work_serial);
  }
}

void ompt_fini() {
  // OpenMP 5.0 sec. 2.12.1 p. 160 L12-13:
  // "The device-finalize event for a target device that has been initialized
  // occurs in some thread before an OpenMP implementation shuts down."
  //
  // OpenMP 5.0 sec. 4.5.2.20 p. 484 L12-18:
  // "A registered callback with type signature ompt_callback_device_finalize_t
  // is dispatched for a device immediately prior to finalizing the device.
  // Prior to dispatching a finalization callback for a device on which tracing
  // is active, the OpenMP implementation stops tracing on the device and
  // synchronously flushes all trace records for the device that have not yet
  // been reported. These trace records are flushed through one or more buffer
  // completion callbacks with type signature ompt_callback_buffer_complete_t
  // as needed prior to the dispatch of the callback with type signature
  // ompt_callback_device_finalize_t."
  //
  // Currently, device tracing is not implemented, and there is no device
  // finalization process in libomptarget, so we dispatch these callbacks here
  // for all devices.
  //
  // The above quotes say flushing of traces occurs "prior to dispatching a
  // finalization callback", which occurs "immediately prior to finalizing the
  // device".  This might imply that flushing of traces is prior to and not
  // part of the finalization process, but we assume instead that "finalizing
  // the device" really indicates the end of the finalization process, which
  // can thus include flushing of traces.  Thus, we add
  // ompt_callback_device_finalize_start for the beginning of the finalization
  // process.
  if (ompt_enabled.enabled) {
    if (ompt_enabled.ompt_callback_device_finalize_start ||
        ompt_enabled.ompt_callback_device_finalize) {
      for (int i = ompt_device_inits_size - 1; i >= 0; --i) {
        int32_t device_num = ompt_device_inits[i];
        if (ompt_enabled.ompt_callback_device_finalize_start) {
          ompt_callbacks.ompt_callback(ompt_callback_device_finalize_start)(
            device_num);
        }
        // TODO: Based on the above discussion, flushing of traces goes here.
        if (ompt_enabled.ompt_callback_device_finalize) {
          ompt_callbacks.ompt_callback(ompt_callback_device_finalize)(
            device_num);
        }
      }
      ompt_device_inits_capacity = ompt_device_inits_size = 0;
      KMP_INTERNAL_FREE(ompt_device_inits);
      ompt_device_inits = NULL;
    }
    ompt_start_tool_result->finalize(&(ompt_start_tool_result->tool_data));
  }

  memset(&ompt_enabled, 0, sizeof(ompt_enabled));
}

ompt_callbacks_active_t ompt_get_enabled() {
  return ompt_enabled;
}

ompt_callbacks_internal_t ompt_get_callbacks() {
  return ompt_callbacks;
}

void ompt_record_device_init(int32_t device_num) {
  if (!ompt_device_inits_capacity) {
    ompt_device_inits_capacity = 4;
    ompt_device_inits = (int32_t *)KMP_INTERNAL_MALLOC(
        ompt_device_inits_capacity * sizeof *ompt_device_inits);
    if (!ompt_device_inits)
      KMP_FATAL(MemoryAllocFailed);
  } else if (ompt_device_inits_capacity < ompt_device_inits_size + 1) {
    ompt_device_inits_capacity *= 2;
    ompt_device_inits = (int32_t *)KMP_INTERNAL_REALLOC(
        ompt_device_inits,
        ompt_device_inits_capacity * sizeof *ompt_device_inits);
  }
  ompt_device_inits[ompt_device_inits_size++] = device_num;
}

void ompt_toggle_in_device_target_region() {
  ompt_in_device_target_region = !ompt_in_device_target_region;
}

const char *ompt_index_data_expressions(uint32_t i) {
  if (!ompt_data_expressions)
    return NULL;
  return ompt_data_expressions[i];
}

void ompt_set_data_expression(const char *expr) {
  ompt_data_expression = expr;
}

/*****************************************************************************
 * interface operations
 ****************************************************************************/

/*****************************************************************************
 * state
 ****************************************************************************/

OMPT_API_ROUTINE int ompt_enumerate_states(int current_state, int *next_state,
                                           const char **next_state_name) {
  const static int len = sizeof(ompt_state_info) / sizeof(ompt_state_info_t);
  int i = 0;

  for (i = 0; i < len - 1; i++) {
    if (ompt_state_info[i].state_id == current_state) {
      *next_state = ompt_state_info[i + 1].state_id;
      *next_state_name = ompt_state_info[i + 1].state_name;
      return 1;
    }
  }

  return 0;
}

OMPT_API_ROUTINE int ompt_enumerate_mutex_impls(int current_impl,
                                                int *next_impl,
                                                const char **next_impl_name) {
  const static int len =
      sizeof(kmp_mutex_impl_info) / sizeof(kmp_mutex_impl_info_t);
  int i = 0;
  for (i = 0; i < len - 1; i++) {
    if (kmp_mutex_impl_info[i].id != current_impl)
      continue;
    *next_impl = kmp_mutex_impl_info[i + 1].id;
    *next_impl_name = kmp_mutex_impl_info[i + 1].name;
    return 1;
  }
  return 0;
}

/*****************************************************************************
 * callbacks
 ****************************************************************************/

OMPT_API_ROUTINE ompt_set_result_t ompt_set_callback(ompt_callbacks_t which,
                                       ompt_callback_t callback) {
  switch (which) {

#define ompt_event_macro(event_name, callback_type, event_id)                  \
  case event_name:                                                             \
    ompt_callbacks.ompt_callback(event_name) = (callback_type)callback;        \
    ompt_enabled.event_name = (callback != 0);                                 \
    if (callback)                                                              \
      return ompt_event_implementation_status(event_name);                     \
    else                                                                       \
      return ompt_set_always;

    FOREACH_OMPT_EVENT(ompt_event_macro)

#undef ompt_event_macro

  default:
    return ompt_set_error;
  }
}

OMPT_API_ROUTINE int ompt_get_callback(ompt_callbacks_t which,
                                       ompt_callback_t *callback) {
  if (!ompt_enabled.enabled)
    return ompt_get_callback_failure;

  switch (which) {

#define ompt_event_macro(event_name, callback_type, event_id)                  \
  case event_name: {                                                           \
    ompt_callback_t mycb =                                                     \
        (ompt_callback_t)ompt_callbacks.ompt_callback(event_name);             \
    if (ompt_enabled.event_name && mycb) {                                     \
      *callback = mycb;                                                        \
      return ompt_get_callback_success;                                        \
    }                                                                          \
    return ompt_get_callback_failure;                                          \
  }

    FOREACH_OMPT_EVENT(ompt_event_macro)

#undef ompt_event_macro

  default:
    return ompt_get_callback_failure;
  }
}

/*****************************************************************************
 * parallel regions
 ****************************************************************************/

OMPT_API_ROUTINE int ompt_get_parallel_info(int ancestor_level,
                                            ompt_data_t **parallel_data,
                                            int *team_size) {
  if (!ompt_enabled.enabled)
    return 0;
  return __ompt_get_parallel_info_internal(ancestor_level, parallel_data,
                                           team_size);
}

OMPT_API_ROUTINE int ompt_get_state(ompt_wait_id_t *wait_id) {
  if (!ompt_enabled.enabled)
    return ompt_state_work_serial;
  int thread_state = __ompt_get_state_internal(wait_id);

  if (thread_state == ompt_state_undefined) {
    thread_state = ompt_state_work_serial;
  }

  return thread_state;
}

/*****************************************************************************
 * tasks
 ****************************************************************************/

OMPT_API_ROUTINE ompt_data_t *ompt_get_thread_data(void) {
  if (!ompt_enabled.enabled)
    return NULL;
  return __ompt_get_thread_data_internal();
}

OMPT_API_ROUTINE int ompt_get_task_info(int ancestor_level, int *type,
                                        ompt_data_t **task_data,
                                        ompt_frame_t **task_frame,
                                        ompt_data_t **parallel_data,
                                        int *thread_num) {
  if (!ompt_enabled.enabled)
    return 0;
  return __ompt_get_task_info_internal(ancestor_level, type, task_data,
                                       task_frame, parallel_data, thread_num);
}

OMPT_API_ROUTINE int ompt_get_task_memory(void **addr, size_t *size,
                                          int block) {
  return __ompt_get_task_memory_internal(addr, size, block);
}

/*****************************************************************************
 * num_procs
 ****************************************************************************/

OMPT_API_ROUTINE int ompt_get_num_procs(void) {
  // copied from kmp_ftn_entry.h (but modified: OMPT can only be called when
  // runtime is initialized)
  return __kmp_avail_proc;
}

/*****************************************************************************
 * places
 ****************************************************************************/

OMPT_API_ROUTINE int ompt_get_num_places(void) {
// copied from kmp_ftn_entry.h (but modified)
#if !KMP_AFFINITY_SUPPORTED
  return 0;
#else
  if (!KMP_AFFINITY_CAPABLE())
    return 0;
  return __kmp_affinity_num_masks;
#endif
}

OMPT_API_ROUTINE int ompt_get_place_proc_ids(int place_num, int ids_size,
                                             int *ids) {
// copied from kmp_ftn_entry.h (but modified)
#if !KMP_AFFINITY_SUPPORTED
  return 0;
#else
  int i, count;
  int tmp_ids[ids_size];
  if (!KMP_AFFINITY_CAPABLE())
    return 0;
  if (place_num < 0 || place_num >= (int)__kmp_affinity_num_masks)
    return 0;
  /* TODO: Is this safe for asynchronous call from signal handler during runtime
   * shutdown? */
  kmp_affin_mask_t *mask = KMP_CPU_INDEX(__kmp_affinity_masks, place_num);
  count = 0;
  KMP_CPU_SET_ITERATE(i, mask) {
    if ((!KMP_CPU_ISSET(i, __kmp_affin_fullMask)) ||
        (!KMP_CPU_ISSET(i, mask))) {
      continue;
    }
    if (count < ids_size)
      tmp_ids[count] = i;
    count++;
  }
  if (ids_size >= count) {
    for (i = 0; i < count; i++) {
      ids[i] = tmp_ids[i];
    }
  }
  return count;
#endif
}

OMPT_API_ROUTINE int ompt_get_place_num(void) {
// copied from kmp_ftn_entry.h (but modified)
#if !KMP_AFFINITY_SUPPORTED
  return -1;
#else
  if (!ompt_enabled.enabled || __kmp_get_gtid() < 0)
    return -1;

  int gtid;
  kmp_info_t *thread;
  if (!KMP_AFFINITY_CAPABLE())
    return -1;
  gtid = __kmp_entry_gtid();
  thread = __kmp_thread_from_gtid(gtid);
  if (thread == NULL || thread->th.th_current_place < 0)
    return -1;
  return thread->th.th_current_place;
#endif
}

OMPT_API_ROUTINE int ompt_get_partition_place_nums(int place_nums_size,
                                                   int *place_nums) {
// copied from kmp_ftn_entry.h (but modified)
#if !KMP_AFFINITY_SUPPORTED
  return 0;
#else
  if (!ompt_enabled.enabled || __kmp_get_gtid() < 0)
    return 0;

  int i, gtid, place_num, first_place, last_place, start, end;
  kmp_info_t *thread;
  if (!KMP_AFFINITY_CAPABLE())
    return 0;
  gtid = __kmp_entry_gtid();
  thread = __kmp_thread_from_gtid(gtid);
  if (thread == NULL)
    return 0;
  first_place = thread->th.th_first_place;
  last_place = thread->th.th_last_place;
  if (first_place < 0 || last_place < 0)
    return 0;
  if (first_place <= last_place) {
    start = first_place;
    end = last_place;
  } else {
    start = last_place;
    end = first_place;
  }
  if (end - start <= place_nums_size)
    for (i = 0, place_num = start; place_num <= end; ++place_num, ++i) {
      place_nums[i] = place_num;
    }
  return end - start + 1;
#endif
}

/*****************************************************************************
 * places
 ****************************************************************************/

OMPT_API_ROUTINE int ompt_get_proc_id(void) {
  if (!ompt_enabled.enabled || __kmp_get_gtid() < 0)
    return -1;
#if KMP_OS_LINUX
  return sched_getcpu();
#elif KMP_OS_WINDOWS
  PROCESSOR_NUMBER pn;
  GetCurrentProcessorNumberEx(&pn);
  return 64 * pn.Group + pn.Number;
#else
  return -1;
#endif
}

/*****************************************************************************
 * compatability
 ****************************************************************************/

/*
 * Currently unused function
OMPT_API_ROUTINE int ompt_get_ompt_version() { return OMPT_VERSION; }
*/

/*****************************************************************************
* application-facing API
 ****************************************************************************/

/*----------------------------------------------------------------------------
 | control
 ---------------------------------------------------------------------------*/

int __kmp_control_tool(uint64_t command, uint64_t modifier, void *arg) {

  if (ompt_enabled.enabled) {
    if (ompt_enabled.ompt_callback_control_tool) {
      return ompt_callbacks.ompt_callback(ompt_callback_control_tool)(
          command, modifier, arg, OMPT_LOAD_RETURN_ADDRESS(__kmp_entry_gtid()));
    } else {
      return -1;
    }
  } else {
    return -2;
  }
}

/*****************************************************************************
 * misc
 ****************************************************************************/

// FIXME: We expose this one to libomptarget, so we drop the OMPT_API_ROUTINE,
// which makes it static.  Should we?
uint64_t ompt_get_unique_id(void) {
  return __ompt_get_unique_id_internal();
}

OMPT_API_ROUTINE void ompt_finalize_tool(void) { __kmp_internal_end_atexit(); }

/*****************************************************************************
 * Target
 ****************************************************************************/

OMPT_API_ROUTINE int ompt_get_target_info(uint64_t *device_num,
                                          ompt_id_t *target_id,
                                          ompt_id_t *host_op_id) {
  return 0; // thread is not in a target region
}

OMPT_API_ROUTINE int ompt_get_num_devices(void) {
  return 1; // only one device (the current device) is available
}

/*****************************************************************************
 * Extensions originally added for OpenACC support
 ****************************************************************************/

OMPT_API_ROUTINE ompt_directive_info_t *ompt_get_directive_info(void) {
  return &ompt_directive_info;
}

OMPT_API_ROUTINE const char *ompt_get_data_expression(void) {
  return ompt_data_expression;
}

/*****************************************************************************
 * API inquiry for tool
 ****************************************************************************/

static ompt_interface_fn_t ompt_fn_lookup(const char *s) {

#define ompt_interface_fn(fn)                                                  \
  fn##_t fn##_f = fn;                                                          \
  if (strcmp(s, #fn) == 0)                                                     \
    return (ompt_interface_fn_t)fn##_f;

  FOREACH_OMPT_INQUIRY_FN(ompt_interface_fn)

  return (ompt_interface_fn_t)0;
}
