// Check "acc enter data" and "acc exit data" diagnostic cases not covered by
// enter-exit-data.c because they are specific to C++.

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
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data copyin(obj.    \
                                    fn  \
                                      )
  // expected-error@+3 {{expected data member}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data copyout(obj.    \
                                    fn  \
                                      )
  // expected-error@+6 {{expected data member}}
  // expected-error@+6 {{expected data member}}
  // expected-error@+6 {{expected data member}}
  // expected-error@+6 {{expected data member}}
  // expected-error@+6 {{expected data member}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data pcopyin(obj.fn)            \
                         present_or_copyin(obj.fn)  \
                         create(obj.fn)             \
                         pcreate(obj.fn)            \
                         present_or_create(obj.fn)
  // expected-error@+4 {{expected data member}}
  // expected-error@+4 {{expected data member}}
  // expected-error@+4 {{expected data member}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data pcopyout(obj.fn)            \
                        present_or_copyout(obj.fn)  \
                        delete(obj.fn)

  // Member expression not permitted: member function call.

  // expected-error@+7 {{expected data member}}
  // expected-error@+7 {{expected data member}}
  // expected-error@+7 {{expected data member}}
  // expected-error@+7 {{expected data member}}
  // expected-error@+7 {{expected data member}}
  // expected-error@+7 {{expected data member}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc enter data'}}
  #pragma acc enter data copyin(obj.fn())             \
                         pcopyin(obj.fn())            \
                         present_or_copyin(obj.fn())  \
                         create(obj.fn())             \
                         pcreate(obj.fn())            \
                         present_or_create(obj.fn())
  // expected-error@+5 {{expected data member}}
  // expected-error@+5 {{expected data member}}
  // expected-error@+5 {{expected data member}}
  // expected-error@+5 {{expected data member}}
  // expected-error@+1 {{expected at least one data clause for '#pragma acc exit data'}}
  #pragma acc exit data copyout(obj.fn())             \
                        pcopyout(obj.fn())            \
                        present_or_copyout(obj.fn())  \
                        delete(obj.fn())

  return 0;
}
