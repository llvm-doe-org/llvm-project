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
 *
 * Don't include headers like assert.h, stdio.h, or iostream.  Instead, call
 * functions declared in acc2omp-backend.h.
 ****************************************************************************/

#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <string>

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
 * Internal helper definitions.
 ****************************************************************************/

enum Presence {
  PresenceNone,
  PresencePartiallyMapped,
  PresenceFullyMapped,
  PresenceSharedMemory
};

static inline Presence checkPresence(void *HostPtr, size_t Bytes, int Device) {
  void *BufferHost;
  void *BufferDevice;
  size_t BufferSize = omp_get_accessible_buffer(HostPtr, Bytes, Device,
                                                &BufferHost, &BufferDevice);
  if (!BufferSize)
    return PresenceNone;
  if (BufferSize == SIZE_MAX)
    return PresenceSharedMemory;
  if (BufferHost <= HostPtr &&
      (char *)HostPtr + Bytes <= (char *)BufferHost + BufferSize)
    return PresenceFullyMapped;
  return PresencePartiallyMapped;
}

#define SET_SOURCE_INFO()                                                      \
  omp_set_source_info(/*src_file=*/NULL, __func__, /*line_no=*/0,              \
                      /*end_line_no=*/0, /*func_line_no=*/0,                   \
                      /*func_end_line_no=*/0)

#define CLEAR_SOURCE_INFO() omp_clear_source_info()

static const char *deviceTypeToString(acc_device_t DevType,
                                      bool WasDefault = false) {
  if (WasDefault) {
    switch (DevType) {
#define CASE(DevTypeEnum)                                                      \
    case acc_device_##DevTypeEnum:                                             \
      return "acc_device_" #DevTypeEnum " (from acc_device_default)";
    ACC2OMP_FOREACH_DEVICE(CASE)
#undef CASE
    }
  }
  switch (DevType) {
#define CASE(DevTypeEnum)                                                      \
  case acc_device_##DevTypeEnum:                                               \
    return "acc_device_" #DevTypeEnum;
  ACC2OMP_FOREACH_DEVICE(CASE)
#undef CASE
  }
  return "<unknown acc_device_t>";
}

static const char *deviceTypeToStringEnv(acc_device_t DevType,
                                         bool WasDefault = false) {
  if (WasDefault) {
    switch (DevType) {
#define CASE(DevTypeEnum)                                                      \
    case acc_device_##DevTypeEnum:                                             \
      return #DevTypeEnum " (default device type)";
    ACC2OMP_FOREACH_DEVICE(CASE)
#undef CASE
    }
  }
  switch (DevType) {
#define CASE(DevTypeEnum)                                                      \
  case acc_device_##DevTypeEnum:                                               \
    return #DevTypeEnum;
  ACC2OMP_FOREACH_DEVICE(CASE)
#undef CASE
  }
  return "<unknown acc_device_t>";
}

/*****************************************************************************
 * Device management routines.
 *
 * Interpretation of acc_device_t enumerators:
 * - OpenACC 3.1 is unclear about the exact semantics of various acc_device_t
 *   enumerators.  Here, we generally describe the semantics we assume, which
 *   are mostly based on our experiments with nvc 20.11-0.  Within each
 *   routine's implementation below appear specific notes about the routine's
 *   handling of these semantics, including notes about cases where behavior
 *   does not match nvc in an attempt to follow our assumed semantics more
 *   consistently.
 * - Each enumerator refers to an ordered list of devices.  Some lists are
 *   mutually exclusive, but some are not.
 * - acc_device_none:
 *   - Refers to an empty list of devices.
 * - Architecture-specific enumerators
 *   - These are enumerators, such as acc_device_nvidia, that name a specific
 *     architecture.  Each refers to the list of all available non-host devices
 *     of the architecture it names.
 *   - Currently, these lists are always mutually exclusive.  It's conceivable
 *     that, in the future, more than one enumerator might logically describe a
 *     single device, but that will require some adjustments to our
 *     implementation of acc_device_default and acc_get_device_type, as noted
 *     below.
 * - acc_device_host:
 *   - Refers to a list containing one device, the host device.
 *   - The host device is logically distinct from any non-host device even if
 *     they are the same physical device and thus of the same architecture.
 *     Thus, the host device never appears in the list referred to by an
 *     architecture-specific enumerator.  For how this decision impacts
 *     acc_on_device, see the acc_on_device notes in Clang's OpenACC status
 *     document.
 * - acc_device_not_host:
 *   - Refers to the list of all non-host devices.
 *   - Ideally, the set of devices in this list is exactly the union of the sets
 *     of devices appearing in lists referred to by architecture-specific
 *     enumerators.
 *   - However, there might be times when an OpenMP runtime (including LLVM's)
 *     supports devices for which our acc_device_t doesn't yet have an
 *     architecture-specific enumerator, and then the list referred to by
 *     acc_device_not_host might have additional devices.  Hopefully that
 *     situation will always be resolved quickly, but we want to handle it
 *     gracefully until it is resolved.
 *   - Our experiments seem to show that, if all non-host devices are of the
 *     same architecture, nvc 20.11-0 handles acc_device_not_host the same as it
 *     would that architecture-specific enumerator.  So far, that agrees with
 *     our implementation.  However, we haven't found a system on which to build
 *     nvc for multiple non-host device architectures, so we're not sure how to
 *     determine if nvc perhaps implements acc_device_not_host as just the
 *     current device's architecture-specific enumerator when the current device
 *     is not host and as acc_device_none when it is.
 * - The current device type vs. the current device:
 *   - In OpenACC 3.1's model, the current device type is represented by
 *     acc-current-device-type-var (which is accessed by acc_get_device_type),
 *     and the current device is represented by the combination of
 *     acc-current-device-type-var and acc-current-device-num-var.
 *   - In this implementation, the current device type is either
 *     acc_device_host, acc_device_not_host, or an architecture-specific
 *     enumerator.  Regardless, the referenced list includes the current device.
 *   - The current device type is acc_device_not_host only when the current
 *     device is a non-host device that cannot yet be represented by an
 *     architecture-specific enumerator.
 *   - If the environment or application has not selected a current device type,
 *     the implementation selects it based on device availability and
 *     compilation options.  For example, currently,
 *     -fopenmp-targets=nvptx64-nvidia-cuda,x86_64-unknown-linux-gnu will
 *     usually cause acc_device_nvidia to be the initial current device type.
 *   - If the environment or application has not selected a current device, the
 *     implementation selects one of the current device type.
 *   - Because the lists referred to by architecture-specific enumerators are
 *     always mutually exclusive for now, after the current device has been
 *     selected, the current device type can be determined uniquely from it.  If
 *     that mutually exclusive property doesn't remain true in the future, the
 *     current device type will need to be recorded separately from the current
 *     device in order for acc_device_default and acc_get_device_type to be
 *     implemented correctly.
 * - acc_device_default:
 *   - Refers to the list of all devices of the current device type.
 *   - The best hint OpenACC 3.1 gives for acc_device_default semantics is in
 *     sec. 3.1 "Runtime Library Definitions", L2976-2978: "The value
 *     acc_device_default will never be returned by any function; its use as an
 *     argument will tell the runtime library to use the default device type for
 *     that implementation."  Unfortunately, this is vague and could be
 *     interpreted as the device type an implementation chooses when the current
 *     device type has not otherwise been set.  Deciding that acc_device_default
 *     equals the current device type better matches the behavior we have
 *     observed in our experiments with nvc.
 * - acc_device_current:
 *   - Refers to a list containing one device, the current device.
 *   - The best hint OpenACC 3.1 gives for acc_device_current semantics is in
 *     sec. 3.2.6 "acc_get_property", L3091-3093: "If dev_type has the value
 *     acc_device_current, then dev_num is ignored and the value of the property
 *     for the current device is returned."  As described below, we extend this
 *     concept of ignoring the specified dev_num to acc_set_device_num.
 *
 * TODO: We haven't implemented ACC_DEVICE_NUM and ACC_DEVICE_TYPE yet.  Under
 * nvc 20.11-0, they have interesting effects.  ACC_DEVICE_NUM seems to
 * permanently set the device number that will be chosen when calling
 * acc_set_device_type for that initial current device's type.  ACC_DEVICE_TYPE
 * seems to make it impossible to choose acc_device_host (or perhaps any other
 * non-host device type, but we don't have a system where we know how to try
 * that).
 ****************************************************************************/

