// TODO: This test has become a mess.  Separate out each feature being tested
// into separate code and add comments to each explaining what's being tested.

// Check implicit and explicit data sharing attributes on "acc parallel".
//
// When ADD_LOOP_TO_PAR is not set, this file checks implicit and explicit
// data sharing attributes on "acc parallel" without "loop".
//
// When ADD_LOOP_TO_PAR is set, it adds "loop seq" and a for loop to those "acc
// parallel" directives in order to check data sharing attributes for combined
// "acc parallel loop" directives.
//
// RUN: %data directives {
// RUN:   (dir=PAR     dir-cflags=                 )
// RUN:   (dir=PARLOOP dir-cflags=-DADD_LOOP_TO_PAR)
// RUN: }

// Check ASTDumper.
//
// RUN: %data dmps {
// RUN:   (mode=MODE_I pre=DMP,DMP-I,DMP-IF,DMP-IP,DMP-%[dir],DMP-%[dir]-I,DMP-%[dir]-IF,DMP-%[dir]-IP)
// RUN:   (mode=MODE_F pre=DMP,DMP-F,DMP-IF,DMP-FP,DMP-%[dir],DMP-%[dir]-F,DMP-%[dir]-IF,DMP-%[dir]-FP)
// RUN:   (mode=MODE_P pre=DMP,DMP-P,DMP-IP,DMP-FP,DMP-%[dir],DMP-%[dir]-P,DMP-%[dir]-IP,DMP-%[dir]-FP)
// RUN: }
// RUN: %for directives {
// RUN:   %for dmps {
// RUN:     %clang -Xclang -verify -Xclang -ast-dump -fsyntax-only -fopenacc \
// RUN:            -DMODE=%[mode] %[dir-cflags] %s \
// RUN:     | FileCheck -check-prefixes=%[pre] %s
// RUN:   }
// RUN: }

// Check ASTWriterStmt, ASTReaderStmt, StmtPrinter, and
// ACCParallelDirective::CreateEmpty (used by ASTReaderStmt).
//
// RUN: %data asts {
// RUN:   (mode=MODE_I pre=PRT,PRT-I,PRT-%[dir],PRT-%[dir]-I,PRT-A,PRT-A-I,PRT-A-%[dir],PRT-A-%[dir]-I)
// RUN:   (mode=MODE_F pre=PRT,PRT-F,PRT-%[dir],PRT-%[dir]-F,PRT-A,PRT-A-F,PRT-A-%[dir],PRT-A-%[dir]-F)
// RUN:   (mode=MODE_P pre=PRT,PRT-P,PRT-%[dir],PRT-%[dir]-P,PRT-A,PRT-A-P,PRT-A-%[dir],PRT-A-%[dir]-P)
// RUN: }
// RUN: %for directives {
// RUN:   %for asts {
// RUN:     %clang -Xclang -verify -fopenacc -emit-ast -o %t.ast \
// RUN:            -DMODE=%[mode] %[dir-cflags] %s
// RUN:     %clang_cc1 -ast-print %t.ast 2>&1 \
// RUN:     | FileCheck -check-prefixes=%[pre] %s
// RUN:   }
// RUN: }

// Check -fopenacc[-ast]-print.  Default is checked above.
//
// Strip comments and blank lines so checking -fopenacc-print output is easier.
// RUN: echo "// expected""-no-diagnostics" > %t-acc.c
// RUN: grep -v '^ *\(//.*\)\?$' %s | sed 's,//.*,,' >> %t-acc.c
//
// TODO: If lit were to support %for inside a %data, we could iterate prt-opts
// (which would need additional fields) within prt-args, significantly
// shortening the prt-args definition.
//
// RUN: %data prt-opts {
// RUN:   (prt-opt=-fopenacc-ast-print)
// RUN:   (prt-opt=-fopenacc-print    )
// RUN: }
// RUN: %data prt-args {
// RUN:   (mode=MODE_I prt=%[prt-opt]=acc     prt-chk=PRT,PRT-I,PRT-%[dir],PRT-%[dir]-I,PRT-A,PRT-A-I,PRT-A-%[dir],PRT-A-%[dir]-I                                              )
// RUN:   (mode=MODE_I prt=%[prt-opt]=omp     prt-chk=PRT,PRT-I,PRT-%[dir],PRT-%[dir]-I,PRT-O,PRT-O-I,PRT-O-%[dir],PRT-O-%[dir]-I                                              )
// RUN:   (mode=MODE_I prt=%[prt-opt]=acc-omp prt-chk=PRT,PRT-I,PRT-%[dir],PRT-%[dir]-I,PRT-A,PRT-A-I,PRT-A-%[dir],PRT-A-%[dir]-I,PRT-AO,PRT-AO-I,PRT-AO-%[dir],PRT-AO-%[dir]-I)
// RUN:   (mode=MODE_I prt=%[prt-opt]=omp-acc prt-chk=PRT,PRT-I,PRT-%[dir],PRT-%[dir]-I,PRT-O,PRT-O-I,PRT-O-%[dir],PRT-O-%[dir]-I,PRT-OA,PRT-OA-I,PRT-OA-%[dir],PRT-OA-%[dir]-I)
//
// RUN:   (mode=MODE_F prt=%[prt-opt]=acc     prt-chk=PRT,PRT-F,PRT-%[dir],PRT-%[dir]-F,PRT-A,PRT-A-F,PRT-A-%[dir],PRT-A-%[dir]-F                                              )
// RUN:   (mode=MODE_F prt=%[prt-opt]=omp     prt-chk=PRT,PRT-F,PRT-%[dir],PRT-%[dir]-F,PRT-O,PRT-O-F,PRT-O-%[dir],PRT-O-%[dir]-F                                              )
// RUN:   (mode=MODE_F prt=%[prt-opt]=acc-omp prt-chk=PRT,PRT-F,PRT-%[dir],PRT-%[dir]-F,PRT-A,PRT-A-F,PRT-A-%[dir],PRT-A-%[dir]-F,PRT-AO,PRT-AO-F,PRT-AO-%[dir],PRT-AO-%[dir]-F)
// RUN:   (mode=MODE_F prt=%[prt-opt]=omp-acc prt-chk=PRT,PRT-F,PRT-%[dir],PRT-%[dir]-F,PRT-O,PRT-O-F,PRT-O-%[dir],PRT-O-%[dir]-F,PRT-OA,PRT-OA-F,PRT-OA-%[dir],PRT-OA-%[dir]-F)
//
// RUN:   (mode=MODE_P prt=%[prt-opt]=acc     prt-chk=PRT,PRT-P,PRT-%[dir],PRT-%[dir]-P,PRT-A,PRT-A-P,PRT-A-%[dir],PRT-A-%[dir]-P                                              )
// RUN:   (mode=MODE_P prt=%[prt-opt]=omp     prt-chk=PRT,PRT-P,PRT-%[dir],PRT-%[dir]-P,PRT-O,PRT-O-P,PRT-O-%[dir],PRT-O-%[dir]-P                                              )
// RUN:   (mode=MODE_P prt=%[prt-opt]=acc-omp prt-chk=PRT,PRT-P,PRT-%[dir],PRT-%[dir]-P,PRT-A,PRT-A-P,PRT-A-%[dir],PRT-A-%[dir]-P,PRT-AO,PRT-AO-P,PRT-AO-%[dir],PRT-AO-%[dir]-P)
// RUN:   (mode=MODE_P prt=%[prt-opt]=omp-acc prt-chk=PRT,PRT-P,PRT-%[dir],PRT-%[dir]-P,PRT-O,PRT-O-P,PRT-O-%[dir],PRT-O-%[dir]-P,PRT-OA,PRT-OA-P,PRT-OA-%[dir],PRT-OA-%[dir]-P)
// RUN: }
// RUN: %for directives {
// RUN:   %for prt-opts {
// RUN:     %for prt-args {
// RUN:       %clang -Xclang -verify %[prt] -DMODE=%[mode] %[dir-cflags] \
// RUN:              %t-acc.c \
// RUN:       | FileCheck -check-prefixes=%[prt-chk] %s
// RUN:     }
// RUN:   }
// RUN: }

