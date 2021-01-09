/*
 * api.cpp -- OpenACC Runtime Library API implementation.
 *
 * This file generally tries to depend on the implementation of OpenMP routines
 * where the desired OpenACC behavior is clearly specified by the OpenMP
 * specification or where we use original OpenMP extensions that we believe
 * should support the desired behavior.  Otherwise, this implementation tries to
 * implement the desired OpenACC behavior in this file to avoid such a
 * dependence.
 */

//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

/*****************************************************************************
 * System include files.
 ****************************************************************************/

// Don't include headers like assert.h, stdio.h, or iostream.  Instead, call
// functions declared in acc2omp-backend.h.
//#include <cstring>

/*****************************************************************************
 * OpenMP standard include files.
 ****************************************************************************/

#include "omp.h"

/*****************************************************************************
 * LLVM OpenMP include files.
 *
 * One of our goals is to be able to wrap any OpenMP runtime.  For that reason,
 * it's expected that anything included here doesn't actually require linking
 * with LLVM's OpenMP runtime.  Only definitions in the header are needed.  If
 * you do need to link something from the OpenMP runtime, see acc2omp-backend.h.
 ****************************************************************************/

// FIXME: Will this be needed?
// - KMP_FALLTHROUGH
//#include "kmp_os.h"

/*****************************************************************************
 * OpenACC standard include files.
 ****************************************************************************/

#include "openacc.h"

/*****************************************************************************
 * Internal includes.
 ****************************************************************************/

#include "internal.h"

/*****************************************************************************
 * Internal helper routines.
 ****************************************************************************/

static inline int getCurrentDevice() {
  // TODO: We assume !omp_get_num_devices() is a good test for disabled
  // offloading.  Is there a better check?
  return omp_get_num_devices() ? omp_get_default_device()
                               : omp_get_initial_device();
}

enum Presence {
  PresenceNone,
  PresencePartiallyMapped,
  PresenceFullyMapped,
  PresenceSharedMemory
};

static inline Presence checkPresence(void *HostPtr, size_t Bytes) {
  void *BufferHost;
  void *BufferDevice;
  size_t BufferSize = omp_get_accessible_buffer(
      HostPtr, Bytes, getCurrentDevice(), &BufferHost, &BufferDevice);
  if (!BufferSize)
    return PresenceNone;
  if (BufferSize == SIZE_MAX)
    return PresenceSharedMemory;
  if (BufferHost <= HostPtr &&
      (char *)HostPtr + Bytes <= (char *)BufferHost + BufferSize)
    return PresenceFullyMapped;
  return PresencePartiallyMapped;
}

/*****************************************************************************
 * Data and memory management routines.
 *
 * Handling of null pointers is often not clearly specified in OpenACC 3.1.  We
 * attempt to follow a concept discussed by the OpenACC technical committee: a
 * null pointer is considered always present.  Specifically, we assume the
 * behavior is as if there is an "acc data" enclosing the entire program that
 * maps a host null pointer to a device null pointer so that the structured
 * reference count is always non-zero.  Because a null pointer doesn't actually
 * point to data, we also assume that the number of bytes specified to a routine
 * is ignored and that any update requested for a null pointer does nothing.
 * However, none of this applies to the lower-level routines acc_map_data,
 * acc_unmap_data, and acc_memcpy_*.  We document each routine's specific
 * handling of null pointers in more detail below and relate it to this concept
 * where applicable.
 ****************************************************************************/

/*----------------------------------------------------------------------------
 * Allocation routines.
 *--------------------------------------------------------------------------*/

void *acc_malloc(size_t bytes) {
  // Handling of bytes=0:
  // - Return a null pointer and do nothing.
  // - This behavior appears to mimic nvc 20.9-0.
  // - OpenACC 3.1 and OpenMP 5.1 are unclear about the behavior in this case.
  if (!bytes)
    return NULL;
  // Handling of shared memory (including host as current device):
  // - Regardless, allocate on the current device.
  // - nvc 20.9-0's acc_malloc returns a non-null pointer.
  // - LLVM OpenMP runtime's omp_target_alloc allocates on the specified device.
  // - OpenACC 3.1, sec. 3.2.24 "acc_alloc", L3428:
  //   "The acc_malloc routine allocates space in the current device memory."
  // - OpenMP 5.1, sec. 3.8.1 "omp_target_alloc", p. 413 L3-4:
  //   "The storage location is dynamically allocated in the device data
  //   environment of the device specified by device_num."
  // - Neither spec seems to specify an exception for shared memory.
  return omp_target_alloc(bytes, getCurrentDevice());
}

