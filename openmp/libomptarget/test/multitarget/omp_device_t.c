// Check omp_device_t, omp_get_device_type, omp_get_num_devices_of_type,
// omp_get_typed_device_num, and omp_get_device_of_type.

// RUN: %libomptarget-compile-run-and-check-generic \
// RUN:   -match-full-lines -strict-whitespace --implicit-check-not=error \
// RUN:   -check-prefixes=CHECK,%libomptarget-targets-uppercase  \
// RUN:   -allow-unused-prefixes

#include <omp.h>
#include <stdbool.h>
#include <stdio.h>

#define STR(Arg) #Arg

#define FOREACH_ARCH(Macro)                                                    \
  Macro(x86_64)                                                                \
  Macro(ppc64le)                                                               \
  Macro(nvptx64)

#define DEF_GET_ARCH(Arch)                                                     \
_Pragma(STR(omp begin declare variant match(device={arch(Arch)})))             \
const char *getArch() { return #Arch; }                                        \
_Pragma("omp end declare variant")

FOREACH_ARCH(DEF_GET_ARCH)

//                          CHECK:key:
//                     CHECK-NEXT:    dt   = omp_get_device_type
//                     CHECK-NEXT:    ndot = omp_get_num_devices_of_type
//                     CHECK-NEXT:    tdn  = omp_get_typed_device_num
//                     CHECK-NEXT:    dot  = omp_get_device_of_type
//                     CHECK-NEXT:omp_get_initial_device() = [[#%d,HOST:]]
//                     CHECK-NEXT:omp_device_none=0:
//                     CHECK-NEXT:    ndot: 0
//                     CHECK-NEXT:    -1: dot=-1
//                     CHECK-NEXT:    0: dot=-1
//                     CHECK-NEXT:omp_device_host=1:
//                     CHECK-NEXT:    ndot: 1
//                     CHECK-NEXT:    -1: dot=-1
//                     CHECK-NEXT:    0: dot=[[#HOST]], dt=1, tdn=0 => {{.*}}
//                     CHECK-NEXT:    1: dot=-1
//                     CHECK-NEXT:omp_device_x86_64=[[#]]:
//                     CHECK-NEXT:    ndot: [[#X86_64:]]
//                     CHECK-NEXT:    -1: dot=-1
//       X86_64-PC-LINUX-GNU-SAME:{{([[:space:]]+[0-9]+: dot=[0-9]+, dt=[0-9]+, tdn=[0-9]+ => x86_64)+}}
//                     CHECK-NEXT:    {{[0-9]+}}: dot=-1
//                     CHECK-NEXT:omp_device_ppc64le=[[#]]:
//                     CHECK-NEXT:    ndot: [[#PPC64LE:]]
//                     CHECK-NEXT:    -1: dot=-1
// POWERPC64LE-IBM-LINUX-GNU-SAME:{{([[:space:]]+[0-9]+: dot=[0-9]+, dt=[0-9]+, tdn=[0-9]+ => ppc64le)+}}
//                     CHECK-NEXT:    {{[0-9]+}}: dot=-1
//                     CHECK-NEXT:omp_device_nvptx64=[[#]]:
//                     CHECK-NEXT:    ndot: [[#NVPTX64:]]
//                     CHECK-NEXT:    -1: dot=-1
//       NVPTX64-NVIDIA-CUDA-SAME:{{([[:space:]]+[0-9]+: dot=[0-9]+, dt=[0-9]+, tdn=[0-9]+ => nvptx64)+}}
//                     CHECK-NEXT:    {{[0-9]+}}: dot=-1
//                     CHECK-NEXT:omp_get_num_devices() = [[#X86_64 + PPC64LE + NVPTX64]]
//                     CHECK-NEXT:omp_get_num_devices() = [[#HOST]]
//                     CHECK-NEXT:calls for bad device numbers:
//                     CHECK-NEXT:    -97: dt=0, tdn=-1
//                     CHECK-NEXT:    -1: dt=0, tdn=-1
//                     CHECK-NEXT:    omp_get_num_devices() + 1: dt=0, tdn=-1
//                     CHECK-NEXT:    omp_get_num_devices() + 8: dt=0, tdn=-1

void checkArch(omp_device_t DevType, const char *DevTypeName) {
  printf("%s=%d:\n", DevTypeName, DevType);
  int NumDevsOfType = omp_get_num_devices_of_type(DevType);
  printf("    ndot: %d\n", NumDevsOfType);
  for (int TypedDevNum = -1; TypedDevNum <= NumDevsOfType; ++TypedDevNum) {
    int DevNum = omp_get_device_of_type(DevType, TypedDevNum);
    printf("    %d: dot=%d", TypedDevNum, DevNum);
    if (0 <= TypedDevNum && TypedDevNum < NumDevsOfType) {
      omp_device_t DT = omp_get_device_type(DevNum);
      int TDN = omp_get_typed_device_num(DevNum);
      printf(", dt=%d%s, tdn=%d%s",
             DT, DT == DevType ? "" : " (error!)",
             TDN, TDN == TypedDevNum ? "" : " (error!)");
      #pragma omp target device(DevNum)
      printf(" => %s", getArch());
    }
    printf("\n");
  }
}

int main() {
  printf("key:\n"
         "    dt   = omp_get_device_type\n"
         "    ndot = omp_get_num_devices_of_type\n"
         "    tdn  = omp_get_typed_device_num\n"
         "    dot  = omp_get_device_of_type\n");
  printf("omp_get_initial_device() = %d\n", omp_get_initial_device());

#define CHECK_ARCH(Arch)                                                       \
  checkArch(omp_device_##Arch, "omp_device_" #Arch);
  CHECK_ARCH(none)
  CHECK_ARCH(host)
  FOREACH_ARCH(CHECK_ARCH)
  printf("omp_get_num_devices() = %d\n", omp_get_num_devices());
  printf("omp_get_num_devices() = %d\n", omp_get_num_devices());

  printf("calls for bad device numbers:\n");
#define CHECK_BAD_DEV_NUM(DevNum)                                              \
  printf("    %s: dt=%d, tdn=%d\n", #DevNum, omp_get_device_type(DevNum),      \
         omp_get_typed_device_num(DevNum));
  CHECK_BAD_DEV_NUM(-97);
  CHECK_BAD_DEV_NUM(-1);
  CHECK_BAD_DEV_NUM(omp_get_num_devices() + 1);
  CHECK_BAD_DEV_NUM(omp_get_num_devices() + 8);

  return 0;
}
