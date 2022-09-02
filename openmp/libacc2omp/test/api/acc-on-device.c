// Check acc_on_device.
//
// It should not be necessary to rely on device management routines (like
// acc_get_device_type) to test acc_on_device.  Moreover, those are tested in
// devices.c with the assumption that acc_on_device has been independently
// tested to be correct.

// Check that our constant expression checks are sane: that they actually call
// acc_on_device where Clang requires constant expressions.
//
// RUN: %clang-acc-fsyntax-only -Xclang -verify=const-expr-err \
// RUN:   -DACC2OMP_ENUM_VARIANTS_SUPPORTED=0 %s

// Check successful cases.
// 
// DEFINE: %{check-exe}( CONST_EXPR %, RUN_ENV %, OFF_TYPE %, EXE %, RUN_IF %) = \
// DEFINE:   : '--------------------' &&                                         \
// DEFINE:   : 'RUN_ENV: %{RUN_ENV}'  &&                                         \
// DEFINE:   : 'EXE: %{EXE}'          &&                                         \
// DEFINE:   : '--------------------' &&                                         \
// DEFINE:   %{RUN_IF} %{RUN_ENV} %{EXE} > %t.out 2>&1 &&                        \
// DEFINE:   %{RUN_IF} FileCheck -input-file=%t.out -match-full-lines %s         \
// DEFINE:     -check-prefixes=CHECK,CHECK-%{OFF_TYPE}                           \
// DEFINE:     -check-prefixes=CHECK-%{CONST_EXPR}                               \
// DEFINE:     -check-prefixes=CHECK-%{CONST_EXPR}-%{OFF_TYPE}
//
// DEFINE: %{check-exes}( CONST_EXPR %, RUN_ENV %, OFF_TYPE %) =                                            \
// DEFINE:   %{check-exe}( %{CONST_EXPR} %, %{RUN_ENV} %, %{OFF_TYPE} %, %t-noacc.exe %, %if-host<|:> %) && \
// DEFINE:   %{check-exe}( %{CONST_EXPR} %, %{RUN_ENV} %, %{OFF_TYPE} %, %t-s2s.exe   %,              %) && \
// DEFINE:   %{check-exe}( %{CONST_EXPR} %, %{RUN_ENV} %, %{OFF_TYPE} %, %t-trad.exe  %,              %)
//
// DEFINE: %{check-run-envs}( CONST_EXPR %) =                                                         \
// DEFINE:   %{check-exes}( %{CONST_EXPR} %,                                 %, %dev-type-0-omp %) && \
// DEFINE:   %{check-exes}( %{CONST_EXPR} %, env OMP_TARGET_OFFLOAD=disabled %, host            %) && \
// DEFINE:   %{check-exes}( %{CONST_EXPR} %, env ACC_DEVICE_TYPE=host        %, host            %)
//
// DEFINE: %{check-context}( CONTEXT_CFLAGS %, CONST_EXPR %) =                 \
// DEFINE:   %if-host<|:> %clang-noacc %{CONTEXT_CFLAGS} -o %t-noacc.exe %s && \
// DEFINE:   %clang-acc-prt-omp %{CONTEXT_CFLAGS} %s > %t-prt-omp.c         && \
// DEFINE:   %clang-omp %{CONTEXT_CFLAGS} -o %t-s2s.exe %t-prt-omp.c        && \
// DEFINE:   %clang-acc %{CONTEXT_CFLAGS} -o %t-trad.exe %s                 && \
// DEFINE:   %{check-run-envs}( %{CONST_EXPR} %)
//
// RUN: %{check-context}(                                                        %, CONST-EXPR    %)
// RUN: %{check-context}( -DACC2OMP_ENUM_VARIANTS_SUPPORTED=1                    %, CONST-EXPR    %)
// RUN: %{check-context}( -DACC2OMP_ENUM_VARIANTS_SUPPORTED=0 -DSKIP_CONST_EXPRS %, NO-CONST-EXPR %)

// END.

// expected-error 0 {{}}

