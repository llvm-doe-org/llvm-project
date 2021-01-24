// RUN: %clang_cc1 -verify -fopenmp -x c++ -triple x86_64-unknown-linux \
// RUN:     -fopenmp-targets=x86_64-unknown-linux -emit-llvm-bc %s \
// RUN:     -o %t-host.bc -fopenmp-version=50
// RUN: %clang_cc1 -verify -fopenmp -x c++ -triple x86_64-unknown-linux \
// RUN:     -emit-llvm %s -fopenmp-is-device \
// RUN:     -fopenmp-host-ir-file-path %t-host.bc -o - -fopenmp-version=50 \
// RUN: | FileCheck %s --implicit-check-not='call i32 {@_Z3bazv|@_Z3barv}'
// RUN: %clang_cc1 -verify -fopenmp -x c++ -triple x86_64-unknown-linux \
// RUN:     -emit-llvm %s -fopenmp-is-device \
// RUN:     -fopenmp-host-ir-file-path %t-host.bc -emit-pch -o %t \
// RUN:     -fopenmp-version=50
// RUN: %clang_cc1 -verify -fopenmp -x c++ -triple x86_64-unknown-linux \
// RUN:     -emit-llvm %s -fopenmp-is-device \
// RUN:     -fopenmp-host-ir-file-path %t-host.bc -include-pch %t -o - \
// RUN:     -fopenmp-version=50 \
// RUN: | FileCheck %s --implicit-check-not='call i32 {@_Z3bazv|@_Z3barv}'

// expected-no-diagnostics

// CHECK-DAG: @_Z3barv
// CHECK-DAG: @_Z3bazv
// CHECK-DAG: @"_Z51bar$ompvariant$S2$s8$Px86$Px86_64$S3$s10$Pmatch_anyv"
// CHECK-DAG: @"_Z51baz$ompvariant$S2$s8$Px86$Px86_64$S3$s10$Pmatch_anyv"
// CHECK-DAG: call i32 @"_Z51bar$ompvariant$S2$s8$Px86$Px86_64$S3$s10$Pmatch_anyv"()
// CHECK-DAG: call i32 @"_Z51baz$ompvariant$S2$s8$Px86$Px86_64$S3$s10$Pmatch_anyv"()

#ifndef HEADER
#define HEADER

#pragma omp declare target

int bar() { return 1; }

int baz() { return 5; }

#pragma omp begin declare variant match(device = {arch(x86, x86_64)}, \
                                        implementation = {extension(match_any)})

int bar() { return 2; }

int baz() { return 6; }

#pragma omp end declare variant

#pragma omp end declare target

int main() {
  int res;
#pragma omp target map(from : res)
  res = bar() + baz();
  return res;
}

#endif
