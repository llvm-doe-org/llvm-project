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

struct S {
  int i = 0;
  int fn();
};

class C {
public:
  int i = 0;
  const int constMember = 0; // #C_constMember
  int fn();
  S s;
};

class Main {
  int thisMember = 0;
  const int thisConstMember = 0; // #Main_thisConstMember
  S thisStructMember;
  void thisMemberFn();

  //----------------------------------------------------------------------------
  // Data clauses: arg semantics in non-const member function.
  //----------------------------------------------------------------------------

  void testInNonConstMember() {
    class C local;
    const class C constLocal; // #testInNonConstMember_constLocal

    //..........................................................................
    // Member expression not permitted: data member.

    // par-error-re@+6 {{in 'firstprivate' clause on '#pragma acc parallel', member expression is not supported except on C++ 'this' pointer{{$}}}}
    // par-error-re@+6 {{in 'private' clause on '#pragma acc parallel', member expression is not supported except on C++ 'this' pointer{{$}}}}
    // par-error-re@+6 {{in 'reduction' clause on '#pragma acc parallel', member expression is not supported except on C++ 'this' pointer{{$}}}}
    // parloop-error-re@+3 {{in 'firstprivate' clause on '#pragma acc parallel loop', member expression is not supported except on C++ 'this' pointer{{$}}}}
    // parloop-error-re@+3 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
    // parloop-error-re@+3 {{in 'reduction' clause on '#pragma acc parallel loop', member expression is not supported except on C++ 'this' pointer{{$}}}}
    #pragma acc parallel LOOP firstprivate(local.i)  \
                              private(local.i)       \
                              reduction(max:local.i)
    FORLOOP
    #pragma acc parallel LOOP firstprivate(this->thisMember)
    FORLOOP
    // parloop-error-re@+1 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
    #pragma acc parallel LOOP private(this->thisMember)
    FORLOOP
    #pragma acc parallel LOOP reduction(max:this->thisMember)
    FORLOOP
    #pragma acc parallel LOOP firstprivate(thisMember)
    FORLOOP
    // parloop-error-re@+1 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported even on implicit C++ 'this' pointer{{$}}}}
    #pragma acc parallel LOOP private(thisMember)
    FORLOOP
    #pragma acc parallel LOOP reduction(+:thisMember)
    FORLOOP

    //..........................................................................
    // Member expression not permitted: member function.

    // expected-error@+2 {{expected data member}}
    #pragma acc parallel LOOP present(local.    \
                                            fn  \
                                              )
    FORLOOP
    // expected-error@+2 {{expected data member}}
    #pragma acc parallel LOOP present(this->              \
                                            thisMemberFn  \
                                                        )
    FORLOOP
    // expected-error-re@+1 {{expected variable name or data member expression or subarray{{$}}}}
    #pragma acc parallel LOOP present(thisMemberFn)
    FORLOOP

    // expected-error@+13 2 {{expected data member}}
    // expected-error@+13 2 {{expected data member}}
    // expected-error@+13 2 {{expected data member}}
    // expected-error@+13 2 {{expected data member}}
    // expected-error@+13 2 {{expected data member}}
    // expected-error@+13 2 {{expected data member}}
    // expected-error@+13 2 {{expected data member}}
    // expected-error@+13 2 {{expected data member}}
    // expected-error@+13 2 {{expected data member}}
    // expected-error@+13 2 {{expected data member}}
    // expected-error@+13 2 {{expected data member}}
    // expected-error@+13 2 {{expected data member}}
    // expected-error@+13 2 {{expected data member}}
    #pragma acc parallel LOOP copy(local.fn, this->thisMemberFn)               \
                              pcopy(local.fn, this->thisMemberFn)              \
                              present_or_copy(local.fn, this->thisMemberFn)    \
                              copyin(local.fn, this->thisMemberFn)             \
                              pcopyin(local.fn, this->thisMemberFn)            \
                              present_or_copyin(local.fn, this->thisMemberFn)  \
                              copyout(local.fn, this->thisMemberFn)            \
                              pcopyout(local.fn, this->thisMemberFn)           \
                              present_or_copyout(local.fn, this->thisMemberFn) \
                              create(local.fn, this->thisMemberFn)             \
                              pcreate(local.fn, this->thisMemberFn)            \
                              present_or_create(local.fn, this->thisMemberFn)  \
                              no_create(local.fn, this->thisMemberFn)
    FORLOOP
    // expected-error-re@+13 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+13 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+13 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+13 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+13 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+13 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+13 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+13 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+13 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+13 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+13 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+13 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+13 {{expected variable name or data member expression or subarray{{$}}}}
    #pragma acc parallel LOOP copy(thisMemberFn)               \
                              pcopy(thisMemberFn)              \
                              present_or_copy(thisMemberFn)    \
                              copyin(thisMemberFn)             \
                              pcopyin(thisMemberFn)            \
                              present_or_copyin(thisMemberFn)  \
                              copyout(thisMemberFn)            \
                              pcopyout(thisMemberFn)           \
                              present_or_copyout(thisMemberFn) \
                              create(thisMemberFn)             \
                              pcreate(thisMemberFn)            \
                              present_or_create(thisMemberFn)  \
                              no_create(thisMemberFn)
    FORLOOP

    // par-error-re@+3 {{in 'firstprivate' clause on '#pragma acc parallel', member expression is not supported except on C++ 'this' pointer{{$}}}}
    // parloop-error-re@+2 {{in 'firstprivate' clause on '#pragma acc parallel loop', member expression is not supported except on C++ 'this' pointer{{$}}}}
    // expected-error@+2 {{expected data member}}
    #pragma acc parallel LOOP firstprivate(local.    \
                                                 fn  \
                                                   )
    FORLOOP
    // expected-error@+2 {{expected data member}}
    #pragma acc parallel LOOP firstprivate(this->              \
                                                 thisMemberFn  \
                                                               )
    FORLOOP
    // expected-error-re@+1 {{expected variable name or data member expression{{$}}}}
    #pragma acc parallel LOOP firstprivate(thisMemberFn)
    FORLOOP

    // par-error-re@+3 {{in 'private' clause on '#pragma acc parallel', member expression is not supported except on C++ 'this' pointer{{$}}}}
    // parloop-error-re@+2 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
    // expected-error@+2 {{expected data member}}
    #pragma acc parallel LOOP private(local.    \
                                            fn  \
                                              )
    FORLOOP
    // parloop-error-re@+2 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
    // expected-error@+2 {{expected data member}}
    #pragma acc parallel LOOP private(this->              \
                                            thisMemberFn  \
                                                        )
    FORLOOP
    // par-error-re@+3 {{expected variable name or data member expression{{$}}}}
    // parloop-error-re@+2 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported even on implicit C++ 'this' pointer{{$}}}}
    // parloop-error-re@+1 {{expected variable name{{$}}}}
    #pragma acc parallel LOOP private(thisMemberFn)
    FORLOOP

    // par-error-re@+3 {{in 'reduction' clause on '#pragma acc parallel', member expression is not supported except on C++ 'this' pointer{{$}}}}
    // parloop-error-re@+2 {{in 'reduction' clause on '#pragma acc parallel loop', member expression is not supported except on C++ 'this' pointer{{$}}}}
    // expected-error@+2 {{expected data member}}
    #pragma acc parallel LOOP reduction(+:local.    \
                                                fn  \
                                                  )
    FORLOOP
    // expected-error@+2 {{expected data member}}
    #pragma acc parallel LOOP reduction(+:this->              \
                                                thisMemberFn  \
                                                            )
    FORLOOP
    // expected-error-re@+1 {{expected variable name or data member expression{{$}}}}
    #pragma acc parallel LOOP reduction(+:thisMemberFn)
    FORLOOP

    //..........................................................................
    // Member expression not permitted: member function call.

    // expected-error@+2 {{expected data member}}
    #pragma acc parallel LOOP present(local.     \
                                            fn()  \
                                                )
    FORLOOP
    // expected-error@+2 {{expected data member}}
    #pragma acc parallel LOOP present(this->                \
                                            thisMemberFn()  \
                                                          )
    FORLOOP
    // expected-error-re@+1 {{expected variable name or data member expression or subarray{{$}}}}
    #pragma acc parallel LOOP present(thisMemberFn())
    FORLOOP

    // expected-error@+13 2 {{expected data member}}
    // expected-error@+13 2 {{expected data member}}
    // expected-error@+13 2 {{expected data member}}
    // expected-error@+13 2 {{expected data member}}
    // expected-error@+13 2 {{expected data member}}
    // expected-error@+13 2 {{expected data member}}
    // expected-error@+13 2 {{expected data member}}
    // expected-error@+13 2 {{expected data member}}
    // expected-error@+13 2 {{expected data member}}
    // expected-error@+13 2 {{expected data member}}
    // expected-error@+13 2 {{expected data member}}
    // expected-error@+13 2 {{expected data member}}
    // expected-error@+13 2 {{expected data member}}
    #pragma acc parallel LOOP copy(local.fn(), this->thisMemberFn())               \
                              pcopy(local.fn(), this->thisMemberFn())              \
                              present_or_copy(local.fn(), this->thisMemberFn())    \
                              copyin(local.fn(), this->thisMemberFn())             \
                              pcopyin(local.fn(), this->thisMemberFn())            \
                              present_or_copyin(local.fn(), this->thisMemberFn())  \
                              copyout(local.fn(), this->thisMemberFn())            \
                              pcopyout(local.fn(), this->thisMemberFn())           \
                              present_or_copyout(local.fn(), this->thisMemberFn()) \
                              create(local.fn(), this->thisMemberFn())             \
                              pcreate(local.fn(), this->thisMemberFn())            \
                              present_or_create(local.fn(), this->thisMemberFn())  \
                              no_create(local.fn(), this->thisMemberFn())
    FORLOOP
    // expected-error-re@+13 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+13 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+13 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+13 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+13 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+13 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+13 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+13 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+13 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+13 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+13 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+13 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+13 {{expected variable name or data member expression or subarray{{$}}}}
    #pragma acc parallel LOOP copy(thisMemberFn())               \
                              pcopy(thisMemberFn())              \
                              present_or_copy(thisMemberFn())    \
                              copyin(thisMemberFn())             \
                              pcopyin(thisMemberFn())            \
                              present_or_copyin(thisMemberFn())  \
                              copyout(thisMemberFn())            \
                              pcopyout(thisMemberFn())           \
                              present_or_copyout(thisMemberFn()) \
                              create(thisMemberFn())             \
                              pcreate(thisMemberFn())            \
                              present_or_create(thisMemberFn())  \
                              no_create(thisMemberFn())
    FORLOOP

    // par-error-re@+3 {{in 'firstprivate' clause on '#pragma acc parallel', member expression is not supported except on C++ 'this' pointer{{$}}}}
    // parloop-error-re@+2 {{in 'firstprivate' clause on '#pragma acc parallel loop', member expression is not supported except on C++ 'this' pointer{{$}}}}
    // expected-error@+2 {{expected data member}}
    #pragma acc parallel LOOP firstprivate(local.      \
                                                 fn()  \
                                                     )
    FORLOOP
    // expected-error@+2 {{expected data member}}
    #pragma acc parallel LOOP firstprivate(this->                \
                                                 thisMemberFn()  \
                                                               )
    FORLOOP
    // expected-error-re@+1 {{expected variable name or data member expression{{$}}}}
    #pragma acc parallel LOOP firstprivate(thisMemberFn())
    FORLOOP

    // par-error-re@+3 {{in 'private' clause on '#pragma acc parallel', member expression is not supported except on C++ 'this' pointer{{$}}}}
    // parloop-error-re@+2 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
    // expected-error@+2 {{expected data member}}
    #pragma acc parallel LOOP private(local.    \
                                            fn()  \
                                              )
    FORLOOP
    // parloop-error-re@+2 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
    // expected-error@+2 {{expected data member}}
    #pragma acc parallel LOOP private(this->                \
                                            thisMemberFn()  \
                                                          )
    FORLOOP
    // par-error-re@+3 {{expected variable name or data member expression{{$}}}}
    // parloop-error-re@+2 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported even on implicit C++ 'this' pointer{{$}}}}
    // parloop-error-re@+1 {{expected variable name{{$}}}}
    #pragma acc parallel LOOP private(thisMemberFn())
    FORLOOP

    // par-error-re@+3 {{in 'reduction' clause on '#pragma acc parallel', member expression is not supported except on C++ 'this' pointer{{$}}}}
    // parloop-error-re@+2 {{in 'reduction' clause on '#pragma acc parallel loop', member expression is not supported except on C++ 'this' pointer{{$}}}}
    // expected-error@+2 {{expected data member}}
    #pragma acc parallel LOOP reduction(+:local.    \
                                                fn()  \
                                                  )
    FORLOOP
    // expected-error@+2 {{expected data member}}
    #pragma acc parallel LOOP reduction(+:this->                \
                                                thisMemberFn()  \
                                                              )
    FORLOOP
    // expected-error-re@+1 {{expected variable name or data member expression{{$}}}}
    #pragma acc parallel LOOP reduction(+:thisMemberFn())
    FORLOOP

    //..........................................................................
    // Nested member expressions not permitted: data member.

    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for local.s
    #pragma acc parallel LOOP present(local.s.i)            \
                              copy(local.s.i)               \
                              pcopy(local.s.i)              \
                              present_or_copy(local.s.i)    \
                              copyin(local.s.i)             \
                              pcopyin(local.s.i)            \
                              present_or_copyin(local.s.i)  \
                              copyout(local.s.i)            \
                              pcopyout(local.s.i)           \
                              present_or_copyout(local.s.i) \
                              create(local.s.i)             \
                              pcreate(local.s.i)            \
                              present_or_create(local.s.i)  \
                              no_create(local.s.i)
    FORLOOP
    // expected-error-re@+2 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+2 {{nested member expression is not supported{{$}}}} // range is for local.s
    #pragma acc parallel LOOP firstprivate(local.s.i) \
                              reduction(+:local.s.i)
    FORLOOP
    // parloop-error-re@+2 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
    // expected-error-re@+1 {{nested member expression is not supported{{$}}}} // range is for local.s
    #pragma acc parallel LOOP private(local.s.i)
    FORLOOP

    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+14 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    #pragma acc parallel LOOP present(this->thisStructMember.i)            \
                              copy(this->thisStructMember.i)               \
                              pcopy(this->thisStructMember.i)              \
                              present_or_copy(this->thisStructMember.i)    \
                              copyin(this->thisStructMember.i)             \
                              pcopyin(this->thisStructMember.i)            \
                              present_or_copyin(this->thisStructMember.i)  \
                              copyout(this->thisStructMember.i)            \
                              pcopyout(this->thisStructMember.i)           \
                              present_or_copyout(this->thisStructMember.i) \
                              create(this->thisStructMember.i)             \
                              pcreate(this->thisStructMember.i)            \
                              present_or_create(this->thisStructMember.i)  \
                              no_create(this->thisStructMember.i)
    FORLOOP
    // expected-error-re@+2 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+2 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    #pragma acc parallel LOOP firstprivate(this->thisStructMember.i) \
                              reduction(+:this->thisStructMember.i)
    FORLOOP
    // parloop-error-re@+2 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
    // expected-error-re@+1 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    #pragma acc parallel LOOP private(this->thisStructMember.i)
    FORLOOP

    // expected-error-re@+14 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+14 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+14 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+14 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+14 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+14 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+14 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+14 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+14 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+14 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+14 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+14 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+14 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+14 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    #pragma acc parallel LOOP present(thisStructMember.i)            \
                              copy(thisStructMember.i)               \
                              pcopy(thisStructMember.i)              \
                              present_or_copy(thisStructMember.i)    \
                              copyin(thisStructMember.i)             \
                              pcopyin(thisStructMember.i)            \
                              present_or_copyin(thisStructMember.i)  \
                              copyout(thisStructMember.i)            \
                              pcopyout(thisStructMember.i)           \
                              present_or_copyout(thisStructMember.i) \
                              create(thisStructMember.i)             \
                              pcreate(thisStructMember.i)            \
                              present_or_create(thisStructMember.i)  \
                              no_create(thisStructMember.i)
    FORLOOP
    // expected-error-re@+2 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+2 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    #pragma acc parallel LOOP firstprivate(thisStructMember.i) \
                              reduction(+:thisStructMember.i)
    FORLOOP
    // parloop-error-re@+2 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
    // expected-error-re@+1 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    #pragma acc parallel LOOP private(thisStructMember.i)
    FORLOOP

    //..........................................................................
    // Nested member expressions not permitted: member function call.

    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for local.s
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
    #pragma acc parallel LOOP present(local.s.fn())            \
                              copy(local.s.fn())               \
                              pcopy(local.s.fn())              \
                              present_or_copy(local.s.fn())    \
                              copyin(local.s.fn())             \
                              pcopyin(local.s.fn())            \
                              present_or_copyin(local.s.fn())  \
                              copyout(local.s.fn())            \
                              pcopyout(local.s.fn())           \
                              present_or_copyout(local.s.fn()) \
                              create(local.s.fn())             \
                              pcreate(local.s.fn())            \
                              present_or_create(local.s.fn())  \
                              no_create(local.s.fn())
    FORLOOP
    // expected-error-re@+4 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+4 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error@+2 {{expected data member}}
    // expected-error@+2 {{expected data member}}
    #pragma acc parallel LOOP firstprivate(local.s.fn()) \
                              reduction(+:local.s.fn())
    FORLOOP
    // parloop-error-re@+3 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
    // expected-error-re@+2 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error@+1 {{expected data member}}
    #pragma acc parallel LOOP private(local.s.fn())
    FORLOOP

    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+28 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
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
    #pragma acc parallel LOOP present(this->thisStructMember.fn())            \
                              copy(this->thisStructMember.fn())               \
                              pcopy(this->thisStructMember.fn())              \
                              present_or_copy(this->thisStructMember.fn())    \
                              copyin(this->thisStructMember.fn())             \
                              pcopyin(this->thisStructMember.fn())            \
                              present_or_copyin(this->thisStructMember.fn())  \
                              copyout(this->thisStructMember.fn())            \
                              pcopyout(this->thisStructMember.fn())           \
                              present_or_copyout(this->thisStructMember.fn()) \
                              create(this->thisStructMember.fn())             \
                              pcreate(this->thisStructMember.fn())            \
                              present_or_create(this->thisStructMember.fn())  \
                              no_create(this->thisStructMember.fn())
    FORLOOP
    // expected-error-re@+4 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+4 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error@+2 {{expected data member}}
    // expected-error@+2 {{expected data member}}
    #pragma acc parallel LOOP firstprivate(this->thisStructMember.fn()) \
                              reduction(+:this->thisStructMember.fn())
    FORLOOP
    // parloop-error-re@+3 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
    // expected-error-re@+2 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error@+1 {{expected data member}}
    #pragma acc parallel LOOP private(this->thisStructMember.fn())
    FORLOOP

    // expected-error-re@+28 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+28 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+28 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+28 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+28 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+28 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+28 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+28 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+28 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+28 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+28 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+28 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+28 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+28 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
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
    #pragma acc parallel LOOP present(thisStructMember.fn())            \
                              copy(thisStructMember.fn())               \
                              pcopy(thisStructMember.fn())              \
                              present_or_copy(thisStructMember.fn())    \
                              copyin(thisStructMember.fn())             \
                              pcopyin(thisStructMember.fn())            \
                              present_or_copyin(thisStructMember.fn())  \
                              copyout(thisStructMember.fn())            \
                              pcopyout(thisStructMember.fn())           \
                              present_or_copyout(thisStructMember.fn()) \
                              create(thisStructMember.fn())             \
                              pcreate(thisStructMember.fn())            \
                              present_or_create(thisStructMember.fn())  \
                              no_create(thisStructMember.fn())
    FORLOOP
    // expected-error-re@+4 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+4 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error@+2 {{expected data member}}
    // expected-error@+2 {{expected data member}}
    #pragma acc parallel LOOP firstprivate(thisStructMember.fn()) \
                              reduction(+:thisStructMember.fn())
    FORLOOP
    // parloop-error-re@+3 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
    // expected-error-re@+2 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error@+1 {{expected data member}}
    #pragma acc parallel LOOP private(thisStructMember.fn())
    FORLOOP

    //..........................................................................
    // Subarray not permitted: even though this[0:1] is special, it is still
    // treated as a subarray in this case.

    // expected-error@+16 {{subarray syntax must include ':'}}
    // par-error@+11 {{in 'firstprivate' clause on '#pragma acc parallel', subarray is not supported}}
    // par-error@+11 {{in 'firstprivate' clause on '#pragma acc parallel', subarray is not supported}}
    // par-error@+11 {{subarray length is unspecified and cannot be inferred because subscripted value is not an array}}
    // par-error@+11 {{subarray length is unspecified and cannot be inferred because subscripted value is not an array}}
    // par-error@+11 {{in 'firstprivate' clause on '#pragma acc parallel', subarray is not supported}}
    // parloop-error@+6 {{in 'firstprivate' clause on '#pragma acc parallel loop', subarray is not supported}}
    // parloop-error@+6 {{in 'firstprivate' clause on '#pragma acc parallel loop', subarray is not supported}}
    // parloop-error@+6 {{subarray length is unspecified and cannot be inferred because subscripted value is not an array}}
    // parloop-error@+6 {{subarray length is unspecified and cannot be inferred because subscripted value is not an array}}
    // parloop-error@+6 {{in 'firstprivate' clause on '#pragma acc parallel loop', subarray is not supported}}
    // expected-error-re@+5 {{expected variable name or data member expression{{$}}}}
    #pragma acc parallel LOOP firstprivate(this[0:1], \
                                           this[:1],  \
                                           this[0:],  \
                                           this[:],   \
                                           this[0])
    FORLOOP
    // expected-error@+17 {{subarray syntax must include ':'}}
    // par-error@+12 {{in 'private' clause on '#pragma acc parallel', subarray is not supported}}
    // par-error@+12 {{in 'private' clause on '#pragma acc parallel', subarray is not supported}}
    // par-error@+12 {{subarray length is unspecified and cannot be inferred because subscripted value is not an array}}
    // par-error@+12 {{subarray length is unspecified and cannot be inferred because subscripted value is not an array}}
    // par-error@+12 {{in 'private' clause on '#pragma acc parallel', subarray is not supported}}
    // parloop-error@+7 {{in 'private' clause on '#pragma acc parallel loop', subarray is not supported}}
    // parloop-error@+7 {{in 'private' clause on '#pragma acc parallel loop', subarray is not supported}}
    // parloop-error@+7 {{subarray length is unspecified and cannot be inferred because subscripted value is not an array}}
    // parloop-error@+7 {{subarray length is unspecified and cannot be inferred because subscripted value is not an array}}
    // parloop-error@+7 {{in 'private' clause on '#pragma acc parallel loop', subarray is not supported}}
    // par-error-re@+6 {{expected variable name or data member expression{{$}}}}
    // parloop-error-re@+5 {{expected variable name{{$}}}}
    #pragma acc parallel LOOP private(this[0:1], \
                                      this[:1],  \
                                      this[0:],  \
                                      this[:],   \
                                      this[0])
    FORLOOP
    // expected-error@+16 {{subarray syntax must include ':'}}
    // par-error@+11 {{in 'reduction' clause on '#pragma acc parallel', subarray is not supported}}
    // par-error@+11 {{in 'reduction' clause on '#pragma acc parallel', subarray is not supported}}
    // par-error@+11 {{subarray length is unspecified and cannot be inferred because subscripted value is not an array}}
    // par-error@+11 {{subarray length is unspecified and cannot be inferred because subscripted value is not an array}}
    // par-error@+11 {{in 'reduction' clause on '#pragma acc parallel', subarray is not supported}}
    // parloop-error@+6 {{in 'reduction' clause on '#pragma acc parallel loop', subarray is not supported}}
    // parloop-error@+6 {{in 'reduction' clause on '#pragma acc parallel loop', subarray is not supported}}
    // parloop-error@+6 {{subarray length is unspecified and cannot be inferred because subscripted value is not an array}}
    // parloop-error@+6 {{subarray length is unspecified and cannot be inferred because subscripted value is not an array}}
    // parloop-error@+6 {{in 'reduction' clause on '#pragma acc parallel loop', subarray is not supported}}
    // expected-error-re@+5 {{expected variable name or data member expression{{$}}}}
    #pragma acc parallel LOOP reduction(min:this[0:1], \
                                            this[:1],  \
                                            this[0:],  \
                                            this[:],   \
                                            this[0])
    FORLOOP

    //..........................................................................
    // Bare 'this' not permitted because it's not a variable/lvalue.

    // expected-error-re@+7 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+7 3 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+7 3 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+7 3 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+7 3 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+7 {{expected variable name or data member expression or subarray{{$}}}}
    #pragma acc parallel LOOP                               \
      present(this)                                         \
      copy(this) pcopy(this) present_or_copy(this)          \
      copyin(this) pcopyin(this) present_or_copyin(this)    \
      copyout(this) pcopyout(this) present_or_copyout(this) \
      create(this) pcreate(this) present_or_create(this)    \
      no_create(this)
    FORLOOP
    // par-error-re@+2 {{expected variable name or data member expression{{$}}}}
    // parloop-error-re@+1 {{expected variable name{{$}}}}
    #pragma acc parallel LOOP private(this)
    FORLOOP
    // expected-error-re@+1 {{expected variable name or data member expression{{$}}}}
    #pragma acc parallel LOOP reduction(min:this)
    FORLOOP

    //..........................................................................
    // Reduction operator doesn't permit variable type, but whether it's a
    // reference type is irrelevant.

    {
      bool b0, &b = b0;
      enum { E1, E2 } e0, &e = e0;
      int i0, &i = i0, jk0, &jk = jk0;
      int a0[2], (&a)[2] = a0; // #reduction_ref_a
      int *p0, *&p = p0; // #reduction_ref_p
      float f0, &f = f0; // #reduction_ref_f
      double d0, &d = d0; // #reduction_ref_d
      float _Complex fc0, &fc = fc0; // #reduction_ref_fc
      double _Complex dc0, &dc = dc0; // #reduction_ref_dc
      struct S { int i; } s0, &s = s0; // #reduction_ref_s
      union U { int i; } u0, &u = u0; // #reduction_ref_u
      extern union U &uDecl; // #reduction_ref_uDecl

      #pragma acc parallel LOOP reduction(max:b,e,i,jk,f,d,p)
        FORLOOP
      // expected-error@+7 6 {{OpenACC reduction operator 'max' argument must be of real or pointer type}}
      // expected-note@#reduction_ref_fc {{variable 'fc' declared here}}
      // expected-note@#reduction_ref_dc {{variable 'dc' declared here}}
      // expected-note@#reduction_ref_a {{variable 'a' declared here}}
      // expected-note@#reduction_ref_s {{variable 's' declared here}}
      // expected-note@#reduction_ref_u {{variable 'u' declared here}}
      // expected-note@#reduction_ref_uDecl {{variable 'uDecl' declared here}}
      #pragma acc parallel LOOP reduction(max:fc,dc,a,s,u,uDecl)
        FORLOOP
      #pragma acc parallel LOOP reduction(min:b,e,i,jk,f,d,p)
        FORLOOP
      // expected-error@+7 6 {{OpenACC reduction operator 'min' argument must be of real or pointer type}}
      // expected-note@#reduction_ref_fc {{variable 'fc' declared here}}
      // expected-note@#reduction_ref_dc {{variable 'dc' declared here}}
      // expected-note@#reduction_ref_a {{variable 'a' declared here}}
      // expected-note@#reduction_ref_s {{variable 's' declared here}}
      // expected-note@#reduction_ref_u {{variable 'u' declared here}}
      // expected-note@#reduction_ref_uDecl {{variable 'uDecl' declared here}}
      #pragma acc parallel LOOP reduction(min:fc,dc,a,s,u,uDecl)
        FORLOOP
      #pragma acc parallel LOOP reduction(+:b,e,i,jk,f,d,fc,dc)
        FORLOOP
      // expected-error@+6 5 {{OpenACC reduction operator '+' argument must be of arithmetic type}}
      // expected-note@#reduction_ref_p {{variable 'p' declared here}}
      // expected-note@#reduction_ref_a {{variable 'a' declared here}}
      // expected-note@#reduction_ref_s {{variable 's' declared here}}
      // expected-note@#reduction_ref_u {{variable 'u' declared here}}
      // expected-note@#reduction_ref_uDecl {{variable 'uDecl' declared here}}
      #pragma acc parallel LOOP reduction(+:p,a,s,u,uDecl)
        FORLOOP
      #pragma acc parallel LOOP reduction(*:b,e,i,jk,f,d,fc,dc)
        FORLOOP
      // expected-error@+6 5 {{OpenACC reduction operator '*' argument must be of arithmetic type}}
      // expected-note@#reduction_ref_p {{variable 'p' declared here}}
      // expected-note@#reduction_ref_a {{variable 'a' declared here}}
      // expected-note@#reduction_ref_s {{variable 's' declared here}}
      // expected-note@#reduction_ref_u {{variable 'u' declared here}}
      // expected-note@#reduction_ref_uDecl {{variable 'uDecl' declared here}}
      #pragma acc parallel LOOP reduction(*:p,a,s,u,uDecl)
        FORLOOP
      #pragma acc parallel LOOP reduction(&&:b,e,i,jk,f,d,fc,dc)
        FORLOOP
      // expected-error@+6 5 {{OpenACC reduction operator '&&' argument must be of arithmetic type}}
      // expected-note@#reduction_ref_p {{variable 'p' declared here}}
      // expected-note@#reduction_ref_a {{variable 'a' declared here}}
      // expected-note@#reduction_ref_s {{variable 's' declared here}}
      // expected-note@#reduction_ref_u {{variable 'u' declared here}}
      // expected-note@#reduction_ref_uDecl {{variable 'uDecl' declared here}}
      #pragma acc parallel LOOP reduction(&&:p,a,s,u,uDecl)
        FORLOOP
      #pragma acc parallel LOOP reduction(||:b,e,i,jk,f,d,fc,dc)
        FORLOOP
      // expected-error@+6 5 {{OpenACC reduction operator '||' argument must be of arithmetic type}}
      // expected-note@#reduction_ref_p {{variable 'p' declared here}}
      // expected-note@#reduction_ref_a {{variable 'a' declared here}}
      // expected-note@#reduction_ref_s {{variable 's' declared here}}
      // expected-note@#reduction_ref_u {{variable 'u' declared here}}
      // expected-note@#reduction_ref_uDecl {{variable 'uDecl' declared here}}
      #pragma acc parallel LOOP reduction(||:p,a,s,u,uDecl)
        FORLOOP
      #pragma acc parallel LOOP reduction(&:b,e,i,jk)
        FORLOOP
      // expected-error@+10 9 {{OpenACC reduction operator '&' argument must be of integer type}}
      // expected-note@#reduction_ref_f {{variable 'f' declared here}}
      // expected-note@#reduction_ref_d {{variable 'd' declared here}}
      // expected-note@#reduction_ref_fc {{variable 'fc' declared here}}
      // expected-note@#reduction_ref_dc {{variable 'dc' declared here}}
      // expected-note@#reduction_ref_p {{variable 'p' declared here}}
      // expected-note@#reduction_ref_a {{variable 'a' declared here}}
      // expected-note@#reduction_ref_s {{variable 's' declared here}}
      // expected-note@#reduction_ref_u {{variable 'u' declared here}}
      // expected-note@#reduction_ref_uDecl {{variable 'uDecl' declared here}}
      #pragma acc parallel LOOP reduction(&:f,d,fc,dc,p,a,s,u,uDecl)
        FORLOOP
      #pragma acc parallel LOOP reduction(|:b,e,i,jk)
        FORLOOP
      // expected-error@+10 9 {{OpenACC reduction operator '|' argument must be of integer type}}
      // expected-note@#reduction_ref_f {{variable 'f' declared here}}
      // expected-note@#reduction_ref_d {{variable 'd' declared here}}
      // expected-note@#reduction_ref_fc {{variable 'fc' declared here}}
      // expected-note@#reduction_ref_dc {{variable 'dc' declared here}}
      // expected-note@#reduction_ref_p {{variable 'p' declared here}}
      // expected-note@#reduction_ref_a {{variable 'a' declared here}}
      // expected-note@#reduction_ref_s {{variable 's' declared here}}
      // expected-note@#reduction_ref_u {{variable 'u' declared here}}
      // expected-note@#reduction_ref_uDecl {{variable 'uDecl' declared here}}
      #pragma acc parallel LOOP reduction(|:f,d,fc,dc,p,a,s,u,uDecl)
        FORLOOP
      #pragma acc parallel LOOP reduction(^:b,e,i,jk)
        FORLOOP
      // expected-error@+10 9 {{OpenACC reduction operator '^' argument must be of integer type}}
      // expected-note@#reduction_ref_f {{variable 'f' declared here}}
      // expected-note@#reduction_ref_d {{variable 'd' declared here}}
      // expected-note@#reduction_ref_fc {{variable 'fc' declared here}}
      // expected-note@#reduction_ref_dc {{variable 'dc' declared here}}
      // expected-note@#reduction_ref_p {{variable 'p' declared here}}
      // expected-note@#reduction_ref_a {{variable 'a' declared here}}
      // expected-note@#reduction_ref_s {{variable 's' declared here}}
      // expected-note@#reduction_ref_u {{variable 'u' declared here}}
      // expected-note@#reduction_ref_uDecl {{variable 'uDecl' declared here}}
      #pragma acc parallel LOOP reduction(^:f,d,fc,dc,p,a,s,u,uDecl)
        FORLOOP
    }

    //..........................................................................
    // Conflicting DSAs.

    // par-error@+3 {{private variable defined again as private variable}}
    // par-note@+2 {{previously defined as private variable here}}
    // parloop-error-re@+1 2 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported even on implicit C++ 'this' pointer{{$}}}}
    #pragma acc parallel LOOP private(thisMember) private(thisMember)
    FORLOOP
    // par-error@+3 {{private variable cannot be reduction variable}}
    // par-note@+2 {{previously defined as private variable here}}
    // parloop-error-re@+1 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported even on implicit C++ 'this' pointer{{$}}}}
    #pragma acc parallel LOOP private(thisMember) reduction(+:this->thisMember)
    FORLOOP
    // expected-error@+2 {{redundant '+' reduction for variable 'Main::thisMember'}}
    // expected-note@+1 {{previous '+' reduction}}
    #pragma acc parallel LOOP reduction(+:this->thisMember) reduction(+:thisMember)
    FORLOOP
    // expected-error@+2 {{conflicting '*' reduction for variable 'Main::thisMember'}}
    // expected-note@+1 {{previous '+' reduction}}
    #pragma acc parallel LOOP reduction(+:this->thisMember) reduction(*:this->thisMember)
    FORLOOP

    //..........................................................................
    // Conflicting DMAs.

    // expected-error@+2 {{copy variable defined again as copy variable}}
    // expected-note@+1 {{previously defined as copy variable here}}
    #pragma acc parallel LOOP copy(thisMember) copy(thisMember)
    FORLOOP
    // expected-error@+2 {{copy variable cannot be copyin variable}}
    // expected-note@+1 {{previously defined as copy variable here}}
    #pragma acc parallel LOOP copy(thisMember) copyin(this->thisMember)
    FORLOOP

    //..........................................................................
    // Conflicting DMA and DSA.

    // par-error@+3 {{copy variable cannot be private variable}}
    // par-note@+2 {{previously defined as copy variable here}}
    // parloop-error-re@+1 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported even on implicit C++ 'this' pointer{{$}}}}
    #pragma acc parallel LOOP copy(this->thisMember) private(thisMember)
    FORLOOP
    // par-error@+3 {{private variable cannot be copy variable}}
    // par-note@+2 {{previously defined as private variable here}}
    // parloop-error-re@+1 {{in 'private' clause on '#pragma acc parallel loop', member expression is not supported{{$}}}}
    #pragma acc parallel LOOP private(this->thisMember) copy(this->thisMember)
    FORLOOP

    //..........................................................................
    // Const variables in non-const member function.

    // this[0:1] is non-const.

    #pragma acc parallel LOOP copyout(this[0:1])
    FORLOOP
    #pragma acc parallel LOOP pcopyout(this[0:1])
    FORLOOP
    #pragma acc parallel LOOP present_or_copyout(this[0:1])
    FORLOOP
    #pragma acc parallel LOOP create(this[0:1])
    FORLOOP
    #pragma acc parallel LOOP pcreate(this[0:1])
    FORLOOP
    #pragma acc parallel LOOP present_or_create(this[0:1])
    FORLOOP

    // Composite is non-const, member is non-const.

    #pragma acc parallel LOOP copyout(local.i)
    FORLOOP
    #pragma acc parallel LOOP pcopyout(local.i)
    FORLOOP
    #pragma acc parallel LOOP present_or_copyout(local.i)
    FORLOOP
    #pragma acc parallel LOOP create(local.i)
    FORLOOP
    #pragma acc parallel LOOP pcreate(local.i)
    FORLOOP
    #pragma acc parallel LOOP present_or_create(local.i)
    FORLOOP

    #pragma acc parallel LOOP copyout(this->thisMember)
    FORLOOP
    #pragma acc parallel LOOP pcopyout(this->thisMember)
    FORLOOP
    #pragma acc parallel LOOP present_or_copyout(this->thisMember)
    FORLOOP
    #pragma acc parallel LOOP create(this->thisMember)
    FORLOOP
    #pragma acc parallel LOOP pcreate(this->thisMember)
    FORLOOP
    #pragma acc parallel LOOP present_or_create(this->thisMember)
    FORLOOP
    #pragma acc parallel private(this->thisMember)
    ;
    #pragma acc parallel LOOP reduction(+:this->thisMember)
    FORLOOP

    #pragma acc parallel LOOP copyout(thisMember)
    FORLOOP
    #pragma acc parallel LOOP pcopyout(thisMember)
    FORLOOP
    #pragma acc parallel LOOP present_or_copyout(thisMember)
    FORLOOP
    #pragma acc parallel LOOP create(thisMember)
    FORLOOP
    #pragma acc parallel LOOP pcreate(thisMember)
    FORLOOP
    #pragma acc parallel LOOP present_or_create(thisMember)
    FORLOOP
    #pragma acc parallel private(thisMember)
    ;
    #pragma acc parallel LOOP reduction(+:thisMember)
    FORLOOP

    // Composite is non-const, member is const.

    // expected-error@+7 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+7 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+7 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-error@+7 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+7 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+7 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#C_constMember 6 {{variable 'C::constMember' declared const here}}
    #pragma acc parallel LOOP copyout(local.constMember)            \
                              pcopyout(local.constMember)           \
                              present_or_copyout(local.constMember) \
                              create(local.constMember)             \
                              pcreate(local.constMember)            \
                              present_or_create(local.constMember)
    FORLOOP
    // expected-error@+8 2 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+8 2 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+8 2 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-error@+8 2 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+8 2 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+8 2 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-error@+8 2 {{const variable cannot be written by 'reduction' clause}}
    // expected-note@#Main_thisConstMember 14 {{variable 'Main::thisConstMember' declared const here}}
    #pragma acc parallel LOOP copyout(this->thisConstMember, thisConstMember)            \
                              pcopyout(this->thisConstMember, thisConstMember)           \
                              present_or_copyout(this->thisConstMember, thisConstMember) \
                              create(this->thisConstMember, thisConstMember)             \
                              pcreate(this->thisConstMember, thisConstMember)            \
                              present_or_create(this->thisConstMember, thisConstMember)  \
                              reduction(+:this->thisConstMember, thisConstMember)
    FORLOOP
    // expected-error@+2 2 {{const variable cannot be initialized after 'private' clause}}
    // expected-note@#Main_thisConstMember 2 {{variable 'Main::thisConstMember' declared const here}}
    #pragma acc parallel private(this->thisConstMember, thisConstMember)
    ;

    // Composite is const, member is non-const.

    // expected-error@+7 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+7 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+7 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-error@+7 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+7 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+7 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#testInNonConstMember_constLocal 6 {{variable 'constLocal' declared const here}}
    #pragma acc parallel LOOP copyout(constLocal.i)            \
                              pcopyout(constLocal.i)           \
                              present_or_copyout(constLocal.i) \
                              create(constLocal.i)             \
                              pcreate(constLocal.i)            \
                              present_or_create(constLocal.i)
    FORLOOP

    // Composite is const, member is const.

    // expected-error@+7 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+7 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+7 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-error@+7 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+7 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+7 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#C_constMember 6 {{variable 'C::constMember' declared const here}}
    #pragma acc parallel LOOP copyout(constLocal.constMember)            \
                              pcopyout(constLocal.constMember)           \
                              present_or_copyout(constLocal.constMember) \
                              create(constLocal.constMember)             \
                              pcreate(constLocal.constMember)            \
                              present_or_create(constLocal.constMember)
    FORLOOP
  }

  //----------------------------------------------------------------------------
  // Data clauses: arg semantics in const member function.
  //----------------------------------------------------------------------------

  void testInConstMember() const { // #Main_testInConstMember
    class C local;
    const class C constLocal; // #Main_testInConstMember_constLocal

    //..........................................................................
    // Const variables in const member function.
    //
    // Include checks that const-ness of enclosing member function doesn't
    // matter for members not on 'this'.

    // this[0:1] is const.

    // expected-error@+7 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+7 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+7 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-error@+7 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+7 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+7 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#Main_testInConstMember 6 {{variable is const within member function 'Main::testInConstMember' declared const here}}
    #pragma acc parallel LOOP copyout(this[0:1])            \
                              pcopyout(this[0:1])           \
                              present_or_copyout(this[0:1]) \
                              create(this[0:1])             \
                              pcreate(this[0:1])            \
                              present_or_create(this[0:1])
    FORLOOP

    // Composite is non-const, member is non-const.

    #pragma acc parallel LOOP copyout(local.i)
    FORLOOP
    #pragma acc parallel LOOP pcopyout(local.i)
    FORLOOP
    #pragma acc parallel LOOP present_or_copyout(local.i)
    FORLOOP
    #pragma acc parallel LOOP create(local.i)
    FORLOOP
    #pragma acc parallel LOOP pcreate(local.i)
    FORLOOP
    #pragma acc parallel LOOP present_or_create(local.i)
    FORLOOP

    // Composite is non-const, member is const.

    // expected-error@+7 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+7 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+7 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-error@+7 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+7 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+7 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#C_constMember 6 {{variable 'C::constMember' declared const here}}
    #pragma acc parallel LOOP copyout(local.constMember)            \
                              pcopyout(local.constMember)           \
                              present_or_copyout(local.constMember) \
                              create(local.constMember)             \
                              pcreate(local.constMember)            \
                              present_or_create(local.constMember)
    FORLOOP

    // Composite is const, member is non-const.

    // expected-error@+7 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+7 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+7 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-error@+7 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+7 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+7 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#Main_testInConstMember_constLocal 6 {{variable 'constLocal' declared const here}}
    #pragma acc parallel LOOP copyout(constLocal.i)            \
                              pcopyout(constLocal.i)           \
                              present_or_copyout(constLocal.i) \
                              create(constLocal.i)             \
                              pcreate(constLocal.i)            \
                              present_or_create(constLocal.i)
    FORLOOP
    // expected-error@+8 2 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+8 2 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+8 2 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-error@+8 2 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+8 2 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+8 2 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-error@+8 2 {{const variable cannot be written by 'reduction' clause}}
    // expected-note@#Main_testInConstMember 14 {{variable is const within member function 'Main::testInConstMember' declared const here}}
    #pragma acc parallel LOOP copyout(this->thisMember, thisMember)            \
                              pcopyout(this->thisMember, thisMember)           \
                              present_or_copyout(this->thisMember, thisMember) \
                              create(this->thisMember, thisMember)             \
                              pcreate(this->thisMember, thisMember)            \
                              present_or_create(this->thisMember, thisMember)  \
                              reduction(+:this->thisMember, thisMember)
    FORLOOP
    // expected-error@+2 2 {{const variable cannot be initialized after 'private' clause}}
    // expected-note@#Main_testInConstMember 2 {{variable is const within member function 'Main::testInConstMember' declared const here}}
    #pragma acc parallel private(this->thisMember, thisMember)
    ;

    // Composite is const, member is const.

    // expected-error@+7 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+7 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+7 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-error@+7 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+7 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+7 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#C_constMember 6 {{variable 'C::constMember' declared const here}}
    #pragma acc parallel LOOP copyout(constLocal.constMember)            \
                              pcopyout(constLocal.constMember)           \
                              present_or_copyout(constLocal.constMember) \
                              create(constLocal.constMember)             \
                              pcreate(constLocal.constMember)            \
                              present_or_create(constLocal.constMember)
    FORLOOP
    // expected-error@+8 2 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+8 2 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+8 2 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-error@+8 2 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+8 2 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+8 2 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-error@+8 2 {{const variable cannot be written by 'reduction' clause}}
    // expected-note@#Main_thisConstMember 14 {{variable 'Main::thisConstMember' declared const here}}
    #pragma acc parallel LOOP copyout(this->thisConstMember, thisConstMember)            \
                              pcopyout(this->thisConstMember, thisConstMember)           \
                              present_or_copyout(this->thisConstMember, thisConstMember) \
                              create(this->thisConstMember, thisConstMember)             \
                              pcreate(this->thisConstMember, thisConstMember)            \
                              present_or_create(this->thisConstMember, thisConstMember)  \
                              reduction(+:this->thisConstMember, thisConstMember)
    FORLOOP
    // expected-error@+2 2 {{const variable cannot be initialized after 'private' clause}}
    // expected-note@#Main_thisConstMember 2 {{variable 'Main::thisConstMember' declared const here}}
    #pragma acc parallel private(this->thisConstMember, thisConstMember)
    ;
  }
};