// Can we print the OpenMP source code, compile, and run it successfully?
//
// RUN: %data exes {
// RUN:   (mode=MODE_I  cflags=                 pre=EXE-I,EXE-IF,EXE       )
// RUN:   (mode=MODE_I  cflags=-DSTORAGE=static pre=EXE-I,EXE-IF,EXE       )
// RUN:   (mode=MODE_F  cflags=                 pre=EXE-F,EXE-IF,EXE-FP,EXE)
// RUN:   (mode=MODE_F  cflags=-DSTORAGE=static pre=EXE-F,EXE-IF,EXE-FP,EXE)
// RUN:   (mode=MODE_FF cflags=                 pre=EXE-F,EXE-IF,EXE-FP,EXE)
// RUN:   (mode=MODE_P  cflags=                 pre=EXE-P,EXE-FP,EXE       )
// RUN:   (mode=MODE_P  cflags=-DSTORAGE=static pre=EXE-P,EXE-FP,EXE       )
// RUN:   (mode=MODE_PP cflags=                 pre=EXE-P,EXE-FP,EXE       )
// RUN: }
// RUN: %for directives {
// RUN:   %for exes {
// RUN:     %for prt-opts {
// RUN:       %clang -Xclang -verify %[prt-opt]=omp %s > %t-omp.c \
// RUN:              -DMODE=%[mode] %[cflags] %[dir-cflags]
// RUN:       echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN:       %clang -Xclang -verify -fopenmp -o %t %t-omp.c \
// RUN:              -DMODE=%[mode] %[cflags] %[dir-cflags]
// RUN:       %t | FileCheck -check-prefixes=%[pre] %s
// RUN:     }
// RUN:   }
// RUN: }
//
// Check execution with normal compilation.
//
// RUN: %for directives {
// RUN:   %for exes {
// RUN:     %clang -Xclang -verify -fopenacc -o %t -DMODE=%[mode] %[cflags] \
// RUN:            %[dir-cflags] %s
// RUN:     %t | FileCheck -check-prefixes=%[pre] %s
// RUN:   }
// RUN: }

// Check that variables aren't passed to the outlined function in the case of
// the private clause.  However, for acc parallel loop, the private clause is
// on the effective parallel directive not the effective loop directive, so
// sometimes variables are passed to the outlined function, so don't test that
// case.
//
// RUN: %data llvm {
// RUN:   (mode=MODE_P  cflags=                )
// RUN:   (mode=MODE_P  cflags=-DSTORAGE=static)
// RUN:   (mode=MODE_PP cflags=                )
// RUN: }
// RUN: %for llvm {
// RUN:   %clang -Xclang -verify -fopenacc -S -emit-llvm -o %t -DMODE=%[mode] \
// RUN:          %[cflags] %s
// RUN:   cat %t | FileCheck -check-prefixes=LLVM %s
// RUN: }
//
// LLVM: define internal void @.omp_outlined.
// LLVM-SAME: ({{[^,]+}} %.global_tid., {{[^,]+}} %.bound_tid.)

// Check err_typecheck_assign_const, which wasn't reported when SemaExpr.cpp's
// captureInCapturedRegion or Sema::tryCaptureVariable removed type qualifiers
// from captured types.
//
// TODO: This should move to parallel-messages.c (unless it's already there).
// Notice that mode here is important: MODE_P checks that the clause has a
// diagnostic, and MODE_I and MODE_F check that const isn't lost so that
// the normal assign to const diagnostic happens.
//
// RUN: %data constErrs {
// RUN:   (mode=MODE_I cflags=-DGCONST=const verify=gconst)
// RUN:   (mode=MODE_I cflags=-DLCONST=const verify=lconst)
// RUN:   (mode=MODE_F cflags=-DGCONST=const verify=gconst)
// RUN:   (mode=MODE_F cflags=-DLCONST=const verify=lconst)
// RUN:   (mode=MODE_P cflags=-DGCONST=const verify=gconst,gconst-priv)
// RUN:   (mode=MODE_P cflags=-DLCONST=const verify=lconst,lconst-priv)
// RUN: }
// RUN: %for directives {
// RUN:   %for constErrs {
// RUN:     %clang -fopenacc -fsyntax-only -Xclang -verify=%[verify] \
// RUN:            %[cflags] -DMODE=%[mode] %[dir-cflags] %s
// RUN:   }
// RUN: }

// Check err_acc_wrong_dsa.  Whether firstprivate or private is first affects
// the code path, so try both.
//
// TODO: Why are we testing this here?  Isn't it tested in parallel-messages.c?
// Moreover, we should make the PP and FF cases errors (like gcc does) and move
// those to parallel-messages.c as well.
//
// RUN: %data dsaConflicts {
// RUN:   (mode=MODE_FP verify=fp,%[dir]-fp)
// RUN:   (mode=MODE_PF verify=pf,%[dir]-pf)
// RUN: }
// RUN: %for directives {
// RUN:   %for dsaConflicts {
// RUN:     %clang -fopenacc -fsyntax-only -Xclang -verify=%[verify] \
// RUN:            -DMODE=%[mode] %[dir-cflags] %s
// RUN:   }
// RUN: }