int acc_get_num_devices(acc_device_t dev_type) {
  switch (dev_type) {
  case acc_device_none:
    return 0;
  case acc_device_not_host:
    return omp_get_num_devices();
  case acc_device_default:
    return acc_get_num_devices(acc_get_device_type());
  case acc_device_current:
    // It seems logical that there's always 1 current device.  However, this
    // behavior is inconsistent with nvc 20.11-0, which returns 0 instead.
    return 1;
  case acc_device_host:
  case acc_device_nvidia:
  case acc_device_x86_64:
  case acc_device_ppc64le:
    // omp_get_num_devices_of_type returns 0 if either:
    // - acc2omp_get_omp_device_t returns omp_device_none because omp_device_t
    //   has no corresponding enumerator, and so our implementation logically
    //   has no devices known to be of type dev_type.
    // - There are no devices of the type dev_type as identified by the
    //   corresponding omp_device_t enumerator.
    return omp_get_num_devices_of_type(acc2omp_get_omp_device_t(dev_type));
  }
  acc2omp_fatal(ACC2OMP_MSG(get_num_devices_invalid_type), dev_type);
  return -1;
}

enum DeviceNumAndTypeSetter {
  SetDeviceNum,
  SetDeviceType,
  SetOmpDefaultDevice
};

static void fatalInvalidDeviceNumAndType(int dev_num, acc_device_t dev_type,
                                         DeviceNumAndTypeSetter Setter,
                                         bool NumIsDefault,
                                         bool TypeIsDefault) {
  switch (Setter) {
  case SetDeviceType:
    ACC2OMP_ASSERT(NumIsDefault, "expected default device number");
    acc2omp_fatal(ACC2OMP_MSG(set_device_type_no_devices),
                  deviceTypeToString(dev_type, TypeIsDefault));
    break;
  case SetDeviceNum:
    ACC2OMP_ASSERT(!NumIsDefault, "unexpected default device number");
    acc2omp_fatal(ACC2OMP_MSG(set_device_num_invalid_num), dev_num,
                  deviceTypeToString(dev_type, TypeIsDefault));
    break;
  case SetOmpDefaultDevice:
    // The default device number diagnostic expects it to be 0.
    ACC2OMP_ASSERT(!NumIsDefault || dev_num == 0,
                   "expected default device number to be 0");
    acc2omp_fatal(NumIsDefault ? ACC2OMP_MSG(env_acc_device_num_default_invalid)
                               : ACC2OMP_MSG(env_acc_device_num_invalid),
                  dev_num, deviceTypeToStringEnv(dev_type, TypeIsDefault));
    break;
  }
  ACC2OMP_UNREACHABLE("expected valid DeviceNumAndTypeSetter");
}

