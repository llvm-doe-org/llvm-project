// This test checks -fopenacc-print output for no preprocessor expansion,
// layout preservation, and exact formatting for many cases that are special
// to -fopenacc-print but not so much to -fopenacc-ast-print.  Cases that cannot
// be rewritten are checked in fopenacc-print-messages.c instead.

// Strip comments and blank lines so checking -fopenacc-print output is easier.
// RUN: grep -v '^ *\(//.*\)\?$' %s | sed 's,//.*,,' > %t-acc.c

// Add a couple of blank lines and comments so we can check that those are
// preserved.
// RUN: echo '' >> %t-acc.c
// RUN: echo '  ' >> %t-acc.c
// RUN: echo '// comment' >> %t-acc.c
// RUN: echo '/* multi-line' >> %t-acc.c
// RUN: echo '   comment */' >> %t-acc.c
// RUN: echo 'int foobar; // end-of-line comment' >> %t-acc.c

// RUN: %data cpps {
// RUN:   (cpp=      )
// RUN:   (cpp=-DERRS)
// RUN: }
// RUN: %data prt-args {
// RUN:   (prt-arg=acc     prt-chk=PRT,PRT-A,PRT-AA)
// RUN:   (prt-arg=omp     prt-chk=PRT,PRT-O,PRT-OO)
// RUN:   (prt-arg=acc-omp prt-chk=PRT,PRT-A,PRT-AO)
// RUN:   (prt-arg=omp-acc prt-chk=PRT,PRT-O,PRT-OA)
// RUN: }
// RUN: %for cpps {
// RUN:   %for prt-args {
// RUN:     %clang -Xclang -verify -fopenacc-print=%[prt-arg] %[cpp] %t-acc.c \
// RUN:            -Wno-openacc-omp-map-ompx-hold \
// RUN:            -Wno-openacc-omp-update-present \
// RUN:     | FileCheck -check-prefixes=%[prt-chk] -match-full-lines \
// RUN:                 -strict-whitespace %s
// RUN:   }
// RUN: }

// END.

// Check that cpp directives are not expanded/removed.
//      PRT:#include <stdio.h>
// PRT-NEXT:#define TWO 2
// PRT-NEXT:#if !ERRS
// PRT-NEXT:/* expected-no-diagnostics */
// PRT-NEXT:#endif
#include <stdio.h>
#define TWO 2
#if !ERRS
/* expected-no-diagnostics */
#endif

// PRT-NEXT:float f = 0;
// PRT-NEXT:int non_const_expr = 2;
// PRT-NEXT:int var, i;
// PRT-NEXT:const int *ptr = 0;
float f = 0;
int non_const_expr = 2;
int var, i;
const int *ptr = 0;

//--------------------------------------------------
// Directive only rewrite, not nested.
//--------------------------------------------------

// PRT-NEXT:void dirOnlyRewriteNotNested() {
void dirOnlyRewriteNotNested() {
  //  PRT-A-NEXT:  #pragma acc update device(i)
  // PRT-AO-NEXT:  // #pragma omp target update to(present: i)
  //  PRT-O-NEXT:  #pragma omp target update to(present: i)
  // PRT-OA-NEXT:  // #pragma acc update device(i)
  #pragma acc update device(i)

  //  PRT-A-NEXT:  #pragma acc parallel
  // PRT-AO-NEXT:  // #pragma omp target teams
  //  PRT-O-NEXT:  #pragma omp target teams
  // PRT-OA-NEXT:  // #pragma acc parallel
  //    PRT-NEXT:  ;
  #pragma acc parallel
  ;

  //  PRT-A-NEXT:  #pragma acc parallel loop gang
  // PRT-AO-NEXT:  // #pragma omp target teams
  // PRT-AO-NEXT:  // #pragma omp distribute
  //  PRT-O-NEXT:  #pragma omp target teams
  //  PRT-O-NEXT:  #pragma omp distribute
  // PRT-OA-NEXT:  // #pragma acc parallel loop gang
  #pragma acc parallel loop gang
    // PRT-NEXT:    for (int i = 0; i < 2; ++i)
    for (int i = 0; i < 2; ++i)
      // PRT-NEXT:      ;
      ;

  // Directive appears to end in macro expansion, but Clang doesn't record
  // #pragma end locations that way, so rewrite succeeds.

  //    PRT-NEXT:  #define MAC num_gangs(1)
  //  PRT-A-NEXT:  #pragma acc parallel MAC
  // PRT-AO-NEXT:  // #pragma omp target teams num_teams(1)
  //  PRT-O-NEXT:  #pragma omp target teams num_teams(1)
  // PRT-OA-NEXT:  // #pragma acc parallel MAC
  //    PRT-NEXT:  ;
  //    PRT-NEXT:  #undef MAC
  #define MAC num_gangs(1)
  #pragma acc parallel MAC
  ;
  #undef MAC

  // Associated statement end is in macro expansion, but only directive needs
  // to be rewritten, so rewrite succeeds.

  //    PRT-NEXT:  #define MAC ;
  //  PRT-A-NEXT:  #pragma acc parallel
  // PRT-AO-NEXT:  // #pragma omp target teams
  //  PRT-O-NEXT:  #pragma omp target teams
  // PRT-OA-NEXT:  // #pragma acc parallel
  //    PRT-NEXT:  MAC
  //    PRT-NEXT:  #undef MAC
  #define MAC ;
  #pragma acc parallel
  MAC
  #undef MAC
}// PRT-NEXT:}

//--------------------------------------------------
// Directive only rewrite, nested.
//--------------------------------------------------

// PRT-NEXT:void dirOnlyRewriteNested() {
void dirOnlyRewriteNested() {
  //  PRT-A-NEXT:  #pragma acc data copy(i)
  // PRT-AO-NEXT:  // #pragma omp target data map(ompx_hold,tofrom: i)
  //  PRT-A-NEXT:  {
  //  PRT-A-NEXT:    #pragma acc update device(i)
  // PRT-AO-NEXT:    // #pragma omp target update to(present: i)
  //  PRT-A-NEXT:  }
  //  PRT-O-NEXT:  #pragma omp target data map(ompx_hold,tofrom: i)
  // PRT-OA-NEXT:  // #pragma acc data copy(i)
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:    #pragma omp target update to(present: i)
  // PRT-OA-NEXT:    // #pragma acc update device(i)
  //  PRT-O-NEXT:  }
  #pragma acc data copy(i)
  {
    #pragma acc update device(i)
  }

  //  PRT-A-NEXT:  #pragma acc parallel
  // PRT-AO-NEXT:  // #pragma omp target teams
  //  PRT-O-NEXT:  #pragma omp target teams
  // PRT-OA-NEXT:  // #pragma acc parallel
  #pragma acc parallel
  //  PRT-A-NEXT:  #pragma acc loop gang
  // PRT-AO-NEXT:  // #pragma omp distribute
  //  PRT-O-NEXT:  #pragma omp distribute
  // PRT-OA-NEXT:  // #pragma acc loop gang
  #pragma acc loop gang
  // PRT-NEXT:  for (int i = 0; i < 5; ++i)
  for (int i = 0; i < 5; ++i)
    // PRT-NEXT:    ;
    ;

  //  PRT-A-NEXT:  #pragma acc parallel loop gang
  // PRT-AO-NEXT:  // #pragma omp target teams
  // PRT-AO-NEXT:  // #pragma omp distribute
  //  PRT-O-NEXT:  #pragma omp target teams
  //  PRT-O-NEXT:  #pragma omp distribute
  // PRT-OA-NEXT:  // #pragma acc parallel loop gang
  #pragma acc parallel loop gang
  // PRT-NEXT:  for (int i = 0; i < 5; ++i)
  for (int i = 0; i < 5; ++i)
    //  PRT-A-NEXT:    #pragma acc loop worker
    // PRT-AO-NEXT:    // #pragma omp parallel for
    //  PRT-O-NEXT:    #pragma omp parallel for
    // PRT-OA-NEXT:    // #pragma acc loop worker
    #pragma acc loop worker
    // PRT-NEXT:    for (int j = 0; j < 5; ++j)
    for (int j = 0; j < 5; ++j)
      // PRT-NEXT:      ;
      ;

  // The following case is special in that an OpenACC construct has an
  // associated statement that is a combined OpenACC construct.  The issue is
  // that, for the combined OpenACC construct, the indentation of the OpenMP
  // translations of the effective OpenACC directives is aligned when printing
  // via the OpenACC node but progressively indented when printing the OpenMP
  // nodes directly.  Thus, when Clang checks to see if the original associated
  // statement is identical to its OpenMP translatation in order to decide
  // whether it should print them separately for acc-omp or omp-acc mode, it
  // cannot simply print them both as OpenMP and compare the results or they
  // will look different due merely to indentation.

  //  PRT-A-NEXT:  #pragma acc data copy(i)
  // PRT-AO-NEXT:  // #pragma omp target data map(ompx_hold,tofrom: i)
  //  PRT-O-NEXT:  #pragma omp target data map(ompx_hold,tofrom: i)
  // PRT-OA-NEXT:  // #pragma acc data copy(i)
  #pragma acc data copy(i)
  //  PRT-A-NEXT:  #pragma acc parallel loop gang
  // PRT-AO-NEXT:  // #pragma omp target teams
  // PRT-AO-NEXT:  // #pragma omp distribute
  //  PRT-O-NEXT:  #pragma omp target teams
  //  PRT-O-NEXT:  #pragma omp distribute
  // PRT-OA-NEXT:  // #pragma acc parallel loop gang
  #pragma acc parallel loop gang
  // PRT-NEXT:  for (int i = 0; i < 5; ++i)
  for (int i = 0; i < 5; ++i)
    // PRT-NEXT:    ;
    ;
}// PRT-NEXT:}

//--------------------------------------------------
// Directive only rewrite, but directive discarded.
//--------------------------------------------------

