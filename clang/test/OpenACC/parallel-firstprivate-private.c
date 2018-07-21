// Check ASTDumper.
//
// RUN: %data dmps {
// RUN:   (mode=MODE_I pre=DMP-I,DMP       )
// RUN:   (mode=MODE_F pre=DMP-F,DMP-FP,DMP)
// RUN:   (mode=MODE_P pre=DMP-P,DMP-FP,DMP)
// RUN: }
// RUN: %for dmps {
// RUN:   %clang -Xclang -verify -Xclang -ast-dump -fsyntax-only -fopenacc \
// RUN:          -DMODE=%[mode] %s \
// RUN:   | FileCheck -check-prefixes=%[pre] %s
// RUN: }

// Check ASTWriterStmt, ASTReaderStmt, StmtPrinter, and
// ACCParallelDirective::CreateEmpty (used by ASTReaderStmt).
//
// RUN: %data asts {
// RUN:   (mode=MODE_I pre=PRT-I-A,PRT)
// RUN:   (mode=MODE_F pre=PRT-F-A,PRT)
// RUN:   (mode=MODE_P pre=PRT-P-A,PRT)
// RUN: }
// RUN: %for asts {
// RUN:   %clang -Xclang -verify -fopenacc -emit-ast -o %t.ast \
// RUN:          -DMODE=%[mode] %s
// RUN:   %clang_cc1 -ast-print %t.ast 2>&1 \
// RUN:   | FileCheck -check-prefixes=%[pre] %s
// RUN: }

// Check -fopenacc-print.  Default is checked above.
//
// RUN: %data prints {
// RUN:   (mode=MODE_I print=acc     pre=PRT-I-A,PRT         )
// RUN:   (mode=MODE_I print=omp     pre=PRT-I-O,PRT         )
// RUN:   (mode=MODE_I print=acc-omp pre=PRT-I-A,PRT-I-AO,PRT)
// RUN:   (mode=MODE_I print=omp-acc pre=PRT-I-O,PRT-I-OA,PRT)
//
// RUN:   (mode=MODE_F print=acc     pre=PRT-F-A,PRT         )
// RUN:   (mode=MODE_F print=omp     pre=PRT-F-O,PRT         )
// RUN:   (mode=MODE_F print=acc-omp pre=PRT-F-A,PRT-F-AO,PRT)
// RUN:   (mode=MODE_F print=omp-acc pre=PRT-F-O,PRT-F-OA,PRT)
//
// RUN:   (mode=MODE_P print=acc     pre=PRT-P-A,PRT         )
// RUN:   (mode=MODE_P print=omp     pre=PRT-P-O,PRT         )
// RUN:   (mode=MODE_P print=acc-omp pre=PRT-P-A,PRT-P-AO,PRT)
// RUN:   (mode=MODE_P print=omp-acc pre=PRT-P-O,PRT-P-OA,PRT)
// RUN: }
// RUN: %for prints {
// RUN:   %clang -Xclang -verify -fopenacc-print=%[print] -DMODE=%[mode] %s \
// RUN:   | FileCheck -check-prefixes=%[pre] %s
// RUN: }

// Can we -ast-print the OpenMP source code, compile, and run it successfully?
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
// RUN: %for exes {
// RUN:   %clang -Xclang -verify -fopenacc-print=omp -DMODE=%[mode] \
// RUN:          %[cflags] %s > %t-omp.c
// RUN:   echo "// expected""-no-diagnostics" >> %t-omp.c
// RUN:   %clang -Xclang -verify -fopenmp -o %t %t-omp.c
// RUN:   %t | FileCheck -check-prefixes=%[pre] %s
// RUN: }
//
// Check execution with normal compilation.
//
// RUN: %for exes {
// RUN:   %clang -Xclang -verify -fopenacc -o %t -DMODE=%[mode] %[cflags] %s
// RUN:   %t | FileCheck -check-prefixes=%[pre] %s
// RUN: }

