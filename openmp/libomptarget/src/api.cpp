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

#include <omptarget.h>

#include "device.h"
#include "private.h"
#include "rtl.h"

#include <climits>
#include <cstring>
#include <cstdlib>

#if OMPT_SUPPORT
# define OMPT_SET_DIRECTIVE_INFO(DirKind)                                      \
  __kmpc_set_directive_info(DirKind, /*is_explicit_event=*/true,               \
                            /*src_file=*/NULL, /*func_name=*/NULL,             \
                            /*line_no=*/0, /*end_line_no=*/0,                  \
                            /*func_line_no=*/0, /*func_end_line_no=*/0)
# define OMPT_CLEAR_DIRECTIVE_INFO()                                           \
    __kmpc_clear_directive_info()
# define OMPT_DISPATCH_CALLBACK_TARGET_DATA_OP(                                \
    DirKind, OpKind, SrcPtr, SrcDevNum, DestPtr, DestDevNum, Size)             \
  ompt_dispatch_callback_target_data_op(ompt_directive_##DirKind,              \
                                        ompt_target_data_##OpKind, SrcPtr,     \
                                        SrcDevNum, DestPtr, DestDevNum, Size)
static void ompt_dispatch_callback_target_data_op(ompt_directive_kind_t DirKind,
                                                  ompt_target_data_op_t OpKind,
                                                  void *SrcPtr, int SrcDevNum,
                                                  void *DestPtr, int DestDevNum,
                                                  size_t Size) {
  if (ompt_get_enabled().ompt_callback_target_data_op) {
    OMPT_SET_DIRECTIVE_INFO(DirKind);
    // FIXME: We don't yet need the host_op_id and codeptr_ra arguments for
    // OpenACC support, so we haven't bothered to implement them yet.
    ompt_get_callbacks().ompt_callback(ompt_callback_target_data_op)(
        /*target_id=*/ompt_id_none, /*host_op_id=*/ompt_id_none, OpKind, SrcPtr,
        SrcDevNum, DestPtr, DestDevNum, Size, /*codeptr_ra=*/NULL);
    OMPT_CLEAR_DIRECTIVE_INFO();
  }
}
#else
# define OMPT_DISPATCH_CALLBACK_TARGET_DATA_OP(                                \
    DirKind, OpKind, SrcPtr, SrcDevNum, DestPtr, DestDevNum, Size)
# define OMPT_SET_DIRECTIVE_INFO(DirKind)
# define OMPT_CLEAR_DIRECTIVE_INFO()
#endif

EXTERN int omp_get_num_devices(void) {
  RTLsMtx->lock();
  size_t Devices_size = Devices.size();
  RTLsMtx->unlock();

  DP("Call to omp_get_num_devices returning %zd\n", Devices_size);

  return Devices_size;
}

EXTERN int omp_get_initial_device(void) {
  DP("Call to omp_get_initial_device returning %d\n", HOST_DEVICE);
  return HOST_DEVICE;
}

#if OMPT_SUPPORT
// FIXME: Access is not thread-safe.  Does it need to be?
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

  // OpenMP 5.0 sec. 3.6.1 p. 398 L18:
  // "The target-data-allocation event occurs when a thread allocates data on a
  // target device."
  // OpenMP 5.0 sec. 4.5.2.25 p. 489 L26-27:
  // "A registered ompt_callback_target_data_op callback is dispatched when
  // device memory is allocated or freed, as well as when data is copied to or
  // from a device."
  // The callback must dispatch after the allocation succeeds because it
  // requires the device address.

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
    OMPT_DISPATCH_CALLBACK_TARGET_DATA_OP(omp_target_alloc, alloc,
                                          /*SrcPtr=*/NULL, HOST_DEVICE, rc,
                                          device_num, size);
#endif
    DP("omp_target_alloc returns host ptr " DPxMOD "\n", DPxPTR(rc));
    return rc;
  }

  OMPT_SET_DIRECTIVE_INFO(ompt_directive_omp_target_alloc);
  if (!device_is_ready(device_num)) {
    DP("omp_target_alloc returns NULL ptr\n");
    return NULL;
  }
  OMPT_CLEAR_DIRECTIVE_INFO();

  rc = Devices[device_num].allocData(size);
