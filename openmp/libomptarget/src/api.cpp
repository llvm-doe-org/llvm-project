//===----------- api.cpp - Target independent OpenMP target RTL -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Implementation of OpenMP API interface functions.
//
//===----------------------------------------------------------------------===//

#include "device.h"
#include "omptarget.h"
#include "private.h"
#include "rtl.h"

#include <climits>
#include <cstdlib>
#include <cstring>

// We need the string version of ompt_trigger_runtime_api so that DEFINE_IDENT
// can form a proper ident_t at compile time.
//
// I don't know how to automatically stringify an enumerator value at compile
// time.  The assert will remind us to update if ompt_trigger_kind_t evolves.
#define STR_(Arg) #Arg
#define STR(Arg) STR_(Arg)
#define OMPT_TRIGGER_RUNTIME_API 6
static_assert(OMPT_TRIGGER_RUNTIME_API == ompt_trigger_runtime_api,
              "unexpected change in ompt_trigger_runtime_api value");

// At compile time, define an ident_t for the enclosing OpenMP runtime library
// routine.  Don't waste run time doing this.
//
// I don't know how to automatically stringify the current function name at
// compile time, so the caller has to specify it.  The assert will catch typos.
#define DEFINE_IDENT(Func)                                                     \
  ident_t Ident = {                                                            \
    0, 0, 0, 0,                                                                \
    ";;" #Func ";0;0;0;0;0;" STR(OMPT_TRIGGER_RUNTIME_API) ";;"              \
  };                                                                           \
  assert(0 == strcmp(__func__, #Func) &&                                       \
         "expected specified function to be enclosing function")

#if OMPT_SUPPORT
# define OMPT_DEFINE_IDENT(Func) DEFINE_IDENT(Func)
# define OMPT_SET_TRIGGER_IDENT() libomp_ompt_set_trigger_ident(&Ident)
# define OMPT_CLEAR_TRIGGER_IDENT() libomp_ompt_clear_trigger_ident()
# define OMPT_DISPATCH_CALLBACK_TARGET_DATA_OP_EMI(                            \
    EndPoint, OpKind, SrcPtr, SrcDevNum, DestPtr, DestDevNum, Size)            \
  ompt_dispatch_callback_target_data_op_emi(                                   \
      Ident, EndPoint, ompt_target_data_##OpKind, SrcPtr, SrcDevNum,           \
      DestPtr, DestDevNum, Size)
static void ompt_dispatch_callback_target_data_op_emi(
    const ident_t &Ident, ompt_scope_endpoint_t EndPoint,
    ompt_target_data_op_t OpKind, void *SrcPtr, int SrcDevNum, void *DestPtr,
    int DestDevNum, size_t Size) {
  if (ompt_target_enabled.ompt_callback_target_data_op_emi) {
    OMPT_SET_TRIGGER_IDENT();
    // FIXME: We don't yet need the target_task_data, target_data, host_op_id,
    // and codeptr_ra arguments for OpenACC support, so we haven't bothered to
    // implement them yet.
    ompt_target_callbacks.ompt_callback(ompt_callback_target_data_op_emi)(
        EndPoint, /*target_task_data=*/NULL, /*target_data=*/NULL,
        /*host_op_id=*/NULL, OpKind, SrcPtr, SrcDevNum, DestPtr, DestDevNum,
        Size, /*codeptr_ra=*/NULL);
    OMPT_CLEAR_TRIGGER_IDENT();
  }
}
#else
# define OMPT_DEFINE_IDENT(Func)
# define OMPT_SET_TRIGGER_IDENT()
# define OMPT_CLEAR_TRIGGER_IDENT()
# define OMPT_DISPATCH_CALLBACK_TARGET_DATA_OP_EMI(                            \
    EndPoint, OpKind, SrcPtr, SrcDevNum, DestPtr, DestDevNum, Size)
#endif

EXTERN int omp_get_num_devices(void) {
  TIMESCOPE();
  PM->RTLsMtx.lock();
  size_t DevicesSize = PM->Devices.size();
  PM->RTLsMtx.unlock();

  DP("Call to omp_get_num_devices returning %zd\n", DevicesSize);

  return DevicesSize;
}

EXTERN int omp_get_device_num(void) {
  TIMESCOPE();
  int HostDevice = omp_get_initial_device();

  DP("Call to omp_get_device_num returning %d\n", HostDevice);

  return HostDevice;
}

EXTERN int omp_get_initial_device(void) {
  TIMESCOPE();
  int HostDevice = omp_get_num_devices();
  DP("Call to omp_get_initial_device returning %d\n", HostDevice);
  return HostDevice;
}

#if OMPT_SUPPORT
// Accesses to DeviceAllocSizes must be thread-safe or allocating and
// deallocating in, for example, an "omp parallel for" crashes (currently,
// openmp/libomptarget/test/offloading/memory_manager.cpp exercises this case).
static std::mutex DeviceAllocSizesMtx;
// FIXME: Does this work if you allocate on multiple devices?  There might be
// address collisions.  Either key it by device or store it in DeviceTy?
static std::map<void *, size_t> DeviceAllocSizes;
#endif

