/*
 * ompt-internal.h - header of OMPT internal data structures
 */

//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef __OMPT_INTERNAL_H__
#define __OMPT_INTERNAL_H__

#include "ompt-event-specific.h"
#include "omp-tools.h"

#define OMPT_VERSION 1

#define _OMP_EXTERN extern "C"

// FIXME: OMPT_FOR_LIBOMPTARGET is defined wherever libomptarget includes this.
// It should probably instead include its own header file, possibly a wrapper
// around this one.  OMPT_SUPPORT and OMPT_OPTIONAL should be set in
// libomptarget's cmake similar to how they're set in runtime's cmake.  For
// now, we define them unconditionally to 1 for libomptarget so we can go ahead
// and guard uses of OMPT there.
#ifdef OMPT_FOR_LIBOMPTARGET
# define OMPT_SUPPORT 1
# define OMPT_OPTIONAL 1
# define OMPT_LIBOMPTARGET_WEAK __attribute__((weak))

# if OMPT_SUPPORT
struct DeviceTy;
void ompt_dispatch_callback_target_emi(ompt_target_t Kind,
                                       ompt_scope_endpoint_t Endpoint,
                                       DeviceTy &Device);
# endif

#else
# define OMPT_LIBOMPTARGET_WEAK
#endif

#if OMPT_SUPPORT
# define OMPT_SUPPORT_IF(...) __VA_ARGS__
#else
# define OMPT_SUPPORT_IF(...)
#endif

/// Type alias for source location information for variable mappings with data
/// layout ";name;filename;row;col;;\0" from clang.
using map_var_info_t = void *;

#define OMPT_INVOKER(x)                                                        \
  ((x == fork_context_gnu) ? ompt_parallel_invoker_program                     \
                           : ompt_parallel_invoker_runtime)

#define ompt_callback(e) e##_callback

#define ompt_emi_callback(e) e##_emi_callback

#define ompt_emi_callback_type(e) e##_emi_t

#define ompt_emi_wrapper(e) e##_emi_wrapper

#define ompt_emi_event(e) e##_emi

/* Struct to collect host callback pointers */
typedef struct ompt_callbacks_internal_s {
#define ompt_event_macro(event, callback, eventid)                             \
  callback ompt_callback(event);

  FOREACH_OMPT_HOST_EVENT(ompt_event_macro)

#undef ompt_event_macro
} ompt_callbacks_internal_t;

/* Struct to collect target callback pointers */
typedef struct ompt_target_callbacks_internal_s {
#define ompt_event_macro(event, callback, eventid)                             \
  callback ompt_callback(event);

  FOREACH_OMPT_51_TARGET_EVENT(ompt_event_macro)
  FOREACH_OMPT_DEVICE_EXT_EVENT(ompt_event_macro)

#undef ompt_event_macro
} ompt_target_callbacks_internal_t;

/* Struct to collect noemi callback pointers */
typedef struct ompt_callbacks_internal_noemi_s {
#define ompt_event_macro(event, callback, eventid)                             \
  callback ompt_callback(event);

  FOREACH_OMPT_NOEMI_EVENT(ompt_event_macro)

#undef ompt_event_macro
} ompt_callbacks_internal_noemi_t;

/* Bitmap to mark OpenMP 5.1 host events as registered*/
typedef struct ompt_callbacks_active_s {
  unsigned int enabled : 1;
#define ompt_event_macro(event, callback, eventid) unsigned int event : 1;

  FOREACH_OMPT_HOST_EVENT(ompt_event_macro)

#undef ompt_event_macro
} ompt_callbacks_active_t;

/* Bitmap to mark OpenMP 5.1 target events as registered*/
typedef struct ompt_target_callbacks_active_s {
  unsigned int enabled : 1;
#define ompt_event_macro(event, callback, eventid) unsigned int event : 1;

  FOREACH_OMPT_51_TARGET_EVENT(ompt_event_macro)
  FOREACH_OMPT_DEVICE_EXT_EVENT(ompt_event_macro)

#undef ompt_event_macro
} ompt_target_callbacks_active_t;

#define TASK_TYPE_DETAILS_FORMAT(info)                                         \
  ((info->td_flags.task_serial || info->td_flags.tasking_ser)                  \
       ? ompt_task_undeferred                                                  \
       : 0x0) |                                                                \
      ((!(info->td_flags.tiedness)) ? ompt_task_untied : 0x0) |                \
      (info->td_flags.final ? ompt_task_final : 0x0) |                         \
      (info->td_flags.merged_if0 ? ompt_task_mergeable : 0x0)

typedef struct {
  ompt_frame_t frame;
  ompt_data_t task_data;
  struct kmp_taskdata *scheduling_parent;
  int thread_num;
} ompt_task_info_t;

typedef struct {
  ompt_data_t parallel_data;
  void *master_return_address;
} ompt_team_info_t;