#if OMPT_SUPPORT
  DeviceAllocSizes[rc] = size;
  OMPT_DISPATCH_CALLBACK_TARGET_DATA_OP(omp_target_alloc, alloc,
                                        /*SrcPtr=*/NULL, HOST_DEVICE, rc,
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

  // OpenMP 5.0 sec. 3.6.2 p. 399 L19:
  // "The target-data-free event occurs when a thread frees data on a target
  // device."
  // OpenMP 5.0 sec. 4.5.2.25 p. 489 L26-27:
  // "A registered ompt_callback_target_data_op callback is dispatched when
  // device memory is allocated or freed, as well as when data is copied to or
  // from a device."
  // We assume the callback should dispatch before the free so that the device
  // address is still valid.

  if (device_num == omp_get_initial_device()) {
    OMPT_DISPATCH_CALLBACK_TARGET_DATA_OP(omp_target_free, delete,
                                          /*SrcPtr=*/NULL, HOST_DEVICE,
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
  OMPT_DISPATCH_CALLBACK_TARGET_DATA_OP(omp_target_free, delete,
                                        /*SrcPtr=*/NULL, HOST_DEVICE,
                                        device_ptr, device_num,
                                        DeviceAllocSizes[device_ptr]);
  DeviceAllocSizes.erase(device_ptr);
#endif
  Devices[device_num].deleteData(device_ptr);
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

  RTLsMtx->lock();
  size_t Devices_size = Devices.size();
  RTLsMtx->unlock();
  if (Devices_size <= (size_t)device_num) {
    DP("Call to omp_target_is_present with invalid device ID, returning "
        "false\n");
    return false;
  }

  DeviceTy& Device = Devices[device_num];
  bool IsLast; // not used
  bool IsHostPtr;
  void *TgtPtr = Device.getTgtPtrBegin(ptr, 0, IsLast, /*UpdateRefCount=*/false,
                                       /*UseHoldRefCount=*/false, IsHostPtr);
  int rc = (TgtPtr != NULL);
  // Under unified memory the host pointer can be returned by the
  // getTgtPtrBegin() function which means that there is no device
  // corresponding point for ptr. This function should return false
  // in that situation.
  if (RTLs->RequiresFlags & OMP_REQ_UNIFIED_SHARED_MEMORY)
    rc = !IsHostPtr;
  DP("Call to omp_target_is_present returns %d\n", rc);
  return rc;
}

EXTERN omp_present_t omp_target_range_is_present(void *ptr, size_t size,
                                                 int device_num) {
  DP("Call to omp_target_range_is_present for device %d and address " DPxMOD
     "\n",
     device_num, DPxPTR(ptr));

  if (!ptr) {
    DP("Call to omp_target_range_is_present with NULL ptr, returning "
       "omp_present_none\n");
    return omp_present_none;
  }

  if (device_num == omp_get_initial_device()) {
    DP("Call to omp_target_range_is_present on host, returning "
       "omp_present_full\n");
    return omp_present_full;
  }

  RTLsMtx->lock();
  size_t Devices_size = Devices.size();
  RTLsMtx->unlock();
  if (Devices_size <= (size_t)device_num) {
    DP("Call to omp_target_range_is_present with invalid device ID, returning "
       "omp_present_none\n");
    return omp_present_none;
  }

  DeviceTy &Device = Devices[device_num];
  LookupResult lr = Device.lookupMapping(ptr, size);
  if (lr.Flags.IsContained) {
    DP("Call to omp_target_range_is_present returns omp_present_full\n");
    return omp_present_full;
  }
  if (lr.Flags.ExtendsBefore || lr.Flags.ExtendsAfter) {
    DP("Call to omp_target_range_is_present returns omp_present_partial\n");
    return omp_present_partial;
  }
  DP("Call to omp_target_range_is_present returns omp_present_none\n");
  return omp_present_none;
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
    return const_cast<void *>(ptr);
  }

  RTLsMtx->lock();
  size_t Devices_size = Devices.size();
  RTLsMtx->unlock();
  if (Devices_size <= (size_t)device_num) {
    DP("Call to omp_get_mapped_ptr with invalid device ID, returning NULL\n");
    return NULL;
  }

  DeviceTy &Device = Devices[device_num];
  LookupResult lr = Device.lookupMapping(const_cast<void *>(ptr), 0);
  if (!lr.Flags.IsContained) {
    DP("Call to omp_get_mapped_ptr for unmapped ptr, returning NULL\n");
    return NULL;
  }
  DP("Call to omp_get_mapped_ptr for mapped ptr, returns non-NULL\n");
  uintptr_t Offset = (uintptr_t)ptr - lr.Entry->HstPtrBegin;
  return (void *)(lr.Entry->TgtPtrBegin + Offset);
}

EXTERN int omp_target_memcpy(void *dst, void *src, size_t length,
    size_t dst_offset, size_t src_offset, int dst_device, int src_device) {
  DP("Call to omp_target_memcpy, dst device %d, src device %d, "
      "dst addr " DPxMOD ", src addr " DPxMOD ", dst offset %zu, "
      "src offset %zu, length %zu\n", dst_device, src_device, DPxPTR(dst),
      DPxPTR(src), dst_offset, src_offset, length);

  if (!dst || !src || length <= 0) {
    DP("Call to omp_target_memcpy with invalid arguments\n");
    return OFFLOAD_FAIL;
  }

  if (src_device != omp_get_initial_device() && !device_is_ready(src_device)) {
      DP("omp_target_memcpy returns OFFLOAD_FAIL\n");
      return OFFLOAD_FAIL;
  }

  if (dst_device != omp_get_initial_device() && !device_is_ready(dst_device)) {
      DP("omp_target_memcpy returns OFFLOAD_FAIL\n");
      return OFFLOAD_FAIL;
  }

  int rc = OFFLOAD_SUCCESS;
  void *srcAddr = (char *)src + src_offset;
  void *dstAddr = (char *)dst + dst_offset;

  if (src_device == omp_get_initial_device() &&
      dst_device == omp_get_initial_device()) {
    DP("copy from host to host\n");
    const void *p = memcpy(dstAddr, srcAddr, length);
    if (p == NULL)
      rc = OFFLOAD_FAIL;
  } else if (src_device == omp_get_initial_device()) {
    DP("copy from host to device\n");
    DeviceTy& DstDev = Devices[dst_device];
    rc = DstDev.submitData(dstAddr, srcAddr, length, nullptr);
  } else if (dst_device == omp_get_initial_device()) {
    DP("copy from device to host\n");
    DeviceTy& SrcDev = Devices[src_device];
    rc = SrcDev.retrieveData(dstAddr, srcAddr, length, nullptr);
  } else {
    DP("copy from device to device\n");
    DeviceTy &SrcDev = Devices[src_device];
    DeviceTy &DstDev = Devices[dst_device];
    // First try to use D2D memcpy which is more efficient. If fails, fall back
    // to unefficient way.
    if (SrcDev.isDataExchangable(DstDev)) {
      rc = SrcDev.data_exchange(srcAddr, DstDev, dstAddr, length, nullptr);
      if (rc == OFFLOAD_SUCCESS)
        return OFFLOAD_SUCCESS;
    }

    void *buffer = malloc(length);
    rc = SrcDev.retrieveData(buffer, srcAddr, length, nullptr);
    if (rc == OFFLOAD_SUCCESS)
      rc = DstDev.submitData(dstAddr, buffer, length, nullptr);
    free(buffer);
  }

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
    DP("Call to omp_target_memcpy_rect with invalid arguments\n");
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
    DP("Call to omp_target_associate_ptr with invalid arguments\n");
    return OFFLOAD_FAIL;
  }

  if (device_num == omp_get_initial_device()) {
    DP("omp_target_associate_ptr: no association possible on the host\n");
    return OFFLOAD_FAIL;
  }

  if (!device_is_ready(device_num)) {
    DP("omp_target_associate_ptr returns OFFLOAD_FAIL\n");
    return OFFLOAD_FAIL;
  }

  DeviceTy& Device = Devices[device_num];
  void *device_addr = (void *)((uint64_t)device_ptr + (uint64_t)device_offset);
  // OpenMP 5.0 sec. 3.6.6 p. 404 L32:
  // "The target-data-associate event occurs when a thread associates data on a
  // target device."
  // OpenMP 5.0 sec. 4.5.2.25 p. 489 L26-27:
  // "A registered ompt_callback_target_data_op callback is dispatched when
  // device memory is allocated or freed, as well as when data is copied to or
  // from a device."
  OMPT_DISPATCH_CALLBACK_TARGET_DATA_OP(omp_target_associate_ptr, associate,
                                        host_ptr, HOST_DEVICE, device_addr,
                                        device_num, size);
  int rc = Device.associatePtr(host_ptr, device_addr, size);
  DP("omp_target_associate_ptr returns %d\n", rc);
  return rc;
}

