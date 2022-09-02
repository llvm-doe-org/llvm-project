// Check that all callbacks are dispatched in the correct order with the
// correct data for a simple program.

// Parameters used throughout substitutions below.
//
// DEFINE: %{DIR_CFLAGS} =
// DEFINE: %{DIR_FC1} =
// DEFINE: %{DIR_FC2} =
// DEFINE: %{DIR_FC3} =
// DEFINE: %{ARR_ENTER_CONSTRUCT} =
// DEFINE: %{ARR_EXIT_CONSTRUCT} =
// DEFINE: %{ARR0_ENTER_LINE_NO} =
// DEFINE: %{ARR0_ENTER_END_LINE_NO} =
// DEFINE: %{ARR0_EXIT_LINE_NO} =
// DEFINE: %{ARR0_EXIT_END_LINE_NO} =
// DEFINE: %{ARR1_ENTER_LINE_NO} =
// DEFINE: %{ARR1_ENTER_END_LINE_NO} =
// DEFINE: %{ARR1_EXIT_LINE_NO} =
// DEFINE: %{ARR1_EXIT_END_LINE_NO} =
// DEFINE: %{KERN_LINE_NO} =
// DEFINE: %{KERN_END_LINE_NO} =
// DEFINE: %{UPDATE0_LINE_NO} =
// DEFINE: %{UPDATE1_LINE_NO} =

// DEFINE: %{check-run-env-start}( EXE %, RUN_ENV %, ENV_FC %) =               \
// DEFINE:   %{RUN_ENV} %{EXE} > %t.out 2> %t.err &&                           \
// DEFINE:   FileCheck -input-file %t.err %s                                   \
// DEFINE:     -allow-empty -check-prefixes=ERR &&                             \
// DEFINE:   FileCheck -input-file %t.out %s                                   \
// DEFINE:     -match-full-lines -strict-whitespace                            \
// DEFINE:     -check-prefixes=CHECK,CHECK-%{DIR_FC1},CHECK-%{DIR_FC2}         \
// DEFINE:     -check-prefixes=CHECK-%{DIR_FC3},%{ENV_FC}                      \
// DEFINE:     -DACC_DEVICE=acc_device_%dev-type-0-acc                         \
// DEFINE:     -DVERSION=%acc-version                                          \
// DEFINE:     -DOFF_DEV=0 -DTHREAD_ID=0 -DASYNC_QUEUE=-1

// DEFINE: %{check-run-env-dbg}( EXE %, SRC_FILE %, RUN_ENV %, ENV_FC %) =     \
// DEFINE:   %{check-run-env-start}( %{EXE} %, %{RUN_ENV} %, %{ENV_FC} %)      \
// DEFINE:     -DARR_ENTER_CONSTRUCT=%{ARR_ENTER_CONSTRUCT}                    \
// DEFINE:     -DARR_EXIT_CONSTRUCT=%{ARR_EXIT_CONSTRUCT}                      \
// DEFINE:     -DDATA_CONSTRUCT=data                                           \
// DEFINE:     -DPARALLEL_CONSTRUCT=parallel                                   \
// DEFINE:     -DUPDATE_CONSTRUCT=update                                       \
// DEFINE:     -DIMPLICIT_FOR_DIRECTIVE=0                                      \
// DEFINE:     -DSRC_FILE='%{SRC_FILE}'                                        \
// DEFINE:     -DFUNC_NAME=main                                                \
// DEFINE:     -DARR0_ENTER_LINE_NO=%{ARR0_ENTER_LINE_NO}                      \
// DEFINE:     -DARR0_ENTER_END_LINE_NO=%{ARR0_ENTER_END_LINE_NO}              \
// DEFINE:     -DARR0_EXIT_LINE_NO=%{ARR0_EXIT_LINE_NO}                        \
// DEFINE:     -DARR0_EXIT_END_LINE_NO=%{ARR0_EXIT_END_LINE_NO}                \
// DEFINE:     -DARR1_ENTER_LINE_NO=%{ARR1_ENTER_LINE_NO}                      \
// DEFINE:     -DARR1_ENTER_END_LINE_NO=%{ARR1_ENTER_END_LINE_NO}              \
// DEFINE:     -DARR1_EXIT_LINE_NO=%{ARR1_EXIT_LINE_NO}                        \
// DEFINE:     -DARR1_EXIT_END_LINE_NO=%{ARR1_EXIT_END_LINE_NO}                \
// DEFINE:     -DKERN_LINE_NO=%{KERN_LINE_NO}                                  \
// DEFINE:     -DKERN_END_LINE_NO=%{KERN_END_LINE_NO}                          \
// DEFINE:     -DUPDATE0_LINE_NO=%{UPDATE0_LINE_NO}                            \
// DEFINE:     -DUPDATE1_LINE_NO=%{UPDATE1_LINE_NO}                            \
// DEFINE:     -DFUNC_LINE_NO=10000                                            \
// DEFINE:     -DFUNC_END_LINE_NO=150000                                       \
// DEFINE:     -DARR0_VAR_NAME=arr0                                            \
// DEFINE:     -DARR1_VAR_NAME='arr1[0:5]'             

