// Check "acc enter data" and "acc exit data" diagnostic cases not covered by
// enter-exit-data.c because they are specific to C++.

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
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data copyin(local.    \
                                        fn  \
                                          )
    // expected-error@+3 {{expected data member}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data copyout(local.    \
                                        fn  \
                                          )
    // expected-error@+3 {{expected data member}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data copyin(this->              \
                                        thisMemberFn  \
                                                    )
    // expected-error@+3 {{expected data member}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data copyout(this->              \
                                        thisMemberFn  \
                                                    )
    // expected-error-re@+2 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data copyin(thisMemberFn)
    // expected-error-re@+2 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data copyout(thisMemberFn)

    // expected-error@+6 2 {{expected data member}}
    // expected-error@+6 2 {{expected data member}}
    // expected-error@+6 2 {{expected data member}}
    // expected-error@+6 2 {{expected data member}}
    // expected-error@+6 2 {{expected data member}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data pcopyin(local.fn, this->thisMemberFn)            \
                           present_or_copyin(local.fn, this->thisMemberFn)  \
                           create(local.fn, this->thisMemberFn)             \
                           pcreate(local.fn, this->thisMemberFn)            \
                           present_or_create(local.fn, this->thisMemberFn)
    // expected-error@+4 2 {{expected data member}}
    // expected-error@+4 2 {{expected data member}}
    // expected-error@+4 2 {{expected data member}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data pcopyout(local.fn, this->thisMemberFn)            \
                          present_or_copyout(local.fn, this->thisMemberFn)  \
                          delete(local.fn, this->thisMemberFn)
    // expected-error-re@+6 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+6 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+6 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+6 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+6 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data pcopyin(thisMemberFn)            \
                           present_or_copyin(thisMemberFn)  \
                           create(thisMemberFn)             \
                           pcreate(thisMemberFn)            \
                           present_or_create(thisMemberFn)
    // expected-error-re@+4 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+4 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+4 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data pcopyout(thisMemberFn)            \
                          present_or_copyout(thisMemberFn)  \
                          delete(thisMemberFn)

    //..........................................................................
    // Member expression not permitted: member function call.

    // expected-error@+3 {{expected data member}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data copyin(local.      \
                                        fn()  \
                                            )
    // expected-error@+3 {{expected data member}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data copyout(local.      \
                                        fn()  \
                                            )
    // expected-error@+3 {{expected data member}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data copyin(this->                \
                                        thisMemberFn()  \
                                                      )
    // expected-error@+3 {{expected data member}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data copyout(this->                \
                                        thisMemberFn()  \
                                                      )
    // expected-error-re@+2 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data copyin(thisMemberFn())
    // expected-error-re@+2 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data copyout(thisMemberFn())

    // expected-error@+6 2 {{expected data member}}
    // expected-error@+6 2 {{expected data member}}
    // expected-error@+6 2 {{expected data member}}
    // expected-error@+6 2 {{expected data member}}
    // expected-error@+6 2 {{expected data member}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data pcopyin(local.fn(), this->thisMemberFn())            \
                           present_or_copyin(local.fn(), this->thisMemberFn())  \
                           create(local.fn(), this->thisMemberFn())             \
                           pcreate(local.fn(), this->thisMemberFn())            \
                           present_or_create(local.fn(), this->thisMemberFn())
    // expected-error@+4 2 {{expected data member}}
    // expected-error@+4 2 {{expected data member}}
    // expected-error@+4 2 {{expected data member}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data pcopyout(local.fn(), this->thisMemberFn())            \
                          present_or_copyout(local.fn(), this->thisMemberFn())  \
                          delete(local.fn(), this->thisMemberFn())
    // expected-error-re@+6 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+6 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+6 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+6 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+6 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data pcopyin(thisMemberFn())            \
                           present_or_copyin(thisMemberFn())  \
                           create(thisMemberFn())             \
                           pcreate(thisMemberFn())            \
                           present_or_create(thisMemberFn())
    // expected-error-re@+4 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+4 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+4 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data pcopyout(thisMemberFn())            \
                          present_or_copyout(thisMemberFn())  \
                          delete(thisMemberFn())

    //..........................................................................
    // Nested member expressions not permitted: data member.

    // expected-error-re@+7 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+7 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+7 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+7 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+7 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+7 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data copyin(local.s.i)             \
                           pcopyin(local.s.i)            \
                           present_or_copyin(local.s.i)  \
                           create(local.s.i)             \
                           pcreate(local.s.i)            \
                           present_or_create(local.s.i)
    // expected-error-re@+5 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+5 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+5 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+5 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data copyout(local.s.i)             \
                          pcopyout(local.s.i)            \
                          present_or_copyout(local.s.i)  \
                          delete(local.s.i)

    // expected-error-re@+7 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+7 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+7 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+7 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+7 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+7 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data copyin(this->thisStructMember.i)             \
                           pcopyin(this->thisStructMember.i)            \
                           present_or_copyin(this->thisStructMember.i)  \
                           create(this->thisStructMember.i)             \
                           pcreate(this->thisStructMember.i)            \
                           present_or_create(this->thisStructMember.i)
    // expected-error-re@+5 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+5 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+5 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+5 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data copyout(this->thisStructMember.i)             \
                          pcopyout(this->thisStructMember.i)            \
                          present_or_copyout(this->thisStructMember.i)  \
                          delete(this->thisStructMember.i)

    // expected-error-re@+7 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+7 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+7 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+7 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+7 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+7 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data copyin(thisStructMember.i)             \
                           pcopyin(thisStructMember.i)            \
                           present_or_copyin(thisStructMember.i)  \
                           create(thisStructMember.i)             \
                           pcreate(thisStructMember.i)            \
                           present_or_create(thisStructMember.i)
    // expected-error-re@+5 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+5 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+5 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+5 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data copyout(thisStructMember.i)             \
                          pcopyout(thisStructMember.i)            \
                          present_or_copyout(thisStructMember.i)  \
                          delete(thisStructMember.i)

    //..........................................................................
    // Nested member expressions not permitted: member function call.

    // expected-error-re@+13 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+13 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+13 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+13 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+13 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+13 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error@+7 {{expected data member}}
    // expected-error@+7 {{expected data member}}
    // expected-error@+7 {{expected data member}}
    // expected-error@+7 {{expected data member}}
    // expected-error@+7 {{expected data member}}
    // expected-error@+7 {{expected data member}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data copyin(local.s.fn())             \
                           pcopyin(local.s.fn())            \
                           present_or_copyin(local.s.fn())  \
                           create(local.s.fn())             \
                           pcreate(local.s.fn())            \
                           present_or_create(local.s.fn())
    // expected-error-re@+9 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+9 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+9 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+9 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error@+5 {{expected data member}}
    // expected-error@+5 {{expected data member}}
    // expected-error@+5 {{expected data member}}
    // expected-error@+5 {{expected data member}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data copyout(local.s.fn())             \
                          pcopyout(local.s.fn())            \
                          present_or_copyout(local.s.fn())  \
                          delete(local.s.fn())

    // expected-error-re@+13 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+13 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+13 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+13 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+13 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+13 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error@+7 {{expected data member}}
    // expected-error@+7 {{expected data member}}
    // expected-error@+7 {{expected data member}}
    // expected-error@+7 {{expected data member}}
    // expected-error@+7 {{expected data member}}
    // expected-error@+7 {{expected data member}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data copyin(this->thisStructMember.fn())             \
                           pcopyin(this->thisStructMember.fn())            \
                           present_or_copyin(this->thisStructMember.fn())  \
                           create(this->thisStructMember.fn())             \
                           pcreate(this->thisStructMember.fn())            \
                           present_or_create(this->thisStructMember.fn())
    // expected-error-re@+9 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+9 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+9 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+9 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error@+5 {{expected data member}}
    // expected-error@+5 {{expected data member}}
    // expected-error@+5 {{expected data member}}
    // expected-error@+5 {{expected data member}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data copyout(this->thisStructMember.fn())             \
                          pcopyout(this->thisStructMember.fn())            \
                          present_or_copyout(this->thisStructMember.fn())  \
                          delete(this->thisStructMember.fn())

    // expected-error-re@+13 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+13 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+13 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+13 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+13 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+13 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error@+7 {{expected data member}}
    // expected-error@+7 {{expected data member}}
    // expected-error@+7 {{expected data member}}
    // expected-error@+7 {{expected data member}}
    // expected-error@+7 {{expected data member}}
    // expected-error@+7 {{expected data member}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data copyin(thisStructMember.fn())             \
                           pcopyin(thisStructMember.fn())            \
                           present_or_copyin(thisStructMember.fn())  \
                           create(thisStructMember.fn())             \
                           pcreate(thisStructMember.fn())            \
                           present_or_create(thisStructMember.fn())
    // expected-error-re@+9 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+9 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+9 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+9 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error@+5 {{expected data member}}
    // expected-error@+5 {{expected data member}}
    // expected-error@+5 {{expected data member}}
    // expected-error@+5 {{expected data member}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data copyout(thisStructMember.fn())             \
                          pcopyout(thisStructMember.fn())            \
                          present_or_copyout(thisStructMember.fn())  \
                          delete(thisStructMember.fn())

    //..........................................................................
    // Bare 'this' not permitted because it's not a variable/lvalue.

    // expected-error-re@+3 3 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+3 3 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data copyin(this) pcopyin(this) present_or_copyin(this)  \
                           create(this) pcreate(this) present_or_create(this)
    // expected-error-re@+3 3 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+3 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data copyout(this) pcopyout(this) present_or_copyout(this)  \
                          delete(this)

    //..........................................................................
    // Conflicting DMAs.

    // expected-error@+2 {{copyin variable defined again as copyin variable}}
    // expected-note@+1 {{previously defined as copyin variable here}}
    #pragma acc enter data copyin(thisMember) copyin(thisMember)
    // expected-error@+2 {{create variable cannot be copyin variable}}
    // expected-note@+1 {{previously defined as create variable here}}
    #pragma acc enter data create(this->thisMember) copyin(thisMember)
    // expected-error@+2 {{delete variable defined again as delete variable}}
    // expected-note@+1 {{previously defined as delete variable here}}
    #pragma acc exit data delete(thisMember) delete(this->thisMember)
    // expected-error@+2 {{copyout variable cannot be delete variable}}
    // expected-note@+1 {{previously defined as copyout variable here}}
    #pragma acc exit data copyout(this->thisMember) delete(this->thisMember)

    //..........................................................................
    // Const variables in non-const member function.

    // this[0:1] is non-const.

    #pragma acc enter data create(this[0:1])
    #pragma acc enter data pcreate(this[0:1])
    #pragma acc enter data present_or_create(this[0:1])
    #pragma acc exit data copyout(this[0:1])
    #pragma acc exit data pcopyout(this[0:1])
    #pragma acc exit data present_or_copyout(this[0:1])

    // Composite is non-const, member is non-const.

    #pragma acc enter data create(local.i)
    #pragma acc enter data pcreate(local.i)
    #pragma acc enter data present_or_create(local.i)
    #pragma acc exit data copyout(local.i)
    #pragma acc exit data pcopyout(local.i)
    #pragma acc exit data present_or_copyout(local.i)

    #pragma acc enter data create(this->thisMember)
    #pragma acc enter data pcreate(this->thisMember)
    #pragma acc enter data present_or_create(this->thisMember)
    #pragma acc exit data copyout(this->thisMember)
    #pragma acc exit data pcopyout(this->thisMember)
    #pragma acc exit data present_or_copyout(this->thisMember)

    #pragma acc enter data create(thisMember)
    #pragma acc enter data pcreate(thisMember)
    #pragma acc enter data present_or_create(thisMember)
    #pragma acc exit data copyout(thisMember)
    #pragma acc exit data pcopyout(thisMember)
    #pragma acc exit data present_or_copyout(thisMember)

    // Composite is non-const, member is const.

    // expected-error@+5 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+5 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+5 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#C_constMember 3 {{variable 'C::constMember' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data create(local.constMember)            \
                           pcreate(local.constMember)           \
                           present_or_create(local.constMember)
    // expected-error@+5 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+5 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+5 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-note@#C_constMember 3 {{variable 'C::constMember' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data copyout(local.constMember)            \
                          pcopyout(local.constMember)           \
                          present_or_copyout(local.constMember)

    // expected-error@+5 2 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+5 2 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+5 2 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#Main_thisConstMember 6 {{variable 'Main::thisConstMember' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data create(this->thisConstMember, thisConstMember)            \
                           pcreate(this->thisConstMember, thisConstMember)           \
                           present_or_create(this->thisConstMember, thisConstMember)
    // expected-error@+5 2 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+5 2 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+5 2 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-note@#Main_thisConstMember 6 {{variable 'Main::thisConstMember' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data copyout(this->thisConstMember, thisConstMember)            \
                          pcopyout(this->thisConstMember, thisConstMember)           \
                          present_or_copyout(this->thisConstMember, thisConstMember)

    // Composite is const, member is non-const.

    // expected-error@+5 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+5 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+5 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#testInNonConstMember_constLocal 3 {{variable 'constLocal' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data create(constLocal.i)             \
                           pcreate(constLocal.i)            \
                           present_or_create(constLocal.i)
    // expected-error@+5 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+5 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+5 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-note@#testInNonConstMember_constLocal 3 {{variable 'constLocal' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data copyout(constLocal.i)            \
                          pcopyout(constLocal.i)           \
                          present_or_copyout(constLocal.i)

    // Composite is const, member is const.

    // expected-error@+5 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+5 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+5 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#C_constMember 3 {{variable 'C::constMember' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data create(constLocal.constMember)            \
                           pcreate(constLocal.constMember)           \
                           present_or_create(constLocal.constMember)
    // expected-error@+5 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+5 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+5 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-note@#C_constMember 3 {{variable 'C::constMember' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data copyout(constLocal.constMember)            \
                          pcopyout(constLocal.constMember)           \
                          present_or_copyout(constLocal.constMember)
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

    // expected-error@+5 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+5 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+5 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#Main_testInConstMember 3 {{variable is const within member function 'Main::testInConstMember' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data create(this[0:1])            \
                           pcreate(this[0:1])           \
                           present_or_create(this[0:1])
    // expected-error@+5 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+5 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+5 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-note@#Main_testInConstMember 3 {{variable is const within member function 'Main::testInConstMember' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data copyout(this[0:1])            \
                          pcopyout(this[0:1])           \
                          present_or_copyout(this[0:1])

    // Composite is non-const, member is non-const.

    #pragma acc enter data create(local.i)
    #pragma acc enter data pcreate(local.i)
    #pragma acc enter data present_or_create(local.i)
    #pragma acc exit data copyout(local.i)
    #pragma acc exit data pcopyout(local.i)
    #pragma acc exit data present_or_copyout(local.i)

    // Composite is non-const, member is const.

    // expected-error@+5 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+5 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+5 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#C_constMember 3 {{variable 'C::constMember' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data create(local.constMember)            \
                           pcreate(local.constMember)           \
                           present_or_create(local.constMember)
    // expected-error@+5 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+5 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+5 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-note@#C_constMember 3 {{variable 'C::constMember' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data copyout(local.constMember)            \
                          pcopyout(local.constMember)           \
                          present_or_copyout(local.constMember)

    // Composite is const, member is non-const.

    // expected-error@+5 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+5 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+5 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#Main_testInConstMember_constLocal 3 {{variable 'constLocal' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data create(constLocal.i)            \
                           pcreate(constLocal.i)           \
                           present_or_create(constLocal.i)
    // expected-error@+5 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+5 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+5 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-note@#Main_testInConstMember_constLocal 3 {{variable 'constLocal' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data copyout(constLocal.i)            \
                          pcopyout(constLocal.i)           \
                          present_or_copyout(constLocal.i)

    // expected-error@+5 2 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+5 2 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+5 2 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#Main_testInConstMember 6 {{variable is const within member function 'Main::testInConstMember' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data create(this->thisMember, thisMember)            \
                           pcreate(this->thisMember, thisMember)           \
                           present_or_create(this->thisMember, thisMember)
    // expected-error@+5 2 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+5 2 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+5 2 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-note@#Main_testInConstMember 6 {{variable is const within member function 'Main::testInConstMember' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data copyout(this->thisMember, thisMember)            \
                          pcopyout(this->thisMember, thisMember)           \
                          present_or_copyout(this->thisMember, thisMember)

    // Composite is const, member is const.

    // expected-error@+5 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+5 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+5 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#C_constMember 3 {{variable 'C::constMember' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data create(constLocal.constMember)            \
                           pcreate(constLocal.constMember)           \
                           present_or_create(constLocal.constMember)
    // expected-error@+5 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+5 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+5 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-note@#C_constMember 3 {{variable 'C::constMember' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data copyout(constLocal.constMember)            \
                          pcopyout(constLocal.constMember)           \
                          present_or_copyout(constLocal.constMember)

    // expected-error@+5 2 {{const variable cannot be initialized after 'create' clause}}
    // expected-error@+5 2 {{const variable cannot be initialized after 'pcreate' clause}}
    // expected-error@+5 2 {{const variable cannot be initialized after 'present_or_create' clause}}
    // expected-note@#Main_thisConstMember 6 {{variable 'Main::thisConstMember' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
    #pragma acc enter data create(this->thisConstMember, thisConstMember)            \
                           pcreate(this->thisConstMember, thisConstMember)           \
                           present_or_create(this->thisConstMember, thisConstMember)
    // expected-error@+5 2 {{const variable cannot be written by 'copyout' clause}}
    // expected-error@+5 2 {{const variable cannot be written by 'pcopyout' clause}}
    // expected-error@+5 2 {{const variable cannot be written by 'present_or_copyout' clause}}
    // expected-note@#Main_thisConstMember 6 {{variable 'Main::thisConstMember' declared const here}}
    // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
    #pragma acc exit data copyout(this->thisConstMember, thisConstMember)            \
                          pcopyout(this->thisConstMember, thisConstMember)           \
                          present_or_copyout(this->thisConstMember, thisConstMember)
  }
};
