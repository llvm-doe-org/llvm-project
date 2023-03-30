//===-------- interface.cpp - Target independent OpenMP target RTL --------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Implementation of the interface to be used by Clang during the codegen of a
// target region.
//
//===----------------------------------------------------------------------===//

#include "device.h"
#include "omptarget.h"
#include "private.h"
#include "rtl.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <mutex>

////////////////////////////////////////////////////////////////////////////////
/// adds requires flags
EXTERN void __tgt_register_requires(int64_t Flags) {
  TIMESCOPE();
  PM->RTLs.registerRequires(Flags);
}

////////////////////////////////////////////////////////////////////////////////
/// adds a target shared library to the target execution image
EXTERN void __tgt_register_lib(__tgt_bin_desc *Desc) {
  TIMESCOPE();
  std::call_once(PM->RTLs.InitFlag, &RTLsTy::loadRTLs, &PM->RTLs);
  for (auto &RTL : PM->RTLs.AllRTLs) {
    if (RTL.register_lib) {
      if ((*RTL.register_lib)(Desc) != OFFLOAD_SUCCESS) {
        DP("Could not register library with %s", RTL.RTLName.c_str());
      }
    }
  }
  PM->RTLs.registerLib(Desc);
}

////////////////////////////////////////////////////////////////////////////////
/// Initialize sufficiently for OMPT even if offloading is disabled.
EXTERN void __tgt_load_rtls() {
  TIMESCOPE();
  std::call_once(PM->RTLs.InitFlag, &RTLsTy::loadRTLs, &PM->RTLs);
}

////////////////////////////////////////////////////////////////////////////////
/// Initialize all available devices without registering any image
EXTERN void __tgt_init_all_rtls() { PM->RTLs.initAllRTLs(); }

////////////////////////////////////////////////////////////////////////////////
/// unloads a target shared library
EXTERN void __tgt_unregister_lib(__tgt_bin_desc *Desc) {
  TIMESCOPE();
  PM->RTLs.unregisterLib(Desc);
  for (auto &RTL : PM->RTLs.UsedRTLs) {
    if (RTL->unregister_lib) {
      if ((*RTL->unregister_lib)(Desc) != OFFLOAD_SUCCESS) {
        DP("Could not register library with %s", RTL->RTLName.c_str());
      }
    }
  }
}

struct OmptIdentRAII {
  OmptIdentRAII(ident_t *Ident) {
#if OMPT_SUPPORT
    libomp_ompt_set_trigger_ident(Ident);
#endif
  }
  ~OmptIdentRAII() {
#if OMPT_SUPPORT
    libomp_ompt_clear_trigger_ident();
#endif
  }
};

/// creates host-to-target data mapping, stores it in the
/// libomptarget.so internal structure (an entry in a stack of data maps)
/// and passes the data to the device.
//
// OpenMP 5.0 sec. 2.12.3 p. 165 L22-23:
// "The target-enter-data-begin event occurs when a thread enters a target
// enter data region."
// "The target-enter-data-end event occurs when a thread exits a target enter
// data region."
// OpenMP 5.0 sec. 2.12.2 p. 162 L29-30:
// "The events associated with entering a target data region are the same
// events as associated with a target enter data construct, described in
// Section 2.12.3 on page 164."
// OpenMP 5.0 sec. 4.5.2.26 p. 490 L24-25 and p. 491 L20-22:
// "The ompt_callback_target_t type is used for callbacks that are dispatched
// when a thread begins to execute a device construct."
// "The kind argument indicates the kind of target region."
// "The endpoint argument indicates that the callback signals the beginning of
/// a scope or the end of a scope."
EXTERN void __tgt_target_data_begin_mapper(ident_t *Loc, int64_t DeviceId,
                                           int32_t ArgNum, void **ArgsBase,
                                           void **Args, int64_t *ArgSizes,
                                           int64_t *ArgTypes,
                                           map_var_info_t *ArgNames,
                                           void **ArgMappers) {
  TIMESCOPE_WITH_IDENT(Loc);
  OmptIdentRAII TheOmptIdentRAII(Loc);
  DP("Entering data begin region for device %" PRId64 " with %d mappings\n",
     DeviceId, ArgNum);
  if (checkDeviceAndCtors(DeviceId, Loc)) {
    DP("Not offloading to device %" PRId64 "\n", DeviceId);
    return;
  }

  DeviceTy &Device = *PM->Devices[DeviceId];

  if (getInfoLevel() & OMP_INFOTYPE_KERNEL_ARGS)
    printKernelArguments(Loc, DeviceId, ArgNum, ArgSizes, ArgTypes, ArgNames,
                         "Entering OpenMP data region");
#ifdef OMPTARGET_DEBUG
  for (int I = 0; I < ArgNum; ++I) {
    DP("Entry %2d: Base=" DPxMOD ", Begin=" DPxMOD ", Size=%" PRId64
       ", Type=0x%" PRIx64 ", Name=%s\n",
       I, DPxPTR(ArgsBase[I]), DPxPTR(Args[I]), ArgSizes[I], ArgTypes[I],
       (ArgNames) ? getNameFromMapping(ArgNames[I]).c_str() : "unknown");
  }
#endif

#if OMPT_SUPPORT
  ompt_dispatch_callback_target_emi(ompt_target_enter_data, ompt_scope_begin,
                                    Device);
#endif
  AsyncInfoTy AsyncInfo(Device);
  int Rc = targetDataBegin(Loc, Device, ArgNum, ArgsBase, Args, ArgSizes,
                           ArgTypes, ArgNames, ArgMappers, AsyncInfo);
  if (Rc == OFFLOAD_SUCCESS)
    Rc = AsyncInfo.synchronize();
#if OMPT_SUPPORT
  ompt_dispatch_callback_target_emi(ompt_target_enter_data, ompt_scope_end,
                                    Device);
#endif
  handleTargetOutcome(Rc == OFFLOAD_SUCCESS, Loc);
}