// PRT-NEXT:void dirOnlyRewriteDirDiscard() {
void dirOnlyRewriteDirDiscard() {
  //  PRT-A-NEXT:  #pragma acc parallel
  // PRT-AO-NEXT:  // #pragma omp target teams
  //  PRT-O-NEXT:  #pragma omp target teams
  // PRT-OA-NEXT:  // #pragma acc parallel
  #pragma acc parallel
  // PRT-AA-NEXT:  #pragma acc loop seq
  // PRT-AO-NEXT:  #pragma acc loop seq // discarded in OpenMP translation
  // PRT-OA-NEXT:  // #pragma acc loop seq // discarded in OpenMP translation
  #pragma acc loop seq
  // PRT-NEXT:  for (int i = 0; i < 5; ++i)
  for (int i = 0; i < 5; ++i)
    // PRT-NEXT:    ;
    ;

  //  PRT-A-NEXT:  #pragma acc parallel loop seq
  // PRT-AO-NEXT:  // #pragma omp target teams
  //  PRT-O-NEXT:  #pragma omp target teams
  // PRT-OA-NEXT:  // #pragma acc parallel loop seq
  #pragma acc parallel loop seq
  // PRT-NEXT:  for (int i = 0; i < 5; ++i)
  for (int i = 0; i < 5; ++i)
    // PRT-AA-NEXT:    #pragma acc loop seq
    // PRT-AO-NEXT:    #pragma acc loop seq // discarded in OpenMP translation
    // PRT-OA-NEXT:    // #pragma acc loop seq // discarded in OpenMP translation
    #pragma acc loop seq
    // PRT-NEXT:    for (int j = 0; j < 5; ++j)
    for (int j = 0; j < 5; ++j)
      // PRT-NEXT:      ;
      ;
}// PRT-NEXT:}

//--------------------------------------------------
// Directive and associate rewrite, outer directive only.
//
// Check for corruption due to the way Clang records the end location of a
// statement.  RewriteOpenACC and the getConstructRange function it calls
// on ACCDirectiveStmt have to handle many cases.  Important issues include
// whether the end location of the outer ACCDirectiveStmt's associated
// statement is actually for its last token or the token before that, whether
// each of those tokens is represented by any descendant node, and whether
// each of those tokens is part of a macro expansion.
// fopenacc-print-messages.c covers macro expansion cases that cannot be
// handled and so produce error diagnostics.
//
// FIXME: In the cases of macro expansions within associated statements, the
// OpenMP version is fully expanded.  Eventually, we'd like to prevent that.
//--------------------------------------------------