EXTERN void *omp_target_alloc(size_t Size, int DeviceNum) {
  OMPT_DEFINE_IDENT(omp_target_alloc);
  OMPT_SET_TRIGGER_IDENT();
  void *Ptr = targetAllocExplicit(Size, DeviceNum, TARGET_ALLOC_DEFAULT,
                                  __func__);
  OMPT_CLEAR_TRIGGER_IDENT();
  // OpenMP 5.1, sec. 3.8.1 "omp_target_alloc", p. 413, L14-15:
  // "The target-data-allocation-begin event occurs before a thread initiates a
  // data allocation on a target device.  The target-data-allocation-end event
  // occurs after a thread initiates a data allocation on a target device."
  //
  // OpenMP 5.1, sec. 4.5.2.25 "ompt_callback_target_data_op_emi_t and
  // ompt_callback_target_data_op_t", p. 536, L25-27:
  // "A thread dispatches a registered ompt_callback_target_data_op_emi or
  // ompt_callback_target_data_op callback when device memory is allocated or
  // freed, as well as when data is copied to or from a device."
  //
  // Contrary to the above specification, we assume the ompt_scope_end callback
  // must dispatch after the allocation succeeds so it can include the device
  // address.  We assume the ompt_scope_begin callback cannot possibly include
  // the device address under any reasonable interpretation.
  //
  // TODO: We have not implemented the ompt_scope_begin callback because we
  // don't need it for OpenACC support.
#if OMPT_SUPPORT
  // If offloading is disabled, the runtime might not be initialized.  In that
  // case, ompt_start_tool and libomp_start_tool haven't been called, so we
  // don't know what OMPT callbacks to dispatch.  Calling __tgt_load_rtls
  // guarantees all those calls are made.
  __tgt_load_rtls();
  DeviceAllocSizesMtx.lock();
  DeviceAllocSizes[Ptr] = Size;
  DeviceAllocSizesMtx.unlock();
  OMPT_DISPATCH_CALLBACK_TARGET_DATA_OP_EMI(
      ompt_scope_end, alloc, /*SrcPtr=*/NULL, omp_get_initial_device(), Ptr,
      DeviceNum, Size);
#endif
  return Ptr;
}

EXTERN void *llvm_omp_target_alloc_device(size_t Size, int DeviceNum) {
  return targetAllocExplicit(Size, DeviceNum, TARGET_ALLOC_DEVICE, __func__);
}

EXTERN void *llvm_omp_target_alloc_host(size_t Size, int DeviceNum) {
  return targetAllocExplicit(Size, DeviceNum, TARGET_ALLOC_HOST, __func__);
}

EXTERN void *llvm_omp_target_alloc_shared(size_t Size, int DeviceNum) {
  return targetAllocExplicit(Size, DeviceNum, TARGET_ALLOC_SHARED, __func__);
}

EXTERN void *llvm_omp_target_dynamic_shared_alloc() { return nullptr; }
EXTERN void *llvm_omp_get_dynamic_shared() { return nullptr; }

EXTERN void omp_target_free(void *DevicePtr, int DeviceNum) {
  OMPT_DEFINE_IDENT(omp_target_free);
  TIMESCOPE();
  DP("Call to omp_target_free for device %d and address " DPxMOD "\n",
     DeviceNum, DPxPTR(DevicePtr));

  if (!DevicePtr) {
    DP("Call to omp_target_free with NULL ptr\n");
    return;
  }

  // OpenMP 5.1, sec. 3.8.2 "omp_target_free", p. 415, L11-12:
  // "The target-data-free-begin event occurs before a thread initiates a data
  // free on a target device.  The target-data-free-end event occurs after a
  // thread initiates a data free on a target device."
  //
  // OpenMP 5.1, sec. 4.5.2.25 "ompt_callback_target_data_op_emi_t and
  // ompt_callback_target_data_op_t", p. 536, L25-27:
  // "A thread dispatches a registered ompt_callback_target_data_op_emi or
  // ompt_callback_target_data_op callback when device memory is allocated or
  // freed, as well as when data is copied to or from a device."
  //
  // TODO: We have not implemented the ompt_scope_end callback because we don't
  // need it for OpenACC support.
  if (DeviceNum == omp_get_initial_device()) {
#if OMPT_SUPPORT
    DeviceAllocSizesMtx.lock();
    auto AllocSizeItr = DeviceAllocSizes.find(DevicePtr);
    size_t AllocSize = 0;
    if (AllocSizeItr != DeviceAllocSizes.end()) {
      AllocSize = AllocSizeItr->second;
      DeviceAllocSizes.erase(AllocSizeItr);
    }
    DeviceAllocSizesMtx.unlock();
    OMPT_DISPATCH_CALLBACK_TARGET_DATA_OP_EMI(ompt_scope_begin, delete,
                                              /*SrcPtr=*/NULL, DeviceNum,
                                              DevicePtr, DeviceNum, AllocSize);
#endif
    free(DevicePtr);
    DP("omp_target_free deallocated host ptr\n");
    return;
  }

  if (!deviceIsReady(DeviceNum)) {
    DP("omp_target_free returns, nothing to do\n");
    return;
  }

#if OMPT_SUPPORT
  DeviceAllocSizesMtx.lock();
  auto AllocSizeItr = DeviceAllocSizes.find(DevicePtr);
  size_t AllocSize = 0;
  if (AllocSizeItr != DeviceAllocSizes.end()) {
    AllocSize = AllocSizeItr->second;
    DeviceAllocSizes.erase(AllocSizeItr);
  }
  DeviceAllocSizesMtx.unlock();
  OMPT_DISPATCH_CALLBACK_TARGET_DATA_OP_EMI(
      ompt_scope_begin, delete, /*SrcPtr=*/NULL, omp_get_initial_device(),
      DevicePtr, DeviceNum, AllocSize);
#endif
  PM->Devices[DeviceNum]->deleteData(DevicePtr);
  DP("omp_target_free deallocated device ptr\n");
}

