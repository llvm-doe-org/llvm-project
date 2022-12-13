// Check implicit and explicit data attributes on "acc parallel" except:
// - Reductions are checked in parallel-reduction.c.
// - The present clause is checked in present.c.
// - The no_create clause is checked in no-create.c.
// - The effect of an enclosing "acc data" is checked in "data.c".
// - Member expressions are checked in parallel-da-member-expr.c and
//   parallel-da-member-expr.cpp.
//
// TODO: This test is cryptic.  Find some way to refactor it into multiple tests
// so it's easier to maintain.
//
// When ADD_LOOP_TO_PAR is not set, this file checks implicit and explicit
// data attributes on "acc parallel" without "loop".
//
// When ADD_LOOP_TO_PAR is set, it adds "loop seq" and a for loop to those "acc
// parallel" directives in order to check data attributes for combined "acc
// parallel loop" directives.

// DEFINE: %{check}( DIR %) =                                                                                                                                                        \
//                                CLANG_ARGS         FC_PRES
// DEFINE:   %{acc-check-dmp-fn}( -DMODE=MODE_I   %, I,ICCiCoCrF,%{DIR},%{DIR}-I,%{DIR}-ICCiCoCrF                                                                              %) && \
// DEFINE:   %{acc-check-dmp-fn}( -DMODE=MODE_C0  %, C,CCiCoCr,CCiCoCrF,CCiCoCrFP,ICCiCoCrF,%{DIR},%{DIR}-C,%{DIR}-CCiCoCr,%{DIR}-CCiCoCrF,%{DIR}-CCiCoCrFP,%{DIR}-ICCiCoCrF   %) && \
// DEFINE:   %{acc-check-dmp-fn}( -DMODE=MODE_C1  %, C,CCiCoCr,CCiCoCrF,CCiCoCrFP,ICCiCoCrF,%{DIR},%{DIR}-C,%{DIR}-CCiCoCr,%{DIR}-CCiCoCrF,%{DIR}-CCiCoCrFP,%{DIR}-ICCiCoCrF   %) && \
// DEFINE:   %{acc-check-dmp-fn}( -DMODE=MODE_C2  %, C,CCiCoCr,CCiCoCrF,CCiCoCrFP,ICCiCoCrF,%{DIR},%{DIR}-C,%{DIR}-CCiCoCr,%{DIR}-CCiCoCrF,%{DIR}-CCiCoCrFP,%{DIR}-ICCiCoCrF   %) && \
// DEFINE:   %{acc-check-dmp-fn}( -DMODE=MODE_Ci0 %, Ci,CCiCoCr,CCiCoCrF,CCiCoCrFP,ICCiCoCrF,%{DIR},%{DIR}-Ci,%{DIR}-CCiCoCr,%{DIR}-CCiCoCrF,%{DIR}-CCiCoCrFP,%{DIR}-ICCiCoCrF %) && \
// DEFINE:   %{acc-check-dmp-fn}( -DMODE=MODE_Ci1 %, Ci,CCiCoCr,CCiCoCrF,CCiCoCrFP,ICCiCoCrF,%{DIR},%{DIR}-Ci,%{DIR}-CCiCoCr,%{DIR}-CCiCoCrF,%{DIR}-CCiCoCrFP,%{DIR}-ICCiCoCrF %) && \
// DEFINE:   %{acc-check-dmp-fn}( -DMODE=MODE_Ci2 %, Ci,CCiCoCr,CCiCoCrF,CCiCoCrFP,ICCiCoCrF,%{DIR},%{DIR}-Ci,%{DIR}-CCiCoCr,%{DIR}-CCiCoCrF,%{DIR}-CCiCoCrFP,%{DIR}-ICCiCoCrF %) && \
// DEFINE:   %{acc-check-dmp-fn}( -DMODE=MODE_Co0 %, Co,CCiCoCr,CCiCoCrF,CCiCoCrFP,ICCiCoCrF,%{DIR},%{DIR}-Co,%{DIR}-CCiCoCr,%{DIR}-CCiCoCrF,%{DIR}-CCiCoCrFP,%{DIR}-ICCiCoCrF %) && \
// DEFINE:   %{acc-check-dmp-fn}( -DMODE=MODE_Co1 %, Co,CCiCoCr,CCiCoCrF,CCiCoCrFP,ICCiCoCrF,%{DIR},%{DIR}-Co,%{DIR}-CCiCoCr,%{DIR}-CCiCoCrF,%{DIR}-CCiCoCrFP,%{DIR}-ICCiCoCrF %) && \
// DEFINE:   %{acc-check-dmp-fn}( -DMODE=MODE_Co2 %, Co,CCiCoCr,CCiCoCrF,CCiCoCrFP,ICCiCoCrF,%{DIR},%{DIR}-Co,%{DIR}-CCiCoCr,%{DIR}-CCiCoCrF,%{DIR}-CCiCoCrFP,%{DIR}-ICCiCoCrF %) && \
// DEFINE:   %{acc-check-dmp-fn}( -DMODE=MODE_Cr0 %, Cr,CCiCoCr,CCiCoCrF,CCiCoCrFP,ICCiCoCrF,%{DIR},%{DIR}-Cr,%{DIR}-CCiCoCr,%{DIR}-CCiCoCrF,%{DIR}-CCiCoCrFP,%{DIR}-ICCiCoCrF %) && \
// DEFINE:   %{acc-check-dmp-fn}( -DMODE=MODE_Cr1 %, Cr,CCiCoCr,CCiCoCrF,CCiCoCrFP,ICCiCoCrF,%{DIR},%{DIR}-Cr,%{DIR}-CCiCoCr,%{DIR}-CCiCoCrF,%{DIR}-CCiCoCrFP,%{DIR}-ICCiCoCrF %) && \
// DEFINE:   %{acc-check-dmp-fn}( -DMODE=MODE_Cr2 %, Cr,CCiCoCr,CCiCoCrF,CCiCoCrFP,ICCiCoCrF,%{DIR},%{DIR}-Cr,%{DIR}-CCiCoCr,%{DIR}-CCiCoCrF,%{DIR}-CCiCoCrFP,%{DIR}-ICCiCoCrF %) && \
// DEFINE:   %{acc-check-dmp-fn}( -DMODE=MODE_F   %, F,CCiCoCrF,CCiCoCrFP,ICCiCoCrF,%{DIR},%{DIR}-F,%{DIR}-CCiCoCrF,%{DIR}-CCiCoCrFP,%{DIR}-ICCiCoCrF                          %) && \
// DEFINE:   %{acc-check-dmp-fn}( -DMODE=MODE_P   %, P,CCiCoCrFP,%{DIR},%{DIR}-P,%{DIR}-CCiCoCrFP                                                                              %) && \
//
//                                CLANG_ARGS         FC_PRES
// DEFINE:   %{acc-check-prt-fn}( -DMODE=MODE_I   %, I,%{DIR},%{DIR}-I     %) && \
// DEFINE:   %{acc-check-prt-fn}( -DMODE=MODE_C0  %, C0,%{DIR},%{DIR}-C0   %) && \
// DEFINE:   %{acc-check-prt-fn}( -DMODE=MODE_C1  %, C1,%{DIR},%{DIR}-C1   %) && \
// DEFINE:   %{acc-check-prt-fn}( -DMODE=MODE_C2  %, C2,%{DIR},%{DIR}-C2   %) && \
// DEFINE:   %{acc-check-prt-fn}( -DMODE=MODE_Ci0 %, Ci0,%{DIR},%{DIR}-Ci0 %) && \
// DEFINE:   %{acc-check-prt-fn}( -DMODE=MODE_Ci1 %, Ci1,%{DIR},%{DIR}-Ci1 %) && \
// DEFINE:   %{acc-check-prt-fn}( -DMODE=MODE_Ci2 %, Ci2,%{DIR},%{DIR}-Ci2 %) && \
// DEFINE:   %{acc-check-prt-fn}( -DMODE=MODE_Co0 %, Co0,%{DIR},%{DIR}-Co0 %) && \
// DEFINE:   %{acc-check-prt-fn}( -DMODE=MODE_Co1 %, Co1,%{DIR},%{DIR}-Co1 %) && \
// DEFINE:   %{acc-check-prt-fn}( -DMODE=MODE_Co2 %, Co2,%{DIR},%{DIR}-Co2 %) && \
// DEFINE:   %{acc-check-prt-fn}( -DMODE=MODE_Cr0 %, Cr0,%{DIR},%{DIR}-Cr0 %) && \
// DEFINE:   %{acc-check-prt-fn}( -DMODE=MODE_Cr1 %, Cr1,%{DIR},%{DIR}-Cr1 %) && \
// DEFINE:   %{acc-check-prt-fn}( -DMODE=MODE_Cr2 %, Cr2,%{DIR},%{DIR}-Cr2 %) && \
// DEFINE:   %{acc-check-prt-fn}( -DMODE=MODE_F   %, F,%{DIR},%{DIR}-F     %) && \
// DEFINE:   %{acc-check-prt-fn}( -DMODE=MODE_P   %, P,%{DIR},%{DIR}-P     %) && \
//
//           When we are targeting the host, memory is shared, so copy, copyin,
//           copyout, create, and their aliases all act like copy.
//                                CLANG_ARGS                          FC_PRES
// DEFINE:   %{acc-check-exe-fn}( -DMODE=MODE_I                    %, I,ICCi,ICCiF,ICCiCrFP,ICCo,ICiCrFP,IF                                                          %) && \
// DEFINE:   %{acc-check-exe-fn}( -DMODE=MODE_I   -DSTORAGE=static %, I,ICCi,ICCiF,ICCiCrFP,ICCo,ICiCrFP,IF                                                          %) && \
// DEFINE:   %{acc-check-exe-fn}( -DMODE=MODE_C0                   %, C,ICCi,ICCiF,ICCiCrFP,ICCo,CCi,CCo                                                             %) && \
// DEFINE:   %{acc-check-exe-fn}( -DMODE=MODE_C0  -DSTORAGE=static %, C,ICCi,ICCiF,ICCiCrFP,ICCo,CCi,CCo                                                             %) && \
// DEFINE:   %{acc-check-exe-fn}( -DMODE=MODE_C1                   %, C,ICCi,ICCiF,ICCiCrFP,ICCo,CCi,CCo                                                             %) && \
// DEFINE:   %{acc-check-exe-fn}( -DMODE=MODE_C1  -DSTORAGE=static %, C,ICCi,ICCiF,ICCiCrFP,ICCo,CCi,CCo                                                             %) && \
// DEFINE:   %{acc-check-exe-fn}( -DMODE=MODE_C2                   %, C,ICCi,ICCiF,ICCiCrFP,ICCo,CCi,CCo                                                             %) && \
// DEFINE:   %{acc-check-exe-fn}( -DMODE=MODE_C2  -DSTORAGE=static %, C,ICCi,ICCiF,ICCiCrFP,ICCo,CCi,CCo                                                             %) && \
// DEFINE:   %{acc-check-exe-fn}( -DMODE=MODE_Ci0                  %, %if-tgt-host<C|Ci>,ICCi,ICCiF,ICCiCrFP,%if-tgt-host<ICCo|ICiCrFP>,CCi,%if-tgt-host<CCo|CiCrFP> %) && \
// DEFINE:   %{acc-check-exe-fn}( -DMODE=MODE_Ci0 -DSTORAGE=static %, %if-tgt-host<C|Ci>,ICCi,ICCiF,ICCiCrFP,%if-tgt-host<ICCo|ICiCrFP>,CCi,%if-tgt-host<CCo|CiCrFP> %) && \
// DEFINE:   %{acc-check-exe-fn}( -DMODE=MODE_Ci1                  %, %if-tgt-host<C|Ci>,ICCi,ICCiF,ICCiCrFP,%if-tgt-host<ICCo|ICiCrFP>,CCi,%if-tgt-host<CCo|CiCrFP> %) && \
// DEFINE:   %{acc-check-exe-fn}( -DMODE=MODE_Ci1 -DSTORAGE=static %, %if-tgt-host<C|Ci>,ICCi,ICCiF,ICCiCrFP,%if-tgt-host<ICCo|ICiCrFP>,CCi,%if-tgt-host<CCo|CiCrFP> %) && \
// DEFINE:   %{acc-check-exe-fn}( -DMODE=MODE_Ci2                  %, %if-tgt-host<C|Ci>,ICCi,ICCiF,ICCiCrFP,%if-tgt-host<ICCo|ICiCrFP>,CCi,%if-tgt-host<CCo|CiCrFP> %) && \
// DEFINE:   %{acc-check-exe-fn}( -DMODE=MODE_Ci2 -DSTORAGE=static %, %if-tgt-host<C|Ci>,ICCi,ICCiF,ICCiCrFP,%if-tgt-host<ICCo|ICiCrFP>,CCi,%if-tgt-host<CCo|CiCrFP> %) && \
// DEFINE:   %{acc-check-exe-fn}( -DMODE=MODE_Co0                  %, %if-tgt-host<C,ICCi,ICCiF,ICCiCrFP,ICCo,CCi,CCo|Co,ICCo,CCo,CoCrP>                             %) && \
// DEFINE:   %{acc-check-exe-fn}( -DMODE=MODE_Co0 -DSTORAGE=static %, %if-tgt-host<C,ICCi,ICCiF,ICCiCrFP,ICCo,CCi,CCo|Co,ICCo,CCo,CoCrP>                             %) && \
// DEFINE:   %{acc-check-exe-fn}( -DMODE=MODE_Co1                  %, %if-tgt-host<C,ICCi,ICCiF,ICCiCrFP,ICCo,CCi,CCo|Co,ICCo,CCo,CoCrP>                             %) && \
// DEFINE:   %{acc-check-exe-fn}( -DMODE=MODE_Co1 -DSTORAGE=static %, %if-tgt-host<C,ICCi,ICCiF,ICCiCrFP,ICCo,CCi,CCo|Co,ICCo,CCo,CoCrP>                             %) && \
// DEFINE:   %{acc-check-exe-fn}( -DMODE=MODE_Co2                  %, %if-tgt-host<C,ICCi,ICCiF,ICCiCrFP,ICCo,CCi,CCo|Co,ICCo,CCo,CoCrP>                             %) && \
// DEFINE:   %{acc-check-exe-fn}( -DMODE=MODE_Co2 -DSTORAGE=static %, %if-tgt-host<C,ICCi,ICCiF,ICCiCrFP,ICCo,CCi,CCo|Co,ICCo,CCo,CoCrP>                             %) && \
// DEFINE:   %{acc-check-exe-fn}( -DMODE=MODE_F                    %, F,ICCiF,ICCiCrFP,ICiCrFP,CiCrFP,IF                                                             %) && \
// DEFINE:   %{acc-check-exe-fn}( -DMODE=MODE_F   -DSTORAGE=static %, F,ICCiF,ICCiCrFP,ICiCrFP,CiCrFP,IF                                                             %) && \
// DEFINE:   %{acc-check-exe-fn}( -DMODE=MODE_P                    %, P,%if-tgt-host<CoCrP,|>ICCiCrFP,ICiCrFP,CiCrFP                                                 %) && \
// DEFINE:   %{acc-check-exe-fn}( -DMODE=MODE_P   -DSTORAGE=static %, P,%if-tgt-host<CoCrP,|>ICCiCrFP,ICiCrFP,CiCrFP                                                 %)

