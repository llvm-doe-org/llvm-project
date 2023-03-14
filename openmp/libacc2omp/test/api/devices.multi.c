// Check device management routines without runtime errors.
//
// Selection of the initial current device via environment variables (e..g,
// ACC_DEVICE_TYPE and ACC_DEVICE_NUM) is checked in devices-init.c assuming
// these routines are correct as checked here, so don't use those environment
// variables here.
//
// TODO: When there are multiple offload device types (the multitarget sub test
// suite), we currently do not know how LLVM's implementation selects the
// default offload device type.  If, one day, we figure out how it's determined,
// we can reintroduce test coverage for that case by replacing all noMULTI and
// MULTI prefixes with CHECK below.  See the related todo in lit.cfg for further
// details.

// DEFINE: %{check}( RUN_ENV %, ND_X86_64 %, ND_PPC64LE %, ND_NVPTX64 %,       \
// DEFINE:           ND_AMDGCN %, DEV_TYPE_INIT %, ND_DEV_TYPE_INIT %) =       \
// DEFINE:   %{RUN_ENV} %t.exe > %t.out 2>&1 &&                                \
// DEFINE:   FileCheck -input-file %t.out %s                                   \
// DEFINE:     -strict-whitespace -match-full-lines                            \
// DEFINE:     -DDEV_TYPE_INIT=acc_device_%{DEV_TYPE_INIT} -D#DEV_NUM_INIT=0   \
// DEFINE:     -D#ND_DEV_TYPE_INIT=%{ND_DEV_TYPE_INIT}                         \
// DEFINE:     -D#ND_NVIDIA=%{ND_NVPTX64} -D#ND_RADEON=%{ND_AMDGCN}            \
// DEFINE:     -D#ND_X86_64=%{ND_X86_64} -D#ND_PPC64LE=%{ND_PPC64LE}           \
// DEFINE:     -check-prefixes=CHECK,%if-multi<|no>MULTI

// RUN: %clang-acc -o %t.exe %s
//                RUN_ENV                            ND_X86_64           ND_PPC64LE           ND_NVPTX64           ND_AMDGCN           DEV_TYPE_INIT      ND_DEV_TYPE_INIT
// RUN: %{check}(                                 %, %x86_64-num-devs %, %ppc64le-num-devs %, %nvptx64-num-devs %, %amdgcn-num-devs %, %dev-type-0-acc %, %dev-type-0-num-devs %)
// RUN: %{check}( env OMP_TARGET_OFFLOAD=disabled %, 0                %, 0                 %, 0                 %, 0                %, host            %, 1                    %)
//
// END.

// expected-no-diagnostics

#include <assert.h>
#include <openacc.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//    CHECK-NOT:{{.}}
//        CHECK:initially:
// noMULTI-NEXT:    acc_get_device_type() = [[DEV_TYPE_INIT]]
//   MULTI-NEXT:    acc_get_device_type() = {{.*}}
// noMULTI-NEXT:    acc_get_device_num([[DEV_TYPE_INIT]]) = [[#DEV_NUM_INIT]]
//   MULTI-NEXT:    acc_get_device_num({{.*}}) = [[#DEV_NUM_INIT]]
// noMULTI-NEXT:    acc_get_num_devices([[DEV_TYPE_INIT]]) = [[#ND_DEV_TYPE_INIT]]
//   MULTI-NEXT:    acc_get_num_devices({{.*}}) = {{.*}}
//   CHECK-NEXT:acc_device_none has 0:
//   CHECK-NEXT:acc_device_host has 1:
//   CHECK-NEXT:    0: num=0, type=acc_device_host, on acc_device_host
// noMULTI-NEXT:acc_device_default has [[#ND_DEV_TYPE_INIT]]:
//   MULTI-NEXT:acc_device_default has {{.*}}:
//   CHECK-SAME:{{([[:space:]]+[0-9]+: num=[0-9]+, type=[a-z0-9_]+, on [a-z0-9_]+)+}}
//   CHECK-NEXT:acc_device_current has 1:
// noMULTI-NEXT:    0: num=0, type=[[DEV_TYPE_INIT]], on [[DEV_TYPE_INIT]]
//   MULTI-NEXT:    0: num=0, type={{.*}}, on {{.*}}
//   CHECK-NEXT:acc_device_nvidia has [[#ND_NVIDIA]]:
//   CHECK-SAME:{{([[:space:]]+[0-9]+: num=[0-9]+, type=acc_device_nvidia, on acc_device_nvidia)*}}
//   CHECK-NEXT:acc_device_radeon has [[#ND_RADEON]]:
//   CHECK-SAME:{{([[:space:]]+[0-9]+: num=[0-9]+, type=acc_device_radeon, on acc_device_radeon)*}}
//   CHECK-NEXT:acc_device_x86_64 has [[#ND_X86_64]]:
//   CHECK-SAME:{{([[:space:]]+[0-9]+: num=[0-9]+, type=acc_device_x86_64, on acc_device_x86_64)*}}
//   CHECK-NEXT:acc_device_ppc64le has [[#ND_PPC64LE]]:
//   CHECK-SAME:{{([[:space:]]+[0-9]+: num=[0-9]+, type=acc_device_ppc64le, on acc_device_ppc64le)*}}
//   CHECK-NEXT:acc_device_not_host has [[#ND_NOT_HOST:ND_NVIDIA + ND_RADEON + ND_X86_64 + ND_PPC64LE]]:
//   CHECK-SAME:{{([[:space:]]+[0-9]+: num=[0-9]+, type=[a-z0-9_]+, on [a-z0-9_]+, on acc_device_not_host)*}}
//    CHECK-NOT:{{.}}