// END.

// expected-no-diagnostics
// PARLOOP-fp-no-diagnostics
// PARLOOP-pf-no-diagnostics

#include <stdio.h>
#include <stdint.h>

#if !ADD_LOOP_TO_PAR
# define LOOP
# define FORLOOP_HEAD
#else
# define LOOP loop seq
# define FORLOOP_HEAD for (int i = 0; i < 1; ++i)
#endif

#ifndef GCONST
# define GCONST
#endif
#ifndef LCONST
# define LCONST
#endif

#ifndef STORAGE
# define STORAGE
#endif

#define MODE_I  1
#define MODE_F  2
#define MODE_P  3
#define MODE_FF 4
#define MODE_PP 5
#define MODE_FP 6
#define MODE_PF 7

#ifndef MODE
# error MODE undefined
#elif MODE == MODE_I
# define CLAUSE(...)
#elif MODE == MODE_F
# define CLAUSE(...) firstprivate(__VA_ARGS__)
#elif MODE == MODE_P
# define CLAUSE(...) private(__VA_ARGS__)
#elif MODE == MODE_FF
// Checks that duplicate clauses don't change the behavior.
# define CLAUSE(...) firstprivate(__VA_ARGS__) firstprivate(__VA_ARGS__)
#elif MODE == MODE_PP
// Checks that duplicate clauses don't change the behavior.
# define CLAUSE(...) private(__VA_ARGS__) private(__VA_ARGS__)
#elif MODE == MODE_FP
# define CLAUSE(...) firstprivate(__VA_ARGS__) private(__VA_ARGS__)
#elif MODE == MODE_PF
# define CLAUSE(...) private(__VA_ARGS__) firstprivate(__VA_ARGS__)
#else
# error unknown MODE
#endif

#ifdef __SIZEOF_INT128__
# define HAS_UINT128 1
# define IF_UINT128(...) __VA_ARGS__
# define MK_UINT128(HI,LW) ((((__uint128_t)(HI))<<64) + (LW))
# define HI_UINT128(X) ((uint64_t)((X)>>64))
# define LW_UINT128(X) ((uint64_t)((X)&UINT64_MAX))
#else
# define HAS_UINT128 0
# define IF_UINT128(...)
#endif

// Scalar global, either:
// - implicitly/explicitly firstprivate and either:
//   - < uintptr size, captured by copy
//   - >= uintptr size, captured by ref with data copied
//   - At one time, the last case was captured by copy, and it was truncated
//     into 64 bits as a result. Thus, we exercise both the high and low 64
//     bits to check that all bits are passed through.
// - explicitly private, captured decl only
STORAGE GCONST int gi = 55; // gconst-note {{variable 'gi' declared const here}}
                            // gconst-priv-note@-1 {{variable 'gi' declared const here}}
#if HAS_UINT128
STORAGE __uint128_t gt = MK_UINT128(0x1400, 0x59); // t=tetra integer
#endif
STORAGE const int *gp = &gi;
// Non-scalar global, either:
// - implicitly shared, no capturing
// - explicitly firstprivate, captured by ref with data copied
// - explicitly private, captured decl only
STORAGE int ga[2] = {100,200};
STORAGE struct S {int i; int j;} gs = {33, 11};
STORAGE union U {float f; int i;} gu = {.i=22};

// Unreferenced in region, if explicit firstprivate, used to crash compiler.
STORAGE int gUnref = 2;

