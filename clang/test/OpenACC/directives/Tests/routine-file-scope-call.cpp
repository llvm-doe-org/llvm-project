// Clang used to fail an assertion for function calls at file scope because
// the OpenACC routine directive analysis assumed functions are called only
// within other functions, as in C.

// We have written no dump or print checks, so the generated dump and print
// tests are dummies.
//
// RUN: %acc-check-exe{clang-args: -Wno-openacc-and-cxx -xc++}

// END.

/* expected-no-diagnostics */

#include <stdio.h>

// EXE-NOT: {{.}}

int HostFunc() { printf("HostFunc called\n"); return 0; }

#pragma acc routine seq
int SeqFunc() { printf("SeqFunc called\n"); return 0; }

struct HostCtor { HostCtor() { printf("HostCtor ctor called\n"); } };

struct SeqCtor { SeqCtor(); };
#pragma acc routine seq
SeqCtor::SeqCtor() { printf("SeqCtor ctor called\n"); }

//      EXE: HostFunc called
// EXE-NEXT: SeqFunc called
// EXE-NEXT: HostCtor ctor called
// EXE-NEXT: SeqCtor ctor called
int HostFunc_fileScopeUser = HostFunc();
int SeqFunc_fileScopeUser = SeqFunc();
HostCtor hostCtor;
SeqCtor seqCtor;

int main() {
  return 0;
}

// EXE-NOT: {{.}}