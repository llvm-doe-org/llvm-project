// Check "acc data" diagnostic cases not covered by data.c because they are
// specific to C++.

// OpenACC disabled
// RUN: %clang_cc1 -verify=noacc,expected-noacc %s

// OpenACC enabled
// RUN: %clang_cc1 -verify=expected,expected-noacc %s \
// RUN:            -fopenacc -Wno-openacc-and-cxx

// END.

// noacc-no-diagnostics

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
    // Member expression not permitted: member function.

    // expected-error@+3 {{expected data member}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data present(local.    \
                                   fn  \
                                     )
    ;
    // expected-error@+3 {{expected data member}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data present(this->              \
                                   thisMemberFn  \
                                               )
    ;
    // expected-error-re@+2 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data present(thisMemberFn)
    ;

    // expected-error@+14 2 {{expected data member}}
    // expected-error@+14 2 {{expected data member}}
    // expected-error@+14 2 {{expected data member}}
    // expected-error@+14 2 {{expected data member}}
    // expected-error@+14 2 {{expected data member}}
    // expected-error@+14 2 {{expected data member}}
    // expected-error@+14 2 {{expected data member}}
    // expected-error@+14 2 {{expected data member}}
    // expected-error@+14 2 {{expected data member}}
    // expected-error@+14 2 {{expected data member}}
    // expected-error@+14 2 {{expected data member}}
    // expected-error@+14 2 {{expected data member}}
    // expected-error@+14 2 {{expected data member}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data copy(local.fn, this->thisMemberFn)               \
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
    ;
    // expected-error-re@+14 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+14 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+14 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+14 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+14 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+14 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+14 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+14 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+14 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+14 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+14 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+14 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+14 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data copy(thisMemberFn)               \
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
    ;

    //..........................................................................
    // Member expression not permitted: member function call.

    // expected-error@+3 {{expected data member}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data present(local.      \
                                   fn    \
                                     ())
    ;
    // expected-error@+3 {{expected data member}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data present(this->                \
                                   thisMemberFn    \
                                               ())
    ;
    // expected-error-re@+3 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data present(                \
                             thisMemberFn    \
                                         ())
    ;

    // expected-error@+14 2 {{expected data member}}
    // expected-error@+14 2 {{expected data member}}
    // expected-error@+14 2 {{expected data member}}
    // expected-error@+14 2 {{expected data member}}
    // expected-error@+14 2 {{expected data member}}
    // expected-error@+14 2 {{expected data member}}
    // expected-error@+14 2 {{expected data member}}
    // expected-error@+14 2 {{expected data member}}
    // expected-error@+14 2 {{expected data member}}
    // expected-error@+14 2 {{expected data member}}
    // expected-error@+14 2 {{expected data member}}
    // expected-error@+14 2 {{expected data member}}
    // expected-error@+14 2 {{expected data member}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data copy(local.fn(), this->thisMemberFn())               \
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
    ;
    // expected-error-re@+14 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+14 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+14 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+14 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+14 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+14 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+14 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+14 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+14 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+14 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+14 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+14 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+14 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data copy(thisMemberFn())               \
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
    ;

    //..........................................................................
    // Nested member expressions not permitted: data member.

    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data present(local.s.i)            \
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
    ;
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+15 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data present(this->thisStructMember.i)            \
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
    ;
    // expected-error-re@+15 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+15 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+15 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+15 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+15 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+15 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+15 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+15 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+15 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+15 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+15 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+15 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+15 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+15 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data present(thisStructMember.i)            \
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
    ;

    //..........................................................................
    // Nested member expressions not permitted: member function call.

    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data present(local.s.fn())            \
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
    ;
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+29 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data present(this->thisStructMember.fn())            \
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
    ;
    // expected-error-re@+29 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+29 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+29 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+29 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+29 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+29 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+29 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+29 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+29 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+29 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+29 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+29 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+29 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+29 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+15 {{expected data member}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data present(thisStructMember.fn())            \
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
    ;

    //..........................................................................
    // Bare 'this' not permitted because it's not a variable/lvalue.

    // expected-error-re@+7 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+7 3 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+7 3 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+7 3 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+7 3 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+7 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data present(this)                                         \
                     copy(this) pcopy(this) present_or_copy(this)          \
                     copyin(this) pcopyin(this) present_or_copyin(this)    \
                     copyout(this) pcopyout(this) present_or_copyout(this) \
                     create(this) pcreate(this) present_or_create(this)    \
                     no_create(this)
    ;

    //..........................................................................
    // Conflicting DMAs.

    // expected-error@+2 {{copy variable defined again as copy variable}}
    // expected-note@+1 {{previously defined as copy variable here}}
    #pragma acc data copy(this->thisMember) copy(thisMember)
    ;
    // expected-error@+2 {{copy variable cannot be copyin variable}}
    // expected-note@+1 {{previously defined as copy variable here}}
    #pragma acc data copy(this->thisMember) copyin(this->thisMember)
    ;

    //..........................................................................
    // Const variables in non-const member function.

    // this[0:1] is non-const.

    #pragma acc data copyout(this[0:1])
    ;
    #pragma acc data pcopyout(this[0:1])
    ;
    #pragma acc data present_or_copyout(this[0:1])
    ;
    #pragma acc data create(this[0:1])
    ;
    #pragma acc data pcreate(this[0:1])
    ;
    #pragma acc data present_or_create(this[0:1])
    ;

    // Composite is non-const, member is non-const.

    #pragma acc data copyout(local.i)
    ;
    #pragma acc data pcopyout(local.i)
    ;
    #pragma acc data present_or_copyout(local.i)
    ;
    #pragma acc data create(local.i)
    ;
    #pragma acc data pcreate(local.i)
    ;
    #pragma acc data present_or_create(local.i)
    ;

    #pragma acc data copyout(this->thisMember)
    ;
    #pragma acc data pcopyout(this->thisMember)
    ;
    #pragma acc data present_or_copyout(this->thisMember)
    ;
    #pragma acc data create(this->thisMember)
    ;
    #pragma acc data pcreate(this->thisMember)
    ;
    #pragma acc data present_or_create(this->thisMember)
    ;

    #pragma acc data copyout(thisMember)
    ;
    #pragma acc data pcopyout(thisMember)
    ;
    #pragma acc data present_or_copyout(thisMember)
    ;
    #pragma acc data create(thisMember)
    ;
    #pragma acc data pcreate(thisMember)
    ;
    #pragma acc data present_or_create(thisMember)
    ;

    // Composite is non-const, member is const.

    // expected-error@+8 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+8 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+8 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-error@+8 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+8 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+8 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#C_constMember 6 {{variable 'C::constMember' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data copyout(local.constMember)            \
                     pcopyout(local.constMember)           \
                     present_or_copyout(local.constMember) \
                     create(local.constMember)             \
                     pcreate(local.constMember)            \
                     present_or_create(local.constMember)
    ;
    // expected-error@+8 2 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+8 2 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+8 2 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-error@+8 2 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+8 2 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+8 2 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#Main_thisConstMember 12 {{variable 'Main::thisConstMember' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data copyout(this->thisConstMember, thisConstMember)            \
                     pcopyout(this->thisConstMember, thisConstMember)           \
                     present_or_copyout(this->thisConstMember, thisConstMember) \
                     create(this->thisConstMember, thisConstMember)             \
                     pcreate(this->thisConstMember, thisConstMember)            \
                     present_or_create(this->thisConstMember, thisConstMember)
    ;

    // Composite is const, member is non-const.

    // expected-error@+8 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+8 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+8 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-error@+8 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+8 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+8 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#testInNonConstMember_constLocal 6 {{variable 'constLocal' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data copyout(constLocal.i)            \
                     pcopyout(constLocal.i)           \
                     present_or_copyout(constLocal.i) \
                     create(constLocal.i)             \
                     pcreate(constLocal.i)            \
                     present_or_create(constLocal.i)
    ;

    // Composite is const, member is const.

    // expected-error@+8 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+8 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+8 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-error@+8 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+8 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+8 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#C_constMember 6 {{variable 'C::constMember' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data copyout(constLocal.constMember)            \
                     pcopyout(constLocal.constMember)           \
                     present_or_copyout(constLocal.constMember) \
                     create(constLocal.constMember)             \
                     pcreate(constLocal.constMember)            \
                     present_or_create(constLocal.constMember)
    ;
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

    // expected-error@+8 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+8 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+8 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-error@+8 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+8 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+8 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#Main_testInConstMember 6 {{variable is const within member function 'Main::testInConstMember' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data copyout(this[0:1])            \
                     pcopyout(this[0:1])           \
                     present_or_copyout(this[0:1]) \
                     create(this[0:1])             \
                     pcreate(this[0:1])            \
                     present_or_create(this[0:1])
    ;

    // Composite is non-const, member is non-const.

    #pragma acc data copyout(local.i)
    ;
    #pragma acc data pcopyout(local.i)
    ;
    #pragma acc data present_or_copyout(local.i)
    ;
    #pragma acc data create(local.i)
    ;
    #pragma acc data pcreate(local.i)
    ;
    #pragma acc data present_or_create(local.i)
    ;

    // Composite is non-const, member is const.

    // expected-error@+8 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+8 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+8 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-error@+8 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+8 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+8 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#C_constMember 6 {{variable 'C::constMember' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data copyout(local.constMember)            \
                     pcopyout(local.constMember)           \
                     present_or_copyout(local.constMember) \
                     create(local.constMember)             \
                     pcreate(local.constMember)            \
                     present_or_create(local.constMember)
    ;

    // Composite is const, member is non-const.

    // expected-error@+8 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+8 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+8 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-error@+8 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+8 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+8 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#Main_testInConstMember_constLocal 6 {{variable 'constLocal' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data copyout(constLocal.i)            \
                     pcopyout(constLocal.i)           \
                     present_or_copyout(constLocal.i) \
                     create(constLocal.i)             \
                     pcreate(constLocal.i)            \
                     present_or_create(constLocal.i)
    ;
    // expected-error@+8 2 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+8 2 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+8 2 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-error@+8 2 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+8 2 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+8 2 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#Main_testInConstMember 12 {{variable is const within member function 'Main::testInConstMember' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data copyout(this->thisMember, thisMember)            \
                     pcopyout(this->thisMember, thisMember)           \
                     present_or_copyout(this->thisMember, thisMember) \
                     create(this->thisMember, thisMember)             \
                     pcreate(this->thisMember, thisMember)            \
                     present_or_create(this->thisMember, thisMember)
    ;

    // Composite is const, member is const.

    // expected-error@+8 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+8 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+8 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-error@+8 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+8 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+8 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#C_constMember 6 {{variable 'C::constMember' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data copyout(constLocal.constMember)            \
                     pcopyout(constLocal.constMember)           \
                     present_or_copyout(constLocal.constMember) \
                     create(constLocal.constMember)             \
                     pcreate(constLocal.constMember)            \
                     present_or_create(constLocal.constMember)
    ;
    // expected-error@+8 2 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+8 2 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+8 2 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-error@+8 2 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+8 2 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+8 2 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#Main_thisConstMember 12 {{variable 'Main::thisConstMember' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
    #pragma acc data copyout(this->thisConstMember, thisConstMember)            \
                     pcopyout(this->thisConstMember, thisConstMember)           \
                     present_or_copyout(this->thisConstMember, thisConstMember) \
                     create(this->thisConstMember, thisConstMember)             \
                     pcreate(this->thisConstMember, thisConstMember)            \
                     present_or_create(this->thisConstMember, thisConstMember)
    ;
  }
};
