// Check implicit "acc routine" directives not covered by routine-implicit.c
// because they are specific to C++.
//
// Implicit routine directives for C++ class/namespace member functions are
// checked as part of routine-placement-for-class.cpp and
// routine-placement-for-namespace.cpp.
//
// We check with implicit worker and vector clauses on loops, both enabled or
// both disabled.  fopenacc-implicit-worker-vector.c checks various combinations
// of -f[no-]openacc-implicit-{worker,vector}.  Implicit gang, worker, and
// vector clauses permitted by something other than implicit routine directives
// are checked in loop-implicit-gang-worker-vector.c, and interaction with tile
// clauses is checked there.
//
// FIXME: We skip execution checks for source-to-source mode because AST
// printing for templates includes separate printing of the template
// specializations, which breaks the compilation.
//
// REDEFINE: %{dmp:fc:args} = -strict-whitespace
// REDEFINE: %{exe:fc:args} = -strict-whitespace -match-full-lines \
// REDEFINE:                  -implicit-check-not='{{.}}'
//
// REDEFINE: %{all:fc:pres} = G
// RUN: %{acc-check-dmp-cxx}
// RUN: %{acc-check-prt-cxx}
// RUN: %{acc-check-exe-cxx-no-s2s}
//
// FIXME: For amdgcn, execution checks below fail on ExCL's explorer with the
// following error, so we skip those checks:
//
//   AMDGPU fatal error 1: Memory access fault by GPU 8 (agent 0x55cbef8353f0)
//   at virtual address (nil). Reasons: Page not present or supervisor privilege
//
// So far, we've determined the same error occurs when explicit worker and
// vector clauses replace the implicit ones added here, and we've determined it
// happens when compiling the OpenMP translation directly with Clacc.
//
// REDEFINE: %{all:clang:args} = -fopenacc-implicit-worker \
// REDEFINE:                     -fopenacc-implicit-vector
// REDEFINE: %{all:fc:pres} = GWV
// RUN: %{acc-check-dmp-cxx}
// RUN: %{acc-check-prt-cxx}
// RUN: %if amdgcn-amd-amdhsa %{%} %else %{ %{acc-check-exe-cxx-no-s2s} %}
//
// END.

/* expected-no-diagnostics */

#include <stdio.h>

#define A_SIZE 4
#define NUM_GANGS 4

// Each of these functions has an (unnecessary) routine directive.  The point is
// that should not imply a routine directive for any lambda calling it.
#pragma acc routine seq
static void initArray(int *a) {
  for (int i = 0; i < A_SIZE; ++i)
    a[i] = i;
}
#pragma acc routine seq
static void printArray(int *a) {
  for (int i = 0; i < A_SIZE; ++i)
    printf("a[%d]=%d\n", i, a[i]);
}

template <typename Lambda>
static void callLambdaInParallel(const char *title, int ngangs, Lambda lambda) {
  printf("%s\n", title);
  int a[A_SIZE];
  initArray(a);
  #pragma acc parallel num_gangs(ngangs)
  lambda(a);
  printArray(a);
}

template <typename Lambda>
static void callLambdaFromHost(const char *title, int ngangs, Lambda lambda) {
  printf("%s\n", title);
  lambda(ngangs);
}

#pragma acc routine gang
template <typename Lambda>
static void gangFnCallingLambda(int *a, Lambda lambda) { lambda(a); }

#pragma acc routine worker
template <typename Lambda>
static void workerFnCallingLambda(int *a, Lambda lambda) { lambda(a); }

#pragma acc routine vector
template <typename Lambda>
static void vectorFnCallingLambda(int *a, Lambda lambda) { lambda(a); }

#pragma acc routine seq
template <typename Lambda>
static void seqFnCallingLambda(int *a, Lambda lambda) { lambda(a); }

#define LOOP                       \
  for (int i = 0; i < A_SIZE; ++i) \
    a[i] += 10;

#pragma acc routine gang
static void gangFn(int *a) {
  #pragma acc loop gang
  LOOP
}

#pragma acc routine worker
static void workerFn(int *a) {
  #pragma acc loop worker
  LOOP
}

#pragma acc routine vector
static void vectorFn(int *a) {
  #pragma acc loop vector
  LOOP
}

#pragma acc routine seq
static void seqFn(int *a) {
  #pragma acc loop seq
  LOOP
}

// Its use in one lambda (with an implicit routine directive) should be
// sufficient to imply its routine directive, so don't use it in other
// functions.
//
// FIXME: The routine directive isn't computed properly when serializing and
// deserializing the AST, so we skip the check in that case.  The problem is
// SemaOpenACC.cpp's record of function calls is currently lost during
// serialization, but template instantiation doesn't happen until
// deserialization.
//
// DMP-LABEL: FunctionDecl {{.*}} impSeqFn 'void (int *)'
//   DMP-NOT: FunctionDecl
//   DMP-SRC:   ACCRoutineDeclAttr {{.*}} Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP
static void impSeqFn(int *a) {
  LOOP
}

//------------------------------------------------------------------------------
// Check lambda calling a function with or without a routine directive.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} checkGangFnCallInLambda 'void ()'
//       DMP: LambdaExpr
//  DMP-NEXT:   CXXRecordDecl
//       DMP:     CXXMethodDecl
//  DMP-NEXT:       ParmVarDecl
//  DMP-NEXT:       CompoundStmt
//  DMP-NEXT:         CallExpr
//  DMP-NEXT:           ImplicitCastExpr
//  DMP-NEXT:             DeclRefExpr {{.*}} 'gangFn'
//       DMP:       ACCRoutineDeclAttr {{.*}} Implicit Gang OMPNodeKind=unknown DirectiveDiscardedForOMP
//
// PRT-LABEL: void checkGangFnCallInLambda() {
//  PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//  PRT-NEXT:     gangFn(a);
//  PRT-NEXT:   });
//  PRT-NEXT: }
//
// EXE-LABEL:checkGangFnCallInLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
static void checkGangFnCallInLambda() {
  callLambdaInParallel("checkGangFnCallInLambda", NUM_GANGS, [](int *a) {
    gangFn(a);
  });
}

// DMP-LABEL: FunctionDecl {{.*}} checkWorkerFnCallInLambda 'void ()'
//       DMP: LambdaExpr
//  DMP-NEXT:   CXXRecordDecl
//       DMP:     CXXMethodDecl
//  DMP-NEXT:       ParmVarDecl
//  DMP-NEXT:       CompoundStmt
//  DMP-NEXT:         CallExpr
//  DMP-NEXT:           ImplicitCastExpr
//  DMP-NEXT:             DeclRefExpr {{.*}} 'workerFn'
//       DMP:       ACCRoutineDeclAttr {{.*}} Implicit Worker OMPNodeKind=unknown DirectiveDiscardedForOMP
//
// PRT-LABEL: void checkWorkerFnCallInLambda() {
//  PRT-NEXT:   callLambdaInParallel("{{.*}}", 1, [](int *a) {
//  PRT-NEXT:     workerFn(a);
//  PRT-NEXT:   });
//  PRT-NEXT: }
//
// EXE-LABEL:checkWorkerFnCallInLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
static void checkWorkerFnCallInLambda() {
  callLambdaInParallel("checkWorkerFnCallInLambda", 1, [](int *a) {
    workerFn(a);
  });
}

// DMP-LABEL: FunctionDecl {{.*}} checkVectorFnCallInLambda 'void ()'
//       DMP: LambdaExpr
//  DMP-NEXT:   CXXRecordDecl
//       DMP:     CXXMethodDecl
//  DMP-NEXT:       ParmVarDecl
//  DMP-NEXT:       CompoundStmt
//  DMP-NEXT:         CallExpr
//  DMP-NEXT:           ImplicitCastExpr
//  DMP-NEXT:             DeclRefExpr {{.*}} 'vectorFn'
//       DMP:       ACCRoutineDeclAttr {{.*}} Implicit Vector OMPNodeKind=unknown DirectiveDiscardedForOMP
//
// PRT-LABEL: void checkVectorFnCallInLambda() {
//  PRT-NEXT:   callLambdaInParallel("{{.*}}", 1, [](int *a) {
//  PRT-NEXT:     vectorFn(a);
//  PRT-NEXT:   });
//  PRT-NEXT: }
//
// EXE-LABEL:checkVectorFnCallInLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
static void checkVectorFnCallInLambda() {
  callLambdaInParallel("checkVectorFnCallInLambda", 1, [](int *a) {
    vectorFn(a);
  });
}

// The routine directive for this lambda is implied by the lambda's use instead
// of its body.
//
// DMP-LABEL: FunctionDecl {{.*}} checkSeqFnCallInLambda 'void ()'
//       DMP: LambdaExpr
//  DMP-NEXT:   CXXRecordDecl
//       DMP:     CXXMethodDecl
//  DMP-NEXT:       ParmVarDecl
//  DMP-NEXT:       CompoundStmt
//  DMP-NEXT:         CallExpr
//  DMP-NEXT:           ImplicitCastExpr
//  DMP-NEXT:             DeclRefExpr {{.*}} 'seqFn'
//       DMP:       ACCRoutineDeclAttr {{.*}} Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP
//
// PRT-LABEL: void checkSeqFnCallInLambda() {
//  PRT-NEXT:   callLambdaInParallel("{{.*}}", 1, [](int *a) {
//  PRT-NEXT:     seqFn(a);
//  PRT-NEXT:   });
//  PRT-NEXT: }
//
// EXE-LABEL:checkSeqFnCallInLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
static void checkSeqFnCallInLambda() {
  callLambdaInParallel("checkSeqFnCallInLambda", 1, [](int *a) {
    seqFn(a);
  });
}

// The routine directive for this lambda is implied by the lambda's use instead
// of its body, and that implies a routine directive for impSeqFn.
//
// DMP-LABEL: FunctionDecl {{.*}} checkImpSeqFnCallInLambda 'void ()'
//       DMP: LambdaExpr
//  DMP-NEXT:   CXXRecordDecl
//       DMP:     CXXMethodDecl
//  DMP-NEXT:       ParmVarDecl
//  DMP-NEXT:       CompoundStmt
//  DMP-NEXT:         CallExpr
//  DMP-NEXT:           ImplicitCastExpr
//  DMP-NEXT:             DeclRefExpr {{.*}} 'impSeqFn'
//       DMP:       ACCRoutineDeclAttr {{.*}} Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP
//
// PRT-LABEL: void checkImpSeqFnCallInLambda() {
//  PRT-NEXT:   callLambdaInParallel("{{.*}}", 1, [](int *a) {
//  PRT-NEXT:     impSeqFn(a);
//  PRT-NEXT:   });
//  PRT-NEXT: }
//
// EXE-LABEL:checkImpSeqFnCallInLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
static void checkImpSeqFnCallInLambda() {
  callLambdaInParallel("checkImpSeqFnCallInLambda", 1, [](int *a) {
    impSeqFn(a);
  });
}

// This lambda has seq function calls, which should not imply a routine
// directive for the lambda, which can thus contain a parallel construct and a
// static local.
//
// DMP-LABEL: FunctionDecl {{.*}} checkSeqFnCallInHostLambda 'void ()'
//       DMP: LambdaExpr
//  DMP-NEXT:   CXXRecordDecl
//       DMP:     CXXMethodDecl
//  DMP-NEXT:       ParmVarDecl
//  DMP-NEXT:       CompoundStmt
//   DMP-NOT:       ACCRoutineDeclAttr
//
//   PRT-LABEL: void checkSeqFnCallInHostLambda() {
//    PRT-NEXT:   callLambdaFromHost("{{.*}}", 1, [](int ngangs) {
//    PRT-NEXT:     int a[{{.*}}];
//    PRT-NEXT:     initArray(a);
//  PRT-A-NEXT:     {{^ *}}#pragma acc parallel num_gangs(ngangs){{$}}
// PRT-AO-NEXT:     {{^ *}}// #pragma omp target teams num_teams(ngangs) map(ompx_hold,tofrom: a){{$}}
//  PRT-O-NEXT:     {{^ *}}#pragma omp target teams num_teams(ngangs) map(ompx_hold,tofrom: a){{$}}
// PRT-OA-NEXT:     {{^ *}}// #pragma acc parallel num_gangs(ngangs){{$}}
//    PRT-NEXT:     gangFn(a);
//    PRT-NEXT:     printArray(a);
//    PRT-NEXT:   });
//    PRT-NEXT: }
//
// EXE-LABEL:checkSeqFnCallInHostLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
static void checkSeqFnCallInHostLambda() {
  callLambdaFromHost("checkSeqFnCallInHostLambda", 1, [](int ngangs) {
    static int a[A_SIZE];
    initArray(a);
    #pragma acc parallel num_gangs(ngangs)
    gangFn(a);
    printArray(a);
  });
}