void acc_free(void *data_dev) {
  // Handling of shared memory (including host as current device):
  // - Regardless, free from the current device, for consistency with
  //   acc_malloc.
  // - OpenACC 3.1, sec. 3.2.25 "acc_free", L3443-3444:
  //   "The acc_free routine will free previously allocated space in the current
  //   device memory; data_dev should be a pointer value that was returned by a
  //   call to acc_malloc."
  // - OpenMP 5.1, sec. 3.8.2 "omp_target_free", p. 414 L12-13:
  //   "The omp_target_free routine frees the device memory allocated by the
  //   omp_target_alloc routine."
  // Handling of null pointer:
  // - OpenACC 3.1, sec. 3.2.25 "acc_free", L3444-3445:
  //   "If the argument is a NULL pointer, no operation is performed."
  // - OpenMP 5.1, sec. 3.8.2 "omp_target_free", p. 415, L3:
  //   "If device_ptr is NULL (or C_NULL_PTR, for Fortran), the operation is
  //   ignored."
  omp_target_free(data_dev, getCurrentDevice());
}

/*----------------------------------------------------------------------------
 * Routines similar to enter data, exit data, and update directives.
 *
 * Handling of null pointer or bytes=0:
 * - Other than returning a null pointer where the return type is void*, do
 *   nothing.  In the case of a null pointer, ignore bytes.
 * - This behavior appears to mimic nvc 20.9-0 except that nvc doesn't yet
 *   implement acc_copyout_finalize or acc_delete_finalize.
 * - OpenACC 3.1 is unclear about the behavior in this case.
 * - The OpenACC technical committee is considering adding "or is a null
 *   pointer" to the shared-memory condition in each of the quotes below.
 * - For a null pointer, this behavior seems to follow the OpenACC technical
 *   commitee's idea that a null pointer is considered always present,
 *   especially if we assume it was automatically mapped to a null pointer with
 *   a non-zero structured reference count (so that reference count incs/decs
 *   have no effect), and if we assume bytes and updates should be ignored
 *   because a null pointer doesn't actually point to data.
 * - For bytes=0:
 *   - Potential use cases:
 *     - bytes=0 for data of size > 0:
 *       - This is part of a more general use case of specifying bytes<N to
 *         operate on data of size N>0 when it is not convenient to determine
 *         the exact value of N at the routine's call site.  That is, this use
 *         case relies on N already stored in the present table.
 *       - The current behavior of enter/exit-data-like routines does not
 *         support this use case for bytes=0 though it does for bytes>0: it does
 *         not adjust dynamic reference counts and perform any consequent
 *         deletions or data transfers.  It does nothing.
 *       - This use case does not include update routines, which always operate
 *         on exactly the specified bytes not on N bytes.
 *     - bytes=0 for data of size 0:
 *       - That is, imagine an application that maps N>=0 bytes to the device
 *         and then calls enter/exit-data-like routines and update routines for
 *         a subset, M<=N.  Fulfilling their usual role, the
 *         enter/exit-data-like routines guarantee the M bytes are present for
 *         the desired lifetime, and the update routines transfer the M bytes.
 *         Imagine we want the application to gracefully handle (not fail for)
 *         the cases of M=0 and N=0.
 *       - To support the M=0 but N>0 case, it is sufficient for these routines
 *         to do nothing when bytes=0: the specified address is present but
 *         operations on the subset M become no-ops.
 *       - To support the M=N=0 case, doing nothing for bytes=0 is sufficient if
 *         other functionality that checks presence, such acc_is_present, is not
 *         used.  That is, presence checks currently return false when N=0 but
 *         not when N>0.  Whether that's desirable is likely
 *         application-dependent.
 *   - Alternative behaviors:
 *     - Modify bytes=0 behavior for consistency with bytes>0 behavior to
 *       support the above use cases and so that these routines agree with
 *       acc_is_present about when a (data_arg, 0) pair is considered present:
 *       - Modify enter-data-like routines when address is not already present:
 *         create a present table entry for the address with size 0.  A unique
 *         device address should also be mapped for the sake of acc_deviceptr
 *         and acc_hostptr.
 *       - Modify enter/exit-data-like routines when address is already present:
 *         adjust the dynamic reference count and perform any consequent
 *         deletions or data transfers.
 *       - Modify update routines: check for presence even though bytes=0.  This
 *         seems fine if present table entries can have size 0.  However, the
 *         check could be viewed as redundant given that no bytes will actually
 *         by transferred.
 *       - If these changes are worthwhile, it might worthwhile to consider
 *         similar behaviors for acc_malloc, acc_map_data, and acc_unmap_data.
 *     - Produce runtime errors:
 *       - The current bytes=0 behavior may be confusing due to inconsistencies
 *         with the bytes>0 behavior.
 *       - The previous alternative behavior to avoid that confusion has subtle
 *         differences in reference counting from nvc's current behavior and
 *         could thus lead to memory errors that are difficult to debug in
 *         existing applications.  Moreover, acc_is_present, for example, would
 *         no longer have identical behavior for bytes=0 and bytes=1.
 *       - Always producing a runtime error for bytes=0 would avoid both these
 *         problems.  It might be the best option if the above use cases are
 *         not worthwhile to support.
 *   - These alternative behaviors need to be discussed with the OpenACC
 *     technical committee.
 * Handling of shared memory (including host as current device):
 * - Other than returning data_arg where the return type is void*, do nothing.
 * - This behavior appears to mimic nvc 20.9-0.
 * - OpenACC 3.1, sec. 3.2.26 "acc_copyin", L3470-3471:
 *   "If the data is in shared memory, no action is taken.  The C/C++ acc_copyin
 *   routine returns the incoming pointer."
 * - OpenACC 3.1, sec. 3.2.27 "acc_create", L3505-3506:
 *   "If the data is in shared memory, no action is taken.  The C/C++ acc_create
 *   routine returns the incoming pointer."
 * - OpenACC 3.1, sec. 3.2.28 "acc_copyout", L3537:
 *   "If the data is in shared memory, no action is taken."
 * - OpenACC 3.1, sec. 3.2.29 "acc_delete", L3565:
 *   "If the data is in shared memory, no action is taken."
 * - OpenACC 3.1, sec. 3.2.30 "acc_update_device", L3591-3593:
 *   "For the data referred to by data_arg, if data is not in shared memory, the
 *   data in the local memory is copied to the corresponding device memory."
 * - OpenACC 3.1, sec. 3.2.31 "acc_update_self", L3616-3618:
 *   "For the data referred to by data_arg, if the data is not in shared memory,
 *   the data in the local memory is copied to the corresponding device memory.
 * - LLVM's OpenMP implementation currently produces a runtime error if
 *   omp_set_default_device(omp_get_initial_device()) is called before a target
 *   enter/exit data or target update directive, reporting that the device is
 *   not ready.  Thus, for now, our
 *   omp_target_map_(to|alloc|from|from_delete|release|delete) and
 *   omp_target_update_(to|from) implementations do the same for device_num =
 *   omp_get_initial_device().  Otherwise, it would handle the case of shared
 *   memory as desired here.
 *--------------------------------------------------------------------------*/

