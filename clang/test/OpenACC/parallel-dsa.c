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
// RUN:   (mode=MODE_I  pre=DMP,DMP-I,DMP-IC,DMP-IF,DMP-IP,DMP-ICF,DMP-%[dir],DMP-%[dir]-I,DMP-%[dir]-IC,DMP-%[dir]-IF,DMP-%[dir]-IP,DMP-%[dir]-ICF)
// RUN:   (mode=MODE_C0 pre=DMP,DMP-C,DMP-IC,DMP-CF,DMP-CP,DMP-ICF,DMP-CFP,DMP-%[dir],DMP-%[dir]-C,DMP-%[dir]-IC,DMP-%[dir]-CF,DMP-%[dir]-CP,DMP-%[dir]-ICF,DMP-%[dir]-CFP)
// RUN:   (mode=MODE_C1 pre=DMP,DMP-C,DMP-IC,DMP-CF,DMP-CP,DMP-ICF,DMP-CFP,DMP-%[dir],DMP-%[dir]-C,DMP-%[dir]-IC,DMP-%[dir]-CF,DMP-%[dir]-CP,DMP-%[dir]-ICF,DMP-%[dir]-CFP)
// RUN:   (mode=MODE_C2 pre=DMP,DMP-C,DMP-IC,DMP-CF,DMP-CP,DMP-ICF,DMP-CFP,DMP-%[dir],DMP-%[dir]-C,DMP-%[dir]-IC,DMP-%[dir]-CF,DMP-%[dir]-CP,DMP-%[dir]-ICF,DMP-%[dir]-CFP)
// RUN:   (mode=MODE_F  pre=DMP,DMP-F,DMP-IF,DMP-CF,DMP-FP,DMP-ICF,DMP-CFP,DMP-%[dir],DMP-%[dir]-F,DMP-%[dir]-IF,DMP-%[dir]-CF,DMP-%[dir]-FP,DMP-%[dir]-ICF,DMP-%[dir]-CFP)
// RUN:   (mode=MODE_P  pre=DMP,DMP-P,DMP-IP,DMP-CP,DMP-FP,DMP-CFP,DMP-%[dir],DMP-%[dir]-P,DMP-%[dir]-IP,DMP-%[dir]-CP,DMP-%[dir]-FP,DMP-%[dir]-CFP)
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
// RUN:   (mode=MODE_I  pre=PRT,PRT-I,PRT-%[dir],PRT-%[dir]-I,PRT-A,PRT-A-I,PRT-A-%[dir],PRT-A-%[dir]-I)
// RUN:   (mode=MODE_C0 pre=PRT,PRT-C0,PRT-%[dir],PRT-%[dir]-C0,PRT-A,PRT-A-C0,PRT-A-%[dir],PRT-A-%[dir]-C0)
// RUN:   (mode=MODE_C1 pre=PRT,PRT-C1,PRT-%[dir],PRT-%[dir]-C1,PRT-A,PRT-A-C1,PRT-A-%[dir],PRT-A-%[dir]-C1)
// RUN:   (mode=MODE_C2 pre=PRT,PRT-C2,PRT-%[dir],PRT-%[dir]-C2,PRT-A,PRT-A-C2,PRT-A-%[dir],PRT-A-%[dir]-C2)
// RUN:   (mode=MODE_F  pre=PRT,PRT-F,PRT-%[dir],PRT-%[dir]-F,PRT-A,PRT-A-F,PRT-A-%[dir],PRT-A-%[dir]-F)
// RUN:   (mode=MODE_P  pre=PRT,PRT-P,PRT-%[dir],PRT-%[dir]-P,PRT-A,PRT-A-P,PRT-A-%[dir],PRT-A-%[dir]-P)
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
// RUN:   (mode=MODE_C0 prt=%[prt-opt]=acc     prt-chk=PRT,PRT-C0,PRT-%[dir],PRT-%[dir]-C0,PRT-A,PRT-A-C0,PRT-A-%[dir],PRT-A-%[dir]-C0                                                )
// RUN:   (mode=MODE_C0 prt=%[prt-opt]=omp     prt-chk=PRT,PRT-C0,PRT-%[dir],PRT-%[dir]-C0,PRT-O,PRT-O-C0,PRT-O-%[dir],PRT-O-%[dir]-C0                                                )
// RUN:   (mode=MODE_C0 prt=%[prt-opt]=acc-omp prt-chk=PRT,PRT-C0,PRT-%[dir],PRT-%[dir]-C0,PRT-A,PRT-A-C0,PRT-A-%[dir],PRT-A-%[dir]-C0,PRT-AO,PRT-AO-C0,PRT-AO-%[dir],PRT-AO-%[dir]-C0)
// RUN:   (mode=MODE_C0 prt=%[prt-opt]=omp-acc prt-chk=PRT,PRT-C0,PRT-%[dir],PRT-%[dir]-C0,PRT-O,PRT-O-C0,PRT-O-%[dir],PRT-O-%[dir]-C0,PRT-OA,PRT-OA-C0,PRT-OA-%[dir],PRT-OA-%[dir]-C0)
//
// RUN:   (mode=MODE_C1 prt=%[prt-opt]=acc     prt-chk=PRT,PRT-C1,PRT-%[dir],PRT-%[dir]-C1,PRT-A,PRT-A-C1,PRT-A-%[dir],PRT-A-%[dir]-C1                                                )
// RUN:   (mode=MODE_C1 prt=%[prt-opt]=omp     prt-chk=PRT,PRT-C1,PRT-%[dir],PRT-%[dir]-C1,PRT-O,PRT-O-C1,PRT-O-%[dir],PRT-O-%[dir]-C1                                                )
// RUN:   (mode=MODE_C1 prt=%[prt-opt]=acc-omp prt-chk=PRT,PRT-C1,PRT-%[dir],PRT-%[dir]-C1,PRT-A,PRT-A-C1,PRT-A-%[dir],PRT-A-%[dir]-C1,PRT-AO,PRT-AO-C1,PRT-AO-%[dir],PRT-AO-%[dir]-C1)
// RUN:   (mode=MODE_C1 prt=%[prt-opt]=omp-acc prt-chk=PRT,PRT-C1,PRT-%[dir],PRT-%[dir]-C1,PRT-O,PRT-O-C1,PRT-O-%[dir],PRT-O-%[dir]-C1,PRT-OA,PRT-OA-C1,PRT-OA-%[dir],PRT-OA-%[dir]-C1)
//
// RUN:   (mode=MODE_C2 prt=%[prt-opt]=acc     prt-chk=PRT,PRT-C2,PRT-%[dir],PRT-%[dir]-C2,PRT-A,PRT-A-C2,PRT-A-%[dir],PRT-A-%[dir]-C2                                                )
// RUN:   (mode=MODE_C2 prt=%[prt-opt]=omp     prt-chk=PRT,PRT-C2,PRT-%[dir],PRT-%[dir]-C2,PRT-O,PRT-O-C2,PRT-O-%[dir],PRT-O-%[dir]-C2                                                )
// RUN:   (mode=MODE_C2 prt=%[prt-opt]=acc-omp prt-chk=PRT,PRT-C2,PRT-%[dir],PRT-%[dir]-C2,PRT-A,PRT-A-C2,PRT-A-%[dir],PRT-A-%[dir]-C2,PRT-AO,PRT-AO-C2,PRT-AO-%[dir],PRT-AO-%[dir]-C2)
// RUN:   (mode=MODE_C2 prt=%[prt-opt]=omp-acc prt-chk=PRT,PRT-C2,PRT-%[dir],PRT-%[dir]-C2,PRT-O,PRT-O-C2,PRT-O-%[dir],PRT-O-%[dir]-C2,PRT-OA,PRT-OA-C2,PRT-OA-%[dir],PRT-OA-%[dir]-C2)
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
// RUN:   (mode=MODE_I  cflags=                 pre=EXE-I,EXE-IC,EXE-IF,EXE-ICF,EXE-IFP,EXE)
// RUN:   (mode=MODE_I  cflags=-DSTORAGE=static pre=EXE-I,EXE-IC,EXE-IF,EXE-ICF,EXE-IFP,EXE)
// RUN:   (mode=MODE_C0 cflags=                 pre=EXE-C,EXE-IC,EXE-ICF,EXE               )
// RUN:   (mode=MODE_C0 cflags=-DSTORAGE=static pre=EXE-C,EXE-IC,EXE-ICF,EXE               )
// RUN:   (mode=MODE_C1 cflags=                 pre=EXE-C,EXE-IC,EXE-ICF,EXE               )
// RUN:   (mode=MODE_C1 cflags=-DSTORAGE=static pre=EXE-C,EXE-IC,EXE-ICF,EXE               )
// RUN:   (mode=MODE_C2 cflags=                 pre=EXE-C,EXE-IC,EXE-ICF,EXE               )
// RUN:   (mode=MODE_C2 cflags=-DSTORAGE=static pre=EXE-C,EXE-IC,EXE-ICF,EXE               )
// RUN:   (mode=MODE_F  cflags=                 pre=EXE-F,EXE-IF,EXE-FP,EXE-ICF,EXE-IFP,EXE)
// RUN:   (mode=MODE_F  cflags=-DSTORAGE=static pre=EXE-F,EXE-IF,EXE-FP,EXE-ICF,EXE-IFP,EXE)
// RUN:   (mode=MODE_P  cflags=                 pre=EXE-P,EXE-FP,EXE-IFP,EXE               )
// RUN:   (mode=MODE_P  cflags=-DSTORAGE=static pre=EXE-P,EXE-FP,EXE-IFP,EXE               )
// RUN: }
// RUN: %for directives {
// RUN:   %for exes {
// RUN:     %for prt-opts {
// RUN:       %clang -Xclang -verify %[prt-opt]=omp %s > %t-omp.c \
// RUN:              -DMODE=%[mode] %[cflags] %[dir-cflags]
// RUN:       echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN:       %clang -Xclang -verify -fopenmp %fopenmp-version -o %t %t-omp.c \
// RUN:              -DMODE=%[mode] %[cflags] %[dir-cflags]
// RUN:       %t | FileCheck -check-prefixes=%[pre] %s
// RUN:     }
// RUN:   }
// RUN: }
//
// Check execution with normal compilation.
//
// RUN: %data tgts {
// RUN:   (run-if=                tgt=HOST    tgt-cflags=                                    )
// RUN:   (run-if=%run-if-x86_64  tgt=X86_64  tgt-cflags=-fopenmp-targets=%run-x86_64-triple )
// RUN:   (run-if=%run-if-nvptx64 tgt=NVPTX64 tgt-cflags=-fopenmp-targets=%run-nvptx64-triple)
// RUN: }
// RUN: %for directives {
// RUN:   %for exes {
// RUN:     %for tgts {
// RUN:       %[run-if] %clang -Xclang -verify -fopenacc %s -o %t \
// RUN:                        -DMODE=%[mode] -DTGT_%[tgt]_EXE \
// RUN:                        %[cflags] %[tgt-cflags] %[dir-cflags]
// RUN:       %[run-if] %t > %t.out 2>&1
// RUN:       %[run-if] FileCheck -input-file %t.out %s \
// RUN:                           -check-prefixes=%[pre]
// RUN:     }
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
// RUN: }
// RUN: %for llvm {
// RUN:   %clang -Xclang -verify -fopenacc -S -emit-llvm -o %t -DMODE=%[mode] \
// RUN:          %[cflags] %s
// RUN:   cat %t | FileCheck -check-prefixes=LLVM %s
// RUN: }
//
// LLVM: define internal void @.omp_outlined.
// LLVM-SAME: ({{[^,]+}} %.global_tid., {{[^,]+}} %.bound_tid.)