// DEFINE: %{check-run-env-nodbg}(EXE %, SRC_FILE %, RUN_ENV %, ENV_FC %) =    \
// DEFINE:   %{check-run-env-start}( %{EXE} %, %{RUN_ENV} %, %{ENV_FC} %)      \
// DEFINE:     -DARR_ENTER_CONSTRUCT=runtime_api                               \
// DEFINE:     -DARR_EXIT_CONSTRUCT=runtime_api                                \
// DEFINE:     -DDATA_CONSTRUCT=runtime_api                                    \
// DEFINE:     -DPARALLEL_CONSTRUCT=runtime_api                                \
// DEFINE:     -DUPDATE_CONSTRUCT=runtime_api                                  \
// DEFINE:     -DIMPLICIT_FOR_DIRECTIVE=1                                      \
//             # FIXME: These should be nullified instead of 'unknown' in order
//             # to distinguish from actual files or functions by that name.
//             # The use of 'unknown' is inherited from upstream LLVM's OpenMP
//             # implementation, which uses 'unknown' in many cases of a default
//             # ident_t.
// DEFINE:     -DSRC_FILE=unknown                                              \
// DEFINE:     -DFUNC_NAME=unknown                                             \
// DEFINE:     -DARR0_ENTER_LINE_NO=0                                          \
// DEFINE:     -DARR0_ENTER_END_LINE_NO=0                                      \
// DEFINE:     -DARR0_EXIT_LINE_NO=0                                           \
// DEFINE:     -DARR0_EXIT_END_LINE_NO=0                                       \
// DEFINE:     -DARR1_ENTER_LINE_NO=0                                          \
// DEFINE:     -DARR1_ENTER_END_LINE_NO=0                                      \
// DEFINE:     -DARR1_EXIT_LINE_NO=0                                           \
// DEFINE:     -DARR1_EXIT_END_LINE_NO=0                                       \
// DEFINE:     -DKERN_LINE_NO=0                                                \
// DEFINE:     -DKERN_END_LINE_NO=0                                            \
// DEFINE:     -DUPDATE0_LINE_NO=0                                             \
// DEFINE:     -DUPDATE1_LINE_NO=0                                             \
// DEFINE:     -DFUNC_LINE_NO=0                                                \
// DEFINE:     -DFUNC_END_LINE_NO=0                                            \
// DEFINE:     -DARR0_VAR_NAME='(null)'                                        \
// DEFINE:     -DARR1_VAR_NAME='(null)'

