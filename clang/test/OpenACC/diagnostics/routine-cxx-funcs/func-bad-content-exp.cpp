// Check accelerator routines with explicit routine directives and contents that
// are not permitted.
//
// RUN: %clang_cc1 -fopenacc -Wno-openacc-and-cxx %s \
// RUN:     -verify=funcBadContent,funcBadContentExp \
// RUN:     -fexceptions -fcxx-exceptions -Wno-unevaluated-expression
//
// END.

#define USEE_ROUTINE_DIR _Pragma("acc routine seq")
#define USEE_BAD_CONTENT 1
#include "usee-defs.inc" // diagnostics reported here