//------------------------------------------------------------------------------
// Check lambda containing an orphaned loop construct or just a plain loop.
//
// Implicit routine directives should be computed based on explicit
// gang/worker/vector clauses on loops, and implicit gang/worker/vector clauses
// on loops should be computed based on the implicit routine directives.
//------------------------------------------------------------------------------

//    DMP-LABEL: FunctionDecl {{.*}} checkGangLoopInLambda 'void ()'
//          DMP:   LambdaExpr
//     DMP-NEXT:     CXXRecordDecl
//          DMP:       CXXMethodDecl
//     DMP-NEXT:         ParmVarDecl
//     DMP-NEXT:         CompoundStmt
//          DMP:           ACCLoopDirective
//     DMP-NEXT:             ACCGangClause
//      DMP-NOT:               <implicit>
//     DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//     DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//     DMP-NEXT:               DeclRefExpr {{.*}} 'a'
// DMP-GWV-NEXT:             ACCWorkerClause {{.*}} <implicit>
// DMP-GWV-NEXT:             ACCVectorClause {{.*}} <implicit>
//   DMP-G-NEXT:             impl: OMPDistributeDirective
// DMP-GWV-NEXT:             impl: OMPDistributeParallelForSimdDirective
//      DMP-NOT:               OMP
//          DMP:               ForStmt
//          DMP:         ACCRoutineDeclAttr {{.*}} Implicit Gang OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//       PRT-LABEL: void checkGangLoopInLambda() {
//        PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//      PRT-A-NEXT:     {{^ *}}#pragma acc loop gang{{$}}
//   PRT-AO-G-NEXT:     {{^ *}}// #pragma omp distribute{{$}}
// PRT-AO-GWV-NEXT:     {{^ *}}// #pragma omp distribute parallel for simd{{$}}
//    PRT-O-G-NEXT:     {{^ *}}#pragma omp distribute{{$}}
//  PRT-O-GWV-NEXT:     {{^ *}}#pragma omp distribute parallel for simd{{$}}
//     PRT-OA-NEXT:     {{^ *}}// #pragma acc loop gang{{$}}
//             PRT:   });
//        PRT-NEXT: }
//
// EXE-LABEL:checkGangLoopInLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
static void checkGangLoopInLambda() {
  callLambdaInParallel("checkGangLoopInLambda", NUM_GANGS, [](int *a) {
    #pragma acc loop gang
    LOOP
  });
}

//    DMP-LABEL: FunctionDecl {{.*}} checkWorkerLoopInLambda 'void ()'
//          DMP:   LambdaExpr
//     DMP-NEXT:     CXXRecordDecl
//          DMP:       CXXMethodDecl
//     DMP-NEXT:         ParmVarDecl
//     DMP-NEXT:         CompoundStmt
//          DMP:           ACCLoopDirective
//     DMP-NEXT:             ACCWorkerClause
//      DMP-NOT:               <implicit>
//     DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//     DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//     DMP-NEXT:               DeclRefExpr {{.*}} 'a'
// DMP-GWV-NEXT:             ACCVectorClause {{.*}} <implicit>
//   DMP-G-NEXT:             impl: OMPParallelForDirective
// DMP-GWV-NEXT:             impl: OMPParallelForSimdDirective
//      DMP-NOT:               OMP
//          DMP:               ForStmt
//          DMP:         ACCRoutineDeclAttr {{.*}} Implicit Worker OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//       PRT-LABEL: void checkWorkerLoopInLambda() {
//        PRT-NEXT:   callLambdaInParallel("{{.*}}", 1, [](int *a) {
//      PRT-A-NEXT:     {{^ *}}#pragma acc loop worker{{$}}
//   PRT-AO-G-NEXT:     {{^ *}}// #pragma omp parallel for{{$}}
// PRT-AO-GWV-NEXT:     {{^ *}}// #pragma omp parallel for simd{{$}}
//    PRT-O-G-NEXT:     {{^ *}}#pragma omp parallel for{{$}}
//  PRT-O-GWV-NEXT:     {{^ *}}#pragma omp parallel for simd{{$}}
//     PRT-OA-NEXT:     {{^ *}}// #pragma acc loop worker{{$}}
//             PRT:   });
//        PRT-NEXT: }
//
// EXE-LABEL:checkWorkerLoopInLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
static void checkWorkerLoopInLambda() {
  callLambdaInParallel("checkWorkerLoopInLambda", 1, [](int *a) {
    #pragma acc loop worker
    LOOP
  });
}

// DMP-LABEL: FunctionDecl {{.*}} checkVectorLoopInLambda 'void ()'
//       DMP:   LambdaExpr
//  DMP-NEXT:     CXXRecordDecl
//       DMP:       CXXMethodDecl
//  DMP-NEXT:         ParmVarDecl
//  DMP-NEXT:         CompoundStmt
//       DMP:           ACCLoopDirective
//  DMP-NEXT:             ACCVectorClause
//   DMP-NOT:               <implicit>
//  DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:               DeclRefExpr {{.*}} 'a'
//  DMP-NEXT:             impl: OMPSimdDirective
//   DMP-NOT:               OMP
//       DMP:               ForStmt
//       DMP:         ACCRoutineDeclAttr {{.*}} Implicit Vector OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//   PRT-LABEL: void checkVectorLoopInLambda() {
//    PRT-NEXT:   callLambdaInParallel("{{.*}}", 1, [](int *a) {
//  PRT-A-NEXT:     {{^ *}}#pragma acc loop vector{{$}}
// PRT-AO-NEXT:     {{^ *}}// #pragma omp simd{{$}}
//  PRT-O-NEXT:     {{^ *}}#pragma omp simd{{$}}
// PRT-OA-NEXT:     {{^ *}}// #pragma acc loop vector{{$}}
//         PRT:   });
//    PRT-NEXT: }
//
// EXE-LABEL:checkVectorLoopInLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
static void checkVectorLoopInLambda() {
  callLambdaInParallel("checkVectorLoopInLambda", 1, [](int *a) {
    #pragma acc loop vector
    LOOP
  });
}

//    DMP-LABEL: FunctionDecl {{.*}} checkGangWorkerLoopInLambda 'void ()'
//          DMP:   LambdaExpr
//     DMP-NEXT:     CXXRecordDecl
//          DMP:       CXXMethodDecl
//     DMP-NEXT:         ParmVarDecl
//     DMP-NEXT:         CompoundStmt
//          DMP:           ACCLoopDirective
//     DMP-NEXT:             ACCGangClause
//      DMP-NOT:               <implicit>
//     DMP-NEXT:             ACCWorkerClause
//      DMP-NOT:               <implicit>
//     DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//     DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//     DMP-NEXT:               DeclRefExpr {{.*}} 'a'
// DMP-GWV-NEXT:             ACCVectorClause {{.*}} <implicit>
//   DMP-G-NEXT:             impl: OMPDistributeParallelForDirective
// DMP-GWV-NEXT:             impl: OMPDistributeParallelForSimdDirective
//      DMP-NOT:               OMP
//          DMP:               ForStmt
//          DMP:         ACCRoutineDeclAttr {{.*}} Implicit Gang OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//       PRT-LABEL: void checkGangWorkerLoopInLambda() {
//        PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//      PRT-A-NEXT:     {{^ *}}#pragma acc loop gang worker{{$}}
//   PRT-AO-G-NEXT:     {{^ *}}// #pragma omp distribute parallel for{{$}}
// PRT-AO-GWV-NEXT:     {{^ *}}// #pragma omp distribute parallel for simd{{$}}
//    PRT-O-G-NEXT:     {{^ *}}#pragma omp distribute parallel for{{$}}
//  PRT-O-GWV-NEXT:     {{^ *}}#pragma omp distribute parallel for simd{{$}}
//     PRT-OA-NEXT:     {{^ *}}// #pragma acc loop gang worker{{$}}
//             PRT:   });
//        PRT-NEXT: }
//
// EXE-LABEL:checkGangWorkerLoopInLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
static void checkGangWorkerLoopInLambda() {
  callLambdaInParallel("checkGangWorkerLoopInLambda", NUM_GANGS, [](int *a) {
    #pragma acc loop gang worker
    LOOP
  });
}

//    DMP-LABEL: FunctionDecl {{.*}} checkGangVectorLoopInLambda 'void ()'
//          DMP:   LambdaExpr
//     DMP-NEXT:     CXXRecordDecl
//          DMP:       CXXMethodDecl
//     DMP-NEXT:         ParmVarDecl
//     DMP-NEXT:         CompoundStmt
//          DMP:           ACCLoopDirective
//     DMP-NEXT:             ACCGangClause
//      DMP-NOT:               <implicit>
//     DMP-NEXT:             ACCVectorClause
//      DMP-NOT:               <implicit>
//     DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//     DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//     DMP-NEXT:               DeclRefExpr {{.*}} 'a'
// DMP-GWV-NEXT:             ACCWorkerClause {{.*}} <implicit>
//   DMP-G-NEXT:             impl: OMPDistributeSimdDirective
// DMP-GWV-NEXT:             impl: OMPDistributeParallelForSimdDirective
//      DMP-NOT:               OMP
//          DMP:               ForStmt
//          DMP:         ACCRoutineDeclAttr {{.*}} Implicit Gang OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//       PRT-LABEL: void checkGangVectorLoopInLambda() {
//        PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//      PRT-A-NEXT:     {{^ *}}#pragma acc loop gang vector{{$}}
//   PRT-AO-G-NEXT:     {{^ *}}// #pragma omp distribute simd{{$}}
// PRT-AO-GWV-NEXT:     {{^ *}}// #pragma omp distribute parallel for simd{{$}}
//    PRT-O-G-NEXT:     {{^ *}}#pragma omp distribute simd{{$}}
//  PRT-O-GWV-NEXT:     {{^ *}}#pragma omp distribute parallel for simd{{$}}
//     PRT-OA-NEXT:     {{^ *}}// #pragma acc loop gang vector{{$}}
//             PRT:   });
//        PRT-NEXT: }
//
// EXE-LABEL:checkGangVectorLoopInLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
static void checkGangVectorLoopInLambda() {
  callLambdaInParallel("checkGangVectorLoopInLambda", NUM_GANGS, [](int *a) {
    #pragma acc loop gang vector
    LOOP
  });
}

// DMP-LABEL: FunctionDecl {{.*}} checkWorkerVectorLoopInLambda 'void ()'
//       DMP:   LambdaExpr
//  DMP-NEXT:     CXXRecordDecl
//       DMP:       CXXMethodDecl
//  DMP-NEXT:         ParmVarDecl
//  DMP-NEXT:         CompoundStmt
//       DMP:           ACCLoopDirective
//  DMP-NEXT:             ACCWorkerClause
//  DMP-NEXT:             ACCVectorClause
//  DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:               DeclRefExpr {{.*}} 'a'
//  DMP-NEXT:             impl: OMPParallelForSimdDirective
//   DMP-NOT:               OMP
//       DMP:               ForStmt
//       DMP:         ACCRoutineDeclAttr {{.*}} Implicit Worker OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//   PRT-LABEL: void checkWorkerVectorLoopInLambda() {
//    PRT-NEXT:   callLambdaInParallel("{{.*}}", 1, [](int *a) {
//  PRT-A-NEXT:     {{^ *}}#pragma acc loop worker vector{{$}}
// PRT-AO-NEXT:     {{^ *}}// #pragma omp parallel for simd{{$}}
//  PRT-O-NEXT:     {{^ *}}#pragma omp parallel for simd{{$}}
// PRT-OA-NEXT:     {{^ *}}// #pragma acc loop worker vector{{$}}
//         PRT:   });
//    PRT-NEXT: }
//
// EXE-LABEL:checkWorkerVectorLoopInLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
static void checkWorkerVectorLoopInLambda() {
  callLambdaInParallel("checkWorkerVectorLoopInLambda", 1, [](int *a) {
    #pragma acc loop worker vector
    LOOP
  });
}

