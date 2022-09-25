// Check accelerator routines with explicit routine directives and contents that
// are not permitted.
//
// Here we check various kinds of diagnostics in various kinds of C++ functions.
// For class/namespace member functions, here we only try with the routine
// directives in the classes/namespaces and the function definitions after.
// ../routine.cpp checks various cases where member function
// prototypes/definitions/uses appear within or after the class/namespace.
//
// RUN: %clang_cc1 -fopenacc -Wno-openacc-and-cxx %s \
// RUN:     -verify=funcBadContent,funcBadContentExp \
// RUN:     -fexceptions -fcxx-exceptions -Wno-unevaluated-expression
//
// END.

#define USEE_ADD_DEF_TO_DECL 0
#define USEE_ROUTINE_DIR _Pragma("acc routine seq")
#define USEE_BAD_CONTENT 1
#include "usee-defs.inc" // diagnostics reported here