// REDEFINE: %{all:clang:args-stable} =
//      RUN: %{check}( PAR %)
// REDEFINE: %{all:clang:args-stable} = -DADD_LOOP_TO_PAR
//      RUN: %{check}( PARLOOP %)

// Check that variables aren't passed to the outlined function in the case of
// the private clause.  However, for acc parallel loop, the private clause is
// on the effective parallel directive not the effective loop directive, so
// sometimes variables are passed to the outlined function, so don't test that
// case.
//
// DEFINE: %{check-llvm}( CFLAGS %) =                                          \
// DEFINE:   %if-exe-host %clang -Xclang -verify -fopenacc -S -emit-llvm       \
// DEFINE:     -o %t.llvm %{CFLAGS} %s &&                                      \
// DEFINE:   %if-exe-host FileCheck -input-file %t.llvm -check-prefixes=LLVM %s
//
// RUN: %{check-llvm}( -DMODE=MODE_P                  %)
// RUN: %{check-llvm}( -DMODE=MODE_P -DSTORAGE=static %)
//
// LLVM: define internal void @.omp_outlined.
// LLVM-SAME: (i{{[0-9]+}}* {{[a-z ]+}} %{{[^, )]*}},
// LLVM-SAME: {{^ *}} i{{[0-9]+}}* {{[a-z ]+}} %{{[^, )]*}})

// END.

/* expected-no-diagnostics */

#include <stdio.h>
#include <stdint.h>

#if TGT_USE_STDIO
# define TGT_PRINTF(...) printf(__VA_ARGS__)
#else
# define TGT_PRINTF(...)
#endif

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