static void setDeviceNumAndType(int dev_num, acc_device_t dev_type,
                                DeviceNumAndTypeSetter Setter,
                                bool NumIsDefault, bool TypeIsDefault) {
  // Keep in mind that we implement acc_set_device_type as merely
  // acc_set_device_num(0, dev_type), so notes below are for both.
  //
  // Handling of invalid device numbers:
  // - Produce a runtime error.
  // - A runtime error is always produced for acc_device_none, which is defined
  //   to have no valid dev_num.
  // - A runtime error is never produced for acc_device_current, which is
  //   defined to ignore the specified dev_num.
  // - OpenACC 3.1, sec. 3.2.2 "acc_set_device_type", "Restrictions",
  //   L3017-3020:
  //   - "If the device type dev_type is not available, the behavior is
  //     implementation-defined; in particular, the program may abort."
  //   - "If some compute regions are compiled to only use one device type,
  //     calling this routine with a different device type may produce undefined
  //     behavior."
  // - OpenACC 3.1, sec. 3.2.4 "acc_set_device_num", "Restrictions", L3061-3062:
  //   "If the value of dev_num is greater than or equal to the value returned
  //   by acc_get_num_devices for that device type, the behavior is
  //   implementation-defined."
  // - nvc 20.11-0 appears to do nothing instead of producing a runtime error.
  //   Either behavior appears to be acceptable under OpenACC 3.1, and we feel
  //   that a runtime error is more helpful.
  //
  // Handling of special values:
  // - OpenACC 3.1, sec. 3.2.4 "acc_set_device_num", L3054-3056: "If the value
  //   of dev_num is negative, the runtime will revert to its default behavior,
  //   which is implementation-defined.  If the value of the dev_type is zero,
  //   the selected device number will be used for all device types."
  // - We're not sure what either of these statements mean, and, in our
  //   experiments using nvc 20.11-0, acc_set_device_num does nothing when
  //   passed one of these special values.
  // - What behavior is reverted to the implementation default?  Does it mean to
  //   choose an implementation-defined default device number for the specified
  //   type?  Is that the one used by acc_set_device_type?
  // - In our experiments using nvc 20.11-0, device numbers don't appear to be
  //   recorded for all device types except the current one (OpenACC 3.1 only
  //   talks about one acc-current-device-num-var not one per device type), so
  //   what does it meant to set the device number for all device types?  Is it
  //   the device number to be used by acc_set_device_type?
  //
  // Interestingly, after acc_set_device_num(dev_num, acc_device_not_host),
  // acc_get_device_type() may not return acc_device_not_host, and so
  // acc_get_device_num(acc_get_device_type()) may not return dev_num.
  switch (dev_type) {
  case acc_device_none:
    fatalInvalidDeviceNumAndType(dev_num, dev_type, Setter, NumIsDefault,
                                 TypeIsDefault);
    return; // should be unreachable
  case acc_device_not_host:
    if (dev_num < 0 || omp_get_num_devices() <= dev_num)
      fatalInvalidDeviceNumAndType(dev_num, dev_type, Setter, NumIsDefault,
                                   TypeIsDefault);
    omp_set_default_device(dev_num);
    return;
  case acc_device_default:
    setDeviceNumAndType(dev_num, acc_get_device_type(), Setter, NumIsDefault,
                        /*TypeIsDefault=*/true);
    return;
  case acc_device_current:
    return;
  case acc_device_host:
  case acc_device_nvidia:
  case acc_device_x86_64:
  case acc_device_ppc64le: {
    // omp_get_device_of_type returns -1 if either:
    // - acc2omp_get_omp_device_t returns omp_device_none because omp_device_t
    //   has no corresponding enumerator, and so our implementation has no
    //   devices known to be of type dev_type.
    // - dev_num is outside the range of devices of the type dev_type as
    //   identified by the corresponding omp_device_t enumerator.
    int DevNumOMP =
        omp_get_device_of_type(acc2omp_get_omp_device_t(dev_type), dev_num);
    if (DevNumOMP == -1)
      fatalInvalidDeviceNumAndType(dev_num, dev_type, Setter, NumIsDefault,
                                   TypeIsDefault);
    omp_set_default_device(DevNumOMP);
    return;
  }
  }
  // acc_set_device_type and acc_set_device_num can receive an invalid device
  // type, and they pass it along here.  acc2omp_set_omp_default_device checks
  // for an invalid device type first.
  switch (Setter) {
  case SetDeviceType:
    acc2omp_fatal(ACC2OMP_MSG(set_device_type_invalid_type), dev_type);
    break;
  case SetDeviceNum:
    acc2omp_fatal(ACC2OMP_MSG(set_device_num_invalid_type), dev_type);
    break;
  case SetOmpDefaultDevice:
    ACC2OMP_UNREACHABLE(
        "unexpected device type from acc2omp_set_omp_default_device");
    return;
  }
}

// OpenACC 3.1 does not explain whether acc_set_device_type affects
// acc-current-device-num-var, especially if acc-current-device-num-var is
// currently out of range for dev_type.  nvc 20.11-0 appears to just set it to
// 0, as we do here.  Also see comments for acc_get_device_num and its
// ambiguous use of its dev_type parameter.  See setDeviceNumAndType comments
// for handling of special or invalid values.
void acc_set_device_type(acc_device_t dev_type) {
  setDeviceNumAndType(0, dev_type, SetDeviceType, /*NumIsDefault=*/true,
                      /*TypeIsDefault=*/false);
}

acc_device_t acc_get_device_type() {
  // OpenACC 3.1, sec. 3.2.3 "acc_get_device_type", "Restrictions", L3039:
  // "If the device type has not yet been selected, the value acc_device_none
  // may be returned."  We are not sure how this situation can arise.
  int DevNumOMP = omp_get_default_device();
  // acc2omp_get_acc_device_t returns acc_device_none when acc_device_t or
  // omp_device_t doesn't yet have an enumerator for the current device's type
  // (in the latter case, omp_get_device_type returns omp_device_none).
  acc_device_t DevTypeACC =
      acc2omp_get_acc_device_t(omp_get_device_type(DevNumOMP));
  if (DevTypeACC == acc_device_none)
    return acc_device_not_host;
  // Must be acc_device_host or an architecture-specific enumerator for which
  // omp_device_t does have a corresponding architecture-specific enumerator.
  return DevTypeACC;
}

// See setDeviceNumAndType comments for handling of special or invalid values.
void acc_set_device_num(int dev_num, acc_device_t dev_type) {
  setDeviceNumAndType(dev_num, dev_type, SetDeviceNum, /*NumIsDefault=*/false,
                      /*TypeIsDefault=*/false);
}