// PRT-NEXT:void fullRewriteOuterDirOnly() {
void fullRewriteOuterDirOnly() {
  // Null statement: There are no descendants, and the end location is from the
  // final semicolon.

  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel num_workers(non_const_expr)
  //  PRT-A-NEXT:  #pragma acc loop worker
  //  PRT-A-NEXT:  for (int i = 0; i < 5; ++i)
  //  PRT-A-NEXT:    ;
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     const int __clang_acc_num_workers__ = non_const_expr;
  // PRT-AO-NEXT:  //     #pragma omp target teams
  // PRT-AO-NEXT:  //         #pragma omp distribute parallel for num_threads(__clang_acc_num_workers__)
  // PRT-AO-NEXT:  //             for (int i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //                 ;
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      const int __clang_acc_num_workers__ = non_const_expr;
  //  PRT-O-NEXT:      #pragma omp target teams
  //  PRT-O-NEXT:          #pragma omp distribute parallel for num_threads(__clang_acc_num_workers__)
  //  PRT-O-NEXT:              for (int i = 0; i < 5; ++i)
  //  PRT-O-NEXT:                  ;
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc parallel num_workers(non_const_expr)
  // PRT-OA-NEXT:  // #pragma acc loop worker
  // PRT-OA-NEXT:  // for (int i = 0; i < 5; ++i)
  // PRT-OA-NEXT:  //   ;
  // PRT-OA-NEXT:  // ^----------ACC----------^
  #pragma acc parallel num_workers(non_const_expr)
  #pragma acc loop worker
  for (int i = 0; i < 5; ++i)
    ;

  //    PRT-NEXT:  #define MAC for (i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel loop vector
  //  PRT-A-NEXT:  MAC
  //  PRT-A-NEXT:    ;
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // #pragma omp target teams
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     int i;
  // PRT-AO-NEXT:  //     #pragma omp distribute simd
  // PRT-AO-NEXT:  //         for (i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //             ;
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  #pragma omp target teams
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      int i;
  //  PRT-O-NEXT:      #pragma omp distribute simd
  //  PRT-O-NEXT:          for (i = 0; i < 5; ++i)
  //  PRT-O-NEXT:              ;
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc parallel loop vector
  // PRT-OA-NEXT:  // MAC
  // PRT-OA-NEXT:  //   ;
  // PRT-OA-NEXT:  // ^----------ACC----------^
  //    PRT-NEXT:  #undef MAC
  #define MAC for (i = 0; i < 5; ++i)
  #pragma acc parallel loop vector
  MAC
    ;
  #undef MAC

  // Compound statement: The end location is from the closing brace, which is
  // not a descendant.

  //    PRT-NEXT:  #define MAC var = non_const_expr;
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel num_workers(non_const_expr)
  //  PRT-A-NEXT:  #pragma acc loop worker
  //  PRT-A-NEXT:  for (int i = 0; i < 5; ++i) {
  //  PRT-A-NEXT:    var = 5;
  //  PRT-A-NEXT:    MAC}
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     const int __clang_acc_num_workers__ = non_const_expr;
  // PRT-AO-NEXT:  //     #pragma omp target teams firstprivate(var,non_const_expr)
  // PRT-AO-NEXT:  //         #pragma omp distribute parallel for num_threads(__clang_acc_num_workers__) shared(var,non_const_expr)
  // PRT-AO-NEXT:  //             for (int i = 0; i < 5; ++i) {
  // PRT-AO-NEXT:  //                 var = 5;
  // PRT-AO-NEXT:  //                 var = non_const_expr;
  // PRT-AO-NEXT:  //             }
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      const int __clang_acc_num_workers__ = non_const_expr;
  //  PRT-O-NEXT:      #pragma omp target teams firstprivate(var,non_const_expr)
  //  PRT-O-NEXT:          #pragma omp distribute parallel for num_threads(__clang_acc_num_workers__) shared(var,non_const_expr)
  //  PRT-O-NEXT:              for (int i = 0; i < 5; ++i) {
  //  PRT-O-NEXT:                  var = 5;
  //  PRT-O-NEXT:                  var = non_const_expr;
  //  PRT-O-NEXT:              }
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc parallel num_workers(non_const_expr)
  // PRT-OA-NEXT:  // #pragma acc loop worker
  // PRT-OA-NEXT:  // for (int i = 0; i < 5; ++i) {
  // PRT-OA-NEXT:  //   var = 5;
  // PRT-OA-NEXT:  //   MAC}
  // PRT-OA-NEXT:  // ^----------ACC----------^
  //    PRT-NEXT:  #undef MAC
  #define MAC var = non_const_expr;
  #pragma acc parallel num_workers(non_const_expr)
  #pragma acc loop worker
  for (int i = 0; i < 5; ++i) {
    var = 5;
    MAC}
  #undef MAC

  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel loop vector
  //  PRT-A-NEXT:  for (i = 0; i < 5; ++i) {
  //  PRT-A-NEXT:    var = 5;
  //  PRT-A-NEXT:    var = non_const_expr;
  //  PRT-A-NEXT:  }
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // #pragma omp target teams firstprivate(var,non_const_expr)
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     int i;
  // PRT-AO-NEXT:  //     #pragma omp distribute simd
  // PRT-AO-NEXT:  //         for (i = 0; i < 5; ++i) {
  // PRT-AO-NEXT:  //             var = 5;
  // PRT-AO-NEXT:  //             var = non_const_expr;
  // PRT-AO-NEXT:  //         }
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  #pragma omp target teams firstprivate(var,non_const_expr)
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      int i;
  //  PRT-O-NEXT:      #pragma omp distribute simd
  //  PRT-O-NEXT:          for (i = 0; i < 5; ++i) {
  //  PRT-O-NEXT:              var = 5;
  //  PRT-O-NEXT:              var = non_const_expr;
  //  PRT-O-NEXT:          }
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc parallel loop vector
  // PRT-OA-NEXT:  // for (i = 0; i < 5; ++i) {
  // PRT-OA-NEXT:  //   var = 5;
  // PRT-OA-NEXT:  //   var = non_const_expr;
  // PRT-OA-NEXT:  // }
  // PRT-OA-NEXT:  // ^----------ACC----------^
  #pragma acc parallel loop vector
  for (i = 0; i < 5; ++i) {
    var = 5;
    var = non_const_expr;
  }

  // Expression statement: The end location is from the token before the
  // semicolon, and only the preceding token might be a descendant.

  // Preceding token is a descendant.
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel num_workers(non_const_expr)
  //  PRT-A-NEXT:  #pragma acc loop worker
  //  PRT-A-NEXT:  for (int i = 0; i < 5; ++i)
  //  PRT-A-NEXT:    var = non_const_expr;
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     const int __clang_acc_num_workers__ = non_const_expr;
  // PRT-AO-NEXT:  //     #pragma omp target teams firstprivate(var,non_const_expr)
  // PRT-AO-NEXT:  //         #pragma omp distribute parallel for num_threads(__clang_acc_num_workers__) shared(var,non_const_expr)
  // PRT-AO-NEXT:  //             for (int i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //                 var = non_const_expr;
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      const int __clang_acc_num_workers__ = non_const_expr;
  //  PRT-O-NEXT:      #pragma omp target teams firstprivate(var,non_const_expr)
  //  PRT-O-NEXT:          #pragma omp distribute parallel for num_threads(__clang_acc_num_workers__) shared(var,non_const_expr)
  //  PRT-O-NEXT:              for (int i = 0; i < 5; ++i)
  //  PRT-O-NEXT:                  var = non_const_expr;
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc parallel num_workers(non_const_expr)
  // PRT-OA-NEXT:  // #pragma acc loop worker
  // PRT-OA-NEXT:  // for (int i = 0; i < 5; ++i)
  // PRT-OA-NEXT:  //   var = non_const_expr;
  // PRT-OA-NEXT:  // ^----------ACC----------^
  #pragma acc parallel num_workers(non_const_expr)
  #pragma acc loop worker
  for (int i = 0; i < 5; ++i)
    var = non_const_expr;

  // Preceding token is a descendant and is expanded from a macro.
  //    PRT-NEXT:  #define MAC non_const_expr
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel loop vector
  //  PRT-A-NEXT:  for (i = 0; i < 5; ++i)
  //  PRT-A-NEXT:    var = MAC ;
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // #pragma omp target teams firstprivate(var,non_const_expr)
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     int i;
  // PRT-AO-NEXT:  //     #pragma omp distribute simd
  // PRT-AO-NEXT:  //         for (i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //             var = non_const_expr;
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  #pragma omp target teams firstprivate(var,non_const_expr)
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      int i;
  //  PRT-O-NEXT:      #pragma omp distribute simd
  //  PRT-O-NEXT:          for (i = 0; i < 5; ++i)
  //  PRT-O-NEXT:              var = non_const_expr;
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc parallel loop vector
  // PRT-OA-NEXT:  // for (i = 0; i < 5; ++i)
  // PRT-OA-NEXT:  //   var = MAC ;
  // PRT-OA-NEXT:  // ^----------ACC----------^
  //    PRT-NEXT:  #undef MAC
  #define MAC non_const_expr
  #pragma acc parallel loop vector
  for (i = 0; i < 5; ++i)
    var = MAC ;
  #undef MAC

  // Preceding token is closing parenthesis in ParenExpr.
  //    PRT-NEXT:  #define MAC (non_const_expr)
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel num_workers(non_const_expr)
  //  PRT-A-NEXT:  #pragma acc loop worker
  //  PRT-A-NEXT:  for (int i = 0; i < 5; ++i)
  //  PRT-A-NEXT:    var = MAC ;
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     const int __clang_acc_num_workers__ = non_const_expr;
  // PRT-AO-NEXT:  //     #pragma omp target teams firstprivate(var,non_const_expr)
  // PRT-AO-NEXT:  //         #pragma omp distribute parallel for num_threads(__clang_acc_num_workers__) shared(var,non_const_expr)
  // PRT-AO-NEXT:  //             for (int i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //                 var = (non_const_expr);
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      const int __clang_acc_num_workers__ = non_const_expr;
  //  PRT-O-NEXT:      #pragma omp target teams firstprivate(var,non_const_expr)
  //  PRT-O-NEXT:          #pragma omp distribute parallel for num_threads(__clang_acc_num_workers__) shared(var,non_const_expr)
  //  PRT-O-NEXT:              for (int i = 0; i < 5; ++i)
  //  PRT-O-NEXT:                  var = (non_const_expr);
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc parallel num_workers(non_const_expr)
  // PRT-OA-NEXT:  // #pragma acc loop worker
  // PRT-OA-NEXT:  // for (int i = 0; i < 5; ++i)
  // PRT-OA-NEXT:  //   var = MAC ;
  // PRT-OA-NEXT:  // ^----------ACC----------^
  //    PRT-NEXT:  #undef MAC
  #define MAC (non_const_expr)
  #pragma acc parallel num_workers(non_const_expr)
  #pragma acc loop worker
  for (int i = 0; i < 5; ++i)
    var = MAC ;
  #undef MAC

  // Preceding token is closing parenthesis in CallExpr.
  //    PRT-NEXT:  #define MAC printf
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel loop vector
  //  PRT-A-NEXT:  for (i = 0; i < 5; ++i)
  //  PRT-A-NEXT:    MAC("hello world\n");
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // #pragma omp target teams
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     int i;
  // PRT-AO-NEXT:  //     #pragma omp distribute simd
  // PRT-AO-NEXT:  //         for (i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //             printf("hello world\n");
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  #pragma omp target teams
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      int i;
  //  PRT-O-NEXT:      #pragma omp distribute simd
  //  PRT-O-NEXT:          for (i = 0; i < 5; ++i)
  //  PRT-O-NEXT:              printf("hello world\n");
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc parallel loop vector
  // PRT-OA-NEXT:  // for (i = 0; i < 5; ++i)
  // PRT-OA-NEXT:  //   MAC("hello world\n");
  // PRT-OA-NEXT:  // ^----------ACC----------^
  //    PRT-NEXT:  #undef MAC
  #define MAC printf
  #pragma acc parallel loop vector
  for (i = 0; i < 5; ++i)
    MAC("hello world\n");
  #undef MAC

  // Preceding token is closing parenthesis in GenericSelectionExpr.
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel loop vector
  //  PRT-A-NEXT:  for (i = 0; i < 5; ++i)
  //  PRT-A-NEXT:    var = _Generic(var, int : 0, default : 1);
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // #pragma omp target teams firstprivate(var)
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     int i;
  // PRT-AO-NEXT:  //     #pragma omp distribute simd
  // PRT-AO-NEXT:  //         for (i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //             var = _Generic(var, int: 0, default: 1);
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  #pragma omp target teams firstprivate(var)
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      int i;
  //  PRT-O-NEXT:      #pragma omp distribute simd
  //  PRT-O-NEXT:          for (i = 0; i < 5; ++i)
  //  PRT-O-NEXT:              var = _Generic(var, int: 0, default: 1);
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc parallel loop vector
  // PRT-OA-NEXT:  // for (i = 0; i < 5; ++i)
  // PRT-OA-NEXT:  //   var = _Generic(var, int : 0, default : 1);
  // PRT-OA-NEXT:  // ^----------ACC----------^
  #pragma acc parallel loop vector
  for (i = 0; i < 5; ++i)
    var = _Generic(var, int : 0, default : 1);

  // Preceding token is closing brace in InitListExpr.
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel loop vector
  //  PRT-A-NEXT:  for (i = 0; i < 5; ++i)
  //  PRT-A-NEXT:    ptr = (int[]){0,1};
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // #pragma omp target teams firstprivate(ptr)
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     int i;
  // PRT-AO-NEXT:  //     #pragma omp distribute simd
  // PRT-AO-NEXT:  //         for (i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //             ptr = (int[2]){0, 1};
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  #pragma omp target teams firstprivate(ptr)
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      int i;
  //  PRT-O-NEXT:      #pragma omp distribute simd
  //  PRT-O-NEXT:          for (i = 0; i < 5; ++i)
  //  PRT-O-NEXT:              ptr = (int[2]){0, 1};
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc parallel loop vector
  // PRT-OA-NEXT:  // for (i = 0; i < 5; ++i)
  // PRT-OA-NEXT:  //   ptr = (int[]){0,1};
  // PRT-OA-NEXT:  // ^----------ACC----------^
  #pragma acc parallel loop vector
  for (i = 0; i < 5; ++i)
    ptr = (int[]){0,1};

  // Preceding token is closing bracket in ArraySubscriptExpr.
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel loop vector
  //  PRT-A-NEXT:  for (i = 0; i < 5; ++i)
  //  PRT-A-NEXT:    var = ptr[0];
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // #pragma omp target teams firstprivate(var,ptr)
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     int i;
  // PRT-AO-NEXT:  //     #pragma omp distribute simd
  // PRT-AO-NEXT:  //         for (i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //             var = ptr[0];
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  #pragma omp target teams firstprivate(var,ptr)
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      int i;
  //  PRT-O-NEXT:      #pragma omp distribute simd
  //  PRT-O-NEXT:          for (i = 0; i < 5; ++i)
  //  PRT-O-NEXT:              var = ptr[0];
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc parallel loop vector
  // PRT-OA-NEXT:  // for (i = 0; i < 5; ++i)
  // PRT-OA-NEXT:  //   var = ptr[0];
  // PRT-OA-NEXT:  // ^----------ACC----------^
  #pragma acc parallel loop vector
  for (i = 0; i < 5; ++i)
    var = ptr[0];

  // Preceding token is closing parenthesis in UnaryExprOrTypeTraitExprClass
  // for sizeof(type), which has no descendant.
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel loop vector
  //  PRT-A-NEXT:  for (i = 0; i < 5; ++i)
  //  PRT-A-NEXT:    var = sizeof(int);
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // #pragma omp target teams firstprivate(var)
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     int i;
  // PRT-AO-NEXT:  //     #pragma omp distribute simd
  // PRT-AO-NEXT:  //         for (i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //             var = sizeof(int);
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  #pragma omp target teams firstprivate(var)
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      int i;
  //  PRT-O-NEXT:      #pragma omp distribute simd
  //  PRT-O-NEXT:          for (i = 0; i < 5; ++i)
  //  PRT-O-NEXT:              var = sizeof(int);
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc parallel loop vector
  // PRT-OA-NEXT:  // for (i = 0; i < 5; ++i)
  // PRT-OA-NEXT:  //   var = sizeof(int);
  // PRT-OA-NEXT:  // ^----------ACC----------^
  #pragma acc parallel loop vector
  for (i = 0; i < 5; ++i)
    var = sizeof(int);

  // Preceding token is closing parenthesis in UnaryExprOrTypeTraitExprClass
  // for _Alignof(type), which has no descendant.
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel loop vector
  //  PRT-A-NEXT:  for (i = 0; i < 5; ++i)
  //  PRT-A-NEXT:    var = _Alignof(int);
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // #pragma omp target teams firstprivate(var)
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     int i;
  // PRT-AO-NEXT:  //     #pragma omp distribute simd
  // PRT-AO-NEXT:  //         for (i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //             var = _Alignof(int);
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  #pragma omp target teams firstprivate(var)
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      int i;
  //  PRT-O-NEXT:      #pragma omp distribute simd
  //  PRT-O-NEXT:          for (i = 0; i < 5; ++i)
  //  PRT-O-NEXT:              var = _Alignof(int);
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc parallel loop vector
  // PRT-OA-NEXT:  // for (i = 0; i < 5; ++i)
  // PRT-OA-NEXT:  //   var = _Alignof(int);
  // PRT-OA-NEXT:  // ^----------ACC----------^
  #pragma acc parallel loop vector
  for (i = 0; i < 5; ++i)
    var = _Alignof(int);

  // Preceding token is closing parenthesis in UnaryExprOrTypeTraitExprClass
  // for sizeof(variable-array-type), which has a descendant.
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel loop vector
  //  PRT-A-NEXT:  for (i = 0; i < 5; ++i)
  //  PRT-A-NEXT:    var = sizeof(int[i]);
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // #pragma omp target teams firstprivate(var)
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     int i;
  // PRT-AO-NEXT:  //     #pragma omp distribute simd
  // PRT-AO-NEXT:  //         for (i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //             var = sizeof(int[i]);
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  #pragma omp target teams firstprivate(var)
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      int i;
  //  PRT-O-NEXT:      #pragma omp distribute simd
  //  PRT-O-NEXT:          for (i = 0; i < 5; ++i)
  //  PRT-O-NEXT:              var = sizeof(int[i]);
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc parallel loop vector
  // PRT-OA-NEXT:  // for (i = 0; i < 5; ++i)
  // PRT-OA-NEXT:  //   var = sizeof(int[i]);
  // PRT-OA-NEXT:  // ^----------ACC----------^
  #pragma acc parallel loop vector
  for (i = 0; i < 5; ++i)
    var = sizeof(int[i]);

  // Preceding token is closing parenthesis in UnaryExprOrTypeTraitExprClass
  // for _Alignof(variable-array-type), which has a descendant.
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel loop vector
  //  PRT-A-NEXT:  for (i = 0; i < 5; ++i)
  //  PRT-A-NEXT:    var = _Alignof(int[i]);
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // #pragma omp target teams firstprivate(var)
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     int i;
  // PRT-AO-NEXT:  //     #pragma omp distribute simd
  // PRT-AO-NEXT:  //         for (i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //             var = _Alignof(int[i]);
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  #pragma omp target teams firstprivate(var)
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      int i;
  //  PRT-O-NEXT:      #pragma omp distribute simd
  //  PRT-O-NEXT:          for (i = 0; i < 5; ++i)
  //  PRT-O-NEXT:              var = _Alignof(int[i]);
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc parallel loop vector
  // PRT-OA-NEXT:  // for (i = 0; i < 5; ++i)
  // PRT-OA-NEXT:  //   var = _Alignof(int[i]);
  // PRT-OA-NEXT:  // ^----------ACC----------^
  #pragma acc parallel loop vector
  for (i = 0; i < 5; ++i)
    var = _Alignof(int[i]);

  // Preceding token is not a closing parenthesis in
  // UnaryExprOrTypeTraitExprClass for sizeof expr, which has a descendant.
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel loop vector
  //  PRT-A-NEXT:  for (i = 0; i < 5; ++i)
  //  PRT-A-NEXT:    var = sizeof i;
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // #pragma omp target teams firstprivate(var)
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     int i;
  // PRT-AO-NEXT:  //     #pragma omp distribute simd
  // PRT-AO-NEXT:  //         for (i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //             var = sizeof i;
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  #pragma omp target teams firstprivate(var)
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      int i;
  //  PRT-O-NEXT:      #pragma omp distribute simd
  //  PRT-O-NEXT:          for (i = 0; i < 5; ++i)
  //  PRT-O-NEXT:              var = sizeof i;
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc parallel loop vector
  // PRT-OA-NEXT:  // for (i = 0; i < 5; ++i)
  // PRT-OA-NEXT:  //   var = sizeof i;
  // PRT-OA-NEXT:  // ^----------ACC----------^
  #pragma acc parallel loop vector
  for (i = 0; i < 5; ++i)
    var = sizeof i;

  // Preceding token is closing parenthesis in UnaryExprOrTypeTraitExprClass
  // for sizeof(expr), which has a ParenExpr descendant.
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel loop vector
  //  PRT-A-NEXT:  for (i = 0; i < 5; ++i)
  //  PRT-A-NEXT:    var = sizeof(i);
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // #pragma omp target teams firstprivate(var)
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     int i;
  // PRT-AO-NEXT:  //     #pragma omp distribute simd
  // PRT-AO-NEXT:  //         for (i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //             var = sizeof (i);
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  #pragma omp target teams firstprivate(var)
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      int i;
  //  PRT-O-NEXT:      #pragma omp distribute simd
  //  PRT-O-NEXT:          for (i = 0; i < 5; ++i)
  //  PRT-O-NEXT:              var = sizeof (i);
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc parallel loop vector
  // PRT-OA-NEXT:  // for (i = 0; i < 5; ++i)
  // PRT-OA-NEXT:  //   var = sizeof(i);
  // PRT-OA-NEXT:  // ^----------ACC----------^
  #pragma acc parallel loop vector
  for (i = 0; i < 5; ++i)
    var = sizeof(i);

}// PRT-NEXT:}

