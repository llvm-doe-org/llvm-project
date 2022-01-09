// This test checks cases where -fopenacc-print cannot handle macro expansions
// and so produces error diagnostics.  Error recovery is always expected so
// that additional cases can be reported as well, and the partially translated
// output is still printed in case that is useful to the user.

// Strip comments and blank lines so checking -fopenacc-print output is easier.
// RUN: grep -v '^ *\(//.*\)\?$' %s | sed 's,//.*,,' > %t-acc.c

// RUN: %data prt-args {
// RUN:   (prt-arg=acc     prt-chk=PRT,PRT-A        prt-ver=noerrs)
// RUN:   (prt-arg=omp     prt-chk=PRT,PRT-O        prt-ver=expected)
// RUN:   (prt-arg=acc-omp prt-chk=PRT,PRT-A,PRT-AO prt-ver=expected)
// RUN:   (prt-arg=omp-acc prt-chk=PRT,PRT-O,PRT-OA prt-ver=expected)
// RUN: }
// RUN: %for prt-args {
// RUN:   %clang -Xclang -verify=%[prt-ver] -fopenacc-print=%[prt-arg] \
// RUN:          -Wno-openacc-omp-update-present -ferror-limit=100 %t-acc.c \
// RUN:   | FileCheck -check-prefixes=%[prt-chk] -match-full-lines \
// RUN:               -strict-whitespace %s
// RUN: }

// END.

/* noerrs-no-diagnostics */

//      PRT:int i;
// PRT-NEXT:int non_const_expr = 2;
int i;
int non_const_expr = 2;

//--------------------------------------------------
// Translatable directives are translated before any error.
//--------------------------------------------------

// PRT-NEXT:void beforeError() {
void beforeError() {
  //  PRT-A-NEXT:  #pragma acc parallel
  // PRT-AO-NEXT:  // #pragma omp target teams
  //  PRT-O-NEXT:  #pragma omp target teams
  // PRT-OA-NEXT:  // #pragma acc parallel
  //    PRT-NEXT:  ;
  #pragma acc parallel
  ;

  //  PRT-A-NEXT:  #pragma acc update device(i)
  // PRT-AO-NEXT:  // #pragma omp target update to(present: i)
  //  PRT-O-NEXT:  #pragma omp target update to(present: i)
  // PRT-OA-NEXT:  // #pragma acc update device(i)
  #pragma acc update device(i)
}// PRT-NEXT:}

//  PRT-A-NEXT:#pragma acc routine seq
// PRT-AO-NEXT:// #pragma omp declare target
//  PRT-O-NEXT:#pragma omp declare target
// PRT-OA-NEXT:// #pragma acc routine seq
//    PRT-NEXT:void beforeError_routine();
//  PRT-O-NEXT:#pragma omp end declare target
// PRT-AO-NEXT:// #pragma omp end declare target
#pragma acc routine seq
void beforeError_routine();

//--------------------------------------------------
// Directive in macro expansion: yes
// No associated code.
//--------------------------------------------------

// PRT-NEXT:void inMacroNoAssoc() {
void inMacroNoAssoc() {
  // PRT-NEXT:  #define MAC _Pragma("acc update device(i)")
  // PRT-NEXT:  /* expected-error{{.*}} */
  // PRT-NEXT:  MAC
  // PRT-NEXT:  #undef MAC
  #define MAC _Pragma("acc update device(i)")
  /* expected-error@+1 {{cannot rewrite OpenACC construct starting within a macro expansion}} */
  MAC
  #undef MAC
}// PRT-NEXT:}

//--------------------------------------------------
// Directive in macro expansion: yes
// Associated code ends in macro expansion: no
//--------------------------------------------------

// PRT-NEXT:void inMacroAssocEndNotInMacro() {
void inMacroAssocEndNotInMacro() {
  // PRT-NEXT:  #define MAC _Pragma("acc parallel")
  // PRT-NEXT:  /* expected-error{{.*}} */
  // PRT-NEXT:  MAC
  // PRT-NEXT:  ;
  // PRT-NEXT:  #undef MAC
  #define MAC _Pragma("acc parallel")
  /* expected-error@+1 {{cannot rewrite OpenACC construct starting within a macro expansion}} */
  MAC
  ;
  #undef MAC
}// PRT-NEXT:}

