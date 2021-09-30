/*
 * acc2omp-backend.cpp -- libacc2omp backend for LLVM's OpenMP runtime.
 */

//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "acc2omp-backend.h"
#include "kmp.h"

#include <cstdarg>
#include <cstdlib>

static kmp_i18n_id_t acc2omp_msg_to_llvm(acc2omp_msgid_t MsgId) {
  switch (MsgId) {
  case acc2omp_msg_env_acc_device_type_invalid:
    return kmp_i18n_msg_AccEnvAccDeviceTypeInvalid;
  case acc2omp_msg_env_acc_device_num_parse_error:
    return kmp_i18n_msg_AccEnvAccDeviceNumParseError;
  case acc2omp_msg_env_acc_device_num_default_invalid:
    return kmp_i18n_msg_AccEnvAccDeviceNumDefaultInvalid;
  case acc2omp_msg_env_acc_device_num_invalid:
    return kmp_i18n_msg_AccEnvAccDeviceNumInvalid;
  case acc2omp_msg_alloc_fail:
    return kmp_i18n_msg_MemoryAllocFailed;
  case acc2omp_msg_acc_proflib_fail:
    return kmp_i18n_msg_AccProflibFail;
  case acc2omp_msg_unsupported_event_register:
    return kmp_i18n_msg_AccUnsupportedEventRegister;
  case acc2omp_msg_unsupported_event_unregister:
    return kmp_i18n_msg_AccUnsupportedEventUnregister;
  case acc2omp_msg_event_unsupported_toggle:
    return kmp_i18n_msg_AccEventUnsupportedToggle;
  case acc2omp_msg_event_unsupported_multiple_register:
    return kmp_i18n_msg_AccEventUnsupportedMultipleRegister;
  case acc2omp_msg_event_register_result:
    return kmp_i18n_msg_AccEventRegisterResult;
  case acc2omp_msg_event_unregister_result:
    return kmp_i18n_msg_AccEventUnregisterResult;
  case acc2omp_msg_event_unregister_unregistered:
    return kmp_i18n_msg_AccEventUnregisterUnregistered;
  case acc2omp_msg_callback_unregister_unregistered:
    return kmp_i18n_msg_AccCallbackUnregisterUnregistered;
  case acc2omp_msg_get_num_devices_invalid_type:
    return kmp_i18n_msg_AccGetNumDevicesInvalidType;
  case acc2omp_msg_set_device_type_invalid_type:
    return kmp_i18n_msg_AccSetDeviceTypeInvalidType;
  case acc2omp_msg_set_device_type_no_devices:
    return kmp_i18n_msg_AccSetDeviceTypeNoDevices;
  case acc2omp_msg_set_device_num_invalid_type:
    return kmp_i18n_msg_AccSetDeviceNumInvalidType;
  case acc2omp_msg_set_device_num_invalid_num:
    return kmp_i18n_msg_AccSetDeviceNumInvalidNum;
  case acc2omp_msg_get_device_num_invalid_type:
    return kmp_i18n_msg_AccGetDeviceNumInvalidType;
  case acc2omp_msg_map_data_host_pointer_null:
    return kmp_i18n_msg_AccMapDataHostPointerNull;
  case acc2omp_msg_map_data_device_pointer_null:
    return kmp_i18n_msg_AccMapDataDevicePointerNull;
  case acc2omp_msg_map_data_shared_memory:
    return kmp_i18n_msg_AccMapDataSharedMemory;
  case acc2omp_msg_map_data_already_present:
    return kmp_i18n_msg_AccMapDataAlreadyPresent;
  case acc2omp_msg_map_data_fail:
    return kmp_i18n_msg_AccMapDataFail;
  case acc2omp_msg_unmap_data_pointer_null:
    return kmp_i18n_msg_AccUnmapDataPointerNull;
  case acc2omp_msg_unmap_data_shared_memory:
    return kmp_i18n_msg_AccUnmapDataSharedMemory;
  case acc2omp_msg_unmap_data_fail:
    return kmp_i18n_msg_AccUnmapDataFail;
  case acc2omp_msg_memcpy_to_device_dest_pointer_null:
    return kmp_i18n_msg_AccMemcpyToDeviceDestPointerNull;
  case acc2omp_msg_memcpy_to_device_src_pointer_null:
    return kmp_i18n_msg_AccMemcpyToDeviceSrcPointerNull;
  case acc2omp_msg_memcpy_to_device_fail:
    return kmp_i18n_msg_AccMemcpyToDeviceFail;
  case acc2omp_msg_memcpy_from_device_dest_pointer_null:
    return kmp_i18n_msg_AccMemcpyFromDeviceDestPointerNull;
  case acc2omp_msg_memcpy_from_device_src_pointer_null:
    return kmp_i18n_msg_AccMemcpyFromDeviceSrcPointerNull;
  case acc2omp_msg_memcpy_from_device_fail:
    return kmp_i18n_msg_AccMemcpyFromDeviceFail;
  case acc2omp_msg_memcpy_device_dest_pointer_null:
    return kmp_i18n_msg_AccMemcpyDeviceDestPointerNull;
  case acc2omp_msg_memcpy_device_src_pointer_null:
    return kmp_i18n_msg_AccMemcpyDeviceSrcPointerNull;
  case acc2omp_msg_memcpy_device_fail:
    return kmp_i18n_msg_AccMemcpyDeviceFail;
  case acc2omp_msg_memcpy_d2d_dest_pointer_null:
    return kmp_i18n_msg_AccMemcpyD2dDestPointerNull;
  case acc2omp_msg_memcpy_d2d_src_pointer_null:
    return kmp_i18n_msg_AccMemcpyD2dSrcPointerNull;
  case acc2omp_msg_memcpy_d2d_dest_device_invalid:
    return kmp_i18n_msg_AccMemcpyD2dDestDeviceInvalid;
  case acc2omp_msg_memcpy_d2d_src_device_invalid:
    return kmp_i18n_msg_AccMemcpyD2dSrcDeviceInvalid;
  case acc2omp_msg_memcpy_d2d_dest_data_inaccessible:
    return kmp_i18n_msg_AccMemcpyD2dDestDataInaccessible;
  case acc2omp_msg_memcpy_d2d_src_data_inaccessible:
    return kmp_i18n_msg_AccMemcpyD2dSrcDataInaccessible;
  case acc2omp_msg_memcpy_d2d_fail:
    return kmp_i18n_msg_AccMemcpyD2dFail;
  }
  KMP_ASSERT2(0, "unexpected acc2omp_msg_t");
  return kmp_i18n_null;
}