// END.

// expected-no-diagnostics

#include <stdio.h>
#include <stdint.h>

#if !ADD_LOOP_TO_PAR
# define LOOP
# define FORLOOP_HEAD
#else
# define LOOP loop seq
# define FORLOOP_HEAD for (int i = 0; i < 1; ++i)
#endif

#ifndef STORAGE
# define STORAGE
#endif

#define MODE_I  1
#define MODE_C0 2
#define MODE_C1 3
#define MODE_C2 4
#define MODE_F  5
#define MODE_P  6

#ifndef MODE
# error MODE undefined
#elif MODE == MODE_I
# define CLAUSE(...)
# define CLAUSE_NOT_PRIVATE(...)
#elif MODE == MODE_C0
# define CLAUSE(...) copy(__VA_ARGS__)
# define CLAUSE_NOT_PRIVATE(...) copy(__VA_ARGS__)
#elif MODE == MODE_C1
# define CLAUSE(...) pcopy(__VA_ARGS__)
# define CLAUSE_NOT_PRIVATE(...) pcopy(__VA_ARGS__)
#elif MODE == MODE_C2
# define CLAUSE(...) present_or_copy(__VA_ARGS__)
# define CLAUSE_NOT_PRIVATE(...) present_or_copy(__VA_ARGS__)
#elif MODE == MODE_F
# define CLAUSE(...) firstprivate(__VA_ARGS__)
# define CLAUSE_NOT_PRIVATE(...) firstprivate(__VA_ARGS__)
#elif MODE == MODE_P
# define CLAUSE(...) private(__VA_ARGS__)
# define CLAUSE_NOT_PRIVATE(...)
#else
# error unknown MODE
#endif

#ifdef __SIZEOF_INT128__
# define MK_UINT128(Hi,Lw) ((((__uint128_t)(Hi))<<64) + (Lw))
# define DEF_UINT128(Var, Hi, Lw) __uint128_t Var = MK_UINT128(Hi,Lw)
# define DEF_UINT128_COPY(Var1, Var2) __uint128_t Var1 = Var2
# define ASSIGN_UINT128(Var, Hi, Lw) Var = MK_UINT128(Hi,Lw)
# define LIST_UINT128(Var) Var
# define HI_UINT128(Var) ((uint64_t)((Var)>>64))
# define LW_UINT128(Var) ((uint64_t)((Var)&UINT64_MAX))
#else
# define DEF_UINT128(Var, Hi, Lw) \
  uint64_t Var##_hi = Hi, Var##_lw = Lw