//  PRT-A-NEXT:#pragma acc routine seq
// PRT-AO-NEXT:// #pragma omp declare target
//  PRT-O-NEXT:#pragma omp declare target
// PRT-OA-NEXT:// #pragma acc routine seq
//    PRT-NEXT:void fullRewriteOuterDirOnly_routineProtoNoMacro();
//  PRT-O-NEXT:#pragma omp end declare target
// PRT-AO-NEXT:// #pragma omp end declare target
#pragma acc routine seq
void fullRewriteOuterDirOnly_routineProtoNoMacro();

//  PRT-A-NEXT:#pragma acc routine seq
// PRT-AO-NEXT:// #pragma omp declare target
//  PRT-O-NEXT:#pragma omp declare target
// PRT-OA-NEXT:// #pragma acc routine seq
//    PRT-NEXT:void fullRewriteOuterDirOnly_routineDefNoMacro() {}
//  PRT-O-NEXT:#pragma omp end declare target
// PRT-AO-NEXT:// #pragma omp end declare target
#pragma acc routine seq
void fullRewriteOuterDirOnly_routineDefNoMacro() {}

//    PRT-NEXT:#define MAC1 seq
//    PRT-NEXT:#define MAC2 void fullRewriteOuterDirOnly_routineProtoMacros()
//  PRT-A-NEXT:#pragma acc routine MAC1
// PRT-AO-NEXT:// #pragma omp declare target
//  PRT-O-NEXT:#pragma omp declare target
// PRT-OA-NEXT:// #pragma acc routine MAC1
//    PRT-NEXT:MAC2;
//  PRT-O-NEXT:#pragma omp end declare target
// PRT-AO-NEXT:// #pragma omp end declare target
//    PRT-NEXT:#undef MAC1
//    PRT-NEXT:#undef MAC2
#define MAC1 seq
#define MAC2 void fullRewriteOuterDirOnly_routineProtoMacros()
#pragma acc routine MAC1
MAC2;
#undef MAC1
#undef MAC2

//    PRT-NEXT:#define MAC1 routine seq
//    PRT-NEXT:#define MAC2 void fullRewriteOuterDirOnly_routineDefMacros() {
//  PRT-A-NEXT:#pragma acc MAC1
// PRT-AO-NEXT:// #pragma omp declare target
//  PRT-O-NEXT:#pragma omp declare target
// PRT-OA-NEXT:// #pragma acc MAC1
//    PRT-NEXT:MAC2}
//  PRT-O-NEXT:#pragma omp end declare target
// PRT-AO-NEXT:// #pragma omp end declare target
//    PRT-NEXT:#undef MAC1
//    PRT-NEXT:#undef MAC2
#define MAC1 routine seq
#define MAC2 void fullRewriteOuterDirOnly_routineDefMacros() {
#pragma acc MAC1
MAC2}
#undef MAC1
#undef MAC2

//--------------------------------------------------
// Directive and associate rewrite, inner directive only.
//--------------------------------------------------

