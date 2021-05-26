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
#include <string>
#include <string.h>
#if KMP_OS_UNIX
#include <dlfcn.h>
#endif

/*****************************************************************************
 * ompt include files
 ****************************************************************************/

#include "acc2omp-handlers.h"
#include "ompt-specific.cpp"

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

// prints for an enabled OMP_TOOL_VERBOSE_INIT.
// In the future a prefix could be added in the first define, the second define
// omits the prefix to allow for continued lines. Example: "PREFIX: Start
// tool... Success." instead of "PREFIX: Start tool... PREFIX: Success."
#define OMPT_VERBOSE_INIT_PRINT(...)                                           \
  if (verbose_init)                                                            \
  fprintf(verbose_file, __VA_ARGS__)
#define OMPT_VERBOSE_INIT_CONTINUED_PRINT(...)                                 \
  if (verbose_init)                                                            \
  fprintf(verbose_file, __VA_ARGS__)

static FILE *verbose_file;
static int verbose_init;

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

#if KMP_OS_WINDOWS
static HMODULE ompt_tool_module = NULL;
#define OMPT_DLCLOSE(Lib) FreeLibrary(Lib)
#else
static void *ompt_tool_module = NULL;
#define OMPT_DLCLOSE(Lib) dlclose(Lib)
#endif

// FIXME: Access to these is not thread-safe.  Does it need to be?
bool ompt_has_user_source_info = false;
// kind and is_explicit_event fields of ompt_user_source_info are not used.
// They're taken from ompt_trigger_ident if set.
ompt_trigger_info_t ompt_user_source_info;
static const ident_t *ompt_trigger_ident = nullptr;
static bool ompt_trigger_ident_parsed = false;
static map_var_info_t ompt_map_var_info = NULL;
static bool ompt_map_var_info_parsed = false;
static unsigned ompt_device_inits_capacity = 0;
static unsigned ompt_device_inits_size = 0;
static int32_t *ompt_device_inits = NULL;
static uint64_t ompt_target_device = UINT64_MAX;

/*****************************************************************************
 * forward declarations
 ****************************************************************************/

static ompt_interface_fn_t ompt_fn_lookup(const char *s);

OMPT_API_ROUTINE ompt_data_t *ompt_get_thread_data(void);

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