typedef struct ompt_lw_taskteam_s {
  ompt_team_info_t ompt_team_info;
  ompt_task_info_t ompt_task_info;
  int heap;
  struct ompt_lw_taskteam_s *parent;
} ompt_lw_taskteam_t;

typedef struct {
  ompt_data_t thread_data;
  ompt_data_t task_data; /* stored here from implicit barrier-begin until
                            implicit-task-end */
  void *return_address; /* stored here on entry of runtime */
  ompt_state_t state;
  ompt_wait_id_t wait_id;
  int ompt_task_yielded;
  int parallel_flags; // information for the last parallel region invoked
  void *idle_frame;
} ompt_thread_info_t;

extern ompt_callbacks_internal_t ompt_callbacks;
extern ompt_target_callbacks_internal_t ompt_target_callbacks;
extern ompt_callbacks_internal_noemi_t ompt_callbacks_noemi;

#if OMPT_SUPPORT && OMPT_OPTIONAL
#if USE_FAST_MEMORY
#define KMP_OMPT_DEPS_ALLOC __kmp_fast_allocate
#define KMP_OMPT_DEPS_FREE __kmp_fast_free
#else
#define KMP_OMPT_DEPS_ALLOC __kmp_thread_malloc
#define KMP_OMPT_DEPS_FREE __kmp_thread_free
#endif
#endif /* OMPT_SUPPORT && OMPT_OPTIONAL */

#ifdef __cplusplus
extern "C" {
#endif

void ompt_pre_init(void);
void ompt_post_init(void);
void ompt_fini(void);

#define OMPT_GET_RETURN_ADDRESS(level) __builtin_return_address(level)
#define OMPT_GET_FRAME_ADDRESS(level) __builtin_frame_address(level)

int __kmp_control_tool(uint64_t command, uint64_t modifier, void *arg);

extern ompt_callbacks_active_t ompt_enabled;
extern ompt_target_callbacks_active_t ompt_target_enabled;

uint64_t ompt_get_unique_id(void) OMPT_LIBOMPTARGET_WEAK;
void ompt_record_device_init(int32_t device_num) OMPT_LIBOMPTARGET_WEAK;
int ompt_get_target_info(uint64_t *device_num, ompt_id_t *target_id,
                         ompt_id_t *host_op_id) OMPT_LIBOMPTARGET_WEAK;
void ompt_set_target_info(uint64_t device_num) OMPT_LIBOMPTARGET_WEAK;
void ompt_clear_target_info() OMPT_LIBOMPTARGET_WEAK;
struct ident;
typedef struct ident ident_t;
void ompt_set_trigger_ident(const ident_t *ident) OMPT_LIBOMPTARGET_WEAK;
void ompt_clear_trigger_ident(void) OMPT_LIBOMPTARGET_WEAK;
extern bool ompt_has_user_source_info;
extern ompt_trigger_info_t ompt_user_source_info;
void ompt_set_map_var_info(map_var_info_t map_var_info) OMPT_LIBOMPTARGET_WEAK;
void ompt_clear_map_var_info() OMPT_LIBOMPTARGET_WEAK;
int omp_get_initial_device(void) OMPT_LIBOMPTARGET_WEAK;

/// This struct is passed into target plugins where OMPT callbacks require
/// additional data.
///
/// It may be tempting to export and weakly link libomptarget symbols (functions
/// or data) in order to access them in the plugins.  While that works in many
/// cases, the test openmp/libomptarget/test/offloading/dynamic_module_load.c
/// reveals that, if a dlopen'ed OpenMP application happens to require a
/// plugin function that depends on that symbol, dlsym will then fail.  For
/// example, a plugin's __tgt_rtl_run_target_team_region_async must access
/// ompt_target_enabled and ompt_target_callbacks via this struct instead of
/// directly in order not to break that test.
///
/// FIXME: Some functions are being accessed directly in plugins because they
/// don't happen to be depended upon by plugin functions required in that test.
/// For example, omp_get_initial_device.  If they were called directly from
/// __tgt_rtl_run_target_team_region_async, they would break that test.  Other
/// examples of dlopen are surely already broken as a result but they don't
/// happen to be tested.  We should also pass such functions via this struct.
typedef struct {
  /// This is the same as \c DeviceTy::DeviceID.  (\c DeviceTy::RTLDeviceID is
  /// already passed to some target plugin functions using a parameter name like
  /// \c DeviceID, but it's not the same when there are multiple RTLs loaded.)
  int32_t global_device_id;
  /// Registered callbacks.
  /// @{
  ompt_target_callbacks_active_t *ompt_target_enabled;
  ompt_target_callbacks_internal_t *ompt_target_callbacks;
  /// @}
} ompt_plugin_api_t;

#if KMP_OS_WINDOWS
#define UNLIKELY(x) (x)
#define OMPT_NOINLINE __declspec(noinline)
#else
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#define OMPT_NOINLINE __attribute__((noinline))
#endif

#ifdef __cplusplus
}
#endif

#endif