// Check that variables aren't passed to the outlined function in the case of
// the private clause.
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
// RUN: %data constErrs {
// RUN:   (mode=MODE_I cflags=-DGCONST=const verify=gconst)
// RUN:   (mode=MODE_I cflags=-DLCONST=const verify=lconst)
// RUN:   (mode=MODE_F cflags=-DGCONST=const verify=gconst)
// RUN:   (mode=MODE_F cflags=-DLCONST=const verify=lconst)
// RUN:   (mode=MODE_P cflags=-DGCONST=const verify=gconst,gconst-priv)
// RUN:   (mode=MODE_P cflags=-DLCONST=const verify=lconst,lconst-priv)
// RUN: }
// RUN: %for constErrs {
// RUN:   %clang -fopenacc -fsyntax-only -Xclang -verify=%[verify] \
// RUN:          %[cflags] -DMODE=%[mode] %s
// RUN: }

// Check err_acc_wrong_dsa.  Whether firstprivate or private is first affects
// the code path, so try both.
//
// RUN: %data dsaConflicts {
// RUN:   (mode=MODE_FP verify=fp)
// RUN:   (mode=MODE_PF verify=pf)
// RUN: }
// RUN: %for dsaConflicts {
// RUN:   %clang -fopenacc -fsyntax-only -Xclang -verify=%[verify] \
// RUN:          -DMODE=%[mode] %s
// RUN: }

// END.

// expected-no-diagnostics

#include <stdio.h>
#include <stdint.h>

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

#pragma acc parallel num_gangs(2)                              \
    CLAUSE(gi,IF_UINT128(gt,)gp, ga,gs , gu , gUnref )         \
    CLAUSE(  li,IF_UINT128(lt,)lp  ,  la  ,ls,  lu , lUnref  ) \
    CLAUSE(shadowed)
// fp-error@-3 6-7 {{firstprivate variable cannot be private}}
// fp-note@-4  6-7 {{defined as firstprivate}}
// fp-error@-4 6-7 {{firstprivate variable cannot be private}}
// fp-note@-5  6-7 {{defined as firstprivate}}
// fp-error@-5 1   {{firstprivate variable cannot be private}}
// fp-note@-6  1   {{defined as firstprivate}}
//
// pf-error@-10 6-7 {{private variable cannot be firstprivate}}
// pf-note@-11  6-7 {{defined as private}}
// pf-error@-11 6-7 {{private variable cannot be firstprivate}}
// pf-note@-12  6-7 {{defined as private}}
// pf-error@-12 1   {{private variable cannot be firstprivate}}
// pf-note@-13  1   {{defined as private}}
//
// gconst-priv-error@-17 {{const variable cannot be private because initialization is impossible}}
// lconst-priv-error@-17 {{const variable cannot be private because initialization is impossible}}

