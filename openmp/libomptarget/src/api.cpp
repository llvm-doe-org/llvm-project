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
#include "private.h"
#include "rtl.h"

#include <climits>
#include <cstring>
#include <cstdlib>

#if OMPT_SUPPORT
# define OMPT_SET_DIRECTIVE_INFO() ompt_set_directive_info(__func__)
# define OMPT_CLEAR_DIRECTIVE_INFO() __kmpc_clear_directive_info()
# define OMPT_DISPATCH_CALLBACK_TARGET_DATA_OP_EMI(                            \
    EndPoint, OpKind, SrcPtr, SrcDevNum, DestPtr, DestDevNum, Size)            \
  ompt_dispatch_callback_target_data_op_emi(                                   \
      __func__, EndPoint, ompt_target_data_##OpKind, SrcPtr, SrcDevNum,        \
      DestPtr, DestDevNum, Size)
static void ompt_set_directive_info(const char *DirKind) {
  __kmpc_set_directive_info(ompt_directive_runtime_api,
                            /*is_explicit_event=*/true, /*src_file=*/NULL,
                            /*func_name=*/DirKind, /*line_no=*/0,
                            /*end_line_no=*/0, /*func_line_no=*/0,
                            /*func_end_line_no=*/0);
}
static void ompt_dispatch_callback_target_data_op_emi(
    const char *DirKind, ompt_scope_endpoint_t EndPoint,
    ompt_target_data_op_t OpKind, void *SrcPtr, int SrcDevNum, void *DestPtr,
    int DestDevNum, size_t Size) {
  if (ompt_get_enabled().ompt_callback_target_data_op_emi) {
    ompt_set_directive_info(DirKind);
    // FIXME: We don't yet need the target_task_data, target_data, host_op_id,
    // and codeptr_ra arguments for OpenACC support, so we haven't bothered to
    // implement them yet.
    ompt_get_callbacks().ompt_callback(ompt_callback_target_data_op_emi)(
        EndPoint, /*target_task_data=*/NULL, /*target_data=*/NULL,
        /*host_op_id=*/NULL, OpKind, SrcPtr, SrcDevNum, DestPtr, DestDevNum,
        Size, /*codeptr_ra=*/NULL);
    OMPT_CLEAR_DIRECTIVE_INFO();
  }
}
#else
# define OMPT_SET_DIRECTIVE_INFO()
# define OMPT_CLEAR_DIRECTIVE_INFO()
# define OMPT_DISPATCH_CALLBACK_TARGET_DATA_OP_EMI(                            \
    EndPoint, OpKind, SrcPtr, SrcDevNum, DestPtr, DestDevNum, Size)
#endif

EXTERN int omp_get_num_devices(void) {
  PM->RTLsMtx.lock();
  size_t DevicesSize = PM->Devices.size();
  PM->RTLsMtx.unlock();

  DP("Call to omp_get_num_devices returning %zd\n", DevicesSize);

  return DevicesSize;
}

EXTERN int omp_get_initial_device(void) {
  int hostDevice = omp_get_num_devices();
  DP("Call to omp_get_initial_device returning %d\n", hostDevice);
  return hostDevice;
}

#if OMPT_SUPPORT
// FIXME: Access is not thread-safe.  Does it need to be?
// FIXME: Does this work if you allocate on multiple devices?  There might be
// address collisions.  Either key it by device or store it in DeviceTy?
static std::map<void *, size_t> DeviceAllocSizes;
#endif

