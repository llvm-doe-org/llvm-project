#ifndef LIBOMPTARGET_OMPT_TARGET_H
#define LIBOMPTARGET_OMPT_TARGET_H

#include "omp-tools.h"

#define _OMP_EXTERN extern "C"

#define OMPT_WEAK_ATTRIBUTE __attribute__((weak))

// The following structs are used to pass target-related OMPT callbacks to
// libomptarget. The structs' definitions should be in sync with the definitions
// in libomptarget/src/ompt_internal.h

#ifndef OMPT_FOR_LIBOMPTARGET
/* Bitmap to mark OpenMP 5.1 target events as registered*/
typedef struct ompt_target_callbacks_active_s {
  unsigned int enabled : 1;
#define ompt_event_macro(event, callback, eventid) unsigned int event : 1;

  FOREACH_OMPT_51_TARGET_EVENT(ompt_event_macro)
  FOREACH_OMPT_DEVICE_EXT_EVENT(ompt_event_macro)

#undef ompt_event_macro
} ompt_target_callbacks_active_t;

/* Struct to collect target callback pointers */
typedef struct ompt_target_callbacks_internal_s {
#define ompt_event_macro(event, callback, eventid)                             \
  callback event##_callback;

  FOREACH_OMPT_51_TARGET_EVENT(ompt_event_macro)
  FOREACH_OMPT_DEVICE_EXT_EVENT(ompt_event_macro)

#undef ompt_event_macro
} ompt_target_callbacks_internal_t;

#endif

extern ompt_target_callbacks_active_t ompt_target_enabled;
extern ompt_target_callbacks_internal_t ompt_target_callbacks;

_OMP_EXTERN OMPT_WEAK_ATTRIBUTE bool
libomp_start_tool(
    ompt_target_callbacks_active_t *libomptarget_ompt_enabled,
    ompt_target_callbacks_internal_t *libomptarget_ompt_callbacks);

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
  /// OpenMP runtime API function.
  int (*omp_get_initial_device)(void);
} ompt_plugin_api_t;

#endif // LIBOMPTARGET_OMPT_TARGET_H