EXTERN int omp_target_is_present(const void *Ptr, int DeviceNum) {
  TIMESCOPE();
  DP("Call to omp_target_is_present for device %d and address " DPxMOD "\n",
     DeviceNum, DPxPTR(Ptr));

  if (!Ptr) {
    DP("Call to omp_target_is_present with NULL ptr, returning false\n");
    return false;
  }

  if (DeviceNum == omp_get_initial_device()) {
    DP("Call to omp_target_is_present on host, returning true\n");
    return true;
  }

  PM->RTLsMtx.lock();
  size_t DevicesSize = PM->Devices.size();
  PM->RTLsMtx.unlock();
  if (DevicesSize <= (size_t)DeviceNum) {
    DP("Call to omp_target_is_present with invalid device ID, returning "
       "false\n");
    return false;
  }

  DeviceTy &Device = *PM->Devices[DeviceNum];
  bool IsLast; // not used
  bool IsHostPtr;
  // omp_target_is_present tests whether a host pointer refers to storage that
  // is mapped to a given device. However, due to the lack of the storage size,
  // only check 1 byte. Cannot set size 0 which checks whether the pointer (zero
  // lengh array) is mapped instead of the referred storage.
  TargetPointerResultTy TPR =
      Device.getTgtPtrBegin(const_cast<void *>(Ptr), 1, IsLast,
                            /*UpdateRefCount=*/false,
                            /*UseHoldRefCount=*/false, IsHostPtr);
  int Rc = (TPR.TargetPointer != NULL);
  // Under unified memory the host pointer can be returned by the
  // getTgtPtrBegin() function which means that there is no device
  // corresponding point for ptr. This function should return false
  // in that situation.
  if (PM->RTLs.RequiresFlags & OMP_REQ_UNIFIED_SHARED_MEMORY)
    Rc = !IsHostPtr;
  DP("Call to omp_target_is_present returns %d\n", Rc);
  return Rc;
}

EXTERN int omp_target_is_accessible(const void *Ptr, size_t Size,
                                    int DeviceNum) {
  TIMESCOPE();
  // OpenMP 5.1, sec. 3.8.4 "omp_target_is_accessible", p. 417, L21-22:
  // "This routine returns true if the storage of size bytes starting at the
  // address given by Ptr is accessible from device device_num. Otherwise, it
  // returns false."
  //
  // The meaning of "accessible" for unified shared memory is established in
  // OpenMP 5.1, sec. 2.5.1 "requires directive".  More generally, the specified
  // host memory is accessible if it can be accessed from the device either
  // directly (because of unified shared memory or because DeviceNum is the
  // value returned by omp_get_initial_device()) or indirectly (because it's
  // mapped to the device).
  DP("Call to omp_target_is_accessible for device %d and address " DPxMOD "\n",
     DeviceNum, DPxPTR(Ptr));

  // FIXME: Is this right?
  //
  // Null pointer is permitted:
  //
  // OpenMP 5.1, sec. 3.8.4 "omp_target_is_accessible", p. 417, L15:
  // "The value of ptr must be a valid host pointer or NULL (or C_NULL_PTR, for
  // Fortran)."
  //
  // However, I found no specification of behavior in this case.
  // omp_target_is_present has the same problem and is implemented the same way.
  // Should Size have any effect on the result when Ptr is NULL?
  if (!Ptr) {
    DP("Call to omp_target_is_accessible with NULL Ptr, returning false\n");
    return false;
  }

  if (DeviceNum == omp_get_initial_device()) {
    DP("Call to omp_target_is_accessible on host, returning true\n");
    return true;
  }

  PM->RTLsMtx.lock();
  size_t Devices_size = PM->Devices.size();
  PM->RTLsMtx.unlock();
  if (Devices_size <= (size_t)DeviceNum) {
    DP("Call to omp_target_is_accessible with invalid device ID, returning "
       "false\n");
    return false;
  }

  DeviceTy &Device = *PM->Devices[DeviceNum];
  bool IsLast;    // not used
  bool IsHostPtr; // not used
  // TODO: How does the spec intend for the Size=0 case to be handled?
  // Currently, for the case where arr[N:M] is mapped, we return true for any
  // address within arr[0:N+M].  However, Size>1 returns true only for arr[N:M].
  // This is based on the discussion so far at the time of this writing at
  // <https://github.com/llvm/llvm-project/issues/54899>.  If the behavior
  // changes, keep comments for omp_get_accessible_buffer in omp.h.var in sync.
  TargetPointerResultTy TPR =
      Device.getTgtPtrBegin(const_cast<void *>(Ptr), Size, IsLast,
                            /*UpdateRefCount=*/false, /*UseHoldRefCount=*/false,
                            IsHostPtr, /*MustContain=*/true);
  int rc = (TPR.TargetPointer != NULL);
  DP("Call to omp_target_is_accessible returns %d\n", rc);
  return rc;
}

