// RUN: %data stds {
// RUN:   (std=          )
// RUN:   (std=-std=c++98)
// RUN:   (std=-std=c++20)
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
// RUN:     not %clang %[std] %[acc] %s 2>&1 | FileCheck --check-prefix=ERROR %s
// RUN:     %clang -Wno-error=openacc-and-cxx %[std] %[acc] %s 2>&1 | \
// RUN:         FileCheck --check-prefix=WARN %s
// RUN:     %clang -Wno-openacc-and-cxx %[std] %[acc] %s 2>&1 | \
// RUN:         FileCheck -allow-empty %s
// RUN:   }
// RUN: }
// END.

// Braces prevent these directives from matching themselves when printing.
//  CHECK-NOT: {{error:}} OpenACC support for C++ is highly experimental
//      ERROR: {{error:}} OpenACC support for C++ is highly experimental
//       WARN: {{warning:}} OpenACC support for C++ is highly experimental
// ERROR-NEXT: {{note:}} you can disable this diagnostic with '-Wno-openacc-and-cxx'
//  WARN-NEXT: {{note:}} you can disable this diagnostic with '-Wno-openacc-and-cxx'

int main() {
  return 0;
}