void acc2omp_warn(acc2omp_msg_t Msg, ...) {
  va_list Args;
  va_start(Args, Msg);
  kmp_msg_t KmpMsg = __kmp_msg_vformat(acc2omp_msg_to_llvm(Msg.Id), Args);
  va_end(Args);
  __kmp_msg(kmp_ms_warning, KmpMsg, __kmp_msg_null);
}

void acc2omp_fatal(acc2omp_msg_t Msg, ...) {
  va_list Args;
  va_start(Args, Msg);
  kmp_msg_t KmpMsg = __kmp_msg_vformat(acc2omp_msg_to_llvm(Msg.Id), Args);
  va_end(Args);
  __kmp_fatal(KmpMsg, __kmp_msg_null);
}

void acc2omp_assert(int Cond, const char *Msg, const char *File, int Line) {
  // The cmake variable LLVM_ENABLE_ASSERTIONS controls both whether libacc2omp
  // ever calls acc2omp_assert and also whether KMP_ASSERT4 is a no-op, so we
  // need not implement a fallback assertion mechanism here as discussed in
  // acc2omp_assert documentation in acc2omp-backend.h
  KMP_ASSERT4(Cond, Msg, File, Line);
}

omp_device_t acc2omp_get_omp_device_t(acc_device_t DevTypeACC) {
  switch (DevTypeACC) {
  case acc_device_none:
  case acc_device_default:
  case acc_device_not_host:
  case acc_device_current:
    return omp_device_none;
  case acc_device_host:
    return omp_device_host;
  case acc_device_nvidia:
    return omp_device_nvptx64;
  case acc_device_radeon:
    return omp_device_amdgcn;
  case acc_device_x86_64:
    return omp_device_x86_64;
  case acc_device_ppc64le:
    return omp_device_ppc64le;
  }
  KMP_ASSERT2(0, "unexpected acc_device_t");
  return omp_device_none;
}

acc_device_t acc2omp_get_acc_device_t(omp_device_t DevTypeOMP) {
  switch (DevTypeOMP) {
  case omp_device_none:
    return acc_device_none;
  case omp_device_host:
    return acc_device_host;
  case omp_device_x86_64:
    return acc_device_x86_64;
  case omp_device_ppc64le:
    return acc_device_ppc64le;
  case omp_device_nvptx64:
    return acc_device_nvidia;
  case omp_device_amdgcn:
    return acc_device_radeon;
  }
  KMP_ASSERT2(0, "unexpected omp_device_t");
  return acc_device_none;
}