// acc_get_device_num returns:
// - The zero-origin index of the current device in the list of all devices of
//   type dev_type if the current device is a device of that type.
// - -1 otherwise.  nvc 20.11-0 returns 0 instead, but that's misleading as it
//   seems to indicate this is the first device of that type.
//
// The OpenACC spec is unclear about how the dev_type parameter affects the
// result:
// - OpenACC 3.1, sec. 3.2.5 "acc_get_device_num, L3065-3066 and L3074-3075:
//   "The acc_get_device_num routine returns the value of
//   acc-current-device-num-var for the current thread."  There is no
//   description of the significance of the dev_type parameter, and there is no
//   indication that acc-current-device-num-var is defined per device type.
// - OpenACC 2.0 (corrected), sec. 3.2.5 "acc_get_device_num":
//   "The acc_get_device_num routine returns the device number of the specified
//   device type that will be used to run the next accelerator parallel or
//   kernels region."  This version refers to the parameter but is still unclear
//   about device types other than the current device type.  Should the runtime
//   maintain a table of device numbers for all device types?  Should
//   acc_set_device_type use that table instead of setting the device number to
//   0?  Our implementation does not maintain such a table, and nvc 20.11-0 does
//   not appear to either in our experiments.
int acc_get_device_num(acc_device_t dev_type) {
  switch (dev_type) {
  case acc_device_none:
    return -1;
  case acc_device_not_host: {
    int DevNumOMP = omp_get_default_device();
    if (DevNumOMP == omp_get_initial_device())
      return -1;
    return DevNumOMP;
  }
  case acc_device_default:
    return acc_get_device_num(acc_get_device_type());
  case acc_device_current:
    return 0;
  case acc_device_host:
  case acc_device_nvidia:
  case acc_device_x86_64:
  case acc_device_ppc64le: {
    // acc_get_device_type returns acc_device_not_host if omp_device_t has
    // no enumerator corresponding to dev_type, and so our implementation
    // logically then has no devices known to be of type dev_type.
    if (dev_type != acc_get_device_type())
      return -1;
    int DevNumACC = omp_get_typed_device_num(omp_get_default_device());
    // acc_get_device_type didn't return acc_device_not_host, and we have a
    // valid device number, so omp_get_typed_device_num should be able to return
    // a valid typed device number.
    ACC2OMP_ASSERT(DevNumACC != -1, "expected valid typed device number");
    return DevNumACC;
  }
  }
  acc2omp_fatal(ACC2OMP_MSG(get_device_num_invalid_type), dev_type);
  return -1;
}