// DMP-LABEL: FunctionDecl {{.*}} checkGangWorkerVectorLoopInLambda 'void ()'
//       DMP:   LambdaExpr
//  DMP-NEXT:     CXXRecordDecl
//       DMP:       CXXMethodDecl
//  DMP-NEXT:         ParmVarDecl
//  DMP-NEXT:         CompoundStmt
//       DMP:           ACCLoopDirective
//  DMP-NEXT:             ACCVectorClause
//  DMP-NEXT:             ACCGangClause
//  DMP-NEXT:             ACCWorkerClause
//  DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:               DeclRefExpr {{.*}} 'a'
//  DMP-NEXT:             impl: OMPDistributeParallelForSimdDirective
//   DMP-NOT:               OMP
//       DMP:               ForStmt
//       DMP:         ACCRoutineDeclAttr {{.*}} Implicit Gang OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//   PRT-LABEL: void checkGangWorkerVectorLoopInLambda() {
//    PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//  PRT-A-NEXT:     {{^ *}}#pragma acc loop vector gang worker{{$}}
// PRT-AO-NEXT:     {{^ *}}// #pragma omp distribute parallel for simd{{$}}
//  PRT-O-NEXT:     {{^ *}}#pragma omp distribute parallel for simd{{$}}
// PRT-OA-NEXT:     {{^ *}}// #pragma acc loop vector gang worker{{$}}
//         PRT:   });
//    PRT-NEXT: }
//
// EXE-LABEL:checkGangWorkerVectorLoopInLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
static void checkGangWorkerVectorLoopInLambda() {
  callLambdaInParallel("checkGangWorkerVectorLoopInLambda", NUM_GANGS, [](int *a) {
    #pragma acc loop vector gang worker
    LOOP
  });
}

// DMP-LABEL: FunctionDecl {{.*}} checkSeqLoopInLambda 'void ()'
//       DMP:   LambdaExpr
//  DMP-NEXT:     CXXRecordDecl
//       DMP:       CXXMethodDecl
//  DMP-NEXT:         ParmVarDecl
//  DMP-NEXT:         CompoundStmt
//       DMP:           ACCLoopDirective
//  DMP-NEXT:             ACCSeqClause
//  DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:               DeclRefExpr {{.*}} 'a'
//  DMP-NEXT:             impl: ForStmt
//       DMP:         ACCRoutineDeclAttr {{.*}} Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//   PRT-LABEL: void checkSeqLoopInLambda() {
//    PRT-NEXT:   callLambdaInParallel("{{.*}}", 1, [](int *a) {
//  PRT-A-NEXT:     {{^ *}}#pragma acc loop seq
// PRT-AO-SAME:     {{^}} // discarded in OpenMP translation
//  PRT-A-SAME:     {{^$}}
// PRT-OA-NEXT:     {{^ *}}// #pragma acc loop seq // discarded in OpenMP translation{{$}}
//         PRT:   });
//    PRT-NEXT: }
//
// EXE-LABEL:checkSeqLoopInLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
static void checkSeqLoopInLambda() {
  callLambdaInParallel("checkSeqLoopInLambda", 1, [](int *a) {
    #pragma acc loop seq
    LOOP
  });
}

// DMP-LABEL: FunctionDecl {{.*}} checkNakedLoopInLambda 'void ()'
//       DMP:   LambdaExpr
//  DMP-NEXT:     CXXRecordDecl
//       DMP:       CXXMethodDecl
//  DMP-NEXT:         ParmVarDecl
//  DMP-NEXT:         CompoundStmt
//       DMP:           ACCLoopDirective
//  DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//  DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:               DeclRefExpr {{.*}} 'a'
//  DMP-NEXT:             impl: ForStmt
//       DMP:         ACCRoutineDeclAttr {{.*}} Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//   PRT-LABEL: void checkNakedLoopInLambda() {
//    PRT-NEXT:   callLambdaInParallel("{{.*}}", 1, [](int *a) {
//  PRT-A-NEXT:     {{^ *}}#pragma acc loop
// PRT-AO-SAME:     {{^}} // discarded in OpenMP translation
//  PRT-A-SAME:     {{^$}}
// PRT-OA-NEXT:     {{^ *}}// #pragma acc loop // discarded in OpenMP translation{{$}}
//         PRT:   });
//    PRT-NEXT: }
//
// EXE-LABEL:checkNakedLoopInLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
static void checkNakedLoopInLambda() {
  callLambdaInParallel("checkNakedLoopInLambda", 1, [](int *a) {
    #pragma acc loop
    LOOP
  });
}

// Currently, auto->seq always and before implicit routine directives.
//
// DMP-LABEL: FunctionDecl {{.*}} checkAutoLoopInLambda 'void ()'
//       DMP:   LambdaExpr
//  DMP-NEXT:     CXXRecordDecl
//       DMP:       CXXMethodDecl
//  DMP-NEXT:         ParmVarDecl
//  DMP-NEXT:         CompoundStmt
//       DMP:           ACCLoopDirective
//  DMP-NEXT:             ACCAutoClause
//  DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:               DeclRefExpr {{.*}} 'a'
//  DMP-NEXT:             impl: ForStmt
//       DMP:         ACCRoutineDeclAttr {{.*}} Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//   PRT-LABEL: void checkAutoLoopInLambda() {
//    PRT-NEXT:   callLambdaInParallel("{{.*}}", 1, [](int *a) {
//  PRT-A-NEXT:     {{^ *}}#pragma acc loop auto
// PRT-AO-SAME:     {{^}} // discarded in OpenMP translation
//  PRT-A-SAME:     {{^$}}
// PRT-OA-NEXT:     {{^ *}}// #pragma acc loop auto // discarded in OpenMP translation{{$}}
//         PRT:   });
//    PRT-NEXT: }
//
// EXE-LABEL:checkAutoLoopInLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
static void checkAutoLoopInLambda() {
  callLambdaInParallel("checkAutoLoopInLambda", 1, [](int *a) {
    #pragma acc loop auto
    LOOP
  });
}

// Currently, auto->seq always and before implicit routine directives.
//
// DMP-LABEL: FunctionDecl {{.*}} checkAutoGangWorkerVectorLoopInLambda 'void ()'
//       DMP:   LambdaExpr
//  DMP-NEXT:     CXXRecordDecl
//       DMP:       CXXMethodDecl
//  DMP-NEXT:         ParmVarDecl
//  DMP-NEXT:         CompoundStmt
//       DMP:           ACCLoopDirective
//  DMP-NEXT:             ACCAutoClause
//  DMP-NEXT:             ACCGangClause
//  DMP-NEXT:             ACCWorkerClause
//  DMP-NEXT:             ACCVectorClause
//  DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:               DeclRefExpr {{.*}} 'a'
//  DMP-NEXT:             impl: ForStmt
//       DMP:         ACCRoutineDeclAttr {{.*}} Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//   PRT-LABEL: void checkAutoGangWorkerVectorLoopInLambda() {
//    PRT-NEXT:   callLambdaInParallel("{{.*}}", 1, [](int *a) {
//  PRT-A-NEXT:     {{^ *}}#pragma acc loop auto gang worker vector
// PRT-AO-SAME:     {{^}} // discarded in OpenMP translation
//  PRT-A-SAME:     {{^$}}
// PRT-OA-NEXT:     {{^ *}}// #pragma acc loop auto gang worker vector // discarded in OpenMP translation{{$}}
//         PRT:   });
//    PRT-NEXT: }
//
// EXE-LABEL:checkAutoGangWorkerVectorLoopInLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
static void checkAutoGangWorkerVectorLoopInLambda() {
  callLambdaInParallel("checkAutoGangWorkerVectorLoopInLambda", 1, [](int *a) {
    #pragma acc loop auto gang worker vector
    LOOP
  });
}

// The routine directive for this lambda is implied by the lambda's use instead
// of its body.
//
// DMP-LABEL: FunctionDecl {{.*}} checkSerialLoopInLambda 'void ()'
//       DMP:   LambdaExpr
//  DMP-NEXT:     CXXRecordDecl
//       DMP:       CXXMethodDecl
//  DMP-NEXT:         ParmVarDecl
//  DMP-NEXT:         CompoundStmt
//  DMP-NEXT:           ForStmt
//       DMP:         ACCRoutineDeclAttr {{.*}} Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//   PRT-LABEL: void checkSerialLoopInLambda() {
//    PRT-NEXT:   callLambdaInParallel("{{.*}}", 1, [](int *a) {
//     PRT-NOT:     #pragma acc
//         PRT:   });
//    PRT-NEXT: }
//
// EXE-LABEL:checkSerialLoopInLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
static void checkSerialLoopInLambda() {
  callLambdaInParallel("checkSerialLoopInLambda", 1, [](int *a) {
    LOOP
  });
}

//------------------------------------------------------------------------------
// Check with multiple impliers.
//
// First one of highest level (gang, worker, or vector) should be identified.
// Implicit clauses of that level and lower should be computed for loop
// constructs.
//------------------------------------------------------------------------------

//    DMP-LABEL: FunctionDecl {{.*}} checkGangHighLoopLowLoopInLambda 'void ()'
//          DMP:   LambdaExpr
//     DMP-NEXT:     CXXRecordDecl
//          DMP:       CXXMethodDecl
//     DMP-NEXT:         ParmVarDecl
//     DMP-NEXT:         CompoundStmt
//          DMP:           ACCLoopDirective
//     DMP-NEXT:             ACCGangClause
//     DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//     DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//     DMP-NEXT:               DeclRefExpr {{.*}} 'a'
// DMP-GWV-NEXT:             ACCWorkerClause {{.*}} <implicit>
// DMP-GWV-NEXT:             ACCVectorClause {{.*}} <implicit>
//   DMP-G-NEXT:             impl: OMPDistributeDirective
// DMP-GWV-NEXT:             impl: OMPDistributeParallelForSimdDirective
//      DMP-NOT:               OMP
//          DMP:               ForStmt
//          DMP:           ACCLoopDirective
//     DMP-NEXT:             ACCVectorClause
//     DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//     DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//     DMP-NEXT:               DeclRefExpr {{.*}} 'a'
//     DMP-NEXT:             ACCGangClause {{.*}} <implicit>
// DMP-GWV-NEXT:             ACCWorkerClause {{.*}} <implicit>
//   DMP-G-NEXT:             impl: OMPDistributeSimdDirective
// DMP-GWV-NEXT:             impl: OMPDistributeParallelForSimdDirective
//      DMP-NOT:               OMP
//          DMP:               ForStmt
//          DMP:         ACCRoutineDeclAttr {{.*}} Implicit Gang OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//       PRT-LABEL: void checkGangHighLoopLowLoopInLambda() {
//        PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//      PRT-A-NEXT:     {{^ *}}#pragma acc loop gang{{$}}
//   PRT-AO-G-NEXT:     {{^ *}}// #pragma omp distribute{{$}}
// PRT-AO-GWV-NEXT:     {{^ *}}// #pragma omp distribute parallel for simd{{$}}
//    PRT-O-G-NEXT:     {{^ *}}#pragma omp distribute{{$}}
//  PRT-O-GWV-NEXT:     {{^ *}}#pragma omp distribute parallel for simd{{$}}
//     PRT-OA-NEXT:     {{^ *}}// #pragma acc loop gang{{$}}
//        PRT-NEXT:     {{LOOP|for \(.*\)[[:space:]].*;$}}
//      PRT-A-NEXT:     {{^ *}}#pragma acc loop vector{{$}}
//   PRT-AO-G-NEXT:     {{^ *}}// #pragma omp distribute simd{{$}}
// PRT-AO-GWV-NEXT:     {{^ *}}// #pragma omp distribute parallel for simd{{$}}
//    PRT-O-G-NEXT:     {{^ *}}#pragma omp distribute simd{{$}}
//  PRT-O-GWV-NEXT:     {{^ *}}#pragma omp distribute parallel for simd{{$}}
//     PRT-OA-NEXT:     {{^ *}}// #pragma acc loop vector{{$}}
//        PRT-NEXT:     {{LOOP|for \(.*\)[[:space:]].*;$}}
//        PRT-NEXT:   });
//        PRT-NEXT: }
//
// EXE-LABEL:checkGangHighLoopLowLoopInLambda
//  EXE-NEXT:a[0]=20
//  EXE-NEXT:a[1]=21
//  EXE-NEXT:a[2]=22
//  EXE-NEXT:a[3]=23
static void checkGangHighLoopLowLoopInLambda() {
  callLambdaInParallel("checkGangHighLoopLowLoopInLambda", NUM_GANGS, [](int *a) {
    #pragma acc loop gang
    LOOP
    #pragma acc loop vector
    LOOP
  });
}

