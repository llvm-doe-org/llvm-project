// Check "acc update" diagnostic cases not covered by update.c because they are
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
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update self(local.    \
                                  fn  \
                                    )
    // expected-error@+3 {{expected data member}}
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update self(this->              \
                                  thisMemberFn  \
                                              )
    // expected-error-re@+2 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update self(thisMemberFn)

    // expected-error@+3 2 {{expected data member}}
    // expected-error@+3 2 {{expected data member}}
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update host(local.fn, this->thisMemberFn)   \
                       device(local.fn, this->thisMemberFn)

    // expected-error-re@+3 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+3 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update host(thisMemberFn)   \
                       device(thisMemberFn)

    //..........................................................................
    // Member expression not permitted: member function call.

    // expected-error@+3 {{expected data member}}
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update self(local.      \
                                  fn()  \
                                      )
    // expected-error@+3 {{expected data member}}
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update self(this->                \
                                  thisMemberFn()  \
                                                )
    // expected-error-re@+2 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update self(thisMemberFn())

    // expected-error@+3 2 {{expected data member}}
    // expected-error@+3 2 {{expected data member}}
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update host(local.fn(), this->thisMemberFn())   \
                       device(local.fn(), this->thisMemberFn())

    // expected-error-re@+3 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+3 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update host(thisMemberFn())   \
                       device(thisMemberFn())

    //..........................................................................
    // Nested member expressions not permitted: data member.

    // expected-error-re@+4 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+4 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+4 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update self(local.s.i)   \
                       host(local.s.i)   \
                       device(local.s.i)
    // expected-error-re@+4 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+4 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+4 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update self(this->thisStructMember.i)   \
                       host(this->thisStructMember.i)   \
                       device(this->thisStructMember.i)
    // expected-error-re@+4 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+4 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+4 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update self(thisStructMember.i)   \
                       host(thisStructMember.i)   \
                       device(thisStructMember.i)

    //..........................................................................
    // Nested member expressions not permitted: member function call.

    // expected-error-re@+7 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+7 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error-re@+7 {{nested member expression is not supported{{$}}}} // range is for local.s
    // expected-error@+4 {{expected data member}}
    // expected-error@+4 {{expected data member}}
    // expected-error@+4 {{expected data member}}
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update self(local.s.fn())   \
                       host(local.s.fn())   \
                       device(local.s.fn())
    // expected-error-re@+7 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+7 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error-re@+7 {{nested member expression is not supported{{$}}}} // range is for this->thisStructMember
    // expected-error@+4 {{expected data member}}
    // expected-error@+4 {{expected data member}}
    // expected-error@+4 {{expected data member}}
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update self(this->thisStructMember.fn())   \
                       host(this->thisStructMember.fn())   \
                       device(this->thisStructMember.fn())
    // expected-error-re@+7 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+7 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error-re@+7 {{nested member expression is not supported even on implicit C++ 'this' pointer{{$}}}} // range is for thisStructMember
    // expected-error@+4 {{expected data member}}
    // expected-error@+4 {{expected data member}}
    // expected-error@+4 {{expected data member}}
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update self(thisStructMember.fn())   \
                       host(thisStructMember.fn())   \
                       device(thisStructMember.fn())

    //..........................................................................
    // Bare 'this' not permitted because it's not a variable/lvalue.

    // expected-error-re@+3 2 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+3 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update self(this) host(this)  \
                       device(this)
    // expected-error-re@+3 2 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error-re@+3 {{expected variable name or data member expression or subarray{{$}}}}
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update self(this) host(this)  \
                       device(this)

    //..........................................................................
    // Conflicting motion clauses.

    // expected-error@+2 {{variable 'this[0:1]' appears in multiple self, host, or device clauses of the same '#pragma acc update' directive}}
    // expected-note@+1 {{previously appeared here}}
    #pragma acc update self(this[0:1]) device(this[0:1])
    // expected-error@+2 {{variable 'Main::thisMember' appears in multiple self, host, or device clauses of the same '#pragma acc update' directive}}
    // expected-note@+1 {{previously appeared here}}
    #pragma acc update device(thisMember) device(thisMember)
    // expected-error@+2 {{variable 'Main::thisMember' appears in multiple self, host, or device clauses of the same '#pragma acc update' directive}}
    // expected-note@+1 {{previously appeared here}}
    #pragma acc update self(thisMember) device(this->thisMember)

    //..........................................................................
    // Const variables in non-const member function.

    // this[0:1] is non-const.

    #pragma acc update self(this[0:1])
    #pragma acc update host(this[0:1])
    #pragma acc update device(this[0:1])

    // Composite is non-const, member is non-const.

    #pragma acc update self(local.i)
    #pragma acc update host(local.i)
    #pragma acc update device(local.i)

    #pragma acc update self(this->thisMember)
    #pragma acc update host(this->thisMember)
    #pragma acc update device(this->thisMember)

    #pragma acc update self(thisMember)
    #pragma acc update host(thisMember)
    #pragma acc update device(thisMember)

    // Composite is non-const, member is const.

    // expected-error@+5 {{const variable cannot be written by 'self' clause}}
    // expected-error@+5 {{const variable cannot be written by 'host' clause}}
    // expected-error@+5 {{const variable cannot be written by 'device' clause}}
    // expected-note@#C_constMember 3 {{variable 'C::constMember' declared const here}}
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update self(local.constMember)   \
                       host(local.constMember)   \
                       device(local.constMember)
    // expected-error@+5 2 {{const variable cannot be written by 'self' clause}}
    // expected-error@+5 2 {{const variable cannot be written by 'host' clause}}
    // expected-error@+5 2 {{const variable cannot be written by 'device' clause}}
    // expected-note@#Main_thisConstMember 6 {{variable 'Main::thisConstMember' declared const here}}
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update self(this->thisConstMember, thisConstMember)   \
                       host(this->thisConstMember, thisConstMember)   \
                       device(this->thisConstMember, thisConstMember)

    // Composite is const, member is non-const.

    // expected-error@+5 {{const variable cannot be written by 'self' clause}}
    // expected-error@+5 {{const variable cannot be written by 'host' clause}}
    // expected-error@+5 {{const variable cannot be written by 'device' clause}}
    // expected-note@#testInNonConstMember_constLocal 3 {{variable 'constLocal' declared const here}}
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update self(constLocal.i)   \
                       host(constLocal.i)   \
                       device(constLocal.i)

    // Composite is const, member is const.

    // expected-error@+5 {{const variable cannot be written by 'self' clause}}
    // expected-error@+5 {{const variable cannot be written by 'host' clause}}
    // expected-error@+5 {{const variable cannot be written by 'device' clause}}
    // expected-note@#C_constMember 3 {{variable 'C::constMember' declared const here}}
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update self(constLocal.constMember)   \
                       host(constLocal.constMember)   \
                       device(constLocal.constMember)
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

    // expected-error@+5 {{const variable cannot be written by 'self' clause}}
    // expected-error@+5 {{const variable cannot be written by 'host' clause}}
    // expected-error@+5 {{const variable cannot be written by 'device' clause}}
    // expected-note@#Main_testInConstMember 3 {{variable is const within member function 'Main::testInConstMember' declared const here}}
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update self(this[0:1])   \
                       host(this[0:1])   \
                       device(this[0:1])

    // Composite is non-const, member is non-const.

    #pragma acc update self(local.i)
    #pragma acc update host(local.i)
    #pragma acc update device(local.i)

    // Composite is non-const, member is const.

    // expected-error@+5 {{const variable cannot be written by 'self' clause}}
    // expected-error@+5 {{const variable cannot be written by 'host' clause}}
    // expected-error@+5 {{const variable cannot be written by 'device' clause}}
    // expected-note@#C_constMember 3 {{variable 'C::constMember' declared const here}}
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update self(local.constMember)   \
                       host(local.constMember)   \
                       device(local.constMember)

    // Composite is const, member is non-const.

    // expected-error@+5 {{const variable cannot be written by 'self' clause}}
    // expected-error@+5 {{const variable cannot be written by 'host' clause}}
    // expected-error@+5 {{const variable cannot be written by 'device' clause}}
    // expected-note@#Main_testInConstMember_constLocal 3 {{variable 'constLocal' declared const here}}
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update self(constLocal.i)   \
                       host(constLocal.i)   \
                       device(constLocal.i)
    // expected-error@+5 2 {{const variable cannot be written by 'self' clause}}
    // expected-error@+5 2 {{const variable cannot be written by 'host' clause}}
    // expected-error@+5 2 {{const variable cannot be written by 'device' clause}}
    // expected-note@#Main_testInConstMember 6 {{variable is const within member function 'Main::testInConstMember' declared const here}}
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update self(this->thisMember, thisMember)   \
                       host(this->thisMember, thisMember)   \
                       device(this->thisMember, thisMember)

    // Composite is const, member is const.

    // expected-error@+5 {{const variable cannot be written by 'self' clause}}
    // expected-error@+5 {{const variable cannot be written by 'host' clause}}
    // expected-error@+5 {{const variable cannot be written by 'device' clause}}
    // expected-note@#C_constMember 3 {{variable 'C::constMember' declared const here}}
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update self(constLocal.constMember)   \
                       host(constLocal.constMember)   \
                       device(constLocal.constMember)
    // expected-error@+5 2 {{const variable cannot be written by 'self' clause}}
    // expected-error@+5 2 {{const variable cannot be written by 'host' clause}}
    // expected-error@+5 2 {{const variable cannot be written by 'device' clause}}
    // expected-note@#Main_thisConstMember 6 {{variable 'Main::thisConstMember' declared const here}}
    // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
    #pragma acc update self(this->thisConstMember, thisConstMember)   \
                       host(this->thisConstMember, thisConstMember)   \
                       device(this->thisConstMember, thisConstMember)
  }
};