static const char *deviceTypeToStr(acc_device_t DevType) {
  switch (DevType) {
#define DEVCASE(DevTypeEnum)                                                   \
  case acc_device_##DevTypeEnum:                                               \
    return "acc_device_" #DevTypeEnum;
  ACC2OMP_FOREACH_DEVICE(DEVCASE)
#undef DEVCASE
  }
  return "<unknown acc_device_t>";
}

static void checkOtherType(acc_device_t DevTypeSet, int DevNumSet,
                           acc_device_t DevTypeCur, acc_device_t DevTypeOther) {
  assert(DevTypeSet != acc_device_none && "expected valid device type was set");
  assert(DevTypeCur != acc_device_none && "expected valid current device type");
  int DevNumOther = acc_get_device_num(DevTypeOther);
  bool Good = false;
  switch (DevTypeOther) {
  case acc_device_none:
    Good = DevNumOther == -1;
    break;
  case acc_device_host:
    if (DevTypeCur == acc_device_host)
      Good = DevNumOther == 0;
    else
      Good = DevNumOther == -1;
    break;
  case acc_device_not_host:
    if (DevTypeCur == acc_device_host)
      Good = DevNumOther == -1;
    else if (DevTypeSet == acc_device_not_host)
      Good = DevNumOther == DevNumSet;
    else
      Good = DevNumOther >= 0;
    break;
  case acc_device_default:
    if (DevTypeCur == acc_device_host)
      Good = DevNumOther == 0;
    else if (DevTypeSet == acc_device_not_host ||
             DevTypeSet == acc_device_current)
      Good = DevNumOther >= 0;
    else
      Good = DevNumOther == DevNumSet;
    break;
  case acc_device_current:
    Good = DevNumOther == 0;
    break;
#define ACC2OMP_DEVICE_ENUMERATOR(Device)                                      \
  case acc_device_##Device:
  ACC2OMP_FOREACH_ARCH_DEVICE(ACC2OMP_DEVICE_ENUMERATOR)
#undef ACC2OMP_DEVICE_ENUMERATOR
    if (DevTypeOther == DevTypeSet)
      Good = DevNumOther == DevNumSet;
    else if (DevTypeOther == DevTypeCur)
      Good = DevNumOther >= 0;
    else
      Good = DevNumOther == -1;
    break;
  }
  if (!Good)
    printf("    error: acc_get_device_num(%s) = %d\n",
           deviceTypeToStr(DevTypeOther), DevNumOther);
}