void *acc_copyin(void *data_arg, size_t bytes) {
  if (!data_arg || !bytes)
    return NULL;
  if (PresenceSharedMemory == checkPresence(data_arg, bytes))
    return data_arg;
  // OpenACC 3.1, sec. 3.2.26 "acc_copyin", L3462:
  // "The acc_copyin routines are equivalent to an enter data directive with a
  // copyin clause."
  return omp_target_map_to(data_arg, bytes, getCurrentDevice());
}

void *acc_present_or_copyin(void *data_arg, size_t bytes) {
  // OpenACC 3.1, sec. 3.2.26 "acc_copyin", L3484-3485:
  // "For compatibility with OpenACC 2.0, acc_present_or_copyin and acc_pcopyin
  // are alternate names for acc_copyin."
  return acc_copyin(data_arg, bytes);
}

void *acc_pcopyin(void *data_arg, size_t bytes) {
  // OpenACC 3.1, sec. 3.2.26 "acc_copyin", L3484-3485:
  // "For compatibility with OpenACC 2.0, acc_present_or_copyin and acc_pcopyin
  // are alternate names for acc_copyin."
  return acc_copyin(data_arg, bytes);
}

void *acc_create(void *data_arg, size_t bytes) {
  if (!data_arg || !bytes)
    return NULL;
  if (PresenceSharedMemory == checkPresence(data_arg, bytes))
    return data_arg;
  // OpenACC 3.1, sec. 3.2.27 "acc_create", L3502:
  // "The acc_create routines are equivalent to an enter data directive with a
  // create clause."
  return omp_target_map_alloc(data_arg, bytes, getCurrentDevice());
}