#define MODE_I   1
#define MODE_C0  2
#define MODE_C1  3
#define MODE_C2  4
#define MODE_Ci0 5
#define MODE_Ci1 6
#define MODE_Ci2 7
#define MODE_Co0 8
#define MODE_Co1 9
#define MODE_Co2 10
#define MODE_Cr0 11
#define MODE_Cr1 12
#define MODE_Cr2 13
#define MODE_F   14
#define MODE_P   15

#ifndef MODE
# error MODE undefined
#elif MODE == MODE_I
# define CLAUSE(...)
#elif MODE == MODE_C0
# define CLAUSE(...) copy(__VA_ARGS__)
#elif MODE == MODE_C1
# define CLAUSE(...) pcopy(__VA_ARGS__)
#elif MODE == MODE_C2
# define CLAUSE(...) present_or_copy(__VA_ARGS__)
#elif MODE == MODE_Ci0
# define CLAUSE(...) copyin(__VA_ARGS__)
#elif MODE == MODE_Ci1
# define CLAUSE(...) pcopyin(__VA_ARGS__)
#elif MODE == MODE_Ci2
# define CLAUSE(...) present_or_copyin(__VA_ARGS__)
#elif MODE == MODE_Co0
# define CLAUSE(...) copyout(__VA_ARGS__)
#elif MODE == MODE_Co1
# define CLAUSE(...) pcopyout(__VA_ARGS__)
#elif MODE == MODE_Co2
# define CLAUSE(...) present_or_copyout(__VA_ARGS__)
#elif MODE == MODE_Cr0
# define CLAUSE(...) create(__VA_ARGS__)
#elif MODE == MODE_Cr1
# define CLAUSE(...) pcreate(__VA_ARGS__)
#elif MODE == MODE_Cr2
# define CLAUSE(...) present_or_create(__VA_ARGS__)
#elif MODE == MODE_F
# define CLAUSE(...) firstprivate(__VA_ARGS__)
#elif MODE == MODE_P
# define CLAUSE(...) private(__VA_ARGS__)
#else
# error unknown MODE
#endif

// If __uint128_t is available, test it,  Otherwise, fake it.
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

// Passing const variable to copyout, create, or private isn't permitted.
#if MODE == MODE_Co0 || MODE == MODE_Co1 || MODE == MODE_Co2 || \
    MODE == MODE_Cr0 || MODE == MODE_Cr1 || MODE == MODE_Cr2 || MODE == MODE_P
# define CONST
#else
# define CONST const
#endif