//    DMP-LABEL: FunctionDecl {{.*}} checkLowLoopGangHighLoopInLambda 'void ()'
//          DMP:   LambdaExpr
//     DMP-NEXT:     CXXRecordDecl
//          DMP:       CXXMethodDecl
//     DMP-NEXT:         ParmVarDecl
//     DMP-NEXT:         CompoundStmt
//          DMP:           ACCLoopDirective
//     DMP-NEXT:             ACCVectorClause
//     DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//     DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//     DMP-NEXT:               DeclRefExpr {{.*}} 'a'
//     DMP-NEXT:             ACCGangClause {{.*}} <implicit>
// DMP-GWV-NEXT:             ACCWorkerClause {{.*}} <implicit>
//   DMP-G-NEXT:             impl: OMPDistributeSimdDirective
// DMP-GWV-NEXT:             impl: OMPDistributeParallelForSimdDirective
//      DMP-NOT:               OMP
//          DMP:               ForStmt
//          DMP:           ACCLoopDirective
//     DMP-NEXT:             ACCGangClause
//     DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//     DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//     DMP-NEXT:               DeclRefExpr {{.*}} 'a'
// DMP-GWV-NEXT:             ACCWorkerClause {{.*}} <implicit>
// DMP-GWV-NEXT:             ACCVectorClause {{.*}} <implicit>
//   DMP-G-NEXT:             impl: OMPDistributeDirective
// DMP-GWV-NEXT:             impl: OMPDistributeParallelForSimdDirective
//      DMP-NOT:               OMP
//          DMP:               ForStmt
//          DMP:         ACCRoutineDeclAttr {{.*}} Implicit Gang OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//       PRT-LABEL: void checkLowLoopGangHighLoopInLambda() {
//        PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//      PRT-A-NEXT:     {{^ *}}#pragma acc loop vector{{$}}
//   PRT-AO-G-NEXT:     {{^ *}}// #pragma omp distribute simd{{$}}
// PRT-AO-GWV-NEXT:     {{^ *}}// #pragma omp distribute parallel for simd{{$}}
//    PRT-O-G-NEXT:     {{^ *}}#pragma omp distribute simd{{$}}
//  PRT-O-GWV-NEXT:     {{^ *}}#pragma omp distribute parallel for simd{{$}}
//     PRT-OA-NEXT:     {{^ *}}// #pragma acc loop vector{{$}}
//        PRT-NEXT:     {{LOOP|for \(.*\)[[:space:]].*;$}}
//      PRT-A-NEXT:     {{^ *}}#pragma acc loop gang{{$}}
//   PRT-AO-G-NEXT:     {{^ *}}// #pragma omp distribute{{$}}
// PRT-AO-GWV-NEXT:     {{^ *}}// #pragma omp distribute parallel for simd{{$}}
//    PRT-O-G-NEXT:     {{^ *}}#pragma omp distribute{{$}}
//  PRT-O-GWV-NEXT:     {{^ *}}#pragma omp distribute parallel for simd{{$}}
//     PRT-OA-NEXT:     {{^ *}}// #pragma acc loop gang{{$}}
//        PRT-NEXT:     {{LOOP|for \(.*\)[[:space:]].*;$}}
//        PRT-NEXT:   });
//        PRT-NEXT: }
//
// EXE-LABEL:checkLowLoopGangHighLoopInLambda
//  EXE-NEXT:a[0]=20
//  EXE-NEXT:a[1]=21
//  EXE-NEXT:a[2]=22
//  EXE-NEXT:a[3]=23
static void checkLowLoopGangHighLoopInLambda() {
  callLambdaInParallel("checkLowLoopGangHighLoopInLambda", NUM_GANGS, [](int *a) {
    #pragma acc loop vector
    LOOP
    #pragma acc loop gang
    LOOP
  });
}

//    DMP-LABEL: FunctionDecl {{.*}} checkNonGangHighLoopLowLoopInLambda 'void ()'
//          DMP:   LambdaExpr
//     DMP-NEXT:     CXXRecordDecl
//          DMP:       CXXMethodDecl
//     DMP-NEXT:         ParmVarDecl
//     DMP-NEXT:         CompoundStmt
//          DMP:           ACCLoopDirective
//     DMP-NEXT:             ACCWorkerClause
//     DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//     DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//     DMP-NEXT:               DeclRefExpr {{.*}} 'a'
// DMP-GWV-NEXT:             ACCVectorClause {{.*}} <implicit>
//   DMP-G-NEXT:             impl: OMPParallelForDirective
// DMP-GWV-NEXT:             impl: OMPParallelForSimdDirective
//      DMP-NOT:               OMP
//          DMP:               ForStmt
//          DMP:           ACCLoopDirective
//     DMP-NEXT:             ACCVectorClause
//     DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//     DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//     DMP-NEXT:               DeclRefExpr {{.*}} 'a'
// DMP-GWV-NEXT:             ACCWorkerClause {{.*}} <implicit>
//   DMP-G-NEXT:             impl: OMPSimdDirective
// DMP-GWV-NEXT:             impl: OMPParallelForSimdDirective
//      DMP-NOT:               OMP
//          DMP:               ForStmt
//          DMP:         ACCRoutineDeclAttr {{.*}} Implicit Worker OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//       PRT-LABEL: void checkNonGangHighLoopLowLoopInLambda() {
//        PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//      PRT-A-NEXT:     {{^ *}}#pragma acc loop worker{{$}}
//   PRT-AO-G-NEXT:     {{^ *}}// #pragma omp parallel for{{$}}
// PRT-AO-GWV-NEXT:     {{^ *}}// #pragma omp parallel for simd{{$}}
//    PRT-O-G-NEXT:     {{^ *}}#pragma omp parallel for{{$}}
//  PRT-O-GWV-NEXT:     {{^ *}}#pragma omp parallel for simd{{$}}
//     PRT-OA-NEXT:     {{^ *}}// #pragma acc loop worker{{$}}
//        PRT-NEXT:     {{LOOP|for \(.*\)[[:space:]].*;$}}
//      PRT-A-NEXT:     {{^ *}}#pragma acc loop vector{{$}}
//   PRT-AO-G-NEXT:     {{^ *}}// #pragma omp simd{{$}}
// PRT-AO-GWV-NEXT:     {{^ *}}// #pragma omp parallel for simd{{$}}
//    PRT-O-G-NEXT:     {{^ *}}#pragma omp simd{{$}}
//  PRT-O-GWV-NEXT:     {{^ *}}#pragma omp parallel for simd{{$}}
//     PRT-OA-NEXT:     {{^ *}}// #pragma acc loop vector{{$}}
//        PRT-NEXT:     {{LOOP|for \(.*\)[[:space:]].*;$}}
//        PRT-NEXT:   });
//        PRT-NEXT: }
//
// EXE-LABEL:checkNonGangHighLoopLowLoopInLambda
//  EXE-NEXT:a[0]=20
//  EXE-NEXT:a[1]=21
//  EXE-NEXT:a[2]=22
//  EXE-NEXT:a[3]=23
static void checkNonGangHighLoopLowLoopInLambda() {
  callLambdaInParallel("checkNonGangHighLoopLowLoopInLambda", 1, [](int *a) {
    #pragma acc loop worker
    LOOP
    #pragma acc loop vector
    LOOP
  });
}

//    DMP-LABEL: FunctionDecl {{.*}} checkLowLoopNonGangHighLoopInLambda 'void ()'
//          DMP:   LambdaExpr
//     DMP-NEXT:     CXXRecordDecl
//          DMP:       CXXMethodDecl
//     DMP-NEXT:         ParmVarDecl
//     DMP-NEXT:         CompoundStmt
//          DMP:           ACCLoopDirective
//     DMP-NEXT:             ACCVectorClause
//     DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//     DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//     DMP-NEXT:               DeclRefExpr {{.*}} 'a'
// DMP-GWV-NEXT:             ACCWorkerClause {{.*}} <implicit>
//   DMP-G-NEXT:             impl: OMPSimdDirective
// DMP-GWV-NEXT:             impl: OMPParallelForSimdDirective
//      DMP-NOT:               OMP
//          DMP:               ForStmt
//          DMP:           ACCLoopDirective
//     DMP-NEXT:             ACCWorkerClause
//     DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//     DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//     DMP-NEXT:               DeclRefExpr {{.*}} 'a'
// DMP-GWV-NEXT:             ACCVectorClause {{.*}} <implicit>
//   DMP-G-NEXT:             impl: OMPParallelForDirective
// DMP-GWV-NEXT:             impl: OMPParallelForSimdDirective
//      DMP-NOT:               OMP
//          DMP:               ForStmt
//          DMP:         ACCRoutineDeclAttr {{.*}} Implicit Worker OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//       PRT-LABEL: void checkLowLoopNonGangHighLoopInLambda() {
//        PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//      PRT-A-NEXT:     {{^ *}}#pragma acc loop vector{{$}}
//   PRT-AO-G-NEXT:     {{^ *}}// #pragma omp simd{{$}}
// PRT-AO-GWV-NEXT:     {{^ *}}// #pragma omp parallel for simd{{$}}
//    PRT-O-G-NEXT:     {{^ *}}#pragma omp simd{{$}}
//  PRT-O-GWV-NEXT:     {{^ *}}#pragma omp parallel for simd{{$}}
//     PRT-OA-NEXT:     {{^ *}}// #pragma acc loop vector{{$}}
//        PRT-NEXT:     {{LOOP|for \(.*\)[[:space:]].*;$}}
//      PRT-A-NEXT:     {{^ *}}#pragma acc loop worker{{$}}
//   PRT-AO-G-NEXT:     {{^ *}}// #pragma omp parallel for{{$}}
// PRT-AO-GWV-NEXT:     {{^ *}}// #pragma omp parallel for simd{{$}}
//    PRT-O-G-NEXT:     {{^ *}}#pragma omp parallel for{{$}}
//  PRT-O-GWV-NEXT:     {{^ *}}#pragma omp parallel for simd{{$}}
//     PRT-OA-NEXT:     {{^ *}}// #pragma acc loop worker{{$}}
//        PRT-NEXT:     {{LOOP|for \(.*\)[[:space:]].*;$}}
//        PRT-NEXT:   });
//        PRT-NEXT: }
//
// EXE-LABEL:checkLowLoopNonGangHighLoopInLambda
//  EXE-NEXT:a[0]=20
//  EXE-NEXT:a[1]=21
//  EXE-NEXT:a[2]=22
//  EXE-NEXT:a[3]=23
static void checkLowLoopNonGangHighLoopInLambda() {
  callLambdaInParallel("checkLowLoopNonGangHighLoopInLambda", 1, [](int *a) {
    #pragma acc loop vector
    LOOP
    #pragma acc loop worker
    LOOP
  });
}