// Check with and without offloading at run time.  This is important because
// some runtime calls that must be instrumented with some callback data are not
// exercised in both cases.  Also check the case when host is selected but there
// are non-host devices and offload is not disabled.  That affects the OpenMP
// device number for host and thus might inadvertently affect profiling data if
// the implementation has a bug.
//
// DEFINE: %{check-run-envs}( CHECK_RUN_ENV %, EXE %, SRC_FILE %) = \
//                             EXE       SRC_FILE       RUN_ENV                            ENV_FC
// DEFINE:   %{CHECK_RUN_ENV}( %{EXE} %, %{SRC_FILE} %,                                 %, %if-host<HOST|OFF>,%if-host<HOST|OFF>-%{DIR_FC1},%if-host<HOST|OFF>-%{DIR_FC2},%if-host<HOST|OFF>-%{DIR_FC3},TGT-%dev-type-0-omp,TGT-%dev-type-0-omp-%{DIR_FC1},TGT-%dev-type-0-omp-%{DIR_FC2},TGT-%dev-type-0-omp-%{DIR_FC3},%if-host<HOST|OFF>-BEFORE-ENV,%if-host<HOST|OFF>-BEFORE-ENV-%{DIR_FC2} %) && \
// DEFINE:   %{CHECK_RUN_ENV}( %{EXE} %, %{SRC_FILE} %, env OMP_TARGET_OFFLOAD=disabled %, HOST,HOST-%{DIR_FC1},HOST-%{DIR_FC2},%if-host<HOST|OFF>-BEFORE-ENV,%if-host<HOST|OFF>-BEFORE-ENV-%{DIR_FC2} %) && \
// DEFINE:   %{CHECK_RUN_ENV}( %{EXE} %, %{SRC_FILE} %, env ACC_DEVICE_TYPE=host        %, HOST,HOST-%{DIR_FC1},HOST-%{DIR_FC2},%if-host<HOST|OFF>-BEFORE-ENV,%if-host<HOST|OFF>-BEFORE-ENV-%{DIR_FC2} %)

// Check both traditional compilation mode and source-to-source mode followed by
// OpenMP compilation.  This is important because, in the latter case, some
// profiling data that depends on OMPT extensions is currently available only
// when debug info is turned on.
//
// DEFINE: %{check-dir} =                                                                 \
// DEFINE:   %clang-acc-prt-omp -DTGT_%dev-type-0-omp %{DIR_CFLAGS} %s > %t-omp.c &&      \
// DEFINE:   %clang-omp -DTGT_%dev-type-0-omp %{DIR_CFLAGS} -I%S %t-omp.c                 \
// DEFINE:     -o %t-omp.exe -gline-tables-only &&                                        \
// DEFINE:   %clang-omp -DTGT_%dev-type-0-omp %{DIR_CFLAGS} -I%S %t-omp.c                 \
// DEFINE:     -o %t-omp-nodbg.exe &&                                                     \
// DEFINE:   %clang-acc -DTGT_%dev-type-0-omp %{DIR_CFLAGS} -o %t-acc.exe %s &&           \
//                              CHECK_RUN_ENV             EXE                 SRC_FILE
// DEFINE:   %{check-run-envs}( %{check-run-env-dbg}   %, %t-omp.exe       %, %t-omp.c %) && \
// DEFINE:   %{check-run-envs}( %{check-run-env-nodbg} %, %t-omp-nodbg.exe %, unknown  %) && \
// DEFINE:   %{check-run-envs}( %{check-run-env-dbg}   %, %t-acc.exe       %, %s       %)