EXTERN void __tgt_target_data_begin_nowait_mapper(
    ident_t *Loc, int64_t DeviceId, int32_t ArgNum, void **ArgsBase,
    void **Args, int64_t *ArgSizes, int64_t *ArgTypes, map_var_info_t *ArgNames,
    void **ArgMappers, int32_t DepNum, void *DepList, int32_t NoAliasDepNum,
    void *NoAliasDepList) {
  TIMESCOPE_WITH_IDENT(Loc);

  __tgt_target_data_begin_mapper(Loc, DeviceId, ArgNum, ArgsBase, Args,
                                 ArgSizes, ArgTypes, ArgNames, ArgMappers);
}

/// passes data from the target, releases target memory and destroys
/// the host-target mapping (top entry from the stack of data maps)
/// created by the last __tgt_target_data_begin.
//
// OpenMP 5.0 sec. 2.12.4 p. 168 L22-23:
// "The target-exit-data-begin event occurs when a thread enters a target exit
// data region."
// "The target-exit-data-end event occurs when a thread exits a target exit
// data region."
// OpenMP 5.0 sec. 2.12.2 p. 162 L31-32:
// "The events associated with exiting a target data region are the same
// events as associated with a target exit data construct, described in
// Section 2.12.4 on page 166."
// OpenMP 5.0 sec. 4.5.2.26 p. 490 L24-25 and p. 491 L20-22:
// "The ompt_callback_target_t type is used for callbacks that are dispatched
// when a thread begins to execute a device construct."
// "The kind argument indicates the kind of target region."
// "The endpoint argument indicates that the callback signals the beginning of
// a scope or the end of a scope."
EXTERN void __tgt_target_data_end_mapper(ident_t *Loc, int64_t DeviceId,
                                         int32_t ArgNum, void **ArgsBase,
                                         void **Args, int64_t *ArgSizes,
                                         int64_t *ArgTypes,
                                         map_var_info_t *ArgNames,
                                         void **ArgMappers) {
  TIMESCOPE_WITH_IDENT(Loc);
  OmptIdentRAII TheOmptIdentRAII(Loc);
  DP("Entering data end region with %d mappings\n", ArgNum);
  if (checkDeviceAndCtors(DeviceId, Loc)) {
    DP("Not offloading to device %" PRId64 "\n", DeviceId);
    return;
  }

  DeviceTy &Device = *PM->Devices[DeviceId];

  if (getInfoLevel() & OMP_INFOTYPE_KERNEL_ARGS)
    printKernelArguments(Loc, DeviceId, ArgNum, ArgSizes, ArgTypes, ArgNames,
                         "Exiting OpenMP data region");
#ifdef OMPTARGET_DEBUG
  for (int I = 0; I < ArgNum; ++I) {
    DP("Entry %2d: Base=" DPxMOD ", Begin=" DPxMOD ", Size=%" PRId64
       ", Type=0x%" PRIx64 ", Name=%s\n",
       I, DPxPTR(ArgsBase[I]), DPxPTR(Args[I]), ArgSizes[I], ArgTypes[I],
       (ArgNames) ? getNameFromMapping(ArgNames[I]).c_str() : "unknown");
  }
#endif

#if OMPT_SUPPORT
  ompt_dispatch_callback_target_emi(ompt_target_exit_data, ompt_scope_begin,
                                    Device);
#endif
  AsyncInfoTy AsyncInfo(Device);
  int Rc = targetDataEnd(Loc, Device, ArgNum, ArgsBase, Args, ArgSizes,
                         ArgTypes, ArgNames, ArgMappers, AsyncInfo);
  if (Rc == OFFLOAD_SUCCESS)
    Rc = AsyncInfo.synchronize();
#if OMPT_SUPPORT
  ompt_dispatch_callback_target_emi(ompt_target_exit_data, ompt_scope_end,
                                    Device);
#endif
  handleTargetOutcome(Rc == OFFLOAD_SUCCESS, Loc);
}

