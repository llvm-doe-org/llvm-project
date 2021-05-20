// Check that all callbacks are dispatched in the correct order with the
// correct data for a simple program.
//
// Check the following cases:
// - An enter data and exit data directive pair (-DDIR=DIR_ENTER_EXIT_DATA)
// - A data construct by itself (-DDIR=DIR_DATA)
// - A parallel construct by itself (-DDIR=DIR_PAR)
// - A parallel construct and update directive nested within data constructs
//   (-DDIR=DIR_DATAPAR)

// RUN: %data dirs {
// RUN:   (dir-cflags=-DDIR=DIR_ENTER_EXIT_DATA dir-fc1=DATA dir-fc2=NOPAR dir-fc3=NODATAPAR
// RUN:    arr-enter-construct=enter_data arr-exit-construct=exit_data
// RUN:    arr0-enter-line-no=20000       arr0-enter-end-line-no=20000
// RUN:    arr0-exit-line-no=30000        arr0-exit-end-line-no=30000
// RUN:    arr1-enter-line-no=20000       arr1-enter-end-line-no=20000
// RUN:    arr1-exit-line-no=30000        arr1-exit-end-line-no=30000
// RUN:    kern-line-no=                  kern-end-line-no=
// RUN:    update0-line-no=               update1-line-no=            )
// RUN:   (dir-cflags=-DDIR=DIR_DATA dir-fc1=DATA dir-fc2=NOPAR dir-fc3=NODATAPAR
// RUN:    arr-enter-construct=data arr-exit-construct=data
// RUN:    arr0-enter-line-no=40000 arr0-enter-end-line-no=50000
// RUN:    arr0-exit-line-no=40000  arr0-exit-end-line-no=50000
// RUN:    arr1-enter-line-no=40000 arr1-enter-end-line-no=50000
// RUN:    arr1-exit-line-no=40000  arr1-exit-end-line-no=50000
// RUN:    kern-line-no=            kern-end-line-no=
// RUN:    update0-line-no=         update1-line-no=            )
// RUN:   (dir-cflags=-DDIR=DIR_PAR dir-fc1=PAR dir-fc2=HASPAR dir-fc3=NODATAPAR
// RUN:    arr-enter-construct=parallel arr-exit-construct=parallel
// RUN:    arr0-enter-line-no=60000     arr0-enter-end-line-no=100000
// RUN:    arr0-exit-line-no=60000      arr0-exit-end-line-no=100000
// RUN:    arr1-enter-line-no=60000     arr1-enter-end-line-no=100000
// RUN:    arr1-exit-line-no=60000      arr1-exit-end-line-no=100000
// RUN:    kern-line-no=60000           kern-end-line-no=100000
// RUN:    update0-line-no=             update1-line-no=             )
// RUN:   (dir-cflags=-DDIR=DIR_DATAPAR dir-fc1=DATAPAR dir-fc2=HASPAR dir-fc3=HASDATAPAR
// RUN:    arr-enter-construct=data arr-exit-construct=data
// RUN:    arr0-enter-line-no=70000 arr0-enter-end-line-no=140000
// RUN:    arr0-exit-line-no=70000  arr0-exit-end-line-no=140000
// RUN:    arr1-enter-line-no=80000 arr1-enter-end-line-no=130000
// RUN:    arr1-exit-line-no=80000  arr1-exit-end-line-no=130000
// RUN:    kern-line-no=90000       kern-end-line-no=100000
// RUN:    update0-line-no=110000   update1-line-no=120000       )
// RUN: }
// RUN: %data tgts {
// RUN:   (run-if=
// RUN:    tgt-acc-device=acc_device_host
// RUN:    tgt-host-or-off=HOST
// RUN:    tgt-cppflags=
// RUN:    tgt-cflags=
// RUN:    tgt-fc=HOST,HOST-%[dir-fc1],HOST-%[dir-fc2],HOST-%[dir-fc3])
// RUN:   (run-if=%run-if-x86_64
// RUN:    tgt-acc-device=acc_device_x86_64
// RUN:    tgt-host-or-off=OFF
// RUN:    tgt-cppflags=
// RUN:    tgt-cflags=-fopenmp-targets=%run-x86_64-triple
// RUN:    tgt-fc=OFF,OFF-%[dir-fc1],OFF-%[dir-fc2],OFF-%[dir-fc3],X86_64,X86_64-%[dir-fc1],X86_64-%[dir-fc2],X86_64-%[dir-fc3])
// RUN:   (run-if=%run-if-ppc64le
// RUN:    tgt-acc-device=acc_device_ppc64le
// RUN:    tgt-host-or-off=OFF
// RUN:    tgt-cppflags=
// RUN:    tgt-cflags=-fopenmp-targets=%run-ppc64le-triple
// RUN:    tgt-fc=OFF,OFF-%[dir-fc1],OFF-%[dir-fc2],OFF-%[dir-fc3],PPC64LE,PPC64LE-%[dir-fc1],PPC64LE-%[dir-fc2],PPC64LE-%[dir-fc3])
// RUN:   (run-if=%run-if-nvptx64
// RUN:    tgt-acc-device=acc_device_nvidia
// RUN:    tgt-host-or-off=OFF
// RUN:    tgt-cppflags=-DNVPTX64
// RUN:    tgt-cflags=-fopenmp-targets=%run-nvptx64-triple
// RUN:    tgt-fc=OFF,OFF-%[dir-fc1],OFF-%[dir-fc2],OFF-%[dir-fc3],NVPTX64,NVPTX64-%[dir-fc1],NVPTX64-%[dir-fc2],NVPTX64-%[dir-fc3])
// RUN: }
//      # Check offloading compilation both with and without offloading at run
//      # time.  This is important because some runtime calls that must be
//      # instrumented with some callback data are not exercised in both cases.
//      # Also check the case when host is selected but there are non-host
//      # devices and offload is not disabled.  That affects the OpenMP device
//      # number for host and thus might inadvertently affect profiling data if
//      # the implementation has a bug.
// RUN: %data run-envs {
// RUN:   (run-env=
// RUN:    env-fc=%[tgt-fc],%[tgt-host-or-off]-BEFORE-ENV,%[tgt-host-or-off]-BEFORE-ENV-%[dir-fc2])
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled'
// RUN:    env-fc=HOST,HOST-%[dir-fc1],HOST-%[dir-fc2],%[tgt-host-or-off]-BEFORE-ENV,%[tgt-host-or-off]-BEFORE-ENV-%[dir-fc2])
// RUN:   (run-env='env ACC_DEVICE_TYPE=host'
// RUN:    env-fc=HOST,HOST-%[dir-fc1],HOST-%[dir-fc2],%[tgt-host-or-off]-BEFORE-ENV,%[tgt-host-or-off]-BEFORE-ENV-%[dir-fc2])
// RUN: }
//      # Check both traditional compilation mode and source-to-source mode
//      # followed by OpenMP compilation.  This is important because, in the
//      # latter case, some profiling data that depends on OMPT extensions is
//      # currently available only when debug info is turned.
// RUN: %for dirs {
// RUN:   %for tgts {
// RUN:     %[run-if] %clang -Xclang -verify -fopenacc-print=omp %acc-includes \
// RUN:                      %s %[tgt-cppflags] %[dir-cflags] > %t-omp.c \
// RUN:                      -Wno-openacc-omp-map-hold
// RUN:     %[run-if] echo "// expected""-no-diagnostics" >> %t-omp.c
//          # With debug info.
// RUN:     %[run-if] %clang -Xclang -verify -fopenmp %fopenmp-version \
// RUN:         %acc-includes %t-omp.c %acc-libs %[tgt-cppflags] \
// RUN:         %[tgt-cflags] %[dir-cflags] -I%S -gline-tables-only -o %t
// RUN:     %for run-envs {
// RUN:       %[run-if] %[run-env] %t > %t.out 2> %t.err
// RUN:       %[run-if] FileCheck -input-file %t.err %s \
// RUN:           -allow-empty -check-prefixes=ERR
// RUN:       %[run-if] FileCheck -input-file %t.out %s \
// RUN:           -match-full-lines -strict-whitespace \
// RUN:           -check-prefixes=CHECK,CHECK-%[dir-fc1],CHECK-%[dir-fc2] \
// RUN:           -check-prefixes=CHECK-%[dir-fc3],%[env-fc] \
// RUN:           -DACC_DEVICE=%[tgt-acc-device] -DVERSION=%acc-version \
// RUN:           -DOFF_DEV=0 -DTHREAD_ID=0 -DASYNC_QUEUE=-1 \
// RUN:           -DARR_ENTER_CONSTRUCT=%[arr-enter-construct] \
// RUN:           -DARR_EXIT_CONSTRUCT=%[arr-exit-construct] \
// RUN:           -DDATA_CONSTRUCT='data' -DPARALLEL_CONSTRUCT='parallel' \
// RUN:           -DUPDATE_CONSTRUCT='update' \
// RUN:           -DIMPLICIT_FOR_DIRECTIVE=0 -DSRC_FILE='%t-omp.c' \
// RUN:           -DFUNC_NAME=main -DARR0_ENTER_LINE_NO=%[arr0-enter-line-no] \
// RUN:           -DARR0_ENTER_END_LINE_NO=%[arr0-enter-end-line-no] \
// RUN:           -DARR0_EXIT_LINE_NO=%[arr0-exit-line-no] \
// RUN:           -DARR0_EXIT_END_LINE_NO=%[arr0-exit-end-line-no] \
// RUN:           -DARR1_ENTER_LINE_NO=%[arr1-enter-line-no] \
// RUN:           -DARR1_ENTER_END_LINE_NO=%[arr1-enter-end-line-no] \
// RUN:           -DARR1_EXIT_LINE_NO=%[arr1-exit-line-no] \
// RUN:           -DARR1_EXIT_END_LINE_NO=%[arr1-exit-end-line-no] \
// RUN:           -DKERN_LINE_NO=%[kern-line-no] \
// RUN:           -DKERN_END_LINE_NO=%[kern-end-line-no] \
// RUN:           -DUPDATE0_LINE_NO=%[update0-line-no] \
// RUN:           -DUPDATE1_LINE_NO=%[update1-line-no] \
// RUN:           -DFUNC_LINE_NO=10000 -DFUNC_END_LINE_NO=150000 \
// RUN:           -DARR0_VAR_NAME=arr0 -DARR1_VAR_NAME='arr1[0:5]'
// RUN:     }
//          # Without debug info.
// RUN:     %[run-if] %clang -Xclang -verify -fopenmp %fopenmp-version \
// RUN:         %acc-includes %t-omp.c %acc-libs %[tgt-cppflags] \
// RUN:         %[tgt-cflags] %[dir-cflags] -I%S -o %t
// RUN:     %for run-envs {
// RUN:       %[run-if] %[run-env] %t > %t.out 2> %t.err
// RUN:       %[run-if] FileCheck -input-file %t.err %s \
// RUN:           -allow-empty -check-prefixes=ERR
// RUN:       %[run-if] FileCheck -input-file %t.out %s \
// RUN:           -match-full-lines -strict-whitespace \
// RUN:           -check-prefixes=CHECK,CHECK-%[dir-fc1],CHECK-%[dir-fc2] \
// RUN:           -check-prefixes=CHECK-%[dir-fc3],%[env-fc] \
// RUN:           -DACC_DEVICE=%[tgt-acc-device] -DVERSION=%acc-version \
// RUN:           -DOFF_DEV=0 -DTHREAD_ID=0 -DASYNC_QUEUE=-1 \
// RUN:           -DARR_ENTER_CONSTRUCT='runtime_api' \
// RUN:           -DARR_EXIT_CONSTRUCT='runtime_api' \
// RUN:           -DDATA_CONSTRUCT='runtime_api' \
// RUN:           -DPARALLEL_CONSTRUCT='runtime_api' \
// RUN:           -DUPDATE_CONSTRUCT='runtime_api' \
// RUN:           -DIMPLICIT_FOR_DIRECTIVE=1 \
//                # FIXME: These should be nullified instead of 'unknown' in
//                # order to distinguish from actual files or functions by that
//                # name.  The use of 'unknown' is inherited from upstream's
//                # LLVM OpenMP implementation, which uses 'unknown' in many
//                # cases of a default ident_t.
// RUN:           -DSRC_FILE='unknown'     -DFUNC_NAME='unknown' \
// RUN:           -DARR0_ENTER_LINE_NO=0   -DARR0_ENTER_END_LINE_NO=0 \
// RUN:           -DARR0_EXIT_LINE_NO=0    -DARR0_EXIT_END_LINE_NO=0 \
// RUN:           -DARR1_ENTER_LINE_NO=0   -DARR1_ENTER_END_LINE_NO=0 \
// RUN:           -DARR1_EXIT_LINE_NO=0    -DARR1_EXIT_END_LINE_NO=0 \
// RUN:           -DKERN_LINE_NO=0         -DKERN_END_LINE_NO=0 \
// RUN:           -DUPDATE0_LINE_NO=0      -DUPDATE1_LINE_NO=0 \
// RUN:           -DFUNC_LINE_NO=0         -DFUNC_END_LINE_NO=0 \
// RUN:           -DARR0_VAR_NAME='(null)' -DARR1_VAR_NAME='(null)'
// RUN:     }
// RUN:   }
// RUN: }
// RUN: %for dirs {
// RUN:   %for tgts {
// RUN:     %[run-if] %clang -Xclang -verify -fopenacc %acc-includes %s \
// RUN:         %[tgt-cflags] %[tgt-cppflags] %[dir-cflags] -o %t
// RUN:     %for run-envs {
// RUN:       %[run-if] %[run-env] %t > %t.out 2> %t.err
// RUN:       %[run-if] FileCheck -input-file %t.err %s \
// RUN:           -allow-empty -check-prefixes=ERR
// RUN:       %[run-if] FileCheck -input-file %t.out %s \
// RUN:           -match-full-lines -strict-whitespace \
// RUN:           -check-prefixes=CHECK,CHECK-%[dir-fc1],CHECK-%[dir-fc2] \
// RUN:           -check-prefixes=CHECK-%[dir-fc3],%[env-fc] \
// RUN:           -DACC_DEVICE=%[tgt-acc-device] -DVERSION=%acc-version \
// RUN:           -DOFF_DEV=0 -DTHREAD_ID=0 -DASYNC_QUEUE=-1 \
// RUN:           -DARR_ENTER_CONSTRUCT=%[arr-enter-construct] \
// RUN:           -DARR_EXIT_CONSTRUCT=%[arr-exit-construct] \
// RUN:           -DDATA_CONSTRUCT='data' -DPARALLEL_CONSTRUCT='parallel' \
// RUN:           -DUPDATE_CONSTRUCT='update' \
// RUN:           -DIMPLICIT_FOR_DIRECTIVE=0 -DSRC_FILE='%s' -DFUNC_NAME=main \
// RUN:           -DARR0_ENTER_LINE_NO=%[arr0-enter-line-no] \
// RUN:           -DARR0_ENTER_END_LINE_NO=%[arr0-enter-end-line-no] \
// RUN:           -DARR0_EXIT_LINE_NO=%[arr0-exit-line-no] \
// RUN:           -DARR0_EXIT_END_LINE_NO=%[arr0-exit-end-line-no] \
// RUN:           -DARR1_ENTER_LINE_NO=%[arr1-enter-line-no] \
// RUN:           -DARR1_ENTER_END_LINE_NO=%[arr1-enter-end-line-no] \
// RUN:           -DARR1_EXIT_LINE_NO=%[arr1-exit-line-no] \
// RUN:           -DARR1_EXIT_END_LINE_NO=%[arr1-exit-end-line-no] \
// RUN:           -DKERN_LINE_NO=%[kern-line-no] \
// RUN:           -DKERN_END_LINE_NO=%[kern-end-line-no] \
// RUN:           -DUPDATE0_LINE_NO=%[update0-line-no] \
// RUN:           -DUPDATE1_LINE_NO=%[update1-line-no] \
// RUN:           -DFUNC_LINE_NO=10000 -DFUNC_END_LINE_NO=150000 \
// RUN:           -DARR0_VAR_NAME=arr0 -DARR1_VAR_NAME='arr1[0:5]'
// RUN:     }
// RUN:   }
// RUN: }
//
// END.

