// Check accelerator routines called where they are not permitted due to their
// levels of parallelism.  Accelerator routine definitions appear in the class
// or namespace.
//
// ../routine.cpp covers the case where users are in the same class or
// namespace.
//
// RUN: %clang_cc1 -fopenacc -Wno-openacc-and-cxx -verify=callParLevel %s \
// RUN:     -fexceptions -fcxx-exceptions -Wno-unevaluated-expression
//
// END.

// Include the usee function definitions with their routine directives, and then
// include the users so it will complain about incompatible levels of
// parallelism (and not about late routine directives).

#define USEE_ADD_DEF_TO_DECL 1
#define USEE_ROUTINE_DIR _Pragma("acc routine gang")
#include "usee-decls.h" // usee routine directives specified here

#define USER_ROUTINE_DIR _Pragma("acc routine worker")
#define USER_USER_ROUTINE_DIR _Pragma("acc routine vector")
#define USER_LOOP_DIR _Pragma("acc loop worker")
#define USER_LOOP_ROUTINE_DIR _Pragma("acc routine worker")
#define INCLUDE_LAMBDA_USES 1
#include "users.inc" // diagnostics reported here