_OMP_EXTERN OMPT_WEAK_ATTRIBUTE ompt_start_tool_result_t *
ompt_start_tool(unsigned int omp_version, const char *runtime_version) {
  ompt_start_tool_result_t *ret = NULL;

  // If acc2omp_ompt_start_tool is available, assume it's the LLVM OpenACC
  // runtime's wrapper around ompt_start_tool for supporting the OpenACC
  // Profiling Interface.
  acc2omp_ompt_start_tool_t acc2omp_ompt_start_tool =
      (acc2omp_ompt_start_tool_t)dlsym(RTLD_DEFAULT, "acc2omp_ompt_start_tool");
  if (acc2omp_ompt_start_tool) {
    ret = acc2omp_ompt_start_tool(omp_version, runtime_version);
    if (ret)
      return ret;
  }

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

  OMPT_VERBOSE_INIT_PRINT("----- START LOGGING OF TOOL REGISTRATION -----\n");
  OMPT_VERBOSE_INIT_PRINT("Search for OMP tool in current address space... ");

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
  if (ret) {
    OMPT_VERBOSE_INIT_CONTINUED_PRINT("Success.\n");
    OMPT_VERBOSE_INIT_PRINT(
        "Tool was started and is using the OMPT interface.\n");
    OMPT_VERBOSE_INIT_PRINT("----- END LOGGING OF TOOL REGISTRATION -----\n");
    return ret;
  }

  // Try tool-libraries-var ICV
  OMPT_VERBOSE_INIT_CONTINUED_PRINT("Failed.\n");
  const char *tool_libs = getenv("OMP_TOOL_LIBRARIES");
  if (tool_libs) {
    OMPT_VERBOSE_INIT_PRINT("Searching tool libraries...\n");
    OMPT_VERBOSE_INIT_PRINT("OMP_TOOL_LIBRARIES = %s\n", tool_libs);
    char *libs = __kmp_str_format("%s", tool_libs);
    char *buf;
    char *fname = __kmp_str_token(libs, sep, &buf);
    // Reset dl-error
    dlerror();

    while (fname) {
#if KMP_OS_UNIX
      OMPT_VERBOSE_INIT_PRINT("Opening %s... ", fname);
      void *h = dlopen(fname, RTLD_LAZY);
      if (!h) {
        OMPT_VERBOSE_INIT_CONTINUED_PRINT("Failed: %s\n", dlerror());
      } else {
        OMPT_VERBOSE_INIT_CONTINUED_PRINT("Success. \n");
        OMPT_VERBOSE_INIT_PRINT("Searching for ompt_start_tool in %s... ",
                                fname);
        start_tool = (ompt_start_tool_t)dlsym(h, "ompt_start_tool");
        if (!start_tool) {
          OMPT_VERBOSE_INIT_CONTINUED_PRINT("Failed: %s\n", dlerror());
        } else
#elif KMP_OS_WINDOWS
      OMPT_VERBOSE_INIT_PRINT("Opening %s... ", fname);
      HMODULE h = LoadLibrary(fname);
      if (!h) {
        OMPT_VERBOSE_INIT_CONTINUED_PRINT("Failed: Error %u\n", GetLastError());
      } else {
        OMPT_VERBOSE_INIT_CONTINUED_PRINT("Success. \n");
        OMPT_VERBOSE_INIT_PRINT("Searching for ompt_start_tool in %s... ",
                                fname);
        start_tool = (ompt_start_tool_t)GetProcAddress(h, "ompt_start_tool");
        if (!start_tool) {
          OMPT_VERBOSE_INIT_CONTINUED_PRINT("Failed: Error %s\n",
                                            GetLastError());
        } else
#else
#error Activation of OMPT is not supported on this platform.
#endif
        { // if (start_tool)
          ret = (*start_tool)(omp_version, runtime_version);
          if (ret) {
            OMPT_VERBOSE_INIT_CONTINUED_PRINT("Success.\n");
            OMPT_VERBOSE_INIT_PRINT(
                "Tool was started and is using the OMPT interface.\n");
            ompt_tool_module = h;
            break;
          }
          OMPT_VERBOSE_INIT_CONTINUED_PRINT(
              "Found but not using the OMPT interface.\n");
          OMPT_VERBOSE_INIT_PRINT("Continuing search...\n");
        }
        OMPT_DLCLOSE(h);
      }
      fname = __kmp_str_token(NULL, sep, &buf);
    }
    __kmp_str_free(&libs);
  } else {
    OMPT_VERBOSE_INIT_PRINT("No OMP_TOOL_LIBRARIES defined.\n");
  }

  // usable tool found in tool-libraries
  if (ret) {
    OMPT_VERBOSE_INIT_PRINT("----- END LOGGING OF TOOL REGISTRATION -----\n");
    return ret;
  }

#if KMP_OS_UNIX
  { // Non-standard: load archer tool if application is built with TSan
    const char *fname = "libarcher.so";
    OMPT_VERBOSE_INIT_PRINT(
        "...searching tool libraries failed. Using archer tool.\n");
    OMPT_VERBOSE_INIT_PRINT("Opening %s... ", fname);
    void *h = dlopen(fname, RTLD_LAZY);
    if (h) {
      OMPT_VERBOSE_INIT_CONTINUED_PRINT("Success.\n");
      OMPT_VERBOSE_INIT_PRINT("Searching for ompt_start_tool in %s... ", fname);
      start_tool = (ompt_start_tool_t)dlsym(h, "ompt_start_tool");
      if (start_tool) {
        ret = (*start_tool)(omp_version, runtime_version);
        if (ret) {
          OMPT_VERBOSE_INIT_CONTINUED_PRINT("Success.\n");
          OMPT_VERBOSE_INIT_PRINT(
              "Tool was started and is using the OMPT interface.\n");
          OMPT_VERBOSE_INIT_PRINT(
              "----- END LOGGING OF TOOL REGISTRATION -----\n");
          return ret;
        }
        OMPT_VERBOSE_INIT_CONTINUED_PRINT(
            "Found but not using the OMPT interface.\n");
      } else {
        OMPT_VERBOSE_INIT_CONTINUED_PRINT("Failed: %s\n", dlerror());
      }
    }
  }
#endif
  OMPT_VERBOSE_INIT_PRINT("No OMP tool loaded.\n");
  OMPT_VERBOSE_INIT_PRINT("----- END LOGGING OF TOOL REGISTRATION -----\n");
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

  const char *ompt_env_verbose_init = getenv("OMP_TOOL_VERBOSE_INIT");
  // possible options: disabled | stdout | stderr | <filename>
  // if set, not empty and not disabled -> prepare for logging
  if (ompt_env_verbose_init && strcmp(ompt_env_verbose_init, "") &&
      !OMPT_STR_MATCH(ompt_env_verbose_init, "disabled")) {
    verbose_init = 1;
    if (OMPT_STR_MATCH(ompt_env_verbose_init, "STDERR"))
      verbose_file = stderr;
    else if (OMPT_STR_MATCH(ompt_env_verbose_init, "STDOUT"))
      verbose_file = stdout;
    else
      verbose_file = fopen(ompt_env_verbose_init, "w");
  } else
    verbose_init = 0;

#if OMPT_DEBUG
  printf("ompt_pre_init(): tool_setting = %d\n", tool_setting);
#endif
  switch (tool_setting) {
  case omp_tool_disabled:
    OMPT_VERBOSE_INIT_PRINT("OMP tool disabled. \n");
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
    fprintf(stderr,
            "Warning: OMP_TOOL has invalid value \"%s\".\n"
            "  legal values are (NULL,\"\",\"disabled\","
            "\"enabled\").\n",
            ompt_env_var);
    break;
  }
  if (verbose_init && verbose_file != stderr && verbose_file != stdout)
    fclose(verbose_file);
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
        ompt_fn_lookup, omp_get_initial_device(),
        &(ompt_start_tool_result->tool_data));

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
    __ompt_get_task_info_internal(0, NULL, &task_data, NULL, &parallel_data,
                                  NULL);
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

  if (ompt_tool_module)
    OMPT_DLCLOSE(ompt_tool_module);
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

void ompt_set_target_info(uint64_t device_num) {
  ompt_target_device = device_num;
}

void ompt_clear_target_info() { ompt_target_device = UINT64_MAX; }

void ompt_set_trigger_ident(const ident_t *ident) {
  ompt_trigger_ident = ident;
  ompt_trigger_ident_parsed = false;
}

void ompt_clear_trigger_ident() {
  ompt_trigger_ident = nullptr;
  ompt_trigger_ident_parsed = false;
}

void ompt_set_map_var_info(map_var_info_t map_var_info) {
  ompt_map_var_info = map_var_info;
  ompt_map_var_info_parsed = false;
}

void ompt_clear_map_var_info() {
  ompt_map_var_info = nullptr;
  ompt_map_var_info_parsed = false;
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
  for (int j = 0; j < ids_size; j++)
    tmp_ids[j] = 0;
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

// FIXME: We also need to declare this one in ompt-internal.h, so we drop the
// OMPT_API_ROUTINE, which makes it static.  Should we?
int ompt_get_target_info(uint64_t *device_num, ompt_id_t *target_id,
                         ompt_id_t *host_op_id) {
  if (ompt_target_device == UINT64_MAX)
    return 0; // thread is not in a target region
  if (device_num)
    *device_num = ompt_target_device;
  // FIXME: We haven't implemented target_id and host_op_id yet because we
  // don't yet need them for OpenACC support.
  return 1;
}

OMPT_API_ROUTINE int ompt_get_num_devices(void) {
  return 1; // only one device (the current device) is available
}

/*****************************************************************************
 * Extensions originally added for OpenACC support
 ****************************************************************************/

class IdentParser {
private:
  const char *Begin;
  const char *End;
public:
  IdentParser(const char *Ident) : Begin(Ident), End(Ident) {
    KMP_ASSERT2(*Begin == ';', "expected first char to be ';'");
  }
  std::string next() {
    Begin = End;
    for (End = Begin + 1; *End != ';'; ++End)
      KMP_ASSERT2(*End != '\0', "expected next field in ident_t::psource");
    return std::string(Begin + 1, End - Begin - 1);
  }
};

OMPT_API_ROUTINE ompt_trigger_info_t *ompt_get_trigger_info(void) {
  // Static so they can be returned to caller, which must copy them elsewhere
  // before the next call overwrites them.
  static ompt_trigger_info_t TriggerInfo;
  static std::string SrcFile;
  static std::string FuncName;

  // Parse all fields from ompt_trigger_ident.psource.  However, if we've
  // already parsed it since the last time it was set, don't do it again.  That
  // would waste time and would needlessly invalidate any strings we returned
  // previously.
  if (!ompt_trigger_ident) {
    TriggerInfo.kind = ompt_trigger_unknown;
    TriggerInfo.is_explicit_event = 0;
    TriggerInfo.src_file = nullptr;
    TriggerInfo.func_name = nullptr;
    TriggerInfo.line_no = 0;
    TriggerInfo.end_line_no = 0;
    TriggerInfo.func_line_no = 0;
    TriggerInfo.func_end_line_no = 0;
  } else if (!ompt_trigger_ident_parsed) {
    ompt_trigger_ident_parsed = true;
    IdentParser TheIdentParser(ompt_trigger_ident->psource);
    SrcFile = TheIdentParser.next();
    if (SrcFile.empty())
      TriggerInfo.src_file = nullptr;
    else
      TriggerInfo.src_file = SrcFile.c_str();
    FuncName = TheIdentParser.next();
    if (FuncName.empty())
      TriggerInfo.func_name = nullptr;
    else
      TriggerInfo.func_name = FuncName.c_str();
    TriggerInfo.line_no = std::stoi(TheIdentParser.next());
    // Skip column, which ompt_trigger_info_t doesn't have.
    TheIdentParser.next();
    // If the next field is empty, there are no more fields.
    std::string EndLineNo = TheIdentParser.next();
    if (EndLineNo.empty()) {
      TriggerInfo.end_line_no = 0;
      TriggerInfo.func_line_no = 0;
      TriggerInfo.func_end_line_no = 0;
      TriggerInfo.kind = ompt_trigger_unknown;
      TriggerInfo.is_explicit_event = 0;
    } else {
      TriggerInfo.end_line_no = std::stoi(EndLineNo);
      TriggerInfo.func_line_no = std::stoi(TheIdentParser.next());
      TriggerInfo.func_end_line_no = std::stoi(TheIdentParser.next());
      TriggerInfo.kind = (ompt_trigger_kind_t)std::stoi(TheIdentParser.next());
      TriggerInfo.is_explicit_event = TriggerInfo.kind != ompt_trigger_unknown;
    }
  }

  // Override fields with user-supplied source info if the omp_set_source_info
  // has been called since the last omp_clear_source_info.
  if (ompt_has_user_source_info) {
    TriggerInfo.src_file = ompt_user_source_info.src_file;
    TriggerInfo.func_name = ompt_user_source_info.func_name;
    TriggerInfo.line_no = ompt_user_source_info.line_no;
    TriggerInfo.end_line_no = ompt_user_source_info.end_line_no;
    TriggerInfo.func_line_no = ompt_user_source_info.func_line_no;
    TriggerInfo.func_end_line_no = ompt_user_source_info.func_end_line_no;
  }
  return &TriggerInfo;
}

OMPT_API_ROUTINE const char *ompt_get_data_expression(void) {
  // Static so it can be returned to caller, which must copy it elsewhere before
  // the next call overwrites it.
  static std::string DataExpression;
  if (!ompt_map_var_info)
    return nullptr;
  // If we've already parsed it since the last time it was set, don't do it
  // again.  That would waste time and would needlessly invalidate the string we
  // returned previously.
  if (ompt_map_var_info_parsed)
    return DataExpression.c_str();
  ompt_map_var_info_parsed = true;
  IdentParser TheIdentParser(reinterpret_cast<const char *>(ompt_map_var_info));
  DataExpression = TheIdentParser.next();
  return DataExpression.c_str();
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

  return NULL;
}