void *acc_present_or_create(void *data_arg, size_t bytes) {
  // OpenACC 3.1, sec. 3.2.27 "acc_create", L3518-3519:
  // "For compatibility with OpenACC 2.0, acc_present_or_create and acc_pcreate
  // are alternate names for acc_create."
  return acc_create(data_arg, bytes);
}

void *acc_pcreate(void *data_arg, size_t bytes) {
  // OpenACC 3.1, sec. 3.2.27 "acc_create", L3518-3519:
  // "For compatibility with OpenACC 2.0, acc_present_or_create and acc_pcreate
  // are alternate names for acc_create."
  return acc_create(data_arg, bytes);
}

void acc_copyout(void *data_arg, size_t bytes) {
  if (!data_arg || !bytes)
    return;
  if (PresenceSharedMemory == checkPresence(data_arg, bytes))
    return;
  // OpenACC 3.1, sec. 3.2.28 "acc_copyout", L3532:
  // "The acc_copyout routines are equivalent to an exit data directive with a
  // copyout clause."
  omp_target_map_from(data_arg, bytes, getCurrentDevice());
}

void acc_copyout_finalize(void *data_arg, size_t bytes) {
  // nvc 20.9-0 does not implement acc_copyout_finalize.
  if (!data_arg || !bytes)
    return;
  if (PresenceSharedMemory == checkPresence(data_arg, bytes))
    return;
  // OpenACC 3.1, sec. 3.2.28 "acc_copyout", L3533-3534:
  // "The acc_copyout_finalize routines are equivalent to an exit data directive
  // with both copyout and finalize clauses."
  omp_target_map_from_delete(data_arg, bytes, getCurrentDevice());
}

void acc_delete(void *data_arg, size_t bytes) {
  if (!data_arg || !bytes)
    return;
  if (PresenceSharedMemory == checkPresence(data_arg, bytes))
    return;
  // OpenACC 3.1, sec. 3.2.29 "acc_delete", L3560:
  // "The acc_delete routines are equivalent to an exit data directive with a
  // delete clause."
  omp_target_map_release(data_arg, bytes, getCurrentDevice());
}

void acc_delete_finalize(void *data_arg, size_t bytes) {
  // nvc 20.9-0 does not implement acc_delete_finalize.
  if (!data_arg || !bytes)
    return;
  if (PresenceSharedMemory == checkPresence(data_arg, bytes))
    return;
  // OpenACC 3.1, sec. 3.2.29 "acc_delete", L3561-3562:
  // "The acc_delete_finalize routines are equivalent to an exit data directive
  // with both delete clause and finalize clauses."
  omp_target_map_delete(data_arg, bytes, getCurrentDevice());
}

void acc_update_device(void *data_arg, size_t bytes) {
  if (!data_arg || !bytes)
    return;
  if (PresenceSharedMemory == checkPresence(data_arg, bytes))
    return;
  // OpenACC 3.1, sec. 3.2.30 "acc_update_device", L3590:
  // "The acc_update_device routine is equivalent to an update directive with a
  // device clause."
  omp_target_update_to(data_arg, bytes, getCurrentDevice());
}

void acc_update_self(void *data_arg, size_t bytes) {
  if (!data_arg || !bytes)
    return;
  if (PresenceSharedMemory == checkPresence(data_arg, bytes))
    return;
  // OpenACC 3.1, sec. 3.2.31 "acc_update_self", L3615:
  // "The acc_update_self routine is equivalent to an update directive with a
  // self clause,"
  omp_target_update_from(data_arg, bytes, getCurrentDevice());
}

