// Check accelerator routines with implicit routine directives and contents that
// are not permitted.
//
// Our goal here isn't to check all kinds of uses that can imply a routine
// directive.  Testing for routine directives in C already thoroughly checks
// fundamental cases, and early-uses.cpp checks that various special C++
// usee/user relationship are generally seen by Clang's OpenACC analysis.
// Instead, the goal here is to make sure that various diagnostics that are
// recorded in the bodies of various special C++ functions are reported
// successfully later when the routine directives are implied.  ../routine.cpp
// checks various contexts in which routine directives can be implied.
//
// RUN: %clang_cc1 -fopenacc -Wno-openacc-and-cxx %s \
// RUN:     -verify=funcBadContent,funcBadContentImp \
// RUN:     -fexceptions -fcxx-exceptions -Wno-unevaluated-expression
//
// END.

#define USEE_ADD_DEF_TO_DECL 0
#define USEE_ROUTINE_DIR // no directive so that it's implied later
#define USEE_BAD_CONTENT 1
#include "usee-defs.inc" // diagnostics recorded

#define USER_ROUTINE_DIR _Pragma("acc routine seq")
#define USER_USER_ROUTINE_DIR _Pragma("acc routine seq")
#define USER_LOOP_DIR _Pragma("acc loop seq")
#define USER_LOOP_ROUTINE_DIR _Pragma("acc routine seq")
#define INCLUDE_LAMBDA_USES 1
#include "users.inc" // routine directives implied (usually multiple times, but
                     // we don't care which ones), triggering recorded diags
