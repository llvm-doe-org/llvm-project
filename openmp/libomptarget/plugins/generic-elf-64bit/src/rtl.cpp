//===-RTLs/generic-64bit/src/rtl.cpp - Target RTLs Implementation - C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// RTL for generic 64-bit machine
//
//===----------------------------------------------------------------------===//

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <ffi.h>
#include <gelf.h>
#include <link.h>
#include <list>
#include <string>
#include <vector>

#include "Debug.h"
#include "omptargetplugin.h"

#ifndef TARGET_NAME
#define TARGET_NAME Generic ELF - 64bit
#endif
#define DEBUG_PREFIX "TARGET " GETNAME(TARGET_NAME) " RTL"

#ifndef TARGET_ELF_ID
#define TARGET_ELF_ID 0
#endif

#ifndef TARGET_OMP_DEVICE_T
#define TARGET_OMP_DEVICE_T omp_device_none
#endif

#include "elf_common.h"

#define NUMBER_OF_DEVICES 4
#define OFFLOADSECTIONNAME "omp_offloading_entries"

/// Array of Dynamic libraries loaded for this target.
struct DynLibTy {
  char *FileName;
  void *Handle;
};

/// Keep entries table per device.
struct FuncOrGblEntryTy {
  __tgt_target_table Table;
};

/// Class containing all the device information.
class RTLDeviceInfoTy {
  std::vector<std::list<FuncOrGblEntryTy>> FuncGblEntries;

public:
  std::list<DynLibTy> DynLibs;

  // Record entry point associated with device.
  void createOffloadTable(int32_t device_id, __tgt_offload_entry *begin,
                          __tgt_offload_entry *end) {
    assert(device_id < (int32_t)FuncGblEntries.size() &&
           "Unexpected device id!");
    FuncGblEntries[device_id].emplace_back();
    FuncOrGblEntryTy &E = FuncGblEntries[device_id].back();

    E.Table.EntriesBegin = begin;
    E.Table.EntriesEnd = end;
  }

  // Return true if the entry is associated with device.
  bool findOffloadEntry(int32_t device_id, void *addr) {
    assert(device_id < (int32_t)FuncGblEntries.size() &&
           "Unexpected device id!");
    FuncOrGblEntryTy &E = FuncGblEntries[device_id].back();

    for (__tgt_offload_entry *i = E.Table.EntriesBegin, *e = E.Table.EntriesEnd;
         i < e; ++i) {
      if (i->addr == addr)
        return true;
    }

    return false;
  }

  // Return the pointer to the target entries table.
  __tgt_target_table *getOffloadEntriesTable(int32_t device_id) {
    assert(device_id < (int32_t)FuncGblEntries.size() &&
           "Unexpected device id!");
    FuncOrGblEntryTy &E = FuncGblEntries[device_id].back();

    return &E.Table;
  }

  RTLDeviceInfoTy(int32_t num_devices) { FuncGblEntries.resize(num_devices); }

  ~RTLDeviceInfoTy() {
    // Close dynamic libraries
    for (auto &lib : DynLibs) {
      if (lib.Handle) {
        dlclose(lib.Handle);
        remove(lib.FileName);
      }
    }
  }
};

static RTLDeviceInfoTy DeviceInfo(NUMBER_OF_DEVICES);