// DMP-LABEL: FunctionDecl {{.*}} checkGangHighCallLowCallInLambda 'void ()'
//       DMP:   LambdaExpr
//  DMP-NEXT:     CXXRecordDecl
//       DMP:       CXXMethodDecl
//  DMP-NEXT:         ParmVarDecl
//  DMP-NEXT:         CompoundStmt
//  DMP-NEXT:           CallExpr
//  DMP-NEXT:             ImplicitCastExpr
//  DMP-NEXT:               DeclRefExpr {{.*}} 'gangFn'
//       DMP:           CallExpr
//  DMP-NEXT:             ImplicitCastExpr
//  DMP-NEXT:               DeclRefExpr {{.*}} 'vectorFn'
//       DMP:         ACCRoutineDeclAttr {{.*}} Implicit Gang OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//   PRT-LABEL: void checkGangHighCallLowCallInLambda() {
//    PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//    PRT-NEXT:     gangFn(a);
//    PRT-NEXT:     vectorFn(a);
//    PRT-NEXT:   });
//    PRT-NEXT: }
//
// EXE-LABEL:checkGangHighCallLowCallInLambda
//  EXE-NEXT:a[0]=20
//  EXE-NEXT:a[1]=21
//  EXE-NEXT:a[2]=22
//  EXE-NEXT:a[3]=23
static void checkGangHighCallLowCallInLambda() {
  callLambdaInParallel("checkGangHighCallLowCallInLambda", 1, [](int *a) {
    gangFn(a);
    vectorFn(a);
  });
}

// DMP-LABEL: FunctionDecl {{.*}} checkLowCallGangHighCallInLambda 'void ()'
//       DMP:   LambdaExpr
//  DMP-NEXT:     CXXRecordDecl
//       DMP:       CXXMethodDecl
//  DMP-NEXT:         ParmVarDecl
//  DMP-NEXT:         CompoundStmt
//  DMP-NEXT:           CallExpr
//  DMP-NEXT:             ImplicitCastExpr
//  DMP-NEXT:               DeclRefExpr {{.*}} 'vectorFn'
//       DMP:           CallExpr
//  DMP-NEXT:             ImplicitCastExpr
//  DMP-NEXT:               DeclRefExpr {{.*}} 'gangFn'
//       DMP:         ACCRoutineDeclAttr {{.*}} Implicit Gang OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//   PRT-LABEL: void checkLowCallGangHighCallInLambda() {
//    PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//    PRT-NEXT:     vectorFn(a);
//    PRT-NEXT:     gangFn(a);
//    PRT-NEXT:   });
//    PRT-NEXT: }
//
// EXE-LABEL:checkLowCallGangHighCallInLambda
//  EXE-NEXT:a[0]=20
//  EXE-NEXT:a[1]=21
//  EXE-NEXT:a[2]=22
//  EXE-NEXT:a[3]=23
static void checkLowCallGangHighCallInLambda() {
  callLambdaInParallel("checkLowCallGangHighCallInLambda", 1, [](int *a) {
    vectorFn(a);
    gangFn(a);
  });
}

// DMP-LABEL: FunctionDecl {{.*}} checkNonGangHighCallLowCallInLambda 'void ()'
//       DMP:   LambdaExpr
//  DMP-NEXT:     CXXRecordDecl
//       DMP:       CXXMethodDecl
//  DMP-NEXT:         ParmVarDecl
//  DMP-NEXT:         CompoundStmt
//  DMP-NEXT:           CallExpr
//  DMP-NEXT:             ImplicitCastExpr
//  DMP-NEXT:               DeclRefExpr {{.*}} 'workerFn'
//       DMP:           CallExpr
//  DMP-NEXT:             ImplicitCastExpr
//  DMP-NEXT:               DeclRefExpr {{.*}} 'vectorFn'
//       DMP:         ACCRoutineDeclAttr {{.*}} Implicit Worker OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//   PRT-LABEL: void checkNonGangHighCallLowCallInLambda() {
//    PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//    PRT-NEXT:     workerFn(a);
//    PRT-NEXT:     vectorFn(a);
//    PRT-NEXT:   });
//    PRT-NEXT: }
//
// EXE-LABEL:checkNonGangHighCallLowCallInLambda
//  EXE-NEXT:a[0]=20
//  EXE-NEXT:a[1]=21
//  EXE-NEXT:a[2]=22
//  EXE-NEXT:a[3]=23
static void checkNonGangHighCallLowCallInLambda() {
  callLambdaInParallel("checkNonGangHighCallLowCallInLambda", 1, [](int *a) {
    workerFn(a);
    vectorFn(a);
  });
}

// DMP-LABEL: FunctionDecl {{.*}} checkLowCallNonGangHighCallInLambda 'void ()'
//       DMP:   LambdaExpr
//  DMP-NEXT:     CXXRecordDecl
//       DMP:       CXXMethodDecl
//  DMP-NEXT:         ParmVarDecl
//  DMP-NEXT:         CompoundStmt
//  DMP-NEXT:           CallExpr
//  DMP-NEXT:             ImplicitCastExpr
//  DMP-NEXT:               DeclRefExpr {{.*}} 'vectorFn'
//       DMP:           CallExpr
//  DMP-NEXT:             ImplicitCastExpr
//  DMP-NEXT:               DeclRefExpr {{.*}} 'workerFn'
//       DMP:         ACCRoutineDeclAttr {{.*}} Implicit Worker OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//   PRT-LABEL: void checkLowCallNonGangHighCallInLambda() {
//    PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//    PRT-NEXT:     vectorFn(a);
//    PRT-NEXT:     workerFn(a);
//    PRT-NEXT:   });
//    PRT-NEXT: }
//
// EXE-LABEL:checkLowCallNonGangHighCallInLambda
//  EXE-NEXT:a[0]=20
//  EXE-NEXT:a[1]=21
//  EXE-NEXT:a[2]=22
//  EXE-NEXT:a[3]=23
static void checkLowCallNonGangHighCallInLambda() {
  callLambdaInParallel("checkLowCallNonGangHighCallInLambda", 1, [](int *a) {
    vectorFn(a);
    workerFn(a);
  });
}

//    DMP-LABEL: FunctionDecl {{.*}} checkGangHighLoopLowCallInLambda 'void ()'
//          DMP:   LambdaExpr
//     DMP-NEXT:     CXXRecordDecl
//          DMP:       CXXMethodDecl
//     DMP-NEXT:         ParmVarDecl
//     DMP-NEXT:         CompoundStmt
//          DMP:           ACCLoopDirective
//     DMP-NEXT:             ACCGangClause
//     DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//     DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//     DMP-NEXT:               DeclRefExpr {{.*}} 'a'
// DMP-GWV-NEXT:             ACCWorkerClause {{.*}} <implicit>
// DMP-GWV-NEXT:             ACCVectorClause {{.*}} <implicit>
//   DMP-G-NEXT:             impl: OMPDistributeDirective
// DMP-GWV-NEXT:             impl: OMPDistributeParallelForSimdDirective
//      DMP-NOT:               OMP
//          DMP:               ForStmt
//          DMP:           CallExpr
//     DMP-NEXT:             ImplicitCastExpr
//     DMP-NEXT:               DeclRefExpr {{.*}} 'workerFn'
//          DMP:         ACCRoutineDeclAttr {{.*}} Implicit Gang OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//       PRT-LABEL: void checkGangHighLoopLowCallInLambda() {
//        PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//      PRT-A-NEXT:     {{^ *}}#pragma acc loop gang{{$}}
//   PRT-AO-G-NEXT:     {{^ *}}// #pragma omp distribute{{$}}
// PRT-AO-GWV-NEXT:     {{^ *}}// #pragma omp distribute parallel for simd{{$}}
//    PRT-O-G-NEXT:     {{^ *}}#pragma omp distribute{{$}}
//  PRT-O-GWV-NEXT:     {{^ *}}#pragma omp distribute parallel for simd{{$}}
//     PRT-OA-NEXT:     {{^ *}}// #pragma acc loop gang{{$}}
//        PRT-NEXT:     {{LOOP|for \(.*\)[[:space:]].*;$}}
//        PRT-NEXT:     workerFn(a);
//        PRT-NEXT:   });
//        PRT-NEXT: }
//
// EXE-LABEL:checkGangHighLoopLowCallInLambda
//  EXE-NEXT:a[0]=20
//  EXE-NEXT:a[1]=21
//  EXE-NEXT:a[2]=22
//  EXE-NEXT:a[3]=23
static void checkGangHighLoopLowCallInLambda() {
  callLambdaInParallel("checkGangHighLoopLowCallInLambda", 1, [](int *a) {
    #pragma acc loop gang
    LOOP
    workerFn(a);
  });
}

//    DMP-LABEL: FunctionDecl {{.*}} checkLowCallGangHighLoopInLambda 'void ()'
//          DMP:   LambdaExpr
//     DMP-NEXT:     CXXRecordDecl
//          DMP:       CXXMethodDecl
//     DMP-NEXT:         ParmVarDecl
//     DMP-NEXT:         CompoundStmt
//          DMP:           CallExpr
//     DMP-NEXT:             ImplicitCastExpr
//     DMP-NEXT:               DeclRefExpr {{.*}} 'workerFn'
//          DMP:           ACCLoopDirective
//     DMP-NEXT:             ACCGangClause
//     DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//     DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//     DMP-NEXT:               DeclRefExpr {{.*}} 'a'
// DMP-GWV-NEXT:             ACCWorkerClause {{.*}} <implicit>
// DMP-GWV-NEXT:             ACCVectorClause {{.*}} <implicit>
//   DMP-G-NEXT:             impl: OMPDistributeDirective
// DMP-GWV-NEXT:             impl: OMPDistributeParallelForSimdDirective
//      DMP-NOT:               OMP
//          DMP:               ForStmt
//          DMP:         ACCRoutineDeclAttr {{.*}} Implicit Gang OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//       PRT-LABEL: void checkLowCallGangHighLoopInLambda() {
//        PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//        PRT-NEXT:     workerFn(a);
//      PRT-A-NEXT:     {{^ *}}#pragma acc loop gang{{$}}
//   PRT-AO-G-NEXT:     {{^ *}}// #pragma omp distribute{{$}}
// PRT-AO-GWV-NEXT:     {{^ *}}// #pragma omp distribute parallel for simd{{$}}
//    PRT-O-G-NEXT:     {{^ *}}#pragma omp distribute{{$}}
//  PRT-O-GWV-NEXT:     {{^ *}}#pragma omp distribute parallel for simd{{$}}
//     PRT-OA-NEXT:     {{^ *}}// #pragma acc loop gang{{$}}
//        PRT-NEXT:     {{LOOP|for \(.*\)[[:space:]].*;$}}
//        PRT-NEXT:   });
//        PRT-NEXT: }
//
// EXE-LABEL:checkLowCallGangHighLoopInLambda
//  EXE-NEXT:a[0]=20
//  EXE-NEXT:a[1]=21
//  EXE-NEXT:a[2]=22
//  EXE-NEXT:a[3]=23
static void checkLowCallGangHighLoopInLambda() {
  callLambdaInParallel("checkLowCallGangHighLoopInLambda", 1, [](int *a) {
    workerFn(a);
    #pragma acc loop gang
    LOOP
  });
}