EXTERN void *omp_target_alloc(size_t size, int device_num) {
  DP("Call to omp_target_alloc for device %d requesting %zu bytes\n",
      device_num, size);

  if (size <= 0) {
    DP("Call to omp_target_alloc with non-positive length\n");
    return NULL;
  }

  void *rc = NULL;

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
  if (device_num == omp_get_initial_device()) {
    rc = malloc(size);
#if OMPT_SUPPORT
    // TODO: If offloading is disabled, the runtime might not be initialized.
    // In that case, ompt_start_tool hasn't been called, so we don't know what
    // OMPT callbacks to dispatch.  Calling __kmpc_get_target_offload guarantees
    // the runtime is initialized, but maybe we should expose something like
    // __kmpc_initialize_runtime to call here instead.
    __kmpc_get_target_offload();
    DeviceAllocSizes[rc] = size;
    OMPT_DISPATCH_CALLBACK_TARGET_DATA_OP_EMI(ompt_scope_end, alloc,
                                              /*SrcPtr=*/NULL, device_num, rc,
                                              device_num, size);
#endif
    DP("omp_target_alloc returns host ptr " DPxMOD "\n", DPxPTR(rc));
    return rc;
  }

  OMPT_SET_DIRECTIVE_INFO();
  if (!device_is_ready(device_num)) {
    DP("omp_target_alloc returns NULL ptr\n");
    return NULL;
  }
  OMPT_CLEAR_DIRECTIVE_INFO();

  rc = PM->Devices[device_num].allocData(size);
#if OMPT_SUPPORT
  DeviceAllocSizes[rc] = size;
  OMPT_DISPATCH_CALLBACK_TARGET_DATA_OP_EMI(
      ompt_scope_end, alloc, /*SrcPtr=*/NULL, omp_get_initial_device(), rc,
      device_num, size);
#endif
  DP("omp_target_alloc returns device ptr " DPxMOD "\n", DPxPTR(rc));
  return rc;
}

