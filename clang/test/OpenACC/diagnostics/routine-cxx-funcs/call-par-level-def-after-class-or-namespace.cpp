// Check accelerator routines called where they are not permitted due to their
// levels of parallelism.  Accelerator routine definitions appear after the
// class or namespace.
//
// Additionally, because this test declares functions without routine directives
// (except in the case of lambdas), then separately defines them with routine
// directives, and then uses them on the accelerator, it checks that implicit
// routine directive analysis uses each function's most recent declaration.  In
// the past, when such a function was a member conversion operator (but this
// test covers many other kinds of functions, which could develop the same
// problem), Clang called its MarkFunctionReferenced with the original
// declaration of the operator, producing an assert fail in Clang's implicit
// routine directive analysis because it didn't consistently see the explicit
// routine directive from the most recent declaration.  Now, Clang's OpenACC
// analysis is careful to consistently look up the most recent declaration of
// any function that is passed to it from elsewhere.
//
// RUN: %clang_cc1 -fopenacc -Wno-openacc-and-cxx -verify=callParLevel %s \
// RUN:     -fexceptions -fcxx-exceptions -Wno-unevaluated-expression
//
// END.

// Include the usee function definitions with their routine directives, and then
// include the users so it will complain about incompatible levels of
// parallelism (and not about late routine directives).

#define USEE_ADD_DEF_TO_DECL 0
#define USEE_ROUTINE_DIR _Pragma("acc routine gang")
#define USEE_BAD_CONTENT 0
#include "usee-defs.inc" // usee routine directives specified here

#define USER_ROUTINE_DIR _Pragma("acc routine worker")
#define USER_USER_ROUTINE_DIR _Pragma("acc routine vector")
#define USER_LOOP_DIR _Pragma("acc loop worker")
#define USER_LOOP_ROUTINE_DIR _Pragma("acc routine worker")
#define INCLUDE_LAMBDA_USES 1
#include "users.inc" // diagnostics reported here
