// RUN: %data stds {
// RUN:   (std=          )
// RUN:   (std=-std=c++11)
// RUN:   (std=-std=c++14)
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
// RUN: %for stds {
// RUN:   %clang %[std] -o %t %s 2>&1 | FileCheck -allow-empty %s
// RUN:   %for accs {
// RUN:     not %clang %[std] %[acc] %s 2>&1 \
// RUN:     | FileCheck --check-prefix ERROR %s
// RUN:   }
// RUN: }

// CHECK-NOT: error: OpenACC support for C++ not yet implemented
// ERROR:     error: OpenACC support for C++ not yet implemented

int main() {
  return 0;
}
