/*
 * api.cpp -- OpenACC Runtime Library API implementation.
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
 * One of our goals is to be able to wrap any OpenMP runtime implementing OMPT.
 * For that reason, it's expected that anything included here doesn't actually
 * require linking with LLVM's OpenMP runtime.  Only definitions in the header
 * are needed.  If you do need to link something from the OpenMP runtime, see
 * acc2omp-backend.h.
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
 * TODO: We assume !omp_get_num_devices() is a good test for disabled
 * offloading.  Is there a better check?
 ****************************************************************************/

void *acc_malloc(size_t bytes) {
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
  // Handling of null pointer:
  // - This produces an error if either pointer is NULL, but it is not clear
  //   from either spec whether that's correct.
  // - nvc 20.9-0 appears to implement this case as a no-op.
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
  // If offloading is disabled (thus shared memory):
  // - Do nothing, for consistency with acc_map_data.
  // - This behavior appears to mimic nvc 20.9-0: acc_unmap_data doesn't affect
  //   the result of acc_is_present, acc_deviceptr, or acc_hostptr, all of which
  //   indicate the host pointer is mapped to itself both before and after
  //   acc_map_data.
  // - OpenACC 3.1 is unclear about the behavior in this case.
  if (!omp_get_num_devices())
    return;
  // Pointer must have been mapped using acc_map_data and must not be NULL:
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
  //   OpenMP runtime produces an error.  For now, to be careful, we just let
  //   the runtime error happen.
  // - For any non-null pointer that wasn't mapped, LLVM's OpenMP runtime
  //   produces a runtime error, so we let that happen too even though nvc
  //   20.9-0 appears to implement this case as a no-op too.
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
  // If offloading is disabled (thus shared memory):
  // - Return data_arg, for consistency with acc_is_present, which always
  //   returns true in this case.
  // - This behavior appears to mimic nvc 20.9-0.
  // - OpenACC 3.1 is unclear about the behavior in this case.
  //
  // Handling of null pointer:
  // - Return a null pointer.
  // - This behavior appears to mimic nvc 20.9-0.
  // - OpenACC 3.1 is unclear about the behavior in this case.
  if (!omp_get_num_devices() || !data_arg)
    return data_arg;
  // OpenACC 3.1, sec. 3.2.34 "acc_deviceptr", L3665-3667:
  // "The acc_deviceptr routine returns the device pointer associated with a
  // host address.  data_arg is the address of a host variable or array that has
  // an active lifetime on the current device.  If the data is not present in
  // the current device memory, the routine returns a NULL value."
  return omp_get_mapped_ptr(data_arg, omp_get_default_device());
}

int acc_is_present(void *data_arg, size_t bytes) {
  // OpenACC 3.1, sec. 3.2.36 "acc_is_present", L3696-3697:
  // "The routine returns .true. if the specified data is in shared memory or
  // is fully present, and .false. otherwise."
  //
  // If offloading is disabled, that means shared memory, so it's present.
  //
  // Handling of null pointer:
  // - Return true.
  // - This behavior appears to mimic nvc 20.9-0 except that, strangely, it
  //   returns false if the OpenACC runtime hasn't started yet.
  // - OpenACC 3.1 is unclear about the behavior in this case, but this behavior
  //   appears to match recent discussions in the OpenACC technical committee.
  if (!omp_get_num_devices() || !data_arg)
    return true;
  return omp_present_full ==
         omp_target_range_is_present(data_arg, bytes, omp_get_default_device());
}