EXTERN void *omp_get_mapped_ptr(const void *Ptr, int DeviceNum) {
  TIMESCOPE();
  DP("Call to omp_get_mapped_ptr for device %d and address " DPxMOD "\n",
     DeviceNum, DPxPTR(Ptr));

  if (!Ptr) {
    DP("Call to omp_get_mapped_ptr with NULL Ptr, returning NULL\n");
    return NULL;
  }

  if (DeviceNum == omp_get_initial_device()) {
    DP("Call to omp_get_mapped_ptr on host, returning host pointer\n");
    // OpenMP 5.1, sec. 3.8.11 "omp_get_mapped_ptr", p. 431, L10-12:
    // "Otherwise it returns the device pointer, which is ptr if device_num is
    // the value returned by omp_get_initial_device()."
    //
    // That is, the spec actually requires us to cast away const.
    return const_cast<void *>(Ptr);
  }

  PM->RTLsMtx.lock();
  size_t Devices_size = PM->Devices.size();
  PM->RTLsMtx.unlock();
  if (Devices_size <= (size_t)DeviceNum) {
    DP("Call to omp_get_mapped_ptr with invalid device ID, returning NULL\n");
    return NULL;
  }

  DeviceTy &Device = *PM->Devices[DeviceNum];
  bool IsLast; // not used
  bool IsHostPtr;
  TargetPointerResultTy TPR =
      Device.getTgtPtrBegin(const_cast<void *>(Ptr), /*Size=*/0, IsLast,
                            /*UpdateRefCount=*/false, /*UseHoldRefCount=*/false,
                            IsHostPtr);
  void *TgtPtr = TPR.TargetPointer;
  // Return nullptr in the case of unified shared memory.
  //
  // TODO: This seems to be implied by the named "mapped" instead of
  // "accessible".  Or should we return the host pointer?  That is, is this
  // supposed to be like omp_target_is_present or omp_target_is_accessible?
  // OpenMP 5.1 doesn't seem clear.  Keep omp_get_mapped_hostptr in sync.
  if (IsHostPtr) {
    DP("Call to omp_get_mapped_ptr for unified shared memory, returning "
       "NULL\n");
    return nullptr;
  }
  DP("Call to omp_get_mapped_ptr returns " DPxMOD "\n", DPxPTR(TgtPtr));
  return TgtPtr;
}

EXTERN void *omp_get_mapped_hostptr(const void *Ptr, int DeviceNum) {
  TIMESCOPE();
  DP("Call to omp_get_mapped_hostptr for device %d and address " DPxMOD "\n",
     DeviceNum, DPxPTR(Ptr));

  if (!Ptr) {
    DP("Call to omp_get_mapped_hostptr with NULL Ptr, returning NULL\n");
    return NULL;
  }

  if (DeviceNum == omp_get_initial_device()) {
    DP("Call to omp_get_mapped_hostptr for host, returning device pointer\n");
    // For consistency with OpenMP 5.1, sec. 3.8.11 "omp_get_mapped_ptr", p.
    // 431, L10-12:
    // "Otherwise it returns the device pointer, which is ptr if device_num is
    // the value returned by omp_get_initial_device()."
    return const_cast<void *>(Ptr);
  }

  PM->RTLsMtx.lock();
  size_t Devices_size = PM->Devices.size();
  PM->RTLsMtx.unlock();
  if (Devices_size <= (size_t)DeviceNum) {
    DP("Call to omp_get_mapped_hostptr with invalid device ID, returning "
       "NULL\n");
    return NULL;
  }

  DeviceTy &Device = *PM->Devices[DeviceNum];
  // TODO: This returns nullptr in the case of unified shared memory.  This is
  // for consistency with the current omp_get_mapped_ptr implementation.
  void *HostPtr = Device.lookupHostPtr(const_cast<void *>(Ptr));
  DP("Call to omp_get_mapped_hostptr returns " DPxMOD "\n", DPxPTR(HostPtr));
  return HostPtr;
}

EXTERN size_t omp_get_accessible_buffer(
  const void *Ptr, size_t Size, int DeviceNum, void **BufferHost,
  void **BufferDevice) {
  TIMESCOPE();
  DP("Call to omp_get_accessible_buffer for address " DPxMOD ", size %zu, and "
     "device %d\n",
     DPxPTR(Ptr), Size, DeviceNum);

  if (DeviceNum == omp_get_initial_device()) {
    DP("Call to omp_get_accessible_buffer for initial device, returning "
       "SIZE_MAX\n");
    if (BufferHost)
      *BufferHost = nullptr;
    if (BufferDevice)
      *BufferDevice = nullptr;
    return SIZE_MAX;
  }

  PM->RTLsMtx.lock();
  size_t Devices_size = PM->Devices.size();
  PM->RTLsMtx.unlock();
  if (Devices_size <= (size_t)DeviceNum) {
    DP("Call to omp_get_accessible_buffer with invalid device ID, returning "
       "0\n");
    if (BufferHost)
      *BufferHost = nullptr;
    if (BufferDevice)
      *BufferDevice = nullptr;
    return 0;
  }

  if (!Ptr) {
    DP("Call to omp_get_accessible_buffer with NULL Ptr, returning SIZE_MAX\n");
    if (BufferHost)
      *BufferHost = nullptr;
    if (BufferDevice)
      *BufferDevice = nullptr;
    return SIZE_MAX;
  }

  DeviceTy &Device = *PM->Devices[DeviceNum];
  size_t BufferSize = Device.getAccessibleBuffer(const_cast<void *>(Ptr),
                                                 /*Size=*/Size, BufferHost,
                                                 BufferDevice);
  DP("Call to omp_get_accessible_buffer returns %zu\n", BufferSize);
  return BufferSize;;
}