/*****************************************************************************
 * Data and memory management routines.
 *
 * Handling of null pointers:
 * - The behavior is often not clearly specified in OpenACC 3.1.
 * - We attempt to follow a concept discussed by the OpenACC technical
 *   committee: a null pointer is considered always present.  Specifically, we
 *   assume the behavior is as if there is an "acc data" enclosing the entire
 *   program that maps a host null pointer to a device null pointer so that the
 *   structured reference count is always non-zero.  Because a null pointer
 *   doesn't actually point to data, we also assume that the number of bytes
 *   specified to a routine is ignored and that any update requested for a null
 *   pointer does nothing.
 * - We document each routine's specific handling of null pointers in more
 *   detail below and relate it to this concept where applicable.
 *
 * Handling of bytes=0:
 * - The behavior is usually not clearly specified in OpenACC 3.1.
 * - We attempt to mimc nvc 20.9-0 as much as possible:
 *   - Except for acc_is_present, for all routines that accept a bytes argument,
 *     do nothing other than returning a null pointer where the return type is
 *     void*.
 *   - For acc_is_present, handle bytes=0 like bytes=1.  This behavior follows
 *     OpenACC 3.1.
 *   - These behaviors seem to, at least partially, address the second potential
 *     use case below.
 * - Potential use cases:
 *   - bytes=0 for data of size > 0:
 *     - This is part of a more general use case of specifying bytes<N to
 *       operate on data of size N>0 when it is not convenient to determine the
 *       exact value of N at the routine's call site.  That is, this use case
 *       relies on N already stored in the present table.
 *     - The current behavior of enter/exit-data-like routines does not support
 *       this use case for bytes=0 though it does for bytes>0: it does not
 *       adjust dynamic reference counts and perform any consequent deletions or
 *       data transfers.  It does nothing.
 *     - This use case does not include other data and memory management
 *       routines, which operate on exactly the specified bytes not on N bytes.
 *   - bytes=0 for data of size 0:
 *     - That is, imagine an application that maps N>=0 bytes to the device and
 *       then calls various data and memory management routines for a subset,
 *       M<=N.  Imagine we want the application to gracefully handle (not fail
 *       for) the cases of M=0 and N=0.
 *     - To support the M=0 but N>0 case, it usually sufficient for acc_malloc,
 *       enter/exit-data-like routines, update routines, acc_map_data, and
 *       memcpy routines to do nothing when bytes=0: the specified address is
 *       present but operations on the subset M become no-ops.  acc_deviceptr,
 *       acc_hostptr, and acc_is_present indicate presence for the base address
 *       of the M bytes as when M>0.
 *     - The same is true for the M=N=0 case except that acc_deviceptr,
 *       acc_hostptr, and acc_is_present now indicate absence for the base
 *       address.  Whether that's desirable may be application-dependent.  It
 *       might be argued that application code that would invoke those for the M
 *       bytes would often be naturally skipped when M=0 (for example, in a loop
 *       with M iterations).
 * - Alternative behaviors:
 *   - Modify bytes=0 behavior for consistency with bytes>0 behavior to better
 *     support the above use cases and so that these routines agree with
 *     acc_deviceptr, acc_hostptr, and acc_is_present about when a (data_arg, 0)
 *     pair is considered present:
 *     - Modify acc_malloc: return a unique device address that can be passed to
 *       acc_map_data with bytes=0.
 *     - Modify enter-data-like routines and acc_map_data when address is not
 *       already present: create a present table entry for the address with size
 *       0.  As for bytes>0, enter-data-like routines should map a unique device
 *       address, and acc_map_data behavior should be undefined if the specified
 *       device address is not unique.  Unique device addresses are for the sake
 *       of acc_deviceptr and acc_hostptr.
 *     - Modify enter/exit-data-like and acc_map_data routines when address is
 *       already present: behave like the bytes>0 case.  That is,
 *       enter/exit-data-like routines should adjust the dynamic reference count
 *       and perform any consequent deletions or data transfers.  acc_map_data
 *       should produce a runtime error.
 *     - Modify update routines: check for presence even though bytes=0.  This
 *       seems fine if present table entries can have size 0.  Even so, the
 *       check could be viewed as redundant given that no bytes will actually
 *       by transferred.
 *     - One use case this alternative does not address: an accidental attempt
 *       to map two consecutive memory ranges with the same starting address
 *       because the size of the first range has fallen to zero.  This case
 *       would have to be handled specially in the application to avoid runtime
 *       errors or undefined behavior.  Such undefined behavior could occur when
 *       the ranges are consecutive only on the device side when, for example,
 *       acc_malloc is being used for memory pooling.
 *     - acc_is_present, for example, would no longer have identical behavior
 *       for bytes=0 and bytes=1.  This at least represents a subtle change from
 *       previous behavior.
 *   - Produce runtime errors:
 *     - The current bytes=0 behavior may be confusing due to inconsistencies
 *       with the bytes>0 behavior.
 *     - The previous alternative behavior to avoid that confusion has subtle
 *       differences in reference counting from nvc's current behavior and
 *       could thus lead to memory errors that are difficult to debug in
 *       existing applications.  Moreover, runtime errors or undefined behavior
 *       may still occur for the bytes=0 case but not the bytes>0 when
 *       attempting to map consecutive memory ranges.
 *     - Always producing a runtime error for bytes=0 would avoid all these
 *       problems.  It might be the best option if the above use cases are
 *       not worthwhile to support.
 * - These alternative behaviors need to be discussed with the OpenACC
 *   technical committee.
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
  SET_SOURCE_INFO();
  void *Res = omp_target_alloc(bytes, omp_get_default_device());
  CLEAR_SOURCE_INFO();
  return Res;
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
  SET_SOURCE_INFO();
  omp_target_free(data_dev, omp_get_default_device());
  CLEAR_SOURCE_INFO();
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
  int Dev = omp_get_default_device();
  if (PresenceSharedMemory == checkPresence(data_arg, bytes, Dev))
    return data_arg;
  // OpenACC 3.1, sec. 3.2.26 "acc_copyin", L3462:
  // "The acc_copyin routines are equivalent to an enter data directive with a
  // copyin clause."
  SET_SOURCE_INFO();
  void *Res = omp_target_map_to(data_arg, bytes, Dev);
  CLEAR_SOURCE_INFO();
  return Res;
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
  int Dev = omp_get_default_device();
  if (PresenceSharedMemory == checkPresence(data_arg, bytes, Dev))
    return data_arg;
  // OpenACC 3.1, sec. 3.2.27 "acc_create", L3502:
  // "The acc_create routines are equivalent to an enter data directive with a
  // create clause."
  SET_SOURCE_INFO();
  void *Res = omp_target_map_alloc(data_arg, bytes, Dev);
  CLEAR_SOURCE_INFO();
  return Res;
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
  int Dev = omp_get_default_device();
  if (PresenceSharedMemory == checkPresence(data_arg, bytes, Dev))
    return;
  // OpenACC 3.1, sec. 3.2.28 "acc_copyout", L3532:
  // "The acc_copyout routines are equivalent to an exit data directive with a
  // copyout clause."
  SET_SOURCE_INFO();
  omp_target_map_from(data_arg, bytes, Dev);
  CLEAR_SOURCE_INFO();
}

void acc_copyout_finalize(void *data_arg, size_t bytes) {
  // nvc 20.9-0 does not implement acc_copyout_finalize.
  if (!data_arg || !bytes)
    return;
  int Dev = omp_get_default_device();
  if (PresenceSharedMemory == checkPresence(data_arg, bytes, Dev))
    return;
  // OpenACC 3.1, sec. 3.2.28 "acc_copyout", L3533-3534:
  // "The acc_copyout_finalize routines are equivalent to an exit data directive
  // with both copyout and finalize clauses."
  SET_SOURCE_INFO();
  omp_target_map_from_delete(data_arg, bytes, Dev);
  CLEAR_SOURCE_INFO();
}

void acc_delete(void *data_arg, size_t bytes) {
  if (!data_arg || !bytes)
    return;
  int Dev = omp_get_default_device();
  if (PresenceSharedMemory == checkPresence(data_arg, bytes, Dev))
    return;
  // OpenACC 3.1, sec. 3.2.29 "acc_delete", L3560:
  // "The acc_delete routines are equivalent to an exit data directive with a
  // delete clause."
  SET_SOURCE_INFO();
  omp_target_map_release(data_arg, bytes, Dev);
  CLEAR_SOURCE_INFO();
}

void acc_delete_finalize(void *data_arg, size_t bytes) {
  // nvc 20.9-0 does not implement acc_delete_finalize.
  if (!data_arg || !bytes)
    return;
  int Dev = omp_get_default_device();
  if (PresenceSharedMemory == checkPresence(data_arg, bytes, Dev))
    return;
  // OpenACC 3.1, sec. 3.2.29 "acc_delete", L3561-3562:
  // "The acc_delete_finalize routines are equivalent to an exit data directive
  // with both delete clause and finalize clauses."
  SET_SOURCE_INFO();
  omp_target_map_delete(data_arg, bytes, Dev);
  CLEAR_SOURCE_INFO();
}

void acc_update_device(void *data_arg, size_t bytes) {
  if (!data_arg || !bytes)
    return;
  int Dev = omp_get_default_device();
  if (PresenceSharedMemory == checkPresence(data_arg, bytes, Dev))
    return;
  // OpenACC 3.1, sec. 3.2.30 "acc_update_device", L3590:
  // "The acc_update_device routine is equivalent to an update directive with a
  // device clause."
  SET_SOURCE_INFO();
  omp_target_update_to(data_arg, bytes, Dev);
  CLEAR_SOURCE_INFO();
}

void acc_update_self(void *data_arg, size_t bytes) {
  if (!data_arg || !bytes)
    return;
  int Dev = omp_get_default_device();
  if (PresenceSharedMemory == checkPresence(data_arg, bytes, Dev))
    return;
  // OpenACC 3.1, sec. 3.2.31 "acc_update_self", L3615:
  // "The acc_update_self routine is equivalent to an update directive with a
  // self clause,"
  SET_SOURCE_INFO();
  omp_target_update_from(data_arg, bytes, Dev);
  CLEAR_SOURCE_INFO();
}

/*----------------------------------------------------------------------------
 * Mapping routines.
 *--------------------------------------------------------------------------*/