/*----------------------------------------------------------------------------
 * Mapping routines.
 *--------------------------------------------------------------------------*/

void acc_map_data(void *data_arg, void *data_dev, size_t bytes) {
  // Handling of null pointer or bytes=0:
  // - Produce a runtime error.
  // - nvc 20.9-0 appears to implement these cases as no-ops.
  // - OpenACC 3.1 and OpenMP 5.1 are unclear about the behavior in these cases.
  // - For a null pointer, this behavior seems to follow the OpenACC technical
  //   committee's idea that a null pointer is considered always present,
  //   especially if we assume it was automatically mapped to a null pointer.
  //   That is, an acc_map_data with a host null pointer is then an error, and
  //   an acc_map_data with a device null pointer is undefined, according to
  //   OpenACC 3.1, sec. 3.2.32 "acc_map_data", L3637-3639: "It is an error to
  //   call acc_map_data for host data that is already present in the current
  //   device memory.  It is undefined to call acc_map_data with a device
  //   address that is already mapped to host data."
  if (!data_arg)
    acc2omp_fatal(ACC2OMP_MSG(map_data_host_pointer_null));
  if (!data_dev)
    acc2omp_fatal(ACC2OMP_MSG(map_data_device_pointer_null));
  if (!bytes)
    acc2omp_fatal(ACC2OMP_MSG(map_data_bytes_zero));

  // When any byte of the specified host memory has already been mapped or is in
  // shared memory, issue a runtime error.  Because omp_target_associate_ptr
  // does not always do so, check for this case using the OpenMP extension
  // omp_get_accessible_buffer.
  //
  // The relevant spec passages are:
  //
  // - OpenACC 3.1, sec. 3.2.32 "acc_map_data", L3637-3638:
  //   "It is an error to call acc_map_data for host data that is already
  //   present in the current device memory."
  // - OpenMP 5.1, sec. 3.8.9 "omp_target_associate_ptr", p. 428, L1-3:
  //   "Only one device buffer can be associated with a given host pointer value
  //   and device number pair.  Attempting to associate a second buffer will
  //   return non-zero.  Associating the same pair of pointers on the same
  //   device with the same offset has no effect and returns zero."
  //
  // There are many cases to consider:
  //
  // - Same host memory is already mapped to same device memory (thus, all
  //   arguments are the same):
  //   - acc_map_data: Error because the same "host data" is "already present".
  //     nvc 20.9-0 has the error.
  //   - omp_target_associate_ptr: No error because it's "Associating the same
  //     pair of pointers on the same device".  LLVM's OpenMP runtime has no
  //     error.
  // - Same host memory is already mapped but not to same device memory:
  //   - acc_map_data: Error because the same "host data" is "already present".
  //     nvc 20.9-0 has the error.
  //   - omp_target_associate_ptr: Error because it's an attempt "to associate a
  //     second buffer".  LLVM's OpenMP runtime has the error.
  // - Host memory has the same starting address as (or starts within) but
  //   extends after (bytes is larger than) host memory that's already mapped:
  //   - acc_map_data: It's not clear if "is already present" means "fully
  //     present" or "at least partially present".  nvc 20.9-0 has the error,
  //     and its diagnostic says "partially present", so the latter
  //     interpretation seems correct.
  //   - omp_target_associate_ptr: Error because the device memory ranges would
  //     surely be considered different "buffers" because their sizes are
  //     different.  LLVM's OpenMP runtime has the error.
  // - Host memory overlaps with and extends before (different starting address)
  //   host memory that's already mapped:
  //   - acc_map_data: The question is still whether "is already present" means
  //     "at least partially present".  nvc 20.9-0 seems to make it an error
  //     only if the end address is the same, but that distinction is surely
  //     just an unfortunate implementation shortcut.
  //   - omp_target_associate_ptr: Does "given host pointer value" refer only to
  //     the specified host start address or to any address within the specified
  //     host memory?  LLVM's OpenMP runtime does not make this an error.
  // - Shared memory (including host as current device):
  //   - It is not clear if either spec covers this case.  However, based on the
  //     above quotes, we might assume acc_map_data should see the memory as
  //     already present and thus produce an error, and we might assume OpenMP
  //     would produce an error only if you don't map it to itself.
  //   - Under nvc 20.9-0, acc_map_data appears to do nothing in this case: the
  //     result of acc_is_present, acc_deviceptr, and acc_hostptr aren't
  //     affected, all of which indicate the host pointer is mapped to itself
  //     both before and after acc_map_data.  However, calling acc_map_data
  //     again for the same host pointer produces a runtime error that it's
  //     already mapped.  Strangely, acc_unmap_data doesn't appear to remove the
  //     entry as it doesn't prevent that error when called in between those two
  //     acc_map_data calls.
  //   - gcc-10 has "libgomp: cannot map data on shared-memory system" at run
  //     time.
  Presence P = checkPresence(data_arg, bytes);
  if (P == PresenceSharedMemory)
    acc2omp_fatal(ACC2OMP_MSG(map_data_shared_memory));
  if (P != PresenceNone)
    acc2omp_fatal(ACC2OMP_MSG(map_data_already_present));

  // Try to create the mapping.
  //
  // Effect on dynamic reference counter:
  // - OpenACC 3.1, sec. 3.2.32 "acc_map_data", L3639-3642:
  //   "After mapping the device memory, the dynamic reference count for the
  //   host data is set to one, but no data movement will occur.  Memory mapped
  //   by acc_map_data may not have the associated dynamic reference count
  //   decremented to zero, except by a call to acc_unmap_data."
  // - OpenMP 5.1, sec. 3.8.9 "omp_target_associate_ptr", p. 427, L27-28:
  //   "The reference count of the resulting mapping will be infinite."
  // - In the OpenACC passage, it's not clear what "may not have" means.  The
  //   application may not or else will be non-portable?  The implementation may
  //   not?  We assume it's the latter and thus follow OpenMP behavior.
  //   However, nvc 20.9-0 does permit exit data to reduce the reference count
  //   to zero and thus unmap the data.
  if (!omp_target_associate_ptr(data_arg, data_dev, bytes, /*device_offset=*/0,
                                getCurrentDevice()))
    return;
  acc2omp_fatal(ACC2OMP_MSG(map_data_fail));
}