// An enter data and exit data directive pair.
// REDEFINE: %{DIR_CFLAGS}=-DDIR=DIR_ENTER_EXIT_DATA 
// REDEFINE: %{DIR_FC1}=DATA
// REDEFINE: %{DIR_FC2}=NOPAR
// REDEFINE: %{DIR_FC3}=NODATAPAR
// REDEFINE: %{ARR_ENTER_CONSTRUCT}=enter_data
// REDEFINE: %{ARR_EXIT_CONSTRUCT}=exit_data
// REDEFINE: %{ARR0_ENTER_LINE_NO}=20000
// REDEFINE: %{ARR0_ENTER_END_LINE_NO}=20000
// REDEFINE: %{ARR0_EXIT_LINE_NO}=30000 
// REDEFINE: %{ARR0_EXIT_END_LINE_NO}=30000
// REDEFINE: %{ARR1_ENTER_LINE_NO}=20000
// REDEFINE: %{ARR1_ENTER_END_LINE_NO}=20000
// REDEFINE: %{ARR1_EXIT_LINE_NO}=30000
// REDEFINE: %{ARR1_EXIT_END_LINE_NO}=30000
// REDEFINE: %{KERN_LINE_NO}=
// REDEFINE: %{KERN_END_LINE_NO}=
// REDEFINE: %{UPDATE0_LINE_NO}=
// REDEFINE: %{UPDATE1_LINE_NO}=
// RUN: %{check-dir}

// A data construct by itself.
// REDEFINE: %{DIR_CFLAGS}=-DDIR=DIR_DATA
// REDEFINE: %{DIR_FC1}=DATA
// REDEFINE: %{DIR_FC2}=NOPAR
// REDEFINE: %{DIR_FC3}=NODATAPAR
// REDEFINE: %{ARR_ENTER_CONSTRUCT}=data
// REDEFINE: %{ARR_EXIT_CONSTRUCT}=data
// REDEFINE: %{ARR0_ENTER_LINE_NO}=40000
// REDEFINE: %{ARR0_ENTER_END_LINE_NO}=50000
// REDEFINE: %{ARR0_EXIT_LINE_NO}=40000
// REDEFINE: %{ARR0_EXIT_END_LINE_NO}=50000
// REDEFINE: %{ARR1_ENTER_LINE_NO}=40000
// REDEFINE: %{ARR1_ENTER_END_LINE_NO}=50000
// REDEFINE: %{ARR1_EXIT_LINE_NO}=40000
// REDEFINE: %{ARR1_EXIT_END_LINE_NO}=50000
// REDEFINE: %{KERN_LINE_NO}=
// REDEFINE: %{KERN_END_LINE_NO}=
// REDEFINE: %{UPDATE0_LINE_NO}=
// REDEFINE: %{UPDATE1_LINE_NO}=
// RUN: %{check-dir}

// A parallel construct by itself.
// REDEFINE: %{DIR_CFLAGS}=-DDIR=DIR_PAR
// REDEFINE: %{DIR_FC1}=PAR
// REDEFINE: %{DIR_FC2}=HASPAR
// REDEFINE: %{DIR_FC3}=NODATAPAR
// REDEFINE: %{ARR_ENTER_CONSTRUCT}=parallel
// REDEFINE: %{ARR_EXIT_CONSTRUCT}=parallel
// REDEFINE: %{ARR0_ENTER_LINE_NO}=60000
// REDEFINE: %{ARR0_ENTER_END_LINE_NO}=100000
// REDEFINE: %{ARR0_EXIT_LINE_NO}=60000
// REDEFINE: %{ARR0_EXIT_END_LINE_NO}=100000
// REDEFINE: %{ARR1_ENTER_LINE_NO}=60000
// REDEFINE: %{ARR1_ENTER_END_LINE_NO}=100000
// REDEFINE: %{ARR1_EXIT_LINE_NO}=60000
// REDEFINE: %{ARR1_EXIT_END_LINE_NO}=100000
// REDEFINE: %{KERN_LINE_NO}=60000
// REDEFINE: %{KERN_END_LINE_NO}=100000
// REDEFINE: %{UPDATE0_LINE_NO}=
// REDEFINE: %{UPDATE1_LINE_NO}=
// RUN: %{check-dir}

