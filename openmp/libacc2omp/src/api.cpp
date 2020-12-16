/*
 * api.cpp -- OpenACC Runtime Library API implementation.
 *
 * This file generally tries to depend on the implementation of OpenMP routines
 * where the desired OpenACC behavior is clearly specified by the OpenMP
 * specification or where we use original OpenMP extensions so that we specify
 * the OpenMP behavior.  Otherwise, this implementation tries to implement the
 * desired OpenACC behavior in this file to avoid such a dependence.
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
 * Data and memory management routines.
 *
 * Correct handling of null pointers is often not clear in OpenACC 3.1.  We
 * attempt to follow a concept discussed by the OpenACC technical committee: a
 * null pointer is considered always present.  Specifically, we assume the
 * behavior is as if there is an "acc data" enclosing the entire program that
 * maps a host null pointer to a device null pointer so that the structured
 * reference count is always non-zero.  We document each routine's specific
 * handling of null pointers in more detail below and relate it to this concept
 * where applicable.
 *
 * TODO: We assume !omp_get_num_devices() is a good test for disabled
 * offloading.  Is there a better check?
 ****************************************************************************/

void *acc_malloc(size_t bytes) {
  // Handling of bytes=0:
  // - Return a null pointer and do nothing.
  // - This behavior appears to mimic nvc 20.9-0.
  // - OpenACC 3.1 and OpenMP 5.1 are unclear about the behavior in this case.
  if (!bytes)
    return NULL;
  // If offloading is disabled (thus shared memory):
  // - Allocate on the host.
  // - This behavior appears to mimic nvc 20.9-0 and gcc 10.2.0.
  // - OpenACC 3.1 is unclear about the behavior in this case.
  return omp_target_alloc(bytes, omp_get_num_devices()
                                     ? omp_get_default_device()
                                     : omp_get_initial_device());
}

void acc_free(void *data_dev) {
  // If offloading is disabled (thus shared memory):
  // - Free from the host, for compatibility with acc_malloc.
  // - This behavior appears to mimic nvc 20.9-0 and gcc 10.2.0.
  // - OpenACC 3.1 is unclear about the behavior in this case.
  //
  // Handling of null pointer:
  // - OpenACC 3.1, sec. 3.2.25 "acc_free", L3444-3445:
  //   "If the argument is a NULL pointer, no operation is performed."
  // - OpenMP 5.1, sec. 3.8.2 "omp_target_free", p. 415, L3:
  //   "If device_ptr is NULL (or C_NULL_PTR, for Fortran), the operation is
  //   ignored."
  omp_target_free(data_dev, omp_get_num_devices() ? omp_get_default_device()
                                                  : omp_get_initial_device());
}

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

  // If offloading is disabled (thus shared memory):
  // - Do nothing.
  // - This behavior appears to mimic nvc 20.9-0: acc_map_data doesn't affect
  //   the result of acc_is_present, acc_deviceptr, or acc_hostptr, all of which
  //   indicate the host pointer is mapped to itself both before and after
  //   acc_map_data.
  // - However, gcc-10 has "libgomp: cannot map data on shared-memory system" at
  //   run time.
  // - OpenACC 3.1 is unclear about the behavior in this case.
  if (!omp_get_num_devices())
    return;

  // When any byte of the specified host memory has already been mapped, issue a
  // runtime error.  Because omp_target_associate_ptr does not always do so,
  // check for this case using the OpenMP extension omp_target_range_is_present.
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
  // - Host memory has the same starting address but extends after (bytes is
  //   larger) than host memory that's already mapped:
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
  int Dev = omp_get_default_device();
  if (omp_present_none != omp_target_range_is_present(data_arg, bytes, Dev))
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
  if (!omp_target_associate_ptr(data_arg, data_dev, bytes, 0, Dev))
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
  // If offloading is disabled (thus shared memory):
  // - Do nothing, for consistency with acc_map_data.
  // - This behavior appears to mimic nvc 20.9-0: acc_unmap_data doesn't affect
  //   the result of acc_is_present, acc_deviceptr, or acc_hostptr, all of which
  //   indicate the host pointer is mapped to itself both before and after
  //   acc_map_data.
  // - OpenACC 3.1 is unclear about the behavior in this case.
  if (!omp_get_num_devices())
    return;
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
  if (!omp_target_disassociate_ptr(data_arg, omp_get_default_device()))
    return;
  acc2omp_fatal(ACC2OMP_MSG(unmap_data_fail));
}

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
  // If offloading is disabled (thus shared memory):
  // - Return data_arg, for consistency with acc_is_present, which always
  //   returns true in this case.
  // - This behavior appears to mimic nvc 20.9-0.
  // - OpenACC 3.1 is unclear about the behavior in this case.
  if (!omp_get_num_devices())
    return data_arg;
  // OpenACC 3.1, sec. 3.2.34 "acc_deviceptr", L3665-3667:
  // "The acc_deviceptr routine returns the device pointer associated with a
  // host address.  data_arg is the address of a host variable or array that has
  // an active lifetime on the current device.  If the data is not present in
  // the current device memory, the routine returns a NULL value."
  return omp_get_mapped_ptr(data_arg, omp_get_default_device());
}

int acc_is_present(void *data_arg, size_t bytes) {
  // Handling of null pointer:
  // - Return true.
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
  // If offloading is disabled (thus shared memory):
  // - Return true.
  // - OpenACC 3.1, sec. 3.2.36 "acc_is_present", L3696-3697:
  //   "The routine returns .true. if the specified data is in shared memory or
  //   is fully present, and .false. otherwise."
  if (!omp_get_num_devices())
    return true;
  // Handling of bytes=0:
  // - OpenACC 3.1, sec. 3.2.36 "acc_is_present", L3697-3699:
  //   "If the byte length is zero, the routine returns nonzero in C/C++ or
  //   .true. in Fortran if the given address is in shared memory or is present
  //   at all in the current device memory."
  // - OpenMP 5.1, sec. 3.8.4 "omp_target_is_accessible", p. 417, L21-22:
  //   "This routine returns true if the storage of size bytes starting at the
  //   address given by ptr is accessible from device device_num.  Otherwise, it
  //   returns false."
  // - Follow OpenACC 3.1.  We assume OpenMP 5.1 implementations should have the
  //   same behavior, but the spec is unclear.
  return omp_present_full ==
         omp_target_range_is_present(data_arg, bytes, omp_get_default_device());
}