EXTERN int omp_target_memcpy(void *Dst, const void *Src, size_t Length,
                             size_t DstOffset, size_t SrcOffset, int DstDevice,
                             int SrcDevice) {
  OMPT_DEFINE_IDENT(omp_target_memcpy);
  TIMESCOPE();
  DP("Call to omp_target_memcpy, dst device %d, src device %d, "
     "dst addr " DPxMOD ", src addr " DPxMOD ", dst offset %zu, "
     "src offset %zu, length %zu\n",
     DstDevice, SrcDevice, DPxPTR(Dst), DPxPTR(Src), DstOffset, SrcOffset,
     Length);

  if (!Dst || !Src || Length <= 0) {
    if (Length == 0) {
      DP("Call to omp_target_memcpy with zero length, nothing to do\n");
      return OFFLOAD_SUCCESS;
    }

    REPORT("Call to omp_target_memcpy with invalid arguments\n");
    return OFFLOAD_FAIL;
  }

  if (SrcDevice != omp_get_initial_device() && !deviceIsReady(SrcDevice)) {
    REPORT("omp_target_memcpy returns OFFLOAD_FAIL\n");
    return OFFLOAD_FAIL;
  }

  if (DstDevice != omp_get_initial_device() && !deviceIsReady(DstDevice)) {
    REPORT("omp_target_memcpy returns OFFLOAD_FAIL\n");
    return OFFLOAD_FAIL;
  }

  int Rc = OFFLOAD_SUCCESS;
  void *SrcAddr = (char *)const_cast<void *>(Src) + SrcOffset;
  void *DstAddr = (char *)Dst + DstOffset;

  // OpenMP 5.1, sec. 4.5.2.25 "ompt_callback_target_data_op_emi_t and
  // ompt_callback_target_data_op_t", p. 536, L25-27:
  // "A thread dispatches a registered ompt_callback_target_data_op_emi or
  // ompt_callback_target_data_op callback when device memory is allocated or
  // freed, as well as when data is copied to or from a device."
  //
  // TODO: Currently, we have not implemented callbacks for direct transfers
  // within host memory (memcpy), within a single device's memory
  // (data_exchange), or between different devices (data_exchange).  It's not
  // clear whether callbacks are desired in each of those cases and whether
  // ompt_target_data_transfer_to_device, ompt_target_data_transfer_from_device,
  // or both (one relative to each device) should be specified in such cases.
  // We have implemented both callbacks when a direct transfer between the
  // devices is not possible and so transfers to/from the initial device are
  // implemented (retrieveData and submitData), and we have implemented the
  // appropriate callback when the transfer is specified as between host and a
  // device (retreiveData or submitData).
  OMPT_SET_TRIGGER_IDENT();
  if (SrcDevice == omp_get_initial_device() &&
      DstDevice == omp_get_initial_device()) {
    DP("copy from host to host\n");
    const void *P = memcpy(DstAddr, SrcAddr, Length);
    if (P == NULL)
      Rc = OFFLOAD_FAIL;
  } else if (SrcDevice == omp_get_initial_device()) {
    DP("copy from host to device\n");
    DeviceTy &DstDev = *PM->Devices[DstDevice];
    AsyncInfoTy AsyncInfo(DstDev);
    Rc = DstDev.submitData(DstAddr, SrcAddr, Length, AsyncInfo);
  } else if (DstDevice == omp_get_initial_device()) {
    DP("copy from device to host\n");
    DeviceTy &SrcDev = *PM->Devices[SrcDevice];
    AsyncInfoTy AsyncInfo(SrcDev);
    Rc = SrcDev.retrieveData(DstAddr, SrcAddr, Length, AsyncInfo);
  } else {
    DP("copy from device to device\n");
    DeviceTy &SrcDev = *PM->Devices[SrcDevice];
    DeviceTy &DstDev = *PM->Devices[DstDevice];
    // First try to use D2D memcpy which is more efficient. If fails, fall back
    // to unefficient way.
    if (SrcDev.isDataExchangable(DstDev)) {
      AsyncInfoTy AsyncInfo(SrcDev);
      Rc = SrcDev.dataExchange(SrcAddr, DstDev, DstAddr, Length, AsyncInfo);
      if (Rc == OFFLOAD_SUCCESS) {
        OMPT_CLEAR_TRIGGER_IDENT();
        return OFFLOAD_SUCCESS;
      }
    }

    void *Buffer = malloc(Length);
    {
      AsyncInfoTy AsyncInfo(SrcDev);
      Rc = SrcDev.retrieveData(Buffer, SrcAddr, Length, AsyncInfo);
    }
    if (Rc == OFFLOAD_SUCCESS) {
      AsyncInfoTy AsyncInfo(SrcDev);
      Rc = DstDev.submitData(DstAddr, Buffer, Length, AsyncInfo);
    }
    free(Buffer);
  }
  OMPT_CLEAR_TRIGGER_IDENT();

  DP("omp_target_memcpy returns %d\n", Rc);
  return Rc;
}