void acc_map_data(void *data_arg, void *data_dev, size_t bytes) {
  // Handling of bytes=0:
  // - Do nothing, regardless of the specified pointers, even if null.
  // - This behavior appears to mimic nvc 20.9-0.
  // - OpenACC 3.1 and OpenMP 5.1 are unclear about the behavior in this case.
  if (!bytes)
    return;
  // Handling of null pointer:
  // - Produce a runtime error.
  // - nvc 20.9-0 appears to implement this case as a no-op.
  // - OpenACC 3.1 and OpenMP 5.1 are unclear about the behavior in this case.
  // - This behavior seems to follow the OpenACC technical committee's idea that
  //   a null pointer is considered always present, especially if we assume it
  //   was automatically mapped to a null pointer.  That is, an acc_map_data
  //   with a host null pointer is then an error, and an acc_map_data with a
  //   device null pointer is undefined, according to OpenACC 3.1, sec. 3.2.32
  //   "acc_map_data", L3637-3639: "It is an error to call acc_map_data for host
  //   data that is already present in the current device memory.  It is
  //   undefined to call acc_map_data with a device address that is already
  //   mapped to host data."
  if (!data_arg)
    acc2omp_fatal(ACC2OMP_MSG(map_data_host_pointer_null));
  if (!data_dev)
    acc2omp_fatal(ACC2OMP_MSG(map_data_device_pointer_null));

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
  int Dev = omp_get_default_device();
  Presence P = checkPresence(data_arg, bytes, Dev);
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
  SET_SOURCE_INFO();
  int Err = omp_target_associate_ptr(data_arg, data_dev, bytes,
                                     /*device_offset=*/0, Dev);
  CLEAR_SOURCE_INFO();
  if (Err)
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
  int Dev = omp_get_default_device();
  if (PresenceSharedMemory == checkPresence(data_arg, /*Bytes=*/0, Dev))
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
  SET_SOURCE_INFO();
  int Err = omp_target_disassociate_ptr(data_arg, Dev);
  CLEAR_SOURCE_INFO();
  if (Err)
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
  int Dev = omp_get_default_device();
  if (PresenceSharedMemory == checkPresence(data_arg, /*Bytes=*/0, Dev))
    return data_arg;
  // OpenACC 3.1, sec. 3.2.34 "acc_deviceptr", L3665-3667:
  // "The acc_deviceptr routine returns the device pointer associated with a
  // host address.  data_arg is the address of a host variable or array that has
  // an active lifetime on the current device.  If the data is not present in
  // the current device memory, the routine returns a NULL value."
  return omp_get_mapped_ptr(data_arg, Dev);
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
  int Dev = omp_get_default_device();
  if (PresenceSharedMemory == checkPresence(data_dev, /*Bytes=*/0, Dev))
    return data_dev;
  // OpenACC 3.1, sec. 3.2.35 "acc_hostptr", L3675-3678:
  // "The acc_hostptr routine returns the host pointer associated with a device
  // address. data_dev is the address of a device variable or array, such as
  // that returned from acc_deviceptr, acc_create or acc_copyin. If the device
  // address is NULL, or does not correspond to any host address, the routine
  // returns a NULL value."
  return omp_get_mapped_hostptr(data_dev, Dev);
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
  return omp_target_is_accessible(data_arg, bytes, omp_get_default_device());
}