EXTERN void __tgt_target_data_end_nowait_mapper(
    ident_t *Loc, int64_t DeviceId, int32_t ArgNum, void **ArgsBase,
    void **Args, int64_t *ArgSizes, int64_t *ArgTypes, map_var_info_t *ArgNames,
    void **ArgMappers, int32_t DepNum, void *DepList, int32_t NoAliasDepNum,
    void *NoAliasDepList) {
  TIMESCOPE_WITH_IDENT(Loc);

  __tgt_target_data_end_mapper(Loc, DeviceId, ArgNum, ArgsBase, Args, ArgSizes,
                               ArgTypes, ArgNames, ArgMappers);
}

EXTERN void __tgt_target_data_update_mapper(ident_t *Loc, int64_t DeviceId,
                                            int32_t ArgNum, void **ArgsBase,
                                            void **Args, int64_t *ArgSizes,
                                            int64_t *ArgTypes,
                                            map_var_info_t *ArgNames,
                                            void **ArgMappers) {
  TIMESCOPE_WITH_IDENT(Loc);
  OmptIdentRAII TheOmptIdentRAII(Loc);
  DP("Entering data update with %d mappings\n", ArgNum);
  if (checkDeviceAndCtors(DeviceId, Loc)) {
    DP("Not offloading to device %" PRId64 "\n", DeviceId);
    return;
  }

  if (getInfoLevel() & OMP_INFOTYPE_KERNEL_ARGS)
    printKernelArguments(Loc, DeviceId, ArgNum, ArgSizes, ArgTypes, ArgNames,
                         "Updating OpenMP data");

  DeviceTy &Device = *PM->Devices[DeviceId];
  ompt_dispatch_callback_target_emi(ompt_target_update, ompt_scope_begin,
                                    Device);
  AsyncInfoTy AsyncInfo(Device);
  int Rc = targetDataUpdate(Loc, Device, ArgNum, ArgsBase, Args, ArgSizes,
                            ArgTypes, ArgNames, ArgMappers, AsyncInfo);
  if (Rc == OFFLOAD_SUCCESS)
    Rc = AsyncInfo.synchronize();
  ompt_dispatch_callback_target_emi(ompt_target_update, ompt_scope_end, Device);
  handleTargetOutcome(Rc == OFFLOAD_SUCCESS, Loc);
}

EXTERN void __tgt_target_data_update_nowait_mapper(
    ident_t *Loc, int64_t DeviceId, int32_t ArgNum, void **ArgsBase,
    void **Args, int64_t *ArgSizes, int64_t *ArgTypes, map_var_info_t *ArgNames,
    void **ArgMappers, int32_t DepNum, void *DepList, int32_t NoAliasDepNum,
    void *NoAliasDepList) {
  TIMESCOPE_WITH_IDENT(Loc);

  __tgt_target_data_update_mapper(Loc, DeviceId, ArgNum, ArgsBase, Args,
                                  ArgSizes, ArgTypes, ArgNames, ArgMappers);
}