EXTERN int
omp_target_memcpy_rect(void *Dst, const void *Src, size_t ElementSize,
                       int NumDims, const size_t *Volume,
                       const size_t *DstOffsets, const size_t *SrcOffsets,
                       const size_t *DstDimensions, const size_t *SrcDimensions,
                       int DstDevice, int SrcDevice) {
  TIMESCOPE();
  DP("Call to omp_target_memcpy_rect, dst device %d, src device %d, "
     "dst addr " DPxMOD ", src addr " DPxMOD ", dst offsets " DPxMOD ", "
     "src offsets " DPxMOD ", dst dims " DPxMOD ", src dims " DPxMOD ", "
     "volume " DPxMOD ", element size %zu, num_dims %d\n",
     DstDevice, SrcDevice, DPxPTR(Dst), DPxPTR(Src), DPxPTR(DstOffsets),
     DPxPTR(SrcOffsets), DPxPTR(DstDimensions), DPxPTR(SrcDimensions),
     DPxPTR(Volume), ElementSize, NumDims);

  if (!(Dst || Src)) {
    DP("Call to omp_target_memcpy_rect returns max supported dimensions %d\n",
       INT_MAX);
    return INT_MAX;
  }

  if (!Dst || !Src || ElementSize < 1 || NumDims < 1 || !Volume ||
      !DstOffsets || !SrcOffsets || !DstDimensions || !SrcDimensions) {
    REPORT("Call to omp_target_memcpy_rect with invalid arguments\n");
    return OFFLOAD_FAIL;
  }

  int Rc;
  if (NumDims == 1) {
    Rc = omp_target_memcpy(Dst, Src, ElementSize * Volume[0],
                           ElementSize * DstOffsets[0],
                           ElementSize * SrcOffsets[0], DstDevice, SrcDevice);
  } else {
    size_t DstSliceSize = ElementSize;
    size_t SrcSliceSize = ElementSize;
    for (int I = 1; I < NumDims; ++I) {
      DstSliceSize *= DstDimensions[I];
      SrcSliceSize *= SrcDimensions[I];
    }

    size_t DstOff = DstOffsets[0] * DstSliceSize;
    size_t SrcOff = SrcOffsets[0] * SrcSliceSize;
    for (size_t I = 0; I < Volume[0]; ++I) {
      Rc = omp_target_memcpy_rect(
          (char *)Dst + DstOff + DstSliceSize * I,
          (char *)const_cast<void *>(Src) + SrcOff + SrcSliceSize * I,
          ElementSize, NumDims - 1, Volume + 1, DstOffsets + 1, SrcOffsets + 1,
          DstDimensions + 1, SrcDimensions + 1, DstDevice, SrcDevice);

      if (Rc) {
        DP("Recursive call to omp_target_memcpy_rect returns unsuccessfully\n");
        return Rc;
      }
    }
  }

  DP("omp_target_memcpy_rect returns %d\n", Rc);
  return Rc;
}

EXTERN int omp_target_associate_ptr(const void *HostPtr, const void *DevicePtr,
                                    size_t Size, size_t DeviceOffset,
                                    int DeviceNum) {
  OMPT_DEFINE_IDENT(omp_target_associate_ptr);
  TIMESCOPE();
  DP("Call to omp_target_associate_ptr with host_ptr " DPxMOD ", "
     "device_ptr " DPxMOD ", size %zu, device_offset %zu, device_num %d\n",
     DPxPTR(HostPtr), DPxPTR(DevicePtr), Size, DeviceOffset, DeviceNum);

  if (!HostPtr || !DevicePtr || Size <= 0) {
    REPORT("Call to omp_target_associate_ptr with invalid arguments\n");
    return OFFLOAD_FAIL;
  }

  if (DeviceNum == omp_get_initial_device()) {
    REPORT("omp_target_associate_ptr: no association possible on the host\n");
    return OFFLOAD_FAIL;
  }

  if (!deviceIsReady(DeviceNum)) {
    REPORT("omp_target_associate_ptr returns OFFLOAD_FAIL\n");
    return OFFLOAD_FAIL;
  }

  DeviceTy &Device = *PM->Devices[DeviceNum];
  void *DeviceAddr = (void *)((uint64_t)DevicePtr + (uint64_t)DeviceOffset);
  // OpenMP 5.1, sec. 3.8.9, p. 428, L10-17:
  // "The target-data-associate event occurs before a thread initiates a device
  // pointer association on a target device."
  // "A thread dispatches a registered ompt_callback_target_data_op callback, or
  // a registered ompt_callback_target_data_op_emi callback with
  // ompt_scope_beginend as its endpoint argument for each occurrence of a
  // target-data-associate event in that thread. These callbacks have type
  // signature ompt_callback_target_data_op_t or
  // ompt_callback_target_data_op_emi_t, respectively."
  //
  // OpenMP 5.1, sec. 4.5.2.25 "ompt_callback_target_data_op_emi_t and
  // ompt_callback_target_data_op_t", p. 536, L25-27:
  // "A thread dispatches a registered ompt_callback_target_data_op_emi or
  // ompt_callback_target_data_op callback when device memory is allocated or
  // freed, as well as when data is copied to or from a device."
  //
  // TODO: We have little choice but to cast away const here: OpenMP 5.1
  // specifies omp_target_associate_ptr with "const void *" but specifies
  // the associated callback with "void *".
  OMPT_DISPATCH_CALLBACK_TARGET_DATA_OP_EMI(
      ompt_scope_beginend, associate, const_cast<void*>(HostPtr),
      omp_get_initial_device(), DeviceAddr, DeviceNum, Size);
  int Rc = Device.associatePtr(const_cast<void *>(HostPtr),
                               const_cast<void *>(DeviceAddr), Size);
  DP("omp_target_associate_ptr returns %d\n", Rc);
  return Rc;
}

