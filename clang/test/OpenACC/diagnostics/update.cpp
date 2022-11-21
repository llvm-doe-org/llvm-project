// Check "acc update" diagnostic cases not covered by update.c because they are
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
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update self(obj.    \
                              fn  \
                                )
  // expected-error@+3 {{expected data member}}
  // expected-error@+3 {{expected data member}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update host(obj.fn)   \
                     device(obj.fn)

  // Member expression not permitted: member function call.

  // expected-error@+4 {{expected data member}}
  // expected-error@+4 {{expected data member}}
  // expected-error@+4 {{expected data member}}
  // expected-error@+1 {{expected at least one 'self', 'host', or 'device' clause for '#pragma acc update'}}
  #pragma acc update self(obj.fn())   \
                     host(obj.fn())   \
                     device(obj.fn())

  return 0;
}