EXTERN int omp_target_disassociate_ptr(void *host_ptr, int device_num) {
  DP("Call to omp_target_disassociate_ptr with host_ptr " DPxMOD ", "
      "device_num %d\n", DPxPTR(host_ptr), device_num);

  if (!host_ptr) {
    DP("Call to omp_target_associate_ptr with invalid host_ptr\n");
    return OFFLOAD_FAIL;
  }

  if (device_num == omp_get_initial_device()) {
    DP("omp_target_disassociate_ptr: no association possible on the host\n");
    return OFFLOAD_FAIL;
  }

  if (!device_is_ready(device_num)) {
    DP("omp_target_disassociate_ptr returns OFFLOAD_FAIL\n");
    return OFFLOAD_FAIL;
  }

  DeviceTy& Device = Devices[device_num];
  void *TgtPtrBegin;
  int64_t Size;
  int rc = Device.disassociatePtr(host_ptr, TgtPtrBegin, Size);
  // OpenMP 5.0 sec. 3.6.7 p. 405 L27:
  // "The target-data-disassociate event occurs when a thread disassociates data
  // on a target device."
  // OpenMP 5.0 sec. 4.5.2.25 p. 489 L26-27:
  // "A registered ompt_callback_target_data_op callback is dispatched when
  // device memory is allocated or freed, as well as when data is copied to
  // or from a device."
  OMPT_DISPATCH_CALLBACK_TARGET_DATA_OP(omp_target_disassociate_ptr,
                                        disassociate, host_ptr, HOST_DEVICE,
                                        TgtPtrBegin, device_num, Size);
  DP("omp_target_disassociate_ptr returns %d\n", rc);
  return rc;
}