EXTERN int omp_target_disassociate_ptr(const void *HostPtr, int DeviceNum) {
  OMPT_DEFINE_IDENT(omp_target_disassociate_ptr);
  TIMESCOPE();
  DP("Call to omp_target_disassociate_ptr with host_ptr " DPxMOD ", "
     "device_num %d\n",
     DPxPTR(HostPtr), DeviceNum);

  if (!HostPtr) {
    REPORT("Call to omp_target_associate_ptr with invalid host_ptr\n");
    return OFFLOAD_FAIL;
  }

  if (DeviceNum == omp_get_initial_device()) {
    REPORT(
        "omp_target_disassociate_ptr: no association possible on the host\n");
    return OFFLOAD_FAIL;
  }

  if (!deviceIsReady(DeviceNum)) {
    REPORT("omp_target_disassociate_ptr returns OFFLOAD_FAIL\n");
    return OFFLOAD_FAIL;
  }

  DeviceTy &Device = *PM->Devices[DeviceNum];
  void *TgtPtrBegin;
  int64_t Size;
  int Rc = Device.disassociatePtr(const_cast<void *>(HostPtr), TgtPtrBegin,
                                  Size);
  // OpenMP 5.1, sec. 3.8.10, p. 430, L2-9:
  // "The target-data-disassociate event occurs before a thread initiates a
  // device pointer disassociation on a target device."
  // "A thread dispatches a registered ompt_callback_target_data_op callback, or
  // a registered ompt_callback_target_data_op_emi callback with
  // ompt_scope_beginend as its endpoint argument for each occurrence of a
  // target-data-disassociate event in that thread. These callbacks have type
  // signature ompt_callback_target_data_op_t or
  // ompt_callback_target_data_op_emi_t, respectively."
  //
  // OpenMP 5.1, sec. 4.5.2.25 "ompt_callback_target_data_op_emi_t and
  // ompt_callback_target_data_op_t", p. 536, L25-27:
  // "A thread dispatches a registered ompt_callback_target_data_op_emi or
  // ompt_callback_target_data_op callback when device memory is allocated or
  // freed, as well as when data is copied to or from a device."
  //
  // TODO: We have little choice but to cast away const here: OpenMP 5.1
  // specifies omp_target_disassociate_ptr with "const void *" but specifies
  // the associated callback with "void *".
  OMPT_DISPATCH_CALLBACK_TARGET_DATA_OP_EMI(
      ompt_scope_beginend, disassociate, const_cast<void *>(HostPtr),
      omp_get_initial_device(), TgtPtrBegin, DeviceNum, Size);
  DP("omp_target_disassociate_ptr returns %d\n", Rc);
  return Rc;
}

EXTERN void *omp_target_map_to(void *ptr, size_t size, int device_num) {
  DEFINE_IDENT(omp_target_map_to);
  int64_t tgt_map_type = OMP_TGT_MAPTYPE_TO;
  int64_t s = size;
  __tgt_target_data_begin_mapper(&Ident, device_num, 1, &ptr, &ptr, &s,
                                 &tgt_map_type, NULL, NULL);
  return omp_get_mapped_ptr(ptr, device_num);
}

EXTERN void *omp_target_map_alloc(void *ptr, size_t size, int device_num) {
  DEFINE_IDENT(omp_target_map_alloc);
  int64_t tgt_map_type = OMP_TGT_MAPTYPE_NONE;
  int64_t s = size;
  __tgt_target_data_begin_mapper(&Ident, device_num, 1, &ptr, &ptr, &s,
                                 &tgt_map_type, NULL, NULL);
  return omp_get_mapped_ptr(ptr, device_num);
}

EXTERN void omp_target_map_from(void *ptr, size_t size, int device_num) {
  DEFINE_IDENT(omp_target_map_from);
  int64_t tgt_map_type = OMP_TGT_MAPTYPE_FROM;
  int64_t s = size;
  __tgt_target_data_end_mapper(&Ident, device_num, 1, &ptr, &ptr, &s,
                               &tgt_map_type, NULL, NULL);
}

EXTERN void omp_target_map_from_delete(void *ptr, size_t size, int device_num) {
  DEFINE_IDENT(omp_target_map_from_delete);
  int64_t tgt_map_type = OMP_TGT_MAPTYPE_FROM | OMP_TGT_MAPTYPE_DELETE;
  int64_t s = size;
  __tgt_target_data_end_mapper(&Ident, device_num, 1, &ptr, &ptr, &s,
                               &tgt_map_type, NULL, NULL);
}

EXTERN void omp_target_map_release(void *ptr, size_t size, int device_num) {
  DEFINE_IDENT(omp_target_map_release);
  int64_t tgt_map_type = OMP_TGT_MAPTYPE_NONE;
  int64_t s = size;
  __tgt_target_data_end_mapper(&Ident, device_num, 1, &ptr, &ptr, &s,
                               &tgt_map_type, NULL, NULL);
}

EXTERN void omp_target_map_delete(void *ptr, size_t size, int device_num) {
  DEFINE_IDENT(omp_target_map_delete);
  int64_t tgt_map_type = OMP_TGT_MAPTYPE_DELETE;
  int64_t s = size;
  __tgt_target_data_end_mapper(&Ident, device_num, 1, &ptr, &ptr, &s,
                               &tgt_map_type, NULL, NULL);
}