// expected-no-diagnostics

#include "callbacks.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define DIR_ENTER_EXIT_DATA 1
#define DIR_DATA            2
#define DIR_PAR             3
#define DIR_DATAPAR         4

#ifndef NVPTX64
# define NVPTX64 0
#endif

// ERR-NOT:{{.}}
void acc_register_library(acc_prof_reg reg, acc_prof_reg unreg,
                          acc_prof_lookup lookup) {
  register_all_callbacks(reg);
}

#line 10000
int main() {
  const char *ompTargetOffload = getenv("OMP_TARGET_OFFLOAD");
  const char *accDeviceType = getenv("ACC_DEVICE_TYPE");
  bool ompTargetOffloadDisabled = ompTargetOffload && !strcmp(ompTargetOffload,
                                                              "disabled");
  bool accDeviceTypeHost = accDeviceType && !strcmp(accDeviceType, "host");
  bool offloadDisabled = ompTargetOffloadDisabled || accDeviceTypeHost;

  // CHECK-NOT:{{.}}

  int arr0[2] = {10, 11};
  int arr1[9] = {20, 21, 22, 23, 24};
  int notPresent0[10];
  int notPresent1[11];

  //      CHECK:arr0 host ptr = [[ARR0_HOST_PTR:0x[a-z0-9]+]]
  // CHECK-NEXT:arr1 host ptr = [[ARR1_HOST_PTR:0x[a-z0-9]+]]
  printf("arr0 host ptr = %p\n", arr0);
  printf("arr1 host ptr = %p\n", arr1);

  // CHECK-NEXT:before kernel
  printf("before kernel\n");

  // Device initialization.
  //
  // OFF-NEXT:acc_ev_device_init_start
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=1, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-NEXT:    line_no=[[ARR0_ENTER_LINE_NO]], end_line_no=[[ARR0_ENTER_END_LINE_NO]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=1, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_[[ARR_ENTER_CONSTRUCT]],
  // OFF-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NEXT:acc_ev_device_init_end
  // OFF-NEXT:  acc_prof_info
  // OFF-NEXT:    event_type=2, valid_bytes=72, version=[[VERSION]],
  // OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-NEXT:    line_no=[[ARR0_ENTER_LINE_NO]], end_line_no=[[ARR0_ENTER_END_LINE_NO]],
  // OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NEXT:  acc_other_event_info
  // OFF-NEXT:    event_type=2, valid_bytes=24,
  // OFF-NEXT:    parent_construct=acc_construct_[[ARR_ENTER_CONSTRUCT]],
  // OFF-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil)
  // OFF-NEXT:  acc_api_info
  // OFF-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NEXT:    device_type=[[ACC_DEVICE]]

  // Parallel construct start if -DDIR=DIR_PAR.
  //
  // CHECK-PAR-NEXT:acc_ev_compute_construct_start
  // CHECK-PAR-NEXT:  acc_prof_info
  // CHECK-PAR-NEXT:    event_type=16, valid_bytes=72, version=[[VERSION]],
  //  HOST-PAR-NEXT:    device_type=acc_device_host, device_number=0,
  //   OFF-PAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // CHECK-PAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // CHECK-PAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // CHECK-PAR-NEXT:    line_no=[[KERN_LINE_NO]], end_line_no=[[KERN_END_LINE_NO]],
  // CHECK-PAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // CHECK-PAR-NEXT:  acc_other_event_info
  // CHECK-PAR-NEXT:    event_type=16, valid_bytes=24,
  // CHECK-PAR-NEXT:    parent_construct=acc_construct_[[PARALLEL_CONSTRUCT]],
  // CHECK-PAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil)
  // CHECK-PAR-NEXT:  acc_api_info
  // CHECK-PAR-NEXT:    device_api=0, valid_bytes=12,
  //  HOST-PAR-NEXT:    device_type=acc_device_host
  //   OFF-PAR-NEXT:    device_type=[[ACC_DEVICE]]

  // Enter data for arr0.
  //
  //         OFF-NEXT:acc_ev_enter_data_start
  //         OFF-NEXT:  acc_prof_info
  //         OFF-NEXT:    event_type=10, valid_bytes=72, version=[[VERSION]],
  //         OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  //         OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //         OFF-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  //         OFF-NEXT:    line_no=[[ARR0_ENTER_LINE_NO]], end_line_no=[[ARR0_ENTER_END_LINE_NO]],
  //         OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //         OFF-NEXT:  acc_other_event_info
  //         OFF-NEXT:    event_type=10, valid_bytes=24,
  //         OFF-NEXT:    parent_construct=acc_construct_[[ARR_ENTER_CONSTRUCT]],
  //         OFF-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil)
  //         OFF-NEXT:  acc_api_info
  //         OFF-NEXT:    device_api=0, valid_bytes=12,
  //         OFF-NEXT:    device_type=[[ACC_DEVICE]]
  //         OFF-NEXT:acc_ev_alloc
  //         OFF-NEXT:  acc_prof_info
  //         OFF-NEXT:    event_type=8, valid_bytes=72, version=[[VERSION]],
  //         OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  //         OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //         OFF-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  //         OFF-NEXT:    line_no=[[ARR0_ENTER_LINE_NO]], end_line_no=[[ARR0_ENTER_END_LINE_NO]],
  //         OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //         OFF-NEXT:  acc_data_event_info
  //         OFF-NEXT:    event_type=8, valid_bytes=56,
  //         OFF-NEXT:    parent_construct=acc_construct_[[ARR_ENTER_CONSTRUCT]],
  //         OFF-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  //         OFF-NEXT:    var_name=[[ARR0_VAR_NAME]], bytes=8,
  //         OFF-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  //         OFF-NEXT:    device_ptr=[[ARR0_DEVICE_PTR:0x[a-z0-9]+]]
  //         OFF-NEXT:  acc_api_info
  //         OFF-NEXT:    device_api=0, valid_bytes=12,
  //         OFF-NEXT:    device_type=[[ACC_DEVICE]]
  //         OFF-NEXT:acc_ev_create
  //         OFF-NEXT:  acc_prof_info
  //         OFF-NEXT:    event_type=6, valid_bytes=72, version=[[VERSION]],
  //         OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  //         OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //         OFF-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  //         OFF-NEXT:    line_no=[[ARR0_ENTER_LINE_NO]], end_line_no=[[ARR0_ENTER_END_LINE_NO]],
  //         OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //         OFF-NEXT:  acc_data_event_info
  //         OFF-NEXT:    event_type=6, valid_bytes=56,
  //         OFF-NEXT:    parent_construct=acc_construct_[[ARR_ENTER_CONSTRUCT]],
  //         OFF-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  //         OFF-NEXT:    var_name=[[ARR0_VAR_NAME]], bytes=8,
  //         OFF-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  //         OFF-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  //         OFF-NEXT:  acc_api_info
  //         OFF-NEXT:    device_api=0, valid_bytes=12,
  //         OFF-NEXT:    device_type=[[ACC_DEVICE]]
  //         OFF-NEXT:acc_ev_enqueue_upload_start
  //         OFF-NEXT:  acc_prof_info
  //         OFF-NEXT:    event_type=20, valid_bytes=72, version=[[VERSION]],
  //         OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  //         OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //         OFF-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  //         OFF-NEXT:    line_no=[[ARR0_ENTER_LINE_NO]], end_line_no=[[ARR0_ENTER_END_LINE_NO]],
  //         OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //         OFF-NEXT:  acc_data_event_info
  //         OFF-NEXT:    event_type=20, valid_bytes=56,
  //         OFF-NEXT:    parent_construct=acc_construct_[[ARR_ENTER_CONSTRUCT]],
  //         OFF-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  //         OFF-NEXT:    var_name=[[ARR0_VAR_NAME]], bytes=8,
  //         OFF-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  //         OFF-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  //         OFF-NEXT:  acc_api_info
  //         OFF-NEXT:    device_api=0, valid_bytes=12,
  //         OFF-NEXT:    device_type=[[ACC_DEVICE]]
  //         OFF-NEXT:acc_ev_enqueue_upload_end
  //         OFF-NEXT:  acc_prof_info
  //         OFF-NEXT:    event_type=21, valid_bytes=72, version=[[VERSION]],
  //         OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  //         OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //         OFF-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  //         OFF-NEXT:    line_no=[[ARR0_ENTER_LINE_NO]], end_line_no=[[ARR0_ENTER_END_LINE_NO]],
  //         OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //         OFF-NEXT:  acc_data_event_info
  //         OFF-NEXT:    event_type=21, valid_bytes=56,
  //         OFF-NEXT:    parent_construct=acc_construct_[[ARR_ENTER_CONSTRUCT]],
  //         OFF-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  //         OFF-NEXT:    var_name=[[ARR0_VAR_NAME]], bytes=8,
  //         OFF-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  //         OFF-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  //         OFF-NEXT:  acc_api_info
  //         OFF-NEXT:    device_api=0, valid_bytes=12,
  //         OFF-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-DATAPAR-NEXT:acc_ev_enter_data_end
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=11, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-DATAPAR-NEXT:    line_no=[[ARR0_ENTER_LINE_NO]], end_line_no=[[ARR0_ENTER_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_other_event_info
  // OFF-DATAPAR-NEXT:    event_type=11, valid_bytes=24,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[DATA_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil)
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]

  // Enter data for arr1.
  //
  // OFF-DATAPAR-NEXT:acc_ev_enter_data_start
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=10, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-DATAPAR-NEXT:    line_no=[[ARR1_ENTER_LINE_NO]], end_line_no=[[ARR1_ENTER_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_other_event_info
  // OFF-DATAPAR-NEXT:    event_type=10, valid_bytes=24,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[DATA_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil)
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  //         OFF-NEXT:acc_ev_alloc
  //         OFF-NEXT:  acc_prof_info
  //         OFF-NEXT:    event_type=8, valid_bytes=72, version=[[VERSION]],
  //         OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  //         OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //         OFF-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  //         OFF-NEXT:    line_no=[[ARR1_ENTER_LINE_NO]], end_line_no=[[ARR1_ENTER_END_LINE_NO]],
  //         OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //         OFF-NEXT:  acc_data_event_info
  //         OFF-NEXT:    event_type=8, valid_bytes=56,
  //         OFF-NEXT:    parent_construct=acc_construct_[[ARR_ENTER_CONSTRUCT]],
  //         OFF-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  //         OFF-NEXT:    var_name=[[ARR1_VAR_NAME]], bytes=20,
  //         OFF-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  //         OFF-NEXT:    device_ptr=[[ARR1_DEVICE_PTR:0x[a-z0-9]+]]
  //         OFF-NEXT:  acc_api_info
  //         OFF-NEXT:    device_api=0, valid_bytes=12,
  //         OFF-NEXT:    device_type=[[ACC_DEVICE]]
  //         OFF-NEXT:acc_ev_create
  //         OFF-NEXT:  acc_prof_info
  //         OFF-NEXT:    event_type=6, valid_bytes=72, version=[[VERSION]],
  //         OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  //         OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //         OFF-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  //         OFF-NEXT:    line_no=[[ARR1_ENTER_LINE_NO]], end_line_no=[[ARR1_ENTER_END_LINE_NO]],
  //         OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //         OFF-NEXT:  acc_data_event_info
  //         OFF-NEXT:    event_type=6, valid_bytes=56,
  //         OFF-NEXT:    parent_construct=acc_construct_[[ARR_ENTER_CONSTRUCT]],
  //         OFF-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  //         OFF-NEXT:    var_name=[[ARR1_VAR_NAME]], bytes=20,
  //         OFF-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  //         OFF-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  //         OFF-NEXT:  acc_api_info
  //         OFF-NEXT:    device_api=0, valid_bytes=12,
  //         OFF-NEXT:    device_type=[[ACC_DEVICE]]
  //         OFF-NEXT:acc_ev_enqueue_upload_start
  //         OFF-NEXT:  acc_prof_info
  //         OFF-NEXT:    event_type=20, valid_bytes=72, version=[[VERSION]],
  //         OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  //         OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //         OFF-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  //         OFF-NEXT:    line_no=[[ARR1_ENTER_LINE_NO]], end_line_no=[[ARR1_ENTER_END_LINE_NO]],
  //         OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //         OFF-NEXT:  acc_data_event_info
  //         OFF-NEXT:    event_type=20, valid_bytes=56,
  //         OFF-NEXT:    parent_construct=acc_construct_[[ARR_ENTER_CONSTRUCT]],
  //         OFF-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  //         OFF-NEXT:    var_name=[[ARR1_VAR_NAME]], bytes=20,
  //         OFF-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  //         OFF-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  //         OFF-NEXT:  acc_api_info
  //         OFF-NEXT:    device_api=0, valid_bytes=12,
  //         OFF-NEXT:    device_type=[[ACC_DEVICE]]
  //         OFF-NEXT:acc_ev_enqueue_upload_end
  //         OFF-NEXT:  acc_prof_info
  //         OFF-NEXT:    event_type=21, valid_bytes=72, version=[[VERSION]],
  //         OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  //         OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //         OFF-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  //         OFF-NEXT:    line_no=[[ARR1_ENTER_LINE_NO]], end_line_no=[[ARR1_ENTER_END_LINE_NO]],
  //         OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //         OFF-NEXT:  acc_data_event_info
  //         OFF-NEXT:    event_type=21, valid_bytes=56,
  //         OFF-NEXT:    parent_construct=acc_construct_[[ARR_ENTER_CONSTRUCT]],
  //         OFF-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  //         OFF-NEXT:    var_name=[[ARR1_VAR_NAME]], bytes=20,
  //         OFF-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  //         OFF-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  //         OFF-NEXT:  acc_api_info
  //         OFF-NEXT:    device_api=0, valid_bytes=12,
  //         OFF-NEXT:    device_type=[[ACC_DEVICE]]
  //         OFF-NEXT:acc_ev_enter_data_end
  //         OFF-NEXT:  acc_prof_info
  //         OFF-NEXT:    event_type=11, valid_bytes=72, version=[[VERSION]],
  //         OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  //         OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //         OFF-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  //         OFF-NEXT:    line_no=[[ARR1_ENTER_LINE_NO]], end_line_no=[[ARR1_ENTER_END_LINE_NO]],
  //         OFF-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //         OFF-NEXT:  acc_other_event_info
  //         OFF-NEXT:    event_type=11, valid_bytes=24,
  //         OFF-NEXT:    parent_construct=acc_construct_[[ARR_ENTER_CONSTRUCT]],
  //         OFF-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil)
  //         OFF-NEXT:  acc_api_info
  //         OFF-NEXT:    device_api=0, valid_bytes=12,
  //         OFF-NEXT:    device_type=[[ACC_DEVICE]]

  // Parallel construct start and associated empty enter data if
  // -DDIR=DIR_DATAPAR.
  //
  // CHECK-DATAPAR-NEXT:acc_ev_compute_construct_start
  // CHECK-DATAPAR-NEXT:  acc_prof_info
  // CHECK-DATAPAR-NEXT:    event_type=16, valid_bytes=72, version=[[VERSION]],
  //  HOST-DATAPAR-NEXT:    device_type=acc_device_host, device_number=0,
  //   OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // CHECK-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // CHECK-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // CHECK-DATAPAR-NEXT:    line_no=[[KERN_LINE_NO]], end_line_no=[[KERN_END_LINE_NO]],
  // CHECK-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // CHECK-DATAPAR-NEXT:  acc_other_event_info
  // CHECK-DATAPAR-NEXT:    event_type=16, valid_bytes=24,
  // CHECK-DATAPAR-NEXT:    parent_construct=acc_construct_[[PARALLEL_CONSTRUCT]],
  // CHECK-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil)
  // CHECK-DATAPAR-NEXT:  acc_api_info
  // CHECK-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  //  HOST-DATAPAR-NEXT:    device_type=acc_device_host
  //   OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  //   OFF-DATAPAR-NEXT:acc_ev_enter_data_start
  //   OFF-DATAPAR-NEXT:  acc_prof_info
  //   OFF-DATAPAR-NEXT:    event_type=10, valid_bytes=72, version=[[VERSION]],
  //   OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  //   OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //   OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  //   OFF-DATAPAR-NEXT:    line_no=[[KERN_LINE_NO]], end_line_no=[[KERN_END_LINE_NO]],
  //   OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //   OFF-DATAPAR-NEXT:  acc_other_event_info
  //   OFF-DATAPAR-NEXT:    event_type=10, valid_bytes=24,
  //   OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[PARALLEL_CONSTRUCT]],
  //   OFF-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil)
  //   OFF-DATAPAR-NEXT:  acc_api_info
  //   OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  //   OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  //   OFF-DATAPAR-NEXT:acc_ev_enter_data_end
  //   OFF-DATAPAR-NEXT:  acc_prof_info
  //   OFF-DATAPAR-NEXT:    event_type=11, valid_bytes=72, version=[[VERSION]],
  //   OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  //   OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //   OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  //   OFF-DATAPAR-NEXT:    line_no=[[KERN_LINE_NO]], end_line_no=[[KERN_END_LINE_NO]],
  //   OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //   OFF-DATAPAR-NEXT:  acc_other_event_info
  //   OFF-DATAPAR-NEXT:    event_type=11, valid_bytes=24,
  //   OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[PARALLEL_CONSTRUCT]],
  //   OFF-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil)
  //   OFF-DATAPAR-NEXT:  acc_api_info
  //   OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  //   OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]

  // Enqueue launch.
  //
  // CHECK-HASPAR-NEXT:acc_ev_enqueue_launch_start
  // CHECK-HASPAR-NEXT:  acc_prof_info
  // CHECK-HASPAR-NEXT:    event_type=18, valid_bytes=72, version=[[VERSION]],
  //  HOST-HASPAR-NEXT:    device_type=acc_device_host, device_number=0,
  //   OFF-HASPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // CHECK-HASPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // CHECK-HASPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // CHECK-HASPAR-NEXT:    line_no=[[KERN_LINE_NO]], end_line_no=[[KERN_END_LINE_NO]],
  // CHECK-HASPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // CHECK-HASPAR-NEXT:  acc_launch_event_info
  // CHECK-HASPAR-NEXT:    event_type=18, valid_bytes=32,
  // CHECK-HASPAR-NEXT:    parent_construct=acc_construct_[[PARALLEL_CONSTRUCT]],
  // CHECK-HASPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  // CHECK-HASPAR-NEXT:    kernel_name=(nil)
  // CHECK-HASPAR-NEXT:  acc_api_info
  // CHECK-HASPAR-NEXT:    device_api=0, valid_bytes=12,
  //  HOST-HASPAR-NEXT:    device_type=acc_device_host
  //   OFF-HASPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // CHECK-HASPAR-NEXT:acc_ev_enqueue_launch_end
  // CHECK-HASPAR-NEXT:  acc_prof_info
  // CHECK-HASPAR-NEXT:    event_type=19, valid_bytes=72, version=[[VERSION]],
  //  HOST-HASPAR-NEXT:    device_type=acc_device_host, device_number=0,
  //   OFF-HASPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // CHECK-HASPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // CHECK-HASPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // CHECK-HASPAR-NEXT:    line_no=[[KERN_LINE_NO]], end_line_no=[[KERN_END_LINE_NO]],
  // CHECK-HASPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // CHECK-HASPAR-NEXT:  acc_launch_event_info
  // CHECK-HASPAR-NEXT:    event_type=19, valid_bytes=32,
  // CHECK-HASPAR-NEXT:    parent_construct=acc_construct_[[PARALLEL_CONSTRUCT]],
  // CHECK-HASPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  // CHECK-HASPAR-NEXT:    kernel_name=(nil)
  // CHECK-HASPAR-NEXT:  acc_api_info
  // CHECK-HASPAR-NEXT:    device_api=0, valid_bytes=12,
  //  HOST-HASPAR-NEXT:    device_type=acc_device_host
  //   OFF-HASPAR-NEXT:    device_type=[[ACC_DEVICE]]

#if DIR == DIR_ENTER_EXIT_DATA
  #line 20000
  #pragma acc enter data copyin(arr0, arr1[0:5])
  #line 30000
  #pragma acc exit data copyout(arr0, arr1[0:5], notPresent0) delete(notPresent1)
#elif DIR == DIR_DATA
  #line 40000
  #pragma acc data copy(arr0, arr1[0:5])
  #line 50000
  ;
#elif DIR == DIR_PAR
  #line 60000
  #pragma acc parallel copy(arr0, arr1[0:5]) num_gangs(1)
#elif DIR == DIR_DATAPAR
  #line 70000
  #pragma acc data copy(arr0)
  {
    #line 80000
    #pragma acc data copy(arr1[0:5])
    {
      // Implicit data regions have data events.
      #line 90000
      #pragma acc parallel num_gangs(1)
#else
# error undefined DIR
#endif
#if DIR == DIR_PAR || DIR == DIR_DATAPAR
      for (int j = 0; j < 2; ++j) {
        //    HOST-HASPAR-NEXT:inside: arr0=[[ARR0_HOST_PTR]], arr0[0]=10
        //    HOST-HASPAR-NEXT:inside: arr1=[[ARR1_HOST_PTR]], arr1[0]=20
        //    HOST-HASPAR-NEXT:inside: arr0=[[ARR0_HOST_PTR]], arr0[1]=11
        //    HOST-HASPAR-NEXT:inside: arr1=[[ARR1_HOST_PTR]], arr1[1]=21
        //  X86_64-HASPAR-NEXT:inside: arr0=[[ARR0_DEVICE_PTR]], arr0[0]=10
        //  X86_64-HASPAR-NEXT:inside: arr1=[[ARR1_DEVICE_PTR]], arr1[0]=20
        //  X86_64-HASPAR-NEXT:inside: arr0=[[ARR0_DEVICE_PTR]], arr0[1]=11
        //  X86_64-HASPAR-NEXT:inside: arr1=[[ARR1_DEVICE_PTR]], arr1[1]=21
        // PPC64LE-HASPAR-NEXT:inside: arr0=[[ARR0_DEVICE_PTR]], arr0[0]=10
        // PPC64LE-HASPAR-NEXT:inside: arr1=[[ARR1_DEVICE_PTR]], arr1[0]=20
        // PPC64LE-HASPAR-NEXT:inside: arr0=[[ARR0_DEVICE_PTR]], arr0[1]=11
        // PPC64LE-HASPAR-NEXT:inside: arr1=[[ARR1_DEVICE_PTR]], arr1[1]=21
        // We omit NVPTX64 here because subsequent events might trigger before
        // kernel execution due to the use of CUDA streams.
        if (!NVPTX64 || offloadDisabled) {
          printf("inside: arr0=%p, arr0[%d]=%d\n", arr0, j, arr0[j]);
          printf("inside: arr1=%p, arr1[%d]=%d\n", arr1, j, arr1[j]);
        }
      #line 100000
      }
#endif
#if DIR == DIR_DATAPAR
    #line 110000
    #pragma acc update self(arr0, notPresent0) device(arr1[0:5], notPresent1) if_present
    #line 120000
    #pragma acc update self(notPresent0) device(notPresent1) if_present
    // Due to the if(0), the following should produce no additional events.
    #pragma acc update self(arr0) device(arr1[0:5]) if_present if(0)
    #line 130000
    }
  #line 140000
  }
#endif

  // Parallel construct end and associated empty exit data if
  // -DDIR=DIR_DATAPAR.
  //
  //   OFF-DATAPAR-NEXT:acc_ev_exit_data_start
  //   OFF-DATAPAR-NEXT:  acc_prof_info
  //   OFF-DATAPAR-NEXT:    event_type=12, valid_bytes=72, version=[[VERSION]],
  //   OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  //   OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //   OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  //   OFF-DATAPAR-NEXT:    line_no=[[KERN_LINE_NO]], end_line_no=[[KERN_END_LINE_NO]],
  //   OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //   OFF-DATAPAR-NEXT:  acc_other_event_info
  //   OFF-DATAPAR-NEXT:    event_type=12, valid_bytes=24,
  //   OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[PARALLEL_CONSTRUCT]],
  //   OFF-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil)
  //   OFF-DATAPAR-NEXT:  acc_api_info
  //   OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  //   OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  //   OFF-DATAPAR-NEXT:acc_ev_exit_data_end
  //   OFF-DATAPAR-NEXT:  acc_prof_info
  //   OFF-DATAPAR-NEXT:    event_type=13, valid_bytes=72, version=[[VERSION]],
  //   OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  //   OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  //   OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  //   OFF-DATAPAR-NEXT:    line_no=[[KERN_LINE_NO]], end_line_no=[[KERN_END_LINE_NO]],
  //   OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  //   OFF-DATAPAR-NEXT:  acc_other_event_info
  //   OFF-DATAPAR-NEXT:    event_type=13, valid_bytes=24,
  //   OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[PARALLEL_CONSTRUCT]],
  //   OFF-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil)
  //   OFF-DATAPAR-NEXT:  acc_api_info
  //   OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  //   OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // CHECK-DATAPAR-NEXT:acc_ev_compute_construct_end
  // CHECK-DATAPAR-NEXT:  acc_prof_info
  // CHECK-DATAPAR-NEXT:    event_type=17, valid_bytes=72, version=[[VERSION]],
  //  HOST-DATAPAR-NEXT:    device_type=acc_device_host, device_number=0,
  //   OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // CHECK-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // CHECK-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // CHECK-DATAPAR-NEXT:    line_no=[[KERN_LINE_NO]], end_line_no=[[KERN_END_LINE_NO]],
  // CHECK-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // CHECK-DATAPAR-NEXT:  acc_other_event_info
  // CHECK-DATAPAR-NEXT:    event_type=17, valid_bytes=24,
  // CHECK-DATAPAR-NEXT:    parent_construct=acc_construct_[[PARALLEL_CONSTRUCT]],
  // CHECK-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil)
  // CHECK-DATAPAR-NEXT:  acc_api_info
  // CHECK-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  //  HOST-DATAPAR-NEXT:    device_type=acc_device_host
  //   OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]

  // Update directive events if -DDIR=DIR_DATAPAR.
  //
  // OFF-DATAPAR-NEXT:acc_ev_update_start
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=14, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-DATAPAR-NEXT:    line_no=[[UPDATE0_LINE_NO]], end_line_no=[[UPDATE0_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_other_event_info
  // OFF-DATAPAR-NEXT:    event_type=14, valid_bytes=24,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[UPDATE_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil)
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-DATAPAR-NEXT:acc_ev_enqueue_upload_start
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=20, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-DATAPAR-NEXT:    line_no=[[UPDATE0_LINE_NO]], end_line_no=[[UPDATE0_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_data_event_info
  // OFF-DATAPAR-NEXT:    event_type=20, valid_bytes=56,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[UPDATE_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  // OFF-DATAPAR-NEXT:    var_name=[[ARR1_VAR_NAME]], bytes=20,
  // OFF-DATAPAR-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-DATAPAR-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-DATAPAR-NEXT:acc_ev_enqueue_upload_end
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=21, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-DATAPAR-NEXT:    line_no=[[UPDATE0_LINE_NO]], end_line_no=[[UPDATE0_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_data_event_info
  // OFF-DATAPAR-NEXT:    event_type=21, valid_bytes=56,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[UPDATE_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  // OFF-DATAPAR-NEXT:    var_name=[[ARR1_VAR_NAME]], bytes=20,
  // OFF-DATAPAR-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-DATAPAR-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-DATAPAR-NEXT:acc_ev_enqueue_download_start
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=22, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-DATAPAR-NEXT:    line_no=[[UPDATE0_LINE_NO]], end_line_no=[[UPDATE0_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_data_event_info
  // OFF-DATAPAR-NEXT:    event_type=22, valid_bytes=56,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[UPDATE_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  // OFF-DATAPAR-NEXT:    var_name=[[ARR0_VAR_NAME]], bytes=8,
  // OFF-DATAPAR-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-DATAPAR-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-DATAPAR-NEXT:acc_ev_enqueue_download_end
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=23, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-DATAPAR-NEXT:    line_no=[[UPDATE0_LINE_NO]], end_line_no=[[UPDATE0_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_data_event_info
  // OFF-DATAPAR-NEXT:    event_type=23, valid_bytes=56,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[UPDATE_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  // OFF-DATAPAR-NEXT:    var_name=[[ARR0_VAR_NAME]], bytes=8,
  // OFF-DATAPAR-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-DATAPAR-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-DATAPAR-NEXT:acc_ev_update_end
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=15, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-DATAPAR-NEXT:    line_no=[[UPDATE0_LINE_NO]], end_line_no=[[UPDATE0_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_other_event_info
  // OFF-DATAPAR-NEXT:    event_type=15, valid_bytes=24,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[UPDATE_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil)
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-DATAPAR-NEXT:acc_ev_update_start
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=14, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-DATAPAR-NEXT:    line_no=[[UPDATE1_LINE_NO]], end_line_no=[[UPDATE1_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_other_event_info
  // OFF-DATAPAR-NEXT:    event_type=14, valid_bytes=24,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[UPDATE_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil)
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-DATAPAR-NEXT:acc_ev_update_end
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=15, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-DATAPAR-NEXT:    line_no=[[UPDATE1_LINE_NO]], end_line_no=[[UPDATE1_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_other_event_info
  // OFF-DATAPAR-NEXT:    event_type=15, valid_bytes=24,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[UPDATE_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil)
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]

  // Exit data for arr1 and then arr0 if not DATAPAR.
  //
  // OFF-NODATAPAR-NEXT:acc_ev_exit_data_start
  // OFF-NODATAPAR-NEXT:  acc_prof_info
  // OFF-NODATAPAR-NEXT:    event_type=12, valid_bytes=72, version=[[VERSION]],
  // OFF-NODATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NODATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NODATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-NODATAPAR-NEXT:    line_no=[[ARR1_EXIT_LINE_NO]], end_line_no=[[ARR1_EXIT_END_LINE_NO]],
  // OFF-NODATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NODATAPAR-NEXT:  acc_other_event_info
  // OFF-NODATAPAR-NEXT:    event_type=12, valid_bytes=24,
  // OFF-NODATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_EXIT_CONSTRUCT]],
  // OFF-NODATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil)
  // OFF-NODATAPAR-NEXT:  acc_api_info
  // OFF-NODATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NODATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NODATAPAR-NEXT:acc_ev_enqueue_download_start
  // OFF-NODATAPAR-NEXT:  acc_prof_info
  // OFF-NODATAPAR-NEXT:    event_type=22, valid_bytes=72, version=[[VERSION]],
  // OFF-NODATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NODATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NODATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-NODATAPAR-NEXT:    line_no=[[ARR1_EXIT_LINE_NO]], end_line_no=[[ARR1_EXIT_END_LINE_NO]],
  // OFF-NODATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NODATAPAR-NEXT:  acc_data_event_info
  // OFF-NODATAPAR-NEXT:    event_type=22, valid_bytes=56,
  // OFF-NODATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_EXIT_CONSTRUCT]],
  // OFF-NODATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  // OFF-NODATAPAR-NEXT:    var_name=[[ARR1_VAR_NAME]], bytes=20,
  // OFF-NODATAPAR-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-NODATAPAR-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-NODATAPAR-NEXT:  acc_api_info
  // OFF-NODATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NODATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NODATAPAR-NEXT:acc_ev_enqueue_download_end
  // OFF-NODATAPAR-NEXT:  acc_prof_info
  // OFF-NODATAPAR-NEXT:    event_type=23, valid_bytes=72, version=[[VERSION]],
  // OFF-NODATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NODATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NODATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-NODATAPAR-NEXT:    line_no=[[ARR1_EXIT_LINE_NO]], end_line_no=[[ARR1_EXIT_END_LINE_NO]],
  // OFF-NODATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NODATAPAR-NEXT:  acc_data_event_info
  // OFF-NODATAPAR-NEXT:    event_type=23, valid_bytes=56,
  // OFF-NODATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_EXIT_CONSTRUCT]],
  // OFF-NODATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  // OFF-NODATAPAR-NEXT:    var_name=[[ARR1_VAR_NAME]], bytes=20,
  // OFF-NODATAPAR-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-NODATAPAR-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-NODATAPAR-NEXT:  acc_api_info
  // OFF-NODATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NODATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NODATAPAR-NEXT:acc_ev_enqueue_download_start
  // OFF-NODATAPAR-NEXT:  acc_prof_info
  // OFF-NODATAPAR-NEXT:    event_type=22, valid_bytes=72, version=[[VERSION]],
  // OFF-NODATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NODATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NODATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-NODATAPAR-NEXT:    line_no=[[ARR0_EXIT_LINE_NO]], end_line_no=[[ARR0_EXIT_END_LINE_NO]],
  // OFF-NODATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NODATAPAR-NEXT:  acc_data_event_info
  // OFF-NODATAPAR-NEXT:    event_type=22, valid_bytes=56,
  // OFF-NODATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_EXIT_CONSTRUCT]],
  // OFF-NODATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  // OFF-NODATAPAR-NEXT:    var_name=[[ARR0_VAR_NAME]], bytes=8,
  // OFF-NODATAPAR-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-NODATAPAR-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-NODATAPAR-NEXT:  acc_api_info
  // OFF-NODATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NODATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NODATAPAR-NEXT:acc_ev_enqueue_download_end
  // OFF-NODATAPAR-NEXT:  acc_prof_info
  // OFF-NODATAPAR-NEXT:    event_type=23, valid_bytes=72, version=[[VERSION]],
  // OFF-NODATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NODATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NODATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-NODATAPAR-NEXT:    line_no=[[ARR0_EXIT_LINE_NO]], end_line_no=[[ARR0_EXIT_END_LINE_NO]],
  // OFF-NODATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NODATAPAR-NEXT:  acc_data_event_info
  // OFF-NODATAPAR-NEXT:    event_type=23, valid_bytes=56,
  // OFF-NODATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_EXIT_CONSTRUCT]],
  // OFF-NODATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  // OFF-NODATAPAR-NEXT:    var_name=[[ARR0_VAR_NAME]], bytes=8,
  // OFF-NODATAPAR-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-NODATAPAR-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-NODATAPAR-NEXT:  acc_api_info
  // OFF-NODATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NODATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NODATAPAR-NEXT:acc_ev_delete
  // OFF-NODATAPAR-NEXT:  acc_prof_info
  // OFF-NODATAPAR-NEXT:    event_type=7, valid_bytes=72, version=[[VERSION]],
  // OFF-NODATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NODATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NODATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-NODATAPAR-NEXT:    line_no=[[ARR1_EXIT_LINE_NO]], end_line_no=[[ARR1_EXIT_END_LINE_NO]],
  // OFF-NODATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NODATAPAR-NEXT:  acc_data_event_info
  // OFF-NODATAPAR-NEXT:    event_type=7, valid_bytes=56,
  // OFF-NODATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_EXIT_CONSTRUCT]],
  // OFF-NODATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  // OFF-NODATAPAR-NEXT:    var_name=[[ARR1_VAR_NAME]], bytes=20,
  // OFF-NODATAPAR-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-NODATAPAR-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-NODATAPAR-NEXT:  acc_api_info
  // OFF-NODATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NODATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NODATAPAR-NEXT:acc_ev_free
  // OFF-NODATAPAR-NEXT:  acc_prof_info
  // OFF-NODATAPAR-NEXT:    event_type=9, valid_bytes=72, version=[[VERSION]],
  // OFF-NODATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NODATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NODATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-NODATAPAR-NEXT:    line_no=[[ARR1_EXIT_LINE_NO]], end_line_no=[[ARR1_EXIT_END_LINE_NO]],
  // OFF-NODATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NODATAPAR-NEXT:  acc_data_event_info
  // OFF-NODATAPAR-NEXT:    event_type=9, valid_bytes=56,
  // OFF-NODATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_EXIT_CONSTRUCT]],
  // OFF-NODATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  // OFF-NODATAPAR-NEXT:    var_name=[[ARR1_VAR_NAME]], bytes=20,
  // OFF-NODATAPAR-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-NODATAPAR-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-NODATAPAR-NEXT:  acc_api_info
  // OFF-NODATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NODATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NODATAPAR-NEXT:acc_ev_delete
  // OFF-NODATAPAR-NEXT:  acc_prof_info
  // OFF-NODATAPAR-NEXT:    event_type=7, valid_bytes=72, version=[[VERSION]],
  // OFF-NODATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NODATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NODATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-NODATAPAR-NEXT:    line_no=[[ARR0_EXIT_LINE_NO]], end_line_no=[[ARR0_EXIT_END_LINE_NO]],
  // OFF-NODATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NODATAPAR-NEXT:  acc_data_event_info
  // OFF-NODATAPAR-NEXT:    event_type=7, valid_bytes=56,
  // OFF-NODATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_EXIT_CONSTRUCT]],
  // OFF-NODATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  // OFF-NODATAPAR-NEXT:    var_name=[[ARR0_VAR_NAME]], bytes=8,
  // OFF-NODATAPAR-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-NODATAPAR-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-NODATAPAR-NEXT:  acc_api_info
  // OFF-NODATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NODATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NODATAPAR-NEXT:acc_ev_free
  // OFF-NODATAPAR-NEXT:  acc_prof_info
  // OFF-NODATAPAR-NEXT:    event_type=9, valid_bytes=72, version=[[VERSION]],
  // OFF-NODATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NODATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NODATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-NODATAPAR-NEXT:    line_no=[[ARR0_EXIT_LINE_NO]], end_line_no=[[ARR0_EXIT_END_LINE_NO]],
  // OFF-NODATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NODATAPAR-NEXT:  acc_data_event_info
  // OFF-NODATAPAR-NEXT:    event_type=9, valid_bytes=56,
  // OFF-NODATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_EXIT_CONSTRUCT]],
  // OFF-NODATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  // OFF-NODATAPAR-NEXT:    var_name=[[ARR0_VAR_NAME]], bytes=8,
  // OFF-NODATAPAR-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-NODATAPAR-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-NODATAPAR-NEXT:  acc_api_info
  // OFF-NODATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NODATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-NODATAPAR-NEXT:acc_ev_exit_data_end
  // OFF-NODATAPAR-NEXT:  acc_prof_info
  // OFF-NODATAPAR-NEXT:    event_type=13, valid_bytes=72, version=[[VERSION]],
  // OFF-NODATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-NODATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-NODATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-NODATAPAR-NEXT:    line_no=[[ARR0_EXIT_LINE_NO]], end_line_no=[[ARR0_EXIT_END_LINE_NO]],
  // OFF-NODATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-NODATAPAR-NEXT:  acc_other_event_info
  // OFF-NODATAPAR-NEXT:    event_type=13, valid_bytes=24,
  // OFF-NODATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_EXIT_CONSTRUCT]],
  // OFF-NODATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil)
  // OFF-NODATAPAR-NEXT:  acc_api_info
  // OFF-NODATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-NODATAPAR-NEXT:    device_type=[[ACC_DEVICE]]

  // Exit data for arr1 if DATAPAR.
  //
  // OFF-DATAPAR-NEXT:acc_ev_exit_data_start
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=12, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-DATAPAR-NEXT:    line_no=[[ARR1_EXIT_LINE_NO]], end_line_no=[[ARR1_EXIT_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_other_event_info
  // OFF-DATAPAR-NEXT:    event_type=12, valid_bytes=24,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_EXIT_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil)
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-DATAPAR-NEXT:acc_ev_enqueue_download_start
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=22, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-DATAPAR-NEXT:    line_no=[[ARR1_EXIT_LINE_NO]], end_line_no=[[ARR1_EXIT_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_data_event_info
  // OFF-DATAPAR-NEXT:    event_type=22, valid_bytes=56,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_EXIT_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  // OFF-DATAPAR-NEXT:    var_name=[[ARR1_VAR_NAME]], bytes=20,
  // OFF-DATAPAR-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-DATAPAR-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-DATAPAR-NEXT:acc_ev_enqueue_download_end
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=23, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-DATAPAR-NEXT:    line_no=[[ARR1_EXIT_LINE_NO]], end_line_no=[[ARR1_EXIT_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_data_event_info
  // OFF-DATAPAR-NEXT:    event_type=23, valid_bytes=56,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_EXIT_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  // OFF-DATAPAR-NEXT:    var_name=[[ARR1_VAR_NAME]], bytes=20,
  // OFF-DATAPAR-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-DATAPAR-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-DATAPAR-NEXT:acc_ev_delete
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=7, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-DATAPAR-NEXT:    line_no=[[ARR1_EXIT_LINE_NO]], end_line_no=[[ARR1_EXIT_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_data_event_info
  // OFF-DATAPAR-NEXT:    event_type=7, valid_bytes=56,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_EXIT_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  // OFF-DATAPAR-NEXT:    var_name=[[ARR1_VAR_NAME]], bytes=20,
  // OFF-DATAPAR-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-DATAPAR-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-DATAPAR-NEXT:acc_ev_free
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=9, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-DATAPAR-NEXT:    line_no=[[ARR1_EXIT_LINE_NO]], end_line_no=[[ARR1_EXIT_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_data_event_info
  // OFF-DATAPAR-NEXT:    event_type=9, valid_bytes=56,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_EXIT_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  // OFF-DATAPAR-NEXT:    var_name=[[ARR1_VAR_NAME]], bytes=20,
  // OFF-DATAPAR-NEXT:    host_ptr=[[ARR1_HOST_PTR]],
  // OFF-DATAPAR-NEXT:    device_ptr=[[ARR1_DEVICE_PTR]]
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-DATAPAR-NEXT:acc_ev_exit_data_end
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=13, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-DATAPAR-NEXT:    line_no=[[ARR1_EXIT_LINE_NO]], end_line_no=[[ARR1_EXIT_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_other_event_info
  // OFF-DATAPAR-NEXT:    event_type=13, valid_bytes=24,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[DATA_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil)
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]

  // Exit data for arr0 if DATAPAR.
  //
  // OFF-DATAPAR-NEXT:acc_ev_exit_data_start
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=12, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-DATAPAR-NEXT:    line_no=[[ARR0_EXIT_LINE_NO]], end_line_no=[[ARR0_EXIT_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_other_event_info
  // OFF-DATAPAR-NEXT:    event_type=12, valid_bytes=24,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[DATA_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil)
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-DATAPAR-NEXT:acc_ev_enqueue_download_start
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=22, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-DATAPAR-NEXT:    line_no=[[ARR0_EXIT_LINE_NO]], end_line_no=[[ARR0_EXIT_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_data_event_info
  // OFF-DATAPAR-NEXT:    event_type=22, valid_bytes=56,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_EXIT_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  // OFF-DATAPAR-NEXT:    var_name=[[ARR0_VAR_NAME]], bytes=8,
  // OFF-DATAPAR-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-DATAPAR-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-DATAPAR-NEXT:acc_ev_enqueue_download_end
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=23, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-DATAPAR-NEXT:    line_no=[[ARR0_EXIT_LINE_NO]], end_line_no=[[ARR0_EXIT_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_data_event_info
  // OFF-DATAPAR-NEXT:    event_type=23, valid_bytes=56,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_EXIT_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  // OFF-DATAPAR-NEXT:    var_name=[[ARR0_VAR_NAME]], bytes=8,
  // OFF-DATAPAR-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-DATAPAR-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-DATAPAR-NEXT:acc_ev_delete
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=7, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-DATAPAR-NEXT:    line_no=[[ARR0_EXIT_LINE_NO]], end_line_no=[[ARR0_EXIT_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_data_event_info
  // OFF-DATAPAR-NEXT:    event_type=7, valid_bytes=56,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_EXIT_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  // OFF-DATAPAR-NEXT:    var_name=[[ARR0_VAR_NAME]], bytes=8,
  // OFF-DATAPAR-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-DATAPAR-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-DATAPAR-NEXT:acc_ev_free
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=9, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-DATAPAR-NEXT:    line_no=[[ARR0_EXIT_LINE_NO]], end_line_no=[[ARR0_EXIT_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_data_event_info
  // OFF-DATAPAR-NEXT:    event_type=9, valid_bytes=56,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_EXIT_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil),
  // OFF-DATAPAR-NEXT:    var_name=[[ARR0_VAR_NAME]], bytes=8,
  // OFF-DATAPAR-NEXT:    host_ptr=[[ARR0_HOST_PTR]],
  // OFF-DATAPAR-NEXT:    device_ptr=[[ARR0_DEVICE_PTR]]
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]
  // OFF-DATAPAR-NEXT:acc_ev_exit_data_end
  // OFF-DATAPAR-NEXT:  acc_prof_info
  // OFF-DATAPAR-NEXT:    event_type=13, valid_bytes=72, version=[[VERSION]],
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // OFF-DATAPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // OFF-DATAPAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // OFF-DATAPAR-NEXT:    line_no=[[ARR0_EXIT_LINE_NO]], end_line_no=[[ARR0_EXIT_END_LINE_NO]],
  // OFF-DATAPAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // OFF-DATAPAR-NEXT:  acc_other_event_info
  // OFF-DATAPAR-NEXT:    event_type=13, valid_bytes=24,
  // OFF-DATAPAR-NEXT:    parent_construct=acc_construct_[[ARR_EXIT_CONSTRUCT]],
  // OFF-DATAPAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil)
  // OFF-DATAPAR-NEXT:  acc_api_info
  // OFF-DATAPAR-NEXT:    device_api=0, valid_bytes=12,
  // OFF-DATAPAR-NEXT:    device_type=[[ACC_DEVICE]]

  // Parallel construct end if -DDIR=DIR_PAR.
  //
  // CHECK-PAR-NEXT:acc_ev_compute_construct_end
  // CHECK-PAR-NEXT:  acc_prof_info
  // CHECK-PAR-NEXT:    event_type=17, valid_bytes=72, version=[[VERSION]],
  //  HOST-PAR-NEXT:    device_type=acc_device_host, device_number=0,
  //   OFF-PAR-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
  // CHECK-PAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
  // CHECK-PAR-NEXT:    src_file=[[SRC_FILE]], func_name=[[FUNC_NAME]],
  // CHECK-PAR-NEXT:    line_no=[[KERN_LINE_NO]], end_line_no=[[KERN_END_LINE_NO]],
  // CHECK-PAR-NEXT:    func_line_no=[[FUNC_LINE_NO]], func_end_line_no=[[FUNC_END_LINE_NO]]
  // CHECK-PAR-NEXT:  acc_other_event_info
  // CHECK-PAR-NEXT:    event_type=17, valid_bytes=24,
  // CHECK-PAR-NEXT:    parent_construct=acc_construct_[[PARALLEL_CONSTRUCT]],
  // CHECK-PAR-NEXT:    implicit=[[IMPLICIT_FOR_DIRECTIVE]], tool_info=(nil)
  // CHECK-PAR-NEXT:  acc_api_info
  // CHECK-PAR-NEXT:    device_api=0, valid_bytes=12,
  //  HOST-PAR-NEXT:    device_type=acc_device_host
  //   OFF-PAR-NEXT:    device_type=[[ACC_DEVICE]]

  // CHECK-NEXT:after kernel
  printf("after kernel\n");

  return 0;
#line 150000
}

// Device shutdown.
//
// OFF-NEXT:acc_ev_device_shutdown_start
// OFF-NEXT:  acc_prof_info
// OFF-NEXT:    event_type=3, valid_bytes=72, version=[[VERSION]],
// OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
// OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
// OFF-NEXT:    src_file=(null), func_name=(null),
// OFF-NEXT:    line_no=0, end_line_no=0,
// OFF-NEXT:    func_line_no=0, func_end_line_no=0
// OFF-NEXT:  acc_other_event_info
// OFF-NEXT:    event_type=3, valid_bytes=24,
// OFF-NEXT:    parent_construct=acc_construct_runtime_api,
// OFF-NEXT:    implicit=1, tool_info=(nil)
// OFF-NEXT:  acc_api_info
// OFF-NEXT:    device_api=0, valid_bytes=12,
// OFF-NEXT:    device_type=[[ACC_DEVICE]]
// OFF-NEXT:acc_ev_device_shutdown_end
// OFF-NEXT:  acc_prof_info
// OFF-NEXT:    event_type=4, valid_bytes=72, version=[[VERSION]],
// OFF-NEXT:    device_type=[[ACC_DEVICE]], device_number=[[OFF_DEV]],
// OFF-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
// OFF-NEXT:    src_file=(null), func_name=(null),
// OFF-NEXT:    line_no=0, end_line_no=0,
// OFF-NEXT:    func_line_no=0, func_end_line_no=0
// OFF-NEXT:  acc_other_event_info
// OFF-NEXT:    event_type=4, valid_bytes=24,
// OFF-NEXT:    parent_construct=acc_construct_runtime_api,
// OFF-NEXT:    implicit=1, tool_info=(nil)
// OFF-NEXT:  acc_api_info
// OFF-NEXT:    device_api=0, valid_bytes=12,
// OFF-NEXT:    device_type=[[ACC_DEVICE]]

// Runtime shutdown.
//
// HOST-BEFORE-ENV-HASPAR-NEXT:acc_ev_runtime_shutdown
// HOST-BEFORE-ENV-HASPAR-NEXT:  acc_prof_info
// HOST-BEFORE-ENV-HASPAR-NEXT:    event_type=5, valid_bytes=72, version=[[VERSION]],
// HOST-BEFORE-ENV-HASPAR-NEXT:    device_type=acc_device_host, device_number=0,
// HOST-BEFORE-ENV-HASPAR-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
// HOST-BEFORE-ENV-HASPAR-NEXT:    src_file=(null), func_name=(null),
// HOST-BEFORE-ENV-HASPAR-NEXT:    line_no=0, end_line_no=0,
// HOST-BEFORE-ENV-HASPAR-NEXT:    func_line_no=0, func_end_line_no=0
// HOST-BEFORE-ENV-HASPAR-NEXT:  acc_other_event_info
// HOST-BEFORE-ENV-HASPAR-NEXT:    event_type=5, valid_bytes=24,
// HOST-BEFORE-ENV-HASPAR-NEXT:    parent_construct=acc_construct_runtime_api,
// HOST-BEFORE-ENV-HASPAR-NEXT:    implicit=1, tool_info=(nil)
// HOST-BEFORE-ENV-HASPAR-NEXT:  acc_api_info
// HOST-BEFORE-ENV-HASPAR-NEXT:    device_api=0, valid_bytes=12,
// HOST-BEFORE-ENV-HASPAR-NEXT:    device_type=acc_device_host
//
// OFF-BEFORE-ENV-NEXT:acc_ev_runtime_shutdown
// OFF-BEFORE-ENV-NEXT:  acc_prof_info
// OFF-BEFORE-ENV-NEXT:    event_type=5, valid_bytes=72, version=[[VERSION]],
// OFF-BEFORE-ENV-NEXT:    device_type=acc_device_host, device_number=0,
// OFF-BEFORE-ENV-NEXT:    thread_id=[[THREAD_ID]], async=acc_async_sync, async_queue=[[ASYNC_QUEUE]],
// OFF-BEFORE-ENV-NEXT:    src_file=(null), func_name=(null),
// OFF-BEFORE-ENV-NEXT:    line_no=0, end_line_no=0,
// OFF-BEFORE-ENV-NEXT:    func_line_no=0, func_end_line_no=0
// OFF-BEFORE-ENV-NEXT:  acc_other_event_info
// OFF-BEFORE-ENV-NEXT:    event_type=5, valid_bytes=24,
// OFF-BEFORE-ENV-NEXT:    parent_construct=acc_construct_runtime_api,
// OFF-BEFORE-ENV-NEXT:    implicit=1, tool_info=(nil)
// OFF-BEFORE-ENV-NEXT:  acc_api_info
// OFF-BEFORE-ENV-NEXT:    device_api=0, valid_bytes=12,
// OFF-BEFORE-ENV-NEXT:    device_type=acc_device_host

// CHECK-NOT:{{.}}
