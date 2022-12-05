// This test checks -fopenacc-ast-print output formatting, mostly for correct
// indentation in various case.  The indentation isn't particular pretty, but we
// want to be sure it's consistent.
//
// TODO: There's much more that could be checked here, especially for
// OpenACC constructs and executable directives.  We should probably mimic the
// structure of fopenacc-print-torture.c while omitting many checks that aren't
// relevant to -fopenacc-ast-print.

// Strip comments and blank lines so checking -fopenacc-ast-print output is
// easier.
// RUN: grep -v '^ *\(//.*\)\?$' %s | sed 's,//.*,,' > %t-acc.c

// DEFINE: %{check-prt-arg}( PRT_ARG %, PRT_CHK %) =                           \
// DEFINE:   : '----------- -fopenacc-ast-print=%{PRT_ARG} -----------'  &&    \
// DEFINE:   %clang -Xclang -verify -fopenacc-ast-print=%{PRT_ARG}             \
// DEFINE:          %t-acc.c -Wno-openacc-omp-ext |                            \
// DEFINE:     FileCheck -check-prefixes=%{PRT_CHK} -match-full-lines          \
// DEFINE:               -strict-whitespace %s

//                        PRT_ARG    PRT_CHK
// RUN: %{check-prt-arg}( acc     %, PRT,PRT-A,PRT-AA %)
// RUN: %{check-prt-arg}( omp     %, PRT,PRT-O,PRT-OO %)
// RUN: %{check-prt-arg}( acc-omp %, PRT,PRT-A,PRT-AO %)
// RUN: %{check-prt-arg}( omp-acc %, PRT,PRT-O,PRT-OA %)

// END.

/* expected-no-diagnostics */

// Generate unique name based on the line number.
#define UNIQUE_NAME CONCAT2(unique_name_, __LINE__)
#define CONCAT2(X, Y) CONCAT(X, Y)
#define CONCAT(X, Y) X##Y

// PRT:int start;
int start;

//------------------------------------------------------------------------------
// A few basic checks for OpenACC constructs and executable directives.
//------------------------------------------------------------------------------

// PRT-NEXT:void unique_name_{{[0-9]+}}() {
void UNIQUE_NAME() {
  //  PRT-A-NEXT:    #pragma acc parallel
  // PRT-AO-NEXT:    // #pragma omp target teams
  //  PRT-O-NEXT:    #pragma omp target teams
  // PRT-OA-NEXT:    // #pragma acc parallel
  //    PRT-NEXT:        {
  //  PRT-A-NEXT:            #pragma acc loop gang
  // PRT-AO-NEXT:            // #pragma omp distribute
  //  PRT-O-NEXT:            #pragma omp distribute
  // PRT-OA-NEXT:            // #pragma acc loop gang
  //    PRT-NEXT:                for (int i = 0; i < 5; ++i)
  //    PRT-NEXT:                    ;
  // PRT-AA-NEXT:            #pragma acc loop seq
  // PRT-AA-NEXT:                for (int i = 0; i < 5; ++i)
  // PRT-AA-NEXT:                    ;
  // PRT-AO-NEXT:            #pragma acc loop seq // discarded in OpenMP translation
  // PRT-AO-NEXT:                for (int i = 0; i < 5; ++i)
  // PRT-AO-NEXT:                    ;
  // PRT-OO-NEXT:            for (int i = 0; i < 5; ++i)
  // PRT-OO-NEXT:                ;
  // PRT-OA-NEXT:            // #pragma acc loop seq // discarded in OpenMP translation
  // PRT-OA-NEXT:                for (int i = 0; i < 5; ++i)
  // PRT-OA-NEXT:                    ;
  //    PRT-NEXT:        }
  #pragma acc parallel
  {
    #pragma acc loop gang
    for (int i = 0; i < 5; ++i)
      ;
    #pragma acc loop seq
    for (int i = 0; i < 5; ++i)
      ;
  }
} // PRT-NEXT:}

//------------------------------------------------------------------------------
// Routine directives.
//------------------------------------------------------------------------------

// Check on function prototype when routine directive has OpenMP translation.
//
//  PRT-A-NEXT:#pragma acc routine seq
// PRT-AO-NEXT:// #pragma omp declare target
//  PRT-O-NEXT:#pragma omp declare target
// PRT-OA-NEXT:// #pragma acc routine seq
//    PRT-NEXT:void unique_name_{{[0-9]+}}();
// PRT-AO-NEXT:// #pragma omp end declare target
//  PRT-O-NEXT:#pragma omp end declare target
#pragma acc routine seq
void UNIQUE_NAME();

// PRT-NEXT:void implicitRoutineSeq();
void implicitRoutineSeq();

// Check on function definition when routine directive has OpenMP translation.
//
//  PRT-A-NEXT:#pragma acc routine seq
// PRT-AO-NEXT:// #pragma omp declare target
//  PRT-O-NEXT:#pragma omp declare target
// PRT-OA-NEXT:// #pragma acc routine seq
//    PRT-NEXT:void inheritedRoutineSeq() {
//    PRT-NEXT:    implicitRoutineSeq();
//    PRT-NEXT:}
// PRT-AO-NEXT:// #pragma omp end declare target
//  PRT-O-NEXT:#pragma omp end declare target
#pragma acc routine seq
void inheritedRoutineSeq() {
  implicitRoutineSeq();
}

// Check that implicit and inherited routine directives do not affect printing.
//
// PRT-NEXT:void implicitRoutineSeq();
// PRT-NEXT:void inheritedRoutineSeq();
// PRT-NEXT:void unique_name_{{[0-9]+}}() {
// PRT-NEXT:    void implicitRoutineSeq();
// PRT-NEXT:    void inheritedRoutineSeq();
// PRT-NEXT:}
void implicitRoutineSeq();
void inheritedRoutineSeq();
void UNIQUE_NAME() {
  void implicitRoutineSeq();
  void inheritedRoutineSeq();
}

// Check on function prototype when routine directive is discarded in OpenMP
// translation.
//
//    PRT-NEXT:void unique_name_{{[0-9]+}}() {
// PRT-AA-NEXT:    #pragma acc routine seq
// PRT-AO-NEXT:    #pragma acc routine seq // discarded in OpenMP translation
// PRT-OA-NEXT:    // #pragma acc routine seq // discarded in OpenMP translation
//    PRT-NEXT:    void unique_name_{{[0-9]+}}();
//    PRT-NEXT:}
void UNIQUE_NAME() {
  #pragma acc routine seq
  void UNIQUE_NAME();
}

// In C, we seem to have no way to check a routine directive that has an OpenMP
// translation and non-empty indentation because currently a routine directive
// with an OpenMP translation is always at file scope.  In C++, that's not true
// because of classes and namespaces, so fopenacc-ast-print-torture.cpp covers
// that case.

// PRT-NOT: {{.}}