/// Implements a kernel entry that executes the target region on the specified
/// device.
///
/// \param Loc Source location associated with this target region.
/// \param DeviceId The device to execute this region, -1 indicated the default.
/// \param NumTeams Number of teams to launch the region with, -1 indicates a
///                 non-teams region and 0 indicates it was unspecified.
/// \param ThreadLimit Limit to the number of threads to use in the kernel
///                    launch, 0 indicates it was unspecified.
/// \param HostPtr  The pointer to the host function registered with the kernel.
/// \param Args     All arguments to this kernel launch (see struct definition).
EXTERN int __tgt_target_kernel(ident_t *Loc, int64_t DeviceId, int32_t NumTeams,
                               int32_t ThreadLimit, void *HostPtr,
                               __tgt_kernel_arguments *Args) {
  TIMESCOPE_WITH_IDENT(Loc);
  OmptIdentRAII TheOmptIdentRAII(Loc);
  DP("Entering target region with entry point " DPxMOD " and device Id %" PRId64
     "\n",
     DPxPTR(HostPtr), DeviceId);
  if (Args->Version != 1) {
    DP("Unexpected ABI version: %d\n", Args->Version);
  }
  if (checkDeviceAndCtors(DeviceId, Loc)) {
    DP("Not offloading to device %" PRId64 "\n", DeviceId);
    return OMP_TGT_FAIL;
  }

  if (getInfoLevel() & OMP_INFOTYPE_KERNEL_ARGS)
    printKernelArguments(Loc, DeviceId, Args->NumArgs, Args->ArgSizes,
                         Args->ArgTypes, Args->ArgNames,
                         "Entering OpenMP kernel");
#ifdef OMPTARGET_DEBUG
  for (int I = 0; I < Args->NumArgs; ++I) {
    DP("Entry %2d: Base=" DPxMOD ", Begin=" DPxMOD ", Size=%" PRId64
       ", Type=0x%" PRIx64 ", Name=%s\n",
       I, DPxPTR(Args->ArgBasePtrs[I]), DPxPTR(Args->ArgPtrs[I]),
       Args->ArgSizes[I], Args->ArgTypes[I],
       (Args->ArgNames) ? getNameFromMapping(Args->ArgNames[I]).c_str()
                        : "unknown");
  }
#endif

  bool IsTeams = NumTeams != -1;
  if (!IsTeams)
    NumTeams = 0;

  DeviceTy &Device = *PM->Devices[DeviceId];
  ompt_dispatch_callback_target_emi(ompt_target, ompt_scope_begin, Device);
  AsyncInfoTy AsyncInfo(Device);
  int Rc = target(Loc, Device, HostPtr, Args->NumArgs, Args->ArgBasePtrs,
                  Args->ArgPtrs, Args->ArgSizes, Args->ArgTypes, Args->ArgNames,
                  Args->ArgMappers, NumTeams, ThreadLimit, Args->Tripcount,
                  IsTeams, AsyncInfo);
  if (Rc == OFFLOAD_SUCCESS)
    Rc = AsyncInfo.synchronize();
  ompt_dispatch_callback_target_emi(ompt_target, ompt_scope_end, Device);
  handleTargetOutcome(Rc == OFFLOAD_SUCCESS, Loc);
  assert(Rc == OFFLOAD_SUCCESS && "__tgt_target_kernel unexpected failure!");
  return OMP_TGT_SUCCESS;
}

EXTERN int __tgt_target_kernel_nowait(
    ident_t *Loc, int64_t DeviceId, int32_t NumTeams, int32_t ThreadLimit,
    void *HostPtr, __tgt_kernel_arguments *Args, int32_t DepNum, void *DepList,
    int32_t NoAliasDepNum, void *NoAliasDepList) {
  TIMESCOPE_WITH_IDENT(Loc);

  return __tgt_target_kernel(Loc, DeviceId, NumTeams, ThreadLimit, HostPtr,
                             Args);
}

// Get the current number of components for a user-defined mapper.
EXTERN int64_t __tgt_mapper_num_components(void *RtMapperHandle) {
  TIMESCOPE();
  auto *MapperComponentsPtr = (struct MapperComponentsTy *)RtMapperHandle;
  int64_t Size = MapperComponentsPtr->Components.size();
  DP("__tgt_mapper_num_components(Handle=" DPxMOD ") returns %" PRId64 "\n",
     DPxPTR(RtMapperHandle), Size);
  return Size;
}

// Push back one component for a user-defined mapper.
EXTERN void __tgt_push_mapper_component(void *RtMapperHandle, void *Base,
                                        void *Begin, int64_t Size, int64_t Type,
                                        void *Name) {
  TIMESCOPE();
  DP("__tgt_push_mapper_component(Handle=" DPxMOD
     ") adds an entry (Base=" DPxMOD ", Begin=" DPxMOD ", Size=%" PRId64
     ", Type=0x%" PRIx64 ", Name=%s).\n",
     DPxPTR(RtMapperHandle), DPxPTR(Base), DPxPTR(Begin), Size, Type,
     (Name) ? getNameFromMapping(Name).c_str() : "unknown");
  auto *MapperComponentsPtr = (struct MapperComponentsTy *)RtMapperHandle;
  MapperComponentsPtr->Components.push_back(
      MapComponentInfoTy(Base, Begin, Size, Type, Name));
}

EXTERN void __tgt_set_info_flag(uint32_t NewInfoLevel) {
  std::atomic<uint32_t> &InfoLevel = getInfoLevelInternal();
  InfoLevel.store(NewInfoLevel);
  for (auto &R : PM->RTLs.AllRTLs) {
    if (R.set_info_flag)
      R.set_info_flag(NewInfoLevel);
  }
}

EXTERN int __tgt_print_device_info(int64_t DeviceId) {
  return PM->Devices[DeviceId]->printDeviceInfo(
      PM->Devices[DeviceId]->RTLDeviceID);
}