#ifdef __cplusplus
extern "C" {
#endif

int32_t __tgt_rtl_get_device_type() { return TARGET_OMP_DEVICE_T; }

int32_t __tgt_rtl_is_valid_binary(__tgt_device_image *image) {
// If we don't have a valid ELF ID we can just fail.
#if TARGET_ELF_ID < 1
  return 0;
#else
  return elf_check_machine(image, TARGET_ELF_ID);
#endif
}

int32_t __tgt_rtl_number_of_devices() { return NUMBER_OF_DEVICES; }

int32_t __tgt_rtl_init_device(int32_t device_id) { return OFFLOAD_SUCCESS; }

__tgt_target_table *__tgt_rtl_load_binary(int32_t device_id,
                                          __tgt_device_image *image) {

  DP("Dev %d: load binary from " DPxMOD " image\n", device_id,
     DPxPTR(image->ImageStart));

  assert(device_id >= 0 && device_id < NUMBER_OF_DEVICES && "bad dev id");

  size_t ImageSize = (size_t)image->ImageEnd - (size_t)image->ImageStart;
  size_t NumEntries = (size_t)(image->EntriesEnd - image->EntriesBegin);
  DP("Expecting to have %zd entries defined.\n", NumEntries);

  // Is the library version incompatible with the header file?
  if (elf_version(EV_CURRENT) == EV_NONE) {
    DP("Incompatible ELF library!\n");
    return NULL;
  }

  // Obtain elf handler
  Elf *e = elf_memory((char *)image->ImageStart, ImageSize);
  if (!e) {
    DP("Unable to get ELF handle: %s!\n", elf_errmsg(-1));
    return NULL;
  }

  if (elf_kind(e) != ELF_K_ELF) {
    DP("Invalid Elf kind!\n");
    elf_end(e);
    return NULL;
  }

  // Find the entries section offset
  Elf_Scn *section = 0;
  Elf64_Off entries_offset = 0;

  size_t shstrndx;

  if (elf_getshdrstrndx(e, &shstrndx)) {
    DP("Unable to get ELF strings index!\n");
    elf_end(e);
    return NULL;
  }

  while ((section = elf_nextscn(e, section))) {
    GElf_Shdr hdr;
    gelf_getshdr(section, &hdr);

    if (!strcmp(elf_strptr(e, shstrndx, hdr.sh_name), OFFLOADSECTIONNAME)) {
      entries_offset = hdr.sh_addr;
      break;
    }
  }

  if (!entries_offset) {
    DP("Entries Section Offset Not Found\n");
    elf_end(e);
    return NULL;
  }

  DP("Offset of entries section is (" DPxMOD ").\n", DPxPTR(entries_offset));

  // load dynamic library and get the entry points. We use the dl library
  // to do the loading of the library, but we could do it directly to avoid the
  // dump to the temporary file.
  //
  // 1) Create tmp file with the library contents.
  // 2) Use dlopen to load the file and dlsym to retrieve the symbols.
  char tmp_name[] = "/tmp/tmpfile_XXXXXX";
  int tmp_fd = mkstemp(tmp_name);

  if (tmp_fd == -1) {
    elf_end(e);
    return NULL;
  }

  FILE *ftmp = fdopen(tmp_fd, "wb");

  if (!ftmp) {
    elf_end(e);
    return NULL;
  }

  fwrite(image->ImageStart, ImageSize, 1, ftmp);
  fclose(ftmp);

  DynLibTy Lib = {tmp_name, dlopen(tmp_name, RTLD_LAZY)};

  if (!Lib.Handle) {
    DP("Target library loading error: %s\n", dlerror());
    elf_end(e);
    return NULL;
  }

  DeviceInfo.DynLibs.push_back(Lib);

  struct link_map *libInfo = (struct link_map *)Lib.Handle;

  // The place where the entries info is loaded is the library base address
  // plus the offset determined from the ELF file.
  Elf64_Addr entries_addr = libInfo->l_addr + entries_offset;

  DP("Pointer to first entry to be loaded is (" DPxMOD ").\n",
     DPxPTR(entries_addr));

  // Table of pointers to all the entries in the target.
  __tgt_offload_entry *entries_table = (__tgt_offload_entry *)entries_addr;

  __tgt_offload_entry *entries_begin = &entries_table[0];
  __tgt_offload_entry *entries_end = entries_begin + NumEntries;

  if (!entries_begin) {
    DP("Can't obtain entries begin\n");
    elf_end(e);
    return NULL;
  }

  DP("Entries table range is (" DPxMOD ")->(" DPxMOD ")\n",
     DPxPTR(entries_begin), DPxPTR(entries_end));
  DeviceInfo.createOffloadTable(device_id, entries_begin, entries_end);

  elf_end(e);

  return DeviceInfo.getOffloadEntriesTable(device_id);
}

// Sample implementation of explicit memory allocator. For this plugin all kinds
// are equivalent to each other.
void *__tgt_rtl_data_alloc(int32_t device_id, int64_t size, void *hst_ptr,
                           int32_t kind) {
  void *ptr = NULL;

  switch (kind) {
  case TARGET_ALLOC_DEVICE:
  case TARGET_ALLOC_HOST:
  case TARGET_ALLOC_SHARED:
  case TARGET_ALLOC_DEFAULT:
    ptr = malloc(size);
    break;
  default:
    REPORT("Invalid target data allocation kind");
  }

  return ptr;
}

int32_t __tgt_rtl_data_submit(
    int32_t device_id, void *tgt_ptr, void *hst_ptr, int64_t size
    OMPT_SUPPORT_IF(, const ompt_plugin_api_t *ompt_api)) {
#if OMPT_SUPPORT
  // OpenMP 5.1, sec. 2.21.7.1 "map Clause", p. 353, L6-7:
  // "The target-data-op-begin event occurs before a thread initiates a data
  // operation on a target device.  The target-data-op-end event occurs after a
  // thread initiates a data operation on a target device."
  //
  // OpenMP 5.1, sec. 3.8.5 "omp_target_memcpy", p. 419, L4-5:
  // "The target-data-op-begin event occurs before a thread initiates a data
  // transfer.  The target-data-op-end event occurs after a thread initiated a
  // data transfer."
  //
  // OpenMP 5.1, sec. 4.5.2.25 "ompt_callback_target_data_op_emi_t and
  // ompt_callback_target_data_op_t", p. 535, L25-27:
  // "A thread dispatches a registered ompt_callback_target_data_op_emi or
  // ompt_callback_target_data_op callback when device memory is allocated or
  // freed, as well as when data is copied to or from a device."
  //
  // There's nothing to actually enqueue as we're about to perform the copy
  // synchronously, so we just dispatch these callbacks back to back.
  //
  // FIXME: We don't yet need the target_task_data, target_data, host_op_id, and
  // codeptr_ra arguments for OpenACC support, so we haven't bothered to
  // implement them yet.
  if (ompt_api->ompt_get_enabled().ompt_callback_target_data_op_emi) {
    ompt_api->ompt_get_callbacks().ompt_callback(
        ompt_callback_target_data_op_emi)(
        ompt_scope_begin, /*target_task_data=*/NULL, /*target_data=*/NULL,
        /*host_op_id=*/NULL, ompt_target_data_transfer_to_device, hst_ptr,
        omp_get_initial_device(), tgt_ptr, ompt_api->global_device_id, size,
        /*codeptr_ra=*/NULL);
    ompt_api->ompt_get_callbacks().ompt_callback(
        ompt_callback_target_data_op_emi)(
        ompt_scope_end, /*target_task_data=*/NULL, /*target_data=*/NULL,
        /*host_op_id=*/NULL, ompt_target_data_transfer_to_device, hst_ptr,
        omp_get_initial_device(), tgt_ptr, ompt_api->global_device_id, size,
        /*codeptr_ra=*/NULL);
  }
#endif
  memcpy(tgt_ptr, hst_ptr, size);
  return OFFLOAD_SUCCESS;
}

int32_t __tgt_rtl_data_retrieve(
    int32_t device_id, void *hst_ptr, void *tgt_ptr, int64_t size
    OMPT_SUPPORT_IF(, const ompt_plugin_api_t *ompt_api)) {
#if OMPT_SUPPORT
  // OpenMP 5.1, sec. 2.21.7.1 "map Clause", p. 353, L6-7:
  // "The target-data-op-begin event occurs before a thread initiates a data
  // operation on a target device.  The target-data-op-end event occurs after a
  // thread initiates a data operation on a target device."
  //
  // OpenMP 5.1, sec. 3.8.5 "omp_target_memcpy", p. 419, L4-5:
  // "The target-data-op-begin event occurs before a thread initiates a data
  // transfer.  The target-data-op-end event occurs after a thread initiated a
  // data transfer."
  //
  // OpenMP 5.1, sec. 4.5.2.25 "ompt_callback_target_data_op_emi_t and
  // ompt_callback_target_data_op_t", p. 535, L25-27:
  // "A thread dispatches a registered ompt_callback_target_data_op_emi or
  // ompt_callback_target_data_op callback when device memory is allocated or
  // freed, as well as when data is copied to or from a device."
  //
  // There's nothing to actually enqueue as we're about to perform the copy
  // synchronously, so we just dispatch these callbacks back to back.
  //
  // FIXME: We don't yet need the target_task_data, target_data, host_op_id, and
  // codeptr_ra arguments for OpenACC support, so we haven't bothered to
  // implement them yet.
  if (ompt_api->ompt_get_enabled().ompt_callback_target_data_op_emi) {
    ompt_api->ompt_get_callbacks().ompt_callback(
        ompt_callback_target_data_op_emi)(
        ompt_scope_begin, /*target_task_data=*/NULL, /*target_data=*/NULL,
        /*host_op_id=*/NULL, ompt_target_data_transfer_from_device, tgt_ptr,
        ompt_api->global_device_id, hst_ptr, omp_get_initial_device(), size,
        /*codeptr_ra=*/NULL);
    ompt_api->ompt_get_callbacks().ompt_callback(
        ompt_callback_target_data_op_emi)(
        ompt_scope_end, /*target_task_data=*/NULL, /*target_data=*/NULL,
        /*host_op_id=*/NULL, ompt_target_data_transfer_from_device, tgt_ptr,
        ompt_api->global_device_id, hst_ptr, omp_get_initial_device(), size,
        /*codeptr_ra=*/NULL);
  }
#endif
  memcpy(hst_ptr, tgt_ptr, size);
  return OFFLOAD_SUCCESS;
}

int32_t __tgt_rtl_data_delete(int32_t device_id, void *tgt_ptr) {
  free(tgt_ptr);
  return OFFLOAD_SUCCESS;
}

int32_t __tgt_rtl_run_target_team_region(
    int32_t device_id, void *tgt_entry_ptr, void **tgt_args,
    ptrdiff_t *tgt_offsets, int32_t arg_num, int32_t team_num,
    int32_t thread_limit, uint64_t loop_tripcount /*not used*/
    OMPT_SUPPORT_IF(, const ompt_plugin_api_t *ompt_api)) {
  // ignore team num and thread limit.

  // Use libffi to launch execution.
  ffi_cif cif;

  // All args are references.
  std::vector<ffi_type *> args_types(arg_num, &ffi_type_pointer);
  std::vector<void *> args(arg_num);
  std::vector<void *> ptrs(arg_num);

  for (int32_t i = 0; i < arg_num; ++i) {
    ptrs[i] = (void *)((intptr_t)tgt_args[i] + tgt_offsets[i]);
    args[i] = &ptrs[i];
  }

  ffi_status status = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, arg_num,
                                   &ffi_type_void, &args_types[0]);

  assert(status == FFI_OK && "Unable to prepare target launch!");

  if (status != FFI_OK)
    return OFFLOAD_FAIL;

#if OMPT_SUPPORT
  // OpenMP 5.1, sec. 2.14.5 "target Construct", p. 201, L17-20:
  // "The target-submit-begin event occurs prior to initiating creation of an
  // initial task on a target device for a target region.  The target-submit-end
  // event occurs after initiating creation of an initial task on a target
  // device for a target region."
  //
  // OpenMP 5.1, sec. 4.5.2.28 "ompt_callback_target_submit_emi_t and
  // ompt_callback_target_submit_t", p. 543, L2-6:
  // "A thread dispatches a registered ompt_callback_target_submit_emi or
  // ompt_callback_target_submit callback on the host before and after a target
  // task initiates creation of an initial task on a device."
  // "The endpoint argument indicates that the callback signals the beginning or
  // end of a scope."
  //
  // There's nothing to actually enqueue as we're about to call the task
  // directly, so we just dispatch these callbacks back to back.
  //
  // FIXME: We don't yet need the target_data or host_op_id argument for OpenACC
  // support, so we haven't bothered to implement it yet.
  if (ompt_api->ompt_get_enabled().ompt_callback_target_submit_emi) {
    ompt_api->ompt_get_callbacks().ompt_callback(
        ompt_callback_target_submit_emi)(ompt_scope_begin, /*target_data=*/NULL,
                                         /*host_op_id=*/NULL,
                                         /*requested_num_teams=*/team_num);
    ompt_api->ompt_get_callbacks().ompt_callback(
        ompt_callback_target_submit_emi)(ompt_scope_end, /*target_data=*/NULL,
                                         /*host_op_id=*/NULL,
                                         /*requested_num_teams=*/team_num);
  }
#endif

  DP("Running entry point at " DPxMOD "...\n", DPxPTR(tgt_entry_ptr));

  void (*entry)(void);
  *((void **)&entry) = tgt_entry_ptr;
  ffi_call(&cif, entry, NULL, &args[0]);
  return OFFLOAD_SUCCESS;
}

int32_t __tgt_rtl_run_target_region(
    int32_t device_id, void *tgt_entry_ptr, void **tgt_args,
    ptrdiff_t *tgt_offsets, int32_t arg_num
    OMPT_SUPPORT_IF(, const ompt_plugin_api_t *ompt_api)) {
  // use one team and one thread.
  return __tgt_rtl_run_target_team_region(device_id, tgt_entry_ptr, tgt_args,
                                          tgt_offsets, arg_num, 1, 1, 0
                                          OMPT_SUPPORT_IF(, ompt_api));
}

#ifdef __cplusplus
}
#endif
