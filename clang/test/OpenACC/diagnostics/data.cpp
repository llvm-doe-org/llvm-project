// Check "acc data" diagnostic cases not covered by data.c because they are
// specific to C++.

// OpenACC disabled
// RUN: %clang_cc1 -verify=noacc,expected-noacc %s

// OpenACC enabled
// RUN: %clang_cc1 -verify=expected,expected-noacc %s \
// RUN:            -fopenacc -Wno-openacc-and-cxx

// END.

// noacc-no-diagnostics

class C {
public:
  int i;
  int fn();
} obj;

int main() {
  //----------------------------------------------------------------------------
  // Data clauses: arg semantics
  //----------------------------------------------------------------------------

  // Member expression not permitted: member function.

  // expected-error@+3 {{expected data member}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
  #pragma acc data present(obj.    \
                               fn  \
                                 )
  ;
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
  // expected-error@+1 {{expected at least one data clause for '#pragma acc data'}}
  #pragma acc data copy(obj.fn)               \
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
  ;

  // Member expression not permitted: member function call.

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
  #pragma acc data present(obj.fn())            \
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
  ;

  return 0;
}
