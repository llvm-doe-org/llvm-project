// Check accelerator routines with explicit routine directives and contents that
// are not permitted.
//
// In the case of a lambda, for which an explicit routine directive currently is
// not possible, the lambda's definition implies the routine directive instead.
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
#define USEE_ROUTINE_DIR _Pragma("acc routine vector")
#define USEE_BAD_CONTENT 1
#define INCLUDE_LAMBDA_USES 1
#include "usee-defs.inc" // diagnostics reported here
