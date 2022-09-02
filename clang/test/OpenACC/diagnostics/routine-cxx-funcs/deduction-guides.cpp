// Check routine directive with C++ template argument deduction guides (C++17).
//
// C++14 is currently the default.  C++17 is required to recognize C++ template
// argument deduction guides.

// DEFINE: %{check}( OPTS %) =                                                 \
// DEFINE:   %clang_cc1 -fopenacc -Wno-openacc-and-cxx %{OPTS} %s              \
// DEFINE:       -fexceptions -fcxx-exceptions -Wno-unevaluated-expression

// RUN:%{check}( -std=c++14 -verify=expected,cxx14 %)
// RUN:%{check}( -std=c++17 -verify=expected,cxx17 %)
//
// END.

template <typename T>
struct DeductionGuide { // #DeductionGuide
  T i;
  DeductionGuide(T i) : i(i) {}
};

// A deduction guide is not a real function and has no body, so an acc routine
// directive for it would be meaningless and is not permitted.
// expected-error@+1 {{#pragma acc routine' must be followed by a lone function prototype or definition}}
#pragma acc routine seq
// cxx14-error@+4 {{use of class template 'DeductionGuide' requires template arguments}}
// cxx14-note@#DeductionGuide {{template is declared here}}
// cxx14-error@+2 {{expected ';' at end of declaration}}
// cxx14-error@+1 {{cannot use arrow operator on a type}}
template <typename T> DeductionGuide(T) -> DeductionGuide<T>;

// Comparing C++14 and C++17 diagnostics here shows C++17 uses the above
// deduction guide for the obj declaration here.  Using a real function in an
// accelerator routine implies a routine directive, but using a deduction guide
// there does not.  Thus, Clang should suppress the implicit routine directive
// without complaining about it.
#pragma acc routine seq
void DeductionGuide_acceleratorUse() {
  // cxx14-error@+2 {{use of class template 'DeductionGuide' requires template arguments}}
  // cxx14-note@#DeductionGuide {{template is declared here}}
  DeductionGuide obj(3);
}