EXTERN void *omp_target_map_to(void *host_ptr, size_t size, int device_num) {
  int64_t tgt_map_type = OMP_TGT_MAPTYPE_TO;
  int64_t s = size;
  OMPT_SET_DIRECTIVE_INFO(ompt_directive_omp_target_map_to);
  __tgt_target_data_begin(device_num, 1, &host_ptr, &host_ptr, &s,
                          &tgt_map_type);
  OMPT_CLEAR_DIRECTIVE_INFO();
  return omp_get_mapped_ptr(host_ptr, device_num);
}

EXTERN void *omp_target_map_alloc(void *host_ptr, size_t size, int device_num) {
  int64_t tgt_map_type = OMP_TGT_MAPTYPE_NONE;
  int64_t s = size;
  OMPT_SET_DIRECTIVE_INFO(ompt_directive_omp_target_map_alloc);
  __tgt_target_data_begin(device_num, 1, &host_ptr, &host_ptr, &s,
                          &tgt_map_type);
  OMPT_CLEAR_DIRECTIVE_INFO();
  return omp_get_mapped_ptr(host_ptr, device_num);
}

EXTERN void omp_target_map_from(void *host_ptr, size_t size, int device_num) {
  int64_t tgt_map_type = OMP_TGT_MAPTYPE_FROM;
  int64_t s = size;
  OMPT_SET_DIRECTIVE_INFO(ompt_directive_omp_target_map_from);
  __tgt_target_data_end(device_num, 1, &host_ptr, &host_ptr, &s, &tgt_map_type);
  OMPT_CLEAR_DIRECTIVE_INFO();
}

EXTERN void omp_target_map_from_delete(void *host_ptr, size_t size,
                                       int device_num) {
  int64_t tgt_map_type = OMP_TGT_MAPTYPE_FROM | OMP_TGT_MAPTYPE_DELETE;
  int64_t s = size;
  OMPT_SET_DIRECTIVE_INFO(ompt_directive_omp_target_map_from_delete);
  __tgt_target_data_end(device_num, 1, &host_ptr, &host_ptr, &s, &tgt_map_type);
  OMPT_CLEAR_DIRECTIVE_INFO();
}

EXTERN void omp_target_map_release(void *host_ptr, size_t size,
                                   int device_num) {
  int64_t tgt_map_type = OMP_TGT_MAPTYPE_NONE;
  int64_t s = size;
  OMPT_SET_DIRECTIVE_INFO(ompt_directive_omp_target_map_release);
  __tgt_target_data_end(device_num, 1, &host_ptr, &host_ptr, &s, &tgt_map_type);
  OMPT_CLEAR_DIRECTIVE_INFO();
}

EXTERN void omp_target_map_delete(void *host_ptr, size_t size, int device_num) {
  int64_t tgt_map_type = OMP_TGT_MAPTYPE_DELETE;
  int64_t s = size;
  OMPT_SET_DIRECTIVE_INFO(ompt_directive_omp_target_map_delete);
  __tgt_target_data_end(device_num, 1, &host_ptr, &host_ptr, &s, &tgt_map_type);
  OMPT_CLEAR_DIRECTIVE_INFO();
}