// PRT-NEXT:void fullRewriteInnerDirOnly() {
void fullRewriteInnerDirOnly() {
  // Null statement: There are no descendants, and the end location is from the
  // final semicolon.

  //    PRT-NEXT:  #define MAC (i = 0; i < 5; ++i)
  //  PRT-A-NEXT:  #pragma acc parallel
  // PRT-AO-NEXT:  // #pragma omp target teams
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc loop vector
  //  PRT-A-NEXT:  for MAC
  //  PRT-A-NEXT:    ;
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     int i;
  // PRT-AO-NEXT:  //     #pragma omp distribute simd
  // PRT-AO-NEXT:  //         for (i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //             ;
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  //  PRT-O-NEXT:  #pragma omp target teams
  // PRT-OA-NEXT:  // #pragma acc parallel
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      int i;
  //  PRT-O-NEXT:      #pragma omp distribute simd
  //  PRT-O-NEXT:          for (i = 0; i < 5; ++i)
  //  PRT-O-NEXT:              ;
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc loop vector
  // PRT-OA-NEXT:  // for MAC
  // PRT-OA-NEXT:  //   ;
  // PRT-OA-NEXT:  // ^----------ACC----------^
  //    PRT-NEXT:  #undef MAC
  #define MAC (i = 0; i < 5; ++i)
  #pragma acc parallel
  #pragma acc loop vector
  for MAC
    ;
  #undef MAC

  //  PRT-A-NEXT:  #pragma acc parallel loop worker
  // PRT-AO-NEXT:  // #pragma omp target teams
  // PRT-AO-NEXT:  // #pragma omp distribute parallel for
  //  PRT-A-NEXT:  for (int j = 0; j < 5; ++j) {
  // PRT-AO-NEXT:    // v----------ACC----------v
  //  PRT-A-NEXT:    #pragma acc loop vector
  //  PRT-A-NEXT:    for (i = 0; i < 5; ++i)
  //  PRT-A-NEXT:      ;
  // PRT-AO-NEXT:    // ---------ACC->OMP--------
  // PRT-AO-NEXT:    // {
  // PRT-AO-NEXT:    //     int i;
  // PRT-AO-NEXT:    //     #pragma omp simd
  // PRT-AO-NEXT:    //         for (i = 0; i < 5; ++i)
  // PRT-AO-NEXT:    //             ;
  // PRT-AO-NEXT:    // }
  // PRT-AO-NEXT:    // ^----------OMP----------^
  //  PRT-A-NEXT:  }
  //
  //  PRT-O-NEXT:  #pragma omp target teams
  //  PRT-O-NEXT:  #pragma omp distribute parallel for
  // PRT-OA-NEXT:  // #pragma acc parallel loop worker
  //  PRT-O-NEXT:  for (int j = 0; j < 5; ++j) {
  // PRT-OA-NEXT:    // v----------OMP----------v
  //  PRT-O-NEXT:    {
  //  PRT-O-NEXT:        int i;
  //  PRT-O-NEXT:        #pragma omp simd
  //  PRT-O-NEXT:            for (i = 0; i < 5; ++i)
  //  PRT-O-NEXT:                ;
  //  PRT-O-NEXT:    }
  // PRT-OA-NEXT:    // ---------OMP<-ACC--------
  // PRT-OA-NEXT:    // #pragma acc loop vector
  // PRT-OA-NEXT:    // for (i = 0; i < 5; ++i)
  // PRT-OA-NEXT:    //   ;
  // PRT-OA-NEXT:    // ^----------ACC----------^
  //  PRT-O-NEXT:  }
  #pragma acc parallel loop worker
  for (int j = 0; j < 5; ++j) {
    #pragma acc loop vector
    for (i = 0; i < 5; ++i)
      ;
  }

  // Compound statement: The end location is from the closing brace, which is
  // not a descendant.

  //    PRT-NEXT:  #define MAC var = non_const_expr;
  //  PRT-A-NEXT:  #pragma acc parallel
  // PRT-AO-NEXT:  // #pragma omp target teams firstprivate(var,non_const_expr)
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc loop vector
  //  PRT-A-NEXT:  for (i = 0; i < 5; ++i) {
  //  PRT-A-NEXT:    var = 5;
  //  PRT-A-NEXT:    MAC
  //  PRT-A-NEXT:  }
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     int i;
  // PRT-AO-NEXT:  //     #pragma omp distribute simd
  // PRT-AO-NEXT:  //         for (i = 0; i < 5; ++i) {
  // PRT-AO-NEXT:  //             var = 5;
  // PRT-AO-NEXT:  //             var = non_const_expr;
  // PRT-AO-NEXT:  //         }
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  //  PRT-O-NEXT:  #pragma omp target teams firstprivate(var,non_const_expr)
  // PRT-OA-NEXT:  // #pragma acc parallel
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      int i;
  //  PRT-O-NEXT:      #pragma omp distribute simd
  //  PRT-O-NEXT:          for (i = 0; i < 5; ++i) {
  //  PRT-O-NEXT:              var = 5;
  //  PRT-O-NEXT:              var = non_const_expr;
  //  PRT-O-NEXT:          }
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc loop vector
  // PRT-OA-NEXT:  // for (i = 0; i < 5; ++i) {
  // PRT-OA-NEXT:  //   var = 5;
  // PRT-OA-NEXT:  //   MAC
  // PRT-OA-NEXT:  // }
  // PRT-OA-NEXT:  // ^----------ACC----------^
  //    PRT-NEXT:  #undef MAC
  #define MAC var = non_const_expr;
  #pragma acc parallel
  #pragma acc loop vector
  for (i = 0; i < 5; ++i) {
    var = 5;
    MAC
  }
  #undef MAC

  //  PRT-A-NEXT:  #pragma acc parallel loop worker
  // PRT-AO-NEXT:  // #pragma omp target teams firstprivate(var,non_const_expr)
  // PRT-AO-NEXT:  // #pragma omp distribute parallel for shared(var,non_const_expr)
  //  PRT-A-NEXT:  for (int j = 0; j < 5; ++j) {
  // PRT-AO-NEXT:    // v----------ACC----------v
  //  PRT-A-NEXT:    #pragma acc loop vector
  //  PRT-A-NEXT:    for (i = 0; i < 5; ++i) {
  //  PRT-A-NEXT:      var = 5;
  //  PRT-A-NEXT:      var = non_const_expr;
  //  PRT-A-NEXT:    }
  // PRT-AO-NEXT:    // ---------ACC->OMP--------
  // PRT-AO-NEXT:    // {
  // PRT-AO-NEXT:    //     int i;
  // PRT-AO-NEXT:    //     #pragma omp simd
  // PRT-AO-NEXT:    //         for (i = 0; i < 5; ++i) {
  // PRT-AO-NEXT:    //             var = 5;
  // PRT-AO-NEXT:    //             var = non_const_expr;
  // PRT-AO-NEXT:    //         }
  // PRT-AO-NEXT:    // }
  // PRT-AO-NEXT:    // ^----------OMP----------^
  //  PRT-A-NEXT:  }
  //
  //  PRT-O-NEXT:  #pragma omp target teams firstprivate(var,non_const_expr)
  //  PRT-O-NEXT:  #pragma omp distribute parallel for shared(var,non_const_expr)
  // PRT-OA-NEXT:  // #pragma acc parallel loop worker
  //  PRT-O-NEXT:  for (int j = 0; j < 5; ++j) {
  // PRT-OA-NEXT:    // v----------OMP----------v
  //  PRT-O-NEXT:    {
  //  PRT-O-NEXT:        int i;
  //  PRT-O-NEXT:        #pragma omp simd
  //  PRT-O-NEXT:            for (i = 0; i < 5; ++i) {
  //  PRT-O-NEXT:                var = 5;
  //  PRT-O-NEXT:                var = non_const_expr;
  //  PRT-O-NEXT:            }
  //  PRT-O-NEXT:    }
  // PRT-OA-NEXT:    // ---------OMP<-ACC--------
  // PRT-OA-NEXT:    // #pragma acc loop vector
  // PRT-OA-NEXT:    // for (i = 0; i < 5; ++i) {
  // PRT-OA-NEXT:    //   var = 5;
  // PRT-OA-NEXT:    //   var = non_const_expr;
  // PRT-OA-NEXT:    // }
  // PRT-OA-NEXT:    // ^----------ACC----------^
  //  PRT-O-NEXT:  }
  #pragma acc parallel loop worker
  for (int j = 0; j < 5; ++j) {
    #pragma acc loop vector
    for (i = 0; i < 5; ++i) {
      var = 5;
      var = non_const_expr;
    }
  }

  // Expression statement: The end location is from the token before the
  // semicolon, and only the preceding token might be a descendant.

  // Preceding token is a descendant.
  //  PRT-A-NEXT:  #pragma acc parallel loop worker
  // PRT-AO-NEXT:  // #pragma omp target teams firstprivate(var,non_const_expr)
  // PRT-AO-NEXT:  // #pragma omp distribute parallel for shared(var,non_const_expr)
  //  PRT-A-NEXT:  for (int j = 0; j < 5; ++j) {
  // PRT-AO-NEXT:    // v----------ACC----------v
  //  PRT-A-NEXT:    #pragma acc loop vector
  //  PRT-A-NEXT:    for (i = 0; i < 5; ++i)
  //  PRT-A-NEXT:      var = non_const_expr;
  // PRT-AO-NEXT:    // ---------ACC->OMP--------
  // PRT-AO-NEXT:    // {
  // PRT-AO-NEXT:    //     int i;
  // PRT-AO-NEXT:    //     #pragma omp simd
  // PRT-AO-NEXT:    //         for (i = 0; i < 5; ++i)
  // PRT-AO-NEXT:    //             var = non_const_expr;
  // PRT-AO-NEXT:    // }
  // PRT-AO-NEXT:    // ^----------OMP----------^
  //  PRT-A-NEXT:  }
  //
  //  PRT-O-NEXT:  #pragma omp target teams firstprivate(var,non_const_expr)
  //  PRT-O-NEXT:  #pragma omp distribute parallel for shared(var,non_const_expr)
  // PRT-OA-NEXT:  // #pragma acc parallel loop worker
  //  PRT-O-NEXT:  for (int j = 0; j < 5; ++j) {
  // PRT-OA-NEXT:    // v----------OMP----------v
  //  PRT-O-NEXT:    {
  //  PRT-O-NEXT:        int i;
  //  PRT-O-NEXT:        #pragma omp simd
  //  PRT-O-NEXT:            for (i = 0; i < 5; ++i)
  //  PRT-O-NEXT:                var = non_const_expr;
  //  PRT-O-NEXT:    }
  // PRT-OA-NEXT:    // ---------OMP<-ACC--------
  // PRT-OA-NEXT:    // #pragma acc loop vector
  // PRT-OA-NEXT:    // for (i = 0; i < 5; ++i)
  // PRT-OA-NEXT:    //   var = non_const_expr;
  // PRT-OA-NEXT:    // ^----------ACC----------^
  //  PRT-O-NEXT:  }
  #pragma acc parallel loop worker
  for (int j = 0; j < 5; ++j) {
    #pragma acc loop vector
    for (i = 0; i < 5; ++i)
      var = non_const_expr;
  }

  // Preceding token is a descendant and is expanded from a macro.
  //    PRT-NEXT:  #define MAC for (i = 0; i < 5; ++i) \
  //    PRT-NEXT:                var = non_const_expr
  //  PRT-A-NEXT:  #pragma acc parallel
  // PRT-AO-NEXT:  // #pragma omp target teams firstprivate(var,non_const_expr)
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc loop vector
  //  PRT-A-NEXT:  MAC;
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     int i;
  // PRT-AO-NEXT:  //     #pragma omp distribute simd
  // PRT-AO-NEXT:  //         for (i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //             var = non_const_expr;
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  //  PRT-O-NEXT:  #pragma omp target teams firstprivate(var,non_const_expr)
  // PRT-OA-NEXT:  // #pragma acc parallel
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      int i;
  //  PRT-O-NEXT:      #pragma omp distribute simd
  //  PRT-O-NEXT:          for (i = 0; i < 5; ++i)
  //  PRT-O-NEXT:              var = non_const_expr;
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc loop vector
  // PRT-OA-NEXT:  // MAC;
  // PRT-OA-NEXT:  // ^----------ACC----------^
  //    PRT-NEXT:  #undef MAC
  #define MAC for (i = 0; i < 5; ++i) \
                var = non_const_expr
  #pragma acc parallel
  #pragma acc loop vector
  MAC;
  #undef MAC

  // Preceding token is closing parenthesis in ParenExpr and is expanded from
  // a macro.
  //    PRT-NEXT:  #define MAC non_const_expr)
  //  PRT-A-NEXT:  #pragma acc parallel
  // PRT-AO-NEXT:  // #pragma omp target teams firstprivate(var,non_const_expr)
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc loop vector
  //  PRT-A-NEXT:  for (i = 0; i < 5; ++i)
  //  PRT-A-NEXT:    var = (MAC;
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     int i;
  // PRT-AO-NEXT:  //     #pragma omp distribute simd
  // PRT-AO-NEXT:  //         for (i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //             var = (non_const_expr);
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  //  PRT-O-NEXT:  #pragma omp target teams firstprivate(var,non_const_expr)
  // PRT-OA-NEXT:  // #pragma acc parallel
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      int i;
  //  PRT-O-NEXT:      #pragma omp distribute simd
  //  PRT-O-NEXT:          for (i = 0; i < 5; ++i)
  //  PRT-O-NEXT:              var = (non_const_expr);
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc loop vector
  // PRT-OA-NEXT:  // for (i = 0; i < 5; ++i)
  // PRT-OA-NEXT:  //   var = (MAC;
  // PRT-OA-NEXT:  // ^----------ACC----------^
  //    PRT-NEXT:  #undef MAC
  #define MAC non_const_expr)
  #pragma acc parallel
  #pragma acc loop vector
  for (i = 0; i < 5; ++i)
    var = (MAC;
  #undef MAC

  // Preceding token is closing parenthesis in CallExpr.
  //  PRT-A-NEXT:  #pragma acc parallel loop gang
  // PRT-AO-NEXT:  // #pragma omp target teams
  // PRT-AO-NEXT:  // #pragma omp distribute
  //  PRT-A-NEXT:  for (int j = 0; j < 5; ++j) {
  // PRT-AO-NEXT:    // v----------ACC----------v
  //  PRT-A-NEXT:    #pragma acc loop vector
  //  PRT-A-NEXT:    for (i = 0; i < 5; ++i)
  //  PRT-A-NEXT:      printf("hello world\n");
  // PRT-AO-NEXT:    // ---------ACC->OMP--------
  // PRT-AO-NEXT:    // {
  // PRT-AO-NEXT:    //     int i;
  // PRT-AO-NEXT:    //     #pragma omp simd
  // PRT-AO-NEXT:    //         for (i = 0; i < 5; ++i)
  // PRT-AO-NEXT:    //             printf("hello world\n");
  // PRT-AO-NEXT:    // }
  // PRT-AO-NEXT:    // ^----------OMP----------^
  //  PRT-A-NEXT:  }
  //
  //  PRT-O-NEXT:  #pragma omp target teams
  //  PRT-O-NEXT:  #pragma omp distribute
  // PRT-OA-NEXT:  // #pragma acc parallel loop gang
  //  PRT-O-NEXT:  for (int j = 0; j < 5; ++j) {
  // PRT-OA-NEXT:    // v----------OMP----------v
  //  PRT-O-NEXT:    {
  //  PRT-O-NEXT:        int i;
  //  PRT-O-NEXT:        #pragma omp simd
  //  PRT-O-NEXT:            for (i = 0; i < 5; ++i)
  //  PRT-O-NEXT:                printf("hello world\n");
  //  PRT-O-NEXT:    }
  // PRT-OA-NEXT:    // ---------OMP<-ACC--------
  // PRT-OA-NEXT:    // #pragma acc loop vector
  // PRT-OA-NEXT:    // for (i = 0; i < 5; ++i)
  // PRT-OA-NEXT:    //   printf("hello world\n");
  // PRT-OA-NEXT:    // ^----------ACC----------^
  //  PRT-O-NEXT:  }
  #pragma acc parallel loop gang
  for (int j = 0; j < 5; ++j) {
    #pragma acc loop vector
    for (i = 0; i < 5; ++i)
      printf("hello world\n");
  }
}// PRT-NEXT:}

//--------------------------------------------------
// Directive and associate rewrite, outer and inner directive.
//--------------------------------------------------

// PRT-NEXT:void fullRewriteOuterAndInnerDir() {
void fullRewriteOuterAndInnerDir() {
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel num_workers(non_const_expr)
  //  PRT-A-NEXT:  #pragma acc loop worker vector
  //  PRT-A-NEXT:  for (i = 0; i < 5; ++i)
  //  PRT-A-NEXT:    ;
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     const int __clang_acc_num_workers__ = non_const_expr;
  // PRT-AO-NEXT:  //     #pragma omp target teams
  // PRT-AO-NEXT:  //         {
  // PRT-AO-NEXT:  //             int i;
  // PRT-AO-NEXT:  //             #pragma omp distribute parallel for simd num_threads(__clang_acc_num_workers__)
  // PRT-AO-NEXT:  //                 for (i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //                     ;
  // PRT-AO-NEXT:  //         }
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      const int __clang_acc_num_workers__ = non_const_expr;
  //  PRT-O-NEXT:      #pragma omp target teams
  //  PRT-O-NEXT:          {
  //  PRT-O-NEXT:              int i;
  //  PRT-O-NEXT:              #pragma omp distribute parallel for simd num_threads(__clang_acc_num_workers__)
  //  PRT-O-NEXT:                  for (i = 0; i < 5; ++i)
  //  PRT-O-NEXT:                      ;
  //  PRT-O-NEXT:          }
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc parallel num_workers(non_const_expr)
  // PRT-OA-NEXT:  // #pragma acc loop worker vector
  // PRT-OA-NEXT:  // for (i = 0; i < 5; ++i)
  // PRT-OA-NEXT:  //   ;
  // PRT-OA-NEXT:  // ^----------ACC----------^
  #pragma acc parallel num_workers(non_const_expr)
  #pragma acc loop worker vector
  for (i = 0; i < 5; ++i)
    ;

  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel loop worker num_workers(non_const_expr)
  //  PRT-A-NEXT:  for (int j = 0; j < 5; ++j)
  //  PRT-A-NEXT:    #pragma acc loop vector
  //  PRT-A-NEXT:    for (i = 0; i < 5; ++i)
  //  PRT-A-NEXT:      ;
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     const int __clang_acc_num_workers__ = non_const_expr;
  // PRT-AO-NEXT:  //     #pragma omp target teams
  // PRT-AO-NEXT:  //         #pragma omp distribute parallel for num_threads(__clang_acc_num_workers__)
  // PRT-AO-NEXT:  //             for (int j = 0; j < 5; ++j) {
  // PRT-AO-NEXT:  //                 int i;
  // PRT-AO-NEXT:  //                 #pragma omp simd
  // PRT-AO-NEXT:  //                     for (i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //                         ;
  // PRT-AO-NEXT:  //             }
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      const int __clang_acc_num_workers__ = non_const_expr;
  //  PRT-O-NEXT:      #pragma omp target teams
  //  PRT-O-NEXT:          #pragma omp distribute parallel for num_threads(__clang_acc_num_workers__)
  //  PRT-O-NEXT:              for (int j = 0; j < 5; ++j) {
  //  PRT-O-NEXT:                  int i;
  //  PRT-O-NEXT:                  #pragma omp simd
  //  PRT-O-NEXT:                      for (i = 0; i < 5; ++i)
  //  PRT-O-NEXT:                          ;
  //  PRT-O-NEXT:              }
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc parallel loop worker num_workers(non_const_expr)
  // PRT-OA-NEXT:  // for (int j = 0; j < 5; ++j)
  // PRT-OA-NEXT:  //   #pragma acc loop vector
  // PRT-OA-NEXT:  //   for (i = 0; i < 5; ++i)
  // PRT-OA-NEXT:  //     ;
  // PRT-OA-NEXT:  // ^----------ACC----------^
  #pragma acc parallel loop worker num_workers(non_const_expr)
  for (int j = 0; j < 5; ++j)
    #pragma acc loop vector
    for (i = 0; i < 5; ++i)
      ;

  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel loop worker vector num_workers(non_const_expr)
  //  PRT-A-NEXT:  for (i = 0; i < 5; ++i)
  //  PRT-A-NEXT:    #pragma acc loop seq private(var)
  //  PRT-A-NEXT:    for (var = 0; var < 5; ++var)
  //  PRT-A-NEXT:      ;
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     const int __clang_acc_num_workers__ = non_const_expr;
  // PRT-AO-NEXT:  //     #pragma omp target teams
  // PRT-AO-NEXT:  //         {
  // PRT-AO-NEXT:  //             int i;
  // PRT-AO-NEXT:  //             #pragma omp distribute parallel for simd num_threads(__clang_acc_num_workers__)
  // PRT-AO-NEXT:  //                 for (i = 0; i < 5; ++i) {
  // PRT-AO-NEXT:  //                     int var;
  // PRT-AO-NEXT:  //                     for (var = 0; var < 5; ++var)
  // PRT-AO-NEXT:  //                         ;
  // PRT-AO-NEXT:  //                 }
  // PRT-AO-NEXT:  //         }
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      const int __clang_acc_num_workers__ = non_const_expr;
  //  PRT-O-NEXT:      #pragma omp target teams
  //  PRT-O-NEXT:          {
  //  PRT-O-NEXT:              int i;
  //  PRT-O-NEXT:              #pragma omp distribute parallel for simd num_threads(__clang_acc_num_workers__)
  //  PRT-O-NEXT:                  for (i = 0; i < 5; ++i) {
  //  PRT-O-NEXT:                      int var;
  //  PRT-O-NEXT:                      for (var = 0; var < 5; ++var)
  //  PRT-O-NEXT:                          ;
  //  PRT-O-NEXT:                  }
  //  PRT-O-NEXT:          }
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc parallel loop worker vector num_workers(non_const_expr)
  // PRT-OA-NEXT:  // for (i = 0; i < 5; ++i)
  // PRT-OA-NEXT:  //   #pragma acc loop seq private(var)
  // PRT-OA-NEXT:  //   for (var = 0; var < 5; ++var)
  // PRT-OA-NEXT:  //     ;
  // PRT-OA-NEXT:  // ^----------ACC----------^
  #pragma acc parallel loop worker vector num_workers(non_const_expr)
  for (i = 0; i < 5; ++i)
    #pragma acc loop seq private(var)
    for (var = 0; var < 5; ++var)
      ;
}// PRT-NEXT:}

//--------------------------------------------------
// Directive and associate rewrite with discarded directive.
//--------------------------------------------------

// PRT-NEXT:void fullRewriteDirDiscard() {
void fullRewriteDirDiscard() {
  //  PRT-A-NEXT:  #pragma acc parallel
  // PRT-AO-NEXT:  // #pragma omp target teams firstprivate(var,non_const_expr)
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc loop seq private(i)
  //  PRT-A-NEXT:  for (i = 0; i < 5; ++i)
  //  PRT-A-NEXT:    var = non_const_expr;
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     int i;
  // PRT-AO-NEXT:  //     for (i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //         var = non_const_expr;
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  //  PRT-O-NEXT:  #pragma omp target teams firstprivate(var,non_const_expr)
  // PRT-OA-NEXT:  // #pragma acc parallel
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      int i;
  //  PRT-O-NEXT:      for (i = 0; i < 5; ++i)
  //  PRT-O-NEXT:          var = non_const_expr;
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc loop seq private(i)
  // PRT-OA-NEXT:  // for (i = 0; i < 5; ++i)
  // PRT-OA-NEXT:  //   var = non_const_expr;
  // PRT-OA-NEXT:  // ^----------ACC----------^
  #pragma acc parallel
  #pragma acc loop seq private(i)
  for (i = 0; i < 5; ++i)
    var = non_const_expr;

  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel loop seq private(i)
  //  PRT-A-NEXT:  for (i = 0; i < 5; ++i)
  //  PRT-A-NEXT:    var = non_const_expr;
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // #pragma omp target teams firstprivate(var,non_const_expr)
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     int i;
  // PRT-AO-NEXT:  //     for (i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //         var = non_const_expr;
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  #pragma omp target teams firstprivate(var,non_const_expr)
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      int i;
  //  PRT-O-NEXT:      for (i = 0; i < 5; ++i)
  //  PRT-O-NEXT:          var = non_const_expr;
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc parallel loop seq private(i)
  // PRT-OA-NEXT:  // for (i = 0; i < 5; ++i)
  // PRT-OA-NEXT:  //   var = non_const_expr;
  // PRT-OA-NEXT:  // ^----------ACC----------^
  #pragma acc parallel loop seq private(i)
  for (i = 0; i < 5; ++i)
    var = non_const_expr;
}// PRT-NEXT:}

//--------------------------------------------------
// cpp macro expansion in clauses.
//
// FIXME: The OpenMP version is fully expanded.  Eventually, we'd like to
// prevent that.
//--------------------------------------------------

// PRT-NEXT:void macroInClauses() {
void macroInClauses() {
  //  PRT-A-NEXT:  #pragma acc parallel vector_length(TWO)
  // PRT-AO-NEXT:  // #pragma omp target teams
  //  PRT-O-NEXT:  #pragma omp target teams
  // PRT-OA-NEXT:  // #pragma acc parallel vector_length(TWO)
  #pragma acc parallel vector_length(TWO)
  //  PRT-A-NEXT:  #pragma acc loop gang vector
  // PRT-AO-NEXT:  // #pragma omp distribute simd simdlen(2)
  //  PRT-O-NEXT:  #pragma omp distribute simd simdlen(2)
  // PRT-OA-NEXT:  // #pragma acc loop gang vector
  #pragma acc loop gang vector
  //    PRT-NEXT:  for (int i = 0; i < 5; ++i)
  for (int i = 0; i < 5; ++i)
  //    PRT-NEXT:    ;
    ;

  // Macro is at end of directive and only directive is rewritten.
  // Fortunately, Clang's end location for a directive is after the last
  // clause, so the rewrite succeeds.

  //    PRT-NEXT:  #define MAC vector_length(TWO)
  #define MAC vector_length(TWO)
  //  PRT-A-NEXT:  #pragma acc parallel MAC
  // PRT-AO-NEXT:  // #pragma omp target teams
  //  PRT-O-NEXT:  #pragma omp target teams
  // PRT-OA-NEXT:  // #pragma acc parallel MAC
  #pragma acc parallel MAC
  //  PRT-A-NEXT:  #pragma acc loop gang vector
  // PRT-AO-NEXT:  // #pragma omp distribute simd simdlen(2)
  //  PRT-O-NEXT:  #pragma omp distribute simd simdlen(2)
  // PRT-OA-NEXT:  // #pragma acc loop gang vector
  #pragma acc loop gang vector
  //    PRT-NEXT:  for (int i = 0; i < 5; ++i)
  for (int i = 0; i < 5; ++i)
  //    PRT-NEXT:    ;
    ;
  //    PRT-NEXT:  #undef MAC
  #undef MAC
}// PRT-NEXT:}

//--------------------------------------------------
// _Pragma form forces rewrite of directive and associate.
//
// If _Pragma were in a macro expansion, the rewrite would just fail with a
// diagnostic.  FIXME: However, when outside a macro expansion, the end
// location Clang assigns the _Pragma is unusable unfortunately, so a full
// rewrite is required.
//--------------------------------------------------

// PRT-NEXT:void pragmaFormForcesFullRewrite() {
void pragmaFormForcesFullRewrite() {
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  _Pragma("acc parallel")
  //  PRT-A-NEXT:  #pragma acc loop gang
  //  PRT-A-NEXT:  for (int i = 0; i < 5; ++i)
  //  PRT-A-NEXT:    ;
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // #pragma omp target teams
  // PRT-AO-NEXT:  //     #pragma omp distribute
  // PRT-AO-NEXT:  //         for (int i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //             ;
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  #pragma omp target teams
  //  PRT-O-NEXT:      #pragma omp distribute
  //  PRT-O-NEXT:          for (int i = 0; i < 5; ++i)
  //  PRT-O-NEXT:              ;
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // _Pragma("acc parallel")
  // PRT-OA-NEXT:  // #pragma acc loop gang
  // PRT-OA-NEXT:  // for (int i = 0; i < 5; ++i)
  // PRT-OA-NEXT:  //   ;
  // PRT-OA-NEXT:  // ^----------ACC----------^
  _Pragma("acc parallel")
  #pragma acc loop gang
  for (int i = 0; i < 5; ++i)
    ;

  //  PRT-A-NEXT:  #pragma acc parallel
  // PRT-AO-NEXT:  // #pragma omp target teams
  //  PRT-O-NEXT:  #pragma omp target teams
  // PRT-OA-NEXT:  // #pragma acc parallel
  //
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  _Pragma("acc loop gang")
  //  PRT-A-NEXT:  for (int i = 0; i < 5; ++i)
  //  PRT-A-NEXT:    ;
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // #pragma omp distribute
  // PRT-AO-NEXT:  //     for (int i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //         ;
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  #pragma omp distribute
  //  PRT-O-NEXT:      for (int i = 0; i < 5; ++i)
  //  PRT-O-NEXT:          ;
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // _Pragma("acc loop gang")
  // PRT-OA-NEXT:  // for (int i = 0; i < 5; ++i)
  // PRT-OA-NEXT:  //   ;
  // PRT-OA-NEXT:  // ^----------ACC----------^
  #pragma acc parallel
  _Pragma("acc loop gang")
  for (int i = 0; i < 5; ++i)
    ;
}// PRT-NEXT:}

//--------------------------------------------------
// Line continuations.
//--------------------------------------------------

// PRT-NEXT:void lineContinuations() {
void lineContinuations() {
  // Adequate indentation, which is reused in commented OpenACC.

  //  PRT-A-NEXT:  #pragma acc parallel \
  //  PRT-A-NEXT:          num_gangs(5)
  // PRT-AO-NEXT:  // #pragma omp target teams num_teams(5)
  //  PRT-O-NEXT:  #pragma omp target teams num_teams(5)
  // PRT-OA-NEXT:  // #pragma acc parallel \
  // PRT-OA-NEXT:  //         num_gangs(5)
  //    PRT-NEXT:  ;
  #pragma acc parallel \
          num_gangs(5)
  ;

  // Inadequate indentation, which is extended in commented OpenACC.

  //  PRT-A-NEXT:  #pragma acc parallel \
  //  PRT-A-NEXT: num_gangs(5)
  // PRT-AO-NEXT:  // #pragma omp target teams num_teams(5)
  //  PRT-O-NEXT:  #pragma omp target teams num_teams(5)
  // PRT-OA-NEXT:  // #pragma acc parallel \
  // PRT-OA-NEXT:  // num_gangs(5)
  //    PRT-NEXT:  ;
  #pragma acc parallel \
 num_gangs(5)
  ;
}// PRT-NEXT:}

// Adequate indentation, which is reused in commented OpenACC.

//  PRT-A-NEXT: #pragma acc routine \
//  PRT-A-NEXT:             seq
// PRT-AO-NEXT: // #pragma omp declare target
//  PRT-O-NEXT: #pragma omp declare target
// PRT-OA-NEXT: // #pragma acc routine \
// PRT-OA-NEXT: //             seq
//    PRT-NEXT:void lineContinuations_routineAdequateIndent();
//  PRT-O-NEXT: #pragma omp end declare target
// PRT-AO-NEXT: // #pragma omp end declare target
 #pragma acc routine \
             seq
void lineContinuations_routineAdequateIndent();

// Inadequate indentation, which is extended in commented OpenACC.

//  PRT-A-NEXT: #pragma acc routine \
//  PRT-A-NEXT:seq
// PRT-AO-NEXT: // #pragma omp declare target
//  PRT-O-NEXT: #pragma omp declare target
// PRT-OA-NEXT: // #pragma acc routine \
// PRT-OA-NEXT: // seq
//    PRT-NEXT:void lineContinuations_routineInadequateIndent();
//  PRT-O-NEXT: #pragma omp end declare target
// PRT-AO-NEXT: // #pragma omp end declare target
 #pragma acc routine \
seq
void lineContinuations_routineInadequateIndent();

//--------------------------------------------------
// Associated statement inadequate indentation extended in commented OpenACC.
//--------------------------------------------------

// PRT-NEXT:void associateInadequateIndentation() {
void associateInadequateIndentation() {
  //  PRT-A-NEXT:  #pragma acc parallel
  // PRT-AO-NEXT:  // #pragma omp target teams firstprivate(var,non_const_expr)
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc loop vector
  //  PRT-A-NEXT:for (i = 0; i < 5; ++i) {
  //  PRT-A-NEXT: var = 5;
  //  PRT-A-NEXT:  var = non_const_expr ;
  //  PRT-A-NEXT:}
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     int i;
  // PRT-AO-NEXT:  //     #pragma omp distribute simd
  // PRT-AO-NEXT:  //         for (i = 0; i < 5; ++i) {
  // PRT-AO-NEXT:  //             var = 5;
  // PRT-AO-NEXT:  //             var = non_const_expr;
  // PRT-AO-NEXT:  //         }
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  //  PRT-O-NEXT:  #pragma omp target teams firstprivate(var,non_const_expr)
  // PRT-OA-NEXT:  // #pragma acc parallel
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      int i;
  //  PRT-O-NEXT:      #pragma omp distribute simd
  //  PRT-O-NEXT:          for (i = 0; i < 5; ++i) {
  //  PRT-O-NEXT:              var = 5;
  //  PRT-O-NEXT:              var = non_const_expr;
  //  PRT-O-NEXT:          }
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc loop vector
  // PRT-OA-NEXT:  // for (i = 0; i < 5; ++i) {
  // PRT-OA-NEXT:  // var = 5;
  // PRT-OA-NEXT:  // var = non_const_expr ;
  // PRT-OA-NEXT:  // }
  // PRT-OA-NEXT:  // ^----------ACC----------^
  #pragma acc parallel
  #pragma acc loop vector
for (i = 0; i < 5; ++i) {
 var = 5;
  var = non_const_expr ;
}
}// PRT-NEXT:}

//--------------------------------------------------
// Directive only rewrite, comment after.
//--------------------------------------------------

// PRT-NEXT:void commentAfterDirOnlyRewrite() {
void commentAfterDirOnlyRewrite() {
  //  PRT-A-NEXT:  #pragma acc parallel /*comment*/
  // PRT-AO-NEXT:  // #pragma omp target teams
  //  PRT-O-NEXT:  #pragma omp target teams
  // PRT-OA-NEXT:  // #pragma acc parallel /*comment*/
  //    PRT-NEXT:  ;
  #pragma acc parallel /*comment*/
  ;

  //  PRT-A-NEXT:  #pragma acc parallel /*multi-line
  //  PRT-A-NEXT:                         comment*/
  // PRT-AO-NEXT:  // #pragma omp target teams
  //  PRT-O-NEXT:  #pragma omp target teams
  // PRT-OA-NEXT:  // #pragma acc parallel /*multi-line
  // PRT-OA-NEXT:  //                        comment*/
  //    PRT-NEXT:  ;
  #pragma acc parallel /*multi-line
                         comment*/
  ;
}// PRT-NEXT:}

//  PRT-A-NEXT:#pragma acc routine seq /*comment*/
// PRT-AO-NEXT:// #pragma omp declare target
//  PRT-O-NEXT:#pragma omp declare target
// PRT-OA-NEXT:// #pragma acc routine seq /*comment*/
//    PRT-NEXT:void commentAfterDirOnlyRewrite_routine();
//  PRT-O-NEXT:#pragma omp end declare target
// PRT-AO-NEXT:// #pragma omp end declare target
#pragma acc routine seq /*comment*/
void commentAfterDirOnlyRewrite_routine();

//  PRT-A-NEXT:#pragma acc routine seq /*multi-line
//  PRT-A-NEXT:                          comment*/
// PRT-AO-NEXT:// #pragma omp declare target
//  PRT-O-NEXT:#pragma omp declare target
// PRT-OA-NEXT:// #pragma acc routine seq /*multi-line
// PRT-OA-NEXT://                           comment*/
//    PRT-NEXT:void commentAfterDirOnlyRewrite_routineMultiline();
//  PRT-O-NEXT:#pragma omp end declare target
// PRT-AO-NEXT:// #pragma omp end declare target
#pragma acc routine seq /*multi-line
                          comment*/
void commentAfterDirOnlyRewrite_routineMultiline();

//--------------------------------------------------
// Directive and associate rewrite, trailing text.
//--------------------------------------------------

// PRT-NEXT:void trailingTextAfterFullRewrite() {
void trailingTextAfterFullRewrite() {
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel num_workers(non_const_expr)
  //  PRT-A-NEXT:  #pragma acc loop worker
  //  PRT-A-NEXT:  for (int i = 0; i < 5; ++i)
  // PRT-AA-NEXT:    ;var = 5;
  // PRT-AO-NEXT:    ;
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     const int __clang_acc_num_workers__ = non_const_expr;
  // PRT-AO-NEXT:  //     #pragma omp target teams
  // PRT-AO-NEXT:  //         #pragma omp distribute parallel for num_threads(__clang_acc_num_workers__)
  // PRT-AO-NEXT:  //             for (int i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //                 ;
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  // PRT-AO-NEXT:var = 5;
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      const int __clang_acc_num_workers__ = non_const_expr;
  //  PRT-O-NEXT:      #pragma omp target teams
  //  PRT-O-NEXT:          #pragma omp distribute parallel for num_threads(__clang_acc_num_workers__)
  //  PRT-O-NEXT:              for (int i = 0; i < 5; ++i)
  //  PRT-O-NEXT:                  ;
  // PRT-OO-NEXT:  }var = 5;
  // PRT-OA-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc parallel num_workers(non_const_expr)
  // PRT-OA-NEXT:  // #pragma acc loop worker
  // PRT-OA-NEXT:  // for (int i = 0; i < 5; ++i)
  // PRT-OA-NEXT:  //   ;
  // PRT-OA-NEXT:  // ^----------ACC----------^
  // PRT-OA-NEXT:var = 5;
  #pragma acc parallel num_workers(non_const_expr)
  #pragma acc loop worker
  for (int i = 0; i < 5; ++i)
    ;var = 5;
  // "var" is flush when semicolon so that, if we're inadvertently using
  // CharSourceRange::getTokenRange instead of CharSourceRange::getCharRange
  // for either reading the OpenACC range or replacing it, we'll incorrectly
  // pick up var in the range.

  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel num_workers(non_const_expr)
  //  PRT-A-NEXT:  #pragma acc loop worker
  //  PRT-A-NEXT:  for (int i = 0; i < 5; ++i)
  // PRT-AA-NEXT:    ;{{  }}
  // PRT-AO-NEXT:    ;
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     const int __clang_acc_num_workers__ = non_const_expr;
  // PRT-AO-NEXT:  //     #pragma omp target teams
  // PRT-AO-NEXT:  //         #pragma omp distribute parallel for num_threads(__clang_acc_num_workers__)
  // PRT-AO-NEXT:  //             for (int i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //                 ;
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^{{  }}
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      const int __clang_acc_num_workers__ = non_const_expr;
  //  PRT-O-NEXT:      #pragma omp target teams
  //  PRT-O-NEXT:          #pragma omp distribute parallel for num_threads(__clang_acc_num_workers__)
  //  PRT-O-NEXT:              for (int i = 0; i < 5; ++i)
  //  PRT-O-NEXT:                  ;
  // PRT-OO-NEXT:  }{{  }}
  // PRT-OA-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc parallel num_workers(non_const_expr)
  // PRT-OA-NEXT:  // #pragma acc loop worker
  // PRT-OA-NEXT:  // for (int i = 0; i < 5; ++i)
  // PRT-OA-NEXT:  //   ;
  // PRT-OA-NEXT:  // ^----------ACC----------^{{  }}
  #pragma acc parallel num_workers(non_const_expr)
  #pragma acc loop worker
  for (int i = 0; i < 5; ++i)
    ;  // comment stripped by a RUN command, but it shows intended whitespace
}// PRT-NEXT:}

//  PRT-A-NEXT: #pragma acc routine seq
// PRT-AO-NEXT: // #pragma omp declare target
// PRT-AA-NEXT: void trailingTextAfterFullRewrite_routineProtoNonWs() ;int trailingTextAfterFullRewrite_protoVar;
// PRT-AO-NEXT: void trailingTextAfterFullRewrite_routineProtoNonWs() ;
// PRT-AO-NEXT: // #pragma omp end declare target
// PRT-AO-NEXT:int trailingTextAfterFullRewrite_protoVar;
//
//  PRT-O-NEXT: #pragma omp declare target
// PRT-OA-NEXT: // #pragma acc routine seq
//  PRT-O-NEXT: void trailingTextAfterFullRewrite_routineProtoNonWs() ;
//  PRT-O-NEXT: #pragma omp end declare target
//  PRT-O-NEXT:int trailingTextAfterFullRewrite_protoVar;
 #pragma acc routine seq
 void trailingTextAfterFullRewrite_routineProtoNonWs() ;int trailingTextAfterFullRewrite_protoVar;
// "int" is flush with semicolon for same reasons described above.

//  PRT-A-NEXT: #pragma acc routine seq
// PRT-AO-NEXT: // #pragma omp declare target
//  PRT-A-NEXT: void trailingTextAfterFullRewrite_routineDefNonWs(){
// PRT-AA-NEXT: }int trailingTextAfterFullRewrite_defVar;
// PRT-AO-NEXT: }
// PRT-AO-NEXT: // #pragma omp end declare target
// PRT-AO-NEXT:int trailingTextAfterFullRewrite_defVar;
//
//  PRT-O-NEXT: #pragma omp declare target
// PRT-OA-NEXT: // #pragma acc routine seq
//  PRT-O-NEXT: void trailingTextAfterFullRewrite_routineDefNonWs(){
//  PRT-O-NEXT: }
//  PRT-O-NEXT: #pragma omp end declare target
//  PRT-O-NEXT:int trailingTextAfterFullRewrite_defVar;
 #pragma acc routine seq
 void trailingTextAfterFullRewrite_routineDefNonWs(){
 }int trailingTextAfterFullRewrite_defVar;
// "int" is flush with semicolon for same reasons described above.

//  PRT-A-NEXT: #pragma acc routine seq
// PRT-AO-NEXT: // #pragma omp declare target
// PRT-AA-NEXT: void trailingTextAfterFullRewrite_routineProtoWs() ;{{  }}
// PRT-AO-NEXT: void trailingTextAfterFullRewrite_routineProtoWs() ;
// PRT-AO-NEXT: // #pragma omp end declare target{{  }}
//
//  PRT-O-NEXT: #pragma omp declare target
// PRT-OA-NEXT: // #pragma acc routine seq
//  PRT-O-NEXT: void trailingTextAfterFullRewrite_routineProtoWs() ;
//  PRT-O-NEXT: #pragma omp end declare target{{  }}
 #pragma acc routine seq
 void trailingTextAfterFullRewrite_routineProtoWs() ;  // comment stripped by a RUN command, but it shows intended whitespace

//  PRT-A-NEXT: #pragma acc routine seq
// PRT-AO-NEXT: // #pragma omp declare target
//  PRT-A-NEXT: void trailingTextAfterFullRewrite_routineDefWs(){
// PRT-AA-NEXT: }{{  }}
// PRT-AO-NEXT: }
// PRT-AO-NEXT: // #pragma omp end declare target{{  }}
//
//  PRT-O-NEXT: #pragma omp declare target
// PRT-OA-NEXT: // #pragma acc routine seq
//  PRT-O-NEXT: void trailingTextAfterFullRewrite_routineDefWs(){
//  PRT-O-NEXT: }
//  PRT-O-NEXT: #pragma omp end declare target{{  }}
 #pragma acc routine seq
 void trailingTextAfterFullRewrite_routineDefWs(){
 }  // comment stripped by a RUN command, but it shows intended whitespace

//--------------------------------------------------
// After an error, all translation of OpenACC subtrees to OpenMP is
// suppressed, so RewriteOpenACC has nothing to do.  Make sure it's handled
// gracefully, in nested directives and subsequent directives.
//--------------------------------------------------

// PRT-NEXT:void afterError() {
void afterError() {
// PRT-NEXT:#if ERRS
#if ERRS
  // PRT-NEXT:  /* expected-error{{.*}} */
  // PRT-NEXT:  /* expected-warning{{.*}} */
  // PRT-NEXT:  #pragma acc parallel vector_length(f)
  /* expected-error@+2 {{expression must have integral or unscoped enumeration type, not 'float'}} */
  /* expected-warning@+1 {{'vector_length' ignored because argument is not an integer constant expression}} */
  #pragma acc parallel vector_length(f)
  // PRT-NEXT:  #pragma acc loop
  #pragma acc loop
  // PRT-NEXT:  for (int i = 0; i < 5; ++i)
  for (int i = 0; i < 5; ++i)
  // PRT-NEXT:    ;
    ;

  // PRT-NEXT:  #pragma acc parallel
  #pragma acc parallel
  // PRT-NEXT:  #pragma acc loop gang
  #pragma acc loop gang
  // PRT-NEXT:  for (int i = 0; i < 5; ++i)
  for (int i = 0; i < 5; ++i)
  // PRT-NEXT:    ;
    ;

// PRT-NEXT:#endif
#endif
}// PRT-NEXT:}
// PRT-EMPTY:
// PRT-NEXT:{{  }}
// PRT-NEXT:// comment
// PRT-NEXT:/* multi-line
// PRT-NEXT:   comment */
// PRT-NEXT:int foobar; // end-of-line comment
// PRT-NOT:{{.}}