#include <openacc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//                     CHECK-NOT: {{.}}
//                         CHECK: acc_device_none:
//                    CHECK-NEXT:   outside kernel: 0
//                    CHECK-NEXT:   inside kernel:  0
//                    CHECK-NEXT: acc_device_host:
//                    CHECK-NEXT:   outside kernel: 1
//               CHECK-host-NEXT:   inside kernel:  1
//             CHECK-x86_64-NEXT:   inside kernel:  0
//            CHECK-ppc64le-NEXT:   inside kernel:  0
//            CHECK-nvptx64-NEXT:   inside kernel:  0
//             CHECK-amdgcn-NEXT:   inside kernel:  0
//                    CHECK-NEXT: acc_device_not_host:
//                    CHECK-NEXT:   outside kernel: 0
//               CHECK-host-NEXT:   inside kernel:  0
//             CHECK-x86_64-NEXT:   inside kernel:  1
//            CHECK-ppc64le-NEXT:   inside kernel:  1
//            CHECK-nvptx64-NEXT:   inside kernel:  1
//             CHECK-amdgcn-NEXT:   inside kernel:  1
//                    CHECK-NEXT: acc_device_default:
//                    CHECK-NEXT:   outside kernel: 0
//                    CHECK-NEXT:   inside kernel:  0
//                    CHECK-NEXT: acc_device_current:
//                    CHECK-NEXT:   outside kernel: 0
//                    CHECK-NEXT:   inside kernel:  0
//                    CHECK-NEXT: acc_device_nvidia:
//                    CHECK-NEXT:   outside kernel: 0
//               CHECK-host-NEXT:   inside kernel:  0
//             CHECK-x86_64-NEXT:   inside kernel:  0
//            CHECK-ppc64le-NEXT:   inside kernel:  0
//            CHECK-nvptx64-NEXT:   inside kernel:  1
//             CHECK-amdgcn-NEXT:   inside kernel:  0
//                    CHECK-NEXT: acc_device_radeon:
//                    CHECK-NEXT:   outside kernel: 0
//               CHECK-host-NEXT:   inside kernel:  0
//             CHECK-x86_64-NEXT:   inside kernel:  0
//            CHECK-ppc64le-NEXT:   inside kernel:  0
//            CHECK-nvptx64-NEXT:   inside kernel:  0
//             CHECK-amdgcn-NEXT:   inside kernel:  1
//                    CHECK-NEXT: acc_device_x86_64:
//                    CHECK-NEXT:   outside kernel: 0
//               CHECK-host-NEXT:   inside kernel:  0
//             CHECK-x86_64-NEXT:   inside kernel:  1
//            CHECK-ppc64le-NEXT:   inside kernel:  0
//            CHECK-nvptx64-NEXT:   inside kernel:  0
//             CHECK-amdgcn-NEXT:   inside kernel:  0
//                    CHECK-NEXT: acc_device_ppc64le:
//                    CHECK-NEXT:   outside kernel: 0
//               CHECK-host-NEXT:   inside kernel:  0
//             CHECK-x86_64-NEXT:   inside kernel:  0
//            CHECK-ppc64le-NEXT:   inside kernel:  1
//            CHECK-nvptx64-NEXT:   inside kernel:  0
//             CHECK-amdgcn-NEXT:   inside kernel:  0
//         CHECK-CONST-EXPR-NEXT: acc_device_host (constant expression):
//         CHECK-CONST-EXPR-NEXT:   outside kernel: 1
//    CHECK-CONST-EXPR-host-NEXT:   inside kernel:  1
//  CHECK-CONST-EXPR-x86_64-NEXT:   inside kernel:  0
// CHECK-CONST-EXPR-ppc64le-NEXT:   inside kernel:  0
// CHECK-CONST-EXPR-nvptx64-NEXT:   inside kernel:  0
//  CHECK-CONST-EXPR-amdgcn-NEXT:   inside kernel:  0
//         CHECK-CONST-EXPR-NEXT: acc_device_not_host (constant expression):
//         CHECK-CONST-EXPR-NEXT:   outside kernel: 0
//    CHECK-CONST-EXPR-host-NEXT:   inside kernel:  0
//  CHECK-CONST-EXPR-x86_64-NEXT:   inside kernel:  1
// CHECK-CONST-EXPR-ppc64le-NEXT:   inside kernel:  1
// CHECK-CONST-EXPR-nvptx64-NEXT:   inside kernel:  1
//  CHECK-CONST-EXPR-amdgcn-NEXT:   inside kernel:  1
//                     CHECK-NOT: {{.}}

static int insideKernel(acc_device_t devType) {
  int res;
  #pragma acc parallel num_gangs(1) copyout(res)
  res = acc_on_device(devType);
  return res;
}

int main() {
#define DEVICE_ENUMERATOR(DevType)                                             \
  printf("acc_device_" #DevType ":\n");                                        \
  printf("  outside kernel: %d\n", acc_on_device(acc_device_##DevType));       \
  printf("  inside kernel:  %d\n", insideKernel(acc_device_##DevType));
  ACC2OMP_FOREACH_DEVICE(DEVICE_ENUMERATOR)
#undef DEVICE_ENUMERATOR
  // FIXME: It would be better to check constant expressions for all device
  // types by macroifying all this and calling within ACC2OMP_FOREACH_DEVICE
  // (and probably just doing that in the existing ACC2OMP_FOREACH_DEVICE
  // invocation below).  However, that requires using a _Pragma, and Clang
  // OpenMP codegen currently fails an assert if we, multiple times, expand a
  // macro containing an OpenMP _Pragma.  This bug seems to be fixed in later
  // Clang versions, so we can improve this after merging in upstream later.
  // Here's a reproducer for the bug so we can easily determine when it's fixed:
  //
  //   #define X _Pragma("omp target") ;
  //   int main() {
  //     X X
  //     return 0;
  //   }
#if !SKIP_CONST_EXPRS
  int res;
  // FIXME: Clang produces confusing location info for these errors.
  // const-expr-err-error@* 4 {{expression is not an integer constant expression}}

  printf("acc_device_host (constant expression):\n");
  enum {ConstHostOutsideKernel = acc_on_device(acc_device_host)};
  printf("  outside kernel: %d\n", ConstHostOutsideKernel);
  #pragma acc parallel num_gangs(1) copyout(res)
  {
    enum {ConstHostInsideKernel = acc_on_device(acc_device_host)};
    res = ConstHostInsideKernel;
  }
  printf("  inside kernel:  %d\n", res);

  printf("acc_device_not_host (constant expression):\n");
  enum {ConstNotHostOutsideKernel = acc_on_device(acc_device_not_host)};
  printf("  outside kernel: %d\n", ConstNotHostOutsideKernel);
  #pragma acc parallel num_gangs(1) copyout(res)
  {
    enum {ConstNotHostInsideKernel = acc_on_device(acc_device_not_host)};
    res = ConstNotHostInsideKernel;
  }
  printf("  inside kernel:  %d\n", res);
#endif
  return 0;
}