EXTERN void omp_target_update_to(void *ptr, size_t size, int device_num) {
  DEFINE_IDENT(omp_target_update_to);
  int64_t tgt_map_type = OMP_TGT_MAPTYPE_TO | OMP_TGT_MAPTYPE_PRESENT;
  int64_t s = size;
  __tgt_target_data_update_mapper(&Ident, device_num, 1, &ptr, &ptr, &s,
                                  &tgt_map_type, NULL, NULL);
}

EXTERN void omp_target_update_from(void *ptr, size_t size, int device_num) {
  DEFINE_IDENT(omp_target_update_from);
  int64_t tgt_map_type = OMP_TGT_MAPTYPE_FROM | OMP_TGT_MAPTYPE_PRESENT;
  int64_t s = size;
  __tgt_target_data_update_mapper(&Ident, device_num, 1, &ptr, &ptr, &s,
                                  &tgt_map_type, NULL, NULL);
}

EXTERN omp_device_t omp_get_device_type(int device_num) {
  if (device_num == omp_get_initial_device()) {
    DP("omp_get_device_type returns omp_device_host\n");
    return omp_device_host;
  }

  PM->RTLsMtx.lock();
  size_t Devices_size = PM->Devices.size();
  PM->RTLsMtx.unlock();

  if (Devices_size <= (size_t)device_num) {
    DP("omp_get_device_type returns omp_device_none for invalid device ID %d\n",
       device_num);
    return omp_device_none;
  }
  omp_device_t DeviceType = PM->Devices[device_num]->RTL->DeviceType;
  DP("omp_get_device_type returns %s=%d for device ID %d\n",
     deviceTypeToString(DeviceType), DeviceType, device_num);
  return DeviceType;
}

EXTERN int omp_get_num_devices_of_type(omp_device_t device_type) {
  if (device_type == omp_device_none) {
    DP("omp_get_num_devices_of_type returns 0 for omp_device_none\n");
    return 0;
  }
  if (device_type == omp_device_host) {
    DP("omp_get_num_devices_of_type returns 1 for omp_device_host\n");
    return 1;
  }

  PM->RTLsMtx.lock();
  RTLInfoTy *RTL = PM->RTLs.AllRTLMap[device_type];
  PM->RTLsMtx.unlock();

  if (!RTL) {
    DP("omp_get_num_devices_of_type returns 0 for %s=%d, which has no mapped "
       "runtime library\n",
       deviceTypeToString(device_type), device_type);
    return 0;
  }
  if (RTL->Idx == -1) {
    DP("omp_get_num_devices_of_type returns 0 for %s=%d, whose mapped runtime "
       "library has no device list\n",
       deviceTypeToString(device_type), device_type);
    return 0;
  }
  int Size = RTL->NumberOfDevices;
  DP("omp_get_num_devices_of_type returns %d for %s=%d\n",
     Size, deviceTypeToString(device_type), device_type);
  return Size;
}

EXTERN int omp_get_typed_device_num(int device_num) {
  if (device_num == omp_get_initial_device()) {
    DP("omp_get_typed_device_num returns 0 for host device\n");
    return 0;
  }

  PM->RTLsMtx.lock();
  size_t Devices_size = PM->Devices.size();
  PM->RTLsMtx.unlock();

  if (Devices_size <= (size_t)device_num) {
    DP("omp_get_typed_device_num returns -1 for invalid device ID %d\n",
       device_num);
    return -1;
  }
  int TypedDeviceNum = PM->Devices[device_num]->RTLDeviceID;
  DP("omp_get_typed_device_num returns %d for device ID %d\n",
     TypedDeviceNum, device_num);
  return TypedDeviceNum;
}

EXTERN int omp_get_device_of_type(omp_device_t device_type,
                                  int typed_device_num) {
  if (device_type == omp_device_none) {
    DP("omp_get_device_of_type returns -1 for omp_device_none\n");
    return -1;
  }
  if (device_type == omp_device_host) {
    if (typed_device_num == 0) {
      DP("omp_get_device_of_type returns host device for omp_device_host with "
         "typed device ID 0\n");
      return omp_get_initial_device();
    }
    DP("omp_get_device_of_type returns -1 for omp_device_host with invalid "
       "typed device ID %d\n", typed_device_num);
    return -1;
  }

  PM->RTLsMtx.lock();
  RTLInfoTy *RTL = PM->RTLs.AllRTLMap[device_type];
  PM->RTLsMtx.unlock();

  if (!RTL) {
    DP("omp_get_device_of_type returns -1 for %s=%d, which has no mapped "
       "runtime library\n",
       deviceTypeToString(device_type), device_type);
    return -1;
  }
  if (RTL->Idx == -1) {
    DP("omp_get_device_of_type returns -1 for %s=%d, whose mapped runtime "
       "library has no device list\n",
       deviceTypeToString(device_type), device_type);
    return -1;
  }
  if (typed_device_num < 0 || RTL->NumberOfDevices <= typed_device_num) {
    DP("omp_get_device_of_type returns -1 for %s=%d with invalid typed device "
       "ID %d\n",
       deviceTypeToString(device_type), device_type, typed_device_num);
    return -1;
  }
  int DeviceNum = RTL->Idx + typed_device_num;
  DP("omp_get_device_of_type returns %d for %s=%d with typed device ID %d\n",
     DeviceNum, deviceTypeToString(device_type), device_type, typed_device_num);
  return DeviceNum;
}