// FIXME: When OpenMP offloading is activated by -fopenmp-targets, pointers
// pass into acc parallel as null, but otherwise they pass in just fine.
// What does the OpenMP spec say is supposed to happen?
#if TGT_HOST
# define DEREF_PTR(Ptr) (*(Ptr))
#else
# define DEREF_PTR(Ptr) 9999
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

  // Const scalars and non-scalars should be fine with either implicit or
  // explicit data attributes.
  CONST int ci = 53;
  CONST int ca[3] = {10, 11, 12};

  // PRT: int START_OF_PRT;
  int START_OF_PRT;

  //                DMP-PARLOOP: ACCParallelLoopDirective
  //           DMP-PARLOOP-NEXT:   ACCSeqClause
  //           DMP-PARLOOP-NEXT:   ACCNumGangsClause
  //           DMP-PARLOOP-NEXT:     IntegerLiteral {{.*}} 'int' 2
  //         DMP-PARLOOP-C-NEXT:   ACCCopyClause
  //        DMP-PARLOOP-Ci-NEXT:   ACCCopyinClause
  //        DMP-PARLOOP-Co-NEXT:   ACCCopyoutClause
  //        DMP-PARLOOP-Cr-NEXT:   ACCCreateClause
  //         DMP-PARLOOP-F-NEXT:   ACCFirstprivateClause
  //         DMP-PARLOOP-P-NEXT:   ACCPrivateClause
  //  DMP-PARLOOP-CCiCoCrFP-NOT:     <implicit>
  // DMP-PARLOOP-CCiCoCrFP-SAME:     {{$}}
  // DMP-PARLOOP-CCiCoCrFP-NEXT:     DeclRefExpr {{.*}} 'gi' 'int'
  // DMP-PARLOOP-CCiCoCrFP-NEXT:     DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  // DMP-PARLOOP-CCiCoCrFP-NEXT:     DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-PARLOOP-CCiCoCrFP-NEXT:     DeclRefExpr {{.*}} 'ga' 'int[2]'
  // DMP-PARLOOP-CCiCoCrFP-NEXT:     DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-PARLOOP-CCiCoCrFP-NEXT:     DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-PARLOOP-CCiCoCrFP-NEXT:     DeclRefExpr {{.*}} 'gUnref' 'int'
  //         DMP-PARLOOP-C-NEXT:   ACCCopyClause
  //        DMP-PARLOOP-Ci-NEXT:   ACCCopyinClause
  //        DMP-PARLOOP-Co-NEXT:   ACCCopyoutClause
  //        DMP-PARLOOP-Cr-NEXT:   ACCCreateClause
  //         DMP-PARLOOP-F-NEXT:   ACCFirstprivateClause
  //         DMP-PARLOOP-P-NEXT:   ACCPrivateClause
  //  DMP-PARLOOP-CCiCoCrFP-NOT:     <implicit>
  // DMP-PARLOOP-CCiCoCrFP-SAME:     {{$}}
  // DMP-PARLOOP-CCiCoCrFP-NEXT:     DeclRefExpr {{.*}} 'li' 'int'
  // DMP-PARLOOP-CCiCoCrFP-NEXT:     DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  // DMP-PARLOOP-CCiCoCrFP-NEXT:     DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-PARLOOP-CCiCoCrFP-NEXT:     DeclRefExpr {{.*}} 'la' 'int[2]'
  // DMP-PARLOOP-CCiCoCrFP-NEXT:     DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-PARLOOP-CCiCoCrFP-NEXT:     DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-PARLOOP-CCiCoCrFP-NEXT:     DeclRefExpr {{.*}} 'lUnref' 'int'
  //         DMP-PARLOOP-C-NEXT:   ACCCopyClause
  //        DMP-PARLOOP-Ci-NEXT:   ACCCopyinClause
  //        DMP-PARLOOP-Co-NEXT:   ACCCopyoutClause
  //        DMP-PARLOOP-Cr-NEXT:   ACCCreateClause
  //         DMP-PARLOOP-F-NEXT:   ACCFirstprivateClause
  //         DMP-PARLOOP-P-NEXT:   ACCPrivateClause
  //  DMP-PARLOOP-CCiCoCrFP-NOT:     <implicit>
  // DMP-PARLOOP-CCiCoCrFP-SAME:     {{$}}
  // DMP-PARLOOP-CCiCoCrFP-NEXT:     DeclRefExpr {{.*}} 'shadowed' 'int'
  // DMP-PARLOOP-CCiCoCrFP-NEXT:     DeclRefExpr {{.*}} 'ci' '{{(const )?}}int'
  // DMP-PARLOOP-CCiCoCrFP-NEXT:     DeclRefExpr {{.*}} 'ca' '{{(const )?}}int[3]'

  //           DMP-PARLOOP-NEXT:   effect: ACCParallelDirective
  //                    DMP-PAR:   ACCParallelDirective
  //                   DMP-NEXT:     ACCNumGangsClause
  //                   DMP-NEXT:       IntegerLiteral {{.*}} 'int' 2

  //                 DMP-C-NEXT:     ACCCopyClause
  //                DMP-Ci-NEXT:     ACCCopyinClause
  //                DMP-Co-NEXT:     ACCCopyoutClause
  //                DMP-Cr-NEXT:     ACCCreateClause
  //                 DMP-F-NEXT:     ACCFirstprivateClause
  //           DMP-CCiCoCrF-NOT:       <implicit>
  //          DMP-CCiCoCrF-SAME:       {{$}}
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'gi' 'int'
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'gp' 'const int *'
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'ga' 'int[2]'
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'gUnref' 'int'
  //                 DMP-C-NEXT:     ACCCopyClause
  //                DMP-Ci-NEXT:     ACCCopyinClause
  //                DMP-Co-NEXT:     ACCCopyoutClause
  //                DMP-Cr-NEXT:     ACCCreateClause
  //                 DMP-F-NEXT:     ACCFirstprivateClause
  //           DMP-CCiCoCrF-NOT:       <implicit>
  //          DMP-CCiCoCrF-SAME:       {{$}}
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'li' 'int'
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'lp' 'const int *'
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'la' 'int[2]'
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'lUnref' 'int'
  //                 DMP-C-NEXT:     ACCCopyClause
  //                DMP-Ci-NEXT:     ACCCopyinClause
  //                DMP-Co-NEXT:     ACCCopyoutClause
  //                DMP-Cr-NEXT:     ACCCreateClause
  //                 DMP-F-NEXT:     ACCFirstprivateClause
  //           DMP-CCiCoCrF-NOT:       <implicit>
  //          DMP-CCiCoCrF-SAME:       {{$}}
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'shadowed' 'int'
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'ci' '{{(const )?}}int'
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'ca' '{{(const )?}}int[3]'
  //           DMP-CCiCoCr-NEXT:     ACCSharedClause {{.*}} <implicit>
  //                 DMP-F-NEXT:     ACCNomapClause {{.*}} <implicit>
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'gi' 'int'
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'gp' 'const int *'
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'ga' 'int[2]'
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'li' 'int'
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'lp' 'const int *'
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'la' 'int[2]'
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'shadowed' 'int'
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'ci' '{{(const )?}}int'
  //          DMP-CCiCoCrF-NEXT:       DeclRefExpr {{.*}} 'ca' '{{(const )?}}int[3]'
  //           DMP-CCiCoCrF-NOT:       DeclRefExpr {{.*}} 'gUnref' 'int'
  //           DMP-CCiCoCrF-NOT:       DeclRefExpr {{.*}} 'lUnref' 'int'

  //             DMP-PAR-P-NEXT:     ACCPrivateClause
  //              DMP-PAR-P-NOT:       <implicit>
  //             DMP-PAR-P-SAME:       {{$}}
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'gi' 'int'
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'gp' 'const int *'
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'ga' 'int[2]'
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'gUnref' 'int'
  //             DMP-PAR-P-NEXT:     ACCPrivateClause
   //             DMP-PAR-P-NOT:       <implicit>
  //             DMP-PAR-P-SAME:       {{$}}
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'li' 'int'
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'lp' 'const int *'
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'la' 'int[2]'
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'lUnref' 'int'
  //             DMP-PAR-P-NEXT:     ACCPrivateClause
  //              DMP-PAR-P-NOT:       <implicit>
  //             DMP-PAR-P-SAME:       {{$}}
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'shadowed' 'int'
                                       // The test omits const when testing private.
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'ci' 'int'
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'ca' 'int[3]'
  //             DMP-PAR-P-NEXT:     ACCNomapClause {{.*}} <implicit>
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'gi' 'int'
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'gp' 'const int *'
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'ga' 'int[2]'
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'li' 'int'
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'lp' 'const int *'
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'la' 'int[2]'
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'shadowed' 'int'
                                       // The test omits const when testing private.
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'ci' 'int'
  //             DMP-PAR-P-NEXT:       DeclRefExpr {{.*}} 'ca' 'int[3]'
  //              DMP-PAR-P-NOT:       DeclRefExpr {{.*}} 'gUnref' 'int'
  //              DMP-PAR-P-NOT:       DeclRefExpr {{.*}} 'lUnref' 'int'

  //                 DMP-I-NEXT:     ACCNomapClause {{.*}} <implicit>
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} 'gi' 'int'
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} 'gp' 'const int *'
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} 'li' 'int'
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} 'lp' 'const int *'
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} 'shadowed' 'int'
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} 'ci' 'const int'
  //                 DMP-I-NEXT:     ACCCopyClause {{.*}} <implicit>
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} 'ga' 'int[2]'
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} 'la' 'int[2]'
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} 'ca' 'const int[3]'
  //                 DMP-I-NEXT:     ACCSharedClause {{.*}} <implicit>
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} 'ga' 'int[2]'
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} 'la' 'int[2]'
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} 'ca' 'const int[3]'
  //                 DMP-I-NEXT:     ACCFirstprivateClause {{.*}} <implicit>
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} 'gi' 'int'
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} 'gp' 'const int *'
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} 'li' 'int'
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} 'lp' 'const int *'
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} 'shadowed' 'int'
  //                 DMP-I-NEXT:       DeclRefExpr {{.*}} 'ci' 'const int'

  //                   DMP-NEXT:     impl: OMPTargetTeamsDirective
  //                   DMP-NEXT:       OMPNum_teamsClause
  //                   DMP-NEXT:         IntegerLiteral {{.*}} 'int' 2

  //           DMP-CCiCoCr-NEXT:       OMPMapClause
  //                 DMP-F-NEXT:       OMPFirstprivateClause
  //           DMP-CCiCoCrF-NOT:         <implicit>
  //          DMP-CCiCoCrF-SAME:         {{$}}
  //          DMP-CCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'gi' 'int'
  //          DMP-CCiCoCrF-NEXT:         DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  //          DMP-CCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'gp' 'const int *'
  //          DMP-CCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'ga' 'int[2]'
  //          DMP-CCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  //          DMP-CCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  //          DMP-CCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'gUnref' 'int'
  //           DMP-CCiCoCr-NEXT:       OMPMapClause
  //                 DMP-F-NEXT:       OMPFirstprivateClause
  //           DMP-CCiCoCrF-NOT:         <implicit>
  //          DMP-CCiCoCrF-SAME:         {{$}}
  //          DMP-CCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'li' 'int'
  //          DMP-CCiCoCrF-NEXT:         DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  //          DMP-CCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'lp' 'const int *'
  //          DMP-CCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'la' 'int[2]'
  //          DMP-CCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  //          DMP-CCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  //          DMP-CCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'lUnref' 'int'
  //           DMP-CCiCoCr-NEXT:       OMPMapClause
  //                 DMP-F-NEXT:       OMPFirstprivateClause
  //           DMP-CCiCoCrF-NOT:         <implicit>
  //          DMP-CCiCoCrF-SAME:         {{$}}
  //          DMP-CCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'shadowed' 'int'
  //          DMP-CCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'ci' '{{(const )?}}int'
  //          DMP-CCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'ca' '{{(const )?}}int[3]'

  //             DMP-PAR-P-NEXT:       OMPPrivateClause
  //              DMP-PAR-P-NOT:         <implicit>
  //             DMP-PAR-P-SAME:         {{$}}
  //             DMP-PAR-P-NEXT:         DeclRefExpr {{.*}} 'gi' 'int'
  //             DMP-PAR-P-NEXT:         DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  //             DMP-PAR-P-NEXT:         DeclRefExpr {{.*}} 'gp' 'const int *'
  //             DMP-PAR-P-NEXT:         DeclRefExpr {{.*}} 'ga' 'int[2]'
  //             DMP-PAR-P-NEXT:         DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  //             DMP-PAR-P-NEXT:         DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  //             DMP-PAR-P-NEXT:         DeclRefExpr {{.*}} 'gUnref' 'int'
  //             DMP-PAR-P-NEXT:       OMPPrivateClause
  //              DMP-PAR-P-NOT:         <implicit>
  //             DMP-PAR-P-SAME:         {{$}}
  //             DMP-PAR-P-NEXT:         DeclRefExpr {{.*}} 'li' 'int'
  //             DMP-PAR-P-NEXT:         DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  //             DMP-PAR-P-NEXT:         DeclRefExpr {{.*}} 'lp' 'const int *'
  //             DMP-PAR-P-NEXT:         DeclRefExpr {{.*}} 'la' 'int[2]'
  //             DMP-PAR-P-NEXT:         DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  //             DMP-PAR-P-NEXT:         DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  //             DMP-PAR-P-NEXT:         DeclRefExpr {{.*}} 'lUnref' 'int'
  //             DMP-PAR-P-NEXT:       OMPPrivateClause
  //              DMP-PAR-P-NOT:         <implicit>
  //             DMP-PAR-P-SAME:         {{$}}
  //             DMP-PAR-P-NEXT:         DeclRefExpr {{.*}} 'shadowed' 'int'
                                         // The test omits const when testing private.
  //             DMP-PAR-P-NEXT:         DeclRefExpr {{.*}} 'ci' 'int'
  //             DMP-PAR-P-NEXT:         DeclRefExpr {{.*}} 'ca' 'int[3]'

  //                 DMP-I-NEXT:       OMPMapClause
  //                  DMP-I-NOT:         <implicit>
  //                 DMP-I-SAME:         {{$}}
  //                 DMP-I-NEXT:         DeclRefExpr {{.*}} 'ga' 'int[2]'
  //                 DMP-I-NEXT:         DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  //                 DMP-I-NEXT:         DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  //                 DMP-I-NEXT:         DeclRefExpr {{.*}} 'la' 'int[2]'
  //                 DMP-I-NEXT:         DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  //                 DMP-I-NEXT:         DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  //                 DMP-I-NEXT:         DeclRefExpr {{.*}} 'ca' 'const int[3]'
  //                 DMP-I-NEXT:       OMPFirstprivateClause
  //                  DMP-I-NOT:         <implicit>
  //                 DMP-I-SAME:         {{$}}
  //                 DMP-I-NEXT:         DeclRefExpr {{.*}} 'gi' 'int'
  //                 DMP-I-NEXT:         DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  //                 DMP-I-NEXT:         DeclRefExpr {{.*}} 'gp' 'const int *'
  //                 DMP-I-NEXT:         DeclRefExpr {{.*}} 'li' 'int'
  //                 DMP-I-NEXT:         DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  //                 DMP-I-NEXT:         DeclRefExpr {{.*}} 'lp' 'const int *'
  //                 DMP-I-NEXT:         DeclRefExpr {{.*}} 'shadowed' 'int'
  //                 DMP-I-NEXT:         DeclRefExpr {{.*}} 'ci' 'const int'

  //                DMP-PARLOOP:     ACCLoopDirective
  //           DMP-PARLOOP-NEXT:       ACCSeqClause

  //         DMP-PARLOOP-P-NEXT:       ACCPrivateClause
  //          DMP-PARLOOP-P-NOT:         <implicit>
  //         DMP-PARLOOP-P-SAME:         {{$}}
  //         DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'gi' 'int'
  //         DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  //         DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'gp' 'const int *'
  //         DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'ga' 'int[2]'
  //         DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  //         DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  //         DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'gUnref' 'int'
  //         DMP-PARLOOP-P-NEXT:       ACCPrivateClause
  //          DMP-PARLOOP-P-NOT:         <implicit>
  //         DMP-PARLOOP-P-SAME:         {{$}}
  //         DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'li' 'int'
  //         DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  //         DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'lp' 'const int *'
  //         DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'la' 'int[2]'
  //         DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  //         DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  //         DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'lUnref' 'int'
  //         DMP-PARLOOP-P-NEXT:       ACCPrivateClause
  //          DMP-PARLOOP-P-NOT:         <implicit>
  //         DMP-PARLOOP-P-SAME:         {{$}}
  //         DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'shadowed' 'int'
                                         // The test omits const when testing private.
  //         DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'ci' 'int'
  //         DMP-PARLOOP-P-NEXT:         DeclRefExpr {{.*}} 'ca' 'int[3]'

  // DMP-PARLOOP-ICCiCoCrF-NEXT:       ACCSharedClause {{.*}} <implicit>
  // DMP-PARLOOP-ICCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'gi' 'int'
  // DMP-PARLOOP-ICCiCoCrF-NEXT:         DeclRefExpr {{.*}} {{'gt' '__uint128_t'|'gt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'gt_lw' 'uint64_t'}}
  // DMP-PARLOOP-ICCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'gp' 'const int *'
  // DMP-PARLOOP-ICCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'ga' 'int[2]'
  // DMP-PARLOOP-ICCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
  // DMP-PARLOOP-ICCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'gu' 'union U':'union U'
  // DMP-PARLOOP-ICCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'li' 'int'
  // DMP-PARLOOP-ICCiCoCrF-NEXT:         DeclRefExpr {{.*}} {{'lt' '__uint128_t'|'lt_hi' 'uint64_t'$[[:space:]]*DeclRefExpr .* 'lt_lw' 'uint64_t'}}
  // DMP-PARLOOP-ICCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'lp' 'const int *'
  // DMP-PARLOOP-ICCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'la' 'int[2]'
  // DMP-PARLOOP-ICCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
  // DMP-PARLOOP-ICCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'lu' 'union U':'union U'
  // DMP-PARLOOP-ICCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'shadowed' 'int'
  // DMP-PARLOOP-ICCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'ci' '{{(const )?}}int'
  // DMP-PARLOOP-ICCiCoCrF-NEXT:         DeclRefExpr {{.*}} 'ca' '{{(const )?}}int[3]'

  // DMP-PARLOOP-ICCiCoCrF-NEXT:       impl: ForStmt
  //         DMP-PARLOOP-P-NEXT:       impl: CompoundStmt
  //         DMP-PARLOOP-P-NEXT:         DeclStmt
  //         DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} gi 'int'
  //         DMP-PARLOOP-P-NEXT:         DeclStmt
  //         DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} {{gt '__uint128_t'|gt_hi 'uint64_t'$[[:space:]]*VarDecl .* gt_lw 'uint64_t'}}
  //         DMP-PARLOOP-P-NEXT:         DeclStmt
  //         DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} gp 'const int *'
  //         DMP-PARLOOP-P-NEXT:         DeclStmt
  //         DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} ga 'int[2]'
  //         DMP-PARLOOP-P-NEXT:         DeclStmt
  //         DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} gs 'struct S':'struct S'
  //         DMP-PARLOOP-P-NEXT:         DeclStmt
  //         DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} gu 'union U':'union U'
  //         DMP-PARLOOP-P-NEXT:         DeclStmt
  //         DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} gUnref 'int'
  //         DMP-PARLOOP-P-NEXT:         DeclStmt
  //         DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} li 'int'
  //         DMP-PARLOOP-P-NEXT:         DeclStmt
  //         DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} {{lt '__uint128_t'|lt_hi 'uint64_t'$[[:space:]]*VarDecl .* lt_lw 'uint64_t'}}
  //         DMP-PARLOOP-P-NEXT:         DeclStmt
  //         DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} lp 'const int *'
  //         DMP-PARLOOP-P-NEXT:         DeclStmt
  //         DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} la 'int[2]'
  //         DMP-PARLOOP-P-NEXT:         DeclStmt
  //         DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} ls 'struct S':'struct S'
  //         DMP-PARLOOP-P-NEXT:         DeclStmt
  //         DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} lu 'union U':'union U'
  //         DMP-PARLOOP-P-NEXT:         DeclStmt
  //         DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} lUnref 'int'
  //         DMP-PARLOOP-P-NEXT:         DeclStmt
  //         DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} shadowed 'int'
                                         // The test omits const when testing private.
  //         DMP-PARLOOP-P-NEXT:         DeclStmt
  //         DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} ci 'int'
  //         DMP-PARLOOP-P-NEXT:         DeclStmt
  //         DMP-PARLOOP-P-NEXT:           VarDecl {{.*}} ca 'int[3]'
  //         DMP-PARLOOP-P-NEXT:         ForStmt

  // PRT-NOT:       #pragma
  //
  //               PRT-A-I:  {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2){{(.*\\$[[:space:]])*.*$}}
  //         PRT-AO-I-NEXT:  {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: ga,gs,gu,la,ls,lu,ca) firstprivate(gi,gt{{(_hi,gt_lw)?}},gp,li,lt{{(_hi,lt_lw)?}},lp,shadowed,ci){{$}}
  //               PRT-O-I:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: ga,gs,gu,la,ls,lu,ca) firstprivate(gi,gt{{(_hi,gt_lw)?}},gp,li,lt{{(_hi,lt_lw)?}},lp,shadowed,ci){{$}}
  //         PRT-OA-I-NEXT:  {{^ *}}// #pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2){{(.*\\$[[:space:]])*.*$}}
  //
  //              PRT-A-C0:  {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{copy\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) copy\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) copy\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //              PRT-A-C1:  {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{pcopy\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) pcopy\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) pcopy\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //              PRT-A-C2:  {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{present_or_copy\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) present_or_copy\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) present_or_copy\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //             PRT-A-Ci0:  {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{copyin\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) copyin\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) copyin\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //             PRT-A-Ci1:  {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{pcopyin\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) pcopyin\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) pcopyin\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //             PRT-A-Ci2:  {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{present_or_copyin\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) present_or_copyin\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) present_or_copyin\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //             PRT-A-Co0:  {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{copyout\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) copyout\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) copyout\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //             PRT-A-Co1:  {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{pcopyout\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) pcopyout\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) pcopyout\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //             PRT-A-Co2:  {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{present_or_copyout\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) present_or_copyout\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) present_or_copyout\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //             PRT-A-Cr0:  {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{create\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) create\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) create\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //             PRT-A-Cr1:  {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{pcreate\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) pcreate\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) pcreate\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //             PRT-A-Cr2:  {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{present_or_create\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) present_or_create\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) present_or_create\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //        PRT-AO-C0-NEXT:  {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(ompx_hold,tofrom: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(ompx_hold,tofrom: shadowed,ci,ca){{$}}
  //        PRT-AO-C1-NEXT:  {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(ompx_hold,tofrom: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(ompx_hold,tofrom: shadowed,ci,ca){{$}}
  //        PRT-AO-C2-NEXT:  {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,tofrom: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(ompx_hold,tofrom: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(ompx_hold,tofrom: shadowed,ci,ca){{$}}
  //       PRT-AO-Ci0-NEXT:  {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,to: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(ompx_hold,to: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(ompx_hold,to: shadowed,ci,ca){{$}}
  //       PRT-AO-Ci1-NEXT:  {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,to: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(ompx_hold,to: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(ompx_hold,to: shadowed,ci,ca){{$}}
  //       PRT-AO-Ci2-NEXT:  {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,to: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(ompx_hold,to: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(ompx_hold,to: shadowed,ci,ca){{$}}
  //       PRT-AO-Co0-NEXT:  {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,from: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(ompx_hold,from: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(ompx_hold,from: shadowed,ci,ca){{$}}
  //       PRT-AO-Co1-NEXT:  {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,from: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(ompx_hold,from: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(ompx_hold,from: shadowed,ci,ca){{$}}
  //       PRT-AO-Co2-NEXT:  {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,from: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(ompx_hold,from: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(ompx_hold,from: shadowed,ci,ca){{$}}
  //       PRT-AO-Cr0-NEXT:  {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,alloc: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(ompx_hold,alloc: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(ompx_hold,alloc: shadowed,ci,ca){{$}}
  //       PRT-AO-Cr1-NEXT:  {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,alloc: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(ompx_hold,alloc: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(ompx_hold,alloc: shadowed,ci,ca){{$}}
  //       PRT-AO-Cr2-NEXT:  {{^ *}}// #pragma omp target teams num_teams(2) map(ompx_hold,alloc: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(ompx_hold,alloc: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(ompx_hold,alloc: shadowed,ci,ca){{$}}
  //              PRT-O-C0:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(ompx_hold,tofrom: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(ompx_hold,tofrom: shadowed,ci,ca){{$}}
  //              PRT-O-C1:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(ompx_hold,tofrom: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(ompx_hold,tofrom: shadowed,ci,ca){{$}}
  //              PRT-O-C2:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,tofrom: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(ompx_hold,tofrom: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(ompx_hold,tofrom: shadowed,ci,ca){{$}}
  //             PRT-O-Ci0:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,to: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(ompx_hold,to: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(ompx_hold,to: shadowed,ci,ca){{$}}
  //             PRT-O-Ci1:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,to: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(ompx_hold,to: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(ompx_hold,to: shadowed,ci,ca){{$}}
  //             PRT-O-Ci2:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,to: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(ompx_hold,to: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(ompx_hold,to: shadowed,ci,ca){{$}}
  //             PRT-O-Co0:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,from: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(ompx_hold,from: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(ompx_hold,from: shadowed,ci,ca){{$}}
  //             PRT-O-Co1:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,from: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(ompx_hold,from: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(ompx_hold,from: shadowed,ci,ca){{$}}
  //             PRT-O-Co2:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,from: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(ompx_hold,from: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(ompx_hold,from: shadowed,ci,ca){{$}}
  //             PRT-O-Cr0:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,alloc: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(ompx_hold,alloc: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(ompx_hold,alloc: shadowed,ci,ca){{$}}
  //             PRT-O-Cr1:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,alloc: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(ompx_hold,alloc: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(ompx_hold,alloc: shadowed,ci,ca){{$}}
  //             PRT-O-Cr2:  {{^ *}}#pragma omp target teams num_teams(2) map(ompx_hold,alloc: gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) map(ompx_hold,alloc: li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) map(ompx_hold,alloc: shadowed,ci,ca){{$}}
  //        PRT-OA-C0-NEXT:  {{^ *}}// #pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{copy\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) copy\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) copy\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //        PRT-OA-C1-NEXT:  {{^ *}}// #pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{pcopy\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) pcopy\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) pcopy\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //        PRT-OA-C2-NEXT:  {{^ *}}// #pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{present_or_copy\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) present_or_copy\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) present_or_copy\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //       PRT-OA-Ci0-NEXT:  {{^ *}}// #pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{copyin\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) copyin\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) copyin\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //       PRT-OA-Ci1-NEXT:  {{^ *}}// #pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{pcopyin\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) pcopyin\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) pcopyin\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //       PRT-OA-Ci2-NEXT:  {{^ *}}// #pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{present_or_copyin\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) present_or_copyin\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) present_or_copyin\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //       PRT-OA-Co0-NEXT:  {{^ *}}// #pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{copyout\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) copyout\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) copyout\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //       PRT-OA-Co1-NEXT:  {{^ *}}// #pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{pcopyout\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) pcopyout\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) pcopyout\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //       PRT-OA-Co2-NEXT:  {{^ *}}// #pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{present_or_copyout\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) present_or_copyout\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) present_or_copyout\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //       PRT-OA-Cr0-NEXT:  {{^ *}}// #pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{create\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) create\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) create\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //       PRT-OA-Cr1-NEXT:  {{^ *}}// #pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{pcreate\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) pcreate\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) pcreate\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //       PRT-OA-Cr2-NEXT:  {{^ *}}// #pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{present_or_create\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) present_or_create\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) present_or_create\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //
  //               PRT-A-F:  {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{firstprivate\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) firstprivate\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) firstprivate\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //         PRT-AO-F-NEXT:  {{^ *}}// #pragma omp target teams num_teams(2) firstprivate(gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) firstprivate(li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) firstprivate(shadowed,ci,ca){{$}}
  //               PRT-O-F:  {{^ *}}#pragma omp target teams num_teams(2) firstprivate(gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) firstprivate(li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) firstprivate(shadowed,ci,ca){{$}}
  //         PRT-OA-F-NEXT:  {{^ *}}// #pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{firstprivate\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) firstprivate\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) firstprivate\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //
  //               PRT-A-P:  {{^ *}}#pragma acc parallel{{ LOOP | loop seq | }}num_gangs(2) {{private\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) private\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) private\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //     PRT-AO-PAR-P-NEXT:  {{^ *}}// #pragma omp target teams num_teams(2) private(gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) private(li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) private(shadowed,ci,ca){{$}}
  //  PRT-AO-PARLOOP-P-NOT:  #pragma
  //      PRT-AO-PARLOOP-P:  {{^ *}}// #pragma omp target teams num_teams(2){{$}}
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
                                       // The test omits const when testing
                                       // private.
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  int ci;
  // PRT-AO-PARLOOP-P-NEXT:  //{{ *}}  int ca[3];
  //           PRT-O-PAR-P:  {{^ *}}#pragma omp target teams num_teams(2) private(gi,gt{{(_hi,gt_lw)?}},gp,ga,gs,gu,gUnref) private(li,lt{{(_hi,lt_lw)?}},lp,la,ls,lu,lUnref) private(shadowed,ci,ca){{$}}
  //     PRT-OA-PAR-P-NEXT:  {{^ *}}// #pragma acc parallel{{ LOOP | }}num_gangs(2) {{private\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) private\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) private\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //       PRT-O-PARLOOP-P:  {{^ *}}#pragma omp target teams num_teams(2){{$}}
  //  PRT-O-PARLOOP-P-NEXT:  {
  //  PRT-O-PARLOOP-P-NEXT:    int gi;
  //  PRT-O-PARLOOP-P-NEXT:    {{__uint128_t gt;|uint64_t gt_hi;$[[:space:]]*  uint64_t gt_lw;}}
  //  PRT-O-PARLOOP-P-NEXT:    int *gp;
  //  PRT-O-PARLOOP-P-NEXT:    int ga[2];
  //  PRT-O-PARLOOP-P-NEXT:    struct S gs;
  //  PRT-O-PARLOOP-P-NEXT:    union U gu;
  //  PRT-O-PARLOOP-P-NEXT:    int gUnref;
  //  PRT-O-PARLOOP-P-NEXT:    int li;
  //  PRT-O-PARLOOP-P-NEXT:    {{__uint128_t lt;|uint64_t lt_hi;$[[:space:]]*  uint64_t lt_lw;}}
  //  PRT-O-PARLOOP-P-NEXT:    int *lp;
  //  PRT-O-PARLOOP-P-NEXT:    int la[2];
  //  PRT-O-PARLOOP-P-NEXT:    struct S ls;
  //  PRT-O-PARLOOP-P-NEXT:    union U lu;
  //  PRT-O-PARLOOP-P-NEXT:    int lUnref;
  //  PRT-O-PARLOOP-P-NEXT:    int shadowed;
                               // The test omits const when testing private.
  //  PRT-O-PARLOOP-P-NEXT:    int ci;
  //  PRT-O-PARLOOP-P-NEXT:    int ca[3];
  //  PRT-OA-PARLOOP-P-NOT:  #pragma
  //      PRT-OA-PARLOOP-P:  {{^ *}}// #pragma acc parallel {{LOOP|loop seq}} num_gangs(2) {{private\(gi,gt(_hi,gt_lw)?,gp,ga,gs,gu,gUnref\) private\(li,lt(_hi,lt_lw)?,lp,la,ls,lu,lUnref\) private\(shadowed,ci,ca\)$|(.*\\$[[:space:]])+.*$}}
  //
  //               PRT-NOT:  #pragma
  #pragma acc parallel LOOP num_gangs(2)                         \
    CLAUSE(gi,LIST_UINT128(gt),gp, ga,gs , gu , gUnref )         \
    CLAUSE(  li,LIST_UINT128(lt),lp  ,  la  ,ls,  lu , lUnref  ) \
    CLAUSE(shadowed, ci, ca)
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
    //      PRT: {{TGT_PRINTF|printf}}("inside :{{([^;]|[[:space:]])*}};
    // PRT-NEXT: {{TGT_PRINTF|printf}}("{{([^;]|[[:space:]])*}};
    //
    // It cannot all be in one printf, or nvptx64 output becomes corrupt, so
    // break it into multiple printfs.  The order of output from different
    // gangs is non-deterministic at the level of printfs, but it's easier to
    // write the FileCheck DAG directives at the level of lines.
    //
    //      EXE-IF-TGT-USE-STDIO-DAG: inside : gi:55->56,
    //     EXE-CCi-TGT-USE-STDIO-DAG: inside : gi:{{55|56}}->56,
    //   EXE-CoCrP-TGT-USE-STDIO-DAG: inside : gi:{{.+}}->56,
    //      EXE-IF-TGT-USE-STDIO-DAG:          gt:[1400,59]->[f08234,a07de1],
    //     EXE-CCi-TGT-USE-STDIO-DAG:          gt:[{{1400|f08234}},{{59|a07de1}}]->[f08234,a07de1],
    //   EXE-CoCrP-TGT-USE-STDIO-DAG:          gt:[{{.+}}]->[f08234,a07de1],
    //      EXE-IF-TGT-USE-STDIO-DAG:          *gp:{{55->98|9999->9999}},
    //     EXE-CCi-TGT-USE-STDIO-DAG:          *gp:{{(56|98)->98|9999->9999}},
    //   EXE-CoCrP-TGT-USE-STDIO-DAG:          *gp:{{.+->98|9999->9999}},
    //    EXE-ICCi-TGT-USE-STDIO-DAG:          ga:[{{100|101}},{{200|202}}]->[101,202],
    //       EXE-F-TGT-USE-STDIO-DAG:          ga:[100,200]->[101,202],
    //   EXE-CoCrP-TGT-USE-STDIO-DAG:          ga:[{{.+,.+}}]->[101,202],
    //    EXE-ICCi-TGT-USE-STDIO-DAG:          gs:[{{33|42}},{{11|1}}]->[42,1],
    //       EXE-F-TGT-USE-STDIO-DAG:          gs:[33,11]->[42,1],
    //   EXE-CoCrP-TGT-USE-STDIO-DAG:          gs:[{{.+,.+}}]->[42,1],
    //    EXE-ICCi-TGT-USE-STDIO-DAG:          gu.i:{{22|13}}->13,
    //       EXE-F-TGT-USE-STDIO-DAG:          gu.i:22->13,
    //   EXE-CoCrP-TGT-USE-STDIO-DAG:          gu.i:{{.+}}->13,
    //      EXE-IF-TGT-USE-STDIO-DAG:          li:99->98,
    //     EXE-CCi-TGT-USE-STDIO-DAG:          li:{{99|98}}->98,
    //   EXE-CoCrP-TGT-USE-STDIO-DAG:          li:{{.+}}->98,
    //      EXE-IF-TGT-USE-STDIO-DAG:          lt:[7a1,62b0]->[79ca,2961],
    //     EXE-CCi-TGT-USE-STDIO-DAG:          lt:[{{7a1|79ca}},{{62b0|2961}}]->[79ca,2961],
    //   EXE-CoCrP-TGT-USE-STDIO-DAG:          lt:[{{.+}}]->[79ca,2961],
    //      EXE-IF-TGT-USE-STDIO-DAG:          *lp:{{55->98|9999->9999}},
    //     EXE-CCi-TGT-USE-STDIO-DAG:          *lp:{{(56|98)->98|9999->9999}},
    //   EXE-CoCrP-TGT-USE-STDIO-DAG:          *lp:{{.+->98|9999->9999}},
    //    EXE-ICCi-TGT-USE-STDIO-DAG:          la:[{{77|55}},{{88|66}}]->[55,66],
    //       EXE-F-TGT-USE-STDIO-DAG:          la:[77,88]->[55,66],
    //   EXE-CoCrP-TGT-USE-STDIO-DAG:          la:[{{.+,.+}}]->[55,66],
    //    EXE-ICCi-TGT-USE-STDIO-DAG:          ls:[{{222|333}},{{111|444}}]->[333,444],
    //       EXE-F-TGT-USE-STDIO-DAG:          ls:[222,111]->[333,444],
    //   EXE-CoCrP-TGT-USE-STDIO-DAG:          ls:[{{.+,.+}}]->[333,444],
    //    EXE-ICCi-TGT-USE-STDIO-DAG:          lu.i:{{167|279}}->279,
    //       EXE-F-TGT-USE-STDIO-DAG:          lu.i:167->279,
    //   EXE-CoCrP-TGT-USE-STDIO-DAG:          lu.i:{{.+}}->279,
    //         EXE-TGT-USE-STDIO-DAG:          shadowed:
    //   EXE-ICCiF-TGT-USE-STDIO-DAG:          ci:53,
    //   EXE-CoCrP-TGT-USE-STDIO-DAG:          ci:
    //   EXE-ICCiF-TGT-USE-STDIO-DAG:          ca:[10,11,12]
    //   EXE-CoCrP-TGT-USE-STDIO-DAG:          ca:[{{.+}},{{.+}},{{.+}}]
    //
    // Duplicate that EXE block exactly.
    //
    //      EXE-IF-TGT-USE-STDIO-DAG: inside : gi:55->56,
    //     EXE-CCi-TGT-USE-STDIO-DAG: inside : gi:{{55|56}}->56,
    //   EXE-CoCrP-TGT-USE-STDIO-DAG: inside : gi:{{.+}}->56,
    //      EXE-IF-TGT-USE-STDIO-DAG:          gt:[1400,59]->[f08234,a07de1],
    //     EXE-CCi-TGT-USE-STDIO-DAG:          gt:[{{1400|f08234}},{{59|a07de1}}]->[f08234,a07de1],
    //   EXE-CoCrP-TGT-USE-STDIO-DAG:          gt:[{{.+}}]->[f08234,a07de1],
    //      EXE-IF-TGT-USE-STDIO-DAG:          *gp:{{55->98|9999->9999}},
    //     EXE-CCi-TGT-USE-STDIO-DAG:          *gp:{{(56|98)->98|9999->9999}},
    //   EXE-CoCrP-TGT-USE-STDIO-DAG:          *gp:{{.+->98|9999->9999}},
    //    EXE-ICCi-TGT-USE-STDIO-DAG:          ga:[{{100|101}},{{200|202}}]->[101,202],
    //       EXE-F-TGT-USE-STDIO-DAG:          ga:[100,200]->[101,202],
    //   EXE-CoCrP-TGT-USE-STDIO-DAG:          ga:[{{.+,.+}}]->[101,202],
    //    EXE-ICCi-TGT-USE-STDIO-DAG:          gs:[{{33|42}},{{11|1}}]->[42,1],
    //       EXE-F-TGT-USE-STDIO-DAG:          gs:[33,11]->[42,1],
    //   EXE-CoCrP-TGT-USE-STDIO-DAG:          gs:[{{.+,.+}}]->[42,1],
    //    EXE-ICCi-TGT-USE-STDIO-DAG:          gu.i:{{22|13}}->13,
    //       EXE-F-TGT-USE-STDIO-DAG:          gu.i:22->13,
    //   EXE-CoCrP-TGT-USE-STDIO-DAG:          gu.i:{{.+}}->13,
    //      EXE-IF-TGT-USE-STDIO-DAG:          li:99->98,
    //     EXE-CCi-TGT-USE-STDIO-DAG:          li:{{99|98}}->98,
    //   EXE-CoCrP-TGT-USE-STDIO-DAG:          li:{{.+}}->98,
    //      EXE-IF-TGT-USE-STDIO-DAG:          lt:[7a1,62b0]->[79ca,2961],
    //     EXE-CCi-TGT-USE-STDIO-DAG:          lt:[{{7a1|79ca}},{{62b0|2961}}]->[79ca,2961],
    //   EXE-CoCrP-TGT-USE-STDIO-DAG:          lt:[{{.+}}]->[79ca,2961],
    //      EXE-IF-TGT-USE-STDIO-DAG:          *lp:{{55->98|9999->9999}},
    //     EXE-CCi-TGT-USE-STDIO-DAG:          *lp:{{(56|98)->98|9999->9999}},
    //   EXE-CoCrP-TGT-USE-STDIO-DAG:          *lp:{{.+->98|9999->9999}},
    //    EXE-ICCi-TGT-USE-STDIO-DAG:          la:[{{77|55}},{{88|66}}]->[55,66],
    //       EXE-F-TGT-USE-STDIO-DAG:          la:[77,88]->[55,66],
    //   EXE-CoCrP-TGT-USE-STDIO-DAG:          la:[{{.+,.+}}]->[55,66],
    //    EXE-ICCi-TGT-USE-STDIO-DAG:          ls:[{{222|333}},{{111|444}}]->[333,444],
    //       EXE-F-TGT-USE-STDIO-DAG:          ls:[222,111]->[333,444],
    //   EXE-CoCrP-TGT-USE-STDIO-DAG:          ls:[{{.+,.+}}]->[333,444],
    //    EXE-ICCi-TGT-USE-STDIO-DAG:          lu.i:{{167|279}}->279,
    //       EXE-F-TGT-USE-STDIO-DAG:          lu.i:167->279,
    //   EXE-CoCrP-TGT-USE-STDIO-DAG:          lu.i:{{.+}}->279,
    //         EXE-TGT-USE-STDIO-DAG:          shadowed:
    //   EXE-ICCiF-TGT-USE-STDIO-DAG:          ci:53,
    //   EXE-CoCrP-TGT-USE-STDIO-DAG:          ci:
    //   EXE-ICCiF-TGT-USE-STDIO-DAG:          ca:[10,11,12]
    //   EXE-CoCrP-TGT-USE-STDIO-DAG:          ca:[{{.+}},{{.+}},{{.+}}]
    //
    // If we're not checking for this above, then we shouldn't be printing it.
    // EXE-NOT: inside
    TGT_PRINTF("inside : gi:%d->%d,\n"
               "         gt:[%lx,%lx]->[%lx,%lx],\n"
               "         *gp:%d->%d,\n"
               "         ga:[%d,%d]->[%d,%d],\n"
               "         gs:[%d,%d]->[%d,%d],\n"
               "         gu.i:%d->%d,\n"
               "         li:%d->%d,\n"
               "         lt:[%lx,%lx]->[%lx,%lx],\n",
               giOld, gi,
               HI_UINT128(gtOld), LW_UINT128(gtOld),
               HI_UINT128(gt), LW_UINT128(gt),
               DEREF_PTR(gpOld), DEREF_PTR(gp),
               gaOld[0], gaOld[1], ga[0], ga[1],
               gsOld.i, gsOld.j, gs.i, gs.j,
               guOld.i, gu.i,
               liOld, li,
               HI_UINT128(ltOld), LW_UINT128(ltOld),
               HI_UINT128(lt), LW_UINT128(lt));
    TGT_PRINTF("         *lp:%d->%d,\n"
               "         la:[%d,%d]->[%d,%d],\n"
               "         ls:[%d,%d]->[%d,%d],\n"
               "         lu.i:%d->%d,\n"
               "         shadowed:%d,\n"
               "         ci:%d,\n"
               "         ca:[%d,%d,%d]\n",
               DEREF_PTR(lpOld), DEREF_PTR(lp),
               laOld[0], laOld[1], la[0], la[1],
               lsOld.i, lsOld.j, ls.i, ls.j,
               luOld.i, lu.i,
               shadowed,
               ci, ca[0],
               ca[1], ca[2]);
  } // PRT-NEXT: }

  // DMP: CallExpr
  //
  //       EXE-ICiCrFP: outside: gi:55->55,
  //           EXE-CCo: outside: gi:55->56,
  //  EXE-ICiCrFP-NEXT:          gt:[1400,59]->[1400,59],
  //      EXE-CCo-NEXT:          gt:[1400,59]->[f08234,a07de1],
  //  EXE-ICiCrFP-NEXT:          *gp:55->55,
  //      EXE-CCo-NEXT:          *gp:56->{{98|9999}},
  //     EXE-ICCo-NEXT:          ga:[100,200]->[101,202],
  //   EXE-CiCrFP-NEXT:          ga:[100,200]->[100,200],
  //     EXE-ICCo-NEXT:          gs:[33,11]->[42,1],
  //   EXE-CiCrFP-NEXT:          gs:[33,11]->[33,11],
  //     EXE-ICCo-NEXT:          gu.i:22->13,
  //   EXE-CiCrFP-NEXT:          gu.i:22->22,
  // EXE-ICCiCrFP-NEXT:          gUnref:2->2,
  //       EXE-Co-NEXT:          gUnref:2->
  //  EXE-ICiCrFP-NEXT:          li:99->99,
  //      EXE-CCo-NEXT:          li:99->98,
  //  EXE-ICiCrFP-NEXT:          lt:[7a1,62b0]->[7a1,62b0],
  //      EXE-CCo-NEXT:          lt:[7a1,62b0]->[79ca,2961]
  //  EXE-ICiCrFP-NEXT:          *lp:55->55,
  //      EXE-CCo-NEXT:          *lp:56->{{98|9999}},
  //     EXE-ICCo-NEXT:          la:[77,88]->[55,66],
  //   EXE-CiCrFP-NEXT:          la:[77,88]->[77,88],
  //     EXE-ICCo-NEXT:          ls:[222,111]->[333,444],
  //   EXE-CiCrFP-NEXT:          ls:[222,111]->[222,111],
  //     EXE-ICCo-NEXT:          lu.i:167->279,
  //   EXE-CiCrFP-NEXT:          lu.i:167->167,
  // EXE-ICCiCrFP-NEXT:          lUnref:9->9,
  //       EXE-Co-NEXT:          lUnref:9->
  // EXE-ICCiCrFP-NEXT:          shadowed:111,
  //       EXE-Co-NEXT:          shadowed:
  // EXE-ICCiCrFP-NEXT:          ci:53,
  //       EXE-Co-NEXT:          ci:
  // EXE-ICCiCrFP-NEXT:          ca:[10,11,12]
  //       EXE-Co-NEXT:          ca:[{{.*}},{{.*}},{{.*}}]
  printf("outside: gi:%d->%d,\n"
         "         gt:[%lx,%lx]->[%lx,%lx],\n"
         "         *gp:%d->%d,\n"
         "         ga:[%d,%d]->[%d,%d],\n"
         "         gs:[%d,%d]->[%d,%d],\n"
         "         gu.i:%d->%d,\n"
         "         gUnref:%d->%d,\n"
         "         li:%d->%d,\n"
         "         lt:[%lx,%lx]->[%lx,%lx],\n"
         "         *lp:%d->%d,\n"
         "         la:[%d,%d]->[%d,%d],\n"
         "         ls:[%d,%d]->[%d,%d],\n"
         "         lu.i:%d->%d,\n"
         "         lUnref:%d->%d,\n"
         "         shadowed:%d,\n"
         "         ci:%d,\n"
         "         ca:[%d,%d,%d]\n",
         giOld, gi,
         HI_UINT128(gtOld), LW_UINT128(gtOld),
         HI_UINT128(gt), LW_UINT128(gt),
         *gpOld,
#if MODE != MODE_C0  && MODE != MODE_C1  && MODE != MODE_C2 && \
    MODE != MODE_Co0 && MODE != MODE_Co1 && MODE != MODE_Co2
         *gp,
#else
         DEREF_PTR(gp),
#endif
         gaOld[0], gaOld[1], ga[0], ga[1],
         gsOld.i, gsOld.j, gs.i, gs.j,
         guOld.i, gu.i,
         gUnrefOld, gUnref,
         liOld, li,
         HI_UINT128(ltOld), LW_UINT128(ltOld),
         HI_UINT128(lt), LW_UINT128(lt),
         *lpOld,
#if MODE != MODE_C0  && MODE != MODE_C1  && MODE != MODE_C2 && \
    MODE != MODE_Co0 && MODE != MODE_Co1 && MODE != MODE_Co2
         *lp,
#else
         DEREF_PTR(lp),
#endif
         laOld[0], laOld[1], la[0], la[1],
         lsOld.i, lsOld.j, ls.i, ls.j,
         luOld.i, lu.i,
         lUnrefOld, lUnref,
         shadowed,
         ci, ca[0],
         ca[1], ca[2]);

  return 0;
}
// EXE-NOT:{{.}}

