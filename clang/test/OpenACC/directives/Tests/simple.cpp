// Check that OpenACC analysis doesn't break a simple C++ program that doesn't
// happen to use OpenACC directives.
//
// We have written no dump or print checks, so the generated dump and print
// tests are dummies.

// RUN: %acc-check-exe{clang-args: -Wno-openacc-and-cxx -xc++}

// END.

/* expected-no-diagnostics */

#include <stdio.h>

// EXE-NOT: {{.}}

int f() { printf("f called\n"); return 0; }

struct S {
  S() { printf("S ctor called\n"); }
};

// Clang used to fail an assertion at the function call and the constructor call
// here because the OpenACC routine directive analysis assumed functions are
// called only within other functions, as in C.
//
//      EXE: f called
// EXE-NEXT: S ctor called
int i = f();
S s;

int main() {
  return 0;
}

// EXE-NOT: {{.}}