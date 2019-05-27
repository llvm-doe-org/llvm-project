// RUN: %data omps {
// RUN:   (omp=-fopenmp       )
// RUN:   (omp=-fopenmp=libomp)
// RUN: }
// RUN: %data omps-cc1 {
// RUN:   (omp=-fopenmp)
// RUN: }
// RUN: %data accs {
// RUN:   (acc=-fopenacc                  )
// RUN:   (acc=-fopenacc-ast-print=acc    )
// RUN:   (acc=-fopenacc-ast-print=omp    )
// RUN:   (acc=-fopenacc-ast-print=acc-omp)
// RUN:   (acc=-fopenacc-ast-print=omp-acc)
// RUN:   (acc=-fopenacc-print=acc        )
// RUN:   (acc=-fopenacc-print=omp        )
// RUN:   (acc=-fopenacc-print=acc-omp    )
// RUN:   (acc=-fopenacc-print=omp-acc    )
// RUN: }
// RUN: %for accs {
// RUN:   %clang %[acc] -o %t %s 2>&1 | FileCheck -allow-empty %s
// RUN:   %clang_cc1 %[acc] %s 2>&1 | FileCheck -allow-empty %s
// RUN: }
// RUN: %for omps {
// RUN:   %clang %[omp] -o %t %s 2>&1 | FileCheck -allow-empty %s
// RUN: }
// RUN: %for omps-cc1 {
// RUN:   %clang_cc1 %[omp] %s 2>&1 | FileCheck -allow-empty %s
// RUN: }
// RUN: %for accs {
// RUN:   %for omps {
// RUN:     not %clang %[acc] %[omp] -o %t %s 2>&1 \
// RUN:     | FileCheck -check-prefix ERROR %s
// RUN:     not %clang %[omp] %[acc] -o %t %s 2>&1 \
// RUN:     | FileCheck -check-prefix ERROR %s
// RUN:   }
// RUN:   %for omps-cc1 {
// RUN:     not %clang_cc1 %[acc] %[omp] %s 2>&1 \
// RUN:     | FileCheck -check-prefix ERROR %s
// RUN:     not %clang_cc1 %[omp] %[acc] %s 2>&1 \
// RUN:     | FileCheck -check-prefix ERROR %s
// RUN:   }
// RUN: }
// END.

// Braces prevent these directives from matching themselves when printing.
// CHECK-NOT: {{error:}} -fopenacc combined with -fopenmp is not supported
// ERROR: {{error:}} -fopenacc combined with -fopenmp is not supported

int main() {
  return 0;
}
