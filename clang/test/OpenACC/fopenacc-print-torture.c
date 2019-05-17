// This test checks -fopenacc-print output for no preprocessor expansion,
// layout preservation, and exact formatting for many cases that are special
// to -fopenacc-print but not so much to -fopenacc-ast-print.

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

// PRT-NEXT:int main() {
int main() {
  // PRT-NEXT:  float f = 0;
  // PRT-NEXT:  int non_const_expr = 2;
  // PRT-NEXT:  int var, i;
  float f = 0;
  int non_const_expr = 2;
  int var, i;

  //--------------------------------------------------
  // Directive only rewrite, not nested.
  //--------------------------------------------------

  //  PRT-A-NEXT:  #pragma acc parallel
  // PRT-AO-NEXT:  // #pragma omp target teams
  //  PRT-O-NEXT:  #pragma omp target teams
  // PRT-OA-NEXT:  // #pragma acc parallel
  #pragma acc parallel
    // PRT-NEXT:    ;
    ;

  //--------------------------------------------------
  // Directive only rewrite, nested.
  //--------------------------------------------------

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

  //--------------------------------------------------
  // Directive only rewrite, but directive discarded.
  //--------------------------------------------------

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

  //--------------------------------------------------
  // Full construct rewrite, outer directive only.
  //
  // Check for corruption due to the way Clang records the end location.  Clang
  // selects different end tokens in different cases, and Clang records the
  // start location not end location of that end token as the end location of
  // the associated statement.  RewriteOpenACC has to adjust for all these
  // cases.
  //--------------------------------------------------

  // Null statement: Clang selects semicolon as end token.

  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel num_workers(non_const_expr)
  //  PRT-A-NEXT:  #pragma acc loop worker
  //  PRT-A-NEXT:  for (int i = 0; i < 5; ++i)
  //  PRT-A-NEXT:    ;
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     const int __clang_acc_num_workers__ = non_const_expr;
  // PRT-AO-NEXT:  //     #pragma omp target teams
  // PRT-AO-NEXT:  //         #pragma omp parallel for num_threads(__clang_acc_num_workers__)
  // PRT-AO-NEXT:  //             for (int i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //                 ;
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      const int __clang_acc_num_workers__ = non_const_expr;
  //  PRT-O-NEXT:      #pragma omp target teams
  //  PRT-O-NEXT:          #pragma omp parallel for num_threads(__clang_acc_num_workers__)
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

  // Compound statement: Clang selects closing brace as end token.

  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel num_workers(non_const_expr)
  //  PRT-A-NEXT:  #pragma acc loop worker
  //  PRT-A-NEXT:  for (int i = 0; i < 5; ++i) {
  //  PRT-A-NEXT:    var = 5;
  //  PRT-A-NEXT:    var = non_const_expr;
  //  PRT-A-NEXT:  }
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     const int __clang_acc_num_workers__ = non_const_expr;
  // PRT-AO-NEXT:  //     #pragma omp target teams firstprivate(var,non_const_expr)
  // PRT-AO-NEXT:  //         #pragma omp parallel for num_threads(__clang_acc_num_workers__) shared(var,non_const_expr)
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
  //  PRT-O-NEXT:          #pragma omp parallel for num_threads(__clang_acc_num_workers__) shared(var,non_const_expr)
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
  // PRT-OA-NEXT:  //   var = non_const_expr;
  // PRT-OA-NEXT:  // }
  // PRT-OA-NEXT:  // ^----------ACC----------^
  #pragma acc parallel num_workers(non_const_expr)
  #pragma acc loop worker
  for (int i = 0; i < 5; ++i) {
    var = 5;
    var = non_const_expr;
  }

  // Expression statement: Clang selects token before semicolon as end token.

  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel num_workers(non_const_expr)
  //  PRT-A-NEXT:  #pragma acc loop worker
  //  PRT-A-NEXT:  for (int i = 0; i < 5; ++i)
  //  PRT-A-NEXT:    var = non_const_expr;
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     const int __clang_acc_num_workers__ = non_const_expr;
  // PRT-AO-NEXT:  //     #pragma omp target teams firstprivate(var,non_const_expr)
  // PRT-AO-NEXT:  //         #pragma omp parallel for num_threads(__clang_acc_num_workers__) shared(var,non_const_expr)
  // PRT-AO-NEXT:  //             for (int i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //                 var = non_const_expr;
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      const int __clang_acc_num_workers__ = non_const_expr;
  //  PRT-O-NEXT:      #pragma omp target teams firstprivate(var,non_const_expr)
  //  PRT-O-NEXT:          #pragma omp parallel for num_threads(__clang_acc_num_workers__) shared(var,non_const_expr)
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

  //--------------------------------------------------
  // Full construct rewrite, inner directive only.
  //--------------------------------------------------

  // Null statement: Clang selects semicolon as end token.

  //  PRT-A-NEXT:  #pragma acc parallel
  // PRT-AO-NEXT:  // #pragma omp target teams firstprivate(i)
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc loop vector
  //  PRT-A-NEXT:  for (i = 0; i < 5; ++i)
  //  PRT-A-NEXT:    ;
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     int i;
  // PRT-AO-NEXT:  //     #pragma omp parallel for simd num_threads(1)
  // PRT-AO-NEXT:  //         for (i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //             ;
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  //  PRT-O-NEXT:  #pragma omp target teams firstprivate(i)
  // PRT-OA-NEXT:  // #pragma acc parallel
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      int i;
  //  PRT-O-NEXT:      #pragma omp parallel for simd num_threads(1)
  //  PRT-O-NEXT:          for (i = 0; i < 5; ++i)
  //  PRT-O-NEXT:              ;
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc loop vector
  // PRT-OA-NEXT:  // for (i = 0; i < 5; ++i)
  // PRT-OA-NEXT:  //   ;
  // PRT-OA-NEXT:  // ^----------ACC----------^
  #pragma acc parallel
  #pragma acc loop vector
  for (i = 0; i < 5; ++i)
    ;

  // Compound statement: Clang selects closing brace as end token.

  //  PRT-A-NEXT:  #pragma acc parallel
  // PRT-AO-NEXT:  // #pragma omp target teams firstprivate(i,var,non_const_expr)
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc loop vector
  //  PRT-A-NEXT:  for (i = 0; i < 5; ++i) {
  //  PRT-A-NEXT:    var = 5;
  //  PRT-A-NEXT:    var = non_const_expr ;
  //  PRT-A-NEXT:  }
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     int i;
  // PRT-AO-NEXT:  //     #pragma omp parallel for simd num_threads(1) shared(var,non_const_expr)
  // PRT-AO-NEXT:  //         for (i = 0; i < 5; ++i) {
  // PRT-AO-NEXT:  //             var = 5;
  // PRT-AO-NEXT:  //             var = non_const_expr;
  // PRT-AO-NEXT:  //         }
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  //  PRT-O-NEXT:  #pragma omp target teams firstprivate(i,var,non_const_expr)
  // PRT-OA-NEXT:  // #pragma acc parallel
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      int i;
  //  PRT-O-NEXT:      #pragma omp parallel for simd num_threads(1) shared(var,non_const_expr)
  //  PRT-O-NEXT:          for (i = 0; i < 5; ++i) {
  //  PRT-O-NEXT:              var = 5;
  //  PRT-O-NEXT:              var = non_const_expr;
  //  PRT-O-NEXT:          }
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc loop vector
  // PRT-OA-NEXT:  // for (i = 0; i < 5; ++i) {
  // PRT-OA-NEXT:  //   var = 5;
  // PRT-OA-NEXT:  //   var = non_const_expr ;
  // PRT-OA-NEXT:  // }
  // PRT-OA-NEXT:  // ^----------ACC----------^
  #pragma acc parallel
  #pragma acc loop vector
  for (i = 0; i < 5; ++i) {
    var = 5;
    var = non_const_expr ;
  }

  // Expression statement: Clang selects token before semicolon as end token.

  //  PRT-A-NEXT:  #pragma acc parallel
  // PRT-AO-NEXT:  // #pragma omp target teams firstprivate(i,var,non_const_expr)
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc loop vector
  //  PRT-A-NEXT:  for (i = 0; i < 5; ++i)
  //  PRT-A-NEXT:    var = non_const_expr ;
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     int i;
  // PRT-AO-NEXT:  //     #pragma omp parallel for simd num_threads(1) shared(var,non_const_expr)
  // PRT-AO-NEXT:  //         for (i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //             var = non_const_expr;
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  //  PRT-O-NEXT:  #pragma omp target teams firstprivate(i,var,non_const_expr)
  // PRT-OA-NEXT:  // #pragma acc parallel
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      int i;
  //  PRT-O-NEXT:      #pragma omp parallel for simd num_threads(1) shared(var,non_const_expr)
  //  PRT-O-NEXT:          for (i = 0; i < 5; ++i)
  //  PRT-O-NEXT:              var = non_const_expr;
  //  PRT-O-NEXT:  }
  // PRT-OA-NEXT:  // ---------OMP<-ACC--------
  // PRT-OA-NEXT:  // #pragma acc loop vector
  // PRT-OA-NEXT:  // for (i = 0; i < 5; ++i)
  // PRT-OA-NEXT:  //   var = non_const_expr ;
  // PRT-OA-NEXT:  // ^----------ACC----------^
  #pragma acc parallel
  #pragma acc loop vector
  for (i = 0; i < 5; ++i)
    var = non_const_expr ;

  //--------------------------------------------------
  // Full construct rewrite, outer and inner directive.
  //--------------------------------------------------

  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc parallel num_workers(non_const_expr)
  //  PRT-A-NEXT:  #pragma acc loop worker vector
  //  PRT-A-NEXT:  for (i = 0; i < 5; ++i)
  //  PRT-A-NEXT:    ;
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     const int __clang_acc_num_workers__ = non_const_expr;
  // PRT-AO-NEXT:  //     #pragma omp target teams firstprivate(i)
  // PRT-AO-NEXT:  //         {
  // PRT-AO-NEXT:  //             int i;
  // PRT-AO-NEXT:  //             #pragma omp parallel for simd num_threads(__clang_acc_num_workers__)
  // PRT-AO-NEXT:  //                 for (i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //                     ;
  // PRT-AO-NEXT:  //         }
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      const int __clang_acc_num_workers__ = non_const_expr;
  //  PRT-O-NEXT:      #pragma omp target teams firstprivate(i)
  //  PRT-O-NEXT:          {
  //  PRT-O-NEXT:              int i;
  //  PRT-O-NEXT:              #pragma omp parallel for simd num_threads(__clang_acc_num_workers__)
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

  //--------------------------------------------------
  // Full construct rewrite with discarded directive.
  //--------------------------------------------------

  //  PRT-A-NEXT:  #pragma acc parallel
  // PRT-AO-NEXT:  // #pragma omp target teams firstprivate(i,var,non_const_expr)
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
  //  PRT-O-NEXT:  #pragma omp target teams firstprivate(i,var,non_const_expr)
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

  //--------------------------------------------------
  // cpp macro expansion in OpenACC.
  //--------------------------------------------------

  //  PRT-A-NEXT:  #pragma acc parallel vector_length(TWO)
  // PRT-AO-NEXT:  // #pragma omp target teams
  //  PRT-O-NEXT:  #pragma omp target teams
  // PRT-OA-NEXT:  // #pragma acc parallel vector_length(TWO)
  #pragma acc parallel vector_length(TWO)
  //  PRT-A-NEXT:  #pragma acc loop gang
  // PRT-AO-NEXT:  // #pragma omp distribute
  //  PRT-O-NEXT:  #pragma omp distribute
  // PRT-OA-NEXT:  // #pragma acc loop gang
  #pragma acc loop gang
  // PRT-NEXT:  for (int i = 0; i < 5; ++i)
  for (int i = 0; i < 5; ++i)
  // PRT-NEXT:    ;
    ;

  //--------------------------------------------------
  // Line continuations.
  //--------------------------------------------------

  // Adequate indentation, which is reused in commented OpenACC.

  //  PRT-A-NEXT:  #pragma acc parallel \
  //  PRT-A-NEXT:          num_gangs(5)
  // PRT-AO-NEXT:  // #pragma omp target teams num_teams(5)
  //  PRT-O-NEXT:  #pragma omp target teams num_teams(5)
  // PRT-OA-NEXT:  // #pragma acc parallel \
  // PRT-OA-NEXT:  //         num_gangs(5)
  #pragma acc parallel \
          num_gangs(5)
    // PRT-NEXT:    ;
    ;

  // Inadequate indentation, which is extended in commented OpenACC.

  //  PRT-A-NEXT:  #pragma acc parallel \
  //  PRT-A-NEXT: num_gangs(5)
  // PRT-AO-NEXT:  // #pragma omp target teams num_teams(5)
  //  PRT-O-NEXT:  #pragma omp target teams num_teams(5)
  // PRT-OA-NEXT:  // #pragma acc parallel \
  // PRT-OA-NEXT:  // num_gangs(5)
  #pragma acc parallel \
 num_gangs(5)
    // PRT-NEXT:    ;
    ;

  //--------------------------------------------------
  // Associated statement inadequate indentation extended in commented OpenACC.
  //--------------------------------------------------

  //  PRT-A-NEXT:  #pragma acc parallel
  // PRT-AO-NEXT:  // #pragma omp target teams firstprivate(i,var,non_const_expr)
  // PRT-AO-NEXT:  // v----------ACC----------v
  //  PRT-A-NEXT:  #pragma acc loop vector
  //  PRT-A-NEXT:for (i = 0; i < 5; ++i) {
  //  PRT-A-NEXT: var = 5;
  //  PRT-A-NEXT:  var = non_const_expr ;
  //  PRT-A-NEXT:}
  // PRT-AO-NEXT:  // ---------ACC->OMP--------
  // PRT-AO-NEXT:  // {
  // PRT-AO-NEXT:  //     int i;
  // PRT-AO-NEXT:  //     #pragma omp parallel for simd num_threads(1) shared(var,non_const_expr)
  // PRT-AO-NEXT:  //         for (i = 0; i < 5; ++i) {
  // PRT-AO-NEXT:  //             var = 5;
  // PRT-AO-NEXT:  //             var = non_const_expr;
  // PRT-AO-NEXT:  //         }
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^
  //
  //  PRT-O-NEXT:  #pragma omp target teams firstprivate(i,var,non_const_expr)
  // PRT-OA-NEXT:  // #pragma acc parallel
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      int i;
  //  PRT-O-NEXT:      #pragma omp parallel for simd num_threads(1) shared(var,non_const_expr)
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

  //--------------------------------------------------
  // Directive only rewrite, comment after.
  //--------------------------------------------------

  //  PRT-A-NEXT:  #pragma acc parallel /*comment*/
  // PRT-AO-NEXT:  // #pragma omp target teams
  //  PRT-O-NEXT:  #pragma omp target teams
  // PRT-OA-NEXT:  // #pragma acc parallel /*comment*/
  #pragma acc parallel /*comment*/
    // PRT-NEXT:    ;
    ;

  //  PRT-A-NEXT:  #pragma acc parallel /*multi-line
  //  PRT-A-NEXT:                         comment*/
  // PRT-AO-NEXT:  // #pragma omp target teams
  //  PRT-O-NEXT:  #pragma omp target teams
  // PRT-OA-NEXT:  // #pragma acc parallel /*multi-line
  // PRT-OA-NEXT:  //                        comment*/
  #pragma acc parallel /*multi-line
                         comment*/
    // PRT-NEXT:    ;
    ;

  //--------------------------------------------------
  // Full construct rewrite, trailing text.
  //--------------------------------------------------

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
  // PRT-AO-NEXT:  //         #pragma omp parallel for num_threads(__clang_acc_num_workers__)
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
  //  PRT-O-NEXT:          #pragma omp parallel for num_threads(__clang_acc_num_workers__)
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
  // PRT-AO-NEXT:  //         #pragma omp parallel for num_threads(__clang_acc_num_workers__)
  // PRT-AO-NEXT:  //             for (int i = 0; i < 5; ++i)
  // PRT-AO-NEXT:  //                 ;
  // PRT-AO-NEXT:  // }
  // PRT-AO-NEXT:  // ^----------OMP----------^{{  }}
  //
  // PRT-OA-NEXT:  // v----------OMP----------v
  //  PRT-O-NEXT:  {
  //  PRT-O-NEXT:      const int __clang_acc_num_workers__ = non_const_expr;
  //  PRT-O-NEXT:      #pragma omp target teams
  //  PRT-O-NEXT:          #pragma omp parallel for num_threads(__clang_acc_num_workers__)
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

  //--------------------------------------------------
  // After an error, all translation of OpenACC subtrees to OpenMP is
  // suppressed, so RewriteOpenACC has nothing to do.  Make sure it's handled
  // gracefully, in nested directives and subsequent directives.
  //--------------------------------------------------

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