//    DMP-LABEL: FunctionDecl {{.*}} checkNonGangHighLoopLowCallInLambda 'void ()'
//          DMP:   LambdaExpr
//     DMP-NEXT:     CXXRecordDecl
//          DMP:       CXXMethodDecl
//     DMP-NEXT:         ParmVarDecl
//     DMP-NEXT:         CompoundStmt
//          DMP:           ACCLoopDirective
//     DMP-NEXT:             ACCWorkerClause
//     DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//     DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//     DMP-NEXT:               DeclRefExpr {{.*}} 'a'
// DMP-GWV-NEXT:             ACCVectorClause {{.*}} <implicit>
//   DMP-G-NEXT:             impl: OMPParallelForDirective
// DMP-GWV-NEXT:             impl: OMPParallelForSimdDirective
//      DMP-NOT:               OMP
//          DMP:               ForStmt
//          DMP:           CallExpr
//     DMP-NEXT:             ImplicitCastExpr
//     DMP-NEXT:               DeclRefExpr {{.*}} 'vectorFn'
//          DMP:         ACCRoutineDeclAttr {{.*}} Implicit Worker OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//       PRT-LABEL: void checkNonGangHighLoopLowCallInLambda() {
//        PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//      PRT-A-NEXT:     {{^ *}}#pragma acc loop worker{{$}}
//   PRT-AO-G-NEXT:     {{^ *}}// #pragma omp parallel for{{$}}
// PRT-AO-GWV-NEXT:     {{^ *}}// #pragma omp parallel for simd{{$}}
//    PRT-O-G-NEXT:     {{^ *}}#pragma omp parallel for{{$}}
//  PRT-O-GWV-NEXT:     {{^ *}}#pragma omp parallel for simd{{$}}
//     PRT-OA-NEXT:     {{^ *}}// #pragma acc loop worker{{$}}
//        PRT-NEXT:     {{LOOP|for \(.*\)[[:space:]].*;$}}
//        PRT-NEXT:     vectorFn(a);
//        PRT-NEXT:   });
//        PRT-NEXT: }
//
// EXE-LABEL:checkNonGangHighLoopLowCallInLambda
//  EXE-NEXT:a[0]=20
//  EXE-NEXT:a[1]=21
//  EXE-NEXT:a[2]=22
//  EXE-NEXT:a[3]=23
static void checkNonGangHighLoopLowCallInLambda() {
  callLambdaInParallel("checkNonGangHighLoopLowCallInLambda", 1, [](int *a) {
    #pragma acc loop worker
    LOOP
    vectorFn(a);
  });
}

//    DMP-LABEL: FunctionDecl {{.*}} checkLowCallNonGangHighLoopInLambda 'void ()'
//          DMP:   LambdaExpr
//     DMP-NEXT:     CXXRecordDecl
//          DMP:       CXXMethodDecl
//     DMP-NEXT:         ParmVarDecl
//     DMP-NEXT:         CompoundStmt
//          DMP:           CallExpr
//     DMP-NEXT:             ImplicitCastExpr
//     DMP-NEXT:               DeclRefExpr {{.*}} 'vectorFn'
//          DMP:           ACCLoopDirective
//     DMP-NEXT:             ACCWorkerClause
//     DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//     DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//     DMP-NEXT:               DeclRefExpr {{.*}} 'a'
// DMP-GWV-NEXT:             ACCVectorClause {{.*}} <implicit>
//   DMP-G-NEXT:             impl: OMPParallelForDirective
// DMP-GWV-NEXT:             impl: OMPParallelForSimdDirective
//      DMP-NOT:               OMP
//          DMP:               ForStmt
//          DMP:         ACCRoutineDeclAttr {{.*}} Implicit Worker OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//       PRT-LABEL: void checkLowCallNonGangHighLoopInLambda() {
//        PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//        PRT-NEXT:     vectorFn(a);
//      PRT-A-NEXT:     {{^ *}}#pragma acc loop worker{{$}}
//   PRT-AO-G-NEXT:     {{^ *}}// #pragma omp parallel for{{$}}
// PRT-AO-GWV-NEXT:     {{^ *}}// #pragma omp parallel for simd{{$}}
//    PRT-O-G-NEXT:     {{^ *}}#pragma omp parallel for{{$}}
//  PRT-O-GWV-NEXT:     {{^ *}}#pragma omp parallel for simd{{$}}
//     PRT-OA-NEXT:     {{^ *}}// #pragma acc loop worker{{$}}
//        PRT-NEXT:     {{LOOP|for \(.*\)[[:space:]].*;$}}
//        PRT-NEXT:   });
//        PRT-NEXT: }
//
// EXE-LABEL:checkLowCallNonGangHighLoopInLambda
//  EXE-NEXT:a[0]=20
//  EXE-NEXT:a[1]=21
//  EXE-NEXT:a[2]=22
//  EXE-NEXT:a[3]=23
static void checkLowCallNonGangHighLoopInLambda() {
  callLambdaInParallel("checkLowCallNonGangHighLoopInLambda", 1, [](int *a) {
    vectorFn(a);
    #pragma acc loop worker
    LOOP
  });
}

//    DMP-LABEL: FunctionDecl {{.*}} checkGangHighCallLowLoopInLambda 'void ()'
//          DMP:   LambdaExpr
//     DMP-NEXT:     CXXRecordDecl
//          DMP:       CXXMethodDecl
//     DMP-NEXT:         ParmVarDecl
//     DMP-NEXT:         CompoundStmt
//          DMP:           CallExpr
//     DMP-NEXT:             ImplicitCastExpr
//     DMP-NEXT:               DeclRefExpr {{.*}} 'gangFn'
//          DMP:           ACCLoopDirective
//     DMP-NEXT:             ACCVectorClause
//     DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//     DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//     DMP-NEXT:               DeclRefExpr {{.*}} 'a'
//     DMP-NEXT:             ACCGangClause {{.*}} <implicit>
// DMP-GWV-NEXT:             ACCWorkerClause {{.*}} <implicit>
//   DMP-G-NEXT:             impl: OMPDistributeSimdDirective
// DMP-GWV-NEXT:             impl: OMPDistributeParallelForSimdDirective
//      DMP-NOT:               OMP
//          DMP:               ForStmt
//          DMP:         ACCRoutineDeclAttr {{.*}} Implicit Gang OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//       PRT-LABEL: void checkGangHighCallLowLoopInLambda() {
//        PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//        PRT-NEXT:     gangFn(a);
//      PRT-A-NEXT:     {{^ *}}#pragma acc loop vector{{$}}
//   PRT-AO-G-NEXT:     {{^ *}}// #pragma omp distribute simd{{$}}
// PRT-AO-GWV-NEXT:     {{^ *}}// #pragma omp distribute parallel for simd{{$}}
//    PRT-O-G-NEXT:     {{^ *}}#pragma omp distribute simd{{$}}
//  PRT-O-GWV-NEXT:     {{^ *}}#pragma omp distribute parallel for simd{{$}}
//     PRT-OA-NEXT:     {{^ *}}// #pragma acc loop vector{{$}}
//        PRT-NEXT:     {{LOOP|for \(.*\)[[:space:]].*;$}}
//        PRT-NEXT:   });
//        PRT-NEXT: }
//
// EXE-LABEL:checkGangHighCallLowLoopInLambda
//  EXE-NEXT:a[0]=20
//  EXE-NEXT:a[1]=21
//  EXE-NEXT:a[2]=22
//  EXE-NEXT:a[3]=23
static void checkGangHighCallLowLoopInLambda() {
  callLambdaInParallel("checkGangHighCallLowLoopInLambda", NUM_GANGS, [](int *a) {
    gangFn(a);
    #pragma acc loop vector
    LOOP
  });
}

//    DMP-LABEL: FunctionDecl {{.*}} checkLowLoopGangHighCallInLambda 'void ()'
//          DMP:   LambdaExpr
//     DMP-NEXT:     CXXRecordDecl
//          DMP:       CXXMethodDecl
//     DMP-NEXT:         ParmVarDecl
//     DMP-NEXT:         CompoundStmt
//          DMP:           ACCLoopDirective
//     DMP-NEXT:             ACCVectorClause
//     DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//     DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//     DMP-NEXT:               DeclRefExpr {{.*}} 'a'
//     DMP-NEXT:             ACCGangClause {{.*}} <implicit>
// DMP-GWV-NEXT:             ACCWorkerClause {{.*}} <implicit>
//   DMP-G-NEXT:             impl: OMPDistributeSimdDirective
// DMP-GWV-NEXT:             impl: OMPDistributeParallelForSimdDirective
//      DMP-NOT:               OMP
//          DMP:               ForStmt
//          DMP:           CallExpr
//     DMP-NEXT:             ImplicitCastExpr
//     DMP-NEXT:               DeclRefExpr {{.*}} 'gangFn'
//          DMP:         ACCRoutineDeclAttr {{.*}} Implicit Gang OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//       PRT-LABEL: void checkLowLoopGangHighCallInLambda() {
//        PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//      PRT-A-NEXT:     {{^ *}}#pragma acc loop vector{{$}}
//   PRT-AO-G-NEXT:     {{^ *}}// #pragma omp distribute simd{{$}}
// PRT-AO-GWV-NEXT:     {{^ *}}// #pragma omp distribute parallel for simd{{$}}
//    PRT-O-G-NEXT:     {{^ *}}#pragma omp distribute simd{{$}}
//  PRT-O-GWV-NEXT:     {{^ *}}#pragma omp distribute parallel for simd{{$}}
//     PRT-OA-NEXT:     {{^ *}}// #pragma acc loop vector{{$}}
//        PRT-NEXT:     {{LOOP|for \(.*\)[[:space:]].*;$}}
//        PRT-NEXT:     gangFn(a);
//        PRT-NEXT:   });
//        PRT-NEXT: }
//
// EXE-LABEL:checkLowLoopGangHighCallInLambda
//  EXE-NEXT:a[0]=20
//  EXE-NEXT:a[1]=21
//  EXE-NEXT:a[2]=22
//  EXE-NEXT:a[3]=23
static void checkLowLoopGangHighCallInLambda() {
  callLambdaInParallel("checkLowLoopGangHighCallInLambda", NUM_GANGS, [](int *a) {
    #pragma acc loop vector
    LOOP
    gangFn(a);
  });
}

//    DMP-LABEL: FunctionDecl {{.*}} checkNonGangHighCallLowLoopInLambda 'void ()'
//          DMP:   LambdaExpr
//     DMP-NEXT:     CXXRecordDecl
//          DMP:       CXXMethodDecl
//     DMP-NEXT:         ParmVarDecl
//     DMP-NEXT:         CompoundStmt
//          DMP:           CallExpr
//     DMP-NEXT:             ImplicitCastExpr
//     DMP-NEXT:               DeclRefExpr {{.*}} 'workerFn'
//          DMP:           ACCLoopDirective
//     DMP-NEXT:             ACCVectorClause
//     DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//     DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//     DMP-NEXT:               DeclRefExpr {{.*}} 'a'
// DMP-GWV-NEXT:             ACCWorkerClause {{.*}} <implicit>
//   DMP-G-NEXT:             impl: OMPSimdDirective
// DMP-GWV-NEXT:             impl: OMPParallelForSimdDirective
//      DMP-NOT:               OMP
//          DMP:               ForStmt
//          DMP:         ACCRoutineDeclAttr {{.*}} Implicit Worker OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//       PRT-LABEL: void checkNonGangHighCallLowLoopInLambda() {
//        PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//        PRT-NEXT:     workerFn(a);
//      PRT-A-NEXT:     {{^ *}}#pragma acc loop vector{{$}}
//   PRT-AO-G-NEXT:     {{^ *}}// #pragma omp simd{{$}}
// PRT-AO-GWV-NEXT:     {{^ *}}// #pragma omp parallel for simd{{$}}
//    PRT-O-G-NEXT:     {{^ *}}#pragma omp simd{{$}}
//  PRT-O-GWV-NEXT:     {{^ *}}#pragma omp parallel for simd{{$}}
//     PRT-OA-NEXT:     {{^ *}}// #pragma acc loop vector{{$}}
//        PRT-NEXT:     {{LOOP|for \(.*\)[[:space:]].*;$}}
//        PRT-NEXT:   });
//        PRT-NEXT: }
//
// EXE-LABEL:checkNonGangHighCallLowLoopInLambda
//  EXE-NEXT:a[0]=20
//  EXE-NEXT:a[1]=21
//  EXE-NEXT:a[2]=22
//  EXE-NEXT:a[3]=23
static void checkNonGangHighCallLowLoopInLambda() {
  callLambdaInParallel("checkNonGangHighCallLowLoopInLambda", 1, [](int *a) {
    workerFn(a);
    #pragma acc loop vector
    LOOP
  });
}