void acc_unmap_data(void *data_arg) {
  // Handling of null pointer:
  // - Produce a runtime error.
  // - nvc 20.9-0 appears to implement these case as a no-op.
  // - OpenACC 3.1 and OpenMP 5.1 are unclear about the behavior in this case.
  //   For details, see the quotes and discussion below concerning whether a
  //   pointer was already mapped.
  // - For a null pointer, this behavior seems to follow the OpenACC technical
  //   committee's idea that a null pointer is considered always present,
  //   especially if we assume it was automatically mapped to a null pointer
  //   with a non-zero structured reference count.  That is, an acc_unmap_data
  //   with a null pointer is then not permitted, according to the quote about a
  //   non-zero structured reference count below.
  if (!data_arg)
    acc2omp_fatal(ACC2OMP_MSG(unmap_data_pointer_null));
  // Handling of shared memory (including host as current device):
  // - Produce a runtime error, for consistency with acc_map_data.
  // - Under nvc 20.9-0, acc_unmap_data appears to do nothing in this case: the
  //   result of acc_is_present, acc_deviceptr, and acc_hostptr aren't affected,
  //   all of which indicate the host pointer is mapped to itself both before
  //   and after acc_unmap_data.
  // - OpenACC 3.1 and OpenMP 5.1 are unclear about the behavior in this case.
  if (PresenceSharedMemory == checkPresence(data_arg, /*Bytes=*/0))
    acc2omp_fatal(ACC2OMP_MSG(unmap_data_shared_memory));
  // Pointer must have been mapped using acc_map_data:
  // - OpenACC 3.1, sec. 3.2.33 "acc_unmap_data", L3653-3655:
  //   "It is undefined behavior to call acc_unmap_data with a host address
  //   unless that host address was mapped to device memory using acc_map_data."
  // - OpenMP 5.1, sec. 3.8.10 "omp_target_disassociate_ptr", p. 429, L20-22:
  //   "A call to this routine on a pointer that is not NULL (or C_NULL_PTR, for
  //   Fortran) and does not have associated data on the given device results in
  //   unspecified behavior."
  // - Strangely, the above passages seem to say that behavior of acc_unmap_data
  //   but not omp_target_disassociate_ptr is undefined if the pointer is NULL,
  //   but nvc 20.9-0 appears to implement this case as a no-op, and LLVM's
  //   OpenMP runtime produces an error.  For now, to be careful, we report an
  //   error above.
  // - For any non-null pointer that wasn't mapped, LLVM's OpenMP runtime
  //   produces a runtime error, so we let that happen even though nvc 20.9-0
  //   appears to implement this case as a no-op too.
  //
  // Structured reference counter must be zero:
  // - OpenACC 3.1, sec 3.2.33 "acc_unmap_data", L3656-3657:
  //   "It is an error to call acc_unmap_data if the structured reference count
  //   for the pointer is not zero."
  // - We have implemented our OpenMP 'hold' map type modifier extension to
  //   mimic that behavior.
  //
  // Effect on dynamic reference counter:
  // - OpenACC 3.1, sec 3.2.33 "acc_unmap_data", L3655-3656:
  //   "After unmapping memory the dynamic reference count for the pointer is
  //   set to zero, but no data movement will occur."
  // - OpenMP 5.1, sec. 3.8.10 "omp_target_disassociate_ptr", p. 429, L22-23:
  //   "The reference count of the mapping is reduced to zero, regardless of its
  //   current value."
  if (!omp_target_disassociate_ptr(data_arg, getCurrentDevice()))
    return;
  acc2omp_fatal(ACC2OMP_MSG(unmap_data_fail));
}