/*----------------------------------------------------------------------------
 * memcpy routines.
 *
 * Handling of bytes=0:
 * - Do nothing, regardless of the specified pointers (even if null or data
 *   pointed to is inaccessible) or device numbers (even if invalid).
 * - This behavior appears to mimic nvc 20.9-0 except that nvc doesn't yet
 *   implement acc_memcpy_d2d.
 * - LLVM OpenMP runtime's omp_target_memcpy specifically checks for this case
 *   and returns non-zero.
 * - OpenACC 3.1 and OpenMP 5.1 are unclear about the behavior in this case.
 * Handling of null pointer:
 * - Produce a runtime error, even if pointers are the same, as discussed
 *   below.
 * - In this case, instead of specifically checking for this case, nvc 20.9-0
 *   usually produces generic runtime errors, such as segmentation faults or
 *   CUDA errors.
 * - LLVM OpenMP runtime's omp_target_memcpy specifically checks for this case
 *   and returns non-zero.
 * - OpenACC 3.1 and OpenMP 5.1 are unclear about the behavior in this case.
 * Handling of invalid device numbers (for acc_memcpy_d2d):
 * - Even if the pointers refer to the same memory, as discussed below, produce
 *   a runtime error that is distinct from the runtime error produced when data
 *   isn't present on the device.  Otherwise, the latter runtime error is
 *   confusing.
 * - OpenACC 3.1, sec. 3.2.42 "acc_memcpy_d2d", L3799-3800:
 *   "The acc_memcpy_d2d and acc_memcpy_d2d_async routines are passed the
 *   address of destination and source host pointers as well as integer device
 *   numbers for the destination and source devices, which must both be of the
 *   current device type."
 * - nvc 20.9-0 doesn't yet implement acc_memcpy_d2d.
 * Handling pointers referring to the same memory:
 * - In the case of acc_memcpy_device, if the pointers are the same, do nothing.
 * - In the case of the other routines, if both pointers refer to shared memory
 *   and are the same, do nothing.
 * - Additionally in the case of acc_memcpy_d2d, if the devices are the same
 *   and the pointers are the same, do nothing, even if the data pointed to is
 *   inaccessible (checking accessibility isn't cheap, so don't do it just to
 *   protect a no-op).
 * - OpenACC 3.1 is unclear about the behavior in this case but does have text
 *   that was likely misworded but intended to specify the above behavior for
 *   acc_memcpy_d2d: OpenACC 3.1, sec. 3.2.42 "acc_memcpy_d2d", L3801-3802:
 *   "If both arrays are in shared memory, then no action is taken."
 * - Text to specify the above behavior in OpenACC has already been proposed:
 *   - It's not meant simply as a clarification that behavior is not left
 *     undefined as for, for example, C99's memcpy.  It's meant as an
 *     optimization for shared memory, exactly as specified for OpenACC data
 *     clauses and the enter/exit-data-like routines and update routines.
 *   - However, it's also extended to copies within a single device's memory,
 *     using either acc_memcpy_device or acc_memcpy_d2d.  Because this
 *     optimization isn't part of the shared memory optimizations, it might be
 *     worthwhile for the OpenACC rationale doc to explain all this.
 * - OpenMP 5.1 could be read to mean this case is well defined for
 *   omp_target_memcpy, but it doesn't specifically discuss overlapping regions,
 *   so we assume this case wasn't actually considered.  Moreover, LLVM's
 *   implementation of omp_target_memcpy does not check for this case and
 *   sometimes calls memcpy, for which this case has undefined behavior due to
 *   C99's restrict qualifier.  Thus, we must check here.  TODO: If OpenMP
 *   clarifies omp_target_memcpy's behavior for this case as effectively a
 *   no-op, then we can eliminate our checks in most cases.
 *--------------------------------------------------------------------------*/

void acc_memcpy_to_device(void *data_dev_dest, void *data_host_src,
                          size_t bytes) {
  if (!bytes)
    return;
  if (!data_dev_dest)
    acc2omp_fatal(ACC2OMP_MSG(memcpy_to_device_dest_pointer_null));
  if (!data_host_src)
    acc2omp_fatal(ACC2OMP_MSG(memcpy_to_device_src_pointer_null));
  int Dev = omp_get_default_device();
  if (PresenceSharedMemory == checkPresence(data_host_src, bytes, Dev) &&
      data_dev_dest == data_host_src)
    return;
  SET_SOURCE_INFO();
  int Err = omp_target_memcpy(data_dev_dest, data_host_src, bytes,
                              /*dst_offset=*/0, /*src_offset=*/0, Dev,
                              omp_get_initial_device());
  CLEAR_SOURCE_INFO();
  if (Err)
    acc2omp_fatal(ACC2OMP_MSG(memcpy_to_device_fail));
}

void acc_memcpy_from_device(void *data_host_dest, void *data_dev_src,
                            size_t bytes) {
  if (!bytes)
    return;
  if (!data_host_dest)
    acc2omp_fatal(ACC2OMP_MSG(memcpy_from_device_dest_pointer_null));
  if (!data_dev_src)
    acc2omp_fatal(ACC2OMP_MSG(memcpy_from_device_src_pointer_null));
  int Dev = omp_get_default_device();
  if (PresenceSharedMemory == checkPresence(data_host_dest, bytes, Dev) &&
      data_host_dest == data_dev_src)
    return;
  SET_SOURCE_INFO();
  int Err = omp_target_memcpy(data_host_dest, data_dev_src, bytes,
                              /*dst_offset=*/0, /*src_offset=*/0,
                              omp_get_initial_device(), Dev);
  CLEAR_SOURCE_INFO();
  if (Err)
    acc2omp_fatal(ACC2OMP_MSG(memcpy_from_device_fail));
}

void acc_memcpy_device(void *data_dev_dest, void *data_dev_src, size_t bytes) {
  if (!bytes)
    return;
  if (!data_dev_dest)
    acc2omp_fatal(ACC2OMP_MSG(memcpy_device_dest_pointer_null));
  if (!data_dev_src)
    acc2omp_fatal(ACC2OMP_MSG(memcpy_device_src_pointer_null));
  if (data_dev_dest == data_dev_src)
    return;
  int Dev = omp_get_default_device();
  SET_SOURCE_INFO();
  int Err = omp_target_memcpy(data_dev_dest, data_dev_src, bytes,
                              /*dst_offset=*/0, /*src_offset=*/0, Dev, Dev);
  CLEAR_SOURCE_INFO();
  if (Err)
    acc2omp_fatal(ACC2OMP_MSG(memcpy_device_fail));
}

