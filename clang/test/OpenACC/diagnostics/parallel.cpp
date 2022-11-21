// Check "acc parallel" diagnostic cases not covered by parallel.c because they
// are specific to C++.
//
// When ADD_LOOP_TO_PAR is not set, this file checks diagnostics for "acc
// parallel" without "loop".
//
// When ADD_LOOP_TO_PAR is set, it adds "loop" and a for loop to those "acc
// parallel" directives in order to check the same diagnostics but for combined
// "acc parallel loop" directives.

// OpenACC disabled
// RUN: %clang_cc1 -verify=noacc,expected-noacc %s
// RUN: %clang_cc1 -verify=noacc,expected-noacc %s -DADD_LOOP_TO_PAR

// OpenACC enabled
// RUN: %clang_cc1 -fopenacc -Wno-openacc-and-cxx %s \
// RUN:   -verify=par,expected,expected-noacc
// RUN: %clang_cc1 -fopenacc -Wno-openacc-and-cxx %s -DADD_LOOP_TO_PAR \
// RUN:   -verify=parloop,expected,expected-noacc

// END.

// noacc-no-diagnostics

#if !ADD_LOOP_TO_PAR
# define LOOP
# define FORLOOP ;
#else
# define LOOP loop
# define FORLOOP for (int i = 0; i < 2; ++i) ;
#endif

class C {
public:
  int i;
  int fn();
} obj;

int main() {
  //----------------------------------------------------------------------------
  // Data clauses: arg semantics
  //----------------------------------------------------------------------------

  // Member expression not permitted: data member.

  // par-error@+6 {{in 'firstprivate' clause on '#pragma acc parallel', member expression is not supported}}
  // par-error@+6 {{in 'private' clause on '#pragma acc parallel', member expression is not supported}}
  // par-error@+6 {{in 'reduction' clause on '#pragma acc parallel', member expression is not supported}}
  // parloop-error@+3 {{in 'firstprivate' clause on '#pragma acc parallel loop', member expression is not supported}}
  // parloop-error@+3 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported}}
  // parloop-error@+3 {{in 'reduction' clause on '#pragma acc parallel loop', member expression is not supported}}
  #pragma acc parallel LOOP firstprivate(obj.i)  \
                            private(obj.i)       \
                            reduction(max:obj.i)
    FORLOOP

  // Member expression not permitted: member function.

  // expected-error@+2 {{expected data member}}
  #pragma acc parallel LOOP present(obj.    \
                                        fn  \
                                          )
    FORLOOP
  // expected-error@+13 {{expected data member}}
  // expected-error@+13 {{expected data member}}
  // expected-error@+13 {{expected data member}}
  // expected-error@+13 {{expected data member}}
  // expected-error@+13 {{expected data member}}
  // expected-error@+13 {{expected data member}}
  // expected-error@+13 {{expected data member}}
  // expected-error@+13 {{expected data member}}
  // expected-error@+13 {{expected data member}}
  // expected-error@+13 {{expected data member}}
  // expected-error@+13 {{expected data member}}
  // expected-error@+13 {{expected data member}}
  // expected-error@+13 {{expected data member}}
  #pragma acc parallel LOOP copy(obj.fn)               \
                            pcopy(obj.fn)              \
                            present_or_copy(obj.fn)    \
                            copyin(obj.fn)             \
                            pcopyin(obj.fn)            \
                            present_or_copyin(obj.fn)  \
                            copyout(obj.fn)            \
                            pcopyout(obj.fn)           \
                            present_or_copyout(obj.fn) \
                            create(obj.fn)             \
                            pcreate(obj.fn)            \
                            present_or_create(obj.fn)  \
                            no_create(obj.fn)
    FORLOOP
  // par-error@+3 {{in 'firstprivate' clause on '#pragma acc parallel', member expression is not supported}}
  // parloop-error@+2 {{in 'firstprivate' clause on '#pragma acc parallel loop', member expression is not supported}}
  // expected-error@+2 {{expected data member}}
  #pragma acc parallel LOOP firstprivate(obj.    \
                                             fn  \
                                               )
    FORLOOP
  // par-error@+3 {{in 'private' clause on '#pragma acc parallel', member expression is not supported}}
  // parloop-error@+2 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported}}
  // expected-error@+2 {{expected data member}}
  #pragma acc parallel LOOP private(obj.    \
                                        fn  \
                                          )
    FORLOOP
  // par-error@+3 {{in 'reduction' clause on '#pragma acc parallel', member expression is not supported}}
  // parloop-error@+2 {{in 'reduction' clause on '#pragma acc parallel loop', member expression is not supported}}
  // expected-error@+2 {{expected data member}}
  #pragma acc parallel LOOP reduction(+:obj.    \
                                            fn  \
                                              )
    FORLOOP

  // Member expression not permitted: member function call.

  // expected-error@+14 {{expected data member}}
  // expected-error@+14 {{expected data member}}
  // expected-error@+14 {{expected data member}}
  // expected-error@+14 {{expected data member}}
  // expected-error@+14 {{expected data member}}
  // expected-error@+14 {{expected data member}}
  // expected-error@+14 {{expected data member}}
  // expected-error@+14 {{expected data member}}
  // expected-error@+14 {{expected data member}}
  // expected-error@+14 {{expected data member}}
  // expected-error@+14 {{expected data member}}
  // expected-error@+14 {{expected data member}}
  // expected-error@+14 {{expected data member}}
  // expected-error@+14 {{expected data member}}
  #pragma acc parallel LOOP present(obj.fn())            \
                            copy(obj.fn())               \
                            pcopy(obj.fn())              \
                            present_or_copy(obj.fn())    \
                            copyin(obj.fn())             \
                            pcopyin(obj.fn())            \
                            present_or_copyin(obj.fn())  \
                            copyout(obj.fn())            \
                            pcopyout(obj.fn())           \
                            present_or_copyout(obj.fn()) \
                            create(obj.fn())             \
                            pcreate(obj.fn())            \
                            present_or_create(obj.fn())  \
                            no_create(obj.fn())
    FORLOOP

  // par-error@+9 {{in 'firstprivate' clause on '#pragma acc parallel', member expression is not supported}}
  // par-error@+9 {{in 'private' clause on '#pragma acc parallel', member expression is not supported}}
  // par-error@+9 {{in 'reduction' clause on '#pragma acc parallel', member expression is not supported}}
  // parloop-error@+6 {{in 'firstprivate' clause on '#pragma acc parallel loop', member expression is not supported}}
  // parloop-error@+6 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported}}
  // parloop-error@+6 {{in 'reduction' clause on '#pragma acc parallel loop', member expression is not supported}}
  // expected-error@+3 {{expected data member}}
  // expected-error@+3 {{expected data member}}
  // expected-error@+3 {{expected data member}}
  #pragma acc parallel LOOP firstprivate(obj.fn())       \
                            private(obj.fn())            \
                            reduction(max:obj.fn())
    FORLOOP

  return 0;
}