/*----------------------------------------------------------------------------
 * Query routines.
 *--------------------------------------------------------------------------*/

void *acc_deviceptr(void *data_arg) {
  // Handling of null pointer:
  // - Return a null pointer.
  // - This behavior appears to mimic nvc 20.9-0.
  // - OpenACC 3.1 and OpenMP 5.1 are unclear about the behavior in this case.
  // - This behavior seems to follow the OpenACC technical committee's idea that
  //   a null pointer is considered always present, especially if we assume it
  //   was automatically mapped to a null pointer.
  if (!data_arg)
    return NULL;
  // Handling of shared memory (including host as current device):
  // - Return data_arg, for consistency with acc_is_present, which always
  //   returns true in this case.
  // - This behavior appears to mimic nvc 20.9-0.
  // - OpenACC 3.1 is unclear about the behavior in this case.
  // - OpenMP 5.1 is unclear about the behavior in this case, but "mapped" seems
  //   to imply an actual present table entry vs. just shared memory.  On the
  //   other hand, the spec is clear that, if the specified device is the
  //   initial device (host), then it returns the host pointer.
  // - TODO: If we decide that omp_get_mapped_ptr definitely isn't intended to
  //   handle shared memory as we require here, it's probably more reasonable to
  //   just call omp_get_accessible_buffer once here to get the desired pointer,
  //   thus eliminating checkPresence and omp_get_mapped_ptr calls here.  Of
  //   course, if we decide omp_get_mapped_ptr definitely is intended to do what
  //   we want, we can just eliminate this checkPresence call.
  if (PresenceSharedMemory == checkPresence(data_arg, /*Bytes=*/0))
    return data_arg;
  // OpenACC 3.1, sec. 3.2.34 "acc_deviceptr", L3665-3667:
  // "The acc_deviceptr routine returns the device pointer associated with a
  // host address.  data_arg is the address of a host variable or array that has
  // an active lifetime on the current device.  If the data is not present in
  // the current device memory, the routine returns a NULL value."
  return omp_get_mapped_ptr(data_arg, getCurrentDevice());
}