void acc_memcpy_d2d(void *data_arg_dest, void *data_arg_src, size_t bytes,
                    int dev_num_dest, int dev_num_src) {
  // Do nothing if bytes=0.
  if (!bytes)
    return;

  // Validate args except for checking presence, which is check below.
  if (!data_arg_dest)
    acc2omp_fatal(ACC2OMP_MSG(memcpy_d2d_dest_pointer_null));
  if (!data_arg_src)
    acc2omp_fatal(ACC2OMP_MSG(memcpy_d2d_src_pointer_null));
  acc_device_t DevType = acc_get_device_type();
  int NumDevs = acc_get_num_devices(DevType);
  if (dev_num_dest < 0 || NumDevs <= dev_num_dest)
    acc2omp_fatal(ACC2OMP_MSG(memcpy_d2d_dest_device_invalid), dev_num_dest,
                  deviceTypeToString(DevType));
  if (dev_num_src < 0 || NumDevs <= dev_num_src)
    acc2omp_fatal(ACC2OMP_MSG(memcpy_d2d_src_device_invalid), dev_num_src,
                  deviceTypeToString(DevType));

  // Do nothing if pointers and devices are the same.
  if (dev_num_dest == dev_num_src && data_arg_dest == data_arg_src)
    return;

  // Convert OpenACC device numbers to OpenMP device numbers.
  int DevNumDestOMP;
  int DevNumSrcOMP;
  if (DevType == acc_device_not_host) {
    DevNumDestOMP = dev_num_dest;
    DevNumSrcOMP = dev_num_src;
  } else {
    omp_device_t DevTypeOMP = acc2omp_get_omp_device_t(DevType);
    // acc_get_device_type returns only acc_device_host, acc_device_not_host,
    // or an architecture-specific enum that was representable as omp_device_t.
    ACC2OMP_ASSERT(DevTypeOMP != omp_device_none,
                   "expected device type to be representable as omp_device_t");
    DevNumDestOMP = omp_get_device_of_type(DevTypeOMP, dev_num_dest);
    DevNumSrcOMP = omp_get_device_of_type(DevTypeOMP, dev_num_src);
    // omp_get_device_of_type returns -1 if the device numbers are invalid, but
    // we already checked them.
    ACC2OMP_ASSERT(DevNumDestOMP != -1, "expected dev_num_dest to be valid");
    ACC2OMP_ASSERT(DevNumSrcOMP != -1, "expected dev_num_src to be valid");
  }

  // Do nothing if pointers are the same and data is in shared memory.
  Presence DestPresence = checkPresence(data_arg_dest, bytes, DevNumDestOMP);
  Presence SrcPresence = checkPresence(data_arg_src, bytes, DevNumSrcOMP);
  if (DestPresence == PresenceSharedMemory &&
      SrcPresence == PresenceSharedMemory && data_arg_dest == data_arg_src)
    return;

  // OpenACC 3.1, sec. 3.2.42, L3802-3803:
  // "If either pointer is not in shared memory, then that array must be present
  // on its respective device."
  if (DestPresence != PresenceFullyMapped &&
      DestPresence != PresenceSharedMemory)
    acc2omp_fatal(ACC2OMP_MSG(memcpy_d2d_dest_data_inaccessible));
  if (SrcPresence != PresenceFullyMapped && SrcPresence != PresenceSharedMemory)
    acc2omp_fatal(ACC2OMP_MSG(memcpy_d2d_src_data_inaccessible));

  // Convert host pointers to device pointers.
  void *DataDevDest = DestPresence == PresenceSharedMemory
                          ? data_arg_dest
                          : omp_get_mapped_ptr(data_arg_dest, DevNumDestOMP);
  void *DataDevSrc = SrcPresence == PresenceSharedMemory
                         ? data_arg_src
                         : omp_get_mapped_ptr(data_arg_src, DevNumSrcOMP);

  // Transfer the data.
  SET_SOURCE_INFO();
  int Err = omp_target_memcpy(DataDevDest, DataDevSrc, bytes, /*dst_offset=*/0,
                              /*src_offset=*/0, DevNumDestOMP, DevNumSrcOMP);
  CLEAR_SOURCE_INFO();
  if (Err)
    acc2omp_fatal(ACC2OMP_MSG(memcpy_d2d_fail));
}

/*****************************************************************************
 * OpenMP runtime handler for initial selection of the OpenACC current device
 * based on, for example, the environment variables \c ACC_DEVICE_TYPE and
 * \c ACC_DEVICE_NUM.  Documentation appears on
 * \c acc2omp_set_omp_default_device_t in acc2omp-handlers.h.
 ****************************************************************************/

extern "C" void acc2omp_set_omp_default_device() {
  // Check that the typedef declaration matches the definition.  Keep the
  // assignment separate from the declaration to avoid an unused variable
  // warning.
  acc2omp_set_omp_default_device_t FnPtr;
  FnPtr = acc2omp_set_omp_default_device;

  const char *DevTypeEnv = getenv("ACC_DEVICE_TYPE");
  const char *DevNumEnv = getenv("ACC_DEVICE_NUM");
  if (DevTypeEnv && !*DevTypeEnv)
    DevTypeEnv = nullptr;
  if (DevNumEnv && !*DevNumEnv)
    DevNumEnv = nullptr;
  if (!DevTypeEnv && !DevNumEnv)
    return;

  // Parse ACC_DEVICE_TYPE.
  acc_device_t DevType = acc_device_none;
  if (!DevTypeEnv)
    DevType = acc_get_device_type();
#define ACC2OMP_DEVICE_ENUMERATOR(Device)                                      \
  else if (!strcasecmp(DevTypeEnv, #Device))                                   \
    DevType = acc_device_##Device;
  ACC2OMP_FOREACH_DEVICE(ACC2OMP_DEVICE_ENUMERATOR)
#undef ACC2OMP_DEVICE_ENUMERATOR
  switch (DevType) {
  case acc_device_none:
  case acc_device_default:
  case acc_device_current:
    ACC2OMP_ASSERT(!!DevTypeEnv, "expected default device type to be valid");
    acc2omp_fatal(ACC2OMP_MSG(env_acc_device_type_invalid), DevTypeEnv);
    break;
  case acc_device_host:
  case acc_device_not_host:
  case acc_device_nvidia:
  case acc_device_x86_64:
  case acc_device_ppc64le:
    break;
  }

  // Parse ACC_DEVICE_NUM.
  int DevNum;
  if (!DevNumEnv) {
    DevNum = 0;
  } else {
    for (const char *C = DevNumEnv; *C != '\0'; ++C) {
      if (*C < '0' || '9' < *C)
        acc2omp_fatal(ACC2OMP_MSG(env_acc_device_num_parse_error), DevNumEnv);
    }
    if (DevNumEnv[0] == '0' && DevNumEnv[1] != '\0')
      acc2omp_fatal(ACC2OMP_MSG(env_acc_device_num_parse_error), DevNumEnv);
    DevNum = atoi(DevNumEnv);
  }

  // Set the device number and type, or complain if the number is invalid for
  // the type.
  setDeviceNumAndType(DevNum, DevType, SetOmpDefaultDevice, !DevNumEnv,
                      !DevTypeEnv);
}