# define DEF_UINT128_COPY(Var1, Var2) \
  uint64_t Var1##_hi = Var2##_hi, Var1##_lw = Var2##_lw
# define ASSIGN_UINT128(Var, Hi, Lw) \
  Var##_hi = Hi; Var##_li = Lw
# define LIST_UINT128(Var) Var##_hi, Var##_lw
# define HI_UINT128(Var) Var##_hi
# define LW_UINT128(Var) Var##_lw
#endif

// FIXME: When OpenMP offloading is activated by -fopenmp-targets, pointers
// pass into acc parallel as null, but otherwise they pass in just fine.
// What does the OpenMP spec say is supposed to happen?

#if !TGT_X86_64_EXE && !TGT_NVPTX64_EXE
# define CHECK_PTR_EXE 1
# define IF_PTR_EXE(...) __VA_ARGS__
#else
# define CHECK_PTR_EXE 0
# define IF_PTR_EXE(...)
#endif

// Scalar global, either:
// - implicitly/explicitly firstprivate and either:
//   - < uintptr size, captured by copy
//   - >= uintptr size, captured by ref with data copied
//   - At one time, the last case was captured by copy, and it was truncated
//     into 64 bits as a result. Thus, we exercise both the high and low 64
//     bits to check that all bits are passed through.
// - explicitly private, captured decl only
STORAGE int gi = 55;
STORAGE DEF_UINT128(gt, 0x1400, 0x59); // t=tetra integer
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
  STORAGE int li = 99;
  STORAGE DEF_UINT128(lt, 0x7a1, 0x62b0); // t=tetra integer
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
  DEF_UINT128_COPY(gtOld, gt);
  const int *gpOld = gp;
  int gaOld[2] = {ga[0], ga[1]};
  int gUnrefOld = gUnref;
  struct S gsOld = gs;
  union U guOld = gu;

  int liOld = li;
  DEF_UINT128_COPY(ltOld, lt);
  const int *lpOld = lp;
  int laOld[2] = {la[0], la[1]};
  struct S lsOld = ls;
  int lUnrefOld = lUnref;
  union U luOld = lu;

  // Implicit firstprivate that is shadowed.
  STORAGE int shadowed = 111;

#if MODE != MODE_P
  // Const scalars and non-scalars should be fine with either implicit or
  // explicit data sharing attributes.
  const int ci = 53;
  const int ca[3] = {10, 11, 12};
