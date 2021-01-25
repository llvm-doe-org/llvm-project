// Check acc_on_device.

// Check that our constant expression checks are sane: that they actually call
// acc_on_device where Clang requires constant expressions.
//
// RUN: %clang -Xclang -verify=const-expr-err -fsyntax-only -fopenacc \
// RUN:        %acc-includes -DACC2OMP_ENUM_VARIANTS_SUPPORTED=0 -o %t.exe %s

// Check successful cases.
//
// RUN: %data contexts {
// RUN:   (context-cflags=                                                         const-expr=CONST-EXPR   )
// RUN:   (context-cflags=-DACC2OMP_ENUM_VARIANTS_SUPPORTED=1                      const-expr=CONST-EXPR   )
// RUN:   (context-cflags='-DACC2OMP_ENUM_VARIANTS_SUPPORTED=0 -DSKIP_CONST_EXPRS' const-expr=NO-CONST-EXPR)
// RUN: }
// RUN: %data tgts {
// RUN:   (run-if=                tgt-cflags=                                     tgt-off=NO-OFF tgt-arch=%host-arch)
// RUN:   (run-if=%run-if-x86_64  tgt-cflags=-fopenmp-targets=%run-x86_64-triple  tgt-off=OFF    tgt-arch=x86_64    )
// RUN:   (run-if=%run-if-ppc64le tgt-cflags=-fopenmp-targets=%run-ppc64le-triple tgt-off=OFF    tgt-arch=ppc64le   )
// RUN:   (run-if=%run-if-nvptx64 tgt-cflags=-fopenmp-targets=%run-nvptx64-triple tgt-off=OFF    tgt-arch=nvptx64   )
// RUN: }
// RUN: %data run-envs {
// RUN:   (run-env=                                  off=%[tgt-off] kern-arch=%[tgt-arch])
// RUN:   (run-env='env OMP_TARGET_OFFLOAD=disabled' off=NO-OFF     kern-arch=%host-arch )
// RUN: }
// RUN: %data exes {
// RUN:   (exe=%t.exe    )
// RUN:   (exe=%t-s2s.exe)
// RUN: }
// RUN: %for contexts {
// RUN:   %clang -Xclang -verify -fopenacc-print=omp %acc-includes \
// RUN:       %[context-cflags] -Wno-openacc-omp-map-hold %s > %t-omp.c
// RUN:   %for tgts {
// RUN:     %[run-if] %clang -Xclang -verify -fopenacc %acc-includes \
// RUN:         %[context-cflags] %[tgt-cflags] -o %t.exe %s
// RUN:     %[run-if] %clang -Xclang -verify -fopenmp %acc-includes \
// RUN:         %[context-cflags] %[tgt-cflags] -o %t-s2s.exe %t-omp.c
// RUN:     %for run-envs {
// RUN:       %for exes {
// RUN:         %[run-if] %[run-env] %[exe] > %t.out 2>&1
// RUN:         %[run-if] FileCheck -input-file %t.out %s -match-full-lines \
// RUN:             -check-prefixes=CHECK,CHECK-%[off] \
// RUN:             -check-prefixes=CHECK-%[const-expr],CHECK-%[const-expr]-%[off] \
// RUN:             -check-prefixes=CHECK-HOST-%host-arch,CHECK-KERN-%[kern-arch]
// RUN:       }
// RUN:     }
// RUN:   }
// RUN: }

// END.

// expected-no-diagnostics

#include <openacc.h>
#include <stdio.h>

//                    CHECK-NOT: {{.}}
//                        CHECK: acc_device_none:
//                   CHECK-NEXT:   outside kernel: 0
//                   CHECK-NEXT:   inside kernel:  0
//                   CHECK-NEXT: acc_device_default:
//                   CHECK-NEXT:   outside kernel: 0
//                   CHECK-NEXT:   inside kernel:  0
//                   CHECK-NEXT: acc_device_host:
//                   CHECK-NEXT:   outside kernel: 1
//            CHECK-NO-OFF-NEXT:   inside kernel:  1
//               CHECK-OFF-NEXT:   inside kernel:  0
//                   CHECK-NEXT: acc_device_not_host:
//                   CHECK-NEXT:   outside kernel: 0
//            CHECK-NO-OFF-NEXT:   inside kernel:  0
//               CHECK-OFF-NEXT:   inside kernel:  1
//                   CHECK-NEXT: acc_device_current:
//                   CHECK-NEXT:   outside kernel: 0
//                   CHECK-NEXT:   inside kernel:  0
//                   CHECK-NEXT: acc_device_nvidia:
//                   CHECK-NEXT:   outside kernel: 0
//       CHECK-KERN-x86_64-NEXT:   inside kernel:  0
//      CHECK-KERN-ppc64le-NEXT:   inside kernel:  0
//      CHECK-KERN-nvptx64-NEXT:   inside kernel:  1
//                   CHECK-NEXT: acc_device_x86_64:
//       CHECK-HOST-x86_64-NEXT:   outside kernel: 1
//      CHECK-HOST-ppc64le-NEXT:   outside kernel: 0
//       CHECK-KERN-x86_64-NEXT:   inside kernel:  1
//      CHECK-KERN-ppc64le-NEXT:   inside kernel:  0
//      CHECK-KERN-nvptx64-NEXT:   inside kernel:  0
//                   CHECK-NEXT: acc_device_ppc64le:
//       CHECK-HOST-x86_64-NEXT:   outside kernel: 0
//      CHECK-HOST-ppc64le-NEXT:   outside kernel: 1
//       CHECK-KERN-x86_64-NEXT:   inside kernel:  0
//      CHECK-KERN-ppc64le-NEXT:   inside kernel:  1
//      CHECK-KERN-nvptx64-NEXT:   inside kernel:  0
//        CHECK-CONST-EXPR-NEXT: acc_device_host (constant expression):
//        CHECK-CONST-EXPR-NEXT:   outside kernel: 1
// CHECK-CONST-EXPR-NO-OFF-NEXT:   inside kernel:  1
//    CHECK-CONST-EXPR-OFF-NEXT:   inside kernel:  0
//        CHECK-CONST-EXPR-NEXT: acc_device_not_host (constant expression):
//        CHECK-CONST-EXPR-NEXT:   outside kernel: 0
// CHECK-CONST-EXPR-NO-OFF-NEXT:   inside kernel:  0
//    CHECK-CONST-EXPR-OFF-NEXT:   inside kernel:  1
//                    CHECK-NOT: {{.}}

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
  // FIXME: Clang produces confusing location info for these errors.
  // const-expr-err-error@* 4 {{expression is not an integer constant expression}}
  printf("acc_device_host (constant expression):\n");
  enum {ConstHostOutsideKernel = acc_on_device(acc_device_host)};
  printf("  outside kernel: %d\n", ConstHostOutsideKernel);
  #pragma acc parallel num_gangs(1)
  {
    enum {ConstHostInsideKernel = acc_on_device(acc_device_host)};
    printf("  inside kernel:  %d\n", ConstHostInsideKernel);
  }
  printf("acc_device_not_host (constant expression):\n");
  enum {ConstNotHostOutsideKernel = acc_on_device(acc_device_not_host)};
  printf("  outside kernel: %d\n", ConstNotHostOutsideKernel);
  #pragma acc parallel num_gangs(1)
  {
    enum {ConstNotHostInsideKernel = acc_on_device(acc_device_not_host)};
    printf("  inside kernel:  %d\n", ConstNotHostInsideKernel);
  }
#endif
  return 0;
}