// PRT-NOT:      #pragma
//
// PRT-I-A:       {{^ *}}#pragma acc parallel num_gangs(2){{$}}
// PRT-I-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) shared(ga,gs,gu,la,ls,lu) firstprivate(gi,{{(gt,)?}}gp,li,{{(lt,)?}}lp,shadowed){{$}}
// PRT-I-O:       {{^ *}}#pragma omp target teams num_teams(2) shared(ga,gs,gu,la,ls,lu) firstprivate(gi,{{(gt,)?}}gp,li,{{(lt,)?}}lp,shadowed){{$}}
// PRT-I-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2){{$}}
//
// PRT-F-A:       {{^ *}}#pragma acc parallel num_gangs(2) firstprivate(gi,{{(gt,)?}}gp,ga,gs,gu,gUnref) firstprivate(li,{{(lt,)?}}lp,la,ls,lu,lUnref) firstprivate(shadowed){{$}}
// PRT-F-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) firstprivate(gi,{{(gt,)?}}gp,ga,gs,gu,gUnref) firstprivate(li,{{(lt,)?}}lp,la,ls,lu,lUnref) firstprivate(shadowed){{$}}
// PRT-F-O:       {{^ *}}#pragma omp target teams num_teams(2) firstprivate(gi,{{(gt,)?}}gp,ga,gs,gu,gUnref) firstprivate(li,{{(lt,)?}}lp,la,ls,lu,lUnref) firstprivate(shadowed){{$}}
// PRT-F-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) firstprivate(gi,{{(gt,)?}}gp,ga,gs,gu,gUnref) firstprivate(li,{{(lt,)?}}lp,la,ls,lu,lUnref) firstprivate(shadowed){{$}}
//
// PRT-P-A:       {{^ *}}#pragma acc parallel num_gangs(2) private(gi,{{(gt,)?}}gp,ga,gs,gu,gUnref) private(li,{{(lt,)?}}lp,la,ls,lu,lUnref) private(shadowed){{$}}
// PRT-P-AO-NEXT: {{^ *}}// #pragma omp target teams num_teams(2) private(gi,{{(gt,)?}}gp,ga,gs,gu,gUnref) private(li,{{(lt,)?}}lp,la,ls,lu,lUnref) private(shadowed){{$}}
// PRT-P-O:       {{^ *}}#pragma omp target teams num_teams(2) private(gi,{{(gt,)?}}gp,ga,gs,gu,gUnref) private(li,{{(lt,)?}}lp,la,ls,lu,lUnref) private(shadowed){{$}}
// PRT-P-OA-NEXT: {{^ *}}// #pragma acc parallel num_gangs(2) private(gi,{{(gt,)?}}gp,ga,gs,gu,gUnref) private(li,{{(lt,)?}}lp,la,ls,lu,lUnref) private(shadowed){{$}}
//
// PRT-NOT:      #pragma
//
// DMP:         ACCParallelDirective
// DMP-NEXT:    ACCNum_gangsClause
// DMP-NEXT:    IntegerLiteral {{.*}} 'int' 2
//
// DMP-F-NEXT:  ACCFirstprivateClause
// DMP-P-NEXT:  ACCPrivateClause
// DMP-FP-NOT:  <implicit>
// DMP-FP-SAME: {{$}}
// DMP-FP-NEXT: DeclRefExpr {{.*}} 'gi' 'int'
// DeclRefExpr for gt is here if defined
// DMP-FP-NOT:  ACC
// DMP-FP:      DeclRefExpr {{.*}} 'gp' 'const int *'
// DMP-FP-NEXT: DeclRefExpr {{.*}} 'ga' 'int [2]'
// DMP-FP-NEXT: DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
// DMP-FP-NEXT: DeclRefExpr {{.*}} 'gu' 'union U':'union U'
// DMP-FP-NEXT: DeclRefExpr {{.*}} 'gUnref' 'int'
//
// DMP-F-NEXT:  ACCFirstprivateClause
// DMP-P-NEXT:  ACCPrivateClause
// DMP-FP-NOT:  <implicit>
// DMP-FP-SAME: {{$}}
// DMP-FP-NEXT: DeclRefExpr {{.*}} 'li' 'int'
// DMP-FP-NOT:  ACC
// DeclRefExpr for lt is here if defined
// DMP-FP:      DeclRefExpr {{.*}} 'lp' 'const int *'
// DMP-FP-NEXT: DeclRefExpr {{.*}} 'la' 'int [2]'
// DMP-FP-NEXT: DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
// DMP-FP-NEXT: DeclRefExpr {{.*}} 'lu' 'union U':'union U'
// DMP-FP-NEXT: DeclRefExpr {{.*}} 'lUnref' 'int'
//
// DMP-F-NEXT:  ACCFirstprivateClause
// DMP-P-NEXT:  ACCPrivateClause
// DMP-FP-NOT:  <implicit>
// DMP-FP-SAME: {{$}}
// DMP-FP-NEXT: DeclRefExpr {{.*}} 'shadowed' 'int'
//
// DMP-I-NEXT:  ACCSharedClause
// DMP-I-SAME:  <implicit>
// DMP-I-SAME:  {{$}}
// DMP-I-NEXT:  DeclRefExpr {{.*}} 'ga' 'int [2]'
// DMP-I-NEXT:  DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
// DMP-I-NEXT:  DeclRefExpr {{.*}} 'gu' 'union U':'union U'
// DMP-I-NEXT:  DeclRefExpr {{.*}} 'la' 'int [2]'
// DMP-I-NEXT:  DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
// DMP-I-NEXT:  DeclRefExpr {{.*}} 'lu' 'union U':'union U'
//
// DMP-I-NEXT:  ACCFirstprivateClause
// DMP-I-SAME:  <implicit>
// DMP-I-SAME:  {{$}}
// DMP-I-NEXT:  DeclRefExpr {{.*}} 'gi' 'int'
// DMP-I-NOT:   ACC
// DMP-I:       DeclRefExpr {{.*}} 'gp' 'const int *'
// DMP-I-NEXT:  DeclRefExpr {{.*}} 'li' 'int'
// DMP-I-NOT:   ACC
// DMP-I:       DeclRefExpr {{.*}} 'lp' 'const int *'
// DMP-I-NEXT:  DeclRefExpr {{.*}} 'shadowed' 'int'
//
// Here we repeat the ACC directive and all its clauses exactly but for OMP.
//
// DMP-NEXT:    impl: OMPTargetTeamsDirective
// DMP-NEXT:    OMPNum_teamsClause
// DMP-NEXT:    IntegerLiteral {{.*}} 'int' 2
//
// DMP-F-NEXT:  OMPFirstprivateClause
// DMP-P-NEXT:  OMPPrivateClause
// DMP-FP-NOT:  <implicit>
// DMP-FP-SAME: {{$}}
// DMP-FP-NEXT: DeclRefExpr {{.*}} 'gi' 'int'
// DeclRefExpr for gt is here if defined
// DMP-FP-NOT:  OMP
// DMP-FP:      DeclRefExpr {{.*}} 'gp' 'const int *'
// DMP-FP-NEXT: DeclRefExpr {{.*}} 'ga' 'int [2]'
// DMP-FP-NEXT: DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
// DMP-FP-NEXT: DeclRefExpr {{.*}} 'gu' 'union U':'union U'
// DMP-FP-NEXT: DeclRefExpr {{.*}} 'gUnref' 'int'
//
// DMP-F-NEXT:  OMPFirstprivateClause
// DMP-P-NEXT:  OMPPrivateClause
// DMP-FP-NOT:  <implicit>
// DMP-FP-SAME: {{$}}
// DMP-FP-NEXT: DeclRefExpr {{.*}} 'li' 'int'
// DMP-FP-NOT:  OMP
// DeclRefExpr for lt is here if defined
// DMP-FP:      DeclRefExpr {{.*}} 'lp' 'const int *'
// DMP-FP-NEXT: DeclRefExpr {{.*}} 'la' 'int [2]'
// DMP-FP-NEXT: DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
// DMP-FP-NEXT: DeclRefExpr {{.*}} 'lu' 'union U':'union U'
// DMP-FP-NEXT: DeclRefExpr {{.*}} 'lUnref' 'int'
//
// DMP-F-NEXT:  OMPFirstprivateClause
// DMP-P-NEXT:  OMPPrivateClause
// DMP-FP-NOT:  <implicit>
// DMP-FP-SAME: {{$}}
// DMP-FP-NEXT: DeclRefExpr {{.*}} 'shadowed' 'int'
//
// DMP-I-NEXT:  OMPSharedClause
// DMP-I-NOT:   <implicit>
// DMP-I-SAME:  {{$}}
// DMP-I-NEXT:  DeclRefExpr {{.*}} 'ga' 'int [2]'
// DMP-I-NEXT:  DeclRefExpr {{.*}} 'gs' 'struct S':'struct S'
// DMP-I-NEXT:  DeclRefExpr {{.*}} 'gu' 'union U':'union U'
// DMP-I-NEXT:  DeclRefExpr {{.*}} 'la' 'int [2]'
// DMP-I-NEXT:  DeclRefExpr {{.*}} 'ls' 'struct S':'struct S'
// DMP-I-NEXT:  DeclRefExpr {{.*}} 'lu' 'union U':'union U'
//
// DMP-I-NEXT:  OMPFirstprivateClause
// DMP-I-NOT:   <implicit>
// DMP-I-SAME:  {{$}}
// DMP-I-NEXT:  DeclRefExpr {{.*}} 'gi' 'int'
// DMP-I-NOT:   OMP
// DMP-I:       DeclRefExpr {{.*}} 'gp' 'const int *'
// DMP-I-NEXT:  DeclRefExpr {{.*}} 'li' 'int'
// DMP-I-NOT:   OMP
// DMP-I:       DeclRefExpr {{.*}} 'lp' 'const int *'
// DMP-I-NEXT:  DeclRefExpr {{.*}} 'shadowed' 'int'
  { // PRT-NEXT: {
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
    // PRT:      printf("inside :
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
  }
  // PRT-NEXT: }
  // DMP:      CallExpr
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

  return 0;
  // EXE-NOT:{{.}}
}

