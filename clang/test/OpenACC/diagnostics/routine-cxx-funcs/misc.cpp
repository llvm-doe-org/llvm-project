// RUN: %clang_cc1 -fopenacc -Wno-openacc-and-cxx -verify %s \
// RUN:     -fexceptions -fcxx-exceptions -Wno-unevaluated-expression
//
// END.

//------------------------------------------------------------------------------
// Check def of an accelerator routine before an explicit routine directive.
//
// There seems to be no way to make that happen for class member functions until
// we support routine directives with names.
//------------------------------------------------------------------------------

struct EarlyDefs {};
// expected-note@+1 {{definition of function 'operator+' appears here}}
EarlyDefs operator+(EarlyDefs x, EarlyDefs y) { return x; }
// expected-error@+1 {{first '#pragma acc routine' for function 'operator+' not in scope at definition}}
#pragma acc routine seq
EarlyDefs operator+(EarlyDefs x, EarlyDefs y);

//------------------------------------------------------------------------------
// Check an accelerator routine with conflicting levels of parallelism from
// multiple routine directives.
//
// There seems to be no way to make that happen for class member functions until
// we support routine directives with names or routine directives in classes
// (that is, not at file scope).
//------------------------------------------------------------------------------

struct MultipleParLevels {
  int operator*() const;
};

// expected-note@+1 {{previous '#pragma acc routine' for function 'operator*' appears here}}
#pragma acc routine seq
MultipleParLevels operator*(MultipleParLevels x, MultipleParLevels y);

// expected-error@+1 {{for function 'operator*', '#pragma acc routine gang' conflicts with previous '#pragma acc routine seq'}}
#pragma acc routine gang
MultipleParLevels operator*(MultipleParLevels x, MultipleParLevels y);

//------------------------------------------------------------------------------
// Check that C++ function names are reported properly when noting implicit
// routine directives.
//
// func-bad-content-imp.cpp checks this for one context (currently that's the
// body of another accelerator routine) that can imply a routine directive, and
// it does so for many kinds of C++ functions.  Here, we focus on the various
// contexts in which it can be implied (which exercise different parts of the
// implementation in some cases), and we do not try to cover all kinds of C++
// functions (but we vary that some).
//------------------------------------------------------------------------------

struct ImpAccRoutineNote {
  ImpAccRoutineNote() {
    // expected-error@+2 {{static local variable 'x' is not permitted within function 'ImpAccRoutineNote::ImpAccRoutineNote' because the latter is attributed with '#pragma acc routine'}}
    // expected-note@#ImpAccRoutineNote_ctor_use {{'#pragma acc routine seq' implied for function 'ImpAccRoutineNote::ImpAccRoutineNote' by use in construct '#pragma acc parallel' here}}
    static int x;
  }
  int operator-() {
    // expected-error@+2 {{static local variable 'x' is not permitted within function 'ImpAccRoutineNote::operator-' because the latter is attributed with '#pragma acc routine'}}
    // expected-note@#ImpAccRoutineNote_opUnary_use {{'#pragma acc routine seq' implied for function 'ImpAccRoutineNote::operator-' by use in construct '#pragma acc parallel' here}}
    static int x;
    return 0;
  }
  int operator+(int) {
    // expected-error@+3 {{static local variable 'x' is not permitted within function 'ImpAccRoutineNote::operator+' because the latter is attributed with '#pragma acc routine'}}
    // expected-note@#ImpAccRoutineNote_opBinary_use {{'#pragma acc routine seq' implied for function 'ImpAccRoutineNote::operator+' by use in function 'ImpAccRoutineNote::operator delete' here}}
    // expected-note@#ImpAccRoutineNote_delete_routine {{'#pragma acc routine' for function 'ImpAccRoutineNote::operator delete' appears here}}
    static int x;
    return 0;
  }
  operator int() {
    // expected-error@+3 {{static local variable 'x' is not permitted within function 'ImpAccRoutineNote::operator int' because the latter is attributed with '#pragma acc routine'}}
    // expected-note@#ImpAccRoutineNote_opConvert_use {{'#pragma acc routine seq' implied for function 'ImpAccRoutineNote::operator int' by use in function 'ImpAccRoutineNote::operator delete' here}}
    // expected-note@#ImpAccRoutineNote_delete_routine {{'#pragma acc routine' for function 'ImpAccRoutineNote::operator delete' appears here}}
    static int x;
    return 0;
  }
  void *operator new(unsigned long);
  void operator delete(void *);
};

void *ImpAccRoutineNote::operator new(unsigned long) {
  #pragma acc parallel
  {
    ImpAccRoutineNote obj; // #ImpAccRoutineNote_ctor_use
    #pragma acc loop gang
    for (int i = 0; i < 5; ++i)
      -obj; // #ImpAccRoutineNote_opUnary_use
  }
}

#pragma acc routine gang // #ImpAccRoutineNote_delete_routine
void ImpAccRoutineNote::operator delete(void *ptr) {
  ImpAccRoutineNote *objPtr = (ImpAccRoutineNote*)ptr;
  *objPtr + 3; // #ImpAccRoutineNote_opBinary_use
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    int j = (int)*objPtr; // #ImpAccRoutineNote_opConvert_use
}