static void checkType(acc_device_t DevType, acc_device_t DevTypeInit,
                      int DevNumInit) {
  // Based on acc_get_num_devices, iterate all devices of the specified type.
  int NumDevs = acc_get_num_devices(DevType);
  printf("%s has %d:\n", deviceTypeToStr(DevType), NumDevs);
  for (int DevNum = 0; DevNum < NumDevs; ++DevNum) {
    printf("    %d:", DevNum);

    // Switch to that device using acc_set_device_num.
    acc_set_device_num(DevNum, DevType);

    // Check that acc_get_device_num gives us back what we set.
    int DN = acc_get_device_num(DevType);
    printf(" num=%d%s", DN, DN == DevNum ? "" : " (error!)");

    // Check that acc_get_device_type reports a reasonable type given the type
    // we're iterating.
    acc_device_t DT = acc_get_device_type();
    bool DTGood;
    if (DevType == acc_device_current || DevType == acc_device_default) {
      // Must be the initial current device type.
      DTGood = DT == DevTypeInit;
    } else if (DevType == acc_device_not_host) {
      // Must be an architecture-specific device type.
      DTGood = DT != acc_device_none && DT != acc_device_host &&
               DT != acc_device_not_host && DT != acc_device_default &&
               DT != acc_device_current;
    } else {
      // Must be exactly the device type we're iterating, which is either
      // acc_device_host or an architecture-specific device type.
      DTGood = DT == DevType;
    }
    // acc_get_device_type should only ever return acc_device_host or an
    // architecture-specific type.  Actually, it could return
    // acc_device_not_host, but that would mean we are testing an architecture
    // we never added to one or both of acc_device_t and omp_device_t, and we
    // should fix that.
    if (DT == acc_device_none || DT == acc_device_not_host ||
        DT == acc_device_default || DT == acc_device_current)
      DTGood = false;
    printf(", type=%s%s", deviceTypeToStr(DT), DTGood ? "" : " (error!)");

    // Check that the device is actually of the device type we're iterating and
    // of the device type returned by acc_get_device_type.  acc_on_device is
    // tested elsewhere, and here we assume it's correct.
    bool OnDT, OnDevType;
    #pragma acc parallel num_gangs(1) copyout(OnDT, OnDevType)
    {
      OnDT = acc_on_device(DT);
      OnDevType = acc_on_device(DevType);
    }
    printf(", %son %s", OnDT ? "" : "not ", deviceTypeToStr(DT));
    if (DevType != DT && DevType != acc_device_default && DevType != acc_device_current)
      printf(", %son %s", OnDevType ? "" : "not ", deviceTypeToStr(DevType));
    printf("\n");

    // Check that acc_get_device_num returns something sane for device types
    // other than the current device type.
#define CHECK_TYPE(Type)                                                       \
    checkOtherType(DevType, DevNum, DT, acc_device_##Type);
    ACC2OMP_FOREACH_DEVICE(CHECK_TYPE)
#undef CHECK_TYPE

    // Check that acc_device_default/acc_device_current really are for the
    // current device even when that's not the initial current device.  And
    // check that acc_device_current ignores the device number.
    acc_set_device_num(DevNum + 1, acc_device_current);
    if (DT != acc_get_device_type() || DevNum != acc_get_device_num(DevType))
      printf("    error: acc_set_device_num for acc_device_current changed the "
             "current device\n");
    acc_set_device_type(acc_device_current);
    if (DT != acc_get_device_type() || DevNum != acc_get_device_num(DevType))
      printf("    error: acc_set_device_type for acc_device_current changed "
             "the current device\n");
    acc_set_device_num(0, acc_device_default);
    if (DT != acc_get_device_type())
      printf("    error: acc_set_device_num for acc_device_default changed the "
             "current device type \n");
    acc_set_device_type(acc_device_default);
    if (DT != acc_get_device_type())
      printf("    error: acc_set_device_type for acc_device_default changed "
             "the current device type\n");

    // Switch back to the initial current device.
    acc_set_device_num(DevNumInit, DevTypeInit);

    // Check that acc_set_device_type(T) behaves like acc_set_device_num(0, T).
    if (DevNum == 0) {
      acc_set_device_type(DevType);
      acc_device_t DT_SDT = acc_get_device_type();
      if (DT != DT_SDT)
        printf("    error: acc_set_device_type(%s) sets %s\n",
               deviceTypeToStr(DevType), deviceTypeToStr(DT_SDT));
      int DN_SDT = acc_get_device_num(DevType);
      if (DN != DN_SDT)
        printf("    error: acc_set_device_type(%s) sets num=%d\n",
               deviceTypeToStr(DevType), DN_SDT);
      acc_set_device_num(DevNumInit, DevTypeInit);
    }
  }
}

int main() {
  acc_device_t DevTypeInit = acc_get_device_type();
  const char *DevTypeInitStr = deviceTypeToStr(DevTypeInit);
  int DevNumInit = acc_get_device_num(DevTypeInit);
  printf("initially:\n"
         "    acc_get_device_type() = %s\n"
         "    acc_get_device_num(%s) = %d\n"
         "    acc_get_num_devices(%s) = %d\n",
         DevTypeInitStr, DevTypeInitStr, DevNumInit, DevTypeInitStr,
         acc_get_num_devices(DevTypeInit));

#define CHECK_TYPE(Type)                                                       \
    checkType(acc_device_##Type, DevTypeInit, DevNumInit);
#define CHECK_TYPE_SKIP_NOT_HOST(Type)                                         \
  if (acc_device_##Type != acc_device_not_host)                                \
    CHECK_TYPE(Type)
  ACC2OMP_FOREACH_DEVICE(CHECK_TYPE_SKIP_NOT_HOST)
  CHECK_TYPE(not_host)
#undef CHECK_TYPE_SKIP_NOT_HOST
#undef CHECK_TYPE

  return 0;
}