EXTERN void omp_target_free(void *device_ptr, int device_num) {
  DP("Call to omp_target_free for device %d and address " DPxMOD "\n",
      device_num, DPxPTR(device_ptr));

  if (!device_ptr) {
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
  if (device_num == omp_get_initial_device()) {
    OMPT_DISPATCH_CALLBACK_TARGET_DATA_OP_EMI(ompt_scope_begin, delete,
                                              /*SrcPtr=*/NULL, device_num,
                                              device_ptr, device_num,
                                              DeviceAllocSizes[device_ptr]);
    free(device_ptr);
    DP("omp_target_free deallocated host ptr\n");
    return;
  }

  if (!device_is_ready(device_num)) {
    DP("omp_target_free returns, nothing to do\n");
    return;
  }

#if OMPT_SUPPORT
  OMPT_DISPATCH_CALLBACK_TARGET_DATA_OP_EMI(
      ompt_scope_begin, delete, /*SrcPtr=*/NULL, omp_get_initial_device(),
      device_ptr, device_num, DeviceAllocSizes[device_ptr]);
  DeviceAllocSizes.erase(device_ptr);
#endif
  PM->Devices[device_num].deleteData(device_ptr);
  DP("omp_target_free deallocated device ptr\n");
}

EXTERN int omp_target_is_present(void *ptr, int device_num) {
  DP("Call to omp_target_is_present for device %d and address " DPxMOD "\n",
      device_num, DPxPTR(ptr));

  if (!ptr) {
    DP("Call to omp_target_is_present with NULL ptr, returning false\n");
    return false;
  }

  if (device_num == omp_get_initial_device()) {
    DP("Call to omp_target_is_present on host, returning true\n");
    return true;
  }

  PM->RTLsMtx.lock();
  size_t DevicesSize = PM->Devices.size();
  PM->RTLsMtx.unlock();
  if (DevicesSize <= (size_t)device_num) {
    DP("Call to omp_target_is_present with invalid device ID, returning "
        "false\n");
    return false;
  }

  DeviceTy &Device = PM->Devices[device_num];
  bool IsLast; // not used
  bool IsHostPtr;
  void *TgtPtr = Device.getTgtPtrBegin(ptr, 0, IsLast, /*UpdateRefCount=*/false,
                                       /*UseHoldRefCount=*/false, IsHostPtr);
  int rc = (TgtPtr != NULL);
  // Under unified memory the host pointer can be returned by the
  // getTgtPtrBegin() function which means that there is no device
  // corresponding point for ptr. This function should return false
  // in that situation.
  if (PM->RTLs.RequiresFlags & OMP_REQ_UNIFIED_SHARED_MEMORY)
    rc = !IsHostPtr;
  DP("Call to omp_target_is_present returns %d\n", rc);
  return rc;
}

EXTERN int omp_target_is_accessible(const void *ptr, size_t size,
                                    int device_num) {
  // OpenMP 5.1, sec. 3.8.4 "omp_target_is_accessible", p. 417, L21-22:
  // "This routine returns true if the storage of size bytes starting at the
  // address given by ptr is accessible from device device_num. Otherwise, it
  // returns false."
  //
  // The meaning of "accessible" for unified shared memory is established in
  // OpenMP 5.1, sec. 2.5.1 "requires directive".  More generally, the specified
  // host memory is accessible if it can be accessed from the device either
  // directly (because of unified shared memory or because device_num is the
  // value returned by omp_get_initial_device()) or indirectly (because it's
  // mapped to the device).
  DP("Call to omp_target_is_accessible for device %d and address " DPxMOD "\n",
     device_num, DPxPTR(ptr));

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
  // Should size have any effect on the result when ptr is NULL?
  if (!ptr) {
    DP("Call to omp_target_is_accessible with NULL ptr, returning false\n");
    return false;
  }

  if (device_num == omp_get_initial_device()) {
    DP("Call to omp_target_is_accessible on host, returning true\n");
    return true;
  }

  PM->RTLsMtx.lock();
  size_t Devices_size = PM->Devices.size();
  PM->RTLsMtx.unlock();
  if (Devices_size <= (size_t)device_num) {
    DP("Call to omp_target_is_accessible with invalid device ID, returning "
       "false\n");
    return false;
  }

  DeviceTy &Device = PM->Devices[device_num];
  bool IsLast;    // not used
  bool IsHostPtr; // not used
  // TODO: How does the spec intend for the size=0 case to be handled?
  // Currently, we return true if we would return true for size=1 (ptr is within
  // a range that's accessible).  Does the spec clarify this somewhere?
  void *TgtPtr = Device.getTgtPtrBegin(const_cast<void *>(ptr), size, IsLast,
                                       /*UpdateRefCount=*/false,
                                       /*UseHoldRefCount=*/false, IsHostPtr,
                                       /*MustContain=*/true);
  int rc = (TgtPtr != NULL);
  DP("Call to omp_target_is_accessible returns %d\n", rc);
  return rc;
}

EXTERN void *omp_get_mapped_ptr(const void *ptr, int device_num) {
  DP("Call to omp_get_mapped_ptr for device %d and address " DPxMOD
     "\n",
     device_num, DPxPTR(ptr));

  if (!ptr) {
    DP("Call to omp_get_mapped_ptr with NULL ptr, returning NULL\n");
    return NULL;
  }

  if (device_num == omp_get_initial_device()) {
    DP("Call to omp_get_mapped_ptr on host, returning host pointer\n");
    // OpenMP 5.1, sec. 3.8.11 "omp_get_mapped_ptr", p. 431, L10-12:
    // "Otherwise it returns the device pointer, which is ptr if device_num is
    // the value returned by omp_get_initial_device()."
    //
    // That is, the spec actually requires us to cast away const.
    return const_cast<void *>(ptr);
  }

  PM->RTLsMtx.lock();
  size_t Devices_size = PM->Devices.size();
  PM->RTLsMtx.unlock();
  if (Devices_size <= (size_t)device_num) {
    DP("Call to omp_get_mapped_ptr with invalid device ID, returning NULL\n");
    return NULL;
  }

  DeviceTy &Device = PM->Devices[device_num];
  bool IsLast; // not used
  bool IsHostPtr;
  void *TgtPtr = Device.getTgtPtrBegin(const_cast<void *>(ptr), /*Size=*/0,
                                       IsLast, /*UpdateRefCount=*/false,
                                       /*UseHoldRefCount=*/false, IsHostPtr);
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

EXTERN void *omp_get_mapped_hostptr(const void *ptr, int device_num) {
  DP("Call to omp_get_mapped_hostptr for device %d and address " DPxMOD "\n",
     device_num, DPxPTR(ptr));

  if (!ptr) {
    DP("Call to omp_get_mapped_hostptr with NULL ptr, returning NULL\n");
    return NULL;
  }

  if (device_num == omp_get_initial_device()) {
    DP("Call to omp_get_mapped_hostptr for host, returning device pointer\n");
    // For consistency with OpenMP 5.1, sec. 3.8.11 "omp_get_mapped_ptr", p.
    // 431, L10-12:
    // "Otherwise it returns the device pointer, which is ptr if device_num is
    // the value returned by omp_get_initial_device()."
    return const_cast<void *>(ptr);
  }

  PM->RTLsMtx.lock();
  size_t Devices_size = PM->Devices.size();
  PM->RTLsMtx.unlock();
  if (Devices_size <= (size_t)device_num) {
    DP("Call to omp_get_mapped_hostptr with invalid device ID, returning "
       "NULL\n");
    return NULL;
  }

  DeviceTy &Device = PM->Devices[device_num];
  // TODO: This returns nullptr in the case of unified shared memory.  This is
  // for consistency with the current omp_get_mapped_ptr implementation.
  void *HostPtr = Device.lookupHostPtr(const_cast<void *>(ptr));
  DP("Call to omp_get_mapped_hostptr returns " DPxMOD "\n", DPxPTR(HostPtr));
  return HostPtr;
}

EXTERN size_t omp_get_accessible_buffer(
  const void *ptr, size_t size, int device_num, void **buffer_host,
  void **buffer_device) {
  DP("Call to omp_get_accessible_buffer for address " DPxMOD ", size %zu, and "
     "device %d\n",
     DPxPTR(ptr), size, device_num);

  if (device_num == omp_get_initial_device()) {
    DP("Call to omp_get_accessible_buffer for initial device, returning "
       "SIZE_MAX\n");
    if (buffer_host)
      *buffer_host = nullptr;
    if (buffer_device)
      *buffer_device = nullptr;
    return SIZE_MAX;
  }

  PM->RTLsMtx.lock();
  size_t Devices_size = PM->Devices.size();
  PM->RTLsMtx.unlock();
  if (Devices_size <= (size_t)device_num) {
    DP("Call to omp_get_accessible_buffer with invalid device ID, returning "
       "0\n");
    if (buffer_host)
      *buffer_host = nullptr;
    if (buffer_device)
      *buffer_device = nullptr;
    return 0;
  }

  if (!ptr) {
    DP("Call to omp_get_accessible_buffer with NULL ptr, returning SIZE_MAX\n");
    if (buffer_host)
      *buffer_host = nullptr;
    if (buffer_device)
      *buffer_device = nullptr;
    return SIZE_MAX;
  }

  DeviceTy &Device = PM->Devices[device_num];
  size_t BufferSize = Device.getAccessibleBuffer(const_cast<void *>(ptr),
                                                 /*Size=*/size, buffer_host,
                                                 buffer_device);
  DP("Call to omp_get_accessible_buffer returns %zu\n", BufferSize);
  return BufferSize;;
}

EXTERN int omp_target_memcpy(void *dst, void *src, size_t length,
    size_t dst_offset, size_t src_offset, int dst_device, int src_device) {
  DP("Call to omp_target_memcpy, dst device %d, src device %d, "
      "dst addr " DPxMOD ", src addr " DPxMOD ", dst offset %zu, "
      "src offset %zu, length %zu\n", dst_device, src_device, DPxPTR(dst),
      DPxPTR(src), dst_offset, src_offset, length);

  if (!dst || !src || length <= 0) {
    REPORT("Call to omp_target_memcpy with invalid arguments\n");
    return OFFLOAD_FAIL;
  }

  if (src_device != omp_get_initial_device() && !device_is_ready(src_device)) {
    REPORT("omp_target_memcpy returns OFFLOAD_FAIL\n");
    return OFFLOAD_FAIL;
  }

  if (dst_device != omp_get_initial_device() && !device_is_ready(dst_device)) {
    REPORT("omp_target_memcpy returns OFFLOAD_FAIL\n");
    return OFFLOAD_FAIL;
  }

  int rc = OFFLOAD_SUCCESS;
  void *srcAddr = (char *)src + src_offset;
  void *dstAddr = (char *)dst + dst_offset;

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
  OMPT_SET_DIRECTIVE_INFO();
  if (src_device == omp_get_initial_device() &&
      dst_device == omp_get_initial_device()) {
    DP("copy from host to host\n");
    const void *p = memcpy(dstAddr, srcAddr, length);
    if (p == NULL)
      rc = OFFLOAD_FAIL;
  } else if (src_device == omp_get_initial_device()) {
    DP("copy from host to device\n");
    DeviceTy &DstDev = PM->Devices[dst_device];
    rc = DstDev.submitData(dstAddr, srcAddr, length, nullptr);
  } else if (dst_device == omp_get_initial_device()) {
    DP("copy from device to host\n");
    DeviceTy &SrcDev = PM->Devices[src_device];
    rc = SrcDev.retrieveData(dstAddr, srcAddr, length, nullptr);
  } else {
    DP("copy from device to device\n");
    DeviceTy &SrcDev = PM->Devices[src_device];
    DeviceTy &DstDev = PM->Devices[dst_device];
    // First try to use D2D memcpy which is more efficient. If fails, fall back
    // to unefficient way.
    if (SrcDev.isDataExchangable(DstDev)) {
      rc = SrcDev.dataExchange(srcAddr, DstDev, dstAddr, length, nullptr);
      if (rc == OFFLOAD_SUCCESS) {
        OMPT_CLEAR_DIRECTIVE_INFO();
        return OFFLOAD_SUCCESS;
      }
    }

    void *buffer = malloc(length);
    rc = SrcDev.retrieveData(buffer, srcAddr, length, nullptr);
    if (rc == OFFLOAD_SUCCESS)
      rc = DstDev.submitData(dstAddr, buffer, length, nullptr);
    free(buffer);
  }
  OMPT_CLEAR_DIRECTIVE_INFO();

  DP("omp_target_memcpy returns %d\n", rc);
  return rc;
}

EXTERN int omp_target_memcpy_rect(void *dst, void *src, size_t element_size,
    int num_dims, const size_t *volume, const size_t *dst_offsets,
    const size_t *src_offsets, const size_t *dst_dimensions,
    const size_t *src_dimensions, int dst_device, int src_device) {
  DP("Call to omp_target_memcpy_rect, dst device %d, src device %d, "
      "dst addr " DPxMOD ", src addr " DPxMOD ", dst offsets " DPxMOD ", "
      "src offsets " DPxMOD ", dst dims " DPxMOD ", src dims " DPxMOD ", "
      "volume " DPxMOD ", element size %zu, num_dims %d\n", dst_device,
      src_device, DPxPTR(dst), DPxPTR(src), DPxPTR(dst_offsets),
      DPxPTR(src_offsets), DPxPTR(dst_dimensions), DPxPTR(src_dimensions),
      DPxPTR(volume), element_size, num_dims);

  if (!(dst || src)) {
    DP("Call to omp_target_memcpy_rect returns max supported dimensions %d\n",
        INT_MAX);
    return INT_MAX;
  }

  if (!dst || !src || element_size < 1 || num_dims < 1 || !volume ||
      !dst_offsets || !src_offsets || !dst_dimensions || !src_dimensions) {
    REPORT("Call to omp_target_memcpy_rect with invalid arguments\n");
    return OFFLOAD_FAIL;
  }

  int rc;
  if (num_dims == 1) {
    rc = omp_target_memcpy(dst, src, element_size * volume[0],
        element_size * dst_offsets[0], element_size * src_offsets[0],
        dst_device, src_device);
  } else {
    size_t dst_slice_size = element_size;
    size_t src_slice_size = element_size;
    for (int i=1; i<num_dims; ++i) {
      dst_slice_size *= dst_dimensions[i];
      src_slice_size *= src_dimensions[i];
    }

    size_t dst_off = dst_offsets[0] * dst_slice_size;
    size_t src_off = src_offsets[0] * src_slice_size;
    for (size_t i=0; i<volume[0]; ++i) {
      rc = omp_target_memcpy_rect((char *) dst + dst_off + dst_slice_size * i,
          (char *) src + src_off + src_slice_size * i, element_size,
          num_dims - 1, volume + 1, dst_offsets + 1, src_offsets + 1,
          dst_dimensions + 1, src_dimensions + 1, dst_device, src_device);

      if (rc) {
        DP("Recursive call to omp_target_memcpy_rect returns unsuccessfully\n");
        return rc;
      }
    }
  }

  DP("omp_target_memcpy_rect returns %d\n", rc);
  return rc;
}

EXTERN int omp_target_associate_ptr(void *host_ptr, void *device_ptr,
    size_t size, size_t device_offset, int device_num) {
  DP("Call to omp_target_associate_ptr with host_ptr " DPxMOD ", "
      "device_ptr " DPxMOD ", size %zu, device_offset %zu, device_num %d\n",
      DPxPTR(host_ptr), DPxPTR(device_ptr), size, device_offset, device_num);

  if (!host_ptr || !device_ptr || size <= 0) {
    REPORT("Call to omp_target_associate_ptr with invalid arguments\n");
    return OFFLOAD_FAIL;
  }

  if (device_num == omp_get_initial_device()) {
    REPORT("omp_target_associate_ptr: no association possible on the host\n");
    return OFFLOAD_FAIL;
  }

  if (!device_is_ready(device_num)) {
    REPORT("omp_target_associate_ptr returns OFFLOAD_FAIL\n");
    return OFFLOAD_FAIL;
  }

  DeviceTy &Device = PM->Devices[device_num];
  void *device_addr = (void *)((uint64_t)device_ptr + (uint64_t)device_offset);
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
  OMPT_DISPATCH_CALLBACK_TARGET_DATA_OP_EMI(ompt_scope_beginend, associate,
                                            host_ptr, omp_get_initial_device(),
                                            device_addr, device_num, size);
  int rc = Device.associatePtr(host_ptr, device_addr, size);
  DP("omp_target_associate_ptr returns %d\n", rc);
  return rc;
}

EXTERN int omp_target_disassociate_ptr(void *host_ptr, int device_num) {
  DP("Call to omp_target_disassociate_ptr with host_ptr " DPxMOD ", "
      "device_num %d\n", DPxPTR(host_ptr), device_num);

  if (!host_ptr) {
    REPORT("Call to omp_target_associate_ptr with invalid host_ptr\n");
    return OFFLOAD_FAIL;
  }

  if (device_num == omp_get_initial_device()) {
    REPORT(
        "omp_target_disassociate_ptr: no association possible on the host\n");
    return OFFLOAD_FAIL;
  }

  if (!device_is_ready(device_num)) {
    REPORT("omp_target_disassociate_ptr returns OFFLOAD_FAIL\n");
    return OFFLOAD_FAIL;
  }

  DeviceTy &Device = PM->Devices[device_num];
  void *TgtPtrBegin;
  int64_t Size;
  int rc = Device.disassociatePtr(host_ptr, TgtPtrBegin, Size);
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
  OMPT_DISPATCH_CALLBACK_TARGET_DATA_OP_EMI(ompt_scope_beginend, disassociate,
                                            host_ptr, omp_get_initial_device(),
                                            TgtPtrBegin, device_num, Size);
  DP("omp_target_disassociate_ptr returns %d\n", rc);
  return rc;
}

EXTERN void *omp_target_map_to(void *ptr, size_t size, int device_num) {
  int64_t tgt_map_type = OMP_TGT_MAPTYPE_TO;
  int64_t s = size;
  OMPT_SET_DIRECTIVE_INFO();
  static ident_t ident = {0, 0, 0, 0, ";;omp_target_map_to;0;0;;"};
  __tgt_target_data_begin_mapper(&ident, device_num, 1, &ptr, &ptr, &s,
                                 &tgt_map_type, NULL, NULL);
  OMPT_CLEAR_DIRECTIVE_INFO();
  return omp_get_mapped_ptr(ptr, device_num);
}

EXTERN void *omp_target_map_alloc(void *ptr, size_t size, int device_num) {
  int64_t tgt_map_type = OMP_TGT_MAPTYPE_NONE;
  int64_t s = size;
  OMPT_SET_DIRECTIVE_INFO();
  static ident_t ident = {0, 0, 0, 0, ";;omp_target_map_alloc;0;0;;"};
  __tgt_target_data_begin_mapper(&ident, device_num, 1, &ptr, &ptr, &s,
                                 &tgt_map_type, NULL, NULL);
  OMPT_CLEAR_DIRECTIVE_INFO();
  return omp_get_mapped_ptr(ptr, device_num);
}

EXTERN void omp_target_map_from(void *ptr, size_t size, int device_num) {
  int64_t tgt_map_type = OMP_TGT_MAPTYPE_FROM;
  int64_t s = size;
  OMPT_SET_DIRECTIVE_INFO();
  static ident_t ident = {0, 0, 0, 0, ";;omp_target_map_from;0;0;;"};
  __tgt_target_data_end_mapper(&ident, device_num, 1, &ptr, &ptr, &s,
                               &tgt_map_type, NULL, NULL);
  OMPT_CLEAR_DIRECTIVE_INFO();
}

EXTERN void omp_target_map_from_delete(void *ptr, size_t size, int device_num) {
  int64_t tgt_map_type = OMP_TGT_MAPTYPE_FROM | OMP_TGT_MAPTYPE_DELETE;
  int64_t s = size;
  OMPT_SET_DIRECTIVE_INFO();
  static ident_t ident = {0, 0, 0, 0, ";;omp_target_map_from_delete;0;0;;"};
  __tgt_target_data_end_mapper(&ident, device_num, 1, &ptr, &ptr, &s,
                               &tgt_map_type, NULL, NULL);
  OMPT_CLEAR_DIRECTIVE_INFO();
}

EXTERN void omp_target_map_release(void *ptr, size_t size, int device_num) {
  int64_t tgt_map_type = OMP_TGT_MAPTYPE_NONE;
  int64_t s = size;
  OMPT_SET_DIRECTIVE_INFO();
  static ident_t ident = {0, 0, 0, 0, ";;omp_target_map_release;0;0;;"};
  __tgt_target_data_end_mapper(&ident, device_num, 1, &ptr, &ptr, &s,
                               &tgt_map_type, NULL, NULL);
  OMPT_CLEAR_DIRECTIVE_INFO();
}

EXTERN void omp_target_map_delete(void *ptr, size_t size, int device_num) {
  int64_t tgt_map_type = OMP_TGT_MAPTYPE_DELETE;
  int64_t s = size;
  OMPT_SET_DIRECTIVE_INFO();
  static ident_t ident = {0, 0, 0, 0, ";;omp_target_map_delete;0;0;;"};
  __tgt_target_data_end_mapper(&ident, device_num, 1, &ptr, &ptr, &s,
                               &tgt_map_type, NULL, NULL);
  OMPT_CLEAR_DIRECTIVE_INFO();
}

EXTERN void omp_target_update_to(void *ptr, size_t size, int device_num) {
  int64_t tgt_map_type = OMP_TGT_MAPTYPE_TO | OMP_TGT_MAPTYPE_PRESENT;
  int64_t s = size;
  OMPT_SET_DIRECTIVE_INFO();
  static ident_t ident = {0, 0, 0, 0, ";;omp_target_update_to;0;0;;"};
  __tgt_target_data_update_mapper(&ident, device_num, 1, &ptr, &ptr, &s,
                                  &tgt_map_type, NULL, NULL);
  OMPT_CLEAR_DIRECTIVE_INFO();
}

EXTERN void omp_target_update_from(void *ptr, size_t size, int device_num) {
  int64_t tgt_map_type = OMP_TGT_MAPTYPE_FROM | OMP_TGT_MAPTYPE_PRESENT;
  int64_t s = size;
  OMPT_SET_DIRECTIVE_INFO();
  static ident_t ident = {0, 0, 0, 0, ";;omp_target_update_from;0;0;;"};
  __tgt_target_data_update_mapper(&ident, device_num, 1, &ptr, &ptr, &s,
                                  &tgt_map_type, NULL, NULL);
  OMPT_CLEAR_DIRECTIVE_INFO();
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
  omp_device_t DeviceType = PM->Devices[device_num].RTL->DeviceType;
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
  int TypedDeviceNum = PM->Devices[device_num].RTLDeviceID;
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
