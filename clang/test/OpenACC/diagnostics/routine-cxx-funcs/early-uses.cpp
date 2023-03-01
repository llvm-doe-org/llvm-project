// Check use of an accelerator routine before an explicit routine directive.
//
// RUN: %clang_cc1 -fopenacc -Wno-openacc-and-cxx -verify=earlyUses %s \
// RUN:     -fexceptions -fcxx-exceptions -Wno-unevaluated-expression
//
// END.

// Include the users before the usee function definitions that bear the routine
// directives so that the routine directives are diagnosed as too late (that is,
// uses are too early).
//
// We assume that, if each kind of use for a kind of function is noted at the
// explicit routine directive error, then that use alone would have been
// sufficient to cause the error.  That dimension of the logic is checked
// thoroughly for C elsewhere.
//
// ../routine.cpp checks various cases where member function
// prototypes/definitions/uses appear within or after the class/namespace.
// Here, member function definitions are always after the class/namespace.

#define USEE_ADD_DEF_TO_DECL 0
#define USER_ROUTINE_DIR
#define USER_USER_ROUTINE_DIR
#define USER_LOOP_DIR _Pragma("acc loop vector")
#define USER_LOOP_ROUTINE_DIR _Pragma("acc routine vector")
#define INCLUDE_LAMBDA_USES 0 // lambda can't be used before def or routine dir
#include "users.inc" // uses recorded

#define USEE_ROUTINE_DIR _Pragma("acc routine seq")
#define USEE_BAD_CONTENT 0
#include "usee-defs.inc" // diagnostics reported here
