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

#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/DynamicLibrary.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ffi.h>
#include <gelf.h>
#include <link.h>
#include <list>
#include <string>
#include <vector>

#include "Debug.h"
#include "omptargetplugin.h"

using namespace llvm;
using namespace llvm::sys;

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
  std::string FileName;
  std::unique_ptr<DynamicLibrary> DynLib;
};

/// Keep entries table per device.
struct FuncOrGblEntryTy {
  __tgt_target_table Table;
  SmallVector<__tgt_offload_entry> Entries;
};

/// Class containing all the device information.
class RTLDeviceInfoTy {
  std::vector<std::list<FuncOrGblEntryTy>> FuncGblEntries;

public:
  std::list<DynLibTy> DynLibs;

  // Record entry point associated with device.
  void createOffloadTable(int32_t DeviceId,
                          SmallVector<__tgt_offload_entry> &&Entries) {
    assert(DeviceId < (int32_t)FuncGblEntries.size() &&
           "Unexpected device id!");
    FuncGblEntries[DeviceId].emplace_back();
    FuncOrGblEntryTy &E = FuncGblEntries[DeviceId].back();

    E.Entries = Entries;
    E.Table.EntriesBegin = E.Entries.begin();
    E.Table.EntriesEnd = E.Entries.end();
  }

  // Return true if the entry is associated with device.
  bool findOffloadEntry(int32_t DeviceId, void *Addr) {
    assert(DeviceId < (int32_t)FuncGblEntries.size() &&
           "Unexpected device id!");
    FuncOrGblEntryTy &E = FuncGblEntries[DeviceId].back();

    for (__tgt_offload_entry *I = E.Table.EntriesBegin,
                             *End = E.Table.EntriesEnd;
         I < End; ++I) {
      if (I->addr == Addr)
        return true;
    }

    return false;
  }

  // Return the pointer to the target entries table.
  __tgt_target_table *getOffloadEntriesTable(int32_t DeviceId) {
    assert(DeviceId < (int32_t)FuncGblEntries.size() &&
           "Unexpected device id!");
    FuncOrGblEntryTy &E = FuncGblEntries[DeviceId].back();

    return &E.Table;
  }

  RTLDeviceInfoTy(int32_t NumDevices) { FuncGblEntries.resize(NumDevices); }

  ~RTLDeviceInfoTy() {
    // Close dynamic libraries
    for (auto &Lib : DynLibs) {
      if (Lib.DynLib->isValid())
        remove(Lib.FileName.c_str());
    }
  }
};

static RTLDeviceInfoTy DeviceInfo(NUMBER_OF_DEVICES);