#endif

  // DMP-PARLOOP:        ACCParallelLoopDirective
  // DMP-PARLOOP-NEXT:     ACCSeqClause
  // DMP-PARLOOP-NEXT:     ACCNum_gangsClause
  // DMP-PARLOOP-NEXT:       IntegerLiteral {{.*}} 'int' 2
  // DMP-PARLOOP-C-NEXT:   ACCCopyClause
  // DMP-PARLOOP-F-NEXT:   ACCFirstprivateClause
  // DMP-PARLOOP-P-NEXT:   ACCPrivateClause
  // DMP-PARLOOP-CFP-NOT:    <implicit>
  // DMP-PARLOOP-CFP-SAME:   {{$}}
  // DMP-PARLOOP-CFP-NEXT:   DeclRefExpr {{.*}} 'gi' 'int'
  // DMP-PARLOOP-CFP-NEXT:   DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  // DMP-PARLOOP-CFP-NEXT:   DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-PARLOOP-CFP-NEXT:   DeclRefExpr {{.*}} 'ga' 'int [2]'
  // DMP-PARLOOP-CFP-NEXT:   DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-PARLOOP-CFP-NEXT:   DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-PARLOOP-CFP-NEXT:   DeclRefExpr {{.*}} 'gUnref' 'int'
  // DMP-PARLOOP-C-NEXT:   ACCCopyClause
  // DMP-PARLOOP-F-NEXT:   ACCFirstprivateClause
  // DMP-PARLOOP-P-NEXT:   ACCPrivateClause
  // DMP-PARLOOP-CFP-NOT:    <implicit>
  // DMP-PARLOOP-CFP-SAME:   {{$}}
  // DMP-PARLOOP-CFP-NEXT:   DeclRefExpr {{.*}} 'li' 'int'
  // DMP-PARLOOP-CFP-NEXT:   DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  // DMP-PARLOOP-CFP-NEXT:   DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-PARLOOP-CFP-NEXT:   DeclRefExpr {{.*}} 'la' 'int [2]'
  // DMP-PARLOOP-CFP-NEXT:   DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-PARLOOP-CFP-NEXT:   DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-PARLOOP-CFP-NEXT:   DeclRefExpr {{.*}} 'lUnref' 'int'
  // DMP-PARLOOP-C-NEXT:   ACCCopyClause
  // DMP-PARLOOP-F-NEXT:   ACCFirstprivateClause
  // DMP-PARLOOP-P-NEXT:   ACCPrivateClause
  // DMP-PARLOOP-CFP-NOT:    <implicit>
  // DMP-PARLOOP-CFP-SAME:   {{$}}
  // DMP-PARLOOP-CFP-NEXT:   DeclRefExpr {{.*}} 'shadowed' 'int'
  // DMP-PARLOOP-C-NEXT:   ACCCopyClause
  // DMP-PARLOOP-F-NEXT:   ACCFirstprivateClause
  // DMP-PARLOOP-CF-NOT:     <implicit>
  // DMP-PARLOOP-CF-SAME:    {{$}}
  // DMP-PARLOOP-CF-NEXT:    DeclRefExpr {{.*}} 'ci' 'const int'
  // DMP-PARLOOP-CF-NEXT:    DeclRefExpr {{.*}} 'ca' 'const int [3]'
  // DMP-PARLOOP-NEXT:     effect: ACCParallelDirective
  // DMP-PAR:              ACCParallelDirective
  // DMP-NEXT:               ACCNum_gangsClause
  // DMP-NEXT:                 IntegerLiteral {{.*}} 'int' 2
  // DMP-C-NEXT:             ACCCopyClause
  // DMP-F-NEXT:             ACCFirstprivateClause
  // DMP-CF-NOT:               <implicit>
  // DMP-CF-SAME:              {{$}}
  // DMP-CF-NEXT:              DeclRefExpr {{.*}} 'gi' 'int'
  // DMP-CF-NEXT:              DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  // DMP-CF-NEXT:              DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-CF-NEXT:              DeclRefExpr {{.*}} 'ga' 'int [2]'
  // DMP-CF-NEXT:              DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-CF-NEXT:              DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-CF-NEXT:              DeclRefExpr {{.*}} 'gUnref' 'int'
  // DMP-C-NEXT:             ACCCopyClause
  // DMP-F-NEXT:             ACCFirstprivateClause
  // DMP-CF-NOT:               <implicit>
  // DMP-CF-SAME:              {{$}}
  // DMP-CF-NEXT:              DeclRefExpr {{.*}} 'li' 'int'
  // DMP-CF-NEXT:              DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  // DMP-CF-NEXT:              DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-CF-NEXT:              DeclRefExpr {{.*}} 'la' 'int [2]'
  // DMP-CF-NEXT:              DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-CF-NEXT:              DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-CF-NEXT:              DeclRefExpr {{.*}} 'lUnref' 'int'
  // DMP-C-NEXT:             ACCCopyClause
  // DMP-F-NEXT:             ACCFirstprivateClause
  // DMP-CF-NOT:               <implicit>
  // DMP-CF-SAME:              {{$}}
  // DMP-CF-NEXT:              DeclRefExpr {{.*}} 'shadowed' 'int'
  // DMP-C-NEXT:             ACCCopyClause
  // DMP-F-NEXT:             ACCFirstprivateClause
  // DMP-CF-NOT:               <implicit>
  // DMP-CF-SAME:              {{$}}
  // DMP-CF-NEXT:              DeclRefExpr {{.*}} 'ci' 'const int'
  // DMP-CF-NEXT:              DeclRefExpr {{.*}} 'ca' 'const int [3]'
  // DMP-PAR-P-NEXT:         ACCPrivateClause
  // DMP-PAR-P-NOT:            <implicit>
  // DMP-PAR-P-SAME:           {{$}}
  // DMP-PAR-P-NEXT:           DeclRefExpr {{.*}} 'gi' 'int'
  // DMP-PAR-P-NEXT:           DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  // DMP-PAR-P-NEXT:           DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-PAR-P-NEXT:           DeclRefExpr {{.*}} 'ga' 'int [2]'
  // DMP-PAR-P-NEXT:           DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-PAR-P-NEXT:           DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-PAR-P-NEXT:           DeclRefExpr {{.*}} 'gUnref' 'int'
  // DMP-PAR-P-NEXT:         ACCPrivateClause
  // DMP-PAR-P-NOT:            <implicit>
  // DMP-PAR-P-SAME:           {{$}}
  // DMP-PAR-P-NEXT:           DeclRefExpr {{.*}} 'li' 'int'
  // DMP-PAR-P-NEXT:           DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  // DMP-PAR-P-NEXT:           DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-PAR-P-NEXT:           DeclRefExpr {{.*}} 'la' 'int [2]'
  // DMP-PAR-P-NEXT:           DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-PAR-P-NEXT:           DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-PAR-P-NEXT:           DeclRefExpr {{.*}} 'lUnref' 'int'
  // DMP-PAR-P-NEXT:         ACCPrivateClause
  // DMP-PAR-P-NOT:            <implicit>
  // DMP-PAR-P-SAME:           {{$}}
  // DMP-PAR-P-NEXT:           DeclRefExpr {{.*}} 'shadowed' 'int'
  // DMP-I-NEXT:             ACCCopyClause {{.*}} <implicit>
  // DMP-I-NEXT:               DeclRefExpr {{.*}} 'ga' 'int [2]'
  // DMP-I-NEXT:               DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-I-NEXT:               DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-I-NEXT:               DeclRefExpr {{.*}} 'la' 'int [2]'
  // DMP-I-NEXT:               DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-I-NEXT:               DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-I-NEXT:               DeclRefExpr {{.*}} 'ca' 'const int [3]'
  // DMP-I-NEXT:             ACCFirstprivateClause {{.*}} <implicit>
  // DMP-I-NEXT:               DeclRefExpr {{.*}} 'gi' 'int'
  // DMP-I-NEXT:               DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  // DMP-I-NEXT:               DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-I-NEXT:               DeclRefExpr {{.*}} 'li' 'int'
  // DMP-I-NEXT:               DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  // DMP-I-NEXT:               DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-I-NEXT:               DeclRefExpr {{.*}} 'shadowed' 'int'
  // DMP-I-NEXT:               DeclRefExpr {{.*}} 'ci' 'const int'
  // DMP-NEXT:               impl: OMPTargetTeamsDirective
  // DMP-NEXT:                 OMPNum_teamsClause
  // DMP-NEXT:                   IntegerLiteral {{.*}} 'int' 2
  // DMP-C-NEXT:               OMPMapClause
  // DMP-F-NEXT:               OMPFirstprivateClause
  // DMP-CF-NOT:                 <implicit>
  // DMP-CF-SAME:                {{$}}
  // DMP-CF-NEXT:                DeclRefExpr {{.*}} 'gi' 'int'
  // DMP-CF-NEXT:                DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  // DMP-CF-NEXT:                DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-CF-NEXT:                DeclRefExpr {{.*}} 'ga' 'int [2]'
  // DMP-CF-NEXT:                DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-CF-NEXT:                DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-CF-NEXT:                DeclRefExpr {{.*}} 'gUnref' 'int'
  // DMP-C-NEXT:               OMPMapClause
  // DMP-F-NEXT:               OMPFirstprivateClause
  // DMP-CF-NOT:                 <implicit>
  // DMP-CF-SAME:                {{$}}
  // DMP-CF-NEXT:                DeclRefExpr {{.*}} 'li' 'int'
  // DMP-CF-NEXT:                DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  // DMP-CF-NEXT:                DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-CF-NEXT:                DeclRefExpr {{.*}} 'la' 'int [2]'
  // DMP-CF-NEXT:                DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-CF-NEXT:                DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-CF-NEXT:                DeclRefExpr {{.*}} 'lUnref' 'int'
  // DMP-C-NEXT:               OMPMapClause
  // DMP-F-NEXT:               OMPFirstprivateClause
  // DMP-CF-NOT:                 <implicit>
  // DMP-CF-SAME:                {{$}}
  // DMP-CF-NEXT:                DeclRefExpr {{.*}} 'shadowed' 'int'
  // DMP-C-NEXT:               OMPMapClause
  // DMP-F-NEXT:               OMPFirstprivateClause
  // DMP-CF-NOT:                 <implicit>
  // DMP-CF-SAME:                {{$}}
  // DMP-CF-NEXT:                DeclRefExpr {{.*}} 'ci' 'const int'
  // DMP-CF-NEXT:                DeclRefExpr {{.*}} 'ca' 'const int [3]'
  // DMP-PAR-P-NEXT:           OMPPrivateClause
  // DMP-PAR-P-NOT:              <implicit>
  // DMP-PAR-P-SAME:             {{$}}
  // DMP-PAR-P-NEXT:             DeclRefExpr {{.*}} 'gi' 'int'
  // DMP-PAR-P-NEXT:             DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  // DMP-PAR-P-NEXT:             DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-PAR-P-NEXT:             DeclRefExpr {{.*}} 'ga' 'int [2]'
  // DMP-PAR-P-NEXT:             DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-PAR-P-NEXT:             DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-PAR-P-NEXT:             DeclRefExpr {{.*}} 'gUnref' 'int'
  // DMP-PAR-P-NEXT:           OMPPrivateClause
  // DMP-PAR-P-NOT:              <implicit>
  // DMP-PAR-P-SAME:             {{$}}
  // DMP-PAR-P-NEXT:             DeclRefExpr {{.*}} 'li' 'int'
  // DMP-PAR-P-NEXT:             DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  // DMP-PAR-P-NEXT:             DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-PAR-P-NEXT:             DeclRefExpr {{.*}} 'la' 'int [2]'
  // DMP-PAR-P-NEXT:             DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-PAR-P-NEXT:             DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-PAR-P-NEXT:             DeclRefExpr {{.*}} 'lUnref' 'int'
  // DMP-PAR-P-NEXT:           OMPPrivateClause
  // DMP-PAR-P-NOT:              <implicit>
  // DMP-PAR-P-SAME:             {{$}}
  // DMP-PAR-P-NEXT:             DeclRefExpr {{.*}} 'shadowed' 'int'
  // DMP-I-NEXT:               OMPMapClause
  // DMP-I-NOT:                  <implicit>
  // DMP-I-SAME:                 {{$}}
  // DMP-I-NEXT:                 DeclRefExpr {{.*}} 'ga' 'int [2]'
  // DMP-I-NEXT:                 DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-I-NEXT:                 DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-I-NEXT:                 DeclRefExpr {{.*}} 'la' 'int [2]'
  // DMP-I-NEXT:                 DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-I-NEXT:                 DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-I-NEXT:                 DeclRefExpr {{.*}} 'ca' 'const int [3]'
  // DMP-I-NEXT:               OMPFirstprivateClause
  // DMP-I-NOT:                  <implicit>
  // DMP-I-SAME:                 {{$}}
  // DMP-I-NEXT:                 DeclRefExpr {{.*}} 'gi' 'int'
  // DMP-I-NEXT:                 DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  // DMP-I-NEXT:                 DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-I-NEXT:                 DeclRefExpr {{.*}} 'li' 'int'
  // DMP-I-NEXT:                 DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  // DMP-I-NEXT:                 DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-I-NEXT:                 DeclRefExpr {{.*}} 'shadowed' 'int'
  // DMP-I-NEXT:                 DeclRefExpr {{.*}} 'ci' 'const int'
  // DMP-PARLOOP:            ACCLoopDirective
  // DMP-PARLOOP-NEXT:         ACCSeqClause
  // DMP-PARLOOP-P-NEXT:       ACCPrivateClause
  // DMP-PARLOOP-P-NOT:          <implicit>
  // DMP-PARLOOP-P-SAME:         {{$}}
  // DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'gi' 'int'
  // DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  // DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'ga' 'int [2]'
  // DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'gUnref' 'int'
  // DMP-PARLOOP-P-NEXT:       ACCPrivateClause
  // DMP-PARLOOP-P-NOT:          <implicit>
  // DMP-PARLOOP-P-SAME:         {{$}}
  // DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'li' 'int'
  // DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  // DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'la' 'int [2]'
  // DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'lUnref' 'int'
  // DMP-PARLOOP-P-NEXT:       ACCPrivateClause
  // DMP-PARLOOP-P-NOT:          <implicit>
  // DMP-PARLOOP-P-SAME:         {{$}}
  // DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'shadowed' 'int'
  // DMP-PARLOOP-ICF-NEXT:     ACCSharedClause {{.*}} <implicit>
  // DMP-PARLOOP-ICF-NEXT:       DeclRefExpr {{.*}} 'gi' 'int'
  // DMP-PARLOOP-ICF-NEXT:       DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  // DMP-PARLOOP-ICF-NEXT:       DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-PARLOOP-ICF-NEXT:       DeclRefExpr {{.*}} 'ga' 'int [2]'
  // DMP-PARLOOP-ICF-NEXT:       DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-PARLOOP-ICF-NEXT:       DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-PARLOOP-ICF-NEXT:       DeclRefExpr {{.*}} 'li' 'int'
  // DMP-PARLOOP-ICF-NEXT:       DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  // DMP-PARLOOP-ICF-NEXT:       DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-PARLOOP-ICF-NEXT:       DeclRefExpr {{.*}} 'la' 'int [2]'
  // DMP-PARLOOP-ICF-NEXT:       DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-PARLOOP-ICF-NEXT:       DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-PARLOOP-ICF-NEXT:       DeclRefExpr {{.*}} 'shadowed' 'int'
  // DMP-PARLOOP-ICF-NEXT:       DeclRefExpr {{.*}} 'ci' 'const int'
  // DMP-PARLOOP-ICF-NEXT:       DeclRefExpr {{.*}} 'ca' 'const int [3]'
  // DMP-PARLOOP-ICF-NEXT:     impl: ForStmt
  // DMP-PARLOOP-P-NEXT:       impl: CompoundStmt
  // DMP-PARLOOP-P-NEXT:         DeclStmt
  // DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} gi 'int'
  // DMP-PARLOOP-P-NEXT:         DeclStmt
  // DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} {{gt '__uint128_t'|gt_hi 'uint64_t'$[[:space:]]*VarDecl .* gt_lw 'uint64_t'}}
  // DMP-PARLOOP-P-NEXT:         DeclStmt
  // DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} gp 'const int *'
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
  // DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} {{lt '__uint128_t'|lt_hi 'uint64_t'$[[:space:]]*VarDecl .* lt_lw 'uint64_t'}}
  // DMP-PARLOOP-P-NEXT:         DeclStmt
  // DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} lp 'const int *'
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
  // PRT-A-PAR-I:            {{^ *}}#pragma acc parallel{{ LOOP | }}num_gangs(2){{(.*\\$[[:space:]])*.*$}}
  // PRT-A-PARLOOP-I:        {{^ *}}#pragma acc parallel {{LOOP|loop seq}} num_gangs(2){{(.*\\$[[:space:]])*.*$}}
  // PRT-AO-I-NEXT:          {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: ga,gs,gu,la,ls,lu,ca) firstprivate(gi,gt{{(_hi,gt_lw)?}},gp,li,lt{{(_hi,lt_lw)?}},lp,shadowed,ci){{$}}
  // PRT-O-I:                {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: ga,gs,gu,la,ls,lu,ca) firstprivate(gi,gt{{(_hi,gt_lw)?}},gp,li,lt{{(_hi,lt_lw)?}},lp,shadowed,ci){{$}}
  // PRT-OA-PAR-I-NEXT:      {{^ *}}// #pragma acc parallel{{ LOOP | }}num_gangs(2){{(.*\\$[[:space:]])*.*$}}
  // PRT-OA-PARLOOP-I-NEXT:  {{^ *}}// #pragma acc parallel {{LOOP|loop seq}} num_gangs(2){{(.*\\$[[:space:]])*.*$}}
  //
  // PRT-A-PAR-C0:           {{^ *}}#pragma acc parallel{{ LOOP | }}num_gangs(2) {{copy\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) copy\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) copy\(shadowed\) copy\(ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-A-PAR-C1:           {{^ *}}#pragma acc parallel{{ LOOP | }}num_gangs(2) {{pcopy\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) pcopy\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) pcopy\(shadowed\) pcopy\(ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-A-PAR-C2:           {{^ *}}#pragma acc parallel{{ LOOP | }}num_gangs(2) {{present_or_copy\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) present_or_copy\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) present_or_copy\(shadowed\) present_or_copy\(ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-A-PARLOOP-C0:       {{^ *}}#pragma acc parallel {{LOOP|loop seq}} num_gangs(2) {{copy\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) copy\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) copy\(shadowed\) copy\(ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-A-PARLOOP-C1:       {{^ *}}#pragma acc parallel {{LOOP|loop seq}} num_gangs(2) {{pcopy\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) pcopy\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) pcopy\(shadowed\) pcopy\(ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-A-PARLOOP-C2:       {{^ *}}#pragma acc parallel {{LOOP|loop seq}} num_gangs(2) {{present_or_copy\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) present_or_copy\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) present_or_copy\(shadowed\) present_or_copy\(ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-AO-C0-NEXT:         {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(tofrom: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(tofrom: shadowed) map(tofrom: ci,ca){{$}}
  // PRT-AO-C1-NEXT:         {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(tofrom: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(tofrom: shadowed) map(tofrom: ci,ca){{$}}
  // PRT-AO-C2-NEXT:         {{^ *}}// #pragma omp target teams num_teams(2) map(tofrom: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(tofrom: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(tofrom: shadowed) map(tofrom: ci,ca){{$}}
  // PRT-O-C0:               {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(tofrom: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(tofrom: shadowed) map(tofrom: ci,ca){{$}}
  // PRT-O-C1:               {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(tofrom: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(tofrom: shadowed) map(tofrom: ci,ca){{$}}
  // PRT-O-C2:               {{^ *}}#pragma omp target teams num_teams(2) map(tofrom: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(tofrom: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(tofrom: shadowed) map(tofrom: ci,ca){{$}}
  // PRT-OA-PAR-C0-NEXT:     {{^ *}}// #pragma acc parallel{{ LOOP | }}num_gangs(2) {{copy\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) copy\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) copy\(shadowed\) copy\(ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-OA-PAR-C1-NEXT:     {{^ *}}// #pragma acc parallel{{ LOOP | }}num_gangs(2) {{pcopy\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) pcopy\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) pcopy\(shadowed\) pcopy\(ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-OA-PAR-C2-NEXT:     {{^ *}}// #pragma acc parallel{{ LOOP | }}num_gangs(2) {{present_or_copy\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) present_or_copy\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) present_or_copy\(shadowed\) present_or_copy\(ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-OA-PARLOOP-C0-NEXT: {{^ *}}// #pragma acc parallel {{LOOP|loop seq}} num_gangs(2) {{copy\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) copy\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) copy\(shadowed\) copy\(ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-OA-PARLOOP-C1-NEXT: {{^ *}}// #pragma acc parallel {{LOOP|loop seq}} num_gangs(2) {{pcopy\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) pcopy\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) pcopy\(shadowed\) pcopy\(ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-OA-PARLOOP-C2-NEXT: {{^ *}}// #pragma acc parallel {{LOOP|loop seq}} num_gangs(2) {{present_or_copy\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) present_or_copy\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) present_or_copy\(shadowed\) present_or_copy\(ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //
  // PRT-A-PAR-F:            {{^ *}}#pragma acc parallel{{ LOOP | }}num_gangs(2) {{firstprivate\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) firstprivate\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) firstprivate\(shadowed\) firstprivate\(ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-A-PARLOOP-F:        {{^ *}}#pragma acc parallel {{LOOP|loop seq}} num_gangs(2) {{firstprivate\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) firstprivate\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) firstprivate\(shadowed\) firstprivate\(ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-AO-F-NEXT:          {{^ *}}// #pragma omp target teams num_teams(2) firstprivate(gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) firstprivate(li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) firstprivate(shadowed) firstprivate(ci,ca){{$}}
  // PRT-O-F:                {{^ *}}#pragma omp target teams num_teams(2) firstprivate(gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) firstprivate(li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) firstprivate(shadowed) firstprivate(ci,ca){{$}}
  // PRT-OA-PAR-F-NEXT:      {{^ *}}// #pragma acc parallel{{ LOOP | }}num_gangs(2) {{firstprivate\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) firstprivate\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) firstprivate\(shadowed\) firstprivate\(ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-OA-PARLOOP-F-NEXT:  {{^ *}}// #pragma acc parallel {{LOOP|loop seq}} num_gangs(2) {{firstprivate\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) firstprivate\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) firstprivate\(shadowed\) firstprivate\(ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //
  // PRT-A-PAR-P:            {{^ *}}#pragma acc parallel{{ LOOP | }}num_gangs(2) {{private\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) private\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) private\(shadowed\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-AO-PAR-P-NEXT:      {{^ *}}// #pragma omp target teams num_teams(2) private(gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) private(li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) private(shadowed){{$}}
  // PRT-A-PARLOOP-P:        {{^ *}}#pragma acc parallel {{LOOP|loop seq}} num_gangs(2) {{private\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) private\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) private\(shadowed\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-AO-PARLOOP-P-NOT:   #pragma
  // PRT-AO-PARLOOP-P:       {{^ *}}// #pragma omp target teams num_teams(2){{$}}
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}{
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  int gi;
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  {{__uint128_t gt;|uint64_t gt_hi;$[[:space:]]*// *  uint64_t gt_lw;}}
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  const int *gp;
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  int ga[2];
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  struct S gs;
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  union U gu;
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  int gUnref;
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  int li;
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  {{__uint128_t lt;|uint64_t lt_hi;$[[:space:]]*// *  uint64_t lt_lw;}}
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  const int *lp;
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  int la[2];
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  struct S ls;
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  union U lu;
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  int lUnref;
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  int shadowed;
  // PRT-O-PAR-P:            {{^ *}}#pragma omp target teams num_teams(2) private(gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) private(li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) private(shadowed){{$}}
  // PRT-OA-PAR-P-NEXT:      {{^ *}}// #pragma acc parallel{{ LOOP | }}num_gangs(2) {{private\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) private\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) private\(shadowed\)$|(.*\\$[[:space:]])+.*$}}
  // PRT-O-PARLOOP-P:        {{^ *}}#pragma omp target teams num_teams(2){{$}}
  // PRT-O-PARLOOP-P-NEXT:   {
  // PRT-O-PARLOOP-P-NEXT:     int gi;
  // PRT-O-PARLOOP-P-NEXT:     {{__uint128_t gt;|uint64_t gt_hi;$[[:space:]]*  uint64_t gt_lw;}}
  // PRT-O-PARLOOP-P-NEXT:     int *gp;
  // PRT-O-PARLOOP-P-NEXT:     int ga[2];
  // PRT-O-PARLOOP-P-NEXT:     struct S gs;
  // PRT-O-PARLOOP-P-NEXT:     union U gu;
  // PRT-O-PARLOOP-P-NEXT:     int gUnref;
  // PRT-O-PARLOOP-P-NEXT:     int li;
  // PRT-O-PARLOOP-P-NEXT:     {{__uint128_t lt;|uint64_t lt_hi;$[[:space:]]*  uint64_t lt_lw;}}
  // PRT-O-PARLOOP-P-NEXT:     int *lp;
  // PRT-O-PARLOOP-P-NEXT:     int la[2];
  // PRT-O-PARLOOP-P-NEXT:     struct S ls;
  // PRT-O-PARLOOP-P-NEXT:     union U lu;
  // PRT-O-PARLOOP-P-NEXT:     int lUnref;
  // PRT-O-PARLOOP-P-NEXT:     int shadowed;
  // PRT-OA-PARLOOP-P-NOT:   #pragma
  // PRT-OA-PARLOOP-P:       {{^ *}}// #pragma acc parallel {{LOOP|loop seq}} num_gangs(2) {{private\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) private\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) private\(shadowed\)$|(.*\\$[[:space:]])+.*$}}
  //
  // PRT-NOT:      #pragma
  #pragma acc parallel LOOP num_gangs(2)                         \
    CLAUSE(gi,LIST_UINT128(gt),gp, ga,gs , gu , gUnref )         \
    CLAUSE(  li,LIST_UINT128(lt),lp  ,  la  ,ls,  lu , lUnref  ) \
    CLAUSE(shadowed) CLAUSE_NOT_PRIVATE(ci, ca)
  // PRT-PARLOOP-NEXT: {{for (.*) [{]|FORLOOP_HEAD}}
  // PRT-PAR-NEXT: {{(FORLOOP_HEAD$[[:space:]])? *}}{
  FORLOOP_HEAD
  {
    // Read before writing in order to exercise SemaExpr.cpp's
    // isVariableAlreadyCapturedInScopeInfo's code that suppresses adding const
    // to the captured variable in the case of OpenACC.
    int giOld = gi;
    DEF_UINT128_COPY(gtOld, gt);
    // Check a decl with no init: it used to break our transform code.
    const int *gpOld;
    gpOld = gp;
    int gaOld[2] = {ga[0], ga[1]};
    struct S gsOld = gs;
    union U guOld = gu;

    int liOld = li;
    DEF_UINT128_COPY(ltOld, lt);
    const int *lpOld = lp;
    int laOld[2] = {la[0], la[1]};
    struct S lsOld = ls;
    union U luOld = lu;

    int tmp = shadowed;
    int shadowed = tmp + 111;

    gi = 56;
    ASSIGN_UINT128(gt, 0xf08234, 0xa07de1);
    gp = &li;
    ga[0] = 101;
    ga[1] = 202;
    gs.i = 42;
    gs.j = 1;
    gu.i = 13;

    li = 98;
    ASSIGN_UINT128(lt, 0x79ca, 0x2961);
    lp = &li;
    la[0] = 55;
    la[1] = 66;
    ls.i = 333;
    ls.j = 444;
    lu.i = 279;

    shadowed += 111;

#if MODE == MODE_P
    // don't dereference uninitialized pointers
    gpOld = gp;
    lpOld = lp;
#endif
    // PRT:      printf("inside :{{([^;]|[[:space:]])*}};
    // PRT-NEXT: printf("{{([^;]|[[:space:]])*}};
    //
    // It cannot all be in one printf, or nvptx64 output becomes corrupt, so
    // break it into multiple printfs.  The order of output from different
    // gangs is non-deterministic at the level of printfs, but it's easier to
    // write the FileCheck DAG directives at the level of lines.
    //
    // EXE-IF-DAG:  inside : gi:55->56,
    // EXE-C-DAG:   inside : gi:{{55|56}}->56,
    // EXE-P-DAG:   inside : gi:{{.+}}->56,
    // EXE-IF-DAG:           gt:[1400,59]->[f08234,a07de1],
    // EXE-C-DAG:            gt:[{{1400|f08234}},{{59|a07de1}}]->[f08234,a07de1],
    // EXE-P-DAG:            gt:[{{.+}}]->[f08234,a07de1],
    // EXE-I-DAG:            {{^ *(\*gp:55->98, )?}}ga:[{{100|101}},{{200|202}}]->[101,202],
    // EXE-C-DAG:            {{^ *(\*gp:(56|98)->98, )?}}ga:[{{100|101}},{{200|202}}]->[101,202],
    // EXE-F-DAG:            {{^ *(\*gp:55->98, )?}}ga:[100,200]->[101,202],
    // EXE-P-DAG:            {{^ *(\*gp:.+->98, )?}}ga:[{{.+,.+}}]->[101,202],
    // EXE-IC-DAG:           gs:[{{33|42}},{{11|1}}]->[42,1], gu.i:{{22|13}}->13,
    // EXE-F-DAG:            gs:[33,11]->[42,1], gu.i:22->13,
    // EXE-P-DAG:            gs:[{{.+,.+}}]->[42,1], gu.i:{{.+}}->13,
    // EXE-IF-DAG:           li:99->98,
    // EXE-C-DAG:            li:{{99|98}}->98,
    // EXE-P-DAG:            li:{{.+}}->98,
    // EXE-IF-DAG:           lt:[7a1,62b0]->[79ca,2961],
    // EXE-C-DAG:            lt:[{{7a1|79ca}},{{62b0|2961}}]->[79ca,2961],
    // EXE-P-DAG:            lt:[{{.+}}]->[79ca,2961],
    // EXE-I-DAG:            {{^ *(\*lp:55->98, )?}}la:[{{77|55}},{{88|66}}]->[55,66],
    // EXE-C-DAG:            {{^ *(\*lp:(56|98)->98, )?}}la:[{{77|55}},{{88|66}}]->[55,66],
    // EXE-F-DAG:            {{^ *(\*lp:55->98, )?}}la:[77,88]->[55,66],
    // EXE-P-DAG:            {{^ *(\*lp:.+->98, )?}}la:[{{.+,.+}}]->[55,66],
    // EXE-IC-DAG:           ls:[{{222|333}},{{111|444}}]->[333,444], lu.i:{{167|279}}->279,
    // EXE-F-DAG:            ls:[222,111]->[333,444], lu.i:167->279,
    // EXE-P-DAG:            ls:[{{.+,.+}}]->[333,444], lu.i:{{.+}}->279,
    // EXE-DAG:              shadowed:
    // EXE-ICF-DAG:          ci:53, ca:[10,11,12]
    //
    // Duplicate that EXE block exactly.
    //
    // EXE-IF-DAG:  inside : gi:55->56,
    // EXE-C-DAG:   inside : gi:{{55|56}}->56,
    // EXE-P-DAG:   inside : gi:{{.+}}->56,
    // EXE-IF-DAG:           gt:[1400,59]->[f08234,a07de1],
    // EXE-C-DAG:            gt:[{{1400|f08234}},{{59|a07de1}}]->[f08234,a07de1],
    // EXE-P-DAG:            gt:[{{.+}}]->[f08234,a07de1],
    // EXE-I-DAG:            {{^ *(\*gp:55->98, )?}}ga:[{{100|101}},{{200|202}}]->[101,202],
    // EXE-C-DAG:            {{^ *(\*gp:(56|98)->98, )?}}ga:[{{100|101}},{{200|202}}]->[101,202],
    // EXE-F-DAG:            {{^ *(\*gp:55->98, )?}}ga:[100,200]->[101,202],
    // EXE-P-DAG:            {{^ *(\*gp:.+->98, )?}}ga:[{{.+,.+}}]->[101,202],
    // EXE-IC-DAG:           gs:[{{33|42}},{{11|1}}]->[42,1], gu.i:{{22|13}}->13,
    // EXE-F-DAG:            gs:[33,11]->[42,1], gu.i:22->13,
    // EXE-P-DAG:            gs:[{{.+,.+}}]->[42,1], gu.i:{{.+}}->13,
    // EXE-IF-DAG:           li:99->98,
    // EXE-C-DAG:            li:{{99|98}}->98,
    // EXE-P-DAG:            li:{{.+}}->98,
    // EXE-IF-DAG:           lt:[7a1,62b0]->[79ca,2961],
    // EXE-C-DAG:            lt:[{{7a1|79ca}},{{62b0|2961}}]->[79ca,2961],
    // EXE-P-DAG:            lt:[{{.+}}]->[79ca,2961],
    // EXE-I-DAG:            {{^ *(\*lp:55->98, )?}}la:[{{77|55}},{{88|66}}]->[55,66],
    // EXE-C-DAG:            {{^ *(\*lp:(56|98)->98, )?}}la:[{{77|55}},{{88|66}}]->[55,66],
    // EXE-F-DAG:            {{^ *(\*lp:55->98, )?}}la:[77,88]->[55,66],
    // EXE-P-DAG:            {{^ *(\*lp:.+->98, )?}}la:[{{.+,.+}}]->[55,66],
    // EXE-IC-DAG:           ls:[{{222|333}},{{111|444}}]->[333,444], lu.i:{{167|279}}->279,
    // EXE-F-DAG:            ls:[222,111]->[333,444], lu.i:167->279,
    // EXE-P-DAG:            ls:[{{.+,.+}}]->[333,444], lu.i:{{.+}}->279,
    // EXE-DAG:              shadowed:
    // EXE-ICF-DAG:          ci:53, ca:[10,11,12]
    printf("inside : gi:%d->%d,\n"
           "         gt:[%lx,%lx]->[%lx,%lx],\n"
           "         "IF_PTR_EXE("*gp:%d->%d, ")"ga:[%d,%d]->[%d,%d],\n"
           "         gs:[%d,%d]->[%d,%d], gu.i:%d->%d,\n"
           "         li:%d->%d,\n"
           "         lt:[%lx,%lx]->[%lx,%lx],\n",
           giOld, gi,
           HI_UINT128(gtOld), LW_UINT128(gtOld),
           HI_UINT128(gt), LW_UINT128(gt),
#if CHECK_PTR_EXE
           *gpOld, *gp,
#endif
           gaOld[0], gaOld[1], ga[0], ga[1],
           gsOld.i, gsOld.j, gs.i, gs.j,
           guOld.i, gu.i,
           liOld, li,
           HI_UINT128(ltOld), LW_UINT128(ltOld),
           HI_UINT128(lt), LW_UINT128(lt));
    printf("         "IF_PTR_EXE("*lp:%d->%d, ")"la:[%d,%d]->[%d,%d],\n"
           "         ls:[%d,%d]->[%d,%d], lu.i:%d->%d,\n"
           "         shadowed:%d,\n"
#if MODE != MODE_P
           "         ci:%d, ca:[%d,%d,%d]\n"
#endif
           ,
#if CHECK_PTR_EXE
           *lpOld, *lp,
#endif
           laOld[0], laOld[1], la[0], la[1],
           lsOld.i, lsOld.j, ls.i, ls.j, luOld.i, lu.i,
           shadowed
#if MODE != MODE_P
           ,
           ci, ca[0], ca[1], ca[2]
#endif
           );
  } // PRT-NEXT: }
  // DMP: CallExpr
  //
  // EXE-IFP:      outside: gi:55->55,
  // EXE-C:        outside: gi:55->56,
  // EXE-IFP:               gt:[1400,59]->[1400,59],
  // EXE-C:                 gt:[1400,59]->[f08234,a07de1],
  // EXE-I-NEXT:            {{^ *(\*gp:55->55, )?}}ga:[100,200]->[101,202],
  // EXE-C-NEXT:            {{^ *(\*gp:56->98, )?}}ga:[100,200]->[101,202],
  // EXE-FP-NEXT:           {{^ *(\*gp:55->55, )?}}ga:[100,200]->[100,200],
  // EXE-IC-NEXT:           gs:[33,11]->[42,1], gu.i:22->13, gUnref:2->2,
  // EXE-FP-NEXT:           gs:[33,11]->[33,11], gu.i:22->22, gUnref:2->2,
  // EXE-IFP-NEXT:          li:99->99,
  // EXE-C-NEXT:            li:99->98,
  // EXE-IFP-NEXT:          lt:[7a1,62b0]->[7a1,62b0],
  // EXE-C-NEXT:            lt:[7a1,62b0]->[79ca,2961]
  // EXE-I-NEXT:            {{^ *(\*lp:55->55, )?}}la:[77,88]->[55,66],
  // EXE-C-NEXT:            {{^ *(\*lp:56->98, )?}}la:[77,88]->[55,66],
  // EXE-FP-NEXT:           {{^ *(\*lp:55->55, )?}}la:[77,88]->[77,88],
  // EXE-IC-NEXT:           ls:[222,111]->[333,444], lu.i:167->279, lUnref:9->9,
  // EXE-FP-NEXT:           ls:[222,111]->[222,111], lu.i:167->167, lUnref:9->9,
  // EXE-NEXT:              shadowed:111,
  // EXE-ICF-NEXT:          ci:53, ca:[10,11,12]
  printf("outside: gi:%d->%d,\n"
         "         gt:[%lx,%lx]->[%lx,%lx],\n"
         "         "IF_PTR_EXE("*gp:%d->%d, ")"ga:[%d,%d]->[%d,%d],\n"
         "         gs:[%d,%d]->[%d,%d], gu.i:%d->%d, gUnref:%d->%d,\n"
         "         li:%d->%d,\n"
         "         lt:[%lx,%lx]->[%lx,%lx],\n"
         "         "IF_PTR_EXE("*lp:%d->%d, ")"la:[%d,%d]->[%d,%d],\n"
         "         ls:[%d,%d]->[%d,%d], lu.i:%d->%d, lUnref:%d->%d,\n"
         "         shadowed:%d,\n"
#if MODE != MODE_P
         "         ci:%d, ca:[%d,%d,%d]\n"
#endif
         ,
         giOld, gi,
         HI_UINT128(gtOld), LW_UINT128(gtOld),
         HI_UINT128(gt), LW_UINT128(gt),
#if CHECK_PTR_EXE
         *gpOld, *gp,
#endif
         gaOld[0], gaOld[1], ga[0], ga[1],
         gsOld.i, gsOld.j, gs.i, gs.j, guOld.i, gu.i, gUnrefOld, gUnref,
         liOld, li,
         HI_UINT128(ltOld), LW_UINT128(ltOld),
         HI_UINT128(lt), LW_UINT128(lt),
#if CHECK_PTR_EXE
         *lpOld, *lp,
#endif
         laOld[0], laOld[1], la[0], la[1],
         lsOld.i, lsOld.j, ls.i, ls.j, luOld.i, lu.i, lUnrefOld, lUnref,
         shadowed
#if MODE != MODE_P
         ,
         ci, ca[0], ca[1], ca[2]
#endif
         );

  return 0;
}
// EXE-NOT:{{.}}

