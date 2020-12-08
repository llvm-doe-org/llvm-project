// RUN: %libomptarget-compile-aarch64-unknown-linux-gnu -fopenmp-version=51
// RUN: %libomptarget-run-aarch64-unknown-linux-gnu 2>&1 \
// RUN: | %fcheck-aarch64-unknown-linux-gnu -match-full-lines

// RUN: %libomptarget-compile-powerpc64-ibm-linux-gnu -fopenmp-version=51
// RUN: %libomptarget-run-powerpc64-ibm-linux-gnu 2>&1 \
// RUN: | %fcheck-powerpc64-ibm-linux-gnu -match-full-lines

// RUN: %libomptarget-compile-powerpc64le-ibm-linux-gnu -fopenmp-version=51
// RUN: %libomptarget-run-powerpc64le-ibm-linux-gnu 2>&1 \
// RUN: | %fcheck-powerpc64le-ibm-linux-gnu -match-full-lines

// RUN: %libomptarget-compile-x86_64-pc-linux-gnu -fopenmp-version=51
// RUN: %libomptarget-run-x86_64-pc-linux-gnu 2>&1 \
// RUN: | %fcheck-x86_64-pc-linux-gnu -match-full-lines

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  //      CHECK:arr:          0x[[#%x,ARR:]]
  // CHECK-NEXT:arr_dev:      0x[[#%x,ARR_DEV:]]
  // CHECK-NEXT:element size: [[#%u,ELE_SIZE:]]
  int arr[3];
  void *arr_dev = omp_target_alloc(sizeof arr, omp_get_default_device());
  if (!arr_dev) {
    fprintf(stderr, "omp_target_alloc failed\n");
    return 1;
  }
  fprintf(stderr, "arr: %p\n", arr);
  fprintf(stderr, "arr_dev: %p\n", arr_dev);
  fprintf(stderr, "element size: %zu\n", sizeof *arr);

  // OpenMP 5.1, sec. 3.8.11 "omp_get_mapped_ptr", p. 431, L2-3,10:
  // "The device_num argument must be greater than or equal to zero and less
  // than or equal to the result of omp_get_num_devices()."
  // "The routine returns NULL (or C_NULL_PTR, for Fortran) if unsuccessful."
  //
  // CHECK-NEXT:negative device: (nil)
  // CHECK-NEXT:invalid positive device: (nil)
  fprintf(stderr, "negative device: %p\n", omp_get_mapped_ptr(arr, -1));
  fprintf(stderr, "invalid positive device: %p\n",
          omp_get_mapped_ptr(arr, omp_get_num_devices()));

  // OpenMP 5.1, sec. 3.8.11 "omp_get_mapped_ptr", p. 431, L7-9:
  // "The omp_get_mapped_ptr routine returns the associated device pointer on
  // device device_num.  A call to this routine for a pointer that is not NULL
  // (or C_NULL_PTR, for Fortran) and does not have an associated pointer on the
  // given device results in a NULL pointer."
  //
  // Check that the correct offset is computed when the address is within a
  // larger allocation.
  //
  // CHECK-NEXT:not yet mapped: (nil)
  // CHECK-NEXT:mapped start:   0x[[#ARR_DEV]]
  // CHECK-NEXT:mapped within:  0x[[#%x,ARR_DEV + ELE_SIZE]]
  // CHECK-NEXT:mapped end:     0x[[#%x,ARR_DEV + ELE_SIZE + ELE_SIZE]]
  // CHECK-NEXT:mapped after:   (nil)
  // CHECK-NEXT:unmapped:       (nil)
  fprintf(stderr, "not yet mapped: %p\n",
          omp_get_mapped_ptr(arr, omp_get_default_device()));
  if (omp_target_associate_ptr(arr, arr_dev, sizeof arr, 0,
                               omp_get_default_device())) {
    fprintf(stderr, "omp_target_associate_ptr failed\n");
    abort();
  }
  fprintf(stderr, "mapped start: %p\n",
          omp_get_mapped_ptr(arr, omp_get_default_device()));
  fprintf(stderr, "mapped within: %p\n",
          omp_get_mapped_ptr(arr + 1, omp_get_default_device()));
  fprintf(stderr, "mapped end: %p\n",
          omp_get_mapped_ptr(arr + 2, omp_get_default_device()));
  fprintf(stderr, "mapped after: %p\n",
          omp_get_mapped_ptr(arr + 3, omp_get_default_device()));
  if (omp_target_disassociate_ptr(arr, omp_get_default_device())) {
    fprintf(stderr, "omp_target_disassociate_ptr failed\n");
    abort();
  }
  fprintf(stderr, "unmapped: %p\n",
          omp_get_mapped_ptr(arr, omp_get_default_device()));

  // OpenMP 5.1, sec. 3.8.11 "omp_get_mapped_ptr", p. 431, L10-12:
  // "The routine returns NULL (or C_NULL_PTR, for Fortran) if unsuccessful.
  // Otherwise it returns the device pointer, which is ptr if device_num is the
  // value returned by omp_get_initial_device()."
  //
  // CHECK-NEXT:initial device: 0x[[#ARR]]
  fprintf(stderr, "initial device: %p\n",
          omp_get_mapped_ptr(arr, omp_get_initial_device()));

  // FIXME: What does OpenMP 5.1 specify for a null pointer?
  //
  // CHECK-NEXT:null pointer: (nil)
  fprintf(stderr, "null pointer: %p\n",
          omp_get_mapped_ptr(NULL, omp_get_default_device()));

  return 0;
}