//    DMP-LABEL: FunctionDecl {{.*}} checkLowLoopNonGangHighCallInLambda 'void ()'
//          DMP:   LambdaExpr
//     DMP-NEXT:     CXXRecordDecl
//          DMP:       CXXMethodDecl
//     DMP-NEXT:         ParmVarDecl
//     DMP-NEXT:         CompoundStmt
//          DMP:           ACCLoopDirective
//     DMP-NEXT:             ACCVectorClause
//     DMP-NEXT:             ACCIndependentClause {{.*}} <implicit>
//     DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//     DMP-NEXT:               DeclRefExpr {{.*}} 'a'
// DMP-GWV-NEXT:             ACCWorkerClause {{.*}} <implicit>
//   DMP-G-NEXT:             impl: OMPSimdDirective
// DMP-GWV-NEXT:             impl: OMPParallelForSimdDirective
//      DMP-NOT:               OMP
//          DMP:               ForStmt
//          DMP:           CallExpr
//     DMP-NEXT:             ImplicitCastExpr
//     DMP-NEXT:               DeclRefExpr {{.*}} 'workerFn'
//          DMP:         ACCRoutineDeclAttr {{.*}} Implicit Worker OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//       PRT-LABEL: void checkLowLoopNonGangHighCallInLambda() {
//        PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//      PRT-A-NEXT:     {{^ *}}#pragma acc loop vector{{$}}
//   PRT-AO-G-NEXT:     {{^ *}}// #pragma omp simd{{$}}
// PRT-AO-GWV-NEXT:     {{^ *}}// #pragma omp parallel for simd{{$}}
//    PRT-O-G-NEXT:     {{^ *}}#pragma omp simd{{$}}
//  PRT-O-GWV-NEXT:     {{^ *}}#pragma omp parallel for simd{{$}}
//     PRT-OA-NEXT:     {{^ *}}// #pragma acc loop vector{{$}}
//        PRT-NEXT:     {{LOOP|for \(.*\)[[:space:]].*;$}}
//        PRT-NEXT:     workerFn(a);
//        PRT-NEXT:   });
//        PRT-NEXT: }
//
// EXE-LABEL:checkLowLoopNonGangHighCallInLambda
//  EXE-NEXT:a[0]=20
//  EXE-NEXT:a[1]=21
//  EXE-NEXT:a[2]=22
//  EXE-NEXT:a[3]=23
static void checkLowLoopNonGangHighCallInLambda() {
  callLambdaInParallel("checkLowLoopNonGangHighCallInLambda", 1, [](int *a) {
    #pragma acc loop vector
    LOOP
    workerFn(a);
  });
}

//------------------------------------------------------------------------------
// Check lambda called from a function with a routine directive.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} checkGangFnCallingGangLambda 'void ()'
//       DMP: LambdaExpr
//       DMP: LambdaExpr
//  DMP-NEXT:   CXXRecordDecl
//       DMP:     CXXMethodDecl
//  DMP-NEXT:       ParmVarDecl
//  DMP-NEXT:       CompoundStmt
//  DMP-NEXT:         CallExpr
//  DMP-NEXT:           ImplicitCastExpr
//  DMP-NEXT:             DeclRefExpr {{.*}} 'gangFn'
//       DMP:       ACCRoutineDeclAttr {{.*}} Implicit Gang OMPNodeKind=unknown DirectiveDiscardedForOMP
//
// PRT-LABEL: void checkGangFnCallingGangLambda() {
//  PRT-NEXT:   callLambdaInParallel({{.*}}) {
//  PRT-NEXT:     gangFnCallingLambda(a, [](int *a) {
//  PRT-NEXT:       gangFn(a);
//  PRT-NEXT:     });
//  PRT-NEXT:   });
//  PRT-NEXT: }
//
// EXE-LABEL:checkGangFnCallingGangLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
static void checkGangFnCallingGangLambda() {
  callLambdaInParallel("checkGangFnCallingGangLambda", NUM_GANGS, [](int *a) {
    gangFnCallingLambda(a, [](int *a) {
      gangFn(a);
    });
  });
}

// DMP-LABEL: FunctionDecl {{.*}} checkGangFnCallingWorkerLambda 'void ()'
//       DMP: LambdaExpr
//       DMP: LambdaExpr
//  DMP-NEXT:   CXXRecordDecl
//       DMP:     CXXMethodDecl
//  DMP-NEXT:       ParmVarDecl
//  DMP-NEXT:       CompoundStmt
//  DMP-NEXT:         CallExpr
//  DMP-NEXT:           ImplicitCastExpr
//  DMP-NEXT:             DeclRefExpr {{.*}} 'workerFn'
//       DMP:       ACCRoutineDeclAttr {{.*}} Implicit Worker OMPNodeKind=unknown DirectiveDiscardedForOMP
//
// PRT-LABEL: void checkGangFnCallingWorkerLambda() {
//  PRT-NEXT:   callLambdaInParallel({{.*}}) {
//  PRT-NEXT:     gangFnCallingLambda(a, [](int *a) {
//  PRT-NEXT:       workerFn(a);
//  PRT-NEXT:     });
//  PRT-NEXT:   });
//  PRT-NEXT: }
//
// EXE-LABEL:checkGangFnCallingWorkerLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
static void checkGangFnCallingWorkerLambda() {
  callLambdaInParallel("checkGangFnCallingWorkerLambda", 1, [](int *a) {
    gangFnCallingLambda(a, [](int *a) {
      workerFn(a);
    });
  });
}

// DMP-LABEL: FunctionDecl {{.*}} checkGangFnCallingVectorLambda 'void ()'
//       DMP: LambdaExpr
//       DMP: LambdaExpr
//  DMP-NEXT:   CXXRecordDecl
//       DMP:     CXXMethodDecl
//  DMP-NEXT:       ParmVarDecl
//  DMP-NEXT:       CompoundStmt
//  DMP-NEXT:         CallExpr
//  DMP-NEXT:           ImplicitCastExpr
//  DMP-NEXT:             DeclRefExpr {{.*}} 'vectorFn'
//       DMP:       ACCRoutineDeclAttr {{.*}} Implicit Vector OMPNodeKind=unknown DirectiveDiscardedForOMP
//
// PRT-LABEL: void checkGangFnCallingVectorLambda() {
//  PRT-NEXT:   callLambdaInParallel({{.*}}) {
//  PRT-NEXT:     gangFnCallingLambda(a, [](int *a) {
//  PRT-NEXT:       vectorFn(a);
//  PRT-NEXT:     });
//  PRT-NEXT:   });
//  PRT-NEXT: }
//
// EXE-LABEL:checkGangFnCallingVectorLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
static void checkGangFnCallingVectorLambda() {
  callLambdaInParallel("checkGangFnCallingVectorLambda", 1, [](int *a) {
    gangFnCallingLambda(a, [](int *a) {
      vectorFn(a);
    });
  });
}

// DMP-LABEL: FunctionDecl {{.*}} checkGangFnCallingSeqLambda 'void ()'
//       DMP: LambdaExpr
//       DMP: LambdaExpr
//  DMP-NEXT:   CXXRecordDecl
//       DMP:     CXXMethodDecl
//  DMP-NEXT:       ParmVarDecl
//  DMP-NEXT:       CompoundStmt
//  DMP-NEXT:         CallExpr
//  DMP-NEXT:           ImplicitCastExpr
//  DMP-NEXT:             DeclRefExpr {{.*}} 'seqFn'
//       DMP:       ACCRoutineDeclAttr {{.*}} Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP
//
// PRT-LABEL: void checkGangFnCallingSeqLambda() {
//  PRT-NEXT:   callLambdaInParallel({{.*}}) {
//  PRT-NEXT:     gangFnCallingLambda(a, [](int *a) {
//  PRT-NEXT:       seqFn(a);
//  PRT-NEXT:     });
//  PRT-NEXT:   });
//  PRT-NEXT: }
//
// EXE-LABEL:checkGangFnCallingSeqLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
static void checkGangFnCallingSeqLambda() {
  callLambdaInParallel("checkGangFnCallingSeqLambda", 1, [](int *a) {
    gangFnCallingLambda(a, [](int *a) {
      seqFn(a);
    });
  });
}

// DMP-LABEL: FunctionDecl {{.*}} checkWorkerFnCallingWorkerLambda 'void ()'
//       DMP: LambdaExpr
//       DMP: LambdaExpr
//  DMP-NEXT:   CXXRecordDecl
//       DMP:     CXXMethodDecl
//  DMP-NEXT:       ParmVarDecl
//  DMP-NEXT:       CompoundStmt
//  DMP-NEXT:         CallExpr
//  DMP-NEXT:           ImplicitCastExpr
//  DMP-NEXT:             DeclRefExpr {{.*}} 'workerFn'
//       DMP:       ACCRoutineDeclAttr {{.*}} Implicit Worker OMPNodeKind=unknown DirectiveDiscardedForOMP
//
// PRT-LABEL: void checkWorkerFnCallingWorkerLambda() {
//  PRT-NEXT:   callLambdaInParallel({{.*}}) {
//  PRT-NEXT:     workerFnCallingLambda(a, [](int *a) {
//  PRT-NEXT:       workerFn(a);
//  PRT-NEXT:     });
//  PRT-NEXT:   });
//  PRT-NEXT: }
//
// EXE-LABEL:checkWorkerFnCallingWorkerLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
static void checkWorkerFnCallingWorkerLambda() {
  callLambdaInParallel("checkWorkerFnCallingWorkerLambda", 1, [](int *a) {
    workerFnCallingLambda(a, [](int *a) {
      workerFn(a);
    });
  });
}

// DMP-LABEL: FunctionDecl {{.*}} checkVectorFnCallingVectorLambda 'void ()'
//       DMP: LambdaExpr
//       DMP: LambdaExpr
//  DMP-NEXT:   CXXRecordDecl
//       DMP:     CXXMethodDecl
//  DMP-NEXT:       ParmVarDecl
//  DMP-NEXT:       CompoundStmt
//  DMP-NEXT:         CallExpr
//  DMP-NEXT:           ImplicitCastExpr
//  DMP-NEXT:             DeclRefExpr {{.*}} 'vectorFn'
//       DMP:       ACCRoutineDeclAttr {{.*}} Implicit Vector OMPNodeKind=unknown DirectiveDiscardedForOMP
//
// PRT-LABEL: void checkVectorFnCallingVectorLambda() {
//  PRT-NEXT:   callLambdaInParallel({{.*}}) {
//  PRT-NEXT:     vectorFnCallingLambda(a, [](int *a) {
//  PRT-NEXT:       vectorFn(a);
//  PRT-NEXT:     });
//  PRT-NEXT:   });
//  PRT-NEXT: }
//
// EXE-LABEL:checkVectorFnCallingVectorLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
static void checkVectorFnCallingVectorLambda() {
  callLambdaInParallel("checkVectorFnCallingVectorLambda", 1, [](int *a) {
    vectorFnCallingLambda(a, [](int *a) {
      vectorFn(a);
    });
  });
}

// DMP-LABEL: FunctionDecl {{.*}} checkSeqFnCallingSeqLambda 'void ()'
//       DMP: LambdaExpr
//       DMP: LambdaExpr
//  DMP-NEXT:   CXXRecordDecl
//       DMP:     CXXMethodDecl
//  DMP-NEXT:       ParmVarDecl
//  DMP-NEXT:       CompoundStmt
//  DMP-NEXT:         CallExpr
//  DMP-NEXT:           ImplicitCastExpr
//  DMP-NEXT:             DeclRefExpr {{.*}} 'seqFn'
//       DMP:       ACCRoutineDeclAttr {{.*}} Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP
//
// PRT-LABEL: void checkSeqFnCallingSeqLambda() {
//  PRT-NEXT:   callLambdaInParallel({{.*}}) {
//  PRT-NEXT:     seqFnCallingLambda(a, [](int *a) {
//  PRT-NEXT:       seqFn(a);
//  PRT-NEXT:     });
//  PRT-NEXT:   });
//  PRT-NEXT: }
//
// EXE-LABEL:checkSeqFnCallingSeqLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
static void checkSeqFnCallingSeqLambda() {
  callLambdaInParallel("checkSeqFnCallingSeqLambda", 1, [](int *a) {
    seqFnCallingLambda(a, [](int *a) {
      seqFn(a);
    });
  });
}