int main() {
  // Scalar local: same as for scalar global
  STORAGE LCONST int li = 99; // lconst-note {{variable 'li' declared const here}}
                              // lconst-priv-note@-1 {{variable 'li' declared const here}}
#if HAS_UINT128
  STORAGE __uint128_t lt = MK_UINT128(0x7a1, 0x62b0); // t=tetra integer
#endif
  STORAGE const int *lp = &gi;
  // Non-scalar local, either:
  // - implicitly shared, captured by ref without data copied
  // - explicitly firstprivate, captured by ref with data copied
  // - explicitly private, captured decl only
  STORAGE int la[2] = {77,88};
  STORAGE struct S ls = {222, 111};
  STORAGE union U lu = {.i=167};

  // Unreferenced in region, if explicit firstprivate, used to crash compiler.
  STORAGE int lUnref = 9;

  int giOld = gi;
#if HAS_UINT128
  __uint128_t gtOld = gt;
#endif
  const int *gpOld = gp;
  int gaOld[2] = {ga[0], ga[1]};
  int gUnrefOld = gUnref;
  struct S gsOld = gs;
  union U guOld = gu;

  int liOld = li;
#if HAS_UINT128
  __uint128_t ltOld = lt;
#endif
  const int *lpOld = lp;
  int laOld[2] = {la[0], la[1]};
  struct S lsOld = ls;
  int lUnrefOld = lUnref;
  union U luOld = lu;

  // Implicit firstprivate that is shadowed.
  STORAGE int shadowed = 111;

  // DMP-PAR:            ACCParallelDirective
  // DMP-PARLOOP:        ACCParallelLoopDirective
  // DMP-PARLOOP-NEXT:     ACCSeqClause
  // DMP-NEXT:             ACCNum_gangsClause
  // DMP-NEXT:               IntegerLiteral {{.*}} 'int' 2
  // DMP-PAR-I-NEXT:       ACCSharedClause {{.*}} <implicit>
  // DMP-PAR-I-NEXT:         DeclRefExpr {{.*}} 'ga' 'int [2]'
  // DMP-PAR-I-NEXT:         DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-PAR-I-NEXT:         DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-PAR-I-NEXT:         DeclRefExpr {{.*}} 'la' 'int [2]'
  // DMP-PAR-I-NEXT:         DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-PAR-I-NEXT:         DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-PAR-I-NEXT:       ACCFirstprivateClause {{.*}} <implicit>
  // DMP-PAR-I-NEXT:         DeclRefExpr {{.*}} 'gi' 'int'
  //                         DeclRefExpr for gt is here if defined
  // DMP-PAR-I-NOT:          ACC
  // DMP-PAR-I:              DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-PAR-I-NEXT:         DeclRefExpr {{.*}} 'li' 'int'
  // DMP-PAR-I-NOT:          ACC
  //                         DeclRefExpr for lt is here if defined
  // DMP-PAR-I:              DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-PAR-I-NEXT:         DeclRefExpr {{.*}} 'shadowed' 'int'
  // DMP-F-NEXT:           ACCFirstprivateClause
  // DMP-P-NEXT:           ACCPrivateClause
  // DMP-FP-NOT:             <implicit>
  // DMP-FP-SAME:            {{$}}
  // DMP-FP-NEXT:            DeclRefExpr {{.*}} 'gi' 'int'
  //                         DeclRefExpr for gt is here if defined
  // DMP-FP-NOT:             ACC
  // DMP-FP:                 DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-FP-NEXT:            DeclRefExpr {{.*}} 'ga' 'int [2]'
  // DMP-FP-NEXT:            DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-FP-NEXT:            DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-FP-NEXT:            DeclRefExpr {{.*}} 'gUnref' 'int'
  // DMP-F-NEXT:           ACCFirstprivateClause
  // DMP-P-NEXT:           ACCPrivateClause
  // DMP-FP-NOT:             <implicit>
  // DMP-FP-SAME:            {{$}}
  // DMP-FP-NEXT:            DeclRefExpr {{.*}} 'li' 'int'
  // DMP-FP-NOT:             ACC
  //                         DeclRefExpr for lt is here if defined
  // DMP-FP:                 DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-FP-NEXT:            DeclRefExpr {{.*}} 'la' 'int [2]'
  // DMP-FP-NEXT:            DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-FP-NEXT:            DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-FP-NEXT:            DeclRefExpr {{.*}} 'lUnref' 'int'
  // DMP-F-NEXT:           ACCFirstprivateClause
  // DMP-P-NEXT:           ACCPrivateClause
  // DMP-FP-NOT:             <implicit>
  // DMP-FP-SAME:            {{$}}
  // DMP-FP-NEXT:            DeclRefExpr {{.*}} 'shadowed' 'int'
  // DMP-PARLOOP-NEXT:     effect: ACCParallelDirective
  // DMP-PARLOOP-NEXT:       ACCNum_gangsClause
  // DMP-PARLOOP-NEXT:         IntegerLiteral {{.*}} 'int' 2
  // DMP-PARLOOP-F-NEXT:     ACCFirstprivateClause
  // DMP-PARLOOP-F-NOT:        <implicit>
  // DMP-PARLOOP-F-SAME:       {{$}}
  // DMP-PARLOOP-F-NEXT:       DeclRefExpr {{.*}} 'gi' 'int'
  //                           DeclRefExpr for gt is here if defined
  // DMP-PARLOOP-F-NOT:        ACC
  // DMP-PARLOOP-F:            DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-PARLOOP-F-NEXT:       DeclRefExpr {{.*}} 'ga' 'int [2]'
  // DMP-PARLOOP-F-NEXT:       DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-PARLOOP-F-NEXT:       DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-PARLOOP-F-NEXT:       DeclRefExpr {{.*}} 'gUnref' 'int'
  // DMP-PARLOOP-F-NEXT:     ACCFirstprivateClause
  // DMP-PARLOOP-F-NOT:        <implicit>
  // DMP-PARLOOP-F-SAME:       {{$}}
  // DMP-PARLOOP-F-NEXT:       DeclRefExpr {{.*}} 'li' 'int'
  // DMP-PARLOOP-F-NOT:        ACC
  //                           DeclRefExpr for lt is here if defined
  // DMP-PARLOOP-F:            DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-PARLOOP-F-NEXT:       DeclRefExpr {{.*}} 'la' 'int [2]'
  // DMP-PARLOOP-F-NEXT:       DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-PARLOOP-F-NEXT:       DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-PARLOOP-F-NEXT:       DeclRefExpr {{.*}} 'lUnref' 'int'
  // DMP-PARLOOP-F-NEXT:     ACCFirstprivateClause
  // DMP-PARLOOP-F-NOT:        <implicit>
  // DMP-PARLOOP-F-SAME:       {{$}}
  // DMP-PARLOOP-F-NEXT:       DeclRefExpr {{.*}} 'shadowed' 'int'
  // DMP-PARLOOP-IP-NEXT:    ACCSharedClause {{.*}} <implicit>
  // DMP-PARLOOP-IP-NEXT:      DeclRefExpr {{.*}} 'ga' 'int [2]'
  // DMP-PARLOOP-IP-NEXT:      DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-PARLOOP-IP-NEXT:      DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-PARLOOP-IP-NEXT:      DeclRefExpr {{.*}} 'la' 'int [2]'
  // DMP-PARLOOP-IP-NEXT:      DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-PARLOOP-IP-NEXT:      DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-PARLOOP-IP-NEXT:    ACCFirstprivateClause {{.*}} <implicit>
  // DMP-PARLOOP-IP-NEXT:      DeclRefExpr {{.*}} 'gi' 'int'
  //                           DeclRefExpr for gt is here if defined
  // DMP-PARLOOP-IP-NOT:       ACC
  // DMP-PARLOOP-IP:           DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-PARLOOP-IP-NEXT:      DeclRefExpr {{.*}} 'li' 'int'
  // DMP-PARLOOP-IP-NOT:       ACC
  //                           DeclRefExpr for lt is here if defined
  // DMP-PARLOOP-IP:           DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-PARLOOP-IP-NEXT:      DeclRefExpr {{.*}} 'shadowed' 'int'
  // DMP-NEXT:               impl: OMPTargetTeamsDirective
  // DMP-NEXT:                 OMPNum_teamsClause
  // DMP-NEXT:                   IntegerLiteral {{.*}} 'int' 2
  // DMP-PARLOOP-F-NEXT:       OMPFirstprivateClause
  // DMP-PARLOOP-F-NOT:          <implicit>
  // DMP-PARLOOP-F-SAME:         {{$}}
  // DMP-PARLOOP-F-NEXT:         DeclRefExpr {{.*}} 'gi' 'int'
  //                             DeclRefExpr for gt is here if defined
  // DMP-PARLOOP-F-NOT:          OMP
  // DMP-PARLOOP-F:              DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-PARLOOP-F-NEXT:         DeclRefExpr {{.*}} 'ga' 'int [2]'
  // DMP-PARLOOP-F-NEXT:         DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-PARLOOP-F-NEXT:         DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-PARLOOP-F-NEXT:         DeclRefExpr {{.*}} 'gUnref' 'int'
  // DMP-PARLOOP-F-NEXT:       OMPFirstprivateClause
  // DMP-PARLOOP-F-NOT:          <implicit>
  // DMP-PARLOOP-F-SAME:         {{$}}
  // DMP-PARLOOP-F-NEXT:         DeclRefExpr {{.*}} 'li' 'int'
  // DMP-PARLOOP-F-NOT:          OMP
  //                             DeclRefExpr for lt is here if defined
  // DMP-PARLOOP-F:              DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-PARLOOP-F-NEXT:         DeclRefExpr {{.*}} 'la' 'int [2]'
  // DMP-PARLOOP-F-NEXT:         DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-PARLOOP-F-NEXT:         DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-PARLOOP-F-NEXT:         DeclRefExpr {{.*}} 'lUnref' 'int'
  // DMP-PARLOOP-F-NEXT:       OMPFirstprivateClause
  // DMP-PARLOOP-F-NOT:          <implicit>
  // DMP-PARLOOP-F-SAME:         {{$}}
  // DMP-PARLOOP-F-NEXT:         DeclRefExpr {{.*}} 'shadowed' 'int'
  // DMP-PARLOOP-IP-NEXT:      OMPSharedClause
  // DMP-PARLOOP-IP-NOT:         <implicit>
  // DMP-PARLOOP-IP-SAME:        {{$}}
  // DMP-PARLOOP-IP-NEXT:        DeclRefExpr {{.*}} 'ga' 'int [2]'
  // DMP-PARLOOP-IP-NEXT:        DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-PARLOOP-IP-NEXT:        DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-PARLOOP-IP-NEXT:        DeclRefExpr {{.*}} 'la' 'int [2]'
  // DMP-PARLOOP-IP-NEXT:        DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-PARLOOP-IP-NEXT:        DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-PARLOOP-IP-NEXT:      OMPFirstprivateClause
  // DMP-PARLOOP-IP-NOT:         <implicit>
  // DMP-PARLOOP-IP-SAME:        {{$}}
  // DMP-PARLOOP-IP-NEXT:        DeclRefExpr {{.*}} 'gi' 'int'
  //                             DeclRefExpr for gt is here if defined
  // DMP-PARLOOP-IP-NOT:         OMP
  // DMP-PARLOOP-IP:             DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-PARLOOP-IP-NEXT:        DeclRefExpr {{.*}} 'li' 'int'
  // DMP-PARLOOP-IP-NOT:         OMP
  //                             DeclRefExpr for lt is here if defined
  // DMP-PARLOOP-IP:             DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-PARLOOP-IP-NEXT:        DeclRefExpr {{.*}} 'shadowed' 'int'
  // DMP-PARLOOP:            ACCLoopDirective
  // DMP-PARLOOP-NEXT:         ACCSeqClause
  // DMP-PARLOOP-P-NEXT:       ACCPrivateClause
  // DMP-PARLOOP-P-NOT:          <implicit>
  // DMP-PARLOOP-P-SAME:         {{$}}
  // DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'gi' 'int'
  //                             DeclRefExpr for gt is here if defined
  // DMP-PARLOOP-P-NOT:          ACC
  // DMP-PARLOOP-P:              DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'ga' 'int [2]'
  // DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'gUnref' 'int'
  // DMP-PARLOOP-P-NEXT:       ACCPrivateClause
  // DMP-PARLOOP-P-NOT:          <implicit>
  // DMP-PARLOOP-P-SAME:         {{$}}
  // DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'li' 'int'
  // DMP-PARLOOP-P-NOT:          ACC
  //                             DeclRefExpr for lt is here if defined
  // DMP-PARLOOP-P:              DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'la' 'int [2]'
  // DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'lUnref' 'int'
  // DMP-PARLOOP-P-NEXT:       ACCPrivateClause
  // DMP-PARLOOP-P-NOT:          <implicit>
  // DMP-PARLOOP-P-SAME:         {{$}}
  // DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'shadowed' 'int'
  // DMP-PARLOOP-IF-NEXT:      ACCSharedClause {{.*}} <implicit>
  // DMP-PARLOOP-IF-NEXT:        DeclRefExpr {{.*}} 'gi' 'int'
  //                             DeclRefExpr for gt is here if defined
  // DMP-PARLOOP-IF-NOT:         ACC
  // DMP-PARLOOP-IF:             DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-PARLOOP-IF-NEXT:        DeclRefExpr {{.*}} 'ga' 'int [2]'
  // DMP-PARLOOP-IF-NEXT:        DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-PARLOOP-IF-NEXT:        DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-PARLOOP-IF-NEXT:        DeclRefExpr {{.*}} 'li' 'int'
  // DMP-PARLOOP-IF-NOT:         ACC
  //                             DeclRefExpr for lt is here if defined
  // DMP-PARLOOP-IF:             DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-PARLOOP-IF-NEXT:        DeclRefExpr {{.*}} 'la' 'int [2]'
  // DMP-PARLOOP-IF-NEXT:        DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-PARLOOP-IF-NEXT:        DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-PARLOOP-IF-NEXT:        DeclRefExpr {{.*}} 'shadowed' 'int'
  // DMP-PARLOOP-IF-NEXT:      impl: ForStmt
  // DMP-PARLOOP-P-NEXT:       impl: CompoundStmt
  // DMP-PARLOOP-P-NEXT:         DeclStmt
  // DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} gi 'int'
  // DMP-PARLOOP-P-NEXT:         DeclStmt
  //                               VarDecl for gt is here if defined
  // DMP-PARLOOP-P:                VarDecl {{.*}} gp 'const int *'
  // DMP-PARLOOP-P-NEXT:         DeclStmt
  // DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} ga 'int [2]'
  // DMP-PARLOOP-P-NEXT:         DeclStmt
  // DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} gs 'struct S':'struct S'
  // DMP-PARLOOP-P-NEXT:         DeclStmt
  // DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} gu 'union U':'union U'
  // DMP-PARLOOP-P-NEXT:         DeclStmt
  // DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} gUnref 'int'
  // DMP-PARLOOP-P-NEXT:         DeclStmt
  // DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} li 'int'
  // DMP-PARLOOP-P-NEXT:         DeclStmt
  //                               VarDecl for lt is here if defined
  // DMP-PARLOOP-P:                VarDecl {{.*}} lp 'const int *'
  // DMP-PARLOOP-P-NEXT:         DeclStmt
  // DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} la 'int [2]'
  // DMP-PARLOOP-P-NEXT:         DeclStmt
  // DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} ls 'struct S':'struct S'
  // DMP-PARLOOP-P-NEXT:         DeclStmt
  // DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} lu 'union U':'union U'
  // DMP-PARLOOP-P-NEXT:         DeclStmt
  // DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} lUnref 'int'
  // DMP-PARLOOP-P-NEXT:         DeclStmt
  // DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} shadowed 'int'
  // DMP-PARLOOP-P-NEXT:         ForStmt
  //
  // PRT-NOT:       #pragma
  //
  // PRT-A-PAR-I:           {{^ *}}#pragma acc parallel{{ LOOP | }}num_gangs(2){{(.*\\$[[:space:]])*.*$}}
  // PRT-A-PARLOOP-I:       {{^ *}}#pragma acc parallel {{LOOP|loop seq}} num_gangs(2){{(.*\\$[[:space:]])*.*$}}
  // PRT-AO-I-NEXT:         {{^ *}}// #pragma omp target teams num_teams(2) shared(ga,gs,gu,la,ls,lu) firstprivate(gi,{{(gt,)?}}gp,li,{{(lt,)?}}lp,shadowed){{$}}
  // PRT-O-I:               {{^ *}}#pragma omp target teams num_teams(2) shared(ga,gs,gu,la,ls,lu) firstprivate(gi,{{(gt,)?}}gp,li,{{(lt,)?}}lp,shadowed){{$}}
  // PRT-OA-PAR-I-NEXT:     {{^ *}}// #pragma acc parallel{{ LOOP | }}num_gangs(2){{(.*\\$[[:space:]])*.*$}}
  // PRT-OA-PARLOOP-I-NEXT: {{^ *}}// #pragma acc parallel {{LOOP|loop seq}} num_gangs(2){{(.*\\$[[:space:]])*.*$}}
  //
  // PRT-A-PAR-F:           {{^ *}}#pragma acc parallel{{ LOOP | }}num_gangs(2) {{firstprivate\(gi,(gt,)?gp,ga,gs,gu,gUnref\) firstprivate\(li,(lt,)?lp,la,ls,lu,lUnref\) firstprivate\(shadowed\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-A-PARLOOP-F:       {{^ *}}#pragma acc parallel {{LOOP|loop seq}} num_gangs(2) {{firstprivate\(gi,(gt,)?gp,ga,gs,gu,gUnref\) firstprivate\(li,(lt,)?lp,la,ls,lu,lUnref\) firstprivate\(shadowed\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-AO-F-NEXT:         {{^ *}}// #pragma omp target teams num_teams(2) firstprivate(gi,{{(gt,)?}}gp,ga,gs,gu,gUnref) firstprivate(li,{{(lt,)?}}lp,la,ls,lu,lUnref) firstprivate(shadowed){{$}}
  // PRT-O-F:               {{^ *}}#pragma omp target teams num_teams(2) firstprivate(gi,{{(gt,)?}}gp,ga,gs,gu,gUnref) firstprivate(li,{{(lt,)?}}lp,la,ls,lu,lUnref) firstprivate(shadowed){{$}}
  // PRT-OA-PAR-F-NEXT:     {{^ *}}// #pragma acc parallel{{ LOOP | }}num_gangs(2) {{firstprivate\(gi,(gt,)?gp,ga,gs,gu,gUnref\) firstprivate\(li,(lt,)?lp,la,ls,lu,lUnref\) firstprivate\(shadowed\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-OA-PARLOOP-F-NEXT: {{^ *}}// #pragma acc parallel {{LOOP|loop seq}} num_gangs(2) {{firstprivate\(gi,(gt,)?gp,ga,gs,gu,gUnref\) firstprivate\(li,(lt,)?lp,la,ls,lu,lUnref\) firstprivate\(shadowed\)$|(.*\\$[[:space:]])+.*$}}
  //
  // PRT-A-PAR-P:           {{^ *}}#pragma acc parallel{{ LOOP | }}num_gangs(2) {{private\(gi,(gt,)?gp,ga,gs,gu,gUnref\) private\(li,(lt,)?lp,la,ls,lu,lUnref\) private\(shadowed\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-AO-PAR-P-NEXT:     {{^ *}}// #pragma omp target teams num_teams(2) private(gi,{{(gt,)?}}gp,ga,gs,gu,gUnref) private(li,{{(lt,)?}}lp,la,ls,lu,lUnref) private(shadowed){{$}}
  // PRT-A-PARLOOP-P:       {{^ *}}#pragma acc parallel {{LOOP|loop seq}} num_gangs(2) {{private\(gi,(gt,)?gp,ga,gs,gu,gUnref\) private\(li,(lt,)?lp,la,ls,lu,lUnref\) private\(shadowed\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-AO-PARLOOP-P-NOT:  #pragma
  // PRT-AO-PARLOOP-P:      {{^ *}}// #pragma omp target teams num_teams(2) shared(ga,gs,gu,la,ls,lu) firstprivate(gi,{{(gt,)?}}gp,li,{{(lt,)?}}lp,shadowed){{$}}
  // PRT-AO-PARLOOP-P-NEXT: //{{ *}}{
  // PRT-AO-PARLOOP-P-NEXT: //{{ *}}  int gi;
  //                        //{{ *}}  int gt; is here if defined
  // PRT-AO-PARLOOP-P:      //{{ *}}  const int *gp;
  // PRT-AO-PARLOOP-P-NEXT: //{{ *}}  int ga[2];
  // PRT-AO-PARLOOP-P-NEXT: //{{ *}}  struct S gs;
  // PRT-AO-PARLOOP-P-NEXT: //{{ *}}  union U gu;
  // PRT-AO-PARLOOP-P-NEXT: //{{ *}}  int gUnref;
  // PRT-AO-PARLOOP-P-NEXT: //{{ *}}  int li;
  //                        //{{ *}}  int lt; is here if defined
  // PRT-AO-PARLOOP-P:      //{{ *}}  const int *lp;
  // PRT-AO-PARLOOP-P-NEXT: //{{ *}}  int la[2];
  // PRT-AO-PARLOOP-P-NEXT: //{{ *}}  struct S ls;
  // PRT-AO-PARLOOP-P-NEXT: //{{ *}}  union U lu;
  // PRT-AO-PARLOOP-P-NEXT: //{{ *}}  int lUnref;
  // PRT-AO-PARLOOP-P-NEXT: //{{ *}}  int shadowed;
  // PRT-O-PAR-P:           {{^ *}}#pragma omp target teams num_teams(2) private(gi,{{(gt,)?}}gp,ga,gs,gu,gUnref) private(li,{{(lt,)?}}lp,la,ls,lu,lUnref) private(shadowed){{$}}
  // PRT-OA-PAR-P-NEXT:     {{^ *}}// #pragma acc parallel{{ LOOP | }}num_gangs(2) {{private\(gi,(gt,)?gp,ga,gs,gu,gUnref\) private\(li,(lt,)?lp,la,ls,lu,lUnref\) private\(shadowed\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-O-PARLOOP-P:       {{^ *}}#pragma omp target teams num_teams(2) shared(ga,gs,gu,la,ls,lu) firstprivate(gi,{{(gt,)?}}gp,li,{{(lt,)?}}lp,shadowed){{$}}
  // PRT-O-PARLOOP-P-NEXT:  {
  // PRT-O-PARLOOP-P-NEXT:    int gi;
  //                          int gt; is here if defined
  // PRT-O-PARLOOP-P:         int *gp;
  // PRT-O-PARLOOP-P-NEXT:    int ga[2];
  // PRT-O-PARLOOP-P-NEXT:    struct S gs;
  // PRT-O-PARLOOP-P-NEXT:    union U gu;
  // PRT-O-PARLOOP-P-NEXT:    int gUnref;
  // PRT-O-PARLOOP-P-NEXT:    int li;
  //                          int lt; is here if defined
  // PRT-O-PARLOOP-P:         int *lp;
  // PRT-O-PARLOOP-P-NEXT:    int la[2];
  // PRT-O-PARLOOP-P-NEXT:    struct S ls;
  // PRT-O-PARLOOP-P-NEXT:    union U lu;
  // PRT-O-PARLOOP-P-NEXT:    int lUnref;
  // PRT-O-PARLOOP-P-NEXT:    int shadowed;
  // PRT-OA-PARLOOP-P-NOT:  #pragma
  // PRT-OA-PARLOOP-P:      {{^ *}}// #pragma acc parallel {{LOOP|loop seq}} num_gangs(2) {{private\(gi,(gt,)?gp,ga,gs,gu,gUnref\) private\(li,(lt,)?lp,la,ls,lu,lUnref\) private\(shadowed\)$|(.*\\$[[:space:]])+.*$}}
  //
  // PRT-NOT:      #pragma
  //
  // PAR-fp-error@+17 6-7 {{firstprivate variable cannot be private}}
  // PAR-fp-note@+16  6-7 {{defined as firstprivate}}
  // PAR-fp-error@+16 6-7 {{firstprivate variable cannot be private}}
  // PAR-fp-note@+15  6-7 {{defined as firstprivate}}
  // PAR-fp-error@+15 1   {{firstprivate variable cannot be private}}
  // PAR-fp-note@+14  1   {{defined as firstprivate}}
  //
  // PAR-pf-error@+10 6-7 {{private variable cannot be firstprivate}}
  // PAR-pf-note@+9   6-7 {{defined as private}}
  // PAR-pf-error@+9  6-7 {{private variable cannot be firstprivate}}
  // PAR-pf-note@+8   6-7 {{defined as private}}
  // PAR-pf-error@+8  1   {{private variable cannot be firstprivate}}
  // PAR-pf-note@+7   1   {{defined as private}}
  //
  // gconst-priv-error@+3 {{const variable cannot be private because initialization is impossible}}
  // lconst-priv-error@+3 {{const variable cannot be private because initialization is impossible}}
  #pragma acc parallel LOOP num_gangs(2)                       \
    CLAUSE(gi,IF_UINT128(gt,)gp, ga,gs , gu , gUnref )         \
    CLAUSE(  li,IF_UINT128(lt,)lp  ,  la  ,ls,  lu , lUnref  ) \
    CLAUSE(shadowed)
  // PRT-PARLOOP-NEXT: {{for (.*) [{]|FORLOOP_HEAD}}
  // PRT-PAR-NEXT: {{(FORLOOP_HEAD$[[:space:]])? *}}{
  FORLOOP_HEAD
  {
    // Read before writing in order to exercise SemaExpr.cpp's
    // isVariableAlreadyCapturedInScopeInfo's code that suppresses adding const
    // to the captured variable in the case of OpenACC.
    int giOld = gi;
#if HAS_UINT128
    __uint128_t gtOld = gt;
#endif
    // Check a decl with no init: it used to break our transform code.
    const int *gpOld;
    gpOld = gp;
    int gaOld[2] = {ga[0], ga[1]};
    struct S gsOld = gs;
    union U guOld = gu;

    int liOld = li;
#if HAS_UINT128
    __uint128_t ltOld = lt;
#endif
    const int *lpOld = lp;
    int laOld[2] = {la[0], la[1]};
    struct S lsOld = ls;
    union U luOld = lu;

    int tmp = shadowed;
    int shadowed = tmp + 111;

    gi = 56; // gconst-error {{cannot assign to variable 'gi' with const-qualified type 'const int'}})
#if HAS_UINT128
    gt = MK_UINT128(0xf08234, 0xa07de1);
#endif
    gp = &li;
    ga[0] = 101;
    ga[1] = 202;
    gs.i = 42;
    gs.j = 1;
    gu.i = 13;

    li = 98; // lconst-error {{cannot assign to variable 'li' with const-qualified type 'const int'}})
#if HAS_UINT128
    lt = MK_UINT128(0x79ca, 0x2961);
#endif
    lp = &li;
    la[0] = 55;
    la[1] = 66;
    ls.i = 333;
    ls.j = 444;
    lu.i = 279;

    shadowed += 111;

#if MODE == MODE_P || MODE == MODE_PP
    // don't dereference uninitialized pointers
    gpOld = gp;
    lpOld = lp;
#endif
    // PRT:      printf("inside :{{([^;]|[[:space:]])*}};
    //
    // EXE-IF:      inside : gi:55->56,{{( gt:[1400,59]->[f08234,a07de1],)?}}
    // EXE-P:       inside : gi:{{.+}}->56,{{( gt:[.+]->[f08234,a07de1],)?}}
    // EXE-I-NEXT:           *gp:55->98, ga:[{{100|101}},{{200|202}}]->[101,202],
    // EXE-F-NEXT:           *gp:55->98, ga:[100,200]->[101,202],
    // EXE-P-NEXT:           *gp:{{.+}}->98, ga:[{{.+,.+}}]->[101,202],
    // EXE-I-NEXT:           gs:[{{33|42}},{{11|1}}]->[42,1], gu.i:{{22|13}}->13,
    // EXE-F-NEXT:           gs:[33,11]->[42,1], gu.i:22->13,
    // EXE-P-NEXT:           gs:[{{.+,.+}}]->[42,1], gu.i:{{.+}}->13,
    // EXE-IF-NEXT:          li:99->98,{{( lt:[7a1,62b0]->[79ca,2961],)?}}
    // EXE-P-NEXT:           li:{{.+}}->98,{{( lt:[.+]->[f08234,a07de1],)?}}
    // EXE-I-NEXT:           *lp:55->98, la:[{{77|55}},{{88|66}}]->[55,66],
    // EXE-F-NEXT:           *lp:55->98, la:[77,88]->[55,66],
    // EXE-P-NEXT:           *lp:{{.+}}->98, la:[{{.+,.+}}]->[55,66],
    // EXE-I-NEXT:           ls:[{{222|333}},{{111|444}}]->[333,444], lu.i:{{167|279}}->279,
    // EXE-F-NEXT:           ls:[222,111]->[333,444], lu.i:167->279,
    // EXE-P-NEXT:           ls:[{{.+,.+}}]->[333,444], lu.i:{{.+}}->279,
    // EXE-NEXT:             shadowed:
    //
    // Duplicate that EXE block exactly (plus -NEXT in the leading lines).
    //
    // EXE-IF-NEXT: inside : gi:55->56,{{( gt:[1400,59]->[f08234,a07de1],)?}}
    // EXE-P-NEXT:  inside : gi:{{.+}}->56,{{( gt:[.+]->[f08234,a07de1],)?}}
    // EXE-I-NEXT:           *gp:55->98, ga:[{{100|101}},{{200|202}}]->[101,202],
    // EXE-F-NEXT:           *gp:55->98, ga:[100,200]->[101,202],
    // EXE-P-NEXT:           *gp:{{.+}}->98, ga:[{{.+,.+}}]->[101,202],
    // EXE-I-NEXT:           gs:[{{33|42}},{{11|1}}]->[42,1], gu.i:{{22|13}}->13,
    // EXE-F-NEXT:           gs:[33,11]->[42,1], gu.i:22->13,
    // EXE-P-NEXT:           gs:[{{.+,.+}}]->[42,1], gu.i:{{.+}}->13,
    // EXE-IF-NEXT:          li:99->98,{{( lt:[7a1,62b0]->[79ca,2961],)?}}
    // EXE-P-NEXT:           li:{{.+}}->98,{{( lt:[.+]->[f08234,a07de1],)?}}
    // EXE-I-NEXT:           *lp:55->98, la:[{{77|55}},{{88|66}}]->[55,66],
    // EXE-F-NEXT:           *lp:55->98, la:[77,88]->[55,66],
    // EXE-P-NEXT:           *lp:{{.+}}->98, la:[{{.+,.+}}]->[55,66],
    // EXE-I-NEXT:           ls:[{{222|333}},{{111|444}}]->[333,444], lu.i:{{167|279}}->279,
    // EXE-F-NEXT:           ls:[222,111]->[333,444], lu.i:167->279,
    // EXE-P-NEXT:           ls:[{{.+,.+}}]->[333,444], lu.i:{{.+}}->279,
    // EXE-NEXT:             shadowed:
    printf("inside : gi:%d->%d,"IF_UINT128(" gt:[%lx,%lx]->[%lx,%lx],")"\n"
           "         *gp:%d->%d, ga:[%d,%d]->[%d,%d],\n"
           "         gs:[%d,%d]->[%d,%d], gu.i:%d->%d,\n"
           "         li:%d->%d,"IF_UINT128(" lt:[%lx,%lx]->[%lx,%lx],")"\n"
           "         *lp:%d->%d, la:[%d,%d]->[%d,%d],\n"
           "         ls:[%d,%d]->[%d,%d], lu.i:%d->%d,\n"
           "         shadowed:%d\n",
           giOld, gi,
#if HAS_UINT128
           HI_UINT128(gtOld), LW_UINT128(gtOld),
           HI_UINT128(gt), LW_UINT128(gt),
#endif
           *gpOld, *gp, gaOld[0], gaOld[1], ga[0], ga[1],
           gsOld.i, gsOld.j, gs.i, gs.j, guOld.i, gu.i,
           liOld, li,
#if HAS_UINT128
           HI_UINT128(ltOld), LW_UINT128(ltOld),
           HI_UINT128(lt), LW_UINT128(lt),
#endif
           *lpOld, *lp, laOld[0], laOld[1], la[0], la[1],
           lsOld.i, lsOld.j, ls.i, ls.j, luOld.i, lu.i,
           shadowed);
  } // PRT-NEXT: }
  // DMP: CallExpr
  //
  // EXE-NEXT:    outside: gi:55->55,{{( gt:[1400,59]->[1400,59],)?}}
  // EXE-I-NEXT:           *gp:55->55, ga:[100,200]->[101,202],
  // EXE-FP-NEXT:          *gp:55->55, ga:[100,200]->[100,200],
  // EXE-I-NEXT:           gs:[33,11]->[42,1], gu.i:22->13, gUnref:2->2,
  // EXE-FP-NEXT:          gs:[33,11]->[33,11], gu.i:22->22, gUnref:2->2,
  // EXE-NEXT:             li:99->99,{{( lt:[7a1,62b0]->[7a1,62b0],)?}}
  // EXE-I-NEXT:           *lp:55->55, la:[77,88]->[55,66],
  // EXE-FP-NEXT:          *lp:55->55, la:[77,88]->[77,88],
  // EXE-I-NEXT:           ls:[222,111]->[333,444], lu.i:167->279, lUnref:9->9,
  // EXE-FP-NEXT:          ls:[222,111]->[222,111], lu.i:167->167, lUnref:9->9,
  // EXE-NEXT:             shadowed:111
  printf("outside: gi:%d->%d,"IF_UINT128(" gt:[%lx,%lx]->[%lx,%lx],")"\n"
         "         *gp:%d->%d, ga:[%d,%d]->[%d,%d],\n"
         "         gs:[%d,%d]->[%d,%d], gu.i:%d->%d, gUnref:%d->%d,\n"
         "         li:%d->%d,"IF_UINT128(" lt:[%lx,%lx]->[%lx,%lx],")"\n"
         "         *lp:%d->%d, la:[%d,%d]->[%d,%d],\n"
         "         ls:[%d,%d]->[%d,%d], lu.i:%d->%d, lUnref:%d->%d,\n"
         "         shadowed:%d\n",
         giOld, gi,
#if HAS_UINT128
         HI_UINT128(gtOld), LW_UINT128(gtOld),
         HI_UINT128(gt), LW_UINT128(gt),
#endif
         *gpOld, *gp, gaOld[0], gaOld[1], ga[0], ga[1],
         gsOld.i, gsOld.j, gs.i, gs.j, guOld.i, gu.i, gUnrefOld, gUnref,
         liOld, li,
#if HAS_UINT128
         HI_UINT128(ltOld), LW_UINT128(ltOld),
         HI_UINT128(lt), LW_UINT128(lt),
#endif
         *lpOld, *lp, laOld[0], laOld[1], la[0], la[1],
         lsOld.i, lsOld.j, ls.i, ls.j, luOld.i, lu.i, lUnrefOld, lUnref,
         shadowed);

  return 0;
}
// EXE-NOT:{{.}}