// PRT-NEXT:#define MAC _Pragma("acc routine seq")
// PRT-NEXT:/* expected-error{{.*}} */
// PRT-NEXT:MAC
// PRT-NEXT:void inMacroAssocEndNotInMacro_routine();
// PRT-NEXT:#undef MAC
#define MAC _Pragma("acc routine seq")
/* expected-error@+1 {{cannot rewrite OpenACC routine directive that appears within a _Pragma operator}} */
MAC
void inMacroAssocEndNotInMacro_routine();
#undef MAC

//--------------------------------------------------
// Directive in macro expansion: yes
// Associated code ends in macro expansion: yes
//
// Associated null statements, compound statements, and expression statements
// exercise different code paths, so check all of them.  For expression
// statements, whether the token preceding the semicolon is part of the same
// macro expansion also affects the code path.
//--------------------------------------------------

// PRT-NEXT:void inMacroAssocEndInMacro() {
void inMacroAssocEndInMacro() {
  // PRT-NEXT:  #define MAC _Pragma("acc parallel") ;
  // PRT-NEXT:  /* expected-error{{.*}} */
  // PRT-NEXT:  MAC
  // PRT-NEXT:  #undef MAC
  #define MAC _Pragma("acc parallel") ;
  /* expected-error@+1 {{cannot rewrite OpenACC construct starting within a macro expansion}} */
  MAC
  #undef MAC

  // PRT-NEXT:  #define MAC1 _Pragma("acc parallel")
  // PRT-NEXT:  #define MAC2 ;
  // PRT-NEXT:  /* expected-error{{.*}} */
  // PRT-NEXT:  MAC1
  // PRT-NEXT:  MAC2
  // PRT-NEXT:  #undef MAC1
  // PRT-NEXT:  #undef MAC2
  #define MAC1 _Pragma("acc parallel")
  #define MAC2 ;
  /* expected-error@+1 {{cannot rewrite OpenACC construct starting within a macro expansion}} */
  MAC1
  MAC2
  #undef MAC1
  #undef MAC2

  // PRT-NEXT:  #define MAC _Pragma("acc parallel") {}
  // PRT-NEXT:  /* expected-error{{.*}} */
  // PRT-NEXT:  MAC
  // PRT-NEXT:  #undef MAC
  #define MAC _Pragma("acc parallel") {}
  /* expected-error@+1 {{cannot rewrite OpenACC construct starting within a macro expansion}} */
  MAC
  #undef MAC

  // PRT-NEXT:  #define MAC1 _Pragma("acc parallel")
  // PRT-NEXT:  #define MAC2 }
  // PRT-NEXT:  /* expected-error{{.*}} */
  // PRT-NEXT:  MAC1
  // PRT-NEXT:  {MAC2
  // PRT-NEXT:  #undef MAC1
  // PRT-NEXT:  #undef MAC2
  #define MAC1 _Pragma("acc parallel")
  #define MAC2 }
  /* expected-error@+1 {{cannot rewrite OpenACC construct starting within a macro expansion}} */
  MAC1
  {MAC2
  #undef MAC1
  #undef MAC2

  // PRT-NEXT:  #define MAC _Pragma("acc parallel") i = 3;
  // PRT-NEXT:  /* expected-error{{.*}} */
  // PRT-NEXT:  MAC
  // PRT-NEXT:  #undef MAC
  #define MAC _Pragma("acc parallel") i = 3;
  /* expected-error@+1 {{cannot rewrite OpenACC construct starting within a macro expansion}} */
  MAC
  #undef MAC

  // PRT-NEXT:  #define MAC1 _Pragma("acc parallel")
  // PRT-NEXT:  #define MAC2 i = 3;
  // PRT-NEXT:  /* expected-error{{.*}} */
  // PRT-NEXT:  MAC1
  // PRT-NEXT:  MAC2
  // PRT-NEXT:  #undef MAC1
  // PRT-NEXT:  #undef MAC2
  #define MAC1 _Pragma("acc parallel")
  #define MAC2 i = 3;
  /* expected-error@+1 {{cannot rewrite OpenACC construct starting within a macro expansion}} */
  MAC1
  MAC2
  #undef MAC1
  #undef MAC2

  // PRT-NEXT:  #define MAC1 _Pragma("acc parallel")
  // PRT-NEXT:  #define MAC2 ;
  // PRT-NEXT:  /* expected-error{{.*}} */
  // PRT-NEXT:  MAC1
  // PRT-NEXT:  i = 3 MAC2
  // PRT-NEXT:  #undef MAC1
  // PRT-NEXT:  #undef MAC2
  #define MAC1 _Pragma("acc parallel")
  #define MAC2 ;
  /* expected-error@+1 {{cannot rewrite OpenACC construct starting within a macro expansion}} */
  MAC1
  i = 3 MAC2
  #undef MAC1
  #undef MAC2

  // PRT-NEXT:  #define MAC1 _Pragma("acc parallel")
  // PRT-NEXT:  #define MAC2 i = 3
  // PRT-NEXT:  #define MAC3 ;
  // PRT-NEXT:  /* expected-error{{.*}} */
  // PRT-NEXT:  MAC1
  // PRT-NEXT:  MAC2 MAC3
  // PRT-NEXT:  #undef MAC1
  // PRT-NEXT:  #undef MAC2
  // PRT-NEXT:  #undef MAC3
  #define MAC1 _Pragma("acc parallel")
  #define MAC2 i = 3
  #define MAC3 ;
  /* expected-error@+1 {{cannot rewrite OpenACC construct starting within a macro expansion}} */
  MAC1
  MAC2 MAC3
  #undef MAC1
  #undef MAC2
  #undef MAC3
}// PRT-NEXT:}

// PRT-NEXT:#define MAC _Pragma("acc routine seq") void inMacroAssocEndInMacro_routineAllIn();
// PRT-NEXT:/* expected-error{{.*}} */
// PRT-NEXT:MAC
// PRT-NEXT:#undef MAC
#define MAC _Pragma("acc routine seq") void inMacroAssocEndInMacro_routineAllIn();
/* expected-error@+1 {{cannot rewrite OpenACC routine directive that appears within a _Pragma operator}} */
MAC
#undef MAC

// PRT-NEXT:#define MAC1 _Pragma("acc routine seq")
// PRT-NEXT:#define MAC2 void inMacroAssocEndInMacro_routineSplit();
// PRT-NEXT:/* expected-error{{.*}} */
// PRT-NEXT:MAC1
// PRT-NEXT:MAC2
// PRT-NEXT:#undef MAC1
// PRT-NEXT:#undef MAC2
#define MAC1 _Pragma("acc routine seq")
#define MAC2 void inMacroAssocEndInMacro_routineSplit();
/* expected-error@+1 {{cannot rewrite OpenACC routine directive that appears within a _Pragma operator}} */
MAC1
MAC2
#undef MAC1
#undef MAC2

// PRT-NEXT:#define MAC1 _Pragma("acc routine seq")
// PRT-NEXT:#define MAC2 ;
// PRT-NEXT:/* expected-error{{.*}} */
// PRT-NEXT:MAC1
// PRT-NEXT:void inMacroAssocEndInMacro_routineSemiIn() MAC2
// PRT-NEXT:#undef MAC1
// PRT-NEXT:#undef MAC2
#define MAC1 _Pragma("acc routine seq")
#define MAC2 ;
/* expected-error@+1 {{cannot rewrite OpenACC routine directive that appears within a _Pragma operator}} */
MAC1
void inMacroAssocEndInMacro_routineSemiIn() MAC2
#undef MAC1
#undef MAC2

// PRT-NEXT:#define MAC1 _Pragma("acc routine seq")
// PRT-NEXT:#define MAC2 void inMacroAssocEndInMacro_routineThree()
// PRT-NEXT:#define MAC3 ;
// PRT-NEXT:/* expected-error{{.*}} */
// PRT-NEXT:MAC1
// PRT-NEXT:MAC2 MAC3
// PRT-NEXT:#undef MAC1
// PRT-NEXT:#undef MAC2
// PRT-NEXT:#undef MAC3
#define MAC1 _Pragma("acc routine seq")
#define MAC2 void inMacroAssocEndInMacro_routineThree()
#define MAC3 ;
/* expected-error@+1 {{cannot rewrite OpenACC routine directive that appears within a _Pragma operator}} */
MAC1
MAC2 MAC3
#undef MAC1
#undef MAC2
#undef MAC3

//--------------------------------------------------
// Directive in macro expansion: no
// No associated code.
//--------------------------------------------------

// PRT-NEXT:void notInMacroNoAssoc() {
void notInMacroNoAssoc() {
  // PRT-NEXT:  /* expected-error{{.*}} */
  // PRT-NEXT:  _Pragma("acc update device(i)")
  /* expected-error@+1 {{cannot rewrite OpenACC directive that has no associated statement and that appears within a _Pragma operator}} */
  _Pragma("acc update device(i)")
}// PRT-NEXT:}

//--------------------------------------------------
// Directive in macro expansion: no
// Associated code ends in macro expansion: no
//--------------------------------------------------

// PRT-NEXT:/* expected-error{{.*}} */
// PRT-NEXT:_Pragma("acc routine seq")
// PRT-NEXT:void notInMacroAssocEndNotInMacro_routine();
/* expected-error@+1 {{cannot rewrite OpenACC routine directive that appears within a _Pragma operator}} */
_Pragma("acc routine seq")
void notInMacroAssocEndNotInMacro_routine();

//--------------------------------------------------
// Directive in macro expansion: no
// Associated code ends in macro expansion: yes
//
// This exercises only cases where the associated code must be rewritten.  The
// #pragma case where only the directive needs to be rewritten succeeds and
// thus is exercised in fopenacc-print-torture.c.
//
// Associated null statements, compound statements, expression statements,
// function prototypes, and function definitions exercise different code paths,
// so check all of them.  For expression statements and function prototypes,
// whether the token preceding the semicolon is part of the same macro expansion
// also affects the code path.
//--------------------------------------------------

// PRT-NEXT:void notInMacroAssocEndInMacro() {
void notInMacroAssocEndInMacro() {
  // PRT-NEXT:  #define MAC ;
  // PRT-NEXT:  #pragma acc parallel num_workers(non_const_expr)
  // PRT-NEXT:  #pragma acc loop worker
  // PRT-NEXT:  for (int i = 0; i < 5; ++i)
  // PRT-NEXT:    /* expected-error{{.*}} */
  // PRT-NEXT:    MAC
  // PRT-NEXT:  #undef MAC
  #define MAC ;
  #pragma acc parallel num_workers(non_const_expr)
  #pragma acc loop worker
  for (int i = 0; i < 5; ++i)
    /* expected-error@+1 {{cannot rewrite OpenACC construct ending within a macro expansion}} */
    MAC
  #undef MAC

  // PRT-NEXT:  #define MAC ;
  // PRT-NEXT:  _Pragma("acc parallel")
  // PRT-NEXT:  /* expected-error{{.*}} */
  // PRT-NEXT:  MAC
  // PRT-NEXT:  #undef MAC
  #define MAC ;
  _Pragma("acc parallel")
  /* expected-error@+1 {{cannot rewrite OpenACC construct ending within a macro expansion}} */
  MAC
  #undef MAC

  // PRT-NEXT:  #define MAC {}
  // PRT-NEXT:  #pragma acc parallel num_workers(non_const_expr)
  // PRT-NEXT:  #pragma acc loop worker
  // PRT-NEXT:  for (int i = 0; i < 5; ++i)
  // PRT-NEXT:    /* expected-error{{.*}} */
  // PRT-NEXT:    MAC
  // PRT-NEXT:  #undef MAC
  #define MAC {}
  #pragma acc parallel num_workers(non_const_expr)
  #pragma acc loop worker
  for (int i = 0; i < 5; ++i)
    /* expected-error@+1 {{cannot rewrite OpenACC construct ending within a macro expansion}} */
    MAC
  #undef MAC

  // PRT-NEXT:  #define MAC {}
  // PRT-NEXT:  _Pragma("acc parallel")
  // PRT-NEXT:  /* expected-error{{.*}} */
  // PRT-NEXT:  MAC
  // PRT-NEXT:  #undef MAC
  #define MAC {}
  _Pragma("acc parallel")
  /* expected-error@+1 {{cannot rewrite OpenACC construct ending within a macro expansion}} */
  MAC
  #undef MAC

  // PRT-NEXT:  #define MAC }
  // PRT-NEXT:  #pragma acc parallel num_workers(non_const_expr)
  // PRT-NEXT:  #pragma acc loop worker
  // PRT-NEXT:  for (int i = 0; i < 5; ++i)
  // PRT-NEXT:    /* expected-error{{.*}} */
  // PRT-NEXT:    {MAC
  // PRT-NEXT:  #undef MAC
  #define MAC }
  #pragma acc parallel num_workers(non_const_expr)
  #pragma acc loop worker
  for (int i = 0; i < 5; ++i)
    /* expected-error@+1 {{cannot rewrite OpenACC construct ending within a macro expansion}} */
    {MAC
  #undef MAC

  // PRT-NEXT:  #define MAC }
  // PRT-NEXT:  _Pragma("acc parallel")
  // PRT-NEXT:  /* expected-error{{.*}} */
  // PRT-NEXT:  {MAC
  // PRT-NEXT:  #undef MAC
  #define MAC }
  _Pragma("acc parallel")
  /* expected-error@+1 {{cannot rewrite OpenACC construct ending within a macro expansion}} */
  {MAC
  #undef MAC

  // PRT-NEXT:  #define MAC (i = 3);
  // PRT-NEXT:  #pragma acc parallel num_workers(non_const_expr)
  // PRT-NEXT:  #pragma acc loop worker
  // PRT-NEXT:  for (int j = 0; j < 5; ++j)
  // PRT-NEXT:    /* expected-error{{.*}} */
  // PRT-NEXT:    MAC
  // PRT-NEXT:  #undef MAC
  #define MAC (i = 3);
  #pragma acc parallel num_workers(non_const_expr)
  #pragma acc loop worker
  for (int j = 0; j < 5; ++j)
    /* expected-error@+1 {{cannot rewrite OpenACC construct ending within a macro expansion}} */
    MAC
  #undef MAC

  // PRT-NEXT:  #define MAC i = 3;
  // PRT-NEXT:  _Pragma("acc parallel")
  // PRT-NEXT:  /* expected-error{{.*}} */
  // PRT-NEXT:  MAC
  // PRT-NEXT:  #undef MAC
  #define MAC i = 3;
  _Pragma("acc parallel")
  /* expected-error@+1 {{cannot rewrite OpenACC construct ending within a macro expansion}} */
  MAC
  #undef MAC

  // PRT-NEXT:  #define MAC ;
  // PRT-NEXT:  #pragma acc parallel num_workers(non_const_expr)
  // PRT-NEXT:  #pragma acc loop worker
  // PRT-NEXT:  for (int j = 0; j < 5; ++j)
  // PRT-NEXT:    /* expected-error{{.*}} */
  // PRT-NEXT:    (i = 3) MAC
  // PRT-NEXT:  #undef MAC
  #define MAC ;
  #pragma acc parallel num_workers(non_const_expr)
  #pragma acc loop worker
  for (int j = 0; j < 5; ++j)
    /* expected-error@+1 {{cannot rewrite OpenACC construct ending within a macro expansion}} */
    (i = 3) MAC
  #undef MAC

  // PRT-NEXT:  #define MAC ;
  // PRT-NEXT:  _Pragma("acc parallel")
  // PRT-NEXT:  /* expected-error{{.*}} */
  // PRT-NEXT:  i = 3 MAC
  // PRT-NEXT:  #undef MAC
  #define MAC ;
  _Pragma("acc parallel")
  /* expected-error@+1 {{cannot rewrite OpenACC construct ending within a macro expansion}} */
  i = 3 MAC
  #undef MAC

  // PRT-NEXT:  #define MAC1 )
  // PRT-NEXT:  #define MAC2 ;
  // PRT-NEXT:  #pragma acc parallel num_workers(non_const_expr)
  // PRT-NEXT:  #pragma acc loop worker
  // PRT-NEXT:  for (int j = 0; j < 5; ++j)
  // PRT-NEXT:    /* expected-error{{.*}} */
  // PRT-NEXT:    (i = 3 MAC1 MAC2
  // PRT-NEXT:  #undef MAC1
  // PRT-NEXT:  #undef MAC2
  #define MAC1 )
  #define MAC2 ;
  #pragma acc parallel num_workers(non_const_expr)
  #pragma acc loop worker
  for (int j = 0; j < 5; ++j)
    /* expected-error@+1 {{cannot rewrite OpenACC construct ending within a macro expansion}} */
    (i = 3 MAC1 MAC2
  #undef MAC1
  #undef MAC2

  // PRT-NEXT:  #define MAC1 3
  // PRT-NEXT:  #define MAC2 ;
  // PRT-NEXT:  _Pragma("acc parallel")
  // PRT-NEXT:  /* expected-error{{.*}} */
  // PRT-NEXT:  i = MAC1 MAC2
  // PRT-NEXT:  #undef MAC1
  // PRT-NEXT:  #undef MAC2
  #define MAC1 3
  #define MAC2 ;
  _Pragma("acc parallel")
  /* expected-error@+1 {{cannot rewrite OpenACC construct ending within a macro expansion}} */
  i = MAC1 MAC2
  #undef MAC1
  #undef MAC2
}// PRT-NEXT:}

// PRT-NEXT:#define MAC void notInMacroAssocEndInMacro_routineWhole();
// PRT-NEXT:#pragma acc routine seq
// PRT-NEXT:/* expected-error{{.*}} */
// PRT-NEXT:MAC
// PRT-NEXT:#undef MAC
#define MAC void notInMacroAssocEndInMacro_routineWhole();
#pragma acc routine seq
/* expected-error@+1 {{for OpenACC routine directive, cannot rewrite associated function declaration that ends within a macro expansion}} */
MAC
#undef MAC

// PRT-NEXT:#define MAC {}
// PRT-NEXT:#pragma acc routine seq
// PRT-NEXT:/* expected-error{{.*}} */
// PRT-NEXT:void notInMacroAssocEndInMacro_routineDefLastTwoTokens() MAC
// PRT-NEXT:#undef MAC
#define MAC {}
#pragma acc routine seq
/* expected-error@+1 {{for OpenACC routine directive, cannot rewrite associated function declaration that ends within a macro expansion}} */
void notInMacroAssocEndInMacro_routineDefLastTwoTokens() MAC
#undef MAC

// PRT-NEXT:#define MAC }
// PRT-NEXT:#pragma acc routine seq
// PRT-NEXT:/* expected-error{{.*}} */
// PRT-NEXT:void notInMacroAssocEndInMacro_routineDefLastToken() {MAC
// PRT-NEXT:#undef MAC
#define MAC }
#pragma acc routine seq
/* expected-error@+1 {{for OpenACC routine directive, cannot rewrite associated function declaration that ends within a macro expansion}} */
void notInMacroAssocEndInMacro_routineDefLastToken() {MAC
#undef MAC

// PRT-NEXT:#define MAC );
// PRT-NEXT:#pragma acc routine seq
// PRT-NEXT:/* expected-error{{.*}} */
// PRT-NEXT:void notInMacroAssocEndInMacro_routineProtoLastTwoTokens(MAC
// PRT-NEXT:#undef MAC
#define MAC );
#pragma acc routine seq
/* expected-error@+1 {{for OpenACC routine directive, cannot rewrite associated function declaration that ends within a macro expansion}} */
void notInMacroAssocEndInMacro_routineProtoLastTwoTokens(MAC
#undef MAC

// PRT-NEXT:#define MAC ;
// PRT-NEXT:#pragma acc routine seq
// PRT-NEXT:/* expected-error{{.*}} */
// PRT-NEXT:void notInMacroAssocEndInMacro_routineProtoLastToken() MAC
// PRT-NEXT:#undef MAC
#define MAC ;
#pragma acc routine seq
/* expected-error@+1 {{for OpenACC routine directive, cannot rewrite associated function declaration that ends within a macro expansion}} */
void notInMacroAssocEndInMacro_routineProtoLastToken() MAC
#undef MAC

//--------------------------------------------------
// Translatable directives are translated after any error.
//--------------------------------------------------

// PRT-NEXT:void afterError() {
void afterError() {
  //  PRT-A-NEXT:  #pragma acc parallel
  // PRT-AO-NEXT:  // #pragma omp target teams
  //  PRT-O-NEXT:  #pragma omp target teams
  // PRT-OA-NEXT:  // #pragma acc parallel
  //    PRT-NEXT:  ;
  #pragma acc parallel
  ;

  //  PRT-A-NEXT:  #pragma acc update device(i)
  // PRT-AO-NEXT:  // #pragma omp target update to(present: i)
  //  PRT-O-NEXT:  #pragma omp target update to(present: i)
  // PRT-OA-NEXT:  // #pragma acc update device(i)
  #pragma acc update device(i)
}// PRT-NEXT:}

//  PRT-A-NEXT:#pragma acc routine seq
// PRT-AO-NEXT:// #pragma omp declare target
//  PRT-O-NEXT:#pragma omp declare target
// PRT-OA-NEXT:// #pragma acc routine seq
//    PRT-NEXT:void afterError_routine();
//  PRT-O-NEXT:#pragma omp end declare target
// PRT-AO-NEXT:// #pragma omp end declare target
#pragma acc routine seq
void afterError_routine();