//------------------------------------------------------------------------------
// Check lambda called from another lambda.  Both lambdas have implicit routine
// directives, and the former's call implies the latter's routine directive.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} checkLambdaCallInLambda 'void ()'
//       DMP: LambdaExpr
//  DMP-NEXT:   CXXRecordDecl
//       DMP:     CXXMethodDecl
//  DMP-NEXT:       ParmVarDecl
//  DMP-NEXT:       CompoundStmt
//       DMP:         CXXOperatorCallExpr
//       DMP:           LambdaExpr
//  DMP-NEXT:             CXXRecordDecl
//       DMP:               CXXMethodDecl
//  DMP-NEXT:                 CompoundStmt
//  DMP-NEXT:                   CallExpr
//  DMP-NEXT:                     ImplicitCastExpr
//  DMP-NEXT:                       DeclRefExpr {{.*}} 'gangFn'
//       DMP:                 ACCRoutineDeclAttr {{.*}} Implicit Gang OMPNodeKind=unknown DirectiveDiscardedForOMP
//       DMP:       ACCRoutineDeclAttr {{.*}} Implicit Gang OMPNodeKind=unknown DirectiveDiscardedForOMP
//
// PRT-LABEL: void checkLambdaCallInLambda() {
//  PRT-NEXT:   callLambdaInParallel("{{.*}}", {{.*}}, [](int *a) {
//  PRT-NEXT:     [a]() {
//  PRT-NEXT:       gangFn(a);
//  PRT-NEXT:     }();
//  PRT-NEXT:   });
//  PRT-NEXT: }
//
// EXE-LABEL:checkLambdaCallInLambda
//  EXE-NEXT:a[0]=10
//  EXE-NEXT:a[1]=11
//  EXE-NEXT:a[2]=12
//  EXE-NEXT:a[3]=13
static void checkLambdaCallInLambda() {
  callLambdaInParallel("checkLambdaCallInLambda", NUM_GANGS, [](int *a) {
    [a]() {
      gangFn(a);
    }();
  });
}

//------------------------------------------------------------------------------
// Check that a lambda can be defined and called in a parallel construct.
//
// FIXME: This test sees a runtime error when offloading, so we skip it in that
// case.  The error is like:
// - For x86_64 offload:
//
//     main.exe: /tmp/llvm/openmp/libomptarget/src/omptarget.cpp:150: int InitLibrary(DeviceTy &): Assertion `CurrDeviceEntry->size == CurrHostEntry->size && "data size mismatch"' failed.
//
// - For nvptx64 offload:
//
//     CUDA error: Loading '__omp_offloading_10305_c618f7__Z20callLambdaInParallelIZ23checkGangFnCallInLambdavE3$_0EvPKciT__l48' Failed
//     CUDA error: named symbol not found
//
// The error also occurs when compiling the OpenMP equivalent code directly, so
// the problem appears not to be in OpenACC support.  Strangely, if we move this
// case to a separate test file, it doesn't occur, so it seems it's somehow
// interacting with the other cases in this test file during codegen.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} checkLambdaDefAndCallInAccParallel 'void ()'
//       DMP: LambdaExpr
//  DMP-NEXT:   CXXRecordDecl
//       DMP:     CXXMethodDecl
//  DMP-NEXT:       CompoundStmt
//  DMP-NEXT:         CallExpr
//  DMP-NEXT:           ImplicitCastExpr
//  DMP-NEXT:             DeclRefExpr {{.*}} 'gangFn'
//       DMP:       ACCRoutineDeclAttr {{.*}} Implicit Gang OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//   PRT-LABEL: void checkLambdaDefAndCallInAccParallel() {
//    PRT-NEXT:   printf
//    PRT-NEXT:   int a[{{.*}}];
//    PRT-NEXT:   initArray(a);
//  PRT-A-NEXT:   {{^ *}}#pragma acc parallel num_gangs({{.*}}){{$}}
// PRT-AO-NEXT:   {{^ *}}// #pragma omp target teams num_teams({{.*}}) map(ompx_hold,tofrom: a){{$}}
//  PRT-O-NEXT:   {{^ *}}#pragma omp target teams num_teams({{.*}}) map(ompx_hold,tofrom: a){{$}}
// PRT-OA-NEXT:   {{^ *}}// #pragma acc parallel num_gangs({{.*}}){{$}}
//    PRT-NEXT:   {
//    PRT-NEXT:     [&]() {
//    PRT-NEXT:       gangFn(a);
//    PRT-NEXT:     }();
//    PRT-NEXT:   }
//    PRT-NEXT:   printArray(a);
//    PRT-NEXT: }
//
// EXE-HOST-LABEL:checkLambdaDefAndCallInAccParallel
//  EXE-HOST-NEXT:a[0]=10
//  EXE-HOST-NEXT:a[1]=11
//  EXE-HOST-NEXT:a[2]=12
//  EXE-HOST-NEXT:a[3]=13
//   EXE-HOST-NOT:{{.}}
#if TGT_HOST
static void checkLambdaDefAndCallInAccParallel() {
  printf("%s\n", "checkLambdaDefAndCallInAccParallel");
  int a[A_SIZE];
  initArray(a);
  #pragma acc parallel num_gangs(NUM_GANGS)
  {
    [&]() {
      gangFn(a);
    }();
  }
  printArray(a);
}
#endif

//------------------------------------------------------------------------------
// Check that the implicit routine directive on a lambda defined within a
// template is recomputed properly during template instantiation.
//
// Specifically, the routine directive is implied by the lambda's auto loop that
// Clang must convert to seq because of the loop-carried dependency, so the
// routine directive has seq.  At one time, Clang mistakenly copied that routine
// directive instead of recomputing it for the template instantiation.  Clang
// then treated it is as an explicit routine directive (because it already
// existed) and thus reported an error as if it conflicted with the explicit
// gang/worker/vector clause on the auto loop.
//------------------------------------------------------------------------------

// DMP-LABEL: FunctionDecl {{.*}} checkAutoPartLoopInLambdaDefInTemplate_templateFn 'void (T *)'
//       DMP:   LambdaExpr
//  DMP-NEXT:     CXXRecordDecl
//       DMP:       CXXMethodDecl
//  DMP-NEXT:         CompoundStmt
//       DMP:           ACCLoopDirective
//  DMP-NEXT:             ACCAutoClause
//  DMP-NEXT:             ACCGangClause
//  DMP-NEXT:             ACCWorkerClause
//  DMP-NEXT:             ACCVectorClause
//  DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:               DeclRefExpr {{.*}} 'a'
//  DMP-NEXT:             impl: ForStmt
//       DMP:         ACCRoutineDeclAttr {{.*}} Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP
// DMP-LABEL: FunctionDecl {{.*}} checkAutoPartLoopInLambdaDefInTemplate_templateFn 'void (int *)'
//       DMP:   LambdaExpr
//  DMP-NEXT:     CXXRecordDecl
//       DMP:       CXXMethodDecl
//  DMP-NEXT:         CompoundStmt
//       DMP:           ACCLoopDirective
//  DMP-NEXT:             ACCAutoClause
//  DMP-NEXT:             ACCGangClause
//  DMP-NEXT:             ACCWorkerClause
//  DMP-NEXT:             ACCVectorClause
//  DMP-NEXT:             ACCSharedClause {{.*}} <implicit>
//  DMP-NEXT:               DeclRefExpr {{.*}} 'a'
//  DMP-NEXT:             impl: ForStmt
//       DMP:         ACCRoutineDeclAttr {{.*}} Implicit Seq OMPNodeKind=unknown DirectiveDiscardedForOMP
//
//   PRT-LABEL: void checkAutoPartLoopInLambdaDefInTemplate_templateFn(T *a) {
//    PRT-NEXT:   auto f = [=]() {
//  PRT-A-NEXT:     {{^ *}}#pragma acc loop auto gang worker vector
// PRT-AO-SAME:     {{^}} // discarded in OpenMP translation
//  PRT-A-SAME:     {{^$}}
// PRT-OA-NEXT:     {{^ *}}// #pragma acc loop auto gang worker vector // discarded in OpenMP translation{{$}}
//    PRT-NEXT:     for (int i = 1; i < {{.*}}; ++i)
//    PRT-NEXT:       a[i] += a[i - 1];
//    PRT-NEXT:   };
//    PRT-NEXT:   f();
//    PRT-NEXT: }
//
// EXE-LABEL:checkAutoPartLoopInLambdaDefInTemplate
//  EXE-NEXT:a[0]=0
//  EXE-NEXT:a[1]=1
//  EXE-NEXT:a[2]=3
//  EXE-NEXT:a[3]=6
template <typename T>
static void checkAutoPartLoopInLambdaDefInTemplate_templateFn(T *a) {
  auto f = [=]() {
    #pragma acc loop auto gang worker vector
    for (int i = 1; i < A_SIZE; ++i)
      a[i] += a[i - 1];
  };
  f();
}
static void checkAutoPartLoopInLambdaDefInTemplate() {
  printf("%s\n", "checkAutoPartLoopInLambdaDefInTemplate");
  int a[A_SIZE];
  initArray(a);
  #pragma acc parallel num_gangs(1)
  checkAutoPartLoopInLambdaDefInTemplate_templateFn(a);
  printArray(a);
}

int main() {
  checkGangFnCallInLambda();
  checkWorkerFnCallInLambda();
  checkVectorFnCallInLambda();
  checkSeqFnCallInLambda();
  checkImpSeqFnCallInLambda();
  checkSeqFnCallInHostLambda();
  checkGangLoopInLambda();
  checkWorkerLoopInLambda();
  checkVectorLoopInLambda();
  checkGangWorkerLoopInLambda();
  checkGangVectorLoopInLambda();
  checkWorkerVectorLoopInLambda();
  checkGangWorkerVectorLoopInLambda();
  checkSeqLoopInLambda();
  checkNakedLoopInLambda();
  checkAutoLoopInLambda();
  checkAutoGangWorkerVectorLoopInLambda();
  checkSerialLoopInLambda();
  checkGangHighLoopLowLoopInLambda();
  checkLowLoopGangHighLoopInLambda();
  checkNonGangHighLoopLowLoopInLambda();
  checkLowLoopNonGangHighLoopInLambda();
  checkGangHighCallLowCallInLambda();
  checkLowCallGangHighCallInLambda();
  checkNonGangHighCallLowCallInLambda();
  checkLowCallNonGangHighCallInLambda();
  checkGangHighLoopLowCallInLambda();
  checkLowCallGangHighLoopInLambda();
  checkNonGangHighLoopLowCallInLambda();
  checkLowCallNonGangHighLoopInLambda();
  checkGangHighCallLowLoopInLambda();
  checkLowLoopGangHighCallInLambda();
  checkNonGangHighCallLowLoopInLambda();
  checkLowLoopNonGangHighCallInLambda();
  checkGangFnCallingGangLambda();
  checkGangFnCallingWorkerLambda();
  checkGangFnCallingVectorLambda();
  checkGangFnCallingSeqLambda();
  checkWorkerFnCallingWorkerLambda();
  checkVectorFnCallingVectorLambda();
  checkSeqFnCallingSeqLambda();
  checkLambdaCallInLambda();
#if TGT_HOST
  checkLambdaDefAndCallInAccParallel();
#endif
  checkAutoPartLoopInLambdaDefInTemplate();
  return 0;
}