void *acc_hostptr(void *data_dev) {
  // Handling of null pointer:
  // - Return a null pointer.
  // - This behavior appears to mimic nvc 20.9-0.
  // - OpenACC 3.1, sec. 3.2.35 "acc_hostptr", L3677-3678:
  //   "If the device address is NULL, or does not correspond to any host
  //   address, the routine returns a NULL value."
  // - This behavior seems to follow the OpenACC technical committee's idea that
  //   a null pointer is considered always present, especially if we assume it
  //   was automatically mapped to a null pointer.
  // - OpenMP 5.1 is unclear about the behavior of acc_get_mapped_ptr in this
  //   case, and our extension omp_get_mapped_hostptr should be consistent with
  //   omp_get_mapped_ptr.
  if (!data_dev)
    return NULL;
  // Handling of shared memory (including host as current device):
  // - Return data_dev, for consistency with acc_deviceptr.
  // - This behavior appears to mimic nvc 20.9-0.
  // - OpenACC 3.1 is unclear about the behavior in this case.
  // - OpenMP 5.1 is unclear about the behavior of acc_get_mapped_ptr in this
  //   case, and our extension omp_get_mapped_hostptr should be consistent with
  //   omp_get_mapped_ptr.
  // - TODO: As in acc_deviceptr, we can simplify the remaining code once we
  //   decide how omp_get_mapped_hostptr should behave.
  if (PresenceSharedMemory == checkPresence(data_dev, /*Bytes=*/0))
    return data_dev;
  // OpenACC 3.1, sec. 3.2.35 "acc_hostptr", L3675-3678:
  // "The acc_hostptr routine returns the host pointer associated with a device
  // address. data_dev is the address of a device variable or array, such as
  // that returned from acc_deviceptr, acc_create or acc_copyin. If the device
  // address is NULL, or does not correspond to any host address, the routine
  // returns a NULL value."
  return omp_get_mapped_hostptr(data_dev, getCurrentDevice());
}

int acc_is_present(void *data_arg, size_t bytes) {
  // Handling of null pointer:
  // - Return true, regardless of bytes.
  // - This behavior appears to mimic nvc 20.9-0 except that, strangely, it
  //   returns false if the OpenACC runtime hasn't started yet.
  // - OpenACC 3.1 is unclear about the behavior in this case.
  // - This behavior seems to follow the OpenACC technical committee's idea that
  //   a null pointer is considered always present.
  // - OpenMP 5.1 sec. 3.8.3 "omp_target_is_present", p. 416, L13 and
  //   OpenMP 5.1 sec. 3.8.4 "omp_target_is_accessible", p. 417, L15:
  //   "The value of ptr must be a valid host pointer or NULL (or C_NULL_PTR,
  //   for Fortran)."
  // - Thus, OpenMP 5.1 explicitly permits this case but is unclear about the
  //   behavior.
  if (!data_arg)
    return true;
  // Handling of shared memory (including host as current device):
  // - Return true.
  // - OpenACC 3.1, sec. 3.2.36 "acc_is_present", L3692-3693:
  //   "The acc_is_present routine tests whether the specified host data is
  //   accessible from the current device."
  // - OpenACC 3.1, sec. 3.2.36 "acc_is_present", L3696-3697:
  //   "The routine returns .true. if the specified data is in shared memory or
  //   is fully present, and .false. otherwise."
  // - OpenMP 5.1, sec. 3.8.4 "omp_target_is_accessible", p. 417, L21-22:
  //   "This routine returns true if the storage of size bytes starting at the
  //   address given by ptr is accessible from device device_num. Otherwise, it
  //   returns false."
  // - omp_target_is_accessible seems to have been designed specifically to
  //   behave like acc_is_present where omp_target_is_present was not: not
  //   only does omp_target_is_accessible add the size_t argument, but its
  //   description also uses the term "accessible".  OpenACC spells out the
  //   intended meaning of that term for shared memory by spelling out the
  //   behavior of acc_is_present, as quoted above.  OpenMP establishes the same
  //   meaning of that term for unified shared memory in OpenMP 5.1, sec. 2.5.1
  //   "requires directive".
  // Handling of bytes=0:
  // - OpenACC 3.1, sec. 3.2.36 "acc_is_present", L3697-3699:
  //   "If the byte length is zero, the routine returns nonzero in C/C++ or
  //   .true. in Fortran if the given address is in shared memory or is present
  //   at all in the current device memory."
  // - OpenMP 5.1, sec. 3.8.4 "omp_target_is_accessible", p. 417, L21-22:
  //   "This routine returns true if the storage of size bytes starting at the
  //   address given by ptr is accessible from device device_num.  Otherwise, it
  //   returns false."
  // - The OpenMP 5.1 spec is not perfectly clear, but we assume it's intended
  //   to handle bytes=0 in the same manner as OpenACC, and that's how LLVM's
  //   OpenMP runtime currently implements it.
  return omp_target_is_accessible(data_arg, bytes, getCurrentDevice());
}