// A parallel construct and update directive nested within data constructs.
// REDEFINE: %{DIR_CFLAGS}=-DDIR=DIR_DATAPAR
// REDEFINE: %{DIR_FC1}=DATAPAR
// REDEFINE: %{DIR_FC2}=HASPAR
// REDEFINE: %{DIR_FC3}=HASDATAPAR
// REDEFINE: %{ARR_ENTER_CONSTRUCT}=data
// REDEFINE: %{ARR_EXIT_CONSTRUCT}=data
// REDEFINE: %{ARR0_ENTER_LINE_NO}=70000
// REDEFINE: %{ARR0_ENTER_END_LINE_NO}=140000
// REDEFINE: %{ARR0_EXIT_LINE_NO}=70000
// REDEFINE: %{ARR0_EXIT_END_LINE_NO}=140000
// REDEFINE: %{ARR1_ENTER_LINE_NO}=80000
// REDEFINE: %{ARR1_ENTER_END_LINE_NO}=130000
// REDEFINE: %{ARR1_EXIT_LINE_NO}=80000
// REDEFINE: %{ARR1_EXIT_END_LINE_NO}=130000
// REDEFINE: %{KERN_LINE_NO}=90000
// REDEFINE: %{KERN_END_LINE_NO}=100000
// REDEFINE: %{UPDATE0_LINE_NO}=110000
// REDEFINE: %{UPDATE1_LINE_NO}=120000
// RUN: %{check-dir}

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

#ifndef TGT_nvptx64
# define TGT_nvptx64 0
#endif

#ifndef TGT_amdgcn
# define TGT_amdgcn 0
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

#if TGT_amdgcn
  // Compiled as host printf.
  int (*printfPtr)(const char *, int *, int, int) =
      (int(*)(const char *, int *, int, int))printf;
#endif

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
        //        HOST-HASPAR-NEXT:inside: arr0=[[ARR0_HOST_PTR]], arr0[0]=10
        //        HOST-HASPAR-NEXT:inside: arr1=[[ARR1_HOST_PTR]], arr1[0]=20
        //        HOST-HASPAR-NEXT:inside: arr0=[[ARR0_HOST_PTR]], arr0[1]=11
        //        HOST-HASPAR-NEXT:inside: arr1=[[ARR1_HOST_PTR]], arr1[1]=21
        //  TGT-x86_64-HASPAR-NEXT:inside: arr0=[[ARR0_DEVICE_PTR]], arr0[0]=10
        //  TGT-x86_64-HASPAR-NEXT:inside: arr1=[[ARR1_DEVICE_PTR]], arr1[0]=20
        //  TGT-x86_64-HASPAR-NEXT:inside: arr0=[[ARR0_DEVICE_PTR]], arr0[1]=11
        //  TGT-x86_64-HASPAR-NEXT:inside: arr1=[[ARR1_DEVICE_PTR]], arr1[1]=21
        // TGT-ppc64le-HASPAR-NEXT:inside: arr0=[[ARR0_DEVICE_PTR]], arr0[0]=10
        // TGT-ppc64le-HASPAR-NEXT:inside: arr1=[[ARR1_DEVICE_PTR]], arr1[0]=20
        // TGT-ppc64le-HASPAR-NEXT:inside: arr0=[[ARR0_DEVICE_PTR]], arr0[1]=11
        // TGT-ppc64le-HASPAR-NEXT:inside: arr1=[[ARR1_DEVICE_PTR]], arr1[1]=21
        //
        // We omit nvptx64 here because subsequent events might trigger before
        // kernel execution due to the use of CUDA streams.
        //
        // FIXME: We omit amdgcn here because it doesn't support target printf
        // yet.  The ugly printfPtr hack is so that, in the case of amdgcn, it
        // only tries to link a host printf, which we need if offload is
        // disabled.
#if !TGT_amdgcn
        // Compiled as host and target printf.
# define printfPtr printf
#endif
        if ((!TGT_nvptx64 && !TGT_amdgcn) || offloadDisabled) {
          printfPtr("inside: arr0=%p, arr0[%d]=%d\n", arr0, j, arr0[j]);
          printfPtr("inside: arr1=%p, arr1[%d]=%d\n", arr1, j, arr1[j]);
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