#ifdef __cplusplus
extern "C" {
#endif

int32_t __tgt_rtl_get_device_type() { return TARGET_OMP_DEVICE_T; }

int32_t __tgt_rtl_is_valid_binary(__tgt_device_image *Image) {
// If we don't have a valid ELF ID we can just fail.
#if TARGET_ELF_ID < 1
  return 0;
#else
  return elf_check_machine(Image, TARGET_ELF_ID);
#endif
}

int32_t __tgt_rtl_number_of_devices() { return NUMBER_OF_DEVICES; }

int32_t __tgt_rtl_init_device(int32_t DeviceId) { return OFFLOAD_SUCCESS; }

__tgt_target_table *__tgt_rtl_load_binary(int32_t DeviceId,
                                          __tgt_device_image *Image) {

  DP("Dev %d: load binary from " DPxMOD " image\n", DeviceId,
     DPxPTR(Image->ImageStart));

  assert(DeviceId >= 0 && DeviceId < NUMBER_OF_DEVICES && "bad dev id");

  size_t ImageSize = (size_t)Image->ImageEnd - (size_t)Image->ImageStart;
  size_t NumEntries = (size_t)(Image->EntriesEnd - Image->EntriesBegin);
  DP("Expecting to have %zd entries defined.\n", NumEntries);

  // load dynamic library and get the entry points. We use the dl library
  // to do the loading of the library, but we could do it directly to avoid the
  // dump to the temporary file.
  //
  // 1) Create tmp file with the library contents.
  // 2) Use dlopen to load the file and dlsym to retrieve the symbols.
  char TmpName[] = "/tmp/tmpfile_XXXXXX";
  int TmpFd = mkstemp(TmpName);

  if (TmpFd == -1)
    return nullptr;

  FILE *Ftmp = fdopen(TmpFd, "wb");

  if (!Ftmp)
    return nullptr;

  fwrite(Image->ImageStart, ImageSize, 1, Ftmp);
  fclose(Ftmp);

  std::string ErrMsg;
  auto DynLib = std::make_unique<sys::DynamicLibrary>(
      sys::DynamicLibrary::getPermanentLibrary(TmpName, &ErrMsg));
  DynLibTy Lib = {TmpName, std::move(DynLib)};

  if (!Lib.DynLib->isValid()) {
    DP("Target library loading error: %s\n", ErrMsg.c_str());
    return NULL;
  }

  __tgt_offload_entry *HostBegin = Image->EntriesBegin;
  __tgt_offload_entry *HostEnd = Image->EntriesEnd;

  // Create a new offloading entry list using the device symbol address.
  SmallVector<__tgt_offload_entry> Entries;
  for (__tgt_offload_entry *E = HostBegin; E != HostEnd; ++E) {
    if (!E->addr)
      return nullptr;

    __tgt_offload_entry Entry = *E;

    void *DevAddr = Lib.DynLib->getAddressOfSymbol(E->name);
    Entry.addr = DevAddr;

    DP("Entry point " DPxMOD " maps to global %s (" DPxMOD ")\n",
       DPxPTR(E - HostBegin), E->name, DPxPTR(DevAddr));

    Entries.emplace_back(Entry);
  }

  DeviceInfo.createOffloadTable(DeviceId, std::move(Entries));
  DeviceInfo.DynLibs.emplace_back(std::move(Lib));

  return DeviceInfo.getOffloadEntriesTable(DeviceId);
}

void __tgt_rtl_print_device_info(int32_t DeviceId) {
  printf("    This is a generic-elf-64bit device\n");
}

// Sample implementation of explicit memory allocator. For this plugin all kinds
// are equivalent to each other.
void *__tgt_rtl_data_alloc(int32_t DeviceId, int64_t Size, void *HstPtr,
                           int32_t Kind) {
  void *Ptr = NULL;

  switch (Kind) {
  case TARGET_ALLOC_DEVICE:
  case TARGET_ALLOC_HOST:
  case TARGET_ALLOC_SHARED:
  case TARGET_ALLOC_DEFAULT:
    Ptr = malloc(Size);
    break;
  default:
    REPORT("Invalid target data allocation kind");
  }

  return Ptr;
}

int32_t __tgt_rtl_data_submit(
    int32_t DeviceId, void *TgtPtr, void *HstPtr, int64_t Size
    OMPT_SUPPORT_IF(, const ompt_plugin_api_t *OmptApi)) {
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
  if (OmptApi->ompt_target_enabled->ompt_callback_target_data_op_emi) {
    OmptApi->ompt_target_callbacks->ompt_callback(
        ompt_callback_target_data_op_emi)(
        ompt_scope_begin, /*target_task_data=*/NULL, /*target_data=*/NULL,
        /*host_op_id=*/NULL, ompt_target_data_transfer_to_device, HstPtr,
        OmptApi->omp_get_initial_device(), TgtPtr, OmptApi->global_device_id,
        Size, /*codeptr_ra=*/NULL);
    OmptApi->ompt_target_callbacks->ompt_callback(
        ompt_callback_target_data_op_emi)(
        ompt_scope_end, /*target_task_data=*/NULL, /*target_data=*/NULL,
        /*host_op_id=*/NULL, ompt_target_data_transfer_to_device, HstPtr,
        OmptApi->omp_get_initial_device(), TgtPtr, OmptApi->global_device_id,
        Size, /*codeptr_ra=*/NULL);
  }
#endif
  memcpy(TgtPtr, HstPtr, Size);
  return OFFLOAD_SUCCESS;
}

int32_t __tgt_rtl_data_retrieve(
    int32_t DeviceId, void *HstPtr, void *TgtPtr, int64_t Size
    OMPT_SUPPORT_IF(, const ompt_plugin_api_t *OmptApi)) {
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
  if (OmptApi->ompt_target_enabled->ompt_callback_target_data_op_emi) {
    OmptApi->ompt_target_callbacks->ompt_callback(
        ompt_callback_target_data_op_emi)(
        ompt_scope_begin, /*target_task_data=*/NULL, /*target_data=*/NULL,
        /*host_op_id=*/NULL, ompt_target_data_transfer_from_device, TgtPtr,
        OmptApi->global_device_id, HstPtr, OmptApi->omp_get_initial_device(),
        Size, /*codeptr_ra=*/NULL);
    OmptApi->ompt_target_callbacks->ompt_callback(
        ompt_callback_target_data_op_emi)(
        ompt_scope_end, /*target_task_data=*/NULL, /*target_data=*/NULL,
        /*host_op_id=*/NULL, ompt_target_data_transfer_from_device, TgtPtr,
        OmptApi->global_device_id, HstPtr, OmptApi->omp_get_initial_device(),
        Size, /*codeptr_ra=*/NULL);
  }
#endif
  memcpy(HstPtr, TgtPtr, Size);
  return OFFLOAD_SUCCESS;
}

int32_t __tgt_rtl_data_delete(int32_t DeviceId, void *TgtPtr) {
  free(TgtPtr);
  return OFFLOAD_SUCCESS;
}

int32_t __tgt_rtl_run_target_team_region(
    int32_t DeviceId, void *TgtEntryPtr, void **TgtArgs, ptrdiff_t *TgtOffsets,
    int32_t ArgNum, int32_t TeamNum, int32_t ThreadLimit,
    uint64_t LoopTripcount /*not used*/
    OMPT_SUPPORT_IF(, const ompt_plugin_api_t *OmptApi)) {
  // ignore team num and thread limit.

  // Use libffi to launch execution.
  ffi_cif Cif;

  // All args are references.
  std::vector<ffi_type *> ArgsTypes(ArgNum, &ffi_type_pointer);
  std::vector<void *> Args(ArgNum);
  std::vector<void *> Ptrs(ArgNum);

  for (int32_t I = 0; I < ArgNum; ++I) {
    Ptrs[I] = (void *)((intptr_t)TgtArgs[I] + TgtOffsets[I]);
    Args[I] = &Ptrs[I];
  }

  ffi_status Status = ffi_prep_cif(&Cif, FFI_DEFAULT_ABI, ArgNum,
                                   &ffi_type_void, &ArgsTypes[0]);

  assert(Status == FFI_OK && "Unable to prepare target launch!");

  if (Status != FFI_OK)
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
  if (OmptApi->ompt_target_enabled->ompt_callback_target_submit_emi) {
    OmptApi->ompt_target_callbacks->ompt_callback(
        ompt_callback_target_submit_emi)(
        ompt_scope_begin, /*target_data=*/NULL, /*host_op_id=*/NULL,
        /*requested_num_teams=*/TeamNum);
    OmptApi->ompt_target_callbacks->ompt_callback(
        ompt_callback_target_submit_emi)(
        ompt_scope_end, /*target_data=*/NULL, /*host_op_id=*/NULL,
        /*requested_num_teams=*/TeamNum);
  }
#endif

  DP("Running entry point at " DPxMOD "...\n", DPxPTR(TgtEntryPtr));

  void (*Entry)(void);
  *((void **)&Entry) = TgtEntryPtr;
  ffi_call(&Cif, Entry, NULL, &Args[0]);
  return OFFLOAD_SUCCESS;
}

int32_t __tgt_rtl_run_target_region(
    int32_t DeviceId, void *TgtEntryPtr, void **TgtArgs, ptrdiff_t *TgtOffsets,
    int32_t ArgNum OMPT_SUPPORT_IF(, const ompt_plugin_api_t *OmptApi)) {
  // use one team and one thread.
  return __tgt_rtl_run_target_team_region(DeviceId, TgtEntryPtr, TgtArgs,
                                          TgtOffsets, ArgNum, 1, 1, 0
                                          OMPT_SUPPORT_IF(, OmptApi));
}

#ifdef __cplusplus
}
#endif
